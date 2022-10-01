// BotAI.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../ai/AASCallback_FindCoverArea.h"

#define IDEAL_ATTACKDIST			140
#define MASK_SHOT	MASK_SHOT_BOUNDINGBOX

int	rvmBot::WP_MACHINEGUN = -1;
int	rvmBot::WP_SHOTGUN = -1;
int	rvmBot::WP_PLASMAGUN = -1;
int	rvmBot::WP_ROCKET_LAUNCHER = -1;

/*
=========================
rvmBot::BotIsDead
=========================
*/
bool rvmBot::BotIsDead( bot_state_t* bs )
{
	idPlayer* player = gameLocal.GetClientByNum( bs->client );
	if( player->health <= 0 )
	{
		return true;
	}

	return false;
}

/*
==================
rvmBot::BotReachedGoal
==================
*/
bool rvmBot::BotReachedGoal( bot_state_t* bs, bot_goal_t* goal )
{
	if( goal->flags & GFL_ITEM )
	{
		//if touching the goal
		if( botGoalManager.BotTouchingGoal( bs->origin, goal ) )
		{
			if( !( goal->flags & GFL_DROPPED ) )
			{
				botGoalManager.BotSetAvoidGoalTime( bs->gs, goal->number, -1 );
			}
			return true;
		}
		//if the goal isn't there
		if( botGoalManager.BotItemGoalInVisButNotVisible( bs->entitynum, bs->eye, bs->viewangles, goal ) )
		{
			return true;
		}
	}
	else
	{
		//if touching the goal
		if( botGoalManager.BotTouchingGoal( bs->origin, goal ) )
		{
			return true;
		}
	}
	return false;
}

/*
==================
rvmBot::BotChooseWeapon
==================
*/
void rvmBot::BotChooseWeapon( bot_state_t* bs )
{
	int newweaponnum;

	//if (bs->cur_ps.weaponstate == WEAPON_RAISING ||
	//	bs->cur_ps.weaponstate == WEAPON_DROPPING) {
	//	//trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	//	bs->input.weapon = bs->weaponnum;
	//}
	//else {
	newweaponnum = botWeaponInfoManager.BotChooseBestFightWeapon( bs->ws, bs->inventory );
	if( bs->weaponnum != newweaponnum )
	{
		bs->weaponchange_time = Bot_Time();
	}
	bs->weaponnum = newweaponnum;
	//BotAI_Print(PRT_MESSAGE, "bs->weaponnum = %d\n", bs->weaponnum);
	//trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	bs->botinput.weapon = bs->weaponnum;
	//}
}


/*
==================
rvmBot::BotGetItemLongTermGoal
==================
*/
int rvmBot::BotGetItemLongTermGoal( bot_state_t* bs, int tfl, bot_goal_t* goal )
{
	//if the bot has no goal
	if( !botGoalManager.BotGetTopGoal( bs->gs, goal ) )
	{
		//BotAI_Print(PRT_MESSAGE, "no ltg on stack\n");
		bs->ltg_time = 0;
	}
	//if the bot touches the current goal
	else if( BotReachedGoal( bs, goal ) )
	{
		BotChooseWeapon( bs );
		bs->ltg_time = 0;
	}

	// Check to see that we can get to our goal, if not get a new goal.
	//if (bs->numMovementWaypoints > 0)
	//{
	//	trace_t tr;
	//	gentity_t* ent = &g_entities[bs->client];
	//	vec3_t waypoint;
	//
	//	VectorCopy(bs->movement_waypoints[bs->currentWaypoint], waypoint);
	//	waypoint[2] += 5.0f;
	//
	//	trap_Trace(&tr, ent->r.currentOrigin, NULL, NULL, waypoint, bs->client, CONTENTS_SOLID);
	//
	//	if (tr.fraction <= 0.7f)
	//	{
	//		BotChooseWeapon(bs);
	//		bs->ltg_time = 0;
	//	}
	//}

	//if it is time to find a new long term goal
	if( bs->ltg_time == 0 )
	{
		//pop the current goal from the stack
		botGoalManager.BotPopGoal( bs->gs );
		//BotAI_Print(PRT_MESSAGE, "%s: choosing new ltg\n", ClientName(bs->client, netname, sizeof(netname)));
		//choose a new goal
		//BotAI_Print(PRT_MESSAGE, "%6.1f client %d: BotChooseLTGItem\n", Bot_Time(), bs->client);
		if( botGoalManager.BotChooseLTGItem( bs->gs, bs->origin, bs->inventory, tfl ) )
		{
			char buf[128];
			//get the goal at the top of the stack
			botGoalManager.BotGetTopGoal( bs->gs, goal );
			botGoalManager.BotGoalName( goal->number, buf, sizeof( buf ) );
			common->DPrintf( "%1.1f: new long term goal %s\n", Bot_Time(), buf );

			bs->ltg_time = Bot_Time() + 20;
			bs->currentGoal.framenum = gameLocal.framenum;
		}
		else  //the bot gets sorta stuck with all the avoid timings, shouldn't happen though
		{
			//
			//trap_BotDumpAvoidGoals(bs->gs);
			//reset the avoid goals and the avoid reach
			botGoalManager.BotResetAvoidGoals( bs->gs );
			//BotResetAvoidReach(bs->ms);
		}
		//get the goal at the top of the stack
		if( !botGoalManager.BotGetTopGoal( bs->gs, goal ) )
		{
			return false;
		}

		bs->currentGoal.framenum = gameLocal.framenum;

		return true;
	}
	return true;
}

/*
==================
rvmBot::EntityIsDead
==================
*/
bool rvmBot::EntityIsDead( idEntity* entity )
{
	{
		idPlayer* player = entity->Cast<idPlayer>();
		if( player && player->health <= 0 )
		{
			return true;
		}
	}
	return false;
}

