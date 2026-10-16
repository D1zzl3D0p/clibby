#pragma once
#include "character.h"
#include "wing_motion.h"
namespace pets::character {
enum class DropPhase { Held, Airborne, Landing, Recovering, Grounded };
struct FlightStep {
  double displacementX = 0, displacementY = 0, impactVelocity = 0;
};
// Qt-free root flight in host units: positive Y is down; elevation is above
// the real floor. A controller previews a copy, then consumes host realization.
class DropMotion {
public:
  explicit DropMotion(double referenceReach = 1);
  void reset();
  void pickup(double elevation);
  void hold(double elevation);
  void release(Vec2 velocity);
  void launch(Vec2 velocity);
  // Authored hops and wing forces share their original acceleration scale.
  void useFlightGravity(double referenceReach = 0);
  FlightStep advance(double dt, FlightForces forces = {});
  // Tolerance is in motion units; adapters convert their pixel rounding here.
  void realizeElevation(double elevation, bool checkCeiling = true, double ceilingTolerance = 1);
  // Rebase floor-relative elevation without changing phase or momentum.
  void shiftReference(double deltaFloor);
  void blockHorizontal();
  void recover(double dt, bool supportSettled);
  DropPhase phase() const { return phase_; }
  double elevation() const { return elevation_; }
  double velocityX() const { return velocity_.x; }
  double velocityY() const { return velocity_.y; }
  double impactStrength() const { return impact_; }
  bool detached() const { return phase_ == DropPhase::Held || phase_ == DropPhase::Airborne; }
private:
  DropPhase phase_ = DropPhase::Grounded;
  double reach_ = 1, elevation_ = 0, impact_ = 0, phaseTime_ = 0;
  Vec2 velocity_;
  double gravityScale_ = 54, flightForceReach_ = 0;
  bool quadraticDrag_ = true;
};
} // namespace pets::character
