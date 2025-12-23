/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//===========================================================================
//
// Name:			ai_cast_funcs.c
// Function:		Wolfenstein AI Character Decision Making
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"      //bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/botai.h"          //bot ai interface

#include "ai_cast.h"

//=================================================================================
//
// Helga, the first boss
//
//=================================================================================

#define HELGA_SPIRIT_BUILDUP_TIME       5000    // last for this long // RealRTCW was 8000
#define HELGA_SPIRIT_FADEOUT_TIME       500    // RealRTCW was 1000
#define HELGA_SPIRIT_DLIGHT_RADIUS_MAX  384
#define HELGA_SPIRIT_FIRE_INTERVAL      800   // RealRTCW was 1000

extern int lastZombieSpiritAttack;

char *AIFunc_Helga_SpiritAttack( cast_state_t *cs ) {
	gentity_t *ent;
	//
	cs->aiFlags |= AIFL_SPECIAL_FUNC;
	ent = &g_entities[cs->entityNum];
	// make sure we're still playing the right anim
	if ( ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) - BG_AnimationIndexForString( "attack1", cs->entityNum ) ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	if ( cs->enemyNum < 0 ) {
		ent->client->ps.torsoTimer  = 0;
		ent->client->ps.legsTimer   = 0;
		return AIFunc_DefaultStart( cs );
	}
	//
	// if we can't see them anymore, abort immediately
	if ( cs->vislist[cs->enemyNum].real_visible_timestamp != cs->vislist[cs->enemyNum].real_update_timestamp ) {
		ent->client->ps.torsoTimer  = 0;
		ent->client->ps.legsTimer   = 0;
		return AIFunc_DefaultStart( cs );
	}
	// we are firing this weapon, so record it
	cs->weaponFireTimes[WP_MONSTER_ATTACK2] = level.time;
	//
	// once an attack has started, only abort once the player leaves our view, or time runs out
	if ( cs->thinkFuncChangeTime < level.time - HELGA_SPIRIT_BUILDUP_TIME ) {
		// if enough time has elapsed, finish this attack
		if ( level.time > cs->thinkFuncChangeTime + HELGA_SPIRIT_BUILDUP_TIME + HELGA_SPIRIT_FADEOUT_TIME ) {
			ent->client->ps.torsoTimer  = 0;
			ent->client->ps.legsTimer   = 0;
			return AIFunc_DefaultStart( cs );
		}
	} else {

		// set timers
		ent->client->ps.torsoTimer  = 1000;
		ent->client->ps.legsTimer   = 1000;

		// draw the client-side effect
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT;

		// inform the client of our enemies position
		VectorCopy( g_entities[cs->enemyNum].client->ps.origin, ent->s.origin2 );
		ent->s.origin2[2] += g_entities[cs->enemyNum].client->ps.viewheight;
	}
	//
	//
	return NULL;
}

char *AIFunc_Helga_SpiritAttack_Start( cast_state_t *cs ) {
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	ent->s.otherEntityNum2 = cs->enemyNum;
	ent->s.effect1Time = level.time;
	cs->aiFlags |= AIFL_SPECIAL_FUNC;
	//
	// dont turn
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	// play an anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_WEAPON, WP_MONSTER_ATTACK2, qtrue );
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );
	//
	cs->aifunc = AIFunc_Helga_SpiritAttack;
	return "AIFunc_Helga_SpiritAttack";
}

//=================================================================================
//
// Standing melee attacks
//
//=================================================================================

#define NUM_HELGA_ANIMS     3
#define MAX_HELGA_IMPACTS   3
int helgaHitTimes[NUM_HELGA_ANIMS][MAX_HELGA_IMPACTS] = {   // up to three hits per attack
	{ANIMLENGTH( 16,20 ),-1},
	{ANIMLENGTH( 11,20 ),ANIMLENGTH( 19,20 ),-1},
	{ANIMLENGTH( 10,20 ),ANIMLENGTH( 17,20 ),ANIMLENGTH( 26,20 )},
};
int helgaHitDamage[NUM_HELGA_ANIMS] = { // RealRTCW was 20,14,12
	50,
	40,
	30
};

