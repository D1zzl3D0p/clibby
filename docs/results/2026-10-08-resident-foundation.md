# Resident foundation — 2026-10-08

Stage 1 is implemented as software. Native acceptance is outstanding; this record
does not establish desktop parity, battery impact or completion of the first
usable milestone, which includes stages 2 and 3.

## Launch and boundaries

Build with the command in [README](../../README.md), then run from a normal
Terminal with desktop access:

```sh
rtk proxy build/qt/better_pets
```

Optional `--assets ABS_DIR` and `--state-dir ABS_DIR` override copied fixture
assets and per-user app-data state. A `QLockFile` is acquired before creating or
showing the pet. A duplicate launch reports the existing owner and exits 0;
state-directory/lock I/O errors exit nonzero. Live locks are never forcibly removed.

The shell owns one precise single-shot next-deadline timer, retained Qt Quick
resources, menu-bar/context actions, atomic settings and a macOS workspace
sleep/wake observer. It rests for thirty seconds between gestures lasting at
most two seconds. Each callback advances at most four steps; late time is dropped.
Hide, Pause, dragging and Suspend stop animation scheduling. Show preserves pause;
Resume preserves visibility. Wake starts fresh timing and reconciles placement.
Pause and placement persist; visibility and notification history do not.

Menu status is “No integrations connected.” The session API, `petctl`, session
labels/arbitration, production character packs, blob and coding-tool adapter are
stage-2/stage-4 work. Windows/Linux native behavior is unvalidated; their sleep
observer is a stub. No eight-hour soak or energy measurement was attempted.

## Software evidence

Tests use Qt's offscreen platform with the software renderer. They exercise real
windows/events and locks, injected monotonic clocks, actual Qt timer connections,
menu actions, atomic preferences and synthetic workspace notifications. Synthetic
sleep notifications verify registration, application-thread delivery and observer
teardown; they do not simulate a machine actually sleeping.

- Resident application: timer suppression, independent flags, bounded late steps,
  no replay after Wake, drag cancellation, invalid-settings fallback and restart,
  persistence on release, visible/deduplicated save failures, lock ownership/errors,
  unavailable-tray startup failure, Quit/queued-callback teardown, renderer failure
  and native observer teardown.
- Resident process: help, malformed arguments, missing assets, lock I/O error,
  untimed residency, duplicate launch, restart after termination and copied assets.
- Existing golden-pose CTest: all 44 poses retain the fixture equations/tolerance.

Final software verification on 2026-10-08:

| Check | Result |
| --- | --- |
| Release build | Both `pet_qt` and `better_pets` built successfully |
| Full CTest suite after the placement fix | 6/6 passed: runtime, host, golden poses, benchmark CLI, resident application, resident process |
| Python regression suite | 37/37 passed |
| Fresh headless pose export | All 44 poses match within 0.0001 |
| Frozen fixture, native evidence and archive checksums | All 203 recorded files unchanged |
| Documentation links | Checked local targets resolve |

The fresh pose export is local ignored output at
`results/resident-foundation-poses-5y26ythl/poses.json`; it is software regression
evidence, not a new native resource measurement. Build, test and review logs are
retained under `.superpowers/resident-foundation/` because Git initialization is
blocked by the workspace permissions.

Independent review identified a monitor-boundary drag jump. A focused red/green
regression now covers crossing in both directions, unequal screens and gaps,
transparent-only overlap, and release/restoration. Placement preserves a target
while the authored interaction region is reachable, including partial offscreen
placement. Missing displays and unreachable interaction regions trigger repair.

One nonblocking review note remains: an artificial negative clock origin can
produce a one-ULP early runtime step (about 5.6e-17 seconds in the review probe).
No practical desktop or battery consequence was identified.

## User desktop feedback and open monitor defect

After trying the resident build, the user reported that things generally looked
good, but crossing monitors alternated between snapping and an intermediate
position under the grabbed mouse. This is native user feedback, not a controlled
acceptance run; it does not establish passes for the other checklist items.

The earlier logical placement regression did not resolve the native problem.
Code triage points to the desktop host boundary: unreachable interaction regions
trigger repair, screen reconciliation can write placement during dragging, and
native display migration may change realized coordinates. The exact cause has
not been reproduced or isolated. No drag fix is claimed in stage 2.

Proceeding with the independent session API is reasonable: no evidence points to
a fundamental runtime or integration architecture defect. Before closing native
drag acceptance, capture a bounded trace of pointer global coordinates, grab
offset, requested/realized window position, screen geometry/scaling, and placement
writer during one crossing in each direction. Use that evidence to choose a
focused host fix. Notifications must not move the pet window.

## Native acceptance checklist — outstanding

Use a real macOS desktop session and a new ignored `results/` directory for
observations, OS/hardware/build identity and any screenshots. Do not change old
native evidence. Check each item only after recording its evidence.

- [ ] Start without benchmark arguments; visible pet and discoverable menu-bar icon.
- [ ] Duplicate launch creates no second pet; Quit and repeated restarts release ownership.
- [ ] Menu-bar/context menus remain usable; Show/Hide and Pause/Resume are independent.
- [ ] Launch, restore and automatic gestures do not steal keyboard focus; dismissing
      a user-opened menu returns control to the previous application.
- [ ] Transparent area passes clicks through; authored interaction area accepts clicks.
- [ ] Drag/release across displays preserves realized placement without jumps.
- [ ] Hidden/paused state settles without animation redraws; restore reuses resources correctly.
- [ ] Retina scaling and actual visible frame presentation are correct.
- [ ] Physical display removal/reconnection, primary-display change and geometry/scaling
      changes keep the pet reachable, including a missing saved display after restart.
- [ ] Actual sleep/wake retains pause/visibility, cancels a lost drag, restores valid
      placement and discards elapsed animation time without a catch-up burst.
- [ ] Settings-write failure is visible in the menu; recovery clears the indication.

Quiet CPU and eight-hour memory gates belong to stage 3. Whole-system controlled
energy/battery evidence remains a separate, unperformed validation requirement.
