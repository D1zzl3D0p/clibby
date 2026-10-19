# Shared productivity foundation — 2026-10-16

## Delivered

The user requested a specification followed by implementation, with equal emphasis
on functionality and personality. The [specification](../superpowers/specs/2026-10-16-productivity-system-design.md)
and [implementation plan](../superpowers/plans/2026-10-16-productivity-system.md)
describe the staged direction. An independent builder check found no blocking gaps.

- Shared Qt Core service with 512-item atomic durable inbox, search, byte-bounded
  pagination, stale-sequence rejection and separate acknowledge/snooze/complete.
- Up to 32 countdown/Pomodoro schedules, persisted across restarts, manually
  advancing work/short-break/long-break phases, with pause/resume/cancel and bounded
  missed-deadline reconciliation. No new recurring resident/panel timers.
- Productivity panel reachable from tray/context menu and existing Toolbox,
  including while the pet is hidden; plain-text source/time/kind, task capture,
  source return and configurable intervals. Remaining time updates on refresh.
- Additive productivity IPC and petctl operations; existing session API preserved.
- Claude/Codex hook translation with locked persistent sequencing, neutral JSON
  hook output, bounded delivery and no global hook/settings modification.
- Optional occurrence_id distinguishes new same-kind requests; internal typed
  session attentionId mirrors occurrence changes without changing legacy clients.

API and examples: [productivity](../api/productivity-v1.md),
[agent hooks](../api/agent-hooks.md). Hooks deliberately transmit fixed lifecycle
metadata rather than raw prompts, transcripts or tool output. They do not invent
source links or approve permissions. Stop means response ready, not task success.
PostToolUse after a matching approval restores working; separate direct permission
requests receive distinct occurrence identities. Idle check-ins remain informational.

## Review and corrections

Independent code review identified and regression-tested these corrections:

- Cancelled/ended agent events clear obsolete temporary input-needed cards.
- API reads at a deadline do not silently consume the timer's pet cue.
- A full inbox allows the first acknowledgment/completion/cancellation repair;
  failed reconciliation suspends scheduling and retains an error.
- New same-kind approval requests rearm acknowledged attention; delayed duplicate
  Claude permission notifications do not restore a completed wait.
- Plain-text angles remain literal, byte-limited pages are contiguous, atomic
  writes are owner-only, and failed writes preserve model/file state.

Final independent review found no additional important issues. Specification
choices remain: manual Pomodoro continuation, bounded latest-per-event history,
no global installation, legacy session providers remain temporary, native capture
and installed-app coverage are later validation. Initial service scaffolding tests
were written before implementation, but its first configure failure was a missing
Qt prefix rather than a demonstrated missing-feature test failure. Subsequent
service edge-case regressions and host/CLI tests were observed failing then passing.

## Verification

- Full Release build succeeded using bundled Qt 6.10.3.
- Python: `.venv/bin/python -m unittest discover -s tests` — **92 passed**.
- Shared service executable — **122 checks passed**; panel executable also passes.
- Focused productivity/panel/resident/session/protocol/CLI CTests pass.
- Full combined CTest: **57 passed, nine failures, four skips** (70 total).
- Separate actual-socket CTest outside the sandbox: **resident_ipc and
  resident_ipc_cli both pass**. The latter now exercises publish/list/acknowledge
  and real helper → petctl → offscreen resident delivery for Claude and Codex.
  This is a recorded payload replay, not a live installed-agent turn.
- Panel captures inspected: ignored `results/productivity-2026-10-16/inbox.png`
  and `timers.png`. Layout is readable without clipping in the offscreen fixture;
  test timestamps use an injected epoch clock and are not real notification data.
- `git diff --check` passes. No commits, staging, global hook installation, pet
  selection changes or character asset edits were performed by this work.

The nine full-suite failures are pre-existing catalogue rest-restoration checks:
catalogue3d_rivet, catalogue3d_paper-heron, catalogue3d_acorn-custodian,
catalogue3d_puddle, catalogue3d_pocket-observatory, catalogue3d_night-librarian,
catalogue3d_sprig, catalogue3d_hearthling and catalogue3d_crumb-golem. The initial
baseline also failed character3d_joint_binding (forward motion did not quiesce);
this did not recur after rebuilding, and no productivity-related binding fix is
claimed. Preserve the pre-existing dirty signature/character work.

Full-suite skips: socket bind sandbox denial for the two IPC tests (separately
passed as above), native Spaces and native 3D GPU capture. Logs/captures belong in
ignored results directories; immutable archived evidence was not changed.

## Remaining stages

Verify configured hooks against actual installed Claude/Codex versions and their
desktop/CLI modes, reliable source navigation, real desktop focus and monitor
behavior, and actual battery impact. No universal OS notification listener has
been implemented. Investigate macOS feasibility before promising broad capture;
Windows/Linux adapters also require native implementation and validation.
Cross-source deduplication, provider importance/quiet rules, recurring reminders,
automatic Pomodoro continuation and richer task views remain follow-up stages.
