# Raven Flight and Shared Perching Implementation Plan

> **For agentic workers:** Use subagents with explicit file ownership and independent review. Execution is authorized by the user in this chat.

**Goal:** Deliver matched 2D/3D ravens with procedural hopping and lift-driven flight, and support window-top placement/walking for every grounded pet.

**Architecture:** Extend the existing Qt-free airborne owner and grounded adapters. A shared procedural wing state generates forces and poses; identified surfaces enter through a native provider and the existing host. Renderers consume solved motion only.

**Tech Stack:** C++17, Qt Quick/optional Quick3D, Python/Pillow and Blender offline authoring, macOS native window geometry.

**Spec:** ../specs/2026-10-10-raven-flight-and-perching-design.md

## Global constraints

- Keep C++17 and existing optional 3D builds; no physics-engine dependency.
- Shared root dynamics and physical wing state; no renderer-owned flap clocks.
- Keep ordinary walking's hard contacts and alternating swing admission.
- Support surfaces are shared by every pet; reachability is capability-specific.
- Preserve frozen fixtures, APIs, utility UI and existing evidence.
- Use the existing timer and suppression policies; native/energy claims require native evidence.
- Prefix commands with rtk; use .venv/bin/python for Python tests.
- This directory has no Git repository; retain files and reports without claiming commits.

## Review focus

1. Insufficient wing force must cause descent, including near a target altitude.
2. Window removal and movement cannot leave stale world contacts or teleport pets.
3. Suppression and re-grabbing cannot preserve active stale force commands.
4. Moving/clipped host placement must be realized once, preserving fractional motion.
5. Missing wings and 2D-only builds must retain existing behavior and selection.

## Task 1: Shared wings, hop and powered airborne motion

Owner: motion worker. Files: qt/characters/{drop_motion,grounded,expression,character,loader}.*, new wing/flight core and tests as needed. No resident or 3D edits.

- [x] Add and run failing tests for explicit hop launch, zero-force drops, bounded force integration, wing force depending on realized stroke, hover failure with insufficient effort, contact detachment/reacquisition and unsupported anatomy.
- [x] Extend DropMotion as the sole airborne integrator, retaining passive drop behavior.
- [x] Add bounded wing state/goal/stroke core shared by force evaluation and poses.
- [x] Expose GroundedController hop, flight-target, landing and wing state interfaces; publish contracts to integration workers before their implementation depends on them.
- [x] Bind 2D shoulder/elbow/wrist art to solved wing state and validate optional anatomy.
- [x] Run scoped core/drop/grounded checks and report exact commands/results.

## Task 2: Matched original assets and spatial presentation

Owner: asset/spatial worker. Files: tools/author_raven*, tools/convert_raven*, characters/raven*, qt/characters3d/*, qt/preview3d/*, qt/resident/spatial_character.*. No root CMake or motion-core edits.

- [x] Author editable layered 2D raven with articulated wings, bird-specific legs, valid atlas, manifest and original-work license.
- [x] Author Blender source/skinned GLB/atlas and rig metadata for corresponding 3D raven.
- [x] Generalize trusted 3D binding/scene selection beyond required wolf joints; preserve wolf regression behavior.
- [x] Consume shared wing and airborne state in spatial poses, never integrate separate lift or flap clocks.
- [x] Expose equivalent hop/flight/land controls in spatial adapter/preview.
- [x] Validate asset counts, rig bindings, finite solved poses and existing spatial tests; capture comparison stills where supported.

## Task 3: Shared support edges and native window provider

Owner: surface worker. Files: new qt/characters/support_surface.*, new qt/resident/window_surfaces*, qt/resident/desktop_host.*, new surface/host tests. No application, existing motion-core or CMake edits.

- [x] Add failing tests for one-way descending acquisition, moving relative anchors, narrow/covered segments, stale/removal policy and placement by nonflying pets.
- [x] Add bounded Qt-free surface observations and selection.
- [x] Implement native macOS geometry provider, excluding own/off-Space/minimized windows and covered edge segments; stub unsupported platforms.
- [x] Generalize host floor/span queries and motion feedback to a selected surface; support manual placement and descending touchdown without changing ordinary floor behavior.
- [x] Publish sampler/selection/host reconciliation interfaces for application integration.
- [x] Run scoped checks, record native discovery availability without requesting unnecessary permissions.

## Task 4: Resident, previews and scheduling integration

Owner: primary agent. Files: qt/resident/application.*, qt/characters/preview*, qt/CMakeLists.txt, qt/characters3d/CMakeLists.txt, build wiring and documentation.

- [x] Wire new sources/tests and copied raven resources, preserving a 2D-only build.
- [x] Add meaningful integration tests for selection, hop/flight/land, support loss, suppression and settled scheduling.
- [x] Integrate explicit controls and bounded autonomous raven flight/perch decisions; reuse existing timer for surface sampling and active motion.
- [x] Keep reachability separate from support, so other pets accept supported placement/walking without flight.
- [x] Final integrated Release verification: combined 37 passed/four skips; 2D-only 27 passed/three skips. Native controls and controlled window perching pass in both dimensions.
- [x] Generate paired software/native captures as available and inspect actual output.
- [x] Request independent review; fix significant findings and rerun covering checks.
- [x] Update handoff, architecture, format and implementation record with exact limits and evidence.

## Execution notes

Independent ownership permits parallel work as requested in AGENTS.md. Shared API changes must be announced before dependent edits. Avoid concurrent edits to one file. Keep raw captures/logs under a new ignored results/raven-flight-2026-10-10 directory.
