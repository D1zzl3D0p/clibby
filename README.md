# Better Pets

A lightweight, procedural desktop pet with modular characters, behaviors, and
application integrations. **Qt Quick is the chosen direction.** The C++17 code
now includes a resident application, local session API, `petctl`, grounded IK pixel
characters and interactive 2D/3D previews alongside the preserved benchmark.
Actionable cards, a summoned toolbox and a real command-outcome producer are now
implemented; a coding/chat adapter remains later work. See the
[utility slice record](docs/results/2026-10-08-utility-slice.md).

Start a new session with [the handoff](docs/handoff.md), then read the
[architecture](docs/architecture.md) and [toolkit findings](docs/results/2026-10-08-comparison.md).

The [resident companion specification](docs/superpowers/specs/2026-10-08-resident-companion-design.md)
defines the next development milestones, interfaces, resource requirements, and
acceptance checks. See the [session-input record](docs/results/2026-10-08-session-inputs.md)
for software checks and outstanding live IPC/native acceptance. Monitor crossing
has an open snapping defect in the [foundation record](docs/results/2026-10-08-resident-foundation.md).

## What is here

- `qt/`: C++17 / Qt Quick renderer, procedural motion, resident runtime/host/shell,
  timed benchmark scenarios, and tests.
- `shared/`: authored artwork, rig, and 44 golden poses. This is a frozen test
  fixture, not the future character-package specification.
- `characters/`: editable original soft-creature and werewolf art, versioned
  data-only manifests, atlases, contact anchors and attribution.
- `bench/`: CPU/RSS profiler for one build or paired build comparisons.
- `tools/`: local Qt setup, build, fixture generation, config, and pose verification.
- `docs/evidence/native-screening/`: preserved native benchmark data.
- `docs/archive/toolkit-study/`: compact source/validation archive of the retired
  Godot-versus-Qt experiment, including original plans and review records.

No service, account, model inference, or cloud dependency is needed. Native
behavior has only been exercised on Apple Silicon macOS. Windows/Linux support
and battery-runtime impact remain unverified.

## Setup and test

The bootstrap targets Apple Silicon macOS. Requires Python 3.12+, `uv`, CMake,
and Apple's command-line C++ tools. It installs Qt 6.10.3 locally. An existing
Qt installation can be passed to the build script instead.

```sh
rtk uv run --cache-dir .cache/uv python tools/bootstrap.py
rtk proxy .venv/bin/python -m unittest discover -s tests -v
rtk proxy .venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
```

The build script uses Release and runs all CTest targets. Dependencies and build
outputs stay in `.deps/`, `.venv/`, `.cache/`, and `build/`; these are ignored.
Qt archive provenance is in `tools/dependencies.json` and `.deps/toolchains.json`.
Check module licenses before product distribution.

## Run the resident pet

From a normal Terminal with desktop access:

```sh
rtk proxy build/qt/better_pets
```

No benchmark arguments are required. The default is Mallow, the soft creature.
Use the Character menu to choose Bramble, the werewolf. Packages are copied beside
the binary at build time; preferences and the instance lock use the per-user app
data directory. Restart restores the selected character within this catalog.
For isolated development state:

```sh
rtk proxy build/qt/better_pets --character "$PWD/characters/soft-creature" --state-dir "$PWD/results/resident-dev-state"
```

An explicit `--character` overrides the remembered selection. `--assets "$PWD/shared"`
retains the original fixture path for regression/development; use either flag.

Left-drag to move the pet. Use the menu-bar icon or right-click context menu for
Show/Hide, Pause/Resume and Quit. Visibility and pause are independent; restarting
launches visibly and retains pause and placement. A duplicate launch exits
successfully. The pet rests between brief gestures; hidden or paused states stop
animation ticks. Session expiry still runs when needed. The Sessions menu lists
current inputs and lets you acknowledge completion, attention requests and failures.
Toolbox… opens the shared status-card inbox and existing pet controls. Cards offer
Open source separately from Acknowledge. Its Travel speed control ranges from
0.5× to 3×, starts at 2× for new settings, and saves your choice across restarts.
The body follows the requested speed smoothly; feet adapt their steps to catch
up within leg reach. Feet retain exact planted contacts and alternate complete swings, including
at faster travel speeds. Changing speed keeps the current walking target.
Dragging adds bounded local body/head/ear/tail follow-through; it does not throw
the window. Pause suppresses these reactions too. Settled springs stop their
animation deadlines. Walk left/right and Stop menu commands drive grounded 2D pets and the 3D werewolf along
the selected display floor with planted feet and generated steps.

