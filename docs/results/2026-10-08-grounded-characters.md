# Grounded characters implementation — 2026-10-08

The approved target-driven ground-motion design is implemented for simplified
Mallow in the preview and resident. An original Blender werewolf and optional
live Qt Quick 3D preview are delivered separately. Native 3D acceptance and
resident integration remain pending.

## Grounded 2D

Mallow uses flat cream pieces and black eyes, with two actual hip/knee/ankle
chains. The Qt-free controller tracks a horizontal target with acceleration and
braking, holds stance contacts in world space, chooses steps from reach, generates
swing arcs and solves analytical two-link IK. There is no elapsed-time walk loop.
Second-order followers add adjustable frequency, damping and response to body,
head, ears and tail; bounded impulses provide force response without rigid-body
physics. Navigation proposals are distinct from realized host movement.

The current 2D locomotion metadata requires equal-length segments per leg and
unit scale through the chain. Independent review exposed inner-reach failures
for unequal chains; those unsupported definitions now reject clearly. The
underlying analytical primitive supports unequal lengths. Actual Mallow endpoint
checks cover thousands of samples, arrival, reversal, reactions and extreme speed.

The preview exposes ground targets, contact overlays and tuning. Click sets a
target; Ctrl-click applies force. CPU composition with nearest sampling is the
Mallow default; palette dither is off. Retained/linear/palette modes remain explicit
comparison controls. Dither is not treated as temporal motion stabilization;
output-grid snapping remains a possible follow-up experiment.

The resident realizes fractional desired displacement through integer desktop
window placement, clips against the selected display and feeds actual movement
back into the controller. Walk left/right and Stop are local menu commands.
Ground activity joins the existing single timer, disappears at settling, and
respects pause, hiding, sleep, dragging and display rebasing. Placement preferences
are persisted on arrival rather than written to disk every motion frame. Session
reactions compose before IK; no new external API or wandering behavior is added.

## Blender and 3D

`tools/author_werewolf.py` creates original geometry, texture, rig, editable
`characters/werewolf-3d/werewolf.blend`, skinned GLB and transparent review renders.
Dweller Howl supplied broad brown-wolf/pale-muzzle/red-shirt reference; no copied
pixels or extracted geometry are redistributed. Export validation measures:

| Property | Result |
|---|---:|
| Triangles | 1,762 |
| Exported vertices | 3,782 |
| Deform joints | 20 |
| Materials / maximum influences | 1 / 1 |
| Embedded atlas | 256×256 |
| Animation clips | 0 |

Installed Blender 5.2.2 and official 4.5.0 crashed before Python during Metal
startup in this environment. An official SHA-verified Blender 3.6.0 temporary
fallback worked with `--gpu-backend opengl`; CPU Cycles produced review renders.
The installed Blender was not replaced. Asset provenance and validation are saved
beside the model. Live armature rotation moves evaluated vertices and restores
rest geometry within 0.00001 meters.

Optional Qt Quick3D, ShaderTools and QuickTimeline 6.10.3 are installed locally.
Balsam converts the trusted GLB at build time; the renderer addresses all 20
actual deform nodes and uses no animation clips. A separate Qt-free 3D adapter
shares navigation/contact planning, solves actual unequal 3D rest lengths, maps
world anchors through yaw and lowers the pelvis to retain reach. Head, arms and
tail have bounded second-order response. Turning feet retain world sole anchors.

The preview renders a 192×128 orthographic View3D pass and explicitly enlarges
its same-sized texture 4× with nearest sampling. Atlas mipmaps, palette dither and
temporal AA are disabled. Blender is never launched by the runtime. The motion
timer stops after settling and pauses/hides through preview controls. Actual
native rendering quiescence remains unverified.

## Evidence

- Release build succeeds with optional 3D enabled. Full CTest: 18 passed,
  3 skipped, 0 failures (21 targets). The two Unix-socket tests skip under this
  sandbox; native GPU capture skips because offscreen Qt selects non-RHI software.
- Python regression suite: 37 passed. Preserved 44 golden poses match at 0.0001.
  Original Bramble package and benchmark motion/source hashes match the snapshot.
- Separate `BETTER_PETS_WITH_3D=OFF` Release configuration/build succeeds;
  its CTest results are 15 passed, 2 IPC skipped, 0 failures (17 targets).
- 3D motion, live joint binding and presentation tests pass. Actual Qt scene-space
  contacts through forward movement, stopping and continuous reversal stay within
  0.0002 meters. Skin joint connections and source/output texture sizes are checked.
- GPU capture demands nonblank model pixels and transparent background; QML,
  conversion or blank-frame errors fail rather than being accepted as skips.
- Actual Qt software captures: `results/grounded-motion-hrn9bbnt/` contains 65
  target/reversal PNGs and sidecars, a contact sheet, states and
  `approach-reverse-stop.gif`. Final root is -70 and motion is inactive. These are
  generated verification poses, not authored animation frames. Visual inspection
  confirmed simple silhouette and ground alignment.
- Independent reviews found and resolved unsupported unequal 2D anatomy and
  preview ground alignment issues. Final review found no blocking issues; the
  tuning-widget Reset/load mismatch was also corrected.

## Remaining acceptance

Run `build/qt/character_preview` and
`build/qt/characters3d/character_preview_3d` from a normal desktop Terminal.
Native werewolf pixels, alpha, skinning extremes, mixed-DPI scaling, settled
rendering, matched 2D/3D CPU/GPU/WindowServer costs and energy are still unverified.
The resident stays 2D until native functional and quiet-render checks pass. Existing
monitor snapping and IPC desktop acceptance remain separate open checks. CPU/RSS,
Blender renders and offscreen transforms do not establish battery impact.
