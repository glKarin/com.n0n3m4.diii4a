// BotAI_SeekLTG.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_SeekLTG
=====================
*/
stateResult_t rvmBot::state_SeekLTG(const stateParms_t& parms)
{
	if( BotIsDead( &bs ) )
	{
		stateThread.SetState("state_Respawn");
		return SRESULT_DONE;
	}

	BotGetItemLongTermGoal( &bs, 0, &bs.currentGoal );

	// No Enemy.
	bs.enemy = -1;

	//if there is an enemy
	if( BotFindEnemy( &bs, -1 ) )
	{
		if( BotWantsToRetreat( &bs ) )
		{
			//keep the current long term goal and retreat
			//AIEnter_Battle_Retreat(bs, "seek ltg: found enemy");
			stateThread.SetState("state_Retreat");
			return SRESULT_DONE;
		}
		else
		{
			//trap_BotResetLastAvoidReach(bs.ms);
			//empty the goal stack
			botGoalManager.BotEmptyGoalStack( bs.gs );

			//go fight
			stateThread.SetState("state_BattleFight");
			return SRESULT_DONE;
		}
	}

	return SRESULT_WAIT;
}
