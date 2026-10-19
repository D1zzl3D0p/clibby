#include "item.h"
#include <QMatrix4x4>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>
#include <QSGTransformNode>
#include <QTransform>
#include <cmath>
#include <algorithm>
namespace pets::character {
namespace {
class CharacterNode final : public QSGNode {
public:
  QSGTexture *texture = nullptr;
  std::vector<QSGTransformNode *> transforms;
  std::vector<QSGImageNode *> images;
  quint64 generation = 0;
  bool composited = false;
  ~CharacterNode() override {
    while (firstChild())
      delete firstChild();
    delete texture;
  }
};
bool equalPose(const Pose &a, const Pose &b) {
  if (a.root.x != b.root.x || a.root.y != b.root.y ||
      a.eyeOpenness != b.eyeOpenness || a.bones.size() != b.bones.size())
    return false;
  for (size_t i = 0; i < a.bones.size(); ++i) {
    const auto &x = a.bones[i];
    const auto &y = b.bones[i];
    if (x.x != y.x || x.y != y.y || x.rotation != y.rotation ||
        x.scaleX != y.scaleX || x.scaleY != y.scaleY)
      return false;
  }
  return true;
}
} // namespace
CharacterItem::CharacterItem(const LoadedCharacter &c, QQuickItem *p)
    : CharacterItem(c.definition, c.atlas, p) {}
CharacterItem::CharacterItem(const Definition &d, const QImage &a,
                             QQuickItem *p)
    : QQuickItem(p), definition_(d), atlas_(a) {
  setFlag(ItemHasContents, true);
  setSize(
      QSizeF(d.width + 2 * d.motionEnvelope, d.height + 2 * d.motionEnvelope));
  // Cache scanline alpha geometry once; posed native masks do not repaint the
  // atlas or allocate another composited frame on each animation tick.
  for (const auto &a : d.attachments) {
    QRegion alpha;
    for (int y = 0; y < a.source.height; ++y) {
      int start = -1;
      for (int x = 0; x <= a.source.width; ++x) {
        const bool opaque = x < a.source.width &&
            atlas_.pixelColor(a.source.x + x, a.source.y + y).alpha() > 0;
        if (opaque && start < 0) start = x;
        if (!opaque && start >= 0) {
          alpha += QRect(start, y, x - start, 1);
          start = -1;
        }
      }
    }
    attachmentAlpha_.push_back(alpha);
  }
  for (const auto &b : d.bones)
    pose_.bones.push_back(b.rest);
}
QRegion CharacterItem::visualRegion() const {
  const auto world = artworkTransforms(definition_, pose_);
  QRegion visual;
  for (size_t i = 0; i < definition_.attachments.size(); ++i) {
    const auto &a = definition_.attachments[i];
    const auto &t = world.at(a.bone);
    if (std::abs(t.m00 * t.m11 - t.m01 * t.m10) < 1e-8) continue;
    const double x = a.offset.x - a.pivot.x, y = a.offset.y - a.pivot.y;
    QTransform transform(t.m00, t.m10, t.m01, t.m11,
        t.tx + definition_.motionEnvelope + t.m00 * x + t.m01 * y,
        t.ty + definition_.motionEnvelope + t.m10 * x + t.m11 * y);
    visual += transform.map(attachmentAlpha_[i]);
  }
  // Integer region transforms and nearest compositing can round pixels outward.
  // A narrow fringe preserves those pixels without claiming the whole canvas.
  const bool composite = mode_ == RenderMode::Composited || options_.palette ||
                         options_.dither || options_.debug;
  const int fringe = composite ? int(std::ceil(std::max(width(), height()) /
                                              options_.resolution)) : 1;
  const auto opaque = visual;
  for (int y = -fringe; y <= fringe; ++y)
    for (int x = -fringe; x <= fringe; ++x)
      if (x || y) visual += opaque.translated(x, y);
  return visual.intersected(QRect(0, 0, int(std::ceil(width())), int(std::ceil(height()))));
}
void CharacterItem::changed() {
  ++generation_;
  update();
}
void CharacterItem::setPose(const Pose &p) {
  if (!equalPose(pose_, p)) {
    pose_ = p;
    changed();
  }
}
void CharacterItem::setMode(RenderMode m) {
  if (m != mode_) {
    mode_ = m;
    changed();
  }
}
void CharacterItem::setResolution(int r) {
  if (r < 24 || r > 512)
    return;
  if (r != options_.resolution) {
    options_.resolution = r;
    changed();
  }
}
void CharacterItem::setSmooth(bool s) {
  if (s != options_.smooth) {
    options_.smooth = s;
    changed();
  }
}
void CharacterItem::setPalette(bool s) {
  if (s != options_.palette) {
    options_.palette = s;
    changed();
  }
}
void CharacterItem::setDither(bool s) {
  if (s != options_.dither) {
    options_.dither = s;
    changed();
  }
}
void CharacterItem::setDebugOverlay(bool s) {
  if (s != options_.debug) {
    options_.debug = s;
    changed();
  }
}
QSGNode *CharacterItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
  auto *root = static_cast<CharacterNode *>(old);
  // Palette processing operates on the low-resolution output, so selecting it
  // opts into the experimental CPU comparison path.
  const bool composite = mode_ == RenderMode::Composited || options_.palette ||
                         options_.dither || options_.debug;
  if (root && root->composited != composite) {
    delete root;
    root = nullptr;
  }
  if (!root) {
    root = new CharacterNode;
    root->composited = composite;
    if (!composite) {
      root->texture = window()->createTextureFromImage(
          atlas_, QQuickWindow::TextureHasAlphaChannel);
      std::vector<size_t> order(definition_.attachments.size());
      for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
      std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return definition_.attachments[a].drawOrder <
               definition_.attachments[b].drawOrder;
      });
      for (auto index : order) {
        const auto &a = definition_.attachments[index];
        auto *t = new QSGTransformNode;
        auto *image = window()->createImageNode();
        image->setTexture(root->texture);
        image->setOwnsTexture(false);
        image->setRect(QRectF(a.offset.x - a.pivot.x, a.offset.y - a.pivot.y,
                              a.source.width, a.source.height));
        image->setSourceRect(
            QRectF(a.source.x, a.source.y, a.source.width, a.source.height));
        t->appendChildNode(image);
        root->appendChildNode(t);
        root->transforms.push_back(t);
        root->images.push_back(image);
      }
    } else {
      auto *image = window()->createImageNode();
      image->setOwnsTexture(false);
      root->appendChildNode(image);
      root->images.push_back(image);
    }
  }
  if (root->generation == generation_)
    return root;
  if (composite) {
    auto *texture = window()->createTextureFromImage(
        renderCharacter(definition_, atlas_, pose_, options_),
        QQuickWindow::TextureHasAlphaChannel);
    root->images[0]->setTexture(texture);
    delete root->texture;
    root->texture = texture;
    root->images[0]->setFiltering(QSGTexture::Nearest);
    const double side = std::max(width(), height());
    root->images[0]->setRect(
        QRectF((width() - side) / 2, (height() - side) / 2, side, side));
  } else {
    const auto world = artworkTransforms(definition_, pose_);
    std::vector<size_t> order(definition_.attachments.size());
    for (size_t i = 0; i < order.size(); ++i)
      order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return definition_.attachments[a].drawOrder <
             definition_.attachments[b].drawOrder;
    });
    for (size_t i = 0; i < order.size(); ++i) {
      const auto &t = world[definition_.attachments[order[i]].bone];
      QMatrix4x4 matrix;
      matrix(0, 0) = t.m00;
      matrix(0, 1) = t.m01;
      matrix(1, 0) = t.m10;
      matrix(1, 1) = t.m11;
      matrix(0, 3) = t.tx + definition_.motionEnvelope;
      matrix(1, 3) = t.ty + definition_.motionEnvelope;
      root->transforms[i]->setMatrix(matrix);
      root->images[i]->setFiltering(options_.smooth ? QSGTexture::Linear
                                                    : QSGTexture::Nearest);
    }
  }
  root->generation = generation_;
  return root;
}
} // namespace pets::character
