#include "ik.h"
#include "motion3d.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
TwoLinkSolution solveTwoLink(Vec2 o, Vec2 t, double a, double b, int bend, bool preserveTarget) {
  if (!std::isfinite(o.x) || !std::isfinite(o.y) || !std::isfinite(t.x) ||
      !std::isfinite(t.y) || !std::isfinite(a) || !std::isfinite(b) || a <= 0 ||
      b <= 0 || a > 8192 || b > 8192 || (bend != 1 && bend != -1))
    throw std::runtime_error("invalid two-link input");
  double x=t.x-o.x,y=t.y-o.y;
  if(std::hypot(x,y)<1e-12) { x=0; y=-1; }
  const auto pose=pets::motion::solveTwoLink({o.x,o.y,0},{t.x,t.y,0},a,b,
                                            {-y*bend,x*bend,0}, preserveTarget);
  return {{pose.knee.x,pose.knee.y},{pose.end.x,pose.end.y}};
}
} // namespace pets::character
