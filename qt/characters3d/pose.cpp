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
 Locomotion l;l.legs.setCount(legs.size());l.gait=gait;l.groundY=220;l.speed=70;l.acceleration=240;l.reachThreshold=18;l.stride=26;l.lift=8;l.minStepDuration=.20;l.maxStepDuration=.30;
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
  l.reachThreshold=std::clamp(reach*.3,.1,128.);l.stride=std::clamp(reach*.45,.1,128.);l.lift=std::min(64.,reach*.12);l.speed=std::clamp(reach*2.2,.1,500.);
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
 contacts_.setCount(l.size());footYaw_.setCount(l.size());swingStartYaw_.setCount(l.size());swingEndYaw_.setCount(l.size());swingContactOffset_.setCount(l.size());supportRestLength_.setCount(l.size());
 if(arms_)for(int i=0;i<2;++i){auto neutral=add((*arms_)[i].hand,(*arms_)[i].gripOffset);double values[]={neutral.x,neutral.y,neutral.z};for(int axis=0;axis<3;++axis){reach_[i][axis].setTuning({2.5,1.,0});reach_[i][axis].reset(values[axis]);}}
 // A short lead lets the support paw travel beneath and behind its hip
 // instead of recovering early to a permanently forward-biased target.
 planner_.setLandingLeadFraction(.1);
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
 for(std::size_t i=0;i<legs_.size();++i){
  const auto hip=neutral.legs[i].hip,ankle=neutral.legs[i].ankle;
  supportRestLength_[i]=pets::motion::neutralSupportRestLength(
   {hip.x*100,-hip.y*100,hip.z*100},{ankle.x*100,-ankle.y*100,ankle.z*100},
   gravity,support.stiffness,l.size());
 }
}
void Motion::setSpeedMultiplier(double multiplier){
 planner_.setSpeedMultiplier(multiplier);
 pendingPlanner_.reset();pendingDelta_=0;
 // Travel speed does not change the timing of turns or body followers.

}
void Motion::setFootPlantDuration(double seconds){
 planner_.setFootPlantDuration(seconds);
 pendingPlanner_.reset();pendingDelta_=0;
 footPlantDuration_=seconds;
 turnCooldown_=std::min(turnCooldown_,seconds);
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
 if(arms_)for(int i=0;i<2;++i){auto v=add((*arms_)[i].hand,(*arms_)[i].gripOffset);v.y+=pose().legs[0].hip.y-legs_[0].hip.y;reach_[i][0].reset(v.x);reach_[i][1].reset(v.y);reach_[i][2].reset(v.z);}
 footYaw_.fill(yawTarget_);swingStartYaw_=footYaw_;swingEndYaw_=footYaw_;swingContactOffset_.fill({});
 turningFoot_=-1;turnTime_=turnStartYaw_=turnEndYaw_=turnCooldown_=0;
 for(std::size_t i=0;i<legs_.size();++i){
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  contacts_[i]=rotateYaw(neutral,yawTarget_);
  contacts_[i].z+=requestedTarget_/100;
 }
}
void Motion::setSpan(pets::character::GroundSpan span){reset(planner_.state().rootX,span);}
void Motion::clearPickupMotion(){
 pendingPlanner_.reset();pendingDelta_=0;
 requestedTarget_=planner_.state().rootX;
 turningFoot_=-1;turnTime_=turnStartYaw_=turnEndYaw_=turnCooldown_=0;
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
 for(std::size_t i=0;i<legs_.size();++i){
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  contacts_[i]=rotateYaw(neutral,footYaw_[i]);
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
   if(turningFoot_>=0)turnStart_.z+=delta;
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
 planner_.setTargetX(x);
 pendingPlanner_.reset();pendingDelta_=0;requestedTarget_=planner_.state().targetX;
 double delta=requestedTarget_-planner_.state().rootX;
 if(std::abs(delta)>headingHomeRadius){facingLeft_=delta<0;retargetYaw();}
}
bool Motion::face(double worldX){
 if(!std::isfinite(worldX))return false;
 const double delta=worldX-planner_.state().rootX;
 if(std::abs(delta)<=headingHomeRadius)return false;
 const double previousTarget=yawTarget_;
 facingLeft_=delta<0;retargetYaw();
 return std::abs(yawTarget_-previousTarget)>=1e-12;
}
bool Motion::feetTurning()const{for(double yaw:footYaw_)if(std::abs(yaw-yawTarget_)>1e-8)return true;return false;}
double Motion::yawStepGoal(int foot,double increment)const{
 double low=footYaw_[foot]-increment,high=footYaw_[foot]+increment;
 for(double yaw:footYaw_){low=std::max(low,yaw-increment);high=std::min(high,yaw+increment);}
 return std::clamp(yawTarget_,low,high);
}
bool Motion::settled()const{
 return expressions_.settled() && reachSettled() && turningFoot_<0 && !feetTurning() &&
  std::abs(planner_.state().rootX-requestedTarget_)<.02 && planner_.settled() && yaw_.settled(yawTarget_) && follower_.settled(0);
}
pets::character::GroundedState Motion::state()const{auto s=planner_.state();s.targetX=requestedTarget_;s.active=!settled();return s;}
pets::character::MotionProposal Motion::propose(double dt){
 if(!std::isfinite(dt)||dt<0)throw std::runtime_error("invalid 3D motion delta");
 pendingDelta_=std::min(dt,1./30);
 pendingPlanner_=planner_;
 auto& pending=*pendingPlanner_;
 const auto supported=pose();
 if(quadruped())pending.setSupportContacts(supported.grounded.array<4>());else pending.setSupportContacts(supported.grounded.array<2>());
 pending.setSwingAdmission(turningFoot_<0);
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
 return pending.propose(pendingDelta_);
}
void Motion::step(double dt){
 const auto proposal=propose(dt);
 realize(planner_.state().rootX+proposal.displacement,false,
         std::max(0.,planner_.state().elevation-proposal.displacementY));
}
void Motion::realize(double rootX,bool blocked,std::optional<double> elevation,double ceilingTolerance){
 if(!std::isfinite(rootX)||(elevation&&(!std::isfinite(*elevation)||*elevation<0)))
  throw std::runtime_error("invalid 3D realized root");
 if(!pendingPlanner_||pendingDelta_==0)return;
 const double dt=pendingDelta_;
 const auto before=planner_.state();
 planner_=std::move(*pendingPlanner_);
 pendingPlanner_.reset();pendingDelta_=0;
 turnCooldown_=std::max(0.,turnCooldown_-dt);
 bool turning=turningFoot_>=0 || feetTurning();
 // Navigation remains body-led through reversal. Feet rotate while lifted;
 // a turn never freezes the root to wait for a support transfer.
 planner_.realize(rootX,blocked,elevation,ceilingTolerance);
 auto after=planner_.state();
 requestedTarget_=after.targetX;
 const auto detached=[](pets::character::DropPhase phase){
  return phase==pets::character::DropPhase::Held||phase==pets::character::DropPhase::Airborne;
 };
 if(detached(before.phase)&&!detached(after.phase))installContacts();
 const double turnIncrement=.4;
 const double turnDuration=flightCapable()?.065:.16;
 const double stanceTwist=.3;
 if(!detached(after.phase)){
 for(std::size_t i=0;i<legs_.size();++i)if(before.feet[i].swing||after.feet[i].swing){
  if(after.feet[i].swing) {
   if(turningFoot_==i)turningFoot_=-1;
   if(!before.feet[i].swing) {
    swingStartYaw_[i]=footYaw_[i];
    swingEndYaw_[i]=yawStepGoal(i,turnIncrement);
   }
   footYaw_[i]=swingStartYaw_[i]+(swingEndYaw_[i]-swingStartYaw_[i])*ease(after.feet[i].phase);
  }
  if(before.feet[i].swing&&!after.feet[i].swing)footYaw_[i]=swingEndYaw_[i];
  Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
  auto rotated=rotateYaw(neutral,footYaw_[i]);
  Vec3 contact{rotated.x,-(after.feet[i].position.y-supportFloor_)/100,after.feet[i].position.x/100+rotated.z-(quadruped()?after.facing:1)*legs_[i].hip.z};
  // A normal catch-up may interrupt a turn replant. Preserve its presented
  // starting contact and blend away the offset over the new lifted phase.
  if(!before.feet[i].swing&&after.feet[i].swing)swingContactOffset_[i]=sub(contacts_[i],contact);
  contacts_[i]=add(contact,mul(swingContactOffset_[i],after.feet[i].swing?1-ease(after.feet[i].phase):0));
  boundTouchdown(contacts_[i]);
 }
 if(turning&&!after.anyFootSwinging()){
  if(turningFoot_<0&&turnCooldown_==0){
   int pick=0;for(std::size_t i=1;i<legs_.size();++i)if(std::abs(footYaw_[i]-yawTarget_)>std::abs(footYaw_[pick]-yawTarget_))pick=int(i);
   if(std::abs(footYaw_[pick]-yawTarget_)>1e-8){
    turningFoot_=pick;turnTime_=0;turnStart_=contacts_[pick];turnStartYaw_=footYaw_[pick];
    // Each foot advances at most one small yaw increment beyond its support.
    turnEndYaw_=yawStepGoal(pick,turnIncrement);
   }
  }
  if(turningFoot_>=0){
   int i=turningFoot_;turnTime_+=dt;double t=std::min(1.,turnTime_/turnDuration),smooth=ease(t);
   footYaw_[i]=turnStartYaw_+(turnEndYaw_-turnStartYaw_)*smooth;
   Vec3 neutral=legs_[i].contact;neutral.z=legs_[i].hip.z;
   auto rotated=rotateYaw(neutral,footYaw_[i]);auto start=rotateYaw(neutral,turnStartYaw_);
   Vec3 base{rotated.x,0,after.rootX/100+rotated.z};
   Vec3 startBase{start.x,0,after.rootX/100+start.z};
   contacts_[i]=add(base,mul(sub(turnStart_,startBase),1-smooth));contacts_[i].y+=.08*lift(t);
   boundTouchdown(contacts_[i]);
   if(t==1){turningFoot_=-1;turnCooldown_=footPlantDuration_;}
  }
 }
 // Keep the torso within the stance yaw while the other foot replants.
 double low=*std::max_element(footYaw_.begin(),footYaw_.end())-stanceTwist,high=*std::min_element(footYaw_.begin(),footYaw_.end())+stanceTwist;
 const double previousYaw=yaw_.value();
 yaw_.step(dt,std::clamp(yawTarget_,low,high));
 if(cameraFacingTurns_){
  // The yaw spring may overshoot its target; resident presentation cannot
  // expose the back or twist beyond the currently planted paw stance.
  low=std::max(low,-3.141592653589793+std::abs(facingBias_));
  high=std::min(high,-std::abs(facingBias_));
  // Limit the stronger spring's angular displacement as well as its heading.
  const double limited=std::clamp(yaw_.value(),previousYaw-4.7*dt,previousYaw+4.7*dt);
  const double bounded=low<=high?std::clamp(limited,low,high):limited;
  if(bounded!=yaw_.value())yaw_.reset(bounded);
 }
 } else {
  yaw_.step(dt,yawTarget_);footYaw_.fill(yaw_.value());
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
 auto state=planner_.state();auto expressive=planner_.pose(pets::character::Action::Rest);Pose p;p.rootX=state.rootX;p.yaw=yaw_.value();p.lean=expressive.bones[0].rotation*std::cos(p.yaw);p.follower=follower_.value();p.contacts=contacts_;p.footYaw=footYaw_;p.wings=state.wings;
 double down=(expressive.bones[0].y-planner_.definition().bones[0].rest.y)/100;
 const bool detached=state.phase==pets::character::DropPhase::Held||state.phase==pets::character::DropPhase::Airborne;
 const auto sharedWorld=detached?pets::character::worldTransforms(planner_.definition(),expressive):std::vector<pets::character::WorldTransform>{};
 LimbArray<Vec3> targets;targets.setCount(legs_.size());
 p.legs.setCount(legs_.size());p.contacts.setCount(legs_.size());p.footYaw.setCount(legs_.size());p.footPitch.setCount(legs_.size());p.swing.setCount(legs_.size());p.grounded.setCount(legs_.size());
 auto inverseYaw=[&](Vec3 v){double c=std::cos(p.yaw),s=std::sin(p.yaw);return Vec3{c*v.x-s*v.z,v.y,s*v.x+c*v.z};};
 for(std::size_t i=0;i<legs_.size();++i){auto l=legs_[i];
  if(detached){
   // Shared detached IK supplies body-relative goals in its forward/down
   // plane. Preserve actual lateral anatomy and solve measured 3D lengths.
   const auto ankle=planner_.definition().locomotion->legs[i].ankle;
   targets[i]={l.ankle.x,(planner_.definition().locomotion->groundY-sharedWorld[ankle].ty)/100,
               (sharedWorld[ankle].tx-planner_.definition().origin.x)/100};
   p.swing[i]=true;p.footYaw[i]=p.yaw;p.footPitch[i]=0;
   continue;
  }
  p.swing[i]=state.feet[i].swing||turningFoot_==i;
  const double phase=state.feet[i].phase;
  p.footPitch[i]=state.feet[i].swing?-.45*std::sin(2*3.141592653589793*phase)*lift(phase):0;
  // Supported feet keep their own world orientation; a lifted foot rotates
  // with its replant rather than inheriting a torso-only spin.
  Vec3 contact=contacts_[i];contact.z-=state.rootX/100;
  targets[i]=inverseYaw(add(contact,rotateYaw(rotatePitch(sub(l.ankle,l.contact),p.footPitch[i]),footYaw_[i])));
  // Ground soles are authoritative. Elastic locomotion IK accommodates the
  // body and yaw around them instead of relocating a planted contact.

 }
 for(std::size_t i=0;i<legs_.size();++i)p.grounded[i]=!detached&&p.contacts[i].y<=1e-10;
 for(std::size_t i=0;i<legs_.size();++i){auto l=legs_[i];Vec3 hip=l.hip;hip.y-=down;
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
  auto arm=(*arms_)[i];
  // Translate the whole rest chain with the pelvis before solving. Moving only
  // the shoulder would change the measured upper length under ground bob.
  const Vec3 pivot=arm.torsoPivot;
  const auto shoulder=add(pivot,rotatePitch(sub(arm.shoulder,pivot),p.lean));
  const auto shift=sub(shoulder,arm.shoulder);
  arm.shoulder=add(arm.shoulder,shift);arm.elbow=add(arm.elbow,shift);arm.hand=add(arm.hand,shift);
  arm.shoulder.y-=down;arm.elbow.y-=down;arm.hand.y-=down;
  Vec3 goal{reach_[i][0].value(),reach_[i][1].value(),reach_[i][2].value()};
  arm.gripOffset=rotatePitch(rotateRoll(arm.gripOffset,p.wrists[i].z),p.wrists[i].x);
  arm.gripOffset=rotateYaw(arm.gripOffset,p.wrists[i].y);
  const double raised=std::clamp((goal.y-arm.shoulder.y+.2)/.55,0.,1.);
  arm.bend={arm.shoulder.x>0?1.:-1.,-3*raised,.2*raised};
  // The procedural endpoint opens outward during a raised reach to protect
  // the thick sleeve and cheek, then returns smoothly through neutral.
  const double neutralY=(*arms_)[i].hand.y+(*arms_)[i].gripOffset.y-down;
  const double amount=std::clamp((goal.y-neutralY)/std::max(.1,1.94-neutralY),0.,1.);
  if(p.wrists[i].x>0.05)goal.x+=(arm.shoulder.x>0?1:-1)*.18*std::sin(amount*3.141592653589793);
  p.arms[i]=solveArm(arm,goal);
 }
 return p;
}
}
