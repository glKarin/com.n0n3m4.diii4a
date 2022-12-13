#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


const idEventDef EV_PollForExit("<pollforexit>", NULL);
const float	proxDoorRefreshMS = 50.f;		//refresh rate of door while open (controls how frequently to check if someone is still close enough)


ABSTRACT_DECLARATION( idEntity, hhProxDoorSection )
END_CLASS

void hhProxDoorSection::Spawn( void ) {
	fl.networkSync = true;
	proximity = 0.0f;
	hasNetData = false;
	proxyParent = NULL;
}

void hhProxDoorSection::ClientPredictionThink( void ) {
	Show();
	idEntity::ClientPredictionThink();
}

void hhProxDoorSection::WriteToSnapshot( idBitMsgDelta &msg ) const {
	WriteBindToSnapshot(msg);
	GetPhysics()->WriteToSnapshot(msg);
}

void hhProxDoorSection::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ReadBindFromSnapshot(msg);
	GetPhysics()->ReadFromSnapshot(msg);

	if (msg.HasChanged()) {
		Show();
		UpdateVisuals();
		Present();
		GetPhysics()->LinkClip();
	}
}



//-------------------------------------------------------------------------------------------------
// hhProxDoor.
//-------------------------------------------------------------------------------------------------
CLASS_DECLARATION( idEntity, hhProxDoor )
	EVENT( EV_Touch,				hhProxDoor::Event_Touch )
	EVENT( EV_PollForExit,			hhProxDoor::Event_PollForExit )
	EVENT( EV_PostSpawn,			hhProxDoor::Event_PostSpawn )
	EVENT( EV_Activate,				hhProxDoor::Event_Activate )
END_CLASS

//--------------------------------
// hhProxDoor::hhProxDoor
//--------------------------------
hhProxDoor::hhProxDoor() {
	areaPortal = 0;
	proxState = PROXSTATE_Inactive;
	doorTrigger = NULL;
	aas_area_closed = false;
	hasNetData = false;
	sndTrigger = NULL;
	nextSndTriggerTime = 0;
	//HUMANHEAD PCF rww 06/06/06 - fix for snappy prox doors
	lastAmount = 0.0f;
	//HUMANHEAD END
}

//--------------------------------
// hhProxDoor::hhProxDoor
//--------------------------------
hhProxDoor::~hhProxDoor() {
	if (doorTrigger) {
		doorTrigger->Unlink();
		delete doorTrigger;
		doorTrigger = NULL;
	}
	if ( sndTrigger ) {
		sndTrigger->Unlink();
		delete sndTrigger;
		sndTrigger = NULL;
	}
}

//--------------------------------
// hhProxDoor::Spawn
//--------------------------------
void hhProxDoor::Spawn() {
	proxState = PROXSTATE_Inactive;
	doorSndState = PDOORSND_Closed;

	spawnArgs.GetBool( "startlocked", "0", doorLocked );
	SetShaderParm( SHADERPARM_MODE, GetDoorShaderParm( doorLocked, true ) );	// 2=locked, 1=unlocked, 0=never locked

	if( !spawnArgs.GetFloat("trigger_distance", "0.0", maxDistance) ) {
		common->Warning( "No 'trigger_distance' specified for entityDef '%s'", this->GetEntityDefName() );
	}
	if( !spawnArgs.GetFloat("movement_distance", "0.0", movementDistance) ) {
		common->Warning( "No 'movement_distance' specified for entityDef '%s'", this->GetEntityDefName() );
	}
	lastDistance = movementDistance + 1.0f; // Safe default -mdl

	if( !spawnArgs.GetFloat("stop_distance", "0.0", stopDistance) ) {
		common->Warning( "No 'stop_distance' specificed for entityDef '%s'", this->GetEntityDefName() );
	}

	spawnArgs.GetBool( "openForMonsters", "1", openForMonsters );
	spawnArgs.GetFloat( "damage", "1", damage );

	//Find any portal boundary we are on
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );

	if (gameLocal.isMultiplayer) {
		fl.networkSync = true;

		if (gameLocal.isClient) {
			doorTrigger = new idClipModel( idTraceModel(idBounds(vec3_origin).Expand(maxDistance)) );
			doorTrigger->Link( gameLocal.clip, this, 255, GetOrigin(), GetAxis() );
			doorTrigger->SetContents( CONTENTS_TRIGGER );

			SetAASAreaState( doorLocked );			//close the area state if we are locked...
			SetDoorState( PROXSTATE_Inactive );
		}
		else {
			PostEventMS( &EV_PostSpawn, 50 ); //rww - this must be delayed or it will happen on the first server frame and we DON'T want that.
		}
	}
	else { //just call it now. some sp maps might target a door piece in the spawn function of an entity or something crazy like that.
		Event_PostSpawn();
	}
}

