# Resident Foundation Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development task by task. Steps use checkbox syntax for tracking.

**Goal:** Add a usable resident Better Pets application with menu controls, safe desktop placement, quiet scheduling, and bounded diagnostics, preserving the benchmark.

**Architecture:** A Qt-free runtime determines animation deadlines and suppression. A Qt desktop host owns the window and display placement. A small shell composes the host, fixture motion adapter, menu, settings, single-instance lock, and native sleep notifications. Keep `pet_qt` as the benchmark and add `better_pets` as the resident executable.

**Tech Stack:** C++17, installed Qt 6.10.3 Core/Gui/Quick/Widgets, Objective-C++ for macOS workspace sleep notifications, CMake, Python unittest/CTest.

**Spec:** [Resident companion specification](../specs/2026-10-08-resident-companion-design.md), stage 1 only. Session API, character packages and resource acceptance soak belong to later stages.

## Global Constraints

- Keep C++17 and Qt Quick. Use typed, in-process boundaries and data-only packs.
- Preserve `shared/` as the frozen reference, including its 60 Hz equations and 44 golden poses.
- Hidden and paused states schedule no animation ticks or redraw requests after settling.
- Active animation has a configurable ceiling of 60 Hz and at most four catch-up steps per callback.
- Quiet gestures last at most two seconds and are spaced at least thirty seconds apart.
- Diagnostics use counters and optionally at most 256 samples; no unbounded frame history.
- No native desktop or battery claim from offscreen tests.
- Prefix shell commands with `rtk`; use `.venv/bin/python` for Python checks.

## Review Focus

- Pause plus hide must remain independent across Show and Resume; test all combinations in task 1.
- A late timer or system wake must not replay a backlog; test large clock jumps and suspend/resume in task 1.
- A removed or renamed display must leave the pet reachable; test negative coordinates, undersized displays and missing saved display in task 2.
- Restart and duplicate launch must not corrupt settings or create duplicate pets; exercise real lock ownership and invalid settings in task 3.
- Menu/drag callbacks and native sleep observers must not outlive their owners; use explicit teardown and offscreen lifecycle tests in tasks 2–3, then review lifetimes.

## Execution and record

The user requested implementation of the approved spec and the project instructions
explicitly prohibit redundant approval rounds. Execute this plan now with subagents
for implementation and independent review. Git initialization is blocked by filesystem
permissions. Work in place, retain `.superpowers/resident-foundation/baseline.tar.gz`,
and record progress/reviews there rather than claiming commits or creating another
repository outside the permission boundary.

### Task 1: Deterministic resident runtime

**Files:** Create `qt/resident/runtime.h`, `qt/resident/runtime.cpp`,
`qt/resident/runtime_tests.cpp`; modify `qt/CMakeLists.txt` only for a Qt-free
`pet_runtime` library and `resident_runtime` CTest target.

**Interfaces:** namespace `pets`; time is double seconds from an injected
monotonic clock. `enum class Control { Show, Hide, Pause, Resume, BeginDrag,
EndDrag, Suspend, Wake, Quit };`. `Runtime(double now = 0)`;
`void control(Control, double now)`; `Tick advance(double now)`;
`std::optional<double> nextDeadline() const`; `const RuntimeState &state() const`;
`const Diagnostics &diagnostics() const`. `RuntimeState` has booleans `visible`
(initially true), `paused`, `dragging`, `suspended`, `quitting` (initially false).
`Tick` contains `unsigned steps` (0–4). Diagnostics are fixed counters
`callbacks`, `steps`, `droppedSteps`. Expose configurable active Hz via a constructor
parameter defaulting to 60; reject nonfinite, nonpositive or over-60 values.

- [x] Write behavior tests with a simple throwing `check`, never release-disabled `assert`:

```cpp
pets::Runtime r(0);
r.control(pets::Control::Pause, 1);
r.control(pets::Control::Hide, 2);
r.control(pets::Control::Show, 3);
check(r.state().visible && r.state().paused, "show preserves pause");
check(!r.nextDeadline(), "paused runtime has no animation timer");
check(r.advance(1000).steps == 0, "paused time produces no steps");
r.control(pets::Control::Resume, 1000);
check(r.advance(1000).steps == 0, "resume never catches up");
```

Also cover quiet start deadline at 30, at most two seconds active, next gesture
at least 30 seconds after the prior gesture ends, drag suppression, huge clock
jump, wake retaining pause/visibility, duplicate controls preserving deadlines,
invalid/backward clock input, configurable tick rate, and bounded counters.

- [x] Run the tests against a minimal API skeleton and observe a behavioral failure.
- [x] Implement one active-window/deadline state machine. Controls cancel the active
  gesture and schedule fresh quiet time when suppression ends. `advance` produces
  at most four steps, counts dropped steps, and does not start missed gestures on
  a large jump. Do not poll during suppression. Keep diagnostic storage constant.
- [x] Configure/build and run `rtk proxy ctest --test-dir build/qt -R resident_runtime --output-on-failure`.
  Expected: all runtime cases pass. Record commands and red/green results in task report.

### Task 2: Desktop host and settings

**Files:** Create `qt/resident/desktop_host.h/.cpp`, `qt/resident/settings.h/.cpp`,
`qt/resident/host_tests.cpp`; modify CMake for `pet_desktop` and `resident_host`.

**Consumes:** Existing `Rig`, `Pose`, and `PetItem`; task 1 runtime remains separate.
**Produces:** `pets::DesktopHost` owning a transparent `QQuickWindow` and renderer;
methods `setPetVisible(bool)`, `applyPose(const Pose&)`, `restorePlacement(QPoint,
QString)`, `reconcileScreens()`, `position()`, `displayName()`, `isPetVisible()`, `cancelDrag()`;
callbacks `std::function<void(bool)> draggingChanged`, `std::function<void(QPoint)>
contextMenuRequested`, `std::function<void()> placementChanged`.
`pets::Preferences { QPoint position; QString display; bool hasPosition=false;
bool paused=false; QString character="fixture-animal"; }`;
`Preferences loadPreferences(const QString &file)` and
`bool savePreferences(const QString &file, const Preferences&)`.
Add a pure geometry helper using `QRect`/`QSize` so placement can be tested without
real displays. Avoid storing raw removed-screen pointers.

- [x] Add tests before implementation for placement on negative-coordinate displays,
  removed saved display fallback, small screen clamping, malformed settings and
  atomic settings round-trip. Create a real offscreen host and assert explicit
  Show/Hide and pose operations preserve expected window state.

```cpp
auto p = pets::loadPreferences(invalidFile);
check(!p.hasPosition && !p.paused, "invalid settings use safe defaults");
// Host tests use QApplication with QT_QPA_PLATFORM=offscreen.
host.setPetVisible(false);
check(!host.isPetVisible(), "hide removes pet window");
host.setPetVisible(true);
check(host.isPetVisible(), "show restores pet window");
```

- [x] Observe failures, then implement host focus flags, static interaction mask,
  drag delivery and context-menu callback. Cancel pointer dragging on hide, lost grab, or suspension; expose cancelDrag() for the shell sleep boundary. Use current-screen available geometry,
  reacting to screen addition/removal/geometry/scaling changes. Drag across displays
  by following the pointer continuously while the interaction region remains
  reachable. Preserve reachable placement on release and restoration; use clamping
  to recover from missing displays or an unreachable position, not on every drag
  move. Cover adjacent displays, unequal sizes, gaps, and outer edges explicitly.
- [x] Persist only allowed preferences in an atomic JSON settings file; cap input
  file size and validate types/ranges. No session/history/visibility persistence.
- [x] Build and run `resident_host`, `resident_runtime` and existing CTests. Expected:
  pass in offscreen software mode; record that native focus/click-through remain unverified.

