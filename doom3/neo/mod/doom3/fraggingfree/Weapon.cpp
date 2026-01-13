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

#ifdef _DENTONMOD
#include "renderer/ModelManager.h"
#endif

#include "gamesys/SysCvar.h"
#include "ai/AI.h"
#include "Player.h"
#include "Trigger.h"
#include "SmokeParticles.h"
#include "WorldSpawn.h"
#include "Misc.h"

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
//const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "dffff" );
//const idEventDef EV_Weapon_CreateProjectile( "createProjectile", NULL, 'e' );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", "ddffffd" ); //was "dffff"
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", "d", 'e' ); //was NULL, 'e'
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
const idEventDef EV_Weapon_LaunchProjectilesEllipse( "launchProjectilesEllipse", "ddffff" ); //"dffff"
const idEventDef EV_Weapon_LaunchPowerup( "launchPowerup", "sfd" );
const idEventDef EV_Weapon_StartWeaponSmoke( "startWeaponSmoke" );
const idEventDef EV_Weapon_StopWeaponSmoke( "stopWeaponSmoke" );
const idEventDef EV_Weapon_StartWeaponParticle( "startWeaponParticle", "s" );
const idEventDef EV_Weapon_StopWeaponParticle( "stopWeaponParticle", "s" );
const idEventDef EV_Weapon_StartWeaponLight( "startWeaponLight", "s" );
const idEventDef EV_Weapon_StopWeaponLight( "stopWeaponLight", "s" );
#endif

//ff1.1 start
//const idEventDef EV_Weapon_SetPwState( "setPwState", "d" );
//const idEventDef EV_Weapon_GetPwState( "getPwState", NULL, 'd' );
//const idEventDef EV_Weapon_AddAmmo( "addAmmo", "d" );
const idEventDef EV_Weapon_UsePwAmmo( "usePwAmmo", "d" );
const idEventDef EV_Weapon_PwAmmoAvailable( "pwAmmoAvailable", NULL, 'f' );
const idEventDef EV_Weapon_GetPkProj( "getPkProj", NULL, 'e' );
const idEventDef EV_Weapon_GetRemoteGrenadeProj( "getRemoteGrenadeProj", NULL, 'e' );
//ff1.1 end

//ff1.3 start
const idEventDef EV_Weapon_SetSkullMode( "setSkullMode" , "d" );
const idEventDef EV_Weapon_GetSkullMode( "getSkullMode", NULL, 'f' );
const idEventDef EV_Weapon_SkullAmmoAvailable( "skullAmmoAvailable", "d", 'f' );
const idEventDef EV_Weapon_UseSkullAmmo( "useSkullAmmo", "dd" );
const idEventDef EV_Weapon_GetAimTarget( "getAimTarget", NULL, 'e' );
const idEventDef EV_Weapon_GetEnemyTarget( "getEnemyTarget", NULL, 'e' );
const idEventDef EV_Weapon_UpdateLockCursor( "updateLockCursor", "E" );
const idEventDef EV_Weapon_HasAmmo( "hasAmmo", NULL, 'd' );
const idEventDef EV_Weapon_AnyFireButtonPressed( "anyFireButtonPressed", NULL, 'd' );

#ifdef _DENTONMOD
const idEventDef EV_Weapon_SetZoom( "setZoom" , "d");
const idEventDef EV_Weapon_IsAdvancedZooming( "isAdvancedZooming", NULL, 'd' );
/*
const idEventDef EV_Weapon_ChangeProjectileDef( "changeProjectileDef" , "d", 'f' );
const idEventDef EV_Weapon_GetProjectileType( "getProjectileType", NULL, 'f' );
*/
#endif
//ff1.3 end

//
// class def
//
CLASS_DECLARATION( idAnimatedEntity, idWeapon )
	EVENT( EV_Weapon_Clear,						idWeapon::Event_Clear )
	EVENT( EV_Weapon_GetOwner,					idWeapon::Event_GetOwner )
	EVENT( EV_Weapon_State,						idWeapon::Event_WeaponState )
	EVENT( EV_Weapon_WeaponReady,				idWeapon::Event_WeaponReady )
	EVENT( EV_Weapon_WeaponOutOfAmmo,			idWeapon::Event_WeaponOutOfAmmo )
	EVENT( EV_Weapon_WeaponReloading,			idWeapon::Event_WeaponReloading )
	EVENT( EV_Weapon_WeaponHolstered,			idWeapon::Event_WeaponHolstered )
	EVENT( EV_Weapon_WeaponRising,				idWeapon::Event_WeaponRising )
	EVENT( EV_Weapon_WeaponLowering,			idWeapon::Event_WeaponLowering )
	EVENT( EV_Weapon_UseAmmo,					idWeapon::Event_UseAmmo )
	EVENT( EV_Weapon_AddToClip,					idWeapon::Event_AddToClip )
	EVENT( EV_Weapon_AmmoInClip,				idWeapon::Event_AmmoInClip )
	EVENT( EV_Weapon_AmmoAvailable,				idWeapon::Event_AmmoAvailable )
	EVENT( EV_Weapon_TotalAmmoCount,			idWeapon::Event_TotalAmmoCount )
	EVENT( EV_Weapon_ClipSize,					idWeapon::Event_ClipSize )
	EVENT( AI_PlayAnim,							idWeapon::Event_PlayAnim )
	EVENT( AI_PlayCycle,						idWeapon::Event_PlayCycle )
	EVENT( AI_SetBlendFrames,					idWeapon::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,					idWeapon::Event_GetBlendFrames )
	EVENT( AI_AnimDone,							idWeapon::Event_AnimDone )
	EVENT( EV_Weapon_Next,						idWeapon::Event_Next )
	EVENT( EV_SetSkin,							idWeapon::Event_SetSkin )
	EVENT( EV_Weapon_Flashlight,				idWeapon::Event_Flashlight )
	EVENT( EV_Light_GetLightParm,				idWeapon::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,				idWeapon::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,				idWeapon::Event_SetLightParms )
	EVENT( EV_Weapon_LaunchProjectiles,			idWeapon::Event_LaunchProjectiles )
	EVENT( EV_Weapon_CreateProjectile,			idWeapon::Event_CreateProjectile )
	EVENT( EV_Weapon_EjectBrass,				idWeapon::Event_EjectBrass )
	EVENT( EV_Weapon_Melee,						idWeapon::Event_Melee )
	EVENT( EV_Weapon_GetWorldModel,				idWeapon::Event_GetWorldModel )
	EVENT( EV_Weapon_AllowDrop,					idWeapon::Event_AllowDrop )
	EVENT( EV_Weapon_AutoReload,				idWeapon::Event_AutoReload )
	EVENT( EV_Weapon_NetReload,					idWeapon::Event_NetReload )
	EVENT( EV_Weapon_IsInvisible,				idWeapon::Event_IsInvisible )
	EVENT( EV_Weapon_NetEndReload,				idWeapon::Event_NetEndReload )
#ifdef _D3XP
	EVENT( EV_Weapon_Grabber,					idWeapon::Event_Grabber )
	EVENT( EV_Weapon_GrabberHasTarget,			idWeapon::Event_GrabberHasTarget )
	EVENT( EV_Weapon_Grabber_SetGrabDistance,	idWeapon::Event_GrabberSetGrabDistance )
	EVENT( EV_Weapon_LaunchProjectilesEllipse,	idWeapon::Event_LaunchProjectilesEllipse )
	EVENT( EV_Weapon_LaunchPowerup,				idWeapon::Event_LaunchPowerup )
	EVENT( EV_Weapon_StartWeaponSmoke,			idWeapon::Event_StartWeaponSmoke )
	EVENT( EV_Weapon_StopWeaponSmoke,			idWeapon::Event_StopWeaponSmoke )
	EVENT( EV_Weapon_StartWeaponParticle,		idWeapon::Event_StartWeaponParticle )
	EVENT( EV_Weapon_StopWeaponParticle,		idWeapon::Event_StopWeaponParticle )
	EVENT( EV_Weapon_StartWeaponLight,			idWeapon::Event_StartWeaponLight )
	EVENT( EV_Weapon_StopWeaponLight,			idWeapon::Event_StopWeaponLight )
#endif

	//ff1.1 start
	//EVENT( EV_Weapon_SetPwState,				idWeapon::Event_SetPwState )
	//EVENT( EV_Weapon_GetPwState,				idWeapon::Event_GetPwState )
	//EVENT( EV_Weapon_AddAmmo,					idWeapon::Event_AddAmmo )
	EVENT( EV_Weapon_UsePwAmmo,					idWeapon::Event_UsePwAmmo )
	EVENT( EV_Weapon_PwAmmoAvailable,			idWeapon::Event_PwAmmoAvailable )
	EVENT( EV_Weapon_GetPkProj,					idWeapon::Event_GetPainKillerProjectile)
	EVENT( EV_Weapon_GetRemoteGrenadeProj,		idWeapon::Event_GetRemoteGrenadeProjectile)
	//ff1.1 end

	//ff1.3 start
	EVENT( EV_Weapon_SetSkullMode,				idWeapon::Event_SetSkullMode )
	EVENT( EV_Weapon_GetSkullMode,				idWeapon::Event_GetSkullMode )
	EVENT( EV_Weapon_SkullAmmoAvailable,		idWeapon::Event_SkullAmmoAvailable )
	EVENT( EV_Weapon_UseSkullAmmo,				idWeapon::Event_UseSkullAmmo )
	EVENT( EV_Weapon_GetAimTarget,				idWeapon::Event_GetAimTarget )
	EVENT( EV_Weapon_GetEnemyTarget,			idWeapon::Event_GetEnemyTarget )
	EVENT( EV_Weapon_UpdateLockCursor,			idWeapon::Event_UpdateLockCursor )
	EVENT( EV_Weapon_HasAmmo,					idWeapon::Event_HasAmmo )
	EVENT( EV_Weapon_AnyFireButtonPressed,		idWeapon::Event_AnyFireButtonPressed )



#ifdef _DENTONMOD
	EVENT( EV_Weapon_SetZoom,					idWeapon::Event_SetZoom )
	EVENT( EV_Weapon_IsAdvancedZooming,			idWeapon::Event_IsAdvancedZooming )
	/*
	EVENT( EV_Weapon_ChangeProjectileDef,		idWeapon::Event_ChangeProjectileDef )
	EVENT( EV_Weapon_GetProjectileType,			idWeapon::Event_GetProjectileType )
	*/
#endif
	//ff1.3 end

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

	//berserk					= 2;
	brassDelay				= 0;

	allowDrop				= true;

	Clear();

	fl.networkSync = true;

#ifdef _SHELLWEAPON
	//pw fix
	itemShellHandle			= -1;
	lastRenderViewTime		= -1;
#endif

	weaponIndex				= -1; //ff1.3
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() {
	Clear();
	delete worldModel.GetEntity();

#ifdef _SHELLWEAPON
	// remove the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
	}
#endif
}


