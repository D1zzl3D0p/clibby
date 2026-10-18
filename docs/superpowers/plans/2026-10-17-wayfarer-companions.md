# Next Six Companions Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development. User authorizes the approved next six and useful independent delegation. Preserve unrelated concurrent work.

**Goal:** Deliver Iona, Milo, Orris, Fen, Oona and Quillan with distinct original assets and finite signature actions.

**Architecture:** Extend the existing trusted biped catalogue, Blender skin exporter and finite procedural signature layer. Reuse real three-joint arms, support solver, single resident timer and attached props/hinges. Do not introduce new runtime systems.

**Tech Stack:** Blender, Python unittest, C++17, Qt Quick 3D.

**Spec:** `docs/superpowers/specs/2026-10-17-humanoid-companions-design.md` (approved full catalogue). Implementation calls below elaborate the already approved identities.

## Constraints and review focus

96×96 native grid; 80 logical pixels/metre; nearest filtering. Original names, models, textures and prose. Adult human forms and differentiated dwarf/goblin/alien anatomy, not identical costume swaps. Preserve shared/ and all unrelated branding/productivity/documentation work. Existing active feature branch is reused to retain the current authorized runtime; checkpoint only this batch.

Props must have real weighted owners and hinges; actions cannot move legs or root. Rendered sole pins, support bounds, interruption, pause and exact return to quiet rest remain authoritative. Check both headings and native frame clipping. Do not imply finger contact, cloth simulation or actual light emission without implementing it.

## Interfaces and implementation calls

IDs: iona-3d, milo-3d, orris-3d, fen-3d, oona-3d, quillan-3d. Types/skins: PascalCase + Skin. All use upper_arm_left/right, forearm_left/right, hand_left/right. Left is positive model X. Props use held_prop under hand_left. Hinges use prop_hinge under held_prop. Quillan has no held prop; articulated ear_fin_left/right live under head.

| Companion | Original silhouette and first finite signature |
| --- | --- |
| Iona | Lean human cartographer in practical short field coat; hinged map opens while free hand traces its face |
| Milo | Long-coated human wayside musician; inspects/tunes held small string instrument and taps a silent rhythm |
| Orris | Older broad dwarf clockmaker, square spectacles and beard; turns a held clock's winding key and adjusts spectacles with free hand |
| Fen | Wiry goblin lantern tinker, distinct ears and long fingers suggested as simple hands; opens/closes a hinged lantern shutter |
| Oona | Human orbital geologist, visible face within open helmet; raises and rotates held mineral beneath a fixed helmet lamp detail |
| Quillan | Slender alien with folded ear-fins; raises/studies an empty hand, then makes an experimental greeting with a small ear-fin motion |

First signatures communicate these activities with broad arm/head/hinge gestures at native density. Do not claim precise finger manipulation or physical string dynamics.

## Tasks

- [x] Assets: write failing exported anatomy/ownership tests for next six, add original recipes and needed backwards-compatible helper extension, generate six licensed source/runtime packages, prove actual skin deformation, measured stature/proportions and bounds. Ownership: tools/humanoid_asset.py, tools/author_wayfarer_companions.py, tests/test_wayfarer_companions.py, new six character dirs. Do not regenerate or alter first-six packages.
- [x] Actions: test unsupported-to-supported profiles, finite zero/C1 cancel, support-channel exclusion and authored arm geometry; implement six finite signatures (26 total) using agreed real joint names. Ownership: qt/characters3d/signature.cpp and signature_tests.cpp. Add a backwards-compatible rotation-space flag in SignatureOffset (signature.h): default neutral model axes preserve old profiles; new Iona/Orris/Fen hinges rotate bone-local Y so the axis follows raised arms. Bind this targeted mode in controller.cpp and prove shaft-direction invariance while the key/page/shutter turns. Ownership expands to signature.h and controller.cpp/h as needed.
- [x] Integration: append trusted identities, build/deploy/test entries and resident selection assertions; adapt live prop checks for Quillan's empty hands and ear-fins; prove actual required local joint rotations and hand-owned pivots. Ownership: qt/characters3d/builtin_assets.h, qt/characters3d/CMakeLists.txt, qt/preview3d/catalogue_tests.cpp, qt/resident/quadruped_spatial_tests.cpp. Preserve existing unrelated CMake resource rename.
- [x] Root: build and run combined/Python/2D checks, capture actual native rest/walk/signature pixels, review all six visually, independent code/asset review, delivery/handoff update, scoped rollback checkpoint. Keep prior evidence; new output in results/wayfarer-companions-2026-10-17/.
