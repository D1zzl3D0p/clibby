# Original low-poly werewolf

Generation status: generated and visually inspected in Blender 3.6.0 using its
OpenGL startup backend and CPU Cycles renders. Installed Blender 5.2.2 and an
official 4.5.0 fallback both crash during Metal GPU detection before Python runs;
the older official 3.6 fallback avoids that startup problem with `--gpu-backend
opengl`. The working 3.6 fallback was extracted in `/private/tmp`, without changing
the installed Blender. See `startup-blocker.txt` for the failed newer builds.

Authoring source: `tools/author_werewolf.py`. Generate with:

```sh
rtk proxy blender --background --factory-startup --gpu-backend opengl --python tools/author_werewolf.py
```

Blender authors the mesh, named deform skeleton, packed original color atlas,
editable `werewolf.blend`, skinned `werewolf.glb` and transparent front/side/
three-quarter review renders. Review renders use CPU Cycles so authoring does not
require a working GPU renderer. No animation clips or sprite sequences are
exported; live joint motion is the runtime renderer's responsibility.

The visual design is original geometry: broad face, narrow amber eyes, pointed
cheeks, oversized paws, short digitigrade legs, a compact tail and an open torn
red shirt. Dweller's Howl was a visual reference for the broad brown-wolf/pale-
muzzle/red-shirt direction; no downloaded pixels, traced geometry or extracted
Dweller asset enter this package.

`rig.json` records named joint hierarchy, rest matrices, leg endpoints and contact
points. Authoring coordinates are meters, +Z up, -Y forward and +X right. glTF
converts them to +Y up, +Z forward with `(x, y, z) -> (x, z, -y)`. Rest matrices
are column-major and explicitly in Blender armature space. Head, ears, paws and
tail are ordinary deform joints. Prototype pieces use single-bone weights and
overlapping low-poly joint forms rather than a high-resolution smooth skin.

`validation.json`, when generated successfully, records counts from the exported
GLB: <=4,000 triangles, <=32 joints, <=4 influences, <=4 materials, one embedded
256x256 atlas, no animation clips. Validation decodes exported weights, verifies
normalization and checks joint names and embedded texture references.

Current measured export: 1,762 triangles, 3,782 vertices after normal/UV seams,
22 deform joints, one material, one influence per vertex, one embedded atlas and
zero animation clips. All floating-point accessors (including positions, normals
and inverse bind matrices) were checked for finite values. Front, side and
three-quarter views were rendered and inspected after correcting UV-layer naming
on the original shirt and ear triangles.

The authoring validation also evaluates the live armature: rotating the left
thigh by 0.35 radians moves skin vertices by up to 0.372 meters, and restoring the
bone returns the evaluated mesh to rest within 0.00001 meters. This verifies
Blender skinning only; the separate Qt preview must verify live runtime joint
access and locomotion.

The atlas is intentionally simple: eight original flat-color strips with nearest
texture filtering. There are no normal maps or fur/cloth simulation. Blender
static renders do not establish procedural locomotion, Qt renderer success,
performance, idle quiescence or battery impact.

## Procedural Qt preview

After building with the optional Qt Quick3D modules, run:

```sh
rtk proxy build/qt/characters3d/character_preview_3d --target 60
```

The live preview moves to a ground target with planted contact anchors, measured
3D limb IK, a supported yaw transition and velocity-responsive spine, head, arms
and tail. No walk clip drives the deform joints. At arrival the motion timer
stops. Launching without a target starts at rest.

Lateral acceleration and braking ease while feet follow independently. A damped
leg-compression support model permits vertical pelvis bounce. Lifted recovery
and yaw transitions ease through their boundaries instead of applying an abrupt
toe-off offset. The desktop integration design is in
[the resident plan](../../docs/3d-resident-plan.md); this asset still runs in the
3D preview rather than the resident pet selector.

Click the ground path to choose a destination; mouse targets use the camera
projection, and Left/Right choose the corresponding screen direction. The gold
dot marks the destination. Space stops, I applies a bounded impulse, P pauses,
and D toggles world-space foot contact dots (green planted, gold swinging).
The dots make contact stability inspectable during a turn.

The model is composed on the GPU at 192x128 and enlarged 4x with nearest sampling.
Headless tests verify deform-joint bindings, target arrival/reversal, world-space
contacts and presentation properties. A native framebuffer test is separate and
skips when no graphics device is available; these logic tests do not establish
visible native rendering, desktop performance or battery impact.
