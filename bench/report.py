"""Summarize complete paired blocks without making an energy inference."""
from __future__ import annotations

from collections import defaultdict
import random
import statistics
import math

METRICS = ("process_cpu_percent", "rss_bytes", "root_cpu_percent", "root_rss_bytes", "windowserver_cpu_percent", "system_cpu_percent")


def _value(run, key):
    item = run.get("metrics", {}).get(key, {})
    value = item.get("value")
    coverage = item.get("coverage_fraction")
    if not isinstance(value, (float, int)) or not math.isfinite(value):
        return None
    if not isinstance(coverage, (float, int)) or not math.isfinite(coverage) or coverage < 0.9:
        return None
    return value


def _exclusions(runs, key):
    result = []
    for run in runs:
        if _value(run, key) is None:
            item = run.get("metrics", {}).get(key, {})
            reason = item.get("reason") or "missing/nonfinite value or coverage below required 0.9"
            result.append({"run_id": run.get("run_id"), "candidate": run["candidate"], "coverage_fraction": item.get("coverage_fraction"), "reason": reason})
    return result


def _interval(values, seed):
    if len(values) < 6:
        return None, "at least six independent valid paired blocks required"
    rng = random.Random(seed)
    means = sorted(statistics.mean(rng.choices(values, k=len(values))) for _ in range(5000))
    return [means[124], means[4874]], None


def summarize_runs(runs, seed=0, environment=None):
    candidates = [run for run in runs if run["kind"] == "candidate"]
    validation_reasons = []
    declared_purpose = str((environment or {}).get("purpose", ""))
    if "validation" in declared_purpose.lower():
        validation_reasons.append(f"Configured purpose: {declared_purpose}")
    for run in candidates:
        engine = run.get("engine_summary") or {}
        if engine.get("graphical_runtime") is False or str(engine.get("platform", "")).lower() in {"offscreen", "minimal", "headless"} or str(engine.get("display_driver", "")).lower() == "headless":
            validation_reasons.append(f"{run['candidate']}: headless/offscreen engine runtime")
    purpose = "validation_only" if validation_reasons else "resource_screening"
    groups = defaultdict(list)
    pairs = defaultdict(lambda: defaultdict(dict))
    for run in candidates:
        groups[(run["candidate"], run["scenario"])].append(run)
        if "pair" in run:
            pairs[(tuple(run["pair"]), run["scenario"])][run["block_id"]][run["candidate"]] = run
    mode = "profiling" if len({run["candidate"] for run in candidates}) == 1 and not pairs else "paired_comparison"
    summaries = []
    for (candidate, scenario), group in sorted(groups.items()):
        valid = [run for run in group if run["valid"]]
        metrics = {}
        for key in METRICS:
            values = [_value(run, key) for run in valid if _value(run, key) is not None]
            exclusions = _exclusions(valid, key)
            metrics[key] = {"mean": statistics.mean(values) if values else None, "valid_runs": len(values), "excluded_runs": len(exclusions), "exclusions": exclusions, "required_coverage_fraction": 0.9, "coverage_fractions": [run.get("metrics", {}).get(key, {}).get("coverage_fraction") for run in valid], "reason": None if values else "no valid observations with required coverage"}
        summaries.append({"candidate": candidate, "scenario": scenario, "attempted_runs": len(group), "valid_runs": len(valid), "comparison_eligible_runs": sum(run.get("comparison_eligible", False) for run in valid), "metrics": metrics, "engine_telemetry": [run.get("engine_summary") for run in group], "failures": [run.get("invalid_reasons", []) for run in group if not run["valid"]]})
    paired = []
    for (pair, scenario), blocks in sorted(pairs.items()):
        complete = [block for block in blocks.values() if all(name in block and block[name]["valid"] for name in pair)]
        metrics = {}
        for key in METRICS:
            values = [_value(block[pair[1]], key) - _value(block[pair[0]], key) for block in complete if all(_value(block[name], key) is not None for name in pair)]
            ci, reason = _interval(values, seed)
            metrics[key] = {"mean_b_minus_a": statistics.mean(values) if values else None, "valid_blocks": len(values), "excluded_blocks": len(complete) - len(values), "exclusions": _exclusions([run for block in complete for run in block.values()], key), "required_coverage_fraction": 0.9, "ci95": ci, "ci_reason": reason, "block_differences": values}
        paired.append({"pair": list(pair), "scenario": scenario, "valid_blocks": len(complete), "metrics": metrics})
    return {"schema_version": 1, "mode": mode, "purpose": purpose, "purpose_reasons": sorted(set(validation_reasons)), "desktop_comparison": False, "battery_acceptance": "unverified", "battery_reason": "No validated whole-system battery energy/discharge measurements; resource usage cannot establish battery-runtime loss.", "toolkit_winner": None, "selection_reason": "Validation only; headless/offscreen or explicitly declared validation metrics are not a desktop performance comparison." if validation_reasons else "Resource screening only; visual, interaction and displayed-cadence equivalence require separate verification.", "attempted_candidate_runs": len(candidates), "valid_candidate_runs": sum(run["valid"] for run in candidates), "baseline_runs": sum(run["kind"] == "baseline" for run in runs), "candidates": summaries, "paired_deltas": paired, "uncertainty_method": "descriptive single-candidate resource summaries; no confidence intervals" if mode == "profiling" else "percentile bootstrap of whole paired blocks; 5000 resamples; minimum 6 blocks; screening interval only", "limitations": ["Normal desktop environment is uncontrolled unless separately documented.", "Root-only metrics exclude descendants and are provisional diagnostics.", "RSS sums may double-count shared memory; footprint unavailable.", "Application callbacks do not prove displayed frame delivery.", "Resource profiling does not establish battery impact or confirmatory energy significance." if mode == "profiling" else "Three paired repetitions do not establish confirmatory energy significance."]}


