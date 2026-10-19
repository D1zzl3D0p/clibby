#include "grounded.h"
#include "facing.h"
#include "ik.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
namespace {
constexpr double pi = 3.141592653589793;
void finite(double x) {
  if (!std::isfinite(x) || std::abs(x) > 1e8)
    throw std::runtime_error("invalid ground coordinate");
}
Definition expressive(Definition d) {
  d.contacts.clear();
  return d;
}
double angle(Vec2 p) { return std::atan2(p.y, p.x); }
double wrap(double a) { return std::remainder(a, 2 * pi); }
double landingPreparation(const DropMotion &drop, double reach) {
  if(drop.phase()!=DropPhase::Airborne || drop.velocityY()<0) return 0;
  // Fast impacts need preparation before the final frame. Use a bounded
  // time-to-contact horizon, with enough near-floor distance for small drops.
  const double horizon=std::clamp(drop.velocityY()*.12,.75*reach,4*reach);
  const double t=1-std::clamp(drop.elevation()/horizon,0.,1.);
  return t*t*t*(10+t*(-15+6*t));
}
double supportedImpactSpeed(double incoming,double reach) {
  // Smooth dissipation retains a useful shallow/deep impact range rather than
  // giving almost every drop the same abruptly capped support momentum.
  return 4*reach*std::tanh(std::max(0.,incoming)/(6*reach));
}
// Forecast the same acceleration/braking policy as navigation. This is a
// bounded lookahead for foot placement, not an independent animation clock.
double navigationStep(double distance, double dt, const Locomotion &tuning,
                      double &velocity, double &acceleration) {
  // Reserve braking distance for the time required to ramp acceleration.
  // This eases starts/reversals without surrendering travel to foot placement.
  const double jerk = tuning.acceleration*6;
  constexpr double response = 8;
  // The velocity follower itself takes time to unload. Its near-target
  // dynamics need an overdamped stopping response (ramp >= 4/response),
  // not just the jerk ramp, otherwise arrival crosses and reverses heading.
  const double ramp = std::max(tuning.acceleration/jerk, 4/response);
  double magnitude = std::sqrt(std::pow(tuning.acceleration*ramp,2) +
                              2*tuning.acceleration*std::abs(distance))-
                     tuning.acceleration*ramp;
  double wanted = std::abs(distance) < .002 ? 0 :
      std::copysign(std::min(tuning.speed,magnitude),distance);
  double desired = std::clamp((wanted-velocity)*response, -tuning.acceleration, tuning.acceleration);
  acceleration += std::clamp(desired-acceleration,-jerk*dt,jerk*dt);
  velocity += acceleration*dt;
  return velocity*dt;
}
double predictedRoot(double root, double velocity, double acceleration,
                     double target, const Locomotion &tuning, double horizon) {
  constexpr int samples = 24;
  double dt = horizon/samples;
  for (int i = 0; i < samples; i++) {
    double distance = target-root;
    if (std::abs(distance)<.02 && std::abs(velocity)<.15) return target;
    root += navigationStep(distance,dt,tuning,velocity,acceleration);
  }
  return root;
}
} // namespace
bool GroundedState::anyFootSwinging() const {
  return std::any_of(feet.begin(),feet.end(),[](const FootState &foot){return foot.swing;});
}
GroundSpan GroundedState::soleExtent() const {
  GroundSpan extent{feet[0].position.x,feet[0].position.x};
  for(const auto &foot:feet) {extent.minimum=std::min(extent.minimum,foot.position.x);extent.maximum=std::max(extent.maximum,foot.position.x);}
  return extent;
}
int GroundedController::followingFoot(int side) const {
  if(!quadruped())return 1-side;
  constexpr int next[]={3,2,0,1};
  return next[side];
}
GroundedController::GroundedController(Definition d)
    : definition_(d), legacy_(expressive(d)) {
  if (!d.locomotion)
    throw std::runtime_error("locomotion required");
  tuning_ = *d.locomotion;
  const auto count = tuning_.legs.size();
  feet_.setCount(count); restX_.setCount(count); lengthA_.setCount(count);
  lengthB_.setCount(count); springRest_.setCount(count); swingStart_.setCount(count);
  swingTime_.setCount(count); swingDuration_.setCount(count);
  supportGeometry_.setCount(count); supportContacts_.setCount(count); supportLoad_.setCount(count);
  // A comfortable resting stance need not reproduce subpixel home positions.
  footHomeRadius_ = tuning_.stride * .35;
  setSecondaryTuning(tuning_.secondary);
  Pose p;
  for (auto b : d.bones)
    p.bones.push_back(b.rest);
  auto w = worldTransforms(d, p);
  homeAnchorX_ = w[d.bindings.at("body")].tx - d.origin.x;
  for (int i = 0; i < int(feet_.size()); i++) {
    auto l = tuning_.legs[i];
    restX_[i] = w[l.ankle].apply(l.contact).x - d.origin.x;
    auto a = d.bones[l.knee].rest, b = d.bones[l.ankle].rest;
    lengthA_[i] = std::hypot(a.x, a.y);
    lengthB_[i] = std::hypot(b.x, b.y);

  }
  supportReach_ = 1e9;
  for(int i=0;i<int(feet_.size());++i) supportReach_=std::min(supportReach_,lengthA_[i]+lengthB_[i]);
  flightReach_=supportReach_;
  if(flightCapable()) {
    double minimum=1e9;
    for(const auto side:{"Left","Right"}) {
      double reach=0;for(const auto role:{"wingElbow","wingWrist"}) {
        const auto& rest=d.bones[d.bindings.at(std::string(role)+side)].rest;
        reach+=std::hypot(rest.x,rest.y);
      }
      minimum=std::min(minimum,reach);
    }
    flightReach_=std::clamp(std::max(supportReach_,minimum),.1,512.);
  }
  drop_ = proposedDrop_ = DropMotion(supportReach_);
  SupportTuning support;
  if(flightCapable())support.stanceReserveFraction=.12;
  setSupportTuning(support);
  setSpeedMultiplier(1);
  reset(0, {-1000, 1000});
}
void GroundedController::setSecondaryTuning(SecondaryTuning t) {
  const SecondOrderTracker validated(t);
  secondaryTuning_ = t;
  lean_.setTuning(t);
  auto follower = t;
  follower.frequency = std::clamp(t.frequency * .7, .1, 30.);
  head_.setTuning(follower);
  follower.frequency = std::clamp(t.frequency * .65, .1, 30.);
  follower.damping = std::clamp(t.damping * 1.2, .1, 4.);
  ears_.setTuning(follower);
  follower = t;
  follower.frequency = std::clamp(t.frequency * .45, .1, 30.);
  follower.damping = std::clamp(t.damping * .75, .1, 4.);
  tail_.setTuning(follower);
}
void GroundedController::setFootPlantDuration(double seconds) {
  if(!std::isfinite(seconds)||seconds<0||seconds>.2)
    throw std::runtime_error("invalid foot plant duration");
  footPlantDuration_=seconds;
}
void GroundedController::setTravelScale(double scale) {
  if(!std::isfinite(scale)||scale<0||scale>1)throw std::runtime_error("invalid travel scale");
  travelScale_=scale;
}
void GroundedController::plantContacts(LimbArray<Vec2> positions) {
  if(positions.size()!=feet_.size())throw std::runtime_error("planted contacts anatomy mismatch");
  for(const auto point:positions){finite(point.x);finite(point.y);}
  for(std::size_t i=0;i<feet_.size();++i){
    feet_[i]={positions[i],positions[i],false};
    swingStart_[i]=positions[i];swingTime_[i]=swingDuration_[i]=0;
    supportContacts_[i]=std::abs(positions[i].y-ground_)<1e-7;
    supportLoad_[i]=supportContacts_[i]?1.:0.;
  }
  hasSupportGeometry_=false;launchWait_=0;
}
void GroundedController::holdBodyHeight(std::optional<double> down) {
  if(down)finite(*down);
  heldBodyDown_=down;
  if(down&&!drop_.detached()){
    vertical_=*down-bodyLowering_;
    verticalVelocity_=verticalAcceleration_=0;
  }
}
void GroundedController::constrainBodyRise(double minimumDown) {
  finite(minimumDown);
  minimumBodyDown_=minimumDown;
  if(drop_.detached()||heldBodyDown_)return;
  const double minimum=minimumDown-bodyLowering_;
  if(vertical_<minimum-1e-8){
    vertical_=minimum;
    verticalVelocity_=std::max(0.,verticalVelocity_);
    verticalAcceleration_=std::max(0.,verticalAcceleration_);
  }
}
double GroundedController::supportTransferTime() const {
  // Coordinated pairs transfer load together; avoid a long serial-foot dwell.
  return pairedGait_?std::min(.030,supportTuning_.landingTime):supportTuning_.landingTime;
}
void GroundedController::setPairedGait(bool enabled) {
  if(enabled&&!quadruped())throw std::runtime_error("paired gait requires four legs");
  pairedGait_=enabled;
}
void GroundedController::setStepDuration(double seconds) {
  if(!std::isfinite(seconds)||seconds<.04||seconds>1)throw std::runtime_error("invalid step duration");
  tuning_.minStepDuration=tuning_.maxStepDuration=seconds;
}
void GroundedController::setSpeedMultiplier(double value) {
  if (!std::isfinite(value) || value < .5 || value > 3)
    throw std::runtime_error("walking speed multiplier outside 0.5..3");
  speedMultiplier_ = value;
  const auto &base = *definition_.locomotion;
  tuning_.speed = base.speed * value;
  tuning_.acceleration = base.acceleration;

}
void GroundedController::setSupportGeometry(std::array<pets::motion::SupportLeg,2> geometry) {
  if(geometry.size()!=feet_.size()) throw std::runtime_error("support geometry anatomy mismatch");
  supportGeometry_ = geometry;
  hasSupportGeometry_ = true;
}
void GroundedController::setSupportBounds(std::optional<GroundSpan> bounds) {
  if(bounds) {
    finite(bounds->minimum); finite(bounds->maximum);
    if(bounds->minimum>bounds->maximum)throw std::runtime_error("inverted sole support interval");
  }
  supportBounds_=bounds;
}
double GroundedController::touchdownX(double desired) const {
  return supportBounds_ ? std::clamp(desired,supportBounds_->minimum,supportBounds_->maximum) : desired;
}
void GroundedController::setSupportContacts(std::array<bool,2> grounded) {
  if(grounded.size()!=feet_.size()) throw std::runtime_error("support contacts anatomy mismatch");
  supportContacts_ = grounded;
}
void GroundedController::setSupportTuning(SupportTuning t) {
  for (double value : {t.stiffness,t.damping,t.gravityPerReach,t.landingTime,t.stanceReserveFraction,t.uprightStiffness,t.uprightDamping,t.contactLossGravityScale,t.airborneDamping})
    if (!std::isfinite(value)) throw std::runtime_error("invalid support tuning");
  if (t.stiffness<100 || t.stiffness>3000 || t.damping<1 || t.damping>100 ||
      t.gravityPerReach<1 || t.gravityPerReach>30 || t.landingTime<.005 ||
      t.landingTime>.1 || t.stanceReserveFraction<0 || t.stanceReserveFraction>.15 || t.uprightStiffness<0 || t.uprightStiffness>3000 ||
      t.uprightDamping<0 || t.uprightDamping>100 || t.contactLossGravityScale<=0 || t.contactLossGravityScale>1 ||
      t.airborneDamping<0 || t.airborneDamping>100)
    throw std::runtime_error("support tuning outside bounds");
  supportTuning_ = t;
  Pose rest; for (auto bone : definition_.bones) rest.bones.push_back(bone.rest);
  auto w = worldTransforms(definition_,rest);
  // Neutral stance and gravity are independent character data. Calibrate the
  // compliant spring rest length at that stance; both adapters use this law.
  gravity_ = t.gravityPerReach*supportReach_;
  bodyLowering_ = 0;
  for (int i = 0; i < int(feet_.size()); ++i) {
    auto leg = tuning_.legs[i];
    double dx = w[leg.ankle].tx-w[leg.hip].tx;
    double reach = lengthA_[i]+lengthB_[i];
    double length = std::min(reach*(1-t.stanceReserveFraction),reach-.05);
    double dy = std::sqrt(std::max(.001,length*length-dx*dx));
    springRest_[i] = pets::motion::neutralSupportRestLength({0,0,0},{dx,dy,0},gravity_,t.stiffness,feet_.size());
    bodyLowering_ += (1./feet_.size())*(tuning_.groundY-leg.contact.y-w[leg.hip].ty-dy);
  }
  landingBodyLift_=supportReach_;
  for(int side=0;side<int(feet_.size());++side) {
    const auto leg=tuning_.legs[side];
    const double dx=w[leg.ankle].tx-w[leg.hip].tx;
    const double reach=(lengthA_[side]+lengthB_[side])*.995;
    const double extendedY=std::sqrt(std::max(0.,reach*reach-dx*dx));
    const double neutralY=tuning_.groundY-leg.contact.y-w[leg.hip].ty-bodyLowering_;
    landingBodyLift_=std::min(landingBodyLift_,std::max(0.,extendedY-neutralY));
  }

}
void GroundedController::setFootHomeRadius(double radius) {
  if (!std::isfinite(radius) || radius < 0 || radius > 10)
    throw std::runtime_error("foot home radius outside 0..10");
  footHomeRadius_ = radius;
}
void GroundedController::setLandingLeadFraction(double fraction) {
  if (!std::isfinite(fraction) || fraction < 0 || fraction > .3)
    throw std::runtime_error("landing lead fraction outside 0..0.3");
  landingLeadFraction_ = fraction;
}
void GroundedController::reset(double x, GroundSpan s, double y,
                               bool preserveSecondary) {
  finite(x);
  finite(s.minimum);
  finite(s.maximum);
  finite(y);
  if (s.minimum > s.maximum)
    throw std::runtime_error("inverted ground span");
  span_ = s;
  supportBounds_.reset();
  minimumBodyDown_.reset();heldBodyDown_.reset();
  drop_.reset();
  wings_.reset(); proposedWings_=wings_; poweredFlight_=false; flightTargetX_=rootX_; flightTargetElevation_=0;
  recoveryStepStarted_=false;
  airborneReach_.reset(.82); airborneSway_.reset();
  rootX_ = target_ = std::clamp(x, s.minimum, s.maximum);
  requestedFacing_ = facing_;
  facingBlend_ = facing_;
  ground_ = y;
  velocity_ = lastVelocity_ = pending_ = intendedLean_ = acceleration_ = 0;
  swingTime_.fill(0);
  supportLoad_.fill(1);
  verticalAcceleration_ = 0;
  hasSupportGeometry_ = false;
  supportContacts_.fill(true);
  swingDuration_.fill(0);
  nextFoot_ = quadruped() ? 2 : 0;
  launchWait_ = 0;
  swingAdmission_ = true;
  if (!preserveSecondary) {
    armSpread_.reset();
    lean_.reset();
    vertical_ = verticalVelocity_ = 0;
    head_.reset();
    ears_.reset();
    tail_.reset();
    legacy_.resetDynamics();
  }
  for (int i = 0; i < int(feet_.size()); i++)
    feet_[i] = {{rootX_ + homeX(i), y}, {rootX_ + homeX(i), y}, false};
}
void GroundedController::pickup(double x, double elevation) {
  finite(x); finite(elevation);
  heldBodyDown_.reset();
  cancelFlight(); wings_.reset();
  drop_.pickup(elevation);
  requestedFacing_=facing_;
  recoveryStepStarted_=false;
  rootX_=target_=std::clamp(x,span_.minimum,span_.maximum);
  pending_=velocity_=lastVelocity_=acceleration_=verticalVelocity_=verticalAcceleration_=0;
  intendedLean_=0; hasSupportGeometry_=false; swingTime_.fill(0); swingDuration_.fill(0);
  supportLoad_.fill(0); supportContacts_.fill(false);
  legacy_.setAction(Action::Rest); legacy_.expressions().clear();
  for(int side=0;side<int(feet_.size());++side)
    feet_[side]={{rootX_+homeX(side),ground_-drop_.elevation()}, {}, true};
}
void GroundedController::hold(double x, double elevation) {
  finite(x); finite(elevation);
  if(drop_.phase()!=DropPhase::Held) { pickup(x,elevation); return; }
  intendedLean_=std::clamp((x-rootX_)/supportReach_*.5,-.1,.1);
  rootX_=target_=std::clamp(x,span_.minimum,span_.maximum);
  pending_=0; drop_.hold(elevation);
  for(int side=0;side<int(feet_.size());++side)
    feet_[side].position={rootX_+homeX(side),ground_-drop_.elevation()};
}
void GroundedController::installDropContacts() {
  poweredFlight_=false;wings_.goal(0,0,0);
  recoveryStepStarted_=false;
  for(int side=0;side<int(feet_.size());++side)
    feet_[side]={{touchdownX(rootX_+homeX(side)),ground_},{touchdownX(rootX_+homeX(side)),ground_},false};
  target_=rootX_; requestedFacing_=facing_; velocity_=lastVelocity_=acceleration_=0;
  supportContacts_.fill(true); supportLoad_.fill(0); hasSupportGeometry_=false;
  swingTime_.fill(0); swingDuration_.fill(0); launchWait_=footPlantDuration_;
}
void GroundedController::release(Vec2 velocity) {
  finite(velocity.x); finite(velocity.y);
  cancelFlight();
  pending_=0; intendedLean_=0; target_=rootX_;
  drop_.release(velocity); velocity_=drop_.velocityX();
  if(drop_.phase()==DropPhase::Grounded) installDropContacts();
}
void GroundedController::setSupportFrame(GroundSpan span,double floor) {
 finite(span.minimum);finite(span.maximum);finite(floor);
 if(span.minimum>span.maximum)throw std::runtime_error("inverted support span");
 const double delta=floor-ground_;
 flightTargetElevation_=std::max(0.,flightTargetElevation_+delta);
 flightTargetX_=std::clamp(flightTargetX_,span.minimum,span.maximum);
 drop_.shiftReference(delta);proposedDrop_.shiftReference(delta);
 if(!drop_.detached())for(auto& foot:feet_){foot.position.y+=delta;foot.landing.y+=delta;}
 span_=span;ground_=floor;
 target_=std::clamp(target_,span.minimum,span.maximum);
}
void GroundedController::reconcile(double x, GroundSpan span, double floor, double elevation) {
  finite(x); finite(span.minimum); finite(span.maximum); finite(floor); finite(elevation);
  if(span.minimum>span.maximum) throw std::runtime_error("inverted ground span");
  const auto phase=drop_.phase();
  const int body=definition_.bindings.at("body");
  const auto presentedBody=bodyPose(Action::Rest,0,{}).bones[body];
  const double presentedVertical=presentedBody.y-
      definition_.bones[body].rest.y-bodyLowering_;
  const double presentedLean=presentedBody.rotation-
      definition_.bones[body].rest.rotation;
  const double incoming=drop_.velocityY(), floorDelta=floor-ground_;
  const double previousRoot=rootX_;
  flightTargetElevation_=std::max(0.,flightTargetElevation_+floorDelta);
  span_=span; rootX_=std::clamp(x,span.minimum,span.maximum); ground_=floor;
  flightTargetX_=std::clamp(flightTargetX_,span.minimum,span.maximum);
  target_=std::clamp(target_,span.minimum,span.maximum); pending_=0;
  drop_.realizeElevation(elevation,false);
  if(rootX_!=x) { drop_.blockHorizontal(); velocity_=acceleration_=0; }
  if(drop_.detached()) {
    if(phase==DropPhase::Grounded || phase==DropPhase::Landing || phase==DropPhase::Recovering) {
      target_=rootX_; velocity_=acceleration_=verticalVelocity_=0;
      legacy_.setAction(Action::Rest); legacy_.expressions().clear();
    }
    supportLoad_.fill(0); supportContacts_.fill(false); hasSupportGeometry_=false;
    for(int side=0;side<int(feet_.size());++side)
      feet_[side]={{rootX_+homeX(side),ground_-drop_.elevation()}, {}, true};
  } else if(phase==DropPhase::Airborne) {
    installDropContacts(); vertical_=presentedVertical;
    lean_.reset(presentedLean); intendedLean_=0;
    verticalVelocity_=supportedImpactSpeed(incoming,supportReach_);
  } else {
    // Display repair moves the presented root into a different world span;
    // obsolete monitor pins move with that one explicit placement repair.
    const double rootDelta=rootX_-previousRoot;
    for(int side=0;side<int(feet_.size());++side) {
      auto &foot=feet_[side];
      foot.position.x+=rootDelta; foot.landing.x+=rootDelta;
      foot.position.y+=floorDelta; foot.landing.y+=floorDelta;
      swingStart_[side].x+=rootDelta; swingStart_[side].y+=floorDelta;
    }
  }
}
void GroundedController::stepDropFollowers(double dt) {
  const bool arms=definition_.bindings.count("armLeft") || definition_.bindings.count("armRight");
  armSpread_.step(dt,arms && drop_.phase()==DropPhase::Airborne && !poweredFlight_ ? 1 : 0);
  const double turn=2*dt/.18;
  facingBlend_+=std::clamp(facing_-facingBlend_,-turn,turn);
  if(drop_.detached()) {
    // This is a local compression follower, never a second flight integrator.
    vertical_*=std::exp(-12*dt);
    if(std::abs(vertical_)<.0001) vertical_=0;
    verticalVelocity_=verticalAcceleration_=0;
    const double reachGoal=drop_.phase()==DropPhase::Airborne && drop_.velocityY()>0 ? .995 : .82;
    airborneReach_.step(dt,reachGoal);
    airborneSway_.step(dt,-.16*std::clamp(drop_.velocityX()/(12*supportReach_),-1.,1.));
    intendedLean_*=std::exp(-12*dt);
    if(std::abs(intendedLean_)<.0001) intendedLean_=0;
    lean_.step(dt,0); head_.step(dt,0); ears_.step(dt,0); tail_.step(dt,0);
  } else {
    const double landingLean=facing_*.14*drop_.impactStrength();
    intendedLean_=0;
    if(drop_.phase()==DropPhase::Recovering && !recoveryStepStarted_ && drop_.impactStrength()>.35) {
      recoveryStepStarted_=true;
      const int side=quadruped() ? 2 : (facing_>0 ? 0 : 1);
      auto &foot=feet_[side]; foot.swing=true; foot.recovering=true;
      swingStart_[side]=foot.position; swingTime_[side]=0;
      swingDuration_[side]=tuning_.maxStepDuration;
      foot.landing={touchdownX(foot.position.x+facing_*.12*supportReach_*drop_.impactStrength()),ground_};
      nextFoot_=followingFoot(side);
    }
    stepSupport(dt);
    const auto body=definition_.bindings.at("body");
    const double maximum=tuning_.groundY-.3*supportReach_-definition_.bones[body].rest.y-bodyLowering_;
    if(vertical_>maximum) { vertical_=maximum; verticalVelocity_=std::min(0.,verticalVelocity_); }
    lean_.step(dt,landingLean);
    head_.step(dt,-landingLean*.35); ears_.step(dt,-lean_.value()*.7);
    tail_.step(dt,-landingLean*.5);
    for(int side=0;side<int(feet_.size());++side) if(feet_[side].swing) stepSwing(side,dt);
  }
  legacy_.step(dt);
  if(!drop_.detached()) { wings_.goal(0,0,0); wings_.step(dt); }
  if(!drop_.detached()) drop_.recover(dt,dynamicsSettled() && !std::any_of(feet_.begin(),feet_.end(),[](const FootState &foot){return foot.swing;}));
}
void GroundedController::stepSwing(int side, double dt) {
  auto &foot=feet_[side];
  swingTime_[side]+=dt;
  const double t=std::clamp(swingTime_[side]/swingDuration_[side],0.,1.);
  foot.phase=t;
  const double smooth=t*t*t*(10+t*(-15+6*t));
  foot.position={swingStart_[side].x+(foot.landing.x-swingStart_[side].x)*smooth,
                 ground_-tuning_.lift*64*std::pow(t*(1-t),3)};
  if(t==1) {
    foot.position=foot.landing; foot.swing=false; foot.phase=0; foot.recovering=false;
    launchWait_=footPlantDuration_;
  }
}
bool GroundedController::hop(double height) {
 if(quadruped())return false;
 finite(height);if(height<=0||height>4*supportReach_)throw std::runtime_error("hop height outside anatomy bounds");
 if(drop_.phase()!=DropPhase::Grounded)return false;
 cancelFlight();const double horizontal=velocity_;
 // Capture presented root, clear gait, and launch without a held phase.
 drop_.launch({horizontal,-std::sqrt(2*18*supportReach_*height)});
 pending_=0;supportContacts_.fill(false);supportLoad_.fill(0);hasSupportGeometry_=false;
 swingTime_.fill(0);swingDuration_.fill(0);verticalVelocity_=verticalAcceleration_=0;
 for(int i=0;i<int(feet_.size());++i)feet_[i]={{rootX_+homeX(i),ground_},{},true};
 return true;
}
bool GroundedController::flightCapable() const { return !quadruped() && definition_.bindings.count("wingShoulderLeft")&&definition_.bindings.count("wingShoulderRight"); }
bool GroundedController::flyTo(double x,double elevation) {
 finite(x);finite(elevation);if(elevation<0||elevation>1e6)throw std::runtime_error("invalid flight destination");
 if(!flightCapable()||drop_.phase()==DropPhase::Held||drop_.phase()==DropPhase::Landing||drop_.phase()==DropPhase::Recovering)return false;
 if(drop_.phase()==DropPhase::Grounded)hop(std::min(supportReach_,std::max(1.,elevation*.3)));
 // Flight needs enough push-off time for the solved wings to extend and
 // accelerate. Tiny walking legs must not cut that time short.
 if(!poweredFlight_ && drop_.velocityY()<0 && drop_.elevation()<flightReach_) {
   // Reserve at least half a wing-reference of ballistic clearance: the
   // extension follower and first powered stroke need real flight time even
   // when the requested hover target is only a little above the support.
   const double launchHeight=std::clamp(elevation*.3,.5*flightReach_,flightReach_);
   const double impulse=std::sqrt(2*18*flightReach_*launchHeight);
   if(drop_.velocityY()>-impulse)drop_.release({drop_.velocityX(),-impulse});
 }
 drop_.useFlightGravity(flightReach_);
 landingFlight_=elevation==0;poweredFlight_=true;flightTargetX_=std::clamp(x,span_.minimum,span_.maximum);flightTargetElevation_=elevation;pending_=0;
 if(std::abs(flightTargetX_-rootX_)>flightReach_*.2)requestedFacing_=flightTargetX_>rootX_?1:-1;
 return true;
}
void GroundedController::land() {
 if(poweredFlight_){landingFlight_=true;flightTargetElevation_=0;flightTargetX_=rootX_;pending_=0;}
}
void GroundedController::cancelFlight() { poweredFlight_=false;landingFlight_=false;pending_=0;wings_.goal(0,0,0); }
void GroundedController::setFlightEffortLimit(double value) { finite(value);if(value<0||value>1)throw std::runtime_error("invalid flight effort limit");flightEffortLimit_=value; }

