# New-session handoff

## Catalogue architecture and quadruped support started

All 18 proposed expansion pets are mapped in the
[catalogue architecture](superpowers/specs/2026-10-16-catalogue-architecture-design.md).
The [quadruped design](superpowers/specs/2026-10-16-quadruped-foundation-design.md)
defines a four-beat planar reference cat, with 3D adaptation later. Both designs
received an independent builder check. The first implemented substage adds
two/four-contact support-force overloads and count-aware neutral calibration,
with aggregate stiffness/damping normalization and independent numerical tests.
It does not yet add quadruped manifests, walking, poses or a cat package.

Combined Release: 40 passed/four skips; rebuilt 2D-only: 29 passed/three skips;
Python: 53 passed. A pre-change `resident_raven_idle3d` failure did not recur after
rebuild; no raven fix is claimed. Independent code review found no issues.
Read the [status and evidence record](results/2026-10-16-catalogue-and-quadruped-foundation.md).
Next: bounded active limb storage, four-role loader validation, gait/turn/drop
integration and all-foot resident aggregation before the first visible cat.

## Latest raven behavior: quick perching and idle window visits

Window approaches now steer promptly and start descending when the swept feet
fit one visible segment, without waiting for an exact hover. Nearby transfers
touch down in 1.5–2.3 seconds; full supported recovery takes 3.8–4.3 seconds,
down from 6.4–7.7. Banking preserves height through procedural wing effort.
Idle ravens seek a window after about eight quiet seconds, retry busy/no-window
states without hopping, and stay on an existing perch. Interaction and reactions
remain higher priority. Collision sweeps use quantized host placement, including
fractional remainders; fractional selected floors remain supported after moves.
Both native higher-window checks pass in under five seconds including recovery.
Combined Qt: 39 passed/four skips; 2D-only: 28 passed/three skips. Production app
rebuilt. Read the [quick-perching record](results/2026-10-16-raven-fast-perching.md).

## Latest raven correction: anatomy, forward flight and native bounds

The raven is 25% smaller in 2D, with visible bent hocks and aft wing strokes.
The 3D bird has clearer eyes, fuller tail feathers and weighted wing covers.
Fly up cruises forward and turns at screen edges; mid-flight retargeting and
quicker grounded turns retain fixed planted contacts. Screen constraints follow
occupied geometry. Instance-only Cocoa frame policy permits transparent padding
above the screen; measured triangle-influence bounds remove false feather
ceilings. Both native dimensions pass high dragging, forward flight, desktop
landing and perching on the higher y=300 fixture. Combined Qt: 37 passed/four
skips; 2D-only: 27 passed/three skips; Python: 52 plus one helper regression.
The app is rebuilt. Refresh results/raven-flight-2026-10-10/comparison.html or
results/raven-corrections-2026-10-16/comparison.html for rest/walk/two flight poses.
Read the [correction record](results/2026-10-16-raven-motion-and-geometry.md).
Everyday windows, multiple monitors, Spaces and energy remain acceptance work.
This supersedes the old padding-based ceiling policy in the October 12 record.

## Latest correction: PNG EXIF warnings

The werewolf GLB embeds a PNG with two malformed `eXIf` chunks. Balsam
preserved them into the runtime texture, where Qt reproduced
`libpng warning: eXIf: invalid` twice on decode. Both 3D converters now strip
unused EXIF from extracted PNG maps, retaining all other chunks byte-for-byte.
CMake tracks the shared helper so existing builds regenerate and copy clean
textures. Original GLB/Blender assets remain intact; conversion removes the
metadata each time. Regression coverage runs real Balsam conversion for both
characters and verifies unchanged image/color data. The rebuilt runtime textures
decode through Qt without the EXIF warning. Restart the resident to reload them.

## Latest behavior: cursor turns in 2D as well as 3D

Active nearby cursor attention now turns grounded 2D pets in place when the
cursor moves behind them, using the same lateral threshold and cooldown as 3D.
The existing facing blend completes smoothly; navigation and planted feet stay
fixed, and held attention returns to sparse polling. Both planar characters and
both headings have regression coverage. Read the
[planar cursor record](results/2026-10-12-planar-cursor-turning.md).
Fresh complete Release: 37 Qt passed, four environment skips, zero failures;
50 Python passed. This supersedes earlier integrated failure counts below.
Native appearance needs review after restarting the rebuilt resident.

## Latest behavior: occasional eye blinks

