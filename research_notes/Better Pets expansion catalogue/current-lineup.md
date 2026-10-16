# Better Pets current lineup and expansion constraints

## What characters and visual styles actually exist?

### Takeaway
The working source catalogue has five packages representing three characters: Mallow, Bramble/Werewolf and Corvid. Two of the three are available in both 2D and 3D; this is not a five-species lineup. Package existence and local implementation do not establish public release or customer availability.

### Cited Findings
- Mallow (`soft-creature`) is a flat cream, black-eyed, asymmetric long-eared creature with a curling tail and small feet. Its simplified art intentionally removes outlines/shading to review grounded IK. It is a 2D original procedural pixel character, not a conventional domestic rabbit despite its rabbit-like silhouette. Its manifest name is Mallow. — [Character overview](/Users/dizzler/Projects/Repos/better-pets/characters/README.md); [Manifest](/Users/dizzler/Projects/Repos/better-pets/characters/soft-creature/manifest.json); [Inspected preview](/Users/dizzler/Projects/Repos/better-pets/characters/soft-creature/preview.png)
- Bramble (`werewolf`) is the 2D charcoal/blue-gray bipedal werewolf: amber eyes, muzzle, chest ruff, claws and digitigrade legs. It has original layered raster art and articulated anatomy. — [Character overview](/Users/dizzler/Projects/Repos/better-pets/characters/README.md); [Manifest](/Users/dizzler/Projects/Repos/better-pets/characters/werewolf/manifest.json)
- Werewolf 3D is a distinct visual treatment of the wolf concept: low-poly brown body, pale muzzle/chest, oversized paws, pointed cheeks and ears, short digitigrade legs and a torn open red shirt. Dweller's Howl explicitly informed the broad brown-wolf/pale-muzzle/red-shirt direction; the original model does not contain downloaded, traced or extracted Dweller assets. Current export: 1,762 triangles, 22 deform joints, one material and one atlas, zero animation clips. — [Asset provenance and model details](/Users/dizzler/Projects/Repos/better-pets/characters/werewolf-3d/README.md); [Inspected three-quarter render](/Users/dizzler/Projects/Repos/better-pets/characters/werewolf-3d/three-quarter.png)
- Corvid (`raven`) is the original planar dark raven: horizontal body, forward heavy bill, shaggy throat, short bent legs, long wedge tail and broad layered folded wings. The manifest name is Corvid. October 16 changes reduce 2D size by 25% and revise anatomy and flight. — [Manifest](/Users/dizzler/Projects/Repos/better-pets/characters/raven/manifest.json); [Latest correction](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-16-raven-motion-and-geometry.md); [Inspected rest preview](/Users/dizzler/Projects/Repos/better-pets/characters/raven/preview.png)
- Corvid 3D (`raven-3d`) is the matched low-poly raven. Latest asset documentation reports 2,608 triangles, 20 joints, one nearest-sampled 256×256 atlas, one material and no animation clips; it includes weighted continuous wing covers and eleven broad tail feathers. Earlier raven result documents still report 2,008 triangles/18 joints and are superseded on counts. — [Latest raven asset README](/Users/dizzler/Projects/Repos/better-pets/characters/raven-3d/README.md); [Manifest](/Users/dizzler/Projects/Repos/better-pets/characters/raven-3d/manifest.json)
- The resident Character menu enumerates sibling directories with manifests, so all five source packages are discoverable when included in the configured catalogue. Mallow is documented as the default. — [Menu implementation](/Users/dizzler/Projects/Repos/better-pets/qt/resident/application.cpp:221); [Resident README](/Users/dizzler/Projects/Repos/better-pets/README.md:63)
- Current handoff confirms both 3D wolf and 2D/3D ravens run as native desktop residents. The wolf asset README's final statement that it remains preview-only is stale. The handoff is explicitly ordered latest-first and supersedes historical statements. — [Current handoff](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md)

