#pragma once
#include "motion3d.h"
#include <array>
#include <cstddef>
namespace pets::motion {
// All vectors use a common spatial frame with positive Y pointing down.
// A planar adapter supplies zero depth; loads are confirmed contact weights.
struct SupportLeg {
  Vec3 hip, ankle;
  double upper=0, lower=0, load=0;
  // Compliant neutral length; zero uses the full anatomical chain.
  double restLength=0;
};
struct SupportState { double offset=0, velocity=0, acceleration=0; };
struct SupportDynamics {
  double stiffness=120, damping=24, gravity=0;
  double uprightStiffness=0, uprightDamping=0, uprightTarget=0;
  double contactLossGravityScale=1, airborneDamping=0;
};
double neutralRestLength(Vec3 hip, Vec3 ankle, double supportedWeight,
                         double stiffness);
// Character stiffness retains the aggregate strength of two full-load legs.
// Only two or four contacts are supported; totalWeight is shared across them.
double neutralSupportRestLength(Vec3 hip, Vec3 ankle, double totalWeight,
                                double stiffness, std::size_t contactCount);
double supportAcceleration(const std::array<SupportLeg,2>&,
                           const SupportDynamics&, double offset, double velocity);
void stepSupport(SupportState&, const std::array<SupportLeg,2>&,
                 double dt, const SupportDynamics&);
double supportAcceleration(const std::array<SupportLeg,4>&,
                           const SupportDynamics&, double offset, double velocity);
void stepSupport(SupportState&, const std::array<SupportLeg,4>&,
                 double dt, const SupportDynamics&);
}
