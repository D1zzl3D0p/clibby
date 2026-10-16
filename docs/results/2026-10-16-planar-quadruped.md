# Planar quadruped implementation

## Outcome

The first genuine four-contact planar pet is implemented: **Marmalade**, an
original ginger tabby. Its atlas contains independently posed pieces rather
than animation frames. Four named two-link chains walk procedurally, preserve
world-planted soles, turn with the body, participate in pickup/drop/recovery and
settle through the resident's existing timer. The resident catalogue discovers
the package through its existing sibling-directory selection path.

This implements the first planar slice of the
[quadruped design](../superpowers/specs/2026-10-16-quadruped-foundation-design.md)
and [execution plan](../superpowers/plans/2026-10-16-planar-quadruped.md).
The broader [18-pet architecture](../superpowers/specs/2026-10-16-catalogue-architecture-design.md)
remains the expansion roadmap, not a claim that all 18 characters now exist.

## Architecture and compatibility

`LimbArray<T>` stores at most four values without per-frame allocation and exposes
only two or four active elements. Copies and state snapshots retain the count;
unused capacity never becomes a phantom foot. Existing two-element fixture
initializers and spatial support setters remain compatible. Wings, ears and
wrists retain their independent paired anatomy.

The version-1 loader accepts explicit `quadrupedWalk` with exactly four unique
roles: foreLeft, foreRight, hindLeft, hindRight. It canonicalizes shuffled JSON
order, rejects malformed/partial/shared or ancestor-overlapping chains, and keeps
the current limits and two-link reach/fold checks. Omitted gait retains existing
biped semantics; unknown gait/count/roles reject before character replacement.
Marmalade deliberately shuffles its authored leg order to exercise this path.

The walk cycle is hindLeft → foreLeft → hindRight → foreRight. Ordinary gait
admits one swinging paw only while the other three have confirmed support.
Resting realignment follows that cycle while skipping comfortable contacts.
Body travel still follows host-realized movement; IK extends to preserve stance
pins rather than sliding feet. The existing count-aware support equations are
used once. Neutral body lowering is averaged over active limbs.

Fore/hind proximal offsets turn about the authored body anchor, while world soles
stay pinned until toe-off. Review exposed a valid-pack case where placement
origin and body anchor differed: homes originally reflected about the wrong
point, displacing feet by 36–40 units. A failing shifted-origin regression and
the `homeAnchorX_` correction now keep hip and home reflection consistent,
including release contact installation. The reviewer independently reproduced
the corrected positions and found no further issues.

Resident arrival checks every active swing, and the host receives the complete
sole extent with its existing radius margin. The new integration test first
failed because the old two-foot aggregation accepted a window supporting only
the forepaws. It now rejects that window, accepts a full-width support, carries
all four paws with a moving surface and releases the pet when support disappears.

## Original artwork and use

The editable source is `tools/author_marmalade.py`. The package includes its
atlas, manifest, independently composed source preview, attribution and MIT
license. It has 19 bones and 18 attachments, separate eye artwork, a visual
facing branch outside IK ancestry and four genuine contact chains. No external
art, extracted sprites or authored animation clips were used.

After restarting the rebuilt resident, select **Marmalade** from the character
menu. For development preview from the project root:

```sh
rtk proxy build/qt/character_preview --character characters/marmalade
```

Regenerate/check source with `.venv/bin/python tools/author_marmalade.py`
and `--check`, through the usual `rtk proxy` prefix.

## Verification

| Check | Final result |
| --- | --- |
| Combined Release build | Passed, no compiler warning/error diagnostics |
| Combined Qt CTest | **42 passed, four skipped, zero failures** (46 total) |
| 2D-only Release build | Passed, no compiler warning/error diagnostics |
| 2D-only Qt CTest | **31 passed, three skipped, zero failures** (34 total) |
| Full Python discovery | **55 passed** |
| Deterministic Marmalade source | Passed directly and as CTest |
| Frozen golden poses | Passed in both configurations |
| Independent whole-change review | One Important finding fixed and re-reviewed; none outstanding |

The tests exercise actual loader normalization/rejection, active-state isolation,
canonical gait, unequal limbs, every leg eventually stepping, quantized/blocked
movement, actual rendered sole endpoints against world pins, reversal, noncentral
placement origin, stable pose reads, eventual quiescence, contact gating,
pickup/regrab, bounded landing targets, floor changes, resident suppression,
selection and whole-body window support. Existing biped/raven and spatial checks
remain in the complete suites.

Red/green records, final build/CTest logs and captures are in the ignored local
directory `results/planar-quadruped-2026-10-16/`. Preview captures cover rest,
walking and reversal. A temporary capture harness using the actual loader,
controller and raster renderer covers held, falling, first touchdown and settled
recovery; these are software poses, not native desktop screenshots. All those
images were visually inspected. The temporary capture compiler initially lacked
the project's Qt/compiler flags; using the configured compiler/flags resolved
that tooling failure without product changes.

Combined skips remain `resident_ipc`, `resident_spaces`, `resident_ipc_cli` and
`character3d_gpu_render`; the 2D-only suite skips the first three. No failure was
hidden or counted as a passing native check.

## Git checkpoints and limits

Git is initialized locally. `main` retains baseline **2dbd7c1**, capturing the
entire pre-implementation project, including the earlier support-math foundation.
New work is on `feat/quadruped-planar` and receives a separate implementation
commit. Builds, dependencies, caches and local results remain ignored. Nothing
was pushed or published. After committing and with a clean working tree,
switching to main restores the baseline source; rebuild to restore its executable.

The work stayed in the existing checkout on a separate branch to reuse the
configured SDK/builds. That choice preserves Git rollback but does not provide
a second physical checkout. The user explicitly requested proceeding with
implementation, so no additional design approval round was inserted.

The implementation is **2D only**. A spatial quadruped adapter, 3D cat art,
seated/flop transitions, trot/run, independent spine physics and quadruped
hop/flight remain outside this slice. Unsupported hop/flight requests return
false. Native gait/turn feel, leg occlusion/cross-over during reversal, manual
throw feel, everyday windows, monitor/Spaces behavior and energy still need
separate acceptance. Software passes and CPU/RSS do not prove battery impact or
desktop parity.
