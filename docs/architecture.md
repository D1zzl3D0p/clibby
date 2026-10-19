# Architecture direction

This document combines implemented updates with the original incremental design.
Use the newest updates here and [the handoff](handoff.md) for delivery status;
dated result records retain their original acceptance limits.

Update, 2026-10-18 (directional leading plant): spatial quadrupeds choose a
world step into the new heading before planning body motion. The leading paw
plants with the body held, then the body follows that support. Six placements
finish in .84s through overlapping fore/hind transfers; at least two contacts
remain grounded. Bipeds keep two placements. Loaded positions and orientations
stay pinned. Full-plan geometry admission checks actual links, upright height
and bounds; host reconciliation translates all directional waypoints, and
retargeting waits for a safe grounded boundary. Subpixel heading corrections
settle without stepping. Quadruped resident views read authored yaw angles
(default -35°), replacing the former hardcoded -10.8°; all final audits must use
actual runtime properties, not a guessed rig-to-view mapping. Explicit API turns
above140° retain the bounded paired fallback. This is kinematic support transfer,
not a COM/force simulation. Selected walking speed and longer spatial strides
remain; planar defaults are unchanged. See the
[turning record](results/2026-10-17-natural-turning.md).

Update, 2026-10-16: the [productivity system design](superpowers/specs/2026-10-16-productivity-system-design.md)
sets functionality and personality at equal weight. `qt/productivity/` now owns a
shared Qt Core persistent inbox/schedule service and separate Widgets panel.
`productivity.*` extends authenticated local IPC without changing temporary session
semantics. Claude/Codex hooks normalize into this common boundary; source occurrence
IDs distinguish genuinely new attention requests from refreshes. The resident
bridges accepted events to bounded pet reactions and adds productivity deadlines
to its existing single timer. OS notification providers are still a capability and
feasibility boundary, not implemented universal capture. See the
[implementation record](results/2026-10-16-productivity-foundation.md).

Update, 2026-10-16: the first catalogue expansion adds thirteen original skinned
biped/quadruped base pets through a fixed trusted asset registry and generic build
conversion. Optional arms/ears/tails derive actual expression capabilities;
bounded personality tuning retains the same scheduler and old defaults. Four new
families have separate deterministic motion prototypes, not production resident
adapters. See the [catalogue delivery record](results/2026-10-16-catalogue-base-implementation.md).

Update, 2026-10-16: the trusted spatial adapter now supports bounded two/four-leg
anatomy. Marmalade 3D uses the shared quadruped planner, measured spatial IK,
skinned woodland artwork and existing timerless resident path. Contact yaw,
support/load calibration, projected masks and host footprint include all active
paws; arm capabilities remain separate. The 96-pixel nearest stage preserves
the selected art density. See the
[spatial quadruped record](results/2026-10-16-spatial-quadruped.md).

Update, 2026-10-10: the trusted built-in 3D werewolf now uses the same resident
host, controls, sessions and timer through a timerless `SpatialCharacter` adapter.
Motion proposes model-world travel and accepts host-realized placement; projected
masks and utility anchors share the scene calibration. Optional 3D dependencies
retain a 2D-only build. The initial delivery and remaining native/energy checks
are recorded in [resident 3D](results/2026-10-10-resident-3d.md). Broader downloaded
3D package and desktop-object contracts remain proposals.

Status: design direction recorded on 2026-10-08 with the resident foundation,
stage-2 session inputs and initial skeletal pixel characters implemented. The
[session API](api/session-v1.md) and [character directory format](characters-v1.md)
are defined; broader world and 3D contracts remain proposals. The benchmark is a comparison fixture; the product
direction is a modular desktop pet with hand-authored characters and external inputs.

