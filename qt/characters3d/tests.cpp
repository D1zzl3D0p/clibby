#include "pose.h"
#include "characters/motion_support.h"
#include "characters/ik.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace pets::character3d;
double distance(Vec3 a,Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
Vec3 rotate(Vec3 v,double yaw){return {std::cos(yaw)*v.x+std::sin(yaw)*v.z,v.y,-std::sin(yaw)*v.x+std::cos(yaw)*v.z};}
Vec3 pitch(Vec3 v,double p){return {v.x,std::cos(p)*v.y-std::sin(p)*v.z,std::sin(p)*v.y+std::cos(p)*v.z};}
void check(bool x,const char *s){if(!x)throw std::runtime_error(s);}
int main(){try {
 // Planar motion must be the zero-depth case of the spatial solver, including
 // targets outside reach. Independent clamping rules previously diverged.
 for(double direction:{-1.,1.})for(double tx:{.1,5.,100.}) {
  const auto planar=pets::character::solveTwoLink({0,0},{tx,3},10,7,int(direction));
  const auto spatial=solveLeg({0,0,0},{tx,3,0},10,7,{-3*direction,tx*direction,0});
  check(distance(spatial.knee,{planar.knee.x,planar.knee.y,0})<1e-10,
        "planar/spatial knee solving differs");
  check(distance(spatial.ankle,{planar.end.x,planar.end.y,0})<1e-10,
        "planar/spatial reach solving differs");
 }
 // Planar support is the same spatial equation with one horizontal axis zero.
 std::array<pets::motion::SupportLeg,2> planarSupport{{
  {{0,0,0},{.1,.78,0},.45,.4,1},{{0,0,0},{-.1,.78,0},.45,.4,1}}};
 auto spatialSupport=planarSupport;
 for(auto& leg:spatialSupport){leg.ankle.z=leg.ankle.x;leg.ankle.x=0;}
 pets::motion::SupportDynamics dynamics{120,24,6};
 pets::motion::SupportState planarState,spatialState;
 auto neutralSupport=planarSupport;
 for(auto& leg:neutralSupport)leg.restLength=pets::motion::neutralRestLength(leg.hip,leg.ankle,3,120);
 check(std::abs(pets::motion::supportAcceleration(neutralSupport,dynamics,0,0))<1e-12,
  "neutral spatial spring calibration does not balance body weight");
 for(int frame=0;frame<120;++frame) {
  pets::motion::stepSupport(planarState,planarSupport,1./240,dynamics);
  pets::motion::stepSupport(spatialState,spatialSupport,1./240,dynamics);
  check(std::abs(planarState.offset-spatialState.offset)<1e-12&&
        std::abs(planarState.velocity-spatialState.velocity)<1e-12,
        "planar and spatial contact support differ");
 }
 for(auto& leg:planarSupport)leg.load=0;
 const auto unsupported=planarState;
 pets::motion::stepSupport(planarState,planarSupport,1./240,dynamics);
 check(std::abs(planarState.acceleration-dynamics.gravity)<1e-12&&
       planarState.velocity>unsupported.velocity,"common support holds an airborne body up");
 Arm arm{{.3,1.6,0},{.5,1.2,0},{.6,.95,0},{0,-.05,.02},{0,0,1}};
 auto reached=solveArm(arm,{.6,1.3,.3});
 check(distance(reached.grip,{.6,1.3,.3})<1e-7,"grip misses reachable goal");
 check(std::abs(distance(reached.shoulder,reached.elbow)-distance(arm.shoulder,arm.elbow))<1e-7,"arm upper length changes");
 check(std::abs(distance(reached.elbow,reached.hand)-distance(arm.elbow,arm.hand))<1e-7,"arm lower length changes");
 auto clipped=solveArm(arm,{100,100,100});
 check(distance(clipped.shoulder,clipped.hand)<=distance(arm.shoulder,arm.elbow)+distance(arm.elbow,arm.hand),"arm reach unbounded");
 bool invalid=false;try { auto bad=arm;bad.bend={NAN,0,0};solveArm(bad,{0,1,0}); }catch(const std::exception&){invalid=true;}
 check(invalid,"nonfinite bend accepted");
 auto folded=solveArm(arm,{arm.shoulder.x+arm.gripOffset.x,arm.shoulder.y+arm.gripOffset.y,arm.shoulder.z+arm.gripOffset.z});
 check(std::isfinite(folded.hand.x)&&std::abs(distance(folded.elbow,folded.hand)-distance(arm.elbow,arm.hand))<1e-7,"coincident arm target invalid");
 auto s=solveLeg({0,1,0},{.04,.1,.2},.45,.40,{0,0,1});
 check(std::abs(distance(s.hip,s.knee)-.45)<1e-8,"upper segment changed");
 check(std::abs(distance(s.knee,s.ankle)-.40)<1e-8,"lower segment changed");
 auto unreachable=solveLeg({0,1,0},{0,-100,0},.45,.4,{0,0,1});
 check(std::abs(distance(unreachable.hip,unreachable.ankle)-.85)<1e-7,"unreachable solve unbounded");
 std::array<Leg,2> legs{{{{.18,1.04,0},{.22,.60,.08},{.22,.25,-.12},{.22,0,.105}},
                       {{-.18,1.04,0},{-.22,.60,.08},{-.22,.25,-.12},{-.22,0,.105}}}};
 // Extreme throws preserve physical velocity; expressive followers use only
 // bounded secondary intent and must never receive the throw's full speed.
 for(double stride:{.5,1.,3.})for(double direction:{-1.,1.}){
  Motion thrown(legs);thrown.reset(0,{-100000,100000});thrown.setSpeedMultiplier(stride);
  thrown.pickup(0,180);thrown.release({direction*10000,-5000});
  const auto initial=thrown.state();
  check(initial.velocity==direction*10000&&initial.verticalVelocity==-5000,
   "3D release caps or amplifies the physical throw velocity");
  const auto proposal=thrown.propose(1./240);
  check(thrown.state().velocity==initial.velocity&&thrown.state().verticalVelocity==initial.verticalVelocity,
   "3D throw proposal changes presented velocity before realization");
  thrown.realize(proposal.displacement,false,initial.elevation-proposal.displacementY);
  check(std::abs(thrown.state().velocity)>1000&&std::abs(thrown.state().velocity)<10000&&
        thrown.state().elevation>initial.elevation,
   "3D expressive safety clamps body motion instead of dissipating drag");
  for(int tick=0;tick<120;++tick){
   thrown.step(1./120);const auto p=thrown.pose();
   check(std::isfinite(p.head.x)&&std::isfinite(p.tail.x)&&std::abs(p.head.x)<.5&&std::abs(p.tail.x)<.75,
    "extreme 3D throw produces an invalid or excessive secondary rotation");
   for(int side=0;side<2;++side)check(std::isfinite(p.legs[side].ankle.x)&&std::isfinite(p.legs[side].ankle.y),
    "extreme 3D throw produces an invalid limb pose");
  }
 }
 // Actual werewolf arm heads and grip offsets: a falling body spreads its
 // arms through the same endpoint followers, then settles back to rest.
 {
  const std::array<Arm,2> measuredArms{{
   {{.33,1.58,0},{.49,1.22,-.01},{.57,.94,.045},{0,-.08,0},{1,-.4,.2}},
   {{-.33,1.58,0},{-.49,1.22,-.01},{-.57,.94,.045},{0,-.08,0},{-1,-.4,.2}}}};
  Motion fallingArms(legs,measuredArms);fallingArms.reset(0,{-2000,2000});
  fallingArms.pickup(0,600);for(int tick=0;tick<1200&&!fallingArms.settled();++tick)fallingArms.step(1./120);
  const auto held=fallingArms.pose();
  for(int side=0;side<2;++side)check(std::abs(held.arms[side]->grip.x)<.7,
   "stationary held werewolf spreads its arms as if falling");
  fallingArms.release({300,0});
  check(distance(fallingArms.pose().arms[0]->grip,held.arms[0]->grip)<1e-12,
   "release snaps directly into the falling arm pose");
  auto previous=fallingArms.pose();bool observedSpread=false,landed=false;
  for(int tick=0;tick<1800&&!fallingArms.settled();++tick){
   fallingArms.step(1./120);const auto p=fallingArms.pose();
   for(int side=0;side<2;++side){
    const auto& a=*p.arms[side];const auto& rest=measuredArms[side];
    check(std::abs(distance(a.shoulder,a.elbow)-distance(rest.shoulder,rest.elbow))<1e-8&&
          std::abs(distance(a.elbow,a.hand)-distance(rest.elbow,rest.hand))<1e-8,
     "falling arm changes its measured anatomical lengths");
    check(std::hypot(a.hand.x,a.hand.z)<1.2&&std::hypot(a.elbow.x,a.elbow.z)<1.2&&a.hand.y<2.2,
     "falling arm exceeds the resident presentation envelope");
    // Match the existing live arm-continuity tolerance at 120 Hz; impact
    // compression and the endpoint follower both contribute to this delta.
    check(distance(a.grip,previous.arms[side]->grip)<.065,
     "falling arm snaps across landing or recovery");
   }
   if(tick==11)for(int side=0;side<2;++side)check(std::abs(p.arms[side]->grip.x)>.8&&p.arms[side]->grip.y>1.3,
    "short fall does not show an outstretched arm response within 100 ms");
   if(tick==35){
    check(fallingArms.state().phase==pets::character::DropPhase::Airborne,
     "falling arm regression lands before the observed spread");
    for(int side=0;side<2;++side)check(std::abs(p.arms[side]->grip.x)>.8&&p.arms[side]->grip.y>1.3,
     "unpowered airborne werewolf does not outstretch both arms");
    observedSpread=true;
   }
   landed|=fallingArms.state().phase!=pets::character::DropPhase::Airborne;
   previous=p;
  }
  check(observedSpread&&landed&&fallingArms.settled(),"falling arm response never returns to quiet rest");
  const auto recovered=fallingArms.pose();
  for(int side=0;side<2;++side){
   auto neutral=Vec3{measuredArms[side].hand.x,measuredArms[side].hand.y-.08,measuredArms[side].hand.z};
   neutral.y+=recovered.legs[0].hip.y-legs[0].hip.y;
   check(distance(recovered.arms[side]->grip,neutral)<.002,"recovered fall arms retain an outstretched goal");
  }
  Motion powered(legs,measuredArms,true);check(powered.flyTo(0,200),"powered arm isolation fixture cannot fly");
  for(int tick=0;tick<40;++tick)powered.step(1./120);
  check(powered.state().poweredFlight,"powered arm isolation fixture loses flight control");
  for(int side=0;side<2;++side)check(std::abs(powered.pose().arms[side]->grip.x)<.7,
   "outstretched fall arms override powered wing motion");
 }
 // Pickup interrupts both staged turning and an unconsumed host proposal.
 // Local rig height must not include the elevation applied by the host window.
 {
  using pets::character::DropPhase;
  Motion held(legs);held.target(-100);for(int tick=0;tick<10;++tick)held.step(1./120);
  held.propose(1./60);held.pickup(25,180);
  held.realize(26);
  check(held.state().rootX==25&&held.state().phase==DropPhase::Held,
   "pickup realizes a stale 3D proposal");
  const auto low=held.pose();held.hold(25,600);const auto high=held.pose();
  check(distance(low.legs[0].hip,high.legs[0].hip)<1e-10,"held elevation moves the local rig twice");
  for(int tick=0;tick<1200&&!held.settled();++tick)held.step(1./120);
  check(held.settled(),"stationary 3D hold retains a motion deadline");
  for(int side=0;side<2;++side){
   const auto p=held.pose();
   check(!p.grounded[side],"held 3D paw remains a support contact");
   check(std::abs(distance(p.legs[side].hip,p.legs[side].knee)-distance(legs[side].hip,legs[side].knee))<1e-8&&
         std::abs(distance(p.legs[side].knee,p.legs[side].ankle)-distance(legs[side].knee,legs[side].ankle))<1e-8,
    "held 3D leg stretches toward an old floor pin");
  }
 }
 // Flight consumes the shared vertical proposal and touchdown installs fresh
 // pins once. Recovery becomes quiet and never replays interrupted travel.
 for(double height:{30.,180.,600.}){
  using pets::character::DropPhase;
  Motion falling(legs);falling.reset(25,{-100,100});falling.target(-80);
  falling.pickup(25,height);falling.release({0,0});
  check(falling.state().elevation==height,"3D release changes elevated placement");
  auto before=falling.pose();bool landed=false;double previousHeight=height;
  for(int tick=0;tick<6000&&!falling.settled();++tick){
   const auto phaseBefore=falling.state().phase;falling.step(1./120);
   const auto s=falling.state();const auto p=falling.pose();
   if(s.phase==DropPhase::Airborne){
    check(s.elevation<=previousHeight+1e-10,"3D drop ignores shared vertical travel");
    check(!p.grounded[0]&&!p.grounded[1],"airborne 3D paw claims support");
    for(int side=0;side<2;++side)
     check(distance(p.legs[side].hip,p.legs[side].ankle)<=distance(legs[side].hip,legs[side].knee)+distance(legs[side].knee,legs[side].ankle)+1e-8,
      "airborne 3D leg exceeds anatomical reach");
   }else{
    landed=true;check(p.grounded[0]||p.grounded[1],"3D landing has no physical sole support");
    if(phaseBefore!=DropPhase::Airborne)for(int side=0;side<2;++side)
     if(before.grounded[side]&&p.grounded[side])check(distance(before.contacts[side],p.contacts[side])<1e-10,
      "3D recovery slides its touchdown contact");
   }
   check(s.elevation>=0&&std::isfinite(p.legs[0].hip.y),"3D drop tunnels or produces an invalid body");
   before=p;previousHeight=s.elevation;
  }
  check(landed&&falling.settled()&&falling.state().phase==DropPhase::Grounded,
   "3D landing/recovery never settles");
  check(std::abs(falling.state().rootX-25)<1e-8,"3D recovery replays interrupted travel");
  falling.pickup(40,100);falling.release({80,100});falling.step(1./120);
  const auto grabbed=falling.state();falling.propose(1./60);falling.pickup(grabbed.rootX,grabbed.elevation);
  falling.realize(grabbed.rootX+10,false,0);
  check(falling.state().rootX==grabbed.rootX&&falling.state().elevation==grabbed.elevation&&
        falling.state().phase==DropPhase::Held,"re-grab loses airborne placement or consumes stale feedback");
 }
 // Use the production rig's measured ankle-to-sole offset, rather than
 // trusting a contact flag, at the last airborne/first landed boundary.
 for(bool left:{false,true})for(double height:{30.,180.,600.}){
  using pets::character::DropPhase;
  Motion crossing(legs);crossing.setFacingBias(-.6108652381980153);
  crossing.setCameraFacingTurns(true);crossing.reset(0,{-100,100});
  if(left){crossing.target(-80);for(int tick=0;tick<6000&&!crossing.settled();++tick)crossing.step(1./120);}
  crossing.pickup(crossing.state().rootX,height);crossing.release({0,0});
  auto lastPose=crossing.pose();auto lastState=crossing.state();
  bool landed=false;
  for(int tick=0;tick<10000&&crossing.state().phase==DropPhase::Airborne;++tick){
   lastPose=crossing.pose();lastState=crossing.state();
   const double nearFloor=std::max(2.,lastState.verticalVelocity/60);
   crossing.step(lastState.elevation>nearFloor?1./120:1e-5);
   landed=crossing.state().phase!=DropPhase::Airborne;
  }
  check(landed,"3D touchdown continuity probe never reaches the floor");
  const auto grounded=crossing.pose();
  auto actualSole=[&](const Pose& p,int side,double elevation){
   const auto ankle=rotate(p.legs[side].ankle,p.yaw);
   const auto& l=legs[side];
   const auto offset=rotate(pitch({l.contact.x-l.ankle.x,l.contact.y-l.ankle.y,l.contact.z-l.ankle.z},p.footPitch[side]),p.footYaw[side]);
   return Vec3{ankle.x+offset.x,ankle.y+offset.y+elevation/100,ankle.z+offset.z+p.rootX/100};
  };
  for(int side=0;side<2;++side){
   const auto before=actualSole(lastPose,side,lastState.elevation);
   const auto after=actualSole(grounded,side,0);
   check(!lastPose.grounded[side]&&grounded.grounded[side]&&std::abs(after.y)<1e-8,
    "3D contact flags disagree with measured touchdown soles");
   check(std::abs(before.y)<.0005&&distance(before,after)<.0005,
    "measured 3D sole jumps from airborne preparation to floor contact");
  }
 }
 // An explicit monitor repair invalidates obsolete pins in the old span.
 // Its large world translation must not become local leg extension.
 {
  Motion moved(legs);moved.setFacingBias(-.6108652381980153);moved.reset(0,{-100,100});
  const auto before=moved.pose();moved.propose(1./60);moved.reconcile(5000,{4900,5100},0);
  moved.realize(5001);
  const auto after=moved.pose();
  check(after.rootX==5000,"3D monitor repair consumes obsolete motion feedback");
  for(int side=0;side<2;++side){
   check(distance(before.legs[side].hip,after.legs[side].hip)<1e-8&&
         distance(before.legs[side].knee,after.legs[side].knee)<1e-8&&
         distance(before.legs[side].ankle,after.legs[side].ankle)<1e-8,
    "3D monitor repair stretches a limb toward an obsolete display pin");
   check(std::abs(after.contacts[side].z-before.contacts[side].z-50)<1e-8,
    "3D monitor repair leaves its contact on the old display");
  }
 }
 {
  Motion supportedRelease(legs);supportedRelease.pickup(50,0);supportedRelease.hold(70,0);
  supportedRelease.release({0,0});const auto p=supportedRelease.pose();
  for(int side=0;side<2;++side){
   Vec3 neutral=legs[side].contact;neutral.z=legs[side].hip.z;
   auto expected=rotate(neutral,p.footYaw[side]);expected.z+=.7;
   check(p.grounded[side]&&distance(p.contacts[side],expected)<1e-10,
    "zero-height 3D release retains a pre-pickup contact");
  }
 }
 {
  using pets::character::DropPhase;
  Motion screen(legs);screen.reset(0,{-100,100});screen.target(-80);
  for(int tick=0;tick<20;++tick)screen.step(1./120);
  screen.reconcile(50,{-100,100},150);
  check(screen.state().rootX==50&&screen.state().elevation==150&&
        !screen.pose().grounded[0]&&!screen.pose().grounded[1],
   "3D display reconciliation snaps elevation or preserves floor support");
  screen.reconcile(50,{-100,100},0);
  check(screen.pose().grounded[0]||screen.pose().grounded[1],"raised display floor fails to install 3D contacts");
  for(int tick=0;tick<6000&&!screen.settled();++tick)screen.step(1./120);
  check(screen.settled()&&screen.state().phase==DropPhase::Grounded,
   "display contact repair retains a stale 3D replant or target");
 }
 {
  auto other=arm;other.shoulder.x*=-1;other.elbow.x*=-1;other.hand.x*=-1;
  Motion interrupted(legs,std::array<Arm,2>{arm,other});interrupted.armGoal(0,{.6,1.7,.3});
  for(int tick=0;tick<300;++tick)interrupted.step(1./120);
  interrupted.pickup(20,200);
  for(int tick=0;tick<1200&&!interrupted.settled();++tick)interrupted.step(1./120);
  const auto p=interrupted.pose();
  check(interrupted.settled()&&p.arms[0]&&p.arms[0]->grip.y<1.2,
   "pickup retains a stale 3D arm reach");
 }
 // Resident facing calibration changes the authored heading, never the
 // travel axis. Both soles and torso begin in the same biased frame.
 {
  constexpr double bias=.6108652381980153;
  Motion quarter(legs);quarter.setFacingBias(-bias);quarter.reset(215,{100,300});
  auto rest=quarter.pose();
  check(quarter.settled()&&std::abs(rest.yaw+bias)<1e-12,"biased neutral heading is not settled");
  for(int side=0;side<2;++side){
   Vec3 neutral=legs[side].contact;neutral.z=legs[side].hip.z;
   auto expected=rotate(neutral,-bias);expected.z+=2.15;
   check(std::abs(rest.footYaw[side]+bias)<1e-12&&distance(rest.contacts[side],expected)<1e-12,
    "biased reset leaves a sole in the unrotated frame");
  }
  for(double destination:{280.,120.,260.}){
   quarter.target(destination);const double heading=destination>quarter.state().rootX?-bias:3.141592653589793-bias;
   for(int tick=0;tick<6000&&!quarter.settled();++tick){
    const auto before=quarter.pose();quarter.step(1./120);const auto p=quarter.pose();
    check(p.grounded[0]||p.grounded[1],"biased heading lifts both support soles");
    for(int side=0;side<2;++side){
     if(before.grounded[side]&&p.grounded[side])
      check(distance(before.contacts[side],p.contacts[side])<1e-10,"biased reversal slides a planted sole");
     const auto ankle=rotate(p.legs[side].ankle,p.yaw);
     const auto offset=rotate(pitch({legs[side].contact.x-legs[side].ankle.x,legs[side].contact.y-legs[side].ankle.y,legs[side].contact.z-legs[side].ankle.z},p.footPitch[side]),p.footYaw[side]);
     check(distance({ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+p.rootX/100},p.contacts[side])<1e-7,
      "biased heading loses its actual sole pivot");
    }
   }
   check(quarter.settled()&&std::abs(quarter.pose().yaw-heading)<1e-8&&std::abs(quarter.state().rootX-destination)<.02,
    "biased travel does not settle at its root and heading");
  }
  const auto before=quarter.pose();quarter.setFacingBias(.3);const auto adjusted=quarter.pose();
  for(int side=0;side<2;++side)check(distance(before.contacts[side],adjusted.contacts[side])<1e-12,
   "facing configuration relocates a pinned sole");
  for(double bad:{double(NAN),double(INFINITY),1.1,-1.1}){
   bool rejected=false;try{quarter.setFacingBias(bad);}catch(const std::exception&){rejected=true;}
   check(rejected,"facing bias accepts a nonfinite or excessive angle");
  }
 }
 // Resident turns mirror the quarter view around the camera instead of
 // showing the character's back. Support pivots retain actual sole contacts.
 {
  constexpr double pi=3.141592653589793,bias=.6108652381980153;
  Motion front(legs);front.setFacingBias(-bias);front.setCameraFacingTurns(true);front.reset(0,{-120,120});
  Motion preview(legs);preview.setFacingBias(-bias);preview.reset(0,{-120,120});
  front.target(-100);preview.target(-100);
  for(int tick=0;tick<12;++tick){front.step(1./600);preview.step(1./600);}
  bool frontFootLed=false,previewFootLed=false;
  for(int side=0;side<2;++side){
   frontFootLed|=std::abs(front.pose().footYaw[side]+bias)>1e-5;
   previewFootLed|=std::abs(preview.pose().footYaw[side]+bias)>1e-5;
  }
  check(frontFootLed&&previewFootLed&&std::abs(front.pose().yaw+bias)<1e-10&&std::abs(preview.pose().yaw+bias)<1e-10,
   "resident or preview body rotates before its leading foot");
  // Repeated external target refresh must not restart the turn plan.
  Motion refreshed=front,uninterrupted=front;
  for(int tick=0;tick<120;++tick){
   refreshed.target(-100);refreshed.step(1./240);uninterrupted.step(1./240);
   check(std::abs(refreshed.pose().yaw-uninterrupted.pose().yaw)<1e-12,
    "unchanged destination restarts the body turnaround");
  }
  for(double destination:{-100.,100.,-100.}){
   front.target(destination);
   const double heading=destination>front.state().rootX?-bias:-pi+bias;
   auto previous=front.pose();
   for(int frame=0;frame<8000&&!front.settled();++frame){
    front.step(1./120);const auto p=front.pose();
    check(std::cos(pi/2+p.yaw)>=std::sin(bias)-1e-10,"resident turnaround exposes its back to the camera");
    check(p.grounded[0]||p.grounded[1],"resident yaw torque lifts both soles");
    check(std::isfinite(p.yaw),"resident support pivot produces nonfinite body yaw");
    check(std::abs(p.yaw-previous.yaw)<=8.5/120+1e-10,"resident turn exceeds its brisk angular speed limit");
    for(int side=0;side<2;++side){
     if(previous.grounded[side]&&p.grounded[side])
      check(distance(previous.contacts[side],p.contacts[side])<1e-10,"resident torque slides a planted sole");
     auto ankle=rotate(p.legs[side].ankle,p.yaw);
     auto offset=rotate(pitch({legs[side].contact.x-legs[side].ankle.x,legs[side].contact.y-legs[side].ankle.y,legs[side].contact.z-legs[side].ankle.z},p.footPitch[side]),p.footYaw[side]);
     check(distance({ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+p.rootX/100},p.contacts[side])<1e-7,
      "resident mirrored heading loses its actual sole pivot");
    }
    previous=p;
   }
   check(front.settled()&&std::abs(front.pose().yaw-heading)<1e-8,"resident mirrored heading never settles");
  }
  // Finish turning while braking; body heading is independent of travel intent.
  front.target(100);for(int tick=0;tick<60;++tick)front.step(1./120);front.target(front.state().rootX);
  for(int tick=0;tick<8000&&!front.settled();++tick)front.step(1./120);
  check(front.settled()&&std::abs(front.pose().yaw+bias)<1e-8,"stop interrupts resident yaw settlement");
 }
 // Attention turns use the same grounded yaw solver without becoming travel.
 {
  constexpr double pi=3.141592653589793,bias=.6108652381980153;
  for(bool resident:{false,true}){
   Motion attentive(legs);attentive.setFacingBias(-bias);attentive.setCameraFacingTurns(resident);
   attentive.reset(25,{-120,120});
   for(double point:{-80.,90.,-80.}){
    const auto beforeRequest=attentive.pose();const auto state=attentive.state();
    check(attentive.face(point),"attention does not begin a changed heading");
    check(attentive.state().rootX==state.rootX&&attentive.state().targetX==state.targetX,
     "attention changes root placement or navigation target");
    check(attentive.pose().yaw==beforeRequest.yaw,"attention snaps the presented body heading");
    const double heading=point<state.rootX?(resident?-pi+bias:pi-bias):-bias;
    bool replanted=false;auto previous=attentive.pose();
    for(int frame=0;frame<8000&&!attentive.settled();++frame){
     check(!attentive.face(point),"repeated attention refresh reports a changed heading");
     attentive.step(1./120);const auto p=attentive.pose();
     check(std::abs(attentive.state().targetX-p.rootX)<.02&&std::abs(attentive.state().velocity)<1e-8,
      "attention pivot starts navigation instead of following its planted arc");
     check(p.grounded[0]||p.grounded[1],"attention turn lifts both support soles");
     check(!(p.swing[0]&&p.swing[1]),"attention turn admits two paw flights");
     replanted=replanted||p.swing[0]||p.swing[1];
     for(int side=0;side<2;++side)if(previous.grounded[side]&&p.grounded[side])
      check(distance(previous.contacts[side],p.contacts[side])<1e-10,"attention slides a planted sole");
     previous=p;
    }
    check(replanted&&attentive.settled()&&std::abs(attentive.pose().yaw-heading)<1e-8,
     "attention does not settle its grounded heading");
    check(!attentive.face(point),"settled attention refresh wakes the turn");
   }
   const auto quiet=attentive.pose();
   for(double point:{quiet.rootX,quiet.rootX+.01,quiet.rootX-.01,double(NAN),double(INFINITY),-double(INFINITY)})
    check(!attentive.face(point),"attention accepts an invalid or home-radius point");
   check(attentive.settled()&&attentive.pose().yaw==quiet.yaw,"rejected attention changes motion");
   attentive.reset(0,{-120,120});attentive.target(80);const double travel=attentive.state().targetX;
   check(attentive.face(-80)&&attentive.state().targetX==travel,
    "attention replaces an existing navigation destination");
  }
 }
 // A pickup freezes an incomplete turn at its current angle. Once grounded,
 // the same attention point must restore its authored heading.
 {
  constexpr double pi=3.141592653589793,bias=.6108652381980153;
  Motion interrupted(legs);interrupted.setFacingBias(-bias);interrupted.setCameraFacingTurns(true);
  interrupted.reset(25,{-120,120});check(interrupted.face(-80),"interrupted attention setup does not turn");
  for(int tick=0;tick<10;++tick)interrupted.step(1./120);
  check(std::abs(interrupted.pose().yaw-(-pi+bias))>.1,"attention setup already finished turning");
  interrupted.pickup(25,30);interrupted.hold(25,30);interrupted.release({0,0});
  for(int tick=0;tick<6000&&!interrupted.settled();++tick)interrupted.step(1./120);
  check(interrupted.settled()&&interrupted.state().phase==pets::character::DropPhase::Grounded,
   "interrupted attention does not recover from its drop");
  check(interrupted.face(-80),"same attention side does not restore heading after pickup");
  for(int tick=0;tick<8000&&!interrupted.settled();++tick)interrupted.step(1./120);
  check(interrupted.settled()&&std::abs(interrupted.pose().yaw-(-pi+bias))<1e-8&&
        std::abs(interrupted.state().rootX-interrupted.state().targetX)<1e-8,
   "restored attention does not settle its planted-arc heading");
  check(!interrupted.face(-80),"restored attention repeatedly wakes its settled heading");
 }
 // The desktop host owns realized root movement. Intent must not advance the
 // rig before host feedback, and grounded soles stay pinned through quantization.
 {
  Motion hosted(legs);hosted.reset(215, {200,260});hosted.setSpeedMultiplier(2);hosted.target(1000);
  check(hosted.state().rootX==215&&hosted.state().targetX==260,"host reset/span uses preview bounds");
  double carry=0;bool walked=false;
  for(int frame=0;frame<2400&&!hosted.settled();++frame) {
   const auto before=hosted.pose();const auto proposal=hosted.propose(1./120);
   const auto pending=hosted.pose();
   check(pending.rootX==before.rootX,"proposal moves the presented root before feedback");
   for(int side=0;side<2;++side)check(distance(pending.contacts[side],before.contacts[side])<1e-12,
    "proposal moves a presented sole before feedback");
   carry+=proposal.displacement;
   const double increment=std::round(carry*4)/4;carry-=increment;
   const double realized=std::clamp(before.rootX+increment,200.,260.);
   hosted.realize(realized,realized==260);
   const auto after=hosted.pose();walked|=after.swing[0]||after.swing[1];
   check(after.rootX==realized,"3D root ignores host quantization");
   check(after.grounded[0]||after.grounded[1],"host realization lifts both soles");
   for(int side=0;side<2;++side)if(before.grounded[side]&&after.grounded[side])
    check(distance(before.contacts[side],after.contacts[side])<1e-10,"host realization slides a planted sole");
   hosted.realize(realized+1);
   check(hosted.pose().rootX==after.rootX,"host realization consumes the interval twice");
  }
  check(walked&&hosted.settled()&&std::abs(hosted.state().rootX-260)<.75,
   "quantized host travel fails to settle near the edge");
  hosted.target(200);hosted.propose(1./60);hosted.realize(259.75,true);
  check(hosted.state().targetX==259.75,"blocked host feedback retains unreachable travel intent");
  for(int tick=0;tick<4000&&!hosted.settled();++tick)hosted.step(1./120);
  check(hosted.settled(),"blocked 3D travel never becomes quiet");
  hosted.reset(-350,{-400,-300});
  check(hosted.settled()&&hosted.state().rootX==-350,"drag rebase retains old dynamics");
  const auto rebased=hosted.pose();
  for(int side=0;side<2;++side)check(rebased.grounded[side]&&std::abs(rebased.contacts[side].z+3.5-legs[side].hip.z)<1e-12,
   "drag rebase leaves contacts in the previous desktop position");
  hosted.setSpan({-340,-320});
  check(hosted.settled()&&hosted.state().rootX==-340,"screen span update fails to rebase to visible bounds");
  hosted.target(-320);const auto oldState=hosted.state();hosted.propose(1./60);
  check(hosted.state().velocity==oldState.velocity&&hosted.state().verticalVelocity==oldState.verticalVelocity,
   "proposal changes presented navigation/support velocity");
  hosted.target(-330);hosted.realize(-335);
  check(hosted.state().rootX==-340&&hosted.state().targetX==-330,"target replacement realizes stale intent");
  hosted.propose(1./60);hosted.reset(-325,{-340,-320});hosted.realize(-324);
  check(hosted.state().rootX==-325&&hosted.settled(),"rebase realizes a pre-drag interval");
  hosted.target(-320);hosted.propose(1./60);hosted.setSpeedMultiplier(.5);hosted.realize(-324);
  check(hosted.state().rootX==-325&&hosted.speedMultiplier()==.5,"speed change restores stale planner configuration");
  hosted.propose(1./60);hosted.setFootPlantDuration(.02);hosted.realize(-324);
  check(hosted.state().rootX==-325,"plant timing change realizes stale intent");
  hosted.propose(1./60);hosted.impulse();const auto impulseState=hosted.state();hosted.realize(-324);
  check(hosted.state().rootX==-325&&hosted.state().verticalVelocity==impulseState.verticalVelocity,
   "host feedback discards a new impulse");
  hosted.reset(-340,{-340,-320});
  bool invalidSpan=false;try{hosted.reset(0,{1,-1});}catch(const std::exception&){invalidSpan=true;}
  check(invalidSpan&&hosted.state().rootX==-340,"invalid host span corrupts current motion");
 }
 // Body intent remains independent of the support phase; starts and braking
 // ease lateral acceleration while leg compression drives bounded vertical bob.
 std::array<double,4> travel{};int settingIndex=0;
 for(double setting : {.5,1.,2.,3.}) {
  Motion following(legs);following.setSpeedMultiplier(setting);following.target(120);
  double minHip=following.pose().legs[0].hip.y,maxHip=minHip;const double restHip=minHip;
  double oldAcceleration=0;
  for(int tick=0;tick<120;++tick) {
   const double dt=1./120;
   const auto before=following.state();following.step(dt);const auto current=following.pose();
   const auto after=following.state();
   check(current.rootX>=before.rootX-1e-8,"forward body travel reverses with a support transfer");
   // This fixture has about .853 m legs: its natural pace is below 73 cm/s.
   check(std::abs(after.velocity)<=73*setting+1e-6,"body exceeds selected anatomy-derived travel speed");
   const double acceleration=(after.velocity-before.velocity)/dt;
   check(std::abs(acceleration)<=240.001,"body acceleration exceeds its bound");
   if(tick==0)check(acceleration<80,"body starts at maximum acceleration");
   if(after.rootX<118)check(std::abs(acceleration-oldAcceleration)<80,"body acceleration jumps between frames");
   oldAcceleration=acceleration;
   minHip=std::min(minHip,current.legs[0].hip.y);maxHip=std::max(maxHip,current.legs[0].hip.y);
   for(int side=0;side<2;++side) {
    check(std::isfinite(current.legs[side].hip.y)&&current.legs[side].hip.y>.35&&current.legs[side].hip.y<legs[side].hip.y+.25,"vertical support is nonfinite or unbounded");
    check(distance(current.legs[side].hip,current.legs[side].knee)>=distance(legs[side].hip,legs[side].knee)-1e-7,"catch-up changes upper length");
   }
   if(tick==119)travel[settingIndex]=current.rootX;
  }
  check(restHip-minHip<.12,"gentle support still produces deep squats between steps");
  check(maxHip-minHip>.003,"3D pelvis does not respond to leg support");
  ++settingIndex;
 }
 check(travel[1]>travel[0]+1,"travel setting no longer increases body displacement");
 for(int i=2;i<4;++i)check(travel[i]>=travel[i-1]-.001,"faster setting reduces forward body travel");
 // The paw sole is forward of the ankle in the authored rig. Its support
 // target must nevertheless pass under the hip during travel, not only rest.
 for(double setting:{.5,1.,2.,3.})for(double direction:{1.,-1.}) {
  Motion stance(legs);stance.setSpeedMultiplier(setting);stance.target(direction*120);
  std::array<int,2> under{},ahead{},behind{};
  for(int tick=0;tick<700;++tick) {
   stance.step(1./240);const auto p=stance.pose();
   if(std::abs(stance.state().velocity)<20||std::abs(p.rootX)<10)continue;
   for(int side=0;side<2;++side) {
    const auto hip=rotate(p.legs[side].hip,p.yaw);
    const double fore=direction*(p.contacts[side].z-p.rootX/100-hip.z);
    if(std::abs(fore)<.025)++under[side];
    if(fore>.025)++ahead[side];
    if(fore<-.025)++behind[side];
   }
  }
  for(int side=0;side<2;++side)check(under[side]>0&&ahead[side]>0&&behind[side]>0,
   "walking paw never passes from ahead through beneath to behind its hip");
 }
 Motion articulatedPaw(legs);articulatedPaw.target(100);bool pitched=false;
 for(int tick=0;tick<500;++tick) {
  const auto previous=articulatedPaw.pose();articulatedPaw.step(1./240);const auto p=articulatedPaw.pose();
  for(int side=0;side<2;++side) {
   pitched|=std::abs(p.footPitch[side])>.08;
   check(std::abs(p.footPitch[side]-previous.footPitch[side])<.04,"ankle pitch jumps between frames");
   if(!p.swing[side])check(std::abs(p.footPitch[side])<1e-12,"supported paw is not flat");
  }
 }
 check(pitched,"paw does not articulate independently of its leg");
 Motion weighted(legs);weighted.setSpeedMultiplier(3);int airbornePawFrames=0;
 const double gravity=600*(distance(legs[0].knee,legs[0].hip)+distance(legs[0].ankle,legs[0].knee));
 for(double destination:{100.,-100.,100.}) {
  weighted.target(destination);
  for(int tick=0;tick<1200;++tick) {
   const auto before=weighted.pose();const auto oldState=weighted.state();
   airbornePawFrames+=!before.grounded[0]||!before.grounded[1];
   weighted.step(1./240);const auto afterState=weighted.state();
   if(before.contacts[0].y>1e-6&&before.contacts[1].y>1e-6) {
    const double expected=gravity*.5-8*std::max(0.,oldState.verticalVelocity);
    check(std::abs(afterState.verticalAcceleration-expected)<1e-7,"airborne 3D paws still transmit leg support");
    if(oldState.verticalVelocity<=0)check(afterState.verticalVelocity>oldState.verticalVelocity,"airborne body does not turn downward");
    else check(afterState.verticalVelocity>0,"air damping holds the body suspended");
   }
  }
 }
 check(airbornePawFrames>0,"actual lifted paw support was not exercised");
 Motion flexibleStance(legs);const auto flex=flexibleStance.pose();
 check(flexibleStance.settled(),"neutral 3D stance is not initially settled");
 for(int side=0;side<2;++side)check(!flex.swing[side]&&std::abs(flex.contacts[side].y)<1e-12,"neutral 3D paw is not grounded");
 for(int side=0;side<2;++side) {
  const auto l=flex.legs[side];
  Vec3 a{l.knee.x-l.hip.x,l.knee.y-l.hip.y,l.knee.z-l.hip.z};
  Vec3 b{l.ankle.x-l.knee.x,l.ankle.y-l.knee.y,l.ankle.z-l.knee.z};
  const double bend=std::acos(std::clamp((a.x*b.x+a.y*b.y+a.z*b.z)/(distance(l.hip,l.knee)*distance(l.knee,l.ankle)),-1.,1.));
  check(bend>.3,"3D neutral stance leaves the knees nearly locked");
 }
 // Repeated travel must keep initiating new steps on both sides; contact
 // recovery must not leave one foot parked while only the other animates.
 for(double setting:{.5,1.,2.,3.}) {
  Motion ongoing(legs);ongoing.setSpeedMultiplier(setting);std::array<int,2> launches{};
  for(double destination:{100.,-100.,100.}) {
   ongoing.target(destination);
   for(int tick=0;tick<3000&&!ongoing.settled();++tick) {
    const auto before=ongoing.state();ongoing.step(1./120);const auto after=ongoing.state();
    for(int side=0;side<2;++side)if(!before.feet[side].swing&&after.feet[side].swing)++launches[side];
   }
   if(!ongoing.settled()){auto s=ongoing.state();auto p=ongoing.pose();std::cerr<<"ongoing speed "<<setting<<" destination "<<destination<<" root "<<s.rootX<<" velocity "<<s.velocity<<" vertical "<<s.verticalVelocity<<"/"<<s.verticalAcceleration<<" yaw "<<p.yaw<<" feet "<<p.footYaw[0]<<"/"<<p.footYaw[1]<<" swing "<<s.anyFootSwinging()<<"\n";}
   check(ongoing.settled(),"ongoing alternating gait does not finish travel");
  }
  for(int side=0;side<2;++side)check(launches[side]>=4,"one 3D foot stops taking new steps");
 }
 // Arrival leaves a comfortable planted stance; millimetre-scale residuals
 // must not trigger extra alignment steps once body navigation has stopped.
 for(double speed:{.5,1.,2.,3.})for(double destination:{5.,9.,15.,21.,33.,60.,80.,100.,120.}) {
  Motion quietHome(legs);quietHome.setSpeedMultiplier(speed);quietHome.target(destination);int tinyReplants=0;
  for(int tick=0;tick<1500;++tick) {
   const auto before=quietHome.state();quietHome.step(1./120);const auto after=quietHome.state();
   if(std::abs(before.rootX-before.targetX)<.05&&std::abs(before.velocity)<.05)
    for(int side=0;side<2;++side)if(std::abs(before.feet[side].position.x-before.rootX)<4)
     tinyReplants+=!before.feet[side].swing&&after.feet[side].swing;
  }
  if(tinyReplants)std::cerr<<"home destination "<<destination<<" speed "<<speed<<" taps "<<tinyReplants<<"\n";
  check(tinyReplants==0,"home alignment keeps tapping paws already in a comfortable stance");
  check(quietHome.settled(),"comfortable arrival stance never becomes quiet");
  const auto quiet=quietHome.pose();quietHome.step(1./60);const auto still=quietHome.pose();
  for(int side=0;side<2;++side)check(distance(quiet.contacts[side],still.contacts[side])<1e-9,
   "comfortable settled paw keeps moving");
 }
 Motion localAdjustments(legs);localAdjustments.target(60);
 for(int tick=0;tick<3000&&!localAdjustments.settled();++tick)localAdjustments.step(1./120);
 check(localAdjustments.settled(),"local adjustment setup did not settle");
 int localLifts=0;
 for(double nudge:{1.,-1.,2.,-2.}) {
  localAdjustments.target(localAdjustments.state().rootX+nudge);
  for(int tick=0;tick<1000&&!localAdjustments.settled();++tick) {
   const auto before=localAdjustments.pose();localAdjustments.step(1./120);const auto after=localAdjustments.pose();
   for(int side=0;side<2;++side)localLifts+=before.grounded[side]&&!after.grounded[side];
   check(std::abs(after.yaw)<.01,"tiny home adjustment turns the whole body around");
  }
  check(localAdjustments.settled(),"tiny home adjustment never settles");
 }
 check(localLifts==0,"tiny home adjustments repeatedly lift the paws");
 // Repeated petting can lift the body beyond floor reach. Once it falls
 // back, recovering soles must return to contact rather than stay suspended.
 Motion recoveredSupport(legs);
 for(int tap=0;tap<60;++tap)recoveredSupport.impulse();
 bool extended=false;double minimumHip=10;
 for(int tick=0;tick<3000&&!recoveredSupport.settled();++tick) {
  recoveredSupport.step(1./240);const auto p=recoveredSupport.pose();
  check(p.grounded[0]||p.grounded[1],"body impulse leaves both paws airborne");
  extended|=distance(p.legs[0].hip,p.legs[0].knee)>distance(legs[0].hip,legs[0].knee)+.001;
  minimumHip=std::min(minimumHip,p.legs[0].hip.y);
 }
 check(extended,"pinned-support fixture did not extend its legs");
 check(recoveredSupport.settled()&&recoveredSupport.pose().grounded[0]&&recoveredSupport.pose().grounded[1],
  "raised recovery paws never regain leg support");
 check(minimumHip>.35,"leg support recovery collapses the body through the floor");
 // Hard contact policy applies to the actual rig during ordinary steps,
 // manual yaw replants and reversals, including coarse presentation intervals.
 for(double speed:{.5,1.,2.,3.})for(double dt:{1./240,1./60,1./30}) {
  Motion pinned(legs);pinned.setSpeedMultiplier(speed);
  for(double destination:{100.,-100.,100.}) {
   pinned.target(destination);
   for(int tick=0;tick<5000&&!pinned.settled();++tick) {
    const auto before=pinned.pose();pinned.step(dt);const auto after=pinned.pose();
    check(after.grounded[0]||after.grounded[1],"both actual 3D soles are airborne");
    for(int side=0;side<2;++side) {
     if(before.grounded[side]&&after.grounded[side]){
      if(distance(before.contacts[side],after.contacts[side])>=1e-9)std::cerr<<"pin speed "<<speed<<" dt "<<dt<<" tick "<<tick<<" side "<<side<<" delta "<<distance(before.contacts[side],after.contacts[side])<<" swing "<<before.swing[side]<<"/"<<after.swing[side]<<" yaw "<<before.yaw<<"/"<<after.yaw<<"\n";
      check(distance(before.contacts[side],after.contacts[side])<1e-9,"floor-level 3D sole translates before toe-off");
     }
     check(distance(after.legs[side].hip,after.legs[side].knee)<distance(legs[side].hip,legs[side].knee)*1.035,"pinned gait overextends upper leg");
     const auto ankle=rotate(after.legs[side].ankle,after.yaw);
     const auto offset=rotate(pitch({legs[side].contact.x-legs[side].ankle.x,legs[side].contact.y-legs[side].ankle.y,legs[side].contact.z-legs[side].ankle.z},after.footPitch[side]),after.footYaw[side]);
     const Vec3 contact{ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+after.rootX/100};
     check(distance(contact,after.contacts[side])<1e-7,"elastic ankle rotation loses its sole pivot");
    }
   }
   check(pinned.settled(),"pinned elastic gait never settles after reversal");
  }

 }
 // A small body adjustment can remain supported by the comfortable existing
 // stance. Settling must not manufacture toe taps solely to hit authored homes.
 for(double target:{6.,-6.}) {
  Motion comfortable(legs);comfortable.reset(0,{-120,120});
  // The negative case checks a nearby adjustment without requesting a reversal.
  if(target<0)comfortable.face(-100);
  for(int tick=0;tick<8000&&!comfortable.settled();++tick)comfortable.step(1./120);
  const auto before=comfortable.pose();const double destination=before.rootX+target;comfortable.target(destination);
  for(int tick=0;tick<2400&&!comfortable.settled();++tick) {
   comfortable.step(1./120);const auto p=comfortable.pose();
   check(!p.swing[0]&&!p.swing[1],"comfortable 3D arrival unnecessarily replants a sole");
   for(int side=0;side<2;++side)
    check(distance(before.contacts[side],p.contacts[side])<1e-10,
     "comfortable 3D arrival slides an existing sole");
  }
  check(comfortable.settled()&&std::abs(comfortable.state().rootX-destination)<.75,
   "comfortable 3D arrival fails to settle near its destination");
 }
 // Host quantization must also release the spatial adapter's scheduling gate
 // when the requested destination falls between realizable desktop positions.
 for(double destination:{45.37,-45.37}) {
  Motion fractional(legs);fractional.setFacingBias(-.6108652381980153);
  fractional.setCameraFacingTurns(true);fractional.reset(0,{-120,120});
  fractional.target(destination);double carry=0;
  for(int tick=0;tick<4000&&!fractional.settled();++tick) {
   const auto before=fractional.pose();const auto proposal=fractional.propose(1./120);
   carry+=proposal.displacement;
   const double increment=std::round(carry);carry-=increment;
   fractional.realize(before.rootX+increment);
   const auto p=fractional.pose();
   check(!(p.swing[0]&&p.swing[1]),"fractional 3D arrival lifts both soles");
   for(int side=0;side<2;++side)if(before.grounded[side]&&p.grounded[side])
    check(distance(before.contacts[side],p.contacts[side])<1e-10,
     "fractional 3D arrival slides a planted sole");
  }
  check(fractional.settled()&&std::abs(fractional.state().rootX-destination)<=.75,
   "fractional 3D arrival retains a motion deadline");
 }
 Motion gentleTurn(legs);gentleTurn.face(-100);
 const auto restTurn=gentleTurn.pose();Motion fineTurn=gentleTurn;
 gentleTurn.step(1./3000);fineTurn.step(1./6000);
 for(int side=0;side<2;++side) {
  const double coarse=distance(gentleTurn.pose().contacts[side],restTurn.contacts[side]);
  const double fine=distance(fineTurn.pose().contacts[side],restTurn.contacts[side]);
  check(coarse<.001&&fine<=coarse*.6+1e-10,
   "turn toe-off has a timestep-independent contact discontinuity");
  check(fineTurn.pose().contacts[side].y>=-1e-9,"turn toe-off penetrates the floor");
 }
 Motion continuousRecovery(legs);continuousRecovery.setSpeedMultiplier(3);continuousRecovery.target(-120);
 for(int tick=0;tick<6000;++tick) {
  const auto before=continuousRecovery.pose();continuousRecovery.step(1./2000);const auto after=continuousRecovery.pose();
  for(int side=0;side<2;++side)check(distance(before.contacts[side],after.contacts[side])<.005,
   "reach recovery pops the foot between neighboring frames");
 }
 Motion turningSpeed(legs);
 turningSpeed.setSpeedMultiplier(.5);turningSpeed.target(-80);
 for(int tick=0;tick<4;++tick)turningSpeed.step(1./60);
 auto turnBefore=turningSpeed.pose();turningSpeed.setSpeedMultiplier(3);turningSpeed.step(1./600);
 auto turnAfter=turningSpeed.pose();
 for(int side=0;side<2;++side)check(std::abs(turnBefore.footYaw[side]-turnAfter.footYaw[side])<.04,"speed adjustment jumps turn phase");
 Motion fastSpeed(legs);fastSpeed.setSpeedMultiplier(3);fastSpeed.target(120);
 for(int tick=0;tick<60;++tick)fastSpeed.step(1./60);
 auto beforeSpeedChange=fastSpeed.pose();
 fastSpeed.setSpeedMultiplier(.5);
 auto afterSpeedChange=fastSpeed.pose();
 check(beforeSpeedChange.rootX==afterSpeedChange.rootX,"speed adjustment snaps root");
 for(int side=0;side<2;++side)check(distance(beforeSpeedChange.contacts[side],afterSpeedChange.contacts[side])<1e-12,"speed adjustment snaps contact");
 check(fastSpeed.state().targetX==120,"speed adjustment replaces target");
 for(int tick=0;tick<3000&&!fastSpeed.settled();++tick)fastSpeed.step(1./60);
 check(fastSpeed.settled(),"slow speed never settles at target");
 fastSpeed.setSpeedMultiplier(2);
 check(fastSpeed.settled(),"speed adjustment wakes idle motion");
 fastSpeed.target(-80);
 for(int tick=0;tick<2500&&!fastSpeed.settled();++tick){
  auto before=fastSpeed.pose();fastSpeed.step(1./60);auto after=fastSpeed.pose();
  for(int side=0;side<2;++side)if(!before.swing[side]&&!after.swing[side])check(distance(before.contacts[side],after.contacts[side])<1e-10,"faster turn drifts planted foot");
 }
 check(fastSpeed.settled(),"faster reversal never settles");
 // Lean and follow-through are model-local: after turning around, moving
 // toward negative world X must still incline the torso toward its front.
 Motion backward(legs);
 backward.target(-120);
 bool balancedTravel=false;
 for(int tick=0;tick<1200 && !backward.settled();++tick) {
  backward.step(1./60);
  const auto p=backward.pose();
  const double localVelocity=backward.state().velocity*std::cos(p.yaw);
  if(std::abs(localVelocity)>10 && std::abs(p.lean)>.005) {
   check(p.lean*localVelocity>0,"turning torso leans against body travel");
   balancedTravel=true;
  }
 }
 check(balancedTravel && backward.settled(),"backward body-led travel did not settle");
 Motion braking(legs);braking.target(100);
 for(int tick=0;tick<40;++tick)braking.step(1./120);
 auto beforeReverse=braking.state();braking.target(-100);braking.step(1./120);
 auto afterReverse=braking.state();
 check(afterReverse.rootX>beforeReverse.rootX,"turn forcibly stops translating body");
 check(std::abs(afterReverse.velocity-beforeReverse.velocity)<=2.0001,"turn ignores navigation acceleration");
 Motion motion(legs, std::array<Arm,2>{arm,arm});
 motion.armGoal(0, {.6,1.3,.3});
 auto posed=motion.pose();
 check(posed.arms[0].has_value(),"arm goal absent");
 Motion plain(legs);
 for(int j=0;j<2;++j) check(distance(posed.legs[j].ankle,plain.pose().legs[j].ankle)<1e-12,"arm goal overrides legs");
 motion.impulse();motion.target(45);
 for(int tick=0;tick<180;++tick) {
  motion.step(1./60);auto bodyPose=motion.pose();
  const auto& held=*bodyPose.arms[0];
  check(std::abs(distance(held.shoulder,held.elbow)-distance(arm.shoulder,arm.elbow))<1e-7,"held arm upper length changes under bob");
  check(std::abs(distance(held.elbow,held.hand)-distance(arm.elbow,arm.hand))<1e-7,"held arm lower length changes under bob");
 }
 const auto beforeRelease=motion.pose().arms[0]->grip;
 motion.clearArmGoal(0);
 check(distance(motion.pose().arms[0]->grip,beforeRelease)<1e-10,"release snaps the grip instead of returning procedurally");
 motion.target(80);
 bool swung=false;
 for(int i=0;i<2000&&!motion.settled();++i){auto before=motion.state();motion.step(1./60);auto after=motion.state();auto p=motion.pose();
  for(int j=0;j<2;++j){swung|=after.feet[j].swing;
   if(!before.feet[j].swing&&!after.feet[j].swing)check(before.feet[j].position.x==after.feet[j].position.x,"planted anchor drift");
   check(distance(p.legs[j].hip,p.legs[j].knee)>=distance(legs[j].hip,legs[j].knee)-1e-7,"moving upper changed");
   check(distance(p.legs[j].knee,p.legs[j].ankle)>=distance(legs[j].knee,legs[j].ankle)-1e-7,"moving lower changed");
  }
 }
 check(swung,"target movement did not step");check(motion.settled(),"movement never settles");
 motion.target(-60);for(int i=0;i<2500&&!motion.settled();++i)motion.step(1./60);
 check(motion.settled()&&std::abs(motion.state().rootX+60)<.02,"reversal never arrives");
 auto reversed=motion.pose();
 check(reversed.legs[0].ankle.x-reversed.bodyShift.x>.12&&reversed.legs[1].ankle.x-reversed.bodyShift.x<-.12,"reversed feet cross the opposite legs");
 // Check the complete turn, not just its final frame: each supported foot
 // remains fixed, lifted feet clear the floor, and both leg chains stay apart.
 for(double destination:{60.,-60.,60.}){
  motion.target(destination);bool replant=false;
  for(int tick=0;tick<2500&&!motion.settled();++tick){
   auto before=motion.pose();motion.step(1./60);auto p=motion.pose();
   for(int side=0;side<2;++side){
    if(!before.swing[side]&&!p.swing[side]){
     check(distance(before.contacts[side],p.contacts[side])<1e-10,"turning planted world anchor drifts");
     // A supported sole may pivot in yaw about its unchanged world contact.
     check(std::isfinite(p.footYaw[side]),"supported pivot produces nonfinite foot yaw");
    }
    replant|=p.swing[side]&&std::abs(p.footYaw[side]-before.footYaw[side])>1e-5;
    check(p.contacts[side].y>=-1e-9,"turn foot penetrates ground");
    auto ankle=rotate(p.legs[side].ankle,p.yaw);
    auto offset=rotate(pitch({legs[side].contact.x-legs[side].ankle.x,legs[side].contact.y-legs[side].ankle.y,legs[side].contact.z-legs[side].ankle.z},p.footPitch[side]),p.footYaw[side]);
    Vec3 contact{ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+p.rootX/100};
    check(distance(contact,p.contacts[side])<1e-7,"world foot orientation disagrees with IK contact");
    double sign=side==0?1:-1;
    check(std::isfinite(p.legs[side].ankle.x),"turning ankle is nonfinite");
    check(std::isfinite(p.legs[side].knee.x),"turning knee is nonfinite");
    check(distance(p.legs[side].hip,p.legs[side].knee)>=distance(legs[side].hip,legs[side].knee)-1e-7,"turn upper length changes");
    check(distance(p.legs[side].knee,p.legs[side].ankle)>=distance(legs[side].knee,legs[side].ankle)-1e-7,"turn lower length changes");
   }
  }
  check(replant,"reversal does not replant feet");
  check(motion.settled(),"replant turn never settles");
  auto p=motion.pose();motion.step(1./60);auto quiet=motion.pose();
  check(distance(p.contacts[0],quiet.contacts[0])<1e-12&&p.yaw==quiet.yaw,"settled turn changes without input");
 }
 motion.impulse();check(!motion.settled(),"impulse did not wake followers");for(int i=0;i<1500&&!motion.settled();++i)motion.step(1./60);check(motion.settled(),"impulse followers never settle");
 // Transfer timing must preserve support without needing elastic leg strain.
 for(double speed:{1.,2.,3.}){
  auto peakStretch=[&](double plant){
   Motion trial(legs);trial.setSpeedMultiplier(speed);trial.setFootPlantDuration(plant);
   double peak=0;auto previous=trial.pose();
   for(double target:{100.,-100.,100.}){
    trial.target(target);
    for(int f=0;f<10000&&!trial.settled();++f){
     trial.step(1./120);auto p=trial.pose();
     check(p.grounded[0]||p.grounded[1],"short transfer lifts both feet");
     for(int side=0;side<2;++side){
      if(previous.grounded[side]&&p.grounded[side])
       check(distance(previous.contacts[side],p.contacts[side])<1e-10,"short transfer slides a planted sole");
      peak=std::max(peak,distance(p.legs[side].hip,p.legs[side].knee)/distance(legs[side].hip,legs[side].knee)-1);
      peak=std::max(peak,distance(p.legs[side].knee,p.legs[side].ankle)/distance(legs[side].knee,legs[side].ankle)-1);
     }
     previous=p;
    }
    check(trial.settled(),"short transfer never settles after reversal");
   }
   return peak;
  };
  check(peakStretch(.01)<.035&&peakStretch(.04)<.035,"planted transfer exceeds measured leg reach");
 }
 // Stationary reversals finish in a finite number of useful placements.
 // Support yaw may pivot; the actual sole contact must remain fixed.
 for(double speed:{.5,1.,2.,3.})for(double bias:{0.,.6108652381980153,.94}) {
  constexpr double pi=3.141592653589793,dt=1./240;
  Motion turning(legs);turning.setSpeedMultiplier(speed);
  turning.setFacingBias(-bias);turning.setCameraFacingTurns(bias>0);
  turning.reset(0,{-120,120});
  for(double target:{-100.,100.,-100.}) {
   const double heading=target<0?(bias>0?-pi+bias:pi):-bias;
   check(turning.face(target),"stationary turn request is rejected");
   auto previous=turning.pose();int lifts=0;bool steered=false;
   const double startYaw=previous.yaw,direction=heading-startYaw;
   for(int frame=0;frame<960&&!turning.settled();++frame) {
    turning.step(dt);const auto p=turning.pose();
    check(std::abs(turning.state().velocity)<1e-8&&std::abs(turning.state().targetX-p.rootX)<.02,
     "stationary pivot starts navigation away from its body arc");
    check(p.grounded[0]||p.grounded[1],"finite turn loses all sole support");
    check(std::abs(p.yaw-previous.yaw)<=8.5*dt+1e-10,
     "finite turn exceeds the body angular speed limit");
    check((p.yaw-previous.yaw)*direction>=-1e-10,
     "finite turn reverses body heading before completion");
    for(int side=0;side<2;++side) {
     if(p.swing[side]&&!previous.swing[side])++lifts;
     if(p.swing[side]&&std::abs(p.footYaw[side]-previous.footYaw[side])>1e-7)steered=true;
     if(previous.grounded[side]&&p.grounded[side]) {
      check(distance(previous.contacts[side],p.contacts[side])<1e-10,
       "finite support pivot translates a grounded sole");
      check(std::abs(p.footYaw[side]-previous.footYaw[side])<1e-9,
       "foot-led turn twists an already loaded sole");
     }
     const auto ankle=rotate(p.legs[side].ankle,p.yaw);
     const auto offset=rotate(pitch({legs[side].contact.x-legs[side].ankle.x,legs[side].contact.y-legs[side].ankle.y,legs[side].contact.z-legs[side].ankle.z},p.footPitch[side]),p.footYaw[side]);
     check(distance({ankle.x+offset.x,ankle.y+offset.y,ankle.z+offset.z+p.rootX/100},p.contacts[side])<1e-7,
      "finite turn loses the reconstructed sole contact");
     check(p.contacts[side].y>=-1e-9,"finite turn penetrates the floor");
     check(distance(p.legs[side].hip,p.legs[side].knee)<=distance(legs[side].hip,legs[side].knee)*1.035,
      "finite turn overextends the upper leg");
     check(distance(p.legs[side].knee,p.legs[side].ankle)<=distance(legs[side].knee,legs[side].ankle)*1.035,
      "finite turn overextends the lower leg");
    }
    previous=p;
   }
   check(turning.settled()&&std::abs(turning.pose().yaw-heading)<1e-8,
    "finite turn does not complete its requested heading");
   check(lifts==2,"stationary turnaround exceeds its planned placement count");
   check(steered,"finite turn never steers a lifted foot");
  }
 }
 // A supported knee follows its paw's bend plane while the torso turns.
 {
  Motion turning(legs);turning.target(-100);bool checked=false;
  auto subtract=[](Vec3 a,Vec3 b){return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};};
  auto scalar=[](Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
  auto reject=[&](Vec3 v,Vec3 axis){double k=scalar(v,axis)/scalar(axis,axis);return Vec3{v.x-axis.x*k,v.y-axis.y*k,v.z-axis.z*k};};
  auto previous=turning.pose();
  for(int frame=0;frame<600;++frame){
   turning.step(1./120);auto p=turning.pose();
   for(int side=0;side<2;++side)if(previous.grounded[side]&&p.grounded[side]&&std::abs(p.yaw-previous.yaw)>1e-7){
    check(distance(previous.contacts[side],p.contacts[side])<1e-10,"supported knee pivot moves its sole anchor");
    auto axis=rotate(subtract(p.legs[side].ankle,p.legs[side].hip),p.yaw);
    auto actual=reject(rotate(subtract(p.legs[side].knee,p.legs[side].hip),p.yaw),axis);
    auto expected=reject(rotate({side==0?.25:-.25,0,1},p.footYaw[side]),axis);
    double cosine=scalar(actual,expected)/std::sqrt(scalar(actual,actual)*scalar(expected,expected));
    check(cosine>.99999,"supported knee twists away from its planted paw");checked=true;
   }
   previous=p;
  }
  check(checked,"reversal never exercises the supported knee pivot bend plane");
 }
 // Crossing rigid reach must not snap a straight knee into a deep bend.
 {
  auto previous=solveLeg({0,0,0},{0,-.84,0},.45,.4,{0,0,1},true);
  for(int i=1;i<=2000;++i){
   Vec3 target{0,-(.84+i*.00001),0};
   auto current=solveLeg({0,0,0},target,.45,.4,{0,0,1},true);
   check(distance(current.ankle,target)<1e-10,"elastic knee easing moves its pinned target");
   check(distance(current.knee,previous.knee)<.0003,"knee snaps across full extension");
   previous=current;
  }
 }
 std::cout<<"3D IK lengths, planted anchors, arrival and reversal passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