The resident now blinks across the current 2D/3D pets, with seeded 7–16 second
quiet intervals and a 190 ms close/hold/reopen. A shared Qt-free driver uses the
existing timer and never starts limb-motion ticks. Suppression freezes progress;
character selection resets to open eyes. Eye artwork is separate in 2D; 3D has
two dedicated eye joints (Werewolf 22 total, Raven 20). Open appearance and
unrelated anatomy are preserved. Read the [blink record](results/2026-10-12-eye-blinks.md).
Fresh complete Release: 37 Qt checks passed, four environment skips, zero failures;
50 Python passed. This supersedes earlier integrated failure counts below.
Native blink feel and energy remain unmeasured; restart the rebuilt resident.

## Latest correction: comfortable arrival and stable turning

The shared planner accepts existing resting soles within 35% of authored stride;
the spatial adapter keeps that anatomy-scaled radius. Travel heading is held
through arrival instead of reversing on fractional desktop position error.
Slow host-adjusted arrivals can settle within 0.75 units; continuous positioning
retains precise arrival. Pins and single-foot flight remain protected. Read the
[arrival/turning record](results/2026-10-12-comfortable-arrival-and-turning.md).
Release build and focused motion checks pass; Python: 50 passed. Integrated Qt:
35 passed, four environment skips, one `resident_raven3d` lower-support failure,
also reproduced with these arrival changes removed. Earlier powered-altitude
failure was superseded by concurrent raven/drop changes; see the record.
Native appearance needs review after restart.

## Latest implementation: procedural pickup, drop and recovery

User-approved pickup/drop is implemented for production procedural 2D pets and
the 3D werewolf. Lift and release preserves height and recent drag momentum;
shared gravity drives the window, procedural limbs acquire floor contacts,
compliant landing absorbs impact, and stronger impacts add one recovery step.
Regrab interrupts every phase. Pause/hide/sleep suspend without catch-up and
settled pets return to quiet scheduling. Native hide/show preserves placement.
Follow-up tuning uses quadratic air resistance, triples ordinary drop gravity, extends
legs before touchdown and compresses them under impact. Both werewolf versions
spread their arms during falls; body and pitch remain continuous at contact.
Read the [implementation and verification record](results/2026-10-11-pickup-drop-recovery.md).
Initial implementation passed 36 integrated Qt checks with four skips. After
quadratic-drag tuning, a clean 2D-only build passes 26 with three skips; drop and
werewolf 3D checks pass, while the combined Raven 3D powered-altitude check fails
with drag enabled or disabled. See the record for isolation evidence. Python:
50 passed. Earlier native focused drop and Spaces checks passed. Manual throw feel, multi-monitor behavior and energy remain
open. Concurrent raven and cursor work is preserved; its status is separate.


## Current work: raven flight and shared perching

The matched 2D/3D ravens now use horizontal bodies, short legs and broader,
feathered wings. Procedural lift, landing and perch departure are corrected;
spatial host rounding no longer acts as a false ceiling. Cocoa reachability
respects native window placement limits. Both native Fly up/Land/window-perch
checks pass against a separately painted test window. Expanded Release suites:
37 passed/four skips (combined), 27 passed/three skips (2D only), zero failures.
The local comparison is results/raven-flight-2026-10-10/comparison.html.
Everyday windows, Spaces and energy remain acceptance work. Read the
[implementation record](results/2026-10-10-raven-flight-and-perching.md),
[approved design](superpowers/specs/2026-10-10-raven-flight-and-perching-design.md)
and [execution plan](superpowers/plans/2026-10-10-raven-flight-and-perching.md).

## Latest behavior: nearby cursor attention

The user chose the mouse cursor as the first nearby interest target. The resident
now sends head-relative cursor observations into calm curiosity, producing
procedural gaze, a slight head tilt and ear interest within a bounded range.
It tracks without walking toward the cursor, turns the 3D body in place when the
cursor is behind its current heading, releases smoothly and respects
manual control, notifications and suppression. Sparse cursor observations use
the existing timer; a stationary gaze settles without animation ticks. Both
2D reflection and 3D camera/model orientation are handled, including held gaze
through a turn. Read the [cursor record](results/2026-10-10-nearby-cursor-attention.md).
Fresh full build and cursor regressions pass; final integrated Qt run: 26 passed,
four environment skips, zero failures. Python: 50 passed. Earlier concurrent
pickup/drop failures are resolved. Subsequent in-place cursor-facing checks pass;
a later combined rebuild awaits the concurrent raven/window-surface APIs (see
cursor record). Stationary 2D cursor captures are inspected;
native gaze appearance and energy remain open.

## Latest implementation: calm curious procedural behavior

