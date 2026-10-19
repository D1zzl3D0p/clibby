# Give more people their kind of companion

**Build Rivet, Paper Heron and Acorn Custodian as the first practical expansion trio; test a domestic cat and dog alongside them before choosing the next major animation investment.** Together, the trio introduces warm mechanical company, restrained paper craft and cheerful woodland fantasy without first requiring four-legged locomotion. A cat or dog offers the clearest familiar-animal contrast with the current lineup, but neither is an inexpensive reskin. The larger catalogue below contains 18 original directions, deliberately varying material, temperament and visual footprint as well as species. Dweller provides useful inspiration in how it gives each character a small identity and activity. Its breadth also makes “we have a robot/dragon/plant too” a weak differentiation strategy. These are product hypotheses and build recommendations, not proven demand or conversion forecasts.

## Broaden the reasons to choose a pet

The **current local development lineup contains three character identities across five packages**: Mallow, a soft cream, long-eared creature; Bramble/Werewolf in 2D and 3D; and Corvid in 2D and 3D. Mallow supplies gentle simplicity, while the wolf and raven provide darker animal and fantasy appeal. This describes source packages and local implementation, not a verified publicly shipped catalogue. A second rendering treatment adds choice, but it does not broaden the cast as much as a new form or temperament. ([Character overview](<../../../characters/README.md>), [current handoff](<../../handoff.md>))

The strongest expansion hypothesis is therefore **more kinds of attachment**: a recognisable household animal, an earnest machine, a crafted object, or quiet nature. Somebody who dislikes wolves and ravens might love a sleepy dog. Somebody who dislikes conventional pets altogether might enjoy a folded-paper bird. Somebody who finds the whole category distracting might need a smaller, stiller companion rather than a different species. We have no customer research establishing which explanation matters most.

Dweller's live homepage, inspected on **16 October 2026**, displayed **164 cast entries across 14 collections, plus its resident wizard Wick**. Those are website listing counts, not independently verified downloadable assets. Its range already spans fantasy, history, folklore, everyday occupations, space, robots, marine animals, food and familiar animals. Its hero and inspected Timber Moose cards use detailed, shaded pixel art; not every listed character received visual inspection. ([Dweller cast](https://dweller.pet/#cast))

