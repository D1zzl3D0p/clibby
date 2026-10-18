# Final Six Companions Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development. The user has approved the full catalogue and requests implementation, including signatures. Preserve unrelated current work; no extra approval round.

**Goal:** Deliver Maren, Tarn, Pell, Serein, Nacre and Windle and complete the approved eighteen-companion expansion.

**Architecture:** Extend trusted fixed catalogue assets and finite procedural signatures. Reuse existing biped/four-contact solving, real weighted appendages, model/local rotation spaces, sparse behavior and single resident scheduling. Original Blender source produces measured GLBs; no new runtime simulation, timer or shader system.

**Tech Stack:** Blender, Python unittest, C++17, Qt Quick 3D.

**Spec:** docs/superpowers/specs/2026-10-17-humanoid-companions-design.md (approved eighteen and activities). Below are implementation calls for first finite vignettes.

## Constraints and review focus

96×96 native grid, 80 logical pixels/metre, nearest filtering. Original models/textures/prose. Different adult/humanlike and creature silhouettes; no copied Dweller implementation. Preserve shared/, original packages, all concurrent branding/productivity/docs changes. Work in current active feature branch to retain authorized runtime; scoped checkpoint only.

Real contact geometry and moving appendage hulls must match export; no root/support-chain signature channels. All actions settle within6.4s, cancel smoothly and freeze on pause. Keep both headings and all moving silhouettes within frame. Props stay with authored owners; local hinges follow moving shafts. Claims must match actual broad gestures; no implied finger physics, cloth, detached object transfer or flight.

## Body/action contract

IDs maren-3d,tarn-3d,pell-3d,serein-3d,nacre-3d,windle-3d; PascalCase types/skins.

- Maren: weathered adult human tidekeeper; real three-joint arms, held_prop under hand_left, rope_loop and rope_knot under held_prop. Broad rope-gather/coil and knot inspection.
- Pell: skeletal humanoid traveling coat; real arms, held_prop pebble under hand_left. Balance/study pebble then lower toward coat pocket; bound pebble remains with hand, no detached transfer claim.
- Serein: orchard faun with real arms, cloven visible support feet, leaf held_prop under hand_left. Consider leaf and make free-hand sleeve-brush gesture; no detached leaf simulation.
- Tarn: angular slate-blue four-legged dragon; four real contacts, no reach/flight capability. Signature joints wing_fold_left/right and wing_tip_left/right (tip under fold), tail_base/tip as authored; unfurl/study/refold one wing. Weighted membranes must unfold visibly without detachment. Tip joints rotate JointLocal Y about their actual vertical authored seam; native samples must prove shaft invariance as folds raise (Blender Z converts to glTF Y).
- Nacre: broad-finned cuttlefish, no humanoid arm IK. Two ventral support arms have actual grounded tips using existing two-contact realization; remaining free arms gather via gather_arm_left/right, fins fin_left/right, all under head or spine as authored. pattern_left/right are weighted coloured mantle patches: briefly reveal with small model translations, then hide at rest. This changes visible pattern without new material runtime/timers. Record honest support/glide behavior.
- Windle: loosely human apparition in scarf, no reach/flight claim. Two weighted lower robe/hem supports reuse two-contact solver. Side wisp_left/right and scarf_end under spine/head gather inward, then release to rest. Visible morphology has wisps/hem rather than conventional boots; movement remains grounded/stylized drift.

Each asset worker must publish exact parent/rest geometry and expected local-axis semantics to motion worker before final tuning. Small contract changes settle among workers/root and are recorded here.

## Tasks

- [x] Humanoid assets: failing exported anatomy/ownership tests, original recipes for Maren/Pell/Serein, adult faces/skeleton/faun geometry, cloven soles, measured real arm/prop deformation and bounds, packages/provenance. Own tools/humanoid_asset.py if needed, new tools/author_final_humanoids.py, tests/test_final_humanoids.py and three dirs. Preserve all previous packages/recipes.
- [x] Creature assets: failing GLB/appendage/contact tests, original Tarn/Nacre/Windle recipes and packages; prove actual weighted free appendages, wing pivots, exposed-pattern geometry, contact soles and no invented capabilities. Own new tools/author_final_creatures.py, tests/test_final_creatures.py and three dirs. Do not edit shared catalogue/humanoid helper; coordinate required export options.
- [x] Motion/integration: six finite signatures (32total), trusted registry (34total), deploy/build/test entries, resident catalogue selection31. Test supported channels, finite bounds/C1cancel, genuine local appendage motion, shaft invariance where applicable, appearance patch translation, owner pivots and actual contacts for two/four leg families. Own qt/characters3d/signature.cpp/.h,signature_tests.cpp,builtin_assets.h,CMakeLists.txt; qt/preview3d/catalogue_tests.cpp; qt/resident/quadruped_spatial_tests.cpp. Controller edits only if a concrete incompatibility is demonstrated; notify root first.
- [x] Root: combined build, actual native rest/walk/turn/signature capture, visual/body/action correction, independent scoped source/asset review, Python/fullQt/2D verification, delivery/handoff/full-catalogue status update, exact staged checkpoint verification and rollback commit. Evidence results/final-companions-2026-10-17/.