/*
==================
BotEntityVisibleTest

returns visibility in the range [0, 1] taking fog and water surfaces into account
==================
*/
float rvmBot::BotEntityVisibleTest( int viewer, idVec3 eye, idAngles viewangles, float fov, int ent, bool allowHeightTest )
{
	int i, contents_mask, passent, hitent, infog, inwater, otherinfog, pc;
	float squaredfogdist, waterfactor, vis, bestvis;
	trace_t trace;
	idEntity* entinfo;
	idVec3 dir, start, end, middle;
	idAngles entangles;
	idPlayer* viewEnt;

	//calculate middle of bounding box
	//BotEntityInfo(ent, &entinfo);
	entinfo = gameLocal.entities[ent];
	viewEnt = gameLocal.entities[viewer]->Cast<idPlayer>();

	//VectorAdd(entinfo->r.mins, entinfo->r.maxs, middle);
	//VectorScale(middle, 0.5, middle);
	//VectorAdd(entinfo->GetOrigin(), middle, middle);
	middle = entinfo->GetPhysics()->GetBounds().GetCenter();
	middle += entinfo->GetOrigin();

	//check if entity is within field of vision
	//VectorSubtract(middle, eye, dir);
	dir = middle - eye;
	entangles = dir.ToAngles();

	if (!viewEnt->CheckFOV(entinfo->GetOrigin()))
	{
		return 0;
	}

	pc = gameLocal.clip[0]->PointContents( eye );
	infog = ( pc & CONTENTS_FOG );
	inwater = ( pc & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) );
	bestvis = 0;
	for( i = 0; i < 3; i++ )
	{
		//if the point is not in potential visible sight
		//if (!AAS_inPVS(eye, middle)) continue;
		//
		contents_mask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
		passent = viewer;
		hitent = ent;
		//VectorCopy(eye, start);
		start = eye;
		//VectorCopy(middle, end);
		end = middle;
		//if the entity is in water, lava or slime
		if(gameLocal.clip[0]->PointContents( middle ) & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) )
		{
			contents_mask |= ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
		}
		//if eye is in water, lava or slime
		if( inwater )
		{
			if( !( contents_mask & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) ) )
			{
				passent = ent;
				hitent = viewer;
				VectorCopy( middle, start );
				VectorCopy( eye, end );
			}
			contents_mask ^= ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
		}

		//trace from start to end
		gameLocal.Trace( trace, start, end, contents_mask, passent );
		// jmarshall
		if( trace.fraction < 0.9f && allowHeightTest )
		{
			end[2] += 50.0f;
			gameLocal.Trace( trace, start, end, contents_mask, passent );
		}
		// jmarshall end

		//if water was hit
		waterfactor = 1.0;
		if( trace.c.contents & ( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER ) )
		{
			//if the water surface is translucent
			if( 1 )
			{
				//trace through the water
				contents_mask &= ~( CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER );
				//trap_Trace(&trace, trace.endpos, NULL, NULL, end, passent, contents_mask);
				gameLocal.Trace( trace, trace.endpos, end, contents_mask, passent );
				waterfactor = 0.5;
			}
		}

		//if a full trace or the hitent was hit
		if( trace.fraction >= 1 || trace.c.entityNum == hitent )
		{
			//check for fog, assuming there's only one fog brush where
			//either the viewer or the entity is in or both are in
			otherinfog = ( gameLocal.clip[0]->PointContents( middle ) & CONTENTS_FOG );
			if( infog && otherinfog )
			{
				VectorSubtract( trace.endpos, eye, dir );
				squaredfogdist = dir.LengthSqr();
			}
			else if( infog )
			{
				VectorCopy( trace.endpos, start );
				//trap_Trace(&trace, start, NULL, NULL, eye, viewer, CONTENTS_FOG);
				gameLocal.Trace( trace, start, eye, CONTENTS_FOG, viewer );
				VectorSubtract( eye, trace.endpos, dir );
				squaredfogdist = dir.LengthSqr();
			}
			else if( otherinfog )
			{
				VectorCopy( trace.endpos, end );
				//trap_Trace(&trace, eye, NULL, NULL, end, viewer, CONTENTS_FOG);
				gameLocal.Trace( trace, eye, end, CONTENTS_FOG, viewer );
				VectorSubtract( end, trace.endpos, dir );
				squaredfogdist = dir.LengthSqr();
			}
			else
			{
				//if the entity and the viewer are not in fog assume there's no fog in between
				squaredfogdist = 0;
			}
			//decrease visibility with the view distance through fog
			vis = 1 / ( ( squaredfogdist * 0.001 ) < 1 ? 1 : ( squaredfogdist * 0.001 ) );

			//if entering water visibility is reduced
			vis *= waterfactor;

			if( vis > bestvis )
			{
				bestvis = vis;
			}

			//if pretty much no fog
			if( bestvis >= 0.95 )
			{
				return bestvis;
			}
		}
		//check bottom and top of bounding box as well
		if( i == 0 )
		{
			middle[2] += entinfo->GetPhysics()->GetBounds()[0][2];    // r.mins[2];
		}
		else if( i == 1 )
		{
			middle[2] += entinfo->GetPhysics()->GetBounds()[1][2] - entinfo->GetPhysics()->GetBounds()[0][2];    //entinfo->r.maxs[2] - entinfo->r.mins[2];
		}
	}
	return bestvis;
}

/*
==================
rvmBot::BotEntityVisible
==================
*/
float rvmBot::BotEntityVisible( int viewer, idVec3 eye, idAngles viewangles, float fov, int ent )
{
	return BotEntityVisibleTest( viewer, eye, viewangles, fov, ent, true );
}

