#pragma once
#include "characters/grounded.h"
#include <QPoint>
#include <QString>
#include <memory>
#include <utility>

namespace pets {
class DesktopHost;
// Trusted built-in scene; package data never supplies executable QML.
bool isSpatialPackage(const QString &directory);
class SpatialCharacter final {
public:
  explicit SpatialCharacter(QString directory = {});
  ~SpatialCharacter();
  void install(DesktopHost &host);
  void reset(const DesktopHost &host);
  void pickup(double logicalX, double elevation);
  void hold(double logicalX, double elevation);
  void release(QPointF velocity);
  void reconcile(const DesktopHost &host);
  void setSupportFrame(const DesktopHost &host);
  std::pair<double,double> supportFootprint() const;
  bool flightCapable() const;
  bool hop(double logicalHeight);
  bool flyTo(double logicalX, double logicalElevation);
  void land();
  void cancelFlight();
  void setSupportBounds(std::optional<character::GroundSpan> logicalSoleInterval);
  void target(double logicalX);
  bool face(double logicalX);
  bool blinkSupported() const;
  void setEyeOpenness(double openness);
  character::MotionProposal propose(double dt);
  void realize(double logicalX, bool blocked = false, std::optional<double> elevation = {});
  character::GroundedState state() const;
  bool settled() const;
  void setSpeed(double multiplier);
  character::ExpressionMotion &expressions();
  bool reach(int side, motion::Vec3 normalizedTarget);
  void releaseReach(int side);
  void reaction(const QString &semantic, double progress);
  void apply(DesktopHost &host);
  void setRenderingEnabled(bool enabled);
  QPoint cardAnchor() const;
  QPoint headAnchor() const;
  // Relative screen displacement mapped around model-neutral forward; this
  // preserves responsiveness within the bounded head range as the body turns.
  motion::Vec3 cursorLookDirection(motion::Vec3 screenDirection) const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
