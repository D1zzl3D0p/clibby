#include "desktop_host.h"
#include "spaces_policy.h"
#include "window_surfaces.h"
#include <QPlatformSurfaceEvent>
#include "characters/item.h"
#include "petitem.h"
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPolygon>
#include <QQuickWindow>
#include <QQuickItem>
#include <QScreen>
#include <QSurfaceFormat>
#include <algorithm>
#include <cmath>
#include <limits>

namespace pets {
Placement placeOnDisplays(QPoint target, QSize size,
                          const QVector<DisplayGeometry> &displays,
                          const QString &preferredDisplay,
                          const QRegion &interactionRegion) {
  const DisplayGeometry *selected = nullptr;
  bool savedDisplayFound = preferredDisplay.isEmpty();
  for (const auto &display : displays) {
    if (display.available.isEmpty())
      continue;
    if (!selected)
      selected = &display;
    if (!preferredDisplay.isEmpty() && display.name == preferredDisplay) {
      selected = &display;
      savedDisplayFound = true;
      break;
    }
  }
  if (!selected)
    return {target, {}};
  if (preferredDisplay.isEmpty()) {
    const double centerX = double(target.x()) + std::max(0, size.width()) / 2.0;
    const double centerY =
        double(target.y()) + std::max(0, size.height()) / 2.0;
    double nearest = std::numeric_limits<double>::infinity();
    for (const auto &display : displays) {
      const auto &area = display.available;
      if (area.isEmpty())
        continue;
      const double dx = std::max(
          {double(area.left()) - centerX, 0.0, centerX - double(area.right())});
      const double dy = std::max(
          {double(area.top()) - centerY, 0.0, centerY - double(area.bottom())});
      const double distance = dx * dx + dy * dy;
      if (distance < nearest) {
        selected = &display;
        nearest = distance;
      }
    }
  }
  const QRegion interaction = interactionRegion.isEmpty()
                                  ? QRegion(QRect(QPoint(), size))
                                  : interactionRegion;
  const auto reachableAt = [&](QPoint position, const QRect &area) {
    return interaction.translated(position).intersects(area);
  };
  if (savedDisplayFound) {
    for (const auto &display : displays) {
      if (reachableAt(target, display.available))
        return {target, selected->name};
    }
  }
  const auto &area = selected->available;
  const qint64 maxX = std::max<qint64>(
      area.x(), qint64(area.x()) + area.width() - std::max(1, size.width()));
  const qint64 maxY = std::max<qint64>(
      area.y(), qint64(area.y()) + area.height() - std::max(1, size.height()));
  QPoint repaired(int(std::clamp<qint64>(target.x(), area.x(), maxX)),
                  int(std::clamp<qint64>(target.y(), area.y(), maxY)));
  if (!interaction.isEmpty() && !reachableAt(repaired, area)) {
    // An undersized screen may expose only transparent padding after a
    // window clamp. Place a real mask pixel at its available origin.
    repaired = area.topLeft() - interaction.begin()->topLeft();
  }
  return {repaired, selected->name};
}
DesktopHost::DesktopHost(const Rig &rig, const QImage &atlas, QObject *parent)
    : DesktopHost(parent) {
  window_->resize(256, 256);
  window_->setMinimumSize({256, 256});
  window_->setMaximumSize({256, 256});
  item_ = new PetItem(rig, atlas, false, window_->contentItem());
  QPolygon polygon;
  for (const auto &point : rig.hitPolygon)
    polygon.append(point.toPoint());
  interactionRegion_ = QRegion(polygon);
  visualRegion_ = interactionRegion_;
  window_->setMask(interactionRegion_);
  initialPlacement();
}
DesktopHost::DesktopHost(const character::LoadedCharacter &loaded,
                         QObject *parent)
    : DesktopHost(parent) {
  setCharacter(loaded);
  initialPlacement();
}
DesktopHost::DesktopHost(QObject *parent)
    : QObject(parent), window_(std::make_unique<QQuickWindow>()) {
  window_->setTitle("Better Pets");
  // macOS tool panels hide on application deactivation. The resident is an
  // independent, nonactivating top-level window and stays visible until Hide.
  window_->setFlags(Qt::Window | Qt::FramelessWindowHint |
                    Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
  auto format = window_->format();
  format.setAlphaBufferSize(8);
  format.setSamples(0);
  window_->setFormat(format);
  window_->setColor(Qt::transparent);
  window_->installEventFilter(this);
  connect(
      window_.get(), &QQuickWindow::sceneGraphError, this,
      [this](QQuickWindow::SceneGraphError, const QString &message) {
        // The receiver may tear down this host in response to failure.
        const auto callback = renderingError;
        if (callback)
          callback(message);
      },
      Qt::QueuedConnection);
  for (auto *screen : QGuiApplication::screens())
    observeScreen(screen);
  connect(qGuiApp, &QGuiApplication::screenAdded, this,
          [this](QScreen *screen) {
            observeScreen(screen);
            reconcileScreens();
          });
  connect(qGuiApp, &QGuiApplication::screenRemoved, this,
          [this] { reconcileScreens(); });
  connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this,
          [this] { reconcileScreens(); });
}
void DesktopHost::initialPlacement() {
  if (auto *primary = QGuiApplication::primaryScreen()) {
    const auto area = primary->availableGeometry();
    place({area.center().x() - window_->width() / 2,
           area.bottom() - window_->height() + 1 - 48},
          primary->name());
    if (grounded_) rebaseGround();
  }
}
DesktopHost::~DesktopHost() {
  draggingChanged = {};
  dragEnded = {};
  contextMenuRequested = {};
  placementChanged = {};
  renderingError = {};
  dragDisplacement = {};
  window_->removeEventFilter(this);
  // Screen observers have this QObject as their context and are disconnected
  // automatically on destruction. No callback owns a raw screen pointer.
  disconnect(qGuiApp, nullptr, this, nullptr);
  for (auto *screen : QGuiApplication::screens())
    disconnect(screen, nullptr, this, nullptr);
}
void DesktopHost::setPetVisible(bool visible) {
  if (!visible) {
    cancelDrag();
    const auto desired=position();
    window_->hide();
    // Cocoa can reinterpret the frameless content origin on visibility changes.
    // Keep the realized motion coordinate stable across that native transition.
    if (position()!=desired) window_->setPosition(desired);
  } else {
    reconcileScreens();
    const auto desired=position();
    window_->show();
    joinAllSpaces(window_.get());
    if (position()!=desired) window_->setPosition(desired);
  }
}
void DesktopHost::applyPose(const Pose &pose) {
  if (item_)
    item_->setPose(pose);
}
void DesktopHost::applyPose(const character::Pose &pose) {
  if (characterItem_) {
    characterItem_->setPose(pose);
    updateVisualMask();
  }
}
void DesktopHost::setCharacter(const character::LoadedCharacter &loaded) {
  // Prepare before replacing the old renderer; loader/constructor failures do
  // not destroy the currently displayed character.
  auto replacement = std::make_unique<character::CharacterItem>(loaded);
  if (loaded.definition.locomotion) {
    replacement->setMode(character::RenderMode::Composited);
    replacement->setSmooth(false);
  }
  const int margin = int(std::ceil(loaded.definition.motionEnvelope));
  const QSize size(loaded.definition.width + 2 * margin,
                   loaded.definition.height + 2 * margin);
  QPolygon polygon;
  for (const auto &p : loaded.definition.interaction)
    polygon.append(QPoint(qRound(p.x) + margin, qRound(p.y) + margin));
  const QRegion mask = QRegion(polygon).intersected(QRect(QPoint(), size));
  if (mask.isEmpty())
    throw std::runtime_error("character has no usable interaction region");
  cancelDrag();
  const bool preserveRoot=grounded_;
  const double previousRoot=groundRootX(), previousFloor=groundWorldY();
  grounded_ = loaded.definition.locomotion.has_value();
  rootAnchorX_ = margin + loaded.definition.origin.x;
  floorAnchorY_ = margin + (grounded_ ? loaded.definition.locomotion->groundY : 0);
  fractionX_ = fractionY_ = 0;
  delete item_;
  item_ = nullptr;
  delete characterItem_;
  delete presentationItem_;
  presentationItem_ = nullptr;
  characterItem_ = replacement.release();
  characterItem_->setParentItem(window_->contentItem());
  window_->setMinimumSize({1, 1});
  window_->setMaximumSize({16777215, 16777215});
  window_->resize(size);
  window_->setMinimumSize(size);
  window_->setMaximumSize(size);
  interactionRegion_ = mask;
  updateVisualMask();
  if (preserveRoot && grounded_)
    window_->setPosition({qRound(previousRoot-rootAnchorX_),qRound(previousFloor-floorAnchorY_)});
  reconcileScreens();
  if (!preserveRoot && grounded_) rebaseGround();
}
void DesktopHost::setPresentation(QQuickItem *item, QSize size,
                                  QPointF rootFloorAnchor, QRegion grab,
                                  QRegion visual) {
  const QRect canvas(QPoint(), size);
  grab &= canvas;
  visual &= canvas;
  if (!item || item == presentationItem_ || item == characterItem_ ||
      item == item_ || size.width() <= 0 || size.height() <= 0 ||
      size.width() > 16777215 || size.height() > 16777215 ||
      !std::isfinite(rootFloorAnchor.x()) || !std::isfinite(rootFloorAnchor.y()) ||
      rootFloorAnchor.x() < 0 || rootFloorAnchor.x() > size.width() ||
      rootFloorAnchor.y() < 0 || rootFloorAnchor.y() > size.height() ||
      grab.isEmpty() || visual.isEmpty())
    throw std::runtime_error("invalid prepared presentation metadata");
  cancelDrag();
  const double previousRoot = groundRootX();
  const double previousFloor = groundWorldY();
  const bool preserveRoot = grounded_;
  delete item_; item_ = nullptr;
  delete characterItem_; characterItem_ = nullptr;
  delete presentationItem_;
  presentationItem_ = item;
  item->setParent(window_->contentItem());
  item->setParentItem(window_->contentItem());
  item->setSize(QSizeF(size));
  grounded_ = true;
  rootAnchorX_ = rootFloorAnchor.x();
  floorAnchorY_ = rootFloorAnchor.y();
  fractionX_ = fractionY_ = 0;
  window_->setMinimumSize({1, 1});
  window_->setMaximumSize({16777215, 16777215});
  window_->resize(size);
  window_->setMinimumSize(size);
  window_->setMaximumSize(size);
  interactionRegion_ = grab;
  visualRegion_ = visual;
  window_->setMask(grab | visual);
  if (preserveRoot)
    window_->setPosition({qRound(previousRoot - rootAnchorX_), qRound(previousFloor-floorAnchorY_)});
  reconcileScreens();
  if (!preserveRoot) rebaseGround();
}
void DesktopHost::updatePresentationMask(QRegion visual, QRegion grab) {
  if (!presentationItem_) return;
  const QRect canvas(QPoint(), window_->size());
  visual &= canvas;
  if (visual.isEmpty()) return;
  visualRegion_ = visual;
  if (!grab.isEmpty()) {
    grab &= canvas;
    if (!grab.isEmpty()) interactionRegion_ = grab;
  }
  const auto mask = visual | interactionRegion_;
  if (window_->mask() != mask) window_->setMask(mask);
}
QSize DesktopHost::presentationSize() const { return window_->size(); }
void DesktopHost::updateVisualMask() {
  // Qt's public mask couples native visual and input shape. Keep the actual
  // silhouette visible, while all host gestures and placement use the authored
  // region. Transparent corners stay outside this tight mask; on platforms
  // supporting masks they pass through. Visible fringe may receive native
  // clicks, but never starts a drag or menu outside the authored grab region.
  visualRegion_ = characterItem_->visualRegion();
  const auto mask = interactionRegion_ | visualRegion_;
  if (window_->mask() != mask) window_->setMask(mask);
}
void DesktopHost::restorePlacement(QPoint position, QString display) {
  cancelDrag();
  place(position, display);
  if (grounded_) {
    const auto area = groundArea();
    const double desktopFloor = area.y() + area.height();
    // A reachable torso does not imply reachable planted feet. Old character
    // window coordinates can place a taller presentation's floor offscreen.
    if (groundWorldY() > desktopFloor) {
      auto repaired = this->position();
      repaired.setY(qRound(desktopFloor - floorAnchorY_));
      window_->setPosition(repaired);
      if (placementChanged) placementChanged();
      if (groundChanged) groundChanged();
    }
  }
}
void DesktopHost::reconcileScreens() {
  place(position(), display_);
  // Reconciliation repairs penetrated floors without discarding elevation.
  if (grounded_ && groundWorldY() > floorWorldY()+.5) {
    const double y=floorWorldY()-floorAnchorY_;
    window_->setPosition({position().x(), support_ ? int(std::ceil(y)) : qRound(y)});
    fractionY_ = 0;
    if (placementChanged) placementChanged();
  }
  if (groundChanged) groundChanged();
}
QRect DesktopHost::groundArea() const {
  auto *fallback = QGuiApplication::primaryScreen();
  for (auto *screen : QGuiApplication::screens())
    if (screen->name() == display_) return screen->availableGeometry();
  return fallback ? fallback->availableGeometry() : QRect(position(), window_->size());
}
character::GroundSpan DesktopHost::groundSpan() const {
  if(landingSpan_) return *landingSpan_;
  if(support_) {
    const auto span=character::usableSurface(*support_,groundRootX(),leftFoot_,rightFoot_,surfaceNow_);
    if(span) {
      const auto desktop=desktopSpan();
      const double minimum=std::max(span->minimum,desktop.minimum);
      const double maximum=std::min(span->maximum,desktop.maximum);
      if(minimum<=maximum) return {minimum,maximum};
      const double edge=std::clamp(groundRootX(),desktop.minimum,desktop.maximum);
      return {edge,edge};
    }
  }
  return desktopSpan();
}
std::optional<character::GroundSpan> DesktopHost::supportBounds() const {
  if(!support_) return {};
  if(landingSpan_) return character::GroundSpan{landingSpan_->minimum+leftFoot_,landingSpan_->maximum+rightFoot_};
  const auto span=character::usableSurface(*support_,groundRootX(),leftFoot_,rightFoot_,surfaceNow_);
  if(!span) return {};
  return character::GroundSpan{span->minimum+leftFoot_,span->maximum+rightFoot_};
}
character::GroundSpan DesktopHost::desktopSpan() const {
  const auto a = groundArea();
  const auto bounds=occupiedBounds();
  const double minimum = a.left() + rootAnchorX_ - bounds.left();
  if(bounds.width()>a.width() || bounds.height()>a.height()) return {minimum,minimum};
  const double maximum=std::max(minimum,a.x()+a.width()-bounds.x()-bounds.width()+rootAnchorX_);
  return {minimum,maximum};
}
QRect DesktopHost::occupiedBounds() const {
  const auto region=visualRegion_ | interactionRegion_;
  return region.isEmpty() ? QRect(QPoint(),window_->size()) : region.boundingRect();
}
double DesktopHost::groundRootX() const { return position().x() + rootAnchorX_; }
double DesktopHost::groundWorldY() const { return position().y() + floorAnchorY_; }
double DesktopHost::desktopFloorWorldY() const { const auto a=groundArea(); return a.y()+a.height(); }
double DesktopHost::minimumGroundWorldY() const {
  return std::min(desktopFloorWorldY(),double(groundArea().top())+floorAnchorY_-occupiedBounds().top());
}
double DesktopHost::floorWorldY() const { return support_ ? support_->topY : desktopFloorWorldY(); }
double DesktopHost::elevation() const { return std::max(0.,floorWorldY()-groundWorldY()); }
DesktopHost::WorldRealization DesktopHost::moveInWorld(double dx, double dy) {
  if (!grounded_ || dragging_ || !std::isfinite(dx) || !std::isfinite(dy))
    return {groundRootX(), elevation(), true};
  const auto planned=planWorldMove(dx,dy);
  fractionX_=planned.remainderX;
  fractionY_=planned.remainderY;
  if(planned.position!=position()) {
    window_->setPosition(planned.position);
    if(placementChanged) placementChanged();
  }
  landingSpan_.reset();
  landingRootX_.reset();
  return {groundRootX(),elevation(),planned.blocked};
}
QPointF DesktopHost::predictedWorldPosition(double dx,double dy) const {
  if(!grounded_ || dragging_ || !std::isfinite(dx) || !std::isfinite(dy))
    return {groundRootX(),groundWorldY()};
  const auto planned=planWorldMove(dx,dy);
  return {planned.position.x()+rootAnchorX_,planned.position.y()+floorAnchorY_};
}
DesktopHost::WorldPlacement DesktopHost::planWorldMove(double dx,double dy) const {
  const auto span=groundSpan();
  const double requestedX=landingRootX_.value_or(groundRootX()+fractionX_+dx);
  const double clippedX=std::clamp(requestedX,span.minimum,span.maximum);
  // Swept acquisition has already established a downward crossing. Realize
  // that contact rather than replaying the unquantized proposal after its
  // fractional remainder was rebased to the newly selected floor.
  const double requestedY=landingSpan_ ? floorWorldY() : groundWorldY()+fractionY_+dy;
  // Leave at least one authored grab pixel reachable at the top boundary.
  const double minimumY=std::min(floorWorldY(),minimumGroundWorldY());
  const double clippedY=std::clamp(requestedY,minimumY,floorWorldY());
  const bool surfaceContact=landingSpan_ || (support_ && clippedY==floorWorldY());
  const QPoint target(qRound(clippedX-rootAnchorX_),surfaceContact ? int(std::ceil(clippedY-floorAnchorY_)) : qRound(clippedY-floorAnchorY_));
  const bool blocked=requestedX!=clippedX;
  return {target,blocked ? 0. : clippedX-(target.x()+rootAnchorX_),
      requestedY!=clippedY ? 0. : clippedY-(target.y()+floorAnchorY_),blocked};
}
void DesktopHost::rebaseGround() {
  if (!grounded_) return;
  fractionX_ = fractionY_ = 0;
  const auto span = groundSpan();
  auto p = position();
  p.setX(qRound(std::clamp(groundRootX(), span.minimum, span.maximum) - rootAnchorX_));
  const double y=floorWorldY()-floorAnchorY_;
  p.setY(support_ ? int(std::ceil(y)) : qRound(y));
  if (p != position()) { window_->setPosition(p); if (placementChanged) placementChanged(); }
}
DesktopHost::GroundRealization DesktopHost::moveOnGround(double displacement) {
  if (!grounded_ || dragging_ || !std::isfinite(displacement)) return {groundRootX(), true};
  const auto span = groundSpan();
  const double requested = groundRootX() + fractionX_ + std::clamp(displacement, -100.0, 100.0);
  const double clipped = std::clamp(requested, span.minimum, span.maximum);
  const int x = qRound(clipped - rootAnchorX_);
  fractionX_ = clipped - (x + rootAnchorX_);
  const bool blocked = requested != clipped;
  if (blocked) fractionX_ = 0;
  if (x != position().x()) {
    window_->setPosition({x, position().y()});
    if (placementChanged) placementChanged();
  }
  return {groundRootX(), blocked};
}
QPoint DesktopHost::position() const { return window_->position(); }
QString DesktopHost::displayName() const { return display_; }
bool DesktopHost::isPetVisible() const { return window_->isVisible(); }
void DesktopHost::cancelDrag() {
  finishDrag(false);
}
void DesktopHost::finishDrag(bool released) {
  if (!dragging_)
    return;
  dragging_ = false;
  fractionX_ = fractionY_ = 0;
  if (dragEnded) dragEnded(released);
  if (draggingChanged)
    draggingChanged(false);
}
void DesktopHost::setSupportFootprint(double left,double right) {
  if(!std::isfinite(left)||!std::isfinite(right)||left>right || right-left>2000) return;
  leftFoot_=left; rightFoot_=right;
  if(support_ && !character::usableSurface(*support_,groundRootX(),left,right,surfaceNow_)) clearSupport();
}
bool DesktopHost::refreshSurfaces(double now) {
  if(!std::isfinite(now)) return false;
  surfaceNow_=now;
  if(now<nextSurfaceSample_) {
    if(support_ && !character::usableSurface(*support_,groundRootX(),leftFoot_,rightFoot_,now)) clearSupport();
    return surfaceAvailable_;
  }
  nextSurfaceSample_=now+.5;
  auto snapshot=observeWindowSurfaces(now);
  surfaceAvailable_=snapshot.available;
  setSurfaceObservations(std::move(snapshot.surfaces),now);
  return snapshot.available;
}
void DesktopHost::setSurfaceObservations(std::vector<character::SupportSurface> surfaces,double now) {
  if(!std::isfinite(now)) return;
  if(surfaces.size()>256) surfaces.resize(256);
  surfaceNow_=now;
  surfaces_=std::move(surfaces);
  if(!support_) return;
  const auto next=std::find_if(surfaces_.begin(),surfaces_.end(),[&](const auto& s){return s.id==support_->id;});
  if(next==surfaces_.end()) { clearSupport(); return; }
  auto delta=character::surfaceMovement(*support_,*next,groundRootX(),leftFoot_,rightFoot_,now);
  if(!delta) { clearSupport(); return; }
  // Realize translation exactly once. Keep fractional offsets so slow moving
  // edges do not accumulate rounding drift. Screen clipping releases support.
  const double x=groundRootX()+delta->x+fractionX_;
  const double y=groundWorldY()+delta->y+fractionY_;
  const bool planted=elevation()==0;
  const QPoint desired(qRound(x-rootAnchorX_),planted ? int(std::ceil(y-floorAnchorY_)) : qRound(y-floorAnchorY_));
  place(desired,display_);
  if(position()!=desired) { clearSupport(); return; }
  fractionX_=x-groundRootX(); fractionY_=y-groundWorldY();
  support_=*next;
  if((delta->x!=0 || delta->y!=0) && groundChanged) groundChanged();
}
bool DesktopHost::selectSupport(const character::SupportSurface& surface) {
  if(!grounded_ || !character::usableSurface(surface,groundRootX(),leftFoot_,rightFoot_,surfaceNow_)) return false;
  support_=surface;
  fractionY_=0;
  if(supportChanged) supportChanged(false);
  if(groundChanged) groundChanged();
  return true;
}
bool DesktopHost::acquireSupportAt(double now,double tolerance) {
  if(!std::isfinite(now)||!std::isfinite(tolerance)||tolerance<0||tolerance>32) return false;
  surfaceNow_=now;
  const character::SupportSurface* closest=nullptr;
  double distance=tolerance;
  for(const auto& s:surfaces_) {
    if(s.topY<minimumGroundWorldY()) continue;
    const double d=std::abs(groundWorldY()-s.topY);
    if(d<=distance && character::usableSurface(s,groundRootX(),leftFoot_,rightFoot_,now)) { closest=&s; distance=d; }
  }
  if(!closest || !grounded_) return false;
  window_->setPosition({position().x(),int(std::ceil(closest->topY-floorAnchorY_))});
  if(placementChanged) placementChanged();
  return selectSupport(*closest);
}
bool DesktopHost::acquireDescendingSupport(double previous,double next,double now,double nextRoot) {
  if(!std::isfinite(now) || dragging_) return false;
  surfaceNow_=now;
  if(!std::isfinite(previous)||!std::isfinite(next)||next<=previous) return false;
  if(!std::isfinite(nextRoot)) nextRoot=groundRootX();
  const character::SupportSurface* first=nullptr;
  double contactX=groundRootX();
  for(const auto& s:surfaces_) {
    if(s.topY<previous||s.topY>next) continue;
    const double t=(s.topY-previous)/(next-previous);
    const double x=groundRootX()+t*(nextRoot-groundRootX());
    if(character::usableSurface(s,x,leftFoot_,rightFoot_,now) && (!first||s.topY<first->topY)) { first=&s; contactX=x; }
  }
  if(!first||!grounded_) return false;
  const auto span=character::usableSurface(*first,contactX,leftFoot_,rightFoot_,now);
  support_=*first;
  landingSpan_=character::GroundSpan{span->minimum,span->maximum};
  landingRootX_=contactX;
  fractionY_=0;
  if(supportChanged) supportChanged(false);
  if(groundChanged) groundChanged();
  return true;
}
void DesktopHost::clearSupport() {
  if(!support_) return;
  support_.reset();
  landingSpan_.reset();
  landingRootX_.reset();
  fractionX_=fractionY_=0;
  if(supportChanged) supportChanged(true);
  if(groundChanged) groundChanged();
}
bool DesktopHost::eventFilter(QObject *watched, QEvent *event) {
  if (watched != window_.get())
    return false;
  switch (event->type()) {
  case QEvent::PlatformSurface:
    if (static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType() ==
        QPlatformSurfaceEvent::SurfaceCreated)
      { allowTransparentWindowPlacement(window_.get()); joinAllSpaces(window_.get()); }
    break;
  case QEvent::Hide:
  case QEvent::UngrabMouse:
  case QEvent::WindowDeactivate:
    cancelDrag();
    break;
  case QEvent::MouseButtonPress: {
    auto *mouse = static_cast<QMouseEvent *>(event);
    if (!isPetVisible() ||
        !interactionRegion_.contains(mouse->position().toPoint()))
      break;
    if (mouse->button() == Qt::RightButton) {
      cancelDrag();
      if (contextMenuRequested)
        contextMenuRequested(mouse->globalPosition().toPoint());
    } else if (mouse->button() == Qt::LeftButton) {
      dragOffset_ = mouse->globalPosition().toPoint() - position();
      if (!dragging_) {
        clearSupport();
        fractionX_ = fractionY_ = 0;
        dragging_ = true;
        if (draggingChanged)
          draggingChanged(true);
      }
    } else
      break;
    event->accept();
    return true;
  }
  case QEvent::MouseMove: {
    if (!dragging_)
      break;
    auto *mouse = static_cast<QMouseEvent *>(event);
    if (!mouse->buttons().testFlag(Qt::LeftButton))
      cancelDrag();
    else {
      const auto before = position();
      place(mouse->globalPosition().toPoint() - dragOffset_);
      if (dragDisplacement && position() != before)
        dragDisplacement(QPointF(position() - before));
    }
    event->accept();
    return true;
  }
  case QEvent::MouseButtonRelease:
    if (static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton &&
        dragging_) {
      finishDrag(true);
      event->accept();
      return true;
    }
    break;
  default:
    break;
  }
  return QObject::eventFilter(watched, event);
}
void DesktopHost::observeScreen(QScreen *screen) {
  connect(screen, &QScreen::geometryChanged, this,
          [this] { reconcileScreens(); });
  connect(screen, &QScreen::availableGeometryChanged, this,
          [this] { reconcileScreens(); });
  connect(screen, &QScreen::logicalDotsPerInchChanged, this,
          [this] { reconcileScreens(); });
  connect(screen, &QScreen::physicalDotsPerInchChanged, this,
          [this] { reconcileScreens(); });
}
void DesktopHost::place(QPoint target, const QString &preferredDisplay) {
  QVector<DisplayGeometry> displays;
  const auto *primary = QGuiApplication::primaryScreen();
  if (primary)
    displays.append({primary->name(), primary->availableGeometry()});
  for (const auto *screen : QGuiApplication::screens()) {
    if (screen != primary)
      displays.append({screen->name(), screen->availableGeometry()});
  }
  auto placement = placeOnDisplays(target, window_->size(), displays,
                                         preferredDisplay, interactionRegion_);
  if(grounded_) {
    QRegion desktop;
    for(const auto& display:displays) desktop|=display.available;
    const auto occupied=visualRegion_ | interactionRegion_;
    if(!occupied.translated(placement.position).subtracted(desktop).isEmpty()) {
      for(const auto& display:displays) if(display.name==placement.display) {
        const auto bounds=occupiedBounds(); const auto area=display.available;
        const int minimumX=area.left()-bounds.left();
        const int maximumX=std::max(minimumX,area.x()+area.width()-bounds.x()-bounds.width());
        placement.position.setX(std::clamp(placement.position.x(),minimumX,maximumX));
        // Keep the head/wing silhouette on screen while allowing transparent
        // window padding above it. Floor penetration is repaired separately.
        placement.position.setY(std::max(placement.position.y(),area.top()-bounds.top()));
        break;
      }
    }
  }
  const bool changedDisplay=display_!=placement.display;
  const bool changed =
      position() != placement.position || display_ != placement.display;
  display_ = placement.display;
  if (position() != placement.position)
    window_->setPosition(placement.position);
  if (changed && placementChanged)
    placementChanged();
  if (changedDisplay && groundChanged) groundChanged();
}
} // namespace pets
