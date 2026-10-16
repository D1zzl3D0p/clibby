# Character directory format, version 1

The production loader reads a local directory containing `manifest.json`, a
raster atlas and optional authoring/preview/attribution files. It is separate from
the frozen `shared/` benchmark format. The supplied packages and editable source
in [characters](../characters/README.md) are working examples.

## Manifest fields

| Field | Meaning |
| --- | --- |
| `version` | Integer `1`; missing, string or other versions reject |
| `id`, `name` | Stable identity and display name; ID is 1–64 ASCII letters/digits/underscores/hyphens |
| `controller` | Built-in `soft` or `articulated`; executable content is unsupported |
| `width`, `height` | Positive design-canvas dimensions, at most 2048 each |
| `origin` | Two-number canvas anchor metadata, not an additional rendering translation |
| `atlas` | Relative local raster filename; no traversal, absolute paths or symlinks |
| `motionEnvelope` | Optional finite margin 0–256 around the canvas; default 12 |
| `bones` | Bone IDs, parent ID or null, rest transform, optional rotation limits |
| `attachments` | Bone ID, `[x,y,width,height]` atlas source, pivot, offset and integer draw order |
| `bindings` | Optional-anatomy semantic role to bone ID; `body` is required |
| `interaction` | 3–128 canvas-contained polygon vertices for the stable input region |
| `contacts` | Optional bone ID and local point, used as ground-contact anchors |

Rest transforms require numeric `x` and `y`; `rotation` defaults to 0 and
`scaleX`/`scaleY` to 1. Angles and rotation limits use radians, y increases
downward. Top-level root rest positions are canvas coordinates; children are
relative to their parent. Parents may appear after children. Bone hierarchy does
not determine attachment layering. Multiple attachments can share one bone;
bones need not have artwork. The head is an ordinary bone.

Limits are 64 bones, 64 attachments, 64 contacts, one atlas no larger than
2048×2048, 1 MiB JSON, 32 MiB total package files and 1024 directory entries.
Unsupported special files and symlinks reject, including internal symlinks.
Validate bounded finite transforms, hierarchy, references, image dimensions and
atlas regions before decoding or replacing a character. At least one attachment
and a usable interaction polygon are required. Rejected selection leaves the
live pet running.
All cumulative affine matrix coefficients and translations are bounded to ±1e8,
including rest hierarchies, to remain safely representable by renderer floats.
Motion exceeding that bound falls back to the validated bind pose.

## Controller semantics

Bindings currently recognized are `body`, `head`, `earLeft`, `earRight`, `tail`,
`footLeft` and `footRight`. Missing optional bindings omit that feature. Both
controllers receive Rest, Idle, Walk, Attend, Celebrate, Failure and Acknowledge;
the soft controller has stronger squash/breathing than the articulated controller.
Idle/Walk/Attend use elapsed time. Transient reaction time uses normalized 0–1
progress. Walk publishes desired root velocity separately; legacy packages retain in-place gait. Grounded packages instead use the optional
locomotion controller described below and explicit targets.

Spring impulses are bounded character-local observations. The first implementation
uses a damped response shared across semantic secondary-motion bindings; stiffness
and damping are built-in constants, not package-tunable yet. At most four
1/120-second substeps run per call. Tiny offsets/velocities snap to exact rest.
Apply joint limits and ground contacts after motion. Contact correction currently
uses local foot translation rather than limb IK. Degenerate contact inverses or
out-of-range corrections fall back to the bind pose rather than publishing an
invalid pose. Detailed elbow/hock gait and mesh deformation remain refinements.

## Eye presentation

Optional semantic `eyes` bindings enable occasional resident blinks. A trailing
pose openness value defaults to one; the renderer compresses only eye artwork
and descendants about their pivot, leaving skeletal/contact transforms unchanged.
Eye artwork should therefore be authored separately with a centered closure
pivot. Missing bindings leave the character unchanged. Built-in 3D assets use
separate eye deform joints for the equivalent vertical closure. Blink timing is
shared resident behavior; version-1 package data remains unchanged. See the
[blink record](results/2026-10-12-eye-blinks.md).

## Procedural expression and autonomous behavior

Production controllers expose capability-aware expression goals and impulses.
Head look uses a direction with +Y up and +Z forward; role rotation vectors use
radians (x pitch, y yaw, z tilt). Head, each ear, tail and spatial wrists have
independent bounded second-order followers. Clear returns smoothly to neutral;
a held goal can settle away from neutral. Goals are applied before joint limits
and contact solving. Production reactions submit these goals rather than changing
bones during pose reads; the frozen benchmark retains its legacy compatibility.

