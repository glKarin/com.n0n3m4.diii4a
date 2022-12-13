#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// Events
const idEventDef EV_ModelDoorSpawnTrigger( "<strig>" );
const idEventDef EV_ModelDoorOpen( "open" );
const idEventDef EV_ModelDoorClose( "close" );

const idEventDef EV_ModelDoorClosedBegin( "<mdclosedbegin>" );
const idEventDef EV_ModelDoorOpeningBegin( "<mdopeningbegin>" );
const idEventDef EV_ModelDoorOpenBegin( "<mdopenbegin>" );
const idEventDef EV_ModelDoorClosingBegin( "<mdclosingbegin>" );

const idEventDef EV_SetBuddiesShaderParm( "setBuddiesShaderParm", "df" );

//--------------------------------
// hhDoorTrigger
//--------------------------------
CLASS_DECLARATION( idEntity, hhDoorTrigger )
	EVENT( EV_Touch,	hhDoorTrigger::Event_TriggerDoor )
END_CLASS

//--------------------------------
// hhDoorTrigger::hhDoorTrigger()
//--------------------------------
hhDoorTrigger::hhDoorTrigger() {
	enabled = true;  	// Need to have the ability to disable a trigger
}

//--------------------------------
// hhDoorTrigger::Event_TriggerDoor
//--------------------------------
void hhDoorTrigger::Event_TriggerDoor( idEntity *other, trace_t *trace ) {
	if( door && IsEnabled() ) {
		door->ProcessEvent( &EV_Touch, other, trace );
	}
}

//---------------------
// hhDoorTrigger::GetEntitiesWithin
// ents should be an array of idEntity of length entLength
//
// If not enabled, always returns 0
//---------------------
int hhDoorTrigger::GetEntitiesWithin( idEntity **ents, int entsLength ) {
	int num;

	if ( !enabled ) {	
		return( 0 );
	}

	num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_SHOT_BOUNDINGBOX, ents, entsLength );

	return( num );

}

void hhDoorTrigger::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( door );
	savefile->WriteBool( enabled );
}

void hhDoorTrigger::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&> ( door ) );
	savefile->ReadBool( enabled );
}

//--------------------------------
// hhModelDoor
//--------------------------------
CLASS_DECLARATION( hhAnimatedEntity, hhModelDoor )
	EVENT( EV_TeamBlocked,						hhModelDoor::Event_TeamBlocked )
	EVENT( EV_PartBlocked,						hhModelDoor::Event_PartBlocked )
	EVENT( EV_Activate,							hhModelDoor::Event_Activate )
	EVENT( EV_Touch,							hhModelDoor::Event_Touch )
	EVENT( EV_ModelDoorOpen,					hhModelDoor::Event_OpenDoor )
	EVENT( EV_ModelDoorClose,					hhModelDoor::Event_CloseDoor )
	EVENT( EV_Thread_SetCallback,				hhModelDoor::Event_SetCallback )

	EVENT( EV_SetBuddiesShaderParm,				hhModelDoor::Event_SetBuddiesShaderParm )

	// Internal events
	EVENT( EV_ModelDoorSpawnTrigger,			hhModelDoor::Event_SpawnNewDoorTrigger )
	EVENT( EV_ModelDoorClosedBegin,				hhModelDoor::Event_STATE_ClosedBegin )
	EVENT( EV_ModelDoorOpeningBegin,			hhModelDoor::Event_STATE_OpeningBegin )
	EVENT( EV_ModelDoorOpenBegin,				hhModelDoor::Event_STATE_OpenBegin )
	EVENT( EV_ModelDoorClosingBegin,			hhModelDoor::Event_STATE_ClosingBegin )
END_CLASS

