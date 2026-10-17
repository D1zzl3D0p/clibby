# Catalogue base implementation plan

> **For agentic workers:** Use coordinated agents with explicit file ownership; verify integrated behavior before committing.

**Goal:** Begin all 17 remaining catalogue pets with genuine original artwork and working motion appropriate to their delivery stage.

**Architecture:** Thirteen new skinned 3D bipeds/quadrupeds reuse the live resident. A fixed trusted built-in registry, generic build conversion and anatomy-derived capabilities replace the cat/wolf-specific assumptions. Four new body families receive deterministic motion implementations and native-grid Blender prototypes outside the resident until their host/drop/suppression contracts are integrated.

**Tech stack:** C++17/Qt Quick 3D resident, original Blender recipes, Python validation, existing single timer.

**Spec:** Existing catalogue architecture and selected B art direction, updated by the user's preference for pixel-rendered 3D and instruction to begin all remaining pets.

## Constraints and review focus

- Preserve existing pets, selected B proportions/style density, two/four-foot contacts and optional 2D builds.
- No fake legs for anchored/crawling/morph bodies, no insect flight claim, no decorative wings claiming flight.
- No absent ears/tails/arms advertised as capabilities and no leaves or antennae relabeled as ears.
- Validate fixed built-in selection, converted joints, finite geometry, host calibration and all actual contacts.
- Signature posture/prop actions remain explicitly pending when base motion does not implement them.
- New captures belong in ignored results; editable recipes/assets and prototype algorithms are versioned.

## Task 1: Shared catalogue activation and capabilities

**Files:** new `qt/characters3d/builtin_assets.h`, `tools/convert_catalogue_qt.py`, renderer binding/stage/scene, optional CMake resources, registry/resident tests.

- [x] Add failing registry, armless biped and missing-anatomy capability tests.
- [x] Implement fixed identities/types, generalized nearest asset conversion and deployment.
- [x] Derive arm/head/ear/tail capabilities from real joints; allow distinct trusted skin names.
- [x] Add bounded quiet personality choices without changing old default profiles.
- [x] Verify every production pack loads, walks/reverses/drops/settles and survives selection.

## Task 2: Nine original bipeds

**Owner:** catalogue_bipeds. `tools/catalogue_asset.py`, `tools/author_catalogue_bipeds.py`, nine asset directories and focused tests.

- [x] Build reusable skinned authoring helpers with measured bounds and finite normalized weights.
- [x] Author Rivet, Paper Heron, Acorn Custodian, Puddle, Pocket Observatory, Night Librarian, Sprig, Hearthling and Crumb Golem.
- [x] Validate distinct geometry/material silhouettes, actual two-foot chains, optional arms and honest base-action coverage.

## Task 3: Four original quadrupeds

**Owner:** catalogue_quadrupeds. `tools/author_catalogue_quadrupeds.py`, four directories and focused tests.

- [x] Author Pip, Biscuit, Clover and Mossback with species-specific proportions and continuous leg meshes.
- [x] Validate canonical chains, true role availability, contacts and source provenance.

## Task 4: Four new motion-family implementations

**Owner:** catalogue_new_families. `experiments/catalogue-families/` and tests.

- [x] Implement bounded anchored stack (Pebblekin), continuous crawler (Lantern Snail), shape morph (Margin), six-contact walk (Velvet).
- [x] Test actual contact/pivot/envelope invariants, deterministic pose reads and finite demonstrations settling.
- [x] Render all four original 96-grid motion studies; mark resident integration and unimplemented actions separately.

## Task 5: Integration and delivery

- [x] Inspect native Qt catalogue captures and new-family motion previews at native size.
- [x] Run combined/2D-only Qt and Python suites, independent review and fix findings.
- [x] Record per-pet implemented/pending status, checkpoint Git and update handoff.

The user authorized beginning implementation of all pets. This plan scopes a concrete base delivery across all 17, without presenting new-family prototypes as completed resident pets or adding another approval round.
