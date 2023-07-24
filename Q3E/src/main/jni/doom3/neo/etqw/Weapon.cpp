// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../decllib/DeclSurfaceType.h"

#include "Weapon.h"
#include "Actor.h"
#include "Light.h"
#include "script/Script_ScriptObject.h"
#include "ScriptEntity.h"
#include "Player.h"
#include "Projectile.h"
#include "Item.h"
#include "ContentMask.h"
#include "script/Script_Helper.h"
#include "client/ClientEntity.h"
#include "guis/UserInterfaceLocal.h"
#include "../decllib/declTypeHolder.h"
#include "proficiency/StatsTracker.h"
#include "client/ClientEffect.h"

#include "vehicles/TransportExtras.h"

#include "botai/BotThreadData.h"

#include "guis/GuiSurface.h"

#include "AntiLag.h"

/*
===============
sdWeaponLockInfo::Load
===============
*/
void sdWeaponLockInfo::Load( const idDict& dict ) {
	supported		= dict.GetBool( "lock_enabled" );
	lockedSound		= gameLocal.declSoundShaderType[ dict.GetString( "snd_target_locked" ) ];
	lockingSound	= gameLocal.declSoundShaderType[ dict.GetString( "snd_target_locking" ) ];
	lockDuration	= SEC2MS( dict.GetFloat( "lock_duration", "1" ) );
	lockDistance	= dict.GetFloat( "lock_distance", "2048" );
	lockFriendly	= dict.GetBool( "lock_friendly" );
	sticky			= dict.GetBool( "lock_sticky" );
	lockFilter		= gameLocal.declTargetInfoType[ dict.GetString( "lock_filter" ) ];
}

/***********************************************************************

  idWeapon  
	
***********************************************************************/

//
// event defs
//
extern const idEventDef EV_PlayCycle;
extern const idEventDef EV_PlayAnim;
extern const idEventDef EV_GetOwner;

const idEventDefInternal EV_Clear( "internal_clear" );
const idEventDef EV_GetOwner( "getOwner", 'e', DOC_TEXT( "Returns the primary owner of this entity, or $null$ if none." ), 0, NULL );
const idEventDef EV_Next( "nextWeapon", '\0', DOC_TEXT( "Switches to the next best weapon." ), 0, NULL );
const idEventDef EV_State( "weaponState", '\0', DOC_TEXT( "Changes the script state in the same manner as $event:setState$, and also sets up the blend time for the next animation." ), 2, "An error will be thrown if the new state cannot be found.", "s", "state", "Name of the new state.", "d", "blendFrames", "Number of frames to blend the next animation in with." );
const idEventDef EV_GetState( "getWeaponState", 's', DOC_TEXT( "Returns the last state the weapon was put into with a call to $event:weaponState$." ), 0, NULL );
const idEventDef EV_AddToClip( "addToClip", '\0', DOC_TEXT( "Adds the specified amount of ammo to the clip at the given index." ), 2, "The amount of ammo in the clip will be automatically capped by the maximum size of the clip, and the amount of ammo available.", "d", "index", "Clip index.", "d", "amount", "Amount of ammo to add." );
const idEventDef EV_AmmoInClip( "ammoInClip", 'd', DOC_TEXT( "Returns the amount of ammo in the clip at the given index." ), 1, "If the clip index is out of range, the result will be 0.", "d", "index", "Clip index." );
const idEventDef EV_AmmoAvailable( "ammoAvailable", 'd', DOC_TEXT( "Returns the number of shots available from the clip at the given index." ), 1, "If the clip index is out of range, or the clip does not use ammo, the result will be -1", "d", "index", "Clip index." );
const idEventDef EV_AmmoRequired( "ammoRequired", 'd', DOC_TEXT( "Returns the amount of ammo required per shot for the clip at the give index." ), 1, "If the clip index is out of range, the result will be 0.", "d", "index", "Clip index." );
const idEventDef EV_AmmoType( "ammoType", 'd', DOC_TEXT( "Returns the index of the $decl:ammoDef$ used by the clip at the given index." ), 1, "If the clip index is out of range, the result will be -1.", "d", "index", "Clip index." );
const idEventDef EV_UseAmmo( "useAmmo", '\0', DOC_TEXT( "Removes ammo from the player's inventory, of the specified amount and type." ), 2, "If the player does not have enough ammo to cover the reduction, the amount to remove will be reduced to the ammo that they have.", "d", "type", "Index of the $decl:ammoDef$.", "d", "amount", "Amount of ammo to use." );
const idEventDef EV_ClipSize( "clipSize", 'd', DOC_TEXT( "Returns the size of the clip at the given index." ), 1, "If the index is out of range, the result will be 0.", "d", "index", "Clip index." );
const idEventDef EV_WeaponReady( "weaponReady", '\0', DOC_TEXT( "Sets the internal state of the weapon to ready, and clears the weapon raising flag." ), 0, NULL );
const idEventDef EV_WeaponReloading( "weaponReloading", '\0', DOC_TEXT( "Sets the internal state of the weapon to reloading, and sends a message to clients telling them that the weapon is reloading." ), 0, NULL );
const idEventDef EV_WeaponHolstered( "weaponHolstered", '\0', DOC_TEXT( "Sets the internal state of the weapon to holstered, which allows weapon switches to occur, and clears the weapon lowering flag." ), 0, NULL );
const idEventDef EV_WeaponRising( "weaponRising", '\0', DOC_TEXT( "Sets the internal state of the weapon to rising, clears the weapon lowering flag, and informs the player animation state machine that the weapon is being raised." ), 0, NULL );
const idEventDef EV_WeaponLowering( "weaponLowering", '\0', DOC_TEXT( "Sets the internal state of the weapon to lowering, clears the weapon raising flag, and informs the player animation state machine that the weapon is being lowered." ), 0, NULL );
const idEventDef EV_Weapon_DoProjectileTracer( "doProjectileTracer", "dvv" );
const idEventDef EV_LaunchProjectiles( "launchProjectiles", '\0', DOC_TEXT( "Launches the specified number and type of projectiles from the weapon." ), 6, "If the clip index is out of range, or the clip specified does not contain projectile information, no projectiles will be launched.\nLaunchPower and fuseOffset apply only to projectiles of type $class:idProjectile$.", "d", "count", "Number of projectiles to launch.", "d", "index", "Clip index to get the projectile info from.", "f", "spread", "Spread factor, in degrees.", "f", "fuseOffset", "Fuse offset to pass to the projectile.", "f", "launchPower", "Scale factor for launch velocity.", "f", "damagePower", "Scale factor for damage." );
const idEventDef EV_CreateProjectile( "createProjectile", 'e', DOC_TEXT( "Creates a projectile using projectile information from the specified clip. This entity will be return, and the weapon will keep track of it, and use as the first projectile for the next call to $event:launchProjectiles$. The projectile will remain hiden until then, so scripts can use this to modify parameters on the projectile prior to launch." ), 1, "If the clip index is out of range, or the clip does not contain any projectile info, no projectile will be spawned, and the result will be $null$.", "d", "index", "Clip index." );
const idEventDef EV_Melee( "melee", 'b', DOC_TEXT( "Performs a trace along the player's view axis, and returns whether it hit anything or not." ), 4, "The result of the trace can be saved using $event:saveMeleeTrace$.", "d", "mask", "Collision mask to use.", "f", "distance", "Distance to trace along the axis.", "b", "ignoreOwner", "Whether to ignore the player in the trace or not.", "b", "antiLag", "Whether the trace should use antilag information or not." );
const idEventDef EV_MeleeAttack( "meleeAttack", 'b', DOC_TEXT( "Applies damage to the entity hit in the last call to $event:melee$, if any, and returns whether it attempted to apply damage." ), 1, "This will always return false on network clients.\nIf the damage scale is negative, special damage will be used instead of the regular damage.", "f", "damageScale", "Damage scale factor." );
const idEventDef EV_SaveMeleeTrace( "saveMeleeTrace", 'o', DOC_TEXT( "Saves the result of the last call to $event:melee$." ), 0, "See also $event:saveTrace$." );

