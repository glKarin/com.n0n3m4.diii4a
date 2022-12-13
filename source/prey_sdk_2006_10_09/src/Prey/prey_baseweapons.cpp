#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define WEAPON_DEBUG if(g_debugWeapon.GetBool()) gameLocal.Warning

/***********************************************************************

  hhWeapon
	
***********************************************************************/
const idEventDef EV_PlayAnimWhenReady( "playAnimWhenReady", "s" );
const idEventDef EV_Weapon_Aside( "<weaponAside>" );
const idEventDef EV_Weapon_EjectAltBrass( "ejectAltBrass" );

const idEventDef EV_Weapon_HasAmmo( "hasAmmo", "", 'd' );
const idEventDef EV_Weapon_HasAltAmmo( "hasAltAmmo", "", 'd' );

const idEventDef EV_Weapon_GetFireDelay( "getFireDelay", "", 'f' );
const idEventDef EV_Weapon_GetAltFireDelay( "getAltFireDelay", "", 'f' );
const idEventDef EV_Weapon_GetSpread( "getSpread", "", 'f' );
const idEventDef EV_Weapon_GetAltSpread( "getAltSpread", "", 'f' );
const idEventDef EV_Weapon_GetString( "getString", "s", 's' );
const idEventDef EV_Weapon_GetAltString( "getAltString", "s", 's' );

const idEventDef EV_Weapon_AddToAltClip( "addToAltClip", "f" );
const idEventDef EV_Weapon_AltAmmoInClip( "altAmmoInClip", "", 'f' );		
const idEventDef EV_Weapon_AltAmmoAvailable( "altAmmoAvailable", "", 'f' );	
const idEventDef EV_Weapon_AltClipSize( "altClipSize", "", 'f' );		

const idEventDef EV_Weapon_FireAltProjectiles( "fireAltProjectiles" );
const idEventDef EV_Weapon_FireProjectiles( "fireProjectiles" );

const idEventDef EV_Weapon_WeaponAside( "weaponAside" );	// nla
const idEventDef EV_Weapon_WeaponPuttingAside( "weaponPuttingAside" ); // nla
const idEventDef EV_Weapon_WeaponUprighting( "weaponUprighting" );	// nla

const idEventDef EV_Weapon_IsAnimPlaying( "isAnimPlaying", "s", 'd' );
const idEventDef EV_Weapon_Raise( "<raise>" );	// nla - For the hands to post an event to raise the weapons

const idEventDef EV_Weapon_SetViewAnglesSensitivity( "setViewAnglesSensitivity", "f" );
const idEventDef EV_Weapon_UseAltAmmo( "useAltAmmo", "d" );

const idEventDef EV_Weapon_Hide( "hideWeapon" );
const idEventDef EV_Weapon_Show( "showWeapon" );

CLASS_DECLARATION( hhAnimatedEntity, hhWeapon )
	EVENT( EV_Weapon_FireAltProjectiles,	hhWeapon::Event_FireAltProjectiles )
	EVENT( EV_Weapon_FireProjectiles,		hhWeapon::Event_FireProjectiles )
	EVENT( EV_PlayAnimWhenReady,			hhWeapon::Event_PlayAnimWhenReady )
	EVENT( EV_SpawnFxAlongBone,				hhWeapon::Event_SpawnFXAlongBone )
	EVENT( EV_Weapon_EjectAltBrass,			hhWeapon::Event_EjectAltBrass )
	EVENT( EV_Weapon_HasAmmo,				hhWeapon::Event_HasAmmo )
	EVENT( EV_Weapon_HasAltAmmo,			hhWeapon::Event_HasAltAmmo )
	EVENT( EV_Weapon_AddToAltClip,			hhWeapon::Event_AddToAltClip )
	EVENT( EV_Weapon_AltAmmoInClip,			hhWeapon::Event_AltAmmoInClip )
	EVENT( EV_Weapon_AltAmmoAvailable,		hhWeapon::Event_AltAmmoAvailable )
	EVENT( EV_Weapon_AltClipSize,			hhWeapon::Event_AltClipSize )
	EVENT( EV_Weapon_GetFireDelay,			hhWeapon::Event_GetFireDelay )
	EVENT( EV_Weapon_GetAltFireDelay,		hhWeapon::Event_GetAltFireDelay )
	EVENT( EV_Weapon_GetString,				hhWeapon::Event_GetString )
	EVENT( EV_Weapon_GetAltString,			hhWeapon::Event_GetAltString )
	EVENT( EV_Weapon_Raise,					hhWeapon::Event_Raise )
	EVENT( EV_Weapon_WeaponAside,			hhWeapon::Event_Weapon_Aside )
	EVENT( EV_Weapon_WeaponPuttingAside,	hhWeapon::Event_Weapon_PuttingAside )
	EVENT( EV_Weapon_WeaponUprighting,		hhWeapon::Event_Weapon_Uprighting )
	EVENT( EV_Weapon_IsAnimPlaying,			hhWeapon::Event_IsAnimPlaying )
	EVENT( EV_Weapon_SetViewAnglesSensitivity,	hhWeapon::Event_SetViewAnglesSensitivity )
	EVENT( EV_Weapon_Hide,					hhWeapon::Event_HideWeapon )
	EVENT( EV_Weapon_Show,					hhWeapon::Event_ShowWeapon )

	//idWeapon
	EVENT( EV_Weapon_State,					hhWeapon::Event_WeaponState )
	EVENT( EV_Weapon_WeaponReady,			hhWeapon::Event_WeaponReady )
	EVENT( EV_Weapon_WeaponOutOfAmmo,		hhWeapon::Event_WeaponOutOfAmmo )
	EVENT( EV_Weapon_WeaponReloading,		hhWeapon::Event_WeaponReloading )
	EVENT( EV_Weapon_WeaponHolstered,		hhWeapon::Event_WeaponHolstered )
	EVENT( EV_Weapon_WeaponRising,			hhWeapon::Event_WeaponRising )
	EVENT( EV_Weapon_WeaponLowering,		hhWeapon::Event_WeaponLowering )
	EVENT( EV_Weapon_AddToClip,				hhWeapon::Event_AddToClip )
	EVENT( EV_Weapon_AmmoInClip,			hhWeapon::Event_AmmoInClip )
	EVENT( EV_Weapon_AmmoAvailable,			hhWeapon::Event_AmmoAvailable )
	EVENT( EV_Weapon_ClipSize,				hhWeapon::Event_ClipSize )
	EVENT( AI_PlayAnim,						hhWeapon::Event_PlayAnim )
	EVENT( AI_PlayCycle,					hhWeapon::Event_PlayCycle )
	EVENT( AI_AnimDone,						hhWeapon::Event_AnimDone )
	EVENT( EV_Weapon_Next,					hhWeapon::Event_Next )
	EVENT( EV_Weapon_Flashlight,			hhWeapon::Event_Flashlight )
	EVENT( EV_Weapon_EjectBrass,			hhWeapon::Event_EjectBrass )
	EVENT( EV_Weapon_GetOwner,				hhWeapon::Event_GetOwner )
	EVENT( EV_Weapon_UseAmmo,				hhWeapon::Event_UseAmmo )
	EVENT( EV_Weapon_UseAltAmmo,			hhWeapon::Event_UseAltAmmo )
END_CLASS

/*
================
hhWeapon::hhWeapon
================
*/
hhWeapon::hhWeapon() {
	owner			= NULL;
	worldModel		= NULL;

	thread = NULL;
	
	//HUMANHEAD: aob
	fireController = NULL;
	altFireController = NULL;
	fl.networkSync = true;
	cameraShakeOffset.Zero(); //rww - had to move here to avoid den/nan/blah in spawn
	//HUMANHEAD END

	Clear();
}