//--------------------------------
// hhModelDoor::Spawn
//--------------------------------
void hhModelDoor::Spawn( void ) {
	idEntity*	master = NULL;

	doorTrigger = NULL;
	fl.takedamage = true;
	SetBlocking(true);

//	spawnArgs.GetFloat( "dmg", "2", &damage );
	spawnArgs.GetFloat( "triggersize", "50", triggersize );
	spawnArgs.GetBool( "no_touch", "0", noTouch );
	spawnArgs.GetInt( "locked", "0", locked );
	spawnArgs.GetFloat( "wait", "1.5", wait );
	if ( wait <= 0.1f ) {
		//sanity check for waittime
		wait = 0.1f;
	}
	spawnArgs.GetFloat( "damage", "1", damage );
	spawnArgs.GetFloat( "airlockwait", "3.0", airLockSndWait );
	airLockSndWait *= 1000.0f; // Convert from seconds to ms

	hhUtils::GetValues( spawnArgs, "buddy", buddyNames, true );
	
	// If "health" is supplied, door will open when killed
	//  So this key determines if we should really take damage or not
	fl.takedamage = (health > 0);
	forcedOpen = false;
	bOpenForMonsters = spawnArgs.GetBool("OpenForMonsters", "1");

	SetShaderParm( SHADERPARM_MODE, GetDoorShaderParm( locked != 0, true ) );	// 2=locked, 1=unlocked, 0=never locked

	//HUMANHEAD: aob - airlock stuff
	airlockMaster = NULL;

	airlockTeam.SetOwner( this );
	airlockTeamName = spawnArgs.GetString( "airlockTeam" );
	master = DetermineTeamMaster( GetAirLockTeamName() );
	if( master ) {
		airlockMaster = static_cast<hhModelDoor*>( master );
		if( airlockMaster != this ) {
			JoinAirLockTeam( airlockMaster );
		}
	}
	//HUMANHEAD END

	openAnim = GetAnimator()->GetAnim( "open" );
	closeAnim = GetAnimator()->GetAnim( "close" );
	idleAnim = GetAnimator()->GetAnim( "idle" );
	painAnim = GetAnimator()->GetAnim( "pain" );

	// Spawn the trigger
	if (!gameLocal.isClient) {
		PostEventMS( &EV_ModelDoorSpawnTrigger, 0 );
	}

	// see if we are on an areaportal
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );

	StartSound( "snd_idle", SND_CHANNEL_ANY );

	if( spawnArgs.GetBool("start_open") ) {
		StartOpen();
	} else {
		StartClosed();
	}

	threadNum = 0;
	nextAirLockSnd = 0;
	
	finishedSpawn = true;

	fl.networkSync = true;

	bShuttleDoors = spawnArgs.GetBool( "shuttle_doors" );
}

//--------------------------------
// hhModelDoor::hhModelDoor
//--------------------------------
hhModelDoor::hhModelDoor() {
	bOpen = false;
	bTransition = false;
	finishedSpawn = false;
	sndTrigger = NULL;
	nextSndTriggerTime = 0;
}

//--------------------------------
// hhModelDoor::~hhModelDoor
//--------------------------------
hhModelDoor::~hhModelDoor() {
	airlockTeam.Remove();
	if ( sndTrigger ) {
		delete sndTrigger;
		sndTrigger = NULL;
	}
}

void hhModelDoor::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( damage );
	savefile->WriteFloat( wait );
	savefile->WriteFloat( triggersize );
	savefile->WriteInt( areaPortal );
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}
	savefile->WriteStringList( buddyNames );
	savefile->WriteInt( locked );
	savefile->WriteInt( openAnim );
	savefile->WriteInt( closeAnim );
	savefile->WriteInt( idleAnim );
	savefile->WriteInt( painAnim );
	savefile->WriteBool( forcedOpen );
	savefile->WriteBool( bOpenForMonsters );
	savefile->WriteBool( noTouch );
	savefile->WriteInt( threadNum );
	savefile->WriteObject( doorTrigger );
	savefile->WriteString( airlockTeamName );
	savefile->WriteObject( airlockMaster );
	savefile->WriteBounds( crusherBounds );

	activatedBy.Save( savefile );

	savefile->WriteBool( bOpen );
	savefile->WriteBool( bTransition );
	savefile->WriteFloat( airLockSndWait );

	savefile->WriteClipModel( sndTrigger );
	savefile->WriteInt( nextSndTriggerTime );
}

