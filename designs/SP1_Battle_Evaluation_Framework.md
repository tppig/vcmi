# SP-1: Battle Evaluation Framework ("the Arena")

**Status:** 🟡 Defined — not started
**Depends on:** SP-0 (completed) — build environment, AI selection knowledge
**Consumed by:** SP-2 (acceptance testing), SP-3 (benchmarking & reward source)
**Last updated:** 2026-07-05

---

## 1. Goal

A push-button way to answer, with numbers instead of eyeballs: **"how good is this battle AI?"** Run a fixed suite of battle scenarios fully automatically, with any chosen AI on the tested side, and produce an objective, comparable score.

## 2. Objectives

1. **Scenario suite:** a small, curated set of battle setups (5–10 to start) that stress different skills — see §5.2.
2. **Automated runner:** a CLI tool (Python) that, given `(scenario, AI-under-test, opponent AI, N repetitions)`, launches VCMI, runs the battles hands-off, and collects outcomes. No human clicks anywhere.
3. **Metrics & scoring:** per-battle outcome (win/loss, surviving army value, rounds taken) aggregated over N runs into per-scenario and overall scores; results stored machine-readable (JSON/CSV) with a human-readable summary.
4. **Baselines on file:** reference scores for `StupidAI`, `MyRuleBasedAI` (Random Mover, the floor), and `BattleAI` (the bar to beat), so every future AI lands on an existing ladder.

## 3. Background

Stage 1 (SP-0) produced working custom AI modules, but "how well does it fight?" is currently answered by watching battles in the GUI — slow, subjective, and not repeatable. Old Stage 1 Phase 5 planned manual repeatable tests; per coordination decision §4.1 in `PROJECT_OVERVIEW.md`, that work is absorbed here and automated instead of done by hand. This framework also becomes the measurement backbone for everything later: SP-2's bridge is validated against it, and the future RL agent (SP-3) trains and benchmarks against it.

Relevant facts inherited from SP-0:
- AIs are selected by name via config keys `combatNeutralAI` / `combatEnemyAI` / `combatAlliedAI` (the `setBattleAI` console command writes only the neutral slot). For automation, we should write the config file directly rather than script console input.
- `-onlyAI` runs all sides under AI; a headless mode exists (Stage 1 doc: "don't add `-headless` when you want to watch" — implying `--headless` is available). Exact flags to be confirmed in source.
- The log line `Creating battle AI <name>` confirms which AI actually loaded — the runner should assert on this to catch silent fallback to `CEmptyAI`.

## 4. Scope

**In scope**
- Designing and building the scenario battle setups (custom test maps and/or scripted army compositions).
- The launcher/runner: process management, config injection (AI slots, quick-combat off/headless on as appropriate), battle start, end detection, timeout watchdog.
- Result extraction: winner, surviving stacks/army value per side, rounds elapsed. Investigation of the cheapest reliable source (existing logs first; a minimal structured battle-end log hook in the engine only if necessary — pooled change, see Overview §6).
- Aggregation, scoring, storage of results; a simple text/CSV report. Baseline runs for the three reference AIs.
- Statistical repetition (N runs per scenario) to absorb randomness; optional deterministic-seed investigation.

**Out of scope**
- Improving any AI's play (that's SP-2 / SP-3 work).
- Adventure-map or full-game evaluation — battles only.
- GUI dashboards, plots, web UI (a CSV is enough for now).
- RL training loop integration (SP-3 will build on the same runner later).
- Large scenario libraries or ladder/ELO systems — keep the suite small and meaningful.

## 5. Design Sketch

### 5.1 Architecture
```
 arena.py (CLI)
   ├─ config writer: sets combat*AI keys, headless/quick-combat settings
   ├─ process runner: launches vcmiclient (-onlyAI [--headless]) on scenario map
   ├─ watchdog: kills a run that exceeds max wall-time (broken-AI guard)
   ├─ result reader: parses battle-end data (logs first; hook if needed)
   └─ scorer/reporter: aggregates N runs → JSON/CSV + summary table
```
The runner must be importable as a Python library too (`run_battle(scenario, ai_a, ai_b) -> Result`) — that is the exact function SP-2 and SP-3 will call.

### 5.2 Scenario suite (initial proposal)
1. **Mirror melee** — identical melee armies both sides (pure decision quality, no comp advantage).
2. **Mirror mixed** — identical armies with shooters + melee (tests shooter protection/targeting).
3. **Shooters vs rushers** — AI-under-test has shooters, opponent melee (kiting/blocking skill).
4. **Rushers vs shooters** — the inverse (closing distance, target priority).
5. **Underdog** — AI-under-test slightly weaker army; measures how much value it preserves even in loss.
6. **Overdog** — slightly stronger army; a good AI should win with minimal losses.
Obstacles/terrain variants and sieges: later additions, not v1. No hero spells in v1 scenarios (spell evaluation enters when SP-2 exposes spell actions).

### 5.3 Metrics (per battle → aggregated)
- **Win** (0/1) → win rate over N.
- **Army value retained** (surviving army strength ÷ starting strength, both sides) → average margin; this differentiates narrow vs crushing wins and graceful vs total losses.
- **Rounds taken** — efficiency tiebreaker.
- Composite scenario score = win rate primary, value-retained secondary; overall score = mean across scenarios. Keep the formula dumb and printed alongside raw numbers.

### 5.4 Handling randomness
Default: N = 20 runs per scenario per matchup; report mean ± spread. Deterministic seeding is an *investigation item*, not assumed (see Overview §6). Morale/luck could alternatively be neutralized at the scenario level (creature choice / map settings) — investigate which is cheaper.

## 6. Milestones & Definition of Done

- **A1 — Minimal runner (retires the key risk):** one scenario, launch → battle runs AI-vs-AI hands-off → winner detected programmatically → process exits cleanly. *Land this before deep SP-2 work.*
- **A2 — Metrics & repetition:** N-run loop, watchdog, army-value + rounds extraction, JSON/CSV output.
- **A3 — Scenario suite:** the 6 scenarios built and runnable via one command.
- **A4 — Baselines:** `StupidAI`, `MyRuleBasedAI`, `BattleAI` graded across the suite; results committed.

**Done when:** one command grades an arbitrary registered AI name across the full suite and prints its score next to the stored baselines, with no human interaction, in bounded time.

## 7. Open Questions / Investigation List

1. Exact launch flags & config keys for headless, only-AI, auto-starting a battle on load, and quick-combat behavior — confirm in current source. Does a battle auto-start from a map, or is a scripted trigger needed?
2. Can battle results be reliably parsed from existing logs, or do we need the minimal battle-end hook (pooled engine change)?
3. Does VCMI exit (or can it be made to exit) after a battle ends in headless mode, or does the runner terminate the process after result capture?
4. RNG seeding: does a seed option exist? If not, is statistical N enough (likely yes for v1)?
5. Scenario delivery: custom `.h3m` maps vs cheat-scripted armies on one generic map — which is more maintainable?

## 8. Status Log

- **2026-07-05** — Sub-project defined; absorbed old Stage 1 Phase 5 scope. Not started.
