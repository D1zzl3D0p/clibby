# Pinned floor contacts

The user made the priorities explicit: a landed sole cannot translate until it
lifts, only one foot may be airborne, and stretching or changing knee bend is
preferable to violating either rule. The 3D ankle may rotate around its contact.

## Implementation

- Shared gait no longer moves or lifts stationary contacts to restore reach.
  It alternates complete swings with the existing short floor transfer. A swing
  lands at its planned world target even when that exceeds anatomical reach.
- Shared spatial IK adds an opt-in contact-preserving mode. It keeps targets
  exact and extends links outside outer reach; inside an unequal-link fold it
  extends the shorter link. Reachable links retain their authored lengths. Arms
  continue to use the rigid default.
- Both renderers extend actual joint translations to match the solution. The
  2D artwork stretches along each segment without changing paw size or contact
  geometry; retained drawing, raster composition and native alpha masks agree.
- The 3D pose removes contact reach/side projections. Ankle yaw/pitch is resolved
  around the fixed sole, and manual turns block admission of a new ordinary
  swing until their foot lands. Body navigation continues during that wait.

Horizontal navigation, normal swing duration and the shared support force
equation are unchanged. At high travel speeds the visible legs can extend more
than before, as explicitly requested. This supersedes overlap/recovery policies
in previous motion records; historical raw evidence remains unchanged.

## Verification

The new shared solver regression first failed on a projected unreachable target.
The new 2D artwork regression first failed on a gap left by extended joint
translations. They now pass. Grounded regressions check rendered world soles
during large realized displacement, reversal, quantized host movement and
0.5–3× travel, while checking strict single-foot flight and observed extension.

Fresh full build and verification: 20 Qt checks passed, 3 environment skips
(two native socket checks and native GPU framebuffer), and 50 Python checks
passed. The live 20-joint scene verification passes at speed 3, including actual
sole positions, temporal pins on consecutive grounded frames, single-foot
flight, observed rendered extension, ankle articulation, arm clearance and
arrival/reversal settling.
The 3D math regression covers actual sole reconstruction and temporal pins at
0.5–3× speed and 30/60/240 Hz, plus repeated body impulses beyond normal reach.

Review captures and raw logs are in `results/pinned-floor-contacts/`.
The 2D walking contact sheet was visually inspected; the GIF shows the same
captured walk. Offscreen captures do not establish native GPU appearance or
desktop parity. Restart the rebuilt resident and preview binaries to review.
