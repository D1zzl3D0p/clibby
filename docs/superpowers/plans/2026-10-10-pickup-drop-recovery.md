# Pickup, drop and recovery implementation plan

> **For agentic workers:** Use superpowers:executing-plans for integration and independent subagents for the shared motion and 3D adapter. Steps use checkbox syntax.

**Goal:** Implement the approved pickup, fall, landing and procedural recovery interaction in both resident presentations.

**Architecture:** Extend shared grounded motion with an explicit drop lifecycle and a small Qt-free flight solver. The host realizes planar motion and reports clipping; pose adapters solve detached limbs in flight and pinned contacts after landing.

**Tech Stack:** C++17, Qt Quick, existing analytical IK and axial support springs.

**Spec:** `docs/superpowers/specs/2026-10-10-pickup-drop-recovery-design.md`

## Global constraints

- Keep C++17 and Qt Quick; add no physics-engine dependency.
- Body dynamics, lifecycle and recovery policy are Qt-free and shared by 2D/3D.
- Generate limb goals and solve poses procedurally; use no canned recovery clips.
- Use the existing single application timer. Stop physics deadlines at rest and during suppression.
- Keep `shared/` and the timed benchmark fixture unchanged.
- No Git repository exists here; preserve a file ledger instead of claiming commits.

## Review focus

- Repeated equal-time and stale pointer samples must not manufacture throw velocity.
- Hide/show and native mouse-ungrab must release safely without resetting elevation.
- Pending 3D proposals must not survive pickup or inject old contacts after landing.
- Character changes and display floor changes must preserve reachable presented position.
- Large impacts must not cause stretched limbs, tunneling or an endless active timer.

## Task 1: Shared flight lifecycle and procedural 2D contacts

**Owner:** motion agent; exclusive files `qt/characters/drop_motion.*`,
`qt/characters/drop_tests.cpp`, `qt/characters/grounded.*`.

**Interfaces:** Extend `MotionProposal` with `displacementY` (screen-down units).
Extend `GroundedState` with phase and elevation (positive above floor).
`GroundedController::pickup(double rootX, double elevation)`,
`hold(double rootX, double elevation)`, `release(Vec2 velocity)`,
`reconcile(double rootX, GroundSpan, double floorY, double elevation)`, and
`realize(double rootX, bool blocked=false, std::optional<double> elevation={})`.
Expose anatomical reference reach for adapter release-velocity conversion.

- [x] Write and run failing deterministic checks for elevated falling, impact,
  finite recovery, interrupted pickup and anatomical airborne IK.
- [x] Implement bounded full-gravity flight; keep position continuous and install
  touchdown contacts once. Transfer landing velocity into axial support with a
  bounded body-floor limit. Generate detached limb goals; preserve grounded gait.
- [x] Run new core checks and existing grounded checks. Send API details before
  root and 3D integration; do not edit CMake or host/application files.

```cpp
controller.pickup(200, 160);
controller.release({0, 0});
auto proposal = controller.propose(1.0/60);
check(proposal.displacementY > 0, "elevated release falls");
```

## Task 2: 3D adapter and contact gating

**Owner:** 3D agent; exclusive files `qt/characters3d/pose.*`,
`qt/characters3d/tests.cpp`, `qt/resident/spatial_character.*`.

**Consumes:** Task 1 interfaces, shared lifecycle and elevation. **Produces:**
SpatialCharacter equivalents in logical pixels for pickup, hold, release,
reconcile and vertical realization, plus scaled `MotionProposal`/`GroundedState`.

- [x] Add failing 3D checks for detached anatomical limbs, re-grab and finite
  landing/recovery with contact flags consistent with lifecycle.
- [x] Clear turn replants, reaches and pending proposals on pickup. Convert
  vertical travel through resident projection; fall elevation applies once via
  the window. Use the shared controller, not a second flight integrator.
- [x] Run 3D contact/skin checks and communicate conversion details.

```cpp
motion.pickup(200, 100);
check(!motion.pose().grounded[0] && !motion.pose().grounded[1],
      "held paws do not support the body");
```

## Task 3: Host placement, release and resident scheduling

**Owner:** root; files `qt/resident/desktop_host.*`,
`qt/resident/application.*`, `qt/resident/host_tests.cpp`,
`qt/resident/character_tests.cpp`, `qt/CMakeLists.txt`.

**Consumes:** shared and spatial APIs. **Produces:** actual interactive behavior.

- [x] Add and run a regression showing native-style button release loses height
  under the old host path. Add app checks for fall, interruption and suppression.
- [x] Distinguish real release from cancellation, expose floor geometry and
  two-axis fractional placement. Remove ordinary release/screen reconciliation
  snapping. Estimate throw velocity from finite realized pointer samples.
- [x] Route elevation and proposal Y through the shared controller. Preserve
  physics under suppression, preempt autonomy/reactions while physics is busy,
  update cards/masks, persist only transition boundaries and quiet settling.
- [x] Add build/test targets; run focused host, character and spatial checks.

```cpp
const auto elevated = shell.position();
shell.control(Control::EndDrag);
check(shell.position() == elevated, "release preserves presented position");
```

## Task 4: Review, verification and handoff

**Owner:** root with an independent reviewer.

- [x] Build Release and run full CTest plus `.venv/bin/python` regressions.
- [x] Review cross-file integration against all five Review Focus conditions;
  fix reproducible defects and rerun affected checks.
- [x] Capture procedural drop poses where supported. Record native versus
  offscreen limits without inferring desktop/battery acceptance.
- [x] Update handoff and implementation record with actual results.

## Progress ledger

Specification approved and user explicitly requested implementation in this chat.
User project instructions request independent subagent work; partition files to
avoid shared edits. Interfaces above are shared among Tasks 1–3; root owns CMake
and integration. No Git operations or worktree initialization are appropriate.

Completed 2026-10-11. Shared motion, 3D adaptation, host integration and independent review are complete. Three review defects (partial offscreen release, contact relocation, and touchdown continuity) were fixed and retested. Native drop and hide/show checks pass. See `docs/results/2026-10-11-pickup-drop-recovery.md` for verification and remaining manual checks.

Follow-up tuning: user chose quadratic air resistance over a new hard cap or constant velocity transfer. Ordinary throws retain measured initial speed and use stable speed-squared damping; host realization clips only to real geometry. Extreme 3D throws bound secondary pose intent separately. Numerical stability, distance ordering, continuous decay and host touchdown regressions added; independent review has no open findings.
