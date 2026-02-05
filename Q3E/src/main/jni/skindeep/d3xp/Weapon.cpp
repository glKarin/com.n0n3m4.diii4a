/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "framework/DeclEntityDef.h"
#include "framework/DeclSkin.h"

#include "idlib/LangDict.h"

#include "gamesys/SysCvar.h"
#include "ai/AI.h"
#include "Player.h"
#include "Trigger.h"
#include "SmokeParticles.h"
#include "WorldSpawn.h"
#include "Misc.h"

#include "bc_meta.h"

#include "Weapon.h"

/***********************************************************************

  idWeapon

***********************************************************************/

//
// event defs
//
const idEventDef EV_Weapon_Clear( "<clear>" );
const idEventDef EV_Weapon_GetOwner( "getOwner", NULL, 'e' );
const idEventDef EV_Weapon_Next( "nextWeapon" );
const idEventDef EV_Weapon_State( "weaponState", "sd" );
const idEventDef EV_Weapon_UseAmmo( "useAmmo", "d" );
const idEventDef EV_Weapon_AddToClip( "addToClip", "d" );
const idEventDef EV_Weapon_AmmoInClip( "ammoInClip", NULL, 'f' );
const idEventDef EV_Weapon_AmmoAvailable( "ammoAvailable", NULL, 'f' );
const idEventDef EV_Weapon_TotalAmmoCount( "totalAmmoCount", NULL, 'f' );
const idEventDef EV_Weapon_ClipSize( "clipSize", NULL, 'f' );
const idEventDef EV_Weapon_WeaponOutOfAmmo( "weaponOutOfAmmo" );
const idEventDef EV_Weapon_WeaponReady( "weaponReady" );
const idEventDef EV_Weapon_WeaponReloading( "weaponReloading" );
const idEventDef EV_Weapon_WeaponHolstered( "weaponHolstered" );
const idEventDef EV_Weapon_WeaponRising( "weaponRising" );
const idEventDef EV_Weapon_WeaponLowering( "weaponLowering" );
const idEventDef EV_Weapon_Flashlight( "flashlight", "d" );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "dfffff" );
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", NULL, 'e' );
const idEventDef EV_Weapon_EjectBrass( "ejectBrass" );
const idEventDef EV_Weapon_Melee( "melee", NULL, 'd' );
const idEventDef EV_Weapon_GetWorldModel( "getWorldModel", NULL, 'e' );
const idEventDef EV_Weapon_AllowDrop( "allowDrop", "d" );
const idEventDef EV_Weapon_AutoReload( "autoReload", NULL, 'f' );
const idEventDef EV_Weapon_NetReload( "netReload" );
const idEventDef EV_Weapon_IsInvisible( "isInvisible", NULL, 'f' );
const idEventDef EV_Weapon_NetEndReload( "netEndReload" );
#ifdef _D3XP
const idEventDef EV_Weapon_GrabberHasTarget( "grabberHasTarget", NULL, 'd' );
const idEventDef EV_Weapon_Grabber( "grabber", "d" );
const idEventDef EV_Weapon_Grabber_SetGrabDistance( "grabberGrabDistance", "f" );
const idEventDef EV_Weapon_LaunchProjectilesEllipse( "launchProjectilesEllipse", "dffff" );
const idEventDef EV_Weapon_LaunchPowerup( "launchPowerup", "sfd" );
const idEventDef EV_Weapon_StartWeaponSmoke( "startWeaponSmoke" );
const idEventDef EV_Weapon_StopWeaponSmoke( "stopWeaponSmoke" );
const idEventDef EV_Weapon_StartWeaponParticle( "startWeaponParticle", "s" );
const idEventDef EV_Weapon_StopWeaponParticle( "stopWeaponParticle", "s" );
const idEventDef EV_Weapon_StartWeaponLight( "startWeaponLight", "s" );
const idEventDef EV_Weapon_StopWeaponLight( "stopWeaponLight", "s" );
#endif


//BC
const idEventDef EV_Weapon_RoundIsChambered("roundischambered", NULL, 'd');
const idEventDef EV_Weapon_SetChambered("setchambered", "d");
const idEventDef EV_Weapon_EjectLiveRound("ejectliveround");
const idEventDef EV_Weapon_GetJiggleState("getJiggleState", NULL, 'f');
const idEventDef EV_Start_Vent("startvent");
const idEventDef EV_Stop_Vent("stopvent");
const idEventDef EV_Weapon_SetNozzleRadius("SetNozzleRadius", "f"); //Set radius of nozzle light effect
const idEventDef EV_Weapon_SetClipAmount("SetClipAmount", "d"); //Force mag to be a set amount.
const idEventDef EV_Weapon_UnloadMag("UnloadMag", NULL, 'd');


const int NOZZLE_LERPTIME = 300;

#define DEFAULT_ZOOMFOV 50


//
// class def
//
CLASS_DECLARATION(idAnimatedEntity, idWeapon)
EVENT(EV_Weapon_Clear, idWeapon::Event_Clear)
EVENT(EV_Weapon_GetOwner, idWeapon::Event_GetOwner)
EVENT(EV_Weapon_State, idWeapon::Event_WeaponState)
EVENT(EV_Weapon_WeaponReady, idWeapon::Event_WeaponReady)
EVENT(EV_Weapon_WeaponOutOfAmmo, idWeapon::Event_WeaponOutOfAmmo)
EVENT(EV_Weapon_WeaponReloading, idWeapon::Event_WeaponReloading)
EVENT(EV_Weapon_WeaponHolstered, idWeapon::Event_WeaponHolstered)
EVENT(EV_Weapon_WeaponRising, idWeapon::Event_WeaponRising)
EVENT(EV_Weapon_WeaponLowering, idWeapon::Event_WeaponLowering)
EVENT(EV_Weapon_UseAmmo, idWeapon::Event_UseAmmo)
EVENT(EV_Weapon_AddToClip, idWeapon::Event_AddToClip)
EVENT(EV_Weapon_AmmoInClip, idWeapon::Event_AmmoInClip)
EVENT(EV_Weapon_AmmoAvailable, idWeapon::Event_AmmoAvailable)
EVENT(EV_Weapon_TotalAmmoCount, idWeapon::Event_TotalAmmoCount)
EVENT(EV_Weapon_ClipSize, idWeapon::Event_ClipSize)
EVENT(AI_PlayAnim, idWeapon::Event_PlayAnim)
EVENT(AI_PlayCycle, idWeapon::Event_PlayCycle)
EVENT(AI_SetBlendFrames, idWeapon::Event_SetBlendFrames)
EVENT(AI_GetBlendFrames, idWeapon::Event_GetBlendFrames)
EVENT(AI_AnimDone, idWeapon::Event_AnimDone)
EVENT(EV_Weapon_Next, idWeapon::Event_Next)
EVENT(EV_SetSkin, idWeapon::Event_SetSkin)
EVENT(EV_Weapon_Flashlight, idWeapon::Event_Flashlight)
EVENT(EV_Light_GetLightParm, idWeapon::Event_GetLightParm)
EVENT(EV_Light_SetLightParm, idWeapon::Event_SetLightParm)
EVENT(EV_Light_SetLightParms, idWeapon::Event_SetLightParms)
EVENT(EV_Weapon_LaunchProjectiles, idWeapon::Event_LaunchProjectiles)
EVENT(EV_Weapon_CreateProjectile, idWeapon::Event_CreateProjectile)
EVENT(EV_Weapon_EjectBrass, idWeapon::Event_EjectBrass)
EVENT(EV_Weapon_Melee, idWeapon::Event_Melee)
EVENT(EV_Weapon_GetWorldModel, idWeapon::Event_GetWorldModel)
EVENT(EV_Weapon_AllowDrop, idWeapon::Event_AllowDrop)
EVENT(EV_Weapon_AutoReload, idWeapon::Event_AutoReload)
EVENT(EV_Weapon_NetReload, idWeapon::Event_NetReload)
EVENT(EV_Weapon_IsInvisible, idWeapon::Event_IsInvisible)
EVENT(EV_Weapon_NetEndReload, idWeapon::Event_NetEndReload)
#ifdef _D3XP
EVENT(EV_Weapon_Grabber, idWeapon::Event_Grabber)
EVENT(EV_Weapon_GrabberHasTarget, idWeapon::Event_GrabberHasTarget)
EVENT(EV_Weapon_Grabber_SetGrabDistance, idWeapon::Event_GrabberSetGrabDistance)
EVENT(EV_Weapon_LaunchProjectilesEllipse, idWeapon::Event_LaunchProjectilesEllipse)
EVENT(EV_Weapon_LaunchPowerup, idWeapon::Event_LaunchPowerup)
EVENT(EV_Weapon_StartWeaponSmoke, idWeapon::Event_StartWeaponSmoke)
EVENT(EV_Weapon_StopWeaponSmoke, idWeapon::Event_StopWeaponSmoke)
EVENT(EV_Weapon_StartWeaponParticle, idWeapon::Event_StartWeaponParticle)
EVENT(EV_Weapon_StopWeaponParticle, idWeapon::Event_StopWeaponParticle)
EVENT(EV_Weapon_StartWeaponLight, idWeapon::Event_StartWeaponLight)
EVENT(EV_Weapon_StopWeaponLight, idWeapon::Event_StopWeaponLight)
#endif

//BC
EVENT(EV_Weapon_RoundIsChambered, idWeapon::Event_RoundIsChambered)
EVENT(EV_Weapon_SetChambered, idWeapon::Event_SetChambered)
EVENT(EV_Weapon_EjectLiveRound, idWeapon::Event_EjectLiveRound)
EVENT(EV_Weapon_GetJiggleState, idWeapon::Event_GetJiggleState)
EVENT(EV_Start_Vent, idWeapon::Event_StartVent)
EVENT(EV_Stop_Vent, idWeapon::Event_StopVent)
EVENT(EV_Weapon_SetNozzleRadius, idWeapon::Event_SetNozzleRadius)
EVENT(EV_Weapon_SetClipAmount, idWeapon::Event_SetClipAmount)
EVENT(EV_Weapon_UnloadMag, idWeapon::Event_UnloadMag)


END_CLASS

/***********************************************************************

	init

***********************************************************************/

/*
================
idWeapon::idWeapon()
================
*/
idWeapon::idWeapon() {
	owner					= NULL;
	worldModel				= NULL;
	weaponDef				= NULL;
	thread					= NULL;

	memset( &guiLight, 0, sizeof( guiLight ) );
	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	memset( &worldMuzzleFlash, 0, sizeof( worldMuzzleFlash ) );
	memset( &nozzleGlow, 0, sizeof( nozzleGlow ) );

	muzzleFlashEnd			= 0;
	flashColor				= vec3_origin;
	muzzleFlashHandle		= -1;
	worldMuzzleFlashHandle	= -1;
	guiLightHandle			= -1;
	nozzleGlowHandle		= -1;
	modelDefHandle			= -1;
#ifdef _D3XP
	grabberState			= -1;
#endif

	berserk					= 2;
	brassDelay				= 0;

	allowDrop				= true;

	Clear();

	fl.networkSync = true;


	autoaimEnabled = false;
	isGrenade = false;
	isJiggling = false;
	multiCarry = false;

	nozzleLerping = false;
	nozzleLerpTimer = 0;
	nozzleLerpValueStart = 0;
	nozzleLerpValueEnd = 0;
	nozzleGlowAffectLightMeter = false;
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() {
	Clear();
	delete worldModel.GetEntity();
}


/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn( void )
{
	if ( !gameLocal.isClient ) {
		// setup the world model
		worldModel = static_cast< idAnimatedEntity * >( gameLocal.SpawnEntityType( idAnimatedEntity::Type, NULL ) );
		worldModel.GetEntity()->fl.networkSync = true;
	}

#ifdef _D3XP
	if ( 1 /*!gameLocal.isMultiplayer*/ ) {
		grabber.Initialize();
	}
#endif

	thread = new idThread();
	thread->ManualDelete();
	thread->ManualControl();
}

/*
================
idWeapon::SetOwner

Only called at player spawn time, not each weapon switch
================
*/
void idWeapon::SetOwner( idPlayer *_owner ) {
	assert( !owner );
	owner = _owner;
	SetName( va( "%s_weapon", owner->name.c_str() ) );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetName( va( "%s_weapon_worldmodel", owner->name.c_str() ) );
	}
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

/*
================
idWeapon::CacheWeapon
================
*/
void idWeapon::CacheWeapon( const char *weaponName ) {
	const idDeclEntityDef *weaponDef;
	const char *brassDefName;
	const char *clipModelName;
	idTraceModel trm;
	const char *guiName;
	
	
	const char *ejectAmmoDefName;
	const char *ejectClipModelName;

	const char *projectileCache;
	
	const char *particle01Cache;

	const char *fx01Cache;
	const char *fx02Cache;

	const char *shrapnelName;

	const char *matCache01;
	const char *matCache02;
	const char *matCache03;


	weaponDef = gameLocal.FindEntityDef( weaponName, false );
	if ( !weaponDef ) {
		return;
	}

	// precache the brass collision model
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );
	if ( brassDefName[0] )
	{
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( brassDef )
		{
			brassDef->dict.GetString( "clipmodel", "", &clipModelName );
			if ( !clipModelName[0] ) {
				clipModelName = brassDef->dict.GetString( "model" );		// use the visual model
			}
			// load the trace model
			collisionModelManager->TrmFromModel( clipModelName, trm );
		}
	}


	//BC precache the ammo model
	ejectAmmoDefName = weaponDef->dict.GetString("def_ejectammo");
	if (ejectAmmoDefName[0])
	{
		const idDeclEntityDef *ejectDef = gameLocal.FindEntityDef(ejectAmmoDefName, false);
		if (ejectDef)
		{
			ejectDef->dict.GetString("clipmodel", "", &ejectClipModelName);

			if (!ejectClipModelName[0])
			{
				ejectClipModelName = ejectDef->dict.GetString("model");		// use the visual model
			}

			// load the trace model
			collisionModelManager->TrmFromModel(ejectClipModelName, trm);
		}
	}


	projectileCache = weaponDef->dict.GetString("cache_projectile");
	if (projectileCache[0])
	{
		collisionModelManager->TrmFromModel(projectileCache, trm);
	}

	particle01Cache = weaponDef->dict.GetString("cache_particle01");
	if (particle01Cache[0])
	{
		declManager->FindType(DECL_PARTICLE, particle01Cache);
	}

	fx01Cache = weaponDef->dict.GetString("cache_fx01");
	if (fx01Cache[0])
	{
		declManager->FindType(DECL_FX, fx01Cache);
	}

	fx02Cache = weaponDef->dict.GetString("cache_fx02");
	if (fx02Cache[0])
	{
		declManager->FindType(DECL_FX, fx02Cache);
	}



	matCache01 = weaponDef->dict.GetString("cache_mat01");
	if (matCache01[0])
	{
		declManager->FindType(DECL_MATERIAL, matCache01);
	}

	matCache02 = weaponDef->dict.GetString("cache_mat02");
	if (matCache02[0])
	{
		declManager->FindType(DECL_MATERIAL, matCache02);
	}

	matCache03 = weaponDef->dict.GetString("cache_mat03");
	if (matCache03[0])
	{
		declManager->FindType(DECL_MATERIAL, matCache03);
	}



	shrapnelName = weaponDef->dict.GetString("cache_shrapnel");
	if (shrapnelName[0])
	{
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef(shrapnelName, false);
		if (brassDef)
		{
			const char *shrapnelModelName;
			brassDef->dict.GetString("clipmodel", "", &shrapnelModelName);
			if (!shrapnelModelName[0]) {
				shrapnelModelName = brassDef->dict.GetString("model");		// use the visual model
			}
			// load the trace model
			collisionModelManager->TrmFromModel(shrapnelModelName, trm);
		}
	}



	



	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		uiManager->FindGui( guiName, true, false, true );
	}
}

