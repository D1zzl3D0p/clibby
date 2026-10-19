# Shared gait implementation audit

Audit date: 2026-10-17. Read-only inspection of the current working tree and
`git show HEAD:qt/characters3d/pose.cpp`. This note changes no runtime source or
build outputs. The working tree includes concurrent companion/appearance work.

## Pace regression

The user's original travel pace must be restored. Motion polish changed a
navigation parameter, rather than only changing its presentation.

In `qt/characters3d/pose.cpp`, HEAD's nonwing definition uses speed **70 cm/s**,
acceleration **240 cm/s²**, stride 26 cm, reach threshold 18 cm, lift 8 cm and
step duration .20–.30 seconds. `GroundedController::setSpeedMultiplier` multiplies
speed, retains base acceleration, and accepts .5–3. Resident settings default to
**2.0**, so the original nominal nonwing travel pace is **140 cm/s**.

The current definition takes the shortest measured leg reach R in centimetres,
sets stride `.32*R`, duration `.28` for two legs or `.24` for four, and then sets
speed `stride/(legCount*duration)`. Acceleration becomes `min(2*R,240)`.
`Motion::setSpeedMultiplier` also sets duration to `.28/multiplier` or
`.24/multiplier`. This preserves the newly reduced distance-per-cycle policy;
it does not recover the original navigation pace.

| Anatomy | Shortest reach cm | Original speed at 2× cm/s | Current nominal speed at 2× cm/s | Reduction | Current acceleration cm/s² |
|---|---:|---:|---:|---:|---:|
| Biscuit actual rig / polish fixture | 55.08525 | 140 | 36.72350 | 73.77% | 110.17050 |
| Biped polish test fixture | 85.21118 | 140 | 97.38420 | 30.44% | 170.42235 |
| Pip actual rig | 82.43 | 140 | 54.95 | 60.75% | 164.86 |
| Mossback actual rig | 37.09 | 140 | 24.72 | 82.34% | 74.18 |

These are parameter calculations, not timed native travel measurements. Reach
was calculated from rig joint head coordinates; the biped is explicitly the
fixture in `locomotion_polish_tests.cpp`, not a claim about every human package.

Current `Motion::propose` further scales desired navigation speed by
`.88+.12*cos(2*pi*phase)` during swing: range .76–1, phase mean .88. The exact
mean realized speed also depends on support waits, acceleration and braking.
During a turn with torso error >.35 rad, the scale is zero, so the controller
brakes while retaining the destination. Restoring `speed=70` while retaining an
unnormalized propulsion scale would still reduce the user's travel pace.

HEAD's wing-capable branch was an explicit exception: speed
`clamp(R*2.2,.1,500)`, stride `.45*R`, reach threshold `.3*R`, lift `.12*R`.
The current wing branch removes those overrides and shares the slower biped
policy. Decide deliberately whether restoring that historical wing exception is
required; do not describe all HEAD characters as having a 70 cm/s baseline.

## Why a fast turn still takes many steps

The limitation is structural, not merely the turn duration:

* `feetTurning()` stays true until every foot reaches the exact yaw target.
* `yawStepGoal()` limits the selected foot to ±.55 rad of **every** other foot.
  The slowest foot therefore limits each whole round of placements to .55 rad.
* The picker always chooses the largest yaw error, so a synchronized stance
  turns in rounds of all two/four feet.
* The torso target is restricted to the intersection of **all** foot yaw
  intervals ±.3 rad (`stanceTwist`), including the currently swinging foot.
  The global foot-heading spread contract substitutes for actual support and
  reach feasibility.
* Travel swing admission is disabled throughout this process. One short manual
  replant runs at a time, .08 s for bipeds and .065 s for quadrupeds. Shortening
  those durations makes a flurry of taps, not a turn with fewer placements.

For synchronized resting feet, no interrupted travel swing, and a turn angle A,
this recurrence gives `N*ceil(A/.55)` manual placements:

| Turn | Biped placements | Quadruped placements |
|---|---:|---:|
| Fixture bias .3 rad: A=pi-.6=2.54159 rad | 10 | 20 |
| Bias 10.8°: A=2.76460 rad | 12 | 24 |
| Full 180° | 12 | 24 |
| Biscuit actual 35° bias: A=1.91986 rad | — | 16 |

These counts are analytical reproduction of the selection/clamp recurrence,
not instrumented native animation counts. An existing travel swing or later
home recovery can change the observed total. HEAD's .4 rad increment needed
still more rounds; replacing it with .55 only reduces the count somewhat.

## Recommended ownership and finite turns

Keep desired navigation speed independent from anatomy. Restore the accepted
nonwing baseline `70*walkingSpeed` and acceleration 240; preserve host-realized
movement, braking, target cancellation and suppression. Keep anatomy-dependent
foot reach, body support, step lift and attainable cadence in the gait layer.
If phase modulation is retained, normalize its travel over a cycle so ordinary
distance/time still matches the accepted pace.

