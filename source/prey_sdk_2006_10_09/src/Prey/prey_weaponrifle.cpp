#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhWeaponZoomable
	
***********************************************************************/
const idEventDef EV_Weapon_ZoomIn( "zoomIn" );
const idEventDef EV_Weapon_ZoomOut( "zoomOut" );

CLASS_DECLARATION( hhWeapon, hhWeaponZoomable )
	EVENT( EV_Weapon_ZoomIn,			hhWeaponZoomable::Event_ZoomIn )
	EVENT( EV_Weapon_ZoomOut,			hhWeaponZoomable::Event_ZoomOut )
END_CLASS

/*
================
hhWeaponZoomable::ZoomIn
================
*/
void hhWeaponZoomable::ZoomIn() {
	if( owner.IsValid() && dict ) {
		owner->GetZoomFov().Init( gameLocal.GetTime(), SEC2MS(dict->GetFloat("zoomDuration")), owner->CalcFov(true), GetZoomFov() );
	}

	clientZoomTime = gameLocal.time + CLIENT_ZOOM_FUDGE; //rww

	bZoomed = true;
}

/*
================
hhWeaponZoomable::ZoomOut
================
*/
void hhWeaponZoomable::ZoomOut() {
	if( owner.IsValid() && dict ) {
		owner->GetZoomFov().Init( gameLocal.GetTime(), SEC2MS(dict->GetFloat("zoomDuration")), owner->CalcFov(true), g_fov.GetInteger() );
	}

	clientZoomTime = gameLocal.time + CLIENT_ZOOM_FUDGE; //rww

	bZoomed = false;
}

/*
================
hhWeaponZoomable::ZoomIn
================
*/
void hhWeaponZoomable::ZoomInStep() {
	int maxFov = spawnArgs.GetInt( "zoomFovMax" );
	if ( zoomFov < maxFov ) {
		zoomFov += spawnArgs.GetInt( "zoomFovStep" );
		StartSound( "snd_scope_click", SND_CHANNEL_ANY, 0, false, NULL );
		if ( zoomFov > maxFov ) {
			zoomFov = maxFov;
		}
		ZoomIn();
	}
	bZoomed = true;
	owner->inventory.zoomFov = zoomFov;
}

/*
================
hhWeaponZoomable::ZoomOut
================
*/
void hhWeaponZoomable::ZoomOutStep() {
	int minFov = spawnArgs.GetInt( "zoomFovMin" );
	if ( zoomFov > minFov ) {
		zoomFov -= spawnArgs.GetInt( "zoomFovStep" );
		StartSound( "snd_scope_click", SND_CHANNEL_ANY, 0, false, NULL );
		if ( zoomFov < minFov ) {
			zoomFov = minFov;
		}
		ZoomIn();
	}
	bZoomed = true;
	owner->inventory.zoomFov = zoomFov;
}

/*
================
hhWeaponZoomable::Event_ZoomIn
================
*/
void hhWeaponZoomable::Event_ZoomIn() {
	ZoomIn();
}

/*
================
hhWeaponZoomable::Event_ZoomOut
================
*/
void hhWeaponZoomable::Event_ZoomOut() {
	ZoomOut();
}

/*
================
hhWeaponZoomable::WriteToSnapshot
================
*/
void hhWeaponZoomable::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhWeapon::WriteToSnapshot(msg);

    msg.WriteBits(renderEntity.suppressSurfaceInViewID, GENTITYNUM_BITS);	//if suppressSurfaceInViewID were to equal the last ent+1 this would be bad
																			//but, since it should never be world/none, this should be fine.
	msg.WriteBits(bZoomed, 1);
	msg.WriteBits(WEAPON_ALTMODE, 1);

	msg.WriteBits(IsHidden(), 1);
}

/*
================
hhWeaponZoomable::ReadFromSnapshot
================
*/
void hhWeaponZoomable::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhWeapon::ReadFromSnapshot(msg);

	int suppressSurfInViewID = msg.ReadBits(GENTITYNUM_BITS);
	bool zoomed = !!msg.ReadBits(1);
	bool weaponAltMode = !!msg.ReadBits(1);
	bool hidden = !!msg.ReadBits(1);
	if (clientZoomTime < gameLocal.time) {
		renderEntity.suppressSurfaceInViewID = suppressSurfInViewID;
		bZoomed = zoomed;
		WEAPON_ALTMODE = weaponAltMode;
		if (hidden != IsHidden()) {
			if (hidden) {
				Hide();
			}
			else {
				Show();
			}
		}
	}
}

/*
================
hhWeaponZoomable::Save
================
*/
void hhWeaponZoomable::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( bZoomed );
}

/*
================
hhWeaponZoomable::Restore
================
*/
void hhWeaponZoomable::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( bZoomed );
}