//--------------------------------
// hhProxDoor::Event_PostSpawn
//--------------------------------
void hhProxDoor::Event_PostSpawn( void ) {
	int numSubObjs;
	int i;
	const char* objDef;

//Parse and spawn our door sections
	numSubObjs = spawnArgs.GetInt( "num_doorobjs", "0" );
	for( i = 0; i < numSubObjs; i++ ) {
		if( !spawnArgs.GetString( va("doorobject%i", i+1), "", &objDef ) ) {
			common->Warning( "failed to find doorobject%i", i+1 );
		}
		else {
			//Set our default rotation/origin.
			idDict args;
			args.SetVector( "origin", this->GetOrigin() );
			args.SetMatrix( "rotation", this->GetAxis() );
			hhProxDoorSection* ent = static_cast<hhProxDoorSection*> ( gameLocal.SpawnObject(objDef, &args) );
			if( !ent ) {
				common->Warning( "failed to spawn doorobject%i for entityDef '%s'", i+1, GetEntityDefName() );
			}
			else {
				ent->proxyParent = this;
				doorPieces.Append( ent );
			}
		}
	} 
//Spawn our trigger and link it up
//	if( doorLocked && !spawnArgs.GetBool("locktrigger") ) {
		//we would never be able to open the door, so don't spawn a trigger
//	}
//	else {
		doorTrigger = new idClipModel( idTraceModel(idBounds(vec3_origin).Expand(maxDistance)) );
		doorTrigger->Link( gameLocal.clip, this, 255, GetOrigin(), GetAxis() );
		doorTrigger->SetContents( CONTENTS_TRIGGER );
//	}

	SpawnSoundTrigger();

	SetAASAreaState( doorLocked );			//close the area state if we are locked...
	SetDoorState( PROXSTATE_Inactive );
}

void hhProxDoor::SpawnSoundTrigger() {
	idBounds		bounds,localbounds;
	int				i;
	int				best;

	// Since models bounds are overestimated, we need to use the bounds from the
	// clipmodel, which was set before the over-estimation
	localbounds = GetPhysics()->GetBounds();

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( localbounds[1][ i ] - localbounds[0][ i ] < localbounds[1][ best ] - localbounds[0][ best ] ) {
			best = i;
		}
	}
	localbounds[1][ best ] += 50;
	localbounds[0][ best ] -= 50;

	// Now transform into absolute coordintates
	if ( GetPhysics()->GetAxis().IsRotated() ) {
		bounds.FromTransformedBounds( localbounds, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}
	else {
		bounds[0] = localbounds[0] + GetPhysics()->GetOrigin();
		bounds[1] = localbounds[1] + GetPhysics()->GetOrigin();
	}
	bounds[0] -= GetPhysics()->GetOrigin();
	bounds[1] -= GetPhysics()->GetOrigin();

	// create a trigger clip model
	sndTrigger = new idClipModel( idTraceModel( bounds ) );
	sndTrigger->Link( gameLocal.clip, this, 254, GetPhysics()->GetOrigin(), mat3_identity );
	sndTrigger->SetContents( CONTENTS_TRIGGER );
}

void hhProxDoor::Save(idSaveGame *savefile) const {
	int i;

	savefile->WriteInt( proxState );
	savefile->WriteInt( doorSndState );

	savefile->WriteInt( doorPieces.Num() );					// idList<class hhProxDoorSection*>
	for (i=0; i<doorPieces.Num(); i++) {
		//savefile->WriteObject( doorPieces[i] );
		doorPieces[i].Save(savefile);
	}

	savefile->WriteClipModel( doorTrigger );
	savefile->WriteFloat( entDistanceSq );
	savefile->WriteFloat( maxDistance );
	savefile->WriteFloat( movementDistance );
	savefile->WriteFloat( stopDistance );
	savefile->WriteInt( areaPortal );
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}
	savefile->WriteBool( doorLocked );
	savefile->WriteFloat( lastAmount );
	savefile->WriteBool( openForMonsters );
	savefile->WriteBool( aas_area_closed );
	savefile->WriteFloat( lastDistance );
	savefile->WriteClipModel( sndTrigger );
	savefile->WriteInt( nextSndTriggerTime );
}

void hhProxDoor::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadInt( (int &)proxState );
	savefile->ReadInt( (int &)doorSndState );

	doorPieces.Clear();
	savefile->ReadInt( num );					// idList<class hhProxDoorSection*>
	doorPieces.SetNum( num );
	for (i=0; i<num; i++) {
		//savefile->ReadObject( reinterpret_cast<idClass *&>(doorPieces[i]) );
		doorPieces[i].Restore(savefile);
	}

	savefile->ReadClipModel( doorTrigger );
	savefile->ReadFloat( entDistanceSq );
	savefile->ReadFloat( maxDistance );
	savefile->ReadFloat( movementDistance );
	savefile->ReadFloat( stopDistance );
	savefile->ReadInt( (int &)areaPortal );
	if ( areaPortal ) {
		int portalState;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}
	savefile->ReadBool( doorLocked );
	savefile->ReadFloat( lastAmount );
	savefile->ReadBool( openForMonsters );
	savefile->ReadBool( aas_area_closed );
	savefile->ReadFloat( lastDistance );

	SetAASAreaState( aas_area_closed );

	spawnArgs.GetFloat( "damage", "1", damage );
	savefile->ReadClipModel( sndTrigger );
	savefile->ReadInt( nextSndTriggerTime );
}

