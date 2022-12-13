#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idDoor, hhDoor )
	EVENT( EV_Mover_ReturnToPos1,		hhDoor::Event_ReturnToPos1 )
	EVENT( EV_ReachedPos,				hhDoor::Event_Reached_BinaryMover )
	EVENT( EV_SetBuddiesShaderParm,		hhDoor::Event_SetBuddiesShaderParm )
	EVENT( EV_Touch,					hhDoor::Event_Touch )
END_CLASS

//--------------------------------
// hhDoor::Spawn
//--------------------------------
void hhDoor::Spawn() {
	airlockMaster = NULL;

	airlockTeam.SetOwner( this );
	airlockTeamName = spawnArgs.GetString( "airlockTeam" );
	spawnArgs.GetFloat( "airlockwait", "3", airLockSndWait );
	airLockSndWait *= 1000.0f; // Convert from seconds to ms

	idEntity* master = DetermineTeamMaster( GetAirLockTeamName() );
	if( master ) {
		airlockMaster = static_cast<hhDoor*>( master );
		if( airlockMaster != this ) {
			JoinAirLockTeam( airlockMaster );
		}
	}

	if( spawnArgs.GetBool("start_open") ) {
		VerifyAirlockTeamStatus();
	}

	if( moveMaster != this ) {
		CopyTeamInfoToMoveMaster( static_cast<hhDoor*>(moveMaster) );
	}
	
	// We only open when dead if we have health to start with
	openWhenDead = health > 0;
	forcedOpen = false;	
	nextAirLockSnd = 0;

	bShuttleDoors = spawnArgs.GetBool( "shuttle_doors" );
}

void hhDoor::Save(idSaveGame *savefile) const {
	savefile->WriteString(airlockTeamName);
	savefile->WriteObject(airlockMaster);
	savefile->WriteBool(openWhenDead);
	savefile->WriteBool(forcedOpen);
	savefile->WriteFloat(airLockSndWait);
}

void hhDoor::Restore( idRestoreGame *savefile ) {
	savefile->ReadString(airlockTeamName);
	savefile->ReadObject( reinterpret_cast<idClass *&>(airlockMaster) );
	savefile->ReadBool(openWhenDead);
	savefile->ReadBool(forcedOpen);
	savefile->ReadFloat(airLockSndWait);

	airlockTeam.SetOwner( this );
	if( airlockMaster && airlockMaster != this ) {
		JoinAirLockTeam( airlockMaster );
	}

	nextAirLockSnd = 0;

	bShuttleDoors = spawnArgs.GetBool( "shuttle_doors" );
}

//--------------------------------
// hhDoor::~hhDoor
//--------------------------------
hhDoor::~hhDoor() {
	airlockTeam.Remove();
}

//--------------------------------
// hhDoor::Killed
//--------------------------------
void hhDoor::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	
	// Only can really be killed if we can be damaged
	if ( openWhenDead ) {
		Open();
	
		fl.takedamage = false;
		forcedOpen = true;	
	}
}		// Killed( idEntity *, idEntity *, int, const idVec3 &, int ) 

//--------------------------------
// hhDoor::Use_BinaryMover
//--------------------------------
void hhDoor::Use_BinaryMover( idEntity *activator ) {
	if ( airlockMaster && activator && !activator->IsType( idPlayer::Type ) ) {
		return; // Don't allow anyone but the player to affect airlock doors
	}

	// only the master should be used
	if ( moveMaster != this ) {
		moveMaster->Use_BinaryMover( activator );
		return;
	}

	if ( !enabled ) {
		return;
	}

	if ( moverState == MOVER_POS1 || moverState == MOVER_2TO1 ) {
		GotoPosition2();
	}
	else if ( moverState == MOVER_POS2 || moverState == MOVER_1TO2 ) {
		GotoPosition1();
	}
}

//--------------------------------
// hhDoor::GotoPosition1
//
// CloseDoor
//--------------------------------
void hhDoor::GotoPosition1() {
	GotoPosition1( spawnArgs.GetBool("toggle") ? 0.0f : wait );
}

//--------------------------------
// hhDoor::GotoPosition1
//
// CloseDoor
//--------------------------------
void hhDoor::GotoPosition1( float wait ) {
	idMover_Binary* slave = NULL;
	int	partial = 0;

	//HUMANHEAD: aob - airlock stuff
	if( !CanClose() ) {
		return;
	}
	//HUMNAHEAD END

	if ( moverState == MOVER_POS2 ) {
		SetGuiStates( guiBinaryMoverStates[MOVER_2TO1] );

		CancelReturnToPos1();

		if( wait > 0 ) {
			//HUMANHEAD: aob
			PostEventSec( &EV_Mover_ReturnToPos1, wait );
			//HUMANHEAD END
		} else {
			ProcessEvent( &EV_Mover_ReturnToPos1 );
		}
	}

	else

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		MatchActivateTeam( MOVER_2TO1, physicsObj.GetTime() - partial );
	}
}

