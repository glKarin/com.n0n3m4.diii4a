// BotAI_Battle_Attacked.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
==================
rvmBot::state_Attacked
==================
*/
stateResult_t rvmBot::state_Attacked(const stateParms_t& parms) {
	// respawn if dead.
	if (BotIsDead(&bs))
	{
		stateThread.SetState("state_Respawn");
		return SRESULT_DONE;
	}

	if (gameLocal.SysScriptTime() > bs.aggressiveAttackTime || bs.weaponnum == 0) {
		stateThread.SetState("state_Retreat");
		return SRESULT_DONE;
	}

	// Ensure the target is a player.
	idPlayer *entinfo = gameLocal.entities[bs.enemy]->Cast<idPlayer>();
	if (!entinfo)
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	// If our enemy is dead, search for another LTG.
	if (EntityIsDead(entinfo))
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	bs.currentGoal.origin = bs.lastenemyorigin;

	//aim at the enemy
	BotAimAtEnemy(&bs);

	//attack the enemy if possible
	BotCheckAttack(&bs);

	return SRESULT_WAIT;
}