void hhProxDoor::ClientPredictionThink( void ) {
	Think();
}

void hhProxDoor::WriteToSnapshot( idBitMsgDelta &msg ) const {
	WriteBindToSnapshot(msg);
	GetPhysics()->WriteToSnapshot(msg);

	msg.WriteBits(doorPieces.Num(), 8);
	for (int i = 0; i < doorPieces.Num(); i++) {
		msg.WriteBits(doorPieces[i].GetSpawnId(), 32);
	}

	msg.WriteBits(proxState, 8);

	msg.WriteFloat(lastAmount);

	msg.WriteBits(aas_area_closed, 1);

	//don't need this, predicting door movement.
	//msg.WriteBits(doorSndState, 8);
}

void hhProxDoor::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	ReadBindFromSnapshot(msg);
	GetPhysics()->ReadFromSnapshot(msg);

	int num = msg.ReadBits(8);
	doorPieces.SetNum(num);
	for (int i = 0; i < num; i++) {
		int spawnId = msg.ReadBits(32);
		if (!spawnId) {
			doorPieces[i] = NULL;
		}
		else {
			doorPieces[i].SetSpawnId(spawnId);
		}
	}

	EProxState newProxState = (EProxState)msg.ReadBits(8);
	if (proxState != newProxState) {
		SetDoorState(newProxState);
	}

	lastAmount = msg.ReadFloat();

	bool closed = !!msg.ReadBits(1);
	if (aas_area_closed != closed) {
		SetAASAreaState(closed);
	}

	/*
	EPDoorSound newSndState = (EPDoorSound)msg.ReadBits(8);
	if (newSndState != doorSndState) {
		UpdateSoundState(newSndState);
	}
	*/

	hasNetData = true;
}

//--------------------------------
// hhProxDoor::Think
//--------------------------------
void hhProxDoor::Ticker( void ) {
	int i;
	float bestDistSq, bestDist = idMath::INFINITY, amount;

	//HUMANHEAD PCF mdl 04/27/06 - Locked doors are forced inactive
	if ( doorLocked && proxState == PROXSTATE_Active ) {
		proxState = PROXSTATE_GoingInactive;
	}

	if ( proxState == PROXSTATE_GoingInactive ) {
		float fullyOpen  = (movementDistance - stopDistance) / movementDistance;
		//HUMANHEAD PCF mdl 04/27/06 - Added 30 HZ multiplier
		float step = (0.2f * ( 1.0f / ( 1000.0f / 60.0f ) ) )  * (60.0f * USERCMD_ONE_OVER_HZ);
		float low = idMath::ClampFloat( 0.0f, 1.0f, lastAmount - step );
		float high = idMath::ClampFloat( 0.0f, 1.0f, lastAmount + step );
		amount = idMath::ClampFloat( low, high, 0.0f );
		//HUMANHEAD PCF mdl 04/27/06 - Removed old lock code that was here
		for( i = 0; i < doorPieces.Num(); i++ ) {
			if (doorPieces[i].IsValid()) { //rww - added in case something decides to remove one of the pieces externally.
				if (gameLocal.isClient && areaPortal && gameRenderWorld->GetPortalState(areaPortal) != PS_BLOCK_NONE) {
					amount = 0.0f; //rww - hax for portal state sync'ing on client and server
				}
				doorPieces[ i ]->SetProximity( amount );
			}
		}

		if ( amount == 0.0f ) {
			SetDoorState( PROXSTATE_Inactive );
			UpdateSoundState( PDOORSND_Closed );
		} else {
			UpdateSoundState( PDOORSND_Closing );
		}

	} else {

		bestDist = movementDistance;
		bestDistSq = PollClosestEntity();
		if( bestDistSq >= 0.f ) {
			bestDist = idMath::Sqrt( bestDistSq );
		}

		if ( bestDistSq == -1.0f ) {
			SetDoorState( PROXSTATE_GoingInactive );
			return;
		}

		// Set the default amount to no change
		amount = lastAmount;
		if( bestDist < movementDistance ) {
			float fullyOpen  = (movementDistance - stopDistance) / movementDistance;
			amount = idMath::ClampFloat( 0.f, fullyOpen, (1.f - ( bestDist / movementDistance )) );
			//HUMANHEAD PCF mdl 04/27/06 - Added 30 HZ multiplier
			float step = (0.2f * ( 1.0f / ( 1000.0f / 60.0f ) ) ) * (60.0f * USERCMD_ONE_OVER_HZ);
			float low = idMath::ClampFloat( 0.0f, 1.0f, lastAmount - step );
			float high = idMath::ClampFloat( 0.0f, 1.0f, lastAmount + step );
			amount = idMath::ClampFloat( low, high, amount );
			if ( doorLocked ) {
				for( i = 0; i < doorPieces.Num(); i++ ) {
					if (doorPieces[i].IsValid()) {
						doorPieces[ i ]->SetProximity( 0.0f );
					}
				}
			} else {
				for( i = 0; i < doorPieces.Num(); i++ ) {
					if (doorPieces[i].IsValid()) { //rww - added in case something decides to remove one of the pieces externally.
						if (gameLocal.isClient && areaPortal && gameRenderWorld->GetPortalState(areaPortal) != PS_BLOCK_NONE) {
							amount = 0.0f; //rww - hax for portal state sync'ing on client and server
						}
						doorPieces[ i ]->SetProximity( amount );
					}
				}
			}
			if( lastAmount == -1 && amount >= 0.f ) {	//was closed, just starting to open
				UpdateSoundState( PDOORSND_Opened );
			}
			else if( lastAmount > amount ) {			//closing
				UpdateSoundState( PDOORSND_Closing );
			}
			else if( lastAmount < amount ) {			//opening
				UpdateSoundState( PDOORSND_Opening );
			}
			else if( amount == fullyOpen ) {			//fully open
				UpdateSoundState( PDOORSND_FullyOpened );
			}
			else {									//stop sounds, since we aren't moving
				UpdateSoundState( PDOORSND_Stopped );
			}
		}
		else {
			SetDoorState( PROXSTATE_GoingInactive );
			UpdateSoundState( PDOORSND_Closing );
		}
	}

	// If we're closing and 75% closed or more, crush anything unlucky enough to be inside us (don't do this on the client -rww)
	if ( !gameLocal.isClient && lastAmount < 0.25f && lastAmount != -1.0f && amount != -1.0f && amount < lastAmount ) {
		CrushEntities();
	}
	lastAmount = amount;
	lastDistance = bestDist;
}

