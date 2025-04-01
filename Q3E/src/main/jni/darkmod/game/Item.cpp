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

/**
* TDM: There is absolutely no reason to use this class any more!!!
* All inventory and frob functionality has been moved to idEntity
**/

#include "precompiled.h"
#pragma hdrstop



#pragma warning(disable : 4996)

#include "Game_local.h"
#include "DarkModGlobals.h"
#include "StimResponse/StimResponseCollection.h"
#include "AbsenceMarker.h"

/*
===============================================================================

  idItem

===============================================================================
*/

const idEventDef EV_DropToFloor( "<dropToFloor>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_RespawnItem( "respawn", EventArgs(), EV_RETURNS_VOID, "Respawn" );
const idEventDef EV_RespawnFx( "<respawnFx>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_GetPlayerPos( "<getplayerpos>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_HideObjective( "<hideobjective>", EventArgs('e', "", ""), EV_RETURNS_VOID, "internal" );
const idEventDef EV_CamShot( "<camshot>", EventArgs(), EV_RETURNS_VOID, "internal" );

CLASS_DECLARATION( idEntity, idItem )
	EVENT( EV_DropToFloor,		idItem::Event_DropToFloor )
	EVENT( EV_Touch,			idItem::Event_Touch )
	EVENT( EV_Activate,			idItem::Event_Trigger )
	EVENT( EV_RespawnItem,		idItem::Event_Respawn )
	EVENT( EV_RespawnFx,		idItem::Event_RespawnFx )
END_CLASS


/*
================
idItem::idItem
================
*/
idItem::idItem()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);

	spin = false;
	inView = false;
	inViewTime = 0;
	lastCycle = 0;
	lastRenderViewTime = -1;
	itemShellHandle = -1;
	shellMaterial = NULL;
	orgOrigin.Zero();
	canPickUp = true;

	m_FrobActionScript = "frob_item";

	noticeabilityIfAbsent = 0.0;
	b_orgOriginSet = false;
	

}

/*
================
idItem::~idItem
================
*/
idItem::~idItem() {
	// remove the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
	}
}

/*
================
idItem::Save
================
*/
void idItem::Save( idSaveGame *savefile ) const {

	savefile->WriteVec3( orgOrigin );
	savefile->WriteBool( spin );
	savefile->WriteBool( pulse );
	savefile->WriteBool( canPickUp );

	savefile->WriteMaterial( shellMaterial );

	savefile->WriteBool( inView );
	savefile->WriteInt( inViewTime );
	savefile->WriteInt( lastCycle );
	savefile->WriteInt( lastRenderViewTime );
	savefile->WriteFloat (noticeabilityIfAbsent);
	absenceEntityPtr.Save (savefile);
}

/*
================
idItem::Restore
================
*/
void idItem::Restore( idRestoreGame *savefile ) {

	savefile->ReadVec3( orgOrigin );
	savefile->ReadBool( spin );
	savefile->ReadBool( pulse );
	savefile->ReadBool( canPickUp );

	savefile->ReadMaterial( shellMaterial );

	savefile->ReadBool( inView );
	savefile->ReadInt( inViewTime );
	savefile->ReadInt( lastCycle );
	savefile->ReadInt( lastRenderViewTime );
	savefile->ReadFloat (noticeabilityIfAbsent);
	absenceEntityPtr.Restore (savefile);

	itemShellHandle = -1;
}

