# Natural turning and restored pace — 2026-10-17

> Rejected on visual review: this candidate still takes too many placements and
> loses upright body height during turns. The checks below established speed,
> contact positions and link lengths, but did not establish acceptable posture.
> A subsequent fresh 34-rig audit measured Biscuit at 56.2% of standing hip height
> during its turn. Treat the implementation and visual-success claims below as
> historical evidence, not acceptance of the current motion.

## Directional leading plant (2026-10-18)

The user rejected the preceding foot-yaw-led candidate because it still looked
like spinning. Fresh diagnosis showed the first Biscuit plant occurred after
32° torso rotation and .777 leg reaches of body travel in the 110° fixture. The
foot goal was derived from an already-selected future body pose, so foot-yaw
anticipation did not establish a foot-led support transfer.

The replacement chooses an independent world touchdown along the requested new
heading, holds body/root heading until that paw plants (about .16s), then moves
the body toward that actual fixed plant. The remaining placements overlap the
fore/hind support transfers. Ordinary turns retain six quadruped placements in
.84s, with at least two grounded contacts, fixed loaded sole positions and fixed
loaded foot headings. Bipeds retain their two-placement leading-plant sequence.
Normal walking strides and 70cm/s base / 140cm/s default2× pace are unchanged.
Wide, short-legged quads can temporarily narrow the fore stance; the final fore
placements restore its authored width. Full-plan reach, posture and bounds
admission is finite (three parameter families, three heights, two leading-paw
choices), evaluated only on plan admission. Safe retargeting uses an all-landed
boundary; world reconciliation translates every new waypoint. Corrections of
at most .1° do not launch another multi-foot turn or proxy catch-up steps.

**Calibration correction:** the earlier broad audit incorrectly treated authored
rig yaw metadata as the live native setting. An independent Qt-property probe
confirmed the previous app used -10.8° for all quadrupeds: a 158.4° reversal,
not the 110° fixture used by most old audits. Native capture checks did use the
real angle, but did not establish upright height or first-plant causality. Those
old results are retained with scope errata; they are not retroactive native
validation. This correction intentionally restores quadruped authored
`presentation.yaw_bias_degrees` (default -35° for legacy Marmalade), rather than
the hardcoded value matching the camera target offset. Five quads now have a
110° turn; Tarn uses its authored -25° view and a 130° turn. This visibly restores
the authored three-quarter resting view. Other biped views are unchanged,
including Nacre/Windle at the existing -35° runtime setting.

The post-build Qt probe reads all34 actual stage properties and all80 actual leg
chains. Geometry matches the audit inputs exactly; final behavior checks use
the probed angle manifest without a guessed fallback. Explicit API angles above
140° retain the bounded earlier paired fallback; those checks preserve heading,
reach, support and placement limits, but do not claim this new leading-plant
style. The resident catalogue uses the new directional route throughout.

Biscuit focused measurements: .24R leading step, .240R ahead of its actual hip,
<.03° torso rotation at first plant and <.001R preplant body travel. While the
plant stays fixed, the body initially advances about .20R toward it. Mossback's
wide family uses .57R, <.04° at plant and then about .55R body approach. These
are authored style criteria, not biological constants or a force-balance model.
The earlier wide-family prototype reached90% standing hip height; full product
admission chooses the highest feasible tested height. Native prototype captures
alone were not used to claim those posture limits at the former158.4° angle.

Evidence lives under ignored `results/directional-plant-2026-10-17/` with old/new
runtime manifests, exact geometry comparison, red regression logs, copied source,
critical-review reproductions and controlled same-view before/after rendering.
Final validation: **1,632 ordinary cases pass**, using all34 actual native
profiles and leg geometries, both directions, .5/2/3×, 30/120Hz, continuous/
rounded hosts and face/navigation. All816 stationary turns use exactly two
biped or six quad placements; actual first plants are forward and ahead of the
hip, torso rotation at plant is at most .0378°, minimum turn hip/rest ratio is
.901444, extension at most1.00309, loaded translation/yaw drift zero. Navigation
arrives and reaches35/140/210cm/s respectively. The first final audit found one
Mossback3× rounded-arrival rejection; an appended .58R family fixes it, with
.9065 hip ratio and no extra steps. Failed raw results are preserved, and the
entire1,632-case matrix was rerun on the corrected source. Root independently
checked its CSV for heading, arrival, first-plant criteria, support, reach,
posture, counts, contact pins and pace. Earlier144 explicit180° fallback cases
remain separately labeled with their own snapshot provenance.