void hhProxDoor::UpdateSoundState( EPDoorSound newState ) {
	if( doorSndState == newState ) {
		return;
	}
	switch( newState ) {
		case PDOORSND_Opening:		
			StopSound( SND_CHANNEL_BODY );
			StartSound( "snd_closing", SND_CHANNEL_BODY );	
			break;

		case PDOORSND_Closing:
			StopSound( SND_CHANNEL_BODY );
			StartSound( "snd_opening", SND_CHANNEL_BODY );	
			break;

		case PDOORSND_Closed:		
			StopSound( SND_CHANNEL_BODY2 );
			StopSound( SND_CHANNEL_BODY );
			StartSound( "snd_closed", SND_CHANNEL_BODY2 );	
			break;

		case PDOORSND_Opened:		
			StopSound( SND_CHANNEL_BODY2 );
			StartSound( "snd_opened", SND_CHANNEL_BODY2 );	
			break;

		case PDOORSND_Stopped:		
			StopSound( SND_CHANNEL_BODY );
			break;

		case PDOORSND_FullyOpened:
			StopSound( SND_CHANNEL_BODY );
			StopSound( SND_CHANNEL_BODY2 );
			StartSound( "snd_fullyopened", SND_CHANNEL_BODY2 );
			break;
	}
	doorSndState = newState;
}

//--------------------------------
// hhProxDoor::OpenPortal
//--------------------------------
void hhProxDoor::OpenPortal( void ) {
	if( areaPortal && !gameLocal.isClient ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_NONE );
	}
	SetAASAreaState( false );
}

//--------------------------------
// hhProxDoor::ClosePortal
//--------------------------------
void hhProxDoor::ClosePortal( void ) {
	if( areaPortal && !gameLocal.isClient ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_VIEW );
	}
	SetAASAreaState( IsLocked() );
}

//--------------------------------
// hhProxDoor::SetAASAreaState
//--------------------------------
void hhProxDoor::SetAASAreaState( bool closed ) {
	aas_area_closed = closed;
	if( !openForMonsters ) {
		aas_area_closed = true;
	}
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL | AREACONTENTS_OBSTACLE, aas_area_closed );
}

//--------------------------------
// hhProxDoor::Event_PollForExit
//--------------------------------
void hhProxDoor::Event_PollForExit() {
	if( PollClosestEntity() == -1.f ) {
		SetDoorState( PROXSTATE_GoingInactive );
		return;
	}
	CancelEvents( &EV_PollForExit );
	PostEventMS( &EV_PollForExit, proxDoorRefreshMS );
}

//--------------------------------
// hhProxDoor::PollClosestEntity
//--------------------------------
float hhProxDoor::PollClosestEntity() {
	int num;
	int i;
	idEntity* ents[MAX_GENTITIES];
	float bestLen;
	float distSq;

	if (!doorTrigger) {
		return 0.0f;
	}

	num = gameLocal.clip.EntitiesTouchingBounds( doorTrigger->GetAbsBounds().Expand( pm_bboxwidth.GetFloat() ), MASK_SHOT_BOUNDINGBOX, ents, MAX_GENTITIES );

	float thinkDistanceSq = (maxDistance + pm_bboxwidth.GetFloat()) * (maxDistance + pm_bboxwidth.GetFloat());
	bestLen = thinkDistanceSq;
	idEntity* bestEnt = NULL;

	for( i = 0; i < num; i++ ) {
		if( ents[ i ] && ents[ i ] != this && ents[ i ]->fl.touchTriggers && !ents[ i ]->IsType(hhProxDoorSection::Type) ) {
			if( ents[ i ]->IsType(hhPlayer::Type) || ents[ i ]->IsType(hhSpiritProxy::Type) || (openForMonsters && ents[ i ]->IsType(idAI::Type) && ents[ i ]->GetHealth() > 0 ) ) {
				if (ents[ i ]->IsType(hhPlayer::Type) && !fl.allowSpiritWalkTouch) { //rww - we don't want spirits to open proxy doors.
					hhPlayer *pl = static_cast<hhPlayer *>(ents[i]);
					if (pl->IsSpiritWalking()) {
						continue;
					}
				}
				distSq = ( ents[ i ]->GetOrigin() - GetOrigin() ).LengthSqr();
				if( distSq < bestLen ) {
					bestLen = distSq;
					bestEnt = ents[ i ];
				}
			}
		}
	}
	if( bestLen < thinkDistanceSq ) {
		entDistanceSq = bestLen;
	}
	else {
		entDistanceSq = -1.f;
	}
	return entDistanceSq;
}

