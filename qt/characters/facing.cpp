#include "facing.h"
#include <algorithm>
#include <cmath>
namespace pets::character {
void applyFacing(const Definition &d, Pose &p, double direction) {
  auto visual = d.bindings.find("facing");
  if (visual == d.bindings.end() || !std::isfinite(direction)) return;
  direction = std::clamp(direction, -1., 1.);
  const double side = std::abs(direction);
  // Preserve the blob's readable volume through the front-facing midpoint.
  p.bones.at(visual->second).scaleX = std::copysign(.82 + .08 * side, direction);
  auto head = d.bindings.find("head");
  if (head != d.bindings.end())
    p.bones.at(head->second).x = d.bones.at(head->second).rest.x * side;
  auto eyes = d.bindings.find("eyes");
  if (eyes != d.bindings.end())
    p.bones.at(eyes->second).x += 8 * side;
  auto tail = d.bindings.find("tail");
  if (tail != d.bindings.end()) {
    auto &t = p.bones.at(tail->second);
    t.x *= side;
    t.scaleX *= std::max(.05, side);
  }
}
}
