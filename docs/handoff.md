# New-session handoff

## Final pre-upload validation passed (2026-10-18)

Independent fresh Release/app build in `build/github-edge-fix-2026-10-18/`
succeeds; all **88 Qt checks pass with five environment skips**, including both
edge-turn regressions, the six original motion failures and app relocation.
Python passes 109 tests. Runtime, assets and tests stayed unchanged during the
final build/suite. Read the [final checkpoint](results/2026-10-18-pre-upload-check.md).
Publication hygiene and link checks pass. No commit or upload was performed;
repository-wide source/brand licensing remains unselected.

## Current: Biscuit rightward movement and cursor attention (2026-10-18)

Biscuit now moves right after reaching the left desktop edge through both the
context-menu action and cursor attention. An edge-only hind-first alternative
keeps the accepted six-placement/.84s turn; directional host movement now
preserves the body offset left by integer presentation rather than requesting
an outward recenter. The accepted normal turns and anatomy-derived walking pace
are preserved. Held cursor intent no longer reverses when the turn arc carries
the body past the pointer. Automatic signatures defer during active cursor
attention, preventing interrupted head tracking; manual signatures remain.

Fresh full app build and relocation pass. Combined Qt verification: **88 passes,
five environment skips, zero failures** (93 total). Clean final native spatial
regressions pass, including menu-to-cursor turn, head up/down/left/right,
stationary/vertical pointer heading retention, deliberate crossing, sustained
attention past 25s, and automatic idle resumption afterward. Independent native
host checks pass all 36 Biscuit speed/size edge trips. Existing leftward mask
rounding up to two pixels and exact-edge destination shortfall remain recorded.
Read the [result record](results/2026-10-18-rightward-cursor.md); raw evidence is
in ignored `results/rightward-cursor-2026-10-18/`. This supersedes the two failures
in the pre-upload checkpoint below. Restart `build/qt-humanoids/Clibby.app`.
No commit/push; unrelated dirty work preserved. Python/2D-only not rerun here.

## Latest pre-upload check after cursor work (2026-10-18)

The stable 1,000-file checkout builds and passes app relocation and 109 Python
tests. Combined Qt results: **86 passes, five environment skips, two failures**.
`resident_spatial` still fails Biscuit's cursor turn back right after left-edge
arrival; `resident_quadruped3d` also fails manual rightward movement from that
edge. Both reproduce in a focused rerun. No source changed during validation.
See the [updated checkpoint](results/2026-10-18-pre-upload-check.md) and ignored
logs under `results/pre-upload-final-2026-10-18/`. Upload hygiene/link checks
pass; no commit or upload was made.

## Pre-upload revalidation (2026-10-18)

All six original motion failures now pass in a rebuilt combined suite: 88 passes,
five environment skips; Python passes 109 tests. The app build and relocation
also pass. A newer, concurrently added `resident_spatial` cursor regression
fails after rebuilding: Biscuit cannot turn back right toward the cursor after
manual travel to the left desktop edge. That separate investigation is active;
do not treat the earlier suite as certifying this latest test snapshot. See the
[revalidation record](results/2026-10-18-pre-upload-check.md). No commit or upload.

## Current: anatomy-derived walking pace (2026-10-18)

User approved roughly shared stepping rhythm with leg length determining speed,
and the existing speed control retained as a multiplier. Spatial ground speed
now uses 0.85 measured leg-chain lengths/second (shortest chain, planner bounds),
including winged ground walkers. Strides already scale with reach; swing timing
now stays roughly invariant across scaled anatomy. Turns and flight are unchanged.
This supersedes universal 70 cm/s base / 140 cm/s default claims below.

New scaled-anatomy travel/rhythm/multiplier/plant regression passes, as do existing
short-rig walking, turns, arrivals and recovery checks. Fresh full app build and
bundle relocation pass. Combined Qt: 88 passed, five environment skips, zero
failures; Python: 109 passed. Independent review found no actionable defects.
Read the [pace record](results/2026-10-18-anatomy-pace.md). Restart rebuilt
`build/qt-humanoids/Clibby.app` to assess feel. No native visual assessment or
battery measurement here. No commit/push; unrelated dirty work preserved.

## Current: directional leading plant (2026-10-18)

The user rejected the prior foot-yaw-led turn as still spinning. The replacement
chooses a world step into the requested heading first, plants the leading paw
with torso/root held, then follows that actual fixed plant. Ordinary turns keep
two biped/six quad placements and .84s quad duration, with two or more quad
supports. Longer walking strides and original70cm/s base/default140cm/s remain.
Tiny <=.1° corrections no longer launch paw steps or proxy catch-up placements.
An extra finite .58R lead alternative fixes Mossback's fast rounded-arrival
reversal without changing previously successful plans.

Important calibration finding: prior audits guessed the authored rig angles,
but the old live app used -10.8° for all quads (158.4° reversal). Those old
metrics were not native-angle validation. The app now intentionally reads
quadruped authored yaw metadata (legacy default -35°): five use110° turns,
Tarn130°. This restores the authored three-quarter resting view. Other biped
views are unchanged, including Nacre/Windle=-35°. Final audit input comes from
fresh Qt probes of all34 live view properties and all80 leg chains, not guesses.
API turns above140° retain the bounded paired fallback, outside the new resident
plant-style claim. Kinematic support transfer does not establish force balance.

Fresh verification: all1,632 ordinary direction/speed/rate/host/face-navigation
cases pass; exactly2/6 stationary placements, first plant <=.0378° torso yaw,
turn hip/rest>=.901444, max extension1.00309, planted translation/yaw drift0,
and unchanged35/140/210cm/s selected paces. Final combined Qt checks:88 passed,
five environment skips, zero failures (93 total, including separate bundle
relocation). Full app build passes. Fresh24-case critical review, subpixel
zero-step regression and post-quiet navigation pass. Native Biscuit/Mossback/Tarn
checks pass;180 captured frames are unclipped. The preceding144 full180° fallback
checks remain labeled with their own snapshot rather than as a new-style result.

Evidence: ignored `results/directional-plant-2026-10-17/`; controlled same-view
before/after is `biscuit-leading-plant.gif`. Its horizontal host motion comes from
exact-rig root traces, since the raw preview follows the root. Read the
[result record](results/2026-10-17-natural-turning.md) for provenance and limits.
Restart `build/qt-humanoids/Clibby.app`. Visual feel still needs the user's eye.
No commit/push; unrelated dirty work preserved. Python/2D-only not rerun here.

## Previous candidate: foot-led turns (rejected for still spinning)

The user rejected the upright point-pivot below because it still read as a spin.
The replacement plants the leading biped foot before the torso follows, then
brings the trailing foot through. Quadrupeds use three staggered fore/hind/fore
transfers with two or more supports throughout, rather than spinning on one paw.
Ordinary turns use two biped or six quadruped placements; difficult full 180°
quadruped turns can use five transfers. Original travel speed and longer walking
strides remain. Loaded feet neither slide nor swivel.

Fresh independent checks pass 816 ordinary cases and 72 full-180° cases across
actual rigs, rates, speeds and host rounding policies. Foot translation and yaw
lead torso yaw; first touchdown occurs by 32° torso rotation. The minimum
turn hip/rest ratio is .919 ordinary/.907 full-180°. A blocked-recovery
regression now preserves a newly requested destination. Evidence is under ignored
`results/foot-led-turn-2026-10-17/`; see the current result record for final build
and capture results. Earlier geometry success was not visual acceptance, and
this kinematic support-transfer gesture still needs the user’s visual assessment.

Delivery: full app build and bundle relocation pass; fresh combined Qt suite
passes 88 checks with five environment skips (93 total), zero failures. Native
Biscuit/Nell/Mossback/Puddle pass, with 240 unclipped animation frames. Restart
`build/qt-humanoids/Clibby.app`. The comparison clip and complete logs are under
`results/foot-led-turn-2026-10-17/`. No commit or push. Previous Python and 2D
results below were not rerun for this pose-only correction.