The [resident companion specification](superpowers/specs/2026-10-08-resident-companion-design.md)
now defines the initial delivery scope and development requirements. Its staged
resident-first sequence refines the broader direction below. The
[stage-1 record](results/2026-10-08-resident-foundation.md) and
[session-input record](results/2026-10-08-session-inputs.md) track software evidence,
the open monitor-crossing defect and outstanding live IPC/native acceptance.

## Eye blink presentation

A Qt-free `Blink` driver produces bounded openness with seeded sparse deadlines.
The resident merges those deadlines into its existing timer and redraws only
when openness changes. Eye activity does not wake limb motion. The 2D pose carries
one scalar consumed by eye-only artwork transforms; the trusted 3D binding scales
separate eye joints. Quiet eyes produce no frame ticks, and suppression freezes
progress without catch-up. Capability is optional. See the
[blink record](results/2026-10-12-eye-blinks.md).

## Nearby cursor observations

The resident samples the global cursor in logical pixels through a replaceable
observation source. Posed head position and pet width normalize proximity;
`Curiosity::Observation::cursorOffset` adds one optional target without renderer
or OS dependencies. Attention owns procedural gaze/ear goals, uses proximity
hysteresis and filters small direction changes. The resident converts held
screen directions into the current planar/spatial frame. Spatial `Motion::face`
and planar `GroundedController::face` change heading without changing navigation;
nearby lateral attention can turn either resident in place. Spatial turns use
the yaw follower and foot replants; planar turns use the facing blend with fixed
sole contacts. Both share a center zone and cooldown to prevent rapid reversals.
Cursor polling shares
the single timer (500 ms distant, 100 ms nearby); suppression cancels it and
stationary goals settle without frame ticks. See the
[cursor record](results/2026-10-10-nearby-cursor-attention.md).

## Autonomous behavior and procedural expression

Implemented 2026-10-10: `pet_behavior` is a Qt-free seeded activity chooser. It
consumes capability, suppression, usable-span and navigation observations and
emits semantic look, role-goal, impulse, reach and travel commands. Character
profiles vary timing and selection without introducing package schema changes.
`ExpressionMotion` tracks independent head, ears, tail and wrist roles through
bounded second-order followers. Spatial reach tracks endpoints before measured
arm IK; clearing a goal returns smoothly. Renderers only consume solved poses.

The resident dispatches these intents alongside notification goals and grounded
navigation. Quiet activity and reaction-phase deadlines join its existing timer;
only unsettled motion requests frame ticks. Held nonzero goals settle. Hidden,
paused, dragged, suspended and manually controlled states preempt autonomy;
missed activities never queue. Existing planted contacts and gait stay shared.
The frozen fixture retains its legacy ambient behavior. See the
[implementation record](results/2026-10-10-curious-procedural-behavior.md).

## Direction and current reality

The [pixel-character implementation](results/2026-10-08-pixel-characters.md) adds
`pet_character_core` (Qt-free bones, poses, actions and bounded secondary motion),
`pet_character` (validated version-1 JSON/image boundary) and
`pet_character_render` (retained 2D attachments plus an experimental CPU comparison
mode). The resident and interactive preview share this production path. Bones and
art attachments are separate; semantic controller bindings do not enforce a fixed
anatomy. See the [format](characters-v1.md). 3D shares the proposed action/anatomy
concepts through a separate optional Qt Quick 3D preview and original Blender
model. Its Qt-free pose adapter shares ground navigation/contact planning while
solving the measured 3D rig separately. See the
[grounded record](results/2026-10-08-grounded-characters.md). Resident 3D integration is implemented; broader native GPU acceptance remains open.

Use C++ for the Qt Quick runtime and QML for UI and a future rig editor. Retain the
current C++17 baseline unless a concrete requirement justifies changing it. Do not add
Rust merely to create a boundary. Python remains useful for asset tools and experiments.

