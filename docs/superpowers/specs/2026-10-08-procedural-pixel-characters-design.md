# Procedural pixel characters

Date: 2026-10-08. Status: user-approved and implemented as a software foundation.
See [implementation evidence and limits](../../results/2026-10-08-pixel-characters.md).
The installed SDK lacks Quick3D/shader tooling; retained 2D is the resident default
and CPU low-resolution composition is an explicitly experimental comparison path.
No native 3D, resource or battery acceptance is claimed.

## Intent and scope

Build polished desktop characters from a small number of authored art pieces,
animated procedurally through a skeleton. A stylized skinned 3D model is another
possible source, but 2D is the default: the user is unsure whether free turning is
needed and prefers 2D if 3D is particularly expensive. Compare authoring effort and
runtime cost before selecting 3D. Reduce the need to draw animation frames without
sacrificing silhouette, expression, or low resource use.

The eventual examples are an original soft, slugcat-inspired creature and a
werewolf. The simple creature is not assumed to be a boneless blob. Develop its
full motion vocabulary before polishing the more articulated werewolf.

This design covers the rendering comparison, minimal character foundation,
interactive preview, and bounded force reactions. Final character artwork and
the werewolf's detailed anatomy are subsequent design work. Existing resident
and session behavior remain the integration baseline. This draft refines the
character-extension direction in the resident specification; it does not close
its desktop, transport, resource, or battery acceptance requirements.

## Rendering comparison

Use a small matching test creature and common action targets for three candidates:

| Candidate | Method | Question it answers |
| --- | --- | --- |
| Direct skeletal 2D | Authored raster pieces attached to bones; nearest sampling | Is the simplest retained renderer visually sufficient? |
| Composited skeletal 2D | Render articulated pieces onto a small transparent canvas, then enlarge with nearest sampling | Does one output pixel grid improve rotation and deformation enough to justify the additional pass? |
| Stylized skeletal 3D | Render a simple skinned model with an orthographic camera onto a small transparent canvas, then enlarge with nearest sampling | Do depth and turning justify model/rig authoring and measured runtime cost? |

Start with 2D. First assess 3D dependency and asset preparation effort; use a
deliberately simple 3D comparison asset rather than polish a werewolf model before
knowing whether this route is worthwhile. If producing the comparison would
require substantial custom model work, report that cost and retain 2D rather
than making completed 3D artwork a gate. The local SDK
does not currently contain the checked Qt Quick 3D module directory; adding that
module is a comparison dependency, not a prerequisite for the 2D foundation.

Evaluate 64, 96, and 128 pixel square canvases as preview choices, not mandatory
pack dimensions. Show actual-size and enlarged views. Record output resolution,
display device-pixel ratio, scale, filtering and anti-aliasing policy. Prefer
integer scaling in physical display pixels; verify native scaling instead of
assuming logical dimensions establish the pixel grid. Linear filtering remains
available where an asset or intermediate pass benefits from it; final pixel
presentation does not enforce linear sampling.

Optional palette reduction and ordered dithering happen on the low-resolution
character output before enlargement. Compare no dither with a fixed pattern tied
to the character canvas. Preserve transparent background and edges, avoid random
per-frame noise, and inspect whether the pattern appears to swim during motion.
Dithering is a shading choice, not a remedy for poor silhouettes or rotation.

Compare rest, breathing, head turns, a limb rotation sweep, squash, walking and
impulse recovery. Keep displayed size, target cadence, gesture duration and camera
framing comparable; record asset/anatomy differences rather than claiming exact
render equivalence. Inspect silhouette, expression, seams, pixel consistency,
shimmer, foot sliding and transparent-edge quality.

Record native process CPU/RSS and available GPU/frame timing, settled quiet
behavior, pause/hide behavior and asset preparation effort. Use fresh ignored
results directories. Offscreen output can verify rendering but cannot establish
desktop resource or battery behavior. Missing GPU/power observations remain
missing. Choose 2D unless the reviewed 3D result provides worthwhile visual or
authoring benefits at an acceptable measured cost; no arbitrary battery pass or
performance threshold is inferred from this comparison.

