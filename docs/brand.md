# Clibby visual identity

Clibby is the product name; `clibby` is the executable and project slug. The
approved identity is the C-shaped mark with its open head cutout and two eyes,
from [vector refinement 03](../art-studies/clibby/README.md). It identifies the
application without adding a mascot or changing the existing creature artwork.

## Mark and colour

Use the production files in [assets/brand](../assets/brand), rather than copying
the original proof image. Preserve the mark's proportions, open cutout, eye
positions and rounded terminals. Scale the supplied geometry uniformly; do not
redraw it at small sizes or fill the cutout with a separate face colour.

| Token | Value | Use |
| --- | --- | --- |
| Ink | `#292638` | Primary mark and light-appearance text |
| Lilac | `#B9ADEB` | App tile, selection and primary accents |
| Cream | `#F7F4EE` | Light backgrounds and reversed mark |

Use the ink mark on a light surface and the cream reversed mark on a dark
surface. The app tile uses ink on lilac. The menu-bar template is monochrome on
transparency so the system can supply the appropriate appearance. Keep the
identity flat; gradients, decorative motion and shadows are unnecessary.

| Asset | Purpose |
| --- | --- |
| `mark.svg`, `mark-*.png` | Transparent primary mark |
| `mark-reversed.svg`, `mark-reversed*.png` | Cream mark for dark surfaces |
| `menu-template.svg`, `menu-template-*.png` | Native menu-bar template |
| `app-icon.svg`, `app-icon*.png` | Lilac app tile; PNG exports from 16 to 1024 pixels |
| `clibby.icns` | macOS application bundle icon |
| `tokens.json` | Colour, mark geometry, spacing and radius values |

## Type and layout

Use system fonts and the platform's normal body size, approximately 13 pt on
macOS. Use semibold headings and a modest size increase for the product title.
When a wordmark label is needed, use lowercase **clibby** in the system font;
this is a descriptive typeset label, not a custom font or outlined wordmark.
Ordinary app titles and prose use **Clibby**.

Use the spacing scale **4, 8, 12, 16, 24, 32** logical pixels. Control corners
use an 8-pixel radius; cards and panels use 12 pixels. The app tile's authored
radius is part of the icon geometry, not a general UI radius.

The shared [Widgets theme](../qt/resident/brand_theme.h) is scoped to the
resident's utility panels and status cards. It follows the system's light/dark
appearance through palette-change events. Preserve normal widget behaviour,
keyboard navigation, shortcuts and visible focus. Keep errors, success and
warnings semantically distinct, with readable text explaining the state; lilac
is the brand accent rather than a substitute for every status colour. Pet
rendering and character palettes remain independent of the utility UI theme.

## Reproduce and use

The editable geometry lives in [art-studies/clibby/build.py](../art-studies/clibby/build.py).
The production exporter uses that shape and writes the SVG, PNG, macOS icon and
token files into `assets/brand/`. From the repository root:

```sh
rtk proxy .venv/bin/python tools/build_brand_assets.py
```

This uses the project's Pillow installation and `rsvg-convert`. The study
builder also generates its proof sheet; macOS Arial is used for proof labels
only. The mark's SVG geometry has no font dependency. Edit the source geometry
and regenerate the outputs together to keep the app icon and UI marks aligned.

The macOS build produces `build/qt/Clibby.app`; the terminal entry point remains
`build/qt/clibby`, and integrations use `build/qt/petctl`. The macOS build bundles its Qt frameworks, plugins and generated QML imports.
The app is relocatable; public distribution signing/notarization are still separate work.

## State and compatibility

The resident and `petctl` share `pets::defaultStateDirectory()`. New installs use
the per-user app-data path for the **Clibby** application and organization. If
that directory is absent and the previous Better Pets directory exists, both
programs reuse the legacy directory in place. If both exist, the Clibby directory
takes priority. An explicit `--state-dir` overrides discovery; supply the same
value to the resident and its clients.

Hook sequence counters have a separate compatibility rule: the adapter keeps
using an existing legacy hook directory even when the new directory also exists,
so its counters and locks remain together. New hook installs use the Clibby
directory; `--sequence-dir` overrides it. See [hook setup](api/agent-hooks.md).
The local socket's `bp-` prefix remains unchanged for compatibility.

Historical research, plans and raw evidence retain the name used when written.
Existing copyright holder notices also remain intact. Historical absolute paths describe the checkout at the time of recording;
current setup instructions resolve paths from the repository root.
