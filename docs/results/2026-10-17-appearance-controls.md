# Companion appearance controls — 2026-10-17

The menu-bar and companion context menus now contain **Appearance**:

- **Companion size:** 50%, 75%, 100%, 125%, 150%, 200%.
- **Pixelation:** Fine, Light, Classic, Chunky, Very chunky.

Both choices apply immediately, persist in version-1 preferences and survive
companion changes. Defaults preserve the original 100%/Classic appearance.
Pixelation changes rendering resolution independently of companion size. Existing
2D artwork cannot gain source detail; the archived fixture disables pixelation.

The host retains canonical geometry and scales art, native masks, support bounds
and anchors together. Planar and spatial controllers convert world coordinates
at their host boundary. Resizing rebases motion at the current world root/floor;
it does not promise to preserve an in-progress signature or navigation target.
Coarse 3D output expands the visual mask to include sampled pixel blocks.

## Verification

- Full combined Qt run: 87 passed, five environment skips, zero failures.
- Python unittest discovery: 109 passed.
- Native macOS spatial regression passed, including 50% and 200% sizes, fine and
  chunky rendering, walking, landing, suppression and companion switching.
- Menu tests exercise live window size, independent pixelation selection,
  persistence and paused timer suppression. Host/surface tests cover masks,
  canonical drag displacement and scaled narrow-window support validation.
- The rebuilt `build/qt-humanoids/Clibby.app` passes relocation verification.
  Qt's bundler reports missing optional SQL-driver dependencies (ODBC, PostgreSQL,
  Mimer); the application does not use these drivers.

Native captures are in ignored `results/appearance-controls-2026-10-17/`.
A bundle check run while packaging was still modifying the executable failed;
after packaging completed, the full suite and relocation check passed.
Native desktop behavior on other platforms and battery impact remain unverified.
