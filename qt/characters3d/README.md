# Optional original low-poly werewolf preview

The resident and separate preview share the optional Qt Quick 3D motion path.
The original Blender werewolf model lives in `characters/werewolf-3d/`.

With compatible Qt Quick 3D, ShaderTools, QuickTimeline and Balsam installed,
configure normally with `CLIBBY_WITH_3D=ON` (default). The optional module
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
into the yawing body's coordinate frame before solving. Turns preserve upright body height. A biped plants its leading foot before the
body follows, then brings the trailing foot through. A quadruped authors a world
step along the requested heading independently of the future body pose, plants
that leading paw with the torso held, then follows the new support through
staggered fore/hind transfers. Ordinary resident turns use two biped or six
quadruped placements; quad duration stays .84s and at least two contacts remain
grounded. Loaded yaw stays fixed. Short/wide rigs can temporarily narrow the
fore stance, restored by the final placements. Admission checks entire plans for
reach, upright height and bounds; reconciliation translates all stored waypoints.
Retargeting waits for an all-landed boundary. Errors <=.1° align without paw
steps. Explicit API reversals above140° retain the earlier bounded paired
fallback (up to ten placements), rather than claiming the new directional style.
This is authored kinematic support transfer, not a force-balance simulation.
Resident quadrupeds use rig `presentation.yaw_bias_degrees` (default -35°) rather
than the former hardcoded -10.8°; runtime view/geometry probes define audit input.
The host realizes the travel-axis arc; transverse pose offsets remain through
later walking and pickup. Completed spatial contacts synchronize shared stance
positions, preventing redundant recovery steps. Spatial biped walking forecasts the midpoint of the following stance and
allows longer anatomy-limited swings. Quadruped travel uses diagonal pairs
and forecasts the actual load-transfer interval. Ground travel speed is 0.85
times the shortest measured hip–knee–ankle chain length per second (bounded
to the planner's supported speed range). Longer legs therefore cover more
ground at roughly the same stepping rhythm. The saved travel-speed control
multiplies each companion's natural pace; winged ground walkers use the same
rule. Swing timing retains separate biped/quadruped coordination and turns
retain their own timing. Flight is unchanged.
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
