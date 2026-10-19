# Character previews and captures

Interactive 2D/3D inspection and reproducible software captures.

Run commands from the repository root after following the [setup instructions](../README.md#setup-and-test).

## Polish and compare characters

```sh
build/qt/character_preview
build/qt/character_preview --character "$PWD/characters/werewolf"
```

Load a local character directory, select an action and compare direct retained
2D with experimental low-resolution composition. Controls include 64/96/128-pixel
canvases, nearest/linear piece filtering, palette/dither, travel speed, pause/reset and
bone/contact overlays. Both previews offer Travel speed from 0.5× to 3×, with
2× as the default and 1× for a slower walk. Captures retain the compatible
`--speed MULTIPLIER` option. Spatial companions retain the original travel pace;
measured leg reach adjusts cadence, contact recovery and reachable body height.
Large spatial reversals use a finite directional leading plant before travel
resumes. Bipeds plant the leading foot before the torso follows; quadrupeds take
an independent step into the new heading, plant with the body held, then transfer
through the remaining fore/hind placements. Ordinary turns use two biped or six
quadruped placements, with at least two quad contacts grounded. Walking speed
is unchanged. Quadruped resting views now use their authored yaw angles, giving
a three-quarter view and avoiding the former oversized native reversal. Explicit
API angles above140° retain the bounded paired fallback. Inspect the rendered
motion rather than treating link-length checks as proof of natural mechanics.
Planar companions retain
their existing adaptive gait. See the [turning correction](results/2026-10-17-natural-turning.md).
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
mkdir -p results/pixel-preview
env QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  build/qt/character_preview --character "$PWD/characters/soft-creature" \
  --capture "$PWD/results/pixel-preview/mallow.png" --action celebrate --time 0.4 \
  --mode composited --resolution 96 --dither --impulse 1,0 --impulse-strength 2
```

Other capture options include `--gaze X,Y`, `--palette`, `--linear`, `--debug`
and a bounded live `--duration SECONDS`. Grounded captures accept `--target X`
and `--reverse-at SECONDS`, and export realized roots and contact state. Interactive preview sessions need not
be repeatable; controlled captures and core tests make regressions inspectable.

Edit the authored pieces and regenerate with
`.venv/bin/python tools/author_characters.py`. See the
[character guide](../characters/README.md), [format](characters-v1.md) and
[implementation record](results/2026-10-08-pixel-characters.md).
The new [grounded implementation record](results/2026-10-08-grounded-characters.md)
includes motion evidence and verification limits.

For the separate original Blender werewolf, install the optional SDK modules and
rebuild, then launch its preview:

```sh
uv run --cache-dir .cache/uv python tools/bootstrap.py --with-3d
.venv/bin/python tools/build_qt.py --qt-prefix .deps/qt/6.10.3/macos
build/qt/characters3d/character_preview_3d
```

Click for a ground target; Left/Right move, Space stops, I applies force and P
pauses; D toggles contact markers. Add `--target 60` to launch it walking.
The [3D guide](../qt/characters3d/README.md) documents captures and rendering.
The editable [Blender source](../characters/werewolf-3d/werewolf.blend), skinned GLB,
original atlas and review renders are in `characters/werewolf-3d/`. Blender runs
only during authoring. The 2D build remains available with
`-DCLIBBY_WITH_3D=OFF`; missing optional modules leave 2D independently usable.
Resident 3D integration is implemented; broader native acceptance and resource
comparison remain separate checks.
