/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "Inventory/WeaponItem.h"

/***********************************************************************

  idWeapon  
	
***********************************************************************/

//
// event defs
//
const idEventDef EV_Weapon_Clear( "<clear>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Weapon_GetOwner( "getOwner", EventArgs(), 'e', "Returns the owning entity" );
const idEventDef EV_Weapon_Next( "nextWeapon", EventArgs(), EV_RETURNS_VOID, "" );
const idEventDef EV_Weapon_State( "weaponState", EventArgs('s', "stateFunction", "", 'd', "blendFrames", ""), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_UseAmmo( "useAmmo", EventArgs('d', "amount", ""), EV_RETURNS_VOID, "Eats the specified amount of ammo" );
const idEventDef EV_Weapon_AddToClip( "addToClip", EventArgs('d', "amount", ""), EV_RETURNS_VOID, "" );
const idEventDef EV_Weapon_AmmoInClip( "ammoInClip", EventArgs(), 'f', "" );
const idEventDef EV_Weapon_AmmoAvailable( "ammoAvailable", EventArgs(), 'f', "Number of shots left in inventory" );
const idEventDef EV_Weapon_TotalAmmoCount( "totalAmmoCount", EventArgs(), 'f', 
	"Amount of ammo in inventory. since each shot may use more than 1 ammo, this is different than ammoAvailable()" );
const idEventDef EV_Weapon_ClipSize( "clipSize", EventArgs(), 'f', "" );
const idEventDef EV_Weapon_WeaponOutOfAmmo( "weaponOutOfAmmo", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_WeaponReady( "weaponReady", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_WeaponReloading( "weaponReloading", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_WeaponHolstered( "weaponHolstered", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_WeaponRising( "weaponRising", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_WeaponLowering( "weaponLowering", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_Flashlight( "flashlight", EventArgs('d', "enable", ""), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_LaunchProjectiles( "launchProjectiles", 
	EventArgs('d', "num_projectiles", "", 'f', "spread", "", 'f', "fuseOffset", "", 'f', "launchPower", "", 'f', "dmgPower", ""), 
	EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_CreateProjectile( "createProjectile", EventArgs(), 'e', "no description" );
const idEventDef EV_Weapon_EjectBrass( "ejectBrass", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_Melee( "melee", EventArgs(), 'd', "no description" );
const idEventDef EV_Weapon_GetWorldModel( "getWorldModel", EventArgs(), 'e', "Returns the entity that controls the world model" );
const idEventDef EV_Weapon_AllowDrop( "allowDrop", EventArgs('d', "allow", ""), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_NetReload( "netReload", EventArgs(), EV_RETURNS_VOID, "no description" );
const idEventDef EV_Weapon_IsInvisible( "isInvisible", EventArgs(), 'f', "no description" );
const idEventDef EV_Weapon_NetEndReload( "netEndReload", EventArgs(), EV_RETURNS_VOID, "no description" );
//const idEventDef EV_Weapon_ShowAttachment( "showAttachment", EventArgs('s', "", "", 'd', "", ""), EV_RETURNS_VOID, "" );
//const idEventDef EV_Weapon_ShowAttachmentInd( "showAttachmentInd", "EventArgs('d', "", "", 'd', "", ""), EV_RETURNS_VOID, "" );

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
	EVENT( AI_PauseAnim,						idWeapon::Event_PauseAnim )
	EVENT( AI_AnimIsPaused,						idWeapon::Event_AnimIsPaused )
	EVENT( AI_PlayCycle,						idWeapon::Event_PlayCycle )
	EVENT( AI_SetBlendFrames,					idWeapon::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,					idWeapon::Event_GetBlendFrames )
	EVENT( AI_AnimDone,							idWeapon::Event_AnimDone )
	EVENT( EV_Weapon_Next,						idWeapon::Event_Next )
	EVENT( EV_SetSkin,							idWeapon::Event_SetSkin )
	EVENT( AI_ShowAttachment,					idWeapon::ShowAttachment )
	EVENT( AI_ShowAttachmentInd,				idWeapon::ShowAttachmentInd )
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
	EVENT( EV_Weapon_NetReload,					idWeapon::Event_NetReload )
	EVENT( EV_Weapon_IsInvisible,				idWeapon::Event_IsInvisible )
	EVENT( EV_Weapon_NetEndReload,				idWeapon::Event_NetEndReload )
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

	brassDelay				= 0;

	allowDrop				= true;
	hideUntilTime			= -1;

	Clear();
}

/*
================
idWeapon::~idWeapon()
================
*/
idWeapon::~idWeapon() 
{
	// destroy weapon attachments
	for( int i=0; i<m_Attachments.Num(); i++ )
	{
		idEntity *ent = m_Attachments[i].ent.GetEntity();
		if( ent )
			ent->Event_Remove();
	}

	Clear();
	delete worldModel.GetEntity();
}


/*
================
idWeapon::Spawn
================
*/
void idWeapon::Spawn( void ) {
	{
		// setup the world model
		worldModel = static_cast< idAnimatedEntity * >( gameLocal.SpawnEntityType( idAnimatedEntity::Type, NULL ) );
	}

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

	savefile->WriteVec3( playerViewOrigin );
	savefile->WriteMat3( playerViewAxis );

	savefile->WriteVec3( viewWeaponOrigin );
	savefile->WriteMat3( viewWeaponAxis );

	savefile->WriteVec3( muzzleOrigin );
	savefile->WriteMat3( muzzleAxis );

	savefile->WriteVec3( pushVelocity );

	savefile->WriteString( weaponDef->GetName() );
	savefile->WriteFloat( meleeDistance );
	savefile->WriteFloat( knockoutRange );
	savefile->WriteFloat( KOBoxSize );
	savefile->WriteString( meleeDefName );
	savefile->WriteInt( brassDelay );
	savefile->WriteString( icon );

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

	savefile->WriteInt( kick_endtime );
	savefile->WriteInt( muzzle_kick_time );
	savefile->WriteInt( muzzle_kick_maxtime );
	savefile->WriteAngles( muzzle_kick_angles );
	savefile->WriteVec3( muzzle_kick_offset );

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

	savefile->WriteInt(m_Attachments.Num());
	for (int i = 0; i < m_Attachments.Num(); i++) 
	{
		m_Attachments[i].Save(savefile);
	}
}

/*
================
idWeapon::Restore
================
*/
void idWeapon::Restore( idRestoreGame *savefile ) {

	savefile->ReadInt( (int &)status );
	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( animDoneTime );
	savefile->ReadBool( isLinked );

	// Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_BLOCK.LinkTo(		scriptObject, "WEAPON_BLOCK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );
	WEAPON_INDICATE.LinkTo(		scriptObject, "WEAPON_INDICATE");

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

	/*const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_projectile" ), false );
	if ( projectileDef ) {
		projectileDict = projectileDef->dict;
	} else {
		projectileDict.Clear();
	}*/

	const idDeclEntityDef *brassDef = gameLocal.FindEntityDef( weaponDef->dict.GetString( "def_ejectBrass" ), false );
	if ( brassDef ) {
		brassDict = brassDef->dict;
	} else {
		brassDict.Clear();
	}

	savefile->ReadFloat( meleeDistance );
	savefile->ReadFloat( knockoutRange );
	savefile->ReadFloat( KOBoxSize );
	savefile->ReadString( meleeDefName );
	savefile->ReadInt( brassDelay );
	savefile->ReadString( icon );

	savefile->ReadInt( guiLightHandle );
	savefile->ReadRenderLight( guiLight );

	savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadRenderLight( muzzleFlash );

	savefile->ReadInt( worldMuzzleFlashHandle );
	savefile->ReadRenderLight( worldMuzzleFlash );

	savefile->ReadVec3( flashColor );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( flashTime );

	savefile->ReadBool( lightOn );

	savefile->ReadInt( kick_endtime );
	savefile->ReadInt( muzzle_kick_time );
	savefile->ReadInt( muzzle_kick_maxtime );
	savefile->ReadAngles( muzzle_kick_angles );
	savefile->ReadVec3( muzzle_kick_offset );

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

	int num;
	savefile->ReadInt(num);
	m_Attachments.SetNum(num);
	for (int i = 0; i < num; i++) 
	{
		m_Attachments[i].Restore(savefile);
	}
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
	WEAPON_BLOCK.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_NETRELOAD.Unlink();
	WEAPON_NETENDRELOAD.Unlink();
	WEAPON_NETFIRING.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();
	WEAPON_INDICATE.Unlink();

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
	while( kv ) 
	{
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "snd_" );
	}
	// And the propagated sounds:
	kv = spawnArgs.MatchPrefix( "sprS_" );
	while( kv ) 
	{
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "sprS_" );
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

	meleeDef		= NULL;
	meleeDefName	= "";
	meleeDistance	= 0.0f;
	knockoutRange	= 0.0f;
	brassDict.Clear();

	flashTime		= 250;
	lightOn			= false;

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

	hasBloodSplat		= false;
	nozzleFx			= false;
	nozzleFxFade		= 1500;
	lastAttack			= 0;
	lastBlock			= 0;
	nozzleGlowHandle	= -1;
	nozzleGlowShader	= NULL;
	nozzleGlowRadius	= 10;
	nozzleGlowColor.Zero();

	weaponAngleOffsetAverages	= 0;
	weaponAngleOffsetScale		= 0.0f;
	weaponAngleOffsetMax		= 0.0f;
	weaponOffsetTime			= 0.0f;
	weaponOffsetScale			= 0.0f;

	arrow2Arrow					= false;

	allowDrop			= true;

	animator.ClearAllAnims( gameLocal.time, 0 );
	FreeModelDef();

	sndHum				= NULL;

	isLinked			= false;
	projectileEnt		= NULL;

	isFiring			= false;

	// TDM: destroy previous weapon attachments
	for( int i=0; i<m_Attachments.Num(); i++ )
	{
		idEntity *ent = m_Attachments[i].ent.GetEntity();
		if( ent )
			ent->Event_Remove();
	}

	m_Attachments.ClearFree();

	m_animRates.ClearFree();

	// clear attack flags on the player
	if( owner )
	{
		owner->SetAttackFlag(COMBAT_MELEE, false);
		owner->SetAttackFlag(COMBAT_RANGED, false);
	}
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
	int			ammoAvail = 0;

	Clear();

	if ( !objectname || !objectname[ 0 ] ) {
		return;
	}

	assert( owner );

	weaponDef			= gameLocal.FindEntityDef( objectname );

	ammoRequired		= weaponDef->dict.GetInt( "ammo_required" );
	
	idStr ammoDefName = weaponDef->dict.GetString("def_ammo");

	if (ammoDefName.IsEmpty() && weaponDef->dict.GetBool("ammo_required"))
	{
		gameLocal.Error("No 'ammo' spawnarg defined for weapon %s", objectname);
	}

	
	clipSize			= weaponDef->dict.GetInt( "clipSize" );
	lowAmmo				= weaponDef->dict.GetInt( "lowAmmo" );

	icon				= weaponDef->dict.GetString( "icon" );
	powerAmmo			= weaponDef->dict.GetBool( "powerAmmo" );

	muzzle_kick_time	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( weaponDef->dict.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= weaponDef->dict.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= weaponDef->dict.GetVector( "muzzle_kick_offset" );

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
	}

	// setup the view model
	vmodel = weaponDef->dict.GetString( "model_view" );
	SetModel( vmodel );

	// setup the world model
	InitWorldModel( weaponDef );

	// copy the sounds from the weapon view model def into out spawnargs
	const idKeyValue *kv = weaponDef->dict.MatchPrefix( "snd_" );
	while( kv ) 
	{
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = weaponDef->dict.MatchPrefix( "snd_", kv );
	}
	// And the propagated sounds
	kv = weaponDef->dict.MatchPrefix( "sprS_" );
	while( kv ) 
	{
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = weaponDef->dict.MatchPrefix( "sprS_", kv );
	}

	// find some joints in the model for locating effects
	barrelJointView = animator.GetJointHandle( "barrel" );
	flashJointView = animator.GetJointHandle( "flash" );
	ejectJointView = animator.GetJointHandle( "eject" );
	guiLightJointView = animator.GetJointHandle( "guiLight" );
	ventLightJointView = animator.GetJointHandle( "ventLight" );

	// ishtvan: Cache the anim rates here to adjust weapon anim speeds
	int anims = animator.NumAnims();
	m_animRates.Clear();
	m_animRates.AssureSize(anims);
	for (int i=0; i<anims; i++) 
	{
		const idAnim *anim = animator.GetAnim(i);
		if (anim != NULL) 
		{
			idStr spawnargname = "anim_rate_";
			spawnargname += anim->Name();
			m_animRates[i] = weaponDef->dict.GetFloat(spawnargname, "1");
		} else 
		{
			m_animRates[i] = 1.0f;
		}
	}

	// get the projectile
	// greebo: Disabled, the projectile def is loaded on demand
	/*projectileName = weaponDef->dict.GetString( "def_projectile" );
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
	}*/

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
	meleeDistance = weaponDef->dict.GetFloat( "melee_distance","56.0");
	knockoutRange = weaponDef->dict.GetFloat("knockout_range", "28.0");
	KOBoxSize = weaponDef->dict.GetFloat("KO_box_size", "4.0");
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

	ammoClip = ammoinclip;
	if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) ) {
		// first time using this weapon so have it fully loaded to start
		ammoClip = clipSize;
		CInventoryWeaponItemPtr weaponItem = owner->GetCurrentWeaponItem();
		if (weaponItem != NULL) {
			ammoAvail = weaponItem->HasAmmo();
		}
		if ( ammoClip > ammoAvail ) {
			ammoClip = ammoAvail;
		}
	}

	renderEntity.gui[ 0 ] = NULL;
	guiName = weaponDef->dict.GetString( "gui" );
	if ( guiName[0] ) {
		renderEntity.gui[ 0 ] = uiManager->FindGui( guiName, true, false, true );
	}

	zoomFov = weaponDef->dict.GetInt( "zoomFov", "70" );

	weaponAngleOffsetAverages = weaponDef->dict.GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale = weaponDef->dict.GetFloat( "weaponAngleOffsetScale", "0.25" );
	weaponAngleOffsetMax = weaponDef->dict.GetFloat( "weaponAngleOffsetMax", "10" );

	weaponOffsetTime = weaponDef->dict.GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale = weaponDef->dict.GetFloat( "weaponOffsetScale", "0.005" );

	// Bow aimer related -- By Dram
	const idKeyValue *AimerKeyVal = weaponDef->dict.MatchPrefix( "def_aimer", NULL );
	idEntity *bow_aimer( NULL );
	if ( AimerKeyVal ) // grayman - always attach the aimer
	{
		idDict aimerArgs;
		aimerArgs.Set( "classname", AimerKeyVal->GetValue().c_str() );
		aimerArgs.Set( "dropToFloor", "0" );
		gameLocal.SpawnEntityDef( aimerArgs, &bow_aimer );
		if ( bow_aimer )
		{
			Attach( bow_aimer, NULL, NULL );
		}

		// show it or hide it initially based on the menu setting

		if ( !cv_bow_aimer.GetBool() )
		{
			bow_aimer->Hide();
		}
	}
	
	// spawn any weapon attachments we might have
	const idKeyValue *KeyVal = weaponDef->dict.MatchPrefix( "def_attach", NULL );
	idEntity *ent(NULL);

	while ( KeyVal ) 
	{
		idDict args2;

		args2.Set( "classname", KeyVal->GetValue().c_str() );

		// don't let them drop to the floor
		args2.Set( "dropToFloor", "0" );
		
		gameLocal.SpawnEntityDef( args2, &ent );
		if ( !ent ) 
		{
			gameLocal.Error( "Couldn't spawn '%s' to attach to entity '%s'", KeyVal->GetValue().c_str(), name.c_str() );
		} else 
		{
			DM_LOG(LC_WEAPON, LT_DEBUG)LOGSTRING("Def_Attaching entity %s to weapon entity %s.\r", ent->name.c_str(), name.c_str());
			
			// check for attachment position spawnarg
			idStr Suffix = KeyVal->GetKey();
			Suffix.StripLeading( "def_attach" );
			idStr PosKey = "pos_attach" + Suffix;
			// String name of the attachment for later accessing
			idStr AttName = "name_attach" + Suffix;

			if( weaponDef->dict.FindKey(PosKey.c_str()) )
				Attach( ent, weaponDef->dict.GetString(PosKey.c_str()), 
						weaponDef->dict.GetString(AttName.c_str()) );
			else
				Attach( ent, NULL, weaponDef->dict.GetString(AttName.c_str()) );
			if (IsRanged()) // grayman #597 - hide arrows for one second & play an 'equip' sound
			{
				ent->Hide();
				ent->SetHideUntilTime(gameLocal.time + 1000);

				// Play an arrow-equipping sound

				if (arrow2Arrow)
				{
					const char *shader = "arrow_equip";
					const idSoundShader *sndEquip = declManager->FindSound(shader);
					StartSoundShader(sndEquip,SND_CHANNEL_BODY,0,false,NULL);
				}
			}
		}
		KeyVal = weaponDef->dict.MatchPrefix( "def_attach", KeyVal );
	}

	if ( !weaponDef->dict.GetString( "weapon_scriptobject", NULL, &objectType ) ) {
		gameLocal.Error( "No 'weapon_scriptobject' set on '%s'.", objectname );
	}
	
	// setup script object
	if ( !scriptObject.SetType( objectType ) ) {
		gameLocal.Error( "Script object '%s' not found on weapon '%s'.", objectType, objectname );
	}

	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_BLOCK.LinkTo(		scriptObject, "WEAPON_BLOCK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_NETRELOAD.LinkTo(	scriptObject, "WEAPON_NETRELOAD" );
	WEAPON_NETENDRELOAD.LinkTo(	scriptObject, "WEAPON_NETENDRELOAD" );
	WEAPON_NETFIRING.LinkTo(	scriptObject, "WEAPON_NETFIRING" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );
	WEAPON_INDICATE.LinkTo(		scriptObject, "WEAPON_INDICATE");

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

	// update attack flags on the player
	if(weaponDef->dict.GetBool("is_weapon_melee"))
	{
		owner->SetAttackFlag(COMBAT_MELEE, true);
	}
	else if(weaponDef->dict.GetBool("is_weapon_ranged"))
	{
		owner->SetAttackFlag(COMBAT_RANGED, true);
	}
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
		renderEntity.gui[ 0 ]->SetStateString( "player_totalammo", va( "%i", ammoamount - inclip) );
		renderEntity.gui[ 0 ]->SetStateString( "player_ammo", ClipSize() ? va( "%i", inclip ) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_clips", ClipSize() ? va("%i", ammoamount / ClipSize()) : "--" );
		renderEntity.gui[ 0 ]->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount - inclip ) );
	}
	renderEntity.gui[ 0 ]->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_empty", ( inclip == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "player_clip_low", ( inclip <= lowAmmo ) );
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
idWeapon::SetArrow2Arrow
================
*/
void idWeapon::SetArrow2Arrow(  bool state ) // grayman #597
{
	arrow2Arrow = state;
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
=====================
idWeapon::Indicate
=====================
*/
void idWeapon::Indicate(bool indicate)
{
	if (isLinked)
	{
		WEAPON_INDICATE = indicate;
	}
}

/*
====================
idWeapon::Perform_KO
====================
*/
void idWeapon::Perform_KO(void)
{
	if (!isLinked)
	{
		return;
	}
	Event_Melee();
	/*
	trace_t trace;
	idVec3 start, end, aiEyePos;
	idEntity* ent;
	idAI* enemy;
	float dist;
	int loc;
	start = playerViewOrigin;
	end = start + playerViewAxis[0] * 56.0f;
	gameLocal.clip.TracePoint(trace, start, end, MASK_SHOT_RENDERMODEL, owner);
	if (trace.fraction < 1.0f)
	{
		ent = gameLocal.entities[trace.c.entityNum];
		common->Printf(ent->name.c_str());
		if (ent->IsBound())
		{
			ent = ent->GetBindMaster();
		}
		if (ent->IsType(idAI::Type))
		{
			enemy = static_cast<idAI*>(ent);
			aiEyePos = enemy->GetEyePosition();
			dist = (aiEyePos - trace.endpos).Length();
			common->Printf("\t%f",dist);
			loc = -1;
			if (dist < 24)
			{
				loc = enemy->GetDamageLocation("head");
			}
			enemy->TestKnockoutBlow(owner, playerViewAxis[0], &trace, loc, 0);
		}
		common->Printf("\n");
		gameRenderWorld->DebugArrow(colorCyan, start, end, 3, 1000);
	}
	*/
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

	// hide attachments:
	for(int i=0; i<m_Attachments.Num(); i++)
	{
		m_Attachments[i].ent.GetEntity()->Hide();
	}
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

	// show attachments

	// grayman #597 - don't show attachments if they're hidden and there's a hide timeout

	for (int i = 0; i < m_Attachments.Num() ; i++)
	{
		idEntity *e = m_Attachments[i].ent.GetEntity();
		if ((gameLocal.time >= e->GetHideUntilTime()) && e->IsHidden())
		{
			e->Show();
		}
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
void idWeapon::BeginAttack( void ) 
{	
	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}

	if ( !isLinked ) {
		return;
	}

	if ( !WEAPON_ATTACK )
	{
		if ( sndHum )
		{
			StopSound( SND_CHANNEL_BODY, false );
		}

		// grayman #597 - show hidden arrows the first time through

		if (IsRanged())
		{
			for (int i = 0; i < m_Attachments.Num() ; i++)
			{
				idEntity *e = m_Attachments[i].ent.GetEntity();

				// grayman - If this is the bow aimer, and the player is using it, show it.
				// If the player isn't using it, hide it.

				if ( idStr::Cmp( e->spawnArgs.GetString("classname"), "atdm:attachment_aimer" ) == 0 )
				{
					if ( cv_bow_aimer.GetBool() )
					{
						if ( e->IsHidden() )
						{
							e->Show();
						}
					}
					else if ( !e->IsHidden() )
					{
						e->Hide();
					}
				}
				else if ( e->IsHidden() ) // attachments other than the bow aimer
				{
					e->Show();
				}
			}
		}
	}

	WEAPON_ATTACK = true;
}

/*
================
idWeapon::EndAttack
================
*/
void idWeapon::EndAttack( void ) 
{
	if ( !WEAPON_ATTACK.IsLinked() )
		return;
	if ( WEAPON_ATTACK ) {
		WEAPON_ATTACK = false;
		if ( sndHum ) {
			StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
		}
	}
}

/*
================
idWeapon::BeginBlock
================
*/
void idWeapon::BeginBlock( void ) 
{	
	lastBlock = gameLocal.time;

	if ( !isLinked )
		return;

	if ( !WEAPON_BLOCK ) 
	{
		if ( sndHum ) 
		{
			StopSound( SND_CHANNEL_BODY, false );
		}
	}
	WEAPON_BLOCK = true;
}

/*
================
idWeapon::EndBlock
================
*/
void idWeapon::EndBlock( void ) 
{
	if ( !WEAPON_BLOCK.IsLinked() ) 
		return;

	if ( WEAPON_BLOCK ) 
	{
		WEAPON_BLOCK = false;
		if ( sndHum )
			StartSoundShader( sndHum, SND_CHANNEL_BODY, 0, false, NULL );
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
//	return !( state == idStr( WP_RISING ) || state == idStr( WP_LOWERING ) || state == idStr( WP_HOLSTERED ) );
	// Never show the crosshair in TDM.
	return false;
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

	normal.x = gameLocal.random.CRandomFloat();
	normal.y = -gameLocal.random.RandomFloat();
	normal.z = -1;
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
void idWeapon::PresentWeapon( bool showViewModel ) 
{
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

	// TDM/Ishtvan : Call RunPhysics to update bound entity positions
	RunPhysics();

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
		if ( g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
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
		if ( barrelJointView ) {
			GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
		} else {
			// default to going straight out the view
			muzzleOrigin = playerViewOrigin;
			muzzleAxis = playerViewAxis;
		}
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( weaponSmoke, weaponSmokeStartTime, gameLocal.random.RandomFloat(), muzzleOrigin, muzzleAxis ) ) {
			weaponSmokeStartTime = ( continuousSmoke ) ? gameLocal.time : 0;
		}
	}

	if ( showViewModel && strikeSmoke && strikeSmokeStartTime != 0 ) {
		// spit out a particle
		if ( !gameLocal.smokeParticles->EmitSmoke( strikeSmoke, strikeSmokeStartTime, gameLocal.random.RandomFloat(), strikePos, strikeAxis ) ) {
			strikeSmokeStartTime = 0;
		}
	}

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
		if ( lightOn && !owner->fl.notarget ) {
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

	UpdateSound(); // grayman #4337
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
		WEAPON_BLOCK		= false;
		WEAPON_RELOAD		= false;
		WEAPON_NETRELOAD	= false;
		WEAPON_NETENDRELOAD	= false;
		WEAPON_NETFIRING	= false;
		WEAPON_RAISEWEAPON	= false;
		WEAPON_LOWERWEAPON	= false;
		WEAPON_INDICATE		= false;
	}

	disabled = true;
	arrow2Arrow = false; // grayman #597 - for arrow switching

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
		gameLocal.Error( "Unknown ammo type '%s'", ammoname );
	}

	if ( ( num < 0 ) || ( num >= AMMO_NUMTYPES ) ) {
		gameLocal.Error( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, AMMO_NUMTYPES );
	}

	return ( ammo_t )num;
}*/

/*
================
idWeapon::GetAmmoNameForNum
================
*/
const char *idWeapon::GetAmmoNameForNum( int ammonum ) {
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

	return NULL;
}

/*
================
idWeapon::GetAmmoPickupNameForNum
================
*/
const char *idWeapon::GetAmmoPickupNameForNum( int ammonum ) {
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
	if (owner && owner->GetCurrentWeaponItem()) {
		return owner->GetCurrentWeaponItem()->HasAmmo();
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

/*
================
idWeapon::WriteToSnapshot
================
*/
void idWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( ammoClip, ASYNC_PLAYER_INV_CLIP_BITS );
	msg.WriteBits( worldModel.GetEntityNum(), 32 );
	msg.WriteBits( worldModel.GetSpawnNum(), 32 );
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
	int entId = msg.ReadBits( 32 );
	int spnId = msg.ReadBits( 32 );
	worldModel.SetDirectlyWithIntegers( entId, spnId );
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
	return false;
}

/*
idWeapon::IsRanged
*/
bool idWeapon::IsRanged() {
	// greebo: Check the weapon dictionary, whether a projectile def is declared
	return (weaponDef != NULL && weaponDef->dict.FindKey("def_projectile") != NULL);
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
	owner->GetCurrentWeaponItem()->UseAmmo(( powerAmmo ) ? amount : ( amount * ammoRequired ) );
	if ( clipSize && ammoRequired ) {
		ammoClip -= powerAmmo ? amount : ( amount * ammoRequired );
		if ( ammoClip < 0 ) {
			ammoClip = 0;
		}
	}
}

/*
===============
idWeapon::Event_AddToClip
===============
*/
void idWeapon::Event_AddToClip( int amount ) {
	int ammoAvail;

	ammoClip += amount;
	if ( ammoClip > clipSize ) {
		ammoClip = clipSize;
	}

	ammoAvail = owner->GetCurrentWeaponItem()->HasAmmo();
	if ( ammoClip > ammoAvail ) {
		ammoClip = ammoAvail;
	}
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
	CInventoryWeaponItemPtr currentWeapon = owner->GetCurrentWeaponItem();
	int ammoAvail = (currentWeapon != NULL) ? currentWeapon->HasAmmo() : 0;
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
idWeapon::Event_TotalAmmoCount
===============
*/
void idWeapon::Event_TotalAmmoCount( void ) {
	CInventoryWeaponItemPtr currentWeapon = owner->GetCurrentWeaponItem();
	int ammoAvail = (currentWeapon != NULL) ? currentWeapon->HasAmmo() : 0;
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
idWeapon::Event_NetReload
===============
*/
void idWeapon::Event_NetReload( void ) {
	assert( owner );
}

/*
===============
idWeapon::Event_NetEndReload
===============
*/
void idWeapon::Event_NetEndReload( void ) {
	assert( owner );
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

#ifndef SUPPRESS_CONSOLE_WARNINGS
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
#endif

		animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {

		// grayman #597 - if this is a raise or lower animation, and we're
		// switching from one arrow to another, skip the animation.

		idStr raiseString = "raise";
		idStr lowerString = "lower";
		bool raiseOrLower = ((raiseString.Cmp(animname) == 0) || (lowerString.Cmp(animname) == 0));
		if (arrow2Arrow && raiseOrLower)
		{
			animator.Clear( channel, gameLocal.time, FRAME2MS( animBlendFrames ) );
			animDoneTime = 0;
			if (lowerString.Cmp(animname) == 0)
			{
				status = WP_HOLSTERED;	// do this right away, otherwise it takes 12 frames to have it done elsewhere
										// and you see a gap in the bow animation
			}
		}
		else
		{
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

#ifndef SUPPRESS_CONSOLE_WARNINGS
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
#endif

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
idWeapon::Event_PauseAnim
===============
*/
void idWeapon::Event_PauseAnim( int channel, bool bPause )
{
	animator.CurrentAnim( channel )->Pause( bPause );
}

/*
===============
idWeapon::Event_AnimIsPaused
===============
*/
void idWeapon::Event_AnimIsPaused( int channel )
{
	idThread::ReturnInt( animator.CurrentAnim( channel )->IsPaused() );
}

/*
===============
idWeapon::Event_AnimDone
===============
*/
void idWeapon::Event_AnimDone( int channel, int blendFrames ) 
{
// ishtvan: rewrote this to be more like what happens in idActor/idAnimState
	int DoneTime = animator.CurrentAnim( channel )->GetEndTime();

	if (DoneTime < 0)
	{
		// playing a cycle or paused
		idThread::ReturnInt( false );
	}
	else if ( DoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) 
	{
		idThread::ReturnInt( true );
	} else 
	{
		idThread::ReturnInt( false );
	}
}

/*
===============
idWeapon::Event_SetBlendFrames
===============
*/
void idWeapon::Event_SetBlendFrames( int channel, int blendFrames ) 
{
	// ishtvan: this is a problem with animrate??
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

/*
================
idWeapon::Event_CreateProjectile
================
*/
void idWeapon::Event_CreateProjectile()
{
	{
		projectileEnt = NULL;

		// Try to get the weapon inventory item
		CInventoryWeaponItemPtr weaponItem = owner->GetWeaponItem(weaponDef->dict.GetString("inv_weapon_name"));

		if (weaponItem == NULL)
		{
			idThread::ReturnEntity( NULL );
			return;
		}

		// Retrieve the currently active projectile def name from the inventory item
		const idStr& projectileDefName = weaponItem->GetProjectileDefName();		

		const idDeclEntityDef* projectileDef = gameLocal.FindEntityDef(projectileDefName);

		if (projectileDef != NULL)
		{
			gameLocal.SpawnEntityDef( projectileDef->dict, &projectileEnt, false );

			if ( projectileEnt )
			{
				projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
				projectileEnt->Bind( owner, false );
				projectileEnt->Hide();
			}
		}

		idThread::ReturnEntity( projectileEnt );
	}
}

/*
================
idWeapon::Event_LaunchProjectiles
================
*/
void idWeapon::Event_LaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	if ( IsHidden() ) {
		return;
	}

	/*if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = weaponDef->dict.GetString( "classname" );
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}*/
	CInventoryWeaponItemPtr weaponItem = owner->GetWeaponItem(weaponDef->dict.GetString("inv_weapon_name"));
	if (weaponItem == NULL) return;

	// Retrieve the currently active projectile def name from the inventory item
	const idStr& projectileDefName = weaponItem->GetProjectileDefName();

	if (projectileDefName.IsEmpty())
	{
		gameLocal.Warning("No projectile entityDef defined on '%s'", name.c_str());
		return;
	}

	const idDeclEntityDef* projectileDef = gameLocal.FindEntityDef(projectileDefName);
	
	if (projectileDef == NULL)
	{
		gameLocal.Warning("Projectile entityDef %s not found.", projectileDefName.c_str());
		return;
	}

	{

		CInventoryWeaponItemPtr weaponItem = owner->GetCurrentWeaponItem();

		// check if we're out of ammo or the clip is empty
		int ammoAvail = weaponItem->HasAmmo();
		if ( !ammoAvail || ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) ) {
			return;
		}

		weaponItem->UseAmmo(ammoRequired);

		if ( clipSize && ammoRequired ) {
			ammoClip -= powerAmmo ? (int) dmgPower : 1;
		}
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	renderEntity.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.CRandomFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.realClientTime );

	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_DIVERSITY, renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel.GetEntity()->SetShaderParm( SHADERPARM_TIMEOFFSET, renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && projectileDef->dict.GetBool( "launchFromBarrel" ) ) {
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

	{

		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();

			const idMat3 *shootLcs = nullptr;
			if (projectileDef->dict.GetBool("fire_along_playerview", "0")) {
				// greebo: Fire the projectile along the playerview direction
				shootLcs = &playerViewAxis;
				//dir = playerViewAxis.ToAngles().ToForward();
			}
			else {
				shootLcs = &muzzleAxis;
				//dir = muzzleAxis[ 0 ]; // Dram: Make the weapon shoot directly from the barrel bone. Found by Ishtvan
			}
			dir = (*shootLcs)[ 0 ] + (*shootLcs)[ 2 ] * ( ang * idMath::Sin( spin ) ) - (*shootLcs)[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			//gameRenderWorld->DebugArrow(colorWhite, muzzleOrigin, muzzleOrigin + dir*100, 1, 15000);

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				gameLocal.SpawnEntityDef( projectileDef->dict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = weaponDef->dict.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, muzzleOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile start point does not clip with anything
			if ( i == 0 ) {
				muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
				start = ownerBounds.GetCenter();
				/* stgatilov: translate to viewpoint first, then to muzzle point.
				 Otherwise shot while leaning over the corner would lag. Picture:
				  XXXX M		C - center of player's bbox
				  XXXX |		V - player's view origin
				  XXXX |		M - muzzle position
				  C----V
				*/
				idClipModel *clm = proj->GetPhysics()->GetClipModel();
				if (!gameLocal.clip.Translation( tr, start, playerViewOrigin, clm, clm->GetAxis(), MASK_SHOT_RENDERMODEL, owner )) {
					gameLocal.clip.Translation( tr, playerViewOrigin, muzzle_pos, clm, clm->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				}
				muzzle_pos = tr.endpos;
				
			}

			proj->Launch( muzzle_pos, dir, pushVelocity, fuseOffset, launchPower, dmgPower );

			// TDM: Update responsible actor for the projectile, so it can be tracked for alerts
			proj->m_SetInMotionByActor = owner;
			proj->m_MovedByActor = owner;
		}

		// toss the brass
		PostEventMS( &EV_Weapon_EjectBrass, brassDelay );
	}

	// add the light for the muzzleflash
	if ( !lightOn ) {
		MuzzleFlashLight();
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// reset muzzle smoke
	weaponSmokeStartTime = gameLocal.realClientTime;
}
/*
=====================
idWeapon::canKnockout
=====================
*/
bool idWeapon::canKnockout(void)
{
	if (!meleeDef || !worldModel.GetEntity()) {
		return false;
	}
	return (meleeDef->dict.GetFloat("knockout") > 0);
}

/*
=====================
idWeapon::getMeleeDistance
=====================
*/
float idWeapon::getMeleeDistance(void)
{
	if (!meleeDef || !worldModel.GetEntity()) {
		return false;
	}
	return meleeDistance;
}

/*
=====================
idWeapon::getKnockoutRange
=====================
*/
float idWeapon::getKnockoutRange(void)
{
	if (!meleeDef || !worldModel.GetEntity()) {
		return false;
	}
	return knockoutRange;
}

/*
======================
idWeapon::getKOBoxSize
======================
*/
float idWeapon::getKOBoxSize(void)
{
	if (!meleeDef || !worldModel.GetEntity()) {
		return false;
	}
	return KOBoxSize;
}

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

	{
		idVec3 start = playerViewOrigin;
		idVec3 end = start + playerViewAxis[0] * ( meleeDistance * owner->PowerUpModifier( MELEE_DISTANCE ) );
		idBounds bo;
		bo.Zero();
		bo.ExpandSelf(KOBoxSize);
		gameLocal.clip.TraceBounds( tr, start, end, bo, MASK_SHOT_RENDERMODEL, owner);
		//gameRenderWorld->DebugArrow(colorGreen, start, end, 3, 1000);
		if ( tr.fraction < 1.0f && gameLocal.entities[tr.c.entityNum] ) 
		{
			//ent = gameLocal.GetTraceEntity( tr );
			ent = gameLocal.entities[ tr.c.entityNum ];
		} else 
		{
			ent = NULL;
		}

		if ( g_debugWeapon.GetBool() ) {
			gameRenderWorld->DebugLine( colorYellow, start, end, 100 );
			if ( ent ) {
				gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 100 );
			}
		}

		bool hit = false;
		idStr hitSound = meleeDef->dict.GetString( "snd_miss" );
		idStr sndName = "snd_miss";

		if ( ent ) {

			float push = meleeDef->dict.GetFloat( "push" );
			idVec3 impulse = -push * owner->PowerUpModifier( SPEED ) * tr.c.normal;

			if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) && ( ent->IsType( idActor::Type ) || ent->IsType( idAFAttachment::Type) ) ) {
				idThread::ReturnInt( 0 );
				return;
			}


			if ( ent->fl.takedamage ) {
				idVec3 kickDir, globalKickDir;
				meleeDef->dict.GetVector( "kickDir", "0 0 0", kickDir );
				globalKickDir = muzzleAxis * kickDir;

				
				if (ent->IsBound())
				{
					ent = ent->GetBindMaster();
				}
				if( ent->IsType(idAI::Type) )
				{
					if (meleeDef->dict.GetFloat("knockout") > 0)
					{
						if ((static_cast<idAI*>(ent)->GetEyePosition() - tr.endpos).Length() < knockoutRange)
						{
							static_cast<idAI*>(ent)->TestKnockoutBlow(owner, idVec3(), &tr, static_cast<idAI*>(ent)->GetDamageLocation("head"), 0);
						}
					}
				}
				ent->Damage(owner, owner, globalKickDir, meleeDefName, owner->PowerUpModifier(MELEE_DAMAGE), CLIPMODEL_ID_TO_JOINT_HANDLE(tr.c.id), &tr);


				hit = true;
			}

			ent->ApplyImpulse(this, tr.c.id, tr.c.point, impulse);
			if ( weaponDef->dict.GetBool( "impact_damage_effect" ) ) {

				if ( ent->spawnArgs.GetBool( "bleed" ) ) {

					ent->AddDamageEffect( tr, impulse, meleeDef->dict.GetString( "classname" ) );
				} 

					idStr materialType;
					int type = tr.c.material->GetSurfaceType();

					if ( type == SURFTYPE_NONE ) 
						materialType = gameLocal.surfaceTypeNames[ GetDefaultSurfaceType() ];
					else
						g_Global.GetSurfName( tr.c.material, materialType );

					// start impact sound based on material type
					hitSound = meleeDef->dict.GetString( va( "snd_%s", materialType.c_str() ) );
					sndName = va( "snd_%s", materialType.c_str() );

					if ( hitSound.IsEmpty() ) 
					{
						hitSound = meleeDef->dict.GetString( "snd_metal" );
						sndName = "snd_metal";
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
						sndName = "";
					}

					strikeSmokeStartTime = gameLocal.time;
					strikePos = tr.c.point;
					strikeAxis = -tr.endAxis;
				
			}
		}

		if ( !hitSound.IsEmpty() ) 
		{
			const idSoundShader *snd = declManager->FindSound( hitSound.c_str() );
			StartSoundShader( snd, SND_CHANNEL_BODY2, 0, true, NULL );
			
			// Propagate the sound to AI, must find global sound first because it's on a different dict
			sndName.StripLeading("snd_");
			sndName = meleeDef->dict.GetString( va("sprS_%s", sndName.c_str()) );
			if( !sndName.IsEmpty() )
				PropSoundDirect( sndName.c_str(), false, false, 0.0f, 0 ); // grayman #3355
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
	angular_velocity.x = 10 * gameLocal.random.CRandomFloat();
	angular_velocity.y = 10 * gameLocal.random.CRandomFloat();
	angular_velocity.z = 10 * gameLocal.random.CRandomFloat();

	debris->GetPhysics()->SetLinearVelocity( linear_velocity );
	debris->GetPhysics()->SetAngularVelocity( angular_velocity );
}

/*
===============
idWeapon::Event_IsInvisible
===============
*/
void idWeapon::Event_IsInvisible( void ) {
	// greebo: No invisibility in TDM currently
	idThread::ReturnFloat( 0 );
}

/*
===============
idWeapon::ClientPredictionThink
===============
*/
void idWeapon::ClientPredictionThink( void ) {
	UpdateAnimation();	
}

/*
================
idWeapon::Attach
================
*/
void idWeapon::Attach( idEntity *ent, const char *PosName, const char *AttName ) 
{
	idAnimatedEntity::Attach( ent, PosName, AttName );

	// set up the same rendering conditions as the weapon viewmodel
	ent->GetRenderEntity()->allowSurfaceInViewID = owner->entityNumber+1;
	ent->GetRenderEntity()->weaponDepthHack = true;
}

/*
================
idWeapon::GetAttachingTransform
================
*/
void idWeapon::GetAttachingTransform( jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis )
{
	GetGlobalJointTransform( true, jointHandle, offset, axis );
}


