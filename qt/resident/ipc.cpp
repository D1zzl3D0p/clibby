#include "ipc.h"
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QQueue>
#include <QTimer>
#include <cmath>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>

namespace pets {
namespace {
constexpr int frameLimit = 16 * 1024;
constexpr int txLimit = 64 * 1024;
constexpr double safeInteger = 9007199254740991.0;
const QStringList productivityMethods{
    "productivity.publish", "productivity.list", "productivity.act",
    "productivity.timer_create", "productivity.timer_act", "productivity.timers",
    "productivity.capabilities"};
bool integer(const QJsonValue& value) {
    return value.isDouble() && std::isfinite(value.toDouble()) &&
           std::floor(value.toDouble()) == value.toDouble();
}
bool validId(const QJsonValue& value) {
    return (value.isString() && value.toString().toUcs4().size() <= 64) ||
           (integer(value) && std::abs(value.toDouble()) <= safeInteger);
}
QJsonObject response(const QJsonValue& id, int code, const QString& message,
                     const QJsonObject& result = {}) {
    QJsonObject value{{"jsonrpc", "2.0"}, {"id", id}};
    if (code) value.insert("error", QJsonObject{{"code", code}, {"message", message}});
    else value.insert("result", result);
    return value;
}
bool privateDirectory(const QString& path) {
    struct stat info{};
    const auto encoded = QFile::encodeName(path);
    return lstat(encoded.constData(), &info) == 0 && S_ISDIR(info.st_mode) &&
           info.st_uid == geteuid() && (info.st_mode & 0777) == 0700;
}
void ensureDirectory(const QString& path) {
    const auto encoded = QFile::encodeName(path);
    if (mkdir(encoded.constData(), 0700) != 0 && errno != EEXIST)
        throw std::runtime_error("cannot create private IPC directory");
    if (!privateDirectory(path))
        throw std::runtime_error("IPC directory must be an owned, non-symlink directory with mode 0700");
}
}
QString ipcEndpoint(const QString& stateDirectory) {
    if (!QDir::isAbsolutePath(stateDirectory)) throw std::runtime_error("IPC state directory must be absolute");
    QString identity = QFileInfo(stateDirectory).canonicalFilePath();
    if (identity.isEmpty()) identity = QDir::cleanPath(stateDirectory);
    const auto digest = QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    const QString endpoint = QDir::tempPath() + "/bp-" + QString::number(geteuid()) + "-" + digest + "/s";
    if (QFile::encodeName(endpoint).size() >= 104)
        throw std::runtime_error("IPC endpoint exceeds Unix socket path limit");
    return endpoint;
}
bool ipcRuntimeDirectoryIsPrivate(const QString& endpoint) {
    return privateDirectory(QFileInfo(endpoint).absolutePath());
}
bool ipcPeerIsCurrentUser(qintptr descriptor) {
    if (descriptor < 0) return false;
#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    uid_t uid; gid_t gid;
    return getpeereid(static_cast<int>(descriptor), &uid, &gid) == 0 && uid == geteuid();
#elif defined(Q_OS_LINUX)
    struct ucred credentials{}; socklen_t size = sizeof(credentials);
    return getsockopt(static_cast<int>(descriptor), SOL_SOCKET, SO_PEERCRED, &credentials, &size) == 0 &&
           size == sizeof(credentials) && credentials.uid == geteuid();
#else
    return false;
#endif
}
RpcProtocol::RpcProtocol(std::function<RpcReply(const QString&, const QJsonObject&)> handler)
    : handler_(std::move(handler)) {}
QJsonObject RpcProtocol::process(const QByteArray& bytes, qint64 nowMilliseconds) {
        if (bytes.size() > frameLimit) return response(QJsonValue::Null, -32006, "frame exceeds 16384 bytes");
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(bytes, &parseError);
        QJsonValue id = QJsonValue::Null;
        auto fail = [&](int code, const QString& message) { return response(id, code, message); };
        const double elapsed = lastMilliseconds_ < 0 ? 0 : qMax<qint64>(0, nowMilliseconds - lastMilliseconds_) / 1000.0;
        lastMilliseconds_ = nowMilliseconds;
        tokens_ = qMin(20.0, tokens_ + elapsed * 10.0);
        if (doc.isObject() && validId(doc.object().value("id"))) id = doc.object().value("id");
        if (tokens_ < 1) { return fail(-32005, "request rate exceeded"); }
        tokens_ -= 1;
        if (QString::fromUtf8(bytes).toUtf8() != bytes) return fail(-32700, "invalid UTF-8 JSON");
        if (parseError.error != QJsonParseError::NoError) {
            // QJsonDocument accepts only arrays/objects; a valid JSON scalar is
            // an invalid RPC envelope rather than a JSON syntax error.
            QJsonParseError scalarError;
            const auto scalar = QJsonDocument::fromJson("[" + bytes + "]", &scalarError);
            if (scalarError.error == QJsonParseError::NoError && scalar.isArray() && scalar.array().size() == 1)
                return fail(-32600, "request must be one object");
            return fail(-32700, "invalid JSON");
        }
        if (!doc.isObject()) { return fail(-32600, "request must be one object"); }
        const auto request = doc.object();
        if (request.value("jsonrpc") != QJsonValue("2.0") || !validId(request.value("id")) || !request.value("method").isString()) {
            id = QJsonValue::Null; return fail(-32600, "invalid JSON-RPC envelope");
        }
        const auto method = request.value("method").toString();
        if (method != "hello" && method != "session.update" && method != "session.clear" && method != "state.get" && !productivityMethods.contains(method)) {
            return fail(-32601, "unknown method");
        }
        if (request.contains("params") && !request.value("params").isObject()) { return fail(-32602, "params must be an object"); }
        const auto params = request.value("params").toObject();
        if (method == "hello") {
            if (!integer(params.value("api_major"))) { return fail(-32602, "api_major must be an integer"); }
            if (params.value("api_major").toDouble() != 1) { return fail(-32001, "unsupported API major"); }
            handshaken_ = true;
            QJsonArray methods{"hello", "session.update", "session.clear", "state.get"};
            for (const auto &name : productivityMethods) methods.append(name);
            return response(id, 0, {}, {
                {"api_major", 1},
                {"methods", methods},
                {"statuses", QJsonArray{"working", "needs_input", "completed", "failed"}},
                {"limits", QJsonObject{{"frame_bytes", frameLimit}, {"clients", 16}, {"session_records", 32},
                    {"pending_requests", 64}, {"tx_bytes", txLimit}, {"incomplete_frame_seconds", 5},
                    {"requests_per_second", 10}, {"burst", 20}, {"id_characters", 64}, {"label_characters", 120},
                    {"ttl_default", 120}, {"ttl_max", 3600}}}
            });
        }
        if (!handshaken_) { return fail(-32001, "hello is required on each connection"); }
        if (method == "state.get" && !params.isEmpty()) { return fail(-32602, "state.get takes no parameters"); }
        const auto reply = handler_(method, params);
        return response(id, reply.errorCode, reply.errorMessage, reply.result);
    }
struct IpcServer::Impl {
    struct Connection : QObject {
        QLocalSocket* socket;
        QTimer deadline;
        QByteArray buffer;
        QElapsedTimer rateClock;
        RpcProtocol protocol;
        bool scheduled = false;
        bool closing = false;
        explicit Connection(QLocalSocket* value, std::function<RpcReply(const QString&, const QJsonObject&)> callback, QObject* parent) : QObject(parent), socket(value), protocol(std::move(callback)) {
            socket->setParent(this); deadline.setSingleShot(true); rateClock.start();
            socket->setReadBufferSize(frameLimit + 1);
        }
    };
    IpcServer* owner;
    QString path;
    QLocalServer server;
    std::function<RpcReply(const QString&, const QJsonObject&)> handler;
    QHash<QLocalSocket*, Connection*> connections;
    QQueue<QPair<QPointer<Connection>, QByteArray>> pending;
    bool drainScheduled = false;
    Impl(IpcServer* parent, const QString& state, decltype(handler) callback)
        : owner(parent), path(ipcEndpoint(state)), handler(std::move(callback)) {
        ensureDirectory(QFileInfo(path).absolutePath());
        struct stat info{}; const auto encoded = QFile::encodeName(path);
        if (lstat(encoded.constData(), &info) == 0) {
            if (!S_ISSOCK(info.st_mode) || info.st_uid != geteuid())
                throw std::runtime_error("existing IPC endpoint is not an owned socket");
            QLocalSocket probe; probe.connectToServer(path);
            if (probe.waitForConnected(100)) throw std::runtime_error("IPC endpoint is already live");
            if (probe.error() != QLocalSocket::ConnectionRefusedError && probe.error() != QLocalSocket::ServerNotFoundError)
                throw std::runtime_error("cannot safely determine whether IPC endpoint is stale");
            // Startup is serialized by the application instance lock.
            if (unlink(encoded.constData()) != 0 && errno != ENOENT)
                throw std::runtime_error("cannot remove stale IPC endpoint");
        } else if (errno != ENOENT) throw std::runtime_error("cannot inspect IPC endpoint");
        server.setMaxPendingConnections(16);
        server.setSocketOptions(QLocalServer::UserAccessOption);
        if (!server.listen(path)) throw std::runtime_error("cannot listen on IPC endpoint: " + server.errorString().toStdString());
        if (chmod(encoded.constData(), 0600) != 0) { server.close(); throw std::runtime_error("cannot secure IPC endpoint"); }
        QObject::connect(&server, &QLocalServer::newConnection, owner, [this] { accept(); });
    }
    ~Impl() {
        QObject::disconnect(&server, nullptr, owner, nullptr);
        for (auto* connection : connections) {
            QObject::disconnect(connection->socket, nullptr, owner, nullptr);
            connection->socket->abort();
            delete connection;
        }
        connections.clear(); server.close();
    }
    void send(Connection* c, const QJsonObject& object, bool close = false) {
        if (c->closing) return;
        QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
        if (bytes.size() + c->socket->bytesToWrite() > txLimit) { c->closing = true; c->socket->abort(); return; }
        c->socket->write(bytes);
        if (close) { c->closing = true; c->deadline.stop(); c->socket->disconnectFromServer(); }
    }
    void accept() {
        // Qt's pending limit and this active limit bound descriptors and storage.
        while (server.hasPendingConnections()) {
            auto* socket = server.nextPendingConnection();
            if (connections.size() >= 16 || !ipcPeerIsCurrentUser(socket->socketDescriptor())) {
                const int code = connections.size() >= 16 ? -32004 : -32002;
                socket->write(QJsonDocument(response(QJsonValue::Null, code, "connection denied")).toJson(QJsonDocument::Compact) + '\n');
                socket->disconnectFromServer(); socket->deleteLater(); continue;
            }
            auto* c = new Connection(socket, handler, owner); connections.insert(socket, c);
            QObject::connect(socket, &QLocalSocket::readyRead, c, [this, c] { read(c); });
            QObject::connect(socket, &QLocalSocket::disconnected, owner, [this, socket, c] {
                connections.remove(socket); c->closing = true; c->deadline.stop(); c->deleteLater();
            });
            QObject::connect(&c->deadline, &QTimer::timeout, c, [this, c] {
                send(c, response(QJsonValue::Null, -32600, "incomplete frame exceeded five seconds"), true);
            });
            if (socket->bytesAvailable()) read(c);
        }
    }
    void scheduleRead(Connection* c) {
        if (c->scheduled || c->closing) return;
        c->scheduled = true;
        QTimer::singleShot(0, c, [this, c] { c->scheduled = false; read(c); });
    }
    void read(Connection* c) {
        if (c->closing) return;
        int budget = frameLimit;
        int frames = 0;
        while (frames < 8 && budget > 0 && !c->closing) {
            int newline = c->buffer.indexOf('\n');
            if (newline >= 0) {
                if (newline > frameLimit) { send(c, response(QJsonValue::Null, -32006, "frame exceeds 16384 bytes"), true); return; }
                auto frame = c->buffer.left(newline); c->buffer.remove(0, newline + 1);
                ++frames;
                if (pending.size() >= 64) send(c, response(QJsonValue::Null, -32005, "global request queue is full"));
                else pending.enqueue({QPointer<Connection>(c), std::move(frame)});
                c->deadline.stop();
                if (!c->buffer.isEmpty()) c->deadline.start(5000);
                continue;
            }
            if (c->buffer.size() > frameLimit) { send(c, response(QJsonValue::Null, -32006, "frame exceeds 16384 bytes"), true); return; }
            if (!c->socket->bytesAvailable()) break;
            const qint64 count = qMin<qint64>(budget, frameLimit + 1 - c->buffer.size());
            const auto bytes = c->socket->read(count);
            if (bytes.isEmpty()) break;
            if (c->buffer.isEmpty()) c->deadline.start(5000);
            c->buffer += bytes; budget -= bytes.size();
        }
        if (!drainScheduled && !pending.isEmpty()) {
            drainScheduled = true; QTimer::singleShot(0, owner, [this] { drain(); });
        }
        if (!c->closing && (c->socket->bytesAvailable() || c->buffer.contains('\n') || c->buffer.size() > frameLimit)) scheduleRead(c);
    }
    void drain() {
        drainScheduled = false;
        for (int i = 0; i < 8 && !pending.isEmpty(); ++i) {
            auto entry = pending.dequeue();
            if (entry.first && !entry.first->closing) dispatch(entry.first, entry.second);
        }
        if (!pending.isEmpty()) { drainScheduled = true; QTimer::singleShot(0, owner, [this] { drain(); }); }
    }
    void dispatch(Connection* c, const QByteArray& bytes) {
        send(c, c->protocol.process(bytes, c->rateClock.elapsed()));
    }
};
IpcServer::IpcServer(const QString& state, std::function<RpcReply(const QString&, const QJsonObject&)> handler, QObject* parent)
    : QObject(parent), impl_(std::make_unique<Impl>(this, state, std::move(handler))) {}
IpcServer::~IpcServer() = default;
QString IpcServer::endpoint() const { return impl_->path; }
}