## Pre-upload checkpoint (2026-10-17; motion failures subsequently fixed)

> The six motion failures recorded at this concurrent checkpoint are superseded
> by the upright-turn correction below: the later complete combined run passes
> 88 checks with five environment skips, including all six former failures.
> The publication/licensing findings remain a separate checkpoint record.

The stable checkout passes 109 Python tests and completes the Release/app build,
but the fresh combined Qt run has **82 passes, five environment skips and six
failures**. All six reproduce in a focused rerun: raven perch recovery, stationary
turn placement count, upright bounded pivot, quadruped support, Marmalade binding
support and werewolf elbow clearance. These results supersede earlier passing
counts for this checkout. See the [pre-upload record](results/2026-10-17-pre-upload-check.md).
Publication hygiene/link checks pass; repository-wide licensing is still
unselected. No fixes, commits, remote configuration or upload were performed.

## Historical correction: upright point-pivot (subsequently rejected)

The user rejected the previous gait again: excessive placements and a visible
crouch/spin. Fresh diagnosis found Biscuit dropping to 56.2% of standing hip
height because lifted feet constrained the whole body's height. The prior
extension-only checks did not establish upright posture.

The replacement uses a planted body/root arc and one finishing placement:
two foot placements for bipeds, four paws for quadrupeds on open ground.
This is an authored dynamic point-pivot approximation (three quadruped paws
briefly unload), not verified casual-animal biomechanics. The host consumes the
travel-axis arc; persistent transverse offsets survive subsequent walking and
pickup. Completed stance synchronization removes additional planner recovery
steps. Original nonwing travel remains 70 cm/s base, 140 cm/s at default 2×.
Centered stance forecasts reduce Nell's default walking placements from 37 to 24
per 12 m; ordinary biped walking stays above 94.7% standing height in the
102-case audit. Mossback walking remains more compressed (87.8% at default).
Unreachable bounded arcs cancel quietly; unexpected blocking lands lifted feet.

Read the [correction record](results/2026-10-17-natural-turning.md), which marks
the rejected candidate as historical. Fresh checks pass: combined Qt 88/five environment skips, 2D-only 36/four
skips, Python 109, and both bundle relocation checks. The 816-case turn audit,
102 steady-walk cases and 240 unclipped native frames are recorded there.
Restart rebuilt `build/qt-humanoids/Clibby.app` to try the candidate. Shoulder and pickup-envelope fixes
remain. Existing appearance/utility changes in this dirty checkout are preserved.

## Latest: companion appearance controls (2026-10-17)

**Appearance** in the menu-bar/context menu now offers companion sizes 50–200%
and five pixelation levels, independently applied and remembered across launches
and companion changes. Host geometry/masks, support footprints, planar/spatial
motion and notification anchors scale together. Coarse pixels retain a sufficient
visual fringe. The archived fixture disables pixelation.

Fresh checks: 109 Python tests; 87 combined Qt passes/five environment skips;
native macOS spatial regression passed at 50%/200% with fine/chunky captures in
`results/appearance-controls-2026-10-17/`. Rebuilt
`build/qt-humanoids/Clibby.app` passes relocation. See the
[delivery record](results/2026-10-17-appearance-controls.md).

## Latest: shared companion polish (2026-10-17)

Spatial companions now use anatomy-scaled strides/cadence, step-phase propulsion,
reachable support height and brisk stepping turns that brake before reversal.
Ordinary-walk link extension is about .309% in the 34-rig anatomy audit, with
planted contacts preserved. Biscuit and other companion heads/torso areas use
pose-following grab envelopes. Twelve original human packages have connected,
sloped shoulders and less arm splay, with armour/cuffs adjusted.

Fresh Release app build succeeds. Python: 109 passed. Combined Qt: 87 passed,
five environment skips, no failures. True 2D-only build: 35 passed, four skips.
Native captures cover Biscuit, Nell, Paper
Heron and Marmalade (240 unclipped walking/turning frames). Read the
[delivery record](results/2026-10-17-companion-polish.md) for evidence and limits.
Use rebuilt `build/qt-humanoids/Clibby.app`. Other final-companion and appearance
changes arrived concurrently and were preserved; no automatic commit was made.

## Latest: approved eighteen-companion expansion complete (2026-10-17)

Maren, Tarn, Pell, Serein, Nacre and Windle are implemented with original models
and finite signatures. All eighteen approved expansion archetypes are selectable
under **Companions**. Tarn has four real contacts and attached folding membranes;
Nacre has free-arm gather and visible mantle markings; Windle has weighted wisps,
scarf and hem supports. These remain grounded stylized movement, without new
flight/aquatic simulation or timers. Tarn tip seams use actual local-Y hinges.

Native validation covers all six bodies/actions, with 630 unclipped signature and
360 walking frames under `results/final-companions-2026-10-17/`. Full Python 109
passes. The exact staged 3D checkpoint passes 84 checks with two environment
skips; the shared 3D build passes 89 with three skips. The isolated 2D checkpoint passes 33 checks with one environment skip. The latest
shared 2D run reports a concurrent size-change failure, excluded from this checkpoint.
Read the [delivery record](results/2026-10-17-final-companions.md) for final
verification, concurrent-work context and first-pass activity limits. Restart
rebuilt `build/qt-humanoids/Clibby.app` and choose **Companions**.

## Latest: next six companions and physical hinges (2026-10-17)

Iona, Milo, Orris, Fen, Oona and Quillan are implemented with distinct original
bodies, real arms, attached props/ear-fins and finite signature actions. Map,
clock-key and lantern-shutter hinges now use explicit joint-local rotation;
older profiles retain their default neutral-model semantics. Native regression
proves each shaft stays aligned while owners raise their hands.

Fresh checks: 98 Python; 82 combined Qt passes/three environment skips;
37 2D-only passes/two skips; no final failures. Native evidence includes 630
unclipped frames in `results/wayfarer-companions-2026-10-17/` and a comparison GIF.
Use rebuilt `build/qt-humanoids/Clibby.app` → **Companions**. Read the
[delivery record](results/2026-10-17-wayfarer-companions.md) for checks and first-pass
activity limits. Maren, Tarn, Pell, Serein, Nacre and Windle remain approved backlog.

## Latest correction: macOS menu-bar click crash (2026-10-17)

The Qt 6.10.3 tray observer reads a mouse-only event property on a macOS 27
gesture event (QTBUG-147449), raising an AppKit exception. macOS now uses a
native status item attached to the existing Qt menu; other platforms retain
the Qt tray. The original event exception was reproduced in a native regression.
The user confirmed the rebuilt `build/qt-humanoids/Clibby.app` works. See the
[correction record](results/2026-10-17-menu-bar-crash.md).

## Latest: documentation navigation (2026-10-17)

The root README now covers overview, status, setup, launch and guide links.
Detailed commands are preserved in [the resident guide](resident-guide.md),
[character previews](character-preview.md), and [benchmark/motion validation](benchmark.md).
Commands in those guides still run from the repository root. Research reports
and supporting notes moved from the repository root into
`docs/research/reports/` and `docs/research/notes/`; the [research index](research/README.md)
links all eight files. Repository references and moved documents' relative links
were updated. Historical evidence and archive contents were preserved.

## Latest: six original humanoid companions (2026-10-17)

Nell, Alden, Saffron, Tess, Rowan and Vesper are implemented and selectable in
**Companions**, with original low-poly assets, distinct adult silhouettes/faces,
real arms and attached props. Each has a finite signature; hand-owned offsets
apply after arm realization while the support solver retains foot ownership.
Human faces use a more frontal resting angle at the same 96×96 native density.
The full approved eighteen-companion catalogue and the remaining twelve are in
[the design](superpowers/specs/2026-10-17-humanoid-companions-design.md).