/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn( void ) {
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

#ifdef _SHELLWEAPON
	//pw fix
	itemShellHandle = -1;
	shellMaterial = declManager->FindMaterial( "weaponHighlightShell" );
#endif
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

	weaponDef = gameLocal.FindEntityDef( weaponName, false );
	if ( !weaponDef ) {
		return;
	}

	// precache the brass collision model
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );
	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( brassDef ) {
			brassDef->dict.GetString( "clipmodel", "", &clipModelName );
			if ( !clipModelName[0] ) {
				clipModelName = brassDef->dict.GetString( "model" );		// use the visual model
			}
			// load the trace model
			collisionModelManager->TrmFromModel( clipModelName, trm );
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

	savefile->WriteInt( status );
	savefile->WriteObject( thread );
	savefile->WriteString( state );
	savefile->WriteString( idealState );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( animDoneTime );
	savefile->WriteBool( isLinked );

	savefile->WriteObject( owner );
	worldModel.Save( savefile );

	savefile->WriteInt( hideTime );
	savefile->WriteFloat( hideDistance );
	savefile->WriteInt( hideStartTime );
	savefile->WriteFloat( hideStart );
	savefile->WriteFloat( hideEnd );
	savefile->WriteFloat( hideOffset );
	savefile->WriteBool( hide );
	savefile->WriteBool( disabled );

	//savefile->WriteInt( berserk );

	savefile->WriteVec3( playerViewOrigin );
	savefile->WriteMat3( playerViewAxis );

	savefile->WriteVec3( viewWeaponOrigin );
	savefile->WriteMat3( viewWeaponAxis );

	savefile->WriteVec3( muzzleOrigin );
	savefile->WriteMat3( muzzleAxis );

	savefile->WriteVec3( pushVelocity );

	//savefile->WriteBounds( meleeBounds ); //ff1.3
	savefile->WriteString( weaponDef->GetName() );
	savefile->WriteFloat( meleeDistance );
	savefile->WriteString( meleeDefName );
	savefile->WriteInt( brassDelay );
	savefile->WriteString( icon );
	savefile->WriteString( iconPw ); //ff1.3

	savefile->WriteInt( guiLightHandle );
	savefile->WriteRenderLight( guiLight );

	savefile->WriteInt( muzzleFlashHandle );
	savefile->WriteRenderLight( muzzleFlash );

	savefile->WriteInt( worldMuzzleFlashHandle );
	savefile->WriteRenderLight( worldMuzzleFlash );

	savefile->WriteVec3( flashColor );
	savefile->WriteInt( muzzleFlashEnd );
	savefile->WriteInt( flashTime );

	savefile->WriteBool( lightOn );
	savefile->WriteBool( silent_fire );

	savefile->WriteInt( kick_endtime );
	savefile->WriteInt( muzzle_kick_time );
	savefile->WriteInt( muzzle_kick_maxtime );
	savefile->WriteAngles( muzzle_kick_angles );
	savefile->WriteVec3( muzzle_kick_offset );

	savefile->WriteInt( ammoType );
	savefile->WriteInt( ammoRequired );
	savefile->WriteInt( clipSize );
	savefile->WriteInt( ammoClip );
	savefile->WriteInt( lowAmmo );
	savefile->WriteBool( powerAmmo );

	// savegames <= 17
	savefile->WriteInt( 0 );

	savefile->WriteInt( zoomFov );

	savefile->WriteJoint( barrelJointView );
	savefile->WriteJoint( flashJointView );
	savefile->WriteJoint( ejectJointView );
	savefile->WriteJoint( guiLightJointView );
	savefile->WriteJoint( ventLightJointView );

	savefile->WriteJoint( flashJointWorld );
	savefile->WriteJoint( barrelJointWorld );
	savefile->WriteJoint( ejectJointWorld );

	savefile->WriteBool( hasBloodSplat );

	savefile->WriteSoundShader( sndHum );

	savefile->WriteParticle( weaponSmoke );
	savefile->WriteInt( weaponSmokeStartTime );
	savefile->WriteBool( continuousSmoke );
	savefile->WriteParticle( strikeSmoke );
	savefile->WriteInt( strikeSmokeStartTime );
	savefile->WriteVec3( strikePos );
	savefile->WriteMat3( strikeAxis );
	savefile->WriteInt( nextStrikeFx );

	savefile->WriteBool( nozzleFx );
	savefile->WriteInt( nozzleFxFade );

	savefile->WriteInt( lastAttack );

	savefile->WriteInt( nozzleGlowHandle );
	savefile->WriteRenderLight( nozzleGlow );

	savefile->WriteVec3( nozzleGlowColor );
	savefile->WriteMaterial( nozzleGlowShader );
	savefile->WriteFloat( nozzleGlowRadius );

	savefile->WriteInt( weaponAngleOffsetAverages );
	savefile->WriteFloat( weaponAngleOffsetScale );
	savefile->WriteFloat( weaponAngleOffsetMax );
	savefile->WriteFloat( weaponOffsetTime );
	savefile->WriteFloat( weaponOffsetScale );

	savefile->WriteBool( allowDrop );
	savefile->WriteObject( projectileEnt );

#ifdef _DENTONMOD

	savefile->WriteInt(weaponParticles.Num());

	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);

		particleFlags_s flags = part->particleFlags;

		LittleBitField( &flags, sizeof( flags ) );
		savefile->Write( &flags, sizeof( flags ) );

		savefile->WriteString( part->name );
		savefile->WriteString( part->particlename );
		savefile->WriteInt( part->startTime );
		savefile->WriteJoint( part->joint );

		if( !part->particleFlags.isSmoke ) {
			savefile->WriteRenderEntity( part->renderEntity );
		}
		if( part->particleFlags.isDir ) {
			savefile->WriteVec3(part->dir);
		}
		if( part->particleFlags.isOffset ) {
			savefile->WriteVec3(part->offset);
		}
	}
	savefile->WriteInt(weaponLights.Num());
	for(int i = 0; i < weaponLights.Num(); i++) {

		WeaponLight_t* light = weaponLights.GetIndex(i);

		lightFlags_s flags = light->lightFlags;

		LittleBitField( &flags, sizeof( flags ) );
		savefile->Write( &flags, sizeof( flags ) );

		savefile->WriteString( light->name );
		savefile->WriteInt( light->startTime );
		savefile->WriteJoint( light->joint );
		savefile->WriteInt( light->lightHandle );
		savefile->WriteRenderLight( light->light );

		if( !light->lightFlags.isAlwaysOn ) {
			savefile->WriteInt( light->endTime );
		}
		if( light->lightFlags.isDir ) {
			savefile->WriteVec3(light->dir);
		}
		if( light->lightFlags.isOffset ) {
			savefile->WriteVec3(light->offset);
		}
	}
#endif // _DENTONMOD

#ifdef _D3XP
	savefile->WriteStaticObject( grabber );
	savefile->WriteInt( grabberState );

	savefile->WriteJoint ( smokeJointView );

	/*
	//OLD D3XP PARTICLES CODE
	savefile->WriteInt(weaponParticles.Num());
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		savefile->WriteString( part->name );
		savefile->WriteString( part->particlename );
		savefile->WriteBool( part->active );
		//savefile->WriteBool( part->activateOnShow ); //ff1.1
		savefile->WriteInt( part->startTime );
		savefile->WriteJoint( part->joint );
		savefile->WriteBool( part->isViewDir );
		savefile->WriteBool( part->isDirFix );
		savefile->WriteBool( part->smoke );
		if(!part->smoke) {
			savefile->WriteObject(part->emitter);
		}
	}
	savefile->WriteInt(weaponLights.Num());
	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		savefile->WriteString( light->name );
		savefile->WriteBool( light->active );
		//savefile->WriteBool( light->activateOnShow ); //ff1.1
		savefile->WriteInt( light->startTime );
		savefile->WriteJoint( light->joint );
		savefile->WriteInt( light->lightHandle );
		savefile->WriteRenderLight( light->light );
	}
	*/
#endif

#ifdef _SHELLWEAPON
	//pw fix
	savefile->WriteInt( lastRenderViewTime );
#endif

	savefile->WriteInt(weaponIndex); //ff1.3
}

/*
================
idWeapon::Restore
================
*/
void idWeapon::Restore( idRestoreGame *savefile ) {
	const char *brassDefName;

	savefile->ReadInt( (int &)status );
	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( animDoneTime );
	savefile->ReadBool( isLinked );

	// Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_SPECIAL.LinkTo(		scriptObject, "WEAPON_SPECIAL" );		// New
	WEAPON_SPECIAL_HOLD.LinkTo(	scriptObject, "WEAPON_SPECIAL_HOLD" );	//
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	worldModel.Restore( savefile );

	savefile->ReadInt( hideTime );
	savefile->ReadFloat( hideDistance );
	savefile->ReadInt( hideStartTime );
	savefile->ReadFloat( hideStart );
	savefile->ReadFloat( hideEnd );
	savefile->ReadFloat( hideOffset );
	savefile->ReadBool( hide );
	savefile->ReadBool( disabled );

	//savefile->ReadInt( berserk );

	savefile->ReadVec3( playerViewOrigin );
	savefile->ReadMat3( playerViewAxis );

	savefile->ReadVec3( viewWeaponOrigin );
	savefile->ReadMat3( viewWeaponAxis );

	savefile->ReadVec3( muzzleOrigin );
	savefile->ReadMat3( muzzleAxis );

	savefile->ReadVec3( pushVelocity );

	idStr objectname;
	savefile->ReadString( objectname );
	weaponDef = gameLocal.FindEntityDef( objectname );
	meleeDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_melee" ), false );

	//ff1.3 start
#ifndef _DENTONMOD
	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_projectile" ), false );
	if ( projectileDef ) {
		projectileDic = projectileDef->dict;
	} else {
		projectileDict.Clear();
	}
#endif

	brassDict.Clear();
	brassDefName = weaponDef->dict.GetString( "def_ejectBrass" );
	if ( brassDefName[0] ) {
		const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( brassDefName, false );
		if ( brassDef ) {
			brassDict = brassDef->dict;
		}
	}
	/*
	const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_ejectBrass" ), false );
	if ( brassDef ) {
		brassDict = brassDef->dict;
	} else {
		brassDict.Clear();
	}
	*/
	//ff1.3 end

	//savefile->ReadBounds( meleeBounds ); //ff1.3
	savefile->ReadFloat( meleeDistance );
	savefile->ReadString( meleeDefName );
	savefile->ReadInt( brassDelay );
	savefile->ReadString( icon );
	savefile->ReadString( iconPw ); //ff1.3

	savefile->ReadInt( guiLightHandle );
	savefile->ReadRenderLight( guiLight );
#ifdef _D3XP
	if ( guiLightHandle >= 0 ) {
		guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
	}
#endif

	savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadRenderLight( muzzleFlash );
#ifdef _D3XP
	if ( muzzleFlashHandle >= 0 ) {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}
#endif

	savefile->ReadInt( worldMuzzleFlashHandle );
	savefile->ReadRenderLight( worldMuzzleFlash );
#ifdef _D3XP
	if ( worldMuzzleFlashHandle >= 0 ) {
		worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &worldMuzzleFlash );
	}
#endif

	savefile->ReadVec3( flashColor );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( flashTime );

	savefile->ReadBool( lightOn );
	savefile->ReadBool( silent_fire );

	savefile->ReadInt( kick_endtime );
	savefile->ReadInt( muzzle_kick_time );
	savefile->ReadInt( muzzle_kick_maxtime );
	savefile->ReadAngles( muzzle_kick_angles );
	savefile->ReadVec3( muzzle_kick_offset );

	savefile->ReadInt( (int &)ammoType );
	savefile->ReadInt( ammoRequired );
	savefile->ReadInt( clipSize );
	savefile->ReadInt( ammoClip );
	savefile->ReadInt( lowAmmo );
	savefile->ReadBool( powerAmmo );

	// savegame versions <= 17
	int foo;
	savefile->ReadInt( foo );

	savefile->ReadInt( zoomFov );

	savefile->ReadJoint( barrelJointView );
	savefile->ReadJoint( flashJointView );
	savefile->ReadJoint( ejectJointView );
	savefile->ReadJoint( guiLightJointView );
	savefile->ReadJoint( ventLightJointView );

	savefile->ReadJoint( flashJointWorld );
	savefile->ReadJoint( barrelJointWorld );
	savefile->ReadJoint( ejectJointWorld );

	savefile->ReadBool( hasBloodSplat );

	savefile->ReadSoundShader( sndHum );

	savefile->ReadParticle( weaponSmoke );
	savefile->ReadInt( weaponSmokeStartTime );
	savefile->ReadBool( continuousSmoke );
	savefile->ReadParticle( strikeSmoke );
	savefile->ReadInt( strikeSmokeStartTime );
	savefile->ReadVec3( strikePos );
	savefile->ReadMat3( strikeAxis );
	savefile->ReadInt( nextStrikeFx );

	savefile->ReadBool( nozzleFx );
	savefile->ReadInt( nozzleFxFade );

	savefile->ReadInt( lastAttack );

	savefile->ReadInt( nozzleGlowHandle );
	savefile->ReadRenderLight( nozzleGlow );
#ifdef _D3XP
	if ( nozzleGlowHandle >= 0 ) {
		nozzleGlowHandle = gameRenderWorld->AddLightDef( &nozzleGlow );
	}
