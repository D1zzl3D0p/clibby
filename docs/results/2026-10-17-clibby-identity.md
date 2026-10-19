# Clibby identity and application rename

The user approved the crescent C with its large open head cutout, paired ears
and matching eyes. That vector geometry is now the production mark. The product
name is **Clibby**, the executable is `clibby`, the optional CMake switch is
`CLIBBY_WITH_3D`, and built spatial resources live in `clibby-3d`.

## Applied identity

- Ink, lilac and cream artwork; SVG masters, pixel-size PNG exports, a native
  monochrome menu template, macOS `.icns`, and geometry/colour tokens.
- Shared scoped light/dark Widgets presentation for the Toolbox, notification
  cards, productivity panel and character preview. The 3D preview's utility
  controls follow the system palette with lilac selection and ink selected text.
- System typography, readable status colours, rounded controls and consistent
  spacing. The pet art, motion and scheduler remain independent of the UI theme.
- Current app/window names, command instructions, tooling messages, package
  metadata, authoring attribution prose and active API documentation use Clibby.

The [identity guide](../brand.md) documents assets, usage and regeneration.
`tools/build_brand_assets.py` regenerates the source SVGs before exporting the
runtime files, preventing artwork/token drift. Every mark rendition shares one
outline and two eye ellipses.

## Application and compatibility

The local macOS build produces `build/qt/Clibby.app` alongside the terminal
executable `build/qt/clibby`. Its icon, name and bundle identity are configured;
character and 3D resources synchronize on every app build, including builds
that do not relink the executable. The bundle still requires the local Qt SDK;
signing, deployment and distribution packaging are separate work.

The resident and `petctl` share default state discovery. New installations use
Clibby app data; when the new directory is absent, an existing legacy directory
is reused in place, preserving preferences, inbox storage, instance locking and
IPC ownership. If both directories exist, the Clibby directory takes priority.
Explicit `--state-dir` remains authoritative. Hook sequence counters preserve
their existing legacy-first rule and lock files. The `bp-` socket prefix and
wire protocol are unchanged.

The real checkout path has not moved. Historical research, plans, raw evidence
and existing copyright holder notices retain their original names.

## Verification

- Release build succeeds, including the macOS bundle.
- Full Python suite: 93 passed.
- Full Qt suite: 67 passed, four platform-dependent skips, no failures.
- Identity checks cover new/legacy state selection, restored application
  identity, embedded icon resources, native template flag and transparent tile
  corners.
- The macOS bundle's icon/name/executable and all 312 copied character/resource
  files were checked against the build outputs.
- Bundle process checks: three passed, two skipped where Unix socket binding is
  denied by the execution environment.
- Light/dark panel captures and notification/Toolbox captures are preserved in
  ignored `results/clibby-brand-2026-10-17/` directories. Toolbox regressions cover
  first-paint overflow, 420/440/640-pixel widths and nested dark-mode cards.
- Header marks render from larger source images with the display pixel ratio;
  the productivity panel checks also pass at 2× scale with Retina proof captures.

These are build, headless rendering and software checks. Native menu appearance,
interactive bundle launch, Spaces transitions and battery impact require
desktop acceptance; this change makes no new parity or energy claim.
