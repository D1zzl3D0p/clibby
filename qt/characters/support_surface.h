#pragma once
#include <optional>
#include <string>
#include <vector>
namespace pets::character {
// All positions are logical desktop pixels, positive Y downward. Geometry is
// unavailable after maxAge; source-qualified identities must not be recycled.
struct SurfaceSegment { double minimum=0, maximum=0; };
struct SupportSurface {
  std::string id;
  double topY=0;
  std::vector<SurfaceSegment> segments;
  double observedAt=0;
  double maxAge=1.25;
};
struct SurfaceDelta { double x=0,y=0; };
std::optional<SurfaceSegment> usableSurface(const SupportSurface &,double rootX,
    double leftFoot,double rightFoot,double now);
std::optional<SupportSurface> descendingSurface(const std::vector<SupportSurface>&,
    double rootX,double leftFoot,double rightFoot,double previousY,double nextY,double now);
std::vector<SurfaceSegment> subtractCovered(SurfaceSegment,std::vector<SurfaceSegment>);
std::optional<SurfaceDelta> surfaceMovement(const SupportSurface& previous,
    const SupportSurface& next,double rootX,double leftFoot,double rightFoot,double now,
    double maximumMovement=64);
}
