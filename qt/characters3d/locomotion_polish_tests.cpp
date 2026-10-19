#include "pose.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character3d;
using pets::character::Gait;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
double distance(Vec3 a,Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
LimbArray<Leg> anatomy(bool quad){
 if(!quad)return {{{.18,1.04,0},{.22,.60,.08},{.22,.25,-.12},{.22,0,.105}},
                  {{-.18,1.04,0},{-.22,.60,.08},{-.22,.25,-.12},{-.22,0,.105}}};
 // Biscuit's authored short legs, in model motion axes.
 LimbArray<Leg> legs;legs.setCount(4);
 for(int i=0;i<4;++i){bool fore=i<2;double side=i%2==0?-.37:.37,hip=fore?.65:-.72,height=fore?.69:.70;
  legs[i]={{side,height,hip},{side,height*.53,hip+(fore?-.09:.15)},
           {side,.1615,hip-.025},{side,0,hip+.085}};
 }
 return legs;
}
// Fixed-rate travel must scale with anatomy, while the user's pace remains relative.
double naturalPace(const LimbArray<Leg>& legs){
 double reach=1e9;
 for(const auto& leg:legs)reach=std::min(reach,distance(leg.hip,leg.knee)+distance(leg.knee,leg.ankle));
 return reach*85;
}
void anatomySetsPace(){
 for(bool quad:{false,true})for(bool wings:{false,true})for(double multiplier:{.5,1.,2.,3.}){
  if(quad&&wings)continue;
  double referenceSpeed=0;int referenceSteps=0;
  for(double scale:{1.,.6,1.4}){
   auto legs=anatomy(quad);
   for(auto& leg:legs)for(auto* p:{&leg.hip,&leg.knee,&leg.ankle,&leg.contact}){
    p->x*=scale;p->y*=scale;p->z*=scale;
   }
   Motion motion(legs,{},wings,0,quad?Gait::QuadrupedWalk:Gait::Biped);
   motion.setSpeedMultiplier(multiplier);motion.reset(0,{-10000,10000});motion.target(9000);
   for(int tick=0;tick<960;++tick)motion.step(1./120);
   int steps=0;auto previous=motion.pose();const double start=motion.state().rootX;
   for(int tick=0;tick<960;++tick){
    motion.step(1./120);const auto p=motion.pose();
    for(std::size_t i=0;i<legs.size();++i){
     steps+=!previous.swing[i]&&p.swing[i];
     if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,
       "anatomy-paced travel slides a planted foot");
    }
    previous=p;
   }
   const double speed=(motion.state().rootX-start)/8;
   if(scale==1.){referenceSpeed=speed;referenceSteps=steps;}
   check(std::abs(speed/referenceSpeed-scale)<.01,"longer legs do not proportionally increase travel speed");
   check(std::abs(steps-referenceSteps)<=2,"leg length changes the shared stepping rhythm");
   // Each rig's half-speed and triple-speed settings still multiply its natural pace.
   Motion normal(legs,{},wings,0,quad?Gait::QuadrupedWalk:Gait::Biped);
   normal.reset(0,{-10000,10000});normal.target(9000);
   for(int tick=0;tick<960;++tick)normal.step(1./120);
   check(std::abs(motion.state().velocity/normal.state().velocity-multiplier)<.01,
         "speed control no longer multiplies the character's natural pace");
  }
 }
}
void walkReach(){
 bool bounded=true,readable=true;
 for(bool quad:{false,true})for(double speed:{.5,1.,2.,3.}){
  auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-.3);motion.setCameraFacingTurns(true);motion.setSpeedMultiplier(speed);motion.reset(0,{-200,200});
  double maximum=1,bobLow=10,bobHigh=-10;int moving=0;auto previous=motion.pose();
  double worstYaw=0,worstHip=0,worstHorizontal=0;
  for(double target:{80.,-50.}){
   motion.target(target);
   for(int tick=0;tick<12000&&!motion.settled();++tick){
    motion.step(1./120);auto p=motion.pose();
    for(std::size_t i=0;i<legs.size();++i){
     const double oldMaximum=maximum;
     maximum=std::max({maximum,distance(p.legs[i].hip,p.legs[i].knee)/distance(legs[i].hip,legs[i].knee),
                              distance(p.legs[i].knee,p.legs[i].ankle)/distance(legs[i].knee,legs[i].ankle)});
     if(maximum>oldMaximum){worstYaw=p.yaw;worstHip=p.legs[i].hip.y;worstHorizontal=std::hypot(p.legs[i].hip.x-p.legs[i].ankle.x,p.legs[i].hip.z-p.legs[i].ankle.z);}
     if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,"polished walk slides a planted foot");
    }
    if(std::abs(motion.state().velocity)>3&&tick>120){bobLow=std::min(bobLow,p.legs[0].hip.y);bobHigh=std::max(bobHigh,p.legs[0].hip.y);++moving;}
    previous=p;
   }
   check(motion.settled()&&std::abs(motion.state().rootX-target)<.03,"polished walk fails to arrive and sleep");
  }
  std::cout<<(quad?"Biscuit":"biped")<<" speed "<<speed<<" max extension "<<maximum<<" bob "<<bobHigh-bobLow<<" worst yaw/hip/horiz "<<worstYaw<<"/"<<worstHip<<"/"<<worstHorizontal<<"\n";
  bounded&=maximum<1.035;
  readable&=moving>20&&bobHigh-bobLow>.008;
 }
 check(bounded,"ordinary walking stretches measured leg links by more than 3.5 percent");
 check(readable,"walking has no readable body weight shift");
}
void realizedTravel(){
 for(bool quad:{false,true})for(double speed:{.5,3.}){
  auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-.3);motion.setCameraFacingTurns(true);motion.setSpeedMultiplier(speed);motion.reset(0,{-200,200});
  motion.target(150);
  for(int tick=0;tick<240;++tick)motion.step(1./120);
  check(motion.state().velocity>1,"mid-walk reversal requires established forward travel");
  motion.target(-150);double prematureReverse=0;
  for(int tick=0;tick<480;++tick){
   const double before=motion.state().rootX;motion.step(1./120);
   if(std::cos(motion.pose().yaw)>.2&&std::abs(motion.state().velocity)>8)
    prematureReverse+=std::max(0.,before-motion.state().rootX);
  }
  check(prematureReverse<1.,"moving companion reverses travel before turning its body");
  motion.reset(0,{-200,200});motion.target(80);
  double fraction=0;
  auto realizePixelStep=[&](bool clip){
   const auto before=motion.pose();const auto state=motion.state();
   const auto proposal=motion.propose(1./120);
   const auto repeated=motion.propose(1./120);
   check(std::abs(repeated.displacement-proposal.displacement)<1e-12,
         "repeated proposal advances navigation twice");
   check(motion.state().rootX==state.rootX&&motion.pose().yaw==before.yaw,
         "proposal changes presented root or body heading");
   // The native 80 px/m presentation realizes logical integer pixels and
   // carries fractional placement until it accumulates a whole pixel.
   fraction+=proposal.displacement*.8;
   const double pixels=std::round(fraction);fraction-=pixels;
   const double wanted=state.rootX+pixels/.8;
   const bool blocked=clip&&wanted>100.;
   motion.realize(blocked?100.:wanted,blocked);
   if(blocked)fraction=0;
   const auto after=motion.pose();
   for(std::size_t i=0;i<legs.size();++i)
    if(before.grounded[i]&&after.grounded[i])
     check(distance(before.contacts[i],after.contacts[i])<1e-9,
           "quantized or clipped host feedback moves a planted sole");
  };
  for(int tick=0;tick<15000&&!motion.settled();++tick)realizePixelStep(false);
  check(motion.settled()&&std::abs(motion.state().rootX-80)<.75,
        "integer host placement cannot arrive and stop scheduling");
  motion.target(160);
  for(int tick=0;tick<15000&&!motion.settled();++tick)realizePixelStep(true);
  check(motion.settled()&&std::abs(motion.state().rootX-100)<1e-9
        &&motion.state().targetX==motion.state().rootX,
        "blocked host feedback fails to cancel unreachable travel and settle");
 }
}
void turnBeforeTravel(){
 for(bool quad:{false,true}){
  auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-.3);motion.setCameraFacingTurns(true);motion.reset(0,{-200,200});motion.target(-80);
  double backward=0,turnedAt=10;
  for(int tick=0;tick<360;++tick){const auto before=motion.state();motion.step(1./120);const auto p=motion.pose();
   // A planted pivot moves the root through an arc; only navigation travel
   // must wait until the body faces its destination.
   if(std::cos(p.yaw)>.2&&std::abs(motion.state().velocity)>8)backward+=std::abs(motion.state().rootX-before.rootX);
   if(turnedAt==10&&std::abs(p.yaw-(-3.141592653589793+.3))<.15)turnedAt=(tick+1)/120.;
  }
  std::cout<<(quad?"quad":"biped")<<" wrong-way travel "<<backward<<" turn seconds "<<turnedAt<<"\n";
  check(backward<1.,"companion walks backward before completing its reversal");
  check(turnedAt<(quad?2.:1.5),"stepping turnaround is too slow");
 }
}
}
void naturalPaceAndFiniteTurns(){
 for(bool quad:{false,true}){
  const auto legs=anatomy(quad);
  Motion pace(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  pace.setFacingBias(-.6108652381980153);pace.setCameraFacingTurns(true);pace.setSpeedMultiplier(2);pace.reset(0,{-2000,2000});pace.target(1800);
  for(int tick=0;tick<480;++tick)pace.step(1./120);
  std::cout<<(quad?"quad":"biped")<<" natural default speed "<<pace.state().velocity<<"\n";
  check(std::abs(pace.state().velocity-naturalPace(legs)*2)<1.,"default control does not multiply anatomy-derived pace");
  Motion turn(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  turn.setFacingBias(-.6108652381980153);turn.setCameraFacingTurns(true);turn.reset(0,{-200,200});turn.face(-100);
  int placements=0,beats=0;double lastLaunch=-10;auto previous=turn.pose();const auto standing=previous;double lowestHeight=1;
  for(int tick=0;tick<1800&&!turn.settled();++tick){turn.step(1./120);auto p=turn.pose();bool beat=false;
   for(std::size_t i=0;i<legs.size();++i)if(!previous.swing[i]&&p.swing[i]){++placements;beat=true;}
   if(beat){const double time=tick/120.;beats+=time-lastLaunch>.08;lastLaunch=time;}
   for(std::size_t i=0;i<legs.size();++i)lowestHeight=std::min(lowestHeight,p.legs[i].hip.y/standing.legs[i].hip.y);previous=p;
  }
  std::cout<<(quad?"quad":"biped")<<" turn placements "<<placements<<" coordinated beats "<<beats<<"\n";
  check(turn.settled(),"finite turn fails to finish");
  check(lowestHeight>.90,"turn loses more than ten percent of standing body height");
  // The leading paw plants independently before the paired transfers begin.
  check(placements<=(quad?6:2)&&beats<=(quad?4:2),"turn still subdivides yaw into mincing replants");
 }
}
void realShortRigRegression(const char* name,LimbArray<Leg> legs,bool quad){
 for(double speed:{.5,2.,3.})for(double dt:{1./120,1./30}){
  Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);
  motion.setSpeedMultiplier(speed);motion.reset(0,{-2000,2000});
  auto tick=[&](){
   const auto before=motion.pose();const auto phase=motion.state().phase;
   motion.step(dt);const auto p=motion.pose();const auto after=motion.state().phase;
   const auto attached=[](pets::character::DropPhase v){return v!=pets::character::DropPhase::Held&&v!=pets::character::DropPhase::Airborne;};
   if(attached(phase)&&attached(after))for(std::size_t i=0;i<legs.size();++i){
    check(p.legs[i].hip.y>0,"real short rig drives its hip below the floor");
    check(distance(p.legs[i].hip,p.legs[i].knee)<1.035*distance(legs[i].hip,legs[i].knee)
       &&distance(p.legs[i].knee,p.legs[i].ankle)<1.035*distance(legs[i].knee,legs[i].ankle),
       "real short rig overextends its measured links");
    if(before.grounded[i]&&p.grounded[i])check(distance(before.contacts[i],p.contacts[i])<1e-9,
       "real short rig slides a planted sole");
   }
  };
  try{
   const auto standing=motion.pose();motion.face(-100);int placements=0;
   auto previous=motion.pose();
   for(int i=0;i<int(10/dt)&&!motion.settled();++i){
    tick();const auto p=motion.pose();
    for(std::size_t leg=0;leg<legs.size();++leg){
     placements+=!previous.swing[leg]&&p.swing[leg];
     check(p.legs[leg].hip.y>.90*standing.legs[leg].hip.y,
           "actual short-body turn collapses its standing posture");
    }
    previous=p;
   }
   check(motion.settled()&&placements==(quad?6:2),
         "actual short-body turn adds unnecessary recovery placements");
   check(std::abs(motion.state().targetX-motion.state().rootX)<.02,
         "face-only pivot walks back to its old root");
   motion.target(1200);double peak=0;
   for(int i=0;i<int((1400/(naturalPace(legs)*speed)+10)/dt)&&!motion.settled();++i){tick();peak=std::max(peak,motion.state().velocity);}
   check(peak>naturalPace(legs)*speed*.99,"real short rig fails to reach its natural pace");
   check(motion.settled()&&std::abs(motion.state().rootX-1200)<.03,"real short rig fails to arrive and sleep");
   motion.target(-100);
   for(int i=0;i<int((1400/(naturalPace(legs)*speed)+10)/dt)&&!motion.settled();++i)tick();
   check(motion.settled(),"real short rig fails to finish its reversal");
   motion.target(200);
   for(int i=0;i<int(30/dt);++i){
    if(i==2)motion.target(-200);if(i==4)motion.target(180);if(i==6)motion.target(-180);
    tick();if(i>6&&motion.settled())break;
   }
   check(motion.settled(),"real short rig fails to settle after rapid retargeting");
   motion.pickup(motion.state().rootX,150);motion.release({0,0});
   for(int i=0;i<int((1400/(naturalPace(legs)*speed)+10)/dt)&&!motion.settled();++i)tick();
   check(motion.settled(),"real short rig fails to recover from a drop");
  }catch(const std::exception& e){
   std::cerr<<name<<" speed "<<speed<<" dt "<<dt<<": "<<e.what()<<"\n";throw;
  }
 }
}
void realShortRigRegressions(){
 for(bool sprig:{false,true}){
  const double x=sprig?.22:.23,h=sprig?.46:.43,k=sprig?.295:.28,z=sprig?.08:-.06;
  LimbArray<Leg> legs{{{x,h,0},{x,k,z},{x,.13,0},{x,0,.1}},
                      {{-x,h,0},{-x,k,z},{-x,.13,0},{-x,0,.1}}};
  realShortRigRegression(sprig?"Sprig":"Puddle",legs,false);
 }
 LimbArray<Leg> legs;legs.setCount(4);
 for(int i=0;i<4;++i){const double x=i%2==0?-.44:.44;const bool fore=i<2;
  legs[i]={{x,fore?.48:.46,fore?.56:-.69},{x,fore?.2544:.2438,fore?.49:-.59},
           {x,.1275,fore?.535:-.715},{x,0,fore?.62:-.63}};
 }
 realShortRigRegression("Mossback",legs,true);
}
void directionalLeadingPlant(){
 constexpr double degree=3.141592653589793/180.;
 for(bool quad:{false,true})for(double dt:{1./120,1./30}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*degree);motion.setCameraFacingTurns(true);motion.reset(0,{-2000,2000});
  for(double point:{-100.,100.}){
   const auto start=motion.pose();auto previous=start;motion.face(point);
   const double heading=point<start.rootX?-3.141592653589793+35*degree:-35*degree;
   const Vec3 direction{std::sin(heading),0,std::cos(heading)};
   bool planted=false;double firstAdvance=-1,hipLead=-1,prePlantTravel=10,bodyAtPlant=10;int placements=0;
   const Vec3 startRoot{std::cos(start.yaw)*start.bodyShift.x+std::sin(start.yaw)*start.bodyShift.z,0,
                        -std::sin(start.yaw)*start.bodyShift.x+std::cos(start.yaw)*start.bodyShift.z+start.rootX/100};
   for(int tick=0;tick<int(5/dt)&&!motion.settled();++tick){
    motion.step(dt);const auto p=motion.pose();
    for(std::size_t i=0;i<legs.size();++i){
     placements+=!previous.swing[i]&&p.swing[i];
     if(!planted&&previous.swing[i]&&p.grounded[i]){
      const auto displacement=Vec3{p.contacts[i].x-start.contacts[i].x,0,p.contacts[i].z-start.contacts[i].z};
      const double reach=distance(legs[i].hip,legs[i].knee)+distance(legs[i].knee,legs[i].ankle);
      firstAdvance=(displacement.x*direction.x+displacement.z*direction.z)/reach;
      const auto hip=p.legs[i].hip;
      const Vec3 worldHip{std::cos(p.yaw)*hip.x+std::sin(p.yaw)*hip.z,hip.y,
                          -std::sin(p.yaw)*hip.x+std::cos(p.yaw)*hip.z+p.rootX/100};
      hipLead=((p.contacts[i].x-worldHip.x)*direction.x+(p.contacts[i].z-worldHip.z)*direction.z)/reach;
      const Vec3 root{std::cos(p.yaw)*p.bodyShift.x+std::sin(p.yaw)*p.bodyShift.z,0,
                      -std::sin(p.yaw)*p.bodyShift.x+std::cos(p.yaw)*p.bodyShift.z+p.rootX/100};
      prePlantTravel=distance(root,startRoot)/reach;
      bodyAtPlant=std::abs(p.yaw-start.yaw);planted=true;
     }
    }
    previous=p;
   }
   check(planted&&firstAdvance>=.08,"leading foot fails to step into the requested direction");
   check(bodyAtPlant<degree,"body does not wait for the directional leading plant");
   check(hipLead>=.08,"leading plant is not ahead of its hip in the new direction");
   check(prePlantTravel<.08,"body travels around the turn before the directional paw plant");
   check(motion.settled()&&std::abs(motion.pose().yaw-heading)<1e-8&&placements<=(quad?6:2),
         "directional turn adds placements or loses its requested heading");
  }
 }
}
void feetLeadTurn(){
 constexpr double degree=3.141592653589793/180.;
 for(bool quad:{false,true})for(double dt:{1./120,1./30}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*degree);motion.setCameraFacingTurns(true);motion.reset(0,{-2000,2000});
  const auto start=motion.pose();auto previous=start;motion.face(-100);
  double footLeadAt=10,bodyTenAt=10,firstLandingBody=10;int placements=0,minimumSupport=4;
  for(int tick=0;tick<int(5/dt)&&!motion.settled();++tick){
   motion.step(dt);const auto p=motion.pose();const double time=(tick+1)*dt;
   const double bodyAngle=std::abs(p.yaw-start.yaw);
   if(bodyAngle>=10*degree)bodyTenAt=std::min(bodyTenAt,time);
   int support=0;
   for(std::size_t i=0;i<legs.size();++i){
    support+=p.grounded[i];placements+=!previous.swing[i]&&p.swing[i];
    if(p.swing[i]&&std::abs(p.footYaw[i]-start.footYaw[i])>=20*degree)
     footLeadAt=std::min(footLeadAt,time);
    if(previous.swing[i]&&!p.swing[i])firstLandingBody=std::min(firstLandingBody,bodyAngle);
    check(p.legs[i].hip.y>.9*start.legs[i].hip.y,"foot-led turn collapses standing posture");
    if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,
         "foot-led turn moves a loaded sole");
   }
   if(p.pivoting)minimumSupport=std::min(minimumSupport,support);
   previous=p;
  }
  check(footLeadAt<bodyTenAt,"turning foot does not lead the body heading");
  check(firstLandingBody<1*degree,"body rotates before the leading foot establishes its new plant");
  if(quad)check(minimumSupport>=2,"quadruped turn still spins about one paw");
  check(motion.settled()&&placements<=(quad?6:2),"foot-led turn adds too many placements or fails to stop");
 }
}
void usefulWalkingStrides(){
 for(bool quad:{false,true}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);
  motion.setSpeedMultiplier(2);motion.reset(0,{-5000,5000});const auto standing=motion.pose();motion.target(4500);
  for(int tick=0;tick<480;++tick)motion.step(1./120);
  const double finish=motion.state().rootX+1200;int placements=0;auto previous=motion.pose();
  while(motion.state().rootX<finish){
   motion.step(1./120);const auto p=motion.pose();
   for(std::size_t i=0;i<legs.size();++i){
    placements+=!previous.swing[i]&&p.swing[i];
    if(!quad)check(p.legs[i].hip.y>.94*standing.legs[i].hip.y,
                 "centered biped walk loses its upright stance");
   }
   check(std::abs(motion.state().velocity-naturalPace(legs)*2)<1.,"useful strides change anatomy-derived pace");
   previous=p;
  }
  std::cout<<(quad?"quad":"biped")<<" walking placements "<<placements<<"\n";
  // Judge useful stride against anatomy, since travel pace now varies by rig.
  const double reach=naturalPace(legs)/85;
  check(placements>0&&1200./placements>=reach*100*(quad?.27:.45),
        "walking still uses unnecessarily short strides for its leg length");
 }
}
void quantizedMossbackReversal(){
 LimbArray<Leg> legs;legs.setCount(4);
 for(int i=0;i<4;++i){const double x=i%2==0?-.44:.44;const bool fore=i<2;
  legs[i]={{x,fore?.48:.46,fore?.56:-.69},{x,fore?.2544:.2438,fore?.49:-.59},
           {x,.1275,fore?.535:-.715},{x,0,fore?.62:-.63}};
 }
 constexpr double bias=-35*3.141592653589793/180,dt=1./120;
 Motion motion(legs,{},false,0,Gait::QuadrupedWalk);motion.setFacingBias(bias);
 motion.setCameraFacingTurns(true);motion.setSpeedMultiplier(3);motion.reset(0,{-4000,4000});
 double fraction=0;
 auto tick=[&](){const auto state=motion.state();const auto proposal=motion.propose(dt);
  fraction+=proposal.displacement*.8;const double pixels=std::round(fraction);fraction-=pixels;
  motion.realize(state.rootX+pixels/.8,false,std::max(0.,state.elevation-proposal.displacementY));
 };
 for(int i=0;i<240;++i)tick();
 motion.face(-100);for(int i=0;i<4800&&!motion.settled();++i)tick();
 motion.target(-500);for(int i=0;i<7200&&!motion.settled();++i)tick();
 check(motion.settled()&&std::abs(motion.state().rootX+500)<.75,"quantized Mossback reversal setup does not arrive");
 motion.target(100);for(int i=0;i<7200&&!motion.settled();++i)tick();
 check(motion.settled()&&std::abs(motion.state().rootX-100)<.75&&std::abs(motion.pose().yaw-bias)<1e-8,
       "Mossback rejects a reachable reversal after a fast quantized arrival");
}
void subpixelHeadingDoesNotStep(){
 constexpr double bias=-35*3.141592653589793/180;
 for(double dt:{1./120,1./30}){
  Motion motion(anatomy(true),{},false,0,Gait::QuadrupedWalk);
  motion.setFacingBias(bias);motion.setCameraFacingTurns(true);motion.reset(0,{-1000,1000});
  motion.face(-100);
  for(int tick=0;tick<3000&&motion.pose().yaw==bias;++tick)motion.step(dt);
  motion.propose(dt);motion.realize(motion.state().rootX,true);
  for(int tick=0;tick<3000&&!motion.settled();++tick)motion.step(dt);
  check(motion.settled(),"blocked leading plant does not settle before a small correction");
  auto previous=motion.pose();motion.face(motion.state().rootX+100);int placements=0;
  for(int tick=0;tick<3000&&!motion.settled();++tick){
   motion.step(dt);const auto p=motion.pose();
   for(int i=0;i<4;++i){
    placements+=!previous.swing[i]&&p.swing[i];
    if(previous.grounded[i]&&p.grounded[i])
     check(distance(previous.contacts[i],p.contacts[i])<1e-9,"small heading correction moves a planted paw");
   }
   previous=p;
  }
  check(motion.settled()&&placements==0&&std::abs(motion.pose().yaw-bias)<1e-8,
        "subpixel heading correction launches another six-step turn");
 }
}
void newDestinationDuringTurnRecovery(){
 for(bool quad:{false,true})for(double dt:{1./120,1./30}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);motion.reset(0,{-5000,5000});
  motion.face(-100);for(int tick=0;tick<int(std::ceil(.12/dt));++tick)motion.step(dt);
  const auto heldRoot=motion.state().rootX;motion.propose(dt);motion.realize(heldRoot,true);
  motion.step(dt);motion.target(200);
  for(int tick=0;tick<int(15/dt)&&!motion.settled();++tick)motion.step(dt);
  check(motion.settled()&&std::abs(motion.state().rootX-200)<.03,
        "blocked-turn recovery discards a new destination in the current heading");
 }
}
void blockedPivotLandsAndSleeps(){
 for(bool quad:{false,true}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);motion.reset(0,{-2000,2000});
  const auto standing=motion.pose();motion.face(-100);
  for(int tick=0;tick<20;++tick)motion.step(1./120);
  const auto heldRoot=motion.state().rootX;auto previous=motion.pose();
  for(int tick=0;tick<600&&!motion.settled();++tick){
   motion.propose(1./120);motion.realize(heldRoot,true);const auto p=motion.pose();
   for(std::size_t i=0;i<legs.size();++i){
    check(p.legs[i].hip.y>.9*standing.legs[i].hip.y,"blocked pivot collapses while landing");
    if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,
      "blocked pivot moves its loaded sole");
   }
   previous=p;
  }
  check(motion.settled(),"persistent host blockage leaves an unfinished pivot awake");
  for(bool grounded:motion.pose().grounded)check(grounded,"blocked pivot sleeps with an airborne paw");
 }
}
void pickupKeepsPivotOffset(){
 for(bool quad:{false,true}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);motion.reset(0,{-2000,2000});
  motion.face(-100);for(int tick=0;tick<2400&&!motion.settled();++tick)motion.step(1./120);
  check(motion.settled(),"pickup test turn fails to settle");
  const auto before=motion.pose();motion.pickup(motion.state().rootX,0);const auto after=motion.pose();
  auto horizontalHip=[](const Pose& p){const auto h=p.legs[0].hip;return Vec3{
   std::cos(p.yaw)*h.x+std::sin(p.yaw)*h.z,0,
   p.rootX/100-std::sin(p.yaw)*h.x+std::cos(p.yaw)*h.z};};
  check(distance(horizontalHip(before),horizontalHip(after))<1e-8,
        "pickup snaps the body out of its completed pivot offset");
 }
}
void boundedPivots(){
 for(bool quad:{false,true})for(double width:{0.,.1,1.,5.,20.}){
  const auto legs=anatomy(quad);Motion motion(legs,{},false,0,quad?Gait::QuadrupedWalk:Gait::Biped);
  motion.setFacingBias(-35*3.141592653589793/180);motion.setCameraFacingTurns(true);
  motion.reset(0,{-width,width});motion.setSupportBounds(pets::character::GroundSpan{-90,90});
  const auto standing=motion.pose();motion.face(-100);auto previous=standing;int placements=0;
  for(int tick=0;tick<240&&!motion.settled();++tick){
   motion.step(1./120);const auto p=motion.pose();
   const double bodyZ=p.rootX/100-std::sin(p.yaw)*p.bodyShift.x+std::cos(p.yaw)*p.bodyShift.z;
   check(std::abs(bodyZ)<=width/100+1e-8,"bounded turn hides unrealized root travel in a body offset");
   for(std::size_t i=0;i<legs.size();++i){
    placements+=!previous.swing[i]&&p.swing[i];
    check(std::abs(p.contacts[i].z)<=.90000001,"bounded pivot puts a paw outside its support");
    check(p.legs[i].hip.y>.9*standing.legs[i].hip.y,"bounded pivot collapses upright posture");
    if(previous.grounded[i]&&p.grounded[i])check(distance(previous.contacts[i],p.contacts[i])<1e-9,
          "bounded pivot moves a loaded paw");
   }
   previous=p;
  }
  check(motion.settled(),"unreachable bounded turn keeps scheduling");
  check(placements<=(quad?6:2),"bounded turn repeats tiny partial maneuvers");
 }
}
void resetDiscardsTurnSupport(){
 const auto legs=anatomy(false);
 std::array<Arm,2> arms{{{{.3,1.6,0},{.45,1.2,0},{.5,.95,0},{0,-.05,0},{1,0,0}},
                       {{-.3,1.6,0},{-.45,1.2,0},{-.5,.95,0},{0,-.05,0},{-1,0,0}}}};
 Motion motion(legs,arms);const auto neutral=motion.pose();
 motion.face(-100);for(int tick=0;tick<20;++tick)motion.step(1./120);
 motion.reset(500,{-1000,1000});const auto reset=motion.pose();
 check(motion.settled(),"reset seeds arm followers from stale turn contacts");
 for(int i=0;i<2;++i)check(distance(reset.arms[i]->grip,neutral.arms[i]->grip)<1e-9,"reset retains an old support-height offset in the arm grip");
}
int main(){try{anatomySetsPace();quantizedMossbackReversal();subpixelHeadingDoesNotStep();directionalLeadingPlant();newDestinationDuringTurnRecovery();feetLeadTurn();usefulWalkingStrides();blockedPivotLandsAndSleeps();pickupKeepsPivotOffset();boundedPivots();resetDiscardsTurnSupport();realShortRigRegressions();naturalPaceAndFiniteTurns();turnBeforeTravel();realizedTravel();walkReach();return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