/*
==================
rvmBot::BotUpdateBattleInventory
==================
*/
void rvmBot::BotUpdateBattleInventory( bot_state_t* bs, int enemy )
{
	idVec3 dir;
	idEntity* entinfo;

	entinfo = gameLocal.entities[enemy];

	VectorSubtract( entinfo->GetOrigin(), bs->origin, dir );
	bs->inventory[ENEMY_HEIGHT] = ( int )dir[2];
	dir[2] = 0;
	bs->inventory[ENEMY_HORIZONTAL_DIST] = ( int )dir.Length();
	//FIXME: add num visible enemies and num visible team mates to the inventory
}

/*
==================
rvmBot::BotAggression
==================
*/
float rvmBot::BotAggression( bot_state_t* bs )
{
	//if the bot has quad
	if( bs->inventory[INVENTORY_QUAD] )
	{
		//if the bot is not holding the gauntlet or the enemy is really nearby
		if( bs->weaponnum != 0 ||
				bs->inventory[ENEMY_HORIZONTAL_DIST] < 80 )
		{
			return 70;
		}
	}
	//if the enemy is located way higher than the bot
	if( bs->inventory[ENEMY_HEIGHT] > 200 )
	{
		return 0;
	}
	//if the bot is very low on health
	if( bs->inventory[INVENTORY_HEALTH] < 60 )
	{
		return 0;
	}
	//if the bot is low on health
	if( bs->inventory[INVENTORY_HEALTH] < 80 )
	{
		//if the bot has insufficient armor
		if( bs->inventory[INVENTORY_ARMOR] < 40 )
		{
			return 0;
		}
	}
	//if the bot can use the bfg
	if( bs->inventory[INVENTORY_BFG10K] > 0 &&
			bs->inventory[INVENTORY_BFGAMMO] > 7 )
	{
		return 100;
	}
	//if the bot can use the railgun
	if( bs->inventory[INVENTORY_RAILGUN] > 0 &&
			bs->inventory[INVENTORY_SLUGS] > 5 )
	{
		return 95;
	}
	//if the bot can use the lightning gun
	if( bs->inventory[INVENTORY_LIGHTNING] > 0 &&
			bs->inventory[INVENTORY_LIGHTNINGAMMO] > 50 )
	{
		return 90;
	}
	//if the bot can use the rocketlauncher
	if( bs->inventory[INVENTORY_ROCKETLAUNCHER] > 0 &&
			bs->inventory[INVENTORY_ROCKETS] > 5 )
	{
		return 90;
	}
	//if the bot can use the plasmagun
	if( bs->inventory[INVENTORY_PLASMAGUN] > 0 &&
			bs->inventory[INVENTORY_CELLS] > 40 )
	{
		return 85;
	}
	//if the bot can use the grenade launcher
	if( bs->inventory[INVENTORY_GRENADELAUNCHER] > 0 &&
			bs->inventory[INVENTORY_GRENADES] > 10 )
	{
		return 80;
	}
	//if the bot can use the shotgun
	if( bs->inventory[INVENTORY_SHOTGUN] > 0 &&
			bs->inventory[INVENTORY_SHELLS] > 10 )
	{
		return 50;
	}

	if( bs->inventory[INVENTORY_BULLETS] > 0 )
	{
		return 60;
	}

	//otherwise the bot is not feeling too good
	return 0;
}


/*
==================
rvmBot::BotWantsToRetreat
==================
*/
int rvmBot::BotWantsToRetreat( bot_state_t* bs )
{
	if( BotAggression( bs ) < 50 )
	{
		return true;
	}
	return false;
}

/*
==================
rvmBot::BotBattleUseItems
==================
*/
void rvmBot::BotBattleUseItems( bot_state_t* bs )
{
	if( bs->inventory[INVENTORY_HEALTH] < 40 )
	{
		if( bs->inventory[INVENTORY_TELEPORTER] > 0 )
		{
			bs->botinput.actionflags |= ACTION_USE;
		}
	}
	if( bs->inventory[INVENTORY_HEALTH] < 60 )
	{
		if( bs->inventory[INVENTORY_MEDKIT] > 0 )
		{
			//trap_EA_Use(bs->client);
			bs->botinput.actionflags |= ACTION_USE;
		}
	}
}

