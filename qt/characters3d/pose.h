#pragma once
#include "characters/grounded.h"
#include "characters/motion3d.h"
#include "characters/expression.h"
#include <array>
#include <optional>
namespace pets::character3d {
using Vec3 = pets::motion::Vec3;
using pets::character::LimbArray;
struct Leg { Vec3 hip,knee,ankle,contact; };
struct LegPose { Vec3 hip,knee,ankle; };
// Rest geometry is model-axis aligned. Expressive wrists rotate the grip offset
// before IK; solved snapshots retain that alignment throughout a reach.
struct Arm { Vec3 shoulder,elbow,hand,gripOffset,bend; Vec3 torsoPivot{0,1.22,0}; };
struct ArmPose { Vec3 shoulder,elbow,hand,grip; };
ArmPose solveArm(const Arm &, Vec3 gripTarget);
struct Pose {
 double rootX=0, yaw=0, lean=0, follower=0;
 Vec3 head{}, tail{};
 std::array<Vec3,2> ears{}, wrists{};
 std::array<pets::character::WingState,2> wings{};
 LimbArray<LegPose> legs;
 std::array<std::optional<ArmPose>,2> arms;
 // Authoritative world support points and absolute foot yaw (radians).
 LimbArray<Vec3> contacts;
 LimbArray<double> footYaw{};
 // Independent ankle pitch; supported paws remain flat.
 LimbArray<double> footPitch{};
 LimbArray<bool> swing{};
 // Physical support is independent of lifted/recovering animation flags.
 LimbArray<bool> grounded{};
};
LegPose solveLeg(Vec3 hip,Vec3 target,double upper,double lower,Vec3 bend,bool preserveTarget=false);
class Motion {
public:
 // Quadruped legs use canonical foreLeft, foreRight, hindLeft, hindRight order.
 // Pair arrays retain the legacy biped constructor semantics through LimbArray.
 explicit Motion(LimbArray<Leg>, std::optional<std::array<Arm,2>> arms = {}, bool wings = false, double wingReferenceReach = 0, pets::character::Gait gait = pets::character::Gait::Biped);
 void armGoal(int side, Vec3 modelGripTarget);
 void clearArmGoal(int side);
 bool reach(int side, Vec3 normalizedGripTarget);
 // Declared reference body height, meters; reach coordinates are fractions.
 static constexpr double referenceHeight() { return 2.; }
 pets::character::ExpressionMotion& expressions() { return expressions_; }
 const pets::character::ExpressionMotion& expressions() const { return expressions_; }
 void setSpeedMultiplier(double);
 void setFootPlantDuration(double seconds);
 // Resident-only signed heading calibration; the preview retains zero bias.
 void setFacingBias(double radians);
 // Configure before reset: mirror left/right headings toward the resident
 // camera, with brisk bounded yaw response. Preview callers retain the default
 // additive heading policy; switching a live back-facing stance needs replants.
 void setCameraFacingTurns(bool enabled);
 double speedMultiplier() const { return planner_.speedMultiplier(); }
 // Desktop coordinates use the existing planner's centimetre travel units.
 // Explicit reset clears dynamics; pickup and reconcile own live placement.
 void reset(double rootX, pets::character::GroundSpan, double floorWorldY = 0);
 void pickup(double rootX, double elevation);
 void hold(double rootX, double elevation);
 void release(pets::character::Vec2 velocity);
 void reconcile(double rootX, pets::character::GroundSpan, double elevation, std::optional<double> floorWorldY = {});
 void setSupportFrame(pets::character::GroundSpan, double floorWorldY);
 void setSupportBounds(std::optional<pets::character::GroundSpan> absoluteSoleInterval);
 bool flightCapable() const;
 bool hop(double height);
 bool flyTo(double worldX,double elevation);
 void land();
 void cancelFlight();
 void setSpan(pets::character::GroundSpan);
 void target(double);
 // Face a point on the shared lateral travel axis without changing navigation.
 // Returns true only for a changed heading outside the comfortable home radius.
 bool face(double worldX);
 // Proposal leaves the complete presented state unchanged. Realize consumes
 // one interval; blocked feedback cancels the unreachable navigation target.
 pets::character::MotionProposal propose(double dt);
 void realize(double rootX, bool blocked = false, std::optional<double> elevation = {}, double ceilingTolerance = 1);
 void step(double);
 void impulse();
 Pose pose() const;
 pets::character::GroundedState state() const;
 bool settled() const;
private:
 LimbArray<Leg> legs_;
 std::optional<std::array<Arm,2>> arms_;
 std::array<std::optional<Vec3>,2> armGoals_;
 std::array<std::array<pets::character::SecondOrderTracker,3>,2> reach_;
 pets::character::ExpressionMotion expressions_;
 bool reachSettled() const;
 Vec3 armFollowerGoal(int side, double bodyOffset) const;
 pets::character::GroundedController planner_;
 std::optional<pets::character::GroundedController> pendingPlanner_;
 double pendingDelta_=0;
 pets::character::SecondOrderTracker yaw_, follower_;
 LimbArray<Vec3> contacts_;
 LimbArray<double> footYaw_{};
 LimbArray<double> swingStartYaw_{}, swingEndYaw_{};
 LimbArray<Vec3> swingContactOffset_{};
 // Planner coordinates keep the heading that authored each in-flight step.
 LimbArray<int> swingFacing_{};
 LimbArray<double> supportRestLength_{};

 Vec3 turnStart_;
 int turningFoot_=-1;
 double turnTime_=0, turnStartYaw_=0, turnEndYaw_=0, turnCooldown_=0;
 double footPlantDuration_=pets::character::defaultFootPlantDuration;
 double yawTarget_=0, requestedTarget_=0, facingBias_=0;
 bool cameraFacingTurns_=false, facingLeft_=false;
 double supportFloor_=0;
 std::optional<pets::character::GroundSpan> supportBounds_;
 bool quadruped() const { return legs_.size()==4; }
 double yawStepGoal(int foot, double increment) const;
 bool feetTurning() const;
 void boundTouchdown(Vec3&) const;
 void retargetYaw();
 void clearPickupMotion();
 void installContacts();
};
}
