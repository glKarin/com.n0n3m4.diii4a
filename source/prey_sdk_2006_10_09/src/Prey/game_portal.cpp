//**************************************************************************
//**
//** GAME_PORTAL.CPP
//**
//** Game code for Prey-specific portals
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define MP_PORTAL_RANGE_DEFAULT		356.0f //rww - was 512, and then it was 256, and now it is 356
#define MAX_PORTAL_BOUNDS			256.0f //used for unchanging bounds in mp

//==========================================================================
//
// hhArtificialPortal
//
//==========================================================================

const idEventDef EV_SetGamePortalState("setPortalState", "dd");

#if GAMEPORTAL_PVS

CLASS_DECLARATION(idEntity, hhArtificialPortal)
	EVENT( EV_SetGamePortalState,	hhArtificialPortal::Event_SetPortalState )
END_CLASS

void hhArtificialPortal::Spawn() {
	areaPortal = gameRenderWorld->FindGamePortal( GetName() );
	if (areaPortal && !gameLocal.isClient) {
		SetPortalState(spawnArgs.GetBool("startOpenPVS"), spawnArgs.GetBool("startOpenSound"));
	}
	fl.networkSync = true;
}

void hhArtificialPortal::SetPortalState(bool openPVS, bool openSound) {
	if (areaPortal && !gameLocal.isClient) {
		int blockMask = (openPVS ? PS_BLOCK_NONE : PS_BLOCK_VIEW) | (openSound ? PS_BLOCK_NONE : PS_BLOCK_SOUND);
		gameLocal.SetPortalState( areaPortal, blockMask );
	}
}

void hhArtificialPortal::Event_SetPortalState(bool openPVS, bool openSound) {
	SetPortalState(openPVS, openSound);
}

void hhArtificialPortal::Save(idSaveGame *savefile) const {
	savefile->WriteInt(areaPortal);
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}
}

void hhArtificialPortal::Restore(idRestoreGame *savefile) {
	savefile->ReadInt(areaPortal);
	if ( areaPortal ) {
		int portalState;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}
}


#endif


//==========================================================================
//
// hhPortal
//
//==========================================================================

const idEventDef EV_Opened("<portalopened>", NULL);
const idEventDef EV_Closed("<portalclosed>", NULL);
const idEventDef EV_PortalSpark("<portalSpark>", NULL );
const idEventDef EV_PortalSparkEnd("<portalSparkEnd>", NULL );
const idEventDef EV_ShowGlowPortal( "showGlowPortal", NULL );
const idEventDef EV_HideGlowPortal( "hideGlowPortal", NULL );

CLASS_DECLARATION(hhAnimatedEntity, hhPortal)
	EVENT( EV_PostSpawn,   			hhPortal::PostSpawn )
	EVENT( EV_Activate,	   			hhPortal::Event_Trigger)
	EVENT( EV_Opened,				hhPortal::Event_Opened )
	EVENT( EV_Closed,				hhPortal::Event_Closed )
	EVENT( EV_ResetGravity,			hhPortal::Event_ResetGravity )
	EVENT( EV_PortalSpark,			hhPortal::Event_PortalSpark )
	EVENT( EV_PortalSparkEnd,		hhPortal::Event_PortalSparkEnd )
	EVENT( EV_ShowGlowPortal,		hhPortal::Event_ShowGlowPortal )
	EVENT( EV_HideGlowPortal,		hhPortal::Event_HideGlowPortal )
END_CLASS

hhPortal::hhPortal(void) {
	areaPortal = 0;
}

hhPortal::~hhPortal() {
	proximityEntities.Clear(); // Clear the list of potential entities to be portalled
	SAFE_REMOVE(m_portalIdleFx);

#if GAMEPORTAL_PVS
	if (areaPortal && !gameLocal.isClient) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_ALL );
	}
#endif
}

