# Raven motion and geometry corrections — 2026-10-16

Status: implemented, rebuilt and verified.

User feedback identified concealed leg joints, excessive planar size, offscreen
heads, unnatural wing sweeps, stationary flight, a native height limit, weak eye
contrast, failed window approaches, slow spatial turns, disconnected wing panels
and sparse tail plumage.

## Changes

- The planar recipe is 25% smaller. Slim tibia/tarsus artwork and a highlighted
  hock replace the opaque leg oval. Winged support reserves 12% of leg reach so
  the solved resting pose retains a visible bend instead of nearly straightening
  the new art. Shared planted-contact guarantees remain intact.
- Planar wing presentation stays aft of the torso; spatial stroke amplitude is
  smaller and trails the body. Physical wing followers still own the stroke,
  extension, effort and force generation. Presentation has no extra animation
  clock. Weighted covers bridge the spatial shoulder, elbow and wrist seams.
- Spatial eyes have larger, lighter rims and glints. The wedge tail has more and
  broader feather vanes. Export: 2,608 triangles, 5,172 vertices, 20 joints,
  two influences at most per vertex, one material and no animation clips.
- Fly up initiates forward cruise and retargets before screen boundaries. Explicit
  fly-to and perch approaches retain destination braking. Grounded planar facing
  follows airborne direction changes; spatial yaw can turn while detached.
  Raven grounded turn transfers take 65 ms while retaining the existing bounded
  foot-angle increments and fixed planted contacts. Wolf timing is preserved.
- Cruise altitude is stored relative to the desktop after perched takeoff;
  reversals cannot reinterpret it against a different floor. Rejected paused
  landing commands preserve cruise intent. Changed display/visual spans update
  the cruise endpoint. Drag, landing and character replacement clear cruise.
- Host constraints follow occupied visual/interaction geometry instead of padded
  canvas dimensions. Both facing directions keep the head inside monitor edges;
  a genuine oversized silhouette still blocks navigation. An instance-only Cocoa
  window subclass overrides frame constraining, allowing transparent padding
  above the screen. It preserves the Qt superclass, focus and Spaces policy.
- Measured skin influence bounds replace oversized circular raven envelopes.
  This avoids inventing vertical clearance around long horizontal feathers.
  Fresh visible window segments and actual sole fit still govern perching.

## Verification

Raw logs and captures belong in ignored results/raven-corrections-2026-10-16/.
Regressions first rejected the old airborne facing, stationary Fly up, slow
turning, almost-straight hock, exaggerated stroke and missing wing covers.
Host regressions cover both-facing screen edges, padding above the native
screen, actual oversized silhouettes and near-ceiling support placement.

Final Release verification: combined Qt 37 passed, four environment skips,
zero failures; 2D-only Qt 27 passed, three environment skips, zero failures.
Python: 52 passed; additional triangle-influence helper regression: one passed.
Native Cocoa checks pass for both ravens: high dragging permits negative padded
window origins, Fly up moves forward and gains height, Land settles on desktop,
and Fly to a window settles on the separate native fixture at y=300 (top edge
268). The 3D approach realizes about 764 pixels of elevation before descending.
These regressions avoid accidentally acquiring the fixture during the preceding
high-drag/desktop stages.

Independent review corrected rejected Land mutating paused cruise, altitude
reference changes after perched takeoff, changed-span cruise retargeting and
full triangle coverage at blended skin seams. Offline bounds group whole faces
by combined influences; live projection uses bounded convex hulls without GPU
readback or per-frame vertex skinning. Measured bounds retain connected feather
panels and remove the old false ceiling.

Rest, walking and two flight poses were captured and inspected. Comparison:
results/raven-corrections-2026-10-16/comparison.html; the earlier comparison URL
now shows the updated captures. The production build/qt/better_pets is rebuilt.
Everyday application geometry, multiple displays, Spaces transitions and battery
impact remain acceptance work; controlled native tests do not establish those.