/*
================
hhWeapon::Spawn
================
*/
void hhWeapon::Spawn() {
	//idWeapon
	if ( !gameLocal.isClient )
	{
		worldModel = static_cast< hhAnimatedEntity * >( gameLocal.SpawnEntityType( hhAnimatedEntity::Type, NULL ) );
		worldModel.GetEntity()->fl.networkSync = true;
	}

	thread = new idThread();
	thread->ManualDelete();
	thread->ManualControl();
	//idWeapon End

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( NULL, 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	SetPhysics( &physicsObj );

	fl.solidForTeam = true;

	// HUMANHEAD: aob
	memset( &eyeTraceInfo, 0, sizeof(trace_t) );
	eyeTraceInfo.fraction = 1.0f;
	BecomeActive( TH_TICKER );
	handedness 			= hhMath::hhMax<int>( 1, spawnArgs.GetInt("handedness", "1") );
	fl.neverDormant = true;
	// HUMANHEAD END
}

/*
================
hhWeapon::~hhWeapon()
================
*/
hhWeapon::~hhWeapon() {
	SAFE_REMOVE( worldModel );
	SAFE_REMOVE( thread );

	SAFE_DELETE_PTR( fireController );
	SAFE_DELETE_PTR( altFireController );

	if ( nozzleFx && nozzleGlowHandle != -1 ) {
		gameRenderWorld->FreeLightDef( nozzleGlowHandle );
	}
}

/*
================
hhWeapon::SetOwner
================
*/
void hhWeapon::SetOwner( idPlayer *_owner ) {
	if( !_owner || !_owner->IsType(hhPlayer::Type) ) {
		owner = NULL;
		return;
	}

	owner = static_cast<hhPlayer*>( _owner );

	if( GetPhysics() && GetPhysics()->IsType(hhPhysics_StaticWeapon::Type) ) {
		static_cast<hhPhysics_StaticWeapon*>(GetPhysics())->SetSelfOwner( owner.GetEntity() );
	}
}

/*
================
hhWeapon::Save
================
*/
void hhWeapon::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( status );
	savefile->WriteObject( thread );
	savefile->WriteString( state );
	savefile->WriteString( idealState );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( animDoneTime );

	owner.Save( savefile );
	worldModel.Save( savefile );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteObject( fireController );
	savefile->WriteObject( altFireController );

	savefile->WriteTrace( eyeTraceInfo );
	savefile->WriteVec3( cameraShakeOffset );
	savefile->WriteInt( handedness );

	savefile->WriteVec3( pushVelocity );

	savefile->WriteString( weaponDef->GetName() );

	savefile->WriteString( icon );

	savefile->WriteInt( kick_endtime );

	savefile->WriteBool( lightOn );

	savefile->WriteInt( zoomFov );

	savefile->WriteInt( weaponAngleOffsetAverages );
	savefile->WriteFloat( weaponAngleOffsetScale );
	savefile->WriteFloat( weaponAngleOffsetMax );
	savefile->WriteFloat( weaponOffsetTime );
	savefile->WriteFloat( weaponOffsetScale );

	savefile->WriteBool( nozzleFx );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->WriteInt( nozzleGlowHandle );
	savefile->WriteJoint( nozzleJointHandle.view );
	savefile->WriteJoint( nozzleJointHandle.world );
	savefile->WriteRenderLight( nozzleGlow );
	savefile->WriteVec3( nozzleGlowColor );
	savefile->WriteMaterial( nozzleGlowShader );
	savefile->WriteFloat( nozzleGlowRadius );
	savefile->WriteVec3( nozzleGlowOffset );
}

/*
================
hhWeapon::Restore
================
*/
void hhWeapon::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)status );
	savefile->ReadObject( reinterpret_cast<idClass*&>(thread) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( animDoneTime );

	//Re-link script fields
	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );
	WEAPON_NEXTATTACK.LinkTo(	scriptObject, "nextAttack" ); //HUMANHEAD rww

	//HUMANHEAD: aob
	WEAPON_ALTATTACK.LinkTo(	scriptObject, "WEAPON_ALTATTACK" );
	WEAPON_ASIDEWEAPON.LinkTo(	scriptObject, "WEAPON_ASIDEWEAPON" );
	WEAPON_ALTMODE.LinkTo(		scriptObject, "WEAPON_ALTMODE" );
	//HUMANHEAD END

	owner.Restore( savefile );
	worldModel.Restore( savefile );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadObject( reinterpret_cast< idClass *&> ( fireController ) );
	savefile->ReadObject( reinterpret_cast< idClass *&> ( altFireController ) );

	savefile->ReadTrace( eyeTraceInfo );
	savefile->ReadVec3( cameraShakeOffset );
	savefile->ReadInt( handedness );

	savefile->ReadVec3( pushVelocity );

	idStr objectname;
	savefile->ReadString( objectname );
	weaponDef = gameLocal.FindEntityDef( objectname, false );
	if (!weaponDef) {
		gameLocal.Error( "Unknown weaponDef:  %s\n", objectname );
	}
    dict = &(weaponDef->dict);

	savefile->ReadString( icon );

	savefile->ReadInt( kick_endtime );

	savefile->ReadBool( lightOn );

	savefile->ReadInt( zoomFov );

	savefile->ReadInt( weaponAngleOffsetAverages );
	savefile->ReadFloat( weaponAngleOffsetScale );
	savefile->ReadFloat( weaponAngleOffsetMax );
	savefile->ReadFloat( weaponOffsetTime );
	savefile->ReadFloat( weaponOffsetScale );

	savefile->ReadBool( nozzleFx );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->ReadInt( nozzleGlowHandle );
	savefile->ReadJoint( nozzleJointHandle.view );
	savefile->ReadJoint( nozzleJointHandle.world );
	savefile->ReadRenderLight( nozzleGlow );
	savefile->ReadVec3( nozzleGlowColor );
	savefile->ReadMaterial( nozzleGlowShader );
	savefile->ReadFloat( nozzleGlowRadius );
	savefile->ReadVec3( nozzleGlowOffset );
}

/*
================
hhWeapon::Clear
================
*/
void hhWeapon::Clear( void ) {
	DeconstructScriptObject();
	scriptObject.Free();

	WEAPON_ATTACK.Unlink();
	WEAPON_RELOAD.Unlink();
	WEAPON_RAISEWEAPON.Unlink();
	WEAPON_LOWERWEAPON.Unlink();
	WEAPON_NEXTATTACK.Unlink(); //HUMANHEAD rww

	//HUMANHEAD: aob
	WEAPON_ALTATTACK.Unlink();
	WEAPON_ASIDEWEAPON.Unlink();
	WEAPON_ALTMODE.Unlink();

	SAFE_DELETE_PTR( fireController );
	SAFE_DELETE_PTR( altFireController );
	//HUMANEAD END

	//memset( &renderEntity, 0, sizeof( renderEntity ) );
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

	// nozzle fx
	nozzleFx = false;
	memset( &nozzleGlow, 0, sizeof( nozzleGlow ) );
	nozzleGlowHandle		= -1;
	nozzleJointHandle.Clear();

	memset( &refSound, 0, sizeof( refSound_t ) );
	if ( owner.IsValid() ) {
		// don't spatialize the weapon sounds
		refSound.listenerId = owner->GetListenerId();
	}

	// clear out the sounds from our spawnargs since we'll copy them from the weapon def
	const idKeyValue *kv = spawnArgs.MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Delete( kv->GetKey() );
		kv = spawnArgs.MatchPrefix( "snd_" );
	}

	weaponDef		= NULL;
	dict			= NULL;

	kick_endtime	= 0;

	icon			= "";

	pushVelocity.Zero();

	status			= WP_HOLSTERED;
	state			= "";
	idealState		= "";
	animBlendFrames	= 0;
	animDoneTime	= 0;

	lightOn			= false;

	zoomFov = 90;

	weaponAngleOffsetAverages = 10;		//initialize this to default number to prevent infinite loops

	FreeModelDef();
}

