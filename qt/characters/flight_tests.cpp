#include "grounded.h"
#include "wing_motion.h"
#include "loader.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character;
static void check(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
static Definition bird(bool wings=true){
 Definition d;d.id=d.name="flight-test";d.controller="articulated";
 d.width=d.height=160;d.origin={80,120};d.bindings["body"]=0;
 d.interaction={{0,0},{160,0},{160,160}};d.attachments.push_back({0,{0,0,1,1},{},{},0});
 d.bones={{"body",-1,{80,90}},{"lh",0,{-6,0}},{"lk",1,{0,16}},{"la",2,{0,16}},
 {"rh",0,{6,0}},{"rk",4,{0,16}},{"ra",5,{0,16}}};
 Locomotion l;l.groundY=120;l.stride=12;l.legs={LegChain{1,2,3,{},1},LegChain{4,5,6,{},1}};d.locomotion=l;
 if(wings)for(auto side:{"Left","Right"}){int n=d.bones.size();
  d.bones.push_back({std::string("wing")+side,0,{-3,-8}});
  d.bones.push_back({std::string("elbow")+side,n,{14,0}});
  d.bones.push_back({std::string("wrist")+side,n+1,{14,0}});
  d.bindings[std::string("wingShoulder")+side]=n;
  d.bindings[std::string("wingElbow")+side]=n+1;
  d.bindings[std::string("wingWrist")+side]=n+2;
 }
 return d;
}
static void tick(GroundedController& c,double dt=1./120){auto s=c.state();auto q=c.propose(dt);c.realize(s.rootX+q.displacement,false,std::max(0.,s.elevation-q.displacementY));}
int main(int argc,char** argv){try{
 if(argc>1) {
  GroundedController authored(loadCharacter(QString::fromLocal8Bit(argv[1])).definition);authored.reset(0,{-1000,1000});
  const auto rest=worldTransforms(authored.definition(),authored.pose(Action::Rest));
  for(const auto& leg:authored.definition().locomotion->legs) {
    const auto hip=rest[leg.hip],knee=rest[leg.knee],ankle=rest[leg.ankle];
    const double reach=std::hypot(knee.tx-hip.tx,knee.ty-hip.ty)+std::hypot(ankle.tx-knee.tx,ankle.ty-knee.ty);
    const double bend=std::abs((ankle.tx-hip.tx)*(knee.ty-hip.ty)-(ankle.ty-hip.ty)*(knee.tx-hip.tx))/std::hypot(ankle.tx-hip.tx,ankle.ty-hip.ty);
    check(bend>reach*.15,"authored raven keeps a visible hock bend at rest");
  }
  check(authored.flyTo(60,120),"authored raven accepts flight");
  for(int frame=0;frame<960;++frame)tick(authored);
  check(authored.state().phase==DropPhase::Airborne&&std::abs(authored.state().elevation-120)<15,"authored wing and leg dimensions reach requested flight height");
  authored.land();for(int frame=0;frame<960&&!authored.settled();++frame)tick(authored);
  check(authored.settled(),"authored raven lands and folds after sustained flight");
 }
 GroundedController turningBird(bird());turningBird.reset(0,{-1000,1000});turningBird.flyTo(200,100);
 for(int frame=0;frame<240;++frame)tick(turningBird);
 const double turnStart=turningBird.state().rootX;
 turningBird.flyTo(-200,100);
 for(int frame=0;frame<360;++frame)tick(turningBird);
 check(turningBird.state().facing==-1 && turningBird.state().facingBlend<-.99,
       "airborne bird turns its visible body toward its new flight direction");
 check(turningBird.state().rootX<turnStart && turningBird.state().phase==DropPhase::Airborne,
       "mid-flight turn redirects procedural thrust without landing");
 GroundedController banked(bird());banked.reset(0,{-10000,10000});banked.flyTo(10000,100);
 for(int frame=0;frame<480;++frame)tick(banked);
 std::cout<<"banked climb height "<<banked.state().elevation<<"\n";
 check(banked.state().elevation>95,"sustained banked flight preserves commanded climb height");
 GroundedController c(bird());check(c.flightCapable(),"complete bird wing anatomy supports flight");
 GroundedController plain(bird(false));check(!plain.flightCapable()&&!plain.flyTo(10,100),"missing wings reject powered flight");
 check(c.hop(40),"supported bird can launch hop");check(c.state().feet[0].swing&&c.state().feet[1].swing,"hop detaches both contacts");
 double apex=0;for(int i=0;i<2000&&!c.settled();++i){tick(c);apex=std::max(apex,c.state().elevation);}
 check(std::abs(apex-40)<.15,"hop follows requested ballistic apex");check(c.settled(),"hop lands and recovers quietly");
 DropMotion a(32),b(32);a.pickup(200);b.pickup(200);a.release({12,-20});b.release({12,-20});
 for(int i=0;i<30;++i){auto x=a.advance(1./120);auto y=b.advance(1./120,FlightForces{});check(std::abs(x.displacementY-y.displacementY)<1e-10,"zero force preserves passive drop");}
 WingMotion wing;check(wing.state()[0].elbow==0&&wing.state()[0].wrist==0,"folded wings preserve authored joint rest");check(wing.forces(32,{}).lift==0,"stationary wings provide no stroke lift");
 wing.goal(1,1,0);bool lift=false;for(int i=0;i<240;++i){wing.step(1./120);auto f=wing.forces(32,{});lift|=f.lift>1;check(f.lift<=6*18*32,"wing acceleration bounded");}check(lift,"realized downstroke supplies lift");
 c.reset(0,{-1000,1000});check(c.flyTo(120,100),"bird accepts powered destination");
 double high=0;for(int i=0;i<2400;++i){tick(c);high=std::max(high,c.state().elevation);}
 std::cout<<"flight peak "<<high<<" final elevation "<<c.state().elevation<<" target x error "<<c.state().rootX-120<<"\n";
 check(std::abs(c.state().elevation-100)<10,"sustained hover tracks requested height without large permanent bias");
 check(high>70&&c.state().phase==DropPhase::Airborne,"powered wings keep body airborne");
 check(std::abs(c.state().rootX-120)<20,"wing thrust approaches horizontal goal");
 // The authored long-wing, short-leg bird still needs stroke startup time
 // when a nearby target would otherwise request a tiny push-off apex.
 auto longWings=bird();for(int joint:{2,3,5,6})longWings.bones[joint].rest.y=7.5;
 longWings.locomotion->groundY=104;longWings.origin.y=104;
 longWings.locomotion->stride=6;longWings.locomotion->lift=2;longWings.locomotion->reachThreshold=4;
 for(const auto side:{"Left","Right"})for(const auto role:{"wingElbow","wingWrist"})longWings.bones[longWings.bindings.at(std::string(role)+side)].rest.x=57.5;
 for(bool perch:{false,true}) {
  GroundedController nearby(longWings);nearby.reset(0,{-1000,1000},perch?-140:0);
  check(nearby.flyTo(0,55),"long-wing bird accepts nearby flight destination");
  if(perch){nearby.reconcile(0,{-1000,1000},0,140);nearby.flyTo(0,195);}
  for(int frame=0;frame<960;++frame){tick(nearby);check(nearby.state().phase==DropPhase::Airborne,"nearby flight stays detached through first full wing stroke");if(perch)check(nearby.state().elevation>140,"nearby perched flight clears departing support before sustained lift");}
  check(std::abs(nearby.state().elevation-(perch?195:55))<15,"nearby long-wing flight settles around requested height");
 }
 // Short bird legs must not reduce the authority of unchanged full wings.
 auto shortLegs=bird();for(int joint:{2,3,5,6})shortLegs.bones[joint].rest.y=3;
 shortLegs.locomotion->groundY=95;shortLegs.origin.y=95;shortLegs.locomotion->stride=3;shortLegs.locomotion->lift=1;shortLegs.locomotion->reachThreshold=2;
 GroundedController compact(shortLegs);compact.reset(0,{-1000,1000});compact.flyTo(0,200);
 for(int frame=0;frame<360;++frame)tick(compact);
 std::cout<<"compact3s "<<compact.state().elevation<<" vy "<<compact.state().verticalVelocity<<"\n";
 check(compact.state().elevation>160,"full wings lift short-legged bird toward target within three seconds");
 for(int frame=360;frame<960;++frame)tick(compact);
 std::cout<<"compact altitude "<<compact.state().elevation<<" velocity "<<compact.state().verticalVelocity<<"\n";
 check(std::abs(compact.state().elevation-200)<15,"short-legged full-wing bird reaches requested hover height promptly");
 compact.land();int descentFrames=0;
 for(;descentFrames<960&&!compact.settled();++descentFrames)tick(compact);
 check(compact.settled()&&compact.state().phase==DropPhase::Grounded,"short-legged bird lands and folds within eight seconds");
 GroundedController ceiling(bird());ceiling.reset(0,{-1000,1000});ceiling.flyTo(0,1000);
 for(int frame=0;frame<1200;++frame){auto old=ceiling.state();auto q=ceiling.propose(1./120);ceiling.realize(old.rootX+q.displacement,false,std::min(200.,std::max(0.,old.elevation-q.displacementY)));}
 ceiling.land();for(int frame=0;frame<960&&!ceiling.settled();++frame)tick(ceiling);
 check(ceiling.settled(),"ceiling-blocked powered bird obeys land and returns to quiet support promptly");
 GroundedController rescued(bird());rescued.reset(0,{-1000,1000});
 rescued.pickup(0,100);rescued.release({0,0});
 check(rescued.flyTo(0,100),"airborne thrown bird can resume powered flight");
 for(int frame=0;frame<1200;++frame)tick(rescued);
 check(rescued.state().phase==DropPhase::Airborne&&rescued.state().elevation>70,
       "powered flight after throw uses compatible gravity and lift");
 DropMotion poweredThrow(32);poweredThrow.pickup(10000);poweredThrow.release({1600,-1200});
 poweredThrow.advance(1./120,FlightForces{1,0,.01});
 check(poweredThrow.velocityX()>1500 && poweredThrow.velocityY()<-1000,
       "small residual wing force cannot abruptly clip throw momentum");
 DropMotion reused(32);reused.launch({0,-100});reused.reset();
 reused.realizeElevation(500,false);const auto reusedStep=reused.advance(1./120);
 DropMotion fresh(32);fresh.realizeElevation(500,false);const auto freshStep=fresh.advance(1./120);
 check(std::abs(reusedStep.displacementY-freshStep.displacementY)<1e-10,
       "reset restores ordinary falling gravity after a hop");
 auto before=c.state();c.propose(1./30);check(c.state().elevation==before.elevation&&c.state().wings[0].shoulder==before.wings[0].shoulder,"proposal leaves realized body and wings unchanged");c.realize(before.rootX,false,before.elevation);c.realize(999,false,999);check(c.state().rootX==before.rootX,"realization consumes proposal once");
 c.setFlightEffortLimit(0);double start=c.state().elevation;for(int i=0;i<240;++i)tick(c);check(c.state().elevation<start,"insufficient wing effort cannot hold altitude");
 c.cancelFlight();for(int i=0;i<4000&&!c.settled();++i)tick(c);check(c.settled(),"cancelled flight settles after landing");
 c.flyTo(60,100);for(int i=0;i<200;++i)tick(c);c.pickup(c.state().rootX,c.state().elevation);c.release({0,0});check(!c.state().poweredFlight,"regrab clears stale powered target");
 // Acquiring a raised edge between proposal and realization retains momentum.
 GroundedController perched(bird());perched.pickup(0,100);perched.release({0,100});
 auto previous=perched.state();auto crossing=perched.propose(1./120);
 perched.setSupportFrame({-200,200},-100+crossing.displacementY);
 perched.realize(previous.rootX+crossing.displacement,false,0);
 check(perched.state().phase==DropPhase::Landing&&perched.state().verticalVelocity>0,"new floor between proposal and realize lands with momentum");
 for(int i=0;i<3000&&!perched.settled();++i)tick(perched);
 check(perched.settled(),"raised-edge landing returns to settled support");
 c.reset(0,{-1000,1000});c.setFlightEffortLimit(1);c.flyTo(80,100);
 for(int i=0;i<400;++i)tick(c);c.land();
 for(int i=0;i<4000&&!c.settled();++i)tick(c);
 check(c.settled()&&!c.state().poweredFlight,"landing goal folds wings and resumes quiet walking support");
 wing.goal(1,0,0);for(int i=0;i<1500;++i)wing.step(1./120);
 check(wing.forces(32,{}).lift==0,"extended stationary wings do not secretly hover");
 plain.reset(0,{-1000,1000});check(plain.hop(20),"legged anatomy can hop without flight wings");
 auto malformed=bird();malformed.bindings.erase("wingWristLeft");bool rejected=false;try{validateDefinition(malformed);}catch(const std::exception&){rejected=true;}check(rejected,"partial wing anatomy rejected");
 auto limited=bird();limited.bones[limited.bindings.at("wingShoulderLeft")].maxRotation=.1;
 rejected=false;try{validateDefinition(limited);}catch(const std::exception&){rejected=true;}
 check(rejected,"wing limits must admit physical stroke so renderer cannot clip lift-producing motion");
 c.reset(0,{-1000,1000});c.flyTo(0,100);for(int i=0;i<150;++i)tick(c);
 c.reconcile(c.state().rootX,{-1000,1000},0,0);
 check(!c.state().poweredFlight,"host forced touchdown clears powered command");
 GroundedController edgeWalker(bird(false));edgeWalker.reset(0,{-24,24});
 edgeWalker.setSupportBounds(GroundSpan{-30,30});edgeWalker.setSpeedMultiplier(3);
 int boundedSteps=0;
 for(double destination:{24.,-24.,24.}) {
   edgeWalker.setTargetX(destination);
   for(int frame=0;frame<600;++frame) {
     const auto old=edgeWalker.state();tick(edgeWalker);const auto state=edgeWalker.state();
     for(int side=0;side<2;++side) {
       if(state.feet[side].swing)check(state.feet[side].landing.x>=-30&&state.feet[side].landing.x<=30,"window anticipation touchdown stays on raw support segment");
       if(!old.feet[side].swing&&!state.feet[side].swing)check(old.feet[side].position.x==state.feet[side].position.x,"bounds never slide current pinned soles");
       if(old.feet[side].swing&&!state.feet[side].swing){++boundedSteps;check(state.feet[side].position.x>=-30&&state.feet[side].position.x<=30,"landed sole is supported by edge");}
     }
   }
 }
 check(boundedSteps>3,"edge travel exercises multiple bounded touchdowns");
 auto narrow=bird(false);narrow.bones[1].rest.x=-1;narrow.bones[4].rest.x=1;
 GroundedController recovery(narrow);recovery.reset(0,{0,0});recovery.setSupportBounds(GroundSpan{-1,1});
 recovery.pickup(0,500);recovery.release({0,0});int recoveryLaunches=0;
 for(int frame=0;frame<4000&&!recovery.settled();++frame) {
   const auto old=recovery.state();tick(recovery);const auto state=recovery.state();
   if(state.phase==DropPhase::Recovering)for(int side=0;side<2;++side)if(state.feet[side].swing) {
     check(state.feet[side].landing.x>=-1&&state.feet[side].landing.x<=1,"impact recovery touchdown stays on narrow edge");
     if(!old.feet[side].swing)++recoveryLaunches;
   }
 }
 check(recoveryLaunches==1&&recovery.settled(),"bounded impact recovery completes one stable step");
 edgeWalker.setSupportBounds(std::nullopt);
 bool invalidBounds=false;try{edgeWalker.setSupportBounds(GroundSpan{1,-1});}catch(const std::exception&){invalidBounds=true;}
 check(invalidBounds,"inverted sole support interval rejected");
 std::cout<<"shared hop, wing force, powered flight, descent, contacts and interruption passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
