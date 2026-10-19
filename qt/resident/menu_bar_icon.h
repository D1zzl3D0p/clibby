#pragma once
#include <memory>
class QMenu;
namespace pets {
// The menu must outlive its menu-bar entry.
class MenuBarIcon {
public:
    static bool isAvailable();
    explicit MenuBarIcon(QMenu *menu);
    ~MenuBarIcon();
    MenuBarIcon(const MenuBarIcon &) = delete;
    MenuBarIcon &operator=(const MenuBarIcon &) = delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
