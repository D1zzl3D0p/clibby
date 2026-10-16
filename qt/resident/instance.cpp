#include "instance.h"
#include <QDir>
namespace pets {
Instance::Instance(const QString& directory) : lock_(directory + "/instance.lock") {
    if (!QDir::isAbsolutePath(directory) || !QDir().mkpath(directory)) {
        error_ = "cannot create absolute state directory: " + directory;
        return;
    }
    lock_.setStaleLockTime(0); // Process liveness, not age, controls long-lived locks.
    if (lock_.tryLock(0)) result_ = Result::Acquired;
    else if (lock_.error() == QLockFile::LockFailedError) result_ = Result::AlreadyRunning;
    else error_ = "cannot acquire instance lock in " + directory + " (error " + QString::number(lock_.error()) + ")";
}
void Instance::release() { lock_.unlock(); }
}
