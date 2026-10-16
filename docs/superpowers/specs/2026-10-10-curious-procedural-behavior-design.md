# Calm, curious procedural behavior

Date: 2026-10-10
Status: proposed design for review; implementation has not started.

## Intent and requirements

The user wants pets that initiate activities and have interesting idle motion.
They selected a calm, curious personality and explicitly require all moving
limbs to use the procedural animation system. Behavior selects goals and applies
impulses; the motion system generates the movement. Extend that system with
simple reusable capabilities as needed. Artwork and texture presentation remain
independent of behavior and motion generation.

Success is a pet that sometimes surveys its surroundings, twitches an ear or
swishes its tail, makes a gentle stretch where its anatomy permits, and takes
short exploratory walks. Activities have anticipation, a brief hold and a
smooth return to rest. Quiet intervals are intentional. Different activities
must not look like the same whole-body wobble with different labels.

Preserve existing body-led travel, acceleration/braking, authored step duration,
strict planted soles, alternating single-foot swings and support dynamics.
Remain C++17, Qt-free in behavior and motion logic, with one resident timer and
bounded work. Native appearance and battery impact require their own evidence.

## Current implementation and gaps

- `qt/resident/runtime.cpp` schedules the same two-second ambient episode after
  each 30-second rest. `application.cpp` renders it through `Action::Idle`.
- `Controller::pose` mixes direct time-derived pose offsets with an impulse
  response. The grounded adapter replaces torso scale/rotation/translation to
  preserve support, removing much of that expression.
- `GroundedController` already owns navigation, grounded IK and second-order
  head/ear/tail followers. These followers currently respond to locomotion and
  whole-body impulses, rather than independent expressive goals.
- The 3D adapter shares grounded planning but lacks expressive head/ear/tail
  outputs. `Motion::armGoal` is an instantaneous endpoint change. The preview
  generates gesture paths, wrist angles and head offsets in its Qt controller.
- The resident is 2D. The 3D preview is the existing integration surface for 3D;
  this work does not implement the separate resident-3D plan.

## Selected approach

Extend the existing procedural motion core with a small semantic goal layer,
then add a deterministic activity chooser above it. This makes autonomous
activities and explicit gestures clients of the same motion capabilities.

Simply randomizing the existing Action enum would leave direct pose animation
and dimensional inconsistencies in place. A general animation graph or scripted
pack runtime would introduce substantially more machinery than this slice needs.
Neither is the selected approach.

## Motion primitives and ownership

Use Qt-free typed values and a bounded fixed set of semantic channels. Behavior
does not name bone indices or publish transforms. Initial primitives are:

1. **Look:** a character-local direction, resolved into bounded head orientation
   through second-order followers. Planar rigs use the axes they can represent;
   spatial rigs support yaw/pitch. Looking alone does not turn the body or feet.
2. **Role goal:** bounded orientation bias for head tilt, individual ears and
   tail. Targets are relative to validated rest orientation and pass through
   the motion followers. This supports a twitch or swish without shaking every
   channel together.
3. **Role impulse:** bounded velocity change in a supported motion channel.
   Reuse the existing spring integration, with independently addressable roles.
   Existing whole-body drag impulses remain a compatibility convenience.
4. **Reach:** a side-specific character-local grip target and wrist orientation.
   Track the endpoint and orientation smoothly, then solve the chain with the
   existing spatial IK. Release tracks back to the neutral grip before removing
   the override. Reach is optional; the existing 3D arms exercise it.
5. **Navigate:** a ground target expressed in the host's existing world travel
   units. Reuse propose/realize and the current gait; no new foot animation path.

The concrete C++ names are an implementation detail. Directions are
dimensionless; rotation goals use radians. Semantic expressive vectors use a
right-handed character-local frame with +Y up and +Z forward, and reach positions
use fractions of a declared reference body height. Each adapter converts once
to its native model coordinates (2D canvas units/+Y down; 3D meters/+Y up).
Planar rigs omit depth. This slice introduces no
global desktop look target or ambiguous coordinate-space inference.

Goal state is latest-value per channel, with finite bounds and explicit clear.
Submitting a missing capability reports unsupported and changes no motion state.
Reject nonfinite values before mutation; clamp finite goals to rig limits. No
unbounded event queue, per-frame JSON, downloaded code or dynamic plugin system.

## Procedural composition

Advance followers during motion steps; pose queries are read-only. Desired goals
and realized follower state are distinct. All smoothing, impulse response,
joint limits, neutral return, IK and motion completion belong to the motion core.
Behavior may choose successive targets and hold durations, but may not calculate
per-frame joint rotations, limb trajectories or sinusoidal bone offsets.

Compose locomotion-driven secondary response with expressive goals before final
joint limits. Keep separate state where locomotion and expression require
independent targets, so realize() does not erase an active look or ear goal.
Left and right ears have independent expressive channels even when their
locomotion response shares tuning.
Settling checks compare followers against their current goals, including a
nonzero held goal. A held, settled pose needs no animation ticks.

Ground contacts remain authoritative. Expression never writes feet or replaces
the support-derived torso pose. First-slice stretches use optional reach plus
head/tail goals; they do not inject a free torso translation or force feet to
match an authored pose. General weight-shift/posture control may be added later
through the support model when there is a concrete activity needing it.

