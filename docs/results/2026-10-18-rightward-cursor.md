# Biscuit rightward movement and cursor attention

Independent final validation also passes a fresh app build, 88 Qt checks with
five environment skips, and 109 Python checks. See the
[pre-upload checkpoint](2026-10-18-pre-upload-check.md) for final logs and scope.

The reported regression affects both the context-menu Walk right action and
cursor attention after Biscuit reaches the left desktop edge. The accepted
leading-plant turn and current anatomy-derived walking pace remain in place.

## Movement correction

The normal fore/hind/fore turn can require an outward support transfer at the
screen edge, so full-pose admission correctly rejects it. A final bounded
hind/fore/hind alternative provides an inward leading plant when the original
families fail. It retains six placements and .84-second duration. Loaded-paw
reach constraints and the host interval constrain its root path; existing
height, reach, sole and recovery-home admission checks still apply.

A separate frame error could cancel even an admitted turn. A small persistent
body offset from integer host realization was being treated as a request to
recenter the window outward. Directional host movement now uses its saved host
origin plus the change in world body position. Copying and support translation
carry that origin with the plan. Body-world and host coordinates are also kept
consistent in admission and the bounded edge projection.

## Cursor correction

The actual cursor trace first completed the requested right turn, then turned
back left: the support arc moved Biscuit past the stationary pointer, and the
attention loop interpreted that movement as a new cursor instruction. Body
heading now consumes a pointer position and requires at least four pixels of
new horizontal pointer movement before another heading decision. Manual
suppression, missing cursor and curiosity reset clear that intent. Vertical
pointer movement and continued head tracking remain independent.

A further native/offscreen difference exposed an automatic signature starting
at 25 seconds during active cursor attention. It suppressed the attention
behavior and cleared the head goal. Sparse automatic signatures now defer
while attention owns the interaction; explicitly requested signatures retain
their existing behavior. The gaze-coordinate mapping itself is unchanged.

## Evidence and limits

Raw evidence is retained in ignored `results/rightward-cursor-2026-10-18/`.
The independent native-host matrix covers Biscuit at speeds .5/2/3 and sizes
.5/1/2 over left/right/left/right gestures: all 36 complete the intended heading
and travel direction. Each turn uses six placements, at least two supports,
hip height at least .905869 of rest, and zero planted translation/yaw drift.
First plant advances approximately .24 measured reach with at most .0378 degrees
of torso yaw. This is kinematic evidence, not a force-balance measurement.

The matrix reports zero rightward screen overshoot. Existing leftward changing-
mask rounding can overshoot by up to two pixels, and cached bounds can leave
travel short of the newly projected exact screen edge. These are not claimed
fixed. The matrix snapshot precedes one documented midpoint-coordinate
correction whose side choice is identical in those edge cases. Application
menu and cursor integration are checked separately from the matrix.

Final verification on the product source:

- Full app build and separate bundle relocation pass.
- Combined Qt suite: 88 passed, five environment skips, zero failures (93 total).
- Final clean `resident_spatial` test passes offscreen and in a native window.
  It covers left-edge arrival, held right-cursor turn, head pitch and both yaw
  directions, vertical-only pointer heading retention, center deadzone stability,
  deliberate left crossing, attention held through 32 seconds without an
  automatic signature, and automatic-signature resumption after the pointer leaves.
- Actual context-menu actions pass two full round trips. Direct host movement
  additionally covers half, normal and double size.
- `git diff --check` passes. Python/2D-only checks were not rerun for this fix.

Build logs retain optional missing SQL-driver dependency diagnostics; the build
exits successfully and relocation passes. Native spatial logs retain the existing
teardown binding message and intentionally injected renderer-failure diagnostic;
the test exits successfully. These messages are not hidden as new regressions.
No commit, push, upload, battery measurement or new subjective visual approval.
