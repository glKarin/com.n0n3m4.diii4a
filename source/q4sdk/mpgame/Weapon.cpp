// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#include "Weapon.h"
#include "Projectile.h"
#include "ai/AI.h"
#include "ai/AI_Manager.h"
#include "client/ClientEffect.h"
//#include "../renderer/tr_local.h"

/***********************************************************************

  rvViewWeapon  
	
***********************************************************************/

// class def
CLASS_DECLARATION( idAnimatedEntity, rvViewWeapon )
	EVENT( EV_CallFunction,		rvViewWeapon::Event_CallFunction )
END_CLASS

/***********************************************************************

	init

***********************************************************************/

/*
================
rvViewWeapon::rvViewWeapon()
================
*/
rvViewWeapon::rvViewWeapon() {
	modelDefHandle		= -1;
	weapon				= NULL;
		
	Clear();

	fl.networkSync = true;	
}

/*
================
rvViewWeapon::~rvViewWeapon()
================
*/
rvViewWeapon::~rvViewWeapon() {
	Clear();
}

/*
================
rvViewWeapon::Spawn
================
*/
void rvViewWeapon::Spawn( void ) {
	GetPhysics()->SetContents( 0 );
	GetPhysics()->SetClipMask( 0 );
	GetPhysics()->SetClipModel( NULL, 1.0f );
}

/*
================
rvViewWeapon::Save
================
*/
void rvViewWeapon::Save( idSaveGame *savefile ) const {
	int i;
	savefile->WriteInt ( pendingGUIEvents.Num() );
	for ( i = 0; i < pendingGUIEvents.Num(); i ++ ) {
		savefile->WriteString ( pendingGUIEvents[i] );
	}

	// TOSAVE: const idDeclSkin *		saveSkin;
	// TOSAVE: const idDeclSkin *		invisSkin;
	// TOSAVE: const idDeclSkin *		saveWorldSkin;
	// TOSAVE: const idDeclSkin *		worldInvisSkin;
	// TOSAVE: const idDeclSkin *		saveHandsSkin;
	// TOSAVE: const idDeclSkin *		handsSkin;

	// TOSAVE: friend		rvWeapon;
	// TOSAVE: rvWeapon*	weapon;
}

/*
================
rvViewWeapon::Restore
================
*/
void rvViewWeapon::Restore( idRestoreGame *savefile ) {
	int i;
	int num;
	savefile->ReadInt ( num );
	pendingGUIEvents.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		savefile->ReadString ( pendingGUIEvents[i] );
	}
}

/*
===============
rvViewWeapon::ClientPredictionThink
===============
*/
void rvViewWeapon::ClientPredictionThink( void ) {
	UpdateAnimation();	
}

/***********************************************************************

	Weapon definition management

***********************************************************************/

/*
================
rvViewWeapon::Clear
================
*/
void rvViewWeapon::Clear( void ) {
	DeconstructScriptObject();
	scriptObject.Free();

	StopAllEffects( );

	// TTimo - the weapon doesn't get a proper Event_DisableWeapon sometimes, so the sound sticks
	// typically, client side instance join in tourney mode just wipes all ents
	StopSound( SND_CHANNEL_ANY, false );

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

	memset( &refSound, 0, sizeof( refSound_t ) );
	refSound.referenceSoundHandle = -1;
	
 	// setting diversity to 0 results in no random sound.  -1 indicates random.
 	refSound.diversity = -1.0f;
	
	if ( weapon && weapon->GetOwner ( ) ) {
		// don't spatialize the weapon sounds
		refSound.listenerId = weapon->GetOwner( )->GetListenerId();
	}

 	animator.ClearAllAnims( gameLocal.time, 0 );

	FreeModelDef();
}

/*
=====================
rvViewWeapon::GetDebugInfo
=====================
*/
void rvViewWeapon::GetDebugInfo( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAnimatedEntity::GetDebugInfo( proc, userData );
	weapon->GetDebugInfo( proc, userData );
}

/***********************************************************************

	GUIs

***********************************************************************/

/*
================
rvViewWeapon::PostGUIEvent
================
*/
void rvViewWeapon::PostGUIEvent( const char* event ) {
	pendingGUIEvents.Append ( event );
}

/***********************************************************************

	Model and muzzleflash

***********************************************************************/

/*
================
rvViewWeapon::SetPowerUpSkin
================
*/
void rvViewWeapon::SetPowerUpSkin( const char *name ) {
/* FIXME
	saveSkin = renderEntity.customSkin;
	renderEntity.customSkin = invisSkin;
	if ( worldModel.GetEntity() ) {
		saveWorldSkin = worldModel.GetEntity()->GetSkin();
		worldModel.GetEntity()->SetSkin( worldInvisSkin );
	}
*/
}

/*
================
rvViewWeapon::UpdateSkin
================
*/
void rvViewWeapon::UpdateSkin( void ) {
/* FIXME
	renderEntity.customSkin = saveSkin;
	if ( worldModel.GetEntity() ) {
		worldModel.GetEntity()->SetSkin( saveWorldSkin );
	}
*/
}
	
/*
================
rvViewWeapon::SetModel
================
*/
void rvViewWeapon::SetModel( const char *modelname, int mods ) {
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

/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
rvViewWeapon::Think
================
*/
void rvViewWeapon::Think( void ) {
	// do nothing because the present is called from the player through PresentWeapon
}

/*
=====================
rvViewWeapon::ConvertLocalToWorldTransform
=====================
*/
void rvViewWeapon::ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis ) {
	if( !weapon ) {
		idAnimatedEntity::ConvertLocalToWorldTransform( offset, axis );
		return;
	}

	offset = GetPhysics()->GetOrigin() + offset * weapon->ForeshortenAxis( GetPhysics()->GetAxis() );
	axis *= GetPhysics()->GetAxis();
}

/*
================
rvViewWeapon::UpdateModelTransform
================
*/
void rvViewWeapon::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if( !weapon ) {
		idAnimatedEntity::UpdateModelTransform();
		return;
	}

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * weapon->ForeshortenAxis( GetPhysics()->GetAxis() );
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = weapon->ForeshortenAxis( GetPhysics()->GetAxis() );
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
================
rvViewWeapon::PresentWeapon
================
*/
void rvViewWeapon::PresentWeapon( bool showViewModel ) {
	// Dont do anything with the weapon while its stale
	if ( fl.networkStale ) {
		return;
	}

// RAVEN BEGIN
// rjohnson: cinematics should never be done from the player's perspective, so don't think the weapon ( and their sounds! )
	if ( gameLocal.inCinematic ) {
		return;
	}
// RAVEN END

	// only show the surface in player view
	renderEntity.allowSurfaceInViewID = weapon->GetOwner()->entityNumber + 1;

	// crunch the depth range so it never pokes into walls this breaks the machine gun gui
	renderEntity.weaponDepthHackInViewID = weapon->GetOwner()->entityNumber + 1;

	weapon->Think();

	// present the model
	if ( showViewModel && !(weapon->wsfl.zoom && weapon->GetZoomGui() ) ) {
		Present();
	} else {
		FreeModelDef();
	}

	UpdateSound();
}

/*
================
rvViewWeapon::WriteToSnapshot
================
*/
void rvViewWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
rvViewWeapon::ReadFromSnapshot
================
*/
void rvViewWeapon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
rvViewWeapon::ClientStale
================
*/
bool rvViewWeapon::ClientStale ( void ) {
	StopSound( SND_CHANNEL_ANY, false );
	
	if ( weapon ) {
		weapon->ClientStale( );
	}
	
	idEntity::ClientStale( );

	return false;
}

/*
================
rvViewWeapon::ClientReceiveEvent
================
*/
bool rvViewWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	if ( idEntity::ClientReceiveEvent( event, time, msg ) ) {
		return true;
	}
	if ( weapon ) {
		return weapon->ClientReceiveEvent ( event, time, msg );
	}
	return false;
}

/***********************************************************************

	Script events

***********************************************************************/

/*
=====================
rvViewWeapon::Event_CallFunction
=====================
*/
void rvViewWeapon::Event_CallFunction( const char *funcname ) {
	if ( weapon ) {
		stateParms_t parms = {0};
		if ( weapon->ProcessState ( funcname, parms ) == SRESULT_ERROR ) {
			gameLocal.Error ( "Unknown function '%s' on entity '%s'", funcname, GetName() );
		}
	}
}

/*
================
rvViewWeapon::SetSkin
================
*/
void rvViewWeapon::SetSkin( const char *skinname ) {
 	const idDeclSkin *skinDecl;
 
 	if ( !skinname || !skinname[ 0 ] ) {
 		skinDecl = NULL;
 	} else {
 		skinDecl = declManager->FindSkin( skinname );
 	}
 
 	renderEntity.customSkin = skinDecl;
	UpdateVisuals();

	// Set the skin on the world model as well
	if ( weapon->GetWorldModel() ) {
 		weapon->GetWorldModel()->SetSkin( skinDecl );
	}
}

void rvViewWeapon::SetSkin( const idDeclSkin* skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();

	if( weapon && weapon->GetWorldModel() ) {
		weapon->GetWorldModel()->SetSkin( skin );
	}
}

/*
================
rvViewWeapon::GetPosition
================
*/
void rvViewWeapon::GetPosition( idVec3& origin, idMat3& axis ) const {
	origin = GetPhysics()->GetOrigin();
	axis = GetPhysics()->GetAxis();
}

void rvViewWeapon::SetOverlayShader( const idMaterial* material ) {
	renderEntity.overlayShader = material;
}

/***********************************************************************

  rvWeapon  
	
***********************************************************************/

CLASS_DECLARATION( idClass, rvWeapon )
END_CLASS

/*
================
rvWeapon::rvWeapon
================
*/
rvWeapon::rvWeapon ( void ) {
	viewModel	= NULL;
	worldModel	= NULL;
	weaponDef	= NULL;

#ifdef _XENON
	aimAssistFOV = 10.0f;
#endif	

	memset ( &animDoneTime, 0, sizeof(animDoneTime) );
	memset ( &wsfl, 0, sizeof(wsfl) );
	memset ( &wfl, 0, sizeof(wfl) );

	hitscanAttackDef = -1;
	
	forceGUIReload = false;
}

/*
================
rvWeapon::~rvWeapon
================
*/
rvWeapon::~rvWeapon( void ) {
	int i;
	
	// Free all current light defs
	for ( i = 0; i < WPLIGHT_MAX; i ++ ) {
		FreeLight ( i );
	}
		
	// Disassociate with the view model
	if ( viewModel ) {
		StopSound( SND_CHANNEL_ANY, false );
		viewModel->weapon = NULL;
	}
}

/*
================
rvWeapon::Init
================
*/
void rvWeapon::Init( idPlayer* _owner, const idDeclEntityDef* def, int _weaponIndex, bool _isStrogg ) {
	int i;
	
	viewModel		= _owner->GetWeaponViewModel( );
	worldModel		= _owner->GetWeaponWorldModel( );
	weaponDef		= def; 
	owner			= _owner;
	scriptObject	= &viewModel->scriptObject;
	weaponIndex 	= _weaponIndex;
	mods			= owner->inventory.weaponMods[ weaponIndex ];
	isStrogg		= _isStrogg;
	
	spawnArgs = weaponDef->dict;

#ifdef _XENON
	aimAssistFOV = spawnArgs.GetFloat( "aimAssistFOV", "10.0f" );
#endif	

	// Apply the mod dictionaries
	for ( i = 0; i < MAX_WEAPONMODS; i ++ ) {		
		const idDict* modDict;
		if ( !(mods & (1<<i) ) ) {
			continue;
		}

		const char* mod;
		if ( !spawnArgs.GetString ( va("def_mod%d", i+1), "", &mod ) || !*mod ) { 
			continue;
		}
		
		modDict = gameLocal.FindEntityDefDict ( mod, false );
		if ( !modDict ) {
			continue;
		}
		
		spawnArgs.Copy ( *modDict );
   	}
   	
   	// Associate the weapon with the view model
	viewModel->weapon = this;
}

