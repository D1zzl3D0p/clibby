"""Executable-boundary tests for explicit command outcome notifications."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
WRAPPER = ROOT / "tools/pet_run.py"


class CommandProducerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.cwd = Path(self.temp.name).resolve()
        self.log = self.cwd / "events.jsonl"
        self.petctl = self.cwd / "petctl"
        self.petctl.write_text(
            f"#!{sys.executable}\n"
            "import json, os, sys\n"
            "with open(os.environ['EVENT_LOG'], 'a') as f: f.write(json.dumps(sys.argv[1:]) + '\\n')\n"
            "if os.environ.get('PETCTL_FAIL'): print('resident unavailable', file=sys.stderr); sys.exit(3)\n"
            "print('{}')\n"
        )
        self.petctl.chmod(0o755)

    def run_command(self, code, options=(), extra_env=None):
        env = dict(os.environ, EVENT_LOG=str(self.log))
        env.update(extra_env or {})
        result = subprocess.run(
            [sys.executable, str(WRAPPER), "--petctl", str(self.petctl),
             *options, "--", sys.executable, "-c", code],
            cwd=self.cwd, env=env, capture_output=True, text=True, timeout=10,
        )
        events = [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []
        return result, events

    def test_success_emits_working_then_completed_with_source_and_keeps_output(self):
        result, events = self.run_command("print('command output')", ["--label", "Compile"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "command output\n")
        self.assertEqual(len(events), 2)
        for event, status, seq in zip(events, ["working", "completed"], ["1", "2"]):
            self.assertEqual(event[0], "update")
            self.assertEqual(event[event.index("--status") + 1], status)
            self.assertEqual(event[event.index("--seq") + 1], seq)
            self.assertEqual(event[event.index("--source-url") + 1], self.cwd.as_uri())
            self.assertEqual(event[event.index("--label") + 1], "Compile")
        self.assertEqual(events[0][events[0].index("--session") + 1], events[1][events[1].index("--session") + 1])

    def test_failure_preserves_exit_code_and_reports_failed(self):
        result, events = self.run_command("import sys; print('failure detail', file=sys.stderr); sys.exit(7)")
        self.assertEqual(result.returncode, 7)
        self.assertIn("failure detail", result.stderr)
        self.assertEqual(events[-1][events[-1].index("--status") + 1], "failed")

    def test_notification_failure_is_visible_and_does_not_change_command_result(self):
        result, events = self.run_command("print('still ran')", extra_env={"PETCTL_FAIL": "1"})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "still ran\n")
        self.assertIn("resident unavailable", result.stderr)
        self.assertEqual(len(events), 2)

    def test_signal_outcome_is_failed_and_preserves_shell_exit_convention(self):
        result, events = self.run_command("import os, signal; os.kill(os.getpid(), signal.SIGTERM)")
        self.assertEqual(result.returncode, 143, result.stderr)
        self.assertEqual(events[-1][events[-1].index("--status") + 1], "failed")

    def test_explicit_identity_and_source_url_forwarded(self):
        result, events = self.run_command("pass", ["--source", "build", "--session", "job-42", "--source-url", "https://example.com/build/42", "--state-dir", str(self.cwd)])
        self.assertEqual(result.returncode, 0, result.stderr)
        for event in events:
            for key, value in [("--source", "build"), ("--session", "job-42"), ("--source-url", "https://example.com/build/42"), ("--state-dir", str(self.cwd))]:
                self.assertEqual(event[event.index(key) + 1], value)

    def test_missing_command_rejected_before_notification(self):
        result = subprocess.run([sys.executable, str(WRAPPER), "--petctl", str(self.petctl)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(self.log.exists())

    def test_invalid_identity_rejected_before_running_command(self):
        result, events = self.run_command("print('must not run')", ["--source", "bad id"])
        self.assertEqual(result.returncode, 2)
        self.assertNotIn("must not run", result.stdout)
        self.assertEqual(events, [])

    def test_running_command_has_only_working_event_until_it_exits(self):
        code = "import json, os; events = [json.loads(x) for x in open(os.environ['EVENT_LOG'])]; assert len(events) == 1; assert events[0][events[0].index('--status') + 1] == 'working'"
        result, events = self.run_command(code)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(events), 2)

    def test_missing_executable_reports_failed_and_returns_127(self):
        env = dict(os.environ, EVENT_LOG=str(self.log))
        result = subprocess.run([sys.executable, str(WRAPPER), "--petctl", str(self.petctl), "--", str(self.cwd / "missing")], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 127, result.stderr)
        events = [json.loads(line) for line in self.log.read_text().splitlines()]
        self.assertEqual(events[-1][events[-1].index("--status") + 1], "failed")

    def test_missing_petctl_is_visible_but_command_still_runs(self):
        result, events = self.run_command("print('command ran')", ["--petctl", str(self.cwd / "absent-petctl")])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "command ran\n")
        self.assertIn("could not publish working", result.stderr)
        self.assertIn("could not publish completed", result.stderr)
        self.assertEqual(events, [])

    def test_transport_diagnostic_with_non_utf8_bytes_does_not_abort_command(self):
        self.petctl.write_text(f"#!{sys.executable}\nimport os, sys\nos.write(2, b'bad \\xff diagnostic'); sys.exit(3)\n")
        result, _ = self.run_command("print('command ran')")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "command ran\n")
        self.assertIn("could not publish", result.stderr)


if __name__ == "__main__":
    unittest.main()