/*
================
idItem::UpdateRenderEntity
================
*/
bool idItem::UpdateRenderEntity(renderEntity_s *renderEntity, const renderView_t *renderView)
{
	bool bRc = true;

	if(pulse)
	{
		if(lastRenderViewTime == renderView->time)
			return false;

		lastRenderViewTime = renderView->time;

		// check for glow highlighting if near the center of the view
		idVec3 dir = renderEntity->origin - renderView->vieworg;
		dir.Normalize();
		float d = dir * renderView->viewaxis[0];

		// two second pulse cycle
		float cycle = ( renderView->time - inViewTime ) / 2000.0f;

		if(d > 0.94f)
		{
			if(!inView)
			{
				inView = true;
				if(cycle > lastCycle)
				{
					// restart at the beginning
					inViewTime = renderView->time;
					cycle = 0.0f;
				}
			}
		}
		else
		{
			if(inView)
			{
				inView = false;
				lastCycle = static_cast<int>(ceil(cycle));
			}
		}

		if(!inView && cycle > lastCycle)
		{
			renderEntity->shaderParms[4] = 0.0f;
		}
		else
		{
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
	}

	// update every single time this is in view
	return bRc;
}



/*
================
idItem::ModelCallback
================
*/
bool idItem::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView )
{
	idItem *ent;

	// this may be triggered by a model trace or other non-view related source
	if ( !renderView )
	{
		return false;
	}

	ent = static_cast<idItem *>(gameLocal.entities[ renderEntity->entityNum ]);
	if(!ent)
	{
		gameLocal.Error( "idItem::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idItem::Think
================
*/
void idItem::Think( void )
{
	if ( thinkFlags & TH_THINK )
	{
		if(spin)
		{
			idAngles	ang;
			idVec3		org;

			ang.pitch = ang.roll = 0.0f;
			ang.yaw = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
			SetAngles( ang );

			float scale = 0.005f + entityNumber * 0.00001f;
			
			org = orgOrigin;
			org.z += 4.0f + cos( ( gameLocal.time + 2000 ) * scale ) * 4.0f;
			SetOrigin( org );
		}
	}

	Present();
}

/*
================
idItem::Present
================
*/
void idItem::Present( void )
{
	idEntity::Present();

	if(!fl.hidden && pulse)
	{
		// also add a highlight shell model
		renderEntity_t	shell;

		shell = renderEntity;

		// we will mess with shader parms when the item is in view
		// to give the "item pulse" effect
		shell.callback = idItem::ModelCallback;

		shell.entityNum = entityNumber;
		shell.customShader = shellMaterial;
		if ( itemShellHandle == -1 )
		{
			itemShellHandle = gameRenderWorld->AddEntityDef( &shell );
		}
		else
		{
			gameRenderWorld->UpdateEntityDef( itemShellHandle, &shell );
		}
	}
}

/*
================
idItem::Spawn
================
*/
void idItem::Spawn( void )
{
	idStr		giveTo;
	idEntity *	ent;
	float		tsize;

	if ( spawnArgs.GetBool( "dropToFloor" ) )
	{
		PostEventMS( &EV_DropToFloor, 0 );
	}

	if ( spawnArgs.GetFloat( "triggersize", "0", tsize ) ) {
		GetPhysics()->GetClipModel()->LoadModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
		GetPhysics()->GetClipModel()->Link( gameLocal.clip );
	}

	if ( spawnArgs.GetBool( "start_off" ) )
	{
		GetPhysics()->SetContents( 0 );
		Hide();
	} else
	{
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}
	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	giveTo = spawnArgs.GetString( "owner" );
	if ( giveTo.Length() )
	{
		ent = gameLocal.FindEntity( giveTo );
		if ( !ent )
		{
			gameLocal.Error( "Item couldn't find owner '%s'", giveTo.c_str() );
		}
		PostEventMS( &EV_Touch, 0, ent, 0 );
	}

	if ( spawnArgs.GetBool( "spin" ) )
	{
		spin = true;
		BecomeActive( TH_THINK );
	}

	//pulse = !spawnArgs.GetBool( "nopulse" );
	//temp hack for tim
	pulse = false;
	orgOrigin = GetPhysics()->GetOrigin();
	b_orgOriginSet = true;

	noticeabilityIfAbsent = spawnArgs.GetFloat("noticeabilityIfAbsent");


	canPickUp = !( spawnArgs.GetBool( "triggerFirst" ) || spawnArgs.GetBool( "no_touch" ) );

	inViewTime = -1000;
	lastCycle = -1;
	itemShellHandle = -1;
	shellMaterial = declManager->FindMaterial( "itemHighlightShell" );

	// What team owns it?
	spawnArgs.GetInt ("ownerTeam", "0", ownerTeam);
}

/*
================
idItem::GetAttributes
================
*/
void idItem::GetAttributes( idDict &attributes )
{
	int					i;
	const idKeyValue	*arg;

	int num = spawnArgs.GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		if ( arg->GetKey().Cmpn( "inv_", 4 ) ) {
			attributes.Set( arg->GetKey().Right( arg->GetKey().Length() - 4 ), arg->GetValue() );
		}
	}
}

/*
================
idItem::GiveToPlayer
================
*/
bool idItem::GiveToPlayer( idPlayer *player )
{
	if ( player == NULL )
	{
		return false;
	}

	// greebo: Inventory Items are not handled here. InvItems should be added to the inventory
	// by calling AddToInventory().
	
	/*if ( spawnArgs.GetBool( "inv_carry" ) )
	{
		return player->GiveInventoryItem( &spawnArgs );
	}*/ 
	
	return false;//player->GiveItem( this );
}


/*
================
TDM: Darkmod spawns an absence marker entity
================

bool idItem::spawnAbsenceMarkerEntity()
{
	const char* pstr_markerDefName = "atdm:absence_marker";
	const idDict *p_markerDef = gameLocal.FindEntityDefDict( pstr_markerDefName, false );
	if( p_markerDef )
	{
		idEntity *ent2;
		gameLocal.SpawnEntityDef( *p_markerDef, &ent2, false );

		if ( !ent2 || !ent2->IsType( AbsenceMarker::Type ) ) 
		{
			gameLocal.Error( "Failed to spawn absence marker entity" );
			return false;
		}

		AbsenceMarker* p_absenceMarker = static_cast<AbsenceMarker*>( ent2 );

		// The absence marker has been created
		absenceEntityPtr = p_absenceMarker;

		// Initialize it
		idEntityPtr<idEntity> thisPtr;
		thisPtr = ((idEntity*) this);
		idMat3 orgOrientation;
		orgOrientation.Identity();

		p_absenceMarker->Init();
		if (!p_absenceMarker->initAbsenceReference (thisPtr, orgOrigin, orgOrientation))
		{
			gameLocal.Error( "Failed to initialize absence reference in absence marker entity" );
			return false;
		}
	}
	else
	{
			gameLocal.Error( "Failed to find definition of absence marker entity " );
			return false;
	}

	// Success
	return true;

}
*/
/*
================
TDM: Darkmod destroys an absence marker entity
================

void idItem::destroyAbsenceMarkerEntity()
{
	if (absenceEntityPtr.IsValid())
	{
		AbsenceMarker* p_absenceMarker = static_cast<AbsenceMarker*>( absenceEntityPtr.GetEntity() );
		delete p_absenceMarker;
		absenceEntityPtr = NULL;
	}
}
*/
/*
================
TDM: Darkmod checks if origin changed to create absence marker entity
idItem::UpdateVisuals
================
*/
void idItem::UpdateVisuals()
{
	// Call base class version
	idEntity::UpdateVisuals();

	// If we have not already spawned our absence entity and we are interested in spawning
	// one if the item moves
	if 
	(
		(noticeabilityIfAbsent > 0.0) && 
		(!absenceEntityPtr.IsValid())
	)
		if 
		(
			( (b_orgOriginSet) && (GetPhysics()->GetOrigin() != orgOrigin) ) ||
			( IsHidden() )
		)
	{
		// Spawn an absence entity
		// spawnAbsenceMarkerEntity();

	}
	// End has been moved, should be noticed, no absence marker spawned yet
	else if
	(
		(noticeabilityIfAbsent > 0.0) &&
		(absenceEntityPtr.IsValid()) &&
		(b_orgOriginSet) &&
		((GetPhysics()->GetOrigin() -orgOrigin).LengthFast() < 256.0)
	) // End should be noticed, absence marker spawned, it was put back
	{
		// Destroy the absence marker entity
		// destroyAbsenceMarkerEntity();
	}


}

/*
================
idItem::Pickup
================
*/
bool idItem::Pickup( idPlayer *player )
{
	if ( !GiveToPlayer( player ) )
	{
		return false;
	}

	// play pickup sound
	StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

	// trigger our targets
	ActivateTargets( player );

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0 );

	// hide the model
	Hide();

	// add the highlight shell
	if ( itemShellHandle != -1 )
	{
		gameRenderWorld->FreeEntityDef( itemShellHandle );
		itemShellHandle = -1;
	}

	// Spawn an absence marker entity
	// spawnAbsenceMarkerEntity();

	float respawn = spawnArgs.GetFloat( "respawn" );
	bool dropped = spawnArgs.GetBool( "dropped" );
	bool no_respawn = spawnArgs.GetBool( "no_respawn" );

	if ( respawn && !dropped && !no_respawn ) {
		const char *sfx = spawnArgs.GetString( "fxRespawn" );
		if ( sfx && *sfx ) {
			PostEventSec( &EV_RespawnFx, respawn - 0.5f );
		} 
		PostEventSec( &EV_RespawnItem, respawn );
	} else if ( !spawnArgs.GetBool( "inv_objective" ) && !no_respawn ) {
		// give some time for the pickup sound to play
		// FIXME: Play on the owner
		if ( !spawnArgs.GetBool( "inv_carry" ) ) {
			PostEventMS( &EV_Remove, 5000 );
		}
	}

	BecomeInactive( TH_THINK );
	return true;
}

/*
================
idItem::ClientPredictionThink
================
*/
void idItem::ClientPredictionThink( void ) {
	// only think forward because the state is not synced through snapshots
	if ( !gameLocal.isNewFrame ) {
		return;
	}
	Think();
}

/*
================
idItem::WriteFromSnapshot
================
*/
void idItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( IsHidden(), 1 );
}