/*
================
rvWeapon::FindViewModelPositionStyle
================
*/
void rvWeapon::FindViewModelPositionStyle( idVec3& viewOffset, idAngles& viewAngles ) const {
	int viewStyle = g_gunViewStyle.GetInteger();
	const char* styleDefName = spawnArgs.GetString( va("def_viewStyle%d", viewStyle) );
	const idDict* styleDef = gameLocal.FindEntityDefDict( styleDefName, false );
	if( !styleDef ) {
		styleDefName = spawnArgs.GetString( "def_viewStyle" );
		styleDef = gameLocal.FindEntityDefDict( styleDefName, false );
	}
	assert( styleDef );
	
	viewAngles = styleDef->GetAngles( "viewangles" );
	viewOffset = styleDef->GetVector( "viewoffset" );
}

/*
================
rvWeapon::Spawn
================
*/
void rvWeapon::Spawn ( void ) {
	
	memset ( &wsfl, 0, sizeof(wsfl) );
	memset ( &wfl, 0, sizeof(wfl) );

// RAVEN BEGIN
// nrausch:
#if defined(_XENON)
	aimAssistFOV = spawnArgs.GetFloat( "aimAssistFOV", "10.0f" );
#endif
// RAVEN END

	// Initialize variables
	projectileEnt	= NULL;
	kick_endtime	= 0;
	hideStart		= 0.0f;
	hideEnd			= 0.0f;
	hideOffset		= 0.0f;
	status			= WP_HOLSTERED;
	lastAttack		= 0;
 	clipPredictTime	= 0;

	muzzleAxis.Identity();
	muzzleOrigin.Zero();
	pushVelocity.Zero();
	playerViewAxis.Identity();
	playerViewOrigin.Zero();
	viewModelAxis.Identity();
	viewModelOrigin.Zero();

	// View
	viewModelForeshorten = spawnArgs.GetFloat ( "foreshorten", "1" );

	FindViewModelPositionStyle( viewModelOffset, viewModelAngles );

	// Offsets
	weaponAngleOffsetAverages	= spawnArgs.GetInt( "weaponAngleOffsetAverages", "10" );
	weaponAngleOffsetScale		= spawnArgs.GetFloat( "weaponAngleOffsetScale", "0.25" );
	weaponAngleOffsetMax		= spawnArgs.GetFloat( "weaponAngleOffsetMax", "10" );
	weaponOffsetTime			= spawnArgs.GetFloat( "weaponOffsetTime", "400" );
	weaponOffsetScale			= spawnArgs.GetFloat( "weaponOffsetScale", "0.005" );

	fireRate	= SEC2MS ( spawnArgs.GetFloat ( "fireRate" ) );
	altFireRate	= SEC2MS ( spawnArgs.GetFloat ( "altFireRate" ) );
	if( altFireRate == 0 ) {
		altFireRate = fireRate;
	}
	spread		= (gameLocal.IsMultiplayer()&&spawnArgs.FindKey("spread_mp"))?spawnArgs.GetFloat ( "spread_mp" ):spawnArgs.GetFloat ( "spread" );
	nextAttackTime = 0;

	// Zoom
	zoomFov = spawnArgs.GetInt( "zoomFov", "-1" );
	zoomGui  = uiManager->FindGui ( spawnArgs.GetString ( "gui_zoom", "" ), true );
	zoomTime = spawnArgs.GetFloat ( "zoomTime", ".15" );
	wfl.zoomHideCrosshair = spawnArgs.GetBool ( "zoomHideCrosshair", "1" );

	// Attack related values
	muzzle_kick_time	= SEC2MS( spawnArgs.GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( spawnArgs.GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= spawnArgs.GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= spawnArgs.GetVector( "muzzle_kick_offset" );

	// General weapon properties
	wfl.silent_fire		= spawnArgs.GetBool( "silent_fire" );
	wfl.hasWindupAnim   = spawnArgs.GetBool( "has_windup", "0" );
	icon				= spawnArgs.GetString( "mtr_icon" );
 	hideTime			= SEC2MS( weaponDef->dict.GetFloat( "hide_time", "0.3" ) );
 	hideDistance		= weaponDef->dict.GetFloat( "hide_distance", "-15" );
 	hideStartTime		= gameLocal.time - hideTime;
 	muzzleOffset		= weaponDef->dict.GetFloat ( "muzzleOffset", "14" );

	// Ammo
	clipSize			= spawnArgs.GetInt( "clipSize" );
	ammoRequired		= spawnArgs.GetInt( "ammoRequired" );
	lowAmmo				= spawnArgs.GetInt( "lowAmmo" );
	ammoType			= GetAmmoIndexForName( spawnArgs.GetString( "ammoType" ) );
	maxAmmo				= owner->inventory.MaxAmmoForAmmoClass ( owner, GetAmmoNameForIndex ( ammoType ) );
	
	if ( ( ammoType < 0 ) || ( ammoType >= MAX_AMMO ) ) {
		gameLocal.Warning( "Unknown ammotype for class '%s'", this->GetClassname ( ) );
	}

	// If the weapon has a clip, then fill it up
 	ammoClip = owner->inventory.clip[weaponIndex];
 	if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) ) {
 		// first time using this weapon so have it fully loaded to start
 		ammoClip = clipSize;
 		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
 		if ( ammoClip > ammoAvail ) {
 			ammoClip = ammoAvail;
		}
	}
	
	// Complex initializations Initialize 
	InitDefs( );
	InitWorldModel( );
	InitViewModel( );
	
	// Requires the view model so must be done after it
	InitLights( );

	viewModel->PostGUIEvent( "weapon_init" );
	viewModel->PostGUIEvent( "weapon_ammo" );
	if ( ammoClip == 0 && AmmoAvailable() == 0 ) {
		viewModel->PostGUIEvent( "weapon_noammo" );
	}

	stateThread.SetName( va("%s_%s_%s", owner->GetName(), viewModel->GetName ( ), spawnArgs.GetString("classname") ) );
	stateThread.SetOwner( this );
	
	forceGUIReload = true;
}

/*
================
rvWeapon::InitViewModel
================
*/
void rvWeapon::InitViewModel( void ) {
	const char*			guiName;
	const char*			temp;
	int					i;
	const idKeyValue*	kv;

	// Reset view model to clean state
	viewModel->Clear ( );
	// Make sure the sound handle is initted
	viewModel->refSound.referenceSoundHandle = -1;
	
	// Intialize the weapon guis
	if ( spawnArgs.GetString ( "gui", "", &guiName ) ) {
		int g = 0;
		do {
			viewModel->GetRenderEntity()->gui[g++] = uiManager->FindGui ( guiName, true, false, true );
			guiName = spawnArgs.GetString ( va("gui%d", g + 1 ) );
		} while ( *guiName && viewModel->GetRenderEntity()->gui[g-1] );
	}

	// Set the view models spawn args
	viewModel->spawnArgs = weaponDef->dict;

	// Set the model for the view model
	if ( isStrogg ) {		
		temp = spawnArgs.GetString ( "model_view_strogg", spawnArgs.GetString ( "model_view" ) );
	} else {
		temp = spawnArgs.GetString ( "model_view" );
	}	
	viewModel->SetModel( temp, mods );

	// Hide surfaces
	for ( kv = spawnArgs.MatchPrefix ( "hidesurface", NULL );
		  kv;
		  kv = spawnArgs.MatchPrefix ( "hidesurface", kv ) ) {
		viewModel->ProcessEvent ( &EV_HideSurface, kv->GetValue() );	
	}

	// Show and Hide the mods
	for ( i = 0; i < MAX_WEAPONMODS; i ++ )	{	
		const idDict* modDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( va("def_mod%d", i+1) ), false );
		if ( !modDict ) {
			continue;
		}
			
		// Hide any show surfaces for mods that arent on
		if ( !(mods & (1<<i)) ) {						
			for ( kv = modDict->MatchPrefix ( "mod_showsurface", NULL );
				  kv;
				  kv = modDict->MatchPrefix ( "mod_showsurface", kv ) ) {
				viewModel->ProcessEvent ( &EV_HideSurface, kv->GetValue() ); // NOTE: HIDING them because we don't have this mod yet
			}
		} else {
			for ( kv = modDict->MatchPrefix ( "mod_hidesurface", NULL );
				  kv;
				  kv = modDict->MatchPrefix ( "mod_hidesurface", kv ) ) {
				viewModel->ProcessEvent ( &EV_HideSurface, kv->GetValue() );	
			}
		}
	}

	// find some joints in the model for locating effects
	viewAnimator		= viewModel->GetAnimator ( );
	barrelJointView		= viewAnimator->GetJointHandle( spawnArgs.GetString ( "joint_view_barrel", "barrel" ) );
	flashJointView		= viewAnimator->GetJointHandle( spawnArgs.GetString ( "joint_view_flash", "flash" ) );
	ejectJointView		= viewAnimator->GetJointHandle( spawnArgs.GetString ( "joint_view_eject", "eject" ) );
	guiLightJointView	= viewAnimator->GetJointHandle( spawnArgs.GetString ( "joint_view_guiLight", "guiLight" ) );
	flashlightJointView = viewAnimator->GetJointHandle( spawnArgs.GetString ( "joint_view_flashlight", "flashlight" ) );

	// Eject offset
	spawnArgs.GetVector ( "ejectOffset", "0 0 0", ejectOffset );

	// Setup a skin for the view model
	if ( spawnArgs.GetString ( "skin", "", &temp ) ) {
		viewModel->GetRenderEntity()->customSkin = declManager->FindSkin ( temp );
	}
	
 	// make sure we have the correct skin
 	viewModel->UpdateSkin();
}

