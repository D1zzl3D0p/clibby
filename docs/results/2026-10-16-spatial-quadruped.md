# Live woodland cat and spatial quadruped support

The selected B woodland cat is now an original skinned built-in 3D character,
`marmalade-3d` (Marmalade 3D), selectable through the existing resident catalogue.
Its 96×96 nearest-rendered source uses the approved broad body/haunches, compact
head, cheek planes, sparse bands and curved tail. The original 2D Marmalade is
preserved. Production Qt lighting is brighter than the initial native capture
and remains an approximation of the offline Blender study, not an identical
palette-mapped render.

## Implementation

- Spatial motion now uses bounded two/four-leg collections and explicit gait,
  retaining paired expressive arms/wings/ears. Four canonical chains consume
  the existing conservative shared gait and support dynamics.
- Every active paw has a world contact, heading and support state. Walking,
  one-at-a-time turn replants, pickup/drop/recovery, support bounds and reconcile
  account for all four. Runtime support footprint includes hind contacts.
- Quadruped IK derives fore/hind knee poles from the authored bend plane. Review
  caught reversed fore elbows with the old shared forward pole; rejecting the
  authored pole along its rest chain also prevents within-stride sign flips.
- The binding validates the trusted rig, applies body expressions before
  positioning solved limb endpoints, supports eye blinks and omits human arm
  reach capability. Measured skin hulls drive cat interaction bounds.
- Original Blender recipe exports continuous weighted leg/tail meshes, one
  nearest atlas, 22 joints and 2,066 triangles. No animation clips or Blender
  dependency exist in the resident. Balsam resources follow the existing optional
  build/install path. Old rigs with no gait retain biped behavior.

## Verification and evidence

Baseline combined suite before implementation: 42 passed, four environment
skips. Final combined Release suite: **48 passed, four skips** (52 total),
including the new core, rig, preview, resident-stage and selection tests. Final
2D-only Release: **31 passed, three skips** (34 total). Python unittest discovery:
**57 passed**. After final cat-lighting adjustment and strengthened hind-foot
footprint test, the affected three resident/presentation checks passed again.

Red/green observations included missing four-leg API, old loader rejection of a
valid cat, absent-gait compatibility and authored fore/hind bend preservation.
Independent review found no remaining actionable issues after the knee-pole fix.

Native Qt captures are under ignored
`results/marmalade-live-3d-2026-10-16/`. Initial darker captures are preserved;
`lighting-final/` contains rest/walk/turn/expression/held/drop/landed frames and
120 frames of actual Qt motion, with recorded state and native 96-pixel versions.
`marmalade-live.gif` presents the eight-second sequence with native light/dark
samples; identical rest frames are merged by GIF encoding while retaining time.
`states.png` shows the six principal motion states. Captures were inspected.
Live transformed and projected soles match all four authoritative contacts in
both headings, through expression and drop. Quiet tests show no root motion;
float quaternion rounding is tolerated at 1e-5.

These native captures establish the renderer's pixels, not everyday desktop
placement, monitor/Spaces acceptance or energy behavior. Full manual interaction
polish remains open. No battery-impact or resource improvement is claimed.

## Use

Restart the rebuilt `build/qt/better_pets` and choose **Marmalade 3D**, or launch
it explicitly with `--character characters/marmalade-3d`. The existing 2D-only build
continues to reject unavailable spatial selection while retaining the current pet.
The preserved art studies and preceding Git checkpoints remain available for rollback.
