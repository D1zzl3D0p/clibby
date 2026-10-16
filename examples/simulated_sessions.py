#!/usr/bin/env python3
"""SIMULATED input only: two independent sources, all four session statuses."""
import argparse
import json
from pathlib import Path
import subprocess
import time
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--petctl", default="build/qt/petctl", help="path to compiled petctl")
    parser.add_argument("--state-dir", help="absolute resident state directory")
    args = parser.parse_args()
    if args.state_dir and not Path(args.state_dir).is_absolute():
        parser.error("--state-dir must be absolute")
    base = [args.petctl]
    if args.state_dir:
        base += ["--state-dir", args.state_dir]
    sequences = {"simulated-build": 0, "simulated-review": 0}
    session_id = "demo-" + uuid.uuid4().hex

    def call(*options):
        result = subprocess.run(base + list(options), capture_output=True, text=True, timeout=4)
        if result.returncode:
            raise RuntimeError(result.stderr.strip() or result.stdout.strip() or f"petctl exit {result.returncode}")
        value = json.loads(result.stdout)
        print(json.dumps(value, ensure_ascii=False), flush=True)
        return value

    def update(source, label, status):
        sequences[source] += 1
        call("update", "--source", source, "--session", session_id, "--label", label,
             "--seq", str(sequences[source]), "--status", status, "--ttl", "30")

    print("SIMULATED sessions: no coding-tool or build events are being observed.", flush=True)
    try:
        update("simulated-build", "Example compilation", "working")
        update("simulated-review", "Example review", "working")
        call("state")
        time.sleep(2)
        update("simulated-review", "Example review asks a question", "needs_input")
        call("state")
        time.sleep(2)
        update("simulated-build", "Example compilation finished", "completed")
        call("state")
        time.sleep(2)
        update("simulated-review", "Example review failed", "failed")
        call("state")
        time.sleep(2)
    finally:
        for source in sequences:
            sequences[source] += 1
            try:
                call("clear", "--source", source, "--session", session_id, "--seq", str(sequences[source]))
            except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
                print(f"Could not clear simulated session {source}: {error}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        raise SystemExit(f"Simulated example failed: {error}")
