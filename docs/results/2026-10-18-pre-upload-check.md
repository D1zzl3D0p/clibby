# Regression revalidation and pre-upload check — 2026-10-18

## Final fresh-build verification: edge failures resolved

The corrected source passes a complete Release/app build from the new
`build/github-edge-fix-2026-10-18/` directory. Combined Qt: **88 passed, five
environment skips, zero failures** (93 total), including `resident_spatial`,
`resident_quadruped3d`, all six original motion regressions and app relocation.
Python: **109 passed**. Runtime, assets and tests stayed unchanged throughout
the final build and Qt suite; only the handoff and delivery documentation were
updated concurrently.

The [movement/cursor record](2026-10-18-rightward-cursor.md) explains the bounded
hind-leading fallback, separate host/body origin, and physical-pointer gesture
latch. Automatic signatures now defer during cursor attention, preventing the
25-second idle deadline from taking over the head. The extended cursor test
covers that deadline, both gaze axes, stable body heading through vertical
movement, and a deliberate opposite crossing. Its center assertion preserves
the established held-gaze deadzone rather than requiring startup-neutral pose.
Manual edge roundtrips pass at 50%, 100% and 200% sizes. No boundary, support,
posture or reach admission check was waived.

Raw final logs: ignored `results/edge-turn-fix-2026-10-18/final-build-and-tests.log`
and `python-tests.log`. Earlier diagnostic/interrupted logs in that directory
remain historical. The fresh app is
`build/github-edge-fix-2026-10-18/Clibby.app`. These software checks do not replace
native UX, energy, signing or notarization acceptance. Upload hygiene passes:
no likely credential matches or oversized blobs; local outputs remain ignored.
Repository-wide source/brand licensing remains unselected. No commit or upload.

## Prior check after cursor work was reported finished

Rebuilt the current checkout with `tools/build_qt.py` in
`build/github-final-check/`. All 1,000 upload-candidate files stayed unchanged
during validation. Compilation, macOS deployment and app relocation pass.
Python passes **109 tests**. The complete Qt suite has **86 passes, five
environment skips and two failures** out of 93; both failures reproduce in a
focused rerun:

- `resident_spatial`: “Biscuit cursor attention can turn back right”.
- `resident_quadruped3d`: “Biscuit cannot move right after reaching the left
  desktop edge”.

The current source therefore still fails both cursor-driven and manual
rightward movement after a left-edge arrival. These results supersede the
earlier passing snapshot below. No production fixes were made during this check.
Logs: ignored `results/pre-upload-final-2026-10-18/build-and-tests.log` and
`rerun-failed.log`; the focused test diagnostics remain in the build's
`Testing/Temporary/LastTest.log`.

All 321 relative Markdown links resolve; whitespace checks pass. Publication
audit found no likely credentials in current files, 1,277 reachable history
blobs or 392 archived files. Content totals 21.28 MB, with a largest file of
1.07 MB; generated outputs are ignored. Repository-wide source/brand licensing
remains unselected. No commit, remote configuration or upload was performed.

## Earlier revalidation

The six failures from [the previous checkpoint](2026-10-17-pre-upload-check.md)
are resolved in the current motion corrections. A rebuilt combined suite passed
**88 tests with five environment skips** (93 total), including all six:

- `resident_raven3d`
- `character3d_motion`
- `locomotion_polish`
- `character3d_quadruped`
- `marmalade3d_joint_binding`
- `character3d_joint_binding`

The later foot-led turn and recovery changes were already present when this
pass started. Review confirmed that the tests retain contact, posture, reach,
placement and arm-clearance requirements. Two obsolete fixed-pace expectations
were updated by concurrent anatomy-pace work while packaging ran; their rebuilt
tests validate the [approved anatomy-derived pace](2026-10-18-anatomy-pace.md),
including scaled anatomy, multiplier ratios and planted contacts.

## Remaining upload gate

A separate new cursor regression arrived in `qt/resident/spatial_tests.cpp`
during this pass. After rebuilding that test, a focused `resident_spatial` run
fails at **“Biscuit cursor attention can turn back right”**, following manual
travel to the left desktop edge. Thus the earlier 88-pass suite does not certify
the latest test snapshot. The separate cursor investigation is active under
ignored `results/rightward-cursor-2026-10-18/`; this pass did not overwrite its
work. Rerun the complete suite once that correction is finished.

## Other verification

- Python: **109 passed**, repeated after the anatomy-pace test updates.
- Release compilation, macOS Qt runtime deployment and app relocation pass.
- Relative Markdown links and whitespace checks pass.
- Publication audit: 998 candidate files at the audit snapshot, approximately
  21.27 MB; largest file 1.07 MB. No likely credential-pattern matches in current
  files, 1,282 reachable history blobs or 392 archive members. No oversized blobs
  requiring LFS were found. Pattern scans do not prove all possible secrets absent.
- Generated outputs are ignored. All 38 character packages include licenses;
  repository-wide source/brand licensing remains unselected and disclosed.

Raw logs are in ignored `results/pre-upload-2026-10-18/`: `qt-suite-final.log`,
`python-tests-final.log`, `spatial-final.log` and `build-and-ctest.log`.
The initial build log includes the superseded fixed-pace assertion failures.
No commits, remote configuration or upload were performed.
