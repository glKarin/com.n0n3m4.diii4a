// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

	Client running game code:
	- entity events don't work and should not be issued
	- entities should never be spawned outside idGameLocal::ClientReadSnapshot

===============================================================================
*/

#define NET_MEMCPY memcpy	//HUMANHEAD rww - to easily test performance differences.
							/*
							leaving as memcpy for now because ingame profiler and codeanalyst show no performance difference whatsoever.
							i'm guessing it would just be slower on machines that don't support any of the higher level simd.
							note that it probably is faster to use simd in debug due to lack of intrinsic function optimizations,
							but, well, who cares?
							*/

// adds tags to the network protocol to detect when things go bad ( internal consistency )
// NOTE: this changes the network protocol
#ifndef ASYNC_WRITE_TAGS
	#define ASYNC_WRITE_TAGS 0
#endif

idCVar net_clientShowSnapshot( "net_clientShowSnapshot", "0", CVAR_GAME | CVAR_INTEGER, "", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar net_clientShowSnapshotRadius( "net_clientShowSnapshotRadius", "128", CVAR_GAME | CVAR_FLOAT, "" );
idCVar net_clientSmoothing( "net_clientSmoothing", "0.8", CVAR_GAME | CVAR_FLOAT, "smooth other clients angles and position.", 0.0f, 0.95f );
idCVar net_clientSelfSmoothing( "net_clientSelfSmoothing", "0.6", CVAR_GAME | CVAR_FLOAT, "smooth self position if network causes prediction error.", 0.0f, 0.95f );
idCVar net_clientMaxPrediction( "net_clientMaxPrediction", "1000", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of milliseconds a client can predict ahead of server." );
idCVar net_clientLagOMeter( "net_clientLagOMeter", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_ARCHIVE, "draw prediction graph" );
#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
idCVar net_statGathering( "net_statGathering", "0", CVAR_GAME | CVAR_BOOL, "", 0, 1 );
idCVar net_snapSuppress( "net_snapSuppress", "", CVAR_GAME, "", 0, 1 );
#endif //HUMANHEAD END

/*
================
idGameLocal::InitAsyncNetwork
================
*/
void idGameLocal::InitAsyncNetwork( void ) {
	int i, type;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		for ( type = 0; type < declManager->GetNumDeclTypes(); type++ ) {
			clientDeclRemap[i][type].Clear();
		}
	}

	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	eventQueue.Init();
	savedEventQueue.Init();

	entityDefBits = -( idMath::BitsForInteger( declManager->GetNumDecls( DECL_ENTITYDEF ) ) + 1 );
	localClientNum = 0; // on a listen server SetLocalUser will set this right
	realClientTime = 0;
	isNewFrame = true;
	clientSmoothing = net_clientSmoothing.GetFloat();
}

/*
================
idGameLocal::ShutdownAsyncNetwork
================
*/
void idGameLocal::ShutdownAsyncNetwork( void ) {
	entityStateAllocator.Shutdown();
	snapshotAllocator.Shutdown();
	eventQueue.Shutdown();
	savedEventQueue.Shutdown();
	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );
}

/*
================
idGameLocal::InitLocalClient
================
*/
void idGameLocal::InitLocalClient( int clientNum ) {
	isServer = false;
	isClient = true;
	localClientNum = clientNum;
	clientSmoothing = net_clientSmoothing.GetFloat();
}

/*
================
idGameLocal::InitClientDeclRemap
================
*/
void idGameLocal::InitClientDeclRemap( int clientNum ) {
	int type, i, num;

	for ( type = 0; type < declManager->GetNumDeclTypes(); type++ ) {

		// only implicit materials and sound shaders decls are used
		if ( type != DECL_MATERIAL && type != DECL_SOUND ) {
			continue;
		}

		num = declManager->GetNumDecls( (declType_t) type );
		clientDeclRemap[clientNum][type].Clear();
		clientDeclRemap[clientNum][type].AssureSize( num, -1 );

		// pre-initialize the remap with non-implicit decls, all non-implicit decls are always going
		// to be in order and in sync between server and client because of the decl manager checksum
		for ( i = 0; i < num; i++ ) {
			const idDecl *decl = declManager->DeclByIndex( (declType_t) type, i, false );
			if ( decl->IsImplicit() ) {
				// once the first implicit decl is found all remaining decls are considered implicit as well
				break;
			}
			clientDeclRemap[clientNum][type][i] = i;
		}
	}
}

/*
================
idGameLocal::ServerSendDeclRemapToClient
================
*/
void idGameLocal::ServerSendDeclRemapToClient( int clientNum, declType_t type, int index ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	// if no client connected for this spot
	if ( entities[clientNum] == NULL ) {
		return;
	}
	// increase size of list if required
	if ( index >= clientDeclRemap[clientNum][type].Num() ) {
		clientDeclRemap[clientNum][(int)type].AssureSize( index + 1, -1 );
	}
	// if already remapped
	if ( clientDeclRemap[clientNum][(int)type][index] != -1 ) {
		return;
	}

	const idDecl *decl = declManager->DeclByIndex( type, index, false );
	if ( decl == NULL ) {
		gameLocal.Error( "server tried to remap bad %s decl index %d", declManager->GetDeclNameFromType( type ), index );
		return;
	}

	// set the index at the server
	clientDeclRemap[clientNum][(int)type][index] = index;

	// write update to client
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_REMAP_DECL );
	outMsg.WriteByte( type );
	outMsg.WriteLong( index );
	outMsg.WriteString( decl->GetName() );
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );
}

/*
================
idGameLocal::ServerRemapDecl
================
*/
int idGameLocal::ServerRemapDecl( int clientNum, declType_t type, int index ) {

	// only implicit materials and sound shaders decls are used
	if ( type != DECL_MATERIAL && type != DECL_SOUND ) {
		return index;
	}

	if ( clientNum == -1 ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			ServerSendDeclRemapToClient( i, type, index );
		}
	} else {
		ServerSendDeclRemapToClient( clientNum, type, index );
	}
	return index;
}

/*
================
idGameLocal::ClientRemapDecl
================
*/
int idGameLocal::ClientRemapDecl( declType_t type, int index ) {

	// only implicit materials and sound shaders decls are used
	if ( type != DECL_MATERIAL && type != DECL_SOUND ) {
		return index;
	}

	// negative indexes are sometimes used for NULL decls
	if ( index < 0 ) {
		return index;
	}

	// make sure the index is valid
	if ( clientDeclRemap[localClientNum][(int)type].Num() == 0 ) {
		gameLocal.Error( "client received decl index %d before %s decl remap was initialized", index, declManager->GetDeclNameFromType( type ) );
		return -1;
	}
	if ( index >= clientDeclRemap[localClientNum][(int)type].Num() ) {
		gameLocal.Error( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		return -1;
	}
	if ( clientDeclRemap[localClientNum][(int)type][index] == -1 ) {
		gameLocal.Error( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		return -1;
	}
	return clientDeclRemap[localClientNum][type][index];
}

/*
================
idGameLocal::ServerAllowClient
================
*/
allowReply_t idGameLocal::ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[ MAX_STRING_CHARS ] ) {
	reason[0] = '\0';

	if ( serverInfo.GetInt( "si_pure" ) && !mpGame.IsPureReady() ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_07139" );
		return ALLOW_NOTYET;
	}

	if ( !serverInfo.GetInt( "si_maxPlayers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_07140" );
		return ALLOW_NOTYET;
	}

	if ( numClients >= serverInfo.GetInt( "si_maxPlayers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_07141" );
		return ALLOW_NOTYET;
	}

	if ( !cvarSystem->GetCVarBool( "si_usepass" ) ) {
		return ALLOW_YES;
	}

	const char *pass = cvarSystem->GetCVarString( "g_password" );
	if ( pass[ 0 ] == '\0' ) {
		common->Warning( "si_usepass is set but g_password is empty" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say si_usepass is set but g_password is empty" );
		// avoids silent misconfigured state
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_07142" );
		return ALLOW_NOTYET;
	}

	if ( !idStr::Cmp( pass, password ) ) {
		return ALLOW_YES;
	}

	idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_07143" );
	Printf( "Rejecting client %s from IP %s: invalid password\n", guid, IP );
	return ALLOW_BADPASS;
}

/*
================
idGameLocal::ServerClientConnect
================
*/
void idGameLocal::ServerClientConnect( int clientNum ) {
	// make sure no parasite entity is left
	if ( entities[ clientNum ] ) {
		common->DPrintf( "ServerClientConnect: remove old player entity\n" );
		delete entities[ clientNum ];
	}
	userInfo[ clientNum ].Clear();
	mpGame.ServerClientConnect( clientNum );
	Printf( "client %d connected.\n", clientNum );

	unreliableSnapMsg[clientNum].Init(0); //HUMANHEAD rww
}

/*
================
idGameLocal::ServerClientBegin
================
*/
void idGameLocal::ServerClientBegin( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	// initialize the decl remap
	InitClientDeclRemap( clientNum );

	// send message to initialize decl remap at the client (this is always the very first reliable game message)
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP );
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// spawn the player
	SpawnPlayer( clientNum );
	if ( clientNum == localClientNum ) {
		mpGame.EnterGame( clientNum );
	}

	// send message to spawn the player at the clients
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPAWN_PLAYER );
	outMsg.WriteByte( clientNum );
	outMsg.WriteLong( spawnIds[ clientNum ] );
	networkSystem->ServerSendReliableMessage( -1, outMsg );
}

