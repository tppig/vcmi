/*
 * MyRuleBasedAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MyRuleBasedAI.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/BattleHex.h"
#include "../../lib/battle/BattleHexArray.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/CRandomGenerator.h"

CMyRuleBasedAI::CMyRuleBasedAI()
	: side(BattleSide::NONE)
	, wasWaitingForRealize(false)
{
	print("created");
}

CMyRuleBasedAI::~CMyRuleBasedAI()
{
	print("destroyed");
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI
		cb->waitTillRealize = wasWaitingForRealize;
	}
}

void CMyRuleBasedAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	print("init called, saving ptr to IBattleCallback");
	env = ENV;
	cb = CB;

	wasWaitingForRealize = CB->waitTillRealize;
	CB->waitTillRealize = false;
}

void CMyRuleBasedAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences)
{
	initBattleInterface(ENV, CB);
}

void CMyRuleBasedAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	// We don't use the tactics phase - just end it immediately (same as StupidAI).
	cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(cb->getBattle(battleID)->battleGetTacticsSide()));
}

void CMyRuleBasedAI::activeStack(const BattleID & battleID, const CStack * stack)
{
	print("activeStack called for " + stack->nodeName());

	// Random Mover v0 ("the drunkard"): ignore all enemies, just wander to a
	// random reachable hex. No attacking, no enemy-seeking - on purpose.
	BattleHexArray availableHexes = cb->getBattle(battleID)->battleGetAvailableHexes(stack, false);

	// Drop hexes the stack already occupies so a pick always results in a real move.
	BattleHexArray moveTargets;
	for(const BattleHex & hex : availableHexes)
		if(!stack->coversPos(hex))
			moveTargets.insert(hex);

	if(moveTargets.empty())
	{
		// Stall-guard: a fully blocked unit (or a siege weapon, which can't walk)
		// still MUST submit some valid action, or the battle hangs waiting on it.
		// This is the only fallback allowed in v0 - it is a safety net, not a decision.
		print(stack->nodeName() + ": no move available, defending");
		cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
		return;
	}

	const BattleHex destination = *RandomGeneratorUtil::nextItem(moveTargets, CRandomGenerator::getDefault());
	print(stack->nodeName() + ": random-moving to hex " + std::to_string(destination.toInt()));
	cb->battleMakeUnitAction(battleID, BattleAction::makeMove(stack, destination));
}

void CMyRuleBasedAI::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide Side, bool replayAllowed)
{
	print("battleStart called");
	side = Side;
}

void CMyRuleBasedAI::print(const std::string & text) const
{
	logAi->trace("CMyRuleBasedAI [%p]: %s", this, text);
}
