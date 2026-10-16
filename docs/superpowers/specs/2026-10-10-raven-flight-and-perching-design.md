# Raven comparison, procedural flight and shared perching

Date: 2026-10-10
Status: approved by the user; implementation and verification in progress.

## Intent

Build original 2D and 3D ravens to compare presentation and exercise a different
body plan. Both have corresponding torso, neck/head/beak, tail, paired legs and
articulated wings. They share action meanings and physical motion, while their
rigs and pose solutions can differ. Include grounded walking, hopping, powered
flight, takeoff, landing and perching/walking on window tops.

Window support is a general pet capability, not a raven-only behavior. Reaching
a surface is independent of occupying it. Pets without flight can be placed on
a window; future climbing can use the same support boundary.

## Approach and alternatives

Extend the existing shared pickup/drop dynamics, add procedural wing actuation,
and generalize ground geometry into identified support surfaces. Keep existing
Qt Quick 2D and optional Qt Quick 3D renderers.

A scripted flight path would simplify animation but would disconnect wing motion
from lift. A full aerodynamic or articulated rigid-body simulation would add
cost and tuning complexity beyond this comparison. Select a lightweight body
force model driven by solved wing motion, with procedural limb IK and followers.

## Shared body and flight motion

Extend the Qt-free body dynamics currently owned by DropMotion. One authoritative
position/velocity state handles gravity, hop impulse, wing force, drag and contact
transitions. Keep grounded leg support in the existing shared axial support law;
it must not become a second flight integrator.

Distinguish supported, held and airborne contact state from the selected action
(walk, hop, takeoff, powered flight, approach, landing and recovery). Ordinary
walking retains its pinned soles and alternating single-foot swing rule. Hop and
flight explicitly detach both feet. Launch clears contacts once; touchdown
installs new anchors once and hands incoming momentum to compliant support.

Initial desktop travel is horizontal position plus elevation, shared by both
renderers. 3D remains a perspective presentation of that travel plane. Free
depth navigation is deferred. Host-realized placement and clipping feed back into
motion; fractional travel is retained and elevation is applied exactly once.

Hop begins with a bounded upward impulse and proceeds ballistically. Takeoff can
combine leg push-off and wing force. Powered flight uses short deterministic
substeps and descending swept contact detection; zero wing force must preserve
passive falling behavior. Pause/hide/sleep freeze state without catch-up.

## Procedural wings and lift

Add explicit left/right wing capabilities and articulated shoulder/elbow/wrist
bindings. Wing fold, extension and stroke are procedural goals; anatomy limits
and bounded followers produce realized joint state. The motion core owns the
stroke oscillator, smooth activation and amplitude/frequency changes. Behavior
requests travel, landing or wing gestures, not per-frame joint transforms.

A shared force evaluator uses actual wing extension, orientation, angular speed,
stroke direction and body-relative air velocity. Downstroke supplies bounded
lift/thrust; recovery folds or feathers to reduce the opposing force. Drag and
gravity remain active. A stationary wing produces no stroke lift. Gliding, if
enabled, requires forward airspeed and uses a separately bounded passive term.

Flight control adjusts wing effort and body attitude toward bounded position and
velocity goals. It cannot set altitude directly or introduce a hidden hover
force. If available lift is insufficient, the body descends. This is stylized,
tunable flight physics, not a claim of accurate bird aerodynamics.

Both dimensions consume the same physical wing state. The planar renderer maps
that state to layered art; the spatial renderer solves its measured wing rig.
No animation clips or separate renderer-owned flap clocks.

## Support surfaces for every pet

Introduce Qt-free surface observations with stable source-qualified identity,
horizontal usable span, top height, movement and availability. The desktop floor
is the first provider. A separate native macOS provider supplies visible window
top edges; window discovery and platform access stay outside behavior/motion.

The provider must filter the pet's own windows, unsupported/minimized/off-Space
windows and covered edge segments. Unknown or stale geometry is unavailable.
Observe geometry at bounded intervals or native change events through the
existing scheduler; do not add permanent high-frequency window polling.
Native discovery feasibility and any required system permission must be verified
before promising support across applications. No window text/content is needed.

Surface selection, contact acquisition and grounded walking are shared across
pets. Reachability filters automatic destinations using the pet's locomotion
capabilities. Manual placement can acquire a valid top edge even when the pet
cannot independently reach it. Narrow edges require space for the actual stance;
decline a perch that cannot support it.

Window tops are one-way landing surfaces: acquire support only on a descending
approach from above, inside a usable segment. Feet do not attach while passing
upward through an edge. Supported soles remain fixed relative to their moving
surface, while their world positions follow that surface. Sudden movement cannot
manufacture unlimited body momentum. Loss of the edge, closure, minimization or
occlusion detaches support; flying pets may recover with flight and other pets
fall through the existing drop/landing system. Moving-edge recovery must respect
body bounds and never silently teleport to another window.

Initial autonomous raven behavior chooses a reachable edge, takes off, travels,
brakes above it, extends feet, lands, folds wings and walks within its usable
span. Direct manipulation and suppression preempt it. Other pets gain placement,
support and edge walking in this slice; automatic flight remains capability-based.

## Assets and presentation

Create a readable, stylized black raven with restrained color accents. The 2D
package contains editable shape recipes and layered raster pieces; the 3D model
has editable Blender source, a small color atlas and a skinned runtime export.
Match broad proportions and desktop size for comparison. Keep individual
feathers decorative initially rather than simulating each feather.

Use bird-specific leg geometry and distinguish the visible ankle/hock from the
knee. Adapt the currently werewolf-specific 3D joint/scene binding to selected
anatomy and available roles. Do not make the 2D-only build require Quick3D.
Expanded wings need validated render bounds, click geometry and utility anchors.

## Delivery and verification

Implement in dependent slices: shared surfaces/body transitions; paired raven
assets and wings; hopping/powered flight; native window provider and perching;
resident comparison and acceptance. Use independent subagents with explicit
ownership for asset authoring and separable implementation/review work.

Core checks cover finite bounded integration, zero-force ballistic equivalence,
realized-wing lift, insufficient-lift descent, hop equivalence, touchdown without
tunneling, momentum handoff and ground gait regressions. Surface checks cover
moving anchors, narrow/covered edges, stale identity, removal, placement and
capability-based reachability. Existing pets must walk on synthetic window
surfaces without gaining flight or losing planted-contact behavior.

Compare 2D/3D using the same physical tuning, action/target timeline, world scale
and seed. Show rest, walk/reversal, hop, takeoff, flight, landing, perched walk,
window movement and lost support at actual size and enlarged. Verify native
window observation, transparency, clipping and scheduling separately from
software captures. Stop motion deadlines when supported and settled; flight
remains active while producing wing strokes.

Preserve frozen shared fixtures and evidence. New runs go in ignored results/
directories. Resource screening and battery measurements remain distinct;
neither asset budgets nor offscreen tests establish energy efficiency.

## Review

Scope reflects the user's requests. The major assumptions for review are a
stylized matched pair, planar desktop flight, force-driven wings, and macOS-first
visible window-top support. Full aerodynamics, other-window collision volumes,
free 3D depth travel, climbing and per-feather physics are deferred.
