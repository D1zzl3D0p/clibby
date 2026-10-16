# Calm curious procedural behavior

The approved direction gives pets occasional self-directed activity while all
limb motion remains procedural. This implements the
[design](../superpowers/specs/2026-10-10-curious-procedural-behavior-design.md)
and [plan](../superpowers/plans/2026-10-10-curious-procedural-behavior.md).

## Delivered behavior

The Qt-free seeded chooser waits 8–20 seconds between activities, then selects
a survey, independent ear twitch, tail swish, supported reach stretch or short
exploration. Profiles differentiate Mallow, Bramble and Werewolf; unsupported
anatomy filters choices. Exploration stays within the supplied usable span,
has a timeout, requires at least 45 seconds between starts and three intervening
choices. Arrival can lead into a survey. Large clock jumps do not replay episodes.

Shared expressive motion accepts bounded role goals, direction-based look and
impulses, tracks each ear independently, and smoothly clears back to neutral.
Spatial reach tracks endpoints and wrist goals before existing measured arm IK.
Production notification gestures also submit procedural goals. Renderers apply
solved snapshots; textures and package version-1 data are unchanged. Existing
planted contacts, body-led navigation and single-foot flight remain authoritative.

The resident includes quiet activity and notification phase deadlines in its
existing single timer. Settled held goals stop frame scheduling. Pause freezes
realized motion; hide, suspend, drag, manual navigation and reactions preempt
autonomy. Resuming autonomous activity starts a fresh quiet interval. The frozen
benchmark keeps its legacy ambient/pose compatibility. Both previews offer
autonomous mode and seed controls; planar preview also exposes expression
primitives. Seeded CLI captures use the production chooser and motion.

## Verification

Fresh Release build succeeded. Full CTest: 25 passed, zero failures, four skipped
(`resident_ipc`, `resident_spaces`, `resident_ipc_cli`, `character3d_gpu_render`).
Python: 50 passed. New checks cover deterministic choices, capability filtering,
preemption, invalid geometry, backward/large clocks, exploration frequency,
role isolation, held-goal settling, smooth reach release and fixed contacts.
Resident checks exercise spontaneous motion before the old ambient deadline,
pause/hidden suppression, notification settling and nongrounded pause continuity.
Existing motion, skin binding, presentation and preview checks pass.

Independent review caught exploration frequency after two intervening choices,
invalid-observation timer churn, held notification frame churn and stale preview
clock state. Those issues are corrected and regression checks pass. Native
socket/GPU/Spaces skips are environment limits, not successful acceptance.

Ignored raw evidence, logs, seeded traces and software captures live under
`results/curious-procedural-2026-10-10/`. Seed-5 chooser timelines reproduced
byte-for-byte over 600 simulated seconds. Mallow/Bramble captures include rest,
survey, independent ear twitch, tail swish, exploration and arrival. Sampled
software states have zero planted-X drift and zero both-airborne frames; ear
motion is intentionally subtle. Captures demonstrate procedural 2D
activities; they do not establish native 3D appearance, desktop parity or battery
impact. Restart the rebuilt resident for visual acceptance. No Git repository
exists in this workspace, so no commits or worktrees were created.
