# Better Pets resident companion specification

Date: 2026-10-08. Status: development specification based on the user-approved
direction; requirements below define the acceptance target.

Implementation status: stages 1 and 2 have software implementations. See the
[foundation record](../../results/2026-10-08-resident-foundation.md) and
[session-input record](../../results/2026-10-08-session-inputs.md) for verified
behavior and outstanding native/live transport acceptance. Implementation alone
does not establish an acceptance pass.

## Purpose and success

Build a simple, robust desktop companion that adds character to the desktop and
communicates what is happening on the computer, especially during AI coding
sessions. The application must remain useful without a cloud service, account,
model inference, or continuous monitoring of unrelated processes.

The first usable milestone is one resident pet on macOS receiving session status
through a local command-line client. A user can start a task, see when it needs
attention or finishes, pause or hide the pet, and leave the application running
for hours with bounded memory and quiet idle behavior.

The next milestone proves extensibility: the same inputs drive the existing
animal and a simple blob, and a real coding-tool adapter uses the same public
interface as the command-line client. These milestones share this design but
should have separate, small implementation plans.

## Decisions and assumptions

The user approved the priorities of resident lifecycle, separated runtime
responsibilities, a minimal session-status interface, and two different bodies.
Simplicity, robustness, extensibility, and low resource use are product requirements.

The following defaults make development concrete without claiming additional
user choices:

- Target Apple Silicon macOS first, where the prototype has been exercised.
  Windows and Linux require separate native validation before support is claimed.
- Use one pet and the current animal artwork for the first milestone. Use a blob
  as the second anatomy; exact artwork is not a prerequisite for the runtime.
- Ship a generic `petctl` integration first. Selection of a named AI coding tool
  is a follow-up product choice, not a dependency of the resident shell or API.
  Verify the chosen tool's supported integration mechanisms before implementing
  its adapter; do not infer status by scraping undocumented private state.
- Keep C++17 and Qt Quick. Use typed, in-process boundaries and data-only packs.
  No dynamic plugin framework or extra resident service is required.

This specification governs the milestones below. The broader
[architecture direction](../../architecture.md) remains the guide for later
extensions. The [toolkit findings](../../results/2026-10-08-comparison.md) retain
their original evidentiary limits.

## Baseline when this specification was written

The current `qt/` application is a timed benchmark harness. It combines argument
parsing, loading, scheduling, window placement, dragging, and telemetry in
`main.cpp`. `pet_motion` depends on Qt Core and reads JSON-backed motion values.
The loader requires the existing twelve named parts. There is no resident shell,
public integration endpoint, or production character loader.

Preserve `shared/` as the frozen reference, including its 60 Hz equations and 44
golden poses. Preserve the benchmark CLI and output contracts while sharing
extracted components where useful. Raw evidence and archives are immutable; new
profiling output goes in ignored `results/` directories with new run names.

## Scope and delivery order

| Stage | Deliverable | Exit condition |
| --- | --- | --- |
| 1. Resident foundation | Application entry point, desktop host, runtime state, event-driven scheduler, menu-bar controls | Launch, drag, pause, hide/restore, sleep/resume, display changes, and quit work without benchmark arguments |
| 2. Useful notifications | Local API, `petctl`, session arbitration, status text and reactions | Two simultaneous simulated sessions remain understandable through updates, expiry, pause, and reconnect |
| 3. Daily-use validation | Native functional checks, bounded diagnostics, resource profiles and soak run | Acceptance checks below pass with recorded evidence; first usable milestone complete |
| 4. Extension proof | Data-only character loading, animal and blob controllers, one selected coding-tool adapter | Both characters consume unchanged integration messages; adding the adapter does not modify renderer or motion code |

Stage 1 is the first implementation task. Extract only the boundaries needed by
that stage; do not make a complete pack ecosystem a prerequisite for a resident
pet. Design the small typed inputs with stages 2 and 4 in mind.

Outside these milestones: multiple simultaneous pets, automatic launch at login,
marketplace/download infrastructure, executable third-party packs, a rig editor,
full physics, walking on other application windows, global input observation,
remote/network control, cloud synchronization, and automatic coding-agent actions.
Screen-bound wandering can be added later through behavior intents; it is not
required for the initial notification flow.

## User experience and lifecycle

Launch opens one transparent pet window and a persistent menu-bar control. A
second launch must discover the existing instance and exit without creating a
second pet. The menu provides Show/Hide, Pause/Resume, session status, and Quit;
character selection is added in stage 4. The pet must not take keyboard focus
when appearing, animating, receiving an update, or being restored.

