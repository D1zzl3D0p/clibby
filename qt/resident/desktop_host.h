#pragma once
#include "characters/loader.h"
#include "characters/grounded.h"
#include "motion.h"
#include "characters/support_surface.h"
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QVector>
#include <functional>
#include <memory>
#include <limits>

class QQuickWindow;
class QQuickItem;
class QScreen;
class PetItem;
namespace pets::character {
class CharacterItem;
}

namespace pets {
struct DisplayGeometry {
  QString name;
  QRect available;
};
struct Placement {
  QPoint position;
  QString display;
};
// Preserve the target while its authored interaction region is reachable on any
// display. A missing saved display or unreachable region repairs placement. The
// first display is primary; unnamed targets select by window center/nearest
// area. An omitted interaction region uses the window rectangle. All units are
// logical.
Placement placeOnDisplays(QPoint target, QSize size,
                          const QVector<DisplayGeometry> &displays,
                          const QString &preferredDisplay = {},
                          const QRegion &interactionRegion = {});

class DesktopHost final : public QObject {
public:
  explicit DesktopHost(const Rig &rig, const QImage &atlas,
                       QObject *parent = nullptr);
  explicit DesktopHost(const character::LoadedCharacter &loaded,
                       QObject *parent = nullptr);
  ~DesktopHost() override;
  void setPetVisible(bool visible);
  void applyPose(const Pose &pose);
  void applyPose(const character::Pose &pose);
  void setCharacter(const character::LoadedCharacter &loaded);
  // A successfully installed item is owned by the window. Metadata is in
  // logical window pixels; callers retain ownership if validation throws.
  void setPresentation(QQuickItem *item, QSize size, QPointF rootFloorAnchor,
                       QRegion grab, QRegion visual);
  void updatePresentationMask(QRegion visual, QRegion grab = {});
  QPointF localGroundAnchor() const { return {rootAnchorX_, floorAnchorY_}; }
  QSize presentationSize() const;
  void restorePlacement(QPoint position, QString display);
  void reconcileScreens();
  QPoint position() const;
  QString displayName() const;
  bool isPetVisible() const;
  const QRegion &interactionRegion() const { return interactionRegion_; }
  void cancelDrag();
  character::GroundSpan groundSpan() const;
  character::GroundSpan desktopSpan() const;
  std::pair<double,double> supportFootprint() const { return {leftFoot_,rightFoot_}; }
  std::optional<character::GroundSpan> supportBounds() const;
  double groundRootX() const;
  double groundWorldY() const;
  double floorWorldY() const;
  double desktopFloorWorldY() const;
  double minimumGroundWorldY() const;
  double elevation() const;
  struct WorldRealization { double rootX, elevation; bool blocked; };
  // Quantized host endpoint, including accumulated fractions. Swept support
  // discovery must use this same trajectory that moveInWorld will realize.
  QPointF predictedWorldPosition(double displacementX,double displacementY) const;
  WorldRealization moveInWorld(double displacementX, double displacementY);
  // Returns actual integer-window presentation; fractional requests accumulate.
  struct GroundRealization { double rootX; bool blocked; };
  GroundRealization moveOnGround(double displacement);
  void rebaseGround();
  bool refreshSurfaces(double nowSeconds);
  void setSurfaceObservations(std::vector<character::SupportSurface>, double nowSeconds);
  const std::vector<character::SupportSurface>& supportSurfaces() const { return surfaces_; }
  std::string selectedSurfaceId() const { return support_ ? support_->id : std::string{}; }
  bool acquireSupportAt(double nowSeconds, double tolerance=12);
  bool acquireDescendingSupport(double previousFootY,double nextFootY,double nowSeconds,
      double nextRootX=std::numeric_limits<double>::quiet_NaN());
  void clearSupport();
  void setSupportFootprint(double left,double right);
  // Fired after host realization. Loss preserves height above the actual desktop.
  std::function<void(bool lost)> supportChanged;
  std::function<void()> groundChanged;

  std::function<void(bool)> draggingChanged;
  // True only for an actual left-button release; interrupted grabs carry no throw.
  std::function<void(bool)> dragEnded;
  std::function<void(QPoint)> contextMenuRequested;
  std::function<void()> placementChanged;
  std::function<void(const QString &)> renderingError;
  // Realized displacement, not desired placement, in character-local units.
  std::function<void(QPointF)> dragDisplacement;

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  explicit DesktopHost(QObject *parent);
  void initialPlacement();
  void observeScreen(QScreen *screen);
  void place(QPoint target, const QString &preferredDisplay = {});
  void finishDrag(bool released);
  std::unique_ptr<QQuickWindow> window_;
  PetItem *item_ = nullptr; // Owned by window content item.
  character::CharacterItem *characterItem_ = nullptr;
  QQuickItem *presentationItem_ = nullptr;
  QString display_;
  QRegion interactionRegion_;
  QRegion visualRegion_;
  QRect occupiedBounds() const;
  void updateVisualMask();
  QPoint dragOffset_;
  bool dragging_ = false;
  bool grounded_ = false;
  double rootAnchorX_ = 0, floorAnchorY_ = 0, fractionX_ = 0, fractionY_ = 0;
  QRect groundArea() const;
  std::vector<character::SupportSurface> surfaces_;
  std::optional<character::SupportSurface> support_;
  double surfaceNow_=0, nextSurfaceSample_=0;
  double leftFoot_=-24,rightFoot_=24;
  bool selectSupport(const character::SupportSurface&);
  bool surfaceAvailable_=false;
  std::optional<character::GroundSpan> landingSpan_;
  std::optional<double> landingRootX_;
  struct WorldPlacement { QPoint position; double remainderX,remainderY; bool blocked; };
  WorldPlacement planWorldMove(double displacementX,double displacementY) const;
};
} // namespace pets