void hhPortal::Spawn(void) {
	idBounds bounds;

	fl.clientEvents = true;

	bNoTeleport = spawnArgs.GetBool( "noTeleport");
	bGlowPortal	= spawnArgs.GetBool( "glowPortal"); // This is a glow portal, and so will have fx, sound, particles, etc
	closeDelay = spawnArgs.GetFloat( "closeDelay");
	monsterportal = spawnArgs.GetBool( "monsterportal" );
	distanceToggle = spawnArgs.GetFloat( "distanceToggle" );
	if (gameLocal.isMultiplayer && bGlowPortal/* && distanceToggle == 0.0f*/) { //mp has a default shutoff distance for glow portals.
		//now ignoring distance toggle keys in mp, and forcing it.
		distanceToggle = MP_PORTAL_RANGE_DEFAULT;
	}
	if (!spawnArgs.GetFloat( "distanceCull", "0.0", distanceCull )) { //rww - distance to shut off the vis gameportal but not the render portal
		//if not value specified, default to shaderParm5+4
		if (!renderEntity.shaderParms[5]) {
			distanceCull = 0.0f;
		}
		else {
			distanceCull = renderEntity.shaderParms[5]+4.0f;
		}
	}
	areaPortalCulling = false;
	if ( !gameLocal.isMultiplayer && bGlowPortal ) {
		alertMonsters = spawnArgs.GetBool( "alertMonsters", "0" );
	} else {
		alertMonsters = false;
	}

	if ( bNoTeleport ) {
		GetPhysics()->SetContents( 0 );
	} else {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	}

	// Setup the initial state for the portal
	if(spawnArgs.GetBool("startActive")) { // Start Active
		portalState = PORTAL_OPENED;

		if ( bGlowPortal ) {
			int anim = GetAnimator()->GetAnim("opened");
			GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
			StartSound( "snd_loop", SND_CHANNEL_ANY );
			PostEventSec( &EV_PortalSpark, gameLocal.random.RandomFloat() );
			SetSkinByName( spawnArgs.GetString( "skin" ) );
		}
	} else { // Start closed
		Hide();
		portalState = PORTAL_CLOSED;
		GetPhysics()->SetContents( 0 ); //rww - if closed, ensure things will not hit and go through
	}

#if GAMEPORTAL_PVS
	const char *gamePortalName = spawnArgs.GetString("gamePortalName", GetName());
	areaPortal = gameRenderWorld->FindGamePortal( gamePortalName );
	if (areaPortal && !gameLocal.isClient) {
		gameLocal.SetPortalState( areaPortal, portalState == PORTAL_CLOSED ? PS_BLOCK_ALL : PS_BLOCK_NONE );
	}
#endif

	// Default to normal gravity.  This could be changed by any zones the portal is within
	SetGravity( gameLocal.GetGravity() );

	BecomeActive( TH_THINK | TH_UPDATEVISUALS | TH_ANIMATE );

	UpdateVisuals();

	PostEventMS( &EV_PostSpawn, 0 );

	fl.networkSync = true; //rww

	if (gameLocal.isMultiplayer) { //rww - if portals grab their renderEntity bounds from the animation, they can cause client-server pvs discrepancies
		renderEntity.bounds[0] = idVec3(-MAX_PORTAL_BOUNDS, -MAX_PORTAL_BOUNDS, -MAX_PORTAL_BOUNDS);
		renderEntity.bounds[1] = idVec3(MAX_PORTAL_BOUNDS, MAX_PORTAL_BOUNDS, MAX_PORTAL_BOUNDS);
	}

	proximityEntities.Clear(); // Clear the list of potential entities to be portalled
}


void hhPortal::PostSpawn( void ) {
	CheckForBuddy();
}

void hhPortal::Save(idSaveGame *savefile) const {
#if GAMEPORTAL_PVS
	savefile->WriteInt( areaPortal );
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}
#endif
	savefile->WriteInt( portalState );
	savefile->WriteBool( bNoTeleport );
	savefile->WriteBool( bGlowPortal );
	savefile->WriteVec3( portalGravity );
	savefile->WriteFloat( closeDelay );
	savefile->WriteBool( monsterportal );
	savefile->WriteBool( alertMonsters );

	savefile->WriteInt( slavePortals.Num() );		// idList<idEntityPtr<hhPortal> >
	for (int i=0; i<slavePortals.Num(); i++) {
		slavePortals[i].Save(savefile);
	}
	
	masterPortal.Save(savefile);

	savefile->WriteFloat(distanceToggle);
	savefile->WriteFloat(distanceCull);
	savefile->WriteBool(areaPortalCulling);

	savefile->WriteInt( proximityEntities.Num() );
	for( int i = 0; i < proximityEntities.Num(); i++ ) {
		proximityEntities[i].entity.Save( savefile );
		savefile->WriteVec3( proximityEntities[i].lastPortalPoint );
	}

	m_portalIdleFx.Save(savefile);
}

void hhPortal::Restore( idRestoreGame *savefile ) {
	int i, num;

#if GAMEPORTAL_PVS
	savefile->ReadInt( areaPortal );
	if ( areaPortal ) {
		int portalState;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}
#endif
	savefile->ReadInt( (int &)portalState );
	savefile->ReadBool( bNoTeleport );
	savefile->ReadBool( bGlowPortal );
	savefile->ReadVec3( portalGravity );
	savefile->ReadFloat( closeDelay );
	savefile->ReadBool( monsterportal );
	savefile->ReadBool( alertMonsters );

	slavePortals.Clear();
	savefile->ReadInt( num );						// idList<idEntityPtr<hhPortal> >
	slavePortals.SetNum( num );
	for (i=0; i<num; i++) {
		slavePortals[i].Restore(savefile);
	}

	masterPortal.Restore(savefile);

	savefile->ReadFloat(distanceToggle);
	savefile->ReadFloat(distanceCull);
	savefile->ReadBool(areaPortalCulling);

	savefile->ReadInt( num );
	proximityEntities.SetNum( num );
	for( i = 0; i < num; i++ ) {
		proximityEntities[i].entity.Restore( savefile );
		savefile->ReadVec3( proximityEntities[i].lastPortalPoint );
	}

	m_portalIdleFx.Restore(savefile);
}

void hhPortal::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat(portalGravity.x);
	msg.WriteFloat(portalGravity.y);
	msg.WriteFloat(portalGravity.z);
	msg.WriteBits(masterPortal.GetSpawnId(), 32);

	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_MODE]);
	msg.WriteFloat(renderEntity.shaderParms[SHADERPARM_TIMEOFFSET]);

	msg.WriteBits(portalState, 4);
}

