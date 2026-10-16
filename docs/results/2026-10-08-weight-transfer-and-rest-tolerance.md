# Weight transfer and comfortable rest

The user's native review found sliding 2D feet, too little visible weight-shift
bob, punishing one-foot/airborne phases, repeated tiny 3D arrival steps and
occasional apparent loss of leg power. This supersedes the prior strict
single-swing/floor-recovery visual acceptance assumptions.

The correction must preserve shared spatial motion, body-led horizontal travel,
ordinary step duration, independent 3D paw pitch and real downward motion in
flight. It should reduce sustained planted-foot sliding, retain distinct left
and right step phases, soften weak support and allow comfortable resting feet
without exact recentering. Native appearance remains the user's acceptance.

## Investigation

At 2x travel the strict single-flight policy made Mallow recover along the floor
on 74% of stance frames (275 px over three seconds), and Bramble on 54% (371 px).
Body motion was only about 0.65 px and 1.04 px respectively. The user observation
comes from the shared planner's sustained floor-tracking fallback, not rendering.

The support equation also gated all grounded damping behind positive spring
compression: extended contacts could lose spring and damping together. The
recovery needs to preserve grounded damping without inventing airborne support.

3D corrected contact positions are retained for continuity. A raised contact
was investigated as a possible persistent-airborne condition, but finite repeated
impulses detached both paws and then recovered successfully, including with the
wider resting zone. This candidate is not established as the native failure.
The confirmed support discontinuity is addressed in the common force code.

The 3D adapter also ignored the shared planner's `recovering` state. The new
lifted reach-recovery state must be propagated with a continuous onset rather
than interpreted as a fixed floor contact. That mapping now uses an 80 ms
quintic blend from the presented sole.

## Corrections established so far

Reach emergencies now admit overlapping swings only after the other foot passes
half its swing and at least half a swing duration since the last launch. This
keeps distinct phases without shortening normal swings. A contact stays fixed
while on the floor; unavoidable reach correction lifts the sole before moving
its anchor. An idle recovery returns to the floor rather than remaining raised.

The common support kernel keeps contact damping at full extension. Effective
gravity scales smoothly from full strength with two loaded contacts to half
strength without contact; downward airborne drag softens descent. No artificial
height cap or airborne ground force is introduced. Rest detection uses quiet
vertical velocity and acceleration rather than forcing the original body height.

The 3D home radius is 4 cm, up from the shared 0.5 cm threshold. Routine launches
consider both remaining travel and comfortable foot position. The same radius
applies to heading changes: tiny backwards adjustments do not force a full turn.
The micro-tap regression failed when only the planner tolerance reverted to
0.5 cm, then passed after restoring 4 cm. Both floor recovery and actual 3D paw
rotation remain continuous.

The initial soft-support pass removed sliding but reduced regular bob to 0.26 px
for Mallow and 0.49 px for Bramble at 2x. That result does not satisfy the requested
more visible weight transfer, so it was retained as an intermediate capture and
not treated as final motion acceptance. Final bob tuning and checks follow.

Passive foot-placement probes showed forward touchdown provides longer useful
stance, without floor sliding. Startup-excluded checks selected shared defaults
of stiffness 200, contact damping 7 and landing lead 0.3; 3D retains its explicit
measured-rig tuning and lead 0.1. The support equation is unchanged between
characters and dimensions. Regular 2–6 second bob at 2x measures about 0.77 px
for Mallow and 2.92 px for Bramble, compared with the intermediate 0.26/0.49 px.
No looping bob offset or contact-independent body force was added. A stiffer
250 profile was rejected because it produced held-flight/large-bounce behavior.

## Final verification

The repeated-bob regression measures frames 240–720 at 120 Hz and requires at
least eight vertical direction changes. It failed against the initial soft
profile, then passed with the selected touchdown/support tuning. Other checks
cover fixed actual floor anchors, minimum launch gap and phase separation,
unchanged swing duration, continuous extended-leg damping, gentler positive
airborne descent, truthful limb/contact geometry, floor clearance, quantized
travel, reversal, idle touchdown and equilibrium settling. Repeated 3D impulses
also detach both actual paws, regain floor contact and settle with the wider home
tolerance. The specific native occasional power-loss event remains unconfirmed;
the established damping discontinuity and ignored recovery mapping are corrected.

Fresh full build succeeded. CTest: 19 passed, 3 environment skips (two native
socket checks and GPU framebuffer). Python: 50 passed. Live 20-joint skin
verification at speed 3 passed, including independent paw pitch, exact sole
reconstruction, wave continuity and held-arm clearance.

Raw final captures, reviewed contact sheet, 40-frame walking GIF, probe source/
outputs and CTest log are in `results/weight-transfer-and-rest-tolerance/`.
The initial small-bob captures are retained in
`results/weight-transfer-and-rest-tolerance-before-bob/`. Quit and relaunch the
rebuilt resident/previews for native review; software evidence does not establish
visual acceptance, native GPU pixels, desktop parity or battery impact.
