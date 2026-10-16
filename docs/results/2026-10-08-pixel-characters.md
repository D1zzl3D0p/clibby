# Procedural pixel character foundation — 2026-10-08

The user approved implementing the [skeletal pixel character design](../superpowers/specs/2026-10-08-procedural-pixel-characters-design.md),
with 2D preferred if 3D preparation or runtime cost proves excessive. This work
delivers the software foundation and editable example art. It does not establish
native desktop acceptance, final character art quality or battery impact.

## Delivered behavior

- Qt-free `pet_character_core` contains arbitrary named bones, local poses,
  semantic controller bindings, actions and bounded secondary motion.
- A separate version-1 loader accepts local data-only directories, validates
  hierarchy/atlas/paths/resource limits and rejects incompatible definitions.
- Artwork is attached to bones independently of hierarchy/draw order. The head
  is an ordinary bone; different anatomy does not require the fixture skeleton.
- Original Mallow and Bramble packages contain a few authored raster pieces,
  editable Pillow shape recipes, manifest, atlas, rest preview and attribution.
  There are no animation frame sequences. Mallow has 9 bones/10 attachments;
  Bramble has 15 bones/17 attachments.
- Direct retained Qt Quick rendering is nearest-filtered by default. The preview
  also offers selectable linear piece filtering, CPU composition at 64/96/128,
  palette reduction, ordered dithering and bone/contact overlays.
- Interactive preview supports loading, seven actions, speed, pause/reset, local
  gaze and click impulses. Capture CLI exports a PNG/configuration sidecar with
  controlled time, action, gaze/impulse and effective rendering path.
- The resident defaults to Mallow, offers a Character menu, preserves selected
  manifest identity independently of directory name, and keeps sessions through
  switches. Explicit `--character` overrides saved selection; failed replacement
  retains the current pet and reports an error.
- Drag displacement drives bounded local follow-through while the host owns
  window placement. Spring deadlines share the existing application timer,
  frequent drag events cannot postpone them indefinitely, and settled springs
  stop their work. Pause freezes visible motion; hide/sleep clear dynamics and
  missed impulses never replay. Notifications do not move the window.

See the [format guide](../characters-v1.md) and [README](../../README.md) for
commands. The original benchmark, fixture path and external session messages
remain separate and compatible.

## Rendering comparison and decision

Retain direct 2D as the resident default. It reuses its atlas and scene nodes.
Low-resolution composition unifies the output pixel grid and gives a visibly
coarser silhouette at 64 than 96/128. Ordered dithering is stable and inspectable
but the current fixed channel palette makes Bramble noticeably noisy; it is
optional, not the default art treatment. These are visual observations from
software captures, not controlled desktop render-quality rankings.

The comparison implementation uses CPU painting and replaces the composition
texture on a changed visual frame. Palette/dither/debug controls select this
experimental route even with direct mode selected, and that effective route is
reported. This is not a completed efficient GPU composition implementation.
The preview's continuous inspection timer does not model resident quiet behavior.

The installed Qt 6.10.3 SDK has no checked Quick3D or ShaderTools module/qsb.
A 3D trial would require adding the module and a skinned-model authoring/import
path. No model or native 3D cost trial was performed. The user's preference and
working 2D route justify retaining 2D now; this is not evidence that 3D is
intrinsically too expensive. GPU composition and 3D remain potential follow-ups
if their visual benefits warrant measured cost.

Controlled captures were generated in new ignored directories:

- Initial: `results/pixel-characters-validation/`.
- Full matrix: `results/pixel-characters-final-rrvti4_b/`: 20 captures plus sidecars,
  covering both characters, direct/64/96/128, dither, linear filtering, impulse,
  gaze, walk and celebration.
- Final enlarged preview matrix: `results/pixel-characters-final-jlw0lhbn/`:
  the same 20 configurations on the final build, after increasing preview height
  so the werewolf also has a useful enlarged view.

Captures report actual platform/DPR/output-pixel scale and
`native_scaling_verified=false`. Actual-size and enlarged examples were visually
inspected, including Mallow's gaze and Bramble's contact/bone overlays. The sidecar
does not claim that offscreen frames establish native scaling or presentation.

## Software verification

The Release build includes `better_pets`, `pet_qt`, `petctl` and
`character_preview`. All 14 runnable CTest targets passed; two live Unix socket
targets remain explicitly skipped after the sandbox's bind denial. The new
targets cover core/loader, actual retained scene-graph rendering, seven preview
CLI cases, deterministic source generation and resident character composition.

The Python regression suite passed 37/37. A fresh fixture export matches all
44 golden poses within 0.0001. All 203 current preservation hashes match the
resident-foundation baseline; original evidence/archive checks remain unchanged.
An older toolkit archive hash for `shared/CONTRACT.md` predates cleanup and differs
from the current contract; the current preservation baseline matches.

Independent reviews found and fixed generated translation overflow, degenerate
contact correction, folder-name/identity restoration and unreachable interaction
geometry, each with reproduced regression cases. Final review also identified
cumulative transforms too large for renderer float matrices. Rest hierarchies
now reject values outside ±1e8 and dynamic poses crossing that safe bound fall
back to validated rest. The scoped final re-review closed this finding.

These tests are software evidence. Live IPC, native click-through/focus/scaling,
monitor crossing, actual sleep/wake, quiet CPU, eight-hour soak and energy/battery
acceptance are still outstanding. No native CPU/GPU/resource comparison is claimed.

## Deliberate limits

The examples are a first art iteration, ready for focused character polish.
Current soft/articulated controllers differ primarily in deformation gain and
use semantic bindings; spring stiffness/damping are built-in. Contacts use local
foot correction, with safe bind-pose fallback for unsupported inverses/limits;
there is no full limb IK, detailed elbow/hock gait or weighted mesh deformation.
Walk publishes desired velocity and previews in place; the resident does not yet
autonomously wander. No full physics engine, throwing, window-edge interaction,
global input observation, rig editor or real coding-tool adapter was added.

The previously reported native cross-monitor snapping defect remains open.
Character work proceeded independently at the user's direction and does not
claim to fix that host issue or complete stage-3 daily-use acceptance.

There is no Git repository in this workspace. A before-work source snapshot and
execution ledger remain under `.superpowers/pixel-characters/`; no commits are claimed.
