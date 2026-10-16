# Stage 2: useful session notifications

Implement the approved [resident companion specification](../specs/2026-10-08-resident-companion-design.md), stage 2 only.

The user authorized proceeding after reporting monitor-crossing snapping. Triage
and record that native defect separately; notification inputs must never move the
pet. Native dragging acceptance remains open. This project is personal work.

## Constraints

Preserve the benchmark, frozen shared fixture and archived evidence. Keep the
session core Qt-free, with bounded state and injected monotonic time. Keep one
resident process, local same-user IPC, no real coding-tool adapter yet. Preserve
pause/hide/drag precedence and idle scheduling. No battery claims from tests.

Git metadata creation is restricted by this execution environment. Work in the
authorized directory and use an ignored source snapshot for review; do not imply
commits exist or bypass that restriction.

## Tasks

- [x] 1. Session core: typed updates, sequence/TTL validation, tombstones,
  arbitration, acknowledgment, suppression, cooldown and reaction deadlines.
  Extend the existing runtime scheduler with bounded reaction animation. Add
  deterministic tests covering limits, expiry, ties, refresh and sleep reset.
- [x] 2. Local transport: owner-private endpoint, peer authentication, bounded
  newline JSON-RPC server and generic `petctl` client. Publish exact version-1
  schemas, error codes and CLI help; test framing, failures and real CLI flow.
- [x] 3. Resident integration: session menu and acknowledgment, plain-text pet
  status, distinct short gestures, merged expiry/animation deadline, shutdown
  cleanup. Add two-source simulated example and lifecycle integration checks.
- [x] 4. Review and verification: build/CTest, Python regression, golden poses,
  preservation checks and independent review. Update handoff and results with
  native monitor defect and remaining native/resource acceptance.

## Verification strategy

Use fake clocks for policy, offscreen Qt for composition, and real local sockets
for protocol/client tests where permitted. Each test must demonstrate behavior,
not mirror implementation. Native visual quality, focus, mixed-scale monitors,
quiet CPU and soak/battery measurements remain separate acceptance work.

Implementation and software verification are complete. Live socket and native
acceptance remain open in [the results record](../../results/2026-10-08-session-inputs.md);
they are not implied by checked implementation tasks.
