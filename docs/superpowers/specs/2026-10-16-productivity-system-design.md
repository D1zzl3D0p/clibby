# Better Pets attention and productivity system

Date: 2026-10-16. User direction: write this specification, then begin implementation.

## Product purpose

Come for functionality, stay for personality. Productivity and companion quality
have equal weight. Better Pets helps people review missed events, identify agents
needing input, find important notifications, set timers and manage follow-up work.
Care chores and pet collection are not prerequisites for utility.

Build the shared system once. Claude Code, Codex, operating-system notifications,
timers, Pomodoro and future task providers translate their observations into the
same local event/action boundary. Pet anatomy and rendering never determine which
productivity features work.

## Architecture and invariants

- Keep the resident C++17/Qt Quick host and existing authenticated local IPC.
- Preserve session API major 1 and its existing behavior. Add productivity methods
  rather than silently changing session expiry or acknowledgment semantics.
- Introduce a focused productivity library using Qt Core for JSON/atomic local
  persistence, without widgets, desktop discovery, network calls or animation.
- Provider adapters own translation only. The common layer owns validation,
  durable inbox records, deduplication, local actions and deadline scheduling.
- One resident timer includes productivity deadlines. No provider or panel owns
  a polling timer. Timers continue while the pet is hidden or paused; no pet motion
  runs under suppression. Showing the panel refreshes its current values.
- Incoming events are data, never arbitrary commands. Source return is explicit
  user action and uses the existing validated URL boundary. Do not approve agent
  permissions, send prompts or execute provider-supplied shell commands.
- Local persistence is bounded and atomic. Report corrupt/unwritable storage;
  never silently claim durable success or overwrite an unreadable store.

## Common event model

Each event has source identity, source-owned event identity, increasing sequence,
kind, title, optional detail and source URL, creation/update times, and optional
correlation identity. First kinds: working, needs_input, response_ready, completed,
failed, cancelled and info. Timers publish through the same model. Response ready
means a turn ended, not that an entire task succeeded. Missing observations never
mean completion. A source can describe unknown/disconnected state in a later
additive extension; this slice does not infer it from elapsed silence.

Use source/event identity for idempotent upsert and sequence rejection, and retain
acknowledgment on unchanged-kind updates. A kind transition makes an item unread.
An optional occurrence_id identifies a distinct attention request within the same
event: a changed provided occurrence rearms attention even if kind is unchanged.
Repeating the same occurrence preserves local flags; omitting it removes the
metadata without itself rearming. Correlation metadata is retained for future cross-source matching; do not merge
unrelated notifications heuristically in the initial implementation.

Local acknowledgment, snooze and task completion are separate. Acknowledge means
seen; snooze temporarily removes an item from the attention view; complete means
the user's follow-up is done and does not modify the source application's state.
Opening a source changes none of these flags. Snooze wake makes an item unread
once. Completed local tasks do not reappear unless the source reports a new kind.

Implementation defaults (builder decisions): 512 inbox items; evict oldest locally
completed or acknowledged items before refusing new items at capacity; never
silently evict unread attention. At most 32 schedules. IDs use the existing
session ID grammar (1–64 ASCII letters/digits/period/underscore/hyphen). Titles
use existing 120-scalar plain-text limits. Detail is at most 2048 UTF-8 bytes.
Sequence is 0 through 2^53−1. JSON file input is capped at 4 MiB. Queries/search
and filtering operate on this bounded local history.

## Timers and Pomodoro

Provide named countdown timers and configurable Pomodoro work/short-break/long-break
durations and work-interval count before a long break. Defaults: 25/5/15 minutes,
long break after four work intervals. Durations: 1 second to seven days. Support
pause, resume, cancel and advancing to the next Pomodoro interval. Default to
manual next-interval start; optional automatic continuation is a later feature.