/*
================
idWeapon::Save
================
*/
void idWeapon::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( autoaimEnabled ); // bool autoaimEnabled
	savefile->WriteBool( isGrenade ); // bool isGrenade

	savefile->WriteBool( isJiggling ); // bool isJiggling
	savefile->WriteBool( multiCarry ); // bool multiCarry

	//idScriptBool WEAPON_ATTACK; // idScriptBool WEAPON_ATTACK
	//idScriptBool WEAPON_RELOAD; // idScriptBool WEAPON_RELOAD
	//idScriptBool WEAPON_NETRELOAD; // idScriptBool WEAPON_NETRELOAD
	//idScriptBool WEAPON_NETENDRELOAD; // idScriptBool WEAPON_NETENDRELOAD
	//idScriptBool WEAPON_NETFIRING; // idScriptBool WEAPON_NETFIRING
	//idScriptBool WEAPON_RAISEWEAPON; // idScriptBool WEAPON_RAISEWEAPON
	//idScriptBool WEAPON_LOWERWEAPON; // idScriptBool WEAPON_LOWERWEAPON

	//idScriptBool WEAPON_RACKTHESLIDE; // idScriptBool WEAPON_RACKTHESLIDE
	//idScriptBool WEAPON_INSPECTMAGAZINE; // idScriptBool WEAPON_INSPECTMAGAZINE
	//idScriptBool WEAPON_INSPECTCHAMBER; // idScriptBool WEAPON_INSPECTCHAMBER
	//idScriptBool WEAPON_INSPECTWEAPON; // idScriptBool WEAPON_INSPECTWEAPON

	//idScriptBool WEAPON_AIMMODE; // idScriptBool WEAPON_AIMMODE

	savefile->WriteInt( status ); // weaponStatus_t status
	savefile->WriteObject( thread ); // idThread * thread
	savefile->WriteString( state ); // idString state
	savefile->WriteString( idealState ); // idString idealState
	savefile->WriteInt( animBlendFrames ); // int animBlendFrames
	savefile->WriteInt( animDoneTime ); // int animDoneTime
	savefile->WriteBool( isLinked ); // bool isLinked
	savefile->WriteObject( projectileEnt ); // idEntity * projectileEnt

	savefile->WriteObject( owner ); // idPlayer * owner
	worldModel.Save( savefile ); // idEntityPtr<idAnimatedEntity> worldModel
	savefile->WriteInt( hideTime ); // int hideTime
	savefile->WriteFloat( hideDistance ); // float hideDistance
	savefile->WriteInt( hideStartTime ); // int hideStartTime
	savefile->WriteFloat( hideStart ); // float hideStart
	savefile->WriteFloat( hideEnd ); // float hideEnd
	savefile->WriteFloat( hideOffset ); // float hideOffset
	savefile->WriteBool( hide ); // bool hide
	savefile->WriteBool( disabled ); // bool disabled
	savefile->WriteInt( berserk ); // int berserk
	savefile->WriteVec3( playerViewOrigin ); // idVec3 playerViewOrigin
	savefile->WriteMat3( playerViewAxis ); // idMat3 playerViewAxis
	savefile->WriteVec3( viewWeaponOrigin ); // idVec3 viewWeaponOrigin
	savefile->WriteMat3( viewWeaponAxis ); // idMat3 viewWeaponAxis
	savefile->WriteVec3( muzzleOrigin ); // idVec3 muzzleOrigin
	savefile->WriteMat3( muzzleAxis ); // idMat3 muzzleAxis

	savefile->WriteVec3( pushVelocity ); // idVec3 pushVelocity

	
	savefile->WriteEntityDef( weaponDef ); // const idDeclEntityDef * weaponDef
	savefile->WriteEntityDef( meleeDef ); // const idDeclEntityDef * meleeDef
	savefile->WriteDict( &projectileDict ); // idDict projectileDict
	savefile->WriteFloat( meleeDistance ); // float meleeDistance
	savefile->WriteString( meleeDefName ); // idString meleeDefName
	savefile->WriteDict( &brassDict ); // idDict brassDict
	savefile->WriteInt( brassDelay ); // int brassDelay
	savefile->WriteString( icon ); // idString icon
	savefile->WriteRenderLight( guiLight ); // renderLight_t guiLight
	savefile->WriteInt( guiLightHandle ); // int guiLightHandle
	savefile->WriteRenderLight( muzzleFlash ); // renderLight_t muzzleFlash
	savefile->WriteInt( muzzleFlashHandle ); // int muzzleFlashHandle

	savefile->WriteRenderLight( worldMuzzleFlash ); // renderLight_t worldMuzzleFlash
	savefile->WriteInt( worldMuzzleFlashHandle ); // int worldMuzzleFlashHandle

	savefile->WriteVec3( flashColor ); // idVec3 flashColor
	savefile->WriteInt( muzzleFlashEnd ); // int muzzleFlashEnd
	savefile->WriteInt( flashTime ); // int flashTime
	savefile->WriteBool( lightOn ); // bool lightOn
	savefile->WriteBool( silent_fire ); // bool silent_fire
	savefile->WriteBool( allowDrop ); // bool allowDrop
	savefile->WriteBool( hasBloodSplat ); // bool hasBloodSplat
	savefile->WriteInt( kick_endtime ); // int kick_endtime
	savefile->WriteInt( muzzle_kick_time ); // int muzzle_kick_time
	savefile->WriteInt( muzzle_kick_maxtime ); // int muzzle_kick_maxtime
	savefile->WriteAngles( muzzle_kick_angles ); // idAngles muzzle_kick_angles
	savefile->WriteVec3( muzzle_kick_offset ); // idVec3 muzzle_kick_offset
	savefile->WriteInt( ammoType ); // ammo_t ammoType
	savefile->WriteInt( ammoRequired ); // int ammoRequired
	savefile->WriteInt( clipSize ); // int clipSize
	savefile->WriteInt( ammoClip ); // int ammoClip
	savefile->WriteInt( lowAmmo ); // int lowAmmo
	savefile->WriteBool( powerAmmo ); // bool powerAmmo

	savefile->WriteBool( isFiring ); // bool isFiring // blendo eric: TODO this might be problematic?

	savefile->WriteInt( zoomFov ); // int zoomFov
	savefile->WriteJoint( barrelJointView ); // jointHandle_t barrelJointView
	savefile->WriteJoint( flashJointView ); // jointHandle_t flashJointView
	savefile->WriteJoint( ejectJointView ); // jointHandle_t ejectJointView
	savefile->WriteJoint( guiLightJointView ); // jointHandle_t guiLightJointView
	savefile->WriteJoint( ventLightJointView ); // jointHandle_t ventLightJointView

	savefile->WriteJoint( flashJointWorld ); // jointHandle_t flashJointWorld
	savefile->WriteJoint( barrelJointWorld ); // jointHandle_t barrelJointWorld
	savefile->WriteJoint( ejectJointWorld ); // jointHandle_t ejectJointWorld

	savefile->WriteJoint( smokeJointView ); // jointHandle_t smokeJointView

	savefile->WriteInt(weaponParticles.Num()); // idHashTable<WeaponParticle_t> weaponParticles
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		savefile->WriteString( part->name );
		savefile->WriteString( part->particlename );
		savefile->WriteBool( part->active );
		savefile->WriteInt( part->startTime );
		savefile->WriteJoint( part->joint );
		savefile->WriteBool( part->smoke );
		if(!part->smoke) {
			savefile->WriteObject(part->emitter);
		}
	}
	savefile->WriteInt(weaponLights.Num()); // idHashTable<WeaponLight_t> weaponLights
	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		savefile->WriteString( light->name );
		savefile->WriteBool( light->active );
		savefile->WriteInt( light->startTime );
		savefile->WriteJoint( light->joint );
		savefile->WriteInt( light->lightHandle );
		savefile->WriteRenderLight( light->light );
	}

	savefile->WriteSoundShader( sndHum ); // const idSoundShader * sndHum
	savefile->WriteParticle( weaponSmoke ); // const idDeclParticle * weaponSmoke
	savefile->WriteInt( weaponSmokeStartTime ); // int weaponSmokeStartTime
	savefile->WriteBool( continuousSmoke ); // bool continuousSmoke
	savefile->WriteParticle( strikeSmoke ); // const idDeclParticle * strikeSmoke
	savefile->WriteInt( strikeSmokeStartTime ); // int strikeSmokeStartTime
	savefile->WriteVec3( strikePos ); // idVec3 strikePos
	savefile->WriteMat3( strikeAxis ); // idMat3 strikeAxis
	savefile->WriteInt( nextStrikeFx ); // int nextStrikeFx
	savefile->WriteBool( nozzleFx ); // bool nozzleFx
	savefile->WriteInt( nozzleFxFade ); // int nozzleFxFade
	savefile->WriteInt( lastAttack ); // int lastAttack
	savefile->WriteRenderLight( nozzleGlow ); // renderLight_t nozzleGlow
	savefile->WriteInt( nozzleGlowHandle ); // int nozzleGlowHandle

	savefile->WriteVec3( nozzleGlowColor ); // idVec3 nozzleGlowColor
	savefile->WriteMaterial( nozzleGlowShader ); // const idMaterial * nozzleGlowShader
	savefile->WriteFloat( nozzleGlowRadius ); // float nozzleGlowRadius
	savefile->WriteInt( weaponAngleOffsetAverages ); // int weaponAngleOffsetAverages
	savefile->WriteFloat( weaponAngleOffsetScale ); // float weaponAngleOffsetScale
	savefile->WriteFloat( weaponAngleOffsetMax ); // float weaponAngleOffsetMax
	savefile->WriteFloat( weaponOffsetTime ); // float weaponOffsetTime
	savefile->WriteFloat( weaponOffsetScale ); // float weaponOffsetScale

	savefile->WriteStaticObject( idWeapon::grabber ); // idGrabber grabber
	savefile->WriteInt( grabberState ); // int grabberState

	savefile->WriteBool( nozzleLerping ); // bool nozzleLerping
	savefile->WriteInt( nozzleLerpTimer ); // int nozzleLerpTimer
	savefile->WriteFloat( nozzleLerpValueStart ); // float nozzleLerpValueStart
	savefile->WriteFloat( nozzleLerpValueEnd ); // float nozzleLerpValueEnd
	savefile->WriteBool( nozzleGlowAffectLightMeter ); // bool nozzleGlowAffectLightMeter

	savefile->WriteDict( &singleAmmoDict ); // idDict singleAmmoDict

	savefile->WriteInt( lastEndattack ); // int lastEndattack
}