/*
================
idGameLocal::ServerClientDisconnect
================
*/
void idGameLocal::ServerClientDisconnect( int clientNum ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DELETE_ENT );
	//HUMANHEAD rww - take cent bits into account
	outMsg.WriteBits( ( spawnIds[ clientNum ] << GENTITYNUM_BITS_PLUSCENT ) | clientNum, 32 ); // see GetSpawnId
	networkSystem->ServerSendReliableMessage( -1, outMsg );

	// free snapshots stored for this client
	FreeSnapshotsOlderThanSequence( clientNum, 0x7FFFFFFF );

	// free entity states stored for this client
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		if ( clientEntityStates[ clientNum ][ i ] ) {
			entityStateAllocator.Free( clientEntityStates[ clientNum ][ i ] );
			clientEntityStates[ clientNum ][ i ] = NULL;
		}
	}

	// clear the client PVS
	memset( clientPVS[ clientNum ], 0, sizeof( clientPVS[ clientNum ] ) );

	// delete the player entity
	delete entities[ clientNum ];

	mpGame.DisconnectClient( clientNum );

}

/*
================
idGameLocal::ServerWriteInitialReliableMessages

  Send reliable messages to initialize the client game up to a certain initial state.
================
*/
void idGameLocal::ServerWriteInitialReliableMessages( int clientNum ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	entityNetEvent_t *event;

	// spawn players
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( entities[i] == NULL || i == clientNum ) {
			continue;
		}
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting( );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPAWN_PLAYER );
		outMsg.WriteByte( i );
		outMsg.WriteLong( spawnIds[ i ] );
		networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	}

	// send all saved events
	for ( event = savedEventQueue.Start(); event; event = event->next ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
		outMsg.WriteBits( event->spawnId, 32 );
		outMsg.WriteByte( event->event );
		outMsg.WriteLong( event->time );
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
		outMsg.WriteBits( event->entTypeId, idClass::GetTypeNumBits() );
#endif //HUMANHEAD END
		outMsg.WriteBits( event->paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		if ( event->paramsSize ) {
			outMsg.WriteData( event->paramsBuf, event->paramsSize );
		}

		networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	}

	// update portals for opened doors
	int numPortals = gameRenderWorld->NumPortals();
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_PORTALSTATES );
	outMsg.WriteLong( numPortals );
	for ( i = 0; i < numPortals; i++ ) {
		outMsg.WriteBits( gameRenderWorld->GetPortalState( (qhandle_t) (i+1) ) , NUM_RENDER_PORTAL_BITS );
	}
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	mpGame.ServerWriteInitialReliableMessages( clientNum );
}

/*
================
idGameLocal::SaveEntityNetworkEvent
================
*/
void idGameLocal::SaveEntityNetworkEvent( const idEntity *ent, int eventId, const idBitMsg *msg ) {
	entityNetEvent_t *event;

	event = savedEventQueue.Alloc();
	event->spawnId = GetSpawnId( ent );
	event->event = eventId;
	event->time = time;
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
	event->entTypeId = ent->GetType()->typeNum;
#endif //HUMANHEAD END
	if ( msg ) {
		event->paramsSize = msg->GetSize();
		NET_MEMCPY( event->paramsBuf, msg->GetData(), msg->GetSize() ); //HUMANHEAD rww - testing out performance for simd memcpy
	} else {
		event->paramsSize = 0;
	}

	savedEventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );
}

/*
================
idGameLocal::FreeSnapshotsOlderThanSequence
================
*/
void idGameLocal::FreeSnapshotsOlderThanSequence( int clientNum, int sequence ) {
	snapshot_t *snapshot, *lastSnapshot, *nextSnapshot;
	entityState_t *state;

	for ( lastSnapshot = NULL, snapshot = clientSnapshots[clientNum]; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence < sequence ) {
			for ( state = snapshot->firstEntityState; state; state = snapshot->firstEntityState ) {
				snapshot->firstEntityState = snapshot->firstEntityState->next;
				entityStateAllocator.Free( state );
			}
			if ( lastSnapshot ) {
				lastSnapshot->next = snapshot->next;
			} else {
				clientSnapshots[clientNum] = snapshot->next;
			}
			snapshotAllocator.Free( snapshot );
		} else {
			lastSnapshot = snapshot;
		}
	}
}

/*
================
idGameLocal::ApplySnapshot
================
*/
bool idGameLocal::ApplySnapshot( int clientNum, int sequence ) {
	snapshot_t *snapshot, *lastSnapshot, *nextSnapshot;
	entityState_t *state;

	FreeSnapshotsOlderThanSequence( clientNum, sequence );

	for ( lastSnapshot = NULL, snapshot = clientSnapshots[clientNum]; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence == sequence ) {
			for ( state = snapshot->firstEntityState; state; state = state->next ) {
				assert(!entities[state->entityNumber] || !entities[state->entityNumber]->fl.clientEntity); //HUMANHEAD rww

				if ( clientEntityStates[clientNum][state->entityNumber] ) {
					entityStateAllocator.Free( clientEntityStates[clientNum][state->entityNumber] );
				}
				clientEntityStates[clientNum][state->entityNumber] = state;
			}
			NET_MEMCPY( clientPVS[clientNum], snapshot->pvs, sizeof( snapshot->pvs ) ); //HUMANHEAD rww - testing out performance for simd memcpy
			if ( lastSnapshot ) {
				lastSnapshot->next = nextSnapshot;
			} else {
				clientSnapshots[clientNum] = nextSnapshot;
			}
			snapshotAllocator.Free( snapshot );
			return true;
		} else {
			lastSnapshot = snapshot;
		}
	}

	return false;
}

/*
================
idGameLocal::WriteGameStateToSnapshot
================
*/
void idGameLocal::WriteGameStateToSnapshot( idBitMsgDelta &msg ) const {
	int i;

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		msg.WriteFloat( globalShaderParms[i] );
	}

	mpGame.WriteToSnapshot( msg );
}

/*
================
idGameLocal::ReadGameStateFromSnapshot
================
*/
void idGameLocal::ReadGameStateFromSnapshot( const idBitMsgDelta &msg ) {
	int i;

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		globalShaderParms[i] = msg.ReadFloat();
	}

	mpGame.ReadFromSnapshot( msg );
}

/*
================
idGameLocal::ServerWriteSnapshot

  Write a snapshot of the current game state for the given client.
================
*/
void idGameLocal::ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients ) {
	PROFILE_SCOPE("ServerWriteSnapshot", PROFMASK_NORMAL); //HUMANHEAD rww

	int i, msgSize, msgWriteBit;
	idPlayer *player, *spectated = NULL;
	idEntity *ent;
	pvsHandle_t pvsHandle;
	idBitMsgDelta deltaMsg;
	snapshot_t *snapshot;
	entityState_t *base, *newBase;
	int numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];

	player = static_cast<idPlayer *>( entities[ clientNum ] );
	if ( !player ) {
		return;
	}
	if ( player->spectating && player->spectator != clientNum && entities[ player->spectator ] ) {
		spectated = static_cast< idPlayer * >( entities[ player->spectator ] );
	} else {
		spectated = player;
	}
	
	// free too old snapshots
	FreeSnapshotsOlderThanSequence( clientNum, sequence - 64 );

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->sequence = sequence;
	snapshot->firstEntityState = NULL;
	snapshot->next = clientSnapshots[clientNum];
	clientSnapshots[clientNum] = snapshot;
	memset( snapshot->pvs, 0, sizeof( snapshot->pvs ) );

	// get PVS for this player
	// don't use PVSAreas for networking - PVSAreas depends on animations (and md5 bounds), which are not synchronized
	numSourceAreas = gameRenderWorld->BoundsInAreas( spectated->GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS );
	pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );

