"""Actual resident and CLI tests; offscreen validates software, not desktop behavior."""
import json
import os
from pathlib import Path
import socket
import re
import subprocess
import sys
import tempfile
import time
import errno


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--cli-only":
        petctl = sys.argv[2]
        # Exercise the published regexes themselves, without adding a schema
        # validator dependency. These patterns use a shared Python/ECMA subset.
        schema = json.loads((Path(__file__).resolve().parents[2] /
                             "docs/api/session-v1.schema.json").read_text())
        for name, valid, invalid in (("sessionId", "build", "build\n"),
                                     ("label", "Build", "Build\n")):
            pattern = schema["$defs"][name]["pattern"]
            assert re.search(pattern, valid) is not None, (name, valid)
            assert re.search(pattern, invalid) is None, (name, invalid)
        with tempfile.TemporaryDirectory() as directory:
            def run(options, code):
                result = subprocess.run([petctl, *options, "--state-dir", directory],
                                        capture_output=True, text=True, timeout=3)
                assert result.returncode == code, (options, result.returncode, result.stdout, result.stderr)
                return result
            run([], 2)
            run(["bogus"], 2)
            run(["state", "--seq", "1"], 2)
            run(["state", "--unknown", "value"], 2)
            run(["state", "--state-dir", "relative"], 2)
            run(["update", "--source", "x"], 2)
            base = ["update", "--source", "x", "--session", "one", "--label", "test", "--seq", "1", "--status", "working"]
            for key, value in (("--source", "bad space"), ("--seq", "9007199254740992"),
                               ("--seq", "-1"), ("--seq", "1.5"), ("--status", "unknown"),
                               ("--label", "x" * 121), ("--label", "line\nbreak"),
                               ("--source", "build\n"), ("--session", "one\n"), ("--seq", "1\n")):
                case = base.copy(); case[case.index(key) + 1] = value; run(case, 2)
            for url in ("https://example.com/task", "http://localhost:8080/task", "file:///tmp/task", ""):
                run(base + ["--source-url", url], 3)
            for url in ("relative", "javascript:alert(1)", "file://server/tmp/task", "https://user@example.com/",
                        "https://example.com/%0A", "https://example.com/\n", "https://example.com/%ZZ", "x" * 2049):
                run(base + ["--source-url", url], 2)
            run(["state", "--source-url", "https://example.com/"], 2)
            run(["clear", "--source", "x", "--session", "one", "--seq", "1", "--source-url", "https://example.com/"], 2)
            run(base + ["--ttl", "0"], 2)
            run(base + ["--ttl", "3601"], 2)
            run(base + ["--ttl", "120\n"], 2)
            run(["clear", "--source", "x", "--session", "one", "--seq", "1", "--status", "working"], 2)
            run(["productivity", "--method", "list", "--json", "{}"], 3)
            run(["productivity", "--method", "list", "--json", "[]"], 2)
            run(["productivity", "--method", "bogus", "--json", "{}"], 2)
            run(["state"], 3)
            run(base, 3)
            result = run(["--help"], 0)
            assert "Exit codes" in result.stdout
        print("CLI argument/error tests passed")
        return 0
    resident, petctl, assets = sys.argv[1:]
    environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software")
    with tempfile.TemporaryDirectory(prefix="pets-ipc-") as directory:
        probe = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            probe.bind(str(Path(directory) / "bind-probe"))
        except OSError as error:
            if error.errno == errno.EPERM:
                print("SKIP: OS Unix bind denied with EPERM; resident CLI sockets unverified")
                return 77
            raise
        finally:
            probe.close()
        state = str(Path(directory).resolve())
        process = subprocess.Popen([resident, "--assets", assets, "--state-dir", state], env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            def cli(*args, code=0):
                result = subprocess.run([petctl, *args, "--state-dir", state], env=environment,
                                        capture_output=True, text=True, timeout=3)
                assert result.returncode == code, (args, result.returncode, result.stdout, result.stderr)
                return json.loads(result.stdout) if code in (0, 4) and result.stdout.strip() else None

            deadline = time.monotonic() + 3
            while True:
                result = subprocess.run([petctl, "state", "--state-dir", state], env=environment,
                                        capture_output=True, text=True, timeout=3)
                if result.returncode == 0:
                    break
                assert process.poll() is None, process.communicate()
                assert time.monotonic() < deadline, result.stderr
                time.sleep(0.02)
            assert cli("state")["sessions"] == []
            # Real productivity API/CLI round trip and both hook translators.
            event = {"source_id": "example", "event_id": "review", "seq": 1,
                     "kind": "needs_input", "title": "Review needs attention"}
            published = cli("productivity", "--method", "publish", "--json", json.dumps(event))
            assert published["item"]["kind"] == "needs_input", published
            listed = cli("productivity", "--method", "list", "--json", '{"attention_only":true}')
            assert len(listed["items"]) == 1, listed
            action = {"source_id": "example", "event_id": "review", "seq": 1,
                      "action": "acknowledge"}
            seen = cli("productivity", "--method", "act", "--json", json.dumps(action))
            assert seen["item"]["acknowledged"] and not seen["item"]["completed"], seen
            helper = Path(__file__).resolve().parents[2] / "tools/pet_agent_hook.py"
            for provider in ("claude", "codex"):
                for hook_event in ("UserPromptSubmit", "PermissionRequest", "Stop"):
                    delivered = subprocess.run([sys.executable, str(helper),
                        "--provider", provider, "--petctl", petctl, "--state-dir", state,
                        "--sequence-dir", str(Path(state) / "hook-sequences")],
                        input=json.dumps({"session_id": "real-replay", "hook_event_name": hook_event,
                                          "tool_name": "Bash"}), text=True, capture_output=True, timeout=5)
                    assert delivered.returncode == 0 and delivered.stdout.strip() == "{}" and not delivered.stderr, delivered
                items = cli("productivity", "--method", "list",
                            "--json", json.dumps({"query": provider}))["items"]
                assert len(items) == 1 and items[0]["kind"] == "response_ready", items
            # Clear transient bridge state so legacy lifecycle assertions stay independent.
            for record in cli("state")["sessions"]:
                cli("clear", "--source", record["source_id"], "--session", record["session_id"],
                    "--seq", str(record["seq"] + 1))
            # Actual child exit -> wrapper -> petctl -> resident session round trip.
            wrapper = Path(__file__).resolve().parents[2] / "tools/pet_run.py"
            for session, exit_code, status in (("success", 0, "completed"), ("failure", 7, "failed")):
                wrapped = subprocess.run([sys.executable, str(wrapper), "--petctl", petctl,
                    "--state-dir", state, "--source", "command", "--session", session,
                    "--label", "Real child", "--source-url", "file:///tmp", "--",
                    sys.executable, "-c", f"raise SystemExit({exit_code})"],
                    capture_output=True, text=True, timeout=12)
                assert wrapped.returncode == exit_code and not wrapped.stderr, wrapped
                record = next(r for r in cli("state")["sessions"] if r["session_id"] == session)
                assert record["status"] == status and record["seq"] == 2 and record["source_url"] == "file:///tmp", record
                cli("clear", "--source", "command", "--session", session, "--seq", "3")
            for source, status in (("build", "working"), ("review", "needs_input")):
                reply = cli("update", "--source", source, "--session", "one", "--label", source,
                            "--seq", "1", "--status", status, "--ttl", "2",
                            "--source-url", "https://example.com/" + source)
                assert reply == {"seq": 1, "ttl": 2}, reply
            snapshot = cli("state")
            assert len(snapshot["sessions"]) == 2, snapshot
            assert snapshot["foreground"]["source_id"] == "review", snapshot
            assert snapshot["foreground"]["source_url"] == "https://example.com/review", snapshot
            assert all(record["source_url"] == "https://example.com/" + record["source_id"]
                       for record in snapshot["sessions"]), snapshot
            assert cli("update", "--source", "build", "--session", "one", "--label", "build",
                       "--seq", "1", "--status", "completed", code=4)["error"]["code"] == -32003
            for seq, status in ((2, "completed"), (3, "failed")):
                cli("update", "--source", "build", "--session", "one", "--label", "build",
                    "--seq", str(seq), "--status", status, "--ttl", "2")
            assert cli("clear", "--source", "review", "--session", "one", "--seq", "2")["cleared"]
            assert len(cli("state")["sessions"]) == 1
            cli("update", "--source", "bad space", "--session", "one", "--label", "bad",
                "--seq", "1", "--status", "working", code=2)
            cli("state", "--seq", "1", code=2)
            cli("update", "--source", "x", code=2)
            cli("state", "--state-dir", "relative", code=2)
            help_result = subprocess.run([petctl, "--help"], capture_output=True, text=True, timeout=3)
            assert help_result.returncode == 0 and "Exit codes" in help_result.stdout
            with tempfile.TemporaryDirectory() as missing:
                result = subprocess.run([petctl, "state", "--state-dir", missing], capture_output=True,
                                        text=True, timeout=3)
                assert result.returncode == 3, result
            time.sleep(2.05)
            assert cli("state")["sessions"] == []
        finally:
            process.terminate()
            try:
                process.communicate(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.communicate(timeout=2)
    print("Resident CLI/socket integration passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