Final application build and relocated-bundle test pass. The complete combined
Qt selection passes **88 checks with five environment skips (93 total), zero
failures**: 87 plus five skips in the92-test main run, then the separate passing
bundle check. Fresh critical review passes24 cases, zero-step small corrections,
and return to140cm/s navigation. Permanent regressions reproduce both the
subpixel extra-step bug and exact Mossback quantized-arrival rejection before
fixing them. Native Biscuit/Mossback/Tarn geometry, contacts, walk/reverse, gaze,
drop, signature, pause/cancel and rest checks pass; all180 checked capture frames
have interior alpha bounds. Captured default2× plans are unchanged by the later
subpixel/failed-arrival-only fallback fixes; those branches have separate tests.

The controlled same-view comparison is `biscuit-leading-plant.gif` under the
ignored result folder. Both sides use the new authored viewing angle; the left
runs the previous pose implementation, the right the directional plant. Raw
native frames are preserved. The preview centers its camera on the host root,
so the comparison restores horizontal host translation from exact-rig motion
traces (`capture-root-before/after.csv`); it does not alter character poses.
The earlier prototype GIF mixed old/new viewing angles and is retained only as
prototype history. `delivery-source-hashes.json` records final product sources.
Restart `build/qt-humanoids/Clibby.app` to load the final implementation. No
commit or push. Python and 2D-only results were not rerun for these spatial changes.
Visual naturalness still requires the user's assessment.

## Previous foot-yaw-led candidate (subsequently rejected)


The prior upright point-pivot still looked like spinning. Its replacement makes
bipeds place the leading foot before the torso follows the new support, then
brings the trailing foot through. Quadrupeds use fore/hind/fore pair transfers,
with staggered paw lifts and at least two grounded paws. Loaded foot headings
remain fixed. Ordinary resident turns need two biped or six quadruped placements;
full 180° quad plans try three transfers first and use five only when needed for
upright reach. A complete plan is checked before admission; infeasible plans
stop quietly. Mid-turn requests replan after touchdown. A new destination during
blocked recovery is preserved.

Original navigation speed remains 70 cm/s base, 140 cm/s at default 2×. Existing
longer walking strides, shoulder and pickup corrections remain. A turn briefly
transfers support before forward travel; it does not lower the selected walking
speed. This is authored kinematic support transfer, not a COM or force simulation.

Fresh independent audit: **816 ordinary cases plus 72 full-180° cases pass**.
The same leading foot translates at least 3% of measured reach and turns 20°
before torso rotation exceeds 10°. First touchdown occurs by 32° torso yaw.
Planted translation and yaw drift are zero; quads retain at least two supports.
Minimum turn hip/rest ratios are .918686 ordinary and .906911 full-180°;
maximum measured link extension is 1.00309. Final headings are checked against
the original request, preventing cancelled turns from passing falsely. A separate
24-case critical review passes recovery, retargeting, pinning and bounds cases.

Raw metrics, 6,510 per-foot frame records, snapshots and reproduction harnesses
live in ignored `results/foot-led-turn-2026-10-17/`. The broad audit preceded one
focused recovery-only assignment fix (`turnRejected_=turnFaceOnly_`); that delta
is covered by permanent 30/120 Hz biped/quad tests and the fresh critical review.
Visual acceptance remains the user’s judgment.

Fresh delivery checks: complete combined Qt suite **88 passed, five environment
skips (93 total), zero failures**; full application build and bundle relocation
pass. Native Biscuit, Nell, Mossback and Puddle each pass live geometry, contacts,
walk/reverse, gaze, drop, signature, pause/cancel and rest checks. All 240 captured
native frames have interior alpha bounds (no border clipping). Native comparison:
`results/foot-led-turn-2026-10-17/final-native/foot-led-turns.gif`.
The rebuilt application is `build/qt-humanoids/Clibby.app`; restart it to load the
new turn. No commit or push was made. Earlier Python/2D evidence is retained as
historical rather than reported as a new run; no Python or planar motion was
changed in this correction. Packaging still emits missing optional SQL-driver
dependency diagnostics; the application build and relocated-bundle smoke test
both complete successfully.

## Upright point-pivot — subsequently rejected for still looking like a spin

