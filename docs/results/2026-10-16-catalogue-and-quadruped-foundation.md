# Catalogue architecture and first quadruped foundation

## Outcome

All 18 expansion concepts now have a [shared architecture map](../superpowers/specs/2026-10-16-catalogue-architecture-design.md), including anatomy families, action dependencies, art/representation policy, compatibility and delivery order. A separate [quadruped design](../superpowers/specs/2026-10-16-quadruped-foundation-design.md) specifies the first walking reference: Marmalade, a conservative four-beat planar cat, pending the user's optional reference-pet preference. Both designs received an independent builder check with no important questions remaining.

Implementation has started with the deliberately smaller [support-dynamics task](../superpowers/plans/2026-10-16-quadruped-support-dynamics.md). The Qt-free support solver now accepts explicit two- or four-contact arrays, uses count-aware neutral calibration, and normalizes aggregate spring/damping response. Existing two-contact entry points remain intact. The new four-contact path has no per-step heap allocation or new dependencies.

This is **not yet a walking quadruped**. Package loading, planner state, admission, poses, turning, drop/recovery and resident foot aggregation still require the subsequent changes defined by the quadruped spec. Current pets and manifests remain biped-based. No new character artwork or 3D quadruped adapter was produced in this substage.

## Force contract

For active count N=2 or 4, effective per-leg stiffness and damping use 2/N. Neutral supported weight uses gravity/N, with effective stiffness applied once. Contact fraction is the summed confirmed loads divided by N. At N=2 the original formulas are preserved; at N=4 extra feet do not simply double aggregate force.

The new calibration helper validates weight before division, including negative subnormal values that could otherwise become negative zero. Integration now explicitly rejects nonfinite incoming acceleration alongside existing offset/velocity validation before changing state. This is a deliberate input-validation tightening; finite biped behavior is unchanged.

## Evidence

The missing APIs were first exercised by new tests, yielding feature-specific compilation failures. A later negative-subnormal regression first failed at runtime, then passed after pre-division validation. New tests use hand-derived expectations for equilibrium, damping, tilted biped geometry, partial/zero support and a biped integration step. They also cover malformed fourth-leg inputs, invalid count/state/timestep, rejection without mutation, zero elapsed time and equal two/four-contact trajectories.

| Verification | Result |
| --- | --- |
| Pre-change combined CTest | 38 passed, four skipped, one failure: `resident_raven_idle3d` |
| Pre-change 2D-only CTest | 28 passed, three skipped, zero failures |
| Focused `support_dynamics` target and direct executable | Passed |
| Combined consumer build | Passed, no warning/error diagnostics in final build logs |
| Final combined CTest | **40 passed, four skipped, zero failures** (44 total) |
| Rebuilt 2D-only configuration | Passed |
| Final 2D-only CTest | **29 passed, three skipped, zero failures** (32 total) |
| Python regression suite | **53 passed** |
| Frozen golden poses | Passed in both CTest configurations |
| Independent code review | No Critical, Important or Minor findings |
| New design relative links | Resolve |

The pre-existing 3D raven idle failure did not recur after the complete rebuild. No raven code was changed, and this work does not claim to have diagnosed or fixed that intermittent baseline condition.

Combined skips: `resident_ipc`, `resident_spaces`, `resident_ipc_cli`, `character3d_gpu_render`. The 2D-only configuration skips the first three. These are not passing native/transport acceptance. No new native visual or energy acceptance was performed.

Commands: `rtk proxy cmake --build build/qt -j 4`; `rtk proxy ctest --test-dir build/qt --output-on-failure`; `rtk proxy cmake --build build/qt-raven-2d-only --parallel 4`; `rtk proxy ctest --test-dir build/qt-raven-2d-only --output-on-failure`; `rtk proxy .venv/bin/python -m unittest discover -s tests -v`. The optional helper regression outside `tests/` was not rerun because its source and consumers were unchanged.

Local evidence and pre-change snapshots: `results/quadruped-foundation-2026-10-16/`. The focused red/green logs and implementation report live there; final CTest logs are copied there after the suite runs. This working directory has no Git repository, so no worktree or commit is claimed.

## Decisions and next slice

The user authorized starting quadruped support, and project instructions reject redundant approval rounds for authorized work. The bounded force-law task therefore proceeded after design review. The full catalogue remains a proposed staged design, rather than an assertion that every detail has received user approval.

Marmalade and 2D-first delivery are my defaults; changing the reference animal does not invalidate the count-aware support work. The cost of a different choice is reference art/tuning and possibly species-specific posture scope. Biped art work for Rivet, Paper Heron and Acorn can proceed independently from shared planner work with explicit file ownership.

Next implementation slice: bounded two/four active limb storage, canonical quadruped role validation, deterministic four-beat admission, contact-preserving planar turning and drop/recovery, all-foot resident aggregates, then an original minimal cat. Preserve existing biped/raven and spatial behavior throughout. Spatial quadruped bindings/art follow separately; do not expand the first loader to arbitrary leg counts.

CPU/RSS, offscreen tests and solver math do not establish battery impact or desktop parity. Keep native gait/readability, everyday surfaces, monitor/Spaces behavior and energy acceptance separate.