#endif

	savefile->ReadVec3( nozzleGlowColor );
	savefile->ReadMaterial( nozzleGlowShader );
	savefile->ReadFloat( nozzleGlowRadius );

	savefile->ReadInt( weaponAngleOffsetAverages );
	savefile->ReadFloat( weaponAngleOffsetScale );
	savefile->ReadFloat( weaponAngleOffsetMax );
	savefile->ReadFloat( weaponOffsetTime );
	savefile->ReadFloat( weaponOffsetScale );

	savefile->ReadBool( allowDrop );
	savefile->ReadObject( reinterpret_cast<idClass *&>( projectileEnt ) );

	//ff1.3 start
#ifdef _DENTONMOD
	if ( !ChangeProjectileDef( owner->GetProjectileType() ) ) { // if restore fails we must clear the dict
		projectileDict.Clear();
	}

	//particles
	int particleCount;
	savefile->ReadInt( particleCount );
	for(int i = 0; i < particleCount; i++) {
		WeaponParticle_t newParticle;
		memset(&newParticle, 0, sizeof(newParticle));

		savefile->Read( &newParticle.particleFlags, sizeof( newParticle.particleFlags ) );
		LittleBitField( &newParticle.particleFlags, sizeof( newParticle.particleFlags ) );

		idStr prtName, particlename;
		savefile->ReadString( prtName );
		savefile->ReadString( particlename );

		strcpy( newParticle.name, prtName.c_str() );
		strcpy( newParticle.particlename, particlename.c_str() );

		savefile->ReadInt( newParticle.startTime );
		savefile->ReadJoint( newParticle.joint );

		if (newParticle.particleFlags.isSmoke) {
			newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particlename, false ) );
		}
		else {
			savefile->ReadRenderEntity( newParticle.renderEntity );
			newParticle.modelDefHandle = -1;
		}
		if ( newParticle.particleFlags.isDir ) {
			savefile->ReadVec3( newParticle.dir );
		}
		if ( newParticle.particleFlags.isOffset ) {
			savefile->ReadVec3( newParticle.offset );
		}

		weaponParticles.Set(newParticle.name, newParticle);
	}

	int lightCount;
	savefile->ReadInt( lightCount );
	for(int i = 0; i < lightCount; i++) {
		WeaponLight_t newLight;
		memset(&newLight, 0, sizeof(newLight));

		savefile->Read( &newLight.lightFlags, sizeof( newLight.lightFlags ) );
		LittleBitField( &newLight.lightFlags, sizeof( newLight.lightFlags ) );

		idStr lightName;
		savefile->ReadString( lightName );
		strcpy( newLight.name, lightName.c_str() );

		savefile->ReadInt( newLight.startTime );
		savefile->ReadJoint( newLight.joint );
		savefile->ReadInt( newLight.lightHandle );
		savefile->ReadRenderLight( newLight.light );
		if ( newLight.lightHandle >= 0 ) {
			newLight.lightHandle = gameRenderWorld->AddLightDef( &newLight.light );
		}

		if ( !newLight.lightFlags.isAlwaysOn ) {
			savefile->ReadInt( newLight.endTime );
		}
		if ( newLight.lightFlags.isDir ) {
			savefile->ReadVec3(newLight.dir);
		}
		if ( newLight.lightFlags.isOffset ) {
			savefile->ReadVec3(newLight.offset);
		}
		weaponLights.Set(newLight.name, newLight);
	}
#endif //_DENTONMOD
	//ff1.3 end

#ifdef _D3XP
	savefile->ReadStaticObject( grabber );
	savefile->ReadInt( grabberState );

	savefile->ReadJoint ( smokeJointView );

	/*
	//OLD D3XP PARTICLES CODE
	int particleCount;
	savefile->ReadInt( particleCount );
	for(int i = 0; i < particleCount; i++) {
		WeaponParticle_t newParticle;
		memset(&newParticle, 0, sizeof(newParticle));

		idStr name, particlename;
		savefile->ReadString( name );
		savefile->ReadString( particlename );

		strcpy( newParticle.name, name.c_str() );
		strcpy( newParticle.particlename, particlename.c_str() );

		savefile->ReadBool( newParticle.active );
		//savefile->ReadBool( newParticle.activateOnShow ); //ff1.1
		savefile->ReadInt( newParticle.startTime );
		savefile->ReadJoint( newParticle.joint );
		savefile->ReadBool( newParticle.isViewDir );
		savefile->ReadBool( newParticle.isDirFix );
		savefile->ReadBool( newParticle.smoke );
		if(newParticle.smoke) {
			newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particlename, false ) );
		} else {
			savefile->ReadObject(reinterpret_cast<idClass *&>(newParticle.emitter));
		}

		weaponParticles.Set(newParticle.name, newParticle);
	}

	int lightCount;
	savefile->ReadInt( lightCount );
	for(int i = 0; i < lightCount; i++) {
		WeaponLight_t newLight;
		memset(&newLight, 0, sizeof(newLight));

		idStr name;
		savefile->ReadString( name );
		strcpy( newLight.name, name.c_str() );

		savefile->ReadBool( newLight.active );
		//savefile->ReadBool( newLight.activateOnShow ); //ff1.1
		savefile->ReadInt( newLight.startTime );
		savefile->ReadJoint( newLight.joint );
		savefile->ReadInt( newLight.lightHandle );
		savefile->ReadRenderLight( newLight.light );
		if ( newLight.lightHandle >= 0 ) {
			newLight.lightHandle = gameRenderWorld->AddLightDef( &newLight.light );
		}
		weaponLights.Set(newLight.name, newLight);
	}
	*/
#endif

#ifdef _SHELLWEAPON
	//pw fix
	savefile->ReadInt( lastRenderViewTime );
	itemShellHandle = -1;
#endif

	savefile->ReadInt(weaponIndex); //ff1.3
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
	WEAPON_SPECIAL.Unlink();			//new
	WEAPON_SPECIAL_HOLD.Unlink();		//new
	WEAPON_RELOAD.Unlink();
	WEAPON_NETRELOAD.Unlink();
	WEAPON_NETENDRELOAD.Unlink();
	WEAPON_NETFIRING.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();

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
	iconPw			= ""; //ff1.3

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

	//meleeBounds.Zero(); //ff1.3
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

	/*
	//OLD D3XP PARTICLES CODE
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
	*/
#endif

#ifdef _DENTONMOD

	//Clean up the weapon particles
	for(int i = 0; i < weaponParticles.Num(); i++) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);
		if(!part->particleFlags.isSmoke) {
			if ( part->modelDefHandle >= 0 )
				gameRenderWorld->FreeEntityDef( part->modelDefHandle );
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
	//------------------------------------ New End
#endif //_DENTONMOD

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

	flashJointWorld = ent->GetAnimator()->GetJointHandle( weaponDef->dict.GetString( "flash_joint", "flash" ) ); //was flash
	barrelJointWorld = ent->GetAnimator()->GetJointHandle( "muzzle" );
	ejectJointWorld = ent->GetAnimator()->GetJointHandle( "eject" );
}

/*
================
idWeapon::GetWeaponDef
================
*/
void idWeapon::GetWeaponDef( const char *objectname, int ammoinclip ) {
	const char *shader;
	const char *objectType;
	const char *vmodel;
	const char *guiName;
	const char *brassDefName;
	const char *smokeName;
	int			ammoAvail;

	Clear();

	if ( !objectname || !objectname[ 0 ] ) {
		return;
	}

	assert( owner );

	weaponDef			= gameLocal.FindEntityDef( objectname );

	weaponIndex			= owner->SlotForWeapon( objectname ); //ff1.3

	ammoType			= GetAmmoNumForName( weaponDef->dict.GetString( "ammoType" ) );
	ammoRequired		= weaponDef->dict.GetInt( "ammoRequired" );
	clipSize			= weaponDef->dict.GetInt( "clipSize" );
	lowAmmo				= weaponDef->dict.GetInt( "lowAmmo" );

	icon				= weaponDef->dict.GetString( "icon" );
	iconPw				= weaponDef->dict.GetString( "iconPw" ); //ff1.3
	silent_fire			= weaponDef->dict.GetBool( "silent_fire" );
	powerAmmo			= weaponDef->dict.GetBool( "powerAmmo" );

	hideTime			= SEC2MS( weaponDef->dict.GetFloat( "hide_time", "0.3" ) );
	hideDistance		= weaponDef->dict.GetFloat( "hide_distance", "-15" );

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
	if ( *guiLightShader != '\0' ) {
		guiLight.shader = declManager->FindMaterial( guiLightShader, false );
		guiLight.lightRadius[0] = guiLight.lightRadius[1] = guiLight.lightRadius[2] = 3;
		guiLight.pointLight = true;
		guiLight.noShadows = true; //ff1.3
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
	barrelJointView = animator.GetJointHandle( weaponDef->dict.GetString( "barrel_joint", "barrel" ) ); //ff1.3 - was: "barrel"
	flashJointView = animator.GetJointHandle( weaponDef->dict.GetString( "flash_joint", "flash" ) ); //ff1.3 - was: "flash"
	ejectJointView = animator.GetJointHandle( "eject" );
	guiLightJointView = animator.GetJointHandle( "guiLight" );
	ventLightJointView = animator.GetJointHandle( "ventLight" );

#ifdef _D3XP
	idStr smokeJoint = weaponDef->dict.GetString("smoke_joint");
	if(smokeJoint.Length() > 0) {
		smokeJointView = animator.GetJointHandle( smokeJoint );
	} else {
		smokeJointView = INVALID_JOINT;
	}
#endif

	// get the projectile

	//ff1.3 start
#ifndef _DENTONMOD
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
#endif
	//ff1.3 end

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

	muzzleFlash.noShadows								= true; //ff1.3
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

	ammoClip = ammoinclip;
	if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) ) {
		// first time using this weapon so have it fully loaded to start
		ammoClip = clipSize;
		ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( ammoClip > ammoAvail ) {
			ammoClip = ammoAvail;
		}
#ifdef _D3XP
		//In D3XP we use ammo as soon as it is moved into the clip. This allows for weapons that share ammo
		owner->inventory.UseAmmo(ammoType, ammoClip);
		owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1
#endif
	}

	renderEntity.gui[ 0 ] = NULL;
	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		renderEntity.gui[ 0 ] = uiManager->FindGui( guiName, true, false, true );
	}

	zoomFov = weaponDef->dict.GetInt( "zoomFov", "70" );
	//berserk = weaponDef->dict.GetInt( "berserk", "2" );

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
	WEAPON_SPECIAL.LinkTo(		scriptObject, "WEAPON_SPECIAL" );		//new
	WEAPON_SPECIAL_HOLD.LinkTo(	scriptObject, "WEAPON_SPECIAL_HOLD" );	//new
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );

	spawnArgs = weaponDef->dict;

#ifdef _DENTONMOD
	projectileDict.Clear();
	ChangeProjectileDef( owner->GetProjectileType() );
#endif

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
	DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "0" ) );
	if ( ent ) {
		ent->DetermineTimeGroup( weaponDef->dict.GetBool( "slowmo", "0" ) );
	}

	/*
	//OLD D3XP PARTICLES CODE

	//Initialize the particles
	if ( !gameLocal.isMultiplayer ) {

		const idKeyValue *pkv = weaponDef->dict.MatchPrefix( "weapon_particle", NULL );
		while( pkv ) {
			WeaponParticle_t newParticle;
			memset( &newParticle, 0, sizeof( newParticle ) );

			idStr name = pkv->GetValue();

			strcpy(newParticle.name, name.c_str());

			idStr jointName = weaponDef->dict.GetString(va("%s_joint", name.c_str()));
			newParticle.joint = animator.GetJointHandle(jointName.c_str());
			newParticle.smoke = weaponDef->dict.GetBool(va("%s_smoke", name.c_str()));
			newParticle.isViewDir = weaponDef->dict.GetBool(va("%s_isViewDir", name.c_str()));
			newParticle.isDirFix = weaponDef->dict.GetBool(va("%s_isDirFix", name.c_str()));
			newParticle.active = false;
			//newParticle.activateOnShow = false; //ff1.1
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
			//newLight.activateOnShow = false; //ff1.1
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
	*/
#endif

#ifdef _DENTONMOD
	InitWeaponFx();
#endif
}

#ifdef _DENTONMOD

