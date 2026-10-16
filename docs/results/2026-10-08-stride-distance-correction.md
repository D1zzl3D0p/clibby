# Longer strides at the same rhythm

Superseded by [body-led motion](2026-10-08-body-led-motion.md): the user prefers
body movement to drive the feet and reports the stride-limited result still jerks.

The user clarified that faster travel should come from larger steps, not faster
stepping. This supersedes the prior speed control's timing behavior. The control
is now labeled Stride length in the resident Toolbox and both previews. Its
0.5–3 range, saved preference, preview default2 and existing API/CLI identifiers
remain compatible. The 3D comparison button now says Short steps 1× rather than
claiming it restores the previous gait.

The planner alternates moving footfalls at the authored maximum swing duration
plus a fixed 40ms support-transfer interval. Landing follows the forecast body
position at swing completion, without an extra forward time offset. Larger
settings increase travel per footfall; navigation speed follows the distance
divided by the fixed interval. Acceleration, swing/support time, facing, 3D turn
replants and follower timing no longer scale with the control. Existing swing
and turn phases keep their elapsed time when it changes. Arrival realigns feet
and settles normally, without a continuing gait clock at rest.

Travel per footfall starts at the lesser of authored stride and 15% of the
shorter leg's total reach, multiplied by the setting, capped at45% reach.
Each foot advances approximately twice that distance in steady alternating
walking. This gives the control useful short-to-long range while preserving
the support reach guard and independently authored limb lengths. Literal3× the
old near-limit travel speed at unchanged cadence is not physically available
with these leg proportions; the control expresses stride distance, not that
promise. No artwork or limb proportions were stretched.

The new regression failed against the previous implementation because swing
duration shrank at higher settings. It now measures actual steady swing time,
footfall intervals, landing distances and realized travel at1/2/3. Higher settings
increase stride/travel while measured cadence stays equal. 3D checks exercise
the same fixed swing and footfall timing at0.5/1/2/3, plus live adjustment, contact
stability and reversal. Loaded production contact checks run at3. The finite
3D travel probe samples after initial acceleration; it does not infer cruise
speed from the first half-second of startup.

Software capture comparisons at identical elapsed time are in
`results/stride-distance-correction/`. Native appearance remains the user's
visual judgment. Quit and relaunch rebuilt binaries to access the revised
controls. Feature development remains paused for character feel.

Fresh full Release build and verification passed: 19 Qt checks, 3 environment
skips (local sockets/native GPU), and 50 Python tests. At the same1.1-second
software capture time, Mallow travels13.11 logical units at setting1 versus35.89
at setting3; Bramble travels21.56 versus56.99. Swing times remain respectively
0.24s and0.26s; the 3D proxy uses0.30s, each plus the same40ms transfer.
