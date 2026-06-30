/*
 * ArmageddonAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/callback/CBattleGameInterface.h"

// Stage 1 / Phase 3.6 — the "Armageddon AI" (a.k.a. "the pyromaniac").
// This AI does exactly one thing: at the start of every turn, if our hero CAN cast
// Armageddon, it does — a global fire nuke that also burns our own non-fire-immune
// troops. That indiscriminate boom is the whole point. When it can't cast, the stack
// simply does nothing (defends in place); it never moves. Kept as its own module so
// it can be switched in at runtime via setBattleAI without rebuilding.
class CArmageddonAI : public CBattleGameInterface
{
	BattleSide side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	bool wasWaitingForRealize;

	void print(const std::string & text) const;

	// If our hero can cast Armageddon right now, do it.
	// Returns true if a spell action was submitted (so activeStack should return).
	bool tryCastArmageddon(const BattleID & battleID) const;
public:
	CArmageddonAI();
	~CArmageddonAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences) override;

	void activeStack(const BattleID & battleID, const CStack * stack) override; //called when it's turn of that stack
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) override;
};