/*
================
idWeapon::Restore
================
*/
void idWeapon::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( autoaimEnabled ); // bool autoaimEnabled
	savefile->ReadBool( isGrenade ); // bool isGrenade

	savefile->ReadBool( isJiggling ); // bool isJiggling
	savefile->ReadBool( multiCarry ); // bool multiCarry

	// Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" ); // idScriptBool WEAPON_ATTACK
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" ); // idScriptBool WEAPON_RELOAD
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" ); // idScriptBool WEAPON_NETRELOAD
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" ); // idScriptBool WEAPON_NETENDRELOAD
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" ); // idScriptBool WEAPON_NETFIRING
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" ); // idScriptBool WEAPON_RAISEWEAPON
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" ); // idScriptBool WEAPON_LOWERWEAPON

	WEAPON_RACKTHESLIDE.LinkTo(scriptObject, "WEAPON_RACKTHESLIDE"); // idScriptBool WEAPON_RACKTHESLIDE
	WEAPON_INSPECTMAGAZINE.LinkTo(scriptObject, "WEAPON_INSPECTMAGAZINE"); // idScriptBool WEAPON_INSPECTMAGAZINE
	WEAPON_INSPECTCHAMBER.LinkTo(scriptObject, "WEAPON_INSPECTCHAMBER"); // idScriptBool WEAPON_INSPECTCHAMBER
	WEAPON_INSPECTWEAPON.LinkTo(scriptObject, "WEAPON_INSPECTWEAPON"); // idScriptBool WEAPON_INSPECTWEAPON

	WEAPON_AIMMODE.LinkTo(scriptObject, "WEAPON_AIMMODE"); // idScriptBool WEAPON_AIMMODE

	savefile->ReadInt( (int&)status ); // weaponStatus_t status
	savefile->ReadObject( CastClassPtrRef(thread) ); // idThread * thread
	savefile->ReadString( state ); // idString state
	savefile->ReadString( idealState ); // idString idealState
	savefile->ReadInt( animBlendFrames ); // int animBlendFrames
	savefile->ReadInt( animDoneTime ); // int animDoneTime
	savefile->ReadBool( isLinked ); // bool isLinked
	savefile->ReadObject( projectileEnt ); // idEntity * projectileEnt

	savefile->ReadObject( CastClassPtrRef(owner) ); // idPlayer * owner
	worldModel.Restore( savefile ); // idEntityPtr<idAnimatedEntity> worldModel
	savefile->ReadInt( hideTime ); // int hideTime
	savefile->ReadFloat( hideDistance ); // float hideDistance
	savefile->ReadInt( hideStartTime ); // int hideStartTime
	savefile->ReadFloat( hideStart ); // float hideStart
	savefile->ReadFloat( hideEnd ); // float hideEnd
	savefile->ReadFloat( hideOffset ); // float hideOffset
	savefile->ReadBool( hide ); // bool hide
	savefile->ReadBool( disabled ); // bool disabled
	savefile->ReadInt( berserk ); // int berserk
	savefile->ReadVec3( playerViewOrigin ); // idVec3 playerViewOrigin
	savefile->ReadMat3( playerViewAxis ); // idMat3 playerViewAxis
	savefile->ReadVec3( viewWeaponOrigin ); // idVec3 viewWeaponOrigin
	savefile->ReadMat3( viewWeaponAxis ); // idMat3 viewWeaponAxis
	savefile->ReadVec3( muzzleOrigin ); // idVec3 muzzleOrigin
	savefile->ReadMat3( muzzleAxis ); // idMat3 muzzleAxis

	savefile->ReadVec3( pushVelocity ); // idVec3 pushVelocity


	savefile->ReadEntityDef( weaponDef ); // const idDeclEntityDef * weaponDef
	savefile->ReadEntityDef( meleeDef ); // const idDeclEntityDef * meleeDef
	savefile->ReadDict( &projectileDict ); // idDict projectileDict
	savefile->ReadFloat( meleeDistance ); // float meleeDistance
	savefile->ReadString( meleeDefName ); // idString meleeDefName
	savefile->ReadDict( &brassDict ); // idDict brassDict
	savefile->ReadInt( brassDelay ); // int brassDelay
	savefile->ReadString( icon ); // idString icon
	savefile->ReadRenderLight( guiLight ); // renderLight_t guiLight
	savefile->ReadInt( guiLightHandle ); // int guiLightHandle
	if ( guiLightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( guiLightHandle, &guiLight );
	}
	savefile->ReadRenderLight( muzzleFlash ); // renderLight_t muzzleFlash
	savefile->ReadInt( muzzleFlashHandle ); // int muzzleFlashHandle
	if ( muzzleFlashHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
	}

	savefile->ReadRenderLight( worldMuzzleFlash ); // renderLight_t worldMuzzleFlash
	savefile->ReadInt( worldMuzzleFlashHandle ); // int worldMuzzleFlashHandle
	if ( worldMuzzleFlashHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	}

	savefile->ReadVec3( flashColor ); // idVec3 flashColor
	savefile->ReadInt( muzzleFlashEnd ); // int muzzleFlashEnd
	savefile->ReadInt( flashTime ); // int flashTime
	savefile->ReadBool( lightOn ); // bool lightOn
	savefile->ReadBool( silent_fire ); // bool silent_fire
	savefile->ReadBool( allowDrop ); // bool allowDrop
	savefile->ReadBool( hasBloodSplat ); // bool hasBloodSplat
	savefile->ReadInt( kick_endtime ); // int kick_endtime
	savefile->ReadInt( muzzle_kick_time ); // int muzzle_kick_time
	savefile->ReadInt( muzzle_kick_maxtime ); // int muzzle_kick_maxtime
	savefile->ReadAngles( muzzle_kick_angles ); // idAngles muzzle_kick_angles
	savefile->ReadVec3( muzzle_kick_offset ); // idVec3 muzzle_kick_offset
	savefile->ReadInt( ammoType ); // ammo_t ammoType
	savefile->ReadInt( ammoRequired ); // int ammoRequired
	savefile->ReadInt( clipSize ); // int clipSize
	savefile->ReadInt( ammoClip ); // int ammoClip
	savefile->ReadInt( lowAmmo ); // int lowAmmo
	savefile->ReadBool( powerAmmo ); // bool powerAmmo
	savefile->ReadBool( isFiring ); // bool isFiring // blendo eric: TODO this might be problematic?
	savefile->ReadInt( zoomFov ); // int zoomFov
	savefile->ReadJoint( barrelJointView ); // jointHandle_t barrelJointView
	savefile->ReadJoint( flashJointView ); // jointHandle_t flashJointView
	savefile->ReadJoint( ejectJointView ); // jointHandle_t ejectJointView
	savefile->ReadJoint( guiLightJointView ); // jointHandle_t guiLightJointView
	savefile->ReadJoint( ventLightJointView ); // jointHandle_t ventLightJointView

	savefile->ReadJoint( flashJointWorld ); // jointHandle_t flashJointWorld
	savefile->ReadJoint( barrelJointWorld ); // jointHandle_t barrelJointWorld
	savefile->ReadJoint( ejectJointWorld ); // jointHandle_t ejectJointWorld

	savefile->ReadJoint( smokeJointView ); // jointHandle_t smokeJointView

	int particleCount;
	savefile->ReadInt( particleCount );  // idHashTable<WeaponParticle_t> weaponParticles
	for(int i = 0; i < particleCount; i++) {
		WeaponParticle_t newParticle;
		memset(&newParticle, 0, sizeof(newParticle));

		idStr name, particlename;
		savefile->ReadString( name );
		savefile->ReadString( particlename );

		strcpy( newParticle.name, name.c_str() );
		strcpy( newParticle.particlename, particlename.c_str() );

		savefile->ReadBool( newParticle.active );
		savefile->ReadInt( newParticle.startTime );
		savefile->ReadJoint( newParticle.joint );
		savefile->ReadBool( newParticle.smoke );
		if(newParticle.smoke) {
			newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particlename, false ) );
		} else {
			savefile->ReadObject(reinterpret_cast<idClass *&>(newParticle.emitter));
		}

		weaponParticles.Set(newParticle.name, newParticle);
	}

	int lightCount;
	savefile->ReadInt( lightCount ); // idHashTable<WeaponLight_t> weaponLights
	for(int i = 0; i < lightCount; i++) {
		WeaponLight_t newLight;
		memset(&newLight, 0, sizeof(newLight));

		idStr name;
		savefile->ReadString( name );
		strcpy( newLight.name, name.c_str() );

		savefile->ReadBool( newLight.active );
		savefile->ReadInt( newLight.startTime );
		savefile->ReadJoint( newLight.joint );
		savefile->ReadInt( newLight.lightHandle );
		savefile->ReadRenderLight( newLight.light );
		if ( newLight.lightHandle != - 1 ) {
			gameRenderWorld->UpdateLightDef( newLight.lightHandle, &newLight.light );
		}
		weaponLights.Set(newLight.name, newLight);
	}

	savefile->ReadSoundShader( sndHum ); // const idSoundShader * sndHum
	savefile->ReadParticle( weaponSmoke ); // const idDeclParticle * weaponSmoke
	savefile->ReadInt( weaponSmokeStartTime ); // int weaponSmokeStartTime
	savefile->ReadBool( continuousSmoke ); // bool continuousSmoke
	savefile->ReadParticle( strikeSmoke ); // const idDeclParticle * strikeSmoke
	savefile->ReadInt( strikeSmokeStartTime ); // int strikeSmokeStartTime
	savefile->ReadVec3( strikePos ); // idVec3 strikePos
	savefile->ReadMat3( strikeAxis ); // idMat3 strikeAxis
	savefile->ReadInt( nextStrikeFx ); // int nextStrikeFx
	savefile->ReadBool( nozzleFx ); // bool nozzleFx
	savefile->ReadInt( nozzleFxFade ); // int nozzleFxFade
	savefile->ReadInt( lastAttack ); // int lastAttack
	savefile->ReadRenderLight( nozzleGlow ); // renderLight_t nozzleGlow
	savefile->ReadInt( nozzleGlowHandle ); // int nozzleGlowHandle
	if ( nozzleGlowHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( nozzleGlowHandle, &nozzleGlow );
	}

	savefile->ReadVec3( nozzleGlowColor ); // idVec3 nozzleGlowColor
	savefile->ReadMaterial( nozzleGlowShader ); // const idMaterial * nozzleGlowShader
	savefile->ReadFloat( nozzleGlowRadius ); // float nozzleGlowRadius
	savefile->ReadInt( weaponAngleOffsetAverages ); // int weaponAngleOffsetAverages
	savefile->ReadFloat( weaponAngleOffsetScale ); // float weaponAngleOffsetScale
	savefile->ReadFloat( weaponAngleOffsetMax ); // float weaponAngleOffsetMax
	savefile->ReadFloat( weaponOffsetTime ); // float weaponOffsetTime
	savefile->ReadFloat( weaponOffsetScale ); // float weaponOffsetScale

	savefile->ReadStaticObject( grabber ); // idGrabber grabber
	savefile->ReadInt( grabberState ); // int grabberState

	savefile->ReadBool( nozzleLerping ); // bool nozzleLerping
	savefile->ReadInt( nozzleLerpTimer ); // int nozzleLerpTimer
	savefile->ReadFloat( nozzleLerpValueStart ); // float nozzleLerpValueStart
	savefile->ReadFloat( nozzleLerpValueEnd ); // float nozzleLerpValueEnd
	savefile->ReadBool( nozzleGlowAffectLightMeter ); // bool nozzleGlowAffectLightMeter

	savefile->ReadDict( &singleAmmoDict ); // idDict singleAmmoDict

	savefile->ReadInt( lastEndattack ); // int lastEndattack
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
	CancelEvents( &EV_Weapon_Clear );

	DeconstructScriptObject();
	scriptObject.Free();

	WEAPON_ATTACK.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_NETRELOAD.Unlink();
	WEAPON_NETENDRELOAD.Unlink();
	WEAPON_NETFIRING.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();

	WEAPON_RACKTHESLIDE.Unlink();
	WEAPON_INSPECTMAGAZINE.Unlink();
	WEAPON_INSPECTCHAMBER.Unlink();
	WEAPON_INSPECTWEAPON.Unlink();

	WEAPON_AIMMODE.Unlink();

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( muzzleFlashHandle );
		muzzleFlashHandle = -1;
	}
	if ( worldMuzzleFlashHandle != -1 ) {
		gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
		worldMuzzleFlashHandle = -1;
	}
	if ( guiLightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( guiLightHandle );
		guiLightHandle = -1;
	}
	if ( nozzleGlowHandle != -1 ) {
		gameRenderWorld->FreeLightDef( nozzleGlowHandle );
		nozzleGlowHandle = -1;
	}

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.entityNum	= entityNumber;

	renderEntity.noShadow		= true;
	renderEntity.noSelfShadow	= true;
	renderEntity.customSkin		= NULL;

	// set default shader parms
	renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	renderEntity.shaderParms[3] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = 0.0f;
	renderEntity.shaderParms[5] = 0.0f;
	renderEntity.shaderParms[6] = 0.0f;
	renderEntity.shaderParms[7] = 0.0f;

	if ( refSound.referenceSound ) {
		refSound.referenceSound->Free( true );
	}
	memset( &refSound, 0, sizeof( refSound_t ) );

	// setting diversity to 0 results in no random sound.  -1 indicates random.
	refSound.diversity = -1.0f;

	if ( owner ) {
		// don't spatialize the weapon sounds
		refSound.listenerId = owner->GetListenerId();
	}

	// clear out the sounds from our spawnargs since we'll copy them from the weapon def
	const idKeyValue *kv = spawnArgs.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "snd_" );
	}

	hideTime		= 300;
	hideDistance	= -15.0f;
	hideStartTime	= gameLocal.time - hideTime;
	hideStart		= 0.0f;
	hideEnd			= 0.0f;
	hideOffset		= 0.0f;
	hide			= false;
	disabled		= false;

	weaponSmoke		= NULL;
	weaponSmokeStartTime = 0;
	continuousSmoke = false;
	strikeSmoke		= NULL;
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	icon			= "";

	playerViewAxis.Identity();
	playerViewOrigin.Zero();
	viewWeaponAxis.Identity();
	viewWeaponOrigin.Zero();
	muzzleAxis.Identity();
	muzzleOrigin.Zero();
	pushVelocity.Zero();

	status			= WP_HOLSTERED;
	state			= "";
	idealState		= "";
	animBlendFrames	= 0;
	animDoneTime	= 0;

	projectileDict.Clear();
	meleeDef		= NULL;
	meleeDefName	= "";
	meleeDistance	= 0.0f;
	brassDict.Clear();

	flashTime		= 250;
	lightOn			= false;
	silent_fire		= false;

#ifdef _D3XP
	grabberState	= -1;
	grabber.Update( owner, true );
#endif

	ammoType		= 0;
	ammoRequired	= 0;
	ammoClip		= 0;
	clipSize		= 0;
	lowAmmo			= 0;
	powerAmmo		= false;

	kick_endtime		= 0;
	muzzle_kick_time	= 0;
	muzzle_kick_maxtime	= 0;
	muzzle_kick_angles.Zero();
	muzzle_kick_offset.Zero();

	zoomFov = 90;

	barrelJointView		= INVALID_JOINT;
	flashJointView		= INVALID_JOINT;
	ejectJointView		= INVALID_JOINT;
	guiLightJointView	= INVALID_JOINT;
	ventLightJointView	= INVALID_JOINT;

	barrelJointWorld	= INVALID_JOINT;
	flashJointWorld		= INVALID_JOINT;
	ejectJointWorld		= INVALID_JOINT;

#ifdef _D3XP
	smokeJointView		= INVALID_JOINT;

	//Clean up the weapon particles
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		if(!part->smoke) {
			//Destroy the emitters
			part->emitter->PostEventMS(&EV_Remove, 0 );
		}
	}
	weaponParticles.Clear();

	//Clean up the weapon lights
	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		if ( light->lightHandle != -1 ) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
		}
	}
	weaponLights.Clear();
#endif

	hasBloodSplat		= false;
	nozzleFx			= false;
	nozzleFxFade		= 1500;
	lastAttack			= 0;
	nozzleGlowHandle	= -1;
	nozzleGlowShader	= NULL;
	nozzleGlowRadius	= 10;
	nozzleGlowColor.Zero();

	weaponAngleOffsetAverages	= 0;
	weaponAngleOffsetScale		= 0.0f;
	weaponAngleOffsetMax		= 0.0f;
	weaponOffsetTime			= 0.0f;
	weaponOffsetScale			= 0.0f;

	allowDrop			= true;

	animator.ClearAllAnims( gameLocal.time, 0 );
	FreeModelDef();

	sndHum				= NULL;

	isLinked			= false;
	projectileEnt		= NULL;

	isFiring			= false;

	//BC
	lastEndattack		= 0;
}

