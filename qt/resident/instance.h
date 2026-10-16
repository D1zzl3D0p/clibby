#pragma once
#include <QLockFile>
#include <QString>

namespace pets {
class Instance final {
public:
    enum class Result { Acquired, AlreadyRunning, Error };
    explicit Instance(const QString& stateDirectory);
    Result result() const { return result_; }
    QString error() const { return error_; }
    void release();
private:
    QLockFile lock_;
    Result result_ = Result::Error;
    QString error_;
};
}