void hhPortal::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	portalGravity.x = msg.ReadFloat();
	portalGravity.y = msg.ReadFloat();
	portalGravity.z = msg.ReadFloat();
	masterPortal.SetSpawnId(msg.ReadBits(32));

	renderEntity.shaderParms[SHADERPARM_MODE] = msg.ReadFloat();
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadFloat();

	portalStates_t newPortalState = (portalStates_t)msg.ReadBits(4);
	if ((newPortalState == PORTAL_CLOSED || newPortalState == PORTAL_OPENED) &&
		newPortalState != portalState && portalState != PORTAL_CLOSING && portalState != PORTAL_OPENING) {
		Trigger(this);
	}
}

void hhPortal::ClientPredictionThink( void ) {
	Think();
}

void hhPortal::CheckPlayerDistances(void) {
	float closest = distanceToggle+distanceCull;
	hhPortal *targetPortal = NULL;

	if (cameraTarget && cameraTarget->IsType(hhPortal::Type)) { //we want to measure distance from the target portal too if we have one
		targetPortal = static_cast<hhPortal *>(cameraTarget);
	}

	for (int i = 0; i < gameLocal.numClients; i++) { //loop through any active clients and get the closest distance to one.
		idEntity *ent = gameLocal.entities[i];
		if (ent && ent->IsType(hhPlayer::Type)) {
			hhPlayer *pl = static_cast<hhPlayer *>(ent);

			if (pl->health > 0 && !pl->spectating && !pl->InVehicle()) { //don't open for spectators or dead players or players in vehicles
				float d = (pl->GetOrigin()-GetOrigin()).Length();
				if (d < closest) {
					closest = d;
				}
				if (targetPortal) { //check distance from the target portal
					d = (pl->GetOrigin()-targetPortal->GetOrigin()).Length();
					if (d < closest) {
						closest = d;
					}
				}
			}
		}
	}

	if (distanceToggle != 0.0f) { //toggling portal based on distance of player
		if (closest < distanceToggle) { //should be open
			if (portalState == PORTAL_CLOSED) {
				Trigger(this);
			}
		}
		else { //should be closed
			if (portalState == PORTAL_OPENED) {
				Trigger(this);
			}
		}
	}
	
	if (distanceCull != 0.0f) { //toggling only area portal based on distance of player
		if (closest >= distanceCull) { //should be closed
			if (!areaPortalCulling) {
				areaPortalCulling = true;
				if (!gameLocal.isClient) {
					gameLocal.SetPortalState( areaPortal, PS_BLOCK_ALL );
				}
			}
		}
		else if (areaPortalCulling && (portalState == PORTAL_OPENING || portalState == PORTAL_OPENED)) { //otherwise make sure it's on (if the portal is open)
			if (!gameLocal.isClient) {
				gameLocal.SetPortalState( areaPortal, PS_BLOCK_NONE );
			}
			areaPortalCulling = false;
		}
	}
}


//==========================================================================
//
// hhPortal::Think
//	
//==========================================================================

#define NEAR_CLIP	0 //6.5

void hhPortal::Think( void ) {
	int				i;
	idEntity		*hit;
	idPlane			plane;

	if ((distanceCull != 0.0f || distanceToggle != 0.0f) && areaPortal) { //check to turn the areaportal on and off based on distance.
		if (!gameLocal.isClient) {
			CheckPlayerDistances();
		}
		else { //since this is not sync'd, and we don't need to sync it, do an extra check here (for mp)
			if (portalState == PORTAL_OPENED && renderEntity.customSkin && renderEntity.customSkin == declManager->FindSkin(spawnArgs.GetString( "skin_onlyWarp" ))) {
				SetSkinByName( spawnArgs.GetString( "skin" ) );
			}
		}

		if (distanceToggle != 0.0f) { //for pop-open portals, check fx state
			if ((portalState == PORTAL_CLOSED || portalState == PORTAL_CLOSING) && bGlowPortal) {
				//rww - broadcast fx while closed
				if (!m_portalIdleFx.IsValid()) {
					const char *portalIdleFx = spawnArgs.GetString("fx_idleclosed", "fx/portal_closed_idle");
					if (portalIdleFx[0]) {
						hhFxInfo fxInfo;
						fxInfo.SetNormal( GetAxis()[2] );
						fxInfo.RemoveWhenDone(false);

						m_portalIdleFx = SpawnFxLocal(portalIdleFx, GetOrigin(), GetAxis(), &fxInfo, true);
						if (!m_portalIdleFx.IsValid()) { //spawn failure?
							gameLocal.Warning("hhPortal::Think: portal could not spawn fx for fx_idleclosed (%s).", portalIdleFx);
						}
					}
				}

				if (m_portalIdleFx.IsValid() && !m_portalIdleFx->IsActive(TH_THINK)) {
					m_portalIdleFx->Nozzle(true);
				}
			}
			else if (m_portalIdleFx.IsValid() && m_portalIdleFx->IsActive(TH_THINK)) { //if it's not closed/closing and has fx, then stop them
				m_portalIdleFx->Nozzle(false);
			}
		}
	}

	hhAnimatedEntity::Think();

	if (gameLocal.isMultiplayer) { //rww - if portals grab their renderEntity bounds from the animation, they can cause client-server pvs discrepancies
		renderEntity.bounds[0] = idVec3(-MAX_PORTAL_BOUNDS, -MAX_PORTAL_BOUNDS, -MAX_PORTAL_BOUNDS);
		renderEntity.bounds[1] = idVec3(MAX_PORTAL_BOUNDS, MAX_PORTAL_BOUNDS, MAX_PORTAL_BOUNDS);
	}

	if( portalState == PORTAL_CLOSED || bNoTeleport ) {
		return;
	}

	// Force visuals to update until a remote renderview has been created for this portal
	if ( !renderEntity.remoteRenderView ) {	
		BecomeActive( TH_UPDATEVISUALS );
	}

	// Bit of a hack for noclipping players:  If they are close to the portal, then add them to the proximity list automatically
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player && player->noclip ) { //rww - note that the local player is NULL for dedicated servers.
		if ( (player->GetOrigin() - GetOrigin()).LengthFast() < 256.0f ) {
			AddProximityEntity( player );
		}
	}

	// Build a plane for the portal surface
	plane.SetNormal(GetPhysics()->GetAxis()[0]);
	plane.FitThroughPoint( GetPhysics()->GetOrigin() + plane.Normal() * NEAR_CLIP );

	idVec3 origin = GetOrigin();
	idMat3 axis = GetAxis();

	for ( i = 0; i < proximityEntities.Num(); i++ ) {
		if ( !proximityEntities[i].entity.IsValid() ) {
			// Remove this entity from the list
			proximityEntities.RemoveIndex( i );
			continue;
		}

		hit = proximityEntities[i].entity.GetEntity();
		idVec3 location = proximityEntities[i].lastPortalPoint;
		idVec3 nextLocation = hit->GetPortalPoint();
		proximityEntities[i].lastPortalPoint = nextLocation;
		if ( !AttemptPortal( plane, hit, location, nextLocation ) ) {
			proximityEntities.RemoveIndex( i );

			// If the entity is a player, then inform the player that they are no longer close to a portal
			if ( hit->IsType( hhPlayer::Type ) ) {
				hhPlayer *player = static_cast<hhPlayer *>(hit);
				player->SetPortalColliding( false );
			}
		}	
	}
}

