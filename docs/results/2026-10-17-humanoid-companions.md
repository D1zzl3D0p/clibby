# First six humanoid companions — 2026-10-17

## Delivered

Nell, Alden, Saffron, Tess, Rowan and Vesper are selectable original built-ins.
The visible selector is **Companions**. Their adult proportions, faces, hair,
complexions and silhouettes differ; each has real three-joint arms and an
attached prop or fastening. Source Blender scenes, GLBs, atlases, rigs, licences,
validation and provenance accompany the reproducible original authoring recipes.
Dweller informed catalogue breadth and pixel-art readability; its names, prose,
models and sprites were not reused.

The complete approved eighteen-companion catalogue is recorded in
[the design](../superpowers/specs/2026-10-17-humanoid-companions-design.md).
The remaining twelve are approved backlog, not implemented by this batch.

| Companion | Finite signature |
| --- | --- |
| Nell | Raises a book and moves its hinged page |
| Alden | Looks down and checks a torso fastening |
| Saffron | Raises and tilts a vial |
| Tess | Turns a small mechanism for inspection |
| Rowan | Studies a held seed/sample |
| Vesper | Raises and opens a hinged pocket watch |

All six reuse walking, reverse heading, gaze, blink, pickup/drop and quiet rest.
Arm and hand-owned prop offsets apply after baseline arm realization. The support
solver retains foot ownership; no new timer or independent prop overlay is added.
Human companions use a more frontal resting angle so faces survive the existing
96×96 nearest-filtered grid at 80 logical pixels/metre. Existing creature density
and orientation are preserved.

## Verification

Fresh Release build in `build/qt-humanoids` succeeds. Full combined CTest:
76 passed, two skipped (`resident_spaces`, `character3d_gpu_render`), no failures.
Native resident-stage captures separately exercised the actual six skins,
rendered foot pins under both headings, hand/prop hierarchy, per-joint local arm
rotation, finite actions, pause/cancel, drop and return to quiet rest. Independent
source and final visual review found no blocking defect.

The true 2D-only build uses `CLIBBY_WITH_3D=OFF` in
`build/qt-humanoids-2d`: 37 passed, one skipped (`resident_spaces`), no failures.
The complete Python asset/regression suite passes all 94 tests.
An export of the exact staged checkpoint also builds independently of concurrent
work and passes 22 focused catalogue, signature and resident-selection tests.

Ignored evidence: `results/humanoid-companions-2026-10-17/`. The final
`frontal/` set contains 105 native action frames per character over seven seconds.
All 630 final frames are 96×96 with no alpha-edge clipping. The assembled
`humanoid-signatures.gif`, overview, contact sheet and `validation.json` sit at
the evidence root. Earlier captures remain for comparison.

Launch the rebuilt `build/qt-humanoids/Clibby.app` and select a companion from
**Companions**. Older build caches can still reference the former checkout path;
use the fresh build after the repository rename.

## Scope and limits

These are first finite signatures, not full finger manipulation, two-hand contact,
object stowing or cloth simulation. Small facial features remain restrained at
native resolution and need user art-direction review. Native captures do not
establish general desktop parity or battery impact. The public-distribution
signing/notarization work is separate. Build deployment reported missing optional
SQL-driver libraries; the combined app relocation check nevertheless passed.
Concurrent branding, packaging and productivity work is preserved separately.
