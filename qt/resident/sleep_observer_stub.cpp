#include "sleep_observer.h"
namespace pets {
std::unique_ptr<SleepObserver> observeSleep(QObject*, std::function<void(Control)>) {
    return std::make_unique<SleepObserver>();
}
}