/*
==================
rvmBot::BotFindEnemy
==================
*/
int rvmBot::BotFindEnemy( bot_state_t* bs, int curenemy )
{
	int i, healthdecrease;
	float f, alertness, easyfragger, vis;
	float squaredist, cursquaredist;
	idPlayer* entinfo = NULL;
	idPlayer* curenemyinfo = NULL;
	idVec3 dir;
	idAngles angles;
	idPlayer* clientEnt;

	alertness = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_ALERTNESS, 0, 1 );
	easyfragger = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_EASY_FRAGGER, 0, 1 );

	clientEnt = gameLocal.entities[bs->client]->Cast<idPlayer>();

	//check if the health decreased
	healthdecrease = bs->lasthealth > bs->inventory[INVENTORY_HEALTH];

	//remember the current health value
	bs->lasthealth = bs->inventory[INVENTORY_HEALTH];
	//
	if( curenemy >= 0 )
	{
		//BotEntityInfo(curenemy, &curenemyinfo);
		curenemyinfo = gameLocal.entities[curenemy]->Cast<idPlayer>();
		// jmarshall - add flag support.
		//if (EntityCarriesFlag(&curenemyinfo)) return qfalse;
		// jmarshall end
		//VectorSubtract(curenemyinfo->r.currentOrigin, bs->origin, dir);
		dir = curenemyinfo->GetPhysics()->GetOrigin() - bs->origin;
		cursquaredist = dir.LengthSqr();// VectorLengthSquared(dir);
	}
	else
	{
		cursquaredist = 0;
	}

	for( i = 0; i < MAX_CLIENTS; i++ )
	{

		if( i == bs->client )
		{
			continue;
		}

		//if it's the current enemy
		if( i == curenemy )
		{
			continue;
		}

		entinfo = gameLocal.entities[i]->Cast<idPlayer>();
		if( !entinfo )
		{
			continue;
		}

		//if the enemy isn't dead and the enemy isn't the bot self
		if( EntityIsDead( entinfo ) || i == bs->entitynum )
		{
			continue;
		}

		if (entinfo->spectating) {
			continue;
		}

		//if the enemy is invisible and not shooting
// jmarshall - add invis
		//if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
		//	continue;
		//}
// jmarshall end

// jmarshall - eval, looks like code to not shoot chatting players or players that just spawned in.
//			   do we care about this?
		//if not an easy fragger don't shoot at chatting players
		//if (easyfragger < 0.5 && EntityIsChatting(&entinfo))
		//	continue;

		//
		//if (lastteleport_time > Bot_Time() - 3) {
		//	VectorSubtract(entinfo.origin, lastteleport_origin, dir);
		//	if (VectorLengthSquared(dir) < Square(70))
		//		continue;
		//}
// jmarshall end

		//calculate the distance towards the enemy
		idVec3 potentialTargetOrigin = entinfo->GetPhysics()->GetOrigin();
		dir = potentialTargetOrigin - bs->origin;
		squaredist = dir.LengthSqr();

		// jmarshall
		//if this entity is not carrying a flag
		//if (!EntityCarriesFlag(&entinfo))
		//{
		//if this enemy is further away than the current one
		if( curenemy >= 0 && squaredist > cursquaredist )
		{
			continue;
		}
		//}
// jmarshall end

		//if the bot has no
		if( squaredist > Square( 900.0 + alertness * 4000.0 ) )
		{
			continue;
		}

		// jmarshall - teams!
		//if on the same team
		//if (BotSameTeam(bs, i))
		//	continue;
		// jmarshall end
		//if the bot's health decreased or the enemy is shooting
		if( curenemy < 0 && ( healthdecrease || entinfo->IsShooting() ) )
		{
			f = 360;
		}
		else
		{
			f = 90 + 90 - ( 90 - ( squaredist > Square( 810 ) ? Square( 810 ) : squaredist ) / ( 810 * 9 ) );
		}
		//check if the enemy is visible

		// If we were last hit by someone then assume they are visible.
		if( bs->attackerEntity != entinfo )
		{
			vis = BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, f, i );
			if( vis <= 0 )
			{
				continue;
			}
		}

		//if the enemy is quite far away, not shooting and the bot is not damaged
		if( curenemy < 0 && squaredist > Square( 100 ) && !healthdecrease && !entinfo->IsShooting() )
		{
			//check if we can avoid this enemy
			VectorSubtract( bs->origin, entinfo->GetOrigin(), dir );
			angles = dir.ToAngles();

			//if the bot isn't in the fov of the enemy
			if( !clientEnt->CheckFOV( entinfo->GetOrigin() ) )
			{
				//update some stuff for this enemy
				BotUpdateBattleInventory( bs, i );

				//if the bot doesn't really want to fight
				if( BotWantsToRetreat( bs ) )
				{
					continue;
				}
			}
		}
		//found an enemy
		bs->enemy = i;//entinfo.number;
		if( curenemy >= 0 )
		{
			bs->enemysight_time = Bot_Time() - 2;
		}
		else
		{
			bs->enemysight_time = Bot_Time();
		}
		bs->enemysuicide = false;
		bs->enemydeath_time = 0;
		bs->enemyvisible_time = Bot_Time();
		return true;
	}
	return false;
}

/*
==================
rvmBot::BotMoveToGoal
==================
*/
void rvmBot::BotMoveToGoal( bot_state_t* bs, bot_goal_t* goal )
{
	bs->currentGoal = *goal;
	bs->currentGoal.framenum = gameLocal.framenum;
}

