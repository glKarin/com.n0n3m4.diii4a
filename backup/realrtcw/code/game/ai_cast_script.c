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
// Name:			ai_cast_script.c
// Function:		Wolfenstein AI Character Scripting
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

/*
Scripting that allows the designers to control the behaviour of AI characters
according to each different scenario.
*/

// action functions need to be declared here so they can be accessed in the scriptAction table
qboolean AICast_ScriptAction_GotoMarker( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_WalkToMarker( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_CrouchToMarker( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GotoCast( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_WalkToCast( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_CrouchToCast( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Wait( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AbortIfLoadgame( cast_state_t *cs, char *params );	//----(SA)	added
qboolean AICast_ScriptAction_Trigger( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_FollowCast( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_PlaySound( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoAttack( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Attack( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_PlayAnim( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ClearAnim( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetAmmo( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetClip( cast_state_t *cs, char *params );			//----(SA)	added
qboolean AICast_ScriptAction_SelectWeapon( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetMoveSpeed( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GiveScore( cast_state_t *cs, char *params );		//----(SA)	added		
qboolean AICast_ScriptAction_SetWave ( cast_state_t *cs, char *params );		//----(SA)	added	
qboolean AICast_ScriptAction_GiveArmor( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_SetArmor( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_GiveAmmo( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_GiveHealth( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_IncreaseRespawns( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_SuggestWeapon( cast_state_t *cs, char *params );	//----(SA)	added
qboolean AICast_ScriptAction_GiveWeapon( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GiveWeaponFull( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GiveInventory( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GivePerk( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_TakeWeapon( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoRespawn( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_RandomRespawn( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Movetype( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AlertEntity( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SaveGame( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SaveCheckpoint( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_FireAtTarget( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GodMode( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Accum( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Wave( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_GlobalAccum( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SpawnCast( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_MissionFailed( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ObjectiveMet( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_PrintBonus( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ObjectivesNeeded( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoAIDamage( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Print( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_FaceTargetAngles( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_FaceEntity( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ResetScript( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Mount( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Unmount( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SavePersistant( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ChangeLevel( cast_state_t *cs, char *params );

// Achievements
qboolean AICast_ScriptAction_AchievementMap_W3D( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AchievementMap_W3DSEC( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_goldchest( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_warcrime( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_speedrun( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_training( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_strangelove( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_rocketstealth( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_crystal( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_stealth1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_stealth2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter3( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter4( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter5( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter6( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter7( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter1_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter2_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter3_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter4_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter5_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter6_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_chapter7_hard( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_boss1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_boss2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_boss3( cast_state_t *cs, char *params );

// Addons achievements
qboolean AICast_ScriptAction_Achievement_curse( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_stalingrad( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_timegate( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_pro51( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_sf( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ra( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ic( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_capuzzo( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_saucers( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_arkot( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_darkm( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_darkm_2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_dm2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_dm2_2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_dm2_3( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_dm2_4( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_tda_day( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_tda_night( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_tda_dark( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_tda_plus( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_tda_arena( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_lion( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_parkour( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_manor1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_manor1_2( cast_state_t *cs, char *params );

// Bonus modes achievements
qboolean AICast_ScriptAction_Achievement_walkinthepark( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ironman( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_hardcore( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_999( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_nightmare( cast_state_t *cs, char *params );

// Winterstein achievements
qboolean AICast_ScriptAction_Achievement_booze( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_party( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_winterstein( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_speedrun_norway( cast_state_t *cs, char *params );

//ETSP achievements
qboolean AICast_ScriptAction_AchievementMap_SIWA( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AchievementMap_SEAWALL( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_GOLDRUSH( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_RADAR( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_RAILGUN( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_FUEL( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ALLGOLDSIWA( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_CHESTSIWA( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_FUEL1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_SPEEDBATTERY( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ROOMBATTERY( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_SAFE( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_HEIST( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_KELLYS( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MANSIONGOLD( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_PANTHER( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_DEPOT( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_DORA( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ALLGOLDFUEL( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_ETBONUS( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_BINOCS( cast_state_t *cs, char *params );

// Vendetta Dilogy achievements
qboolean AICast_ScriptAction_Achievement_VENDETTA1_1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_3( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_4( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_5( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_6( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_7( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_8( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_9( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_10( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA1_11( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_3( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_4( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_5( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_6( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_7( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_8( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_VENDETTA2_9( cast_state_t *cs, char *params );

// Warbell achievements
qboolean AICast_ScriptAction_Achievement_WARBELL1( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_WARBELL2( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_WARBELL3( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_WARBELL4( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_WARBELL5( cast_state_t *cs, char *params );

// Malta Update Achievements
qboolean AICast_ScriptAction_Achievement_MALTA_NIGHTMARE( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_LEAP( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_OSA( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_GOAT( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_COURSE( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_RADIO( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_EGYPT( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_WIDE( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_FIREFLY( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_LAIR( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_HIDEOUT( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_BARTENDER( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_BETRAYER( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Achievement_MALTA_AGENT2( cast_state_t *cs, char *params );

//Ice
qboolean AICast_ScriptAction_Achievement_ICE_BEAT( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_Achievement_ICE_DH( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_Achievement_ICE_STEALTH( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_Achievement_ICE_SECRET( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_Achievement_ICE_DEFENSE ( cast_state_t *cs, char *params ) ;
qboolean AICast_ScriptAction_Achievement_ICE_EE( cast_state_t *cs, char *params ) ;

qboolean AICast_ScriptAction_Achievement_6PERKS( cast_state_t *cs, char *params ) ;


qboolean AICast_ScriptAction_EndGame( cast_state_t *cs, char *params );			//----(SA)	added
qboolean AICast_ScriptAction_Announce( cast_state_t *cs, char *params );		
qboolean AICast_ScriptAction_Teleport( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_FoundSecret( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoSight( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Sight( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoAvoid( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Avoid( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Attrib( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_DenyAction( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_LightningDamage( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Headlook( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_BackupScript( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_RestoreScript( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_StateType( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_KnockBack( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Zoom( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Parachute( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Cigarette( cast_state_t *cs, char *params );		//----(SA)	added
qboolean AICast_ScriptAction_StartCam( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_StopCam( cast_state_t *cs, char *params );			//----(SA)	added
qboolean AICast_ScriptAction_StartCamBlack( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_EntityScriptName( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AIScriptName( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_SetHealth( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_NoTarget( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_Cvar( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_decoy( cast_state_t *cs, char *params );


qboolean AICast_ScriptAction_MusicStart( cast_state_t *cs, char *params );		//----(SA)
qboolean AICast_ScriptAction_MusicPlay( cast_state_t *cs, char *params );		//----(SA)
qboolean AICast_ScriptAction_MusicStop( cast_state_t *cs, char *params );		//----(SA)
qboolean AICast_ScriptAction_MusicFade( cast_state_t *cs, char *params );		//----(SA)
qboolean AICast_ScriptAction_MusicQueue( cast_state_t *cs, char *params );		//----(SA)

qboolean AICast_ScriptAction_ExplicitRouting( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_LockPlayer( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_ScreenFade( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_AnimCondition( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_PushAway( cast_state_t *cs, char *params );
qboolean AICast_ScriptAction_CatchFire( cast_state_t *cs, char *params );

qboolean AICast_ScriptAction_ChangeAiTeam(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_ChangeAiName(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_ChangeAiSkin(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_ChangeAiHead(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_DropWeapon(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_AccumPrint(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_GlobalAccumPrint(cast_state_t* cs, char* params);
qboolean AICast_ScriptAction_Burned(cast_state_t* cs, char* params);

// these are the actions that each event can call
cast_script_stack_action_t scriptActions[] =
{
	{"drop_weapon", AICast_ScriptAction_DropWeapon},
	{"changeaiteam", AICast_ScriptAction_ChangeAiTeam},
	{"changeainame", AICast_ScriptAction_ChangeAiName},
	{"changeaiskin", AICast_ScriptAction_ChangeAiSkin},
	{"changeaihead", AICast_ScriptAction_ChangeAiHead},
	{"burn", AICast_ScriptAction_Burned},
	{"accumprint", AICast_ScriptAction_AccumPrint},
	{"globalaccumprint", AICast_ScriptAction_GlobalAccumPrint},
	{"gotomarker",       AICast_ScriptAction_GotoMarker},
	{"runtomarker",      AICast_ScriptAction_GotoMarker},
	{"walktomarker", AICast_ScriptAction_WalkToMarker},
	{"crouchtomarker",   AICast_ScriptAction_CrouchToMarker},
	{"gotocast",     AICast_ScriptAction_GotoCast},
	{"runtocast",        AICast_ScriptAction_GotoCast},
	{"walktocast",       AICast_ScriptAction_WalkToCast},
	{"crouchtocast", AICast_ScriptAction_CrouchToCast},
	{"followcast",       AICast_ScriptAction_FollowCast},
	{"playsound",        AICast_ScriptAction_PlaySound},
	{"playanim",     AICast_ScriptAction_PlayAnim},
	{"clearanim",        AICast_ScriptAction_ClearAnim},
	{"wait",         AICast_ScriptAction_Wait},
	{"abort_if_loadgame",AICast_ScriptAction_AbortIfLoadgame},		//----(SA)	added
	{"trigger",          AICast_ScriptAction_Trigger},
	{"setammo",          AICast_ScriptAction_SetAmmo},
	{"setclip",          AICast_ScriptAction_SetClip},				//----(SA)	added
	{"selectweapon", AICast_ScriptAction_SelectWeapon},
	{"noattack",     AICast_ScriptAction_NoAttack},
	{"suggestweapon",    AICast_ScriptAction_SuggestWeapon},		//----(SA)	added
	{"attack",           AICast_ScriptAction_Attack},
	{"setmovespeed",     AICast_ScriptAction_SetMoveSpeed},					
	{"givearmor",        AICast_ScriptAction_GiveArmor},			//----(SA)	added
	{"setarmor",     AICast_ScriptAction_SetArmor},					//----(SA)	added
	{"giveammo",        AICast_ScriptAction_GiveAmmo},			
	{"givehealth",        AICast_ScriptAction_GiveHealth},
	{"increaserespawns",        AICast_ScriptAction_IncreaseRespawns},					
	{"giveinventory",    AICast_ScriptAction_GiveInventory},
	{"giveperk",    AICast_ScriptAction_GivePerk},
	{"giveweapon",       AICast_ScriptAction_GiveWeapon},
	{"giveweaponfull",   AICast_ScriptAction_GiveWeaponFull},
	{"takeweapon",       AICast_ScriptAction_TakeWeapon},
	{"norespawn",       AICast_ScriptAction_NoRespawn},
	{"randomrespawn",       AICast_ScriptAction_RandomRespawn},
	{"movetype",     AICast_ScriptAction_Movetype},
	{"alertentity",      AICast_ScriptAction_AlertEntity},
	{"savegame",     AICast_ScriptAction_SaveGame},
	{"savecheckpoint",     AICast_ScriptAction_SaveCheckpoint},
	{"fireattarget", AICast_ScriptAction_FireAtTarget},
	{"godmode",          AICast_ScriptAction_GodMode},
	{"accum",            AICast_ScriptAction_Accum},
	{"wave",            AICast_ScriptAction_Wave},
	{"globalaccum",      AICast_ScriptAction_GlobalAccum},
	{"spawncast",        AICast_ScriptAction_SpawnCast},
	{"missionfailed",    AICast_ScriptAction_MissionFailed},
	{"missionsuccess",   AICast_ScriptAction_ObjectiveMet},
	{"objectivemet", AICast_ScriptAction_ObjectiveMet},				// dupe of missionsuccess so scripts can changeover to a more logical name
	{"objectivesneeded",AICast_ScriptAction_ObjectivesNeeded},
	{"printbonus",     AICast_ScriptAction_PrintBonus},
	{"noaidamage",       AICast_ScriptAction_NoAIDamage},
	{"print",            AICast_ScriptAction_Print},
	{"facetargetangles",AICast_ScriptAction_FaceTargetAngles},
	{"face_entity",      AICast_ScriptAction_FaceEntity},
	{"resetscript",      AICast_ScriptAction_ResetScript},
	{"mount",            AICast_ScriptAction_Mount},
	{"unmount",          AICast_ScriptAction_Unmount},
	{"savepersistant",   AICast_ScriptAction_SavePersistant},
	{"changelevel",      AICast_ScriptAction_ChangeLevel},
	// Achievements
	{"achievement_map_w3d",      AICast_ScriptAction_AchievementMap_W3D},
	{"achievement_map_w3dsec",      AICast_ScriptAction_AchievementMap_W3DSEC},
	{"achievement_goldchest",      AICast_ScriptAction_Achievement_goldchest},
	{"achievement_warcrime",      AICast_ScriptAction_Achievement_warcrime},
	{"achievement_speedrun",      AICast_ScriptAction_Achievement_speedrun},
	{"achievement_training",      AICast_ScriptAction_Achievement_training},
	{"achievement_strangelove",      AICast_ScriptAction_Achievement_strangelove},
	{"achievement_rocketstealth",      AICast_ScriptAction_Achievement_rocketstealth},
	{"achievement_crystal",      AICast_ScriptAction_Achievement_crystal},
	{"achievement_stealth1",      AICast_ScriptAction_Achievement_stealth1},
	{"achievement_stealth2",      AICast_ScriptAction_Achievement_stealth2},
	{"achievement_chapter1",      AICast_ScriptAction_Achievement_chapter1},
	{"achievement_chapter2",      AICast_ScriptAction_Achievement_chapter2},
	{"achievement_chapter3",      AICast_ScriptAction_Achievement_chapter3},
	{"achievement_chapter4",      AICast_ScriptAction_Achievement_chapter4},
	{"achievement_chapter5",      AICast_ScriptAction_Achievement_chapter5},
	{"achievement_chapter6",      AICast_ScriptAction_Achievement_chapter6},
	{"achievement_chapter7",      AICast_ScriptAction_Achievement_chapter7},
	{"achievement_chapter1_hard",      AICast_ScriptAction_Achievement_chapter1_hard},
	{"achievement_chapter2_hard",      AICast_ScriptAction_Achievement_chapter2_hard},
	{"achievement_chapter3_hard",      AICast_ScriptAction_Achievement_chapter3_hard},
	{"achievement_chapter4_hard",      AICast_ScriptAction_Achievement_chapter4_hard},
	{"achievement_chapter5_hard",      AICast_ScriptAction_Achievement_chapter5_hard},
	{"achievement_chapter6_hard",      AICast_ScriptAction_Achievement_chapter6_hard},
	{"achievement_chapter7_hard",      AICast_ScriptAction_Achievement_chapter7_hard},
	{"achievement_boss1",      AICast_ScriptAction_Achievement_boss1},
	{"achievement_boss2",      AICast_ScriptAction_Achievement_boss2},
	{"achievement_boss3",      AICast_ScriptAction_Achievement_boss3},
	// Addons achievements
	{"achievement_curse",      AICast_ScriptAction_Achievement_curse},
	{"achievement_stalingrad",      AICast_ScriptAction_Achievement_stalingrad},
	{"achievement_timegate",      AICast_ScriptAction_Achievement_timegate},
	{"achievement_pro51",      AICast_ScriptAction_Achievement_pro51},
	{"achievement_sf",      AICast_ScriptAction_Achievement_sf},
	{"achievement_ra",      AICast_ScriptAction_Achievement_ra},
	{"achievement_ic",      AICast_ScriptAction_Achievement_ic},
	{"achievement_capuzzo",      AICast_ScriptAction_Achievement_capuzzo},
	{"achievement_saucers",      AICast_ScriptAction_Achievement_saucers},
	{"achievement_arkot",      AICast_ScriptAction_Achievement_arkot},
	{"achievement_darkm",      AICast_ScriptAction_Achievement_darkm},
	{"achievement_darkm_2",      AICast_ScriptAction_Achievement_darkm_2},
	{"achievement_dm2",      AICast_ScriptAction_Achievement_dm2},
	{"achievement_dm2_2",      AICast_ScriptAction_Achievement_dm2_2},
	{"achievement_dm2_3",      AICast_ScriptAction_Achievement_dm2_3},
	{"achievement_dm2_4",      AICast_ScriptAction_Achievement_dm2_4},
	{"achievement_tda_day",      AICast_ScriptAction_Achievement_tda_day},
	{"achievement_tda_night",      AICast_ScriptAction_Achievement_tda_night},
	{"achievement_tda_dark",      AICast_ScriptAction_Achievement_tda_dark},
	{"achievement_tda_plus",      AICast_ScriptAction_Achievement_tda_plus},
	{"achievement_tda_arena",      AICast_ScriptAction_Achievement_tda_arena},
	{"achievement_lion",      AICast_ScriptAction_Achievement_lion},
	{"achievement_parkour",      AICast_ScriptAction_Achievement_parkour},
	{"achievement_manor1",      AICast_ScriptAction_Achievement_manor1},
	{"achievement_manor1_2",      AICast_ScriptAction_Achievement_manor1_2},
	// Bonus modes achievements
	{"achievement_walkinthepark",      AICast_ScriptAction_Achievement_walkinthepark},
	{"achievement_ironman",      AICast_ScriptAction_Achievement_ironman},
	{"achievement_hardcore",      AICast_ScriptAction_Achievement_hardcore},
	{"achievement_999",      AICast_ScriptAction_Achievement_999},
	{"achievement_nightmare",      AICast_ScriptAction_Achievement_nightmare},
	// Winterstein achievements
	{"achievement_booze",      AICast_ScriptAction_Achievement_booze},
	{"achievement_party",      AICast_ScriptAction_Achievement_party},
	{"achievement_winterstein",      AICast_ScriptAction_Achievement_winterstein},
	{"achievement_speedrun_norway",      AICast_ScriptAction_Achievement_speedrun_norway},
	// ETSP achievements
	{"achievement_map_siwa",      AICast_ScriptAction_AchievementMap_SIWA},
	{"achievement_map_seawall",      AICast_ScriptAction_AchievementMap_SEAWALL},
	{"achievement_map_goldrush",      AICast_ScriptAction_Achievement_GOLDRUSH},
	{"achievement_map_radar",      AICast_ScriptAction_Achievement_RADAR},
	{"achievement_map_railgun",      AICast_ScriptAction_Achievement_RAILGUN},
	{"achievement_map_fueldump",      AICast_ScriptAction_Achievement_FUEL},
	{"achievement_allgoldsiwa",      AICast_ScriptAction_Achievement_ALLGOLDSIWA},
	{"achievement_chestsiwa",      AICast_ScriptAction_Achievement_CHESTSIWA},
	{"achievement_speedbattery",      AICast_ScriptAction_Achievement_SPEEDBATTERY},
	{"achievement_roombattery",      AICast_ScriptAction_Achievement_ROOMBATTERY},
	{"achievement_safe",      AICast_ScriptAction_Achievement_SAFE},
	{"achievement_heist",      AICast_ScriptAction_Achievement_HEIST},
	{"achievement_kellys",      AICast_ScriptAction_Achievement_KELLYS},
	{"achievement_mansiongold",      AICast_ScriptAction_Achievement_MANSIONGOLD},
	{"achievement_panther",      AICast_ScriptAction_Achievement_PANTHER},
	{"achievement_depot",      AICast_ScriptAction_Achievement_DEPOT},
	{"achievement_dora",      AICast_ScriptAction_Achievement_DORA},
	{"achievement_fuel1",      AICast_ScriptAction_Achievement_FUEL1},
	{"achievement_allgoldfuel",      AICast_ScriptAction_Achievement_ALLGOLDFUEL},
	{"achievement_etbonus",      AICast_ScriptAction_Achievement_ETBONUS},
	{"achievement_binocs",      AICast_ScriptAction_Achievement_BINOCS},
	// Vendetta Dilogy achievements
	{"achievement_VENDETTA1_1",      AICast_ScriptAction_Achievement_VENDETTA1_1},
	{"achievement_VENDETTA1_2",      AICast_ScriptAction_Achievement_VENDETTA1_2},
	{"achievement_VENDETTA1_3",      AICast_ScriptAction_Achievement_VENDETTA1_3},
	{"achievement_VENDETTA1_4",      AICast_ScriptAction_Achievement_VENDETTA1_4},
	{"achievement_VENDETTA1_5",      AICast_ScriptAction_Achievement_VENDETTA1_5},
	{"achievement_VENDETTA1_6",      AICast_ScriptAction_Achievement_VENDETTA1_6},
	{"achievement_VENDETTA1_7",      AICast_ScriptAction_Achievement_VENDETTA1_7},
	{"achievement_VENDETTA1_8",      AICast_ScriptAction_Achievement_VENDETTA1_8},
	{"achievement_VENDETTA1_9",      AICast_ScriptAction_Achievement_VENDETTA1_9},
	{"achievement_VENDETTA1_10",      AICast_ScriptAction_Achievement_VENDETTA1_10},
	{"achievement_VENDETTA1_11",      AICast_ScriptAction_Achievement_VENDETTA1_11},
	{"achievement_VENDETTA2_1",      AICast_ScriptAction_Achievement_VENDETTA2_1},
	{"achievement_VENDETTA2_2",      AICast_ScriptAction_Achievement_VENDETTA2_2},
	{"achievement_VENDETTA2_3",      AICast_ScriptAction_Achievement_VENDETTA2_3},
	{"achievement_VENDETTA2_4",      AICast_ScriptAction_Achievement_VENDETTA2_4},
	{"achievement_VENDETTA2_5",      AICast_ScriptAction_Achievement_VENDETTA2_5},
	{"achievement_VENDETTA2_6",      AICast_ScriptAction_Achievement_VENDETTA2_6},
	{"achievement_VENDETTA2_7",      AICast_ScriptAction_Achievement_VENDETTA2_7},
	{"achievement_VENDETTA2_8",      AICast_ScriptAction_Achievement_VENDETTA2_8},
	{"achievement_VENDETTA2_9",      AICast_ScriptAction_Achievement_VENDETTA2_9},
    // Warbell achievements
	{"achievement_map_warbell",      AICast_ScriptAction_Achievement_WARBELL1},
	{"achievement_watersWarbell",    AICast_ScriptAction_Achievement_WARBELL2},
	{"achievement_blavWarbell",      AICast_ScriptAction_Achievement_WARBELL3},
	{"achievement_olaricWarbell",    AICast_ScriptAction_Achievement_WARBELL4},
	{"achievement_heinrichWarbell",  AICast_ScriptAction_Achievement_WARBELL5},
	// Malta Update Achievements
	{"achievement_malta_nightmare",  AICast_ScriptAction_Achievement_MALTA_NIGHTMARE},
	{"achievement_malta_leap",       AICast_ScriptAction_Achievement_MALTA_LEAP},
	{"achievement_malta_osa",       AICast_ScriptAction_Achievement_MALTA_OSA},
	{"achievement_malta_goat",       AICast_ScriptAction_Achievement_MALTA_GOAT},
	{"achievement_malta_course",       AICast_ScriptAction_Achievement_MALTA_COURSE},
	{"achievement_malta_radio",       AICast_ScriptAction_Achievement_MALTA_RADIO},
	{"achievement_malta_egypt",       AICast_ScriptAction_Achievement_MALTA_EGYPT},
	{"achievement_malta_wide",       AICast_ScriptAction_Achievement_MALTA_WIDE},
	{"achievement_malta_firefly",       AICast_ScriptAction_Achievement_MALTA_FIREFLY},
	{"achievement_malta_lair",       AICast_ScriptAction_Achievement_MALTA_LAIR},
	{"achievement_malta_hideout",       AICast_ScriptAction_Achievement_MALTA_HIDEOUT},
	{"achievement_malta_bartender",       AICast_ScriptAction_Achievement_MALTA_BARTENDER},
	{"achievement_malta_betrayer",       AICast_ScriptAction_Achievement_MALTA_BETRAYER},
	{"achievement_malta_agent2",       AICast_ScriptAction_Achievement_MALTA_AGENT2},
	//Ice
	{"achievement_map_ice",       AICast_ScriptAction_Achievement_ICE_BEAT},
	{"achievement_dhIce",       AICast_ScriptAction_Achievement_ICE_DH},
	{"achievement_stealthIce",       AICast_ScriptAction_Achievement_ICE_STEALTH},
	{"achievement_secretIce",       AICast_ScriptAction_Achievement_ICE_SECRET},
	{"achievement_defenseIce",       AICast_ScriptAction_Achievement_ICE_DEFENSE},
	{"achievement_easterIce",       AICast_ScriptAction_Achievement_ICE_EE},
	
	{"achievement_6perks",       AICast_ScriptAction_Achievement_6PERKS},
     // achievements end
	{"endgame",          AICast_ScriptAction_EndGame},				//----(SA)	added
	{"announce",     AICast_ScriptAction_Announce},
	{"teleport",     AICast_ScriptAction_Teleport},					//----(SA)	added
	{"foundsecret",      AICast_ScriptAction_FoundSecret},
	{"nosight",          AICast_ScriptAction_NoSight},
	{"sight",            AICast_ScriptAction_Sight},
	{"noavoid",          AICast_ScriptAction_NoAvoid},
	{"avoid",            AICast_ScriptAction_Avoid},
	{"attrib",           AICast_ScriptAction_Attrib},
	{"denyactivate", AICast_ScriptAction_DenyAction},
	{"lightningdamage",  AICast_ScriptAction_LightningDamage},
	{"deny",         AICast_ScriptAction_DenyAction},
	{"headlook",     AICast_ScriptAction_Headlook},
	{"backupscript", AICast_ScriptAction_BackupScript},
	{"restorescript",    AICast_ScriptAction_RestoreScript},
	{"statetype",        AICast_ScriptAction_StateType},
	{"knockback",        AICast_ScriptAction_KnockBack},
	{"zoom",         AICast_ScriptAction_Zoom},
	{"parachute",        AICast_ScriptAction_Parachute},
	{"cigarette",        AICast_ScriptAction_Cigarette},			//----(SA)	added
	{"startcam",     AICast_ScriptAction_StartCam},
	{"startcamblack",    AICast_ScriptAction_StartCamBlack},
	{"stopcam",          AICast_ScriptAction_StopCam},				//----(SA)	added
	{"entityscriptname",AICast_ScriptAction_EntityScriptName},
	{"aiscriptname", AICast_ScriptAction_AIScriptName},
	{"sethealth",        AICast_ScriptAction_SetHealth},
	{"notarget",     AICast_ScriptAction_NoTarget},
	{"cvar",         AICast_ScriptAction_Cvar},

//----(SA)	added some music interface
	{"mu_start",     AICast_ScriptAction_MusicStart},			// (char *new_music, int time)	// time to fade in
	{"mu_play",          AICast_ScriptAction_MusicPlay},		// (char *new_music)
	{"mu_stop",          AICast_ScriptAction_MusicStop},		// (int time)	// time to fadeout
	{"mu_fade",          AICast_ScriptAction_MusicFade},		// (float target_volume, int time)	// time to fade to target
	{"mu_queue",     AICast_ScriptAction_MusicQueue},			// (char *new_music)	// music that will start when previous fades to 0
//----(SA)	end

	{"explicit_routing", AICast_ScriptAction_ExplicitRouting},
	{"lockplayer",       AICast_ScriptAction_LockPlayer},
	{"screenfade",       AICast_ScriptAction_ScreenFade},
	{"anim_condition",   AICast_ScriptAction_AnimCondition},
	{"pushaway",     AICast_ScriptAction_PushAway},
	{"catchfire",        AICast_ScriptAction_CatchFire},
	{"givescore",        AICast_ScriptAction_GiveScore},		
	{"setwave",        AICast_ScriptAction_SetWave},		

	{NULL,              0}
};

qboolean AICast_EventMatch_StringEqual( cast_script_event_t *event, char *eventParm );
qboolean AICast_EventMatch_IntInRange( cast_script_event_t *event, char *eventParm );

// the list of events that can start an action sequence
// NOTE!!: only append to this list, DO NOT INSERT!!
cast_script_event_define_t scriptEvents[] =
{
	{"spawn",            0},          // called as each character is spawned into the game
	{"playerstart",      0},          // called when player hits 'start' button
	{"enemysight",       AICast_EventMatch_StringEqual}, // enemy has been sighted for the first time (once only)
	{"sight",            AICast_EventMatch_StringEqual}, // non-enemy has been sighted for the first time (once only)
	{"enemydead",        AICast_EventMatch_StringEqual}, // our enemy is now dead
	{"trigger",          AICast_EventMatch_StringEqual}, // something has triggered us (always followed by an identifier)
	{"pain",         AICast_EventMatch_IntInRange},  // we've been hurt
	{"death",            AICast_EventMatch_StringEqual},	// RIP
	{"activate",     AICast_EventMatch_StringEqual}, // "param" has just activated us
	{"enemysightcorpse",AICast_EventMatch_StringEqual},  // sighted the given enemy as a corpse, for the first time
	{"friendlysightcorpse", 0},                       // sighted a friendly as a corpse for the first time
	{"avoiddanger",      AICast_EventMatch_StringEqual}, // we are avoiding something dangerous
	{"blocked",          AICast_EventMatch_StringEqual}, // blocked by someone else
	{"statechange",      AICast_EventMatch_StringEqual}, // changing aistates
	{"bulletimpact", 0},
	{"inspectbodystart", AICast_EventMatch_StringEqual}, // starting to travel to body for inspection
	{"inspectbodyend",   AICast_EventMatch_StringEqual}, // reached body for inspection
	{"inspectsoundstart",    AICast_EventMatch_StringEqual}, // reached sound for inspection
	{"inspectsoundend",  AICast_EventMatch_StringEqual}, // reached sound for inspection
	{"attacksound",      AICast_EventMatch_StringEqual}, // play a custom attack sound, and/or deny playing the default sound
	{"fakedeath",        0},
	{"bulletimpactsound",    0},
	{"inspectfriendlycombatstart", 0},
	{"painenemy",        AICast_EventMatch_StringEqual},
	{"forced_mg42_unmount",  0},
	{"respawn",          0},
	{"wave_start",          0},
	{"wave_end",          0},
	{"start_survival",          0},
	{"specialwave_start",            0},
	{"specialwave_end",          0},
	{NULL,              0}
};


/*
===============
AICast_EventMatch_StringEqual
===============
*/
qboolean AICast_EventMatch_StringEqual( cast_script_event_t *event, char *eventParm ) {
	if ( !event->params || !event->params[0] || ( eventParm && !Q_strcasecmp( event->params, eventParm ) ) ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
===============
AICast_EventMatch_IntInRange
===============
*/
qboolean AICast_EventMatch_IntInRange( cast_script_event_t *event, char *eventParm ) {
	char *pString, *token;
	int int1, int2, eInt;

	// get the cast name
	pString = eventParm;
	token = COM_ParseExt( &pString, qfalse );
	int1 = atoi( token );
	token = COM_ParseExt( &pString, qfalse );
	int2 = atoi( token );

	eInt = atoi( event->params );

	if ( eventParm && eInt > int1 && eInt <= int2 ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
===============
AICast_EventForString
===============
*/
int AICast_EventForString( char *string ) {
	int i;

	for ( i = 0; scriptEvents[i].eventStr; i++ )
	{
		if ( !Q_strcasecmp( string, scriptEvents[i].eventStr ) ) {
			return i;
		}
	}

	return -1;
}

/*
===============
AICast_ActionForString
===============
*/
cast_script_stack_action_t *AICast_ActionForString( cast_state_t *cs, char *string ) {
	int i;

	for ( i = 0; scriptActions[i].actionString; i++ )
	{
		if ( !Q_strcasecmp( string, scriptActions[i].actionString ) ) {
			if ( !Q_strcasecmp( string, "foundsecret" ) ) {
				level.numSecrets++;
				G_SendMissionStats();
			}
			return &scriptActions[i];
		}
	}

	return NULL;
}

/*
=============
AICast_ScriptLoad

  Loads the script for the current level into the buffer
=============
*/
void AICast_ScriptLoad( void ) {
	char filename[MAX_QPATH];
	vmCvar_t mapname;
	fileHandle_t f;
	int len;

	level.scriptAI = NULL;

	trap_Cvar_VariableStringBuffer( "ai_scriptName", filename, sizeof( filename ) );
	if ( strlen( filename ) > 0 ) {
		trap_Cvar_Register( &mapname, "ai_scriptName", "", CVAR_ROM );
	} else {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	}
	//Q_strncpyz( filename, "maps/", sizeof( filename ) );
	// RealRTCW new difficulty system
	if ( g_gameskill.integer == GSKILL_EASY ) {
	Q_strncpyz( filename, "maps/easy/", sizeof( filename ) );
    }
    else if ( g_gameskill.integer == GSKILL_MEDIUM ) {
	Q_strncpyz( filename, "maps/medium/", sizeof( filename ) );
	}
	else if ( g_gameskill.integer == GSKILL_HARD ) {
	Q_strncpyz( filename, "maps/hard/", sizeof( filename ) );
	}
	else if ( g_gameskill.integer == GSKILL_MAX ) {
	Q_strncpyz( filename, "maps/max/", sizeof( filename ) );
	}
	else if ( g_gameskill.integer == GSKILL_REALISM ) {
	Q_strncpyz( filename, "maps/realism/", sizeof( filename ) );
	}
	Q_strcat( filename, sizeof( filename ), mapname.string );
	Q_strcat( filename, sizeof( filename ), ".ai" );

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 ) {

		// Check for original ai script
		trap_Cvar_VariableStringBuffer( "ai_scriptName", filename, sizeof( filename ) );

		Q_strncpyz( filename, "maps/", sizeof( filename ) );
		Q_strcat( filename, sizeof( filename ), mapname.string );
		Q_strcat( filename, sizeof( filename ), ".ai" );

		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		// make sure we clear out the temporary scriptname
		trap_Cvar_Set( "ai_scriptName", "" );
		if ( len < 0 ) {
			return;
		}
	}

	level.scriptAI = G_Alloc( len );
	trap_FS_Read( level.scriptAI, len, f );

	trap_FS_FCloseFile( f );

	return;
}

/*
==============
AICast_ScriptParse

  Parses the script for the given character
==============
*/
#define MAX_SCRIPT_EVENTS   128
cast_script_event_t cast_temp_events[MAX_SCRIPT_EVENTS];
void AICast_ScriptParse( cast_state_t *cs ) {
	gentity_t   *ent;
	char        *pScript;
	char        *token;
	qboolean wantName;
	qboolean inScript;
	int eventNum;
	int numEventItems;
	cast_script_event_t *curEvent;
	char params[MAX_INFO_STRING];
	cast_script_stack_action_t  *action;
	int i;
	int bracketLevel;
	qboolean buildScript;       //----(SA)	added

	if ( !level.scriptAI ) {
		return;
	}

	ent = &g_entities[cs->entityNum];
	if ( !ent->aiName ) {
		return;
	}

	buildScript = qtrue;

	pScript = level.scriptAI;
	wantName = qtrue;
	inScript = qfalse;
	COM_BeginParseSession( "AICast_ScriptParse" );
	bracketLevel = 0;
	numEventItems = 0;

	memset( cast_temp_events, 0, sizeof( cast_temp_events ) );

	while ( 1 )
	{
		token = COM_Parse( &pScript );

		if ( !token[0] ) {
			if ( !wantName ) {
				G_Error( "AICast_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine() );
			}
			break;
		}

		// end of script
		if ( token[0] == '}' ) {
			if ( inScript ) {
				break;
			}
			if ( wantName ) {
				G_Error( "AICast_ScriptParse(), Error (line %d): '}' found, but not expected.\n", COM_GetCurrentParseLine() );
			}
			wantName = qtrue;
		} else if ( token[0] == '{' )    {
			if ( wantName ) {
				G_Error( "AICast_ScriptParse(), Error (line %d): '{' found, NAME expected.\n", COM_GetCurrentParseLine() );
			}
		} else if ( wantName )   {
			if ( !Q_strcasecmp( ent->aiName, token ) ) {
				inScript = qtrue;
				numEventItems = 0;
			}
			wantName = qfalse;
		} else if ( inScript )   {
			if ( !Q_strcasecmp( token, "attributes" ) ) {
				// read in all the attributes
				AICast_CheckLevelAttributes( cs, ent, &pScript );
				continue;
			}
			eventNum = AICast_EventForString( token );
			if ( eventNum < 0 ) {
				G_Error( "AICast_ScriptParse(), Error (line %d): unknown event: %s.\n", COM_GetCurrentParseLine(), token );
			}
			if ( numEventItems >= MAX_SCRIPT_EVENTS ) {
				G_Error( "AICast_ScriptParse(), Error (line %d): MAX_SCRIPT_EVENTS reached (%d)\n", COM_GetCurrentParseLine(), MAX_SCRIPT_EVENTS );
			}

			// if this is a "friendlysightcorpse" event, then disable corpse vis sharing
			if ( !Q_stricmp( token, "friendlysightcorpse" ) ) {
				cs->aiFlags &= ~AIFL_CORPSESIGHTING;
			}

			curEvent = &cast_temp_events[numEventItems];
			curEvent->eventNum = eventNum;
			memset( params, 0, sizeof( params ) );

			// parse any event params before the start of this event's actions
			while ( ( token = COM_Parse( &pScript ) ) && ( token[0] != '{' ) )
			{
				if ( !token[0] ) {
					G_Error( "AICast_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine() );
				}

				if ( eventNum == 13 ) {   // statechange event, check params
					if ( strlen( token ) > 1 ) {
						if ( BG_IndexForString( token, animStateStr, qtrue ) < 0 ) {
							G_Error( "AICast_ScriptParse(), Error (line %d): unknown state type '%s'.\n", COM_GetCurrentParseLine(), token );
						}
					}
				}

				if ( strlen( params ) ) { // add a space between each param
					Q_strcat( params, sizeof( params ), " " );
				}
				Q_strcat( params, sizeof( params ), token );
			}

			if ( strlen( params ) ) { // copy the params into the event
				curEvent->params = G_Alloc( strlen( params ) + 1 );
				Q_strncpyz( curEvent->params, params, strlen( params ) + 1 );
			}

			// parse the actions for this event
			while ( ( token = COM_Parse( &pScript ) ) && ( token[0] != '}' ) )
			{
				if ( !token[0] ) {
					G_Error( "AICast_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine() );
				}

				action = AICast_ActionForString( cs, token );
				if ( !action ) {
					G_Error( "AICast_ScriptParse(), Error (line %d): unknown action: %s.\n", COM_GetCurrentParseLine(), token );
				}

				curEvent->stack.items[curEvent->stack.numItems].action = action;

				memset( params, 0, sizeof( params ) );
				token = COM_ParseExt( &pScript, qfalse );
				for ( i = 0; token[0]; i++ )
				{
					if ( strlen( params ) ) { // add a space between each param
						Q_strcat( params, sizeof( params ), " " );
					}

					if ( i == 0 ) {
						// Special case: playsound's need to be cached on startup to prevent in-game pauses
						if ( !Q_stricmp( action->actionString, "playsound" ) ) {
							G_SoundIndex( token );
						}

//----(SA)	added a bit more
						if (    buildScript && (
									!Q_stricmp( action->actionString, "mu_start" ) ||
									!Q_stricmp( action->actionString, "mu_play" ) ||
									!Q_stricmp( action->actionString, "mu_queue" ) ||
									!Q_stricmp( action->actionString, "startcam" ) ||
									!Q_stricmp( action->actionString, "startcamblack" ) )
								) {
							if ( strlen( token ) ) { // we know there's a [0], but don't know if it's '0'
								trap_SendServerCommand( cs->entityNum, va( "addToBuild %s\n", token ) );
							}
						}

						if ( !Q_stricmp( action->actionString, "giveweapon" ) ) { // register weapon for client pre-loading
							gitem_t *weap = BG_FindItem2( token );    // (SA) FIXME: rats, need to fix this for weapon names with spaces: 'mauser rifle'
//							if(weap)
							RegisterItem( weap );   // don't be nice, just do it.  if it can't find it, you'll bomb out to the error menu
						}
//----(SA)	end
					}

					if ( strrchr( token,' ' ) ) { // need to wrap this param in quotes since it has more than one word
						Q_strcat( params, sizeof( params ), "\"" );
					}

					Q_strcat( params, sizeof( params ), token );

					if ( strrchr( token,' ' ) ) { // need to wrap this param in quotes since it has more than one word
						Q_strcat( params, sizeof( params ), "\"" );
					}

					token = COM_ParseExt( &pScript, qfalse );
				}

				if ( strlen( params ) ) { // copy the params into the event
					curEvent->stack.items[curEvent->stack.numItems].params = G_Alloc( strlen( params ) + 1 );
					Q_strncpyz( curEvent->stack.items[curEvent->stack.numItems].params, params, strlen( params ) + 1 );
				}

				curEvent->stack.numItems++;

				if ( curEvent->stack.numItems >= AICAST_MAX_SCRIPT_STACK_ITEMS ) {
					G_Error( "AICast_ScriptParse(): script exceeded MAX_SCRIPT_ITEMS (%d), line %d\n", AICAST_MAX_SCRIPT_STACK_ITEMS, COM_GetCurrentParseLine() );
				}
			}

			numEventItems++;
		} else    // skip this character completely
		{

			while ( ( token = COM_Parse( &pScript ) ) )
			{
				if ( !token[0] ) {
					G_Error( "AICast_ScriptParse(), Error (line %d): '}' expected, end of script found.\n", COM_GetCurrentParseLine() );
				} else if ( token[0] == '{' ) {
					bracketLevel++;
				} else if ( token[0] == '}' ) {
					if ( !--bracketLevel ) {
						break;
					}
				}
			}
		}
	}

	// alloc and copy the events into the cast_state_t for this cast
	if ( numEventItems > 0 ) {
		cs->castScriptEvents = G_Alloc( sizeof( cast_script_event_t ) * numEventItems );
		memcpy( cs->castScriptEvents, cast_temp_events, sizeof( cast_script_event_t ) * numEventItems );
		cs->numCastScriptEvents = numEventItems;

		cs->castScriptStatus.castScriptEventIndex = -1;
	}
}

/*
================
AICast_ScriptChange
================
*/
void AICast_ScriptChange( cast_state_t *cs, int newScriptNum ) {
	cast_script_status_t scriptStatusBackup;

	cs->scriptCallIndex++;

	// backup the current scripting
	scriptStatusBackup = cs->castScriptStatus;

	// set the new script to this cast, and reset script status
	cs->castScriptStatus.castScriptStackHead = 0;
	cs->castScriptStatus.castScriptStackChangeTime = level.time;
	cs->castScriptStatus.castScriptEventIndex = newScriptNum;
	cs->castScriptStatus.scriptId = scriptStatusBackup.scriptId + 1;
	cs->castScriptStatus.scriptGotoId = -1;
	cs->castScriptStatus.scriptGotoEnt = -1;
	cs->castScriptStatus.scriptFlags |= SFL_FIRST_CALL;

	// try and run the script, if it doesn't finish, then abort the current script (discard backup)
	if ( AICast_ScriptRun( cs, qtrue ) ) {
		// completed successfully
		cs->castScriptStatus.castScriptStackHead = scriptStatusBackup.castScriptStackHead;
		cs->castScriptStatus.castScriptStackChangeTime = scriptStatusBackup.castScriptStackChangeTime;
		cs->castScriptStatus.castScriptEventIndex = scriptStatusBackup.castScriptEventIndex;
		cs->castScriptStatus.scriptId = scriptStatusBackup.scriptId;
		cs->castScriptStatus.scriptFlags = scriptStatusBackup.scriptFlags;
	}
}

/*
================
AICast_ScriptEvent

  An event has occured, for which a script may exist
================
*/
void AICast_ScriptEvent( struct cast_state_s *cs, char *eventStr, char *params ) {
	int i, eventNum;

	eventNum = -1;

	// find out which event this is
	for ( i = 0; scriptEvents[i].eventStr; i++ )
	{
		if ( !Q_strcasecmp( eventStr, scriptEvents[i].eventStr ) ) { // match found
			eventNum = i;
			break;
		}
	}

	if ( eventNum < 0 ) {
		if ( g_cheats.integer ) { // dev mode
			G_Printf( "devmode-> AICast_ScriptEvent(), unknown event: %s\n", eventStr );
		}
	}

	// show debugging info
	if (    (   ( aicast_debug.integer == 1 ) ||
				(   ( aicast_debug.integer == 2 ) &&
					( ( strlen( aicast_debugname.string ) < 1 ) || ( g_entities[cs->entityNum].aiName && !strcmp( aicast_debugname.string, g_entities[cs->entityNum].aiName ) ) ) ) ) ) {
		G_Printf( "(%s) AIScript event: %s %s ", g_entities[cs->entityNum].aiName, eventStr, params );
	}

	cs->aiFlags &= ~AIFL_DENYACTION;

	// see if this cast has this event
	for ( i = 0; i < cs->numCastScriptEvents; i++ )
	{
		if ( cs->castScriptEvents[i].eventNum == eventNum ) {
			if (    ( !cs->castScriptEvents[i].params )
					||  ( !scriptEvents[eventNum].eventMatch || scriptEvents[eventNum].eventMatch( &cs->castScriptEvents[i], params ) ) ) {

				// show debugging info
				if (    (   ( aicast_debug.integer == 1 ) ||
							(   ( aicast_debug.integer == 2 ) &&
								( ( strlen( aicast_debugname.string ) < 1 ) || ( g_entities[cs->entityNum].aiName && !strcmp( aicast_debugname.string, g_entities[cs->entityNum].aiName ) ) ) ) ) ) {
					G_Printf( "found, calling script: (%s) %s %s\n", g_entities[cs->entityNum].aiName, eventStr, params );
				}

				AICast_ScriptChange( cs, i );
				break;
			}
		}
	}

	// show debugging info
	if (    (   ( aicast_debug.integer == 1 ) ||
				(   ( aicast_debug.integer == 2 ) &&
					( ( strlen( aicast_debugname.string ) < 1 ) || ( g_entities[cs->entityNum].aiName && !strcmp( aicast_debugname.string, g_entities[cs->entityNum].aiName ) ) ) ) ) ) {
		if ( i == cs->numCastScriptEvents ) {
			G_Printf( "not found\n" );
		}
	}

}

/*
================
AICast_ForceScriptEvent

  Definately run this event now, overriding any paised state
================
*/
void AICast_ForceScriptEvent( struct cast_state_s *cs, char *eventStr, char *params ) {
	int oldPauseTime;

	oldPauseTime = cs->scriptPauseTime;
	cs->scriptPauseTime = 0;

	AICast_ScriptEvent( cs, eventStr, params );

	cs->scriptPauseTime = oldPauseTime;
}

/*
=============
AICast_ScriptRun

  returns qtrue if the script completed
=============
*/
qboolean AICast_ScriptRun( cast_state_t *cs, qboolean force ) {
	cast_script_stack_t *stack;

	if ( !aicast_scripts.integer ) {
		return qtrue;
	}

	if ( cs->castScriptStatus.castScriptEventIndex < 0 ) {
		return qtrue;
	}

	if ( !cs->castScriptEvents ) {
		cs->castScriptStatus.castScriptEventIndex = -1;
		return qtrue;
	}

	// only allow the PLAYER'S spawn function through if we're NOT still waiting on everything to finish loading in
	if ( !cs->entityNum && saveGamePending && Q_stricmp( "spawn", scriptEvents[cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].eventNum].eventStr ) ) {
		//char loading[4];
		//trap_Cvar_VariableStringBuffer( "savegame_loading", loading, sizeof(loading) );
		//if (strlen( loading ) > 0 && atoi(loading) != 0)	// we're loading a savegame
		return qfalse;
	}

	if ( !force && ( cs->scriptPauseTime >= level.time ) ) {
		return qtrue;
	}

	stack = &cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack;

	if ( !stack->numItems ) {
		cs->castScriptStatus.castScriptEventIndex = -1;
		return qtrue;
	}

	while ( cs->castScriptStatus.castScriptStackHead < stack->numItems )
	{
		//
		// show debugging info
		if (    ( cs->castScriptStatus.castScriptStackChangeTime == level.time ) &&
				(   ( aicast_debug.integer == 1 ) ||
					(   ( aicast_debug.integer == 2 ) &&
						( ( strlen( aicast_debugname.string ) < 1 ) || ( g_entities[cs->entityNum].aiName && !strcmp( aicast_debugname.string, g_entities[cs->entityNum].aiName ) ) ) ) ) ) {
			G_Printf( "(%s) AIScript command: %s %s\n", g_entities[cs->entityNum].aiName, stack->items[cs->castScriptStatus.castScriptStackHead].action->actionString, ( stack->items[cs->castScriptStatus.castScriptStackHead].params ? stack->items[cs->castScriptStatus.castScriptStackHead].params : "" ) );
		}
		//
		if ( !stack->items[cs->castScriptStatus.castScriptStackHead].action->actionFunc( cs, stack->items[cs->castScriptStatus.castScriptStackHead].params ) ) {
			// check that we are still running the same script that we were when we call the action
			if ( cs->castScriptStatus.castScriptEventIndex >= 0 && stack == &cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack ) {
				cs->castScriptStatus.scriptFlags &= ~SFL_FIRST_CALL;
			}
			return qfalse;
		}
		// move to the next action in the script
		cs->castScriptStatus.castScriptStackHead++;
		// record the time that this new item became active
		cs->castScriptStatus.castScriptStackChangeTime = level.time;
		// reset misc stuff
		cs->castScriptStatus.scriptGotoId = -1;
		cs->castScriptStatus.scriptGotoEnt = -1;
		cs->castScriptStatus.scriptFlags |= SFL_FIRST_CALL;
	}

	cs->castScriptStatus.castScriptEventIndex = -1;

	return qtrue;
}