/*
================
idWeapon::InitWeaponFx
================
*/
void idWeapon::InitWeaponFx( void )
{
	const idKeyValue *pkv;

	//Initialize the particles

	pkv = weaponDef->dict.MatchPrefix( "weapon_particle", NULL );
	while( pkv ) {
		WeaponParticle_t newParticle;
		memset( &newParticle, 0, sizeof( newParticle ) );

		idStr prtName = pkv->GetValue();

		strcpy(newParticle.name, prtName.c_str());

		newParticle.particleFlags.isOffset = weaponDef->dict.GetVector(  va( "%s_offset", prtName.c_str() ), "0 0 0", newParticle.offset ); // this offset will be added to the origin
		newParticle.particleFlags.isDir = weaponDef->dict.GetVector(  va( "%s_dir", prtName.c_str() ), "1 0 0", newParticle.dir ); // adjust the dir
		newParticle.particleFlags.isViewDir = weaponDef->dict.GetBool(  va( "%s_isViewDir", prtName.c_str() ), "0"); //ivan
		newParticle.particleFlags.isOnWorldModel = weaponDef->dict.GetBool(  va( "%s_onWorldModel", prtName.c_str() ), "0");

		idStr jointName = weaponDef->dict.GetString(va("%s_joint", prtName.c_str()));

		if ( !newParticle.particleFlags.isOnWorldModel ) {	// Smoke is not differentiated as world and view particles
			newParticle.particleFlags.isSmoke = weaponDef->dict.GetBool(va("%s_smoke", prtName.c_str()), "0");
			newParticle.joint = animator.GetJointHandle( jointName.c_str() );
		}
		else {
			idEntity *ent = worldModel.GetEntity();
			newParticle.particleFlags.isSmoke = false;
			newParticle.joint = ent->GetAnimator()->GetJointHandle( jointName.c_str() );
		}

		/*
		if( newParticle.joint == INVALID_JOINT ){
			gameLocal.Printf( "joint %s for %s is INVALID\n", jointName.c_str(), prtName.c_str());
		}else{
			gameLocal.Printf( "joint %s for %s is OK\n", jointName.c_str(), prtName.c_str());
		}
		*/

		newParticle.particleFlags.isActive = false;
		newParticle.startTime = 0;

		idStr particle = weaponDef->dict.GetString(va("%s_particle", prtName.c_str()));
		strcpy(newParticle.particlename, particle.c_str());
		newParticle.particleFlags.isContinuous = weaponDef->dict.GetBool(va("%s_continuous", prtName.c_str()), "1");

		if(newParticle.particleFlags.isSmoke) {
			//				newParticle.particleFlags.isContinuous = weaponDef->dict.GetBool(va("%s_continuous", prtName.c_str()), "1");
			newParticle.particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particle.c_str(), false ) );
		}
		else {

			newParticle.renderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0;
			newParticle.renderEntity.shaderParms[ SHADERPARM_GREEN ]	= 1.0;
			newParticle.renderEntity.shaderParms[ SHADERPARM_BLUE ]		= 1.0;
			newParticle.renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= 1.0;

			if ( newParticle.particleFlags.isOnWorldModel ) {
				newParticle.renderEntity.suppressSurfaceInViewID = owner->entityNumber+1; // Make sure this is third person effect only.
			}
			else {
				newParticle.renderEntity.weaponDepthHack = weaponDef->dict.GetBool(va("%s_weaponDepthHack", prtName.c_str()), "1"); //0
				newParticle.renderEntity.allowSurfaceInViewID = owner->entityNumber+1; // Make sure this is first person effect only.
			}

			const idDeclModelDef *modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, newParticle.particlename ) );
			if ( modelDef ) {
				newParticle.renderEntity.hModel = renderModelManager->FindModel( newParticle.particlename );
			}
			newParticle.renderEntity.timeGroup = timeGroup; //ff1.3
			newParticle.modelDefHandle = -1;
		}
		weaponParticles.Set(prtName.c_str(), newParticle);

		pkv = weaponDef->dict.MatchPrefix( "weapon_particle", pkv );
	}

	const idKeyValue *lkv = weaponDef->dict.MatchPrefix( "weapon_light", NULL );
	while( lkv ) {
		WeaponLight_t newLight;
		memset( &newLight, 0, sizeof( newLight ) );

		newLight.lightHandle = -1;
		newLight.lightFlags.isActive = false;
		newLight.startTime = 0;

		idStr lightName = lkv->GetValue(), debug;
		strcpy(newLight.name, lightName.c_str());

		newLight.lightFlags.isOffset = weaponDef->dict.GetVector(  va( "%s_offset", lightName.c_str() ), "0 0 0", newLight.offset ); // this offset will be added to the origin
		newLight.lightFlags.isDir = weaponDef->dict.GetVector(  va( "%s_dir", lightName.c_str() ), "1 0 0", newLight.dir ); // Direction can be adjusted with this

		idStr lightShader = weaponDef->dict.GetString(va("%s_shader", lightName.c_str()));
		newLight.light.shader = declManager->FindMaterial( lightShader.c_str(), false );

		//weaponDef->dict.GetVector( va("%s_radius", lightName.c_str()), "300 300 300", newLight.light.lightRadius );
		float radius = weaponDef->dict.GetFloat(va("%s_radius", lightName.c_str()));
		newLight.light.lightRadius[0] = radius;
		newLight.light.lightRadius[1] = radius;
		newLight.light.lightRadius[2] = radius;

		newLight.lightFlags.isAlwaysOn = weaponDef->dict.GetBool( va( "%s_alwaysOn", lightName.c_str () ), "1" );
		newLight.light.pointLight = weaponDef->dict.GetBool( va( "%s_pointLight", lightName.c_str() ), "1" ); //"0"
		newLight.light.noShadows = weaponDef->dict.GetBool( va( "%s_noShadows", lightName.c_str() ), "1" ); //"0"

		if (newLight.light.pointLight) {
			newLight.light.lightCenter = weaponDef->dict.GetVector( va( "%s_center", lightName.c_str() ) );
		}else{
			newLight.light.target	= weaponDef->dict.GetVector( va( "%s_target", lightName.c_str() ) );
			newLight.light.up		= weaponDef->dict.GetVector( va( "%s_up", lightName.c_str() ) );
			newLight.light.right	= weaponDef->dict.GetVector( va( "%s_right", lightName.c_str() ) );
			newLight.light.end		= newLight.light.target;
		}

		if (!newLight.lightFlags.isAlwaysOn) {
			newLight.endTime = SEC2MS( weaponDef->dict.GetFloat( va( "%s_endTime", lightName.c_str() ), "0.25" ) );
		}

		idVec3 lightColor = weaponDef->dict.GetVector( va( "%s_color", lightName.c_str() ), "1 1 1");

		newLight.light.shaderParms[ SHADERPARM_RED ]		= lightColor[0];
		newLight.light.shaderParms[ SHADERPARM_GREEN ]		= lightColor[1];
		newLight.light.shaderParms[ SHADERPARM_BLUE ]		= lightColor[2];
		newLight.light.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;

		newLight.lightFlags.isOnWorldModel = weaponDef->dict.GetBool(  va( "%s_onWorldModel", lightName.c_str() ), "0");

		idStr jointName = weaponDef->dict.GetString(va("%s_joint", lightName.c_str()));


		if ( newLight.lightFlags.isOnWorldModel ) { // third person only light
			idEntity *ent = worldModel.GetEntity();
			newLight.joint = ent->GetAnimator()->GetJointHandle( jointName.c_str() );
			newLight.light.suppressLightInViewID = owner->entityNumber+1;
		}
		else {
			newLight.joint = animator.GetJointHandle(jointName.c_str());
			newLight.light.allowLightInViewID = owner->entityNumber+1;
		}

		weaponLights.Set(lightName.c_str(), newLight);

		lkv = weaponDef->dict.MatchPrefix( "weapon_light", lkv );
	}
}
#endif //_DENTONMOD

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

	//ff1.3 start
	int pwAmmoamount = PwAmmoAvailable();
	renderEntity.gui[ 0 ]->SetStateString( "player_pwammo", va( "%i", pwAmmoamount) );
	//ff1.3 end

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

/********************************************************************
idWeapon::UpdateWeaponFx( void )

Updates custom weapon particles and lights- By Clone JC Denton
********************************************************************/

#ifdef _DENTONMOD

void idWeapon::UpdateWeaponFx( void )
{
	for( int i = 0; i < weaponParticles.Num(); i++ ) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);

		if(part->particleFlags.isActive) {
			if(part->particleFlags.isSmoke) {
				//get default pos
				if(part->joint != INVALID_JOINT) {
					GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );
				} else {
					// default to going straight out the view
					muzzleOrigin = playerViewOrigin;
					muzzleAxis = playerViewAxis;
				}
				if( part->particleFlags.isViewDir ){ //use view dir
					muzzleAxis = viewWeaponAxis;
				}

				//apply offset
				if ( part->particleFlags.isOffset ) {
					muzzleOrigin += muzzleAxis * part->offset;
				}

				//apply rotation
				if ( part->particleFlags.isDir ) {
					idVec3 &dir = muzzleAxis[ 0 ];

					dir = muzzleAxis * part->dir;
					dir.Normalize();

					muzzleAxis = dir.ToMat3 ();
				} else {
					//direction fix! - the "up" of the particle means "forward"
					idVec3 tmp =  muzzleAxis[2];
					muzzleAxis[2] =  muzzleAxis[0];
					muzzleAxis[0] = -tmp;
				}

				if ( !gameLocal.smokeParticles->EmitSmoke( part->particle, part->startTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup ) ) {
					if ( !part->particleFlags.isContinuous ) {
						part->particleFlags.isActive = false;	// all done
						part->startTime = 0;
					}
					else {
						part->startTime = gameLocal.time; // for continuous effect
					}
				}
			}
			else {

				if( part->renderEntity.hModel ) {
					//get default pos
					if(part->joint != INVALID_JOINT) {
						GetGlobalJointTransform( !part->particleFlags.isOnWorldModel, part->joint, muzzleOrigin, muzzleAxis );
					} else {
						// default to going straight out the view
						muzzleOrigin = playerViewOrigin;
						muzzleAxis = playerViewAxis;
					}

					if( part->particleFlags.isViewDir ){ //use view dir
						muzzleAxis = viewWeaponAxis;
					}

					//apply offset
					if ( part->particleFlags.isOffset ) {
						muzzleOrigin += muzzleAxis * part->offset;
					}

					//apply rotation
					if ( part->particleFlags.isDir ) {
						//gameLocal.Printf("dir %s\n", part->dir.ToString());

						idVec3 &dir = muzzleAxis[ 0 ];

						dir = muzzleAxis * part->dir;
						dir.Normalize();
						muzzleAxis = dir.ToMat3();
					}else{
						//direction fix! - the "up" of the particle means "forward"
						idVec3 tmp =  muzzleAxis[2];
						muzzleAxis[2] = muzzleAxis[0];
						muzzleAxis[0] = -tmp;
					}
					if( g_debugWeapon.GetBool() ){
						gameRenderWorld->DebugArrow( colorYellow, muzzleOrigin, muzzleOrigin+muzzleAxis[ 0 ]*10, 0 );
					}
					/*
					//was:
					GetGlobalJointTransform( !part->particleFlags.isOnWorldModel, part->joint, part->renderEntity.origin, part->renderEntity.axis );

					if ( part->particleFlags.isOffset ) {
						part->renderEntity.origin += part->renderEntity.axis * part->offset;
					}
					if ( part->particleFlags.isDir ) {

						idVec3 &dir = part->renderEntity.axis[ 0 ];

						dir = part->renderEntity.axis * part->dir;
						dir.Normalize();
						part->renderEntity.axis = dir.ToMat3();
					}
					*/

					part->renderEntity.origin = muzzleOrigin;
					part->renderEntity.axis = muzzleAxis;

					if ( part->modelDefHandle != -1 ) {
						gameRenderWorld->UpdateEntityDef( part->modelDefHandle, &part->renderEntity );
					}
					else {
						part->renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
						part->renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.RandomFloat(); // For effects like muzzleflashes etc.
						part->modelDefHandle = gameRenderWorld->AddEntityDef( &part->renderEntity );
					}
				}
			}
		}
	}

	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);

		if(light->lightFlags.isActive) {
			if (GetGlobalJointTransform( !light->lightFlags.isOnWorldModel, light->joint, light->light.origin, light->light.axis )) {
				if ( light->lightFlags.isOffset ) {
					light->light.origin += light->light.axis[ 0 ] * light->offset[ 0 ] + light->light.axis[ 1 ] * light->offset[ 1 ] + light->light.axis[ 2 ] * light->offset[ 2 ];
				}
				if ( light->lightFlags.isDir ) {
					idVec3 &dir = light->light.axis[ 0 ];

					dir = light->light.axis * light->dir;
					dir.Normalize();
					light->light.axis = dir.ToMat3();
				}
				if ( ( light->lightHandle != -1 ) ) {
					gameRenderWorld->UpdateLightDef( light->lightHandle, &light->light );
				}
				else {
					light->light.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
					light->light.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]; // For muzzleflashes.
					light->lightHandle = gameRenderWorld->AddLightDef( &light->light );
				}

				if ( !light->lightFlags.isAlwaysOn && ( gameLocal.time >= light->endTime ) ) {
					light->endTime -= light->startTime; // readjust the value.
					Event_StopWeaponLight( light->name );
				}
			}
		}
	}
}