const idEventDef EV_GetMeleeFraction( "getMeleeFraction", 'f', DOC_TEXT( "Returns the fraction that passed of the last melee." ), 0, "Use $event:melee$ to perform a melee trace." );
const idEventDef EV_GetMeleeEndPos( "getMeleeEndPos", 'v', DOC_TEXT( "Returns the end position of the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace." );
const idEventDef EV_GetMeleePoint( "getMeleePoint", 'v', DOC_TEXT( "Returns the point of collision, if any, of the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeNormal( "getMeleeNormal", 'v', DOC_TEXT( "Returns the normal at the collision point from the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeEntity( "getMeleeEntity", 'e', DOC_TEXT( "Returns the entity hit from the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeSurfaceFlags( "getMeleeSurfaceFlags", 'd', DOC_TEXT( "Returns the surface flags of the surface hit by the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeSurfaceType( "getMeleeSurfaceType", 's', DOC_TEXT( "Returns the name of the $decl:surfaceType$ of the surface hit by the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeSurfaceColor( "getMeleeSurfaceColor", 'v', DOC_TEXT( "Returns the color of the surface hit by the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid." );
const idEventDef EV_GetMeleeJoint( "getMeleeJoint", 's', DOC_TEXT( "Returns the name of the joint associated with the surface hit by the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid.\nIf the surface hit does not belong to an animated model, the result will be an empty string." );
const idEventDef EV_GetMeleeBody( "getMeleeBody", 's', DOC_TEXT( "Returns the name of the body associated with the surface hit by the last melee trace." ), 0, "Use $event:melee$ to perform a melee trace.\nIf no collision occured, this data is invalid.\nIf the surface hit does not belong to an articulated figure, the result will be an empty string." );
const idEventDef EV_Weapon_SendTracerMessage( "sendTracerMessage", "vvf" );

const idEventDef EV_EnableTargetLock( "enableTargetLock", '\0', DOC_TEXT( "Enables/disables support for target locking." ), 1, "If the currently active weapon does not support target locking, this will have no effect.", "b", "state", "Should target locking be enabled or not." );
const idEventDef EV_SetFov( "setFov", '\0', DOC_TEXT( "Starts a fov transition." ), 3, NULL, "f", "start", "Fov to start the transition at.", "f", "end", "Fov to end the transition at.", "f", "duration", "Length in seconds for the transition to take." );
const idEventDef EV_SetFovStart( "setFovStart", '\0', DOC_TEXT( "Starts a delayed fov transition." ), 4, NULL, "f", "start", "Fov to start the transition at.", "f", "end", "Fov to end the transition at.", "f", "offset", "Time in seconds to delay the transition for.", "f", "duration", "Length in seconds for the transition to take." );
const idEventDef EV_ClearFov( "clearFov", '\0', DOC_TEXT( "Resets any active fov modifications." ), 0, NULL );
const idEventDef EV_GetFov( "getFov", 'f', DOC_TEXT( "Returns the value of fov from any active fov modifications." ), 0, "If there are no active fov modifications, the result will be 0." );
const idEventDef EV_EnableClamp( "enableClamp", '\0', DOC_TEXT( "Enables view rate/limit clamping on the weapon, with the given base angles." ), 1, "See also $event:disableClamp$.", "v", "base", "Base set of angles that any limit clamps will be based on." );
const idEventDef EV_DisableClamp( "disableClamp", '\0', DOC_TEXT( "Disables view rate/limit clamping on the weapon." ), 0, "See also $event:enableClamp$." );
const idEventDef EV_GetspreadCurrentValue( "getCurrentSpread", 'f', DOC_TEXT( "Returns the current value of spread for the weapon." ), 0, NULL );
const idEventDef EV_IncreaseSpreadValue( "increaseSpread", '\0', DOC_TEXT( "Increases the spread for the weapon based on the values for firing the weapon." ), 0, NULL );
const idEventDef EV_SetSpreadModifier( "setSpreadModifier", '\0', DOC_TEXT( "Applies a spread modifier which will be applied to the current value before being returned." ), 1, NULL, "f", "scale", "Scale to set." );
const idEventDef EV_Fired( "fired", '\0', DOC_TEXT( "Performs animation callbacks and view angle kicks as if a call to $event:launchProjectiles$ had occured." ), 0, NULL );
const idEventDef EV_GetWorldModel( "getWorldModel", 'o', DOC_TEXT( "Returns the script object for the world model client entity at the specified index." ), 1, "If the index is out of range, the result will be $null$.", "d", "index", "Index of the model to return." );
const idEventDef EV_EnableSway( "enableSway", '\0', DOC_TEXT( "Enables/disables scope swaying." ), 1, NULL, "b", "value", "Whether to enable or disable." );
const idEventDef EV_SetDriftScale( "setDriftScale", '\0', DOC_TEXT( "Sets the weapon visual drifting mode. If the factor is less than 0.1, ironsights mode will be used, otherwise regular mode will be used." ), 1, "FIXME: This event needs tidied up, it no longer does what it used to.", "f", "scale", "Scale factor to apply." );
const idEventDef EV_ResetTracerCounter( "resetTracerCounter", '\0', DOC_TEXT( "Resets the internal counter for counting the interval between tracers to 0." ), 0, NULL );
const idEventDef EV_GetLastTracer( "getLastTracer", 'h', DOC_TEXT( "Returns a handle to the last tracer effect that was created." ), 0, "If the tracer no longer exist, or has never been created, the result is 0." );
const idEventDef EV_SetupAnimClass( "setupAnimClass", '\0', DOC_TEXT( "Modifies the weapon and weapon class prefixes for use by the player third person animations." ), 1, NULL, "s", "key", "Key to use to look up the new prefixes." );
const idEventDef EV_HasWeaponAnim( "hasWeaponAnim", 'b', DOC_TEXT( "Returns whether the weapon has an animation with the specified name." ), 1, NULL, "s", "anim", "Name of the animation to check for." );
const idEventDef EV_SetStatName( "setStatName", '\0', DOC_TEXT( "Allows on the fly changing of the name used for stat tracking for the weapon." ), 1, "If an empty string is passed, stats will be disabled.", "s", "name", "New stats name to use." );

//
// class def
//
CLASS_DECLARATION( idAnimatedEntity, idWeapon )
	EVENT( EV_Clear,					idWeapon::Event_Clear )
	EVENT( EV_GetOwner,					idWeapon::Event_GetOwner )
	EVENT( EV_State,					idWeapon::Event_WeaponState )
	EVENT( EV_GetState,					idWeapon::Event_GetWeaponState )
	EVENT( EV_WeaponReady,				idWeapon::Event_WeaponReady )
	EVENT( EV_WeaponReloading,			idWeapon::Event_WeaponReloading )
	EVENT( EV_WeaponHolstered,			idWeapon::Event_WeaponHolstered )
	EVENT( EV_WeaponRising,				idWeapon::Event_WeaponRising )
	EVENT( EV_WeaponLowering,			idWeapon::Event_WeaponLowering )
	EVENT( EV_AddToClip,				idWeapon::Event_AddToClip )
	EVENT( EV_AmmoInClip,				idWeapon::Event_AmmoInClip )
	EVENT( EV_AmmoAvailable,			idWeapon::Event_AmmoAvailable )
	EVENT( EV_AmmoRequired,				idWeapon::Event_AmmoRequired )
	EVENT( EV_AmmoType,					idWeapon::Event_AmmoType )
	EVENT( EV_UseAmmo,					idWeapon::Event_UseAmmo )
	EVENT( EV_ClipSize,					idWeapon::Event_ClipSize )
	EVENT( EV_PlayAnim,					idWeapon::Event_PlayAnim )
	EVENT( EV_PlayCycle,				idWeapon::Event_PlayCycle )
	EVENT( AI_SetBlendFrames,			idWeapon::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idWeapon::Event_GetBlendFrames )
	EVENT( AI_AnimDone,					idWeapon::Event_AnimDone )
	EVENT( EV_Next,						idWeapon::Event_Next )
	EVENT( EV_LaunchProjectiles,		idWeapon::Event_LaunchProjectiles )
	EVENT( EV_Weapon_DoProjectileTracer,idWeapon::Event_DoProjectileTracer )
	EVENT( EV_CreateProjectile,			idWeapon::Event_CreateProjectile )
	EVENT( EV_Melee,					idWeapon::Event_Melee )
	EVENT( EV_MeleeAttack,				idWeapon::Event_MeleeAttack )
	EVENT( EV_SaveMeleeTrace,			idWeapon::Event_SaveMeleeTrace )

	EVENT( EV_GetMeleeFraction,			idWeapon::Event_GetMeleeFraction )
	EVENT( EV_GetMeleeEndPos,			idWeapon::Event_GetMeleeEndPos )
	EVENT( EV_GetMeleePoint,			idWeapon::Event_GetMeleePoint )
	EVENT( EV_GetMeleeNormal,			idWeapon::Event_GetMeleeNormal )
	EVENT( EV_GetMeleeEntity,			idWeapon::Event_GetMeleeEntity )
	EVENT( EV_GetMeleeSurfaceFlags,		idWeapon::Event_GetMeleeSurfaceFlags )
	EVENT( EV_GetMeleeSurfaceType,		idWeapon::Event_GetMeleeSurfaceType )
	EVENT( EV_GetMeleeSurfaceColor,		idWeapon::Event_GetMeleeSurfaceColor )
	EVENT( EV_GetMeleeJoint,			idWeapon::Event_GetMeleeJoint )
	EVENT( EV_GetMeleeBody,				idWeapon::Event_GetMeleeBody )

	EVENT( EV_EnableTargetLock,			idWeapon::Event_EnableTargetLock )
	EVENT( EV_SetFov,					idWeapon::Event_SetFov )
	EVENT( EV_SetFovStart,				idWeapon::Event_SetFovStart )
	EVENT( EV_ClearFov,					idWeapon::Event_ClearFov )
	EVENT( EV_EnableClamp,				idWeapon::Event_EnableClamp )
	EVENT( EV_DisableClamp,				idWeapon::Event_DisableClamp )
	EVENT( EV_GetFov,					idWeapon::Event_GetFov )
	EVENT( EV_GetspreadCurrentValue,	idWeapon::Event_GetSpreadValue )
	EVENT( EV_IncreaseSpreadValue,		idWeapon::Event_IncreaseSpreadValue )
	EVENT( EV_SetSpreadModifier,		idWeapon::Event_SetSpreadModifier )
	EVENT( EV_Hide,						idWeapon::Event_Hide )
	EVENT( EV_Show,						idWeapon::Event_Show )
	EVENT( EV_Fired,					idWeapon::Event_Fired )
	EVENT( EV_GetWorldModel,			idWeapon::Event_GetWorldModel )
	EVENT( EV_EnableSway,				idWeapon::Event_EnableSway )
	EVENT( EV_SetDriftScale,			idWeapon::Event_SetDriftScale )
	EVENT( EV_ResetTracerCounter,		idWeapon::Event_ResetTracerCounter )
	EVENT( EV_GetLastTracer,			idWeapon::Event_GetLastTracer )
	EVENT( EV_SetupAnimClass,			idWeapon::Event_SetupAnimClass )
	EVENT( EV_HasWeaponAnim,			idWeapon::Event_HasWeaponAnim )
	EVENT( EV_SetStatName,				idWeapon::Event_SetStatName )

	EVENT( EV_Weapon_SendTracerMessage,	idWeapon::Event_SendTracerMessage )
END_CLASS

/*
===============================================================================

	sdWeaponNetworkInterface

===============================================================================
*/

/*
================
sdWeaponNetworkInterface::HandleNetworkMessage
================
*/
void sdWeaponNetworkInterface::HandleNetworkMessage( idPlayer* player, const char* message ) {
	idScriptObject* scriptObject = owner->GetScriptObject();
	if ( !scriptObject ) {
		return;
	}
	const sdProgram::sdFunction* function = scriptObject->GetFunction( "OnNetworkMessage" );
	if ( !function ) {
		return;
	}

	gameLocal.SetActionCommand( message );

	sdScriptHelper helper;
	helper.Push( player->GetScriptObject() );
	owner->CallNonBlockingScriptEvent( function, helper );
}

/*
================
sdWeaponNetworkInterface::HandleNetworkEvent
================
*/
void sdWeaponNetworkInterface::HandleNetworkEvent( const char* message ) {
	if ( !owner->GetScriptObject() ) {
		gameLocal.Warning( "sdWeaponNetworkInterface::HandleNetworkEvent With No Owner Scriptobject" );
		return;
	}

	const sdProgram::sdFunction* function = owner->GetScriptObject()->GetFunction( "OnNetworkEvent" );
	if ( !function ) {
		return;
	}

	gameLocal.SetActionCommand( message );

	sdScriptHelper helper;
	owner->CallNonBlockingScriptEvent( function, helper );
}

/*
===============================================================================

	idWeapon

===============================================================================
*/

/*
================
idWeapon::idWeapon
================
*/
idWeapon::idWeapon() {
	owner					= NULL;
	weaponItem				= NULL;
	thread					= NULL;
	onActivateFunc			= NULL;
	onUsedFunc				= NULL;
	onWeapNextFunc			= NULL;
	onWeapPrevFunc			= NULL;
	cleanupFunc				= NULL;
	climateSkin				= NULL;

	playerWeaponNum			= NULL_WEAP;

	getGrenadeFuseStartFunc = NULL;
	ironSightsEnabledFunc	= NULL;

	networkInterface.Init( this );

	memset( &meleeTrace, 0, sizeof( meleeTrace ) );

	viewOffset.Zero();

	ownerStanceState		= WSV_STANDING;
	spreadModifier			= 1.0f;
	spreadEvalVelocity		= true;
	spreadEvalView			= true;

	spreadCurrentValue		= 0.f;
	spreadValueMax			= 0.f;

	for ( int i = WSV_STANDING; i < WSV_NUM; i++ ) {
		spreadValues[ i ].min = 0.f;
		spreadValues[ i ].max = 1.f;
		spreadValues[ i ].inc = 1.f;
		spreadValues[ i ].numIgnoreRounds = 0;
		spreadValues[ i ].maxSettleTime = 1000;
		spreadValues[ i ].viewRateMin = 0;
		spreadValues[ i ].viewRateMax = 0;
		spreadValues[ i ].viewRateInc = 0;
	}

	aimValues[ WAV_IRONSIGHTS ].bobscaleyaw		= -0.01f;
	aimValues[ WAV_IRONSIGHTS ].bobscalepitch	= 0.005f;
	aimValues[ WAV_IRONSIGHTS ].lagscaleyaw		= 0.9f;
	aimValues[ WAV_IRONSIGHTS ].lagscalepitch	= 0.9f;
	aimValues[ WAV_IRONSIGHTS ].speedlr			= 0.f;

	aimValues[ WAV_NORMAL ].bobscaleyaw			= -0.1f;
	aimValues[ WAV_NORMAL ].bobscalepitch		= 0.2f;
	aimValues[ WAV_NORMAL ].lagscaleyaw			= 0.91f;
	aimValues[ WAV_NORMAL ].lagscalepitch		= 0.91f;
	aimValues[ WAV_NORMAL ].speedlr				= 0.01f;

	driftScale				= 1.f;

	tracerCounter			= 0;

	stats.timeUsed			= NULL;
	stats.shotsFired		= NULL;

	dofModelDefHandle		= -1;

	largeFOVScale			= -4.f;

	Clear();
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() {
	Clear();

	SetNumWorldModels( 0 );

	if ( thread != NULL ) {
		gameLocal.program->FreeThread( thread );
		thread = NULL;
	}
}

/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn( void ) {
	thread = gameLocal.program->CreateThread();
	thread->SetName( GetName() );
	thread->ManualDelete();
	thread->ManualControl();
}

/*
================
idWeapon::SetOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetOwner( idPlayer* _owner ) {
	owner = _owner;

	UpdateVisibility();
}

/*
================
idWeapon::SetNumWorldModels
================
*/
void idWeapon::SetNumWorldModels( int count ) {
	if ( count < 0 ) {
		gameLocal.Error( "idWeapon::SetNumWorldModels Invalid Count '%d'", count );
	}

	int oldCount = worldModels.Num();

	for ( int i = oldCount - 1; i >= count; i-- ) {
		if ( !worldModels[ i ].IsValid() ) {
			continue;
		}

		delete worldModels[ i ].GetEntity();
	}

	barrelJointsWorld.SetNum( count, false );
	worldModels.SetNum( count, false );

	for ( int i = oldCount; i < count; i++ ) {
		// setup the world model and its script entity
		sdClientAnimated* newWorldModel = new sdClientAnimated();
		newWorldModel->Create( NULL, gameLocal.program->GetDefaultType() );

		worldModels[ i ] = newWorldModel;
		if ( worldModels[ i ].GetEntity() != newWorldModel ) {
			gameLocal.ListClientEntities_f( idCmdArgs() );
			gameLocal.Error( "idWeapon::SetNumWorldModels Entity Mismatch when spawning World Model" );
		}

		barrelJointsWorld[ i ] = INVALID_JOINT;
	}
}

/*
================
idWeapon::ScriptCleanup
================
*/
void idWeapon::ScriptCleanup() {
	if ( cleanupFunc == NULL ) {
		return;
	}

	sdScriptHelper h;
	scriptObject->CallNonBlockingScriptEvent( cleanupFunc, h );
}

/*
================
idWeapon::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idWeapon::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/***********************************************************************

	Weapon definition management

***********************************************************************/

/*
================
idWeapon::Clear
================
*/
void idWeapon::Clear( void ) {
	LogTimeUsed();

	CancelEvents( &EV_Clear );

	DeconstructScriptObject();

	WEAPON_ATTACK.Unlink();
	WEAPON_ALTFIRE.Unlink();
	WEAPON_MODESWITCH.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();
	WEAPON_HIDE.Unlink();

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.spawnID	= gameLocal.GetSpawnId( this );//entityNumber;

	renderEntity.flags.noShadow		= true;
	renderEntity.flags.noSelfShadow	= true;

	// set default shader parms
	renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	renderEntity.shaderParms[3] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = 0.0f;
	renderEntity.shaderParms[5] = 0.0f;
	renderEntity.shaderParms[6] = 0.0f;
	renderEntity.shaderParms[7] = 0.0f;

	if ( refSound.referenceSound != NULL ) {
		refSound.referenceSound->Free( false );
	}
	memset( &refSound, 0, sizeof( refSound_t ) );
	
	// setting diversity to 0 results in no random sound.  -1 indicates random.
	refSound.diversity = -1.0f;

	if ( owner ) {
		// don't spatialize the weapon sounds
		refSound.listenerId						= owner->GetListenerId();
		renderEntity.allowSurfaceInViewID		= owner->entityNumber + 1;	
	}

	spawnArgs.Clear();

	hideTime		= 300;
	hideDistance	= -15.0f;
	hideStartTime	= gameLocal.time - hideTime;
	hideStart		= 0.0f;
	hideEnd			= 0.0f;
	hideOffset		= 0.0f;
	hide			= false;
	WEAPON_HIDE		= false;
	disabled		= false;

	timeStartedUsing = 0;

	zoomFov.Init( 0, 0, 0, 0 );

	playerViewAxis.Identity();
	playerViewOrigin.Zero();
	viewWeaponAxis.Identity();
	viewWeaponOrigin.Zero();

	status			= WP_HOLSTERED;
	state			= "";
	idealState		= "";
	animBlendFrames	= 0;
	animDoneTime	= 0;

	meleeDamage			= NULL;
	meleeSpecialDamage	= NULL;

	kick_endtime		= 0;
	muzzle_kick_time	= 0;
	muzzle_kick_maxtime	= 0;
	muzzle_kick_angles.Zero();
	muzzle_kick_offset.Zero();

	zoomPitchAmplitude		= 0.0f;
	zoomPitchFrequency		= 0.0f;
	zoomPitchMinAmplitude	= 0.0f;
	zoomYawAmplitude		= 0.0f;
	zoomYawFrequency		= 0.0f;
	zoomYawMinAmplitude		= 0.0f;
	swayEnabled				= false;

	largeFOVScale			= -4.f;

	driftScale				= 1.f;

	barrelJointView		= INVALID_JOINT;

	animator.ClearAllAnims( gameLocal.time, 0 );

	Hide();

	isLinked			= false;
	projectileEnt		= NULL;

	clampEnabled		= false;

	stats.shotsFired		= NULL;
	stats.timeUsed			= NULL;
}

/*
================
idWeapon::InitWorldModel
================
*/
void idWeapon::InitWorldModel( int index ) {
	idStr suffix = index > 0 ? va( "_%d", index + 1 ) : "";

	const char* model	= spawnArgs.GetString( va( "model_world%s", suffix.c_str() ) );
	const char* attach	= spawnArgs.GetString( va( "joint_attach%s", suffix.c_str() ) );

	sdClientAnimated* worldModel = worldModels[ index ].GetEntity();

	worldModel->SetSkin( NULL );
	if ( *model && *attach ) {
		worldModel->Show();
		worldModel->SetModel( model );

		const idDeclModelDef* modelDef = worldModel->GetAnimator()->ModelDef();
		if ( modelDef ) {
			worldModel->SetSkin( modelDef->GetDefaultSkin() );
		}

		jointHandle_t attachJoint = owner->GetAnimator()->GetJointHandle( attach );

		worldModel->Bind( owner, attachJoint );
		worldModel->SetOrigin( vec3_origin );
		worldModel->SetAxis( mat3_identity );
	} else {
		worldModel->SetModel( "" );
		worldModel->Hide();
	}

	idAnimator* worldAnimator = worldModel->GetAnimator();
	barrelJointsWorld[ index ]	= worldAnimator->GetJointHandle( "muzzle" );
}

/*
==============
idWeapon::LinkScriptVariables
==============
*/
void idWeapon::LinkScriptVariables( void ) {
	WEAPON_ATTACK.LinkTo(		*scriptObject, "WEAPON_ATTACK" );
	WEAPON_ALTFIRE.LinkTo(		*scriptObject, "WEAPON_ALTFIRE" );
	WEAPON_MODESWITCH.LinkTo(	*scriptObject, "WEAPON_MODESWITCH" );
	WEAPON_RELOAD.LinkTo(		*scriptObject, "WEAPON_RELOAD" );
	WEAPON_RAISEWEAPON.LinkTo(	*scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	*scriptObject, "WEAPON_LOWERWEAPON" );
	WEAPON_HIDE.LinkTo(			*scriptObject, "WEAPON_HIDE", true  );
}

/*
================
idWeapon::LogTimeUsed
================
*/
void idWeapon::LogTimeUsed( void ) {
	if ( !stats.timeUsed ) {
		return;
	}

	if ( owner == NULL ) {
		return;
	}

	int t = MS2SEC( gameLocal.time - timeStartedUsing );
	stats.timeUsed->IncreaseValue( owner->entityNumber, t );
}

/*
================
idWeapon::SetupAnimClass
================
*/
void idWeapon::SetupAnimClass( const char* prefix ) {
	owner->SetPrefix( ANIMCHANNEL_ALL, idActor::AP_WEAPON_CLASS, spawnArgs.GetString( va( "%s_class", prefix ) ) );
	owner->SetPrefix( ANIMCHANNEL_ALL, idActor::AP_WEAPON, spawnArgs.GetString( prefix ) );
}

/*
================
idWeapon::GetWeaponDef
================
*/
void idWeapon::GetWeaponDef( const sdDeclInvItem* item ) {
	const idDeclSkin* oldSkin = renderEntity.customSkin;
	if ( oldSkin == climateSkin ) {
		oldSkin = NULL;
	}

	Clear();
	
	timeStartedUsing = gameLocal.time;

	renderEntity.flags.weaponDepthHack	= true;	// crunch the depth range so it never pokes into walls this breaks the machine gun gui
	renderEntity.flags.forceUpdate		= true; // force it to update so it moves smoothly (because of epsilon check on angles and origins)

	assert( owner );
	weaponItem			= item;
	spawnArgs			= item->GetData();

	SetupAnimClass( "anim_prefix" );

	feedback.recoilTime		= spawnArgs.GetInt( "recoilTime" );
	feedback.recoilAngles	= spawnArgs.GetAngles( "recoilAngles", "5 0 0" );
	feedback.kickback		= spawnArgs.GetFloat( "kickback" );
	feedback.kickbackProne	= spawnArgs.GetFloat( "kickback_prone" );

	int maxVisDist = spawnArgs.GetInt( "maxVisDist", "2048" );
	spawnArgs.SetInt( "maxVisDist", maxVisDist );

	int numWorldModels = spawnArgs.GetInt( "num_world_models", "1" );
	SetNumWorldModels( numWorldModels );
	
	// jrad - copy these arguments over to the worldModel so we can play sounds and effects
	for ( int i = 0; i < worldModels.Num(); i++ ) {
		worldModels[ i ]->Create( &spawnArgs, NULL );
	}

	muzzle_kick_time	= SEC2MS( spawnArgs.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( spawnArgs.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= spawnArgs.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= spawnArgs.GetVector( "muzzle_kick_offset" );

	hideTime			= SEC2MS( spawnArgs.GetFloat( "hide_time", "0.3" ) );
	hideDistance		= spawnArgs.GetFloat( "hide_distance", "-15" );	

	lockInfo.Load( spawnArgs );
	lockInfo.SetSupported( false );

	int temp;

	if ( spawnArgs.GetInt( "player_weapon_num", "", temp ) ) {
		playerWeaponNum = ( playerWeaponTypes_t ) temp; //mal: get the weapon number, for quick and easy reference for the bots.
	} else {
		playerWeaponNum = NULL_WEAP;
	}

	// setup the view model
	const char* vmodel = spawnArgs.GetString( "model_view" );
	SetModel( vmodel );
	renderEntity.flags.usePointTestForAmbientCubeMaps = true;

	// setup the world model
	for ( int i = 0; i < worldModels.Num(); i++ ) {
		InitWorldModel( i );
	}

	largeFOVScale = spawnArgs.GetFloat( "largeFOVScale", "-1" );

	viewOffset = spawnArgs.GetVector( "view_offset", "0 0 0" );

	viewForeShortenAxis.Identity();
	viewForeShorten = spawnArgs.GetFloat( "view_foreshorten", "1" );

	// find some joints in the model for locating effects
	barrelJointView		= animator.GetJointHandle( "muzzle" );

	const char* damagename = spawnArgs.GetString( "dmg_melee" );
	if ( *damagename ) {
		meleeDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if ( !meleeDamage ) {
			gameLocal.Error( "idWeapon::GetWeaponDef Unknown Damage '%s'", damagename );
		}
	}

	damagename = spawnArgs.GetString( "dmg_melee_special" );
	if ( *damagename ) {
		meleeSpecialDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if ( !meleeSpecialDamage ) {
			gameLocal.Error( "idWeapon::GetWeaponDef Unknown Damage '%s'", damagename );
		}
	}

	if ( !networkSystem->IsDedicated() && renderEntity.hModel != NULL ) {
		// free current gui surfaces
		for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; ) {
			rvClientEntity* next = cent->bindNode.Next();

			sdGuiSurface* guiSurface = cent->Cast< sdGuiSurface >();
			if ( guiSurface != NULL ) {
				guiSurface->Dispose();
			}
			cent = next;
		}	

		// spawn gui entities
		for ( int i = 0; i < renderEntity.hModel->NumGUISurfaces(); i++ ) {
			const guiSurface_t* guiSurface = renderEntity.hModel->GetGUISurface( i );

			const char* guiName = spawnArgs.GetString( "gui" );
			if ( *guiName == '\0' ) {
				continue;
			}

			guiHandle_t handle = gameLocal.LoadUserInterface( guiName, true, false );

			if ( handle.IsValid() ) {
				sdGuiSurface* cent = reinterpret_cast< sdGuiSurface* >( sdGuiSurface::Type.CreateInstance() );

				cent->Init( this, *guiSurface, handle, renderEntity.allowSurfaceInViewID, true );

				sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
				if ( ui != NULL ) {
					ui->SetEntity( this );
					ui->Activate();
				}
			}
		}
	}

	clipBased = spawnArgs.GetBool( "clip_based", "1" );
	usesStroyent = spawnArgs.GetBool( "uses_stroyent", "1" );

	spreadValues[ WSV_STANDING ].min = spawnArgs.GetFloat( "spread_min", "0.5" );
	spreadValues[ WSV_STANDING ].max = spawnArgs.GetFloat( "spread_max", "10" );
	spreadValues[ WSV_STANDING ].inc = spawnArgs.GetFloat( "spread_inc", "1" );
	spreadValues[ WSV_STANDING ].numIgnoreRounds = spawnArgs.GetInt( "spread_ignore", "0" );
	spreadValues[ WSV_STANDING ].maxSettleTime = spawnArgs.GetInt( "spread_max_settle_time", "1000" );
	spreadValues[ WSV_STANDING ].viewRateMin = spawnArgs.GetFloat( "spread_viewrate_min", "30" );
	spreadValues[ WSV_STANDING ].viewRateMax = spawnArgs.GetFloat( "spread_viewrate_max", "150" );
	spreadValues[ WSV_STANDING ].viewRateInc = spawnArgs.GetFloat( "spread_viewrate_inc", "10" );

	spreadValues[ WSV_CROUCHING ].min = spawnArgs.GetFloat( "spread_crouch_min", "0.5" );
	spreadValues[ WSV_CROUCHING ].max = spawnArgs.GetFloat( "spread_crouch_max", "6" );
	spreadValues[ WSV_CROUCHING ].inc = spawnArgs.GetFloat( "spread_crouch_inc", "1" );
	spreadValues[ WSV_CROUCHING ].numIgnoreRounds = spawnArgs.GetInt( "spread_crouch_ignore", "0" );
	spreadValues[ WSV_CROUCHING ].maxSettleTime = spawnArgs.GetInt( "spread_crouch_max_settle_time", "750" );
	spreadValues[ WSV_CROUCHING ].viewRateMin = spawnArgs.GetFloat( "spread_crouch_viewrate_min", "15" );
	spreadValues[ WSV_CROUCHING ].viewRateMax = spawnArgs.GetFloat( "spread_crouch_viewrate_max", "75" );
	spreadValues[ WSV_CROUCHING ].viewRateInc = spawnArgs.GetFloat( "spread_crouch_viewrate_inc", "8" );

	spreadValues[ WSV_PRONE ].min = spawnArgs.GetFloat( "spread_prone_min", "0.5" );
	spreadValues[ WSV_PRONE ].max = spawnArgs.GetFloat( "spread_prone_max", "5" );
	spreadValues[ WSV_PRONE ].inc = spawnArgs.GetFloat( "spread_prone_inc", "1" );
	spreadValues[ WSV_PRONE ].numIgnoreRounds = spawnArgs.GetInt( "spread_prone_ignore", "0" );
	spreadValues[ WSV_PRONE ].maxSettleTime = spawnArgs.GetInt( "spread_prone_max_settle_time", "500" );
	spreadValues[ WSV_PRONE ].viewRateMin = spawnArgs.GetFloat( "spread_prone_viewrate_min", "7.5" );
	spreadValues[ WSV_PRONE ].viewRateMax = spawnArgs.GetFloat( "spread_prone_viewrate_max", "37.5" );
	spreadValues[ WSV_PRONE ].viewRateInc = spawnArgs.GetFloat( "spread_prone_viewrate_inc", "6" );

	spreadValues[ WSV_JUMPING ].min = spawnArgs.GetFloat( "spread_jump_min", "10" );
	spreadValues[ WSV_JUMPING ].max = spawnArgs.GetFloat( "spread_jump_max", "15" );
	spreadValues[ WSV_JUMPING ].inc = spawnArgs.GetFloat( "spread_jump_inc", "1" );
	spreadValues[ WSV_JUMPING ].numIgnoreRounds = spawnArgs.GetInt( "spread_jump_ignore", "0" );
	spreadValues[ WSV_JUMPING ].maxSettleTime = spawnArgs.GetInt( "spread_jump_max_settle_time", "5000" );
	spreadValues[ WSV_JUMPING ].viewRateMin = spawnArgs.GetFloat( "spread_jump_viewrate_min", "60" );
	spreadValues[ WSV_JUMPING ].viewRateMax = spawnArgs.GetFloat( "spread_jump_viewrate_max", "300" );
	spreadValues[ WSV_JUMPING ].viewRateInc = spawnArgs.GetFloat( "spread_jump_viewrate_inc", "10" );

	aimValues[ WAV_IRONSIGHTS ].bobscaleyaw = spawnArgs.GetFloat( "wbobscaleyaw_aim", "-0.01f" );
	aimValues[ WAV_IRONSIGHTS ].bobscalepitch = spawnArgs.GetFloat( "wbobscalepitch_aim", "0.005" );
	aimValues[ WAV_IRONSIGHTS ].lagscaleyaw = spawnArgs.GetFloat( "wlagscaleyaw_aim", "0.9" );
	aimValues[ WAV_IRONSIGHTS ].lagscalepitch = spawnArgs.GetFloat( "wlagscalepitch_aim", "0.9" );
	aimValues[ WAV_IRONSIGHTS ].speedlr = spawnArgs.GetFloat( "wspeedrl_aim", "0" );

	aimValues[ WAV_NORMAL ].bobscaleyaw = spawnArgs.GetFloat( "wbobscaleyaw", "-0.1" );
	aimValues[ WAV_NORMAL ].bobscalepitch = spawnArgs.GetFloat( "wbobscalepitch", "0.2" );
	aimValues[ WAV_NORMAL ].lagscaleyaw = spawnArgs.GetFloat( "wlagscaleyaw", "0.91" );
	aimValues[ WAV_NORMAL ].lagscalepitch = spawnArgs.GetFloat( "wlagscalepitch", "0.91" );
	aimValues[ WAV_NORMAL ].speedlr = spawnArgs.GetFloat( "wspeedrl", "0.01" );

	spreadCurrentValue = spreadValues[ WSV_STANDING ].min;

	crosshairSpreadMin = spawnArgs.GetFloat( "crosshair_spread_min", "0" );
	crosshairSpreadMax = spawnArgs.GetFloat( "crosshair_spread_max", "1" );
	crosshairSpreadScale = spawnArgs.GetFloat( "crosshair_spread_scale", "1" );

	SetupStats( spawnArgs.GetString( "stat_name" ) );

	climateSkin = NULL;
	if ( gameLocal.mapSkinPool != NULL ) {
		const char* skinKey = spawnArgs.GetString( "climate_skin_key" );
		if ( *skinKey != '\0' ) {
			const char* skinName = gameLocal.mapSkinPool->GetDict().GetString( va( "skin_%s", skinKey ) );
			if ( *skinName == '\0' ) {
				gameLocal.Warning( "idWeapon::GetWeaponDef No Skin Set For '%s'", skinKey );
			} else {
				const idDeclSkin* skin = gameLocal.declSkinType[ skinName ];
				if ( skin == NULL ) {
					gameLocal.Warning( "idWeapon::GetWeaponDef Skin '%s' Not Found", skinName );
				} else {
					climateSkin = skin;
				}
			}
		}
	}

	zoomPitchAmplitude = spawnArgs.GetFloat( "zoom_pitch_amplitude" );
	zoomPitchFrequency = spawnArgs.GetFloat( "zoom_pitch_frequency" );
	zoomPitchMinAmplitude = spawnArgs.GetFloat( "zoom_pitch_min_amplitude" );
	zoomYawAmplitude = spawnArgs.GetFloat( "zoom_yaw_amplitude" );
	zoomYawFrequency = spawnArgs.GetFloat( "zoom_yaw_frequency" );
	zoomYawMinAmplitude = spawnArgs.GetFloat( "zoom_yaw_min_amplitude" );

	activateAttack = spawnArgs.GetBool( "activate_attack" );

	spreadValueMax = 0;
	for( int i = 0; i < WSV_NUM; i++ ) {
		if( spreadValueMax < spreadValues[ i ].max ) {
			spreadValueMax = spreadValues[ i ].max;
		}
	}	

	const char* objectType;
	if ( !spawnArgs.GetString( "weapon_scriptobject", NULL, &objectType ) ) {
		gameLocal.Error( "No 'weapon_scriptobject' set on '%s'.", item->GetName() );
	}

	gameLocal.ParseClamp( clampPitch, "clamp_pitch", spawnArgs );
	gameLocal.ParseClamp( clampYaw, "clamp_yaw", spawnArgs );

	// setup script object
	scriptObject = gameLocal.program->AllocScriptObject( this, objectType );
	
	LinkScriptVariables();

	onActivateFunc		= scriptObject->GetFunction( "OnActivate" );
	onActivateFuncHeld	= scriptObject->GetFunction( "OnActivateHeld" );
	onUsedFunc			= scriptObject->GetFunction( "OnUsed" );
	onWeapNextFunc		= scriptObject->GetFunction( "OnWeapNext" );
	onWeapPrevFunc		= scriptObject->GetFunction( "OnWeapPrev" );
	cleanupFunc			= scriptObject->GetFunction( "Cleanup" );

	getGrenadeFuseStartFunc = scriptObject->GetFunction( "GetFuseStart" );
	ironSightsEnabledFunc = scriptObject->GetFunction( "GetIronSightsStatus" );

	dofSkin = declHolder.FindSkin( spawnArgs.GetString( "skin_dof" ), false );

	isLinked = true;

	modelDisabled = false;

	driftScale = 1.f;

	// call script object's constructor
	ConstructScriptObject();

	SetSkin( oldSkin );

	Show();

	sdScriptHelper h1;
	owner->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnWeaponChanged" ), h1 );

	UpdatePlayerWeaponInfo(); //mal: update the player's weapon info

	UpdateVisibility();
	UpdateShadows();
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/

/*
================
idWeapon::SetModel
================
*/
void idWeapon::SetModel( const char *modelname ) {
	assert( modelname );

	if ( !modelname[ 0 ] ) {
		return;
	}

	if ( modelDefHandle >= 0 ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
	}

	renderEntity.hModel = animator.SetModel( modelname );
	if ( renderEntity.hModel ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	} else {
		renderEntity.customSkin = NULL;
		renderEntity.callback = NULL;
		renderEntity.numJoints = 0;
		renderEntity.joints = NULL;
	}
}

/*
================
idWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool idWeapon::GetGlobalJointTransform( bool viewModel, const jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis ) {
	if ( viewModel ) {
		// view model
		if ( animator.GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = offset * viewForeShortenAxis + viewWeaponOrigin;
			axis = axis * viewWeaponAxis;
			return true;
		}
	} else {
		if ( worldModels.Num() > 0 ) { // FIXME
			sdClientAnimated* worldModel = worldModels[ 0 ];

			// world model
			if ( worldModel->GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
				offset = worldModel->GetOrigin() + offset * worldModel->GetAxis();
				axis = axis * worldModel->GetAxis();
				return true;
			}
		}
	}
	offset = viewWeaponOrigin;
	axis = viewWeaponAxis;
	return false;
}

/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
idWeapon::Think
================
*/
void idWeapon::Think( void ) {
}

/*
============
idWeapon::UpdateSpreadValue
============
*/
void idWeapon::UpdateSpreadValue() {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( spreadCurrentValue < spreadValues[ ownerStanceState ].min ) {
		// can happen after stance transitions
		spreadCurrentValue = spreadValues[ ownerStanceState ].min;
	} else if ( spreadCurrentValue > spreadValues[ ownerStanceState ].min ) {
		// decrease spread based on stance
		float minSpread = spreadValues[ ownerStanceState ].min;
		float maxSpread = spreadValues[ ownerStanceState ].max;
		float diffSpread = maxSpread - minSpread;
		float maxSettleTime = spreadValues[ ownerStanceState ].maxSettleTime;

		if ( diffSpread < 0.0001f || maxSettleTime < 0.0001f ) {
			spreadCurrentValue = minSpread;
		} else {
#define SPREAD_FOR_TIME( time ) ( -idMath::Sin( (time) * 0.5f * idMath::PI ) + 1.0f )
#define TIME_FOR_SPREAD( spread ) ( idMath::ASin( -(spread) + 1.0f ) / ( 0.5f * idMath::PI ) )

			float currentSpread = spreadCurrentValue;
			float nextSpread;
			float currentTime;

			if ( spreadCurrentValue > spreadValues[ ownerStanceState ].max ) {
				currentSpread = spreadValues[ ownerStanceState ].max;
				currentTime = 0.0f;
			} else {
				float spreadFraction = ( spreadCurrentValue - minSpread ) / diffSpread;

				if ( spreadFraction < 0.001f ) {
					spreadFraction = 0.0f;
				}

				currentTime = TIME_FOR_SPREAD( spreadFraction );
			}

			nextSpread = minSpread + ( SPREAD_FOR_TIME( currentTime + ( gameLocal.msec / maxSettleTime ) ) * diffSpread );

			float delta = currentSpread - nextSpread;

			spreadCurrentValue -= delta;

			if ( spreadCurrentValue < minSpread + 0.001f ) {
				spreadCurrentValue = minSpread;
			}
		}
	}

	if ( g_debugWeaponSpread.GetBool() ) {
		gameLocal.Printf( "Spread: %.4f Modifier: %.2f\n", spreadCurrentValue, spreadModifier );
	}
}

/*
============
idWeapon::UpdateSpreadValue
============
*/
void idWeapon::UpdateSpreadValue( const idVec3& velocity, const idAngles& angles, const idAngles& oldAngles ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( spreadCurrentValue >= spreadValues[ ownerStanceState ].max ) {
		// spread is bad enough already
		return;
	}

	float viewChange = 0.0f;

	if ( spreadEvalVelocity ) {
		// increase spread based on velocity
		for ( int i = 0; i < 3; i++ ) {
			viewChange += idMath::Fabs( velocity[ i ] );
		}
	}

	if ( spreadEvalView ) {
		// increase spread based on view changes
		for ( int i = 0; i < 2; i++ ) {
			viewChange += idMath::Fabs( angles[ i ] - oldAngles[ i ] );
		}
	}

	float viewRateMin = spreadValues[ ownerStanceState ].viewRateMin;
	float viewRateRange = spreadValues[ ownerStanceState ].viewRateMax - viewRateMin;
	float viewRateInc = spreadValues[ ownerStanceState ].viewRateInc;

	// convert to movement / msec
	if ( gameLocal.msec > 0 ) {
		viewChange /= MS2SEC( gameLocal.msec );
	} else {
		viewChange = 0.0f;
	}

	// clamp
	viewChange = idMath::ClampFloat( 0.0f, viewRateRange, viewChange - viewRateMin ) / viewRateRange;

	// update spread
	spreadCurrentValue += MS2SEC( gameLocal.msec ) * viewChange * viewRateInc;
	if ( spreadCurrentValue > spreadValues[ ownerStanceState ].max ) {
		spreadCurrentValue = spreadValues[ ownerStanceState ].max;
	}
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
bool idWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch ( event ) {
		case EVENT_RELOAD: {
			if ( !weaponItem ) {
				return true;
			}			
			if ( msg.ReadLong() != weaponItem->Index() ) {
				return true;
			}

			if ( scriptObject != NULL ) {
				sdScriptHelper h1;
				scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnNetworkReload" ), h1 );
			}
			return true;
		}
	}
	return idEntity::ClientReceiveEvent( event, time, msg );
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
void idWeapon::Event_SendTracerMessage( const idVec3& start, const idVec3& end, float strength ) {
	if ( !gameLocal.isServer ) {
		return;
	}

	sdEntityBroadcastUnreliableEvent message( this, EVENT_TRACER );
	message.WriteLong( weaponItem->Index() );
	message.WriteVector( start );
	message.WriteVector( end );
	message.WriteFloat( strength );
	message.Send();
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
bool idWeapon::ClientReceiveUnreliableEvent( int event, int time, const idBitMsg &msg ) {
	switch ( event ) {
		case EVENT_TRACER: {
			if ( !weaponItem ) {
				return true;
			}			
			if ( msg.ReadLong() != weaponItem->Index() ) {
				return true;
			}

			if ( scriptObject != NULL ) {
				idVec3 start = msg.ReadVector();
				idVec3 end = msg.ReadVector();
				float strength = msg.ReadFloat();

				sdScriptHelper h1;
				h1.Push( start );
				h1.Push( end );
				h1.Push( strength );
				scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnNetworkTracer" ), h1 );
			}
			return true;
		}
	}
	return false;
}

/*
============
idWeapon::DoStanceTransition
============
*/
void idWeapon::DoStanceTransition( weaponSpreadValueIndex_t oldStanceState ) {
}

/*
================
idWeapon::Raise
================
*/
void idWeapon::Raise( void ) {
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = true;
	}
}

/*
================
idWeapon::PutAway
================
*/
void idWeapon::PutAway( void ) {	
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = true;
	}
}

/*
================
idWeapon::Reload
NOTE: this is only for impulse-triggered reload, auto reload is scripted
================
*/
void idWeapon::Reload( void ) {
	if ( isLinked ) {
		WEAPON_RELOAD = true;
	}
}

/*
================
idWeapon::LowerWeapon
================
*/
void idWeapon::LowerWeapon( void ) {
	if ( hide ) {
		return;
	}

	ScriptCleanup();

	hideStart	= 0.0f;
	hideEnd		= hideDistance;
	if ( gameLocal.time - hideStartTime < hideTime ) {
		hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
	} else {
		hideStartTime = gameLocal.time;
	}
	
	hide = true;
	WEAPON_HIDE = true;
}

/*
================
idWeapon::RaiseWeapon
================
*/
void idWeapon::RaiseWeapon( void ) {
	if ( !hide ) {
		return;
	}

	hideStart	= hideDistance;
	hideEnd		= 0.0f;
	if ( gameLocal.time - hideStartTime < hideTime ) {
		hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
	} else {
		hideStartTime = gameLocal.time;
	}

	hide = false;
	WEAPON_HIDE = false;
}

/*
================
idWeapon::Event_Hide
================
*/
void idWeapon::Event_Hide( void ) {
	modelDisabled = true;
	UpdateVisibility();
}

/*
================
idWeapon::Event_Show
================
*/
void idWeapon::Event_Show( void ) {
	modelDisabled = false;
	UpdateVisibility();
}

/*
================
idWeapon::HideWorldModel
================
*/
void idWeapon::HideWorldModel( void ) {
	worldModelDisabled = true;
	UpdateVisibility();
}

/*
================
idWeapon::ShowWorldModel
================
*/
void idWeapon::ShowWorldModel( void ) {
	worldModelDisabled = false;
	UpdateVisibility();
}

/*
================
idWeapon::OwnerDied
================
*/
void idWeapon::OwnerDied( void ) {
	modelDisabled = true;
	UpdateVisibility();

	if ( scriptObject != NULL ) {
		scriptObject->CallEvent( "OwnerDied" );
	}

	// don't clear the weapon immediately since the owner might have killed himself by firing the weapon
	// within the current stack frame
	PostEventMS( &EV_Clear, 0 );
}

/*
================
idWeapon::BeginAltFire
================
*/
void idWeapon::BeginAltFire( void ) {
	WEAPON_ALTFIRE = true;
}

/*
================
idWeapon::EndAltFire
================
*/
void idWeapon::EndAltFire( void ) {
	WEAPON_ALTFIRE = false;
}

/*
================
idWeapon::BeginModeSwitch
================
*/
void idWeapon::BeginModeSwitch( void ) {
	WEAPON_MODESWITCH = true;
}

/*
================
idWeapon::EndModeSwitch
================
*/
void idWeapon::EndModeSwitch( void ) {
	WEAPON_MODESWITCH = false;
}

/*
================
idWeapon::BeginAttack
================
*/
void idWeapon::BeginAttack( void ) {
	WEAPON_ATTACK = true;
}

/*
================
idWeapon::EndAttack
================
*/
void idWeapon::EndAttack( void ) {
	if ( !WEAPON_ATTACK.IsLinked() ) {
		return;
	}
	
	WEAPON_ATTACK = false;
}

/*
================
idWeapon::isReady
================
*/
bool idWeapon::IsReady( void ) const {
	return ( ( status == WP_RELOAD ) || ( status == WP_READY ) );
}

/*
================
idWeapon::IsReloading
================
*/
bool idWeapon::IsReloading( void ) const {
	return ( status == WP_RELOAD );
}

/*
================
idWeapon::IsHolstered
================
*/
bool idWeapon::IsHolstered( void ) const {
	return ( status == WP_HOLSTERED );
}

/*
================
idWeapon::IsAttacking
================
*/
bool idWeapon::IsAttacking( void ) const {
	return WEAPON_ATTACK != 0;
}

/*
============
idWeapon::spreadCurrentValue
============
*/
float idWeapon::GetSpreadValue( void ) const {
	return spreadCurrentValue;
}

/*
============
idWeapon::GetSpreadValueNormalized
============
*/
float idWeapon::GetSpreadValueNormalized( bool useGlobalMax ) const {
	if ( useGlobalMax ) { 
		if ( spreadValueMax > 0.0f ) {
			return spreadCurrentValue / spreadValueMax;
		} else {
			return 0.0f;
		}
	}
	
	if ( spreadValues[ ownerStanceState ].max > 0.0f ) {
		return spreadCurrentValue / spreadValues[ ownerStanceState ].max;
	} else {
		return 0.0f;
	}
}


/*
============
idWeapon::SetOwnerStanceState
============
*/
void idWeapon::SetOwnerStanceState( weaponSpreadValueIndex_t state ) {
	if ( ownerStanceState == state ) {
		return;
	}
	
	weaponSpreadValueIndex_t oldState = ownerStanceState;
	ownerStanceState = state;
	DoStanceTransition( oldState );
}

/***********************************************************************

	Script state management

***********************************************************************/

/*
=====================
idWeapon::SetState
=====================
*/
void idWeapon::SetState( const char *statename, int blendFrames ) {
	if ( !isLinked ) {
		return;
	}

	const sdProgram::sdFunction* func = scriptObject->GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject->GetTypeName() );
	}

	thread->CallFunction( scriptObject, func );
	state = statename;

	animBlendFrames = blendFrames;
	if ( g_debugWeapon.GetBool() ) {
		gameLocal.Printf( "%d: weapon state : %s\n", gameLocal.time, statename );
	}

	idealState = "";
}

/***********************************************************************

	Visual presentation

***********************************************************************/

/*
================
idWeapon::MuzzleRise

The machinegun and chaingun will incrementally back up as they are being fired
================
*/
void idWeapon::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
	int			time;
	float		amount;
	idAngles	ang;
	idVec3		offset;

	time = kick_endtime - gameLocal.time;
	if ( time <= 0 ) {
		return;
	}

	if ( muzzle_kick_maxtime <= 0 ) {
		return;
	}

	if ( time > muzzle_kick_maxtime ) {
		time = muzzle_kick_maxtime;
	}
	
	amount = ( float )time / ( float )muzzle_kick_maxtime;
	ang		= muzzle_kick_angles * amount;
	offset	= muzzle_kick_offset * amount;

	origin = origin - axis * offset;
	axis = ang.ToMat3() * axis;
}

/*
================
idWeapon::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
sdProgramThread* idWeapon::ConstructScriptObject( void ) {
	thread->EndThread();

	// init the script object's data
	scriptObject->ClearObject();

	sdScriptHelper h;
	CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h );

	// call script object's constructor
	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for weapon", scriptObject->GetTypeName() );
	}

	thread->CallFunction( scriptObject, constructor );
	thread->Execute();

	return thread;
}

/*
================
idWeapon::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idWeapon::DeconstructScriptObject( void ) {
	FixupRemoteCamera();

	onActivateFunc		= NULL;
	onUsedFunc			= NULL;
	onWeapNextFunc		= NULL;
	onWeapPrevFunc		= NULL;
	cleanupFunc			= NULL;
	getGrenadeFuseStartFunc = NULL;
	ironSightsEnabledFunc	= NULL;

	if ( !scriptObject ) {
		return;
	}

	if ( thread ) {
		thread->EndThread();
	
		// call script object's destructor
		const sdProgram::sdFunction* destructor = scriptObject->GetDestructor();
		if ( destructor ) {
			// start a thread that will run immediately and end
			sdScriptHelper helper;
			CallNonBlockingScriptEvent( destructor, helper );
		}
	}

	gameLocal.program->FreeScriptObject( scriptObject );
}

idCVar g_debugWeaponState( "g_debugWeaponState", "-1", CVAR_GAME | CVAR_INTEGER, "shows the current weapon state for the client index specified" );

/*
================
idWeapon::UpdateScript
================
*/
void idWeapon::UpdateScript( void ) {
	if ( !isLinked || gameLocal.IsPaused() ) {
		return;
	}

	if ( g_debugWeaponState.GetInteger() == owner->entityNumber ) {
		gameLocal.Printf( "Weapon ('%s') State for Client %d: '%s'\n", weaponItem != NULL ? weaponItem->GetName() : "<none>", owner->entityNumber, state.c_str() );
	}

	if ( idealState.Length() ) {
		SetState( idealState, animBlendFrames );
	}

	// update script state, which may call Event_LaunchProjectiles, among other things
	int count = 10;
	while( ( thread->Execute() || idealState.Length() ) && count-- ) {
		if ( !idealState.Length() ) {
			state = "";
			break;
		}

		SetState( idealState, animBlendFrames );
	}

	WEAPON_RELOAD = false;
}

/*
================
idWeapon::Present
================
*/
void idWeapon::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

#if 0
	// if set to invisible, skip
	if ( renderEntity.hModel && !IsHidden() ) {
		dofRenderEntity = renderEntity;
		dofRenderEntity.allowSurfaceInViewID = DOF_VIEWWEAPON_VIEW_ID;
		dofRenderEntity.customSkin = dofSkin;

		// add to refresh list
		if ( dofModelDefHandle == -1 ) {
			dofModelDefHandle = gameRenderWorld->AddEntityDef( &dofRenderEntity );
		} else {
			gameRenderWorld->UpdateEntityDef( dofModelDefHandle, &dofRenderEntity );
		}
	}
#endif

	idEntity::Present();
}

/*
================
idWeapon::FreeModelDef
================
*/
void idWeapon::FreeModelDef( void ) {
	if ( dofModelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( dofModelDefHandle );
		dofModelDefHandle = -1;
	}
	idEntity::FreeModelDef();
}

/*
================
idWeapon::PresentWeapon
================
*/
void idWeapon::PresentWeapon( void ) {
	playerViewOrigin	= owner->firstPersonViewOrigin;
	playerViewAxis		= owner->firstPersonViewAxis;

	bool ignorePitch = weaponItem ? weaponItem->GetIgnoreViewPitch() : false;
	if ( ignorePitch ) {
		// ignore the pitch component of the view
		playerViewAxis[ 0 ].z = 0.0f;
		playerViewAxis[ 1 ].z = 0.0f;
		playerViewAxis[ 2 ].Set( 0.0f, 0.0f, 1.0f );
		playerViewAxis[ 0 ].Normalize();
		playerViewAxis[ 1 ].Normalize();
	}

	// calculate weapon position based on player movement bobbing
	owner->CalculateViewWeaponPos( viewWeaponOrigin, viewWeaponAxis, ignorePitch );

	// hide offset is for dropping the gun when approaching a GUI or NPC
	// This is simpler to manage than doing the weapon put-away animation
	if ( gameLocal.time - hideStartTime < hideTime ) {		
		float frac = ( float )( gameLocal.time - hideStartTime ) / ( float )hideTime;
		if ( hideStart < hideEnd ) {
			frac = 1.0f - frac;
			frac = 1.0f - frac * frac;
		} else {
			frac = frac * frac;
		}
		hideOffset = hideStart + ( hideEnd - hideStart ) * frac;
	} else {
		hideOffset = hideEnd;
		if ( hide && disabled ) {
			Hide();
		}
	}
	viewWeaponOrigin += hideOffset * viewWeaponAxis[ 2 ];

	// kick up based on repeat firing
	MuzzleRise( viewWeaponOrigin, viewWeaponAxis );

	// calculate the foreshortened axis
	viewForeShortenAxis = viewWeaponAxis;
	viewForeShortenAxis[0] *= viewForeShorten;

	// set the physics position and orientation
	GetPhysics()->SetOrigin( viewWeaponOrigin );
	GetPhysics()->SetAxis( viewForeShortenAxis );

	UpdateVisuals();

	if ( gameLocal.isNewFrame ) {
		UpdateAnimation();
		Present();
	}
}

/*
============
idWeapon::UpdateShadows
============
*/
void idWeapon::UpdateShadows( void ) {
	UpdateVisibility();
}

/*
============
idWeapon::SwayAngles
============
*/
void idWeapon::SwayAngles( idAngles& angles ) const {
	if ( !swayEnabled ) {
		return;
	}

	float phase;

	float swayScale = owner->GetSwayScale();

	phase = MS2SEC( gameLocal.time ) * zoomPitchFrequency * idMath::PI * 2.0f;
	angles[ PITCH ] += zoomPitchAmplitude * idMath::Sin( phase ) * ( zoomPitchMinAmplitude ) * swayScale;

	phase = MS2SEC( gameLocal.time ) * zoomYawFrequency * idMath::PI * 2.0f;
	angles[ YAW ] += zoomYawAmplitude * idMath::Sin( phase ) * ( zoomYawMinAmplitude ) * swayScale;
}

/***********************************************************************

	Ammo

***********************************************************************/

/*
================
idWeapon::GetAmmoType
================
*/
ammoType_t idWeapon::GetAmmoType( const char *ammoname ) {
	const sdDeclAmmoType* ammoDecl = gameLocal.declAmmoTypeType.LocalFind( ammoname, false );
	if( !ammoDecl ) {
		gameLocal.Error( "idWeapon::GetAmmoType Invalid Ammo Type %s", ammoname );
	}
	return ammoDecl->GetAmmoType();
}

/*
================
idWeapon::AmmoAvailable
================
*/
int idWeapon::ShotsAvailable( int modIndex ) const {	
	if ( !weaponItem || modIndex >= weaponItem->GetClips().Num() ) {
		return -1;
	}

	const itemClip_t& clipInfo = weaponItem->GetClips()[ modIndex ];

	if ( clipInfo.ammoPerShot <= 0 ) {
		return -1;
	}

	return Max( 0, ( owner != NULL ? owner->AmmoForWeapon( modIndex ) : 0 ) / clipInfo.ammoPerShot );
}

/*
================
idWeapon::GetAmmoType
================
*/
ammoType_t idWeapon::GetAmmoType( int modIndex ) const {
	if ( !weaponItem || modIndex >= weaponItem->GetClips().Num() ) {
		return ( ammoType_t )( -1 );
	}
	return weaponItem->GetClips()[ modIndex ].ammoType;
}

/*
================
idWeapon::GetClipSize
================
*/
int	idWeapon::GetClipSize( int modIndex ) const {
	if ( !weaponItem || modIndex >= weaponItem->GetClips().Num() ) {
		return 0;
	}
	return weaponItem->GetClips()[ modIndex ].maxAmmo;
}

/*
================
idWeapon::LowAmmo
================
*/
int	idWeapon::LowAmmo( int modIndex ) const {
	if ( !weaponItem || modIndex >= weaponItem->GetClips().Num() ) {
		return 0;
	}
	return weaponItem->GetClips()[ modIndex ].lowAmmo;
}

/*
================
idWeapon::AmmoRequired
================
*/
int	idWeapon::AmmoRequired( int modIndex ) const {
	if ( !weaponItem || modIndex >= weaponItem->GetClips().Num() ) {
		return 0;
	}
	return weaponItem->GetClips()[ modIndex ].ammoPerShot;
}

/*
================
idWeapon::IsClipBased
================
*/
bool idWeapon::IsClipBased() const {
	return clipBased;
}

/*
================
idWeapon::UsesStroyent
================
*/
bool idWeapon::UsesStroyent() const {
	return usesStroyent;
}

/*
================
idWeapon::UpdateVisibility
================
*/
void idWeapon::UpdateVisibility( void ) {
	viewState_t viewModelState = VS_NONE;
	viewState_t worldViewState = VS_FULL;

	int entityNumber = -1;
	viewState_t shadowState = VS_NONE;

	if ( owner ) {
		entityNumber	= owner->entityNumber;
		shadowState		= owner->HasShadow();

		if ( owner->IsBeingBriefed() ) {
			worldViewState = VS_NONE;
		} else if ( owner->IsSpectating() ) {
			worldViewState	= VS_NONE;
		} else if ( idEntity* proxy = owner->GetProxyEntity() ) {
			if ( !proxy->GetUsableInterface()->GetAllowPlayerWeapon( owner ) ) {
				worldViewState	= VS_NONE;
			} else {
				if ( gameLocal.IsLocalViewPlayer( owner ) ) {
					worldViewState = VS_REMOTE;
					viewModelState = VS_FULL;
				}
			}
		} else if ( owner->IsDead() ) {
			worldViewState	= VS_NONE;
		} else if ( gameLocal.IsLocalViewPlayer( owner ) ) {
			worldViewState = VS_REMOTE;
			viewModelState = VS_FULL;
		}
	} else {
		worldViewState	= VS_NONE;
	}

	if ( modelDisabled ) {
		viewModelState = VS_NONE;
	}
	
	if ( worldModelDisabled ) {
		worldViewState = VS_NONE;
	}

	if ( !ui_showGun.GetBool() ) {
		viewModelState = VS_NONE;
	}

	switch ( viewModelState ) {
		case VS_NONE:
			Hide();
			break;
		default:
			Show();
			break;
	}

	for ( int i = 0; i < worldModels.Num(); i++ ) {
		sdClientAnimated* worldModel = worldModels[ i ];
		if ( worldModel == NULL ) {
			continue;
		}

		renderEntity_t* worldRenderEntity = worldModel->GetRenderEntity();

		switch ( worldViewState ) {
			case VS_NONE: {
				worldModel->Hide();
				break;
			}
			case VS_REMOTE: {
				worldModel->Show();
				worldRenderEntity->suppressSurfaceInViewID	= entityNumber + 1;
				break;
			}

			case VS_FULL: {
				worldModel->Show();
				worldRenderEntity->suppressSurfaceInViewID = 0;
				break;
			}
		}

		switch ( shadowState ) {
			case VS_NONE: {
				worldRenderEntity->suppressShadowInViewID	= 0;
				worldRenderEntity->flags.noShadow			= true;
				break;
			}
			case VS_REMOTE: {
				worldRenderEntity->suppressShadowInViewID	= owner->entityNumber + 1;
				worldRenderEntity->flags.noShadow			= false;
				break;
			}
			case VS_FULL: {
				worldRenderEntity->suppressShadowInViewID = 0;
				worldRenderEntity->flags.noShadow			= false;
				break;
			}
		}
	}
}

/*
================
idWeapon::OnProxyEnter
================
*/
void idWeapon::OnProxyEnter( void ) {
	PutAway();

	UpdateVisibility();

	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnProxyEnter" ), h1 );
	}
}

/*
================
idWeapon::OnProxyExit
================
*/
void idWeapon::OnProxyExit( void ) {
	RaiseWeapon();

	UpdateVisibility();

	if ( scriptObject ) {
		sdScriptHelper h1;
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnProxyExit" ), h1 );
	}
}

/*
================
idWeapon::SetSkin
================
*/
void idWeapon::SetSkin( const idDeclSkin* skin ) {
	if ( skin == NULL ) {
		renderEntity.customSkin = climateSkin;
	} else {
		renderEntity.customSkin = skin;
	}
	UpdateVisuals();

	for ( int i = 0; i < worldModels.Num(); i++ ) {
		sdClientAnimated* worldModel = worldModels[ i ];

		worldModel->SetSkin( skin );
	}
}

/***********************************************************************

	Script events

***********************************************************************/

/*
===============
idWeapon::Event_Clear
===============
*/
void idWeapon::Event_Clear( void ) {
	Clear();
}

/*
===============
idWeapon::Event_GetOwner
===============
*/
void idWeapon::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( owner );
}

/*
===============
idWeapon::Event_WeaponState
===============
*/
void idWeapon::Event_WeaponState( const char *statename, int blendFrames ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject->GetTypeName() );
	}

	idealState = statename;
	animBlendFrames = blendFrames;
	thread->DoneProcessing();
}

/*
===============
idWeapon::Event_GetWeaponState
===============
*/
void idWeapon::Event_GetWeaponState( void ) {
	if ( idealState.Length() ) {
		sdProgram::ReturnString( idealState );
		return;
	}
	sdProgram::ReturnString( state );		
}

/*
===============
idWeapon::Event_WeaponReady
===============
*/
void idWeapon::Event_WeaponReady( void ) {
	status = WP_READY;
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
}

/*
===============
idWeapon::Event_WeaponReloading
===============
*/
void idWeapon::Event_WeaponReloading( void ) {
	status = WP_RELOAD;

	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( this, EVENT_RELOAD );
		msg.WriteLong( weaponItem->Index() );
		msg.Send( false, sdReliableMessageClientInfoAll() );
	}
}

/*
===============
idWeapon::Event_WeaponHolstered
===============
*/
void idWeapon::Event_WeaponHolstered( void ) {
	status = WP_HOLSTERED;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
}

/*
===============
idWeapon::SetInstantSwitch
===============
*/
void idWeapon::InstantSwitch( void ) {
	// HACK - this tricks the weapon updating code to think that this weapon is ready to change
	Event_WeaponHolstered();
}

/*
=========================
idWeapon::CanAttack
=========================
*/
bool idWeapon::CanAttack( void ) {
	return true;
}

/*
===============
idWeapon::Event_WeaponRising
===============
*/
void idWeapon::Event_WeaponRising( void ) {
	status = WP_RISING;
	if ( isLinked ) {
		WEAPON_LOWERWEAPON = false;
	}
	owner->WeaponRisingCallback();
}

/*
===============
idWeapon::Event_WeaponLowering
===============
*/
void idWeapon::Event_WeaponLowering( void ) {
	status = WP_LOWERING;
	if ( isLinked ) {
		WEAPON_RAISEWEAPON = false;
	}
	owner->WeaponLoweringCallback();
}

/*
===============
idWeapon::Event_AddToClip
===============
*/
void idWeapon::Event_AddToClip( int modIndex, int amount ) {
	if ( modIndex >= weaponItem->GetClips().Num() ) {
		return;
	}

	int clip = owner->GetClip( modIndex );

	const itemClip_t& clipInfo = weaponItem->GetClips()[ modIndex ];

	clip += amount;
	clip = Min( clip, clipInfo.maxAmmo );

	int ammoAvail = owner->AmmoForWeapon( modIndex );
	clip = Min( clip, ammoAvail );

	owner->SetClip( modIndex, clip );
}

/*
===============
idWeapon::Event_AmmoInClip
===============
*/
void idWeapon::Event_AmmoInClip( int modIndex ) {
	sdProgram::ReturnInteger( owner->GetClip( modIndex ) );
}

/*
===============
idWeapon::Event_AmmoAvailable
===============
*/
void idWeapon::Event_AmmoAvailable( int modIndex ) {
	sdProgram::ReturnInteger( ShotsAvailable( modIndex ) );
}

/*
===============
idWeapon::Event_AmmoRequired
===============
*/
void idWeapon::Event_AmmoRequired( int modIndex ) {
	sdProgram::ReturnInteger( AmmoRequired( modIndex ) );
}

/*
===============
idWeapon::Event_AmmoType
===============
*/
void idWeapon::Event_AmmoType( int modIndex ) {
	sdProgram::ReturnInteger( GetAmmoType( modIndex ) );
}

/*
===============
idWeapon::Event_UseAmmo
===============
*/
void idWeapon::Event_UseAmmo( ammoType_t type, int amount ) {
	if ( type == -1 ) {
		gameLocal.Warning( "idWeapon::Event_UseAmmo Invalid Ammo Type" );
		return;
	}

	owner->UseAmmo( type, amount );
}

/*
===============
idWeapon::Event_ClipSize
===============
*/
void idWeapon::Event_ClipSize( int modIndex ) {
	sdProgram::ReturnInteger( GetClipSize( modIndex ) );
}

/*
===============
idWeapon::Event_EnableTargetLock
===============
*/
void idWeapon::Event_EnableTargetLock( bool enable ) {
	lockInfo.SetSupported( enable );
}

/*
===============
idWeapon::Event_EnableSway
===============
*/
void idWeapon::Event_EnableSway( bool enable ) {
	swayEnabled = ( enable );
}

/*
===============
idWeapon::Event_SetDriftScale
===============
*/
void idWeapon::Event_SetDriftScale( float scale ) {
	driftScale = scale;
}

/*
===============
idWeapon::Event_ResetTracerCounter
===============
*/
void idWeapon::Event_ResetTracerCounter() { 
	tracerCounter = 0;
}

/*
===============
idWeapon::Event_SetFovStart
===============
*/
void idWeapon::Event_SetFovStart( float startFov, float endFov, float startTime, float duration ) {
	zoomFov.Init( MS2SEC( gameLocal.time ) + startTime, duration, startFov, endFov );
}

/*
===============
idWeapon::Event_SetFov
===============
*/
void idWeapon::Event_SetFov( float startFov, float endFov, float duration ) {
	zoomFov.Init( MS2SEC( gameLocal.time ), duration, startFov, endFov );
}

/*
===============
idWeapon::Event_ClearFov
===============
*/
void idWeapon::Event_ClearFov( void ) {
	zoomFov.Init( 0, 0, 0, 0 );
}

/*
===============
idWeapon::Event_EnableClamp
===============
*/
void idWeapon::Event_EnableClamp( const idAngles& baseAngles ) {
	clampBaseAngles = baseAngles;
	clampEnabled	= true;
}

/*
===============
idWeapon::Event_DisableClamp
===============
*/
void idWeapon::Event_DisableClamp( void ) {
	clampEnabled	= false;
}

/*
===============
idWeapon::Event_PlayAnim
===============
*/
void idWeapon::Event_PlayAnim( animChannel_t channel, const char *animname ) {
	int anim;
	
	anim = animator.GetAnim( animname );
	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();

		for ( int i = 0; i < worldModels.Num(); i++ ) {
			sdClientAnimated* worldModel = worldModels[ i ];

			idAnimator* worldAnimator = worldModel->GetAnimator();
			anim = worldAnimator->GetAnim( animname );
			if ( anim ) {
				worldAnimator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
			}
		}
	}
	animBlendFrames = 0;
	sdProgram::ReturnFloat( 0.0f );
}

/*
===============
idWeapon::Event_PlayCycle
===============
*/
void idWeapon::Event_PlayCycle( animChannel_t channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();

		for ( int i = 0; i < worldModels.Num(); i++ ) {
			sdClientAnimated* worldModel = worldModels[ i ];
			if ( worldModel == NULL ) {
				continue;
			}

			idAnimator* worldAnimator = worldModel->GetAnimator();

			anim = worldAnimator->GetAnim( animname );
			worldAnimator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
	}
	animBlendFrames = 0;
	sdProgram::ReturnFloat( 0.0f );
}

/*
===============
idWeapon::Event_AnimDone
===============
*/
void idWeapon::Event_AnimDone( animChannel_t channel, int blendFrames ) {
	if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		sdProgram::ReturnBoolean( true );
	} else {
		sdProgram::ReturnBoolean( false );
	}
}

