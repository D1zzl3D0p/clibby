#include "reaction_motion.h"
#include <algorithm>
#include <cmath>

namespace pets {
Pose reactionPose(const Pose& rest, BehaviorIntent intent, double progress) {
    Pose pose = rest;
    if (!std::isfinite(progress) || progress <= 0 || progress >= 1) return pose;
    constexpr double pi = 3.141592653589793;
    const double envelope = std::sin(pi * progress);
    switch (intent) {
    case BehaviorIntent::Acknowledge: pose.root.ry() += 3 * envelope * std::sin(4 * pi * progress); break;
    case BehaviorIntent::Attend: pose.root.rx() += 5 * envelope * std::sin(8 * pi * progress); break;
    case BehaviorIntent::Celebrate: pose.root.ry() -= 14 * envelope * std::abs(std::sin(6 * pi * progress)); break;
    case BehaviorIntent::ExpressFailure:
        pose.root.ry() += 7 * envelope;
        pose.root.rx() += 2 * envelope * std::sin(12 * pi * progress);
        break;
    }
    return pose;
}
}