Normal desktop startup fails clearly if persistent menu-bar controls are unavailable.
Offscreen software tests omit tray UI. Native focus, click-through, real displays,
and actual sleep/wake still require the [manual checklist](docs/results/2026-10-08-resident-foundation.md).
The real coding-tool adapter is not implemented. The new art is an editable first
iteration. Autonomous wandering remains deferred.

Marmalade 3D adds the selected woodland cat art with four procedural legs,
continuous skinned limbs and a 96×96 nearest-rendered source. Select it from
Character or start the rebuilt resident with:

```sh
rtk proxy build/qt/better_pets --character "$PWD/characters/marmalade-3d"
```

See the [spatial quadruped record](docs/results/2026-10-16-spatial-quadruped.md)
for verification and the remaining desktop acceptance work.

The live werewolf is selectable through Character → werewolf-3d in a
Quick3D-enabled build. To start it explicitly:

```sh
rtk proxy build/qt/better_pets --character "$PWD/characters/werewolf-3d"
```

It shares walking controls, speed, saved placement, notifications and Toolbox
with the 2D pets. Its resident presentation uses a three-quarter facing bias
and a slight top-down orthographic camera. Its transparent window follows the model and native placement
feeds back into floor contacts. Software-only builds retain the current 2D pet
and explain why 3D is unavailable. The renderer uses trusted deployed assets in
`build/qt/better-pets-3d`; keep that folder beside the executable when moving it.
See the [resident 3D record](docs/results/2026-10-10-resident-3d.md) for validation
and remaining native acceptance checks.

The first expansion also includes Pip, Biscuit, Clover, Mossback, Rivet, Paper
Heron, Acorn Custodian, Puddle, Pocket Observatory, Night Librarian, Sprig,
Hearthling and Crumb Golem as selectable 3D base pets. Each uses shared procedural
movement with its own original anatomy; proposed signature actions remain
separate work. Pebblekin, Margin, Lantern Snail and Velvet currently have working
experimental motion studies outside the selector. See the
[per-pet implementation status](docs/results/2026-10-16-catalogue-base-implementation.md).

## Notify from real work

With the resident running, wrap a command to report its actual outcome:

```sh
rtk proxy .venv/bin/python tools/pet_run.py --label "Project tests" -- .venv/bin/python -m unittest discover -s tests
```

Open source returns to the current project folder by default; `--source-url` can
supply a supported web link. Acknowledge quiets the card without changing the
reported outcome. See [producer options](docs/api/command-producer.md) and the
[verification limits](docs/results/2026-10-08-utility-slice.md). Live IPC delivery
still needs a normal Terminal trial; software tests here do not establish native
click/focus behavior or battery impact.

## Polish and compare characters

```sh
rtk proxy build/qt/character_preview
rtk proxy build/qt/character_preview --character "$PWD/characters/werewolf"
```

Load a local character directory, select an action and compare direct retained
2D with experimental low-resolution composition. Controls include 64/96/128-pixel
canvases, nearest/linear piece filtering, palette/dither, travel speed, pause/reset and
bone/contact overlays. Both previews offer Travel speed from 0.5× to 3×, with
2× as the default and 1× for shorter steps. Captures retain the compatible
`--speed MULTIPLIER` option for this body travel setting. Feet follow the body with adaptive steps; leg reach
limits foot placement rather than stopping body movement.
Reaction and capture time remain unchanged.
Move the pointer in the preview for gaze; click to apply
a ground target for Mallow (Ctrl-click applies an impulse). Grounded controls
include target position and follower frequency, damping and response. Mallow
anticipates steps and turns its quarter-view silhouette over 0.18 seconds while
keeping planted feet fixed. The small and large views show actual output pixels and integer
physical-pixel enlargement. Native mixed-DPI behavior still needs checking.

Grounded Mallow defaults to CPU composition with nearest sampling and no palette
dither. Bramble retains the direct 2D default and reuses its atlas and scene nodes.
Composition currently uses CPU painting and uploads a new texture when visual
state changes. Palette, dither or debug overlays select that experimental path,
even when the mode selector says direct. It is a visual comparison tool, not a
verified low-power GPU renderer. The preview animates continuously for inspection;
it does not model the resident's sparse quiet schedule.

For a controlled software capture (a PNG and configuration sidecar):

```sh
rtk proxy mkdir -p results/pixel-preview
rtk proxy env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  build/qt/character_preview --character "$PWD/characters/soft-creature" \
  --capture "$PWD/results/pixel-preview/mallow.png" --action celebrate --time 0.4 \
  --mode composited --resolution 96 --dither --impulse 1,0 --impulse-strength 2
```

Other capture options include `--gaze X,Y`, `--palette`, `--linear`, `--debug`
and a bounded live `--duration SECONDS`. Grounded captures accept `--target X`
and `--reverse-at SECONDS`, and export realized roots and contact state. Interactive preview sessions need not
be repeatable; controlled captures and core tests make regressions inspectable.

