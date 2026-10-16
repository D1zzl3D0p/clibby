#pragma once
#include "character.h"
#include <QImage>
#include <QString>
namespace pets::character {
struct LoadedCharacter {
  Definition definition;
  QImage atlas;
};
LoadedCharacter loadCharacter(const QString &directory);
} // namespace pets::character
