#pragma once
#include <QJsonObject>
#include <QString>
#include <optional>
#include <stdexcept>

namespace pets::productivity {
struct Error : std::runtime_error {
    int code;
    Error(int value, const QString &message) : std::runtime_error(message.toStdString()), code(value) {}
};
class Service {
public:
    explicit Service(const QString &file);
    QString error() const { return error_; }
    QJsonObject call(const QString &operation, const QJsonObject &params, qint64 nowMs);
    void advance(qint64 nowMs);
    std::optional<qint64> nextDeadline() const;
private:
    QString file_, error_;
    QJsonObject state_;
    bool loadFailed_ = false;
    void persist(const QJsonObject &candidate);
};
}