/*
==================
rvmBot::BotAimAtEnemy
==================
*/
void rvmBot::BotAimAtEnemy( bot_state_t* bs )
{
	int i, enemyvisible;
	float dist, f, aim_skill, aim_accuracy, speed, reactiontime;
	idVec3 dir, bestorigin, end, start, groundtarget, cmdmove, enemyvelocity;
	idVec3 mins( -4, -4, -4 );
	idVec3 maxs( 4, 4, 4 );
	weaponinfo_t wi;
	//aas_entityinfo_t entinfo;
	idPlayer* entinfo;
	bot_goal_t goal;
	trace_t trace;
	idVec3 target;
	idPlayer* self;

	//if the bot has no enemy
	if( bs->enemy < 0 )
	{
		return;
	}

	self = gameLocal.entities[bs->entitynum]->Cast<idPlayer>();

	//get the enemy entity information
	//BotEntityInfo(bs->enemy, &entinfo);
	entinfo = gameLocal.entities[bs->enemy]->Cast<idPlayer>();

	//if this is not a player (should be an obelisk)
	if( bs->enemy >= MAX_CLIENTS )
	{
		//if the obelisk is visible
		VectorCopy( entinfo->GetOrigin(), target );
#ifdef MISSIONPACK
		// if attacking an obelisk
		if( bs->enemy == redobelisk.entitynum ||
				bs->enemy == blueobelisk.entitynum )
		{
			target[2] += 32;
		}
#endif
		//aim at the obelisk
		VectorSubtract( target, bs->eye, dir );
		//vectoangles(dir, bs->viewangles);
		bs->viewangles = dir.ToAngles();

		//set the aim target before trying to attack
		VectorCopy( target, bs->aimtarget );
		return;
	}

	//
	//BotAI_Print(PRT_MESSAGE, "client %d: aiming at client %d\n", bs->entitynum, bs->enemy);
	//
	aim_skill = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1 );
	aim_accuracy = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1 );
	//
	if( aim_skill > 0.95 )
	{
		//don't aim too early
		reactiontime = 0.5 * botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1 );
		if( bs->enemysight_time > Bot_Time() - reactiontime )
		{
			return;
		}
		if( bs->teleport_time > Bot_Time() - reactiontime )
		{
			return;
		}
	}

	//get the weapon information
	botWeaponInfoManager.BotGetWeaponInfo( bs->ws, bs->weaponnum, &wi );

	//get the weapon specific aim accuracy and or aim skill
	if( wi.number == WP_MACHINEGUN )
	{
		aim_accuracy = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_MACHINEGUN, 0, 1 );
	}
	else if( wi.number == WP_SHOTGUN )
	{
		aim_accuracy = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_SHOTGUN, 0, 1 );
	}
	//else if (wi.number == WP_GRENADE_LAUNCHER) {
	//	aim_accuracy = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_GRENADELAUNCHER, 0, 1);
	//	aim_skill = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_GRENADELAUNCHER, 0, 1);
	//}
	else if( wi.number == WP_ROCKET_LAUNCHER )
	{
		aim_accuracy = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_ROCKETLAUNCHER, 0, 1 );
		aim_skill = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_ROCKETLAUNCHER, 0, 1 );
	}
	//else if (wi.number == WP_LIGHTNING) {
	//	aim_accuracy = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_LIGHTNING, 0, 1);
	//}
	//else if (wi.number == WP_RAILGUN) {
	//	aim_accuracy = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_RAILGUN, 0, 1);
	//}
	else if( wi.number == WP_PLASMAGUN )
	{
		aim_accuracy = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_ACCURACY_PLASMAGUN, 0, 1 );
		aim_skill = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_AIM_SKILL_PLASMAGUN, 0, 1 );
	}
	//else if (wi.number == WP_BFG) {
	//	aim_accuracy = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY_BFG10K, 0, 1);
	//	aim_skill = Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL_BFG10K, 0, 1);
	//}
	//
	if( aim_accuracy <= 0 )
	{
		aim_accuracy = 0.0001f;
	}
	//get the enemy entity information
	//BotEntityInfo(bs->enemy, &entinfo);
	entinfo = gameLocal.entities[bs->enemy]->Cast<idPlayer>();

	//if the enemy is invisible then shoot crappy most of the time
	//if (entinfo->IsInvisible()) {
	//	if (rvmBotUtil::random() > 0.1)
	//		aim_accuracy *= 0.4f;
	//}
	// jmarshall - fix aim accuracy.
	//VectorSubtract(entinfo->GetOrigin(), entinfo.lastvisorigin, enemyvelocity);
	//VectorScale(enemyvelocity, 1 / entinfo.update_time, enemyvelocity);
	////enemy origin and velocity is remembered every 0.5 seconds
	//if (bs->enemyposition_time < Bot_Time()) {
	//	//
	//	bs->enemyposition_time = Bot_Time() + 0.5;
	//	VectorCopy(enemyvelocity, bs->enemyvelocity);
	//	VectorCopy(entinfo.origin, bs->enemyorigin);
	//}
	////if not extremely skilled
	//if (aim_skill < 0.9) {
	//	VectorSubtract(entinfo.origin, bs->enemyorigin, dir);
	//	//if the enemy moved a bit
	//	if (VectorLengthSquared(dir) > Square(48)) {
	//		//if the enemy changed direction
	//		if (DotProduct(bs->enemyvelocity, enemyvelocity) < 0) {
	//			//aim accuracy should be worse now
	//			aim_accuracy *= 0.7f;
	//		}
	//	}
	//}
	// jmarshall end

	//check visibility of enemy
	enemyvisible = BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy );

	//if the enemy is visible
	if( enemyvisible )
	{
		//
		VectorCopy( entinfo->GetOrigin(), bestorigin );
		bestorigin[2] += 8;
		//get the start point shooting from
		//NOTE: the x and y projectile start offsets are ignored
		VectorCopy( bs->origin, start );
		start[2] += self->GetViewHeight();// bs->cur_ps.viewheight;
		start[2] += wi.offset[2];
		//
		//trap_Trace(&trace, start, mins, maxs, bestorigin, bs->entitynum, MASK_SHOT);
		gameLocal.Trace( trace, start, bestorigin, MASK_SHOT, bs->entitynum );
		//if the enemy is NOT hit
		if( trace.fraction <= 1 && trace.c.entityNum != bs->enemy )
		{
			bestorigin[2] += 16;
		}
		//if it is not an instant hit weapon the bot might want to predict the enemy
		if( wi.speed )
		{
			//
			VectorSubtract( bestorigin, bs->origin, dir );
			dist = dir.Length();
			VectorSubtract( entinfo->GetOrigin(), bs->enemyorigin, dir );
			//if the enemy is NOT pretty far away and strafing just small steps left and right
			if( !( dist > 100 && dir.LengthSqr() < Square( 32 ) ) )
			{
				//if skilled anough do exact prediction
				if( aim_skill > 0.8 &&
						//if the weapon is ready to fire
						!self->IsShooting() )
				{
					//aas_clientmove_t move;
					idVec3 origin;
					idVec3 last_enemy_visible_position;
					VectorCopy( last_enemy_visible_position, bs->last_enemy_visible_position );
					last_enemy_visible_position[2] += 20.0f;

					//
					VectorSubtract( entinfo->GetOrigin(), bs->origin, dir );

					////distance towards the enemy
					//dist = VectorLength(dir);
					////direction the enemy is moving in
					VectorSubtract( entinfo->GetOrigin(), last_enemy_visible_position, dir );
					////
					//VectorScale(dir, 1 / entinfo->update_time, dir);
					////
					//VectorCopy(entinfo->origin, origin);
					//origin[2] += 1;

					////
					//VectorClear(cmdmove);
					////AAS_ClearShownDebugLines();
					//trap_AAS_PredictClientMovement(&move, bs->enemy, origin,
					//	PRESENCE_CROUCH, qfalse,
					//	dir, cmdmove, 0,
					//	dist * 10 / wi.speed, 0.1f, 0, 0, qfalse);
					//VectorCopy(move.endpos, bestorigin);
					//BotAI_Print(PRT_MESSAGE, "%1.1f predicted speed = %f, frames = %f\n", Bot_Time(), VectorLength(dir), dist * 10 / wi.speed);

					bot_goal_t goal;
					VectorMA( entinfo->GetOrigin(), 30, dir, goal.origin );
					BotMoveToGoal( bs, &goal );
				}
				//if not that skilled do linear prediction
				else if( aim_skill > 0.4 )
				{
					// jmarshall - fix linear prediction.
					idVec3 last_enemy_visible_position;
					VectorCopy( last_enemy_visible_position, bs->last_enemy_visible_position );
					last_enemy_visible_position[2] += 20.0f;

					//VectorSubtract(entinfo->GetOrigin(), bs->origin, dir);
					////distance towards the enemy
					//dist = VectorLength(dir);
					////direction the enemy is moving in
					VectorSubtract( entinfo->GetOrigin(), last_enemy_visible_position, dir );
					//dir[2] = 0;
					////
					//speed = VectorNormalize(dir) / entinfo.update_time;
					////botimport.Print(PRT_MESSAGE, "speed = %f, wi->speed = %f\n", speed, wi->speed);
					////best spot to aim at
					//VectorMA(entinfo.origin, (dist / wi.speed) * speed, dir, bestorigin);
					bot_goal_t goal;
					VectorMA( entinfo->GetOrigin(), 30, dir, goal.origin );
					BotMoveToGoal( bs, &goal );
					// jmarshall end
				}
			}
		}
		//if the projectile does radial damage
		if( aim_skill > 0.6 && wi.proj.damagetype & DAMAGETYPE_RADIAL )
		{
			//if the enemy isn't standing significantly higher than the bot
			if( entinfo->GetOrigin()[2] < bs->origin[2] + 16 )
			{
				//try to aim at the ground in front of the enemy
				VectorCopy( entinfo->GetOrigin(), end );
				end[2] -= 64;
				//trap_Trace(&trace, entinfo->GetOrigin(), NULL, NULL, end, bs->enemy, MASK_SHOT);
				gameLocal.Trace( trace, entinfo->GetOrigin(), end, MASK_SHOT, bs->enemy );
				//
				VectorCopy( bestorigin, groundtarget );
// jmarshall - add start solid
				//if (trace.startsolid)
				//	groundtarget[2] = entinfo->GetOrigin()[2] - 16;
				//else
				groundtarget[2] = trace.endpos[2] - 8;
// jmarshall end
				//trace a line from projectile start to ground target
				//trap_Trace(&trace, start, NULL, NULL, groundtarget, bs->entitynum, MASK_SHOT);
				gameLocal.Trace( trace, start, groundtarget, MASK_SHOT, bs->entitynum );
				//if hitpoint is not vertically too far from the ground target
				if( idMath::Fabs( trace.endpos[2] - groundtarget[2] ) < 50 )
				{
					VectorSubtract( trace.endpos, groundtarget, dir );
					//if the hitpoint is near anough the ground target
					if( dir.LengthSqr() < Square( 60 ) )
					{
						VectorSubtract( trace.endpos, start, dir );
						//if the hitpoint is far anough from the bot
						if( dir.LengthSqr() > Square( 100 ) )
						{
							//check if the bot is visible from the ground target
							trace.endpos[2] += 1;
							//trap_Trace(&trace, trace.endpos, NULL, NULL, entinfo->GetOrigin(), bs->enemy, MASK_SHOT);
							gameLocal.Trace( trace, trace.endpos, entinfo->GetOrigin(), MASK_SHOT, bs->enemy );
							if( trace.fraction >= 1 )
							{
								//botimport.Print(PRT_MESSAGE, "%1.1f aiming at ground\n", AAS_Time());
								VectorCopy( groundtarget, bestorigin );
							}
						}
					}
				}
			}
		}
		bestorigin[0] += 20 * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
		bestorigin[1] += 20 * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
		bestorigin[2] += 10 * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
	}
	else
	{
		//
		VectorCopy( bs->lastenemyorigin, bestorigin );
		bestorigin[2] += 8;
		// jmarshall - fix this.
		//if the bot is skilled anough
		//if (aim_skill > 0.5) {
		//	//do prediction shots around corners
		//	if (wi.number == WP_BFG ||
		//		wi.number == WP_ROCKET_LAUNCHER ||
		//		wi.number == WP_GRENADE_LAUNCHER) {
		//		//create the chase goal
		//		goal.entitynum = bs->client;
		//		goal.areanum = bs->areanum;
		//		VectorCopy(bs->eye, goal.origin);
		//		VectorSet(goal.mins, -8, -8, -8);
		//		VectorSet(goal.maxs, 8, 8, 8);
		//		//
		//		if (trap_BotPredictVisiblePosition(bs->lastenemyorigin, bs->lastenemyareanum, &goal, TFL_DEFAULT, target)) {
		//			VectorSubtract(target, bs->eye, dir);
		//			if (VectorLengthSquared(dir) > Square(80)) {
		//				VectorCopy(target, bestorigin);
		//				bestorigin[2] -= 20;
		//			}
		//		}
		//		aim_accuracy = 1;
		//	}
		//}
		// jmarshall end
	}
	//
	if( enemyvisible )
	{
		//trap_Trace(&trace, bs->eye, NULL, NULL, bestorigin, bs->entitynum, MASK_SHOT);
		gameLocal.Trace( trace, bs->eye, bestorigin, MASK_SHOT, bs->entitynum );
		VectorCopy( trace.endpos, bs->aimtarget );
	}
	else
	{
		VectorCopy( bestorigin, bs->aimtarget );
	}
	//get aim direction
	VectorSubtract( bestorigin, bs->eye, dir );
	//
	if( wi.number == WP_MACHINEGUN ||
			wi.number == WP_SHOTGUN /*|| // jmarshall add lighting railgun.
		wi.number == WP_LIGHTNING ||
		wi.number == WP_RAILGUN*/ )
	{
		//distance towards the enemy
		dist = dir.Length();// VectorLength(dir);
		if( dist > 150 )
		{
			dist = 150;
		}
		f = 0.6 + dist / 150 * 0.4;
		aim_accuracy *= f;
	}
	//add some random stuff to the aim direction depending on the aim accuracy
	if( aim_accuracy < 0.8 )
	{
		dir.Normalize();
		for( i = 0; i < 3; i++ )
		{
			dir[i] += 0.3 * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
		}
	}
	//set the ideal view angles
	//vectoangles(dir, bs->viewangles);
	bs->viewangles = dir.ToAngles();

	//take the weapon spread into account for lower skilled bots
	bs->viewangles[PITCH] += 6 * wi.vspread * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
	bs->viewangles[PITCH] = idMath::AngleMod( bs->viewangles[PITCH] );
	bs->viewangles[YAW] += 6 * wi.hspread * rvmBotUtil::crandom() * ( 1 - aim_accuracy );
	bs->viewangles[YAW] = idMath::AngleMod( bs->viewangles[YAW] );
	// jmarshall - add bot_challenge.
	//if the bots should be really challenging
	//if (bot_challenge.integer) {
	//	//if the bot is really accurate and has the enemy in view for some time
	//	if (aim_accuracy > 0.9 && bs->enemysight_time < Bot_Time() - 1) {
	//		//set the view angles directly
	//		if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
	//		VectorCopy(bs->ideal_viewangles, bs->viewangles);
	//		trap_EA_View(bs->client, bs->viewangles);
	//	}
	//}
	//vectoangles(bi->dir, bi->viewangles);
	// jmarshall end
}


