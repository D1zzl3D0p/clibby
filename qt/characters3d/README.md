# Optional original low-poly werewolf preview

The resident remains 2D. This separate preview is a real Qt Quick 3D scene,
using the original Blender model from `characters/werewolf-3d/`.

With compatible Qt Quick 3D, ShaderTools, QuickTimeline and Balsam installed,
configure normally with `BETTER_PETS_WITH_3D=ON` (default). The optional module
always builds its Qt-free motion tests; unavailable modules or assets leave the
working 2D application independently buildable.

Balsam converts the trusted GLB at build time into ignored
`build/qt/characters3d/converted/`. It emits a skin with 22 addressable joint
nodes and inverse bind poses. The converter verifies every rig name, forces
nearest atlas sampling and disables mipmaps. Blender is never launched by the
preview or resident.

Launch:

```sh
rtk proxy build/qt/characters3d/character_preview_3d --target 60
```

Click to set a camera-projected ground target; Left/Right choose the matching
screen direction, Space stops, I applies an impulse and P pauses. The gold
marker shows the destination. D toggles world-space foot dots (green planted,
gold swinging), so supported turns can be inspected. Without a target the
preview starts at rest. Captures and bounded scenarios:

```sh
rtk proxy build/qt/characters3d/character_preview_3d --target 80 --reversal --duration 8 --capture /private/tmp/werewolf.png
rtk proxy build/qt/characters3d/character_preview_3d --target 80 --duration 10 --hide-after 4
rtk proxy build/qt/characters3d/character_preview_3d --transparent --duration 2 --capture /private/tmp/werewolf-alpha.png
```

`--duration` bounds execution when explicitly supplied. Otherwise the preview
stays interactive. Capturing performs one readback at completion; there is no
CPU readback/composition loop. A 192×128 orthographic GPU pass feeds an explicit
192×128 ShaderEffectSource enlarged 4× with `smooth: false`. The source View3D
also has those physical texture dimensions; a larger intermediate linear image
is not baked before nearest enlargement. Palette dithering and temporal AA are
off. The preview background and labels can be removed for alpha verification.

## Motion and coordinates

`pose.h/.cpp` is Qt-free. It shares the 2D grounded controller's horizontal
navigation, planted anchors, reach-triggered stepping and second-order body
tracking. Its contact planner uses a conservative equal-link planar proxy
because the shared 2D definition supports equal segments. This is only contact
planning: the separate 3D solve uses the actual unequal, lateral 3D rest lengths
from the Blender rig. The 2D affine pose never becomes a 3D pose format.

Blender meters +Z up/-Y forward convert to glTF/Qt meters +Y up/+Z forward.
Preview target units are centimeters. World ground anchors are transformed back
into the yawing body's coordinate frame before solving. Both legs can lower the
pelvis to maintain reach during a turn. Rest quaternions and parent world
rotation convert solved directions into actual deform-node local rotations.
A reversal pauses navigation for alternating lifted foot replants in small yaw
increments. A supported foot retains both its world sole contact and its own
world yaw; a lifted foot turns with its replant. The torso follows the supported
stance, preventing the legs from crossing during a body-only half-turn.
Body/head/arm/tail response is generated from acceleration and velocity, with
no imported walk animation. The motion timer stops at exact settling and pause;
hiding via the preview's command suspends it too. Native rendering quiescence
still needs verification.

## Verified and pending evidence

- Qt-free tests check actual segment lengths, unreachable-target bounds,
  shared planted anchors, arrival and repeated reversal, planted contact/yaw
  stability, world contact reconstruction, lifted foot clearance and lateral
  knee/ankle separation.
- `--verify-joints` loads the real converted skin without initializing a GPU;
  checks all 22 live joints, changed thigh transforms, and actual Qt scene-space
  sole contacts throughout forward motion, stopping and supported yaw replants
  (maximum allowed error 0.0002 meters). Both traversals settle. The live binding
  check also samples wave/point/present upper-arm clearance against the authored
  cheek taper, checks left/right held elbow clearance and held motion.
- `--verify-presentation` checks actual QML source/output texture sizes, nearest
  enlargement, meter-scale camera settings, ground/contact markers and
  camera-projected target selection without claiming rendered pixels.
- The native GPU test demands nonblank model pixels and transparent background.
  It skips with code 77 only for explicit graphics-backend unavailability; QML,
  conversion, CLI or blank native frames fail.

This sandbox's offscreen Qt platform selects a non-RHI software backend, which
cannot render View3D. The GPU test therefore skips: no Qt-native werewolf pixel,
GPU cost, WindowServer, transparency, scaling, settled-render or energy result
is claimed. Blender reference renders and transform tests do not replace that
acceptance. Native launch/capture, extreme skinning inspection and matched 2D/3D
resource comparison remain necessary before resident 3D integration.