bool hhPortal::AttemptPortal( idPlane &plane, idEntity *hit, idVec3 location, idVec3 nextLocation ) {

	// Don't try to portal self
	if( hit == this ) {
		return false;
	}		

	if ( hit->IsBound() ) {	// Do not portal bound objects -- let the master portalling handle bound entities
		return false;
	}

	// Don't allow idMovers to portal.  TODO:  Restrict other entities?
	if ( hit->IsType( idMover::Type ) || hit->IsType(hhVehicle::Type) ) { //rww - do not allow shuttles either
		return false;
	}

	if (hit->IsType(hhPlayer::Type)) { //rww - don't portal dead players
		hhPlayer *pl = static_cast<hhPlayer *>(hit);
		if (pl->health <= 0) {
			return false;
		}

		// Check if the player is intersecting this portal.  If not, then don't try to portal it
		if ( !GetPhysics()->GetAbsBounds().IntersectsBounds( pl->GetPhysics()->GetAbsBounds() ) ) {
			return false;
		}
	}

	int side = plane.Side( location );
	if ( side == PLANESIDE_ON || side == PLANESIDE_CROSS ) {
		side = PLANESIDE_BACK;
	}

	int nextSide = plane.Side( nextLocation );
	if ( nextSide == PLANESIDE_ON || nextSide == PLANESIDE_CROSS ) {
		nextSide = PLANESIDE_BACK;
	}

	if( side == PLANESIDE_BACK ) { // On the backside, remove this entity from the list
		return false;
	} else if ( side == nextSide ) { // Entirely on one side
		// Check if the entity is too far from the plane and remove it from the list
		if ( !GetPhysics()->GetAbsBounds().IntersectsBounds( hit->GetPhysics()->GetAbsBounds() ) ) {
			return false;
		}

		return true;
	}		

	// Compute the location on the plane where the entity would hit
	float scale;
	idVec3 dir = nextLocation - location;
	plane.RayIntersection( location, dir, scale );

	// Portal the entity
	PortalEntity( hit, location + dir * scale * 1.01f );

	// Add this entity to the destination portal's proximityEntity list
	if (cameraTarget && cameraTarget->IsType(hhPortal::Type)) {
		hhPortal *targetPortal = static_cast<hhPortal *>(cameraTarget);
		targetPortal->CollideWithPortal( hit ); // CJR PCF 04/26/06:  Previously CheckPortal
	}

	return false;
}

void hhPortal::PortalProjectile( hhProjectile *projectile, idVec3 collideLocation, idVec3 nextLocation ) {
	idPlane plane;
	plane.SetNormal(GetPhysics()->GetAxis()[0]);
	plane.FitThroughPoint( GetPhysics()->GetOrigin() );

	AttemptPortal( plane, projectile, collideLocation, nextLocation );
}

