/*
 * ArmageddonAI.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ArmageddonAI.h"
#include "../../lib/CStack.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CBattleCallback.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/constants/Enumerations.h"
#include <vcmi/spells/Magic.h>

CArmageddonAI::CArmageddonAI()
	: side(BattleSide::NONE)
	, wasWaitingForRealize(false)
{
	print("created");
}

CArmageddonAI::~CArmageddonAI()
{
	print("destroyed");
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI
		cb->waitTillRealize = wasWaitingForRealize;
	}
}

void CArmageddonAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	print("init called, saving ptr to IBattleCallback");
	env = ENV;
	cb = CB;

	wasWaitingForRealize = CB->waitTillRealize;
	CB->waitTillRealize = false;
}

void CArmageddonAI::initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences)
{
	initBattleInterface(ENV, CB);
}

void CArmageddonAI::yourTacticPhase(const BattleID & battleID, int distance)
{
	// We don't use the tactics phase - just end it immediately (same as StupidAI).
	cb->battleMakeTacticAction(battleID, BattleAction::makeEndOFTacticPhase(cb->getBattle(battleID)->battleGetTacticsSide()));
}

void CArmageddonAI::activeStack(const BattleID & battleID, const CStack * stack)
{
	print("activeStack called for " + stack->nodeName());

	// "The pyromaniac": the hero's Armageddon is the only thing this AI does.
	// Fires whenever the hero is allowed to cast.
	if(tryCastArmageddon(battleID))
		return;

	// Otherwise the stack does nothing. A unit must still submit *some* valid action
	// every turn or the battle hangs waiting on it, so "do nothing" = defend in place.
	print(stack->nodeName() + ": nothing to do, defending");
	cb->battleMakeUnitAction(battleID, BattleAction::makeDefend(stack));
}

bool CArmageddonAI::tryCastArmageddon(const BattleID & battleID) const
{
	// If our hero can cast Armageddon right now, do it - no evaluation, no mercy.
	// Armageddon is a battlefield-wide fire nuke that also hits our own
	// (non-fire-immune) troops. That indiscriminate boom is the whole point.
	const auto battle = cb->getBattle(battleID);
	const CGHeroInstance * hero = battle->battleGetMyHero();
	if(!hero)
		return false; // no hero on our side -> nobody to cast

	// General gate: is the hero allowed to cast at all this turn? (has mana, hasn't
	// already cast this round, not silenced, ...)
	if(battle->battleCanCastSpell(hero, spells::Mode::HERO) != ESpellCastProblem::OK)
		return false;

	// Spell-specific gate: does the hero actually know Armageddon and can it be cast now?
	const CSpell * armageddon = SpellID(SpellID::ARMAGEDDON).toSpell();
	if(!armageddon || !armageddon->canBeCast(battle.get(), spells::Mode::HERO, hero))
		return false;

	// Armageddon has no destination (AimType::NOTHING) -> leave the action's target empty.
	BattleAction spellcast;
	spellcast.actionType = EActionType::HERO_SPELL;
	spellcast.spell = SpellID::ARMAGEDDON;
	spellcast.side = side;
	spellcast.stackNumber = -1; // the hero, not a stack

	print("hero: casting Armageddon");
	cb->battleMakeSpellAction(battleID, spellcast);
	return true;
}

void CArmageddonAI::battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, BattleSide Side, bool replayAllowed)
{
	print("battleStart called");
	side = Side;
}

void CArmageddonAI::print(const std::string & text) const
{
	logAi->trace("CArmageddonAI [%p]: %s", this, text);
}