The useful lesson is its character formula: a readable silhouette, a memorable disposition and one small activity. Dweller advertises walking, waving, hopping and an individual “job”; its actual desktop app was not installed or tested for this research. Better Pets can explore that formula through original materials and procedural motion. Use original silhouettes, art, names and actions; do not extract sprites, trace characters or reproduce distinctive costumes. ([Dweller](https://dweller.pet/), [terms](https://dweller.pet/#terms))

## Eighteen concepts create genuinely different choices

Every entry below is a proposal. The audience column identifies **who to include in concept testing**, rather than a demonstrated market segment. Effort categories describe the current architecture boundary, not a time estimate:

**A — existing two-leg route:** original art, rigging and motion validation, with proposed personality actions still requiring development. **B — new quadruped capability:** four-foot support and gait work before convincing walking pets. **C — new body or locomotion contract:** legless, crawling, floating or multi-legged motion; simple artwork does not make these cheap.

The grounded loader currently requires **exactly two legs**. Existing winged-biped support helps birds, but does not automatically solve every bird anatomy or animation. Personalities are not yet freely configurable pack data, so each signature below needs explicit scoping. ([Character contract](<../../characters-v1.md>), [loader](<../../../qt/characters/loader.cpp>))

### Familiar company gives newcomers an obvious entry point

| Pet and form | Visual/material direction | Personality and proposed signature | Audience hypothesis | Effort |
|---|---|---|---|---|
| **Marmalade — domestic cat** | Rounded ginger cat; broad graphic patches, soft matte surfaces | Self-possessed; slow blink, shoulder stretch, tail-tip inspection | People wanting a recognisable household companion | B |
| **Pip — small floppy-eared dog** | Biscuit-and-cream body, large ears, warm low-poly volumes | Affectionate but relaxed; head tilt, brief tail wag, comfortable flop | Dog lovers and people drawn to cheerful warmth | B |
| **Clover — lop rabbit** | Compact cream rabbit, oversized folded ears, minimal shading | Cautious curiosity; nose investigation, ear settle, short hop | Gentle-animal fans who want a real species rather than Mallow's fantasy form | B |
| **Biscuit — capybara** | Wide caramel silhouette, blunt nose, tiny ears | Unhurried; long sit, deliberate look, rare stretch | People who enjoy calm, mildly comic animals | B |
| **Puddle — upright duck** | Butter-yellow and white, simple rounded shapes, orange feet | Cheerfully nosy; careful waddle, head bob, satisfied wing tuck | Bright, approachable animal appeal | A; validate proportions |

### Crafted companions offer an alternative to conventional cuteness

| Pet and form | Visual/material direction | Personality and proposed signature | Audience hypothesis | Effort |
|---|---|---|---|---|
| **Rivet — upright service robot** | Rounded terracotta shell, cream face panel, two sturdy feet | Earnest and slightly clumsy; straightens antenna, gives a tiny salute | Mechanical-character fans and people wanting warmth without fur | A |
| **Paper Heron — folded-paper bird** | Ivory and one muted accent; clear triangular planes | Precise and contemplative; careful step, restrained bow, tucked rest | Craft, stationery and restrained-design interests | A; no paper simulation |
| **Pocket Observatory — walking telescope** | Two-footed brass-and-navy instrument, large circular lens | Patient stargazer; frames its own little star card | Science interests and playful desk-object appeal | A |
| **Night Librarian — walking book** | Small clothbound volume, feet and ribbon bookmark | Reserved and gently funny; opens one greeting spread, curls ribbon | Readers and people wanting a bookish companion | A; hinged page groups |
| **Pebblekin — balancing stones** | Three irregular slate forms; face optional | Stoic; tiny upper-stone turn, brief wobble and recovery | People preferring animated sculpture to a pet | C; stationary form still needs a contract |
| **Margin — ink organism** | Compact monochrome brush shape, negative-space eyes | Dry curiosity; briefly becomes a comma or exclamation pose | Graphic-art and minimal-desktop preferences | C; bounded morph poses |

### Nature and gentle fantasy add warmth without another dark mascot

| Pet and form | Visual/material direction | Personality and proposed signature | Audience hypothesis | Effort |
|---|---|---|---|---|
| **Acorn Custodian — woodland sprite** | Amber seed body, cap, twig arms, two feet, leaf accent | Quietly industrious; tidies one owned leaf and admires the result | Cozy woodland and cheerful fantasy interests | A |
| **Mossback — tortoise** | Broad stone-coloured shell, moss accent, solid matte forms | Patient observer; extends neck, considers cursor, retreats | Nature lovers and slow-companion preferences | B |
| **Sprig — two-footed succulent pot** | Pale ceramic pot, chunky green leaves, short articulated legs | Content gardener; slowly presents one leaf toward a target | Plant enthusiasts seeking ambient company | A; rigid leaves initially |
| **Hearthling — upright ember salamander** | Charcoal body, amber belly, small geometric plates | Drowsy warmth; tail curl, pleased stretch, brief colour accent | Cozy fantasy without imposing armour or menace | A; explicitly bipedal |
| **Crumb Golem — biscuit-stone creature** | Sandy chunks, sturdy feet, mismatched little arms | Stubborn optimist; balances one loose crumb, tries again | Whimsical food/object characters and gentle humour | A |
| **Velvet — observational moth** | Restrained cream-and-brown wing pattern, fine antennae | Shy; antenna grooming, slow folded-wing reveal | Insect enthusiasts and elegant naturalist tastes | C; six legs and flight need work |
| **Lantern Snail — shell-bearing crawler** | Rounded muted shell with a small lantern-door motif | Deliberate explorer; peeks out, opens shell door to greet | Slow, unusual animals and gentle fantasy | C; crawling/contact model |

These concepts compete on **material and behaviour**, not claims that their species are missing elsewhere. Dweller already includes robots, plants, food, insects and snails. Adjacent products also show different companion propositions: Viridi presents plants as quiet company, while Rusty's Retirement presents robots doing small jobs and explicitly offers slower Focus Mode. Those are observed product descriptions, not evidence that the concepts above improve focus or will attract buyers. ([Dweller](https://dweller.pet/), [Viridi](https://store.steampowered.com/app/375950/Viridi/), [Rusty's Retirement](https://store.steampowered.com/app/2666510/Rustys_Retirement/))

## Six briefs separate the first builds from the bigger opportunity

**1. Rivet — first build.** Make a warm, compact machine with a broad stable stance and an expressive face panel. Its emotional hook is earnestness: it pauses, checks its own antenna and straightens up with modest pride. Start with grounded walking, a head tilt, eye changes and one short greeting; the salute is a proposed additional action. Avoid real desktop maintenance, screen scanning or file manipulation. Rivet offers a clear change from the existing animal cast while retaining the two-leg route.

**2. Paper Heron — second build.** Aim for an attractive folded object even when completely still. Give it a long angular neck, visible paper planes and a compact tucked pose. One deliberate step and a small bow should carry the character; decorative unfolding is optional later work. Test thin-leg readability and planted contacts early. Its purpose is to test whether material restraint attracts people who reject furry or heavily characterised mascots.

**3. Acorn Custodian — third build.** Use a bright amber body and a broad seed cap, with one leaf as its personal possession. The hook is comfortable, undemanding industry. Begin with a proud stance, an inquisitive lean and cap adjustment; add the leaf-tidying vignette as a bounded character action. The leaf belongs to its own animation, rather than becoming a desktop-cleaning system. This gives the first trio a cheerful fantasy anchor and avoids making the entire expansion austere.

**4. Marmalade — strongest familiar-animal research candidate.** Prioritise recognisable cat posture over costumes: rounded seated silhouette, shoulder line, expressive ears and a long settling tail. A still portrait and short seated motion study can test appeal before walking development. The eventual full pet requires genuine quadruped support; a seated preview must not be presented as a completed walking character. This is the clearest test of whether the current cast lacks the animal somebody already loves.

**5. Pip — alternative flagship for quadruped investment.** Make the dog friendly in shape and restrained in tempo. Its resting pose should communicate companionship without continual demands; save the lively tail response for a brief interaction. Compare Pip directly with Marmalade before selecting the first quadruped. Do not commit to fetch, toys, affection meters or simulated care as part of the initial character. Neither species has demonstrated higher demand in the available evidence.

**6. Mossback — deliberate nature option.** The shell should read at small size, with a short neck extension doing most of the expressive work. Avoid detailed moss simulation and constant walking. Mossback tests a less conventional animal attachment and a much slower tempo. It belongs after, or alongside, a justified quadruped capability investment; despite its simple shell, four-foot contact remains real development work.

**The first-build ranking and audience-expansion ranking serve different decisions.** Rivet, Paper Heron and Acorn Custodian are the most practical coherent trio now. Marmalade and Pip deserve immediate concept testing because familiarity could broaden appeal substantially; committing to their locomotion should follow evidence of preference. Mossback is a useful alternative if respondents favour quiet nature over domestic animals.

## Test what people would actually keep visible

Show original small-size silhouettes and short resting/interaction previews to people who currently decline Mallow, Bramble and Corvid. Include the existing cast as a baseline and allow “none.” Ask which companion they would leave visible during ordinary work, what they reject about the others, and whether stillness, material, species or personality drove the choice. Rotate presentation order and avoid treating a “cute” rating as intent to use.

After prototypes exist, compare activation and continued use over ordinary sessions, including voluntary hiding or disabling. Keep size and movement opportunities comparable so a more active preview does not win solely by attracting attention. A small preference study can choose the next prototype; it cannot establish a market-wide conversion uplift.

Give every candidate one readable silhouette, one satisfying resting pose and one memorable short action. Proposed quiet defaults should include long settled rests and no neglect penalties; motion controls and catalogue previews are product work to scope, not assumed current features. Do not infer battery savings from stillness, low geometry or application CPU figures: whole-system energy needs separate measurement. Wider desktop behaviour also needs its own acceptance checks. ([Architecture](<../../architecture.md>), [evidence limits](<../../results/2026-10-08-comparison.md>))

The useful catalogue is one where each addition earns a different reason to stay. Build the feasible trio, use familiar animals to test the largest unanswered appeal question, and let those results determine whether the next investment is more character art or a new kind of body.
