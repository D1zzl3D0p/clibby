#pragma once
#include "character.h"
namespace pets::character {
struct TwoLinkSolution {
  Vec2 knee, end;
};
TwoLinkSolution solveTwoLink(Vec2 origin, Vec2 target, double upper,
                             double lower, int bend, bool preserveTarget = false);
} // namespace pets::character
