# Quadruped Support Dynamics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Start quadruped implementation with tested count-aware support forces and neutral calibration while preserving the existing biped API.

**Architecture:** Add explicit four-contact overloads to the Qt-free support solver and keep two-contact callers unchanged. Normalize effective leg stiffness/damping and contact fraction centrally; validate each active contact, and provide count-aware neutral calibration. This completes a support-dynamics substage, not a walking quadruped.

**Tech Stack:** C++17, existing Qt-free `pet_character_core`, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-16-quadruped-foundation-design.md`, especially Support normalization. Catalogue parent: `2026-10-16-catalogue-architecture-design.md`.

## Global Constraints

- C++17; no new dependencies or runtime processes.
- Qt-free motion core; no per-step heap allocation.
- Effective per-leg stiffness `k * 2/N`, damping `d * 2/N`, contact fraction `sum(load)/N`, and neutral supported weight `gravity/N`.
- For N=2 preserve current tuning and public signatures.
- All shell commands use `rtk`; Python verification uses `.venv/bin/python`.
- New evidence goes in new ignored `results/` directories, never historical evidence paths.
- No manifest/gait/3D quadruped capability is claimed in this substage.

## Review Focus

- Invalid fourth-leg geometry/load must reject, including when timestep is zero.
- A lost contact must reduce support fraction relative to four contacts, not a hard-coded denominator of two.
- Four contacts must preserve aggregate stiffness/damping and neutral equilibrium, rather than doubling body response.
- Existing two-contact equations and spatial callers must retain their behavior.
- Nonfinite/invalid timestep, state, count and calibration inputs must reject without mutating caller state.

## Workspace and authority

The user explicitly requested starting quadruped support and the project instructions prohibit extra approval rounds for already-authorized work. The first substage is bounded to support mathematics; the written design received an independent builder check with no important questions left. The optional reference-pet preference is irrelevant to support forces. The workspace has no `.git`; work in place with pre-change snapshots and do not claim worktrees or commits.

## Task 1: Add count-aware support dynamics and prove compatibility

**Files:**
- Modify: `qt/characters/motion_support.h`, `qt/characters/motion_support.cpp`
- Create: `qt/characters/support_dynamics_tests.cpp`
- Modify: `qt/CMakeLists.txt` (one Qt-free test target)

**Interfaces:**
- Consumes: existing `SupportLeg`, `SupportDynamics`, `SupportState`, `Vec3` and two-contact functions.
- Produces: existing two-contact API unchanged, plus
  `double supportAcceleration(const std::array<SupportLeg,4>&, const SupportDynamics&, double offset, double velocity)`;
  `void stepSupport(SupportState&, const std::array<SupportLeg,4>&, double dt, const SupportDynamics&)`;
  `double neutralSupportRestLength(Vec3 hip, Vec3 ankle, double totalWeight, double stiffness, std::size_t contactCount)`.
- The calibration helper accepts only count 2 or 4, calls the existing neutral solver with weight `totalWeight/N` and stiffness `stiffness*2/N`, and rejects invalid count/inputs.

- [x] **Step 1: Write focused tests before production changes.** Register executable `support_dynamics_tests` and CTest name `support_dynamics`. Hand-derived fixture: vertical unit span, restLength=1.1, full load, k=100, d=10, gravity=20. Two and four identical contacts both give acceleration 0 at velocity 0; both give -40 at downward velocity 2. At quarter-second-invalid dt reject without changing state. Zero-contact case with contactLossGravityScale=.5, airborneDamping=8, velocity=2 gives acceleration -6. A four-leg fixture with two full/ two zero loads, zero compression/damping, gravity=20, contactLossGravityScale=.5 gives acceleration 15. Assert with tolerance 1e-10. A calibration fixture with unit vertical span, totalWeight=20 and k=100 yields rest length 1.1 for both supported counts. Also cover finite inputs, invalid counts, zero k, invalid fourth contact, dt=0, and equal 2/4-contact integrated trajectories from offset=.02/velocity=.1 over 120 steps of 1/120 with tolerance 1e-10. Test the legacy two-contact reference acceleration independently, including tilted geometry if useful.
- [x] **Step 2: Build/run red verification.** Build the focused target. Expected: feature-specific failure because four-contact overload/calibration does not exist; preserve the diagnostic. Do not mistake missing compiler/Qt configuration for the intended failure.
- [x] **Step 3: Implement only these interfaces.** Use internal count-specialized helpers (only N=2 or N=4), preserve validation before state updates, and factor validation/integration without duplicating the force equation. Scale stiffness/damping once, contact fraction once. Preserve the old neutral-rest API unchanged. No loader or planner migration in this task.
- [x] **Step 4: Verify focused behavior.** Build `support_dynamics_tests`; run `rtk proxy ctest --test-dir build/qt -R '^support_dynamics$' --output-on-failure`. Expected: one CTest target passes. Check actual executable output and no warnings.
- [x] **Step 5: Verify consumers and review.** Rebuild the combined configuration and run full CTest. Build/test the existing 2D-only configuration too. Record any failures by name against the pre-change baseline. Fresh independent review covers the whole code change and spec compliance. Document status and next steps; no commits possible without Git.

## Subsequent work

The next separate implementation plan covers bounded planner anatomy, loader roles, gait admission, four-leg turn/drop handling, resident aggregate queries and a minimal cat. The full quadruped design remains the contract; this intentionally small first task must not be reported as completion of that whole slice.
