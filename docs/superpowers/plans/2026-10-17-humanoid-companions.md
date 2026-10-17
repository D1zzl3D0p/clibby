# Humanoid Companions Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development or executing-plans; the user already authorized implementation and independent delegation.

**Goal:** Deliver the first six approved human/near-human companions in Clibby.

**Architecture:** Original Blender recipes export real biped/arm skins and bound
props. The trusted registry, existing spatial adapter and finite signature layer
provide resident behavior without new timers or executable package input.

**Tech Stack:** C++17, Qt Quick 3D, Blender, Python unittest.

**Spec:** `docs/superpowers/specs/2026-10-17-humanoid-companions-design.md`

## Constraints and review focus

Preserve current pixel density and existing desktop/API behavior; never touch
`shared/`. Keep unrelated dirty Clibby/productivity work intact. Check actual
arm/prop geometry rather than capability flags alone; mask moving hinges;
cancel and pause signatures; preserve rendered foot pins under both headings;
ensure adults have distinct shapes and visible faces at native size.

## Tasks

- [x] Author six originals. Own `tools/humanoid_asset.py`,
  `tools/author_humanoid_companions.py`, `tests/test_humanoid_companions.py` and
  six new `characters/<name>-3d/` directories. Start with failing asset/skin tests,
  build original geometry, export and prove hand-owned prop/hinge weights and
  measured bounds. Do not modify shared catalogue helper.
- [x] Add six `SignatureMotion` profiles and procedural binding after arm
  realization. Own `qt/characters3d/signature.cpp`, `signature_tests.cpp`,
  `qt/preview3d/controller.cpp/.h`. Test finite profiles, C1 cancellation, real
  arm rotations, no support-channel ownership and legacy behavior.
- [x] Register/deploy/select six new trusted identities. Own
  `qt/characters3d/builtin_assets.h`, `qt/characters3d/CMakeLists.txt`,
  `qt/preview3d/catalogue_tests.cpp` and focused resident selection tests. Adapt
  live checks to real arms while retaining old unsupported-capability assertions.
- [x] Root: review architecture, record approved catalogue, update visible
  Companions wording, build/capture actual resident-stage pixels, inspect art,
  resolve integration/visual defects, run independent review and appropriate
  combined/2D/Python verification, update handoff and save a scoped checkpoint.

Shared interface: asset IDs `nell`, `alden`, `saffron`, `tess`, `rowan`, `vesper`;
skins PascalCase plus `Skin`; normal arms `upper_arm_left/right`,
`forearm_left/right`, `hand_left/right`; new bound prop joint `held_prop` under
the left hand, optional `prop_hinge` under held_prop. Alden has `fastening` under
spine instead of a held object. Every action uses only real joints.

Existing `feat/quadruped-planar` is the active feature branch. Work in place to
use the authorized current Clibby runtime and preserve concurrent changes;
new assets and explicit ownership keep independent work separate. No extra
approval round, merge or push is required.
