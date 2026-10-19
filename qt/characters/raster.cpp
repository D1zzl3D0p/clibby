#include "raster.h"
#include <QPainter>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
void reducePalette(QImage &image, bool dither) {
  static constexpr int bayer[4][4] = {
      {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  image = image.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x) {
      const QColor c = image.pixelColor(x, y);
      if (!c.alpha())
        continue;
      const double delta =
          dither ? (bayer[y % 4][x % 4] / 16.0 - 0.46875) * 51.0 : 0;
      auto channel = [&](int v) {
        return std::clamp(int(std::round((v + delta) / 51.0)) * 51, 0, 255);
      };
      image.setPixelColor(x, y,
                          QColor(channel(c.red()), channel(c.green()),
                                 channel(c.blue()), c.alpha()));
    }
}
QImage renderCharacter(const Definition &d, const QImage &atlas,
                       const Pose &pose, const RasterOptions &options) {
  if (options.resolution < 24 || options.resolution > 512)
    throw std::invalid_argument("canvas resolution must be between 24 and 512");
  const auto world = worldTransforms(d, pose);
  const auto art = artworkTransforms(d, pose);
  QImage result(options.resolution, options.resolution,
                QImage::Format_ARGB32_Premultiplied);
  result.fill(Qt::transparent);
  QPainter painter(&result);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, options.smooth);
  painter.setRenderHint(QPainter::Antialiasing, false);
  const double w = d.width + 2 * d.motionEnvelope,
               h = d.height + 2 * d.motionEnvelope;
  const double scale = options.resolution / std::max(w, h);
  QTransform canvas;
  canvas.translate((options.resolution - w * scale) / 2,
                   (options.resolution - h * scale) / 2);
  canvas.scale(scale, scale);
  std::vector<size_t> order(d.attachments.size());
  for (size_t i = 0; i < order.size(); ++i)
    order[i] = i;
  std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return d.attachments[a].drawOrder < d.attachments[b].drawOrder;
  });
  for (auto index : order) {
    const auto &a = d.attachments[index];
    const auto &t = art.at(a.bone);
    QTransform local(t.m00, t.m10, t.m01, t.m11, t.tx + d.motionEnvelope,
                     t.ty + d.motionEnvelope);
    painter.setTransform(local * canvas);
    painter.drawImage(
        QRectF(a.offset.x - a.pivot.x, a.offset.y - a.pivot.y, a.source.width,
               a.source.height),
        atlas, QRectF(a.source.x, a.source.y, a.source.width, a.source.height));
  }
  if (options.debug) {
    painter.setTransform(canvas);
    painter.setPen(QPen(QColor(30, 240, 120), 0));
    for (size_t i = 0; i < world.size(); ++i) {
      QPointF end(world[i].tx + d.motionEnvelope,
                  world[i].ty + d.motionEnvelope);
      if (d.bones[i].parent >= 0) {
        const auto &p = world[d.bones[i].parent];
        painter.drawLine(
            QPointF(p.tx + d.motionEnvelope, p.ty + d.motionEnvelope), end);
      }
      painter.drawEllipse(end, 1.5, 1.5);
    }
    painter.setPen(QPen(QColor(255, 90, 200), 0));
    for (const auto &c : d.contacts) {
      auto pt = world[c.bone].apply(c.point);
      painter.drawEllipse(
          QPointF(pt.x + d.motionEnvelope, pt.y + d.motionEnvelope), 2, 2);
    }
  }
  painter.end();
  if (options.palette || options.dither)
    reducePalette(result, options.dither);
  return result;
}
} // namespace pets::character
