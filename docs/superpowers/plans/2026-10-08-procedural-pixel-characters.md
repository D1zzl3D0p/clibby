# Procedural Pixel Characters Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Track completed tasks in `.superpowers/pixel-characters/progress.md`.

**Goal:** Deliver a reusable skeletal 2D character path, interactive pixel preview, and bounded force reactions integrated with the resident shell.

**Architecture:** Keep the benchmark fixture unchanged. Introduce Qt-free bone/attachment/pose/controller values with a Qt JSON/image loader, retained direct scene-graph rendering and a low-resolution comparison mode. The resident consumes the same session policy through character controllers; preview controls exercise actions, pixel modes and impulses.

**Tech Stack:** C++17, Qt 6.10.3 Quick/Widgets, Python/Pillow asset authoring tools, CTest.

**Spec:** `docs/superpowers/specs/2026-10-08-procedural-pixel-characters-design.md`

## Global Constraints

- Preserve `shared/`, benchmark CLI, equations, golden poses and archived evidence.
- Data-only local characters; no executable downloaded content or new IPC methods.
- Maximum 64 bones/attachments, 2048×2048 atlas, 32 MiB package, 1 MiB JSON.
- Character-local units and angles in radians; desktop coordinates remain host-owned.
- Head is an ordinary bone; attachment draw order is independent of parent order.
- 2D is the default; 3D preparation cost is assessed before requiring model work.
- Preview interaction need not be deterministic; core regression cases use controlled steps.
- No full physics engine, thrown desktop windows, global input observation or rig editor.
- Work in place: this workspace has no Git repository. Preserve a source snapshot before changes; no commits/worktree claims.
- User explicitly authorized implementation of the outlined design; proceed without another approval round.

## Review Focus

- Cyclic/out-of-order hierarchy and missing semantic bindings reject safely or choose documented optional fallback.
- Path/symlink escape, oversize metadata/image and incompatible controller preserve a running pet on failed selection.
- Pause during dragging freezes visual motion; hide/sleep remove spring deadlines and never replay impulses.
- Foot contact and clipped movement do not permit unchecked root displacement or indefinite spring work.
- Pixel output keeps transparency, stable dither and independent layering; comparison mode cost is reported honestly.

## Task 1: Character values, loader and controller

**Files:** `qt/characters/character.h`, `character.cpp`, `loader.h`, `loader.cpp`, `tests.cpp`.

**Interfaces:** Namespace `pets::character`; `Definition`, `Bone`, `Attachment`, `Transform`, `Pose`, `Action`, `Controller`; Qt loader `LoadedCharacter loadCharacter(const QString&)` contains `Definition definition` and `QImage atlas`.

- [x] Write failing C++ cases for arbitrary bone IDs, parent-after-child topology, hierarchy cycles, independent attachments, finite bounds, spring settling and grounded foot anchors.
- [x] Establish typed local transforms `{x,y,rotation,scaleX,scaleY}`, root offset, indexed bones and semantic binding map. Provide `Controller::pose(Action,double,Vec2)` and `impulse(Vec2,double)`, `step(double)`, `resetDynamics()`, `settled()`.
- [x] Implement base actions `Rest`, `Idle`, `Walk`, `Attend`, `Celebrate`, `Failure`, `Acknowledge`, with soft/articulated built-in controllers; spring parameters are bounded and contact constraints applied after secondary motion.
- [x] Validate directories/assets before decoding, then load the manifest/atlas transactionally. Tests include traversal, symlinks, invalid references, wrong types and image limits.
- [x] Build/run the focused character tests, record results in the ledger.

## Task 2: Editable art and two example anatomies

**Files:** `tools/author_characters.py`, `characters/soft-creature/`, `characters/werewolf/`.

- [x] Author a small atlas from editable vector/pixel shape definitions with Pillow; pieces are drawn once and transformed by bones, not frame sequences.
- [x] Generate explicit bones, semantic bindings, attachment pivots/layers, interaction regions and motion envelopes matching Task 1's schema.
- [x] Supply original soft-creature and werewolf examples, source attribution and preview art. Mark their art as a first iteration subject to visual polish.
- [x] Load both examples through the validator; inspect atlas and rendered action captures rather than declaring artistic quality from code.

## Task 3: Renderer and interactive comparison preview

**Files:** `qt/characters/item.h`, `item.cpp`, `raster.h`, `raster.cpp`, `render_tests.cpp`, `qt/preview/main.cpp`.

**Interfaces:** `CharacterItem` owns a validated definition/atlas, accepts resolved `Pose`, offers direct/composited mode, resolution, smooth sampling, palette/dither options. CPU composition may be used as an explicitly experimental comparison fallback when installed shader tooling is unavailable; it must not be presented as a measured efficient GPU implementation.

- [x] Add failing render cases for transparency, changed bone poses, independent layers and stable ordered dithering.
- [x] Implement retained direct QSG nodes in attachment draw order with world transforms computed independently from that order. Texture filtering is a choice rather than globally linear.
- [x] Implement 64/96/128 low-resolution transparent composition, nearest final enlargement and fixed ordered palette dithering. Keep allocations bounded, update only on pose/style changes, identify upload/composition costs in diagnostics/docs.
- [x] Build a Widgets-controlled Quick preview with character selection/load, action, pixel mode, resolution, dither/palette/filter controls, speed, pause/reset, gaze, impulse and bone/contact overlays.
- [x] Add CLI screenshot cases with output/config identity for repeatable software verification. Record 3D absence/dependency feasibility instead of fabricating a 3D measurement.
- [x] Run render tests and inspect captures at native pixel and enlarged sizes.

## Task 4: Resident composition and build integration

**Files:** `qt/CMakeLists.txt`, `qt/resident/desktop_host.*`, `application.*`, `settings.*`, `main.cpp`, related tests, `tools/build_qt.py`.

- [x] Add failing host/application cases for character-derived bounds, failed replacement keeping the live pet, unchanged session messages, drag springs and suppression.
- [x] Preserve fixture mode for existing tests/benchmark; resident uses production characters when supplied, with selection menu and persisted validated character identity.
- [x] Route existing session reactions into character actions; notification motion remains local to the surface.
- [x] Extend host drag feedback with bounded local impulse observations. Combine spring deadlines with the existing single application timer, cap catch-up, reset timestamps after suppression and stop spring work when settled.
- [x] Register new libraries/executables/tests and copy example character assets next to the build. Keep Qt Quick 3D optional and unavailable by default.
- [x] Run the full Qt and Python suites, preserve 44 golden poses, and distinguish transport skips/native acceptance from software passes.

## Task 5: Review, evidence and handoff

**Files:** `README.md`, `docs/handoff.md`, `docs/architecture.md`, `docs/results/2026-10-08-pixel-characters.md`.

- [x] Independently review the character boundary, renderer and resident integration; fix actionable findings and rerun affected checks.
- [x] Inspect example/action/style captures and refine obvious silhouette/seam problems.
- [x] Document launch, preview and capture commands, schema, force behavior, experimental CPU-composition cost and unavailable 3D/native measurements.
- [x] Update current handoff with delivered scope and remaining native monitor, power and live IPC checks; make no battery or final-art-quality claim.
