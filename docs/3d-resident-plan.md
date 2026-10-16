# Making the low-poly werewolf a resident pet

Status: implemented initial integration, 2026-10-10; see [implementation and open acceptance](results/2026-10-10-resident-3d.md). The original proposal below is preserved. The proposal below does not itself establish native input acceptance or battery impact. The current animation corrections can proceed independently.

The [official Qt View3D reference](https://doc.qt.io/qt-6/qml-qtquick3d-view3d.html)
supports embedding the view as a Quick item, offscreen composition, explicit
texture dimensions and `mapFrom3DScene` projection. Keep implementation within
the installed Qt 6.10.3 API; the live documentation also describes newer APIs
that this project cannot assume.

## Recommended delivery

Add the werewolf as an optional character in the existing `better_pets`
application. Keep one instance lock, tray menu, preference file, session inbox,
Toolbox, source/acknowledgment dispatcher, and application scheduler. Use the live
skinned model rather than a sprite export. Keep the existing 2D build available
when the optional Qt modules or a GPU are absent.

The user's clarified ownership is model first: the character owns world-space
position and pose, and the transparent native window follows its projected root.
The window is a presentation surface, not a target the model must chase. Convert
world points to window-local drawing coordinates by subtracting the window's
projected origin. Host rounding and screen limits remain realization feedback;
they are the only placement constraints that can alter presented world motion.

The first integration should support selecting the 3D pet, dragging, left/right
travel, stop, pause, hide/show, saved placement and selection, and the existing
session reactions/cards. A rig editor, arbitrary downloaded models, autonomous
wandering, physics-driven desktop movement, and new IPC methods are outside this
slice. Existing lateral navigation remains authoritative; vertical leg support
changes the character pose inside its window rather than moving that window.

A separate 3D resident executable would duplicate placement and notification
policy and create two competing instance owners. Rendering to PNGs or reading
back the GPU every frame would discard the retained GPU path. Neither is needed.

## What exists and what blocks integration

| Existing source | Reusable behavior | Required change |
| --- | --- | --- |
| `qt/resident/desktop_host.cpp/.h` | Transparent nonactivating topmost `QQuickWindow`, dragging, screen reconciliation, logical placement, fractional horizontal realization | Install a 3D presentation item and derive its dimensions, floor/root anchors, visible mask and grab region |
| `qt/resident/application.cpp/.h` | Single-shot scheduler, suppression, session policy, cards, menus, preferences and transactional 2D swaps | Route motion/render/anchor operations through a small character-instance boundary instead of testing only `characterController_`/`groundedController_` |
| `qt/preview3d/controller.cpp/.h` | Real skin binding, 20 joint transforms, gestures, reach/grips and scene-space anchors | Extract reusable scene binding/application; remove its private motion timer from resident use |
| `qt/preview3d/Stage.qml` | Transparent `View3D`, original skin/light, explicit 192×128 GPU texture and nearest enlargement | Extract an embeddable `Item`; retain preview `Window`, labels, controls and shortcuts in the preview wrapper |
| `qt/characters3d/pose.cpp/.h` | Qt-free measured 3D limb solve, yaw/contact planning, settled state, speed and impulse | Add host-authoritative reset/span/propose/realize APIs; current `step()` realizes its own proposal |
| `qt/characters3d/CMakeLists.txt` and `tools/convert_werewolf_qt.py` | Build-time Balsam conversion, named live skin, nearest atlas and optional dependencies | Produce a reusable render target and deployed trusted QML/mesh/texture resources for the resident |
| `characters/werewolf-3d/` | Original GLB, rig, atlas, license and attribution | Add selectable package metadata; it currently has no `manifest.json` and cannot pass the 2D loader/catalog path |

Current motion source and the latest correction records define the movement
behavior to preserve during integration.

## Boundaries and ownership

Introduce an internal `ResidentCharacter` abstraction with operations for
identity/capabilities, travel speed, reset against realized host position and
span, target/stop, propose/realize, impulse, semantic reaction, settled state,
pose application and projected utility anchors. Return presentation metadata
such as logical size, root/floor anchors, grab region and conservative visual
region separately from animation state. Keep the fixture adapter working; do
not convert the frozen benchmark contract into a universal rig format.

Implement the existing 2D path as the first adapter without changing its loader
or pack schema. Implement one trusted built-in 3D werewolf adapter. A small
discriminated package descriptor can dispatch existing version-1 2D manifests
and the new built-in 3D descriptor. Use a distinct format/kind rather than
silently treating a 3D manifest as a valid 2D `Definition`. Preserve catalog
limits, canonical-directory checks, unique IDs, resource bounds and prepare-
before-swap behavior. Failed loading or unavailable 3D rendering must leave the
current pet and sessions usable and present an actionable character error.

The 3D descriptor should contain identity/name, preview/license references,
built-in rig/asset identity, intended logical output size, pixels-per-meter,
floor/root anchors and authored interaction/envelope data. It must not authorize
arbitrary package-provided QML or executable shaders. Conversion happens at
build time; Blender and Balsam are not runtime dependencies.

Extract a `pet_character3d_render` library from the preview controller. Keep
joint binding, rest transforms, quaternion application, gesture evaluation and
projection there. Keep the Qt-free navigation/IK in `pet_character3d_core`.
Resident use calls `advance(dt)` only when the existing application deadline
fires. Preview use can retain its own thin timing wrapper. Do not run both
timers, and do not restart a timer merely because a speed preference changes
while the pet is settled.

## Coordinate contract

The desktop host uses logical pixels and screen coordinates with Y downward.
The GLB uses meters, Y upward and Z forward. The current planner uses centimeter
travel units and publishes `rootX`, while the preview presents that movement on
model Z using `rootX / 100`. These names do not make centimeters interchangeable
with desktop pixels.

Define one explicit projection calibration, `logicalPixelsPerMeter`, and a
screen-horizontal ground direction for the resident stage. An orthographic
camera or rotated stage basis must project this ground direction entirely onto
screen X. The current three-quarter camera projects its +Z travel path with a
vertical component; copying its camera and translating the native window only
on X would make feet drift vertically relative to the desktop floor. For the
initial resident, choose the camera/stage basis for a horizontal path and retain
the current lighting and pixel treatment. Camera angle can remain an internal
presentation setting; it need not block delivery with a user question.

Host displacement in pixels converts to centimeters as
`100 * displacementPixels / logicalPixelsPerMeter`. Realized host displacement
converts back before the contact planner advances. Rebase the planner on drag
release, display changes, hide/show and suspend/wake, using the same bounded
policy as 2D. Add the missing `Motion::reset`, `propose` and `realize` operations;
keep `step` as a preview convenience that composes them. The motion model owns
the requested world transform; the native host reports the actually presented
transform when integer rounding or a screen edge constrains window following.

Inside the pet window, draw the world root minus the window's projected origin;
this keeps it near the padded root anchor as the window follows. Publish feet in
the same coordinate frame, preserving absolute contact state in the motion
controller. Never add world travel a second time to the local avatar. Stable yaw,
vertical support and gestures change the world pose without a new window-origin
constraint. A fixed padded envelope keeps vertical bounce and planted contacts
inside the surface; it avoids resizing the native window every frame. The scene
binding must use avatar-relative world transforms consistently when
reconstructing skin contacts.

Project head/card/effect/grip scene points through the same `View3D` mapping and
nearest-output scale. `ResidentApplication::cardAnchor()` currently evaluates
2D `utilityAnchors()`; replace that assumption with the adapter's projected
logical-pixel card anchor. Continue passing host position plus local anchor to
the existing `StatusLabel::placeBeside`. Desktop grip targets require an explicit
inverse projection onto a chosen model plane; keep current model-local held
goals internal until a real desktop object interaction needs that contract.

## Transparent presentation and pointer behavior

Reuse DesktopHost's alpha buffer, transparent window color and focus flags.
The embedded 3D scene uses `SceneEnvironment.Transparent` with no preview
background, floor marker, contact dots, target marker, controls or shortcuts.
Retain the explicit low-resolution render target and nearest presentation; size
the final item for a desktop pet rather than the preview's 768×680 window. Fit
the camera and motion envelope to include wave, tail, ears and bounded vertical
support at every yaw, with enough headroom to prevent the earlier cropping bug.

Qt's current native window mask couples visual clipping and input shape. The 2D
host unions its alpha-derived posed region with a separate stable grab region;
3D cannot cheaply derive that alpha region by CPU readback every frame. For the
first 3D adapter, project conservative authored mesh/joint envelope geometry
into a logical `QRegion`, including the full posed arm, paw, head and tail
extent. Union that region with the authored grab region for the native mask.
Use only the grab region to start dragging or open the pet context menu.
Cache/change the mask only when its integer region changes.

This provides a reviewable compromise rather than claiming pixel-perfect
click-through: conservative silhouette regions can intercept transparent fringe
pixels even though the host ignores those clicks. Native acceptance must measure
that fringe and confirm that the useful surrounding desktop stays clickable.
If unacceptable, implement a platform-specific input-only region/hit-testing
adapter or GPU-side picking as a later targeted change; do not silently introduce
per-frame framebuffer readback. A rectangular mask is acceptable only as a
temporary integration probe, not the finished desktop interaction behavior.

## Sessions, rendering lifecycle and resources

Map existing session intents to the same semantic 3D gestures already supported
by the preview: working→nod, needs-input→look, completed→success,
failed→failure. Drive gesture progress from the existing reaction start/end
times, including cancellation and suppression, rather than replaying an
independent one-second timer. Keep Open source and Acknowledge dispatch and
sequence revalidation in the resident shell; cards remain shared Widgets UI.

Pause/hidden/suspended states stop animation deadlines, freeze or reset local
motion according to existing policy, and stop live texture updates. Settling
removes the ground/support/gesture deadline while session expiry can still wake
the shell. No QML `NumberAnimation`, looping timeline or always-running motion
timer should keep the scene dirty. Check actual settled GPU redraw behavior;
`settled()` alone is not evidence that Qt Quick 3D stops rendering.

Move converted assets into an application resource or an installed trusted
directory, including generated mesh/texture dependencies, rather than relying on
source/build absolute macros in the deployed binary. Keep Quick3D/ShaderTools/
QuickTimeline/Balsam optional as in the current build. The software Qt Quick
backend cannot render `View3D`; detect this when selecting the 3D adapter and
retain the previous character. Native GPU failure after installation should use
the existing rendering-error path and leave controls reachable.

## Implementation order and acceptance

1. Extract embeddable 3D presentation and timerless binding. Keep the preview
   joint and presentation checks passing, including 20-joint live skin binding,
   wave pitch, held grips and exact contacts.
2. Add host-authoritative 3D motion realization and explicit coordinate
   calibration. Test fractional/quantized travel, screen-bound clamping,
   horizontal projection, rebase, speed changes, reversal and vertical support
   without changes to realized lateral travel.
3. Add the internal character adapter and built-in package descriptor. Test
   2D↔3D selection, persisted identity/speed, unavailable optional modules,
   malformed metadata, asset-load failures and rollback preserving sessions.
4. Install the 3D item in DesktopHost with projected masks and anchors. Test
   bounds across gestures/yaw/support extremes, card placement, grab region,
   screen reconciliation, and drag/release. Keep 2D host regression checks.
5. Route existing runtime controls and reactions through the adapter. Use the
   fake resident clock to prove no motion timer after settle/pause/hide, expiry
   scheduling still works, reactions cancel correctly, and cards retain shared
   acknowledgment/source behavior.
6. Verify native macOS presentation: visible unfocused, transparent corners,
   no cropped limbs, bounded fringe interception, accurate drag/context menu,
   pause/hide/wake, Retina scaling and multi-monitor crossing. The existing
   monitor-crossing defect remains an explicit acceptance item.
7. Compare matched 2D/3D idle, walking, reaction, paused and hidden scenarios.
   Record process CPU/RSS plus GPU/WindowServer/energy where available. Require
   settled render quiescence before making an efficiency claim. Offscreen
   transforms and static screenshots cannot establish battery impact.

No new functional user choice is necessary to start this implementation: the
existing werewolf asset, resident controls, live renderer and current pet
selection workflow determine the first slice. Native size/camera/fringe can be
presented for visual review after a working resident build exists. This plan is
the requested investigation output, not a claim that those integration steps
have already shipped.
