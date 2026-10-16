#pragma once
#include "second_order.h"
#include <array>
namespace pets::character {
// Mass-normalized body forces in host units (positive Y down).
struct FlightForces { double lift=0, thrust=0, drag=0; };
// Canonical left-side joint deltas from authored folded rest; mirror right-side presentation.
struct WingState { double shoulder=0, elbow=0, wrist=0, extension=0, strokeVelocity=0, attitude=0; };
class WingMotion {
public:
 void reset();
 void goal(double extension, double effort, double attitude);
 void step(double dt);
 std::array<WingState,2> state() const;
 FlightForces forces(double referenceReach, Vec2 bodyVelocity, double effortLimit=1) const;
 bool settled() const;
private:
 SecondOrderTracker extension_{{3,1,0}}, effort_{{3,1,0}}, stroke_{{9,1,0}}, attitude_{{3,1,0}};
 double extensionGoal_=0, effortGoal_=0, attitudeGoal_=0, phase_=0, strokeVelocity_=0;
};
}
