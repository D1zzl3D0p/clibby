# Character clipping and gait corrections

Feature work is paused for the user's character correctness feedback. The user
reported cropped Mallow/Bramble textures, broken-looking Mallow knees, short
robotic steps and a torso that lagged behind the hips.

## Changes

The resident's native window mask used the authored interaction polygon as a
visual cutout. It now includes the posed opaque artwork and a narrow rounding
fringe. Alpha scanline geometry is cached at package load, and native masks
update only when changed. The authored interaction region remains stable for
gestures and placement. Existing canvas margins contain the sampled artwork;
neither canvas size nor the host's anchor mapping needed expansion.

Qt's public native mask couples visible shape and OS input shape. Visible fringe
outside the authored grab region may intercept clicks, although it cannot start
pet gestures. Native click-through and appearance need desktop verification.

Mallow's editable artwork now uses shallow forward knees on both legs instead
of mirrored outboard knees. Knee solve direction follows facing, changing branch
at the edge-on turn midpoint while contacts remain fixed. Bramble gains the same
visual facing branch and genuine grounded locomotion instead of its legacy sine
walk. Its origin now matches its authored contact floor at y=162. Unequal limb
lengths are supported with validation against the inner folded reach radius.

Torso lean responds to navigation intent immediately and includes sustained
velocity balance; the delayed acceleration response is smaller. Head response
counterbalances it. A small support-transfer bob, 40ms double-support period and
farther landing forecast produce fuller steps. Mallow speed increases 38→50
logical units/s, Bramble's grounded speed is 70. Reach constraints still limit
actual speed. Quiet settling and the existing scheduling policy remain intact.

The 3D werewolf uses speed70, acceleration240, longer stride and swing durations;
torso/head/arm response converts world velocity to local facing coordinates so
reversed walking retains the correct lean. Prior arm clearance and lifted yaw
replants remain in place. This correction does not redesign wave/hold gestures.

## Verification and review

The mask regression reproduced the crop before correction. Pixel containment
covers both production characters across seven actions, twelve phases, drag
responses and grounded gait, including an uncropped padded raster check.
Motion regressions cover immediate/cruise torso balance in both directions,
stride distance, direction-aware knees, independent unequal link lengths and
world contacts, plus reversed local 3D response. Editable assets regenerate
deterministically.

Fresh Release build succeeded. Qt checks: 18 passed, 3 skipped (two local-socket
checks blocked by the execution environment and one native GPU check). Python:
50 passed. These establish software behavior, not native visual acceptance or
battery performance. The user previously verified Open source and toast
acknowledgment; that observation remains recorded separately.

Qt software captures of 35 forward/reversal phases for each 2D character are in
`results/gait-correction-after/`, with `walking-review.gif` for visual review.
The captures were inspected for complete artwork, knee bend and torso balance.
Native appearance, especially the revised feel of the gait, still needs the
user's judgment. Quit and relaunch the rebuilt resident/preview to load changes.
