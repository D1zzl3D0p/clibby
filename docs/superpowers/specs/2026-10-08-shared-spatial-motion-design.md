# Shared spatial motion

User goal: remove repeated 2D/3D motion fixes, reduce the 3D squat between
steps, and prevent 2D steps bunching into hops. Preserve body-led horizontal
travel, eased lateral acceleration, normal swing duration and independently
rotating 3D paws. Treat 2D motion as a planar case of spatial motion.

## Chosen scope

Keep the retained sprite renderer and Quick3D skin renderer as presentation
adapters. Move common behavior into the Qt-free character core rather than
forcing sprites through a GPU mesh renderer. Both already consume the same
navigation and foot planner; consolidate two-link IK and contact support too.

One spatial two-link solver accepts origin, target, segment lengths and a bend
pole. The 2D wrapper embeds coordinates at zero depth and projects the result.
One spatial support integrator accepts actual hip/ankle vectors and contact
flags. The 3D adapter supplies posed geometry, including yaw, independent paw
pitch, reach correction and actual floor contact. The 2D adapter supplies planar
geometry. Spring, gravity, damping and any upright assist are common scalar
parameters; there is no dimension-dependent force branch.

Gravity must be independent of the stance reserve. Contact denial must reduce
support and allow downward acceleration. If stylized upright assistance is
needed for short legs at the selected travel speed, it must be an explicit
common tuning parameter rather than a 2D implementation fork.

Investigate actual launch timing before changing gait. Reach recovery must
retain distinct left/right phases instead of repeatedly synchronizing them.
Normal swing duration and horizontal travel remain unchanged. Do not claim
strict alternating floor contact if high-speed travel exceeds anatomical reach.

## Implementation and acceptance

1. Capture failing planar/spatial IK parity and bunched-step regressions.
2. Extract spatial IK; delegate the 2D and 3D public wrappers to it.
3. Extract support math and remove the dimension-specific profile branch.
4. Supply actual spatial support geometry from the 3D adapter; calibrate gentle
   gravity independently of resting bend. Correct launch arbitration centrally.
5. Verify parity, real airborne acceleration, limb/contact preservation, phase
   separation, unchanged swing timing, settling and live skin transforms.
6. Produce fresh 2D captures and run the full existing Qt/Python checks.

Native 3D appearance and battery impact remain user review/measurement tasks.
Resident 3D integration remains outside this motion correction.