/*
================
idItem::ReadFromSnapshot
================
*/
void idItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}
}

/*
================
idItem::ClientReceiveEvent
================
*/
bool idItem::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_PICKUP: {

			// play pickup sound
			StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

			// hide the model
			Hide();

			// remove the highlight shell
			if ( itemShellHandle != -1 ) {
				gameRenderWorld->FreeEntityDef( itemShellHandle );
				itemShellHandle = -1;
			}
			return true;
		}
		case EVENT_RESPAWN: {
			Event_Respawn();
			return true;
		}
		case EVENT_RESPAWNFX: {
			Event_RespawnFx();
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}

/*
================
idItem::Event_DropToFloor
================
*/
void idItem::Event_DropToFloor( void ) {
	trace_t trace;

	// don't drop the floor if bound to another entity
	if ( GetBindMaster() != NULL && GetBindMaster() != this ) {
		return;
	}

	gameLocal.clip.TraceBounds( trace, renderEntity.origin, renderEntity.origin - idVec3( 0, 0, 64 ), renderEntity.bounds, MASK_SOLID | CONTENTS_CORPSE, this );
	SetOrigin( trace.endpos );
}

/*
================
idItem::Event_Touch
================
*/
void idItem::Event_Touch( idEntity *other, trace_t *trace ) {
	// only allow touches from the player
	if ( !other->IsType( idPlayer::Type ) ) {
		return;
	}

	if ( !canPickUp ) {
		return;
	}

	Pickup( static_cast<idPlayer *>(other) );
}

