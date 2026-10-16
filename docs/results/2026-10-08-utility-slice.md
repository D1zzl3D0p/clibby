# Actionable notification and body-fixture slice

Implemented on 2026-10-08. This record supersedes the utility handoff's initial
proposal-only status; the research report remains the original recommendation.

## User trial update

The user later verified Open source and Acknowledge on the native toast. They also
reported focus-driven hiding and werewolf clipping; see the
[visibility correction](2026-10-08-visibility-correction.md). Those reports do not
close GPU, resources or battery checks, and previous contact tests did not prove
visually correct 3D turns.

## Useful flow

`tools/pet_run.py` runs a real command with inherited input/output, publishes
`working`, and publishes `completed` or `failed` from its actual exit status.
It preserves that exit status. Every default run has a unique session identity;
sequence 1 is working and sequence 2 is the observed outcome. It uses the existing
session API through `petctl`. Notification failures are visible on stderr and do
not prevent the work itself. Sends are bounded; there is no polling or heartbeat.
The default source is the current project directory as a local file URL.
See [command producer usage](../api/command-producer.md).

The resident now presents an ordinary keyboard-accessible card above the head,
with separate **Open source** and **Acknowledge** buttons. Opening preserves unread
state. Acknowledging completion, failure or input request removes it from foreground
while retaining the source's reported status until expiry. The brief gesture ends;
the unread card stays static, with no pulsing or continuous gesture. Working status
can be opened but cannot be acknowledged. Labels remain literal plain text.

The **Toolbox…** menu command summons a scrolling inbox using the same card class,
records and registered dispatcher. Existing Pause/Resume, Walk left/right and Stop
controls use the same resident actions. Unavailable locomotion is disabled. This is
a small initial toolbox, not a general utility suite or third-party launcher catalog.
Source grouping is lexical by source/session. Acknowledged records remain available
here, including their source links. No timer/reminder provider or pinned app catalog
is implemented in this slice.

Optional `source_url` is additive in API major 1 and `petctl --source-url`.
Supported launch targets are absolute HTTP/HTTPS URLs without credentials and local
file URLs without an authority, at most 2048 UTF-8 bytes. Relative URLs, unknown
schemes, remote file authorities and literal/decoded controls are rejected. External
input cannot request a shell command. The user must click to launch a target.
Every card action revalidates its displayed sequence against current source state;
expired or changed cards cannot act on an unseen event. Launch failures appear on
the card that initiated them. The schema and [API contract](../api/session-v1.md)
include the metadata.

Hide/suspend dismiss the toolbox and notification. Pause preserves attached text,
while the toolbox continues to show current provider state. Expired foreground
cards disappear even while paused. Sleep/wake retain the existing explicit reset
policy. TTL and silence never invent success, a refresh request or a chat outcome.
The resident still owns one next-deadline timer; cards introduce none. Unchanged
menus/card contents are retained rather than rebuilt on every animation tick.

## Reusable body fixtures

[Body fixture API](../utility-body-fixtures.md) defines head/card/effect anchors,
optional grip bindings and semantic gestures. The resident transforms its posed
2D card anchor into window coordinates including the render margin. Its session
reactions use semantic nod/look/success/failure fallbacks on armless Mallow.
Nearest rendering and planted leg contacts remain authoritative.

The optional werewolf preview adds target-driven unequal-link arm IK, measured grip
sockets and one-second semantic gestures. W waves; H reaches; R releases; D shows
contact/grip dots. Body bob translates the complete arm fixture so measured lengths
remain fixed. These are model-relative goals with a fixed wrist orientation and
reach limits. Anatomical angle limits, desktop-space reaching and resident 3D
integration remain deferred.

## Verification and limits

- Release build passed; full CTest: **18 passed, 3 skipped** out of 21 targets.
- Full Python suite: **48 passed**, including 11 command-producer executable tests.
- Resident tests cover source validation/serialization, Open source without
  acknowledgment, changed/expired actions, paused expiry, completion acknowledgment,
  shared toolbox cards, Pause/Resume and launch-error feedback.
- Body tests cover transformed anchors, gesture endpoints, unequal reach, grip
  alignment, fixed arm lengths under bob and leg-contact preservation.
- Offscreen widget captures were inspected at
  `results/utility-validation/notification.png` and `toolbox.png`. They prove layout
  rendering only, not native focus, clicking or desktop placement.
- `resident_ipc` and `resident_ipc_cli` remain skipped because Unix socket binding
  is denied in this sandbox. The latter now contains a real child-command → wrapper
  → petctl → resident success/failure round trip for a normal Terminal. It did not
  execute here. Producer and shell are separately exercised; no live end-to-end
  delivery is claimed.
- `character3d_gpu_render` remains skipped. Native GPU pixels, monitor snapping,
  click/focus behavior, quiet resource cost and battery impact are still open.

## Native trial

From the project directory in a normal Terminal:

```sh
rtk proxy build/qt/better_pets
```

In another Terminal:

```sh
rtk proxy .venv/bin/python tools/pet_run.py --label "Project tests" -- .venv/bin/python -m unittest discover -s tests
```

Check that completion is accurate, Open source opens this project directory without
acknowledging, Acknowledge clears the attached card, and Toolbox… retains the record.
Also run the live socket checks:

```sh
rtk proxy ctest --test-dir build/qt -R '^(resident_ipc|resident_ipc_cli)$' --output-on-failure
```

There is still no coding/chat adapter. The exact app name/path for “Vorsaint” is
unresolved; no app features or launcher action were invented. This directory has
no Git repository, and no commits are claimed.