/*
================
hhWeapon::InitWorldModel
================
*/
void hhWeapon::InitWorldModel( const idDict *dict ) {
	idEntity *ent;

	ent = worldModel.GetEntity();
	if ( !ent || !dict ) {
		return;
	}

	const char *model = dict->GetString( "model_world" );
	const char *attach = dict->GetString( "joint_attach" );

	if (gameLocal.isMultiplayer) { //HUMANHEAD rww - shadow default based on whether the player shadows
		ent->GetRenderEntity()->noShadow = MP_PLAYERNOSHADOW_DEFAULT;
	}

	if ( model[0] && attach[0] ) {
		ent->Show();
		ent->SetModel( model );
		ent->GetPhysics()->SetContents( 0 );
		ent->GetPhysics()->SetClipModel( NULL, 1.0f );
		ent->BindToJoint( owner.GetEntity(), attach, true );
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
}

/*
================
hhWeapon::GetWeaponDef

HUMANHEAD: aob - this should only be called after the weapon has just been created
================
*/
void hhWeapon::GetWeaponDef( const char *objectname ) {
	//HUMANHEAD: aob - put logic in helper function
	ParseDef( objectname );
	//HUMANHEAD END

	if( owner->inventory.weaponRaised[owner->GetWeaponNum(objectname)] || gameLocal.isMultiplayer )
		ProcessEvent( &EV_Weapon_State, "Raise", 0 );
	else {
		ProcessEvent( &EV_Weapon_State, "NewRaise", 0 );
		owner->inventory.weaponRaised[owner->GetWeaponNum(objectname)] = true;
	}
}

/*
================
hhWeapon::ParseDef

HUMANHEAD: aob
================
*/
void hhWeapon::ParseDef( const char* objectname ) {
	if( !objectname || !objectname[ 0 ] ) {
		return;
	}

	if( !owner.IsValid() ) {
		gameLocal.Error( "hhWeapon::ParseDef: no owner" );
	}

	weaponDef = gameLocal.FindEntityDef( objectname, false );
	if (!weaponDef) {
		gameLocal.Error( "Unknown weaponDef:  %s\n", objectname );
	}
	dict = &(weaponDef->dict);

	// setup the world model
	InitWorldModel( dict );

	if (!owner.IsValid() || !owner.GetEntity()) {
		gameLocal.Error("NULL owner in hhWeapon::ParseDef!");
	}

	//HUMANHEAD: aob
	const idDict* infoDict = gameLocal.FindEntityDefDict( dict->GetString("def_fireInfo"), false );
	if( infoDict ) {
		fireController = CreateFireController();
		if( fireController ) {
			fireController->Init( infoDict, this, owner.GetEntity() );
		}
	}

	infoDict = gameLocal.FindEntityDefDict( dict->GetString("def_altFireInfo"), false );
	if( infoDict ) {
		altFireController = CreateAltFireController();
		if( altFireController ) {
			altFireController->Init( infoDict, this, owner.GetEntity() );
		}
	}
	//HUMANHEAD END

	icon = dict->GetString( "inv_icon" );

	// copy the sounds from the weapon view model def into out spawnargs
	const idKeyValue *kv = dict->MatchPrefix( "snd_" );
	while( kv ) {
		spawnArgs.Set( kv->GetKey(), kv->GetValue() );
		kv = dict->MatchPrefix( "snd_", kv );
	}

	weaponAngleOffsetAverages = dict->GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale = dict->GetFloat( "weaponAngleOffsetScale", "0.1" );
	weaponAngleOffsetMax = dict->GetFloat( "weaponAngleOffsetMax", "10" );

	weaponOffsetTime = dict->GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale = dict->GetFloat( "weaponOffsetScale", "0.002" );
	
	zoomFov = dict->GetInt( "zoomFov", "70" );

	InitScriptObject( dict->GetString("scriptobject") );

	nozzleFx			= weaponDef->dict.GetBool( "nozzleFx", "0" );
	nozzleGlowColor		= weaponDef->dict.GetVector("nozzleGlowColor", "1 1 1");
	nozzleGlowRadius	= weaponDef->dict.GetFloat("nozzleGlowRadius", "10");
	nozzleGlowOffset	= weaponDef->dict.GetVector( "nozzleGlowOffset", "0 0 0" );
	nozzleGlowShader = declManager->FindMaterial( weaponDef->dict.GetString( "mtr_nozzleGlowShader", "" ), false );
	GetJointHandle( weaponDef->dict.GetString( "nozzleJoint", "" ), nozzleJointHandle );

	//HUMANHEAD bjk
	int clipAmmo = owner->inventory.clip[owner->GetWeaponNum(objectname)];
	if ( ( clipAmmo < 0 ) || ( clipAmmo > fireController->ClipSize() ) ) {
		// first time using this weapon so have it fully loaded to start
		clipAmmo = fireController->ClipSize();
		if ( clipAmmo > fireController->AmmoAvailable() ) {
			clipAmmo = fireController->AmmoAvailable();
		}
	}
	fireController->AddToClip(clipAmmo);

	WEAPON_ALTMODE = owner->inventory.altMode[owner->GetWeaponNum(objectname)];
	//HUMANHEAD END

	Show();
}

/*
================
idWeapon::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool hhWeapon::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
================
hhWeapon::InitScriptObject
================
*/
void hhWeapon::InitScriptObject( const char* objectType ) {
	if( !objectType || !objectType[0] ) {
		gameLocal.Error( "No scriptobject set on '%s'.", dict->GetString("classname") );
	}

	if( !idStr::Icmp(scriptObject.GetTypeName(), objectType) ) {
		//Same script object, don't reload it
		return;
	}

	// setup script object
	if( !scriptObject.SetType(objectType) ) {
		gameLocal.Error( "Script object '%s' not found on weapon '%s'.", objectType, dict->GetString("classname") );
	}

	WEAPON_ATTACK.LinkTo(		scriptObject, "WEAPON_ATTACK" );
	WEAPON_RELOAD.LinkTo(		scriptObject, "WEAPON_RELOAD" );
	WEAPON_RAISEWEAPON.LinkTo(	scriptObject, "WEAPON_RAISEWEAPON" );
	WEAPON_LOWERWEAPON.LinkTo(	scriptObject, "WEAPON_LOWERWEAPON" );
	WEAPON_NEXTATTACK.LinkTo(	scriptObject, "nextAttack" ); //HUMANHEAD rww

	//HUMANHEAD: aob
	WEAPON_ALTATTACK.LinkTo(	scriptObject, "WEAPON_ALTATTACK" );
	WEAPON_ASIDEWEAPON.LinkTo( 	scriptObject, "WEAPON_ASIDEWEAPON" );
	WEAPON_ALTMODE.LinkTo(		scriptObject, "WEAPON_ALTMODE" );
	//HUMANHEAD END

	// call script object's constructor
	ConstructScriptObject();
}

/***********************************************************************

	GUIs

***********************************************************************/

/*
================
hhWeapon::Icon
================
*/
const char *hhWeapon::Icon( void ) const {
	return icon;
}

/*
================
hhWeapon::UpdateGUI
================
*/
void hhWeapon::UpdateGUI() {
	if ( !renderEntity.gui[0] ) {
		return;
	}
	
	if ( status == WP_HOLSTERED ) {
		return;
	}

	int ammoamount = AmmoAvailable();
	int altammoamount = AltAmmoAvailable();

	// show remaining ammo
	renderEntity.gui[ 0 ]->SetStateInt( "ammoamount", ammoamount );
	renderEntity.gui[ 0 ]->SetStateInt( "altammoamount", altammoamount );
	renderEntity.gui[ 0 ]->SetStateBool( "ammolow", ammoamount > 0 && ammoamount <= LowAmmo() );
	renderEntity.gui[ 0 ]->SetStateBool( "altammolow", altammoamount > 0 && altammoamount <= LowAltAmmo() );
	renderEntity.gui[ 0 ]->SetStateBool( "ammoempty", ( ammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateBool( "altammoempty", ( altammoamount == 0 ) );
	renderEntity.gui[ 0 ]->SetStateInt( "clipammoAmount", AmmoInClip() );
	renderEntity.gui[ 0 ]->SetStateInt( "clipaltammoAmount", AltAmmoInClip() );
	renderEntity.gui[ 0 ]->StateChanged(gameLocal.time);
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/
/*
================
hhWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool hhWeapon::GetJointWorldTransform( const char* jointName, idVec3 &offset, idMat3 &axis ) {
	weaponJointHandle_t handle;

	GetJointHandle( jointName, handle );
	return GetJointWorldTransform( handle, offset, axis );
}

/*
================
hhWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool hhWeapon::GetJointWorldTransform( const weaponJointHandle_t& handle, idVec3 &offset, idMat3 &axis, bool muzzleOnly ) {
	//FIXME: this seems to work but may need revisiting
	//FIXME: totally forgot about mirrors and portals.  This may not work.
	if( (!pm_thirdPerson.GetBool() && owner == gameLocal.GetLocalPlayer()) || (gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->spectating && gameLocal.GetLocalPlayer()->spectator == owner->entityNumber) || (gameLocal.isServer && !muzzleOnly)) { //rww - mp server should always create projectiles from the viewmodel
		// view model
		if ( hhAnimatedEntity::GetJointWorldTransform(handle.view, offset, axis) ) {
			return true;
		}
	} else {
		// world model
		if ( worldModel.IsValid() && worldModel.GetEntity()->GetJointWorldTransform(handle.world, offset, axis) ) {
			return true;
		}
	}
	offset = GetOrigin();
	axis = GetAxis();
	return false;
}

/*
================
hhWeapon::GetJointHandle

HUMANHEAD: aob
================
*/
void hhWeapon::GetJointHandle( const char* jointName, weaponJointHandle_t& handle ) {
	if( dict ) {
		handle.view = GetAnimator()->GetJointHandle( jointName );
	}

	if( worldModel.IsValid() ) {
		handle.world = worldModel->GetAnimator()->GetJointHandle( jointName );
	}
}

/*
================
hhWeapon::SetPushVelocity
================
*/
void hhWeapon::SetPushVelocity( const idVec3 &pushVelocity ) {
	this->pushVelocity = pushVelocity;
}


/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
hhWeapon::Raise
================
*/
void hhWeapon::Raise( void ) {
	WEAPON_RAISEWEAPON = true;
	WEAPON_ASIDEWEAPON = false;
}

/*
================
hhWeapon::PutAway
================
*/
void hhWeapon::PutAway( void ) {
	//hasBloodSplat = false;
	WEAPON_LOWERWEAPON = true;
	WEAPON_RAISEWEAPON = false;		//HUMANHEAD bjk PCF 5-4-06 : fix for weapons being up after death
	WEAPON_ASIDEWEAPON = false;
	WEAPON_ATTACK = false;
	WEAPON_ALTATTACK = false;
}


/*
================
hhWeapon::PutAside
================
*/
void hhWeapon::PutAside( void ) {
	//HUMANHEAD bjk PCF (4-27-06) - no setting unnecessary weapon state
	//WEAPON_LOWERWEAPON = false;
	//WEAPON_RAISEWEAPON = false;
	WEAPON_ASIDEWEAPON = true;
	WEAPON_ATTACK = false;
	WEAPON_ALTATTACK = false;
}

/*
================
hhWeapon::PutUpright
================
*/
void hhWeapon::PutUpright( void ) {
	WEAPON_ASIDEWEAPON = false;
}

/*
================
hhWeapon::SnapDown

HUMANHEAD: aob
================
*/
void hhWeapon::SnapDown() {
	ProcessEvent( &EV_Weapon_State, "Down", 0 );
}

/*
================
hhWeapon::SnapUp

HUMANHEAD: aob
================
*/
void hhWeapon::SnapUp() {
	ProcessEvent( &EV_Weapon_State, "Up", 0 );
}

/*
================
hhWeapon::Reload
================
*/
void hhWeapon::Reload( void ) {
	WEAPON_RELOAD = true;
}

/*
================
hhWeapon::HideWeapon
================
*/
void hhWeapon::HideWeapon( void ) {
	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
hhWeapon::ShowWeapon
================
*/
void hhWeapon::ShowWeapon( void ) {
	Show();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
}

/*
================
hhWeapon::HideWorldModel
================
*/
void hhWeapon::HideWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
hhWeapon::ShowWorldModel
================
*/
void hhWeapon::ShowWorldModel( void ) {
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Show();
	}
}

/*
================
hhWeapon::OwnerDied
================
*/
void hhWeapon::OwnerDied( void ) {
	Hide();
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->Hide();
	}
}

/*
================
hhWeapon::BeginAltAttack
================
*/
void hhWeapon::BeginAltAttack( void ) {
	WEAPON_ALTATTACK = true;

	//if ( status != WP_OUTOFAMMO ) {
	//	lastAttack = gameLocal.time;
	//}
}

/*
================
hhWeapon::EndAltAttack
================
*/
void hhWeapon::EndAltAttack( void ) {
	WEAPON_ALTATTACK = false;

}

/*
================
hhWeapon::BeginAttack
================
*/
void hhWeapon::BeginAttack( void ) {
	//if ( status != WP_OUTOFAMMO ) {
	//	lastAttack = gameLocal.time;
	//}

	WEAPON_ATTACK = true;
}

/*
================
hhWeapon::EndAttack
================
*/
void hhWeapon::EndAttack( void ) {
	if ( !WEAPON_ATTACK.IsLinked() ) {
		return;
	}
	if ( WEAPON_ATTACK ) {
		WEAPON_ATTACK = false;
	}
}

/*
================
hhWeapon::isReady
================
*/
bool hhWeapon::IsReady( void ) const {
	return !IsHidden() && ( ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
}

/*
================
hhWeapon::IsReloading
================
*/
bool hhWeapon::IsReloading( void ) const {
	return ( status == WP_RELOAD );
}

/*
================
hhWeapon::IsChangable
================
*/
bool hhWeapon::IsChangable( void ) const {
	//HUMANHEAD: aob - same as IsReady except w/o IsHidden check
	//Allows us to switch weapons while they are hidden
	//Hope this doesn't fuck to many things
	return ( ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
}

/*
================
hhWeapon::IsHolstered
================
*/
bool hhWeapon::IsHolstered( void ) const {
	return ( status == WP_HOLSTERED );
}


/*
================
hhWeapon::IsLowered
================
*/
bool hhWeapon::IsLowered( void ) const {
	return ( status == WP_HOLSTERED ) || ( status == WP_LOWERING );
}


/*
================
hhWeapon::IsRising
================
*/
bool hhWeapon::IsRising( void ) const {
	return ( status == WP_RISING );
}


/*
================
hhWeapon::IsAside
================
*/
bool hhWeapon::IsAside( void ) const {
	return ( status == WP_ASIDE );
}


/*
================
hhWeapon::ShowCrosshair
================
*/
bool hhWeapon::ShowCrosshair( void ) const {
	//HUMANHEAD: aob - added cinematic check
	return !( state == idStr( WP_RISING ) || state == idStr( WP_LOWERING ) || state == idStr( WP_HOLSTERED ) ) && !owner->InCinematic();
}

/*
=====================
hhWeapon::DropItem
=====================
*/
idEntity* hhWeapon::DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died ) {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return NULL;
	}
	
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[0] ) {
		return NULL;
	}
	StopSound( SND_CHANNEL_BODY, !died ); //HUMANHEAD rww - on death, do not broadcast the stop, because the weapon itself is about to be removed
	return idMoveableItem::DropItem( classname, worldModel.GetEntity()->GetPhysics()->GetOrigin(), worldModel.GetEntity()->GetPhysics()->GetAxis(), velocity, activateDelay, removeDelay );
}

/*
=====================
hhWeapon::CanDrop
=====================
*/
bool hhWeapon::CanDrop( void ) const {
	if ( !weaponDef || !worldModel.GetEntity() ) {
		return false;
	}
	const char *classname = weaponDef->dict.GetString( "def_dropItem" );
	if ( !classname[ 0 ] ) {
		return false;
	}
	return true;
}

/***********************************************************************

	Script state management

***********************************************************************/

/*
=====================
hhWeapon::SetState
=====================
*/
void hhWeapon::SetState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
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

	Visual presentation

***********************************************************************/

/*
================
hhWeapon::MuzzleRise

HUMANHEAD: aob
================
*/
void hhWeapon::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
	if( fireController ) {
		fireController->CalculateMuzzleRise( origin, axis );
	}

	if( altFireController ) {
		altFireController->CalculateMuzzleRise( origin, axis );
	}
}

/*
================
hhWeapon::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *hhWeapon::ConstructScriptObject( void ) {
	const function_t *constructor;

	//HUMANHEAD: aob
	if( !thread ) {
		return thread;
	}
	//HUMANHEAD END

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
hhWeapon::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void hhWeapon::DeconstructScriptObject( void ) {
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
hhWeapon::UpdateScript
================
*/
void hhWeapon::UpdateScript( void ) {
	int	count;

	if ( !IsLinked() ) {
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
hhWeapon::PresentWeapon
================
*/
void hhWeapon::PresentWeapon( bool showViewModel ) {
	UpdateScript();

	//HUMANHEAD rww - added this gui owner logic here
	bool allowGuiUpdate = true;
	if ( owner.IsValid() && gameLocal.localClientNum != owner->entityNumber ) {
		// if updating the hud for a followed client
		if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
			if ( !p->spectating || p->spectator != owner->entityNumber ) {
				allowGuiUpdate = false;
			}
		} else {
			allowGuiUpdate = false;
		}
	}
	if (allowGuiUpdate) {
		UpdateGUI();
	}

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
#ifdef HUMANHEAD
		if ( gameLocal.isMultiplayer || (g_showPlayerShadow.GetBool() && pm_modelView.GetInteger() == 1) || pm_thirdPerson.GetBool() 
			|| (pm_modelView.GetInteger()  == 2 && owner->health <= 0)) {
			worldModel->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			worldModel->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
#else
		
		// deal with the third-person visible world model
		// don't show shadows of the world model in first person
		if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel.GetEntity()->GetRenderEntity()->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
		}
	}
#endif	// HUMANHEAD pdm
	}

	// update the muzzle flash light, so it moves with the gun
	//HUMANHEAD: aob
	if( fireController ) {
		fireController->UpdateMuzzleFlash();
	}

	if( altFireController ) {
		altFireController->UpdateMuzzleFlash();
	}
	//HUMANHEAD END

	UpdateNozzleFx();	// expensive

	UpdateSound();
}