/*
================
AIFunc_Helga_Melee
================
*/
char *AIFunc_Helga_Melee( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	gentity_t *enemy;
	cast_state_t *ecs;
	int hitDelay = -1, anim;
	trace_t tr;
	float enemyDist;
	aicast_predictmove_t move;
	vec3_t vec;

	cs->aiFlags |= AIFL_SPECIAL_FUNC;

	if ( !ent->client->ps.torsoTimer || !ent->client->ps.legsTimer ) {
		cs->aiFlags &= ~AIFL_SPECIAL_FUNC;
		return AIFunc_DefaultStart( cs );
	}

	if ( cs->enemyNum < 0 ) {
		ent->client->ps.legsTimer = 0;      // allow legs us to move
		ent->client->ps.torsoTimer = 0;     // allow legs us to move
		cs->aiFlags &= ~AIFL_SPECIAL_FUNC;
		return AIFunc_DefaultStart( cs );
	}

	ecs = AICast_GetCastState( cs->enemyNum );
	enemy = &g_entities[cs->enemyNum];

	anim = ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) - BG_AnimationIndexForString( "attack3", cs->entityNum );
	if ( anim < 0 || anim >= NUM_HELGA_ANIMS ) {
		// animation interupted
		cs->aiFlags &= ~AIFL_SPECIAL_FUNC;
		return AIFunc_DefaultStart( cs );
		//G_Error( "AIFunc_HelgaZombieMelee: helgaBoss using invalid or unknown attack anim" );
	}
	if ( cs->animHitCount < MAX_HELGA_IMPACTS && helgaHitTimes[anim][cs->animHitCount] >= 0 ) {

		// face them
		VectorCopy( cs->bs->origin, vec );
		vec[2] += ent->client->ps.viewheight;
		VectorSubtract( enemy->client->ps.origin, vec, vec );
		VectorNormalize( vec );
		vectoangles( vec, cs->ideal_viewangles );
		cs->ideal_viewangles[PITCH] = AngleNormalize180( cs->ideal_viewangles[PITCH] );

		// get hitDelay
		if ( !cs->animHitCount ) {
			hitDelay = helgaHitTimes[anim][cs->animHitCount];
		} else {
			hitDelay = helgaHitTimes[anim][cs->animHitCount] - helgaHitTimes[anim][cs->animHitCount - 1];
		}

		// check for inflicting damage
		if ( level.time - cs->weaponFireTimes[cs->weaponNum] > hitDelay ) {
			// do melee damage
			enemyDist = VectorDistance( enemy->r.currentOrigin, ent->r.currentOrigin );
			enemyDist -= g_entities[cs->enemyNum].r.maxs[0];
			enemyDist -= ent->r.maxs[0];
			if ( enemyDist < 10 + AICast_WeaponRange( cs, cs->weaponNum ) ) {
				trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, enemy->r.currentOrigin, ent->s.number, MASK_SHOT );
				if ( tr.entityNum == cs->enemyNum ) {
					G_Damage( &g_entities[tr.entityNum], ent, ent, vec3_origin, tr.endpos,
							  helgaHitDamage[anim], 0, MOD_MONSTER_MELEE );
					G_AddEvent( enemy, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[STAYSOUNDSCRIPT] ) );
				}
			}
			cs->weaponFireTimes[cs->weaponNum] = level.time;
			cs->animHitCount++;
		}
	}

	// if they are outside range, move forward
	AICast_PredictMovement( ecs, 2, 0.3, &move, &g_entities[cs->enemyNum].client->pers.cmd, -1 );
	VectorSubtract( move.endpos, cs->bs->origin, vec );
	vec[2] = 0;
	enemyDist = VectorLength( vec );
	enemyDist -= g_entities[cs->enemyNum].r.maxs[0];
	enemyDist -= ent->r.maxs[0];
	if ( enemyDist > 8 ) {    // we can get closer
		//if (!ent->client->ps.legsTimer) {
		//	cs->castScriptStatus.scriptNoMoveTime = 0;
		trap_EA_MoveForward( cs->entityNum );
		//}
		//ent->client->ps.legsTimer = 0;		// allow legs us to move
	}

	return NULL;
}

/*
================
AIFunc_Helga_MeleeStart
================
*/
char *AIFunc_Helga_MeleeStart( cast_state_t *cs ) {
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];
	ent->s.effect1Time = level.time;
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	cs->animHitCount = 0;
	cs->aiFlags |= AIFL_SPECIAL_FUNC;

	// face them
	AICast_AimAtEnemy( cs );

	// play an anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_WEAPON, cs->weaponNum, qtrue );
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );

	// play a sound
	G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[ATTACKSOUNDSCRIPT] ) );

	cs->aifunc = AIFunc_Helga_Melee;
	cs->aifunc( cs );   // think once now, to prevent a delay
	return "AIFunc_Helga_Melee";
}


//===================================================================

/*
==============
AIFunc_FlameZombie_Portal
==============
*/
char *AIFunc_FlameZombie_Portal( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	if ( cs->thinkFuncChangeTime < level.time - PORTAL_ZOMBIE_SPAWNTIME ) {
		// HACK, make them aware of the player
		AICast_UpdateVisibility( &g_entities[cs->entityNum], AICast_FindEntityForName( "player" ), qfalse, qtrue );
		ent->s.time2 = 0;   // turn spawning effect off
		return AIFunc_DefaultStart( cs );
	}
	//
	return NULL;
}

/*
==============
AIFunc_FlameZombie_PortalStart
==============
*/
char *AIFunc_FlameZombie_PortalStart( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	ent->s.time2 = level.time + 200;    // hijacking this for portal spawning effect
	//
	// play a special animation
	ent->client->ps.torsoAnim =
		( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_EXTRA1;
	ent->client->ps.legsAnim =
		( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_EXTRA1;
	ent->client->ps.torsoTimer = PORTAL_ZOMBIE_SPAWNTIME - 200;
	ent->client->ps.legsTimer = PORTAL_ZOMBIE_SPAWNTIME - 200;
	//
	cs->thinkFuncChangeTime = level.time;
	//
	cs->aifunc = AIFunc_FlameZombie_Portal;
	return "AIFunc_FlameZombie_Portal";
}