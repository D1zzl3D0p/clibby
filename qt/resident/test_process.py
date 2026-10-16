"""Resident process lifecycle checks; explicit offscreen mode is software evidence."""
import os
from pathlib import Path
import subprocess
import errno
import socket
import sys
import tempfile
import time
import unittest

APP = str(Path(sys.argv.pop(1)).resolve())
ASSETS = str(Path(sys.argv.pop(1)).resolve())
ENV = {**os.environ, "QT_QPA_PLATFORM": "offscreen", "QT_QUICK_BACKEND": "software"}


class ResidentProcessTests(unittest.TestCase):
    def require_local_socket(self):
        # This probes the OS capability, never infers a skip from an app failure.
        with tempfile.TemporaryDirectory() as directory:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as probe:
                try:
                    probe.bind(str(Path(directory) / "p"))
                except OSError as error:
                    if error.errno == errno.EPERM:
                        self.skipTest("execution sandbox denies Unix socket bind (EPERM)")
                    raise

    def run_app(self, *args):
        return subprocess.run([APP, *args], env=ENV, capture_output=True, text=True, timeout=10)

    def start(self, state, assets=True):
        args = [APP, "--state-dir", str(state)]
        if assets:
            args += ["--assets", ASSETS]
        return subprocess.Popen(args, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def ready(self, process, state):
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                _, stderr = process.communicate()
                self.fail(f"resident exited before ready: {process.returncode}: {stderr}")
            if (state / "settings.json").exists():
                try:
                    owner = (state / "instance.lock").read_text().splitlines()[0]
                    if owner == str(process.pid):
                        return
                except (OSError, IndexError):
                    pass
            time.sleep(0.02)
        self.fail("resident never established lock and settings")

    @staticmethod
    def stop(process):
        if process is not None:
            if process.poll() is None:
                process.terminate()
            try:
                process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.communicate(timeout=5)

    def test_help_succeeds_without_gui_or_assets(self):
        result = self.run_app("--help")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--state-dir", result.stdout)
        self.assertIn("--assets", result.stdout)

    def test_malformed_cli_and_missing_assets_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            for args in [("--unknown",), ("--assets",), ("unexpected",),
                         ("--assets", "relative"), ("--state-dir", "relative"),
                         ("--assets", str(Path(directory) / "missing"))]:
                with self.subTest(args=args):
                    result = self.run_app(*args, "--state-dir", directory)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("error", result.stderr.lower())

    def test_lock_io_error_is_not_duplicate_success(self):
        with tempfile.TemporaryDirectory() as directory:
            blocker = Path(directory) / "file"
            blocker.touch()
            result = self.run_app("--assets", ASSETS, "--state-dir", str(blocker))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("error", result.stderr.lower())

    def test_resident_duplicate_and_restart_after_termination(self):
        self.require_local_socket()
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory)
            first = restarted = None
            try:
                first = self.start(state)
                self.ready(first, state)
                time.sleep(0.15)
                self.assertIsNone(first.poll(), "resident must remain alive without a benchmark duration")
                duplicate = self.run_app("--assets", ASSETS, "--state-dir", directory)
                self.assertEqual(duplicate.returncode, 0, duplicate.stderr)
                self.assertIn("already running", duplicate.stdout.lower())
                self.assertIsNone(first.poll())
                self.stop(first)
                first = None
                restarted = self.start(state)
                self.ready(restarted, state)
                time.sleep(0.15)
                self.assertIsNone(restarted.poll(), "terminated owner does not prevent restart")
            finally:
                self.stop(first)
                self.stop(restarted)

    def test_copied_default_assets_are_usable(self):
        self.require_local_socket()
        with tempfile.TemporaryDirectory() as directory:
            process = None
            try:
                process = self.start(Path(directory), assets=False)
                self.ready(process, Path(directory))
                self.assertIsNone(process.poll())
            finally:
                self.stop(process)


if __name__ == "__main__":
    unittest.main()
