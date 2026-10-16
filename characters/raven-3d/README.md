# Original Raven 3D

Editable source: `raven.blend`; reproducible recipe: `tools/author_raven_3d.py`. Original mesh and atlas, MIT licensed. Cornell identification informed anatomical proportions: horizontal elongated body, thick neck, heavy bill, shaggy throat, short exposed tarsi, broad overlapping folded feather vanes and long wedge tail. No photographs or external artwork are included.

2,608 triangles, 5,172 exported vertices, 20 deform joints, one material, one embedded nearest sampled 256×256 atlas and up to two influences per vertex across continuous weighted wing covers; no animation clips. Measured unfolded wing chains span about 2.08 times the authored bill-to-tail length, before primary vane reach. Runtime wings must abduct, unfold and roll the vertical flank vanes into a horizontal flight plane.

Blender CPU side/front/three-quarter renders inspect authored rest geometry; native runtime flight validation belongs to the shared motion and renderer integration.

Reference: https://www.allaboutbirds.org/guide/Common_Raven/id

October 16 corrections add larger slate eye rims and glints, eleven broad wedge tail feathers and a full tail covert mound. One continuous mesh cover spans each shoulder–elbow–wrist chain with interpolated two-bone weights. Authoring validation rejects missing blended coverage at either seam. Runtime presentation maps the shared realized stroke into a trailing sweep with a reduced vertical component; no independent flap clock or forces are introduced.

Runtime visual envelopes are measured from the actual GLB POSITION/JOINTS/WEIGHTS and inverse bind matrices by `tools/measure_raven_bounds.py`. `skinBounds` groups each triangle’s union of vertex influences into local joint boxes, projected as a convex hull under live joint transforms. This conservatively includes blended covers without a large circular wing radius or per-frame vertex readback. The wolf retains its prior envelopes.