Dragging moves the pet and temporarily owns placement. Transparent areas outside
the interaction region pass pointer input through. Releasing the pet preserves
its realized position without a jump. A context menu replaces the benchmark's
right-click-to-quit behavior in the resident app; the benchmark keeps its behavior.

Visibility and pause are independent flags. Hide removes the pet window while
leaving the menu and IPC available. Pause freezes animation and suppresses
reactions while allowing status data and menu text to update. The paused pet's
pose and label remain frozen; synchronize its label once on resume. Show does not clear
pause. Resume does not show a hidden pet. Neither operation replays missed
animations. Quit closes IPC, releases instance ownership, and exits cleanly.

On system sleep, suspend animation. On resume, remove all session records,
sequence history, acknowledgment state, active reactions and reaction cooldown
before choosing a visual state. Discard elapsed animation time. New post-resume
updates are fresh session creations; adapters must not resend obsolete tasks.
This avoids depending on whether a particular clock counts suspended time.
Restore a valid screen position before drawing, retaining visibility and pause.

Persist only selected character, last position/display, and pause preference.
Launch visibly so a previously hidden pet remains discoverable. Never persist
live session state or notification history. Missing or invalid settings fall
back to safe defaults. If a saved display is absent, place the pet in the primary
display's available area. React to screen removal and geometry/scaling changes;
keep the pet reachable and its logical and device coordinates consistent.

## Components and contracts

| Component | Responsibility and boundary |
| --- | --- |
| Core runtime | Qt-free C++ values, session state, priority rules, reaction deadlines, behavior intents; no JSON, windows, or OS calls |
| Motion controller | Character definition plus intent and bounded time step produce pose and desired root displacement; no integration-specific branches |
| Renderer | Retained Qt Quick scene graph, textures and pose application; preserve current resource reuse and render-thread ownership |
| Desktop host | Window lifecycle, interaction regions, dragging, display geometry and focus policy; reports actual placement and supported capabilities |
| Scheduler | Computes the next required deadline from runtime state and visibility; owns timers outside the core |
| Integration adapter | Validates external messages, applies permissions and limits, and converts them into typed session updates |
| Character loader | Validates data and constructs immutable definitions; added incrementally for the second milestone |
| Application shell | Composes services, settings, menu actions, startup and shutdown; benchmark reporting stays in the benchmark entry point |

The minimal internal values are `SessionUpdate`, `RuntimeControl`,
`DesktopObservation`, `BehaviorIntent`, `PoseSnapshot`, and `PlacementFeedback`.
Use enums and ordinary C++ values, not scenario strings or JSON in the motion
loop. An intent names an action such as rest, attend, celebrate, or express
failure, with intensity and an optional target. A pose contains indexed local
transforms; external clients never supply joint transforms.

Desktop observations initially contain available screen rectangles, display IDs,
visibility, and drag state. Coordinates are logical desktop units at the host
boundary and character-local units within a rig. The host owns conversions.
Desired movement and actual clipped placement remain separate values. Generic
world-object publication and state subscriptions are deferred until used by a
concrete feature.

## Session state and notification behavior

A session is identified by `(source_id, session_id)`. Each update carries a
bounded plain-text display label, an increasing sequence number, a status, and a
time to live. The four public statuses are `working`, `needs_input`, `completed`,
and `failed`. Absence or expiry means unknown/inactive; it never means success.

| Status | Default visible response | Lifetime |
| --- | --- | --- |
| `working` | Brief acknowledgment, then quiet rest; text identifies the session | Renew while the task is active |
| `needs_input` | Brief attention gesture and a static status indicator | Remain indicated until updated, acknowledged, or expired |
| `completed` | Brief celebration and status text | Expire automatically; no looping celebration |
| `failed` | Brief distinct failure gesture and a static status indicator | Remain indicated until updated, acknowledged, or expired |

Acknowledgment is a local user action available in the session menu. It suppresses
that status's attention indicator and reaction without changing the reported
status or expiry. A later status transition clears acknowledgment. Refreshing an
unchanged status never restarts a reaction. A new task reuses a session ID only
with a greater sequence number, or chooses a new ID.

Choose the foreground session by unacknowledged `needs_input`, unacknowledged
`failed`, `completed`, then `working`; ties use the most recent status transition,
then lexical session key. Acknowledged `needs_input` and `failed` sessions are
excluded from foreground selection but remain in the menu. If no eligible session
remains, show neutral rest without a session label. The menu lists all active sessions;
the pet's short label shows source, session label, and status. This ensures the
meaning is understandable without interpreting animation or color alone.

Priority is quit/system suspension, hidden or paused state, direct dragging,
notification reaction, then ambient behavior. Notifications never move the pet's
window, open another application, play audio, or steal focus. Dragging suppresses
reactions but retains current session data. When suppression ends, display the
current status without replaying gestures received during suppression.