Fresh combined Release checks: 76 passed, two environment skips, no failures.
Final native action evidence covers 630 unclipped frames and includes
`results/humanoid-companions-2026-10-17/humanoid-signatures.gif`.
Use the fresh `build/qt-humanoids/Clibby.app` after the repository rename.
Read [the delivery record](results/2026-10-17-humanoid-companions.md) for checks,
reproduction paths and the limits of the first signature vignettes.

## Latest correction: portable Clibby app startup (2026-10-17)

The repository is now `/Users/dizzler/Projects/Repos/clibby`. The initial app
bundle still referenced Qt at the former `better-pets/.deps` path and aborted
in dyld before startup after the directory move. The build was freshly configured
against the current SDK. The macOS target now runs `macdeployqt` after linking,
including generated 3D QML imports, and carries frameworks, plugins and QML
modules inside the bundle. Signing/notarization for public distribution remain
separate work.

`clibby_bundle_relocation` first reproduced the missing QtWidgets failure and now
passes after copying the app to a temporary location with Qt/DYLD environment
overrides cleared, verifying that loaded Qt frameworks come from that copy.
Native Cocoa 3D startup stayed alive and answered live IPC with a temporary state
directory. Fresh full Qt checks: 76 passed, two environment skips, zero failures.
Original crash evidence is in ignored `results/clibby-startup-2026-10-17/`.


## Latest: GitHub preparation (2026-10-17)

Repository housekeeping refreshed the README's implemented-feature status and
standard setup commands, added [contributor guidance](../CONTRIBUTING.md), text/
binary Git attributes, and preventive credential/editor/Blender-backup ignores.
Forty-one machine-local research/report links now resolve within the repository.
Historical evidence, source assets and existing runtime work were preserved.
No remote, commit, push or repository-wide license was created by this cleanup.
Character package licenses remain scoped to their packages.

Verification during cleanup: 94 Python tests pass; relative Markdown links,
ignore probes and `git diff --check` pass. A fresh Release build in ignored
`build/github-check/` compiles; its 71-test CTest snapshot has 66 passes, four
environment skips, and one failure: `resident_quadruped3d` reports `3D rig missing`.
A focused rerun reproduces it. Humanoid source/assets were arriving concurrently
in this checkout, so these results describe a changing working tree and are not
a final release gate. Rebuild and rerun the full checks after that work settles.
The old `build/qt/` cache points to the checkout's former name; use a fresh build
directory after a move, as described in the contributor guide.

Publication audit found no likely credential patterns in the inspected current
files, archive text or 868 Git-history blobs. Tracked content was about 14 MB,
with no oversized history blobs requiring LFS. Pattern scans are limited checks;
historical evidence intentionally retains original machine paths and metadata.

## Latest: Clibby identity and rename (2026-10-17)

The product is now **Clibby** (executable `build/qt/clibby`). The approved
crescent/head mark is exported reproducibly from `art-studies/clibby/build.py`
through `tools/build_brand_assets.py` to `assets/brand/`: SVGs, PNGs,
monochrome menu template, macOS icon and tokens. The shared scoped Widgets theme
covers cards, Toolbox, productivity panels and the 2D authoring preview; 3D
preview controls follow the system palette. Pet artwork and scheduling are unchanged.

macOS also builds `build/qt/Clibby.app`, including the icon, character packages
and 3D resources, synchronized on every app rebuild. It uses local Qt libraries;
distribution packaging is still separate work. Read [the identity guide](brand.md)
for typography, colour, spacing, asset generation and compatibility details;
the [delivery record](results/2026-10-17-clibby-identity.md) lists verification.

The resident and `petctl` discover the same Clibby state directory. Existing
legacy data is reused in place when a Clibby directory does not yet exist;
explicit `--state-dir` still wins. Hook counters retain their own legacy locks.
The `bp-` socket prefix and protocol are preserved. Setup paths are relative to the checkout root. Historical sections and evidence
below keep their original product/executable names and machine paths.

Release build passes; the full Python suite passes 93 checks. Qt checks pass
67 with four environment skips (IPC transport, native Spaces and GPU render).
Light/dark UI captures are under `results/clibby-brand-2026-10-17/ui/`.
Native menu appearance and app launch interaction still require desktop review.

## Latest: first signature-action pass

All fourteen live catalogue-style pets now have finite planted-feet signature
actions, including Marmalade. The menu/toolbox's **Signature action** triggers
the selected pet's named action; quiet autonomy uses the existing single timer.
Real new weighted appendages support antenna/arm, wings, cap, book covers/ribbon
and leaf motion. Pause freezes actions; walking/notifications release smoothly;
drag/hide/reset discard them. No extra reach or flight capability is implied.
Supported flops/sitting/hops, owned props and experimental-family promotion
remain pending. Read the [per-pet signature record](results/2026-10-16-signature-actions.md).
Restart the rebuilt resident to use the new actions.

## Latest: shared attention inbox and productivity foundation

The functionality-first direction is specified in the
[productivity design](superpowers/specs/2026-10-16-productivity-system-design.md).
The first working slice adds an atomic persistent inbox with search, pagination,
acknowledge/snooze/local follow-up completion, quick task capture, countdowns and
configurable manual-transition Pomodoro. One shared Qt Core service feeds a new
Productivity… panel, menu/Toolbox entry and additive local IPC/CLI methods. Existing
temporary session API semantics remain intact. Timers share the single resident
timer and can create missed items while the pet is hidden. Source-defined occurrence
IDs let distinct approval requests rearm attention without inventing progress.

Claude Code/Codex adapters and configuration templates translate documented hooks;
they do not install themselves or make approval decisions. Local IPC tests have
actually replayed both helpers through petctl into a running offscreen resident.
Installed desktop/CLI hook delivery, native UX and all-app notification capture
remain separate validation; macOS's documented UserNotifications API covers our
own notifications, not a verified universal feed. Read the
[API](api/productivity-v1.md), [hook setup](api/agent-hooks.md) and
[delivery/verification record](results/2026-10-16-productivity-foundation.md).

All 92 Python tests pass. Full combined Qt run: 57 passed, four environment skips,
nine pre-existing catalogue signature/rest failures; the two socket tests separately
pass outside the sandbox. The baseline also had a character3d_joint_binding failure
which did not recur after rebuilding; no binding fix is claimed. Existing dirty
signature/character work was preserved. Restart the rebuilt resident and choose
Productivity… to try this slice. Native daily-use and battery acceptance remain open.

## Latest: first implementation across all remaining pets

All 17 remaining catalogue concepts now have original model/motion code.
**Thirteen live 3D base pets** are selectable: Pip, Biscuit, Clover, Mossback,
Rivet, Paper Heron, Acorn Custodian, Puddle, Pocket Observatory, Night Librarian,
Sprig, Hearthling and Crumb Golem. **Four working experimental families** remain
outside the selector: Pebblekin, Margin, Lantern Snail and Velvet. Their actual
anchored/crawl/morph/six-foot motion is implemented under
`experiments/catalogue-families/`; native host/drop/turn integration is pending.
Most proposed signature prop/posture actions also remain pending; these are base
characters, not a finished 18-pet catalogue.

Fixed trusted registry/generic conversion deploys the new originals. Real joint
availability controls ears/tail/arms/blinks; five bounded quiet dispositions
retain the single scheduler and legacy defaults. Native capture caught and fixed
a midstride quadruped-facing basis jump; per-swing heading now survives reversal
until touchdown. Twenty production-geometry regression scenarios pass.

