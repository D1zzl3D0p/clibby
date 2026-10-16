# Procedural pickup, drop and recovery

Implemented 2026-10-11 after the user approved the design and implementation.

## Behavior

Production procedural 2D pets and the 3D werewolf share Held → Airborne → Landing → Recovering → Grounded motion. Dragging lifts the actual pet window; release keeps its height and uses recent realized drag momentum. Gravity advances the window in two axes with fractional placement and swept floor contact. Procedural limbs remain anatomically bounded while detached. Near touchdown their targets converge to sole contact, and the existing compliant support spring absorbs impact. Stronger impacts add crouch/pitch and one recovery step before ordinary behavior resumes. Pickup interrupts any phase.

Release cancellation and stale/equal-time pointer samples cannot inject a throw. Pause freezes motion; hide and sleep suspend without catch-up. Physics preempts autonomous navigation and reactions until settled, then the resident returns to quiet scheduling. Display changes translate existing contacts with the root. Partially offscreen but reachable placement remains stable on release. Native Cocoa hide/show now restores the realized position after Qt adjusts it.

## Implementation

- `qt/characters/drop_motion.*` provides Qt-free flight/lifecycle dynamics; `grounded.*` integrates compliant landing, bounded procedural limbs and recovery.
- `qt/characters3d/pose.*` and `qt/resident/spatial_character.*` adapt that same controller to measured 3D anatomy and projected logical pixels. No second flight integrator is introduced.
- `qt/resident/desktop_host.*` realizes two-axis motion and distinguishes release from cancellation. `application.*` estimates release velocity and coordinates suppression, rendering and persistence.
- Regression coverage lives in `drop_tests.cpp`, grounded/3D motion tests and resident host/character/spatial tests. `resident_character_tests characters --drop-only` runs focused native drop checks.

The frozen benchmark and shared fixture were preserved. Concurrent cursor and raven work in this workspace was retained.

## Verification

| Check | Result |
|---|---|
| Release build with 3D | Passed |
| Full integrated Qt suite | 36 passed, 4 environment skips, 0 failures (40 total) |
| 2D-only Release build | Passed |
| Full 2D-only Qt suite | 26 passed, 3 environment skips, 0 failures (29 total) |
| Python suite | 50 passed |
| Native Cocoa focused pickup/drop checks | Both passed, including height preservation under hide/show, regrab and cancellation |
| Native Cocoa 3D drop/render check | Passed selection, realized fall/recovery, suppression, sessions and restoration; transparent framebuffer had 83,160 visible pixels and one redraw over 500 ms at rest |
| Native Cocoa Spaces/restore checks | Fixture host, modular host and attached status card passed; host position preserved across hide/show |
| Independent integration review | Three reproducible defects fixed and independently retested; no unresolved findings |

The headless suite skips IPC, native Spaces and GPU presentation checks where unavailable. Native Spaces was separately tested. Native 3D drop/render was separately verified; its deliberate synthetic renderer failure diagnostic is expected rollback-test output. Earlier stale 2D binaries mixed incompatible state layouts; rebuilding consistently resolved the controller-state assertion. A subsequent rebuild incorporated concurrently changed raven sources and both full suites passed.

Review probes and native captures are preserved locally under ignored `results/drop-implementation/`. Review fixes cover partial offscreen releases, root/contact relocation, and actual sole continuity at touchdown. 3D regressions cover both headings and multiple fall heights, plus large root relocation. Automated native checks use synthetic pointer events; manual throw feel, appearance during an interactive fall, multiple physical monitors, and battery impact remain unmeasured.

Restart `build/qt/better_pets` to try the rebuilt resident. Lift a production procedural pet and release; a fast recent drag supplies momentum. Regrab during the fall or recovery to interrupt it.

## Follow-up: heavier gravity and continuous throws

User feedback identified floaty falls, strongly limited throw momentum and reversed landing preparation. Investigation confirmed two release speed caps: resident axis limits (800/1000 logical px/s) and a shared anatomy-relative radial limit (12 leg reaches/s). These made stronger flicks converge to the same slow release. Both release limits are removed; finite inputs and real display collisions still apply. Ordinary drops now use three times the previous acceleration (54 rather than 18 leg reaches/s²). Authored hops and powered wing flight retain their 18-reach gravity, including a raven that begins powered flight while already airborne. Force integration no longer abruptly clamps an existing throw when residual wing forces are present. Reset and newly lost support restore ordinary drop gravity.

