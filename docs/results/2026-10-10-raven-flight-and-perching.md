# Raven flight and shared window support

Status: revised raven models and controls implemented; final verification recorded below.

The approved [design](../superpowers/specs/2026-10-10-raven-flight-and-perching-design.md)
adds a matched 2D/3D raven, procedural hopping and wing-driven powered flight,
and identified support surfaces available to every production pet. Execution is
tracked in the [plan](../superpowers/plans/2026-10-10-raven-flight-and-perching.md).

## Shared motion

The existing Qt-free drop owner remains authoritative for airborne position and
momentum. Explicit hop launches are ballistic. Flight tracks extension, stroke
and attitude through procedural followers; bounded lift and thrust derive from
realized wing speed and extension. The body descends without sufficient force.
Landing hands incoming momentum to existing compliant leg support. Renderers
consume wing state rather than generating separate flapping animations.

The complete semantic binding set is wingShoulderLeft, wingElbowLeft,
wingWristLeft and the corresponding Right roles. Chains must be connected,
distinct, unit-scaled, body descendants with limits admitting the physical
stroke. Missing all wings leaves existing pets wingless; partial anatomy rejects.
The general data-only manifest and frozen benchmark fixtures remain separate.

Ordinary walking retains hard pins and alternating single-foot admission. Hop
and flight detach both soles. Surface reference changes preserve pending motion
and momentum. Optional absolute sole bounds constrain generated touchdowns to
the usable window segment, without moving already planted feet.

## Shared surfaces

Surface observations contain source-qualified identity, top height, visible
horizontal segments and bounded freshness. The host accepts manual placement
and descending one-way acquisition, limits edge walking by stance and releases
missing/stale supports. Small translated surfaces carry their supported pet;
larger discontinuities release it. Surface-relative destinations move along
with support rather than drawing an idle pet back to its old desktop position.

The native macOS provider requests window geometry only and excludes the pet's
own process. The [on-screen list](https://developer.apple.com/documentation/coregraphics/cgwindowlistoption/optiononscreenonly)
is ordered front to back; covered top-edge segments are subtracted. Offscreen
and other platform providers report unavailable. Native cross-application
appearance and unusual window geometry require desktop acceptance.

Visible native observations join the resident's existing timer at 500 ms;
suppression stops them. Actual flight remains active while wings stroke. Settled
perches remove motion deadlines. This changes idle observation work and does not
establish battery suitability.

## Initial controls and comparison

The resident adds Hop, Fly up, Land and Fly to a window, including Toolbox
buttons. Window support is independent of reachability: wingless pets can be
placed and walk on an edge; flight controls require wing anatomy. The first
flight path uses the desktop plane, horizontal position plus elevation.

The planar preview accepts --hop HEIGHT, --fly X,ELEVATION and --land-at SECONDS.
PNG sidecars retain elevation, phase, powered-flight flag and physical wing state.
Captures and raw test evidence belong under ignored
results/raven-flight-2026-10-10/. Native and software evidence are distinct.

## Models and comparison

The original planar package includes an editable shape recipe, layered PNGs and
runtime atlas/rig. The spatial package includes editable Blender source, a
skinned GLB, a 256×256 atlas and measured rig metadata: 2,008 triangles, 18 joints,
one material and no animation clips. Skin bindings adapt to raven anatomy while
preserving wolf selection. Flight shoulder presentation includes extension in
both dimensions, derived from the shared physical wing state.

Native Qt rest and flight captures succeeded after the initial offscreen GPU
attempt reported unavailable. The preview camera follows elevation to keep the
whole bird visible. Rest/flight comparison is saved locally at
results/raven-flight-2026-10-10/comparison.html; planar captures also show hop and
landing. Captures are appearance evidence with separate camera framing, not a
performance comparison.

## Review corrections

Independent review and reproductions corrected unreachable vertical approaches,
flights from an upper to a lower perch, mutation under suppression, departure
during landing recovery, moving relative destinations, evolving 3D sole
footprints and margin/contact translation at narrow edges. A perched takeoff
pushes off before clearing its support reference. Descent begins only when actual
soles fit; the approach adjusts to current stance and visible segment bounds.
Future touchdowns reserve the small contact margin; existing pins translate
without being rebuilt against obsolete bounds.

Both planar and spatial resident regressions pass for powered flight, hop,
pause/hide, manual placement by a wingless pet, window translation/removal,
moving-window landing recovery and travel from an upper perch to a lower one.

## Anatomy and control revision (2026-10-12)

Both original recipes now produce horizontal raven bodies, forward heads and
heavy bills, short exposed legs, wedge tails and larger layered feathered wings.
The planar atlas is 512×256 with 15 nonoverlapping layers. The spatial export
contains 2,008 triangles, 4,124 vertices, 18 joints, one material and no clips.
Expanded flight envelopes contain the new wings. Shoulder spread, axial roll,
elbow unfold and wrist orientation derive from the shared solved wing state.
The native preview camera shows more of the feather surfaces; deterministic
simulation before capture avoids GPU startup changing the sampled flight pose.

Flight gravity and initial push-off scale with wing reach rather than short
walking legs. A small target still reserves enough launch time for extension
and the first stroke. Landing reduces actuation instead of repeatedly hovering.
Host ceiling tolerance is one logical pixel converted to spatial motion units;
previously subpixel window rounding falsely zeroed upward velocity.

Cocoa constrains programmatic window placement at the screen top. Reachability
now includes the pet window's floor padding, so an impossible high perch is
rejected rather than accepted into a hover timeout. Manual snap tolerance also
cannot select support above that ceiling. No available window produces an
explanation in the resident menu action. Moving a window lower exposes a
reachable perch. Native fixture checks restrict fresh geometry observations
to a separately painted test window so unrelated windows cannot change the
asserted destination; they do not establish all-application acceptance.

## Verification ledger

- Final expanded combined Release suite: 37 passed, four environment skips,
  zero failures (41 checks). The skips cover IPC permissions, Spaces and
  offscreen GPU availability. Earlier concurrent cursor failures are superseded.
- Final 2D-only suite: 27 passed, three environment skips, zero failures
  (30 checks). Python suite previously passed all 50; native asset recipes and
  atlas checks also pass in the Qt suites.
- Both resident raven regressions cover hop, powered altitude, land, suppression,
  wingless manual placement, support translation/removal, landing recovery and
  travel from an upper to a reachable lower edge. A narrow edge near the desktop
  boundary that cannot contain the airborne stance is correctly unavailable.
- Native Cocoa checks passed Fly up, Land and flight to the separate window
  for both versions. Actual window placement and settled support height are
  asserted, with fresh native geometry. Native rest/flight GPU captures were
  regenerated and inspected; paired software captures were refreshed.
- Independent review covered wing transforms, launch and floor rebasing,
  realization tolerance and the native ceiling. The manual near-ceiling
  acquisition finding was corrected and regression tested.
- Raw logs and captures: ignored results/raven-flight-2026-10-10/, including
  anatomy-full-qt-latest.log, anatomy-full-2d-latest.log and native-*-controls.log.

Everyday application window behavior, multi-monitor/Spaces interactions and
battery impact remain desktop acceptance work. These checks establish functional
motion and controlled native perching, not energy suitability.

No Git repository exists in this workspace; no commits are claimed.
