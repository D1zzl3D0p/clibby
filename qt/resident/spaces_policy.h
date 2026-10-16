#pragma once
class QWindow;
namespace pets {
// Apply to an existing native window on the GUI thread. Other platforms and
// offscreen tests are no-ops; this never creates windows or moves the pet.
void joinAllSpaces(QWindow *window);
// Permit transparent padding outside a screen. The host bounds the actual
// pet geometry; Cocoa's automatic whole-window constraint would undo that.
void allowTransparentWindowPlacement(QWindow *window);
}