Persist schedule configuration, phase, remaining time/deadline and interval count.
Use injected epoch-millisecond time for restart/sleep reconciliation; a due timer
becomes one durable attention item even after a long absence. Never replay an
entire chain of missed Pomodoro phases. Pause preserves remaining duration.
Completed countdowns stop; Pomodoro waits for the next interval. UI shows phase
and state and allows linking a timer to a task label. No per-second idle redraws
are needed for the initial panel; display remaining duration when refreshed.

## User surfaces and actions

Add a Productivity entry to the resident menu/toolbox opening an accessible Qt
Widgets panel. It offers inbox search and attention/all filters, source/time/kind,
open source, acknowledge, snooze, complete and quick task capture. A second section
offers timer/Pomodoro creation, duration settings, state and pause/resume/next/cancel.
Use plain text for all provider content. Preserve existing attached cards and
Toolbox controls. Accepted important provider events can feed the existing bounded
session reaction system; historical restore must not replay gestures.

## Claude and Codex adapters

Ship a local Python standard-library helper accepting bounded JSON on stdin and
forwarding normalized events using petctl. Explicit provider selection determines
the mapping. Cover prompt submission (working), permission requests (needs input),
turn stop (response ready), explicit failure and session interruption/end where
documented. Claude Notification supports permission_prompt and idle_prompt; idle
must be labelled as a check-in rather than asserted blocked. Do not parse private
chat state or depend on hidden transcript formats. Hooks must not affect approval
decisions or prevent a turn from stopping. Use source/session identity, persisted
sequence allocation and locking so concurrent hooks cannot race. Include sample
configurations and tests; do not modify the user's global agent configurations.

Initial supported scope is documented Claude Code/Codex hook payloads. Actual
installed desktop and CLI delivery must be verified separately; sample replay and
adapter tests are not proof of live app coverage.

## Operating-system notification coverage

Treat OS capture as a source with declared capabilities (observe live, retrieve
history, open source, dismiss in source). Unknown/unsupported means unavailable,
not an empty inbox. Windows documents UserNotificationListener with consent.
macOS UNUserNotificationCenter documents only the calling app's notifications;
all-app collection remains a feasibility task, not a promised capability.
Linux needs a desktop-specific approach around the notification server. No
private database scraping, screen recording, or Accessibility capture is enabled
by this implementation. Future broad capture needs explicit per-source opt-in,
retention controls and sensitive-content exclusions. Native dismissed/read state
must not be equated with Better Pets acknowledgment.

## Delivery stages

1. Implement shared durable inbox, local actions, schedules, API/CLI, resident
   panel and tested Claude/Codex hook translation. This is the first working slice.
2. Validate real installed Claude/Codex flows and macOS notification capture
   feasibility; record exact capabilities and failure behavior per host.
3. Add supported OS adapters, cross-source correlation, source-specific importance
   and quiet routing, recurring reminders, richer task views and optional automatic
   Pomodoro continuation through the same common model.

## Acceptance for stage 1

- Events survive restart; stale/repeated sequences do not duplicate or re-alert.
- Search/filter and local acknowledgment/snooze/completion work independently.
- Unwritable/corrupt stores report errors without data destruction.
- Countdown and Pomodoro transitions are deterministic with injected time,
  including pause, restart, sleep gaps and no catch-up phase storm.
- Existing session tests remain compatible; no extra periodic resident timers.
- Hook tests cover both providers, unsupported events, malformed/oversized input,
  concurrent sequence allocation and truthful stop/input semantics.
- Panel tests exercise real shared model actions with temporary storage.
- Run existing Python and Qt suites and identify environmental skips separately.
- Native app hooks, desktop focus, notification capture and battery remain
  unverified until actually measured. Do not claim all three delivery stages done.

## Primary references checked during design

- https://code.claude.com/docs/en/hooks
- https://learn.chatgpt.com/docs/hooks
- https://developer.apple.com/documentation/usernotifications/unusernotificationcenter
- https://learn.microsoft.com/en-us/windows/uwp/design/shell/tiles-and-notifications/notification-listener
- https://specifications.freedesktop.org/notification/latest-single/
