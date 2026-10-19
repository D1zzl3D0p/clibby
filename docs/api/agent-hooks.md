# Agent lifecycle adapters

`tools/pet_agent_hook.py` translates documented Claude Code and Codex hook JSON
into the common productivity API. It requires Python 3 and a running Clibby
resident plus `petctl`. It does not install hooks or change agent settings.

## Configure explicitly

The checked-in examples are templates, not active configuration:

- `examples/hooks/claude.settings.json`: merge selected entries into a Claude
  project `.claude/settings.json` or `.claude/settings.local.json`.
- `examples/hooks/codex.hooks.json`: merge selected entries into a trusted Codex
  project `.codex/hooks.json`. Review/trust the definitions using Codex `/hooks`.

Replace `/ABSOLUTE/PATH` entries. Quote script/executable paths containing spaces
inside each JSON command string using escaped double quotes. An absolute Python
path is preferable if the agent's PATH differs from your terminal. Preserve any
existing hooks. No global configuration edits are performed by Clibby.

Both examples use synchronous delivery with a three-second provider timeout.
The adapter bounds its lock wait and petctl delivery to two seconds together;
filesystem operations and stdin reading are additionally bounded by the provider
timeout. Synchronous hooks preserve provider invocation ordering better than
background hooks. They may briefly delay a lifecycle event but never decide an
approval, prevent Stop, or submit another prompt. Failed/malformed/unsupported
observations never become successful work completion. Live delivery is best effort:
there is no retry queue while the resident is unavailable.

Optional arguments:

```text
python3 /path/clibby/tools/pet_agent_hook.py --provider codex \
  --petctl /path/petctl --state-dir /path/resident-state \
  --sequence-dir /path/adapter-sequences
```

`--provider` is required and is `claude` or `codex`. `--petctl` is an executable
path, not a shell command. `--state-dir` is forwarded to petctl; it is not the
adapter directory. `--sequence-dir` overrides the persistent lock/counter directory.
By default it is `~/Library/Application Support/Clibby/agent-hooks` on macOS,
`%LOCALAPPDATA%/Clibby/agent-hooks` on Windows, or
`$XDG_STATE_HOME/clibby/agent-hooks` (fallback `~/.local/state/...`) on Linux.
If the previous Better Pets hook directory already exists, the adapter keeps using
it, including its locks and counters, to preserve sequence ordering across the
rename. `--sequence-dir` remains an explicit override. All invocations targeting
the same resident must share a sequence directory.
Do not delete active counters: restarting allocation can cause stale updates.

For a shell-free replay, provide this JSON as stdin to the command above:

```json
{"session_id":"demo-session","hook_event_name":"PermissionRequest","tool_name":"Bash"}
```

The adapter invokes an argv list equivalent to:

```text
petctl [--state-dir DIR] productivity --method publish --json EVENT_JSON
```

## Translation and semantics

| Provider event | Common kind | Meaning |
| --- | --- | --- |
| Both: UserPromptSubmit | working | A prompt was submitted |
| Both: PermissionRequest | needs_input | An approval is requested; no decision is made |
| Both: PostToolUse | working | Resume after a pending permission tool returns |
| Claude: PostToolUseFailure | working | Resume after a pending permission tool fails; no whole-task failure inferred |
| Both: Stop | response_ready | A response ended; the task may remain unfinished |
| Both: SessionEnd | info | The session ended; no success or cancellation is inferred |
| Claude: StopFailure | failed | The turn ended with a documented failure event |
| Claude: Notification / permission_prompt | needs_input | A permission notification was emitted |
| Claude: Notification / idle_prompt | info | Idle check-in; not proof that the agent is blocked |
| Codex: Interrupt | cancelled | The active turn was interrupted; not a claim about the entire task |

Other events and notification types are ignored. Tool failures do not imply whole
task failures. No polling, silence-to-completion inference or transcript parsing
is used. Identical session/optional `agent_id` pairs share one event within each
provider; subagents with distinct IDs use separate events. A SHA-256 digest keeps
the session identity within the API's 64-character grammar. A file lock serializes
both counter allocation and IPC delivery. Atomic counter replacement precedes
delivery, so failed sends may leave sequence gaps but do not reuse numbers.
Sequence order reflects adapter lock acquisition, not an inferred historical order
for independently concurrent provider events. Counter corruption fails closed.

Tool progress is forwarded only when the last successfully delivered kind was
`needs_input`. When the permission payload has a tool name, only a return from
that same tool clears the pending wait. Ordinary tool traffic does not publish
events. The `needs_input → working → needs_input` transition makes each subsequent
approval request unread again after users acknowledged an earlier request.
Install the progress hooks along with the permission hooks for this behavior.
Every direct `PermissionRequest` also carries an `occurrence_id` allocated from
its locked sequence (`request-N`). A newly observed request therefore becomes
unread even without an intervening tool return, such as after a denied approval.
This reports a new occurrence without fabricating a working or completed state.

Claude `permission_prompt` notifications lack a reliable occurrence identity and
can arrive late. Once a direct `PermissionRequest` was observed in a session,
the adapter suppresses its redundant permission notifications; direct requests
remain authoritative. Notification-only configuration remains a fallback for
sessions without direct requests, but cannot reliably separate repeated approval
occurrences. Sequence files store the last delivered kind and pending tool name
alongside the counter; legacy integer counters upgrade without resetting sequence.

Only fixed titles, kind, hashed identity, source ID, sequence and generated
occurrence identity are forwarded.
The helper never reads transcript files or forwards prompts, assistant text,
tool input/output, cwd or arbitrary provider notifications. No source URL is
invented: reliable source navigation remains an integration follow-up.

Stdin is capped at 64 KiB. Valid command invocations always return zero and print
only `{}` on stdout, including ignored events and delivery failures. This is
neutral JSON for Codex Stop and makes no Claude/Codex permission decision.
Generic diagnostics go to stderr; petctl output is suppressed so it cannot become
hook instructions or leak source contents. Invalid CLI configuration uses normal
argument-parser errors and should be corrected before installing a hook.

## Verification boundary

Run adapter replay, process-concurrency, privacy and malformed-input tests with:

```text
rtk proxy .venv/bin/python -m unittest discover -s tests -p test_pet_agent_hook.py
```

Tests establish translation and ordered delivery through a stand-in petctl, not
native application coverage. Verify real Claude/Codex versions, desktop versus CLI
orchestration, hook trust, permissions, interruptions and resident restarts on the
target host before claiming live support. Codex documents that local command hooks
are unsupported with cloud orchestration, even when execution happens locally.
Cross-platform lock code exists, but Windows execution has not been verified here.
These hooks do not provide OS-wide notification capture.

References checked 2026-10-16: [Claude Code hooks](https://code.claude.com/docs/en/hooks)
and [Codex hooks](https://learn.chatgpt.com/docs/hooks). Configuration and event
availability depend on the installed provider version.