The user approved autonomous calm curiosity, requiring every limb movement to
use the procedural system. The Qt-free activity chooser now drives quiet surveys,
independent ear twitches, tail swishes, occasional bounded exploration and 3D
reach stretches. Shared role followers accept look goals, rotations and impulses;
2D and 3D render solved poses. Production notifications also submit goals. Held
poses settle without frame ticks; pause, hide, drag, manual travel and reactions
preempt autonomy through the existing single timer. The frozen fixture keeps its
legacy behavior. Preview tools expose autonomy, seeds and motion primitives.
Read the [implementation record](results/2026-10-10-curious-procedural-behavior.md)
and [approved design](superpowers/specs/2026-10-10-curious-procedural-behavior-design.md).

Fresh full Release verification: 25 Qt checks passed, four environment skips,
and 50 Python checks passed. This supersedes the curiosity-frequency failure
reported in the 3D record below. Software captures cover seeded 2D behavior;
native appearance, Spaces acceptance and battery measurements remain open.
Restart the rebuilt resident to try autonomous behavior.

## Latest implementation: live 3D desktop resident

Startup now repairs saved grounded placements whose floor falls below the
desktop edge, preserving horizontal position and valid saved placements. This
fixes older shorter-character top-left coordinates restored into the taller 3D
pet. Host/spatial/application and native explicit/remembered startup checks pass;
the concurrently added elevated-drop release-height character test remains
failing. Read the startup floor repair section in the resident 3D record.

The user accepted the current werewolf movement and authorized desktop
integration. The 3D werewolf is now selectable in `better_pets`, sharing the
existing controls, Toolbox, sessions, preferences and scheduler. Model-first
motion now accepts host realization; the transparent window follows projected
world travel. Timerless rendering, projected masks/cards, 2D↔3D rollback,
exact saved placement and suppression are implemented. Read the
[resident 3D record](results/2026-10-10-resident-3d.md).

The resident now uses the user's requested three-quarter perspective: −35°
right / −145° left headings with a 12° top-down camera, preserving horizontal
travel and floor contacts. Mirrored headings keep the torso toward the camera
through a shorter 110° reversal; a bounded angular impulse and stronger yaw
spring quicken the body response while planted paws replant individually.
Fresh Release build succeeded; the latest expanded Qt run passed 27 and skipped
2 environment checks. The earlier curiosity assertion now passes. Resident,
3D skin/contact/projection and socket checks passed. Earlier scoped verification
passed 22 Qt checks (2 socket environment skips); 50 Python checks passed. The separate 2D-only build remains usable. Native Cocoa
resident rendering/lifecycle passed, with transparent pixels and one final frame
swap over a 500 ms settled observation. The 3D resident was relaunched visibly with the revised perspective for
user review, including the camera-facing turn. Native left-facing capture and
500 ms settled rendering check passed. Native Spaces fixture/modular hide/show placement tests failed;
manual fringe, multi-monitor and sleep/wake acceptance and energy measurements
remain open. Historical statements that resident 3D has not shipped below are
superseded by this implementation record.

## Latest desktop change: all macOS Spaces

The resident pet and attached status card now request visibility across macOS
desktop Spaces on their existing physical monitor. Native Cocoa checks pass for
both host variants, hide/show and native surface recreation; the helper preserves
geometry without polling or timers. Read the
[Spaces record](results/2026-10-10-all-spaces.md) for policy and acceptance limits.
Fresh Release: 22 Qt passed, 4 environment skips; 50 Python passed; 3 direct native
Spaces checks passed. Actual Space/fullscreen/Stage Manager switching still needs
visual acceptance after restart. A preexisting 20-pixel Cocoa Hide/Show position
shift was reproduced independently with a no-op policy and remains open.

## Latest correction: shorter foot transfer and 3D reversal wiggle

The shared extra planted pause is now 10 ms after comparing 40/20/10/0 ms.
At 2×/120 Hz this reduces excess stretch about 26–27% in 2D and 23% in 3D,
while retaining bob, strict pins and single-foot flight. Shared contact IK
smooths the straight/elastic knee transition. 3D knee poles follow paw heading,
and each paw's lifted yaw change is bounded to 0.4 radians rather than up to 0.8.
Normal swing duration stays unchanged; large reversals may need more replants.
Read the [timing/reversal record](results/2026-10-10-shorter-plant-and-reversal.md).
Fresh full build: 20 Qt passed, 3 environment skips; 50 Python passed; live
20-joint deformation/contact verification passed at 3×. Restart for native review.

## Latest correction: hard floor pins and stretch-first legs

