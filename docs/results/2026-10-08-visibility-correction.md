# Resident focus visibility correction

The user reported that the pet disappeared when another application received focus.
The resident requested `Qt::Tool`, which on macOS maps to a tool panel that hides
when the application is inactive. The host event filter only canceled dragging on
deactivation; it did not intentionally request Hide. Qt documents this behavior in
[its window/widget attributes](https://doc.qt.io/qt-6.8/qt.html).

The resident now requests an independent `Qt::Window` with the existing frameless,
stays-on-top and does-not-accept-focus flags. Explicit Show/Hide and shutdown still
control visibility. Notification and toolbox widgets retain their tool-window
behavior with `WA_MacAlwaysShowToolWindow` enabled. This avoids focus-driven hiding
without a focus listener, re-show loop or another timer. The frozen benchmark is
unchanged.

Regression checks failed before the change and pass after it: the host must not
request an auto-hiding tool panel, a deactivation event does not hide the pet,
and notification widgets retain the macOS always-show attribute. These are
software checks; actual background stacking/focus/Spaces behavior still requires a
native trial of the rebuilt resident. Restart the resident through Quit, then run
`rtk proxy build/qt/better_pets` from this project directory.

The user clarified that the notification native trial worked: **Open source**
opened its source and **Acknowledge** dismissed the toast. This is user-observed
functional evidence, not automated socket-test execution or battery evidence.

Final verification: Release rebuild passed; full Qt suite **18 passed, 3 skipped**
(two socket checks and native GPU capture). Full Python suite **48 passed**.
No extra animation timer or focus polling was added.