/********************************************************************
idWeapon::StopWeaponFx( void )

Stops custom weapon particles and lights temporarily- By Clone JC Denton
********************************************************************/

void idWeapon::StopWeaponFx( void )
{
	for( int i = 0; i < weaponParticles.Num(); i++ ) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);

		if(part->particleFlags.isActive) {
			//Free the particles
			if( !part->particleFlags.isSmoke && part->modelDefHandle > -1 ) {
				gameRenderWorld->FreeEntityDef( part->modelDefHandle );
				part->modelDefHandle = -1;
			}
		}
	}

	for(int i = 0; i < weaponLights.Num(); i++) {
		WeaponLight_t* light = weaponLights.GetIndex(i);
		if( light->lightFlags.isActive ) {
			if(light->lightHandle != -1) {
				gameRenderWorld->FreeLightDef( light->lightHandle );
				light->lightHandle = -1;
			}
		}
	}
}
#endif

/*
================
idWeapon::UpdateFlashPosition
================
*/
void idWeapon::UpdateFlashPosition( void ) {
	// the flash has an explicit joint for locating it
	GetGlobalJointTransform( true, flashJointView, muzzleFlash.origin, muzzleFlash.axis );

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
	//owner->SetWeaponZoom( false ); //ff1.3
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
idWeapon::ShowWorldModel
================
*/
void idWeapon::ShowWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}

	/*
	//ff1.1 start - reactivate the ones that have "activateOnShow" set to true. lol

	for( int i = 0; i < weaponParticles.Num(); i++ ) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);

		if (part->activateOnShow) {
			part->activateOnShow = false;

			//start start
			part->active = true;
			part->startTime = gameLocal.time;
			if (!part->smoke) { //Toggle the emitter
				part->emitter->Show();
				part->emitter->PostEventMS(&EV_Activate, 0, this);
			}
			//start end
		}
	}

	for( int i = 0; i < weaponLights.Num(); i++ ) {
		WeaponLight_t* light = weaponLights.GetIndex(i);

		if (light->activateOnShow) {
			light->activateOnShow = false;
			//start start
			light->active = true;
			light->startTime = gameLocal.time;
			//start end
		}
	}

	//ff1.1 end
	*/
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

	/*
	//ff1.1 start - remember the ones we need to reactivate thanks to "activateOnShow"

	for( int i = 0; i < weaponParticles.Num(); i++ ) {
		WeaponParticle_t* part = weaponParticles.GetIndex(i);

		if (part->active) {
			part->activateOnShow = true;

			//stop start
			part->active = false;
			part->startTime = 0;
			if (!part->smoke) { //Toggle the emitter
				part->emitter->Hide();
				part->emitter->PostEventMS(&EV_Activate, 0, this);
			}
			//stop end
		}
	}

	for( int i = 0; i < weaponLights.Num(); i++ ) {
		WeaponLight_t* light = weaponLights.GetIndex(i);

		if (light->active) {
			light->activateOnShow = true;

			//stop start
			light->active = false;
			light->startTime = 0;
			if (light->lightHandle != -1) {
				gameRenderWorld->FreeLightDef( light->lightHandle );
				light->lightHandle = -1;
			}
			//stop end
		}
	}

	//ff1.1 end
	*/
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

//ff1.3 start
/*
================
idWeapon::BeginSpecialFunction
================
*/
void idWeapon::BeginSpecialFunction( bool keyTapped ) {	// new
	/*
	old
	if ( isLinked ) {
		if (keyTapped) {
			WEAPON_SPECIAL = true;
		}
		WEAPON_SPECIAL_HOLD = true;
	}
	*/

	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}

	if ( !isLinked ) {
		return;
	}

	/*
	if ( !WEAPON_ATTACK ) {
		if ( sndHum && grabberState == -1 ) {	// _D3XP :: don't stop grabber hum
			StopSound( SND_CHANNEL_BODY, false );
		}
	}
	WEAPON_ATTACK = true;
	*/
	if (keyTapped) {
		WEAPON_SPECIAL = true;
	}
	WEAPON_SPECIAL_HOLD = true;

}

/*
================
idWeapon::EndSpecialFunc
================
*/
void idWeapon::EndSpecialFunction( void ) {		//New
	if ( !WEAPON_SPECIAL_HOLD.IsLinked() ) {
		return;
	}
	if ( WEAPON_SPECIAL_HOLD ) {
		WEAPON_SPECIAL_HOLD = false;
	}
}

