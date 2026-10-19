# Contributing to Clibby

Clibby is an early desktop companion prototype. Start with the [README](README.md)
for setup and the [latest handoff](docs/handoff.md) for current status. The
[architecture](docs/architecture.md) explains the runtime boundaries. Detailed
[resident usage](docs/resident-guide.md), [preview](docs/character-preview.md),
and [benchmark](docs/benchmark.md) guides keep command examples outside the
quick start; [research reports and notes](docs/research/README.md) live together.

## Development checks

From the repository root, after setup:

```sh
.venv/bin/python -m unittest discover -s tests -v
.venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
```

The build helper configures Release, builds, and runs CTest. Use `--build-dir`
with a fresh directory under `build/` after moving the checkout: CMake caches
contain absolute paths. For an existing build, rerun its checks with:

```sh
ctest --test-dir build/qt --output-on-failure
```

Qt Quick 3D is optional; install it with
`.venv/bin/python tools/bootstrap.py --with-3d` to exercise the full resident
catalogue. For a 2D-only build:

```sh
cmake -S qt -B build/qt-2d -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/.deps/qt/6.10.3/macos" -DCLIBBY_WITH_3D=OFF
cmake --build build/qt-2d --parallel 4
ctest --test-dir build/qt-2d --output-on-failure
```

## Scope and evidence

- Keep character artwork, anatomy and behavior modular. Preserve the frozen
  `shared/` fixture and its motion contract.
- Include editable sources, exported assets, and provenance together when
  changing artwork. See [character authoring](characters/README.md) and
  [brand asset generation](docs/brand.md).
- Preserve raw evidence in `docs/evidence/` and `docs/archive/`. Historical names,
  paths and measurements in those records describe the original experiment.
- Put new captures, state directories and benchmark output in ignored `results/`
  directories with new run names. Keep SDKs, build output and credentials local.
- Report actual checks and skips. Offscreen tests and CPU/RSS measurements do
  not establish desktop parity or battery impact. Native display, focus,
  monitor/Spaces and energy checks remain separate.

Describe the problem, resulting behavior, and validation in a proposed change.
Avoid mixing unrelated character or runtime work into repository housekeeping.

## Licensing

Character packages include their own license and attribution files. There is
currently no repository-wide license grant for the remaining source or brand
assets. Preserve existing notices; do not assume package licenses cover the
whole repository. Qt and other dependencies retain their own licenses.
