# Body-led motion correction

The user reports that stride-only tuning leaves Mallow too slow and makes the
werewolf jerk, and requests the body move with the legs keeping up. This record
supersedes the adjustable-speed and stride-distance corrections.

## Result

Travel speed (0.5–3×, default 2×) now scales authored body navigation speed.
Acceleration, braking, target arrival and desktop limits control body placement;
foot reach, support transfer and swing duration no longer cap or pause it.
Saved settings and the existing `walkingSpeed`/`--speed` interfaces remain usable.

Each foot follows the presented body independently. Anticipatory touchdown and
reach-triggered lifted recovery preserve actual limb lengths, including unequal
werewolf links. Recovery steps can overlap when short legs cannot support the
requested speed with strict alternating contacts. Ordinary authored swing timing
remains unchanged, but extreme travel can demand earlier catch-up. This is a
visual follower, not a simulated force-driven walk.

A constant stance reserve replaces foot-dependent body lowering. Grounded torso
transforms use the stable navigation baseline; head, ear, tail and facial
expressions remain. The 3D werewolf also translates during heading changes,
decelerates normally on reversal and updates foot heading during lifted motion.
Its adapter corrects lifted targets against actual 3D leg reach and anatomical
side clearance without lowering the pelvis or stopping the root. It may travel
backward temporarily while turning toward a new target.

## Verification

Fresh Release build completed. CTest: 19 passed, 3 skipped (two native socket
checks and native GPU rendering are unavailable in this execution environment).
Python unittest suite: 50 passed. The preview CLI checks and live 3D joint-binding
checks passed as part of CTest.

Regression coverage checks independent navigation trajectories at multiple speed
settings, sustained authored cruise speed, no foot-induced cruise stalls,
constant torso height, exact limb/contact geometry, host placement quantization,
braking reversal and eventual foot/secondary settling. 3D checks retain side
clearance, live deform binding and wave/hold contact validation.

Software 2D captures for both pets at 2× are saved under
`results/body-led-motion/`: 24 frames at 50 ms intervals, raw metadata,
`body-led-review.gif` and `contact-sheet.png`. The captures show forward body
progression and intact silhouettes, but do not establish native visual acceptance.
No native GPU visual or battery claim is made. User judgment of the revised feel,
especially overlapping recovery at high speed, remains open. Quit and relaunch
the rebuilt resident or previews before trying this correction. Feature work
remains paused for movement quality.