/*
================
hhWeapon::EnterCinematic
================
*/
void hhWeapon::EnterCinematic( void ) {
	StopSound( SND_CHANNEL_ANY, false );

	if ( IsLinked() ) {
		SetState( "EnterCinematic", 0 );
		thread->Execute();

		WEAPON_ATTACK		= false;
		WEAPON_RELOAD		= false;
//		WEAPON_NETRELOAD	= false;
//		WEAPON_NETENDRELOAD	= false;
		WEAPON_RAISEWEAPON	= false;
		WEAPON_LOWERWEAPON	= false;
	}

	//disabled = true;

	LowerWeapon();
}

/*
================
hhWeapon::ExitCinematic
================
*/
void hhWeapon::ExitCinematic( void ) {
	//disabled = false;

	if ( IsLinked() ) {
		SetState( "ExitCinematic", 0 );
		thread->Execute();
	}

	RaiseWeapon();
}

/*
================
hhWeapon::NetCatchup
================
*/
void hhWeapon::NetCatchup( void ) {
	if ( IsLinked() ) {
		SetState( "NetCatchup", 0 );
		thread->Execute();
	}
}

/*
================
hhWeapon::GetClipBits
================
*/
int hhWeapon::GetClipBits(void) const {
	return ASYNC_PLAYER_INV_CLIP_BITS;
}