Release uses the last 60 ms of realized dragging, including a final release sample, so a quick flick is less diluted by earlier slow movement and a stationary hold loses stale momentum. Regression checks first reproduced the capped release and slow 500-unit fall. They now check uncapped strong throws, continuous interior horizontal travel, heavier fall time, compatible powered flight after a throw and gravity restoration after reset.

Landing preparation now starts within a bounded time-to-contact horizon. Legs reach nearly full anatomical extension before touchdown; the presented body offset carries continuously into compliant support, which then compresses under load. Impact handoff uses smooth bounded dissipation rather than a hard cap that made shallow and deep impacts similar. Pitch also enters through a follower rather than snapping at contact. Measured test extension ratios progress 0.824 held → 0.998 before contact → 0.859 under load; shallow/deep maximum compression is 1.816/3.562 fixture units. Independent 10 µs touchdown probes confirm actual sole/body continuity and reduce the former 8° pitch snap to about 0.000006 radians.

The planar werewolf uses optional arm/forearm bindings for smooth falling arm spread; editable asset generation includes those bindings. The 3D werewolf uses measured arm IK with body-relative outstretched hand targets and a faster 4 Hz airborne response; neutral held posture, ordinary 2.5 Hz reaching, powered wing flight and smooth recovery remain intact. Arm lengths and rendered target bounds are checked. Native captures are preserved as `results/drop-implementation/tuned-3d.png` and its `.fall.png` companion.

Final follow-up verification: full Release rebuild passed; integrated Qt 36 passed/four environment skips, 2D-only Qt 26 passed/three environment skips, Python 50 passed. Focused native Cocoa strong-throw checks passed. Native 3D selection, drop/recovery, restoration, transparent rendering and quiet settle checks passed with the revised fall pose. Independent review has no remaining findings. Manual feel remains for the user to judge after restarting the rebuilt resident.

### Revised throw cap

At the user's request, release speed is now limited to 2,400 logical pixels/s by one radial screen-space cap in the resident. This supersedes the unrestricted input described above. The cap preserves direction, treats 2D and 3D equally, and applies once to the initial throw; it does not limit later gravity acceleration or cause a midair speed plateau. Throws below the limit retain their measured vector. Regression first failed without the cap, then passed for an extreme diagonal flick at the cap while the existing 1,300+ px/s throw remained unaffected.

### Latest direction: quadratic air resistance

The user replaced the hard-cap/fixed-transfer idea with quadratic air resistance. Initial velocity now comes directly from measured dragging. Shared ordinary drop flight applies resistance opposite the velocity vector, proportional to speed squared; this replaces the temporary 2,400 px/s cap and 70% transfer. Faster flings continue to start faster and travel farther, while greater speed produces greater resistance continuously. Authored hopping and wing-flight tuning remain separate. Verification is recorded after implementation below.

The drag coefficient starts at `0.015/referenceReach`: acceleration is `-k * |velocity| * velocity` alongside gravity. The solver uses 240 Hz substeps, symmetric external-force kicks and exact combined linear/quadratic damping with logarithmic displacement. Swept floor contact avoids tunnelling. Extremely large finite inputs cannot reverse under drag; no artificial release-speed cutoff is applied. A fixture fling at 1,600 units/s travels 1,240.89 units versus 697.697 at 800 units/s with the same height and vertical release speed. A 500-unit drop with 32-unit reach still lands in about 0.79 s.

Independent review fixed combined linear/quadratic damping reversal at extreme inputs and removed the host's 1,000-unit per-frame displacement clamps. Real floor, ceiling and horizontal bounds remain authoritative. A 1,600-unit synthetic support sweep checks complete host touchdown realization. Secondary 3D head/tail motion remains bounded separately from physical velocity. Review probes and stability evidence are preserved under `results/drop-implementation/air-resistance-review.md`.

Final quadratic-drag verification on 2026-10-12: both Release configurations were rebuilt cleanly after mixed-generation linked artifacts were detected. The full 2D-only suite passes 26 checks with three environment skips. Shared drop/flight, resident host/surface/character, 3D motion/expression and werewolf spatial checks pass. The combined 3D suite has 35 passes, four environment skips and one Raven powered-altitude failure, which paired relinks reproduce identically with quadratic drag enabled and disabled; it is not resolved by this throw tuning. Earlier lower-perch failures belonged to older linked generations, and the clean 2D Raven integration now passes. See the paired isolation record under `results/drop-implementation/raven-drag-isolation/review.md`. Final numerical review has no open air-resistance findings. The resident executable is rebuilt; restart it to test the feel. No new native GUI or battery claim is made for the quadratic-drag revision.
