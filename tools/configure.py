#!/usr/bin/env python3
"""Resolve the Qt executable into a native or validation-only profiling config."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "bench/config.local.json")
    parser.add_argument("--validation-only", action="store_true",
                        help="Use Qt offscreen software rendering; never a desktop performance result")
    parser.add_argument("--scenarios", nargs="+", choices=("static", "idle-motion", "walk-fixed",
                        "walk-desktop", "hidden", "mixed-active", "mixed-rest"))
    args = parser.parse_args()
    example = ROOT / "bench/config.example.json"
    config = json.loads(example.read_text())
    config["assets"] = str(ROOT / "shared")
    if args.scenarios:
        config["scenarios"] = args.scenarios
    for candidate in config["candidates"]:
        candidate["argv"][0] = str((example.parent / candidate["argv"][0]).resolve())
    qt = config["candidates"][0]
    if args.validation_only:
        qt["name"] = "qt-offscreen-validation"
        qt["environment"] = {"QT_QPA_PLATFORM": "offscreen", "QT_QUICK_BACKEND": "software"}
        qt["renderer"] = "offscreen software; validation only"
        config["environment"]["purpose"] = "pipeline validation, not desktop resource measurement"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(config, indent=2) + "\n")
    print(args.output)


if __name__ == "__main__":
    main()
