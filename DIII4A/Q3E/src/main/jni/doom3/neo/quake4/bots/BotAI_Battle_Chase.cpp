// BotAI_Battle_Chase.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_Chase
=====================
*/
stateResult_t rvmBot::state_Chase(const stateParms_t& parms)
{
	bot_goal_t goal;
	idVec3 target, dir;
	//bot_moveresult_t moveresult;
	float range;

	//if (BotIsObserver(bs)) {
	//	AIEnter_Observer(bs, "battle chase: observer");
	//	return qfalse;
	//}
	//
	////if in the intermission
	//if (BotIntermission(bs)) {
	//	AIEnter_Intermission(bs, "battle chase: intermission");
	//	return qfalse;
	//}

	// respawn if dead.
	if( BotIsDead( &bs ) )
	{
		stateThread.SetState("state_Respawn");
		return SRESULT_DONE;
	}

	//if no enemy
	if( bs.enemy < 0 )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	//if the enemy is visible
	if( BotEntityVisibleTest( bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy, false ) )
	{
		//AIEnter_Battle_Fight(bs, "battle chase");
		stateThread.SetState("state_BattleFight");
		return SRESULT_DONE;
	}

	//if there is another enemy
	if( BotFindEnemy( &bs, -1 ) )
	{
		//AIEnter_Battle_Fight(bs, "battle chase: better enemy");
		//stateThread.SetState("state_BattleFight");
		stateThread.SetState("state_BattleFight");
		return SRESULT_DONE;
	}
	////there is no last enemy area
	//if (!bs.lastenemyareanum) {
	//	AIEnter_Seek_LTG(bs, "battle chase: no enemy area");
	//	return qfalse;
	//}
// jmarshall
	//
	//bs.tfl = TFL_DEFAULT;
	//if (bot_grapple.integer) bs.tfl |= TFL_GRAPPLEHOOK;
	////if in lava or slime the bot should be able to get out
	//if (BotInLavaOrSlime(bs)) bs.tfl |= TFL_LAVA | TFL_SLIME;
	////
	//if (BotCanAndWantsToRocketJump(bs)) {
	//	bs.tfl |= TFL_ROCKETJUMP;
	//}
	//map specific code
	//BotMapScripts(bs);
// jmarshall end

	//create the chase goal
	goal.entitynum = bs.enemy;
//	goal.areanum = bs.lastenemyareanum;
	VectorCopy( bs.lastenemyorigin, goal.origin );

	goal.mins = idMath::CreateVector( -8, -8, -8 );
	goal.maxs = idMath::CreateVector( 8, 8, 8 );

	// jmarshall - goal origin is last visible position
	{
		// Do a trace between the last_enemy_visible_position and the goal origin,
		// if for some reason we don't have line of sight to it, switch to LTG.
		trace_t trace;

		gameLocal.Trace( trace, bs.last_enemy_visible_position, gameLocal.entities[bs.client]->GetOrigin(), CONTENTS_SOLID, 0 );

		if( trace.fraction <= 0.9f )
		{
			//AIEnter_Seek_LTG(bs, "can't see last enemy position");
			stateThread.SetState("state_SeekLTG");
			return SRESULT_DONE;
		}
	}
	VectorCopy( bs.last_enemy_visible_position, goal.origin );
	// jmarshall end

	//if the last seen enemy spot is reached the enemy could not be found
	if( botGoalManager.BotTouchingGoal( bs.origin, &goal ) )
	{
		bs.chase_time = 0;
	}

	//if there's no chase time left
	if( !bs.chase_time || bs.chase_time < Bot_Time() - 10 )
	{
		//AIEnter_Seek_LTG(bs, "battle chase: time out");
		stateThread.SetState("state_SeekLTG");
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
			//the bot gets 5 seconds to pick up the nearby goal item
			bs.nbg_time = Bot_Time() + 0.1 * range + 1;
			//BotResetLastAvoidReach(bs.ms);
			//AIEnter_Battle_NBG(bs, "battle chase: nbg");
			stateThread.SetState("state_BattleNBG");
			return SRESULT_DONE;
		}
	}

	BotUpdateBattleInventory( &bs, bs.enemy );

	////initialize the movement state
	//BotSetupForMovement(bs);

	//move towards the goal
	//trap_BotMoveToGoal(&moveresult, bs.ms, &goal, bs.tfl);

	if( idMath::FRandRange( 0.0f, 4.0f ) <= 2.0f )
	{
		BotMoveToGoal( &bs, &goal );
	}
	else
	{
		BotMoveInRandomDirection( &bs );
	}


	//if the movement failed
	//if (moveresult.failure) {
	//	//reset the avoid reach, otherwise bot is stuck in current area
	//	trap_BotResetAvoidReach(bs.ms);
	//	//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
	//	bs.ltg_time = 0;
	//}
	////
	//BotAIBlocked(bs, &moveresult, qfalse);
	//
	//if (moveresult.flags & (MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)) {
	//	VectorCopy(moveresult.ideal_viewangles, bs.ideal_viewangles);
	//}

	if( !( bs.flags & BFL_IDEALVIEWSET ) )
	{
		BotAimAtEnemy( &bs );
		// jmarshall
		//if (bs.chase_time > FloatTime() - 2) {
		//	BotAimAtEnemy(bs);
		//}
		//else {
		//	if (BotMovementViewTarget(bs.ms, &goal, bs.tfl, 300, target)) {
		//		VectorSubtract(target, bs.origin, dir);
		//		vectoangles(dir, bs.viewangles);
		//	}
		//	//else {
		//	//	vectoangles(moveresult.movedir, bs.ideal_viewangles);
		//	//}
		//}
		// jmarshall end
//		bs.ideal_viewangles[2] *= 0.5; // jmarshall <-- view angles!
	}

	if (BotEntityVisible(bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy))
	{
		//attack the enemy if possible
		BotCheckAttack(&bs);
	}

	//if the weapon is used for the bot movement
	//if (moveresult.flags & MOVERESULT_MOVEMENTWEAPON) bs.weaponnum = moveresult.weapon;
	//if the bot is in the area the enemy was last seen in
	//if (bs.areanum == bs.lastenemyareanum) bs.chase_time = 0;
	//if the bot wants to retreat (the bot could have been damage during the chase)
	if( BotWantsToRetreat( &bs ) )
	{
		//AIEnter_Battle_Retreat(bs, "battle chase: wants to retreat");
		stateThread.SetState("state_Retreat");
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}
