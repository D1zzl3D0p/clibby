# Companion polish implementation plan

## Current correction: directional leading plant

- [x] Reproduce body rotation before the leading paw plants; tighten the regression.
- [x] Measure directional world displacement, not just foot-yaw onset.
- [x] Choose a reachable leading step first and derive body follow-through from its plant.
- [x] Keep ordinary placements (2 biped/6 quad), turn time and original walk speed.
- [x] Verify actual rigs, upright reach, contacts, host bounds and interrupted motion.
- [x] Rebuild and inspect native Biscuit and contrasting anatomies; record visual limits.

## Previous candidate: feet lead, body follows (still looked like spinning)

The user rejected the upright point-pivot because it still reads as a spin.
Keep the accepted walking pace/cadence improvements. Replace the turn's
body-first yaw schedule with foot-led support transfers.

- [x] Measure body/foot heading onset, first touchdown and actual support count
  in a fresh all-rig baseline; preserve evidence under ignored results.
- [x] Add a regression that rejects body-leading foot yaw and one-paw quad spins.
- [x] Prototype reachable leading placements and body-following support paths.
  Quadrupeds retain two supports and ordinarily use three coordinated pair transfers
  (five for difficult full reversals);
  bipeds use a leading plant followed by a trailing placement.
- [x] Render the isolated prototype before integrating it into the application.
- [x] Verify upright posture, fixed stance pins, foot-first sequencing, placement
  budget, original speed, host clipping, interrupted turns and quiet settling.
- [x] Rebuild/capture the implemented candidate and record limits; visual feel is
  separate from the geometry checks.

Correction: the user rejected the first gait pass's step count and speed loss.
The research-informed correction restores the original travel pace and replaces
the incremental yaw rounds with finite support pivots and coordinated turns.
See `docs/results/2026-10-17-natural-turning.md` and its linked research report.

> **For agentic workers:** Use the existing scoped workers for independent pickup/art changes; execute shared gait work in this session. Preserve the current dirty checkout and do not commit unrelated work.

**Goal:** Grounded, subtly expressive walking with reachable legs, brisk turns, reliable pickup and connected human shoulders.

**Architecture:** Keep the Qt-free shared planner and spatial adapter, measured anatomy, world-space stance pins and host-realized placement. Scale gait to anatomy, constrain travel to the stance's usable reach, and gate large spatial reversals behind stepping turns. Body weight/push-off follow support and step phase, with no extra timer.

**Tech Stack:** C++17, Qt Quick 3D, procedural Blender/Python assets.

**Spec:** Approved direction in this chat: short steps, subtle weight shifts, quick planted turns before large reversals; fix shared pickup and human shoulder proportions.

## Global constraints

- Preserve existing uncommitted work, frozen shared fixture and raw evidence.
- No new dependencies or per-companion runtime loops/timers.
- Planted feet stay pinned; held/airborne, pause and host clipping retain ownership.
- Keep intentional character silhouette differences. Python checks use `.venv/bin/python`.

## Review focus

- Rapid mid-swing reversals must stay continuous and eventually arrive.
- Short-legged quadrupeds at all supported speeds must retain bounded leg extension.
- Integer host placement and blocked travel must settle.
- Pickup regions must follow head/body poses without swallowing transparent corners.
- Signature arms/props must remain connected after shoulder changes.

### Task 1: Shared gait (primary)

**Files:** `qt/characters/grounded.{h,cpp}`, `qt/characters3d/pose.{h,cpp}`, motion regression tests.
**Interfaces:** Existing propose/realize contract; add bounded travel scaling only if required for spatial turns. Spatial anatomy supplies reach-scaled tuning; planner consumes support geometry.

- [x] Add regressions for short-legged reach, speed range, brisk reversal and readable gait body motion; run to expected failure.
- [x] Scale gait by measured reach and number of sequential contacts, constrain support height before overextension, and keep footholds reachable.
- [x] Gate large heading reversals with brisk stepping turns and preserve mid-swing trajectories.
- [x] Add subtle phase/support-driven weight and push-off; verify arrival and quiet settling.
- [x] Run core and spatial tests, then full suite after integration.

### Task 2: Pickup (pickup worker)

**Files:** `qt/preview3d/resident_scene.cpp`, `qt/preview3d/catalogue_tests.cpp`, `qt/resident/spatial_tests.cpp`.
**Interfaces:** Existing `projectedGrabRegion()` QRegion and host mouse event path.

- [x] Reproduce Biscuit head/torso rejection with a regression.
- [x] Project pose-following core torso/head envelopes, independently of tails/wings/props.
- [x] Verify catalogue regions and actual press/drag/release through the resident host.

### Task 3: Shoulders (proportion worker)

**Files:** `tools/humanoid_asset.py`, humanoid/wayfarer recipes, generated human assets and geometry tests.
**Interfaces:** Existing authoring helper and joint names; no runtime/schema changes.

- [x] Test sleeve seating and connected shoulder/chest geometry.
- [x] Decouple upper-chest shape and shoulder span, reduce sleeve/elbow spread, adjust armour.
- [x] Regenerate twelve human assets and inspect representative source renders.

### Task 4: Integration and delivery (primary)

- [x] Rebuild `build/qt-humanoids/Clibby.app`; run Python and combined Qt suites.
- [x] Capture representative rest/walk/reversal/native-density output, inspect visually and review scoped diff independently.
- [x] Record exact evidence and limitations in a delivery record and handoff.

## Execution notes

Ruling: Work in place because current uncommitted companion/runtime assets are the target baseline. Separate file ownership prevents worker conflicts; no automatic commits or worktree copies.
Ruling: User approval of the proposed motion direction authorizes implementation; no further process approval rounds.
Ruling: Combine anatomy-scaled travel/cadence with a measured support-height ceiling rather than clamping host root placement. This preserves world contacts and host feedback while removing ordinary-walk overextension; the ceiling is fed back into support physics.