### Inferences
- The strongest unfilled visual territory is familiar non-anthropomorphic domestic pets, round/soft animals, bright playful palettes, tiny minimalist companions and non-animal mascots. Existing wolf and raven imagery clusters around darker fantasy/naturalist appeal; Mallow provides softness but is deliberately a minimal movement-review design. This is an inference from the inspected assets, not user testing.
- Adding another dark winged creature alone would expand content but probably broaden aesthetic reach less than a cheerful domestic animal or small expressive mascot. A 2D/3D variant of one concept should not be marketed as the same degree of catalogue breadth as a new anatomy or temperament.

### Gaps
- No customer segmentation, interviews, conversion figures or reason-for-rejection evidence was found in the inspected local material. We cannot claim which species will convert users.
- No evidence establishes whether the five local packages are public offerings, paid products, or distributed beyond this development environment. Use “current local lineup,” not “released catalogue.”

## What can new pets inherit, and what requires engine work?

### Takeaway
Existing procedural biped and winged-biped capabilities make new birds and upright creatures the lowest-risk additions. A convincing cat, dog or capybara can be strategically valuable but needs quadruped locomotion work: the current grounded schema and loader require exactly two legs.

### Cited Findings
- Version-1 production packs contain data-only anatomy, named bones, layered raster attachments, semantic roles, interaction polygons and contacts; arbitrary executable QML/JavaScript/native code is excluded. Anatomy need not match the frozen benchmark rig. — [Pack format](/Users/dizzler/Projects/Repos/better-pets/docs/characters-v1.md); [Architecture](/Users/dizzler/Projects/Repos/better-pets/docs/architecture.md)
- The current grounded format requires `groundY` and exactly two distinct hip–knee–ankle chains. Loader enforcement reads `legs.size() == 2`. The architectural ambition to support quadrupeds and blobs is not evidence those controllers are delivered. — [Grounded format](/Users/dizzler/Projects/Repos/better-pets/docs/characters-v1.md:153); [Loader](/Users/dizzler/Projects/Repos/better-pets/qt/characters/loader.cpp:194); [Architecture](/Users/dizzler/Projects/Repos/better-pets/docs/architecture.md)
- Complete left/right shoulder–elbow–wrist wing semantics select shared powered flight; missing wings means wingless, partial wings reject. Hops use ballistic launch. Actual wing motion generates bounded lift and thrust; renderers consume the shared solved motion. — [Wing and airborne contract](/Users/dizzler/Projects/Repos/better-pets/docs/characters-v1.md); [Raven result](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-10-raven-flight-and-perching.md)
- Grounded pets share drag/pickup/drop, compliant landing, hard planted contacts, walking, cursor attention and in-place turning. Raven adds fly/land/window-perch controls. Wingless pets can stand and walk on observed window tops after manual placement. — [Current handoff](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md); [Shared support](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-10-raven-flight-and-perching.md)
- Current calm curiosity chooses surveys, independent ear twitches, tail swishes, occasional bounded exploration and supported spatial reach stretches, filtering unsupported anatomy. Profiles are built into code (Fallback, Mallow, Bramble, Werewolf), rather than declarative pack personalities. Held goals settle without animation ticks. — [Behavior result](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-10-curious-procedural-behavior.md); [Current profile enum](/Users/dizzler/Projects/Repos/better-pets/qt/behavior/curiosity.h); [Pack personality status](/Users/dizzler/Projects/Repos/better-pets/docs/characters-v1.md)
- Blinking is shared resident behavior with separate eye artwork/joints; 7–16 second quiet intervals and 190 ms closure sequence are implemented. — [Blink result](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-12-eye-blinks.md)
- Qt Quick/C++17 is the active product path; Godot is archived. Runtime/behavior/sessions have Qt-free boundaries, local JSON-RPC and petctl provide a language-neutral integration direction, and renderers must not choose behavior or own integration parsing. — [Architecture](/Users/dizzler/Projects/Repos/better-pets/docs/architecture.md); [Handoff](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md)
- Low application CPU/RSS and offscreen tests do not establish battery impact. Native general-window, multi-monitor and Spaces acceptance remain open despite controlled native flight/perch success. Whole-system battery evidence remains missing. — [Latest handoff](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md); [Toolkit evidence limits](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-08-comparison.md)

