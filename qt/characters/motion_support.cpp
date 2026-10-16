#include "motion_support.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::motion {
namespace {
bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
template<std::size_t N>
void validate(const std::array<SupportLeg,N>& legs,const SupportDynamics& d,
              double offset,double velocity){
  static_assert(N==2||N==4,"support requires two or four contacts");
  for(double value:{d.stiffness,d.damping,d.gravity,d.uprightStiffness,
                    d.uprightDamping,d.uprightTarget,d.contactLossGravityScale,d.airborneDamping,offset,velocity})
    if(!std::isfinite(value))throw std::runtime_error("nonfinite support input");
  if(d.stiffness<=0||d.damping<0||d.gravity<0||d.uprightStiffness<0||d.uprightDamping<0||d.contactLossGravityScale<=0||d.contactLossGravityScale>1||d.airborneDamping<0)
    throw std::runtime_error("invalid support dynamics");
  for(const auto& leg:legs)
    if(!finite(leg.hip)||!finite(leg.ankle)||!std::isfinite(leg.upper)||
       !std::isfinite(leg.lower)||leg.upper<=0||leg.lower<=0||
       !std::isfinite(leg.load)||leg.load<0||leg.load>1||
       !std::isfinite(leg.restLength)||leg.restLength<0)
      throw std::runtime_error("invalid support leg");
}
template<std::size_t N>
double acceleration(const std::array<SupportLeg,N>& legs,const SupportDynamics& d,
                    double offset,double velocity){
  double support=0,load=0;
  const double stiffness=d.stiffness*(2./N),damping=d.damping*(2./N);
  for(const auto& leg:legs){
    const double dx=leg.ankle.x-leg.hip.x,dy=leg.ankle.y-leg.hip.y,dz=leg.ankle.z-leg.hip.z;
    const double length=std::max(1e-9,std::hypot(std::hypot(dx,dy),dz));
    const double rest=leg.restLength>0?leg.restLength:leg.upper+leg.lower;
    const double compression=std::max(0.,rest-length);
    load += leg.load;
    if(dy>0)
      support+=leg.load*std::max(0.,stiffness*compression*dy/length+damping*velocity);
  }
  const double contactFraction=std::clamp(load/N,0.,1.);
  const double gravity=d.gravity*(d.contactLossGravityScale+
      (1-d.contactLossGravityScale)*contactFraction);
  const double airDrag=d.airborneDamping*(1-contactFraction)*std::max(0.,velocity);
  return gravity-support-airDrag-d.uprightStiffness*(offset-d.uprightTarget)-d.uprightDamping*velocity;
}
template<std::size_t N>
void integrate(SupportState& state,const std::array<SupportLeg,N>& legs,
               double dt,const SupportDynamics& dynamics){
  validate(legs,dynamics,state.offset,state.velocity);
  if(!std::isfinite(state.acceleration))throw std::runtime_error("nonfinite support state");
  if(!std::isfinite(dt)||dt<0||dt>1./30+1e-12)throw std::runtime_error("invalid support interval");
  if(dt==0)return;
  const int count=std::max(1,int(std::ceil(dt*240)));
  const double h=dt/count,initial=state.offset;
  for(int step=0;step<count;++step){
    auto current=legs;
    for(auto& leg:current)leg.hip.y+=state.offset-initial;
    state.acceleration=acceleration(current,dynamics,state.offset,state.velocity);
    state.velocity+=state.acceleration*h;
    state.offset+=state.velocity*h;
  }
}
}
double neutralRestLength(Vec3 hip,Vec3 ankle,double supportedWeight,double stiffness){
  if(!finite(hip)||!finite(ankle)||!std::isfinite(supportedWeight)||supportedWeight<0||
     !std::isfinite(stiffness)||stiffness<=0)
    throw std::runtime_error("invalid neutral support input");
  const double dx=ankle.x-hip.x,dy=ankle.y-hip.y,dz=ankle.z-hip.z;
  const double span=std::hypot(std::hypot(dx,dy),dz);
  if(dy<=1e-9||span<=1e-9)throw std::runtime_error("neutral ankle must be below its hip");
  return span+supportedWeight*span/(stiffness*dy);
}
double neutralSupportRestLength(Vec3 hip,Vec3 ankle,double totalWeight,
                                double stiffness,std::size_t contactCount){
  if(contactCount!=2&&contactCount!=4)
    throw std::runtime_error("support requires two or four contacts");
  // Validate before division: a negative subnormal weight can round to -0.
  if(!std::isfinite(totalWeight)||totalWeight<0)
    throw std::runtime_error("invalid neutral support input");
  return neutralRestLength(hip,ankle,totalWeight/contactCount,stiffness*(2./contactCount));
}
double supportAcceleration(const std::array<SupportLeg,2>& legs,
                           const SupportDynamics& dynamics,double offset,double velocity){
  validate(legs,dynamics,offset,velocity);
  return acceleration(legs,dynamics,offset,velocity);
}
void stepSupport(SupportState& state,const std::array<SupportLeg,2>& legs,
                 double dt,const SupportDynamics& dynamics){
  integrate(state,legs,dt,dynamics);
}
double supportAcceleration(const std::array<SupportLeg,4>& legs,
                           const SupportDynamics& dynamics,double offset,double velocity){
  validate(legs,dynamics,offset,velocity);
  return acceleration(legs,dynamics,offset,velocity);
}
void stepSupport(SupportState& state,const std::array<SupportLeg,4>& legs,
                 double dt,const SupportDynamics& dynamics){
  integrate(state,legs,dt,dynamics);
}
}
