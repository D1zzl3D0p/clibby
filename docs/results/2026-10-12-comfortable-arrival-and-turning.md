# Comfortable arrival and stable turning — 2026-10-12

The user reported unnecessary toe taps after arriving and repeated intermediate
turning poses in the soft creature. Existing planted soles should be accepted
when the stance is already comfortable.

## Cause and correction

The shared ground planner required resting soles within 0.5 units of authored
home positions. Its default now accepts 35% of the character's stride (4.2 px
for Mallow, 6.3 px for the planar werewolf). Explicit radius overrides remain
supported. The spatial adapter no longer overrides this with a fixed 4 cm;
its ordinary 26 cm planner stride now permits 9.1 cm. The separate 4 cm spatial
heading/attention threshold remains intact. Existing contacts remain pinned;
only the decision to launch another ordinary step changes.

Facing previously followed the sign of the remaining destination error every
realization. Integer desktop positions can alternate around a fractional target,
reversing the visual turn before it finishes. Requested travel now chooses the
heading once, with a 0.75-unit deadband, and realization commits that heading.
Reset, pickup and touchdown clear uncommitted heading requests along with
pending navigation. Replacing a journey with a nearby target also cancels its
uncommitted turn; regressions reproduce both cancellation cases.

Host-adjusted arrival accepts a realized root within 0.75 units once velocity is
below 2 units/s and acceleration below 4 units/s². Continuous realization retains
the original precise arrival rule. This removes indefinitely active fractional
destinations without forcing feet or body to snap to an unrealizable position.

## Verification

New regressions failed before their respective fixes: a small body adjustment
lifted an existing sole; quantized navigation reversed heading and never became
quiet; the spatial adapter unnecessarily replanted a comfortable sole.
Regressions now cover both headings, pinned contact preservation, single-foot
flight, small adjustments and fractional desktop destinations. The existing
quantized spatial edge check now accepts nearby arrival instead of exact equality.

The full Release build succeeds. Focused grounded and spatial motion checks pass;
Python's repository unittest suite passes all 50 tests. The integrated Qt run
passed 35 checks, skipped four environment checks and failed `resident_raven3d`.
The initial failure was "wing-powered flight gains altitude". Concurrent
raven/drop changes then advanced that test to "raven reaches lower support
rather than returning to upper edge", which remains the final run's failure.
Independently linked baselines with these arrival changes removed reproduce
both respective failures. Logs are in ignored
`results/arrival-turning-2026-10-12/`; concurrent raven/drop work is separate.

Native visual feel still needs review after restarting the rebuilt resident.
These motion tests do not establish desktop parity or battery impact.
