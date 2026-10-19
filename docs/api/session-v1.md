# Clibby session API, major 1

This is the generic local session interface for the resident `clibby` app.
The checked-in example sends **simulated inputs**; no coding-tool adapter is
implemented. The machine-readable wire contract is
[session-v1.schema.json](session-v1.schema.json), using JSON Schema 2020-12.

## Connection and framing

Use a local Unix-domain stream socket and send one UTF-8 JSON object followed
by LF per request. The 16,384-byte frame limit excludes LF and includes any CR
or JSON whitespace. Invalid UTF-8 is rejected. Responses are compact JSON objects
followed by LF. Requests require `jsonrpc: "2.0"`, a `method`, and an `id`: a
string of at most 64 Unicode characters or an integer from -9007199254740991 through
9007199254740991. Null IDs, notifications, batches, and positional params are
rejected. Params must be an object when supplied. Clients ignore unknown response
fields; compatible version-1 additions may introduce fields. Incompatible
semantics require a new API major.

Each connection must successfully call `hello` before session or state methods.
A new short-lived `petctl` invocation performs its own handshake. Disconnecting
does not clear session data: TTL is authoritative. Clients should refresh active
sessions before expiry and explicitly clear canceled tasks. Transport failure
never means completion. Do not replay expired or pre-sleep tasks as new work.

Discovery uses `QDir::tempPath()/bp-<effective uid>-<digest>/s`, where `digest` is
the first 16 hexadecimal characters of SHA-256 of the UTF-8 canonical state
directory (or its cleaned absolute path when canonicalization is unavailable).
Create the state directory before discovery when using symlinks. Both programs
default to the same `Clibby` app/organization per-user app-data directory. If
that directory is absent but the previous Better Pets directory exists, both
reuse the legacy directory in place. If both exist, the Clibby directory takes
priority. `--state-dir ABS_DIR` selects an independent instance. The instance lock is
acquired before server construction. Socket paths must be under 104 bytes.

