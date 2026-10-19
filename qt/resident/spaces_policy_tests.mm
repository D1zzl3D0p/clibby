#include "desktop_host.h"
#include "spaces_policy.h"
#include "status_label.h"
#include <QApplication>
#include <QQuickWindow>
#include <iostream>
#include <stdexcept>
#import <AppKit/AppKit.h>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void checkSpacesPolicy(WId handle) {
    check(handle != 0, "Qt window has a native handle");
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(handle);
    NSWindow *window = view.window;
    check(window != nil, "Qt native view has an NSWindow");
    const auto behavior = window.collectionBehavior;
    check((behavior & NSWindowCollectionBehaviorCanJoinAllSpaces) != 0,
          "native window joins all desktop Spaces");
    check((behavior & NSWindowCollectionBehaviorMoveToActiveSpace) == 0,
          "native window does not move to the active Space");
    check((behavior & NSWindowCollectionBehaviorFullScreenAuxiliary) != 0,
          "native window can accompany a fullscreen window");
    check((behavior & NSWindowCollectionBehaviorFullScreenPrimary) == 0,
          "native window is not a primary fullscreen window");
    check((behavior & NSWindowCollectionBehaviorFullScreenNone) == 0,
          "native window does not exclude fullscreen participation");
    if (@available(macOS 13.0, *)) {
        check((behavior & NSWindowCollectionBehaviorCanJoinAllApplications) != 0,
              "native window can join all application groups");
        check((behavior & NSWindowCollectionBehaviorPrimary) == 0 &&
              (behavior & NSWindowCollectionBehaviorAuxiliary) == 0,
              "all-application role excludes conflicting primary and auxiliary roles");
    }
}

void checkPolicyDoesNotMove(QWindow *window) {
    const auto position = window->position();
    const auto screen = window->screen();
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(window->winId());
    check(view.window != nil, "position check has a native window");
    const auto frame = view.window.frame;
    pets::joinAllSpaces(window);
    check(window->position() == position && window->screen() == screen &&
          NSEqualRects(view.window.frame, frame),
          "applying Spaces policy preserves native geometry and physical display");
    checkSpacesPolicy(window->winId());
}

QQuickWindow *petWindow() {
    for (auto *window : QGuiApplication::topLevelWindows())
        if (window->title() == "Clibby")
            if (auto *quick = dynamic_cast<QQuickWindow *>(window)) return quick;
    throw std::runtime_error("DesktopHost owns a discoverable pet window");
}

void checkHost(pets::DesktopHost &host) {
    host.setPetVisible(true);
    QApplication::processEvents();
    auto *window = petWindow();
    check(host.isPetVisible(), "pet is visible after show");
    checkSpacesPolicy(window->winId());
    checkPolicyDoesNotMove(window);
    const auto display = host.displayName();
    const auto screen = window->screen();
    const auto position=host.position();
    host.setPetVisible(false);
    QApplication::processEvents();
    check(!host.isPetVisible(), "pet is hidden after hide");
    check(host.position()==position,"hide preserves realized pet position");
    host.setPetVisible(true);
    QApplication::processEvents();
    check(host.isPetVisible(), "pet is visible after restore");
    check(host.position()==position,"show preserves realized pet position");
    checkSpacesPolicy(window->winId());
    // Restoration retains placement, the physical display and the host window.
    check(host.displayName() == display && window->screen() == screen && petWindow() == window,
          "restore retains physical display and original pet window");
    host.setPetVisible(false);
    window->destroy();
    host.setPetVisible(true);
    QApplication::processEvents();
    check(host.isPetVisible() && petWindow() == window,
          "native surface recreation restores original host window");
    checkSpacesPolicy(window->winId());
    checkPolicyDoesNotMove(window);
}

void fixtureHost(const QString &assets) {
    const auto rig = loadRig(assets);
    pets::DesktopHost host(rig, QImage(rig.atlas));
    checkHost(host);
}

void modularHost() {
    pets::character::LoadedCharacter loaded;
    auto &definition = loaded.definition;
    definition.width = definition.height = 64;
    definition.motionEnvelope = 8;
    definition.bones = {{"body", -1, {20, 20}}};
    definition.attachments = {{0, {0, 0, 8, 8}, {}, {}, 0}};
    definition.interaction = {{20, 20}, {28, 20}, {28, 28}, {20, 28}};
    loaded.atlas = QImage(8, 8, QImage::Format_ARGB32);
    loaded.atlas.fill(Qt::red);
    pets::DesktopHost host(loaded);
    checkHost(host);
}

void attachedStatusCard() {
    pets::StatusLabel card;
    card.placeBeside({200, 200});
    card.show();
    QApplication::processEvents();
    check(card.isVisible(), "attached status card is visible");
    checkSpacesPolicy(card.winId());
    const auto position = card.pos();
    card.hide();
    QApplication::processEvents();
    check(!card.isVisible(), "attached status card hides");
    card.show();
    QApplication::processEvents();
    check(card.isVisible(), "attached status card restores");
    checkSpacesPolicy(card.winId());
    check(card.pos() == position, "attached card restore preserves position");
}
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    if (QGuiApplication::platformName() != "cocoa") {
        std::cout << "SKIP native Spaces policy requires the cocoa platform\n";
        return 77;
    }
    int failures = 0;
    const auto run = [&](const char *name, const auto &test) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception &error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };
    run("fixture host native Spaces policy", [&] {
        check(argc == 2, "fixture asset argument required");
        fixtureHost(QString::fromLocal8Bit(argv[1]));
    });
    run("modular host native Spaces policy", modularHost);
    run("attached status card native Spaces policy", attachedStatusCard);
    return failures == 0 ? 0 : 1;
}
