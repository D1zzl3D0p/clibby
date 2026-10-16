# Shared spatial motion correction

The user found 3D squat between steps and near-simultaneous hopping in 2D, and
requested one motion system with 2D as a planar special case of 3D.

## Findings

The two renderers already shared body navigation and foot planning, but used
separate IK implementations and dimension-specific body support rules. Their
reach clamps differed. A planar/spatial knee parity regression failed before
extraction. The new spatial IK kernel owns validation, reach limits and bend
projection; the planar wrapper embeds zero-depth inputs and projects the result.

Per-foot urgent reach recovery bypassed the global launch order. At 2x travel,
Bramble launches were separated by about 42 ms despite 260 ms swings; both
feet were airborne about 72% of the time. Mallow reached 74%. This explains the
paired hopping pattern rather than a problem in how either renderer draws.

The 3D gravity parameter was coupled to the stance reserve by a minimum load.
Reducing gravity alone could not soften flight. Gravity and stance calibration
must be independent in the shared support model.

## Architecture

The retained 2D renderer and Quick3D renderer remain presentation adapters.
Common motion belongs in the Qt-free character core. Sprite characters need
neither a mesh conversion nor GPU rendering to share spatial motion; their
depth coordinates are zero. Different limb dimensions may require different
tuning, but the support equation must not depend on renderer or dimensionality.

The IK parity regression failed with `planar/spatial knee solving differs`
against the original independent solvers, then passed after both wrappers
delegated to `pets::motion::solveTwoLink`. Reach clamping now has one owner.

## Final implementation

`pets::motion` owns spatial IK, axial support integration and neutral spring
calibration. `GroundedController` owns navigation, alternating phases, contact
loading and vertical state. It embeds planar hips/ankles with zero depth; 3D
supplies actual yaw-transformed geometry, measured lengths and sole contact
flags in the same downward-positive frame. External geometry is consumed once
per interval and cleared on reset. The `groundedOnly` force branch is removed;
the optional common upright assist is zero in both current profiles.

Gravity is independent of stance reserve. One helper calibrates neutral spring
lengths against the desired stance and body weight. 2D defaults: stiffness 100,
damping 20, gravity per minimum reach 4, reserve 3%, contact loading 12 ms. 3D:
stiffness 120, damping 24, gravity per reach 6 (previously 12), reserve 2.5%,
contact loading 60 ms.

The planner admits one swing at a time, alternating with a 40 ms floor transfer.
Ordinary swing duration and horizontal body speed are unchanged. If travel
exceeds floor reach, the stance foot explicitly recovers along the floor and
remains loaded. This is stylized recovery, not simulated friction. Fixed floor
contacts are retained while anatomically feasible. Actual 3D planted reach
correction no longer adds an unnecessary 8 mm toe-off; `Pose.grounded`
distinguishes physical support from visual recovery.

An ongoing-launch regression found Mallow could hold its first swing forever:
body lean raised one hip past floor reach. A 3% stance reserve resolves that
clearance without shorter steps. Both characters complete at least eight
launches per side in six seconds at every 0.5–3x setting. Traces show alternating
Mallow launches every 275 ms and Bramble launches every 300 ms, with zero
simultaneous planned swings. Actual loss of both contacts still produces gravity.

Planted median extension is about 96% in both 2D pets, idle 97%; brief transfer
compression remains. One-second 3D walking checks measure 2.4–7.4 cm maximum hip
drop across speeds, versus over 22 cm previously. These are geometric checks,
not aesthetic acceptance.

## Verification

Fresh full build succeeded. CTest: 19 passed, 3 environment skips (two native
socket checks and GPU framebuffer). Python: 50 passed. Live skin verification at
speed 3 passed for all 20 joints, rotated soles, independent paw pitch, wave
continuity and held-arm clearance. Regressions cover planar/spatial IK and force
parity, calibrated equilibrium, gravity-only flight, ongoing alternating trips,
unchanged swing timing and body cruise speed, exact limb/contact geometry, floor
clearance and settling.

Final raw 2D captures, reviewed contact sheet, 40-frame GIF and CTest log are in
`results/shared-spatial-motion/`. Intermediate held-foot captures are preserved
in `results/shared-spatial-motion-before-stance-clearance/`. Native 3D pixels
remain unavailable here; restart rebuilt binaries for user review. Battery
impact and resident 3D integration remain separate open work.
