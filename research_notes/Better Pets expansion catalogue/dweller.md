# Dweller.pet inspiration audit

## What does Dweller currently offer?

### Takeaway
Directly inspected the live homepage on 2026-10-16 in the Codex in-app browser. Dweller already has a very broad catalogue: the displayed counts total **164 cast entries across 14 collections, plus resident Wick**; this is a count of homepage entries, not a verified count of downloadable or unique app assets. [Dweller](https://dweller.pet/)

### Cited Findings
- Dweller markets small Mac desktop companions. The homepage offers free version 1.0.8, 6.8 MB, macOS 13 or later, with signing/notarization claimed by the vendor. [Dweller](https://dweller.pet/)
- The collections and displayed counts are Timber Moose (10), The Watch (15), Edo (14), North Sea (12), Old Empires (12), High Seas (13), Bestiary (15), Night Shift (8), Far Reaches (10), Hollow (10), Deep Blue (10), Oddballs (17), Olympus (11), and Keepsakes (7). [Cast](https://dweller.pet/#cast)
- The resident is Wick, an elderly wizard described as the first pet to appear when the app opens. Visual observation: detailed, shaded pixel-art presentation, a blue pointed hat, white beard, staff and layered robes; warm brown/black site with gold pixel-lettering. [Dweller](https://dweller.pet/)
- Timber Moose includes anthropomorphic Dumpling, Maki, Pickle; Butterfly, Snail, Narwhal, Warthog, Bearded Dragon, Walrus, Whale Shark. Visual inspection showed fairly detailed shaded sprites, food faces, a headband-wearing snail and a blue butterfly. It is not solely a medieval humanoid collection. [Timber Moose](https://dweller.pet/#col-timber-moose)
- The Watch includes Warden, Paladin, Jester, Smith, Bard, Beekeeper, Coffer, Doctor, Drake, Hedge, Mason, Outlaw and Witch. Edo includes Komuso, Ashigaru, Oni, Ronin, Kitsune, Shinobi, Kappa, Tanuki, Tengu, Yurei and Miko. North Sea includes Shieldmaiden, Berserker, Skald, Seer, Valkyrie, Draugr, Raven, Troll and Wolf. [The Watch](https://dweller.pet/#col-the-watch), [Edo](https://dweller.pet/#col-edo), [North Sea](https://dweller.pet/#col-north-sea)
- Old Empires includes Scribe, Gladiator, Hoplite, Legion, Emperor, Jackal, Mummy, Pharaoh, Sphinx, Cyclops and Minotaur. High Seas includes Surfer, Keeper, Deckhand, Diver, Angler, Captain, Mermaid, Navigator, Gull, Kraken and Hermit. Bestiary includes Golem, Wyrmling, Griffin, Gargoyle, Goblin, Imp, Lich, Phoenix, Sapling, Scarecrow, Shade, Unicorn, Yeti, Hound and Wisp. [Old Empires](https://dweller.pet/#col-old-empires), [High Seas](https://dweller.pet/#col-high-seas), [Bestiary](https://dweller.pet/#col-bestiary)
- Night Shift covers everyday occupations and developer humour: Barista, Courier, Guard, Intern, Janitor, Sysadmin, Ducky, Sweeper. Far Reaches covers Astronaut, Moonlet, Saucer, Scout (space dog), Unit (robot), Visitor, Mechanic, Glorp, Satellite, Rover. [Night Shift](https://dweller.pet/#col-night-shift), [Far Reaches](https://dweller.pet/#col-far-reaches)
- Hollow includes Bones, Count, Howl, Jack, Midnight, Sheet, Flit, Shamble, Webster, Digger; captions point toward skeleton/vampire/werewolf/pumpkin/black cat/ghost/bat/zombie/spider/grave-digger motifs, but these identity readings were not all visually verified. Deep Blue includes Jelly, Puffer, Seahorse, Whale Calf, Octopus, Lantern, Shark Pup, Turtle, Otter, Crab. [Hollow](https://dweller.pet/#col-hollow), [Deep Blue](https://dweller.pet/#col-deep-blue)
- Oddballs covers Caterpillar, Chameleon, Clam, Floaties, Goldfish, Lobster, Porcupine, Sloth, Ant, Crossing, Shades, Giraffe, Chilly, Mosquito, Onion, Rocket and Rooster. Olympus includes Chimera, Harpy, Satyr, Centaur, Siren, Hydra, Pegasus, Cerberus, Pythia, Icarus, Medusa. Keepsakes includes Highland, Cow, Mop (caption identifies a dog), Snowman, Pumpkin, Stitches and Banana. [Oddballs](https://dweller.pet/#col-oddballs), [Olympus](https://dweller.pet/#col-olympus), [Keepsakes](https://dweller.pet/#col-keepsakes)
- The homepage says cast members walk, wave, hop and perform an individual small job. Hovering is advertised as a job preview; clicking lets a pet onto the web page. Desktop pets can be dragged to a chosen height and wander there; size, count and liveliness are user controlled. These are vendor descriptions; no app installation/runtime verification was performed. [Dweller](https://dweller.pet/)
- Dweller accepts imported Codex pet folders containing `pet.json` and `spritesheet.webp`; its description refers to frame rows for idle, walk, wave and other actions. [Dweller](https://dweller.pet/)
- The vendor reports 16 MB memory with four detailed pets, 0% app CPU during idle/breathing/blinking, 0.1–0.6% while walking, and fewer than one idle wakeup/second. The stated test system is Apple M5 Pro with macOS 26.6.2; the site explicitly says the compositor draws and its costs are outside the app figures. **Do not turn these into independently established efficiency or battery comparisons.** [Dweller](https://dweller.pet/)
- Terms ask users not to copy pets out of Dweller. Imported user art must be art the user has the right to use. Use original art and original character treatment when drawing inspiration. [Terms](https://dweller.pet/#terms)

### Inferences
- Dweller's strength is not merely number of species: recognisable silhouette + personal prop + amusing one-line personality + occasional individual action makes an otherwise passive companion feel like a character.
- Better Pets should not claim that dragons, ghosts, robots, jellyfish, plants, occupations, food, folklore or oddball wildlife are gaps in Dweller: examples of all are already present.

### Gaps
- The generic web reader could not access the homepage, and a broad web search failed to provide useful Dweller indexing. The live in-app browser loaded it successfully; all source findings above come from that live page.
- I did not download/install the app, verify imported packs, independently measure performance, inspect every animation, or test the web drag controls. Thus specific interaction quality, number of unique actions, download catalogue parity, exact asset format versions and actual power use remain unknown.
- Visual aesthetics were directly checked for the hero and Timber Moose cards; the complete catalogue was read through the accessibility tree, not individually reviewed visually.
- FAQ expansion via a DOM locator did not resolve the control. No claims depend on hidden FAQ text.

## Which audiences does it appear designed to appeal to?

### Takeaway
Audience segmentation is an interpretation of the live product presentation, not a measured market study. Dweller presents gentle, low-obligation companionship with unusually wide theme coverage. [Dweller](https://dweller.pet/)

### Cited Findings
- The site repeatedly pairs grand archetypes with restful or mundane personalities: warriors taking breaks, legendary beings waiting quietly, a tired mummy, and a self-contained sea creature. It offers detailed character sprites rather than a plain geometric mascot in the views inspected. [Cast](https://dweller.pet/#cast)
- It foregrounds free access, small download size, low app resource use, no account, no analytics/telemetry, and explicit network behaviour. These are vendor claims, not an independent audit. [Privacy](https://dweller.pet/#privacy)
- It provides obvious pathways for fantasy/history interests, Japanese folklore, nautical themes, gothic characters, space/robots, aquatic animals, novelty food, familiar animals, and office/developer humour. [Cast](https://dweller.pet/#cast)

### Inferences
- Strong fit: fantasy/RPG and pixel-art nostalgia; gentle humour; Mac users concerned about resource use; people who enjoy collecting a cast; people seeking companionship without a separate game.
- Less obvious fit: users wanting a restrained design object; people who dislike pixel art or anthropomorphic faces; people preferring observational wildlife to character comedy; people who want an unusual material/shape language rather than another humanoid/animal species.
- No evidence establishes actual age, gender, conversion rates, sales, market size or unmet demand. Avoid claiming that any proposed persona is validated.

### Gaps
- No customer analytics, sales figures, user research, or published preference data were available from the inspected page.

## Which adjacent original directions are promising for Better Pets?

### Takeaway
The useful design opening is **different appeal and movement**, not trying to outnumber Dweller. Treat the following as original product hypotheses to test against the current Better Pets cast; they are not claimed market facts.

### Cited Findings
- Dweller's observed differentiators are its large themed character cast, detailed shaded pixel art in inspected examples, modest background personality and custom spritesheet compatibility. [Dweller](https://dweller.pet/)
- Its range already includes Sapling, Wisp, Moonlet, robots, food, amphibious/marine life, wildlife, and gothic/fantasy archetypes; broad nouns alone do not establish differentiation. [Bestiary](https://dweller.pet/#col-bestiary), [Far Reaches](https://dweller.pet/#col-far-reaches), [Timber Moose](https://dweller.pet/#col-timber-moose), [Hollow](https://dweller.pet/#col-hollow)

### Inferences
- **Restrained kinetic object:** an original balance pebble, articulated desk lamp, folded strip or tiny mobile. Give it weight, settling and a distinctive idle; faces optional. Hypothesis: appeals to people who want a tasteful moving object and do not identify as pet collectors. Distinguish through material and motion, not a prop stuck onto the existing body.
- **Paper menagerie:** origami crane/fox/rabbit with actual unfolding, creasing and hopping motion. Hypothesis: maker/stationery/design appeal, a flat limited palette and sharp silhouette separate from detailed sprites. A crane is a useful motion/shape anchor; a second pet should have a genuinely different fold/body topology.
- **Living field notes:** a small moth, jumping spider or leaf insect behaving observably—antenna grooming, wing adjustment, cautious investigation—without clothes or quips. Hypothesis: attracts naturalist and unusual-animal interest. Dweller already has Butterfly, Webster/spider and numerous animals; distinction must be original procedural anatomy and observational behaviour.
- **Pocket conservatory:** an ambulatory succulent pot, fern curl or moss clump with slow unfurling and optional seasonal variation, framed as ambient life rather than a sapling character. Hypothesis: plant/quiet workspace appeal. Avoid claiming the basic plant idea is absent from Dweller.
- **Ink or light organism:** a monochrome brushstroke creature or restrained luminous thread with a coherent topology, little face and deliberate motion. Hypothesis: appeals to abstract-art/minimal desktop preferences. Dweller has Wisp, Shade, Jelly and Moonlet, so the value comes from material expression and procedural motion, not the word “wisp”.
- **Mechanical creature with legible physical behaviour:** a little brass wind-up beetle, ceramic rolling rover or spring bird with imperfect winding/settling. Dweller already has Unit/Rover/Satellite; the reason to choose ours must be tactile materials and distinct locomotion, not “add a robot”.
- **Identity through activity:** a pet that sorts seeds, folds one sheet, balances a found object, constructs a temporary nest or gently investigates a cursor. Each action should reinforce its species/material. Dweller demonstrates the value of a small job; Better Pets can make those jobs modular while keeping character-specific motion.
- For a first expansion shortlist, prefer three clearly different audiences (restrained kinetic object, origami animal, observational tiny wildlife) over three additional cute quadrupeds. Include familiar cute pets later if current catalogue lacks the obvious entry point.
- Validate propositions with a small sheet of original silhouettes and 5–10-second motion samples, asking which users would actually keep on their desktop, what they currently reject, and whether movement feels calming or intrusive. Catalogue breadth alone cannot answer the user's conversion goal.

### Gaps
- These are concept directions, not implementation commitments or validated conversion forecasts. Current Better Pets architecture, art direction, existing cast and animation cost constraints must be checked before prioritising.
- No copied Dweller names, art, sprites, exact prose, branded characters or distinctive costume designs are needed to apply these design lessons.
