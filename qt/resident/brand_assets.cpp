#include "brand_assets.h"
#include <QResource>

// Resource initialization must be in the global namespace for static libraries.
static void registerBrandResources() { Q_INIT_RESOURCE(clibby_brand); }

namespace pets::branding {
void initialize() {
    static const bool initialized = [] { registerBrandResources(); return true; }();
    Q_UNUSED(initialized);
}
QIcon appIcon() {
    initialize();
    QIcon icon;
    for (int size : {32, 64, 128, 256, 512, 1024})
        icon.addFile(QString(":/clibby/app-icon-%1.png").arg(size), QSize(size, size));
    return icon;
}
QIcon menuIcon() {
    initialize();
    QIcon icon;
    icon.addFile(":/clibby/menu-template-32.png", QSize(16, 16));
    icon.addFile(":/clibby/menu-template-64.png", QSize(32, 32));
    icon.setIsMask(true);
    return icon;
}
}
