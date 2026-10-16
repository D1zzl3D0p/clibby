#include "pose.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace pets::character3d;
using pets::character::Role;
double distance(Vec3 a,Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 std::array<Leg,2> legs{{{{.18,.85,0},{.18,.43,.13},{.18,.08,0},{.18,0,.1}},{{-.18,.85,0},{-.18,.43,.13},{-.18,.08,0},{-.18,0,.1}}}};
 std::array<Arm,2> arms{{{{.42,1.6,0},{.57,1.28,0},{.61,.97,0},{0,-.08,0},{1,-.4,.2}},{{-.42,1.6,0},{-.57,1.28,0},{-.61,.97,0},{0,-.08,0},{-1,-.4,.2}}}};
 Motion m(legs,arms);const auto initial=m.pose();
 m.armGoal(0,{.7,1.4,.3});
 check(distance(m.pose().arms[0]->grip,initial.arms[0]->grip)<1e-12,"reach applies instantly");
 m.expressions().look({.5,.2,1});m.expressions().goal(Role::EarLeft,{0,0,.25});m.expressions().goal(Role::Tail,{0,.3,0});
 for(int i=0;i<1200&&!m.settled();++i){m.step(1./120);auto p=m.pose();for(int side=0;side<2;++side)check(distance(p.contacts[side],initial.contacts[side])<1e-12,"expression moves floor contacts");}
 check(m.settled(),"held nonzero expression does not settle");auto held=m.pose();
 check(std::abs(held.head.y)>.1&&std::abs(held.ears[0].z)>.1&&std::abs(held.tail.y)>.1,"expression absent from snapshot");
 check(distance(held.arms[0]->grip,{.7,1.4,.3})<.001,"tracked reach misses goal");
 m.clearArmGoal(0);m.expressions().clear();
 check(distance(m.pose().arms[0]->grip,held.arms[0]->grip)<1e-12,"release snaps arm");
 for(int i=0;i<1200&&!m.settled();++i)m.step(1./120);
 check(m.settled(),"released expression does not settle");auto neutral=initial.arms[0]->grip;neutral.y+=m.pose().legs[0].hip.y-initial.legs[0].hip.y;check(distance(m.pose().arms[0]->grip,neutral)<.001,"release fails neutral return");
 check(!m.reach(1,{NAN,0,0}),"nonfinite normalized reach accepted");
 Motion legsOnly(legs);check(!legsOnly.reach(0,{.3,.7,.1}),"missing reach capability accepted");
 check(!legsOnly.expressions().goal(Role::WristLeft,{1,0,0}),"missing wrist capability accepted");
 m.reset(0,{-120,120});check(m.settled(),"reset retains expressive activity");
 std::cout<<"3D tracked expressions, reach, release and contact isolation passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
