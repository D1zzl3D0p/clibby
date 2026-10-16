#include "spaces_policy.h"
#include <QGuiApplication>
#include <QWindow>
#import <AppKit/AppKit.h>
#import <objc/runtime.h>
#include <string>

namespace pets {
namespace {
NSRect petFrame(id,SEL,NSRect frame,NSScreen*) { return frame; }
}
void allowTransparentWindowPlacement(QWindow *window) {
    if (!window || !window->handle() || QGuiApplication::platformName() != "cocoa") return;
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    NSWindow *nativeWindow = view.window;
    if (!nativeWindow) return;
    Class base = object_getClass(nativeWindow);
    const std::string name = class_getName(base);
    if (name.rfind("BetterPetsFreePlacement_",0)==0) return;
    const std::string subclassName = "BetterPetsFreePlacement_" + name;
    Class subclass = objc_getClass(subclassName.c_str());
    if (!subclass) {
        subclass = objc_allocateClassPair(base,subclassName.c_str(),0);
        if (!subclass) return;
        const auto method = class_getInstanceMethod(base,@selector(constrainFrameRect:toScreen:));
        if (!method || !class_addMethod(subclass,@selector(constrainFrameRect:toScreen:),
                reinterpret_cast<IMP>(petFrame),method_getTypeEncoding(method))) {
            objc_disposeClassPair(subclass);
            return;
        }
        objc_registerClassPair(subclass);
    }
    // Only this pet window receives the override. Qt's superclass behavior,
    // object layout, lifetime and all other application windows are retained.
    object_setClass(nativeWindow,subclass);
}
void joinAllSpaces(QWindow *window) {
    // Offscreen WIds are not NSViews. Do not create a platform window here.
    if (!window || !window->handle() || QGuiApplication::platformName() != "cocoa") return;
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    NSWindow *nativeWindow = view.window;
    if (!nativeWindow) return;
    auto behavior = nativeWindow.collectionBehavior;
    behavior &= ~(NSWindowCollectionBehaviorMoveToActiveSpace |
                  NSWindowCollectionBehaviorFullScreenPrimary |
                  NSWindowCollectionBehaviorFullScreenNone);
    behavior |= NSWindowCollectionBehaviorCanJoinAllSpaces |
                NSWindowCollectionBehaviorFullScreenAuxiliary;
    if (@available(macOS 13.0, *)) {
        behavior &= ~(NSWindowCollectionBehaviorPrimary | NSWindowCollectionBehaviorAuxiliary);
        behavior |= NSWindowCollectionBehaviorCanJoinAllApplications;
    }
    nativeWindow.collectionBehavior = behavior;
}
}