Renderers apply solved snapshots to sprite attachments or skin joints. They own
no gesture clocks or autonomous bone offsets. Move the current preview's head,
arm and wrist gesture generation into procedural goal sequences; keep its Qt
controller responsible for input conversion and presenting the result. Preserve
the outward reach path, wrist/grip geometry and arm clearance safeguards inside
the motion implementation. Existing production character reactions must also
use the new primitives rather than retaining a parallel expression bypass.
The frozen benchmark fixture remains unchanged.

## Activity chooser

A small Qt-free controller owns a seeded random generator, one current activity,
its bounded phase/deadline, and a small recent-activity history. The same seed and
input timeline reproduce the same decisions. No general behavior tree is needed.

Initial activities are quiet rest, survey, ear twitch, tail swish, optional reach
stretch, and short explore. Survey selects a few local directions with uneven
holds; a twitch uses a small role impulse; swish uses a bounded tail goal and
return; stretch uses a safe reach goal with a hold and release. Explore chooses
a reachable target along the current ground span, then surveys after arrival.
Capability filtering occurs before selection. Unsupported activities are omitted,
not replaced by generic whole-body wobbling.

Initial tuning: quiet gaps of 8–20 seconds, surveys of 2–5 seconds, isolated
secondary gestures of 0.5–2 seconds, and stretches of 2–4 seconds. At most one
in four choices is explore, with at least 45 seconds between exploration starts.
Explore distance is 0.5–1.5 character widths within the current safe span and
has an eight-second timeout. Tiny/unavailable travel space removes explore.
No immediately repeated activity when another supported choice exists. These
are first-pass defaults for visual review, not resource or personality claims.

Mallow emphasizes surveys and occasional ear movement; Bramble emphasizes head
tilt and tail response; the werewolf preview adds supported reach stretches.
Start with built-in profiles derived from the supplied characters and safe
fallbacks for others. Do not extend the pack schema until these primitives and
profiles have been exercised.

## Arbitration, interruption and placement

Visibility, pause, sleep and quitting suppress autonomous decisions and motion
scheduling. Direct manipulation/explicit targets and notification reactions have
priority over autonomous goals. Cancel conflicting autonomous goals; resume with
a fresh quiet interval rather than replaying missed activities.

During a visible preemption, clear expressive goals toward neutral through the
procedural system and decelerate navigation with the existing controller. Do not
reset planted feet to cancel a walk. Pause freezes the current pose; hidden/sleep
transitions use the existing host rebase rules. Character swaps clear old goals,
recompute capabilities and start a new quiet interval.

Blocked host realization ends exploration without retries against the same edge.
Manual placement becomes the next exploration origin. Reuse screen-safe host
placement; crossing monitors, window-edge discovery and desktop object awareness
remain separate work.

## Scheduling and integration

Join behavior deadlines and motion-settling deadlines into the existing resident
single-shot timer. Replace the fixed ambient episode with chooser deadlines,
while retaining notification expiry and existing suppression behavior. Never
poll for the next activity with an always-running animation timer.

An active goal transition, navigation or unsettled impulse uses the current
bounded motion rate. Once all motion channels settle, stop stepping until a goal
changes or a hold/activity deadline expires. Long clock jumps do not create a
backlog of activities, impulses or large integration steps. A sparse rest
deadline produces no redraw unless the resulting pose changes.

Both previews get autonomous mode plus manual inspection of the same primitives,
including a reproducible seed. Manual controls preempt autonomous work. The 3D
preview's existing continuous development rendering is not evidence of resident
quiet scheduling or battery suitability.

## Verification and acceptance

Core checks cover deterministic decisions, bounds, capability omission, per-role
isolation, tracked look/reach/release, finite poses, interruption continuity,
settling at nonzero goals, neutral return, and long-clock-jump behavior.

Motion regressions retain hard world-space sole pins, single-foot flight,
step timing and host realization semantics. Verify that idle expressions produce
visible head/ear/tail movement without moving contacts. Reach checks retain arm
lengths, wrist/grip alignment, clearance, and smooth endpoint transitions.

Resident checks cover priority over exploration, blocked targets, swap/reset,
pause/hide/sleep, bounded callbacks and removal of active animation deadlines
after settling. Run the applicable Qt and Python suites; distinguish skipped
native/socket checks from passes.

Provide seeded captures showing quiet, survey, twitch/swish, explore/arrival and
optional stretch in 2D and 3D previews. Review at actual display size, not only
enlarged. The user judges the calmness and appeal of the rebuilt native pets.
Record new output under a fresh ignored results directory. No CPU/RSS or
offscreen test establishes battery impact.

## Scope boundaries

Included: reusable expressive goals/impulses, tracked 3D reach, migration of
production expression paths, a small autonomous chooser, resident 2D integration,
both previews, and regression evidence.

Deferred: resident 3D, eye/blink texture variants, sitting/lying/jumping, new
ground-support posture primitives, object-aware behavior, screen observation,
external behavior IPC, authoring UI, executable packs and a general animation
graph. Extend those through the same procedural ownership when selected.