Final combined Release: **61 passed/four skips** (65 total); 2D-only: **31
passed/three skips** (34 total); Python: **72 passed**. Independent runtime and
experimental reviews have no remaining findings after the paw-basis and actual
stone-contact corrections. Native Qt motion sequences and prototype render
studies are in ignored `results/catalogue-live-2026-10-16/` and
`results/catalogue-families-2026-10-16/`. Read the
[per-pet delivery record](results/2026-10-16-catalogue-base-implementation.md).
Restart the rebuilt resident for live selection. Manual desktop/monitor/Spaces
polish, signature actions, prototype promotion and energy remain separate work.

## Latest: live spatial quadruped woodland cat

The selected B cat is now **Marmalade 3D**, selectable in the rebuilt Qt resident.
Four-leg spatial motion/binding uses the shared gait, actual skinned limbs and
all-foot support footprint. Fore/hind knee planes preserve authored anatomy;
old bipeds remain compatible. Final combined Release: 48 passed/four skips;
2D-only: 31 passed/three skips; Python: 57 passed. Independent review has no
remaining findings. Native Qt rest/walk/turn/held/drop/landing and an eight-second
motion capture are inspected under `results/marmalade-live-3d-2026-10-16/`.
Restart and choose Marmalade 3D to use it. Everyday desktop/monitor/Spaces polish
and energy remain unverified. Read the
[implementation record](results/2026-10-16-spatial-quadruped.md).

## Current art direction: pixel-rendered 3D cat

The user prefers the 3D motion probe and requested art-direction work at the
existing pixel density: refined, lively, simple core forms; avoid childish
proportions and excessive detail. Three editable model studies (lean tabby,
woodland cat, graphic copper) share the procedural gait and 96×96 grid under
ignored `results/marmalade-3d-art-direction-2026-10-16/`. The user selected **B,
woodland cat**, for its strong feline outline and broader applicability. Preserve
its body/tail silhouette; the small facial refinement is preserved alongside the
original B source in `art-studies/marmalade/`. This art selection preceded the
production integration recorded above. Read the
[art-direction record](results/2026-10-16-marmalade-art-direction.md).

## Git baseline and first walking quadruped

Git now exists: `main` preserves the pre-implementation baseline `2dbd7c1`;
the new work is on `feat/quadruped-planar`. Marmalade, an original planar cat,
now loads through the validated four-role package path and uses a conservative
four-beat walk, contact-preserving turns and pickup/drop/recovery. The resident
uses every active paw for arrival and window support. Existing 3D pets remain
two-legged; no spatial quadruped adapter is claimed.

Combined Release: 42 passed/four skips; 2D-only: 31 passed/three skips; Python:
55 passed. Review caught and verified a fix for off-center-origin reversal;
no findings remain. Software rest/walk/reverse/held/drop captures are inspected.
Native movement polish, desktop acceptance and energy remain open. Restart the
rebuilt resident and select Marmalade, or use the existing character preview.
Read the [implementation record](results/2026-10-16-planar-quadruped.md) and
[plan](superpowers/plans/2026-10-16-planar-quadruped.md).

## Catalogue architecture and quadruped support started

All 18 proposed expansion pets are mapped in the
[catalogue architecture](superpowers/specs/2026-10-16-catalogue-architecture-design.md).
The [quadruped design](superpowers/specs/2026-10-16-quadruped-foundation-design.md)
defines a four-beat planar reference cat, with 3D adaptation later. Both designs
received an independent builder check. The first implemented substage adds
two/four-contact support-force overloads and count-aware neutral calibration,
with aggregate stiffness/damping normalization and independent numerical tests.
It does not yet add quadruped manifests, walking, poses or a cat package.

Combined Release: 40 passed/four skips; rebuilt 2D-only: 29 passed/three skips;
Python: 53 passed. A pre-change `resident_raven_idle3d` failure did not recur after
rebuild; no raven fix is claimed. Independent code review found no issues.
Read the [status and evidence record](results/2026-10-16-catalogue-and-quadruped-foundation.md).
Next: bounded active limb storage, four-role loader validation, gait/turn/drop
integration and all-foot resident aggregation before the first visible cat.

## Latest raven behavior: quick perching and idle window visits

Window approaches now steer promptly and start descending when the swept feet
fit one visible segment, without waiting for an exact hover. Nearby transfers
touch down in 1.5–2.3 seconds; full supported recovery takes 3.8–4.3 seconds,
down from 6.4–7.7. Banking preserves height through procedural wing effort.
Idle ravens seek a window after about eight quiet seconds, retry busy/no-window
states without hopping, and stay on an existing perch. Interaction and reactions
remain higher priority. Collision sweeps use quantized host placement, including
fractional remainders; fractional selected floors remain supported after moves.
Both native higher-window checks pass in under five seconds including recovery.
Combined Qt: 39 passed/four skips; 2D-only: 28 passed/three skips. Production app
rebuilt. Read the [quick-perching record](results/2026-10-16-raven-fast-perching.md).

## Latest raven correction: anatomy, forward flight and native bounds

The raven is 25% smaller in 2D, with visible bent hocks and aft wing strokes.
The 3D bird has clearer eyes, fuller tail feathers and weighted wing covers.
Fly up cruises forward and turns at screen edges; mid-flight retargeting and
quicker grounded turns retain fixed planted contacts. Screen constraints follow
occupied geometry. Instance-only Cocoa frame policy permits transparent padding
above the screen; measured triangle-influence bounds remove false feather
ceilings. Both native dimensions pass high dragging, forward flight, desktop
landing and perching on the higher y=300 fixture. Combined Qt: 37 passed/four
skips; 2D-only: 27 passed/three skips; Python: 52 plus one helper regression.
The app is rebuilt. Refresh results/raven-flight-2026-10-10/comparison.html or
results/raven-corrections-2026-10-16/comparison.html for rest/walk/two flight poses.
Read the [correction record](results/2026-10-16-raven-motion-and-geometry.md).
Everyday windows, multiple monitors, Spaces and energy remain acceptance work.
This supersedes the old padding-based ceiling policy in the October 12 record.

## Latest correction: PNG EXIF warnings

The werewolf GLB embeds a PNG with two malformed `eXIf` chunks. Balsam
preserved them into the runtime texture, where Qt reproduced
`libpng warning: eXIf: invalid` twice on decode. Both 3D converters now strip
unused EXIF from extracted PNG maps, retaining all other chunks byte-for-byte.
CMake tracks the shared helper so existing builds regenerate and copy clean
textures. Original GLB/Blender assets remain intact; conversion removes the
metadata each time. Regression coverage runs real Balsam conversion for both
characters and verifies unchanged image/color data. The rebuilt runtime textures
decode through Qt without the EXIF warning. Restart the resident to reload them.

## Latest behavior: cursor turns in 2D as well as 3D

Active nearby cursor attention now turns grounded 2D pets in place when the
cursor moves behind them, using the same lateral threshold and cooldown as 3D.
The existing facing blend completes smoothly; navigation and planted feet stay
fixed, and held attention returns to sparse polling. Both planar characters and
both headings have regression coverage. Read the
[planar cursor record](results/2026-10-12-planar-cursor-turning.md).
Fresh complete Release: 37 Qt passed, four environment skips, zero failures;
50 Python passed. This supersedes earlier integrated failure counts below.
Native appearance needs review after restarting the rebuilt resident.

## Latest behavior: occasional eye blinks

The resident now blinks across the current 2D/3D pets, with seeded 7–16 second
quiet intervals and a 190 ms close/hold/reopen. A shared Qt-free driver uses the
existing timer and never starts limb-motion ticks. Suppression freezes progress;
character selection resets to open eyes. Eye artwork is separate in 2D; 3D has
two dedicated eye joints (Werewolf 22 total, Raven 20). Open appearance and
unrelated anatomy are preserved. Read the [blink record](results/2026-10-12-eye-blinks.md).
Fresh complete Release: 37 Qt checks passed, four environment skips, zero failures;
50 Python passed. This supersedes earlier integrated failure counts below.
Native blink feel and energy remain unmeasured; restart the rebuilt resident.

