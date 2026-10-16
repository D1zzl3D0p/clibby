#!/usr/bin/env python3
"""Compare engine pose dumps with the shared reference, rejecting missing/NaN data."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path


def compare_pose_documents(reference: dict, actual: dict, tolerance: float = 0.0001) -> list[str]:
    errors: list[str] = []

    def compare(expected, observed, path):
        if len(errors) >= 100:
            return
        if isinstance(expected, dict):
            if not isinstance(observed, dict):
                errors.append(f"{path}: expected object")
                return
            for key, value in expected.items():
                if key not in observed:
                    errors.append(f"{path}.{key}: missing")
                else:
                    compare(value, observed[key], f"{path}.{key}")
        elif isinstance(expected, list):
            if not isinstance(observed, list) or len(expected) != len(observed):
                errors.append(f"{path}: array length/type mismatch")
                return
            for index, (left, right) in enumerate(zip(expected, observed)):
                compare(left, right, f"{path}[{index}]")
        elif type(expected) in (int, float):
            if type(observed) not in (int, float) or not math.isfinite(observed):
                errors.append(f"{path}: expected finite number")
            elif abs(expected - observed) > tolerance:
                errors.append(f"{path}: {observed} differs from {expected}")
        elif observed != expected:
            errors.append(f"{path}: {observed!r} differs from {expected!r}")

    compare(reference, actual, "fixture")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=Path(__file__).resolve().parents[1] / "shared/golden.json")
    parser.add_argument("--actual", action="append", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    reference = json.loads(args.reference.read_text())
    results = []
    for path in args.actual:
        try:
            errors = compare_pose_documents(reference, json.loads(path.read_text()))
        except (OSError, ValueError) as error:
            errors = [str(error)]
        results.append({"path": str(path), "passed": not errors, "errors": errors})
    report = {"tolerance": 0.0001, "results": results, "passed": all(r["passed"] for r in results)}
    content = json.dumps(report, indent=2, allow_nan=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(content)
    print(content, end="")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