//--------------------------------
// hhProxDoor::SetDoorState
//--------------------------------
void hhProxDoor::SetDoorState( EProxState doorState ) {
	int i;

	//HUMANHEAD PCF mdl 04/27/06 - Don't allow locked doors to become active
	if ( doorLocked && doorState == PROXSTATE_Active ) {
		return;
	}

	switch( doorState ) {
		case PROXSTATE_Active:
			BecomeActive( TH_TICKER );
			CancelEvents( &EV_PollForExit );
			PostEventMS( &EV_PollForExit, 500 );
			OpenPortal();
			break;

		case PROXSTATE_GoingInactive:
			break;

		case PROXSTATE_Inactive:
			// Guarantee the door is closed
			for( i = 0; i < doorPieces.Num(); i++ ) {
				if (doorPieces[i].IsValid()) {
					doorPieces[ i ]->SetProximity( 0.0 );
				}
			}
			ClosePortal();
			CancelEvents( &EV_PollForExit );
			BecomeInactive( TH_TICKER );
			break;
	}

	proxState = doorState;
}

//--------------------------------
// hhProxDoor::IsLocked
//--------------------------------
bool hhProxDoor::IsLocked() {
	return doorLocked;
}

//--------------------------------
// hhProxDoor::Lock
//--------------------------------
void hhProxDoor::Lock( int f ) {
	doorLocked = f > 0 ? true : false;
	//HUMANHEAD PCF mdl 04/27/06 - Changed PROXSTATE_Inactive to PROXSTATE_GoingInactive
	SetDoorState( doorLocked ? PROXSTATE_GoingInactive : PROXSTATE_Active );
	SetShaderParm( SHADERPARM_MODE, GetDoorShaderParm( doorLocked, false ) );	// 2=locked, 1=unlocked, 0=never locked
	StopSound( SND_CHANNEL_ANY );
}

//--------------------------------
// hhProxDoor::CrushEntities
//--------------------------------
void hhProxDoor::CrushEntities() {
	int num;
	int i;
	idEntity* ents[MAX_GENTITIES];

	num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SHOT_BOUNDINGBOX|CONTENTS_RENDERMODEL, ents, MAX_GENTITIES );

	const char *damageType = spawnArgs.GetString("damageType");
	for( i = 0; i < num; i++ ) {
		if( ents[ i ] != this && !ents[ i ]->IsType(hhProxDoorSection::Type) && !ents[ i ]->IsType(hhPlayer::Type) && !ents[ i ]->IsType(idAFAttachment::Type) ) { // Check for the player and idAFAttachment (head) in the case of spirit walk
			ents[ i ]->Damage( this, this, vec3_origin, damageType, damage, INVALID_JOINT );
			ents[ i ]->SquishedByDoor( this );
		}
	}
}

//--------------------------------
// hhProxDoor::Event_Touch
//--------------------------------
void hhProxDoor::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( sndTrigger && trace->c.id == sndTrigger->GetId() ) {
		if (other && other->IsType(hhPlayer::Type) && IsLocked() && gameLocal.time > nextSndTriggerTime) {
			StartSound("snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
			nextSndTriggerTime = gameLocal.time + 10000;
		}
		return;
	}
	if( proxState == PROXSTATE_Active ) {
		return;
	}

	if ( !other ) {
		gameLocal.Warning("hhProxDoor:  Event_Touch given NULL for other\n");
		return;
	}

	float dist = ( other->GetOrigin() - GetOrigin() ).Length();
	if (dist > movementDistance) {
		return;
	}

	if( !IsLocked() ) {
		SetDoorState( PROXSTATE_Active );
	}
}

//--------------------------------
// hhProxDoor::Event_Activate
//--------------------------------
void hhProxDoor::Event_Activate( idEntity* activator ) {
	if( spawnArgs.GetBool("locktrigger") ) {
		Lock(!doorLocked);
		SetAASAreaState( doorLocked );
	}
}


//-------------------------------------------------------------------------------------------------
// hhProxDoorTranslator.
//-------------------------------------------------------------------------------------------------
CLASS_DECLARATION( hhProxDoorSection, hhProxDoorTranslator )
END_CLASS