/*
===============
idWeapon::Event_SetBlendFrames
===============
*/
void idWeapon::Event_SetBlendFrames( animChannel_t channel, int blendFrames ) {
	animBlendFrames = blendFrames;
}

/*
===============
idWeapon::Event_GetBlendFrames
===============
*/
void idWeapon::Event_GetBlendFrames( animChannel_t channel ) {
	sdProgram::ReturnInteger( animBlendFrames );
}

/*
================
idWeapon::Event_Next
================
*/
void idWeapon::Event_Next( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	// change to another weapon if possible
	owner->GetInventory().SelectBestWeapon( false );
}

/*
================
idWeapon::Event_CreateProjectile
================
*/
void idWeapon::Event_CreateProjectile( int modIndex ) {
	if ( modIndex >= weaponItem->GetClips().Num() ) {
		return;
	}

	if ( gameLocal.isClient ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	if( !weaponItem->GetClips()[ modIndex ].projectile ) {
		gameLocal.Warning( "CreateProjectile: projectile for clip %d NULL. Possibly out of range?", modIndex );
		sdProgram::ReturnEntity( NULL );
		return;
	}

	projectileEnt = NULL;
	gameLocal.SpawnEntityDef( weaponItem->GetClips()[ modIndex ].projectile->dict, true, &projectileEnt );
	if ( projectileEnt ) {
		projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
		projectileEnt->Bind( owner, false );
		projectileEnt->Hide();
		projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
		sdProgram::ReturnEntity( projectileEnt );
	} else {
		sdProgram::ReturnEntity( NULL );
	}
}

/*
================
idWeapon::Event_LaunchProjectiles
================
*/
void idWeapon::Event_LaunchProjectiles( int numProjectiles, int projectileIndex, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	if ( projectileIndex < 0 || projectileIndex >= weaponItem->GetClips().Num() ) {
		return;
	}

	const itemClip_t& projectileInfo = weaponItem->GetClips()[ projectileIndex ];

	if ( !projectileInfo.projectile ) {
		return;
	}

	const idDict& projectileDict = projectileInfo.projectile->dict;
	bool isBullet = projectileDict.GetBool( "is_bullet" );

	missileRandom.SetSeed( gameLocal.time );

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] = missileRandom.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	for ( int i = 0; i < worldModels.Num(); i++ ) {
		sdClientAnimated* worldModel = worldModels[ i ];

		renderEntity_t* worldRenderEnt = worldModel->GetRenderEntity();
		worldRenderEnt->shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];
		worldRenderEnt->shaderParms[ SHADERPARM_TIMEOFFSET ] = renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ];
	}

	// the muzzle bone's position, used for launching projectiles and trailing smoke
	idVec3 muzzleOrigin;
	idMat3 muzzleAxis;

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	} else {
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;
	}

	// calculate the muzzle position for tracers
	jointHandle_t barrelJoint;
	bool viewModel;
	
	if ( gameLocal.GetLocalViewPlayer() == GetOwner() && !pm_thirdPerson.GetBool() ) {
		barrelJoint = barrelJointView;
		viewModel = true;
	} else {
		if ( worldModels.Num() > 0 ) {
			barrelJoint = barrelJointsWorld[ 0 ]; // FIXME
			viewModel = false;
		} else {
			barrelJoint = INVALID_JOINT;
			viewModel = false;
		}
	}

	idVec3 tracerMuzzleOrigin;
	idMat3 tracerMuzzleAxis;
	if ( barrelJoint != INVALID_JOINT && projectileDict.GetBool( "tracerFromBarrel" ) ) {
		GetGlobalJointTransform( viewModel, barrelJoint, tracerMuzzleOrigin, tracerMuzzleAxis );
	} else {
		tracerMuzzleOrigin = muzzleOrigin;
		tracerMuzzleAxis = muzzleAxis;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.time ) {
		kick_endtime = gameLocal.time;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.time + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.time + muzzle_kick_maxtime;
	}

	{
		idBounds ownerBounds = owner->GetPhysics()->GetAbsBounds();

		if ( stats.shotsFired != NULL ) {
			stats.shotsFired->IncreaseValue( owner->entityNumber, numProjectiles );
		}
			
		float spreadRad = DEG2RAD( spread );
		for ( int i = 0; i < numProjectiles; i++ ) {
			float ang = idMath::Sin( spreadRad * missileRandom.RandomFloat() );
			float spin = ( float )DEG2RAD( 360.0f ) * missileRandom.RandomFloat();

			idVec3 dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			if ( isBullet ) {
				// TODO: move this to a better place
				if ( projectileEnt != NULL ) {
					gameLocal.Error( "Can't call createProjectile with a bullet with \"is_bullet\" set" );
				}

				bool forceTracer = false;
				int tracerInterval = projectileDict.GetInt( "tracer_interval" );
				
				if ( tracerInterval > 0 ) {
					if ( tracerCounter % tracerInterval == 0 ) {
						forceTracer = true;
					}
				}

				rvClientEffect* tracerEffect = NULL;
				LaunchBullet( owner, owner, projectileDict, muzzleOrigin, dir.ToMat3(), tracerMuzzleOrigin, tracerMuzzleAxis, dmgPower, forceTracer, &tracerEffect );
				lastTracer = tracerEffect;

				tracerCounter++;
			} else {
				if ( gameLocal.isClient ) {
					if ( projectileInfo.clientProjectile.Length() ) {
						sdClientProjectile* clientEntity = new sdClientProjectile();
						clientEntity->CreateByName( &projectileDict, projectileInfo.clientProjectile );
						clientEntity->SetOrigin( muzzleOrigin );
						clientEntity->SetAxis( dir.ToMat3() );
						clientEntity->Launch( owner, tracerMuzzleOrigin, tracerMuzzleAxis );
					}
				} else {
					idEntity* ent = NULL;
					if ( projectileEnt != NULL ) {
						ent = projectileEnt;
						projectileEnt->Show();
						projectileEnt->Unbind();
						projectileEnt = NULL;
					} else {
						gameLocal.SpawnEntityDef( projectileDict, true, &ent );
					}

					// jrad - FIXME: switching on type...remove this legacy DOOM3 stuff...
					if ( ent == NULL ) {
						gameLocal.Error( "'%s' is not a valid projectile", projectileInfo.projectile->GetName() );
					}

					float distance;
					idVec3 startPos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
					idBounds projBounds = ent->GetPhysics()->GetBounds().Rotate( playerViewAxis );
					if ( ( ownerBounds - projBounds ).RayIntersection( startPos, playerViewAxis[ 0 ], distance ) ) {
						startPos += distance * playerViewAxis[ 0 ];
					}

					idBounds newProjBounds = projBounds;
					newProjBounds.TranslateSelf( startPos );
					if ( !ownerBounds.ContainsBounds( newProjBounds ) ) {
						// goes outside the player's bounds
						idClipModel* clipModel = ent->GetPhysics()->GetClipModel();
						int contentMask = ent->GetPhysics()->GetClipMask();
						if ( gameLocal.clip.Contents( CLIP_DEBUG_PARMS_SCRIPT startPos, clipModel, playerViewAxis, contentMask, owner ) ) {
							// its embedded in something it clips against!
							// start from a point behind the muzzle and trace forwards to find the contact point
							float halfbbox = pm_bboxwidth.GetFloat() * 0.5f;
							trace_t	trace;
							gameLocal.clip.Translation( CLIP_DEBUG_PARMS_SCRIPT trace, muzzleOrigin - ( halfbbox + distance ) * playerViewAxis[ 0 ], 
														startPos, clipModel, playerViewAxis, contentMask, owner );

							startPos = trace.endpos - playerViewAxis[ 0 ] * 0.25f;

							// NOTE: to be truly rigorous we should check here if its still inside something 
							//       and then kill the projectile or something like that, as its really an
							//       invalid position to launch the projectile from
						}
					}

					if ( ent->IsType( idProjectile::Type ) ) {

						idProjectile* proj = ent->Cast< idProjectile >();
						proj->Create( owner, startPos, dir );
						proj->AddOwner( owner );
						idEntity* proxy = owner->GetProxyEntity();
						if ( proxy ) {
							proj->AddOwner( proxy );
						}
						proj->SetEnemy( owner->GetEnemy() );
						proj->Launch( startPos, dir, vec3_zero, fuseOffset, launchPower, dmgPower );

					} else if ( ent->IsType( idItem::Type ) ) {
						idItem* item = ent->Cast< idItem >();

						idVec3 pushVelocity = dir * item->spawnArgs.GetFloat( "speed", "250" );



						item->SetDropper( owner );
						item->SetGameTeam( owner->GetGameTeam() );
						item->SetOrigin( startPos );
						idAngles angles = playerViewAxis.ToAngles();
						angles.pitch = 0.0f;
						angles.roll = 0.0f;
						idMat3 axis = angles.ToMat3();
						item->SetAxis( axis );
						item->GetPhysics()->SetLinearVelocity( pushVelocity );
						item->SetPickupTime( gameLocal.time + SEC2MS( item->spawnArgs.GetFloat( "wait_time", "1" ) ) );
						item->PostEventMS( &EV_Remove, item->spawnArgs.GetInt( "life_time" ) );

					} else if ( ent->IsType( sdScriptEntity_Projectile::Type ) && 
								( ent->GetPhysics()->IsType( idPhysics_RigidBody::Type ) || ent->GetPhysics()->IsType( sdPhysics_SimpleRigidBody::Type ) ) ) {
						sdScriptEntity_Projectile* scriptEntity = ent->Cast< sdScriptEntity_Projectile >();

						scriptEntity->SetGameTeam( owner->GetGameTeam() );
						scriptEntity->Create( owner, startPos, dir );
					} else {
						gameLocal.Error( "'%s' is not a valid projectile", projectileInfo.projectile->GetName() );
					}
				}
			}
		}
	}

	owner->WeaponFireFeedback( feedback );
}

