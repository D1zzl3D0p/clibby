# Toolkit findings — 2026-10-08

## Decision and limits

Proceed with Qt Quick as the presentation and desktop layer for Better Pets.
It is the stronger resource-screening candidate in our prototype and provides a
useful base for the planned modular runtime. This is an engineering direction,
not a demonstrated battery-efficiency win or a claim that Godot cannot be optimized.

Godot is retired from the active source and setup flow. Original source,
configuration, dependency metadata, validation results, plans and reviews are
preserved in the [study archive](../archive/toolkit-study/README.md).

## Native measurements

The user ran the native benchmark outside the agent execution sandbox on an
Apple M4 Pro (12 CPU cores, 16 GPU cores), 24 GB memory, macOS 27.2. There were
three paired repetitions per scenario, five seconds warmup and twenty seconds
measurement. All **24/24 candidate trials completed**, with 24 bracketing no-pet
baselines. Successful collection is distinct from controlled equivalence.

Evidence: [original generated summary](../evidence/native-screening/summary.md),
[manifest](../evidence/native-screening/manifest.json), and per-run telemetry
under `docs/evidence/native-screening/runs/`. The data was moved unchanged;
embedded absolute paths describe the original run location. Archive manifest
hashes retain the original `results/native-screening/` path prefix.

| Scenario | Godot process CPU | Qt process CPU | Godot RSS MiB | Qt RSS MiB |
|---|---:|---:|---:|---:|
| Static | 1.241% | 0.757% | 272.69 | 80.74 |
| Animated idle | 11.096% | 9.062% | 277.06 | 80.80 |
| Desktop walking | 16.079% | 12.309% | 278.95 | 80.90 |
| Starts hidden | 0.843% | 0.004% | 276.59 | 66.24 |

These are means of time-weighted per-run process-tree metrics. **100% CPU means
one logical core**, not the whole machine. RSS is resident memory, not physical
footprint. Every observed process tree contained one process.

Qt used about 71% less visible RSS, 18% less animated-idle application CPU and
23% less walking application CPU. The active CPU differences were about 2.0 and
3.8 percentage points of one core. Qt RSS was lower in every pair; its CPU was
lower in every idle, walking and hidden pair, and two of the three static pairs.

Independent audits recomputed metrics from raw samples and inspected engine
telemetry. Process-tree coverage was 95.276–99.996%; missing terminal samples
were excluded, not treated as zero. Godot used Mobile/Metal; Qt visible runs used
Metal. Qt hidden runs never initialized the scene graph and correctly report a
null backend.

## User observations and comparison qualifications

The user reported that both pets looked broadly equivalent, with Godot slightly
fuzzier and Qt sharper. This is a visual observation, not proof that either
renderer intrinsically has better image quality. Pixel scaling and filtering
were not independently established as equivalent.

The user confirmed dragging Qt during the benchmark and being unable to drag
Godot. Qt has a drag handler; the Godot prototype did not. This explains position
writes in stationary Qt trials: 70/110 writes in static runs `0001`/`0006`, and 43
in idle run `0022`. Qt walking run `0030` recorded 583 clipped placement attempts;
the precise cause of each clipped attempt was not logged. These trials were not
untouched controlled runs.

The user described Godot's hidden scenario as leaving an empty transparent
background, and noticed Qt moving its window by default. We have not separately
verified the hidden-window issue. Qt's `walk-desktop` scenario intentionally moves
the window; `idle-motion` should only move when dragged. Future functional checks
should distinguish each scenario and actual visibility.

Other limits:

- Both simulations advanced near 60 Hz with zero reported dropped steps or
  overruns. Godot walking reported about 52 draw-completion events/second; Qt
  reported about 60 presentation-queue events/second. These instrumentation
  points differ and neither proves physically displayed frames. Walking interval
  p95 was about 33 ms for Godot and 17.4 ms for Qt.
- Godot rounded placement at device-pixel resolution and Qt at logical-pixel
  resolution. Starting positions, movement writes and drag support differed.
- Both reported a 256×256 logical surface at DPR 2; neither timed run supplied
  an independently measured framebuffer size. Qt's final invisible/unexposed
  flags were sampled after the event loop exited.
- The hidden result measures starting hidden. Qt's 66 MiB does not establish
  memory after a previously visible pet becomes hidden.
- WindowServer metrics were unavailable (`AccessDenied`). Whole-system power,
  component power, wakeups and physical footprint were unavailable. Background
  system CPU varied substantially; the power source was not recorded.
- All runs remain `comparison_eligible=false`. Three short repetitions, user
  interaction and uncontrolled background work cannot establish energy significance.

**Battery-runtime impact remains unknown.** The proposed under-5% runtime-loss
target needs controlled whole-system battery measurements. It cannot be inferred
from memory savings or application CPU percentages.

## Software evidence from the study

Qt 6.10.3 Release and both Godot 4.7.2 exports matched the same 44 golden poses
within 0.0001 units. At the end of the original study, 35 Python tests, both Qt
CTest targets (including six CLI lifecycle cases), and eight CLI tests per Godot
release passed. These are historical counts; the [handoff](../handoff.md) records
current verification after cleanup.

The Qt [offscreen screenshot](../../qt/evidence/qt-offscreen-smoke.png) was inspected.
It validates rendering software, not native Metal performance. The initial agent
environment could not create native windows; the user's later Terminal run
provided the native evidence above. Headless/offscreen results were never used
as desktop resource measurements.

Review fixes included complete hit-region containment, deadline handling after
process suspension, startup readiness, requested-duration sample coverage,
process-group cleanup, strict telemetry validation and explicit missing metrics.
The archived reviews explain these fixes. Preserve these safeguards when changing
the remaining Qt prototype and profiler.

## Next work

Use the [modular architecture](../architecture.md) as the product direction.
Retain the Qt fixture and resource profiler as regression tools while extracting
the engine-independent core. Verify focus, input regions, scaling, hide/restore,
multi-monitor movement and native presentation before making broader portability
or efficiency claims. Repeat future timing without touching the pet.

Future profiling commands live in the [current README](../../README.md) and
[profiler guide](../../bench/README.md). The old Godot comparison setup is archival;
reviving it is not a prerequisite for continuing the Qt product.
