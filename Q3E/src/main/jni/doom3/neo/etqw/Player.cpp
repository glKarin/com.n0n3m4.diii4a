// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Player.h"
#include "CommandMapInfo.h"
#include "Weapon.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "ContentMask.h"
#include "rules/GameRules.h"
#include "rules/UserGroup.h"
#include "rules/AdminSystem.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleView.h"
#include "vehicles/JetPack.h"
#include "vehicles/VehicleWeapon.h"
#include "Projectile.h"
#include "WorldSpawn.h"
#include "Camera.h"
#include "guis/UserInterfaceLocal.h"
#include "guis/UserInterfaceManager.h"
#include "guis/UIList.h"
#include "structures/DeployRequest.h"
#include "demos/DemoManager.h"
#include "structures/DeployMask.h"
#include "InputMode.h"
#include "misc/PlayerBody.h"
#include "roles/Tasks.h"
#include "misc/WorldToScreen.h"
#include "../decllib/DeclSurfaceType.h"
#include "./effects/FootPrints.h"
#include "../bse/BSEInterface.h"
#include "../bse/BSE_Envelope.h"
#include "../bse/BSE_SpawnDomains.h"
#include "../bse/BSE_Particle.h"
#include "../bse/BSE.h"
#include "../decllib/declTypeHolder.h"
#include "rules/VoteManager.h"
#include "roles/FireTeams.h"
#include "vehicles/Pathing.h"
#include "misc/DefenceTurret.h"
#include "../sdnet/SDNetFriendsManager.h"

#include "botai/Bot.h"
#include "botai/BotThreadData.h"
#include "Misc.h"

#include "guis/GuiSurface.h"
#include "AntiLag.h"

//const int g_defaultPlayerContents = MASK_SHOT_BOUNDINGBOX;
const int g_defaultPlayerContents = CONTENTS_SLIDEMOVER;

const float	PLAYER_ORIGIN_MAX				= 32767;
const int	PLAYER_ORIGIN_TOTAL_BITS		= 24;
const int	PLAYER_ORIGIN_EXPONENT_BITS		= idMath::BitsForInteger( idMath::BitsForFloat( PLAYER_ORIGIN_MAX ) ) + 1;
const int	PLAYER_ORIGIN_MANTISSA_BITS		= PLAYER_ORIGIN_TOTAL_BITS - 1 - PLAYER_ORIGIN_EXPONENT_BITS;

#ifdef PLAYER_DAMAGE_LOG
idCVar g_drawPlayerDamage( "g_drawPlayerDamage", "0", CVAR_BOOL | CVAR_GAME, "Draws numbers above the player's head every time they take damage ( Must be enabled on the server too )" );
#endif // PLAYER_DAMAGE_LOG

idCVar g_pauseNoClip( "g_pauseNoClip", "0", CVAR_GAME | CVAR_BOOL, "lets you noclip around the game while it is paused" );

#define ENABLE_JP_FLOAT_CHECKS
#if defined( ENABLE_JP_FLOAT_CHECKS )
	#undef FLOAT_CHECK_BAD
	#undef VEC_CHECK_BAD

#define FLOAT_CHECK_BAD( x ) \
	if ( FLOAT_IS_NAN( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is NAN", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_INF( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is INF", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_IND( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is IND", __FILE__, __LINE__ ); \
	if ( FLOAT_IS_DENORMAL( x ) ) gameLocal.Error( "Bad floating point number in %s(%i): " #x " is DEN", __FILE__, __LINE__ ); \


	#define VEC_CHECK_BAD( vec )	FLOAT_CHECK_BAD( ( vec ).x ); FLOAT_CHECK_BAD( ( vec ).y ); FLOAT_CHECK_BAD( ( vec ).z );
	#define QUAT_CHECK_BAD( quat )	FLOAT_CHECK_BAD( ( quat ).x ); FLOAT_CHECK_BAD( ( quat ).y ); FLOAT_CHECK_BAD( ( quat ).z ); FLOAT_CHECK_BAD( ( quat ).w );
	#define MAT_CHECK_BAD( m )		VEC_CHECK_BAD( m[ 0 ] ); VEC_CHECK_BAD( m[ 1 ] ); VEC_CHECK_BAD( m[ 2 ] );
	#define ANG_CHECK_BAD( ang )	FLOAT_CHECK_BAD( ( ang ).pitch ); FLOAT_CHECK_BAD( ( ang ).roll ); FLOAT_CHECK_BAD( ( ang ).yaw );

#define FLOAT_FIX_BAD( x ) \
	if ( FLOAT_IS_NAN( x ) ) { gameLocal.Warning( "Bad floating point number in %s(%i): " #x " is NAN", __FILE__, __LINE__ ); x = 0.0f; } \
	if ( FLOAT_IS_INF( x ) ) { gameLocal.Warning( "Bad floating point number in %s(%i): " #x " is INF", __FILE__, __LINE__ ); x = 0.0f; } \
	if ( FLOAT_IS_IND( x ) ) { gameLocal.Warning( "Bad floating point number in %s(%i): " #x " is IND", __FILE__, __LINE__ ); x = 0.0f; } \
	if ( FLOAT_IS_DENORMAL( x ) ) { gameLocal.Warning( "Bad floating point number in %s(%i): " #x " is DEN", __FILE__, __LINE__ ); x = 0.0f; } \

	#define VEC_FIX_BAD( vec )		FLOAT_FIX_BAD( ( vec ).x ); FLOAT_FIX_BAD( ( vec ).y ); FLOAT_FIX_BAD( ( vec ).z );
	#define QUAT_FIX_BAD( quat )	FLOAT_FIX_BAD( ( quat ).x ); FLOAT_FIX_BAD( ( quat ).y ); FLOAT_FIX_BAD( ( quat ).z ); FLOAT_FIX_BAD( ( quat ).w );
	#define MAT_FIX_BAD( m )		VEC_FIX_BAD( m[ 0 ] ); VEC_FIX_BAD( m[ 1 ] ); VEC_FIX_BAD( m[ 2 ] );
	#define ANG_FIX_BAD( ang )		FLOAT_FIX_BAD( ( ang ).pitch ); FLOAT_FIX_BAD( ( ang ).roll ); FLOAT_FIX_BAD( ( ang ).yaw );

#else
	#define MAT_CHECK_BAD( m )
#endif

/*
==============
sdPlayerNetworkData::MakeDefault
==============
*/
void sdPlayerNetworkData::MakeDefault( void ) {
	hasPhysicsData = false;
	physicsData.MakeDefault();
	scriptData.MakeDefault();

	playerStateData.MakeDefault();

	deltaViewAngles[ 0 ]	= 0;
	deltaViewAngles[ 1 ]	= 0;
	viewAngles[ 0 ]			= 0;
	viewAngles[ 1 ]			= 0;
	proxyEntitySpawnId		= 0;
}

/*
==============
sdPlayerNetworkData::Write
==============
*/
void sdPlayerNetworkData::Write( idFile* file ) const {
	file->WriteBool( hasPhysicsData );
	physicsData.Write( file );
	scriptData.Write( file );

	playerStateData.Write( file );

	file->WriteShort( deltaViewAngles[ 0 ] );
	file->WriteShort( deltaViewAngles[ 1 ] );
	file->WriteShort( viewAngles[ 0 ] );
	file->WriteShort( viewAngles[ 1 ] );
	file->WriteInt( proxyEntitySpawnId );
}

/*
==============
sdPlayerNetworkData::Read
==============
*/
void sdPlayerNetworkData::Read( idFile* file ) {
	file->ReadBool( hasPhysicsData );
	physicsData.Read( file );
	scriptData.Read( file );

	playerStateData.Read( file );

	file->ReadShort( deltaViewAngles[ 0 ] );
	file->ReadShort( deltaViewAngles[ 1 ] );
	file->ReadShort( viewAngles[ 0 ] );
	file->ReadShort( viewAngles[ 1 ] );
	file->ReadInt( proxyEntitySpawnId );
}

/*
==============
sdPlayerBroadcastData::MakeDefault
==============
*/
void sdPlayerBroadcastData::MakeDefault( void ) {
	inventoryData.MakeDefault();
	physicsData.MakeDefault();
	playerStateData.MakeDefault();
	scriptData.MakeDefault();

	health				= 0;
	maxHealth			= 0;

	baseDeathYaw		= 0.f;

	targetLockSpawnId	= 0;
	targetLockEndTime	= 0;

	isLagged			= false;
	forceRespawn		= false;
	wantSpawn			= false;
}

/*
==============
sdPlayerBroadcastData::Write
==============
*/
void sdPlayerBroadcastData::Write( idFile* file ) const {
	physicsData.Write( file );
	inventoryData.Write( file );
	playerStateData.Write( file );
	scriptData.Write( file );

	file->WriteShort( health );
	file->WriteShort( maxHealth );

	file->WriteInt( targetLockSpawnId );
	file->WriteInt( targetLockEndTime );

	file->WriteFloat( baseDeathYaw );

	file->WriteBool( isLagged );
	file->WriteBool( forceRespawn );
	file->WriteBool( wantSpawn );
}

/*
==============
sdPlayerBroadcastData::Read
==============
*/
void sdPlayerBroadcastData::Read( idFile* file ) {
	physicsData.Read( file );
	inventoryData.Read( file );
	playerStateData.Read( file );
	scriptData.Read( file );

	file->ReadShort( health );
	file->ReadShort( maxHealth );

	file->ReadInt( targetLockSpawnId );
	file->ReadInt( targetLockEndTime );

	file->ReadFloat( baseDeathYaw );

	file->ReadBool( isLagged );
	file->ReadBool( forceRespawn );
	file->ReadBool( wantSpawn );
}

/*
==============
sdPlayerStateData::MakeDefault
==============
*/
void sdPlayerStateData::MakeDefault( void ) {
	timers.SetNum( 0, false );

	stepUpTime			= 0;
	stepUpDelta			= 0.f;
}

/*
==============
sdPlayerStateData::Write
==============
*/
void sdPlayerStateData::Write( idFile* file ) const {
	file->WriteInt( timers.Num() );
	for ( int i = 0; i < timers.Num(); i++ ) {
		file->WriteInt( timers[ i ] );
	}

	file->WriteInt( stepUpTime );
	file->WriteFloat( stepUpDelta );
}

/*
==============
sdPlayerStateData::Read
==============
*/
void sdPlayerStateData::Read( idFile* file ) {
	int count;
	file->ReadInt( count );

	timers.SetNum( count );
	for ( int i = 0; i < timers.Num(); i++ ) {
		file->ReadInt( timers[ i ] );
	}

	file->ReadInt( stepUpTime );
	file->ReadFloat( stepUpDelta );
}

/*
==============
sdPlayerStateBroadcast::MakeDefault
==============
*/
void sdPlayerStateBroadcast::MakeDefault( void ) {
	inventoryData.MakeDefault();

	lastHitCounter	= 0;
	lastHitEntity	= 0;
	lastHitHeadshot = false;

	lastDamageDecl		= -1;
	lastDamageDir		= 0;
}

/*
==============
sdPlayerStateBroadcast::Write
==============
*/
void sdPlayerStateBroadcast::Write( idFile* file ) const {
	inventoryData.Write( file );

	file->WriteInt( lastHitCounter );
	file->WriteInt( lastHitEntity );

	file->WriteInt( lastDamageDecl );
	file->WriteInt( lastDamageDir );

	file->WriteBool( lastHitHeadshot );
}

/*
==============
sdPlayerStateBroadcast::Read
==============
*/
void sdPlayerStateBroadcast::Read( idFile* file ) {
	inventoryData.Read( file );

	file->ReadInt( lastHitCounter );
	file->ReadInt( lastHitEntity );

	file->ReadInt( lastDamageDecl );
	file->ReadInt( lastDamageDir );

	file->ReadBool( lastHitHeadshot );
}

/*
===============================================================================

	Player control.
	This object handles all player movement and world interaction.

===============================================================================
*/

// distance between ladder rungs (actually is half that distance, but this sounds better)
const int LADDER_RUNG_DISTANCE = 32;

// time before a next or prev weapon switch happens
const int WEAPON_SWITCH_DELAY = 150;

// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

// minimum speed to bob and play run/walk animations at
const float MIN_BOB_SPEED = 5.0f;

const int LIMBO_FORCE_HEALTH = -100;

const int PLAYER_ICON_FLASH_TIME = SEC2MS( 3 );

extern const idEventDef EV_GetRenderViewAngles;
extern const idEventDef EV_GetTeamDamageDone;
extern const idEventDef EV_SetTeamDamageDone;
extern const idEventDef EV_SetArmor;
extern const idEventDef EV_GetArmor;
extern const idEventDef EV_GetEnemy;
extern const idEventDef EV_GetVehicle;
extern const idEventDef EV_Freeze;
extern const idEventDef EV_GetViewAngles;

const idEventDef EV_Player_GetButton( "getButton", 'b', DOC_TEXT( "Returns whether the player is holding down the specified key or not." ), 1, "Key may be PK_ATTACK, PK_RUN, PK_MODESWITCH, PK_MOUSELOOKOFF, PK_SPRINT, PK_ACTIVATE, PK_ALTFIRE, PK_LEANLEFT, PK_LEANRIGHT or PK_TOPHAT.", "d", "key", "Key to check." );
const idEventDef EV_Player_GetMove( "getMove", 'v', DOC_TEXT( "Returns a vector representing the directional keys the player is pressing." ), 0, "The values in the vector will be in the range 0 - 1.\nThe vector is not normalized." );
const idEventDef EV_Player_GetUserCmdAngles( "getUserCmdAngles", 'v', DOC_TEXT( "Returns the raw mouse input angles." ), 0, NULL );
const idEventDef EV_GetViewAngles( "getViewAngles", 'v', DOC_TEXT( "Returns the entity's view angles." ), 0, NULL );
const idEventDef EV_Player_SetViewAngles( "setViewAngles", '\0', DOC_TEXT( "Sets the player's view angles." ), 1, NULL, "v", "angles", "Angles to set." );
const idEventDef EV_Player_GetCameraViewAngles( "getCameraViewAngles", 'v', DOC_TEXT( "Returns the view angles of the player whilst looking through a remote viewing device." ), 0, NULL );
const idEventDef EV_GetRenderViewAngles( "getRenderViewAngles", 'v', DOC_TEXT( "Returns the axes used to render the player's view, converted to angles." ), 0, NULL );
const idEventDef EV_Player_GetViewOrigin( "getViewOrigin", 'v', DOC_TEXT( "Returns the origin used to render the player's view." ), 0, NULL );
const idEventDef EV_Player_GetWeaponEntity( "getWeaponEntity", 'e', DOC_TEXT( "Returns the player's weapon." ), 0, NULL );
const idEventDef EV_Player_IsGunHidden( "isGunHidden", 'b', DOC_TEXT( "Returns the value of the player's ui_showGun setting." ), 0, NULL );
const idEventDef EV_Freeze( "freeze", '\0', DOC_TEXT( "Lock/unlocks the physics for this entity." ), 1, NULL, "b", "value", "Whether to lock or unlock." );
const idEventDef EV_Player_FreezeTurning( "freezeTurning", '\0', DOC_TEXT( "Enables/disables the ability for the player to turn their view." ), 1, NULL, "b", "state", "Whether to freeze or unfreeze." );
const idEventDef EV_Player_SetRemoteCamera( "setRemoteCamera", '\0', DOC_TEXT( "Sets the entity to be used as a remote viewing device for the player." ), 1, NULL, "E", "camera", "Entity to set as the camera." );
const idEventDef EV_Player_SetRemoteCameraViewOffset( "setRemoteCameraViewOffset", '\0', DOC_TEXT( "Sets an offset to be applied to the view from a remote viewing device." ), 1, "See also $event:setRemoteCamera$.", "v", "offset", "The offset to apply." );
const idEventDef EV_Player_GetViewingEntity( "getViewingEntity", 'e', DOC_TEXT( "Returns any active remove viewing device, or if none, returns the player." ), 0, NULL );
const idEventDef EV_Player_GiveProficiency( "giveProficiency", '\0', DOC_TEXT( "Gives XP to the player in the specified category." ), 4, "If a task is supplied, XP will also be awarded to other players on that task.", "d", "type", "Index of the $decl:proficiencyType$.", "f", "scale", "Multiplier on the XP count.", "o", "task", "Task that the XP is being given for, if any.", "s", "reason", "Reason to store in log." );
const idEventDef EV_Player_GiveClassProficiency( "giveClassProficiency", '\0', DOC_TEXT( "Gives XP to the player in their class category." ), 2, NULL, "f", "count", "Amount of XP to give.", "s", "reason", "Reason to store in the log." );
const idEventDef EV_Player_Heal( "heal", 'd', DOC_TEXT( "Gives health to the player, reducing their accumulated team damage taken, and returns the amount that was actually healed." ), 1, NULL, "d", "count", "Amount to heal." );
const idEventDef EV_Player_Unheal( "unheal", 'd', DOC_TEXT( "Removes health from the player, but does not show a damage indicator, and reutrns the amount that was actually removed." ), 1, NULL, "d", "count", "Amount to remove." );
const idEventDef EV_Player_MakeInvulnerable( "makeInvulnerable", '\0', DOC_TEXT( "Makes the player invulnerable for the given length of time." ), 1, NULL, "f", "time", "Time to be invulnerable for, in seconds." );
const idEventDef EV_Player_GiveClass( "giveClass", '\0', DOC_TEXT( "Changes the player's $decl:playerClass$." ), 1, "If the $decl:playerClass$ is not valid for the team the player is on, nothing will be changed.\nIf the name is not a valid $decl:playerClass$, an error will be thrown.", "s", "name", "Name of the $decl:playerClass$ to change to." );
const idEventDef EV_Player_GivePackage( "givePackage", 'b', DOC_TEXT( "Attempts to give the items in an $decl:itemPackageDef$ to the player, and returns whether or not anything was given." ), 1, NULL, "s", "name", "Name of the $decl:itemPackageDef$." );
const idEventDef EV_Player_SetClassOption( "setClassOption", '\0', DOC_TEXT( "Selects a weapon selection option for the player." ), 2, NULL, "d", "index", "Index of the option to set.", "d", "value", "Index of the weapon to select." );
const idEventDef EV_Player_SendToolTip( "sendToolTip", '\0', DOC_TEXT( "Plays the specified $decl:toolTip$." ), 1, "This will only function for the local client.", "d", "index", "Index of the $decl:toolTip$ to play." );
const idEventDef EV_Player_CancelToolTips( "cancelToolTips", '\0', DOC_TEXT( "Cancels the playing $decl:toolTip$ and any pending ones." ), 0, "This will only function for the local client." );
const idEventDef EV_Player_SetWeapon( "setWeapon", '\0', DOC_TEXT( "Selects a weapon by name." ), 2, NULL, "s", "name", "Name of the $decl:invItemDef$ to select.", "b", "instant", "If set, the weapon will not wait for the previous weapon to lower before switching." );
const idEventDef EV_Player_SelectBestWeapon( "selectBestWeapon", '\0', DOC_TEXT( "Selects the player's best available weapon." ), 1, NULL, "b", "allowCurrent", "If not set, and the current weapon is the best weapon, the next best weapon will be chosen." );
const idEventDef EV_Player_GetCurrentWeapon( "getCurrentWeapon", 's', DOC_TEXT( "Returns the name of the $decl:invItemDef$ of the current weapon." ), 0, NULL );
const idEventDef EV_Player_HasWeapon( "hasWeapon", 'b', DOC_TEXT( "Returns whether the player has the specified weapon in their inventory." ), 1, NULL, "s", "name", "Name of the $decl:invItemDef$ to check for." );
const idEventDef EV_GetVehicle( "getVehicle", 'e', DOC_TEXT( "Returns the vehicle currently associated with this entity, or $null$ if none." ), 0, NULL );
const idEventDef EV_Player_EnterDeploymentMode( "enterDeploymentMode", '\0', DOC_TEXT( "Brings up the deployment interface." ), 0, NULL );
const idEventDef EV_Player_ExitDeploymentMode( "exitDeploymentMode", '\0', DOC_TEXT( "Closes the deployment interface." ), 0, NULL );
const idEventDef EV_Player_GetDeployResult( "getDeployResult", 'd', DOC_TEXT( "Returns the deployment state for the supplied $decl:deployObject$ at the location the player is looking at." ), 1, "The result will be DR_CLEAR, DR_WARNING, DR_FAILED, DR_CONDITION_FAILED, or DR_OUT_OF_RANGE.", "d", "index", "Index of the $decl:deployObject$ to check." );
const idEventDef EV_Player_GetDeploymentActive( "getDeploymentActive", 'b', DOC_TEXT( "Returns whether the deployment interface is currently active." ), 0, NULL );
const idEventDef EV_Player_GetAmmoFraction( "getAmmoFraction", 'f', DOC_TEXT( "Returns the fraction of maximum ammo that the player has." ), 0, "This is only valid for the local client, and on the server." );
const idEventDef EV_Player_GetUserName( "getUserName", 's', DOC_TEXT( "Returns the player's name." ), 0, NULL );
const idEventDef EV_Player_GetCleanUserName( "getCleanUserName", 's', DOC_TEXT( "Returns the player's name." ), 0, NULL );
const idEventDef EV_Player_GetClassName( "getClassName", 's', DOC_TEXT( "Returns the name of the player's current $decl:playerClass$." ), 0, "If the player does not currently have a $decl:playerClass$ the result will be an empty string." );
const idEventDef EV_Player_GetCachedClassName( "getCachedClassName", 's', DOC_TEXT( "Returns the name of the player's pending $decl:playerClass$, which they will spawn as at the next spawn." ), 0, "If the player does not have a pending $decl:playerClass$, the result will be an empty string." );
const idEventDef EV_Player_GetPlayerClass( "getPlayerClass", 'd', DOC_TEXT( "Returns the index of the player's current $decl:playerClass$." ), 0, "If the player does not current have a $decl:playerClass$ the result will be -1." );
const idEventDef EV_Player_GetShortRank( "getShortRankName", 'h', DOC_TEXT( "Returns a localized string for the abbreviated version of the player's rank." ), 0, NULL );
const idEventDef EV_Player_GetRank( "getRankName", 'h', DOC_TEXT( "Returns a localized string for the player's rank." ), 0, NULL );
const idEventDef EV_Player_GetProficiency( "getProficiency", 'd', DOC_TEXT( "Returns the level the player has achieved in the specified $decl:proficiencyType$." ), 1, "If the index is out of range, the game will likely crash or return garbage data.", "d", "index", "Index of the $decl:proficiencyType$." );
const idEventDef EV_Player_GetXP( "getXP", 'f', DOC_TEXT( "Returns the amount of XP the player has been awarded in the specified $decl:proficiencyType$." ), 2, "If the index is -1, the overall total XP will be returned.\nIf the index is otherwise out of range, the game will likely crash or return garbage data.\nThe fromBase setting only matters in campaign mode, and does not apply when the index is -1.", "d", "index", "Index of the $decl:proficiencyType$, or -1.", "b", "fromBase", "If set, only the XP from this map will be returned." );
const idEventDef EV_Player_GetCrosshairEntity( "getCrosshairEntity", 'e', DOC_TEXT( "Returns the entity under the player's crosshair, or $null$ if none." ), 0, NULL );
const idEventDef EV_Player_GetCrosshairDistance( "getCrosshairDistance", 'f', DOC_TEXT( "Returns the distance to the item under the crosshair." ), 1, NULL, "b", "needValidInfo", "If set, and there isn't a valid entity under the crosshair, the result will be 'infinity'." );
const idEventDef EV_Player_GetCrosshairEndPos( "getCrosshairEndPos", 'v', DOC_TEXT( "Returns the end position of the crosshair trace." ), 0, NULL );
const idEventDef EV_Player_GetCrosshairStartTime( "getCrosshairStartTime", 'f', DOC_TEXT( "Returns the last game time the player started looking at an object under the crosshair." ), 0, NULL );
const idEventDef EV_Player_SetSpawnPoint( "setSpawnPoint", '\0', DOC_TEXT( "Selects a spawn point for the player." ), 1, NULL, "E", "spawn", "Spawn point to select, or $null$ for the default spawn." );
const idEventDef EV_Player_GetSpawnPoint( "getSpawnPoint", 'e', DOC_TEXT( "Returns the spawn point the player currently has selected, or $null$ if they are using the default spawn." ), 0, NULL );
const idEventDef EV_Player_SetSpeedModifier( "setSpeedModifier", '\0', DOC_TEXT( "Scales the speed all of the player's movement, apart from when prone." ), 1, NULL, "f", "scale", "Scale factor to apply." );
const idEventDef EV_Player_SetSprintScale( "setSprintScale", '\0', DOC_TEXT( "Scales the player's sprint speed." ), 1, NULL, "f", "scale", "Scale factor to apply." );
const idEventDef EV_Player_NeedsRevive( "needsRevive", 'b', DOC_TEXT( "Returns whether the player can current be revived or not." ), 0, NULL );
const idEventDef EV_Player_IsInvulnerable( "isInvulnerable", 'b', DOC_TEXT( "Returns whether the player is currently invulnerable." ), 0, NULL );
const idEventDef EV_Player_Revive( "revive", '\0', DOC_TEXT( "Revives the player, with the given fraction of their max health." ), 2, NULL, "e", "reviver", "Player doing the reviving.", "f", "healthScale", "Fraction of the max health to revive with." );
const idEventDef EV_Player_SetProxyEntity( "setProxyEntity", '\0', DOC_TEXT( "Sets the player's proxy entity and proxy position index." ), 2, NULL, "E", "proxy", "Proxy entity to set, or $null$ if none.", "d", "index", "Position index in the proxy, ignored if proxy is $null$." );
const idEventDef EV_Player_GetProxyEntity( "getProxyEntity", 'e', DOC_TEXT( "Returns the proxy entity for the player, or $null$ if none." ), 0, NULL );
const idEventDef EV_Player_GetProxyAllowWeapon( "getProxyAllowWeapon", 'b', DOC_TEXT( "Returns whether the player is allowed to use their own weapons in their current proxy position." ), 0, "If the player is not currently in a proxy, the result will be false." );
const idEventDef EV_Player_SetSniperAOR( "setSniperAOR", '\0', DOC_TEXT( "Enables/disables sniper AoR mode." ), 1, NULL, "b", "value", "Whether to enable/disable." );
const idEventDef EV_Player_Enter( "enter", '\0', DOC_TEXT( "Puts the player into a proxy entity in the first available position." ), 1, "If the entity is not a valid proxy, or there are no free spaces, nothing will change.", "e", "proxy", "The proxy entity to enter." );
const idEventDef EV_Player_GetKilledTime( "getKilledTime", 'f', DOC_TEXT( "Returns the last game time in seconds that the player was killed." ), 0, NULL );
const idEventDef EV_Player_GetClassKey( "getClassKey", 's', DOC_TEXT( "Looks up meta data from the player's current $decl:playerClass$." ), 1, "If the player does not have a $decl:playerClass$ or the key cannot be found, an empty string will be returned.", "s", "key", "Key to look up." );
const idEventDef EV_Player_ForceRespawn( "forceRespawn", '\0', DOC_TEXT( "Kills the player and taps them out." ), 0, NULL );
const idEventDef EV_Player_CreateIcon( "createIcon", 'd', DOC_TEXT( "Allocates a player icon, and returns a handle to it." ), 3, "If the $decl:material$ is invalid, the result will be -1.\nSee also $event:freeIcon$.\nIf you supply a timeout, you should not try and free the icon manually.", "s", "material", "Name of the $decl:materal$ for the icon.", "d", "priority", "Priority for showing this icon over others.", "f", "timeout", "Timeout in seconds, less that or equal to 0 is no timeout." );
const idEventDef EV_Player_FreeIcon( "freeIcon", '\0', DOC_TEXT( "Frees a player icon previously allocated using $event:createIcon$." ), 1, NULL, "d", "index", "Index of the icon." );

const idEventDef EV_Player_SetAmmo( "setAmmo", '\0', DOC_TEXT( "Sets the amount of ammo the player has of a specified $decl:ammoDef$." ), 2, "If the index is invalid, the game will likely crash.", "d", "index", "Index of the $decl:ammoDef$.", "d", "count", "Amount of ammo to set." );
const idEventDef EV_Player_GetAmmo( "getAmmo", 'd', DOC_TEXT( "Returns the amount of ammo the player has of a specified $decl:ammoDef$." ), 1, "If the index is invalid, the game will likely crash or return garbage data.", "d", "index", "Index of the $decl:ammoDef$." );
const idEventDef EV_Player_SetMaxAmmo( "setMaxAmmo", '\0', DOC_TEXT( "Sets the maximum amount of ammo the player is allowed to carry, of a specified $decl:ammoDef$." ), 2, "If the index is out of range, the game will likely crash.", "d", "index", "Index of the $decl:ammoDef$.", "d", "limit", "Limit to set." );
const idEventDef EV_Player_GetMaxAmmo( "getMaxAmmo", 'd', DOC_TEXT( "Returns the maximum amount of ammo the player is allowed to carry, of a specified $decl:ammoDef$." ), 1, "If the index is invalid, the game will likely crash or return garbage data.", "d", "index", "Index of the $decl:ammoDef$." );

const idEventDef EV_Player_SetTargetTimeScale( "setTargetTimeScale", '\0', DOC_TEXT( "Applies a scale factor to the player's lock ons." ), 1, NULL, "f", "scale", "Scale factor to apply." );

const idEventDef EV_Player_GetRemoteCamera( "getRemoteCamera", 'e', DOC_TEXT( "Returns the player's current remote camera, or $null$ if none." ), 0, NULL );

const idEventDef EV_Player_DisableSprint( "disableSprint", '\0', DOC_TEXT( "Disables/enables the ability for the player to sprint." ), 1, NULL, "b", "value", "Whether to disable or not." );
const idEventDef EV_Player_DisableRun( "disableRun", '\0', DOC_TEXT( "Disables/enables the ability for the player to run." ), 1, "Is disabled, this will implicitly disable sprinting.", "b", "value", "Whether to disable or not." );
const idEventDef EV_Player_DisableFootsteps( "disableFootsteps", '\0', DOC_TEXT( "Disables/enables footstep sounds and decals for the player." ), 1, "This flag will be ignored if the player is disguised.", "b", "value", "Whether to disable or not" );
const idEventDef EV_Player_DisableFallingDamage( "disableFallingDamage", '\0', DOC_TEXT( "Disables/enables damage the player receives from impacting surfaces at speed." ), 1, NULL, "b", "value", "Whether to disable or not." );

const idEventDef EV_Player_Disguise( "disguise", '\0', DOC_TEXT( "Disguises the player as the specified player, or removes disguise if the value is $null$." ), 1, NULL, "E", "player", "Player to disguise as, or $null$ to undisguise." );
const idEventDef EV_Player_SetSpectateClient( "setSpectateClient", '\0', DOC_TEXT( "Sets the player that this player is to spectate, or $null$ to switch to free-fly mode." ), 1, NULL, "E", "player", "Player to spectate, or $null$ for free-fly." );
const idEventDef EV_Player_GetDisguiseClient( "getDisguiseClient", 'e', DOC_TEXT( "Returns the player this player is disguised as, or $null$ if they are not disguised." ), 0, NULL );
const idEventDef EV_Player_IsDisguised( "isDisguised", 'b', DOC_TEXT( "Returns whether the player is disguised or not." ), 0, NULL );
const idEventDef EV_Player_GetViewSkin( "getViewSkin", 's', DOC_TEXT( "Returns the name of the active view $decl:skin$." ), 0, "If there is no active view $decl:skin$, the result will be an empty string." );
const idEventDef EV_Player_SetViewSkin( "setViewSkin", '\0', DOC_TEXT( "Sets the active view $decl:skin$." ), 1, "If the name is not that of a valid $decl:skin$, the view $decl:skin$ will be disabled.", "s", "name", "Name of the $decl:skin$ to apply." );
const idEventDef EV_Player_SetGUIClipIndex( "setGUIClipIndex", '\0', DOC_TEXT( "Informs the GUI system which clip to display on the hud." ), 1, NULL, "d", "index", "Index of the clip to display." );
const idEventDef EV_Player_GetDeploymentRequest( "getDeploymentRequest", 'd', DOC_TEXT( "Returns the index of the $decl:DeployObject$ of the player's current deployment." ), 0, "If the player has no active deployment, the result will be -1." );
const idEventDef EV_Player_GetDeploymentPosition( "getDeploymentPosition", 'v', DOC_TEXT( "Returns the position in the world where the player would deploy from their current view." ), 0, NULL );

const idEventDef EV_Player_GetActiveTask( "getActiveTask", 'o', DOC_TEXT( "Returns the task the player currently has selected." ), 0, NULL );

const idEventDef EV_Player_BinAdd( "binAdd", '\0', DOC_TEXT( "Adds an entity to the player's list of tracked entities." ), 1, "If the entity is already in the list, this request will be ignored.\nA maximum of 32 entities can be stored, an error will be thrown if this limit is exceeded.", "e", "entity", "Entity to add to the list." );
const idEventDef EV_Player_BinGet( "binGet", 'e', DOC_TEXT( "Returns the entity from the player's list of tracked entities at the specified index." ), 1, "If the index is out of range, or there is no entity at that index, $null$ will be returned.", "d", "index", "Index of the entity to return." );
const idEventDef EV_Player_BinGetSize( "binGetSize", 'd', DOC_TEXT( "Returns number of entities in the player's tracked entity list." ), 0, NULL );
const idEventDef EV_Player_BinRemove( "binRemove", '\0', DOC_TEXT( "Removes an entity from the player's list of tracked entities." ), 1, "If the entity is not in the list, nothing will happen.", "e", "entity", "Entity to remove from the list." );

const idEventDef EV_SetArmor( "setArmor", '\0', DOC_TEXT( "Sets the fraction of damage that this entity will ignore." ), 1, NULL, "f", "fraction", "Fraction to set." );
const idEventDef EV_GetArmor( "getArmor", 'f', DOC_TEXT( "Returns the fraction of damage that this entity will ignore." ), 0, NULL );

const idEventDef EV_Player_DisableShadows( "disableShadows", '\0', DOC_TEXT( "Disables/enables shadows for the player." ), 1, NULL, "b", "value", "Whether to disable or not." );

const idEventDef EV_Player_InhibitGuis( "inhibitGuis", '\0', DOC_TEXT( "Disables/enables the ability for the player to interact with in-game guis." ), 1, NULL, "b", "value", "Whether to disable or not." );

const idEventDef EV_Player_GetPostArmFindBestWeapon( "getPostArmFindBestWeapon", 'b', DOC_TEXT( "Returns the value of the player's ui_postArmFindBestWeapon setting." ), 0, NULL );

const idEventDef EV_Player_SameFireTeam( "sameFireTeam", 'b', DOC_TEXT( "Returns whether the player is on the same fire team as this player." ), 1, "If the entity is not a player, the result will be false.", "e", "player", "Other player to check with." );
const idEventDef EV_Player_IsFireTeamLeader( "isFireTeamLeader", 'b', DOC_TEXT( "Returns whether this player is the leader of a fire team." ), 0, NULL );

const idEventDef EV_Player_IsLocalPlayer( "isLocalPlayer", 'b', DOC_TEXT( "Returns whether this player is the local player or not." ), 0, NULL );
const idEventDef EV_Player_IsToolTipPlaying( "isToolTipPlaying", 'b', DOC_TEXT( "Returns whether there are any tooltips playing or not." ), 0, "This only works with the local player." );
const idEventDef EV_Player_IsSinglePlayerToolTipPlaying( "isSinglePlayerToolTipPlaying", 'b', DOC_TEXT( "Returns whether the current tooltip is a single player tooltip, false if there is no tooltip playing." ), 0, "This only works with the local player." );

const idEventDef EV_Player_ResetTargetLock( "resetTargetLock", '\0', DOC_TEXT( "Clears any locking or locked target." ), 0, NULL );
const idEventDef EV_Player_IsLocking( "isLocking", 'b', DOC_TEXT( "Returns whether the player is currently locking on or not." ), 0, NULL );

const idEventDef EV_Player_SetPlayerChargeOrigin( "setPlayerChargeOrigin", "e" );
const idEventDef EV_Player_SetPlayerChargeArmed( "setPlayerChargeArmed", "be" );
const idEventDef EV_Player_SetPlayerItemState( "setPlayerItemState", "eb" );
const idEventDef EV_Player_SetPlayerGrenadeState( "setPlayerGrenadeState", "eb" );
const idEventDef EV_Player_SetPlayerMineState( "setPlayerMineState", "ebb" );
const idEventDef EV_Player_SetPlayerMineArmed( "setPlayerMineArmed", "beb" );
const idEventDef EV_Player_SetPlayerSupplyCrateState( "setPlayerSupplyCrateState", "eb" );
const idEventDef EV_Player_SetPlayerAirStrikeState( "setPlayerAirstrikeState", "ebb" );
const idEventDef EV_Player_SetPlayerCovertToolState( "setPlayerCovertToolState", "eb" );
const idEventDef EV_Player_SetPlayerSpawnHostState( "setPlayerSpawnHostState", "eb" );
const idEventDef EV_Player_SetSmokeNadeState( "setSmokeNadeState", "e" );
const idEventDef EV_Player_SetArtyAttackLocation( "setArtyAttackLocation", "vd" );
const idEventDef EV_Player_SetPlayerKillTarget( "setPlayerKillTarget", "e" );
const idEventDef EV_Player_SetPlayerRepairTarget( "setPlayerRepairTarget", "e" );
const idEventDef EV_Player_SetPlayerPickupRequestTime( "setPlayerPickupRequestTime", "e" );
const idEventDef EV_Player_SetPlayerCommandRequestTime( "setPlayerCommandRequestTime" );
const idEventDef EV_Player_SetTeleporterState( "setTeleporterState", "b" );
const idEventDef EV_Player_SetRepairDroneState( "setRepairDroneState", "b" );
const idEventDef EV_Player_SetForceShieldState( "setForceShieldState", "be" );
const idEventDef EV_Player_IsBot( "isBot", "", 'b' );
const idEventDef EV_Player_SetBotEscort( "setBotEscort", "e" );
const idEventDef EV_PLayer_SetPlayerSpawnHostTarget( "setPlayerSpawnHostTarget", "e" );

const idEventDef EV_Player_EnableClientModelSights( "enableClientModelSights", '\0', DOC_TEXT( "Adds a sight model to the player's view." ), 1, NULL, "s", "name", "Name of the $decl:entityDef$ to spawn the sight model with." );
const idEventDef EV_Player_DisableClientModelSights( "disableClientModelSights", '\0', DOC_TEXT( "Clears any sight model on the player's view." ), 0, NULL );

const idEventDef EV_Player_GetCrosshairTitle( "getCrosshairTitle", 'w', DOC_TEXT( "Returns the title text to use for crosshair info drawing." ), 1, NULL, "b", "allowDisguise", "Whether to include disguise information or not." );

const idEventDef EV_GetTeamDamageDone( "getTeamDamageDone", 'd', DOC_TEXT( "Returns the amount of non-enemy damage that has been applied to the entity." ), 0, NULL );
const idEventDef EV_SetTeamDamageDone( "setTeamDamageDone", '\0', DOC_TEXT( "Modifies the amount of non-enemy damage that has been applied to the entity." ), 1, NULL, "d", "amount", "Amount to set." );


const idEventDef EV_Player_AdjustDeathYaw( "adjustDeathYaw", '\0', DOC_TEXT( "Allows for adjusting the yaw of the player's body whilst waiting for a medic." ), 1, NULL, "f", "yaw", "New yaw to set." );
const idEventDef EV_Player_SetCarryingObjective( "setCarryingObjective", '\0', DOC_TEXT( "Sets flag which controls whether the objective icon is shown by the player's name on the scoreboard." ), 1, NULL, "b", "value", "Whether to set or clear the flag." );
const idEventDef EV_Player_AddDamageEvent( "addDamageEvent", '\0', DOC_TEXT( "Add a damage event for the player." ), 4, NULL, "d", "time", "Time for event.", "f", "dir", "Direction for the damage event.", "f", "damage", "Amount of damage done.", "b", "update", "Continually update direction." );
const idEventDef EV_Player_IsInLimbo( "isInLimbo", 'b', DOC_TEXT( "Returns whether the player is currently in the limbo waiting to respawn." ), 0, NULL );

CLASS_DECLARATION( idActor, idPlayer )
	EVENT( EV_Player_GetButton,				idPlayer::Event_GetButton )
	EVENT( EV_Player_GetMove,				idPlayer::Event_GetMove )
	EVENT( EV_Player_GetUserCmdAngles,		idPlayer::Event_GetUserCmdAngles )
	EVENT( EV_GetViewAngles,				idPlayer::Event_GetViewAngles )
	EVENT( EV_Player_SetViewAngles,			idPlayer::Event_SetViewAngles )
	EVENT( EV_Player_GetCameraViewAngles,	idPlayer::Event_GetCameraViewAngles )
	EVENT( EV_GetRenderViewAngles,			idPlayer::Event_GetRenderViewAngles )
	EVENT( EV_Player_GetViewOrigin,			idPlayer::Event_GetViewOrigin )
	EVENT( EV_Player_GetWeaponEntity,		idPlayer::Event_GetWeaponEntity )
	EVENT( EV_Player_IsGunHidden,			idPlayer::Event_IsGunHidden )
	EVENT( EV_Freeze,						idPlayer::Event_Freeze )
	EVENT( EV_Player_FreezeTurning,			idPlayer::Event_FreezeTurning )
	EVENT( EV_Player_SetRemoteCamera,		idPlayer::Event_SetRemoteCamera )
	EVENT( EV_Player_SetRemoteCameraViewOffset,	idPlayer::Event_SetRemoteCameraViewOffset )
	EVENT( EV_Player_GetViewingEntity,		idPlayer::Event_GetViewingEntity )
	EVENT( EV_SetSkin,						idPlayer::Event_SetSkin )
	EVENT( EV_Player_GiveProficiency,		idPlayer::Event_GiveProficiency )
	EVENT( EV_Player_GiveClassProficiency,	idPlayer::Event_GiveClassProficiency )
	EVENT( EV_Player_Heal,					idPlayer::Event_Heal )
	EVENT( EV_Player_Unheal,				idPlayer::Event_Unheal )
	EVENT( EV_Player_MakeInvulnerable,		idPlayer::Event_MakeInvulnerable )
	EVENT( EV_Player_GiveClass,				idPlayer::Event_GiveClass )
	EVENT( EV_Player_GivePackage,			idPlayer::Event_GivePackage )
	EVENT( EV_Player_SetClassOption,		idPlayer::Event_SetClassOption )
	EVENT( EV_Player_SendToolTip,			idPlayer::Event_SendToolTip )
	EVENT( EV_Player_CancelToolTips,		idPlayer::Event_CancelToolTips )
	EVENT( EV_Player_SetWeapon,				idPlayer::Event_SetWeapon )
	EVENT( EV_Player_SelectBestWeapon,		idPlayer::Event_SelectBestWeapon )
	EVENT( EV_Player_GetCurrentWeapon,		idPlayer::Event_GetCurrentWeapon )
	EVENT( EV_Player_HasWeapon,				idPlayer::Event_HasWeapon )
	EVENT( EV_GetVehicle,					idPlayer::Event_GetVehicle )
	EVENT( EV_Player_GetAmmoFraction,		idPlayer::Event_GetAmmoFraction )
	EVENT( EV_Player_GetUserName,			idPlayer::Event_GetUserName )
	EVENT( EV_Player_GetCleanUserName,		idPlayer::Event_GetCleanUserName )
	EVENT( EV_Player_GetClassName,			idPlayer::Event_GetClassName )
	EVENT( EV_Player_GetCachedClassName,	idPlayer::Event_GetCachedClassName )
	EVENT( EV_Player_GetPlayerClass,		idPlayer::Event_GetPlayerClass )
	EVENT( EV_Player_GetShortRank,			idPlayer::Event_GetShortRank )
	EVENT( EV_Player_GetRank,				idPlayer::Event_GetRank )
	EVENT( EV_Player_GetProficiency,		idPlayer::Event_GetProficiencyLevel )
	EVENT( EV_Player_GetXP,					idPlayer::Event_GetXP )
	EVENT( EV_Player_GetCrosshairEntity,	idPlayer::Event_GetCrosshairEntity )
	EVENT( EV_Player_GetCrosshairDistance,	idPlayer::Event_GetCrosshairDistance )
	EVENT( EV_Player_GetCrosshairEndPos,	idPlayer::Event_GetCrosshairEndPos )
	EVENT( EV_Player_GetCrosshairStartTime,	idPlayer::Event_GetCrosshairStartTime )
	EVENT( EV_Player_EnterDeploymentMode,	idPlayer::Event_DeploymentMode )
	EVENT( EV_Player_ExitDeploymentMode,	idPlayer::Event_ExitDeploymentMode )
	EVENT( EV_Player_GetDeployResult,		idPlayer::Event_GetDeployResult )
	EVENT( EV_Player_GetDeploymentActive,	idPlayer::Event_GetDeploymentActive )
	EVENT( EV_Player_SetSpawnPoint,			idPlayer::Event_SetSpawnPoint )
	EVENT( EV_Player_GetSpawnPoint,			idPlayer::Event_GetSpawnPoint )
	EVENT( EV_Player_SetSpeedModifier,		idPlayer::Event_SetSpeedModifier )
	EVENT( EV_Player_SetSprintScale,		idPlayer::Event_SetSprintScale )
	EVENT( EV_GetEnemy,						idPlayer::Event_GetEnemy )
	EVENT( EV_Player_NeedsRevive,			idPlayer::Event_NeedsRevive )
	EVENT( EV_Player_IsInvulnerable,		idPlayer::Event_IsInvulernable )
	EVENT( EV_Player_Revive,				idPlayer::Event_Revive )
	EVENT( EV_Player_SetProxyEntity,		idPlayer::Event_SetProxyEntity )
	EVENT( EV_Player_GetProxyEntity,		idPlayer::Event_GetProxyEntity )
	EVENT( EV_Player_GetProxyAllowWeapon,	idPlayer::Event_GetProxyAllowWeapon )
	EVENT( EV_Player_SetSniperAOR,			idPlayer::Event_SetSniperAOR )
	EVENT( EV_Player_Enter,					idPlayer::Event_Enter )
	EVENT( EV_Player_GetKilledTime,			idPlayer::Event_GetKilledTime )
	EVENT( EV_Player_GetClassKey,			idPlayer::Event_GetClassKey )
	EVENT( EV_Player_ForceRespawn,			idPlayer::Event_ForceRespawn )

	EVENT( EV_Player_CreateIcon,			idPlayer::Event_CreateIcon )
	EVENT( EV_Player_FreeIcon,				idPlayer::Event_FreeIcon )

	EVENT( EV_Player_SetAmmo,				idPlayer::Event_SetAmmo )
	EVENT( EV_Player_GetAmmo,				idPlayer::Event_GetAmmo )
	EVENT( EV_Player_SetMaxAmmo,			idPlayer::Event_SetMaxAmmo )
	EVENT( EV_Player_GetMaxAmmo,			idPlayer::Event_GetMaxAmmo )

	EVENT( EV_Player_SetTargetTimeScale,	idPlayer::Event_SetTargetTimeScale )

	EVENT( EV_Player_GetRemoteCamera,		idPlayer::Event_GetRemoteCamera )

	EVENT( EV_Player_DisableSprint,			idPlayer::Event_DisableSprint )
	EVENT( EV_Player_DisableRun,			idPlayer::Event_DisableRun )
	EVENT( EV_Player_DisableFootsteps,		idPlayer::Event_DisableFootsteps )
	EVENT( EV_Player_DisableFallingDamage,	idPlayer::Event_DisableFallingDamage )

	EVENT( EV_Player_Disguise,				idPlayer::Event_Disguise )
	EVENT( EV_Player_GetDisguiseClient,		idPlayer::Event_GetDisguiseClient )
	EVENT( EV_Player_IsDisguised,			idPlayer::Event_IsDisguised )

	EVENT( EV_Player_SetSpectateClient,		idPlayer::Event_SetSpectateClient )

	EVENT( EV_Player_SetViewSkin,			idPlayer::Event_SetViewSkin )
	EVENT( EV_Player_GetViewSkin,			idPlayer::Event_GetViewSkin )

	EVENT( EV_Player_SetGUIClipIndex,		idPlayer::Event_SetGUIClipIndex )

	EVENT( EV_Player_GetDeploymentRequest,	idPlayer::Event_GetDeploymentRequest )
	EVENT( EV_Player_GetDeploymentPosition,	idPlayer::Event_GetDeploymentPosition )

	EVENT( EV_Hide,							idPlayer::Event_Hide )
	EVENT( EV_Show,							idPlayer::Event_Show )

	EVENT( EV_Player_GetActiveTask,			idPlayer::Event_GetActiveTask )

	EVENT( EV_Player_BinAdd,				idPlayer::Event_BinAdd )
	EVENT( EV_Player_BinGet,				idPlayer::Event_BinGet )
	EVENT( EV_Player_BinGetSize,			idPlayer::Event_BinGetSize )
	EVENT( EV_Player_BinRemove,				idPlayer::Event_BinRemove )

	EVENT( EV_SetArmor,						idPlayer::Event_SetArmor )
	EVENT( EV_GetArmor,						idPlayer::Event_GetArmor )

	EVENT( EV_Player_DisableShadows,		idPlayer::Event_DisableShadows )

	EVENT( EV_Player_InhibitGuis,			idPlayer::Event_InhibitGuis )

	EVENT( EV_Player_GetPostArmFindBestWeapon, idPlayer::Event_GetPostArmFindBestWeapon )
	EVENT( EV_Player_SameFireTeam,			idPlayer::Event_SameFireTeam )
	EVENT( EV_Player_IsFireTeamLeader,		idPlayer::Event_IsFireTeamLeader )

	EVENT( EV_Player_IsLocalPlayer,			idPlayer::Event_IsLocalPlayer )
	EVENT( EV_Player_IsToolTipPlaying,		idPlayer::Event_IsToolTipPlaying )
	EVENT( EV_Player_IsSinglePlayerToolTipPlaying, idPlayer::Event_IsSinglePlayerToolTipPlaying )
    EVENT( EV_Player_ResetTargetLock,		idPlayer::Event_ResetTargetLock )
	EVENT( EV_Player_IsLocking,				idPlayer::Event_IsLocking )

	EVENT( EV_Player_SetPlayerChargeOrigin,		idPlayer::Event_SetPlayerChargeOrigin )
	EVENT( EV_Player_SetPlayerChargeArmed,		idPlayer::Event_SetPlayerChargeArmed )
	EVENT( EV_Player_SetPlayerItemState,		idPlayer::Event_SetPlayerItemState )
	EVENT( EV_Player_SetPlayerGrenadeState,		idPlayer::Event_SetPlayerGrenadeState )
	EVENT( EV_Player_SetPlayerMineState,		idPlayer::Event_SetPlayerMineState )
	EVENT( EV_Player_SetPlayerMineArmed,		idPlayer::Event_SetPlayerMineArmed )
	EVENT( EV_Player_SetPlayerSupplyCrateState, idPlayer::Event_SetPlayerSupplyCrateState )
	EVENT( EV_Player_SetPlayerAirStrikeState,	idPlayer::Event_SetPlayerAirStrikeState )
	EVENT( EV_Player_SetPlayerCovertToolState,	idPlayer::Event_SetPlayerCovertToolState )
	EVENT( EV_Player_SetPlayerSpawnHostState,	idPlayer::Event_SetPlayerSpawnHostState )
	EVENT( EV_Player_SetSmokeNadeState,			idPlayer::Event_SetSmokeNadeState )
	EVENT( EV_Player_SetArtyAttackLocation,		idPlayer::Event_SetArtyAttackLocation )
	EVENT( EV_Player_SetPlayerKillTarget,		idPlayer::Event_SetPlayerKillTarget )
	EVENT( EV_Player_SetPlayerRepairTarget,		idPlayer::Event_SetPlayerRepairTarget )
	EVENT( EV_Player_SetPlayerPickupRequestTime, idPlayer::Event_SetPlayerPickupRequestTime )
	EVENT( EV_Player_SetPlayerCommandRequestTime, idPlayer::Event_SetPlayerCommandRequestTime )
	EVENT( EV_Player_SetTeleporterState,		idPlayer::Event_SetTeleporterState )
	EVENT( EV_Player_SetForceShieldState,	idPlayer::Event_SetForceShieldState )
	EVENT( EV_Player_SetRepairDroneState,	idPlayer::Event_SetRepairDroneState )
	EVENT( EV_Player_IsBot,					idPlayer::Event_IsBot )
	EVENT( EV_Player_SetBotEscort,			idPlayer::Event_SetBotEscort )
	EVENT( EV_PLayer_SetPlayerSpawnHostTarget, idPlayer::Event_SetPlayerSpawnHostTarget )

	EVENT( EV_Player_EnableClientModelSights,	idPlayer::Event_EnableClientModelSights )
	EVENT( EV_Player_DisableClientModelSights,	idPlayer::Event_DisableClientModelSights )

	EVENT( EV_Player_GetCrosshairTitle,			idPlayer::Event_GetCrosshairTitle )

	EVENT( EV_GetTeamDamageDone,				idPlayer::Event_GetTeamDamageDone )
	EVENT( EV_SetTeamDamageDone,				idPlayer::Event_SetTeamDamageDone )

	EVENT( EV_Player_AdjustDeathYaw,			idPlayer::Event_AdjustDeathYaw )

	EVENT( EV_Player_SetCarryingObjective,		idPlayer::Event_SetCarryingObjective )
	EVENT( EV_Player_AddDamageEvent,			idPlayer::Event_AddDamageEvent )
	EVENT( EV_Player_IsInLimbo,					idPlayer::Event_IsInLimbo )

END_CLASS

/*
===============================================================================

	sdPlayerInteractiveInterface

===============================================================================
*/

/*
==============
sdPlayerInteractiveInterface::Init
==============
*/
void sdPlayerInteractiveInterface::Init( idPlayer* owner ) {
	_owner				= owner;
	_activateFunc		= owner->GetScriptObject()->GetFunction( "OnActivate" );
	_activateHeldFunc	= owner->GetScriptObject()->GetFunction( "OnActivateHeld" );
}

/*
==============
sdPlayerInteractiveInterface::OnActivate
==============
*/
bool sdPlayerInteractiveInterface::OnActivate( idPlayer* player, float distance ) {
	if ( !_activateFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	_owner->GetScriptObject()->CallNonBlockingScriptEvent( _activateFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
==============
sdPlayerInteractiveInterface::OnActivateHeld
==============
*/
bool sdPlayerInteractiveInterface::OnActivateHeld( idPlayer* player, float distance ) {
	if ( !_activateHeldFunc ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	_owner->GetScriptObject()->CallNonBlockingScriptEvent( _activateHeldFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

idCVar g_playerPushForce( "g_playerPushForce", "200", CVAR_GAME | CVAR_FLOAT, "force players can be pushed by other players" );

/*
==============
sdPlayerInteractiveInterface::OnUsed
==============
*/
bool sdPlayerInteractiveInterface::OnUsed( idPlayer* player, float distance ) {
	if ( distance > 32.f ) {
		return false;
	}

	if ( player->IsSpectating() || player->IsDead() ) {
		return false;
	}

	if ( _owner->IsProne() ) {
		return true;
	}

	if ( gameLocal.time < _owner->nextPlayerPushTime ) {
		return true;
	}
	_owner->nextPlayerPushTime = gameLocal.time + SEC2MS( 1 );

	float force = g_playerPushForce.GetFloat();
	if ( _owner->IsCrouching() ) {
		force *= 0.5f;
	}

	_owner->GetPhysics()->ApplyImpulse( -1, vec3_origin, player->GetRenderView()->viewaxis[ 0 ] * force * _owner->GetPhysics()->GetMass() );

	return true;
}

/*
===============================================================================

	idPlayer

===============================================================================
*/

/*
==============
idPlayer::idPlayer
==============
*/
idPlayer::idPlayer( void ) {
	memset( &usercmd, 0, sizeof( usercmd ) );

	playerFlags.noclip				= false;
	playerFlags.godmode				= false;
	playerFlags.weaponChanged		= false;
	playerFlags.ingame				= false;
	playerFlags.sniperAOR			= false;
	playerFlags.clientWeaponLock	= false;
	playerFlags.spawnAnglesSet		= false;
	playerFlags.manualTaskSelection	= false;
	playerFlags.ready				= false;
	playerFlags.carryingObjective	= false;
	spawnAngles						= ang_zero;
	viewAngles						= ang_zero;
	cmdAngles						= ang_zero;
	clientViewAngles				= ang_zero;

	weaponSelectionItemIndex		= -1;
	oldButtons.btnValue				= 0;
	oldFlags						= 0;
	clientOldButtons.btnValue		= 0;
	clientButtons.btnValue			= 0;
	clientButtonsUsed				= false;

	nextCallVoteTime				= 0;

	lastReviveTime					= 0;

	voiceChatLimit					= 0;

	lastAliveTimeRegistered			= 0;
	nextPlayerPushTime				= 0;

	nextBannerPlayTime				= 0;
	nextBannerIndex					= 0;
	nextTeamSwitchTime				= 0;
	nextTeamBalanceSwitchTime		= 0;
	nextReadyToggleTime				= 0;

	gameStartTime					= 0;

	targetNode.SetOwner( this );

	lastDamageDealtTime				= 0;
	lastDamageDealtType				= TA_NEUTRAL;
	newDamageDealt					= false;
	nextSndHitTime					= 0;
	nextTooltipTime					= 0;
	lastDamageFriendlyVO			= 0;

	combatState						= 0;

	maxHealth						= 100;
	health							= maxHealth;

	weapon							= NULL;

	ownsVehicle						= false;
	aasPullPlayer					= false;
	lastOwnedVehicleSpawnID			= -1;
	lastOwnedVehicleTime			= 0;

	rating							= NULL;

	lastDmgTime						= 0;

	nextSuicideTime					= 0;

	playerFlags.forceRespawn		= false;
	spectateClient					= 0;
	disguiseClient					= -1;
	disguiseTeam					= NULL;
	disguiseClass					= NULL;
	disguiseRank					= NULL;
	disguiseRating					= NULL;
	playerFlags.wantSpectate		= true;
	playerFlags.sprintDisabled		= false;
	playerFlags.runDisabled			= false;

	lastHitCounter					= 0;

	activeTask						= -1;

	eyePosition						= EP_INVALID;
	oldEyePosition					= EP_INVALID;
	eyePosChangeTime				= -1;

	firstPersonViewOrigin			= vec3_zero;
	firstPersonViewAxis				= mat3_identity;

	hipJoint						= INVALID_JOINT;
	chestJoint						= INVALID_JOINT;
	torsoJoint						= INVALID_JOINT;
	headJoint						= INVALID_JOINT;

	arms[ 0 ].shoulderJoint			= INVALID_JOINT;
	arms[ 0 ].elbowJoint			= INVALID_JOINT;
	arms[ 0 ].handJoint				= INVALID_JOINT;
	arms[ 0 ].footJoint				= INVALID_JOINT;

	arms[ 1 ].shoulderJoint			= INVALID_JOINT;
	arms[ 1 ].elbowJoint			= INVALID_JOINT;
	arms[ 1 ].handJoint				= INVALID_JOINT;
	arms[ 1 ].footJoint				= INVALID_JOINT;

	headModelJoint					= INVALID_JOINT;
	headModelOffset.Zero();

	bobFoot							= 0;
	bobFrac							= 0.0f;
	bobfracsin						= 0.0f;
	bobCycle						= 0;
	xyspeed							= 0.0f;
	stepUpTime						= 0;
	stepUpDelta						= 0.0f;
	idealLegsYaw					= 0.0f;
	legsYaw							= 0.0f;
	playerFlags.legsForward			= true;
	oldViewYaw						= 0.0f;
	viewBobAngles					= ang_zero;
	viewBob							= vec3_zero;
	landChange						= 0;
	landTime						= 0;

	lastVehicleViewAnglesChange		= 0;

	lastCrosshairTraceTime			= 0;
	crosshairEntActivate			= false;

	lagIcon							= -1;
	godIcon							= -1;

	skin							= NULL;

	proxyEntity						= NULL;
	proxyPositionId					= 0;
	proxyViewMode					= 0;

	remoteCameraViewOffset.Zero();

	memset( &renderView, 0, sizeof( renderView ) );

	focusTime							= 0;
	focusGUIent							= NULL;
	focusUI								= 0;

	lastDamageDecl						= -1;
	lastDamageDir						= vec3_zero;

	inventoryExt.SetOwner( this );

	lastSpectateTeleport				= 0;
	nextSpectateChange					= 0;

	playerFlags.isLagged				= false;

	playerFlags.commandMapInfoVisible	= false;
	weaponSwitchTimeout					= 0;

	viewSkin							= NULL;

	raiseWeaponFunction = NULL;
	lowerWeaponFunction = NULL;
	reloadWeaponFunction = NULL;
	weaponFiredFunction = NULL;
	onProficiencyChangedFunction = NULL;
	onSpawnFunction = NULL;
	onKilledFunction = NULL;
	onHitActivateFunction = NULL;
	onSetTeamFunction = NULL;
	onFullyKilledFunction = NULL;
	onPainFunction = NULL;
	onDisguisedFunction = NULL;
	onConsumeHealthFunction = NULL;
	onConsumeStroyentFunction = NULL;
	onFireTeamJoinedFunction = NULL;
	onFireTeamBecomeLeader = NULL;
	onHealedFunction = NULL;
	onJumpFunction = NULL;
	onSendingVoice = NULL;
	sniperScopeUpFunc = NULL;
	isPantingFunc = NULL;
	needsParachuteFunc = NULL;
	fl.unlockInterpolate				= true;
	waterEffects						= NULL;

	suppressPredictionReset = false;

	sendingVoice = false;

	lastWeaponMenuState					= false;
	lastWeaponSwitchPos					= -1;
	enableWeaponSwitchTimeout			= true;
	lastTimelineTime					= -1;

	currentToolTip						= NULL;
}

/*
==============
idPlayer::OnProficiencyLevelGain
==============
*/
void idPlayer::OnProficiencyLevelGain( const sdDeclProficiencyType* type, int oldLevel, int newLevel ) {
	UpdateRating();

	using namespace sdProperties;
	if ( gameLocal.IsLocalPlayer( this ) ) {

		const sdDeclPlayerClass* pc = GetInventory().GetClass();
		if ( pc != NULL ) {
			for ( int i = 0; i < pc->GetNumProficiencies(); i++ ) {
				const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( i );

				if ( category.index != type->Index() ) {
					continue;
				}

				for ( int level = oldLevel; level < newLevel && level < category.upgrades.Num(); level++ ) {
					const sdDeclLocStr* message = category.upgrades[ level ].text;
					const sdDeclLocStr* messageCategory = category.text;
					if ( gameLocal.rules != NULL ) {
						idVec4 color;
						sdProperties::sdFromString( color, sdGameRules::g_chatDefaultColor.GetString() );
						gameLocal.rules->AddChatLine( sdGameRules::CHAT_MODE_DEFAULT, color, L"%ls", message->GetText() );
					}
					if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" ) ) {
						if( sdProperty* property = scope->GetProperty( "proficiencyText", PT_WSTRING ) ) {
							if( message != NULL ) {
								*property->value.wstringValue = message->GetText();
							} else {
								*property->value.wstringValue = L"";
								gameLocal.Warning( "idPlayer::OnProficiencyLevelGain: NULL upgrade message" );
							}
						}
						if( sdProperty* property = scope->GetProperty( "proficiencyCategoryText", PT_WSTRING ) ) {
							if( messageCategory != NULL ) {
								*property->value.wstringValue = messageCategory->GetText();
							}
						}
						if( sdProperty* property = scope->GetProperty( "proficiencyCategoryTime", PT_FLOAT ) ) {
							*property->value.floatValue = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );
						}
						// must setup sound string property before icon
						if( sdProperty* property = scope->GetProperty( "proficiencySound", PT_STRING ) ) {
							*property->value.stringValue = category.upgrades[ level ].sound;
						}
						if( sdProperty* property = scope->GetProperty( "proficiencyIcon", PT_STRING ) ) {
							*property->value.stringValue = category.upgrades[ level ].materialInfo;
						}
					} else {
						gameLocal.Warning( "idPlayer::OnProficiencyLevelGain: Couldn't find global 'gameHud' scope in guiGlobals." );
					}
/* jrad - handled by the GUI now
					if ( category.upgrades[ level ].toolTip != NULL ) {
						SendToolTip( category.upgrades[ level ].toolTip );
					}
*/
				}
				break;
			}
		}
	}

	const sdDeclProficiencyType::stats_t& stats = type->GetStats();
	if ( !stats.name.IsEmpty() ) {
		sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

		for ( int level = oldLevel; level < newLevel; level++ ) {
			sdPlayerStatEntry* stat = tracker.GetStat( tracker.AllocStat( va( "%s_level_%i", stats.name.c_str(), level + 1 ), sdNetStatKeyValue::SVT_INT ) );
			if ( !stat ) {
				continue;
			}

			stat->IncreaseValue( entityNumber, 1 );
		}
	}

	sdScriptHelper h1;
	h1.Push( type->Index() );
	h1.Push( oldLevel );
	h1.Push( newLevel );
	scriptObject->CallNonBlockingScriptEvent( onProficiencyChangedFunction, h1 );

	botThreadData.UpdateClientAbilities( entityNumber, newLevel );
}

/*
==============
idPlayer::OnProficiencyGain
==============
*/
void idPlayer::OnProficiencyGain( int index, float count, const char* reason ) {
	const sdDeclProficiencyType* type = gameLocal.declProficiencyTypeType[ index ];

	const sdDeclProficiencyType::stats_t& stats = type->GetStats();
	if ( stats.xp != NULL ) {
		stats.xp->IncreaseValue( entityNumber, count );
	}
	if ( stats.totalXP != NULL ) {
		stats.totalXP->IncreaseValue( entityNumber, count );
	}

	if ( g_debugProficiency.GetBool() ) {
		idStr text = va( "Player '%s' Gained %f Points in %s category, for %s\n", userInfo.name.c_str(), count, type->GetLookupTitle(), reason != NULL ? reason : "Unknown Reason" );
		text.RemoveColors();

		gameLocal.LogProficiency( text.c_str() );
		gameLocal.Printf( text.c_str() );
		if ( stats.xp == NULL || stats.totalXP == NULL ) {
			gameLocal.Printf( "No stats for: %s\n", type->GetName() );
		}
	}
}

/*
==============
idPlayer::LinkScriptVariables

set up conditions for animation
==============
*/
void idPlayer::LinkScriptVariables( void ) {
	AI_SPRINT.LinkTo(			*scriptObject, "AI_SPRINT" );
	AI_FORWARD.LinkTo(			*scriptObject, "AI_FORWARD" );
	AI_BACKWARD.LinkTo(			*scriptObject, "AI_BACKWARD" );
	AI_STRAFE_LEFT.LinkTo(		*scriptObject, "AI_STRAFE_LEFT" );
	AI_STRAFE_RIGHT.LinkTo(		*scriptObject, "AI_STRAFE_RIGHT" );
	AI_ATTACK_HELD.LinkTo(		*scriptObject, "AI_ATTACK_HELD" );
	AI_WEAPON_FIRED.LinkTo(		*scriptObject, "AI_WEAPON_FIRED" );
	AI_JUMP.LinkTo(				*scriptObject, "AI_JUMP" );
	AI_DEAD.LinkTo(				*scriptObject, "AI_DEAD" );
	AI_CROUCH.LinkTo(			*scriptObject, "AI_CROUCH" );
	AI_PRONE.LinkTo(			*scriptObject, "AI_PRONE" );
	AI_ONGROUND.LinkTo(			*scriptObject, "AI_ONGROUND" );
	AI_ONLADDER.LinkTo(			*scriptObject, "AI_ONLADDER" );
	AI_HARDLANDING.LinkTo(		*scriptObject, "AI_HARDLANDING" );
	AI_SOFTLANDING.LinkTo(		*scriptObject, "AI_SOFTLANDING" );
	AI_RUN.LinkTo(				*scriptObject, "AI_RUN" );
	AI_RELOAD.LinkTo(			*scriptObject, "AI_RELOAD" );
	AI_TELEPORT.LinkTo(			*scriptObject, "AI_TELEPORT" );
	AI_TURN_LEFT.LinkTo(		*scriptObject, "AI_TURN_LEFT" );
	AI_TURN_RIGHT.LinkTo(		*scriptObject, "AI_TURN_RIGHT" );
	AI_PUTAWAY_ACTIVE.LinkTo(	*scriptObject, "AI_PUTAWAY_ACTIVE" );
	AI_TAKEOUT_ACTIVE.LinkTo(	*scriptObject, "AI_TAKEOUT_ACTIVE" );
	AI_LEAN.LinkTo(				*scriptObject, "AI_LEAN" );
	AI_INWATER.LinkTo(			*scriptObject, "AI_INWATER" );
}

/*
================
idPlayer::UpdateAnimationControllers
================
*/
bool idPlayer::UpdateAnimationControllers( void ) {
	return false;
}

/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity( int spawnId ) {
	weapon.ForceSpawnId( spawnId );
	if ( !weapon.IsValid() ) {
		gameLocal.Warning( "idPlayer::SetupWeaponEntity Invalid Weapon" );
	} else {
		weapon->SetOwner( this );
		if ( gameLocal.IsLocalViewPlayer( this ) ) {
			gameLocal.localPlayerProperties.SetActiveWeapon( weapon );
		}
		playerFlags.weaponChanged = true;
	}
}

/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity( void ) {
	if ( weapon.GetEntity() ) {
		// get rid of old weapon
		weapon.GetEntity()->Clear();
	} else if ( !gameLocal.isClient ) {
		weapon = gameLocal.SpawnEntityTypeT< idWeapon >( false );
		if ( gameLocal.isServer ) {
			sdEntityBroadcastEvent msg( this, EVENT_SETWEAPON );
			msg.WriteLong( weapon.GetSpawnId() );
			msg.Send( true, sdReliableMessageClientInfoAll() );
		}
		weapon->SetOwner( this );
		if ( gameLocal.IsLocalViewPlayer( this ) ) {
			gameLocal.localPlayerProperties.SetActiveWeapon( weapon );
		}
	}
}

/*
==============
idPlayer::ShutdownThreads
==============
*/
void idPlayer::ShutdownThreads( void ) {
	idActor::ShutdownThreads();

	GetInventory().ShutdownThreads();
}

/*
==============
idPlayer::HasAbility
==============
*/
bool idPlayer::HasAbility( qhandle_t handle ) const {
	if ( GetInventory().HasAbility( handle ) ) {
		return true;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		sdUsableInterface* iface = proxy->GetUsableInterface();
		if ( iface ) {
			if ( iface->HasAbility( handle, this ) ) {
				return true;
			}
		}
	}

	for ( int i = 0; i < abilityEntityBin.Num(); i++ ) {
		idEntity* entity = abilityEntityBin[ i ].GetEntity();
		if ( entity == NULL ) {
			continue;
		}

		if ( entity->HasAbility( handle ) ) {
			return true;
		}
	}

	return false;
}

/*
==============
idPlayer::Init
==============
*/
void idPlayer::Init( bool setWeapon ) {
	lastTimeInPlayZone				= -1;
	playerFlags.inPlayZone			= false;

	targetEntity					= NULL;
	targetEntityPrevious			= NULL;
	targetLockEndTime				= 0;
	targetLockLastTime				= 0;
	targetLockSafeTime				= 0;
	targetLockTimeScale				= 1.f;
	playerFlags.targetLocked		= false;

	playerFlags.sniperAOR			= false;

	playerFlags.weaponLock			= false;
	playerFlags.clientWeaponLock	= false;
	playerFlags.sprintDisabled		= false;
	playerFlags.runDisabled			= false;
	playerFlags.noFootsteps			= false;
	playerFlags.noFallingDamage		= false;

	playerFlags.noShadow			= false;

	playerFlags.carryingObjective	= false;

	armor							= 0.f;

	nextTeleportTime				= 0;
	lastTeleportTime				= 0;

	nextHealthCheckTime				= 0;

	// make sure the end of tooltip events are run
	if ( currentToolTip != NULL ) {
		CancelToolTipTimeline();
	}

	currentToolTip					= NULL;
	lastTimelineTime				= -1;

	teamDamageDone					= 0;

	eyePosition						= EP_NORMAL;
	oldEyePosition					= EP_NORMAL;
	eyeOffset						= GetEyeOffset( eyePosition );

	nextBattleSenseBonusTime		= gameLocal.time + SEC2MS( 45.f );

	raiseWeaponFunction = scriptObject->GetFunction( "RaiseWeapon" );
	lowerWeaponFunction = scriptObject->GetFunction( "LowerWeapon" );
	reloadWeaponFunction = scriptObject->GetFunction( "ReloadWeapon" );
	weaponFiredFunction = scriptObject->GetFunction( "OnWeaponFired" );

	onProficiencyChangedFunction = scriptObject->GetFunction( "OnProficiencyChanged" );
	onSpawnFunction = scriptObject->GetFunction( "OnSpawn" );
	onKilledFunction = scriptObject->GetFunction( "OnKilled" );
	onHitActivateFunction = scriptObject->GetFunction( "OnHitActivate" );
	onSetTeamFunction = scriptObject->GetFunction( "OnSetTeam" );
	onFullyKilledFunction = scriptObject->GetFunction( "OnFullyKilled" );
	onPainFunction = scriptObject->GetFunction( "OnPain" );
	onDisguisedFunction = scriptObject->GetFunction( "OnDisguised" );
	onConsumeHealthFunction = scriptObject->GetFunction( "OnConsumeHealth" );
	onConsumeStroyentFunction = scriptObject->GetFunction( "OnConsumeStroyent" );
	onFireTeamJoinedFunction = scriptObject->GetFunction( "OnFireTeamJoined" );
	onFireTeamBecomeLeader = scriptObject->GetFunction( "OnFireTeamBecomeLeader" );
	onHealedFunction = scriptObject->GetFunction( "OnHealed" );
	onJumpFunction = scriptObject->GetFunction( "OnJumpHeld" );
	onSendingVoice = scriptObject->GetFunction( "OnSendingVoice" );
	sniperScopeUpFunc = scriptObject->GetFunction( "IsSniperScopeUp" );
	isPantingFunc = scriptObject->GetFunction( "PlayerIsPanting" );
	needsParachuteFunc = scriptObject->GetFunction( "NeedsParachute" );

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.InitGUIStates();
	}

	playerFlags.noclip				= false;
	SetGodMode( false );
	playerFlags.turningFrozen		= false;

	memset( &oldButtons, 0, sizeof( oldButtons ) );

	oldFlags				= 0;

	memset( &clientOldButtons, 0, sizeof( clientOldButtons ) );
	memset( &clientButtons, 0, sizeof( clientButtons ) );

	lastDmgTime				= 0;
	lastGroundContactTime	= 0;

	bobCycle				= 0;
	bobFrac					= 0.0f;
	landChange				= 0;
	landTime				= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= 0;

	baseDeathYaw			= 0.f;

	// damage values
	fl.takedamage			= true;

	if ( gameLocal.IsLocalPlayer( this ) ) {
		// remove any damage effects
		gameLocal.playerView.ClearEffects();
	}

	ClearPain();
	ClearDamageEvents();
	ClearDamageDealt();

	if ( !gameLocal.isClient ) {
		GetProficiencyTable().SetSpawnLevels();
	}

	// set up the inventory
	GetInventory().Init( spawnArgs, false, setWeapon );

	lastVehicleViewAnglesChange = 0;
	lastVehicleViewAngles.Zero();


	bobCycle			= 0;
	invulnerableEndTime	= 0;
	health				= maxHealth;

	SetupWeaponEntity();

	idealLegsYaw = 0.0f;
	legsYaw = 0.0f;
	playerFlags.legsForward	= true;
	oldViewYaw = 0.0f;

	speedModifier		= 1.f;
	sprintScale			= 1.f;
	swayScale			= 1.f;

	// set the gravity
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.ResetProne();

	stepUpTime = 0;
	stepUpDelta = 0.0f;
	viewBobAngles.Zero();
	viewBob.Zero();

	weaponAngVel.Zero();
	lastWeaponViewAngles.Zero();
	lastWeaponViewAnglesValid = false;


	// initialize the script variables
	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_DEAD			= false;
	AI_CROUCH		= false;
	AI_PRONE		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RUN			= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;
	AI_SPRINT		= false;
	AI_LEAN			= 0.f;
	AI_INWATER		= false;

	lastGroundContactTime = gameLocal.time;

	nextSpectateChange	= 0;

	SetRemoteCamera( NULL, false );
	remoteCameraViewOffset.Zero();

	viewSkin = NULL;
	playerFlags.scriptHide = false;

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.SetLocalPlayer( this );
	}

	suppressPredictionReset = false;
	ResetAntiLag();

	lookAtTask		= -1;
}

/*
==============
idPlayer::Revive
==============
*/
void idPlayer::Revive( idPlayer* other, float healthScale ) {
	GetInventory().LogRevive();

	// Gordon: we need to save these off, if just passed by reference, they may be cleared, which is teh suck
	idVec3		org = GetPhysics()->GetOrigin();
	idAngles	ang = viewAngles;
	ang.pitch = 0.f; // Gordon: clear the pitch though, since you would have been looking up ( generally ) before, so you don't wanna look at the sky

	// is this spot clear?
	// checking using the appropriate clip model for the pose
	const idClipModel* clipModel = physicsObj.GetNormalClipModel();
	const idClipModel* proneLegsModel = NULL;
	idVec3 proneLegsOffset = vec3_origin;
	if ( physicsObj.IsCrouching() ) {
		clipModel = physicsObj.GetCrouchClipModel();
	} else if ( physicsObj.IsProne() ) {
		clipModel = physicsObj.GetProneClipModel();
		proneLegsModel = physicsObj.GetProneLegsClipModel();
		proneLegsOffset = physicsObj.GetProneLegsPos( vec3_origin, ang );
	}

	idVec3 offset = vec3_origin;
	// if we're coincident with any other players jiggle the offset a bit to get us off their exact origin
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* other = gameLocal.GetClient( i );
		if ( other == NULL || !( other->GetPhysics()->GetContents() & MASK_PLAYERSOLID ) ) {
			continue;
		}

		idVec3 delta = other->GetPhysics()->GetOrigin() - org;
		if ( delta.LengthSqr() < 0.01f ) {
			// coincident - add a random jiggle
			float direction = gameLocal.random.RandomFloat() * idMath::TWO_PI;
			offset.Set( 0.1f * idMath::Sin( direction ), 0.1f * idMath::Cos( direction ), 0.0f );
		}
	}

	int contents = gameLocal.clip.Contents( org + offset, clipModel, mat3_identity, MASK_PLAYERSOLID, this );
	if ( proneLegsModel != NULL ) {
		contents |= gameLocal.clip.Contents( org + offset + proneLegsOffset, proneLegsModel, mat3_identity, MASK_PLAYERSOLID, this );
	}

	if ( contents ) {
		// ideal spot blocked
		// sweep around to find a suitable spot to spawn
		// favour the direction the reviving player is looking in
		float halfWidth = pm_bboxwidth.GetFloat() * idMath::SQRT_TWO * 0.5f;
		idVec3 reachableOffset( 0.0f, 0.0f, clipModel->GetBounds()[ 1 ].z * 0.5f );
		float startAngle = other != NULL ? other->firstPersonViewAxis.ToAngles().yaw : 0;

		for ( float angle = startAngle; angle < startAngle + 360.0f; angle += 45.0f ) {
			idAngles sweepAngle( 0.0f, angle, 0.0f );
			idVec3 sweepOffsetBase;
			sweepAngle.ToVectors( &sweepOffsetBase, NULL, NULL );

			for ( float radius = halfWidth; radius <= halfWidth * 4.0f; radius += halfWidth ) {
				// find the offset for this position
				idVec3 sweepOffset = radius * sweepOffsetBase;
				sweepOffset += offset;

				int contents = gameLocal.clip.Contents( org + sweepOffset, clipModel, mat3_identity, MASK_PLAYERSOLID, this );
				if ( proneLegsModel != NULL ) {
					contents |= gameLocal.clip.Contents( org + sweepOffset + proneLegsOffset, proneLegsModel, mat3_identity, MASK_PLAYERSOLID, this );
				}

				if ( contents ) {
					// invalid spot
//					gameRenderWorld->DebugBounds( colorRed, clipModel->GetBounds(), org + sweepOffset, mat3_identity, 10000 );
					continue;
				}

				// make sure its reachable from the spawn spot
				idVec3 start = org + reachableOffset;
				idVec3 end = org + sweepOffset + reachableOffset;
				trace_t trace;
				gameLocal.clip.TracePoint( trace, start, end, MASK_PLAYERSOLID & ( ~CONTENTS_SLIDEMOVER ), this );
				if ( trace.fraction < 1.0f ) { 
					// can't reach it
//					gameRenderWorld->DebugBounds( colorRed, clipModel->GetBounds(), org + sweepOffset, mat3_identity, 10000 );
					continue;
				}

//				gameRenderWorld->DebugBounds( colorGreen, clipModel->GetBounds(), org + sweepOffset, mat3_identity, 10000 );

				// found a valid spot
				org += sweepOffset;
				angle = startAngle + 360.0f;
				break;
			}
		}
	} else {
		org += offset;
	}

	SpawnToPoint( org, ang, true, false );
	SetHealth( maxHealth * healthScale );

	UpdateCombatModel();
}

/*
==============
idPlayer::Spawn

Prepare any resources used by the player.
==============
*/
void idPlayer::Spawn( void ) {
	idStr		temp;
	idBounds	bounds;

	if ( entityNumber >= MAX_CLIENTS ) {
		gameLocal.Error( "entityNum > MAX_CLIENTS for player.  Player may only be spawned with a client." );
	}

#ifdef PLAYER_DAMAGE_LOG
	InitDamageLog();
#endif // PLAYER_DAMAGE_LOG

	// allow thinking during cinematics
	playerFlags.weaponChanged = false;

	// set our collision model
	physicsObj.SetSelf( this );
	physicsObj.SetupPlayerClipModels();
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetContents( g_defaultPlayerContents );
	physicsObj.SetContents( CONTENTS_RENDERMODEL, 1 );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	SetPhysics( &physicsObj );

	// set up the inventory ( including abilities based stuff )
	GetInventory().Init( spawnArgs, true, false );

	SetLastDamageDealtTime( 0 );
	ClearDamageEvents();

	// set up conditions for animation
	LinkScriptVariables();

	animator.RemoveOriginOffset( true );

	UserInfoChanged();

	// supress model in non-player views, but allow it in mirrors and remote views
	renderEntity.suppressSurfaceInViewID = entityNumber + 1;

	renderEntity.noSelfShadowInViewID = entityNumber + 1;

	// always start in spectating state waiting to be spawned in
	// do this before SetClipModel to get the right bounding box
	Spectate();

	Init( false );
	if ( !gameLocal.isClient ) {
		idVec3		spawnOrigin;
		idAngles	spawnAngles;
		gameLocal.SelectInitialSpawnPoint( this, spawnOrigin, spawnAngles );
		SpawnToPoint( spawnOrigin, spawnAngles, false, false );

		// set yourself ready to spawn. idMultiplayerGame will decide when/if appropriate and call SpawnFromSpawnSpot
		SetSpectateOrigin();
		SetupWeaponEntity();
		assert( IsSpectator() );
	}

	targetIdRangeNormal		= spawnArgs.GetFloat( "target_id_range", "8192" );

	gameLocal.ParseClamp( deadClampPitch, "dead_clamp_pitch", spawnArgs );
	gameLocal.ParseClamp( deadClampYaw, "dead_clamp_yaw", spawnArgs );

	gameLocal.ParseClamp( proneClampPitch, "prone_clamp_pitch", spawnArgs );
	gameLocal.ParseClamp( proneClampYaw, "prone_clamp_yaw", spawnArgs );

	physicsObj.SetProneTime( idPhysics_Player::PT_CROUCH_TO_PRONE, spawnArgs.GetInt( "prone_crouch_to_prone_time", "1500" ));
	physicsObj.SetProneTime( idPhysics_Player::PT_PRONE_TO_CROUCH, spawnArgs.GetInt( "prone_prone_to_crouch_time", "1500" ));
	physicsObj.SetProneTime( idPhysics_Player::PT_STAND_TO_PRONE, spawnArgs.GetInt( "prone_stand_to_prone_time", "3000" ));
	physicsObj.SetProneTime( idPhysics_Player::PT_PRONE_TO_STAND, spawnArgs.GetInt( "prone_prone_to_stand_time", "3000" ));

	UpdateShadows();
	UpdateVisuals();
	Present();

	UpdateCombatModel();
	SetSelectionCombatModel();

	if ( entityNumber >= gameLocal.numClients ) {
		gameLocal.numClients = entityNumber + 1;
	}

	if ( !gameLocal.isClient ) {
		gameLocal.SetupGameStateBase( gameLocal.GetNetworkInfo( entityNumber ) );
		gameLocal.SetupEntityStateBase( gameLocal.GetNetworkInfo( entityNumber ) );
	}

	gameLocal.rules->SpawnPlayer( this );

	interactiveInterface.Init( this );

	spectateClient = entityNumber;

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.UpdateTeam( team );
		gameLocal.localPlayerProperties.SetClipIndex( 0 );
		if ( !sdDemoManager::GetInstance().InPlayBack() && com_timeServer.GetInteger() <= 0 ) {
			gameLocal.localPlayerProperties.GetLimboMenu()->Enable( true, true );
		}
	}

	lastTeamSetTime						= gameLocal.time;

	damageXPScale						= spawnArgs.GetFloat( "damage_xp_scale", "1" );

	oobDamageInterval					= spawnArgs.GetInt( "oob_damage_interval", "500" );
	const char* oobDamageName			= spawnArgs.GetString( "dmg_oob" );

	healthCheckTickTime					= SEC2MS( spawnArgs.GetFloat( "health_check_interval" ) );
	healthCheckTickCount				= spawnArgs.GetInt( "health_check_count" );

	BecomeActive( TH_THINK );

	playerIcon.Init( spawnArgs );
	playerIconFlashTime = -1;

	waterEffects = sdWaterEffects::SetupFromSpawnArgs( spawnArgs );

	CalculateRenderView();

	predictionErrorDecay_Origin.Init( this );
	predictionErrorDecay_Angles.Init( this );
}

/*
==============
idPlayer::PlayQuickChat
==============
*/
void idPlayer::PlayQuickChat( const idVec3& location, int sendingClientNum, const sdDeclQuickChat* quickChat, idEntity* target ) {
	if ( quickChat == NULL ) {
		return;
	}

	idPlayer* other = sendingClientNum == -1 ? NULL : gameLocal.GetClient( sendingClientNum );

	if ( gameLocal.isClient ) {
		if ( other != NULL ) {
			const char* callback = quickChat->GetCallback();
			if ( *callback ) {
				idScriptObject* obj = other->GetScriptObject();
				sdScriptHelper helper;
				if ( target != NULL ) {
					helper.Push( target->GetScriptObject() );
				}
				obj->CallNonBlockingScriptEvent( obj->GetFunction( callback ), helper );
			}
		}
	}

	// Gordon: Hmmmm, this looks dodgy
	if ( !gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
		int spawnID = -1;
		if ( target != NULL ) {
			spawnID = gameLocal.GetSpawnId( target );
		}

		sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_QUICK_CHAT );
		outMsg.WriteVector( location );
		outMsg.WriteChar( sendingClientNum );
		outMsg.WriteLong( quickChat->Index() );
		outMsg.WriteLong( spawnID );
		outMsg.Send( sdReliableMessageClientInfo( entityNumber ) );
		return;
	}

	bool mute = false;
	if ( other != NULL ) {
		mute = g_noQuickChats.GetBool() || gameLocal.IsClientQuickChatMuted( other );
	}

	if ( !mute || ( sendingClientNum == entityNumber ) ) {
		if ( quickChat->GetText() != NULL ) {
			sdGameRules::chatMode_t chatMode = sdGameRules::CHAT_MODE_QUICK;
			if ( quickChat->IsTeam() ) {
				chatMode = sdGameRules::CHAT_MODE_QUICK_TEAM;
			} else if ( quickChat->IsFireTeam() ) {
				chatMode = sdGameRules::CHAT_MODE_QUICK_FIRETEAM;
			}

			gameLocal.rules->AddChatLine( location, chatMode, sendingClientNum, quickChat->GetText()->GetText() );
		}

		if ( quickChat->GetAudio() != NULL ) {
			gameSoundWorld->PlayShaderDirectly( quickChat->GetAudio(), SND_QUICKCHAT + sendingClientNum );
		}
	}
}

template< class T >
class pointerSortCompare_t {
public:
	int operator()( const T a, const T b ) {
		return a - b;
	}
};

/*
==============
idPlayer::CancelToolTips
==============
*/
void idPlayer::CancelToolTips( void ) {
	// make sure the end of tooltip events are run
	if ( currentToolTip != NULL ) {
		CancelToolTipTimeline();
	}

	currentToolTip = NULL;
	for ( int i = 0; i < toolTips.Num(); i++ ) {
		delete toolTips[ i ].second;
	}
	toolTips.Clear();

	nextTooltipTime = 0;
	gameLocal.localPlayerProperties.SetToolTipInfo( L"", -1, NULL );

	gameSoundWorld->PlayShaderDirectly( NULL, SND_PLAYER_TOOLTIP );
}

/*
==============
idPlayer::PlayProneFailedToolTip
==============
*/
void idPlayer::PlayProneFailedToolTip( void ) {
	const sdDeclToolTip* toolTip;
	const sdInventory& inv = GetInventory();
	const sdDeclInvItem* item = inv.GetItem( inv.GetIdealWeapon() );
	if ( item != NULL && !item->GetAllowProne() ) {
		toolTip = gameLocal.declToolTipType[ item->GetData().GetString( "tt_prone_failed" ) ];
	} else {
		toolTip = gameLocal.declToolTipType[ spawnArgs.GetString( "tt_prone_failed" ) ];
	}

	if ( toolTip != NULL ) {
		SendToolTip( toolTip );
	}
}

/*
==============
idPlayer::SendToolTip
==============
*/
void idPlayer::SendToolTip( const sdDeclToolTip* toolTip, sdToolTipParms* toolTipParms ) {
	sdAutoPtr< sdToolTipParms > cleanup( toolTipParms );

	if ( toolTip == NULL ) {
		return;
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		SpawnToolTip( toolTip, cleanup.Release() );
		return;
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_TOOLTIP );
		msg.WriteLong( toolTip->Index() );
		if ( toolTipParms == NULL ) {
			msg.WriteLong( 0 );
		} else {
			msg.WriteLong( toolTipParms->Num() );
			for ( int i = 0; i < toolTipParms->Num(); i++ ) {
				msg.WriteString( toolTipParms->Get( i ) );
			}
		}
		msg.Send( sdReliableMessageClientInfo( entityNumber ) );
	} else {
		assert( false );
	}
}

/*
==============
idPlayer::SpawnToolTip
==============
*/
void idPlayer::SpawnToolTip( const sdDeclToolTip* toolTip, sdToolTipParms* toolTipParms ) {
	sdAutoPtr< sdToolTipParms > cleanup( toolTipParms );

	if( g_tooltipTimeScale.GetFloat() <= 0.0f ) {
		return;
	}

	sdDemoManagerLocal::demoState_t demoState = sdDemoManager::GetInstance().GetState();
	if ( demoState == sdDemoManagerLocal::DS_PAUSED || demoState == sdDemoManagerLocal::DS_PLAYING ) {
		return;
	}
	
	if ( gameLocal.localClientNum != entityNumber || !toolTip || toolTip == currentToolTip ) {
		return;
	}

	if ( gameLocal.rules->IsEndGame() ) {
		return;
	}

	if ( ( toolTip->GetLastTimeUsed() <= sys->Milliseconds() ) && ( ( sys->Milliseconds() - toolTip->GetLastTimeUsed() ) < toolTip->GetNextShowDelay() ) ) {
		return;
	}

	for ( int i = 0; i < toolTips.Num(); i++ ) {
		if ( toolTips[ i ].first == toolTip ) {
			return;
		}
	}

	int maxCount = toolTip->GetMaxPlayCount();
	if ( maxCount > 0 ) {
		if ( toolTip->GetCurrentPlayCount() >= maxCount ) {
			return;
		}
	}

	if ( gameLocal.isClient && toolTip->GetCurrentSinglePlayerPlayCount() > 0 ) {
		return;
	}

	if ( !IsToolTipPlaying() ) {
		nextTooltipTime = gameLocal.ToGuiTime( gameLocal.time );
	}

	toolTipParms_t& parms = toolTips.Alloc();
	parms.first = toolTip;
	parms.second = cleanup.Release();
}

/*
==============
idPlayer::SetActionMessage
==============
*/
void idPlayer::SetActionMessage( const char* message ) {
	actionMessage = message;
	actionMessageTime = gameLocal.time;
}

/*
==============
idPlayer::UpdateToolTips
==============
*/
void idPlayer::UpdateToolTips( void ) {
	if( g_tooltipTimeScale.GetFloat() <= 0.0f || gameLocal.rules->IsEndGame() ) {
		CancelToolTips();
		return;
	}

	UpdateToolTipTimeline();

	int now = gameLocal.ToGuiTime( gameLocal.time );

	if ( nextTooltipTime != 0 && now >= nextTooltipTime ) {
		if ( currentToolTip != NULL ) {
			CancelToolTipTimeline();
			currentToolTip->SetLastTimeUsed();
			currentToolTip = NULL;
		}

		if ( toolTips.Num() ) {
			toolTipParms_t& parms = toolTips[ 0 ];

			// cancel the queued tooltip if a single player tooltip while in a vehicle.
			if ( parms.first->GetSinglePlayerToolTip() && GetProxyEntity() != NULL ) {
				delete parms.second;
				toolTips.RemoveIndex( 0 );
				return;
			}

			idWStr text;
			parms.first->GetMessage( parms.second, text );
			gameLocal.localPlayerProperties.SetToolTipInfo( text.c_str(), parms.first->GetLocationIndex(), parms.first->GetIcon() );

			currentToolTip = parms.first;

			if ( currentToolTip->UsingSoundLength() ) {
				nextTooltipTime = now + currentToolTip->GetLength();
			} else {
				nextTooltipTime = now + ( currentToolTip->GetLength() * g_tooltipTimeScale.GetFloat() );
			}
			lastTimelineTime = now;

			if ( ( g_playTooltipSound.GetInteger() == 1 && gameLocal.isServer ) || g_playTooltipSound.GetInteger() == 2 ) {
				if ( const idSoundShader* sound = currentToolTip->GetSoundShader() ) {
					gameSoundWorld->PlayShaderDirectly( sound, SND_PLAYER_TOOLTIP, NULL );

					if ( sound != NULL ) {
						soundShaderParms_t parms;
						sound->GetParms( &parms );
						if ( parms.soundClass == 2 ) {
							idSoundWorld* gameSoundWorld = common->GetGameSoundWorld();
							if ( gameSoundWorld != NULL ) {
								gameSoundWorld->FadeSoundClasses( 0, g_tooltipVolumeScale.GetFloat(), 0.5f );
							}
						}
					}
				}
			}

			gameLocal.localPlayerProperties.SetUnpauseKeyString( currentToolTip->GetUnpauseKeyString() );

			delete parms.second;
			toolTips.RemoveIndex( 0 );
			return;
		}

		nextTooltipTime = 0;
		gameLocal.localPlayerProperties.SetToolTipInfo( L"", -1, NULL );
	}
}

/*
==============
idPlayer::UpdateToolTipTimeline
==============
*/
void idPlayer::UpdateToolTipTimeline( void ) {

	int now = gameLocal.ToGuiTime( gameLocal.time );

	// run tooltip timeline events
	if ( !g_trainingMode.GetBool() || currentToolTip == NULL || lastTimelineTime == -1 ) {
		return;
	}

	const idList< sdDeclToolTip::timelinePair_t >& timeline = currentToolTip->GetTimeline();
	idList< sdDeclToolTip::timelinePair_t >::ConstIterator it = timeline.Begin();

	int start = nextTooltipTime - currentToolTip->GetLength();

	int eventTime;
	while ( it != timeline.End() ) {
		if ( currentToolTip->UsingSoundLength() ) {
			eventTime = it->first + start;
		} else {
			eventTime = start + ( it->first * g_tooltipTimeScale.GetFloat() );
		}

		if ( eventTime >= lastTimelineTime && eventTime < now ) {
			// end of timeline tooltips will always be called when tooltip is done, not from here
			if ( it->first != sdDeclToolTip::TLTIME_END ) {
				RunToolTipTimelineEvent( it->second );
			}
		}
		it++;
	}

	lastTimelineTime = now;
}

/*
==============
idPlayer::CancelToolTipTimeline
==============
*/
void idPlayer::CancelToolTipTimeline( void ) {
	// always fade game sounds back in
	idSoundWorld* gameSoundWorld = common->GetGameSoundWorld();
	if ( gameSoundWorld != NULL ) {
		gameSoundWorld->FadeSoundClasses( 0, 0, 0.5f );
	}

	if ( currentToolTip == NULL || lastTimelineTime == -1 ) {
		return;
	}

	const idList< sdDeclToolTip::timelinePair_t >& timeline = currentToolTip->GetTimeline();
	idList< sdDeclToolTip::timelinePair_t >::ConstIterator it = timeline.Begin();

	while ( it != timeline.End() ) {
		if ( it->first == sdDeclToolTip::TLTIME_END ) {
			RunToolTipTimelineEvent( it->second );
		}
		it++;
	}

	lastTimelineTime = -1;
	lookAtTask = -1;
}

/*
==============
idPlayer::RunToolTipTimelineEvent
==============
*/
void idPlayer::RunToolTipTimelineEvent( const sdDeclToolTip::timelineEvent_t& event  ) {
	int weapon = -1;
	playerTaskList_t tasks;
	int i;

	switch ( event.eventType ) {
		case sdDeclToolTip::TL_GUIEVENT:
			uiManager->OnToolTipEvent( event.arg1.c_str() );
			break;
		case sdDeclToolTip::TL_PAUSE:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "pauseGame\n" );
			}
			break;
		case sdDeclToolTip::TL_UNPAUSE:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "unPauseGame\n" );
			}
			break;
		case sdDeclToolTip::TL_SHOWINVENTORY:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				weapon = GetInventory().FindWeapon( event.arg1.c_str() );
				if ( weapon != -1 ) {
					sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
					lastWeaponSwitchPos = GetInventory().GetSwitchingWeapon();
					if ( lastWeaponSwitchPos == -1 ) {
						lastWeaponSwitchPos = GetInventory().GetIdealWeapon();
					}
					if ( weaponMenu->IsMenuVisible() ) {
						lastWeaponMenuState = true;
					}

					GetInventory().SetSwitchingWeapon( weapon );
					ShowWeaponMenu( sdWeaponSelectionMenu::AT_ENABLED, true );
				} else {
					gameLocal.Warning( "Unknown weapon index: for weapon '%s': %i", event.arg1.c_str(), weapon );
				}
			}
			break;
		case sdDeclToolTip::TL_HIDEINVENTORY:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				if ( lastWeaponSwitchPos != -1 ) {
					GetInventory().SetSwitchingWeapon( lastWeaponSwitchPos );
				}
				if ( lastWeaponMenuState  ) {
					ShowWeaponMenu( sdWeaponSelectionMenu::AT_ENABLED );
				} else {
					ShowWeaponMenu( sdWeaponSelectionMenu::AT_DISABLED );
				}
				lastWeaponSwitchPos = -1;
				lastWeaponMenuState = false;
			}
			break;
		case sdDeclToolTip::TL_WAYPOINTHIGHLIGHT:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				sdTaskManager::GetInstance().BuildTaskList( this, tasks );

				for ( i = 0; i < tasks.Num(); i++ ) {
					if ( !event.arg1.Icmp( tasks[ i ]->GetInfo()->GetData().GetString( "tt_task" ) ) ) {
						tasks[ i ]->FlashIcon( tasks[ i ]->GetInfo()->GetData().GetInt( "tt_wp_flash_time", "4000" ) );
					}
				}
			}
			break;
		case sdDeclToolTip::TL_LOOKATTASK:
			if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isServer ) {
				if ( GetActiveTask() != NULL && !idStr::Icmp( GetActiveTask()->GetInfo()->GetData().GetString( "tt_task" ), currentToolTip->GetName() ) ) {
					sdPlayerTask* task = GetActiveTask();
					lookAtTask = task->GetHandle();
					task->FlashIcon( task->GetInfo()->GetData().GetInt( "tt_wp_flash_time", "4000" ) );
				} else {
					sdTaskManager::GetInstance().BuildTaskList( this, tasks );

					for ( i = 0; i < tasks.Num(); i++ ) {
						if ( !idStr::Icmp( tasks[ i ]->GetInfo()->GetData().GetString( "tt_task" ), currentToolTip->GetName() ) ) {
							lookAtTask = tasks[ i ]->GetHandle();
							tasks[ i ]->FlashIcon( tasks[ i ]->GetInfo()->GetData().GetInt( "tt_wp_flash_time", "4000" ) );
						}
					}
				}
			}
			break;
		default:
			gameLocal.Warning( "Unknown timeline event %i", event.eventType );
			break;
	}
}

/*
==============
idPlayer::UsercommandCallback
==============
*/
void idPlayer::UsercommandCallback( usercmd_t& cmd ) {
	if ( gameLocal.IsPaused() && g_pauseNoClip.GetBool() ) {
		gameLocal.UpdatePauseNoClip( cmd );
	}

	if ( clientButtonsUsed ) {
		clientOldButtons	= clientButtons;
		clientButtonsUsed	= false;
	}
	clientButtons		= cmd.buttons;

	if ( !gameLocal.localPlayerProperties.ManualVoiceChat() ) {
		if ( cmd.clientButtons.fireteamVoice ) {
			networkSystem->EnableVoip( VO_FIRETEAM );
		} else if ( cmd.clientButtons.teamVoice ) {
			networkSystem->EnableVoip( VO_TEAM );
		} else if ( cmd.clientButtons.voice ) {
			networkSystem->EnableVoip( VO_GLOBAL );
		} else {
			networkSystem->DisableVoip();
		}
	}

	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();

	if ( module ) {
		module->UsercommandCallback( cmd );
	}

	for ( module = gameLocal.localPlayerProperties.GetPassiveHudModule(); module; module = module->GetNode().Next() ) {
		module->UsercommandCallback( cmd );
	}

	if ( playerFlags.clientWeaponLock ) {
		if ( cmd.buttons.btn.attack ) {
			cmd.buttons.btn.attack = false;
		} else if( cmd.buttons.btn.altAttack ) {
			cmd.buttons.btn.altAttack = false;
		} else {
			playerFlags.clientWeaponLock = false;
		}
	}

	if ( InhibitUserCommands() ) {
		cmd.forwardmove	= 0;
		cmd.rightmove	= 0;
		cmd.upmove		= 0;

		cmd.buttons.btn.attack		= false;
		cmd.buttons.btn.altAttack	= false;
		cmd.buttons.btn.activate	= false;
		cmd.buttons.btn.modeSwitch	= false;
		cmd.buttons.btn.leanLeft	= false;
		cmd.buttons.btn.leanRight	= false;
		cmd.buttons.btn.tophat		= false;
	}
}

/*
==============
idPlayer::GetSensitivity
==============
*/
extern idCVar m_playerPitchScale;
extern idCVar m_playerYawScale;

bool idPlayer::GetSensitivity( float& scaleX, float& scaleY ) {
	if ( InhibitUserCommands() ) {
		scaleX = 0.f;
		scaleY = 0.f;
		return true;
	}

	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
	if ( module && module->GetSensitivity( scaleX, scaleY ) ) {
		return true;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy != NULL ) {
		if ( !proxy->GetUsableInterface()->GetAllowPlayerWeapon( this ) ) {
			if ( proxy->GetUsableInterface()->GetSensitivity( this, scaleX, scaleY ) ) {
				return true;
			}
		}
	}

	bool changed = false;

	// scale the default with the player settings
	if ( m_playerPitchScale.GetFloat() != 1.0f ) {
		scaleY *= m_playerPitchScale.GetFloat();
		changed = true;
	}
	if ( m_playerYawScale.GetFloat() != 1.0f ) {
		scaleX *= m_playerYawScale.GetFloat();
		changed = true;
	}

	idWeapon* weapon = GetWeapon();
	if ( weapon != NULL ) {
		float fov;
		if ( weapon->GetFov( fov ) ) {
			float viewScale = fov / 90.f;
			scaleX *= viewScale;
			scaleY *= viewScale;
			return true;
		}
	}

	return changed;
}

/*
==============
idPlayer::KeyMove
==============
*/

//idCVar g_hackPlayerMove( "g_hackPlayerMove", "0", CVAR_BOOL | CVAR_GAME, "Hacks player movement for the local player so that they run back & forward constantly (worst case for network prediction)" );
//idCVar g_hackPlayerMoveTime( "g_hackPlayerMoveTime", "5", CVAR_INTEGER | CVAR_GAME, "How many frames between forward/backward switch" );

bool idPlayer::KeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	// pass move to any proxy
	idEntity* proxyEntity = GetProxyEntity();
	if ( proxyEntity ) {
		return proxyEntity->OnKeyMove( forward, right, up, cmd );
	}


/*
	if ( g_hackPlayerMove.GetBool() ) {
		if ( playerFlags.noclip ) {
			return false;
		}

		int duration = Max( 1, g_hackPlayerMoveTime.GetInteger() );
		cmd.forwardmove = ( ( ( gameLocal.time / gameLocal.msec ) / duration ) % 2 );
		if ( cmd.forwardmove ) {
			cmd.forwardmove = 127;
		} else {
			cmd.forwardmove = -127;
		}

		cmd.rightmove = right;
		cmd.upmove = up;

		return true;
	}
*/
	return false;
}

/*
==============
idPlayer::ControllerMove
==============
*/
void idPlayer::ControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers, const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	idEntity* proxyEntity = GetProxyEntity();
	if ( proxyEntity ) {
		proxyEntity->OnControllerMove( doGameCallback, numControllers, controllerNumbers, controllerAxis, viewAngles, cmd );
	} else {
		for ( int i = 0; i < numControllers; i++ ) {
			int num = controllerNumbers[ i ];
			sdInputModePlayer::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
		}
	}
}

/*
==============
idPlayer::MouseMove
==============
*/
void idPlayer::MouseMove( const idAngles& baseAngles, idAngles& angleDelta ) {
	idEntity* proxyEntity = GetProxyEntity();
	if ( proxyEntity ) {
		proxyEntity->OnMouseMove( this, viewAngles, angleDelta );
	} else if ( remoteCamera.IsValid() ) {
		remoteCamera->OnMouseMove( this, viewAngles, angleDelta );
	} else {
		if ( GetWeapon() != NULL ) {
			idAngles result = viewAngles + angleDelta;
			GetWeapon()->ClampAngles( result, viewAngles );
			angleDelta = result - viewAngles;
			angleDelta.Normalize180();
		}
	}
}

/*
==============
idPlayer::WriteStats
==============
*/
void idPlayer::WriteStats( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( !gameLocal.rules->IsGameOn() ) {
		return;
	}

	sdGlobalStatsTracker::GetInstance().Write( entityNumber, userInfo.cleanName );
}

/*
==============
idPlayer::~idPlayer()

Release any resources used by the player.
==============
*/
idPlayer::~idPlayer() {
	LogTimePlayed();
	GetInventory().LogClassTime();

	WriteStats();
	sdGlobalStatsTracker::GetInstance().CancelStatsRequest( entityNumber );
	sdGlobalStatsTracker::GetInstance().Clear( entityNumber );

	SetActiveTask( taskHandle_t() );

	DeconstructScriptObject();

	idWeapon* w = weapon.GetEntity();
	if ( w ) {
		w->SetOwner( NULL );

		if ( !gameLocal.isClient ) {
			w->PostEventMS( &EV_Remove, 0 );
		}
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.SetLocalPlayer( NULL );
	}

	if ( waterEffects != NULL ) {
		delete waterEffects;
		waterEffects = NULL;
	}

#ifdef PLAYER_DAMAGE_LOG
	ClearDamageLog();
#endif // PLAYER_DAMAGE_LOG
}

/*
===============
idPlayer::ServerSpectate
================
*/
void idPlayer::ServerSpectate( void ) {
	assert( !gameLocal.isClient );

	if ( IsSpectator() ) {
		return;
	}

	SetGameTeam( NULL );
}

/*
===========
idPlayer::SpawnFromSpawnSpot

Chooses a spawn location and spawns the player
============
*/
void idPlayer::SpawnFromSpawnSpot( void ) {
	ForceNetworkUpdate();

	if ( !IsSpectator() ) {
		if ( !IsInLimbo() ) {
			OnFullyKilled( false );
		}
	}

	idVec3		spawnOrigin;
	idAngles	spawnAngles;
	const sdSpawnPoint* point = gameLocal.SelectInitialSpawnPoint( this, spawnOrigin, spawnAngles );
	bool parachute = false;
	if ( point ) {
		point->SetLastUsedTime( gameLocal.time );

		idEntity* owner = point->GetOwner();
		if ( owner ) {
			if ( owner ) {
				sdScriptHelper helper;
				helper.Push( GetScriptObject() );
				owner->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnPlayerSpawned" ), helper );
			}
		}

		parachute = point->GetParachute();
	}
	SpawnToPoint( spawnOrigin, spawnAngles, false, parachute );
}

/*
===========
idPlayer::UpdateVisibility
===========
*/
void idPlayer::UpdateVisibility( void ) {
	bool hide = playerFlags.scriptHide;

	if ( IsSpectating() ) {
		hide = true;
	} else {
		idEntity* proxy = GetProxyEntity();
		if ( proxy ) {
			sdUsableInterface* iface = proxy->GetUsableInterface();
			if ( !iface ) {
				gameLocal.Warning( "idPlayer::UpdateVisibility Proxy with Missing Interface '%s'", proxy->GetName() );
			} else {
				if ( !iface->GetShowPlayer( this ) ) {
					hide = true;
				}
			}
		}
	}

	if ( hide ) {
		Hide();
	} else {
		Show();
	}
}

/*
===========
idPlayer::UpdateCombatModel
============
*/
void idPlayer::UpdateCombatModel( void ) {
	bool wantCombatModel = false;
	if ( IsDead() || physicsObj.IsProne() || physicsObj.GetLeanOffset() != 0.f || physicsObj.GetProneChangeEndTime() != 0 ) {
		wantCombatModel = true;
	}

	if ( wantCombatModel ) {
		if ( combatModel == NULL ) {
			SetCombatModel();
			LinkCombat();
		}
	} else {
		if ( combatModel != NULL ) {
			RemoveCombatModel();
		}
	}
}

/*
===========
idPlayer::OnRespawn
============
*/
void idPlayer::OnRespawn( bool revive, bool parachute, int respawnTime, bool forced ) {
	if ( gameLocal.isServer ) {
		sdGlobalStatsTracker::GetInstance().SetStatBaseLine( entityNumber );

		sdEntityBroadcastEvent evtMsg( this, EVENT_RESPAWN );

		const idVec3& org = GetPhysics()->GetOrigin();
		evtMsg.WriteFloat( org[ 0 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		evtMsg.WriteFloat( org[ 1 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		evtMsg.WriteFloat( org[ 2 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		evtMsg.WriteUShort( ANGLE2SHORT( viewAngles[ 0 ] ) );
		evtMsg.WriteUShort( ANGLE2SHORT( viewAngles[ 1 ] ) );
		evtMsg.WriteBits( revive, 1 );
		evtMsg.WriteBits( parachute, 1 );
		evtMsg.Send( false, sdReliableMessageClientInfoAll() );
	}

	if ( !revive && !IsSpectator() ) {
		if ( !forced ) {
			GetInventory().LogTapOut();
		}
		GetInventory().LogRespawn();
	}

	crosshairInfo.Invalidate();
	crosshairInfo.SetEntity( NULL );

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.SetContextEntity( NULL );
	}

	physicsObj.SetupPlayerClipModels();

	UpdateCombatModel();

	lastAliveTimeRegistered = gameLocal.time;

	if ( !gameLocal.isClient ) {
		SetSpectateClient( this );
	} else {
		if ( !revive ) {
			Init( false );
		}
		SetPhysics( &physicsObj );
		physicsObj.SetClipModelAxis();
		physicsObj.LinkClip();
		physicsObj.SetLinearVelocity( vec3_origin );
	}
	physicsObj.SetFrozen( false );

	sdScriptHelper h;
	h.Push( revive );
	h.Push( parachute );
	CallNonBlockingScriptEvent( onSpawnFunction, h );

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.ForceCommandMap( false );
	}

	killer = NULL;

	if ( revive ) {
		lastReviveTime = respawnTime;
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "EnterAnimState_Revive" ), h1 );
	}

	UpdateShadows();
	UpdateRating();
}

/*
===========
idPlayer::SpawnToPoint

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState

when called here with spectating set to true, just place yourself and init
============
*/
void idPlayer::SpawnToPoint( const idVec3& spawnOrigin, const idAngles& spawnAngles, bool revive, bool parachute ) {
	assert( !gameLocal.isClient );

	if ( !revive ) {
		Init( !parachute );
	}

	suppressPredictionReset = true;

	fl.noknockback = false;

	// set back the player physics
	SetPhysics( &physicsObj );

	physicsObj.SetClipModelAxis();
	physicsObj.LinkClip();
	physicsObj.SetLinearVelocity( vec3_origin );

	// setup our initial view
	if ( !IsSpectator() ) {
		SetOrigin( spawnOrigin );
	} else {
		idVec3 specOrigin;
		specOrigin = spawnOrigin;
		specOrigin[ 2 ] += pm_normalheight.GetFloat() + SPECTATE_RAISE;
		SetOrigin( specOrigin );
	}

	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	clientViewAngles = ang_zero;
	SetDeltaViewAngles( ang_zero );
	SetViewAngles( spawnAngles );
	this->spawnAngles = spawnAngles;
	playerFlags.spawnAnglesSet = false;

	playerFlags.legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	SetRemoteCamera( NULL, false );
	remoteCameraViewOffset.Zero();

	AI_TELEPORT = true;

	// don't allow full run speed for a bit
	physicsObj.SetKnockBack( 100 );

	bool forced = IsInLimbo();

	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	playerFlags.forceRespawn = false;
	playerFlags.wantSpawn = false;

	OnRespawn( revive, parachute, gameLocal.time, forced );

	damageAreasScale.Clear();
	damageAreasScale.SetNum( LDA_COUNT );
	damageAreasScale[ LDA_LEGS ] = spawnArgs.GetFloat( "loc_damage_area_damage_legs", "0.5" );
	damageAreasScale[ LDA_TORSO ] = spawnArgs.GetFloat( "loc_damage_area_damage_torso", "1" );
	damageAreasScale[ LDA_HEAD ] = spawnArgs.GetFloat( "loc_damage_area_damage_head", "2" );
	damageAreasScale[ LDA_NECK ] = spawnArgs.GetFloat( "loc_damage_area_damage_neck", "1.5" );
	damageAreasScale[ LDA_HEADBOX ] = spawnArgs.GetFloat( "loc_damage_area_damage_headbox", "2" );

	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].justSpawned = true; //mal: lets track when this client spawned/respawned. Useful to the bots...
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.primaryWeaponNeedsUpdate = true;
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].spawnTime = gameLocal.time; //mal: track when this player entered the map - also useful to bots.
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].revived = revive; //mal: was this client spawned fresh, or just revived, into the world?

	if ( revive ) { //mal: thank the medic for the revive - ONLY humans!
        if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].isBot ) {
			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].mySavior != -1 ) {
				idPlayer* mySavior = gameLocal.GetClient( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].mySavior );

				if ( mySavior != NULL && !mySavior->userInfo.isBot ) {
					botThreadData.VOChat( THANKS, entityNumber, true );
				}
			}
 		}
	} else {
		if ( gameLocal.random.RandomInt( 100 ) > 75 ) {
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].backPedalTime = gameLocal.time + SPAWN_CONSIDER_TIME;
		}
	}
	
	ResetAntiLag();

	suppressPredictionReset = false;
}

/*
==============
idPlayer::UserInfoChanged
==============
*/
void idPlayer::UserInfoChanged( void ) {
	const idDict& dict = networkSystem->GetUserInfo( entityNumber );

	idStr oldName					= userInfo.rawName;
	idStr oldFullName				= userInfo.name;

	userInfo.FromDict( dict );

	if ( oldName.Length() > 0 && userInfo.rawName.Length() > 0 ) {
		if ( oldName.Icmp( userInfo.rawName ) != 0 ) {
			gameLocal.OnUserNameChanged( this, oldName, userInfo.rawName );
			
			idWStrList args( 2 );
			args.Append( va( L"%hs", oldFullName.c_str() ) );
			args.Append( va( L"%hs", userInfo.rawName.c_str() ) );
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_MESSAGE, -1, common->LocalizeText( "game/player_rename", args ).c_str() );
		}
	}
}

/*
===============
idPlayer::GetGameTeam
===============
*/
sdTeamInfo* idPlayer::GetGameTeam( void ) const {
	return team;
}

/*
===============
idPlayer::UpdateCrosshairInfo
===============
*/
bool idPlayer::UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info ) {
	idEntity::crosshairInfo = &info;

	sdScriptHelper helper;
	helper.Push( player == NULL ? NULL : player->GetScriptObject() );
	bool ret = CallFloatNonBlockingScriptEvent( scriptObject->GetFunction( "OnUpdateCrosshairInfo" ), helper ) != 0.f;

	idEntity::crosshairInfo = NULL;

	return ret;
}

/*
===============
idPlayer::ClearTargetLock
===============
*/
void idPlayer::ClearTargetLock( void ) {
	targetLockLastTime = gameLocal.time;
	targetLockEndTime = 0;
	targetLockSafeTime = 0;
	playerFlags.targetLocked = false;

	SetTargetEntity( NULL );
	if ( gameLocal.IsLocalViewPlayer( this ) ) {
		gameSoundWorld->PlayShaderDirectly( NULL, SND_PLAYER_TARGETLOCK );
	}
}

/*
==============
idPlayer::GetEnemy
==============
*/
idEntity* idPlayer::GetEnemy( void ) {
	return GetTargetLocked() ? targetEntity.GetEntity() : NULL;
}

/*
===============
idPlayer::CheckTargetLockValid
===============
*/
bool idPlayer::CheckTargetLockValid( idEntity* entity, const sdWeaponLockInfo* lockInfo ) {
	assert( lockInfo );

	if ( entity->GetHealth() <= 0 || !entity->fl.takedamage ) {
		return false;
	}

	if ( !lockInfo->LockFriendly() ) {
		if ( GetEntityAllegiance( entity ) == TA_FRIEND ) {
			return false;
		}
	} else {
		if ( GetEntityAllegiance( entity ) == TA_ENEMY ) {
			return false;
		}
	}

	const sdDeclTargetInfo*	lockFilter = lockInfo->GetLockFilter();
	if ( lockFilter && !lockFilter->FilterEntity( entity ) ) {
		return false;
	}

	if ( proxyEntity != NULL ) {
		sdTransport* proxyTransport = proxyEntity->Cast< sdTransport >();
		if ( proxyTransport != NULL ) {
			sdVehicleWeapon* weapon = proxyTransport->GetActiveWeapon( this );
			if ( weapon != NULL && weapon->HasLockClamp() ) {
				if ( !weapon->IsValidLockDirection( renderView.viewaxis[ 0 ] ) ) {
					return false;
				}
			}
		}
	}

	return true;
}

/*
===============
idPlayer::CheckTargetCanLock
===============
*/
bool idPlayer::CheckTargetCanLock( idEntity* target, float boundsScale, float& distance, const idFrustum& frustum ) {
	// already have a target, make it harder to lose
	const idBounds*	bounds = target->GetSelectionBounds();
	const idVec3*	targetOrg;
	const idMat3*	targetAxis;
	if ( bounds ) {
		targetOrg	= &target->GetRenderEntity()->origin;
		targetAxis	= &target->GetRenderEntity()->axis;
	} else {
		bounds		= &target->GetPhysics()->GetBounds();
		targetOrg	= &target->GetPhysics()->GetOrigin();
		targetAxis	= &target->GetPhysics()->GetAxis();
	}

	idVec3	size		= bounds->Size();
	float	bestValue	= size[ 0 ];
	for ( int i = 1; i < 3; i++ ) {
		if ( size[ i ] >= bestValue ) {
			continue;
		}
		bestValue = size[ i ];
	}

	if ( frustum.CullSphere( idSphere( *targetOrg, bestValue ) ) ) {
		return false;
	}

	sdBounds2D bounds2d;
	sdWorldToScreenConverter::TransformClipped( *bounds, *targetAxis, *targetOrg, bounds2d, frustum, idVec2( SCREEN_WIDTH, SCREEN_HEIGHT ) );

	idVec2 center	= bounds2d.GetCenter();
	float radiusSqr = ( bounds2d.GetMins() - center ).LengthSqr() * Square( boundsScale );

	idVec2 crosshair( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f );
	float offset	= ( crosshair - center ).LengthSqr();

	if ( offset > radiusSqr ) {
		return false;
	}

	idVec3 diff = ( ( bounds->GetCenter() + *targetOrg ) - renderView.vieworg );
	float viewDist = ( renderView.viewaxis[ 0 ] * diff );
	if ( viewDist < 0.f ) {
		return false;
	}

	distance = offset;

	return true;
}

/*
===============
idPlayer::UpdateTargetLock
===============
*/
void idPlayer::UpdateTargetLock( void ) {
	const sdWeaponLockInfo* lockInfo = GetCurrentLockInfo();
	if ( lockInfo == NULL ) {
		ClearTargetLock();
		return;
	}

	idFrustum frustum;
	BuildViewFrustum( frustum, lockInfo->GetLockDistance() );

	float bestDist			= idMath::INFINITY;
	idEntity* bestTarget	= NULL;

	float scale = 0.f;
	if ( playerFlags.targetLocked ) {
		bool allowIt = true;

		idEntity* target = targetEntity;
		if ( !target || !CheckTargetLockValid( target, lockInfo ) ) {
			allowIt = false;
		} else {
			float dist;
			if ( CheckTargetCanLock( target, 1.25f, dist, frustum ) ) {
				bestDist	= dist;
				bestTarget	= target;
			} else {
				allowIt = false;
			}
		}

		if ( !allowIt ) {
			ClearTargetLock();
		}
	}

	idEntity* other;
	if ( !playerFlags.targetLocked ) {
		for ( idLinkList< idEntity >* node = gameLocal.GetTargetEntities(); node; node = node->NextNode() ) {
			idEntity* ent = node->Owner();

			if ( ent == this ) {
				continue;
			}

			if ( !CheckTargetLockValid( ent, lockInfo ) ) {
				continue;
			}

			float dist;
			float boundsScale = 1.f;

			if ( IsType( idBot::Type ) ) {
				botThreadData.CheckTargetLockInfo( entityNumber, boundsScale );
			}

			if ( CheckTargetCanLock( ent, boundsScale, dist, frustum ) ) {
				if ( dist < bestDist ) {
					bestDist	= dist;
					bestTarget	= ent;
				}
			}
		}

		other = bestTarget;
	} else {
		other = targetEntity;
	}

	if ( other != NULL ) {
		idVec3 temp;
		if ( !other->CanDamage( renderView.vieworg, temp, MASK_SHOT_RENDERMODEL, this ) ) {
			other = NULL;
		}
	}

	if ( other != targetEntity ) {
		if ( gameLocal.time > targetLockSafeTime ) {
			if ( !other ) {
				if ( !lockInfo->IsSticky() ) {
					ClearTargetLock();
				}
			} else {
				// new lock
				SetTargetEntity( other );
				playerFlags.targetLocked = false;
				targetEntityPrevious = other;
			}
		}
	} else if ( targetEntity.IsValid() ) {
		if ( gameLocal.time > targetLockEndTime && !playerFlags.targetLocked ) {
			// continued lock
			playerFlags.targetLocked = true;
			if ( gameLocal.IsLocalViewPlayer( this ) ) {
				gameSoundWorld->PlayShaderDirectly( lockInfo->GetLockedSound(), SND_PLAYER_TARGETLOCK );
			}
		}
		if ( playerFlags.targetLocked ) {
			targetLockSafeTime = gameLocal.time + SEC2MS( 1 );
		}
	}
}

/*
================
idPlayer::GetCurrentLockInfo
================
*/
const sdWeaponLockInfo* idPlayer::GetCurrentLockInfo( void ) {
	const sdWeaponLockInfo* lockInfo = NULL;
	if ( health > 0 ) {
		idEntity* proxy = GetProxyEntity();
		if ( proxy != NULL && !proxy->GetUsableInterface()->GetAllowPlayerWeapon( this ) ) {
			lockInfo = proxy->GetUsableInterface()->GetWeaponLockInfo( this );
		} else {
			idWeapon* realWeapon = GetWeapon();
			if ( realWeapon ) {
				lockInfo = realWeapon->GetLockInfo();
			}
		}
	}

	if ( lockInfo != NULL && lockInfo->IsSupported() ) {
		return lockInfo;
	}

	return NULL;
}

/*
============
idPlayer::SetTargetEntity
============
*/
void idPlayer::SetTargetEntity( idEntity* entity ) {

	if ( targetEntity == entity ) {
		return;
	}

	const sdProgram::sdFunction* func = NULL;

	if ( targetEntity && targetEntity->GetScriptObject() ) {
		func = targetEntity->GetScriptObject()->GetFunction( "OnEndPlayerTargetLock" );
		if ( func ) {
			sdScriptHelper helper;
			targetEntity->CallNonBlockingScriptEvent( func, helper );
		}
	}

	if ( entity && entity->GetScriptObject() ) {
		func = entity->GetScriptObject()->GetFunction( "OnBeginPlayerTargetLock" );
		if ( func ) {
			sdScriptHelper helper;
			entity->CallNonBlockingScriptEvent( func, helper );
		}
	}

	targetEntity = entity;

	const sdWeaponLockInfo* lockInfo = GetCurrentLockInfo();
	if ( targetEntity == NULL || lockInfo == NULL ) {
		if ( gameLocal.IsLocalViewPlayer( this ) ) {
			gameSoundWorld->PlayShaderDirectly( NULL, SND_PLAYER_TARGETLOCK );
		}
		return;
	}

	idEntity* proxy = GetProxyEntity();

	// play the sound here as the client may only notified of the target entity changing when the network state is applied
	if ( targetEntity != NULL && lockInfo != NULL ) {
		if ( gameLocal.IsLocalViewPlayer( this ) ) {
			gameSoundWorld->PlayShaderDirectly( lockInfo->GetLockingSound(), SND_PLAYER_TARGETLOCK );
		}
		targetLockDuration = lockInfo->GetLockDuration();
		targetLockDuration /= targetLockTimeScale;
		targetLockEndTime = gameLocal.time + targetLockDuration;
		targetLockSafeTime = 0;
		targetLockLastTime = 0;
	}
}

/*
============
idPlayer::LocationalDamageAreaForString
============
*/
locationDamageArea_t idPlayer::LocationalDamageAreaForString( const char* const str ) const {
	// nick: todo: make into a table lookup if expanded

	if( !idStr::Icmp( "legs", str ) ) {
		return LDA_LEGS;
	} else if( !idStr::Icmp( "torso", str ) ) {
		return LDA_TORSO;
	} else if( !idStr::Icmp( "head", str ) ) {
		return LDA_HEAD;
	} else if( !idStr::Icmp( "neck", str ) ) {
		return LDA_NECK;
	} else if( !idStr::Icmp( "headbox", str ) ) {
		return LDA_HEADBOX;
	}

	return LDA_INVALID;
}

/*
===============
idPlayer::GetCrosshairInfo
===============
*/
const sdCrosshairInfo& idPlayer::GetCrosshairInfo( bool doTrace ) {
	idPlayer* crosshairPlayer = GetViewClient();

	float range = targetIdRangeNormal;
	float radius;

	if ( IsBeingBriefed() ) {
		return crosshairInfo;
	} else if ( g_showCrosshairInfo.GetInteger() == 0 ) {
		return crosshairInfo;
	} else if ( g_showCrosshairInfo.GetInteger() > 1 ) {
		doTrace = true;
	}

	if ( doTrace || crosshairInfo.IsValid() ) {

		idEntity* proxy = crosshairPlayer->GetProxyEntity();
		if ( proxy ) {
			proxy->DisableClip( false );
		}

		if ( crosshairPlayer->IsType( idBot::Type ) ) {//mal: give bots a break
			radius = 32.0f;
		} else {
			radius = 8.0f;
		}

		trace_t trace;
		memset( &trace, 0, sizeof( trace ) );
		idVec3 end = renderView.vieworg + ( renderView.viewaxis[ 0 ] * range );

		bool retVal = gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, renderView.vieworg, end, MASK_SHOT_RENDERMODEL | CONTENTS_SHADOWCOLLISION | CONTENTS_SLIDEMOVER | CONTENTS_BODY | CONTENTS_PROJECTILE | CONTENTS_CROSSHAIRSOLID, crosshairPlayer, TM_CROSSHAIR_INFO, radius );

		// set up approximate distance
		if ( trace.c.material != NULL && trace.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) {
			crosshairInfo.SetDistance( range );
		} else {
			crosshairInfo.SetDistance( range * trace.fraction );
		}

		if ( retVal ) {
			if ( trace.c.entityNum != ENTITYNUM_NONE ) {

				idEntity* traceEnt = gameLocal.entities[ trace.c.entityNum ];

				if ( crosshairInfo.GetEntity() != traceEnt || !crosshairInfo.IsValid() ) {
					crosshairInfo.SetEntity( traceEnt );
					crosshairInfo.SetStartTime( gameLocal.time );
				}

				if ( traceEnt != NULL && trace.c.entityNum != ENTITYNUM_WORLD ) {
					if ( g_showCrosshairInfo.GetInteger() == 2 && gameLocal.CheatsOk( true ) ) {
						if ( gameLocal.IsLocalPlayer( this ) ) {
							const char* entScript = "";
							idScriptObject* obj = traceEnt->GetScriptObject();
							if ( obj ) {
								entScript = obj->GetTypeName();
							}
							const char* teamName = "none";
							sdTeamInfo* t = traceEnt->GetGameTeam();
							if ( t ) {
								teamName = t->GetLookupName();
							}

							idVec3 vec = traceEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
							float dist = vec.LengthFast();

							const char* text = va( "name: %s\nscript: %s\nhealth: %i/%i\nteam: %s\ndist: %f\n", traceEnt->GetName(), entScript, traceEnt->GetHealth(), traceEnt->GetMaxHealth(), teamName, dist );
							gameRenderWorld->DrawText( text, renderView.vieworg + ( renderView.viewaxis[ 0 ] * 192.f ), 0.25f, colorRed, renderView.viewaxis );
						}
					}

					if ( traceEnt->UpdateCrosshairInfo( crosshairPlayer, crosshairInfo ) ) {
						crosshairInfo.Validate();
					}
				}
			}

			if ( g_showCrosshairInfo.GetInteger() == 3 && gameLocal.CheatsOk( true ) ) {
				if ( gameLocal.IsLocalPlayer( this ) ) {
					const idMaterial* mat = trace.c.material;

					const char* materialName = "";
					int surfaceFlags = 0;

					if ( mat != NULL ) {
						materialName = mat->GetName();
						surfaceFlags = mat->GetSurfaceFlags();
					}

					const char* text = va( "material: %s\nsurface flags: %d\n", materialName, surfaceFlags );
					gameRenderWorld->DrawText( text, renderView.vieworg + ( renderView.viewaxis[ 0 ] * 192.f ), 0.25f, colorRed, renderView.viewaxis );
				}
			}
		}


		if ( proxy ) {
			proxy->EnableClip();
		}

		crosshairInfo.SetTrace( trace );
	}

	if ( doTrace || crosshairInfo.IsValid() ) {
        if ( IsType( idBot::Type ) ) {
			botThreadData.CheckCrossHairInfo( crosshairPlayer, crosshairInfo );
		}
	}

	return crosshairInfo;
}

/*
===============
idPlayer::DrawLocalPlayerAttitude
===============
*/
void idPlayer::DrawLocalPlayerAttitude( sdUserInterfaceLocal* ui, float x, float y, float w, float h ) {
	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( !viewPlayer ) {
		return;
	}

	sdTransport* transport = viewPlayer->GetProxyEntity()->Cast< sdTransport >();
	if ( !transport ) {
		return;
	}


	idMat3 rotateAxes;
	idMat3 axes = transport->GetRenderEntity()->axis;

	idAngles::YawToMat3( -axes.ToAngles().yaw, rotateAxes );

	idVec3 up = transport->GetRenderEntity()->axis[ 2 ];
	up *= rotateAxes;

	Swap( up[ 0 ], up[ 2 ] );
	idAngles angles = up.ToAngles();

	float pitch = idMath::AngleNormalize180( angles.pitch );
	float bank = idMath::AngleNormalize180( angles.yaw );

	const idMaterial* pitchMaterial = transport->GetAttitudeMaterial();


	idVec2 center( x + w * 0.5f, y + h * 0.5f );
	idVec2 radius( w * 0.5f, h * 0.5f );

	const int NUM_CIRCLE_POINTS = 16;

	idWinding2D winding;
	int i;
	for ( i = 0; i < NUM_CIRCLE_POINTS; i++ ) {
		float angle = DEG2RAD( ( ( 360.f * i ) / static_cast< float >( NUM_CIRCLE_POINTS ) ) );

		float sa, ca;
		idMath::SinCos( angle, sa, ca );

		winding.AddPoint( center.x + ( ca * radius.x ), center.y + ( sa * radius.y ) );
		winding.SetPointST( i, ( ( ca + 1 ) * 0.5f ), ( ( sa + 1 ) * 0.5f ) - ( 1.0f * ( pitch / 180.f ) ) );
	}


	idVec2 coords, extents;
	coords.Set( 0.5f, 0.5f );
	winding.RotationST( coords, DEG2RAD( bank ) );

	deviceContext->DrawWindingMaterial( winding, pitchMaterial, colorWhite );
}

/*
============
idPlayer::GetActiveTask
============
*/
sdPlayerTask* idPlayer::GetActiveTask( void ) const {
	return sdTaskManager::GetInstance().TaskForHandle( activeTask );
}

/*
============
idPlayer::SelectNextTask
============
*/
void idPlayer::SelectNextTask( void ) {
	playerTaskList_t tasks;

	sdTaskManager::GetInstance().BuildTaskList( this, tasks );
	if ( tasks.Num() == 0 ) {
		const char* sound = spawnArgs.GetString( "snd_no_mission", "" );
		idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
		if ( sound[ 0 ] != '\0' && sw != NULL ) {
			sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( sound ) );
		}
		return;
	}

	// NULL active task means we have selected the current objective
	const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( team );
	sdPlayerTask* objectiveTask = objectiveTasks.Next();
	if ( objectiveTask != NULL ) {
		tasks.Insert( NULL, 0 );
	}

	sdPlayerTask* task = GetActiveTask();

	int index = tasks.FindIndex( task );
	if ( index == -1 ) {
		index = tasks.Num() - 1;
	}

	int num = Min( sdPlayerTask::MAX_CHOOSABLE_TASKS + 1, tasks.Num() );
	index = ( index + 1 ) % num;

	if ( tasks[ index ] != task ) {
		taskHandle_t handle = tasks[ index ] != NULL ? tasks[ index ]->GetHandle() : taskHandle_t();

		if( gameLocal.IsLocalPlayer( this ) ) {
			gameLocal.localPlayerProperties.SetTaskIndex( va( L"%i/%i", index + 1, num ) );
		}

		if ( gameLocal.isClient ) {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_TASK );
			msg.WriteBits( handle + 1, sdPlayerTask::TASK_BITS + 1 );
			msg.Send();
		} else {
			sdTaskManager::GetInstance().SelectTask( this, handle );
		}
	}
}

/*
============
idPlayer::SetActiveTask
============
*/
void idPlayer::SetActiveTask( taskHandle_t task ) {
	if ( activeTask == task ) {
		if ( gameLocal.IsLocalPlayer( this ) && !task.IsValid() ) {
			gameLocal.localPlayerProperties.ClearTask();
		}
		return;
	}

	bool isLocal = gameLocal.IsLocalPlayer( this );

	if ( activeTask.IsValid() ) {
		sdPlayerTask* taskObject = sdTaskManager::GetInstance().TaskForHandle( activeTask );
		if ( taskObject != NULL ) {
			taskObject->OnPlayerLeft( this );
		}
	}

	activeTask = task;

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SELECTTASK );
		msg.WriteBits( activeTask + 1, ( sdPlayerTask::TASK_BITS + 1 ) );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	if ( activeTask.IsValid() ) {
		sdPlayerTask* taskObject = sdTaskManager::GetInstance().TaskForHandle( activeTask );
		if ( taskObject != NULL ) {
			if ( isLocal ) {
				gameLocal.localPlayerProperties.OnTaskSelected( taskObject );
			}
			taskObject->OnPlayerJoined( this );
		}
	} else {
		if ( isLocal ) {
			gameLocal.localPlayerProperties.OnTaskSelected( NULL );
		}
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		localPlayer->UpdateActiveTask();
	}
}

/*
============
idPlayer::InhibitUserCommands
============
*/
bool idPlayer::InhibitUserCommands() {
	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
	if ( module && module->InhibitUserCommands() ) {
		return true;
	}

	if ( gameLocal.rules->IsEndGame() ) {
		return true;
	}

	if ( gameLocal.IsMainMenuActive() ) {
		return true;
	}

	return false;
}

/*
=====================
idPlayer::RunPausedPhysics
=====================
*/
bool idPlayer::RunPausedPhysics( void ) const {
	if ( team != NULL ) {
		return false;
	}
	return g_allowSpecPauseFreeFly.GetBool();
}

/*
=====================
idPlayer::InhibitMovement
=====================
*/
bool idPlayer::InhibitMovement( void ) const {
	return physicsObj.IsFrozen() || remoteCamera.IsValid();
}

/*
=====================
idPlayer::InhibitTurning
=====================
*/
bool idPlayer::InhibitTurning( void ) {
	if ( ( !playerFlags.noclip && ( gameLocal.GetCamera() || playerFlags.turningFrozen ) ) || teleportEntity.IsValid() ) {
		return true;
	}
	if ( gameLocal.IsPaused() ) {
		if ( team != NULL ) {
			return true;
		}
		if ( !g_allowSpecPauseFreeFly.GetBool() ) {
			return true;
		}
	}
	return false;
}

/*
=====================
idPlayer::UpdateConditions
=====================
*/
void idPlayer::UpdateConditions( void ) {
	idEntity* proxy = GetProxyEntity();
	sdJetPack* jetPack = proxy->Cast< sdJetPack >();

	if ( ( proxy != NULL && jetPack == NULL ) || teleportEntity.IsValid() ) {
		AI_FORWARD			= false;
		AI_BACKWARD			= false;
		AI_STRAFE_LEFT		= false;
		AI_STRAFE_RIGHT		= false;
		AI_RUN				= false;
		AI_DEAD				= false;
		AI_SPRINT			= false;
		AI_LEAN				= 0.f;
		AI_ONGROUND			= true;
		AI_INWATER			= 0;
		return;
	}

	// minus the push velocity to avoid playing the walking animation and sounds when riding a mover
	idVec3 velocity = physicsObj.GetLinearVelocity() - physicsObj.GetPushedLinearVelocity();
	float fallspeed = velocity * physicsObj.GetGravityNormal();

	if ( jetPack ) {
		xyspeed = velocity.LengthFast();
		AI_ONGROUND			= jetPack->OnGround();
		AI_JUMP				= jetPack->GetJetPackPhysics()->HasJumped();

		AI_DEAD				= false;
		AI_LEAN				= 0.f;
		AI_INWATER			= 0;
		AI_PRONE			= false;
		AI_CROUCH			= false;
		AI_ONLADDER			= false;
	} else {
		AI_RUN				= usercmd.buttons.btn.run && !AI_SPRINT;
		AI_DEAD				= health <= 0;
		AI_INWATER			= physicsObj.GetWaterLevel() >= WATERLEVEL_WAIST;
	}

	bool allowMovement = AI_ONGROUND || AI_INWATER;

	if ( gameLocal.time - lastDmgTime < 500 ) { // Gordon: This kinda sucks?
		float forwardspeed = velocity * viewAxis[ 0 ];
		float sidespeed = velocity * viewAxis[ 1 ];
		if ( allowMovement ) {
			AI_FORWARD		= ( forwardspeed > 20.01f );
			AI_BACKWARD		= ( forwardspeed < -20.01f );
			AI_STRAFE_LEFT	= ( sidespeed > 20.01f );
			AI_STRAFE_RIGHT	= ( sidespeed < -20.01f );
		} else {
			AI_FORWARD		= false;
			AI_BACKWARD		= false;
			AI_STRAFE_LEFT	= false;
			AI_STRAFE_RIGHT	= false;
		}
	} else if ( xyspeed > MIN_BOB_SPEED ) {
		if( InhibitMovement() ) {
			AI_FORWARD		= false;
			AI_BACKWARD		= false;
			AI_STRAFE_LEFT	= false;
			AI_STRAFE_RIGHT	= false;
		} else {
			if ( allowMovement ) {
				AI_FORWARD		= usercmd.forwardmove > 0;
				AI_BACKWARD		= usercmd.forwardmove < 0;
				AI_STRAFE_LEFT	= usercmd.rightmove < 0;
				AI_STRAFE_RIGHT	= usercmd.rightmove > 0;
			} else {
				AI_FORWARD		= false;
				AI_BACKWARD		= false;
				AI_STRAFE_LEFT	= false;
				AI_STRAFE_RIGHT	= false;
			}
		}
	} else {
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	}


}

/*
==================
WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::WeaponFireFeedback( const weaponFeedback_t& feedback ) {
	// play the fire animation
	AI_WEAPON_FIRED = true;

	if ( weaponFiredFunction != NULL ) {
		sdScriptHelper h1;
		CallNonBlockingScriptEvent( weaponFiredFunction, h1 );
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		// update view feedback
		gameLocal.playerView.WeaponFireFeedback( feedback );
	}

	idAngles temp = viewAngles;
	if ( IsProne() ) {
		temp.pitch -= feedback.kickbackProne;
	} else {
		temp.pitch -= feedback.kickback;
	}
	SetViewAngles( temp );
}

/*
===============
idPlayer::StopFiring
===============
*/
void idPlayer::StopFiring( void ) {
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED = false;
	AI_RELOAD		= false;

	if ( weapon.IsValid() ) {
		weapon.GetEntity()->EndAttack();
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
void idPlayer::FireWeapon( void ) {
	idMat3 axis;
	idVec3 muzzle;

	if ( g_editEntityMode.GetInteger() ) {
		GetViewPos( muzzle, axis );
		if ( gameLocal.editEntities->SelectEntity( muzzle, axis[0], this ) ) {
			return;
		}
	}

	idWeapon* realWeapon = weapon.GetEntity();

	if ( realWeapon->IsReady() ) {
		const sdDeclInvItem* item = GetInventory().GetCurrentItem();
		assert( item != NULL );

		bool shotsAvailable = true;
		if ( item->GetClips().Num() > 0 ) {
			shotsAvailable = false;
			for ( int i = 0; i < item->GetClips().Num(); i++ ) {
				if ( realWeapon->ShotsAvailable( i ) != 0 ) {
					shotsAvailable = true;
					break;
				}
			}
		}

		bool noAttack = true;
		if ( shotsAvailable ) {
			if ( CanAttack() && shotsAvailable ) {
				AI_ATTACK_HELD = true;
				weapon.GetEntity()->BeginAttack();
				noAttack = false;
			}
		} else if ( userInfo.autoSwitchEmptyWeapons ) {
			if ( !gameLocal.isClient ) {
				if ( !GetInventory().GetWeaponChanged() ) {
					GetInventory().SelectBestWeapon( false );
				}
			}
		}

		if ( noAttack ) {
			AI_ATTACK_HELD = false;
			weapon.GetEntity()->EndAttack();
		}
	}
}

/*
=========================
idPlayer::CanAttack
=========================
*/
bool idPlayer::CanAttack( void ) {
	idWeapon* realWeapon = weapon.GetEntity();
	return realWeapon->CanAttack();
}

/*
===============
idPlayer::UseWeaponAmmo
===============
*/
void idPlayer::UseWeaponAmmo( int modIndex ) {
	if ( !IsWeaponValid() ) {
		return; // blarf
	}

	idWeapon* realWeapon = GetWeapon();
	int amount = realWeapon->AmmoRequired( modIndex );
	ammoType_t type = realWeapon->GetAmmoType( modIndex );
	UseAmmo( type, amount );
}

/*
===============
idPlayer::AmmoForWeapon
===============
*/
int idPlayer::AmmoForWeapon( int modIndex ) {
	idWeapon* realWeapon = GetWeapon();
	if ( !realWeapon ) {
		return 0;
	}

	ammoType_t type = realWeapon->GetAmmoType( modIndex );
	if ( type == -1 ) {
		return 0;
	}
	return GetInventory().GetAmmo( type );
}

/*
===============
idPlayer::UseAmmo
===============
*/
void idPlayer::UseAmmo( ammoType_t type, int amount ) {
	int ammo = GetInventory().GetAmmo( type );
	// take an ammo away if not infinite
	if( ammo >= 0 ) {
		if ( amount > ammo ) {
			amount = ammo;
		}

		GetInventory().SetAmmo( type, ammo - amount );
	}
}

/*
===============
idPlayer::Give
===============
*/
bool idPlayer::Give( const char *statname, const char *value ) {
	if ( AI_DEAD ) {
		return false;
	}

	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( Heal( atoi( value ) ) == 0 ) {
			return false;
		}
	} else if ( !idStr::Icmp( statname, "damage" ) ) {
		if ( atof( value ) <= 0.0f ) {
			return false;
		}
		Damage( this, this, vec3_origin, DAMAGE_FOR_NAME( "damage_give" ), atof( value ), NULL );
	}
	return true;
}

/*
================
idPlayer::Heal
================
*/
int idPlayer::Heal( int count ) {
	if ( health <= 0 ) {
		return 0;
	}

	if ( !playerFlags.inPlayZone ) {
		return 0;
	}

	int take = Min( count, maxHealth - health );
	if ( take < 0 ) {
		take = 0;
	}

	SetHealth( health + take );

	teamDamageDone -= take;
	if ( teamDamageDone < 0 ) {
		teamDamageDone = 0;
	}

	return take;
}

/*
================
idPlayer::SetHealth

modified to have low skilled bots, on the opposite team of the local player, be weaker - only in training or easy mode.
================
*/
void idPlayer::SetHealth( int count ) {
	int oldHealth = health;

	if ( userInfo.isBot ) {
		if ( ( botThreadData.GetBotSkill() == BOT_SKILL_DEMO || botThreadData.GetBotSkill() == BOT_SKILL_EASY ) && botThreadData.GetGameWorldState()->gameLocalInfo.gameIsBotMatch ) {
			idPlayer* localPlayer = gameLocal.GetLocalPlayer();
			bool lowerHealth = false;
			if ( team != NULL && localPlayer != NULL && localPlayer->team != NULL ) {
				if ( team->GetBotTeam() != localPlayer->team->GetBotTeam() ) {
					lowerHealth = true;
				}
			}

			if ( count > LOW_SKILL_BOT_HEALTH && lowerHealth == true ) {
				count = LOW_SKILL_BOT_HEALTH;
			}
		}
	}

	health = count;

	if ( health > oldHealth && health > 0 ) {
		OnHealed( oldHealth, health );
	}
}

/*
================
idPlayer::OnHealed
================
*/
void idPlayer::OnHealed( int oldHealth, int health ) {
	assert( health > oldHealth );

	if ( onHealedFunction != NULL ) {
		sdScriptHelper h1;
		h1.Push( oldHealth );
		h1.Push( health );
		scriptObject->CallNonBlockingScriptEvent( onHealedFunction, h1 );
	}
}

/*
================
idPlayer::GiveClassProficiency
================
*/
void idPlayer::GiveClassProficiency( float count, const char* reason ) {
	sdProficiencyManagerLocal& manager = sdProficiencyManager::GetInstance();
	if ( IsSpectator() ) {
		return;
	}

	const sdDeclPlayerClass* cls = GetInventory().GetClass();
	if ( cls == NULL || cls->GetNumProficiencies() < 1 ) {
		return;
	}

	sdProficiencyManager::GetInstance().GiveProficiency( cls->GetProficiency( 0 ).index, count, this, 1.f, reason );
}

/*
================
idPlayer::Event_SetSkin
================
*/
void idPlayer::Event_SetSkin( const char *skinname ) {
	if ( !skinname || !*skinname ) {
		renderEntity.customSkin = NULL;
	} else {
		renderEntity.customSkin = gameLocal.declSkinType[ skinname ];
	}
}

/*
================
idPlayer::Event_GiveProficiency
================
*/
void idPlayer::Event_GiveProficiency( int index, float scale, idScriptObject* object, const char* reason ) {
	const sdDeclProficiencyItem* item = gameLocal.declProficiencyItemType.SafeIndex( index );
	if ( !item ) {
		gameLocal.Warning( "idPlayer::Event_GiveProficiency Invalid Proficiency Handle" );
		return;
	}

	sdPlayerTask* task = NULL;
	if ( object != NULL ) {
		task = object->GetClass()->Cast< sdPlayerTask >();
		if ( task == NULL ) {
			gameLocal.Warning( "idPlayer::Event_GiveProficiency Invalid Task" );
			return;
		}
	}

	sdProficiencyManagerLocal& manager = sdProficiencyManager::GetInstance();
	manager.GiveProficiency( item, this, scale, task, *reason != '\0' ? reason : NULL );
}

/*
================
idPlayer::Event_GiveClassProficiency
================
*/
void idPlayer::Event_GiveClassProficiency( float count, const char* reason ) {
	GiveClassProficiency( count, reason );
}

/*
================
idPlayer::Event_Heal
================
*/
void idPlayer::Event_Heal( int count ) {
	sdProgram::ReturnInteger( Heal( count ) );
}

/*
================
idPlayer::Event_Unheal
================
*/
void idPlayer::Event_Unheal( int count ) {
	if ( health <= 0 ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	// health - 1 so that this function can never kill a player
	int take = Min( count, health - 1 );
	health -= take;
	lastDamageDecl = -1;
	lastDamageDir = vec3_zero;

	sdProgram::ReturnInteger( take );
}

/*
===============
idPlayer::CanGetClass
===============
*/
bool idPlayer::CanGetClass( const sdDeclPlayerClass* pc ) {
	idCVar* limitCVar = pc->GetLimitCVar();
	if ( limitCVar ) {
		int count = gameLocal.ClassCount( pc, this, GetGameTeam() );
		if ( count >= limitCVar->GetInteger() ) {
			return false;
		}
	}

	return true;
}

/*
===============
idPlayer::GiveItem
===============
*/
bool idPlayer::GiveClass( const char* classname ) {
	if ( IsSpectator() ) {
		return false;
	}

	const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ classname ];
	if ( pc == NULL ) {
		gameLocal.Error( "idPlayer::GiveClass - Tried to give player \"%s\" class: \"%s\"", userInfo.name.c_str(), classname );
	}

	sdTeamInfo* classTeam = pc->GetTeam();
	if ( classTeam != NULL ) {
		if ( team != classTeam ) {
			return false;
		}
	}

	if ( !CanGetClass( pc ) ) {
		return false;
	}

	return GetInventory().GiveClass( pc, true );
}

/*
===============
idPlayer::ChangeClass
===============
*/
void idPlayer::ChangeClass( const sdDeclPlayerClass* pc, int classOption ) {
	if ( IsSpectator() ) {
		return;
	}

	if ( !CanGetClass( pc ) ) {
		return;
	}

	sdTeamInfo* classTeam = pc->GetTeam();
	if ( classTeam != NULL ) {
		if ( team != classTeam ) {
			return;
		}
	}

	if ( gameLocal.rules->IsWarmup() && !IsDead() ) {
		bool changed = GetInventory().GiveClass( pc, false );
		changed |= GetInventory().SetClassOption( 0, classOption, false );
		if ( changed ) {
			GetInventory().SendClassInfo( false );
		}
	} else {
		bool changed = GetInventory().SetCachedClass( pc, false );
		changed |= GetInventory().SetCachedClassOption( 0, classOption, false );
		if ( changed ) {
			GetInventory().SendClassInfo( true );
		}
	}
}

/*
===============
idPlayer::GivePackage
===============
*/
bool idPlayer::GivePackage( const sdDeclItemPackage* package ) {
	if ( IsSpectator() ) {
		return false;
	}

	if ( !package ) {
		return false;
	}

	return GetInventory().GiveConsumables( package );
}


/*
=====================
idPlayer::IsWeaponValid
	NASTY!!!!
=====================
*/
bool idPlayer::IsWeaponValid( void ) {
	return( weapon.IsValid() && GetInventory().IsCurrentWeaponValid() );
}

/*
===============
idPlayer::Reload
===============
*/
void idPlayer::Reload( void ) {
	if ( IsSpectating() || remoteCamera.GetEntity() != NULL ) {
		return;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		if ( !proxy->GetUsableInterface()->GetAllowPlayerWeapon( this ) ) {
			return;
		}
	}

	if ( InhibitWeapon() ) {
		return;
	}

	if ( weapon.IsValid() ) {
		weapon->Reload();
	}
}

/*
===============
idPlayer::NextWeapon
===============
*/
void idPlayer::NextWeapon( bool safe ) {
	sdUsableInterface* iface = NULL;

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		iface = proxy->GetUsableInterface();
	}

	if ( iface && !iface->GetAllowPlayerWeapon( this ) ) {
		if ( gameLocal.isClient ) {
			return;
		}
		iface->NextWeapon( this );
	} else {
		if ( InhibitWeaponSwitch() ) {
			return;
		}

		if( safe ) {
			GetInventory().CycleNextSafeWeapon();
			ShowWeaponMenu( sdWeaponSelectionMenu::AT_DIRECT_SELECTION );
			AcceptWeaponSwitch( false );
		} else {
			GetInventory().CycleWeaponsNext();
			ShowWeaponMenu( sdWeaponSelectionMenu::AT_ENABLED );
		}

//		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	}
}

/*
===============
idPlayer::PrevWeapon
===============
*/
void idPlayer::PrevWeapon( void ) {
	sdUsableInterface* iface = NULL;

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		iface = proxy->GetUsableInterface();
	}

	if ( iface && !iface->GetAllowPlayerWeapon( this ) ) {
		if ( gameLocal.isClient ) {
			return;
		}
		proxy->GetUsableInterface()->PrevWeapon( this );
	} else {
		if ( InhibitWeaponSwitch() ) {
			return;
		}

		GetInventory().CycleWeaponsPrev();
		ShowWeaponMenu( sdWeaponSelectionMenu::AT_ENABLED );

//		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	}
}

/*
===============
idPlayer::SelectWeapon
===============
*/
void idPlayer::SelectWeapon( int num, bool force ) {
	bool allowPause = false;
	int unpauseWeaponSlot = currentToolTip != NULL ? currentToolTip->GetUnpauseWeaponSlot() - 1 : -1;
	if ( unpauseWeaponSlot >= 0 ) {
		allowPause = true;
	}

	if ( InhibitWeaponSwitch( allowPause ) ) {
		return;
	}

	bool looped = false;
	int weapon = GetInventory().CycleWeaponByPosition( num, true, looped, force, false );
	if ( weapon != -1 ) {
		if ( num == unpauseWeaponSlot && gameLocal.isPaused && gameLocal.isServer ) {
			lastWeaponSwitchPos = -1;
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "unPauseGame\n" ); 
		}

		GetInventory().SetSwitchingWeapon( weapon );
	}
	ShowWeaponMenu( sdWeaponSelectionMenu::AT_DIRECT_SELECTION );
	if ( weapon != -1 ) {
		AcceptWeaponSwitch( false );
	}
}

/*
===============
idPlayer::InhibitHud
===============
*/
bool idPlayer::InhibitHud( void ) {
	if ( !g_showHud.GetBool() ) {
		return true;
	}

	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
	if ( module ) {
		sdUserInterfaceLocal* ui = module->GetGui();
		if ( ui ) {
			return false;
		}
	}

	if ( focusUI ) {
		return true;
	}

	return false;
}

/*
===============
idPlayer::ActiveGui
===============
*/
guiHandle_t idPlayer::ActiveGui( void ) {
	return focusUI;
}

/*
===============
idPlayer::Weapon_Proxy
===============
*/
void idPlayer::Weapon_Proxy( void ) {
	if ( weapon.IsValid() ) {
		idWeapon* w = weapon;
		w->EndAttack();
		w->EndAltFire();
	}
	// cancel weapon any passive switches
	if ( weaponSwitchTimeout  ) {
		GetInventory().CancelWeaponSwitch();
		ShowWeaponMenu( sdWeaponSelectionMenu::AT_DISABLED );
	}
}

/*
===============
idPlayer::CheckBoundsForDeployment
===============
*/
deployResult_t idPlayer::CheckBoundsForDeployment( const sdDeployMask::extents_t& extents, const sdDeployMaskInstance& mask, const sdDeclDeployableObject* deploymentObject, const sdPlayZone* pz ) {
	deployResult_t result = mask.IsValid( extents );
	if ( result == DR_FAILED ) {
		return result;
	}

	const sdHeightMapInstance& heightMap = pz->GetHeightMap();
	if ( !heightMap.IsValid() ) {
		return DR_FAILED;
	}

	idBounds maskBounds;
	mask.GetBounds( extents, maskBounds, &heightMap );

	idEntity* entities[ MAX_GENTITIES ];

	if ( pz == NULL ) {
		return DR_FAILED;
	}

	if ( result == DR_CLEAR ) {
		maskBounds.GetMaxs().z += 128.f;

		int count = gameLocal.clip.EntitiesTouchingBounds( maskBounds, MASK_ALL, entities, MAX_GENTITIES );
		for ( int i = 0; i < count; i++ ) {
			if ( !entities[ i ]->fl.preventDeployment ) {
				continue;
			}

			if ( entities[ i ]->OverridePreventDeployment( this ) ) {
				continue;
			}

			result = DR_WARNING;
			break;
		}
	}

	if ( result == DR_CLEAR ) {
		sdDeployRequest* request = gameLocal.GetDeploymentRequest( this );
		if ( request ) {
			result = DR_WARNING;
		}
	}

	if ( result == DR_CLEAR ) {
		result = gameLocal.CheckDeploymentRequestBlock( maskBounds );
	}

	return result;
}

idCVar g_skipDeployChecks( "g_skipDeployChecks", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "Skips deployment territory checks, etc" );

/*
===============
idPlayer::GetDeployPosition
===============
*/
bool idPlayer::GetDeployPosition( idVec3& point ) {
	point.Zero();

	renderView_t tempView;
	SetupDeploymentView( tempView );

	const idVec3& org	= tempView.vieworg;
	const idMat3& axis	= tempView.viewaxis;

	trace_t trace;
	memset( &trace, 0, sizeof( trace ) );
	idVec3 end = org + ( axis[ 0 ] * 2048.f );

	gameLocal.clip.TranslationWorld( CLIP_DEBUG_PARMS trace, org, end, NULL, mat3_identity, MASK_SOLID | MASK_OPAQUE );
	if ( trace.fraction == 1.f ) {
		return false;
	}

	point = trace.endpos;

	return true;
}

/*
===============
idPlayer::GetDeployResult
===============
*/
deployResult_t idPlayer::GetDeployResult( idVec3& point, const sdDeclDeployableObject* deploymentObject ) {
	if ( !GetDeployPosition( point ) ) {
		return DR_OUT_OF_RANGE;
	}

	return CheckDeployPosition( point, deploymentObject );
}

/*
===============
idPlayer::CheckDeployPosition
===============
*/
deployResult_t idPlayer::CheckDeployPosition( idVec3& point, const sdDeclDeployableObject* deploymentObject ) {
	idVec3 basePos = point;

	if ( IsSpectating() || deploymentObject == NULL ) {
		return DR_CONDITION_FAILED;
	}

	const sdPlayZone* playZone = gameLocal.GetPlayZone( point, sdPlayZone::PZF_DEPLOYMENT );
	if ( playZone == NULL ) {
		return DR_CONDITION_FAILED;
	}

	const sdDeployMaskInstance* mask = playZone->GetMask( deploymentObject->GetDeploymentMask() );
	if ( mask == NULL ) {
		return DR_CONDITION_FAILED;
	}

	const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( point, sdPlayZone::PZF_HEIGHTMAP );
	if ( playZoneHeight == NULL ) {
		return DR_CONDITION_FAILED;
	}

	point = mask->SnapToGrid( point, 1.f );

	if ( g_skipDeployChecks.GetBool() ) {
		return DR_CLEAR;
	}

	if ( !gameLocal.DoSkyCheck( basePos ) ) {
		return DR_CONDITION_FAILED;
	}

	sdTeamInfo* t = GetGameTeam();
	if ( !gameLocal.TerritoryForPoint( point, t, true ) ) {
		return DR_CONDITION_FAILED;
	}

	idBounds bounds( point );
	bounds.ExpandSelf( deploymentObject->GetObjectSize() );

	sdDeployMask::extents_t extents;
	mask->CoordsForBounds( bounds, extents );

	return CheckBoundsForDeployment( extents, *mask, deploymentObject, playZoneHeight );
}

/*
===============
idPlayer::ServerDeploy
===============
*/
bool idPlayer::ServerDeploy( const sdDeclDeployableObject* object, float rotation, int delayMS ) {
	idVec3 point;
	deployResult_t result = GetDeployResult( point, object );
	if ( result != DR_CLEAR ) {
		return false;
	}
	return gameLocal.RequestDeployment( this, object, point, rotation, delayMS );
}

/*
==============
idPlayer::GetClip
==============
*/
int idPlayer::GetClip( int modIndex ) {
	if ( inventoryExt.GetCurrentWeapon() < 0 ) {
		return 0;
	}

	return inventoryExt.GetClip( inventoryExt.GetCurrentWeapon(), modIndex );
}

/*
==============
idPlayer::SetClip
==============
*/
void idPlayer::SetClip( int modIndex, int count ) {
	inventoryExt.SetClip( inventoryExt.GetCurrentWeapon(), modIndex, count );
}

/*
===============
idPlayer::InhibitWeapon
===============
*/
bool idPlayer::InhibitWeapon( void ) const {
	if ( ( gameLocal.time - lastReviveTime ) < SEC2MS( 1.5f ) ) {
		return true;
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
		if ( module != NULL && module->HideWeapon() ) {
			return true;
		}
	}

	if ( physicsObj.IsProne() && !InhibitMovement() ) {
		usercmd_t& cmd = gameLocal.usercmds[ entityNumber ];
		if ( cmd.forwardmove != 0 || cmd.rightmove != 0 ) {
			return true;
		}
	}

	if ( physicsObj.GetWaterLevel() >= WATERLEVEL_WAIST ) {
		return true;
	}

	if ( physicsObj.OnLadder() ) {
		return true;
	}

	if ( IsBeingBriefed() ) {
		return true;
	}

	if ( teleportEntity.IsValid() ) {
		return true;
	}

	if ( gameLocal.time <= lastTeleportTime + gameLocal.msec * 2 ) {
		return true;
	}

	return false;
}

/*
===============
idPlayer::InhibitWeaponSwitch
===============
*/
bool idPlayer::InhibitWeaponSwitch( bool allowPause ) const {
	if ( IsSpectating() ) {
		return true;
	}

	if ( IsBeingBriefed() ) {
		return true;
	}

	if ( !allowPause && gameLocal.IsPaused() ) {
		return true;
	}

	if ( physicsObj.IsProne() ) {
		usercmd_t& cmd = gameLocal.usercmds[ entityNumber ];
		if ( cmd.forwardmove != 0 || cmd.rightmove != 0 ) {
			return true;
		}
	}

	if ( physicsObj.GetProneChangeEndTime() > gameLocal.time ) {
		return true;
	}

//	if ( physicsObj.OnLadder() ) {
//		return true;
//	}

	if ( physicsObj.GetWaterLevel() >= WATERLEVEL_WAIST ) {
		return true;
	}

	if ( remoteCamera.GetEntity() != NULL ) {
		return true;
	}

	const idWeapon* weapon = GetWeapon();
	if ( weapon ) {
		const sdDeclInvItem* item = weapon->GetInvItemDecl();
		if ( item && !item->GetWeaponChangeAllowed() ) {
			return true;
		}
	}

	return false;
}

/*
===============
idPlayer::SelectWeaponByName
===============
*/
void idPlayer::SelectWeaponByName( const char* weaponName ) {
	const sdDeclPlayerClass* pc = GetInventory().GetClass();
	if ( pc == NULL ) {
		return;
	}

	GetInventory().SelectWeaponByName( pc->GetClassData().GetString( weaponName ), false );
}

/*
===============
idPlayer::Weapon_Combat
===============
*/
idCVar g_skipWeaponSwitchAnimations( "g_skipWeaponSwitchAnimations", "1", CVAR_BOOL, "If 1, players won't play their weapon switching animations." );

void idPlayer::Weapon_Combat( void ) {
	if( playerFlags.weaponLock ) {
		if( usercmd.buttons.btn.attack ) {
			usercmd.buttons.btn.attack = false;
		} else {
			playerFlags.weaponLock = false;
		}
	}

	bool currentValid = IsWeaponValid();
	idWeapon* realWeapon = weapon.GetEntity();
	if( !realWeapon ) {
		return;
	}

	bool raise = !InhibitWeapon();
	if ( !raise ) {
		realWeapon->LowerWeapon();
	} else if ( currentValid ) {
		realWeapon->RaiseWeapon();
	}

	if ( realWeapon->IsReloading() ) {
		if ( !AI_RELOAD ) {
			AI_RELOAD = true;
			//SetState( "ReloadWeapon" );
			if( reloadWeaponFunction ) {
				sdScriptHelper h;
				scriptObject->CallNonBlockingScriptEvent( reloadWeaponFunction, h );
			}
		}
	} else {
		AI_RELOAD = false;
	}

	if( gameLocal.IsLocalPlayer( this ) ) {
		sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();

		if( ( enableWeaponSwitchTimeout && weaponSwitchTimeout && gameLocal.ToGuiTime( gameLocal.time ) >= weaponSwitchTimeout ) || IsDead() || NeedsRevive() ) {
			GetInventory().CancelWeaponSwitch();
			ShowWeaponMenu( sdWeaponSelectionMenu::AT_DISABLED );
		}

		if( weaponMenu->IsPassiveSwitchActive() ) {
			if ( ( !clientOldButtons.btn.attack && clientButtons.btn.attack ) || ( !clientOldButtons.btn.altAttack && clientButtons.btn.altAttack ) ) {
				if ( !gameLocal.isPaused ) {
					AcceptWeaponSwitch();
				}
			}
		}
	}

	// Gordon: not sure what this weaponActive thing is about
	bool weaponActive = ( usercmd.buttons.btn.attack || usercmd.buttons.btn.altAttack || usercmd.buttons.btn.modeSwitch );
	bool allowSwitch =	( !AI_PUTAWAY_ACTIVE || ( AI_PUTAWAY_ACTIVE && weaponActive )) &&
						( !AI_TAKEOUT_ACTIVE || ( AI_TAKEOUT_ACTIVE && weaponActive ));

	if( !IsSpectating() && ( allowSwitch || g_skipWeaponSwitchAnimations.GetBool() )) {
		if ( playerFlags.weaponChanged || GetInventory().GetWeaponChanged() || !currentValid || !realWeapon->IsLinked() ) {
			if ( gameLocal.IsLocalPlayer( this ) ) {
				sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
				if ( module != NULL && module->HideWeapon() ) {
					module->Enable( false );
				}
			}

			if( currentValid ) {
				if( realWeapon->IsReady() ) {
					realWeapon->PutAway();
				}
			}

			if( realWeapon->IsHolstered() || !currentValid ) {
				sdInventory& inv = GetInventory();

				inv.UpdateCurrentWeapon();

				if( inv.IsCurrentWeaponValid() ) {
					const sdDeclInvItem* item = inv.GetCurrentItem();
					realWeapon->ScriptCleanup();
					realWeapon->GetWeaponDef( item );

					realWeapon->Raise();
					if( weaponActive ) {
						realWeapon->MakeReady();
					}
					playerFlags.weaponChanged = false;
				}
			}
		} else if( realWeapon->IsHolstered() ) {
			if ( currentValid ) {
				if ( realWeapon->ShotsAvailable( 0 ) == 0 ) {
					if ( !gameLocal.isClient ) {
						if ( userInfo.autoSwitchEmptyWeapons ) {
							NextWeapon( true );
						}
					}
				} else {
					realWeapon->Raise();
					if( raiseWeaponFunction ) {
						sdScriptHelper h;
						scriptObject->CallNonBlockingScriptEvent( raiseWeaponFunction, h );
					}
				}
			}
		}
	}

	if ( /* gameLocal.rules->IsWarmup() || */ !raise ) {
		if( currentValid ) {
			realWeapon->EndAttack();
			realWeapon->EndAltFire();
			realWeapon->EndModeSwitch();
		}
	} else {
		// check for attack
		AI_WEAPON_FIRED = false;
		if ( currentValid ) {
			if ( usercmd.buttons.btn.attack || ( realWeapon->ActivateAttack() && usercmd.buttons.btn.activate && !crosshairEntActivate ) ) {
				FireWeapon();
			} else {
				if ( oldButtons.btn.attack ) {
					AI_ATTACK_HELD = false;
				}
				realWeapon->EndAttack();
			}

			if( usercmd.buttons.btn.altAttack ) {
				realWeapon->BeginAltFire();
			} else {
				realWeapon->EndAltFire();
			}

			if( usercmd.buttons.btn.modeSwitch ) {
				realWeapon->BeginModeSwitch();
			} else {
				realWeapon->EndModeSwitch();
			}
		}
	}
}

/*
===============
idPlayer::LowerWeapon
===============
*/
void idPlayer::LowerWeapon( void ) {
	if ( weapon.GetEntity() && !weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->LowerWeapon();
	}
}

/*
===============
idPlayer::RaiseWeapon
===============
*/
void idPlayer::RaiseWeapon( void ) {
	if ( weapon.GetEntity() && weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->RaiseWeapon();
	}
}

/*
===============
idPlayer::WeaponLoweringCallback
===============
*/
void idPlayer::WeaponLoweringCallback( void ) {
	if( lowerWeaponFunction ) {
		sdScriptHelper h;
		scriptObject->CallNonBlockingScriptEvent( lowerWeaponFunction, h );
	}
}

/*
===============
idPlayer::WeaponRisingCallback
===============
*/
void idPlayer::WeaponRisingCallback( void ) {
	if( raiseWeaponFunction ) {
		sdScriptHelper h;
		scriptObject->CallNonBlockingScriptEvent( raiseWeaponFunction, h );
	}
}

/*
===============
idPlayer::Weapon_GUI
===============
*/
void idPlayer::Weapon_GUI( void ) {
	playerFlags.weaponLock			= true;
	playerFlags.clientWeaponLock	= true;

	idWeapon* w = weapon;
	if ( w ) {
		w->LowerWeapon();
	}
}

/*
===============
idPlayer::UpdateWeapon
===============
*/
void idPlayer::UpdateWeapon( void ) {
	if ( health <= 0 ) {
		return;
	}

	assert( !IsSpectating() );

	idEntity* proxy = GetProxyEntity();

	// gui handling overrides weapon use
	if( ActiveGui() ) {
		Weapon_GUI();
	} else {
		bool allowWeapon = true;
		if ( proxy ) {
			sdUsableInterface* iface = proxy->GetUsableInterface();
			if ( !iface->GetAllowPlayerWeapon( this ) ) {
				allowWeapon = false;
			}
		}

		if ( !allowWeapon ) {
			Weapon_Proxy();
		} else {
			Weapon_Combat();
		}
	}

	if ( gameLocal.isNewFrame ) {
		weapon->UpdateScript();
	}

	if ( !proxy ) {
		weapon->PresentWeapon();
	}
	SetWeaponSpreadInfo();
	weapon->UpdateSpreadValue();
}

/*
===============
idPlayer::SpectateFreeFly
===============
*/
void idPlayer::SpectateFreeFly( bool force ) {
	idVec3		newOrig;

	idPlayer* player = GetSpectateClient();

	if ( !force && gameLocal.time <= nextSpectateChange ) {
		return;
	}

	SetSpectateClient( this );

	if ( player && player != this && !player->IsSpectating() ) {
		newOrig = player->GetPhysics()->GetOrigin();
		if ( player->physicsObj.IsCrouching() ) {
			newOrig[ 2 ] += pm_crouchviewheight.GetFloat();
		} else {
			newOrig[ 2 ] += pm_normalviewheight.GetFloat();
		}
		newOrig[ 2 ] += SPECTATE_RAISE;
		idBounds b = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
		idVec3 start = player->GetPhysics()->GetOrigin();
		start[ 2 ] += pm_spectatebbox.GetFloat() * 0.5f;

		// assuming spectate bbox is inside stand or crouch box
		trace_t t;
		gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS t, start, newOrig, b, mat3_identity, MASK_PLAYERSOLID, player );

		newOrig.Lerp( start, newOrig, t.fraction );
		SetOrigin( newOrig );

		idAngles angle = player->viewAngles;
		angle[ 2 ] = 0;
		SetViewAngles( angle );
	} else {
		idVec3		spawnOrigin;
		idAngles	spawnAngles;

		gameLocal.SelectInitialSpawnPoint( this, spawnOrigin, spawnAngles );
		spawnOrigin[ 2 ] += pm_normalviewheight.GetFloat();
		spawnOrigin[ 2 ] += SPECTATE_RAISE;

		SetOrigin( spawnOrigin );
		SetViewAngles( spawnAngles );
	}
	nextSpectateChange = gameLocal.time + 500;
}

idCVar g_noBotSpectate( "g_noBotSpectate", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "disables the ability to spectate bots" );

/*
================
idGameLocal::GetNextSpectateClient
================
*/
idPlayer* idPlayer::GetNextSpectateClient( bool reverse ) const {
	if ( IsSpectator() ) {
		sdTeamInfo* currentTeam = NULL;
		idPlayer* player = gameLocal.GetClient( spectateClient );
		if ( player != NULL ) {
			currentTeam = player->GetGameTeam();
		}

		// go through twice
		for ( int pass = 0; pass < 2; pass++ ) {
			// attempt to cycle through everyone from the current team
			int temp = spectateClient;
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( !reverse ) {
					temp++;
					if ( temp >= MAX_CLIENTS ) {
						break;
					}
				} else {
					temp--;
					if ( temp < 0 ) {
						break;
					}
				}

				idPlayer* player = gameLocal.GetClient( temp );
				if ( player == NULL ) {
					continue;
				}

				if ( g_noBotSpectate.GetBool() ) {
					if ( player->IsType( idBot::Type ) ) {
						continue;
					}
				}

				if ( player->IsSpectating() ) {
					continue;
				}

				if ( currentTeam != NULL ) {
					if ( player->GetGameTeam() != currentTeam ) {
						continue;
					}
				}

				return player;
			}

			// attempt to find the first person 
			temp = -1;
			if ( reverse ) {
				temp = MAX_CLIENTS;
			}
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( !reverse ) {
					temp++;
				} else {
					temp--;
				}

				if ( temp == spectateClient ) {
					break;
				}


				idPlayer* player = gameLocal.GetClient( temp );
				if ( player == NULL ) {
					continue;
				}

				if ( g_noBotSpectate.GetBool() ) {
					if ( player->IsType( idBot::Type ) ) {
						continue;
					}
				}

				if ( player->IsSpectating() || player->GetGameTeam() == currentTeam ) {
					continue;
				}

				return player;
			}

			currentTeam = NULL;
		}
	} else {
		int temp = spectateClient;
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !reverse ) {
				temp = ( temp + 1 ) % MAX_CLIENTS;
			} else {
				temp = ( temp - 1 ) % MAX_CLIENTS;
			}

			idPlayer* player = gameLocal.GetClient( temp );
			if ( player == NULL ) {
				continue;
			}

			if ( g_noBotSpectate.GetBool() ) {
				if ( player->IsType( idBot::Type ) ) {
					continue;
				}
			}

			if ( player->IsSpectating() ) {
				continue;
			}

			if ( player->GetGameTeam() == team ) {
				return player;
			}
		}
	}

	return NULL;
}

/*
===============
idPlayer::GetPrevSpectateClient
===============
*/
idPlayer* idPlayer::GetPrevSpectateClient( void ) const {
	return GetNextSpectateClient( true );
}

/*
===============
idPlayer::SpectateCycle
===============
*/
void idPlayer::SpectateCycle( bool force, bool reverse ) {
	if ( ( gameLocal.time <= nextSpectateChange || gameLocal.IsPaused() ) && !force ) {
		return;
	}
	nextSpectateChange	= gameLocal.time + SEC2MS( 0.5f );

	idPlayer* player = GetNextSpectateClient( reverse );
	if ( player == NULL ) {
		return;
	}
	SetSpectateClient( player );
}

/*
===============
idPlayer::SpectateCrosshair
===============
*/
void idPlayer::SpectateCrosshair( bool force ) {
	if ( ( gameLocal.time <= nextSpectateChange ) && !force ) {
		return;
	}

	if ( !crosshairInfo.IsValid() ) {
		return;
	}

	idEntity* crosshairEntity = crosshairInfo.GetEntity();
	if ( crosshairEntity == NULL ) {
		return;
	}

	idPlayer* foundPlayer = NULL;

	// get passengers of vehicles
	sdUsableInterface* usable = crosshairEntity->GetUsableInterface();
	if ( usable != NULL && usable->GetNumPositions() > 0 ) {
		for ( int i = 0; i < usable->GetNumPositions(); i++ ) {
			idPlayer* player = usable->GetPlayerAtPosition( i );
			if ( player != NULL ) {
				foundPlayer = player;
				break;
			}
		}
	}

	// check if its a player
	if ( foundPlayer == NULL ) {
		foundPlayer = crosshairEntity->Cast< idPlayer >();
	}

	// still didn't find a player
	if ( foundPlayer == NULL ) {
		return;
	}

	SetSpectateClient( foundPlayer );
	nextSpectateChange	= gameLocal.time + SEC2MS( 0.5f );
}

/*
===============
idPlayer::SpectateObjective
===============
*/
void idPlayer::SpectateObjective( void ) {
	idEntity* spectateEntity = sdObjectiveManager::GetInstance().GetSpectateEntity();
	if ( spectateEntity == NULL ) {
		return;
	}

	idPlayer* spectatePlayer = spectateEntity->Cast< idPlayer >();
	if ( spectatePlayer != NULL ) {
		SetSpectateClient( spectatePlayer );
		return;
	}

	// its not a player - instead its an entity to be observed
	// will need to find a place to observe from
	idVec3 lookAtOrigin = spectateEntity->GetPhysics()->GetAbsBounds().GetCenter();
	float idealDistance = 48.0f;

	idClipModel* playerModel = GetPhysics()->GetClipModel( 0 );

	const idMat3& axis = spectateEntity->GetPhysics()->GetAxis();
	idVec3 startDirection = -axis[ 0 ];
	idAngles startAngles = startDirection.ToAngles();
	startAngles.Normalize180();

	int clipMask = MASK_DEADSOLID;

	// test a bunch of angles to find one that works
	// note this could be rather expensive, but it shouldn't take long to find a suitable result
	idVec3 cameraOrigin = lookAtOrigin;
	idVec3 lookDirection = viewAxis[ 0 ];
	for ( float pitch = 0.0f; pitch <= 90.0f; pitch += 30.0f ) {
		for ( float yaw = 0.0f; yaw <= 180.0f; yaw += 30.0f ) {
			idAngles	testAngles;
			idVec3		testDirection;
			trace_t		trace;

			float testSigns[][ 2 ] = {	{ -1.0f, -1.0f },
										{ -1.0f, 1.0f },
										{ 1.0f, -1.0f },
										{ 1.0f, 1.0f } };

			int numTests = sizeof( testSigns ) / ( sizeof( float ) * 2 );
			if ( ( yaw == 0.0f && pitch == 0.0f ) || yaw == 180.0f ) {
				numTests = 1;
			}

			for ( int i = 0; i < numTests; i++ ) {
				testAngles.Set( 20.0f + testSigns[ i ][ 1 ]*pitch, startAngles.yaw - 30.0f + testSigns[ i ][ 0 ]*yaw, 0.0f );
				testDirection = testAngles.ToForward();

				idVec3 testFromOrigin = lookAtOrigin - testDirection * idealDistance;
				idVec3 testToOrigin = lookAtOrigin;

				gameLocal.clip.Translation( trace, testToOrigin, testFromOrigin, playerModel, mat3_identity, clipMask, NULL );
				if ( trace.fraction > 0.5f ) {
					testFromOrigin = trace.endpos;
					if ( !gameLocal.clip.Contents( testFromOrigin, playerModel, mat3_identity, clipMask, NULL ) ) {
						gameLocal.clip.TracePoint( trace, testFromOrigin, testToOrigin, clipMask, NULL );
						float distLeft = ( 1.0f - trace.fraction ) * ( testFromOrigin - testToOrigin ).Length();
						if ( distLeft < 2.0f ) {
							// got a valid result, drop out of the tests
							testToOrigin = trace.endpos;
							cameraOrigin = testFromOrigin;
							lookDirection = testDirection;
							pitch = 10000.0f;
							yaw = 10000.0f;
							break;
						}
					}
				}
			}
		}
	}


	SetSpectateClient( NULL );
	SetViewAngles( lookDirection.ToAngles() );
	GetPhysics()->SetLinearVelocity( vec3_origin );
	SetOrigin( cameraOrigin );
}

/*
===============
idPlayer::SpectateCommand
===============
*/
void idPlayer::SpectateCommand( spectateCommand_t command, const idVec3& inOrigin, const idAngles& inAngles ) {
	assert( IsSpectator() );
	assert( !gameLocal.isClient );

	idPlayer* spectatePlayer = GetSpectateClient();
	bool forceCycle = !spectatePlayer || ( spectatePlayer->IsSpectating() && spectatePlayer != this );

	// sanity check the origin & angle values - n0 h4xx0rz h3r3, kthxbai!!1!one!
	idVec3 origin = inOrigin;
	idAngles angles = inAngles;
	for ( int i = 0; i < 3; i++ ) {
		if ( FLOAT_IS_NAN( origin[ i ] ) || FLOAT_IS_INF( origin[ i ] ) ||
			FLOAT_IS_IND( origin[ i ] ) || FLOAT_IS_DENORMAL( origin[ i ] ) ) {
			origin[ i ] = 0.0f;
		}
		if ( FLOAT_IS_NAN( angles[ i ] ) || FLOAT_IS_INF( angles[ i ] ) ||
			FLOAT_IS_IND( angles[ i ] ) || FLOAT_IS_DENORMAL( angles[ i ] ) ) {
			angles[ i ] = 0.0f;
		}
	}


	if ( command == SPECTATE_NEXT ) {
		SpectateCycle( forceCycle, false );
	} else if ( command == SPECTATE_PREV ) {
		SpectateCycle( forceCycle, true );
	} else if ( command == SPECTATE_OBJECTIVE ) {
		SpectateObjective();
	} else if ( command == SPECTATE_POSITION ) {
		// check the position is valid
		const idBounds& worldBounds = gameLocal.clip.GetWorldBounds();
		origin.Clamp( worldBounds.GetMins(), worldBounds.GetMaxs() );

		idClipModel* playerModel = GetPhysics()->GetClipModel( 0 );
		if ( !gameLocal.clip.Contents( origin, playerModel, mat3_identity, MASK_DEADSOLID, NULL ) ) {
			SetSpectateClient( NULL );
			SetViewAngles( angles );
			GetPhysics()->SetLinearVelocity( vec3_origin );
			SetOrigin( origin );
		}
	}
}

/*
===============
idPlayer::UpdateSpectating
===============
*/
void idPlayer::UpdateSpectating( const usercmd_t& oldCmd ) {
	assert( IsSpectating() );
	assert( !gameLocal.isClient );

	idPlayer* player = GetSpectateClient();

	if ( team == NULL ) {
		playerFlags.forceRespawn = false;

		int maxSpecTime = MINS2MS( g_maxSpectateTime.GetFloat() );
		if ( maxSpecTime > 0 ) {
			if ( ( gameLocal.time - lastTeamSetTime ) > maxSpecTime ) {
				gameLocal.rules->SetClientTeam( this, gameLocal.rules->FindNeedyTeam()->GetIndex() + 1, true, NULL );
				return;
			}
		}

		if ( !player || ( player->IsSpectating() && player != this ) ) {
			SpectateCycle( true );
		} else if ( usercmd.buttons.btn.altAttack && !oldCmd.buttons.btn.altAttack ) {
			SpectateFreeFly( false );
		} else if ( usercmd.buttons.btn.attack && !oldCmd.buttons.btn.attack ) {
			SpectateCycle( false );
		}
	} else {
		if ( !player || ( player->IsSpectating() && player != this ) || ( GetEntityAllegiance( player ) != TA_FRIEND ) ) {
			SpectateCycle( true );
		} else if ( usercmd.buttons.btn.attack && !oldCmd.buttons.btn.attack ) {
			SpectateCycle( false );
		}
	}
}

/*
==============
idPlayer::Collide
==============
*/
bool idPlayer::Collide( const trace_t &collision, const idVec3& velocity, int bodyId ) {
	assert( !playerFlags.noclip );

	if ( velocity.IsZero() ) {
		return false;
	}

	idEntity* other = gameLocal.entities[ collision.c.entityNum ];
	if ( other ) {
		other->OnTouch( this, collision );
	}

	AI_SOFTLANDING = false;
	AI_HARDLANDING = false;

	float delta = ( velocity * -collision.c.normal );
	if ( delta < 1.0f ) {
		return false;
	}

	float gravityVel = ( velocity * GetPhysics()->GetGravityNormal() );

	bool noDamage = playerFlags.godmode || playerFlags.noFallingDamage;
	if ( collision.c.material != NULL && collision.c.material->GetSurfaceFlags() & SURF_NODAMAGE ) {
		noDamage = true;
		StartSound( "snd_land_hard", SND_ANY, 0, NULL );
	}

	bool doLand = true;

	const float VELOCITY_LAND_HARD = 1400.f;
	const float VELOCITY_LAND_MEDIUM = 1280.f;
	const float VELOCITY_LAND_LIGHT = 720.f;
	const float VELOCITY_LAND_SOFT = 80.f;

	if ( gravityVel > VELOCITY_LAND_HARD ) {
		AI_HARDLANDING = true;
		landChange = -32;
	} else if ( gravityVel > VELOCITY_LAND_MEDIUM ) {
		AI_HARDLANDING = true;
		landChange	= -24;
	} else if ( gravityVel > VELOCITY_LAND_LIGHT ) {
		AI_HARDLANDING = true;
		landChange	= -16;
	} else if ( gravityVel > VELOCITY_LAND_SOFT ) {
		AI_SOFTLANDING = true;
		landChange	= -8;
	} else {
		doLand = false;
	}

	if ( doLand ) {
		painDebounceTime	= gameLocal.time + painDelay + 1;  // ignore pain since we'll play our landing anim
		landTime			= gameLocal.time;
	}

	if ( !noDamage ) {
		if ( delta > VELOCITY_LAND_HARD ) {
			Damage( NULL, NULL, idVec3( 0.0f, 0.0f, -1.0f ), DAMAGE_FOR_NAME( "damage_fatalfall" ), 1.f, 0 );
		} else if ( delta > VELOCITY_LAND_MEDIUM ) {
			Damage( NULL, NULL, idVec3( 0.0f, 0.0f, -1.0f ), DAMAGE_FOR_NAME( "damage_hardfall" ), 1.f, 0 );
		} else if ( delta > VELOCITY_LAND_LIGHT ) {
			Damage( NULL, NULL, idVec3( 0.0f, 0.0f, -1.0f ), DAMAGE_FOR_NAME( "damage_softfall" ), 1.f, 0 );
		}

		const sdDeclPlayerClass* cls = GetInventory().GetClass();
		if ( gameLocal.isNewFrame ) {
			if ( cls != NULL && ( gameLocal.time - lastGroundContactTime ) > SEC2MS( 0.5f ) ) {
				const char *soundStr = NULL;
				const idSoundShader *shader = NULL;

				if ( delta > VELOCITY_LAND_MEDIUM ) {
					soundStr = cls->GetClassData().GetString( "snd_land_hard" );
					shader = gameLocal.declSoundShaderType[ soundStr ];
				} else if ( delta > VELOCITY_LAND_MEDIUM / 2.0f ) {
					soundStr = cls->GetClassData().GetString( "snd_land_soft" );
					shader = gameLocal.declSoundShaderType[ soundStr ];
				}

				if ( shader != NULL ) {
					StartSoundShader( shader, SND_PLAYER_LAND, 0, NULL );
				}
			}
		}
	}

	// crash landing effect
	if ( team != NULL && !IsDead() ) {
		float threshold = 0.0f;
		const char* effectName;
		if ( noDamage ) {
			threshold = team->GetParachuteLandThreshold();
			effectName = team->GetParachuteLandFxName();
		} else {
			threshold = team->GetCrashLandThreshold();
			effectName = team->GetCrashLandFxName();
		}
		if ( threshold > 0.0f && delta > threshold ) {
			int effectHandle = gameLocal.FindEffect( effectName )->Index();
			gameLocal.PlayEffect( effectHandle, colorWhite.ToVec3(), collision.endpos, collision.c.normal.ToMat3(), false, vec3_origin );
		}
	}

	return false;
}

/*
================
idPlayer::ClearFocus

Clears the focus cursor
================
*/
void idPlayer::ClearFocus( void ) {
	focusGUIent		= NULL;
	focusUI			= 0;
}

/*
================
idPlayer::InhibitGuiFocus
================
*/
bool idPlayer::InhibitGuiFocus( void ) {
	if ( ( IsWeaponValid() && GetWeapon()->IsAttacking() ) || usercmd.buttons.btn.attack || IsSpectating() ) {
		return true;
	}

	// prevent the player getting into a GUI when the animation is playing on their weapon etc
	if ( playerFlags.inhibitGUIs ) {
		return true;
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
		if ( module && module->InhibitGuiFocus() ) {
			return true;
		}
	}

	return false;
}

/*
================
idPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
================
*/
void idPlayer::UpdateFocus( void ) {
	sdUserInterfaceLocal*	oldUI		= gameLocal.GetUserInterface( focusUI );
	idEntity*				oldFocus	= focusGUIent;

	if ( focusTime <= gameLocal.time ) {
		ClearFocus();
	}

	if ( InhibitGuiFocus() ) {
		return;
	}

	idVec3 start	= GetEyePosition();
	idVec3 end		= start + viewAngles.ToForward() * 80.0f;

	idBounds bounds( start );
	bounds.AddPoint( end );

	idEntity* entityList[ MAX_GENTITIES ];
	int listedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	guiPoint_t pt;

	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	for ( int i = 0; i < listedEntities; i++ ) {
		idEntity* ent = entityList[ i ];

		if ( ent->IsHidden() ) {
			continue;
		}

		renderEntity_t* focusGUIrenderEntity = ent->GetRenderEntity();
		if ( focusGUIrenderEntity == NULL ) {
			continue;
		}

		if ( ent->fl.noGuiInteraction ) {
			continue;
		}

		sdGuiSurface* guiSurface = NULL;
		sdUserInterfaceLocal* ui = NULL;

		for ( rvClientEntity* cent = ent->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
			guiSurface = cent->Cast< sdGuiSurface >();
			if ( guiSurface != NULL ) {
				ui = gameLocal.GetUserInterface( guiSurface->GetRenderable().GetGuiHandle() );
				if ( ui == NULL || !ui->IsInteractive() ) {
					continue;
				}

				pt = guiSurface->GetRenderable().Trace( start, end );

				if ( !( pt.x < 0.0f ) ) {
					break;
				}
			}
		}

		if ( ui != NULL && !( pt.x < 0.0f ) ) {
			// we have a hit
			ClearFocus();

			focusUI		= guiSurface->GetRenderable().GetGuiHandle();
			focusGUIent = ent;
			focusTime	= gameLocal.time + FOCUS_GUI_TIME;

			if ( gameLocal.IsLocalPlayer( this ) ) {
				ui->SetCursor( idMath::Ftoi( ( pt.x * SCREEN_WIDTH ) + 0.5 ), idMath::Ftoi( ( pt.y * SCREEN_HEIGHT ) + 0.5 ) );
			}

			break;
		}
	}

	if ( focusGUIent && focusUI ) {
		if ( !oldFocus || oldFocus != focusGUIent ) {
			if ( gameLocal.IsLocalPlayer( this ) ) {
				if( sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( focusUI )) {
					ui->Activate();
					ui->SetIgnoreLocalCursorUpdates( true );
				}
				focusGUIent->OnGuiActivated();
				StartSound( "snd_guienter", SND_ANY, 0, NULL );
			}
		}

	} else if ( oldFocus && oldUI ) {
		if ( gameLocal.IsLocalPlayer( this ) ) {
			oldUI->Deactivate();
			oldUI->SetIgnoreLocalCursorUpdates( false );

			oldFocus->OnGuiDeactivated();
			StartSound( "snd_guiexit" , SND_ANY, 0, NULL );
		}

		idWeapon* w = weapon;
		if ( w != NULL ) {
			w->Show();
		}
	}
}

/*
===============
idPlayer::BobCycle
===============
*/
void idPlayer::BobCycle( const idVec3 &pushVelocity ) {
	float		bobmove;
	int			old, deltaTime;
	idVec3		vel, gravityDir, velocity, origin;
	idMat3		viewaxis;
	float		bob;
	float		delta;
	float		speed;
	float		f;

	if ( physicsObj.IsFrozen() || GetProxyEntity() || playerFlags.noclip || remoteCamera.GetEntity() != NULL || gameLocal.IsPaused() ) {
		return;
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	velocity = physicsObj.GetLinearVelocity() - pushVelocity;

	gravityDir = physicsObj.GetGravityNormal();
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();

	// do not evaluate the bob for other clients
	// when doing a spectate follow, don't do any weapon bobbing
	if ( ( gameLocal.isClient && !gameLocal.IsLocalViewPlayer( this ) ) ) {
		viewBobAngles.Zero();
		viewBob.Zero();
		return;
	}

	if( pm_skipBob.GetBool() ) {
		viewBobAngles.Zero();
		viewBob.Zero();
		return;
	}

	if ( !physicsObj.HasGroundContacts() || IsSpectating() ) {
		// airborne
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else if ( ( !usercmd.forwardmove && !usercmd.rightmove ) || ( xyspeed <= MIN_BOB_SPEED ) ) {
		// start at beginning of cycle again
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else {
		if ( physicsObj.IsCrouching() ) {
			bobmove = pm_crouchbob.GetFloat();
			// ducked characters never play footsteps
		} else {
			// vary the bobbing based on the speed of the player
			bobmove = pm_walkbob.GetFloat() * ( 1.0f - bobFrac ) + pm_runbob.GetFloat() * bobFrac;
		}

		// check for footstep / splash sounds
		old = bobCycle;
		bobCycle = (int)( old + bobmove * gameLocal.msec ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( idMath::Sin( ( bobCycle & 127 ) / 127.f * idMath::PI ) );
	}

	// calculate angles for view bobbing
	viewBobAngles.Zero();

	viewAngles.ToMat3NoRoll( viewaxis );
	viewaxis *= physicsObj.GetGravityAxis();

	// add angles based on velocity
	delta = velocity * viewaxis[0];
	viewBobAngles.pitch += delta * pm_runpitch.GetFloat();

	delta = velocity * viewaxis[1];
	viewBobAngles.roll -= delta * pm_runroll.GetFloat();

	// add angles based on bob
	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;

	delta = bobfracsin * pm_bobpitch.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching
	}
	viewBobAngles.pitch += delta;
	delta = bobfracsin * pm_bobroll.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching accentuates roll
	}
	if ( bobFoot & 1 ) {
		delta = -delta;
	}
	viewBobAngles.roll += delta;

	// calculate position for view bobbing
	viewBob.Zero();

	if ( physicsObj.HasSteppedUp() ) {

		// check for stepping up before a previous step is completed
		deltaTime = gameLocal.time - stepUpTime;
		if ( deltaTime < STEPUP_TIME ) {
			stepUpDelta = stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME + physicsObj.GetStepUp();
		} else {
			stepUpDelta = physicsObj.GetStepUp();
		}
		if ( stepUpDelta > 2.0f * pm_stepsize.GetFloat() ) {
			stepUpDelta = 2.0f * pm_stepsize.GetFloat();
		}
		stepUpTime = gameLocal.time;
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// if the player stepped up recently
	deltaTime = gameLocal.time - stepUpTime;
	if ( deltaTime < STEPUP_TIME ) {
		viewBob += gravity * ( stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME );
	}

	// add bob height after any movement smoothing
	bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
	if ( bob > 6 ) {
		bob = 6;
	}
	viewBob[2] += bob;

	// add fall height
	delta = static_cast< float >( gameLocal.time - landTime );
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		viewBob -= gravity * ( landChange * f );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.f - ( delta / static_cast< float >( LAND_RETURN_TIME ) );
		viewBob -= gravity * ( landChange * f );
	}
}

/*
================
idPlayer::UpdateDeltaViewAngles
================
*/
void idPlayer::UpdateDeltaViewAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
	}

	SetDeltaViewAngles( delta );
}

/*
================
idPlayer::SetViewAngles
================
*/
void idPlayer::SetViewAngles( const idAngles &inAngles ) {
	idAngles angles = inAngles;
	angles.FixDenormals();

	UpdateDeltaViewAngles( angles );
	viewAngles = angles;
	clientViewAngles = angles;

	ANG_FIX_BAD( clientViewAngles );
}

/*
================
idPlayer::SetOrigin
================
*/
void idPlayer::SetOrigin( const idVec3 &org ) {
	GetPhysics()->SetOrigin( org );
	ResetPredictionErrorDecay();
	UpdateVisuals();
	Present();
}


/*
================
idPlayer::SetAxis
================
*/
void idPlayer::SetAxis( const idMat3 &axis ) {
	idAngles angles = axis.ToAngles();

	// roll is BAD on player
	angles.roll = 0.f;

	SetViewAngles( angles );
	idActor::SetAxis( angles.ToMat3() );

	ResetPredictionErrorDecay();
}

/*
=====================
idPlayer::SetPosition
=====================
*/
void idPlayer::SetPosition( const idVec3 &org, const idMat3 &axis ) {
	GetPhysics()->SetOrigin( org );
	viewAxis = axis;
	ResetPredictionErrorDecay();
	UpdateVisuals();
}

/*
================
idPlayer::UpdateViewAngles
================
*/
void idPlayer::UpdateViewAngles( void ) {
	int i;
	idAngles oldViewAngles = viewAngles;

	if ( InhibitTurning() ) {
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		UpdateDeltaViewAngles( viewAngles );
		return;
	}

	for ( i = 0; i < 3; i++ ) {
		cmdAngles[ i ] = SHORT2ANGLE( usercmd.angles[ i ] );
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy != NULL ) {
		proxy->GetUsableInterface()->UpdateViewAngles( this );
	} else {
		if ( remoteCamera.IsValid() ) {
			if ( sdScriptEntity* scriptEnt = remoteCamera.GetEntity()->Cast< sdScriptEntity >() ) {
				cameraViewAngles = scriptEnt->GetRemoteViewAngles( this );
			} else {
				for ( i = 0; i < 3; i++ ) {
					cameraViewAngles[ i ] = idMath::AngleNormalize180( cmdAngles[ i ] );
				}
			}
		} else {

			// circularly clamp the angles with deltas
			if ( !( usercmd.buttons.btn.mLookOff ) ) {
				for ( i = 0; i < 3; i++ ) {
					viewAngles[ i ] = idMath::AngleNormalize180( cmdAngles[ i ] + deltaViewAngles[ i ] );
				}
			} else {
				viewAngles.yaw = idMath::AngleNormalize180( cmdAngles[ YAW ] + deltaViewAngles[ YAW ] );
			}
		}
	}

	if ( proxy != NULL ) {
		proxy->GetUsableInterface()->ClampViewAngles( this, oldViewAngles );
	} else if ( remoteCamera.IsValid() ) {
		idVec2 bounds;
		bounds[ 0 ] = pm_minviewpitch.GetFloat();
		bounds[ 1 ] = pm_maxviewpitch.GetFloat();

		if ( IsFPSUnlock() ) {
			gameLocal.unlock.minAngles.pitch = bounds[0];
			gameLocal.unlock.maxAngles.pitch = bounds[1];
		}

		if ( cameraViewAngles.pitch > bounds[ 1 ] ) {
			cameraViewAngles.pitch = bounds[ 1 ];
		} else if ( cameraViewAngles.pitch < bounds[ 0 ] ) {
			cameraViewAngles.pitch = bounds[ 0 ];
		}
	} else {
		// if dead
		bool dead = ( team != NULL && ( IsDead() || IsInLimbo() ) );

		if ( !dead ) {
			idWeapon* weapon = GetWeapon();
			if ( weapon ) {
				weapon->ClampAngles( viewAngles, oldViewAngles );
				weapon->UpdateSpreadValue( physicsObj.GetLinearVelocity(), viewAngles, oldViewAngles );
			}
		}

		idVec2 bounds;
		if ( IsProne() ) {
			bounds[ 0 ] = pm_minproneviewpitch.GetFloat();
			bounds[ 1 ] = pm_maxproneviewpitch.GetFloat();
		} else {
			bounds[ 0 ] = pm_minviewpitch.GetFloat();
			bounds[ 1 ] = pm_maxviewpitch.GetFloat();
		}

		if ( IsFPSUnlock() ) {
			gameLocal.unlock.minAngles.pitch = bounds[0];
			gameLocal.unlock.maxAngles.pitch = bounds[1];
		}

		if ( viewAngles.pitch > bounds[ 1 ] ) {
			viewAngles.pitch = bounds[ 1 ];
		} else if ( viewAngles.pitch < bounds[ 0 ] ) {
			viewAngles.pitch = bounds[ 0 ];
		}

		if ( !dead ) {
			if ( physicsObj.IsProne() ) {
				if ( gameLocal.time < physicsObj.GetProneChangeEndTime() || physicsObj.ProneCheck( physicsObj.GetOrigin(), viewAngles ) & PR_FAILED ) {
					viewAngles.yaw = oldViewAngles.yaw;
				} else {
					sdVehiclePosition::ClampAngle( viewAngles, oldViewAngles, proneClampPitch, 0 );
					sdVehiclePosition::ClampAngle( viewAngles, oldViewAngles, proneClampYaw, 1 );
				}
			}
		}
	}

	if ( sdScriptEntity* scriptEnt = remoteCamera.GetEntity()->Cast< sdScriptEntity >() ) {
		scriptEnt->SetRemoteViewAngles( cameraViewAngles, this );
		UpdateDeltaViewAngles( oldViewAngles );
	} else {
		UpdateDeltaViewAngles( viewAngles );
	}

	if ( remoteCamera.IsValid() ) {
		return;
	}

	if ( !IsDead() ) {
		if ( physicsObj.OnLadder() ) {
			viewAxis = ( -physicsObj.GetLadderNormal() ).ToMat3();
		} else {
			// orient the model towards the direction we're looking
			idAngles::YawToMat3( viewAngles.yaw, viewAxis );
		}
	} else {
		// orient the model towards the direction we're looking
		idAngles::YawToMat3( baseDeathYaw, viewAxis );
	}

	if ( gameLocal.isNewFrame ) {
		idVec3 oldViewAxisOrientator = viewAxisOrientator;

		idVec3 upVec( 0.f, 0.f, 1.f );
		if ( physicsObj.OnLadder() || IsDead() ) {
			viewAxisOrientator = upVec;
		} else if ( physicsObj.IsProne() && physicsObj.IsGrounded() ) {
			viewAxisOrientator = Lerp( viewAxisOrientator, physicsObj.GetGroundNormal(), 0.05f );
			viewAxisOrientator.FixDenormals();
		} else {
			viewAxisOrientator = Lerp( viewAxisOrientator, upVec, 0.25f );
			viewAxisOrientator.FixDenormals();
		}

		if ( !oldViewAxisOrientator.Compare( viewAxisOrientator, 0.001f ) ) {
			viewAxisOrientator.NormalizeFast();

			// Gordon: Doubt this comes out ortogonal
			viewAxisOrientation.Identity();
			viewAxisOrientation[ 0 ] -= ( viewAxisOrientation[ 0 ] * viewAxisOrientator ) * viewAxisOrientator;
			viewAxisOrientation[ 0 ].NormalizeFast();
			viewAxisOrientation[ 1 ] -= ( viewAxisOrientation[ 1 ] * viewAxisOrientator ) * viewAxisOrientator;
			viewAxisOrientation[ 1 ].NormalizeFast();
			viewAxisOrientation[ 2 ].Cross( viewAxisOrientation[ 0 ], viewAxisOrientation[ 1 ] );
		}
	}

	viewAxis *= viewAxisOrientation;
}

/*
==============
idPlayer::Spectate
==============
*/
void idPlayer::Spectate( void ) {
	if ( !gameLocal.isClient ) {
		idEntity* proxy = GetProxyEntity();
		if ( proxy ) {
			proxy->GetUsableInterface()->OnExit( this, true );
		}
	}

	GetInventory().ClearClass();

	// join the spectators
	SetSpectateClient( this );
	if ( !gameLocal.isClient ) {
		idVec3		spawnOrigin;
		idAngles	spawnAngles;

		gameLocal.SelectInitialSpawnPoint( this, spawnOrigin, spawnAngles );
		spawnOrigin[ 2 ] += pm_normalviewheight.GetFloat();
		spawnOrigin[ 2 ] += SPECTATE_RAISE;

		SetOrigin( spawnOrigin );
		SetViewAngles( spawnAngles );
	}

	Init( false );
	SetPhysics( &physicsObj );
	if ( physicsObj.GetClipModel() ) {
		physicsObj.UnlinkClip();
	}
	physicsObj.SetupPlayerClipModels();
	UpdateVisibility();
}

/*
==============
idPlayer::SetVehicleCameraMode
==============
*/
void idPlayer::SetVehicleCameraMode( int viewMode, bool resetAngles, bool force ) {
	if ( !GetProxyEntity() ) {
		return;
	}

	SetProxyViewMode( viewMode, force );
	if ( resetAngles ) {
		viewAngles.yaw = 0.0f;
		clientViewAngles.yaw = 0.0f;
		UpdateDeltaViewAngles( viewAngles );
	}
}

/*
==============
idPlayer::SwapVehiclePosition
==============
*/
void idPlayer::SwapVehiclePosition( void ) {
	if ( IsSpectating() || gameLocal.isClient ) {
		return;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		proxy->GetUsableInterface()->SwapPosition( this );
	}
}

/*
==============
idPlayer::CycleVehicleCameraMode
==============
*/
void idPlayer::CycleVehicleCameraMode( void ) {
	if ( IsSpectating() || gameLocal.isClient ) {
		return;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		proxy->GetUsableInterface()->SwapViewMode( this );
	}
}

/*
==============
idPlayer::DoWeapNext
==============
*/
void idPlayer::DoWeapNext() {
	idWeapon* weapon = GetWeapon();
	if ( weapon && weapon->OnWeapNext() ) {
		return;
	}

	NextWeapon();

}

/*
==============
idPlayer::DoWeapPrev
==============
*/
void idPlayer::DoWeapPrev() {
	idWeapon* weapon = GetWeapon();
	if ( weapon && weapon->OnWeapPrev() ) {
		return;
	}

	PrevWeapon();
}

/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse ) {
	if ( gameLocal.isClient && entityNumber == gameLocal.localClientNum ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_IMPULSE );
		outMsg.WriteBits( impulse, idMath::BitsForInteger( UCI_NUM_IMPULSES ) );
		outMsg.Send();
	}

	if ( impulse >= UCI_WEAP0 && impulse <= UCI_WEAP12 ) {
		sdUsableInterface* iface = NULL;

		idEntity* proxy = GetProxyEntity();
		if ( proxy ) {
			iface = proxy->GetUsableInterface();
		}

		if ( iface && !iface->GetAllowPlayerWeapon( this ) ) {
			iface->SelectWeapon( this, impulse - UCI_WEAP0 );
		} else {
			if ( gameLocal.IsLocalPlayer( this ) ) {
				SelectWeapon( impulse, false );
			}
		}
		return;
	}

	switch( impulse ) {
		case UCI_RELOAD: {
			Reload();
			break;
		}
		case UCI_WEAPNEXT: {
			DoWeapNext();
			break;
		}
		case UCI_WEAPPREV: {
			DoWeapPrev();
			break;
		}
		case UCI_VEHICLE_CAMERA_MODE: {
			if ( !gameLocal.IsPaused() ) {
				CycleVehicleCameraMode();
			}
			break;
		}
		case UCI_USE_VEHICLE: {
			if ( !gameLocal.isClient && !gameLocal.IsPaused() ) {
				ServerUseObject();
			}
			break;
		}
		case UCI_STROYUP: {
			if ( !gameLocal.isClient && !gameLocal.IsPaused() ) {
				ConsumeHealth();
			}
			break;
		}
		case UCI_STROYDOWN: {
			if ( !gameLocal.isClient && !gameLocal.IsPaused() ) {
				ConsumeStroyent();
			}
			break;
		}
		case UCI_PRONE: {
			if ( !InhibitMovement() && !IsSpectating() && ( GetProxyEntity() == NULL ) && !gameLocal.IsPaused() ) {
				if ( !physicsObj.TryProne() ) {
					PlayProneFailedToolTip();
				}
			}
			break;
		}
		case UCI_READY: {
			if ( !gameLocal.isClient ) {
				if ( team != NULL ) {
					if ( IsReady() ) {
						SetReady( false, false );
					} else {
						SetReady( true, false );
					}
				}
			}
			break;
		}
	}
}

/*
==============
idPlayer::EvaluateControls
==============
*/
void idPlayer::EvaluateControls( const usercmd_t& oldCmd ) {
	if ( !gameLocal.isClient ) {
		if ( !IsSpectator() ) {
			if ( !IsInLimbo() ) {
				if ( health < LIMBO_FORCE_HEALTH ) {
					ServerForceRespawn( false );
				} else {
					if ( IsDead() ) {
						if ( !team->AllowRevive() || physicsObj.GetWaterLevel() >= WATERLEVEL_HEAD ) {
							ServerForceRespawn( false );
						} else {
							if ( usercmd.upmove > 0 ) {
								playerFlags.wantSpawn = true;
							} else if ( usercmd.upmove < 0 ) {
								playerFlags.wantSpawn = false;
							}
						}
					}
				}
			}
		}
	}

	if ( gameLocal.isNewFrame && gameLocal.IsLocalPlayer( this ) ) {
		if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
			PerformImpulse( usercmd.impulse );
		}
	}

	oldFlags = usercmd.flags;

	AdjustSpeed();

	// update the viewangles
	UpdateViewAngles();

	viewAngles.FixDenormals();
	ANG_FIX_BAD( viewAngles );

	// update the client viewangles
	if ( gameLocal.isClient && !gameLocal.IsLocalPlayer( this ) ) {
		idMat3 tempAxis = viewAngles.ToMat3();
		tempAxis.FixDenormals();
		MAT_FIX_BAD( tempAxis );

		predictionErrorDecay_Angles.Decay( tempAxis );
		tempAxis.FixDenormals();
		MAT_FIX_BAD( tempAxis );

		idQuat oldAxis = clientViewAngles.ToQuat();
		idQuat newAxis = tempAxis.ToQuat();
		oldAxis.FixDenormals();
		newAxis.FixDenormals();
		QUAT_FIX_BAD( oldAxis );
		QUAT_FIX_BAD( newAxis );

		idQuat resultQuat;
		resultQuat.Slerp( oldAxis, newAxis, 0.6f );
		resultQuat.FixDenormals();
		QUAT_FIX_BAD( resultQuat );

		clientViewAngles = resultQuat.ToAngles();
		clientViewAngles.FixDenormals();
		ANG_FIX_BAD( clientViewAngles );
	} else {
		clientViewAngles = viewAngles;
		clientViewAngles.FixDenormals();
		ANG_FIX_BAD( clientViewAngles );
	}
}


/*
==============
idPlayer::InhibitSprint
==============
*/
bool idPlayer::InhibitSprint( void ) const {
	if ( playerFlags.noclip ) {
		return false;
	}

	if ( physicsObj.IsProne() || physicsObj.IsCrouching() ) {
		return true;
	}

	if ( GetProxyEntity() != NULL ) {
		return true;
	}

	if ( usercmd.forwardmove <= 0 ) {
		return true;
	}

	if ( playerFlags.sprintDisabled || playerFlags.runDisabled ) {
		return true;
	}

	if ( physicsObj.OnLadder() ) {
		return true;
	}

	if ( physicsObj.GetWaterLevel() >= WATERLEVEL_WAIST ) {
		return true;
	}

	return false;
}

/*
==============
idPlayer::AdjustSpeed
==============
*/
void idPlayer::AdjustSpeed( void ) {
	float speedFwd;
	float speedBack;
	float speedSide;

	bool crouched = false;
	bool run = false;
	bool sprint = false;
	bool moving = usercmd.forwardmove || usercmd.rightmove;

	crouched = physicsObj.IsCrouching();

	if ( ( gameLocal.time - lastReviveTime ) > SEC2MS( 2.f ) ) {
		if ( !InhibitSprint() ) {
			sprint = usercmd.buttons.btn.sprint;
		}
		if ( !sprint ) {
			run = !playerFlags.runDisabled && ( usercmd.buttons.btn.run || usercmd.buttons.btn.sprint );
		}
	}

	float crouchSpeed	= pm_crouchspeed.GetFloat();
	float proneSpeed	= pm_pronespeed.GetFloat();

	if ( InhibitMovement() ) {
		crouchSpeed	= 0.f;
		speedFwd = speedBack = speedSide = 0.f;
		proneSpeed	= 0.f;
		bobFrac		= 0.f;
	} else if ( IsSpectating() ) {
		if ( sprint ) {
			speedFwd = speedBack = speedSide = pm_spectatespeedsprint.GetFloat();
		} else if ( run ) {
			speedFwd = speedBack = speedSide = pm_spectatespeed.GetFloat();
		} else {
			speedFwd = speedBack = speedSide = pm_spectatespeedwalk.GetFloat();
		}
		bobFrac		= 0.f;
	} else if ( playerFlags.noclip ) {
		if ( sprint ) {
			speedFwd = speedBack = speedSide = pm_noclipspeedsprint.GetFloat();
		} else if ( run ) {
			speedFwd = speedBack = speedSide = pm_noclipspeed.GetFloat();
		} else {
			speedFwd = speedBack = speedSide = pm_noclipspeedwalk.GetFloat();
		}
		bobFrac		= 0.f;
	} else if( sprint ) {
		speedFwd	= pm_sprintspeedforward.GetFloat() * sprintScale;
		speedBack	= 0.0f;
		speedSide	= pm_sprintspeedstrafe.GetFloat() * sprintScale;
		bobFrac		= 1.f;
	} else if( run || physicsObj.IsCrouching() || physicsObj.IsProne() ) {
		speedFwd	= pm_runspeedforward.GetFloat();
		speedBack	= pm_runspeedback.GetFloat();
		speedSide	= pm_runspeedstrafe.GetFloat();
		bobFrac		= 1.f;
	} else {
		speedFwd = speedBack = speedSide = pm_walkspeed.GetFloat();
		bobFrac		= 0.f;
	}

	AI_SPRINT		= sprint;
	AI_LEAN			= physicsObj.GetLeanOffset();

	if ( !playerFlags.noclip ) {
		speedFwd	*= speedModifier;
		speedBack	*= speedModifier;
		speedSide	*= speedModifier;
		crouchSpeed	*= speedModifier;
		proneSpeed	*= 1.f;
	}

	physicsObj.SetSpeed( speedFwd, speedBack, speedSide, crouchSpeed, proneSpeed );
}

idCVar anim_minBodyPitch( "anim_minBodyPitch", "-40", CVAR_GAME | CVAR_FLOAT, "min pitch of body adjustment" );
idCVar anim_maxBodyPitch( "anim_maxBodyPitch", "10", CVAR_GAME | CVAR_FLOAT, "max pitch of body adjustment" );

/*
==============
idPlayer::AdjustBodyAngles
==============
*/
// Gordon: FIXME: Need to smooth out the pitch changes for non local clients
void idPlayer::AdjustBodyAngles( void ) {
	if ( IsDead() || IsInLimbo() || remoteCamera.GetEntity() != NULL || IsSpectator() || gameLocal.IsPaused() ) {
		return;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy != NULL ) {
		if ( !proxy->GetUsableInterface()->GetAllowAdjustBodyAngles( this ) ) {
			return;
		}
	}

	idMat3	legsAxis;
	bool	blend;
	float	diff;

	blend = true;

	float minYaw = -45.0f;
	float maxYaw = 45.0f;

	idAnimBlend* animBlend = animator.CurrentAnim( ANIMCHANNEL_TORSO );
	idAnimBlend* animBlendLegs = animator.CurrentAnim( ANIMCHANNEL_LEGS );

	if ( proxy != NULL || OnLadder() ) {
		idealLegsYaw = 0.0f;
		playerFlags.legsForward = true;
		blend = false;
	} else if ( animBlend && animator.GetAnimFlags( animBlend->AnimNum() ).ai_fixed_forward ||
		 animBlendLegs && animator.GetAnimFlags( animBlendLegs->AnimNum() ).ai_fixed_forward ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( static_cast< float >( usercmd.forwardmove ), static_cast< float >( -usercmd.rightmove ), 0.0f ).ToYaw() );
		playerFlags.legsForward = true;
	} else if ( !physicsObj.HasGroundContacts() || InhibitMovement() ) {
		idealLegsYaw = 0.0f;
		playerFlags.legsForward = true;
	} else if ( usercmd.forwardmove < 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( static_cast< float >( -usercmd.forwardmove ), static_cast< float >( usercmd.rightmove ), 0.0f ).ToYaw() );
		playerFlags.legsForward = false;
	} else if ( usercmd.forwardmove > 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( static_cast< float >( usercmd.forwardmove ), static_cast< float >( -usercmd.rightmove ), 0.0f ).ToYaw() );
		playerFlags.legsForward = true;
	} else if ( ( usercmd.rightmove != 0 ) && physicsObj.IsCrouching() ) {
		if ( !playerFlags.legsForward ) {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( static_cast< float >( idMath::Abs( usercmd.rightmove ) ), static_cast< float >( usercmd.rightmove ), 0.0f ).ToYaw() );
		} else {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( static_cast< float >( idMath::Abs( usercmd.rightmove ) ), static_cast< float >( -usercmd.rightmove ), 0.0f ).ToYaw() );
		}
	} else if ( usercmd.rightmove != 0 ) {
		idealLegsYaw = 0.0f;
		playerFlags.legsForward = true;
	} else {
		playerFlags.legsForward = true;
		diff = idMath::Fabs( idealLegsYaw - legsYaw );
		idealLegsYaw = idealLegsYaw - idMath::AngleNormalize180( clientViewAngles.yaw - oldViewYaw );
		if ( diff < 0.1f ) {
			legsYaw = idealLegsYaw;
			blend = false;
		}
	}

	if ( !physicsObj.IsCrouching() ) {
		playerFlags.legsForward = true;
	}

	oldViewYaw = clientViewAngles.yaw;

	AI_TURN_LEFT = false;
	AI_TURN_RIGHT = false;
	if ( idealLegsYaw < minYaw ) {
		idealLegsYaw = 0;
		AI_TURN_RIGHT = true;
		blend = true;
	} else if ( idealLegsYaw > maxYaw ) {
		idealLegsYaw = 0;
		AI_TURN_LEFT = true;
		blend = true;
	}

	if ( blend ) {
		legsYaw = legsYaw * 0.9f + idealLegsYaw * 0.1f;
		if ( idMath::Fabs( legsYaw ) < idMath::FLT_EPSILON ) {
			legsYaw = 0.0f;
		}
	}

	idAngles::YawToMat3( legsYaw, legsAxis );
	animator.SetJointAxis( hipJoint, JOINTMOD_WORLD, legsAxis );

	float clampedPitch = idMath::ClampFloat( anim_minBodyPitch.GetFloat(), anim_maxBodyPitch.GetFloat(), clientViewAngles.pitch );
	if ( ( animBlend && animator.GetAnimFlags( animBlend->AnimNum() ).no_pitch ) || AI_PRONE || proxy != NULL || OnLadder() ) {
		clampedPitch = 0.f;
	}

	float torsoYaw = 0.f;
	if ( proxy != NULL && !OnLadder() ) {
		torsoYaw = clientViewAngles.yaw;
	}

	// torso
	idAngles angles( clampedPitch, torsoYaw, 0.0f );
	animator.SetJointAxis( torsoJoint, JOINTMOD_WORLD, angles.ToMat3() );

	// head
	angles.Set( clampedPitch / 4, -legsYaw / 4, 0.0f );
	animator.SetJointAxis( headJoint, JOINTMOD_WORLD, angles.ToMat3() );
}

/*
==============
idPlayer::GetEyeOffset
==============
*/
idVec3 idPlayer::GetEyeOffset( eyePos_t pos ) {
	switch ( pos ) {
		case EP_SPECTATOR:
		case EP_PROXY:
			return vec3_zero;

		case EP_DEAD:
			return idVec3( 0.f, 0.f, pm_deadviewheight.GetFloat() );
		case EP_CROUCH:
			return idVec3( 0.f, 0.f, pm_crouchviewheight.GetFloat() );
		case EP_NORMAL:
			return idVec3( 0.f, 0.f, pm_normalviewheight.GetFloat() );
		case EP_PRONE:
			return idVec3( pm_proneviewdistance.GetFloat(), 0.f, pm_proneviewheight.GetFloat() );
	}

	return vec3_zero;
}

/*
==============
idPlayer::GetEyeChangeRate
==============
*/
float idPlayer::GetEyeChangeRate( eyePos_t pos ) {
	switch ( pos ) {
		case EP_PROXY:
			return 0.f;

		case EP_SPECTATOR:
		case EP_DEAD:
		case EP_CROUCH:
		case EP_NORMAL:
		case EP_PRONE:
			return pm_crouchrate.GetFloat();
	}

	return 0.f;
}

/*
==============
idPlayer::Move
==============
*/

idCVar ai_fallTime( "ai_fallTime", "0.25", CVAR_FLOAT, "Number of seconds before the player plays the falling animation" );

void idPlayer::Move( const usercmd_t &usercmd, const idAngles &viewAngles ) {
	if ( IsBeingBriefed() ) {
		return;
	}

	// save old origin and velocity for crashlanding
	idVec3 oldOrigin	= physicsObj.GetOrigin();
	idVec3 oldVelocity	= physicsObj.GetLinearVelocity();
	idVec3 pushVelocity = physicsObj.GetPushedLinearVelocity();

	// set physics variables
	physicsObj.SetMaxStepHeight( pm_stepsize.GetFloat() );
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );

	if ( playerFlags.noclip ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetContents( 0, 1 );
		physicsObj.SetMovementType( PM_NOCLIP );
	} else if ( IsSpectating() ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetContents( 0, 1 );

		if ( team ) {
			physicsObj.SetMovementType( PM_FREEZE );
		} else {
			physicsObj.SetMovementType( PM_SPECTATOR );
		}
	} else if ( IsDead() ) {
		physicsObj.SetContents( CONTENTS_CORPSE );
		physicsObj.SetContents( 0, 1 );
		physicsObj.SetMovementType( PM_DEAD );
	} else {
		physicsObj.SetContents( g_defaultPlayerContents );
		physicsObj.SetContents( CONTENTS_RENDERMODEL, 1 );
		physicsObj.SetMovementType( PM_NORMAL );
	}

	if ( IsSpectating() ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else if ( health <= 0 ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else {
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}

	physicsObj.SetPlayerInput( usercmd, viewAngles, !InhibitMovement() );

	RunPhysics();

	if ( gameLocal.isNewFrame ) {
		eyePos_t newEyePos;

		if ( IsSpectator() ) {
			newEyePos = EP_SPECTATOR;
		} else if ( health <= 0 ) {
			newEyePos = EP_DEAD;
		} else if( physicsObj.IsCrouching() ) {
			newEyePos = EP_CROUCH;
		} else if( physicsObj.IsProne() ) {
			newEyePos = EP_PRONE;
		} else if( GetProxyEntity() ) {
			newEyePos = EP_PROXY;
		} else {
			newEyePos = EP_NORMAL;
		}

		if ( newEyePos != eyePosition ) {
			if ( newEyePos == EP_PRONE ) {
				eyePosChangeTime		= gameLocal.time;
				eyePosChangeStart		= eyeOffset;
				if ( eyePosition == EP_CROUCH ) {
					eyePosChangeDuration = physicsObj.GetProneTime( idPhysics_Player::PT_CROUCH_TO_PRONE );
				} else {
					eyePosChangeDuration = physicsObj.GetProneTime( idPhysics_Player::PT_STAND_TO_PRONE );
				}
			} else {
				idVec3 offset = GetEyeOffset( newEyePos );
				float changeRate = GetEyeChangeRate( newEyePos );
				if ( changeRate == 0.f ) {
					eyeOffset = offset;
				} else {
					eyePosChangeTime		= gameLocal.time;
					eyePosChangeStart		= eyeOffset;
					eyePosChangeDuration	= SEC2MS( idMath::Fabs( offset.z - eyePosChangeStart.z ) / changeRate );

					if ( eyePosChangeDuration == 0 ) {
						eyePosChangeTime = -1;
					}
				}
			}

			oldEyePosition	= eyePosition;
			eyePosition		= newEyePos;
		}

		if ( eyePosChangeTime != -1 ) {
			float elapsed = ( gameLocal.time - eyePosChangeTime ) / ( float )eyePosChangeDuration;
			elapsed = Min( elapsed, 1.f );

			idVec3 newValue = GetEyeOffset( eyePosition );

			if ( eyePosition == EP_PRONE ) {
				const char* tableName = spawnArgs.GetString( oldEyePosition == EP_CROUCH ? "prone_eye_from_crouch" : "prone_eye_from_standing" );
				const idDeclTable* table = gameLocal.declTableType[ tableName ];
				if ( !table ) {
					elapsed = 1.f;
				} else {
					elapsed = table->TableLookup( elapsed );
				}
			}
			eyeOffset = Lerp( eyePosChangeStart, newValue, elapsed );

			if ( gameLocal.time > ( eyePosChangeTime + eyePosChangeDuration ) ) {
				eyePosChangeTime = -1;
			}
		}
	}

	if ( playerFlags.noclip ) {
		AI_CROUCH	= false;
		AI_ONGROUND	= false;
		AI_ONLADDER	= false;
		AI_JUMP		= false;
		AI_PRONE	= false;
		AI_INWATER	= false;
	} else {

		if( physicsObj.HasGroundContacts() ) {
			lastGroundContactTime = gameLocal.time;
			AI_ONGROUND	= true;
		} else {
			AI_ONGROUND	= ( gameLocal.time - lastGroundContactTime ) < SEC2MS( ai_fallTime.GetFloat() ) && ( physicsObj.GetLinearVelocity().z <= 0.f );
		}

		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();
		AI_PRONE	= physicsObj.IsProne();
	}

	if ( gameLocal.isNewFrame ) {
		if ( AI_ONLADDER ) {
			int oldRung = static_cast< int >( oldOrigin.z / LADDER_RUNG_DISTANCE );
			int newRung = static_cast< int >( physicsObj.GetOrigin().z / LADDER_RUNG_DISTANCE );

			if ( oldRung != newRung ) {
				StartSound( "snd_stepladder", SND_ANY, 0, NULL );
			}
		}
		BobCycle( pushVelocity );
	}
}

/*
==============
idPlayer::UpdatePlayZoneInfo
==============
*/
void idPlayer::UpdatePlayZoneInfo( void ) {
	const sdPlayZone* playZone = NULL;

	// For testing path issues
/*	if ( gameLocal.IsLocalPlayer( this ) ) {
		playZone = gameLocal.GetPlayZone( physicsObj.GetOrigin(), sdPlayZone::PZF_PATH );
		if ( playZone != NULL ) {
			sdVehiclePathGrid* path = playZone->GetPath( "vehicle_magog_npc" );
			if ( path != NULL ) {
				path->DebugDraw();
			}
		}
	}*/

	playZone = gameLocal.GetPlayZone( physicsObj.GetOrigin(), sdPlayZone::PZF_PLAYZONE );
	if ( playZone != NULL ) {
		const sdDeployMaskInstance* deployMask = playZone->GetMask( gameLocal.GetPlayZoneMask() );
		if ( deployMask == NULL || deployMask->IsValid( physicsObj.GetAbsBounds() ) == DR_CLEAR ) {
			lastTimeInPlayZone	= gameLocal.time;
			playerFlags.inPlayZone = true;
			return;
		}
	}

	playerFlags.inPlayZone = false;

	if ( IsSpectator() ) {
		float dist = 0.f;
		const sdPlayZone* pz = gameLocal.ClosestPlayZone( physicsObj.GetOrigin(), dist, sdPlayZone::PZF_PLAYZONE );
		if ( pz != NULL ) {
			idVec3 push( 0.f, 0.f, 0.f );

			int side = pz->GetBounds().SideForPoint( physicsObj.GetOrigin().ToVec2(), sdBounds2D::SPACE_INCREASE_FROM_TOP );
			if ( side & sdBounds2D::SIDE_LEFT ) {
				push.x = 1.f;
			} else if ( side & sdBounds2D::SIDE_RIGHT ) {
				push.x = -1.f;
			}

			if ( side & sdBounds2D::SIDE_TOP ) {
				push.y = 1.f;
			} else if ( side & sdBounds2D::SIDE_BOTTOM ) {
				push.y = -1.f;
			}

			push.NormalizeFast();

			float scale = dist / 1024.f;
			if ( scale > 1.f ) {
				scale = 1.f;
			}

			push *= 60000.f * scale;

			physicsObj.ApplyImpulse( 0, physicsObj.GetOrigin(), push );
		}
		return;
	}


	if ( !GetProxyEntity() && lastTimeInPlayZone != -1 ) {
		if ( health > 0 ) {
			SpawnToolTip( team->GetOOBToolTip() );
		}

		if ( gameLocal.isClient ) {
			return;
		}

		if ( gameLocal.time - lastTimeInPlayZone >= oobDamageInterval ) {
			lastTimeInPlayZone = gameLocal.time;

			float dist = 0.f;
			gameLocal.ClosestPlayZone( physicsObj.GetOrigin(), dist, sdPlayZone::PZF_PLAYZONE );

			const float step = 512.f;
			idStr damageDeclName;

			if ( dist <= ( step * 1 ) ) {
				damageDeclName = "damage_oob_warning";
			} else if ( dist <= ( step * 2 ) ) {
				damageDeclName = "damage_oob_1st";
			} else if ( dist <= ( step * 3 ) ) {
				damageDeclName = "damage_oob_2nd";
			} else if ( dist <= ( step * 4 ) ) {
				damageDeclName = "damage_oob_3rd";
			} else {
				damageDeclName = "damage_oob_4th";
			}

			const sdDeclDamage* damageDecl = gameLocal.declDamageType[ damageDeclName.c_str() ];
			if ( damageDecl == NULL ) {
				gameLocal.Warning( "idPlayer::UpdatePlayZoneInfo: couldn't find damage decl %s", damageDeclName.c_str() );
				return;
			}

			Damage( NULL, NULL, idVec3( 0.f, 0.f, 1.f ), damageDecl, 1.f, NULL );
		}
	}
}

/*
==============
idPlayer::UpdateShadows
==============
*/
void idPlayer::UpdateShadows( void ) {
	viewState_t state = HasShadow();

	switch ( state ) {
		case VS_NONE: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= true;
			break;
		}
		case VS_REMOTE: {
			renderEntity.suppressShadowInViewID = entityNumber + 1;
			renderEntity.flags.noShadow			= false;
			break;
		}
		case VS_FULL: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= false;
			break;
		}
	}

	idWeapon* weapon = GetWeapon();
	if ( weapon ) {
		weapon->UpdateShadows();
	}
	GetInventory().GetItemPool().UpdateModelShadows();
}

/*
==============
idPlayer::HasShadow
==============
*/
viewState_t idPlayer::HasShadow( void ) {
	if ( IsSpectator() ) {
		return VS_NONE;
	}

	if ( playerFlags.noShadow ) {
		return VS_NONE;
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		sdUsableInterface* iface = proxy->GetUsableInterface();
		if ( !iface ) {
			gameLocal.Warning( "idPlayer::HasShadow Proxy with Missing Interface '%s'", proxy->GetName() );
		} else {
			if ( iface->GetShowPlayerShadow( this ) ) {
				return VS_FULL;
			}
		}
	}

	if ( !g_showPlayerShadow.GetBool() ) {
		return VS_NONE;
	}


	if ( gameLocal.IsLocalViewPlayer( this ) ) {
		return VS_REMOTE;
	}

	return VS_FULL;
}

idCVar g_debugLocationalDamage( "g_debugLocationalDamage", "0", CVAR_GAME | CVAR_BOOL, "shows locational damage information on the player" );
idCVar g_debugLocationalDamageStartAngle( "g_debugLocationalDamageStartAngle", "0", CVAR_GAME | CVAR_FLOAT, "" );
idCVar g_debugLocationalDamageFinishAngle( "g_debugLocationalDamageFinishAngle", "360", CVAR_GAME | CVAR_FLOAT, "" );

/*
==============
idPlayer::Think

Called every tic for each player
==============
*/
void idPlayer::Think( void ) {
	bool isLocalPlayer = gameLocal.IsLocalPlayer( this );
	fl.allowPredictionErrorDecay = true;

	if ( gameLocal.isServer ) {
		if ( !IsSpectator() ) {
			int limit = MINS2MS( g_timeoutToSpec.GetFloat() );
			if ( limit != 0 ) {
				int offset = networkSystem->ServerGetClientTimeSinceLastInput( entityNumber );
				if ( offset > limit ) {
					SetWantSpectate( true );
				}
			}
		}
	}

	UpdatePlayerIcons();

	// latch button actions
	oldFlags = usercmd.flags;
	oldButtons = usercmd.buttons;

	// grab out usercmd
	usercmd_t oldCmd = usercmd;
	usercmd = gameLocal.usercmds[ entityNumber ];

	if ( com_timeServer.GetInteger() > 0 && isLocalPlayer ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
		usercmd.angles[0] = usercmd.angles[1] = usercmd.angles[2];
	}

	// if this is the very first frame of the map, set the delta view angles
	// based on the usercmd angles
	if ( !playerFlags.spawnAnglesSet && ( gameLocal.GameState() != GAMESTATE_STARTUP ) ) {
		playerFlags.spawnAnglesSet = true;
		if ( !gameLocal.isClient ) {
			SetViewAngles( spawnAngles );
			oldFlags = usercmd.flags;
		}
	}

	if ( sendingVoice != IsSendingVoice() ) {
		sendingVoice = IsSendingVoice();

		sdScriptHelper h1;
		h1.Push( sendingVoice );
		scriptObject->CallNonBlockingScriptEvent( onSendingVoice, h1 );
	}

	EvaluateControls( oldCmd );

	if ( gameLocal.isNewFrame ) {
		AdjustBodyAngles();
	}

	idEntity* proxy = GetProxyEntity();
	idEntity* master = GetTeamMaster();
	if ( !master || master == this ) {
		bool allowMove = true;
		if ( gameLocal.isClient ) {
			allowMove = !playerFlags.isLagged;
		}
		if ( allowMove ) {
			if ( teleportEntity.IsValid() ) {
				allowMove = false;
			} else if ( proxy ) {
				sdUsableInterface* iface = GetProxyEntity()->GetUsableInterface();
				allowMove = iface->GetAllowPlayerMove( this );
			}
		}
		if ( allowMove && !aasPullPlayer ) {

			// don't allow client to move when lagged,
			// or when piloting a vehicle that doesn't let you (eg flyer hive)
			Move( usercmd, viewAngles );

			if ( gameLocal.isNewFrame ) {
				if ( !physicsObj.HasGroundContacts() && usercmd.upmove > 0 ) {
					sdScriptHelper h1;
					CallNonBlockingScriptEvent( onJumpFunction, h1 );
				}
			}
		}

		if ( gameLocal.isNewFrame ) {
			if ( !playerFlags.noclip ) {
				if ( !gameLocal.isClient || gameLocal.IsLocalViewPlayer( this ) ) {
					TouchTriggers();
				}
			}
		}
	} else {
		bool allowDamage = false;

		if ( proxy ) {
			sdUsableInterface* iface = GetProxyEntity()->GetUsableInterface();
			allowDamage = iface->GetAllowPlayerDamage( this );
		}

		if ( allowDamage ) {
			physicsObj.SetContents( g_defaultPlayerContents );
			physicsObj.SetContents( CONTENTS_RENDERMODEL, 1 );
			if ( gameLocal.isNewFrame ) {
				if ( !gameLocal.isClient || gameLocal.IsLocalViewPlayer( this ) ) {
					TouchTriggers();
				}
			}
		} else {
			physicsObj.SetContents( 0 );
			physicsObj.SetContents( 0, 1 );
		}
	}

	UpdatePlayZoneInfo();

	// Gordon: Disabling for now, we don't have any interactive in game guis, and this needs to be reworked to be faster
//	UpdateFocus();

	if ( gameLocal.isNewFrame ) {
		clientButtonsUsed = true;

		GetInventory().UpdateItems();

		// update player script
		UpdateScript();

		// service animations
		if ( !IsSpectating() ) {
			if ( !gameLocal.isClient || !( aorFlags & AOR_INHIBIT_ANIMATION ) ) {
				UpdateConditions();
				UpdateAnimState();
			}
		}

		if ( !proxy ) {
			CalculateView();
		}

		if ( gameLocal.IsLocalViewPlayer( this ) || gameLocal.isServer ) {
			if ( ( gameLocal.time - lastCrosshairTraceTime ) > 150 ) {
				GetCrosshairInfo( true );
				UpdateTargetLock();

				lastCrosshairTraceTime = gameLocal.time;
			} else {
				GetCrosshairInfo( false );
			}
		}

		if ( isLocalPlayer || gameLocal.isServer ) {
			sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
			if ( isLocalPlayer && module && module->Active() ) {
				module->HandleInput();
			} else {
				if ( !gameLocal.IsPaused() ) {
					DoActivate();
				}
			}
		}
	}

	if ( IsSpectating() ) {
		if ( !gameLocal.isClient ) {
			UpdateSpectating( oldCmd );
		}
		idWeapon* w = weapon;
		if ( w != NULL ) {
			if ( w->IsLinked() ) {
				w->Clear();
			}
			w->UpdateVisibility();
		}
	} else if ( weapon.IsValid() ) {
		UpdateWeapon();
	} else {
		if( weaponSwitchTimeout && ( IsDead() || NeedsRevive() ) ) {
			GetInventory().CancelWeaponSwitch();
			ShowWeaponMenu( sdWeaponSelectionMenu::AT_DISABLED );
		}
	}

	if ( gameLocal.isNewFrame ) {
		// never cast shadows from our first-person muzzle flashes
		renderEntity.noSelfShadowInViewID = entityNumber + 1;

		UpdateAnimation();

		UpdateVisibility();

#ifdef PLAYER_DAMAGE_LOG
		UpdateDamageLog();
#endif // PLAYER_DAMAGE_LOG

		if ( !proxy ) {
			if ( predictionErrorDecay_Origin.NeedsUpdate() || predictionErrorDecay_Angles.NeedsUpdate() ) {
				UpdateVisuals();
			}

			Present();
		}

		if ( isLocalPlayer ) {
			UpdateToolTips();

			if ( g_showPlayerSpeed.GetBool() ) {
				float ups = physicsObj.GetLinearVelocity().Length();
				if ( ups > 0.0f ) {
					float mps = InchesToMetres( ups );
					float mph = UPSToMPH( ups );
					float kph = UPSToKPH( ups );

					gameLocal.Printf( "Player Speed: %.02f %.02f %.02f (%.02fups %.02f mph %.02fmps %.02fkph)\n",
						physicsObj.GetLinearVelocity().x,
						physicsObj.GetLinearVelocity().y,
						physicsObj.GetLinearVelocity().z,
						ups, mph, mps, kph );

					idVec3 hVel = physicsObj.GetLinearVelocity();
					hVel.z = 0.0f;

					ups = hVel.Length();
					mps = InchesToMetres( ups );
					mph = UPSToMPH( ups );
					kph = UPSToKPH( ups );

					gameLocal.Printf( "Horizontal only: %.02fups %.02f mph %.02fmps %.02fkph\n",
						ups, mph, mps, kph );
				}
			}
		}

#if 0
		if ( !gameLocal.IsLocalViewPlayer( this ) ) {
			if ( proxy == gameLocal.GetLocalViewPlayer()->GetProxyEntity() && proxy && proxy->GetRenderEntity() ) {
				rvClientEntityPtr< sdClientAnimated > &cockpit = gameLocal.playerView.GetCockpit();
				bool dh = false;
				if ( cockpit.IsValid() && cockpit.GetEntity()->GetRenderEntity() ) {
					dh = cockpit.GetEntity()->GetRenderEntity()->flags.weaponDepthHack;
				}
				renderEntity.flags.weaponDepthHack = dh;
				idWeapon* weapon = GetWeapon();
				if ( weapon && weapon->GetRenderEntity() ) {
					weapon->GetRenderEntity()->flags.weaponDepthHack = dh;
				}
			} else {
				renderEntity.flags.weaponDepthHack = false;
				idWeapon* weapon = GetWeapon();
				if ( weapon && weapon->GetRenderEntity() ) {
					weapon->GetRenderEntity()->flags.weaponDepthHack = false;
				}
			}
		}
#endif
	}

	// FIXME: Move to script
	if ( !gameLocal.isClient ) {
		if ( invulnerableEndTime > gameLocal.time ) {
			if ( usercmd.buttons.btn.attack || usercmd.buttons.btn.altAttack || usercmd.buttons.btn.activate ) {
				SetInvulnerableEndTime( 0 );
			}
		}

		if ( health > 0 && gameLocal.time > nextBattleSenseBonusTime && !IsSpectator() ) {
			if ( combatState != 0 ) {
				if ( combatState & COMBATSTATE_KILLEDPLAYER && combatState & COMBATSTATE_DAMAGERECEIVED ) {
					sdProficiencyManager::GetInstance().GiveProficiency( gameLocal.battleSenseBonus[ 4 ], this, 1.f, NULL, "Battlesense Rank 5" );
				} else if ( combatState & COMBATSTATE_DAMAGEDEALT && combatState & COMBATSTATE_DAMAGERECEIVED ) {
					sdProficiencyManager::GetInstance().GiveProficiency( gameLocal.battleSenseBonus[ 3 ], this, 1.f, NULL, "Battlesense Rank 4" );
				} else if ( combatState & COMBATSTATE_KILLEDPLAYER ) {
					sdProficiencyManager::GetInstance().GiveProficiency( gameLocal.battleSenseBonus[ 2 ], this, 1.f, NULL, "Battlesense Rank 3" );
				} else if ( combatState & COMBATSTATE_DAMAGEDEALT ) {
					sdProficiencyManager::GetInstance().GiveProficiency( gameLocal.battleSenseBonus[ 1 ], this, 1.f, NULL, "Battlesense Rank 2" );
				} else if ( combatState & COMBATSTATE_DAMAGERECEIVED ) {
					sdProficiencyManager::GetInstance().GiveProficiency( gameLocal.battleSenseBonus[ 0 ], this, 1.f, NULL, "Battlesense Rank 1" );
				}
			}

			nextBattleSenseBonusTime = gameLocal.time + SEC2MS( 45 );
			combatState = 0;
		}

		if ( gameLocal.time > nextHealthCheckTime ) {
			if ( health > maxHealth ) {
				if ( nextHealthCheckTime != 0 ) {
					// Gordon: set it up like this so the first time it catches it
					// it'll defer to a tick later, so you don't lose stuff straight away
					int diff = health - maxHealth;
					if ( diff > healthCheckTickCount ) {
						diff = healthCheckTickCount;
					}
					SetHealth( health - diff );
				}

				nextHealthCheckTime = gameLocal.time + healthCheckTickTime;
			} else {
				nextHealthCheckTime = 0;
			}
		}
	}

	if ( !IsSpectator() ) {
		if ( gameLocal.isServer ) {
			UpdatePlayerInformation();
		}
	} else {
		clientInfo_t& clientInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];
		clientInfo.team = NOTEAM;

		if ( userInfo.isBot ) {
			if ( !gameLocal.rules->IsEndGame() ) {
				clientInfo.spectatorCounter++;

				if ( clientInfo.spectatorCounter > MAX_BOT_SPECTATE_TIMER ) {
					networkSystem->ServerKickClient( entityNumber, "", false );
					return;
				}
			}
		}
	}

	if ( !gameLocal.isClient ) {
		if ( nextBannerPlayTime != -1 ) {
			int bannerDelay = SEC2MS( g_bannerDelay.GetFloat() );
			if ( nextBannerPlayTime == 0 ) {
				nextBannerPlayTime = gameLocal.time + bannerDelay;
			} else {
				int max = 0;
				for ( int i = 0; i < NUM_BANNER_MESSAGES; i++ ) {
					if ( *g_bannerCvars[ i ]->GetString() != '\0' ) {
						max = i;
					}
				}
				if ( max != 0 ) {
					int loopDelay = SEC2MS( g_bannerLoopDelay.GetFloat() );

					if ( nextBannerIndex > max ) {
						nextBannerIndex = 0;

						if ( loopDelay == 0 ) {
							nextBannerPlayTime = -1;
						}
					}

					if ( nextBannerPlayTime != -1 ) {
						const char* text = g_bannerCvars[ nextBannerIndex ]->GetString();

						nextBannerIndex++;
						if ( nextBannerIndex > max ) {
							nextBannerIndex = 0;
							if ( loopDelay == 0 ) {
								nextBannerPlayTime = -1;
							} else {
								nextBannerPlayTime = gameLocal.time + loopDelay;
							}
						} else {
							nextBannerPlayTime = gameLocal.time + bannerDelay;
						}

						if ( *text != '\0' ) {
							SendUnLocalisedMessage( va( L"%hs", text ) );
						}
					}
				} else {
					nextBannerPlayTime = -1;
				}
			}
		}
	}

	if ( g_debugLocationalDamage.GetBool() && !IsSpectator() ) {

		idVec3 middle = physicsObj.GetOrigin() + physicsObj.GetBounds().GetCenter();
		idVec3 bottom = physicsObj.GetOrigin() + physicsObj.GetBounds().GetMins();
		idVec3 top = physicsObj.GetOrigin() + physicsObj.GetBounds().GetMaxs();

		const int NUM_HEIGHT_CHECKS = 32;

		float height = top[ 2 ] - bottom[ 2 ];
		float increment = height / ( float )NUM_HEIGHT_CHECKS;

		float highest = -999999.f;
		float lowest = 999999.f;
		for ( int i = 0; i < LDA_COUNT; i++ ) {
			if ( damageAreasScale[ i ] > highest ) {
				highest = damageAreasScale[ i ];
			}
			if ( damageAreasScale[ i ] < lowest ) {
				lowest = damageAreasScale[ i ];
			}
		}

		idAngles aim;
		aim.roll = 0.f;
		aim.pitch = 0.f;
		for ( aim.yaw = g_debugLocationalDamageStartAngle.GetFloat(); aim.yaw < g_debugLocationalDamageFinishAngle.GetFloat(); aim.yaw += 5.f ) {
			for ( int i = 0; i <= NUM_HEIGHT_CHECKS; i++ ) {
				float h = bottom[ 2 ] + ( increment * i );

				idVec3 inner = middle;
				inner[ 2 ] = h;

				idVec3 outer = inner + ( aim.ToForward() * 64.f );

				trace_t trace;
				gameLocal.TracePoint( trace, outer, inner, CONTENTS_SLIDEMOVER | CONTENTS_RENDERMODEL );
				if ( trace.fraction != 1.f && trace.c.entityNum == entityNumber ) {
					locationDamageArea_t area;
					idVec3 direction = inner - outer;
					direction.Normalize();
					float scale = GetDamageScaleForTrace( trace, direction, area );

					idVec4 colour = Lerp( colorYellow, colorRed, ( scale - lowest ) / ( highest - lowest ) );

					gameRenderWorld->DebugLine( colour, trace.endpos, outer, 0, true );
				}
			}
		}
	}
}

/*
==============
idPlayer::Kill
==============
*/
void idPlayer::Kill( idEntity* killer, bool noBody, const sdDeclDamage* damage, const sdDeclDamage* applyDamage ) {
	if ( IsSpectator() || IsInLimbo() ) {
		return;
	}

	if ( IsDead() ) {
		ServerForceRespawn( noBody );
		return;
	}

	if ( !killer ) {
		killer = this;
	}

	SetGodMode( false );
	playerFlags.noclip	= false;

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		proxy->GetUsableInterface()->OnExit( this, true );
	}

	if ( !applyDamage ) {
		if ( damage ) {
			applyDamage = damage;
		} else {
			applyDamage = DAMAGE_FOR_NAME( "damage_suicide" );
		}
	}

	int damageAmt = health - LIMBO_FORCE_HEALTH;
	killer->DamageFeedback( this, killer, health, LIMBO_FORCE_HEALTH, applyDamage, false );

	SetHealth( LIMBO_FORCE_HEALTH );
	Killed( killer, killer, damageAmt, vec3_zero, -1, applyDamage );
	ServerForceRespawn( noBody );
}

/*
==================
idPlayer::PlayDeathSound
==================
*/
void idPlayer::PlayDeathSound( void ) {
	const sdDeclPlayerClass* cls = GetInventory().GetClass();
	if ( cls != NULL ) {
		const char* sound = NULL;
		sound = cls->GetClassData().GetString( "snd_death" );

		if ( *sound ) {
			const idSoundShader* shader = gameLocal.declSoundShaderType[ sound ];
			if ( shader ) {
				StartSoundShader( shader, SND_VOICE, 0, NULL );
			} else {
				gameLocal.Warning( "idPlayer::PlayDeathSound Missing Sound '%s'", sound );
			}
		}
	}
}

/*
==================
idPlayer::HandleLifeStatsMessage
==================
*/
void idPlayer::HandleLifeStatsMessage( int statIndex, const sdPlayerStatEntry::statValue_t& oldValue, const sdPlayerStatEntry::statValue_t& newValue ) {
	const lifeStat_t& stat = gameLocal.lifeStats[ statIndex ];

	idWStrList parms;
	switch ( newValue.GetType() ) {
		case sdNetStatKeyValue::SVT_INT:
			if ( stat.isTimeBased ) {
				idStr::hmsFormat_t format;
				format.showZeroMinutes = true;
				parms.Append( va( L"%hs", idStr::MS2HMS( oldValue.GetInt() * 1000, format ) ) );
				parms.Append( va( L"%hs", idStr::MS2HMS( newValue.GetInt() * 1000, format ) ) );
			} else {
				parms.Append( va( L"%d", oldValue.GetInt() ) );
				parms.Append( va( L"%d", newValue.GetInt() ) );
			}
			break;
		case sdNetStatKeyValue::SVT_FLOAT:
			parms.Append( va( L"%i", idMath::Ftoi( idMath::Ceil( oldValue.GetFloat() ) ) ) );
			parms.Append( va( L"%i", idMath::Ftoi( idMath::Ceil( newValue.GetFloat() ) ) ) );
			break;
	}

	const sdDeclLocStr* lifeStatText = stat.textLong;

	idWStr text;
	text = common->LocalizeText( lifeStatText, parms );

	using namespace sdProperties;
	sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" );
	if( !scope ) {
		gameLocal.Warning( "idPlayer::HandleLifeStatsMessage: Couldn't find global 'gameHud' scope in guiGlobals." );
		return;
	}

	if( sdProperty* property = scope->GetProperty( "lifeStatsString", PT_WSTRING )) {
		*property->value.wstringValue = text;
	}

	if( sdProperty* property = scope->GetProperty( "lifeStatsTitle", PT_INT )) {
		*property->value.intValue = GetGameTeam()->GetLifeStatTitle()->Index();
	}
}

/*
==================
idPlayer::SendLifeStatsMessage
==================
*/
void idPlayer::SendLifeStatsMessage( int statIndex, const sdPlayerStatEntry::statValue_t& oldValue, const sdPlayerStatEntry::statValue_t& newValue ) {
	if ( gameLocal.IsLocalPlayer( this ) ) {
		HandleLifeStatsMessage( statIndex, oldValue, newValue );
		return;
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_LIFESTAT );
		msg.WriteLong( statIndex );
		msg.WriteByte( newValue.GetType() );
		switch ( newValue.GetType() ) {
			case sdNetStatKeyValue::SVT_INT:
				msg.WriteLong( oldValue.GetInt() );
				msg.WriteLong( newValue.GetInt() );
				break;
			case sdNetStatKeyValue::SVT_FLOAT:
				msg.WriteFloat( oldValue.GetFloat() );
				msg.WriteFloat( newValue.GetFloat() );
				break;
		}
		msg.Send( false, sdReliableMessageClientInfo( entityNumber ) );
	}
}

/*
==================
idPlayer::CalcLifeStats
==================
*/
void idPlayer::CalcLifeStats( void ) {
	if ( gameLocal.isClient ) {
		return;
	}
	if ( team != NULL && 
		gameLocal.rules->GetState() == sdGameRules::GS_GAMEON && 
		!gameLocal.IsDoingMapRestart() && 
		( gameLocal.GetLocalPlayer() == NULL || !g_trainingMode.GetBool() ) // Gordon: training mode only applies here when using a listen server
		) {
		
		sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();
		const idList< lifeStat_t >& lifeStats = gameLocal.lifeStats;

		size_t improvedValuesSize = sizeof( int ) * lifeStats.Num();
		int* improvedValues = ( int* )_alloca( improvedValuesSize );
		memset( improvedValues, 0, improvedValuesSize );
		int numImprovedValues = 0;

		size_t valuesSize = sizeof( sdPlayerStatEntry::statValue_t ) * lifeStats.Num();
		sdPlayerStatEntry::statValue_t* oldValues = ( sdPlayerStatEntry::statValue_t* )_alloca( valuesSize );
		sdPlayerStatEntry::statValue_t* newValues = ( sdPlayerStatEntry::statValue_t* )_alloca( valuesSize );

		for ( int i = 0; i < lifeStats.Num(); i++ ) {
			statHandle_t handle = tracker.GetStat( lifeStats[ i ].stat );
			if ( !handle.IsValid() ) {
				continue;
			}

			sdPlayerStatEntry* entry = tracker.GetStat( handle );

			sdNetStatKeyValue::statValueType newStatType;
			switch ( entry->GetType() ) {
				case sdNetStatKeyValue::SVT_FLOAT:
					newStatType = sdNetStatKeyValue::SVT_FLOAT_MAX;
					break;
				case sdNetStatKeyValue::SVT_INT:
					newStatType = sdNetStatKeyValue::SVT_INT_MAX;
					break;
				default:
					gameLocal.Warning( "idPlayer::CalcLifeStats Invalid Stat Used For Life Stats '%s'", lifeStats[ i ].stat.c_str() );
					continue;
			}


			newValues[ i ] = entry->GetDeltaValue( entityNumber );

			const char* lifeStatName = va( "lifestat_%s", lifeStats[ i ].stat.c_str() );

			sdPlayerStatEntry* lifeStatEntry = tracker.GetStat( tracker.AllocStat( lifeStatName, newStatType ) );
			oldValues[ i ] = lifeStatEntry->GetValue( entityNumber );
			gameLocal.GetGlobalStatsValueMax( entityNumber, lifeStatName, oldValues[ i ] );

			switch ( lifeStatEntry->GetType() ) {
				case sdNetStatKeyValue::SVT_FLOAT_MAX: {
					float newValue = newValues[ i ].GetFloat();
					float oldValue = oldValues[ i ].GetFloat();
					if ( idMath::Ceil( newValue ) > idMath::Ceil( oldValue ) ) {
						lifeStatEntry->SetValue( entityNumber, newValue );
						if ( oldValue > 0.f ) {
							improvedValues[ numImprovedValues ] = i;
							numImprovedValues++;
						}
					}
					break;
				}
				case sdNetStatKeyValue::SVT_INT_MAX: {
					int newValue = newValues[ i ].GetInt();
					int oldValue = oldValues[ i ].GetInt();
					if ( newValue > oldValue ) {
						lifeStatEntry->SetValue( entityNumber, newValue );
						if ( oldValue > 0 ) {
							improvedValues[ numImprovedValues ] = i;
							numImprovedValues++;
						}
					}
					break;
				}
				default:
					assert( false );
					break;
			}
		}

		if ( numImprovedValues > 0 ) {
			int index = improvedValues[ gameLocal.random.RandomInt( numImprovedValues ) ];
			SendLifeStatsMessage( index, oldValues[ index ], newValues[ index ] );
		}
	}

	sdGlobalStatsTracker::GetInstance().SetStatBaseLine( entityNumber );
}

/*
==================
idPlayer::RegisterTimeAlive
==================
*/
void idPlayer::RegisterTimeAlive( void ) {
	sdStatsTracker& stats = sdGlobalStatsTracker::GetInstance();
	int timeAlive = MS2SEC( gameLocal.time - lastAliveTimeRegistered );
	stats.GetStat( stats.AllocStat( "total_time_alive", sdNetStatKeyValue::SVT_INT ) )->IncreaseValue( entityNumber, timeAlive );
	lastAliveTimeRegistered = gameLocal.time;
}

/*
==================
idPlayer::OnKilled
==================
*/
void idPlayer::OnKilled( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	AI_DEAD = true;

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "EnterAnimState_Death" ), h1 );

	animator.ClearAllJoints();

	physicsObj.SetMovementType( PM_DEAD );

	if ( health != NO_DEATH_SOUND ) {
		PlayDeathSound();
	}

	// get rid of weapon
	if ( weapon.IsValid() ) {
		weapon.GetEntity()->OwnerDied();
	}

	killedTime = gameLocal.time;

	RegisterTimeAlive();

	sdScriptHelper helper;
	helper.Push( inflictor ? inflictor->GetScriptObject() : NULL );
	helper.Push( attacker ? attacker->GetScriptObject() : NULL );
	helper.Push( damage );
	helper.Push( dir );
	helper.Push( location );
	CallNonBlockingScriptEvent( onKilledFunction, helper );

	CalcLifeStats();

	UpdateCombatModel();
	SetSelectionCombatModel();

	sdDemoManager::GetInstance().GetDemoAnalyzer().LogPlayerDeath( this, inflictor, attacker );
}

class sdTeamKill_ComplaintFinaliser : public sdVoteFinalizer {
public:
	sdTeamKill_ComplaintFinaliser( idPlayer* player, idPlayer* attacker ) : _player( player ), _attacker( attacker ) {
	}

	virtual ~sdTeamKill_ComplaintFinaliser( void ) {
	}

	virtual void OnVoteCompleted( bool passed ) const {
		idPlayer* player = _player;
		if ( player == NULL ) {
			return;
		}

		idPlayer* attacker = _attacker;
		if ( attacker == NULL ) {
			return;
		}

		if ( passed ) {
			gameLocal.LogComplaint( player, attacker );
		} else {
			if ( player->Cast<idBot>() == NULL && attacker->Cast<idBot>() == NULL ) {
				idWStrList args; args.Append( va( L"%hs", player->userInfo.name.c_str() ) );
				attacker->SendLocalisedMessage( declHolder.declLocStrType[ "game/tkforgiven" ], args );
			}
		}
	}

private:
	idEntityPtr< idPlayer >	_player;
	idEntityPtr< idPlayer >	_attacker;
};



/*
==================
idPlayer::Killed
==================
*/
void idPlayer::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3& dir, int location, const sdDeclDamage* damageDecl ) {
	if ( attacker == this ) {
		GetInventory().LogSuicide();
	} else {
		GetInventory().LogDeath();
	}

	DropDisguise();

	if ( !gameLocal.isClient ) {
		baseDeathYaw = viewAngles.yaw;

		sdTeleporter* teleportEnt = teleportEntity;
		if ( teleportEnt ) {
			teleportEnt->FinishTeleport( this );
			teleportEnt->CancelTeleport( this );
			SetTeleportEntity( NULL );
		}
	}

	assert( !gameLocal.isClient );

	// stop taking knockback once dead
	fl.noknockback = true;

	if ( health < -999 && health != NO_DEATH_SOUND ) {
		SetHealth( -999 );
	}

	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		proxy->GetUsableInterface()->OnExit( this, true );
	}

	OnKilled( inflictor, attacker, damage, dir, location );

	fl.takedamage = true;		// can still be gibbed

	UpdateVisuals();

	playerFlags.wantSpawn	= false;

	if ( !gameLocal.isClient ) {
		if ( damageDecl ) {
			Obituary( attacker, damageDecl );

			if ( gameLocal.isServer ) {
				sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_OBITUARY );
				msg.WriteBits( entityNumber, idMath::BitsForInteger( MAX_CLIENTS ) );
				msg.WriteLong( gameLocal.GetSpawnId( attacker ) );
				msg.WriteBits( damageDecl->Index() + 1, gameLocal.GetNumDamageDeclBits() );
				msg.Send( sdReliableMessageClientInfoAll() );
			}
		}
	}

	ClearTargetLock();

	idPlayer* attackerPlayer = attacker->Cast< idPlayer >();
	if ( attackerPlayer != NULL && userInfo.showComplaints ) {
        if ( attackerPlayer != this ) {
			if ( !gameLocal.rules->IsWarmup() ) {
				if ( GetEntityAllegiance( attacker ) != TA_FRIEND ) {
					UpdatePlayerKills( entityNumber, attacker );
				} else {
					if ( gameLocal.isServer ) {
						if ( damageDecl->GetAllowComplaint() ) {
							bool allow = true;

							if ( attacker != NULL ) {
								const clientInfo_t& attackerInfo = botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ];
								if ( attackerInfo.isBot && !botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].isBot && attackerInfo.tkReviveTime < gameLocal.time ) {
									botThreadData.VOChat( SORRY, attacker->entityNumber, false );
								}
							}

							// team kill
							idCVar* complaintCVar = damageDecl->GetTeamKillCheckCvar();
							if ( complaintCVar != NULL ) {
								allow = complaintCVar->GetBool();
							}

							if ( allow ) {
								sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
								if ( vote != NULL ) {
									vote->DisableFinishMessage();
									vote->MakePrivateVote( this );
									vote->SetText( gameLocal.declToolTipType[ "teamkill_complaint" ] );
									vote->SetFinalizer( new sdTeamKill_ComplaintFinaliser( this, attackerPlayer ) );
									vote->Start();
								}
							}
						}
					}
				}
			}
		}
	}

	botThreadData.OnPlayerKilled( entityNumber );
}

/*
============
idPlayer::ClearIKJoints
============
*/
void idPlayer::ClearIKJoints( void ) {
	animator.SetJointAxis( GetHipJoint(), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetTorsoJoint(), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetHeadJoint(), JOINTMOD_NONE, mat3_identity );

	animator.SetJointAxis( GetHandJoint( 0 ), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetElbowJoint( 0 ), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetShoulderJoint( 0 ), JOINTMOD_NONE, mat3_identity );

	animator.SetJointAxis( GetHandJoint( 1 ), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetElbowJoint( 1 ), JOINTMOD_NONE, mat3_identity );
	animator.SetJointAxis( GetShoulderJoint( 1 ), JOINTMOD_NONE, mat3_identity );
}

/*
============
idPlayer::GetHeadModelCenter
============
*/
void idPlayer::GetHeadModelCenter( idVec3& output ) {
	// get the local position of the head joint
	idMat3 headAxis;
	idVec3 headOrigin;
	GetAnimator()->GetJointTransform( GetHeadModelJoint(), gameLocal.time, headOrigin, headAxis );

	// calculate the world position of the head model
	headAxis = headAxis * renderEntity.axis;
	headOrigin = renderEntity.origin + headOrigin * renderEntity.axis;
	headOrigin += GetHeadModelOffset() * headAxis;

	output = headOrigin;
}

/*
============
idPlayer::GetDamageScaleForTrace
============
*/
float idPlayer::GetDamageScaleForTrace( const trace_t& t, const idVec3& traceDirection, locationDamageArea_t& area ) {
	if ( locationalDamageInfo.Num() < 1 ) {
		area = LDA_INVALID;
		return 1.f;
	}

	idVec3 target = t.c.point;

	if ( gameLocal.isServer ) {
		target -= sdAntiLagManager::GetInstance().GetLastTraceHitOrigin( entityNumber );
	} else {
		target -= renderEntity.origin;
	}
	target *= renderEntity.axis.Transpose();

	// do the head check
	idVec3 headModelCenter;
	GetHeadModelCenter( headModelCenter );
	
	const idClipModel* headModel = physicsObj.GetHeadClipModel();
	if ( headModel != NULL && headModel->IsLinked() ) {
		// see if the trace would hit the head model
		idBounds headBounds = headModel->GetBounds();
		headBounds.TranslateSelf( headModelCenter );
		idVec3 start = t.c.point - traceDirection * 256.0f;
		float distance;
		if ( headBounds.RayIntersection( start, traceDirection, distance ) ) {
			idVec3 headHitPos = start + traceDirection * distance;

			//physicsObj.GetClipModel( 1 )->Draw( physicsObj.GetOrigin(), mat3_identity, 0.0f, 30000.0f );
			//gameRenderWorld->DebugArrow( colorGreen, start, t.c.point, 7.0f, 30000.0f );
			//gameRenderWorld->DebugArrow( colorBlue, t.c.point, headHitPos, 7.0f, 30000.0f );

			// check that it would have hit that pos, if it hadn't hit the main clip model first
			float playerDiameterSqr = Square( pm_bboxwidth.GetFloat() );
			float playerRadius = pm_bboxwidth.GetFloat() * 0.5f;

			idVec3 hitDelta = headHitPos - t.c.point;
			float zDist = idMath::Fabs( hitDelta.z );
			hitDelta.z = 0.0f;
			float xyDist = hitDelta.LengthSqr();

			if ( xyDist <= playerDiameterSqr && zDist < playerRadius ) {
				// hit the headbox!
				area = LDA_HEADBOX;
				return damageAreasScale[ area ];
			}
		}
	}



	idVec3 jointPos;
	animator.GetJointTransform( locationalDamageInfo[ 0 ].joint, gameLocal.time, jointPos );

	float distances[ LDA_COUNT ];
	for ( int i = 0; i < LDA_COUNT; i++ ) {
		distances[ i ] = -1.f;
	}

	for ( int i = 0; i < locationalDamageInfo.Num(); i++ ) {
		if ( locationalDamageInfo[ i ].area == LDA_HEADBOX ) {
			// headbox doesn't count, its handled above
			continue;
		}

		animator.GetJointTransform( locationalDamageInfo[ i ].joint, gameLocal.time, jointPos );

		float& current = distances[ locationalDamageInfo[ i ].area ];

		float dist = ( target - jointPos ).LengthFast();
		if ( dist < current || current < 0.f ) {
			current = dist;
		}
	}

	int bestIndex = -1;
	int secondBestIndex = -1;

	for ( int i = 0; i < LDA_COUNT; i++ ) {
		if ( distances[ i ] < 0 ) {
			continue;
		}

		if ( bestIndex == -1 || distances[ i ] < distances[ bestIndex ] ) {
			secondBestIndex = bestIndex;
			bestIndex = i;
			continue;
		}

		if ( secondBestIndex == -1 || distances[ i ] < distances[ secondBestIndex ] ) {
			secondBestIndex = i;
			continue;
		}
	}

	const float DAMAGE_BLEND_DIST = 8.f;

	area = ( locationDamageArea_t )bestIndex;

	float diff = distances[ secondBestIndex ] - distances[ bestIndex ];
	if ( diff <= DAMAGE_BLEND_DIST ) {
		float blend = ( DAMAGE_BLEND_DIST - diff ) / DAMAGE_BLEND_DIST;
		return Lerp( damageAreasScale[ bestIndex ], ( damageAreasScale[ secondBestIndex ] + damageAreasScale[ bestIndex ] ) * 0.5f, blend );
	}

	return damageAreasScale[ bestIndex ];
}

/*
=================
idPlayer::CalcDamagePoints
=================
*/
void idPlayer::CalcDamagePoints( idEntity *inflictor, idEntity *attacker, const sdDeclDamage* damageDecl, const float damageScale, const trace_t* collision, float& _health, const idVec3& dir, bool& headshot ) {
	bool noScale;
	float damage = damageDecl->GetDamage( this, noScale );

	headshot = false;

	locationDamageArea_t area = LDA_INVALID;
	if ( !noScale ) {
		float scale = ( 1.f - armor ) * damageScale;
		
		if ( collision != NULL && !gameLocal.isClient && damageDecl->GetCanHeadshot() ) {
			scale *= GetDamageScaleForTrace( *collision, dir, area );
		}
		damage *= scale;
	}

	idPlayer* player = attacker->Cast< idPlayer >();

	if ( gameLocal.rules->GetState() != sdGameRules::GS_GAMEON && !g_warmupDamage.GetBool() ) {
		damage = 0;
	} else if ( attacker != this ) { // you get self damage no matter what
		if ( !CheckTeamDamage( inflictor, damageDecl ) ) {
			damage = 0;
		}
	} else {
		damage *= damageDecl->GetSelfDamageScale();
	}

	idEntity* proxyEnt = GetProxyEntity();
	if ( proxyEnt != NULL ) {
		damage *= proxyEnt->GetUsableInterface()->GetDamageScale( this );
	}

	// check for completely getting out of the damage
	if ( !damageDecl->GetNoGod() && IsInvulernable() ) {
		// check for godmode
		damage = 0;
	}

	_health		= damage;

	if ( damage > 0.f ) {
		// Hit stats
		if ( player && GetEntityAllegiance( player ) == TA_ENEMY ) {
			const sdDeclDamage::stats_t& stats = damageDecl->GetStats();

			if ( area == LDA_NECK || area == LDA_TORSO ) {
				if ( stats.shotsHitTorso ) {
					stats.shotsHitTorso->IncreaseValue( player->entityNumber, 1 );
				}
			}
			if ( area == LDA_HEADBOX ) {
				headshot = true;
				if ( stats.shotsHitHead ) {
					stats.shotsHitHead->IncreaseValue( player->entityNumber, 1 );
				}
			}
		}

		if ( g_debugDamage.GetBool() ) {
			idStr areaName = "unknown";

			switch ( area ) {
				case LDA_LEGS:
					areaName = "legs";
					break;
				case LDA_TORSO:
					areaName = "chest";
					break;
				case LDA_HEAD:
					areaName = "headjoint";
					break;
				case LDA_NECK:
					areaName = "neck";
					break;
				case LDA_HEADBOX:
					areaName = "face";
					break;
			}

			idStr selfName = userInfo.name;
			idStr otherName = "unknown";

			idPlayer* other = attacker->Cast< idPlayer >();
			float spread = -666.0f;
			if ( other ) {
				otherName = other->userInfo.name;
				const idWeapon* currentWeapon = other->GetWeapon();
				if ( currentWeapon ) {
					spread = currentWeapon->GetSpreadValue();
				}
			}

			idStr damageName = damageDecl->GetName();

			idVec3 endPos = collision ? collision->c.point : idVec3( 0.f, 0.f, 0.f );

			idVec3 myLoc = GetPhysics()->GetOrigin();
			idVec3 otherLoc = attacker ? attacker->GetPhysics()->GetOrigin() : idVec3( 0.f, 0.f, 0.f );

			idStr status = "survived";
			if ( health - damage < 0 ) {
				status = "died";
			}

			idStr message = va( "%i\t\"%s\"\t( %s )\t\"%s\"\t( %s )\t\"%s\"\t\"%s\"\t%f\t%f\t( %s )\t( %s )\t\"%s\"\t%i\n", gameLocal.time, selfName.c_str(), myLoc.ToString(), otherName.c_str(), otherLoc.ToString(), areaName.c_str(), damageName.c_str(), damage, spread, endPos.ToString(), dir.ToString(), status.c_str(), health );

			gameLocal.LogDamage( message.c_str() );
		}
	}
}

/*
============
idPlayer::UpdateKillStats
============
*/
void idPlayer::UpdateKillStats( idPlayer* player, const sdDeclDamage* damageDecl, bool headshot ) {
	teamAllegiance_t allegiance = GetEntityAllegiance( player );

	const sdDeclDamage::stats_t& stats = damageDecl->GetStats();

	if ( allegiance == TA_ENEMY ) {
		if ( stats.deaths != NULL ) {
			stats.deaths->IncreaseValue( entityNumber, 1 );
		}
		if ( stats.totalDeaths != NULL ) {
			stats.totalDeaths->IncreaseValue( entityNumber, 1 );
		}

		if ( stats.kills != NULL ) {
			stats.kills->IncreaseValue( player->entityNumber, 1 );
		}
		if ( stats.totalKills != NULL ) {
			stats.totalKills->IncreaseValue( player->entityNumber, 1 );
		}
		if ( headshot && stats.totalHeadshotKills != NULL ) {
			stats.totalHeadshotKills->IncreaseValue( player->entityNumber, 1 );
		}
	} else if ( allegiance == TA_FRIEND ) {
		if ( player != this ) {
			if ( stats.teamKills ) {
				stats.teamKills->IncreaseValue( player->entityNumber, 1 );
			}
			if ( stats.totalTeamKills ) {
				stats.totalTeamKills->IncreaseValue( player->entityNumber, 1 );
			}
		}
	}
}

/*
============
idPlayer::DamageFeedback
============
*/
void idPlayer::DamageFeedback( idEntity* victim, idEntity* inflictor, int oldHealth, int newHealth, const sdDeclDamage* damageDecl, bool headshot ) {
	if ( victim != this && victim != inflictor && !inflictor->fl.noDamageFeedback ) {
		lastHitEntity = victim;
		lastHitHeadshot = headshot;
		SetLastDamageDealtTime( gameLocal.time );
	}

	const sdDeclDamage::stats_t& stats = damageDecl->GetStats();

	float scale = 0.f;
	if ( victim != this && victim != inflictor ) {
		if ( victim->GetEntityAllegiance( inflictor ) != TA_FRIEND && GetEntityAllegiance( victim ) == TA_ENEMY ) {
			scale = 1.f;

			int diff = oldHealth - Max( 0, newHealth );

			if ( diff > 0 ) {
				if ( stats.damage != NULL ) {
					stats.damage->IncreaseValue( entityNumber, diff );
				}

				if ( stats.totalDamage != NULL ) {
					stats.totalDamage->IncreaseValue( entityNumber, diff );
				}
			}

			if ( stats.shotsHit != NULL ) {
				stats.shotsHit->IncreaseValue( entityNumber, 1 );
			}

/*		} else if ( victim->GetEntityAllegiance( inflictor ) == TA_FRIEND && GetEntityAllegiance( victim ) != TA_ENEMY ) {
			scale = -1.f;*/ // Gordon: Removed for now
		}
	}

	bool killed = oldHealth > 0 && newHealth <= 0;
	if ( killed ) {
		if ( scale > 0.f ) {
			combatState |= COMBATSTATE_KILLEDPLAYER;
		}
		victim->UpdateKillStats( this, damageDecl, headshot );
	}

	if ( scale != 0.f ) {
		idPlayer* sharer = NULL;
		float shareFactor = 0.f;
		idEntity* proxy = GetProxyEntity();
		if ( proxy != NULL ) {
			sharer = proxy->GetUsableInterface()->GetXPSharer( shareFactor );
			if ( sharer == this ) {
				sharer = NULL;
			}
		}

		if ( newHealth < 0 ) {
			newHealth = 0;
		}

		float diff = ( oldHealth - newHealth ) * victim->GetDamageXPScale();
		if ( diff > 0 ) {
			float xpValue = scale * diff;

			// give damage bonus
			const sdDeclProficiencyItem* prof = damageDecl->GetDamageBonus();
			if ( prof ) {
				idStr xpReason = va( "Damage Dealt to %s", victim->GetEntityDefName() );

				if ( scale > 0.f ) {
					combatState |= COMBATSTATE_DAMAGEDEALT;
				}
				sdProficiencyManager::GetInstance().GiveProficiency( prof, this, xpValue, NULL, xpReason.c_str() );

				if ( stats.xp ) {
					stats.xp->IncreaseValue( entityNumber, prof->GetProficiencyCount() * xpValue );
				}

				if ( sharer != NULL && scale > 0.f ) { // only share positive XP
					if ( inflictor == this || inflictor == proxy || inflictor->IsOwner( proxy ) ) { // don't share random damage from other sources ( charges, etc )
						idStr shareBonusReason = va( "XP Share Bonus: %s", xpReason.c_str() );
						sdProficiencyManager::GetInstance().GiveProficiency( prof, sharer, xpValue * shareFactor, NULL, shareBonusReason.c_str() );
					}
				}
			}

			// if we're disguised and it was melee damage
			if ( !gameLocal.isClient && damageDecl->GetMelee() && IsDisguised() ) {
				if ( CheckDetected( 750.0f ) ) {
					DropDisguise();

					// Gordon: FIXME: W-HAT?
					// need to allow the player to continue stabbing, we need to force because the player is stabbing currently
					GetInventory().SetIdealWeapon( 0, true );
				}
			}
		}
	}
}

/*
============
idPlayer::BuildViewFrustum
============
*/
void idPlayer::BuildViewFrustum( idFrustum& frustum, float range ) const {
	frustum.SetOrigin( renderView.vieworg );
	frustum.SetAxis( renderView.viewaxis );
	float dNear = 0.0f;
	float dFar	= range;
	float dLeft = idMath::Tan( DEG2RAD( renderView.fov_x * 0.5f ) ) * range;
	float dUp	= idMath::Tan( DEG2RAD( renderView.fov_y * 0.5f ) ) * range;
	frustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
============
idPlayer::CheckDetected

see if this player is currently visible to any other players
============
*/
bool idPlayer::CheckDetected( float detectionRange ) {
	const float	detectionRangeSqr = Square( detectionRange );

	const idVec3& myOrigin = GetPhysics()->GetOrigin();

	idBox box( GetPhysics()->GetBounds(), myOrigin, GetPhysics()->GetAxis() );

	// get all the other players
	sdInstanceCollector< idPlayer > players( true );
	for ( int i = 0; i < players.Num(); i++ ) {
		idPlayer* other = players[ i ];
		if ( other == this ) {
			continue;
		}

		if ( GetEntityAllegiance( other ) != TA_ENEMY ) {
			continue;
		}

		if ( other->IsDead() || other->IsInLimbo() ) {
			continue;
		}

		//
		// First, quick check - are they within the detection range
		//
		const idVec3& otherOrigin = other->GetPhysics()->GetOrigin();
		idVec3 offset = myOrigin - otherOrigin;

		if ( offset.LengthSqr() > detectionRangeSqr ) {
			continue;
		}

		// construct their view frustum
		idFrustum f;
		other->BuildViewFrustum( f, detectionRange );

		if ( f.CullBox( box ) ) {
			continue;
		}

		//
		// Final, expensive check - trace to all corners of the BBox and see if they're visible
		//
		idVec3 temp;
		if ( CanDamage( renderView.vieworg, temp, MASK_SHOT_RENDERMODEL, this ) ) {
			return true;
		}
	}

	return false;
}

/*
============
idPlayer::ApplyRadiusPush
============
*/
void idPlayer::ApplyRadiusPush( const idVec3& pushOrigin, const idVec3& entityOrigin, const sdDeclDamage* damageDecl, float pushScale, float radius ) {
	// scale the push for the inflictor
	// scale by the push scale of the entity
	idVec3 impulse = entityOrigin - pushOrigin;
	float dist = impulse.Normalize();
	if ( dist > radius ) {
		return;
	}

	float distScale = Square( 1.0f - dist / radius );
	float scale = distScale * pushScale * g_knockback.GetFloat();
	if ( damageDecl != NULL ) {
		scale *= damageDecl->GetKnockback();
	}
	if ( idMath::Fabs( scale ) < idMath::FLT_EPSILON ) {
		return;
	}
	impulse *= scale;

	physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + impulse );
	physicsObj.SetKnockBack( 100 );
}

/*
============
idPlayer::Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space

damageDef	an idDict with all the options for damage effects

inflictor, attacker, dir, and point can be NULL for environmental effects
============
*/
void idPlayer::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damageDecl, const float damageScale, const trace_t* collision, bool forceKill ) {
	idVec3		kick;
	float		damage;
	idVec3		localDamageVector;

	if ( !fl.takedamage || playerFlags.noclip || IsSpectating() ) {
		return;
	}

	int oldHealth = health;

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}
	
	bool headshot = false;
	CalcDamagePoints( inflictor, attacker, damageDecl, damageScale, collision, damage, dir, headshot );

	if ( forceKill ) {
		damage = health;
	}

	if ( botThreadData.GetGameWorldState()->gameLocalInfo.gameIsBotMatch ) {
		sdTransport* vehicle = inflictor->Cast< sdTransport >();

		if ( vehicle != NULL ) {
			if ( GetEntityAllegiance( inflictor ) != TA_ENEMY ) {
				damage = 0.0f;
			}
		}
	}

	if ( damage > 0 ) {
		if ( damage < 1 ) {
			damage = 1;
		}

        if ( attacker )	{ //mal: lets check who attacked us, and if its a client whos not on our team, save that info for later - unless its warmup, then get ANYONE who attacks us.
			if ( attacker != this && ( attacker->entityNumber >= 0 && attacker->entityNumber < MAX_CLIENTS ) ) {
				if ( GetEntityAllegiance( inflictor ) != TA_FRIEND || gameLocal.rules->IsWarmup() )  {
					botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].lastAttacker = attacker->entityNumber; //mal: save off who attacked us last, so we can do some tactical planning
	                botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].lastAttackerTime = gameLocal.time;
					botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackClient = entityNumber; //mal: let them keep track of who they attacked last as well.
					botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackClientTime = gameLocal.time;
				}
			}
		}

		if ( !gameLocal.isClient ) {
			int intDamage = static_cast< int >( damage );

			bool applyTeamDamage = damageDecl->IsTeamDamage();
			if ( GetEntityAllegiance( inflictor ) == TA_ENEMY ) {
				if ( damageDecl->GetDamageBonus() ) {
					combatState |= COMBATSTATE_DAMAGERECEIVED;
				}
			} else {
				applyTeamDamage = true;
			}

			if ( applyTeamDamage ) {
				teamDamageDone += intDamage;
			}

			health -= intDamage;
#ifdef PLAYER_DAMAGE_LOG
			if ( g_drawPlayerDamage.GetBool() ) {
				if ( gameLocal.isServer ) {
					sdEntityBroadcastEvent msg( this, EVENT_TAKEDAMAGE );
					msg.WriteShort( intDamage > 65535 ? 65535 : intDamage );
					msg.WriteBits( damageDecl->Index(), gameLocal.GetNumDamageDeclBits() );
					msg.Send( false, sdReliableMessageClientInfoAll() );
				}

				if ( gameLocal.GetLocalPlayer() != NULL ) {
					LogDamage( damage, damageDecl );
				}
			}
#endif // PLAYER_DAMAGE_LOG

			// inform the attacker that they hit someone
			attacker->DamageFeedback( this, inflictor, oldHealth, health, damageDecl, headshot );

			if ( health <= 0 ) {
				// if you die from a damage decl that has "gib" set then you will be totally dead
				if ( damageDecl->GetGib() || health < -999 ) {
					SetHealth( -999 );
				}

				lastDmgTime = gameLocal.time;

				if ( oldHealth > 0 ) {
					Killed( inflictor, attacker, static_cast< int >( damage ), dir, JOINTHANDLE_FOR_TRACE( collision ), damageDecl );
				}

			} else {
				// let the anim script know we took damage
				Pain( inflictor, attacker, static_cast< int >( damage ), dir, JOINTHANDLE_FOR_TRACE( collision ), damageDecl );
				lastDmgTime = gameLocal.time;
			}
		}
	}

	// determine knockback
	float knockback = damageDecl->GetDamageKnockback();
	if ( knockback != 0.f && !fl.noknockback ) {
		kick = dir;
		kick.Normalize();
		kick *= g_knockback.GetFloat() * knockback;
		physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + kick );

		// set the timer so that the player can't cancel out the movement immediately
		physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
	}

	// move the world direction vector to local coordinates
	idVec3 damageFrom;
	damageFrom = dir;
	damageFrom.Normalize();

	viewAxis.ProjectVector( damageFrom, localDamageVector );

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( health > 0 && gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.playerView.DamageImpulse( localDamageVector, damageDecl );
	}

	lastDamageDecl		= damageDecl->Index();
	lastDamageDir		= damageFrom;

	if( gameLocal.IsLocalPlayer( this )) {
		sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
		weaponMenu->SetSwitchActive( sdWeaponSelectionMenu::AT_DISABLED );
	}
}

/*
====================
idPlayer::DefaultFov

Returns the base FOV
====================
*/
float idPlayer::DefaultFov( void ) {
	float fov = g_fov.GetFloat();

	if ( !gameLocal.CheatsOk( false ) ) {
		if ( fov < 90.0f ) {
			return 90.0f;
		} else if ( fov > 110.0f ) {
			return 110.0f;
		}
	}

	return fov;
}

/*
====================
idPlayer::CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float idPlayer::CalcFov( void ) {
	bool gotFov = false;

	float fov;

	if ( !gotFov ) {
		idEntity* proxy = GetProxyEntity();
		if ( proxy != NULL ) {
			if ( !proxy->GetUsableInterface()->GetAllowPlayerWeapon( this ) ) {
				fov = proxy->GetUsableInterface()->GetFov( this );
				gotFov = true;
			}
		}
	}

	if ( !gotFov ) {
		if ( gameLocal.IsLocalPlayer( this ) ) {
			sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
			if ( module ) {
				gotFov = module->GetFov( fov );
			}
		}
	}

	if ( !gotFov ) {
		idWeapon* weap = GetWeapon();
		if ( weap ) {
			gotFov = weap->GetFov( fov );
		}
	}

	if ( !gotFov ) {
		fov = DefaultFov();
	}

	// bound normal viewsize
	if ( fov < 1 ) {
		fov = 1;
	} else if ( fov > 179 ) {
		fov = 179;
	}

	return fov;
}

/*
==============
idPlayer::CalculateViewWeaponPos

Calculate the bobbing position of the view weapon
==============
*/


void idPlayer::CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis, bool ignorePitch ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	float driftScale = weapon->GetDriftScale();
	bool ironsight = ( driftScale < 0.1f );
	bool isLocal = gameLocal.IsLocalPlayer( this );

	const weaponAimValues_t& aimValues = weapon->GetAimValues( ironsight ? WAV_IRONSIGHTS : WAV_NORMAL );

	if ( !lastWeaponViewAnglesValid ) {
		lastWeaponViewAngles = viewAngles;
		lastWeaponViewAnglesValid = true;
	}

	float diffYaw = idMath::AngleDelta( viewAngles.yaw, lastWeaponViewAngles.yaw );
	float diffPitch = idMath::AngleDelta( viewAngles.pitch, lastWeaponViewAngles.pitch );
	weaponAngVel.x = weaponAngVel.x * aimValues.lagscalepitch + diffPitch * ( 1.f - aimValues.lagscalepitch );
	weaponAngVel.z = weaponAngVel.z * aimValues.lagscaleyaw + diffYaw * ( 1.f - aimValues.lagscaleyaw );
	if ( !gameLocal.IsPaused() ) {
		weaponAngVel.y = weaponAngVel.y * 0.9f + xyspeed * ( 1.f - 0.9f );
	}
	weaponAngVel.FixDenormals();

	// CalculateRenderView must have been called first
	const idVec3 &viewOrigin = firstPersonViewOrigin;
	idMat3 viewAxis = firstPersonViewAxis;

	if ( ignorePitch ) {
		// ignore the pitch component of the view
		viewAxis[ 0 ].z = 0.0f;
		viewAxis[ 1 ].z = 0.0f;
		viewAxis[ 2 ].Set( 0.0f, 0.0f, 1.0f );
		viewAxis[ 0 ].Normalize();
		viewAxis[ 1 ].Normalize();
	}

	float vely = physicsObj.GetLinearVelocity() * viewAxis[1];

	// these cvars are just for hand tweaking before moving a value to the weapon def
	idVec3 gunpos( g_gun_x.GetFloat(), g_gun_y.GetFloat(), g_gun_z.GetFloat() );

	gunpos += weapon->GetViewOffset();

	idVec3 localGunViewOfs = gunpos;

	float deffov = CalcFov();
	if ( deffov > 90.f ) {
		float fovofs = idMath::ClampFloat( 0.f, 1.f, (deffov - 90.f) / (110.f-90.f) ) * weapon->GetLargeFOVScale();
		gunpos.x += fovofs;
		// aprox ofsets
		// 100 -3
		// 105 -3.5
		// 110 -4
	}

	origin = viewOrigin + ( gunpos * viewAxis );

	if ( isLocal ) {
		origin += weaponAngVel.z * viewAxis[ 1 ] * aimValues.bobscaleyaw;
		origin += weaponAngVel.x * viewAxis[ 2 ] * aimValues.bobscalepitch;
	}

	if ( !ironsight ) {
		origin += vely * viewAxis[1] * -0.0025f;
		origin += bobfracsin * -0.25f * viewAxis[2];
//		origin += weaponAngVel.y * bobfracsin * 0.00025f * viewAxis[2];
//		origin += weaponAngVel.y * bobfracsin * 0.00025f * viewAxis[1];
	} else {
		origin += vely * viewAxis[1] * -0.00025f;
	}

	origin -= weaponAngVel.y * aimValues.speedlr * viewAxis[2];

	idVec3 gravity = physicsObj.GetGravityNormal();

	// drop the weapon when landing after a jump / fall
	int delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin -= gravity * ( landChange*0.25f * delta / LAND_DEFLECT_TIME );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin -= gravity * ( landChange*0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME );
	}

	idAngles driftAngles;

	idVec3 localOfs = ( origin - firstPersonViewOrigin ) * firstPersonViewAxis.Transpose();
	localOfs -= localGunViewOfs;
	localOfs.x = idMath::ClampFloat( -1.f, 1.f, localOfs.x );
	localOfs.y = idMath::ClampFloat( -2.5f, 2.5f, localOfs.y );
	localOfs.z = idMath::ClampFloat( -5.5f, 1.25f, localOfs.z );
	localOfs += localGunViewOfs;
	origin = ( localOfs * firstPersonViewAxis ) + firstPersonViewOrigin;

	// speed sensitive idle drift
	float scale = 10.f;//weaponAngVel.y + 10.0f;
	float fracsin = scale * idMath::Sin( MS2SEC( gameLocal.time ) * 2.f ) * 0.01f;// * driftScale;
	driftAngles.roll	= fracsin;
	driftAngles.yaw		= fracsin;
	driftAngles.pitch	= fracsin;

//	this->oldViewYaw
	if ( gameLocal.isNewFrame ) {
	//common->Printf( "%f %f\n", bobfracsin, fracsin );//, viewAngles.yaw-lastYaw );
	//common->Printf( "%f %f %f : %f %f %f : %f %f\n", deltaViewAngles[0], deltaViewAngles[1], deltaViewAngles[2], viewAngles[0], viewAngles[1], viewAngles[2], oldViewYaw, viewAngles.yaw - oldViewYaw );
		lastWeaponViewAngles = viewAngles;
	}

	axis = driftAngles.ToMat3() * viewAxis;
}

/*
===============
idPlayer::CalculateLookAtView
===============
*/
bool idPlayer::CalculateLookAtView( idPlayer* other, renderView_t& rView, bool doTraceCheck, float maxDist ) {
	idVec3 lookAtPoint = other->GetPhysics()->GetBounds().GetCenter();
	lookAtPoint += other->GetLastPredictionErrorDecayOrigin();

	idVec3 diff = lookAtPoint - firstPersonViewOrigin;
	float dist = diff.Normalize();
	if ( ( maxDist > 0.0f && dist > maxDist ) || dist < idMath::FLT_EPSILON ) {
		return false;
	}

	if ( doTraceCheck ) {
		trace_t trace;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, firstPersonViewOrigin, lookAtPoint, CONTENTS_SOLID, this );
		if ( trace.c.entityNum != other->entityNumber && trace.fraction != 1.f ) {
			return false;
		}
	}

	rView.vieworg		= firstPersonViewOrigin;

	if ( idMath::Fabs( diff.x ) < idMath::FLT_EPSILON && idMath::Fabs( diff.y ) < idMath::FLT_EPSILON ) {
		idAngles ang = ang_zero;
		if ( diff.z >= 0.0f ) {
			ang.pitch = -90.0f;
		} else {
			ang.pitch = 90.0f;
		}
		rView.viewaxis = ang.ToMat3();
	} else {
		rView.viewaxis[ 0 ]	= diff;
		rView.viewaxis[ 2 ] = idVec3( 0.f, 0.f, 1.f );
		rView.viewaxis[ 2 ] -= diff[ 2 ] * diff;
		rView.viewaxis[ 2 ].Normalize();
		rView.viewaxis[ 1 ] = rView.viewaxis[ 2 ].Cross( rView.viewaxis[ 0 ] );
	}

	// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
	// allow the right player view weapons
	rView.viewID		= entityNumber + 1;

	return true;
}

/*
===============
idPlayer::OffsetThirdPersonView
===============
*/
void idPlayer::OffsetThirdPersonView( float angle, float range, float height, bool clip, renderView_t& rView ) {
	idVec3			view;
	idVec3			focusAngles;
	trace_t			trace;
	idVec3			focusPoint;
	float			focusDist;
	float			forwardScale, sideScale;
	idVec3			origin;
	idAngles		angles;
	idMat3			axis;
	idBounds		bounds;

	angles = viewAngles;
	origin = firstPersonViewOrigin;
	axis = firstPersonViewAxis;

	focusPoint = origin + angles.ToForward() * THIRD_PERSON_FOCUS_DISTANCE;
	focusPoint.z += height;
	view = origin;
	view.z += 8 + height;

	angles.pitch *= 0.5f;
	rView.viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView.viewaxis[ 0 ];
	view += range * sideScale * renderView.viewaxis[ 1 ];

	if ( clip ) {
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, origin, view, gameLocal.clip.GetThirdPersonOffsetModel(), mat3_identity, MASK_SHOT_RENDERMODEL & ~CONTENTS_FORCEFIELD, this, TM_THIRDPERSON_OFFSET );
		if ( trace.fraction != 1.0f ) {
			view = trace.endpos;
			view.z += ( 1.0f - trace.fraction ) * 32.0f;

			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
			gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, origin, view, gameLocal.clip.GetThirdPersonOffsetModel(), mat3_identity, MASK_SHOT_RENDERMODEL & ~CONTENTS_FORCEFIELD, this, TM_THIRDPERSON_OFFSET );
			view = trace.endpos;
		}
	}

	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1.0f ) {
		focusDist = 1.0f;	// should never happen
	}

	if( pm_thirdPersonNoPitch.GetBool() ) {
		angles.pitch = 0.0f;
	} else {
		angles.pitch = - RAD2DEG( atan2( focusPoint.z, focusDist ) );
	}


	angles.yaw -= angle;

	rView.vieworg = view;
	rView.viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	rView.viewID = 0;
}

/*
===============
idPlayer::GetEyePosition
===============
*/
idVec3 idPlayer::GetEyePosition( void ) const {
	return physicsObj.GetOrigin() + ( eyeOffset * viewAxis );
}

/*
===============
idPlayer::GetViewPos
===============
*/
void idPlayer::GetViewPos( idVec3& origin, idMat3& axis ) const {
	idAngles angles;

	if ( gameLocal.rules->IsEndGame() ) {
		idEntity* camera = gameLocal.rules->GetEndGameCamera();
		if ( camera != NULL ) {
			origin = camera->GetRenderEntity()->origin;
			axis = camera->GetRenderEntity()->axis;
			return;
		}
	}

	// if dead, fix the angle and don't add any kick
	if ( IsDead() ) {
		axis = clientViewAngles.ToMat3();
		origin = GetEyePosition();
		return;
	}

	sdTeleporter* teleportEnt = teleportEntity;
	if ( teleportEnt != NULL ) {
		idEntity* viewEnt = teleportEnt->GetViewEntity();
		if ( viewEnt ) {
			origin	= viewEnt->GetPhysics()->GetOrigin();
			axis	= viewEnt->GetPhysics()->GetAxis();
			return;
		}
	}

	if ( !playerFlags.noclip ) {
		if ( remoteCamera.IsValid() ) {
			idEntity* cameraEnt = remoteCamera.GetEntity();

			if ( cameraEnt->IsType( idCamera::Type ) ) {
				// some default - not sure what to do here..
				axis	= clientViewAngles.ToMat3();
				origin	= GetEyePosition();
				return;
			}

			axis = cameraViewAngles.ToMat3();

			const renderEntity_t* cameraRenderEntity = cameraEnt->GetRenderEntity();
			origin = cameraRenderEntity->origin + remoteCameraViewOffset;

			return;
		}
	}

	// Standard player view
	origin = GetEyePosition() + viewBob;

	// Gordon: ensure the view is never bobbed below our origin ( falling when prone )
	// with a little bit of a safety margin
	const idVec3& viewNormal = viewAxis[ 2 ];
	float viewRealGround = ( physicsObj.GetOrigin() * viewNormal ) + 4.f;
	float viewGround = origin * viewNormal;
	if ( viewGround < viewRealGround ) {
		origin += ( viewRealGround - viewGround ) * viewNormal;
	}

	angles = clientViewAngles;

	float leanOffset = physicsObj.GetLeanOffset();

	// adjust for 'lean'
	if ( leanOffset != 0.f ) {
		angles.roll += leanOffset * 0.5f;

		idVec3 right;
		angles.ToVectors( NULL, &right, NULL );
		origin += right * leanOffset;
	}

	angles += viewBobAngles + gameLocal.playerView.AngleOffset( this );

	axis = angles.ToMat3() * physicsObj.GetGravityAxis();
}

/*
===============
idPlayer::GetRenderViewAxis
===============
*/
void idPlayer::GetRenderViewAxis( idMat3 &axis ) const {
	axis = renderView.viewaxis;
}

/*
===============
idPlayer::UpdatePausedObjectiveView
===============
*/
void idPlayer::UpdatePausedObjectiveView() {
	sdPlayerTask* task = sdTaskManager::GetInstance().TaskForHandle( lookAtTask );

	if ( task == NULL ) {

		// find a mission associated with it
		sdTeamInfo* team = GetGameTeam();
		if ( team == NULL ) {
			return;
		}

		const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( team );
		task = objectiveTasks.Next();

		if ( task == NULL ) {
			return;
		}
	}

	if ( task->GetEntity() == NULL ) {
		return;
	}

	idBounds lookAtBounds = task->GetEntity()->GetPhysics()->GetAbsBounds();
	idVec3 lookAtOrigin = lookAtBounds.GetCenter();
	if ( gameLocal.GetWorldPlayZoneIndex( lookAtOrigin ) != gameLocal.GetWorldPlayZoneIndex( firstPersonViewOrigin ) ) {
		return;
	}

	idVec3 lookAtDelta = lookAtOrigin - firstPersonViewOrigin;
	if ( lookAtDelta.LengthSqr() > idMath::FLT_EPSILON ) {
		idQuat fromQuat = firstPersonViewAxis.ToQuat();
		idQuat toQuat = lookAtDelta.ToAngles().ToQuat();

		float t = ( gameLocal.ToGuiTime( gameLocal.time ) - gameLocal.pauseStartGuiTime ) / 1000.0f;

		idQuat nowQuat;
		nowQuat.Slerp( fromQuat, toQuat, idMath::ClampFloat( 0.0f, 1.0f, t ) );

		// clear out roll
		idAngles nowAngles = nowQuat.ToAngles();
		nowAngles.roll = 0.0f;
		
		SetViewAngles( nowAngles );
		firstPersonViewAxis = nowAngles.ToMat3();
	}
}

/*
===============
idPlayer::CalculateFirstPersonView
===============
*/
void idPlayer::CalculateFirstPersonView( bool fullUpdate ) {
	if ( gameLocal.IsPaused() && g_pauseNoClip.GetBool() ) {
		gameLocal.GetPausedView( firstPersonViewOrigin, firstPersonViewAxis );
		return;
	}

	// let the proxy update
	idEntity* proxy = GetProxyEntity();
	if ( proxy != NULL ) {
		proxy->GetUsableInterface()->UpdateViewPos( this, firstPersonViewOrigin, firstPersonViewAxis, fullUpdate );
	} else {
		GetViewPos( firstPersonViewOrigin, firstPersonViewAxis );

		if ( gameLocal.IsPaused() && currentToolTip != NULL && ( sdTaskManager::GetInstance().TaskForHandle( lookAtTask ) != NULL || currentToolTip->GetLookAtObjective() )  ) {
			UpdatePausedObjectiveView();
		}

		if ( IsBeingBriefed() ) {
			UpdateBriefingView();
		}

		if ( gameLocal.isNewFrame && gameLocal.isClient ) {
			if ( gameLocal.IsLocalViewPlayer( this ) ) {
				if ( !gameLocal.IsLocalPlayer( this ) || sdDemoManager::GetInstance().InPlayBack() ) {
					gameLocal.playerView.UpdateSpectateView( this );
				} else {
					gameLocal.playerView.UpdateSpectateView( NULL );
				}
			}
		}
	}
}

/*
==================
idPlayer::GetRenderView

Returns the renderView that was calculated for this tic
==================
*/
renderView_t* idPlayer::GetRenderView( void ) {
	return &renderView;
}

/*
==================
idPlayer::SetupDeploymentView
==================
*/
void idPlayer::SetupDeploymentView( renderView_t& rView ) {
	OffsetThirdPersonView( pm_deployThirdPersonAngle.GetFloat(), pm_deployThirdPersonRange.GetFloat(), pm_deployThirdPersonHeight.GetFloat(), true, rView );
	gameLocal.CalcFov( CalcFov(), rView.fov_x, rView.fov_y );
}

/*
==================
idPlayer::SetTeleportEntity
==================
*/
void idPlayer::SetTeleportEntity( sdTeleporter* teleport ) {
	if ( teleport == NULL && teleportEntity.IsValid() ) {
		lastTeleportTime = gameLocal.time;
	}

	teleportEntity = teleport;

	idScriptObject* teleporterScript = teleport == NULL ? NULL : teleport->GetScriptObject();

	sdScriptHelper h1;
	h1.Push( teleporterScript );
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnTeleportEntityChanged" ), h1 );

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETTELEPORT );
		msg.WriteLong( teleportEntity.GetSpawnId() );
		msg.Send( teleportEntity.IsValid(), sdReliableMessageClientInfoAll() );
	}
}

/*
============
idPlayer::CheckWater
============
*/
void idPlayer::CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) {
	if ( waterEffects == NULL ) {
		return;
	}

	waterEffects->SetOrigin( GetPhysics()->GetOrigin() );
	waterEffects->SetAxis( GetPhysics()->GetAxis() );
	waterEffects->SetVelocity( GetPhysics()->GetLinearVelocity() );
	waterEffects->CheckWater( this, waterBodyOrg, waterBodyAxis, waterBodyModel );
}

/*
=================
idPlayer::IsFPSUnlock
=================
*/
bool idPlayer::IsFPSUnlock( void ) {
	idPlayer* viewPlayer = GetViewClient();
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	idPlayer* localViewPlayer = gameLocal.GetLocalViewPlayer();

	if ( this != localPlayer ) {
		return false;
	}
	
	if ( localViewPlayer == NULL ) {
		return false;
	}

	if ( gameLocal.GetActiveViewer() != this ) {
		return false;
	}

	if ( teleportEntity.IsValid() ) {
		return false;
	}

	if ( viewPlayer->GetProxyEntity() != NULL ) {
		return false;
	}

	if ( !viewPlayer->playerFlags.noclip ) {
		if ( viewPlayer->remoteCamera.IsValid() ) {
			return false;
		}

		if ( gameLocal.GetCamera() ) {
			return false;
		}
	}

	if ( ( viewPlayer == localPlayer ) && gameLocal.localPlayerProperties.GetDeployMenu()->Active() ) {
		return false;
	}
	
	if ( pm_thirdPerson.GetBool() ) {
		return false;
	}
	
	if ( this == localViewPlayer ) {
		if ( viewPlayer->IsDead() ) {
			return false;
		} else if ( viewPlayer->IsInLimbo() ) {
			return false;
		}
	}

	if ( localViewPlayer->IsProne() ) {
		return false;
	}

	return true;
}

/*
==================
idPlayer::CalculateRenderView

create the renderView for the current tic
==================
*/
void idPlayer::CalculateRenderView( void ) {
	idPlayer* viewPlayer = GetViewClient();

	idPlayer* localplayer = gameLocal.GetLocalPlayer();

	idMat3 lastViewAxis = renderView.viewaxis;
	idVec3 lastViewOrg	= renderView.vieworg;
	memset( &renderView, 0, sizeof( renderView ) );

	renderView.lastViewAxis = lastViewAxis;
	renderView.lastViewOrg = lastViewOrg;
	renderView.time = gameLocal.time;

	// calculate size of 3D view
	renderView.x = 0;
	renderView.y = 0;
	renderView.width = SCREEN_WIDTH;
	renderView.height = SCREEN_HEIGHT;
	renderView.viewID = 0;
	renderView.foliageDepthHack = 0.f;

	for ( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView.shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	if ( gameLocal.IsPaused() && g_pauseNoClip.GetBool() ) {
		renderView.vieworg = viewPlayer->firstPersonViewOrigin;
		renderView.viewaxis = viewPlayer->firstPersonViewAxis;
		gameLocal.CalcFov( 90.f, renderView.fov_x, renderView.fov_y );
		return;
	}

	sdTeleporter* teleportEnt = teleportEntity;
	if ( teleportEnt != NULL || gameLocal.rules->IsEndGame() ) {
		renderView.vieworg	= viewPlayer->firstPersonViewOrigin;
		renderView.viewaxis = viewPlayer->firstPersonViewAxis;

		renderView.viewID = entityNumber + 1;

		gameLocal.CalcFov( 90.f, renderView.fov_x, renderView.fov_y );
		return;
	}

	idEntity* proxy = viewPlayer->GetProxyEntity();
	if ( proxy ) {
		proxy->GetUsableInterface()->CalculateRenderView( viewPlayer, renderView );
		return;
	}

	if ( !viewPlayer->playerFlags.noclip ) {

		if ( viewPlayer->remoteCamera.IsValid() ) {
			idEntity* cameraEnt = viewPlayer->remoteCamera.GetEntity();
			if ( cameraEnt->IsType( idCamera::Type ) ) {
				cameraEnt->Cast< idCamera >()->GetViewParms( &renderView );
				return;
			}

			renderView.vieworg	= viewPlayer->firstPersonViewOrigin;
			renderView.viewaxis = viewPlayer->firstPersonViewAxis;

			// field of view
			float fov = 90.0f;
			idWeapon* weap = GetWeapon();
			if ( weap != NULL ) {
				float newFov = 90.0f;
				if ( weap->GetFov( newFov ) ) {
					fov = newFov;
				}
			}
			gameLocal.CalcFov( fov, renderView.fov_x, renderView.fov_y );

			return;
		}

		if ( gameLocal.GetCamera() ) {
			gameLocal.GetCamera()->GetViewParms( &renderView );
			return;
		}
	}

	if ( ( ( viewPlayer == gameLocal.GetLocalPlayer() ) && gameLocal.localPlayerProperties.GetDeployMenu()->Active() ) ) {
		viewPlayer->SetupDeploymentView( renderView );
		return;
	}

	if ( pm_thirdPerson.GetBool() ) {
		if ( gameLocal.IsLocalPlayer( this ) && gameLocal.isNewFrame ) {
			pm_thirdPersonAngle.SetFloat( pm_thirdPersonAngle.GetFloat() + ( pm_thirdPersonOrbit.GetFloat() * MS2SEC( gameLocal.msec ) ) );
		}
		viewPlayer->OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool(), renderView );
		gameLocal.CalcFov( CalcFov(), renderView.fov_x, renderView.fov_y );
		return;
	}

	if ( gameLocal.GetLocalViewPlayer() == this ) {
		if ( viewPlayer->IsDead() ) {
			// find a nearby person to look at
			qhandle_t handle = sdRequirementManager::GetInstance().RegisterAbility( "deadview" );
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* other = gameLocal.GetClient( i );
				if ( !other || other == viewPlayer ) {
					continue;
				}
				teamAllegiance_t allegiance = viewPlayer->GetEntityAllegiance( other );
				if ( allegiance != TA_FRIEND ) {
					continue;
				}
				if ( !other->HasAbility( handle ) ) {
					continue;
				}
				if ( other->IsDead() ) {
					continue;
				}
				if ( !viewPlayer->CalculateLookAtView( other, renderView, true, 2048.f ) ) {
					continue;
				}

				gameLocal.CalcFov( CalcFov(), renderView.fov_x, renderView.fov_y );
				return;
			}

			if ( !killer.IsValid() || gameLocal.time > killerTime || !viewPlayer->CalculateLookAtView( killer, renderView ) ) {
				viewPlayer->OffsetThirdPersonView( pm_deathThirdPersonAngle.GetFloat(), pm_deathThirdPersonRange.GetFloat(), pm_deathThirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool(), renderView );
			}

			gameLocal.CalcFov( CalcFov(), renderView.fov_x, renderView.fov_y );
			return;
		} else if ( viewPlayer->IsInLimbo() ) {
			viewPlayer->OffsetThirdPersonView( pm_deathThirdPersonAngle.GetFloat(), pm_deathThirdPersonRange.GetFloat(), pm_deathThirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool(), renderView );
			gameLocal.CalcFov( CalcFov(), renderView.fov_x, renderView.fov_y );
			return;
		} 
	}

	// Standard view
	renderView.vieworg = viewPlayer->firstPersonViewOrigin;
	renderView.viewaxis = viewPlayer->firstPersonViewAxis;

	// suppress the right player bodies and allow the right player view weapons
	renderView.viewID = entityNumber + 1;

	// field of view
	gameLocal.CalcFov( CalcFov(), renderView.fov_x, renderView.fov_y );
}

idCVar g_debugCrosshairDamageIndicator( "g_debugCrosshairDamageIndicator", "0", CVAR_GAME | CVAR_BOOL, "prints info on crosshair damage indicator to help track down issues with it" );

/*
=============
idPlayer::SetLastDamageDealtTime
=============
*/
void idPlayer::SetLastDamageDealtTime( int time ) {
	if ( !lastHitEntity.IsValid() ) {
		return;
	}

	if ( !gameLocal.isClient && time != 0 && lastDamageDealtTime != time && lastHitEntity != this ) {
		lastHitCounter++;
	}
	lastDamageDealtTime = time;
	newDamageDealt = ( gameLocal.ToGuiTime( lastDamageDealtTime ) > gameLocal.localViewChangedTime );

	if ( g_debugCrosshairDamageIndicator.GetBool() ) {
		gameLocal.Printf( "%d %d ent: %s\n", gameLocal.ToGuiTime( lastDamageDealtTime ), gameLocal.localViewChangedTime, lastHitEntity.GetEntity()->GetName() );
	}

	if( lastHitEntity != NULL ) {
		lastDamageDealtType = GetEntityAllegiance( lastHitEntity );
	} else {
		lastDamageDealtType = TA_NEUTRAL;
	}

	if ( !time ) {
		// level start and inits
		return;
	}

	// avoid hit beep because of view switch
	if ( nextSndHitTime < gameLocal.localViewChangedTime ) {
		nextSndHitTime = gameLocal.realClientTime + 100;
	}

	if ( g_hitBeep.GetInteger() > 0 && g_hitBeep.GetInteger() < 3 && time > nextSndHitTime ) {
		idEntity* ent = lastHitEntity.GetEntity();
		if ( ent != NULL ) {
			int duration = ent->PlayHitBeep( this, lastHitHeadshot );
			nextSndHitTime = gameLocal.realClientTime + duration;
		}
	}

	if ( lastHitEntity.IsValid() && gameLocal.IsLocalPlayer( this ) && GetEntityAllegiance( lastHitEntity ) == TA_FRIEND ) {
		if ( gameLocal.time - lastDamageFriendlyVO > SEC2MS( 10 ) && gameLocal.rules->IsGameOn() ) {
			idPlayer* player = lastHitEntity.GetEntity()->Cast<idPlayer>();
			if ( player != NULL && player->GetHealth() > 0 ) {
				lastDamageFriendlyVO = gameLocal.time;
				const idSoundShader* shader = gameLocal.declSoundShaderType[ team->GetDict().GetString( "snd_friendlyfire" ) ];
				gameSoundWorld->PlayShaderDirectly( shader, SND_PLAYER_VO );
			}
		}
	}
}

/*
================
idPlayer::OnLadder
================
*/
bool idPlayer::OnLadder( void ) const {
	return physicsObj.OnLadder();
}

typedef enum playerKey_e {
	PK_ATTACK,
	PK_RUN,
	PK_MODESWITCH,
	PK_MOUSELOOKOFF,
	PK_SPRINT,
	PK_ACTIVATE,
	PK_ALTFIRE,
	PK_LEANLEFT,
	PK_LEANRIGHT,
	PK_TOPHAT,
} playerKey_t;

/*
==================
idPlayer::Event_GetButton
==================
*/
void idPlayer::Event_GetButton( int key ) {
	bool value = false;
	switch( key ) {
		case PK_ATTACK:
			if ( !InhibitWeapon() ) {
				value = usercmd.buttons.btn.attack;
			}
			break;
		case PK_RUN:
			value = usercmd.buttons.btn.run;
			break;
		case PK_MODESWITCH:
			value = usercmd.buttons.btn.modeSwitch;
			break;
		case PK_MOUSELOOKOFF:
			value = usercmd.buttons.btn.mLookOff;
			break;
		case PK_SPRINT:
			value = usercmd.buttons.btn.sprint;
			break;
		case PK_ACTIVATE:
			if ( !InhibitWeapon() ) {
				value = usercmd.buttons.btn.activate;
			}
			break;
		case PK_ALTFIRE:
			if ( !InhibitWeapon() ) {
				value = usercmd.buttons.btn.altAttack;
			}
			break;
		case PK_LEANLEFT:
			value = usercmd.buttons.btn.leanLeft;
			break;
		case PK_LEANRIGHT:
			value = usercmd.buttons.btn.leanRight;
			break;
		case PK_TOPHAT:
			value = usercmd.buttons.btn.tophat;
			break;
	}
	sdProgram::ReturnBoolean( value );
}

/*
==================
idPlayer::Event_GetMove
==================
*/
void idPlayer::Event_GetMove( void ) {
	usercmd_t& cmd = gameLocal.usercmds[ entityNumber ];
	idVec3 move( cmd.forwardmove, cmd.rightmove, cmd.upmove );
	sdProgram::ReturnVector( move * ( 1 / 127.f ) );
}

/*
==================
idPlayer::Event_GetUserCmdAngles
==================
*/
void idPlayer::Event_GetUserCmdAngles( void ) {
	sdProgram::ReturnVector( idVec3( cmdAngles[ 0 ], cmdAngles[ 1 ], cmdAngles[ 2 ] ) );
}

/*
================
idPlayer::Event_GetViewAngles
================
*/
void idPlayer::Event_GetViewAngles( void ) {
	sdProgram::ReturnVector( idVec3( clientViewAngles[ 0 ], clientViewAngles[ 1 ], clientViewAngles[ 2 ] ) );
}

/*
================
idPlayer::Event_SetViewAngles
================
*/
void idPlayer::Event_SetViewAngles( const idVec3& angles ) {
	SetViewAngles( idAngles( angles ) );
//	deltaViewAngles.Zero();
}

/*
================
idPlayer::Event_GetCameraViewAngles
================
*/
void idPlayer::Event_GetCameraViewAngles( void ) {
	sdProgram::ReturnVector( idVec3( cameraViewAngles[ 0 ], cameraViewAngles[ 1 ], cameraViewAngles[ 2 ] ) );
}

/*
================
idPlayer::Event_GetRenderViewAngles
================
*/
void idPlayer::Event_GetRenderViewAngles( void ) {
	idAngles renderAngles = renderView.viewaxis.ToAngles();
	sdProgram::ReturnVector( idVec3( renderAngles[ 0 ], renderAngles[ 1 ], renderAngles[ 2 ] ) );
}

/*
================
idPlayer::Event_GetViewOrigin
================
*/
void idPlayer::Event_GetViewOrigin( void ) {
	sdProgram::ReturnVector( renderView.vieworg );
}

/*
==================
idPlayer::Event_GetWeaponEntity
==================
*/
void idPlayer::Event_GetWeaponEntity( void ) {
	sdProgram::ReturnEntity( weapon.GetEntity() );
}

/*
==================
idPlayer::Event_IsGunHidden
==================
*/
void idPlayer::Event_IsGunHidden( void ) {
	sdProgram::ReturnBoolean( !userInfo.showGun );
}

/*
================
idPlayer::Event_GiveClass
================
*/
void idPlayer::Event_GiveClass( const char* classname ) {
	GiveClass( classname );
}

/*
================
idPlayer::Event_GivePackage
================
*/
void idPlayer::Event_GivePackage( const char* packageName ) {
	sdProgram::ReturnBoolean( GivePackage( gameLocal.declItemPackageType[ packageName ] ) );
}

/*
================
idPlayer::Event_SetClassOption
================
*/
void idPlayer::Event_SetClassOption( int optionIndex, int itemIndex ) {
	GetInventory().SetClassOption( optionIndex, itemIndex );
}

/*
================
idPlayer::Event_SendToolTip
================
*/
void idPlayer::Event_SendToolTip( int toolTipIndex ) {
	if ( toolTipIndex < 0 || toolTipIndex >= gameLocal.declToolTipType.Num() ) {
		return;
	}

	SpawnToolTip( gameLocal.declToolTipType[ toolTipIndex ] );
}

/*
================
idPlayer::Event_CancelToolTips
================
*/
void idPlayer::Event_CancelToolTips( void ) {
	CancelToolTips();
}

/*
================
idPlayer::Event_SetWeapon
================
*/
void idPlayer::Event_SetWeapon( const char* weaponName, bool instant ) {
	if ( gameLocal.isClient ) {
		return;
	}

	GetInventory().SelectWeaponByName( weaponName, true );
	if ( instant ) {
		GetWeapon()->InstantSwitch();
	}
}

/*
================
idPlayer::Event_SelectBestWeapon
================
*/
void idPlayer::Event_SelectBestWeapon( bool allowCurrent ) {
	if ( gameLocal.isClient ) {
		return;
	}

	GetInventory().SelectBestWeapon( allowCurrent );
}

/*
================
idPlayer::Event_GetCurrentWeapon
================
*/
void idPlayer::Event_GetCurrentWeapon( void ) {
	if ( !GetWeapon() ) {
		sdProgram::ReturnString( "" );
		return;
	}

	const sdDeclInvItem* item = GetWeapon()->GetInvItemDecl();
	sdProgram::ReturnString( item ? item->GetName() : "" );
}

/*
================
idPlayer::Event_HasWeapon
================
*/
void idPlayer::Event_HasWeapon( const char* weaponName ) {
	if ( GetInventory().FindWeapon( weaponName ) == -1 ) {
		sdProgram::ReturnBoolean( false );
		return;
	}
	sdProgram::ReturnBoolean( true );
}

/*
================
idPlayer::Event_GetVehicle
================
*/
void idPlayer::Event_GetVehicle( void ) {
	sdProgram::ReturnEntity( GetProxyEntity()->Cast< sdTransport >() );
}

/*
==================
idPlayer::EnterDeploymentMode
==================
*/
void idPlayer::EnterDeploymentMode( void ) {
	if ( !gameLocal.IsLocalPlayer( this ) ) {
		return;
	}

	gameLocal.localPlayerProperties.GetDeployMenu()->Enable( true );
}

/*
==================
idPlayer::ExitDeploymentMode
==================
*/
void idPlayer::ExitDeploymentMode( void ) {
	if ( !gameLocal.IsLocalPlayer( this ) ) {
		return;
	}

	gameLocal.localPlayerProperties.GetDeployMenu()->Enable( false );
}

/*
================
idPlayer::ServerUseObject
================
*/
void idPlayer::ServerUseObject( void ) {
	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		sdInteractiveInterface* iface = proxy->GetInteractiveInterface();
		if ( iface && iface->OnUsed( this, 0.f ) ) {
			return;
		}
	}

	if ( remoteCamera.GetEntity() != NULL ) {
		return;
	}

	idWeapon* weapon = GetWeapon();
	if ( weapon && weapon->OnUsed( this, 0.f ) ) {
		return;
	}

	if ( crosshairInfo.IsUseValid() ) {
		idEntity* crosshairEnt = crosshairInfo.GetEntity();
		if ( crosshairEnt ) {
			sdInteractiveInterface* iface = crosshairEnt->GetInteractiveInterface();
			if ( iface ) {
				if ( iface->OnUsed( this, crosshairInfo.GetDistance() ) ) {
					return;
				}
			}
		}
	}
}

/*
================
idPlayer::DoActivate
================
*/
void idPlayer::DoActivate( void ) {
	crosshairEntActivate = false;

	if ( InhibitWeapon() ) {
		return;
	}

	if ( usercmd.buttons.btn.activate ) {
		if ( !oldButtons.btn.activate ) {
			//
			// Pressed
			//
			idEntity* proxy = GetProxyEntity();
			if ( proxy ) {
				sdInteractiveInterface* iface = proxy->GetInteractiveInterface();
				if ( iface && iface->OnActivate( this, 0.f ) ) {
					return;
				}
			}

			idWeapon* weapon = GetWeapon();
			if ( weapon != NULL && weapon->OnActivate( this, 0 ) ) {
				return;
			}

			if ( crosshairInfo.IsUseValid() ) {
				idEntity* crosshairEnt = crosshairInfo.GetEntity();
				if ( crosshairEnt ) {
					sdInteractiveInterface* iface = crosshairEnt->GetInteractiveInterface();
					if ( iface && iface->OnActivate( this, crosshairInfo.GetDistance() ) ) {
						if ( !IsType( idBot::Type ) ) {
							crosshairEntActivate = true;
						}
						return;
					}
				}
			}

			// all else failed, give the player a callback
			if ( onHitActivateFunction ) {
				sdScriptHelper h1;
				GetScriptObject()->CallNonBlockingScriptEvent( onHitActivateFunction, h1 );
			}
		} else {
			//
			// Held
			//
			idEntity* proxy = GetProxyEntity();
			if ( proxy ) {
				sdInteractiveInterface* iface = proxy->GetInteractiveInterface();
				if ( iface && iface->OnActivateHeld( this, 0.f ) ) {
					return;
				}
			}

			idWeapon* weapon = GetWeapon();
			if ( weapon != NULL && weapon->OnActivateHeld( this, 0 ) ) {
				return;
			}

			if ( crosshairInfo.IsUseValid() ) {
				idEntity* crosshairEnt = crosshairInfo.GetEntity();
				if ( crosshairEnt ) {
					sdInteractiveInterface* iface = crosshairEnt->GetInteractiveInterface();
					if ( iface && iface->OnActivateHeld( this, crosshairInfo.GetDistance() ) ) {
						if ( !IsType( idBot::Type ) ) {
							crosshairEntActivate = true;
						}
						return;
					}
				}
			}
		}
	}
}

/*
================
idPlayer::UpdateModelTransform
================
*/
void idPlayer::UpdateModelTransform( void ) {
	idEntity* proxy = GetProxyEntity();
	if ( proxy ) {
		proxy->GetWorldOriginAxis( GetBindJoint(), renderEntity.origin, renderEntity.axis );
		return;
	}

	idVec3 modelOffset = vec3_zero;
	if ( physicsObj.IsProne() ) {
		modelOffset = physicsObj.GetProneOffset();
	}

	renderEntity.axis	= viewAxis;
	renderEntity.origin = physicsObj.GetOrigin() + ( modelOffset * renderEntity.axis );

	if ( gameLocal.isClient && proxyEntity == NULL && !playerFlags.noclip && !IsBound() && !OnLadder() ) {
		if ( !gameLocal.IsLocalPlayer( this ) || net_clientSelfSmoothing.GetBool() ) {
			predictionErrorDecay_Origin.Decay( renderEntity.origin );
			if ( health > 0 ) {
				idAngles tempViewAngles = clientViewAngles;
				tempViewAngles.pitch = tempViewAngles.roll = 0.0f;
				renderEntity.axis = tempViewAngles.ToMat3();
			}
		}
	}
}

/*
============
idPlayer::UpdatePredictionErrorDecay
============
*/
void idPlayer::UpdatePredictionErrorDecay( void ) {
	predictionErrorDecay_Origin.Update();
	predictionErrorDecay_Angles.Update();
}

/*
============
idPlayer::OnNewOriginRead
============
*/
void idPlayer::OnNewOriginRead( const idVec3& newOrigin ) {
	idVec3 modelOffset = vec3_origin;
	if ( physicsObj.IsProne() ) {
		modelOffset = physicsObj.GetProneOffset();
	}
	idMat3 axis = viewAngles.ToMat3();
	idVec3 origin = physicsObj.GetOrigin() + ( modelOffset * axis );

	predictionErrorDecay_Origin.OnNewInfo( origin );
}

/*
============
idPlayer::OnNewAxesRead
============
*/
void idPlayer::OnNewAxesRead( const idMat3& newAxes ) {
	predictionErrorDecay_Angles.OnNewInfo( newAxes );
}

/*
============
idPlayer::ResetPredictionErrorDecay
============
*/
void idPlayer::ResetPredictionErrorDecay( const idVec3* origin, const idMat3* axes ) {
	if ( suppressPredictionReset ) {
		return;
	}

	if ( GetProxyEntity() != NULL ) {
		return;
	}

	idVec3 resetOrigin = GetPhysics()->GetOrigin();
	if ( origin != NULL ) {
		resetOrigin = *origin;
	}
	idMat3 resetAxes = viewAxis;
	if ( axes != NULL ) {
		resetAxes = *axes;
	}

	idVec3 modelOffset = vec3_zero;
	if ( physicsObj.IsProne() ) {
		modelOffset = physicsObj.GetProneOffset();
	}

	predictionErrorDecay_Origin.Reset( resetOrigin + ( modelOffset * resetAxes ) );
	predictionErrorDecay_Angles.Reset( resetAxes );

	physicsObj.ResetCollisionMerge( resetOrigin );
	ResetAntiLag();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_RESETPREDICTIONDECAY );
		msg.WriteFloat( resetOrigin[ 0 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		msg.WriteFloat( resetOrigin[ 1 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
		msg.WriteFloat( resetOrigin[ 2 ], PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );

		// send the important parts of the angles
		idAngles sendAngles = resetAxes.ToAngles();
		sendAngles.Normalize360();
		msg.WriteUShort( ANGLE2SHORT( sendAngles.yaw ) );
		msg.WriteUShort( ANGLE2SHORT( sendAngles.pitch ) );

		msg.Send( false, sdReliableMessageClientInfoAll() );
	}

	interpolateHistory[ 0 ] = resetOrigin;
	interpolateHistory[ 1 ] = resetOrigin;
}

/*
================
idPlayer::ResetAntiLag
================
*/
void idPlayer::ResetAntiLag( void ) {
	sdAntiLagManager::GetInstance().ResetForPlayer( this );
}

/*
================
idPlayer::GetPhysicsToSoundTransform
================
*/
bool idPlayer::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	idEntity *camera;

	camera = remoteCamera.GetEntity();
	if ( camera == NULL ) {
		camera = gameLocal.GetCamera();
	}

	if ( camera != NULL ) {
		renderView_t view;
		memset( &view, 0, sizeof( view ) );

		if ( camera->IsType( idCamera::Type ) ) {
			reinterpret_cast< idCamera* >( camera )->GetViewParms( &view );
			origin = view.vieworg;
			axis = view.viewaxis;
			return true;
		} else {
			origin = camera->GetPhysics()->GetOrigin() + remoteCameraViewOffset;
			axis = camera->GetPhysics()->GetAxis();
			return true;
		}
	} else {
		return idActor::GetPhysicsToSoundTransform( origin, axis );
	}
}

/*
================
idPlayer::OnProxyUpdate
================
*/
void idPlayer::OnProxyUpdate( int newSpawnId, int newPositionId ) {
	idEntity* oldProxy	= GetProxyEntity();
	int oldId			= proxyPositionId;

	idEntity* newProxyEnt = gameLocal.EntityForSpawnId( newSpawnId );

	if ( ( oldProxy == newProxyEnt ) && ( !oldProxy || oldId == newPositionId ) ) {
		return;
	}

	if ( oldProxy ) {
		if ( oldProxy != newProxyEnt ) {
			// if its the same entity then ForcePlacement will take care of it
			oldProxy->GetUsableInterface()->OnExit( this, true );
		}
	} else {
		oldId = -1;
	}

	if ( newProxyEnt ) {
		newProxyEnt->GetUsableInterface()->ForcePlacement( this, newPositionId, oldId, oldProxy == newProxyEnt );
	}
}

/*
================
idPlayer::ClientReceiveEvent
================
*/
bool idPlayer::ClientReceiveEvent( int event, int time, const idBitMsg& msg ) {
	switch ( event ) {
		case EVENT_RESPAWN: {
			idVec3 org;			
			org[ 0 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
			org[ 1 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
			org[ 2 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );

			idAngles ang;
			ang[ 0 ] = SHORT2ANGLE( msg.ReadUShort() );
			ang[ 1 ] = SHORT2ANGLE( msg.ReadUShort() );
			ang[ 2 ] = 0.f;

			SetOrigin( org );
			SetViewAngles( ang );

			idMat3 temp;
			ang.ToMat3NoRoll( temp );

			predictionErrorDecay_Origin.Reset( org );
			predictionErrorDecay_Angles.Reset( temp );

			bool revive = msg.ReadBits( 1 ) != 0;
			bool parachute = msg.ReadBits( 1 ) != 0;

			OnRespawn( revive, parachute, time, false );
			return true;
		}
		case EVENT_SPECTATE: {
			int id = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
			SetSpectateId( id );
			return true;
		}
		case EVENT_SETCLASS: {
			return GetInventory().SetClass( msg, false );
		}
		case EVENT_SETCACHEDCLASS: {
			return GetInventory().SetClass( msg, true );
		}
		case EVENT_SETTELEPORT: {
			int spawnId = msg.ReadLong();
			SetTeleportEntity( gameLocal.EntityForSpawnId( spawnId )->Cast< sdTeleporter >() );
			return true;
		}
		case EVENT_DISGUISE: {
			int disguiseSpawnId			= msg.ReadLong();
			int disguiseRankIndex		= msg.ReadLong();
			int disguiseRatingIndex		= msg.ReadLong();

			const sdDeclRank* rank = gameLocal.declRankType.SafeIndex( disguiseRankIndex );
			const sdDeclRating* rating = gameLocal.declRatingType.SafeIndex( disguiseRatingIndex );

			char buffer[ 256 ];
			msg.ReadString( buffer, sizeof( buffer ) );

			int disguiseClassIndex		= msg.ReadBits( gameLocal.GetNumPlayerClassBits() ) - 1;

			const sdDeclPlayerClass* pc = disguiseClassIndex == -1 ? NULL : gameLocal.declPlayerClassType[ disguiseClassIndex ];
			sdTeamInfo* playerTeam		= sdTeamManager::GetInstance().ReadTeamFromStream( msg );

			SetDisguised( disguiseSpawnId, pc, playerTeam, rank, rating, buffer );

			return true;
		}
		case EVENT_SELECTTASK: {
			SetActiveTask( msg.ReadBits( sdPlayerTask::TASK_BITS + 1 ) - 1 );
			return true;
		}
		case EVENT_SETPROXY: {
			int spawnId = msg.ReadLong();
			int positionId = msg.ReadLong();
			OnProxyUpdate( spawnId, positionId );
			return true;
		}
		case EVENT_SETPROXYVIEW: {
			SetProxyViewMode( msg.ReadByte(), true );
			return true;
		}
		case EVENT_BINADD: {
			BinAddEntity( gameLocal.EntityForSpawnId( msg.ReadLong() ) );
			return true;
		}
		case EVENT_BINREMOVE: {
			BinRemoveEntity( gameLocal.EntityForSpawnId( msg.ReadLong() ) );
			return true;
		}
#ifdef PLAYER_DAMAGE_LOG
		case EVENT_TAKEDAMAGE: {
			if ( !g_drawPlayerDamage.GetBool() ) {
				return true;
			}
			int damage = msg.ReadShort();
			int damageIndex = msg.ReadBits( gameLocal.GetNumDamageDeclBits() );
			const sdDeclDamage* damageDecl = gameLocal.declDamageType[ damageIndex ];
			LogDamage( damage, damageDecl );
			return true;
		}
#endif // PLAYER_DAMAGE_LOG
		case EVENT_SETCAMERA: {
			int spawnId = msg.ReadLong();
			SetRemoteCamera( gameLocal.EntityForSpawnId( spawnId ), true );
			return true;
		}
		case EVENT_SETREADY: {
			SetReady( msg.ReadBool(), true );
			return true;
		}
		case EVENT_SETINVULNERABLE: {
			SetInvulnerableEndTime( msg.ReadLong() );
			return true;
		}
		case EVENT_VOTE_DELAY: {
			SetNextCallVoteTime( msg.ReadLong() );
			return true;
		}
		case EVENT_RESETPREDICTIONDECAY: {
			idVec3 resetOrigin;
			resetOrigin[ 0 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
			resetOrigin[ 1 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );
			resetOrigin[ 2 ] = msg.ReadFloat( PLAYER_ORIGIN_EXPONENT_BITS, PLAYER_ORIGIN_MANTISSA_BITS );

			idAngles resetAngles( 0.0f, 0.0f, 0.0f );
			resetAngles.yaw = SHORT2ANGLE( msg.ReadUShort() );
			resetAngles.pitch = SHORT2ANGLE( msg.ReadUShort() );
			idMat3 resetAxis = resetAngles.ToMat3();
			ResetPredictionErrorDecay( &resetOrigin, &resetAxis );
			SetOrigin( resetOrigin );
			return true;
		}
		case EVENT_GODMODE: {
			SetGodMode( msg.ReadBool() );
			return true;
		}
		case EVENT_NOCLIP: {
			SetNoClip( msg.ReadBool() );
			return true;
		}
		case EVENT_SETWEAPON: {
			SetupWeaponEntity( msg.ReadLong() );
			return true;
		}
		case EVENT_LIFESTAT: {
			int statIndex = msg.ReadLong();
			sdNetStatKeyValue::statValueType type = ( sdNetStatKeyValue::statValueType )msg.ReadByte();

			switch ( type ) {
				case sdNetStatKeyValue::SVT_INT: {
					sdPlayerStatEntry::statValue_t oldValue( msg.ReadLong() );
					sdPlayerStatEntry::statValue_t newValue( msg.ReadLong() );
					HandleLifeStatsMessage( statIndex, oldValue, newValue );
					break;
				}
				case sdNetStatKeyValue::SVT_FLOAT: {
					sdPlayerStatEntry::statValue_t oldValue( msg.ReadFloat() );
					sdPlayerStatEntry::statValue_t newValue( msg.ReadFloat() );
					HandleLifeStatsMessage( statIndex, oldValue, newValue );
					break;
				}
			}
			return true;
		}
		default: {
			return idActor::ClientReceiveEvent( event, time, msg );
		}
	}
	return false;
}

/*
================
idPlayer::CalculateView
================
*/
void idPlayer::CalculateView( void ) {
	CalculateFirstPersonView( true );
	CalculateRenderView();
}

/*
================
idPlayer::SetRemoteCamera
================
*/
void idPlayer::SetRemoteCamera( idEntity* other, bool force ) {
	if ( gameLocal.isClient && !force ) {
		return;
	}

	idEntity* cam = remoteCamera;

	if ( cam == other ) {
		return;
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETCAMERA );
		msg.WriteLong( gameLocal.GetSpawnId( other ) );
		msg.Send( true, sdReliableMessageClientInfoAll() );
	}

	remoteCamera = other;
	if ( gameLocal.GetLocalViewPlayer() == this ) {
		gameLocal.localPlayerProperties.SetActiveCamera( remoteCamera );
	}

	crosshairInfo.Invalidate();
	CalculateView();
}

/*
================
idPlayer::SetGameTeam
================
*/
void idPlayer::SetGameTeam( sdTeamInfo* _team ) {
	if ( team != _team ) {
		lastTeamSetTime = gameLocal.time;

		if ( !gameLocal.isClient ) {
			sdTeleporter* tpEnt = teleportEntity;
			if ( tpEnt != NULL ) {
				tpEnt->CancelTeleport( this );
				SetTeleportEntity( NULL );
			}
		}

		SetReady( false, true );
		SetSpawnPoint( NULL );

		sdTeamInfo* oldTeam = team;
		team				= _team;

		if ( team != NULL ) {
			if ( !gameLocal.isClient ) {
				SetWantSpectate( false );

				idVec3		spawnOrigin;
				idAngles	spawnAngles;
				gameLocal.SelectInitialSpawnPoint( this, spawnOrigin, spawnAngles );
				SetPosition( spawnOrigin, spawnAngles.ToMat3() );
				SetSpectateClient( NULL );
				ServerForceRespawn( false );
			}
			gameLocal.RegisterTargetEntity( targetNode );
			nextTeamSwitchTime = gameLocal.time + SEC2MS( g_teamSwitchDelay.GetInteger() );
		} else {
			Spectate();
			targetNode.Remove();
		}

		UpdatePlayerTeamInfo();

		if ( gameLocal.IsLocalPlayer( this ) ) {
			gameLocal.localPlayerProperties.OnTeamChanged( oldTeam, team );
			sdTaskManager::GetInstance().OnLocalTeamChanged();
		}
		if ( gameLocal.IsLocalViewPlayer( this ) ) {
			gameLocal.OnLocalViewPlayerTeamChanged();
		}

		sdScriptHelper helper;
		helper.Push( oldTeam );
		helper.Push( team );
		CallNonBlockingScriptEvent( onSetTeamFunction, helper );

		gameLocal.rules->OnTeamChange( this, oldTeam, team );

		const sdDeclLocStr* joinMessage = declHolder.declLocStrType[ "game/playerjoined" ];
		if ( joinMessage != NULL && userInfo.name.Length() > 0 ) {
			idWStrList args( 2 );
			args.Append( va( L"%hs", userInfo.name.c_str() ) );
			if ( team != NULL ) {
				// FIXME: The team should store the decl, the name should not be used as a lookup
				const sdDeclLocStr* teamName = declHolder.declLocStrType.LocalFind( va( "game/%s", team->GetLookupName() ) );
				args.Append( teamName->GetText() );
			} else {
				const sdDeclLocStr* spectator = declHolder.declLocStrType.LocalFind( "game/spectators" );
				args.Append( spectator->GetText() );
			}
			idWStr localized = common->LocalizeText( joinMessage, args );
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_MESSAGE, -1, localized.c_str() );
		}

		physicsObj.SetupPlayerClipModels();
		UpdateVisibility();
		UpdateShadows();
		UpdateRating();
	}
}

/*
================
idPlayer::Hide
================
*/
void idPlayer::Hide( void ) {
	if ( IsHidden() ) {
		return;
	}

	idWeapon* weap = weapon.GetEntity();
	if ( weap ) {
		weap->UpdateVisibility();
	}

	GetInventory().OnHide();
	idActor::Hide();
}

/*
================
idPlayer::Show
================
*/
void idPlayer::Show( void ) {
	if ( !IsHidden() ) {
		return;
	}

	idWeapon* weap = weapon.GetEntity();
	if ( weap ) {
		weap->UpdateVisibility();
	}

	GetInventory().OnShow();
	idActor::Show();
}

/*
===============
idPlayer::SetSpectateOrigin
===============
*/
void idPlayer::SetSpectateOrigin( ) {
	idVec3 neworig = GetPhysics()->GetOrigin();
	neworig[ 2 ] += eyeOffset.z;
	neworig[ 2 ] += 25.0f;
	SetOrigin( neworig );
}

/*
==================
idPlayer::HandleGuiEvent
==================
*/
bool idPlayer::HandleGuiEvent( const sdSysEvent* event ) {
	if ( sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( ActiveGui() ) ) {
		return ui->PostEvent( event );
	}

	return false;
}

/*
==================
idPlayer::TranslateGuiBind
==================
*/
bool idPlayer::TranslateGuiBind( const idKey& key, sdKeyCommand** cmd ) {
	if ( sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( ActiveGui() ) ) {
		if ( ui->Translate( key, cmd ) ) {
			return true;
		}
	}

	return false;
}

/*
==================
idPlayer::Event_Freeze
==================
*/
void idPlayer::Event_Freeze( bool freeze ) {
	physicsObj.SetFrozen( freeze );
}

/*
==================
idPlayer::Event_FreezeTurning
==================
*/
void idPlayer::Event_FreezeTurning( bool freeze ) {
	playerFlags.turningFrozen = freeze;
}

/*
==================
idPlayer::Event_SetRemoteCamera
==================
*/
void idPlayer::Event_SetRemoteCamera( idEntity* other ) {
	SetRemoteCamera( other, false );
}

/*
==================
idPlayer::Event_SetRemoteCameraViewOffset
==================
*/
void idPlayer::Event_SetRemoteCameraViewOffset( const idVec3& offset ) {
	remoteCameraViewOffset = offset;
}

/*
===============
idPlayer::Event_GetViewingEntity
===============
*/
void idPlayer::Event_GetViewingEntity( void ) {
	idEntity* cam = remoteCamera.GetEntity();
	if ( cam == NULL ) {
		sdProgram::ReturnEntity( this );
	} else {
		sdProgram::ReturnEntity( cam );
	}
}

/*
===============
idPlayer::Event_MakeInvulnerable
===============
*/
void idPlayer::Event_MakeInvulnerable( float len ) {
	if ( gameLocal.isClient ) {
		return;
	}

	SetInvulnerableEndTime( gameLocal.time + SEC2MS( len ) );
}

/*
===============
idPlayer::Event_GetAmmoFraction
===============
*/
void idPlayer::Event_GetAmmoFraction( void ) {
	assert( gameLocal.isServer || gameLocal.IsLocalViewPlayer( this ) );
	sdProgram::ReturnFloat( inventoryExt.GetAmmoFraction() );
}

/*
===============
idPlayer::Event_GetUserName
===============
*/
void idPlayer::Event_GetUserName( void ) {
	sdProgram::ReturnString( userInfo.name );
}

/*
===============
idPlayer::Event_GetCleanUserName
===============
*/
void idPlayer::Event_GetCleanUserName( void ) {
	sdProgram::ReturnString( userInfo.cleanName );
}

/*
===============
idPlayer::Event_GetClassName
===============
*/
void idPlayer::Event_GetClassName( void ) {
	sdProgram::ReturnString( inventoryExt.GetClass() != NULL ? inventoryExt.GetClass()->GetName() : "spec" );
}

/*
===============
idPlayer::Event_GetCachedClassName
===============
*/
void idPlayer::Event_GetCachedClassName( void ) {
	sdProgram::ReturnString( inventoryExt.GetCachedClass() != NULL ? inventoryExt.GetCachedClass()->GetName() : "spec" );
}

/*
===============
idPlayer::Event_GetPlayerClass
===============
*/
void idPlayer::Event_GetPlayerClass( void ) {
	sdProgram::ReturnInteger( inventoryExt.GetClass() != NULL ? inventoryExt.GetClass()->Index() : -1 );
}

/*
===============
idPlayer::Event_GetShortRank
===============
*/
void idPlayer::Event_GetShortRank( void ) {
	const sdDeclRank* rank = GetProficiencyTable().GetRank();
	const sdDeclLocStr* str = rank != NULL ? rank->GetShortTitle() : NULL;
	sdProgram::ReturnHandle( str != NULL ? str->Index() : -1 );
}

/*
===============
idPlayer::Event_GetRank
===============
*/
void idPlayer::Event_GetRank( void ) {
	const sdDeclRank* rank = GetProficiencyTable().GetRank();
	const sdDeclLocStr* str = rank != NULL ? rank->GetTitle() : NULL;
	sdProgram::ReturnHandle( str != NULL ? str->Index() : -1 );
}

/*
===============
idPlayer::Event_GetProficiencyLevel
===============
*/
void idPlayer::Event_GetProficiencyLevel( int index ) {
	if ( index == -1 ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}
	sdProgram::ReturnInteger( GetProficiencyTable().GetLevel( index ) );
}

/*
===============
idPlayer::Event_GetXP
===============
*/
void idPlayer::Event_GetXP( int index, bool base ) {
	if ( index == -1 ) {
		sdProgram::ReturnFloat( GetProficiencyTable().GetXP() );
		return;
	}

	if ( base ) {
		sdProgram::ReturnFloat( GetProficiencyTable().GetPointsSinceBase( index ) );
		return;
	}

	sdProgram::ReturnFloat( GetProficiencyTable().GetPoints( index ) );
}

/*
===============
idPlayer::Event_GetCrosshairEntity
===============
*/
void idPlayer::Event_GetCrosshairEntity( void ) {
	if ( !crosshairInfo.IsUseValid() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}
	sdProgram::ReturnEntity( crosshairInfo.GetEntity() );
}

/*
===============
idPlayer::Event_GetCrosshairDistance
===============
*/
void idPlayer::Event_GetCrosshairDistance( bool needValidInfo ) {
	if ( needValidInfo && !crosshairInfo.IsUseValid() ) {
		sdProgram::ReturnFloat( idMath::INFINITY );
		return;
	}
	sdProgram::ReturnFloat( crosshairInfo.GetDistance() );
}

/*
===============
idPlayer::Event_GetCrosshairEndPos
===============
*/
void idPlayer::Event_GetCrosshairEndPos( void ) {
	sdProgram::ReturnVector( crosshairInfo.GetTrace().endpos );
}

/*
===============
idPlayer::Event_GetCrosshairStartTime
===============
*/
void idPlayer::Event_GetCrosshairStartTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( crosshairInfo.GetStartTime() ) );
}

/*
===============
idPlayer::Event_DeploymentMode
===============
*/
void idPlayer::Event_DeploymentMode( void ) {
	EnterDeploymentMode();
}

/*
===============
idPlayer::Event_ExitDeploymentMode
===============
*/
void idPlayer::Event_ExitDeploymentMode( void ) {
	ExitDeploymentMode();
}

/*
===============
idPlayer::Event_GetDeployResult
===============
*/
void idPlayer::Event_GetDeployResult( int deployObjectIndex ) {
	const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType.SafeIndex( deployObjectIndex );
	if ( !object ) {
		gameLocal.Warning( "idPlayer::Event_GetDeployResult Invalid Deploy Object Index" );
		sdProgram::ReturnInteger( DR_FAILED );
		return;
	}

	idVec3 point;
	sdProgram::ReturnInteger( GetDeployResult( point, object ) );
}

/*
===============
idPlayer::Event_GetDeploymentActive
===============
*/
void idPlayer::Event_GetDeploymentActive( void ) {
	if ( !gameLocal.IsLocalPlayer( this ) ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( gameLocal.localPlayerProperties.GetDeployMenu()->Active() );
}

/*
===============
idPlayer::Event_SetSpawnPoint
===============
*/
void idPlayer::Event_SetSpawnPoint( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( other != NULL ) {
		if ( GetEntityAllegiance( other ) != TA_FRIEND ) {
			return;
		}
	}

	SetSpawnPoint( other );
}

/*
===============
idPlayer::Event_GetSpawnPoint
===============
*/
void idPlayer::Event_GetSpawnPoint( void ) {
	sdProgram::ReturnEntity( selectedSpawnPoint );
}

/*
===============
idPlayer::Event_SetSpeedModifier
===============
*/
void idPlayer::Event_SetSpeedModifier( float value ) {
	speedModifier = value;
}

/*
===============
idPlayer::Event_SetSprintScale
===============
*/
void idPlayer::Event_SetSprintScale( float value ) {
	sprintScale = value;
}

/*
===============
idPlayer::Event_GetEnemy
===============
*/
void idPlayer::Event_GetEnemy( void ) {
	sdProgram::ReturnEntity( playerFlags.targetLocked ? targetEntity.GetEntity() : NULL );
}

/*
===============
idPlayer::Event_NeedsRevive
===============
*/
void idPlayer::Event_NeedsRevive( void ) {
	sdProgram::ReturnBoolean( NeedsRevive() );
}

/*
===============
idPlayer::Event_IsInvulernable
===============
*/
void idPlayer::Event_IsInvulernable( void ) {
	sdProgram::ReturnBoolean( IsInvulernable() );
}

/*
===============
idPlayer::Event_Revive
===============
*/
void idPlayer::Event_Revive( idEntity* other, float healthScale ) {
	idPlayer* playerOther = other->Cast< idPlayer >();

	if ( playerOther != NULL ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].mySavior = playerOther->entityNumber;
	}

	Revive( playerOther, healthScale );
}

extern idCVar net_clientMaxPrediction;

/*
===============
idPlayer::UpdatePlayerIcons
===============
*/
void idPlayer::UpdatePlayerIcons( void ) {
	if ( !gameLocal.isClient ) {
		SetLagged( networkSystem->ServerGetClientTimeSinceLastPacket( entityNumber ) > net_clientMaxPrediction.GetInteger() );
	}
}

/*
===============
idPlayer::SetLagged
===============
*/
void idPlayer::SetLagged( bool lagged ) {
	if ( playerFlags.isLagged == lagged ) {
		return;
	}

	playerFlags.isLagged = lagged;

	if ( playerFlags.isLagged ) {
		lagIcon = playerIcon.CreateIcon( gameLocal.declMaterialType[ spawnArgs.GetString( "mtr_icon_lag" ) ], 0, 0 );
	} else {
		playerIcon.FreeIcon( lagIcon );
		lagIcon = -1;
	}
}

/*
===============
idPlayer::SetGodMode
===============
*/
void idPlayer::SetGodMode( bool god ) {
	if ( playerFlags.godmode == god ) {
		return;
	}

	playerFlags.godmode = god;
	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_GODMODE );
		msg.WriteBool( god );
		msg.Send( god, sdReliableMessageClientInfoAll() );
	}
	UpdateGodIcon();
}

/*
===============
idPlayer::SetNoClip
===============
*/
void idPlayer::SetNoClip( bool noclip ) {
	if ( playerFlags.noclip == noclip ) {
		return;
	}

	playerFlags.noclip = noclip;
	ResetPredictionErrorDecay();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_NOCLIP );
		msg.WriteBool( noclip );
		msg.Send( noclip, sdReliableMessageClientInfoAll() );
	}
}

/*
===============
idPlayer::SetReady
===============
*/
void idPlayer::SetReady( bool value, bool force ) {
	if ( playerFlags.ready == value ) {
		return;
	}

	if ( gameLocal.isServer ) {
		if ( !force ) {
			if ( gameLocal.time < nextReadyToggleTime ) {
				return;
			}
			nextReadyToggleTime = gameLocal.time + SEC2MS( 5.f );
		}
	}

	playerFlags.ready = value;

	if( gameLocal.rules != NULL ) {
		gameLocal.rules->OnPlayerReady( this, value );
	}

	if( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETREADY );
		msg.WriteBool( value );
		msg.Send( playerFlags.ready, sdReliableMessageClientInfoAll() );
	}
}

/*
============
idPlayer::UpdateGodIcon
============
*/
void idPlayer::UpdateGodIcon() {
	if ( godIcon != -1 ) {
		playerIcon.FreeIcon( godIcon );
		godIcon = -1;
	}

	if ( playerFlags.godmode ) {
		godIcon = playerIcon.CreateIcon( gameLocal.declMaterialType[ spawnArgs.GetString( "mtr_icon_god" ) ], 0, 0 );
	} else if( invulnerableEndTime > gameLocal.time && godIcon == -1 ) {
		godIcon = playerIcon.CreateIcon( gameLocal.declMaterialType[ spawnArgs.GetString( "mtr_icon_god" ) ], 0, invulnerableEndTime - gameLocal.time );
	}
}

/*
============
idPlayer::SetInGame
============
*/
void idPlayer::SetInGame( bool inGame ) {
	if ( inGame && !playerFlags.ingame ) {
		gameLocal.Printf( "%s entered the game.\n", userInfo.name.c_str() );
	}
	playerFlags.ingame = inGame;
}

idCVar g_drawPlayerIcons( "g_drawPlayerIcons", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "Enables/Disables player icons" );
idCVar g_playerIconSize( "g_playerIconSize", "20", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "Size of the screen space player icons" );
idCVar g_playerArrowIconSize( "g_playerArrowIconSize", "10", CVAR_FLOAT | CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "Size of the screen space player arrow icons" );
idCVar g_playerIconAlphaScale( "g_playerIconAlphaScale",	"0.5",	CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE,			"alpha to apply to world-based objective icons" );

/*
============
idPlayer::IsObscuredBySmoke
============
*/
bool idPlayer::IsObscuredBySmoke( idPlayer* viewer ) {
	int smokeExplodeDelay = 6000;
	float maxSmokeRadius = 360.0f;
	float maxSmokeRadiusTime = 5000;
	idVec3 point;
	idVec3 start = viewer->firstPersonViewOrigin;
	idVec3 end = firstPersonViewOrigin;
	idVec3 temp;
	idVec3 vec, pVec, vProj;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		smokeBombInfo_t& smokeGrenade = botThreadData.GetGameWorldState()->smokeGrenades[ i ];

		if ( smokeGrenade.entNum == 0 && smokeGrenade.birthTime + SMOKE_GRENADE_LIFETIME < gameLocal.time ) {
			continue;
		}

		if ( smokeGrenade.birthTime + smokeExplodeDelay > gameLocal.time ) {
			continue;
		} //mal: takes a few seconds for the effect to be generated

		if ( smokeGrenade.xySpeed > 50.0f ) {
			continue;
		} //mal: dont worry about it if its still moving

		point = smokeGrenade.origin;

		point[ 2 ] += 32.0f; //mal: raise it off the ground.

		float smokeRadius = maxSmokeRadius * ( ( gameLocal.time - ( smokeGrenade.birthTime + smokeExplodeDelay - 3000 ) ) / maxSmokeRadiusTime );

		if ( smokeRadius > maxSmokeRadius ) {
			smokeRadius = maxSmokeRadius;
		}

		if ( bot_debug.GetBool() ) {
			gameRenderWorld->DebugCircle( colorRed, point, idVec3( 0, 0, 1 ), smokeRadius, 24, 16, true );
			gameRenderWorld->DebugLine( colorOrange, point, point + idVec3( 0, 0, 128.0f ) );
		}

		pVec = point - start;
		vec = end - start;
		float lengthSqr = vec.LengthSqr();
		float f = ( pVec * vec ) / lengthSqr;
		f = idMath::ClampFloat( 0.0f, 1.0f, f );
		vProj = pVec - ( f * vec );

		float dist = vProj.LengthSqr();

		if ( dist < Square( smokeRadius ) ) {
			return true;
		}
	}

	return false;
}

/*
===============
idPlayer::GetPlayerIcon
==============
*/
bool idPlayer::GetPlayerIcon( idPlayer* viewer, sdPlayerDisplayIcon& icon, const sdWorldToScreenConverter& converter ) {
	if ( !g_drawPlayerIcons.GetBool() ) {
		return false;
	}

	if ( viewer->IsSpectator() || viewer->teleportEntity.IsValid() ) {
		return false;
	}

	int selfWorldZoneIndex = gameLocal.GetWorldPlayZoneIndex( GetPhysics()->GetOrigin() );
	int otherWorldZoneIndex = gameLocal.GetWorldPlayZoneIndex( viewer->GetPhysics()->GetOrigin() );
	if ( selfWorldZoneIndex != otherWorldZoneIndex ) {
		return false;
	}

	sdTeamInfo* myTeam = team;
	idPlayer* myPlayer = this;

	const sdDeclPlayerClass* myClass = GetInventory().GetClass();

	if ( viewer->GetEntityAllegiance( this ) != TA_FRIEND ) {
		if ( IsDisguised() ) {
			myTeam = disguiseTeam;
			myClass = disguiseClass;
			myPlayer = gameLocal.EntityForSpawnId( disguiseClient )->Cast< idPlayer >();
			if ( myPlayer == NULL ) {
				myPlayer = this;
			}
		}
	}

	if ( myClass == NULL ) {
		return false;
	}

	bool isFriendly = false;
	bool isSquad = false;
	bool squadLeader = false;
	bool isBuddy = false;

#if !defined( SD_DEMO_BUILD )
	if ( networkService->GetActiveUser() != NULL ) {
		sdNetFriendsManager& friendsManager = networkService->GetFriendsManager();
		idPlayer* disguiseEntity = GetDisguiseEntity()->Cast< idPlayer >();
		idStr name = disguiseEntity->GetUserInfo().baseName;
		name.RemoveColors();
		if ( friendsManager.FindFriend( friendsManager.GetFriendsList(), name.c_str() ) != NULL ) {
			isBuddy = true;
		}
	}
#endif /* !SD_DEMO_BUILD */

	icon.distance = ( GetPhysics()->GetOrigin() - viewer->GetRenderView()->vieworg ).LengthSqr();

	sdTeamInfo* viewTeam = viewer->GetGameTeam();
	sdTransport* vehicle = GetProxyEntity()->Cast< sdTransport >();

	if ( viewTeam == myTeam ) {
		// Gordon: FIXME: Standardise this.
		isFriendly = true;

		if ( vehicle != NULL ) {
			if ( viewer->GetProxyEntity() == vehicle ) {
				return false;
			}
		}

		sdFireTeam* ft1 = gameLocal.rules->GetPlayerFireTeam( viewer->entityNumber );
		sdFireTeam* ft2 = gameLocal.rules->GetPlayerFireTeam( myPlayer->entityNumber );

		if ( ft1 != NULL || ft2 != NULL ) {
			if ( ft1 == ft2 ) {
				isSquad = true;

				if ( ft2->IsCommander( this ) ) {
					squadLeader = true;
				}
			}
		} else {
			sdPlayerTask* task1 = viewer->GetActiveTask();
			sdPlayerTask* task2 = myPlayer->GetActiveTask();
			if ( task1 == task2 ) {
				isSquad = true;
			}
		}
	} else {
		if ( icon.distance > Square( 2048.f ) ) {
			return false;
		}

		if ( vehicle == NULL ) {
			if ( !IsVisibleOcclusionTest() ) {
				return false;
			}

			if ( IsObscuredBySmoke( viewer ) ) {
				return false;
			}
		} else {
			if ( !vehicle->IsVisibleOcclusionTest() ) {
				return false;
			}
		}
	}

	float iconSize = g_playerIconSize.GetFloat();
	float arrowIconSize = g_playerArrowIconSize.GetFloat();

	icon.player = this;

	if ( isSquad && ( g_keepFireTeamList.GetBool() || g_showPlayerClassIcon.GetBool() || gameLocal.localPlayerProperties.GetShowFireTeam() ) ) {
		icon.offScreenIcon.material = myClass->GetIconOffScreen();;
		icon.offScreenIcon.color = colorWhite;
		icon.offScreenIcon.size = idVec2( arrowIconSize, arrowIconSize );
	} else {
		icon.offScreenIcon.material = NULL;
	}

	icon.arrowIcon.material = NULL;
	if ( g_showPlayerArrows.GetInteger() != 0 ) {
		if ( isFriendly ) {
			icon.arrowIcon.material = myClass->GetIconFriendlyArrow();
		} else {
			if ( g_showPlayerArrows.GetInteger() == 1 ) {
				icon.arrowIcon.material = myClass->GetIconEnemyArrow();
			}
		}
		icon.arrowIcon.size = idVec2( arrowIconSize, arrowIconSize );

		if ( isFriendly && isBuddy ) {
			icon.arrowIcon.color = idEntity::GetBuddyColor();
		} else if ( squadLeader ) {
			icon.arrowIcon.color = idEntity::GetFireteamLeaderColor();
		} else if ( isSquad ) {
			icon.arrowIcon.color = idEntity::GetFireteamColor();
		} else {
			icon.arrowIcon.color = idEntity::GetColorForAllegiance( isFriendly ? TA_FRIEND : TA_ENEMY );
		}

		if ( gameLocal.time - playerIconFlashTime < PLAYER_ICON_FLASH_TIME || IsSendingVoice() ) {
			icon.arrowIcon.color.w = idMath::Cos( gameLocal.time * 0.02f ) * 0.5f + 0.5f;
		}
	}

	icon.icon.material = playerIcon.GetActiveIcon();

	if ( icon.icon.material == NULL ) {
		if ( isSquad ) {
			if ( g_keepFireTeamList.GetBool() || g_showPlayerClassIcon.GetBool() || gameLocal.localPlayerProperties.GetShowFireTeam() ) {
				icon.icon.material = myClass->GetIconClass();
			}
		}
	}
	icon.icon.size = idVec2( iconSize, iconSize );
	icon.icon.color = idEntity::GetColorForAllegiance( isFriendly ? TA_FRIEND : TA_ENEMY );
	icon.icon.color.w *= g_playerIconAlphaScale.GetFloat();

	idVec3 origin;
	playerIcon.GetPosition( this, headJoint, 16, origin );
	converter.Transform( origin, icon.origin );

	return true;
}

/*
===============
idPlayer::FlashPlayerIcon
==============
*/
void idPlayer::FlashPlayerIcon( void ) {
	if ( gameLocal.GetLocalViewPlayer() == this ) {
		return;
	}

	playerIconFlashTime = gameLocal.time;
}

/*
===============
idPlayer::OnUpdateVisuals
==============
*/
void idPlayer::OnUpdateVisuals( void ) {
	LinkCombat();
	if ( gameLocal.isNewFrame ) {
		GetInventory().Present();
	}
}

/*
===============
idPlayer::CanCollide
==============
*/
bool idPlayer::CanCollide( const idEntity* other, int traceId ) const {
	if ( traceId == TM_CROSSHAIR_INFO ) {
		if ( this == other ) {
			if ( remoteCamera.IsValid() ) { // Gordon: should be able to view ourselves from a remote view
				return true;
			}
		}
	}
	if ( idEntity::CanCollide( other, traceId ) ) {
		if ( GetProxyEntity() == other ) {
			return other->ShouldProxyCollide( this );
		}
		return true;
	}

	return false;
}

/*
===============
idPlayer::DisableClip
==============
*/
void idPlayer::DisableClip( bool activateContacting ) {
	idEntity::DisableClip( activateContacting );
	DisableCombat();
}

/*
===============
idPlayer::EnableClip
==============
*/
void idPlayer::EnableClip( void ) {
	if ( fl.forceDisableClip ) {
		return;
	}
	idEntity::EnableClip();
	EnableCombat();
}

/*
===============
idPlayer::SetMaxHealth

modified to have low skilled bots, on the opposite team of the local player, be weaker - only in training or easy mode.
==============
*/
void idPlayer::SetMaxHealth( int count ) {
	if ( userInfo.isBot ) {
		if ( ( botThreadData.GetBotSkill() == BOT_SKILL_DEMO || botThreadData.GetBotSkill() == BOT_SKILL_EASY ) && botThreadData.GetGameWorldState()->gameLocalInfo.gameIsBotMatch ) {
			idPlayer* localPlayer = gameLocal.GetLocalPlayer();
			if ( localPlayer != NULL && localPlayer->team != NULL && team != NULL ) {
				if ( team->GetBotTeam() != localPlayer->team->GetBotTeam() ) {
					maxHealth = LOW_SKILL_BOT_HEALTH;
				} else {
					maxHealth = count;
				}
			} else {
				maxHealth = LOW_SKILL_BOT_HEALTH;
			}
		} else {
			maxHealth = count;
		}
	} else {
		maxHealth = count;
	}
}

/*
===============
idPlayer::WriteDemoBaseData
==============
*/
void idPlayer::WriteDemoBaseData( idFile* file ) const {
	idEntity::WriteDemoBaseData( file );

	file->WriteInt( spectateClient );

	GetInventory().WriteDemoBaseData( file );

	file->WriteInt( disguiseClient );
	file->WriteInt( disguiseRank == NULL ? -1 : disguiseRank->Index() );
	file->WriteInt( disguiseRating == NULL ? -1 : disguiseRating->Index() );
	file->WriteString( disguiseUserName.c_str() );

	// write out info about the disguise class
	int disguiseClassIndex = disguiseClass != NULL ? disguiseClass->Index() : -1;
	file->WriteInt( disguiseClassIndex );
	sdTeamManager::GetInstance().WriteTeamToStream( disguiseTeam, file );

	file->WriteInt( activeTask );

	file->WriteInt( remoteCamera.GetSpawnId() );

	file->WriteInt( proxyEntity.GetSpawnId() );
	file->WriteInt( proxyPositionId );
	file->WriteInt( proxyViewMode );
	file->WriteInt( selectedSpawnPoint.GetSpawnId() );
	file->WriteInt( teleportEntity.GetSpawnId() );
	file->WriteInt( invulnerableEndTime );

	file->WriteBool( playerFlags.ready );
	file->WriteBool( playerFlags.godmode );
	file->WriteBool( playerFlags.noclip );

	file->WriteInt( weapon.GetSpawnId() );
}

/*
===============
idPlayer::ReadDemoBaseData
==============
*/
void idPlayer::ReadDemoBaseData( idFile* file ) {
	idEntity::ReadDemoBaseData( file );

	file->ReadInt( spectateClient );

	GetInventory().ReadDemoBaseData( file );

	file->ReadInt( disguiseClient );

	int disguiseRankIndex;
	file->ReadInt( disguiseRankIndex );
	disguiseRank = gameLocal.declRankType.SafeIndex( disguiseRankIndex );

	int disguiseRatingIndex;
	file->ReadInt( disguiseRatingIndex );
	disguiseRating = gameLocal.declRatingType.SafeIndex( disguiseRatingIndex );

	file->ReadString( disguiseUserName );

	// read the disguise class information
	int disguiseClassIndex;
	file->ReadInt( disguiseClassIndex );
	if ( disguiseClassIndex != -1 ) {
		disguiseClass = gameLocal.declPlayerClassType[ disguiseClassIndex ];
	} else {
		disguiseClass = NULL;
	}
	disguiseTeam = sdTeamManager::GetInstance().ReadTeamFromStream( file );

	GetInventory().UpdateForDisguise();

	int dummy;
	file->ReadInt( dummy );
	activeTask = dummy;

	int newRemoteCameraSpawnId;
	file->ReadInt( newRemoteCameraSpawnId );
	SetRemoteCamera( gameLocal.EntityForSpawnId( newRemoteCameraSpawnId ), true );

	int newProxySpawnId;
	file->ReadInt( newProxySpawnId );

	int newProxyPositionId;
	file->ReadInt( newProxyPositionId );

	OnProxyUpdate( newProxySpawnId, newProxyPositionId );

	int newProxyViewMode;
	file->ReadInt( newProxyViewMode );
	SetProxyViewMode( newProxyViewMode, true );

	int spawnId;
	file->ReadInt( spawnId );
	SetSpawnPoint( gameLocal.EntityForSpawnId( spawnId ) );

	file->ReadInt( spawnId );
	SetTeleportEntity( gameLocal.EntityForSpawnId( spawnId )->Cast< sdTeleporter >() );

	file->ReadInt( invulnerableEndTime );

	bool readyDummy;
	file->ReadBool( readyDummy );
	playerFlags.ready = readyDummy;

	bool godDummy;
	file->ReadBool( godDummy );
	SetGodMode( godDummy );

	bool noclipDummy;
	file->ReadBool( noclipDummy );
	SetNoClip( noclipDummy );

	int weaponSpawnId;
	file->ReadInt( weaponSpawnId );
	SetupWeaponEntity( weaponSpawnId );
}

/*
===============
idPlayer::SendLocalisedMessage
==============
*/
void idPlayer::SendLocalisedMessage( const sdDeclLocStr* locStr, const idWStrList& parms ) {
	assert( parms.Num() <= 8 );
	assert( locStr );

	if ( locStr == NULL ) {
		return;
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		idWStr message = common->LocalizeText( locStr, parms );
		gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_SAY, -2, message.c_str() );
		return;
	}

	sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_LOCALISEDMESSAGE );
	outMsg.WriteVector( vec3_origin );
	outMsg.WriteLong( locStr->Index() );

	int parmCount = Min( parms.Num(), 8 );
	outMsg.WriteLong( parmCount );
	for ( int i = 0; i < parmCount; i++ ) {
		outMsg.WriteString( parms[ i ].c_str() );
	}
	outMsg.Send( sdReliableMessageClientInfo( entityNumber ) );
}

/*
===============
idPlayer::SendUnLocalisedMessage
==============
*/
void idPlayer::SendUnLocalisedMessage( const wchar_t* message ) {
	int clientNum = -2;

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_SAY, clientNum, message );
		return;
	}

	sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_CHAT );
	outMsg.WriteVector( vec3_origin );
	outMsg.WriteChar( clientNum );
	outMsg.WriteString( message );
	outMsg.Send( sdReliableMessageClientInfo( entityNumber ) );
}

/*
===============
idPlayer::NeedsRevive
==============
*/
bool idPlayer::NeedsRevive( void ) const {
	return IsDead();
}

/*
===============
idPlayer::ServerForceRespawn
==============
*/
void idPlayer::ServerForceRespawn( bool noBody ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( !playerFlags.forceRespawn ) {
		playerFlags.forceRespawn = true;
		OnForceRespawn( noBody );
	}
}

/*
===============
idPlayer::CreateBody
==============
*/
void idPlayer::CreateBody( void ) {
	if ( gameLocal.isClient ) {
		return;
	}
	const sdPlayerClassSetup* cls = inventoryExt.GetClassSetup();
	if ( cls && cls->GetClass() && team ) {
		const idDeclEntityDef* bodyDef = cls->GetClass()->GetBodyDef();
		if ( bodyDef == NULL ) {
			gameLocal.Warning( "idPlayer::CreateBody No Body on Class '%s'", cls->GetClass()->GetName() );
			return;
		}
		idEntity* bodyEnt;
		gameLocal.SpawnEntityDef( bodyDef->dict, true, &bodyEnt );
		sdPlayerBody* body = bodyEnt->Cast< sdPlayerBody >();
		if ( body == NULL ) {
			gameLocal.Warning( "idPlayer::CreateBody Failed to Spawn Body for Class '%s'", cls->GetClass()->GetName() );
			return;
		}
		body->Init( this, cls, team );
	}
}

/*
===============
idPlayer::OnFullyKilled
==============
*/
void idPlayer::OnFullyKilled( bool noBody ) {
	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "EnterAnimState_TapOut" ), h1 );

	// ao: Before call to onFullyKilledFunction so dead_body correctly creates a create-spawnhost-task
	if ( !noBody ) {
		CreateBody();
	}

	sdScriptHelper helper;
	CallNonBlockingScriptEvent( onFullyKilledFunction, helper );

	ownsVehicle = false; //mal: if player is killed into limbo, or taps, any vehicles he was driving no longer belongs to him.
}

/*
===============
idPlayer::OnForceRespawn
==============
*/
void idPlayer::OnForceRespawn( bool noBody ) {
	killedTime = gameLocal.time;

	OnFullyKilled( noBody );

	physicsObj.SetupPlayerClipModels();

	if ( !gameLocal.isClient ) {
		GetInventory().SetIdealWeapon( -1, true );
	}
}

/*
===============
idPlayer::SetSpawnPoint
===============
*/
void idPlayer::SetSpawnPoint( idEntity* other ) {
	if ( selectedSpawnPoint == other ) {
		return;
	}

	if ( !gameLocal.isClient ) {
		idEntity* oldSpawnHost = selectedSpawnPoint;
		if ( oldSpawnHost != NULL ) {
			idScriptObject* obj = oldSpawnHost->GetScriptObject();
			if ( obj != NULL ) {
				sdScriptHelper h1;
				h1.Push( scriptObject );
				obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnSpawnPointDeactivate" ), h1 );
			}
		}
	}

	selectedSpawnPoint = other;

	if ( !gameLocal.isClient ) {
		idEntity* newSpawnHost = other;
		if ( newSpawnHost != NULL ) {
			idScriptObject* obj = newSpawnHost->GetScriptObject();
			if ( obj != NULL ) {
				sdScriptHelper h1;
				h1.Push( scriptObject );
				obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnSpawnPointActivate" ), h1 );
			}
		}
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SETSPAWNPOINT );
		msg.WriteLong( selectedSpawnPoint.GetSpawnId() );
		msg.Send( sdReliableMessageClientInfo( entityNumber ) );
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.OnSpawnChanged();
	}
}

/*
===============
idPlayer::SetProxyEntity
===============
*/
void idPlayer::SetProxyEntity( idEntity* proxy, int positionIndex ) {
	idEntity* oldProxy	= GetProxyEntity();
	idWeapon* weapon	= GetWeapon();

	if ( proxy == NULL ) {
		positionIndex = -1;
	}

	bool changed = oldProxy != proxy;
	bool positionChanged = proxyPositionId != positionIndex;

	if ( changed ) {
		proxyEntity = NULL;

		if ( oldProxy ) {
			oldProxy->GetUsableInterface()->OnExit( this, true );
		}

		proxyEntity	= proxy;
	}


	if ( changed || positionChanged ) {
		proxyPositionId = positionIndex;

		// send callbacks to the view
		sdTransport* transportProxy = proxy->Cast< sdTransport >();
		if ( transportProxy != NULL ) {
			if ( !changed ) {
				transportProxy->GetViewForPlayer( this ).OnPlayerSwitched( this, true );
			} else {
				transportProxy->GetViewForPlayer( this ).OnPlayerEntered( this );
			}
		}

		changed = true;
		if ( weapon ) {
			weapon->UpdateVisibility();
		}
	}

	if ( changed ) {
		physicsObj.UpdateBounds();
		physicsObj.ResetProne();

		eyePosition = EP_NORMAL;
		eyeOffset = GetEyeOffset( eyePosition );

		if ( weapon ) {
			if ( proxyEntity.IsValid() ) {
				weapon->OnProxyEnter();
			} else {
				weapon->OnProxyExit();
			}
		}

		if ( gameLocal.isServer ) {
			sdEntityBroadcastEvent msg( this, EVENT_SETPROXY );
			msg.WriteLong( proxyEntity.GetSpawnId() );
			msg.WriteLong( proxyPositionId );
			msg.Send( proxyEntity.IsValid(), sdReliableMessageClientInfoAll() );
		}

		if ( proxyEntity.IsValid() ) {
			waterEffects->ResetWaterState();
		} else {
			sdScriptHelper h1;
			scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "EnterAnimState_Idle" ), h1 );
		}

		crosshairInfo.Invalidate();
		CalculateView();
	}
}

/*
============
idPlayer::SetProxyViewMode
============
*/
void idPlayer::SetProxyViewMode( int mode, bool force ) {
	if ( mode != proxyViewMode || force ) {
		proxyViewMode = mode;

		if ( gameLocal.isServer ) {
			sdEntityBroadcastEvent msg( this, EVENT_SETPROXYVIEW );
			msg.WriteByte( proxyViewMode );
			msg.Send( true, sdReliableMessageClientInfoAll() );
		}
	}

	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();
	if ( transport != NULL ) {
		transport->GetViewForPlayer( this ).OnPlayerSwitched( this, false );
	}
}

/*
============
idPlayer::SetWeaponSpreadInfo
============
*/
void idPlayer::SetWeaponSpreadInfo( void ) const {
	weaponSpreadValueIndex_t state = WSV_STANDING;

	if ( physicsObj.HasJumped() ) {
		state = WSV_JUMPING;
	} else if ( physicsObj.IsCrouching() ) {
		state = WSV_CROUCHING;
	} else if ( physicsObj.IsProne() ) {
		state = WSV_PRONE;
	}

	weapon->SetOwnerStanceState( state );
}

/*
============
idPlayer::Event_SetProxyEntity
============
*/
void idPlayer::Event_SetProxyEntity( idEntity* proxy, int positionId ) {
	SetProxyEntity( proxy, positionId );
}

/*
============
idPlayer::Event_GetProxyEntity
============
*/
void idPlayer::Event_GetProxyEntity() {
	sdProgram::ReturnEntity( GetProxyEntity() );
}

/*
============
idPlayer::Event_GetProxyAllowWeapon
============
*/
void idPlayer::Event_GetProxyAllowWeapon( void ) {
	idEntity* proxy = GetProxyEntity();
	if ( proxy == NULL ) {
		gameLocal.Warning( "idPlayer::Event_GetProxyAllowWeapon proxy is NULL" );
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( proxy->GetUsableInterface()->GetAllowPlayerWeapon( this ) );
}

/*
============
idPlayer::Event_SetSniperAOR
============
*/
void idPlayer::Event_SetSniperAOR( bool enabled ) {
	playerFlags.sniperAOR = enabled;
}

/*
============
idPlayer::Event_Enter
============
*/
void idPlayer::Event_Enter( idEntity* ent ) {
	sdUsableInterface* iface = ent->GetUsableInterface();
	if( iface ) {
		iface->FindPositionForPlayer( this );
	}
}

/*
============
idPlayer::Event_GetKilledTime
============
*/
void idPlayer::Event_GetKilledTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( killedTime ) );
}

/*
============
idPlayer::Event_ForceRespawn
============
*/
void idPlayer::Event_ForceRespawn( void ) {
	Kill( NULL );
	ServerForceRespawn( false );
}

/*
============
idPlayer::CanPlay
============
*/
bool idPlayer::CanPlay( void ) const {
	return !GetWantSpectate() && ( gameLocal.isClient || playerFlags.ingame );
}

/*
============
idPlayer::ShowWeaponMenu
============
*/
void idPlayer::ShowWeaponMenu( sdWeaponSelectionMenu::eActivationType show, bool noTimeout ) {
	if( !gameLocal.IsLocalPlayer( this ) ) {
		return;
	}

	int now = gameLocal.ToGuiTime( gameLocal.time );
	enableWeaponSwitchTimeout = !noTimeout;

	if( show == sdWeaponSelectionMenu::AT_DIRECT_SELECTION ) {
		weaponSwitchTimeout = now + SEC2MS( g_weaponSwitchTimeout.GetFloat() );
	}

	if( show != sdWeaponSelectionMenu::AT_DISABLED && GetInventory().GetSwitchingWeapon() == -1 ) {
		return;
	}

	if( IsSpectating() || WantRespawn() ) {
		show = sdWeaponSelectionMenu::AT_DISABLED;
	}

	sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
	weaponMenu->SetSwitchActive( show );

	if( show == sdWeaponSelectionMenu::AT_DISABLED ) {
		weaponSwitchTimeout = 0;
	} else {
		weaponSwitchTimeout = now + SEC2MS( g_weaponSwitchTimeout.GetFloat() );
		if( !noTimeout && weaponSwitchTimeout == now ) {
			AcceptWeaponSwitch( false );
			return;
		} else {
			// cancel context/radio menu if you try to switch weapons while either is up
			sdQuickChatMenu* contextMenu = gameLocal.localPlayerProperties.GetContextMenu();
			if ( contextMenu->Enabled() ) {
				contextMenu->Enable( false );
			}

			sdQuickChatMenu* quickChatMenu = gameLocal.localPlayerProperties.GetQuickChatMenu();
			if ( quickChatMenu->Enabled() ) {
				quickChatMenu->Enable( false );
			}
			sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
			if( !weaponMenu->Enabled() ) {
				weaponMenu->Enable( true );
			}

			sdFireTeamMenu* fireTeamMenu = gameLocal.localPlayerProperties.GetFireTeamMenu();
			if( fireTeamMenu->Enabled() ) {
				fireTeamMenu->Enable( false );
			}
		}
	}

	using namespace sdProperties;
	sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "weapons" );
	if( !scope ) {
		gameLocal.Warning( "idPlayer::UpdateWeaponMenu: Couldn't find global 'weapons' scope in guiGlobals." );
		return;
	}

	if( sdProperty* property = scope->GetProperty( "menuExpireTime", PT_FLOAT )) {
		*property->value.floatValue = weaponSwitchTimeout;
	}
}


/*
============
idPlayer::AcceptWeaponSwitch
============
*/
bool idPlayer::AcceptWeaponSwitch( bool hideWeaponMenu ) {

//mal: bots dont need the menu stuff - just do the weap switch and leave.
	if ( userInfo.isBot ) {
		SwitchWeapon();
		return true;
	}

	if( !gameLocal.IsLocalPlayer( this ) || IsSpectating() || WantRespawn() ) {
		return false;
	}

	bool retVal = false;
	sdWeaponSelectionMenu* weaponMenu = gameLocal.localPlayerProperties.GetWeaponSelectionMenu();
	if( weaponMenu->IsMenuVisible() ) {

		if( GetInventory().GetSwitchingWeapon() != GetInventory().GetCurrentWeapon() ) {
			retVal = true;

			using namespace sdProperties;
			sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "weapons" );
			if( scope ) {
				if( weaponSelectionItemIndex != -1 ) {
					if( sdProperty* property = scope->GetProperty( "weaponSelected", PT_FLOAT )) {
						*property->value.floatValue = weaponSelectionItemIndex;
					}
				}
			} else {
				gameLocal.Warning( "idPlayer::AcceptWeaponSwitch: Couldn't find global 'weapons' scope in guiGlobals." );
			}
		}

		SwitchWeapon();
		if( hideWeaponMenu ) {
			ShowWeaponMenu( sdWeaponSelectionMenu::AT_DISABLED );
		}
	}

	return retVal;
}


/*
============
idPlayer::SwitchWeapon
============
*/
void idPlayer::SwitchWeapon() {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_CHANGEWEAPON );
		msg.WriteLong( GetInventory().GetSwitchingWeapon() );
		msg.Send();
	} else {
		GetInventory().AcceptWeaponSwitch();
	}
}

/*
============
idPlayer::Pain
============
*/
bool idPlayer::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	if ( !damageDecl->GetNoPain() ) {
		lastHurtTime = gameLocal.time;
		if ( gameLocal.IsLocalPlayer( this ) ) {
			idVec3 dirNormalized = -dir;
			dirNormalized.Normalize();

			if ( !damageDecl->GetNoDirection() ) {
				AddDamageEvent( gameLocal.time, dirNormalized.ToAngles().Normalize180().yaw, damage, true );
			} else {
				AddDamageEvent( gameLocal.time, 0, damage, false );
				AddDamageEvent( gameLocal.time, 90, damage, false );
				AddDamageEvent( gameLocal.time, 180, damage, false );
				AddDamageEvent( gameLocal.time, 270, damage, false );
			}

	/*		if( g_debugDamage.GetBool() ) {
				gameLocal.Printf( "Damage vector: %s angle %f\n", dirNormalized.ToString(), lastHitAngle );
			}*/
		}
	}

	sdScriptHelper helper;
	helper.Push( inflictor ? inflictor->GetScriptObject() : NULL );
	helper.Push( attacker ? attacker->GetScriptObject() : NULL );
	helper.Push( damage );
	helper.Push( dir );
	helper.Push( location );
	CallNonBlockingScriptEvent( onPainFunction, helper );

	return idActor::Pain( inflictor, attacker, damage, dir, location, damageDecl );
}

/*
=====================
idPlayer::PlayPain
=====================
*/
void idPlayer::PlayPain( const char* size ) {
	const sdDeclPlayerClass* cls = GetInventory().GetClass();
	if ( cls == NULL ) {
		return;
	}

	const char* sound = NULL;
	sound = cls->GetClassData().GetString( va( "snd_pain_%s", size ) );

	if ( *sound ) {
		const idSoundShader* shader = gameLocal.declSoundShaderType[ sound ];
		if ( shader ) {
			StartSoundShader( shader, SND_VOICE, 0, NULL );
		} else {
			gameLocal.Warning( "idPlayer::PlayPain Missing Sound '%s'", sound );
		}
	}
}

/*
============
idPlayer::Event_GetClassKey
============
*/
void idPlayer::Event_GetClassKey( const char* key ) {
	const char* ret = GetInventory().GetClass() ? GetInventory().GetClass()->GetClassData().GetString( key ) : "";
	sdProgram::ReturnString( ret );
}


/*
============
idPlayer::Event_CreateIcon
============
*/
void idPlayer::Event_CreateIcon( const char* materialName, int priority, float timeout ) {
	sdProgram::ReturnInteger( playerIcon.CreateIcon( gameLocal.declMaterialType[ materialName ], priority, SEC2MS( timeout ) ) );
}

/*
============
idPlayer::Event_FreeIcon
============
*/
void idPlayer::Event_FreeIcon( qhandle_t handle ) {
	playerIcon.FreeIcon( handle );
}

/*
================
idPlayer::ApplyNetworkState
================
*/
void idPlayer::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdPlayerNetworkData );
		NET_APPLY_STATE_SCRIPT

		// Set new state
		if ( newData.hasPhysicsData ) {
			physicsObj.ApplyNetworkState( mode, newData.physicsData );
		}

		deltaViewAngles[ 0 ]	= SHORT2ANGLE( newData.deltaViewAngles[ 0 ] );
		deltaViewAngles[ 1 ]	= SHORT2ANGLE( newData.deltaViewAngles[ 1 ] );
		deltaViewAngles[ 2 ]	= 0.f;

		viewAngles[ 0 ]			= SHORT2ANGLE( newData.viewAngles[ 0 ] );
		viewAngles[ 1 ]			= SHORT2ANGLE( newData.viewAngles[ 1 ] );
		viewAngles[ 2 ]			= 0.f;

		if ( ShouldReadPlayerState() ) {
			ApplyPlayerStateData( newData.playerStateData );
		}

		idMat3 newViewAxis = viewAngles.ToMat3();
		OnNewAxesRead( newViewAxis );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPlayerBroadcastData );
		NET_APPLY_STATE_SCRIPT

		int oldHealth				= health;
		bool oldForceRespawn		= playerFlags.forceRespawn;

		physicsObj.ApplyNetworkState( mode, newData.physicsData );

		GetInventory().ApplyNetworkState( newData.inventoryData );

		// update state
		maxHealth					= newData.maxHealth;
		SetHealth( newData.health );

		SetLagged( newData.isLagged );
		playerFlags.forceRespawn	= newData.forceRespawn;
		playerFlags.wantSpawn		= newData.wantSpawn;
		baseDeathYaw				= newData.baseDeathYaw;

		SetTargetEntity( gameLocal.EntityForSpawnId( newData.targetLockSpawnId ) );
		targetLockEndTime			= newData.targetLockEndTime;

		if ( playerFlags.forceRespawn != oldForceRespawn ) {
			if ( playerFlags.forceRespawn && !oldForceRespawn ) {
				OnForceRespawn( true );
			}
			physicsObj.SetupPlayerClipModels();
		}

		if ( oldHealth > 0 && health <= 0 ) {
			OnKilled( NULL, NULL, oldHealth - health, vec3_zero, -1 );
		} else if ( health < oldHealth && health > 0 ) {
			if ( ShouldReadPlayerState() ) {
				lastDamageDecl				= newData.playerStateData.lastDamageDecl;
				lastDamageDir				= idBitMsg::BitsToDir( newData.playerStateData.lastDamageDir, 9 );

				// damage feedback
				const sdDeclDamage* damageDecl = gameLocal.declDamageType.LocalFindByIndex( lastDamageDecl, true );
				if ( damageDecl ) {
					gameLocal.playerView.DamageImpulse( lastDamageDir * viewAxis.Transpose(), damageDecl );
					Pain( NULL, NULL, oldHealth - health, lastDamageDir, -1, damageDecl );
				}
			}

			lastDmgTime = gameLocal.time;
		}

		if ( ShouldReadPlayerState() ) {
			ApplyPlayerStateBroadcast( newData.playerStateData );
		}

		return;
	}
}

/*
================
idPlayer::ResetNetworkState
================
*/
void idPlayer::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPlayerBroadcastData );

		physicsObj.ResetNetworkState( mode, newData.physicsData );
		return;
	}
}

/*
============
idPlayer::ReadNetworkState
============
*/
void idPlayer::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdPlayerNetworkData );
		NET_READ_STATE_SCRIPT

		newData.hasPhysicsData = msg.ReadBool();
		if ( newData.hasPhysicsData ) {
			physicsObj.ReadNetworkState( mode, baseData.physicsData, newData.physicsData, msg );
		} else {
			newData.physicsData = baseData.physicsData;
		}

		// Read new state
		newData.deltaViewAngles[ 0 ]	= msg.ReadDeltaShort( baseData.deltaViewAngles[ 0 ] );
		newData.deltaViewAngles[ 1 ]	= msg.ReadDeltaShort( baseData.deltaViewAngles[ 1 ] );

		newData.viewAngles[ 0 ]			= msg.ReadDeltaShort( baseData.viewAngles[ 0 ] );
		newData.viewAngles[ 1 ]			= msg.ReadDeltaShort( baseData.viewAngles[ 1 ] );

		if ( ShouldReadPlayerState() ) {
			ReadPlayerStateData( baseData.playerStateData, newData.playerStateData, msg );
		} else {
			newData.playerStateData = baseData.playerStateData;
		}

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPlayerBroadcastData );
		NET_READ_STATE_SCRIPT

		int oldHealth			= health;
		bool oldForceRespawn	= playerFlags.forceRespawn;

		physicsObj.ReadNetworkState( mode, baseData.physicsData, newData.physicsData, msg );

		newData.health				= ( short )msg.ReadDeltaShort( baseData.health );
		newData.maxHealth			= ( short )msg.ReadDeltaShort( baseData.maxHealth );

		newData.targetLockSpawnId	= msg.ReadDeltaLong( baseData.targetLockSpawnId );
		newData.targetLockEndTime	= msg.ReadDeltaLong( baseData.targetLockEndTime );

		newData.isLagged			= msg.ReadBool();
		newData.forceRespawn		= msg.ReadBool();
		newData.wantSpawn			= msg.ReadBool();

		newData.baseDeathYaw		= msg.ReadDeltaFloat( baseData.baseDeathYaw );

		GetInventory().ReadNetworkState( baseData.inventoryData, newData.inventoryData, msg );

		if ( ShouldReadPlayerState() ) {
			ReadPlayerStateBroadcast( baseData.playerStateData, newData.playerStateData, msg );
		} else {
			newData.playerStateData = baseData.playerStateData;
		}

		return;
	}
}

/*
============
idPlayer::WriteNetworkState
============
*/
void idPlayer::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_ON_VISIBLE {
		NET_GET_STATES( sdPlayerNetworkData );
		NET_WRITE_STATE_SCRIPT

		if ( GetProxyEntity() ) {
			msg.WriteBool( false );
			newData.hasPhysicsData = false;
			newData.physicsData = baseData.physicsData;
		} else {
			msg.WriteBool( true );
			newData.hasPhysicsData = true;
			physicsObj.WriteNetworkState( mode, baseData.physicsData, newData.physicsData, msg );
		}

		// Store new state
		newData.deltaViewAngles[ 0 ]	= ANGLE2SHORT( deltaViewAngles[ 0 ] );
		newData.deltaViewAngles[ 1 ]	= ANGLE2SHORT( deltaViewAngles[ 1 ] );
		newData.viewAngles[ 0 ]			= ANGLE2SHORT( viewAngles[ 0 ] );
		newData.viewAngles[ 1 ]			= ANGLE2SHORT( viewAngles[ 1 ] );
		newData.proxyEntitySpawnId		= gameLocal.GetSpawnId( GetProxyEntity() );

		// Write new state
		msg.WriteDeltaShort( baseData.deltaViewAngles[ 0 ], newData.deltaViewAngles[ 0 ] );
		msg.WriteDeltaShort( baseData.deltaViewAngles[ 1 ], newData.deltaViewAngles[ 1 ] );
		msg.WriteDeltaShort( baseData.viewAngles[ 0 ], newData.viewAngles[ 0 ] );
		msg.WriteDeltaShort( baseData.viewAngles[ 1 ], newData.viewAngles[ 1 ] );

		if ( ShouldWritePlayerState() ) {
			WritePlayerStateData( baseData.playerStateData, newData.playerStateData, msg );
		} else {
			newData.playerStateData = baseData.playerStateData;
		}

		sdAntiLagManager::GetInstance().CreateBranch( this );
		return;
	}

	NET_ON_BROADCAST {
		NET_GET_STATES( sdPlayerBroadcastData );
		NET_WRITE_STATE_SCRIPT

		physicsObj.WriteNetworkState( mode, baseData.physicsData, newData.physicsData, msg );

		newData.health				= ( short )health;
		newData.maxHealth			= ( short )maxHealth;
		newData.targetLockSpawnId	= targetEntity.GetSpawnId();
		newData.targetLockEndTime	= targetLockEndTime;
		newData.isLagged			= playerFlags.isLagged;
		newData.forceRespawn		= playerFlags.forceRespawn;
		newData.wantSpawn			= playerFlags.wantSpawn;
		newData.baseDeathYaw		= baseDeathYaw;

		msg.WriteDeltaShort( baseData.health, newData.health );
		msg.WriteDeltaShort( baseData.maxHealth, newData.maxHealth );

		msg.WriteDeltaLong( baseData.targetLockSpawnId, newData.targetLockSpawnId );
		msg.WriteDeltaLong( baseData.targetLockEndTime, newData.targetLockEndTime );

		msg.WriteBool( newData.isLagged );
		msg.WriteBool( newData.forceRespawn );
		msg.WriteBool( newData.wantSpawn );

		msg.WriteDeltaFloat( baseData.baseDeathYaw, newData.baseDeathYaw );

		GetInventory().WriteNetworkState( baseData.inventoryData, newData.inventoryData, msg );

		if ( ShouldWritePlayerState() ) {
			WritePlayerStateBroadcast( baseData.playerStateData, newData.playerStateData, msg );
		} else {
			newData.playerStateData = baseData.playerStateData;
		}

		return;
	}
}

/*
============
idPlayer::CheckNetworkStateChanges
============
*/
bool idPlayer::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	NET_ON_VISIBLE {
		NET_GET_BASE( sdPlayerNetworkData );
		NET_CHECK_STATE_SCRIPT

		if ( baseData.proxyEntitySpawnId != gameLocal.GetSpawnId( GetProxyEntity() ) ) {
			return true;
		}

		if ( !GetProxyEntity() ) {
			if ( physicsObj.CheckNetworkStateChanges( mode, baseData.physicsData ) ) {
				return true;
			}
		}

		if ( baseData.deltaViewAngles[ 0 ] != ANGLE2SHORT( deltaViewAngles[ 0 ] ) ) {
			return true;
		}
		if ( baseData.deltaViewAngles[ 1 ] != ANGLE2SHORT( deltaViewAngles[ 1 ] ) ) {
			return true;
		}

		if ( baseData.viewAngles[ 0 ] != ANGLE2SHORT( viewAngles[ 0 ] ) ) {
			return true;
		}
		if ( baseData.viewAngles[ 1 ] != ANGLE2SHORT( viewAngles[ 1 ] ) ) {
			return true;
		}

		if ( ShouldWritePlayerState() ) {
			if ( CheckPlayerStateData( baseData.playerStateData ) ) {
				return true;
			}
		}

		return false;
	}

	NET_ON_BROADCAST {
		NET_GET_BASE( sdPlayerBroadcastData );
		NET_CHECK_STATE_SCRIPT

		if ( physicsObj.CheckNetworkStateChanges( mode, baseData.physicsData ) ) {
			return true;
		}

		NET_CHECK_FIELD( health, health );
		NET_CHECK_FIELD( maxHealth, maxHealth );

		NET_CHECK_FIELD( targetLockSpawnId, targetEntity.GetSpawnId() );
		NET_CHECK_FIELD( targetLockEndTime, targetLockEndTime );

		NET_CHECK_FIELD( isLagged,		playerFlags.isLagged );
		NET_CHECK_FIELD( forceRespawn,	playerFlags.forceRespawn ); // FIXME: Make Event
		NET_CHECK_FIELD( wantSpawn,		playerFlags.wantSpawn ); // FIXME: Make Event

		NET_CHECK_FIELD( baseDeathYaw, baseDeathYaw );

		if ( GetInventory().CheckNetworkStateChanges( baseData.inventoryData ) ) {
			return true;
		}

		if ( ShouldWritePlayerState() ) {
			if ( CheckPlayerStateBroadcast( baseData.playerStateData ) ) {
				return true;
			}
		}

		return false;
	}

	return false;
}

/*
============
idPlayer::CreateNetworkStructure
============
*/
sdEntityStateNetworkData* idPlayer::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdPlayerNetworkData();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdPlayerBroadcastData();
	}
	return NULL;
}

/*
============
idPlayer::OnSnapshotHitch
============
*/
void idPlayer::OnSnapshotHitch() {
	suppressPredictionReset = false;
	ResetPredictionErrorDecay();
}

/*
============
idPlayer::ApplyPlayerStateData
============
*/
void idPlayer::ApplyPlayerStateData( const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdPlayerStateData );

	stepUpDelta			= newData.stepUpDelta;
	stepUpTime			= newData.stepUpTime;

	for ( int i = 0; i < newData.timers.Num(); i++ ) {
		qhandle_t handle = gameLocal.GetTargetTimerForServerHandle( i );
		if ( handle == -1 ) {
			gameLocal.Warning( "idPlayer::ApplyPlayerStateData Time Set For Unknown Timer" );
		} else {
			gameLocal.SetTargetTimer( handle, this, newData.timers[ i ] );
		}
	}
}

/*
============
idPlayer::ReadPlayerStateData
============
*/
void idPlayer::ReadPlayerStateData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerStateData );

	newData.stepUpTime		= msg.ReadDeltaLong( baseData.stepUpTime );
	newData.stepUpDelta		= msg.ReadDeltaFloat( baseData.stepUpDelta );

	newData.timers.SetNum( gameLocal.numServerTimers );
	if ( !msg.ReadBool() ) {
		newData.timers = baseData.timers;
	} else {
		for ( int i = 0; i < gameLocal.numServerTimers; i++ ) {
			if ( i < baseData.timers.Num() ) {
				newData.timers[ i ] = msg.ReadDeltaLong( baseData.timers[ i ] );
			} else {
				newData.timers[ i ] = msg.ReadLong();
			}
		}
	}
}

/*
============
idPlayer::WritePlayerStateData
============
*/
void idPlayer::WritePlayerStateData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerStateData );

	newData.stepUpDelta		= stepUpDelta;
	newData.stepUpTime		= stepUpTime;

	msg.WriteDeltaLong( baseData.stepUpTime, newData.stepUpTime );
	msg.WriteDeltaFloat( baseData.stepUpDelta, newData.stepUpDelta );

	bool changed = true;
	if ( baseData.timers.Num() == gameLocal.targetTimers.Num() ) {
		changed = false;
		for ( int i = 0; i < gameLocal.targetTimers.Num(); i++ ) {
			if ( baseData.timers[ i ] == gameLocal.targetTimers[ i ].endTimes[ entityNumber ] ) {
				continue;
			}
			changed = true;
			break;
		}
	}

	newData.timers.SetNum( gameLocal.targetTimers.Num() );

	if ( changed ) {
		msg.WriteBool( true );
		for ( int i = 0; i < gameLocal.targetTimers.Num(); i++ ) {
			newData.timers[ i ] = gameLocal.targetTimers[ i ].endTimes[ entityNumber ];
			if ( i < baseData.timers.Num() ) {
				msg.WriteDeltaLong( baseData.timers[ i ], newData.timers[ i ] );
			} else {
				msg.WriteLong( newData.timers[ i ] );
			}
		}
	} else {
		msg.WriteBool( false );
		newData.timers = baseData.timers;
	}
}

/*
============
idPlayer::CheckPlayerStateData
============
*/
bool idPlayer::CheckPlayerStateData( const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdPlayerStateData );

	if ( baseData.timers.Num() != gameLocal.targetTimers.Num() ) {
		return true;
	}

	for ( int i = 0; i < gameLocal.targetTimers.Num(); i++ ) {
		if ( baseData.timers[ i ] != gameLocal.targetTimers[ i ].endTimes[ entityNumber ] ) {
			return true;
		}
	}

	if ( baseData.stepUpTime != stepUpTime ) {
		return true;
	}

	if ( baseData.stepUpDelta != stepUpDelta ) {
		return true;
	}

	return false;
}

/*
============
idPlayer::ApplyPlayerStateBroadcast
============
*/
void idPlayer::ApplyPlayerStateBroadcast( const sdEntityStateNetworkData& newState ) {
	NET_GET_NEW( sdPlayerStateBroadcast );

	if ( lastHitCounter != newData.lastHitCounter ) {
		lastHitCounter	= newData.lastHitCounter;
		lastHitEntity	= gameLocal.EntityForSpawnId( newData.lastHitEntity );
		lastHitHeadshot	= newData.lastHitHeadshot;

		SetLastDamageDealtTime( gameLocal.time );
	}

	GetInventory().ApplyPlayerState( newData.inventoryData );
}

/*
============
idPlayer::ReadPlayerStateBroadcast
============
*/
void idPlayer::ReadPlayerStateBroadcast( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerStateBroadcast );

	newData.lastHitCounter	= msg.ReadDeltaLong( baseData.lastHitCounter );
	if ( newData.lastHitCounter != baseData.lastHitCounter ) {
		newData.lastHitEntity	= msg.ReadDeltaLong( baseData.lastHitEntity );
		newData.lastHitHeadshot	= msg.ReadBool();
	} else {
		newData.lastHitEntity	= baseData.lastHitEntity;
		newData.lastHitHeadshot	= baseData.lastHitHeadshot;
	}

	newData.lastDamageDecl	= msg.ReadDelta( baseData.lastDamageDecl + 1, gameLocal.GetNumDamageDeclBits() ) - 1;
	newData.lastDamageDir	= msg.ReadDelta( baseData.lastDamageDir, 9 );

	ASYNC_SECURITY_READ( msg )
	GetInventory().ReadPlayerState( baseData.inventoryData, newData.inventoryData, msg );
	ASYNC_SECURITY_READ( msg )
}

/*
============
idPlayer::WritePlayerStateBroadcast
============
*/
void idPlayer::WritePlayerStateBroadcast( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdPlayerStateBroadcast );

	newData.lastHitCounter		= lastHitCounter;
	if ( newData.lastHitCounter != baseData.lastHitCounter ) {
		newData.lastHitEntity	= lastHitEntity.GetSpawnId();
		newData.lastHitHeadshot	= lastHitHeadshot;
	} else {
		newData.lastHitEntity	= baseData.lastHitEntity;
		newData.lastHitHeadshot	= baseData.lastHitHeadshot;
	}

	newData.lastDamageDecl	= lastDamageDecl;
	newData.lastDamageDir	= idBitMsg::DirToBits( lastDamageDir, 9 );

	msg.WriteDeltaLong( baseData.lastHitCounter, newData.lastHitCounter );
	if ( newData.lastHitCounter != baseData.lastHitCounter ) {
		msg.WriteDeltaLong( baseData.lastHitEntity, newData.lastHitEntity );
		msg.WriteBool( newData.lastHitHeadshot );
	}

	msg.WriteDelta( baseData.lastDamageDecl + 1, newData.lastDamageDecl + 1, gameLocal.GetNumDamageDeclBits() );
	msg.WriteDelta( baseData.lastDamageDir, newData.lastDamageDir, 9 );

	ASYNC_SECURITY_WRITE( msg )
	GetInventory().WritePlayerState( baseData.inventoryData, newData.inventoryData, msg );
	ASYNC_SECURITY_WRITE( msg )
}

/*
============
idPlayer::CheckPlayerStateBroadcast
============
*/
bool idPlayer::CheckPlayerStateBroadcast( const sdEntityStateNetworkData& baseState ) const {
	NET_GET_BASE( sdPlayerStateBroadcast );

	NET_CHECK_FIELD( lastHitCounter, lastHitCounter );
	NET_CHECK_FIELD( lastDamageDecl, lastDamageDecl );

	if ( idBitMsg::DirToBits( lastDamageDir, 9 ) != baseData.lastDamageDir ) {
		return true;
	}

	return GetInventory().CheckPlayerStateChanges( baseData.inventoryData );
}

/*
============
idPlayer::IsSpectating
============
*/
bool idPlayer::IsSpectating( void ) const {
	return IsInLimbo() || IsSpectator() || gameLocal.rules->IsEndGame();
}

/*
============
idPlayer::IsSpectator
============
*/
bool idPlayer::IsSpectator( void ) const {
	return team == NULL;
}

/*
============
idPlayer::Event_SetAmmo
============
*/
void idPlayer::Event_SetAmmo( ammoType_t ammoType, int amount ) {
	GetInventory().SetAmmo( ammoType, amount );
}

/*
============
idPlayer::Event_GetAmmo
============
*/
void idPlayer::Event_GetAmmo( ammoType_t ammoType ) {
	sdProgram::ReturnInteger( GetInventory().GetAmmo( ammoType ) );
}

/*
============
idPlayer::Event_SetMaxAmmo
============
*/
void idPlayer::Event_SetMaxAmmo( ammoType_t ammoType, int amount ) {
	GetInventory().SetMaxAmmo( ammoType, amount );
}

/*
============
idPlayer::Event_GetMaxAmmo
============
*/
void idPlayer::Event_GetMaxAmmo( ammoType_t ammoType ) {
	sdProgram::ReturnInteger( GetInventory().GetMaxAmmo( ammoType ) );
}

/*
============
idPlayer::Event_SetTargetTimeScale
============
*/
void idPlayer::Event_SetTargetTimeScale( float scale ) {
	targetLockTimeScale = scale;
}

/*
============
idPlayer::Event_DisableShadows
============
*/
void idPlayer::Event_DisableShadows( bool value ) {
	playerFlags.noShadow = value;
	UpdateShadows();
}

/*
============
idPlayer::Event_GetRemoteCamera
============
*/
void idPlayer::Event_GetRemoteCamera( void ) {
	sdProgram::ReturnEntity( remoteCamera );
}

/*
============
idPlayer::Event_DisableSprint
============
*/
void idPlayer::Event_DisableSprint( bool disable ) {
	playerFlags.sprintDisabled = disable;
}

/*
============
idPlayer::Event_DisableSprint
============
*/
void idPlayer::Event_DisableRun( bool disable ) {
	playerFlags.runDisabled = disable;
}

/*
============
idPlayer::Event_DisableFootsteps
============
*/
void idPlayer::Event_DisableFootsteps( bool disable ) {
	playerFlags.noFootsteps = disable;
}


/*
============
idPlayer::Event_DisableFallingDamage
============
*/
void idPlayer::Event_DisableFallingDamage( bool disable ) {
	playerFlags.noFallingDamage = disable;
}

/*
============
idPlayer::Event_Disguise
============
*/
void idPlayer::Event_Disguise( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( other == NULL ) {
		SetDisguised( -1, NULL, NULL, NULL, NULL, NULL );
		return;
	}

	sdPlayerBody* body = other->Cast< sdPlayerBody >();
	if ( body != NULL ) {
		idPlayer* player = body->GetClient();

		if ( player != NULL ) {
			SetDisguised( gameLocal.GetSpawnId( player ), body->GetPlayerClassSetup()->GetClass(), body->GetGameTeam(), body->GetRank(), body->GetRating(), player->userInfo.name.c_str() );
		}

		return;
	}

	gameLocal.Warning( "idPlayer::Event_Disguise Invalid Entity!" );
}

/*
============
idPlayer::Event_IsDisguised
============
*/
void idPlayer::Event_IsDisguised( void ) {
	sdProgram::ReturnBoolean( disguiseClient != -1 );
}

/*
============
idPlayer::Event_GetDisguiseClient
============
*/
void idPlayer::Event_GetDisguiseClient( void ) {
	if ( disguiseClient == -1 ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	sdProgram::ReturnEntity( gameLocal.EntityForSpawnId( disguiseClient ) );
}

/*
============
idPlayer::Event_SetSpectateClient
============
*/
void idPlayer::Event_SetSpectateClient( idEntity* other ) {
	if ( !other ) {
		SetSpectateClient( NULL );
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		gameLocal.Warning( "idPlayer::Event_SetSpectateClient Can Only Spectate Another Player!" );
		return;
	}
	SetSpectateClient( player );
}

/*
============
idPlayer::IsProne
============
*/
bool idPlayer::IsProne( void ) const {
	return physicsObj.IsProne();
}

/*
============
idPlayer::IsCrouching
============
*/
bool idPlayer::IsCrouching( void ) const {
	return physicsObj.IsCrouching();
}


/*
============
idPlayer::InhibitProne
============
*/
bool idPlayer::InhibitProne( void ) const {
	const sdInventory& inv = GetInventory();

	const sdDeclInvItem* item = inv.GetItem( inv.GetIdealWeapon() );
	if ( item != NULL ) {
		return !item->GetAllowProne();
	}

	return false;
}

/*
============
idPlayer::GetViewClient
============
*/
idPlayer* idPlayer::GetViewClient( void ) {
	idPlayer* player = GetSpectateClient();
	return player ? player : this;
}

/*
============
idPlayer::GetSpectateClient
============
*/
idPlayer* idPlayer::GetSpectateClient( void ) const {
	return gameLocal.GetClient( spectateClient );
}

/*
============
idPlayer::GetSpectateClientId
============
*/
int idPlayer::GetSpectateClientId( void ) {
	return spectateClient;
}

/*
============
idPlayer::SetSpectateClient
============
*/
void idPlayer::SetSpectateClient( idPlayer* player ) {
	if ( !player ) {
		SetSpectateId( entityNumber );
	} else {
		SetSpectateId( player->entityNumber );
	}
}

/*
============
idPlayer::SetSpectateId
============
*/
void idPlayer::SetSpectateId( int id ) {
	if ( spectateClient == id ) {
		return;
	}

	if ( gameLocal.IsLocalPlayer( this ) ) {
		gameLocal.localPlayerProperties.SetActiveWeapon( NULL );
		gameLocal.localPlayerProperties.SetActivePlayer( NULL );
	}

	spectateClient = id;

	if ( gameLocal.IsLocalPlayer( this ) ) {
		if ( bot_spectateDebug.GetBool() ) {
			cvarSystem->SetCVarInteger( "bot_hud", id );
			bot_showPath.SetInteger( id );
		}

		gameLocal.OnLocalViewPlayerChanged();

		// ammo values might not have changed, explicitly set them
		if ( gameLocal.isClient ) {
			clientNetworkInfo_t& networkInfo = gameLocal.GetNetworkInfo( entityNumber );
			if ( networkInfo.states[ spectateClient ][ NSM_BROADCAST ] != NULL ) {
				idPlayer* viewPlayer = gameLocal.GetClient( spectateClient );
				if ( viewPlayer != NULL )  {
					sdPlayerBroadcastData* broadCast = static_cast< sdPlayerBroadcastData* >( networkInfo.states[ spectateClient ][ NSM_BROADCAST ]->data );
					sdInventoryPlayerStateData& invState = broadCast->playerStateData.inventoryData;
					sdInventory& inv = viewPlayer->GetInventory();
					for ( int i = 0; i < invState.ammo.Num(); i++ ) {
						inv.SetAmmo( i, invState.ammo[ i ] );
					}
				}
			}
		}
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SPECTATE );
		msg.WriteBits( spectateClient, idMath::BitsForInteger( MAX_CLIENTS ) );
		msg.Send( spectateClient != entityNumber, sdReliableMessageClientInfoAll() );
	}

	physicsObj.SetupPlayerClipModels();

	idWeapon::UpdateWeaponVisibility();
}

/*
============
idPlayer::OnSetClientSpectatee
============
*/
void idPlayer::OnSetClientSpectatee( idPlayer* spectatee ) {
	if ( gameLocal.IsPaused() ) {
		return;
	}

	assert( gameLocal.isServer );
	if ( spectatee != NULL ) {
		if ( IsSpectator() ) {
			SetSpectateClient( spectatee );
		} else {
			if ( IsInLimbo() && spectatee->GetGameTeam() == team ) {
				SetSpectateClient( spectatee );
			}
		}
	}
}

/*
============
idPlayer::SetUserGroup
============
*/
void idPlayer::SetUserGroup( qhandle_t group ) {
	qhandle_t currentGroup = GetUserGroup();
	if ( currentGroup == group ) {
		return;
	}

	gameLocal.rules->SetPlayerUserGroup( entityNumber, group );

	if ( group != -1 ) {
		const sdUserGroup& theGroup = sdUserGroupManager::GetInstance().GetGroup( group );

		if ( theGroup.HasPermission( PF_QUIET_LOGIN ) ) {
			return;
		}

		gameSoundWorld->PlayShaderDirectly( gameLocal.declSoundShaderType[ spawnArgs.GetString( "snd_login" ) ] );

		if ( gameLocal.IsLocalPlayer( this ) ) {
			gameLocal.Printf( "You have joined the '%s' user group\n", theGroup.GetName() );
		} else {
			gameLocal.Printf( "%s has joined the '%s' user group\n", userInfo.name.c_str(), theGroup.GetName() );
		}
	}
}

/*
============
idPlayer::GetUserGroup
============
*/
qhandle_t idPlayer::GetUserGroup( void ) {
	return gameLocal.rules->GetPlayerUserGroup( entityNumber );
}

/*
============
idPlayer::Obituary
============
*/
void idPlayer::Obituary( idEntity* killerEnt, const sdDeclDamage* damageDecl ) {
	idPlayer* killerPlayer = killerEnt->Cast< idPlayer >();

	gameLocal.localPlayerProperties.OnObituary( this, killerPlayer );

	if ( killerPlayer != this ) {
		killer = killerPlayer;
		// hardcoded,
		killerTime = gameLocal.time + 5000;
	}

	const sdDeclToolTip* obituary;
	if ( killerPlayer == NULL ) {
		if ( killerEnt != NULL && team == killerEnt->GetGameTeam() && damageDecl->GetUnknownFriendlyObituary() != NULL ) {
			obituary = damageDecl->GetUnknownFriendlyObituary();
		} else {
			obituary = damageDecl->GetUnknownObituary();
		}
		if ( obituary == NULL ) {
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_OBITUARY, -1, va( L"%hs killed by unknown (UNSPECIFIED METHOD OF DEATH on %hs)", userInfo.name.c_str(), damageDecl->GetName() ) );
			return;
		}
	} else if ( killerPlayer == this ) {
		obituary = damageDecl->GetSelfObituary();
		if ( obituary == NULL ) {
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_OBITUARY, -1, va( L"%hs killed self (UNSPECIFIED METHOD OF DEATH on %hs)", userInfo.name.c_str(), damageDecl->GetName() ) );
			return;
		}
	} else if ( GetEntityAllegiance( killerPlayer ) == TA_FRIEND ) {
		obituary = damageDecl->GetTeamKillObituary();
		if ( obituary == NULL ) {
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_OBITUARY, -1, va( L"%hs" WS_COLOR_RED L" was team killed by " WS_COLOR_DEFAULT L" %hs " WS_COLOR_DEFAULT L"(UNSPECIFIED METHOD OF DEATH on %hs)", userInfo.name.c_str(), killerPlayer->userInfo.name.c_str(), damageDecl->GetName() ) );
			return;
		}
	} else {
		obituary = damageDecl->GetObituary();
		if ( obituary == NULL ) {
			gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_OBITUARY, -1, va( L"%hs died (UNSPECIFIED METHOD OF DEATH on %hs)", userInfo.name.c_str(), damageDecl->GetName() ) );
			return;
		}
	}

	sdToolTipParms parms;
	parms.Add( va( L"%hs", userInfo.name.c_str() ) );
	parms.Add( killerPlayer != NULL ? va( L"%hs", killerPlayer->userInfo.name.c_str() ) : L"<UNKNOWN>" );

	idWStr text;
	obituary->GetMessage( &parms, text );
	gameLocal.rules->AddChatLine( vec3_origin, sdGameRules::CHAT_MODE_OBITUARY, -1, text.c_str() );
}

/*
============
idPlayer::DropDisguise
============
*/
void idPlayer::DropDisguise( void ) {
	if ( disguiseClient == -1 ) {
		return;
	}

	SetDisguised( -1, NULL, NULL, NULL, NULL, NULL );
}

/*
============
idPlayer::SetDisguised
============
*/
void idPlayer::SetDisguised( int disguiseSpawnId, const sdDeclPlayerClass* playerClass, sdTeamInfo* playerTeam, const sdDeclRank* rank, const sdDeclRating* rating, const char* name ) {
	if ( disguiseSpawnId == -1 ) {
		disguiseClient = -1;
		disguiseClass = NULL;
		disguiseTeam = NULL;
		disguiseRank = NULL;
		disguiseRating = NULL;
		disguiseUserName.Clear();
	} else {
		disguiseClient = disguiseSpawnId;
		disguiseClass = playerClass;
		disguiseTeam = playerTeam;
		disguiseRank = rank;
		disguiseRating = rating;
		disguiseUserName = name;
	}

	StopFiring();

	idPlayer* other = gameLocal.EntityForSpawnId( disguiseSpawnId )->Cast< idPlayer >();

	sdScriptHelper h1;
	h1.Push( other ? other->GetScriptObject() : NULL );
	scriptObject->CallNonBlockingScriptEvent( onDisguisedFunction, h1 );

	GetInventory().UpdateForDisguise();

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_DISGUISE );
		msg.WriteLong( disguiseSpawnId );
		msg.WriteLong( disguiseRank == NULL ? -1 : disguiseRank->Index() );
		msg.WriteLong( disguiseRating == NULL ? -1 : disguiseRating->Index() );
		msg.WriteString( disguiseUserName.c_str() );
		if ( disguiseClass != NULL ) {
			msg.WriteBits( disguiseClass->Index() + 1, gameLocal.GetNumPlayerClassBits() );
		} else {
			msg.WriteBits( 0, gameLocal.GetNumPlayerClassBits() );
		}
		sdTeamManager::GetInstance().WriteTeamToStream( disguiseTeam, msg );

		msg.Send( disguiseSpawnId != -1, sdReliableMessageClientInfoAll() );
	}
}

/*
============
idPlayer::GetDisguiseEntity
============
*/
idEntity* idPlayer::GetDisguiseEntity( void ) {
	if ( disguiseClient != -1 ) {
		idPlayer* disguisePlayer = gameLocal.EntityForSpawnId( disguiseClient )->Cast< idPlayer >();
		if ( disguisePlayer != NULL ) {
			return disguisePlayer;
		}
	}

	return this;
}


/*
============
idPlayer::Event_SetViewSkin
============
*/
void idPlayer::Event_SetViewSkin( const char *name ) {
	viewSkin = declHolder.declSkinType.LocalFind( name, false );
}

/*
============
idPlayer::Event_GetViewSkin
============
*/
void idPlayer::Event_GetViewSkin( void ) {
	if ( viewSkin ) {
		sdProgram::ReturnString( viewSkin->GetName() );
	} else {
		sdProgram::ReturnString( "" );
	}
}

/*
============
idPlayer::Event_SetGUIClipIndex
============
*/
void idPlayer::Event_SetGUIClipIndex( int index ) {
	if ( gameLocal.IsLocalViewPlayer( this ) ) {
		gameLocal.localPlayerProperties.SetClipIndex( index );
	}
}

/*
===============
idPlayer::Event_GetDeploymentRequest
===============
*/
void idPlayer::Event_GetDeploymentRequest( void ) {
	sdDeployRequest* request = gameLocal.GetDeploymentRequest( this );
	if ( !request ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdProgram::ReturnInteger( request->GetObject()->Index() );
}

/*
===============
idPlayer::Event_GetDeploymentPosition
===============
*/
void idPlayer::Event_GetDeploymentPosition( void ) {
	idVec3 out;
	GetDeployPosition( out );
	sdProgram::ReturnVector( out );
}

/*
===============
idPlayer::ConsumeHealth
===============
*/
void idPlayer::ConsumeHealth( void ) {
	sdScriptHelper helper;
	CallNonBlockingScriptEvent( onConsumeHealthFunction, helper );
}

/*
===============
idPlayer::ConsumeStroyent
===============
*/
void idPlayer::ConsumeStroyent( void ) {
	sdScriptHelper helper;
	CallNonBlockingScriptEvent( onConsumeStroyentFunction, helper );
}

/*
===============
idPlayer::Event_Hide
===============
*/
void idPlayer::Event_Hide( void ) {
	playerFlags.scriptHide = true;
}

/*
===============
idPlayer::Event_Show
===============
*/
void idPlayer::Event_Show( void ) {
	playerFlags.scriptHide = false;
}

/*
===============
idPlayer::Event_GetActiveTask
===============
*/
void idPlayer::Event_GetActiveTask( void ) {
	sdPlayerTask* task = activeTask.IsValid() ? sdTaskManager::GetInstance().TaskForHandle( activeTask ) : NULL;
	sdProgram::ReturnObject( task ? task->GetScriptObject() : NULL );
}

/*
============
idPlayer::ZoomInCommandMap_f
============
*/
void idPlayer::ZoomInCommandMap_f( const idCmdArgs& args ) {
	g_commandMapZoom.SetFloat( g_commandMapZoom.GetFloat() - g_commandMapZoomStep.GetFloat() );
}

/*
============
idPlayer::ZoomOutCommandMap_f
============
*/
void idPlayer::ZoomOutCommandMap_f( const idCmdArgs& args ) {
	g_commandMapZoom.SetFloat( g_commandMapZoom.GetFloat() + g_commandMapZoomStep.GetFloat() );
}

/*
============
idPlayer::SetupCommandMapZoom
============
*/
void idPlayer::SetupCommandMapZoom( void ) {
	// Gordon: FIXME: This should just be on localPlayerProperties
	sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" );
	if ( scope == NULL ) {
		gameLocal.Warning( "idPlayer::ZoomInCommandMap: Couldn't find global 'gameHud' scope in guiGlobals." );
		return;
	}

	sdProperties::sdProperty* prop = scope->GetProperty( "newMapZoomLevel", sdProperties::PT_FLOAT );
	if ( prop != NULL ) {
		*prop->value.floatValue = g_commandMapZoom.GetFloat();
	}
}

/*
============
idPlayer::BinAddEntity
============
*/
void idPlayer::BinAddEntity( idEntity* other ) {
	BinCleanup();

	if ( other == NULL ) {
		return;
	}

	if ( entityBin.FindIndex( other ) != -1 ) {
		return;
	}

	if ( entityBin.Append( other ) == -1 ) {
		gameLocal.Error( "idPlayer::BinAddEntity - entityBin exceeded MAX_PLAYER_BIN_SIZE!" );
	}

	if ( other->SupportsAbilities() ) {
		if ( abilityEntityBin.Append( other ) == -1 ) {
			gameLocal.Error( "idPlayer::BinAddEntity - abilityEntityBin exceeded MAX_PLAYER_BIN_SIZE!" );
		}
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent evtMsg( this, EVENT_BINADD );
		evtMsg.WriteLong( gameLocal.GetSpawnId( other ) );
		evtMsg.Send( false, sdReliableMessageClientInfoAll() );
	}
}

/*
============
idPlayer::BinRemoveEntity
============
*/
void idPlayer::BinRemoveEntity( idEntity* other ) {
	BinCleanup();

	abilityEntityBin.RemoveFast( other );

	int index = entityBin.FindIndex( other );
	if ( index == -1 ) {
		return;
	}
	entityBin.RemoveIndexFast( index );

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent evtMsg( this, EVENT_BINREMOVE );
		evtMsg.WriteLong( gameLocal.GetSpawnId( other ) );
		evtMsg.Send( false, sdReliableMessageClientInfoAll() );
	}
}

/*
============
idPlayer::Event_BinAdd
============
*/
void idPlayer::Event_BinAdd( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	BinAddEntity( other );
}

/*
============
idPlayer::Event_BinGet
============
*/
void idPlayer::Event_BinGet( int index ) {
	if ( index < 0 || index >= entityBin.Num() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	idEntity* returnVal = entityBin[ index ];
	if ( returnVal == NULL ) {
		BinCleanup();
	}

	sdProgram::ReturnEntity( returnVal );
}

/*
============
idPlayer::Event_BinGetSize
============
*/
void idPlayer::Event_BinGetSize( void ) {
	BinCleanup();
	sdProgram::ReturnInteger( entityBin.Num() );
}

/*
============
idPlayer::Event_BinRemove
============
*/
void idPlayer::Event_BinRemove( idEntity* other ) {
	if ( gameLocal.isClient ) {
		return;
	}

	BinRemoveEntity( other );
}

/*
============
idPlayer::Event_SetArmor
============
*/
void idPlayer::Event_SetArmor( float _armor ) {
	armor = idMath::ClampFloat( 0.f, 1.f, _armor );
}

/*
============
idPlayer::Event_SetArmor
============
*/
void idPlayer::Event_GetArmor( void ) {
	sdProgram::ReturnFloat( armor );
}

/*
=========================
idPlayer::Event_InhibitGuis
=========================
*/
void idPlayer::Event_InhibitGuis( bool inhibit ) {
	SetInhibitGuis( inhibit );
}

/*
=========================
idPlayer::Event_GetPostArmFindBestWeapon
=========================
*/
void idPlayer::Event_GetPostArmFindBestWeapon( void ) {
	sdProgram::ReturnBoolean( userInfo.postArmFindBestWeapon );
}

/*
=========================
idPlayer::Event_SetForceShieldState

Let the bots know a shield exists(?) in the world.
=========================
*/
void idPlayer::Event_SetForceShieldState( bool destroy, idEntity* self ) {
	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setForceShieldState\"!\n");
		}
		return;
	}

	if ( destroy == false ) {
		for( int i = 0; i < MAX_SHIELDS; i++ ) {

			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].spawnID = gameLocal.GetSpawnId( self );
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].origin = self->GetPhysics()->GetOrigin();
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].lastShieldDroppedTime = gameLocal.time;
			break;
		}
	} else {
		for( int i = 0; i < MAX_SHIELDS; i++ ) {

			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].entNum = 0;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].spawnID = -1;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].forceShields[ i ].origin = vec3_zero;
			break;
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerChargeArmed

Let the bots know if this charge has been armed or not.
=========================
*/
void idPlayer::Event_SetPlayerChargeArmed( bool chargeArmed, idEntity* self ) {

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerChargeOrigin\"!\n");
		}
		return;
	}

 	if ( chargeArmed == false ) {
		for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
			if ( botThreadData.GetGameWorldState()->chargeInfo[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
				continue;
			}

			botThreadData.GetGameWorldState()->chargeInfo[ i ].state = BOMB_NULL;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].origin = vec3_zero;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].entNum = 0;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].team = NOTEAM;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].ownerSpawnID = -1;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].spawnID = -1;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].ownerEntNum = -1;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].checkedAreaNum = false;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].areaNum = 0;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].isOnObjective = false;
			break;
		}
	} else {
		for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
			if ( botThreadData.GetGameWorldState()->chargeInfo[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
				continue;
			}

			botThreadData.GetGameWorldState()->chargeInfo[ i ].state = BOMB_ARMED;
			botThreadData.GetGameWorldState()->chargeInfo[ i ].explodeTime = gameLocal.time + ( ( botThreadData.GetGameWorldState()->gameLocalInfo.chargeExplodeTime - TIME_BEFORE_CHARGE_BLOWS_AWARENESS_TIME ) * 1000 );
			break;
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerBombOrigin
=========================
*/
void idPlayer::Event_SetPlayerChargeOrigin( idEntity* self ) {

	bool foundSlot = false;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("A NULL entity was passed to SetPlayerChargeOrigin!\n");
		}
		return;
	}

	for( int i = 0; i < MAX_CLIENT_CHARGES; i++ ) {
		if ( botThreadData.GetGameWorldState()->chargeInfo[ i ].entNum != 0 ) {
			continue;
		}

		botThreadData.GetGameWorldState()->chargeInfo[ i ].origin = self->GetPhysics()->GetOrigin();
		botThreadData.GetGameWorldState()->chargeInfo[ i ].entNum = self->entityNumber;
		botThreadData.GetGameWorldState()->chargeInfo[ i ].state = BOMB_NULL;
		botThreadData.GetGameWorldState()->chargeInfo[ i ].explodeTime = 0;
		botThreadData.GetGameWorldState()->chargeInfo[ i ].bbox = self->GetPhysics()->GetAbsBounds();
		botThreadData.GetGameWorldState()->chargeInfo[ i ].team = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team;
		botThreadData.GetGameWorldState()->chargeInfo[ i ].ownerSpawnID = gameLocal.GetSpawnId( this );
		botThreadData.GetGameWorldState()->chargeInfo[ i ].spawnID = gameLocal.GetSpawnId( self );
		botThreadData.GetGameWorldState()->chargeInfo[ i ].ownerEntNum = entityNumber;
		foundSlot = true;
		break;
	}

	if ( foundSlot == false ) { //mal: this should NEVER happen, but just in case.....
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("No open slot available for \"setPlayerChargeOrigin\"!\n");
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerItemState

Tracks the birth/death of the players items ( ammo/health/supply packs ).
=========================
*/
void idPlayer::Event_SetPlayerItemState( idEntity *self, bool destroy ) {

	bool foundSlot = false;
	int i, j;

	if ( self == NULL ) {
		gameLocal.DWarning("Invalid item passed to \"SetPlayerItemState\"!\n"); //mal: shouldn't happen, but just in case.....
		return;
	}

	if ( destroy == false ) { //mal: I'm a brand new health/ammo/supply pack, born into the world.
		for( i = 0; i < MAX_ITEMS; i++ ) {
			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].spawnID = gameLocal.GetSpawnId( self );
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].team = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].areaNum = 0;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].checkedAreaNum = false;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].available = true;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ i ].inPlayZone = false;
			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) { //mal: this should never happen, but just in case ...
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].team = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].areaNum = 0;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].checkedAreaNum = false;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].spawnID = gameLocal.GetSpawnId( self );
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].available = true;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].packs[ 0 ].inPlayZone = false;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("No open slot available for \"setPlayerItemState\"!\n");
			}
		}
	} else { //mal: I've been picked up, or I expired. Find out who owns me, and remove me.
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

            for( i = 0; i < MAX_ITEMS; i++ ) {

				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].team = NOTEAM;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].areaNum = 0;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].checkedAreaNum = false;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].origin = vec3_zero;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].entNum = 0;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].xySpeed = 0.0f;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].spawnID = -1;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].available = false;
				botThreadData.GetGameWorldState()->clientInfo[ j ].packs[ i ].inPlayZone = false;
				foundSlot = true;
				break;
			}
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerGrenadeState

Tracks the birth/death of the players grenades ( EMP, Shrap, etc ).
=========================
*/
void idPlayer::Event_SetPlayerGrenadeState( idEntity *self, bool destroy ) {

	bool foundSlot = false;
	int i, j;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerGrenadeState\"!\n");
		}
		return;
	}

	if ( destroy == false ) { //mal: I'm a brand new grenade, born into the world.
		for( i = 0; i < MAX_GRENADES; i++ ) {
			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.grenades[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.grenades[ i ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.grenades[ i ].spawnID = gameLocal.GetSpawnId( self );
			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) { //mal: this should never happen, but just in case ...
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.grenades[ 0 ].entNum = self->entityNumber;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("No open slot available for \"setPlayerGrenadeState\"!\n");
			}
		}
	} else { //mal: I blew up. Find out who owns me, and remove me.
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

			if ( botThreadData.GetGameWorldState()->smokeGrenades[ j ].spawnID == gameLocal.GetSpawnId( self ) ) {
				botThreadData.GetGameWorldState()->smokeGrenades[ j ].entNum = 0;
				botThreadData.GetGameWorldState()->smokeGrenades[ j ].xySpeed = 0.0f;
				botThreadData.GetGameWorldState()->smokeGrenades[ j ].spawnID = -1;
				foundSlot = true;
				continue;
			}

			for( i = 0; i < MAX_GRENADES; i++ ) {
				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].origin = vec3_zero;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].entNum = 0;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].spawnID = -1;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].xySpeed = 0.0f;
				foundSlot = true;
				break;
			}
		}
	}

	if ( !foundSlot ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Can't find an open slot for exploding grenade in \"setPlayerGrenadeState\"!\n");
		}
	}
}

/*
=========================
idPlayer::Event_SetSmokeNadeState

Tracks the birth of the players smoke grenades.
=========================
*/
void idPlayer::Event_SetSmokeNadeState( idEntity *self ) {

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setSmokeNadeState\"!\n");
		}
		return;
	}

	bool foundSlot = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( botThreadData.GetGameWorldState()->smokeGrenades[ i ].entNum != 0 ) {
			continue;
		}

		if ( botThreadData.GetGameWorldState()->smokeGrenades[ i ].birthTime + SMOKE_GRENADE_LIFETIME > gameLocal.time ) {
			continue;
		}

		botThreadData.GetGameWorldState()->smokeGrenades[ i ].birthTime = gameLocal.time;
		botThreadData.GetGameWorldState()->smokeGrenades[ i ].entNum = self->entityNumber;
		botThreadData.GetGameWorldState()->smokeGrenades[ i ].spawnID = gameLocal.GetSpawnId( self );
		foundSlot = true;
		break;
	}

	if ( !foundSlot ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Can't find an open slot for smoke grenade in \"setSmokeNadeState\"!\n");
		}
	}
}

/*
=========================
idPlayer::Event_SetArtyAttackLocation
=========================
*/
void idPlayer::Event_SetArtyAttackLocation( const idVec3& vector, int artyType ) {

	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.birthTime = gameLocal.time;
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.origin = vector;
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.type = artyType;

	if ( artyType == TARGET_ARTILLERY ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.radius = 1024.0f;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.deathTime = 20000 + gameLocal.time;
	} else if ( artyType == TARGET_ROCKETS ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.radius = 512.0f;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.deathTime = 15000 + gameLocal.time;
	} else if (artyType == TARGET_SSM ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.radius = 2048.0f;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.artyAttackInfo.deathTime = 15000 + gameLocal.time;
	}
}

/*
=========================
idPlayer::Event_SetPlayerMineState

Tracks the birth/death of the players landmines.
=========================
*/
void idPlayer::Event_SetPlayerMineState( idEntity *self, bool destroy, bool spotted ) {

	bool foundSlot = false;
	int i, j;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerMineState\"!\n");
		}
		return;
	}

	if ( destroy == false && spotted == false ) { //mal: I'm a brand new landmine, born into the world.
		clientInfo_t& owner = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

		for( i = 0; i < MAX_MINES; i++ ) {
			if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].spawnID = gameLocal.GetSpawnId( self );

			if ( owner.abilities.selfArmingMines == false ) {
				botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].selfArming = false;
			} else {
				botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].selfArming = true;
			}

			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) { //mal: this should never happen, but just in case ...
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ 0 ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.landMines[ i ].spawnID = gameLocal.GetSpawnId( self );

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("No open slot available for \"setPlayerMineState\"!\n");
			}
		}
	} else if ( destroy == true ) { //mal: I blew up or expired. Find out who owns me, and remove me.
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

            for( i = 0; i < MAX_MINES; i++ ) {

				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].origin = vec3_zero;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum = 0;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].xySpeed = 0.0f;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].state = BOMB_NULL;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spotted = false;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID = -1;
				foundSlot = true;
				break;
			}
		}

		if ( foundSlot == false ) {
			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("Found no landmine that matches entity %i in \"setPlayerMineState\"!\n", self->entityNumber );
			}
		}

	} else if ( spotted == true ) {
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

            for( i = 0; i < MAX_MINES; i++ ) {

				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spotted = true;
				foundSlot = true;
				break;
			}
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerMineArmed

Let the bots know if this mine has been armed or not. Since mine may not have belonged to bot, check all clients to see who it belongs to.
=========================
*/
void idPlayer::Event_SetPlayerMineArmed( bool chargeArmed, idEntity *self, bool isVisible ) {

	bool foundSlot = false;
	int i, j;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerMineArmed\"!\n");
		}
		return;
	}

 	if ( chargeArmed == false ) {
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

            for( i = 0; i < MAX_MINES; i++ ) {

				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].state = BOMB_NULL;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].origin = vec3_zero;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum = 0;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spotted = false;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].xySpeed = 0.0f;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID = -1;
				foundSlot = true;
				break;
			}
		}
	} else {
		for( j = 0; j < MAX_CLIENTS; j++ ) {

			if ( foundSlot ) {
				break;
			}

            for( i = 0; i < MAX_MINES; i++ ) {

				if ( botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
					continue;
				}

				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].state = BOMB_ARMED;
				botThreadData.GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spotted = isVisible;
				foundSlot = true;
				break;
			}
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerCovertToolState

Let the bots know if there is an hive/3rd eye camera out there in the world.
=========================
*/
void idPlayer::Event_SetPlayerCovertToolState( idEntity *self, bool destroy ) {

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerCovertToolState\"!\n");
		}
		return;
	}

	if ( destroy ) {
        botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.entNum = 0;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.clientIsUsing = false;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.origin = vec3_zero;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.xySpeed = 0.0f;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.spawnID = -1;
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.entNum = self->entityNumber;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.clientIsUsing = true;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.covertToolInfo.spawnID = gameLocal.GetSpawnId( self );
	}
}

/*
=========================
idPlayer::Event_SetPlayerSpawnHostState

Let the bots know if there is a spawnhost out there in the world.
=========================
*/
void idPlayer::Event_SetPlayerSpawnHostState( idEntity *self, bool destroy ) {
	bool foundSlot = false;
	int i;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerSpawnHostState\"!\n");
		}
		return;
	}

	if ( destroy ) {
		for( i = 0; i < MAX_SPAWNHOSTS; i++ ) {
			if ( botThreadData.GetGameWorldState()->spawnHosts[ i ].spawnID != gameLocal.GetSpawnId( self ) ) {
				continue;
			}

			botThreadData.GetGameWorldState()->spawnHosts[ i ].entNum = 0;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].areaNum = 0;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].origin = vec3_zero;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].areaChecked = false;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].spawnID = -1;
			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("Found no spawnhost that matches entity %i in \"setPlayerSpawnHostState\"!\n", self->entityNumber );
			}
		}
	} else {
		for( i = 0; i < MAX_SPAWNHOSTS; i++ ) {
			if ( botThreadData.GetGameWorldState()->spawnHosts[ i ].entNum != 0 ) {
				continue;
			}

			botThreadData.GetGameWorldState()->spawnHosts[ i ].entNum = self->entityNumber;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].origin = self->GetPhysics()->GetOrigin();
			botThreadData.GetGameWorldState()->spawnHosts[ i ].spawnID = gameLocal.GetSpawnId( self );
			botThreadData.GetGameWorldState()->spawnHosts[ i ].areaNum = 0;
			botThreadData.GetGameWorldState()->spawnHosts[ i ].areaChecked = false;
			foundSlot = true;
			break;
		}

		if ( foundSlot == false ) {
			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning("Found no room to create spawnhost %i in \"setPlayerSpawnHostState\"!\n", self->entityNumber );
			}
		}
	}
}

/*
=========================
idPlayer::Event_SetPlayerAirStrikeState

Let the bots know if there is an airstrike out there in the world to worry about.
=========================
*/
void idPlayer::Event_SetPlayerAirStrikeState( idEntity *self, bool destroy, bool strikeOnWay ) {

	idVec3 dir, attackDir;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid entity passed to \"setPlayerAirStrikeState\"!\n");
		}
		return;
	}

	if ( destroy == true ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.entNum = 0;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.timeTilStrike = 0;
	} else if ( strikeOnWay == true ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.oldEntNum = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.entNum;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.entNum = 0;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.timeTilStrike = gameLocal.time + 8000;

//mal: this doesn't need to be perfect, just "good enough" that it avoids most strikes.
		dir = self->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		dir.NormalizeFast();
		attackDir = dir.Cross( idVec3( 0.0f, 0.0f, 1.0f ) );
		attackDir.NormalizeFast();
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.dir = attackDir.ToMat3();
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.entNum = self->entityNumber;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.timeTilStrike = 0;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.spawnID = gameLocal.GetSpawnId( self );

//mal: figure out the dir this strike is going to go, to safely get away from it. Mimics code from the airstrike script.
//mal: bots need some kind of initial bounds of the danger to get them moving. They'll actually get the correct bounds when its been determined a strike is on way, above.
		dir = self->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		dir.NormalizeFast();
		attackDir = dir.Cross( idVec3( 0.0f, 0.0f, 1.0f ) );
		attackDir.NormalizeFast();
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.airStrikeInfo.dir = attackDir.ToMat3();
	}
}

/*
=========================
idPlayer::Event_SetPlayerSupplyCrateState

Tracks the birth/death of the players supply crate(s).
=========================
*/
void idPlayer::Event_SetPlayerSupplyCrateState( idEntity *self, bool destroy ) {

	int i;

	if ( self == NULL ) {
		if ( bot_debug.GetBool() ) {
			gameLocal.DWarning("Invalid item passed to \"SetPlayerSupplyCrateState\"!\n"); //mal: shouldn't happen, but just in case.....
		}
		return;
	}

	if ( destroy == false ) { //mal: I'm a brand new health/ammo/supply pack, born into the world.
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].supplyCrate.entNum = self->entityNumber;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].supplyCrate.team = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].supplyCrate.bbox = self->GetPhysics()->GetBounds();
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].supplyCrate.spawnID = gameLocal.GetSpawnId( self );
	} else { //mal: I've been destroyed, or used up. Find out who owns me, and remove me.
		for( i = 0; i < MAX_CLIENTS; i++ ) {

			if ( botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.spawnID != gameLocal.GetSpawnId( self ) ) {
				continue;
			}

            botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.team = NOTEAM;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.areaNum = 0;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.checkedAreaNum = false;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.origin = vec3_zero;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.entNum = 0;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.xySpeed = 0.0f;
			botThreadData.GetGameWorldState()->clientInfo[ i ].supplyCrate.spawnID = -1;
			break;
		}
	}
}

/*
============
idPlayer::Event_SetPlayerKillTarget
============
*/
void idPlayer::Event_SetPlayerKillTarget( idEntity* killTarget ) {
	if ( killTarget != NULL ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].killTargetNum = killTarget->entityNumber;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].killTargetSpawnID = gameLocal.GetSpawnId( killTarget );
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].killTargetUpdateTime = gameLocal.time;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].killTargetNeedsChat = true;
	}
}

/*
============
idPlayer::Event_SetPlayerRepairTarget
============
*/
void idPlayer::Event_SetPlayerRepairTarget( idEntity* repairTarget ) {
	if ( repairTarget != NULL ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].repairTargetNum = repairTarget->entityNumber;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].repairTargetSpawnID = gameLocal.GetSpawnId( repairTarget );
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].repairTargetUpdateTime = gameLocal.time;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].repairTargetNeedsChat = true;
	}
}

/*
============
idPlayer::Event_SetPlayerPickupRequestTime
============
*/
void idPlayer::Event_SetPlayerPickupRequestTime( idEntity* pickUpTarget ) {
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].pickupRequestTime = gameLocal.time;
	if ( pickUpTarget != NULL ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].pickupTargetSpawnID = gameLocal.GetSpawnId( pickUpTarget );
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].pickupTargetSpawnID = -1;
	}
}

/*
============
idPlayer::Event_SetPlayerCommandRequestTime
============
*/
void idPlayer::Event_SetPlayerCommandRequestTime( void ) {
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].commandRequestTime = gameLocal.time;
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].commandRequestChatSent = false;
}

/*
============
idPlayer::Event_SetPlayerSpawnHostTarget
============
*/
void idPlayer::Event_SetPlayerSpawnHostTarget( idEntity* spawnHostTarget ) {
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].spawnHostTargetSpawnID = gameLocal.GetSpawnId( spawnHostTarget );
}

/*
============
idPlayer::Event_SetTeleporterState
============
*/
void idPlayer::Event_SetTeleporterState( bool destroy ) {
	if ( destroy ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].hasTeleporterInWorld = false;
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].hasTeleporterInWorld = true;
	}
}

/*
============
idPlayer::Event_SetRepairDroneState
============
*/
void idPlayer::Event_SetRepairDroneState( bool destroy ) {
	if ( destroy ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].hasRepairDroneInWorld = false;
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].hasRepairDroneInWorld = true;
	}
}

/*
=====================
idPlayer::Event_IsBot
=====================
*/
void idPlayer::Event_IsBot() {
	sdProgram::ReturnBoolean( userInfo.isBot );
}

/*
============
idPlayer::Event_SetBotEscort
============
*/
void idPlayer::Event_SetBotEscort( idEntity* botEscort ) {
	if ( botEscort != NULL ) {
		clientInfo_t& botPlayer = botThreadData.GetGameWorldState()->clientInfo[ botEscort->entityNumber ];

		if ( botPlayer.classType == COVERTOPS && botPlayer.isDisguised ) {
			botThreadData.bots[ botEscort->entityNumber ]->Bot_AddDelayedChat( botEscort->entityNumber, IM_DISGUISED, 3, true );
			return;
		} 

		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].escortSpawnID = gameLocal.GetSpawnId( botEscort );
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].escortRequestTime = gameLocal.time;
		botThreadData.GetGameWorldState()->clientInfo[ botEscort->entityNumber ].resetState = MAJOR_RESET_EVENT; //mal: give it total priority!
	}
}

/*
============
idPlayer::SetStroyBombState
============
*/
void idPlayer::SetStroyBombState( idEntity* bomb ) {
	for( int i = 0; i < MAX_STROYBOMBS; i++ ) {
		if ( botThreadData.GetGameWorldState()->stroyBombs[ i ].entNum != 0 ) {
			continue;
		}

		botThreadData.GetGameWorldState()->stroyBombs[ i ].entNum = bomb->entityNumber;
		botThreadData.GetGameWorldState()->stroyBombs[ i ].spawnID = gameLocal.GetSpawnId( bomb );
		botThreadData.GetGameWorldState()->stroyBombs[ i ].origin = bomb->GetPhysics()->GetOrigin();
		break;
	}
}

/*
============
idPlayer::BinCleanup
============
*/
void idPlayer::BinCleanup() {
	for ( int i = 0; i < entityBin.Num(); ) {
		if ( !entityBin[ i ].IsValid() ) {
			entityBin.RemoveIndexFast( i );
		} else {
			i++;
		}
	}

	for ( int i = 0; i < abilityEntityBin.Num(); ) {
		if ( !abilityEntityBin[ i ].IsValid() ) {
			abilityEntityBin.RemoveIndexFast( i );
		} else {
			i++;
		}
	}
}

/*
============
idPlayer::BinFindEntityOfType
============
*/
idEntity* idPlayer::BinFindEntityOfType( const idDeclEntityDef* type ) {
	for ( int i = 0; i < entityBin.Num(); i++ ) {
		if ( !entityBin[ i ].IsValid() ) {
			entityBin.RemoveIndexFast( i );
			i--;
			continue;
		}
		if ( entityBin[ i ]->entityDefNumber == type->Index() ) {
			return entityBin[ i ];
		}
	}

	return NULL;
}

/*
============
idPlayer::WriteInitialReliableMessages
============
*/
void idPlayer::WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {
	for ( int i = 0; i < entityBin.Num(); i++ ) {
		if ( entityBin[ i ].IsValid() ) {
			sdEntityBroadcastEvent evtMsg( this, EVENT_BINADD );
			evtMsg.WriteLong( entityBin[ i ].GetSpawnId() );
			evtMsg.Send( false, target );
		}
	}
}

/*
============
idPlayer::GetDecalUsage
============
*/
cheapDecalUsage_t idPlayer::GetDecalUsage( void ) {
	return CDU_INHIBIT;
}

/*
============
idPlayer::UpdateRating
============
*/
void idPlayer::UpdateRating( void ) {
	rating = NULL;

	if ( !team ) {
		return;
	}

	const sdDeclPlayerClass* pc = GetInventory().GetClass();
	if ( !pc || pc->GetNumProficiencies() <= 0 ) {
		return;
	}

	const sdDeclPlayerClass::proficiencyCategory_t& baseCategory = pc->GetProficiency( 0 );

	int count = GetProficiencyTable().GetLevel( baseCategory.index );
	if ( count >= gameLocal.declProficiencyTypeType[ baseCategory.index ]->GetNumLevels() ) {
		for ( int i = 1; i < pc->GetNumProficiencies(); i++ ) {
			const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( i );

			int level = GetProficiencyTable().GetLevel( category.index );
			if ( level >= gameLocal.declProficiencyTypeType[ category.index ]->GetNumLevels() ) {
				count++;
			}
		}
	}

	rating = team->GetRating( count );
}

/*
============
idPlayer::OnTeleportStarted
============
*/
void idPlayer::OnTeleportStarted( sdTeleporter* teleporter ) {
	SetTeleportEntity( teleporter );

	GetPhysics()->SetLinearVelocity( vec3_zero );
}

/*
============
idPlayer::OnTeleportFinished
============
*/
void idPlayer::OnTeleportFinished( void ) {
	SetTeleportEntity( NULL );

	nextTeleportTime = gameLocal.time; // + SEC2MS( 2.f );
}

/*
============
idPlayer::OnFireTeamJoined
============
*/
void idPlayer::OnFireTeamJoined( sdFireTeam* newFireTeam ) {
	if ( scriptObject != NULL ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( onFireTeamJoinedFunction, h1 );
	}

	UpdateActiveTask();
}

/*
============
idPlayer::OnFireTeamJoined
============
*/
void idPlayer::OnFireTeamDisbanded( void ) {
	if ( gameLocal.IsLocalPlayer( this ) ) {
		const sdDeclToolTip* decl = gameLocal.declToolTipType[ "tooltip_fireteam_disbanded" ];
		if ( decl == NULL ) {
			gameLocal.Warning( "idPlayer::OnFireTeamDisbanded Invalid Tooltip" );
			return;
		}

		SendToolTip( decl );
	}

	UpdateActiveTask();
}

/*
============
idPlayer::OnBecomeFireTeamLeader
============
*/
void idPlayer::OnBecomeFireTeamLeader( void ) {
	if ( scriptObject != NULL ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( onFireTeamBecomeLeader, h1 );
	}

	UpdateActiveTask();
}

/*
============
idPlayer::UpdateActiveTask
============
*/
void idPlayer::UpdateActiveTask( void ) {
	if ( !gameLocal.IsLocalPlayer( this ) ) {
		return;
	}

	idPlayer* taskPlayer = this;

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( entityNumber );
	if ( fireTeam != NULL ) {
		taskPlayer = fireTeam->GetCommander();
		if ( taskPlayer == NULL ) {
			assert( false );
			taskPlayer = this;
		}
	}

	gameLocal.localPlayerProperties.SetActiveTask( taskPlayer->GetActiveTask() );
}



#ifdef PLAYER_DAMAGE_LOG

/*
============
idPlayer::LogDamage
============
*/
void idPlayer::LogDamage( int damage, const sdDeclDamage* damageDecl ) {
	for ( int i = 0; i < damageLog.log.Num(); i++ ) {
		damageLog.log[ i ].location.z += 16.f;
	}

	damageLogInfo_t& info = damageLog.log.Alloc();

	const idBounds& bounds = physicsObj.GetAbsBounds();
	info.location	= bounds.GetCenter();
	info.location.z = bounds.GetMaxs().z;
	info.damage		= damage;
	info.damageDecl	= damageDecl;
	info.time		= gameLocal.time;

	damageLog.numbers.PresentRenderEntity();
}

/*
============
idPlayer::UpdateDamageLog
============
*/
void idPlayer::UpdateDamageLog( void ) {
	if ( damageLog.log.Num() == 0 ) {
		return;
	}

	for ( int i = 0; i < damageLog.log.Num(); ) {
		damageLogInfo_t& info = damageLog.log[ i ];
		if ( gameLocal.time - info.time > SEC2MS( 2.f ) ) {
			damageLog.log.RemoveIndex( i );
			continue;
		}

		i++;
	}

	if ( damageLog.log.Num() == 0 ) {
		damageLog.numbers.FreeRenderEntity();
		return;
	}

	damageLog.numbers.Update( this );
}

/*
============
idPlayer::ClearDamageLog
============
*/
void idPlayer::ClearDamageLog( void ) {
	damageLog.log.Clear();
	damageLog.numbers.FreeRenderEntity();
}

/*
============
idPlayer::InitDamageLog
============
*/
void idPlayer::InitDamageLog( void ) {
	damageLog.numbers.Init( this );
}

/*
============
idPlayer::sdDamageLogNumbers::Update
============
*/
void idPlayer::sdDamageLogNumbers::Update( idPlayer* player ) {
	const damageLog_t& log = player->GetDamageLog();

	SetDoubleBufferedModel();

	srfTriangles_t* surface = GetTriSurf();

	surface->bounds.Clear();
	surface->numIndexes = 0;
	surface->numVerts = 0;

	renderEntity.hModel->FreeVertexCache();

	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( !viewPlayer ) {
		return;
	}

	idVec3 viewOrg	= viewPlayer->GetRenderView()->vieworg;
	idMat3 viewAxis	= viewPlayer->GetRenderView()->viewaxis;

	int numChars = 0;

	for ( int i = 0; i < log.log.Num(); i++ ) {
		const char* text = va( "%d", log.log[ i ].damage );
		int length = idStr::Length( text );

		if ( numChars + length >= MAX_CHARS ) {
			break;
		}

		const char* p;

		float textScale = 0.5f;
		float textWidth = length * SMALLCHAR_WIDTH * textScale;
		const float upShift = MS2SEC( gameLocal.time - log.log[ i ].time ) * 32.f;
		float leftShift = textWidth * -0.5f;

		for ( p = text; *p != '\0'; p++ ) {
			char ch = *p & 255;

			if ( ch == ' ' ) {
				continue;
			}

			int row, col;
			float frow, fcol;
			float size;

			row = ch >> 4;
			col = ch & 15;

			frow = row * 0.0625f;
			fcol = col * 0.0625f;
			size = 0.0625f;

			idDrawVert* face	= surface->verts + surface->numVerts;

			float w = SMALLCHAR_WIDTH * textScale;
			float h = SMALLCHAR_HEIGHT * textScale;

			idVec3 left		= viewAxis[ 1 ] * w * -1;
			idVec3 up		= viewAxis[ 2 ] * h;

			idVec3 start	= log.log[ i ].location - ( leftShift * viewAxis[ 1 ] );
			start.z += upShift;

			face->xyz = start;
			face->color[0] = 0xFF;
			face->color[1] = 0x00;
			face->color[2] = 0x00;
			face->color[3] = 0xFF;
			face->SetST( fcol, frow + size );
			face++;

			face->xyz = start + left;
			face->color[0] = 0xFF;
			face->color[1] = 0x00;
			face->color[2] = 0x00;
			face->color[3] = 0xFF;
			face->SetST( fcol + size, frow + size );
			face++;

			face->xyz = start + up + left;
			face->color[0] = 0xFF;
			face->color[1] = 0x00;
			face->color[2] = 0x00;
			face->color[3] = 0xFF;
			face->SetST(fcol + size, frow );
			face++;

			face->xyz = start + up;
			face->color[0] = 0xFF;
			face->color[1] = 0x00;
			face->color[2] = 0x00;
			face->color[3] = 0xFF;
			face->SetST( fcol, frow );
			face++;

			surface->indexes[ surface->numIndexes + 0 ] = surface->numVerts + 2;
			surface->indexes[ surface->numIndexes + 1 ] = surface->numVerts + 1;
			surface->indexes[ surface->numIndexes + 2 ] = surface->numVerts + 0;
			surface->indexes[ surface->numIndexes + 3 ] = surface->numVerts + 3;
			surface->indexes[ surface->numIndexes + 4 ] = surface->numVerts + 2;
			surface->indexes[ surface->numIndexes + 5 ] = surface->numVerts + 0;

			surface->numVerts += 4;
			surface->numIndexes += 6;

			leftShift += w;

			numChars++;
		}

		surface->bounds.AddPoint( log.log[ i ].location );
	}

/*	for ( int i = 0; i < surface->numIndexes; i += 3 ) {
		gameRenderWorld->DebugLine( colorRed, surface->verts[ surface->indexes[ i ] ].xyz, surface->verts[ surface->indexes[ i + 1 ] ].xyz );
		gameRenderWorld->DebugLine( colorRed, surface->verts[ surface->indexes[ i + 1 ] ].xyz, surface->verts[ surface->indexes[ i + 2 ] ].xyz );
		gameRenderWorld->DebugLine( colorRed, surface->verts[ surface->indexes[ i + 2 ] ].xyz, surface->verts[ surface->indexes[ i ] ].xyz );
	}*/

	renderEntity.bounds = surface->bounds;
	PresentRenderEntity();
}

/*
============
idPlayer::sdDamageLogNumbers::Init
============
*/
void idPlayer::sdDamageLogNumbers::Init( idPlayer* player ) {
	//ClearSurfaces();
	AddSurfaceDB( declHolder.FindMaterial( "textures/bigchars" ), MAX_CHARS * 4, MAX_CHARS * 6 );

	GetTriSurf()->numVerts = 0;
	GetTriSurf()->numIndexes = 0;

	renderEntity.origin.Zero();
	renderEntity.axis.Identity();
	renderEntity.flags.noShadow = true;
	renderEntity.flags.noSelfShadow = true;
	renderEntity.flags.noDynamicInteractions = true;
	renderEntity.shaderParms[ SHADERPARM_RED ] = 1.f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0.f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0.f;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.f;
}

#endif // PLAYER_DAMAGE_LOG

/*
============
idPlayer::Event_SameFireTeam
============
*/
void idPlayer::Event_SameFireTeam( idEntity* other ) {
	if ( other == NULL ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdFireTeam *ft1 = gameLocal.rules->GetPlayerFireTeam( entityNumber );
	sdFireTeam *ft2 = gameLocal.rules->GetPlayerFireTeam( other->entityNumber );

	if ( ft1 == NULL || ft2 == NULL ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	if ( ft1 == ft2 ) {
		sdProgram::ReturnBoolean( true );
		return;
	}

	sdProgram::ReturnBoolean( false );
}

/*
============
idPlayer::Event_IsFireTeamLeader
============
*/
void idPlayer::Event_IsFireTeamLeader() {
	sdFireTeam *ft = gameLocal.rules->GetPlayerFireTeam( entityNumber );

	if ( ft == NULL ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	if ( ft->IsCommander( this ) ) {
		sdProgram::ReturnBoolean( true );
		return;
	}

	sdProgram::ReturnBoolean( false );
}

/*
============
idPlayer::Event_IsLocalPlayer
============
*/
void idPlayer::Event_IsLocalPlayer( void ) {
	sdProgram::ReturnBoolean( gameLocal.IsLocalPlayer( this ) );
}

/*
============
idPlayer::Event_IsToolTipPlaying
============
*/
void idPlayer::Event_IsToolTipPlaying( void ) {
	sdProgram::ReturnBoolean( IsToolTipPlaying() );
}

/*
============
idPlayer::Event_IsSinglePlayerToolTipPlaying
============
*/
void idPlayer::Event_IsSinglePlayerToolTipPlaying( void ) {
	sdProgram::ReturnBoolean( IsSinglePlayerToolTipPlaying()  );
}

/*
============
idPlayer::UpdatePlayerInformation

allows me to update the players info game side, so that the bots can access this info cheaply and fast.
============
*/
void idPlayer::UpdatePlayerInformation( void ) {
	clientInfo_t &clientInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	waterLevel_t waterLevel;
	idWeapon* gun = weapon.GetEntity(); //mal: get some info on our gun
	waterLevel = physicsObj.GetWaterLevel();

	clientInfo.proxyInfo.entNum = -1;
	clientInfo.usingMountedGPMG = false;
	clientInfo.mountedGPMGEntNum = -1;

//mal: get the general energy timer charge bar
	int chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.chargeTimer, this );

	chargeTime -= gameLocal.time;

	int timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.energyTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.classChargeUsed = chargeTime;


//mal: get the HE/Plasma charge bar
	chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.bombTimer, this );
	chargeTime -= gameLocal.time;
	timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.bombTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.bombChargeUsed = chargeTime;

//mal: get the firesupport charge bar
	chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.fireSupportTimer, this );
	chargeTime -= gameLocal.time;
	timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.fireSupportTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.fireSupportChargedUsed = chargeTime;

//mal: get the supply charge bar
	chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.supplyTimer, this );
	chargeTime -= gameLocal.time;
	timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.supplyTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.supplyChargeUsed = chargeTime;

//mal: get the deploy  charge bar
	chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.deployTimer, this );
	chargeTime -= gameLocal.time;
	timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.deployTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.deployChargeUsed = chargeTime;

//mal: last, get the device charge bar
	chargeTime = gameLocal.GetTargetTimerValue( clientInfo.scriptHandler.deviceTimer, this );
	chargeTime -= gameLocal.time;
	timerTime = ( botThreadData.GetGameWorldState()->gameLocalInfo.deviceTimerTime * 10 );

	if ( chargeTime < 0 || timerTime == 0 ) {
		chargeTime = 0;
	} else {
		chargeTime /= timerTime; //mal: set it to a 0 - 100 scale
	}
	clientInfo.deviceChargeUsed = chargeTime;

	clientInfo.xySpeed = ( InVehicle() ) ? proxyEntity->GetPhysics()->GetLinearVelocity().LengthFast() : GetPhysics()->GetLinearVelocity().LengthFast();
	clientInfo.health = health;
	clientInfo.maxHealth = maxHealth;
	clientInfo.inLimbo = IsInLimbo();
	clientInfo.invulnerableEndTime = invulnerableEndTime;
	clientInfo.viewAngles = viewAngles;
	clientInfo.hasGroundContact = GetPhysics()->HasGroundContacts();
	clientInfo.inWater = ( waterLevel > WATERLEVEL_WAIST /*WATERLEVEL_FEET*/ ) ? true : false;
	clientInfo.hasJumped = physicsObj.HasJumped();
	clientInfo.origin = GetPhysics()->GetOrigin();
	clientInfo.isNoTarget = fl.notarget;
	clientInfo.viewAxis = firstPersonViewAxis;
	clientInfo.inEnemyTerritory = gameLocal.TerritoryForPoint( GetPhysics()->GetOrigin(), GetGameTeam(), true, true ) == NULL;
	clientInfo.isDisguised = IsDisguised();
	clientInfo.lastKilledTime = killedTime;
	clientInfo.altitude = InchesToMetres( GetPhysics()->GetOrigin() );
	clientInfo.spawnID = gameLocal.GetSpawnId( this );
	clientInfo.isTeleporting = teleportEntity.IsValid();
	clientInfo.lastOwnedVehicleSpawnID = lastOwnedVehicleSpawnID;
	clientInfo.lastOwnedVehicleTime = lastOwnedVehicleTime;

	clientInfo.missionEntNum = MISSION_OBJ;
	sdPlayerTask* activeTask = GetActiveTask();

	if ( activeTask != NULL ) {
		idEntity* targetEntity = activeTask->GetEntity();

		if ( targetEntity != NULL ) {
			clientInfo.missionEntNum = targetEntity->entityNumber;
		} else {
			clientInfo.missionEntNum = MISSION_NULL;
		}
	}

	idEntity* spawnHost = GetSpawnPoint();

	if ( spawnHost != NULL && spawnHost->IsType( sdDynamicSpawnPoint::Type ) ) {
		clientInfo.spawnHostEntNum = spawnHost->entityNumber;
	} else {
		clientInfo.spawnHostEntNum = -1;
	}

	clientInfo.onLadder = OnLadder();
	clientInfo.isLeaning = physicsObj.IsLeaning();

	if ( clientInfo.weapInfo.covertToolInfo.entNum == 0 && clientInfo.weapInfo.covertToolInfo.clientIsUsing == false ) {
		clientInfo.viewOrigin = firstPersonViewOrigin;
	} else {
		clientInfo.viewOrigin = GetPhysics()->GetOrigin();
		clientInfo.viewOrigin[ 2 ] += eyeOffset[ 2 ];
	}

	if ( clientInfo.isDisguised ) {
		idPlayer* disguisePlayer = gameLocal.EntityForSpawnId( disguiseClient )->Cast< idPlayer >();
		if ( disguisePlayer != NULL ) {
			clientInfo.disguisedClient = disguisePlayer->entityNumber;
		}
	}

	clientInfo.absBounds = GetPhysics()->GetAbsBounds();
	clientInfo.localBounds = GetPhysics()->GetBounds();
	clientInfo.isCamper = ( clientInfo.isBot == false ) ? IsCamping() : false;
	clientInfo.isInRadar = IsInRadar( this );
	clientInfo.inPlayZone = playerFlags.inPlayZone;
	clientInfo.targetLocked = playerFlags.targetLocked;
	clientInfo.bodyAxis = GetPhysics()->GetAxis();
	clientInfo.isPanting = IsPanting();

	sdTeamInfo* team = GetTeam();

	if ( team != NULL ) {
		clientInfo.team = team->GetBotTeam();
		clientInfo.spectatorCounter = 0;
	} else {
		clientInfo.team = NOTEAM;
	}

	const sdDeclPlayerClass* pc = GetInventory().GetClass();

	if ( pc != NULL ) {
		clientInfo.classType = pc->GetPlayerClassNum();
	} else {
		clientInfo.classType = NOCLASS;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( needsParachuteFunc, h1 );
	clientInfo.needsParachute = gameLocal.program->GetReturnedBoolean();

	if ( targetEntity == NULL ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].targetLockEntNum = -1;
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].targetLockTime = 0;
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].targetLockEntNum = targetEntity->entityNumber;
		if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].targetLockTime == 0 ) {
			botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].targetLockTime = gameLocal.time;
		}
	}

	if ( IsProne() ) {
		clientInfo.posture = IS_PRONE;
		clientInfo.crouchCounter++; //mal: we'll count prone as a crouch, so that the bots will assume a crouch to stay out of sight, if escorting us.
	} else if ( IsCrouching() ) {
		clientInfo.posture = IS_CROUCHED;
		clientInfo.crouchCounter++;
	} else {
		clientInfo.posture = IS_STANDING;
		clientInfo.crouchCounter = 0;
	}

	if ( usercmd.forwardmove >= 127 && usercmd.rightmove < 127 && usercmd.rightmove > -127 ) {
		clientInfo.isMovingForward = true;
	} else {
		clientInfo.isMovingForward = false;
	}

	if ( usercmd.forwardmove > 0 || usercmd.rightmove > 0 || usercmd.rightmove < 0 || usercmd.forwardmove < 0 ) {
		clientInfo.isTryingToMove = true;
	} else {
		clientInfo.isTryingToMove = false;
	}

	if ( GetProxyEntity() != NULL ) {
		if ( usercmd.buttons.btn.attack ) {
			clientInfo.weapInfo.isFiringWeap = true;
		}

		sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

		if ( transport != NULL ) {
            botThreadData.FindCurrentWeaponInVehicle( this, transport );
			clientInfo.enemyHasLockon = transport->HasLockonDanger();

			clientInfo.proxyInfo.entNum = transport->entityNumber;

			sdJetPack* jetPack = GetProxyEntity()->Cast< sdJetPack >();

			if ( jetPack != NULL ) {
				clientInfo.proxyInfo.boostCharge = jetPack->GetChargeFraction();
				clientInfo.hasGroundContact = jetPack->GetPhysics()->HasGroundContacts();
			} else {
				clientInfo.proxyInfo.boostCharge = 0.0f;
			}

			if ( transport->GetActiveWeapon( this ) != NULL ) {
				clientInfo.proxyInfo.hasTurretWeapon = false;
				if ( transport->GetActiveWeapon( this )->GetWeaponJointHandle() != INVALID_JOINT ) {
					clientInfo.proxyInfo.hasTurretWeapon = true;
					transport->GetActiveWeapon( this )->GetWeaponOriginAxis( clientInfo.proxyInfo.weaponOrigin, clientInfo.proxyInfo.weaponAxis );
				}
                clientInfo.proxyInfo.weaponIsReady = transport->GetActiveWeapon( this )->IsWeaponReady();
			}
		} else {
			clientInfo.usingMountedGPMG = true;
			clientInfo.mountedGPMGEntNum = GetProxyEntity()->entityNumber;
		}		
	}

	if ( gun != NULL ) {
        clientInfo.weapInfo.isReady = gun->IsReady();
		clientInfo.weapInfo.isReloading = gun->IsReloading();
		clientInfo.weapInfo.isFiringWeap = gun->IsAttacking();
		clientInfo.weapInfo.isIronSightsEnabled = gun->IsIronSightsEnabled();
		clientInfo.weapInfo.isScopeUp = IsSniperScopeUp();

		if ( clientInfo.weapInfo.weapon == GRENADE && health > 0 ) {
 			clientInfo.weapInfo.grenadeFuseStart = gun->GetGrenadeFuseTime();
		} else {
			clientInfo.weapInfo.grenadeFuseStart = -1; //mal: this needs to be set -1, since its set -1 in the scripts
		}
	}
}

/*
============
idPlayer::UpdatePlayerTeamInfo
============
*/
void idPlayer::UpdatePlayerTeamInfo( void ) {
	botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team = IsSpectator() ? NOTEAM : team->GetBotTeam();
}

#define MAX_CAMP_DIST 900.0f	//mal: the max dist someone will be considered a camper

/*
============
idPlayer::UpdatePlayerKills

Updates the players "kill" list. We track the client number, so bots who have been this clients victim one too many times, can possibly get "pissed"
and look for revenge. "Too many times" is considered to be 3+.
============
*/

void idPlayer::UpdatePlayerKills( int victimNum, idEntity *client ) {
	bool foundKiller = false;
	int i, j, killedCount;
	int& killNum = botThreadData.GetGameWorldState()->clientInfo[ client->entityNumber ].killCounter;
	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ client->entityNumber ];

	playerInfo.favoriteKill = -1;
	playerInfo.kills[ killNum ].clientNum = victimNum;
	playerInfo.kills[ killNum ].ourOrigin = GetPhysics()->GetOrigin();
	playerInfo.killsSinceSpawn++;

	killNum++;

	if ( killNum >= MAX_KILLS ) {
		killNum = 0;
	}

//mal: check to see who, if anyone, this client has been picking on.
	for( j = 0; j < MAX_CLIENTS; j++ ) {

		if ( foundKiller != false ) {
			break;
		}

		if ( j == client->entityNumber ) { //mal: dont count ourselves - no matter how much of a danger to ourselves we are! :-P
			continue;
		}

		if ( playerInfo.kills[ j ].clientNum == -1 ) {
			continue;
		}

		killedCount = 0;

        for( i = 0; i < MAX_KILLS; i++ ) {

			if ( playerInfo.kills[ i ].clientNum == -1 ) {
				continue;
			}

			if ( playerInfo.kills[ i ].clientNum == j ) {
				killedCount++;
			}

			if ( killedCount >= 3 ) { //mal: we COULD be picking on 2+ guys over and over, but chances of that are slim, and its not that important.
				playerInfo.favoriteKill = j;
				foundKiller = true;
				break;
			}
		}
	}
}

/*
============
idPlayer::IsCamping

Quick check to see if the player is camping. This way we can be fair if the player leaves his camp spot.
============
*/
bool idPlayer::IsCamping( void ) {
	bool isCamper = false;
	int camperCount = 0;
	const idVec3& ourOrg = GetPhysics()->GetOrigin();
	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	for( int i = 0; i < MAX_KILLS; i++ ) {

		if ( playerInfo.kills[ i ].clientNum == -1 ) {
			continue;
		}

		idVec3 vec = ourOrg - playerInfo.kills[ i ].ourOrigin;

		if ( vec.LengthSqr() < Square( MAX_CAMP_DIST ) ) {
			camperCount++;
		}

		if ( camperCount >= 3 ) {
			isCamper = true;
			break;
		}
	}

	return isCamper;
}

/*
===============
idPlayer::IsSniperScopeUp
===============
*/
bool idPlayer::IsSniperScopeUp( void ) {
	if ( !sniperScopeUpFunc || scriptObject == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( sniperScopeUpFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
idPlayer::IsPanting

Is this player panting loudly?
===============
*/
bool idPlayer::IsPanting( void ) {
	if ( !isPantingFunc || scriptObject == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( isPantingFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
idPlayer::InVehicle
===============
*/
bool idPlayer::InVehicle( void ) {
	if ( GetProxyEntity() == NULL ) {
		return false;
	}

	sdTransport* vehicle = GetProxyEntity()->Cast< sdTransport >();

	if ( vehicle == NULL ) {
		return false;
	}

	return true;
}

/*
============
idPlayer::AddDamageEvent
============
*/
void idPlayer::AddDamageEvent( int time, float angle, float damage, bool updateDirection ) {
	assert( damageEvents.Num() == MAX_DAMAGE_EVENTS && MAX_DAMAGE_EVENTS > NUM_REPAIR_INDICATORS );

	if ( idMath::Fabs( damage ) < idMath::FLT_EPSILON ) {
		return;
	}

	// find the oldest event and replace it
	int bestIndex = 0;
	int i = 0;
	int max = damageEvents.Num() - NUM_REPAIR_INDICATORS;

	// repair uses their own indexes
	if ( damage <= 0 ) {
		bestIndex = max - NUM_REPAIR_INDICATORS;
		i = bestIndex;
		max = damageEvents.Num();
	}

	for( ; i < max; i++ ) {
		if( damageEvents[ i ].hitTime < damageEvents[ bestIndex ].hitTime ) {
			bestIndex = i;
		}
	}
	damageEvents[ bestIndex ].hitTime = time;
	damageEvents[ bestIndex ].hitAngle = angle;
	damageEvents[ bestIndex ].hitDamage = damage;
	damageEvents[ bestIndex ].updateDirection = updateDirection;
}

/*
============
idPlayer::ClearDamageEvents
============
*/
void idPlayer::ClearDamageEvents() {
	damageEvents.SetNum( MAX_DAMAGE_EVENTS );
	for( int i = 0; i < damageEvents.Num(); i++ ) {
		damageEvents[ i ].hitAngle = 0.0f;
		damageEvents[ i ].hitDamage = 0.0f;
		damageEvents[ i ].hitTime = 0;
		damageEvents[ i ].updateDirection = true;
	}
}

/*
============
idPlayer::GetLastDamageEvent
============
*/
const idPlayer::damageEvent_t& idPlayer::GetLastDamageEvent() const {
	// find the oldest event and replace it
	int bestIndex = 0;
	for( int i = 0; i < damageEvents.Num(); i++ ) {
		if( damageEvents[ i ].hitTime > damageEvents[ bestIndex ].hitTime ) {
			bestIndex = i;
		}
	}
	return damageEvents[ bestIndex ];
}

idCVar g_debugFootsteps( "g_debugFootsteps", "0", CVAR_GAME | CVAR_BOOL, "prints which surfacetype the player is walking on" );
idCVar g_disableFootsteps( "g_disableFootsteps", "0", CVAR_GAME | CVAR_BOOL, "enable/disable footsteps" );

/*
=====================
idPlayer::PlayFootStepSound
=====================
*/
void idPlayer::PlayFootStep( const char* prefix, bool rightFoot ) {
	const char *sound = NULL;
	const char *fx = NULL;

	if ( g_disableFootsteps.GetBool() ) {
		return;
	}

	if ( !GetPhysics()->HasGroundContacts() ) {
		return;
	}

	if ( playerFlags.noFootsteps ) {
		return;
	}

	const sdDeclPlayerClass* cls = GetInventory().GetClass();
	if ( cls == NULL ) {
		return;
	}

	// start footstep sound based on material type
	const sdDeclSurfaceType* surfaceType = GetPhysics()->GetContact( 0 ).surfaceType;
	if ( surfaceType ) {
		if ( *prefix == '\0' ) {
			sound = cls->GetClassData().GetString( va( "snd_footstep_%s", surfaceType->GetType() ) );
			fx = cls->GetClassData().GetString( va( "fx_footstep_%s", surfaceType->GetType() ) );
		} else {
			sound = cls->GetClassData().GetString( va( "snd_%s_footstep_%s", prefix, surfaceType->GetType() ) );
			fx = cls->GetClassData().GetString( va( "fx_%s_footstep_%s", prefix, surfaceType->GetType() ) );
		}

		if ( g_debugFootsteps.GetBool() ) {
			gameLocal.Printf( "idPlayer::PlayFootStepSound Current Surfacetype '%s'\n", surfaceType->GetType() );
		}
	} else {
		if ( g_debugFootsteps.GetBool() ) {
			gameLocal.Printf( "idPlayer::PlayFootStepSound No Material or Missing/Invalid Surfacetype\n" );
		}
	}

	if ( !sound || !*sound ) {
		if ( *prefix == '\0' ) {
			sound = cls->GetClassData().GetString( "snd_footstep" );
		} else {
			sound = cls->GetClassData().GetString( va( "snd_%s_footstep", prefix ) );
		}
	}

	if ( !fx || !*fx ) {
		if ( *prefix == '\0' ) {
			fx = cls->GetClassData().GetString( "fx_footstep" );
		} else {
			fx = cls->GetClassData().GetString( va( "fx_%s_footstep", prefix ) );
		}
	}

	if ( physicsObj.GetWaterLevel() != WATERLEVEL_NONE ) {
		// sound
		const char *waterPtr = cls->GetClassData().GetString( "snd_footstep_water" );
		if ( waterPtr != NULL && idStr::Length( waterPtr ) > 0 ) {
			sound = waterPtr;
		}

		// fx handled in waterEffects
		fx = "";
	}

	if ( *sound ) {
		const idSoundShader* shader = gameLocal.declSoundShaderType[ sound ];
		if ( shader ) {
			StartSoundShader( shader, SND_BODY, 0, NULL );
		} else {
			gameLocal.Warning( "idActor::PlayFootStep Missing Sound '%s'", sound );
		}
	}

	if ( pm_thirdPerson.GetBool() || !gameLocal.IsLocalPlayer( this ) ) {
	jointHandle_t jh = arms[ rightFoot ? 1 : 0 ].footJoint;
		if ( jh != INVALID_JOINT ) {
#if 0
			{
				idVec3 traceOrig;
				GetWorldOrigin( jh, traceOrig );
				idMat3 jointaxis;
				GetWorldAxis( jh, jointaxis );

				idVec3 startPos = traceOrig + idVec3( 0.f, 0.f, 16.f );
				idVec3 endPos = traceOrig - idVec3( 0.f, 0.f, 16.f );
				// trace
				trace_t trace;
				if ( gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, startPos, endPos, MASK_SOLID, this ) ) {
					idVec3 f = jointaxis[1];
					f.z = 0.f;
					f.Normalize();
					footPrintManager->AddFootPrint( trace.endpos, f, trace.c.normal, rightFoot );
				}
			}
#endif

			if ( *fx ) {
				int effect = gameLocal.declEffectsType.LocalFind( fx )->Index();
				if ( effect != -1 ) {
					PlayEffect( effect, colorWhite.ToVec3(), jh );
				} else {
					gameLocal.Warning( "idActor::PlayFootStep Missing FX '%s'", fx );
				}
			}
		} else {
			gameLocal.Warning( "idActor::PlayFootStep invalid joint handle" );
		}
	}
}

/*
=====================
idPlayer::Event_ResetTargetLock
=====================
*/
void idPlayer::Event_ResetTargetLock( void ) {
	if ( !gameLocal.isClient ) {
		ClearTargetLock();
	}
}

/*
=====================
idPlayer::Event_IsLocking
=====================
*/
void idPlayer::Event_IsLocking( void ) {
	sdProgram::ReturnBoolean( targetLockEndTime != 0 );
}

/*
==============
idPlayer::Event_EnableClientModelSights
==============
*/
void idPlayer::Event_EnableClientModelSights( const char* name ) {
	if ( sight.IsValid() ) {
		gameLocal.Error( "idPlayer::Event_EnableClientModelSights Called with Sight already active" );
	}

	const idDeclEntityDef* def = gameLocal.declEntityDefType[ name ];
	if ( !def ) {
		return;
	}

	sight = new sdClientAnimated();
	sight->Create( &def->dict, gameLocal.program->GetDefaultType() );
	sight->GetRenderEntity()->flags.noShadow = true;
	sight->GetRenderEntity()->flags.noSelfShadow = true;
	sight->GetRenderEntity()->flags.weaponDepthHack = true;
	sightFOV = def->dict.GetFloat( "fov", "90" );
	float fovx, fovy;
	gameLocal.CalcFov( sightFOV, fovx, fovy );
	sight->GetRenderEntity()->weaponDepthHackFOV_x = fovx;
	sight->GetRenderEntity()->weaponDepthHackFOV_y = fovy;
}

/*
==============
idPlayer::Event_DisableClientModelSights
==============
*/
void idPlayer::Event_DisableClientModelSights( void ) {
	if ( sight.IsValid() ) {
		sight->Dispose();
		sight = NULL;
	}
}

/*
==============
idPlayer::OnBindMasterVisChanged
==============
*/
void idPlayer::OnBindMasterVisChanged() {
	// player manages everything itself
}

/*
==============
idPlayer::SetInvulnerableEndTime
==============
*/
void idPlayer::SetInvulnerableEndTime( int time ) {
	if ( invulnerableEndTime == time ) {
		return;
	}

	invulnerableEndTime = time;
	if ( gameLocal.DoClientSideStuff() ) {
		UpdateGodIcon();
	}

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_SETINVULNERABLE );
		msg.WriteLong( invulnerableEndTime );
		msg.Send( invulnerableEndTime != 0, sdReliableMessageClientInfoAll() );
	}
}

/*
==============
idPlayer::Suicide
==============
*/
void idPlayer::Suicide( void ) {
	if ( IsInLimbo() ) {
		return;
	}

	if ( gameLocal.time < nextSuicideTime ) {
		return;
	}
	nextSuicideTime = gameLocal.time + SEC2MS( 1.f );

	Kill( NULL );
}

/*
==============
idPlayer::SetGameStartTime
==============
*/
void idPlayer::SetGameStartTime( int time ) {
	gameStartTime = time;
}

/*
==============
idPlayer::LogTimePlayed
==============
*/
void idPlayer::LogTimePlayed( void ) {
	if ( gameStartTime == 0 ) {
		return;
	}

	sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

	sdPlayerStatEntry* stat = tracker.GetStat( tracker.AllocStat( "total_time_played", sdNetStatKeyValue::SVT_INT ) );
	if ( stat != NULL ) {
		int t = MS2SEC( gameLocal.time - gameStartTime );
		stat->IncreaseValue( entityNumber, t );
		gameStartTime = 0;
	}
}

/*
==============
idPlayer::SetNextCallVoteTime
==============
*/
void idPlayer::SetNextCallVoteTime( int time ) {
	nextCallVoteTime = time;

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_VOTE_DELAY );
		msg.WriteLong( nextCallVoteTime );
		msg.Send( false, sdReliableMessageClientInfo( entityNumber ) );
	}
}

/*
==============
idPlayer::RequestQuickChat
==============
*/
void idPlayer::RequestQuickChat( const sdDeclQuickChat* quickChatDecl, int targetSpawnId ) {
	assert( quickChatDecl != NULL );

	int value = g_maxVoiceChats.GetInteger();
	if ( value == 0 ) {
		SendLocalisedMessage( declHolder.declLocStrType[ "game/voicechat/disabled" ], idWStrList() );
		return;
	}

	int msOver = SEC2MS( g_maxVoiceChatsOver.GetInteger() );

	int additionalOffset = ( msOver / ( float )value );

	int now = gameLocal.ToGuiTime( gameLocal.time );

	if ( voiceChatLimit < now ) {
		voiceChatLimit = now;
	}

	int newOffset = voiceChatLimit + additionalOffset;
	if ( newOffset - now > msOver ) {
		SendLocalisedMessage( declHolder.declLocStrType[ "game/voicechat/throttled" ], idWStrList() );
		return;
	}
	voiceChatLimit = newOffset;

	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_QUICK_CHAT );
		outMsg.WriteLong( quickChatDecl->Index() );
		outMsg.WriteLong( targetSpawnId );
		outMsg.Send();
	} else {
		gameLocal.ServerSendQuickChatMessage( this, quickChatDecl, NULL, gameLocal.EntityForSpawnId( targetSpawnId ) );
	}
}

/*
================
idPlayer::Event_GetCrosshairTitle
================
*/
void idPlayer::Event_GetCrosshairTitle( bool allowDisguise ) {
	const sdDeclLocStr* locStr = declHolder.declLocStrType.LocalFind( "guis/game/crosshair_title" );

	idWStrList parms;

	const sdDeclRank* chRank;
	const char* chName;
	const sdDeclRating* chRating;
	const sdDeclPlayerClass* chClass;

	if ( disguiseClient != -1 && allowDisguise ) {
		idPlayer* other = gameLocal.EntityForSpawnId( disguiseClient )->Cast< idPlayer >();

		chRank = disguiseRank;

		if ( other != NULL ) {
			disguiseUserName = other->userInfo.name;
		}

		chName = disguiseUserName.c_str();
		chRating = disguiseRating;
		chClass = disguiseClass;
	} else {
		chRank = GetProficiencyTable().GetRank();
		chName = userInfo.name.c_str();
		chRating = rating;
		chClass = GetInventory().GetClass();
	}

	const sdDeclLocStr* temp;

	temp = chRank == NULL ? NULL : chRank->GetShortTitle();
	parms.Append( temp == NULL ? L"" : temp->GetText() );

	parms.Append( va( L"%hs", chName ) );

	temp = chRating == NULL ? NULL : chRating->GetTitle();
	parms.Append( temp == NULL ? L"" : temp->GetText() );

	temp = chClass->GetTitle();
	parms.Append( temp == NULL ? L"" : temp->GetText() );

	sdProgram::ReturnWString( common->LocalizeText( locStr, parms ).c_str() );
}

/*
================
idPlayer::Event_GetTeamDamageDone
================
*/
void idPlayer::Event_GetTeamDamageDone( void ) {
	sdProgram::ReturnInteger( teamDamageDone );
}

/*
================
idPlayer::Event_SetTeamDamageDone
================
*/
void idPlayer::Event_SetTeamDamageDone( int value ) {
	if ( value < 0 ) {
		value = 0;
	}
	teamDamageDone = value;
}

/*
================
idPlayer::Event_AdjustDeathYaw
================
*/
void idPlayer::Event_AdjustDeathYaw( float value ) {
	baseDeathYaw += value;
}

/*
================
idPlayer::GetAORView
================
*/
void idPlayer::GetAORView( idVec3& origin, idMat3& axis ) {
	if ( teleportEntity.IsValid() ) {
		// use teleport exit point
		teleportEntity->GetTargetPosition( origin, axis );
		return;
	} else if ( proxyEntity.IsValid() ) {
		// use proxy entity's teleport exit point
		sdTransport* transportProxy = proxyEntity->Cast< sdTransport >();
		if ( transportProxy != NULL && transportProxy->IsTeleporting() ) {
			transportProxy->GetTeleportEntity()->GetTargetPosition( origin, axis );
			return;
		}
	}

	CalculateFirstPersonView( false );
	origin	= firstPersonViewOrigin;
	axis	= firstPersonViewAxis;
}

/*
================
idPlayer::GetLastPredictionErrorDecayOrigin
================
*/
const idVec3& idPlayer::GetLastPredictionErrorDecayOrigin( void ) { 
	if ( gameLocal.isClient ) {
		return predictionErrorDecay_Origin.GetLastReturned(); 
	} else {
		return renderEntity.origin;
	}
}

/*
================
idPlayer::Event_SetCarryingObjective
================
*/
void idPlayer::Event_SetCarryingObjective( bool isCarrying ) {
	playerFlags.carryingObjective = isCarrying;
}

/*
================
idPlayer::Event_AddDamageEvent
================
*/
void idPlayer::Event_AddDamageEvent( int time, float angle, float damage, bool updateDirection ) {
	AddDamageEvent( SEC2MS( time ), angle, damage, updateDirection );
}

/*
================
idPlayer::Event_IsInLimbo
================
*/
void idPlayer::Event_IsInLimbo( void ) {
	sdProgram::ReturnBoolean( IsInLimbo() );
}

/*
================
idPlayer::IsBeingBriefed
================
*/
bool idPlayer::IsBeingBriefed() const {
	if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].briefingTime < gameLocal.time ) {
		return false;
	}

	return true;
}

/*
================
idPlayer::UpdateBriefingView
================
*/
void idPlayer::UpdateBriefingView() {
	if ( botThreadData.actorMissionInfo.actorClientNum > -1 && botThreadData.actorMissionInfo.actorClientNum < MAX_CLIENTS ) {
		idPlayer* actor = gameLocal.GetClient( botThreadData.actorMissionInfo.actorClientNum );

		if ( actor != NULL ) {
			idVec3 lookAtOrigin = actor->firstPersonViewOrigin;
			lookAtOrigin.z -= 16.0f;
			idVec3 lookAtDelta = lookAtOrigin - firstPersonViewOrigin;

			if ( lookAtDelta.LengthSqr() > idMath::FLT_EPSILON ) {
				idQuat fromQuat = firstPersonViewAxis.ToQuat();
				idQuat toQuat = lookAtDelta.ToAngles().ToQuat();
				float t = ( gameLocal.ToGuiTime( gameLocal.time ) - gameLocal.pauseStartGuiTime ) / 1000.0f;
				idQuat nowQuat;
				nowQuat.Slerp( fromQuat, toQuat, idMath::ClampFloat( 0.0f, 1.0f, t ) );

				idAngles nowAngles = nowQuat.ToAngles();
				nowAngles.roll = 0.0f;
		
				SetViewAngles( nowAngles );
				firstPersonViewAxis = nowAngles.ToMat3();
			}

			actor->GetWeapon()->UpdateWeaponVisibility();
		}
	}
}