def markdown(report):
    validation = report.get("purpose") == "validation_only"
    profiling = report.get("mode") == "profiling"
    title = "# Resource profiling report" if profiling else "# Resource screening report"
    lines = ["# Validation pipeline report" if validation else title, ""]
    if validation:
        label = "headless/offscreen, " if any("headless/offscreen" in reason for reason in report["purpose_reasons"]) else ""
        lines.extend([f"**VALIDATION ONLY — {label}not a desktop performance comparison.**", "", "Operationally valid runs only demonstrate that the collection pipeline completed. All metrics and paired differences below are noncomparable validation diagnostics.", ""])
    lines.extend(["Battery acceptance: **unverified**. " + report["battery_reason"], "", f"Valid candidate runs: {report['valid_candidate_runs']}/{report['attempted_candidate_runs']}. No performance winner is selected.", "", "| Candidate | Scenario | Valid / attempted | Tree CPU % | Tree RSS MiB | Root CPU % | Root RSS MiB |", "|---|---|---:|---:|---:|---:|---:|"])
    def display(value, divisor=1):
        return "unavailable" if value is None else f"{value / divisor:.3f}"
    for candidate in report["candidates"]:
        values = candidate["metrics"]
        lines.append(f"| {candidate['candidate']} | {candidate['scenario']} | {candidate['valid_runs']} / {candidate['attempted_runs']} | {display(values['process_cpu_percent']['mean'])} | {display(values['rss_bytes']['mean'], 1024**2)} | {display(values['root_cpu_percent']['mean'])} | {display(values['root_rss_bytes']['mean'], 1024**2)} |")
    lines.extend(["", "CPU: 100% is one logical core. Root-only measurements are incomplete process-tree diagnostics.", ""])
    if profiling:
        lines.extend(["Single-candidate profile; no paired comparison was run.", ""])
    else:
        lines.extend(["## Paired differences", "", "Differences are candidate B minus candidate A; negative numbers are preserved.", ""])
    for pair in report["paired_deltas"]:
        lines.append(f"- {pair['scenario']}, {pair['pair'][1]} minus {pair['pair'][0]}: {pair['valid_blocks']} valid paired blocks.")
        for key, value in pair["metrics"].items():
            if value["mean_b_minus_a"] is not None:
                ci = value["ci95"] if value["ci95"] is not None else value["ci_reason"]
                lines.append(f"  - {key}: {display(value['mean_b_minus_a'])}; 95% interval: {ci}.")
    lines.extend(["", "## Limitations", ""] + [f"- {limitation}" for limitation in report["limitations"]])
    lines.extend(["", "Engine cadence and exact event labels are retained in summary.json and each engine-summary.json. Raw process samples and baseline observations are in runs/*/samples.json. Power, wakeups and physical footprint remain null with reasons.", ""])
    return "\n".join(lines)