//ff1.3 end

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
	return !hide && !IsHidden() && ( ( status == WP_RELOAD ) || ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
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
void idWeapon::WeaponStolen( void ) {
	assert( !gameLocal.isClient );
	if ( projectileEnt ) {
		if ( isLinked ) {
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
idEntity * idWeapon::DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died ) {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return NULL;
	}
	if ( !allowDrop ) {
		return NULL;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[0] ) {
		return NULL;
	}
	StopSound( SND_CHANNEL_BODY, true );
	StopSound( SND_CHANNEL_BODY3, true );

	return idMoveableItem::DropItem( classname, worldModel.GetEntity()->GetPhysics()->GetOrigin(), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay );
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
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	thread->CallFunction( this, func, true );
	state = statename;

	animBlendFrames = blendFrames;
	if ( g_debugWeapon.GetBool() ) {
		gameLocal.Printf( "%d: weapon state : %s\n", gameLocal.time, statename );
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

	if ( ventLightJointView == INVALID_JOINT ) {
		return;
	}

	//
	// vent light
	//
	if ( nozzleGlowHandle == -1 ) {
		memset(&nozzleGlow, 0, sizeof(nozzleGlow));
		if ( owner ) {
			nozzleGlow.allowLightInViewID = owner->entityNumber+1;
		}
		nozzleGlow.pointLight = true;
		nozzleGlow.noShadows = true;
		nozzleGlow.lightRadius.x = nozzleGlowRadius;
		nozzleGlow.lightRadius.y = nozzleGlowRadius;
		nozzleGlow.lightRadius.z = nozzleGlowRadius;
		nozzleGlow.shader = nozzleGlowShader;
		nozzleGlow.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
		nozzleGlow.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
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
bool idWeapon::BloodSplat( float size ) {
	float s, c;
	idMat3 localAxis, axistemp;
	idVec3 localOrigin, normal;

	if ( hasBloodSplat ) {
		return true;
	}

	hasBloodSplat = true;

	if ( modelDefHandle < 0 ) {
		return false;
	}

	if ( !GetGlobalJointTransform( true, ejectJointView, localOrigin, localAxis ) ) {
		return false;
	}

	localOrigin[0] += gameLocal.random.RandomFloat() * -10.0f;
	localOrigin[1] += gameLocal.random.RandomFloat() * 1.0f;
	localOrigin[2] += gameLocal.random.RandomFloat() * -2.0f;

	normal = idVec3( gameLocal.random.CRandomFloat(), -gameLocal.random.RandomFloat(), -1 );
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

	const idMaterial *mtr = declManager->FindMaterial( "textures/decals/duffysplatgun" );

	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	return true;
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
	WEAPON_SPECIAL = false; //ff1.3
}

/*
================
idWeapon::AlertMonsters
================
*/
void idWeapon::AlertMonsters( void ) {
	trace_t	tr;
	idEntity *ent;
	//ff1.3 start
	//was: idVec3 end = muzzleFlash.origin + muzzleFlash.axis * muzzleFlash.target;
	idVec3 end = muzzleFlash.origin + (muzzleFlash.pointLight ? (muzzleFlash.axis[0] * muzzleFlash.lightRadius[0]) : (muzzleFlash.axis * muzzleFlash.target));
	//ff1.3 end

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
	//ff1.3 start
	/* was:
	end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
	end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	*/
	if ( muzzleFlash.pointLight ) {
		end += muzzleFlash.axis[2] * 90 * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
		end += muzzleFlash.axis[1] * 90 * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	} else {
		end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
		end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	}
	//ff1.3 end

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
		//was: if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) { //ivan
		if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || owner->UseThirdPersonCamera() ) { //ivan
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	}

	if ( nozzleFx ) {
		UpdateNozzleFx();
	}

	// muzzle smoke
	if ( showViewModel && !disabled && weaponSmoke && ( weaponSmokeStartTime != 0 ) ) {
		// use the barrel joint if available

#ifdef _D3XP
		if(smokeJointView != INVALID_JOINT) {
			GetGlobalJointTransform( true, smokeJointView, muzzleOrigin, muzzleAxis );
		} else if (barrelJointView != INVALID_JOINT) {
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
#else
		if ( barrelJointView ) {
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
#endif
		} else {
			// default to going straight out the view
			muzzleOrigin = playerViewOrigin;
			muzzleAxis = playerViewAxis;
		}
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( weaponSmoke, weaponSmokeStartTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup /*_D3XP*/ ) ) {
			weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;
		}
	}

	if ( showViewModel && strikeSmoke && strikeSmokeStartTime != 0 ) {
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( strikeSmoke, strikeSmokeStartTime, gameLocal.random.RandomFloat(), strikePos, strikeAxis, timeGroup /*_D3XP*/ ) ) {
			strikeSmokeStartTime = 0;
		}
	}

#ifdef _DENTONMOD
	//disabled flag ensures that fx wont be played when entering cinematic
	if ( showViewModel && !disabled ) {
		UpdateWeaponFx();
	}
#endif //_DENTONMOD

#ifdef _D3XP

	//OLD D3XP PARTICLES CODE
	/*
	//ff start
	if ( showViewModel ) { //was if ( showViewModel && !hide ) {
		//ff end

		for( int i = 0; i < weaponParticles.Num(); i++ ) {
			WeaponParticle_t* part = weaponParticles.GetIndex(i);

			if(part->active) {
				if(part->smoke) {
					if(part->joint != INVALID_JOINT) {

						GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );

						//ivan start
						if ( part->isViewDir ) { //use view dir
							muzzleAxis = viewWeaponAxis;
						}

						if ( part->isViewDir || part->isDirFix ) {
							//direction fix!
							//the "up" of the particle means "forward"
							idVec3 tmp =  muzzleAxis[2];
							muzzleAxis[2] =  muzzleAxis[0];
							muzzleAxis[0] = -tmp;
						}
						//ivan end
					} else {
						// default to going straight out the view
						muzzleOrigin = playerViewOrigin;
						muzzleAxis = playerViewAxis;
					}
					if ( !gameLocal.smokeParticles->EmitSmoke( part->particle, part->startTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis, timeGroup ) ) {
						part->active = false;	// all done
						part->startTime = 0;
					}
				} else {
					//Manually update the position of the emitter so it follows the weapon
					renderEntity_t* rendEnt = part->emitter->GetRenderEntity();

					if(part->joint != INVALID_JOINT) {
						GetGlobalJointTransform( true, part->joint, muzzleOrigin, muzzleAxis );

						//ivan start
						if ( part->isViewDir ) { //use view dir
							muzzleAxis = viewWeaponAxis; //playerViewAxis;
						}

						if ( part->isViewDir || part->isDirFix ) {
							//direction fix!
							//the "up" of the particle means "forward"
							idVec3 tmp =  muzzleAxis[2];
							muzzleAxis[2] =  muzzleAxis[0];
							muzzleAxis[0] = -tmp;

							//part->emitter->GetRenderEntity()->origin = muzzleOrigin;
							//part->emitter->GetRenderEntity()->axis = muzzleAxis;
						}
					} else {
						// default to going straight out the view
						muzzleOrigin = playerViewOrigin;
						muzzleAxis = playerViewAxis;
					}

					rendEnt->origin = muzzleOrigin;
					rendEnt->axis = muzzleAxis;
					//ivan end

					if ( part->emitter->GetModelDefHandle() != -1 ) {
						gameRenderWorld->UpdateEntityDef( part->emitter->GetModelDefHandle(), rendEnt );
					}
				}
			}
		}

		for(int i = 0; i < weaponLights.Num(); i++) {
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
	*/

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
	if ( guiLight.lightRadius[0] && guiLightJointView != INVALID_JOINT ) {
		GetGlobalJointTransform( true, guiLightJointView, guiLight.origin, guiLight.axis );

		if ( ( guiLightHandle != -1 ) ) {
			gameRenderWorld->UpdateLightDef( guiLightHandle, &guiLight );
		} else {
			guiLightHandle = gameRenderWorld->AddLightDef( &guiLight );
		}
	}

	if ( status != WP_READY && sndHum ) {
		StopSound( SND_CHANNEL_BODY, false );
	}

	UpdateSound();

#ifdef _SHELLWEAPON
	//pf fix
	if ( !fl.hidden ) {
		// also add a highlight shell model
		renderEntity_t	shell;

		shell = renderEntity;

		// we will mess with shader parms when the item is in view
		// to give the "item pulse" effect
		shell.callback = idWeapon::ModelCallback;
		shell.entityNum = entityNumber;
		shell.customShader = shellMaterial;
		if ( itemShellHandle == -1 ) {
			itemShellHandle = gameRenderWorld->AddEntityDef( &shell );
		} else {
			gameRenderWorld->UpdateEntityDef( itemShellHandle, &shell );
		}

	}
#endif
}


#ifdef _SHELLWEAPON
/*
================
idItem::UpdateRenderEntity
================
*/
bool idWeapon::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const {

	if ( lastRenderViewTime == renderView->time ) {
		return false;
	}

	lastRenderViewTime = renderView->time;

	/*
	// check for glow highlighting if near the center of the view
	idVec3 dir = renderEntity->origin - renderView->vieworg;
	dir.Normalize();
	float d = dir * renderView->viewaxis[0];

	// two second pulse cycle
	float cycle = ( renderView->time - inViewTime ) / 2000.0f;

	if ( d > 0.94f ) {
		if ( !inView ) {
			inView = true;
			if ( cycle > lastCycle ) {
				// restart at the beginning
				inViewTime = renderView->time;
				cycle = 0.0f;
			}
		}
	} else {
		if ( inView ) {
			inView = false;
			lastCycle = ceil( cycle );
		}
	}

	// fade down after the last pulse finishes
	if ( !inView && cycle > lastCycle ) {
		renderEntity->shaderParms[4] = 0.0f;
	} else {
		// pulse up in 1/4 second
		cycle -= (int)cycle;
		if ( cycle < 0.1f ) {
			renderEntity->shaderParms[4] = cycle * 10.0f;
		} else if ( cycle < 0.2f ) {
			renderEntity->shaderParms[4] = 1.0f;
		} else if ( cycle < 0.3f ) {
			renderEntity->shaderParms[4] = 1.0f - ( cycle - 0.2f ) * 10.0f;
		} else {
			// stay off between pulses
			renderEntity->shaderParms[4] = 0.0f;
		}
	}
	*/

	// update every single time this is in view
	return true;
}

/*
================
idItem::ModelCallback
================
*/
bool idWeapon::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView ) {
	const idWeapon *ent;

	// this may be triggered by a model trace or other non-view related source
	if ( !renderView ) {
		return false;
	}

	ent = static_cast<idWeapon *>(gameLocal.entities[ renderEntity->entityNum ]);
	if ( !ent ) {
		gameLocal.Error( "idItem::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

#endif

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
		WEAPON_SPECIAL		= false;	//	new
		WEAPON_SPECIAL_HOLD	= false;	//	new
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

	if ( !ammoname[ 0 ] ) {
		return 0;
	}

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	if ( !ammoDict->GetInt( ammoname, "-1", num ) ) {

		//ff1.3 start - code removed: use only ff-defined ammo types
		/*
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
		*/
		gameLocal.Error( "Unknown ammo type '%s'", ammoname );
		//ff1.3 end
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
	int i;
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
	//ff1.3 start - code removed: use only ff-defined ammo types
	/*
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
	*/
	//ff1.3 end
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
				return kv->GetValue();
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
void idWeapon::ResetAmmoClip( void ) {
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
		assert( 0 );
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
void idWeapon::Event_UseAmmo( int amount ) {
	if ( gameLocal.isClient ) {
		return;
	}

	//ff1.3 - was:
	//owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? amount : ( amount * ammoRequired ) );

	if ( clipSize && ammoRequired ) {
		ammoClip -= powerAmmo ? amount : ( amount * ammoRequired );
		if ( ammoClip < 0 ) {
			ammoClip = 0;
		}
	}
	//ff1.3 - fix ammo removed both from clip and inverntory
	else{
		owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? amount : ( amount * ammoRequired ) );
	}

	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1

	/*
	//ff1.3 - empty waepon switch fix
	if( !AmmoInClip() && AmmoAvailable() && !PwAmmoAvailable() ){

	}
	//ff1.3
	*/
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
	if ( ammoClip > clipSize ) {
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

	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1
}

/*
===============
idWeapon::Event_AmmoInClip
===============
*/
void idWeapon::Event_AmmoInClip( void ) {
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
	//ammoAvail += AmmoInClip(); //ff1.3 - vanilla D3 behaviour restored
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
	if ( gameLocal.isClient ) {
		idThread::ReturnFloat( 0.0f );
		return;
	}
	idThread::ReturnFloat( gameLocal.userInfo[ owner->entityNumber ].GetBool( "ui_autoReload" ) );
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
void idWeapon::Event_CreateProjectile( int projType ) { //ff1.3 - projType added
	if ( !gameLocal.isClient ) {

		//ff1.3 start
#ifdef _DENTONMOD
		if ( projType != owner->GetProjectileType() ) {
			if (!ChangeProjectileDef(projType)) {
				gameLocal.Warning( "Cannot fire proj number %d", projType );
				idThread::ReturnEntity( NULL );
				return;
			}
		}
#endif
		//ff1.3 end

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
void idWeapon::Event_LaunchProjectiles( int projType, int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower, int spreadMode ) { //ff1.3 - projType added
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	float			distance; //--
	trace_t			tr;
	idVec3			start; //--
	idVec3			muzzle_pos; //only used for bbox checks
	//idBounds		ownerBounds, projBounds; //--

	bool			barrelLaunch;
	bool			tracer, beam;

	if ( IsHidden() ) {
		return;
	}

	//ff1.3 start
#ifdef _DENTONMOD
	if ( projType != owner->GetProjectileType() ) {
		if (!ChangeProjectileDef(projType)) {
			gameLocal.Warning( "Cannot fire proj number %d", projType );
			return;
		}
	}
#endif
	//ff1.3 end

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on an MP client
	if ( !gameLocal.isClient ) {

		//ff1.3 start
		if ( projType == PROJ_TYPE_PW ) {
			owner->inventory.UsePwAmmo( weaponIndex, 1 );
		} else {
		//ff1.3 end
#ifdef _D3XP

		    //int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
            if ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) {
                return;
            }

#else
			// check if we're out of ammo or the clip is empty
			int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
			if ( !ammoAvail || ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) ) {
				return;
			}
#endif
		    // if this is a power ammo weapon ( currently only the bfg ) then make sure
            // we only fire as much power as available in each clip
            if ( powerAmmo ) {
                // power comes in as a float from zero to max
                // if we use this on more than the bfg will need to define the max
                // in the .def as opposed to just in the script so proper calcs
                // can be done here.
                dmgPower = ( int )dmgPower + 1;
                if ( dmgPower > ammoClip ) {
                    dmgPower = ammoClip;
                }
            }

#ifdef _D3XP
			if(clipSize == 0) {
				//Weapons with a clip size of 0 launch strait from inventory without moving to a clip
#endif
				//In D3XP we used the ammo when the ammo was moved into the clip so we don't want to
				//use it now.
				owner->inventory.UseAmmo( ammoType, ( powerAmmo ) ? dmgPower : ammoRequired );
#ifdef _D3XP
			}
#endif

			if ( clipSize && ammoRequired ) {
#ifdef _D3XP
				ammoClip -= powerAmmo ? dmgPower : ammoRequired;
#else
				ammoClip -= powerAmmo ? dmgPower : 1;
#endif
			}

		}//ff1.3 - normal ammo end
	}

	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1

	if ( !silent_fire ) {
		// wake up nearby monsters
		gameLocal.AlertAI( owner, playerViewOrigin );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.realClientTime );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	//-----------
	// predict instant hit projectiles
	const bool isPrediction = gameLocal.isClient && projectileDict.GetBool( "net_instanthit" );

	const float spreadRad = DEG2RAD( spread );

	idVec3 view_pos = playerViewOrigin + playerViewAxis[ 0 ] * 2.0f; // Muzzle pos for translation clip model only-- For barrel Launched projectiles

	float muzzleDistFromView;
	float traceDist, muzzleToTargetDist;
	idVec3 muzzleDir;

	beam			= projectileDict.GetFloat( "fuse" ) <= 0 || projectileDict.GetBool( "rail_beam" );
	tracer			= !beam && projectileDict.GetBool( "tracers" ) && (projectileDict.GetFloat("tracer_probability", "1.0") > gameLocal.random.RandomFloat());
	barrelLaunch	= projectileDict.GetBool( "launchFromBarrel" );

	if ( barrelLaunch || tracer || beam ) { //ivan - useBarrelDir added, so we get muzzleAxis
		// calculate the muzzle position
		if ( barrelJointView != INVALID_JOINT ) { //ivan - was barrelJointView

			// there is an explicit joint for the muzzle
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
			if ( barrelLaunch ) {
				tracer = false;
			}

			muzzle_pos = muzzleOrigin; // + playerViewAxis[ 0 ] * 2.0f;	// muzzle_pos for multiplayer prediction as well as for launching the projectiles
			// muzzleDistFromView = (muzzle_pos - view_pos).Length( ) * 3.5f;
			muzzleDistFromView = (muzzle_pos - view_pos).LengthSqr( ) * 3.5f; // This is faster
		} else {					// if we dont find a proper bone then cancel all the effects.
			barrelLaunch = false;
			tracer = false;
			beam = false;
		}
	}

	const float tracer_speed = projectileDict.GetFloat( "tracer_speed", "0.0f" );

	// add some to the kick time, incrementally moving repeat firing weapons back
	MuzzleKick(); //ff1.3 - moved to function
	/*
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}
	*/

	//was: idVec3 &launch_pos = view_pos; //wrong because view_pos would be changed after 1st projectile
	idVec3 launch_pos = ( barrelLaunch ) ? muzzle_pos : view_pos;

	owner->AddProjectilesFired( num_projectiles );
	for( i = 0; i < num_projectiles; i++ ) {
		//ff1.3 start
		/*
		//was:
		ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
		dir.Normalize();
		*/
		if ( spreadMode == WP_SPREADMODE_RANDOM ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();
		} else if ( spreadMode == WP_SPREADMODE_STEP ) {
			//es: 5 projectiles, 90° spread (that is from -90 to +90 -> 90*2)
			//(90*2)/(5-1) = each step is 1/4 of 180°= 45°
			//steps: 0, 45, -45, 90, -90
			//i-proj: odd ? ((i+1)/2) * step : (-i/2) * step
			//1th proj, i = 0: (-0)/2 * step = 0 * 45°
			//2th proj, i = 1: (1+1)/2 * step = 1 * 45°
			//3th proj, i = 2: (-2)/2 * step = -1 * 45°
			//4th proj, i = 3: (3+1)/2 * step  2 * 45°
			//5th proj, i = 4: (-4)/2 * step = -2 * 45°
			ang =  (num_projectiles > 1) ? (spread/(num_projectiles-1) * ((i%2 != 0) ? (i+1) : (-i)) ) : 0.0f;
			dir = playerViewAxis[ 0 ] - playerViewAxis[ 1 ] * ( idMath::Sin( (float)DEG2RAD( ang ) ) ); //horizontal
			dir.Normalize();
		} else {
			gameLocal.Error("Unknown WEAPON spread mode: %d", spreadMode );
		}
		//ff1.3 end

		if ( barrelLaunch || tracer || beam ) { // Do not execute this part unless projectile is barrel launched or has a tracer effect.
			gameLocal.clip.Translation( tr, view_pos, view_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
			traceDist = (tr.endpos - view_pos).LengthSqr();
			//gameRenderWorld->DebugLine( colorRed, view_pos, tr.endpos, 10000 );

			if ( traceDist > muzzleDistFromView ) { // make sure the muzzle is not to close to walls etc
				if ( barrelLaunch ) {
					dir = tr.endpos - muzzle_pos;
					dir.Normalize();
				}
				else if ( tracer ) {
					muzzleDir = tr.endpos - muzzle_pos;
					//	muzzleToTargetDist = muzzleDir.Length();
					muzzleToTargetDist = muzzleDir.LengthSqr(); // This is faster
					muzzleDir.Normalize();
				}
			}
			else{
				if ( tracer || beam ) {	// Dont do tracers when weapon is too close to walls, objects etc.
					tracer = false;
					beam = false;
				}
			}
		}

		if ( isPrediction ) {
			if ( tr.fraction < 1.0f ) {
				if ( barrelLaunch ) {	//a new trace should be made for multiplayer prediction of barrel launched projectiles
					gameLocal.clip.Translation( tr, muzzle_pos, muzzle_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
				}
				else
				{
					gameLocal.clip.Translation( tr, view_pos, view_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
				}
				idProjectile::ClientPredictionCollide( this, projectileDict, tr, vec3_origin, true );
			}
		}
		else {

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( (projType == PROJ_TYPE_PW) ? "def_projectile_pw" : (projType == PROJ_TYPE_SEC) ? "def_projectile_sec" : "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			if ( projectileDict.GetBool( "net_instanthit" ) ) {
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}

			/*
			if ( i == 0 ){ //only first time so bbox check result doesn't get overwritten
				if( barrelLaunch ){
					launch_pos = muzzle_pos;
				}else{
					launch_pos = view_pos;
				}
			}
			*/

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, launch_pos /*muzzleOrigin*/, dir );

			//bbox check start
			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
				idBounds ownerBounds = owner->GetPhysics()->GetAbsBounds();
				idBounds projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

				// DG: sometimes the assertion in idBounds::operator-(const idBounds&) triggers
				//     (would get bounding box with negative volume)
				//     => check that before doing ownerBounds - projBounds (equivalent to the check in the assertion)
				idVec3 obDiff = ownerBounds[1] - ownerBounds[0];
				idVec3 pbDiff = projBounds[1] - projBounds[0];
				bool boundsSubLegal =  obDiff.x > pbDiff.x && obDiff.y > pbDiff.y && obDiff.z > pbDiff.z;
				if ( boundsSubLegal && ( ownerBounds - projBounds ).RayIntersection( launch_pos, playerViewAxis[0], distance ) ) {
					start = launch_pos + distance * playerViewAxis[0];
				} else {
					start = playerViewOrigin; //ff1.3 may happen for sec core/shock/flame - was: ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, launch_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				launch_pos = tr.endpos;
			}
			//bbox check end

#ifdef _DENTONMOD
			if( tracer ) {
		/*		if ( traceDist <= muzzleToTargetDist ) // Ideally, this should never happen
					gameLocal.Printf ( " Unpredicted traceDistance in idWeapon::Event_LaunchProjectiles " );
		*/
				bool beamTracer = (projectileDict.GetString( "beam_skin", NULL ) != NULL);

				if ( tracer_speed != 0.0f ) {
					if( beamTracer ) { // Check whether it's a beamTracer
						proj->setTracerEffect( new dnBeamSpeedTracer(proj, tracer_speed, muzzleToTargetDist * idMath::RSqrt( muzzleToTargetDist ), muzzle_pos, muzzleDir.ToMat3()) );
					}
					else {
						proj->setTracerEffect( new dnSpeedTracer(proj, tracer_speed, muzzleToTargetDist * idMath::RSqrt( muzzleToTargetDist ), muzzle_pos, muzzleDir.ToMat3()) );
					}
				}
				else {
					if( beamTracer ) {
						proj->setTracerEffect( new dnBeamTracer(proj, traceDist/muzzleToTargetDist, view_pos, muzzle_pos, muzzleDir.ToMat3()) );
					}
					else {
						proj->setTracerEffect( new dnTracer(proj, traceDist/muzzleToTargetDist, view_pos, muzzle_pos, muzzleDir.ToMat3()) );
					}
				}
			}
			else if( beam ) {
				proj->setTracerEffect( new dnRailBeam(proj, muzzleOrigin) );
			}
#endif

			proj->Launch( launch_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower );
		}
	}

	if ( !gameLocal.isClient ) {
		// toss the brass
		if (brassDelay >= 0){  // eject brass behaviour can be disabled by simply setting the delay to 0
			PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
		}
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	if ( !continuousSmoke || weaponSmokeStartTime != 0 ) { //ff1.3 - don't reset if continuous but disabled
		weaponSmokeStartTime = gameLocal.realClientTime;
	}
}
	//-----------
#ifdef OLD_CODE
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
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	if ( gameLocal.isClient ) {

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

	} else {

		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			if ( projectileDict.GetBool( "net_instanthit" ) ) {
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
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
		}

		// toss the brass
#ifdef _D3XP
		if(brassDelay >= 0)
#endif
		PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	if ( !continuousSmoke || weaponSmokeStartTime != 0 ) { //ff1.3 - don't reset if continuous but disabled
		weaponSmokeStartTime = gameLocal.realClientTime;
	}
}
#endif

#ifdef _DENTONMOD

bool idWeapon::ChangeProjectileDef( int number ) {
	const char *projectileName;

	if( projectileEnt != NULL ) {
		gameLocal.Warning("Projectile Entity exists");
		return false;
	}


	//ivan - make denton's code compatible with FF
	if( number < 0 || number >= MAX_PROJ_TYPES ){
		gameLocal.Warning( "Invalid projType '%d'", number );
		return false;
	}

	projectileName = weaponDef->dict.GetString( (number == PROJ_TYPE_PW) ? "def_projectile_pw" : (number == PROJ_TYPE_SEC) ? "def_projectile_sec" : "def_projectile" );
	if ( *projectileName == '\0' ) {
		//gameLocal.Warning( "No proj defined for projType '%d'", number );
		return false;
	}

	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( projectileName, false );
	if ( projectileDef ) {
		const char *spawnclass = projectileDef->dict.GetString( "spawnclass" );
		idTypeInfo *cls = idClass::GetClass( spawnclass );
		if ( !cls || !cls->IsType( idProjectile::Type ) ) {
			gameLocal.Warning( "Invalid spawnclass in ChangeProjectileDef" );
		} else {
			projectileDict = projectileDef->dict;
			owner->SetProjectileType( number );

			float time_in_secs;
			if ( projectileDict.GetFloat( "muzzle_kick_time", "0.0f", time_in_secs ) ) {
				muzzle_kick_time = SEC2MS( time_in_secs );
			}
			else {
				muzzle_kick_time = SEC2MS( spawnArgs.GetFloat( "muzzle_kick_time" ) );
			}
			if ( projectileDict.GetFloat( "muzzle_kick_maxtime", "0.0f", time_in_secs ) ) {
				muzzle_kick_maxtime = SEC2MS( time_in_secs );
			}
			else {
				muzzle_kick_maxtime = SEC2MS( spawnArgs.GetFloat( "muzzle_kick_maxtime" ) );
			}

			if( !projectileDict.GetAngles( "muzzle_kick_angles", "0 0 0", muzzle_kick_angles ) ) {
				muzzle_kick_angles = spawnArgs.GetAngles( "muzzle_kick_angles" );
			}
			if( !projectileDict.GetVector( "muzzle_kick_offset", "0 0 0", muzzle_kick_offset ) ) {
				muzzle_kick_offset = spawnArgs.GetVector( "muzzle_kick_offset" );
			}

			return true;
		}
	}
	return false;
}

/*
void idWeapon::Event_ChangeProjectileDef( int number ) {

	if( number == owner->GetProjectileType() ) {
		idThread::ReturnFloat( 1 );
	}

	idThread::ReturnFloat( ChangeProjectileDef(number) ? 1 : 0 );
}

void idWeapon::Event_GetProjectileType( void ) {
	idThread::ReturnFloat( owner->GetProjectileType() );
}
*/
#endif

#ifdef _D3XP
/*
================
idWeapon::Event_LaunchProjectilesEllipse
================
*/
void idWeapon::Event_LaunchProjectilesEllipse( int projType, int num_projectiles, float spreada, float spreadb, float fuseOffset, float power ) {
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

	//ff1.3 start
#ifdef _DENTONMOD
	if ( projType != owner->GetProjectileType() ) {
		if (!ChangeProjectileDef(projType)) {
			gameLocal.Warning( "Cannot fire proj number %d", projType );
			return;
		}
	}
#endif
	//ff1.3 end

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	// avoid all ammo considerations on a client
	if ( !gameLocal.isClient ) {
        //ff1.3 start
        if ( projType == PROJ_TYPE_PW ) {
            owner->inventory.UsePwAmmo( weaponIndex, 1 );
        } else {
        //ff1.3 end

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
		}//ff1.3 - normal ammo end
		owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1

		if ( !silent_fire ) {
			// wake up nearby monsters
			gameLocal.AlertAI( owner, playerViewOrigin );
		}

	}

	//if ( weaponDef->dict.GetBool( "resetShaderParms", "1" ) ) { //ivan - new "resetShaderParms" key
		// set the shader parm to the time of last projectile firing,
		// which the gun material shaders can reference for single shot barrel glows, etc
		renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );

		if ( worldModel.GetEntity() ) {
			worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
			worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
		}
	//}

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
	MuzzleKick(); //ff1.3 - moved to function
	/*
	if ( kick_endtime < gameLocal.time ) {
		kick_endtime = gameLocal.time;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.time + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.time + muzzle_kick_maxtime;
	}
	*/

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
				const char *projectileName = weaponDef->dict.GetString( (projType == PROJ_TYPE_PW) ? "def_projectile_pw" : (projType == PROJ_TYPE_SEC) ? "def_projectile_sec" : "def_projectile" );
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
	if( useAmmo ) {
		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( !ammoAvail ) {
			return;
		}
		owner->inventory.UseAmmo( ammoType, ammoRequired );
		owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip ); //ff1.1
	}

	/* ff1.1
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
	*/

	owner->Give(powerup, va("%f", duration));


}

void idWeapon::Event_StartWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.time;
}

void idWeapon::Event_StopWeaponSmoke() {

	// reset muzzle smoke
	weaponSmokeStartTime = 0;
}

/*
//OLD D3XP PARTICLES CODE

void idWeapon::Event_StartWeaponParticle( const char* name) {
	WeaponParticle_t* part;
	weaponParticles.Get(name, &part);
	if (part && !part->active) { //ff1.1 "active" condition added
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
	if (part && part->active) { //ff1.1 "active" condition added
		part->active = false;
		//part->activateOnShow = false; //ff1.1 make sure we don't reactivate it on show
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
	if (light && !light->active) { //ff1.1 "active" condition added
		light->active = true;
		light->startTime = gameLocal.time;
	}
}

void idWeapon::Event_StopWeaponLight( const char* name) {
	WeaponLight_t* light;
	weaponLights.Get(name, &light);
	if (light && light->active) { //ff1.1 "active" condition added
		light->active = false;
		//light->activateOnShow = false; //ff1.1 make sure we don't reactivate it on show
		light->startTime = 0;
		if(light->lightHandle != -1) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
			light->lightHandle = -1;
		}
	}
}
*/
#endif

#ifdef _DENTONMOD

void idWeapon::StartWeaponParticle( const char* prtName) {
	WeaponParticle_t* part;
	weaponParticles.Get(prtName, &part);

	if(part) {
		part->particleFlags.isActive = true;

		if( part->particleFlags.isSmoke ) {
			part->startTime = gameLocal.time;
		} else {
			/*
			//toggle
			if( part->modelDefHandle > -1 ) {
				gameRenderWorld->FreeEntityDef( part->modelDefHandle );
				part->modelDefHandle = -1;
			}
			*/
			//part->renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
			//part->renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.RandomFloat(); // For effects like muzzleflashes etc.
		}
	}
}

void idWeapon::Event_StartWeaponParticle( const char* prtName) {
	StartWeaponParticle( prtName );
}

void idWeapon::StopWeaponParticle( const char* prtName) {
	WeaponParticle_t* part;
	weaponParticles.Get(prtName, &part);
	if(part) {
		part->particleFlags.isActive = false;
		part->startTime = 0;

		//Free the particles
		if(!part->particleFlags.isSmoke && part->modelDefHandle >= 0) {
			gameRenderWorld->FreeEntityDef( part->modelDefHandle );
			part->modelDefHandle = -1;
		}
	}
}

void idWeapon::Event_StopWeaponParticle( const char* prtName) {
	StopWeaponParticle( prtName );
}

void idWeapon::Event_StartWeaponLight( const char* lightName) {
	WeaponLight_t* light;
	weaponLights.Get(lightName, &light);
	if( light ) {
		light->lightFlags.isActive = true;
		light->endTime -= light->startTime; //ff1.3: fix for already existing lights: readjust the value. startTime is 0 by default so it works also for new lights.
		light->startTime = gameLocal.time;

		// these will be different each fire
		light->light.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		light->light.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

		if( !light->lightFlags.isAlwaysOn ){
			light->endTime += light->startTime;
		}
	}
}

void idWeapon::Event_StopWeaponLight( const char* lightName) {
	WeaponLight_t* light;
	weaponLights.Get(lightName, &light);
	if(light) {
		light->lightFlags.isActive = false;
		light->startTime = 0;
		if(light->lightHandle != -1) {
			gameRenderWorld->FreeLightDef( light->lightHandle );
			light->lightHandle = -1;
		}
	}
}
#endif //_DENTONMOD


/*
=====================
idWeapon::Event_Melee
=====================
*/
void idWeapon::Event_Melee( void ) {
	idEntity	*ent;
	trace_t		tr;
	//ff1.3 start
	idVec3		start, end;
	bool		barrelLaunch;
	float		strikeOffset;
	//ff1.3 end

	if ( !meleeDef ) {
		gameLocal.Error( "No meleeDef on '%s'", weaponDef->dict.GetString( "classname" ) );
	}

	if ( !gameLocal.isClient ) {
		//ff1.3 start - melee from barrel
		//was: idVec3 start = playerViewOrigin;
		strikeOffset = weaponDef->dict.GetFloat("smoke_strike_offset");
		barrelLaunch = ( barrelJointView != INVALID_JOINT ) ? weaponDef->dict.GetBool( "melee_from_barrel" ) : false;
		if ( barrelLaunch ) {
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
			start = muzzleOrigin;
			//use viewWeaponAxis instead of muzzleAxis so plasmagun is aligned to the particle
			end = start + viewWeaponAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		} else {
			start = playerViewOrigin;
			end = start + playerViewAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		}
		/*
		float width = weaponDef->dict.GetFloat("melee_width","0");
		if ( width > 0 ) { //
			idBounds meleeBox;
			meleeBox.Zero();
			meleeBox.ExpandSelf( width );
			gameLocal.clip.TraceBounds( tr, start, end, meleeBox, MASK_SHOT_RENDERMODEL, owner ); //ignore player
		} else {
			gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );  //ignore player
		}
		*/
		gameLocal.clip.TracePoint( tr, start, end, MASK_SHOT_RENDERMODEL, owner );
		//ff1.3 end

		if ( tr.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity( tr );

			//ff1.3 start - fix for damagables bound to other entities (movers, ...)
			if ( ent && !ent->fl.takedamage ) {
				ent = gameLocal.entities[ tr.c.entityNum ];
			}
			//ff1.3 end
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
/*
#ifdef _D3XP
				//Adjust the melee powerup modifier for the invulnerability boss fight
				float mod = owner->PowerUpModifier( MELEE_DAMAGE );
				if(!strcmp(ent->GetEntityDefName(), "monster_hunter_invul")) {
					//Only do a quater of the damage mod
					mod *= 0.25f;
				}
				ent->Damage( owner, owner, globalKickDir, meleeDefName, mod, tr.c.id );
#else
				*/
				ent->Damage( owner, owner, globalKickDir, meleeDefName, owner->PowerUpModifier( MELEE_DAMAGE ), tr.c.id );
//#endif
				hit = true;
			}

			if ( weaponDef->dict.GetBool( "impact_damage_effect" ) ) {
				bool bleed = ent->spawnArgs.GetBool( "bleed" );
				if ( bleed /*ent->spawnArgs.GetBool( "bleed" )*/ ) {

					hitSound = meleeDef->dict.GetString( owner->PowerUpActive( BERSERK ) ? "snd_hit_berserk" : "snd_hit" );

					ent->AddDamageEffect( tr, impulse, meleeDef->dict.GetString( "classname" ) );

				//} else {
				}
				if( !bleed || weaponDef->dict.GetBool( "smoke_strike_onblood" ) ){
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
					//ff1.3 start
					strikePos = tr.c.point /*+ strikeOffset * tr.c.normal*/ - strikeOffset * playerViewAxis[0]; // Actual effect can start a little away object.
					//was: strikePos = tr.c.point;
					//ff1.3 end
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
void idWeapon::Event_EjectBrass( void ) {
	if ( !g_showBrass.GetBool() || !owner->CanShowWeaponViewmodel() ) {
		return;
	}

	if ( ejectJointView == INVALID_JOINT || !brassDict.GetNumKeyVals() ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	idMat3 axis;
	idVec3 origin, linear_velocity, angular_velocity;
	idEntity *ent;

	if ( !GetGlobalJointTransform( true, ejectJointView, origin, axis ) ) {
		return;
	}

	gameLocal.SpawnEntityDef( brassDict, &ent, false );
	if ( !ent || !ent->IsType( idDebris::Type ) ) {
		gameLocal.Error( "'%s' is not an idDebris", weaponDef ? weaponDef->dict.GetString( "def_ejectBrass" ) : "def_ejectBrass" );
	}
	idDebris *debris = static_cast<idDebris *>(ent);
	debris->Create( owner, origin, axis );
	debris->Launch();

	linear_velocity = 40 * ( playerViewAxis[0] + playerViewAxis[1] + playerViewAxis[2] );
	angular_velocity.Set( 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat(), 10 * gameLocal.random.CRandomFloat() );

	debris->GetPhysics()->SetLinearVelocity( linear_velocity );
	debris->GetPhysics()->SetAngularVelocity( angular_velocity );
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

//ff1.1 start
/*
===============
idWeapon::Event_SetPwState
===============

void idWeapon::Event_SetPwState( int value ) {
	owner->SetPwStateCurrentWeapon(value);
}
*/
/*
===============
idWeapon::Event_GetPwState
===============

void idWeapon::Event_GetPwState() {
	idThread::ReturnFloat( owner->GetPwStateCurrentWeapon() );
}
*/

/*
===============
idWeapon::Event_UsePwAmmo
===============
*/
void idWeapon::Event_UsePwAmmo( int amount ) {
	owner->inventory.UsePwAmmo( weaponIndex, amount ); //owner->UsePwAmmoCurrentWeapon( amount );
	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip );
}



/*
===============
idWeapon::Event_PwAmmoAvailable
===============
*/
void idWeapon::Event_PwAmmoAvailable( void ) {
	int ammoAvail = owner->inventory.HasPwAmmo( weaponIndex ); //owner->GetPwAmmoCurrentWeapon();
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_AddAmmo
===============

void idWeapon::Event_AddAmmo( int amount ) {
	if ( gameLocal.isClient ) {
		return;
	}

	owner->inventory.AddAmmo( ammoType, amount );
	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip );
}
*/
//ff1.1 end

//ff1.3 start

/*
================
idWeapon::Event_GetPainKillerProjectile
================
*/
void idWeapon::Event_GetPainKillerProjectile( void ){
	idThread::ReturnEntity( owner->GetPainKillerProjectile() );
}

/*
================
idWeapon::Event_GetRemoteGrenadeProjectile
================
*/
void idWeapon::Event_GetRemoteGrenadeProjectile( void ){
	idThread::ReturnEntity( owner->GetRemoteGrenadeProjectile() );
}

/*
================
idWeapon::GetMuzzlePos
================
*/
void idWeapon::GetMuzzlePos( idVec3 &pos ) {
	if ( barrelJointView != INVALID_JOINT ) {
		GetGlobalJointTransform( true, barrelJointView, pos, muzzleAxis );
	} else {
		pos = playerViewOrigin;
	}
}

/*
================
idWeapon::PwAmmoAvailable
================
*/
int idWeapon::PwAmmoAvailable( void ) const {
	if ( owner ) {
		return owner->inventory.HasPwAmmo( weaponIndex );
	} else {
		return 0;
	}
}

/*
===============
idWeapon::Event_SkullAmmoAvailable
===============
*/
void idWeapon::Event_SkullAmmoAvailable( int mode ) {
	int ammoAvail = owner->inventory.HasSkullAmmo( mode );
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_UseSkullAmmo
===============
*/
void idWeapon::Event_UseSkullAmmo( int mode, int amount ) {
	owner->inventory.UseSkullAmmo( mode, amount );
	owner->UpdHudAmmoCurrentWeapon( ammoType, ammoClip );
}

/*
================
idWeapon::IconPw
================
*/
const char *idWeapon::IconPw( void ) const {
	return iconPw;
}

/*
================
idWeapon::Event_SetSkullMode
================
*/
void idWeapon::Event_SetSkullMode( int number ) {
	owner->inventory.SetSkullMode( number );
}

/*
================
idWeapon::Event_GetSkullMode
================
*/
void idWeapon::Event_GetSkullMode( void ) {
	idThread::ReturnFloat( owner->inventory.GetSkullMode() );
}

/*
================
idWeapon::Event_GetAimTarget
================
*/
void idWeapon::Event_GetAimTarget( void ) {
	idThread::ReturnEntity( owner->GetAimTarget(4000.0f) );
}

/*
================
idWeapon::Event_GetEnemyTarget
================
*/
void idWeapon::Event_GetEnemyTarget( void ) {
	idEntity *ent =  owner->GetAimTarget(4000.0f);
	idThread::ReturnEntity( ( ent && ent->health > 0 && ent->IsType(idAI::Type) && (static_cast< idAI * >(ent)->team != owner->team) ) ? ent : NULL );
}


/*
================
idWeapon::Event_UpdateLockCursor
================
*/
void idWeapon::Event_UpdateLockCursor( idEntity* target ) {
	owner->UpdateLockCursor( target );
}

/*
================
idWeapon::Event_HasAmmo
================
*/
void idWeapon::Event_HasAmmo( void ) {
	int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	int pwAmmoAvail = owner->inventory.HasPwAmmo( weaponIndex );
	bool hasAmmo = ( ammoAvail > 0 || pwAmmoAvail > 0 || (clipSize > 0 && ammoClip > 0) );
	idThread::ReturnInt( hasAmmo );
}

/*
================
idWeapon::MuzzleKick
================
*/
void idWeapon::MuzzleKick( void ) {
	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}
}

#ifdef _DENTONMOD
/*
===============
idWeapon::Event_StartZoom
===============
*/
void idWeapon::Event_SetZoom( int mode ) {
	if ( mode ) {
		//owner->SetWeaponZoom( true );
		owner->EnableWeaponZoom( ( mode != 1 ) );
	}
	else {
		//owner->SetWeaponZoom( false );
		owner->DisableWeaponZoom();
	}
}

/*
===============
idWeapon::Event_IsAdvancedZooming
===============
*/
void idWeapon::Event_IsAdvancedZooming( void ) {
	if ( owner->IsAdvancedWeaponZooming() ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idWeapon::Event_IsAdvancedZooming
===============
*/
void idWeapon::Event_AnyFireButtonPressed( void ) {
	idThread::ReturnInt( owner->AnyFireButtonPressed() );
}

#endif

//ff1.3 end