/*
================
idWeapon::InitWorldModel
================
*/
void idWeapon::InitWorldModel( const idDeclEntityDef *def ) {
	idEntity *ent;

	ent = worldModel.GetEntity();

	assert( ent );
	assert( def );

	const char *model = def->dict.GetString( "model_world" );
	const char *attach = def->dict.GetString( "joint_attach" );

	ent->SetSkin( NULL );
	if ( model[0] && attach[0] ) {
		ent->Show();
		ent->SetModel( model );
		if ( ent->GetAnimator()->ModelDef() ) {
			ent->SetSkin( ent->GetAnimator()->ModelDef()->GetDefaultSkin() );
		}
		ent->GetPhysics()->SetContents( 0 );
		ent->GetPhysics()->SetClipModel( NULL, 1.0f );
		ent->BindToJoint( owner, attach, true );
		ent->GetPhysics()->SetOrigin( vec3_origin );
		ent->GetPhysics()->SetAxis( mat3_identity );

		// supress model in player views, but allow it in mirrors and remote views
		renderEntity_t *worldModelRenderEntity = ent->GetRenderEntity();
		if ( worldModelRenderEntity ) {
			worldModelRenderEntity->suppressSurfaceInViewID = owner->entityNumber+1;
			worldModelRenderEntity->suppressShadowInViewID = owner->entityNumber+1;
			worldModelRenderEntity->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	} else {
		ent->SetModel( "" );
		ent->Hide();
	}

	flashJointWorld = ent->GetAnimator()->GetJointHandle( "flash" );
	barrelJointWorld = ent->GetAnimator()->GetJointHandle( "muzzle" );
	ejectJointWorld = ent->GetAnimator()->GetJointHandle( "eject" );
}

idMat3 idWeapon::GetWorldModelAxis()
{
	return worldModel.GetEntity()->GetPhysics()->GetAxis();
}

/*
================
idWeapon::GetWeaponDef
================
*/
void idWeapon::GetWeaponDef( const char *objectname, int ammoinclip )
{
	const char *shader;
	const char *objectType;
	const char *vmodel;
	const char *guiName;
	const char *projectileName;
	const char *brassDefName;
	const char *smokeName;
	int			ammoAvail;

	//BC
	const char *guiName2;

	const char *singleAmmoDefName;

	Clear();

	if ( !objectname || !objectname[ 0 ] ) {
		return;
	}

	assert( owner );

	weaponDef			= gameLocal.FindEntityDef( objectname );

	ammoType			= GetAmmoNumForName( weaponDef->dict.GetString( "ammoType" ) );
	ammoRequired		= weaponDef->dict.GetInt( "ammoRequired" );
	clipSize			= weaponDef->dict.GetInt( "clipSize" );
	lowAmmo				= weaponDef->dict.GetInt( "lowAmmo" );

	icon				= weaponDef->dict.GetString( "icon" );
	silent_fire			= weaponDef->dict.GetBool( "silent_fire" );
	powerAmmo			= weaponDef->dict.GetBool( "powerAmmo" );

	muzzle_kick_time	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= weaponDef->dict.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= weaponDef->dict.GetVector( "muzzle_kick_offset" );

	hideTime			= SEC2MS( weaponDef->dict.GetFloat( "hide_time", "0.3" ) );
	hideDistance		= weaponDef->dict.GetFloat( "hide_distance", "-15" );

	dynamicSpectrum = weaponDef->dict.GetBool("dynamicSpectrum", "0");

	// muzzle smoke
	smokeName = weaponDef->dict.GetString( "smoke_muzzle" );
	if ( *smokeName != '\0' ) {
		weaponSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		weaponSmoke = NULL;
	}
	continuousSmoke = weaponDef->dict.GetBool( "continuousSmoke" );
	
	weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;

	smokeName = weaponDef->dict.GetString( "smoke_strike" );
	if ( *smokeName != '\0' ) {
		strikeSmoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		strikeSmoke = NULL;
	}
	strikeSmokeStartTime = 0;
	strikePos.Zero();
	strikeAxis = mat3_identity;
	nextStrikeFx = 0;

	// setup gui light
	memset( &guiLight, 0, sizeof( guiLight ) );
	const char *guiLightShader = weaponDef->dict.GetString( "mtr_guiLightShader" );
	if ( *guiLightShader != '\0' )
	{
		idVec3 guiLightColor = weaponDef->dict.GetVector("color_guiLight", "1 1 1");
		guiLight.shader = declManager->FindMaterial( guiLightShader, false );
		guiLight.lightRadius[0] = guiLight.lightRadius[1] = guiLight.lightRadius[2] = weaponDef->dict.GetFloat("radius_guiLight", "3");
		guiLight.pointLight = true;
		guiLight.shaderParms[0] = guiLightColor.x;
		guiLight.shaderParms[1] = guiLightColor.y;
		guiLight.shaderParms[2] = guiLightColor.z;
	}

	// setup the view model
	vmodel = weaponDef->dict.GetString( "model_view" );
	SetModel( vmodel );

	// setup the world model
	InitWorldModel( weaponDef );

	// copy the sounds from the weapon view model def into out spawnargs
	const idKeyValue *kv = weaponDef->dict.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = weaponDef->dict.MatchPrefix( "snd_", kv );
	}

	// find some joints in the model for locating effects
	barrelJointView = animator.GetJointHandle(spawnArgs.GetString("bone_muzzle", "muzzle"));
	flashJointView = animator.GetJointHandle( "flash" );
	ejectJointView = animator.GetJointHandle(weaponDef->dict.GetString("bone_eject","")); //"eject"


	
	guiLightJointView = animator.GetJointHandle( "guiLight" );

	idStr ventJoint = weaponDef->dict.GetString("vent_joint");
	if (ventJoint.Length() > 0)
	{
		ventLightJointView = animator.GetJointHandle(ventJoint);
	}
	else
	{
		ventLightJointView = INVALID_JOINT;
	}
	

#ifdef _D3XP
	idStr smokeJoint = weaponDef->dict.GetString("smoke_joint");
	if(smokeJoint.Length() > 0)
	{
		smokeJointView = animator.GetJointHandle( smokeJoint );
	}
	else
	{
		smokeJointView = INVALID_JOINT;
	}
#endif

	// get the projectile
	projectileDict.Clear();

	projectileName = weaponDef->dict.GetString( "def_projectile" );
	if ( projectileName[0] != '\0' ) {
		const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( projectileName, false );
		if ( !projectileDef ) {
			gameLocal.Warning( "Unknown projectile '%s' in weapon '%s'", projectileName, objectname );
		} else {
			const char *spawnclass = projectileDef->dict.GetString( "spawnclass" );
			idTypeInfo *cls = idClass::GetClass( spawnclass );

			if ( !cls || !cls->IsType( idProjectile::Type ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' on projectile '%s' (used by weapon '%s')", spawnclass, projectileName, objectname );
			} else {
				projectileDict = projectileDef->dict;
			}
		}
	}

	// set up muzzleflash render light
	const idMaterial*flashShader;
	idVec3			flashTarget;
	idVec3			flashUp;
	idVec3			flashRight;
	float			flashRadius;
	bool			flashPointLight;

	weaponDef->dict.GetString( "mtr_flashShader", "", &shader );
	flashShader = declManager->FindMaterial( shader, false );
	flashPointLight = weaponDef->dict.GetBool( "flashPointLight", "1" );
	weaponDef->dict.GetVector( "flashColor", "0 0 0", flashColor );
	flashRadius		= (float)weaponDef->dict.GetInt( "flashRadius" );	// if 0, no light will spawn
	flashTime		= SEC2MS( weaponDef->dict.GetFloat( "flashTime", "0.25" ) );
	flashTarget		= weaponDef->dict.GetVector( "flashTarget" );
	flashUp			= weaponDef->dict.GetVector( "flashUp" );
	flashRight		= weaponDef->dict.GetVector( "flashRight" );

	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	muzzleFlash.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	muzzleFlash.allowLightInViewID = owner->entityNumber+1;

	// the weapon lights will only be in first person
	guiLight.allowLightInViewID = owner->entityNumber+1;
	nozzleGlow.allowLightInViewID = owner->entityNumber+1;

	muzzleFlash.pointLight								= flashPointLight;
	muzzleFlash.shader									= flashShader;
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= flashColor[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= flashColor[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= flashColor[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;

	muzzleFlash.lightRadius[0]							= flashRadius;
	muzzleFlash.lightRadius[1]							= flashRadius;
	muzzleFlash.lightRadius[2]							= flashRadius;

	if ( !flashPointLight ) {
		muzzleFlash.target								= flashTarget;
		muzzleFlash.up									= flashUp;
		muzzleFlash.right								= flashRight;
		muzzleFlash.end									= flashTarget;
	}

	// the world muzzle flash is the same, just positioned differently
	worldMuzzleFlash = muzzleFlash;
	worldMuzzleFlash.suppressLightInViewID = owner->entityNumber+1;
	worldMuzzleFlash.allowLightInViewID = 0;
	worldMuzzleFlash.lightId = LIGHTID_WORLD_MUZZLE_FLASH + owner->entityNumber;

	//-----------------------------------

	nozzleFx			= weaponDef->dict.GetBool("nozzleFx");
	nozzleFxFade		= weaponDef->dict.GetInt("nozzleFxFade", "1500");
	nozzleGlowColor		= weaponDef->dict.GetVector("nozzleGlowColor", "1 1 1");
	nozzleGlowRadius	= weaponDef->dict.GetFloat("nozzleGlowRadius", "10");
	weaponDef->dict.GetString( "mtr_nozzleGlowShader", "", &shader );
	nozzleGlowShader = declManager->FindMaterial( shader, false );

	// get the melee damage def
	meleeDistance = weaponDef->dict.GetFloat( "melee_distance" );
	meleeDefName = weaponDef->dict.GetString( "def_melee" );
	if ( meleeDefName.Length() ) {
		meleeDef = gameLocal.FindEntityDef( meleeDefName, false );
		if ( !meleeDef ) {
			gameLocal.Error( "Unknown melee '%s'", meleeDefName.c_str() );
		}
	}

	// get the brass def
	brassDict.Clear();
	brassDelay = weaponDef->dict.GetInt( "ejectBrassDelay", "0" );
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );

	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( !brassDef ) {
			gameLocal.Warning( "Unknown brass '%s'", brassDefName );
		} else {
			brassDict = brassDef->dict;
		}
	}

	if ( ( ammoType < 0 ) || ( ammoType >= AMMO_NUMTYPES ) ) {
		gameLocal.Warning( "Unknown ammotype in object '%s'", objectname );
	}

	ammoClip = max(0,ammoinclip);

	if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) )
	{
		// first time using this weapon so have it fully loaded to start
		ammoClip = clipSize;
		ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		
		if ( ammoClip > ammoAvail )
		{
			ammoClip = ammoAvail;
		}

		//In D3XP we use ammo as soon as it is moved into the clip. This allows for weapons that share ammo
		//owner->inventory.UseAmmo(ammoType, ammoClip);
		int weaponIndex = owner->inventory.GetWeaponIndex( owner, spawnArgs.GetString( "inv_weapon", "" ) );
		int weaponSlot = owner->inventory.GetHotbarslotViaWeaponIndex( weaponIndex );
		owner->inventory.hotbarSlots[weaponSlot].clip = ammoClip; //BC update clip array in player inventory. If we don't do this, then this ammo initialization gets called twice at gamestart and uses up 2x ammo.
	}

	renderEntity.gui[ 0 ] = NULL;
	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		renderEntity.gui[ 0 ] = uiManager->FindGui( guiName, true, false, true );
	}

	//BC second gui on weapon.
	guiName2 = weaponDef->dict.GetString("gui2");
	if (guiName2[0]) {
		renderEntity.gui[1] = uiManager->FindGui(guiName2, true, false, true);
	}

	zoomFov = weaponDef->dict.GetInt( "zoomFov", va("%d", DEFAULT_ZOOMFOV)); //default zoom fov
	berserk = weaponDef->dict.GetInt( "berserk", "2" );

	weaponAngleOffsetAverages = weaponDef->dict.GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale = weaponDef->dict.GetFloat( "weaponAngleOffsetScale", "0.25" );
	weaponAngleOffsetMax = weaponDef->dict.GetFloat( "weaponAngleOffsetMax", "10" );

	weaponOffsetTime = weaponDef->dict.GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale = weaponDef->dict.GetFloat( "weaponOffsetScale", "0.005" );

	if ( !weaponDef->dict.GetString( "weapon_scriptobject", NULL, &objectType ) ) {
		gameLocal.Error( "No 'weapon_scriptobject' set on '%s'.", objectname );
	}

	// setup script object
	if ( !scriptObject.SetType( objectType ) ) {
		gameLocal.Error( "Script object '%s' not found on weapon '%s'.", objectType, objectname );
	}

	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );


	WEAPON_RACKTHESLIDE.LinkTo(scriptObject, "WEAPON_RACKTHESLIDE");
	WEAPON_INSPECTMAGAZINE.LinkTo(scriptObject, "WEAPON_INSPECTMAGAZINE");
	WEAPON_INSPECTCHAMBER.LinkTo(scriptObject, "WEAPON_INSPECTCHAMBER");
	WEAPON_INSPECTWEAPON.LinkTo(scriptObject, "WEAPON_INSPECTWEAPON");

	WEAPON_AIMMODE.LinkTo(scriptObject, "WEAPON_AIMMODE");


	spawnArgs = weaponDef->dict;

	shader = spawnArgs.GetString( "snd_hum" );
	if ( shader && *shader ) {
		sndHum = declManager->FindSound( shader );
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}

	isLinked = true;

	// call script object's constructor
	ConstructScriptObject();

	// make sure we have the correct skin
	UpdateSkin();

#ifdef _D3XP
	idEntity *ent = worldModel.GetEntity();
	DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "1" ) );
	if ( ent )
	{
		ent->DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "1" ) );
	}

	//Initialize the particles
	if ( !gameLocal.isMultiplayer )
	{
		const idKeyValue *pkv = weaponDef->dict.MatchPrefix( "weapon_particle", NULL );

		while( pkv )
		{
			WeaponParticle_t newParticle;
			memset( &newParticle, 0, sizeof( newParticle ) );

			idStr name = pkv->GetValue();

			strcpy(newParticle.name, name.c_str());

			idStr jointName = weaponDef->dict.GetString(va("%s_joint", name.c_str()));
			newParticle.joint = animator.GetJointHandle(jointName.c_str());
			newParticle.smoke = weaponDef->dict.GetBool(va("%s_smoke", name.c_str()));
			newParticle.active = false;
			newParticle.startTime = 0;

			idStr particle = weaponDef->dict.GetString(va("%s_particle", name.c_str()));
			strcpy(newParticle.particlename, particle.c_str());

			if(newParticle.smoke) {
				newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particle, false ) );
			} else {
				idDict args;

				const idDeclEntityDef *emitterDef = gameLocal.FindEntityDef( "func_emitter", false );
				args = emitterDef->dict;
				args.Set("model", particle.c_str());
				args.SetBool("start_off", true);

				idEntity* ent;
				gameLocal.SpawnEntityDef(args, &ent, false);
				newParticle.emitter = (idFuncEmitter*)ent;

				newParticle.emitter->BecomeActive(TH_THINK);
			}

			weaponParticles.Set(name.c_str(), newParticle);

			pkv = weaponDef->dict.MatchPrefix( "weapon_particle", pkv );
		}

		const idKeyValue *lkv = weaponDef->dict.MatchPrefix( "weapon_light", NULL );
		while( lkv ) {
			WeaponLight_t newLight;
			memset( &newLight, 0, sizeof( newLight ) );

			newLight.lightHandle = -1;
			newLight.active = false;
			newLight.startTime = 0;

			idStr name = lkv->GetValue();
			strcpy(newLight.name, name.c_str());

			idStr jointName = weaponDef->dict.GetString(va("%s_joint", name.c_str()));
			newLight.joint = animator.GetJointHandle(jointName.c_str());

			idStr shader = weaponDef->dict.GetString(va("%s_shader", name.c_str()));
			newLight.light.shader = declManager->FindMaterial( shader, false );

			float radius = weaponDef->dict.GetFloat(va("%s_radius", name.c_str()));
			newLight.light.lightRadius[0] = newLight.light.lightRadius[1] = newLight.light.lightRadius[2] = radius;
			newLight.light.pointLight = true;
			newLight.light.noShadows = true;

			newLight.light.allowLightInViewID = owner->entityNumber+1;

			weaponLights.Set(name.c_str(), newLight);

			lkv = weaponDef->dict.MatchPrefix( "weapon_light", lkv );
		}
	}
#endif


	//bc
	autoaimEnabled = weaponDef->dict.GetBool("autoaimenabled", "0");
	isGrenade = weaponDef->dict.GetBool("isgrenade", "0");
	multiCarry = weaponDef->dict.GetBool("multiCarry", "0");

	displayName = spawnArgs.GetString("displayname");

	//Ejection ammo type.
	singleAmmoDict.Clear();
	singleAmmoDefName = weaponDef->dict.GetString("def_ejectammo");
	if (singleAmmoDefName[0])
	{
		const idDeclEntityDef *ejectAmmoDef = gameLocal.FindEntityDef(singleAmmoDefName, false);
		if (!ejectAmmoDef)
		{
			gameLocal.Warning("Unknown eject ammo '%s'", singleAmmoDefName);
		}
		else
		{
			singleAmmoDict = ejectAmmoDef->dict;
		}
	}

	nozzleGlowAffectLightMeter = weaponDef->dict.GetBool( "nozzleGlowAffectLightMeter", "0" );
}

/***********************************************************************

	GUIs

***********************************************************************/

/*
================
idWeapon::Icon
================
*/
const char *idWeapon::Icon( void ) const {
	return icon;
}

/*
================
idWeapon::UpdateGUI
================
*/
void idWeapon::UpdateGUI( void ) {
	if ( !renderEntity.gui[ 0 ] ) {
		return;
	}

	if ( status == WP_HOLSTERED ) {
		return;
	}

	if ( owner->weaponGone ) {
		// dropping weapons was implemented wierd, so we have to not update the gui when it happens or we'll get a negative ammo count
		return;
	}

	if ( gameLocal.localClientNum != owner->entityNumber ) {
		// if updating the hud for a followed client
		if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				return;
			}
		} else {
			return;
		}
	}

	int inclip = AmmoInClip();
	int ammoamount = AmmoAvailable();

	if ( ammoamount < 0 ) {
		// show infinite ammo
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", "" );
	} else {
		// show remaining ammo
#ifdef _D3XP
		renderEntity.gui[ 0 ]->SetStateString( "player_totalammo", va( "%i", ammoamount) );
#else
		renderEntity.gui[ 0 ]->SetStateString( "player_totalammo", va( "%i", ammoamount - inclip) );
#endif
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", ClipSize() ? va( "%i", inclip ) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_clips", ClipSize() ? va("%i", ammoamount / ClipSize()) : "--" );

#ifdef _D3XP
		renderEntity.gui[ 0 ]->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount ) );
#else
		renderEntity.gui[ 0 ]->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount - inclip ) );
#endif
	}
	renderEntity.gui[ 0 ]->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_empty", ( inclip == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_low", ( inclip <= lowAmmo ) );

#ifdef _D3XP
	//Let the HUD know the total amount of ammo regardless of the ammo required value
	renderEntity.gui[ 0 ]->SetStateString( "player_ammo_count", va("%i", AmmoCount()));

	//Grabber Gui Info
	renderEntity.gui[ 0 ]->SetStateString( "grabber_state", va("%i", grabberState));
#endif
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/

/*
================
idWeapon::UpdateFlashPosition
================
*/
void idWeapon::UpdateFlashPosition( void ) {
	// the flash has an explicit joint for locating it
	GetGlobalJointTransform( true, flashJointView, muzzleFlash.origin, muzzleFlash.axis );

	//bc to help debug flashlight direction:
	//gameRenderWorld->DebugArrow(colorGreen, muzzleFlash.origin, muzzleFlash.origin + muzzleFlash.axis.ToAngles().ToForward() * 64, 4, 100);

	// if the desired point is inside or very close to a wall, back it up until it is clear
	idVec3	start = muzzleFlash.origin - playerViewAxis[0] * 16;
	idVec3	end = muzzleFlash.origin + playerViewAxis[0] * 8;
	trace_t	tr;
	gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
	// be at least 8 units away from a solid
	muzzleFlash.origin = tr.endpos - playerViewAxis[0] * 8;
	

	// put the world muzzle flash on the end of the joint, no matter what
	GetGlobalJointTransform( false, flashJointWorld, worldMuzzleFlash.origin, worldMuzzleFlash.axis );
}

/*
================
idWeapon::MuzzleFlashLight
================
*/
void idWeapon::MuzzleFlashLight( void ) {

	if ( !lightOn && ( !g_muzzleFlash.GetBool() || !muzzleFlash.lightRadius[0] ) ) {
		return;
	}

	if ( flashJointView == INVALID_JOINT ) {
		return;
	}

	UpdateFlashPosition();

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]		= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	worldMuzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
	worldMuzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	muzzleFlash.noShadows = true; //BC turn off shadows.

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.time + flashTime;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
		worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
	}
}

