#pragma once
#include <QObject>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <memory>

namespace pets {
struct RpcReply {
    QJsonObject result;
    int errorCode = 0;
    QString errorMessage;
};
QString ipcEndpoint(const QString& stateDirectory);
// Both server and client fail closed if peer credentials cannot be verified.
bool ipcPeerIsCurrentUser(qintptr descriptor);
bool ipcRuntimeDirectoryIsPrivate(const QString& endpoint);
// Per-connection protocol state; time is monotonic milliseconds. Separated from
// OS transport so negotiation/validation can be tested where bind is denied.
class RpcProtocol {
public:
    explicit RpcProtocol(std::function<RpcReply(const QString&, const QJsonObject&)> handler);
    QJsonObject process(const QByteArray& frame, qint64 nowMilliseconds);
private:
    std::function<RpcReply(const QString&, const QJsonObject&)> handler_;
    bool handshaken_ = false;
    double tokens_ = 20;
    qint64 lastMilliseconds_ = -1;
};
class IpcServer : public QObject {
public:
    IpcServer(const QString& stateDirectory,
              std::function<RpcReply(const QString&, const QJsonObject&)> handler,
              QObject* parent = nullptr);
    ~IpcServer() override;
    QString endpoint() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
