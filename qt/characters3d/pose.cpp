#include "pose.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character3d {
namespace {
constexpr double headingHomeRadius=4; // centimetres in the shared planner
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,double b){return {a.x*b,a.y*b,a.z*b};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
double length(Vec3 a){return std::sqrt(dot(a,a));}
double ease(double t){t=std::clamp(t,0.,1.);return t*t*t*(10+t*(-15+6*t));}
double lift(double t){t=std::clamp(t,0.,1.);return 64*std::pow(t*(1-t),3);}
Vec3 normalized(Vec3 a){return mul(a,1/std::max(1e-12,length(a)));}
Vec3 rotateRoll(Vec3 v,double angle){double c=std::cos(angle),s=std::sin(angle);return {c*v.x-s*v.y,s*v.x+c*v.y,v.z};}
Vec3 rotatePitch(Vec3 v,double pitch){double c=std::cos(pitch),s=std::sin(pitch);return {v.x,c*v.y-s*v.z,s*v.y+c*v.z};}
Vec3 rotateYaw(Vec3 v,double yaw){double c=std::cos(yaw),s=std::sin(yaw);return {c*v.x+s*v.z,v.y,-s*v.x+c*v.z};}
pets::character::Definition definition(LimbArray<Leg> legs,bool wings,double wingReferenceReach,pets::character::Gait gait){
 using namespace pets::character;
 if((gait==Gait::Biped&&legs.size()!=2)||(gait==Gait::QuadrupedWalk&&legs.size()!=4))throw std::runtime_error("spatial gait anatomy mismatch");
 if(legs.size()==4&&wings)throw std::runtime_error("quadruped wings unsupported");
 for(const auto& leg:legs){
  for(auto point:{leg.hip,leg.knee,leg.ankle,leg.contact})if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))throw std::runtime_error("invalid spatial leg");
  if(length(sub(leg.knee,leg.hip))<1e-6||length(sub(leg.ankle,leg.knee))<1e-6)throw std::runtime_error("degenerate spatial leg");
 }
 Definition d; d.id="werewolf-3d-planner";d.name="Werewolf ground intent";d.attachments.push_back({0,{0,0,1,1},{},{},0});d.interaction={{0,0},{256,0},{256,256}};d.controller="articulated";d.width=256;d.height=256;d.origin={128,220};d.motionEnvelope=20;
 double pelvis=legs[0].hip.y;for(const auto& leg:legs)pelvis=std::min(pelvis,leg.hip.y);
 d.bones.push_back({"pelvis",-1,{128,220-pelvis*100}});d.bindings["body"]=0;
 double reach=1e9;for(const auto& leg:legs)reach=std::min(reach,100*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee))));
 Locomotion l;l.legs.setCount(legs.size());l.gait=gait;l.groundY=220;
 // Measured reach bounds the contact envelope; spatial cadence and centered
 // touchdown forecasts are configured independently of navigation pace.
 l.minStepDuration=l.maxStepDuration=legs.size()==4?.24:.28;
 l.stride=std::clamp(reach*.32,.1,128.);
 // Shared natural rhythm: travel scales with measured two-link leg length.
 // 0.85 leg lengths/second keeps human-sized companions near their prior pace.
 // The existing speed multiplier applies on top of this anatomy-derived pace.
 l.speed=std::clamp(reach*.85,.1,500.);l.acceleration=240;
 l.reachThreshold=reach*.2;l.lift=std::min(8.,reach*.09);
 for(std::size_t i=0;i<legs.size();i++){
  auto a=legs[i];int base=d.bones.size();
  // The shared contact planner uses an equal-link proxy. Actual deform IK below
  // solves the independently measured unequal 3D limb lengths.
  double u=length(sub(a.knee,a.hip)),v=length(sub(a.ankle,a.knee));
  // Support soles are centered on the hip; preserve the actual paw-to-ankle
  // offset in the proxy, so its spring sees the same rearward ankle as IK.
  a.ankle.z=a.hip.z-(a.contact.z-a.ankle.z);
  double dz=a.ankle.z-a.hip.z,dy=a.ankle.y-a.hip.y,n=std::hypot(dz,dy),r=(u+v)*.5;
  double side=std::sqrt(std::max(0.,r*r-n*n*.25));
  a.knee.z=(a.hip.z+a.ankle.z)*.5-side*dy/std::max(n,1e-12);
  a.knee.y=(a.hip.y+a.ankle.y)*.5+side*dz/std::max(n,1e-12);
  d.bones.push_back({"hip"+std::to_string(i),0,{a.hip.z*100,(pelvis-a.hip.y)*100}});
  d.bones.push_back({"knee"+std::to_string(i),base,{(a.knee.z-a.hip.z)*100,(a.hip.y-a.knee.y)*100}});
  d.bones.push_back({"ankle"+std::to_string(i),base+1,{(a.ankle.z-a.knee.z)*100,(a.knee.y-a.ankle.y)*100}});
  // The authored paw extends forward of its ankle. Plan support around the
  // hip projection, rather than carrying that rest-paw lead into every step.
  l.legs[i]={base,base+1,base+2,{(a.hip.z-a.ankle.z)*100,(a.ankle.y-a.contact.y)*100},1};
 }
 // Virtual role bones let the adapter read the same shared impact followers
 // as planar characters without introducing another response simulation.
 for(const auto* role:{"head","earLeft","earRight","tail"}){
  d.bindings[role]=int(d.bones.size());d.bones.push_back({role,0,{}});
 }
 if(wings) {
  if(!std::isfinite(wingReferenceReach)||wingReferenceReach<0||wingReferenceReach>512)throw std::runtime_error("invalid spatial wing reference");
  const double wingSegment=wingReferenceReach>0?wingReferenceReach*.5:10;
  const double reach=100*std::min(length(sub(legs[0].knee,legs[0].hip))+length(sub(legs[0].ankle,legs[0].knee)),length(sub(legs[1].knee,legs[1].hip))+length(sub(legs[1].ankle,legs[1].knee)));
  l.lift=std::min(8.,reach*.09);
  for(const auto side:{"Left","Right"}) {
   int shoulder=d.bones.size();
   d.bindings[std::string("wingShoulder")+side]=shoulder;d.bones.push_back({std::string("wingShoulder")+side,0,{0,-8}});
   d.bindings[std::string("wingElbow")+side]=shoulder+1;d.bones.push_back({std::string("wingElbow")+side,shoulder,{wingSegment,0}});
   d.bindings[std::string("wingWrist")+side]=shoulder+2;d.bones.push_back({std::string("wingWrist")+side,shoulder+1,{wingSegment,0}});
  }
 }
 d.locomotion=l;return d;
}
}
LegPose solveLeg(Vec3 hip,Vec3 target,double upper,double lower,Vec3 bend,bool preserveTarget){
 auto solved=pets::motion::solveTwoLink(hip,target,upper,lower,bend,preserveTarget);
 return {solved.origin,solved.knee,solved.end};
}
ArmPose solveArm(const Arm &a, Vec3 goal) {
 auto finite=[](Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
 if(!finite(a.shoulder)||!finite(a.elbow)||!finite(a.hand)||!finite(a.gripOffset)||!finite(a.bend)||!finite(goal))throw std::runtime_error("invalid arm fixture or target");
 auto solved=solveLeg(a.shoulder,sub(goal,a.gripOffset),length(sub(a.elbow,a.shoulder)),length(sub(a.hand,a.elbow)),a.bend);
 return {solved.hip,solved.knee,solved.ankle,add(solved.ankle,a.gripOffset)};
}
void Motion::armGoal(int side,Vec3 target) {
 if(side<0||side>1||!arms_)throw std::runtime_error("missing arm capability");
 if(!std::isfinite(target.x)||!std::isfinite(target.y)||!std::isfinite(target.z))throw std::runtime_error("invalid arm target");
 const auto& arm=(*arms_)[side];
 const auto neutral=add(arm.hand,arm.gripOffset);
 // Keep endpoints bounded around the measured chain; IK handles exact reach.
 armGoals_[side]={std::clamp(target.x,neutral.x-1.,neutral.x+1.),std::clamp(target.y,.2,2.2),std::clamp(target.z,-.65,.65)};
}
bool Motion::reach(int side,Vec3 v){
 if(side<0||side>1||!arms_||!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))return false;
 armGoal(side,mul(v,referenceHeight()));return true;
}
Vec3 Motion::armFollowerGoal(int side,double bodyOffset)const{
 const auto& arm=(*arms_)[side];
 if(armGoals_[side])return *armGoals_[side];
 auto goal=add(arm.hand,arm.gripOffset);
 const auto state=planner_.state();
 if(state.phase==pets::character::DropPhase::Airborne&&!state.poweredFlight){
  // A body-relative balance response, solved with measured anatomy. Existing
  // endpoint followers open the arms on release and ease them back on landing.
  const double reach=length(sub(arm.elbow,arm.shoulder))+length(sub(arm.hand,arm.elbow));
  const double sideSign=arm.shoulder.x>0?1.:-1.;
  goal={arm.shoulder.x+sideSign*.88*reach,arm.shoulder.y-.12*reach,arm.shoulder.z+.1*reach};
 }
 goal.y+=bodyOffset;
 return goal;
}
bool Motion::reachSettled()const{
 if(!arms_)return true;
 const double bodyOffset=pose().legs[0].hip.y-legs_[0].hip.y;
 for(int i=0;i<2;++i){auto goal=armFollowerGoal(i,bodyOffset);double values[]={goal.x,goal.y,goal.z};for(int axis=0;axis<3;++axis)if(!reach_[i][axis].settled(values[axis]))return false;}
 return true;
}
void Motion::clearArmGoal(int side) {
 if(side<0||side>1)throw std::runtime_error("invalid arm side");
 armGoals_[side].reset();
 expressions_.clear(side==0?pets::character::Role::WristLeft:pets::character::Role::WristRight);
}
Motion::Motion(LimbArray<Leg> l,std::optional<std::array<Arm,2>> arms,bool wings,double wingReferenceReach,pets::character::Gait gait):legs_(l),arms_(arms),
 expressions_(pets::character::roleBit(pets::character::Role::Head)|pets::character::roleBit(pets::character::Role::EarLeft)|pets::character::roleBit(pets::character::Role::EarRight)|pets::character::roleBit(pets::character::Role::Tail)|(arms?(pets::character::roleBit(pets::character::Role::WristLeft)|pets::character::roleBit(pets::character::Role::WristRight)):0)),planner_(definition(l,wings,wingReferenceReach,gait)){
 if(quadruped()&&arms_)throw std::runtime_error("quadruped arms unsupported");
 turnStarts_.setCount(l.size());turningLegs_.setCount(l.size());turningLegs_.fill(false);
 contacts_.setCount(l.size());footYaw_.setCount(l.size());swingStartYaw_.setCount(l.size());swingEndYaw_.setCount(l.size());swingContactOffset_.setCount(l.size());swingFacing_.setCount(l.size());supportRestLength_.setCount(l.size());
 if(arms_)for(int i=0;i<2;++i){auto neutral=add((*arms_)[i].hand,(*arms_)[i].gripOffset);double values[]={neutral.x,neutral.y,neutral.z};for(int axis=0;axis<3;++axis){reach_[i][axis].setTuning({2.5,1.,0});reach_[i][axis].reset(values[axis]);}}
 // A short lead lets the support paw travel beneath and behind its hip
 // instead of recovering early to a permanently forward-biased target.
 planner_.setLandingLeadFraction(.1);planner_.setCenteredSteps(true);
 if(quadruped()){planner_.setPairedGait(true);planner_.setStepDuration(.18);}
 // Keep the shared anatomy-scaled comfortable stance radius. A spatial rig
 // must not require tighter home contacts than the same procedural planner.
 // Recentered soles put ankles farther behind the hip. This reserve retains
 // the authored knee flex; softer support yields under real body weight.
 const pets::character::SupportTuning support{120,24,6,.06,wings?.12:.025};
 planner_.setSupportTuning(support);planner_.reset(0,{-120,120});
 yaw_.reset();follower_.setTuning({2,.8,0});follower_.reset();
 for(std::size_t i=0;i<legs_.size();++i){contacts_[i]=l[i].contact;contacts_[i].z=l[i].hip.z;}
 const auto neutral=pose();
 if(arms_)for(int i=0;i<2;++i){auto grip=add((*arms_)[i].hand,(*arms_)[i].gripOffset);grip.y+=neutral.legs[0].hip.y-legs_[0].hip.y;reach_[i][0].reset(grip.x);reach_[i][1].reset(grip.y);reach_[i][2].reset(grip.z);}
 double reach=1e9;for(const auto& leg:l)reach=std::min(reach,length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee)));
 const double gravity=support.gravityPerReach*100*reach;
 setSpeedMultiplier(1);
 for(std::size_t i=0;i<legs_.size();++i){
  const auto hip=neutral.legs[i].hip,ankle=neutral.legs[i].ankle;
  supportRestLength_[i]=pets::motion::neutralSupportRestLength(
   {hip.x*100,-hip.y*100,hip.z*100},{ankle.x*100,-ankle.y*100,ankle.z*100},
   gravity,support.stiffness,l.size());
 }
}
void Motion::setSpeedMultiplier(double multiplier){
 planner_.setSpeedMultiplier(multiplier);
 // Reach / anatomy-derived pace keeps step timing independent of leg length.
 double reach=1e9;for(const auto& leg:legs_)reach=std::min(reach,100*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee))));
 const double pace=planner_.definition().locomotion->speed*multiplier;
 const double stanceBudget=.55*reach/pace-(quadruped()?.06:0.);
 planner_.setStepDuration(quadruped()?std::clamp(stanceBudget+.06,.10,.30):std::clamp(stanceBudget,.04,.34));
 if(quadruped())planner_.setPairedGait(true);
 pendingPlanner_.reset();pendingDelta_=0;
 // Travel speed does not change the timing of turns or body followers.

}
void Motion::setFootPlantDuration(double seconds){
 planner_.setFootPlantDuration(seconds);
 pendingPlanner_.reset();pendingDelta_=0;
 footPlantDuration_=seconds;
}
void Motion::setFacingBias(double radians){
 constexpr double pi=3.141592653589793;
 if(!std::isfinite(radians)||std::abs(radians)>pi/3)
  throw std::runtime_error("invalid 3D facing bias");
 facingBias_=radians;
 retargetYaw();
 pendingPlanner_.reset();pendingDelta_=0;
}
void Motion::setCameraFacingTurns(bool enabled){
 if(cameraFacingTurns_==enabled)return;
 cameraFacingTurns_=enabled;
 yaw_.setTuning(enabled?pets::character::SecondaryTuning{7,1,0}:pets::character::SecondaryTuning{});
 retargetYaw();
 pendingPlanner_.reset();pendingDelta_=0;
}
void Motion::retargetYaw(){
 constexpr double pi=3.141592653589793;
 const double heading=cameraFacingTurns_?(facingLeft_?-pi+std::abs(facingBias_):-std::abs(facingBias_))
                                       :(facingLeft_?pi:0)+facingBias_;
 if(std::abs(heading-yawTarget_)<1e-12)return;
 // One bounded velocity kick begins a changed heading promptly; repeated
 // requests for the same target never accumulate angular energy.
 if(cameraFacingTurns_)yaw_.impulse(std::clamp((heading-yaw_.value())*4,-3.,3.));
 yawTarget_=heading;
}
void Motion::reset(double rootX,pets::character::GroundSpan span,double floor){
 supportFloor_=floor;supportBounds_.reset();
 // Validate before altering adapter state, matching the shared planner.
 planner_.reset(rootX,span,floor);
 pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().rootX;facingLeft_=false;
 yawTarget_=cameraFacingTurns_?-std::abs(facingBias_):facingBias_;
 yaw_.reset(yawTarget_);follower_.reset();expressions_.reset();armGoals_={};
 footYaw_.fill(yawTarget_);swingStartYaw_=footYaw_;swingEndYaw_=footYaw_;swingContactOffset_.fill({});
 swingFacing_.fill(planner_.state().facing);
 quietAdjustment_=false;turnActive_=turnRecovering_=turnRejected_=false;turningLegs_.fill(false);turnBeat_=turnBeats_=0;turnBodyShift_={};
 pendingTurn_.reset();planner_.holdBodyHeight({});
 turnTime_=turnStartYaw_=turnEndYaw_=0;
 for(std::size_t i=0;i<legs_.size();++i){
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  contacts_[i]=rotateYaw(neutral,yawTarget_);
  contacts_[i].z+=requestedTarget_/100;
 }
 standingBodyDown_=legs_[0].hip.y-pose().legs[0].hip.y;
 if(arms_)for(int i=0;i<2;++i){auto v=add((*arms_)[i].hand,(*arms_)[i].gripOffset);v.y+=pose().legs[0].hip.y-legs_[0].hip.y;reach_[i][0].reset(v.x);reach_[i][1].reset(v.y);reach_[i][2].reset(v.z);}
}
void Motion::setSpan(pets::character::GroundSpan span){reset(planner_.state().rootX,span);}
void Motion::clearPickupMotion(){
 const auto persistentOffset=turnBodyShift_;
 pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().rootX;
 swingFacing_.fill(planner_.state().facing);
 quietAdjustment_=false;turnActive_=turnRecovering_=turnRejected_=false;turningLegs_.fill(false);turnBeat_=turnBeats_=0;turnBodyShift_={};
 pendingTurn_.reset();planner_.holdBodyHeight({});
 turnTime_=turnStartYaw_=turnEndYaw_=0;
 turnBodyShift_=persistentOffset;
 // Detached paws share the presented heading; no ground replant survives.
 yawTarget_=yaw_.value();yaw_.reset(yawTarget_);follower_.reset();
 footYaw_.fill(yawTarget_);swingStartYaw_=footYaw_;swingEndYaw_=footYaw_;swingContactOffset_.fill({});
 armGoals_={};expressions_.clear();
}
void Motion::pickup(double rootX,double elevation){
 planner_.pickup(rootX,elevation);clearPickupMotion();
}
void Motion::hold(double rootX,double elevation){
 planner_.hold(rootX,elevation);pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().rootX;
}
void Motion::release(pets::character::Vec2 velocity){
 planner_.release(velocity);pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().targetX;
 if(planner_.state().phase==pets::character::DropPhase::Grounded)installContacts();
}
void Motion::installContacts(){
 const auto state=planner_.state();
 swingFacing_.fill(state.facing);
 for(std::size_t i=0;i<legs_.size();++i){
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  contacts_[i]=add(rotateYaw(neutral,footYaw_[i]),turnBodyShift_);
  contacts_[i].y=-(state.feet[i].position.y-supportFloor_)/100;
  // The planar quadruped home already follows facing. Replace that proxy
  // offset with its continuous spatial rotation instead of reflecting twice.
  contacts_[i].z+=state.feet[i].position.x/100-(quadruped()?state.facing:1)*legs_[i].hip.z;
  boundTouchdown(contacts_[i]);
 }
}
void Motion::reconcile(double rootX,pets::character::GroundSpan span,double elevation,std::optional<double> floor){
 if(floor)supportFloor_=*floor;
 const auto before=planner_.state();
 planner_.reconcile(rootX,span,supportFloor_,elevation);pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().targetX;
 const auto after=planner_.state();
 if(before.phase!=pets::character::DropPhase::Held&&before.phase!=pets::character::DropPhase::Airborne&&
    (after.phase==pets::character::DropPhase::Held||after.phase==pets::character::DropPhase::Airborne))
  clearPickupMotion();
 if((before.phase==pets::character::DropPhase::Held||before.phase==pets::character::DropPhase::Airborne)&&
    after.phase!=pets::character::DropPhase::Held&&after.phase!=pets::character::DropPhase::Airborne)
  installContacts();
 else if(after.phase!=pets::character::DropPhase::Held&&after.phase!=pets::character::DropPhase::Airborne&&
         std::abs(after.rootX-before.rootX)>1e-12)
  {
   // Shared reconcile already translates planted support. Carry the actual
   // yawed soles by the same amount; rebuilding here would clamp them against
   // the old edge before the host supplies its translated bounds.
   const double delta=(after.rootX-before.rootX)/100;
   for(auto& contact:contacts_)contact.z+=delta;
   if(turnActive_){
    for(auto& start:turnStarts_)start.z+=delta;
    turnRootStart_.z+=delta;turnRootEnd_.z+=delta;turnPivot_.z+=delta;turnHostStart_+=100*delta;
   if(directionalTurn_){
    for(auto& goal:directionGoals_)goal.z+=delta;
    for(auto& goal:directionFinal_)goal.z+=delta;
    directionRootFirst_.z+=delta;directionRootSecond_.z+=delta;
   }
    turnDesiredRoot_+=100*delta;
   }
  }
}
void Motion::setSupportFrame(pets::character::GroundSpan span,double floor) {
 planner_.setSupportFrame(span,floor);
 if(pendingPlanner_)pendingPlanner_->setSupportFrame(span,floor);
 supportFloor_=floor;
}
void Motion::boundTouchdown(Vec3& contact) const {
 if(supportBounds_)contact.z=std::clamp(contact.z,supportBounds_->minimum/100,supportBounds_->maximum/100);
}
void Motion::setSupportBounds(std::optional<pets::character::GroundSpan> bounds) {
 // Reserve the yaw envelope missing from the shared proxy's travel-axis feet.
 if(bounds&&(!std::isfinite(bounds->minimum)||!std::isfinite(bounds->maximum)||bounds->minimum>bounds->maximum))throw std::runtime_error("invalid spatial support bounds");
 supportBounds_=bounds;
 if(bounds) {
  double margin=0;for(const auto& leg:legs_)margin=std::max(margin,100*(std::hypot(leg.contact.x,leg.hip.z)+std::abs(leg.hip.z)));
  const double center=(bounds->minimum+bounds->maximum)/2;
  bounds=pets::character::GroundSpan{std::min(center,bounds->minimum+margin),std::max(center,bounds->maximum-margin)};
 }
 planner_.setSupportBounds(bounds);
 if(pendingPlanner_)pendingPlanner_->setSupportBounds(bounds);
}
bool Motion::flightCapable()const{return planner_.flightCapable();}
bool Motion::hop(double height){
 if(!planner_.hop(height))return false;
 clearPickupMotion();return true;
}
bool Motion::flyTo(double x,double elevation){
 if(!planner_.flyTo(x,elevation))return false;
 clearPickupMotion();face(x);return true;
}
void Motion::land(){planner_.land();pendingPlanner_.reset();pendingDelta_=0;}
void Motion::cancelFlight(){planner_.cancelFlight();pendingPlanner_.reset();pendingDelta_=0;}