/*
================
rvWeapon::InitLights
================
*/
void rvWeapon::InitLights ( void ) {
	const char*		shader;
	idVec4			color;
	renderLight_t*	light;
	
	memset ( lights, 0, sizeof(lights) );
	memset ( lightHandles, -1, sizeof(lightHandles) );
		
	// setup gui light
	light = &lights[WPLIGHT_GUI];
	shader = spawnArgs.GetString( "mtr_guiLightShader", "" );
	if ( shader && *shader && viewModel->GetRenderEntity()->gui[0] ) {
		light->shader = declManager->FindMaterial( shader, false );
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = spawnArgs.GetFloat("glightRadius", "3" );
		color = viewModel->GetRenderEntity()->gui[0]->GetLightColor ( );
		light->shaderParms[ SHADERPARM_RED ]	 = color[0] * color[3];
		light->shaderParms[ SHADERPARM_GREEN ] = color[1] * color[3];
		light->shaderParms[ SHADERPARM_BLUE ]	 = color[2] * color[3];
		light->shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		light->pointLight = true;
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// dluetscher: changed lights to no shadow for performance reasons
		light->noShadows = true;
// RAVEN END
		light->lightId = WPLIGHT_GUI * 100 + owner->entityNumber;
		light->allowLightInViewID = owner->entityNumber+1;
		spawnArgs.GetVector ( "glightOffset", "0 0 0", guiLightOffset );
	}

	// Muzzle flash
	light = &lights[WPLIGHT_MUZZLEFLASH];
	shader = spawnArgs.GetString( "mtr_flashShader", "muzzleflash" );
	if ( shader && *shader ) {
		light->shader = declManager->FindMaterial( shader, false );
		spawnArgs.GetVec4( "flashColor", "0 0 0 0", color );
		light->shaderParms[ SHADERPARM_RED ]		= color[0];
		light->shaderParms[ SHADERPARM_GREEN ]		= color[1];
		light->shaderParms[ SHADERPARM_BLUE ]		= color[2];
		light->shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] = (float)spawnArgs.GetInt( "flashRadius" );
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// dluetscher: changed lights to no shadow for performance reasons
		light->noShadows = true;
// RAVEN END
		light->pointLight = spawnArgs.GetBool( "flashPointLight", "1" );
		if ( !light->pointLight ) {
			light->target = spawnArgs.GetVector ( "flashTarget" );
			light->up	  = spawnArgs.GetVector ( "flashUp" );
			light->right  = spawnArgs.GetVector ( "flashRight" );
			light->end    = light->target;
		}
		light->lightId = WPLIGHT_MUZZLEFLASH * 100 + owner->entityNumber;
		light->allowLightInViewID = owner->entityNumber+1;
		muzzleFlashTime	= SEC2MS( spawnArgs.GetFloat( "flashTime", "0.25" ) );
		muzzleFlashEnd = 0;
		spawnArgs.GetVector ( "flashViewOffset", "0 0 0", muzzleFlashViewOffset );	
	}

	// the world muzzle flash is the same, just positioned differently
	lights[WPLIGHT_MUZZLEFLASH_WORLD] = lights[WPLIGHT_MUZZLEFLASH];
	light = &lights[WPLIGHT_MUZZLEFLASH_WORLD];
	light->suppressLightInViewID = owner->entityNumber+1;
	light->allowLightInViewID = 0;
	light->lightId = WPLIGHT_MUZZLEFLASH_WORLD * 100 + owner->entityNumber;

	// flashlight	
	light = &lights[WPLIGHT_FLASHLIGHT];
	shader = spawnArgs.GetString( "mtr_flashlightShader", "lights/muzzleflash" );
	if ( shader && *shader ) {
		light->shader	  = declManager->FindMaterial( shader, false );
		spawnArgs.GetVec4( "flashlightColor", "0 0 0 0", color );
		light->shaderParms[ SHADERPARM_RED ]			= color[0];
		light->shaderParms[ SHADERPARM_GREEN ]			= color[1];
		light->shaderParms[ SHADERPARM_BLUE ]			= color[2];
		light->shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;
		light->lightRadius[0] = light->lightRadius[1] = light->lightRadius[2] =
			(float)spawnArgs.GetInt( "flashlightRadius" );
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
		light->detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// dluetscher: changed lights to no shadow for performance reasons
		light->noShadows = cvarSystem->GetCVarInteger("com_machineSpec") < 3;
// RAVEN END
		light->pointLight = spawnArgs.GetBool( "flashlightPointLight", "1" );
		if ( !light->pointLight ) {
			light->target	= spawnArgs.GetVector( "flashlightTarget" );
			light->up		= spawnArgs.GetVector( "flashlightUp" );
			light->right	= spawnArgs.GetVector( "flashlightRight" );
			light->end		= light->target;
		}
		
		light->allowLightInViewID = owner->entityNumber+1;
		light->lightId = WPLIGHT_FLASHLIGHT * 100 + owner->entityNumber;
		spawnArgs.GetVector ( "flashlightViewOffset", "0 0 0", flashlightViewOffset );	
	}

	// the world muzzle flashlight is the same, just positioned differently
	lights[WPLIGHT_FLASHLIGHT_WORLD] = lights[WPLIGHT_FLASHLIGHT];
	light = &lights[WPLIGHT_FLASHLIGHT_WORLD];
	light->suppressLightInViewID = owner->entityNumber+1;
	light->allowLightInViewID = 0;
	light->lightId = WPLIGHT_FLASHLIGHT_WORLD * 100 + owner->entityNumber; 	
}

/*
================
rvWeapon::InitDefs
================
*/
void rvWeapon::InitDefs( void ) {
	const char*				name;
	const idDeclEntityDef*	def;
	const char*				spawnclass;
	idTypeInfo*				cls;
	
	// get the projectile
	attackDict.Clear();

	// Projectile
	if ( spawnArgs.GetString( "def_projectile", "", &name ) && *name ) {
		def = gameLocal.FindEntityDef( name, false );
		if ( !def ) {
			gameLocal.Warning( "Unknown projectile '%s' for weapon '%s'", name, weaponDef->GetName() );
		} else {
			spawnclass = def->dict.GetString( "spawnclass" );
			cls		   = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::GetClassType() ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' for projectile '%s' (used by weapon '%s')", spawnclass, name, weaponDef->GetName ( ) );
			} else {
				attackDict = def->dict;
			}
		}
	} else if ( spawnArgs.GetString( "def_hitscan", "", &name ) && *name ) {
		def = gameLocal.FindEntityDef( name, false );
		if ( !def ) {
			gameLocal.Warning( "Unknown hitscan '%s' for weapon '%s'", name, weaponDef->GetName ( ) );
		} else {
			attackDict = def->dict;
			hitscanAttackDef = def->Index();
		}
		wfl.attackHitscan = true;
	} 

	// Alternate projectile
	attackAltDict.Clear ();
	if ( spawnArgs.GetString( "def_altprojectile", "", &name ) && *name ) {
		def = gameLocal.FindEntityDef( name, false );
		if ( !def ) {
			gameLocal.Warning( "Unknown alt projectile '%s' for weapon '%s'", name, weaponDef->GetName() );
		} else {
			spawnclass = def->dict.GetString( "spawnclass" );
			cls = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::GetClassType() ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' for alt projectile '%s' (used by weapon '%s')", spawnclass, name, weaponDef->GetName ( ) );
			} else {
				attackAltDict = def->dict;
			}
		}
	} else if ( spawnArgs.GetString( "def_althitscan", "", &name ) && *name ) {
		def = gameLocal.FindEntityDef( name, false );
		if ( !def ) {
			gameLocal.Warning( "Unknown hitscan '%s' for weapon '%s'", name, weaponDef->GetName ( ) );
		} else {
			attackAltDict = def->dict;
		}
		wfl.attackAltHitscan = true;
	} 

	// get the melee damage def
	meleeDistance = spawnArgs.GetFloat( "melee_distance" );
	if ( spawnArgs.GetString( "def_melee", "", &name ) && *name ) {
		meleeDef = gameLocal.FindEntityDef( name, false );
		if ( !meleeDef ) {
			gameLocal.Error( "Unknown melee '%s' for weapon '%s'", name, weaponDef->GetName() );
		}
	} else {
		meleeDef = NULL;
	}

	// get the brass def
	brassDict.Clear();
	if ( spawnArgs.GetString( "def_ejectBrass", "", &name ) && *name ) {
		def = gameLocal.FindEntityDef( name, false );
		if ( !def ) {
			gameLocal.Warning( "Unknown brass def '%s' for weapon '%s'", name, weaponDef->GetName() );
		} else {
			brassDict = def->dict;
			// force any brass to spawn as client moveable
			brassDict.Set( "spawnclass", "rvClientMoveable" );
		}
	}
}

/*
================
rvWeapon::Think
================
*/
void rvWeapon::Think ( void ) {	
	
	// Cache the player origin and axis
	playerViewOrigin = owner->firstPersonViewOrigin;
	playerViewAxis   = owner->firstPersonViewAxis;

	// calculate weapon position based on player movement bobbing
	owner->CalculateViewWeaponPos( viewModelOrigin, viewModelAxis );

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
	}
	viewModelOrigin += hideOffset * viewModelAxis[ 2 ];

	// kick up based on repeat firing
	MuzzleRise( viewModelOrigin, viewModelAxis );

	if ( viewModel ) {
		// set the physics position and orientation
		viewModel->GetPhysics()->SetOrigin( viewModelOrigin );
		viewModel->GetPhysics()->SetAxis( viewModelAxis );
 		viewModel->UpdateVisuals();
	} else {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
	}
	
	// Update the zoom variable before updating the script
	wsfl.zoom = owner->IsZoomed( );

	// Only update the state loop on new frames
 	if ( gameLocal.isNewFrame ) {
		stateThread.Execute( );
	}

	if ( viewModel ) {
		viewModel->UpdateAnimation( );
	}

	// Clear reload and flashlight flags
	wsfl.reload		= false;
	wsfl.flashlight	= false;
	
	// deal with the third-person visible world model 
	// don't show shadows of the world model in first person
	if ( worldModel && worldModel->GetRenderEntity() ) {
		// always show your own weapon
		if( owner->entityNumber == gameLocal.localClientNum ) {
			worldModel->GetRenderEntity()->suppressLOD = 1;
		} else {
			worldModel->GetRenderEntity()->suppressLOD = 0;
		}

		if ( gameLocal.IsMultiplayer() && g_skipPlayerShadowsMP.GetBool() ) {
			// Disable all weapon shadows for the local client
			worldModel->GetRenderEntity()->suppressShadowInViewID	= gameLocal.localClientNum+1;
			worldModel->GetRenderEntity()->suppressShadowInLightID = WPLIGHT_MUZZLEFLASH * 100 + owner->entityNumber;
		} else if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
			// Show all weapon shadows
			worldModel->GetRenderEntity()->suppressShadowInViewID	= 0;
		} else {
			// Only show weapon shadows for other clients
			worldModel->GetRenderEntity()->suppressShadowInViewID	= owner->entityNumber+1;
			worldModel->GetRenderEntity()->suppressShadowInLightID = WPLIGHT_MUZZLEFLASH * 100 + owner->entityNumber;
		}
	}

	UpdateGUI();

	// Update lights
	UpdateFlashlight ( );
	UpdateMuzzleFlash ( );

	// update the gui light
	renderLight_t& light = lights[WPLIGHT_GUI];
	if ( light.lightRadius[0] && guiLightJointView != INVALID_JOINT ) {
		if ( viewModel ) {
			idVec4 color = viewModel->GetRenderEntity()->gui[0]->GetLightColor ( );
			light.shaderParms[ SHADERPARM_RED ]	  = color[0] * color[3];
			light.shaderParms[ SHADERPARM_GREEN ] = color[1] * color[3];
			light.shaderParms[ SHADERPARM_BLUE ]  = color[2] * color[3];
			GetGlobalJointTransform( true, guiLightJointView, light.origin, light.axis, guiLightOffset );		
			UpdateLight ( WPLIGHT_GUI );
		} else {
			common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		}
	}

	// Alert Monsters if the flashlight is one or a muzzle flash is active?
	if ( !gameLocal.isMultiplayer ) {
		if ( !owner->fl.notarget && (lightHandles[WPLIGHT_MUZZLEFLASH] != -1 || lightHandles[WPLIGHT_FLASHLIGHT] != -1 ) ) {
			AlertMonsters ( );
		}
	}
}

/*
================
rvWeapon::InitWorldModel
================
*/
void rvWeapon::InitWorldModel( void ) {
	idEntity *ent;

	ent = worldModel;
	if ( !ent ) {
		gameLocal.Warning ( "InitWorldModel failed due to missing entity" );
		return;
	}

	const char *model = spawnArgs.GetString( "model_world" );
	const char *attach = spawnArgs.GetString( "joint_attach" );

	if ( model[0] && attach[0] ) {
		ent->Show();
		ent->SetModel( model );
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
			worldModelRenderEntity->suppressShadowInLightID = WPLIGHT_MUZZLEFLASH * 100 + owner->entityNumber;
		}
	} else {
		ent->SetModel( "" );
		ent->Hide();
	}

	// the renderEntity is reused, so the relevant fields (except this one) appear to be correctly reinitialized
	worldModel->GetRenderEntity()->suppressSurfaceMask = 0;

	// Cache the world joints
	worldAnimator			= ent->GetAnimator ( );
	flashJointWorld			= worldAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_world_flash", "flash" ) );
	flashlightJointWorld	= worldAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_world_flashlight", "flashlight" ) );
	ejectJointWorld			= worldAnimator->GetJointHandle ( spawnArgs.GetString ( "joint_world_eject", "eject" ) );
}

/*
================
rvWeapon::SetState
================
*/
void rvWeapon::SetState( const char *statename, int blendFrames ) {
	stateThread.SetState( statename, blendFrames );
}

/*
================
rvWeapon::PostState
================
*/
void rvWeapon::PostState( const char* statename, int blendFrames ) {
	stateThread.PostState( statename, blendFrames );
}

/*
=====================
rvWeapon::ExecuteState
=====================
*/
void rvWeapon::ExecuteState ( const char* statename ) {
	SetState ( statename, 0 );
	stateThread.Execute ( );
}

/*
================
rvWeapon::UpdateLight
================
*/
void rvWeapon::UpdateLight ( int lightID ) {
	if ( lightHandles[lightID] == -1 ) {
		lightHandles[lightID] = gameRenderWorld->AddLightDef ( &lights[lightID] );
	} else {
		gameRenderWorld->UpdateLightDef( lightHandles[lightID], &lights[lightID] );
	}
}