/***********************************************************************

  hhWeaponRifle
	
***********************************************************************/
//This probably should be on entity but since no body else needs it I'll leave it here
const idEventDef EV_SuppressSurfaceInViewID( "<suppressSurfaceInViewID>", "d" );

CLASS_DECLARATION( hhWeaponZoomable, hhWeaponRifle )
	EVENT( EV_SuppressSurfaceInViewID,	hhWeaponRifle::Event_SuppressSurfaceInViewID )
END_CLASS

/*
================
hhWeaponRifle::Spawn
================
*/
void hhWeaponRifle::Spawn(void) {
	zoomOverlayGui = NULL;
	clientZoomTime = 0; //rww - no need to save/restore this
	fl.clientEvents = true; //rww - for "prediction"
}

/*
================
hhWeaponRifle::~hhWeaponRifle
================
*/
hhWeaponRifle::~hhWeaponRifle(void) {
	// Ensure that the scope view is disabled anytime the rifle goes away
	ZoomOut();

	zoomOverlayGui = NULL;

	SAFE_REMOVE( laserSight );
}

/*
================
hhWeaponRifle::ParseDef
================
*/
void hhWeaponRifle::ParseDef( const char* objectname ) {
	hhWeaponZoomable::ParseDef( objectname ); 

	SAFE_REMOVE(laserSight);

	//rww - client-server-localized.
	laserSight = static_cast<hhBeamSystem*>( gameLocal.SpawnClientObject(dict->GetString("beam_laserSight"), NULL) );
	if ( laserSight.IsValid() ) { // cjr - Don't allow the laser show up in the player's zoomed view
		assert(owner.IsValid());
		laserSight->GetRenderEntity()->suppressSurfaceInViewID = owner->entityNumber + 1;
		laserSight->fl.neverDormant = true;
		laserSight->snapshotOwner = this; //rww
	}

	DeactivateLaserSight();

	if( owner->inventory.zoomFov )
		zoomFov = owner->inventory.zoomFov;
	WEAPON_ALTMODE = false;
}

/*
================
hhWeaponRifle::Ticker
================
*/
void hhWeaponRifle::Ticker() {
	trace_t		trace;
	idVec3		start;
	idMat3		axis;

	if( laserSight.IsValid() && laserSight->IsActive() ) {
		GetJointWorldTransform( dict->GetString("joint_laserSight"), start, axis );
		gameLocal.clip.TracePoint( trace, start, start + GetAxis()[0] * CM_MAX_TRACE_DIST, MASK_VISIBILITY, owner.GetEntity() );
		laserSight->SetOrigin( start );
		laserSight->SetTargetLocation( trace.endpos );
	}

	hhWeaponZoomable::Ticker();
}

/*
================
hhWeaponRifle::ZoomIn
================
*/
void hhWeaponRifle::ZoomIn() {
	CancelEvents( &EV_SuppressSurfaceInViewID );

	bool alreadyZoomed = bZoomed;

	hhWeaponZoomable::ZoomIn();

	SetViewAnglesSensitivity( owner->GetZoomFov().GetEndValue() );

	ProcessEvent( &EV_SuppressSurfaceInViewID, owner->entityNumber + 1 );
	RestoreGUI( "gui_zoomOverlay", &zoomOverlayGui );

	if (zoomOverlayGui && !alreadyZoomed) {
		zoomOverlayGui->HandleNamedEvent("zoomIn");

		// Display Tips
		if (!gameLocal.isMultiplayer && g_tips.GetBool()) {
			gameLocal.SetTip(zoomOverlayGui, "_impulse15", "#str_41156", NULL, NULL, "tip1");
			gameLocal.SetTip(zoomOverlayGui, "_impulse14", "#str_41157", NULL, NULL, "tip2");
			zoomOverlayGui->HandleNamedEvent( "tipWindowUp" );
		}
	}

	ActivateLaserSight();

	if (owner.IsValid() && owner.GetEntity()) {
		if (owner->entityNumber == gameLocal.localClientNum) {
			renderSystem->SetScopeView( true ); // CJR:  Enable special render scope view
		}
		owner->bScopeView = true;
	}
}

