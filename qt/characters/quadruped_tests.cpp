#include "grounded.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace pets::character;
static void check(bool value,const char *why) {
  if(!value) throw std::runtime_error(why);
}
static Definition quadruped() {
  Definition d;
  d.id="synthetic-quad"; d.name="Synthetic quad"; d.controller="soft";
  d.width=160;d.height=100;d.origin={80,45};d.bindings["body"]=0;
  d.interaction={{0,0},{160,0},{160,100}};
  d.bones.push_back({"body",-1,{80,45}});
  Locomotion l; l.gait=Gait::QuadrupedWalk;l.legs.setCount(4);
  l.groundY=85;l.speed=28;l.stride=14;
  const double offsets[]={24,18,-24,-18};
  for(int i=0;i<4;++i) {
    const int h=1+3*i;
    d.bones.push_back({"hip"+std::to_string(i),0,{offsets[i],0}});
    d.bones.push_back({"knee"+std::to_string(i),h,{0,24.+i}});
    d.bones.push_back({"ankle"+std::to_string(i),h+1,{0,20.-i}});
    l.legs[i]={h,h+1,h+2,{0,1},i<2?1:-1};
    d.contacts.push_back({h+2,{0,1}});
  }
  d.attachments.push_back({0,{0,0,1,1}});d.locomotion=l;return d;
}
static void endpoints(const GroundedController &c,double floor=0) {
  const auto s=c.state();const auto p=c.pose(Action::Rest);const auto w=worldTransforms(c.definition(),p);
  for(int i=0;i<4;++i) {
    const auto l=c.definition().locomotion->legs[i];const auto sole=w[l.ankle].apply(l.contact);
    check(std::isfinite(sole.x)&&std::isfinite(sole.y),"nonfinite rendered sole");
    if(s.phase==DropPhase::Grounded || !s.feet[i].swing) {
      check(std::abs(sole.x+s.rootX-c.definition().origin.x-s.feet[i].position.x)<1e-6,"rendered sole X lost pin");
      check(std::abs(sole.y-85+floor-s.feet[i].position.y)<1e-6,"rendered sole Y lost pin");
    }
  }
}
static void tick(GroundedController &c,bool quantized=false,bool blocked=false,double *residual=nullptr) {
  const auto before=c.state();const auto q=c.propose(1./120);
  double x=before.rootX+q.displacement;
  if(quantized) {const double desired=x+*residual;x=std::round(desired);*residual=desired-x;}
  c.realize(blocked?before.rootX:x,blocked);
  const auto after=c.state();
  int swings=0;
  for(int i=0;i<4;++i) {
    swings+=after.feet[i].swing;
    if(!before.feet[i].swing&&!after.feet[i].swing) {
      check(before.feet[i].position.x==after.feet[i].position.x,"stance X slipped");
      check(before.feet[i].position.y==after.feet[i].position.y,"stance Y slipped");
    }
  }
  if(after.phase==DropPhase::Grounded)check(swings<=1,"multiple ordinary swings");
}
static void settle(GroundedController &c,bool quantized=false) {
  double residual=0;
  for(int f=0;f<8000&&!c.settled();++f)tick(c,quantized,false,&residual);
  check(c.settled(),"quad failed to settle");
  check(!c.state().anyFootSwinging(),"settled quad exposes swinging foot");
}
int main() {
 try {
  GroundedController c(quadruped());
  check(c.state().feet.size()==4,"state dropped hind contacts");
  check(c.state().soleExtent().minimum==-24&&c.state().soleExtent().maximum==24,"all-foot extent");
  check(!c.hop(10)&&!c.flyTo(10,10),"unsupported quad launch accepted");
  endpoints(c);settle(c);
  // Turning transforms proximal geometry while every remaining stance pin stays fixed.
  GroundedController turning(quadruped());settle(turning);
  check(turning.face(-100),"rest turn rejected");
  std::vector<int> turnOrder;
  for(int f=0;f<120;++f){
    const auto before=turning.state();tick(turning);endpoints(turning);
    const auto after=turning.state();
    for(int i=0;i<4;++i)if(!before.feet[i].swing&&after.feet[i].swing)turnOrder.push_back(i);
  }
  check(!turnOrder.empty()&&turnOrder.front()==2,"rest turn ignored cyclic replant priority");
  settle(turning);endpoints(turning);
  const auto turned=worldTransforms(turning.definition(),turning.pose(Action::Rest));
  check(turned[1].tx<turned[7].tx,"rest turn left fore hips behind body");
  const auto quiet=turning.state();
  for(int f=0;f<20;++f){endpoints(turning);check(!turning.state().active,"pose reads restarted motion");}
  for(int i=0;i<4;++i)check(turning.state().feet[i].position.x==quiet.feet[i].position.x,"pose read moved contact");
  // Placement origin may differ from the body anchor: reversal preserves the
  // anatomical hip-to-home offset about the authored body, not the canvas origin.
  auto shiftedDefinition=quadruped();shiftedDefinition.origin.x=60;
  GroundedController shifted(shiftedDefinition);settle(shifted);
  check(shifted.face(-100),"shifted-origin turn rejected");
  for(int f=0;f<8000&&!shifted.settled();++f){tick(shifted);endpoints(shifted);}
  check(shifted.settled(),"shifted-origin reversal failed to settle");
  const double reflectedHomes[]={-4,2,44,38};
  for(int i=0;i<4;++i)
    check(std::abs(shifted.state().feet[i].position.x-reflectedHomes[i])<5,
          "reversed home reflected about placement origin instead of body anchor");
  shifted.pickup(0,0);shifted.release({0,0});
  for(int i=0;i<4;++i)
    check(std::abs(shifted.state().feet[i].position.x-reflectedHomes[i])<1e-9,
          "drop home disagrees with reflected body anchor");
  endpoints(shifted);
  c.setTargetX(180);
  std::array<bool,4> stepped{};std::vector<int> order;
  for(int f=0;f<5000&&!c.settled();++f) {
    const auto before=c.state();tick(c);const auto after=c.state();endpoints(c);
    for(int i=0;i<4;++i)if(!before.feet[i].swing&&after.feet[i].swing){stepped[i]=true;order.push_back(i);}
  }
  check(c.settled(),"walking arrival failed");
  check(std::all_of(stepped.begin(),stepped.end(),[](bool v){return v;}),"a paw never stepped");
  check(order.size()>=4&&order[0]==2&&order[1]==0&&order[2]==3&&order[3]==1,"wrong four-beat order");
  c.setTargetX(-120.4);settle(c,true);endpoints(c);
  const auto reversed=worldTransforms(c.definition(),c.pose(Action::Rest));
  check(reversed[1].tx<reversed[7].tx,"fore/hind hips did not turn");
  c.setTargetX(200);for(int f=0;f<30;++f)tick(c);tick(c,false,true);settle(c);endpoints(c);
  // A false contact must prevent admission even with the other three planted.
  GroundedController missing(quadruped());missing.setSupportContacts(std::array<bool,4>{{true,true,true,false}});
  missing.setTargetX(80);for(int f=0;f<80;++f)tick(missing);
  check(!missing.state().anyFootSwinging(),"swing admitted without three other supports");
  missing.setSupportContacts(std::array<bool,4>{{true,true,true,true}});settle(missing);
  bool rejected=false;try {missing.setSupportContacts(std::array<bool,2>{{true,true}});}catch(const std::exception&){rejected=true;}
  check(rejected,"support count mismatch accepted");
  // Drop/regrab retains four contacts and bounded touchdown coordinates.
  c.pickup(10,100);c.release({12,0});for(int f=0;f<15;++f)tick(c);
  c.pickup(20,80);c.hold(25,90);c.setSupportBounds(GroundSpan{0,50});c.release({0,0});
  for(int f=0;f<8000&&!c.settled();++f){tick(c);if(c.state().phase!=DropPhase::Airborne)endpoints(c);}
  check(c.settled(),"drop/regrab failed to settle");
  check(c.state().feet.size()==4,"drop reset active count");
  for(const auto &foot:c.state().feet)check(foot.position.x>=0&&foot.position.x<=50,"drop contact outside bounds");
  const auto old=c.state();c.setSupportFrame({-1000,1000},4);
  for(int i=0;i<4;++i)check(c.state().feet[i].position.y==old.feet[i].position.y+4,"floor omitted contact");
  endpoints(c,4);settle(c);endpoints(c,4);
  c.reset(0,{-1000,1000});check(c.state().feet.size()==4,"reset lost active count");
  auto biped=quadruped();biped.locomotion->gait=Gait::Biped;biped.locomotion->legs.setCount(2);
  biped.contacts.resize(2);biped.bones.resize(7);
  GroundedController legacy(biped);check(legacy.state().feet.size()==2,"biped exposes phantom contacts");
  rejected=false;try {legacy.setSupportContacts(std::array<bool,4>{{true,true,true,true}});}catch(const std::exception&){rejected=true;}
  check(rejected,"four-contact setter changed biped anatomy");
  std::cout<<"quadruped motion passed\n";return 0;
 } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