Built-in calm curiosity selects surveys, ear twitches, tail swishes and bounded
travel, plus reach stretches where spatial arm IK is supported. Missing optional
roles omit their activities. Personality is currently built-in for Mallow,
Bramble, Werewolf and fallback anatomy; version-1 package data is unchanged.
Explicit interaction and notification reactions take priority. See the
[implementation record](results/2026-10-10-curious-procedural-behavior.md).

## Optional wing anatomy and powered flight

Complete `wingShoulderLeft`, `wingElbowLeft`, `wingWristLeft` bindings and their
`Right` counterparts select shared procedural wing motion for grounded packages.
They identify contiguous shoulder–elbow–wrist chains descending from the body,
with distinct bones, unit-scale ancestry and rotation limits admitting the full
physical stroke. Partial/disconnected/overlapping wing anatomy rejects. Wing
art remains ordinary layered attachments; there are no animation clips or pack
scripts. No new manifest version is required for these optional bindings.

The shared airborne owner handles explicit hops and lift/thrust derived from
realized wing motion. Walking retains its ordinary planted-contact rules; hopping
and flight permit both soles to detach. Renderer-specific adapters consume the
same physical wing state. Support surfaces and reachability are separate: a
wingless pet can stand and walk on an observed window top after manual placement.
See the [raven implementation record](results/2026-10-10-raven-flight-and-perching.md).

## Selection and presentation

The resident treats sibling package directories as a catalog. It persists the
manifest ID, resolves it independently of folder names and rejects ambiguous
duplicate IDs. Selection through the running resident is restricted to that
catalog; launch with `--character ABS_DIR` for another catalog or an explicit
startup choice. The development preview can load any validated local directory.

The resident uses nearest-filtered retained sprite attachments for legacy
packages and low-resolution CPU composition for grounded packages. Its surface is
the character canvas plus the declared motion margin; input geometry remains
stable while bones deform. Preview composition renders a transparent square
64/96/128-pixel canvas, preserving aspect ratio, then enlarges with nearest
filtering. Optional palette reduction and fixed 4×4 ordered dithering act on that
output before enlargement. Transparent pixels and edge alpha are retained.

Current composition is CPU painting with changed-frame texture uploads. It is
explicitly experimental. A future GPU pass or 3D renderer can reuse action and
anatomy concepts without requiring identical 2D and 3D pose payloads.

## Optional grounded locomotion

A version-1 manifest may declare `locomotion`. Packages without it retain the
legacy pose controller. Grounded packages use stateful horizontal navigation,
world-planted contact anchors and analytical two-link IK; the `Walk` action does
not generate their foot cycle. All lengths and velocities use logical canvas/
desktop units and seconds. Grounded pose coordinates remain canvas-local.

```json
"locomotion": {
  "groundY": 125,
  "legs": [
    {"hip":"hip-l", "knee":"knee-l", "ankle":"foot-l", "contact":[0,3], "bend":1},
    {"hip":"hip-r", "knee":"knee-r", "ankle":"foot-r", "contact":[0,3], "bend":-1}
  ],
  "speed": 38,
  "acceleration": 150,
  "reachThreshold": 7,
  "stride": 10,
  "lift": 4,
  "minStepDuration": 0.12,
  "maxStepDuration": 0.28,
  "secondary": {"frequency":3, "damping":0.8, "response":0}
}
```

`groundY` and exactly two legs are required. Chain bones must be distinct across
both legs, with knee parent=hip and ankle parent=knee. Both hips descend from
the root body binding. Chain joints must permit the full [-pi,pi] rotation range;
restricted joint limits are unsupported by this analytical solver. Rest knee/ankle translation
lengths define the segments (0.1–256); all chain ancestors must have unit scales.
Contact offsets are ankle-local; ankle orientation stays horizontal after solving.
`bend` is +1 or -1, defining the forward-facing knee side. The solver reverses
this side at the edge-on midpoint of a facing change, preserving contact positions. Canvas floor must lie below both hips
within their supported reach and declared motion margin.

Other fields default to the example above except reachThreshold=10, stride=14,
lift=6. Allowed ranges: speed 0.1–500, acceleration 0.1–2000, reachThreshold and
stride 0.1–128, lift 0–64, minimum step duration 0.04–1, maximum duration from
minimum through 2 seconds. Reach threshold must be less than 75% of summed
segment length, stride less than 90%, and lift less than 75%. Secondary frequency
is 0.1–30 Hz, damping 0.1–4, response -4–4. Nonfinite parameters reject.

