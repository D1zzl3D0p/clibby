# Next six companions — 2026-10-17

## Delivered

Iona, Milo, Orris, Fen, Oona and Quillan are original selectable built-ins under
**Companions**. Each includes source Blender scene, GLB, atlas, rig, previews,
licence, measured validation and provenance. The original recipe is
`tools/author_wayfarer_companions.py`; backwards-compatible helper extensions
leave the first six packages and their geometry unchanged.

| Companion | Signature action |
| --- | --- |
| Iona, Field Cartographer | Raises and opens a folded map, then makes a free-hand route-study gesture |
| Milo, Wayside Musician | Raises a small string instrument, makes a tuning gesture and taps a silent rhythm |
| Orris, Traveling Clockmaker | Winds a held clock's key, then raises a hand toward his spectacles |
| Fen, Lantern Tinker | Raises a lantern and adjusts its hinged shutter |
| Oona, Orbital Geologist | Raises and rotates a mineral beneath a fixed helmet-lamp detail |
| Quillan, Quiet Visitor | Studies an empty hand, then tries a greeting with articulated ear-fins |

Adult proportions, faces, limb lengths and body widths vary. Orris is compact
and broad, Fen is wiry with long ears, Quillan is tall/slender with folded fins,
and Oona's open helmet leaves her face visible. The native 96×96 grid, nearest
filtering and 80 logical pixels/metre are preserved. Triangle counts span
1,166–1,560, with normalized skin weights and measured moving hull coverage.
Names, prose, models and textures are original; Dweller remains breadth/style
inspiration only.

## Motion and review correction

All six use real arms and the existing contact solver, finite signatures and
single resident timer. Staged free-hand and fin channels make the activities
progress before returning to rest. Props follow actual left-hand joints;
Quillan is empty-handed with head-owned weighted ear-fins.

Independent review caught neutral-axis hinge motion that could rock a raised
key/page/shutter off its authored shaft. A targeted `SignatureRotationSpace`
flag now lets these three new hinges rotate in joint-local space. Default model
space preserves the previous twenty profiles. Native tests prove shaft-direction
invariance through each complete action, while also requiring actual local
rotation. Arm signatures still apply after arm realization; no signature owns
root or support-chain motion.

## Verification

- Fresh Release resident/app build succeeds in `build/qt-humanoids`.
- Full combined CTest: **82 passed**, three environment skips (`menu_bar_icon`,
  `resident_spaces`, `character3d_gpu_render`), no failures.
- True 2D-only Release build (`CLIBBY_WITH_3D=OFF`): **37 passed**, two environment
  skips (`menu_bar_icon`, `resident_spaces`), no failures.
- Full Python suite: **98 passed**. One earlier run during parallel compilation
  failed the existing startup-delay benchmark's CPU/RSS coverage assertion;
  the complete fresh run after compilation/captures finished passes unchanged.
- All six actual native skins pass foot contacts under both headings, gaze,
  drop, real hierarchy/pivots, local signature joints, pause/cancel and quiet rest.
  Iona/Orris additionally check free-hand rise and approach toward map/head.
- Final native signature captures contain **630 unclipped 96×96 frames** over
  seven seconds per companion; rest/walk/turn and walking sequences are retained.
- An export of the exact staged checkpoint builds independently of concurrent
  work and passes all 28 focused catalogue/signature/resident-selection checks.
- All 360 additional native walking frames are also unclipped.
- Independent source/asset and final native visual review has no remaining
  actionable finding. Scoped whitespace checks pass.

Ignored evidence: `results/wayfarer-companions-2026-10-17/`. Final outputs are
`wayfarer-signatures.gif`, `wayfarer-overview.png`, `wayfarer-contact-sheet.png`,
`validation.json`, source comparison and raw `native/` captures. Capture/assembly
scripts and worker/review reports remain beside the evidence.

Launch the rebuilt `build/qt-humanoids/Clibby.app` and choose from **Companions**.
The app deployment repeats the existing optional SQL-driver dependency warnings;
its relocation test passes. Public signing/notarization remains separate.

## Limits and remaining catalogue

These first activities use broad procedural gestures. Precise finger contact,
physical string vibration, map marking, emitted helmet light, cloth simulation
and object stowing are not implemented. No battery or broad desktop-parity claim
follows from these checks.

Twelve of the eighteen approved expansion companions are now implemented.
Maren, Tarn, Pell, Serein, Nacre and Windle remain approved backlog.
Concurrent branding, packaging, documentation and productivity work is preserved
outside this scoped checkpoint.