/*
================
hhWeapon::GetZoomFov
================
*/
int	hhWeapon::GetZoomFov( void ) {
	return zoomFov;
}

/*
================
hhWeapon::GetWeaponAngleOffsets
================
*/
void hhWeapon::GetWeaponAngleOffsets( int *average, float *scale, float *max ) {
	*average = weaponAngleOffsetAverages;
	*scale = weaponAngleOffsetScale;
	*max = weaponAngleOffsetMax;
}

/*
================
hhWeapon::GetWeaponTimeOffsets
================
*/
void hhWeapon::GetWeaponTimeOffsets( float *time, float *scale ) {
	*time = weaponOffsetTime;
	*scale = weaponOffsetScale;
}


/***********************************************************************

	Ammo

**********************************************************************/

/*
================
hhWeapon::WriteToSnapshot
================
*/
void hhWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
#if 0
	//FIXME: need to add altFire stuff too
	if( fireController ) {
		msg.WriteBits( fireController->AmmoInClip(), GetClipBits() );
	} else {
		msg.WriteBits( 0, GetClipBits() );
	}
	msg.WriteBits(worldModel.GetSpawnId(), 32);
#else //rww - forget this silliness, let's do this the Right Way.
	msg.WriteBits(owner.GetSpawnId(), 32);
	msg.WriteBits(worldModel.GetSpawnId(), 32);

	if (fireController) {
		fireController->WriteToSnapshot(msg);
	}
	else { //rwwFIXME this is an ugly hack
		msg.WriteBits(0, 32);
		msg.WriteBits(0, 32);
		msg.WriteBits(0, GetClipBits());
	}
	if (altFireController) {
		altFireController->WriteToSnapshot(msg);
	}
	else { //rwwFIXME this is an ugly hack
		msg.WriteBits(0, 32);
		msg.WriteBits(0, 32);
		msg.WriteBits(0, GetClipBits());
	}

	msg.WriteBits(WEAPON_ASIDEWEAPON, 1);
	//HUMANHEAD PCF rww 05/09/06 - no need to sync these values
	/*
	msg.WriteBits(WEAPON_LOWERWEAPON, 1);
	msg.WriteBits(WEAPON_RAISEWEAPON, 1);
	*/

	WriteBindToSnapshot(msg);
#endif
}

/*
================
hhWeapon::ReadFromSnapshot
================
*/
void hhWeapon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
#if 0
	//FIXME: need to add altFire stuff too
	if( fireController ) {
		fireController->AddToClip( msg.ReadBits(GetClipBits()) );
	} else {
		msg.ReadBits( GetClipBits() );
	}

	worldModel.SetSpawnId(msg.ReadBits(32));
#else
	bool wmInit = false;

	owner.SetSpawnId(msg.ReadBits(32));

	if (worldModel.SetSpawnId(msg.ReadBits(32))) { //do this once we finally get the entity in the snapshot
		wmInit = true;
	}

	//rwwFIXME this is a little hacky, i think. is there a way to do it in ::Spawn? but, the weapon doesn't know its owner at that point.
	if (!dict) {
		if (!owner.IsValid() || !owner.GetEntity()) {
			gameLocal.Error("NULL owner in hhWeapon::ReadFromSnapshot!");
		}

		assert(owner.IsValid());

		GetWeaponDef(spawnArgs.GetString("classname"));
		//owner->ForceWeapon(this); it doesn't like this.
	}
	assert(fireController);
	assert(altFireController);

	if (wmInit) { //need to do this once the ent is valid on the client
		InitWorldModel( dict );
		GetJointHandle( weaponDef->dict.GetString( "nozzleJoint", "" ), nozzleJointHandle );
		fireController->UpdateWeaponJoints();
		altFireController->UpdateWeaponJoints();
	}

	fireController->ReadFromSnapshot(msg);
	altFireController->ReadFromSnapshot(msg);

	WEAPON_ASIDEWEAPON = !!msg.ReadBits(1);
	//HUMANHEAD PCF rww 05/09/06 - no need to sync these values
	/*
	WEAPON_LOWERWEAPON = !!msg.ReadBits(1);
	WEAPON_RAISEWEAPON = !!msg.ReadBits(1);
	*/

	ReadBindFromSnapshot(msg);
#endif
}

/*
================
hhWeapon::ClientPredictionThink
================
*/
void hhWeapon::ClientPredictionThink( void ) {
	Think();
}

/***********************************************************************

	Script events

***********************************************************************/

/*
hhWeapon::Event_Raise
*/
void hhWeapon::Event_Raise() {
	Raise();
}


/*
===============
hhWeapon::Event_WeaponState
===============
*/
void hhWeapon::Event_WeaponState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	idealState = statename;
	animBlendFrames = blendFrames;
	thread->DoneProcessing();
}

/*
===============
hhWeapon::Event_WeaponReady
===============
*/
void hhWeapon::Event_WeaponReady( void ) {
	status = WP_READY;
	WEAPON_RAISEWEAPON = false;

	//HUMANHEAD bjk PCF (4-27-06) - no setting unnecessary weapon state
	//WEAPON_LOWERWEAPON = false;
	//WEAPON_ASIDEWEAPON = false;
}

/*
===============
hhWeapon::Event_WeaponOutOfAmmo
===============
*/
void hhWeapon::Event_WeaponOutOfAmmo( void ) {
	status = WP_OUTOFAMMO;
	WEAPON_RAISEWEAPON = false;

	//HUMANHEAD bjk PCF (4-27-06) - no setting unnecessary weapon state
	//WEAPON_LOWERWEAPON = false;
	//WEAPON_ASIDEWEAPON = false;
}

/*
===============
hhWeapon::Event_WeaponReloading
===============
*/
void hhWeapon::Event_WeaponReloading( void ) {
	status = WP_RELOAD;
}

/*
===============
hhWeapon::Event_WeaponHolstered
===============
*/
void hhWeapon::Event_WeaponHolstered( void ) {
	status = WP_HOLSTERED;
	WEAPON_LOWERWEAPON = false;
}

/*
===============
hhWeapon::Event_WeaponRising
===============
*/
void hhWeapon::Event_WeaponRising( void ) {
	status = WP_RISING;
	WEAPON_LOWERWEAPON = false;

	//HUMANHEAD bjk PCF (4-27-06) - no setting unnecessary weapon state
	//WEAPON_ASIDEWEAPON = false;

	owner->WeaponRisingCallback();
}


/*
===============
hhWeapon::Event_WeaponAside
===============
*/
void hhWeapon::Event_Weapon_Aside( void ) {
	status = WP_ASIDE;
}

/*
===============
hhWeapon::Event_Weapon_PuttingAside
===============
*/
void hhWeapon::Event_Weapon_PuttingAside( void ) {
	status = WP_PUTTING_ASIDE;
}

