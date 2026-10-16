"""Software-only preview CLI and capture contracts."""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

EXECUTABLE = Path(sys.argv.pop(1)).resolve()
CHARACTER = Path(sys.argv.pop(1)).resolve()

class PreviewCliTests(unittest.TestCase):
    def run_preview(self, *args):
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software")
        return subprocess.run([str(EXECUTABLE), "--character", str(CHARACTER), *map(str,args)], env=env, capture_output=True, timeout=10)

    def test_help_lists_capture_styles_and_force_options(self):
        result = self.run_preview("--help")
        self.assertEqual(result.returncode, 0)
        for option in (b"--palette", b"--linear", b"--debug", b"--impulse", b"--impulse-strength", b"--speed"):
            self.assertIn(option, result.stdout)

    def test_invalid_options_reject_without_capture(self):
        cases = [("--duration", value) for value in ("nan", "inf", "0", "-1", "86401")]
        cases += [("--speed", value) for value in ("nan", "inf", "0.4", "3.1")]
        cases += [("--capture", ""), ("--time", "nan"), ("--mode", "3d"), ("--resolution", "65"), ("--action", "missing"), ("--impulse", "nan,1"), ("--impulse", "9000,0"), ("--impulse", "1"), ("--impulse-strength", "inf"), ("--impulse-strength", "5"), ("--gaze", "inf,0"), ("--gaze", "8193,0")]
        for args in cases:
            with self.subTest(args=args):
                result = self.run_preview(*args)
                self.assertEqual(result.returncode, 2, result.stderr.decode())

    def test_seeded_autonomous_capture_produces_procedural_expression(self):
        with tempfile.TemporaryDirectory() as directory:
            captures = []
            for index, seconds in enumerate((0, 13.1)):
                output = Path(directory) / f"curious{index}.png"
                result = self.run_preview("--capture", output, "--autonomous", "--seed", "1", "--time", seconds)
                self.assertEqual(result.returncode, 0, result.stderr.decode())
                captures.append(output.read_bytes())
                config = json.loads(Path(str(output) + ".json").read_text())
                self.assertTrue(config["autonomous"])
                self.assertEqual(config["seed"], 1)
            self.assertTrue(captures[0] != captures[1], "seeded idle expression changes rendered pixels")

    def test_live_duration_exits_boundedly(self):
        result = self.run_preview("--duration", ".05")
        self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_failed_load_reports_error(self):
        result = self.run_preview("--character", str(CHARACTER / "missing"), "--duration", ".05")
        self.assertEqual(result.returncode, 2)
        self.assertTrue(result.stderr.strip())

    def test_capture_identity_and_bounded_exit(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)/"capture.png"
            result = self.run_preview("--capture", output, "--action", "rest", "--time", ".08", "--mode", "composited", "--resolution", "64", "--palette", "--dither", "--linear", "--debug", "--impulse", "1,0", "--impulse-strength", "2")
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            config = json.loads(Path(str(output)+".json").read_text())
            self.assertEqual(config["character"], str(CHARACTER))
            self.assertEqual(config["action"], "rest")
            self.assertEqual(config["time"], .08)
            self.assertEqual(config["mode"], "composited")
            self.assertEqual(config["resolution"], 64)
            for key in ("palette", "dither", "debug", "effective_cpu_composition"):
                self.assertTrue(config[key])
            self.assertEqual(config["piece_filter"], "linear")
            self.assertEqual(config["impulse"], [1,0])
            self.assertEqual(config["impulse_strength"], 2)
            self.assertEqual(config["platform"], "offscreen")
            self.assertFalse(config["native_scaling_verified"])
            data = output.read_bytes()
            self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
            self.assertEqual(struct.unpack(">II", data[16:24]), (config["capture_width"],config["capture_height"]))

    def test_force_and_gaze_change_controlled_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            captures = []
            for index, args in enumerate(((), ("--impulse", "1,0"), ("--gaze", "45,0"))):
                output = Path(directory)/f"pose{index}.png"
                result = self.run_preview("--capture", output, "--action", "rest", "--time", ".08", *args)
                self.assertEqual(result.returncode, 0, result.stderr.decode())
                captures.append(output.read_bytes())
            self.assertNotEqual(captures[0], captures[1])
            self.assertNotEqual(captures[0], captures[2])

    def test_invalid_output_fails_boundedly(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_preview("--capture", Path(directory)/"absent"/"capture.png")
            self.assertEqual(result.returncode, 3)

    def test_grounded_target_arrives_and_settles(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "grounded.png"
            result = self.run_preview("--capture", output, "--target", "60", "--time", "8")
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            config = json.loads(Path(str(output) + ".json").read_text())
            self.assertTrue(config["grounded"])
            self.assertAlmostEqual(config["root_x"], 60, delta=.5)
            self.assertFalse(config["motion_active"])
            self.assertEqual(config["mode"], "composited")
            self.assertEqual(config["piece_filter"], "nearest")

    def test_grounded_script_can_reverse(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "reverse.png"
            result = self.run_preview("--capture", output, "--target", "60",
                                      "--reverse-at", "1", "--time", "8")
            self.assertEqual(result.returncode, 0, result.stderr.decode())
            config = json.loads(Path(str(output) + ".json").read_text())
            self.assertAlmostEqual(config["root_x"], -60, delta=.5)
            self.assertFalse(config["motion_active"])

    def test_walking_speed_changes_actual_motion_at_same_wall_time(self):
        with tempfile.TemporaryDirectory() as directory:
            positions = []
            for multiplier in (1, 2, 3):
                output = Path(directory) / f"speed{multiplier}.png"
                result = self.run_preview("--capture", output, "--target", "90",
                                          "--time", ".9", "--speed", multiplier)
                self.assertEqual(result.returncode, 0, result.stderr.decode())
                config = json.loads(Path(str(output) + ".json").read_text())
                self.assertEqual(config["walking_speed"], multiplier)
                self.assertEqual(config["time"], .9)
                positions.append(config["root_x"])
            self.assertGreater(positions[1], positions[0] + 10)
            # This short preview trip can start braking before reaching the
            # 3x speed cap. Full cruise scaling is covered by grounded motion
            # tests over a long span; here faster settings must not travel less.
            self.assertGreaterEqual(positions[2], positions[1] - 1e-6)

if __name__ == "__main__":
    unittest.main()
