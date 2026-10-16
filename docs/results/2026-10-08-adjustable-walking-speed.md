# Adjustable walking speed

Superseded by [longer steps at the same rhythm](2026-10-08-stride-distance-correction.md):
the user clarified distance should increase while cadence stays fixed.

The user confirmed the previous corrections work, but the motion still feels
robotic and requested roughly 2–3 times the speed or a way to adjust it. This
change provides Walking speed from 0.5× to 3× in the resident Toolbox and both
character previews. New resident settings and previews start at 2×. The resident
saves its choice across restarts and character changes; previews start fresh,
with `--speed MULTIPLIER` available for repeatable captures. The 3D preview also
has an Original 1× button.

This scales real navigation, not the entire animation clock. Grounded speed
scales by the multiplier, acceleration by its square, and swing/double-support
durations and planning horizons by its inverse. Facing and follower timing are
coordinated; follower drive is normalized so faster travel does not simply
increase ear/tail/torso amplitude. Wave/reaction time remains unchanged. Contact
reach continues to limit realized speed when necessary.

Live changes preserve root, destination, foot positions and current swing phase.
The 3D replant phase is preserved too: a regression caught a turn-foot jump when
speed changed during a replant. Settings reject nonfinite/out-of-range values.
No new timer or idle animation is added. The resident retains its existing quiet
runtime deadline and stops animation when paused or settled.

Tests compare realized travel at 1×, 2× and 3× in both motion cores, check loaded
production contacts at 3×, and exercise persistence, UI wiring, character swap,
invalid values, mid-walk/mid-turn changes and quiet states. The 2D CLI captures
verify greater travel at identical elapsed time and record `walking_speed`.
The resident control screenshot is in `results/walking-speed-control/`.

Speed is now tunable; whether a chosen setting looks natural remains a visual
judgment. This does not claim faster walking alone resolves every robotic pose,
or establish native GPU/resource/battery acceptance.

Fresh full Release build passed: 19 Qt checks passed, 3 environment skips
(two socket checks and native GPU rendering); 50 Python tests passed.