The preserved benchmark `main.cpp` still combines scenario timing, placement and
telemetry. The resident `clibby` entry point composes separate services:
Qt-free `pets::Runtime` computes suppression and quiet/active deadlines;
`pets::DesktopHost` owns the transparent window, renderer and display placement;
`pets::ResidentApplication` owns the precise single-shot timer, menu, preferences,
lock and RAII macOS sleep observer. `pets::Sessions` owns bounded session policy;
`IpcServer` validates authenticated local transport before dispatching to the shell.
`petctl` uses that same public boundary. The shell advances a typed `MotionPhase`
fixture adapter with cached numeric parameters and reuses `PetItem` without
benchmark telemetry hooks. Typed notification intents add bounded local pose
offsets without changing window placement. A shared Qt Widgets card presents
Open source/Acknowledge above the posed head anchor and in a summoned toolbox;
a sequence-aware registered dispatcher revalidates user actions. The real
command-outcome wrapper uses the existing session boundary. Cards own no timers.
See the [utility slice](results/2026-10-08-utility-slice.md). Hidden and paused states have no
animation ticks; the same application timer still handles pending session expiry.
Placement saves on drag release, pause changes, display reconciliation and
shutdown. Runtime diagnostics retain saturating counters, not frame history.

`pet_motion` still depends on Qt Core and remains the frozen fixture adapter.
Opt-in Claude/Codex hook adapters now use the productivity boundary. Broader
object-aware behavior remains proposed. `Runtime` and `Sessions` are
Qt-free; motion and display still use Qt types. Small internal boundaries do not
freeze future character contracts. macOS sleep observation is native;
other platforms use a stub and have no support claim. Actual desktop behavior
and battery impact still need native evidence.

Initially use modular libraries in one process and typed interfaces on the hot path.
External integrations can run separately where useful. Compiled component replacement
requires rebuild/restart; future pack reload is separate. Modularity needs no dynamic plugins.

The frozen twelve-part rig and motion contract in `shared/` remain benchmark fixtures
and regression references, not a universal anatomy or production character format.

## Proposed boundaries

- **Core:** Qt-free C++ value types and deterministic behavior/motion logic.
  Typed inputs and outputs; no windows, QML objects, JSON parsing, or OS calls.
- **Behavior:** chooses actions and targets from inputs, context, and personality.
  Produces intents such as rest, look, approach, or walk; does not draw or move OS
  windows directly.
- **Motion and rig:** resolve intents against available anatomy and capabilities,
  producing poses, contact states, and desired root motion.
- **Renderer:** converts pose snapshots into retained Qt Quick scene-graph state.
  Owns textures, ordered geometry, and render-thread synchronization. It does not
  choose behaviors or discover desktop geometry.
- **Desktop host:** owns transparent windows, focus policy, pointer delivery,
  dragging, logical/device coordinates, visibility, and supported positioning.
- **Surface providers:** expose typed usable regions, edges, obstacles, or other
  world geometry, with source identity and lifetime. Start with screen bounds;
  window edges and integration-supplied surfaces are separate optional providers.
- **Input adapters:** normalize pointer interaction and external integration data.
  Permissions and validation precede delivery to the core.
- **Pack loader:** validates versioned data, resolves capabilities, and constructs
  immutable character definitions. QML UI controls trusted runtime services.

The flow is input/world observations → behavior intents → motion/pose → renderer
and desktop host. Host feedback (clipping or unavailable surfaces) returns as typed
observations. Distinguish desired from realized motion for gait/contact decisions.

## External integration direction

Versioned JSON-RPC over authenticated local IPC is implemented, with `petctl`
as one client. See the [session API](api/session-v1.md) and
[productivity API](api/productivity-v1.md) for discovery, schemas and limits.
The broader input surfaces below remain design direction. Shell commands,
editor adapters, and other apps should share this integration path.

Four input surfaces express different lifetimes and semantics:
- **Commands:** explicit requests such as pause, select a pet, or look at a target;
  return acceptance, rejection, and any relevant completion status.
