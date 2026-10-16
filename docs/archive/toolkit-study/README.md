# Retired toolkit study

This archive preserves the Godot/Qt experiment before the Qt-focused cleanup.
It is historical evidence, not the active implementation plan. The current
[findings](../../results/2026-10-08-comparison.md) include the user's later visual
observations and confirmation of dragging during the benchmark.

`source-and-validation.tar.gz` contains 392 original files: both implementations,
shared fixtures, Python tools/tests, setup/configuration, the original README,
plans/reviews, Qt validation images, toolchain metadata, and all raw benchmark
records. Large downloaded toolchains, exported applications, virtual environments
and build caches are deliberately excluded. Source/setup versions are retained
for a future reconstruction, not a guarantee upstream downloads remain available.

`manifest.json` records the archive SHA256 and SHA256 of every original file.
Every member was compared byte-for-byte with its source before cleanup. The
native screening files are also retained unpacked under
[`docs/evidence/native-screening`](../../evidence/native-screening/summary.md).
Their hashes use the original `results/native-screening/` prefix in the manifest.

To inspect without extracting:

```sh
rtk proxy tar -tzf docs/archive/toolkit-study/source-and-validation.tar.gz
```

If restoration is needed, extract into a separate scratch directory, not over
the active Qt project. Old commands and embedded absolute paths refer to the
original study layout. The archive has no current production authority.
