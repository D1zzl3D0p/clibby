# Planar cursor attention turns — 2026-10-12

The user requested that 2D pets turn toward a cursor behind them while actively
looking at it, matching the existing 3D cursor behavior.

The resident now routes the existing attention-facing request to either the
spatial character or the grounded planar controller. Both use the same
head-relative lateral threshold (35% of character width), 800 ms cooldown,
proximity policy and suppression for manual control, reactions, dragging,
pause, hide, sleep and airborne motion.

`GroundedController::face` changes only the requested heading. It leaves the
navigation target, root and foot contacts unchanged; realization uses the
existing 180 ms facing blend. Pending heading differences count as active motion
so the existing single timer wakes for a turn. Repeated requests for the same
heading do not wake animation. Held gaze is converted through the current
reflection during the turn, as it was previously during walking reversals.

The resident regression failed before implementation because the planar pet
did not turn toward the cursor behind it. Tests now cover both soft-creature and
planar werewolf, both headings, completed turns, unchanged position and target,
pinned soles, quiet holds and center jitter. The original head-only gaze test
uses a 25%-width offset inside the turning threshold so it continues to test
gaze in each fixed facing. Quiet polling is checked after the next scheduled
sample, rather than midway through an existing 100 ms interval.

Grounded unit checks also cover unchanged navigation, repeated and invalid
requests, settling in either heading and rejection while held. Independent
production review found no actionable issues.

Fresh complete Release build succeeds. Integrated Qt: 37 passed, four environment
skips, zero failures (41 registered checks, including concurrent blink work).
Python unittest suite: 50 passed. This combined run supersedes the earlier raven
failure reported during arrival tuning. The log is preserved in ignored
`results/cursor-2d-turning-2026-10-12/qt-suite.log`.

Restart the rebuilt resident for native visual review. Automated/offscreen
checks do not establish native visual feel or battery impact.
