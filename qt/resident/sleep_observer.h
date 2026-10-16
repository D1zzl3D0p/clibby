#pragma once
#include "runtime.h"
#include <functional>
#include <memory>
class QObject;
namespace pets {
class SleepObserver {
public: virtual ~SleepObserver() = default;
};
std::unique_ptr<SleepObserver> observeSleep(QObject* context, std::function<void(Control)> callback);
}