//--------------------------------
// hhProxDoorTranslator::Spawn
//--------------------------------
void hhProxDoorTranslator::Spawn( void ) {
	idVec3	sectionOffset;

	sectionType = PROXSECTION_Translator;

	if( !spawnArgs.GetVector("section_offset", "0 0 0", sectionOffset) ) {
		common->Warning( "No section offset found for '%s'", this->GetEntityDefName() );
	}
	targetOrigin = sectionOffset;
	baseOrigin = GetOrigin();

	fl.networkSync = true;
}

//--------------------------------
// hhProxDoorTranslator::Save
//--------------------------------
void hhProxDoorTranslator::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( baseOrigin );
	savefile->WriteVec3( targetOrigin );
}

//--------------------------------
// hhProxDoorTranslator::Restore
//--------------------------------
void hhProxDoorTranslator::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( baseOrigin );
	savefile->ReadVec3( targetOrigin );
}

//--------------------------------
// hhProxDoorTranslator::ClientPredictionThink
//--------------------------------
void hhProxDoorTranslator::ClientPredictionThink( void ) {
	hhProxDoorSection::ClientPredictionThink();
}

//--------------------------------
// hhProxDoorTranslator::WriteToSnapshot
//--------------------------------
void hhProxDoorTranslator::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//hhProxDoorSection::WriteToSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	msg.WriteFloat(baseOrigin.x);
	msg.WriteFloat(baseOrigin.y);
	msg.WriteFloat(baseOrigin.z);
	msg.WriteFloat(targetOrigin.x);
	msg.WriteFloat(targetOrigin.y);
	msg.WriteFloat(targetOrigin.z);

	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();
		idCQuat q = state->axis.ToCQuat();
		msg.WriteFloat(q.x);
		msg.WriteFloat(q.y);
		msg.WriteFloat(q.z);
	}

	msg.WriteFloat(proximity);
#else
	WriteBindToSnapshot(msg);
	msg.WriteBits(proxyParent.GetSpawnId(), 32);
#endif
}

//--------------------------------
// hhProxDoorTranslator::ReadFromSnapshot
//--------------------------------
void hhProxDoorTranslator::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	//hhProxDoorSection::ReadFromSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	baseOrigin.x = msg.ReadFloat();
	baseOrigin.y = msg.ReadFloat();
	baseOrigin.z = msg.ReadFloat();
	targetOrigin.x = msg.ReadFloat();
	targetOrigin.y = msg.ReadFloat();
	targetOrigin.z = msg.ReadFloat();

	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();
		idCQuat q;
		q.x = msg.ReadFloat();
		q.y = msg.ReadFloat();
		q.z = msg.ReadFloat();
		state->axis = q.ToMat3();
	}

	float prox = msg.ReadFloat();
	SetProximity(prox);
#else
	ReadBindFromSnapshot(msg);
	proxyParent.SetSpawnId(msg.ReadBits(32));
	if (proxyParent.IsValid() && proxyParent->IsType(hhProxDoor::Type)) {
		hhProxDoor *parentPtr = static_cast<hhProxDoor *>(proxyParent.GetEntity());

		if (parentPtr->hasNetData) {
			if (!hasNetData) {
				idVec3 parentOrigin;
				idMat3 parentAxis;

				parentOrigin = proxyParent->GetOrigin();//proxyParent->spawnArgs.GetVector("origin", "0 0 0", parentOrigin);
				parentAxis = proxyParent->GetAxis();//proxyParent->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1", parentAxis);

				SetOrigin(parentOrigin);
				SetAxis(parentAxis);

				baseOrigin = GetOrigin();

				spawnArgs.SetVector("origin", parentOrigin);
				spawnArgs.SetMatrix("rotation", parentAxis);

				hasNetData = true;
			}
		}
		else {
			hasNetData = false;
		}
	}
#endif
}

//--------------------------------
// hhProxDoorTranslator::SetProximity
//--------------------------------
void hhProxDoorTranslator::SetProximity( float prox ) {
	SetOrigin( baseOrigin + (targetOrigin * prox) );
	proximity = prox;
}


//-------------------------------------------------------------------------------------------------
// hhProxDoorRotator.
//-------------------------------------------------------------------------------------------------
CLASS_DECLARATION( hhProxDoorSection, hhProxDoorRotator )
	EVENT( EV_PostSpawn,			hhProxDoorRotator::Event_PostSpawn )
END_CLASS

//--------------------------------
// hhProxDoorRotator::Spawn
//--------------------------------
void hhProxDoorRotator::Spawn( void ) {
	sectionType = PROXSECTION_Rotator;

	if (gameLocal.isMultiplayer) {
		if (!gameLocal.isClient) {
			PostEventMS( &EV_PostSpawn, 0 );
			fl.networkSync = true;
		}
	}
	else { //just call it now. some sp maps might target a door piece in the spawn function of an entity or something crazy like that.
		Event_PostSpawn();
	}
}

