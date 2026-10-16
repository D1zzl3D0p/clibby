# Wave wrist and 2D knee follow-up

The previous gait correction did include the 3D werewolf: speed, stride and
forward torso balance, including local response after reversal. Its wave pose
was not changed in that pass.

The user reported that the raised wave needs 180 degrees of paw pitch and more
height, and the 2D werewolf left knee still appeared unflipped. Inspection found
the preview reset the hand to rest-world orientation after arm IK, while the
2D authored left leg retained bend +1 against right leg bend -1.

Both 2D werewolf legs now have the same forward authored knee/ankle direction
and bend -1. The existing facing-aware solver reverses both together. A new
loaded-production knee-side check failed on the werewolf before correction and
passes afterward, checking both legs in both travel directions independently
of foot placement. The werewolf is now a separate grounded CTest target so this
anatomy check cannot be omitted by testing only Mallow. All asset changes remain
reproducible through the editable shape recipe.

The 3D wave now pitches the paw smoothly to 180 degrees about model x and returns
along the gesture envelope. The solver rotates the grip offset before solving,
then applies the corresponding world wrist orientation, so the visible grip
continues to track its goal. The greeting target is raised from y=1.70 to y=1.94,
retaining the outward clearance arc and adding forward clearance with a
controlled downward elbow bend. Success uses the same greeting pose;
point/present and explicit holds retain their own wrist behavior.

Native GPU rendering is unavailable in the execution environment. Live rig
transform checks can verify geometry, orientation and motion continuity; they
cannot establish the user's visual judgment of the revised wave.

Fresh full Release rebuild passed. Qt: 19 checks passed, 3 environment skips
(local sockets and native GPU). Python: 50 passed. Live deformation assertions
exercise the actual wave peak, 180-degree anatomical pitch, height above1.9,
grip-path agreement, cheek/torso clearance, bounded wrist/grip changes between
frames and eased first/last movement. The new boundary test found an initial
sharp grip movement; a squared sine envelope gives zero edge velocity.

Quit and relaunch resident and previews to load new assets and code. Corrected
2D knee software captures in both directions are in
`results/wave-knee-correction/`. Native 3D visual acceptance remains outstanding.