/*
================
rvWeapon::FreeLight
================
*/
void rvWeapon::FreeLight ( int lightID ) {
	if ( lightHandles[lightID] != -1 ) {
		gameRenderWorld->FreeLightDef( lightHandles[lightID] );
		lightHandles[lightID] = -1;
	}
}


/***********************************************************************

	Networking

***********************************************************************/

/*
================
rvWeapon::WriteToSnapshot
================
*/
void rvWeapon::WriteToSnapshot( idBitMsgDelta &msg ) const {
	// this can probably be reduced a bit, there's no clip/reload in MP
	// it seems that's used to drive some of the weapon model firing animations though (such as RL)
	msg.WriteBits( ammoClip, ASYNC_PLAYER_INV_CLIP_BITS );
}

/*
================
rvWeapon::ReadFromSnapshot
================
*/
void rvWeapon::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ammoClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
}

/*
================
rvWeapon::SkipFromSnapshot
================
*/
void rvWeapon::SkipFromSnapshot ( const idBitMsgDelta &msg ) {
	msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
}

/*
================
rvWeapon::ClientStale
================
*/
void rvWeapon::ClientStale( void ) {
}

/*
================
rvWeapon::ClientReceiveEvent
================
*/
bool rvWeapon::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
 	switch( event ) {
 		case EVENT_RELOAD: {
 			if ( gameLocal.time - time < 1000 ) {
 				wsfl.netReload	 = true;
 				wsfl.netEndReload = false;
 			}
 			return true;
 		}
 		case EVENT_ENDRELOAD: {
			wsfl.netEndReload = true;
 			return true;
 		}
 		case EVENT_CHANGESKIN: {
/*
			// FIXME: use idGameLocal::ReadDecl
 			int index = msg.ReadLong();
 			renderEntity.customSkin = ( index != -1 ) ? static_cast<const idDeclSkin *>( declManager->DeclByIndex( DECL_SKIN, index ) ) : NULL;
 			UpdateVisuals();
 			if ( worldModel.GetEntity() ) {
 				worldModel.GetEntity()->SetSkin( renderEntity.customSkin );
 			}
 */
 			return true;
 		}
 	}
 	return false;
}

/***********************************************************************

	Save / Load

***********************************************************************/

/*
================
rvWeapon::Save
================
*/
void rvWeapon::Save ( idSaveGame *savefile ) const {
	int i;

	// Flags
	savefile->Write			( &wsfl, sizeof( wsfl ) );
	savefile->Write			( &wfl, sizeof( wfl ) );

	// Write all cached joints
	savefile->WriteJoint	( barrelJointView );
	savefile->WriteJoint	( flashJointView );
	savefile->WriteJoint	( ejectJointView );
	savefile->WriteJoint	( guiLightJointView );
	savefile->WriteJoint	( flashlightJointView );

	savefile->WriteJoint	( flashJointWorld );
	savefile->WriteJoint	( ejectJointWorld );
	savefile->WriteJoint	( flashlightJointWorld );

	savefile->WriteInt		( status );
	savefile->WriteInt		( lastAttack );

	// Hide / Show
	savefile->WriteInt		( hideTime );
 	savefile->WriteFloat	( hideDistance );
 	savefile->WriteInt		( hideStartTime );
	savefile->WriteFloat	( hideStart );
	savefile->WriteFloat	( hideEnd );
	savefile->WriteFloat	( hideOffset );

	// Write attack related values
	savefile->WriteVec3		( pushVelocity );
	savefile->WriteInt		( kick_endtime );
	savefile->WriteInt		( muzzle_kick_time );
	savefile->WriteInt		( muzzle_kick_maxtime );
	savefile->WriteAngles	( muzzle_kick_angles );
	savefile->WriteVec3		( muzzle_kick_offset );
	savefile->WriteVec3		( muzzleOrigin );
	savefile->WriteMat3		( muzzleAxis );
	savefile->WriteFloat	( muzzleOffset );
	projectileEnt.Save ( savefile );
	savefile->WriteVec3		( ejectOffset );	// cnicholson: Added unsaved var

	savefile->WriteInt		( fireRate );
	savefile->WriteFloat	( spread );
	// savefile->WriteInt	( nextAttackTime ); // cnicholson: This is set to 0 in restore, so don't save it

	// cnicholson: These 3 idDicts are setup during restore, no need to save them.
	// TOSAVE: idDict							attackAltDict;
	// TOSAVE: idDict							attackDict;
	// TOSAVE: idDict							brassDict;

	// Defs
	// TOSAVE: const idDeclEntityDef *			meleeDef;	// cnicholson: This is setup in restore, so don't save it
	savefile->WriteFloat	( meleeDistance );

	// Zoom
	savefile->WriteInt				( zoomFov );
	savefile->WriteUserInterface	( zoomGui, true );
	savefile->WriteFloat			( zoomTime );

	// Lights
	for ( i = 0; i < WPLIGHT_MAX; i ++ ) {		
		savefile->WriteInt( lightHandles[i] );
		savefile->WriteRenderLight( lights[i] );
	}
	savefile->WriteVec3		( guiLightOffset );
	savefile->WriteInt		( muzzleFlashEnd );
	savefile->WriteInt		( muzzleFlashTime );
	savefile->WriteVec3		( muzzleFlashViewOffset );
	savefile->WriteVec3		( flashlightViewOffset );
	savefile->WriteBool		( flashlightOn );			// cnicholson: Added unsaved var
	savefile->WriteVec3		( flashlightViewOffset );	// cnicholson: Added unsaved var

	// Write ammo values
	savefile->WriteInt		( ammoType );
	savefile->WriteInt		( ammoRequired );
	savefile->WriteInt		( clipSize );
	savefile->WriteInt		( ammoClip );
	savefile->WriteInt		( lowAmmo );
	savefile->WriteInt		( maxAmmo );

	// multiplayer
 	savefile->WriteInt		( clipPredictTime );	// TOSAVE: Save MP value?

	// View 
	savefile->WriteVec3		( playerViewOrigin );
	savefile->WriteMat3		( playerViewAxis );

	savefile->WriteVec3		( viewModelOrigin );
	savefile->WriteMat3		( viewModelAxis );
	savefile->WriteAngles	( viewModelAngles );
	savefile->WriteVec3		( viewModelOffset );	// cnicholson: Added unsaved var

	// Offsets
	savefile->WriteInt		( weaponAngleOffsetAverages );
	savefile->WriteFloat	( weaponAngleOffsetScale );
	savefile->WriteFloat	( weaponAngleOffsetMax );
	savefile->WriteFloat	( weaponOffsetTime );
	savefile->WriteFloat	( weaponOffsetScale );

	savefile->WriteString	( icon );
	savefile->WriteBool		( isStrogg );

	// TOSAVE: idDict							spawnArgs;

	// TOSAVE: idEntityPtr<rvViewWeapon>		viewModel;			// cnicholson: Setup in restore, no need to save
	// TOSAVE: idAnimator*						viewAnimator;
	// TOSAVE: idEntityPtr<idAnimatedEntity>	worldModel;			// cnicholson: Setup in restore, no need to save
	// TOSAVE: idAnimator*						worldAnimator;
	// TOSAVE: const idDeclEntityDef*			weaponDef;
	// TOSAVE: idScriptObject*					scriptObject;
	savefile->WriteObject	( owner );
	savefile->WriteInt		( weaponIndex );	// cnicholson: Added unsaved var
	savefile->WriteInt		( mods );			// cnicholson: Added unsaved var

	savefile->WriteFloat	( viewModelForeshorten );

	stateThread.Save( savefile );

	for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		savefile->WriteInt( animDoneTime[i] );
	}

	savefile->WriteInt		( methodOfDeath );	// cnicholson: Added unsaved var
}

/*
================
rvWeapon::Restore
================
*/
void rvWeapon::Restore ( idRestoreGame *savefile ) {
	int						i;
	const idDeclEntityDef*	def;

	// General
	savefile->Read			( &wsfl, sizeof( wsfl ) );
	savefile->Read			( &wfl, sizeof( wfl ) );

	// Read cached joints
	savefile->ReadJoint		( barrelJointView );
	savefile->ReadJoint		( flashJointView );
	savefile->ReadJoint		( ejectJointView );
	savefile->ReadJoint		( guiLightJointView );
	savefile->ReadJoint		( flashlightJointView );

	savefile->ReadJoint		( flashJointWorld );
	savefile->ReadJoint		( ejectJointWorld );
	savefile->ReadJoint		( flashlightJointWorld );

	savefile->ReadInt		( (int&)status );
	savefile->ReadInt		( lastAttack );

	// Hide / Show
	savefile->ReadInt		( hideTime );
 	savefile->ReadFloat		( hideDistance );
 	savefile->ReadInt		( hideStartTime );
	savefile->ReadFloat		( hideStart );
	savefile->ReadFloat		( hideEnd );
	savefile->ReadFloat		( hideOffset );

	// Read attack related values
	savefile->ReadVec3		( pushVelocity );
	savefile->ReadInt		( kick_endtime );
	savefile->ReadInt		( muzzle_kick_time );
	savefile->ReadInt		( muzzle_kick_maxtime );
	savefile->ReadAngles	( muzzle_kick_angles );
	savefile->ReadVec3		( muzzle_kick_offset );
	savefile->ReadVec3		( muzzleOrigin );
	savefile->ReadMat3		( muzzleAxis );
	savefile->ReadFloat		( muzzleOffset );
	projectileEnt.Restore ( savefile );
	savefile->ReadVec3		( ejectOffset );	// cnicholson: Added unrestored var

	savefile->ReadInt		( fireRate );
	savefile->ReadFloat		( spread );
	nextAttackTime = 0;

	// Attack Alt Def
	attackAltDict.Clear( );
	wfl.attackAltHitscan = false;
	def = gameLocal.FindEntityDef( spawnArgs.GetString( "def_altprojectile" ), false );
	if ( def ) {
		attackAltDict = def->dict;
	} else {
		def = gameLocal.FindEntityDef( spawnArgs.GetString( "def_althitscan" ), false );
		if ( def ) {
			attackAltDict = def->dict;
			wfl.attackAltHitscan = true;
		}
	}

	// Attack def
	attackDict.Clear( );
	def = gameLocal.FindEntityDef( spawnArgs.GetString( "def_projectile" ), false );
	wfl.attackHitscan = false;
	if ( def ) {
		attackDict = def->dict;
	} else {
		def = gameLocal.FindEntityDef( spawnArgs.GetString( "def_hitscan" ), false );
		if ( def ) {
			attackDict = def->dict;
			wfl.attackHitscan = true;
		}
	}

	// Brass Def
	def = gameLocal.FindEntityDef( spawnArgs.GetString( "def_ejectBrass" ), false );
	if ( def ) {
		brassDict = def->dict;
	} else {
		brassDict.Clear();
	}

	// Melee Def
	meleeDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_melee" ), false );
	savefile->ReadFloat( meleeDistance );

	// Zoom
	savefile->ReadInt			( zoomFov );
	savefile->ReadUserInterface	( zoomGui, &spawnArgs );
	savefile->ReadFloat			( zoomTime );

	// Lights
	for ( i = 0; i < WPLIGHT_MAX; i ++ ) {
		savefile->ReadInt ( lightHandles[i] );
		savefile->ReadRenderLight( lights[i] );
		if ( lightHandles[i] != -1 ) {
			//get the handle again as it's out of date after a restore!
			lightHandles[i] = gameRenderWorld->AddLightDef ( &lights[i] );
		}
	}
	savefile->ReadVec3		( guiLightOffset );
	savefile->ReadInt		( muzzleFlashEnd );
	savefile->ReadInt		( muzzleFlashTime );
	savefile->ReadVec3		( muzzleFlashViewOffset );
	savefile->ReadVec3		( flashlightViewOffset );
	savefile->ReadBool		( flashlightOn );			// cnicholson: Added unrestored var
	savefile->ReadVec3		( flashlightViewOffset );	// cnicholson: Added unrestored var

	// Read the ammo values
	savefile->ReadInt		( (int&)ammoType );
	savefile->ReadInt		( ammoRequired );
	savefile->ReadInt		( clipSize );
	savefile->ReadInt		( ammoClip );
	savefile->ReadInt		( lowAmmo );
	savefile->ReadInt		( maxAmmo );

	// multiplayer
 	savefile->ReadInt		( clipPredictTime );		// TORESTORE: Restore MP value?

	// View 
	savefile->ReadVec3		( playerViewOrigin );
	savefile->ReadMat3		( playerViewAxis );

	savefile->ReadVec3		( viewModelOrigin );
	savefile->ReadMat3		( viewModelAxis );
	savefile->ReadAngles	( viewModelAngles );
	savefile->ReadVec3		( viewModelOffset );	// cnicholson: Added unrestored var

	// Offsets
	savefile->ReadInt		( weaponAngleOffsetAverages );
	savefile->ReadFloat		( weaponAngleOffsetScale );
	savefile->ReadFloat		( weaponAngleOffsetMax );
	savefile->ReadFloat		( weaponOffsetTime );
	savefile->ReadFloat		( weaponOffsetScale );

	savefile->ReadString	( icon );
	savefile->ReadBool		( isStrogg );

	// TORESTORE: idDict							spawnArgs;

	// TORESTORE: idAnimator*						viewAnimator;
	// TORESTORE: idAnimator*						worldAnimator;
	// TORESTORE: const idDeclEntityDef*			weaponDef;
	// TORESTORE: idScriptObject*					scriptObject;

	// Entities
	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	viewModel = owner->GetWeaponViewModel ( );
	worldModel = owner->GetWeaponWorldModel ( );

	savefile->ReadInt		( weaponIndex );	// cnicholson: Added unrestored var
	savefile->ReadInt		( mods );			// cnicholson: Added unrestored var

	savefile->ReadFloat		( viewModelForeshorten );

	stateThread.Restore( savefile, this );

	for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		savefile->ReadInt( animDoneTime[i] );
	}
	
	savefile->ReadInt		( methodOfDeath );	// cnicholson: Added unrestored var

