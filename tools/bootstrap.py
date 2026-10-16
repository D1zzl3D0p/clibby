#!/usr/bin/env python3
"""Install the pinned Qt development toolchain locally on Apple Silicon macOS."""
from __future__ import annotations

import argparse
import json
import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / ".deps"
QT = "6.10.3"


def run(*args: str) -> None:
    subprocess.run(list(args), cwd=ROOT, check=True)


def install_qt(with_3d: bool = False) -> None:
    prefix = DEPS / f"qt/{QT}/macos"
    if not all((prefix / f"lib/cmake/{module}/{module}Config.cmake").exists()
               for module in ("Qt6", "Qt6Quick", "Qt6Qml")):
        run("uv", "run", "--cache-dir", str(ROOT / ".cache/uv"),
            "--with", "aqtinstall==3.3.0", "aqt", "install-qt", "mac", "desktop",
            QT, "clang_64", "--outputdir", str(DEPS / "qt"),
            "--archives", "qtbase", "qtdeclarative")
    if with_3d and not all(
        (prefix / f"lib/cmake/{module}/{module}Config.cmake").exists()
        for module in ("Qt6Quick3D", "Qt6ShaderTools", "Qt6QuickTimeline")
    ):
        run("uv", "run", "--cache-dir", str(ROOT / ".cache/uv"),
            "--with", "aqtinstall==3.3.0", "aqt", "install-qt", "mac", "desktop",
            QT, "clang_64", "--outputdir", str(DEPS / "qt"),
            "--modules", "qtquick3d", "qtshadertools", "qtquicktimeline",
            "--noarchives")
    run(str(prefix / "bin/qtpaths"), "--qt-version")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--with-3d", action="store_true",
                        help="Install optional Quick3D, shader and asset-import modules")
    args = parser.parse_args()
    if platform.system() != "Darwin" or platform.machine() != "arm64":
        parser.error("This bootstrap targets Apple Silicon macOS; use an existing Qt SDK on other platforms.")
    DEPS.mkdir(parents=True, exist_ok=True)
    install_qt(args.with_3d)
    manifest = {"qt": json.loads((ROOT / "tools/dependencies.json").read_text())["qt"]}
    (DEPS / "toolchains.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"bootstrap: {error}", file=sys.stderr)
        raise SystemExit(1)