void hhProxDoorRotator::Event_PostSpawn( void ) {
	idVec3	sectionOffset;
	idVec3	rotVector;
	float	rotAngle;

	if( !spawnArgs.GetVector("section_offset", "0 0 0", sectionOffset) ) {
		common->Warning( "No section offset found for '%s'", this->GetEntityDefName() );
	}
	if( !spawnArgs.GetVector("rot_vector", "0 0 0", rotVector) ) {
		common->Warning( "No rotation vector found for '%s'", this->GetEntityDefName() );
	}
	if( !spawnArgs.GetFloat("rot_angle", "0 0 0", rotAngle) ) {
		common->Warning( "No rotation angle found for '%s'", this->GetEntityDefName() );
	}

	idDict args;
	args.SetVector( "origin", GetOrigin() + (sectionOffset * GetAxis()) );
	args.SetMatrix( "rotation", GetAxis() );
	args.SetVector( "rot_vector", rotVector );
	args.SetFloat( "rot_angle", rotAngle );
	bindParent = static_cast<hhProxDoorSection*>( gameLocal.SpawnEntityType(hhProxDoorRotMaster::Type, &args) );
	if( !bindParent.IsValid() ) {
		common->Warning( "Failed to spawn bindParent for '%s'", GetEntityDefName() );
	}
	else {
		bindParent->proxyParent = this;
	}
	Bind( bindParent.GetEntity(), true );
}

//--------------------------------
// hhProxDoorRotator::Save
//--------------------------------
void hhProxDoorRotator::Save(idSaveGame *savefile) const {
	bindParent.Save(savefile);
}

//--------------------------------
// hhProxDoorRotator::Restore
//--------------------------------
void hhProxDoorRotator::Restore( idRestoreGame *savefile ) {
	bindParent.Restore(savefile);
}

//--------------------------------
// hhProxDoorRotator::ClientPredictionThink
//--------------------------------
void hhProxDoorRotator::ClientPredictionThink( void ) {
	hhProxDoorSection::ClientPredictionThink();
}

//--------------------------------
// hhProxDoorRotator::WriteToSnapshot
//--------------------------------
void hhProxDoorRotator::WriteToSnapshot( idBitMsgDelta &msg ) const {
//	hhProxDoorSection::WriteToSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	WriteBindToSnapshot(msg);
	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();

		msg.WriteFloat(state->origin.x);
		msg.WriteFloat(state->origin.y);
		msg.WriteFloat(state->origin.z);
		msg.WriteFloat(state->localOrigin.x);
		msg.WriteFloat(state->localOrigin.y);
		msg.WriteFloat(state->localOrigin.z);
		idCQuat q = state->localAxis.ToCQuat();
		msg.WriteFloat(q.x);
		msg.WriteFloat(q.y);
		msg.WriteFloat(q.z);
	}
#else
	msg.WriteBits(proxyParent.GetSpawnId(), 32);
	WriteBindToSnapshot(msg);
	msg.WriteBits(bindParent.GetSpawnId(), 32);
#endif
	//not needed without prediction
	//msg.WriteBits(bindParent.GetSpawnId(), 32);
}

//--------------------------------
// hhProxDoorRotator::ReadFromSnapshot
//--------------------------------
void hhProxDoorRotator::ReadFromSnapshot( const idBitMsgDelta &msg ) {
//	hhProxDoorSection::ReadFromSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	ReadBindFromSnapshot(msg);
	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();

		state->origin.x = msg.ReadFloat();
		state->origin.y = msg.ReadFloat();
		state->origin.z = msg.ReadFloat();
		state->localOrigin.x = msg.ReadFloat();
		state->localOrigin.y = msg.ReadFloat();
		state->localOrigin.z = msg.ReadFloat();
		idCQuat q;
		q.x = msg.ReadFloat();
		q.y = msg.ReadFloat();
		q.z = msg.ReadFloat();
		state->localAxis = q.ToMat3();
	}
#else
	proxyParent.SetSpawnId(msg.ReadBits(32));
	if (proxyParent.IsValid() && proxyParent->IsType(hhProxDoor::Type)) {
		hhProxDoor *parentPtr = static_cast<hhProxDoor *>(proxyParent.GetEntity());

		if (parentPtr->hasNetData) {
			if (!hasNetData) {
				idVec3 parentOrigin;
				idMat3 parentAxis;

				parentOrigin = proxyParent->GetOrigin();//proxyParent->spawnArgs.GetVector("origin", "0 0 0", parentOrigin);
				parentAxis = proxyParent->GetAxis();//proxyParent->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1", parentAxis);

				SetOrigin(parentOrigin);
				SetAxis(parentAxis);

				spawnArgs.SetVector("origin", parentOrigin);
				spawnArgs.SetMatrix("rotation", parentAxis);

				hasNetData = true;
			}
		}
		else {
			hasNetData = false;
		}
	}
	ReadBindFromSnapshot(msg);
	bindParent.SetSpawnId(msg.ReadBits(32));
#endif
	//not needed without prediction
	/*
	int spawnId = msg.ReadBits(32);
	if (!spawnId) {
		bindParent = NULL;
	}
	else {
		bindParent.SetSpawnId(spawnId);
	}
	*/
}

//--------------------------------
// hhProxDoorRotator::SetProximity
//--------------------------------
void hhProxDoorRotator::SetProximity( float prox ) {
	if( bindParent.IsValid() ) {
		bindParent->SetProximity( prox );
	}
	proximity = prox;
}