void hhModelDoor::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( damage );
	savefile->ReadFloat( wait );
	savefile->ReadFloat( triggersize );
	savefile->ReadInt( areaPortal );
	if ( areaPortal ) {
		int portalState;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}
	savefile->ReadStringList( buddyNames );
	savefile->ReadInt( locked );
	savefile->ReadInt( openAnim );
	savefile->ReadInt( closeAnim );
	savefile->ReadInt( idleAnim );
	savefile->ReadInt( painAnim );
	savefile->ReadBool( forcedOpen );
	savefile->ReadBool( bOpenForMonsters );
	savefile->ReadBool( noTouch );
	savefile->ReadInt( threadNum );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( doorTrigger ) );
	savefile->ReadString( airlockTeamName );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( airlockMaster ) );
	savefile->ReadBounds( crusherBounds );

	activatedBy.Restore( savefile );

	savefile->ReadBool( bOpen );
	savefile->ReadBool( bTransition );
	savefile->ReadFloat( airLockSndWait );

	airlockTeam.SetOwner( this );
	if( airlockMaster && airlockMaster != this ) {
		JoinAirLockTeam( airlockMaster );
	}

	nextAirLockSnd = 0;
	finishedSpawn = true;
	bShuttleDoors = spawnArgs.GetBool( "shuttle_doors" );

	savefile->ReadClipModel( sndTrigger );
	savefile->ReadInt( nextSndTriggerTime );
}

//--------------------------------
// hhModelDoor::Lock
//--------------------------------
void hhModelDoor::Lock( int f ) {
	locked = f;
	float parmValue = GetDoorShaderParm( locked != 0, false );
	SetShaderParm( SHADERPARM_MODE, parmValue );
	SetBuddiesShaderParm( SHADERPARM_MODE, parmValue );

	if (locked) {
		CloseDoor();
	}
}

//--------------------------------
// hhModelDoor::CanOpen
//--------------------------------
bool hhModelDoor::CanOpen() const {
	if( IsLocked() ) {
		return false;
	}

	if( !GetAirLockMaster() ) {
		return true;
	}

	for( hhModelDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node != this && node->IsOpen() ) {
			return false;
		}
	}

	return true;
}

//--------------------------------
// hhModelDoor::CanClose
//--------------------------------
bool hhModelDoor::CanClose() const {
	return true;
}

//--------------------------------
// hhModelDoor::OpenDoor
//--------------------------------
void hhModelDoor::OpenDoor() {
	if( IsClosed() && CanOpen() ) {
		bTransition = true;
		PostEventMS( &EV_ModelDoorOpeningBegin, 0 );
	}
}

//--------------------------------
// hhModelDoor::CloseDoor
//--------------------------------
void hhModelDoor::CloseDoor() {
	if( bOpen && !bTransition && CanClose() && !forcedOpen ) {
		bTransition = true;
		PostEventMS( &EV_ModelDoorClosingBegin, 0 );
		
		// CJR:  It's possible to open a door, then get back into it while it's animating closed, and then get squished
		// the fix for this is to block the player from getting back in while it's closing.  Projectiles will still pass through
		GetPhysics()->SetContents( CONTENTS_PLAYERCLIP );
	}
}

//--------------------------------
// hhModelDoor::SetBlocking
//--------------------------------
void hhModelDoor::SetBlocking( bool on ) {
	GetPhysics()->SetContents( on ? CONTENTS_SOLID : 0 );
}

//--------------------------------
// hhModelDoor::ClosePortal
//--------------------------------
void hhModelDoor::ClosePortal( void ) {
	if ( areaPortal ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_VIEW );
	}
}

//--------------------------------
// hhModelDoor::OpenPortal
//--------------------------------
void hhModelDoor::OpenPortal( void ) {
	if ( areaPortal ) {
		gameLocal.SetPortalState( areaPortal, PS_BLOCK_NONE );
	}
}


//--------------------------------
// hhModelDoor::InformDone
//--------------------------------
void hhModelDoor::InformDone() {
	idThread::ObjectMoveDone( threadNum, this );
	threadNum = 0;
}

//--------------------------------
// hhModelDoor::Damage
//--------------------------------
void hhModelDoor::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if (fl.takedamage) {
		if ( !bOpen && painAnim ) { 
			GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, painAnim, gameLocal.time, 500);
		}

		hhAnimatedEntity::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);
	}
}

//--------------------------------
// hhModelDoor::Killed
//--------------------------------
void hhModelDoor::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if (IsLocked()) {
		Lock(0);
	}
	OpenDoor();
	fl.takedamage = false;
	forcedOpen = true;	
}