The runtime directory is atomically created with mode 0700 and verified using
`lstat`: directory, current effective owner, exact mode, no symlink. The endpoint
is mode 0600. The server authenticates every peer with `getpeereid` on macOS/BSD
or `SO_PEERCRED` on Linux; the client verifies the directory and server peer.
Unsupported peer verification fails closed. Qt's `UserAccessOption` alone does
not secure macOS sockets; directory privacy and peer checks are required.
[Qt documents this platform difference](https://doc.qt.io/qt-6/qlocalserver.html#socketOptions-prop).
The permission profile trusts same-user processes to publish and query; source
IDs are namespaces, not authenticated application identities.

Existing endpoints are inspected without following symlinks. Only an owned
socket is a cleanup candidate. A successful connection probe preserves the live
endpoint; cleanup proceeds only on connection-refused/server-not-found errors.
Ambiguous errors and non-socket paths abort startup. Startup fails clearly when
the OS denies socket binding; there is no insecure transport fallback.

## Methods and results

Request examples use IDs 1–4; IDs may be chosen independently per connection.

`hello` requires integer `api_major: 1`:

```json
{"jsonrpc":"2.0","id":1,"method":"hello","params":{"api_major":1}}
```

Its result contains `api_major`, `methods` (`hello`, `session.update`,
`session.clear`, `state.get`), `statuses` (`working`, `needs_input`, `completed`,
`failed`), and `limits`. Limits are `frame_bytes: 16384`, `clients: 16`,
`session_records: 32`, `pending_requests: 64`, `tx_bytes: 65536`,
`incomplete_frame_seconds: 5`, `requests_per_second: 10`, `burst: 20`,
`id_characters: 64`, `label_characters: 120`, `ttl_default: 120`, and
`ttl_max: 3600`.

`session.update` requires `source_id`, `session_id`, `label`, `seq`, and `status`.
`ttl` is optional, default 120. Source/session IDs contain 1–64 ASCII letters,
digits, periods, underscores or hyphens. Labels contain at most 120 Unicode
characters, reject control characters, and display as plain text. Empty labels
are permitted. Sequences are integers from 0 through 9007199254740991. TTL is
an integer from 1 through 3600 seconds.

`source_url` is optional action metadata, bounded to 2,048 UTF-8 bytes. Omit it
or send an empty string when no source is available; each accepted update replaces
previous source metadata. Supported URLs are strict absolute `https`/`http` URLs
with a host and local `file:///absolute/path` URLs with no remote authority.
Relative URLs, credentials, unknown schemes, malformed escapes and literal or
percent-encoded control characters are rejected. Source metadata is plain data:
publishing a URL never opens it. The resident opens a source only after an explicit
local Open source action.

```json
{"jsonrpc":"2.0","id":2,"method":"session.update","params":{"source_id":"build","session_id":"example","label":"Compile","seq":1,"status":"working","ttl":120}}
{"jsonrpc":"2.0","id":2,"result":{"seq":1,"ttl":120}}
```

Sequence must strictly increase for the session key. A rejected refresh does
not renew TTL. Retain only the latest state. A status transition clears local
acknowledgment; an unchanged refresh does not restart reactions. Expiry removes
both session and sequence history. Sleep/wake clears all sessions/history.

`session.clear` requires `source_id`, `session_id`, and `seq`:

```json
{"jsonrpc":"2.0","id":3,"method":"session.clear","params":{"source_id":"build","session_id":"example","seq":2}}
{"jsonrpc":"2.0","id":3,"result":{"seq":2,"cleared":true}}
```

A clear retains its accepted sequence until the session's existing expiry,
without extending it. Clear of an unknown key succeeds without allocating a
record and returns `cleared: false`. Cleared records count toward capacity.

`state.get` accepts omitted params or `{}` only:

```json
{"jsonrpc":"2.0","id":4,"method":"state.get"}
```

Its result has `visible` and `paused` booleans, `character` string, `foreground`
(a session object or null), and `sessions` (at most 32 active session objects).
Each session has `source_id`, `session_id`, `label`, `seq`, `status`, numeric
`ttl_remaining` in seconds, boolean `acknowledged`, and optional `source_url` when nonempty. Queries process expiry
before returning a snapshot. No session data is persisted. Local acknowledgment
changes the local acknowledgment flag without changing reported status or TTL.

Foreground priority is unacknowledged `needs_input`, unacknowledged `failed`,
unacknowledged `completed`, then `working`; ties use latest status transition then lexical
session key. Acknowledged input/failure/completed sessions remain listed but are excluded
from foreground. Pause/hide retain data. Requests cannot change pause,
visibility, character, execute code, open a source, or quit the app.

## Errors and limits

Errors use `{"jsonrpc":"2.0","id":<id>,"error":{"code":<integer>,"message":<text>}}`.
The ID is null when no valid request ID can be recovered, including framing and
connection-level errors. Human-readable messages are not a stable contract.

| Code | Meaning |
| --- | --- |
| -32700 | Invalid JSON syntax or UTF-8 |
| -32600 | Invalid envelope, batch/notification/null ID, or incomplete frame timeout |
| -32601 | Unknown method |
| -32602 | Invalid parameters, types, bounds, status, label or IDs |
| -32603 | Internal error, including application shutdown |
| -32001 | Unsupported API major or missing handshake |
| -32002 | Peer permission denied |
| -32003 | Stale/repeated sequence |
| -32004 | Session or connection capacity exceeded |
| -32005 | Per-connection rate or global pending queue exceeded |
| -32006 | Request frame exceeds 16 KiB |

At most 16 clients, 32 session records including pending-expiry clears, and 64
pending requests are retained. Rate limits are ten requests/second with twenty
tokens of initial burst per connection. Each event callback reads at most 16 KiB
and extracts/processes at most eight requests before yielding. Receive buffers
are bounded; partial frames have a five-second deadline from their first byte.
Quiet connections have no periodic polling timer. Oversized and incomplete
frames receive a bounded error then disconnect. Outgoing queued data is capped
at 64 KiB; a slow reader exceeding that cap is disconnected. Connection-level
rejections may arrive as an error followed by EOF.

## CLI and simulated example

`petctl` uses Qt Core/Network and needs no GUI/display initialization. Successful
commands print the compact **result object** to stdout; RPC rejections print the
compact error envelope. Usage/connection diagnostics go to stderr.

```sh
build/qt/petctl --help
build/qt/petctl update --source build --session example --label Compile --seq 1 --status working --ttl 120 --source-url https://example.com/task
build/qt/petctl state
build/qt/petctl clear --source build --session example --seq 2
.venv/bin/python examples/simulated_sessions.py --petctl build/qt/petctl
```

`--state-dir ABS_DIR` is accepted anywhere. Exit codes are 0 success/help, 2
usage or invalid CLI values, 3 connection/timeout/protocol failure, and 4 RPC
rejection. Each connection and response has a 1.5-second CLI deadline; response
input is capped at 64 KiB. Options are `--source`, `--session`, `--label`, `--seq`,
`--status`, `--ttl`, and `--source-url` as appropriate to the command. `state` takes no command
options; `clear` takes source/session/sequence only.

The simulated example uses two sources and all four statuses. It publishes no
real build/editor activity. It supports `--petctl` and `--state-dir` and clears
its two sessions on exit when the resident is reachable.

## Verification status

Protocol, deterministic session/core, credential helpers and CLI argument tests
can run without native binding. Actual-socket and resident/CLI tests require
local Unix bind permission. They return skip code 77 only after an OS probe
specifically returns `EPERM`; other failures fail the tests. The agent execution
sandbox currently denies binding, so actual sockets, framing timeouts/floods,
live endpoint protection, and two-source CLI round trips remain unverified in
this environment. Native desktop and cross-user rejection checks remain pending;
software/offscreen results establish neither desktop parity nor battery impact.
