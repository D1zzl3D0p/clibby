# macOS menu-bar crash correction

The supplied macOS 27.2 crash report ends in `NSEvent.clickCount`, called by
Qt 6.10.3's Cocoa system-tray activation observer when menu tracking begins.
macOS 27 can deliver this interaction through a gesture with a non-mouse
current event. Qt reads a mouse-only property and AppKit raises an exception.
This matches [QTBUG-147449](https://qt-project.atlassian.net/browse/QTBUG-147449)
and the event guard in Qt's development branch.

Clibby now owns a native `NSStatusItem` on macOS, with the existing
`QMenu::toNSMenu()` menu and Qt delegate. This bypasses the broken activation
observer while preserving the original actions, submenus and dynamic state.
The status item is removed before its menu is destroyed. Other platforms keep
the existing Qt system tray. No global event overrides or Qt binary patches
are installed.

The native regression injects an application-defined current event and posts
the same menu-tracking notification. The original Qt tray implementation failed
with `Invalid message sent to event ... AppDefined`; the native implementation
is checked with repeated creation/destruction and menu label synchronization.
Run `QT_QPA_PLATFORM=cocoa build/qt-humanoids/menu_bar_icon_tests` with desktop
access; the normal offscreen suite skips this native check.

The rebuilt `build/qt-humanoids/Clibby.app` launched, and the user confirmed
the toolbar interaction works. Raw report and verification output are in ignored
`results/menu-bar-crash-2026-10-17/`. Python verification: 94 passed. Bundle
relocation passed when rerun after app packaging completed; the earlier check
overlapped packaging and observed the intermediate unbundled executable.

Final native regression passes. The final Qt run has 74 passes, five environment
skips and six failures: `catalogue3d_iona`, `catalogue3d_milo`,
`catalogue3d_orris`, `catalogue3d_fen`, `catalogue3d_oona` and
`catalogue3d_quillan` each report `unknown or unsupported catalogue asset`.
Those six companion tests were added by concurrent catalogue work during this
task (the earlier run contained 79 tests; the final run contains 85). Their
implementation is outside this crash fix. The bundle relocation check passes
in the final run. The native menu regression is run separately because it
requires desktop access.