### Inferences
- Low-risk concepts: small owl, pigeon, penguin/duck-style biped, tiny robot with two feet, mushroom sprite, upright plush creature. Each needs original art/rig and tuning; birds with different anatomy must be validated rather than assuming the raven is a free reskin.
- Medium/new-controller concepts: slime, ghost, floating orb/robot. Art is inexpensive, but free hovering, squish locomotion and legless floor contact need explicit motion semantics; do not promise they already fit the two-leg solver.
- Higher-cost but potentially wider-reach concepts: domestic cat/dog, fox/red panda/capybara. Their appeal hypothesis comes from familiar companionship and varied silhouettes; credible four-foot stance/gait is new shared capability, not just extra sprites. A true quadruped controller could unlock a useful family of subsequent pets.
- Behavior differentiation should be specified alongside each concept: sleepy/slow versus curious/perchy versus mildly mischievous. Reusing the current identical generic activity mix with only recoloring is unlikely to realize the full value of a new catalogue. These personality choices require implementation/tuning and are not current pack features.
- Prefer quiet settled poses and brief procedural signatures to continual flapping, particle trails, fur/cloth simulation or perpetual bouncing. This follows the event-driven architecture; actual energy must still be measured.

### Gaps
- The inspected production behavior does not establish feeding, care meters, petting affection, toys, fetch, sound, multiplayer pet interaction or long-term progression. Treat these as proposals requiring design and development.
- No measured per-character 2D-versus-3D power comparison is available. Do not label new 3D pets battery-safe based on triangle counts.

## What is known about user preferences and workspace state?

### Takeaway
The user has explicitly prioritized smooth procedural motion, authored art, modularity, arbitrary application integration and low battery impact. Species/audience expansion choices remain hypotheses to validate, rather than established customer demand.

### Cited Findings
- User preferences recorded in the active handoff: lightweight wandering desktop pets, eventual object interaction, smooth procedural animation/authored artwork, modular components, stable interfaces for arbitrary applications, battery life as a primary constraint. — [Handoff preference record](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md:419)
- Recent accepted work emphasizes fixed floor contacts, visibly grounded anatomy, comfortable turns, correct bird geometry, cursor curiosity and occasional blinks. This supports physical credibility and subtle life as known project priorities. — [Latest handoff](/Users/dizzler/Projects/Repos/better-pets/docs/handoff.md)
- Several local results state this workspace has no Git repository. Read-only `rtk git status --short` on 2026-10-16 returned “fatal: not a git repository (or any of the parent directories): .git”. Consequently no reliable Git-based inventory of unrelated modifications is possible. — [Existing record confirming no Git repository](/Users/dizzler/Projects/Repos/better-pets/docs/results/2026-10-10-raven-flight-and-perching.md)

### Inferences
- “People who dislike the current offerings” could mean dislike of species, art style, desktop clutter or lack of usefulness. A catalogue should deliberately offer different silhouette, palette, activity and footprint options; more species alone does not test all four explanations.
- A useful first-wave selection should combine immediate shared-controller reuse with one deliberate capability expansion. A bright small bird/mascot can ship with lower motion risk while a quadruped domestic pet tests broader demand.

### Gaps
- The user has not specified desired catalogue size, exact visual direction, release targets or budget in the current request. Do not interpret research as authorization to build every proposed pet.
- This researcher edited only this notes file. No code, assets, tests or product configuration were modified; unrelated pre-existing edits cannot be distinguished without version-control metadata.