//--------------------------------
// hhModelDoor::SetBuddiesShaderParm
//--------------------------------
void hhModelDoor::SetBuddiesShaderParm( int parm, float value ) {
	idEntity* buddy = NULL;

	for( int ix = buddyNames.Num() - 1; ix >= 0; --ix ) {
		if( !buddyNames[ix].Length() ) {
			continue;
		}

		buddy = gameLocal.FindEntity( buddyNames[ix].c_str() );
		if( !buddy ) {
			continue;
		}

		buddy->SetShaderParm( parm, value );
	}
}

//--------------------------------
// hhModelDoor::ToggleBuddiesShaderParm
//--------------------------------
void hhModelDoor::ToggleBuddiesShaderParm( int parm, float firstValue, float secondValue, float toggleDelay ) {
	SetBuddiesShaderParm( parm, firstValue );

	CancelEvents( &EV_SetBuddiesShaderParm );
	PostEventSec( &EV_SetBuddiesShaderParm, toggleDelay, parm, secondValue );
}

//--------------------------------
// hhModelDoor::DetermineTeamMaster
//--------------------------------
idEntity* hhModelDoor::DetermineTeamMaster( const char* teamName ) {
	idEntity* ent = NULL;
	 
	if ( teamName && teamName[0] ) {
		// find the first entity spawned on this team (which could be us)
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if (ent->IsType(hhModelDoor::Type) && !idStr::Icmp( static_cast<hhModelDoor *>(ent)->GetAirLockTeamName(), teamName )) {
				return ent;
			}
			if (ent->IsType(hhDoor::Type) && !idStr::Icmp( static_cast<hhDoor *>(ent)->GetAirLockTeamName(), teamName )) {
				return ent;
			}
		}
	}

	return NULL;
}

//--------------------------------
// hhModelDoor::JoinAirLockTeam
//--------------------------------
void hhModelDoor::JoinAirLockTeam( hhModelDoor *master ) {
	assert( master );

	airlockTeam.AddToEnd( master->airlockTeam );
}

//--------------------------------
// hhModelDoor::VerifyAirlockTeamStatus
//--------------------------------
void hhModelDoor::VerifyAirlockTeamStatus() {
	if( !GetAirLockMaster() ) {
		return;
	}

	for( hhModelDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node != this && !node->IsClosed() ) {
			gameLocal.Warning( "Airlock team '%s' has more than one member starting open", GetAirLockTeamName() );
		}
	}
}

//--------------------------------
// hhModelDoor::StartOpen
//--------------------------------
void hhModelDoor::StartOpen() {
	VerifyAirlockTeamStatus();

	bTransition = false;
	bOpen = true;
	OpenPortal();
	SetBlocking(false);
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, openAnim, gameLocal.time, 500);
	Event_STATE_OpenBegin();
}

//--------------------------------
// hhModelDoor::StartClosed
//--------------------------------
void hhModelDoor::StartClosed() {
	bTransition = false;
	bOpen = false;
	Event_STATE_ClosedBegin();
}

