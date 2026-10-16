# Procedural pickup, drop and recovery

Date: 2026-10-10

Status: approved by the user; implemented and verified on 2026-10-11.

## Intent and success

The user wants to lift a desktop pet, release it from a height, watch it fall
and react to impact, and see it recover. The user chose a physical body with
procedural limbs over independently simulated ragdoll joints and approved the
Held → Falling → Landing → Recovery interaction described in chat.

Success means releasing an elevated pet preserves its position and recent drag
momentum instead of snapping to the screen bottom. Gravity produces continuous
travel; feet acquire real contacts on landing; existing compliant support absorbs
impact; procedural recovery returns the pet to ordinary behavior. Both production
2D characters and the 3D werewolf use the same lifecycle and body dynamics.

## Scope and constraints

- Keep C++17 and Qt Quick; add no physics-engine dependency.
- Body dynamics, lifecycle and recovery policy are Qt-free and shared by 2D/3D.
- Generate limb goals and solve poses procedurally; use no canned recovery clips.
- The floor is the bottom boundary of the selected display's available geometry.
- Desktop travel is planar: horizontal motion and vertical elevation; the 3D
  presentation retains its existing perspective and does not add depth travel.
- Preserve grounded hard floor pins and alternating single-foot gait. Both feet
  may be airborne while held or falling; this explicitly extends the previous
  ground-only contact rule.
- Use the existing single application timer. Stop physics deadlines at rest and
  during suppression; no background physics thread or permanent animation loop.
- Preserve session API semantics, character selection, Toolbox, cards and Spaces.
- Keep `shared/` and the timed benchmark fixture unchanged.
- Full ragdolls, other-window surfaces, pet-to-pet collisions, persistent injury,
  and upside-down somersaults are outside this first version.

## User-visible behavior

### Held

Left-button pickup interrupts walking, curiosity and active pose reactions.
The existing grab point follows the pointer. Previously planted feet detach
immediately. Limbs use bounded body-relative goals and followers, responding to
movement without trying to maintain floor contacts. Re-grabbing during falling,
landing or recovery enters Held without a positional jump.

Use realized drag displacement and monotonic time to estimate recent velocity.
Keep a bounded recent sample window, discard stale samples, and cap release
speed. A stationary hold releases with zero throw velocity; repeated equal-time
events cannot divide by zero. Screen clipping must not manufacture momentum.

### Airborne

On ordinary button release, preserve the realized body position and apply the
estimated planar velocity. Gravity advances the body; the host follows the
result in both axes. Fall dynamics do not depend on the Stride length control.
Feet and arms follow anatomical, body-relative goals: hanging while ascending
or held and preparing for touchdown while descending. Neither foot is treated
as supporting the body before actual touchdown.

Horizontal screen limits constrain travel and remove outward velocity. Falling
ends at first descending sole contact with the floor, using swept contact or an
equivalent crossing calculation so fast drops cannot tunnel through it.

### Landing

At touchdown, establish fresh world contact anchors exactly once. Carry incoming
vertical velocity into compliant support rather than resetting it to rest. Leg
compression, body pitch and head/ear/tail response scale with impact speed.
Bound compression and apply a body-floor constraint for extreme impacts so
neither geometry nor numerical state passes through the floor.

Small drops settle with a shallow bend. Larger drops produce a deeper crouch
and a bounded stagger. Limit tilt to what the rig and presentation support;
this version does not require a fully prone body or a ragdoll collapse.

### Recovering and Grounded

Recovery raises the supported body toward comfortable equilibrium and restores
neutral limb goals. Where a stagger requires a step, use the existing procedural
step machinery: one lifted foot at a time, with the other sole pinned. Grounded
contacts remain fixed until explicit toe-off; recovery must not slide them.

Return to Grounded only after body and limb dynamics meet finite settling
thresholds. Resume curiosity from a fresh deadline; do not replay interrupted
walking targets, arm reaches or reactions. Pending session records remain
available through the existing inbox policy.

## Ownership and coordinate contract

Add a small Qt-free body-dynamics component under `qt/characters/` with explicit
Held, Airborne, Landing, Recovering and Grounded states. It owns body position,
velocity, contact transitions and impact/recovery parameters. Inputs include
realized placement, held movement, release velocity, floor geometry and bounded
elapsed time. Outputs include desired planar movement, lifecycle state and
procedural pose/support goals. No windows, screen discovery or JSON live here.

Extend the existing controller adapters to consume this state. Keep the existing
shared axial support law in `motion_support.*`; intentional free fall uses full
fall gravity rather than walking's reduced contact-loss gravity and drag.
There must be one authoritative vertical momentum state, with an explicit
handoff to support dynamics at landing, not two independent gravity integrators.

Use screen logical coordinates at the host boundary, positive Y downward. Floor
Y is independent of the window's current local floor anchor. Body height above
the floor belongs to motion state rather than being inferred as a new floor on
every drag. Continuous desired position and integer host realization are distinct;
retain fractional placement and return clipping feedback in both axes.

