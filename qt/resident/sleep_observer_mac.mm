#include "sleep_observer.h"
#include <QObject>
#include <QPointer>
#include <atomic>
#import <AppKit/AppKit.h>

namespace pets {
namespace {
struct Delivery {
    QPointer<QObject> context;
    std::function<void(Control)> callback;
    std::atomic<bool> active{true};
};
id registerNotification(NSNotificationCenter* center, NSNotificationName name,
                        std::shared_ptr<Delivery> state, Control command) {
    // Direct function parameters are copied into the Objective-C block. A block
    // nested inside a capturing C++ lambda could retain that temporary closure.
    return [center addObserverForName:name object:nil queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification*) {
            // Main queue confines QPointer access to the application's thread.
            // Qt's receiver context also expires queued delivery.
            if (!state->active || !state->context) return;
            QMetaObject::invokeMethod(state->context.data(), [state, command] {
                if (state->active && state->context) state->callback(command);
            }, Qt::QueuedConnection);
        }];
}
class WorkspaceSleepObserver final : public SleepObserver {
public:
    WorkspaceSleepObserver(QObject* context, std::function<void(Control)> callback)
        : delivery_(std::make_shared<Delivery>()) {
        delivery_->context = context;
        delivery_->callback = std::move(callback);
        center_ = [[NSWorkspace sharedWorkspace] notificationCenter];
        sleep_ = registerNotification(center_, NSWorkspaceWillSleepNotification, delivery_, Control::Suspend);
        wake_ = registerNotification(center_, NSWorkspaceDidWakeNotification, delivery_, Control::Wake);
    }
    ~WorkspaceSleepObserver() override {
        delivery_->active = false;
        [center_ removeObserver:sleep_];
        [center_ removeObserver:wake_];
    }
private:
    std::shared_ptr<Delivery> delivery_;
    NSNotificationCenter* __strong center_ = nil;
    id __strong sleep_ = nil;
    id __strong wake_ = nil;
};
}
std::unique_ptr<SleepObserver> observeSleep(QObject* context, std::function<void(Control)> callback) {
    return std::make_unique<WorkspaceSleepObserver>(context, std::move(callback));
}
}