The user requires landed soles to remain fixed until toe-off and forbids both
feet being airborne together. Both dimensions now use exact contact targets and
elastic leg IK, with actual rendered joint extension. The 2D limb artwork also
extends without enlarging paws; 3D ankle rotation preserves the sole pivot.
Shared gait alternates complete swings, and 3D turn replants block ordinary
swing admission while lifted. Body-led navigation and ordinary step duration
continue unchanged. Read the [pinned-contact record](results/2026-10-09-pinned-floor-contacts.md).
This supersedes reach projection, lifted recovery and emergency overlap below.
Fresh full build: 20 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin verification passed at speed 3. Native appearance needs review
after restarting the rebuilt binaries.

## Latest correction: fixed floor contacts, weight-transfer bob and comfortable rest

The user rejected floor sliding and requested more walking bob with gentler
one-foot/airborne failures, plus less 3D arrival tapping and investigation of
occasional leg-power loss. Shared reach recovery now lifts rather than slides;
emergency swings overlap only after half-phase/minimum launch-gap guards.
Forward touchdown and lighter damping produce repeated spring-driven bob, with
ordinary swing duration unchanged. Common support keeps damping at full leg
extension, weakens gravity on contact loss and adds downward airborne drag.
The 3D adapter smoothly maps lifted recovery, uses a 4 cm home/heading tolerance
and settles at actual equilibrium rather than forcing the original body height.
Read the [weight-transfer record](results/2026-10-08-weight-transfer-and-rest-tolerance.md).
This supersedes the previous floor-recovery/single-flight policy below.
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin check passed at speed 3. Final 2D bob is checked after startup with
repeated vertical reversals and fixed floor anchors. The exact native occasional
3D power-loss event was not reproduced; extended-contact damping and ignored
recovery mapping are corrected, with finite impulse recovery verified.

## Latest correction: shared spatial motion and alternating steps

The user requested one motion system to avoid fixing the same physics twice.
Spatial IK, axial support and neutral stance calibration now share one Qt-free
core: 2D supplies zero depth, 3D supplies actual spatial geometry. The dimensional
support fork is removed; neither profile uses upright assistance. Gravity is
independent of stance reserve and 3D gravity halved. Shared gait admission
alternates one swing at a time with 40 ms floor transfer; unreachable stance
feet explicitly recover along the floor rather than hopping together. Ordinary
timing and body-led speed remain unchanged. Ongoing-launch tests also catch
held-foot deadlocks. Read the [shared motion record](results/2026-10-08-shared-spatial-motion.md).
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin verification passed at speed 3. Measured 3D hip drop is now
2.4–7.4 cm rather than over 22 cm. Native appearance remains open.

## Latest correction: planted knees, real 3D support and ankle pitch

The user clarified that 2D knees were flexing excessively throughout planted
stance, not during foot lift. They chose gentler gravity with stylized upright
2D support to preserve the existing step rhythm. Planted median extension is
now approximately 95–97%, idle 98%, with no quicker emergency swings. The 3D
werewolf separately selects strict actual-sole support, softer springs and
touchdown loading; both airborne paws fall without a height clamp. Its support
proxy matches measured limbs/paw offset, and paws pitch independently of shins.
Elevated held targets retain elbow clearance during body bounce. Read the
[contact/ankle record](results/2026-10-09-contact-support-and-ankles.md).
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live skin
verification also passed at speed 3. Native appearance remains open. The support
model uses axial springs with analytical IK, not independent joint torque physics.

## Latest follow-up: stronger support and centered 3D paws

The user's native review found both 2D bodies sliding along the floor and the 3D
paws remaining ahead during travel. Spring stiffness is now stronger while body
weight is independent of that stiffness; calibrated rest lengths and a higher
stance reduce sag. Step timing and lateral navigation remain unchanged. The 3D
planner centers support around the hip projection and reduces forward landing
lead rather than carrying the authored paw offset into every step. Read the
[stronger-support record](results/2026-10-08-stronger-leg-support.md) for evidence.

## Current work: vertical leg support and 3D resident design

The user returned to animation: keep body-led horizontal travel, allow the body
to bounce with spring-like leg support, smooth lateral acceleration/braking
(explicitly clarified as lateral, not foot speed), and include the 3D werewolf.
The shared controller now has damped vertical support from leg compression,
smooth contact loading and eased lateral navigation. The 3D adapter has continuous
reach recovery and interrupted-turn contact continuity. Read the
[spring-support record](results/2026-10-08-spring-supported-motion.md) for final
verification and limitations. Movement review takes priority over functionality.