## Latest correction: comfortable arrival and stable turning

The shared planner accepts existing resting soles within 35% of authored stride;
the spatial adapter keeps that anatomy-scaled radius. Travel heading is held
through arrival instead of reversing on fractional desktop position error.
Slow host-adjusted arrivals can settle within 0.75 units; continuous positioning
retains precise arrival. Pins and single-foot flight remain protected. Read the
[arrival/turning record](results/2026-10-12-comfortable-arrival-and-turning.md).
Release build and focused motion checks pass; Python: 50 passed. Integrated Qt:
35 passed, four environment skips, one `resident_raven3d` lower-support failure,
also reproduced with these arrival changes removed. Earlier powered-altitude
failure was superseded by concurrent raven/drop changes; see the record.
Native appearance needs review after restart.

## Latest implementation: procedural pickup, drop and recovery

User-approved pickup/drop is implemented for production procedural 2D pets and
the 3D werewolf. Lift and release preserves height and recent drag momentum;
shared gravity drives the window, procedural limbs acquire floor contacts,
compliant landing absorbs impact, and stronger impacts add one recovery step.
Regrab interrupts every phase. Pause/hide/sleep suspend without catch-up and
settled pets return to quiet scheduling. Native hide/show preserves placement.
Follow-up tuning uses quadratic air resistance, triples ordinary drop gravity, extends
legs before touchdown and compresses them under impact. Both werewolf versions
spread their arms during falls; body and pitch remain continuous at contact.
Read the [implementation and verification record](results/2026-10-11-pickup-drop-recovery.md).
Initial implementation passed 36 integrated Qt checks with four skips. After
quadratic-drag tuning, a clean 2D-only build passes 26 with three skips; drop and
werewolf 3D checks pass, while the combined Raven 3D powered-altitude check fails
with drag enabled or disabled. See the record for isolation evidence. Python:
50 passed. Earlier native focused drop and Spaces checks passed. Manual throw feel, multi-monitor behavior and energy remain
open. Concurrent raven and cursor work is preserved; its status is separate.


## Current work: raven flight and shared perching

The matched 2D/3D ravens now use horizontal bodies, short legs and broader,
feathered wings. Procedural lift, landing and perch departure are corrected;
spatial host rounding no longer acts as a false ceiling. Cocoa reachability
respects native window placement limits. Both native Fly up/Land/window-perch
checks pass against a separately painted test window. Expanded Release suites:
37 passed/four skips (combined), 27 passed/three skips (2D only), zero failures.
The local comparison is results/raven-flight-2026-10-10/comparison.html.
Everyday windows, Spaces and energy remain acceptance work. Read the
[implementation record](results/2026-10-10-raven-flight-and-perching.md),
[approved design](superpowers/specs/2026-10-10-raven-flight-and-perching-design.md)
and [execution plan](superpowers/plans/2026-10-10-raven-flight-and-perching.md).

## Latest behavior: nearby cursor attention

The user chose the mouse cursor as the first nearby interest target. The resident
now sends head-relative cursor observations into calm curiosity, producing
procedural gaze, a slight head tilt and ear interest within a bounded range.
It tracks without walking toward the cursor, turns the 3D body in place when the
cursor is behind its current heading, releases smoothly and respects
manual control, notifications and suppression. Sparse cursor observations use
the existing timer; a stationary gaze settles without animation ticks. Both
2D reflection and 3D camera/model orientation are handled, including held gaze
through a turn. Read the [cursor record](results/2026-10-10-nearby-cursor-attention.md).
Fresh full build and cursor regressions pass; final integrated Qt run: 26 passed,
four environment skips, zero failures. Python: 50 passed. Earlier concurrent
pickup/drop failures are resolved. Subsequent in-place cursor-facing checks pass;
a later combined rebuild awaits the concurrent raven/window-surface APIs (see
cursor record). Stationary 2D cursor captures are inspected;
native gaze appearance and energy remain open.

## Latest implementation: calm curious procedural behavior

The user approved autonomous calm curiosity, requiring every limb movement to
use the procedural system. The Qt-free activity chooser now drives quiet surveys,
independent ear twitches, tail swishes, occasional bounded exploration and 3D
reach stretches. Shared role followers accept look goals, rotations and impulses;
2D and 3D render solved poses. Production notifications also submit goals. Held
poses settle without frame ticks; pause, hide, drag, manual travel and reactions
preempt autonomy through the existing single timer. The frozen fixture keeps its
legacy behavior. Preview tools expose autonomy, seeds and motion primitives.
Read the [implementation record](results/2026-10-10-curious-procedural-behavior.md)
and [approved design](superpowers/specs/2026-10-10-curious-procedural-behavior-design.md).

Fresh full Release verification: 25 Qt checks passed, four environment skips,
and 50 Python checks passed. This supersedes the curiosity-frequency failure
reported in the 3D record below. Software captures cover seeded 2D behavior;
native appearance, Spaces acceptance and battery measurements remain open.
Restart the rebuilt resident to try autonomous behavior.

## Latest implementation: live 3D desktop resident

Startup now repairs saved grounded placements whose floor falls below the
desktop edge, preserving horizontal position and valid saved placements. This
fixes older shorter-character top-left coordinates restored into the taller 3D
pet. Host/spatial/application and native explicit/remembered startup checks pass;
the concurrently added elevated-drop release-height character test remains
failing. Read the startup floor repair section in the resident 3D record.

The user accepted the current werewolf movement and authorized desktop
integration. The 3D werewolf is now selectable in `better_pets`, sharing the
existing controls, Toolbox, sessions, preferences and scheduler. Model-first
motion now accepts host realization; the transparent window follows projected
world travel. Timerless rendering, projected masks/cards, 2D↔3D rollback,
exact saved placement and suppression are implemented. Read the
[resident 3D record](results/2026-10-10-resident-3d.md).

The resident now uses the user's requested three-quarter perspective: −35°
right / −145° left headings with a 12° top-down camera, preserving horizontal
travel and floor contacts. Mirrored headings keep the torso toward the camera
through a shorter 110° reversal; a bounded angular impulse and stronger yaw
spring quicken the body response while planted paws replant individually.
Fresh Release build succeeded; the latest expanded Qt run passed 27 and skipped
2 environment checks. The earlier curiosity assertion now passes. Resident,
3D skin/contact/projection and socket checks passed. Earlier scoped verification
passed 22 Qt checks (2 socket environment skips); 50 Python checks passed. The separate 2D-only build remains usable. Native Cocoa
resident rendering/lifecycle passed, with transparent pixels and one final frame
swap over a 500 ms settled observation. The 3D resident was relaunched visibly with the revised perspective for
user review, including the camera-facing turn. Native left-facing capture and
500 ms settled rendering check passed. Native Spaces fixture/modular hide/show placement tests failed;
manual fringe, multi-monitor and sleep/wake acceptance and energy measurements
remain open. Historical statements that resident 3D has not shipped below are
superseded by this implementation record.

## Latest desktop change: all macOS Spaces

The resident pet and attached status card now request visibility across macOS
desktop Spaces on their existing physical monitor. Native Cocoa checks pass for
both host variants, hide/show and native surface recreation; the helper preserves
geometry without polling or timers. Read the
[Spaces record](results/2026-10-10-all-spaces.md) for policy and acceptance limits.
Fresh Release: 22 Qt passed, 4 environment skips; 50 Python passed; 3 direct native
Spaces checks passed. Actual Space/fullscreen/Stage Manager switching still needs
visual acceptance after restart. A preexisting 20-pixel Cocoa Hide/Show position
shift was reproduced independently with a no-op policy and remains open.

## Latest correction: shorter foot transfer and 3D reversal wiggle

