# Nearby cursor attention

The user selected the mouse cursor as the first nearby interest target. The
resident now observes its global logical pixel position, measures displacement
from the posed head, and supplies it to the existing Qt-free activity chooser.
Cursor interest tracks the head and can turn the 3D body in place;
cursor pursuit by walking is not enabled.

Attention begins within 2.5 pet widths and releases beyond 3 widths, avoiding
boundary flicker. Planar width uses the character canvas; spatial width uses
the projected grab region rather than its large transparent window. A small
deadzone prevents unstable aim when the cursor crosses the head. The chooser
submits a gaze direction, slight interested head tilt and supported ear goals.
Only meaningful direction changes emit new look commands. All realized movement
uses the existing bounded procedural followers; leaving clears smoothly.

The user subsequently requested body facing when a cursor is behind the 3D pet.
`Motion::face(worldX)` now selects a heading without altering navigation or root
position, using the existing yaw follower and alternating foot replants. A cursor
more than .35 pet widths to one side requests that facing while attention owns
control. The center zone retains heading; changed turns have an .8-second cooldown.
Repeated same-heading requests add no impulses or animation wakeups. The head
continues to refresh its local target through the turn. This chooses between the
resident's existing left/right presentation headings; a screen cursor has no
independent world-depth position. Grounded feet retain strict support rules. Interrupted pickup/drop turns also
resume when the same cursor regains attention. Cursor-facing regression evidence
is in `results/cursor-facing-2026-10-10/`. Its complete build and 26-check Qt run
passed before concurrent raven/window-surface API edits began; the final core
interruption regression passes, while rebuilding the latest combined tree waits
for those unrelated APIs. Do not infer native turn appearance from these checks.

Attention preempts autonomous activity and stops exploration once. Manual
navigation, dragging, pause, hidden/suspended states and notifications retain
priority. Leaving starts a fresh quiet interval. Cursor observation joins the
existing single timer: 500 ms while distant, 100 ms while interested, with no
cursor polling under suppression or higher-priority control. Settled held gaze
does not request animation frames. This introduces sparse cursor wakeups while
visible; actual battery cost has not been measured.

Planar lateral direction accounts for mirrored head ancestry. Spatial aim maps
relative screen displacement through the camera and current model orientation,
then adds model-neutral forward. This preserves useful head range instead of
clamping an absolute camera-facing gaze. The resident refreshes the held local
goal while a body turn settles, without waking already settled motion. This is
a bounded expressive approximation, not eye-ray convergence or scene recognition.

Verification covers proximity hysteresis, direction changes/jitter, stationary
settling, smooth release, missing anatomy, reaction/suppression ownership, clock
changes, exploration interruption, resident polling/pose continuity, mirrored
2D direction and 3D orientation conversion. See ignored evidence under
`results/nearby-cursor-2026-10-10/` for final build/test results.

Fresh complete build succeeded. Final integrated CTest: 26 passed, four
environment skips, zero failures. Python: 50 passed. Cursor core, resident
character, resident application and spatial checks pass. Earlier concurrent
pickup/drop failures were resolved by that task; their intermediate outputs
remain preserved alongside the final passing ctest-latest.log.

Stationary 2D software captures for Mallow and Bramble show left/right/upward
tracking, settled hold and exit release. Held raw frames are identical and the
chooser emits no repeated commands; release returns head expression to zero.
Captures use the actual chooser and procedural followers, with a separate stage
cursor marker; they establish no OS cursor or native GPU measurements.

Native gaze
appearance and energy acceptance remain open after restarting the rebuilt pet.
