# HoMM3 Battle AI — Project Overview & Sub-Project Tracker

**Role of this document:** the single supervisor-level view of the whole project. It tracks every sub-project (goal, status, dependencies), records coordination decisions that prevent duplicated work, and is the first document to update when scope moves between sub-projects.

**Last updated:** 2026-07-05

---

## 1. Project North Star

Build a strong custom battle AI for VCMI (Heroes of Might and Magic III engine reimplementation), climbing from hand-written rules toward a reinforcement-learning agent — with a fast iteration loop and objective measurement at every step.

---

## 2. Sub-Project Map

| ID | Sub-project | One-liner | Status |
|----|-------------|-----------|--------|
| SP-0 | Stage 1: VCMI Build & Rule-Based AI Foundation | Own AI module compiled into VCMI, selectable in-game, proven control of units and hero spells | ✅ **COMPLETED** (2026-07, at Milestone 3.6; Phases 4–5 redistributed — see §4.1) |
| SP-1 | Battle Evaluation Framework ("the Arena") | Automated, repeatable battle scenarios that grade any battle AI with objective scores | 🟡 **DEFINED — not started** |
| SP-2 | Python Bridge AI ("BridgeAI") | A C++ AI module that delegates every decision to an external Python process — edit AI logic with zero rebuilds | 🟡 **DEFINED — not started** |
| SP-3 | (Future) RL Battle Agent | Train an RL policy through the SP-2 bridge, graded by SP-1 | ⚪ Placeholder — not scoped yet |

Detailed design/plan docs:
- `VCMI_Stage1_Plan_and_Design.md` (SP-0, now closed out)
- `SP1_Battle_Evaluation_Framework.md`
- `SP2_Python_Bridge_AI.md`

---

## 3. Dependency & Data-Flow Picture

```
                 SP-0 (done)
        build system, AI-module registration
        pattern, activeStack know-how, stall-guard
               │                    │
               ▼                    ▼
   SP-1 Evaluation Framework   SP-2 Python Bridge AI
   ─ scenario battle setups    ─ BridgeAI C++ module
   ─ headless auto-runner      ─ state/action protocol
   ─ result extraction  ───────► used by SP-2 for
   ─ scoring & reports         acceptance testing
               │                    │
               └────────┬───────────┘
                        ▼
              SP-3 RL Agent (future)
     (SP-2 = the environment boundary,
      SP-1 = the benchmark & reward source)
```

**Build order guidance:** SP-1 and SP-2 can proceed in parallel (the bridge protocol needs no harness), but **SP-1's minimal runner (Milestone A1) should land first** — it immediately speeds up SP-2 debugging (auto-launching test battles instead of clicking through the GUI), and it defines the battle-result hook SP-2's acceptance criteria rely on.

---

## 4. Coordination Decisions (anti-duplication log)

These are binding until revisited. Each records *what*, *why*, and *who owns it*.

### 4.1 Stage 1 Phases 4–5 are redistributed, not executed as written
- **Old Phase 5** (repeatable test battles, comparison vs `BattleAI`) is the manual prototype of SP-1. It is **absorbed into SP-1** — doing it by hand first would be throwaway effort.
- **Old Phase 4** (v1 rule ladder in C++) is **deferred and re-homed**: the rule-based AI will be implemented **in Python on top of the SP-2 bridge**, not in C++. Rationale: the whole point of SP-2 is a seconds-long edit loop; writing the rules in C++ first and porting later duplicates work. The Python rule-based AI becomes simultaneously (a) SP-2's end-to-end validation milestone and (b) SP-1's first seriously graded subject.
- **Consequence:** `MyRuleBasedAI` (C++) stays frozen as the Random Mover — it is now a *test fixture and weak baseline*, not a development track. `ArmageddonAI` likewise stays frozen as the hero-spell reference implementation, valuable when SP-2 adds spell actions to the protocol.

### 4.2 "Run a battle automatically and read its result" is built ONCE — in SP-1
Both new sub-projects need: launch VCMI with chosen AIs on chosen sides, auto-start a specific battle, detect battle end, extract the outcome. **SP-1 owns this** (scenario assets, launcher, result extraction — including any small engine-side hook if log scraping proves insufficient). SP-2 consumes it as a library/CLI and must not grow its own copy. If SP-2 needs a runner feature SP-1 lacks, the feature request goes to SP-1's backlog.

