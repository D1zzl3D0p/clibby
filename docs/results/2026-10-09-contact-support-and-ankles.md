# Contact support and ankle articulation

The user's native review found excessive continuous 2D knee flexion, overly
straight 3D hips with repeated little hops, and no visible drop when both 3D paws
were airborne. The user clarified that the 2D problem concerns planted stance,
not foot lift, and chose gentler gravity with stylized support to retain the
existing walking rhythm. Horizontal body-led travel and eased lateral
acceleration/braking remain the intended behavior.

The support solver is a simplified vertical mass with axial per-leg springs
and analytical IK. It does not simulate independent hip/knee joint torques.
Strict 3D support must use the actual posed soles: a planar planner contact
cannot carry weight when 3D reach correction or a turn has lifted the visible
paw. Artificial vertical height bounds also cannot substitute for airborne
gravity. Landing contact loading and a short turn replant cooldown reduce
immediate relaunches.

The 3D paws now pitch independently of the shins during swings, with flat
endpoints and rotated sole offsets in the reach/contact solution. The live
skin adapter cancels inherited shin rotation before applying yaw and ankle
pitch. Gesture paths follow pelvis height; elevated held targets use a lower
elbow pole so body bounce does not fold the sleeve into the cheek.

The 2D profile uses stiffness 1000, damping 64, gravity 1 per unit of minimum
leg reach and a 2% stance reserve. A small upright assist (10% of stiffness,
20% of damping) remains active during visual flight. Idle extension is 98%;
planted median extension in sustained speed 1/2/3 and quantized host checks is
95.7–96.7% for Mallow and 95.4–97.1% for Bramble. No emergency swing-duration
shortening remains. These figures describe geometry, not native visual acceptance.

The strict 3D profile uses stiffness 120, damping 24, gravity 12 per unit of
measured minimum reach, 60 ms contact loading and 5% stance reserve. The planner
proxy uses full measured 3D segment lengths and the actual 22.5 cm ankle-to-sole
offset. It no longer assumes the shorter authored planar ankle offset. Flight
can cause about 23 cm of crouch at 2x travel; tests require physical hip clearance
rather than imposing the previous artificial vertical height bound.

Fresh full build succeeded. CTest: 19 passed, 3 environment skips (two native
socket checks and GPU framebuffer). Python: 50 passed. The live 20-joint skin
verifier also passed at speed 3, including independent ankle orientation, rotated
sole contact reconstruction, wave pitch/continuity and arm clearance while holding.
Regression checks cover unchanged ordinary swing phase advancement, planted
extension, 2D artwork floor clearance, actual both-paws-airborne freefall, exact
limb lengths/contact positions, sustained under-hip crossing, reversal and settling.

Raw 2D captures and reviewed contact sheet are in
`results/contact-support-and-ankles/`; `walking-review.gif` shows 40 frames at
50 ms intervals for both characters at speed 2. Native 3D pixels could not be
reviewed in this environment. Quit and relaunch the rebuilt previews/resident for
native appearance review; software checks do not establish motion aesthetics,
desktop parity or battery impact. Resident 3D integration remains a separate plan.
