#pragma once
#include "character.h"
namespace pets::character {
class SecondOrderTracker {
public:
  explicit SecondOrderTracker(SecondaryTuning tuning = {});
  void setTuning(SecondaryTuning);
  void reset(double value = 0);
  void impulse(double velocity);
  double step(double dt, double target);
  double value() const { return value_; }
  bool settled(double target) const;

private:
  SecondaryTuning tuning_;
  double value_ = 0, velocity_ = 0, previous_ = 0;
};
} // namespace pets::character
