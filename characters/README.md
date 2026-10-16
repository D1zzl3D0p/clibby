# Original procedural pixel examples

`soft-creature` (Mallow) is a flat cream silhouette with black eyes, asymmetric
ears, a curling tail and small feet. It has no outlines, shading or decorative
face detail: this revision is intended for reviewing grounded movement and IK.
`werewolf` (Bramble) uses a charcoal/blue-gray
palette, amber eyes, muzzle, chest ruff, claws and segmented digitigrade legs.
These are first-iteration original character designs, with no third-party art.
Each package's MIT `LICENSE` covers its art and anatomy.

`marmalade` (Marmalade) is an original ginger tabby with a horizontal torso,
four independent two-link contact chains, separate eye artwork and a curled
tail. It is the first planar `quadrupedWalk` package. Its four named roles are
deliberately shuffled in JSON to exercise canonical loader normalization.
Standing, walking, turning and pickup/drop are procedural; seated poses and a
3D version are not included. Regenerate or verify it separately:

```sh
rtk proxy .venv/bin/python tools/author_marmalade.py
rtk proxy .venv/bin/python tools/author_marmalade.py --check
```

Launch `build/qt/character_preview --character characters/marmalade` from the
project root, or select Marmalade from the rebuilt resident's character menu.
The new package is copied beside the resident by the existing asset target.

Regenerate both packages with the editable shape source:

```sh
rtk proxy .venv/bin/python tools/author_characters.py
rtk proxy .venv/bin/python tools/author_characters.py --check
```

Pillow draws the explicitly authored polygon/ellipse recipes at integer pixels,
without antialiasing. Edit the coordinates and colors in the authoring script,
then regenerate. No frame sequence is generated: each atlas contains individual
art pieces that the runtime transforms. `preview.png` is an independent,
transparent rest-pose composition for reviewing silhouette, layers and seams.
The runtime preview remains the authority for animated pose inspection.

Manifest coordinates are character-local design pixels, y increasing downward.
The required top-level `version: 1` selects this production package schema.
`origin` records the canvas ground anchor as metadata; it does not translate
the root bone or rendered pose. Bone coordinates already locate art in the canvas.
Mallow's canvas is 128×144; Bramble's is 144×176. Both reserve a 16-unit motion
envelope beyond the canvas. Each attachment names an atlas rectangle, a pivot
inside its image, a local offset and an independent drawing order. Multiple
pieces share the head and trunk bones to separate expression/ruff from silhouette.
Head and ears are ordinary bones. Bones can be invisible; no fixed part list
is imposed on the loader. The atlas is a padded transparent 512×256 PNG.

Mallow declares grounded `locomotion` metadata; its `soft` controller label remains
for compatible legacy action consumers. It has two contiguous hip–knee–ankle
chains with unit scale and equal 12.53-unit segments, an authored outboard knee
bend, and ankle-local foot contacts at `[0, 3]`. The floor is canvas Y=125;
stance and step planning operate in world coordinates. These are real limb
attachments, not separate walking frames. CPU composition with nearest-neighbor
sampling is its intended default; palette dithering is off.

Bramble uses the legacy `articulated` controller. Semantic bindings identify body,
head, ears, tail and feet. Bramble's upper/lower leg and arm bones
demonstrate independent anatomy but the first controller action set does not
yet supply detailed elbow/hock motion. The input polygon, motion envelope and
ground contact anchors serve different purposes and are declared independently.
The package contains no executable behavior or external path dependencies.
