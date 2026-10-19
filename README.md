# Clibby

<img src="assets/brand/app-icon-128.png" width="96" height="96" alt="Clibby — the crescent C and companion head mark">

A lightweight, procedural desktop companion built with C++17 and Qt Quick.
Modular 2D/3D characters share grounded movement, quiet autonomous behavior,
signature actions, and a local integration interface. A toolbox and persistent
attention inbox support tasks, timers, Pomodoro, command outcomes, and opt-in
Claude Code/Codex hooks.

No service, account, model inference, or cloud dependency is needed.

## Status

Clibby is an early development build exercised on Apple Silicon macOS.
Windows/Linux support and battery-runtime impact remain unverified. Broader
native focus, display/Spaces behavior and installed hook delivery still need
acceptance testing. The macOS app includes its Qt runtime; public distribution
signing and notarization remain separate work.

The [latest handoff](docs/handoff.md) records implemented stages, verification
and open checks. It supersedes historical proposals and test counts.

## Setup and test

Requires Python 3.12+, `uv`, CMake, and Apple's command-line C++ tools.
The bootstrap targets Apple Silicon macOS and installs Qt 6.10.3 locally.
Run from the repository root:

```sh
uv run --cache-dir .cache/uv python tools/bootstrap.py --with-3d
.venv/bin/python -m unittest discover -s tests -v
.venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
```

Omit `--with-3d` for the base SDK. An existing Qt installation can be supplied
with `--qt-prefix`. The build helper configures Release, builds and runs CTest.
SDKs, environments, build outputs and new captures stay in ignored directories.
See [contributing](CONTRIBUTING.md) for 2D-only builds and rebuilding after moving
a checkout. RTK, used in development notes, is an optional command wrapper.

## Run the resident pet

Open the built app from a normal Terminal with desktop access:

```sh
open build/qt/Clibby.app
```

Or run `build/qt/clibby`. Drag to move the pet; use the menu-bar icon or context
menu for character selection, Show/Hide, Pause/Resume, Toolbox and Productivity.
The default character is Mallow; optional 3D builds include the expanded creature
and companion catalogue. Preferences retain character selection and placement.

See the [resident guide](docs/resident-guide.md) for controls, development state,
character options, command notifications and session examples.

## Guides and project layout

- [Character previews and captures](docs/character-preview.md),
  [character authoring](characters/README.md), and [package format](docs/characters-v1.md).
- [Session API](docs/api/session-v1.md), [productivity API](docs/api/productivity-v1.md),
  and [agent hook setup](docs/api/agent-hooks.md).
- [Benchmark and motion validation](docs/benchmark.md) and
  [profiler reference](bench/README.md). CPU/RSS and offscreen checks do not
  establish battery impact or desktop parity.
- [Architecture](docs/architecture.md), [visual identity](docs/brand.md),
  and [research reports and notes](docs/research/README.md).

Runtime and tests live in `qt/`; editable character packages in `characters/`;
build and authoring tools in `tools/`; production brand assets in `assets/brand/`.
`art-studies/` and `experiments/` hold art explorations and motion prototypes.
`shared/` is a frozen benchmark fixture. Historical measurements and the retired
toolkit study remain in `docs/evidence/` and `docs/archive/`.

## Licensing and provenance

Clibby's source and original brand assets are licensed under the [MIT License](LICENSE).
Character packages retain their individual licenses and attribution; Qt and other
dependencies retain their own licenses. See [contributing](CONTRIBUTING.md#licensing)
and preserve existing notices when redistributing. Historical research and evidence
retain the names and metadata used when recorded.
