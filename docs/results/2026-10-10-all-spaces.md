# Resident pet across macOS Spaces

The user requested that the pet stay visible while switching desktops/Spaces on
its current physical monitor. The resident host now applies a small Cocoa window
policy to its existing native window and the attached above-head status card.

`NSWindowCollectionBehaviorCanJoinAllSpaces` keeps the window in all desktop
Spaces. The policy removes `MoveToActiveSpace`, enables `FullScreenAuxiliary`,
and on macOS 13+ uses `CanJoinAllApplications` with conflicting roles removed.
These request eligibility alongside fullscreen applications and Stage Manager;
actual fullscreen/Stage Manager visibility remains a native acceptance check.
Unrelated collection behavior bits are preserved. Reference:
[Apple's collection behavior API](https://developer.apple.com/documentation/appkit/nswindow/collectionbehavior-swift.struct).

The host applies the policy after Show and on native surface creation; the
attached card applies it on Show. The helper checks for an existing Cocoa native
handle before converting it to an NSView. Other platforms and offscreen tests
are no-ops. This adds no timers, polling, new pet windows or position writes.
The summoned toolbox remains an ordinary utility window.

## Verification

- Both host variants and the status card failed the native all-Spaces assertion
  before implementation, then passed on Cocoa after implementation.
- Native checks cover initial show, hide/show, host native surface recreation,
  exclusive collection behavior flags, physical monitor/window identity, and
  unchanged native geometry when reapplying the policy.
- Fresh Release build succeeded. Full CTest: **22 passed, 4 skipped**, zero
  failures. Skips: socket transport, socket CLI, native Spaces (the standard
  suite runs it offscreen), and native GPU capture.
- Direct Cocoa Spaces executable: **3 checks passed**. Full Python: **50 passed**.
- Independent scoped code review found no must-fix issues.

The direct native command from the repository root is:

```sh
rtk proxy build/qt/resident_spaces_tests /Users/dizzler/Projects/Repos/better-pets/shared
```

The test does not switch the user's desktops or fullscreen applications. Restart
the rebuilt resident, then check normal Space switching, fullscreen applications,
Stage Manager if used, and a second monitor. Confirm that explicit Hide still
hides the pet and its card. These visual acceptance checks remain open.

Investigation also reproduced a preexisting Cocoa Hide/Show coordinate issue:
both fixture and modular hosts report a 20-pixel upward shift after restoration.
An isolated no-op policy executable reproduced the identical shift. This change
does not fix that placement issue; tests separate policy geometry preservation
from the existing restoration behavior. New runtime placement code was not added.

Concurrent resident 3D work changed during this run. An intermediate full-suite
failure in `resident_spatial` was gone in the final fresh build; no 3D files were
modified for the Spaces feature. No battery claims are made.