The user rejected this candidate's feel despite its passing geometry checks.
The next correction is recorded in the active handoff and foot-led plan; do not
interpret the counts and checks below as visual acceptance of this point-pivot.

The previous candidate kept measured links short by lowering the whole body.
A fresh 34-rig diagnosis measured Biscuit at 56.2% of standing hip height during
a stationary turn. Airborne paws, rather than loaded supports, caused most of
that collapse. Mossback also exposed infeasible touchdown geometry.

The replacement plans a finite body/root arc about one loaded contact, unloads
the other feet, then replants the old pivot. Open-ground turns use exactly two
biped or four quadruped placements, without subsequent planner catch-up steps.
Upright height, reachable lifted ankles and world contacts are planned together.
The host realizes the travel-axis arc; transverse offsets remain through
walking, reaches and pickup. A blocked arc lands its unloaded feet and stops.
A full arc that cannot fit the available root/sole bounds is rejected quietly,
retaining the presented heading instead of repeating tiny partial turns.

This compact quadruped gesture briefly unloads three paws. It is an authored
dynamic point-pivot approximation, not a verified casual quadruped turn or a
static balance simulation. Visual acceptance remains a separate requirement.

Walking now predicts the midpoint of the following planted interval. Spatial
bipeds opt into that forecast and a longer anatomy-limited swing; planar defaults
are unchanged. Paired quadrupeds use a shorter coordinated load-transfer period
and the actual stance forecast, with subtle phase compression. The accepted
70 cm/s base and 140 cm/s default remain unchanged. Tiny settled adjustments
within 4 cm keep their comfortable stance instead of scheduling extra steps.
Targets during landing/recovery retain the shared controller's acceptance
policy, preventing a moving perch from accidentally requesting a floor turn.

Exact-source independent evidence is in
`results/upright-turn-independent-2026-10-17/`:

- **816 turn cases:** all 34 rigs, .5×/2×/3×, 30/120 Hz, continuous/rounded hosts,
  face-only and navigation requests. All settle; all 408 face-only cases have
  exactly two/four total placements; minimum turn hip/rest ratio 96.94%;
  maximum link extension about 0.309%; planted-contact translation and proposal
  mutation are zero; navigation arrival is within .75 cm.
- **102 steady-walk cases:** requested 35/140/210 cm/s retained. Every biped stays
  above 94.7% standing hip height. At default speed over 12 m, Nell drops from
  37 to 24 placements; Biscuit 78 to 70; Marmalade 70 to 52; Mossback 114 to 98.
- Ordinary quad walking still has limits: Mossback reaches 87.8% standing height
  at default speed, and slow-speed cadence is still clock driven. These are not
  claims that all gait aesthetics or anatomy constraints are solved.
- Later tiny-adjustment admission and persistent-block recovery changes are
  covered by permanent failing-then-passing regressions; their branches are
  outside the independent long-route/face-only matrix. Pickup continuity,
  newer-facing requests and recovery interruptions have separate independent
  checks under `results/upright-turn-review-2026-10-17/`.

Fresh combined app build and complete checks pass: **88 passed, five environment
skips**. True 2D-only app: **36 passed, four skips**. Bundle relocation passes in
both configurations; Python **109 passed**. Native 2× captures cover Nell,
Biscuit, Puddle and Mossback: **240 unclipped 96×96 frames**, with live contacts
and joint placement verified. Preview and per-frame evidence are under
`results/upright-turn-diagnosis-2026-10-17/delivery-*`.

The final integration pass also corrected arm reach/bend handedness after
persistent torso translation: explicit targets remain torso-relative, and arm
side derives from rest anatomy rather than a translated shoulder's sign. Full
joint-binding checks cover sleeve/cheek clearance and exact grip targets.
No battery-impact claim follows from these checks.

Restart `build/qt-humanoids/Clibby.app` to use the rebuilt candidate. No commit
or push was made. The user's assessment of the rendered point-pivot gesture
remains the acceptance criterion for its feel.

## Earlier candidate — rejected on visual review

This corrects the gait portion of the earlier companion-polish pass. The user
rejected its excessive turning steps and severe travel-speed reduction. Pickup
and shoulder changes remain in place.

## Research and root cause

The [research report](../research/reports/Natural%20companion%20turning.md) links
primary human and animal biomechanics sources and separates observations from
animation choices. Healthy brisk human standing half-turns commonly used about
two to three placements in the cited experiment. Turning can include a loaded
support pivot; a planted contact is not necessarily a permanently yaw-locked
flat sole. Quadruped studies support coordinated and asymmetric contacts,
including overlapping swing intervals. No reviewed source establishes a
universal ordinary capybara half-turn placement count.

