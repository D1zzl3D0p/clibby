# Session inputs — 2026-10-08

Stage 2 is implemented for software evaluation: a bounded session core, local
JSON-RPC API, `petctl`, session menu/acknowledgment, plain-text status label and
four short reactions. Live transport and native visual acceptance remain open;
this is not completion of the first usable milestone or a battery claim.

## Behavior and boundaries

The [version-1 contract](../api/session-v1.md) and its linked JSON Schema specify
the public messages, errors, limits and permissions. The Qt-free core stores at
most 32 records, including clear tombstones. Sequence numbers, TTL, priority,
acknowledgment, three-second reactions and five-second cooldown are deterministic.
There is no reaction backlog or persistent notification history.

One application deadline combines animation, expiry and reaction completion.
Hidden/paused pets have no animation ticks; an empty suppressed runtime has no
periodic application timer. Pause freezes the label and pose while data/menu
updates continue. Resume synchronizes status without replaying a gesture. Sleep
and wake clear session records, history, acknowledgment and cooldown.

Status text uses a separate input-transparent, nonactivating label window, leaving
the pet's drag geometry unchanged. Motion consumes typed behavior intents and
changes local pose only. The existing renderer, benchmark CLI and frozen motion
equations are preserved. Native label appearance, focus and click-through still
require desktop testing.

The Unix endpoint uses an owner-private directory and same-user peer credentials.
No RPC can execute code, load files, quit, or change user pause/visibility. The CLI
is a short-lived Qt Core/Network executable. The example is explicitly simulated;
there is no real coding-tool adapter or new character-package loader.

## Verification

| Check | Observed result |
| --- | --- |
| Release build | `pet_qt`, `better_pets`, `petctl` and tests built |
| CTest | 9 targets passed; 2 live socket targets explicitly skipped after OS bind returned EPERM |
| Session/runtime policy | 8 session groups and 12 runtime groups pass; independent strict-warning and sanitizer checks also pass |
| Resident composition | 15 groups pass offscreen, including paused text, expiry, acknowledgment, sleep reset, bounded distinct poses and existing lifecycle checks |
| Protocol policy | Handshake, envelope/types, invalid UTF-8, allowlist, frame limit, rate limit, dispatch, directory validation and peer helpers pass without binding a listener |
| CLI arguments | Help, malformed/invalid options and values, and missing-resident failure exit code pass |
| Existing resident process suite | 3 cases pass; 2 launch/restart cases skip after the same direct OS bind probe |
| Python regression suite | 37/37 pass on the final run |
| Fresh pose export | All 44 poses match within 0.0001 |
| Preserved fixture/evidence/archive | All 203 recorded hashes unchanged |

Independent core and shell reviews found no must-fix issues. Final review caught
a trailing-newline validation defect in CLI/schema anchors; it was reproduced,
fixed with regression cases, and passed focused re-review.

The first concurrent Python run had two short-window profiler failures. A direct
process-metric probe succeeded and the later full run passed without code or test
changes. This is recorded as timing-sensitive test behavior, not evidence of a
session-feature regression or a proven permission cause.

The source snapshot, focused reports and review records are in ignored
`.superpowers/session-inputs/`. Fresh poses are in
`results/session-inputs-poses-378k50yt/poses.json`. No Git commits are claimed: this
session's filesystem rules deny `.git` writes. That is an execution-environment
restriction, unrelated to organizational workspace policies.

## Outstanding acceptance and next step

The user reported generally good native stage-1 behavior with cross-monitor
snapping/alternation. It is documented in the
[foundation record](2026-10-08-resident-foundation.md). Code triage supports a
localized host/native placement issue; the exact cause is unverified. Stage 2
does not claim to fix it. Trace and fix it before closing native drag acceptance.

From a normal Terminal in this directory:

```sh
rtk proxy .venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
rtk proxy build/qt/better_pets
```

In a second Terminal, use a visibly running, unpaused pet:

```sh
rtk proxy .venv/bin/python examples/simulated_sessions.py --petctl build/qt/petctl
rtk proxy build/qt/petctl state
```

If using `--state-dir`, pass the same absolute path to both resident and example.
Run all CTests without socket skips. Actual framing timeouts, floods, slow readers,
live endpoint protection, CLI round trips and cross-user rejection remain
unverified here. Exercise two sources, expiry, acknowledgment, pause/hide,
reconnect, actual sleep/wake, label focus/click-through and monitor behavior.
Add dedicated slow-reader, 16-client capacity and global 64-request queue tests
before closing transport acceptance; those paths currently have explicit code
bounds and review, but no dedicated automated transport cases.

Then proceed to stage 3: native lifecycle acceptance, quiet CPU screening and the
eight-hour memory soak. Whole-system energy/battery testing is still separate.
