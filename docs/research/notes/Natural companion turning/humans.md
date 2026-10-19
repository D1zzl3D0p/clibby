# Human and biped turning

## How do healthy people redirect walking by 90° or 180°?

### Takeaway
Healthy brisk turns combine braking, rotation, and redirected foot placement while retaining walking rhythm. [Hase and Stein, 1999](https://journals.physiology.org/doi/10.1152/jn.1999.81.6.2914)

### Cited Findings
- During cued rapid U-turns, right turns with right foot forward commonly spun around that foot. Left turns instead transferred onto an externally rotated left leg before stepping the right leg into the new direction. Step turns offered wider support; some participants always preferred them. Rotation began during braking and was essentially completed within a step cycle. [Hase and Stein, original EMG/force study](https://journals.physiology.org/doi/10.1152/jn.1999.81.6.2914)
- A separate 90° study of ten healthy young participants analyzed approach, pivot, and departure steps in side-step and cross-step turns. All three steps contributed through increased lower-limb rotation and rotational moments. This is a three-step analysis window, not proof that all 90° turns require exactly three new placements. [Kawabata et al., 2025, abstract](https://www.sciencedirect.com/science/article/pii/S1050641124001032), [DOI](https://doi.org/10.1016/j.jelekin.2024.102959)
- In fifteen healthy young adults performing 45° turns, spin turns reduced lateral margin of stability and stride width during the turn; step-turn reductions occurred during departure. Spin turns were somewhat faster, step turns somewhat slower, than straight walking in that protocol. The authors identify spin as a potential efficient rapid redirection strategy, while noting instability differs by phase. These are 45° findings, not quantitative 180° norms. [He et al., 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6151057/)

### Inferences
- **Engineering inference:** Turn choice should consider the support leg and available space. A broad side step is a plausible comfortable default; a compact inside-foot pivot plus a sweeping/crossing recovery step provides a brisk option. Do not randomly select either without regard to which leg currently supports the character.
- **Engineering inference:** Preserve the original fast-walk cadence and forward locomotion quality as the baseline. Blend a short braking/redirection episode into that cadence, then resume it. Research supports this architecture; it does not verify any specific existing `originalfastwalk` implementation.
- **Engineering inference:** Use an event-level turn plan with a finite completion time and named support transfers. Each placement should substantially advance the remaining turn. Constantly recalculating tiny corrective footholds can create an artificial chase loop.
- **Engineering inference:** Distinguish a compact U-turn from walking around a smooth 180° arc. A turning radius changes how many ordinary walking steps are available; one count cannot describe both maneuvers.

### Gaps
- The 90° source above was available as the publisher-indexed abstract; its full text returned an access error. It cannot establish exact toe/heel timing or per-foot yaw ranges here.
- No primary source reviewed justifies a universal 0.55-radian (~31.5°) maximum for either body yaw change or foot yaw change per placement. Those are also different quantities. The numerical conversion is arithmetic, not a biological threshold.

## What are realistic standing-turn counts and body-segment sequencing?

### Takeaway
A small number of placements is compatible with a normal brisk half-turn. Head, trunk, pelvis, and feet begin at different times, but their movements overlap; a healthy turn is not a sequence of fully completed segment rotations. [Standing-turn experiment, 2023](https://www.nature.com/articles/s41598-023-49201-2), [head-control experiment, 2006](https://pubmed.ncbi.nlm.nih.gov/16506002/)

### Cited Findings
- Fifty healthy young adults performed standing 180° turns with instructed durations 1.5, 2, and 3 seconds. Table 2 reports median total step counts of 2.5 for both sexes at fast speed, 3.0/2.5 at moderate speed, and 3.0/3.0 at slow speed. Fast head/thorax/pelvis onset medians were 0.328/0.366/0.430 seconds for males and 0.323/0.340/0.393 for females. Leading-foot onsets were 0.459/0.514 seconds. These are population medians of experimental measures, not an integer prescription. [Article](https://www.nature.com/articles/s41598-023-49201-2), [verified table](https://www.nature.com/articles/s41598-023-49201-2/tables/2)
- In twenty healthy subjects making step turns, head rotation initially advanced relative to pelvis, the middle interval predominantly coupled head and pelvis, and late head-in-space velocity fell below pelvis velocity as the head stabilized. Peak head speed exceeded pelvis speed by about 54°/s toward visible targets versus 25°/s toward remembered targets. [Solomon et al., 2006, original abstract](https://pubmed.ncbi.nlm.nih.gov/16506002/)
- Thigpen et al. compared twenty healthy young adults (20–30), fifteen older adults without turning difficulty, and fifteen with difficulty during a walking-turn task. Only pivot-type turning occurred in the young group; the difficulty group almost never used it and generally required more time and steps. This is evidence separating healthy youthful behavior from turning difficulty, not a universal diagnostic rule. [Thigpen et al., 2000](https://pubmed.ncbi.nlm.nih.gov/11087304/)
- In healthy women performing a standing 180° turn after picking up a bowl, the older group used preparatory stepping more frequently and rotated the pelvis more slowly than young controls. The task and age distinction matter when selecting animation references. [Meinhart-Shibata et al., 2005](https://pubmed.ncbi.nlm.nih.gov/16139746/)

### Inferences
- **Engineering inference:** Start a brisk standing half-turn prototype around two to three visible placements, then adjust by character proportions and visual review. This is a plausible animation target grounded in the standing study, not a demand that every task or character use exactly this count.
- **Engineering inference:** Let gaze/head lead slightly, trunk and pelvis follow in overlapping curves, and settle the head before the pelvis finishes. Avoid both completely rigid rotation and an exaggerated head turn held while the legs execute many shuffles.
- **Engineering inference:** The fast standing-onset differences suggest a head lead of tens to roughly one hundred milliseconds relative to trunk/pelvis, with stepping beginning shortly afterward. Use relative phase, not the absolute cue-response latency, as the portable animation idea.
- **Engineering inference:** Keep hesitant multi-step reorientation available as a deliberate cautious behavior. Do not accidentally make it the default youthful energetic locomotion style.

### Gaps
- Published step counts depend on onset/offset definitions, task, averaging across trials, age, and turn radius. A 2.5 median does not mean an individual performed half a foot placement.
- The standing table labels an angular “step size” and temporal “step duration”; those measures should not be transplanted directly into renderer constants without checking the referenced extraction method. In particular, its angular step size is not established here as loaded support-foot yaw.
- Healthy older adults are not synonymous with pathological subjects, and the difficulty group above is not a model of all neurological disorders. No diagnosis-specific quantitative claims are needed for the product.

## Can a supporting foot turn, and what should heel/toe contact look like?

### Takeaway
A forefoot can support body rotation during turning. [Hase and Stein, 1999](https://journals.physiology.org/doi/abs/10.1152/jn.1999.81.6.2914)

### Cited Findings
- In Hase and Stein's spin example, support persisted on the ball of the axial foot while the body rotated; the other foot then landed and freed the axial leg to step away. In their step example, load transferred quickly, both feet partly reoriented, and the final placement landed on the heel into the new walking direction. [Original force/kinematic description](https://journals.physiology.org/doi/abs/10.1152/jn.1999.81.6.2914)

### Inferences
- **Engineering inference:** Separate contact position, contact patch, and foot yaw. A planted forefoot pivot can keep the ball of the foot anchored while the heel sweeps; a lifted foot may reorient during swing; a newly loaded foot may then roll heel-to-toe. These are different contact events and need different constraints.
- **Engineering inference:** A visual support pivot should show a small heel lift or changing contact patch, weight over the pivot side, and recovery-step clearance. Rotating a completely flat sole about its center reads as sliding; yawing an entire body while both full soles stay rigidly fixed reads as torsional distortion.
- **Engineering inference:** Allocate turn angle across anticipatory foot placement, body-over-support rotation, possible support-foot pivot, and redirected swing placement. A 180° body turn does not imply one ankle must twist 180° against a fixed sole.
- **Engineering inference:** Use a bounded pivot phase with an explicit exit into the recovery step. Do not loosen support-foot constraints indefinitely or rotate both soles continuously just to make a turn finish.
- **Engineering inference:** The current ~0.55-radian incremental scheme should not be defended as “human realism” without measured data. A larger reoriented swing placement plus a brief forefoot pivot may reduce turn placements substantially while preserving support logic.

### Gaps
- The reviewed studies do not supply a universal maximum safe or typical stance-foot yaw, pivot speed, pivot pressure, or exact heel lift height for all 90°/180° turns. Footwear, friction, initial posture, and turn strategy need separate treatment.
- A ball-of-foot support interval proves forefoot loading and an axial turning strategy; it does not alone establish a complete world-space foot-yaw trajectory or quantify how much rotation occurs through sole-ground slip versus proximal joints.
- No product code was changed or executed for these notes; all implementation implications are proposals for a later measured prototype.