The previous implementation imposed a .55-radian limit against every foot's
heading, making feet advance through repeated rounds. Biscuit's usual resident
turn required sixteen individual placements. It also replaced the original
70 cm/s base navigation speed with a formula divided by contact count and step
duration. At the default 2×, Biscuit dropped from 140 to about 36.72 cm/s before
additional phase-dependent speed reduction.

## Revised implementation

- Restore the accepted 70 cm/s base and 240 cm/s² acceleration for nonwinged
  companions, with the original .5×–3× multiplier. Preserve the prior measured-
  wing-reach travel calibration for winged characters. Remove the phase envelope
  that reduced cruising speed.
- Let leg reach influence cadence, predicted contacts and support height, while
  preserving the requested pace. Short legs need faster recovery at high speed;
  slowing navigation is not the workaround.
- Bipeds use a finite two-placement pivot/step turn for ordinary resident angles
  and three placements for a full half-turn. Foot position remains planted
  during supported yaw pivots.
- Quadruped travel can coordinate diagonal pairs with two supports, replacing
  the serial single-paw mutex. Their compact turns use alternating forequarter
  and hindquarter placements, sized to the anatomy. Biscuit's normal resident
  turnaround now uses four coordinated beats/eight paw placements.
- Shift the body laterally within horizontal leg-reach constraints during
  compact turns, so short/wide bodies can reorient without collapsing or
  stretching. Renderer pelvis and default arm chains consume the same shift.
  This is kinematic reach accommodation, not a proven center-of-mass or force
  balance solution.
- Finish current travel swings and bounded braking before the planned turn;
  target changes replan at a placement boundary. Pickup/reset discard the plan.
  Reset initializes arm followers only after new contacts and turn state exist.

## Evidence and checks

Test-first regressions reproduced the speed loss, excessive turn count, short-
body failures in the first prototype and arm-height initialization from stale
contacts during reset. New tests require original cruising speed, finite useful
placements, positional planting, bounded leg extension, actual short-body rigs,
rapid retargeting, host quantization/blocking and quiet arrival.

Independent source snapshots and harnesses live in ignored
`results/natural-turning-2026-10-17/`. The final fore/hind candidate matrix covers
**204 cases**: all 34 current anatomies at .5×/2×/3× and 120/30 Hz. Routes include
long straight travel, reversal, rapid mid-turn changes and pickup/drop. Observed
cruising speeds remain **35/140/210 cm/s**, maximum link extension is about
**0.309%**, planted-contact translation is zero, and all cases arrive and settle.
Thirty-six additional stationary quadruped-turn cases pass. Sources and hashes
are retained beside results; earlier failed prototypes are kept separately.

Fresh combined Release/app build succeeds. Final combined checks: **88 passed**,
five environment skips, no failures. True 2D-only build: **36 passed**, four
environment skips, no failures. Bundle relocation checks pass after deployment
in both builds. Full Python suite: **109 passed**. Whitespace checks pass.

Native 2× captures cover Biscuit, Nell, Puddle and Mossback: **240 unclipped**
96×96 walking/turning frames, plus snapshots. The final captures and comparison
are in `results/natural-turning-2026-10-17/final-native/`. Representative native
frames and turn sequences were reviewed visually. Independent source review
caught the reset arm-ordering issue, which has a failing-then-passing regression;
the follow-up review found no remaining actionable issue. A bounded-perch
regression also exposed a neutral-foot target outside the usable support interval;
all home comparisons now use the bounded reachable target, and bird edge walking
settles again. The anatomical matrix audits the fore/hind candidate snapshot;
subsequent reset/bounds fixes and state cleanup are covered by the permanent
regressions and final complete suites.

Restart `build/qt-humanoids/Clibby.app` to use the correction. No automatic
commit, push or change to existing user appearance preferences was made.

## Limits

These are research-informed procedural approximations. Fore/hind turn beats and
their exact timings are authored animation decisions, not a measured universal
animal sequence. Foot contact uses a point-pivot approximation rather than a
complete heel/toe pressure patch. The tests establish kinematic reach and
continuity, not full rigid-body balance, biological equivalence, battery impact
or subjective acceptance of every character's motion.