`GroundedController::propose(dt)` records one elapsed interval, capped at 1/30
second and proposes displacement. The host reports presented root X through
`realize(x,blocked)` exactly once. Physical anchors use world coordinates while
rendered X is `footWorldX - rootWorldX + origin.x`. A host must preserve fractional
navigation placement and report its actual presented position. `pose()` reads
state without advancing it. Rebase through `reset()` after display/package/
drag transitions. Blocked realization resolves the target to the presented root.
Arrival finishes outstanding generated steps and settles secondary trackers;
`settled()` then removes the need for an active motion timer. Reactions are
applied before IK, so contacts remain planted through body/head expression.

The retained `setSpeedMultiplier` API now controls body travel speed from0.5
to3 times the authored speed. Navigation follows its acceleration/braking policy
and desktop limits independently of feet. Per-foot anticipatory steps follow
the presented body movement. Swings alternate with a short floor transfer;
a foot cannot lift before the other has landed, including 3D turn steps.
Floor contacts stay fixed from touchdown until toe-off. Normal swing duration
does not change. Forward touchdown gives the body time to pass over a planted
foot and lets the shared springs generate visible weight-transfer bob.
The shared default extra pause after touchdown is 10 ms, reduced from 40 ms
after comparing 40/20/10/0 ms. `setFootPlantDuration` accepts 0–200 ms; at least
one foot remains grounded even at zero. This controls double-support dwell, not
the time a foot supports the entire opposite swing. Presentation intervals
quantize the actual handoff to the next available frame.
Contact targets are never projected into anatomical reach. Legs extend when
necessary to reach their fixed soles; changing knee bend and stretching take
precedence over sliding or putting both feet in the air.
Leg IK delegates to a shared spatial two-link solver. The 2D adapter embeds
its inputs with zero depth; the 3D adapter uses measured spatial limb geometry.
A narrow compliant transition retains small knee flex near full extension,
avoiding a sudden bend when an extended leg comes back inside rigid reach.
Torso support likewise uses one spatial axial-spring integrator, receiving actual
hip/ankle positions and confirmed contact loads. Renderer choice does not select
a different force equation. Character tuning controls gravity, stiffness,
damping and stance independently; gravity is not raised to enforce a stance.
Touchdown ramps the load rather than instantly reapplying full force.
Grounded damping remains active through full leg extension. Effective gravity
decreases with contact loss and downward airborne drag softens the drop, while
the unsupported body still descends. Comfortable foot positions can settle at a
different equilibrium height: rest checks quiet velocity and acceleration.
The 3D character's 4 cm home tolerance also suppresses unnecessary heading flips
and alignment steps on tiny corrections.
These remain simplified axial springs with analytical IK, not independent joint
torque physics. The 3D paws pitch independently through swing and return flat
at contact; ankle/sole offsets rotate with that pitch around the pinned sole.
The 3D knee bend plane follows the paw's world heading rather than twisting
with the torso during reversal. Each lifted paw turns at most 0.4 radians,
also staying within 0.4 radians of its supporting partner.
Head/ear expressions remain, while action-dependent torso motion does not displace
the contact geometry. Lateral acceleration and braking ease under a jerk limit;
feet still cannot cap horizontal body travel. Swing position uses quintic easing
and a lift arc with zero endpoint velocity and acceleration.
Planted contacts stay fixed regardless of reach, and arrival realigns feet before
settling. This is procedural visual following, not a force-based walking model.

Upper/lower links may have different lengths. The declared floor and maximum
swing lift must leave the ankle below the inner fold radius (the absolute
difference between segment lengths). Invalid folded-reach tuning rejects before
activation. The rigid solver preserves authored lengths for arms; grounded leg
IK uses its contact-preserving mode. Both rendered skeletons extend their links
to match that solution, keeping the actual soles at the planned world contacts.
Head, ears and tail use separate second-order state: frequency multipliers are
0.7, 0.65 and 0.45 relative to body tuning; ear damping multiplies by 1.2 and tail
damping by 0.75, within the documented parameter bounds. Their targets derive
from realized navigation intent/body lean and stop at rest; no gait clock drives
their motion.

## Visual facing for grounded characters

The optional `facing` semantic binding identifies a visual-only branch below
the body. Attach body artwork, head, ears and tail there; IK legs remain outside
it. This branch must not appear in IK ancestry. Authored rest transforms retain
positive scales. Runtime poses alone allow this branch's horizontal scale in
[-1,1], for a 0.18-second direction turn. Root placement and planted contacts are never mirrored. Mallow's asymmetric
eye placement supplies the slight quarter view.

Foot planning considers predicted acceleration/braking over a bounded horizon,
starts swings before support reaches its limit and predicts touchdown position.
Reach informs landing placement, without moving existing contacts or limiting
body navigation at extreme speed.
