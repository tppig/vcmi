# VCMI Rule-Based Battle AI — Stage 1 Plan & Design

**Project:** Build a custom battle AI for VCMI, climbing toward a reinforcement-learning agent.
**This document covers:** Stage 1 only — get a working build of VCMI and implement a simple, rule-based battle AI that plays full battles on its own.
**Platform:** Windows PC, building locally.

---

## 1. Goal & Scope

**Goal of Stage 1:** End up with your *own* battle-AI module, compiled into VCMI, that you can select in-game and watch fight a full battle by itself using hand-written rules (no learning yet).

**In scope:**
- Setting up a working VCMI build from source.
- Understanding how VCMI's battle AI plugs in.
- Creating your own battle-AI module and implementing simple decision rules.
- Running and observing it in repeatable test battles.

**Out of scope (later stages):**
- Reinforcement learning / vcmi-gym (Stage 2+).
- Adventure-map AI (you are only touching the *battle* AI).
- Spellcasting logic (optional stretch goal; keep v1 simple).

**Definition of Done for Stage 1:**
1. A self-built `vcmiclient` runs and loads your test battle map.
2. Your custom AI compiles into its own library (DLL) and is selectable via `setBattleAI`.
3. It plays a full battle autonomously with sensible behaviour.
4. You can run repeatable battles and compare it against the default `BattleAI`.

---

## 2. Design Background — How VCMI's Battle AI Works

This is the part to understand *before* writing code, because it shapes everything.

### 2.1 The battle AI is a self-contained module, selected through a factory

VCMI ships several battle AIs (e.g. `StupidAI`, the simple one, and `BattleAI`, the current default). Each one lives in its own folder under `AI/` and is its own self-contained class.

> **Source-verified correction (was wrong in an earlier draft):** the current source does **not** load each AI as a separate runtime `.dll` with an exported entry point. Instead, each AI builds as a CMake **`OBJECT` library** (`add_library(StupidAI OBJECT …)`) that is **statically linked into the main `vcmi` library** (`target_link_libraries(vcmi PRIVATE StupidAI)` in `libFacade/CMakeLists.txt`), gated by a CMake option like `ENABLE_STUPID_AI`. Selection happens through a central factory — `AIFactory::createBattleAI(name)` in `lib/callback/AIFactory.cpp` — which is a simple `if(name == "StupidAI") return std::make_shared<CStupidAI>();` switch, each branch wrapped in `#ifdef ENABLE_…_AI`. The `setBattleAI <name>` console command still picks the AI by name; that name is matched in this factory.

**Consequence:** to add your AI, you create a new module folder alongside the existing ones **and register its name in two separate systems** — the build/code side (its own CMake target, the `ENABLE_*` wiring, and the `AIFactory` branch) *and* the data side (the `config/schemas/settings.json` enum that validates AI names). You do **not** rewrite the engine. See Phase 3 step 11 for the exact list; the schema one is the easiest to forget.

### 2.2 Architecture / data flow

```
                +------------------------------------------+
                |                VCMI engine               |
                |                                          |
                |   VCMI_lib  ───────────────►  Server     |
                |  (shared core:               (runs all   |
                |   battle state,               game rules)|
                |   stacks, hexes)                 │       |
                +----------------------------------│-------+
                                                   │
                       "Unit X is now active.      │  action
                        Here is the battle state.  │  (move / attack /
                        What do you do?"           ▼   shoot / wait / defend)
                +------------------------------------------+
                |        YOUR BATTLE AI MODULE (.dll)      |
                |                                          |
                |   reads battle state  ──►  rule logic    |
                |                            decides action|
                +------------------------------------------+
```

The loop, each time one of your units gets its turn:
1. The server notifies your AI module that a specific stack is now active and gives it access to the current battle state (your units, enemy units, the hex grid, obstacles).
2. Your module decides what that unit should do.
3. Your module sends the chosen action back to the server, which executes it and updates the battle.

Your whole job in Stage 1 is implementing step 2 with hand-written rules.

### 2.3 Important build relationship

- Your AI module **depends on** `VCMI_lib` (it uses its types: stacks, battle state, etc.).
- `VCMI_lib` does **not** depend on your module.
- Therefore: changing only your module → recompile just your module's object files, then **re-link the `vcmi` library** (fast — no `VCMI_lib` recompile). Changing `VCMI_lib` → rebuild everything (slow). **Keep all your work inside your own module.**
- Note: because the AI is statically linked (not a runtime-loaded DLL), the relink step is real but cheap. The key cost you're avoiding is recompiling `VCMI_lib`, which the static-link model does **not** force.

---

## 3. Detailed Steps

### Phase 1 — Build environment + a working build of *unmodified* VCMI

