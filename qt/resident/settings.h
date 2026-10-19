#pragma once
#include <QPoint>
#include <QString>

namespace pets {
struct Preferences {
    QPoint position;
    QString display;
    bool hasPosition = false;
    bool paused = false;
    double walkingSpeed = 2.0;
    double companionScale = 1.0;
    double pixelation = 1.0;
    QString character = "fixture-animal";
};
Preferences loadPreferences(const QString& file);
bool savePreferences(const QString& file, const Preferences& preferences);
}
