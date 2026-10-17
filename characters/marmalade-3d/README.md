# Marmalade 3D

Original skinned woodland cat derived from the approved B silhouette in
`art-studies/marmalade/woodland-refined`. The full ribcage, shoulder, haunch,
compact head, cheek planes, sparse bands and curved tail preserve that direction.
The native review images use a 96×96 orthographic canvas, matching the art studies.

`marmalade.glb` has one mesh, one nearest sampled nine-colour atlas material,
22 deform joints, 2,066 triangles and at most two weights per vertex. Each leg is
a continuous ring mesh with blended knee and ankle seams. Separate overlapping
body forms preserve the low-poly silhouette. Tail rings blend across both tail
joints. Eyes have dedicated joints for blink deformation. There are no animation
clips; the resident supplies poses.

Blender coordinates are Z up, -Y forward, +X right; glTF is Y up, +Z forward.
`rig.json` records four ground contacts, canonical limb roles, rest matrices, and
conservative skin influence hulls measured from exported triangles. The nominal
body anchor is 1.02 metres high, hips 1.04, ankles .17, paw soles zero.

Rebuild with `tools/author_marmalade_3d.py` in Blender. Convert through
`tools/convert_marmalade_qt.py` and Qt Balsam. Blender is an authoring dependency
only. `validation.json` contains exported geometry/weight and evaluated armature
deformation checks; this is asset evidence, not a resident performance result.

All geometry and texture are original and MIT licensed. The preserved art study
is unchanged.
