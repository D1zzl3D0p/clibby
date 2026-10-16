#!/usr/bin/env python3
"""Run a command and publish its explicit outcome to Better Pets."""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import unicodedata
import uuid


def publish(args, status, seq):
    """Notification transport failure must not change the command's outcome."""
    command = [args.petctl, "update", "--source", args.source,
               "--session", args.session, "--label", args.label,
               "--seq", str(seq), "--status", status, "--ttl", str(args.ttl),
               "--source-url", args.source_url]
    if args.state_dir:
        command += ["--state-dir", args.state_dir]
    try:
        result = subprocess.run(command, capture_output=True, text=True, errors="replace", timeout=4)
        if result.returncode:
            detail = result.stderr.strip() or result.stdout.strip() or f"exit {result.returncode}"
            print(f"pet-run: could not publish {status}: {detail}", file=sys.stderr)
    except (OSError, subprocess.TimeoutExpired) as error:
        print(f"pet-run: could not publish {status}: {error}", file=sys.stderr)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--petctl", default=str(Path(__file__).resolve().parents[1] / "build/qt/petctl"))
    parser.add_argument("--state-dir", help="absolute resident state directory")
    parser.add_argument("--source", default="command")
    parser.add_argument("--session", default="run-" + uuid.uuid4().hex)
    parser.add_argument("--label", help="plain-text command label, at most 120 characters")
    parser.add_argument("--source-url", default=Path.cwd().resolve().as_uri(), help="source target for the resident's Open source action")
    parser.add_argument("--ttl", type=int, default=3600, help="retention in seconds (1..3600); no periodic refresh")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- COMMAND [ARG ...]")
    args = parser.parse_args(argv)
    if args.command[:1] == ["--"]:
        args.command = args.command[1:]
    if not args.command:
        parser.error("a command is required after --")
    if not all(re.fullmatch(r"[A-Za-z0-9._-]{1,64}", value) for value in (args.source, args.session)):
        parser.error("source/session IDs require 1..64 ASCII letters/digits/._-")
    if args.state_dir and not Path(args.state_dir).is_absolute():
        parser.error("--state-dir must be absolute")
    if not 1 <= args.ttl <= 3600:
        parser.error("--ttl must be 1..3600")
    if args.label is None:
        args.label = Path(args.command[0]).name
    if len(args.label) > 120 or any(unicodedata.category(c) == "Cc" for c in args.label):
        parser.error("--label must have at most 120 characters and no controls")

    publish(args, "working", 1)
    try:
        result = subprocess.run(args.command)
        code = result.returncode if result.returncode >= 0 else 128 - result.returncode
    except FileNotFoundError as error:
        print(f"pet-run: {error}", file=sys.stderr)
        code = 127
    except OSError as error:
        print(f"pet-run: {error}", file=sys.stderr)
        code = 126
    except KeyboardInterrupt:
        code = 130
    publish(args, "completed" if code == 0 else "failed", 2)
    return code


if __name__ == "__main__":
    sys.exit(main())