/*
================
idWeapon::UpdateSkin
================
*/
bool idWeapon::UpdateSkin( void ) {
	const function_t *func;

	if ( !isLinked ) {
		return false;
	}

	func = scriptObject.GetFunction( "UpdateSkin" );
	if ( !func ) {
		common->Warning( "Can't find function 'UpdateSkin' in object '%s'", scriptObject.GetTypeName() );
		return false;
	}

	// use the frameCommandThread since it's safe to use outside of framecommands
	gameLocal.frameCommandThread->CallFunction( this, func, true );
	gameLocal.frameCommandThread->Execute();

	return true;
}

/*
================
idWeapon::SetModel
================
*/
void idWeapon::SetModel( const char *modelname ) {
	assert( modelname );

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

	// hide the model until an animation is played
	Hide();
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
			offset = offset * viewWeaponAxis + viewWeaponOrigin;
			axis = axis * viewWeaponAxis;
			return true;
		}
	} else {
		// world model
		if ( worldModel.GetEntity() && worldModel.GetEntity()->GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
			offset = worldModel.GetEntity()->GetPhysics()->GetOrigin() + offset * worldModel.GetEntity()->GetPhysics()->GetAxis();
			axis = axis * worldModel.GetEntity()->GetPhysics()->GetAxis();
			return true;
		}
	}
	offset = viewWeaponOrigin;
	axis = viewWeaponAxis;
	return false;
}

/*
================
idWeapon::SetPushVelocity
================
*/
void idWeapon::SetPushVelocity( const idVec3 &pushVelocity ) {
	this->pushVelocity = pushVelocity;
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
	// do nothing because the present is called from the player through PresentWeapon

	if (clipSize > 0)
	{
		bool showPrompt = false;

		if (WEAPON_INSPECTMAGAZINE)
		{
			if (state == "InspectMagazine")
			{
				owner->hud->SetStateString("attack_key", gameLocal.GetKeyFromBinding("_attack"));
				showPrompt = true;
			}
		}
		
		//owner->hud->SetStateBool("unloadprompt", showPrompt);
	}
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
	hasBloodSplat = false;
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
void idWeapon::Reload( void )
{
	if ( isLinked )
	{
		WEAPON_RELOAD = true;
	}
}



/*
================
idWeapon::LowerWeapon
================
*/
void idWeapon::LowerWeapon( void ) {
	if ( !hide ) {
		hideStart	= 0.0f;
		hideEnd		= hideDistance;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = true;
	}
}

/*
================
idWeapon::RaiseWeapon
================
*/
void idWeapon::RaiseWeapon( void ) {
	Show();

	if ( hide ) {
		hideStart	= hideDistance;
		hideEnd		= 0.0f;
		if ( gameLocal.time - hideStartTime < hideTime ) {
			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
		} else {
			hideStartTime = gameLocal.time;
		}
		hide = false;
	}
}

/*
================
idWeapon::HideWeapon
================
*/
void idWeapon::HideWeapon( void ) {
	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
	muzzleFlashEnd = 0;
}

/*
================
idWeapon::ShowWeapon
================
*/
void idWeapon::ShowWeapon( void ) {
	Show();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
	if ( lightOn ) {
		MuzzleFlashLight();
	}
}

/*
================
idWeapon::HideWorldModel
================
*/
void idWeapon::HideWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
idWeapon::ShowWorldModel
================
*/
void idWeapon::ShowWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
}

/*
================
idWeapon::OwnerDied
================
*/
void idWeapon::OwnerDied( void ) {
	if ( isLinked ) {
		SetState( "OwnerDied", 0 );
		thread->Execute();

#ifdef _D3XP
		// Update the grabber effects
		if ( /*!gameLocal.isMultiplayer &&*/ grabberState != -1 ) {
			grabber.Update( owner, hide );
		}
#endif
	}

	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}

	// don't clear the weapon immediately since the owner might have killed himself by firing the weapon
	// within the current stack frame
	PostEventMS( &EV_Weapon_Clear, 0 );
}

/*
================
idWeapon::BeginAttack
================
*/
void idWeapon::BeginAttack( void ) {
	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}

	if ( !isLinked ) {
		return;
	}

	if ( !WEAPON_ATTACK ) {
		if ( sndHum && grabberState == -1 ) {	// _D3XP :: don't stop grabber hum
			StopSound( SND_CHANNEL_BODY, false );
		}
	}
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
	if ( WEAPON_ATTACK ) {
		WEAPON_ATTACK = false;
		if ( sndHum && grabberState == -1 ) {	// _D3XP :: don't stop grabber hum
			StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
		}
	}
}

/*
================
idWeapon::isReady
================
*/
bool idWeapon::IsReady( void ) const {
	return !hide && !IsHidden() && ( ( status == WP_RELOAD ) || ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) || (status == WP_RACKSLIDE)  );
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
idWeapon::ShowCrosshair
================
*/
bool idWeapon::ShowCrosshair( void ) const {
	return !( state == idStr( WP_RISING ) || state == idStr( WP_LOWERING ) || state == idStr( WP_HOLSTERED ) );
}

/*
=====================
idWeapon::CanDrop
=====================
*/
bool idWeapon::CanDrop( void ) const {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return false;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[ 0 ] ) {
		return false;
	}
	return true;
}

/*
================
idWeapon::WeaponStolen
================
*/
void idWeapon::WeaponStolen( void )
{
	assert( !gameLocal.isClient );
	
	if ( projectileEnt )
	{
		if ( isLinked )
		{
			SetState( "WeaponStolen", 0 );
			thread->Execute();
		}
		projectileEnt = NULL;
	}

	// set to holstered so we can switch weapons right away
	status = WP_HOLSTERED;

	HideWeapon();
}

/*
=====================
idWeapon::DropItem
=====================
*/
idEntity * idWeapon::DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died, bool combatThrow)
{
	//idVec3 spawnPos;
	idVec3 right;

	if ( !weaponDef || !worldModel.GetEntity() ) {
		return NULL;
	}
	if ( !allowDrop ) {
		return NULL;
	}

	
	const char *classname = NULL;

	if (combatThrow)
	{
		//First attempt the throw item, and then fall back to dropItem.
		classname = weaponDef->dict.GetString("def_throwItem");
		if (!classname[0])
		{
			classname = weaponDef->dict.GetString("def_dropItem");
			if (!classname[0])
			{
				return NULL;
			}
		}
	}
	else
	{
		classname = weaponDef->dict.GetString("def_dropItem");
		if (!classname[0])
		{
			return NULL;
		}
	}

	StopSound( SND_CHANNEL_BODY, true );
	StopSound( SND_CHANNEL_BODY3, true );

	//BC when dropping item, spawn item at camera center so player can accurately throw things toward crosshair.
	//return idMoveableItem::DropItem( classname, worldModel.GetEntity()->GetPhysics()->GetOrigin(), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay );

	//spawnPos = owner->GetPhysics()->GetOrigin();
	//spawnPos.z = worldModel.GetEntity()->GetPhysics()->GetOrigin().z;

	//return idMoveableItem::DropItem(classname, GetMuzzlePosition(false), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay);

	//This is what gets called when the player throws the weapon.
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(NULL, &right, NULL); //We offset it a little for the throwing mechanic.
	return idMoveableItem::DropItem(classname, owner->firstPersonViewOrigin + right * QUICKTHROW_OFFSET_RIGHT, worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay, this->owner, combatThrow);
}

idVec3 idWeapon::GetThrowSpawnPosition()
{
    idVec3 right;
    gameLocal.GetLocalPlayer()->viewAngles.ToVectors(NULL, &right, NULL);
    return (owner->firstPersonViewOrigin + right * QUICKTHROW_OFFSET_RIGHT);
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
	const function_t *func;

	if ( !isLinked ) {
		return;
	}

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		//assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	thread->CallFunction( this, func, true );
	state = statename;

	animBlendFrames = blendFrames;
	if ( g_debugWeapon.GetBool() ) {
		gameLocal.Printf( "%dms weapon state : '%s'\n", gameLocal.time, statename );
	}

	idealState = "";
}


/***********************************************************************

	Particles/Effects

***********************************************************************/

/*
================
idWeapon::UpdateNozzelFx
================
*/
void idWeapon::UpdateNozzleFx( void ) {
	if ( !nozzleFx ) {
		return;
	}

	//
	// shader parms
	//
	int la = gameLocal.time - lastAttack + 1;
	float s = 1.0f;
	float l = 0.0f;
	if ( la < nozzleFxFade ) {
		s = ((float)la / nozzleFxFade);
		l = 1.0f - s;
	}
	renderEntity.shaderParms[5] = s;
	renderEntity.shaderParms[6] = l;

	if ( ventLightJointView == INVALID_JOINT )
	{
		return;
	}

	//
	// vent light
	//
	if ( nozzleGlowHandle == -1 )
	{
		memset(&nozzleGlow, 0, sizeof(nozzleGlow));
		if ( owner )
		{
			nozzleGlow.allowLightInViewID = owner->entityNumber+1;
		}
		nozzleGlow.pointLight = true;
		nozzleGlow.noShadows = true;

		//BC was nozzleGlowRadius
		nozzleGlow.lightRadius.x = .1f;
		nozzleGlow.lightRadius.y = .1f;
		nozzleGlow.lightRadius.z = .1f;

		nozzleGlow.shader = nozzleGlowShader;
		nozzleGlow.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
		nozzleGlow.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		nozzleGlow.affectLightMeter = nozzleGlowAffectLightMeter;
		GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );
		nozzleGlowHandle = gameRenderWorld->AddLightDef(&nozzleGlow);
	}

	

	

	GetGlobalJointTransform( true, ventLightJointView, nozzleGlow.origin, nozzleGlow.axis );

	nozzleGlow.shaderParms[ SHADERPARM_RED ] = nozzleGlowColor.x * s;
	nozzleGlow.shaderParms[ SHADERPARM_GREEN ] = nozzleGlowColor.y * s;
	nozzleGlow.shaderParms[ SHADERPARM_BLUE ] = nozzleGlowColor.z * s;
	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}


/*
================
idWeapon::BloodSplat
================
*/
bool idWeapon::BloodSplat( float size )
{
	idMat3 localAxis, axistemp;
	idVec3 localOrigin, normal;

	if ( hasBloodSplat )
	{
		return true;
	}

	hasBloodSplat = true;

	if ( modelDefHandle < 0 ) {
		return false;
	}

	if ( !GetGlobalJointTransform( true, ejectJointView, localOrigin, localAxis ) )
	{
		return false;
	}

	//const idMaterial *mtr = declManager->FindMaterial("textures/decals/bodywound");
	//gameRenderWorld->ProjectOverlay(modelDefHandle, localPlane, mtr);

	localOrigin[0] += gameLocal.random.CRandomFloat() * 4.0f;
	localOrigin[1] += gameLocal.random.CRandomFloat() * 4.0f;
	localOrigin[2] += gameLocal.random.CRandomFloat() * 2.0f;

	if (g_bloodEffects.GetBool())
	{
		ProjectOverlay(localOrigin, idVec3(0, 0, -1), size, "textures/decals/weaponsplat");
	}

	return true;

	/*

	localOrigin[0] += gameLocal.random.RandomFloat() * -10.0f;
	localOrigin[1] += gameLocal.random.RandomFloat() * 1.0f;
	localOrigin[2] += gameLocal.random.RandomFloat() * -2.0f;

	normal = idVec3( gameLocal.random.CRandomFloat(), -gameLocal.random.RandomFloat(), 1 );
	normal.Normalize();

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	localAxis[2] = -normal;
	localAxis[2].NormalVectors( axistemp[0], axistemp[1] );
	localAxis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	localAxis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	localAxis[0] *= 1.0f / size;
	localAxis[1] *= 1.0f / size;

	idPlane		localPlane[2];

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -(localOrigin * localAxis[0]) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -(localOrigin * localAxis[1]) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( "textures/decals/bodywound" );

	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	common->Printf("BloodSplat\n");
	return true;*/
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
idThread *idWeapon::ConstructScriptObject( void ) {
	const function_t *constructor;

	thread->EndThread();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for weapon", scriptObject.GetTypeName() );
	}

	// init the script object's data
	scriptObject.ClearObject();
	thread->CallFunction( this, constructor, true );
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
	const function_t *destructor;

	if ( !thread ) {
		return;
	}

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	thread->EndThread();

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and end
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		thread->EndThread();
	}

	// clear out the object's memory
	scriptObject.ClearObject();
}

/*
================
idWeapon::UpdateScript
================
*/
void idWeapon::UpdateScript( void ) {
	int	count;

	if ( !isLinked ) {
		return;
	}

	// only update the script on new frames
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( idealState.Length() ) {
		SetState( idealState, animBlendFrames );
	}

	// update script state, which may call Event_LaunchProjectiles, among other things
	count = 10;
	while( ( thread->Execute() || idealState.Length() ) && count-- ) {
		// happens for weapons with no clip (like grenades)
		if ( idealState.Length() ) {
			SetState( idealState, animBlendFrames );
		}
	}

	WEAPON_RELOAD = false;
	WEAPON_RACKTHESLIDE = false;
	//WEAPON_AIMMODE = false;
}

/*
================
idWeapon::AlertMonsters
================
*/
void idWeapon::AlertMonsters( void ) {
	trace_t	tr;
	idEntity *ent;
	idVec3 end = muzzleFlash.origin + muzzleFlash.axis * muzzleFlash.target;

	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}

	// jitter the trace to try to catch cases where a trace down the center doesn't hit the monster
	end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
	end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	gameLocal.clip.TracePoint( tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::Type ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}
}



/*
================
idWeapon::PresentWeapon
================
*/
void idWeapon::PresentWeapon( bool showViewModel ) {
	playerViewOrigin = owner->firstPersonViewOrigin;
	playerViewAxis = owner->firstPersonViewAxis;

	// calculate weapon position based on player movement bobbing
	owner->CalculateViewWeaponPos( viewWeaponOrigin, viewWeaponAxis );

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

	// set the physics position and orientation
	GetPhysics()->SetOrigin( viewWeaponOrigin );
	GetPhysics()->SetAxis( viewWeaponAxis );
	UpdateVisuals();

	// update the weapon script
	UpdateScript();

	UpdateGUI();

	// update animation
	UpdateAnimation();

	// only show the surface in player view
	renderEntity.allowSurfaceInViewID = owner->entityNumber+1;

	// crunch the depth range so it never pokes into walls this breaks the machine gun gui
	renderEntity.weaponDepthHack = true;

	// present the model
	if ( showViewModel ) {
		Present();
	} else {
		FreeModelDef();
	}

	if ( worldModel.GetEntity() && worldModel.GetEntity()->GetRenderEntity() ) {
		// deal with the third-person visible world model
		// don't show shadows of the world model in first person
		if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	}

	if ( nozzleFx )
	{
		UpdateNozzleFx();
	}

	// muzzle smoke
	if ( showViewModel && !disabled && weaponSmoke && ( weaponSmokeStartTime != 0 ) )
	{
		// use the barrel joint if available

		if(smokeJointView != INVALID_JOINT)
		{
			GetGlobalJointTransform( true, smokeJointView, muzzleOrigin, muzzleAxis );
		}
		else if (barrelJointView != INVALID_JOINT)
		{
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
		}
		else
		{
			// default to going straight out the view
			muzzleOrigin = playerViewOrigin;
			muzzleAxis = playerViewAxis;
		}

		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( weaponSmoke, weaponSmokeStartTime , gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup /*_D3XP*/ ) )
		{
			weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;
		}
	}

	if ( showViewModel && strikeSmoke && strikeSmokeStartTime != 0 )
	{
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( strikeSmoke, strikeSmokeStartTime, gameLocal.random.RandomFloat(), strikePos, strikeAxis, timeGroup /*_D3XP*/ ) )
		{
			strikeSmokeStartTime = 0;
		}
	}

