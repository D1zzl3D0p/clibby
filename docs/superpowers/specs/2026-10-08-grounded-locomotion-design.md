# Grounded target-driven locomotion

Status: revised design for review, not implemented. This replaces the proposed
clock-driven gait for the first soft creature, not the frozen benchmark fixture.

## Intent and scope

The user selected horizontal movement on the ground, a much simpler composited
creature, and genuine target-driven procedural animation. Success is a pet that
accelerates, plants its feet, takes steps because its body needs support, reverses,
and settles at a target without a repeating walk animation. Its head, ears and
tail should convey inertia and personality. The werewolf becomes a separate
Blender/3D project after this locomotion foundation.

Use a flat cream/white silhouette, black eyes, two ears, a tail and small feet.
Remove outlines, shading, claws, internal fur detail and other small decorative
features from Mallow. Art consists of reusable attachments, not generated frames.
Two hip–knee–ankle chains remain present even when the white body hides the hips.

This phase includes preview and resident integration, a single flat floor per
display, target input, limited impulses and quiet scheduling. Jumping, falling,
window-top surfaces, terrain, obstacles, autonomous wandering and ragdolls are
later work. Existing session actions and package switching remain usable.

## Selected approach

Use stateful navigation, world-space foot anchors and analytical two-link IK.
Secondary motion uses tunable second-order tracking. This adds the needed motion
state without a general physics engine. A clocked gait with randomized timing
would still skate and would not satisfy the request. A full rigid-body simulation
would add collision and tuning work before the intended feel is established.

The supplied transcript describes responsive tracking, with frequency `f`,
damping `zeta` and response `r`; its robot uses IK, but the transcript does not
teach the IK solve. Separate navigation, foot planning, IK and expressive tracking
so each can be tuned without compromising the others.

Reference: [supplied transcript](</Users/dizzler/Downloads/NoteGPT_Transcript_Giving Personality to Procedural Animations using Math.txt>),
particularly 00:00:35–00:01:40, 00:04:05–00:07:11 and 00:09:05–00:14:23.
Do not require the external Downloads file to build or run the project.

## Core boundaries and coordinates

Add focused Qt-free units for second-order tracking, two-link IK and grounded
locomotion under `qt/characters/`. Keep `Controller::pose(Action,time,gaze)` for
existing packages. A package with a supported `locomotion` declaration uses the
new stateful controller for walking; calling the old Walk action is not its gait.

World X is horizontal in logical desktop units; ground Y is downward-positive.
Navigation stores root position and velocity outside `Pose.root`. The rendered
pose remains local to the character canvas. Foot stance anchors are ground/world
positions, not offsets that move with the window. The preview uses the same unit
convention in an independent world; no global pointer observation is required.

Use a two-stage host exchange per simulation tick:

1. Navigation proposes a bounded horizontal displacement within a `GroundSpan`.
2. The host realizes placement and reports actual root X and blocked status.
3. Locomotion updates foot/body state once, against that realized position, then
   emits the local pose and debug contact state.

Keep a fractional position accumulator in the host adapter; integer OS placement
must not discard every subpixel displacement. Report the presented logical root
position to the contact solver so planted feet do not follow an unrealized move.
Host clipping clears velocity into a boundary and resolves the target to the
reachable span. An unreachable target must not keep the scheduler ticking forever.

## Navigation and foot planning

`setTargetX` accepts a finite coordinate and clamps it to the current ground span.
Speed and acceleration are bounded. Compute braking from remaining distance;
navigation approaches the target without expressive overshoot driving the window.
Facing changes only after a directional threshold; it must not flip back and
forth as the root settles.

Keep each stance foot fixed on the ground. Start a step when reach or support
error crosses a threshold; select the eligible foot with the greatest error.
Landing points use predicted root position and velocity, with stride/reach limits.
Only one foot swings at a time. A bounded step duration advances a generated arc
between recorded endpoints; no loop phase determines which foot steps. Duration
may use speed/step distance but has a minimum and maximum so a final step finishes
even when the body stops. Finish in a comfortable stance rather than continuously
alternating feet at rest.

On reversal, keep stance anchors and adjust the next step. An active swing may
retarget within a bounded displacement, preserving position continuity. Do not
mirror the whole skeleton during a planted step: change gaze/head orientation
first, and turn the body/stance through reachable targets. The simplified blob
has a largely symmetric silhouette, so it needs no instant sprite flip.

Solve each hip-to-foot chain analytically, retaining its authored knee bend.
Clamp the solve distance inside its reachable annulus; prevent zero-distance
division, straight-leg singularities and inverse-trig domain errors. Segment
lengths stay constant. Body height yields when necessary to preserve reach.
All outputs remain finite and within the existing pose/motion-envelope bounds.

## Anatomy and package compatibility

