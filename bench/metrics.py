"""Monotonic resource sampling of one process tree at a time."""
from __future__ import annotations

import math
import json
import os
import signal
import subprocess
import sys
import time
import threading
from pathlib import Path

try:
    import psutil
except ImportError:
    psutil = None


def metric(value=None, reason=None):
    return {"value": value, "reason": reason if value is None else None}


def cpu_percent(before, after, elapsed):
    """One core fully occupied is 100%, including on multi-core hosts."""
    if elapsed <= 0 or after < before:
        raise ValueError("CPU delta and elapsed time must be monotonic")
    return 100.0 * (after - before) / elapsed


def _unavailable():
    return {
        "physical_footprint_bytes": metric(reason="physical footprint collection unavailable"),
        "wakeups_per_second": metric(reason="task wakeup collection unavailable"),
        "whole_system_power_w": metric(reason="validated total-system energy telemetry unavailable"),
        "cpu_power_w": metric(reason="component power collection unavailable"),
        "gpu_power_w": metric(reason="component power collection unavailable"),
    }


class Probe:
    def __init__(self, pid=None):
        self.pid = pid
        self.previous = {}
        self.root_previous = None
        self.system_previous = None
        self.window_previous = None
        self.window_pid = None
        self.window_reason = "WindowServer metric only available on macOS"
        if psutil is None:
            self.window_reason = "psutil is not installed"
        elif sys.platform == "darwin":
            self.window_reason = "WindowServer process not visible or permission denied"
            try:
                for process in psutil.process_iter(["name"]):
                    if process.info["name"] == "WindowServer":
                        self.window_pid = process.pid
                        break
            except (psutil.Error, OSError) as exc:
                self.window_reason = f"WindowServer unavailable: {type(exc).__name__}"

    def read(self, elapsed):
        result = _unavailable()
        if psutil is None:
            for name in ("process_cpu_percent", "rss_bytes", "root_cpu_percent", "root_rss_bytes", "system_cpu_percent", "windowserver_cpu_percent", "windowserver_rss_bytes", "process_count"):
                result[name] = metric(reason="psutil is not installed")
            if self.pid is None:
                for name in ("process_cpu_percent", "rss_bytes", "root_cpu_percent", "root_rss_bytes", "process_count"):
                    result[name] = metric(reason="baseline has no pet process")
            return result
        try:
            system = psutil.cpu_times()
            busy = sum(getattr(system, key, 0.0) for key in ("user", "system", "nice", "irq", "softirq", "steal"))
            result["system_cpu_percent"] = metric(cpu_percent(self.system_previous, busy, elapsed)) if self.system_previous is not None else metric(reason="first sample has no CPU delta")
            self.system_previous = busy
        except (psutil.Error, OSError, ValueError) as exc:
            result["system_cpu_percent"] = metric(reason=f"system CPU unavailable: {type(exc).__name__}")
            self.system_previous = None
        if self.pid is None:
            for name in ("process_cpu_percent", "rss_bytes", "root_cpu_percent", "root_rss_bytes", "process_count"):
                result[name] = metric(reason="baseline has no pet process")
        else:
            try:
                root = psutil.Process(self.pid)
                cpu = root.cpu_times()
                current_cpu = cpu.user + cpu.system
                result["root_cpu_percent"] = metric(cpu_percent(self.root_previous, current_cpu, elapsed)) if self.root_previous is not None else metric(reason="first sample has no CPU delta")
                result["root_rss_bytes"] = metric(root.memory_info().rss)
                self.root_previous = current_cpu
            except (psutil.Error, OSError, ValueError) as exc:
                result["root_cpu_percent"] = metric(reason=f"root process unavailable: {type(exc).__name__}")
                result["root_rss_bytes"] = metric(reason=f"root process unavailable: {type(exc).__name__}")
                self.root_previous = None
            try:
                root = psutil.Process(self.pid)
                processes = [root] + root.children(recursive=True)
                current = {}
                rss = 0
                delta = 0.0
                for process in processes:
                    with process.oneshot():
                        key = (process.pid, process.create_time())
                        cpu = process.cpu_times()
                        current[key] = cpu.user + cpu.system
                        rss += process.memory_info().rss
                    if key in self.previous:
                        delta += max(0.0, current[key] - self.previous[key])
                result["process_cpu_percent"] = metric(100.0 * delta / elapsed) if self.previous else metric(reason="first sample has no CPU delta")
                result["rss_bytes"] = metric(rss)
                result["process_count"] = metric(len(current))
                self.previous = current
            except (psutil.Error, OSError) as exc:
                for name in ("process_cpu_percent", "rss_bytes", "process_count"):
                    result[name] = metric(reason=f"process tree unavailable: {type(exc).__name__}")
                self.previous = {}
        try:
            if self.window_pid is None:
                raise LookupError(self.window_reason)
            window = psutil.Process(self.window_pid)
            cpu = window.cpu_times()
            current = cpu.user + cpu.system
            result["windowserver_cpu_percent"] = metric(cpu_percent(self.window_previous, current, elapsed)) if self.window_previous is not None else metric(reason="first sample has no CPU delta")
            result["windowserver_rss_bytes"] = metric(window.memory_info().rss)
            self.window_previous = current
        except (psutil.Error, OSError, LookupError, ValueError) as exc:
            reason = str(exc) if isinstance(exc, LookupError) else f"WindowServer unavailable: {type(exc).__name__}"
            result["windowserver_cpu_percent"] = metric(reason=reason)
            result["windowserver_rss_bytes"] = metric(reason=reason)
            self.window_previous = None
        return result


