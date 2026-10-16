# Quadruped foundation and first cat

Status: builder-reviewed design, 16 October 2026. The planar loader, gait, pose and resident slice is implemented with Marmalade. Native acceptance and the spatial adapter remain open. See the [implementation record](../../results/2026-10-16-planar-quadruped.md) and [earlier support foundation](../../results/2026-10-16-catalogue-and-quadruped-foundation.md).

Parent: [catalogue architecture](2026-10-16-catalogue-architecture-design.md).

## Objective and first deliverable

Support a genuine four-contact planar pet through the existing resident: walk to a target, stop, reverse, follow the cursor with its head, blink, be picked up/dropped, stand on a sufficiently wide observed support and settle without continuous motion ticks. Use an original minimal Marmalade cat as the reference, pending the user's optional reference-pet preference. Keep all existing two-leg behavior and existing 3D pets working.

The body remains driven by realized navigation, with generated planted steps and analytical IK. This is procedural visual motion with compliant support, not a new rigid-body dynamics engine. Four legs are never implemented as two unrelated biped planners or as forelegs disguised as wrists.

## Compatibility and package contract

Retain directory manifest version `1` and existing `soft`/`articulated` controller selection. Extend only `locomotion`:

- Absent `gait` means the existing biped policy and exactly two legs, in current order. Existing manifests and defaults retain their meaning.
- `gait: "quadrupedWalk"` requires exactly four chains and exactly one of each `role`: `foreLeft`, `foreRight`, `hindLeft`, `hindRight`. Input order is immaterial; normalize to that canonical order at load time.
- Two-leg manifests may explicitly declare `gait: "biped"`; optional leg roles, if present, must match the existing left/right order. No implicit four-leg policy is inferred from count alone.
- Reject unknown gait IDs, missing/duplicate roles, wrong family/count combinations, partial chains, shared joint bones, unsupported scales/limits, invalid contact offsets and nonfinite/out-of-range tuning before activation. Six legs remain unsupported. Selection failure preserves the current pet.
- The chain keys stay `hip`, `knee`, `ankle`, `contact`, `bend` for compatibility. On a foreleg, `hip` means the proximal shoulder attachment; it does not imply human anatomy.
- All existing bounds, two-link geometry validation, full-range joint-limit rule, unit-scale ancestry, motion envelope and data-only restrictions continue to apply to every chain. Ancestor branches outside the two-link chain may be shared, but a chain joint cannot be another chain's ancestor.

Old loaders reject four-leg arrays safely through their existing exactly-two-leg check. This is an additive reader extension, not a promise that older runtimes can run new packs. No changed session-v1 semantics or pack scripts are introduced. Update `docs/characters-v1.md` with explicit compatibility examples when implemented.

For the first cat use standing anatomy whose neutral and swing targets pass the current fold/reach rules, head/ears/tail and separate eye artwork. It may use existing blinks and expression primitives. Seated rest, shoulder stretch and tail inspection are future actions, not acceptance requirements for four-legged walking.

## Bounded anatomy and state

Use a Qt-free fixed-capacity four-element limb container with an active count of two or four for planner anatomy, foot state, geometry, contact flags, loads and per-leg trackers. Its `size()`, range iteration and indexed access must refer only to active elements; capacity slots cannot appear as phantom feet. Choose the exact helper name during implementation planning. Do not resize vectors or allocate new foot containers each frame or each `state()` read.

Default construction and legacy two-leg paths retain two active elements. Preserve existing two-element support entry points through compatibility overloads while adapters migrate. Existing array initializations must be explicitly handled, not silently converted into four active entries. Leave expressive wrists, ears and wing pairs as pairs: they are not walking-foot collections.

The outward `MotionProposal` and `GroundedController::propose/realize`, target, pickup/hold/release, reconcile and support-frame responsibilities stay the same. Add pure aggregate queries for “any active foot swinging” and “active sole extent” so resident callers do not encode leg counts. Pose/state reads do not advance dynamics.

