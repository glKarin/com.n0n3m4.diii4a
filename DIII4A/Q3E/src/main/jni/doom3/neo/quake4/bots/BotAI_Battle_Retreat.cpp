// BotAI_Battle_Retreat.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_Retreat
=====================
*/
stateResult_t rvmBot::state_Retreat(const stateParms_t& parms)
{
	bot_goal_t goal;
	idPlayer* entinfo;
	rvmBot* owner;
	idVec3 target, dir;
	float attack_skill, range;

	// respawn if dead.
	if( BotIsDead( &bs ) )
	{
		stateThread.SetState("state_Respawn");
		return SRESULT_DONE;
	}

	// if no enemy.
	if( bs.enemy < 0 )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	// Ensure the target is a player.
	entinfo = gameLocal.entities[bs.enemy]->Cast<idPlayer>();
	if( !entinfo )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	owner = gameLocal.entities[bs.entitynum]->Cast<rvmBot>();

	// If our enemy is dead, search for another LTG.
	if( EntityIsDead( entinfo ) )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	//if there is another better enemy
	if( BotFindEnemy( &bs, bs.enemy ) )
	{
		common->DPrintf( "found new better enemy\n" );
	}

	//update the attack inventory values
	BotUpdateBattleInventory( &bs, bs.enemy );

	//if the bot doesn't want to retreat anymore... probably picked up some nice items
	if( BotWantsToChase( &bs ) )
	{
		//empty the goal stack, when chasing, only the enemy is the goal
		botGoalManager.BotEmptyGoalStack( bs.gs );

		//go chase the enemy
		//AIEnter_Battle_Chase(bs, "battle retreat: wants to chase");
		stateThread.SetState("state_Chase");
		return SRESULT_DONE;
	}

	//update the last time the enemy was visible
	if( BotEntityVisible( bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy ) )
	{
		bs.enemyvisible_time = Bot_Time();
		target = entinfo->GetOrigin();
		bs.lastenemyorigin = target;
	}

	//if the enemy is NOT visible for 4 seconds
	if( bs.enemyvisible_time < Bot_Time() - 4 )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}
	//else if the enemy is NOT visible
	else if( bs.enemyvisible_time < Bot_Time() )
	{
		//if there is another enemy
		if( BotFindEnemy( &bs, -1 ) )
		{
			//AIEnter_Battle_Fight(bs, "battle retreat: another enemy");
			stateThread.SetState("state_BattleFight");
			return SRESULT_DONE;
		}
	}

	//use holdable items
	BotBattleUseItems( &bs );

	//get the current long term goal while retreating
	if( !BotGetItemLongTermGoal( &bs, 0, &bs.currentGoal ) )
	{
		//AIEnter_Battle_SuicidalFight(bs, "battle retreat: no way out");
		stateThread.SetState("state_BattleFight");
		bs.flags |= BFL_FIGHTSUICIDAL;
		return SRESULT_DONE;
	}

	//check for nearby goals periodicly
	if( bs.check_time < Bot_Time() )
	{
		bs.check_time = Bot_Time() + 1;
		range = 150;

		//
		if( BotNearbyGoal( &bs, 0, &goal, range ) )
		{
			//trap_BotResetLastAvoidReach(bs.ms);
			//time the bot gets to pick up the nearby goal item
			bs.nbg_time = Bot_Time() + range / 100 + 1;
			//AIEnter_Battle_NBG(bs, "battle retreat: nbg");
			stateThread.SetState("state_BattleNBG");
			return SRESULT_DONE;
		}
	}

	MoveToCoverPoint();

	if (bot_skill.GetInteger() > 1)
	{
		bs.firethrottlewait_time = 0;
	}

	BotChooseWeapon( &bs );

	BotAimAtEnemy( &bs );

	//attack the enemy if possible
	BotCheckAttack( &bs );

	return SRESULT_WAIT;
}
