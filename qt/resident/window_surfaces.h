#pragma once
#include "characters/support_surface.h"
namespace pets {
struct WindowSurfaceSnapshot {
  bool available=false;
  std::vector<character::SupportSurface> surfaces;
};
// Geometry only, front-to-back visible window list. Never requests permissions.
WindowSurfaceSnapshot observeWindowSurfaces(double nowSeconds);
}
