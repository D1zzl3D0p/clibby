#pragma once
#include "controller.h"
#include <QString>
#include <QQmlEngine>
#include <QQuickItem>

// These files are installed beside the application, never supplied by packs.
QString residentRigPath(QString asset="werewolf");
QQuickItem* createResidentStage(QQmlEngine& engine, WolfMotion& motion, QString* error=nullptr, QString asset="werewolf");