The shared extra planted pause is now 10 ms after comparing 40/20/10/0 ms.
At 2×/120 Hz this reduces excess stretch about 26–27% in 2D and 23% in 3D,
while retaining bob, strict pins and single-foot flight. Shared contact IK
smooths the straight/elastic knee transition. 3D knee poles follow paw heading,
and each paw's lifted yaw change is bounded to 0.4 radians rather than up to 0.8.
Normal swing duration stays unchanged; large reversals may need more replants.
Read the [timing/reversal record](results/2026-10-10-shorter-plant-and-reversal.md).
Fresh full build: 20 Qt passed, 3 environment skips; 50 Python passed; live
20-joint deformation/contact verification passed at 3×. Restart for native review.

## Latest correction: hard floor pins and stretch-first legs

The user requires landed soles to remain fixed until toe-off and forbids both
feet being airborne together. Both dimensions now use exact contact targets and
elastic leg IK, with actual rendered joint extension. The 2D limb artwork also
extends without enlarging paws; 3D ankle rotation preserves the sole pivot.
Shared gait alternates complete swings, and 3D turn replants block ordinary
swing admission while lifted. Body-led navigation and ordinary step duration
continue unchanged. Read the [pinned-contact record](results/2026-10-09-pinned-floor-contacts.md).
This supersedes reach projection, lifted recovery and emergency overlap below.
Fresh full build: 20 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin verification passed at speed 3. Native appearance needs review
after restarting the rebuilt binaries.

## Latest correction: fixed floor contacts, weight-transfer bob and comfortable rest

The user rejected floor sliding and requested more walking bob with gentler
one-foot/airborne failures, plus less 3D arrival tapping and investigation of
occasional leg-power loss. Shared reach recovery now lifts rather than slides;
emergency swings overlap only after half-phase/minimum launch-gap guards.
Forward touchdown and lighter damping produce repeated spring-driven bob, with
ordinary swing duration unchanged. Common support keeps damping at full leg
extension, weakens gravity on contact loss and adds downward airborne drag.
The 3D adapter smoothly maps lifted recovery, uses a 4 cm home/heading tolerance
and settles at actual equilibrium rather than forcing the original body height.
Read the [weight-transfer record](results/2026-10-08-weight-transfer-and-rest-tolerance.md).
This supersedes the previous floor-recovery/single-flight policy below.
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin check passed at speed 3. Final 2D bob is checked after startup with
repeated vertical reversals and fixed floor anchors. The exact native occasional
3D power-loss event was not reproduced; extended-contact damping and ignored
recovery mapping are corrected, with finite impulse recovery verified.

## Latest correction: shared spatial motion and alternating steps

The user requested one motion system to avoid fixing the same physics twice.
Spatial IK, axial support and neutral stance calibration now share one Qt-free
core: 2D supplies zero depth, 3D supplies actual spatial geometry. The dimensional
support fork is removed; neither profile uses upright assistance. Gravity is
independent of stance reserve and 3D gravity halved. Shared gait admission
alternates one swing at a time with 40 ms floor transfer; unreachable stance
feet explicitly recover along the floor rather than hopping together. Ordinary
timing and body-led speed remain unchanged. Ongoing-launch tests also catch
held-foot deadlocks. Read the [shared motion record](results/2026-10-08-shared-spatial-motion.md).
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live
20-joint skin verification passed at speed 3. Measured 3D hip drop is now
2.4–7.4 cm rather than over 22 cm. Native appearance remains open.

## Latest correction: planted knees, real 3D support and ankle pitch

The user clarified that 2D knees were flexing excessively throughout planted
stance, not during foot lift. They chose gentler gravity with stylized upright
2D support to preserve the existing step rhythm. Planted median extension is
now approximately 95–97%, idle 98%, with no quicker emergency swings. The 3D
werewolf separately selects strict actual-sole support, softer springs and
touchdown loading; both airborne paws fall without a height clamp. Its support
proxy matches measured limbs/paw offset, and paws pitch independently of shins.
Elevated held targets retain elbow clearance during body bounce. Read the
[contact/ankle record](results/2026-10-09-contact-support-and-ankles.md).
Fresh full build: 19 Qt passed, 3 environment skips; 50 Python passed; live skin
verification also passed at speed 3. Native appearance remains open. The support
model uses axial springs with analytical IK, not independent joint torque physics.

## Latest follow-up: stronger support and centered 3D paws

The user's native review found both 2D bodies sliding along the floor and the 3D
paws remaining ahead during travel. Spring stiffness is now stronger while body
weight is independent of that stiffness; calibrated rest lengths and a higher
stance reduce sag. Step timing and lateral navigation remain unchanged. The 3D
planner centers support around the hip projection and reduces forward landing
lead rather than carrying the authored paw offset into every step. Read the
[stronger-support record](results/2026-10-08-stronger-leg-support.md) for evidence.

## Current work: vertical leg support and 3D resident design

The user returned to animation: keep body-led horizontal travel, allow the body
to bounce with spring-like leg support, smooth lateral acceleration/braking
(explicitly clarified as lateral, not foot speed), and include the 3D werewolf.
The shared controller now has damped vertical support from leg compression,
smooth contact loading and eased lateral navigation. The 3D adapter has continuous
reach recovery and interrupted-turn contact continuity. Read the
[spring-support record](results/2026-10-08-spring-supported-motion.md) for final
verification and limitations. Movement review takes priority over functionality.

The user also requested figuring out resident 3D integration and clarified that
the window should follow the model. The [3D resident plan](3d-resident-plan.md)
implements that ownership in its proposed coordinate contract: world-space model
motion, projected window following, and host realization feedback. It reuses the
existing resident, controls and cards. Resident 3D implementation has not shipped.

## Functionality work resumed

The user reports the body-led correction is looking good and requests returning
to functionality iteration. Movement quality is accepted for continuing feature
work; the feature pause below is historical. Open source and Acknowledge were
also verified by the user. The current utility foundation includes command-run
outcomes, shared notification/inbox cards and the summoned Toolbox. No coding/chat
adapter, pinned app/project shortcuts or timer/reminder provider exists yet.
The next functionality priority is being selected with the user; real work
notifications are the recommendation. This acceptance does not establish battery
measurements or unexecuted native GPU/socket test results.

## Latest correction: body-led movement

The user prefers the body to move smoothly with legs catching up, and reports
Mallow remains too slow and the werewolf jerks after stride-only tuning. Travel
speed now drives body navigation directly, without foot/cadence caps. Independent
feet recover inside their anatomical reach; recovery steps may overlap at high
speeds. A constant stance baseline replaces reach-driven torso drops. The 3D
adapter also keeps translation moving during turns, with foot-side/actual-reach
recovery instead of stopping or lowering the pelvis. Read the
[body-led record](results/2026-10-08-body-led-motion.md). Native feel remains open.
This supersedes the speed/stride-only corrections below; features remain paused.

## Previous correction: longer steps at unchanged rhythm

The user clarified faster travel should come from larger steps, not quicker
stepping. The control is now Stride length in Toolbox and both previews, retaining
saved values and default2. Swing/support, turns and follower timing stay fixed;
stride distance drives sustainable travel speed within leg reach. Read the
[stride correction](results/2026-10-08-stride-distance-correction.md).
This supersedes the timing scaling described in the prior speed record.
Fresh Release: 19 Qt checks passed, 3 environment skips; 50 Python passed.
User judgment of the new feel remains open; features are still paused.

## Latest correction: adjustable walking speed

The user confirms the previous fixes work but still finds motion robotic.
Walking speed is now adjustable from 0.5× to 3× in the resident Toolbox and both
previews, with 2× as the new default. The resident saves the setting across
restarts and character swaps. Speed coordinates navigation, foot timing,
turn replants and followers while preserving contacts and targets. Reaction
and capture time remain unchanged. Read the
[walking-speed record](results/2026-10-08-adjustable-walking-speed.md).
Fresh Release: 19 Qt checks passed, 3 environment skips; 50 Python passed.
Restart rebuilt binaries to access controls. The user can now judge 2×/3×
against 1×; natural-looking motion and battery acceptance are not inferred from
software checks. Feature work remains paused for character feel.