/*
==================
rvmBot::BotWantsToChase
==================
*/
bool rvmBot::BotWantsToChase( bot_state_t* bs )
{
	if( BotAggression( bs ) > 50 )
	{
		return true;
	}
	return false;
}



/*
==================
rvmBot::BotNearbyGoal
==================
*/
int rvmBot::BotNearbyGoal( bot_state_t* bs, int tfl, bot_goal_t* ltg, float range )
{
	int ret;

	// jmarshall - check for air.
	//check if the bot should go for air
	//if (BotGoForAir(bs, tfl, ltg, range)) return qtrue;
	////if the bot is carrying the enemy flag
	//if (BotCTFCarryingFlag(bs)) {
	//	//if the bot is just a few secs away from the base
	//	if (trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
	//		bs->teamgoal.areanum, TFL_DEFAULT) < 300) {
	//		//make the range really small
	//		range = 50;
	//	}
	//}
	// jmarshall end

	//
	ret = botGoalManager.BotChooseNBGItem( bs->gs, bs->origin, bs->inventory, tfl, ltg, range );

	return ret;
}


/*
=======================
rvmBot::BotGetRandomPointNearPosition
=======================
*/
void rvmBot::BotGetRandomPointNearPosition( idVec3 point, idVec3& randomPoint, float radius )
{
	idAAS* aas = gameLocal.GetBotAAS();
	idAASFile* file = aas->GetAASFile();

	int areaNum = aas->PointAreaNum(point);
	const aasArea_t& area = file->GetArea(areaNum);
	
	// justin: I'm changing this from the method I used in Doom 3 to a method that doesn't require me
	// to change the AAS format
	//int firstEdge = area.firstEdge;
	//int i = rvRandom::irand(0, area.numEdges);

	int firstFace = area.firstFace;
	int numFaces = area.numFaces;
	int faceId = rvRandom::irand(firstFace, numFaces);

	const aasIndex_t index = abs(file->GetFaceIndex(faceId));
	const aasFace_t& face = file->GetFace(index);

	int firstEdge = face.firstEdge;
	int i = rvRandom::irand(0, face.numEdges);

	const aasEdge_t &edge = file->GetEdge(abs(file->GetEdgeIndex(firstEdge + i)));

	randomPoint = file->GetVertex(edge.vertexNum[0]);
}

