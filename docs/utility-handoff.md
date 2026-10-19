# Utility and body fixtures handoff

> Historical session notes. Use [the current handoff](handoff.md) for implemented
> features, repository status and remaining checks. Paths and future-tense claims
> below describe the original session.

The user requested clearing conversational context and starting a fresh session
with the utility direction below. Workspace:
`/Users/dizzler/Projects/Repos/clibby`. Read root AGENTS.md, docs/handoff.md
and docs/architecture.md. Shell commands use rtk; Python uses .venv/bin/python.
This directory has no Git repository. Preserve shared/, benchmark and raw evidence.

## User trial and current correction

After the body-led movement correction, the user says everything is looking good
and requests resuming functionality iteration. Character fixes no longer block
feature work. See the latest entry in [the main handoff](handoff.md); the earlier
correction status below is historical. The next feature priority is being
selected: real work notifications, Toolbox shortcuts, or timers/reminders.

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

## Implementation update

The command producer, shared actionable cards, summoned toolbox, body anchors,
semantic fallbacks and werewolf arm/grip fixtures are now implemented. Read
[the result and native trial](results/2026-10-08-utility-slice.md) before continuing.
Original proposals below describe the session's starting point. Live socket,
native focus/placement, GPU and battery acceptance are still outstanding.

## Current goal and research

The user wants useful pets rather than creatures that only walk: moving arms,
speech bubbles, held signs and attention effects; work/chat notifications; perhaps
a power menu/toolbox with tools, notifications and displays. They requested
comparison and usefulness ratings for equivalent projects. Research is saved in
`docs/research/reports/Pet utility and toolbox design.md`; underlying sourced notes are in
`docs/research/notes/Pet utility and toolbox design/`.

Recommended direction (not yet implemented):

1. Attention inbox (5/5 subjective usefulness): genuine completion, input-request,
   failure and reminder events with source grouping, quiet indicators, Open source
   and Acknowledge.
2. Summoned toolbox (4/5): curated app/project/chat actions and existing utilities.
3. Status cards (4/5): timers, reminders and provider progress.

Use one card/action model and registered action dispatcher across these surfaces,
with separate provider state and presentation policy. Avoid duplicating a large
utility suite. Scores are judgments, not measured utility or battery results.

First proposed vertical slice: one real event producer -> existing session policy
-> a brief gesture and readable above-head card -> Open source or Acknowledge ->
static unread indicator after motion settles. Select a supported source the user
actually uses; no real coding-tool adapter currently exists. Current session API
already provides working/needs_input/completed/failed, TTL, sequencing, arbitration
and acknowledgment; its current example is simulated. A response finishing does
not necessarily mean the whole task finished. Refresh/stale must be explicit
source events, never inferred from silence or TTL expiry. No private-chat scraping.

Fixtures: head-relative attention/card/effect anchors, optional grip sockets,
semantic look/wave/point/present/nod/success/failure goals. The werewolf already has
arm/hand joints; add target-driven arm IK and hand/grip goals there first. Mallow
arms can be optional, with head/ears/tail/body and bubble fallbacks. Preserve leg
IK/contact authority. Put readable text/buttons in ordinary accessible 2D UI,
even for a 3D pet. Keep one event/deadline timer; no persistent polling/gesturing.

## Implemented foundation

Qt Quick/C++17 is active. Mallow uses CPU composition, nearest sampling, target-
driven ground navigation and analytical IK, with anticipatory steps, a quarter
view and short visual turn. Original Blender werewolf has a separate optional
Quick3D live skin/IK preview, corrected projected target controls and D contact
markers. Resident stays 2D pending native GPU acceptance. See grounded and
motion-polish results records. Latest full Qt checks: 18 passed, 3 skipped (2 IPC
sandbox checks and native GPU capture). Battery/native acceptance is unverified.

Preview binaries: build/qt/character_preview and
build/qt/characters3d/character_preview_3d --target 60. Resident:
build/qt/clibby. Old 2D Bramble remains separate.

## Outstanding reference

The user called a local Mac toolbox app 'Vorsaint'. Native inventory had no such
name and getApp returned Invalid app. Finder inspection stalled and was aborted;
no app features were observed. The prior session asked for its exact app name or
.app path, but no answer arrived before the session change. Confirm identity
before inspecting it; do not invent its features. Inspect only safe utility UI,
not private content or destructive actions. Consider launching existing tools
before reimplementing them.

The utility research describes a recommendation, not an approved detailed design.
Continue toward a concrete implementation scope with the user's goal and existing
authorizations in mind; avoid redundant approval rounds for established work.