## Latest follow-up: wave wrist and werewolf knee

The user's follow-up found two gaps: the 3D wave retained the paw's rest
orientation, and the 2D werewolf left leg still had the opposite knee pole.
Both authored 2D knees now flex forward and reverse together. The raised 3D
wave pitches the paw 180 degrees with a rotated grip offset, forward clearance
and controlled elbow bend. Read the
[wave/knee follow-up](results/2026-10-08-wave-and-knee-corrections.md).
The prior 3D gait changes were already included; wave pose changes are this pass.
Fresh Release verification: 19 Qt passed, 3 environment skips; 50 Python passed.
Native visual acceptance remains open. Feature development remains paused.

## Previous correction: clipping and gait

Feature development is paused for character fixes. Both 2D pets now use grounded
walking, with fuller strides, immediate torso balance and direction-aware knees.
Mallow has shallow forward knee artwork; Bramble supports its unequal authored
limbs and turns its visual branch. The native mask includes posed opaque artwork
instead of cropping it to the grab polygon. See the
[gait and clipping record](results/2026-10-08-gait-and-clipping-corrections.md).
Release build and checks: 18 Qt passed, 3 environment skips; 50 Python passed.
Software motion review: `results/gait-correction-after/walking-review.gif`.
Native appearance and click-through are still open; restart rebuilt binaries.

## User trial and previous correction

The user verified that Open source worked and Acknowledge dismissed the toast.
Focus loss hid the pet on macOS; the resident window policy is corrected in
[the visibility record](results/2026-10-08-visibility-correction.md).
The user also reported werewolf clipping during wave/hold/reverse walking.
Those native observations supersede any implication that earlier geometric tests
established visually correct motion. The correction adds alternating lifted foot
replants, support foot yaw, outward arm bends and a safe wave path; read the
[werewolf correction](results/2026-10-08-werewolf-corrections.md). Full rebuild:
18 Qt checks passed, 3 skipped; Python suite: 48 passed. Native visual recheck
of the corrected binaries is still outstanding.

## Latest utility implementation

The first utility slice is now implemented: real command outcomes through
`tools/pet_run.py`, interactive above-head Open source/Acknowledge cards, a summoned
Toolbox… with shared status cards and existing pet controls, reusable head/card/effect
anchors and semantic fallbacks, and werewolf arm IK/grip preview fixtures. Read the
[utility implementation record](results/2026-10-08-utility-slice.md) and
[body-fixture API](utility-body-fixtures.md). Fresh Release verification: 18 Qt checks
passed, 3 skipped; 48 Python tests passed. Live sockets, native desktop behavior,
GPU pixels and battery acceptance remain open. No coding/chat adapter exists.

## Start here

Current user goal for the fresh session: make the pets useful through an
attention inbox, a summoned toolbox and shared status cards, and add the body
fixtures needed to present them. Read
[the utility research report](research/reports/Pet%20utility%20and%20toolbox%20design.md)
and [the utility handoff](utility-handoff.md) before choosing implementation scope.
The research is complete. The latest utility implementation above supersedes
the original proposal-only status; broader launcher/timer providers remain proposals. The user requested a fresh session with this goal.

Latest direction is implemented in software: ground-only target-driven IK,
simplified Mallow art, second-order personality followers, resident ground-motion
realization and a separate original low-poly Blender werewolf with optional
Qt Quick 3D preview. Read the [grounded record](results/2026-10-08-grounded-characters.md)
and [roadmap](superpowers/plans/2026-10-08-grounded-characters-roadmap.md).
Mallow defaults to CPU composition/nearest sampling with palette dither off.
Its latest polish adds anticipatory stepping, a slight quarter-view silhouette
and a 0.18-second visual turn that preserves planted feet. The werewolf preview
shares anticipatory planning and has corrected camera-projected target controls
and optional contact markers. See the [polish record](results/2026-10-08-motion-polish.md).
The local SDK now includes Quick3D, ShaderTools and QuickTimeline. The live skin's
20 joint bindings and world contacts are software-verified, but native GPU pixels,
transparency, quiet rendering and resource comparison remain open. The resident
remains 2D. Dweller supplied broad visual reference; the new werewolf mesh and
texture are original, with no copied Dweller assets.

Latest work: the approved [procedural pixel character design](superpowers/specs/2026-10-08-procedural-pixel-characters-design.md)
now has a software implementation. Read the [implementation record](results/2026-10-08-pixel-characters.md)
and [character format](characters-v1.md). The resident defaults to Mallow and can
switch to Bramble; `build/qt/character_preview` provides interactive action/style/
force inspection. This user-directed character work proceeded before completion
of stage-3 native/resource acceptance. It does not close those outstanding checks.

The user wants a lightweight desktop pet that wanders and eventually interacts
with objects, uses smooth procedural animation and authored artwork, and has
highly modular components plus stable interfaces for arbitrary applications.
Battery life is a primary constraint. Qt Quick is the chosen implementation
direction after the prototype comparison.

The latest request authorized implementation of the approved resident-companion
specification. The
[resident companion specification](superpowers/specs/2026-10-08-resident-companion-design.md)
now defines scope, behavior, interfaces, resource requirements and acceptance.
The resident foundation and stage-2 session notifications are implemented with
software tests. Read the [session-input record](results/2026-10-08-session-inputs.md)
and [API contract](api/session-v1.md). Live socket acceptance remains outstanding
because this execution sandbox denies socket binding. The user reports generally
good desktop behavior with an open cross-monitor snapping defect in the
[resident foundation record](results/2026-10-08-resident-foundation.md).
Read [architecture.md](architecture.md)
for the broader direction and [the findings](results/2026-10-08-comparison.md)
for measured evidence.

## What works now

- A C++17 Qt 6.10.3 Release prototype in `qt/`, with an authored 12-part character,
  breathing/gaze/tail/gait motion, transparent window, drag handler, right-click
  quit, timed scenarios, telemetry, pose export, and untimed screenshot support.
- A fixed-step Python reference and 44 golden poses in `shared/`. Do not turn
  this frozen fixture into the production package specification.
- A resource profiler in `bench/` and local setup/build/config tools in `tools/`.
  The default profile is one Qt build bracketed by no-pet baselines. Multi-build
  comparisons remain supported for future regressions.
- A Qt-free deterministic runtime, a desktop host with screen-safe placement,
  atomic preferences, and `better_pets`: a resident shell with menu controls,
  one next-deadline timer, a per-state-directory instance lock and a macOS
  sleep/wake observer. Default assets are copied next to the executable.
- Bounded Qt-free session state, arbitration, TTL/sequence handling, acknowledgment,
  suppression and short reactions, with a plain-text pet label and session menu.
- Same-user local JSON-RPC transport, `petctl`, and a simulated two-source example.
  Protocol policy and shell composition are tested separately from live sockets.
- Python regression tests and Qt CTest targets; results distinguish software
  verification, skipped transport checks and outstanding native acceptance.

`pet_qt` remains the timed benchmark harness with its original CLI and telemetry.
`better_pets` runs without benchmark arguments, launches visibly, retains pause
and placement, and provides Show/Hide, Pause/Resume, session status/acknowledgment
and Quit. It uses the current fixture and renderer without frame-history telemetry.
There is a separate version-1 data-only character loader, Qt-free `pet_character_core`,
bone/attachment/controller interfaces, original soft and articulated example
packages, retained nearest-filtered 2D renderer and force-aware resident path.
The frozen fixture path remains available through `--assets`. There is still no
real coding-tool adapter, object-aware behavior or QML product UI. Utility cards
use ordinary Qt Widgets with native buttons. `pet_motion`
still depends on Qt Core; `pet_runtime` and `pet_character_core` are Qt-free.