/*
=======================
rvmBot::BotMoveInRandomDirection
=======================
*/
int rvmBot::BotMoveInRandomDirection( bot_state_t* bs )
{
	rvmBot* ent = gameLocal.entities[bs->client]->Cast<rvmBot>();

	//ent->ResetPathFinding();

	float dist = idMath::Distance( ent->GetPhysics()->GetOrigin(), bs->random_move_position );

	if( !ent->PointVisible( bs->random_move_position ) )
	{
		dist = 0;
	}

	if( dist < 25 || bs->random_move_position.Length() == 0 )
	{
		BotGetRandomPointNearPosition( ent->GetPhysics()->GetOrigin(), bs->random_move_position, 50.0f );
	}

	VectorSubtract( bs->random_move_position, ent->GetPhysics()->GetOrigin(), bs->botinput.dir );

	idAngles ang( 0, bs->botinput.dir.ToYaw(), 0 );
	bs->botinput.speed = pm_speed.GetInteger();
	bs->botinput.dir.Normalize();
	bs->useRandomPosition = true;
	bs->botinput.speed = 400; // 200 = walk, 400 = run.
	return 0;
}

/*
============
rvmBot::ShowHideArea
============
*/
void rvmBot::MoveToCoverPoint(void)
{
	int areaNum, numObstacles;
	idVec3 target;
	aasGoal_t goal;
	aasObstacle_t obstacles[10];
	idVec3 origin = GetOrigin();
	idAAS* aas = gameLocal.GetBotAAS();

	areaNum = gameLocal.GetBotAAS()->PointReachableAreaNum(origin, aas->DefaultSearchBounds(), (AREA_REACHABLE_WALK | AREA_REACHABLE_FLY));
	target = aas->AreaCenter(aas->PointAreaNum(gameLocal.GetLocalPlayer()->GetOrigin()));

	// consider the target an obstacle
	obstacles[0].absBounds = idBounds(target).Expand(16);
	numObstacles = 1;

	idAASCallback_FindCoverArea findCover(target);
	if (aas->FindNearestGoal(goal, areaNum, origin, target, TFL_WALK | TFL_AIR, 0.0f, 0.0f, obstacles, numObstacles, findCover))
	{
		bs.currentGoal.origin = goal.origin;
	}
}