Edit the authored pieces and regenerate with
`rtk proxy .venv/bin/python tools/author_characters.py`. See the
[character guide](characters/README.md), [format](docs/characters-v1.md) and
[implementation record](docs/results/2026-10-08-pixel-characters.md).
The new [grounded implementation record](docs/results/2026-10-08-grounded-characters.md)
includes motion evidence and verification limits.

For the separate original Blender werewolf, install the optional SDK modules and
rebuild, then launch its preview:

```sh
rtk uv run --cache-dir .cache/uv python tools/bootstrap.py --with-3d
rtk proxy .venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
rtk proxy build/qt/characters3d/character_preview_3d
```

Click for a ground target; Left/Right move, Space stops, I applies force and P
pauses; D toggles contact markers. Add `--target 60` to launch it walking.
The [3D guide](qt/characters3d/README.md) documents captures and rendering.
The editable [Blender source](characters/werewolf-3d/werewolf.blend), skinned GLB,
original atlas and review renders are in `characters/werewolf-3d/`. Blender runs
only during authoring. The 2D build remains available with
`-DBETTER_PETS_WITH_3D=OFF`; missing optional modules leave 2D independently usable.
Native 3D pixels and resource comparison still need desktop validation before
resident integration.

## Try session notifications

With the resident running, use the same `--state-dir` for both programs if you
overrode its default. Each session needs increasing sequence numbers:

```sh
rtk proxy build/qt/petctl update --source demo --session build --label "Build" --seq 1 --status working
rtk proxy build/qt/petctl update --source demo --session build --label "Build" --seq 2 --status needs_input
rtk proxy build/qt/petctl state
rtk proxy build/qt/petctl clear --source demo --session build --seq 3
```

The pet shows plain status text and a short gesture. Pause freezes its pose and
label while the menu stays current. Hide retains incoming data. Expiry removes
stale sessions; sleep/wake clears them. Reactions never move the pet window.

For the explicitly **simulated** two-source example:

```sh
rtk proxy .venv/bin/python examples/simulated_sessions.py --petctl build/qt/petctl
```

This is a generic integration interface, not a connection to a real AI tool.
The [API contract](docs/api/session-v1.md) documents framing, schemas, limits,
permissions, responses and CLI exit codes. Live socket tests require an environment
that permits Unix socket binding; restricted execution reports those checks skipped.

## Try the benchmark

Run from a normal Terminal with desktop access, in this project directory:

```sh
rtk proxy build/qt/pet_qt --scenario idle-motion --duration 60 \
  --assets "$PWD/shared" --output /tmp/better-pets-preview.json
```

Drag with the left mouse button; right-click to quit. The prototype exits after
the requested duration. `walk-desktop` deliberately moves its window;
`idle-motion` animates in place unless dragged. `hidden` starts hidden and exits
on its timer; it is not a visible-to-hidden lifecycle test or tray application.

For an untimed offscreen validation on a host without desktop access:

```sh
rtk proxy env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  build/qt/pet_qt --scenario static --duration 1 \
  --assets "$PWD/shared" --output /tmp/better-pets-validation.json \
  --screenshot /tmp/better-pets.png
```

Offscreen validation does not measure desktop performance.

## Profile resource use

```sh
rtk proxy .venv/bin/python tools/configure.py --scenarios static idle-motion walk-desktop hidden
rtk proxy .venv/bin/python -m bench.run \
  --config bench/config.local.json --output results/qt-screening \
  --warmup 5 --duration 20 --baseline-duration 5 --cooldown 1 --repeats 3
```

Use a new output directory for each experiment. Leave the pet untouched during
timing. The configuration defaults to one Qt candidate with no-pet baselines;
the [profiler guide](bench/README.md) explains multi-build comparisons, scenarios,
and interpretation. `tools/configure.py --validation-only` generates an offscreen
pipeline check, never a desktop benchmark.

Inspect valid-run counts and errors in the generated summary. A successful
profiler exit means the report was written, not that every application trial passed.

CPU percentages use 100% for one logical core. RSS is not physical footprint.
Neither metric proves battery-runtime loss. The proposed under-5% battery target
still requires controlled whole-system energy measurements.

## Verify motion

```sh
rtk proxy build/qt/pet_qt --scenario static --duration 1 \
  --assets "$PWD/shared" --output /tmp/better-pets-unused.json \
  --dump-poses /tmp/better-pets-poses.json
rtk proxy .venv/bin/python tools/verify.py --actual /tmp/better-pets-poses.json
```

The [fixture contract](shared/CONTRACT.md) defines coordinates, motion equations,
pause/resume rules and pose tolerance. [The fixture preview](shared/preview.png)
is simple authored geometry, not the intended final character design.
