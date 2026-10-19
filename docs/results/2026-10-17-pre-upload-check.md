# Pre-upload check — 2026-10-17

> Historical checkpoint: all six failures below subsequently pass. See the
> [2026-10-18 revalidation](2026-10-18-pre-upload-check.md) for current checks and
> the separate cursor regression introduced afterward.

The final check ran after concurrent work finished. Hash comparison confirmed
that all 997 existing upload-candidate files remained unchanged during validation.
No runtime fixes, staging, commits or upload were performed.

## Verification

- Python: **109 passed** using `.venv/bin/python -m unittest discover -s tests`.
- Release build and macOS runtime deployment completed in ignored
  `build/github-final-check/`.
- Qt: **82 passed, five environment skips, six failures** out of 93 tests.
  All six failures reproduced with `ctest --test-dir build/github-final-check
  --rerun-failed`. The bundle relocation check passed.
- All **314 relative Markdown links** present before this report resolved.
- `git diff --check` passed. Credential/editor/build/backup ignore probes passed;
  no ignored files were tracked.

These fresh results supersede earlier passing counts for the current checkout.
The failures are not explained away as environment skips; their causes have not
been diagnosed or fixed by this check.

| Failing test | Reported assertion |
| --- | --- |
| `resident_raven3d` | Moving window during recovery retains relative walking destination |
| `character3d_motion` | Stationary turnaround exceeds its planned placement count |
| `locomotion_polish` | Bounded pivot collapses upright posture |
| `character3d_quadruped` | Quadruped loses its declared support contacts |
| `marmalade3d_joint_binding` | Walking lost required foot support |
| `character3d_joint_binding` | Elbow folds into torso clearance |

Skipped tests: resident IPC transport, native menu-bar event test, native Spaces,
IPC CLI integration and GPU render. Test logs remain under
`build/github-final-check/Testing/Temporary/`; the focused rerun replaces
`LastTest.log` with the six reproduced failures.

## Publication hygiene

The read-only audit inspected 997 candidate files (21.21 MB total; largest file
1.07 MB), 1,328 reachable Git-history blobs and 392 archived files. No likely
credential-pattern matches or history blobs over 50 MB were found. This is a
pattern scan, not proof that every possible secret is absent. Generated outputs
remain ignored. All 38 character packages include licenses; repository-wide
source/brand licensing remains unselected and disclosed in the README.

No Git remote is configured. The working tree contains uncommitted work and new
files, including the research moves. A clean validation gate still requires
addressing the six reproduced regressions and rerunning the full suite.