The user also requested figuring out resident 3D integration and clarified that
the window should follow the model. The [3D resident plan](3d-resident-plan.md)
implements that ownership in its proposed coordinate contract: world-space model
motion, projected window following, and host realization feedback. It reuses the
existing resident, controls and cards. Resident 3D implementation has not shipped.

## Functionality work resumed

The user reports the body-led correction is looking good and requests returning
to functionality iteration. Movement quality is accepted for continuing feature
work; the feature pause below is historical. Open source and Acknowledge were
also verified by the user. The current utility foundation includes command-run
outcomes, shared notification/inbox cards and the summoned Toolbox. No coding/chat
adapter, pinned app/project shortcuts or timer/reminder provider exists yet.
The next functionality priority is being selected with the user; real work
notifications are the recommendation. This acceptance does not establish battery
measurements or unexecuted native GPU/socket test results.

## Latest correction: body-led movement

The user prefers the body to move smoothly with legs catching up, and reports
Mallow remains too slow and the werewolf jerks after stride-only tuning. Travel
speed now drives body navigation directly, without foot/cadence caps. Independent
feet recover inside their anatomical reach; recovery steps may overlap at high
speeds. A constant stance baseline replaces reach-driven torso drops. The 3D
adapter also keeps translation moving during turns, with foot-side/actual-reach
recovery instead of stopping or lowering the pelvis. Read the
[body-led record](results/2026-10-08-body-led-motion.md). Native feel remains open.
This supersedes the speed/stride-only corrections below; features remain paused.

## Previous correction: longer steps at unchanged rhythm

The user clarified faster travel should come from larger steps, not quicker
stepping. The control is now Stride length in Toolbox and both previews, retaining
saved values and default2. Swing/support, turns and follower timing stay fixed;
stride distance drives sustainable travel speed within leg reach. Read the
[stride correction](results/2026-10-08-stride-distance-correction.md).
This supersedes the timing scaling described in the prior speed record.
Fresh Release: 19 Qt checks passed, 3 environment skips; 50 Python passed.
User judgment of the new feel remains open; features are still paused.

## Latest correction: adjustable walking speed

The user confirms the previous fixes work but still finds motion robotic.
Walking speed is now adjustable from 0.5× to 3× in the resident Toolbox and both
previews, with 2× as the new default. The resident saves the setting across
restarts and character swaps. Speed coordinates navigation, foot timing,
turn replants and followers while preserving contacts and targets. Reaction
and capture time remain unchanged. Read the
[walking-speed record](results/2026-10-08-adjustable-walking-speed.md).
Fresh Release: 19 Qt checks passed, 3 environment skips; 50 Python passed.
Restart rebuilt binaries to access controls. The user can now judge 2×/3×
against 1×; natural-looking motion and battery acceptance are not inferred from
software checks. Feature work remains paused for character feel.

## Latest follow-up: wave wrist and werewolf knee

The user's follow-up found two gaps: the 3D wave retained the paw's rest
orientation, and the 2D werewolf left leg still had the opposite knee pole.
Both authored 2D knees now flex forward and reverse together. The raised 3D
wave pitches the paw 180 degrees with a rotated grip offset, forward clearance
and controlled elbow bend. Read the
[wave/knee follow-up](results/2026-10-08-wave-and-knee-corrections.md).
The prior 3D gait changes were already included; wave pose changes are this pass.
Fresh Release verification: 19 Qt passed, 3 environment skips; 50 Python passed.
Native visual acceptance remains open. Feature development remains paused.

## Previous correction: clipping and gait

Feature development is paused for character fixes. Both 2D pets now use grounded
walking, with fuller strides, immediate torso balance and direction-aware knees.
Mallow has shallow forward knee artwork; Bramble supports its unequal authored
limbs and turns its visual branch. The native mask includes posed opaque artwork
instead of cropping it to the grab polygon. See the
[gait and clipping record](results/2026-10-08-gait-and-clipping-corrections.md).
Release build and checks: 18 Qt passed, 3 environment skips; 50 Python passed.
Software motion review: `results/gait-correction-after/walking-review.gif`.
Native appearance and click-through are still open; restart rebuilt binaries.

## User trial and previous correction

The user verified that Open source worked and Acknowledge dismissed the toast.
Focus loss hid the pet on macOS; the resident window policy is corrected in
[the visibility record](results/2026-10-08-visibility-correction.md).
The user also reported werewolf clipping during wave/hold/reverse walking.
Those native observations supersede any implication that earlier geometric tests
established visually correct motion. The correction adds alternating lifted foot
replants, support foot yaw, outward arm bends and a safe wave path; read the
[werewolf correction](results/2026-10-08-werewolf-corrections.md). Full rebuild:
18 Qt checks passed, 3 skipped; Python suite: 48 passed. Native visual recheck
of the corrected binaries is still outstanding.