The window follows root elevation; the local pose applies only relative limb,
compression and recovery offsets. Apply fall elevation once. In 3D, convert
shared travel units through the existing calibrated scale and vertical projection
with explicit sign; preserve the current camera and floor-contact appearance.

### Existing integration points

- `qt/resident/desktop_host.*`: remove unconditional floor rebasing on ordinary
  release; distinguish release from cancellation; expose actual floor geometry
  and two-axis realized placement. Keep authored grab regions and visual masks.
- `qt/resident/application.*`: coordinate pickup/release, shared motion deadlines,
  suppression and autonomy arbitration. Replace drag-triggered controller reset
  with lifecycle transitions. Avoid writing preferences on every falling frame.
- `qt/characters/grounded.*`: detach contacts, generate bounded airborne IK goals,
  initialize touchdown contacts and integrate impact/recovery with support.
  Airborne IK must not use elastic floor-preserving targets.
- `qt/characters3d/pose.*`: forward lifecycle to the shared controller; gate
  physical contact flags and turn replants on actual support. Clear pending
  motion proposals on pickup/release and clear stale arm goals.
- `qt/resident/spatial_character.*` and the 3D scene binding: convert vertical
  state and apply the shared pose while keeping window-following and projection
  consistent. Update masks and card anchors during landing/recovery.

## Suppression, cancellation and persistence

Pause freezes position, velocity and pose. Resume continues with a fresh clock
baseline. Dragging while paused permits direct placement but performs no physics;
release stores the resulting position, with no throw momentum, for resume.

Hide and system sleep suspend motion and rendering. Show/wake continue from the
preserved state with no elapsed-time catch-up. If a drag is interrupted by hide,
lost mouse grab or deactivation, end Held with zero throw velocity and enter
Airborne only when unsuppressed. Distinguish these events from a real release.

Display changes reconcile floor and reachability once without teleporting to
the floor. A newly raised floor may require immediate contact repair. Character
swaps preserve presented root position, clear incompatible dynamics, and choose
Grounded or zero-velocity Airborne from the new sole/floor relationship. Failure
to load a character keeps the existing character and its motion state.

Save placement on release, completed settling, suppression, reconciliation and
shutdown. Do not persist per-frame velocity or recovery state. Restart restores
the saved reachable position and begins at rest if supported, otherwise with a
zero-velocity fall. Frozen fixture characters retain their existing behavior.

## Numerical and scheduling requirements

Validate finite input and ordered geometry at boundaries. Bound elapsed time,
use small deterministic substeps, and discard suppressed elapsed time. Bound
release velocity, impact response, angular motion and recovery goals relative
to anatomy. Fast or irregular input must not produce unbounded limb extension,
NaNs, floor tunneling or indefinite recovery.

Active falling/landing/recovery joins the resident's existing motion deadline.
Held followers schedule only while unsettled; stationary holds can become quiet.
Settling removes physics deadlines, while existing session/curiosity deadlines
remain independent. No CPU or offscreen check establishes battery impact.

## Verification and acceptance

Deterministic core checks must establish:

- Elevated release preserves position; zero-height release remains supported.
- Fall time increases with release height and does not change with stride setting.
- Recent drag velocity produces bounded throws; stale, clipped and equal-time
  samples cannot produce accidental impulses.
- Held/Airborne feet are unsupported and limb length stays anatomically bounded.
- Fast drops detect first contact, remain above the body-floor limit, and produce
  bounded stronger landing responses than small drops.
- Landing installs contacts once; supported soles stay pinned until toe-off.
- Recovery terminates, resumes ordinary behavior, and removes physics deadlines.
- Re-grab interrupts every phase without a position jump or leftover proposal.
- Pause/hide/sleep preserve state without catch-up; cancellation releases safely.

Host/application checks cover actual two-axis placement and fractional feedback,
release versus cancellation, display reconciliation, character swapping,
placement persistence, cards/masks and unchanged session behavior. Run the 2D
and optional 3D build paths, existing grounded/skin/projection tests, frozen
golden poses and Python regression suite.

Native acceptance must exercise both 2D pets and the 3D werewolf: low/high drops,
sideways release, stationary hold, repeated re-grabs, stronger landing/recovery,
pause and hide midway, monitor crossing and sleep/wake. Inspect clipping,
transparent fringe, click-through regions, cards, contact appearance and settled
rendering. Keep new captures/logs in ignored `results/` run directories. Software
verification and outstanding native observations must be reported separately.

## Review and next step

Self-review: scope follows the approved interaction; shared ownership avoids
duplicated physics; explicit airborne IK prevents stretched floor pins; lifecycle
rules cover interruption, persistence and quiet scheduling. No product code has
changed. After written-spec review, create the implementation plan with explicit
file ownership and tests, then execute the reviewed plan in this chat using the
project's requested subagents where independent work is useful.
