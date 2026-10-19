# Shared companion polish — 2026-10-17

The gait portion was subsequently rejected for too many turning steps and slow
travel. It is superseded by the [natural-turning correction](2026-10-17-natural-turning.md).
Pickup and shoulder corrections remain implemented; measurements below describe
this earlier checkpoint.

## Changes

The spatial gait now derives speed, stride and clearance from measured leg
reach and contact count. Travel speed changes cadence as well as translation,
so short-legged quadrupeds no longer try to cover a biped's distance between
paw lifts. A modest propulsion envelope follows the active step phase.

Large reversals brake travel while the companion performs quick sequential
replants. The turn budget is .55 radians per lift, with bounded torso yaw and
stable planted world contacts. Existing lifted travel trajectories finish in
their original heading frame. The initial-turn regression reaches its new
heading in about .71 seconds for the reference biped and 1.11 seconds for Biscuit,
without translating backwards first. Moving reversals retain bounded braking.

Body support uses a common reachable-height limit from the actual spatial legs.
This limit feeds back into the physical support state; it is not a visual
correction applied after independent physics. Existing compliant support now
produces readable height variation as feet transfer weight. Quiet motion stays
quiet if callers continue supplying intervals after settlement.

Pickup regions follow live pelvis, spine and head envelopes with bounded
connections. Biscuit's head/front torso and the other quadrupeds no longer
depend on a narrow upright pelvis ellipse. Remote tails, wings and props do not
define the core grab target. These remain approximate interaction envelopes,
not an alpha-pixel mask.

All twelve original human companions have regenerated source assets with
continuous sloped upper chests, seated shoulder sockets and less elbow splay.
Alden's armour and Oona's cuffs follow the revised anatomy. Intentional broad
and slender builds remain distinct. The final companion additions arriving in
parallel are separate work.

## Evidence

- Test-first failures reproduced Biscuit mouse-pickup rejection and visible
  catalogue core-joint rejection, excessive human arm splay, and wrong-way
  travel (48.34 cm before turning in the reference biped).
- New permanent motion tests cover measured link reach, readable height change,
  planted feet, initial and moving reversals, repeated proposal immutability,
  fractional pixel placement, host blocking and scheduling settlement.
- Independent actual-anatomy harness: all **34 current rigs**, at .5× and 3×,
  at both 120 Hz and 30 Hz, arrive and settle with at most **0.309%** link
  extension. Mid-travel pickup/drop also settles in all 68 anatomy/speed cases.
  These leg-only harnesses do not replace arm/prop or flight tests.
- Actual Biscuit head and torso mouse press/move/release passes through the
  resident host. Catalogue pickup checks cover rest, reversed headings and a
  signature pose.
- Native Qt captures of Biscuit, Nell, Paper Heron and Marmalade include 240
  walking/turning frames, plus rest/walk/turn snapshots, at the resident's
  96×96 nearest presentation. Representative frames and source shoulder
  comparisons were visually inspected.
- Independent scoped source review found no actionable issue.

Ignored evidence lives in `results/companion-polish-2026-10-17/`: native frames,
four walking GIFs, `motion-comparison.png`, core harness sources/results and
`gait-anatomy-summary.json`. Shoulder before/after references and measurements
are in `results/shoulder-polish-2026-10-18/` (directory date reflects generation
host time; this record uses the user's local date).

## Verification status

Full Python suite: **109 passed**. Fresh combined Release build succeeds,
including `build/qt-humanoids/Clibby.app`. Final combined CTest: **87 passed**,
five environment skips (`resident_ipc`, `resident_ipc_cli`, `menu_bar_icon`,
`resident_spaces`, `character3d_gpu_render`), no failures. The native capture
checks ran separately with graphics access and all 240 native-density walking
frames have nonempty, unclipped alpha bounds. Scoped whitespace checks pass.
The true 2D-only Release build also succeeds; its final suite has **35 passed**,
four environment skips and no failures. Its relocation check passes after
bundle deployment completes.

An earlier integration run caught a concurrently incomplete appearance-menu
test; the subsequent rebuilt full suite passes without changes to that feature
in this work. An earlier new-gait run exposed quiet-rest reactivation in
Mossback/Puddle/Sprig; freezing already-settled spatial proposals fixed it and
all three catalogue regressions now pass.

## Limits

The walk remains deterministic procedural motion, not a rigid-body muscle
simulation. Native captures establish rendered poses, not subjective acceptance
of every character's feel. Tiny leg compliance remains around full extension.
Extreme external impulses and unsupported anatomy are outside the ordinary-walk
reach measurements. No battery or cross-platform desktop parity claim follows
from this polish pass.