Qt 6.10 supports [2D render-to-texture and explicit texture dimensions](https://doc.qt.io/qt-6.10/qml-qtquick-shadereffectsource.html),
[3D offscreen textures with explicit dimensions](https://doc.qt.io/qt-6.10/qml-qtquick3d-view3d.html),
and [vertex skinning](https://doc.qt.io/qt-6.10/quick3d-vertex-skinning.html).
These establish available mechanisms, not measured suitability for this project.

## Character and motion boundaries

Introduce a production character path separate from the frozen benchmark loader.
Keep the fixture's twelve named parts, motion equations and golden poses intact.

Bones contain IDs, parent relationships, rest transforms and optional joint
limits. A head is an ordinary bone. Gaze, limbs, tail and contacts are optional
semantic bindings resolved by a compatible controller, not renderer-special
names. No universal anatomy or mandatory head/limb count is imposed.

Art attachments are separate from bones. A 2D attachment references an atlas
region, bone, local offset/pivot and independent draw order. Multiple attachments
may share a bone, and bones need not carry artwork. A future 3D skin additionally
has mesh data, bind transforms and vertex weights. Weighted 2D deformation can be
added when rigid pieces prove insufficient; it is not required for the first
comparison.

Share typed action intents, semantic bindings, timing and character-local
hierarchy conventions. Allow renderer-specific pose/attachment representations
where 2D angles and 3D rotations/skinning differ. Do not force every 2D pet through
a 3D scene. The renderer consumes a resolved pose and does not choose behavior.

Controllers interpret rest, gaze, walk and the existing session reactions in
character-specific ways. Controller requirements and optional fallbacks are
explicit. Character selection must not require integration-specific motion code
or an external API change. Initial gaze targets are preview-local or explicitly
supplied; this does not introduce global pointer observation.

Use local data-only definitions with bounded validation, reusing the resident
specification's initial package limits. Check hierarchy, references, finite
transforms, atlas bounds, paths and controller compatibility before replacing the
live character. Failed loading preserves the existing pet and reports the reason.
Production bounds derive from the character and allowed motion envelope rather
than a fixed 256-pixel window. Keep input regions, movement bounds and contact
anchors distinct; cosmetic deformation must not make a grabbed pet jump.

## Force reactions

Add a small secondary-motion layer:

`intent/base pose -> bounded spring response -> joint/contact constraints -> rendered pose`

Support transient impulses and maintained drag/acceleration observations with
explicit character-local position, direction, magnitude and bounded parameters.
Inputs come from the preview and host-owned pet interaction; no external bone
streaming API or general physics engine is introduced.

Start with body lean and spring follow-through for head, ears and tail where
present. Use damping, displacement/velocity limits and bounded fixed/substeps.
Foot/contact constraints take precedence over spring offsets, preventing planted
feet from being moved through the ground. Walking controllers report desired root
motion separately from realized host placement so clipping cannot silently turn
into foot sliding.

Visual root offsets occur inside the pet surface. Moving the desktop window is a
separate host operation. Notifications retain their existing no-window-motion
rule. Throwing the pet, ballistic window movement, ragdolls and interactions with
other application windows are outside this first slice.

During host dragging, placement remains directly controlled by the grab while
secondary bones may react to bounded acceleration. This deliberately extends the
current drag suppression rule: suppress ambient/session animation, but permit
bounded interaction-motion updates on pointer events and a temporary animation
deadline while spring motion remains unsettled. Pause still suppresses all
animation updates, including those during dragging. Pause freezes pose and clears
dynamic velocities; hide and sleep cancel dynamic responses. Resume, show and
wake start without replay or accumulated impulses. After secondary motion falls
below documented rest tolerances, settle its offsets and stop its animation work.
Stop overall animation deadlines only when no base controller or reaction needs
updates, following the resident quiet/rest policy. Do not keep ticking indefinitely
for an asymptotic spring tail.

## Interactive preview and verification

Provide a development preview for character selection, action triggering,
rendering mode, canvas resolution, palette/dither toggles, speed, pause/reset,
gaze targets and manual impulses. Bone, pivot and contact overlays are optional
debug views. Capture screenshots of the visible result and configuration.

The preview may be freely interactive and need not produce deterministic sessions.
Use controlled time and fixed input cases for core regressions where they help
verify bounded motion and recovery. No full rig editor is required.

Verification covers arbitrary bone hierarchies, independent attachment ordering,
invalid definitions preserving a live character, finite bounded spring recovery,
contact constraints, suppression and no catch-up after sleep. Retain fixture pose
and benchmark compatibility. Inspect new render captures; use native checks for
scaling, focus, click regions and pause/hide settling.

## Delivery order

1. Compare direct and composited skeletal 2D in an experimental preview using
   minimal adapters and a shared bounded impulse harness. Assess 3D preparation
   cost and, where practical, measure a small 3D candidate. These experimental
   adapters do not establish the production package contract.
2. Promote the selected path into the minimum production bone/attachment/controller
   boundary exercised by the comparison, with bounds and data validation.
3. Integrate bounded force reactions with host/session arbitration and verify
   settling, contact and suppression; preview-only success is insufficient.
4. Polish the simple creature's art and complete action set, then develop the
   werewolf using the same intent/controller boundary.

Native monitor-crossing repair and outstanding resident acceptance may proceed
independently. A marketplace, universal skeleton, full physics simulation, rig
editor and real coding-tool adapter are not dependencies of this character work.

This workspace is not a Git repository, so this draft has not been committed.
