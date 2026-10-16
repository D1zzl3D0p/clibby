# Shorter planted transfer and 3D reversal motion

The user requested trying shorter planted times to reduce visible stretching,
then reported a knee/paw wiggle during repeated 3D direction changes. Hard sole
pins and one-foot flight remain mandatory.

## Compared timing

Compared 40, 20, 10 and 0 ms extra double-support dwell. Ordinary swing duration,
body travel, floor pins and support-force tuning remain unchanged. A supporting
foot still waits for the entire opposite swing; this removes extra delay after
touchdown. Frame intervals quantize the actual handoff.

Selected **10 ms as the shared default**, retaining a brief contact handoff and
walking bob while reducing stretch. Zero gives still lower peak 2D stretching;
after the paw-turn correction it also improves final 3D peaks, but is slower to
settle in the 1× 3D three-trip comparison. The selected value is a compromise,
not a claim that 10 ms minimizes every metric. The earlier 3D comparison before
the yaw fix found zero less consistent; the final comparison supersedes that.

At 2× travel, 120 Hz, final maximum segment length as a ratio of authored length:

| Character | 40 ms | 20 ms | 10 ms | 0 ms |
| --- | ---: | ---: | ---: | ---: |
| Mallow 2D | 1.630 | 1.515 | 1.467 | 1.389 |
| Bramble 2D | 1.465 | 1.380 | 1.339 | 1.287 |
| Werewolf 3D | 1.1183 | 1.1004 | 1.0913 | 1.0783 |

The selected interval reduces excess stretching by approximately 26–27% for
the 2D characters and 23% for 3D in these cases. 2D walking bob remains visible:
Mallow 5.50→4.91 px and Bramble 9.67→8.16 px. All 64 2D timing/speed/frame-rate
comparisons settled after reversal, with no double-air frames and maximum
rendered contact error 1.15e−13. All final 3D timing/speed comparisons settled,
with no pin movement or double-air frames.

## Wiggle corrections

- The analytical knee bend has a square-root singularity at a perfectly straight
  leg. Shared contact IK now smoothly transitions to elastic reach over a narrow
  1% band, retaining small flex instead of snapping at full extension. Both
  renderers use this common correction. Targets remain exact; arms remain rigid.
- The 3D knee pole previously rotated with the torso while its paw was planted.
  It now follows that paw's world heading, transformed into the torso solve frame.
- A lifted paw could advance up to 0.8 radians because it was bounded only by
  its partner's heading. It is now also bounded to 0.4 radians from its own
  starting heading. Peak yaw movement per 120 Hz frame halved, 0.07797→0.03898
  radians. Turn swing duration stays 0.16 s, so larger turns can require more
  replants. The 3× three-trip comparison takes about 0.53 s longer after this cap.

This softens knee transition and staged paw twists; it does not claim all visible
3D jitter is eliminated. Remaining motion can be judged in the restarted native
preview. Offscreen joint verification does not establish native GPU appearance.

## Verification and evidence

Timing regression failed before the new setting controlled the gait gate. Shared
reach-sweep, 3D paw-relative knee-plane and own-paw yaw-budget regressions each
failed before their fixes. Production checks retain exact rendered contacts,
strict single-foot flight, continuous travel, normal swing duration and settling.

Probe sources, complete intermediate/final CSVs and raw logs are saved under
`results/shorter-plant-times/`. Fresh full build: 20 Qt checks passed, 3 environment
skips (native sockets and GPU framebuffer); 50 Python checks passed. Live
20-joint deformation/sole-pin verification passed at 3× speed with the final
default, knee poles and yaw cap. Restart the rebuilt binaries for visual review.
