"""Provider hook replay and process-level ordering tests (no live agent required)."""
import concurrent.futures
import json
import runpy
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / "tools/pet_agent_hook.py"


class AgentHookTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.directory = Path(self.tmp.name)
        self.log = self.directory / "events.jsonl"
        self.petctl = self.directory / "fake petctl"
        self.petctl.write_text(
            f"#!{sys.executable}\n"
            "import json,sys,time\n"
            "from pathlib import Path\n"
            "args=sys.argv[1:]\n"
            "event=json.loads(args[args.index('--json')+1])\n"
            "time.sleep(0.015)\n"
            f"with Path({str(self.log)!r}).open('a') as out: out.write(json.dumps({{'args':args,'event':event}})+'\\n')\n"
            "print('not hook output')\n"
        )
        self.petctl.chmod(0o700)

    def test_default_directory_preserves_legacy_sequences(self):
        default_directory = runpy.run_path(str(HELPER))["default_sequence_dir"]
        for platform, base, old_name, new_name in (
            ("darwin", self.directory / "Library/Application Support", "Better Pets", "Clibby"),
            ("linux", self.directory / "xdg-state", "better-pets", "clibby"),
        ):
            with self.subTest(platform=platform), patch("sys.platform", platform), \
                    patch.object(Path, "home", return_value=self.directory), \
                    patch.dict(os.environ, {"XDG_STATE_HOME": str(self.directory / "xdg-state")}):
                current = base / new_name / "agent-hooks"
                legacy = base / old_name / "agent-hooks"
                self.assertEqual(default_directory(), current)
                legacy.mkdir(parents=True)
                (legacy / "session.json").write_text("27")
                self.assertEqual(default_directory(), legacy)
                current.mkdir(parents=True)
                self.assertEqual(default_directory(), legacy)
                self.assertEqual((legacy / "session.json").read_text(), "27")

    def run_hook(self, provider="codex", event="Stop", raw=None, extra=(), **fields):
        payload = {"session_id": "session-1", "hook_event_name": event, **fields}
        return subprocess.run(
            [sys.executable, str(HELPER), "--provider", provider,
             "--petctl", str(self.petctl), "--sequence-dir", str(self.directory / "seq"),
             *extra], input=raw if raw is not None else json.dumps(payload).encode(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
        )

    def records(self):
        return [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def test_provider_states_and_privacy(self):
        for provider in ("codex", "claude"):
            for event, kind in (("UserPromptSubmit", "working"),
                                ("PermissionRequest", "needs_input"),
                                ("Stop", "response_ready"), ("SessionEnd", "info")):
                result = self.run_hook(provider, event, prompt="TOP SECRET",
                                       last_assistant_message="TOP SECRET",
                                       transcript_path="/TOP SECRET")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(json.loads(result.stdout), {})
                record = self.records()[-1]["event"]
                self.assertEqual(record["kind"], kind)
                self.assertEqual(record["source_id"], provider)
                self.assertNotIn("TOP SECRET", json.dumps(record))
                self.assertEqual(len(record["event_id"]), 64)

    def test_claude_notifications_do_not_assert_idle_is_blocked(self):
        for notification, kind in (("permission_prompt", "needs_input"),
                                   ("idle_prompt", "info")):
            result = self.run_hook("claude", "Notification", notification_type=notification)
            self.assertEqual(result.stderr, b"")
            self.assertEqual(self.records()[-1]["event"]["kind"], kind)
        self.assertIn("check-in", self.records()[-1]["event"]["title"])

    def test_explicit_failure_and_interrupt(self):
        self.run_hook("claude", "StopFailure")
        self.assertEqual(self.records()[-1]["event"]["kind"], "failed")
        self.run_hook("codex", "Interrupt")
        self.assertEqual(self.records()[-1]["event"]["kind"], "cancelled")

    def test_unsupported_events_are_noop(self):
        for provider, event in (("codex", "Notification"), ("codex", "StopFailure"),
                                ("claude", "Interrupt"), ("claude", "PostToolUse")):
            result = self.run_hook(provider, event)
            self.assertEqual(result.returncode, 0)
            self.assertEqual(json.loads(result.stdout), {})
        self.assertEqual(self.records(), [])

    def test_invalid_and_oversized_json_are_nonblocking(self):
        for raw in (b"{", b"[]", b"null", b"\xff", b" " * (65536 + 1)):
            result = self.run_hook(raw=raw)
            self.assertEqual(result.returncode, 0)
            self.assertEqual(json.loads(result.stdout), {})
            self.assertTrue(result.stderr)
        self.assertEqual(self.records(), [])

    def test_missing_or_wrong_session_is_nonblocking(self):
        for session in (None, "", 7, "x" * 4097):
            result = self.run_hook(session_id=session)
            self.assertEqual(result.returncode, 0)
            self.assertTrue(result.stderr)
        self.assertEqual(self.records(), [])

    def test_concurrent_delivery_has_persistent_increasing_sequence(self):
        with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
            results = list(pool.map(lambda _: self.run_hook(), range(8)))
        for result in results:
            self.assertEqual(result.stderr, b"")
        records = [row["event"] for row in self.records()]
        self.assertEqual([r["seq"] for r in records], list(range(1, 9)))
        self.assertEqual(len({r["event_id"] for r in records}), 1)
        self.run_hook()
        self.assertEqual(self.records()[-1]["event"]["seq"], 9)

    def test_provider_and_subagent_have_separate_identity(self):
        self.run_hook()
        self.run_hook(agent_id="child")
        self.run_hook("claude")
        records = [r["event"] for r in self.records()]
        self.assertNotEqual(records[0]["event_id"], records[1]["event_id"])
        self.assertEqual([r["seq"] for r in records], [1, 1, 1])

    def test_resident_directory_forwarded_as_one_argument(self):
        path = str(self.directory / "resident state")
        self.run_hook(extra=("--state-dir", path))
        self.assertEqual(self.records()[0]["args"][:4],
                         ["--state-dir", path, "productivity", "--method"])

    def test_missing_petctl_does_not_block_stop_or_leak_payload(self):
        result = self.run_hook(extra=("--petctl", str(self.directory / "missing")),
                               prompt="SECRET")
        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stdout), {})
        self.assertTrue(result.stderr)
        self.assertNotIn(b"SECRET", result.stderr)

    def test_repeated_direct_permission_requests_have_distinct_occurrences(self):
        for provider in ("claude", "codex"):
            self.run_hook(provider, "PermissionRequest", tool_name="Bash")
            self.run_hook(provider, "PermissionRequest", tool_name="Bash")
            records = [r["event"] for r in self.records() if r["event"]["source_id"] == provider]
            self.assertEqual([r["kind"] for r in records], ["needs_input", "needs_input"])
            self.assertNotEqual(records[0].get("occurrence_id"), records[1].get("occurrence_id"))
            self.assertEqual([r["occurrence_id"] for r in records], ["request-1", "request-2"])

    def test_tool_progress_rearms_each_permission_request(self):
        for provider in ("claude", "codex"):
            self.run_hook(provider, "PermissionRequest", tool_name="Bash")
            self.run_hook(provider, "PostToolUse", tool_name="Bash")
            self.run_hook(provider, "PermissionRequest", tool_name="Bash")
            kinds = [r["event"]["kind"] for r in self.records() if r["event"]["source_id"] == provider]
            self.assertEqual(kinds, ["needs_input", "working", "needs_input"])

    def test_progress_events_are_suppressed_unless_resuming_permission(self):
        self.run_hook("claude", "UserPromptSubmit")
        self.run_hook("claude", "PostToolUse", tool_name="Bash")
        self.assertEqual(len(self.records()), 1)
        self.run_hook("claude", "PermissionRequest", tool_name="Bash")
        self.run_hook("claude", "PostToolUse", tool_name="Read")
        self.assertEqual(len(self.records()), 2)
        self.run_hook("claude", "PostToolUseFailure", tool_name="Bash")
        self.assertEqual(self.records()[-1]["event"]["kind"], "working")
        self.run_hook("claude", "PostToolUseFailure", tool_name="Bash")
        self.assertEqual(len(self.records()), 3)

    def test_legacy_integer_counter_upgrades_without_sequence_reset(self):
        self.run_hook("codex", "Stop")
        counter = next((self.directory / "seq").glob("*.json"))
        counter.write_text("17")
        result = self.run_hook("codex", "PermissionRequest")
        self.assertEqual(result.stderr, b"")
        self.assertEqual(self.records()[-1]["event"]["seq"], 18)
        self.assertEqual(json.loads(counter.read_text())["last_kind"], "needs_input")

    def test_failed_tool_progress_remains_retryable(self):
        self.run_hook("codex", "PermissionRequest", tool_name="Bash")
        script = self.petctl.read_text()
        self.petctl.write_text(f"#!{sys.executable}\\nraise SystemExit(9)\\n")
        result = self.run_hook("codex", "PostToolUse", tool_name="Bash")
        self.assertTrue(result.stderr)
        self.petctl.write_text(script)
        result = self.run_hook("codex", "PostToolUse", tool_name="Bash")
        self.assertEqual(result.stderr, b"")
        self.assertEqual([r["event"]["kind"] for r in self.records()], ["needs_input", "working"])
        self.assertEqual(self.records()[-1]["event"]["seq"], 3)

    def test_delayed_claude_permission_notification_cannot_restore_old_wait(self):
        self.run_hook("claude", "PermissionRequest", tool_name="Bash")
        self.run_hook("claude", "Notification", notification_type="permission_prompt")
        self.assertEqual(len(self.records()), 1)
        self.run_hook("claude", "PostToolUse", tool_name="Bash")
        self.run_hook("claude", "Notification", notification_type="permission_prompt")
        self.assertEqual([r["event"]["kind"] for r in self.records()], ["needs_input", "working"])

    def test_nonzero_delivery_is_advisory_and_sequence_is_not_reused(self):
        self.petctl.write_text(f"#!{sys.executable}\nraise SystemExit(9)\n")
        result = self.run_hook()
        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stdout), {})
        self.assertTrue(result.stderr)
        counter = next((self.directory / "seq").glob("*.json"))
        self.assertEqual(json.loads(counter.read_text())["seq"], 1)
        self.run_hook()
        self.assertEqual(json.loads(counter.read_text())["seq"], 2)

    def test_slow_delivery_is_bounded_and_advisory(self):
        self.petctl.write_text(f"#!{sys.executable}\nimport time\ntime.sleep(20)\n")
        result = self.run_hook()
        self.assertEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stdout), {})
        self.assertIn(b"TimeoutExpired", result.stderr)

    def test_corrupt_sequence_file_does_not_reset_or_publish(self):
        self.run_hook()
        counter = next((self.directory / "seq").glob("*.json"))
        counter.write_text("broken")
        result = self.run_hook()
        self.assertEqual(result.returncode, 0)
        self.assertTrue(result.stderr)
        self.assertEqual(len(self.records()), 1)
        self.assertEqual(counter.read_text(), "broken")


if __name__ == "__main__":
    unittest.main()