#ifdef _D3XP
	if ( showViewModel && !hide ) {

		for( int i = 0; i < weaponParticles.Num(); i++ ) {
			WeaponParticle_t* part = weaponParticles.GetIndex(i);

			if(part->active) {
				if(part->smoke) {
					if(part->joint != INVALID_JOINT) {
						GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );
					} else {
						// default to going straight out the view
						muzzleOrigin = playerViewOrigin;
						muzzleAxis = playerViewAxis;
					}

					if ( !gameLocal.smokeParticles->EmitSmoke( part->particle, part->startTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup /*_D3XP*/ ) )
					{
						part->active = false;	// all done
						part->startTime = 0;
					}
				} else {
					//Manually update the position of the emitter so it follows the weapon
					renderEntity_t* rendEnt = part->emitter->GetRenderEntity();
					GetGlobalJointTransform( true, part->joint, rendEnt->origin, rendEnt->axis );

					if ( part->emitter->GetModelDefHandle() != -1 ) {
						gameRenderWorld->UpdateEntityDef( part->emitter->GetModelDefHandle(), rendEnt );
					}
				}
			}
		}

		for(int i = 0; i < weaponLights.Num(); i++)
		{
			WeaponLight_t* light = weaponLights.GetIndex(i);

			if(light->active) {

				GetGlobalJointTransform( true, light->joint, light->light.origin, light->light.axis );
				if ( ( light->lightHandle != -1 ) ) {
					gameRenderWorld->UpdateLightDef( light->lightHandle, &light->light );
				} else {
					light->lightHandle = gameRenderWorld->AddLightDef( &light->light );
				}
			}
		}
	}

	// Update the grabber effects
	if ( grabberState != -1 ) {
		grabberState = grabber.Update( owner, hide );
	}
#endif

	// remove the muzzle flash light when it's done
	if ( ( !lightOn && ( gameLocal.time >= muzzleFlashEnd ) ) || IsHidden() ) {
		if ( muzzleFlashHandle != -1 ) {
			gameRenderWorld->FreeLightDef( muzzleFlashHandle );
			muzzleFlashHandle = -1;
		}
		if ( worldMuzzleFlashHandle != -1 ) {
			gameRenderWorld->FreeLightDef( worldMuzzleFlashHandle );
			worldMuzzleFlashHandle = -1;
		}
	}

	// update the muzzle flash light, so it moves with the gun
	if ( muzzleFlashHandle != -1 ) {
		UpdateFlashPosition();
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );

		// wake up monsters with the flashlight
		if ( !gameLocal.isMultiplayer && lightOn && !owner->fl.notarget ) {
			AlertMonsters();
		}
	}

	// update the gui light
	if ( guiLight.lightRadius[0] && guiLightJointView != INVALID_JOINT )
	{
		GetGlobalJointTransform( true, guiLightJointView, guiLight.origin, guiLight.axis );

		if ( ( guiLightHandle != -1 ) )
		{
			gameRenderWorld->UpdateLightDef( guiLightHandle, &guiLight );
		}
		else
		{
			guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
		}
	}

	if ( status != WP_READY && sndHum ) {
		StopSound( SND_CHANNEL_BODY, false );
	}

	UpdateSound();

	if (nozzleLerping)
	{
		float lerpedRadius, lerp;
		
		lerp = (gameLocal.time - nozzleLerpTimer) / (float)NOZZLE_LERPTIME;
		if (lerp > 1)
			lerp = 1;

		lerpedRadius = idMath::Lerp(nozzleLerpValueStart, nozzleLerpValueEnd, lerp);		

		nozzleGlow.lightRadius.x = lerpedRadius;
		nozzleGlow.lightRadius.y = lerpedRadius;
		nozzleGlow.lightRadius.z = lerpedRadius;
		gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);

		if (lerp >= 1)
		{
			nozzleLerping = false;
		}
	}

	int currentWeaponType = owner->GetCurrentWeaponType();
	if (currentWeaponType > 0) //ignore 'unarmed' weapon.
	{
		if (owner->inventory.hotbarSlots[owner->currentWeaponSlot].health <= 1)
		{
			//Weapon is at critical low health.
			renderEntity.shaderParms[7] = 1;
		}
		else
		{
			//Health is not critical. Disable the pulse effect.
			renderEntity.shaderParms[7] = 0;
		}
	}
}

/*
================
idWeapon::EnterCinematic
================
*/
void idWeapon::EnterCinematic( void ) {
	StopSound( SND_CHANNEL_ANY, false );

	if ( isLinked ) {
		SetState( "EnterCinematic", 0 );
		thread->Execute();

		WEAPON_ATTACK		= false;
		WEAPON_RELOAD		= false;
		WEAPON_NETRELOAD	= false;
		WEAPON_NETENDRELOAD	= false;
		WEAPON_NETFIRING	= false;
		WEAPON_RAISEWEAPON	= false;
		WEAPON_LOWERWEAPON	= false;

#ifdef _D3XP
		grabber.Update( this->GetOwner(), true );
#endif
	}

	disabled = true;

	LowerWeapon();

	//BC allow zoomfov during spectator mode.
	zoomFov = DEFAULT_ZOOMFOV;
}

/*
================
idWeapon::ExitCinematic
================
*/
void idWeapon::ExitCinematic( void ) {
	disabled = false;

	if ( isLinked ) {
		SetState( "ExitCinematic", 0 );
		thread->Execute();
	}

	RaiseWeapon();
}

/*
================
idWeapon::NetCatchup
================
*/
void idWeapon::NetCatchup( void ) {
	if ( isLinked ) {
		SetState( "NetCatchup", 0 );
		thread->Execute();
	}
}

/*
================
idWeapon::GetZoomFov
================
*/
int	idWeapon::GetZoomFov( void ) {
	return zoomFov;
}

/*
================
idWeapon::GetWeaponAngleOffsets
================
*/
void idWeapon::GetWeaponAngleOffsets( int *average, float *scale, float *max ) {
	*average = weaponAngleOffsetAverages;
	*scale = weaponAngleOffsetScale;
	*max = weaponAngleOffsetMax;
}

/*
================
idWeapon::GetWeaponTimeOffsets
================
*/
void idWeapon::GetWeaponTimeOffsets( float *time, float *scale ) {
	*time = weaponOffsetTime;
	*scale = weaponOffsetScale;
}


/***********************************************************************

	Ammo

***********************************************************************/

/*
================
idWeapon::GetAmmoNumForName
================
*/
ammo_t idWeapon::GetAmmoNumForName( const char *ammoname ) {
	int num;
	const idDict *ammoDict;

	assert( ammoname );

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	if ( !ammoname[ 0 ] ) {
		return 0;
	}

	if ( !ammoDict->GetInt( ammoname, "-1", num ) ) {
#ifdef _D3XP
		//Lets look in a game specific ammo type definition for the weapon
		idStr gamedir;
		int i;
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 ) {
				gamedir = cvarSystem->GetCVarString( "fs_game_base" );
			} else if ( i == 1 ) {
				gamedir = cvarSystem->GetCVarString( "fs_game" );
			}
			if ( gamedir.Length() > 0 ) {
				ammoDict = gameLocal.FindEntityDefDict( va("ammo_types_%s", gamedir.c_str()), false );
				if ( ammoDict ) {
					if ( ammoDict->GetInt( ammoname, "-1", num ) ) {
						break;
					}
				}
			}
		}
		if ( i == 2 ) {
			gameLocal.Error( "Unknown ammo type '%s'", ammoname );
		}
#endif
	}


	if ( ( num < 0 ) || ( num >= AMMO_NUMTYPES ) ) {
		gameLocal.Error( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, AMMO_NUMTYPES );
	}

	return ( ammo_t )num;
}

/*
================
idWeapon::GetAmmoNameForNum
================
*/
const char *idWeapon::GetAmmoNameForNum( ammo_t ammonum ) {
	int i, j;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;
	char text[ 32 ];

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	sprintf( text, "%d", ammonum );

	num = ammoDict->GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		kv = ammoDict->GetKeyVal( i );
		if ( kv->GetValue() == text ) {
			return kv->GetKey();
		}
	}

#ifdef _D3XP
	// Look in the game specific ammo types
	idStr gamedir;
	for ( i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			gamedir = cvarSystem->GetCVarString( "fs_game_base" );
		} else if ( i == 1 ) {
			gamedir = cvarSystem->GetCVarString( "fs_game" );
		}
		if ( gamedir.Length() > 0 ) {
			ammoDict = gameLocal.FindEntityDefDict( va("ammo_types_%s", gamedir.c_str()), false );
			if ( ammoDict ) {
				num = ammoDict->GetNumKeyVals();
				for( j = 0; j < num; j++ ) {
					kv = ammoDict->GetKeyVal( j );
					if ( kv->GetValue() == text ) {
						return kv->GetKey();
					}
				}
			}
		}
	}
#endif

	return NULL;
}

/*
================
idWeapon::GetAmmoPickupNameForNum
================
*/
const char *idWeapon::GetAmmoPickupNameForNum( ammo_t ammonum ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;

	ammoDict = gameLocal.FindEntityDefDict( "ammo_names", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_names'\n" );
	}

	const char *name = GetAmmoNameForNum( ammonum );

	if ( name && *name ) {
		num = ammoDict->GetNumKeyVals();
		for( i = 0; i < num; i++ ) {
			kv = ammoDict->GetKeyVal( i );
			if ( idStr::Icmp( kv->GetKey(), name) == 0 ) {
				return common->GetLanguageDict()->GetString( kv->GetValue() ); //BC 3-24-2025: now returns localized name.
			}
		}
	}

	return "";
}

/*
================
idWeapon::AmmoAvailable
================
*/
int idWeapon::AmmoAvailable( void ) const {
	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, ammoRequired );
	} else {
		return 0;
	}
}

/*
================
idWeapon::AmmoInClip
================
*/
int idWeapon::AmmoInClip( void ) const {
	return ammoClip;
}

/*
================
idWeapon::ResetAmmoClip
================
*/
void idWeapon::ResetAmmoClip( void )
{
	ammoClip = -1;
}

/*
================
idWeapon::GetAmmoType
================
*/
ammo_t idWeapon::GetAmmoType( void ) const {
	return ammoType;
}

/*
================
idWeapon::ClipSize
================
*/
int	idWeapon::ClipSize( void ) const {
	return clipSize;
}

/*
================
idWeapon::LowAmmo
================
*/
int	idWeapon::LowAmmo() const {
	return lowAmmo;
}

/*
================
idWeapon::AmmoRequired
================
*/
int	idWeapon::AmmoRequired( void ) const {
	return ammoRequired;
}

#ifdef _D3XP
/*
================
idWeapon::GetGrabberState

Returns the current grabberState
================
*/
int idWeapon::GetGrabberState() const {

	return grabberState;
}

/*
================
idWeapon::AmmoCount

Returns the total number of rounds regardless of the required ammo
================
*/
int idWeapon::AmmoCount() const {

	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, 1 );
	} else {
		return 0;
	}
}
#endif

/*
================
idWeapon::WriteToSnapshot
================
*/
void idWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( ammoClip, ASYNC_PLAYER_INV_CLIP_BITS );
	msg.WriteBits( worldModel.GetSpawnId(), 32 );
	msg.WriteBits( lightOn, 1 );
	msg.WriteBits( isFiring ? 1 : 0, 1 );
}

/*
================
idWeapon::ReadFromSnapshot
================
*/
void idWeapon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ammoClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
	worldModel.SetSpawnId( msg.ReadBits( 32 ) );
	bool snapLight = msg.ReadBits( 1 ) != 0;
	isFiring = msg.ReadBits( 1 ) != 0;

	// WEAPON_NETFIRING is only turned on for other clients we're predicting. not for local client
	if ( owner && gameLocal.localClientNum != owner->entityNumber && WEAPON_NETFIRING.IsLinked() ) {

		// immediately go to the firing state so we don't skip fire animations
		if ( !WEAPON_NETFIRING && isFiring ) {
			idealState = "Fire";
		}

		// immediately switch back to idle
		if ( WEAPON_NETFIRING && !isFiring ) {
			idealState = "Idle";
		}

		WEAPON_NETFIRING = isFiring;
	}

	if ( snapLight != lightOn ) {
		Reload();
	}
}

/*
================
idWeapon::ClientReceiveEvent
================
*/
bool idWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_RELOAD: {
			if ( gameLocal.time - time < 1000 ) {
				if ( WEAPON_NETRELOAD.IsLinked() ) {
					WEAPON_NETRELOAD = true;
					WEAPON_NETENDRELOAD = false;
				}
			}
			return true;
		}
		case EVENT_ENDRELOAD: {
			if ( WEAPON_NETENDRELOAD.IsLinked() ) {
				WEAPON_NETENDRELOAD = true;
			}
			return true;
		}
		case EVENT_CHANGESKIN: {
			int index = gameLocal.ClientRemapDecl( DECL_SKIN, msg.ReadInt() );
			renderEntity.customSkin = ( index != -1 ) ? static_cast<const idDeclSkin *>( declManager->DeclByIndex( DECL_SKIN, index ) ) : NULL;
			UpdateVisuals();
			if ( worldModel.GetEntity() ) {
				worldModel.GetEntity()->SetSkin( renderEntity.customSkin );
			}
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
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
	idThread::ReturnEntity( owner );
}

/*
===============
idWeapon::Event_WeaponState
===============
*/
void idWeapon::Event_WeaponState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		//assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	idealState = statename;

	if ( !idealState.Icmp( "Fire" ) ) {
		isFiring = true;
	} else {
		isFiring = false;
	}

	animBlendFrames = blendFrames;
	thread->DoneProcessing();
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
	if ( sndHum ) {
		StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
	}

}

/*
===============
idWeapon::Event_WeaponOutOfAmmo
===============
*/
void idWeapon::Event_WeaponOutOfAmmo( void ) {
	status = WP_OUTOFAMMO;
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
idWeapon::Event_UseAmmo
===============
*/
bool idWeapon::Event_UseAmmo( int amount )
{
	if ( gameLocal.isClient )
	{
		return false;
	}

	
	if ( clipSize && ammoRequired && ammoClip > 0)
	{
		ammoClip -= powerAmmo ? amount : ( amount * ammoRequired );

		if ( ammoClip < 0 )
		{
			ammoClip = 0;
		}

		return true;
	}
	else
	{
		owner->inventory.UseAmmo(ammoType, (powerAmmo) ? amount : (amount * ammoRequired));
	}

	return false;
}

/*
===============
idWeapon::Event_AddToClip
===============
*/
void idWeapon::Event_AddToClip( int amount ) {
	int ammoAvail;

	if ( gameLocal.isClient ) {
		return;
	}

#ifdef _D3XP
	int oldAmmo = ammoClip;
	ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired ) + AmmoInClip();
#endif

	ammoClip += amount;
	if ( ammoClip > clipSize )
	{
		ammoClip = clipSize;
	}

#ifdef _D3XP
#else
	ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
#endif

	if ( ammoClip > ammoAvail ) {
		ammoClip = ammoAvail;
	}

#ifdef _D3XP
	// for shared ammo we need to use the ammo when it is moved into the clip
	int usedAmmo = ammoClip - oldAmmo;
	owner->inventory.UseAmmo(ammoType, usedAmmo);
#endif
}




/*
===============
idWeapon::Event_AmmoInClip
===============
*/
void idWeapon::Event_AmmoInClip( void )
{
	int ammo = AmmoInClip();
	idThread::ReturnFloat( ammo );
}

/*
===============
idWeapon::Event_AmmoAvailable
===============
*/
void idWeapon::Event_AmmoAvailable( void ) {
#ifdef _D3XP
	int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	ammoAvail += AmmoInClip();
#else
	int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
#endif

	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_TotalAmmoCount
===============
*/
void idWeapon::Event_TotalAmmoCount( void ) {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, 1 );
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_ClipSize
===============
*/
void idWeapon::Event_ClipSize( void ) {
	idThread::ReturnFloat( clipSize );
}

/*
===============
idWeapon::Event_AutoReload
===============
*/
void idWeapon::Event_AutoReload( void ) {
	assert( owner );
	
	//BC disable auto reload.
	idThread::ReturnFloat(0.0f);
	return;
	
	/*if ( gameLocal.isClient )
	{
		idThread::ReturnFloat( 0.0f );
		return;
	}
	idThread::ReturnFloat( gameLocal.userInfo[ owner->entityNumber ].GetBool( "ui_autoReload" ) );*/
}

/*
===============
idWeapon::Event_NetReload
===============
*/
void idWeapon::Event_NetReload( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RELOAD, NULL, false, -1 );
	}
}

