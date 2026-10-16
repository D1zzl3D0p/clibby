# Low-poly Blender werewolf

Status: follow-on design for review, not implemented. Begin after the simplified
creature establishes grounded target-driven motion. This is one new 3D werewolf;
the existing 2D Bramble stays available but receives no parallel art redesign.

## Intent and visual direction

The user requested Blender, a simple PS2-like model, procedural skeletal motion
and a werewolf closer to Dweller's design. Use Dweller's
[Howl still](https://dweller.pet/pets/howl/still.png) as the presumed reference:
brown fur, pale muzzle/chest, upright wolf ears, substantial hands/feet and a torn
red shirt. This reference identification is an assumption the user can correct.

Create original geometry and texture work with altered proportions, face and
garment details. Dweller's [terms](https://dweller.pet/#terms) request that pets not
be copied out. Do not redistribute its downloaded art or texture it onto the
model. Keep asset provenance and an original-work license alongside our package.

Aim for a readable, slightly hunched, friendly creature at desktop size: broad
head/shoulders, clear muzzle and ears, compact torso, readable paws and simplified
digitigrade silhouette. No sculpted fur, cloth simulation, normal maps or realistic
materials. The PS2 reference describes aesthetic simplicity, not emulation of
console rendering defects.

## Deliverables and authoring

Use installed Blender (currently 5.2.2 LTS) to author an editable `.blend`, an
exported skinned `.glb`, one small texture atlas and front/side/three-quarter
review renders. Keep an authoring/export Python script with editable parameters;
Blender is an offline asset tool, not a resident dependency.

Initial budgets: at most 4,000 exported triangles, one 256x256 color atlas, no more
than four materials, 32 deform joints and four joint influences per vertex. These
are project limits to verify, not proof of low runtime cost. Exported vertex counts
may exceed Blender's mesh vertex count at UV/normal seams. Validate export counts,
weights, rest pose, texture references and named joint mapping.

Rig root/pelvis/spine/head, ears, arm/hand chains, leg/foot chains and tail if the
chosen silhouette includes one. The head is an ordinary joint. Keep Blender's
authoring constraints/control objects out of the deform rig exported at runtime.
No authored walk-cycle clips are required. Blender's
[glTF exporter](https://docs.blender.org/manual/en/5.1/addons/import_export/scene_gltf2.html)
supports skinning; verify the installed exporter's actual settings during execution.

## Rendering approach and feasibility check

Prefer an optional Qt Quick 3D preview/render module, retaining Qt as host. The
local 6.10.3 SDK currently has neither Quick3D nor ShaderTools. First establish a
compatible SDK/module route and a minimal skinned-joint proof with transparent
background, nearest texture sampling, controlled low-resolution target and idle
quiescence. Do not make the working 2D build require these modules.

Qt 6.10 [View3D](https://doc.qt.io/qt-6.10/qml-qtquick3d-view3d.html) provides
explicit offscreen texture dimensions. Use a fixed orthographic three-quarter
camera, simple baked color plus restrained lighting, render at small resolution,
and enlarge with nearest sampling. Disable anti-aliasing/temporal effects for the
baseline. Render the 3D pass on the GPU; do not read it back into QPainter each
frame. Compare native-size low-poly rendering and low-resolution pixel rendering.
Static palette dithering is optional and off initially.

Prove live access to deform-joint transforms before choosing the asset conversion
route. Prefer trusted build-time conversion into addressable Qt scene nodes;
do not assume a generic runtime GLB loader exposes the required joints. Check
Quick3D distribution/license fit before making it a shipped dependency. If the
backend cannot be built or validated here, still deliver the Blender asset and
record the exact renderer blocker; do not claim pre-rendered sprite clips fulfill
the requested 3D procedural renderer.

## Motion integration

Reuse target intent, horizontal navigation, contact/step planning and expressive
tracking concepts from the grounded controller. The 2D affine `Pose` is not a 3D
joint format: add a separate Qt-free 3D pose adapter with explicit axis, unit and
joint conventions. Map the horizontal floor into a fixed 3D ground plane with
left/right foot depth separation. Camera perspective does not change navigation.

Start with bounded two-link leg solves and articulated foot/hock presentation;
if a genuinely additional load-bearing segment is required by the approved rig,
add a bounded constrained-chain solver rather than mislabel a two-link solver.
Body/head orientation, arm inertia and foot goals respond to movement. Allow yaw
turning through a supported stance rather than flipping the whole model in one
frame. Inspect skinning at knees, shoulders, muzzle and shirt across extreme poses.

No frame sequence substitutes for locomotion. General free-roaming 3D navigation,
cloth/fur physics and terrain remain out of scope. The 3D preview is the first
integration; enable resident selection only after contact behavior, transparent
presentation and stop-rendering behavior work.

## Acceptance and resource evidence

Show actual-size and enlarged motion captures for approach, stop, reversal and
impulses, plus multiple static viewpoints. Verify rig-driven deformation and
ground contacts in the Qt renderer, not only inside Blender. The model must be
recognizably low-poly and readable without increasing texture/model complexity
to conceal locomotion defects.

Record optional-module build status, asset budgets, graphics backend, texture
resolution, frame timing, CPU/RSS, visible settled/active and hidden states under
new ignored `results/` run names. Compare against the simplified 2D build with the
same scenario. Native rendering, GPU/WindowServer cost and energy need real
desktop evidence; triangle count and offscreen CPU do not establish battery impact.
