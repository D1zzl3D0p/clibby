#pragma once
namespace pets::motion {
// Renderer-independent spatial motion. Planar rigs supply zero depth.
struct Vec3 { double x=0, y=0, z=0; };
struct TwoLinkPose { Vec3 origin, knee, end; };
// Contact mode keeps the endpoint exact with a narrow smooth compliance band
// near full extension, avoiding a straight-knee singularity.
// The default remains rigid for arms and other unconstrained endpoints.
TwoLinkPose solveTwoLink(Vec3 origin, Vec3 target, double upper, double lower,
                         Vec3 bend, bool preserveTarget = false);
}
