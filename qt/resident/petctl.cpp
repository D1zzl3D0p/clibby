#include "ipc.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <iostream>
#include <stdexcept>

namespace {
class ConnectionFailure : public std::runtime_error { using std::runtime_error::runtime_error; };
const char* help =
    "Usage: petctl [--state-dir ABS_DIR] COMMAND [OPTIONS]\n"
    "Commands: update, clear, state\n"
    "update --source ID --session ID --label TEXT --seq N --status STATUS [--ttl SECONDS] [--source-url URL]\n"
    "clear  --source ID --session ID --seq N\n"
    "state  (no command options)\n"
    "Statuses: working, needs_input, completed, failed. TTL defaults to 120 (1..3600).\n"
    "IDs: 1..64 ASCII letters/digits/._-. Sequence: 0..9007199254740991.\n"
    "--state-dir may appear anywhere; defaults to Better Pets per-user app data.\n"
    "Successful command result is compact JSON on stdout; RPC errors are JSON envelopes.\n"
    "Exit codes: 0 success/help, 2 usage, 3 connection/timeout, 4 RPC rejection.\n";
qint64 number(const QString& text, qint64 maximum, const char* name, qint64 minimum = 0) {
    if (!QRegularExpression("\\A[0-9]+\\z").match(text).hasMatch()) throw std::runtime_error(std::string(name) + " must be an integer");
    bool ok; auto value = text.toLongLong(&ok);
    if (!ok || value < minimum || value > maximum) throw std::runtime_error(std::string(name) + " is outside range");
    return value;
}
QJsonObject transact(QLocalSocket& socket, const QString& method, const QJsonObject& params, int id) {
    const auto bytes = QJsonDocument(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}}).toJson(QJsonDocument::Compact) + '\n';
    if (socket.write(bytes) != bytes.size()) throw ConnectionFailure("cannot write request");
    socket.flush();
    QElapsedTimer timer; timer.start();
    QByteArray input;
    while (true) {
        const int newline = input.indexOf('\n');
        if (newline >= 0) {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(input.left(newline), &error);
            if (error.error != QJsonParseError::NoError || !doc.isObject() || doc.object().value("jsonrpc") != QJsonValue("2.0") ||
                doc.object().value("id") != QJsonValue(id) ||
                (doc.object().contains("result") == doc.object().contains("error"))) throw ConnectionFailure("invalid server response");
            return doc.object();
        }
        if (input.size() >= 65536) throw ConnectionFailure("server response exceeds limit");
        if (socket.bytesAvailable()) input += socket.read(65536 - input.size());
        else {
            int remaining = 1500 - static_cast<int>(timer.elapsed());
            if (remaining <= 0 || !socket.waitForReadyRead(remaining)) throw ConnectionFailure("server response timed out or disconnected");
        }
    }
}
void output(const QJsonObject& value) { std::cout << QJsonDocument(value).toJson(QJsonDocument::Compact).constData() << '\n'; }
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("Better Pets"); app.setOrganizationName("Better Pets");
    try {
        QString state, command;
        QMap<QString, QString> options;
        for (int i = 1; i < argc; ++i) {
            const auto arg = QString::fromLocal8Bit(argv[i]);
            if (arg == "--help" || arg == "-h") { std::cout << help; return 0; }
            if (arg == "--state-dir") {
                if (++i >= argc) throw std::runtime_error("missing state directory");
                state = QString::fromLocal8Bit(argv[i]);
                if (!QDir::isAbsolutePath(state)) throw std::runtime_error("state directory must be absolute");
            } else if (arg.startsWith("--")) {
                const auto key = arg.mid(2);
                if (!QStringList{"source", "session", "label", "seq", "status", "ttl", "source-url"}.contains(key)) throw std::runtime_error("unknown option");
                if (++i >= argc || options.contains(key)) throw std::runtime_error("missing or duplicate option");
                options[key] = QString::fromLocal8Bit(argv[i]);
            } else {
                if (!command.isEmpty()) throw std::runtime_error("unexpected argument");
                command = arg;
            }
        }
        if (!QStringList{"update", "clear", "state"}.contains(command)) throw std::runtime_error("command must be update, clear, or state");
        QStringList required, allowed;
        if (command == "update") { required = {"source", "session", "label", "seq", "status"}; allowed = required; allowed << "ttl" << "source-url"; }
        if (command == "clear") { required = {"source", "session", "seq"}; allowed = required; }
        for (const auto& key : required) if (!options.contains(key)) throw std::runtime_error("missing --" + key.toStdString());
        for (auto it = options.begin(); it != options.end(); ++it) if (!allowed.contains(it.key())) throw std::runtime_error("option not valid for command");
        QJsonObject params;
        if (command != "state") {
            const QRegularExpression pattern("\\A[A-Za-z0-9._-]{1,64}\\z");
            for (const auto& key : QStringList{"source", "session"}) {
                if (!pattern.match(options[key]).hasMatch()) throw std::runtime_error("invalid source/session ID");
                params.insert(key + "_id", options[key]);
            }
            params.insert("seq", static_cast<double>(number(options["seq"], 9007199254740991LL, "seq")));
        }
        if (command == "update") {
            const auto label = options["label"];
            if (label.toUcs4().size() > 120) throw std::runtime_error("label exceeds 120 Unicode characters");
            for (QChar ch : label) if (ch.category() == QChar::Other_Control) throw std::runtime_error("label contains control characters");
            if (!QStringList{"working", "needs_input", "completed", "failed"}.contains(options["status"])) throw std::runtime_error("invalid status");
    const auto validSourceUrl = [](const QString &text) {
      if (text.isEmpty()) return true;
      if (text.toUtf8().size() > 2048) return false;
      const auto decoded = QUrl::fromPercentEncoding(text.toUtf8());
      for (QChar ch : decoded)
        if (ch.category() == QChar::Other_Control) return false;
      const QUrl url(text, QUrl::StrictMode);
      if (!url.isValid() || url.isRelative() || url.authority(QUrl::FullyEncoded).contains('@')) return false;
      const auto scheme = url.scheme().toLower();
      if (scheme == "https" || scheme == "http") return !url.host().isEmpty();
      return scheme == "file" && url.authority().isEmpty() && url.isLocalFile()
          && QDir::isAbsolutePath(url.toLocalFile());
    };
            if (options.contains("source-url")) {
                if (!validSourceUrl(options["source-url"])) throw std::runtime_error("invalid source URL");
                params.insert("source_url", options["source-url"]);
            }
            params.insert("label", label); params.insert("status", options["status"]);
            if (options.contains("ttl")) params.insert("ttl", static_cast<double>(number(options["ttl"], 3600, "ttl", 1)));
        }
        if (state.isEmpty()) state = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString endpoint;
        try { endpoint = pets::ipcEndpoint(state); } catch (const std::exception& e) { throw ConnectionFailure(e.what()); }
        if (!pets::ipcRuntimeDirectoryIsPrivate(endpoint)) throw ConnectionFailure("IPC runtime directory is missing or unsafe");
        QLocalSocket socket; socket.setReadBufferSize(65536); socket.connectToServer(endpoint);
        if (!socket.waitForConnected(1500)) throw ConnectionFailure("cannot connect to resident: " + socket.errorString().toStdString());
        if (!pets::ipcPeerIsCurrentUser(socket.socketDescriptor())) throw ConnectionFailure("resident peer credentials rejected");
        auto reply = transact(socket, "hello", {{"api_major", 1}}, 1);
        if (reply.contains("error")) { output(reply); return 4; }
        if (reply.value("result").toObject().value("api_major") != QJsonValue(1)) throw ConnectionFailure("unsupported resident API major");
        reply = transact(socket, command == "state" ? "state.get" : "session." + command, params, 2);
        if (reply.contains("error")) { output(reply); return 4; }
        if (!reply.value("result").isObject()) throw ConnectionFailure("invalid command result");
        output(reply.value("result").toObject()); return 0;
    } catch (const ConnectionFailure& e) { std::cerr << "petctl: " << e.what() << '\n'; return 3;
    } catch (const std::exception& e) { std::cerr << "petctl: " << e.what() << '\n'; return 2; }
}