## Latest utility implementation

The first utility slice is now implemented: real command outcomes through
`tools/pet_run.py`, interactive above-head Open source/Acknowledge cards, a summoned
Toolbox… with shared status cards and existing pet controls, reusable head/card/effect
anchors and semantic fallbacks, and werewolf arm IK/grip preview fixtures. Read the
[utility implementation record](results/2026-10-08-utility-slice.md) and
[body-fixture API](utility-body-fixtures.md). Fresh Release verification: 18 Qt checks
passed, 3 skipped; 48 Python tests passed. Live sockets, native desktop behavior,
GPU pixels and battery acceptance remain open. No coding/chat adapter exists.

## Start here

Current user goal for the fresh session: make the pets useful through an
attention inbox, a summoned toolbox and shared status cards, and add the body
fixtures needed to present them. Read
[the utility research report](../reports/Pet%20utility%20and%20toolbox%20design.md)
and [the utility handoff](utility-handoff.md) before choosing implementation scope.
The research is complete. The latest utility implementation above supersedes
the original proposal-only status; broader launcher/timer providers remain proposals. The user requested a fresh session with this goal.

Latest direction is implemented in software: ground-only target-driven IK,
simplified Mallow art, second-order personality followers, resident ground-motion
realization and a separate original low-poly Blender werewolf with optional
Qt Quick 3D preview. Read the [grounded record](results/2026-10-08-grounded-characters.md)
and [roadmap](superpowers/plans/2026-10-08-grounded-characters-roadmap.md).
Mallow defaults to CPU composition/nearest sampling with palette dither off.
Its latest polish adds anticipatory stepping, a slight quarter-view silhouette
and a 0.18-second visual turn that preserves planted feet. The werewolf preview
shares anticipatory planning and has corrected camera-projected target controls
and optional contact markers. See the [polish record](results/2026-10-08-motion-polish.md).
The local SDK now includes Quick3D, ShaderTools and QuickTimeline. The live skin's
20 joint bindings and world contacts are software-verified, but native GPU pixels,
transparency, quiet rendering and resource comparison remain open. The resident
remains 2D. Dweller supplied broad visual reference; the new werewolf mesh and
texture are original, with no copied Dweller assets.

Latest work: the approved [procedural pixel character design](superpowers/specs/2026-10-08-procedural-pixel-characters-design.md)
now has a software implementation. Read the [implementation record](results/2026-10-08-pixel-characters.md)
and [character format](characters-v1.md). The resident defaults to Mallow and can
switch to Bramble; `build/qt/character_preview` provides interactive action/style/
force inspection. This user-directed character work proceeded before completion
of stage-3 native/resource acceptance. It does not close those outstanding checks.

The user wants a lightweight desktop pet that wanders and eventually interacts
with objects, uses smooth procedural animation and authored artwork, and has
highly modular components plus stable interfaces for arbitrary applications.
Battery life is a primary constraint. Qt Quick is the chosen implementation
direction after the prototype comparison.

The latest request authorized implementation of the approved resident-companion
specification. The
[resident companion specification](superpowers/specs/2026-10-08-resident-companion-design.md)
now defines scope, behavior, interfaces, resource requirements and acceptance.
The resident foundation and stage-2 session notifications are implemented with
software tests. Read the [session-input record](results/2026-10-08-session-inputs.md)
and [API contract](api/session-v1.md). Live socket acceptance remains outstanding
because this execution sandbox denies socket binding. The user reports generally
good desktop behavior with an open cross-monitor snapping defect in the
[resident foundation record](results/2026-10-08-resident-foundation.md).
Read [architecture.md](architecture.md)
for the broader direction and [the findings](results/2026-10-08-comparison.md)
for measured evidence.

## What works now

- A C++17 Qt 6.10.3 Release prototype in `qt/`, with an authored 12-part character,
  breathing/gaze/tail/gait motion, transparent window, drag handler, right-click
  quit, timed scenarios, telemetry, pose export, and untimed screenshot support.
- A fixed-step Python reference and 44 golden poses in `shared/`. Do not turn
  this frozen fixture into the production package specification.
- A resource profiler in `bench/` and local setup/build/config tools in `tools/`.
  The default profile is one Qt build bracketed by no-pet baselines. Multi-build
  comparisons remain supported for future regressions.
