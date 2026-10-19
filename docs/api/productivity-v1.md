# Productivity API, additive session API major 1 extension

The resident accepts `productivity.*` methods through the existing authenticated
local JSON-RPC transport. Run `hello` with `api_major: 1` first; inspect its methods
list to discover support. Existing session.update/clear/state.get behavior remains
unchanged. The CLI uses this same boundary:

```sh
rtk proxy build/qt/petctl productivity --method publish --json '{"source_id":"example","event_id":"review","seq":1,"kind":"needs_input","title":"Review is ready for your input"}'
rtk proxy build/qt/petctl productivity --method list --json '{"attention_only":true}'
rtk proxy build/qt/petctl productivity --method act --json '{"source_id":"example","event_id":"review","seq":1,"action":"acknowledge"}'
rtk proxy build/qt/petctl productivity --method timer_create --json '{"label":"Tea","seconds":180}'
rtk proxy build/qt/petctl productivity --method timer_create --json '{"label":"Review","mode":"pomodoro","seconds":1500,"break_seconds":300,"long_break_seconds":900,"rounds":4}'
rtk proxy build/qt/petctl productivity --method timers --json '{}'
```

Use `--state-dir ABS_DIR` when targeting an alternate resident instance. CLI exit
codes remain 0 success, 2 invalid command, 3 connection failure, 4 RPC rejection.
Payloads are JSON objects, never executable commands.

## Operations

| Suffix | Parameters | Result |
| --- | --- | --- |
| publish | source_id, event_id, seq, kind, title; optional detail, source_url, correlation_id, occurrence_id | item |
| list | optional query, attention_only, offset, limit | items, total |
| act | source_id, event_id, current seq, action; optional seconds for snooze | item |
| timer_create | label, seconds; optional mode, break_seconds, long_break_seconds, rounds | timer |
| timer_act | id, action | timer |
| timers | none | timers |
| capabilities | none | local features and OS notification availability |

Kinds: working, needs_input, response_ready, completed, failed, cancelled, info.
The source_id `timer` is reserved for internal schedules. Source/event/correlation
IDs (including optional occurrence_id) use 1–64 ASCII letters, digits, period, underscore or hyphen. Sequence is an
increasing integer in 0..9007199254740991 for each source/event pair. Title is
nonempty plain text up to 120 Unicode scalars / 480 UTF-8 bytes; detail up to 2048
UTF-8 bytes. Control characters are rejected. Literal angle brackets are text,
not markup. URLs permit only validated absolute http/https and local file URLs;
no credentials, controls or malformed escapes. An omitted URL clears the previous
source action. Providers should always send the current metadata they intend.

Items include created_at/updated_at epoch milliseconds, acknowledged, completed,
and snoozed_until (zero when absent). `act` accepts acknowledge, complete or snooze
(default 300 seconds). Opening a source does not acknowledge it. Completing is
local follow-up completion and does not modify the source task. A new kind clears
local action flags; an unchanged kind preserves them unless a provided occurrence_id
differs. A new occurrence makes a genuinely new same-kind request unread; repeating
the same occurrence preserves local flags. Omission removes occurrence metadata
without itself rearming. Snooze wake clears the
snooze and makes unfinished items unread once. Acknowledged records remain in All.

Queries match title, detail and source case-insensitively. Attention excludes
working/cancelled, acknowledged, locally completed and currently snoozed items.
Default/maximum page size is 50; a byte budget can shorten a page to keep replies
inside the transport limit. Advance offset by the returned items count until
offset >= total. The 512-item limit bounds local history; older acknowledged or
completed items may be evicted. This is not an unlimited audit log.

## Schedules

Mode is countdown (default) or pomodoro. Durations are integer seconds in 1..604800.
Pomodoro defaults are work=1500 (supply seconds explicitly), short break=300, long
break=900, long break after 4 work intervals; rounds range is 1..100. At most 32
schedules; finished/cancelled schedules are reclaimable at capacity.

Timer fields include id, label, mode, phase (work/short_break/long_break), state
(running/paused/waiting/finished/cancelled), remaining_ms, deadline epoch ms and
work_intervals. `timer_act` supports pause, resume, cancel and next. Next is valid
only for a waiting Pomodoro. Work completion increments work_intervals; next
starts the appropriate break, and next after a break starts work. There is no
automatic continuation in this slice. A timer due during sleep/restart creates
one durable item rather than replaying missed intervals. System wall-clock
changes affect deadlines; precise monotonic countdown plus clock-change handling
is a follow-up consideration.

Timers and snoozes share the resident scheduler. Hidden/paused pets do not animate,
but productivity deadlines continue. The panel owns no polling timer; click
Refresh to update remaining durations between phase changes.

## Persistence and errors

`productivity.json` in the resident state directory stores bounded versioned local
state using atomic owner-only writes. A corrupt/unreadable store is preserved and
reported; it is never silently replaced. Failed writes leave the previous model
and file intact. The UI displays failures and scheduling suspends after failed
reconciliation rather than spinning. After repairing storage/capacity, a successful
mutation or explicit panel refresh can retry processing.

Errors: -32601 unknown method, -32602 invalid parameters/target/state, -32003 stale
sequence, -32004 capacity, -32010 unavailable/corrupt/unwritable storage. The normal
transport frame, peer, queue and rate limits still apply. Missing source identities
or source metadata are not inferred from OS notifications.

## Limits of this delivery

Claude/Codex translation and config templates: [agent hooks](agent-hooks.md).
Hook installation and live installed-app verification are separate. Existing
legacy session publishers still use their temporary session semantics; they do
not automatically become durable productivity providers. Adapters should publish
to the productivity boundary for durable events.

All-app OS notification capture is unavailable/unverified. Correlation metadata
is retained, but cross-source merging, provider importance rules, recurring
reminders and richer task management remain later delivery stages. The panel and
model tests do not establish native focus behavior or battery impact.
