#pragma once
#include "motion.h"

namespace pets {
enum class BehaviorIntent { Acknowledge, Attend, Celebrate, ExpressFailure };
// Local pose offsets only: no window placement and no integration identifiers.
Pose reactionPose(const Pose& rest, BehaviorIntent intent, double progress);
}
