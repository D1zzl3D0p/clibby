# Planar Quadruped Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Deliver an original minimal Marmalade cat using genuine four-paw procedural locomotion in the existing planar preview and resident, with biped compatibility and a rollback checkpoint.

**Architecture:** Extend bounded anatomy/state to two or four active legs. Keep the existing proposal/realization transaction, shared count-aware force solver and two-link IK; add an explicit four-beat admission policy and all-contact aggregate queries. The spatial adapters remain two-legged and continue using their existing APIs.

**Tech Stack:** C++17, Qt 6.10.3, Qt-free character core, Pillow authoring, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-16-quadruped-foundation-design.md` and its catalogue-architecture parent.

## Global Constraints

- C++17; no new dependencies, processes or animation timers.
- Preserve all existing two-leg manifests, APIs and regressions; frozen `shared/` is unchanged.
- Supported walking anatomy is exactly two or four legs, not arbitrary leg counts.
- Four named contacts normalize to `foreLeft`, `foreRight`, `hindLeft`, `hindRight`.
- Ordinary walk cycle: hindLeft → foreLeft → hindRight → foreRight; at most one swing and three supporting contacts for admission.
- Stance soles remain fixed until toe-off; navigation follows host-realized coordinates.
- Support stiffness/damping and neutral calibration use the completed count-aware solver, without double normalization.
- Pack content remains data-only; all existing loader bounds and validation remain active.
- Use `rtk` for shell commands and `.venv/bin/python` for Python checks.
- New captures/logs live under a new ignored `results/` run directory.

## Review Focus

- Reversed manifest order must normalize into stable anatomical identities; missing/duplicate roles and ancestor-overlapping chains reject.
- Active iteration/state snapshots must never expose spare capacity as phantom feet, including old spatial adapters.
- Fore/hind proximal geometry must turn with the body while planted sole endpoints remain fixed.
- A narrow surface fitting only two paws must reject the complete four-paw footprint; moving/fractional supports and dropping retain all contacts.
- Blocked/quantized motion, regrab and suppression must not leave a swing or motion deadline running forever.

## Agreed internal seam

`pets::character::LimbArray<T>` in `qt/characters/limb_array.h` is fixed-capacity four, default active count two. It provides checked `setCount(std::size_t)` accepting only 2/4; `size()`, active `begin/end`, indexed access, `fill(const T&)`, and `template<std::size_t N> std::array<T,N> array() const` (reject count mismatch). Support initialization/assignment from initializer lists with exactly 2/4 values and from `std::array<T,2/4>` so current fixtures compile. No per-frame allocation. `setCount` must value-initialize newly active elements. Existing nested brace initializers in controller fields will be replaced with count-preserving fill calls as necessary.

`enum class Gait { Biped, QuadrupedWalk };` belongs in character.h. `Locomotion::gait` defaults to Biped; `Locomotion::legs` becomes `LimbArray<LegChain>`. Canonical storage index is the role identity; no per-frame role-string parsing. Existing LegChain fields stay unchanged. Definition validation requires matching gait/count and independent chains.

`GroundedState::feet` becomes `LimbArray<FootState>`. Provide `bool anyFootSwinging() const` and `GroundSpan soleExtent() const` on GroundedState, returning active contact min/max world X without sole-radius margin. The resident subtracts rootX and applies its existing two-unit margin. GroundedController's existing std::array<...,2> support setters remain; new array<...,4> overloads reject anatomy-count mismatch. No public session API change.

## Task 1: Bounded anatomy and validated four-role packages

**Files:** create `qt/characters/limb_array.h`; modify `character.h`, `character.cpp`, `loader.cpp`, `tests.cpp`.

**Interfaces:** produces the agreed seam above for Task 2. Loader consumes optional `locomotion.gait` and per-chain JSON `role`; returns normalized Locomotion.

- [x] Write tests first for active 2/4 iteration/copy semantics, malformed counts, canonical role normalization, invalid gait/count, missing/duplicate roles, shared/ancestor-overlapping chains, nonfinite tuning and unchanged legacy package loading. Use a synthetic valid 4-chain manifest/atlas with deliberately shuffled JSON order.
- [x] Run red verification proving the missing four-leg feature, rather than a setup failure.
- [x] Implement bounded storage and additive parsing. Omitted gait means biped/exactly two. Explicit biped optional roles must match left/right index; quadrupedWalk requires all four named roles. Preserve existing numeric/geometry/resource validation.
- [x] Run focused character tests; inspect two-leg fixture compile compatibility. Coordinate shared builds with Task 2; do not race full builds.

## Task 2: Four-contact grounded planner and pose integration

**Files:** modify `qt/characters/grounded.h`, `grounded.cpp`; create `quadruped_tests.cpp`. Other files only with coordinator agreement.

**Interfaces:** consumes Task 1 seam and existing array2/4 support APIs; produces GroundedState aggregates and four-legged GroundedController. Existing public planner methods/signatures remain.

- [x] Write deterministic synthetic-quad behavior tests before implementation. Cover all four eventual steps, one ordinary swing, three confirmed supporting feet, independent stance pins under quantized/blocked realization, finite unequal-chain poses, actual rendered sole endpoints matching pins, rest/travel reversal and active-count isolation. Include drop/regrab, changing support floor/bounds and eventual settled state.
- [x] Run expected red verification, recording diagnostic/output.
- [x] Replace walking-state pairs with count-aware bounded storage, preserving biped policy. Initialize count from immutable anatomy, preserve it on resets/fill and state reads. Keep wing/ear/arm pairs unchanged. Use conservative four-beat policy; finish existing swing before admitting another, and avoid a new gait clock at rest.
- [x] Use active-count neutral calibration and mean body lowering; dispatch correct count-aware force overload without double scaling. Generalize pickup/drop/recovery and all-foot settling. Bound every installed touchdown to current sole interval. Reject unsupported quadruped powered-flight requests; explicit hopping remains outside initial capability scope.
- [x] Turn quadruped proximal offsets with facing about body anchor without mirroring world pins. Evaluate reach and IK using the same transformed proximal geometry. Keep family-specific branch logic explicit and preserve legacy biped output.
- [x] Pass focused quadruped and existing grounded/drop/flight/spatial tests. Register target through coordinator-owned CMake. Prefer tests that exercise observable contacts and settling over private counters.

## Task 3: Original Marmalade reference art

**Files:** create `tools/author_marmalade.py`, `characters/marmalade/` assets and `tests/test_marmalade.py`; no edits to other author recipes.

**Interfaces:** consumes manifest role/gait contract from Task 1. Produces deterministic layered atlas, manifest, preview, attribution and MIT license. ID `marmalade`, name `Marmalade`, built-in articulated controller, gait quadrupedWalk, four independent two-link chains.

- [x] Test reproducible generation, four independent chains/contacts, valid bounds/fold tuning and separate eye art; inspect generated preview. Art recipe is geometry, not animation frames.
- [x] Generate a readable warm ginger side-view cat, with horizontal torso, visible fore/hind paws, ears, face and tail. Separate visual facing branch from IK ancestry. Start with standing/walking anatomy; no seated/flop claims.
- [x] Validate through the actual C++ loader/planner once Tasks 1/2 land, not only Python structural assertions. Match rendered limb endpoints to chain geometry. Keep far/near leg layering readable.
- [x] Register deterministic source check and capture real procedural rest/walk/reverse/drop with existing preview tooling. New captures go to ignored results, source preview stays in package.

## Task 4: Resident integration, full verification and checkpoint

**Files:** modify `qt/resident/application.cpp`, `qt/resident/character_tests.cpp`, `qt/resident/surface_host_tests.cpp` where required; `qt/CMakeLists.txt`, `characters/README.md`, `docs/characters-v1.md`, handoff/architecture and new result record.

**Interfaces:** consumes GroundedState aggregate queries and packaged cat; no host placement protocol change.

- [x] Add resident-level tests for four-foot arrival/suppression/selection, complete footprint on a narrow surface and support movement/loss. Preserve the current live pet after invalid selection. Confirm actual foot extents, not just two central feet.
- [x] Replace hard-coded two-foot arrival/footprint aggregation with active aggregates. Keep menu/pack discovery and spatial whitelist behavior unchanged.
- [x] Build combined and 2D-only configurations; run full CTest, Python suite, deterministic cat source check and frozen golden reference verification. Record skipped/native limitations explicitly.
- [x] Inspect native-size software captures for rest/walk/reversal/held/drop and update docs with implemented versus open acceptance. Native desktop/energy claims require separate evidence.
- [x] Independent whole-change code review; address important findings; create a local implementation checkpoint after checks. Baseline `2dbd7c1` on main remains unchanged.

## Execution record

The user explicitly authorized implementation and Git initialization. Baseline `2dbd7c1` captures the existing state; current branch `feat/quadruped-planar` isolates implementation. Work remains in the existing checkout to reuse the configured SDK/builds. Tasks 1 and 2 have disjoint file owners but a fixed shared interface; Task 3 is independent art work; coordinator owns Task 4 and build scheduling. No push, publication or destructive rollback is part of this task.
