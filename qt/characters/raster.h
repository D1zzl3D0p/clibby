#pragma once
#include "character.h"
#include <QImage>
namespace pets::character {
struct RasterOptions {
  int resolution = 96;
  bool smooth = false, palette = false, dither = false, debug = false;
};
QImage renderCharacter(const Definition &, const QImage &, const Pose &,
                       const RasterOptions & = {});
void reducePalette(QImage &, bool dither);
} // namespace pets::character
