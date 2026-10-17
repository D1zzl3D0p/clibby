# Signature actions: first planted-feet pass

Fourteen live catalogue pets, including Marmalade, now have distinct short
procedural actions. The rebuilt resident exposes their named **Signature action**
in the menu and toolbox, and offers quiet automatic opportunities: first after
25 seconds, then a 90-second cooldown, with a five-second retry when busy.

| Pet | This pass | Remaining larger signature work |
| --- | --- | --- |
| Marmalade | Head turn and curled tail-tip inspection | Supported shoulder stretch and seated rest |
| Pip | Perk, head tilt and lively wag | Supported flop |
| Biscuit | Unhurried nose survey and ears | Sit and supported stretch |
| Clover | Nose dip and asymmetric ear settle | Four-contact hop |
| Mossback | Neck/head retreat toward shell | Shell-overlap and deeper retraction polish |
| Rivet | Antenna straightening and raised left-arm salute | Hand contact with antenna |
| Paper Heron | Bow with folded-wing response | Tucked resting posture |
| Acorn Custodian | Cap tilt/lift and settle | Hand-to-cap contact and owned-leaf tidying |
| Puddle | Wing tuck and head bob | Walking-specific waddle polish |
| Pocket Observatory | Deliberate lens survey | Owned star card and presentation |
| Night Librarian | Hinged greeting spread and bookmark sway | Further ribbon-following polish |
| Sprig | Independent front-leaf presentation | Further leaf responses |
| Hearthling | Ember-tail curl with head response | Supported stretch and bounded colour accent |
| Crumb Golem | Careful arm-balancing gesture | Owned crumb, reach, loss and retry |

The four experimental locomotion families remain outside the resident. No new
flight, IK reach, seated support, flop or quadruped-hop capability is claimed.

## Implementation

`SignatureMotion` is Qt-free and timer-free. Compiled profiles name actual joints,
choose bounded model-axis amplitudes and continuous finite envelopes, and return
exact zero channels after 4.2–5.8 seconds. Cancellation captures the current pose
and velocity, then follows a 0.35-second Hermite release. No action owns a root,
pelvis, spine or leg joint. Profile selection follows the fixed trusted asset
registry; a missing required appendage disables the action.

Seven original biped models now have real weighted appendages: antenna, selected
decorative arms, folded wings, cap, covers/bookmark and one succulent leaf. All
remain explicitly armless for IK capability. Re-exported source models, GLBs,
validation, provenance and measured skin influence bounds are versioned.

The controller converts model-axis rotations and translations into each current
parent frame before realizing the solved legs. The resident advances action time
through its existing motion proposal and renders once after realization. Active
actions prevent idle quiescence; completed actions release frame-rate deadlines.
Pause freezes progress, navigation and notifications release smoothly, and drag,
hide, suspend, reset and selection discard the action. Cursor/autonomy cannot
steer the pet while its signature owns behavior.

## Evidence

Core tests cover every profile, finite bounds, exact zero, endpoint continuity,
captured cancellation, invalid intervals and timing partition invariance. Real
GLB weight tests and isolated Blender deformations establish that each new joint
moves its actual geometry and leaves unrelated vertices unchanged. Direction
tests use Rivet's authored mast and mitten vectors.

The live fourteen-pet test checks actual rendered soles throughout each action,
real appendage transforms, pause, navigation cancellation and return to rest.
Quick3D retains tiny near-identity quaternion residuals: the evaluator reaches
exact zero, while live final matrices are checked within 0.001 (under a pixel at
the native density). An earlier stricter matrix check and the old verifier's
paused-but-stepping setup were corrected after reproducing their failures.

Native captures and assembled review material are in ignored
`results/signature-actions-2026-10-16/`: fourteen seven-second clips, 105 native
frames each, a motion board, overview and inspected contact sheet. All 1,470
frames have nonempty alpha bounds inside the 96-pixel grid, with no edge clipping.
This pass favors restrained gestures; deeper postures and props remain as listed.

Independent review found no actionable signature-specific issues after the
corrections, and reran the action core, Rivet, Librarian and original wolf binding.
Final combined workspace Release suite: **66 passed, four environment skips**
(70 total); 2D-only: **33 passed, three skips** (36 total); Python discovery:
**92 passed**. These totals include concurrent productivity work in the shared
checkout. After the final preview-autonomy change, all **17** focused signature,
catalogue and wolf-binding checks passed again. Desktop monitor/Spaces acceptance
and energy measurements remain separate work.

The staged signature-only snapshot was also exported to an ignored verification
copy, built independently of the concurrent productivity changes, and passed all
17 focused checks. This caught and corrected a partial-staging line-position
error before committing; the working implementation was unaffected.
