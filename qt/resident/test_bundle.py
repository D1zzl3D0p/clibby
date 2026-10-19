"""Verify a macOS bundle can launch after relocation without the Qt SDK."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile


def main():
    bundle = Path(sys.argv[1]).resolve()
    frameworks = bundle / "Contents/Frameworks"
    assert (frameworks / "QtWidgets.framework/Versions/A/QtWidgets").is_file(), "bundle is missing QtWidgets; startup still depends on the build SDK"
    assert (bundle / "Contents/PlugIns/platforms/libqcocoa.dylib").is_file(), "bundle is missing the macOS platform plugin"
    with tempfile.TemporaryDirectory(prefix="clibby-relocated-") as temporary:
        relocated = Path(temporary).resolve() / "Clibby.app"
        shutil.copytree(bundle, relocated, symlinks=True)
        executable = relocated / "Contents/MacOS/Clibby"
        environment = {key: value for key, value in os.environ.items()
                       if not key.startswith(("QT_", "QML", "DYLD_"))}
        environment["DYLD_PRINT_LIBRARIES"] = "1"
        help_result = subprocess.run([str(executable), "--help"], cwd=temporary,
                                     env=environment, capture_output=True, text=True, timeout=30)
        assert help_result.returncode == 0, help_result.stderr
        assert "Usage: clibby" in help_result.stdout, help_result.stdout
        qt_loads = [line for line in help_result.stderr.splitlines() if ".framework/" in line and "/Qt" in line]
        assert qt_loads, "no Qt loader evidence collected"
        assert all(str(relocated) in line for line in qt_loads), "Qt libraries loaded outside relocated app: " + repr(qt_loads)
        print("Relocated bundle launches and loads its own Qt frameworks.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
