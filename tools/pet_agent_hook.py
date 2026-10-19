#!/usr/bin/env python3
"""Advisory Claude Code/Codex lifecycle adapter; never makes hook decisions."""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time


MAX_INPUT = 65536
MAX_SEQ = 2**53 - 1
TIME_BUDGET = 2.0


def translate(provider, payload):
    """Whitelist lifecycle metadata; never copy prompts, tool input or responses."""
    if not isinstance(payload, dict):
        raise ValueError("expected a JSON object")
    session = payload.get("session_id")
    agent = payload.get("agent_id", "")
    if (not isinstance(session, str) or not session or len(session) > 4096
            or not isinstance(agent, str) or len(agent) > 4096):
        raise ValueError("invalid session identity")
    event = payload.get("hook_event_name")
    states = {
        "UserPromptSubmit": ("working", "Working"),
        "PermissionRequest": ("needs_input", "Permission requested"),
        "Stop": ("response_ready", "Response ready"),
        "SessionEnd": ("info", "Session ended"),
        "PostToolUse": ("working", "Continuing after tool"),
    }
    if provider == "claude":
        states["StopFailure"] = ("failed", "Turn failed")
        states["PostToolUseFailure"] = ("working", "Continuing after tool failure")
        if event == "Notification":
            notification = payload.get("notification_type")
            if notification == "permission_prompt":
                states[event] = ("needs_input", "Permission requested")
            elif notification == "idle_prompt":
                states[event] = ("info", "Idle check-in")
    elif provider == "codex":
        states["Interrupt"] = ("cancelled", "Turn interrupted")
    else:
        raise ValueError("unsupported provider")
    if not isinstance(event, str) or event not in states:
        return None
    identity = hashlib.sha256(json.dumps([session, agent], ensure_ascii=True).encode()).hexdigest()
    kind, title = states[event]
    return {"source_id": provider, "event_id": identity, "kind": kind,
            "title": f"{'Claude' if provider == 'claude' else 'Codex'}: {title}"}


@contextmanager
def file_lock(path, deadline):
    """Use a stable lock file: the adjacent counter is replaced atomically."""
    with path.open("a+b") as lock:
        if os.name == "nt":
            import msvcrt
            lock.seek(0, os.SEEK_END)
            if lock.tell() == 0:
                lock.write(b"0")
                lock.flush()

            def acquire():
                lock.seek(0)
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)

            def release():
                lock.seek(0)
                msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)
        else:
            import fcntl

            def acquire():
                fcntl.flock(lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)

            def release():
                fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        while True:
            try:
                acquire()
                break
            except (BlockingIOError, OSError):
                if time.monotonic() >= deadline:
                    raise TimeoutError("sequence lock unavailable") from None
                time.sleep(0.01)
        try:
            yield
        finally:
            release()


def default_sequence_dir():
    if sys.platform == "darwin":
        base = Path.home() / "Library/Application Support"
        current, legacy = base / "Clibby/agent-hooks", base / "Better Pets/agent-hooks"
    elif os.name == "nt":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home()))
        current, legacy = base / "Clibby/agent-hooks", base / "BetterPets/agent-hooks"
    else:
        base = Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local/state"))
        current, legacy = base / "clibby/agent-hooks", base / "better-pets/agent-hooks"
    # Keep existing counters and their locks together; moving them during a hook
    # invocation could reset sequence allocation or race an older adapter.
    return legacy if legacy.exists() else current


def deliver(event, args, payload):
    directory = Path(args.sequence_dir).expanduser() if args.sequence_dir else default_sequence_dir()
    directory.mkdir(mode=0o700, parents=True, exist_ok=True)
    key = event["source_id"] + "-" + event["event_id"]
    counter = directory / (key + ".json")
    deadline = time.monotonic() + TIME_BUDGET
    with file_lock(directory / (key + ".lock"), deadline):
        state = {"seq": 0, "last_kind": "", "pending_tool": "", "direct_permissions": False}
        if counter.exists():
            with counter.open("rb") as stream:
                data = stream.read(1025)
            if len(data) > 1024:
                raise ValueError("invalid sequence store")
            stored = json.loads(data)
            # Upgrade counters written by the original adapter without resetting seq.
            if type(stored) is int:
                state["seq"] = stored
            elif isinstance(stored, dict):
                state = stored
            else:
                raise ValueError("invalid sequence store")
            if (type(state.get("seq")) is not int or not 0 <= state["seq"] < MAX_SEQ
                    or not isinstance(state.get("last_kind"), str)
                    or not isinstance(state.get("pending_tool"), str)
                    or type(state.get("direct_permissions")) is not bool):
                raise ValueError("invalid sequence store")
        hook = payload.get("hook_event_name")
        tool = payload.get("tool_name", "")
        if not isinstance(tool, str) or len(tool) > 128:
            tool = ""
        if hook in ("PostToolUse", "PostToolUseFailure"):
            if (state["last_kind"] != "needs_input"
                    or (state["pending_tool"] and state["pending_tool"] != tool)):
                return
        if (hook == "Notification" and payload.get("notification_type") == "permission_prompt"
                and state["direct_permissions"]):
            # Direct requests are authoritative. Notifications lack a reliable
            # occurrence identity and may arrive after the approved tool finished.
            return
        if hook == "PermissionRequest":
            state["direct_permissions"] = True
        event["seq"] = state["seq"] + 1
        if hook == "PermissionRequest":
            # The direct hook itself observes a fresh attention occurrence even
            # when no intervening tool execution happened (e.g. denied approval).
            event["occurrence_id"] = f"request-{event['seq']}"
        state["seq"] = event["seq"]

        def save_counter():
            fd, temporary = tempfile.mkstemp(prefix=key + ".", dir=directory)
            try:
                with os.fdopen(fd, "w") as stream:
                    json.dump(state, stream)
                    stream.flush()
                    os.fsync(stream.fileno())
                os.replace(temporary, counter)
            finally:
                if os.path.exists(temporary):
                    os.unlink(temporary)
        # Persist allocation before delivery, preserving last successfully sent kind.
        save_counter()
        command = [args.petctl]
        if args.state_dir:
            command += ["--state-dir", args.state_dir]
        command += ["productivity", "--method", "publish", "--json", json.dumps(event)]
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("delivery budget expired")
        # Hold the lock through delivery: allocation alone cannot prevent stale arrivals.
        subprocess.run(command, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL, timeout=remaining, check=True)
        state["last_kind"] = event["kind"]
        state["pending_tool"] = tool if event["kind"] == "needs_input" else ""
        save_counter()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--provider", choices=("claude", "codex"), required=True)
    parser.add_argument("--petctl", default="petctl", help="petctl executable (one argv entry)")
    parser.add_argument("--state-dir", help="resident state directory forwarded to petctl")
    parser.add_argument("--sequence-dir", help="adapter sequence/lock directory")
    args = parser.parse_args()
    try:
        data = sys.stdin.buffer.read(MAX_INPUT + 1)
        if len(data) > MAX_INPUT:
            raise ValueError("hook input exceeds 64 KiB")
        payload = json.loads(data)
        event = translate(args.provider, payload)
        if event:
            deliver(event, args, payload)
    except (ValueError, OSError, subprocess.SubprocessError, RecursionError) as error:
        # Do not disclose payloads, tool commands, transcript paths or petctl output.
        print(f"Clibby hook: event not delivered ({type(error).__name__}).", file=sys.stderr)
    # Codex Stop requires JSON; {} makes no decision in either provider.
    print("{}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