void Motion::target(double x){
 if(planner_.state().phase!=pets::character::DropPhase::Grounded)return;
 const bool comfortable=settled()||quietAdjustment_;
 turnRejected_=false;turnFaceOnly_=false;
 planner_.setTargetX(x);
 pendingPlanner_.reset();pendingDelta_=0;requestedTarget_=planner_.state().targetX;
 double delta=requestedTarget_-planner_.state().rootX;
 quietAdjustment_=comfortable&&std::abs(delta)<=headingHomeRadius;
 if(std::abs(delta)>headingHomeRadius){facingLeft_=delta<0;retargetYaw();}
}
bool Motion::face(double worldX){
 if(!std::isfinite(worldX))return false;
 const double delta=worldX-planner_.state().rootX;
 if(std::abs(delta)<=headingHomeRadius)return false;
 const double previousTarget=yawTarget_;
 turnRejected_=false;quietAdjustment_=false;
 facingLeft_=delta<0;retargetYaw();
 return std::abs(yawTarget_-previousTarget)>=1e-12;
}
bool Motion::feetTurning()const{return std::abs(yaw_.value()-yawTarget_)>1e-8;}
bool Motion::settled()const{
 return expressions_.settled() && reachSettled() && !turnActive_ && !feetTurning() &&
  std::abs(planner_.state().rootX-requestedTarget_)<.02 && planner_.settled() && yaw_.settled(yawTarget_) && follower_.settled(0);
}
pets::character::GroundedState Motion::state()const{auto s=planner_.state();s.targetX=requestedTarget_;s.active=!settled();return s;}
pets::character::MotionProposal Motion::propose(double dt){
 pendingTurn_.reset();
 if(!std::isfinite(dt)||dt<0)throw std::runtime_error("invalid 3D motion delta");
 if(settled()){
  pendingPlanner_.reset();pendingDelta_=0;
  return {0,requestedTarget_,0};
 }
 pendingDelta_=std::min(dt,1./30);
 pendingPlanner_=planner_;
 auto& pending=*pendingPlanner_;
 const auto supported=pose();
 if(planner_.state().phase!=pets::character::DropPhase::Held&&planner_.state().phase!=pets::character::DropPhase::Airborne)
  pending.constrainBodyRise(100*supported.minimumBodyDown);
 if(quadruped())pending.setSupportContacts(supported.grounded.array<4>());else pending.setSupportContacts(supported.grounded.array<2>());
 // Finish a lifted travel step, then replant briskly before a large turn
 // can send the body backwards. Keep the original navigation destination.
 const bool turning=feetTurning()||turnActive_;
 pending.setSwingAdmission(!quietAdjustment_&&!turnRejected_&&(!turning || (!turnActive_&&std::abs(planner_.state().velocity)>8)));
 pending.setTravelScale(turning?0.:1.);
 LimbArray<pets::motion::SupportLeg> supportGeometry;supportGeometry.setCount(legs_.size());
 for(std::size_t i=0;i<legs_.size();++i){
  const auto hip=rotateYaw(supported.legs[i].hip,supported.yaw);
  const auto ankle=rotateYaw(supported.legs[i].ankle,supported.yaw);
  supportGeometry[i]={{hip.x*100,-hip.y*100,hip.z*100},
                      {ankle.x*100,-ankle.y*100,ankle.z*100},
                      length(sub(legs_[i].knee,legs_[i].hip))*100,
                      length(sub(legs_[i].ankle,legs_[i].knee))*100,
                      supported.grounded[i]?1.:0.,supportRestLength_[i]};
 }
 if(quadruped())pending.setSupportGeometry(supportGeometry.array<4>());else pending.setSupportGeometry(supportGeometry.array<2>());
 pending.setTargetX(requestedTarget_);
 pending.face(pending.state().rootX+(facingLeft_?-100.:100.));
 auto proposal=pending.propose(pendingDelta_);
 if(turning&&planner_.state().phase==pets::character::DropPhase::Grounded&&!planner_.state().anyFootSwinging()&&std::abs(planner_.state().velocity)<8){
  pendingTurn_=std::make_shared<Motion>(*this);
  pendingTurn_->pendingTurn_.reset();
  pendingTurn_->advanceTurn(pendingDelta_);
  pending.holdBodyHeight(100*pendingTurn_->turnBodyDown_);
  proposal.displacement=pendingTurn_->turnDesiredRoot_-planner_.state().rootX;
 }else pending.holdBodyHeight({});
 return proposal;
}
void Motion::step(double dt){
 const auto proposal=propose(dt);
 realize(planner_.state().rootX+proposal.displacement,false,
         std::max(0.,planner_.state().elevation-proposal.displacementY));
}
void Motion::copyTurn(const Motion& other){
 yaw_=other.yaw_;contacts_=other.contacts_;footYaw_=other.footYaw_;swingStartYaw_=other.swingStartYaw_;
 turnBodyShift_=other.turnBodyShift_;turnRootStart_=other.turnRootStart_;turnHostStart_=other.turnHostStart_;
 turnRootEnd_=other.turnRootEnd_;turnPivot_=other.turnPivot_;
 directionalTurn_=other.directionalTurn_;directionGoals_=other.directionGoals_;directionFinal_=other.directionFinal_;
 directionRootFirst_=other.directionRootFirst_;directionRootSecond_=other.directionRootSecond_;
 directionLead_=other.directionLead_;directionKeep_=other.directionKeep_;
 directionFirstYaw_=other.directionFirstYaw_;directionSecondYaw_=other.directionSecondYaw_;directionForeYaw_=other.directionForeYaw_;
 turnBodyDown_=other.turnBodyDown_;turnDesiredRoot_=other.turnDesiredRoot_;
 turnBodyStartDown_=other.turnBodyStartDown_;turnBodyGoalDown_=other.turnBodyGoalDown_;
 turnPivotLeg_=other.turnPivotLeg_;turnFaceOnly_=other.turnFaceOnly_;
 turnRejected_=other.turnRejected_;turnRequestedYaw_=other.turnRequestedYaw_;turnRecovering_=other.turnRecovering_;
 turnStarts_=other.turnStarts_;turningLegs_=other.turningLegs_;
 turnBeat_=other.turnBeat_;turnBeats_=other.turnBeats_;
 turnOriginYaw_=other.turnOriginYaw_;turnPlanYaw_=other.turnPlanYaw_;
 turnDuration_=other.turnDuration_;turnActive_=other.turnActive_;
 turnTime_=other.turnTime_;turnStartYaw_=other.turnStartYaw_;turnEndYaw_=other.turnEndYaw_;
}
bool Motion::turnGeometryFeasible() const{
 Motion probe=*this;probe.pendingTurn_.reset();probe.pendingPlanner_.reset();
 const auto span=planner_.groundSpan();
 for(int sample=0;sample<1440&&probe.turnActive_;++sample){
  probe.advanceTurn(1./240);
  const auto p=probe.pose();
  const Vec3 root{probe.turnBodyShift_.x,0,probe.planner_.state().rootX/100+probe.turnBodyShift_.z};
  if(probe.turnDesiredRoot_<span.minimum-1e-7||probe.turnDesiredRoot_>span.maximum+1e-7)return false;
  for(std::size_t i=0;i<legs_.size();++i){
   const auto& leg=legs_[i];const auto& solved=p.legs[i];
   if(solved.hip.y<.9*(leg.hip.y-standingBodyDown_)-1e-7)return false;
   const double reach=length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee));
   if(length(sub(solved.hip,solved.ankle))>reach+1e-7)return false;
   if(supportBounds_){
    if(!p.swing[i]&&(p.contacts[i].z*100<supportBounds_->minimum-1e-7||p.contacts[i].z*100>supportBounds_->maximum+1e-7))return false;
    auto neutral=leg.contact;neutral.z=leg.hip.z;
    const auto recovery=add(root,rotateYaw(neutral,p.yaw));
    if(recovery.z*100<supportBounds_->minimum-1e-7||recovery.z*100>supportBounds_->maximum+1e-7)return false;
   }
  }
 }
 return !probe.turnActive_;
}
void Motion::advanceDirectionalTurn(double dt){

 if(!turnActive_){
  const auto p=pose();turnOriginYaw_=yaw_.value();turnPlanYaw_=yawTarget_;
  turnRequestedYaw_=yawTarget_;turnRejected_=false;turnRecovering_=false;
  turnBeat_=0;turnBeats_=3;turnTime_=0;turnActive_=true;
  turnBodyStartDown_=legs_[0].hip.y-p.legs[0].hip.y;
  turnFaceOnly_=std::abs(requestedTarget_-planner_.state().rootX)<.75;
  turnStarts_=contacts_;turnRootStart_={turnBodyShift_.x,0,planner_.state().rootX/100+turnBodyShift_.z};
  turnHostStart_=planner_.state().rootX;
  directionGoals_.setCount(4);directionFinal_.setCount(4);
  const double delta=turnPlanYaw_-turnOriginYaw_,side=std::sin(delta)<0?-1.:1.;
  const int preferred=legs_[0].hip.x*side>0?0:1;
  const auto forward=rotateYaw({0,0,1},turnPlanYaw_);
  bool feasible=false;
  for(int family:{0,1,2,3}){
   for(double ratio:{.95,.92,.90}){
    for(int choice:{0,1}){
     directionLead_=(choice?1-preferred:preferred)+(family==3?2:0);directionKeep_=directionLead_^2;
     const int mate=directionLead_^1;
     const int firstPair=directionLead_&2,lastPair=firstPair^2;
     directionFirstYaw_=turnOriginYaw_+delta*(family?24.:16.)/110;
     directionSecondYaw_=turnOriginYaw_+delta*(family?74.:64.)/110;
     directionForeYaw_=turnOriginYaw_+delta*(family?49.:40.)/110;
     turnBodyGoalDown_=1e9;
     for(const auto& leg:legs_)turnBodyGoalDown_=std::min(turnBodyGoalDown_,leg.hip.y-ratio*(leg.hip.y-standingBodyDown_));
     const auto& lead=legs_[directionLead_];
     const double reach=length(sub(lead.knee,lead.hip))+length(sub(lead.ankle,lead.knee));
     auto hip=rotateYaw(p.legs[directionLead_].hip,p.yaw);hip.z+=planner_.state().rootX/100;
     // A slightly longer wide plant accommodates the small residual stance
     // offsets left by a quantized host after fast walking arrivals.
     const double leadFraction=family==3?.24:(family==2?.58:(family?.57:.24));
     const double advance=std::max(leadFraction*reach,dot(sub(hip,contacts_[directionLead_]),forward)+.08*reach);
     // The world foot destination is authored first, into the requested path.
     // Root and following contacts are then solved from this new plant.
     directionGoals_[directionLead_]=add(contacts_[directionLead_],mul(forward,advance));
     directionGoals_[mate]=add(directionGoals_[directionLead_],mul(rotateYaw(sub(legs_[mate].hip,lead.hip),directionForeYaw_),family==3?1.:(family?.85:1.)));
     const auto fc=mul(add(legs_[firstPair].hip,legs_[firstPair+1].hip),.5);
     const auto fo=mul(add(sub(legs_[firstPair].ankle,legs_[firstPair].contact),sub(legs_[firstPair+1].ankle,legs_[firstPair+1].contact)),.5);
     const auto fa=mul(add(directionGoals_[firstPair],directionGoals_[firstPair+1]),.5);
     directionRootFirst_=sub(add(fa,rotateYaw(fo,directionForeYaw_)),rotateYaw(fc,directionFirstYaw_));directionRootFirst_.y=0;
     directionRootSecond_=sub(add(fa,rotateYaw(fo,directionForeYaw_)),rotateYaw(fc,directionSecondYaw_));directionRootSecond_.y=0;
     const auto hc=mul(add(legs_[lastPair].hip,legs_[lastPair+1].hip),.5);
     const auto ho=mul(add(sub(legs_[lastPair].ankle,legs_[lastPair].contact),sub(legs_[lastPair+1].ankle,legs_[lastPair+1].contact)),.5);
     auto ha=sub(add(directionRootSecond_,rotateYaw(hc,directionSecondYaw_)),rotateYaw(ho,turnPlanYaw_));ha.y=0;
     for(int i=0;i<4;++i)if((i&2)!=firstPair){directionGoals_[i]=add(ha,rotateYaw(sub(legs_[i].hip,hc),turnPlanYaw_));directionGoals_[i].y=0;}
     turnRootEnd_=sub(ha,rotateYaw(hc,turnPlanYaw_));turnRootEnd_.y=0;
     for(int i=0;i<4;++i){auto home=legs_[i].contact;home.z=legs_[i].hip.z;directionFinal_[i]=add(turnRootEnd_,rotateYaw(home,turnPlanYaw_));swingStartYaw_[i]=footYaw_[i];}
     if(turnGeometryFeasible()){feasible=true;break;}
    }
    if(feasible)break;
   }
   if(feasible)break;
  }
  if(!feasible){
   turnRejected_=true;yaw_.reset(turnOriginYaw_);turnBodyDown_=turnBodyStartDown_;
   turnBeat_=turnBeats_=0;turnDesiredRoot_=planner_.state().rootX;turnFaceOnly_=true;
   turnActive_=false;turningLegs_.fill(false);return;
  }
 }
 const double previousTime=turnTime_;turnTime_+=dt;
 double time=std::min(.84,turnTime_);
 bool replaced=false;
 if(std::abs(turnPlanYaw_-yawTarget_)>1e-8){
  for(double boundary:{.56,.84})if(previousTime<boundary&&time>=boundary){time=boundary;replaced=true;break;}
 }
 const double foreYaw=directionForeYaw_,firstYaw=directionFirstYaw_,secondYaw=directionSecondYaw_;
 const int firstPair=directionLead_&2,lastPair=firstPair^2;
 double heading=turnOriginYaw_;Vec3 root=turnRootStart_;
 if(time>.16&&time<=.28){const double e=ease((time-.16)/.12);heading=turnOriginYaw_+(firstYaw-turnOriginYaw_)*e;root=add(mul(turnRootStart_,1-e),mul(directionRootFirst_,e));}
 else if(time>.28&&time<=.56){const double e=ease((time-.28)/.28);heading=firstYaw+(secondYaw-firstYaw)*e;
  const auto fc=mul(add(legs_[firstPair].hip,legs_[firstPair+1].hip),.5),fo=mul(add(sub(legs_[firstPair].ankle,legs_[firstPair].contact),sub(legs_[firstPair+1].ankle,legs_[firstPair+1].contact)),.5),fa=mul(add(directionGoals_[firstPair],directionGoals_[firstPair+1]),.5);
  root=sub(add(fa,rotateYaw(fo,foreYaw)),rotateYaw(fc,heading));root.y=0;
 }else if(time>.56){const double e=ease((time-.56)/.28);heading=secondYaw+(turnPlanYaw_-secondYaw)*e;
  const auto hc=mul(add(legs_[lastPair].hip,legs_[lastPair+1].hip),.5),ho=mul(add(sub(legs_[lastPair].ankle,legs_[lastPair].contact),sub(legs_[lastPair+1].ankle,legs_[lastPair+1].contact)),.5),ha=mul(add(directionGoals_[lastPair],directionGoals_[lastPair+1]),.5);
  root=add(sub(add(ha,rotateYaw(ho,turnPlanYaw_)),rotateYaw(hc,heading)),rotateYaw(mul(ho,-e),heading));root.y=0;
 }
 // A small inward weight shift leaves room for the lifted paw silhouette
 // while the first directional plant still leads the held torso heading.
 if(directionLead_>=2&&time<.28){
  const auto span=planner_.groundSpan();const auto& lead=legs_[directionLead_];
  const double R=length(sub(lead.knee,lead.hip))+length(sub(lead.ankle,lead.knee));
  const double inward=turnHostStart_<(span.minimum+span.maximum)/2?1.:-1.;
  const double weight=ease(std::min(1.,time/.04))*(1-ease(std::clamp((time-.16)/.12,0.,1.)));
  root.z+=inward*.04*R*weight;
 }
 // Move the host by the body path delta; its persistent local offset is
 // already presented and must not become an outward recenter at an edge.
 yaw_.reset(heading);turnDesiredRoot_=turnHostStart_+(root.z-turnRootStart_.z)*100;turnBodyShift_={root.x,0,root.z-planner_.state().rootX/100};
 turnBodyDown_=turnBodyStartDown_+(turnBodyGoalDown_-turnBodyStartDown_)*ease(std::min(1.,time/.16));
 for(int i=0;i<4;++i){
  double begin,end,oldYaw,newYaw;Vec3 start,goal;
  if((i&2)==firstPair&&time>(i==directionLead_?.56:.60)){begin=i==directionLead_?.56:.60;end=i==directionLead_?.80:.84;start=directionGoals_[i];goal=directionFinal_[i];oldYaw=foreYaw;newYaw=turnPlanYaw_;}
  else if((i&2)==firstPair){begin=i==directionLead_?0:.04;end=i==directionLead_?.16:.28;start=turnStarts_[i];goal=directionGoals_[i];oldYaw=swingStartYaw_[i];newYaw=foreYaw;}
  else{begin=i==directionKeep_?.28:.16;end=i==directionKeep_?.56:.52;start=turnStarts_[i];goal=directionGoals_[i];oldYaw=swingStartYaw_[i];newYaw=turnPlanYaw_;}
  const bool wasSwinging=turningLegs_[i];const double phase=std::clamp((time-begin)/(end-begin),0.,1.);
  turningLegs_[i]=phase>0&&phase<1;
  if(phase==0||phase>=1&&!wasSwinging)continue;
  const double e=ease(phase);footYaw_[i]=oldYaw+(newYaw-oldYaw)*ease(std::min(1.,phase/.7));
  auto contact=add(mul(start,1-e),mul(goal,e));
  contact.y+=std::min(.05,.08*(length(sub(legs_[i].knee,legs_[i].hip))+length(sub(legs_[i].ankle,legs_[i].knee))))*lift(phase);
  contacts_[i]=contact;
 }
 // At an edge the hind pair can lead inward. Keep its root path inside
 // the intersection of the loaded limbs' reach disks and the host span.
 // Admission simulates these same bounded iterations before lifting a paw.
 auto bounds=planner_.groundSpan();
 const double originOffset=100*turnRootStart_.z-turnHostStart_;
 bounds.minimum+=originOffset;bounds.maximum+=originOffset;
 if(directionLead_>=2){
  for(int pass=0;pass<64;++pass){
   root.z=std::clamp(root.z,bounds.minimum/100,bounds.maximum/100);
   for(int i=0;i<4;++i)if(!turningLegs_[i]){
    const auto off=rotateYaw(sub(legs_[i].ankle,legs_[i].contact),footYaw_[i]);
    const auto hip=rotateYaw(legs_[i].hip,heading);
    const auto center=sub(add(contacts_[i],off),hip);
    const double vertical=legs_[i].hip.y-turnBodyDown_-off.y;
    const double R=length(sub(legs_[i].knee,legs_[i].hip))+length(sub(legs_[i].ankle,legs_[i].knee));
    const double radius=std::sqrt(std::max(0.,R*R-vertical*vertical));
    Vec3 d{sub(root,center)};d.y=0;const double dist=length(d);
    if(dist>radius&&dist>0){root.x=center.x+d.x*radius/dist;root.z=center.z+d.z*radius/dist;}
   }
  }
  root.z=std::clamp(root.z,bounds.minimum/100,bounds.maximum/100);
  turnDesiredRoot_=turnHostStart_+(root.z-turnRootStart_.z)*100;turnBodyShift_={root.x,0,root.z-planner_.state().rootX/100};
 }
 if(time>=.84||replaced){turnBeat_=turnBeats_;turnTime_=0;turnActive_=false;turningLegs_.fill(false);}
}
void Motion::advancePairedTurn(double dt){
 if(!turnActive_){
  const auto p=pose();turnOriginYaw_=yaw_.value();turnPlanYaw_=yawTarget_;
  turnRequestedYaw_=yawTarget_;turnRejected_=false;turnRecovering_=false;
  turnBeat_=0;turnBeats_=3;turnTime_=0;turnActive_=true;
  turnBodyStartDown_=legs_[0].hip.y-p.legs[0].hip.y;
  turnFaceOnly_=std::abs(requestedTarget_-planner_.state().rootX)<.75;
  bool feasible=false;
  for(int beats:{3,5}){
   turnBeats_=beats;
   for(double ratio:{.95,.94,.93,.92,.91,.90}){
    turnBodyGoalDown_=1e9;
    for(const auto& leg:legs_)turnBodyGoalDown_=std::min(turnBodyGoalDown_,leg.hip.y-ratio*(leg.hip.y-standingBodyDown_));
    if(turnGeometryFeasible()){feasible=true;break;}
   }
   if(feasible)break;
  }
  if(!feasible){
   turnRejected_=true;yaw_.reset(turnOriginYaw_);
   turnBodyDown_=turnBodyStartDown_;turnBeat_=turnBeats_=0;
   turnDesiredRoot_=planner_.state().rootX;turnFaceOnly_=true;
   turnActive_=false;turningLegs_.fill(false);return;
  }
 }
 const int loaded=turnBeat_%2?0:2,swung=turnBeat_%2?2:0;
 const double delta=turnPlanYaw_-turnOriginYaw_;
 const double angle=std::abs(delta),radians=3.141592653589793/180;
 const double first=std::min(angle*32./110,32*radians)/std::max(angle,1e-12);
 const double middle=1-std::min(angle*34./110,34*radians)/std::max(angle,1e-12);
 const double bodyThree[]={first,middle,1.},feetThree[]={(first+middle)*.5,1.,1.};
 const double bodyFive[]={26./180,68./180,117./180,153./180,1.},feetFive[]={43./180,.5,.75,1.,1.};
 const auto bodyFractions=turnBeats_==3?bodyThree:bodyFive;
 const auto footFractions=turnBeats_==3?feetThree:feetFive;
 const double landing=turnOriginYaw_+delta*footFractions[turnBeat_];
 if(turnTime_==0){
  turnRootStart_={turnBodyShift_.x,0,planner_.state().rootX/100+turnBodyShift_.z};
  turnStarts_=contacts_;turnStartYaw_=yaw_.value();turnEndYaw_=turnOriginYaw_+delta*bodyFractions[turnBeat_];
  turnDuration_=.28;turningLegs_.fill(false);
  for(int j=0;j<2;++j)swingStartYaw_[swung+j]=footYaw_[swung+j];
 }
 const auto hipCenter=mul(add(legs_[loaded].hip,legs_[loaded+1].hip),.5);
 const auto offsetCenter=mul(add(sub(legs_[loaded].ankle,legs_[loaded].contact),sub(legs_[loaded+1].ankle,legs_[loaded+1].contact)),.5);
 const auto anchorCenter=mul(add(contacts_[loaded],contacts_[loaded+1]),.5);
 const double loadedYaw=footYaw_[loaded];
 auto center=[&](double heading){auto c=sub(add(anchorCenter,rotateYaw(offsetCenter,loadedYaw)),rotateYaw(hipCenter,heading));c.y=0;return c;};
 const auto q0=rotateYaw(sub(turnRootStart_,center(turnStartYaw_)),-turnStartYaw_);
 auto q1=turnBeat_==turnBeats_-1?mul(offsetCenter,-1):Vec3{};q1.y=0;
 turnRootEnd_=add(center(turnEndYaw_),rotateYaw(q1,turnEndYaw_));
 const auto sh=mul(add(legs_[swung].hip,legs_[swung+1].hip),.5);
 const auto so=mul(add(sub(legs_[swung].ankle,legs_[swung].contact),sub(legs_[swung+1].ankle,legs_[swung+1].contact)),.5);
 auto landingCenter=sub(add(turnRootEnd_,rotateYaw(sh,turnEndYaw_)),rotateYaw(so,landing));landingCenter.y=0;
 turnTime_+=dt;const double t=std::clamp((turnTime_-.04)/.20,0.,1.),body=ease(t);
 const double heading=turnStartYaw_+(turnEndYaw_-turnStartYaw_)*body;
 auto root=add(center(heading),rotateYaw(add(mul(q0,1-body),mul(q1,body)),heading));root.y=0;
 turnDesiredRoot_=root.z*100;turnBodyShift_={root.x,0,root.z-planner_.state().rootX/100};
 turnBodyDown_=turnBeat_==0?turnBodyStartDown_+(turnBodyGoalDown_-turnBodyStartDown_)*body:turnBodyGoalDown_;
 yaw_.reset(heading);
 // Loaded yaw is immutable. The two swing paws begin .04s apart and aim before
 // the torso follows; each touchdown becomes an authoritative world pin.
 for(int j=0;j<2;++j){const int i=swung+j;
  const double phase=std::clamp((turnTime_-.04*j)/.24,0.,1.);
  const bool wasSwinging=turningLegs_[i];
  turningLegs_[i]=phase>0&&phase<1;
  if(phase>=1&&!wasSwinging)continue;
  const double foot=ease(phase);
  const double oldYaw=swingStartYaw_[i];
  footYaw_[i]=oldYaw+(landing-oldYaw)*ease(std::min(1.,phase/.65));
  const auto& leg=legs_[i];
  auto goal=add(landingCenter,rotateYaw(sub(leg.hip,sh),landing));goal.y=0;
  if(turnBeat_==turnBeats_-1){auto home=leg.contact;home.z=leg.hip.z;goal=add(turnRootEnd_,rotateYaw(home,turnPlanYaw_));}
  if(phase==0)continue;
  auto contact=add(mul(turnStarts_[i],1-foot),mul(goal,foot));
  contact.y+=std::min(.05,.08*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee))))*lift(phase);
  const auto offset=rotateYaw(sub(leg.ankle,leg.contact),footYaw_[i]);auto ankle=add(contact,offset);
  auto hip=add(root,rotateYaw(leg.hip,heading));hip.y-=turnBodyDown_;
  const double reach=.995*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee))),vertical=hip.y-ankle.y;
  const double radius=std::sqrt(std::max(0.,reach*reach-vertical*vertical));
  const double dx=ankle.x-hip.x,dz=ankle.z-hip.z,distance=std::hypot(dx,dz);
  if(distance>radius){ankle.x=hip.x+dx*radius/distance;ankle.z=hip.z+dz*radius/distance;}
  contacts_[i]=sub(ankle,offset);
 }
 if(turnTime_>=turnDuration_){++turnBeat_;turnTime_=0;if(turnBeat_>=turnBeats_||std::abs(turnPlanYaw_-yawTarget_)>1e-8){turnActive_=false;turningLegs_.fill(false);turnBeat_=turnBeats_;}}
}
void Motion::advanceTurn(double dt){
 if(turnRecovering_){
  turnTime_+=dt;const double t=std::min(1.,turnTime_/.16),smooth=ease(t);
  const Vec3 root{turnBodyShift_.x,0,planner_.state().rootX/100+turnBodyShift_.z};
  turnDesiredRoot_=planner_.state().rootX;
  for(std::size_t i=0;i<legs_.size();++i)if(turningLegs_[i]){
   auto neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
   auto goal=add(root,rotateYaw(neutral,yaw_.value()));boundTouchdown(goal);
   contacts_[i]=add(mul(turnStarts_[i],1-smooth),mul(goal,smooth));
  }
  if(t==1){
   turnRecovering_=turnActive_=false;turnRejected_=turnFaceOnly_;
   turnBeat_=turnBeats_=0;turningLegs_.fill(false);
  }
  return;
 }
 // A subpixel heading correction does not justify another finite stepping
 // gesture. Keep every world contact and its loaded orientation unchanged.
 constexpr double smallHeading=3.141592653589793/1800; // .1 degree
 if(!turnActive_&&std::abs(yawTarget_-yaw_.value())<=smallHeading){
  turnBodyDown_=legs_[0].hip.y-pose().legs[0].hip.y;
  yaw_.reset(yawTarget_);turnRequestedYaw_=yawTarget_;turnRejected_=false;
  turnDesiredRoot_=planner_.state().rootX;
  turnFaceOnly_=std::abs(requestedTarget_-planner_.state().rootX)<.75;
  turnBeat_=turnBeats_=0;turnTime_=0;turningLegs_.fill(false);
  return;
 }
 if(quadruped()){
  if(!turnActive_)directionalTurn_=std::abs(yawTarget_-yaw_.value())<=2.44;
  if(directionalTurn_)advanceDirectionalTurn(dt);else advancePairedTurn(dt);
  return;
 }
 if(!turnActive_){
  const auto presented=pose();
  turnBodyStartDown_=legs_[0].hip.y-presented.legs[0].hip.y;
  turnBodyGoalDown_=standingBodyDown_;
  for(const auto& leg:legs_){
   const double reach=.995*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee)));
   const double horizontal=std::hypot(leg.ankle.x-leg.contact.x,leg.ankle.z-leg.contact.z);
   turnBodyGoalDown_=std::max(turnBodyGoalDown_,leg.hip.y-leg.ankle.y-std::sqrt(std::max(0.,reach*reach-horizontal*horizontal)));
  }
  turnRejected_=false;turnRequestedYaw_=yawTarget_;
  turnOriginYaw_=yaw_.value();turnPlanYaw_=yawTarget_;turnBeat_=0;turnBeats_=2;
  // Authored dynamic point support: one sole owns the pivot while the other
  // contacts unload. This is a kinematic gesture, not a static COM model.
  turnRootStart_={turnBodyShift_.x,0,planner_.state().rootX/100+turnBodyShift_.z};
  // Keep repeated turns near the scene's depth origin, then prefer forward
  // arc progress. Evaluate the actual contacts, including prior root offsets.
  const double direction=facingLeft_?-1.:1.;
  bool feasible=false;
  const auto span=planner_.groundSpan();
  const double requestedYaw=turnPlanYaw_;
  {
   const double candidateYaw=requestedYaw;
   double best=1e100;
   for(std::size_t i=0;i<legs_.size();++i){
    if(contacts_[i].y>1e-10)continue;
    const auto& lead=legs_[i];
    auto home=lead.contact;home.z=lead.hip.z;
    const double total=.995*(length(sub(lead.knee,lead.hip))+length(sub(lead.ankle,lead.knee)));
    const double vertical=lead.hip.y-turnBodyStartDown_-lead.ankle.y;
    const double budget=std::sqrt(std::max(0.,total*total-vertical*vertical));
    const double side=std::sin(candidateYaw-turnOriginYaw_)<0?-1.:1.;
    Vec3 aim=rotateYaw({0,0,1},candidateYaw-turnOriginYaw_);
    aim.x+=side;aim=normalized(aim);
    auto ankle=add(turnRootStart_,rotateYaw(add(lead.hip,mul(aim,.60*budget)),turnOriginYaw_));
    ankle.y=lead.ankle.y;
    auto landing=sub(ankle,rotateYaw(sub(lead.ankle,lead.contact),candidateYaw));landing.y=0;
    if(supportBounds_&&(landing.z*100<supportBounds_->minimum||landing.z*100>supportBounds_->maximum))continue;
    // Preserve side ownership in the old body frame: no crossing the midline.
    const auto localAnkle=rotateYaw(sub(ankle,turnRootStart_),-turnOriginYaw_);
    if(localAnkle.x*lead.hip.x<=0)continue;
    auto end=sub(landing,rotateYaw(home,candidateYaw));end.y=0;
    const auto startLocal=rotateYaw(sub(turnRootStart_,landing),-turnOriginYaw_);
    const auto endLocal=rotateYaw(sub(end,landing),-candidateYaw);
    double reverse=0,depth=0,previous=turnRootStart_.z;
    bool valid=true;
    for(int sample=1;sample<=32;++sample){
     const double phase=sample/32.;
     const auto local=add(mul(startLocal,1-phase),mul(endLocal,phase));
     const auto root=add(landing,rotateYaw(local,turnOriginYaw_+(candidateYaw-turnOriginYaw_)*phase));
     if(root.z*100<span.minimum-1e-8||root.z*100>span.maximum+1e-8)valid=false;
     if(supportBounds_)for(const auto& leg:legs_){
      auto neutral=leg.contact;neutral.z=leg.hip.z;
      const auto sole=add(root,rotateYaw(neutral,turnOriginYaw_+(candidateYaw-turnOriginYaw_)*phase));
      if(sole.z*100<supportBounds_->minimum-1e-8||sole.z*100>supportBounds_->maximum+1e-8)valid=false;
     }
     reverse+=std::max(0.,-direction*(root.z-previous));previous=root.z;
     depth=std::max(depth,std::abs(root.x));
    }
    if(supportBounds_)for(const auto& leg:legs_){
     auto neutral=leg.contact;neutral.z=leg.hip.z;
     const auto goal=add(end,rotateYaw(neutral,candidateYaw));
     if(goal.z*100<supportBounds_->minimum-1e-8||goal.z*100>supportBounds_->maximum+1e-8)valid=false;
    }
    const double rolePenalty=(quadruped()&&i>=2?10.:0.)+(lead.hip.x*side<0?2.:0.);
    const double score=rolePenalty+depth+reverse;
    if(valid&&score<best){best=score;turnPivotLeg_=int(i);turnPivot_=landing;turnPlanYaw_=candidateYaw;feasible=true;}
   }
  }
  if(!feasible){
   // No finite upright arc fits the available support. Keep the presented
   // heading and cancel this unreachable maneuver instead of crossing bounds.
   turnRejected_=true;yaw_.reset(turnOriginYaw_);footYaw_.fill(turnOriginYaw_);
   turnBodyDown_=turnBodyStartDown_;turnBeat_=turnBeats_=0;
   turnDesiredRoot_=planner_.state().rootX;turnFaceOnly_=true;
   turnActive_=false;turningLegs_.fill(false);return;
  }
  /* turnPivot_ is the lead paw touchdown, not its old support point. */
  auto pivotHome=legs_[turnPivotLeg_].contact;pivotHome.z=legs_[turnPivotLeg_].hip.z;
  turnRootEnd_=sub(turnPivot_,rotateYaw(pivotHome,turnPlanYaw_));
  turnRootEnd_.y=0;
  turnFaceOnly_=std::abs(requestedTarget_-planner_.state().rootX)<.75;
  turnTime_=0;turnActive_=true;
  if(!turnGeometryFeasible()){
   turnRejected_=true;yaw_.reset(turnOriginYaw_);turnBodyDown_=turnBodyStartDown_;
   turnBeat_=turnBeats_=0;turnDesiredRoot_=planner_.state().rootX;
   turnFaceOnly_=true;turnActive_=false;turningLegs_.fill(false);return;
  }
 }
 if(turnTime_==0){
  turnStarts_=contacts_;turnStartYaw_=yaw_.value();turnEndYaw_=turnPlanYaw_;
  turnDuration_=turnBeat_==0?.20:std::max(.28,std::abs(turnPlanYaw_-turnOriginYaw_)*1.875/8.5)+.04;
  turningLegs_.fill(turnBeat_!=0);
  turningLegs_[turnPivotLeg_]=turnBeat_==0;
  for(std::size_t i=0;i<legs_.size();++i)if(turningLegs_[i])swingStartYaw_[i]=footYaw_[i];
 }
 turnTime_+=dt;
 const double t=std::min(1.,turnTime_/turnDuration_);
 const double phase=turnBeat_==0?t:std::clamp((turnTime_-.04)/(turnDuration_-.04),0.,1.);
 const double smooth=ease(phase);
 const double heading=turnBeat_==0?turnOriginYaw_:turnOriginYaw_+(turnPlanYaw_-turnOriginYaw_)*smooth;
 yaw_.reset(heading);
 for(std::size_t i=0;i<legs_.size();++i)if(turningLegs_[i])footYaw_[i]=swingStartYaw_[i]+(turnPlanYaw_-swingStartYaw_[i])*smooth;
 const auto localStart=rotateYaw(sub(turnRootStart_,turnPivot_),-turnOriginYaw_);
 const auto localEnd=rotateYaw(sub(turnRootEnd_,turnPivot_),-turnPlanYaw_);
 const auto local=add(mul(localStart,1-smooth),mul(localEnd,smooth));
 Vec3 root=turnBeat_==0?turnRootStart_:add(turnPivot_,rotateYaw(local,heading));
 turnBodyDown_=turnBeat_==0?turnBodyStartDown_:turnBodyStartDown_+(turnBodyGoalDown_-turnBodyStartDown_)*smooth;
 root.y=0;turnDesiredRoot_=root.z*100;
 turnBodyShift_={root.x,0,root.z-planner_.state().rootX/100};
 for(std::size_t i=0;i<legs_.size();++i){
  if(!turningLegs_[i])continue;
  const auto& leg=legs_[i];
  Vec3 neutral=leg.contact;neutral.z=leg.hip.z;
  auto goal=turnBeat_==0?turnPivot_:add(turnRootEnd_,rotateYaw(neutral,turnPlanYaw_));
  boundTouchdown(goal);
  auto contact=add(mul(turnStarts_[i],1-smooth),mul(goal,smooth));
  contact.y+=std::min(.06,.09*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee))))*lift(phase);
  const auto offset=rotateYaw(sub(leg.ankle,leg.contact),footYaw_[i]);
  auto ankle=add(contact,offset);
  auto hip=add(root,rotateYaw(leg.hip,heading));hip.y-=turnBodyDown_;
  const double reach=.995*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee)));
  const double vertical=hip.y-ankle.y;
  const double horizontal=std::sqrt(std::max(0.,reach*reach-vertical*vertical));
  const double dx=ankle.x-hip.x,dz=ankle.z-hip.z,distance=std::hypot(dx,dz);
  if(distance>horizontal){ankle.x=hip.x+dx*horizontal/distance;ankle.z=hip.z+dz*horizontal/distance;}
  contacts_[i]=sub(ankle,offset);
 }
 if(t==1){
  ++turnBeat_;turnTime_=0;
  if(turnBeat_>=turnBeats_||std::abs(turnPlanYaw_-yawTarget_)>1e-8){
   turnActive_=false;turningLegs_.fill(false);turnBeat_=turnBeats_;
  }
 }
}
void Motion::realize(double rootX,bool blocked,std::optional<double> elevation,double ceilingTolerance){
 if(!std::isfinite(rootX)||(elevation&&(!std::isfinite(*elevation)||*elevation<0)))
  throw std::runtime_error("invalid 3D realized root");
 if(!pendingPlanner_||pendingDelta_==0)return;
 const double dt=pendingDelta_;
 const auto before=planner_.state();
 planner_=std::move(*pendingPlanner_);
 pendingPlanner_.reset();pendingDelta_=0;
 const auto presentedTurn=std::move(pendingTurn_);
 pendingTurn_.reset();
 // Navigation brakes during a large stepping turn, retaining its target.
 planner_.realize(rootX,blocked,elevation,ceilingTolerance);
 auto after=planner_.state();
 if(blocked)requestedTarget_=after.rootX;
 else if(!presentedTurn)requestedTarget_=after.targetX;
 const auto detached=[](pets::character::DropPhase phase){
  return phase==pets::character::DropPhase::Held||phase==pets::character::DropPhase::Airborne;
 };
 if(detached(before.phase)&&!detached(after.phase))installContacts();
 if(!detached(after.phase)){
 for(std::size_t i=0;i<legs_.size();++i)if(before.feet[i].swing||after.feet[i].swing){
  if(after.feet[i].swing) {
   if(!before.feet[i].swing) {
    swingStartYaw_[i]=footYaw_[i];
    swingEndYaw_[i]=yaw_.value();
    swingFacing_[i]=after.facing;
   }
   footYaw_[i]=swingStartYaw_[i]+(swingEndYaw_[i]-swingStartYaw_[i])*ease(after.feet[i].phase);
  }
  if(before.feet[i].swing&&!after.feet[i].swing)footYaw_[i]=swingEndYaw_[i];
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  auto rotated=rotateYaw(neutral,footYaw_[i]);
  // A target reversal changes the planner heading immediately, but does not
  // rewrite an already lifted foot's trajectory. Keep that step's original
  // proxy-to-spatial basis through touchdown; the next lift takes the new one.
  Vec3 contact{turnBodyShift_.x+rotated.x,-(after.feet[i].position.y-supportFloor_)/100,after.feet[i].position.x/100+turnBodyShift_.z+rotated.z-(quadruped()?swingFacing_[i]:1)*legs_[i].hip.z};
  // A normal catch-up may interrupt a turn replant. Preserve its presented
  // starting contact and blend away the offset over the new lifted phase.
  if(!before.feet[i].swing&&after.feet[i].swing){
   const auto start=rotateYaw(neutral,swingStartYaw_[i]);
   const Vec3 startContact{turnBodyShift_.x+start.x,-(before.feet[i].position.y-supportFloor_)/100,
    before.feet[i].position.x/100+turnBodyShift_.z+start.z-(quadruped()?swingFacing_[i]:1)*legs_[i].hip.z};
   swingContactOffset_[i]=sub(contacts_[i],startContact);
  }
  contacts_[i]=add(contact,mul(swingContactOffset_[i],after.feet[i].swing?1-ease(after.feet[i].phase):0));
  boundTouchdown(contacts_[i]);
 }
 if(presentedTurn){
  if(blocked&&!turnRecovering_){
   // Keep the presented pivot, then land its unloaded feet without advancing
   // an arc that the host cannot realize.
   turnBodyShift_.z+=(before.rootX-after.rootX)/100;
   turnStarts_=contacts_;turnTime_=0;turnActive_=turnRecovering_=true;
   for(std::size_t i=0;i<legs_.size();++i)turningLegs_[i]=contacts_[i].y>1e-10;
   yaw_.reset(yaw_.value());yawTarget_=yaw_.value();turnRequestedYaw_=yawTarget_;
   turnBodyDown_=legs_[0].hip.y-pose().legs[0].hip.y;
   requestedTarget_=after.rootX;turnFaceOnly_=true;planner_.setTargetX(after.rootX);
  }else{
  copyTurn(*presentedTurn);
  if(!turnActive_&&turnBeats_==0&&!turnRejected_&&turnFaceOnly_)quietAdjustment_=true;
  if(turnRejected_&&std::abs(yawTarget_-turnRequestedYaw_)<1e-8){
   yawTarget_=yaw_.value();requestedTarget_=after.rootX;planner_.setTargetX(after.rootX);
  }
  // Contacts and the body arc were planned before host realization. Preserve
  // the exact presented pose when the host rounds or clips its root proposal.
  turnBodyShift_.z+=(before.rootX-after.rootX)/100;
  if(turnFaceOnly_)requestedTarget_=after.rootX;
  if(!turnActive_&&turnBeat_>=turnBeats_){
   LimbArray<pets::character::Vec2> proxy;proxy.setCount(legs_.size());
   for(std::size_t i=0;i<legs_.size();++i){
    auto neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
    const auto rotated=rotateYaw(neutral,footYaw_[i]);
    proxy[i]={(contacts_[i].z-turnBodyShift_.z-rotated.z)*100+
              (quadruped()?after.facing:1)*legs_[i].hip.z*100,
              supportFloor_-contacts_[i].y*100};
   }
   planner_.plantContacts(proxy);swingFacing_.fill(after.facing);
  }
  }
 }
 } else {
  turnActive_=false;turningLegs_.fill(false);
  yaw_.step(dt,yawTarget_);footYaw_.fill(yaw_.value());
 }
 if(!detached(after.phase)){
  const auto supported=pose();
  planner_.constrainBodyRise(100*supported.minimumBodyDown);
 }
 // Flight has its own shared body/limb response. Never feed uncapped throw
 // speed into expressive head/tail intent; ordinary reference walking is 70
 // cm/s, and this secondary allowance remains above that normal range.
 const double followerIntent=detached(after.phase)?0:after.velocity/speedMultiplier()*std::cos(yaw_.value());
 follower_.step(dt,std::clamp(followerIntent,-100.,100.));
 expressions_.step(dt);
 if(arms_){const double bodyOffset=pose().legs[0].hip.y-legs_[0].hip.y;
  for(int i=0;i<2;++i){
   auto goal=armFollowerGoal(i,bodyOffset);double values[]={goal.x,goal.y,goal.z};
   const bool falling=after.phase==pets::character::DropPhase::Airborne&&!after.poweredFlight&&!armGoals_[i];
   for(int axis=0;axis<3;++axis){
    // Short desktop falls need a readable balance response immediately; normal
    // reaches and the return through support retain the gentler default tuning.
    reach_[i][axis].setTuning({falling?4.:2.5,1.,0});
    reach_[i][axis].step(dt,values[axis]);
   }
  }
 }
}
void Motion::impulse(){pendingPlanner_.reset();pendingDelta_=0;planner_.impulse({1,-.5},2);follower_.impulse(20);}
Pose Motion::pose()const{
 auto state=planner_.state();auto expressive=planner_.pose(pets::character::Action::Rest);Pose p;p.pivoting=turnActive_;p.rootX=state.rootX;p.yaw=yaw_.value();p.lean=expressive.bones[0].rotation*std::cos(p.yaw);p.follower=follower_.value();p.contacts=contacts_;p.footYaw=footYaw_;p.wings=state.wings;
 double down=(expressive.bones[0].y-planner_.definition().bones[0].rest.y)/100;
 const bool detached=state.phase==pets::character::DropPhase::Held||state.phase==pets::character::DropPhase::Airborne;
 const auto sharedWorld=detached?pets::character::worldTransforms(planner_.definition(),expressive):std::vector<pets::character::WorldTransform>{};
 LimbArray<Vec3> targets;targets.setCount(legs_.size());
 p.legs.setCount(legs_.size());p.contacts.setCount(legs_.size());p.footYaw.setCount(legs_.size());p.footPitch.setCount(legs_.size());p.swing.setCount(legs_.size());p.grounded.setCount(legs_.size());
 auto inverseYaw=[&](Vec3 v){double c=std::cos(p.yaw),s=std::sin(p.yaw);return Vec3{c*v.x-s*v.z,v.y,s*v.x+c*v.z};};
 p.bodyShift=inverseYaw(turnBodyShift_);
 for(std::size_t i=0;i<legs_.size();++i){auto l=legs_[i];
  if(detached){
   // Shared detached IK supplies body-relative goals in its forward/down
   // plane. Preserve actual lateral anatomy and solve measured 3D lengths.
   const auto ankle=planner_.definition().locomotion->legs[i].ankle;
   targets[i]={l.ankle.x,(planner_.definition().locomotion->groundY-sharedWorld[ankle].ty)/100,
               (sharedWorld[ankle].tx-planner_.definition().origin.x)/100};
   targets[i]=add(targets[i],p.bodyShift);
   p.swing[i]=true;p.footYaw[i]=p.yaw;p.footPitch[i]=0;
   continue;
  }
  p.swing[i]=state.feet[i].swing||(turnActive_&&turningLegs_[i]);
  const double phase=state.feet[i].phase;
  p.footPitch[i]=state.feet[i].swing?-.32*std::sin(2*3.141592653589793*phase)*lift(phase):0;
  // Supported feet keep their own world orientation; a lifted foot rotates
  // with its replant rather than inheriting a torso-only spin.
  Vec3 contact=contacts_[i];contact.z-=state.rootX/100;
  targets[i]=inverseYaw(add(contact,rotateYaw(rotatePitch(sub(l.ankle,l.contact),p.footPitch[i]),footYaw_[i])));
  // Ground soles are authoritative. Elastic locomotion IK accommodates the
  // body and yaw around them instead of relocating a planted contact.

 }
 // Solve body height within the measured legs' common reachable envelope.
 // Weight support remains dynamic, but cannot lift a short-legged body above
 // its planted ankles and ask elastic IK to hide the mismatch.
 p.minimumBodyDown=-1e6;
 if(!detached)for(std::size_t i=0;i<legs_.size();++i){
  if(p.swing[i])continue;
  const auto& leg=legs_[i];
  const double radius=.995*(length(sub(leg.knee,leg.hip))+length(sub(leg.ankle,leg.knee)));
  const double horizontal=std::hypot(leg.hip.x+p.bodyShift.x-targets[i].x,leg.hip.z+p.bodyShift.z-targets[i].z);
  const double vertical=std::sqrt(std::max(0.,radius*radius-horizontal*horizontal));
  p.minimumBodyDown=std::max(p.minimumBodyDown,leg.hip.y-targets[i].y-vertical);
 }
 if(!detached){
  if(turnActive_)down=turnBodyDown_;
  down=std::max(down,p.minimumBodyDown);
  for(std::size_t i=0;i<legs_.size();++i)if(p.swing[i]){
   auto hip=add(legs_[i].hip,p.bodyShift);hip.y-=down;
   const auto delta=sub(targets[i],hip);
   const double reach=.995*(length(sub(legs_[i].knee,legs_[i].hip))+length(sub(legs_[i].ankle,legs_[i].knee)));
   if(length(delta)>reach){
    targets[i]=add(hip,mul(delta,reach/length(delta)));
    auto contact=sub(rotateYaw(targets[i],p.yaw),rotateYaw(rotatePitch(sub(legs_[i].ankle,legs_[i].contact),p.footPitch[i]),p.footYaw[i]));
    contact.z+=state.rootX/100;p.contacts[i]=contact;
   }
  }
 }
 for(std::size_t i=0;i<legs_.size();++i)p.grounded[i]=!detached&&!p.swing[i]&&p.contacts[i].y<=1e-10;
 for(std::size_t i=0;i<legs_.size();++i){auto l=legs_[i];Vec3 hip=add(l.hip,p.bodyShift);hip.y-=down;
  // Keep a supported knee's bend plane aligned with its paw while the torso
  // turns independently; rotate that world pole back into the solve frame.
  // Cats can author opposing fore/hind bend planes. Preserve each measured
  // knee pole, while legacy bipeds keep their existing outward/forward pole.
  Vec3 restBend{l.hip.x>0?.25:-.25,0,1};
  if(quadruped()){
   const auto axis=sub(l.ankle,l.hip),knee=sub(l.knee,l.hip);
   restBend=sub(knee,mul(axis,dot(knee,axis)/std::max(1e-12,dot(axis,axis))));
   // An authored straight chain has no bend plane; retain its role's flex.
   if(length(restBend)<1e-10)restBend={0,0,i<2?-1.:1.};
  }
  const auto bend=rotateYaw(restBend,footYaw_[i]-p.yaw);
  p.legs[i]=solveLeg(hip,targets[i],length(sub(l.knee,l.hip)),length(sub(l.ankle,l.knee)),bend,!detached);
  if(detached){
   p.contacts[i]=rotateYaw(add(p.legs[i].ankle,sub(l.contact,l.ankle)),p.yaw);
   p.contacts[i].z+=state.rootX/100;
  }
 }
 p.head=expressions_.value(pets::character::Role::Head);p.head.x+=p.follower*.12*3.141592653589793/180;
 p.ears={expressions_.value(pets::character::Role::EarLeft),expressions_.value(pets::character::Role::EarRight)};
 p.tail=expressions_.value(pets::character::Role::Tail);p.tail.x-=p.follower*.22*3.141592653589793/180;
 p.wrists={expressions_.value(pets::character::Role::WristLeft),expressions_.value(pets::character::Role::WristRight)};
 const auto sharedRole=[&](const char* role){return expressive.bones[planner_.definition().bindings.at(role)].rotation;};
 p.head.x+=sharedRole("head");p.ears[0].z+=sharedRole("earLeft");p.ears[1].z+=sharedRole("earRight");p.tail.x+=sharedRole("tail");
 if(arms_) for(int i=0;i<2;++i) {
  const double sideSign=(*arms_)[i].shoulder.x>0?1.:-1.;
  auto arm=(*arms_)[i];
  arm.shoulder=add(arm.shoulder,p.bodyShift);arm.elbow=add(arm.elbow,p.bodyShift);arm.hand=add(arm.hand,p.bodyShift);arm.torsoPivot=add(arm.torsoPivot,p.bodyShift);
  // Translate the whole rest chain with the pelvis before solving. Moving only
  // the shoulder would change the measured upper length under ground bob.
  const Vec3 pivot=arm.torsoPivot;
  const auto shoulder=add(pivot,rotatePitch(sub(arm.shoulder,pivot),p.lean));
  const auto shift=sub(shoulder,arm.shoulder);
  arm.shoulder=add(arm.shoulder,shift);arm.elbow=add(arm.elbow,shift);arm.hand=add(arm.hand,shift);
  arm.shoulder.y-=down;arm.elbow.y-=down;arm.hand.y-=down;
  Vec3 goal{reach_[i][0].value(),reach_[i][1].value(),reach_[i][2].value()};
  goal=add(goal,p.bodyShift);
  arm.gripOffset=rotatePitch(rotateRoll(arm.gripOffset,p.wrists[i].z),p.wrists[i].x);
  arm.gripOffset=rotateYaw(arm.gripOffset,p.wrists[i].y);
  const double raised=std::clamp((goal.y-arm.shoulder.y+.2)/.55,0.,1.);
  arm.bend={sideSign,-3*raised,.2*raised};
  // The procedural endpoint opens outward during a raised reach to protect
  // the thick sleeve and cheek, then returns smoothly through neutral.
  const double neutralY=(*arms_)[i].hand.y+(*arms_)[i].gripOffset.y-down;
  const double amount=std::clamp((goal.y-neutralY)/std::max(.1,1.94-neutralY),0.,1.);
  if(p.wrists[i].x>0.05)goal.x+=sideSign*.18*std::sin(amount*3.141592653589793);
  p.arms[i]=solveArm(arm,goal);
 }
 return p;
}
}