void GroundedController::setTargetX(double x) {
  finite(x);
  if(drop_.phase()!=DropPhase::Grounded) return;
  target_ = std::clamp(x, span_.minimum, span_.maximum);
  // Face the requested journey, not fractional host placement error on arrival.
  if (std::abs(target_-rootX_) > .75)
    requestedFacing_ = target_ > rootX_ ? 1 : -1;
  else
    requestedFacing_ = facing_;
}
bool GroundedController::face(double worldX) {
  if (!std::isfinite(worldX) || drop_.phase()!=DropPhase::Grounded)
    return false;
  const double delta=worldX-rootX_;
  if (std::abs(delta)<=.75) return false;
  const int direction=delta>0 ? 1 : -1;
  if (direction==requestedFacing_) return false;
  requestedFacing_=direction;
  return true;
}
MotionProposal GroundedController::propose(double dt) {
  finite(dt);
  if (dt < 0)
    throw std::runtime_error("negative ground delta");
  pending_ = std::min(dt, 1. / 30);
  if(drop_.phase()!=DropPhase::Grounded) {
    proposedDrop_=drop_;proposedWings_=wings_;proposedFlight_={};
    const int count=std::max(1,int(std::ceil(pending_*240)));const double h=pending_/count;
    for(int step=0;step<count;++step) {
      double effort=0,attitude=0;
      if(poweredFlight_&&proposedDrop_.phase()==DropPhase::Airborne) {
        const double error=flightTargetElevation_-proposedDrop_.elevation();
        // Banking tilts the realized lift vector away from vertical. Ask the
        // wings for extra effort so responsive steering does not lose height.
        const double verticalAuthority=std::cos(proposedWings_.state()[0].attitude);
        effort=std::clamp((.43+error/flightReach_*.3+proposedDrop_.velocityY()/flightReach_*.1)/verticalAuthority,0.,1.)*flightEffortLimit_;
        // Landing reduces actuation near the selected floor; no altitude clamp.
        if(landingFlight_)effort=.25*std::clamp(proposedDrop_.elevation()/flightReach_,0.,1.)*flightEffortLimit_;
        const double dx=flightTargetX_-(rootX_+proposedFlight_.displacementX);
        attitude=std::clamp(dx/flightReach_*.30-proposedDrop_.velocityX()/flightReach_*.16,-.65,.65);
      }
      proposedWings_.goal(poweredFlight_&&proposedDrop_.phase()==DropPhase::Airborne?1.:0.,effort,attitude);proposedWings_.step(h);
      auto forces=flightCapable()?proposedWings_.forces(flightReach_,{proposedDrop_.velocityX(),proposedDrop_.velocityY()}):FlightForces{};
      const auto delta=proposedDrop_.advance(h,forces);
      proposedFlight_.displacementX+=delta.displacementX;proposedFlight_.displacementY+=delta.displacementY;
      proposedFlight_.impactVelocity=std::max(proposedFlight_.impactVelocity,delta.impactVelocity);
    }
    return {std::clamp(rootX_+proposedFlight_.displacementX,span_.minimum,span_.maximum)-rootX_,
            rootX_,proposedFlight_.displacementY};
  }
  if (pending_ == 0) return {0, target_};
  double distance = target_ - rootX_;
  auto travel=tuning_;
  travel.speed*=travelScale_;
  double move = navigationStep(distance,pending_,travel,velocity_,acceleration_);
  if (std::abs(distance)<.02 && std::abs(velocity_)<.15 && std::abs(acceleration_)<1.) {
    move = distance;
    velocity_ = acceleration_ = 0;
  }
  return {std::clamp(rootX_ + move, span_.minimum, span_.maximum) - rootX_, target_};
}
void GroundedController::realize(double x, bool blocked, std::optional<double> elevation, double ceilingTolerance) {
  finite(x);
  if(elevation) finite(*elevation);
  double dt = pending_;
  pending_ = 0;
  if (dt == 0)
    return;
  const bool hostAdjusted = std::abs(x-(rootX_+velocity_*dt)) > 1e-7;
  rootX_ = std::clamp(x, span_.minimum, span_.maximum);
  if(drop_.phase()!=DropPhase::Grounded) {
    const auto previous=drop_.phase();
    const int body=definition_.bindings.at("body");
    const auto presentedBody=bodyPose(Action::Rest,0,{}).bones[body];
    const double presentedVertical=presentedBody.y-
        definition_.bones[body].rest.y-bodyLowering_;
    const double presentedLean=presentedBody.rotation-
        definition_.bones[body].rest.rotation;
    const double incoming=proposedFlight_.impactVelocity>0 ? proposedFlight_.impactVelocity : proposedDrop_.velocityY();
    drop_=proposedDrop_;wings_=proposedWings_;
    if(elevation) drop_.realizeElevation(*elevation,true,ceilingTolerance);
    if(blocked || (rootX_==span_.minimum && drop_.velocityX()<0) ||
                  (rootX_==span_.maximum && drop_.velocityX()>0)) drop_.blockHorizontal();
    velocity_=drop_.velocityX(); target_=rootX_;
    if(previous==DropPhase::Airborne && drop_.phase()==DropPhase::Landing) {
      poweredFlight_=false;wings_.goal(0,0,0);
      installDropContacts();
      vertical_=presentedVertical;
      verticalVelocity_=supportedImpactSpeed(incoming,supportReach_);
      intendedLean_=0; lean_.reset(presentedLean);
      lean_.impulse(facing_*.6*drop_.impactStrength());
      head_.impulse(-facing_*.5*drop_.impactStrength());
      ears_.impulse(.8*drop_.impactStrength());
      tail_.impulse(-facing_*.7*drop_.impactStrength());
    } else if(drop_.detached()) {
      for(int side=0;side<int(feet_.size());++side) {
        feet_[side].position={rootX_+homeX(side),ground_-drop_.elevation()};
        feet_[side].swing=true;
      }
    }
    if(poweredFlight_ && !landingFlight_)facing_=requestedFacing_;
    stepDropFollowers(dt);
    return;
  }
  if (blocked) {
    target_ = rootX_;
    velocity_ = acceleration_ = 0;
  } else if ((std::abs(target_ - rootX_) < .02 && std::abs(velocity_) < .15 &&
              std::abs(acceleration_) < 1.) ||
             (hostAdjusted && std::abs(target_-rootX_) <= .75 &&
              std::abs(velocity_) < 2 && std::abs(acceleration_) < 4)) {
    // Integer desktop placement can never reach a fractional target exactly.
    // Accept a nearby realized root only after travel has slowed to rest.
    target_ = rootX_;
    velocity_ = acceleration_ = 0;
  }
  facing_ = requestedFacing_;
  // A brief edge-on passage connects the two authored quarter views. This
  // affects only the visual branch, never the world-space support contacts.
  double turn = 2 * dt / .18;
  facingBlend_ += std::clamp(facing_ - facingBlend_, -turn, turn);
  double acceleration = (velocity_ - lastVelocity_) / dt;
  lastVelocity_ = velocity_;
  // Torso balance is driven by intent in the same interval as navigation;
  // only the small settling response is delayed. Acceleration-only lean left
  // a vertical torso following already-moving hips and reversed on braking.
  double direction = std::abs(target_ - rootX_) > .02
                         ? std::copysign(1., target_ - rootX_) : 0;
  intendedLean_ = direction * .018 * std::min(1., std::abs(target_ - rootX_) /
                                                     tuning_.stride) +
                  .045 * std::clamp(velocity_ / tuning_.speed, -1., 1.);
  lean_.step(dt, std::clamp(acceleration * .00008, -.018, .018));
  armSpread_.step(dt,0);
  wings_.step(dt);
  stepSupport(dt);
  head_.step(dt, -intendedLean_ * .35);
  ears_.step(dt, -lean_.value() * .7 + velocity_ / speedMultiplier_ * .001);
  tail_.step(dt, -velocity_ / speedMultiplier_ * .004);
  legacy_.step(dt);
  // The body follows navigation; the legs are visual followers, not brakes.
  // A foot may leave its pinned contact only after the other has landed.
  launchWait_ = std::max(0., launchWait_ - dt);
  bool moving = std::abs(target_ - rootX_) > .02 || std::abs(velocity_) > .02;
  auto hipWorld = worldTransforms(definition_, bodyPose(Action::Rest, 0, {}));
  auto footReach = [&](int side, double footY) {
  auto leg = tuning_.legs[side];
  double y = tuning_.groundY + footY-ground_ - leg.contact.y;
  double dy = y-hipWorld[leg.hip].ty;
  double radius = lengthA_[side]+lengthB_[side]-.01;
  double extent = std::sqrt(std::max(0., radius*radius-dy*dy));
  double center = rootX_ - definition_.origin.x + hipWorld[leg.hip].tx + leg.contact.x;
  GroundSpan interval{center-extent, center+extent};
  double fold = std::abs(lengthA_[side]-lengthB_[side])+.01;
  if (std::abs(dy) < fold) {
    double inner = std::sqrt(fold*fold-dy*dy);
    if (facing_ > 0) interval.minimum = center+inner;
    else interval.maximum = center-inner;
  }
  return interval;
  };
  if(pairedGait_) {
    // Keep an already lifted trajectory intact; a mode/target change never
    // releases its supporting diagonal before touchdown and load transfer.
    const bool swinging=std::any_of(feet_.begin(),feet_.end(),[](const auto& f){return f.swing;});
    const int lead=nextFoot_<2?nextFoot_:3-nextFoot_;
    const int mate=3-lead;
    bool supportsReady=true;
    for(int i=0;i<4;++i)if(i!=lead&&i!=mate)
      supportsReady=supportsReady&&!feet_[i].swing&&supportContacts_[i]&&
          std::abs(feet_[i].position.y-ground_)<1e-7&&supportLoad_[i]>=1.;
    const bool neutral=std::abs(feet_[lead].position.x-touchdownX(rootX_+homeX(lead)))>footHomeRadius_||
        std::abs(feet_[mate].position.x-touchdownX(rootX_+homeX(mate)))>footHomeRadius_;
    if(!swinging&&swingAdmission_&&supportsReady&&launchWait_==0&&(moving||neutral)) {
      // Forecast through touchdown and halfway through the following stance.
      // Navigation retains its requested speed; the contact catches ahead of
      // the future hip rather than chasing its position at launch.
      const double duration=tuning_.maxStepDuration;
      const double future=predictedRoot(rootX_,velocity_,acceleration_,target_,tuning_,duration);
      const double stanceMid=predictedRoot(rootX_,velocity_,acceleration_,target_,tuning_,
                                          duration+.5*duration+std::max(footPlantDuration_,supportTransferTime()));
      for(int i:{lead,mate}) {
        auto& foot=feet_[i];foot.swing=true;foot.recovering=false;
        swingStart_[i]=foot.position;swingTime_[i]=0;swingDuration_[i]=duration;
        const auto reach=footReach(i,ground_);
        const double leadDistance=std::clamp(stanceMid-future,
            -.5*(reach.maximum-reach.minimum),.5*(reach.maximum-reach.minimum));
        foot.landing={touchdownX(future+homeX(i)+leadDistance),ground_};
      }
      nextFoot_=1-lead;
      launchWait_=duration+footPlantDuration_;
    } else if(!swinging&&!moving&&!neutral) {
      // At rest a comfortable pair may be skipped, so it cannot starve the
      // other pair's final posture recovery.
      nextFoot_=1-lead;
    }
    for(int i=0;i<4;++i)if(feet_[i].swing)stepSwing(i,dt);
    return;
  }
  // Visit quad candidates in the same cyclic order during travel and a rest turn.
  // Comfortable homes may be skipped at rest without starving later contacts.
  int candidate = nextFoot_;
  for (int ordinal = 0; ordinal < int(feet_.size()); ++ordinal) {
    const int i = quadruped() ? candidate : ordinal;
    if(quadruped()) candidate = followingFoot(candidate);
    auto &f = feet_[i];
    f.recovering = false;
    auto reach = footReach(i, ground_);
    double future = predictedRoot(rootX_, velocity_, acceleration_, target_, tuning_,
                                  tuning_.maxStepDuration);
    const double boundedHome = touchdownX(rootX_+homeX(i));
    bool neutral = std::abs(f.position.x - boundedHome) > footHomeRadius_;
    bool supportsReady = quadruped() ? true : !feet_[1-i].swing;
    if(quadruped()) {
      supportsReady = true;
      for(int j=0;j<int(feet_.size());++j) if(j!=i)
        supportsReady = supportsReady && !feet_[j].swing && supportContacts_[j] &&
            std::abs(feet_[j].position.y-ground_)<1e-7 && supportLoad_[j]>=1.;
    }
    bool routine = swingAdmission_ && supportsReady && launchWait_ == 0 &&
        ((moving && i == nextFoot_ && (std::abs(target_-rootX_) > footHomeRadius_ || neutral)) ||
         (!moving && neutral));
    if (!f.swing && routine) {
      f.swing = true;
      swingStart_[i] = f.position;
      swingTime_[i] = 0;
      swingDuration_[i] = tuning_.maxStepDuration;
      // Aim ahead of the future hip, inside the real horizontal reach interval.
      double ahead = moving ? std::copysign((reach.maximum-reach.minimum)*landingLeadFraction_, velocity_) : 0;
      if(centeredSteps_&&moving){
        const double midpoint=predictedRoot(rootX_,velocity_,acceleration_,target_,tuning_,
          1.5*tuning_.maxStepDuration+std::max(footPlantDuration_,supportTransferTime()));
        ahead=std::clamp(midpoint-future,-.5*(reach.maximum-reach.minimum),.5*(reach.maximum-reach.minimum));
      }
      f.landing = {touchdownX(future + homeX(i) + ahead), ground_};
      nextFoot_ = followingFoot(i);
      launchWait_ = tuning_.maxStepDuration + footPlantDuration_;
    }
    if (f.swing) {
      stepSwing(i,dt);
    }

  }

}
void GroundedController::stepSupport(double dt) {
  const auto hips = worldTransforms(definition_, bodyPose(Action::Rest,0,{}));
  LimbArray<pets::motion::SupportLeg> legs;
  legs.setCount(feet_.size());
  for (int i = 0; i < int(feet_.size()); ++i) {
    auto leg = tuning_.legs[i];
    bool grounded = hasSupportGeometry_ ? supportGeometry_[i].load > 0 :
        !feet_[i].swing && std::abs(feet_[i].position.y-ground_) < 1e-7;
    grounded = grounded && supportContacts_[i];
    supportLoad_[i] = grounded ? std::min(1.,supportLoad_[i]+dt/supportTransferTime()) : 0;
    if (hasSupportGeometry_) legs[i] = supportGeometry_[i];
    else legs[i] = {{hips[leg.hip].tx,hips[leg.hip].ty,0},
        {feet_[i].position.x-rootX_+definition_.origin.x-leg.contact.x,
         tuning_.groundY+feet_[i].position.y-ground_-leg.contact.y,0},
        lengthA_[i],lengthB_[i],1,springRest_[i]};
    double load = supportLoad_[i];
    legs[i].load *= load*load*load*(10+load*(-15+6*load));
  }
  hasSupportGeometry_ = false;
  pets::motion::SupportState state{vertical_,verticalVelocity_,verticalAcceleration_};
  pets::motion::SupportDynamics dynamics{supportTuning_.stiffness,supportTuning_.damping,gravity_,
      supportTuning_.uprightStiffness,supportTuning_.uprightDamping,0,
      supportTuning_.contactLossGravityScale,supportTuning_.airborneDamping};
  if(quadruped()) pets::motion::stepSupport(state,legs.array<4>(),dt,dynamics);
  else pets::motion::stepSupport(state,legs.array<2>(),dt,dynamics);
  vertical_ = state.offset; verticalVelocity_ = state.velocity; verticalAcceleration_ = state.acceleration;
  if(heldBodyDown_)holdBodyHeight(heldBodyDown_);
  else if(minimumBodyDown_)constrainBodyRise(*minimumBodyDown_);
}
Pose GroundedController::bodyPose(Action action, double time, Vec2 gaze) const {
  Pose p =
      legacy_.pose(action == Action::Walk ? Action::Rest : action, time, gaze);
  auto body = definition_.bindings.at("body");
  p.bones[body].scaleX = p.bones[body].scaleY = 1;
  p.bones[body].x = definition_.bones[body].rest.x;
  p.bones[body].rotation = definition_.bones[body].rest.rotation +
      std::clamp(intendedLean_ + lean_.value(), -.15, .15)*(1-landingPreparation(drop_,supportReach_));
  p.bones[body].y = definition_.bones[body].rest.y + bodyLowering_ +
      vertical_*(1-landingPreparation(drop_,supportReach_))-
      landingBodyLift_*landingPreparation(drop_,supportReach_);
  if(pairedGait_&&!heldBodyDown_&&drop_.phase()==DropPhase::Grounded&&std::abs(velocity_)>.02){
    // Small compression as a diagonal recovers makes the weight transfer
    // readable without changing navigation pace or extending either leg.
    double recovery=0;
    for(const auto& foot:feet_)if(foot.swing)
      recovery=std::max(recovery,std::sin(3.141592653589793*foot.phase));
    p.bones[body].y+=std::min(1.5,.025*supportReach_)*recovery;
  }
  for(auto side:{std::pair<const char*,double>{"Left",1},{"Right",-1}}) {
    auto shoulder=definition_.bindings.find(std::string("arm")+side.first);
    auto forearm=definition_.bindings.find(std::string("forearm")+side.first);
    if(shoulder!=definition_.bindings.end())
      p.bones[shoulder->second].rotation+=side.second*armSpread_.value();
    if(forearm!=definition_.bindings.end())
      p.bones[forearm->second].rotation-=side.second*.15*armSpread_.value();
  }
  for (auto role : {"earLeft", "earRight", "tail"}) {
    auto follower = definition_.bindings.find(role);
    if (follower != definition_.bindings.end())
      p.bones[follower->second].rotation += std::clamp(
          std::string(role) == "tail" ? tail_.value() : ears_.value(), -.25,
          .25);
  }
  auto head = definition_.bindings.find("head");
  if (head != definition_.bindings.end())
    p.bones[head->second].rotation += std::clamp(head_.value(), -.15, .15);
  for (size_t i = 0; i < p.bones.size(); i++)
    p.bones[i].rotation =
        std::clamp(p.bones[i].rotation, definition_.bones[i].minRotation,
                   definition_.bones[i].maxRotation);
  for (auto l : tuning_.legs)
    for (int i : {l.hip, l.knee, l.ankle})
      p.bones[i] = definition_.bones[i].rest;
  if(quadruped()) {
    const auto authored = worldTransforms(definition_,p);
    const auto anchor = authored[body];
    for(const auto &leg:tuning_.legs) {
      const auto old=authored[leg.hip];
      const double x=anchor.tx+(old.tx-anchor.tx)*facingBlend_;
      const double y=old.ty;
      const int parent=definition_.bones[leg.hip].parent;
      const auto frame=parent<0 ? WorldTransform{} : authored[parent];
      const double determinant=frame.m00*frame.m11-frame.m01*frame.m10;
      p.bones[leg.hip].x=(frame.m11*(x-frame.tx)-frame.m01*(y-frame.ty))/determinant;
      p.bones[leg.hip].y=(-frame.m10*(x-frame.tx)+frame.m00*(y-frame.ty))/determinant;
    }
  }
  return p;
}
Pose GroundedController::pose(Action action, double time, Vec2 gaze) const {
  if(drop_.phase()!=DropPhase::Grounded) { action=Action::Rest; time=0; gaze={}; }
  auto p = bodyPose(action, time, gaze);
  auto w = worldTransforms(definition_, p);
  for (int i = 0; i < int(feet_.size()); i++) {
    auto l = tuning_.legs[i];
    Vec2 hip{w[l.hip].tx, w[l.hip].ty};
    Vec2 target{feet_[i].position.x - rootX_ + definition_.origin.x,
                tuning_.groundY + feet_[i].position.y -
                    ground_}; // contacts stay oriented horizontally
    target.x -= l.contact.x;
    target.y -= l.contact.y;
    if(drop_.detached()) {
      const double reach=lengthA_[i]+lengthB_[i];
      const double prepare=landingPreparation(drop_,supportReach_);
      const Vec2 hanging{hip.x+reach*(airborneSway_.value()+(quadruped() ? 0 : (i==0 ? -.08 : .08))),
                         hip.y+reach*std::clamp(airborneReach_.value(),.65,.995)};
      const Vec2 contact{definition_.origin.x+homeX(i)-l.contact.x,
                         tuning_.groundY-l.contact.y};
      target={hanging.x+(contact.x-hanging.x)*prepare,
              hanging.y+(contact.y-hanging.y)*prepare};
    }
    // The pole switches only during the edge-on passage. A 2D two-link
    // chain has two discrete branches; interpolating the knee would shorten
    // bones and slide the contact instead of making an anatomical turn.
    int viewDirection = facingBlend_ < 0 ? -1 : 1;
    auto solution = solveTwoLink(hip, target, lengthA_[i], lengthB_[i],
                                 l.bend * viewDirection, !drop_.detached());
    double parentAngle = 0;
    int parent = definition_.bones[l.hip].parent;
    if (parent >= 0)
      parentAngle = std::atan2(w[parent].m10, w[parent].m00);
    auto kneeRest = definition_.bones[l.knee].rest,
         ankleRest = definition_.bones[l.ankle].rest;
    double upper = angle({solution.knee.x - hip.x, solution.knee.y - hip.y});
    double lowerAngle = angle(
        {solution.end.x - solution.knee.x, solution.end.y - solution.knee.y});
    // Child translations encode any extension chosen by contact-preserving IK.
    // This makes the rendered chain reach the pinned sole, not just its angles.
    double upperScale = std::hypot(solution.knee.x-hip.x,solution.knee.y-hip.y)/lengthA_[i];
    double lowerScale = std::hypot(solution.end.x-solution.knee.x,solution.end.y-solution.knee.y)/lengthB_[i];
    p.bones[l.knee].x = kneeRest.x*upperScale;
    p.bones[l.knee].y = kneeRest.y*upperScale;
    p.bones[l.ankle].x = ankleRest.x*lowerScale;
    p.bones[l.ankle].y = ankleRest.y*lowerScale;
    p.bones[l.hip].rotation =
        wrap(upper - angle({kneeRest.x, kneeRest.y}) - parentAngle);
    p.bones[l.knee].rotation =
        wrap(lowerAngle - angle({ankleRest.x, ankleRest.y}) - upper +
             angle({kneeRest.x, kneeRest.y}));
    p.bones[l.ankle].rotation =
        wrap(-lowerAngle + angle({ankleRest.x, ankleRest.y}));
  }
  if(flightCapable()) {
    const auto wings=wings_.state();
    for(int side=0;side<2;++side) {
      const std::string suffix=side==0?"Left":"Right";
      const auto& wing=wings[side];
      // Raven feathers sweep aft from the authored side-view shoulder.
      // Limit only their projected stroke; the solved wing state still drives
      // unchanged physical lift and thrust in the shared controller.
      const double shoulder=definition_.id=="raven"
          ? .18*wing.shoulder+(side==0?.20:-.20)*wing.extension
          : wing.shoulder+1.25*wing.extension;
      for(const auto entry:{std::pair<const char*,double>{"wingShoulder",shoulder},{"wingElbow",(wing.elbow+1.35)*(pi/1.35)},{"wingWrist",(wing.wrist-.85)*(pi/.85)}}) {
        int joint=definition_.bindings.at(std::string(entry.first)+suffix);
        p.bones[joint].rotation=std::clamp(definition_.bones[joint].rest.rotation+(side==0?1:-1)*entry.second,definition_.bones[joint].minRotation,definition_.bones[joint].maxRotation);
      }
    }
  }
  applyFacing(definition_, p, facingBlend_);
  return p;
}
bool GroundedController::dynamicsSettled() const {
  return requestedFacing_ == facing_ && facingBlend_ == facing_ &&
         lean_.settled(0) && std::abs(verticalAcceleration_) < .1 &&
         std::abs(verticalVelocity_) < .03 && head_.settled(0) &&
         ears_.settled(0) && tail_.settled(0) && legacy_.settled() && wings_.settled() && armSpread_.settled(0);
}
bool GroundedController::settled() const {
  if(drop_.phase()==DropPhase::Held)
    return dynamicsSettled() && vertical_==0 && intendedLean_==0 &&
           airborneReach_.settled(.82) && airborneSway_.settled(0);
  if(drop_.phase()!=DropPhase::Grounded) return false;
  return std::abs(target_-rootX_)<.02 && velocity_==0 && !std::any_of(feet_.begin(),feet_.end(),[](const FootState &foot){return foot.swing;}) && dynamicsSettled();
}
GroundedState GroundedController::state() const {
  return {rootX_, velocity_, target_, facing_, feet_, !settled(), facingBlend_,
          drop_.phase()==DropPhase::Airborne ? drop_.velocityY() : verticalVelocity_,
          verticalAcceleration_,drop_.phase(),drop_.elevation(),wings_.state(),poweredFlight_};
}
void GroundedController::impulse(Vec2 d, double m) {
  finite(d.x);
  finite(d.y);
  finite(m);
  double n = std::hypot(d.x, d.y);
  if (n == 0)
    return;
  m = std::clamp(m, -4., 4.);
  lean_.impulse(d.x / n * m);
  verticalVelocity_ += d.y / n * m * 2;
  head_.impulse(-d.x / n * m);
  ears_.impulse(d.x / n * m * .7);
  tail_.impulse(-d.x / n * m * 1.3);
}
} // namespace pets::character
