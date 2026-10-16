#include "pose.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character3d;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static std::array<Leg,2> anatomy(){return {{{{.04,.30,0},{.04,.16,.025},{.04,.02,0},{.04,0,.015}},{{-.04,.30,0},{-.04,.16,.025},{-.04,.02,0},{-.04,0,.015}}}};}
int main(){try{
 Motion quickTurn(anatomy(),{},true);quickTurn.setCameraFacingTurns(true);quickTurn.setFacingBias(-.61);quickTurn.reset(0,{-100,100});
 const double originalYaw=quickTurn.pose().yaw;quickTurn.face(-80);
 for(int frame=0;frame<120;++frame)quickTurn.step(1./120);
 check(std::abs(quickTurn.pose().yaw-originalYaw)>1.8 && std::abs(quickTurn.pose().yaw+3.141592653589793-.61)<.03,
       "bird completes grounded reversal within one second");
 auto nearLegs=anatomy();for(auto& leg:nearLegs){leg.hip.y=.17;leg.knee.y=.095;leg.ankle.y=.02;}
 for(bool perch:{false,true}) {
  Motion nearby(nearLegs,{},true,115);nearby.reset(0,{-100,100},perch?-140:0);nearby.flyTo(0,55);
  if(perch){nearby.reconcile(0,{-100,100},140,0);nearby.flyTo(0,195);}
  for(int frame=0;frame<960;++frame){nearby.step(1./120);check(nearby.state().phase==pets::character::DropPhase::Airborne,"nearby spatial flight waits for full wing stroke without floor reattachment");if(perch)check(nearby.state().elevation>140,"nearby spatial flight remains above departing perch");}
  check(std::abs(nearby.state().elevation-(perch?195:55))<15,"nearby spatial long-wing bird reaches hover target");
 }
 auto tiny=anatomy();for(auto& leg:tiny){leg.hip.y=.06;leg.knee.y=.035;leg.ankle.y=.01;}
 Motion shortBird(tiny,{},true,28);shortBird.reset(0,{-100,100});shortBird.flyTo(0,200);
 for(int frame=0;frame<360;++frame)shortBird.step(1./120);
 check(shortBird.state().elevation>160,"measured spatial wings retain climb authority with tiny legs");
 shortBird.land();for(int frame=0;frame<960&&!shortBird.settled();++frame)shortBird.step(1./120);
 check(shortBird.settled(),"short-legged spatial bird lands and folds after sustained flight");
 Motion raven(anatomy(),{},true);Motion plain(anatomy(),{},true);
 check(raven.flightCapable(),"raven spatial proxy enables shared wing capability");
 check(raven.hop(30),"spatial raven can hop");double apex=0;
 for(int i=0;i<4000&&!raven.settled();++i){raven.step(1./120);apex=std::max(apex,raven.state().elevation);auto p=raven.pose();if(raven.state().phase==pets::character::DropPhase::Airborne)check(!p.grounded[0]&&!p.grounded[1],"hop detaches both spatial soles");}
 check(std::abs(apex-30)<.15&&raven.settled(),"shared spatial hop apex and landing recovery");
 raven.reset(0,{-120,120});check(raven.flyTo(60,100),"spatial raven accepts powered destination");
 bool stroked=false;for(int i=0;i<1800;++i){raven.step(1./120);auto p=raven.pose();auto s=raven.state();stroked|=std::abs(p.wings[0].shoulder)>.1;check(p.wings[0].shoulder==s.wings[0].shoulder&&p.wings[0].strokeVelocity==s.wings[0].strokeVelocity,"spatial renderer consumes authoritative shared wings");}
 check(stroked&&raven.state().elevation>40&&std::abs(raven.state().rootX-60)<10,"shared wing lift and thrust move spatial raven");
 raven.land();for(int i=0;i<6000&&!raven.settled();++i)raven.step(1./120);
 check(raven.settled()&&raven.pose().grounded[0]&&raven.pose().grounded[1],"spatial flight lands with planted quiet soles");
 raven.reset(0,{-120,120});raven.pickup(0,100);raven.release({0,90});auto before=raven.state();auto q=raven.propose(1./120);
 raven.setSupportFrame({-100,100},-100+q.displacementY);raven.realize(before.rootX+q.displacement,false,0);
 check(raven.state().phase==pets::character::DropPhase::Landing&&raven.state().verticalVelocity>0,"spatial floor change preserves pending touchdown momentum");
 for(int i=0;i<6000&&!raven.settled();++i)raven.step(1./120);
 check(raven.settled()&&std::abs(raven.pose().contacts[0].y)<1e-9,"spatial floor reference is applied only by host");
 auto stable=raven.pose();raven.step(1./60);check(raven.pose().contacts[0].z==stable.contacts[0].z,"landed spatial support remains fixed");
 Motion bounded(anatomy(),{},true);bounded.reset(0,{-10,10});bounded.setSupportBounds(pets::character::GroundSpan{-10,10});
 for(double target:{10.,-10.,10.}) {
  bounded.target(target);
  for(int frame=0;frame<3000&&!bounded.settled();++frame) {
   const auto before=bounded.pose();bounded.step(1./120);const auto p=bounded.pose();
   for(int side=0;side<2;++side) {
    if(!p.swing[side])check(p.contacts[side].z*100>=-10-1e-8&&p.contacts[side].z*100<=10+1e-8,"spatial sole touchdown stays inside raw edge during turn");
    if(before.grounded[side]&&p.grounded[side])check(before.contacts[side].z==p.contacts[side].z,"spatial bounds preserve planted contacts");
   }
  }
  check(bounded.settled(),"bounded spatial edge walking settles after reversal");
 }
 const auto carried=bounded.pose();const auto carriedState=bounded.state();
 bounded.reconcile(carriedState.rootX+10,{0,20},0,5);
 const auto moved=bounded.pose();
 for(int side=0;side<2;++side)check(std::abs((moved.contacts[side].z-carried.contacts[side].z)*100-10)<1e-8,"moving support translates spatial pins before new bounds arrive");
 bounded.setSupportBounds(pets::character::GroundSpan{0,20});
 std::cout<<"shared spatial raven hop, flight, wings, pending edge touchdown and quiet pins passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