/*
===============
idWeapon::Event_NetEndReload
===============
*/
void idWeapon::Event_NetEndReload( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_ENDRELOAD, NULL, false, -1 );
	}
}

/*
===============
idWeapon::Event_PlayAnim
===============
*/
void idWeapon::Event_PlayAnim( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			if ( anim ) {
				worldModel.GetEntity()->GetAnimator()->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
			}
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_PlayCycle
===============
*/
void idWeapon::Event_PlayCycle( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		if ( !( owner && owner->GetInfluenceLevel() ) ) {
			Show();
		}
		animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = animator.CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			worldModel.GetEntity()->GetAnimator()->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
		}
	}
	animBlendFrames = 0;
	idThread::ReturnInt( 0 );
}

/*
===============
idWeapon::Event_AnimDone
===============
*/
void idWeapon::Event_AnimDone( int channel, int blendFrames ) {
	if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idWeapon::Event_SetBlendFrames
===============
*/
void idWeapon::Event_SetBlendFrames( int channel, int blendFrames ) {
	animBlendFrames = blendFrames;
}

/*
===============
idWeapon::Event_GetBlendFrames
===============
*/
void idWeapon::Event_GetBlendFrames( int channel ) {
	idThread::ReturnInt( animBlendFrames );
}

/*
================
idWeapon::Event_Next
================
*/
void idWeapon::Event_Next( void ) {
	// change to another weapon if possible
	owner->NextBestWeapon();
}

/*
================
idWeapon::Event_SetSkin
================
*/
void idWeapon::Event_SetSkin( const char *skinname ) {
	const idDeclSkin *skinDecl;

	if ( !skinname || !skinname[ 0 ] ) {
		skinDecl = NULL;
	} else {
		skinDecl = declManager->FindSkin( skinname );
	}

	renderEntity.customSkin = skinDecl;
	UpdateVisuals();

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetSkin( skinDecl );
	}

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteInt( ( skinDecl != NULL ) ? gameLocal.ServerRemapDecl( -1, DECL_SKIN, skinDecl->Index() ) : -1 );
		ServerSendEvent( EVENT_CHANGESKIN, &msg, false, -1 );
	}
}

/*
================
idWeapon::Event_Flashlight
================
*/
void idWeapon::Event_Flashlight( int enable ) {
	if ( enable ) {
		lightOn = true;
		MuzzleFlashLight();
	} else {
		lightOn = false;
		muzzleFlashEnd = 0;
	}
}

/*
================
idWeapon::Event_GetLightParm
================
*/
void idWeapon::Event_GetLightParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( muzzleFlash.shaderParms[ parmnum ] );
}

/*
================
idWeapon::Event_SetLightParm
================
*/
void idWeapon::Event_SetLightParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	muzzleFlash.shaderParms[ parmnum ]		= value;
	worldMuzzleFlash.shaderParms[ parmnum ]	= value;
	UpdateVisuals();
}

/*
================
idWeapon::Event_SetLightParms
================
*/
void idWeapon::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= parm0;
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= parm1;
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= parm2;
	muzzleFlash.shaderParms[ SHADERPARM_ALPHA ]			= parm3;

	worldMuzzleFlash.shaderParms[ SHADERPARM_RED ]		= parm0;
	worldMuzzleFlash.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	worldMuzzleFlash.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	worldMuzzleFlash.shaderParms[ SHADERPARM_ALPHA ]	= parm3;

	UpdateVisuals();
}

#ifdef _D3XP
/*
================
idWeapon::Event_Grabber
================
*/
void idWeapon::Event_Grabber( int enable ) {
	if ( enable ) {
		grabberState = 0;
	} else {
		grabberState = -1;
	}
}

/*
================
idWeapon::Event_GrabberHasTarget
================
*/
void idWeapon::Event_GrabberHasTarget() {
	idThread::ReturnInt( grabberState );
}

/*
================
idWeapon::Event_GrabberSetGrabDistance
================
*/
void idWeapon::Event_GrabberSetGrabDistance( float dist ) {

	grabber.SetDragDistance( dist );
}
#endif

/*
================
idWeapon::Event_CreateProjectile
================
*/
void idWeapon::Event_CreateProjectile( void ) {
	if ( !gameLocal.isClient ) {
		projectileEnt = NULL;
		gameLocal.SpawnEntityDef( projectileDict, &projectileEnt, false );
		if ( projectileEnt ) {
			projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
			projectileEnt->Bind( owner, false );
			projectileEnt->Hide();
		}
		idThread::ReturnEntity( projectileEnt );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

/*
================
idWeapon::Event_LaunchProjectiles
================
*/
void idWeapon::Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower, float chamberNewRound) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	if ( IsHidden() )
	{
		return;
	}

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on an MP client
	if ( !gameLocal.isClient )
	{
		//if ( ( clipSize != 0 ) && ( ammoClip <= 0 ) )

		
		if (!Event_RoundIsChambered() && clipSize != 0) //BC no round has been chambered.
		{
			//Dry fire.
			return;
		}
		
		//We use the round in the chamber.		
		Event_SetChambered(false);
		

		
		// if this is a power ammo weapon ( currently only the bfg ) then make sure
		// we only fire as much power as available in each clip
		if ( powerAmmo )
		{
			// power comes in as a float from zero to max
			// if we use this on more than the bfg will need to define the max
			// in the .def as opposed to just in the script so proper calcs
			// can be done here.
			dmgPower = ( int )dmgPower + 1;
			if ( dmgPower > ammoClip )
			{
				dmgPower = ammoClip;
			}
		}


		if(clipSize == 0)
		{
			//Weapons with a clip size of 0 launch strait from inventory without moving to a clip
			//In D3XP we used the ammo when the ammo was moved into the clip so we don't want to
			//use it now.
			owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? dmgPower : ammoRequired );
		}


		if ( clipSize && ammoRequired && ammoClip >= ammoRequired && chamberNewRound)
		{
			
			//Move the round from the mag --> into the chamber.
			Event_SetChambered(true);
			ammoClip -= powerAmmo ? dmgPower : ammoRequired;
			
		}
	}

	if ( !silent_fire )
	{
		//Make a suspicious noise marker when weapon is fired.

		if (owner)
		{
			if (owner == gameLocal.GetLocalPlayer()) //Only happens for player guns... this kinda sucks
			{
				gameLocal.SpawnInterestPoint(owner, owner->GetEyePosition(), "interest_weaponfire");
			}
		}		
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.realClientTime );

	if ( worldModel.GetEntity() )
	{
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ))
	{
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	}
	else
	{
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.realClientTime )
	{
		kick_endtime = gameLocal.realClientTime;
	}

	kick_endtime += muzzle_kick_time;

	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime )
	{
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	if ( gameLocal.isClient )
	{
		// predict instant hit projectiles
		if ( projectileDict.GetBool( "net_instanthit" ) ) {
			float spreadRad = DEG2RAD( spread );
			muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
			for( i = 0; i < num_projectiles; i++ ) {
				ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
				spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
				dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
				dir.Normalize();
				gameLocal.clip.Translation( tr, muzzle_pos, muzzle_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
				if ( tr.fraction < 1.0f ) {
					idProjectile::ClientPredictionCollide( this, projectileDict, tr, vec3_origin, true );
				}
			}
		}

	}
	else
	{
		//FIRE THE WEAPON HERE.

		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ )
		{
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();

			//BC handle autoaim.
			if (owner->autoaimPos == vec3_zero || !autoaimEnabled)
			{
				//no autoaim. Apply bullet spread.

				
				//For multi-projectile weapon (shotguns), guarantee first bullet hits center crosshair. OR, for single-projectile weapons, make first shot hit center crosshair on first shot.
				if ((num_projectiles > 1 && i <= 0) || (num_projectiles == 1 && gameLocal.time - lastEndattack > 500)) 
					dir = playerViewAxis[0];				
				else
					dir = playerViewAxis[0] + playerViewAxis[2] * (ang * idMath::Sin(spin)) - playerViewAxis[1] * (ang * idMath::Cos(spin)); //normal bullet spread.

				dir.Normalize();
			}
			else
			{
				//autoaim.
				dir = owner->autoaimPos - muzzleOrigin;
				dir.Normalize();
			}
			
			
			//Fire projectile.

			if ( projectileEnt )
			{
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			}
			else
			{
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) )
			{
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			if ( projectileDict.GetBool( "net_instanthit" ) )
			{
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			if (owner->autoaimPos != vec3_zero && autoaimEnabled)
			{
				//BC this is an autoaim'd bullet.
				idVec3 lockDir = owner->autoaimPos - muzzleOrigin;
				lockDir.Normalize();
				muzzle_pos = owner->autoaimPos + (lockDir * -8);

				if (gameLocal.GetLocalPlayer()->autoaimEnt.GetEntity() != NULL)
				{
					//Guarantee the damage.
					proj->SetOrigin(owner->autoaimPos + idVec3(0,0,-6));
					gameLocal.GetLocalPlayer()->autoaimEnt.GetEntity()->Damage(proj, owner, vec3_zero, "damage_autoaim", 1.0f, 0);
					proj->fl.hidden = true;
				}
			}			
			else if ( i == 0 ) // make sure the projectile starts inside the bounding box of the owner
			{
				muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;

				// DG: sometimes the assertion in idBounds::operator-(const idBounds&) triggers
				//     (would get bounding box with negative volume)
				//     => check that before doing ownerBounds - projBounds (equivalent to the check in the assertion)
				idVec3 obDiff = ownerBounds[1] - ownerBounds[0];
				idVec3 pbDiff = projBounds[1] - projBounds[0];
				bool boundsSubLegal =  obDiff.x > pbDiff.x && obDiff.y > pbDiff.y && obDiff.z > pbDiff.z;
				if ( boundsSubLegal && ( ownerBounds - projBounds ).RayIntersection( muzzle_pos, playerViewAxis[0], distance ) ) {
					start = muzzle_pos + distance * playerViewAxis[0];
				} else {
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower );

			if (owner->airless)
			{
				//BC push player backward when they fire a gun in zero g.
				int pushback = spawnArgs.GetInt("pushback_zerog", "48");
				owner->GetPhysics()->SetLinearVelocity(owner->GetPhysics()->GetLinearVelocity() + (owner->viewAngles.ToForward() * (pushback * -1)));
			}
		}

		// toss the brass

		if (brassDelay >= 0)
		{
			PostEventMS(&EV_Weapon_EjectBrass, brassDelay);
		}

		lastEndattack = gameLocal.time;
	}

	// add the light for the muzzleflash
	if ( !lightOn )
	{
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.realClientTime;

	
	//BC a projectile was launched (gun was fired)
	if (clipSize && ammoRequired)
	{
		if (ammoClip <= 0 && !Event_RoundIsChambered())
		{
			owner->SayVO_WithIntervalDelay_msDelayed("snd_vo_gun_noammo", 300);
			//BC NOTE: This vo cue is also triggered in weapon_base.script		
		}
		else if (ammoClip <= 2 && WEAPON_ATTACK == false)
		{
			owner->SayVO_WithIntervalDelay_msDelayed("snd_vo_gun_lowammo", 300);
		}
	}

	CheckPlayerFootShot();
}

void idWeapon::CheckPlayerFootShot()
{
	if (owner != gameLocal.GetLocalPlayer())
		return;
	
	if (gameLocal.GetLocalPlayer()->IsCrouching() || gameLocal.GetLocalPlayer()->viewAngles.pitch < 88.5f)
		return;
	
	trace_t footTr;
	gameLocal.clip.TracePoint(footTr,
		gameLocal.GetLocalPlayer()->firstPersonViewOrigin,
		gameLocal.GetLocalPlayer()->firstPersonViewOrigin + gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 128,
		MASK_SHOT_RENDERMODEL, NULL);

	if (footTr.fraction >= 1)
		return;
	
	if (footTr.c.entityNum == gameLocal.GetLocalPlayer()->entityNumber)
	{
		//Traceline is hitting player's own foot (or any body part)
		idStr damageDefName = projectileDict.GetString("def_damage");
		if (damageDefName.Length() > 0)
		{
			gameLocal.GetLocalPlayer()->Damage(this, owner, vec3_zero, damageDefName.c_str(), 1.0f, 0);
		}
	}
}

#ifdef _D3XP
/*
================
idWeapon::Event_LaunchProjectilesEllipse
================
*/
void idWeapon::Event_LaunchProjectilesEllipse( int num_projectiles, float spreada, float spreadb, float fuseOffset, float power ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			anga, angb;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	if ( IsHidden() ) {
		return;
	}

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on a client
	if ( !gameLocal.isClient ) {

		if ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) {
			return;
		}

		if( clipSize == 0 ) {
			//Weapons with a clip size of 0 launch strait from inventory without moving to a clip
			owner->inventory.UseAmmo( ammoType, ammoRequired );
		}

		if ( clipSize && ammoRequired ) {
			ammoClip -= ammoRequired;
		}

		if ( !silent_fire )
		{
			// wake up nearby monsters when weapon is fired.
			gameLocal.SetSuspiciousNoise(this, owner->GetPhysics()->GetOrigin(), spawnArgs.GetInt("noise_radius", "1024"), NOISE_COMBATPRIORITY);
		}

	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDict.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	} else {
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.time ) {
		kick_endtime = gameLocal.time;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.time + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.time + muzzle_kick_maxtime;
	}

	if ( !gameLocal.isClient ) {
		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRadA = DEG2RAD( spreada );
		float spreadRadB = DEG2RAD( spreadb );

		for( i = 0; i < num_projectiles; i++ ) {
			//Ellipse Form
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			anga = idMath::Sin(spreadRadA * gameLocal.random.RandomFloat());
			angb = idMath::Sin(spreadRadB * gameLocal.random.RandomFloat());
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( angb*idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( anga*idMath::Cos( spin ) );
			dir.Normalize();

			gameLocal.SpawnEntityDef( projectileDict, &ent );
			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
				muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
				if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, playerViewAxis[0], distance ) ) {
					start = muzzle_pos + distance * playerViewAxis[0];
				}
				else {
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, power );
		}

		// toss the brass
		if( brassDelay >= 0 ) {
			PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
		}
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.time;

}

/**
* Gives the player a powerup as if it were a weapon shot. It will use the ammo amount specified
* as ammoRequired.
*/
void idWeapon::Event_LaunchPowerup( const char* powerup, float duration, int useAmmo ) {

	if ( IsHidden() ) {
		return;
	}

	// check if we're out of ammo
	if(useAmmo) {
		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( !ammoAvail ) {
			return;
		}
		owner->inventory.UseAmmo( ammoType, ammoRequired );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->Give(powerup, va("%f", duration), health);


}

void idWeapon::Event_StartWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.time;
}

void idWeapon::Event_StopWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = 0;
}

void idWeapon::Event_StartWeaponParticle( const char* name) {
	WeaponParticle_t* part;
	weaponParticles.Get(name, &part);
	if(part) {
		part->active = true;
		part->startTime = gameLocal.time;

		//Toggle the emitter
		if(!part->smoke) {
			part->emitter->Show();
			part->emitter->PostEventMS(&EV_Activate, 0, this);
		}
	}
}

void idWeapon::Event_StopWeaponParticle( const char* name) {
	WeaponParticle_t* part;
	weaponParticles.Get(name, &part);
	if(part) {
		part->active = false;
		part->startTime = 0;

		//Toggle the emitter
		if(!part->smoke) {
			part->emitter->Hide();
			part->emitter->PostEventMS(&EV_Activate, 0, this);
		}
	}
}

void idWeapon::Event_StartWeaponLight( const char* name) {
	WeaponLight_t* light;
	weaponLights.Get(name, &light);
	if(light) {
		light->active = true;
		light->startTime = gameLocal.time;
	}
}

void idWeapon::Event_StopWeaponLight( const char* name) {
	WeaponLight_t* light;
	weaponLights.Get(name, &light);
	if(light) {
		light->active = false;
		light->startTime = 0;
		if(light->lightHandle != -1) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
			light->lightHandle = -1;
		}
	}
}
#endif
/*
=====================
idWeapon::Event_Melee
=====================
*/
void idWeapon::Event_Melee( void ) {
	idEntity	*ent;
	trace_t		tr;

	if ( !meleeDef ) {
		gameLocal.Error( "No meleeDef on '%s'", weaponDef->dict.GetString( "classname" ) );
	}

	if ( !gameLocal.isClient ) {
		idVec3 start = playerViewOrigin;
		idVec3 end = start + playerViewAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
		if ( tr.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( tr );
		} else {
			ent = NULL;
		}

		if ( g_debugWeapon.GetBool() ) {
			gameRenderWorld->DebugLine( colorYellow, start, end, 100 );
			if ( ent ) {
				gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 100 );
			}
		}

		bool hit = false;
		const char *hitSound = meleeDef->dict.GetString( "snd_miss" );

		if ( ent ) {

			float push = meleeDef->dict.GetFloat( "push" );
			idVec3 impulse = -push * owner->PowerUpModifier( SPEED ) * tr.c.normal;

			if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) && ( ent->IsType( idActor::Type ) || ent->IsType( idAFAttachment::Type) ) ) {
				idThread::ReturnInt( 0 );
				return;
			}

			ent->ApplyImpulse( this, tr.c.id, tr.c.point, impulse );

			// weapon stealing - do this before damaging so weapons are not dropped twice
			if ( gameLocal.isMultiplayer
				&& weaponDef && weaponDef->dict.GetBool( "stealing" )
				&& ent->IsType( idPlayer::Type )
				&& !owner->PowerUpActive( BERSERK )
				&& ( (gameLocal.gameType != GAME_TDM ) || gameLocal.serverInfo.GetBool( "si_teamDamage" ) || ( owner->team != static_cast< idPlayer * >( ent )->team ) )
				) {

#ifdef CTF      /* Code is formed oddly for easy merge */

				if ( gameLocal.mpGame.IsGametypeFlagBased() )
				{ /* Do nothing ... */ }
				else
#endif
				owner->StealWeapon( static_cast< idPlayer * >( ent ) );
			}

			if ( ent->fl.takedamage ) {
				idVec3 kickDir, globalKickDir;
				meleeDef->dict.GetVector( "kickDir", "0 0 0", kickDir );
				globalKickDir = muzzleAxis * kickDir;
#ifdef _D3XP
				//Adjust the melee powerup modifier for the invulnerability boss fight
				float mod = owner->PowerUpModifier( MELEE_DAMAGE );
				if(!strcmp(ent->GetEntityDefName(), "monster_hunter_invul")) {
					//Only do a quater of the damage mod
					mod *= 0.25f;
				}
				ent->Damage( owner, owner, globalKickDir, meleeDefName, mod, tr.c.id );
#else
				ent->Damage( owner, owner, globalKickDir, meleeDefName, owner->PowerUpModifier( MELEE_DAMAGE ), tr.c.id );
#endif
				hit = true;
			}

			if ( weaponDef->dict.GetBool( "impact_damage_effect" ) ) {

				if ( ent->spawnArgs.GetBool( "bleed" ) ) {

					hitSound = meleeDef->dict.GetString( owner->PowerUpActive( BERSERK ) ? "snd_hit_berserk" : "snd_hit" );

					ent->AddDamageEffect( tr, impulse, meleeDef->dict.GetString( "classname" ) );

				} else {

					int type = tr.c.material->GetSurfaceType();
					if ( type == SURFTYPE_NONE ) {
						type = GetDefaultSurfaceType();
					}

					const char *materialType = gameLocal.sufaceTypeNames[ type ];

					// start impact sound based on material type
					hitSound = meleeDef->dict.GetString( va( "snd_%s", materialType ) );
					if ( *hitSound == '\0' ) {
						hitSound = meleeDef->dict.GetString( "snd_metal" );
					}

					if ( gameLocal.time > nextStrikeFx ) {
						const char *decal;
						// project decal
						decal = weaponDef->dict.GetString( "mtr_strike" );
						if ( decal && *decal ) {
							gameLocal.ProjectDecal( tr.c.point, -tr.c.normal, 8.0f, true, 6.0, decal );
						}
						nextStrikeFx = gameLocal.time + 200;
					} else {
						hitSound = "";
					}

					strikeSmokeStartTime = gameLocal.time;
					strikePos = tr.c.point;
					strikeAxis = -tr.endAxis;
				}
			}
		}

		if ( *hitSound != '\0' ) {
			const idSoundShader *snd = declManager->FindSound( hitSound );
			StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
		}

		idThread::ReturnInt( hit );
		owner->WeaponFireFeedback( &weaponDef->dict );
		return;
	}

	idThread::ReturnInt( 0 );
	owner->WeaponFireFeedback( &weaponDef->dict );
}