/*
==================
rvmBot::BotCheckAttack
==================
*/
void rvmBot::BotCheckAttack( bot_state_t* bs )
{
	float points, reactiontime, fov, firethrottle;
	int attackentity;
	trace_t bsptrace;
	//float selfpreservation;
	idVec3 forward, right, start, end, dir;
	idAngles angles;
	weaponinfo_t wi;
	trace_t trace;
	//aas_entityinfo_t entinfo;
	idEntity* entinfo;
	idVec3 mins( -8, -8, -8 );
	idVec3 maxs( 8, 8, 8 );
	idPlayer* self;

	attackentity = bs->enemy;
	//
	//BotEntityInfo(attackentity, &entinfo);
	entinfo = gameLocal.entities[attackentity];

	self = gameLocal.entities[bs->client]->Cast<idPlayer>();

	//
	reactiontime = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_REACTIONTIME, 0, 1 );
	if( bs->enemysight_time > Bot_Time() - reactiontime )
	{
		return;
	}
	if( bs->teleport_time > Bot_Time() - reactiontime )
	{
		return;
	}

	//if changing weapons
	if( bs->weaponchange_time > Bot_Time() - 0.1 )
	{
		return;
	}

	//check fire throttle characteristic
	if( bs->firethrottlewait_time > Bot_Time() )
	{
		return;
	}
	// jmarshall - this wasn't original behaivor, but multiplying it by 40 feels better for gameplay
	firethrottle = botCharacterStatsManager.Characteristic_BFloat( bs->character, CHARACTERISTIC_FIRETHROTTLE, 0, 1 ) * 40.0f;
	// jmarshall end
	if( bs->firethrottleshoot_time < Bot_Time() )
	{
		if( rvmBotUtil::random() > firethrottle )
		{
			bs->firethrottlewait_time = Bot_Time() + firethrottle;
			bs->firethrottleshoot_time = 0;
		}
		else
		{
			bs->firethrottleshoot_time = Bot_Time() + 1 - firethrottle;
			bs->firethrottlewait_time = 0;
		}
	}

	VectorSubtract( bs->aimtarget, bs->eye, dir );

// jmarshall - add gauntlet
	//if (bs->weaponnum == WP_GAUNTLET) {
	//	if (dir.LengthSqr() > Square(60)) {
	//		return;
	//	}
	//}
// jmarshall end
	if( dir.LengthSqr() < Square( 100 ) )
	{
		fov = 120;
	}
	else
	{
		fov = 50;
	}

	//vectoangles(dir, angles);
	angles = dir.ToAngles();
	if( !self->CheckFOV( entinfo->GetPhysics()->GetOrigin() ) )
	{
		return;
	}

	//trap_Trace(&bsptrace, bs->eye, NULL, NULL, bs->aimtarget, bs->client, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	gameLocal.Trace( bsptrace, bs->eye, bs->aimtarget, CONTENTS_SOLID | CONTENTS_PLAYERCLIP, bs->client );

	if( bsptrace.fraction < 1 && bsptrace.c.entityNum != attackentity )
	{
		return;
	}

	//get the weapon info
	botWeaponInfoManager.BotGetWeaponInfo( bs->ws, bs->weaponnum, &wi );

	//get the start point shooting from
	VectorCopy( bs->origin, start );
	start[2] += self->GetViewHeight();// bs->cur_ps.viewheight;
	bs->viewangles.ToVectors( &forward, &right, NULL );

	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	//end point aiming at
	VectorMA( start, 1000, forward, end );
	//a little back to make sure not inside a very close enemy
	VectorMA( start, -12, forward, start );
	//trap_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	gameLocal.Trace( trace, start, end, MASK_SHOT, bs->entitynum );

	//if the entity is a client
	if( trace.c.entityNum > 0 && trace.c.entityNum <= MAX_CLIENTS )
	{
		if( trace.c.entityNum != attackentity )
		{
			// jmarshall - teams
			//if a teammate is hit
			//if (BotSameTeam(bs, trace.entityNum))
			//	return;
			// jmarshall end
		}
	}
	//if won't hit the enemy or not attacking a player (obelisk)
	if( trace.c.entityNum != attackentity || attackentity >= MAX_CLIENTS )
	{
		//if the projectile does radial damage
		if( wi.proj.damagetype & DAMAGETYPE_RADIAL )
		{
			if( trace.fraction * 1000 < wi.proj.radius )
			{
				points = ( wi.proj.damage - 0.5 * trace.fraction * 1000 ) * 0.5;
				if( points > 0 )
				{
					return;
				}
			}
			//FIXME: check if a teammate gets radial damage
		}
	}
	//if fire has to be release to activate weapon
	if( wi.flags & WFL_FIRERELEASED )
	{
		if( bs->flags & BFL_ATTACKED )
		{
			//trap_EA_Attack(bs->client);
			bs->botinput.actionflags |= ACTION_ATTACK;
		}
	}
	else
	{
		//trap_EA_Attack(bs->client);
		bs->botinput.actionflags |= ACTION_ATTACK;
	}
	bs->flags ^= BFL_ATTACKED;
}
