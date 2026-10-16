#include "second_order.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace pets::character {
namespace {
void check(bool valid) {
  if (!valid)
    throw std::runtime_error("invalid second-order input");
}
} // namespace
SecondOrderTracker::SecondOrderTracker(SecondaryTuning t) { setTuning(t); }
void SecondOrderTracker::setTuning(SecondaryTuning t) {
  check(std::isfinite(t.frequency) && t.frequency >= .1 && t.frequency <= 30 &&
        std::isfinite(t.damping) && t.damping >= .1 && t.damping <= 4 &&
        std::isfinite(t.response) && std::abs(t.response) <= 4);
  tuning_ = t;
}
void SecondOrderTracker::reset(double v) {
  check(std::isfinite(v) && std::abs(v) <= 8192);
  value_ = previous_ = v;
  velocity_ = 0;
}
void SecondOrderTracker::impulse(double v) {
  check(std::isfinite(v));
  velocity_ = std::clamp(velocity_ + v, -100., 100.);
}
bool SecondOrderTracker::settled(double t) const {
  return std::abs(value_ - t) < .0005 && std::abs(velocity_) < .002;
}
double SecondOrderTracker::step(double dt, double target) {
  check(std::isfinite(dt) && dt >= 0 && std::isfinite(target) &&
        std::abs(target) <= 8192);
  dt = std::min(dt, 1. / 30);
  if (dt == 0)
    return value_;
  double derivative = std::clamp((target - previous_) / dt, -100., 100.);
  previous_ = target;
  int count = std::max(1, int(std::ceil(dt * 120)));
  double h = dt / count;
  const double pi = 3.141592653589793,
               k1 = tuning_.damping / (pi * tuning_.frequency),
               k2 = 1 / std::pow(2 * pi * tuning_.frequency, 2),
               k3 = tuning_.response * tuning_.damping /
                    (2 * pi * tuning_.frequency);
  double stable = std::max({k2, h * h / 2 + h * k1 / 2, h * k1});
  for (int i = 0; i < count; i++) {
    value_ = std::clamp(value_ + h * velocity_, -8192., 8192.);
    velocity_ = std::clamp(
        velocity_ +
            h * (target + k3 * derivative - value_ - k1 * velocity_) / stable,
        -100., 100.);
  }
  if (settled(target)) {
    value_ = target;
    velocity_ = 0;
  }
  return value_;
}
} // namespace pets::character