/*
================
idItem::Event_Trigger
================
*/
void idItem::Event_Trigger( idEntity *activator ) {

	if ( !canPickUp && spawnArgs.GetBool( "triggerFirst" ) ) {
		canPickUp = true;
		return;
	}

	if ( activator && activator->IsType( idPlayer::Type ) ) {
		Pickup( static_cast<idPlayer *>( activator ) );
	}
}

/*
================
idItem::Event_Respawn
================
*/
void idItem::Event_Respawn( void ) {
	BecomeActive( TH_THINK );
	Show();
	inViewTime = -1000;
	lastCycle = -1;
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	SetOrigin( orgOrigin );
	StartSound( "snd_respawn", SND_CHANNEL_ITEM, 0, false, NULL );
	CancelEvents( &EV_RespawnItem ); // don't double respawn
}

/*
================
idItem::Event_RespawnFx
================
*/
void idItem::Event_RespawnFx( void ) {
	const char *sfx = spawnArgs.GetString( "fxRespawn" );
	if ( sfx && *sfx ) {
		idEntityFx::StartFx( sfx, NULL, NULL, this, true );
	}
}

/*
===============================================================================

  idMoveableItem
	
===============================================================================
*/

CLASS_DECLARATION( idItem, idMoveableItem )
	EVENT( EV_DropToFloor,	idMoveableItem::Event_DropToFloor )
	EVENT( EV_Gib,			idMoveableItem::Event_Gib )
END_CLASS

/*
================
idMoveableItem::idMoveableItem
================
*/
idMoveableItem::idMoveableItem() {
	trigger = NULL;
	smoke = NULL;
	smokeTime = 0;
}

/*
================
idMoveableItem::~idMoveableItem
================
*/
idMoveableItem::~idMoveableItem() {
	if ( trigger ) {
		delete trigger;
	}
}

/*
================
idMoveableItem::Save
================
*/
void idMoveableItem::Save( idSaveGame *savefile ) const {
   	savefile->WriteStaticObject( physicsObj );

	savefile->WriteClipModel( trigger );

	savefile->WriteParticle( smoke );
	savefile->WriteInt( smokeTime );
}

/*
================
idMoveableItem::Restore
================
*/
void idMoveableItem::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadClipModel( trigger );

	savefile->ReadParticle( smoke );
	savefile->ReadInt( smokeTime );
}

