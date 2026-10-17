# Woodland cat spatial quadruped implementation plan

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans task by task.

**Goal:** Make the selected woodland cat a selectable live 3D pet with four procedural legs.

**Architecture:** Extend the existing spatial adapter to bounded two/four-leg anatomy, using the existing shared planner and host feedback. Export the approved original artwork as a skinned GLB and convert it with Balsam. Keep presentation, scheduling and desktop integration shared with existing pets.

**Tech stack:** C++17, Qt Quick 3D 6.10.3, Blender authoring, Python build tools.

**Spec:** `docs/superpowers/specs/2026-10-16-quadruped-foundation-design.md` (spatial stage), `docs/results/2026-10-16-marmalade-art-direction.md` (selected B art).

## Global constraints

- Canonical four-role order: foreLeft, foreRight, hindLeft, hindRight.
- Preserve existing two-legged behavior and optional 2D-only builds.
- No package code, per-frame readback, extra scheduling timers or frozen-fixture changes.
- Maintain selected B silhouette and approximately 96×96 art density; native review remains separate from offscreen tests.
- Commands use `rtk`; Python checks use `.venv/bin/python`; generated captures stay in ignored results.

## Review focus

- Turning or host-blocked travel must retain every planted world sole.
- Forelegs must never inherit arm gestures, reaches or biped hanging anatomy.
- Four-foot footprints must reject narrow supports and survive moving support/reconcile.
- Parent head/torso expression transforms must not displace rendered soles after IK.
- Selection, remembered state, malformed built-ins and 2D-only failure must preserve the current pet.

## Task 1: Spatial anatomy and contact motion

**Files:** `qt/characters3d/pose.{h,cpp}`, `quadruped_tests.cpp`.

**Interface:** Motion accepts `LimbArray<Leg>`, existing optional arms/wings parameters and trailing `Gait gait=Biped`. Pose foot/leg collections have the same active count. Quadruped uses the shared four-beat planner with measured spatial IK and independent world contacts.

- [x] Add failing count/gait, walk, reversal, stationary face, pin, support-frame and drop tests.
- [x] Replace walking-foot pairs with active limb storage, preserving expressive pairs and old inputs.
- [x] Generalize yaw admission/replants and support normalization to all active feet.
- [x] Verify old spatial tests plus new tests; correct any contact or settling defects.

## Task 2: Skinned original woodland artwork and binding

**Files:** `tools/author_marmalade_3d.py`, `tools/convert_marmalade_qt.py`, `characters/marmalade-3d/`, `qt/preview3d/controller.{h,cpp}`, `quadruped_binding_tests.cpp`.

**Interface:** Built-in id `marmalade-3d`, asset `marmalade`; rig gait `quadrupedWalk`, skin `MarmaladeSkin`, contact keys canonical roles; deform chains `thigh_ROLE`, `shin_ROLE`, `foot_ROLE`.

- [x] Add loader rejection/acceptance and no-arm capability tests before binding implementation.
- [x] Export continuous weighted leg meshes and original body/head/ears/tail/eye anatomy in standard existing model coordinates.
- [x] Convert nearest atlas with addressable joints; validate GLB and measured skin bounds.
- [x] Bind all four chains, preserve blinks/expressions and verify rendered soles against solved contacts in both headings.

## Task 3: Resident delivery, captures and regression

**Files:** `qt/characters3d/CMakeLists.txt`, `qt/preview3d/{ResidentStage.qml,resident_scene.cpp,main.cpp}`, `qt/resident/spatial_character.cpp`, new resident quadruped tests, documentation.

- [x] Wire optional assets/resources, preview selection and fixed built-in descriptor allowlist.
- [x] Use native 96×96 nearest source and calibrated stage; aggregate four projected contact bounds.
- [x] Verify live stage joints, pixel setup, quiet settling, travel/reversal, drop, blink and selection; preserve native calibration.
- [x] Capture actual Qt rest/walk/turn/held/drop, inspect at native size and compare chosen B.
- [x] Run combined and 2D-only Release suites plus Python, review changes independently, fix findings, save Git checkpoint and handoff.

The user explicitly authorized proceeding; work continues on the existing feature branch and its rollback checkpoints without an additional approval round.