Replace the unbounded per-foot yaw catch-up loop with a finite turn plan:

1. Capture current world contacts, their true support state and any already
   lifted trajectory. Brake the old travel smoothly. A short finite turnaround
   can interrupt travel, but ordinary steady travel must recover its original
   pace immediately afterward.
2. For humans, choose two main placements: lead foot opens to an intermediate
   heading, trailing foot comes through to the new direction. Admit one final
   lead-foot alignment only when needed. Finish by plan completion/tolerance,
   not by repeated exact-equality catch-up of every foot to torso yaw.
3. During each placement, rotate the torso around the real stance contact and
   transfer support. Constrain pelvis height/translation from the measured leg
   chains and current contact geometry. The swinging foot's yaw should not cap
   torso yaw; a planted foot can retain its old world orientation while the
   body passes over it, within authored joint limits. A hard ±.3 rad yaw limit
   for every old foot is incompatible with two or three placements for a large
   turn and must be intentionally replaced.
4. Choose touchdown positions jointly with root/pelvis motion. Build the
   ankle target with the real sole offset and test its annular two-link reach
   and bend plane. Merely rotating neutral homes around a fixed center can
   send a quadruped's long fore/hind hip spacing outside short-leg reach.
5. For quadrupeds, plan coordinated fore/hind progression around a support
   pivot/arc, with inside/outside roles and maintained separation. Replant each
   paw once as a planned four-placement turn, allowing a bounded extra
   correction only if required by actual reach. Retain three-contact support
   where feasible; if a deliberately faster gait uses supported diagonal pairs,
   update that support contract explicitly and test its support geometry.
   Repeated all-paw yaw rounds are not a coordinated turn.
6. New targets can replace future placements while preserving the current
   lifted path and world basis through touchdown. Reset/pickup/support changes
   clear or transport the complete plan, exactly as current contact ownership
   requires. No timers or frame-history storage are needed.

Reach feasibility at the restored pace is a gait problem. In particular, a
four-contact, one-paw-at-a-time walk with fixed long durations cannot guarantee
both arbitrary high root speed and short rigid legs. Derive touchdown cadence
from desired travel distance and usable reach, and permit a deliberate gait
transition or shorter supported transfer when needed. Do not resolve that
conflict by silently capping navigation speed to `.32*shortestReach/cycle`.
Keep the existing body-height reach solve, but recognize that lowering the
pelvis alone cannot repair an ankle already outside horizontal reach.

## Test contracts to replace deliberately

`qt/characters3d/tests.cpp` currently requires each lift's yaw change ≤.550001
and at least 12 lifts across its staged-turn fixture (near lines 829–852).
Those assertions encode the shuffle mechanism and must be replaced by finite
turn-plan placement budgets, monotonic progress and reachable world contacts.
The torso-within-every-foot ±.3 rad assertion (near line 345) likewise encodes
the old heuristic, rather than support-aware pivot feasibility.

`quadruped_tests.cpp` requires at most one lifted foot in walking **and turns**.
That remains valid if planned turns keep three contacts; change it only if the
new gait intentionally uses paired support, then assert stable supported pairs
and no unsupported phase rather than dropping support checks.

The previous elastic-strain expectation was already replaced with a <3.5%
link-extension limit. Preserve that limit or improve it; do not restore leg
stretch as a side effect of restoring speed. Existing very long arrival loops
and turn-duration-only bounds do not establish accepted travel pace or a small
placement count. Fixed-tick reversal fixtures also depend on cadence; use
explicit observed in-flight state to trigger reversals while retaining coverage.

Add meaningful regressions for steady straight travel at .5×/1×/2×/3× against
the accepted baseline, default2× distance/time on a long route, and finite
stationary-turn placement counts independent of speed setting. Distinguish
turn placements from ordinary route steps and final posture recovery.

## Contracts to retain as regression safeguards

* Authoritative world contact position/yaw remains fixed throughout stance;
  rendered IK and sole offsets reconstruct that same contact.
* Reach/link bounds, finite joints, authored opposing quadruped bend planes,
  no foot crossing or floor penetration, and appropriate support through turns.
* No heading-basis discontinuity when reversing during an existing lifted step,
  including rapid repeated target changes and moving supports.
* Proposal does not mutate presented state; repeated proposals are identical;
  quantized/clipped host realization preserves pins and cancels blocked targets.
* Braking respects acceleration, large reversals avoid substantial wrong-way
  travel, target arrival settles completely, and settled pose reads stay quiet.
* Pickup/drop, bounds, wings, arm ownership and speed changes preserve their
  independent behavior and cannot revive stale pending proposals.

No tests or builds were run for this audit; numeric parameter/placement checks
were calculated with the repository virtual environment. Native visual evidence
is still necessary to assess the new turn choreography and restored perceived
pace. CPU/offscreen checks would not establish battery impact or desktop parity.
