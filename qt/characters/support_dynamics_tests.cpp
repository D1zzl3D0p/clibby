#include "motion_support.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace pets::motion;
namespace {
void near(double actual, double expected, const char* label) {
  if(!std::isfinite(actual)||std::abs(actual-expected)>1e-10)
    throw std::runtime_error(label);
}
template<class F> void rejects(F action, const char* label) {
  try { action(); } catch(const std::runtime_error&) { return; }
  throw std::runtime_error(label);
}
template<std::size_t N> std::array<SupportLeg,N> legs() {
  std::array<SupportLeg,N> result;
  for(auto& leg:result) leg={{0,0,0},{0,1,0},.6,.6,1,1.1};
  return result;
}
template<std::size_t N> void invalidStepLeavesState(
    const std::array<SupportLeg,N>& contacts, double dt,
    const SupportDynamics& dynamics, SupportState state, const char* label) {
  const auto before=state;
  rejects([&]{stepSupport(state,contacts,dt,dynamics);},label);
  auto same=[](double a,double b){return a==b||(std::isnan(a)&&std::isnan(b));};
  if(!same(state.offset,before.offset)||!same(state.velocity,before.velocity)||
     !same(state.acceleration,before.acceleration)) throw std::runtime_error("rejection mutated state");
}
}

int main() {
  const double nan=std::numeric_limits<double>::quiet_NaN();
  const double inf=std::numeric_limits<double>::infinity();
  SupportDynamics d; d.stiffness=100; d.damping=10; d.gravity=20;
  auto two=legs<2>(); auto four=legs<4>();
  // Each biped leg supplies 100*.1=10; damping adds 10*2=20 per leg.
  near(supportAcceleration(two,d,0,0),0,"legacy equilibrium");
  near(supportAcceleration(two,d,0,2),-40,"legacy damping");
  near(supportAcceleration(four,d,0,0),0,"four-contact equilibrium");
  near(supportAcceleration(four,d,0,2),-40,"four-contact damping normalization");
  // Unit 3-4-5 direction: .2 compression projects by .8, producing 16 per leg.
  auto tilted=two;
  for(auto& leg:tilted) {leg.ankle={.6,.8,0}; leg.restLength=1.2;}
  near(supportAcceleration(tilted,d,0,0),-12,"legacy tilted projection");
  auto partial=four;
  for(auto& leg:partial) leg.restLength=1;
  partial[2].load=partial[3].load=0;
  d.contactLossGravityScale=.5; d.airborneDamping=8;
  near(supportAcceleration(partial,d,0,0),15,"half of four contacts gravity");
  d.damping=0;
  near(supportAcceleration(partial,d,0,2),7,"half of four contacts air drag");
  for(auto& leg:partial) leg.load=0;
  near(supportAcceleration(partial,d,0,2),-6,"zero-contact fall damping");
  d.damping=10;
  for(std::size_t count:{2u,4u})
    near(neutralSupportRestLength({0,0,0},{0,1,0},20,100,count),1.1,"neutral calibration");
  near(neutralRestLength({0,0,0},{0,1,0},10,100),1.1,"legacy neutral calibration");
  for(std::size_t count:{0u,1u,3u,5u})
    rejects([&]{neutralSupportRestLength({0,0,0},{0,1,0},20,100,count);},"unsupported count accepted");
  for(double weight:{-1.,-std::numeric_limits<double>::denorm_min(),nan,inf})
    rejects([&]{neutralSupportRestLength({0,0,0},{0,1,0},weight,100,4);},"invalid weight accepted");
  for(double stiffness:{0.,-1.,nan,inf})
    rejects([&]{neutralSupportRestLength({0,0,0},{0,1,0},20,stiffness,4);},"invalid stiffness accepted");
  rejects([&]{neutralSupportRestLength({nan,0,0},{0,1,0},20,100,4);},"invalid neutral hip accepted");
  rejects([&]{neutralSupportRestLength({0,0,0},{0,inf,0},20,100,4);},"invalid neutral ankle accepted");
  rejects([&]{neutralSupportRestLength({0,0,0},{1,0,0},20,100,4);},"non-downward neutral accepted");
  const SupportState initial{.02,.1,7};
  for(double dt:{-.01,.25,nan,inf}) invalidStepLeavesState(four,dt,d,initial,"invalid dt accepted");
  auto stationary=initial;
  stepSupport(stationary,four,0,d);
  near(stationary.offset,.02,"zero dt offset"); near(stationary.velocity,.1,"zero dt velocity");
  near(stationary.acceleration,7,"zero dt acceleration");
  // Every invalid fourth-leg field must reject even before a zero-length step.
  for(int field=0;field<9;++field) {
    auto bad=four;
    switch(field) {
      case 0:bad[3].hip.x=nan;break; case 1:bad[3].ankle.z=inf;break;
      case 2:bad[3].upper=0;break; case 3:bad[3].lower=-1;break;
      case 4:bad[3].load=nan;break; case 5:bad[3].load=-.1;break;
      case 6:bad[3].load=1.1;break; case 7:bad[3].restLength=nan;break;
      case 8:bad[3].restLength=-1;break;
    }
    rejects([&]{supportAcceleration(bad,d,0,0);},"invalid fourth contact accepted");
    invalidStepLeavesState(bad,0,d,initial,"invalid fourth contact accepted at zero dt");
    invalidStepLeavesState(bad,.01,d,initial,"invalid fourth contact accepted at positive dt");
  }
  for(auto badState:{SupportState{nan,.1,7},SupportState{.02,inf,7},SupportState{.02,.1,nan}})
    invalidStepLeavesState(four,0,d,badState,"nonfinite state accepted");
  for(int field=0;field<4;++field) {
    auto bad=d;
    if(field==0) bad.stiffness=0;
    if(field==1) bad.gravity=nan;
    if(field==2) bad.damping=-1;
    if(field==3) bad.contactLossGravityScale=0;
    invalidStepLeavesState(four,0,bad,initial,"invalid dynamics accepted");
  }
  rejects([&]{supportAcceleration(four,d,nan,0);},"nonfinite offset accepted");
  rejects([&]{supportAcceleration(four,d,0,inf);},"nonfinite velocity accepted");
  SupportState biped=initial,quadruped=initial;
  for(int i=0;i<120;++i) {
    stepSupport(biped,two,1./120,d); stepSupport(quadruped,four,1./120,d);
    near(quadruped.offset,biped.offset,"integrated offset normalization");
    near(quadruped.velocity,biped.velocity,"integrated velocity normalization");
    near(quadruped.acceleration,biped.acceleration,"integrated acceleration normalization");
  }
  if(std::abs(biped.offset-initial.offset)<1e-6) throw std::runtime_error("integration did not advance");
  // Initial compression supplies 20, plus damping 2, against gravity 20: a=-2.
  SupportState reference{.02,.1,7};
  stepSupport(reference,two,1./240,d);
  near(reference.acceleration,-2,"legacy one-step acceleration");
  near(reference.velocity,.09166666666666667,"legacy one-step velocity");
  near(reference.offset,.020381944444444445,"legacy one-step offset");
  std::cout<<"PASS support dynamics: normalization, legacy forces, calibration, validation and integration\n";
}
