#pragma once
#include "character.h"
namespace pets::character {
// Visual-only branch: leg ancestry and world contacts remain untouched.
void applyFacing(const Definition &, Pose &, double direction);
}