#if ASYNC_WRITE_TAGS
	idRandom tagRandom;
	tagRandom.SetSeed( random.RandomInt() );
	msg.WriteLong( tagRandom.GetSeed() );
#endif

	//HUMANHEAD rww - append the unreliable messages and clear the queue (was doing this after ents, but want some events to occur before the ent's snapshot is read)
	unreliableSnapMsg[clientNum].WriteToMsg(msg);
	unreliableSnapMsg[clientNum].Init(0);

	// create the snapshot
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		// if the entity is not in the player PVS
		if ( !ent->PhysicsTeamInPVS( pvsHandle ) && ent->entityNumber != clientNum ) {
#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
			if ( ent->entityNumber >= MAX_CLIENTS && gameLocal.spawnIds[ent->entityNumber] <= mapSpawnCount && ent->GetNumPVSAreas() <= 0 ) {
				common->DWarning( "server can't sync map entity 0x%x (%s) because it has no valid pvs, sequence 0x%x", ent->entityNumber, ent->name.c_str(), sequence );
			}
#endif //HUMANHEAD END

			continue;
		}

		// add the entity to the snapshot pvs
		snapshot->pvs[ ent->entityNumber >> 5 ] |= 1 << ( ent->entityNumber & 31 );

		// if that entity is not marked for network synchronization
		if ( !ent->fl.networkSync ) {
			continue;
		}

		assert(!ent->fl.clientEntity && ent->entityNumber < MAX_GENTITIES); //HUMANHEAD rww

		// save the write state to which we can revert when the entity didn't change at all
		msg.SaveWriteState( msgSize, msgWriteBit );

		// write the entity to the snapshot
		msg.WriteBits( ent->entityNumber, GENTITYNUM_BITS );

		base = clientEntityStates[clientNum][ent->entityNumber];
		if ( base ) {
			base->state.BeginReading();
		}
		newBase = entityStateAllocator.Alloc();
		newBase->entityNumber = ent->entityNumber;
		newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
		newBase->state.BeginWriting();

#if !GOLD //HUMANHEAD rww
		newBase->state.SetDebugEntType(ent->GetType()->typeNum);
#endif //HUMANHEAD END

		deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );

		deltaMsg.WriteBits( spawnIds[ ent->entityNumber ], 32 - GENTITYNUM_BITS_PLUSCENT ); //HUMANHEAD rww cent bits
		deltaMsg.WriteBits( ent->GetType()->typeNum, idClass::GetTypeNumBits() );
		deltaMsg.WriteBits( ServerRemapDecl( -1, DECL_ENTITYDEF, ent->entityDefNumber ), entityDefBits );

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		if (net_statGathering.GetBool()) {
			deltaMsg.BeginEntLog(ent->GetType()->typeNum);
		}

		const char *suppress = net_snapSuppress.GetString();
		if (suppress && !stricmp(suppress, ent->GetType()->classname)) {
			deltaMsg.WriteBits(1, 1);
		}
		else {
			deltaMsg.WriteBits(0, 1);
#endif //HUMANHEAD END

		// write the class specific data to the snapshot
		ent->WriteToSnapshot( deltaMsg );

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		}

		if (net_statGathering.GetBool()) {
			deltaMsg.BeginEntLog(0);
		}
#endif //HUMANHEAD END

		if ( !deltaMsg.HasChanged() ) {
			msg.RestoreWriteState( msgSize, msgWriteBit );
			entityStateAllocator.Free( newBase );
		} else {
			newBase->next = snapshot->firstEntityState;
			snapshot->firstEntityState = newBase;

#if ASYNC_WRITE_TAGS
			msg.WriteLong( tagRandom.RandomInt() );
#endif
		}
	}

	msg.WriteBits( ENTITYNUM_NONE, GENTITYNUM_BITS );

	// write the PVS to the snapshot
#if ASYNC_WRITE_PVS
	for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
		if ( i < numSourceAreas ) {
			msg.WriteLong( sourceAreas[ i ] );
		} else {
			msg.WriteLong( 0 );
		}
	}
	gameLocal.pvs.WritePVS( pvsHandle, msg );
#endif
	for ( i = 0; i < ENTITY_PVS_SIZE; i++ ) {
		msg.WriteDeltaLong( clientPVS[clientNum][i], snapshot->pvs[i] );
	}

	// free the PVS
	pvs.FreeCurrentPVS( pvsHandle );

	// write the game and player state to the snapshot
	base = clientEntityStates[clientNum][ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game and player state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->next = snapshot->firstEntityState;
	snapshot->firstEntityState = newBase;
	newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
	newBase->state.BeginWriting();
	deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );
	if ( player->spectating && player->spectator != player->entityNumber && gameLocal.entities[ player->spectator ] && gameLocal.entities[ player->spectator ]->IsType( idPlayer::Type ) ) {
		static_cast< idPlayer * >( gameLocal.entities[ player->spectator ] )->WritePlayerStateToSnapshot( deltaMsg );
	} else {
		player->WritePlayerStateToSnapshot( deltaMsg );
	}
	WriteGameStateToSnapshot( deltaMsg );

	// copy the client PVS string
	NET_MEMCPY( clientInPVS, snapshot->pvs, ( numPVSClients + 7 ) >> 3 ); //HUMANHEAD rww - testing out performance for simd memcpy
	LittleRevBytes( clientInPVS, sizeof( int ), sizeof( clientInPVS ) / sizeof ( int ) );
}

/*
================
idGameLocal::ServerApplySnapshot
================
*/
bool idGameLocal::ServerApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( clientNum, sequence );
}

/*
================
idGameLocal::NetworkEventWarning
================
*/
void idGameLocal::NetworkEventWarning( const entityNetEvent_t *event, const char *fmt, ... ) {
	char buf[1024];
	int length = 0;
	va_list argptr;

	//HUMANHEAD rww - take cent bits into account
	int entityNum	= event->spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 );
	int id			= event->spawnId >> GENTITYNUM_BITS_PLUSCENT;

	length += idStr::snPrintf( buf+length, sizeof(buf)-1-length, "event %d for entity %d %d: ", event->event, entityNum, id );
	va_start( argptr, fmt );
	length = idStr::vsnPrintf( buf+length, sizeof(buf)-1-length, fmt, argptr );
	va_end( argptr );
	idStr::Append( buf, sizeof(buf), "\n" );
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
	if (event->entTypeId) {
		idTypeInfo *ti = idClass::GetType(event->entTypeId);
		if (ti) {
			idStr::Append(buf, sizeof(buf), "event's ent is of type '");
			idStr::Append(buf, sizeof(buf), ti->classname);
			idStr::Append(buf, sizeof(buf), "'.\n");
		}
		else {
			idStr::Append(buf, sizeof(buf), "typenum on event was invalid.\n");
		}
	}
	else {
		idStr::Append(buf, sizeof(buf), "unknown type on event.\n");
	}
#endif //HUMANHEAD END

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
	common->Warning( buf );
#else
	common->DWarning( buf );
#endif
}

/*
================
idGameLocal::ServerProcessEntityNetworkEventQueue
================
*/
void idGameLocal::ServerProcessEntityNetworkEventQueue( void ) {
	idEntity			*ent;
	entityNetEvent_t	*event;
	idBitMsg			eventMsg;

	while ( eventQueue.Start() ) {
		event = eventQueue.Start();

		if ( event->time > time ) {
			break;
		}

		idEntityPtr< idEntity > entPtr;
			
		if( !entPtr.SetSpawnId( event->spawnId ) ) {
			NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
		} else {
			ent = entPtr.GetEntity();
			assert( ent );

			eventMsg.Init( event->paramsBuf, sizeof( event->paramsBuf ) );
			eventMsg.SetSize( event->paramsSize );
			eventMsg.BeginReading();
			if ( !ent->ServerReceiveEvent( event->event, event->time, eventMsg ) ) {
				NetworkEventWarning( event, "unknown event" );
			}
		}

		entityNetEvent_t* freedEvent = eventQueue.Dequeue();
		assert( freedEvent == event );
		eventQueue.Free( event );
	}
}