//==========================================================================
//==========================================================================
void hhPortal::CheckForBuddy() {
	const char *	buddyName;
	idEntity * 		foundEntity;
	hhPortal*		buddy;
	
	// Check for a buddy portal name
	buddyName = spawnArgs.GetString( "partner", NULL );
	if ( buddyName ) {
		gameLocal.Warning( "Portal %s has 'partner' key.  Please change this to 'buddy'", GetName() );
	}
	else {
		buddyName = spawnArgs.GetString( "buddy" );	
	}
	if ( !buddyName || !buddyName[ 0 ] ) {
		return;
	}
	
	// Get the actual entity
	foundEntity = gameLocal.FindEntity( buddyName );	
	if ( !foundEntity ) {
		return;
	}

	if ( foundEntity->IsType( hhPortal::Type ) ) {

		buddy = (hhPortal *) foundEntity;

		if (buddy->spawnArgs.FindKey("buddy")) {
			gameLocal.Error( "Portals %s and %s both have 'buddy' key set.  Use only on master", GetName(), buddy->GetName() );
			return;
		}

		// Don't link up to buddy if a monster portal, just points to portal room
		if ( !monsterportal || closeDelay == 0.0f ) {
			//? OK, let's be lazy, assume only 2 portals are buddied.  Make us the master
			buddy->SetMasterPortal( this );
			//! Set the cameraTargets of each, then update each
			buddy->spawnArgs.Set( "cameraTarget", name.c_str() );
			buddy->ProcessEvent( &EV_UpdateCameraTarget );
		}

		AddSlavePortal( buddy );
	}

	spawnArgs.Set( "cameraTarget", buddyName );
	ProcessEvent( &EV_UpdateCameraTarget );
}


//=============================
// hhPortal::TriggerTargets
//=============================
void hhPortal::TriggerTargets() {
	const char *	key = "";
	idEntity *		entity = NULL;
	idList< idStr > entityNames;
	

	// Find the right key based on our state
	if ( portalState == PORTAL_OPENING ) {
		key = "targetOpening";
	}
	else if ( portalState == PORTAL_OPENED ) {
		key = "targetOpened";
	}
	else if ( portalState == PORTAL_CLOSING ) {
		key = "targetClosing";
	}
	else if ( portalState == PORTAL_CLOSED ) {
		key = "targetClosed";
	}

	//gameLocal.Printf( "Keying %s\n", key );
	
	// Loop through the entities, spawning each one
	hhUtils::GetValues( spawnArgs, key, entityNames, true );
	
	for ( int i = 0; i < entityNames.Num(); ++i ) {
		//gameLocal.Printf( "Gonna trigger %s\n", entityNames[ i ].c_str() );
		entity = gameLocal.FindEntity( entityNames[ i ] );
		if ( entity ) {
			entity->PostEventMS( &EV_Activate, 0, this );
		}
	} 
}


//==========================================================================
//
// hhPortal::CheckPortal
//
// If this entity can be portaled, typically called from the low-level physics clip functions
//
// CJR PCF 04/26/06:  Changed this function to separate CheckPortal and CollideWithPortal
//==========================================================================

bool hhPortal::CheckPortal( const idEntity *other, int contentMask ) {
	if ( contentMask & CONTENTS_GAME_PORTAL ) { // Check if the other entity should clip against the portal
		return false;
	}

	if ( !other ) {
		return true;
	}

	if ( other->fl.noPortal ) {
		return false; // Do not allow this entity to portal, make it collide with the portal instead
	}

	return true;
}

bool hhPortal::CheckPortal( const idClipModel *mdl, int contentMask ) {
	if ( contentMask & CONTENTS_GAME_PORTAL ) { // Check if the other entity should clip against the portal
		return false;
	}

	if ( !mdl ) {
		return true;
	}

	return CheckPortal( mdl->GetEntity(), contentMask );
}

//==========================================================================
//
// hhPortal::CollideWithPortal
//
// Called from low-level clip functions, actually collide the entity with the portal
//
// CJR PCF 04/26/06:  Formerly part of CheckPortal
//==========================================================================

void hhPortal::CollideWithPortal( const idEntity *other ) {
	if ( !other ) {
		return;
	}

	if ( other->IsType( hhProjectile::Type ) ) { // Projectiles move so fast that they should get portaled next time they think
		// Only add this projectile to the collided list if it hits the front side of the portal
		idPlane plane;
		plane.SetNormal(GetPhysics()->GetAxis()[0]);
		plane.FitThroughPoint( GetPhysics()->GetOrigin() );

		int side = plane.Side( other->GetOrigin() );
		if ( side == PLANESIDE_FRONT ) { // It's on the front, so add this portal to the list
			hhProjectile *projectile = (hhProjectile *)(other);

			// Check if the projectile will actually impact the portal
			idVec3 end = projectile->GetOrigin() + projectile->GetPhysics()->GetLinearVelocity();	

			if ( plane.LineIntersection( projectile->GetOrigin(), end ) ) { // Projectile collides with the portal
				projectile->SetCollidedPortal( this, projectile->GetPortalPoint(), projectile->GetPhysics()->GetLinearVelocity() );
			}
		}
	} else {
		AddProximityEntity( other );
	}	
}

void hhPortal::CollideWithPortal( const idClipModel *mdl ) {
	if ( !mdl ) {
		return;
	}

	CollideWithPortal( mdl->GetEntity() );
}

//==========================================================================
//
// hhPortal::AddProximityEntity
//
// Saves this entity on a list, and will check if it can be portalled
// the next time the portal thinks
//==========================================================================

