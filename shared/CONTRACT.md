# Shared desktop-pet fixture contract, version 1

This frozen fixture was created for the retired toolkit study and remains the
Qt motion regression contract. It is not the future modular character-package
format. Historical Godot invocation details below describe the archived engine.

`rig.json` is frozen version 1. Coordinates are logical pixels, x right/y down;
positive radians rotate clockwise. The transparent surface is 256 × 256. The
PNG contains straight RGBA, sRGB artwork; engines convert to their native
premultiplied representation once on upload. Use linear texture filtering, no
MSAA, nearest integral atlas rectangles, no effects. Each part's local matrix is
`translate(x,y) * rotate(rotation) * scale(scale_x,scale_y)`. Its texture occupies
`[-pivot.x,-pivot.y,size.x,size.y]` before transformation. Parents precede children;
draw in the listed order (depth first gives this same order). Null parents attach
to a virtual root at `origin`. `position` and `rotation` are base local transforms.

## Invocation and fixture output

Both apps accept `--scenario NAME --duration SECONDS --output PATH --assets DIR`
and optional `--dump-poses PATH` / `--screenshot PATH`. Godot receives these after
its `--`. Duration must be finite and positive, scenario recognized, assets valid.
Both also accept `--warmup SECONDS` (nonnegative and less than duration), and
`--ready-file PATH`. Duration includes warmup. After warmup the motion state and
measurement counters reset to tick zero. After setup, immediately before starting
those clocks, publish the readiness file atomically as `{"ready":true,"pid":PID}`.
The collector waits for readiness instead of treating process creation as the
scenario origin; its file-observation timing is explicitly approximate.
Reject invalid arguments with a nonzero exit and clear message. Output is buffered
telemetry written at exit. Dump poses outside timing runs: read `golden.json` and
write `{ "version": 1, "poses": [...] }`, evaluating each golden entry's scenario
and tick. Each pose is `{ "scenario": str, "tick": int, "root_offset": [x,y],
"parts": [{"id": str,"x":number,"y":number,"rotation":number,
"scale_x":number,"scale_y":number}, ...] }`. Order is rig order. Compare numeric
components with absolute tolerance 0.0001. Scenario/tick are metadata; frozen-pose
equality means equality of `root_offset` and `parts`.

## Time, phases, suspension

The reference API `evaluate_pose(tick,scenario)` returns transforms only:
`{"root_offset": [x,y], "parts": [...]}`. `tick` is a nonnegative integer of
**wall timeline** intervals at 60 Hz. State at tick zero is precisely the rig's
base pose, root `[0,0]`, active counter `a=0`, walking counter `w=0`, cumulative
walking distance `d=0`, tail angle `q=0`, tail velocity `v=0`. No initial motion
formula is applied. Complete each interval j (j starts at 1) using the phase at
`(j-1)/60`, then produce pose at tick j. Thus the first walking step after a
45-second idle phase occurs at tick 2701; tick 2700 is the pose at its boundary.

`phase_at(seconds, scenario)` uses finite nonnegative seconds. Static,
idle-motion, walk-fixed, walk-desktop and hidden map to themselves. For
mixed-active, `seconds % 60 < 45` is idle-motion, otherwise walk-desktop. For
mixed-rest, `<40` is static, `[40,55)` walk-desktop, `[55,60)` idle-motion.
At 60 seconds cycles restart. Static and hidden skip the entire integration and
pose update: counters, transforms, tail and root freeze. Resume preserves state;
active time never includes the suspended interval. A deterministic tail spring
provides the resume transition. Stop per-frame application scheduling during
static/hidden; schedule only the next phase or end deadline. Root remains at its
last offset when walking ends (no discontinuous reset at idle/static boundaries).

At runtime advance only completed 1/60-second intervals, no interpolation. Cap
catch-up at four steps per callback, drop excess intervals and count dropped
steps/overruns; repeated overruns invalidate cadence equivalence. Scenario phase
deadlines follow wall time even after an overrun; never integrate static/hidden
intervals or replay suspended elapsed time. Offline golden evaluation has no
overruns and computes every interval exactly. Use double-precision arithmetic.

## One active step (exact evaluation order)

Use `tau = 2*pi`, `dt=1/60`, and the motion constants in rig.json. `clamp` is
min(max(value,lower),upper). Keep all state across phase transitions.

1. Increment `a`. If phase is walk-fixed/walk-desktop: save old root x, increment
   `w`, set `root.x=32*sin(tau*w/240)`, and add `abs(root.x-old_root.x)` to `d`.
   Otherwise root and `w,d` stay unchanged. Root y is always zero.
2. `g = tau*d/32`. `gait = sin(g)` during walking, otherwise zero.
3. Scripted head target: `tx=24*sin(tau*a/180)`, `ty=8*sin(tau*a/240)`.
   `head_angle=0.5*atan2(ty,48+tx)`. `breath=sin(tau*a/180)`.
4. Tail target `target=0.25*sin(tau*a/120)+0.1*gait`.
   `v=clamp(v+(36*(target-q)-12*v)*dt,-2,2)`;
   `q=clamp(q+v*dt,-0.6,0.6)`. If `abs(q)==0.6`, set `v=0`.
5. Start all local transforms from rig base values, scales 1. Set body
   `y=base_y-1.5*breath`, `scale_y=1+0.015*breath`.
   Add `head_angle` to head rotation. Add `q` to tail rotation.
   Add `+0.35*gait` to rear_left/front_right rotation and `-0.35*gait` to
   rear_right/front_left rotation. For both eyes add `tx/24*1.5` to local x and
   `ty/8*0.75` to local y. All other components remain base values.

The root oscillation is 64 pixels peak to peak. Distance accumulation, including
reversals, drives gait. In walk-fixed add root offset to the virtual render root
inside the fixed window. In walk-desktop apply it to the desktop window position
and leave the render root at origin; mixed scenarios retain this desktop mapping
for the entire run, including subsequent frozen phases. Choose an initial window
origin with 32-pixel movement margins on the available screen; clamp desktop
position to screen bounds if needed and record the clipping. Window integer
rounding is desktop-adapter behavior, not part of the floating-point pose fixture.

`hit_polygon` is a conservative, clockwise convex enclosure of the entire animated
silhouette, including ear tips, paws and tail excursions. Its vertices are
**absolute 256 × 256 surface coordinates and already include `rig.origin`**.
Do not add `rig.origin` again. Only in walk-fixed, add `root_offset` exactly once
to each polygon vertex. In walk-desktop and both mixed scenarios, add no pose
offset to the polygon because window movement carries the offset. The enclosure
retains transparent margins for passthrough, including at the ±32-pixel walking
extrema. Regression tests transform nonzero-alpha atlas pixels through the full
part hierarchy at golden boundaries and sampled motion extrema and check polygon
containment. Desktop input, focus, and transparency still need untimed visual
checks in addition to golden poses.