//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
/*
================
idGameLocal::ServerSendSpecialMessage
================
*/
void idGameLocal::ServerSendSpecialMessage( serverSpecialMsg_e msgType, int to, const char *fromNonLoc, int numTextPtrs, const char **text ) {
	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	assert(SPECIALMSG_NUM < (1<<4));
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPECIAL );
	outMsg.WriteString( fromNonLoc );
	outMsg.WriteBits(msgType, 4);
	outMsg.WriteBits(numTextPtrs, 8);
	for (int i = 0; i < numTextPtrs; i++) {
		outMsg.WriteString( text[i], -1, false );
	}
	networkSystem->ServerSendReliableMessage( to, outMsg );

	if ( to == -1 || to == localClientNum ) {
		switch (msgType) {
			//HUMANHEAD PCF rww 05/17/06
			case SPECIALMSG_ALREADYRUNNINGMAP:
			//HUMANHEAD END
			case SPECIALMSG_JOINED:
				mpGame.AddChatLine( "%s^0: %s\n", common->GetLanguageDict()->GetString(fromNonLoc), va(common->GetLanguageDict()->GetString(text[0]), text[1]) );
				break;
			//HUMANHEAD PCF rww 05/17/06
			case SPECIALMSG_JUSTONE:
				mpGame.AddChatLine( "%s^0: %s\n", common->GetLanguageDict()->GetString(fromNonLoc), common->GetLanguageDict()->GetString(text[0]) );
				break;
			//HUMANHEAD END
			default:
				break;
		}
	}
}
//HUMANHEAD END

/*
================
idGameLocal::ServerSendChatMessage
================
*/
void idGameLocal::ServerSendChatMessage( int to, const char *name, const char *text ) {
	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CHAT );
	outMsg.WriteString( name );
	outMsg.WriteString( text, -1, false );
	networkSystem->ServerSendReliableMessage( to, outMsg );

	if ( to == -1 || to == localClientNum ) {
		mpGame.AddChatLine( "%s^0: %s\n", name, text );
	}
}

/*
================
idGameLocal::ServerProcessReliableMessage
================
*/
void idGameLocal::ServerProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	int id;

	id = msg.ReadByte();
	switch( id ) {
		//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
		case GAME_RELIABLE_MESSAGE_SPECIAL: {
			Error("ServerProcessReliableMessage got GAME_RELIABLE_MESSAGE_SPECIAL.");
			break;
		}
		//HUMANHEAD END

		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT: {
			char name[128];
			char text[128];

			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );

			mpGame.ProcessChatMessage( clientNum, id == GAME_RELIABLE_MESSAGE_TCHAT, name, text, NULL );

			break;
		}
		case GAME_RELIABLE_MESSAGE_VCHAT: {
			int index = msg.ReadLong();
			bool team = msg.ReadBits( 1 ) != 0;
			mpGame.ProcessVoiceChat( clientNum, team, index );
			break;
		}
		case GAME_RELIABLE_MESSAGE_KILL: {
			mpGame.WantKilled( clientNum );
			break;
		}
		case GAME_RELIABLE_MESSAGE_DROPWEAPON: {
			mpGame.DropWeapon( clientNum );
			break;
		}
		case GAME_RELIABLE_MESSAGE_CALLVOTE: {
			mpGame.ServerCallVote( clientNum, msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_CASTVOTE: {
			bool vote = ( msg.ReadByte() != 0 );
			mpGame.CastVote( clientNum, vote );
			break;
		}
#if 0
		// uncomment this if you want to track when players are in a menu
		case GAME_RELIABLE_MESSAGE_MENU: {
			bool menuUp = ( msg.ReadBits( 1 ) != 0 );
			break;
		}
#endif
		case GAME_RELIABLE_MESSAGE_EVENT: {
			entityNetEvent_t *event;

			// allocate new event
			event = eventQueue.Alloc();
			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_DROP );

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
			event->entTypeId = msg.ReadBits(idClass::GetTypeNumBits());
#endif //HUMANHEAD END

			event->paramsSize = msg.ReadBits( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
			if ( event->paramsSize ) {
				if ( event->paramsSize > MAX_EVENT_PARAM_SIZE ) {
					NetworkEventWarning( event, "invalid param size" );
					return;
				}
				msg.ReadByteAlign();
				msg.ReadData( event->paramsBuf, event->paramsSize );
			}
			break;
		}
		default: {
			Warning( "Unknown client->server reliable message: %d", id );
			break;
		}
	}
}

/*
================
idGameLocal::ClientShowSnapshot
================
*/
void idGameLocal::ClientShowSnapshot( int clientNum ) const {
	int baseBits;
	idEntity *ent;
	idPlayer *player;
	idMat3 viewAxis;
	idBounds viewBounds;
	entityState_t *base;

	if ( !net_clientShowSnapshot.GetInteger() ) {
		return;
	}

	player = static_cast<idPlayer *>( entities[clientNum] );
	if ( !player ) {
		return;
	}

	viewAxis = player->viewAngles.ToMat3();
	viewBounds = player->GetPhysics()->GetAbsBounds().Expand( net_clientShowSnapshotRadius.GetFloat() );

	for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {

		if ( net_clientShowSnapshot.GetInteger() == 1 && ent->snapshotBits == 0 ) {
			continue;
		}

		const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

		if ( !entBounds.IntersectsBounds( viewBounds ) ) {
			continue;
		}

		base = clientEntityStates[clientNum][ent->entityNumber];
		if ( base ) {
			baseBits = base->state.GetNumBitsWritten();
		} else {
			baseBits = 0;
		}

		if ( net_clientShowSnapshot.GetInteger() == 2 && baseBits == 0 ) {
			continue;
		}

		gameRenderWorld->DebugBounds( colorGreen, entBounds );
		gameRenderWorld->DrawText( va( "%d: %s (%d,%d bytes of %d,%d)\n", ent->entityNumber,
						ent->name.c_str(), ent->snapshotBits >> 3, ent->snapshotBits & 7, baseBits >> 3, baseBits & 7 ),
							entBounds.GetCenter(), 0.1f, colorWhite, viewAxis, 1 );
	}
}

/*
================
idGameLocal::UpdateLagometer
================
*/
void idGameLocal::UpdateLagometer( int aheadOfServer, int dupeUsercmds ) {
		int i, j, ahead;
		for ( i = 0; i < LAGO_HEIGHT; i++ ) {
			memmove( (byte *)lagometer + LAGO_WIDTH * 4 * i, (byte *)lagometer + LAGO_WIDTH * 4 * i + 4, ( LAGO_WIDTH - 1 ) * 4 );
		}
		j = LAGO_WIDTH - 1;
		for ( i = 0; i < LAGO_HEIGHT; i++ ) {
			lagometer[i][j][0] = lagometer[i][j][1] = lagometer[i][j][2] = lagometer[i][j][3] = 0;
		}
		ahead = idMath::Rint( (float)aheadOfServer / 16.0f );
		if ( ahead >= 0 ) {
			for ( i = 2 * Max( 0, 5 - ahead ); i < 2 * 5; i++ ) {
				lagometer[i][j][1] = 255;
				lagometer[i][j][3] = 255;
			}
		} else {
			for ( i = 2 * 5; i < 2 * ( 5 + Min( 10, -ahead ) ); i++ ) {
				lagometer[i][j][0] = 255;
				lagometer[i][j][1] = 255;
				lagometer[i][j][3] = 255;
			}
		}
		for ( i = LAGO_HEIGHT - 2 * Min( 6, dupeUsercmds ); i < LAGO_HEIGHT; i++ ) {
			lagometer[i][j][0] = 255;
			if ( dupeUsercmds <= 2 ) {
				lagometer[i][j][1] = 255;
			}
			lagometer[i][j][3] = 255;
		}
}

