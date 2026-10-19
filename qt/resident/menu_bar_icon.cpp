#include "menu_bar_icon.h"
#include "brand_assets.h"
#include <QMenu>
#include <QSystemTrayIcon>
namespace pets {
struct MenuBarIcon::Impl { QSystemTrayIcon tray{branding::menuIcon()}; };
bool MenuBarIcon::isAvailable() { return QSystemTrayIcon::isSystemTrayAvailable(); }
MenuBarIcon::MenuBarIcon(QMenu *menu) : impl_(std::make_unique<Impl>()) {
    impl_->tray.setToolTip("Clibby");
    impl_->tray.setContextMenu(menu);
    impl_->tray.show();
}
MenuBarIcon::~MenuBarIcon() = default;
}