> Do not write any AI code yet. The goal of this phase is purely: "I can compile and run VCMI from source." This isolates build problems from AI-logic problems.

1. **Install Visual Studio 2022 Community** (free for non-commercial use). During install, select the **"Desktop development with C++"** workload. This gives you the MSVC compiler, CMake, and Git integration. (VCMI requires C++20, so VS 2022 specifically — older versions won't compile it.)
2. **Clone the repo with submodules:**
   ```
   git clone --recursive https://github.com/vcmi/vcmi.git
   ```
   If you forget `--recursive`, run `git submodule update --init --recursive` afterward.
3. **Get the prebuilt dependencies.** Do **not** build Qt/Boost/SDL/FFmpeg yourself — that wastes hours and 10–20 GB. Follow the official "Building VCMI for Windows" guide (vcmi.eu → Developers → Building Windows) and use its **prebuilt dependency package** (Conan or vcpkg-export). Unpack it where the guide says.
4. **Configure with CMake** using the **RelWithDebInfo** configuration (release speed + debug info). Avoid full Debug — it rebuilds every dependency in debug and is extremely slow.
5. **Build the whole solution once.** This first build is the slow step (tens of minutes depending on your CPU). It only happens once.
6. **Run your self-built `vcmiclient`** and confirm it launches, finds your Heroes 3 data, and opens your test battle map.

✅ **Milestone 1:** self-built VCMI runs your battle map.

---

### Phase 2 — Understand the battle AI interface

7. **Locate the AI modules** in the repo, under the `AI/` directory — find `StupidAI` and `BattleAI`.
8. **Read `StupidAI` first** (it's the minimal one). Identify, *from the actual source* (treat the code as the source of truth, since signatures change):
   - The base interface class the battle AI implements.
   - The method called when a unit becomes active (where you receive the active stack + battle state and must return an action).
   - The method called for the optional "tactics phase" before battle.
   - How an action (move / attack / shoot / wait / defend) is constructed and returned.
9. **Read enough of `BattleAI`** to see a fuller example of querying battle state (reachable hexes, enemy stacks, damage estimates), but don't try to absorb all of it.

✅ **Milestone 2:** you can point to the exact method where "decide this unit's action" happens.

---

### Phase 3 — Create your own AI module skeleton

10. **Copy `AI/StupidAI` → `AI/MyRuleBasedAI`.** Rename the files and the class name (e.g. `CStupidAI` → `CMyRuleBasedAI`) consistently. (There is no exported DLL entry point to rename — the current model uses the factory instead.)
11. **Register the module — there are TWO independent name-whitelists, a code-side one and a data-side one. Miss either and the AI won't run.**

    **Build / code registration (four places, following exactly how `StupidAI` is wired):**
    - **a.** Its own `AI/MyRuleBasedAI/CMakeLists.txt` — `add_library(MyRuleBasedAI OBJECT …)`, link `vcmiMain`, `vcmi_set_output_dir`, `enable_pch`.
    - **b.** The top-level `CMakeLists.txt` — add an `ENABLE_MYRULEBASED_AI` option *and* the matching `add_definitions(-DENABLE_MYRULEBASED_AI)`.
    - **c.** `AI/CMakeLists.txt` (`add_subdirectory(MyRuleBasedAI)`) + `libFacade/CMakeLists.txt` (`target_link_libraries(vcmi PRIVATE MyRuleBasedAI)`), both under `if(ENABLE_MYRULEBASED_AI)`.
    - **d.** `lib/callback/AIFactory.cpp` — add `#include "../../AI/MyRuleBasedAI/MyRuleBasedAI.h"` (under an `#ifdef`) and a new `if(name == "MyRuleBasedAI") return std::make_shared<CMyRuleBasedAI>();` branch in `createBattleAI`.

    **Data / settings-schema registration (easy to forget — this one bit us):**
    - **e.** `config/schemas/settings.json` — add `"MyRuleBasedAI"` to the `enum` array of **all three** combat-AI keys: `combatEnemyAI`, `combatAlliedAI`, `combatNeutralAI`. This is a hard validation gate: `setBattleAI` writes the name into `combatNeutralAI`, and if the name isn't in this enum the engine **rejects the whole `ai` settings block and silently falls back to defaults** — your AI never loads even though the C++ is correct. No rebuild needed for this edit: the build links `config/` into the output dir (`COPY_CONFIG_ON_BUILD`), so editing the source schema takes effect on the next launch.
        - **Symptom if you forget it:** at startup the log prints `Data in settings is invalid! At /ai/combatNeutralAI → Error: Key must have one of predefined values: [...]`, and battles keep using the old/default AI.
12. **Build and confirm `setBattleAI MyRuleBasedAI` selects it in-game** (behaviour will still equal StupidAI's at this point — that's fine; you're verifying the plumbing). Note: adding the factory branch touches `lib/`, so this *first* registration build is a heavier rebuild; subsequent edits to your AI's `.cpp` stay in the fast path.
    - **Confirm via the log, not the command's output.** The cleanest proof is the line `Creating battle AI MyRuleBasedAI` (logged at info level when a battle starts). Don't trust the `setBattleAI` "Setting changed" message — for an unknown name the factory returns `CEmptyAI` rather than failing, so a typo still reports success while units do nothing.
    - **Remember which side `setBattleAI` controls:** it writes only `combatNeutralAI`, i.e. the **neutral** side. The enemy hero (`combatEnemyAI`) and your own auto-combat (`combatAlliedAI`) are separate slots set only in the config file. So to watch your AI via `setBattleAI`, fight a **neutral** stack.

✅ **Milestone 3:** your module builds as its own DLL and is selectable in-game.

---

### Phase 3.5 — First custom behaviour: the "Random Mover" (for fun / proof of control)

> A deliberately silly throwaway step before any real AI logic. The point is **not** to play well — it's to prove that *your* code is now driving the units, that you can read the battle state, submit a valid action, and watch the result. This de-risks Phase 4: once a unit visibly wanders where your code told it to, you know the whole pipeline (factory → `activeStack` → callback → engine) is yours to command.

**Behaviour spec (v0 — "the drunkard"):**
- It does **not** attack and does **not** seek the enemy. Each turn it just picks a random hex it can reach and walks there.
- Concretely, inside `activeStack`: query the unit's available/reachable hexes (the same `battleGetAvailableHexes` / reachability calls StupidAI uses), pick one at random, and submit a `makeMove`.
- **Required safety fallback:** if the unit genuinely has no legal move (fully blocked), it must still submit *some* valid action or the battle will hang waiting on it — fall back to `makeWait`/`makeDefend`. This is the one unavoidable exception to "no defend"; it's a stall-guard, not a decision.
- Skip the special cases for now by simply not handling them specially — but be aware catapults/siege weapons can't "move," so the fallback above also covers them.
- `yourTacticPhase` just ends the phase immediately (same as StupidAI).
- Add a one-line log per turn: `"stack X: random-moving to hex N"` (or `"... no move available, waiting"`).

**Why this is safe to throw away:** it lives entirely inside `activeStack`. When you start Phase 4, you replace that one method body with the real rule ladder — nothing else from this step needs unwinding.

✅ **Milestone 3.5:** you select `MyRuleBasedAI`, start a battle, and watch your units wander randomly each turn under your own code — a full battle runs to completion without hanging.

---

### Phase 4 — Implement the rule-based logic

13. **Implement your decision rules** (see Section 5 for the proposed v1 rule set). Start dead simple, then add rules one at a time.
14. **After each change, build only your module** and re-test. Keep the cycle tight.
15. **Add lightweight logging** of each decision ("stack A: shooting stack B" / "stack A: moving to hex N and attacking C") so you can see *why* it did what it did.

✅ **Milestone 4:** your AI plays a full battle on its own with behaviour driven by your rules.

---

### Phase 5 — Test, observe, iterate

16. **Set up repeatable test battles.** Use your custom map and the in-battle **auto-combat** button so the AI plays your side, or launch with `-onlyAI` for a fully hands-off AI-vs-AI game.
17. **Compare against the default.** Put your AI on one side and `BattleAI` on the other (use `setBattleAI` to control the neutral side) and see how your rules hold up. Run the same setup several times to get a feel for win rate.
18. **Iterate on the rules** based on what you observe (e.g. units suiciding, ranged units wasting shots, melee units not blocking shooters).

✅ **Milestone 5:** repeatable battles + a baseline comparison vs `BattleAI`.

---

## 4. The Fast Iteration Loop (memorise this)

After the one-time full build, your day-to-day loop is:

1. Edit your AI's `.cpp` files (the decision logic lives in `activeStack`).
2. Rebuild. Because your AI is an `OBJECT` library linked into `vcmi`, building the client target recompiles only your changed object(s) and relinks `vcmi` — `VCMI_lib` is untouched:
   - In Visual Studio: build the `vcmiclient` target (or your AI target then the client).
   - Command line: `cmake --build . --target vcmiclient`
3. Launch VCMI → `setBattleAI MyRuleBasedAI` → watch it fight.

> Reminder: as long as you only touch files under `AI/MyRuleBasedAI/`, you never trigger a `VCMI_lib` recompile. The one exception was the *initial* registration in `AIFactory.cpp` (Phase 3) — after that, stay out of `lib/`.

Speed tips: build **RelWithDebInfo** (not full Debug); let the build use all CPU cores; consider enabling `ENABLE_CCACHE=ON` in CMake to cache compiled objects; and **never edit `VCMI_lib`** — staying out of it keeps you in the fast path.

---

## 5. Rule Design — v1 Logic

Keep the first version simple and greedy. A reasonable priority order per active unit:

1. **If the unit is a shooter, has ammo, and is not blocked in melee:**
   shoot the "best" enemy in range — e.g. the one where `expected_damage / enemy_value` is highest, or simply the lowest-health-but-high-threat target.
2. **Else if an enemy is reachable this turn:**
   move adjacent to the best target and attack it (prefer killing a stack outright, or hitting the highest-threat enemy).
3. **Else (no enemy reachable):**
   move toward the nearest enemy (close distance), or **defend** if moving into the open would be bad.

Deliberately left out of v1 (add later): spellcasting, morale/luck consideration, protecting your own shooters by blocking lanes, retaliation avoidance. Add these one at a time once the basic loop is solid.

Design principle: each rule should be small and independently testable, so when behaviour looks wrong you can tell which rule fired.

---

## 6. Risks & Gotchas

- **First build is slow** — expected, one-time. Don't panic.
- **Editor/engine crashes are often mod-related** — if something misbehaves, test with mods disabled.
- **Don't use full Debug config** — painfully slow; use RelWithDebInfo.
- **Don't touch `VCMI_lib`** — it forces a full rebuild every time.
- **Match the library/version** — your AI is statically linked into *your* freshly built `vcmi` library; don't mix your build's binaries with a downloaded VCMI's. (The old "cannot find entry point" DLL-loading failure mode no longer applies, since there is no runtime DLL load — but a registration mistake will instead show up as `setBattleAI MyRuleBasedAI` silently falling back to `CEmptyAI`, the factory's default for an unrecognized name.)
- **Keep API assumptions out of this doc** — always confirm exact class/method names in the current source, not from memory or this plan.

---

## 7. Useful In-Game Tools (reference)

Open the in-game chat with **Tab**. Cheat words are typed plain; client commands are prefixed with `/`.

- `setBattleAI <name>` — set which AI the neutral/enemy side uses (persists across quit).
- In-battle **auto-combat button** — hand your own side to the AI so you can watch.
- `-onlyAI` (launch flag) — run with no human player; all sides AI; good for hands-off AI-vs-AI viewing. Don't add `-headless` when you want to watch.
- `/autoskip` — skip your turns so only the AI moves, GUI stays visible (end the first turn manually).
- `vcmimove`, `vcmiarmy <creature>`, `vcmiresources` — cheats to set up test situations quickly.
- Turn **Quick Combat off** so battles open visually instead of auto-resolving.

---

## 8. How This Sets Up Stage 2 (Reinforcement Learning)

Everything you learn here maps directly onto the RL stage:
- The **battle interface** you study in Phase 2 (active stack → decide action → return action) is exactly the boundary an RL environment wraps. The existing **vcmi-gym** project drives VCMI battles through a similar boundary, headless, from Python.
- Your **rule-based AI becomes a baseline** to measure your future RL agent against.
- Your **build skills** carry over — vcmi-gym needs a VCMI fork built from source (and it's Linux-oriented, so expect to rebuild there).

So Stage 1 is not throwaway work; it's the foundation.

---

## 9. Hardware Note

- **For Stage 1, your current PC is fine.** The first compile is just slow; running rule-based battles is light.
- **For Stage 2 (RL), the workload is CPU-bound** (many parallel battle simulations) with only a small neural network, so CPU cores + RAM matter more than a high-end GPU.
- **You don't need to buy hardware now.** When training throughput becomes the bottleneck, you can either run single-environment training (slower but correct) or rent cloud GPU/compute by the hour, which is far cheaper than a big PC purchase for bursty use.

---

## 10. Suggested Order of Attack (quick checklist)

- [ ] Install VS 2022 Community (+ C++ workload)
- [ ] Clone VCMI with submodules
- [ ] Set up prebuilt dependencies
- [ ] Configure CMake (RelWithDebInfo) + full build once
- [ ] Run self-built VCMI on the test map  ✅ Milestone 1
- [ ] Read `StupidAI`; find the decision method  ✅ Milestone 2
- [ ] Copy → `MyRuleBasedAI`, register in CMake, build target, select in-game  ✅ Milestone 3
- [ ] Random Mover v0: units wander randomly under your code  ✅ Milestone 3.5
- [ ] Implement v1 rules + logging  ✅ Milestone 4
- [ ] Repeatable tests + compare vs `BattleAI`  ✅ Milestone 5
