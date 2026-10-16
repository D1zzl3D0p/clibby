# Occasional eye blinks

The user requested inexpensive, occasional blinks as a small personality cue.
The resident now uses one Qt-free seeded blink driver for supported eyes across
Mallow, Bramble and both Corvid/Werewolf dimensions. Quiet intervals vary from
7 to 16 seconds. Each blink closes over 55 ms, holds closed for 25 ms and reopens
over 110 ms, using a smooth curve rather than an authored animation clip.

The driver contributes one quiet deadline to the existing resident timer and
requests at most 60 Hz only during its 190 ms active interval. It does not wake
the limb followers or restart grounded movement. Pause, hide, suspend and drag
freeze progress without deadlines; resume continues an active blink without
catch-up. Late clocks skip missed blinks instead of queuing them. Character
selection presents open eyes and starts a fresh interval. Missing eye capability
disables blinking without adding scheduling work.

## Rendering and assets

`Pose::eyeOpenness` defaults to one. The 2D renderer compresses only artwork
bound to the semantic eyes subtree about its pivot, leaving joints, contacts and
utility anchors unchanged. Retained and composited pose caches include the
scalar. Nonfinite openness renders as open, and values clamp to [0,1]. Closed
eye artwork retains a thin line through a minimum vertical scale of .05.
Bramble and Corvid now separate eyes from head artwork; Mallow's pivot is
centered for closure. Open Mallow/Bramble rest and Corvid rest/flight software
images remain byte-identical to the original recipes.

Each 3D model adds two dedicated eye deform joints: Werewolf has 22 and Raven
20. `WolfMotion` applies openness only to their authored local vertical scale.
The geometry count is unchanged. Export checks found 80 eye vertices per rig;
closure moves those vertices, leaves other vertices fixed and restores original
geometry on reopening. Live binding checks also verify eye scaling and unchanged
transforms for unrelated anatomy. Texture resolution and materials are unchanged.
The frozen benchmark keeps its existing poses and behavior.

## Verification

Fresh complete Release build succeeded. Full integrated CTest: 37 passed,
four environment skips, zero failures (23.90 seconds). Python: 50 passed.
The skips were resident_ipc, resident_spaces, resident_ipc_cli and
character3d_gpu_render. New timing, raster/cache and resident tests cover complete
closure/reopening, quiet scheduling, finite bounds, clock changes and frozen
pause/resume. The 3D resident test observes actual eye-joint closure and pause;
the presentation check verifies closed geometry and restoration. Both model
joint-binding checks pass. Independent review found no material issues.

Ignored test output and closed-eye Blender review images are under
`results/blinks-2026-10-12/`; exact 2D open-equivalence evidence is under
`results/eye-blinks-2026-10-12/`. The images have been inspected. Software and
Blender checks do not establish native GPU appearance or whole-system battery
cost. Restart the rebuilt resident to review the live blink feel.