#ifdef _XENON
	aimAssistFOV = spawnArgs.GetFloat( "aimAssistFOV", "10.0f" );
#endif	
}

/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
rvWeapon::Hide
================
*/
void rvWeapon::Hide( void ) {
 	muzzleFlashEnd = 0;

	if ( viewModel ) {
		viewModel->Hide();
	}
	if ( worldModel ) {
		worldModel->Hide ( );
	}

	// Stop flashlight and gui lights
	FreeLight ( WPLIGHT_GUI );
	FreeLight ( WPLIGHT_FLASHLIGHT );
	FreeLight ( WPLIGHT_FLASHLIGHT_WORLD );
}

/*
================
rvWeapon::Show
================
*/
void rvWeapon::Show ( void ) {
	if ( viewModel ) {
		viewModel->Show();
	}
	if ( worldModel ) {
		worldModel->Show();
	}
}

/*
================
rvWeapon::IsHidden
================
*/
bool rvWeapon::IsHidden( void ) const {
	return !viewModel || viewModel->IsHidden();
}

/*
================
rvWeapon::HideWorldModel
================
*/
void rvWeapon::HideWorldModel ( void ) {
	if ( worldModel ) {
		worldModel->Hide();
	}
}

/*
================
rvWeapon::ShowWorldModel
================
*/
void rvWeapon::ShowWorldModel ( void ) {
	if ( worldModel ) {
		worldModel->Show();
	}
}


/*
================
rvWeapon::LowerWeapon
================
*/
void rvWeapon::LowerWeapon( void ) {
	if ( !wfl.hide ) {
		hideStart	= 0.0f;
		hideEnd		= hideDistance;
 		if ( gameLocal.time - hideStartTime < hideTime ) {
 			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
   		} else {
 			hideStartTime = gameLocal.time;
		}
		wfl.hide = true;
	}
}

/*
================
rvWeapon::RaiseWeapon
================
*/
void rvWeapon::RaiseWeapon( void ) {
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	viewModel->Show();

	if ( forceGUIReload ) {
 		forceGUIReload = false;
 		int ammo = AmmoInClip();
 		for ( int g = 0; g < MAX_RENDERENTITY_GUI && viewModel->GetRenderEntity()->gui[g]; g ++ ) {
			idUserInterface* gui = viewModel->GetRenderEntity()->gui[g];
 			if ( gui ) {
				gui->SetStateInt ( "player_ammo", ammo );
				
				if ( ClipSize ( ) ) {
					gui->SetStateFloat ( "player_ammopct", (float)ammo / (float)ClipSize() );
					gui->SetStateInt ( "player_clip_size", ClipSize() );
				} else { 
					gui->SetStateFloat ( "player_ammopct", (float)ammo / (float)maxAmmo );
					gui->SetStateInt ( "player_clip_size", maxAmmo );
				}
				gui->SetStateInt ( "player_cachedammo", ammo );
				gui->HandleNamedEvent ( "weapon_ammo" );
 			}
 		}
	}

	if ( wfl.hide ) {
 		hideStart	= hideDistance;
		hideEnd		= 0.0f;
 		if ( gameLocal.time - hideStartTime < hideTime ) {
 			hideStartTime = gameLocal.time - ( hideTime - ( gameLocal.time - hideStartTime ) );
   		} else {
 			hideStartTime = gameLocal.time;
		}
		wfl.hide = false;
	}
}

/*
================
rvWeapon::PutAway
================
*/
void rvWeapon::PutAway( void ) {
	wfl.hasBloodSplat  = false;
	wsfl.lowerWeapon   = true;

	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	viewModel->PostGUIEvent ( "weapon_lower" );
}

/*
================
rvWeapon::Raise
================
*/
void rvWeapon::Raise( void ) {
	wsfl.raiseWeapon = true;

	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	viewModel->PostGUIEvent ( "weapon_raise" );
}

/*
================
rvWeapon::Flashlight
================
*/
void rvWeapon::Flashlight ( void ) {
	wsfl.flashlight = true;
}

/*
================
rvWeapon::SetPushVelocity
================
*/
void rvWeapon::SetPushVelocity( const idVec3& _pushVelocity ) {
	pushVelocity = _pushVelocity;
}

/*
================
rvWeapon::Reload
NOTE: this is only for impulse-triggered reload, auto reload is scripted
================
*/
void rvWeapon::Reload( void ) {
	if ( clipSize ) {
		wsfl.reload = true;
	}
}

/*
================
rvWeapon::CancelReload
================
*/
void rvWeapon::CancelReload( void ) {
	wsfl.attack = true;
}

/*
================
rvWeapon::AutoReload
================
*/
bool rvWeapon::AutoReload ( void ) {
	assert( owner );

 	// on a network client, never predict reloads of other clients. wait for the server
 	if ( gameLocal.isClient ) {
 		return false;
 	}
	return gameLocal.userInfo[ owner->entityNumber ].GetBool( "ui_autoReload" );
}

/*
================
rvWeapon::NetReload
================
*/
void rvWeapon::NetReload ( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		if ( !viewModel ) {
			common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
			return;
		}
		viewModel->ServerSendEvent( EVENT_RELOAD, NULL, false, -1 );
	}
}

/*
===============
rvWeapon::NetEndReload
===============
*/
void rvWeapon::NetEndReload ( void ) {
	assert( owner );
	if ( gameLocal.isServer ) {
		if ( !viewModel ) {
			common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
			return;
		}
		viewModel->ServerSendEvent( EVENT_ENDRELOAD, NULL, false, -1 );
	}
}   

/*
================
rvWeapon::SetStatus
================
*/
void rvWeapon::SetStatus ( weaponStatus_t _status ) {	
	status = _status;
	switch ( status ) {
		case WP_READY:			
			wsfl.raiseWeapon = false;
			if ( !viewModel ) {
				common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
				break;
			}
			viewModel->PostGUIEvent ( "weapon_ready" );
			break;
		case WP_OUTOFAMMO:
			wsfl.raiseWeapon = false;
			break;		
		case WP_RELOAD:
			if ( !viewModel ) {
				common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
				break;
			}
			viewModel->PostGUIEvent ( "weapon_reload" );
			break;
		case WP_HOLSTERED:
		case WP_RISING:
			wsfl.lowerWeapon = false;
		 	owner->WeaponRisingCallback();
			break;
		case WP_LOWERING:
			wsfl.raiseWeapon = false;
		 	owner->WeaponLoweringCallback();
			break;
	}
}

/*
================
rvWeapon::OwnerDied
================
*/
void rvWeapon::OwnerDied( void ) {

	CleanupWeapon();

	ExecuteState( "OwnerDied" );

	if ( viewModel ) {
		viewModel->StopSound( SCHANNEL_ANY, false );
		viewModel->StopAllEffects( );
		viewModel->Hide();
	}
	if ( worldModel ) {
		worldModel->Hide();
	}
}

/*
================
rvWeapon::BeginAttack
================
*/
void rvWeapon::BeginAttack( void ) {
	wsfl.attack = true;

	if ( status != WP_OUTOFAMMO ) {
		lastAttack = gameLocal.time;
	}
}

/*
================
rvWeapon::EndAttack
================
*/
void rvWeapon::EndAttack( void ) {
	wsfl.attack = false;
}

/*
================
rvWeapon::isReady
================
*/
bool rvWeapon::IsReady( void ) const {
	return !wfl.hide && ! ( gameLocal.time - hideStartTime < hideTime ) && ( viewModel && !viewModel->IsHidden()) && ( ( status == WP_READY ) || ( status == WP_OUTOFAMMO ) );
}

/*
================
rvWeapon::IsReloading
================
*/
bool rvWeapon::IsReloading( void ) const {
	return ( status == WP_RELOAD );
}

/*
================
rvWeapon::IsHolstered
================
*/
bool rvWeapon::IsHolstered( void ) const {
	return ( status == WP_HOLSTERED );
}

/*
================
rvWeapon::ShowCrosshair
================
*/
bool rvWeapon::ShowCrosshair( void ) const {
	if ( owner->IsZoomed ( ) && zoomGui && wfl.zoomHideCrosshair ) {
		return false;
	}
	return !( status == WP_HOLSTERED );
}

/*
=====================
rvWeapon::CanDrop
=====================
*/
bool rvWeapon::CanDrop( void ) const {
	const char *classname = spawnArgs.GetString( "def_dropItem" );
	if ( !classname[ 0 ] ) {
		return false;
	}
	return true;
}

/*
=====================
rvViewWeapon::CanZoom
=====================
*/
bool rvWeapon::CanZoom( void ) const {
#ifdef _XENON
	// apparently a xenon specific bug in medlabs.
	return zoomFov != -1 && !IsHidden();
#else
	return zoomFov != -1;
#endif
}

/***********************************************************************

	Visual presentation

***********************************************************************/

