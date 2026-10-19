#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace pets {
// Reuse an existing legacy store in place: preferences, inbox, lock and IPC all
// retain one owner. No copying of live stores or silent merging is performed.
inline QString chooseStateDirectory(const QString &current, const QString &legacy) {
    if (current.isEmpty() || QDir(current).exists()) return current;
    return !legacy.isEmpty() && QDir(legacy).exists() ? legacy : current;
}
inline QString defaultStateDirectory() {
    const auto current = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const auto app = QCoreApplication::applicationName();
    const auto organization = QCoreApplication::organizationName();
    // Called once at startup before resident services or client connections.
    QCoreApplication::setApplicationName("Better Pets");
    QCoreApplication::setOrganizationName("Better Pets");
    const auto legacy = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QCoreApplication::setApplicationName(app);
    QCoreApplication::setOrganizationName(organization);
    return chooseStateDirectory(current, legacy);
}
}
