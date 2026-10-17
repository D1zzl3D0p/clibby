# Approved companion expansion

The user approved the complete eighteen-character list on 2026-10-17 and
explicitly requested implementation beginning with Nell, Alden, Saffron, Tess,
Rowan and Vesper. Names, descriptions, geometry, costumes and motion are original;
Dweller informed breadth and readable pixel-art presentation only. No borrowed
sprites, extracted models or copied descriptions enter the product.

## Approved catalogue and order

First batch: Nell (After-Hours Reader), Alden (Roadworn Sentinel), Saffron
(Hearth Alchemist), Tess (Clockwork Repairer), Rowan (Woodland Surveyor), Vesper
(Velvet Antiquarian).

Approved subsequent companions: Iona (Field Cartographer), Milo (Wayside
Musician), Orris (Traveling Clockmaker), Fen (Lantern Tinker), Oona (Orbital
Geologist), Quillan (Quiet Visitor), Maren (Tidekeeper), Tarn (Slatewing Dragon),
Pell (Bone Collector), Serein (Orchard Faun), Nacre (Drifting Cuttlefish), Windle
(Wandering Apparition). Approval of the catalogue does not mean these later
characters have been implemented in the first batch.


| Later companion | Approved identity and activity |
| --- | --- |
| Iona | Lean human field cartographer; unfolds a map, traces a route and adds a mark |
| Milo | Human wayside musician with long coat; tunes a string and taps a silent rhythm |
| Orris | Older dwarf clockmaker; winds a tiny clock and adjusts square spectacles |
| Fen | Wiry goblin lantern tinker; opens a lantern and adjusts its shutter |
| Oona | Human orbital geologist with visible face; inspects a mineral under a helmet light |
| Quillan | Slender alien with folded ear-fins; studies a hand and practices a human greeting |
| Maren | Weathered human tidekeeper; coils rope and checks a knot |
| Tarn | Slate-blue angular dragon; unfurls, inspects and refolds one wing |
| Pell | Skeletal humanoid in a traveling coat; balances and pockets a smooth pebble |
| Serein | Orchard faun with cloven feet; considers a leaf and brushes it from a sleeve |
| Nacre | Broad-finned cuttlefish; gathers arms and briefly changes mantle pattern |
| Windle | Loosely human apparition with scarf; gathers its outline before drifting again |

## First batch design

Use the existing original low-poly Blender authoring and pixel-rendered 3D path:
96×96 native render, 80 logical pixels/metre, nearest filtering, no new timers.
Adult figures are about five to six heads tall, with broad clothing shapes,
small readable faces and little surface noise. Individual height, build, face,
hair and complexion matter; they are not costume swaps on one identical figure.
The first implementation includes walk, turn, gaze, blink, pickup/drop, settled
rest and a finite signature using actual articulated arms and bound props.

| Character | Form and material | First signature |
| --- | --- | --- |
| Nell | Older dark-skinned human, silver-streaked hair, cardigan, glasses; soft broad figure | Holds her own book; marks/turns a hinged page and briefly considers it |
| Alden | Broad mature human, open face, grey facial hair, practical armour and short faded cloak | Looks down and checks a coat fastening; rolls shoulder into rest |
| Saffron | Taller human woman, swept hair, long layered coat, crooked hat | Lifts/tilts a small bound vial to examine its contents |
| Tess | Short stocky human, cropped curls, rolled sleeves, tool apron | Turns and inspects a small held mechanism |
| Rowan | Lean androgynous elf, olive complexion, angular face, long ears, short mantle | Examines a held seed/sample and lowers it |
| Vesper | Tall pale humanlike vampire, long dark hair, narrow formal coat and collar | Opens and studies a small pocket watch, then closes it |

These concrete complexion/body choices are implementation calls within the
approved diversity/art direction, not separate user-selected portraits. Props
belong to the character hierarchy and move with a real hand or torso attachment.
No desktop object manipulation, cloth simulation or independent dropped props
is implied. Two-hand contact or finger manipulation must not be claimed unless
actually implemented and checked. Full elaborate vignette polish may follow
the first finite signatures, with omissions recorded explicitly.

## Architecture and invariants

- Add only fixed trusted built-in IDs `nell-3d`, `alden-3d`, `saffron-3d`,
  `tess-3d`, `rowan-3d`, `vesper-3d`, with matching asset/type skin names.
- Reuse the measured two-contact biped solver and actual three-joint arms.
  Arms are real optional capability, unlike earlier decorative catalogue arms.
- Signature channels use model-neutral axes, return to exact core zero and
  cancel through the existing 0.35-second release. Additive arm gestures are
  applied after arm realization; support legs remain solver-authoritative.
- Child prop hinges rotate about actual authored pivots, with no duplicate
  independent overlay or floating copy. Measured influence hulls cover masks.
- Preview/native validation must exercise actual weighted joints, rendered sole
  pins, actual hand-to-prop binding, both headings, pause, interruption and rest.
- Existing names and JSON/IPC fields remain compatible. Visible selector copy
  may say Companions, without gratuitously renaming protocol/package fields.
- Retain single resident scheduling, sparse personality and signature deadlines,
  no battery or desktop parity claim from these tests.
- Preserve concurrent identity/productivity changes and all raw evidence.

## Acceptance

Six independently selectable original companions; source models, GLBs, atlases,
rigs, validation, provenance and licences. Original recipes reproduce them.
Per-character native rest/action/walk review and a comparison animation must
show faces, different silhouettes, bounded frame coverage and planted soles.
Core tests and the existing catalogue, original wolf/raven and resident suites
must remain passing; environment skips remain explicit.