/*
=====================
idWeapon::Event_DoProjectileTracer
=====================
*/
void idWeapon::Event_DoProjectileTracer( int projectileIndex, const idVec3& start, const idVec3& end ) {
	if ( projectileIndex < 0 || projectileIndex >= weaponItem->GetClips().Num() ) {
		return;
	}

	const itemClip_t& projectileInfo = weaponItem->GetClips()[ projectileIndex ];

	if ( !projectileInfo.projectile ) {
		return;
	}

	const idDict& projectileDict = projectileInfo.projectile->dict;
	bool isBullet = projectileDict.GetBool( "is_bullet" );

	// calculate the muzzle position
	idVec3 tracerMuzzleOrigin;
	idMat3 tracerMuzzleAxis;
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, tracerMuzzleOrigin, tracerMuzzleAxis );
	} else {
		// go straight out of the view
		tracerMuzzleOrigin = playerViewOrigin;
		tracerMuzzleAxis = playerViewAxis;
	}

	// calculate the muzzle position for tracers
	jointHandle_t barrelJoint;
	bool viewModel;
	
	if ( gameLocal.GetLocalViewPlayer() == GetOwner() && !pm_thirdPerson.GetBool() ) {
		barrelJoint = barrelJointView;
		viewModel = true;
	} else {
		if ( worldModels.Num() > 0 ) {
			barrelJoint = barrelJointsWorld[ 0 ]; // FIXME
			viewModel = false;
		} else {
			barrelJoint = INVALID_JOINT;
			viewModel = false;
		}
	}

	if ( barrelJoint != INVALID_JOINT && projectileDict.GetBool( "tracerFromBarrel" ) ) {
		GetGlobalJointTransform( viewModel, barrelJoint, tracerMuzzleOrigin, tracerMuzzleAxis );
	}

	// draw a tracer
	int effectHandle = gameLocal.GetEffectHandle( projectileDict, "fx_tracer", "" );
	idVec3 dir = end - tracerMuzzleOrigin;
	dir.Normalize();

	lastTracer = gameLocal.PlayEffect( effectHandle, idVec3( 1.0f, 1.0f, 1.0f ), tracerMuzzleOrigin, dir.ToMat3(), false, end ); 
}

