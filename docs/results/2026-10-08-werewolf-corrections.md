# Werewolf pose corrections — October 8, 2026

The reported hold/wave clipping and feet staying behind during reversal had separate causes. The arm solver bent elbows forward with no side-aware outward clearance, and the wave hand traveled straight toward a raised target beside the cheek. Feet used a fixed world orientation and lateral ground location while the body turned a half-circle, crossing the leg chains.

## Changes

- Reversals briefly pause navigation and alternate lifted foot replants in small yaw increments. Each planted foot keeps its world sole contact and yaw; the torso follows the supported stance. Walking resumes after both feet face the new direction.
- The 3D motion pose supplies authoritative world contacts, per-foot yaw and support state to the live Qt joints and inspection dots. The ankle-to-sole offset rotates with the individual foot before the leg solve.
- Werewolf arms use a mirrored outward/downward elbow bend. Wave/success use an outward raise-and-return arc and a lower lateral hand target; point/present targets also keep the arm beside the torso.
- Limb lengths, world sole contact reconstruction, nearest composition and motion-timer settling remain verified. These changes add no idle timer.

## Evidence

The new reversal regression failed before the core correction with `reversed feet cross the opposite legs`. The live held-arm regression failed before the arm correction with `elbow folds into torso clearance`.

After correction, `character3d_motion`, `character3d_joint_binding`, and `character3d_presentation` pass. Core tests inspect repeated reversals frame by frame: planted contacts and yaw remain fixed, feet lift during replants, contact offsets match the solved ankles, knees/ankles remain on their own side, measured segment lengths remain fixed, and settling stops pose changes. Live checks load the actual 20-joint skin, reconstruct sole contacts throughout navigation/reversal, test both held arms, and sample wave/point/present sleeve clearance against the independently authored cheek taper.

The native GPU test skips because this execution environment has no working Quick3D graphics backend. Offscreen presentation emits existing camera-resolution warnings and a font-alias warning; its property checks pass. No native pixel, exhaustive mesh collision, energy or desktop rendering acceptance is claimed.

## Native check

Relaunch `build/qt/characters3d/character_preview_3d` from a normal Terminal. Use W to wave, H to hold, R to release, and Left/Right to reverse. D shows contact/grip dots: a planted sole should stay fixed while the other foot lifts and turns, and the body should remain aligned with the stance. Inspect the actual sleeve, paw, knee and ankle silhouettes from both directions. The resident desktop pet is still the separate 2D shell.
