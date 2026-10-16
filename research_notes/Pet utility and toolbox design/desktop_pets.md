# Desktop pet utilities and reusable interaction features

Researched 2026-10-08. Ratings below are design judgments for Better Pets, not measured user satisfaction or benchmark results. Primary project documentation was examined; projects were not installed or performance-tested. Scores mean 1 = mostly novelty, 3 = useful to some workflows, 5 = directly addresses a frequent workflow need.

## Which equivalent projects offer useful features?

### Takeaway
Agent-monitoring pets offer the strongest immediate utility: make finished work and requests for input visible, then let the user return to the originating task. Traditional desktop pets offer good interaction and animation vocabulary, but much weaker practical utility.

### Cited Findings

| Project | Verified features | Better Pets utility / 5 | What to adopt and what to defer |
|---|---|---:|---|
| AgentPet | Menu-bar list of agents with project, state and elapsed time; done/input notifications; themed bubbles; per-project pets, usage HUD and optional break reminder. It also rewards tokens and completed sessions with XP. | 5 | Adopt source grouping, needs-input count, contextual cards and optional reminders. Defer token-based rewards and leaderboards: they add distraction and can reward spending rather than outcomes. [Official repository](https://github.com/ntd4996/agentpet) |
| UniPet | Local agent hooks/plugins feed idle/running/waiting/failed/review states through CLI, HTTP or WebSocket. Events include source, state, message, action and TTL. Renderer uses bubbles and companion motions. Electron overlay; native shell is future work. | 5 | Adopt the normalized event contract and adapter boundary, not its renderer implementation. [Official repository](https://github.com/ydyangdan/UniPet) |
| PeonPing | Agent complete/input/error notifications; independently controlled audio and visual banners; optional macOS Focus suppression; short-task suppression and repeated-idle deduplication; clicking supported notifications returns to the originating terminal/window. | 5 | Adopt suppression, deduplication and source-return. Optional audio can help when the pet is out of sight. Avoid noisy defaults. [Official README](https://github.com/PeonPing/peon-ping/blob/main/README.md) |
| DyberPet | Pets, tasks, shop, progression, items, mini-pets and optional AI/to-do assistance. Published release notes connect focus animations to timers, distinguish start/end/cancel events, merge like notifications, and allow popup disabling. LLM module is not fully open-source. | 4 for timers/tasks; 2 for care economy | Adopt timer/reminder integration and notification merging. Defer food/shop obligation and another independent conversational assistant. [README](https://github.com/ChaozhongLiu/DyberPet/blob/main/README_EN.md), [official releases](https://github.com/ChaozhongLiu/DyberPet/releases) |
| VPet | Click toolbar, dialogue bubbles, work timer, petting/lifting/climbing; extensible animations/items/work/dialogue/themes/plugins. Documentation explicitly names alarm and notes as plugin examples. | 3 | Adopt a click-to-open tools surface and prop attachment concept; keep alarm/notes as integrations rather than presume they are all bundled defaults. [Official README](https://github.com/LorisYounger/VPet/blob/main/README_en.md) |
| Agent Pet (TrizenX) | Agent hooks map activity to gestures: digging for shell, typing for edits, waving for approval and flattening when rate-limited. README describes versioned events/adapters and a diagnostics command; bundled art is placeholder and release status remains limited. | 4 | Adopt a small, legible gesture language. Detailed per-tool animation is optional: busy/waiting/failed is more important than differentiating every tool. [Official repository](https://github.com/TrizenX/agent-pet) |
| VS Code Pets | Pointer interaction, hover bubbles, named pets, ball chase, multi-pet friendship and chase. | 2 | Adopt occasional direct play and personal names for attachment. These do not provide notification utility by themselves. [Official documentation](https://tonybaloney.github.io/vscode-pets/pets/) |
| Shimeji-ee family | Configured floor/wall/ceiling/window movement, sitting and window-throwing behaviors; customizable XML behavior/action sets and tray controls. Sources here are the historical exported engine and a maintained fork, not a verified current canonical Kilkakon release. | 2 | Adopt pose composability and explicit allowed-behavior controls. Defer window climbing and manipulation: effort and interference outweigh ground-only product value today. [Engine behavior definitions](https://github.com/TigerHix/shimeji-ee/blob/master/conf/behaviors.xml), [VShimeji README](https://github.com/Valkryst/VShimeji/blob/main/README.md) |
| eSheep / DesktopPet | XML-configured characters, window detection, taskbar movement and multiple monitors. Author explicitly describes the project as companionship rather than a utility. | 1 for tasks; 3 for ambience | Adopt predictable unobtrusive placement. It is a benchmark for charm, not an actionable toolbox. [Official repository](https://github.com/Adrianotiger/desktopPet) |
| Desktop Goose | Steals pointer, leaves mud, opens messages/images; configurable phrases, memes, audio/behavior and mod API. Author's page explicitly says Mac does not support mods yet. | 1 for utility; 4 for character acting | Adopt the physical delivery-of-a-note idea. Do not adopt forced pointer capture, screen clutter or repeated disruptive windows. [Creator page](https://samperson.itch.io/desktop-goose) |

### Inferences

- A pet should represent work the user already cares about. Adding hunger, shops and mandatory care creates another queue to service.
- “Refresh a chat” needs a precise source-defined event, such as session expired, disconnected, approval required or ready for review. Completion and waiting are verified precedents; arbitrary chat refresh detection is not.
- Aggregate multiple sources into one attention count and ordered inbox. One pet per active chat can quickly become visual clutter, though an optional per-project companion may be attractive.

### Gaps

- None of these sources establishes a comparable battery, CPU or RSS budget for Better Pets. Native/Qt/Electron language choice alone does not establish resource use.
- README support claims are not evidence of reliable integrations on this Mac. Lifecycle ordering, missing hooks, reconnects, stale states and source focusing need actual tests.
- No verified adoption or satisfaction data was found to justify claiming the ratings are objective.

## Which features can be collated into larger capabilities?

### Takeaway
Three reusable capabilities cover almost all practical features: an attention inbox, a summoned toolbox, and small optional status displays. They can share one card/action model while animation remains a presentation layer.

### Cited Findings

- Normalized lifecycle events and source identity already power UniPet's connectors. AgentPet demonstrates a list of simultaneous agents rather than only one global animation. [UniPet](https://github.com/ydyangdan/UniPet), [AgentPet](https://github.com/ntd4996/agentpet)
- VPet separates click toolbar, speech presentation and timer in its documented architecture. DyberPet timer events include start/end/cancel and notification merging. [VPet](https://github.com/LorisYounger/VPet/blob/main/README_en.md), [DyberPet releases](https://github.com/ChaozhongLiu/DyberPet/releases)
- PeonPing demonstrates that attention delivery, audible cues and quiet policies need independent controls. [PeonPing](https://github.com/PeonPing/peon-ping/blob/main/README.md)

### Inferences

| Consolidated capability | Contents | Priority | Suggested rules |
|---|---|---|---|
| Attention inbox | Agent done, approval/input required, failure, disconnected/stale state, reminder due, timer complete, build/job outcome | First | Deduplicate by source/event, retain unresolved actionable items, expire transient success, show one compact summary, open originating task on click, snooze/dismiss, respect quiet mode. |
| Summoned toolbox | User-pinned app links, project/chat shortcuts, start timer, capture note, user-defined safe actions, utility modules | Second | Click pet or shortcut to summon; searchable conventional panel; pin recent tools. Do not force navigation through animated hands or signs. |
| Status cards | Current focus countdown, agent queue, selected job progress, optional usage/resource readings | Third | User chooses cards. Only show progress when provider actually reports it. Update by events/deadlines instead of polling every animation frame. |

Recommended schema concept: cards provide source, title, short detail, status/severity, timestamp, expiry and registered actions. A provider supplies data; an action registry executes explicit user requests; a presentation policy chooses badge/bubble/gesture/sound/system notice. The same card can appear above the head, in an expanded panel, or in history. This is a design recommendation, not a claim that equivalent projects share this exact schema.

Suggested usefulness ranking: actionable agent inbox 5; source-return shortcut 5; pinned toolbox 4; timers/reminders 4; project/status cards 4; notes/integrations 3; optional break encouragement 3; conversational AI 2; ball chase 2; care economy 2; pranks/window interference 1. Toolbox usefulness depends on the user's actual tool list, including the separate Vorsaint inspection.

### Gaps

- This worker did not inspect Vorsaint; the parent is responsible for local app evidence.
- Exact provider permissions and available Codex chat deep links require local architecture/integration review before implementation.

## What body fixtures and presentation primitives do these utilities require?

### Takeaway
A small gesture rig plus stable attachment anchors is enough. Readable UI should remain a separate layer so every character can communicate reliably, even when it has no hands or expressive mouth.

### Cited Findings

- VPet's documented display splits message bubble and toolbar from animation rendering, and its food renderer supports layered props. [Official architecture](https://github.com/LorisYounger/VPet/blob/main/README_en.md)
- Agent Pet explicitly connects approval to a wave and activity to recognizable actions. VS Code Pets demonstrates hover speech independent of any deep conversational system. [Agent Pet](https://github.com/TrizenX/agent-pet), [VS Code Pets](https://tonybaloney.github.io/vscode-pets/pets/)
- Desktop Goose establishes the visual vocabulary of physically delivering a message, without showing that its disruption improves utility. [Creator page](https://samperson.itch.io/desktop-goose)

### Inferences

- Minimum rig: torso/chest aim, shoulder-elbow-wrist chains, hand/grip anchors, head/gaze, optional mouth/jaw, and existing tail/ears. Mallow can use short mitten arms; werewolf can use its actual hand bones.
- Add logical anchors for speech top, attention badge, front/back prop layer, left/right grip and pointing target. Anchors should be projected consistently in both 2D and 3D and included in preview overlays.
- Gesture vocabulary: look toward user, raise/wave one hand, point to attached card, hold sign with one/two grips, present item, nod, small success acknowledgement, puzzled/shrug or lowered-head failure. Compose procedural targets and bounded timed gesture envelopes over grounded movement; do not author a frame sheet per state.
- Independent UI primitives: bubble with plain text and optional one action, compact attention count, expandable card panel, progress/countdown, restrained effect burst. Detailed text/actions belong in a normal panel with keyboard support, not painted into a low-resolution sign texture.
- Add motion priorities so hand gesture can run while walking, while a two-hand sign can ask the character to stop. Foot planting must retain authority; expression/arms/props must not overwrite leg IK.
- Attention should escalate once (gaze → wave/badge → persistent card or optional native notice), then settle. Continuous waving or flashing is likely to make the pet annoying.

### Gaps

- The fixture list is an implementation recommendation inferred from behavior requirements. It requires preview testing at actual desktop size, especially arm readability and safe bubble placement near display edges.
- Full talking lip-sync, throwing props, sitting, jumping and climbing are not prerequisites for the proposed utility MVP.