/*
================
idMoveableItem::Spawn
================
*/
void idMoveableItem::Spawn( void ) 
{
	idTraceModel trm;
	float density, friction, air_friction_linear, air_friction_angular, bouncyness, tsize;
	idStr clipModelName;
	idBounds FrobBounds;
	idTraceModel FrobTrm;
	int numSides(0);

	// create a frob box separate from the collision box for easier frobbing
	// First check if frobbox_mins and frobbox_maxs are set
	if ( spawnArgs.GetVector( "frobbox_mins", NULL, FrobBounds[0] ) &&
				spawnArgs.GetVector( "frobbox_maxs", NULL, FrobBounds[1] ) )
	{
		if ( FrobBounds[0][0] > FrobBounds[1][0] || FrobBounds[0][1] >FrobBounds[1][1] || FrobBounds[0][2] > FrobBounds[1][2] )
		{
			gameLocal.Error( "Invalid frob box bounds '%s'-'%s' on entity '%s'", FrobBounds[0].ToString(), FrobBounds[1].ToString(), name.c_str() );
		}
	} 
	else 
	{
		spawnArgs.GetFloat( "frobbox_size", "10.0", tsize );
		FrobBounds.Zero();
		FrobBounds.ExpandSelf( tsize );
	}

	if ( spawnArgs.GetInt( "frobbox_cylinder", "0", numSides ) && numSides > 0 ) {
		FrobTrm.SetupCylinder( FrobBounds, numSides < 3 ? 3 : numSides );
	} else if ( spawnArgs.GetInt( "frobbox_cone", "0", numSides ) && numSides > 0 ) {
		FrobTrm.SetupCone( FrobBounds, numSides < 3 ? 3 : numSides );
	} else {
		FrobTrm.SetupBox( FrobBounds );
	}

	// Initialize frob bounds based on previous spawnarg setup
	trigger = new idClipModel( FrobTrm );
	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	trigger->SetContents( CONTENTS_FROBABLE );

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		gameLocal.Error( "idMoveableItem '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
		return;
	}

	// if the model should be shrinked
	if ( spawnArgs.GetBool( "clipshrink" ) ) {
		trm.Shrink( CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	spawnArgs.GetFloat( "friction", "0.05", friction );
	// reverse compatibility, new contact_friction key replaces friction only if present
	if( spawnArgs.FindKey("contact_friction") )
	{
		spawnArgs.GetFloat( "contact_friction", "0.05", friction );
	}
	spawnArgs.GetFloat( "linear_friction", "0.6", air_friction_linear );
	spawnArgs.GetFloat( "angular_friction", "0.6", air_friction_angular );

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm ), density );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( air_friction_linear, air_friction_angular, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	// greebo: Allow the entityDef to override the clipmodel contents
	if( spawnArgs.FindKey( "clipmodel_contents" ) )
	{
		physicsObj.SetContents( spawnArgs.GetInt("clipmodel_contents") );
	}

	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );
	SetPhysics( &physicsObj );

	smoke = NULL;
	smokeTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_trail" );
	if ( *smokeName != '\0' ) {
		smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeTime = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	}
}

/*
================
idMoveableItem::Think
================
*/
void idMoveableItem::Think( void ) {

	RunPhysics();

	if ( thinkFlags & TH_PHYSICS ) {
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}
	
	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		if ( !gameLocal.smokeParticles->EmitSmoke( smoke, smokeTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() ) ) {
			smokeTime = 0;
			BecomeInactive( TH_UPDATEPARTICLES );
		}
	}

	Present();
}

/*
================
idMoveableItem::Pickup
================
*/
bool idMoveableItem::Pickup( idPlayer *player ) {
	bool ret = idItem::Pickup( player );
	if ( ret ) {
		trigger->SetContents( 0 );
	} 
	return ret;
}

/*
================
idMoveableItem::DropItem
================
*/
idEntity *idMoveableItem::DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay ) {
	idDict args;
	idEntity *item;

	args.Set( "classname", classname );
	args.Set( "dropped", "1" );

	// we sometimes drop idMoveables here, so set 'nodrop' to 1 so that it doesn't get put on the floor
	args.Set( "nodrop", "1" );

	if ( activateDelay ) {
		args.SetBool( "triggerFirst", true );
	}

	gameLocal.SpawnEntityDef( args, &item );
	if ( item ) {
		// set item position
		item->GetPhysics()->SetOrigin( origin );
		item->GetPhysics()->SetAxis( axis );
		item->GetPhysics()->SetLinearVelocity( velocity );
		item->UpdateVisuals();
		if ( activateDelay ) {
			item->PostEventMS( &EV_Activate, activateDelay, item );
		}

/**
* TDM: Do not automatically remove dropped items 
* (Original Id code automatically removed it after 5 minutes)
**/
		if ( removeDelay ) 
		{
			item->PostEventMS( &EV_Remove, removeDelay );
		}	
	}
	return item;
}

