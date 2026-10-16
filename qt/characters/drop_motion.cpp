#include "drop_motion.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
namespace {
void valid(double value) {
  if(!std::isfinite(value) || std::abs(value)>1e8)
    throw std::runtime_error("invalid drop input");
}
struct QuadraticStep { Vec2 displacement, velocity; };
QuadraticStep quadraticStep(Vec2 velocity, Vec2 acceleration, double drag, double linearDrag, double dt) {
  // Symmetric force kicks around the exact combined linear/quadratic damping
  // flow. Positive damping cannot reverse even extreme incoming velocities.
  const Vec2 midpoint{velocity.x+acceleration.x*dt*.5,
                      velocity.y+acceleration.y*dt*.5};
  const double speed=std::hypot(midpoint.x,midpoint.y), rate=drag*speed;
  const double decayTime=linearDrag>0 ? -std::expm1(-linearDrag*dt)/linearDrag : dt;
  const double factor=std::exp(-linearDrag*dt)/(1+rate*decayTime);
  const double distanceTime=rate>0 ? std::log1p(rate*decayTime)/rate : decayTime;
  return {{midpoint.x*distanceTime,midpoint.y*distanceTime},
          {midpoint.x*factor+acceleration.x*dt*.5,
           midpoint.y*factor+acceleration.y*dt*.5}};
}
}
DropMotion::DropMotion(double reach) : reach_(reach) {
  valid(reach); if(reach<=0) throw std::runtime_error("invalid drop anatomy");
}
void DropMotion::reset() {
  phase_=DropPhase::Grounded; elevation_=impact_=phaseTime_=0; velocity_={};
  gravityScale_=54; flightForceReach_=reach_;
  quadraticDrag_=true;
}
void DropMotion::pickup(double elevation) {
  valid(elevation);
  phase_=DropPhase::Held; elevation_=std::max(0.,elevation);
  velocity_={}; impact_=phaseTime_=0;
}
void DropMotion::hold(double elevation) {
  valid(elevation); elevation_=std::max(0.,elevation);
}
void DropMotion::release(Vec2 velocity) {
  valid(velocity.x); valid(velocity.y);
  gravityScale_=54; flightForceReach_=reach_;
  quadraticDrag_=true;
  velocity_=velocity; impact_=phaseTime_=0;
  phase_=(elevation_>0 || velocity.y<0) ? DropPhase::Airborne : DropPhase::Grounded;
  if(phase_==DropPhase::Grounded) velocity_={};
}
void DropMotion::launch(Vec2 velocity) {
 if(phase_!=DropPhase::Grounded)throw std::runtime_error("launch needs grounded body");
 release(velocity);
 useFlightGravity();
}
void DropMotion::useFlightGravity(double referenceReach) {
 valid(referenceReach);if(referenceReach<0)throw std::runtime_error("invalid flight reference");
 flightForceReach_=referenceReach>0?referenceReach:reach_;
  gravityScale_=18*flightForceReach_/reach_;
  quadraticDrag_=false;
}
FlightStep DropMotion::advance(double dt, FlightForces forces) {
  for(double value:{forces.lift,forces.thrust,forces.drag})valid(value);
  // Match WingMotion's multiplication order at the exact force ceiling.
  const double maximumForce=6*(18*std::max(reach_,flightForceReach_));
  if(forces.lift<0||forces.drag<0||forces.drag>20||forces.lift>maximumForce||std::abs(forces.thrust)>maximumForce)throw std::runtime_error("flight force outside bounds");
  valid(dt); if(dt<0) throw std::runtime_error("negative drop interval");
  dt=std::min(dt,1./30);
  FlightStep result;
  if(phase_!=DropPhase::Airborne || dt==0) return result;
  if(quadraticDrag_) {
    // Ordinary throws retain their measured initial velocity. Aerodynamic
    // acceleration is -k |v| v; anatomy scales k, never the release velocity.
    const double drag=.015/reach_;
    const int count=std::max(1,int(std::ceil(dt*240)));
    const double h=dt/count;
    for(int step=0;step<count && phase_==DropPhase::Airborne;++step) {
      const Vec2 acceleration{forces.thrust,gravityScale_*reach_-forces.lift};
      auto sample=quadraticStep(velocity_,acceleration,drag,forces.drag,h);
      double sweepEnd=h;
      auto furthest=sample;
      if(velocity_.y>0 && acceleration.y<0 && sample.velocity.y<=0) {
        // Strong lift can reverse descent within one substep. Sweep the
        // descending portion too, even if the endpoint is already ascending.
        double lo=0,hi=h;
        for(int iteration=0;iteration<52;++iteration) {
          const double middle=(lo+hi)*.5;
          if(quadraticStep(velocity_,acceleration,drag,forces.drag,middle).velocity.y>0)lo=middle;
          else hi=middle;
        }
        sweepEnd=lo;
        furthest=quadraticStep(velocity_,acceleration,drag,forces.drag,sweepEnd);
      }
      const bool touchdown=furthest.displacement.y>=elevation_ && furthest.velocity.y>0;
      if(touchdown) {
        double lo=0,hi=sweepEnd;
        for(int iteration=0;iteration<52;++iteration) {
          const double middle=(lo+hi)*.5;
          if(quadraticStep(velocity_,acceleration,drag,forces.drag,middle).displacement.y>=elevation_)hi=middle;
          else lo=middle;
        }
        sample=quadraticStep(velocity_,acceleration,drag,forces.drag,hi);
        sample.displacement.y=elevation_;
      }
      result.displacementX+=sample.displacement.x;
      result.displacementY+=sample.displacement.y;
      velocity_=sample.velocity;
      elevation_=std::max(0.,elevation_-sample.displacement.y);
      if(touchdown) {
        result.impactVelocity=velocity_.y;
        impact_=std::clamp(velocity_.y/(12*reach_),0.,1.);
        phase_=DropPhase::Landing;phaseTime_=0;velocity_={};
      }
    }
    return result;
  }
  if(forces.lift!=0 || forces.thrust!=0 || forces.drag!=0) {
    const int count=std::max(1,int(std::ceil(dt*240)));
    const double h=dt/count;
    for(int step=0;step<count&&phase_==DropPhase::Airborne;++step) {
      const double ax=forces.thrust-forces.drag*velocity_.x;
      const double ay=gravityScale_*reach_-forces.lift-forces.drag*velocity_.y;
      const double predicted=velocity_.y*h+.5*ay*h*h;
      double elapsed=h;
      const bool touchdown=predicted>=elevation_ && velocity_.y+ay*h>0;
      if(touchdown) {
        double lo=0,hi=h;
        for(int i=0;i<36;++i){double mid=(lo+hi)*.5;if(velocity_.y*mid+.5*ay*mid*mid>=elevation_)hi=mid;else lo=mid;}
        elapsed=hi;
      }
      const double dy=velocity_.y*elapsed+.5*ay*elapsed*elapsed;
      result.displacementX+=velocity_.x*elapsed+.5*ax*elapsed*elapsed;
      result.displacementY+=dy;
      velocity_.x+=ax*elapsed;
      velocity_.y+=ay*elapsed;
      elevation_=std::max(0.,elevation_-dy);
      if(touchdown){result.impactVelocity=velocity_.y;impact_=std::clamp(velocity_.y/(12*reach_),0.,1.);phase_=DropPhase::Landing;phaseTime_=0;velocity_={};}
    }
    return result;
  }
  // Exact constant-gravity integration and descending crossing prevent floor
  // tunnelling. Walking's contact-loss gravity and drag never enter flight.
  const double gravity=gravityScale_*reach_;
  const double crossing=(-velocity_.y+std::sqrt(velocity_.y*velocity_.y+2*gravity*elevation_))/gravity;
  const double elapsed=std::min(dt,crossing);
  result.displacementX=velocity_.x*elapsed;
  result.displacementY=velocity_.y*elapsed+.5*gravity*elapsed*elapsed;
  velocity_.y+=gravity*elapsed;
  elevation_=std::max(0.,elevation_-result.displacementY);
  if(crossing<=dt) {
    elevation_=0;
    result.impactVelocity=velocity_.y;
    impact_=std::clamp(velocity_.y/(12*reach_),0.,1.);
    phase_=DropPhase::Landing; phaseTime_=0;
    // Vertical momentum is handed off to axial support by the adapter now.
    velocity_={};
  }
  return result;
}
void DropMotion::realizeElevation(double elevation, bool checkCeiling, double ceilingTolerance) {
  valid(elevation);valid(ceilingTolerance);
  if(ceilingTolerance<0)throw std::runtime_error("negative presentation tolerance");
  // Subpixel host feedback is expected; a larger upward shortfall is a ceiling.
  if(checkCeiling && phase_==DropPhase::Airborne && velocity_.y<0 && elevation<elevation_-ceilingTolerance)
    velocity_.y=0;
  elevation_=std::max(0.,elevation);
  if(phase_==DropPhase::Held) return;
  if(elevation_>1e-7 && phase_!=DropPhase::Airborne) {
    gravityScale_=54; flightForceReach_=reach_;
    quadraticDrag_=true;
    phase_=DropPhase::Airborne; velocity_={}; impact_=phaseTime_=0;
  } else if(elevation_==0 && phase_==DropPhase::Airborne && velocity_.y>=0) {
    impact_=std::clamp(velocity_.y/(12*reach_),0.,1.);
    phase_=DropPhase::Landing; phaseTime_=0;
    velocity_={};
  }
}
void DropMotion::shiftReference(double deltaFloor) { valid(deltaFloor); elevation_=std::max(0.,elevation_+deltaFloor); }
void DropMotion::blockHorizontal() { velocity_.x=0; }
void DropMotion::recover(double dt, bool settled) {
  valid(dt); if(dt<0) throw std::runtime_error("negative recovery interval");
  dt=std::min(dt,1./30); phaseTime_+=dt;
  if(phase_==DropPhase::Landing && phaseTime_>=.08) {
    phase_=DropPhase::Recovering; phaseTime_=0;
  } else if(phase_==DropPhase::Recovering) {
    impact_*=std::exp(-6*dt);
    if(impact_<.00001) impact_=0;
    if(phaseTime_>=.1 && impact_==0 && settled) {
      phase_=DropPhase::Grounded; phaseTime_=0;
    }
  }
}
} // namespace pets::character