## Decisions to carry forward

1. Keep Qt as the presentation/desktop implementation; keep future core values,
   character formats and external messages independent of Qt.
2. Use C++ for runtime logic/rendering and QML for future UI/editor. Keep C++17
   unless a concrete need justifies changing it. Do not add Rust or a browser
   runtime just to achieve modularity.
3. Prefer typed internal interfaces in one application. Keep JSON/IPC at external
   boundaries. Modular does not require a process or dynamic plugin for each part.
4. Maintain the version-1 JSON-RPC session API and CLI contract. Version 1 is
   additive; incompatible semantics require a new major. Subscriptions and
   semantic world objects remain deferred until a feature needs them.
5. Use data-only character packages separating art, anatomy, motion parameters,
   interaction geometry and personality. Different body types need compatible
   controllers and semantic bindings, not one mandatory skeleton.
6. Expose desktop capabilities explicitly. Qt cannot remove Wayland restrictions
   on global placement and observation. Windows/Linux are not validated.
7. Stop unnecessary scheduling when frozen/hidden; observe desktop changes
   independently from rendering. Battery impact still needs measurement.

## What the benchmark actually established

The user completed 24/24 native candidate runs on an M4 Pro Mac. Qt used roughly
81 MiB RSS while visible versus Godot's 273–279 MiB, with about 18% lower animated
idle CPU and 23% lower walking CPU. CPU 100% means one logical core. These are
screening observations about the tested builds, not battery-life percentages.

The user confirmed dragging Qt during timing and being unable to drag Godot.
They described Qt as sharper, Godot slightly fuzzy, Godot hidden as an empty
transparent background, and Qt as moving its window. Dragging explains logged
movement in stationary Qt trials. `walk-desktop` deliberately moves its window;
an idle pet should remain in place unless dragged. Do not reinterpret the user's
visual observations as a controlled equivalence test or a proven engine defect.

Both simulations ran near 60 Hz, but walking render-event counts differed and
measure different stages. Hidden starts hidden rather than hiding after use.
Whole-system power, wakeups, physical footprint and WindowServer measurements
were unavailable. No battery target is passed, and archived runs remain marked
`comparison_eligible=false`.

## Cleanup and preserved evidence

- Removed active `godot/`, `tools/build_godot.py`, Godot bootstrap/config choices,
  Godot downloads/toolchain/templates and exported applications.
- Archived the old comparison source, setup/config, plans/reviews, and validation
  records in `docs/archive/toolkit-study/source-and-validation.tar.gz`. Its
  manifest contains SHA256 checksums. Restore only into a separate directory if
  needed; old plans are historical, not current instructions.
- Moved original native records unchanged to `docs/evidence/native-screening/`.
  Embedded absolute paths describe their old location. New runs go under ignored
  `results/`, never over preserved evidence.
- Kept the installed Qt SDK, Python environment, shared fixture, Qt renderer and
  profiling safeguards. Qt still needs its local SDK to rebuild.
- Replaced the comparison-focused README with current setup, preview, tests and
  resource-profile commands. Added root `AGENTS.md` to direct new sessions here.

## Suggested next session

For character polish, launch `build/qt/character_preview` to inspect target-driven
Mallow, or `build/qt/characters3d/character_preview_3d` from a normal Terminal to
validate the original Blender werewolf's native pixels. Inspect actual-size walk,
stop, reversal and force response before increasing art detail. The 2D locomotion
format now supports unequal segments with fold-radius validation; both production
2D characters use target-driven grounded walking. Native quiet CPU, soak, mixed-DPI,
focus/click-through checks, energy and monitor snapping remain independent work.
Keep resident 3D integration gated on native functional/resource evidence.

Start with the session-input record and API contract. Run the socket/CLI tests in
a normal Terminal, then exercise the simulated example and native checklist.
Trace and fix monitor-crossing snapping before closing desktop acceptance.
Proceed to stage 3 daily-use validation: quiet CPU, bounded diagnostics and soak.
Use macOS first. The current character foundation proves different anatomies;
daily-use validation and adapter extensibility remain open.
The named tool has not been selected; it does not block the resident foundation.
Preserve the golden-pose tests throughout extraction.

Native functional checks should cover pixel scaling, focus, click regions,
dragging, hide/restore, movement clipping, display changes and actual frame
presentation. Profile future changes without user interaction during timing.
Controlled whole-system battery testing follows functional validation.

Suggested opening message:

> Continue Better Pets utility and body-fixture work from `docs/utility-handoff.md`,
> `docs/handoff.md` and `docs/research/reports/Pet utility and toolbox design.md`. Develop the
> first useful notification round trip and reusable presentation fixtures while
> preserving procedural grounded motion and quiet scheduling.

## Environment and workflow

Workspace: `/Users/dizzler/Projects/Repos/better-pets`.
Qt SDK: `.deps/qt/6.10.3/macos`; resident: `build/qt/better_pets`;
benchmark: `build/qt/pet_qt`.
Character preview: `build/qt/character_preview`.
Use `.venv/bin/python` for tests; system Python may lack Pillow or psutil.
Shell commands use the user's `rtk` prefix. Commands and prerequisites are in
the [README](../README.md); the build script runs CTest automatically.

The agent execution sandbox could not create native desktop windows. Offscreen
Qt is useful for software validation only. The user successfully ran the native
benchmark from normal Terminal. Do not bypass permission boundaries or label
offscreen metrics as desktop results.

Git was initialized on 2026-10-16 at the user's request. Baseline `2dbd7c1`
preserves the project before planar quadruped implementation. New implementation
work is isolated on `feat/quadruped-planar`; historical no-Git records describe
the earlier environment and are not current workflow requirements.
The user prefers substantial independent subagent work and dislikes redundant
confirmation requests for already-authorized actions.

## Cleanup verification

Latest character implementation verification is in the
[pixel-character record](results/2026-10-08-pixel-characters.md). The cleanup
counts below are historical.

Verified after cleanup on 2026-10-08:

| Check | Observed result |
|---|---|
| Python suite (`.venv/bin/python -m unittest discover -s tests -v`) | 37 passed: 21 profiler, 12 fixture, 4 pose-verifier tests |
| Qt Release rebuild (`tools/build_qt.py`) | Passed; both CTest targets passed, including six CLI lifecycle cases |
| Fresh Qt pose export and `tools/verify.py` | All 44 reference poses match within 0.0001 |
| Qt-only bootstrap through offline locked `uv run` | Passed; Qt 6.10.3, no download needed |
| Native and offscreen config generation | One Qt candidate each; native default retained in `bench/config.local.json` |
| Single-candidate offscreen pipeline | 3/3 valid candidates plus six baselines; static, idle-motion, hidden; no paired deltas |
| Preserved study archive | SHA256 and all 392 file hashes verified |
| Moved native benchmark evidence | All 195 files verified unchanged |
| Documentation | All checked current relative links resolve |
| Independent cleanup review | No remaining must-fix findings |

Pipeline evidence is local, ignored output under
`results/cleanup-validation/longer-run/`. It explicitly reports `mode: profiling`,
`purpose: validation_only`, and unverified battery acceptance. It is not a new
desktop-performance result.

The first 0.8-second smoke window produced 2/3 valid trials: hidden root metrics
had 89.9748% coverage and were correctly rejected by the unchanged 90% threshold.
The three-second window passed all three. Both attempts are retained locally;
use several seconds for pipeline smoke checks, and meaningful longer durations
for actual resource screening.

No new native GUI or energy trial was attempted during cleanup. Stage 2 added the
generic session interface. Character formats were subsequently implemented as
recorded above; real tool adapters remain later work. The broader modular
architecture remains incremental.
