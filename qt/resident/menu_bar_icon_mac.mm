#include "menu_bar_icon.h"
#include "brand_assets.h"
#include <QGuiApplication>
#include <QMenu>
#import <AppKit/AppKit.h>
namespace pets {
struct MenuBarIcon::Impl {
    NSStatusItem *item = nil;
    ~Impl() {
        item.menu = nil;
        if (item) [NSStatusBar.systemStatusBar removeStatusItem:item];
    }
};
bool MenuBarIcon::isAvailable() { return QGuiApplication::platformName() == "cocoa"; }
MenuBarIcon::MenuBarIcon(QMenu *menu) : impl_(std::make_unique<Impl>()) {
    // Qt 6.10's tray activation observer reads clickCount on non-mouse events
    // delivered by macOS 27 (QTBUG-147449). A native status item needs no such
    // observer. Keep Qt's NSMenu delegate and all existing QAction dispatch.
    impl_->item = [NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength];
    const auto image = branding::menuIcon().pixmap(32, 32).toImage();
    CGImageRef pixels = image.toCGImage();
    NSImage *nativeImage = [[NSImage alloc] initWithCGImage:pixels size:NSMakeSize(16, 16)];
    CGImageRelease(pixels);
    [nativeImage setTemplate:YES];
    impl_->item.button.image = nativeImage;
    impl_->item.button.toolTip = @"Clibby";
    impl_->item.menu = menu->toNSMenu();
}
MenuBarIcon::~MenuBarIcon() = default;
}