/*
================
idMoveableItem::DropItems

  The entity should have the following key/value pairs set:
	"def_drop<type>Item"			"item def"
	"drop<type>ItemJoint"			"joint name"
	"drop<type>ItemRotation"		"pitch yaw roll"
	"drop<type>ItemOffset"			"x y z"
	"skin_drop<type>"				"skin name"
  To drop multiple items the following key/value pairs can be used:
	"def_drop<type>Item<X>"			"item def"
	"drop<type>Item<X>Joint"		"joint name"
	"drop<type>Item<X>Rotation"		"pitch yaw roll"
	"drop<type>Item<X>Offset"		"x y z"
  where <X> is an aribtrary string.
================
*/
void idMoveableItem::DropItems( idAnimatedEntity  *ent, const char *type, idList<idEntity *> *list ) {
	const idKeyValue *kv;
	const char *skinName, *c, *jointName;
	idStr key, key2;
	idVec3 origin;
	idMat3 axis;
	idAngles angles;
	const idDeclSkin *skin;
	jointHandle_t joint;
	idEntity *item;

	// drop all items
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), NULL );
	while ( kv ) {

		c = kv->GetKey().c_str() + kv->GetKey().Length();
		if ( idStr::Icmp( c - 5, "Joint" ) != 0 && idStr::Icmp( c - 8, "Rotation" ) != 0 ) {

			key = kv->GetKey().c_str() + 4;
			key2 = key;
			key += "Joint";
			key2 += "Offset";
			jointName = ent->spawnArgs.GetString( key );
			joint = ent->GetAnimator()->GetJointHandle( jointName );
			if ( !ent->GetJointWorldTransform( joint, gameLocal.time, origin, axis ) ) {
				gameLocal.Warning( "%s refers to invalid joint '%s' on entity '%s'", key.c_str(), jointName, ent->name.c_str() );
				origin = ent->GetPhysics()->GetOrigin();
				axis = ent->GetPhysics()->GetAxis();
			}
			if ( g_dropItemRotation.GetString()[0] ) {
				angles.Zero();
				sscanf( g_dropItemRotation.GetString(), "%f %f %f", &angles.pitch, &angles.yaw, &angles.roll );
			} else {
				key = kv->GetKey().c_str() + 4;
				key += "Rotation";
				ent->spawnArgs.GetAngles( key, "0 0 0", angles );
			}
			axis = angles.ToMat3() * axis;

			origin += ent->spawnArgs.GetVector( key2, "0 0 0" );

			item = DropItem( kv->GetValue(), origin, axis, vec3_origin, 0, 0 );
			if ( list && item ) {
				list->Append( item );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), kv );
	}

	// change the skin to hide all items
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if ( skinName[0] ) {
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
======================
idMoveableItem::WriteToSnapshot
======================
*/
void idMoveableItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
======================
idMoveableItem::ReadFromSnapshot
======================
*/
void idMoveableItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
============
idMoveableItem::Gib
============
*/
void idMoveableItem::Gib( const idVec3 &dir, const char *damageDefName ) {
	// spawn smoke puff
	const char *smokeName = spawnArgs.GetString( "smoke_gib" );
	if ( *smokeName != '\0' ) {
		const idDeclParticle *smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		gameLocal.smokeParticles->EmitSmoke( smoke, gameLocal.time, gameLocal.random.CRandomFloat(), renderEntity.origin, renderEntity.axis );
	}
	// remove the entity
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idMoveableItem::Event_DropToFloor
================
*/
void idMoveableItem::Event_DropToFloor( void ) {
	// the physics will drop the moveable to the floor
}

/*
============
idMoveableItem::Event_Gib
============
*/
void idMoveableItem::Event_Gib( const char *damageDefName ) {
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

void idMoveableItem::Hide( void )
{
	idEntity::Hide();
	physicsObj.SetContents( 0 );
	trigger->SetContents( 0 );
}

void idMoveableItem::Show( void )
{
	idEntity::Show();
	physicsObj.SetContents( m_preHideContents );

	trigger->SetContents( CONTENTS_FROBABLE );
}

