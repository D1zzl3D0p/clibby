# Grounded characters implementation roadmap

Status: grounded 2D runtime/resident and original Blender model plus optional
3D preview implemented. Software/contact/presentation checks pass. Native GPU
pixels, quiet rendering and matched resource comparison remain pending; resident
3D integration is deferred. See the [results record](../../results/2026-10-08-grounded-characters.md).

Designs: [grounded locomotion](../specs/2026-10-08-grounded-locomotion-design.md)
and [Blender werewolf](../specs/2026-10-08-blender-werewolf-design.md).
Keep implementation in these two ordered projects; 3D does not block the 2D build.

## Project 1: simplified creature and grounded locomotion

1. **Tracking and IK primitives.** Add Qt-free `second_order.h/.cpp` and
   `ik.h/.cpp` under `qt/characters/`, with focused CTest coverage. Check constant
   input settling, tunable step responses, bounded work under lag, finite input
   handling, segment lengths, bend continuity and reachable/unreachable endpoints.
   Avoid a full physics dependency. Wire focused targets in `qt/CMakeLists.txt`.

2. **Anatomy and simplified Mallow.** Extend `character.h`, `loader.cpp`, package
   validation tests and `docs/characters-v1.md` with the optional supported
   locomotion metadata described in the spec. Modify `tools/author_characters.py`
   and `characters/soft-creature/` to use flat body/head/ears/tail/feet attachments
   and genuine contiguous leg chains. Verify both current version-1 packages
   still load, invalid chains/scales fail transactionally, and authoring `--check`
   reproduces assets. Keep `shared/` and benchmark source unchanged.

3. **Stateful ground controller.** Add `grounded.h/.cpp` for bounded navigation,
   planted anchors, reach-triggered step selection, generated swing arcs, IK and
   expressive body state. Specify proposal/realization/pose sequencing in its
   public header. Tests cover no stance drift, arrival, reversal, final-step
   completion, clipped targets, impulses, reset/rebase and exact quiet settling.
   Keep old action controllers usable; do not synthesize walking from elapsed time.

4. **Interactive motion preview.** Modify `qt/preview/main.cpp`; extract a preview
   world/input adapter if it would otherwise grow further. Add ground/target UI,
   contact debug overlays, tuning and scripted movement scenarios. Extend
   `qt/preview/test_preview.py` for target/reversal state exports and effective
   rendering metadata. Keep existing action/style capture options for legacy
   packages. Produce motion sequences under fresh ignored `results/` directories
   and review actual-size slow walk, stop/reversal and impulses. Default Mallow to
   CPU composition and nearest sampling; palette dither off.

5. **Resident realization and deadlines.** Modify `desktop_host.h/.cpp` with a
   horizontal ground-motion API returning realized root position and blocked
   state. Implement fractional accumulation and explicit display-floor mapping.
   Modify `application.h/.cpp` to choose the grounded controller, add local menu
   target actions, and join locomotion into the existing timer. Extend
   `character_tests.cpp`/`host_tests.cpp` for clipping, integer presentation,
   pause/hide/sleep, drag/release, display rebase, switching and deadline removal.
   Compose session reactions into expressive targets before IK, preserving foot
   anchors and advancing motion once when deadlines overlap; test those cases.
   Explicitly select CPU composition/nearest sampling for grounded Mallow in
   resident creation and character switching, matching its preview default.
   Do not add wandering, global pointer polling or change session-v1 semantics.

6. **Review and evidence.** Run `tools/build_qt.py`, Python regression tests,
   preserved 44-pose verification and asset checks. Review motion recordings and
   remaining limitations independently; fix contact or scheduling defects before
   increasing art detail. Update handoff/architecture and write a new results
   record that distinguishes software, visual and native evidence. No native or
   battery acceptance claim from offscreen tests.

## Project 2: original Blender werewolf and optional 3D preview

1. **Backend feasibility.** Find a compatible Qt Quick 3D/ShaderTools distribution
   route and verify distribution fit. Keep `qt/CMakeLists.txt` optional for 3D.
   Demonstrate one deform joint, transparency, a small render texture with nearest
   presentation, and render quiescence. Record actual results before promising a
   package loader or resident renderer. No silent substitution with frame clips.
   A recorded backend blocker does not block the Blender model/rig delivery in
   step 2; live-renderer steps remain pending until the blocker is resolved.

2. **Blender model and rig.** Add `tools/author_werewolf.py` and a separate
   `characters/werewolf-3d/` asset source/package. Produce `.blend`, `.glb`, atlas,
   provenance and review renders. Use Howl's broad visual direction with original
   proportions, face and clothing. Verify <=4,000 triangles, <=32 deform joints,
   <=4 weights per vertex, a 256x256 atlas and <=4 materials after export.

3. **Live skeletal renderer.** Add a focused optional `qt/characters3d/` module
   and separate `qt/preview3d/` entry point. Resolve joint mapping from trusted
   converted assets, pose conventions and low-resolution GPU composition. The
   resident must not launch Blender. Prove articulated deformation in Qt before
   importing the complete model. Preserve nearest texture/output sampling.

4. **Ground-motion adapter.** Feed shared navigation/contact intent into bounded
   3D leg solving and body/head/arm followers. Handle facing through supported
   turns. Validate contacts, chain reach, skinning extremes and stopping with
   recorded motion at desktop size. The 2D affine pose remains a 2D type.

5. **Resource comparison and resident decision.** Measure fresh 2D/3D settled,
   moving and hidden scenarios with identical target paths and no user input
   during timing. Check native transparency/pixel scaling and GPU/WindowServer
   impact where available. Integrate resident 3D selection only after functional
   and quiet-rendering checks pass; document remaining native/energy work.

## Review focus

- World/local coordinates and integer host placement must not cause foot skating.
- Reversal and final steps must finish without a perpetual active deadline.
- Render snapping must not corrupt physical anchors or substitute for stable IK.
- Pause/display/package changes must not replay time or retain stale contacts.
- A valid Blender GLB is not evidence of addressable joints in the Qt renderer.
- The original 2D path, session API and preserved benchmark remain independently
  buildable and verifiable when the optional 3D modules are unavailable.

All execution commands retain the `rtk` prefix; Python uses `.venv/bin/python`.
This workspace has no Git repository; use source snapshots/evidence rather than
claiming commits. During execution, delegate independent art/core/review tasks
with explicit file ownership, as requested in AGENTS.md.
