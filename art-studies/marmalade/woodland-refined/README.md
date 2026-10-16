# Woodland B facial readability refinement

Preserves the approved full ribcage, shoulder, haunch, compact head, cheek tuft, short neck, ear shape, chest bib, paws, curled tail, broad dorsal bands, nine-colour palette, camera, native96 grid, lighting, and120-frame procedural motion.

The original eye geometry and tiny gold iris were replaced with one small ink eye per side. Eye placement moves outward and upward to survive rasterisation at96px; dark emission increases contrast without adding an oversized eye or catchlight. The two eye clusters read at roughly2 pixels in the resting right heading. Nose height changes from.035 to.045 and width from.075 to.080; faceink contrast also applies to the nose. No new fur texture, whiskers, spots, or inner-ear clutter was added.

120 frames rendered with installed Blender5.2.2 CPU Cycles,4samples, then mapped to the unchanged nine-colour palette with binary alpha. See validation.json for raster checks.

This remains an offline art study. Feet retain the prior heuristic depth adapter around exported controller poses; source.blend stores the first pose and author_render.py rebuilds the sequence. Camera projection and low pixel density still change eye coverage through turning. Eyes and nose remain deliberately small, and do not imply runtime facial controls. No production source or original woodland files changed.
