"""CLI for single-app resource profiling and paired build comparisons."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import itertools
import json
import math
import os
from pathlib import Path
import platform
import random
import re
import sys
import time

from .metrics import collect_baseline, collect_process, psutil
from .report import markdown, summarize_runs

SCENARIOS = {"static", "idle-motion", "walk-fixed", "walk-desktop", "hidden", "mixed-active", "mixed-rest"}


def read_config(path):
    config = json.loads(path.read_text())
    if not isinstance(config, dict) or config.get("schema_version") != 1:
        raise ValueError("config schema_version must be 1")
    scenarios = config.get("scenarios")
    if not isinstance(scenarios, list) or not scenarios or not all(isinstance(s, str) and s in SCENARIOS for s in scenarios) or len(set(scenarios)) != len(scenarios):
        raise ValueError("scenarios must be distinct supported scenario names")
    candidates = config.get("candidates")
    if not isinstance(candidates, list) or not candidates:
        raise ValueError("at least one candidate required")
    names = set()
    for candidate in candidates:
        if not isinstance(candidate, dict):
            raise ValueError("each candidate must be an object")
        name = candidate.get("name", "")
        if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]*", name) or name in names:
            raise ValueError("candidate names must be unique simple filename identifiers")
        names.add(name)
        argv = candidate.get("argv")
        if not isinstance(argv, list) or not argv or not all(isinstance(arg, str) and arg and "\0" not in arg for arg in argv):
            raise ValueError("candidate argv must be a nonempty string array")
        if not isinstance(candidate.get("renderer"), str):
            raise ValueError("candidate renderer label required")
        environment = candidate.get("environment", {})
        if not isinstance(environment, dict) or not all(isinstance(k, str) and isinstance(v, str) for k, v in environment.items()):
            raise ValueError("candidate environment must map strings to strings")
    if not isinstance(config.get("assets"), str):
        raise ValueError("assets path required")
    assets = (path.parent / config["assets"]).resolve()
    if not assets.is_dir():
        raise ValueError(f"assets directory not found: {assets}")
    config["assets"] = str(assets)
    if "seed" in config and (not isinstance(config["seed"], int) or isinstance(config["seed"], bool)):
        raise ValueError("seed must be an integer")
    return config


def make_blocks(config, repeats, seed):
    rng = random.Random(seed)
    blocks = []
    for scenario in config["scenarios"]:
        if len(config["candidates"]) == 1:
            for _ in range(repeats):
                blocks.append({"block_id": f"block-{len(blocks):04d}", "scenario": scenario, "order": [config["candidates"][0]["name"]]})
            continue
        for pair in itertools.combinations([candidate["name"] for candidate in config["candidates"]], 2):
            offset = rng.randrange(2)
            orders = [list(pair if (repeat + offset) % 2 == 0 else reversed(pair)) for repeat in range(repeats)]
            rng.shuffle(orders)
            for order in orders:
                blocks.append({"block_id": f"block-{len(blocks):04d}", "scenario": scenario, "pair": list(pair), "order": order})
    return blocks


def _write(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def validate_engine_summary(engine, *, scenario, duration, warmup):
    """Accept native field names, but require equivalent workload evidence."""
    if not isinstance(engine, dict) or engine.get("scenario") != scenario:
        raise ValueError("engine telemetry scenario missing/mismatched")
    # Validate optional nested values too, before strict JSON report serialization.
    def finite_tree(value):
        if isinstance(value, float) and not math.isfinite(value):
            raise ValueError("engine telemetry contains nonfinite numeric value")
        if isinstance(value, dict):
            for child in value.values():
                finite_tree(child)
        elif isinstance(value, list):
            for child in value:
                finite_tree(child)
    finite_tree(engine)
    if engine.get("error") or engine.get("errors") or engine.get("error_status"):
        raise ValueError("engine reports error")
    for keys in (("engine",), ("engine_version", "version"), ("frame_event_semantics",)):
        if not any(isinstance(engine.get(key), str) and engine[key] for key in keys):
            raise ValueError(f"required engine metadata missing: {keys[0]}")
    if not isinstance(engine.get("backend"), str) or not engine["backend"]:
        backend_reason = engine.get("backend_reason") or engine.get("backend_unavailable_reason")
        if engine.get("backend") is not None or not isinstance(backend_reason, str) or not backend_reason:
            raise ValueError("backend must be named or null with a reason")
    def number(*keys, integer=False):
        value = next((engine[key] for key in keys if key in engine), None)
        if isinstance(value, bool) or not isinstance(value, (float, int)) or not math.isfinite(value) or value < 0 or (integer and value != int(value)):
            raise ValueError(f"required finite nonnegative {'integer' if integer else 'number'} missing: {keys[0]}")
        return value
    requested = number("duration_seconds", "duration")
    reported_warmup = number("warmup_seconds")
    if not math.isclose(requested, warmup + duration, abs_tol=1e-6, rel_tol=0) or not math.isclose(reported_warmup, warmup, abs_tol=1e-6, rel_tol=0):
        raise ValueError("engine duration/warmup differ from requested workload")
    elapsed = number("elapsed_seconds")
    measured = number("measurement_elapsed_seconds", "measurement_seconds")
    tolerance = 0.05  # Explicit timer/poll scheduling allowance, not exact alignment.
    if abs(elapsed - requested) > tolerance or abs(measured - duration) > tolerance or abs((elapsed - measured) - warmup) > tolerance:
        raise ValueError("engine actual warmup/measurement duration outside 50ms timing allowance")
    if engine.get("measurement_clock_reset", True) is not True:
        raise ValueError("engine did not reset measurement clock")
    for keys in (("pose_evaluations",), ("window_position_writes",), ("dropped_steps",), ("simulation_overruns", "overruns"), ("frame_swapped_events", "engine_frame_post_draw_events")):
        number(*keys, integer=True)
    intervals = engine.get("frame_event_intervals_seconds")
    if not isinstance(intervals, list) or not all(isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and value >= 0 for value in intervals):
        raise ValueError("frame event intervals must be a finite nonnegative array")
    return engine


def run(config, output, *, warmup, duration, repeats, interval, baseline_duration, cooldown, exit_margin, startup_timeout=30.0):
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    seed = config.get("seed", random.SystemRandom().randrange(2**32))
    blocks = make_blocks(config, repeats, seed)
    settings = dict(warmup=warmup, duration=duration, repeats=repeats, interval=interval, baseline_duration=baseline_duration, cooldown=cooldown, exit_margin=exit_margin, startup_timeout=startup_timeout, readiness_poll_interval=0.005, required_metric_coverage=0.9)
    manifest = {"schema_version": 1, "created_utc": datetime.now(timezone.utc).isoformat(), "seed": seed, "blocks": blocks, "config": config, "settings": settings, "host": {"platform": platform.platform(), "machine": platform.machine(), "logical_cpus": os.cpu_count(), "python": sys.version, "psutil": psutil.__version__ if psutil else None}, "environment": {"control_status": "uncontrolled; no settings changed or user apps closed", "power_source": "unverified", "display": "unverified", "thermal_recovery": "unverified; cooldown is a delay only", **config.get("environment", {})}, "mixed_complete_cycles": math.isclose(duration % 60, 0, abs_tol=1e-6) and duration >= 60, "status": "running"}
    manifest["mixed_cycles_requested"] = manifest.pop("mixed_complete_cycles")
    manifest["exact_phase_alignment"] = False
    _write(output / "manifest.json", manifest)
    candidates = {candidate["name"]: candidate for candidate in config["candidates"]}
    runs = []
    try:
        for block in blocks:
            for position, name in enumerate([None, *block["order"], None]):
                run_id = f"{len(runs):04d}-{block['scenario']}-{name or 'baseline'}"
                directory = output / "runs" / run_id
                directory.mkdir(parents=True)
                print(f"{run_id}: starting", flush=True)
                if name is None:
                    result = collect_baseline(duration=baseline_duration, interval=interval)
                    result["baseline_position"] = "before" if position == 0 else "after"
                else:
                    candidate = candidates[name]
                    engine_path = directory / "engine-summary.json"
                    ready_path = directory / "ready.json"
                    argv = candidate["argv"] + ["--scenario", block["scenario"], "--duration", str(warmup + duration), "--warmup", str(warmup), "--output", str(engine_path), "--assets", config["assets"], "--ready-file", str(ready_path)]
                    try:
                        result = collect_process(argv, duration=duration, warmup=warmup, interval=interval, exit_margin=exit_margin, output_dir=directory, environment=candidate.get("environment"), ready_file=ready_path, startup_timeout=startup_timeout)
                    except OSError as exc:
                        result = {"kind": "candidate", "valid": False, "invalid_reasons": [f"launch_failed: {exc}"], "samples": [], "metrics": {}, "exit_code": None}
                    result["candidate"] = name
                    result["argv"] = argv
                    result["renderer"] = candidate["renderer"]
                    try:
                        engine = json.loads(engine_path.read_text())
                        validate_engine_summary(engine, scenario=block["scenario"], duration=duration, warmup=warmup)
                        result["engine_summary"] = engine
                    except (OSError, ValueError) as exc:
                        result["valid"] = False
                        result["invalid_reasons"].append(f"engine_telemetry_invalid: {exc}")
                        result["engine_summary"] = None
                    result["comparison_eligible"] = False
                    result["comparison_reason"] = "visual, functional and displayed-cadence equivalence not independently established"
                result.update({"run_id": run_id, "block_id": block["block_id"], "scenario": block["scenario"]})
                if "pair" in block:
                    result["pair"] = block["pair"]
                _write(directory / "samples.json", result.pop("samples"))
                _write(directory / "result.json", result)
                runs.append(result)
                print(f"{run_id}: {'valid' if result['valid'] else 'invalid'}", flush=True)
                if cooldown:
                    time.sleep(cooldown)
        manifest["status"] = "complete"
    finally:
        manifest["completed_runs"] = len(runs)
        _write(output / "manifest.json", manifest)
        report = summarize_runs(runs, seed, environment=config.get("environment"))
        _write(output / "summary.json", report)
        (output / "summary.md").write_text(markdown(report))
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--warmup", type=float, default=30)
    parser.add_argument("--duration", type=float, default=120)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--interval", type=float, default=1)
    parser.add_argument("--baseline-duration", type=float)
    parser.add_argument("--cooldown", type=float, default=5)
    parser.add_argument("--exit-margin", type=float, default=5)
    parser.add_argument("--startup-timeout", type=float, default=30)
    args = parser.parse_args(argv)
    try:
        config = read_config(args.config.resolve())
        if args.baseline_duration is None:
            args.baseline_duration = args.duration
        for name in ("duration", "interval", "baseline_duration", "warmup", "cooldown", "exit_margin", "startup_timeout"):
            value = getattr(args, name)
            if not math.isfinite(value) or value < 0 or (name in {"duration", "interval", "baseline_duration", "startup_timeout"} and value == 0):
                raise ValueError(f"invalid {name}: must be finite and {'positive' if name in {'duration', 'interval', 'baseline_duration'} else 'nonnegative'}")
        if args.repeats < 1:
            raise ValueError("repeats must be positive")
        if args.output.exists():
            raise ValueError("output directory already exists; choose a fresh path")
        run(config, args.output, **{key: getattr(args, key) for key in ("warmup", "duration", "repeats", "interval", "baseline_duration", "cooldown", "exit_margin", "startup_timeout")})
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