//-------------------------------------------------------------------------------------------------
// hhProxDoorRotMaster.
//-------------------------------------------------------------------------------------------------
CLASS_DECLARATION( hhProxDoorSection, hhProxDoorRotMaster )
END_CLASS

//--------------------------------
// hhProxDoorRotMaster::Spawn
//--------------------------------
void hhProxDoorRotMaster::Spawn( void ) {
	idVec3	rot_vector;
	spawnArgs.GetVector("rot_vector", "0 0 1", rot_vector);
	baseAxis = GetAxis();
	rotVector = GetAxis() * rot_vector;

	spawnArgs.GetFloat("rot_angle", "0.0", rotAngle);

	fl.networkSync = true;
}

//--------------------------------
// hhProxDoorRotMaster::Save
//--------------------------------
void hhProxDoorRotMaster::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( rotVector );
	savefile->WriteFloat( rotAngle );
	savefile->WriteMat3( baseAxis );
}

//--------------------------------
// hhProxDoorRotMaster::Restore
//--------------------------------
void hhProxDoorRotMaster::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( rotVector );
	savefile->ReadFloat( rotAngle );
	savefile->ReadMat3( baseAxis );
}

//--------------------------------
// hhProxDoorRotMaster::ClientPredictionThink
//--------------------------------
void hhProxDoorRotMaster::ClientPredictionThink( void ) {
	hhProxDoorSection::ClientPredictionThink();
}

//--------------------------------
// hhProxDoorRotMaster::WriteToSnapshot
//--------------------------------
void hhProxDoorRotMaster::WriteToSnapshot( idBitMsgDelta &msg ) const {
	//hhProxDoorSection::WriteToSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	msg.WriteFloat(rotVector.x);
	msg.WriteFloat(rotVector.y);
	msg.WriteFloat(rotVector.z);

	msg.WriteFloat(rotAngle);

	idCQuat q = baseAxis.ToCQuat();
	msg.WriteFloat(q.x);
	msg.WriteFloat(q.y);
	msg.WriteFloat(q.z);

	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();
		msg.WriteFloat(state->origin.x);
		msg.WriteFloat(state->origin.y);
		msg.WriteFloat(state->origin.z);
	}
	msg.WriteFloat(proximity);
#else
	WriteBindToSnapshot(msg);
	msg.WriteBits(proxyParent.GetSpawnId(), 32);
#endif
}

//--------------------------------
// hhProxDoorRotMaster::ReadFromSnapshot
//--------------------------------
void hhProxDoorRotMaster::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	//hhProxDoorSection::ReadFromSnapshot(msg);
#ifdef _SYNC_PROXDOORS
	rotVector.x = msg.ReadFloat();
	rotVector.y = msg.ReadFloat();
	rotVector.z = msg.ReadFloat();

	rotAngle = msg.ReadFloat();

	idCQuat q;
	q.x = msg.ReadFloat();
	q.y = msg.ReadFloat();
	q.z = msg.ReadFloat();
	baseAxis = q.ToMat3();

	if (GetPhysics()->IsType(idPhysics_Static::Type)) {
		idPhysics_Static *phys = static_cast<idPhysics_Static *>(GetPhysics());
		staticPState_t *state = phys->GetPState();
		state->origin.x = msg.ReadFloat();
		state->origin.y = msg.ReadFloat();
		state->origin.z = msg.ReadFloat();
	}
	float prox = msg.ReadFloat();
	SetProximity(prox);
#else
	ReadBindFromSnapshot(msg);
	proxyParent.SetSpawnId(msg.ReadBits(32));
    if (proxyParent.IsValid() && proxyParent->IsType(hhProxDoorRotator::Type)) {
		hhProxDoorRotator *parentPtr = static_cast<hhProxDoorRotator *>(proxyParent.GetEntity());
		if (parentPtr->hasNetData) {
			if (!hasNetData) {
				idVec3 sectionOffset, rot_vector;
				idVec3 parentOrigin;
				idMat3 parentAxis;

				proxyParent->spawnArgs.GetVector("origin", "0 0 0", parentOrigin);
				proxyParent->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1", parentAxis);

				proxyParent->spawnArgs.GetVector("section_offset", "0 0 0", sectionOffset);
				proxyParent->spawnArgs.GetVector("rot_vector", "0 0 0", rot_vector);
				proxyParent->spawnArgs.GetFloat("rot_angle", "0.0", rotAngle);
				SetOrigin(parentOrigin+(sectionOffset*parentAxis));
				SetAxis(parentAxis);

				baseAxis = GetAxis();
				rotVector = GetAxis() * rot_vector;

				hasNetData = true;
			}
		}
		else {
			parentPtr->hasNetData = false;
		}
	}
#endif
}

//--------------------------------
// hhProxDoorRotMaster::SetProximity
//--------------------------------
void hhProxDoorRotMaster::SetProximity( float prox ) {
	idRotation desRotation( vec3_origin, rotVector, prox*rotAngle );
	SetAxis( baseAxis * desRotation.ToMat3() );

	//rww - proxy door pieces keep getting stuck, so make sure this bastard keeps running physics
	if (!IsActive(TH_PHYSICS)) {
		BecomeActive(TH_PHYSICS);
	}
	proximity = prox;
}