/*
================
hhWeaponRifle::ZoomOut
================
*/
void hhWeaponRifle::ZoomOut() {
	CancelEvents( &EV_SuppressSurfaceInViewID );

	bool wasZoomed = bZoomed;

	hhWeaponZoomable::ZoomOut();

	if (owner.IsValid() && owner.GetEntity()) { //rww - this seems to be possible when a player disconnects while zoomed i think
		SetViewAnglesSensitivity( owner->GetZoomFov().GetEndValue() );
	}

	// Remove tips
	if (zoomOverlayGui) {
		zoomOverlayGui->HandleNamedEvent( "tipWindowDown" );
	}

	if ( dict ) {
		PostEventMS( &EV_SuppressSurfaceInViewID, SEC2MS(dict->GetFloat("zoomDuration")) * 0.5f, 0 );
	}
	zoomOverlayGui = NULL;

	if( wasZoomed ) {
		SetShaderParm(7, -MS2SEC(gameLocal.time));
	}

	DeactivateLaserSight();

	if (owner.IsValid() && owner.GetEntity()) {
		if (owner->entityNumber == gameLocal.localClientNum) {
			renderSystem->SetScopeView( false ); // CJR:  Disable special render scope view
		}
		owner->bScopeView = false;
	}
}

/*
================
hhWeaponRifle::ActivateLaserSight
================
*/
void hhWeaponRifle::ActivateLaserSight() {
	if( laserSight.IsValid() ) {
		laserSight->Activate( true );
		BecomeActive( TH_TICKER );
	}
}

/*
================
hhWeaponRifle::DeactivateLaserSight
================
*/
void hhWeaponRifle::DeactivateLaserSight() {
	if( laserSight.IsValid() ) {
		laserSight->Activate( false );
		BecomeInactive( TH_TICKER );
	}
}

/*
================
hhWeaponRifle::UpdateGUI
================
*/
void hhWeaponRifle::UpdateGUI() {
	float parmValue = 0.0f;
	idAngles angles;

	if( zoomOverlayGui ) {//This is for the sniper
		float minFov = spawnArgs.GetFloat( "zoomFovMin" );
		float maxFov = spawnArgs.GetFloat( "zoomFovMax" );
		parmValue = idMath::ClampFloat(0.3f, 1.0f, 1.0f - 0.7f*(zoomFov - minFov)/(maxFov - minFov));
		zoomOverlayGui->SetStateFloat( "fov", parmValue );

		angles = GetAxis().ToAngles();
		parmValue = angles.Normalize360().yaw;
		zoomOverlayGui->SetStateFloat( "yaw", parmValue );
		zoomOverlayGui->StateChanged( gameLocal.time );
		zoomOverlayGui->Redraw( gameLocal.GetTime() );
	}
}

/*
================
hhWeaponRifle::Event_SuppressSurfaceInViewID
================
*/
void hhWeaponRifle::Event_SuppressSurfaceInViewID( const int id ) {
	renderEntity.suppressSurfaceInViewID = id;
}

/*
================
hhWeaponRifle::Save
================
*/
void hhWeaponRifle::Save( idSaveGame *savefile ) const {
	laserSight.Save( savefile );
	savefile->WriteUserInterface( zoomOverlayGui, false );
}

/*
================
hhWeaponRifle::Restore
================
*/
void hhWeaponRifle::Restore( idRestoreGame *savefile ) {
	laserSight.Restore( savefile );
	savefile->ReadUserInterface( zoomOverlayGui );
}

/*
================
hhWeaponRifle::WriteToSnapshot
================
*/
void hhWeaponRifle::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhWeaponZoomable::WriteToSnapshot(msg);

	bool laserSightActive = (laserSight.IsValid() && IsActive(TH_TICKER));
	msg.WriteBits(laserSightActive, 1);

	if (zoomOverlayGui) {
		msg.WriteBits(1, 1);
	}
	else {
		msg.WriteBits(0, 1);
	}
}

/*
================
hhWeaponRifle::ReadFromSnapshot
================
*/
void hhWeaponRifle::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhWeaponZoomable::ReadFromSnapshot(msg);

	bool laserSightActive = !!msg.ReadBits(1);
	if (laserSight.IsValid() && IsActive(TH_TICKER) != laserSightActive) {
		if (laserSightActive) {
			ActivateLaserSight();
		}
		else {
			DeactivateLaserSight();
		}
	}

	bool guiActive = !!msg.ReadBits(1);
	if (clientZoomTime < gameLocal.time) { 
		if ((!!zoomOverlayGui) != guiActive) {
			if (guiActive) {
				if (!zoomOverlayGui) {
					RestoreGUI( "gui_zoomOverlay", &zoomOverlayGui );
					if (zoomOverlayGui) {
						zoomOverlayGui->HandleNamedEvent("zoomIn");
					}
				}
			}
			else {
				if (zoomOverlayGui) {
					zoomOverlayGui = NULL;
				}
			}
		}
	}
}

CLASS_DECLARATION( hhWeaponFireController, hhSniperRifleFireController )
END_CLASS

void hhSniperRifleFireController::CalculateMuzzlePosition( idVec3& origin, idMat3& axis ) {
	origin = owner->GetEyePosition();
	axis = self->GetAxis();
}