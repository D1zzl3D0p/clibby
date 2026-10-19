#include "grounded.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class T> auto enable(T& c,int)->decltype(c.setPairedGait(true),void()){c.setPairedGait(true);}
template<class T> void enable(T&,long){throw std::runtime_error("paired gait capability is missing");}
Definition anatomy(){
 Definition d;d.id="paired-test";d.name="Paired test";d.controller="soft";
 d.width=160;d.height=120;d.origin={80,45};d.bindings["body"]=0;
 d.interaction={{0,0},{160,0},{160,120}};d.attachments.push_back({0,{0,0,1,1}});
 d.bones.push_back({"body",-1,{80,45}});
 Locomotion l;l.gait=Gait::QuadrupedWalk;l.legs.setCount(4);l.groundY=95;
 l.speed=70;l.acceleration=240;l.stride=26;l.lift=6;l.minStepDuration=l.maxStepDuration=.18;
 for(int i=0;i<4;++i){int h=1+3*i;d.bones.push_back({"hip"+std::to_string(i),0,{i<2?35.:-35.,0}});
  d.bones.push_back({"knee"+std::to_string(i),h,{-5,30}});
  d.bones.push_back({"ankle"+std::to_string(i),h+1,{5,22}});
  l.legs[i]={h,h+1,h+2,{0,0},i<2?1:-1};}
 d.locomotion=l;return d;
}
void tick(GroundedController& c,bool dropping=false){
 auto before=c.state();auto q=c.propose(1./120);c.realize(before.rootX+q.displacement,false,std::max(0.,before.elevation-q.displacementY));auto s=c.state();
 if(s.phase==DropPhase::Grounded&&before.phase==DropPhase::Grounded){
  int lifted=0;for(int i=0;i<4;++i){lifted+=s.feet[i].swing;
   if(!before.feet[i].swing&&!s.feet[i].swing)check(before.feet[i].position.x==s.feet[i].position.x&&before.feet[i].position.y==s.feet[i].position.y,"paired stance contact slides");}
  check(lifted<=2,"paired gait loses its two support contacts");
  if(!dropping){check(s.feet[0].swing==s.feet[3].swing&&s.feet[1].swing==s.feet[2].swing,"diagonal pair does not share a swing");
   check(std::abs(s.feet[0].phase-s.feet[3].phase)<1e-10&&std::abs(s.feet[1].phase-s.feet[2].phase)<1e-10,"diagonal pair phases differ");}
 }
}
void pivotHeightTransfer(){
 GroundedController c(anatomy());c.reset(0,{-2000,2000});
 const auto resetHeight=c.pose(Action::Rest).bones[0].y;
 c.setSwingAdmission(false);c.holdBodyHeight(2.);
 c.setSupportContacts(std::array<bool,4>{true,false,false,false});
 for(int frame=0;frame<120;++frame)tick(c);
 check(std::abs(c.pose(Action::Rest).bones[0].y-c.definition().bones[0].rest.y-2.)<1e-9,
       "dynamic pivot support collapses held upright height");
 const auto before=c.pose(Action::Rest).bones[0].y;
 c.holdBodyHeight({});c.setSupportContacts(std::array<bool,4>{true,true,true,true});
 check(c.pose(Action::Rest).bones[0].y==before,"releasing pivot height jumps the body");
 c.holdBodyHeight(20.);c.reset(0,{-2000,2000});
 check(std::abs(c.pose(Action::Rest).bones[0].y-resetHeight)<1e-9,
       "reset retains pivot height ownership");
}
void exercise(){for(double speed:{.5,1.,2.,3.}){
 GroundedController c(anatomy());enable(c,0);c.setStepDuration(.18);c.setSpeedMultiplier(speed);c.reset(0,{-2000,2000});
 bool pairA=false,pairB=false;double peak=0;c.setTargetX(1200);
 for(int frame=0;frame<1200;++frame){tick(c);auto s=c.state();peak=std::max(peak,s.velocity);pairA|=s.feet[0].swing&&s.feet[3].swing;pairB|=s.feet[1].swing&&s.feet[2].swing;}
 check(pairA&&pairB,"paired travel never exercises both diagonal pairs");
 check(peak>70*speed*.99,"paired gait reduces accepted navigation pace");
 for(int frame=0;frame<20000&&!c.settled();++frame)tick(c);
 check(c.settled()&&std::abs(c.state().rootX-1200)<.03,"paired travel fails to arrive and settle");
 c.pickup(c.state().rootX,100);c.release({0,0});bool landed=false;
 for(int frame=0;frame<10000&&!c.settled();++frame){tick(c,true);landed|=c.state().phase==DropPhase::Grounded;}
 check(landed&&c.settled(),"paired gait drop does not recover and settle");
}}
}
int main(){try{pivotHeightTransfer();exercise();std::cout<<"Paired gait pace, contacts, coordination and drops passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
