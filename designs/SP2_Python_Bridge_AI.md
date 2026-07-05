# SP-2: Python Bridge AI ("BridgeAI")

**Status:** 🟡 Defined — not started
**Depends on:** SP-0 (completed) — module registration procedure, `activeStack` know-how; SP-1 runner (for acceptance milestones B3+)
**Consumed by:** SP-3 (the RL agent will sit on this exact boundary)
**Last updated:** 2026-07-05

---

## 1. Goal

Make AI iteration take **seconds, not tens of minutes**: build one C++ AI module, `BridgeAI`, that never needs to change again for logic reasons — every decision is delegated over IPC to an external Python process. Editing AI behavior = editing Python. This same boundary is, by design, the environment interface a future RL agent (SP-3) plugs into.

## 2. Objectives

1. **`BridgeAI` C++ module:** implements the standard battle-AI interface; on each `activeStack`, serializes the battle state, sends it to a connected Python process, waits (with timeout) for an action, translates it into an engine action, and submits it.
2. **Wire protocol v1:** a versioned, documented message schema (state → out, action → in, battle-start/battle-end events) complete enough for real play: move, melee attack, shoot, wait, defend — with hero-spell actions as a fast-follow (reference: `ArmageddonAI`).
3. **Python library `vcmi_bridge`:** hides sockets/serialization behind a callback API — roughly `serve(on_active_stack: state -> action)` — plus typed state/action classes.
4. **Two Python reference AIs:** (a) the drunkard reimplemented in Python (parity smoke test vs C++ `MyRuleBasedAI`), and (b) the **v1 rule ladder** from Stage 1 §5 — the re-homed old Phase 4 (see Overview §4.1) — which becomes the project's first real rule-based fighter.
5. **Robustness invariants:** timeout/disconnect → fallback action (defend), so a battle can never hang on the bridge (Overview §4.5).

## 3. Background

Two pressures created this sub-project:
1. **Iteration cost.** SP-0 showed even the "fast path" (recompile one module + relink) costs real minutes per change, and any `lib/` touch is far worse. Rule tuning involves hundreds of small edits — the compile loop is the bottleneck.
2. **The RL future.** Stage 2 always planned to drive battles from Python (cf. vcmi-gym). Building the bridge now means the rule-based AI and the RL agent share one interface, one state encoding, and one set of lessons.

Inherited from SP-0:
- The decision boundary is `activeStack` (state in → one action out); hero spells are also decided there, submitted via `battleMakeSpellAction` with `stackNumber = -1` (see `ArmageddonAI`).
- Registering a new AI requires the **two-whitelist procedure** (own CMake target + `ENABLE_*` option + `AIFactory` branch + `settings.json` enums ×3). This is BridgeAI's only `lib/`-touching change — pooled per Overview §4.3/§6.
- Prior art: **vcmi-gym** drives VCMI battles from Python headlessly via a fork. Study its state/action encoding before finalizing ours (Overview §4.4); we deliberately build a lightweight in-tree module on Windows instead of adopting the fork.

## 4. Scope

**In scope**
- The `BridgeAI` module (C++), its registration, and its IPC client/server code.
- Protocol design & versioned documentation: state schema (units with id/side/position/stats/HP/shots, hex grid & obstacles, reachable hexes, legal actions), action schema, lifecycle messages (hello/version handshake, battle start, active-stack request, action reply, battle end w/ outcome).
- Transport: localhost TCP socket with length-prefixed JSON as the v1 default (simple, debuggable, language-agnostic); revisit only if profiling demands it.
- The `vcmi_bridge` Python package + the two reference AIs (drunkard, v1 rule ladder).
- Failure handling: timeouts, reconnect policy, fallback actions, clear logging on both sides.
- Battle-end outcome message aligned with SP-1's result schema (Overview §4.4).

**Out of scope**
- RL training, gym-style API wrappers, vectorized environments (SP-3).
- Rewriting `MyRuleBasedAI`/`ArmageddonAI` — they stay frozen as fixtures/references.
- Performance work beyond "human-watchable and harness-runnable" (per-decision latency of a few ms is irrelevant at rule-AI scale; RL-scale throughput is an SP-3 concern).
- Scenario creation and grading infrastructure (SP-1 owns it; BridgeAI is just another AI name the SP-1 runner selects).
- Adventure-map AI, multiplayer, non-local transport, security hardening.

## 5. Design Sketch

```
   VCMI engine                          Python process
 ┌─────────────────┐   localhost TCP   ┌─────────────────────┐
 │  BridgeAI (C++) │◄─────────────────►│  vcmi_bridge lib    │
 │  activeStack:   │  JSON messages    │  serve(callback)    │
 │   state → send  │                   │   your_ai.py:       │
 │   recv → action │                   │    state → action   │
 │   timeout →     │                   └─────────────────────┘
 │   defend        │
 └─────────────────┘
```

Key decisions to hold:
- **Engine is the server of truth; Python is the policy.** BridgeAI sends *legal* options (reachable hexes, valid targets) so Python never needs to reimplement rules of movement; Python may still send an illegal action by bug → BridgeAI validates, logs, and falls back to defend rather than crashing.
- **Blocking request/response per decision.** One outstanding request at a time; simple and sufficient.
- **Protocol versioned from message #1** (`{"v": 1, ...}`); breaking changes bump it (Overview watch-list #3).
- **Connection model:** BridgeAI listens (or connects — decide during investigation) at battle start; if no Python peer within grace period, log loudly and play pure-defend so harness runs fail visibly-but-cleanly.

## 6. Milestones & Definition of Done

- **B1 — Plumbing:** `BridgeAI` registered (two-whitelist), selectable, echoes a hardcoded defend for every stack. Proves module + registration.
- **B2 — Round trip:** state serialized out, Python replies with wait/defend, battle completes end-to-end via the socket. Protocol v1 doc drafted.
- **B3 — Python drunkard parity:** random mover in Python behaves like C++ `MyRuleBasedAI`; verified via SP-1 runner (win-rate vs `StupidAI` statistically indistinguishable from the C++ drunkard's baseline).
- **B4 — Full action set:** move/attack/shoot/wait/defend all usable; the **v1 rule ladder** implemented in Python plays full battles.
- **B5 — Hero spells (fast-follow):** spell actions in protocol; Armageddon-bot reproduced in Python.

**Done when:** you can edit a Python rule file, rerun the same battle within seconds with **zero rebuilds**, and the SP-1 arena grades the Python rule AI above `MyRuleBasedAI`'s baseline — with no battle ever hanging on a bridge failure.

## 7. Open Questions / Investigation List

1. Threading/blocking: is it safe for `activeStack` to block on a socket read (with timeout), or must the reply be delivered asynchronously? Check how the engine calls the AI (thread, expected latency, watchdogs).
2. What's the cheapest sufficient state serialization available via the AI callback interface (`battleGetAvailableHexes`, stack queries, obstacle queries) — and what does vcmi-gym encode that we'd otherwise forget (e.g., retaliation state, wait-order, war machines)?
3. Server vs client role for BridgeAI's socket; port/config via settings or env var (avoid new engine config plumbing if possible).
4. How to represent "legal actions" compactly — full enumeration vs reachable-hex set + target list.
5. Battle-end message: confirm alignment with whatever result source SP-1's investigation (its §7.2) settles on — one schema, two consumers.

## 8. Status Log

- **2026-07-05** — Sub-project defined; absorbed old Stage 1 Phase 4 (rule ladder moves to Python, milestone B4). Not started.