/*
================
rvWeapon::MuzzleRise
================
*/
void rvWeapon::MuzzleRise( idVec3 &origin, idMat3 &axis ) {
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
rvWeapon::UpdateFlashPosition
================
*/
void rvWeapon::UpdateMuzzleFlash ( void ) {
	// remove the muzzle flash light when it's done
	if ( gameLocal.time >= muzzleFlashEnd || !gameLocal.GetLocalPlayer() || !owner || gameLocal.GetLocalPlayer()->GetInstance() != owner->GetInstance() ) {
		FreeLight ( WPLIGHT_MUZZLEFLASH );
		FreeLight ( WPLIGHT_MUZZLEFLASH_WORLD );
		return;
	}

	renderLight_t& light	  = lights[WPLIGHT_MUZZLEFLASH];
	renderLight_t& lightWorld = lights[WPLIGHT_MUZZLEFLASH_WORLD];

	light.origin = playerViewOrigin + (playerViewAxis * muzzleFlashViewOffset);
	light.axis = playerViewAxis;

	// put the world muzzle flash on the end of the joint, no matter what
	GetGlobalJointTransform( false, flashJointWorld, lightWorld.origin, lightWorld.axis );

	UpdateLight ( WPLIGHT_MUZZLEFLASH );
	UpdateLight ( WPLIGHT_MUZZLEFLASH_WORLD );
}

/*
================
rvWeapon::UpdateFlashlight
================
*/
void rvWeapon::UpdateFlashlight ( void ) {
	// Turn flashlight off?
	if (! owner->IsFlashlightOn ( ) ) {
		FreeLight ( WPLIGHT_FLASHLIGHT );
		FreeLight ( WPLIGHT_FLASHLIGHT_WORLD );
		return;
	}

	renderLight_t& light	  = lights[WPLIGHT_FLASHLIGHT];
	renderLight_t& lightWorld = lights[WPLIGHT_FLASHLIGHT_WORLD];
	trace_t	tr;

	// the flash has an explicit joint for locating it
	GetGlobalJointTransform( true, flashlightJointView, light.origin, light.axis, flashlightViewOffset );

	// if the desired point is inside or very close to a wall, back it up until it is clear
	gameLocal.TracePoint( owner, tr, light.origin - playerViewAxis[0] * 8.0f, light.origin, MASK_SHOT_BOUNDINGBOX, owner );

	// be at least 8 units away from a solid
	light.origin = tr.endpos - (tr.fraction < 1.0f ? (playerViewAxis[0] * 8) : vec3_origin);

	// put the world muzzle flash on the end of the joint, no matter what
	if ( flashlightJointWorld != INVALID_JOINT ) {
		GetGlobalJointTransform( false, flashlightJointWorld, lightWorld.origin, lightWorld.axis );
	} else {
		lightWorld.origin = playerViewOrigin + playerViewAxis[0] * 20.0f;
		lightWorld.axis = playerViewAxis;
	}

	UpdateLight ( WPLIGHT_FLASHLIGHT );
	UpdateLight ( WPLIGHT_FLASHLIGHT_WORLD );
}

/*
================
rvWeapon::MuzzleFlash
================
*/
void rvWeapon::MuzzleFlash ( void ) {
	renderLight_t& light	  = lights[WPLIGHT_MUZZLEFLASH];
	renderLight_t& lightWorld = lights[WPLIGHT_MUZZLEFLASH_WORLD];

	if ( !g_muzzleFlash.GetBool() || flashJointView == INVALID_JOINT || !light.lightRadius[0] ) {
		return;
	}
	if ( g_perfTest_weaponNoFX.GetBool() ) {
		return;
	}

	if ( viewModel ) {
		// these will be different each fire
		light.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		light.shaderParms[ SHADERPARM_DIVERSITY ]	= viewModel->GetRenderEntity()->shaderParms[ SHADERPARM_DIVERSITY ];
		light.noShadows = true;

		lightWorld.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.time );
		lightWorld.shaderParms[ SHADERPARM_DIVERSITY ]	= viewModel->GetRenderEntity()->shaderParms[ SHADERPARM_DIVERSITY ];
		lightWorld.noShadows = true;

		// the light will be removed at this time
		muzzleFlashEnd = gameLocal.time + muzzleFlashTime;
	} else {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
	}
	UpdateMuzzleFlash ( );
}


/*
================
rvWeapon::UpdateGUI
================
*/
void rvWeapon::UpdateGUI( void ) {
	idUserInterface* gui;
	
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	gui = viewModel->GetRenderEntity()->gui[0];
	if ( !gui || status == WP_HOLSTERED ) {
		return;
	}
	int g;
for ( g = 0; g < MAX_RENDERENTITY_GUI && viewModel->GetRenderEntity()->gui[g]; g ++ ) {
	gui = viewModel->GetRenderEntity()->gui[g];

   	if ( gameLocal.localClientNum != owner->entityNumber ) {
		// if updating the hud for a followed client
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( !p ) {
			return;
		}
		if ( !p->spectating || p->spectator != owner->entityNumber ) {
			return;
		}
	}

	int ammo = AmmoInClip();
	if ( ammo >= 0 ) {
		// show remaining ammo
		if ( gui->State().GetInt ( "player_cachedammo", "-1") != ammo ) {
			gui->SetStateInt ( "player_ammo", ammo );
			
			if ( ClipSize ( ) ) {
				gui->SetStateFloat ( "player_ammopct", (float)ammo / (float)ClipSize() );
				gui->SetStateInt ( "player_clip_size", ClipSize() );
			} else { 
				gui->SetStateFloat ( "player_ammopct", (float)ammo / (float)maxAmmo );
				gui->SetStateInt ( "player_clip_size", maxAmmo );
			}
			gui->SetStateInt ( "player_cachedammo", ammo );
			gui->HandleNamedEvent ( "weapon_ammo" );
		}	
	}	

//		viewModel->GetRenderEntity()->gui[g]->SetStateInt ( "player_clip_size", ClipSize() );
}
	for ( int i = 0; i < viewModel->pendingGUIEvents.Num(); i ++ ) {
		gui->HandleNamedEvent( viewModel->pendingGUIEvents[i] );
	}
	viewModel->pendingGUIEvents.Clear();
}

/*
================
rvWeapon::UpdateCrosshairGUI
================
*/
void rvWeapon::UpdateCrosshairGUI( idUserInterface* gui ) const {	
// RAVEN BEGIN
// cnicholson: Added support for universal crosshair

	// COMMENTED OUT until Custom crosshair GUI is implemented.
	if ( g_crosshairCustom.GetBool() ) {										// If there's a custom crosshair, use it.
		gui->SetStateString( "crossImage", g_crosshairCustomFile.GetString());

		const idMaterial *material = declManager->FindMaterial( g_crosshairCustomFile.GetString() );
		if ( material ) {
			material->SetSort( SS_GUI );
		}			
	} else {
		gui->SetStateString( "crossImage", spawnArgs.GetString( "mtr_crosshair" ) );

		const idMaterial *material = declManager->FindMaterial( spawnArgs.GetString( "mtr_crosshair" ) );
		if ( material ) {
			material->SetSort( SS_GUI );
		}			
	}

// Original Block
	//gui->SetStateString ( "crossImage", spawnArgs.GetString ( "mtr_crosshair" ) );
// RAVEN END
	gui->SetStateString( "crossColor", g_crosshairColor.GetString() );
	gui->SetStateInt( "crossOffsetX", spawnArgs.GetInt( "crosshairOffsetX", "0" ) );
	gui->SetStateInt( "crossOffsetY", spawnArgs.GetInt( "crosshairOffsetY", "0" ) );
 	gui->StateChanged( gameLocal.time );
}

/*
================
rvWeapon::ForeshortenAxis
================
*/
idMat3 rvWeapon::ForeshortenAxis( const idMat3& axis ) const {
	return idMat3( axis[0] * viewModelForeshorten, axis[1], axis[2] );
}

/*
================
rvWeapon::GetAngleOffsets
================
*/
void rvWeapon::GetAngleOffsets ( int *average, float *scale, float *max ) {
	*average = weaponAngleOffsetAverages;
	*scale = weaponAngleOffsetScale;
	*max = weaponAngleOffsetMax;
}

/*
================
rvWeapon::GetTimeOffsets
================
*/
void rvWeapon::GetTimeOffsets ( float *time, float *scale ) {
	*time = weaponOffsetTime;
	*scale = weaponOffsetScale;
}

/*
================
rvWeapon::GetGlobalJointTransform

This returns the offset and axis of a weapon bone in world space, suitable for attaching models or lights
================
*/
bool rvWeapon::GetGlobalJointTransform ( bool view, const jointHandle_t jointHandle, idVec3 &origin, idMat3 &axis, const idVec3& offset ) {
	if ( view) {
		// view model
		if ( viewModel && viewAnimator->GetJointTransform( jointHandle, gameLocal.time, origin, axis ) ) {
			origin = offset * axis + origin;
			origin = origin * ForeshortenAxis(viewModelAxis) + viewModelOrigin;
			axis = axis * viewModelAxis;
			return true;
		}
	} else {
		// world model
		if ( worldModel && worldAnimator->GetJointTransform( jointHandle, gameLocal.time, origin, axis ) ) {
			origin = offset * axis + origin;
			origin = worldModel->GetPhysics()->GetOrigin() + origin * worldModel->GetPhysics()->GetAxis();
			axis = axis * worldModel->GetPhysics()->GetAxis();
			return true;
		}
	}
	origin = viewModelOrigin + offset * viewModelAxis;
	axis   = viewModelAxis;
	return false;
}

/***********************************************************************

	Ammo

***********************************************************************/

/*
================
rvWeapon::GetAmmoIndexForName
================
*/
int rvWeapon::GetAmmoIndexForName( const char *ammoname ) {
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

	if ( ( num < 0 ) || ( num >= MAX_AMMOTYPES ) ) {
		gameLocal.Error( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, MAX_AMMOTYPES );
	}

	return num;
}

