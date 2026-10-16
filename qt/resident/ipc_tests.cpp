#include "ipc.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QThread>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void pump(int ms) {
    QElapsedTimer timer; timer.start();
    while (timer.elapsed() < ms) { QCoreApplication::processEvents(); QThread::msleep(1); }
}
QJsonObject receive(QLocalSocket& socket, int timeout = 1000) {
    QElapsedTimer timer; timer.start();
    while (!socket.canReadLine() && timer.elapsed() < timeout) pump(1);
    check(socket.canReadLine(), "response deadline");
    return QJsonDocument::fromJson(socket.readLine()).object();
}
QJsonObject request(QLocalSocket& socket, const QByteArray& bytes) {
    socket.write(bytes + '\n'); socket.flush(); return receive(socket);
}
int error(const QJsonObject& value) { return value.value("error").toObject().value("code").toInt(); }
void connect(QLocalSocket& socket, const QString& endpoint) {
    socket.connectToServer(endpoint); check(socket.waitForConnected(1000), "connect"); pump(5);
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        int dispatched = 0;
        pets::RpcProtocol protocol([&](const QString& method, const QJsonObject& params) {
            ++dispatched; return pets::RpcReply{{{"method", method}, {"params", params}}, 0, {}};
        });
        qint64 now = 0;
        auto rpc = [&](const QByteArray& bytes) { now += 100; return protocol.process(bytes, now); };
        check(error(rpc(R"({"jsonrpc":"2.0","id":1,"method":"state.get"})")) == -32001, "unit handshake required");
        check(error(rpc(R"({"jsonrpc":"2.0","id":1,"method":"hello","params":{"api_major":1.5}})")) == -32602, "unit integer version");
        check(error(rpc(R"({"jsonrpc":"2.0","id":1,"method":"hello","params":{"api_major":2}})")) == -32001, "unit incompatible version");
        check(rpc(R"({"jsonrpc":"2.0","id":"hello","method":"hello","params":{"api_major":1}})").value("id") == QJsonValue("hello"), "unit string ID echoed");
        check(rpc(R"({"jsonrpc":"2.0","id":2,"method":"state.get"})").contains("result") && dispatched == 1, "unit state dispatched");
        for (const auto& pair : QList<QPair<QByteArray,int>>{
            {"{", -32700}, {"[]", -32600}, {"null", -32600},
            {R"({"jsonrpc":"2.0","method":"state.get"})", -32600},
            {R"({"jsonrpc":"2.0","id":null,"method":"state.get"})", -32600},
            {R"({"jsonrpc":"2.0","id":9007199254740992,"method":"state.get"})", -32600},
            {R"({"jsonrpc":"1.0","id":1,"method":"state.get"})", -32600},
            {R"({"jsonrpc":"2.0","id":1,"method":"quit"})", -32601},
            {R"({"jsonrpc":"2.0","id":1,"method":"state.get","params":[]})", -32602},
            {R"({"jsonrpc":"2.0","id":1,"method":"state.get","params":{"extra":1}})", -32602},
            {QByteArray(16385, 'x'), -32006}}) {
            check(error(rpc(pair.first)) == pair.second, "unit rejection code");
        }
        QByteArray invalidUtf8 = "{\"jsonrpc\":\"2.0\",\"id\":\""; invalidUtf8.append(char(0xff)); invalidUtf8 += "\",\"method\":\"state.get\"}";
        check(error(rpc(invalidUtf8)) == -32700, "invalid UTF-8 rejected");
        const auto forwarded = rpc(R"({"jsonrpc":"2.0","id":3,"method":"session.update","params":{"source_id":"a","session_id":"b","seq":7,"status":"working","label":"test"}})");
        check(forwarded.value("result").toObject().value("params").toObject().value("seq").toInt() == 7, "typed adapter receives unchanged parameters");
        pets::RpcProtocol limited([](auto, auto) { return pets::RpcReply{}; });
        const QByteArray hello = R"({"jsonrpc":"2.0","id":1,"method":"hello","params":{"api_major":1}})";
        for (int i = 0; i < 20; ++i) check(limited.process(hello, 0).contains("result"), "burst accepts twenty");
        check(error(limited.process(hello, 0)) == -32005, "burst rejects twenty-first");
        check(limited.process(hello, 100).contains("result"), "rate replenishes after 100ms");
        check(!pets::ipcPeerIsCurrentUser(-1), "unit invalid peer denied");
        int pairDescriptors[2];
        check(::socketpair(AF_UNIX, SOCK_STREAM, 0, pairDescriptors) == 0, "peer socketpair");
        check(pets::ipcPeerIsCurrentUser(pairDescriptors[0]), "same user peer authenticated");
        ::close(pairDescriptors[0]); ::close(pairDescriptors[1]);
        QTemporaryDir privacy;
        check(!pets::ipcRuntimeDirectoryIsPrivate(privacy.path() + "/missing/s"), "missing private directory rejected");
        const auto unsafe = privacy.path() + "/unsafe";
        check(::mkdir(unsafe.toUtf8().constData(), 0755) == 0, "make unsafe directory");
        check(!pets::ipcRuntimeDirectoryIsPrivate(unsafe + "/s"), "world-accessible directory rejected");
        check(::chmod(unsafe.toUtf8().constData(), 0700) == 0, "secure directory");
        check(pets::ipcRuntimeDirectoryIsPrivate(unsafe + "/s"), "owned 0700 directory accepted");
        const auto alias = privacy.path() + "/alias";
        check(::symlink(unsafe.toUtf8().constData(), alias.toUtf8().constData()) == 0, "make symlink");
        check(!pets::ipcRuntimeDirectoryIsPrivate(alias + "/s"), "symlink runtime directory rejected");
        check(pets::ipcEndpoint(unsafe) == pets::ipcEndpoint(alias), "state directory canonicalized");
        if (app.arguments().contains("--protocol-only")) { std::cout << "Protocol tests passed\n"; return 0; }
        QTemporaryDir dir;
        // Skip only an explicitly observed OS permission denial, never a generic
        // listen/startup failure. Native transport acceptance stays outstanding.
        int descriptor = ::socket(AF_UNIX, SOCK_STREAM, 0);
        check(descriptor >= 0, "bind probe socket");
        sockaddr_un address{}; address.sun_family = AF_UNIX;
        const auto probePath = (dir.path() + "/probe").toUtf8();
        check(probePath.size() < int(sizeof(address.sun_path)), "probe path");
        std::memcpy(address.sun_path, probePath.constData(), probePath.size() + 1);
        int bound = ::bind(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address));
        int bindError = errno; ::close(descriptor);
        if (bound != 0 && bindError == EPERM) { std::cout << "SKIP: OS Unix bind denied with EPERM; actual sockets unverified\n"; return 77; }
        check(bound == 0, "OS bind probe failed"); ::unlink(probePath.constData());
        int calls = 0;
        pets::IpcServer server(dir.path(), [&](const QString& method, const QJsonObject& params) {
            ++calls; return pets::RpcReply{{{"method", method}, {"params", params}}, 0, {}};
        });
        check(server.endpoint().toUtf8().size() < 104, "short endpoint");
        check(pets::ipcRuntimeDirectoryIsPrivate(server.endpoint()), "private runtime directory");
        check(!pets::ipcPeerIsCurrentUser(-1), "invalid peer fails closed");
        struct stat info{}; check(lstat(server.endpoint().toUtf8().constData(), &info) == 0 && (info.st_mode & 0777) == 0600, "socket mode");
        QLocalSocket client; connect(client, server.endpoint());
        check(error(request(client, R"({"jsonrpc":"2.0","id":1,"method":"state.get"})")) == -32001, "handshake required");
        check(error(request(client, R"({"jsonrpc":"2.0","id":2,"method":"hello","params":{"api_major":2}})")) == -32001, "incompatible version");
        check(request(client, R"({"jsonrpc":"2.0","id":"hi","method":"hello","params":{"api_major":1}})").value("result").toObject().value("api_major").toInt() == 1, "hello result");
        check(request(client, R"({"jsonrpc":"2.0","id":3,"method":"state.get"})").contains("result") && calls == 1, "dispatch state");
        check(error(request(client, "{")) == -32700, "invalid JSON");
        check(error(request(client, "[]")) == -32600, "batches rejected");
        check(error(request(client, R"({"jsonrpc":"2.0","method":"state.get"})")) == -32600, "notifications rejected");
        check(error(request(client, R"({"jsonrpc":"2.0","id":null,"method":"state.get"})")) == -32600, "null ID rejected");
        check(error(request(client, R"({"jsonrpc":"2.0","id":9007199254740992,"method":"state.get"})")) == -32600, "unsafe ID rejected");
        check(error(request(client, R"({"jsonrpc":"2.0","id":4,"method":"quit"})")) == -32601, "method allowlist");
        check(error(request(client, R"({"jsonrpc":"2.0","id":5,"method":"state.get","params":[]})")) == -32602, "object params");
        check(error(request(client, R"({"jsonrpc":"2.0","id":6,"method":"state.get","params":{"extra":1}})")) == -32602, "empty state params");
        QLocalSocket oversized; connect(oversized, server.endpoint());
        oversized.write(QByteArray(16385, 'x')); oversized.flush();
        check(error(receive(oversized)) == -32006, "oversize rejected before newline");
        QLocalSocket incomplete; connect(incomplete, server.endpoint()); incomplete.write("{"); incomplete.flush();
        check(error(receive(incomplete, 5600)) == -32600, "incomplete deadline");
        QLocalSocket flood; connect(flood, server.endpoint());
        QByteArray burst;
        for (int i = 0; i < 25; ++i) burst += "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"hello\",\"params\":{\"api_major\":1}}\n";
        flood.write(burst); flood.flush(); bool floodLimited = false;
        for (int i = 0; i < 25; ++i) floodLimited |= error(receive(flood)) == -32005;
        check(floodLimited, "burst bounded");
        bool liveRefused = false;
        try { pets::IpcServer second(dir.path(), [](auto, auto) { return pets::RpcReply{}; }); }
        catch (const std::exception&) { liveRefused = true; }
        check(liveRefused, "live endpoint preserved");
        QLocalSocket after; connect(after, server.endpoint());
        check(request(after, R"({"jsonrpc":"2.0","id":1,"method":"hello","params":{"api_major":1}})").contains("result"), "server responsive after flood");
        std::cout << "IPC tests passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
