# Qt resource profiler

Run `rtk proxy .venv/bin/python -m bench.run --config bench/config.local.json --output results/run-name --warmup 30 --duration 120 --repeats 3` with the project's Python 3.12+ environment and `psutil`.

Configuration schema version 1 (paths resolve relative to the config file; executable argv entries are passed literally, so use absolute paths for executable/project arguments):

```json
{
  "schema_version": 1,
  "assets": "../shared",
  "scenarios": ["static", "idle-motion", "walk-desktop", "hidden", "mixed-active"],
  "seed": 20261008,
  "environment": {
    "foreground_workload": "uncontrolled desktop",
    "display": "unverified",
    "power_source": "unverified",
    "functional_equivalence": "unverified"
  },
  "candidates": [
    {"name": "qt-metal", "argv": ["/absolute/path/pet_qt"], "renderer": "Metal", "build_id": "local-release", "environment": {"QSG_RHI_BACKEND": "metal"}}
  ]
}
```

Each candidate receives `--scenario NAME --duration TOTAL_SECONDS --warmup WARMUP_SECONDS --output ABS_SUMMARY_PATH --assets ABS_SHARED_DIR --ready-file ABS_READY_PATH`. Engine duration includes warmup. After initialization, engines atomically publish `{"ready":true,"pid":OWN_PID}` immediately before starting their warmup clocks. The collector waits up to `--startup-timeout` (default 30 seconds), then begins its warmup from the ready observation. Readiness polling requests 5ms intervals; the actual last-absent-to-observed latency bracket is saved. OS scheduling can exceed that interval. Marker publication and native clock startup also have scheduling uncertainty, so `exact_phase_alignment` remains false; requested whole mixed cycles are not falsely labeled exact observed complete cycles. Engines reset scenario phase to zero at the warmup boundary and emit buffered telemetry at exit. No shell is used. Candidate names must be unique simple filename identifiers. Supported scenarios match `shared/CONTRACT.md`. At least one candidate is required.

With one Qt candidate, each scenario and repetition runs a no-pet baseline, the candidate, and another no-pet baseline. Reports label this as resource profiling (`mode: profiling`); `paired_deltas` is empty and no paired confidence interval is calculated.

To compare two Qt builds or configurations, manually add a second candidate with a distinct name, executable argv and build identifier to the same configuration. For each scenario and unordered candidate pair, the collector then runs repeated blocks: no-pet baseline, candidate A, candidate B, no-pet baseline. Within each pair, order is balanced (counts differ by at most one) and shuffled with the recorded seed. Existing paired-result fields and telemetry aliases remain supported. `--baseline-duration` defaults to the measurement duration, `--cooldown` defaults to 5 seconds between runs, and `--interval` defaults to 1 second. The cooldown is a declared delay, not evidence of thermal recovery. `--exit-margin` defaults to 5 seconds. Short durations are allowed for smoke tests; mixed measurements shorter than a full minute or not a whole number of minutes are labeled incomplete cycles. Such smoke runs do not establish a performance comparison.

The manifest saves config, seed, order, host, collector settings, and explicitly uncontrolled environmental conditions. Every run saves raw samples, stdout, stderr, engine telemetry and exit status. Nonzero exits, premature exits, timeouts, missing/malformed engine telemetry, and inaccessible primary process metrics invalidate candidate runs. Metrics unavailable due to permissions or platform limitations are JSON null with reasons. If descendant enumeration is denied, the known root PID is still measured and the result carries `metric_scope: root_only`, `telemetry_complete: false`; this can be a valid process run but cannot qualify for complete process-tree comparison. Process CPU uses cumulative user+system time deltas divided by elapsed monotonic wall time: 100% is one core, independent of machine core count. RSS sums the live process tree and may double-count shared pages. Descendants first seen during collection have their lifetime CPU recorded only from their first observation onward; unobserved short-lived descendants cannot be measured. WindowServer is reported separately and never attributed exclusively to a pet. Baselines have no pet process; baseline pet CPU/RSS are null, not zero. System CPU includes other apps and the collector.

Reports contain valid run counts, resource means and engine cadence telemetry. Multi-candidate runs also contain paired B-minus-A CPU/RSS differences. Confidence intervals bootstrap independent complete paired blocks only when at least six valid blocks exist; they never treat per-second observations as independent replicates. With fewer blocks, intervals are null with a reason. All blocks and baselines remain in raw output. No performance winner is selected: visual/functional equivalence and displayed cadence require separate verification. Power, wakeups and footprint are unavailable in this collector, explicitly null with reasons. Battery acceptance is always **unverified**: process resources and component-power estimates cannot validate whole-system battery-runtime loss.

Each metric's coverage divides its observed measurement intervals by the **requested** duration, including unsampled tails as missing. Primary root metrics require 90% coverage; reports apply that threshold independently to every metric before averaging or pairing and retain exclusions/coverage counts. Exit is detected by a dedicated blocking wait thread, independently of the sampling interval (thread scheduling latency is still possible). A premature exit is invalid even when the next sample would have been after the deadline. Owned process groups are cleaned on every completion path, including successful roots that leave descendants; observed lingering descendants invalidate the run.

Minimum engine telemetry: nonempty `engine`, `engine_version` (legacy alias `version`), scenario, `backend` (or null with nonempty `backend_reason`), `frame_event_semantics`; finite nonnegative `duration_seconds` (legacy `duration`), `warmup_seconds`, `elapsed_seconds`, and `measurement_elapsed_seconds` (legacy `measurement_seconds`); nonnegative integral `pose_evaluations`, `window_position_writes`, `dropped_steps`, `simulation_overruns` (legacy `overruns`), and `frame_swapped_events` (legacy `engine_frame_post_draw_events`); and finite nonnegative `frame_event_intervals_seconds` array. Requested duration and warmup must match within one microsecond; actual elapsed, measured duration and inferred warmup must match within an explicit 50ms timer allowance. A false `measurement_clock_reset`, engine error, missing evidence, or any nonfinite number anywhere in telemetry invalidates that run. Native field names and event meanings are retained without pretending physical presentation was measured.

For example, a short pipeline check uses `--warmup 0.2 --duration 3 --baseline-duration 0.2 --cooldown 0 --interval 0.1 --repeats 1`. Sub-second runs are sensitive to a missing exit sample and can correctly fail the 90% coverage requirement. Do not interpret a smoke run as a performance comparison.

Reports classify runs as `purpose: validation_only` when config `environment.purpose` declares validation or any engine reports `graphical_runtime: false`, an offscreen/minimal/headless platform, or the headless display driver. Markdown prominently labels those results **VALIDATION ONLY**, retains metrics as noncomparable pipeline diagnostics, and states that successful collection is not a desktop performance comparison. A software renderer alone does not imply headless mode. Native resource screening still requires independent equivalence checks; no report assumes those passed. The legacy field `backend_unavailable_reason` is accepted as an explicit alias of `backend_reason`; a missing backend still requires a nonempty reason.
