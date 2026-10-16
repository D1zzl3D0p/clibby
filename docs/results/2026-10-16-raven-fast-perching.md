# Quicker raven perching and idle window behavior

Status: implemented, rebuilt and verified.

Window transfers previously approached the destination with weak horizontal
steering, then waited for an almost exact hover point before descending.
The measured nearby lower-window transfer took 7.73 seconds in 2D and 6.37
seconds in 3D including recovery.

The shared wing pilot now steers and brakes more responsively. It holds the
requested facing through arrival rather than flipping on a small overshoot.
Banked flight requests additional wing effort for the actual tilted lift vector,
so quicker steering retains the requested height. A sustained-bank regression
reproduced 88 units against a target of 100 before compensation, and 99.8 after.
A perch approach starts descending once the current and projected half-second
foot sweep fit inside one visible segment, with a small safety margin. It still
requires vertical clearance and a controlled vertical speed, fresh window
geometry, actual descending contact and supported landing recovery. The bird
never teleports onto a window.

Raven idle behavior now attempts a usable window after about eight quiet
seconds. Temporary manual motion, reactions or airborne activity defer a due
attempt by two seconds instead of discarding it for a long interval. Touchdown
does not restart the pending cooldown. A raven already on a window stays there;
no available window causes a quiet 30-second retry rather than a fallback hop.
Pause, hide, suspension, dragging and notifications retain priority. Sampling
and deadlines reuse the resident timer.

The stricter touchdown checks exposed a fractional-placement bug: a predicted
continuous step could end just before an edge while native integer placement
crossed it. Landing sweeps now use the host's quantized predicted position,
including accumulated fractions and the same clamps as realized movement.
This preserves one-way acquisition while preventing thin edges being skipped.

Final deterministic nearby-window timings:

| Version | Touchdown | Settled after recovery | Previous settled time |
|---|---:|---:|---:|
| 2D | 2.30 s | 4.25 s | 7.73 s |
| 3D | 1.88 s | 3.82 s | 6.37 s |

Straight nearby climbs touch down in 1.52 s (2D) and 1.47 s (3D).
Actual descending acquisition is asserted immediately at the requested edge;
a later idle retry cannot conceal a missed first landing.

Native Cocoa higher-window transfers, including recovery, pass in 4.42 s (2D)
and 3.87 s (3D). Native forward flight retains 98/101 logical pixels of height
against the 100-pixel target. Both native runs also cover high dragging and Land.
Full combined Release suite: 39 passed, four environment skips, zero failures;
2D-only: 28 passed, three skips, zero failures. New idle tests run in both versions
and cover settled manual input, autonomous window acquisition, perched dwell,
quiet no-window retry, pause/resume, hide, drag and notifications. Fractional
host tests cover the rounded crossing, stationary contact, moving supports,
manual selection, rebasing and display repair. Existing core/drop/flight/host
regressions remain green. The app is rebuilt at build/qt/better_pets.

Independent review also tightened projected foot clearance to one visible
segment rather than allowing the current and future feet on separate segments.
Per-frame motion still comes from the shared procedural flight system; idle
window observation and retry deadlines reuse the existing timer.
Raw evidence: ignored results/raven-fast-perch-2026-10-16/.
