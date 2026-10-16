#include "wing_motion.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
void WingMotion::reset(){extension_.reset();effort_.reset();stroke_.reset();attitude_.reset();extensionGoal_=effortGoal_=attitudeGoal_=phase_=strokeVelocity_=0;}
void WingMotion::goal(double extension,double effort,double attitude){
 if(!std::isfinite(extension)||!std::isfinite(effort)||!std::isfinite(attitude)||extension<0||extension>1||effort<0||effort>1||std::abs(attitude)>.7)throw std::runtime_error("invalid wing goal");
 extensionGoal_=extension;effortGoal_=effort;attitudeGoal_=attitude;
}
void WingMotion::step(double dt){
 if(!std::isfinite(dt)||dt<0)throw std::runtime_error("invalid wing interval");dt=std::min(dt,1./30);if(dt==0)return;
 const double previous=stroke_.value();extension_.step(dt,extensionGoal_);effort_.step(dt,effortGoal_);attitude_.step(dt,attitudeGoal_);
 phase_=std::remainder(phase_+dt*2*3.141592653589793*4,2*3.141592653589793);
 stroke_.step(dt,.95*effort_.value()*std::sin(phase_));strokeVelocity_=(stroke_.value()-previous)/dt;
 if(settled()){phase_=0;strokeVelocity_=0;}
}
std::array<WingState,2> WingMotion::state()const{
 const double extension=std::clamp(extension_.value(),0.,1.);
 WingState w{stroke_.value(),-1.35*extension,.85*extension,extension,strokeVelocity_,attitude_.value()};return {w,w};
}
FlightForces WingMotion::forces(double reach,Vec2 velocity,double limit)const{
 if(!std::isfinite(reach)||reach<=0||!std::isfinite(velocity.x)||!std::isfinite(velocity.y)||!std::isfinite(limit)||limit<0||limit>1)throw std::runtime_error("invalid wing force input");
 const auto wing=state()[0];
 // Only actual downstroke speed, with extended span, produces active force.
 // Limit is actuation strength (useful for reduced stamina), never hover force.
 const double gravity=18*reach;
 const double lift=std::clamp(std::max(0.,wing.strokeVelocity)*wing.extension*wing.extension*.38*gravity*limit,0.,6*gravity);
 return {lift*std::cos(attitude_.value()),lift*std::sin(attitude_.value()),effort_.value()>1e-5 ? .65 : 0.};
}
bool WingMotion::settled()const{return extension_.settled(extensionGoal_)&&effort_.settled(effortGoal_)&&effortGoal_==0&&stroke_.settled(0)&&attitude_.settled(attitudeGoal_);}
}