- **Events:** transient observations such as a completed task or notification;
  they are not indefinitely retained state.
- **State:** query or subscribe to authoritative runtime state, such as activity,
  position, and capabilities. Distinguish that from client-supplied, source-owned
  context such as a focus session, with expiry or explicit clearing.
- **World objects:** identified targets, regions, or surfaces, with declared
  coordinate spaces, source ownership, and lifecycle.

Negotiate API version and capabilities. Give each client only approved abilities;
local IPC alone does not establish permission. Use source identity, bounded
payloads, rate limits, and TTLs for temporary inputs. Remove or expire stale state
and objects after disconnect according to an explicit policy. Decide arbitration
centrally: direct user control, dragging, pause, and safety bounds take priority
above optional external reactions. Define cancellation and conflict behavior.

Apply backpressure: bounded queues, coalescing of replaceable state, and visible
rejection or dropping rules for excess events. Translate JSON at the boundary
into typed messages. No JSON, IPC round trips, or external integration callbacks
in the per-frame pose/render path. Integrations request intent; they do not stream
individual joint transforms by default.

## Character packages and authoring

The [catalogue expansion design](superpowers/specs/2026-10-16-catalogue-architecture-design.md)
maps 18 concepts onto explicit body families and staged actions. Its
[quadruped slice](superpowers/specs/2026-10-16-quadruped-foundation-design.md)
starts with four named contacts and a conservative planar walk. Shared support
math has two/four-contact overloads and normalized neutral calibration. The
live loader and planar planner now support four canonical leg roles, conservative
single-paw gait and all-contact host aggregation, with Marmalade as the reference.
The spatial adapter also supports two/four-leg anatomy, as recorded above. See the
[planar implementation status](results/2026-10-16-planar-quadruped.md) and the
[earlier foundation record](results/2026-10-16-catalogue-and-quadruped-foundation.md).

The initial directory-based production format and the original Mallow/Bramble
examples are implemented. Editable Pillow shape recipes draw a few raster pieces,
not animation frames. Grounded Mallow defaults to nearest-filtered CPU composition; Bramble uses
retained 2D.
changing characters validates before swapping, preserves session state and resolves
saved manifest identity within its configured sibling-directory catalog.
Grounded Mallow uses planted world-space contacts, reach-triggered generated
steps and analytical two-link IK. Its second-order followers expose frequency,
damping and response. The host returns realized integer window movement to the
controller; ground deadlines join the existing timer and disappear on settling.
Legacy packages retain bounded local springs/contact correction. Full rigid-body physics remains deferred. Bounded autonomous curiosity and
travel now use the shared behavior scheduler described above. Drag motion can drive these
local responses while direct placement remains host-owned; pause still wins.
Sparse spring deadlines join the existing application timer and disappear when
settled. Preview palette/dither/debug modes opt into CPU composition with changed-
frame texture uploads; their performance and battery suitability are unverified.

Use versioned data-only packages with a manifest, preview, asset attribution and
licenses, art, rig, and declared capabilities. Keep these concepts distinct:

- Art: hand-authored SVG/PNG sources, exported runtime atlas, filtering, variants.
- Anatomy: local transforms, pivots, hierarchy, independent draw order, semantic
  roles, joint limits, and contact anchors.
- Motion: built-in controller selection and bounded parameters for gaze,
  breathing, springs, locomotion, or squash/stretch.
- Personality/behavior: optional preferences, reaction weights, and timing;
  separate from geometry and executable runtime logic.
- Interaction: click/petting hit regions, movement colliders, and contact anchors
  serve different purposes and must not collapse into one conservative polygon.

Support biped, quadruped, and blob designs through capability-aware controllers.
Skin swaps require compatible rigs; different anatomies supply their own rig and
bindings. Missing optional features can be disabled; missing required capabilities
reject the controller with an explanation. Downloaded QML, JavaScript, native plugins,
and shaders are outside the default pack format.