void hhPortal::AddProximityEntity( const idEntity *other) {
	// Go through the list and guarantee that this entity isn't in multiple times
	// note:  cannot use IdList::AddUnique, because the lastPortalPoint might be different during this add
	for( int i = 0; i < proximityEntities.Num(); i++ ) {
		if ( proximityEntities[i].entity.GetEntity() == other ) {
			return;
		}
	}

	// Add this entity to the potential portal list
	proximityEntity_t prox;
	prox.entity = other;
	prox.lastPortalPoint = ((idEntity *)(other))->GetPortalPoint();

	proximityEntities.Append( prox );

	// If the entity is a player, then inform the player that they are close to this portal
	// needed for weapon projectile firing
	if ( other->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = (hhPlayer *)(other);

		player->SetPortalColliding( true );
	}
}

//==========================================================================
//
// hhPortal::PortalEntity
//
// To rotate a vector from one portal space into another:
//	    transform vector into Source Portal Space (mul by axis transpose)
//		flip X & Y (only flip X for angles, not for position)
//		transform vector into Destination Portal Space
//==========================================================================

void PortalRotate( idVec3 &vec, const idMat3 &sourceTranspose, const idMat3 &dest, const bool flipX ) {
	vec *= sourceTranspose;
	vec.y *= -1;
	if ( flipX ) {
		vec.x *= -1;
	}
	vec *= dest;
}

bool hhPortal::PortalEntity( idEntity *ent, const idVec3 &point ) {
	idMat3 sourceAxis;
	idMat3 destAxis;
	idMat3 newEntAxis;

	if ( !ent ) {
		return(false);
	}

	if ( cameraTarget ) {
		sourceAxis = GetAxis().Transpose();
		destAxis = cameraTarget->GetAxis();

		// Compute new location
		idVec3 newLocation = point - GetOrigin();
		PortalRotate( newLocation, sourceAxis, destAxis, false );
		newLocation += cameraTarget->GetOrigin();

		// Compute new axis
		newEntAxis = ent->GetAxis();

		// Rotate the vector into new portal space
		PortalRotate( newEntAxis[0], sourceAxis, destAxis, true );
		PortalRotate( newEntAxis[1], sourceAxis, destAxis, true );
		PortalRotate( newEntAxis[2], sourceAxis, destAxis, true );
		
		// Actually attempt to portal the entity
		if ( PortalTeleport( ent, newLocation, newEntAxis, sourceAxis, destAxis ) ) {
			if ( alertMonsters && !gameLocal.isMultiplayer && ent->IsType( idPlayer::Type ) ) {
				gameLocal.SendMessageAI( this, GetOrigin(), 2000, MA_EnemyPortal );		
			}
			ent->Portalled( this ); // Inform the actor that it was just portalled
			return(true); // Portal succeeded
		}
	}

	return(false); // Portal failed
}

//==========================================================================
//
// hhPortal::PortalTeleport
//
//==========================================================================