- A Qt-free deterministic runtime, a desktop host with screen-safe placement,
  atomic preferences, and `better_pets`: a resident shell with menu controls,
  one next-deadline timer, a per-state-directory instance lock and a macOS
  sleep/wake observer. Default assets are copied next to the executable.
- Bounded Qt-free session state, arbitration, TTL/sequence handling, acknowledgment,
  suppression and short reactions, with a plain-text pet label and session menu.
- Same-user local JSON-RPC transport, `petctl`, and a simulated two-source example.
  Protocol policy and shell composition are tested separately from live sockets.
- Python regression tests and Qt CTest targets; results distinguish software
  verification, skipped transport checks and outstanding native acceptance.

`pet_qt` remains the timed benchmark harness with its original CLI and telemetry.
`better_pets` runs without benchmark arguments, launches visibly, retains pause
and placement, and provides Show/Hide, Pause/Resume, session status/acknowledgment
and Quit. It uses the current fixture and renderer without frame-history telemetry.
There is a separate version-1 data-only character loader, Qt-free `pet_character_core`,
bone/attachment/controller interfaces, original soft and articulated example
packages, retained nearest-filtered 2D renderer and force-aware resident path.
The frozen fixture path remains available through `--assets`. There is still no
real coding-tool adapter, object-aware behavior or QML product UI. Utility cards
use ordinary Qt Widgets with native buttons. `pet_motion`
still depends on Qt Core; `pet_runtime` and `pet_character_core` are Qt-free.

## Decisions to carry forward

1. Keep Qt as the presentation/desktop implementation; keep future core values,
   character formats and external messages independent of Qt.
2. Use C++ for runtime logic/rendering and QML for future UI/editor. Keep C++17
   unless a concrete need justifies changing it. Do not add Rust or a browser
   runtime just to achieve modularity.
3. Prefer typed internal interfaces in one application. Keep JSON/IPC at external
   boundaries. Modular does not require a process or dynamic plugin for each part.
4. Maintain the version-1 JSON-RPC session API and CLI contract. Version 1 is
   additive; incompatible semantics require a new major. Subscriptions and
   semantic world objects remain deferred until a feature needs them.
5. Use data-only character packages separating art, anatomy, motion parameters,
   interaction geometry and personality. Different body types need compatible
   controllers and semantic bindings, not one mandatory skeleton.
6. Expose desktop capabilities explicitly. Qt cannot remove Wayland restrictions
   on global placement and observation. Windows/Linux are not validated.
7. Stop unnecessary scheduling when frozen/hidden; observe desktop changes
   independently from rendering. Battery impact still needs measurement.

## What the benchmark actually established

The user completed 24/24 native candidate runs on an M4 Pro Mac. Qt used roughly
81 MiB RSS while visible versus Godot's 273–279 MiB, with about 18% lower animated
idle CPU and 23% lower walking CPU. CPU 100% means one logical core. These are
screening observations about the tested builds, not battery-life percentages.

The user confirmed dragging Qt during timing and being unable to drag Godot.
They described Qt as sharper, Godot slightly fuzzy, Godot hidden as an empty
transparent background, and Qt as moving its window. Dragging explains logged
movement in stationary Qt trials. `walk-desktop` deliberately moves its window;
an idle pet should remain in place unless dragged. Do not reinterpret the user's
visual observations as a controlled equivalence test or a proven engine defect.

Both simulations ran near 60 Hz, but walking render-event counts differed and
measure different stages. Hidden starts hidden rather than hiding after use.
Whole-system power, wakeups, physical footprint and WindowServer measurements
were unavailable. No battery target is passed, and archived runs remain marked
`comparison_eligible=false`.

## Cleanup and preserved evidence

- Removed active `godot/`, `tools/build_godot.py`, Godot bootstrap/config choices,
  Godot downloads/toolchain/templates and exported applications.
- Archived the old comparison source, setup/config, plans/reviews, and validation
  records in `docs/archive/toolkit-study/source-and-validation.tar.gz`. Its
  manifest contains SHA256 checksums. Restore only into a separate directory if
  needed; old plans are historical, not current instructions.
- Moved original native records unchanged to `docs/evidence/native-screening/`.
  Embedded absolute paths describe their old location. New runs go under ignored
  `results/`, never over preserved evidence.
- Kept the installed Qt SDK, Python environment, shared fixture, Qt renderer and
  profiling safeguards. Qt still needs its local SDK to rebuild.
- Replaced the comparison-focused README with current setup, preview, tests and
  resource-profile commands. Added root `AGENTS.md` to direct new sessions here.

