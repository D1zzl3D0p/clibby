# Anticipatory motion and facing polish — 2026-10-08

Mallow now selects steps from a bounded forecast of root acceleration and braking,
not only current support error. The first swing starts within 0.1 seconds and
before one pixel of root travel in the tested startup scenario. Touchdown leads
the predicted root, bounded by stride/reach; both stance and swing reach constrain
extreme-speed navigation. No walk clip or repeating gait phase is used.

Mallow artwork now reads as a slight quarter view through asymmetric eyes, offset
head and closer ears. A visual-only `facing` branch holds body art, head, ears and
tail; target intent drives a 0.18-second front-facing turn between right
and left views. Legs and root placement stay outside that branch. Runtime signed
scale is allowed only on the declared visual branch; rest validation stays
positive and the loader rejects facing branches in IK ancestry. Facing completion
joins settling. The preview exports facing direction and blend in capture metadata.

The werewolf already used live procedural IK. It now inherits the anticipatory
planner. Its preview additionally fixes reversed screen-to-ground click mapping
and left/right controls, adds a destination marker and ground path, and exposes
planted/swing contact markers with D. Start it moving with:

```sh
rtk proxy build/qt/characters3d/character_preview_3d --target 60
```

## Verification

Release rebuild passed. Full CTest: 18 passed, 3 skipped, 0 failures. The skipped
checks remain two sandbox-blocked IPC tests and native GPU capture. New grounded
checks cover early steps, direction-correct lead landings, continuous facing,
reversal before velocity changes sign, exact settling, extreme speed and 4,000
samples of actual Mallow endpoint accuracy. Reflection is checked in both CPU
composition and retained rendering. The final turn keeps body volume readable,
centres the eyes briefly and tucks the tail before changing sides.
Live Qt werewolf joints retain world contacts through movement and reversal.
Independent review found no blocking issues.

New actual Qt software motion captures are in ignored
`results/grounded-motion-ixwhv9tn/`: 65 PNGs/sidecars, states, contact sheet and
`approach-reverse-stop.gif`. The final root is -69.9811 at the -70 target and
motion is inactive. Visual inspection confirms opposite quarter views and trailing
tail directions. Separate startup-turn captures are in
`results/grounded-turn-polish/`. These are verification recordings of generated
poses, not runtime animation assets.

Native werewolf pixels, transparency and resource/battery impact remain pending;
resident 3D integration is still deferred. Mallow changes run in both its preview
and resident. The preserved benchmark and old Bramble artwork remain independent.

Earlier edge-on turn experiments remain in `grounded-motion-gjtw99jf/` and
`grounded-turn-polish/turn.png` for comparison; the final revision is
`grounded-turn-polish/turn-revised.png`. Small ear/follower differences at the
midpoint remain a possible art refinement after native visual review.
