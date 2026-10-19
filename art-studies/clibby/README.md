# Clibby — vector refinement 03

Reconstructed from the user-selected lilac app tile, rather than tracing the
generated image. This approved geometry is the source for the production artwork in `assets/brand/`, wired into the resident app. See `docs/brand.md` for the complete identity and runtime export instructions.

## Geometry

- A 256 × 256 coordinate space.
- One closed outline forms the outer C and the open, offset head cutout.
- The outer curve starts from a circle centred at (128, 128), radius 104.
  The right terminals and inner head use a small number of optical curve adjustments.
- The head is almost centred horizontally, retaining a slight rightward bias and the existing downward offset. Its contour and eyes moved 14 units left in refinement 02. Refinement 03 enlarges the head contour, keeps the eye positions, and shortens the upper-right transition with a rounded terminal, avoiding the long neck-like edge. Its opening is continuous with the
  surrounding background; there is no separately coloured face.
- Two ear curves mirror about x = 137. Both eyes have radii (10, 18), centres
  (116, 145) and (156, 145), and share one baseline.
- No strokes, masks, filters, raster content, fonts or external dependencies
  occur in the SVG masters. Transparent mark variants use the same outline.

The head is deliberately not an exact circle: the priority is the approved
tile's friendly shape and a clear C. The outline is authored in `build.py`;
edit it there and rebuild to keep every rendition consistent.

## Colours

| Token | Value | Use |
| --- | --- | --- |
| Ink | `#292638` | Mark and eyes |
| Lilac | `#B9ADEB` | App tile; visible through the face |
| Cream | `#F7F4EE` | Proof background and reversed mark |

Each icon uses two colours, or a single colour on transparency. Rounded-square
tile geometry is a presentation study, not a packaged macOS application icon.

## Files

- `mark.svg`: editable transparent master.
- `mark-reversed.svg`: identical geometry in cream.
- `menu-template.svg`: identical geometry in black on transparency, ready for
  later template-image integration. Native menu bar behaviour is not tested here.
- `app-icon.svg`: flat lilac tile with the ink mark.
- Matching PNG exports and size samples (16, 18, 22, 32, 64, 128 and 256 pixels).
- `proof.png`: large mark, tile, true pixel-size samples and light/dark previews.
  The heading is a proof label, not a final outlined wordmark.

## Reproduce

From the repository root:

```sh
rtk proxy .venv/bin/python art-studies/clibby/build.py
```

Uses the project's Pillow installation, `rsvg-convert`, and macOS Arial for
proof labels only. SVG artwork itself has no font requirement. The renderer
exports the same SVG geometry at every size; no size-specific redraws exist.
