// BotAI_Battle_NBG.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_BattleNBG
=====================
*/
stateResult_t rvmBot::state_BattleNBG(const stateParms_t& parms)
{
	int areanum;
	bot_goal_t goal;
	//aas_entityinfo_t entinfo;
	idEntity* entinfo;
	//bot_moveresult_t moveresult;
	float attack_skill;
	idVec3 target, dir;

	//if (BotIsObserver(bs)) {
	//	AIEnter_Observer(bs, "battle nbg: observer");
	//	return qfalse;
	//}
	////if in the intermission
	//if (BotIntermission(bs)) {
	//	AIEnter_Intermission(bs, "battle nbg: intermission");
	//	return qfalse;
	//}

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

	//BotEntityInfo(bs.enemy, &entinfo);
	entinfo = gameLocal.entities[bs.enemy]->Cast<idPlayer>();
	if( entinfo->health <= 0 )
	{
		//AIEnter_Seek_NBG(bs, "battle nbg: enemy dead");
		stateThread.SetState("state_SeekNBG");
		return SRESULT_DONE;
	}

	//bs.tfl = TFL_DEFAULT;
	//if (bot_grapple.integer) bs.tfl |= TFL_GRAPPLEHOOK;
	////if in lava or slime the bot should be able to get out
	//if (BotInLavaOrSlime(bs)) bs.tfl |= TFL_LAVA | TFL_SLIME;
	////
	//if (BotCanAndWantsToRocketJump(bs)) {
	//	bs.tfl |= TFL_ROCKETJUMP;
	//}
	////map specific code
	//BotMapScripts(bs);

	//update the last time the enemy was visible
	if( BotEntityVisible( bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy ) )
	{
		bs.enemyvisible_time = Bot_Time();
		//VectorCopy(entinfo->GetOrigin(), target);
		target = entinfo->GetOrigin();
		// if not a player enemy
		if( bs.enemy >= MAX_CLIENTS )
		{
#ifdef MISSIONPACK
			// if attacking an obelisk
			if( bs.enemy == redobelisk.entitynum ||
					bs.enemy == blueobelisk.entitynum )
			{
				target[2] += 16;
			}
#endif
		}
		//update the reachability area and origin if possible
		//areanum = BotPointAreaNum(target);
		//if (areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy( target, bs.lastenemyorigin );
		//	bs.lastenemyareanum = areanum;
		//}
	}

	//if the bot has no goal or touches the current goal
	if( !botGoalManager.BotGetTopGoal( bs.gs, &goal ) )
	{
		bs.nbg_time = 0;
	}
	else if( BotReachedGoal( &bs, &goal ) )
	{
		bs.nbg_time = 0;
	}
	//
	if( bs.nbg_time < Bot_Time() )
	{
		//pop the current goal from the stack
		botGoalManager.BotPopGoal( bs.gs );
		//if the bot still has a goal
		if( botGoalManager.BotGetTopGoal( bs.gs, &goal ) )
		{
			//AIEnter_Battle_Retreat(bs, "battle nbg: time out");
			stateThread.SetState("state_Retreat");
		}
		else
		{
			//AIEnter_Battle_Fight(bs, "battle nbg: time out");
			stateThread.SetState("state_BattleFight");
		}

		return SRESULT_DONE;
	}

	//move towards the goal
	BotMoveToGoal( &bs, &goal );

	//initialize the movement state
	//BotSetupForMovement(bs);
	////move towards the goal
	//trap_BotMoveToGoal(&moveresult, bs.ms, &goal, bs.tfl);
	////if the movement failed
	//if (moveresult.failure) {
	//	//reset the avoid reach, otherwise bot is stuck in current area
	//	trap_BotResetAvoidReach(bs.ms);
	//	//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
	//	bs.nbg_time = 0;
	//}
	////
	//BotAIBlocked(bs, &moveresult, qfalse);

	//update the attack inventory values
	BotUpdateBattleInventory( &bs, bs.enemy );

	//choose the best weapon to fight with
	BotChooseWeapon( &bs );

	//if the view is fixed for the movement
	//if (moveresult.flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
	//	VectorCopy(moveresult.ideal_viewangles, bs.ideal_viewangles);
	//}
	//else if (!(moveresult.flags & MOVERESULT_MOVEMENTVIEWSET)
	//	&& !(bs.flags & BFL_IDEALVIEWSET)) {
	//	attack_skill = trap_Characteristic_BFloat(bs.character, CHARACTERISTIC_ATTACK_SKILL, 0, 1);
	//	//if the bot is skilled anough and the enemy is visible
	//	if (attack_skill > 0.3) {
	//		//&& BotEntityVisible(bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy)
	//		BotAimAtEnemy(bs);
	//	}
	//	else {
	//		if (trap_BotMovementViewTarget(bs.ms, &goal, bs.tfl, 300, target)) {
	//			VectorSubtract(target, bs.origin, dir);
	//			vectoangles(dir, bs.ideal_viewangles);
	//		}
	//		else {
	//			vectoangles(moveresult.movedir, bs.ideal_viewangles);
	//		}
	//		bs.ideal_viewangles[2] *= 0.5;
	//	}
	//}
	//if (attack_skill > 0.3) {
	//&& BotEntityVisible(bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy)
	BotAimAtEnemy( &bs );
	//}

	//if the weapon is used for the bot movement
	//if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs.weaponnum = moveresult.weapon;
	//attack the enemy if possible
	BotCheckAttack( &bs );
	return SRESULT_WAIT;
}
