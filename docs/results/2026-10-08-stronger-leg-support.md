# Stronger leg support and centered 3D paw steps

The user's native trial found Mallow and the 2D werewolf sitting low enough that
their body artwork slid along the floor. The 3D werewolf's paws stayed ahead
during walking and came underneath only near arrival. This feedback supersedes
the previous spring pass's implication of visually adequate support.

## Correction

Shared vertical spring stiffness increases from 320 to 1000, with damping from
24 to 32. Body weight is now independent of stiffness (`4 * minimum leg reach`);
each spring's natural rest length is calibrated to the supported stance. This
avoids increasing gravity along with spring strength, which previously defeated
the attempt to reduce sag. The stance reserve is reduced from 7% to 3% of limb
reach, exposing more leg below the body. Reduced fixed weight also limits the
fall during overlapping lifted recovery. The existing bounded vertical response
and smooth unloading remain. Horizontal navigation and foot timing are unchanged.

The 3D model's authored paw sole lies forward of its ankle. The planner previously
carried that resting sole offset into each future landing. It now centers the
neutral support target on the hip projection and uses a smaller landing lead
(0.1 of reachable interval width instead of 0.3). The shared planner's new bounded
`setLandingLeadFraction` hook defaults to 0.3, preserving 2D foot placement.
Actual 3D sole offsets are still used when solving and validating deform IK.
This centers the paws while preserving exact planted contacts and limb lengths;
it does not add a competing hip rotation force after IK.

## Evidence

Both original failures were reproduced by regressions before correction. Actual
2D packages at 1/2/3× with continuous and quantized presentation now keep each
hip at least 72% of limb reach above its ground ankle target. Conservative
transformed bounds of the entire trunk textures stay at least one logical pixel
above the floor, including transparent atlas corners. The tests do not merely
check the hip or ignore body artwork.

The 3D regression excludes startup and rest (`abs(root) > 10 cm`, speed above
20 cm/s). Each paw passes from ahead through within 2.5 cm beneath its hip and
behind during travel, in both directions and at 0.5/1/2/3×. Recentring alone
did not pass sustained walking, so the remaining landing lead was reduced too.
Existing 6,000-interval contact continuity, side clearance, exact segment/contact
geometry, arrival, reversal and settled-idle checks remain passing.

Fresh Release build passed. Full CTest: 19 passed, 3 environment skips (two native
socket checks and GPU framebuffer rendering). Python suite: 50 passed. Additional
live 3D deform-joint/contact verification at 3× passed for all 20 joints.

Software 2D review output is under `results/stronger-support-motion/`, with 40
frames per character at 50 ms intervals and raw capture metadata. These captures
include `support-review.gif` and an inspected `support-comparison.png` against
the previous pass. The comparison shows higher body carriage and more visible
leg clearance at matched timestamps. They are review evidence for the 2D pose;
native 3D appearance and battery acceptance
remain open. Quit and relaunch rebuilt binaries before trying the corrected pets.
The 3D character remains a preview; the resident integration plan is unchanged.