bool hhPortal::PortalTeleport( idEntity *ent, const idVec3 &origin, const idMat3 &axis, const idMat3 &sourceAxis, const idMat3 &destAxis ) {
	idClipModel *clip = ent->GetPhysics()->GetClipModel();

	if ( !clip ) {
		return false;
	}

	// Properly set velocity relative to the new portal
	idVec3 vel = ent->GetPhysics()->GetLinearVelocity();
	PortalRotate( vel, sourceAxis, destAxis, true );
	ent->GetPhysics()->SetLinearVelocity( vel );	

	//rww - check if this new orientation is going to be in solid or not. if it is, try displacing the new origin based on the portal
	idVec3 useOrigin = origin;
	trace_t transCheck;
	if (gameLocal.clip.TranslationWithExceptions(transCheck, useOrigin, useOrigin, NULL, clip, axis, ent->GetPhysics()->GetClipMask(), ent)) {
		if (cameraTarget) {
			bool safeSpot = true;
			const float distExtrusion = 2.0f;
			const float heightAdjust = (ent->GetPhysics()->GetBounds()[1].z-fabsf(ent->GetPhysics()->GetBounds()[0].z))/2.0f;
			idVec3 testOrigin = cameraTarget->GetOrigin();
			testOrigin += cameraTarget->GetAxis()[0]*distExtrusion;
			testOrigin -= cameraTarget->GetAxis()[2]*heightAdjust;
			if (gameLocal.clip.TranslationWithExceptions(transCheck, testOrigin, testOrigin, NULL, clip, axis, ent->GetPhysics()->GetClipMask(), ent)) {
				testOrigin += cameraTarget->GetAxis()[2]*(heightAdjust*2.0f); //then try going up further (could be upside-down or something)

				if (gameLocal.clip.TranslationWithExceptions(transCheck, testOrigin, testOrigin, NULL, clip, axis, ent->GetPhysics()->GetClipMask(), ent)) {
					//damn, this portal is really busted.
					safeSpot = false;
				}
			}
			if (safeSpot) {
				useOrigin = testOrigin; //got a safe spot
				if (ent->IsType(hhPlayer::Type)) {
					hhPlayer *plEnt = static_cast<hhPlayer *>(ent);
					if (plEnt->IsWallWalking()) { //if it's a wallwalking player, try to trace down on the other side for wallwalk
						idVec3 downPoint = useOrigin - (axis[2]*64.0f);
						if (gameLocal.clip.Translation(transCheck, useOrigin, downPoint, clip, axis, ent->GetPhysics()->GetClipMask(), ent)) {
							if (gameLocal.GetMatterType(transCheck, NULL) == SURFTYPE_WALLWALK) { //if we hit wallwalk, use the trace endpoint
								useOrigin = transCheck.endpos;
							}
						}
					}
				}
			}
			else {
				testOrigin = cameraTarget->GetOrigin();
				testOrigin += cameraTarget->GetAxis()[0]*distExtrusion;
				testOrigin -= cameraTarget->GetAxis()[2]*heightAdjust;

				useOrigin = testOrigin;

#if !GOLD
				if (developer.GetBool()) {
					const char *entName = ent->GetName();
					if (!entName || !entName[0]) {
						entName = "<unknown>";
					}
					gameLocal.Warning("Ent '%s' could not get a clean trace on the other side of gameportal at (%f %f %f).", entName, useOrigin.x, useOrigin.y, useOrigin.z);
					hhUtils::DebugAxis( testOrigin, cameraTarget->GetAxis(), 32.0f, 5000 );
				}
#endif
			}
		}
	}

	// If the actor is a player, then disable camera interpolation for this move.  This must be done before linking
	if( ent->IsType(hhPlayer::Type) ) {
		hhPlayer* player = static_cast<hhPlayer*>( ent );


		idVec3 viewDir = player->GetAxis()[0];
		PortalRotate( viewDir, sourceAxis, destAxis, true );
		player->TeleportNoKillBox( useOrigin, axis, viewDir, player->GetUntransformedViewAngles() );
	} else {
		// Valid move.  This code is done in SetOrientation for the player.
		ent->SetOrigin( useOrigin );
		ent->SetAxis( axis );
	}

	// Re-link the actor into the clip tree
	clip->Link( gameLocal.clip );

	if ( ent->IsType( idActor::Type ) ) {
		static_cast<idActor *>(ent)->LinkCombat();
	}

	// players telefrag anything at the new position
	if ( ent->IsType( hhPlayer::Type ) ) {
		gameLocal.KillBox( ent );
	}

	// Set the gravity correctly on the entity that was portaled, based upon the destination portal's gravity
	ent->CancelEvents( &EV_ResetGravity );

	if (!ent->IsType(hhVehicle::Type)) {
		ent->SetGravity( cameraTarget->GetGravity() );
	}

	// If the actor is a player, then check for wallwalk.  This should be done after linking and setting gravity
	if( ent->IsType(hhPlayer::Type) ) {
		hhPlayer* player = static_cast<hhPlayer*>(ent);
		if (player->GetPhysics() && player->GetPhysics()->IsType(hhPhysics_Player::Type)) {
			static_cast<hhPhysics_Player *>(player->GetPhysics())->CheckWallWalk( true );
		}
	}

	// Sound issues
	if ( bGlowPortal ) {
		StartSound( "snd_portal_entity", SND_CHANNEL_ANY ); // Play the portal sound at the origin portal
		if(this->cameraTarget) { // Play the portal sound at the destination portal as well
			const idSoundShader *def = declManager->FindSound(spawnArgs.GetString("snd_portal_entity"));//gameSoundWorld->FinishShader(spawnArgs.GetString("snd_portal_entity"));
			this->cameraTarget->StartSoundShader( def, SND_CHANNEL_ANY );
		}
	}

	return(true);
}

//==========================================================================
//
// hhPortal::Event_Trigger
//
// Toggle portal state.
//==========================================================================

void hhPortal::Event_Trigger(idEntity *activator) {

	// If we have a master portal, have him trigger everyone
	if ( masterPortal.IsValid() ) {
		masterPortal->Event_Trigger( activator );
		return;
	}
	
	// If we have slave portals, trigger them first
	if ( slavePortals.Num() > 0 ) {
		for ( int i = 0; i < slavePortals.Num(); ++i ) {
			slavePortals[ i ]->Trigger( activator );
		}
	}

	Trigger( activator );
}


void hhPortal::Trigger( idEntity *activator ) {

	hhFxInfo fxInfo;

	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( true );

	if(portalState == PORTAL_CLOSED) {
		portalState = PORTAL_OPENING;
		if (!bNoTeleport) { //rww - if teleporting, ensure things collide with me while i am opening
			GetPhysics()->SetContents( CONTENTS_SOLID );
		}
#if GAMEPORTAL_PVS
		if (areaPortal && !gameLocal.isClient) {
			gameLocal.SetPortalState( areaPortal, PS_BLOCK_NONE );
		}
#endif
		TriggerTargets();		// nla	
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = MS2SEC( gameLocal.time );

		if ( bGlowPortal ) {
			if (!gameLocal.isMultiplayer) { //rww - superfast portals don't use fx
				BroadcastFxInfo( spawnArgs.GetString("fx_open"), GetOrigin(), GetAxis(), &fxInfo );
			}

			StartSound( "snd_open", SND_CHANNEL_ANY );

			if ( spawnArgs.GetBool( "fast_open", "0" ) ) {
				SetSkinByName( spawnArgs.GetString( "skin" ) );
			} else {
				SetSkinByName( spawnArgs.GetString( "skin_onlyWarp" ) ); // Only show the warp when first opening
			}

			int anim = GetAnimator()->GetAnim(spawnArgs.GetString("open_anim", "open"));
			GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
			PostEventMS( &EV_Opened, GetAnimator()->AnimLength( anim ) );
			SetShaderParm( SHADERPARM_MODE, 0 ); // ensure that sparking is off

			fl.neverDormant = true; // Don't allow the portal to go dormant while opening/closing

		} else {
			PostEventMS( &EV_Opened, 500 );		
		}

		PostEventMS( &EV_Show, 10 ); // Delay showing for a frame so the skin and registers are properly set beforehand
	} else if(portalState == PORTAL_OPENED || portalState == PORTAL_OPENING) {
		portalState = PORTAL_CLOSING;
		TriggerTargets();		// nla

		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );

		if ( bGlowPortal ) {
			if (!gameLocal.isMultiplayer) { //rww - superfast portals don't use fx
				BroadcastFxInfo( spawnArgs.GetString("fx_close"), GetOrigin(), GetAxis(), &fxInfo );
			}
	
			StopSound( SND_CHANNEL_ANY );
			StartSound( "snd_close", SND_CHANNEL_ANY );

			int anim = GetAnimator()->GetAnim("close");
			GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0.5 );
			PostEventMS( &EV_Closed, GetAnimator()->AnimLength( anim ) );

			fl.neverDormant = true; // Don't allow the portal to go dormant while opening/closing

		} else {
			PostEventMS( &EV_Closed, 400 );
		}
	}
}