def _aggregate(samples, duration):
    measured = [sample for sample in samples if sample["phase"] == "measurement"]
    result = {}
    for key in samples[0]["metrics"]:
        available = [sample for sample in measured if sample["metrics"][key]["value"] is not None]
        if available:
            weight = sum(sample["measurement_interval_s"] for sample in available)
            result[key] = metric(sum(sample["metrics"][key]["value"] * sample["measurement_interval_s"] for sample in available) / weight)
            result[key]["coverage_fraction"] = min(1.0, weight / duration)
        else:
            reasons = sorted({sample["metrics"][key]["reason"] for sample in measured or samples if sample["metrics"][key]["reason"]})
            result[key] = metric(reason="; ".join(reasons) or "no measurement samples")
            result[key]["coverage_fraction"] = 0.0
    return result


def _validate_times(duration, warmup, interval, exit_margin):
    for value, name, zero_allowed in ((duration, "duration", False), (warmup, "warmup", True), (interval, "interval", False), (exit_margin, "exit_margin", True)):
        if not math.isfinite(value) or value < 0 or (value == 0 and not zero_allowed):
            raise ValueError(f"{name} must be finite and {'nonnegative' if zero_allowed else 'positive'}")


def _stop(process):
    """Reap the owned child and terminate its private process group."""
    try:
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGTERM)
        else:
            process.terminate()
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=0.5)
    except subprocess.TimeoutExpired:
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        process.wait(timeout=1)
    finally:
        if os.name == "posix":
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass


class ExitWatch:
    """Wait in a dedicated thread; sampling interval cannot delay exit detection."""
    def __init__(self, process):
        self.event = threading.Event()
        self.observed_at = None
        self.cleaned = False
        self.thread = threading.Thread(target=self._wait, args=(process,), daemon=True)
        self.thread.start()

    def _wait(self, process):
        process.wait()
        self.observed_at = time.monotonic()
        self.event.set()


