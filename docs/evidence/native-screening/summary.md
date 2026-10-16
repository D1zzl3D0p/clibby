# Resource screening report

Battery acceptance: **unverified**. No validated whole-system battery energy/discharge measurements; resource usage cannot establish battery-runtime loss.

Valid candidate runs: 24/24. No toolkit winner is selected.

| Candidate | Scenario | Valid / attempted | Tree CPU % | Tree RSS MiB | Root CPU % | Root RSS MiB |
|---|---|---:|---:|---:|---:|---:|
| godot-metal | hidden | 3 / 3 | 0.843 | 276.592 | 0.902 | 276.590 |
| godot-metal | idle-motion | 3 / 3 | 11.096 | 277.056 | 11.087 | 277.056 |
| godot-metal | static | 3 / 3 | 1.241 | 272.689 | 1.237 | 272.688 |
| godot-metal | walk-desktop | 3 / 3 | 16.079 | 278.945 | 16.068 | 278.944 |
| qt-metal | hidden | 3 / 3 | 0.004 | 66.243 | 0.024 | 66.252 |
| qt-metal | idle-motion | 3 / 3 | 9.062 | 80.802 | 9.069 | 80.798 |
| qt-metal | static | 3 / 3 | 0.757 | 80.736 | 0.745 | 80.747 |
| qt-metal | walk-desktop | 3 / 3 | 12.309 | 80.897 | 12.361 | 80.909 |

CPU: 100% is one logical core. Root-only measurements are incomplete process-tree diagnostics.

## Paired differences

Differences are candidate B minus candidate A; negative numbers are preserved.

- hidden, qt-metal minus godot-metal: 3 valid paired blocks.
  - process_cpu_percent: -0.838; 95% interval: at least six independent valid paired blocks required.
  - rss_bytes: -220566839.402; 95% interval: at least six independent valid paired blocks required.
  - root_cpu_percent: -0.878; 95% interval: at least six independent valid paired blocks required.
  - root_rss_bytes: -220555795.241; 95% interval: at least six independent valid paired blocks required.
  - system_cpu_percent: 1.525; 95% interval: at least six independent valid paired blocks required.
- idle-motion, qt-metal minus godot-metal: 3 valid paired blocks.
  - process_cpu_percent: -2.033; 95% interval: at least six independent valid paired blocks required.
  - rss_bytes: -205786613.942; 95% interval: at least six independent valid paired blocks required.
  - root_cpu_percent: -2.018; 95% interval: at least six independent valid paired blocks required.
  - root_rss_bytes: -205791232.733; 95% interval: at least six independent valid paired blocks required.
  - system_cpu_percent: 3.724; 95% interval: at least six independent valid paired blocks required.
- static, qt-metal minus godot-metal: 3 valid paired blocks.
  - process_cpu_percent: -0.484; 95% interval: at least six independent valid paired blocks required.
  - rss_bytes: -201277864.854; 95% interval: at least six independent valid paired blocks required.
  - root_cpu_percent: -0.491; 95% interval: at least six independent valid paired blocks required.
  - root_rss_bytes: -201264617.505; 95% interval: at least six independent valid paired blocks required.
  - system_cpu_percent: -1.003; 95% interval: at least six independent valid paired blocks required.
- walk-desktop, qt-metal minus godot-metal: 3 valid paired blocks.
  - process_cpu_percent: -3.770; 95% interval: at least six independent valid paired blocks required.
  - rss_bytes: -207669085.262; 95% interval: at least six independent valid paired blocks required.
  - root_cpu_percent: -3.707; 95% interval: at least six independent valid paired blocks required.
  - root_rss_bytes: -207654748.842; 95% interval: at least six independent valid paired blocks required.
  - system_cpu_percent: 3.310; 95% interval: at least six independent valid paired blocks required.

## Limitations

- Normal desktop environment is uncontrolled unless separately documented.
- Root-only metrics exclude descendants and are provisional diagnostics.
- RSS sums may double-count shared memory; footprint unavailable.
- Application callbacks do not prove displayed frame delivery.
- Three paired repetitions do not establish confirmatory energy significance.

Engine cadence and exact event labels are retained in summary.json and each engine-summary.json. Raw process samples and baseline observations are in runs/*/samples.json. Power, wakeups and physical footprint remain null with reasons.
