#pragma once
#include "loader.h"
#include "raster.h"
#include <QQuickItem>
#include <QRegion>
namespace pets::character {
enum class RenderMode { Direct, Composited };
class CharacterItem final : public QQuickItem {
public:
  explicit CharacterItem(const LoadedCharacter &, QQuickItem *parent = nullptr);
  CharacterItem(const Definition &, const QImage &,
                QQuickItem *parent = nullptr);
  void setPose(const Pose &);
  void setMode(RenderMode);
  void setResolution(int);
  void setSmooth(bool);
  void setPalette(bool);
  void setDither(bool);
  void setDebugOverlay(bool);
  // Posed opaque art, independently of the stable authored input polygon.
  QRegion visualRegion() const;
  const Definition &definition() const { return definition_; }

protected:
  QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

private:
  Definition definition_;
  QImage atlas_;
  std::vector<QRegion> attachmentAlpha_;
  Pose pose_;
  RenderMode mode_ = RenderMode::Direct;
  RasterOptions options_;
  quint64 generation_ = 1;
  void changed();
};
} // namespace pets::character