Add optional version-1 `locomotion` metadata, explicitly supported by the loader
and controller. Existing packages without it retain their current behavior. It
declares two contiguous two-link chains, foot contact offsets, bend direction,
ground anchor, speed/acceleration, reach thresholds, stride and lift limits, step
duration bounds, and secondary-motion parameters. Document units and bounds.

Reject duplicate/shared chain bones, disconnected chains, zero lengths, invalid
bend directions and non-finite/out-of-range tuning. This solver accepts unit-scale
chain ancestry; reject unsupported scale/shear rather than silently changing limb
lengths. Visual body squash must not scale the IK ancestry. Keep the original
version-1 character loading limits and transactional selection behavior.

## Secondary motion and force response

Use the transcript's parameterization:

`k1 = zeta / (pi*f)`, `k2 = 1 / (2*pi*f)^2`,
`k3 = r*zeta / (2*pi*f)`.

Frequency controls response speed; damping controls settling; response controls
initial follow-through/anticipation. Body tilt follows acceleration and target
intent, height follows support geometry, and head/ears/tail follow their own
targets with different tuning. Apply underdamping primarily to bounded local
expressive offsets. Do not spring the desktop window past ground boundaries.

Integrate in bounded substeps, with a coefficient adjustment when needed for
stability. Cap inferred target velocity and work per update. Discard suspended
time rather than replaying a backlog. Reject non-finite inputs; snap sufficiently
small displacement/velocity to exact rest. Existing drag/impulse response feeds
this state, respecting body and reach limits. This is limited secondary physics,
not collision simulation.

## Preview and resident

Preview: a ground line, target marker, click/drag-to-set-X, reset and impulses,
actual-size/composited views, optional bone/anchor/landing overlays, and live
frequency/damping/response controls. Add scripted target/reversal CLI scenarios
and state export for repeatable validation; determinism is useful for tests but
not a product requirement. Separate target input from the existing gaze handler.

Resident: add a narrow host API for horizontal ground movement with realized
placement feedback. Ground uses the selected display's available rectangle bottom
and the character's authored foot anchor. A target cannot cross a display gap in
this phase. If the host window's local root is `R`, window width is `W`, and the
available display extends from `L` to exclusive `U`, the root span is
`[L + R.x, U - W + R.x]`; floor-root Y is available-bottom minus the
root-to-foot-anchor offset. Include the existing motion margin in window/anchor
coordinates. Keep the whole window inside this span; if it cannot fit, disable
ground movement safely rather than inventing an inverted span. Grounded mode
explicitly rebases placement to that floor; legacy pets
keep their existing placement behavior. Provide a same-window menu target control
(Walk left / Walk right / Stop) without changing the frozen external session API.
Do not poll the global cursor or add automatic wandering.

Join active locomotion and settling into the existing single next-deadline timer.
Gather coinciding runtime/session/motion deadlines, advance locomotion once for
one elapsed interval, realize placement once and publish one final pose. Pose
generation itself does not advance simulation. Session reactions supply bounded
expressive targets before grounded IK; they must not replace the pose with the
legacy controller and restore its rest-pose foot contacts. Preserve arbitration,
duration and acknowledgment behavior.
Stop locomotion/settling deadlines after arrival; retain legitimate sparse idle
and session-expiry deadlines. Pause freezes state and resumes
without catch-up. Hide/sleep discard suspended time and rebase on return. Dragging
cancels navigation; release re-establishes the floor stance. Character switches
and display changes reset anchors and resolve a reachable target transactionally.

## Pixels

CPU composition at selectable low resolution remains the first creature's
default in both preview and resident. Grounded resident creation/selection must
explicitly configure this mode; `CharacterItem` currently defaults to direct
rendering. Attachment sampling and enlargement use nearest-neighbor filtering.
No linear piece filter. Keep palette dither off by default and label it accurately;
it is not temporal stabilization. Compare unsnapped motion with pixel-grid-aligned
rendering using motion captures. Do not round physical foot anchors to output
pixels. Temporal coverage/hysteresis is an optional later experiment if visible
flicker remains; it must not be sold as solved by a static dither overlay.

## Acceptance

- Two-link endpoints reach feasible targets, lengths remain constant, and
  singular/unreachable inputs produce bounded finite poses.
- Stance-foot drift is below 0.1 logical unit in the solver, including host
  clamping and fractional movement; pixel rounding is assessed separately.
- Slow approach, acceleration, stopping, mid-step reversal and impulses remain
  continuous; at least one foot supports every normal grounded step.
- Arrival within 0.5 logical unit, completed final step and settled followers
  remove the active motion deadline. Blocked targets also become quiet.
- Lag spikes, pause/resume, hide/restore, drag/release, package switch and display
  changes do not explode, replay movement or reuse stale foot anchors.
- Review short motion recordings at actual size and enlarged nearest scale;
  still captures cannot establish natural movement or temporal pixel stability.
- Existing character/session tests, the Python suite, and all 44 frozen golden
  poses remain passing. Native desktop checks and battery evidence remain separate.