//--------------------------------
// hhModelDoor::Event_SpawnNewDoorTrigger
//
// All of the parts of a door have been spawned, so create
// a trigger that encloses all of them
//--------------------------------
void hhModelDoor::Event_SpawnNewDoorTrigger( void ) {
	idBounds		bounds,localbounds;
	idBounds		triggerBounds, soundTriggerBounds;
	int				i;
	int				best;

	// Since models bounds are overestimated, we need to use the bounds from the
	// clipmodel, which was set before the over-estimation
	localbounds = GetPhysics()->GetBounds();

	// Save the original bounds in case we need to crush something
	crusherBounds = GetPhysics()->GetAbsBounds();
	
	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( localbounds[1][ i ] - localbounds[0][ i ] < localbounds[1][ best ] - localbounds[0][ best ] ) {
			best = i;
		}
	}
	triggerBounds = localbounds;
	triggerBounds[1][ best ] += triggersize;
	triggerBounds[0][ best ] -= triggersize;

	// Now transform into absolute coordintates
	if ( GetPhysics()->GetAxis().IsRotated() ) {
		bounds.FromTransformedBounds( triggerBounds, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}
	else {
		bounds[0] = triggerBounds[0] + GetPhysics()->GetOrigin();
		bounds[1] = triggerBounds[1] + GetPhysics()->GetOrigin();
	}

	// create a trigger with this size
	idDict args;
	args.Set( "mins", bounds[0].ToString(0) );
	args.Set( "maxs", bounds[1].ToString(0) );

	doorTrigger = ( hhDoorTrigger * ) gameLocal.SpawnEntityType( hhDoorTrigger::Type, &args );
	doorTrigger->GetPhysics()->SetContents( CONTENTS_TRIGGER );
	doorTrigger->door = this;

	// Disable the trigger if it is no_touch and not start_open
	if ( noTouch ) {
		doorTrigger->Disable();
	}


	// ------------------------
	// Create the sound trigger
	// ------------------------

	float soundTriggerSize = triggersize * 0.3f;
	soundTriggerBounds = localbounds;
	soundTriggerBounds[1][ best ] += soundTriggerSize;
	soundTriggerBounds[0][ best ] -= soundTriggerSize;

	// Now transform into absolute coordintates
	if ( GetPhysics()->GetAxis().IsRotated() ) {
		bounds.FromTransformedBounds( soundTriggerBounds, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}
	else {
		bounds[0] = soundTriggerBounds[0] + GetPhysics()->GetOrigin();
		bounds[1] = soundTriggerBounds[1] + GetPhysics()->GetOrigin();
	}
	bounds[0] -= GetPhysics()->GetOrigin();
	bounds[1] -= GetPhysics()->GetOrigin();

	// create a trigger clip model
	sndTrigger = new idClipModel( idTraceModel( bounds ) );
	sndTrigger->Link( gameLocal.clip, this, 254, GetPhysics()->GetOrigin(), mat3_identity );
	sndTrigger->SetContents( CONTENTS_TRIGGER );

	// HACK: Also, since all the parts are now spawned, update the buddies' shaderparms
	if (locked) {
		float parmValue = GetDoorShaderParm( locked != 0, true );
		for ( i = 0; i < buddyNames.Num(); i++ ) {
			if ( buddyNames[ i ].Length() ) {
				idEntity *buddy = gameLocal.FindEntity( buddyNames[ i ] );
				if( buddy ) {
					buddy->SetShaderParm(SHADERPARM_MODE, parmValue);
				}
			}
		}
	}

}

//--------------------------------
// hhModelDoor::ToggleDoorState
//--------------------------------
void hhModelDoor::ToggleDoorState( void ) {
	if ( bOpen ) {
		CloseDoor();
	}
	else { 
		OpenDoor();
	}
}

//--------------------------------
// hhModelDoor::ForceAirLockTeamClosed
//--------------------------------
void hhModelDoor::ForceAirLockTeamClosed() {
	if( !GetAirLockMaster() ) {
		return;
	}

	ToggleBuddiesShaderParm( SHADERPARM_DIVERSITY, 1.0f, 0.0f, 2.0f );

	for( hhModelDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node != this && !node->IsClosed() ) {
			node->CloseDoor();
		}
	}
}

//--------------------------------
// hhModelDoor::ForceAirLockTeamOpen
//--------------------------------
void hhModelDoor::ForceAirLockTeamOpen() {
	if( !GetAirLockMaster() ) {
		return;
	}

	for( hhModelDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node != this && !node->IsOpen() ) {
			node->OpenDoor();
		}
	}
}