Only a newly accepted status transition of the resulting foreground session can
start a reaction. Initial creation counts as a transition; unchanged refreshes
do not. Foreground changes caused by expiry, clear, acknowledgment, or suppression
ending update the display without creating a reaction. Cancel an active reaction
when its session/status ceases to be foreground or suppression begins; never
resume a canceled reaction.

Only one reaction runs at a time, for at most three seconds. Keep no queue of
historical reactions. The five-second reaction-start cooldown applies to every
status, including higher-priority arrivals. During cooldown, discard the gesture
and update the label immediately when visible and unpaused; otherwise update
session data and menu text only. These are initial tunable product defaults,
not benchmark fixture parameters.

## Local integration interface

Provide version 1 JSON-RPC requests over a local Unix-domain socket on macOS,
with one UTF-8 JSON object per line. `petctl` uses the same interface as future
adapters. Use a per-user private runtime directory, owner-only endpoint access,
and same-user peer verification. Endpoint discovery and instance locking belong
to the host adapter. Never delete an endpoint belonging to a live instance.

The initial permission profile trusts processes running as the same OS user to
publish session status and query the bounded snapshot. Source IDs are namespaces,
not authenticated application identities. Other OS users are denied; isolated
permissions between same-user applications are outside this milestone. No
integration method executes code, loads arbitrary files, quits the app, or changes
user pause/visibility preferences.

| Method | Request | Result |
| --- | --- | --- |
| `hello` | `api_major: 1` | API major, supported methods, statuses and limits |
| `session.update` | Source/session IDs, label, sequence, status, TTL | Accepted sequence and effective TTL, or explicit rejection |
| `session.clear` | Source/session IDs and sequence | Clear session; retain the accepted clear sequence until its existing expiry, without extending that expiry |
| `state.get` | No parameters | Visibility, pause, selected character, foreground session and bounded active session list |

Perform `hello` before other requests on each connection. Reject unsupported
major versions. Version 1 is additive only: clients ignore unknown response
fields; incompatible semantics require a new major. Requests require IDs and
receive responses; batches and fire-and-forget notifications are not supported
in this first profile. Invalid JSON, methods, parameters, permission, version,
stale sequence, and capacity errors produce machine-readable failures. Publish
the exact JSON schema, error-code table, and command help alongside the API
implementation before an external adapter depends on it.

Initial limits are 16 KiB per framed request, 16 connected clients, 32 session
records including cleared records awaiting expiry, 64 pending requests globally,
64 ASCII characters per source/session ID, and 120 Unicode characters per label.
IDs use letters, digits, periods, underscores and hyphens; labels reject control
characters and are rendered as plain text. Sequences are integers from 0 through
2^53-1 and increase per session key. TTL is an integer from 1 through 3600 seconds,
defaulting to 120 seconds; clients should refresh active state before expiry.
Reject oversized or incomplete frames after a five-second receive deadline and
bound response buffers to 64 KiB per connection, disconnecting slow readers.
Do not allocate according to untrusted declared lengths.

Rate-limit each connection to ten requests per second with a burst of twenty;
the global queue limit also applies across reconnects. Reject excess input without
blocking animation or the menu. Store only the latest accepted state per session;
repeated sequence numbers are rejected and cannot renew TTL. Clear of an unknown
key succeeds without allocating a record. Removing an expired record also removes
its sequence history; senders must not replay an expired task as fresh work.

Disconnect does not immediately clear sessions because `petctl` is short-lived.
TTL is the authority for stale state, measured from acceptance using an injected
monotonic clock during normal operation. Expiry is processed before arbitration
and queries. Adapters send clear when possible on cancellation, and must not
claim completion merely because their transport or process exited.

The CLI exposes update, clear, and state commands with explicit failure exit
codes. A checked-in example drives all four statuses and two concurrent sessions.
It must be labeled simulated input; a real coding-tool integration is accepted
only after observing actual supported events from the selected tool.

## Character extension proof

Keep fixture loading available to the benchmark. Create a separate versioned
production definition for stage 4, starting with local directories containing a
manifest, raster atlas, rig, controller parameters, interaction geometry, preview,
and attribution/license information. ZIP import and runtime SVG rendering are
deferred. Manifest IDs and versions are independent of the external API version.

Each character declares its built-in controller and capabilities. Rigs specify
local transforms, pivots, parent relationships, independent draw order, and
semantic bindings. The animal can bind head, limbs and tail; the blob uses its
own squash/stretch controller. Both support rest and all four status responses
without a common mandatory skeleton. Missing optional capabilities select a
documented fallback; missing required capabilities reject the pack.

