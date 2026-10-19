#include "app_identity.h"
#include "brand_assets.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir root;
        check(root.isValid(), "temporary state root");
        const auto current = root.path() + "/Clibby";
        const auto legacy = root.path() + "/legacy";
        check(pets::chooseStateDirectory(current, legacy) == current, "new installs use Clibby");
        QDir().mkpath(legacy);
        QFile file(legacy + "/inbox.json");
        check(file.open(QIODevice::WriteOnly), "legacy inbox can be written");
        file.write("existing inbox"); file.close();
        check(pets::chooseStateDirectory(current, legacy) == legacy, "upgrade preserves legacy store and lock");
        check(QFile::exists(legacy + "/inbox.json") && !QDir(current).exists(), "legacy resolution is non-destructive");
        QDir().mkpath(current);
        check(pets::chooseStateDirectory(current, legacy) == current, "explicit new store has priority");
        check(pets::chooseStateDirectory({}, legacy).isEmpty(), "missing current location fails closed");
        app.setApplicationName("Clibby"); app.setOrganizationName("Clibby");
        pets::defaultStateDirectory();
        check(app.applicationName() == "Clibby" && app.organizationName() == "Clibby", "legacy lookup restores identity");
        check(!pets::branding::appIcon().isNull(), "app icon resources linked");
        check(!pets::branding::menuIcon().isNull() && pets::branding::menuIcon().isMask(), "menu icon is a template");
        const auto image = pets::branding::appIcon().pixmap(128, 128).toImage();
        check(image.pixelColor(64, 64).alpha() > 0 && image.pixelColor(0, 0).alpha() == 0, "tile renders with transparent corners");
        std::cout << "identity, legacy store resolution, and icon resources passed\n";
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
