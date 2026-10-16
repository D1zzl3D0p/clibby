"""Run against the built binary; all graphical tests use explicit offscreen mode.

This validates process lifecycle and scheduler behavior, not desktop integration.
"""
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

APP = str(pathlib.Path(sys.argv.pop(1)).resolve())
ASSETS = str(pathlib.Path(sys.argv.pop(1)).resolve())


class CliTests(unittest.TestCase):
    def run_app(self, *extra, duration="0.2", scenario="static", warmup="0"):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        output = pathlib.Path(folder.name) / "telemetry.json"
        args = [APP, "--scenario", scenario, "--duration", duration, "--warmup", warmup,
                "--assets", ASSETS, "--output", str(output), *extra]
        result = subprocess.run(args, capture_output=True, text=True, timeout=15,
                                env={**os.environ, "QT_QPA_PLATFORM": "offscreen", "QT_QUICK_BACKEND": "software"})
        return result, json.loads(output.read_text()) if output.exists() else None

    def test_rejects_malformed_cli(self):
        for values in [{"duration": "nan"}, {"duration": "0"}, {"duration": "-1"},
                       {"scenario": "bad"}, {"warmup": "99"}]:
            with self.subTest(values=values):
                result, data = self.run_app(**values)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("error", result.stderr.lower())
                self.assertIsNone(data)
        result, _ = self.run_app("--unknown", "x")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("error", result.stderr.lower())
        for assets in ["/missing-fixture", "shared"]:
            result = subprocess.run([APP, "--scenario", "static", "--duration", "0.1", "--assets", assets,
                                     "--output", "/private/tmp/qt-cli-invalid-output.json"], capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("assets", result.stderr.lower())

    def test_static_stops_pose_scheduler(self):
        result, data = self.run_app(duration="0.4", warmup="0.1")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["pose_evaluations"], 0)
        self.assertEqual(data["scheduler_callbacks"], 0)
        self.assertGreaterEqual(data["elapsed_seconds"], 0.38)
        self.assertIsNone(data["displayed_frames"])

    def test_hidden_stops_pose_and_drawing(self):
        result, data = self.run_app(scenario="hidden")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(data["pose_evaluations"], 0)
        self.assertEqual(data["scheduler_callbacks"], 0)
        self.assertEqual(data["frame_swapped_events"], 0)

    def test_active_advances_and_warmup_resets_clock(self):
        result, data = self.run_app(scenario="idle-motion", duration="0.6", warmup="0.2")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertGreater(data["pose_evaluations"], 15)
        self.assertLessEqual(data["pose_evaluations"], 25)
        self.assertTrue(data["measurement_clock_reset"])
        self.assertEqual(data["measurement_start_pose"]["root_offset"], [0, 0])

    def test_headless_pose_dump(self):
        with tempfile.TemporaryDirectory() as folder:
            target = pathlib.Path(folder) / "poses.json"
            result, _ = self.run_app("--dump-poses", str(target))
            self.assertEqual(result.returncode, 0, result.stderr)
            poses = json.loads(target.read_text())["poses"]
            self.assertGreater(len(poses), 10)

    def test_ready_marker_identifies_process(self):
        with tempfile.TemporaryDirectory() as folder:
            ready = pathlib.Path(folder) / "ready.json"
            output = pathlib.Path(folder) / "telemetry.json"
            process = subprocess.Popen([APP, "--scenario", "hidden", "--duration", "0.1",
                                        "--output", str(output), "--assets", ASSETS, "--ready-file", str(ready)],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                       env={**os.environ, "QT_QPA_PLATFORM": "offscreen", "QT_QUICK_BACKEND": "software"})
            _, stderr = process.communicate(timeout=15)
            self.assertEqual(process.returncode, 0, stderr)
            marker = json.loads(ready.read_text())
            self.assertTrue(marker["ready"])
            self.assertEqual(marker["pid"], process.pid)


if __name__ == "__main__":
    unittest.main()