/*
================
idGameLocal::ClientReadSnapshot
================
*/
void idGameLocal::ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg ) {
	int				i, typeNum, entityDefNumber, numBitsRead;
	idTypeInfo		*typeInfo;
	idEntity		*ent;
	idPlayer		*player, *spectated;
	pvsHandle_t		pvsHandle;
	idDict			args;
	const char		*classname;
	idBitMsgDelta	deltaMsg;
	snapshot_t		*snapshot;
	entityState_t	*base, *newBase;
	int				spawnId;
	int				numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];
	hhWeapon		*weap;	// HUMANHEAD pdm: changed to hhWeapon *

	if ( net_clientLagOMeter.GetBool() && renderSystem ) {
		UpdateLagometer( aheadOfServer, dupeUsercmds );
		if ( !renderSystem->UploadImage( LAGO_IMAGE, (byte *)lagometer, LAGO_IMG_WIDTH, LAGO_IMG_HEIGHT ) ) {
			common->Printf( "lagometer: UploadImage failed. turning off net_clientLagOMeter\n" );
			net_clientLagOMeter.SetBool( false );
		}
	}

	InitLocalClient( clientNum );

	// clear any debug lines from a previous frame
	gameRenderWorld->DebugClearLines( time );

	// clear any debug polygons from a previous frame
	gameRenderWorld->DebugClearPolygons( time );

	// update the game time
	framenum = gameFrame;
	time = gameTime;
	previousTime = time - msec;
	timeRandom = time; //HUMANHEAD rww

	// so that StartSound/StopSound doesn't risk skipping
	isNewFrame = true;

	// clear the snapshot entity list
	snapshotEntities.Clear();

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->sequence = sequence;
	snapshot->firstEntityState = NULL;
	snapshot->next = clientSnapshots[clientNum];
	clientSnapshots[clientNum] = snapshot;

#if ASYNC_WRITE_TAGS
	idRandom tagRandom;
	tagRandom.SetSeed( msg.ReadLong() );
#endif

	//HUMANHEAD rww - read the appended unreliable messages
	ClientReadUnreliableSnapMessages(clientNum, msg);

	// read all entities from the snapshot
	for ( i = msg.ReadBits( GENTITYNUM_BITS ); i != ENTITYNUM_NONE; i = msg.ReadBits( GENTITYNUM_BITS ) ) {

		base = clientEntityStates[clientNum][i];
		if ( base ) {
			base->state.BeginReading();
		}
		newBase = entityStateAllocator.Alloc();
		newBase->entityNumber = i;
		newBase->next = snapshot->firstEntityState;
		snapshot->firstEntityState = newBase;
		newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
		newBase->state.BeginWriting();

		numBitsRead = msg.GetNumBitsRead();

		deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );

		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS_PLUSCENT ); //HUMANHEAD rww cent bits
		typeNum = deltaMsg.ReadBits( idClass::GetTypeNumBits() );
		entityDefNumber = ClientRemapDecl( DECL_ENTITYDEF, deltaMsg.ReadBits( entityDefBits ) );

		typeInfo = idClass::GetType( typeNum );
		if ( !typeInfo ) {
			Error( "Unknown type number %d for entity %d with class number %d", typeNum, i, entityDefNumber );
		}

		ent = entities[i];

		// if there is no entity or an entity of the wrong type
		if ( !ent || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber || spawnId != spawnIds[ i ] ) {

			if ( i < MAX_CLIENTS && ent ) {
				// SPAWN_PLAYER should be taking care of spawning the entity with the right spawnId
				common->Warning( "ClientReadSnapshot: recycling client entity %d\n", i );
			}

			delete ent;

			spawnCount = spawnId;

			args.Clear();
			args.SetInt( "spawn_entnum", i );
			args.Set( "name", va( "entity%d", i ) );

			if ( entityDefNumber >= 0 ) {
				if ( entityDefNumber >= declManager->GetNumDecls( DECL_ENTITYDEF ) ) {
					Error( "server has %d entityDefs instead of %d", entityDefNumber, declManager->GetNumDecls( DECL_ENTITYDEF ) );
				}
				classname = declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
				args.Set( "classname", classname );
				//HUMANHEAD rww - extra error checking on client
				if ( !SpawnEntityDef( args, &ent, true, false, true ) || !entities[i] || entities[i]->GetType()->typeNum != typeNum ) {
					Error( "Failed to spawn entity with classname '%s' of type '%s'", classname, typeInfo->classname );
				}
			} else {
				ent = SpawnEntityType( *typeInfo, &args, true );
				if ( !entities[i] || entities[i]->GetType()->typeNum != typeNum ) {
					Error( "Failed to spawn entity of type '%s'", typeInfo->classname );
				}
			}
			if ( i < MAX_CLIENTS && i >= numClients ) {
				numClients = i + 1;
			}
			assert(ent->entityNumber == i); //HUMANHEAD rww
		}

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = sequence;

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		bool suppress = !!deltaMsg.ReadBits(1);
		if (!suppress) {
#endif //HUMANHEAD END

		//HUMANHEAD rww - if the entity was zombified, do what's needed to unzombify it
		if (ent->fl.clientZombie) {
			ent->NetResurrect();
		}
		//HUMANHEAD END

		// read the class specific data from the snapshot
		ent->ReadFromSnapshot( deltaMsg );
#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		}
#endif //HUMANHEAD END

		ent->snapshotBits = msg.GetNumBitsRead() - numBitsRead;

#if ASYNC_WRITE_TAGS
		if ( msg.ReadLong() != tagRandom.RandomInt() ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "writeGameState" );
			if ( entityDefNumber >= 0 && entityDefNumber < declManager->GetNumDecls( DECL_ENTITYDEF ) ) {
				classname = declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
				Error( "write to and read from snapshot out of sync for classname '%s' of type '%s'", classname, typeInfo->classname );
			} else {
				Error( "write to and read from snapshot out of sync for type '%s'", typeInfo->classname );
			}
		}
#endif
	}

	player = static_cast<idPlayer *>( entities[clientNum] );
	if ( !player ) {
		return;
	}

	// if prediction is off, enable local client smoothing
	player->SetSelfSmooth( dupeUsercmds > 2 );

	if ( player->spectating && player->spectator != clientNum && entities[ player->spectator ] ) {
		spectated = static_cast< idPlayer * >( entities[ player->spectator ] );
	} else {
		spectated = player;
	}

	// get PVS for this player
	// don't use PVSAreas for networking - PVSAreas depends on animations (and md5 bounds), which are not synchronized
	numSourceAreas = gameRenderWorld->BoundsInAreas( spectated->GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS );
	pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );

	// read the PVS from the snapshot
#if ASYNC_WRITE_PVS
	int serverPVS[idEntity::MAX_PVS_AREAS];
	i = numSourceAreas;
	while ( i < idEntity::MAX_PVS_AREAS ) {
		sourceAreas[ i++ ] = 0;
	}
	for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
		serverPVS[ i ] = msg.ReadLong();
	}
	if ( memcmp( sourceAreas, serverPVS, idEntity::MAX_PVS_AREAS * sizeof( int ) ) ) {
		common->Warning( "client PVS areas != server PVS areas, sequence 0x%x", sequence );
		for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
			common->DPrintf( "%3d ", sourceAreas[ i ] );
		}
		common->DPrintf( "\n" );
		for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
			common->DPrintf( "%3d ", serverPVS[ i ] );
		}
		common->DPrintf( "\n" );
	}
	gameLocal.pvs.ReadPVS( pvsHandle, msg );
