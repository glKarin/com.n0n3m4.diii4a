// BotAI_Seek_NBG.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_SeekNBG
=====================
*/
stateResult_t rvmBot::state_SeekNBG(const stateParms_t& parms)
{
	bot_goal_t goal;
	idVec3 target, dir;
	//bot_moveresult_t moveresult;

	//if (BotIsObserver(bs)) {
	//	AIEnter_Observer(bs, "seek nbg: observer");
	//	return qfalse;
	//}
	////if in the intermission
	//if (BotIntermission(bs)) {
	//	AIEnter_Intermission(bs, "seek nbg: intermision");
	//	return qfalse;
	//}

	// respawn if dead.
	if( BotIsDead( &bs ) )
	{
		stateThread.SetState("state_Respawn");
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
	//no enemy
	bs.enemy = -1;
	//if the bot has no goal
	if( !botGoalManager.BotGetTopGoal( bs.gs, &goal ) )
	{
		bs.nbg_time = 0;
	}
	//if the bot touches the current goal
	else if( BotReachedGoal( &bs, &goal ) )
	{
		BotChooseWeapon( &bs );
		bs.nbg_time = 0;
	}

	if( bs.nbg_time < Bot_Time() )
	{
		//pop the current goal from the stack
		botGoalManager.BotPopGoal( bs.gs );
		//check for new nearby items right away
		//NOTE: we canNOT reset the check_time to zero because it would create an endless loop of node switches
		bs.check_time = Bot_Time() + 0.05;
		//go back to seek ltg
//		AIEnter_Seek_LTG(bs, "seek nbg: time out");
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	//predict obstacles
	//if (BotAIPredictObstacles(bs, &goal))
	//	return qfalse;
	////initialize the movement state
	//BotSetupForMovement(bs);
	////move towards the goal
	//trap_BotMoveToGoal(&moveresult, bs.ms, &goal, bs.tfl);
	////if the movement failed
	//if (moveresult.failure) {
	//	//reset the avoid reach, otherwise bot is stuck in current area
	//	trap_BotResetAvoidReach(bs.ms);
	//	bs.nbg_time = 0;
	//}
	BotMoveToGoal( &bs, &goal );

	//check if the bot is blocked
	//BotAIBlocked(bs, &moveresult, qtrue);
	////
	//BotClearPath(bs, &moveresult);

// jmarshall - fix look at code.
	//if the viewangles are used for the movement
	//if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
	//	VectorCopy(moveresult.ideal_viewangles, bs.ideal_viewangles);
	//}
	////if waiting for something
	//else if (moveresult.flags & MOVERESULT_WAITING) {
	//	if (random() < bs.thinktime * 0.8) {
	//		BotRoamGoal(bs, target);
	//		VectorSubtract(target, bs.origin, dir);
	//		vectoangles(dir, bs.ideal_viewangles);
	//		bs.ideal_viewangles[2] *= 0.5;
	//	}
	//}
	//else if (!(bs.flags & BFL_IDEALVIEWSET)) {
	//	if (!trap_BotGetSecondGoal(bs.gs, &goal)) trap_BotGetTopGoal(bs.gs, &goal);
	//	if (trap_BotMovementViewTarget(bs.ms, &goal, bs.tfl, 300, target)) {
	//		VectorSubtract(target, bs.origin, dir);
	//		vectoangles(dir, bs.ideal_viewangles);
	//	}
	//	//FIXME: look at cluster portals?
	//	else vectoangles(moveresult.movedir, bs.ideal_viewangles);
	//	bs.ideal_viewangles[2] *= 0.5;
	//}
	////if the weapon is used for the bot movement
	//if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs.weaponnum = moveresult.weapon;
// jmarshall end
	//if there is an enemy
	if( BotFindEnemy( &bs, -1 ) )
	{
		if( BotWantsToRetreat( &bs ) )
		{
			//keep the current long term goal and retreat
			//AIEnter_Battle_NBG(bs, "seek nbg: found enemy");
			stateThread.SetState("state_BattleNBG");
			return SRESULT_DONE;
		}
		else
		{
			//trap_BotResetLastAvoidReach(bs.ms);
			//empty the goal stack
			botGoalManager.BotEmptyGoalStack( bs.gs );
			//go fight
			//AIEnter_Battle_Fight(bs, "seek nbg: found enemy");
			stateThread.SetState("state_BattleFight");
			return SRESULT_DONE;
		}
	}
	return SRESULT_WAIT;
}
