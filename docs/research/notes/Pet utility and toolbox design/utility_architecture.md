# Pet utility architecture and toolbox patterns

## Which utility patterns transfer well to a desktop pet?

### Takeaway
The strongest pattern is an actionable status item: receive a real event, show one concise card, and offer the relevant next action. A small pet toolbox can reuse that same card/action machinery; it does not need to reproduce a launcher or dozens of utilities.

### Cited Findings
- Raycast ActionPanel provides context-aware actions, semantic sections, shortcuts and primary/secondary actions. Its toast supports changing an in-progress item to success/failure and attaching primary/secondary actions; closed-window toast display falls back to HUD. Its command launcher supports passing context and launching another command, but the launch promise does not signify that execution finished. — [ActionPanel](https://developers.raycast.com/api-reference/user-interface/action-panel), [Toast](https://developers.raycast.com/api-reference/feedback/toast), [Command](https://developers.raycast.com/api-reference/command)
- Alfred workflows connect triggers, inputs, actions, UI, utilities and outputs. Output objects can display notifications, large text, clipboard output or script results. Universal Actions first select an object (file/text/link), then narrow the available actions to the object type; custom workflow actions join that list. Universal Action hotkeys require Accessibility permission. — [Workflows](https://www.alfredapp.com/help/workflows/), [Universal Actions](https://www.alfredapp.com/help/features/universal-actions/)
- SwiftBar accepts scripts in arbitrary languages and interprets stdout as a status header and menu items. Items can open URLs or launch scripts. Standard plugins optionally refresh periodically; ephemeral items are created on demand with a lifetime; streamable plugins remain alive and emit updates. Notification URLs support text, a click destination, and silent delivery. — [SwiftBar official README](https://github.com/swiftbar/SwiftBar)
- Hammerspoon's chooser offers a searchable list with text, subtext and optional images; a completion callback receives the chosen item, and its default global callback restores prior focus after dismissal. Its notification module supports delivery and click callbacks, including callback registration surviving config reloads. — [Chooser](https://www.hammerspoon.org/docs/hs.chooser.html), [Notifications](https://www.hammerspoon.org/docs/hs.notify.html)

### Inferences
Scores below are judgments about usefulness **to Better Pets**, not product quality, popularity, or measured performance.

| Project pattern | Transfer usefulness /5 | Borrow | Avoid expanding into |
|---|---:|---|---|
| Raycast context action + actionable status | 5 | One primary next step, concise secondary actions, update one card as state changes | Full search engine/extension store |
| SwiftBar status header + on-demand tools | 5 | Cheap event-fed glance state; status and actions share one source | Frequent shell polling, persistent process for every small tool |
| Alfred workflows + typed object actions | 4 | Trigger→data→action→feedback composition, drop/file context | General visual workflow editor in the first release |
| Hammerspoon chooser + focus restoration | 4 | Keyboard-accessible toolbox, restore focus, action callbacks | Desktop-wide automation runtime/accessibility observation by default |

An event-fed card with action identifiers offers the useful subset of all four. The pet becomes a friendly presence and point of access, while its UI keeps text legible and the actual work stays in existing tools.

### Gaps
- No empirical energy measurement for these projects was gathered. Refresh architecture guides a proposed battery policy, not a battery-life claim.
- Vorsaint inspection belongs to the coordinator's separate local-app work. No capabilities of Vorsaint are assumed here.

## What does Better Pets already have, and what body/UI fixtures are missing?

### Takeaway
The project already has a useful bounded session core, not merely a creature walking on a plane. The missing pieces are genuine producer integrations, actionable presentation and optional semantic anatomy that both renderers can implement.

### Cited Findings
- Session v1 accepts `working`, `needs_input`, `completed`, `failed`, source/session identifiers, strictly increasing sequences and bounded TTL. It retains latest state, separates acknowledgment from source status, arbitrates foreground priorities, suppresses reactions when hidden/paused and avoids retriggering on unchanged refreshes. It has a same-user local JSON-RPC endpoint and `petctl`. The checked-in producer is simulated, and live socket acceptance remains outstanding in this sandbox. — [Session API](../../../api/session-v1.md), [Handoff](../../../handoff.md)
- The existing interface deliberately cannot execute code or open files. Its same-user boundary does not authenticate the claimed source namespace as a particular app. Labels are bounded plain text, with no persistent session data. — [Session API](../../../api/session-v1.md)
- Current `StatusLabel` is a separate input-transparent, non-focus-taking QLabel. It positions beside the pet from fixed offsets and clamps to a screen; it is not an interactive speech/action card. The application already owns tray/context menus and per-session acknowledgment. — [Label header](../../../../qt/resident/status_label.h), [Label implementation](../../../../qt/resident/status_label.cpp), [Resident application](../../../../qt/resident/application.cpp)
- The 2D definition has bones, attachments, string semantic bindings, contacts, interaction polygon and locomotion. Its action enum already includes Attend, Celebrate, Failure and Acknowledge. Mallow currently binds body/facing/head/eyes/ears/tail/feet and has no arms. — [Character types](../../../../qt/characters/character.h), [Mallow manifest](../../../../characters/soft-creature/manifest.json)
- The werewolf rig already has upper-arm, forearm and hand joints on both sides. Current 3D controller drives legs and velocity/acceleration followers on spine/head/upper arms/tail, but exposes no target-driven reach/hold/point interface. The shared 3D Pose only contains root/yaw/lean/follower and legs. — [Werewolf rig](../../../../characters/werewolf-3d/rig.json), [3D pose types](../../../../qt/characters3d/pose.h), [3D controller](../../../../qt/preview3d/controller.cpp)

### Inferences
Recommended neutral capability vocabulary, resolved by each anatomy/renderer rather than demanding one skeleton:

- `lookAt` and `attentionAnchor`: head/eyes tracking plus an authored above-head attachment for cards. In 3D the anchor projects into display coordinates; the card remains readable 2D UI.
- Optional `reachLeft/reachRight`: shoulder→elbow→hand chains, limits, bend preference/pole, and grip socket. Actual measured segment lengths and reach clamping; no equal-length shortcut inherited from Mallow leg planning.
- `point`, `wave`, `hold`: semantic gesture goals and bounded response dynamics. Gesture side should favor the visible side. Holding a sign combines grip pose + prop socket; sign text is UI/data rather than baked pixel frames.
- Optional `mouth`/`expression`: expression landmarks without promising generated speech or an LLM conversation. Ear/head/body motion can substitute for facial articulation.
- `effectAnchor` and transient effect budget: a small attention mark, sparkle or pulse with maximum duration/count; static unread badge after the brief cue instead of endless bounce.
- Explicit fallback: limb-free Mallow looks toward a bubble and makes a small body/ear gesture. Wave requires arms; hold requires a grip. Missing anatomy never prevents notification delivery.

Recommended fixture order: head/card/effect anchors first; one procedural arm wave/point/hold vertical slice for werewolf and optionally simple Mallow nubs second; two-hand held props later. Keep body/locomotion and expressive layers distinct, with contact constraints applied after upper-body goals so attention cannot break planted feet. Test marker positions and joint reach while reversing, paused, dragging and screen-clipped.

### Gaps
- No clickable action dispatch, action descriptor schema, prop sockets or hand IK was found in the inspected types/path. These recommendations require new code; they are not existing capabilities.
- Native renderer acceptance, focus/click regions and energy still need desktop evidence, independently of functional core tests.

## Which feature families should ship first, and what proves genuine utility?

### Takeaway
Ship an attention inbox first, then a small contextual toolbox and a few deadline-based displays. A single model of a card, source, lifetime and actions can serve notifications, timers, progress and tool results.

### Cited Findings
- Claude Code documents `UserPromptSubmit`, `PermissionRequest`, `Notification`, `Stop`, `StopFailure` and session lifecycle hooks. A permission notification can be delayed; `PermissionRequest` is the direct permission event. `Stop` means the main agent finished responding, not necessarily that all work finished; its background-task fields can distinguish that. `StopFailure` covers API errors. `agent_completed` can mean finished **or failed**. These facts require explicit mapping rather than assuming a notification is success. — [Official hook reference](https://code.claude.com/docs/en/hooks)
- Better Pets TTL expiry removes records, and transport failure never means completion. Acknowledgment is local; it does not change source status. — [Session API](../../../api/session-v1.md)
- Raycast has per-item contextual actions; SwiftBar offers temporary event-created items as well as menus; Alfred uses typed items to select suitable actions. — [Raycast actions](https://developers.raycast.com/api-reference/user-interface/action-panel), [SwiftBar](https://github.com/swiftbar/SwiftBar), [Alfred Universal Actions](https://www.alfredapp.com/help/features/universal-actions/)

### Inferences
| Family | Usefulness /5 | Cost | Battery fit proposal | Consolidates |
|---|---:|---|---|---|
| Attention inbox | 5 | Medium: adapter + card/action UI; session policy exists | Excellent with producer push + deadline expiry + brief cues | Chat turn ready, needs input, failed task, build result, stale-source notice |
| Contextual toolbox | 4 | Medium for curated registry; high for generalized plugin platform | Excellent when populated/launched on click or hotkey | Open chat/project, run existing shortcut/tool, copy result, open Vorsaint |
| Deadline/status cards | 4 | Low-medium reusing card core; higher with independent remote data integrations | Excellent for deadline-driven timers; use coarse visible-only progress | Pomodoro, countdown, reminder, job progress, waiting/task count |
| Direct manipulation and richer props | 3 | Medium-high: hand IK, drag/drop, object capabilities | Good on demand; avoid background observation | Drop a file/link, hold a sign, point at a card, demonstrate tool result |

These scores assume the user's coding/chat workflow, not general consumer pet priorities. Defer a full dashboard, system telemetry polling, clipboard history, automatic window management and conversational AI until a concrete need outweighs scope and background cost.

First vertical slice:
1. Choose **one real producer the user already runs**, after coordinator inspection of available installed tooling; do not inspect private chats. Candidate Claude Code offers documented hooks; a wrapper around a real build command is a dependable fallback. Codex desktop integration needs a supported local event surface before selection; available chat-list tools in this assistant session do not themselves establish a resident adapter API.
2. Producer emits genuine lifecycle data through existing `petctl`/IPC: input submitted→working; source asks for input→needs_input; response ready→completed label **“Response ready”**; source failure→failed. Adapter distinguishes turn readiness from whole-task completion and background work. No transcript scraping or LLM classification is necessary.
3. Pet gives one short cue, looks/points toward the above-head card, then leaves a quiet unread indicator. Card presents source/title/status and **Open source**, **Acknowledge**. Related sources collate in a small inbox rather than competing bubbles.
4. Open source is a validated user-initiated destination or locally registered adapter callback. Additive action metadata must not turn incoming JSON into shell execution. Keep acknowledgment distinct from opening, and action errors visible rather than automatically marking source work complete.
5. Verify a real event round trip and accurate task label, source opening, duplicate suppression, TTL/offline state, acknowledgment, hide/pause/sleep behavior, deadline scheduling, and native focus/click handling.

“Chat needs refresh” requires an explicit source event with known semantics. Missing activity, dropped IPC, absent heartbeat and TTL expiry are **unknown/offline**, not completion or a reason to reload a chat automatically. If a provider reports a genuinely stale view, display “Refresh available” with a user action.

One shared `Card` concept can carry stable identity, source identity, title/plain text, status, unread/ack state, optional progress/deadline, lifetime, and action descriptors. Reuse existing session identity/arbitration rather than create a competing notifications database. Toolbox entries and notification cards can use one action registry. A curated “Open Vorsaint” entry is initially more useful and cheaper than implementing Vorsaint's whole toolbox; deeper commands depend on discovering its supported launch/deep-link interfaces.

### Gaps
- The actual first chat integration depends on which supported producer is locally available. Claude hook examples are candidates, not proof the user runs Claude Code or a deployed adapter exists.
- No supported Codex desktop local event export or Vorsaint command/deep-link contract was verified in this research scope.
- Rankings/cost/battery fit are design judgments. Battery fit must be tested with the resident and native desktop, including WindowServer/GPU and wakeups.