//==========================================================================
//
// hhPortal::Event_Opened
//
//==========================================================================

void hhPortal::Event_Opened( void ) {
	portalState = PORTAL_OPENED;
	if (!bNoTeleport) { //rww - if teleporting, ensure things collide with me while i am open
		GetPhysics()->SetContents( CONTENTS_SOLID );
	}
	TriggerTargets();	// nla

	if ( bGlowPortal ) {
		StartSound( "snd_loop", SND_CHANNEL_ANY );
		int anim = GetAnimator()->GetAnim("opened");
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );

		fl.neverDormant = false; // The portal can go dormant once opened/closed

		PostEventSec( &EV_PortalSpark, gameLocal.random.RandomFloat() );
	}	

	UpdateVisuals();
	
	// If we are open, and should close automatically, post an event to trigger us closed - nla
	if ( closeDelay > 0.0f ) {
		PostEventSec( &EV_Activate, closeDelay, NULL );

		// Remove the portal now that it has done it's job
		if (monsterportal) {
#ifdef _DEBUG
			// If another portal is camera targetting me it will cause a crash so check for it
			for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
				if (ent->cameraTarget && ent->cameraTarget == this) {
					assert(0);
				}
			}

#endif
			PostEventSec( &EV_Remove, closeDelay+5.0f );
		}
	}
}

//==========================================================================
//
// hhPortal::Event_Closed
//
//==========================================================================

void hhPortal::Event_Closed( void ) {
	renderEntity.shaderParms[SHADERPARM_MODE] = 1.0f;
	portalState = PORTAL_CLOSED;
	GetPhysics()->SetContents( 0 ); //rww - do not collide with things while closed
#if GAMEPORTAL_PVS
	if (areaPortal && !gameLocal.isClient) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_ALL );
	}
#endif
	TriggerTargets();	// nla

	fl.neverDormant = false; // The portal can go dormant once opened/closed
	
	Hide();
	UpdateVisuals();
	
	//? Have an option to remove the portals? - nla
	if ( spawnArgs.GetBool( "remove_on_close", "0" ) ) {
		PostEventMS( &EV_Remove, 0 );		
	}
}

//==========================================================================
//
// hhPortal::Event_PortalSpark
//
//==========================================================================

void hhPortal::Event_PortalSpark( void ) {
	int		spark;
	float	nextTime;

	if ( this->IsHidden() ) { // No sparking if hidden
		SetShaderParm( SHADERPARM_MODE, 0 ); // set the material parm (one-based)
		return;
	}

	spark = gameLocal.random.RandomInt( spawnArgs.GetInt( "sparkCount" ) );

	// Set the shader parm as needed
	SetShaderParm( SHADERPARM_MODE, spark + 1 ); // set the material parm (one-based)

	if ( gameLocal.random.RandomFloat() < 0.1f ) { // 1/10th of a chance that another spark will happen quickly after this one
		nextTime = 0.2f;
	} else { // Normal random time between sparks.
		nextTime = spawnArgs.GetFloat( "sparkTimeMin" ) + gameLocal.random.RandomFloat() * spawnArgs.GetFloat( "sparkTimeRnd" );
	}

	StartSound( "snd_portal_spark", SND_CHANNEL_ANY );
	
	PostEventSec( &EV_PortalSpark, nextTime );
	PostEventSec( &EV_PortalSparkEnd, 0.1f ); // spark only lasts for 0.1 sec
	
}

//==========================================================================
//
// hhPortal::Event_PortalSparkEnd
//
//==========================================================================

void hhPortal::Event_PortalSparkEnd( void ) {
	SetShaderParm( SHADERPARM_MODE, 0 ); // Disable the spark parm
}

//==========================================================================
//
// hhPortal::Event_ShowGlowPortal
//
// Simply sets the skin on the portal back to the default or to the
// specified "skin"
//==========================================================================

void hhPortal::Event_ShowGlowPortal( void ) {
	SetSkinByName( spawnArgs.GetString( "skin" ) );
}

//==========================================================================
//
// hhPortal::Event_HideGlowPortal
//
// Sets the skin on the portal to a specific skin that hides the glowy parts
//==========================================================================

void hhPortal::Event_HideGlowPortal( void ) {
	SetSkinByName( spawnArgs.GetString( "skin_onlyWarp" ) );
}