/*
================
rvWeapon::GetAmmoNameForNum
================
*/
const char* rvWeapon::GetAmmoNameForIndex( int index ) {
	int i;
	int num;
	const idDict *ammoDict;
	const idKeyValue *kv;
	char text[ 32 ];

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	sprintf( text, "%d", index );

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
rvWeapon::TotalAmmoCount
================
*/
int rvWeapon::TotalAmmoCount ( void ) const {
	return owner->inventory.HasAmmo( ammoType, 1 );
}

/*
================
rvWeapon::AmmoAvailable
================
*/
int rvWeapon::AmmoAvailable( void ) const {
	if ( owner ) {
		return owner->inventory.HasAmmo( ammoType, ammoRequired );
	} else {
		return 0;
	}
}

/*
================
rvWeapon::AmmoInClip
================
*/
int rvWeapon::AmmoInClip( void ) const {
	if ( !clipSize ) {
		return AmmoAvailable();
	}
	return ammoClip;
}

/*
================
rvWeapon::ResetAmmoClip
================
*/
void rvWeapon::ResetAmmoClip( void ) {
	ammoClip = -1;
}

/*
================
rvWeapon::GetAmmoType
================
*/
int rvWeapon::GetAmmoType( void ) const {
	return ammoType;
}

/*
================
rvWeapon::ClipSize
================
*/
int	rvWeapon::ClipSize( void ) const {
	return clipSize;
}

/*
================
rvWeapon::LowAmmo
================
*/
int	rvWeapon::LowAmmo() const {
	return lowAmmo;
}

/*
================
rvWeapon::AmmoRequired
================
*/
int	rvWeapon::AmmoRequired( void ) const {
	return ammoRequired;
}

/*
================
rvWeapon::SetClip
================
*/
void rvWeapon::SetClip ( int amount ) {
	ammoClip = amount;
	if ( amount < 0 ) {
		ammoClip = 0;
	} else if ( amount > clipSize ) {
		ammoClip = clipSize;
	}
	
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	viewModel->PostGUIEvent ( "weapon_ammo" );
	if ( ammoClip == 0 && AmmoAvailable() == 0 ) {
		viewModel->PostGUIEvent ( "weapon_noammo" );
	}
}

/*
================
rvWeapon::UseAmmo
================
*/
void rvWeapon::UseAmmo( int amount ) {
	owner->inventory.UseAmmo( ammoType, amount * ammoRequired );
	if ( clipSize && ammoRequired ) {
		ammoClip -= ( amount * ammoRequired );
		if ( ammoClip < 0 ) {
			ammoClip = 0;
		}
	}
}

/*
================
rvWeapon::AddToClip
================
*/
void rvWeapon::AddToClip ( int amount ) {
	int ammoAvail;

 	if ( gameLocal.isClient ) {
 		return;
 	}

	ammoClip += amount;
	if ( ammoClip > clipSize ) {
		ammoClip = clipSize;
	}

	ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
	if ( ammoAvail > 0 && ammoClip > ammoAvail ) {
		ammoClip = ammoAvail;
	}

	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	viewModel->PostGUIEvent ( "weapon_ammo" );
	if ( ammoClip == 0 && AmmoAvailable() == 0 ) {
		viewModel->PostGUIEvent ( "weapon_noammo" );
	}
}

/***********************************************************************

	Attack

***********************************************************************/


/*
================
rvWeapon::Attack
================
*/
void rvWeapon::Attack( bool altAttack, int num_attacks, float spread, float fuseOffset, float power ) {
	idVec3 muzzleOrigin;
	idMat3 muzzleAxis;
	
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	if ( viewModel->IsHidden() ) {
		return;
	}

	// avoid all ammo considerations on an MP client
	if ( !gameLocal.isClient ) {
		// check if we're out of ammo or the clip is empty
		int ammoAvail = owner->inventory.HasAmmo( ammoType, ammoRequired );
		if ( !ammoAvail || ( ( clipSize != 0 ) && ( ammoClip <= 0 ) ) ) {
			return;
		}

		owner->inventory.UseAmmo( ammoType, ammoRequired );
		if ( clipSize && ammoRequired ) {
 			clipPredictTime = gameLocal.time;	// mp client: we predict this. mark time so we're not confused by snapshots
			ammoClip -= 1;
		}

		// wake up nearby monsters
		if ( !wfl.silent_fire ) {
			gameLocal.AlertAI( owner );
		}
	}

	// set the shader parm to the time of last projectile firing,
	// which the gun material shaders can reference for single shot barrel glows, etc
	viewModel->SetShaderParm ( SHADERPARM_DIVERSITY, gameLocal.random.CRandomFloat() );
	viewModel->SetShaderParm ( SHADERPARM_TIMEOFFSET, -MS2SEC( gameLocal.realClientTime ) );

	if ( worldModel.GetEntity() ) {
		worldModel->SetShaderParm( SHADERPARM_DIVERSITY, viewModel->GetRenderEntity()->shaderParms[ SHADERPARM_DIVERSITY ] );
		worldModel->SetShaderParm( SHADERPARM_TIMEOFFSET, viewModel->GetRenderEntity()->shaderParms[ SHADERPARM_TIMEOFFSET ] );
	}

	// calculate the muzzle position
	if ( barrelJointView != INVALID_JOINT && spawnArgs.GetBool( "launchFromBarrel" ) ) {
		// there is an explicit joint for the muzzle
		GetGlobalJointTransform( true, barrelJointView, muzzleOrigin, muzzleAxis );
	} else {
		// go straight out of the view
		muzzleOrigin = playerViewOrigin;
		muzzleAxis = playerViewAxis;		
		muzzleOrigin += playerViewAxis[0] * muzzleOffset;
	}

	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( kick_endtime < gameLocal.realClientTime ) {
		kick_endtime = gameLocal.realClientTime;
	}
	kick_endtime += muzzle_kick_time;
	if ( kick_endtime > gameLocal.realClientTime + muzzle_kick_maxtime ) {
		kick_endtime = gameLocal.realClientTime + muzzle_kick_maxtime;
	}

	// add the muzzleflash
	MuzzleFlash();

	// quad damage overlays a sound
	if ( owner->PowerUpActive( POWERUP_QUADDAMAGE ) ) {
		viewModel->StartSound( "snd_quaddamage", SND_CHANNEL_VOICE, 0, false, NULL );
	}

	// Muzzle flash effect
	bool muzzleTint = spawnArgs.GetBool( "muzzleTint" );
	viewModel->PlayEffect( "fx_muzzleflash", flashJointView, false, vec3_origin, false, false, EC_IGNORE, muzzleTint ? owner->GetHitscanTint() : vec4_one );

	if ( worldModel && flashJointWorld != INVALID_JOINT ) {
		worldModel->PlayEffect( gameLocal.GetEffect( weaponDef->dict, "fx_muzzleflash_world" ), flashJointWorld, vec3_origin, mat3_identity, false, vec3_origin, false, false, EC_IGNORE, muzzleTint ? owner->GetHitscanTint() : vec4_one );
	}

	owner->WeaponFireFeedback( &weaponDef->dict );

	// Inform the gui of the ammo change
	viewModel->PostGUIEvent ( "weapon_ammo" );
	if ( ammoClip == 0 && AmmoAvailable() == 0 ) {
		viewModel->PostGUIEvent ( "weapon_noammo" );
	}
	
	// The attack is either a hitscan or a launched projectile, do that now.
	if ( !gameLocal.isClient ) {
		idDict& dict = altAttack ? attackAltDict : attackDict;
		power *= owner->PowerUpModifier( PMOD_PROJECTILE_DAMAGE );
		if ( altAttack ? wfl.attackAltHitscan : wfl.attackHitscan ) {
			Hitscan( dict, muzzleOrigin, muzzleAxis, num_attacks, spread, power );
		} else {
			LaunchProjectiles( dict, muzzleOrigin, muzzleAxis, num_attacks, spread, fuseOffset, power );
		}
		//asalmon:  changed to keep stats even in single player 
		statManager->WeaponFired( owner, weaponIndex, num_attacks );
		
	}
}

/*
================
rvWeapon::LaunchProjectiles
================
*/
void rvWeapon::LaunchProjectiles ( idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_projectiles, float spread, float fuseOffset, float power ) {
	idProjectile*	proj;
	idEntity*		ent;
	int				i;
	float			spreadRad;
	idVec3			dir;
	idBounds		ownerBounds;

	if ( gameLocal.isClient ) {
		return;
	}
	
	// Let the AI know about the new attack
	if ( !gameLocal.isMultiplayer ) {
		aiManager.ReactToPlayerAttack ( owner, muzzleOrigin, muzzleAxis[0] );
	}
		
	ownerBounds = owner->GetPhysics()->GetAbsBounds();
	spreadRad   = DEG2RAD( spread );
	
	idVec3 dirOffset;
	idVec3 startOffset;

	spawnArgs.GetVector( "dirOffset", "0 0 0", dirOffset );
	spawnArgs.GetVector( "startOffset", "0 0 0", startOffset );

	for( i = 0; i < num_projectiles; i++ ) {
		float	 ang;
		float	 spin;
		idVec3	 dir;
		idBounds projBounds;
		idVec3	 muzzle_pos;
		
		// Calculate a random launch direction based on the spread
		ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
		dir += dirOffset;
		dir.Normalize();

		// If a projectile entity has already been created then use that one, otherwise
		// spawn a new one based on the given dictionary
		if ( projectileEnt ) {
			ent = projectileEnt;
			ent->Show();
			ent->Unbind();
			projectileEnt = NULL;
		} else {
			dict.SetInt( "instance", owner->GetInstance() );
			gameLocal.SpawnEntityDef( dict, &ent, false );
		}

		// Make sure it spawned
		if ( !ent ) {
			gameLocal.Error( "failed to spawn projectile for weapon '%s'", weaponDef->GetName ( ) );
		}
		
		assert ( ent->IsType( idProjectile::GetClassType() ) );

		// Create the projectile
		proj = static_cast<idProjectile*>(ent);
		proj->Create( owner, muzzleOrigin + startOffset, dir, NULL, owner->extraProjPassEntity );

		projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

		// make sure the projectile starts inside the bounding box of the owner
		if ( i == 0 ) {
			idVec3  start;
			float   distance;
			trace_t	tr;
//RAVEN BEGIN
//asalmon: xbox must use muzzle Axis for aim assistance
#ifdef _XBOX
			muzzle_pos = muzzleOrigin + muzzleAxis[ 0 ] * 2.0f;
			if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, muzzleAxis[0], distance ) ) {
				start = muzzle_pos + distance * muzzleAxis[0];
			} 
#else
			muzzle_pos = muzzleOrigin + playerViewAxis[ 0 ] * 2.0f;
			if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, playerViewAxis[0], distance ) ) {
				start = muzzle_pos + distance * playerViewAxis[0];
			} 
#endif
//RAVEN END
			else {
				start = ownerBounds.GetCenter();
			}
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			gameLocal.Translation( owner, tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
// RAVEN END
			muzzle_pos = tr.endpos;
		}
		
		// Launch the actual projectile
		proj->Launch( muzzle_pos + startOffset, dir, pushVelocity, fuseOffset, power );
		
		// Increment the projectile launch count and let the derived classes
		// mess with it if they want.
		OnLaunchProjectile ( proj );
	}
}

/*
================
rvWeapon::OnLaunchProjectile
================
*/
void rvWeapon::OnLaunchProjectile ( idProjectile* proj ) {
	owner->AddProjectilesFired( 1 );
	if ( proj ) {
		proj->methodOfDeath = owner->GetCurrentWeapon();
	}
}

/*
================
rvWeapon::Hitscan
================
*/
void rvWeapon::Hitscan( const idDict& dict, const idVec3& muzzleOrigin, const idMat3& muzzleAxis, int num_hitscans, float spread, float power ) {
	idVec3  fxOrigin;
	idMat3  fxAxis;
	int		i;
	float	ang;
	float	spin;
	idVec3	dir;
	int		areas[ 2 ];

	idBitMsg	msg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	// Let the AI know about the new attack
	if ( !gameLocal.isMultiplayer ) {
		aiManager.ReactToPlayerAttack( owner, muzzleOrigin, muzzleAxis[0] );
	}

	GetGlobalJointTransform( true, flashJointView, fxOrigin, fxAxis, dict.GetVector( "fxOriginOffset" ) );

	if ( gameLocal.isServer ) {

		assert( hitscanAttackDef >= 0 );
		assert( owner && owner->entityNumber < MAX_CLIENTS );
		int ownerId = owner ? owner->entityNumber : 0;

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( GAME_UNRELIABLE_MESSAGE_HITSCAN );
		msg.WriteLong( hitscanAttackDef );
		msg.WriteBits( ownerId, idMath::BitsForInteger( MAX_CLIENTS ) );
		msg.WriteFloat( muzzleOrigin[0] );
		msg.WriteFloat( muzzleOrigin[1] );
		msg.WriteFloat( muzzleOrigin[2] );
		msg.WriteFloat( fxOrigin[0] );
		msg.WriteFloat( fxOrigin[1] );
		msg.WriteFloat( fxOrigin[2] );
	}

	float spreadRad = DEG2RAD( spread );
	idVec3 end;
	for( i = 0; i < num_hitscans; i++ ) {
		if( weaponDef->dict.GetBool( "machinegunSpreadStyle" ) ) {	
			float r = gameLocal.random.RandomFloat() * idMath::PI * 2.0f;
			float u = idMath::Sin( r ) * gameLocal.random.CRandomFloat() * spread * 16;
			r = idMath::Cos( r ) * gameLocal.random.CRandomFloat() * spread * 16;
#ifdef _XBOX
			end = muzzleOrigin + ( ( 8192 * 16 ) * muzzleAxis[ 0 ] );
			end += ( r * muzzleAxis[ 1 ] );
			end += ( u * muzzleAxis[ 2 ] );
#else
			end = muzzleOrigin + ( ( 8192 * 16 ) * playerViewAxis[ 0 ] );
			end += ( r * playerViewAxis[ 1 ] );
			end += ( u * playerViewAxis[ 2 ] );
#endif
			dir = end - muzzleOrigin;
		} else if( weaponDef->dict.GetBool( "shotgunSpreadStyle" ) ) {
			int radius;
			float angle;

			// this may look slightly odd, but ensures with an odd number of pellets we get
			// two complete circles, and the outer one gets the extra hit
			int circleHitscans = num_hitscans - (num_hitscans / 2);
			if (i < circleHitscans)
			{
				radius = spread * 14;
				angle = i * (idMath::TWO_PI / circleHitscans);
			}
			else
			{
				radius = spread * 6;
				angle = (i - circleHitscans) * (idMath::TWO_PI / num_hitscans) * 2;
			}
			
			float r = radius * (idMath::Cos(angle) + (gameLocal.random.CRandomFloat() * 0.2f));
			float u = radius * (idMath::Sin(angle) + (gameLocal.random.CRandomFloat() * 0.2f));
			
#ifdef _XBOX
			end = muzzleOrigin + ( ( 8192 * 16 ) * muzzleAxis[ 0 ] );
			end += ( r * muzzleAxis[ 1 ] );
			end += ( u * muzzleAxis[ 2 ] );
#else
			end = muzzleOrigin + ( ( 8192 * 16 ) * playerViewAxis[ 0 ] );
			end += ( r * playerViewAxis[ 1 ] );
			end += ( u * playerViewAxis[ 2 ] );
#endif
			dir = end - muzzleOrigin;
		} else {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			//RAVEN BEGIN
			//asalmon: xbox must use the muzzleAxis so the aim can be adjusted for aim assistance
#ifdef _XBOX
			dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - muzzleAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
#else
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
#endif
			//RAVEN END
		}
		dir.Normalize();

		gameLocal.HitScan( dict, muzzleOrigin, dir, fxOrigin, owner, false, 1.0f, NULL, areas );

		if ( gameLocal.isServer ) {
			msg.WriteDir( dir, 24 );
			if ( i == num_hitscans - 1 ) {
				// NOTE: we emit to the areas of the last hitscan
				// there is a remote possibility that multiple hitscans for shotgun would cover more than 2 areas,
				// so in some rare case a client might miss it
				gameLocal.SendUnreliableMessagePVS( msg, owner, areas[0], areas[1] );
			}
		}
	}
}