Start authoring with named layers and a template rig. Later add a GUI for pivots,
semantic roles, anchors, motion sliders, and previewing representative actions.
Export a validated ZIP with JSON, images, preview, and licenses. Validate finite
numbers, acyclic hierarchy, references, atlas bounds, capabilities, archive paths,
and resource limits. Source artwork should remain editable; runtime SVG rendering
and a universal animation editor are not prerequisites.

## Raven airborne motion and shared support

Production motion has one Qt-free airborne owner for held, ballistic, powered,
landing and recovering phases. Wings expose solved shoulder/elbow/wrist state;
actual extension and stroke velocity generate bounded lift and thrust. Both
planar and spatial poses consume this state. Missing complete wing anatomy
leaves a pet unable to fly. A hop remains a ballistic launch.

Window geometry enters as fresh source-qualified horizontal support segments.
The desktop host owns realized placement and selected support; changing the
floor reference preserves pending displacement and momentum. New sole contacts
are bounded to the usable segment. Surface movement carries relative contacts
and destinations; support loss hands motion back to gravity. Placement and
walking are shared by every grounded pet, while autonomous flight requires wing
capability. macOS supplies geometry only; unsupported providers report absence.
The resident samples available geometry at 500 ms through its existing timer and
stops sampling under suppression. See the
[raven record](results/2026-10-10-raven-flight-and-perching.md) for verification.

## Desktop and power constraints

Native adapters must expose capabilities rather than promise identical behavior
on every OS. In particular, ordinary Wayland clients cannot assume arbitrary
global positioning, global input, or knowledge of other windows. Desktop-specific
protocols/extensions may be needed; provide supported fallback behavior when a
capability is absent. Verify each target platform before committing to its UX.

Use event-driven scheduling. Frozen or hidden pets should stop application
animation scheduling; sparse deadlines can wake resting behavior. Retain GPU
resources, avoid per-frame allocations/uploads, and request redraws only when
visual state changes. Active tick rate, catch-up policy, and interpolation are
production design choices; the fixture's 60 Hz contract remains unchanged.
Battery impact is unverified: architecture and process CPU measurements do not establish
whole-system battery savings or the proposed under-5% target.

## Original milestone and open decisions (2026-10-08)

This section preserves the initial scope. Character examples, local IPC transport,
versioned APIs and the first integrations have since been implemented; use the
updates above for current status.

Prove the boundaries with two materially different body types and two external
integrations, using the same runtime and API. Select the specific examples before
implementation. First define typed intent/pose/world interfaces and a small pack
and IPC contract, then extract the core, renderer, and desktop host in stages.
Preserve golden-pose regression coverage and verify behavior at each boundary.

Open questions: target OS order; the first two anatomies and integrations;
capability vocabulary; movement/contact conventions; API versioning and IPC
transport; client permissions and arbitration; package compatibility policy;
rest/active timing; and authoring ergonomics. Do not freeze a broad schema before
those examples exercise it. Marketplace infrastructure, executable third-party
plugins, full physics, and a general animation tool are outside this milestone.

## Primary references

- [Qt C++/QML integration](https://doc.qt.io/qt-6/qtqml-cppintegration-overview.html)
- [Qt Quick scene graph and synchronization](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html)
- [Qt security model: QML is trusted application code](https://doc.qt.io/qt-6/shared-security-model.html)
- [Official Python bindings](https://doc.qt.io/qtforpython-6/)
- [Qt Bridge for Rust 0.3 beta, September 2026](https://www.qt.io/blog/qt-bridge-for-rust-0.3-cxx-qt-compatibility-and-soundness)
- [JSON-RPC 2.0 specification](https://www.jsonrpc.org/specification)
- [Qt local IPC socket](https://doc.qt.io/qt-6/qlocalsocket.html)
- [Qt window positioning and platform limitations](https://doc.qt.io/qt-6/qwindow.html#position)