## Suggested next session

For character polish, launch `build/qt/character_preview` to inspect target-driven
Mallow, or `build/qt/characters3d/character_preview_3d` from a normal Terminal to
validate the original Blender werewolf's native pixels. Inspect actual-size walk,
stop, reversal and force response before increasing art detail. The 2D locomotion
format now supports unequal segments with fold-radius validation; both production
2D characters use target-driven grounded walking. Native quiet CPU, soak, mixed-DPI,
focus/click-through checks, energy and monitor snapping remain independent work.
Keep resident 3D integration gated on native functional/resource evidence.

Start with the session-input record and API contract. Run the socket/CLI tests in
a normal Terminal, then exercise the simulated example and native checklist.
Trace and fix monitor-crossing snapping before closing desktop acceptance.
Proceed to stage 3 daily-use validation: quiet CPU, bounded diagnostics and soak.
Use macOS first. The current character foundation proves different anatomies;
daily-use validation and adapter extensibility remain open.
The named tool has not been selected; it does not block the resident foundation.
Preserve the golden-pose tests throughout extraction.

Native functional checks should cover pixel scaling, focus, click regions,
dragging, hide/restore, movement clipping, display changes and actual frame
presentation. Profile future changes without user interaction during timing.
Controlled whole-system battery testing follows functional validation.

Suggested opening message:

> Continue Better Pets utility and body-fixture work from `docs/utility-handoff.md`,
> `docs/handoff.md` and `reports/Pet utility and toolbox design.md`. Develop the
> first useful notification round trip and reusable presentation fixtures while
> preserving procedural grounded motion and quiet scheduling.

## Environment and workflow

Workspace: `/Users/dizzler/Projects/Repos/better-pets`.
Qt SDK: `.deps/qt/6.10.3/macos`; resident: `build/qt/better_pets`;
benchmark: `build/qt/pet_qt`.
Character preview: `build/qt/character_preview`.
Use `.venv/bin/python` for tests; system Python may lack Pillow or psutil.
Shell commands use the user's `rtk` prefix. Commands and prerequisites are in
the [README](../README.md); the build script runs CTest automatically.

The agent execution sandbox could not create native desktop windows. Offscreen
Qt is useful for software validation only. The user successfully ran the native
benchmark from normal Terminal. Do not bypass permission boundaries or label
offscreen metrics as desktop results.

This directory has no `.git` repository. Earlier initialization was denied by
the environment; no commits are claimed. Files and evidence are present locally.
The user prefers substantial independent subagent work and dislikes redundant
confirmation requests for already-authorized actions.

## Cleanup verification

Latest character implementation verification is in the
[pixel-character record](results/2026-10-08-pixel-characters.md). The cleanup
counts below are historical.

Verified after cleanup on 2026-10-08:

| Check | Observed result |
|---|---|
| Python suite (`.venv/bin/python -m unittest discover -s tests -v`) | 37 passed: 21 profiler, 12 fixture, 4 pose-verifier tests |
| Qt Release rebuild (`tools/build_qt.py`) | Passed; both CTest targets passed, including six CLI lifecycle cases |
| Fresh Qt pose export and `tools/verify.py` | All 44 reference poses match within 0.0001 |
| Qt-only bootstrap through offline locked `uv run` | Passed; Qt 6.10.3, no download needed |
| Native and offscreen config generation | One Qt candidate each; native default retained in `bench/config.local.json` |
| Single-candidate offscreen pipeline | 3/3 valid candidates plus six baselines; static, idle-motion, hidden; no paired deltas |
| Preserved study archive | SHA256 and all 392 file hashes verified |
| Moved native benchmark evidence | All 195 files verified unchanged |
| Documentation | All checked current relative links resolve |
| Independent cleanup review | No remaining must-fix findings |

Pipeline evidence is local, ignored output under
`results/cleanup-validation/longer-run/`. It explicitly reports `mode: profiling`,
`purpose: validation_only`, and unverified battery acceptance. It is not a new
desktop-performance result.

The first 0.8-second smoke window produced 2/3 valid trials: hidden root metrics
had 89.9748% coverage and were correctly rejected by the unchanged 90% threshold.
The three-second window passed all three. Both attempts are retained locally;
use several seconds for pipeline smoke checks, and meaningful longer durations
for actual resource screening.

No new native GUI or energy trial was attempted during cleanup. Stage 2 added the
generic session interface. Character formats were subsequently implemented as
recorded above; real tool adapters remain later work. The broader modular
architecture remains incremental.
