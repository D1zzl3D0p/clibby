#pragma once
#include "expression.h"
#include "limb_array.h"
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pets::character {
struct Vec2 {
  double x = 0, y = 0;
};
struct Transform {
  double x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1;
};
struct Bone {
  std::string id;
  int parent = -1;
  Transform rest;
  double minRotation = -3.141592653589793, maxRotation = 3.141592653589793;
};
struct Rect {
  int x = 0, y = 0, width = 0, height = 0;
};
struct Attachment {
  int bone = 0;
  Rect source;
  Vec2 pivot, offset;
  int drawOrder = 0;
};
struct Contact {
  int bone = 0;
  Vec2 point;
};
struct SecondaryTuning {
  double frequency = 3, damping = .8, response = 0;
};
struct LegChain {
  int hip = -1, knee = -1, ankle = -1;
  Vec2 contact;
  int bend = 1;
};
enum class Gait { Biped, QuadrupedWalk };
struct Locomotion {
  LimbArray<LegChain> legs;
  Gait gait = Gait::Biped;
  double groundY = 0, speed = 38, acceleration = 150;
  double reachThreshold = 10, stride = 14, lift = 6;
  double minStepDuration = .12, maxStepDuration = .28;
  SecondaryTuning secondary;
};
struct Definition {
  std::string id, name, controller;
  int width = 96, height = 96;
  Vec2 origin; // Placement/contact metadata in canvas coordinates; not a render
               // offset.
  std::vector<Bone> bones;
  std::vector<Attachment> attachments;
  std::map<std::string, int> bindings;
  std::vector<Vec2> interaction;
  std::vector<Contact> contacts;
  double motionEnvelope = 12;
  std::optional<Locomotion> locomotion;
};
struct Pose {
  Vec2 root;
  std::vector<Transform> bones;
  Vec2 desiredRootMotion;
  // Appearance-only eyelid closure; leaves joints and contacts untouched.
  double eyeOpenness = 1;
}; // Desired motion is units/second; host realizes placement separately.
enum class Action { Rest, Idle, Walk, Attend, Celebrate, Failure, Acknowledge };
// Row-major affine matrix: x'=m00*x+m01*y+tx; y'=m10*x+m11*y+ty.
struct WorldTransform {
  double m00 = 1, m01 = 0, m10 = 0, m11 = 1, tx = 0, ty = 0;
  Vec2 apply(Vec2 p) const;
};
// Every rest-world affine coefficient (including translation) must be finite
// and within +/-1e8. This conservative range is safe for float scene matrices.
// Arbitrary poses use the same bound; controllers fall back to validated rest
// when an action or contact correction would exceed it.
void validateDefinition(const Definition &definition);
std::vector<WorldTransform> worldTransforms(const Definition &, const Pose &);
// Attachment-only limb extension and eyelid closure; joint/contact transforms
// remain unchanged. Nonfinite eye openness falls back to fully open eyes.
std::vector<WorldTransform> artworkTransforms(const Definition &, const Pose &);
class Controller {
public:
  explicit Controller(Definition definition);
  // Idle/Walk/Attend use elapsed seconds; transient reactions use normalized
  // elapsed [0,1] seconds and return to the idle base at 1. Gaze is a local
  // direction, not a globally observed pointer position.
  Pose pose(Action action, double time, Vec2 gaze = {}) const;
  // Production actions submit persistent procedural targets. Legacy pose-only
  // callers retain the frozen preview/fixture contract until they opt in.
  void setAction(Action action, double time = 0);
  ExpressionMotion &expressions() { procedural_ = true; return expressions_; }
  const ExpressionMotion &expressions() const { return expressions_; }
  void impulse(Vec2 direction, double magnitude = 1);
  // At most four 1/120-second substeps; excess elapsed time is discarded.
  // Offsets below .0005 and velocity below .002 snap to exact settled zero.
  void step(double dt);
  void resetDynamics();
  bool settled() const;
  const Definition &definition() const { return definition_; }

private:
  Definition definition_;
  Vec2 displacement_, velocity_;
  ExpressionMotion expressions_;
  bool procedural_ = false;
  Action action_ = Action::Rest;
  std::uint32_t reactionRoles_ = 0;
};
} // namespace pets::character