Only active legs participate in reach calibration, settling, contact installation, touchdown bounds and recovery. A standing quadruped reports four genuine contacts. An airborne quadruped reports no support loads and prepares all four legs for landing; it does not borrow biped index-based left/right hanging poses.

## Walking and turning

The quadruped policy uses the deterministic cycle **hindLeft → foreLeft → hindRight → foreRight**. Start from a stable all-contact stance. A leg starts an ordinary swing only after the previous swing has landed and the configured plant dwell is satisfied. At most one ordinary swinging foot exists; at least three other confirmed supporting contacts are required for admission. Recovery/airborne phases have their own rules and are not disguised as ordinary gait.

Choose the next eligible leg in cyclic order from predictive reach/placement demand; never skip indefinitely, and do not run a gait clock while stationary. Touchdown is based on realized root movement and the existing bounded prediction model. Keep each stance sole fixed in world space from touchdown to toe-off, including blocked/quantized host placement. Use existing quintic swing easing and two-link IK. Preserve endpoint contacts through extension rather than sliding them to fit reach.

Arrival finishes outstanding steps and necessary stance realignment, then clears active motion deadlines. No unsupported foot is launched solely to chase insignificant root rounding error. Biped admission, arrival tolerances, plant duration and stretch behavior retain their current policy.

Role identity is anatomical and never swaps during reversal. Fore/hind proximal offsets must turn with the horizontal body, rather than leaving the shoulders behind the pelvis while only the artwork mirrors. The planar adapter reflects/interpolates quadruped proximal positions about the authored body anchor through the existing facing transition, recomputes local joint transforms, and solves each chain back to its unchanged world sole. The visual-only facing branch remains outside IK ancestry. Preserve per-chain bend signs relative to facing; the discrete 2D branch change remains concealed at edge-on passage as in existing motion.

Do not rotate all four planted soles with the torso. New touchdown homes follow the new heading through ordinary one-at-a-time replants. Inspect the cat at actual size for cross-over, far-leg occlusion and apparent foot sliding before calling the turn visually accepted.

## Support normalization

Generalize the shared support solver to an explicit validated active count. Do not merely replace `load * .5` with a larger sum while keeping four full-strength leg springs.

For active count `N`, retain the meaning of current per-character support stiffness `k` and damping `d` by using effective per-leg stiffness `k * 2/N` and damping `d * 2/N`. Calibrate each leg's neutral rest length with supported weight `gravity/N` and that effective stiffness. Compute contact fraction as `sum(confirmed loads)/N`, clamped to `[0,1]`. Average neutral body-lowering contributions by `1/N`. Loads remain bounded in `[0,1]` and touchdown ramps them as today.

For `N=2` these reduce exactly to current tuning and gravity/2 calibration. For equivalent four-leg geometry the aggregate spring/damping strength and neutral equilibrium do not double merely because the contact count doubled. All effective normalization lives at one shared force-law/calibration boundary; neither planar nor spatial adapters apply an additional factor.

Retain the existing bounded vertical integrator, gravity/contact-loss behavior and analytical limb solve. First-slice quadrupeds use one torso support state plus current bounded lean. Independent fore/hind support and spine/pitch dynamics are later improvements requiring their own evidence; do not silently claim a full support-polygon stability model from three stance pins.

## Pickup, drop and surfaces

Reuse the shared airborne state owner and host-owned placement. Pickup detaches all four support contacts; hold follows realized position; release preserves height and recent drag momentum. Landing installs four bounded contact targets, ramps loads and permits at most one recovery replant at a time. All-contact geometric installation may occur at touchdown as today; subsequent recovery is not a four-leg simultaneous walking swing. Regrab interrupts every phase.

New touchdown targets must fit the usable support segment as well as current package bounds. The host continues consuming an enclosing sole interval. Aggregate **all active feet**, with the current sole-radius margin, for resident footprint and arrival checks. A window fitting only the middle two paws is insufficient. Selected support movement carries all stance pins; support loss hands motion to gravity. Preserve freshness, ownership, fractional root placement and swept-landing rules already implemented for surfaces.

