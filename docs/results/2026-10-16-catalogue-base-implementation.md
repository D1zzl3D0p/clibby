# First implementation across the remaining catalogue

The user requested beginning all other pets after selecting the woodland cat
and its live pixel-rendered 3D implementation. All 17 remaining concepts now
have original model and motion code. Thirteen are selectable resident base pets;
four are explicitly separate working motion-family prototypes. They are not
all finished characters with completed signature actions.

## Delivery per pet

| Pet | Implemented now | Remaining distinguishing work |
| --- | --- | --- |
| Pip | Four-foot dog, pendant ears, muzzle/tail, friendly quiet profile | Supported flop; bespoke tail response tuning |
| Biscuit | Broad short-legged capybara, tiny ears, calm profile | Sit and rare stretch transitions |
| Clover | Lop rabbit, unequal limbs/haunches, cautious profile | Four-contact hop and ear-settle polish |
| Mossback | Shell-bearing tortoise, short legs, real head, calm profile | Neck retreat and shell overlap/retraction |
| Rivet | Terracotta biped robot, face/antenna, curious profile | Antenna adjustment and articulated salute |
| Paper Heron | Folded bird, thin legs, angular neck, reserved profile | Bow, tucked rest and wing goals |
| Acorn Custodian | Seed sprite, broad cap/leaf, curious profile | Cap adjustment and owned-leaf tidying |
| Puddle | Duck, broad orange feet, decorative wings, friendly profile | Duck-specific waddle and wing-tuck polish |
| Pocket Observatory | Brass/navy walking telescope, lens pivot, reserved profile | Owned star card and presentation |
| Night Librarian | Walking clothbound book, page block/ribbon, reserved profile | Cover hinges, greeting spread, ribbon follower |
| Sprig | Ceramic succulent biped with rigid leaves, calm profile | Independently targeted leaf presentation |
| Hearthling | Upright ember salamander, real tail, calm profile | Supported stretch and bounded colour accent |
| Crumb Golem | Biscuit-stone biped with decorative arms, friendly profile | Prop reach, crumb balance and retry |
| Pebblekin | Experimental anchored stack with bounded stone pivot/wobble | Resident host, support changes, pickup/drop, interaction |
| Margin | Experimental bounded mesh contour morphs for comma/exclamation | Resident deforming mesh and interaction adapter |
| Lantern Snail | Experimental continuous-foot crawl, head/stalks/door | Host realization, support transitions, turn and pickup/drop |
| Velvet | Experimental six world-pinned feet, stable tripods, wing reveal | Native six-contact adapter, turning/recovery, grooming and flight |

The live bases share walk/reverse/stop, gaze/attention, pickup/drop and quiet
settling. Blink and ear/tail responses derive from actual anatomy. A book or pot
has no invented face/ears; decorative robot arms and bird wings do not acquire
unsupported reaching or flight. Some proposed signatures require new joints,
owned props or posture/contact transitions rather than merely extra artwork.

## Shared implementation

`tools/catalogue_asset.py` exports original continuous weighted limbs, a restrained
atlas, measured influence bounds and validation/provenance. Biped and quadruped
recipes remain separate. The resident uses a fixed compiled identity/type registry
and generic Balsam conversion/deployment, retaining the existing built-in security
boundary. Rig skin names, explicit armless bipeds and optional ears/tails are
validated. Character selection cannot supply arbitrary QML paths.

Five bounded personality configurations vary rest interval and permitted action
weights without a new timer. Calm rests last 24–45 seconds; friendly, cautious,
reserved and curious configurations have their own bounded choices. Existing
three-argument behavior defaults are unchanged; an independent exact-output
comparison covered 20,000 legacy transitions. Capabilities filter every choice.

The four new families live in `experiments/catalogue-families/`. Pure immutable
evaluators drive actual authored Blender geometry and finish at an exact quiet
pose. They do not implement the resident propose/realize, suppression or host
lifecycle and are absent from the production selector. Six logical feet do not
establish insect flight; the snail never borrows an invented walking gait.

## Evidence and acceptance

New native Qt captures and 60-frame four-second motion sequences for all 13
live bases are under ignored `results/catalogue-live-2026-10-16/`. They exercise
the actual 96-pixel resident stage; sole transforms are checked against solved
contacts during capture. Blender study output for all four new families is under
`results/catalogue-families-2026-10-16/`, with 120 frames per pet and exact final
settling. Combined overview material distinguishes live bases from prototypes.

Each production pack has focused asset validation. The integration test checks
active anatomy, every rendered sole, support count, both navigation headings,
held gaze, drop, quiet rest, real role capabilities and unsupported reach/flight.
The resident selection test cycles all 13 new identities and restores the cat.
Final combined Release suite: **61 passed, four environment skips** (65 total).
2D-only Release: **31 passed, three skips** (34 total). Python unittest discovery:
**72 passed**. Independent focused experimental review reran all 12 family checks.

Independent runtime review found no actionable issues. Experimental review
caught a virtual pivot outside Pebblekin's upper mesh; the correction maintains
actual supported rocking contact within the middle stone's face, testing the
same geometry used by the renderer. The contact point can move as the stone rocks.

Native frame-bound checks also caught a midstride reversal defect shared by the
quadrupeds: an already lifted paw's planar trajectory retained its old heading,
while the spatial adapter subtracted the newly flipped fore/hind offset. Pip's
reproduction jumped 1.30734 metres in one 1/120-second step. The adapter now
retains each swing's coordinate basis through touchdown and adopts the new
heading on the next lift. Twenty production-geometry reversal scenarios cover
ordinary/repeated flips, stance pins, support translation, reset and recovery;
legacy spatial suites remain green. The corrected four-quadruped captures are
under `results/catalogue-live-2026-10-16/final/`; unaffected biped captures are
preserved there for assembly. Earlier raw failing frames remain untouched.

Native renderer captures establish pixels and transformation behavior. Everyday
desktop interaction, monitor/Spaces behavior, comparative resource use and battery
impact remain separate acceptance work. These are first implementations for
visual/behavior review, not evidence of audience preference or conversion gains.