// State code
void hhModelDoor::Event_STATE_ClosedBegin() {

	// Alert any entities squished in the door
	idEntity		*touch[ MAX_GENTITIES ];
	int contentsMask = (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE|CONTENTS_TRIGGER);	// corpses, moveableitems, spiritplayers, moveables
	int num = hhUtils::EntitiesTouchingClipmodel(GetPhysics()->GetClipModel(), touch, MAX_GENTITIES, contentsMask);
	for (int ix=0; ix<num; ix++) {
		if (touch[ix] != this && touch[ix] != doorTrigger) {
			idEntity *ent = touch[ix];

			if ( doorTrigger && damage > 0.0f ) {
				// HUMANHEAD mdl
				// Throw spirit players back to their bodies if they get caught in a closing door
				// Don't check this for MP, the damage below is enough to send them back
				if ( !gameLocal.isMultiplayer && ent->IsType( hhPlayer::Type ) ) {
					hhPlayer *player = reinterpret_cast< hhPlayer * > ( ent );
					if ( player->IsSpiritWalking() ) {
						player->StopSpiritWalk();
						continue;
					}
				}

				if( ent->fl.takedamage ) {
					ent->Damage( this, this, vec3_origin, "damage_doorbonk", damage, INVALID_JOINT );
				}
			}

			ent->SquishedByDoor(this);
		}
	}

	SetBlocking(true);
	ClosePortal();
	InformDone();

	if ( idleAnim ) {
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.time, 500 );
	}

	// If we are noTouch, then disable the trigger.  (Could have been enabled by GUI activation)
	if ( noTouch && doorTrigger ) {
		doorTrigger->Disable();
	}

	// Trigger entities if we have finished spawning
	if ( !finishedSpawn ) {
		return;
	}
	
	ActivatePrefixed("triggerClosed", GetActivator());

	if( GetAirLockMaster() ) {
		// Mark the other team members as locked
		for( hhModelDoor* node = airlockMaster->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {

			// Skip this door and it's buddies
			if( node == this || buddyNames.Find( node->name ) ) {
				continue;
			}

			// Don't change doors that are really locked
			if( node->IsLocked() ) {
				continue;
			}

			// Mark as locked
			node->SetBuddiesShaderParm( SHADERPARM_MISC, 0.0f );
		}
	}

	HH_ASSERT( bOpen == true && bTransition == true );
	bTransition = false;
	bOpen = false;
}

void hhModelDoor::Event_STATE_OpeningBegin() {
	SetBlocking(false);
	OpenPortal();
	StartSound( "snd_open", SND_CHANNEL_ANY );

	// Fire any triggerStartOpen entities
	ActivatePrefixed("triggerStartOpen", GetActivator());

	idEntity *ent;
	idEntity *next;
	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->GetTeamChain();
		if ( ent && ent->IsType( hhProjectile::Type ) ) {
			ent->Unbind();			// bjk drops all bound projectiles such as arrows and mines
			next = teamChain;
			if (ent->IsType(hhProjectileSpiritArrow::Type)) {
				ent->PostEventMS(&EV_Remove, 0);
			}
			//HUMANHEAD bjk PCF (4-28-06) - explode crawlers
			if (ent->IsType(hhProjectileStickyCrawlerGrenade::Type)) {
				static_cast<hhProjectile*>(ent)->PostEventSec( &EV_Explode, 0.2f );
			}
		}
	}

	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, openAnim, gameLocal.time, 500);
	int opentime = (openAnim) ? GetAnimator()->GetAnim( openAnim )->Length() : 0;
	PostEventMS( &EV_ModelDoorOpenBegin, opentime );

	if( airlockMaster ) {
		// Mark the other team members as locked
		for( hhModelDoor* node = airlockMaster->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {

			// Skip this door and it's buddies
			if( node == this || buddyNames.Find( node->name ) ) {
				continue;
			}

			// Don't change doors that are really locked
			if( node->IsLocked() ) {
				continue;
			}

			// Mark as locked
			node->SetBuddiesShaderParm( SHADERPARM_MISC, 1.0f );
		}

		// This prevents the our shader from being set as locked from the player moving too quickly between airlock doors
		SetBuddiesShaderParm( SHADERPARM_MISC, 0.0f );
	}

	HH_ASSERT( bOpen == false && bTransition == true );
}

void hhModelDoor::Event_STATE_OpenBegin() {
	InformDone();
	
	// If we are no touch, then we were opened by something else, so let us care about the player being in the trigger/enable the trigger
	if ( noTouch && doorTrigger ) {
		doorTrigger->Enable();
	}

	// Trigger entities if we have finished spawning
	if ( !finishedSpawn ) {
		return;
	}

	ActivatePrefixed("triggerOpened", GetActivator());

	WakeTouchingEntities();

	HH_ASSERT( bOpen == false && bTransition == true );
	bOpen = true;
	bTransition = false;
}

void hhModelDoor::Event_STATE_ClosingBegin() {
	StartSound("snd_close", SND_CHANNEL_ANY);

	//GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	//FIXME: Despite the blend in/out here, it still pops when the idle is reapplied
	GetAnimator()->PlayAnim(ANIMCHANNEL_ALL, closeAnim, gameLocal.time, 500);
	int closetime = (closeAnim) ? GetAnimator()->GetAnim( closeAnim )->Length() : 0;
	PostEventMS( &EV_ModelDoorClosedBegin, closetime );

	HH_ASSERT( bOpen == true && bTransition == true );
}

