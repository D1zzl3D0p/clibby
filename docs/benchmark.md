# Benchmark and motion validation

The preserved toolkit fixture and CPU/RSS profiling workflow.

Read the [toolkit findings](results/2026-10-08-comparison.md) for the original
Qt direction and the limits of the native resource comparison.

Run commands from the repository root after following the [setup instructions](../README.md#setup-and-test).

## Try the benchmark

Run from a normal Terminal with desktop access, in this project directory:

```sh
build/qt/pet_qt --scenario idle-motion --duration 60 \
  --assets "$PWD/shared" --output /tmp/clibby-preview.json
```

Drag with the left mouse button; right-click to quit. The prototype exits after
the requested duration. `walk-desktop` deliberately moves its window;
`idle-motion` animates in place unless dragged. `hidden` starts hidden and exits
on its timer; it is not a visible-to-hidden lifecycle test or tray application.

For an untimed offscreen validation on a host without desktop access:

```sh
env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  build/qt/pet_qt --scenario static --duration 1 \
  --assets "$PWD/shared" --output /tmp/clibby-validation.json \
  --screenshot /tmp/clibby.png
```

Offscreen validation does not measure desktop performance.

## Profile resource use

```sh
.venv/bin/python tools/configure.py --scenarios static idle-motion walk-desktop hidden
.venv/bin/python -m bench.run \
  --config bench/config.local.json --output results/qt-screening \
  --warmup 5 --duration 20 --baseline-duration 5 --cooldown 1 --repeats 3
```

Use a new output directory for each experiment. Leave the pet untouched during
timing. The configuration defaults to one Qt candidate with no-pet baselines;
the [profiler guide](../bench/README.md) explains multi-build comparisons, scenarios,
and interpretation. `tools/configure.py --validation-only` generates an offscreen
pipeline check, never a desktop benchmark.

Inspect valid-run counts and errors in the generated summary. A successful
profiler exit means the report was written, not that every application trial passed.

CPU percentages use 100% for one logical core. RSS is not physical footprint.
Neither metric proves battery-runtime loss. The proposed under-5% battery target
still requires controlled whole-system energy measurements.

## Verify motion

```sh
build/qt/pet_qt --scenario static --duration 1 \
  --assets "$PWD/shared" --output /tmp/clibby-unused.json \
  --dump-poses /tmp/clibby-poses.json
.venv/bin/python tools/verify.py --actual /tmp/clibby-poses.json
```

The [fixture contract](../shared/CONTRACT.md) defines coordinates, motion equations,
pause/resume rules and pose tolerance. [The fixture preview](../shared/preview.png)
is simple authored geometry, not the intended final character design.