/*
=====================
idWeapon::Event_SaveMeleeTrace
=====================
*/
void idWeapon::Event_SaveMeleeTrace( void ) {
	sdLoggedTrace* trace = gameLocal.RegisterLoggedTrace( meleeTrace );
	sdProgram::ReturnObject( trace ? trace->GetScriptObject() : NULL );
}

/*
================
idWeapon::Event_GetMeleeFraction
================
*/
void idWeapon::Event_GetMeleeFraction( void ) {
	sdProgram::ReturnFloat( meleeTrace.fraction );
}

/*
================
idWeapon::Event_GetMeleeEndPos
================
*/
void idWeapon::Event_GetMeleeEndPos( void ) {
	sdProgram::ReturnVector( meleeTrace.endpos );
}

/*
================
idWeapon::Event_GetMeleePoint
================
*/
void idWeapon::Event_GetMeleePoint( void ) {
	sdProgram::ReturnVector( meleeTrace.c.point );
}

/*
================
idWeapon::Event_GetMeleeNormal
================
*/
void idWeapon::Event_GetMeleeNormal( void ) {
	if ( meleeTrace.fraction < 1.0f ) {
		sdProgram::ReturnVector( meleeTrace.c.normal );
	} else {
		sdProgram::ReturnVector( vec3_origin );
	}
}

