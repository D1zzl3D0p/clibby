#pragma once
#include "character.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
// Semantic intent; bodies without arms use their existing head/body reactions.
enum class Gesture { Look, Wave, Point, Present, Nod, Success, Failure };
inline Action gestureAction(Gesture gesture) {
  switch (gesture) {
  case Gesture::Nod: return Action::Acknowledge;
  case Gesture::Success: return Action::Celebrate;
  case Gesture::Failure: return Action::Failure;
  default: return Action::Attend;
  }
}
// One second envelope, exact zero at both boundaries. No scheduling ownership.
inline double gestureEnvelope(double elapsed) {
  if (!std::isfinite(elapsed)) throw std::runtime_error("invalid gesture time");
  return elapsed <= 0 || elapsed >= 1 ? 0 : std::sin(elapsed * 3.141592653589793);
}
struct UtilityAnchors {
  Vec2 head, card, effect;
  std::optional<Vec2> leftGrip, rightGrip;
};
// Canvas coordinates; callers apply their existing canvas-to-window transform.
// Text and controls remain conventional 2D UI, never part of character artwork.
inline UtilityAnchors utilityAnchors(const Definition &d, const Pose &p) {
  auto world = worldTransforms(d, p);
  auto head = d.bindings.find("head");
  int bone = head == d.bindings.end() ? d.bindings.at("body") : head->second;
  Vec2 center = world[bone].apply({});
  double top = center.y;
  for (const auto &a : d.attachments) {
    int ancestor = a.bone;
    while (ancestor >= 0 && ancestor != bone) ancestor = d.bones[ancestor].parent;
    if (ancestor != bone) continue;
    for (double x : {a.offset.x - a.pivot.x, a.offset.x - a.pivot.x + a.source.width})
      for (double y : {a.offset.y - a.pivot.y, a.offset.y - a.pivot.y + a.source.height})
        top = std::min(top, world[a.bone].apply({x,y}).y);
  }
  UtilityAnchors anchors{center, {center.x, top - 6}, {center.x, top - 2}, {}, {}};
  auto grip = [&](const char *name) -> std::optional<Vec2> {
    auto found = d.bindings.find(name);
    return found == d.bindings.end() ? std::nullopt : std::optional<Vec2>(world[found->second].apply({}));
  };
  anchors.leftGrip = grip("gripLeft"); anchors.rightGrip = grip("gripRight");
  return anchors;
}
} // namespace pets::character
