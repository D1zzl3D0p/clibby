# Utility body fixtures

The resident consumes `qt/characters/fixtures.h` without another library:

- `utilityAnchors(definition, pose)` returns canvas-local head, card and effect
  points. Card/effect clear the transformed art below the head hierarchy;
  a body binding provides the head fallback. Apply the existing canvas/window
  transform before placing ordinary accessible text or controls.
- Optional `gripLeft` / `gripRight` bindings return posed grip points. Mallow has
  neither, so the fixture returns no grip instead of pretending it has hands.
- `Gesture` expresses Look, Wave, Point, Present, Nod, Success and Failure.
  `gestureAction` maps these to existing head/body actions for armless bodies.
  This is a mapping API, not automatic resident dispatch. The shell must call
  it when selecting a notification reaction. The caller owns a single bounded reaction; `gestureEnvelope` is exactly zero
  outside its one-second interval and never owns a timer. The resident maps its session reactions through
  Nod, Look, Success and Failure; the seven arm gestures are exercised in the
  werewolf preview.

The 3D core adds unequal-link `Arm` and `solveArm`, plus optional arm fixtures
and explicit `Motion::armGoal` / `clearArmGoal`. Goals are model-space grip
positions. Grip offsets retain a model-axis orientation; unreachable targets
clamp the hand to the chain's reach, preserving both segment lengths. Each arm
has an explicit elbow bend preference. Anatomical angular limits and wrist
orientation goals remain future work; these fixtures enforce reach, not a full
human joint-limit model. Arm goals never change the leg planner or leg pose.

The live werewolf preview measures shoulder/elbow/hand chains from its existing
rig and publishes scene-space head/card/effect and grip points. It recomputes
shoulder position after spine followers, applies arm IK, then restores the hand's
body-relative rest-world orientation. Held grip goals are static and create no
animation timer; an existing moving body continues to solve them. `gesture`
accepts look/wave/point/present/nod/success/failure and settles after one second.
Explicit reach goals take precedence over gesture arm goals. Pause suspends this
same existing preview timer; elapsed gesture time does not accumulate while
paused. No rendering filters or grounded contact ownership changed.

Preview controls: W wave, H left-hand grip goal, R release, D contact/grip dots.
The model-space grip target remains attached to the avatar when it turns or
moves. Desktop/world-space reaching and native 3D resident integration are not
implemented. Text remains outside the low-resolution 3D render.

Verification covers transformed 2D anchors, armless fallback, bounded gesture
endpoints, reachable/unreachable/coincident unequal arm goals, finite input,
leg independence, and live grip alignment with grounded walk/reversal and gesture
settling. Offscreen binding and presentation checks verify scene transforms and
nearest properties; they do not establish native GPU pixels, focus behavior,
quiet resource use or battery impact. Native GPU capture remains skipped here.