/*
================
idWeapon::Event_GetMeleeEntity
================
*/
void idWeapon::Event_GetMeleeEntity( void ) {
	if ( meleeTrace.fraction < 1.0f ) {
		sdProgram::ReturnEntity( gameLocal.entities[ meleeTrace.c.entityNum ] );
	} else {
		sdProgram::ReturnEntity( NULL );
	}
}

/*
================
idWeapon::Event_GetMeleeJoint
================
*/
void idWeapon::Event_GetMeleeJoint( void ) {
	if ( meleeTrace.fraction < 1.0f && meleeTrace.c.id < 0 ) {
		idEntity* ent = gameLocal.entities[ meleeTrace.c.entityNum ];
		if ( ent->GetAnimator() != NULL ) {
			sdProgram::ReturnString( ent->GetAnimator()->GetJointName( CLIPMODEL_ID_TO_JOINT_HANDLE( meleeTrace.c.id ) ) );
			return;
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
idWeapon::Event_GetMeleeBody
================
*/
void idWeapon::Event_GetMeleeBody( void ) {
	if ( meleeTrace.fraction < 1.0f && meleeTrace.c.id < 0 ) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>( gameLocal.entities[ meleeTrace.c.entityNum ] );
		if ( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() ) {
			int bodyId = af->BodyForClipModelId( meleeTrace.c.id );
			idAFBody *body = af->GetAFPhysics()->GetBody( bodyId );
			if ( body ) {
				sdProgram::ReturnString( body->GetName() );
				return;
			}
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
idWeapon::Event_GetMeleeSurfaceFlags
================
*/
void idWeapon::Event_GetMeleeSurfaceFlags( void ) {
	if ( meleeTrace.fraction < 1.0f && meleeTrace.c.material != NULL ) {
		sdProgram::ReturnInteger( meleeTrace.c.material->GetSurfaceFlags() );
		return;
	}
	sdProgram::ReturnInteger( 0 );
}

/*
================
idWeapon::Event_GetMeleeSurfaceType
================
*/
void idWeapon::Event_GetMeleeSurfaceType( void ) {
	if ( meleeTrace.fraction < 1.0f && meleeTrace.c.surfaceType ) {
		sdProgram::ReturnString( meleeTrace.c.surfaceType->GetName() );
	} else {
		sdProgram::ReturnString( "" );
	}
}

/*
================
idWeapon::Event_GetMeleeSurfaceColor
================
*/
void idWeapon::Event_GetMeleeSurfaceColor( void ) {
	sdProgram::ReturnVector( meleeTrace.c.surfaceColor );
}


/*
=====================
idWeapon::Event_Melee
=====================
*/
void idWeapon::Event_Melee( int contentMask, float distance, bool ignoreOwner, bool useAntiLag ) {
	meleeTrace.fraction = 1.f;

	idVec3 start = playerViewOrigin;
	idVec3 end = start + playerViewAxis[ 0 ] * distance;
	idEntity* ignoree = ignoreOwner ? owner : NULL;
	if ( useAntiLag ) {
		sdAntiLagManager::GetInstance().Trace( meleeTrace, start, end, contentMask, owner, ignoree );
	} else {
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_SCRIPT meleeTrace, start, end, contentMask, ignoree );
	}

	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, start, end, 2000 );

		idEntity* ent = NULL;
		if ( meleeTrace.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( meleeTrace );
		}

		if ( ent ) {
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetAxis(), 1000 );
		}
	}

	sdProgram::ReturnBoolean( meleeTrace.fraction < 1.f );
}

/*
=====================
idWeapon::Event_MeleeAttack
=====================
*/
void idWeapon::Event_MeleeAttack( float damageScale ) {
	bool hit = false;

	if ( stats.shotsFired != NULL ) {
		stats.shotsFired->IncreaseValue( owner->entityNumber, 1 );
	}

	const sdDeclDamage* damage = meleeDamage;
	if ( damageScale < 0.0f ) {
		damage = meleeSpecialDamage;
		damageScale = -damageScale;
		if ( !damage ) {
			gameLocal.Error( "idWeapon::Event_Melee No Special Melee Damage on '%s'", weaponItem->GetName() );
		}
	} else {
		if ( !damage ) {
			gameLocal.Error( "idWeapon::Event_Melee No Melee Damage on '%s'", weaponItem->GetName() );
		}
	}

	if ( damage->GetRecordHitStats() ) {
		if ( gameLocal.totalShotsFiredStat != NULL ) {
			gameLocal.totalShotsFiredStat->IncreaseValue( owner->entityNumber, 1 );
		}
	}

	if ( !gameLocal.isClient ) {
		idEntity* ownerent = owner;
		idEntity* ent;

		if ( meleeTrace.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( meleeTrace );
		} else {
			ent = NULL;
		}

		if ( ent ) {
			float push = damage->GetPush();
			idVec3 impulse = -push * meleeTrace.c.normal;

			ent->ApplyImpulse( this, meleeTrace.c.id, meleeTrace.c.point, impulse );

			if ( ent->fl.takedamage ) {
				idVec3 damageDir = playerViewAxis.ToAngles().ToForward();
				ent->Damage( this, owner, damageDir, damage, damageScale, &meleeTrace );
				hit = true;

				if ( damage->GetRecordHitStats() ) {
					if ( gameLocal.totalShotsHitStat != NULL ) {
						gameLocal.totalShotsHitStat->IncreaseValue( owner->entityNumber, 1 );
					}
				}
			}
		}
	} else {
		sdProgram::ReturnBoolean( false );
	}

	owner->WeaponFireFeedback( feedback );
	sdProgram::ReturnBoolean( hit );
}

/*
===============
idWeapon::OnUsed
===============
*/
bool idWeapon::OnUsed( idPlayer* player, float distance ) {
	if ( onUsedFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	scriptObject->CallNonBlockingScriptEvent( onUsedFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
===============
idWeapon::OnActivate
===============
*/
bool idWeapon::OnActivate( idPlayer* player, float distance ) {
	if ( onActivateFunc == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	scriptObject->CallNonBlockingScriptEvent( onActivateFunc, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
===============
idWeapon::OnActivateHeld
===============
*/
bool idWeapon::OnActivateHeld( idPlayer* player, float distance ) {
	if ( onActivateFuncHeld == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	h1.Push( player->GetScriptObject() );
	h1.Push( distance );
	scriptObject->CallNonBlockingScriptEvent( onActivateFuncHeld, h1 );

	return gameLocal.program->GetReturnedFloat() != 0.f;
}

/*
===============
idWeapon::GetGrenadeFuseTime
===============
*/
int idWeapon::GetGrenadeFuseTime( void ) {
	if ( !getGrenadeFuseStartFunc || scriptObject == NULL ) {
		return -1;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( getGrenadeFuseStartFunc, h1 );
	return idMath::Ftoi( gameLocal.program->GetReturnedFloat() );
}

/*
===============
idWeapon::IsIronSightsEnabled
===============
*/
bool idWeapon::IsIronSightsEnabled( void ) {
	if ( !ironSightsEnabledFunc || scriptObject == NULL ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( ironSightsEnabledFunc, h1 );
	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
idWeapon::OnWeapNext
===============
*/
bool idWeapon::OnWeapNext() {
	if ( !onWeapNextFunc ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( onWeapNextFunc, h1 );

	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
idWeapon::OnWeapPrev
===============
*/
bool idWeapon::OnWeapPrev() {
	if ( !onWeapPrevFunc ) {
		return false;
	}

	sdScriptHelper h1;
	scriptObject->CallNonBlockingScriptEvent( onWeapPrevFunc, h1 );

	return gameLocal.program->GetReturnedBoolean();
}

/*
===============
idWeapon::GetFov
===============
*/
bool idWeapon::GetFov( float& fov ) const {
	if ( zoomFov.GetStartTime() != 0.f ) {
		fov = zoomFov.GetCurrentValue( MS2SEC( gameLocal.time ) );
		return true;
	}
	return false;
}

/*
================
idWeapon::ClampAngles
================
*/
void idWeapon::ClampAngles( idAngles& angles, const idAngles& oldAngles ) const {
	if ( !clampEnabled ) {
		return;
	}

	idAngles oldAnglesAdjusted = oldAngles - clampBaseAngles;

	angles -= clampBaseAngles;

	sdVehiclePosition::ClampAngle( angles, oldAnglesAdjusted, clampPitch, 0 );
	sdVehiclePosition::ClampAngle( angles, oldAnglesAdjusted, clampYaw, 1 );

	angles += clampBaseAngles;
}

/*
============
idWeapon::Event_GetFov
============
*/
void idWeapon::Event_GetFov( void ) {
	float fov = 0.f;
	GetFov( fov );
	sdProgram::ReturnFloat( fov );
}

/*
============
idWeapon::Event_GetSpreadFraction
============
*/
void idWeapon::Event_GetSpreadValue( void ) {
	// this is only used when zoomed, so it doesn't matter that it won't be reflected on the crosshair icon
	sdProgram::ReturnFloat( GetSpreadValue() * spreadModifier );
}

/*
============
idWeapon::Event_IncreaseSpreadValue
============
*/
void idWeapon::Event_IncreaseSpreadValue( void ) {
	float oldSpread = spreadCurrentValue;
	spreadCurrentValue += spreadValues[ ownerStanceState ].inc;
	if ( spreadCurrentValue >= spreadValues[ ownerStanceState ].max && oldSpread <= spreadValues[ ownerStanceState ].max ) {
		spreadCurrentValue = spreadValues[ ownerStanceState ].max;
	}
}

/*
============
idWeapon::Event_SetSpreadModifier
============
*/
void idWeapon::Event_SetSpreadModifier( float modifier ) {
	spreadModifier = modifier;
}

class idCVarCallback_WeaponChanges : public idCVarCallback {
public:
	virtual void OnChanged( void ) {
		idWeapon::UpdateWeaponVisibility();
	}
} g_updateWeaponChanges;

class idCVarCallback_ModelChanges : public idCVarCallback {
public:
	virtual void OnChanged( void ) {
		gameLocal.UpdatePlayerShadows();
	}
} g_updateModelChanges;

/*
============
idWeapon::RegisterCVarCallback
============
*/
void idWeapon::RegisterCVarCallback( void ) {
	ui_showGun.RegisterCallback( &g_updateWeaponChanges );
	pm_thirdPerson.RegisterCallback( &g_updateWeaponChanges );

	pm_thirdPerson.RegisterCallback( &g_updateModelChanges );
	g_showPlayerShadow.RegisterCallback( &g_updateModelChanges );
}

/*
============
idWeapon::UnRegisterCVarCallback
============
*/
void idWeapon::UnRegisterCVarCallback( void ) {
	ui_showGun.UnRegisterCallback( &g_updateWeaponChanges );
	pm_thirdPerson.UnRegisterCallback( &g_updateWeaponChanges );

	pm_thirdPerson.UnRegisterCallback( &g_updateModelChanges );
	g_showPlayerShadow.UnRegisterCallback( &g_updateModelChanges );
}

/*
============
idWeapon::UpdateWeaponVisibility
============
*/
void idWeapon::UpdateWeaponVisibility( void ) {
	sdInstanceCollector< idWeapon > weapons( false );

	for ( int i = 0; i < weapons.Num(); i++ ) {
		idWeapon* weapon = weapons[ i ];
		weapon->UpdateVisibility();
	}
}

/*
============
idWeapon::Event_Fired
============
*/
void idWeapon::Event_Fired( void ) {
	owner->WeaponFireFeedback( feedback );
}

/*
============
idWeapon::Event_GetWorldModel
============
*/
void idWeapon::Event_GetWorldModel( int index ) {
	if ( index < 0 || index >= worldModels.Num() ) {
		sdProgram::ReturnObject( NULL );
		return;
	}

	if ( !worldModels[ index ].IsValid() ) {
		gameLocal.Warning( "idWeapon::Event_GetWorldModel Invalid model index" );
		sdProgram::ReturnObject( NULL );
		return;
	}

	sdProgram::ReturnObject( worldModels[ index ]->GetScriptObject() );
}

/*
============
idWeapon::Event_GetLastTracer
============
*/
void idWeapon::Event_GetLastTracer( void ) {
	if ( lastTracer.IsValid() ) {
		sdProgram::ReturnHandle( lastTracer.GetSpawnId() );
	} else {
		sdProgram::ReturnHandle( 0 );
	}
}

/*
============
idWeapon::Event_SetupAnimClass
============
*/
void idWeapon::Event_SetupAnimClass( const char* prefix ) {
	SetupAnimClass( prefix );
}

/*
============
idWeapon::Event_HasWeaponAnim
============
*/
void idWeapon::Event_HasWeaponAnim( const char* anim ) {
	bool hasAnim = animator.HasAnim( anim );
	sdProgram::ReturnBoolean( hasAnim );
}

/*
============
idWeapon::Event_SetStatName
============
*/
void idWeapon::Event_SetStatName( const char* statName ) {
	LogTimeUsed();
	timeStartedUsing = gameLocal.time;
	SetupStats( statName );
}

/*
============
idWeapon::SetupStats
============
*/
void idWeapon::SetupStats( const char* statName ) {
	if ( *statName ) {
		sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

		stats.shotsFired		= tracker.GetStat( tracker.AllocStat( va( "%s_shots_fired", statName ), sdNetStatKeyValue::SVT_INT ) );
		stats.timeUsed			= tracker.GetStat( tracker.AllocStat( va( "%s_time_used", statName ), sdNetStatKeyValue::SVT_INT ) );
	} else {
		stats.shotsFired		= NULL;
		stats.timeUsed			= NULL;
	}
}

/*
============
idWeapon::UpdatePlayerWeaponInfo

Updates the players current weapon
============
*/
void idWeapon::UpdatePlayerWeaponInfo() {
	if ( !weaponItem ) { //mal: this should NEVER happen!
		gameLocal.Warning( "No valid weapon item passed to UpdatePlayerWeaponInfo!" );
		return;
	}

	botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].weapInfo.weapon = GetPlayerWeaponNum();
}

/*
============
idWeapon::GetGameTeam
============
*/
sdTeamInfo* idWeapon::GetGameTeam( void ) const {
	return owner != NULL ? owner->GetGameTeam() : NULL;
}