def _group_exists(process):
    if os.name != "posix":
        return False
    try:
        os.killpg(process.pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return None


def _await_ready(process, watch, ready_file, timeout, launched):
    poll = 0.005
    previous = time.monotonic()
    while True:
        if watch.event.is_set():
            return None, {}, "exit_before_ready"
        try:
            payload = json.loads(ready_file.read_text())
            observed = time.monotonic()
            if not isinstance(payload, dict) or payload.get("ready") is not True or payload.get("pid") != process.pid:
                return None, {}, "invalid_readiness"
            return observed, {"method": "atomic ready-file observation", "poll_interval_s": poll, "last_absent_monotonic_s": previous, "ready_observed_monotonic_s": observed, "observation_latency_bound_s": observed - previous, "exact_phase_alignment": False, "reason": "engine publishes readiness immediately before its clock starts; polling and native scheduling introduce origin uncertainty"}, None
        except FileNotFoundError:
            previous = time.monotonic()
        except (OSError, ValueError):
            return None, {}, "invalid_readiness"
        remaining = launched + timeout - time.monotonic()
        if remaining <= 0:
            return None, {}, "startup_timeout"
        watch.event.wait(min(poll, remaining))


def _collect(process, duration, warmup, interval, exit_margin, started, watch=None, synchronization=None):
    probe = Probe(process.pid if process else None)
    previous = time.monotonic()
    samples = [{"elapsed_s": previous - started, "monotonic_s": previous, "interval_s": 0.0, "measurement_interval_s": 0.0, "phase": "initial", "metrics": probe.read(1.0)}]
    invalid = []
    deadline = started + warmup + duration
    warmup_end = started + warmup
    if process:
        while time.monotonic() < deadline and not watch.event.is_set():
            now = time.monotonic()
            next_time = min(previous + interval, deadline)
            if previous < warmup_end < next_time:
                next_time = warmup_end
            watch.event.wait(max(0, next_time - now))
            now = time.monotonic()
            if watch.event.is_set():
                break
            phase = "measurement" if previous >= warmup_end else "warmup"
            samples.append({"elapsed_s": now - started, "monotonic_s": now, "interval_s": now - previous, "measurement_interval_s": max(0, min(now, deadline) - max(previous, warmup_end)) if phase == "measurement" else 0.0, "phase": phase, "metrics": probe.read(now - previous)})
            previous = now
        if not watch.event.wait(max(0, deadline + exit_margin - time.monotonic())):
            invalid.append("timeout")
            _stop(process)
            watch.cleaned = True
            watch.event.wait(1)
        uncertainty = (synchronization or {}).get("observation_latency_bound_s", 0)
        if watch.observed_at is not None and watch.observed_at < deadline - uncertainty:
            invalid.append("early_exit")
        if process.returncode != 0:
            invalid.append("nonzero_exit")
        if _group_exists(process):
            invalid.append("lingering_descendants")
    else:
        while previous < deadline:
            next_time = min(previous + interval, deadline)
            time.sleep(max(0, next_time - time.monotonic()))
            now = time.monotonic()
            samples.append({"elapsed_s": now - started, "monotonic_s": now, "interval_s": now - previous, "measurement_interval_s": max(0, min(now, deadline) - previous), "phase": "measurement", "metrics": probe.read(now - previous)})
            previous = now
    metrics = _aggregate(samples, duration)
    if process:
        for name in ("root_cpu_percent", "root_rss_bytes"):
            if metrics[name]["value"] is None or metrics[name]["coverage_fraction"] < 0.9:
                invalid.append(f"{name}_unavailable_or_incomplete")
    complete = all(metrics[name]["value"] is not None and metrics[name]["coverage_fraction"] >= 0.9 for name in ("process_cpu_percent", "rss_bytes")) if process else None
    return {"kind": "candidate" if process else "baseline", "pid": process.pid if process else None, "exit_code": process.returncode if process else None, "exit_observed_monotonic_s": watch.observed_at if watch else None, "exit_observation_method": "dedicated blocking wait thread; OS/thread scheduling latency remains" if watch else None, "valid": not invalid, "process_valid": not invalid if process else None, "telemetry_complete": complete, "metric_scope": ("process_tree" if complete else "root_only") if process else "no_pet", "invalid_reasons": invalid, "metrics": metrics, "samples": samples, "warmup_s": warmup, "measurement_s": duration, "measurement_start_monotonic_s": warmup_end, "measurement_end_monotonic_s": deadline, "synchronization": synchronization, "elapsed_s": time.monotonic() - started}


def collect_process(argv, *, duration, warmup=0.0, interval=1.0, exit_margin=5.0, output_dir=None, environment=None, ready_file=None, startup_timeout=30.0):
    _validate_times(duration, warmup, interval, exit_margin)
    if not argv or not all(isinstance(arg, str) for arg in argv):
        raise ValueError("argv must be a nonempty array of strings")
    if not math.isfinite(startup_timeout) or startup_timeout <= 0:
        raise ValueError("startup_timeout must be finite and positive")
    if ready_file is not None and Path(ready_file).exists():
        raise ValueError("ready file must not already exist")
    directory = Path(output_dir) if output_dir else None
    if directory:
        directory.mkdir(parents=True, exist_ok=True)
    stdout = (directory / "stdout.log").open("wb") if directory else subprocess.DEVNULL
    stderr = (directory / "stderr.log").open("wb") if directory else subprocess.DEVNULL
    process = None
    watch = None
    started = time.monotonic()
    try:
        process = subprocess.Popen(argv, stdout=stdout, stderr=stderr, env={**os.environ, **(environment or {})}, start_new_session=os.name == "posix")
        watch = ExitWatch(process)
        origin = started
        synchronization = {"method": "process launch; diagnostic only", "exact_phase_alignment": False, "observation_latency_bound_s": 0}
        error = None
        if ready_file is not None:
            origin, synchronization, error = _await_ready(process, watch, Path(ready_file), startup_timeout, started)
            if error:
                _stop(process)
                watch.cleaned = True
                watch.event.wait(1)
                origin = time.monotonic()
        result = _collect(process, duration, warmup, interval, exit_margin, origin, watch, synchronization)
        result["startup_elapsed_s"] = origin - started
        if error:
            result["invalid_reasons"].append(error)
            result["valid"] = result["process_valid"] = False
        return result
    finally:
        if process is not None and not (watch and watch.cleaned):
            _stop(process)
        if directory:
            stdout.close()
            stderr.close()


def collect_baseline(*, duration, interval=1.0):
    _validate_times(duration, 0, interval, 0)
    return _collect(None, duration, 0, interval, 0, time.monotonic())