### Task 3: Resident shell and lifecycle integration

**Files:** Create `qt/resident/main.cpp`, `qt/resident/application.h/.cpp`,
`qt/resident/instance.h/.cpp`, `qt/resident/sleep_observer.h`,
`qt/resident/sleep_observer_mac.mm`, `qt/resident/sleep_observer_stub.cpp`,
`qt/resident/application_tests.cpp`, `qt/resident/test_process.py`;
modify `qt/CMakeLists.txt`, `qt/motion.h/.cpp`, `tools/build_qt.py`, `README.md`,
`docs/handoff.md`, `docs/architecture.md`; create a native acceptance checklist.

**Consumes:** Task 1 Runtime and task 2 DesktopHost/Preferences APIs.
**Produces:** `better_pets` executable; `pets::ResidentApplication` composes a
runtime, host, one precise single-shot timer, menu/tray and preferences.
Expose `void control(Control)` as the menu/event boundary and read-only runtime
and timer status for integration checks where useful to diagnostics; avoid a
test-only production command protocol. Use a `std::function<double()>` clock
injection to make lifecycle tests deterministic. Native sleep observer uses RAII
and dispatches Suspend/Wake to that same boundary on the application thread.

- [x] Add failing tests that pause/hide stop the actual timer; Show does not unpause;
  Resume hidden stays hidden; quit releases resources; invalid settings recover;
  duplicate lock is refused and available after release; late timer processing
  produces bounded steps. Use real components in offscreen software mode.
- [x] Add process tests: `--help` succeeds; malformed arguments fail; missing assets
  fail; a normal resident process remains alive until terminated; duplicate launch
  exits successfully; restart after previous termination works. Isolate settings
  with temporary state directories, and clean every spawned process in `finally`.
- [x] Implement CLI `better_pets [--assets ABS_DIR] [--state-dir ABS_DIR]`. Default
  assets are copied beside the executable at build time; default state is the
  per-user app data path. Both overrides support development and isolated checks.
  Use QLockFile before showing windows; report already-running and exit 0. Treat
  lock I/O failure differently from an existing instance. Do not forcibly remove
  a live lock. Use QApplication for QSystemTrayIcon/QMenu with retained Qt Quick rendering.
- [x] Implement Show/Hide, Pause/Resume, disabled "No integrations connected" status,
  and Quit actions. Cancel host pointer dragging on Suspend/Hide so wake cannot remain stuck in a lost drag. Pet context menu uses the same actions. Do not silently run
  unreachable in normal desktop mode if no tray is available; error clearly.
  Offscreen integration testing can instantiate the shell without showing tray UI.
- [x] Add typed `MotionPhase { Static, Idle, Walk }` overload and cache numeric
  motion parameters at construction; retain existing string entry point as a
  benchmark adapter. Keep fixture equations exact. The resident shell calls only
  the typed overload. Reuse the renderer without frame telemetry hooks.
- [x] Add native macOS NSWorkspace sleep/wake observer with explicit deregistration;
  non-macOS builds get a stub and no support claim. Persist placement on user moves,
  pause changes, and orderly shutdown; avoid per-frame disk writes.
- [x] Build all targets and run the full CTest and Python suites. Export fresh poses
  to a new ignored result directory and verify all 44 match. Document launch command,
  implemented boundaries, next stage, and native acceptance still needing evidence.
- [x] Independent final code review against the spec and baseline snapshot, then
  fix material findings with targeted red/green tests and rerun affected checks.

## Native acceptance record

Build/offscreen tests establish implementation behavior, not desktop acceptance.
The stage 1 checklist must explicitly retain unverified focus, click-through,
menu-bar usability, physical display changes, and actual sleep/wake checks if
this sandbox cannot create native windows. Do not run an eight-hour soak or claim
battery targets in stage 1; those belong to stage 3.