Pause/hide/sleep suspend motion without catch-up and without independent timers. Settled contact/secondary state removes motion deadlines; existing sparse attention/blink/session/surface deadlines remain governed by their existing policies.

## Implementation boundaries

Likely change sites, to be confirmed by the implementation plan:

- `qt/characters/character.h`, `character.cpp`, `loader.cpp`: bounded active anatomy and family/role validation.
- `qt/characters/grounded.h`, `grounded.cpp`: family-specific admission, per-leg state, four-leg pose/turn/drop/recovery, aggregate queries.
- `qt/characters/motion_support.h`, `motion_support.cpp`: count-aware force/calibration and two-leg overload compatibility.
- `qt/resident/application.cpp`: active-foot arrival and sole footprint aggregation.
- `qt/preview/`: expose a validated cat through the existing preview path, only where needed.
- `characters/` plus `tools/author_characters.py` or a focused new recipe: original editable cat and runtime atlas/manifest/preview/license.
- `qt/CMakeLists.txt`: focused test registration and packaging of the new reference pet.

Audit fixed-pair callers in `qt/characters3d/pose.cpp`, `qt/resident/spatial_character.cpp` and `qt/preview3d/controller.cpp` for compatibility with shared changes. Keep their actual walking anatomy two-legged. The resident must not route a four-leg atlas pack through a known two-leg spatial rig. A new spatial quadruped pose/binding/import adapter is a separate second slice, sharing roles, solver and force semantics but not fabricating a 3D cat now.

## Acceptance evidence

Write meaningful regressions around behavior, not only container shape:

1. Existing manifests load and existing biped motion/flight/expression/drop cases retain behavior. Malformed four-leg packs reject atomically, including missing roles, shared joints and folded/unreachable swing tuning.
2. A synthetic quadruped with unequal fore/hind links has finite poses and four distinct contacts. Under travel every leg steps, at most one ordinary swing occurs, and other world pins stay fixed through quantized/blocked host movement.
3. Reverse at rest and in travel: identities stay fixed; proximal fore/hind geometry follows facing; contact pins remain fixed until toe-off; settling completes. Compare actual rendered sole anchors against planner contacts, not planner values alone.
4. Equivalent two/four-contact support has matching aggregate equilibrium/dynamics within numerical tolerance. Partial-contact damping, zero-contact fall and landing stay bounded; legacy two-leg force results stay unchanged.
5. Pickup/release/regrab, suppression during swing/recovery, support movement/loss, narrow support rejection and fractional floor changes cover all four feet. Reconcile and character changes do not retain stale contact state.
6. Once at rest, repeated pose/state reads are stable and motion does not keep the application timer active. Old spatial characters remain two-legged with no phantom default-capacity contacts.

Use existing `grounded_tests.cpp`, `motion3d_tests.cpp`, `drop_tests.cpp`, character validation tests and resident character/surface tests, splitting a dedicated quadruped test file if clearer. Record baseline failures before changes. Verify combined and 2D-only Qt configurations plus relevant Python tests and the frozen golden-pose contract. All shell commands retain `rtk`; Python checks use `.venv/bin/python`.

Preview captures and native actual-size walk/turn/drop review are required for visual acceptance. Native window placement and monitor/Spaces checks remain separate from offscreen software tests. Put new captures under new ignored `results/` names; preserve historical evidence. No CPU, battery or cross-platform claim is implied.

## Stages and completion boundary

1. Bounded anatomy/support foundation with existing biped compatibility and synthetic four-leg tests.
2. Four-beat planar gait, turn/drop/recovery and resident footprint integration.
3. Minimal original cat package, full regressions and clear preview/native acceptance record.
4. Reuse with Pip/Biscuit and challenge short/unequal limbs with Mossback/Clover; scope posture/hop actions independently.
5. Spatial four-leg adapter and original 3D reference art after the shared foundation is proven.

The first three stages constitute the first implementation slice. Final artwork polish, seated/flop transitions, trot/run/bounding, autonomous hopping, spine physics, six-leg planning and new integration protocols are outside it. A passing software suite establishes software behavior; it does not close native visual or energy acceptance.
