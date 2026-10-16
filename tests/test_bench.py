"""Collector regression tests run real child processes; no fake process APIs."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest


class CollectorTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(importlib.util.find_spec("bench.metrics"), "collector implementation missing")
        from bench.metrics import collect_process
        self.collect = collect_process

    def test_early_exit_is_invalid(self):
        result = self.collect([sys.executable, "-c", "pass"], duration=0.3, interval=0.05)
        self.assertFalse(result["valid"])
        self.assertIn("early_exit", result["invalid_reasons"])

    def test_nonzero_exit_is_invalid(self):
        result = self.collect([sys.executable, "-c", "raise SystemExit(7)"], duration=0.1, interval=0.03)
        self.assertFalse(result["valid"])
        self.assertEqual(result["exit_code"], 7)

    def test_exit_between_last_two_samples_retains_missing_tail(self):
        result = self.collect([sys.executable, "-c", "import time; time.sleep(0.2)"], duration=0.3, interval=0.15)
        self.assertFalse(result["valid"], result)
        self.assertIn("early_exit", result["invalid_reasons"])
        self.assertLess(result["metrics"]["root_cpu_percent"]["coverage_fraction"], 0.8)

    def test_startup_delay_does_not_consume_warmup_or_measurement(self):
        with tempfile.TemporaryDirectory() as temporary:
            results = []
            for delay in (0.01, 0.25):
                ready = Path(temporary) / f"ready-{delay}.json"
                code = f"import json,os,pathlib,time; time.sleep({delay}); p=pathlib.Path({str(ready)!r}); q=p.with_suffix('.tmp'); q.write_text(json.dumps(dict(ready=True,pid=os.getpid()))); q.replace(p); time.sleep(0.5)"
                result = self.collect([sys.executable, "-c", code], duration=0.3, warmup=0.1, interval=0.025, ready_file=ready, startup_timeout=1)
                results.append(result)
                self.assertTrue(result["valid"], result["invalid_reasons"])
                self.assertAlmostEqual(result["measurement_end_monotonic_s"] - result["measurement_start_monotonic_s"], 0.3)
                self.assertGreaterEqual(result["metrics"]["root_cpu_percent"]["coverage_fraction"], 0.9)
                self.assertLessEqual(result["synchronization"]["poll_interval_s"], 0.01)
                self.assertFalse(result["synchronization"]["exact_phase_alignment"])
            self.assertGreater(results[1]["startup_elapsed_s"] - results[0]["startup_elapsed_s"], 0.15)

    def test_missing_readiness_times_out_and_reaps_process(self):
        with tempfile.TemporaryDirectory() as temporary:
            result = self.collect([sys.executable, "-c", "import time; time.sleep(30)"], duration=0.2, ready_file=Path(temporary) / "ready.json", startup_timeout=0.1)
            self.assertFalse(result["valid"])
            self.assertIn("startup_timeout", result["invalid_reasons"])
            with self.assertRaises(ProcessLookupError):
                os.kill(result["pid"], 0)

    def test_root_exit_cleans_descendants_on_clean_and_failed_exits(self):
        for exit_code in (0, 7):
            with self.subTest(exit_code=exit_code), tempfile.TemporaryDirectory() as temporary:
                path = Path(temporary) / "child.pid"
                parent = f"import subprocess,sys,time,pathlib; p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)']); pathlib.Path({str(path)!r}).write_text(str(p.pid)); time.sleep(0.08); sys.exit({exit_code})"
                result = self.collect([sys.executable, "-c", parent], duration=0.2, interval=0.05)
                pid = int(path.read_text())
                alive = True
                for _ in range(20):
                    try:
                        os.kill(pid, 0)
                        time.sleep(0.02)
                    except ProcessLookupError:
                        alive = False
                        break
                if alive:
                    os.kill(pid, 9)
                self.assertFalse(alive, "descendant leaked after root exited")
                self.assertIn("lingering_descendants", result["invalid_reasons"])

    def test_timeout_cleans_up_spawned_process(self):
        result = self.collect([sys.executable, "-c", "import time; time.sleep(60)"], duration=0.1, exit_margin=0.1, interval=0.03)
        self.assertFalse(result["valid"])
        self.assertIn("timeout", result["invalid_reasons"])
        self.assertIsNotNone(result["exit_code"])
        with self.assertRaises(ProcessLookupError):
            os.kill(result["pid"], 0)

    def test_cpu_normalization_is_one_core_not_machine_capacity(self):
        from bench.metrics import cpu_percent
        self.assertEqual(cpu_percent(12.0, 12.5, 0.5), 100.0)
        self.assertEqual(cpu_percent(12.0, 13.0, 0.5), 200.0)

    def test_no_pet_baseline_labels_unavailable_metrics(self):
        from bench.metrics import collect_baseline
        result = collect_baseline(duration=0.12, interval=0.04)
        self.assertEqual(result["kind"], "baseline")
        self.assertIsNone(result["metrics"]["process_cpu_percent"]["value"])
        self.assertIn("no pet", result["metrics"]["process_cpu_percent"]["reason"])
        self.assertIsNone(result["metrics"]["whole_system_power_w"]["value"])
        self.assertTrue(result["metrics"]["whole_system_power_w"]["reason"])

    def test_process_collects_samples_after_warmup(self):
        code = "import time; end=time.monotonic()+0.55\nwhile time.monotonic()<end: sum(range(1000))"
        result = self.collect([sys.executable, "-c", code], warmup=0.1, duration=0.3, interval=0.05, exit_margin=1)
        self.assertGreater(len(result["samples"]), 2)
        measured = [s for s in result["samples"] if s["phase"] == "measurement"]
        self.assertTrue(all(s["elapsed_s"] >= 0.1 for s in measured))
        metric = result["metrics"]["root_cpu_percent"]
        if metric["value"] is None:
            self.assertTrue(metric["reason"])
            self.assertFalse(result["valid"])
        else:
            self.assertGreater(metric["value"], 5)
            self.assertTrue(result["valid"], result["invalid_reasons"])

    def test_timeout_cleans_up_descendants_that_ignore_termination(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "child.pid"
            child = "import signal,time; signal.signal(signal.SIGTERM,signal.SIG_IGN); time.sleep(60)"
            parent = f"import subprocess,sys,time,pathlib; p=subprocess.Popen([sys.executable,'-c',{child!r}]); pathlib.Path({str(path)!r}).write_text(str(p.pid)); time.sleep(60)"
            result = self.collect([sys.executable, "-c", parent], duration=0.25, exit_margin=0.1, interval=0.05)
            self.assertIn("timeout", result["invalid_reasons"])
            pid = int(path.read_text())
            alive = True
            for _ in range(20):
                try:
                    os.kill(pid, 0)
                    time.sleep(0.025)
                except ProcessLookupError:
                    alive = False
                    break
            if alive:
                os.kill(pid, 9)
            self.assertFalse(alive, "descendant survived process-group timeout cleanup")


class ReportingTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(importlib.util.find_spec("bench.report"), "report implementation missing")

    def test_no_power_does_not_pass_battery(self):
        from bench.report import summarize_runs
        report = summarize_runs([])
        self.assertEqual(report["battery_acceptance"], "unverified")
        self.assertIsNone(report["toolkit_winner"])

    def test_validation_reports_cannot_look_like_desktop_comparisons(self):
        from bench.report import summarize_runs, markdown
        for engine in ({"graphical_runtime": False}, {"platform": "offscreen"}, {"platform": "minimal"}, {"display_driver": "headless"}):
            with self.subTest(engine=engine):
                run = {"kind": "candidate", "candidate": "test", "scenario": "static", "valid": True, "engine_summary": engine, "metrics": {"root_cpu_percent": {"value": 12, "coverage_fraction": 1}}}
                report = summarize_runs([run])
                self.assertEqual(report["purpose"], "validation_only")
                self.assertFalse(report["desktop_comparison"])
                self.assertEqual(report["candidates"][0]["metrics"]["root_cpu_percent"]["mean"], 12)
                text = markdown(report)
                self.assertIn("VALIDATION ONLY", text)
                self.assertIn("not a desktop performance comparison", text)
        configured = summarize_runs([], environment={"purpose": "pipeline validation"})
        self.assertEqual(configured["purpose"], "validation_only")
        software = summarize_runs([{"kind":"candidate","candidate":"test","scenario":"static","valid":True,"engine_summary":{"platform":"cocoa","backend":"Software"},"metrics":{}}])
        self.assertEqual(software["purpose"], "resource_screening")

    def test_deltas_exclude_invalid_runs_and_preserve_negative_difference(self):
        from bench.report import summarize_runs
        runs = []
        for block in range(3):
            for name, cpu, rss in [("a", 12, 100), ("b", 8, 150)]:
                runs.append({"kind": "candidate", "block_id": str(block), "pair": ["a", "b"], "scenario": "static", "candidate": name, "valid": not (block == 2 and name == "b"), "metrics": {"process_cpu_percent": {"value": cpu, "coverage_fraction": 1}, "rss_bytes": {"value": rss, "coverage_fraction": 1}}})
        report = summarize_runs(runs)
        pair = report["paired_deltas"][0]
        self.assertEqual(pair["valid_blocks"], 2)
        self.assertEqual(pair["metrics"]["process_cpu_percent"]["mean_b_minus_a"], -4)
        self.assertEqual(pair["metrics"]["rss_bytes"]["mean_b_minus_a"], 50)
        self.assertIsNone(pair["metrics"]["rss_bytes"]["ci95"])
        self.assertEqual(report["valid_candidate_runs"], 5)

    def test_incomplete_tree_observations_are_excluded_per_metric(self):
        from bench.report import summarize_runs
        runs = [{"kind": "candidate", "block_id": "0", "pair": ["a", "b"], "scenario": "static", "candidate": name, "valid": True, "metric_scope": "root_only", "metrics": {"process_cpu_percent": {"value": 100, "coverage_fraction": 0.1}, "root_cpu_percent": {"value": cpu, "coverage_fraction": 1}}} for name, cpu in (("a", 10), ("b", 15))]
        report = summarize_runs(runs)
        tree = report["candidates"][0]["metrics"]["process_cpu_percent"]
        self.assertIsNone(tree["mean"])
        self.assertEqual(tree["excluded_runs"], 1)
        self.assertIn("coverage", tree["exclusions"][0]["reason"])
        self.assertIsNone(report["paired_deltas"][0]["metrics"]["process_cpu_percent"]["mean_b_minus_a"])
        self.assertEqual(report["paired_deltas"][0]["metrics"]["root_cpu_percent"]["mean_b_minus_a"], 5)

    def test_engine_summary_rejects_missing_mismatched_and_nonfinite_evidence(self):
        import bench.run as runner
        self.assertTrue(hasattr(runner, "validate_engine_summary"))
        valid = {"engine": "Qt", "engine_version": "6.10.3", "backend": "Metal", "scenario": "static", "duration_seconds": 0.4, "warmup_seconds": 0.1, "elapsed_seconds": 0.405, "measurement_elapsed_seconds": 0.305, "measurement_clock_reset": True, "pose_evaluations": 0, "window_position_writes": 0, "simulation_overruns": 0, "dropped_steps": 0, "frame_swapped_events": 1, "frame_event_semantics": "queued", "frame_event_intervals_seconds": [], "error": None}
        runner.validate_engine_summary(valid, scenario="static", duration=0.3, warmup=0.1)
        headless_godot = {**valid, "engine": "Godot", "backend": None, "backend_unavailable_reason": "headless validation run has no GPU backend"}
        runner.validate_engine_summary(headless_godot, scenario="static", duration=0.3, warmup=0.1)
        for changed in ({"scenario": "static"}, {**valid, "duration_seconds": 2}, {**valid, "elapsed_seconds": float("nan")}, {**valid, "measurement_elapsed_seconds": 0.05}, {**valid, "backend": None}, {**valid, "frame_swapped_events": -1}):
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                runner.validate_engine_summary(changed, scenario="static", duration=0.3, warmup=0.1)


class CommandTests(unittest.TestCase):
    def test_single_candidate_profiles_each_scenario_with_bracketed_baselines(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            app = directory / "profile child.py"
            app.write_text('''import argparse,json,pathlib,time,os
p=argparse.ArgumentParser()
for name in ('scenario','output','assets','ready-file'):p.add_argument('--'+name)
p.add_argument('--duration',type=float);p.add_argument('--warmup',type=float)
a=p.parse_args();ready=pathlib.Path(a.ready_file);pending=ready.with_suffix('.tmp')
pending.write_text(json.dumps({'ready':True,'pid':os.getpid()}));pending.replace(ready)
time.sleep(a.duration)
pathlib.Path(a.output).write_text(json.dumps({'engine':'Qt','engine_version':'test','backend':'test','scenario':a.scenario,'duration_seconds':a.duration,'warmup_seconds':a.warmup,'elapsed_seconds':a.duration,'measurement_elapsed_seconds':a.duration-a.warmup,'pose_evaluations':0,'window_position_writes':0,'dropped_steps':0,'simulation_overruns':0,'frame_swapped_events':0,'frame_event_semantics':'test callback','frame_event_intervals_seconds':[],'error':None}))
''')
            config = directory / "config.json"
            config.write_text(json.dumps({"schema_version": 1, "assets": ".", "scenarios": ["static", "hidden"], "seed": 11, "candidates": [{"name": "qt", "argv": [sys.executable, str(app)], "renderer": "test"}]}))
            output = directory / "results"
            process = subprocess.run([sys.executable, "-m", "bench.run", "--config", str(config), "--output", str(output), "--warmup", "0.02", "--duration", "0.16", "--interval", "0.01", "--baseline-duration", "0.02", "--cooldown", "0", "--repeats", "2"], capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            manifest = json.loads((output / "manifest.json").read_text())
            self.assertEqual([block["scenario"] for block in manifest["blocks"]], ["static", "static", "hidden", "hidden"])
            self.assertTrue(all(block["order"] == ["qt"] and "pair" not in block for block in manifest["blocks"]))
            runs = [json.loads(path.read_text()) for path in sorted(output.glob("runs/*/result.json"))]
            self.assertEqual([run["kind"] for run in runs], ["baseline", "candidate", "baseline"] * 4)
            self.assertEqual([run["baseline_position"] for run in runs if run["kind"] == "baseline"], ["before", "after"] * 4)
            report = json.loads((output / "summary.json").read_text())
            self.assertEqual(report["mode"], "profiling")
            self.assertIn("descriptive", report["uncertainty_method"])
            self.assertIn("no confidence intervals", report["uncertainty_method"])
            self.assertTrue(all("paired repetitions" not in limitation for limitation in report["limitations"]))
            self.assertEqual(report["attempted_candidate_runs"], 4)
            self.assertEqual(report["baseline_runs"], 8)
            self.assertEqual(report["paired_deltas"], [])
            self.assertEqual(report["battery_acceptance"], "unverified")
            self.assertIn("Resource profiling report", (output / "summary.md").read_text())
            self.assertNotIn("Paired differences", (output / "summary.md").read_text())

    def test_cli_rejects_empty_candidate_list(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config = directory / "config.json"
            config.write_text(json.dumps({"schema_version": 1, "assets": ".", "scenarios": ["static"], "candidates": []}))
            process = subprocess.run([sys.executable, "-m", "bench.run", "--config", str(config), "--output", str(directory / "out")], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn("candidate", process.stderr)
            self.assertFalse((directory / "out").exists())

    def test_nonfinite_engine_summary_is_recorded_invalid_without_aborting_suite(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            app = directory / "bad.py"
            app.write_text('''import argparse,json,pathlib,time,os
p=argparse.ArgumentParser();p.add_argument('--ready-file');p.add_argument('--output');p.add_argument('--duration',type=float)
a,_=p.parse_known_args();pathlib.Path(a.ready_file).write_text(json.dumps({'ready':True,'pid':os.getpid()}));time.sleep(a.duration)
pathlib.Path(a.output).write_text('{"scenario":"static","elapsed_seconds":NaN}')
''')
            config = directory / "config.json"
            config.write_text(json.dumps({"schema_version":1,"assets":".","scenarios":["static"],"candidates":[{"name":name,"argv":[sys.executable,str(app)],"renderer":"test"} for name in ("a","b")]}))
            output = directory / "results"
            process = subprocess.run([sys.executable,"-m","bench.run","--config",str(config),"--output",str(output),"--warmup","0","--duration","0.08","--interval","0.01","--baseline-duration","0.01","--cooldown","0","--repeats","1"],capture_output=True,text=True)
            self.assertEqual(process.returncode,0,process.stderr)
            report=json.loads((output/"summary.json").read_text())
            self.assertEqual(report["attempted_candidate_runs"],2)
            self.assertEqual(report["valid_candidate_runs"],0)
            self.assertTrue(all("engine_telemetry_invalid" in item["failures"][0][-1] for item in report["candidates"]))

    def test_cli_saves_bracketed_baselines_summary_and_engine_arguments(self):
        # Catches losing argv boundaries, warmup contract, or raw/manifest output.
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "shared").mkdir()
            app = directory / "child app.py"
            app.write_text('''import argparse, json, pathlib, time, os
p=argparse.ArgumentParser()
p.add_argument('--scenario');p.add_argument('--duration',type=float)
p.add_argument('--warmup',type=float);p.add_argument('--output');p.add_argument('--assets')
p.add_argument('--ready-file')
a=p.parse_args();p=pathlib.Path(a.ready_file);t=p.with_suffix('.tmp');t.write_text(json.dumps({'ready':True,'pid':os.getpid()}));t.replace(p)
time.sleep(a.duration)
pathlib.Path(a.output).write_text(json.dumps({'engine':'Qt','engine_version':'test','backend':'test','scenario':a.scenario,'warmup_seconds':a.warmup,'duration_seconds':a.duration,'elapsed_seconds':a.duration,'measurement_elapsed_seconds':a.duration-a.warmup,'assets':a.assets,'error':None,'frame_swapped_events':1,'pose_evaluations':0,'window_position_writes':0,'simulation_overruns':0,'dropped_steps':0,'frame_event_semantics':'test callback','frame_event_intervals_seconds':[]}))
''')
            config = {"schema_version": 1, "assets": "shared", "scenarios": ["static"], "seed": 7, "candidates": [{"name": name, "argv": [sys.executable, str(app)], "renderer": "test"} for name in ("a", "b")]}
            config_path = directory / "config.json"
            config_path.write_text(json.dumps(config))
            output = directory / "results"
            process = subprocess.run([sys.executable, "-m", "bench.run", "--config", str(config_path), "--output", str(output), "--warmup", "0.04", "--duration", "0.12", "--repeats", "2", "--baseline-duration", "0.04", "--cooldown", "0", "--interval", "0.02"], capture_output=True, text=True)
            self.assertEqual(process.returncode, 0, process.stderr)
            manifest = json.loads((output / "manifest.json").read_text())
            self.assertEqual(manifest["seed"], 7)
            blocks = manifest["blocks"]
            self.assertEqual(len(blocks), 2)
            self.assertEqual(blocks[0]["order"], list(reversed(blocks[1]["order"])))
            report = json.loads((output / "summary.json").read_text())
            self.assertEqual(report["battery_acceptance"], "unverified")
            runs = [json.loads(path.read_text()) for path in sorted(output.glob("runs/*/result.json"))]
            self.assertEqual([run["kind"] for run in runs], ["baseline", "candidate", "candidate", "baseline"] * 2)
            engines = list(output.glob("runs/*/engine-summary.json"))
            self.assertEqual(len(engines), 4)
            for path in engines:
                engine = json.loads(path.read_text())
                self.assertAlmostEqual(engine["duration_seconds"], 0.16)
                self.assertEqual(engine["warmup_seconds"], 0.04)
                self.assertEqual(engine["assets"], str((directory / "shared").resolve()))
            self.assertTrue((output / "summary.md").is_file())

    def test_cli_rejects_invalid_config_before_creating_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            config = directory / "config.json"
            config.write_text(json.dumps({"schema_version": 1, "assets": ".", "scenarios": ["typo"], "candidates": []}))
            process = subprocess.run([sys.executable, "-m", "bench.run", "--config", str(config), "--output", str(directory / "out")], capture_output=True, text=True)
            self.assertNotEqual(process.returncode, 0)
            self.assertIn("scenario", process.stderr.lower())
            self.assertFalse((directory / "out").exists())


if __name__ == "__main__":
    unittest.main()
