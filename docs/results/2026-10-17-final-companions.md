# Final six companions — 2026-10-17

## Delivered

Maren, Tarn, Pell, Serein, Nacre and Windle complete the approved eighteen-companion
expansion. All eighteen have original selectable models and finite signature
activities under **Companions**. The new six retain the same 96×96 native grid,
nearest filtering and 80 logical pixels/metre.

| Companion | Body and signature |
| --- | --- |
| Maren | Weathered human tidekeeper; gathers rope loops and inspects a knot |
| Tarn | Angular slate-blue four-legged dragon; unfurls, studies and refolds a wing |
| Pell | Visible skeletal humanoid in a traveling coat; balances/studies a pebble and lowers it toward a pocket |
| Serein | Horned orchard faun with cloven hooves; studies a leaf and brushes a sleeve |
| Nacre | Broad-finned cuttlefish; gathers free arms and briefly reveals amber mantle markings |
| Windle | Pale apparition with open side gaps, layered wisps and scarf; gathers its outline and releases |

Original Blender scenes, GLBs, atlases, previews, rigs, scoped licences,
validation and provenance accompany each package. Reproducible recipes live in
`tools/author_final_humanoids.py` and `tools/author_final_creatures.py`. Existing
helpers and earlier packages/recipes are preserved. New meshes span 411–1,852
triangles; tests decode actual geometry/weights, all triangle influence hulls,
real floor-contact triangles and visible skeletal/faun anatomy.

## Motion and anatomy

Maren/Pell/Serein use real articulated arms and hand-owned props. Tarn uses four
actual claw pads and weighted wing membranes. Nacre's two ventral support arms
and Windle's weighted hem supports reuse existing two-contact realization; they
have no humanoid reach or powered-flight capability. Their movement is grounded,
stylized drift rather than a new aquatic/levitation physics system.

Nacre's markings use real weighted coloured patches concealed inside the actual
faceted mantle at rest and briefly exposed by bounded offsets. No new shader,
material animation system or timer is added. Free arms, fins, wisps and scarf
have real weighted owners and measured hulls. Signatures never own root/support
chains; all six pause, cancel and return to quiet rest through the existing path.

Independent review caught a coordinate mismatch in Tarn's tips: Blender's
vertical Z becomes glTF/model Y, while an initial neutral-model Z offset could open
a membrane seam by about 10 cm. Tips now use actual joint-local Y rotation. Native
samples prove shaft alignment through the complete action while the tip visibly
rotates; the existing rotation-space mechanism and older 26 profiles are retained.

## Verification and evidence

Fresh resident/app build and all six native validation/capture runs succeed.
The 32-profile core suite passes finite bounds, C1 cancellation, support isolation,
exact rest, correct hinge spaces and staged channels. Full Python: 109 tests pass.

Fresh full shared-checkout CTest: **89 passed**, three environment skips
(`menu_bar_icon`, `resident_spaces`, `character3d_gpu_render`), no failures.
The exact staged checkpoint builds independently and its full CTest suite
passes **84 checks**, with two environment skips (`resident_spaces`,
`character3d_gpu_render`) and no failures. The exact isolated 2D build (`BETTER_PETS_WITH_3D=OFF`) also succeeds:
**33 passed**, one environment skip (`resident_spaces`), no failures.
The refreshed shared 2D build succeeds, but its last run has 36 passes,
one failure and two skips: the newly added size-change test in
`resident_characters` reports a screen-placement mismatch at scale 2. It is outside
this companion checkpoint; isolated `resident_characters` passes unchanged. The earlier pre-sizing 2D run passed 37 checks with
two skips. Isolated 2D compatibility is checked separately below.
An earlier shared-checkout run while unrelated locomotion/sizing work was landing
reported quiet-rest failures for Mossback/Puddle/Sprig, and a 2D build caught new
sizing tests before their API landed. The existing quiet-rest assertion was kept;
no speculative change to concurrent runtime work was made. Fresh individual
checks for all three older creatures pass. Checkpoint isolation excludes unrelated
pickup projection assertions that arrived in the shared native test file.

Ignored evidence: `results/final-companions-2026-10-17/`. The native captures
contain 630 unclipped signature frames (seven seconds each) and 360 unclipped
walking frames, all 96×96. Final comparison: `final-signatures.gif`,
`final-overview.png`, `final-contact-sheet.png`, `validation.json`, plus raw
`native/` images/logs and capture/assembly scripts. Independent final source,
asset and native visual review has no remaining actionable finding.

Use the rebuilt `build/qt-humanoids/Clibby.app` → **Companions**. The existing
optional SQL-driver packaging warnings recur; signing/notarization remains
separate work.

## First-pass limits

Activities use broad procedural gestures. There is no rope/cloth/finger physics,
detached pebble transfer or persistent pocket storage. Windle's gathering is a
subtle wisp/scarf change rather than an elaborate collapse. Nacre/Windle are
opaque stylized models with real grounded supports; Tarn's wings do not imply
flight. Native captures/tests do not establish battery impact or broad desktop
parity. These limits preserve the approved simple core and current pixel density.