Validate finite bounded numbers, acyclic hierarchies, references, image/atlas
bounds, local asset paths, and controller compatibility before replacing a live
character. Reject path traversal and symlink escapes. Initial limits are 64 parts,
one atlas of at most 2048 by 2048 pixels, 32 MiB total package files and 1 MiB JSON
metadata. Cap reads and inspect image dimensions before allocating decoded data.
Packs contain no executable QML, JavaScript, native code, or shaders.

Character switching loads and validates first, then swaps definitions and renderer
resources at a safe boundary. Failure leaves the old pet running and shows a
useful error. Switching preserves runtime/session state and a reachable desktop
position. A fallback label communicates status even where motion is unavailable.

## Resource behavior and diagnostics

Quiet mode is the default. The pet holds a resting pose between brief ambient
gestures lasting at most two seconds, spaced at least thirty seconds apart.
Working status does not require continuous animation. Optional continuous idle
animation is a later preference, not the default cost of receiving notifications.

Hidden and paused states schedule no animation ticks or redraw requests after
settling. They may still process IPC, menu input, OS events, and session-expiry
deadlines. Use one next-deadline wakeup for expiry/behavior work instead of polling.
An empty hidden/paused runtime has no periodic application timer.

Active animation has a configurable ceiling of 60 Hz and at most four catch-up
steps per callback. Drop excess elapsed time, count it, and avoid catch-up after
suspension. Schedule drawing only when visual state changes. Preserve retained
scene-graph nodes and textures; avoid JSON parsing and allocation growth per frame.

Resident diagnostics use counters and an optional ring of at most 256 samples.
Disable unbounded frame-interval history and continuous disk logging in normal
operation. Detailed telemetry remains an explicit bounded benchmark operation.

Initial native screening targets on the reference Apple Silicon machine are
less than 1% mean process CPU for a five-minute quiet-idle run (100% is one core)
and no more than 20 MiB growth between first-hour and final-hour median RSS in an
eight-hour soak after warmup. Record absolute RSS, workload, visibility, hardware,
OS/build, background activity and coverage; these targets are proposed engineering
gates, not measured results or proof of no leak. Any failure requires investigation
and a documented decision before declaring the milestone complete.

Battery-runtime impact remains unverified. After functional acceptance, run
controlled native whole-system energy/battery comparisons with no-pet baselines.
The earlier proposed under-5% battery-runtime-loss objective remains a separate
validation target. Missing power evidence must be reported as missing, never as
a pass inferred from RSS, CPU, or offscreen tests.

## Acceptance and verification

| Area | Required evidence |
| --- | --- |
| Regression | Existing Python suite and Qt CTest targets pass; all 44 golden poses still match within 0.0001; benchmark CLI and telemetry remain compatible |
| Lifecycle | Native launch without scenario/duration/output, duplicate launch, pause/resume, hide/show, explicit quit and repeated restart succeed |
| Scheduling | Fake-clock tests prove no animation ticks while hidden/paused, bounded catch-up, correct TTL deadlines and no replay after suppression or sleep |
| Desktop | Native checks cover focus preservation, click-through regions, dragging without jumps, scaling, monitor removal/reconnection and reachable placement |
| Notifications | All four statuses and two simultaneous sources follow priority rules; expiry, acknowledgment, stale sequences and cooldown are deterministic |
| IPC | Valid CLI flow works; malformed/oversized input, incompatible versions, unauthorized peers, floods, slow clients and reconnects remain bounded and responsive |
| Resource use | Counters/buffers remain capped under synthetic long runs; native quiet CPU and eight-hour memory targets are evaluated with new evidence |
| Extensibility | In stage 4, animal and blob react through identical messages; invalid packs preserve the live pet; a real adapter requires no renderer/controller changes |

Offscreen runs validate software behavior only. Native desktop checks and energy
measurements require a session with real desktop access; record unperformed checks
as outstanding. Do not declare an entire milestone complete from unit tests alone.

## Development handoff

Create the first implementation plan for stage 1 only: resident entry point,
minimal runtime and host extraction, lifecycle controls, bounded diagnostics,
and lifecycle verification. Preserve the renderer and fixture behavior while
extracting dependencies. Later plans implement the API and extension proof using
the same contract vocabulary, refining exact wire and pack schemas with tests.

The working directory currently has no Git repository. Establish version control
before implementation where the environment permits, preserving all existing
files and ignore rules; this document does not imply a commit already exists.

The named coding-tool choice is the only deferred product selection for stage 4.
It does not block stages 1–3. Numerical limits and visual timings above are
explicit starting defaults; changes must update this specification and the
associated acceptance checks together.
