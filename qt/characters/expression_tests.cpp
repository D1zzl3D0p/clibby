#include "expression.h"
#include "grounded.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
#undef assert
#define assert(condition) do { if(!(condition)) throw std::runtime_error("check failed: " #condition); } while(false)
using namespace pets::character;
using pets::motion::Vec3;
static Definition creature() {
  Definition d; d.id="expression-test"; d.name="test"; d.controller="soft";
  d.width=d.height=100; d.origin={50,60};
  d.interaction={{0,0},{100,0},{100,100}};
  d.bones={{"body",-1,{50,60}}, {"lh",0,{-6,0}}, {"lk",1,{0,16}},
           {"la",2,{0,16}}, {"rh",0,{6,0}}, {"rk",4,{0,16}},
           {"ra",5,{0,16}}, {"head",0,{0,-20}}, {"el",7,{-5,-8}},
           {"er",7,{5,-8}}, {"tail",0,{-8,0}}};
  d.bindings={{"body",0},{"head",7},{"earLeft",8},{"earRight",9},{"tail",10}};
  d.contacts={{3,{}},{6,{}}}; d.attachments.push_back({0,{0,0,1,1}});
  Locomotion l; l.groundY=90; l.stride=12;
  l.legs={LegChain{1,2,3,{},1},LegChain{4,5,6,{},-1}}; d.locomotion=l;
  return d;
}
static void settle(ExpressionMotion &m) {
  for (int i=0; i<2400 && !m.settled(); ++i) m.step(1./120);
  assert(m.settled());
}
int main() {
  ExpressionMotion m(roleBit(Role::Head)|roleBit(Role::EarLeft)|roleBit(Role::EarRight)|roleBit(Role::Tail));
  assert(m.settled());
  assert(!m.goal(Role::WristLeft,{0,0,.2}));
  assert(!m.impulse(Role::Count,{0,0,1}));
  assert(m.goal(Role::EarLeft,{0,0,.3}));
  assert(m.value(Role::EarLeft).z==0 && !m.settled());
  m.step(1./120);
  assert(m.value(Role::EarLeft).z<.3);
  settle(m);
  assert(std::abs(m.value(Role::EarLeft).z-.3)<.00001);
  assert(m.value(Role::EarRight).z==0 && m.value(Role::Head).z==0 && m.value(Role::Tail).z==0);
  const auto previous=m.value(Role::EarLeft);
  assert(!m.goal(Role::EarLeft,{0,0,std::numeric_limits<double>::infinity()}));
  assert(!m.impulse(Role::EarLeft,{std::numeric_limits<double>::quiet_NaN(),0,0}));
  assert(m.value(Role::EarLeft).z==previous.z && m.settled());
  assert(m.clear(Role::EarLeft));
  assert(m.value(Role::EarLeft).z==previous.z && !m.settled());
  m.step(1./120);
  assert(m.value(Role::EarLeft).z>0);
  settle(m);
  assert(m.value(Role::EarLeft).z==0);
  assert(m.look({1,1,1}));
  assert(m.value(Role::Head).x==0);
  settle(m);
  assert(std::abs(m.value(Role::Head).x)>.2 && std::abs(m.value(Role::Head).y)>.2);
  const auto looked=m.value(Role::Head);
  assert(!m.look({0,0,0}));
  assert(!m.look({std::numeric_limits<double>::quiet_NaN(),1,1}));
  assert(m.value(Role::Head).x==looked.x);
  assert(m.goal(Role::Head,{0,0,.2})); // tilt composes with gaze
  settle(m);
  assert(m.value(Role::Head).x==looked.x && m.value(Role::Head).z==.2);
  assert(m.goal(Role::Tail,{1e300,-1e300,1e300}));
  settle(m);
  assert(std::abs(m.value(Role::Tail).z)<1.5);
  m.clear(); settle(m);
  assert(m.value(Role::Head).x==0 && m.value(Role::Head).y==0);
  assert(m.impulse(Role::EarRight,{0,0,2}));
  assert(!m.settled()); m.step(1./120);
  assert(m.value(Role::EarRight).z!=0 && m.value(Role::EarLeft).z==0);
  ExpressionMotion copy=m;
  copy.reset(); assert(copy.settled() && !m.settled());
  settle(m);
  assert(m.goal(Role::Tail,{0,0,.4}));
  m.step(3600); // bounded clock jump
  assert(m.value(Role::Tail).z<.4);
  m.reset(); assert(m.settled());
  Controller planar(creature());
  planar.setAction(Action::Celebrate,.1);
  assert(!planar.settled());
  const auto instant=planar.pose(Action::Celebrate,.1);
  planar.step(1./120);
  planar.step(1./120);
  assert(planar.pose(Action::Celebrate,.9).bones[7].rotation==
         planar.pose(Action::Rest,100).bones[7].rotation);
  assert(planar.pose(Action::Celebrate,.9).bones[7].rotation!=instant.bones[7].rotation);
  planar.setAction(Action::Rest,0);
  for(int i=0;i<2400&&!planar.settled();++i)planar.step(1./120);
  assert(planar.settled());
  planar.expressions().look({1,0,1});
  for(int i=0;i<2400&&!planar.settled();++i)planar.step(1./120);
  assert(planar.settled() && std::abs(planar.pose(Action::Rest,0).bones[7].rotation)>.1);
  GroundedController grounded(creature());
  grounded.setAction(Action::Rest,0);
  grounded.expressions().goal(Role::EarLeft,{0,0,.3});
  grounded.expressions().look({0,1,1});
  const auto pins=grounded.state().feet;
  for(int i=0;i<2400&&!grounded.settled();++i) {
    auto move=grounded.propose(1./120);
    grounded.realize(grounded.state().rootX+move.displacement);
    auto state=grounded.state(); const auto world=worldTransforms(creature(),grounded.pose());
    for(int side=0;side<2;++side) {
      assert(!state.feet[side].swing && state.feet[side].position.x==pins[side].position.x);
      const auto point=world[side?6:3].apply({});
      assert(std::abs(point.x+state.rootX-50-pins[side].position.x)<1e-8);
      assert(std::abs(point.y-90-pins[side].position.y)<1e-8);
    }
  }
  assert(grounded.settled());
  assert(std::abs(grounded.pose().bones[8].rotation-.3)<1e-6);
  assert(grounded.pose().bones[9].rotation==0);
  assert(std::abs(grounded.pose().bones[7].rotation)>.2);
  const auto held=grounded.pose();
  const auto repeated=grounded.pose(Action::Failure,.6,{200,0});
  for(unsigned i=0;i<held.bones.size();++i)assert(held.bones[i].rotation==repeated.bones[i].rotation);
  grounded.expressions().clear();
  assert(!grounded.settled());
  for(int i=0;i<2400&&!grounded.settled();++i) {
    auto move=grounded.propose(1./120); grounded.realize(grounded.state().rootX+move.displacement);
  }
  assert(grounded.settled());
  std::cout << "independent expressive roles, bounds, gaze, clear and settling passed\n";
}