#endif
	for ( i = 0; i < ENTITY_PVS_SIZE; i++ ) {
		snapshot->pvs[i] = msg.ReadDeltaLong( clientPVS[clientNum][i] );
	}

	// add entities in the PVS that haven't changed since the last applied snapshot
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		// if the entity is already in the snapshot
		if ( ent->snapshotSequence == sequence ) {
			continue;
		}

		//HUMANHEAD rww
		if (ent->fl.clientEntity) {
			continue;
		}
		//HUMANHEAD END

		// if the entity is not in the snapshot PVS
		if ( !( snapshot->pvs[ent->entityNumber >> 5] & ( 1 << ( ent->entityNumber & 31 ) ) ) ) {
			if ( ent->PhysicsTeamInPVS( pvsHandle ) ) {
				//HUMANHEAD rww - id code was checking ent->entityNumber, correct thing to check when relating to map spawn count is gameLocal.spawnIds[ent->entityNumber]
				if ( ent->entityNumber >= MAX_CLIENTS && gameLocal.spawnIds[ent->entityNumber] <= mapSpawnCount ) {
					// server says it's not in PVS, client says it's in PVS
					// if that happens on map entities, most likely something is wrong
					// I can see that moving pieces along several PVS could be a legit situation though
					// this is a band aid, which means something is not done right elsewhere
					common->DWarning( "client thinks map entity 0x%x (%s) is stale, sequence 0x%x", ent->entityNumber, ent->name.c_str(), sequence );

					//HUMANHEAD PCF rww 05/04/06 - no longer needed
					/*
					if (developer.GetBool()) { //HUMANHEAD rww
						//draw a box around it
						idMat3		axis = player->viewAngles.ToMat3();
						idVec3		up = axis[ 2 ] * 5.0f;
						idBounds	viewTextBounds( player->GetPhysics()->GetOrigin() );
						idBounds	viewBounds( player->GetPhysics()->GetOrigin() );
						viewTextBounds.ExpandSelf( 8192.0f );
						viewBounds.ExpandSelf( 8192.0f );

						if ( viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
							const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
							int contents = ent->GetPhysics()->GetContents();
							if ( contents & CONTENTS_BODY ) {
								gameRenderWorld->DebugBounds( colorCyan, entBounds );
							} else if ( contents & CONTENTS_TRIGGER ) {
								gameRenderWorld->DebugBounds( colorOrange, entBounds );
							} else if ( contents & CONTENTS_SOLID ) {
								gameRenderWorld->DebugBounds( colorGreen, entBounds );
							} else {
								if ( !entBounds.GetVolume() ) {
									gameRenderWorld->DebugBounds( colorMdGrey, entBounds.Expand( 8.0f ) );
								} else {
									gameRenderWorld->DebugBounds( colorMdGrey, entBounds );
								}
							}
							if ( viewTextBounds.IntersectsBounds( entBounds ) ) {
								gameRenderWorld->DrawText( ent->name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
								gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );
							}
						}
					} //HUMANHEAD END
					*/
				} else {
					ent->NetZombify(); //HUMANHEAD rww
				}
			}
			//HUMANHEAD rww - if an entity exited the pvs and we missed the event, kill it as soon as we unlag
			//do not ever flag map ents which are not network sync'd as zombies.
			else if (!ent->fl.clientZombie && (ent->fl.networkSync || gameLocal.spawnIds[ent->entityNumber] > mapSpawnCount)) {
				ent->NetZombify();
			}
			//HUMANHEAD END
			continue;
		}

		//HUMANHEAD rww - if the entity was zombified, do what's needed to unzombify it
		if (ent->fl.clientZombie) {
			ent->NetResurrect();
		}
		//HUMANHEAD END

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = sequence;
		ent->snapshotBits = 0;

		base = clientEntityStates[clientNum][ent->entityNumber];
		if ( !base ) {
			// entity has probably fl.networkSync set to false
			continue;
		}

		base->state.BeginReading();

		deltaMsg.Init( &base->state, NULL, (const idBitMsg *)NULL );

		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS_PLUSCENT ); //HUMANHEAD rww cent bits
		typeNum = deltaMsg.ReadBits( idClass::GetTypeNumBits() );
		entityDefNumber = deltaMsg.ReadBits( entityDefBits );

		typeInfo = idClass::GetType( typeNum );

		assert(!ent->fl.clientEntity); //HUMANHEAD rww

		// if the entity is not the right type
		if ( !typeInfo || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber ) {
			// should never happen - it does though. with != entityDefNumber only?
#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
			common->Warning( "entity '%s' is not the right type %p 0x%d 0x%x 0x%x 0x%x", ent->GetName(), typeInfo, ent->GetType()->typeNum, typeNum, ent->entityDefNumber, entityDefNumber );
#else
			common->DWarning( "entity '%s' is not the right type %p 0x%d 0x%x 0x%x 0x%x", ent->GetName(), typeInfo, ent->GetType()->typeNum, typeNum, ent->entityDefNumber, entityDefNumber );
#endif
			continue;
		}

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		bool suppress = !!deltaMsg.ReadBits(1);
		if (!suppress) {
#endif //HUMANHEAD END

		// read the class specific data from the base state
		ent->ReadFromSnapshot( deltaMsg );

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
		}
#endif //HUMANHEAD END
	}

	// free the PVS
	pvs.FreeCurrentPVS( pvsHandle );

	// read the game and player state from the snapshot
	base = clientEntityStates[clientNum][ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game and player state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->next = snapshot->firstEntityState;
	snapshot->firstEntityState = newBase;
	newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
	newBase->state.BeginWriting();
	deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );
	if ( player->spectating && player->spectator != player->entityNumber && gameLocal.entities[ player->spectator ] && gameLocal.entities[ player->spectator ]->IsType( idPlayer::Type ) ) {
		static_cast< idPlayer * >( gameLocal.entities[ player->spectator ] )->ReadPlayerStateFromSnapshot( deltaMsg );
		weap = static_cast< idPlayer * >( gameLocal.entities[ player->spectator ] )->weapon.GetEntity();
		if ( weap && ( weap->GetRenderEntity()->bounds[0] == weap->GetRenderEntity()->bounds[1] ) ) {
			// update the weapon's viewmodel bounds so that the model doesn't flicker in the spectator's view
			weap->GetAnimator()->GetBounds( gameLocal.time, weap->GetRenderEntity()->bounds );
			weap->UpdateVisuals();
		}
	} else {
		player->ReadPlayerStateFromSnapshot( deltaMsg );
	}
	ReadGameStateFromSnapshot( deltaMsg );

	// visualize the snapshot
	ClientShowSnapshot( clientNum );

	// process entity events
	ClientProcessEntityNetworkEventQueue();
}

/*
================
idGameLocal::ClientApplySnapshot
================
*/
bool idGameLocal::ClientApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( clientNum, sequence );
}

/*
================
idGameLocal::ClientProcessEntityNetworkEventQueue
================
*/
void idGameLocal::ClientProcessEntityNetworkEventQueue( void ) {
	idEntity			*ent;
	entityNetEvent_t	*event;
	idBitMsg			eventMsg;

	while( eventQueue.Start() ) {
		event = eventQueue.Start();

		// only process forward, in order
		if ( event->time > time ) {
			break;
		}

		idEntityPtr< idEntity > entPtr;
			
		if( !entPtr.SetSpawnId( event->spawnId ) ) {
			//HUMANHEAD rww - take cent bits into account
			if( !gameLocal.entities[ event->spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 ) ] ) {
				// if new entity exists in this position, silently ignore
				NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
			}
		} else {
			ent = entPtr.GetEntity();
			assert( ent );

			eventMsg.Init( event->paramsBuf, sizeof( event->paramsBuf ) );
			eventMsg.SetSize( event->paramsSize );
			eventMsg.BeginReading();
			if ( !ent->ClientReceiveEvent( event->event, event->time, eventMsg ) ) {
				NetworkEventWarning( event, "unknown event" );
			}
		}

		entityNetEvent_t* freedEvent = eventQueue.Dequeue();
		assert( freedEvent == event );
		eventQueue.Free( event );
	}
}

