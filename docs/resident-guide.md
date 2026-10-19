# Using the resident pet

Character selection, desktop controls, notifications and local development state.

The [productivity delivery record](results/2026-10-16-productivity-foundation.md)
documents the inbox, task and timer foundation and its verification limits.

Run commands from the repository root after following the [setup instructions](../README.md#setup-and-test).

## Run the resident pet

On macOS, open the built `build/qt/Clibby.app`, or launch it from a normal
Terminal with desktop access:

```sh
open build/qt/Clibby.app
```

The terminal entry point is also available:

```sh
build/qt/clibby
```

The app bundle includes its Qt runtime and can be moved. Public distribution
signing and notarization remain separate work. The [visual identity guide](brand.md)
documents the approved artwork and how to regenerate its production assets.

No benchmark arguments are required. The default is Mallow, the soft creature.
Use the Character menu to choose Bramble, the werewolf. Packages are copied beside
the binary at build time; preferences and the instance lock use the per-user app
data directory. New installs use the Clibby app/organization directory. If it is
absent and the previous Better Pets directory exists, the resident and `petctl`
reuse that directory in place; an existing Clibby directory takes priority.
Restart restores the selected character within this catalog.
For isolated development state:

```sh
build/qt/clibby --character "$PWD/characters/soft-creature" --state-dir "$PWD/results/resident-dev-state"
```

An explicit `--character` overrides the remembered selection. `--assets "$PWD/shared"`
retains the original fixture path for regression/development; use either flag.

Left-drag to move the pet. Use the menu-bar icon or right-click context menu for
Show/Hide, Pause/Resume and Quit. Visibility and pause are independent; restarting
launches visibly and retains pause and placement. A duplicate launch exits
successfully. The pet rests between brief gestures; hidden or paused states stop
animation ticks. Session expiry still runs when needed. The Sessions menu lists
current inputs and lets you acknowledge completion, attention requests and failures.
**Appearance → Companion size** changes the companion to 50%, 75%, 100%, 125%,
150% or 200%. **Appearance → Pixelation** offers Fine, Light, Classic, Chunky
and Very chunky. These controls are available from the menu-bar icon and the
companion's right-click menu. Changes apply immediately and are remembered across
launches and companion changes. Pixelation changes rendering density independently
of size; finer rendering cannot add detail to existing 2D source artwork. The
archived fixture keeps its original renderer and disables the pixelation choices.

Toolbox… opens the shared status-card inbox and existing pet controls. Cards offer
Open source separately from Acknowledge. Its Travel speed control ranges from
0.5× to 3×, starts at 2× for new settings, and saves your choice across restarts.
The body follows the requested speed smoothly; feet adapt their steps to catch
up within leg reach. Feet retain exact planted contacts and alternate complete swings, including
at faster travel speeds. Changing speed keeps the current walking target.

Productivity… opens the durable attention inbox and task capture, with separate
Acknowledge, Snooze and Complete follow-up actions. Its Timers and Pomodoro tab
supports configurable intervals and pause/resume/next/cancel. Productivity remains
available when the pet is hidden; timers use the resident's existing scheduler.
See the [productivity API](api/productivity-v1.md) and
[Claude/Codex hook templates](api/agent-hooks.md). Hooks are opt-in templates;
all-app OS notification capture is not enabled.

Dragging adds bounded local body/head/ear/tail follow-through. Supported
characters use shared pickup, release and landing motion. Pause suppresses these reactions too. Settled springs stop their
animation deadlines. Walk left/right and Stop menu commands drive grounded 2D pets and the 3D werewolf along
the selected display floor with planted feet and generated steps.

Normal desktop startup fails clearly if persistent menu-bar controls are unavailable.
Offscreen software tests omit tray UI. Native focus, click-through, real displays,
and actual sleep/wake still require the [manual checklist](results/2026-10-08-resident-foundation.md).
Opt-in coding-tool hook adapters are available as templates; installed delivery
still needs native validation. Quiet autonomous curiosity shares the resident
scheduler and yields to direct control, pause and hide.

Marmalade 3D adds the selected woodland cat art with four procedural legs,
continuous skinned limbs and a 96×96 nearest-rendered source. Select it from
Character or start the rebuilt resident with:

```sh
build/qt/clibby --character "$PWD/characters/marmalade-3d"
```

See the [spatial quadruped record](results/2026-10-16-spatial-quadruped.md)
for verification and the remaining desktop acceptance work.

The live werewolf is selectable through Character → werewolf-3d in a
Quick3D-enabled build. To start it explicitly:

```sh
build/qt/clibby --character "$PWD/characters/werewolf-3d"
```

It shares walking controls, speed, saved placement, notifications and Toolbox
with the 2D pets. Its resident presentation uses a three-quarter facing bias
and a slight top-down orthographic camera. Its transparent window follows the model and native placement
feeds back into floor contacts. Software-only builds retain the current 2D pet
and explain why 3D is unavailable. The renderer uses trusted deployed assets in
`build/qt/clibby-3d`; keep that folder beside the executable when moving it.
See the [resident 3D record](results/2026-10-10-resident-3d.md) for validation
and remaining native acceptance checks.

The first expansion also includes Pip, Biscuit, Clover, Mossback, Rivet, Paper
Heron, Acorn Custodian, Puddle, Pocket Observatory, Night Librarian, Sprig,
Hearthling and Crumb Golem as selectable 3D base pets. Each uses shared procedural
movement with its own original anatomy. Fourteen catalogue pets, including
Marmalade 3D, have finite planted-feet signature actions. See the
[signature-action record](results/2026-10-16-signature-actions.md). Pebblekin, Margin, Lantern Snail and Velvet currently have working
experimental motion studies outside the selector. See the
[per-pet implementation status](results/2026-10-16-catalogue-base-implementation.md).

## Notify from real work

With the resident running, wrap a command to report its actual outcome:

```sh
.venv/bin/python tools/pet_run.py --label "Project tests" -- .venv/bin/python -m unittest discover -s tests
```

Open source returns to the current project folder by default; `--source-url` can
supply a supported web link. Acknowledge quiets the card without changing the
reported outcome. See [producer options](api/command-producer.md) and the
[verification limits](results/2026-10-08-utility-slice.md). Live IPC delivery
still needs a normal Terminal trial; software tests here do not establish native
click/focus behavior or battery impact.

## Try session notifications

With the resident running, use the same `--state-dir` for both programs if you
overrode its default. Each session needs increasing sequence numbers:

```sh
build/qt/petctl update --source demo --session build --label "Build" --seq 1 --status working
build/qt/petctl update --source demo --session build --label "Build" --seq 2 --status needs_input
build/qt/petctl state
build/qt/petctl clear --source demo --session build --seq 3
```

The pet shows plain status text and a short gesture. Pause freezes its pose and
label while the menu stays current. Hide retains incoming data. Expiry removes
stale sessions; sleep/wake clears them. Reactions never move the pet window.

For the explicitly **simulated** two-source example:

```sh
.venv/bin/python examples/simulated_sessions.py --petctl build/qt/petctl
```

This is a generic integration interface, not a connection to a real AI tool.
The [API contract](api/session-v1.md) documents framing, schemas, limits,
permissions, responses and CLI exit codes. Live socket tests require an environment
that permits Unix socket binding; restricted execution reports those checks skipped.