// -------------------------------------------------------------
// Non-state code
// -------------------------------------------------------------

//--------------------------------
// hhModelDoor::Event_Blocked
//--------------------------------
void hhModelDoor::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	// reverse direction
	ToggleDoorState();
}

//--------------------------------
// hhModelDoor::Event_PartBlocked
//--------------------------------
void hhModelDoor::Event_PartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_doorbonk", 1.0f, INVALID_JOINT );
	}
}

//--------------------------------
// hhModelDoor::Event_Activate
//--------------------------------
void hhModelDoor::Event_Activate( idEntity *activator ) {
	idEntity *buddy = NULL;
		 
	if ( IsLocked() ) {		
		int oldLocked = locked;
		Lock(0);
		if ( oldLocked == 2 ) {
			return;
		}

		ToggleDoorState();
		CancelEvents( &EV_ModelDoorClose );
		PostEventMS( &EV_ModelDoorClose, wait * 1000 );
	}
	else {
		TryOpen( activator );
	}
	
}	


//--------------------------------
// hhModelDoor::WakeTouchingEntities
//--------------------------------
void hhModelDoor::WakeTouchingEntities() {
	idEntity		*touch[ MAX_GENTITIES ];
	idEntity		*ent;
	int				num;

	num = gameLocal.clip.EntitiesTouchingBounds( GetPhysics()->GetAbsBounds(), MASK_ALL, touch, MAX_GENTITIES );
	for ( int i = 0; i < num; i++ ) {
		ent = touch[ i ];
		if ( !ent || ent == this ) {
			continue;
		}
		ent->ActivatePhysics(this);
	}
}

//--------------------------------
// hhModelDoor::TryOpen
//  If conditions are right, open.  Otherwise don't open
//--------------------------------
void hhModelDoor::TryOpen( idEntity *whoTrying ) {

	activatedBy	= whoTrying;	// Should thie be above OpenDoor

	// Only allow the player to affect airlock doors
	if ( GetAirLockMaster() && !whoTrying->IsType( idPlayer::Type ) ) {
		return;
	}

	// Check if this actor is valid to open this door (if the player is spiritwalking) - cjr
	if ( !whoTrying->ShouldTouchTrigger( this ) ) {
		return;
	}

	// If we're shuttle doors, make sure we only open for actors in vehicles.
	if ( bShuttleDoors && ( ! whoTrying->IsType( idActor::Type ) || ! reinterpret_cast<idActor *> ( whoTrying )->InVehicle() ) ) {
		return;
	}

	if ( !bOpenForMonsters && whoTrying->IsType( hhMonsterAI::Type ) ) {
		return;
	}

	// Delay Close event since we're still in the doorway
	CancelEvents( &EV_ModelDoorClose );
	PostEventMS( &EV_ModelDoorClose, wait * 1000 );

	//This feels like ahack but I need to get this done
	if( IsClosed() && !CanOpen() && GetAirLockMaster() ) {
		ForceAirLockTeamClosed();
		if( gameLocal.time > nextAirLockSnd ) {
			int length;
			StartSound( "snd_airlock", SND_CHANNEL_BODY, 0, false, &length );
			nextAirLockSnd = gameLocal.time + length + airLockSndWait;
		}
	}
	OpenDoor();
}


//--------------------------------
// hhModelDoor::EntitiesInTrigger
//--------------------------------
bool hhModelDoor::EntitiesInTrigger() {
	idEntity *	ents[ MAX_GENTITIES ];
	idEntity *  ent;
	int 		num;

	if ( !doorTrigger ) {
		return( false );
	}
	
	num = doorTrigger->GetEntitiesWithin( ents, MAX_GENTITIES );
	for ( int i = 0; i < num; ++i ) {
		ent = ents[ i ];

		if ( ent->fl.touchTriggers && ent->ShouldTouchTrigger( this ) ) {

			if ( !bOpenForMonsters && ent->IsType( hhMonsterAI::Type ) ) {
				continue;
			}

			if ( bShuttleDoors && ( ! ent->IsType( idActor::Type ) || ! reinterpret_cast<idActor *> ( ent )->InVehicle() ) ) {
				continue;
			}

			if ( !GetAirLockMaster() || ent->IsType( idPlayer::Type ) ) { // Only allow the player to affect airlock doors
				return( true );
			}
		}
	}
	
	return( false );	
}