/*
================
idGameLocal::ClientProcessReliableMessage
================
*/
void idGameLocal::ClientProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	int			id, line;
	idPlayer	*p;
	idDict		backupSI;

	InitLocalClient( clientNum );

	id = msg.ReadByte();
	switch( id ) {
		case GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP: {
			InitClientDeclRemap( clientNum );
			break;
		}
		case GAME_RELIABLE_MESSAGE_REMAP_DECL: {
			int type, index;
			char name[MAX_STRING_CHARS];

			type = msg.ReadByte();
			index = msg.ReadLong();
			msg.ReadString( name, sizeof( name ) );

			const idDecl *decl = declManager->FindType( (declType_t)type, name, false );
			if ( decl != NULL ) {
				if ( index >= clientDeclRemap[clientNum][type].Num() ) {
					clientDeclRemap[clientNum][type].AssureSize( index + 1, -1 );
				}
				clientDeclRemap[clientNum][type][index] = decl->Index();
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_SPAWN_PLAYER: {
			int client = msg.ReadByte();
			int spawnId = msg.ReadLong();
			if ( !entities[ client ] ) {
				SpawnPlayer( client );
				entities[ client ]->FreeModelDef();
			}
			// fix up the spawnId to match what the server says
			// otherwise there is going to be a bogus delete/new of the client entity in the first ClientReadFromSnapshot
			spawnIds[ client ] = spawnId;
			break;
		}
		case GAME_RELIABLE_MESSAGE_DELETE_ENT: {
			int spawnId = msg.ReadBits( 32 );
			idEntityPtr< idEntity > entPtr;
			if( !entPtr.SetSpawnId( spawnId ) ) {
				break;
			}
			delete entPtr.GetEntity();
			break;
		}
		//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
		case GAME_RELIABLE_MESSAGE_SPECIAL: {
			char name[128];
			char text[8][128];
			msg.ReadString( name, sizeof( name ) );
			serverSpecialMsg_e msgType = (serverSpecialMsg_e)msg.ReadBits(4);
			int numTextPtrs = msg.ReadBits(8);
			for (int i = 0; i < numTextPtrs; i++) {
				msg.ReadString( text[i], sizeof(text[i]) );
			}
			switch (msgType) {
				//HUMANHEAD PCF rww 05/17/06
				case SPECIALMSG_ALREADYRUNNINGMAP:
				//HUMANHEAD END
				case SPECIALMSG_JOINED:
					mpGame.AddChatLine( "%s^0: %s\n", common->GetLanguageDict()->GetString(name), va(common->GetLanguageDict()->GetString(text[0]), text[1]) );
					break;
				//HUMANHEAD PCF rww 05/17/06
				case SPECIALMSG_JUSTONE:
					mpGame.AddChatLine( "%s^0: %s\n", common->GetLanguageDict()->GetString(name), common->GetLanguageDict()->GetString(text[0]) );
					break;
				//HUMANHEAD END
				default:
					break;
			}
			break;
		}
		//HUMANHEAD END
		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT: { // (client should never get a TCHAT though)
			char name[128];
			char text[128];
			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );
			mpGame.AddChatLine( "%s^0: %s\n", name, text );
			break;
		}
		case GAME_RELIABLE_MESSAGE_SOUND_EVENT: {
			snd_evt_t snd_evt = (snd_evt_t)msg.ReadByte();
			mpGame.PlayGlobalSound( -1, snd_evt );
			break;
		}
		case GAME_RELIABLE_MESSAGE_SOUND_INDEX: {
			int index = gameLocal.ClientRemapDecl( DECL_SOUND, msg.ReadLong() );
			if ( index >= 0 && index < declManager->GetNumDecls( DECL_SOUND ) ) {
				const idSoundShader *shader = declManager->SoundByIndex( index );
				mpGame.PlayGlobalSound( -1, SND_COUNT, shader->GetName() );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_DB: {
			idMultiplayerGame::msg_evt_t msg_evt = (idMultiplayerGame::msg_evt_t)msg.ReadByte();
			int parm1, parm2;
			parm1 = msg.ReadByte( );
			parm2 = msg.ReadByte( );
			mpGame.PrintMessageEvent( -1, msg_evt, parm1, parm2 );
			break;
		}
		//HUMANHEAD rww
		case GAME_RELIABLE_MESSAGE_DB_DEATH: {
			idMultiplayerGame::msg_evt_t msg_evt = (idMultiplayerGame::msg_evt_t)msg.ReadByte();
			int parm1, parm2;
			parm1 = msg.ReadByte( );
			parm2 = msg.ReadByte( );
			int inflictorDef = gameLocal.ClientRemapDecl(DECL_ENTITYDEF, msg.ReadBits(gameLocal.entityDefBits));
			mpGame.PrintDeathMessageEvent( -1, msg_evt, parm1, parm2, inflictorDef );
			break;
		}
		//HUMANHEAD END
		case GAME_RELIABLE_MESSAGE_EVENT: {
			entityNetEvent_t *event;

			// allocate new event
			event = eventQueue.Alloc();
			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
			event->entTypeId = msg.ReadBits(idClass::GetTypeNumBits());
#endif //HUMANHEAD END

			event->paramsSize = msg.ReadBits( idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
			if ( event->paramsSize ) {
				if ( event->paramsSize > MAX_EVENT_PARAM_SIZE ) {
					NetworkEventWarning( event, "invalid param size" );
					return;
				}
				msg.ReadByteAlign();
				msg.ReadData( event->paramsBuf, event->paramsSize );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_SERVERINFO: {
			idDict info;
			msg.ReadDeltaDict( info, NULL );
			gameLocal.SetServerInfo( info );
			break;
		}
		case GAME_RELIABLE_MESSAGE_RESTART: {
			MapRestart();
			break;
		}
		case GAME_RELIABLE_MESSAGE_TOURNEYLINE: {
			line = msg.ReadByte( );
			p = static_cast< idPlayer * >( entities[ clientNum ] );
			if ( !p ) {
				break;
			}
			p->tourneyLine = line;
			break;
		}
		case GAME_RELIABLE_MESSAGE_STARTVOTE: {
#if _HH_LOCALIZE_VOTESTRINGS
			char strings[16][MAX_STRING_CHARS]; //string # sent in 4 bits, can't be > 16
			const char *strPtr[16];
			int clientNum = msg.ReadByte( );
			int strType = msg.ReadBits(6);
			int numStrings = msg.ReadBits(4);
			for (int i = 0; i < numStrings; i++) {
				msg.ReadString( strings[i], sizeof( strings[i] ) );
				strPtr[i] = &strings[i][0];
			}
			mpGame.ClientStartVote( clientNum, (idMultiplayerGame::voteStringType_e)strType, numStrings, strPtr );
#else
			char voteString[ MAX_STRING_CHARS ];
			int clientNum = msg.ReadByte( );
			msg.ReadString( voteString, sizeof( voteString ) );
			mpGame.ClientStartVote( clientNum, voteString );
#endif
			break;
		}
		case GAME_RELIABLE_MESSAGE_UPDATEVOTE: {
			int result = msg.ReadByte( );
			int yesCount = msg.ReadByte( );
			int noCount = msg.ReadByte( );
			mpGame.ClientUpdateVote( (idMultiplayerGame::vote_result_t)result, yesCount, noCount );
			break;
		}
		case GAME_RELIABLE_MESSAGE_PORTALSTATES: {
			int numPortals = msg.ReadLong();
			assert( numPortals == gameRenderWorld->NumPortals() );
			for ( int i = 0; i < numPortals; i++ ) {
				gameRenderWorld->SetPortalState( (qhandle_t) (i+1), msg.ReadBits( NUM_RENDER_PORTAL_BITS ) );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_PORTAL: {
			qhandle_t portal = msg.ReadLong();
			int blockingBits = msg.ReadBits( NUM_RENDER_PORTAL_BITS );
			assert( portal > 0 && portal <= gameRenderWorld->NumPortals() );
			gameRenderWorld->SetPortalState( portal, blockingBits );
			break;
		}
		case GAME_RELIABLE_MESSAGE_STARTSTATE: {
			mpGame.ClientReadStartState( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_WARMUPTIME: {
			mpGame.ClientReadWarmupTime( msg );
			break;
		}
		default: {
			Error( "Unknown server->client reliable message: %d", id );
			break;
		}
	}
}

/*
================
idGameLocal::ClientPrediction
================
*/
gameReturn_t idGameLocal::ClientPrediction( int clientNum, const usercmd_t *clientCmds ) {
	PROFILE_SCOPE("ClientPrediction", PROFMASK_NORMAL); //HUMANHEAD rww

	idEntity *ent;
	idPlayer *player;
	gameReturn_t ret;

	ret.sessionCommand[ 0 ] = '\0';

	player = static_cast<idPlayer *>( entities[clientNum] );
	if ( !player ) {
		return ret;
	}

	// check for local client lag
	//HUMANHEAD rww
	if (player->IsType(hhArtificialPlayer::Type)) {
		player->isLagged = false;
	}
	else {
	//HUMANHEAD END
		if ( networkSystem->ClientGetTimeSinceLastPacket() >= net_clientMaxPrediction.GetInteger() ) {
			player->isLagged = true;
		} else {
			player->isLagged = false;
		}
	}

	InitLocalClient( clientNum );

	// update the game time
	framenum++;
	previousTime = time;
	time += msec;
	timeRandom = time; //HUMANHEAD rww

	// update the real client time and the new frame flag
	if ( time > realClientTime ) {
		realClientTime = time;
		isNewFrame = true;
	} else {
		isNewFrame = false;
	}

	// set the user commands for this frame
	NET_MEMCPY( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) ); //HUMANHEAD rww - testing out performance for simd memcpy

	//HUMANHEAD rww - do this for portals
	SetupPlayerPVS();
	//HUMANHEAD END

	//HUMANHEAD rww - we have to sort the snapshot entities for their thinking order, because we do more complex
	//things than doom3 did with binding objects.
	//rwwFIXME must come up with a way of only setting this bool when we need to resort the list!
	sortSnapshotTeamMasters = true;
	SortSnapshotEntityList();

	// run prediction on all entities from the last snapshot
	for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {
		ent->thinkFlags |= TH_PHYSICS;
		ent->ClientPredictionThink();
	}

	//HUMANHEAD rww - client think entities, for local fx and other things that really don't need to get sent over the net.
	if (isNewFrame) {
		for (ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next()) {
			if (ent->fl.clientEntity) {
				ent->thinkFlags |= TH_PHYSICS;
				ent->ClientPredictionThink();
			}
		}
	}
	//HUMANHEAD END

	// service any pending events
	idEvent::ServiceEvents();

	//HUMANHEAD rww - do this for portals
	FreePlayerPVS();
	//HUMANHEAD END

	// show any debug info for this frame
	if ( isNewFrame ) {
		RunDebugInfo();
		D_DrawDebugLines();
	}

	if ( sessionCommand.Length() ) {
		strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
	}

	//HUMANHEAD rww
	if (logitechLCDEnabled) {
		PROFILE_START("LogitechLCDUpdate", PROFMASK_NORMAL);
		LogitechLCDUpdate();
		PROFILE_STOP("LogitechLCDUpdate", PROFMASK_NORMAL);
	}
	//HUMANHEAD END

	return ret;
}

//HUMANHEAD rww
/*
===============
idGameLocal::ServerAddUnreliableSnapMessage
add an unreliable message into the queue to be appended to the next snapshot.
===============
*/
void idGameLocal::ServerAddUnreliableSnapMessage(int clientNum, const idBitMsg &msg) {
	unreliableSnapMsg[clientNum].Add(msg.GetData(), msg.GetSize());
}

/*
===============
idGameLocal::ClientReadUnreliableSnapMessages
read unreliable messages from the snapshot
===============
*/
void idGameLocal::ClientReadUnreliableSnapMessages(int clientNum, const idBitMsg &msg) {
	idMsgQueue	localQueue;
	idBitMsg	localMsg;
	int			size;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	localQueue.ReadFromMsg( msg );

	localMsg.Init( msgBuf, sizeof( msgBuf ) );
	while ( localQueue.GetDirect( localMsg.GetData(), size ) ) {
		localMsg.SetSize( size );
		localMsg.BeginReading();
		ClientProcessReliableMessage(clientNum, localMsg); //feed them in as reliable messages.
		localMsg.BeginWriting();
	}
}
//HUMANHEAD END

/*
===============
idGameLocal::Tokenize
===============
*/
void idGameLocal::Tokenize( idStrList &out, const char *in ) {
	char buf[ MAX_STRING_CHARS ];
	char *token, *next;
	
	idStr::Copynz( buf, in, MAX_STRING_CHARS );
	token = buf;
	next = strchr( token, ';' );
	while ( token ) {
		if ( next ) {
			*next = '\0';
		}
		idStr::ToLower( token );
		out.Append( token );
		if ( next ) {
			token = next + 1;
			next = strchr( token, ';' );
		} else {
			token = NULL;
		}		
	}
}

/*
===============
idGameLocal::DownloadRequest
===============
*/
bool idGameLocal::DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] ) {
	if ( !cvarSystem->GetCVarInteger( "net_serverDownload" ) ) {
		return false;
	}
	if ( cvarSystem->GetCVarInteger( "net_serverDownload" ) == 1 ) {
		// 1: single URL redirect
		if ( !strlen( cvarSystem->GetCVarString( "si_serverURL" ) ) ) {
			common->Warning( "si_serverURL not set" );
			return false;
		}
		idStr::snPrintf( urls, MAX_STRING_CHARS, "1;%s", cvarSystem->GetCVarString( "si_serverURL" ) );
		return true;
	} else {
		// 2: table of pak URLs
		// first token is the game pak if request, empty if not requested by the client
		// there may be empty tokens for paks the server couldn't pinpoint - the order matters
		idStr reply = "2;";
		idStrList dlTable, pakList;
		int i, j;

		Tokenize( dlTable, cvarSystem->GetCVarString( "net_serverDlTable" ) );
		Tokenize( pakList, paks );

		for ( i = 0; i < pakList.Num(); i++ ) {
			if ( i > 0 ) {
				reply += ";";
			}
			if ( pakList[ i ][ 0 ] == '\0' ) {
 				if ( i == 0 ) {
					// pak 0 will always miss when client doesn't ask for game bin
					common->DPrintf( "no game pak request\n" );
				} else {
					common->DPrintf( "no pak %d\n", i );
				}
				continue;
			}
			for ( j = 0; j < dlTable.Num(); j++ ) {
				if ( !fileSystem->FilenameCompare( pakList[ i ], dlTable[ j ] ) ) {
					break;
				}
			}
			if ( j == dlTable.Num() ) {
				common->Printf( "download for %s: pak not matched: %s\n", IP, pakList[ i ].c_str() );
			} else {
				idStr url = cvarSystem->GetCVarString( "net_serverDlBaseURL" );
				url.AppendPath( dlTable[ j ] );
				reply += url;
				common->DPrintf( "download for %s: %s\n", IP, url.c_str() );
			}
		}
		
		idStr::Copynz( urls, reply, MAX_STRING_CHARS );
		return true;
	}
	return false;
}

/*
===============
idEventQueue::Alloc
===============
*/
entityNetEvent_t* idEventQueue::Alloc() {
	entityNetEvent_t* event = eventAllocator.Alloc();
	event->prev = NULL;
	event->next = NULL;
	return event;
}

/*
===============
idEventQueue::Free
===============
*/
void idEventQueue::Free( entityNetEvent_t *event ) {
	// should only be called on an unlinked event!
	assert( !event->next && !event->prev );
	eventAllocator.Free( event );
}

/*
===============
idEventQueue::Shutdown
===============
*/
void idEventQueue::Shutdown() {
	eventAllocator.Shutdown();
	this->Init();
}

/*
===============
idEventQueue::Init
===============
*/
void idEventQueue::Init( void ) {
	start = NULL;
	end = NULL;
}

/*
===============
idEventQueue::Dequeue
===============
*/
entityNetEvent_t* idEventQueue::Dequeue( void ) {
	entityNetEvent_t* event = start;
	if ( !event ) {
		return NULL;
	}

	start = start->next;

	if ( !start ) {
		end = NULL;
	} else {
		start->prev = NULL;
	}

	event->next = NULL;
	event->prev = NULL;

	return event;
}

/*
===============
idEventQueue::RemoveLast
===============
*/
entityNetEvent_t* idEventQueue::RemoveLast( void ) {
	entityNetEvent_t *event = end;
	if ( !event ) {
		return NULL;
	}

	end = event->prev;

	if ( !end ) {
		start = NULL;
	} else {
		end->next = NULL;		
	}

	event->next = NULL;
	event->prev = NULL;

	return event;
}

/*
===============
idEventQueue::Enqueue
===============
*/
void idEventQueue::Enqueue( entityNetEvent_t *event, outOfOrderBehaviour_t behaviour ) {
	if ( behaviour == OUTOFORDER_DROP ) {
		// go backwards through the queue and determine if there are
		// any out-of-order events
		while ( end && end->time > event->time ) {
			entityNetEvent_t *outOfOrder = RemoveLast();
			common->DPrintf( "WARNING: new event with id %d ( time %d ) caused removal of event with id %d ( time %d ), game time = %d.\n", event->event, event->time, outOfOrder->event, outOfOrder->time, gameLocal.time );
			Free( outOfOrder );
		}
	} else if ( behaviour == OUTOFORDER_SORT && end ) {
		// NOT TESTED -- sorting out of order packets hasn't been
		//				 tested yet... wasn't strictly necessary for
		//				 the patch fix.
		entityNetEvent_t *cur = end;
		// iterate until we find a time < the new event's
		while ( cur && cur->time > event->time ) {
			cur = cur->prev;
		}
		if ( !cur ) {
			// add to start
			event->next = start;
			event->prev = NULL;
			start = event;
		} else {
			// insert
			event->prev = cur;
			event->next = cur->next;
			cur->next = event;
		}
		return;
	} 

	// add the new event
	event->next = NULL;
	event->prev = NULL;

	if ( end ) {
		end->next = event;
		event->prev = end;
	} else {
		start = event;
	}
	end = event;
}