/*
===============
hhWeapon::Event_Weapon_Uprighting
===============
*/
void hhWeapon::Event_Weapon_Uprighting( void ) {
	status = WP_UPRIGHTING;
}

/*
===============
hhWeapon::Event_WeaponLowering
===============
*/
void hhWeapon::Event_WeaponLowering( void ) {
	status = WP_LOWERING;
	WEAPON_RAISEWEAPON = false;

	//HUMANHEAD: aob
	WEAPON_ASIDEWEAPON = false;
	//HUMANHEAD END

	owner->WeaponLoweringCallback();
}

/*
===============
hhWeapon::Event_AddToClip
===============
*/
void hhWeapon::Event_AddToClip( int amount ) {
	if( fireController ) {
		fireController->AddToClip( amount );
	}
}

/*
===============
hhWeapon::Event_AmmoInClip
===============
*/
void hhWeapon::Event_AmmoInClip( void ) {
	int ammo = AmmoInClip();
	idThread::ReturnFloat( ammo );	
}

/*
===============
hhWeapon::Event_AmmoAvailable
===============
*/
void hhWeapon::Event_AmmoAvailable( void ) {
	int ammoAvail = AmmoAvailable();
	idThread::ReturnFloat( ammoAvail );
}

/*
===============
hhWeapon::Event_ClipSize
===============
*/
void hhWeapon::Event_ClipSize( void ) {
	idThread::ReturnFloat( ClipSize() );	
}

