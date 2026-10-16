# Spring-supported motion and 3D resident direction

The user subsequently reported floor-sliding body art and persistent forward 3D
paw placement. The [stronger-support follow-up](2026-10-08-stronger-leg-support.md)
supersedes this record's support constants and 3D landing bias.

The user requests prescribed lateral body travel with free vertical response,
spring-like leg straightening to support the body, and smoother lateral
acceleration/braking. The acceleration clarification applies to body travel.
These changes supersede the fixed body height in the body-led correction.

## Movement

The shared grounded controller integrates a vertical mass under calibrated
gravity and two axial compression springs. Leg compression produces upward
support, damping absorbs landings, and support unloads smoothly as a foot lifts.
The equilibrium is calibrated against each character's compressed two-foot rest
stance. Integration substeps are at most 1/240 second; vertical excursion is
bounded against limb reach. No repeating body-bob animation clock is added.
Horizontal movement remains independent of foot reach. A bounded rate of change
of acceleration eases lateral starts, braking and reversals. The contact planner
forecasts this same navigation policy.

Foot position uses quintic interpolation and a smooth lift hump with zero
endpoint velocity/acceleration. Recovery overlap remains permitted at high travel
speed. Limb lengths, contact authority and forward-facing knee branches remain
unchanged. The optional 3D preview reads the same supported pelvis height, eases
turn replants and yaw, and replaces its abrupt 8 mm reach-recovery toe lift with
a continuous correction. A newly detected 35.7 mm contact pop when ordinary
stepping interrupts a turn is removed by preserving and smoothly blending the
presented start contact.

This is a constrained procedural support model, not a full rigid-body simulation.
Vertical support in 3D comes from the planar planner proxy; the actual 3D rig
still applies measured reach and side-clearance correction during yaw.

## 3D desktop pet

[The resident integration plan](../3d-resident-plan.md) is the requested design
output. It uses the existing application, sessions, Toolbox and controls; extracts
a timerless GPU renderer; adds trusted selectable 3D metadata; and calibrates
model/world/desktop coordinates and projected anchors/masks. The character owns
its world pose and the transparent native window follows its projection, with
rounding and screen-edge feedback. A padded render envelope accommodates bounce
and gestures. The resident 3D character is not implemented in this pass.

## Verification and review

The first software captures exposed lateral overshoot and a visible 2D facing
flip near arrival even though the initial test suite passed. Those captures are
retained under `results/spring-support-motion-before-arrival-fix/`. Braking now
reserves distance for both the acceleration ramp and velocity-follower settling.
New actual-package regressions require monotonic arrival without facing reversal
for targets 20/90/500 at 0.5/1/2/3× and 30/120 Hz. Both packages pass, along with
the existing bounded-jerk, cruise speed, contact, length and settling checks.

Final Release build passed. Fresh full CTest: 19 passed, 3 environment skips
(two native socket checks and GPU framebuffer rendering). Python unittest suite:
50 passed. An additional live 3D deform verification at 3× passed all 20 joint
bindings and contact checks. The 3D motion suite exercises vertical response,
turn/recovery continuity at 6,000 tiny intervals, fixed limb lengths, planted
contacts, reversal and eventual settling.

The short-span preview CLI speed check now recognizes that 2× and 3× can both
start braking before reaching their cruise caps. It checks real displacement
improvement over 1× and no reduction at 3×; long-span grounded tests retain
distinct authored cruise speeds at every multiplier.

Corrected software review is saved under `results/spring-support-motion/`:
40 frames per character at 50 ms intervals, raw metadata, `spring-review.gif`
and `contact-sheet.png`. The reviewed captures retain full silhouettes, vertical
pose response and forward progression without the earlier facing flip. Full
arrival and settling are checked separately; the two-second review intentionally
shows approach and braking, not a completed native acceptance trial.

Native GPU pixels, transparent desktop interaction and battery impact remain
separate acceptance checks. Quit and relaunch rebuilt binaries for native review.
