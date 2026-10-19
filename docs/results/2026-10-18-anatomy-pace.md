# Anatomy-derived walking pace — 2026-10-18

Approved behavior: characters use roughly the same stepping rhythm, with leg
length determining travel speed. The saved speed control remains a multiplier.

The spatial motion adapter now sets natural ground speed to 0.85 times the
shortest measured hip–knee–ankle chain length per second, bounded to the existing
planner's 0.1–500 cm/s range. This calibration keeps human-sized companions near
their previous pace. The shortest chain retains the existing conservative reach
policy for unequal limbs. Both bipeds and quadrupeds use the rule, including
winged ground walkers. Flight is unchanged.

Stride length already scales with measured reach. The existing swing-duration
calculation divides reach by selected travel speed, so uniformly scaling a rig
now changes travel distance while retaining its stepping rhythm. Gait-specific
coordination, acceleration, contact planting and turn timing remain in place.
The multiplier still changes both travel pace and the corresponding gait timing.
The Toolbox tooltip describes the natural-pace multiplier.

Verification:

- The new proportional-travel regression failed against fixed-speed motion
  before implementation, then passed with the anatomy-derived pace.
- Scaled anatomy checks cover 60%, 100% and 140% legs, all four selected pace
  settings (0.5, 1, 2 and 3), bipeds, winged bipeds and quadrupeds. They check
  actual steady travel, stepping counts, multiplier ratios and fixed plants.
- Existing short-rig arrival/reversal/drop checks use travel budgets appropriate
  to the new slower pace. Obsolete universal 70/140 cm/s assertions now check
  anatomy-relative pace; useful stride checks are normalized by leg length.
- Full Release/app build completed; bundle relocation passed. The Qt deployment
  tool emitted missing optional SQL driver dependency diagnostics, but the build
  returned success and the subsequent relocated application check passed.
- Combined Qt suite: 88 passed, five environment skips, zero failures (93 total).
  Skips: resident_ipc, resident_ipc_cli, menu_bar_icon, resident_spaces,
  character3d_gpu_render. Raw log: ignored
  `results/anatomy-pace-2026-10-18/qt-suite.log`.
- Python suite: 109 passed using `.venv/bin/python -m unittest discover -s tests -q`.
- Independent scoped code review found no actionable defects.

An initial suite was started before app packaging finished; its relocation
failure is superseded by the complete successful post-build run above. The two
old fixed-pace test failures were updated for the approved behavior.

Use rebuilt `build/qt-humanoids/Clibby.app`. Native visual feel has not been
assessed in this change; automated motion checks do not establish appearance,
desktop parity or battery impact. No commit or push; preexisting work preserved.
