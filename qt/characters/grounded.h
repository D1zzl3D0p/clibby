#pragma once
#include "character.h"
#include "second_order.h"
#include "motion_support.h"
#include "drop_motion.h"
#include <array>
#include <stdexcept>
#include <type_traits>
namespace pets::character {
struct GroundSpan {
  double minimum = 0, maximum = 0;
};
struct MotionProposal {
  double displacement = 0, targetX = 0;
  double displacementY = 0;
};
struct FootState {
  Vec2 position, landing;
  bool swing = false;
  double phase = 0;
  bool recovering = false;
};
struct SupportTuning {
  // Compliant weight transfer retains authored step duration and gentle air fall.
  double stiffness = 200, damping = 7, gravityPerReach = 4;
  double landingTime = .012, stanceReserveFraction = .03;
  double uprightStiffness = 0, uprightDamping = 0;
  double contactLossGravityScale = .5, airborneDamping = 8;
};
inline constexpr double defaultFootPlantDuration = .01;
struct GroundedState {
  double rootX = 0, velocity = 0, targetX = 0;
  int facing = 1;
  LimbArray<FootState> feet;
  bool active = false;
  double facingBlend = 1;
  double verticalVelocity = 0, verticalAcceleration = 0;
  DropPhase phase = DropPhase::Grounded;
  double elevation = 0;
  std::array<WingState,2> wings{};
  bool poweredFlight = false;
  bool anyFootSwinging() const;
  GroundSpan soleExtent() const;
};
// propose stores one bounded elapsed interval; realize consumes it exactly
// once. rootX is host-presented world anchor X. Pose coordinates stay
// canvas-local.
class GroundedController {
public:
  explicit GroundedController(Definition);
  void reset(double rootX, GroundSpan, double groundWorldY = 0,
             bool preserveSecondary = false);
  void setTargetX(double);
  // Change only the facing goal; feet and navigation remain in place.
  bool face(double worldX);
  bool hop(double height);
  bool flyTo(double worldX, double elevation);
  void land();
  void cancelFlight();
  bool flightCapable() const;
  void setFlightEffortLimit(double limit);
  MotionProposal propose(double dt);
  void realize(double rootX, bool blocked = false, std::optional<double> elevation = {}, double ceilingTolerance = 1);
  void pickup(double rootX, double elevation);
  void hold(double rootX, double elevation);
  void release(Vec2 velocity);
  void reconcile(double rootX, GroundSpan, double floorY, double elevation);
  // Floor change between proposal and realization; preserves the pending step.
  void setSupportFrame(GroundSpan, double floorWorldY);
  double referenceReach() const { return supportReach_; }
  Pose pose(Action action = Action::Idle, double reactionTime = 0,
            Vec2 gaze = {}) const;
  void setAction(Action action, double time = 0) { legacy_.setAction(action,time); }
  ExpressionMotion &expressions() { return legacy_.expressions(); }
  const ExpressionMotion &expressions() const { return legacy_.expressions(); }
  GroundedState state() const;
  bool settled() const;
  void impulse(Vec2 direction, double magnitude = 1);
  void setSecondaryTuning(SecondaryTuning);
  void setSpeedMultiplier(double);
  void setLandingLeadFraction(double);
  void setFootHomeRadius(double);
  // Minimum double-support pause after touchdown; swing duration is separate.
  void setFootPlantDuration(double seconds);
  void setSwingAdmission(bool allowed) { swingAdmission_ = allowed; }
  void setSupportTuning(SupportTuning);
  void setSupportContacts(std::array<bool, 2> grounded);
  // A constrained array overload keeps legacy braced two-contact calls unambiguous.
  template<std::size_t N, std::enable_if_t<N == 4, int> = 0>
  void setSupportContacts(std::array<bool, N> grounded) {
    if(feet_.size()!=N) throw std::runtime_error("support contacts anatomy mismatch");
    supportContacts_=grounded;
  }
  void setSupportGeometry(std::array<pets::motion::SupportLeg, 2>);
  template<std::size_t N, std::enable_if_t<N == 4, int> = 0>
  void setSupportGeometry(std::array<pets::motion::SupportLeg, N> geometry) {
    if(feet_.size()!=N) throw std::runtime_error("support geometry anatomy mismatch");
    supportGeometry_=geometry; hasSupportGeometry_=true;
  }
  // Absolute sole limits for future touchdowns; nullopt preserves desktop gait.
  // Existing stance pins are never clamped or moved by this setter.
  void setSupportBounds(std::optional<GroundSpan> absoluteSoleInterval);
  double speedMultiplier() const { return speedMultiplier_; }
  const Definition &definition() const { return definition_; }

private:
  Definition definition_;
  Locomotion tuning_;
  SecondaryTuning secondaryTuning_;
  SupportTuning supportTuning_;
  LimbArray<pets::motion::SupportLeg> supportGeometry_;
  bool hasSupportGeometry_ = false, swingAdmission_ = true;
  LimbArray<bool> supportContacts_;
  LimbArray<double> supportLoad_;
  double speedMultiplier_ = 1, bodyLowering_ = 0, landingLeadFraction_ = .3, footHomeRadius_ = .5;
  double vertical_ = 0, verticalVelocity_ = 0, gravity_ = 0, supportReach_ = 0, verticalAcceleration_ = 0;
  double landingBodyLift_ = 0;
  GroundSpan span_;
  std::optional<GroundSpan> supportBounds_;
  double rootX_ = 0, target_ = 0, velocity_ = 0, pending_ = 0, ground_ = 0,
         lastVelocity_ = 0, intendedLean_ = 0, acceleration_ = 0;
  int facing_ = 1, requestedFacing_ = 1;
  double facingBlend_ = 1;
  LimbArray<FootState> feet_;
  LimbArray<double> restX_, lengthA_, lengthB_, springRest_;
  double homeAnchorX_ = 0;
  LimbArray<Vec2> swingStart_;
  LimbArray<double> swingTime_, swingDuration_;
  double launchWait_ = 0, footPlantDuration_ = defaultFootPlantDuration;
  DropMotion drop_, proposedDrop_;
  WingMotion wings_, proposedWings_;
  bool poweredFlight_ = false, landingFlight_ = false;
  double flightReach_ = 0;
  double flightTargetX_=0, flightTargetElevation_=0, flightEffortLimit_=1;
  FlightStep proposedFlight_;
  bool recoveryStepStarted_ = false;
  int nextFoot_ = 0;
  bool quadruped() const { return tuning_.gait == Gait::QuadrupedWalk; }
  double homeX(int side) const {
    return quadruped() ? homeAnchorX_ + facing_ * (restX_[side] - homeAnchorX_) : restX_[side];
  }
  int followingFoot(int side) const;
  Pose bodyPose(Action, double, Vec2) const;
  void stepSupport(double dt);
  void installDropContacts();
  void stepDropFollowers(double dt);
  void stepSwing(int side, double dt);
  double touchdownX(double desired) const;
  bool dynamicsSettled() const;
  SecondOrderTracker lean_, head_, ears_, tail_, airborneReach_, airborneSway_, armSpread_;
  Controller legacy_;
};
} // namespace pets::character