### 4.3 Engine-side code changes are pooled and minimized
SP-0 taught us: touching `lib/` triggers slow rebuilds, and every new AI needs the two-whitelist registration (CMake/AIFactory + `settings.json` enums). Therefore:
- All engine-touching changes across sub-projects (BridgeAI registration, any result-reporting or deterministic-seed hook) are tracked in one list (§6) and **batched into as few `lib/`-touching builds as possible**.
- The registration checklist from SP-0 (Phase 3 step 11, incl. the settings-schema gotcha) is the canonical procedure for registering `BridgeAI`.

### 4.4 The SP-2 protocol is designed with SP-1 and SP-3 in mind
- The state/action encoding must serve *both* a hand-written Python rule AI (readable) and a future RL agent (complete, stable, versioned). SP-2 should study **vcmi-gym**'s state/action encoding as prior art before inventing one, but is not required to adopt it (vcmi-gym is a Linux-oriented fork; we are a lightweight in-tree module on Windows).
- Battle-outcome data emitted at battle end should use the same schema SP-1's result extraction uses, so SP-1 scores and future RL rewards come from one source of truth.

### 4.5 The stall-guard is a universal invariant
SP-0's rule stands everywhere: an AI (or a bridge on timeout/disconnect) must **always submit some valid action** (wait/defend fallback) or the battle hangs. SP-2 implements this as a protocol-level timeout; SP-1's runner additionally implements a watchdog (max wall-time per battle) so a broken AI fails a run instead of freezing the harness.

---

## 5. Current Status Snapshot

| Item | Status | Notes |
|------|--------|-------|
| VCMI builds from source (RelWithDebInfo, Windows/VS2022) | ✅ | Fast path: only touch `AI/<module>/` |
| `MyRuleBasedAI` (Random Mover) registered & working | ✅ | Frozen — serves as weak baseline / fixture |
| `ArmageddonAI` (hero-spell demo) registered & working | ✅ | Frozen — spellcasting reference for SP-2 protocol |
| SP-1 design doc | ✅ written | Open questions listed in its §7 |
| SP-2 design doc | ✅ written | Open questions listed in its §7 |
| SP-1 implementation | ⬜ not started | Start with Milestone A1 (minimal runner) |
| SP-2 implementation | ⬜ not started | Can start protocol draft in parallel |

---

## 6. Pooled Engine-Touching Change List (see §4.3)

| Change | Needed by | Status |
|--------|-----------|--------|
| Register `BridgeAI` (CMake option, AIFactory branch, settings.json enums ×3) | SP-2 | ⬜ |
| Battle-end result reporting (structured log line or hook) — *only if existing logs are insufficient* | SP-1 (primary), SP-2 reuses | ⬜ investigate first |
| Deterministic RNG seed option for repeatable battles — *only if needed and feasible* | SP-1 | ⬜ investigate first |

Rule: before adding to this list, first check whether existing VCMI facilities (launch flags, logs, config) already provide it.

---

## 7. Supervisor Watch-List (risks spanning sub-projects)

1. **Headless automation is the load-bearing unknown.** Both sub-projects assume VCMI can be driven battle-to-battle without a human. SP-1 Milestone A1 exists precisely to retire this risk early. If it proves hard, both plans get restructured — escalate immediately.
2. **Randomness vs repeatability.** Morale/luck and AI randomness mean single battles are noisy. SP-1 handles this statistically (N repetitions) first; deterministic seeding is a later optimization, not a blocker.
3. **Protocol churn.** Once the Python rule AI (and later RL) is built on the SP-2 protocol, breaking changes get expensive. SP-2 must version the protocol from message #1.
4. **Scope creep into the engine.** Every convenience is tempting to implement in `lib/`. Default answer is no (§4.3).

---

## 8. Change Log

- **2026-07-05** — Project restructured into sub-projects. SP-0 (Stage 1) closed at Milestone 3.6; Phases 4–5 redistributed per §4.1. SP-1 and SP-2 defined and documented.