/*
================
rvWeapon::AlertMonsters
================
*/
void rvWeapon::AlertMonsters( void ) {
	trace_t	tr;
	idEntity *ent;
	idVec3 end;
	renderLight_t& muzzleFlash = lights[WPLIGHT_MUZZLEFLASH];
	
	end = muzzleFlash.origin + muzzleFlash.axis * muzzleFlash.target;
 	gameLocal.TracePoint( owner, tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
 		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
 		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::GetClassType() ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::GetClassType() ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}

	// jitter the trace to try to catch cases where a trace down the center doesn't hit the monster
	end += muzzleFlash.axis * muzzleFlash.right * idMath::Sin16( MS2SEC( gameLocal.time ) * 31.34f );
	end += muzzleFlash.axis * muzzleFlash.up * idMath::Sin16( MS2SEC( gameLocal.time ) * 12.17f );
	gameLocal.TracePoint( owner, tr, muzzleFlash.origin, end, CONTENTS_OPAQUE | MASK_SHOT_RENDERMODEL | CONTENTS_FLASHLIGHT_TRIGGER, owner );
	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, muzzleFlash.origin, end, 0 );
 		gameRenderWorld->DebugArrow( colorGreen, muzzleFlash.origin, tr.endpos, 2, 0 );
	}

	if ( tr.fraction < 1.0f ) {
 		ent = gameLocal.GetTraceEntity( tr );
		if ( ent->IsType( idAI::GetClassType() ) ) {
			static_cast<idAI *>( ent )->TouchedByFlashlight( owner );
		} else if ( ent->IsType( idTrigger::GetClassType() ) ) {
			ent->Signal( SIG_TOUCH );
			ent->ProcessEvent( &EV_Touch, owner, &tr );
		}
	}
}

/*
================
rvWeapon::EjectBrass
================
*/
void rvWeapon::EjectBrass ( void ) {
	if ( g_brassTime.GetFloat() <= 0.0f || !owner->CanShowWeaponViewmodel() ) {
		return;
	}

	if ( g_perfTest_weaponNoFX.GetBool() ) {
		return;
	}

	if ( gameLocal.isMultiplayer ) {
		return;
	}

	if ( ejectJointView == INVALID_JOINT || !brassDict.GetNumKeyVals() ) {
		return;
	}

	idMat3 axis;
	idVec3 origin;
	idVec3 linear_velocity;
	idVec3 angular_velocity;
	int	   brassTime;

	if ( !GetGlobalJointTransform( true, ejectJointView, origin, axis ) ) {
		return;
	}

	// Spawn the client side moveable for the brass
	rvClientMoveable* cent = NULL;

	gameLocal.SpawnClientEntityDef( brassDict, (rvClientEntity**)(&cent), false );

	if( !cent ) {
		return;
	}

	cent->SetOwner( GetOwner() );
	cent->SetOrigin ( origin + playerViewAxis * ejectOffset );
	cent->SetAxis ( playerViewAxis );	
	
	// Depth hack the brass to make sure it clips in front of view weapon properly
	cent->GetRenderEntity()->weaponDepthHackInViewID = GetOwner()->entityNumber + 1;
	
	// Clear the depth hack soon after it clears the view
	cent->PostEventMS ( &CL_ClearDepthHack, 200 );
	
	// Fade the brass out so they dont accumulate
	brassTime =(int)SEC2MS(g_brassTime.GetFloat() / 2.0f);
	cent->PostEventMS ( &CL_FadeOut, brassTime, brassTime );

	// Generate a good velocity for the brass
	idVec3 linearVelocity = brassDict.GetVector("linear_velocity").Random( brassDict.GetVector("linear_velocity_range"), gameLocal.random );
	cent->GetPhysics()->SetLinearVelocity( GetOwner()->GetPhysics()->GetLinearVelocity() + linearVelocity * cent->GetPhysics()->GetAxis() );
	idAngles angularVelocity = brassDict.GetAngles("angular_velocity").Random( brassDict.GetVector("angular_velocity_range"), gameLocal.random );
	cent->GetPhysics()->SetAngularVelocity( angularVelocity.ToAngularVelocity() * cent->GetPhysics()->GetAxis() );
}

/*
================
rvWeapon::BloodSplat
================
*/
bool rvWeapon::BloodSplat( float size ) {
	float s, c;
	idMat3 localAxis, axistemp;
	idVec3 localOrigin, normal;

	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return false;
	}
	
	if ( wfl.hasBloodSplat ) {
		return true;
	}

	wfl.hasBloodSplat = true;

	if ( viewModel->modelDefHandle < 0 ) {
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

	gameRenderWorld->ProjectOverlay( viewModel->modelDefHandle, localPlane, mtr );

	return true;
}

/*
================
rvWeapon::EnterCinematic
================
*/
void rvWeapon::EnterCinematic( void ) {
	if( viewModel ) {
		viewModel->StopSound( SND_CHANNEL_ANY, false );
	}
	ExecuteState( "EnterCinematic" );

	memset( &wsfl, 0, sizeof(wsfl) );

	wfl.disabled = true;

	LowerWeapon();
}

/*
================
rvWeapon::ExitCinematic
================
*/
void rvWeapon::ExitCinematic( void ) {
	wfl.disabled = false;
	ExecuteState ( "ExitCinematic" );
	RaiseWeapon();
}

/*
================
rvWeapon::NetCatchup
================
*/
void rvWeapon::NetCatchup( void ) {
	ExecuteState ( "NetCatchup" );
}
   
/*
===============
rvWeapon::PlayAnim
===============
*/
void rvWeapon::PlayAnim( int channel, const char *animname, int blendFrames ) {
	
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	int	anim;
	
	anim = viewAnimator->GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, viewModel->GetName(), viewModel->GetEntityDefName() );
		viewAnimator->Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = 0;
	} else {
		viewModel->Show();
		viewAnimator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = viewAnimator->CurrentAnim( channel )->GetEndTime();
		
		// Play the animation on the world model as well
		if ( worldAnimator ) {
			worldAnimator->GetAnim( animname );
			if ( anim ) {
				worldAnimator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
			}
		}
	}
}

/*
===============
rvWeapon::PlayCycle
===============
*/
void rvWeapon::PlayCycle( int channel, const char *animname, int blendFrames ) {
	int anim;
	
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return;
	}
	
	anim = viewAnimator->GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, viewModel->GetName(), viewModel->GetEntityDefName() );
		viewAnimator->Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = 0;
	} else {
		viewModel->Show();
		viewAnimator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = viewAnimator->CurrentAnim( channel )->GetEndTime();

		// Play the animation on the world model as well
		if ( worldAnimator ) {
			anim = worldAnimator->GetAnim( animname );
			if ( anim ) {
				worldAnimator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
			}
		}
	}
}

/*
===============
rvWeapon::AnimDone
===============
*/
bool rvWeapon::AnimDone( int channel, int blendFrames ) {
	if ( animDoneTime[channel] - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		return true;
	}
	return false;
}

/*
===============
rvWeapon::StartSound
===============
*/
bool rvWeapon::StartSound ( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	if ( !viewModel ) {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
		return false;
	}
	return viewModel->StartSound( soundName, channel, soundShaderFlags, broadcast, length );
}

/*
===============
rvWeapon::StopSound
===============
*/
void rvWeapon::StopSound( const s_channelType channel, bool broadcast ) {
	if ( viewModel ) {
		viewModel->StopSound( channel, broadcast );
	} else {
		common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
	}
}

/*
===============
rvWeapon::PlayEffect
===============
*/
rvClientEffect* rvWeapon::PlayEffect( const char* effectName, jointHandle_t joint, bool loop, const idVec3& endOrigin, bool broadcast ) {
	if ( viewModel ) {
		return viewModel->PlayEffect( effectName, joint, loop, endOrigin, broadcast );
	}
	
	common->Warning( "NULL viewmodel %s\n", __FUNCTION__ );
	return 0;
}

/*
================
rvWeapon::CacheWeapon
================
*/
void rvWeapon::CacheWeapon( const char *weaponName ) {
	const idDeclEntityDef *weaponDef;
	const char *brassDefName;
	const char *clipModelName;
	idTraceModel trm;

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
			if ( idStr::Icmp( clipModelName, SIMPLE_TRI_NAME ) == 0 ) {
				trm.SetupPolygon( simpleTri, 3 );
			} else {
				if ( !clipModelName[0] ) {
					clipModelName = brassDef->dict.GetString( "model" );		// use the visual model
				}
				// load the trace model
				collisionModelManager->TrmFromModel( gameLocal.GetMapName(), clipModelName, trm );
			}
		}
	}

	const idKeyValue* kv;

	kv = weaponDef->dict.MatchPrefix( "gui", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
		uiManager->FindGui( kv->GetValue().c_str(), true, false, true );
		}
		kv = weaponDef->dict.MatchPrefix( "gui", kv );
	}
}


/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvWeapon )
	STATE ( "Raise",			rvWeapon::State_Raise )
	STATE ( "Lower",			rvWeapon::State_Lower )
	STATE ( "ExitCinematic",	rvWeapon::State_ExitCinematic )
	STATE ( "NetCatchup",		rvWeapon::State_NetCatchup )
	STATE ( "EjectBrass",		rvWeapon::Frame_EjectBrass )
END_CLASS_STATES

/*
================
rvWeapon::State_Raise

Raise the weapon
================
*/
stateResult_t rvWeapon::State_Raise ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};	
	switch ( parms.stage ) {
		// Start the weapon raising
		case STAGE_INIT:
			SetStatus ( WP_RISING );
			PlayAnim( ANIMCHANNEL_ALL, "raise", 0 );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 4 ) ) {
				SetState ( "Idle", 4 );
				return SRESULT_DONE;
			}
			if ( wsfl.lowerWeapon ) {
				SetState ( "Lower", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvWeapon::State_Lower

Lower the weapon
================
*/
stateResult_t rvWeapon::State_Lower ( const stateParms_t& parms ) {	
	enum {
		STAGE_INIT,
		STAGE_WAIT,
		STAGE_WAITRAISE
	};	
	switch ( parms.stage ) {
		case STAGE_INIT:
			SetStatus ( WP_LOWERING );
			PlayAnim ( ANIMCHANNEL_ALL, "putaway", parms.blendFrames );
			return SRESULT_STAGE(STAGE_WAIT);
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_ALL, 0 ) ) {
				SetStatus ( WP_HOLSTERED );
				return SRESULT_STAGE(STAGE_WAITRAISE);
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITRAISE:
			if ( wsfl.raiseWeapon ) {
				SetState ( "Raise", 0 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
=====================
rvWeapon::State_ExitCinematic
=====================
*/
stateResult_t rvWeapon::State_NetCatchup ( const stateParms_t& parms ) {
	SetState ( "idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
=====================
rvWeapon::State_ExitCinematic
=====================
*/
stateResult_t rvWeapon::State_ExitCinematic ( const stateParms_t& parms ) {
	SetState ( "Idle", 0 );
	return SRESULT_DONE;
}

/*
=====================
rvWeapon::Frame_EjectBrass
=====================
*/
stateResult_t rvWeapon::Frame_EjectBrass( const stateParms_t& parms ) {
	EjectBrass();
	return SRESULT_DONE;
}

/*
=====================
rvWeapon::GetDebugInfo
=====================
*/
void rvWeapon::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	idClass::GetDebugInfo ( proc, userData );
	proc ( "rvWeapon", "state",	stateThread.GetState()?stateThread.GetState()->state->name : "<none>", userData );
}