//--------------------------------
// hhDoor::GotoPosition2
//
// OpenDoor
//--------------------------------
void hhDoor::GotoPosition2() {
	GotoPosition2( 0.0f );
}

//--------------------------------
// hhDoor::GotoPosition2
//
// OpenDoor
//--------------------------------
void hhDoor::GotoPosition2( float wait ) {
	int partial = 0;
	hhDoor *airlockMaster = GetAirLockMaster();

	//HUMANHEAD: aob - airlock stuff
	if( !CanOpen() && airlockMaster ) {
		ForceAirLockTeamClosed();
		if( gameLocal.time > nextAirLockSnd ) {
			int length;
			StartSound( "snd_airlock", SND_CHANNEL_BODY, 0, false, &length );
			nextAirLockSnd = gameLocal.time + length + airLockSndWait;
		}
		return;
	}
	//HUMNAHEAD END

	if ( moverState == MOVER_POS1 ) {
		ActivatePrefixed("triggerStartOpen", GetActivator());

		SetGuiStates( guiBinaryMoverStates[MOVER_1TO2] );

		MatchActivateTeam( MOVER_1TO2, gameLocal.time );

		// open areaportal
		ProcessEvent( &EV_Mover_OpenPortal );
	}
	else if ( moverState == MOVER_2TO1 ) {	// only partway up before reversing
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		MatchActivateTeam( MOVER_1TO2, physicsObj.GetTime() - partial );
	}

	if( airlockMaster ) {
		// Mark the other team members as locked
		for( hhDoor* node = airlockMaster->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {

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
}

//--------------------------------
// hhDoor::Open
//--------------------------------
void hhDoor::Open( void ) {
	GotoPosition2( wait );
}

//--------------------------------
// hhDoor::Close
//--------------------------------
void hhDoor::Close( void ) {
	// HUMANHEAD nla
	if ( ForcedOpen() ) { return; }
	// HUMANHEAD

	GotoPosition1( 0.0f );
}

//--------------------------------
// hhDoor::CanOpen
//--------------------------------
bool hhDoor::CanOpen() const {
	if( !GetAirLockMaster() ) {
		return true;
	}

	for( hhDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node == this ) {
			continue;
		}

		if( OnMyMoveTeam(node) ) {
			continue;
		}
			
		if( node->IsOpen() ) {
			return false;
		}
	}

	return true;
}

//--------------------------------
// hhDoor::CanClose
//--------------------------------
bool hhDoor::CanClose() const {
	return true;
}

//--------------------------------
// hhDoor::OnMyMoveTeam
//--------------------------------
bool hhDoor::OnMyMoveTeam( hhDoor* doorPiece ) const {
	for( const idMover_Binary* localDoorPiece = this; localDoorPiece != NULL; localDoorPiece = localDoorPiece->GetActivateChain() ) {
		if( localDoorPiece == doorPiece ) {
			return true;
		}
	}

	return false;
}

//--------------------------------
// hhDoor::CopyTeamInfoToMoveMaster
//--------------------------------
void hhDoor::CopyTeamInfoToMoveMaster( hhDoor* master ) {
	if( !master ) {
		return;
	}

	for( int ix = buddies.Num() - 1; ix >= 0; --ix ) {
		master->buddies.AddUnique( buddies[ix] );
	}

	for( int ix = buddyNames.Num() - 1; ix >= 0; --ix ) {
		master->buddyNames.AddUnique( buddyNames[ix] );
	}
}

//--------------------------------
// hhDoor::DetermineTeamMaster
//--------------------------------
idEntity* hhDoor::DetermineTeamMaster( const char* teamName ) {
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
// hhDoor::JoinAirLockTeam
//--------------------------------
void hhDoor::JoinAirLockTeam( hhDoor *master ) {
	assert( master );

	airlockTeam.AddToEnd( master->airlockTeam );
}

//--------------------------------
// hhDoor::VerifyAirlockTeamStatus
//--------------------------------
void hhDoor::VerifyAirlockTeamStatus() {
	if( !GetAirLockMaster() ) {
		return;
	}

	for( hhDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node == this ) {
			continue;
		}

		if( OnMyMoveTeam(node) ) {
			continue;
		}
			
		if( !node->IsClosed() ) {
			gameLocal.Warning( "Airlock team '%s' has more than one member starting open", GetAirLockTeamName() );
		}
	}
}

//--------------------------------
// hhDoor::IsClosed
//--------------------------------
bool hhDoor::IsClosed() const {
	return ( moverState == MOVER_POS1 );
}

//--------------------------------
// hhDoor::ForceAirLockTeamClosed
//--------------------------------
void hhDoor::ForceAirLockTeamClosed() {
	hhDoor* localMoveMaster = NULL;

	if( !GetAirLockMaster() ) {
		return;
	}
	
	for( hhDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node == this ) {
			continue;
		}

		if( !node->moveMaster->IsType(hhDoor::Type) ) {
			continue;
		}
		
		localMoveMaster = static_cast<hhDoor*>( node->moveMaster );
		//if( node != localMoveMaster ) {
		//	continue;
		//}

		localMoveMaster->CancelReturnToPos1();
		if( !localMoveMaster->IsClosed() ) {
			localMoveMaster->Close();
		}
	}
}

//--------------------------------
// hhDoor::ForceAirLockTeamOpen
//--------------------------------
void hhDoor::ForceAirLockTeamOpen() {
	hhDoor* localMoveMaster = NULL;

	if( !GetAirLockMaster() ) {
		return;
	}

	for( hhDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {
		if( node == this ) {
			continue;
		}

		if( !node->moveMaster->IsType(hhDoor::Type) ) {
			continue;
		}

		localMoveMaster = static_cast<hhDoor*>( node->moveMaster );
		//if( node != localMoveMaster ) {
		//	continue;
		//}

		localMoveMaster->CancelReturnToPos1();
		if( !localMoveMaster->IsOpen() ) {
			localMoveMaster->Open();
		}
	}
}

//--------------------------------
// hhDoor::CancelReturnToPos1
//--------------------------------
void hhDoor::CancelReturnToPos1() {
	for( idMover_Binary* slave = this; slave != NULL; slave = slave->GetActivateChain() ) {
		slave->CancelEvents( &EV_Mover_ReturnToPos1 );
	}
}

//--------------------------------
// hhDoor::SetBuddiesShaderParm
//--------------------------------
void hhDoor::SetBuddiesShaderParm( int parm, float value ) {
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
// hhDoor::ToggleBuddiesShaderParm
//--------------------------------
void hhDoor::ToggleBuddiesShaderParm( int parm, float firstValue, float secondValue, float toggleDelay) {
	SetBuddiesShaderParm( parm, firstValue );
	PostEventMS( &EV_SetBuddiesShaderParm, toggleDelay, parm, secondValue );
}

//--------------------------------
// hhDoor::Event_SetBuddiesShaderParm
//--------------------------------
void hhDoor::Event_SetBuddiesShaderParm( int parm, float value ) {
	SetBuddiesShaderParm( parm, value );
}

//--------------------------------
// hhDoor::Event_ReturnToPos1
//--------------------------------
void hhDoor::Event_ReturnToPos1( void ) {

	if ( !ForcedOpen() ) {
		idDoor::Event_ReturnToPos1();
	}

}		// Event_ReturnToPos1()

//--------------------------------
// hhDoor::Event_Reached_BinaryMover
//--------------------------------
void hhDoor::Event_Reached_BinaryMover( void ) {
	if ( moverState == MOVER_2TO1 ) {
		SetBlocked(false);
		ActivatePrefixed("triggerClosed", this);
	} else if (moverState == MOVER_1TO2) {
		ActivatePrefixed("triggerOpened", this);
	}

	if ( moverState == MOVER_1TO2 ) {
		// reached pos2
		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		// HUMANHEAD aob - removed
		//if ( moveMaster == this ) {
			//StartSound( "snd_opened", SND_CHANNEL_ANY );
		//}
		//HUMANHEAD END

		SetMoverState( MOVER_POS2, gameLocal.time );

		SetGuiStates( guiBinaryMoverStates[MOVER_POS2] );

		UpdateBuddies(1);		

		if( enabled && wait >= 0 && !spawnArgs.GetBool("toggle") ) {
			// return to pos1 after a delay
			//HUMANHEAD: aob
			CancelReturnToPos1();
			//HUMANHEAD END
			PostEventSec( &EV_Mover_ReturnToPos1, wait );
		}
		
		// fire targets
		ActivateTargets( moveMaster->GetActivator() );
	} else if ( moverState == MOVER_2TO1 ) {
		// reached pos1
		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		SetMoverState( MOVER_POS1, gameLocal.time );

		SetGuiStates( guiBinaryMoverStates[MOVER_POS1] );

		UpdateBuddies(0);		

		// close areaportals
		if ( moveMaster == this ) {
			ProcessEvent( &EV_Mover_ClosePortal );
		}

		if( GetAirLockMaster() ) {
			// Mark the other team members as unlocked
			for( hhDoor* node = GetAirLockMaster()->airlockTeam.ListHead()->Owner(); node != NULL; node = node->airlockTeam.Next() ) {

				// Skip this door and it's buddies
				if( node == this || buddyNames.Find( node->name ) ) {
					continue;
				}

				// Don't change doors that are really locked
				if( node->IsLocked() ) {
					continue;
				}

				// Mark as unlocked when door closes
				node->SetBuddiesShaderParm( SHADERPARM_MISC, 0.0f );
			}
		}
	} else {
		gameLocal.Error( "Event_Reached_BinaryMover: bad moverState" );
	}
}

/*
================
hhDoor::Event_Touch
================
*/
void hhDoor::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( bShuttleDoors && ( ! other->IsType( idActor::Type ) || ! reinterpret_cast<idActor *> ( other )->InVehicle() ) ) {
		if ( gameLocal.time > nextSndTriggerTime ) {
			StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
			nextSndTriggerTime = gameLocal.time + 10000;
		}
		return;
	}

	idDoor::Event_Touch( other, trace );
}