/*
===============
hhWeapon::Event_PlayAnim
===============
*/
void hhWeapon::Event_PlayAnim( int channel, const char *animname ) {
	int anim = 0;
	
	anim = GetAnimator()->GetAnim( animname );
	if ( !anim ) {
		//HUMANHEAD: aob
		WEAPON_DEBUG( "missing '%s' animation on '%s'", animname, name.c_str() );
		//HUMANHEAD END
		GetAnimator()->Clear( channel, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		//Show(); AOB - removed so we can use hidden weapons  Hope this doesn't fuck to many things
		GetAnimator()->PlayAnim( channel, anim, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
		animDoneTime = GetAnimator()->CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			if ( anim ) {
				worldModel.GetEntity()->GetAnimator()->PlayAnim( channel, anim, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
			}
		}
	}
	animBlendFrames = 0;
}

/*
===============
hhWeapon::Event_AnimDone
===============
*/
void hhWeapon::Event_AnimDone( int channel, int blendFrames ) {
	if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
================
hhWeapon::Event_Next
================
*/
void hhWeapon::Event_Next( void ) {
	// change to another weapon if possible
	owner->NextBestWeapon();
}

/*
================
hhWeapon::Event_Flashlight
================
*/
void hhWeapon::Event_Flashlight( int enable ) {
/*
	if ( enable ) {
		lightOn = true;
		MuzzleFlashLight();
	} else {
		lightOn = false;
		muzzleFlashEnd = 0;
	}
*/
}
/*
================
hhWeapon::Event_FireProjectiles

HUMANHEAD: aob
================
*/
void hhWeapon::Event_FireProjectiles() {
	//HUMANHEAD: aob - moved logic to this helper function
	LaunchProjectiles( fireController );
	//HUMANHEAD END
}

/*
================
hhWeapon::Event_GetOwner
================
*/
void hhWeapon::Event_GetOwner( void ) {
	idThread::ReturnEntity( owner.GetEntity() );
}

/*
===============
hhWeapon::Event_UseAmmo
===============
*/
void hhWeapon::Event_UseAmmo( int amount ) {
	UseAmmo();
}

/*
===============
hhWeapon::Event_UseAltAmmo
===============
*/
void hhWeapon::Event_UseAltAmmo( int amount ) {
	UseAltAmmo();
}

/*
================
hhWeapon::RestoreGUI

HUMANHEAD: aob
================
*/
void hhWeapon::RestoreGUI( const char* guiKey, idUserInterface** gui ) {
	if( guiKey && guiKey[0] && gui ) {
		AddRenderGui( dict->GetString(guiKey), gui, dict );
	}
}

/*
================
hhWeapon::Think
================
*/
void hhWeapon::Think() {
	if( thinkFlags & TH_TICKER ) {
		if (owner.IsValid()) {
			Ticker();
		}
	}
}

/*
================
hhWeapon::SpawnWorldModel
================
*/
idEntity* hhWeapon::SpawnWorldModel( const char* worldModelDict, idActor* _owner ) {
/*
	assert( _owner && worldModelDict );

	idEntity*	pEntity = NULL;
	idDict* pArgs = declManager->FindEntityDefDict( worldModelDict, false );
	if( !pArgs ) { return NULL; }

	idStr ownerWeaponBindBone = _owner->spawnArgs.GetString( "bone_weapon_bind" );
	idStr attachBone = pArgs->GetString( "attach" );
	if( attachBone.Length() && ownerWeaponBindBone.Length() ) {
		pEntity = gameLocal.SpawnEntityType( idEntity::Type, pArgs );
		HH_ASSERT( pEntity );

		pEntity->GetPhysics()->SetContents( 0 );
		pEntity->GetPhysics()->DisableClip();
		pEntity->MoveJointToJoint( attachBone.c_str(), _owner, ownerWeaponBindBone.c_str() );
		pEntity->AlignJointToJoint( attachBone.c_str(), _owner, ownerWeaponBindBone.c_str() );
		pEntity->BindToJoint( _owner, ownerWeaponBindBone.c_str(), true );

		// supress model in player views and in any views on the weapon itself
		pEntity->renderEntity.suppressSurfaceInViewID = _owner->entityNumber + 1;
	}
*/
	return NULL;
}

/*
================
hhWeapon::SetModel
================
*/
void hhWeapon::SetModel( const char *modelname ) {
	assert( modelname );

	if ( modelDefHandle >= 0 ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
	}

	hhAnimatedEntity::SetModel( modelname );

	// hide the model until an animation is played
	Hide();
}

/*
================
hhWeapon::FreeModelDef
================
*/
void hhWeapon::FreeModelDef() {
	hhAnimatedEntity::FreeModelDef();
}

/*
================
hhWeapon::GetPhysicsToVisualTransform
================
*/
bool hhWeapon::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	bool bResult = hhAnimatedEntity::GetPhysicsToVisualTransform( origin, axis );

	assert(!FLOAT_IS_NAN(cameraShakeOffset.x));
	assert(!FLOAT_IS_NAN(cameraShakeOffset.y));
	assert(!FLOAT_IS_NAN(cameraShakeOffset.z));

	if( !cameraShakeOffset.Compare(vec3_zero, VECTOR_EPSILON) ) {
		origin = (bResult) ? cameraShakeOffset + origin : cameraShakeOffset;
		origin *= GetPhysics()->GetAxis().Transpose();
		if( !bResult ) { axis = mat3_identity; }
		return true;
	}

	return bResult;
}

/*
================
hhWeapon::GetMasterDefaultPosition
================
*/
void hhWeapon::GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	idActor*	actor = NULL;
	idEntity*	master = GetBindMaster();

	if( master ) {
		if( master->IsType(idActor::Type) ) {
			actor = static_cast<idActor*>( master );
			actor->DetermineOwnerPosition( masterOrigin, masterAxis );

			masterOrigin = actor->ApplyLandDeflect( masterOrigin, 1.1f );
		} else {
			hhAnimatedEntity::GetMasterDefaultPosition( masterOrigin, masterAxis );
		}
	}
}

/*
================
hhWeapon::SetShaderParm
================
*/
void hhWeapon::SetShaderParm( int parmnum, float value ) {
	hhAnimatedEntity::SetShaderParm(parmnum, value);
	if ( worldModel.IsValid() ) {
		worldModel->SetShaderParm(parmnum, value);
	}
}

/*
================
hhWeapon::SetSkin
================
*/
void hhWeapon::SetSkin( const idDeclSkin *skin ) {
	hhAnimatedEntity::SetSkin(skin);
	if ( worldModel.IsValid() ) {
		worldModel->SetSkin(skin);
	}
}

/*
================
hhWeapon::PlayCycle
================
*/
void hhWeapon::Event_PlayCycle( int channel, const char *animname ) {
	int anim;

	anim = GetAnimator()->GetAnim( animname );
	if ( !anim ) {
		//HUMANHEAD: aob
		WEAPON_DEBUG( "missing '%s' animation on '%s'", animname, name.c_str() );
		//HUMANHEAD END
		GetAnimator()->Clear( channel, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
		animDoneTime = 0;
	} else {
		//Show();AOB - removed so we can use hidden weapons  Hope this doesn't fuck to many things

		// NLANOTE - Used to be CARandom
		GetAnimator()->CycleAnim( channel, anim, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
		animDoneTime = GetAnimator()->CurrentAnim( channel )->GetEndTime();
		if ( worldModel.GetEntity() ) {
			anim = worldModel.GetEntity()->GetAnimator()->GetAnim( animname );
			// NLANOTE - Used to be CARandom
			worldModel.GetEntity()->GetAnimator()->CycleAnim( channel, anim, gameLocal.GetTime(), FRAME2MS( animBlendFrames ) );
		}
	}
	animBlendFrames = 0;
}

/*
================
hhWeapon::Event_IsAnimPlaying
================
*/
void hhWeapon::Event_IsAnimPlaying( const char *animname ) {
	idThread::ReturnInt( animator.IsAnimPlaying(animname) );
}


/*
================
hhWeapon::Event_EjectBrass
================
*/
void hhWeapon::Event_EjectBrass() {
	if( fireController ) {
		fireController->EjectBrass();
	}
}

/*
================
hhWeapon::Event_EjectAltBrass
================
*/
void hhWeapon::Event_EjectAltBrass() {
	if( altFireController ) {
		altFireController->EjectBrass();
	}
}

/*
================
hhWeapon::Event_PlayAnimWhenReady
================
*/
void hhWeapon::Event_PlayAnimWhenReady( const char* animName ) {
/*
	if( !IsReady() && (!dict || !dict->GetBool("pickupHasRaise") || !IsRaising()) ) {
		CancelEvents( &EV_PlayAnimWhenReady );
		PostEventSec( &EV_PlayAnimWhenReady, 0.5f, animName );
		return;
	}

	PlayAnim( animName, 0, &EV_Weapon_Ready );
*/
}

/*
================
hhWeapon::Event_SpawnFXAlongBone
================
*/
void hhWeapon::Event_SpawnFXAlongBone( idList<idStr>* fxParms ) {
	if ( !owner->CanShowWeaponViewmodel() || !fxParms ) {
		return;
	}

	HH_ASSERT( fxParms->Num() == 2 );

	hhFxInfo fxInfo;
	fxInfo.UseWeaponDepthHack( true );
	BroadcastFxInfoAlongBone( dict->GetString((*fxParms)[0].c_str()), (*fxParms)[1].c_str(), &fxInfo, NULL, false ); //rww - default to not broadcast from events
}

/*
==============================
hhWeapon::Event_FireAltProjectiles
==============================
*/
void hhWeapon::Event_FireAltProjectiles() {
	LaunchProjectiles( altFireController );
}

// This is called once per frame to precompute where the weapon is pointing.  This is used for determining if crosshairs should display as targetted as
// well as fire controllers to update themselves.  This must be called before UpdateCrosshairs().
void hhWeapon::PrecomputeTraceInfo() {
	// This is needed for fireControllers, even if there are no crosshairs
	idVec3 eyePos = owner->GetEyePosition();
	idMat3 weaponAxis = GetAxis();
	float traceDist = 1024.0f;	// was CM_MAX_TRACE_DIST

	// Perform eye trace
	gameLocal.clip.TracePoint( eyeTraceInfo, eyePos, eyePos + weaponAxis[0] * traceDist, 
		MASK_SHOT_BOUNDINGBOX | CONTENTS_GAME_PORTAL, owner.GetEntity() );

	// CJR: If the trace hit a portal, then force it to trace the max distance, as if it's tracing through the portal
	if ( eyeTraceInfo.fraction < 1.0f ) {
		idEntity *ent = gameLocal.GetTraceEntity( eyeTraceInfo );
		if ( ent->IsType( hhPortal::Type ) ) {
			eyeTraceInfo.endpos = eyePos + weaponAxis[0] * traceDist;
			eyeTraceInfo.fraction = 1.0f;
		}
	}

}

/*
==============================
hhWeapon::UpdateCrosshairs
==============================
*/
void hhWeapon::UpdateCrosshairs( bool &crosshair, bool &targeting ) {
	idEntity*			ent = NULL;
	trace_t				traceInfo;

	crosshair = false;
	targeting = false;
	if (spawnArgs.GetBool("altModeWeapon")) {
		if (WEAPON_ALTMODE) {	// Moded weapon in alt-mode
			if (altFireController) {
				crosshair = altFireController->UsesCrosshair();
			}
		}
		else {					// Moded weapon in normal mode
			if (fireController) {
				crosshair = fireController->UsesCrosshair();
			}
		}
	}
	else {						// Normal non-moded weapon
		if (altFireController && altFireController->UsesCrosshair()) {
			crosshair = true;
		}
		if (fireController && fireController->UsesCrosshair()) {
			crosshair = true;
		}
	}

	ent = NULL;
	if( crosshair ) {
		traceInfo = GetEyeTraceInfo();
		if( traceInfo.fraction < 1.0f ) {
			ent = gameLocal.GetTraceEntity(traceInfo);
		}
		targeting = ( ent && ent->fl.takedamage && !(ent->IsType( hhDoor::Type ) || ent->IsType( hhModelDoor::Type ) || ent->IsType( hhConsole::Type ) ) );
	}
}

/*
==============================
hhWeapon::GetAmmoType
==============================
*/
ammo_t hhWeapon::GetAmmoType( void ) const {
	return (fireController) ? fireController->GetAmmoType() : 0;
}

/*
==============================
hhWeapon::AmmoAvailable
==============================
*/
int	hhWeapon::AmmoAvailable( void ) const {
	return (fireController) ? fireController->AmmoAvailable() : 0;
}

/*
==============================
hhWeapon::AmmoInClip
==============================
*/
int	hhWeapon::AmmoInClip( void ) const {
	return (fireController) ? fireController->AmmoInClip() : 0;
}

/*
==============================
hhWeapon::ClipSize
==============================
*/
int	hhWeapon::ClipSize( void ) const {
	return (fireController) ? fireController->ClipSize() : 0;
}

/*
==============================
hhWeapon::AmmoRequired
==============================
*/
int	hhWeapon::AmmoRequired( void ) const {
	return (fireController) ? fireController->AmmoRequired() : 0;
}

/*
==============================
hhWeapon::LowAmmo
==============================
*/
int hhWeapon::LowAmmo() {
	return (fireController) ? fireController->LowAmmo() : 0;
}

/*
==============================
hhWeapon::GetAltAmmoType
==============================
*/
ammo_t hhWeapon::GetAltAmmoType( void ) const {
	return (altFireController) ? altFireController->GetAmmoType() : 0;
}

/*
==============================
hhWeapon::AltAmmoAvailable
==============================
*/
int	hhWeapon::AltAmmoAvailable( void ) const {
	return (altFireController) ? altFireController->AmmoAvailable() : 0;
}

/*
==============================
hhWeapon::AltAmmoInClip
==============================
*/
int	hhWeapon::AltAmmoInClip( void ) const {
	return (altFireController) ? altFireController->AmmoInClip() : 0;
}

/*
==============================
hhWeapon::AltClipSize
==============================
*/
int	hhWeapon::AltClipSize( void ) const {
	return (altFireController) ? altFireController->ClipSize() : 0;
}

/*
==============================
hhWeapon::AltAmmoRequired
==============================
*/
int	hhWeapon::AltAmmoRequired( void ) const {
	return (altFireController) ? altFireController->AmmoRequired() : 0;
}

/*
==============================
hhWeapon::LowAltAmmo
==============================
*/
int hhWeapon::LowAltAmmo() {
	return (altFireController) ? altFireController->LowAmmo() : 0;
}

/*
==============================
hhWeapon::GetAltMode
HUMANHEAD bjk
==============================
*/
bool hhWeapon::GetAltMode() const {
	return WEAPON_ALTMODE != 0;
	
}

/*
==============================
hhWeapon::UseAmmo
==============================
*/
void hhWeapon::UseAmmo() {
	if (fireController) {
		fireController->UseAmmo();
	}
}

/*
==============================
hhWeapon::UseAltAmmo
==============================
*/
void hhWeapon::UseAltAmmo() {
	if (altFireController) {
		altFireController->UseAmmo();
	}
}

/*
==============================
hhWeapon::CheckDeferredProjectiles

HUMANEAD: rww
==============================
*/
void hhWeapon::CheckDeferredProjectiles(void) {
	if (fireController) {
		fireController->CheckDeferredProjectiles();
	}
	if (altFireController) {
		altFireController->CheckDeferredProjectiles();
	}
}

/*
==============================
hhWeapon::LaunchProjectiles

HUMANEAD: aob
==============================
*/
void hhWeapon::LaunchProjectiles( hhWeaponFireController* controller ) {
	if ( IsHidden() ) {
		return;
	}

	// wake up nearby monsters
	if ( !spawnArgs.GetBool( "silent_fire" ) ) {
		gameLocal.AlertAI( owner.GetEntity() );
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	SetShaderParm( SHADERPARM_DIVERSITY, gameLocal.random.CRandomFloat() );
	SetShaderParm( SHADERPARM_TIMEOFFSET, -MS2SEC( gameLocal.realClientTime ) );

	float low = ((float)controller->AmmoAvailable()*controller->AmmoRequired())/controller->LowAmmo();

	if( !controller->LaunchProjectiles(pushVelocity) ) {
		return;
	}

	if (!gameLocal.isClient) { //HUMANHEAD rww - let everyone hear this sound, and broadcast it (so don't try to play it for client-projectile weapons)
		if( controller->AmmoAvailable()*controller->AmmoRequired() <= controller->LowAmmo() && low > 1 && spawnArgs.FindKey("snd_lowammo")) {
			StartSound( "snd_lowammo", SND_CHANNEL_ANY, 0, true, NULL );
		}
	}

	controller->UpdateMuzzleKick();

	// add the light for the muzzleflash
	controller->MuzzleFlash();

	//HUMANEHAD: aob
	controller->WeaponFeedback();
	//HUMANHEAD END
}

/*
===============
hhWeapon::SetViewAnglesSensitivity

HUMANHEAD: aob
===============
*/
void hhWeapon::SetViewAnglesSensitivity( float fov ) {
	if( owner.IsValid() ) {
		owner->SetViewAnglesSensitivity( fov / g_fov.GetFloat() );
	}
}

/*
===============
hhWeapon::GetDict

HUMANHEAD: aob
===============
*/
const idDict* hhWeapon::GetDict( const char* objectname ) {
	return gameLocal.FindEntityDefDict( objectname, false );
}

/*
===============
hhWeapon::GetFireInfoDict

HUMANHEAD: aob
===============
*/
const idDict* hhWeapon::GetFireInfoDict( const char* objectname ) {
	const idDict* dict = GetDict( objectname );
	if( !dict ) {
		return NULL;
	}

	return gameLocal.FindEntityDefDict( dict->GetString("def_fireInfo"), false );
}

/*
===============
hhWeapon::GetAltFireInfoDict

HUMANHEAD: aob
===============
*/
const idDict* hhWeapon::GetAltFireInfoDict( const char* objectname ) {
	const idDict* dict = GetDict( objectname );
	if( !dict ) {
		return NULL;
	}

	return gameLocal.FindEntityDefDict( dict->GetString("def_altFireInfo"), false );
}

void hhWeapon::Event_GetString(const char *key) {
	if ( fireController ) {
		idThread::ReturnString( fireController->GetString(key) );
	}
}

void hhWeapon::Event_GetAltString(const char *key) {
	if ( altFireController ) {
		idThread::ReturnString( altFireController->GetString(key) );
	}
}

/*
===============
hhWeapon::Event_AddToAltClip
===============
*/
void hhWeapon::Event_AddToAltClip( int amount ) {
	HH_ASSERT( altFireController );
		
	altFireController->AddToClip( amount ); 
}

/*
===============
hhWeapon::Event_AmmoInClip
===============
*/
void hhWeapon::Event_AltAmmoInClip( void ) {
	HH_ASSERT( altFireController );

	idThread::ReturnFloat( altFireController->AmmoInClip() );	
}

/*
===============
hhWeapon::Event_AltAmmoAvailable
===============
*/
void hhWeapon::Event_AltAmmoAvailable( void ) {
	HH_ASSERT( altFireController );

	idThread::ReturnFloat( altFireController->AmmoAvailable() );
}

/*
===============
hhWeapon::Event_ClipSize
===============
*/
void hhWeapon::Event_AltClipSize( void ) {
	HH_ASSERT( altFireController );

	idThread::ReturnFloat( altFireController->ClipSize() );	
}

/*
==============================
hhWeapon::Event_GetFireDelay
==============================
*/
void hhWeapon::Event_GetFireDelay() {
	HH_ASSERT( fireController );

	idThread::ReturnFloat( fireController->GetFireDelay() );
}

/*
==============================
hhWeapon::Event_GetAltFireDelay
==============================
*/
void hhWeapon::Event_GetAltFireDelay() {
	HH_ASSERT( altFireController );

	idThread::ReturnFloat( altFireController->GetFireDelay() );
}

/*
==============================
hhWeapon::Event_HasAmmo
==============================
*/
void hhWeapon::Event_HasAmmo() {
	idThread::ReturnInt( fireController->HasAmmo() );
}

/*
==============================
hhWeapon::Event_HasAltAmmo
==============================
*/
void hhWeapon::Event_HasAltAmmo() {
	idThread::ReturnInt( altFireController->HasAmmo() );
}

/*
===============
hhWeapon::Event_SetViewAnglesSensitivity

HUMANHEAD: aob
===============
*/
void hhWeapon::Event_SetViewAnglesSensitivity( float fov ) {
	SetViewAnglesSensitivity( fov );
}

/*
================
hhWeapon::UpdateNozzleFx
================
*/
void hhWeapon::UpdateNozzleFx( void ) {

	if ( !nozzleFx || !g_projectileLights.GetBool()) {
		return;
	}

	if ( nozzleJointHandle.view == INVALID_JOINT ) {
		return;
	}

	//
	// vent light
	//
	if ( nozzleGlowHandle == -1 ) {
		memset(&nozzleGlow, 0, sizeof(nozzleGlow));
		if ( owner.IsValid() ) {
			nozzleGlow.allowLightInViewID = owner->entityNumber+1;
		}
		nozzleGlow.pointLight = true;
		nozzleGlow.noShadows = true;
		nozzleGlow.lightRadius.x = nozzleGlowRadius;
		nozzleGlow.lightRadius.y = nozzleGlowRadius;
		nozzleGlow.lightRadius.z = nozzleGlowRadius;
		nozzleGlow.shader = nozzleGlowShader;
		GetJointWorldTransform( nozzleJointHandle, nozzleGlow.origin, nozzleGlow.axis );

		nozzleGlow.origin += nozzleGlowOffset * nozzleGlow.axis;
		nozzleGlowHandle = gameRenderWorld->AddLightDef(&nozzleGlow);
	}

	GetJointWorldTransform(nozzleJointHandle, nozzleGlow.origin, nozzleGlow.axis );
	nozzleGlow.origin += nozzleGlowOffset * nozzleGlow.axis;

	nozzleGlow.shaderParms[ SHADERPARM_RED ] = nozzleGlowColor.x;
	nozzleGlow.shaderParms[ SHADERPARM_GREEN ] = nozzleGlowColor.y;
	nozzleGlow.shaderParms[ SHADERPARM_BLUE ] = nozzleGlowColor.z;

	// Copy parms from the weapon into this light
	for( int i = 4; i < 8; i++ ) {
		nozzleGlow.shaderParms[ i ]	= GetRenderEntity()->shaderParms[ i ];
	}

	gameRenderWorld->UpdateLightDef(nozzleGlowHandle, &nozzleGlow);
}

void hhWeapon::Event_HideWeapon(void) {
	HideWeapon();
}

void hhWeapon::Event_ShowWeapon(void) {
	ShowWeapon();
}

/*
===============
hhWeapon::FillDebugVars
===============
*/
void hhWeapon::FillDebugVars(idDict *args, int page) {
	idStr text;

	switch(page) {
	case 1:
		args->SetBool("WEAPON_LOWERWEAPON", WEAPON_LOWERWEAPON != 0);
		args->SetBool("WEAPON_RAISEWEAPON", WEAPON_RAISEWEAPON != 0);
		args->SetBool("WEAPON_ASIDEWEAPON", WEAPON_ASIDEWEAPON != 0);
		args->SetBool("WEAPON_ATTACK", WEAPON_ATTACK != 0);
		args->SetBool("WEAPON_ALTATTACK", WEAPON_ALTATTACK != 0);
		args->SetBool("WEAPON_ALTMODE", WEAPON_ALTMODE != 0);
		args->SetBool("WEAPON_RELOAD", WEAPON_RELOAD != 0);
		break;
	}
}
