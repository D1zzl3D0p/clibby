# Productivity system implementation plan

> **For agentic workers:** Use the subagent-driven-development workflow for the
> independent model and adapter tasks, with the coordinator integrating the host.

**Goal:** Begin the approved productivity direction with a durable inbox, timers,
Pomodoro, a resident panel and tested Claude/Codex hook adapters.

**Architecture:** A Qt Core service owns bounded persistent records and schedules.
Existing IPC and a new Widgets panel call the same service. Python hooks translate
provider payloads and invoke petctl without controlling agent approvals.

**Tech stack:** C++17, Qt Core/Widgets, Python standard library, existing CMake.

**Spec:** ../specs/2026-10-16-productivity-system-design.md

## Global constraints

- Preserve dirty character/signature work; no bulk checkout, staging or reset.
- Work in the existing authorized checkout; no additional approval round.
- Keep existing session API semantics and the resident's one timer.
- Stage 1 does not claim native hook delivery or all-app notification access.
- Shell commands use rtk; Python tests use .venv/bin/python.

## Review focus

- Persistence failure must not produce acknowledged success or lose the old file.
- Sleep/restart must create one due item, not replay missed Pomodoro phases.
- Concurrent hooks must keep sequence numbers ordered and preserve JSON stdout.
- Untrusted source text/URLs must never become markup or executable actions.
- Full inbox and oversized query responses must remain bounded and usable.

## Shared interfaces

New library pet_productivity lives in qt/productivity/. Namespace
pets::productivity. Service(const QString &file) loads local state; error() returns
a QString (empty on success). call(const QString &operation, const QJsonObject
&params, qint64 nowMs) returns QJsonObject and throws Error with public int code
and std::runtime_error::what(). advance(qint64 nowMs) reconciles due events and
persists only changes; nextDeadline() returns std::optional<qint64> epoch ms.

Operations exposed as productivity.OPERATION:
- publish: source_id,event_id,seq,kind,title; optional detail,source_url,correlation_id.
  Result {item: ITEM}; ITEM includes source_id,event_id,seq,kind,title,detail,
  source_url,created_at,updated_at,acknowledged,completed,snoozed_until.
- list: optional query (case insensitive), attention_only boolean, offset and
  limit (default 50, maximum 50); result {items:[ITEM],total:N}. Attention excludes
  working/cancelled, acknowledged, completed and currently snoozed records.
- act: source_id,event_id,seq,action (acknowledge,snooze,complete), optional
  seconds (snooze default 300); result {item:ITEM}. Require matching current seq.
- timer_create: label, seconds; optional mode countdown/pomodoro (default countdown),
  break_seconds=300,long_break_seconds=900,rounds=4. Result {timer:TIMER}.
- timer_act: id,action (pause,resume,cancel,next); result {timer:TIMER}.
- timers: no params; result {timers:[TIMER]}. TIMER fields id,label,mode,phase
  (work,short_break,long_break),state (running,paused,waiting,finished,cancelled),
  remaining_ms,deadline,work_intervals. Due events use source_id=timer and event_id
  equal to timer id, with an increasing internal seq and kind=completed.
- capabilities: no params; describe available local features and OS capture as
  unavailable/unverified. Return error -32601 for unknown operation, -32602 invalid,
  -32003 stale, -32004 capacity, -32010 storage/unavailable. Missing act targets are
  invalid. All responses stay below existing 64 KiB limit via bounded list pages;
  timers listing must similarly fit (32 schedules with bounded label/config).

petctl adds `productivity --method OPERATION --json JSON_OBJECT`; legacy commands
stay intact. Hook adapter invokes `petctl [--state-dir DIR] productivity --method
publish --json JSON_OBJECT` using an argv list, not a shell.

## Task 1: Persistent common model and schedules

Owner: model worker. Files: qt/productivity/service.h, service.cpp,
service_tests.cpp, CMakeLists.txt only. Root CMake registration belongs to coordinator.

- [x] Write and run failing executable tests for restart, stale sequences, local
  action separation, pagination, capacity and corrupt/unwritable storage.
- [x] Implement service and transactional atomic persistence using QSaveFile,
  rollback on commit failure and no overwrite after failed load.
- [x] Add failing injected-time tests and implement countdown/Pomodoro,
  pause/resume/cancel/next, restart, long absence and snooze wake.
- [x] Build/run focused CTest target productivity_service. Expected: pass.

## Task 2: Provider translation and sequence delivery

Owner: adapter worker. Files: tools/pet_agent_hook.py,
tests/test_pet_agent_hook.py, examples/hooks/*, docs/api/agent-hooks.md only.

- [x] Write failing tests for provider mappings, malformed payloads, monotonic
  concurrent sends and safe hook stdout.
- [x] Implement bounded stdin, explicit provider mapping, supported hook config
  examples and serialized file-locked sequence allocation/delivery. Do not install
  hooks or edit user settings. Do not send transcript contents or raw prompts.
- [x] Run .venv/bin/python -m unittest discover -s tests -p test_pet_agent_hook.py.
  Expected: all pass. Preserve real-delivery verification as outstanding.

## Task 3: Resident integration, API, CLI and panel

Owner: coordinator. Files: new qt/productivity/panel.{h,cpp}, panel_tests.cpp;
focused edits qt/resident/application.{h,cpp}, ipc.cpp, petctl.cpp,
qt/CMakeLists.txt; docs/api/productivity-v1.md.

- [x] Add failing protocol/CLI/host tests for productivity dispatch, persistent
  events and scheduling while hidden; run to establish missing functionality.
- [x] Register service library, connect productivity.* requests and capability
  advertisement, preserve response frame limits and existing session paths.
- [x] Add a Productivity panel reachable from tray and Toolbox, plain text search,
  attention/all filter, source/time/kind, actions, task capture and timer controls.
- [x] Merge service deadline into the existing timer with wall-to-monotonic delay
  conversion, advance on wake and report storage errors without busy retries.
- [x] Test panel with a temporary real service and actions. Run focused tests,
  full Qt CTest and full Python unittest suite. Record skips/failures accurately.

## Task 4: Review and handoff

- [x] Independent review of new files and focused integration changes; fix
  important findings with regression tests.
- [x] Update docs/handoff.md and result record with implemented scope, exact
  commands, verification counts and native/OS capture limits. Preserve all user work.

## Execution record

All four tasks delivered; see docs/results/2026-10-16-productivity-foundation.md.
Independent ownership: model, adapters, panel; coordinator integrated host/API/CLI.
Ruling: worked in place to preserve current uncommitted character work and use
its current interfaces; no additional worktree or global hook installation.
Ruling: optional occurrence_id added after review to distinguish genuinely new
same-kind approvals; internal session attentionId preserves legacy API behavior.
Native installed-agent delivery and OS capture remain delivery stages 2–3.