//--------------------------------
// hhModelDoor::Event_Touch
//--------------------------------
void hhModelDoor::Event_Touch( idEntity *other, trace_t* trace ) {

	if ( sndTrigger && trace->c.id == sndTrigger->GetId() ) {
		if (other && other->IsType(hhPlayer::Type) && IsLocked() && gameLocal.time > nextSndTriggerTime) {
			StartSound("snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
			nextSndTriggerTime = gameLocal.time + 10000;
		}
		return;
	}

	// Skip if locked, or noTouch
	if( IsLocked() || noTouch ) {
		// play locked sound
		return;
	}

	TryOpen( other );
}


/*
================
idModelDoor::GetActivator
================
*/
idEntity *hhModelDoor::GetActivator( void ) const {
	return activatedBy.GetEntity();
}

/*
================
hhModelDoor::ClientPredictionThink
================
*/
void hhModelDoor::ClientPredictionThink( void ) {
	hhAnimatedEntity::ClientPredictionThink();
}

/*
================
hhModelDoor::WriteToSnapshot
================
*/
void hhModelDoor::WriteToSnapshot( idBitMsgDelta &msg ) const {
	hhAnimatedEntity::WriteToSnapshot(msg);
	hhAnimator *animator = (hhAnimator *)GetAnimator();
	const idAnimBlend *anim = animator->CurrentAnim(ANIMCHANNEL_ALL);
	if (anim) {
		msg.WriteBits(1, 1);
		msg.WriteBits(anim->AnimNum(), 32);
	}
	else {
		msg.WriteBits(0, 1);
	}

	msg.WriteBits(GetPhysics()->GetContents(), 32);
}

/*
================
hhModelDoor::ReadFromSnapshot
================
*/
void hhModelDoor::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhAnimatedEntity::ReadFromSnapshot(msg);

	bool hasAnim = !!msg.ReadBits(1);
	if (hasAnim) {
		int animNum = msg.ReadBits(32);
		hhAnimator *animator = (hhAnimator *)GetAnimator();
		const idAnimBlend *anim = animator->CurrentAnim(ANIMCHANNEL_ALL);
		if (!anim || anim->AnimNum() != animNum) {
			animator->PlayAnim(ANIMCHANNEL_ALL, animNum, gameLocal.time, 500);
		}
	}

	int contents = msg.ReadBits(32);
	if (contents != GetPhysics()->GetContents()) {
		GetPhysics()->SetContents(contents);
	}
}


//--------------------------------
// hhModelDoor::Event_ToggleDoorState
//--------------------------------
void hhModelDoor::Event_ToggleDoorState( void ) {
	ToggleDoorState();
}

//--------------------------------
// hhModelDoor::Event_OpenDoor
//--------------------------------
void hhModelDoor::Event_OpenDoor() {
	OpenDoor();
}

//--------------------------------
// hhModelDoor::Event_CloseDoor
//--------------------------------
void hhModelDoor::Event_CloseDoor() {

	// Added due to the fact that monsters which don't move, also don't ActivateTriggers.  
	// So now we check before we close if we can actually close
	
	// We can't actually close if are transitioning or if we have someone in our trigger that we care about
	if ( bTransition || EntitiesInTrigger() ) {
		// So if someone here, cancel any further events
		CancelEvents( &EV_ModelDoorClose );
		// And try again in a bit.
		PostEventMS( &EV_ModelDoorClose, wait * 1000 );
		return;	
	}

	CloseDoor();
}

//--------------------------------
// hhModelDoor::Event_SetCallback
//--------------------------------
void hhModelDoor::Event_SetCallback( void ) {
	if ( !threadNum && bTransition ) {
		threadNum = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

//--------------------------------
// hhModelDoor::Event_SetBuddiesShaderParm
//--------------------------------
void hhModelDoor::Event_SetBuddiesShaderParm( int parm, float value ) {
	SetBuddiesShaderParm( parm, value );
}