/*
=====================
idWeapon::Event_GetWorldModel
=====================
*/
void idWeapon::Event_GetWorldModel( void ) {
	idThread::ReturnEntity( worldModel.GetEntity() );
}

/*
=====================
idWeapon::Event_AllowDrop
=====================
*/
void idWeapon::Event_AllowDrop( int allow ) {
	if ( allow ) {
		allowDrop = true;
	} else {
		allowDrop = false;
	}
}

/*
================
idWeapon::Event_EjectBrass

Toss a shell model out from the breach if the bone is present
================
*/
void idWeapon::Event_EjectBrass( void )
{
	if ( !g_showBrass.GetBool() || !owner->CanShowWeaponViewmodel() )
	{
		return;
	}

	if ( ejectJointView == INVALID_JOINT || !brassDict.GetNumKeyVals() )
	{
		return;
	}

	if ( gameLocal.isClient )
	{
		return;
	}

	idMat3 axis;
	idVec3 origin, linear_velocity, angular_velocity;
	idEntity *ent;

	if ( !GetGlobalJointTransform( true, ejectJointView, origin, axis ) )
	{
		return;
	}

	gameLocal.SpawnEntityDef( brassDict, &ent, false );
	if ( !ent || !ent->IsType( idDebris::Type ) )
	{
		gameLocal.Error( "'%s' is not an idDebris", weaponDef ? weaponDef->dict.GetString( "def_ejectBrass" ) : "def_ejectBrass" );
	}

	

	idDebris *debris = static_cast<idDebris *>(ent);
	debris->Create( owner, origin, axis );
	debris->Launch();
	debris->UpdateGravity();

	

	//gameRenderWorld->DebugSphere(colorRed, idSphere(origin, 0.5f), 4000, true);

	
	

	//linear_velocity = (30 + (gameLocal.random.CRandomFloat()*10)) * ( playerViewAxis[0] + playerViewAxis[1] + playerViewAxis[2] );

	//BC right-side ejection port

	linear_velocity = idVec3(0, 0, 70 + (gameLocal.random.CRandomFloat() * 30)) + (playerViewAxis[1] * -20) + (playerViewAxis[0] * (40 + (gameLocal.random.CRandomFloat() * 30)));

	angular_velocity.Set( 400 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat() );

	debris->GetPhysics()->SetLinearVelocity( linear_velocity );
	debris->GetPhysics()->SetAngularVelocity( angular_velocity );
}

//BC this duplicates the ejectbrass functionality. But instead of brass, ejects a live round moveableItem.
void idWeapon::Event_EjectLiveRound(void)
{
	idMat3 axis;
	idVec3 origin, linear_velocity, angular_velocity;
	idEntity *ent;

	if (ejectJointView == INVALID_JOINT || gameLocal.isClient)
	{
		return;
	}

	if (!GetGlobalJointTransform(true, ejectJointView, origin, axis))
	{
		return;
	}

	gameLocal.SpawnEntityDef(singleAmmoDict, &ent, false);
	if (!ent)
	{
		return;
	}

	ent->SetOrigin(origin);
	ent->SetAxis(axis);

	linear_velocity = idVec3(0, 0, 70 + (gameLocal.random.CRandomFloat() * 30)) + (playerViewAxis[1] * -20) + (playerViewAxis[0] * (40 + (gameLocal.random.CRandomFloat() * 30)));
	angular_velocity.Set(400 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat());

	ent->GetPhysics()->SetLinearVelocity(linear_velocity);
	ent->GetPhysics()->SetAngularVelocity(angular_velocity);
	ent->GetPhysics()->SetGravity(idVec3(0, 0, singleAmmoDict.GetFloat("gravity", "-300")));

	
	StartSound("snd_ejectround", SND_CHANNEL_BODY3, 0, false, NULL);
	//StartSoundShader(declManager->FindSound("ejectliveround"), SND_CHANNEL_BODY3, 0, false, NULL);

	owner->Event_SetDropAmmoMsg(1);
}

/*
===============
idWeapon::Event_IsInvisible
===============
*/
void idWeapon::Event_IsInvisible( void ) {
	if ( !owner ) {
		idThread::ReturnFloat( 0 );
		return;
	}
	idThread::ReturnFloat( owner->PowerUpActive( INVISIBILITY ) ? 1 : 0 );
}

/*
===============
idWeapon::ClientPredictionThink
===============
*/
void idWeapon::ClientPredictionThink( void ) {
	UpdateAnimation();
}






//BC START ==================================================================

idVec3 idWeapon::GetMuzzlePosition(bool viewModel)
{
	if (viewModel)
	{
		idVec3 pos;
		idMat3 axis;

		GetGlobalJointTransform(true, barrelJointView, pos, axis);

		return pos;
	}
	else
	{
		idVec3 spawnPos;
		spawnPos = owner->GetPhysics()->GetOrigin();
		spawnPos.z = worldModel.GetEntity()->GetPhysics()->GetOrigin().z;

		return spawnPos;
	}
}

idVec3 idWeapon::GetJointPosition(jointHandle_t jointName)
{
	idVec3 pos;
	idMat3 axis;
	GetGlobalJointTransform(true, jointName, pos, axis);
	return pos;
}

void idWeapon::RackTheSlide(void)
{
	if (!isLinked)
		return;
	
	if (status == WP_RELOAD)
		return;

	if (WEAPON_INSPECTMAGAZINE) //do not allow rack slide during mag inspect.
		return;

	WEAPON_RACKTHESLIDE = true;
	status = WP_RACKSLIDE;

	if (Event_RoundIsChambered())
	{
		//Racking the slide while a live round is chambered. Eject a live bullet.
		Event_SetChambered(false);
		//PostEventMS(&EV_Weapon_EjectBrass, 300);
		//Event_EjectBrass();

		PostEventMS(&EV_Weapon_EjectLiveRound, spawnArgs.GetInt("eject_delay", "300"));
	}

	//If mag has any rounds left, then chamber the next round.
	if (ammoClip > 0)
	{
		//Consume one ammo from the current magazine.
		ammoClip -= ammoRequired;

		//if (Event_UseAmmo(1))
		//{
			Event_SetChambered(true);
		//}
	}
}

void idWeapon::InspectMagazine(bool active)
{
	if (!isLinked)
		return;

	WEAPON_INSPECTMAGAZINE = active;
}

bool idWeapon::IsInspectingMagazine()
{
	return isLinked ? WEAPON_INSPECTMAGAZINE : false;
}

void idWeapon::InspectChamber(bool active)
{
	if (!isLinked)
		return;

	WEAPON_INSPECTCHAMBER = active;
}

bool idWeapon::IsInspectingChamber()
{
	return isLinked ? WEAPON_INSPECTCHAMBER : false;
}

int idWeapon::Event_RoundIsChambered(void)
{
	bool chambered = owner->inventory.hotbarSlots[owner->currentWeaponSlot].chambered;
	
	idThread::ReturnInt(chambered);
	return chambered;
}

void idWeapon::Event_SetChambered(float value)
{
	if (value > 0)
		owner->inventory.hotbarSlots[owner->currentWeaponSlot].chambered = true;
	else
		owner->inventory.hotbarSlots[owner->currentWeaponSlot].chambered = false;
}

void idWeapon::InspectWeapon(bool active)
{
	if (!isLinked)
		return;

	WEAPON_INSPECTWEAPON = active;
}

void idWeapon::Event_GetJiggleState()
{
	idThread::ReturnInt(isJiggling ? 1 : 0);
}

void idWeapon::Event_StartVent()
{
	nozzleGlow.lightRadius.x = nozzleGlowRadius;
	nozzleGlow.lightRadius.y = nozzleGlowRadius;
	nozzleGlow.lightRadius.z = nozzleGlowRadius;
	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}

void idWeapon::Event_StopVent()
{

	nozzleGlow.lightRadius.x = .1f;
	nozzleGlow.lightRadius.y = .1f;
	nozzleGlow.lightRadius.z = .1f;
	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}

void idWeapon::Event_SetNozzleRadius(float value)
{
	nozzleLerping = true;
	nozzleLerpTimer = gameLocal.time;//transition time.	
	nozzleLerpValueStart = nozzleGlow.lightRadius.x;
	nozzleLerpValueEnd = value;
}

void idWeapon::Event_SetClipAmount(int amount)
{
	int ammoAvail;
	int oldAmmo;

	if (gameLocal.isClient)
	{
		return;
	}

	oldAmmo = ammoClip;
	ammoAvail = owner->inventory.HasAmmo(ammoType, ammoRequired);

	ammoClip = Min(amount, ammoAvail); //load the mag with either the full mag amount, or the remaining rounds remaining in reserves.

	if (ammoClip > clipSize)
	{
		ammoClip = clipSize;
	}

	owner->inventory.UseAmmo(ammoType, ammoClip);
}

bool idWeapon::Event_UnloadMag()
{
	//Yank ammo out of magazine & chamber. Put ammo back into inventory.
	
	int amountToReturn = 0;

	//BC 2-22-2025: fix bug where you magically get more ammo whenever you reload. Remove this logic bit for auto unchambering when unloading mag.
	//Handle chamber.
	//if (Event_RoundIsChambered())
	//{
	//	owner->inventory.UseAmmo(ammoType, -1); //Return the chambered bullet.
	//	Event_SetChambered(false);
	//	amountToReturn += 1;
	//}

	//Handle mag.
	int ammoInClip = AmmoInClip();
	if (ammoInClip > 0)
	{
		//Return ammo from magazine and put it back into player pocket.
		//owner->inventory.UseAmmo(ammoType, -ammoInClip); //BC 2-22-2025: fix bug where ammo was being returned to player pocket twice. Commented out this line.
		ammoClip = 0;
		int weaponIndex = owner->inventory.GetWeaponIndex( owner, spawnArgs.GetString("inv_weapon", "") );
		int weaponSlot = owner->inventory.GetHotbarslotViaWeaponIndex( weaponIndex );
		owner->inventory.hotbarSlots[weaponSlot].clip = 0; //Empty the mag.
		amountToReturn += ammoInClip;
	}
	

	//BC 2-22-2025: fix bug where ammo was being returned to player pocket twice. It now only happens once, in this line.
	bool success = owner->inventory.GiveAmmo(owner, GetAmmoNameForNum(ammoType), idStr::Format("%d", amountToReturn).c_str()); //put ammo back in pocket.
	if (amountToReturn > 0 && success)
	{
		StartSound("snd_unload", SND_CHANNEL_ANY, 0, false, NULL);
		idThread::ReturnInt(1);
		return true;
	}

	StartSound("snd_cancel2", SND_CHANNEL_ANY, 0, false, NULL);
	idThread::ReturnInt(0);
	return false;
}

bool idWeapon::IsReadyToBash(void) const
{
	if (status == WP_READY && !WEAPON_INSPECTMAGAZINE && !WEAPON_RACKTHESLIDE && !WEAPON_INSPECTCHAMBER) //TODO: exit here if player is in Aim Mode
		return true;

	return false;
}

bool idWeapon::IsRacking(void) const {
	return (status == WP_RACKSLIDE);
}

void idWeapon::SetAimMode(bool value)
{
	WEAPON_AIMMODE = value;
}

bool idWeapon::IsAttacking()
{
	return (idStr::Icmp(state, "Fire") == 0);
}