#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_StartNextMove( "<startnextmove>", NULL );
const idEventDef EV_Kill( "<kill>" );

CLASS_DECLARATION(idMover, hhTrackMover)
	EVENT( EV_PostSpawn,				hhTrackMover::Event_PostSpawn )
	EVENT( EV_StartNextMove,			hhTrackMover::Event_StartNextMove )
	EVENT( EV_Activate,					hhTrackMover::Event_Activate )
	EVENT( EV_Deactivate,				hhTrackMover::Event_Deactivate )
	EVENT( EV_Kill,						hhTrackMover::Event_Kill )
END_CLASS

hhTrackMover::hhTrackMover() {
	currentNode = NULL;
}

void hhTrackMover::Spawn( void ) {
	timeBetweenMoves = spawnArgs.GetInt("moveDelayMS");
	Hide();
	GetPhysics()->SetContents( 0 );
	fl.takedamage = false;
	PostEventMS(&EV_PostSpawn, 0);
}

// Return the next hhTrackNode to move to based on current player position
idEntity *hhTrackMover::GetNextDestination( void ) {

	idEntity *node, *minNode;
	float distToPlayerSquared, minDistToPlayerSquared;

	if ( !currentNode ) {
		return NULL;
	}

	// Get player closest to currentNode
	hhPlayer *player = NULL;
	float bestDistSqr = idMath::INFINITY;
	idVec3 origin = currentNode->GetPhysics()->GetOrigin();
	for ( int i = 0; i < MAX_CLIENTS ; i++ ) {
		idEntity *ent = gameLocal.entities[ i ];
		if (ent && ent->IsType(hhPlayer::Type)) {
			idVec3 toEnt = ent->GetPhysics()->GetOrigin() - origin;
			float distSqr = toEnt.LengthSqr();
			if (distSqr < bestDistSqr) {
				bestDistSqr = distSqr;
				player = static_cast<hhPlayer *>(ent);
			}
		}
	}
	if ( !player ) {
		return NULL;
	}

	// Find closest adjacent node to player
	minNode = currentNode;
	minDistToPlayerSquared = (player->GetPhysics()->GetOrigin() - currentNode->GetPhysics()->GetOrigin()).LengthSqr();
	for (int t=0; t<currentNode->targets.Num(); t++) {
		node = currentNode->targets[t].GetEntity();
		if (node) {
			idVec3 toPlayer = player->GetPhysics()->GetOrigin() - node->GetPhysics()->GetOrigin();
			distToPlayerSquared = toPlayer.LengthSqr();
			if (distToPlayerSquared < minDistToPlayerSquared) {
				minDistToPlayerSquared = distToPlayerSquared;
				minNode = node;

				if(p_mountedGunDebug.GetInteger()) {
					gameRenderWorld->DebugLine(colorGreen, currentNode->GetPhysics()->GetOrigin(), node->GetPhysics()->GetOrigin(), 2000, false);
				}
			}
			else {
				if(p_mountedGunDebug.GetInteger()) {
					gameRenderWorld->DebugLine(colorYellow, currentNode->GetPhysics()->GetOrigin(), node->GetPhysics()->GetOrigin(), 2000, false);
				}
			}
		}
	}

	return minNode;
}

void hhTrackMover::DoneMoving( void ) {
	if( state == StateGlobal ) {
		idMover::DoneMoving();
		PostEventMS( &EV_StartNextMove, 0);
	}
}

void hhTrackMover::Event_PostSpawn() {
	bool activate;
	spawnArgs.GetBool("autoactivate", "0", activate);

	if( !targets.Num() || !targets[0].IsValid() ) {
		gameLocal.Error("hhTrackMover %s doesn't have a target tracknode\n", name.c_str());
	}

	currentNode = targets[0].GetEntity();
	dest_position = GetLocalCoordinates(currentNode->GetPhysics()->GetOrigin());
	SetOrigin( dest_position );

	if (activate) {
		state = StateGlobal;
		PostEventMS( &EV_StartNextMove, 1000 );
	} else {
		state = StateIdle;
	}
}

void hhTrackMover::Event_Activate( idEntity *activator ) {
	if( state == StateIdle ) {
		PostEventMS( &EV_StartNextMove, 0 );
		state = StateGlobal;
	}
}

void hhTrackMover::Event_Deactivate() {
	if( state == StateGlobal ) {
		state = StateIdle;

		DoneMoving();
		DoneRotating();
		CancelEvents(&EV_StartNextMove);

	}
}

void hhTrackMover::Event_StartNextMove() {
	idAngles Angles;
	idVec3 Origin;
	idEntity *ent = GetNextDestination();

	if (ent && ent != currentNode) {

		Origin = GetLocalCoordinates( ent->GetPhysics()->GetOrigin() );
		if( !GetPhysics()->GetOrigin().Compare(Origin) ) {
			dest_position = Origin;
			BeginMove( NULL );
		}

		Angles = ent->GetPhysics()->GetAxis().ToAngles();
		if( !GetPhysics()->GetAxis().ToAngles().Compare(Angles) ) {
			dest_angles = Angles;
			BeginRotation( NULL, true );
		}

		currentNode = ent;
	} else {
		PostEventMS( &EV_StartNextMove, timeBetweenMoves );
	}
}

void hhTrackMover::Event_Kill() {
	idMover::DoneMoving();
	idMover::DoneRotating();
	CancelEvents(&EV_StartNextMove);
	state = StateDead;
}

void hhTrackMover::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( state );
	savefile->WriteInt( timeBetweenMoves );
	savefile->WriteObject( currentNode );
}

void hhTrackMover::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( reinterpret_cast<int &> ( state ) );
	savefile->ReadInt( timeBetweenMoves );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( currentNode ) );
}
