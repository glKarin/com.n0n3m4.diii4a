// BotAI_Battle_Fight.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
=====================
rvmBot::state_BattleFight
=====================
*/
stateResult_t rvmBot::state_BattleFight(const stateParms_t& parms)
{
	int areanum;
	idVec3 target;
	idPlayer* entinfo;

	// respawn if dead.
	if( BotIsDead( &bs ) )
	{
		stateThread.SetState("state_Respawn");
		return SRESULT_DONE;
	}

	//if there is another better enemy
	if( BotFindEnemy( &bs, bs.enemy ) )
	{
		common->DPrintf( "found new better enemy\n" );
	}

	//if no enemy
	if( bs.enemy < 0 )
	{
		stateThread.SetState("state_SeekLTG");
		return SRESULT_DONE;
	}

	//BotEntityInfo(bs.enemy, &entinfo);
	entinfo = gameLocal.entities[bs.enemy]->Cast<idPlayer>();// &g_entities[bs.enemy];

	//if the enemy is dead
	if( bs.enemydeath_time )
	{
		if( bs.enemydeath_time < Bot_Time() - 1.0 )
		{
			bs.enemydeath_time = 0;
			if( bs.enemysuicide )
			{
				// jmarshall - bot chat
				//BotChat_EnemySuicide(bs);
				// jmarshall end
			}
			// jmarshall - bot chat stand
			//			if (bs.lastkilledplayer == bs.enemy && BotChat_Kill(bs)) {
			//				bs.stand_time = FloatTime() + BotChatTime(bs);
			//				AIEnter_Stand(bs, "battle fight: enemy dead");
			//			}
			//			else {
			bs.ltg_time = 0;
			//AIEnter_Seek_LTG(bs, "battle fight: enemy dead");
			stateThread.SetState("state_SeekLTG");
			//			}
			// jmarshall end
			return SRESULT_DONE;
		}
	}
	else
	{
		if( EntityIsDead( entinfo ) )
		{
			bs.enemydeath_time = Bot_Time();
		}
	}
// jmarshall - isinvisible
	//if the enemy is invisible and not shooting the bot looses track easily
	//if (entinfo->IsInvisible() && !entinfo->IsShooting()) {
	//	if (rvmBotUtil::random() < 0.2) {
	//		stateThread.SetState("state_SeekLTG");
	//		return;
	//	}
	//}
	//
	target = entinfo->GetOrigin();

	//update the reachability area and origin if possible
	//areanum = BotPointAreaNum(target);
	//if (areanum && trap_AAS_AreaReachability(areanum)) {
	//	VectorCopy(target, bs.lastenemyorigin);
	//	bs.lastenemyareanum = areanum;
	//}
//	bs.lastenemyareanum = areanum;

	//update the attack inventory values
	BotUpdateBattleInventory( &bs, bs.enemy );

	//if the bot's health decreased
// jmarshall - bot chat
	//if (bs.lastframe_health > bs.inventory[INVENTORY_HEALTH]) {
	//	if (BotChat_HitNoDeath(bs)) {
	//		bs.stand_time = FloatTime() + BotChatTime(bs);
	//		AIEnter_Stand(bs, "battle fight: chat health decreased");
	//		return qfalse;
	//	}
	//}

	//if the bot hit someone
	//if (bs.cur_ps.persistant[PERS_HITS] > bs.lasthitcount) {
	//	if (BotChat_HitNoKill(bs)) {
	//		bs.stand_time = FloatTime() + BotChatTime(bs);
	//		AIEnter_Stand(bs, "battle fight: chat hit someone");
	//		return qfalse;
	//	}
	//}
// jmarshall end

	//if the enemy is not visible
	if( !BotEntityVisible( bs.entitynum, bs.eye, bs.viewangles, 360, bs.enemy ) )
	{
		if( BotWantsToChase( &bs ) )
		{
			//AIEnter_Battle_Chase(bs, "battle fight: enemy out of sight");
			stateThread.SetState("state_Chase");
			return SRESULT_DONE;
		}
		else
		{
			//AIEnter_Seek_LTG(bs, "battle fight: enemy out of sight");
			stateThread.SetState("state_SeekLTG");
			return SRESULT_DONE;
		}
	}
	//use holdable items
	BotBattleUseItems( &bs );
	//
	//bs.tfl = TFL_DEFAULT;
	//if (bot_grapple.integer) bs.tfl |= TFL_GRAPPLEHOOK;
	////if in lava or slime the bot should be able to get out
	//if (BotInLavaOrSlime(bs)) bs.tfl |= TFL_LAVA | TFL_SLIME;
	////
	//if (BotCanAndWantsToRocketJump(bs)) {
	//	bs.tfl |= TFL_ROCKETJUMP;
	//}
	//choose the best weapon to fight with
	BotChooseWeapon( &bs );

	// Move randomly around our AAS area.
	BotMoveInRandomDirection(&bs);

	//aim at the enemy
	BotAimAtEnemy( &bs );

	//attack the enemy if possible
	BotCheckAttack( &bs );	

	//if the bot wants to retreat
	if( !( bs.flags & BFL_FIGHTSUICIDAL ) )
	{
		if( BotWantsToRetreat( &bs ) )
		{
			//AIEnter_Battle_Retreat(bs, "battle fight: wants to retreat");
			stateThread.SetState("state_Retreat");
			return SRESULT_DONE;
		}
	}

	return SRESULT_WAIT;
}
