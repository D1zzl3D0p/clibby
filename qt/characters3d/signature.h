#pragma once
#include "characters/motion3d.h"
#include <string>
#include <string_view>
#include <vector>

namespace pets::character3d {
// Neutral model glTF axes (Y up, Z forward): additive Euler radians after the
// ordinary expression, meters from rest. The binding converts to joint space.
// Signature channels never own the body root or any support-chain joint.
struct SignatureOffset {
 std::string joint;
 pets::motion::Vec3 rotation;
 pets::motion::Vec3 translation;
};
class SignatureMotion {
public:
 explicit SignatureMotion(std::string_view asset);
 bool supported() const;
 std::string_view name() const;
 std::vector<std::string_view> requiredJoints() const;
 bool start();
 void cancel();
 void reset();
 // Zero freezes; negative or nonfinite intervals throw before any mutation.
 void advance(double dt);
 bool active() const;
 std::vector<SignatureOffset> offsets() const;
private:
 int profile_=-1;
 double elapsed_=0, cancelElapsed_=0;
 bool active_=false, cancelling_=false;
 std::vector<SignatureOffset> cancelPose_, cancelVelocity_;
 std::vector<SignatureOffset> evaluate(double elapsed) const;
};
}
