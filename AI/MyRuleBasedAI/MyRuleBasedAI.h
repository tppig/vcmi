/*
 * MyRuleBasedAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/callback/CBattleGameInterface.h"

// Stage 1 / Phase 3.5 — the "Random Mover" (v0).
// Deliberately silly: it does not attack or seek the enemy. Each turn it just
// walks to a random reachable hex. Its only job is to prove that our own code
// is driving the units end-to-end (factory -> activeStack -> callback -> engine).
// Phase 4 will replace the body of activeStack() with the real rule ladder.
class CMyRuleBasedAI : public CBattleGameInterface
{
	BattleSide side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	bool wasWaitingForRealize;

	void print(const std::string & text) const;
public:
	CMyRuleBasedAI();
	~CMyRuleBasedAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences) override;

	void activeStack(const BattleID & battleID, const CStack * stack) override; //called when it's turn of that stack
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide side, bool replayAllowed) override;
};
