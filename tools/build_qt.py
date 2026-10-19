#!/usr/bin/env python3
"""Build the resident app and benchmark in Release and run Qt CTest checks."""
import argparse
import pathlib
import subprocess


def main():
    root = pathlib.Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qt-prefix", type=pathlib.Path, required=True)
    parser.add_argument("--build-dir", type=pathlib.Path, default=root / "build" / "qt")
    args = parser.parse_args()
    prefix = args.qt_prefix.resolve()
    if not (prefix / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file():
        parser.error(f"Qt6Config.cmake not found under {prefix}")
    build = args.build_dir.resolve()
    subprocess.run(["cmake", "-S", str(root / "qt"), "-B", str(build),
                    "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_PREFIX_PATH={prefix}"], check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", "4"], check=True)
    subprocess.run(["ctest", "--test-dir", str(build), "--output-on-failure"], check=True)
    print(build / "pet_qt")
    print(build / "clibby")
    print(build / "character_preview")


if __name__ == "__main__":
    main()
