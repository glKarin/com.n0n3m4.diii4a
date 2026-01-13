/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "framework/FileSystem.h"
#include "framework/async/NetworkSystem.h"
#include "renderer/RenderSystem.h"

#include "gamesys/SysCmds.h"
#include "Entity.h"
#include "Player.h"

#include "Sound.h"
#include "Mover.h"
#include "Misc.h" 
#include "Trigger.h"
#include "gamesys/SysCvar.h" //added for netcode optimization stuff
#include "Camera.h"
#include "ai/AI.h"
#include "WorldSpawn.h"

#include "Game_local.h"

/*
===============================================================================

	Client running game code:
	- entity events don't work and should not be issued
	- entities should never be spawned outside idGameLocal::ClientReadSnapshot

===============================================================================
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
idCVar net_clientLagOMeter( "net_clientLagOMeter", "1", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_ARCHIVE, "draw prediction graph" );
idCVar net_clientCoopDebug( "net_clientCoopDebug", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_ARCHIVE, "TMP Cvar for debug" );
idCVar net_clientShowSnapshotType( "net_clientShowSnapshotType", "0", CVAR_GAME | CVAR_INTEGER, "0: All entities, 1: Mover entities, 2: Static Entties, 3:Team master only", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3>);

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
	if ( (entities[clientNum] == NULL && !mpGame.IsGametypeCoopBased()) || (coopentities[clientNum] == NULL && mpGame.IsGametypeCoopBased())) {
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
	outMsg.WriteInt( index );
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

	//add some assert here to debug a crash by: ERROR: client received unmapped sound decl index 2314 from server
	//assert( !((clientDeclRemap[localClientNum][(int)type].Num() == 0) || ( index >= clientDeclRemap[localClientNum][(int)type].Num() ) || ( clientDeclRemap[localClientNum][(int)type][index] == -1 ))  ); 
	
	// make sure the index is valid
	if ( clientDeclRemap[localClientNum][(int)type].Num() == 0 ) {
		gameLocal.Error( "client received decl index %d before %s decl remap was initialized", index, declManager->GetDeclNameFromType( type ) );
		return -1;
	}
	if ( index >= clientDeclRemap[localClientNum][(int)type].Num() ) {
		if (mpGame.IsGametypeCoopBased()) { //fixme later, avoid crash in coop
			gameLocal.Warning( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		} else {
			gameLocal.Error( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		}
		return -1;
	}
	if ( clientDeclRemap[localClientNum][(int)type][index] == -1 ) {
		if (mpGame.IsGametypeCoopBased()) { //fixme later, avoid crash in coop
			gameLocal.Warning( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		} else {
			gameLocal.Error( "client received unmapped %s decl index %d from server", declManager->GetDeclNameFromType( type ), index );
		}
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
void idGameLocal::ServerClientConnect( int clientNum, const char *guid ) {
	// make sure no parasite entity is left
	if (mpGame.IsGametypeCoopBased()) {
		if ( coopentities[ clientNum ] ) {
			common->DPrintf( "ServerClientConnect: remove old player entity\n" );
			delete coopentities[ clientNum ];
		}
	} else {
		if ( entities[ clientNum ] ) {
			common->DPrintf( "ServerClientConnect: remove old player entity\n" );
			delete entities[ clientNum ];
		}
	}
	//COOP: Reset entity snapshot priority info
	int i;
	for (i = 0; i < MAX_GENTITIES; i++) {
		if (mpGame.IsGametypeCoopBased() && coopentities[i]) {
			coopentities[i]->firstTimeInClientPVS[clientNum] = true;
		}
	}

	userInfo[ clientNum ].Clear();
	mpGame.ServerClientConnect( clientNum );
	//sync cvars here

	Printf( "client %d connected.\n", clientNum );
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

	if (mpGame.IsGametypeCoopBased()) {
		outMsg.WriteInt( coopIds[ clientNum ] );
	} 
	outMsg.WriteInt( spawnIds[ clientNum ] );
	
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
	if (mpGame.IsGametypeCoopBased()) {
		outMsg.WriteBits((coopIds[clientNum] << GENTITYNUM_BITS) | clientNum, 32);
	}
	outMsg.WriteBits( ( spawnIds[ clientNum ] << GENTITYNUM_BITS ) | clientNum, 32 ); // see GetSpawnId
	
	networkSystem->ServerSendReliableMessage( -1, outMsg );

	// free snapshots stored for this client
	FreeSnapshotsOlderThanSequence( clientNum, 0x7FFFFFFF );

	// free entity states stored for this client
	for ( i = 0; i < MAX_GENTITIES; i++ ) {

		if (mpGame.IsGametypeCoopBased() && coopentities[i]) { //COOP: Reset entity snapshot priority info
			coopentities[i]->firstTimeInClientPVS[clientNum] = true;
			coopentities[i]->inSnapshotQueue[clientNum] = 0;
			coopentities[i]->snapshotMissingCount[clientNum] = 0; 
		}

		if ( clientEntityStates[ clientNum ][ i ] ) {
			entityStateAllocator.Free( clientEntityStates[ clientNum ][ i ] );
			clientEntityStates[ clientNum ][ i ] = NULL;
		}
	}

	// clear the client PVS
	memset( clientPVS[ clientNum ], 0, sizeof( clientPVS[ clientNum ] ) );

	// delete the player entity
	if (mpGame.IsGametypeCoopBased()) {
		delete coopentities[ clientNum ];
	} else {
		delete entities[ clientNum ];
	}
	

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

	// dhewm3: Client-Server SDK Check

	outMsg.Init(msgBuf, sizeof(msgBuf));
	outMsg.BeginWriting();
	outMsg.WriteByte(GAME_RELIABLE_MESSAGE_SDK_CHECK);
	outMsg.WriteString(g_mod_version.GetString());
	networkSystem->ServerSendReliableMessage(clientNum, outMsg);

	// spawn players
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( entities[i] == NULL || i == clientNum ) {
			continue;
		}
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting( );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPAWN_PLAYER );
		outMsg.WriteByte( i );

		if (mpGame.IsGametypeCoopBased()) {
			outMsg.WriteInt( coopIds[ i ] );
		}
		outMsg.WriteInt( spawnIds[ i ] );
		
		networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	}

	// send all saved events
	for ( event = savedEventQueue.Start(); event; event = event->next ) {

		if ((serverEventsCount >= MAX_SERVER_EVENTS_PER_FRAME) && gameLocal.mpGame.IsGametypeCoopBased()) {
			addToServerEventOverFlowList(event, clientNum); //Avoid serverSendEvent overflow in coop
			continue;
		}

		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
		if (mpGame.IsGametypeCoopBased()) {
			outMsg.WriteBits( event->coopId, 32 );
			outMsg.WriteBits( event->spawnId, 32 ); 
		} else {
			outMsg.WriteBits( event->spawnId, 32 );
		}
		
		outMsg.WriteByte( event->event );
		outMsg.WriteInt( event->time );
		outMsg.WriteBits( event->paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		if ( event->paramsSize ) {
			outMsg.WriteData( event->paramsBuf, event->paramsSize );
		}

		networkSystem->ServerSendReliableMessage( clientNum, outMsg );

		serverEventsCount++; //added for coop to avoid server Reliable Message overflow
	}

	// update portals for opened doors
	int numPortals = gameRenderWorld->NumPortals();
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_PORTALSTATES );
	outMsg.WriteInt( numPortals );
	for ( i = 0; i < numPortals; i++ ) {
		outMsg.WriteBits( gameRenderWorld->GetPortalState( (qhandle_t) (i+1) ) , NUM_RENDER_PORTAL_BITS );
	}
	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// Sync deleted map entities with client (NEW COOP)
	if (gameLocal.mpGame.IsGametypeCoopBased()) {
		outMsg.Init(msgBuf, sizeof(msgBuf));
		outMsg.BeginWriting();
		outMsg.WriteByte(GAME_RELIABLE_MESSAGE_REMOVED_ENTITIES);
		WriteRemovedEntitiesToEvent(outMsg);

		networkSystem->ServerSendReliableMessage(clientNum, outMsg);
	}

	mpGame.ServerWriteInitialReliableMessages( clientNum );
}

/*
================
idGameLocal::SaveEntityNetworkEvent
================
*/
void idGameLocal::SaveEntityNetworkEvent( const idEntity *ent, int eventId, const idBitMsg *msg , bool saveLastOnly) {
	entityNetEvent_t *event;

	if (mpGame.IsGametypeCoopBased() && saveLastOnly) { //used in  coop to avoid overflow of saved events in some entities
		// send all saved events
		int eventCoopId = GetCoopId( ent );
		int eventSpawnId = GetSpawnId( ent );
		for ( event = savedEventQueue.Start(); event; event = event->next ) {
			if ((event->coopId != eventCoopId) || (event->spawnId != eventSpawnId)) {
				continue;
			}
			SetEntityNetEventData(event, eventId, msg, time);
			return;
		}
		event = NULL; //stradex: is this necessary of I'm just retarded?
	}

	event = savedEventQueue.Alloc();
	
	if (mpGame.IsGametypeCoopBased()) {
		event->coopId = GetCoopId( ent );
		event->spawnId = GetSpawnId( ent );
	} else {
		event->spawnId = GetSpawnId( ent );
	}
	SetEntityNetEventData(event, eventId, msg, time);

	savedEventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );
}

/*
================
idGameLocal::SetEntityNetEventData
================
*/
void idGameLocal::SetEntityNetEventData(entityNetEvent_t* event, int eventId, const idBitMsg* msg, int eventTime) {
	event->event = eventId;
	event->time = eventTime;
	if (msg) {
		event->paramsSize = msg->GetSize();
		memcpy(event->paramsBuf, msg->GetData(), msg->GetSize());
	}
	else {
		event->paramsSize = 0;
	}
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
				if (mpGame.IsGametypeCoopBased()) { //testing new netcode sync for coop
					if ( clientEntityStates[clientNum][state->entityCoopNumber] ) {
						entityStateAllocator.Free( clientEntityStates[clientNum][state->entityCoopNumber] );
					}
					clientEntityStates[clientNum][state->entityCoopNumber] = state;
				} else {
					if ( clientEntityStates[clientNum][state->entityNumber] ) {
						entityStateAllocator.Free( clientEntityStates[clientNum][state->entityNumber] );
					}
					clientEntityStates[clientNum][state->entityNumber] = state;
				}

			}
			memcpy( clientPVS[clientNum], snapshot->pvs, sizeof( snapshot->pvs ) );
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

	if (mpGame.IsGametypeCoopBased()) { //added for COOP
		return ServerWriteSnapshotCoop(clientNum, sequence, msg, clientInPVS, numPVSClients);
	}

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
	msg.WriteInt( tagRandom.GetSeed() );
#endif

	// create the snapshot
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		//Clientside projectiles start
		if (ent->clientsideNode.InList()) { //Stradex: ignore clientside only entities to avoid weird shit
			continue;
		}

		if (ent->IsType(idProjectile::Type) && gameLocal.userInfo[ clientNum ].GetBool( "net_clientUnlagged", "1")) {
			idEntity* tmpOwner = static_cast<idProjectile*>(ent)->GetOwner();
			if (tmpOwner && tmpOwner->entityNumber == clientNum) { //projectiles created by current player
				continue;
			}

		}
		//Clientside projectiles end

		// if the entity is not in the player PVS
		if ( !ent->PhysicsTeamInPVS( pvsHandle ) && ent->entityNumber != clientNum ) {
			continue;
		}

		// add the entity to the snapshot pvs
		snapshot->pvs[ ent->entityNumber >> 5 ] |= 1 << ( ent->entityNumber & 31 );

		// if that entity is not marked for network synchronization
		if ( !ent->fl.networkSync ) {
			continue;
		}

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

		deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );

		deltaMsg.WriteBits( spawnIds[ ent->entityNumber ], 32 - GENTITYNUM_BITS );
		deltaMsg.WriteBits( ent->GetType()->typeNum, idClass::GetTypeNumBits() );
		deltaMsg.WriteBits( ServerRemapDecl( -1, DECL_ENTITYDEF, ent->entityDefNumber ), entityDefBits );

		// write the class specific data to the snapshot
		ent->WriteToSnapshot( deltaMsg );

		if ( !deltaMsg.HasChanged() ) {
			msg.RestoreWriteState( msgSize, msgWriteBit );
			entityStateAllocator.Free( newBase );
		} else {
			newBase->next = snapshot->firstEntityState;
			snapshot->firstEntityState = newBase;

#if ASYNC_WRITE_TAGS
			msg.WriteInt( tagRandom.RandomInt() );
#endif
		}
	}

	msg.WriteBits( ENTITYNUM_NONE, GENTITYNUM_BITS );

	// write the PVS to the snapshot
#if ASYNC_WRITE_PVS
	for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
		if ( i < numSourceAreas ) {
			msg.WriteInt( sourceAreas[ i ] );
		} else {
			msg.WriteInt( 0 );
		}
	}
	gameLocal.pvs.WritePVS( pvsHandle, msg );
#endif
	for ( i = 0; i < ENTITY_PVS_SIZE; i++ ) {
		msg.WriteDeltaInt( clientPVS[clientNum][i], snapshot->pvs[i] );
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
	memcpy( clientInPVS, snapshot->pvs, ( numPVSClients + 7 ) >> 3 );
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

	int entityNum	= event->spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	int id			= event->spawnId >> GENTITYNUM_BITS;

	length += idStr::snPrintf( buf+length, sizeof(buf)-1-length, "event %d for entity %d %d: ", event->event, entityNum, id );
	va_start( argptr, fmt );
	length = idStr::vsnPrintf( buf+length, sizeof(buf)-1-length, fmt, argptr );
	va_end( argptr );
	idStr::Append( buf, sizeof(buf), "\n" );

	common->DWarning( buf );
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

		if (mpGame.IsGametypeCoopBased() && (event->coopId >= 0)) {
			if( !entPtr.SetCoopId( event->coopId ) ) {
				NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
			}  else {
				ent = entPtr.GetCoopEntity();
				ProcessEntityReceiveEvent(ent, eventMsg, event, false);
			}
		} else {
			if( !entPtr.SetSpawnId( event->spawnId ) ) {
				NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
			} else {
				ent = entPtr.GetEntity();
				ProcessEntityReceiveEvent(ent, eventMsg, event, false);
			}
		}

		entityNetEvent_t* freedEvent id_attribute((unused)) = eventQueue.Dequeue();
		assert( freedEvent == event );
		eventQueue.Free( event );
	}
}

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
			int index = msg.ReadInt();
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

			if (mpGame.IsGametypeCoopBased()) {
				event->coopId = msg.ReadBits( 32 );
				event->spawnId = msg.ReadBits( 32 );
			} else {
				event->spawnId = msg.ReadBits( 32 );
			}
			
			event->event = msg.ReadByte();
			event->time = msg.ReadInt();

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
		//coop only specific stuff
		case GAME_RELIABLE_MESSAGE_ADDCHECKPOINT: {
			mpGame.WantAddCheckpoint(clientNum);
			break;
		}
		case GAME_RELIABLE_MESSAGE_GOTOCHECKPOINT: {
			mpGame.WantUseCheckpoint(clientNum);
			break;
		}
		case GAME_RELIABLE_MESSAGE_GLOBALCHECKPOINT: {
			mpGame.WantAddCheckpoint(clientNum, true);
			break;
		}
		case GAME_RELIABLE_MESSAGE_NOCLIP: {
			mpGame.WantNoClip(clientNum);
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

	if (mpGame.IsGametypeCoopBased()) {
		player = static_cast<idPlayer *>( coopentities[clientNum] );
	} else {
		player = static_cast<idPlayer *>( entities[clientNum] );
	}

	
	if ( !player ) {
		return;
	}

	viewAxis = player->viewAngles.ToMat3();
	viewBounds = player->GetPhysics()->GetAbsBounds().Expand( net_clientShowSnapshotRadius.GetFloat() );

	for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {

		if ( net_clientShowSnapshot.GetInteger() == 1 && ent->snapshotBits == 0 ) {
			continue;
		}

		if (net_clientShowSnapshotType.GetInteger() == 1 && !ent->IsType(idMover::Type))
			continue;

		if (net_clientShowSnapshotType.GetInteger() == 2 && !ent->IsType(idStaticEntity::Type))
			continue;

		if (net_clientShowSnapshotType.GetInteger() == 3 && ent->GetTeamMaster() != ent) { //testing
			continue;
		}
		const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

		if ( !entBounds.IntersectsBounds( viewBounds ) ) {
			continue;
		}
		if (mpGame.IsGametypeCoopBased()) { //added to test new netcode sync coop
			base = clientEntityStates[clientNum][ent->entityCoopNumber];
		} else {
			base = clientEntityStates[clientNum][ent->entityNumber];
		}
		
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


	if (mpGame.IsGametypeCoopBased()) { //Extra for coop
		return ClientReadSnapshotCoop(clientNum, sequence, gameFrame, gameTime, dupeUsercmds, aheadOfServer, msg); //specific coop method for this to avoid breaking original D3 Netcode
	}

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
	idWeapon		*weap;

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
	tagRandom.SetSeed( msg.ReadInt() );
#endif

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

		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
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
				if ( !SpawnEntityDef( args, &ent ) || !entities[i] || entities[i]->GetType()->typeNum != typeNum ) {
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
		}

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = sequence;

		// read the class specific data from the snapshot
		ent->ReadFromSnapshot( deltaMsg );

		ent->snapshotBits = msg.GetNumBitsRead() - numBitsRead;

#if ASYNC_WRITE_TAGS
		if ( msg.ReadInt() != tagRandom.RandomInt() ) {
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
		serverPVS[ i ] = msg.ReadInt();
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
		snapshot->pvs[i] = msg.ReadDeltaInt( clientPVS[clientNum][i] );
	}

	// add entities in the PVS that haven't changed since the last applied snapshot
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		if (ent->clientsideNode.InList()) { //Stradex: ignore clientside only entities to avoid weird shit
			continue;
		}

		// if the entity is already in the snapshot
		if ( ent->snapshotSequence == sequence ) {
			continue;
		}

		// if the entity is not in the snapshot PVS
		if ( !( snapshot->pvs[ent->entityNumber >> 5] & ( 1 << ( ent->entityNumber & 31 ) ) ) ) {
			if ( ent->PhysicsTeamInPVS( pvsHandle ) ) {
				if ( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < mapSpawnCount ) {
					// server says it's not in PVS, client says it's in PVS
					// if that happens on map entities, most likely something is wrong
					// I can see that moving pieces along several PVS could be a legit situation though
					// this is a band aid, which means something is not done right elsewhere
					common->DWarning( "client thinks map entity 0x%x (%s) is stale, sequence 0x%x", ent->entityNumber, ent->name.c_str(), sequence );
				} else {
					ent->FreeModelDef();
					ent->UpdateVisuals();
					ent->GetPhysics()->UnlinkClip();
				}
			}
			continue;
		}

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

		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
		typeNum = deltaMsg.ReadBits( idClass::GetTypeNumBits() );
		entityDefNumber = deltaMsg.ReadBits( entityDefBits );

		typeInfo = idClass::GetType( typeNum );

		// if the entity is not the right type
		if ( !typeInfo || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber ) {
			// should never happen - it does though. with != entityDefNumber only?
			common->DWarning( "entity '%s' is not the right type %p 0x%d 0x%x 0x%x 0x%x", ent->GetName(), typeInfo, ent->GetType()->typeNum, typeNum, ent->entityDefNumber, entityDefNumber );
			continue;
		}

		// read the class specific data from the base state
		ent->ReadFromSnapshot( deltaMsg );
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

		if (mpGame.IsGametypeCoopBased() && (event->coopId >= 0)) {
			if( !entPtr.SetCoopId( event->coopId ) ) {
				if( !gameLocal.coopentities[ event->coopId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
					// if new entity exists in this position, silently ignore
					NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
				}
			} else {
				ent = entPtr.GetCoopEntity();
				ProcessEntityReceiveEvent(ent, eventMsg, event, true);
			}
		} else {
			if( !entPtr.SetSpawnId( event->spawnId ) ) {
				if( !gameLocal.entities[ event->spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
					NetworkEventWarning( event, "Entity does not exist any longer, or has not been spawned yet." );
				}
			} else {
				ent = entPtr.GetEntity();
				ProcessEntityReceiveEvent(ent, eventMsg, event, true);
			}
		}

		entityNetEvent_t* freedEvent id_attribute((unused)) = eventQueue.Dequeue();
		assert( freedEvent == event );
		eventQueue.Free( event );
	}
}

/*
================
idGameLocal::NetworkProcessReceiveEvent
================
*/
void idGameLocal::ProcessEntityReceiveEvent(idEntity* ent, idBitMsg &eventMsg, entityNetEvent_t* event, bool isClientEvent) {
	if (!ent && mpGame.IsGametypeCoopBased()) { //FIXME: This should never happen, later I should disable this and DEBUG whenever the assert(ent) happens to see what's going on wrong.
		gameLocal.Warning("[COOP] Trying to read unkwown entity\n");
	}
	else {
		assert(ent);

		eventMsg.Init(event->paramsBuf, sizeof(event->paramsBuf));
		eventMsg.SetSize(event->paramsSize);
		eventMsg.BeginReading();
		if (isClientEvent) {
			if (!ent->ClientReceiveEvent(event->event, event->time, eventMsg)) {
				NetworkEventWarning(event, "unknown event");
			}
		} else {
			if (!ent->ServerReceiveEvent(event->event, event->time, eventMsg)) {
				NetworkEventWarning(event, "unknown event");
			}
		}

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
			index = msg.ReadInt();
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
			int coopId, spawnId;
			int client = msg.ReadByte();
			if (mpGame.IsGametypeCoopBased()) { //net netcode sync for coop
				coopId = msg.ReadInt();
				spawnId = msg.ReadInt();
				if ( !coopentities[ client ] ) {
					SpawnPlayer( client );
					entities[ client ]->FreeModelDef();
				}
				coopIds[ client ] = coopId;
				spawnIds[ client ] = spawnId;
			} else {
				spawnId = msg.ReadInt();
				if ( !entities[ client ] ) {
					SpawnPlayer( client );
					entities[ client ]->FreeModelDef();
				}
				spawnIds[ client ] = spawnId;
			}

			// fix up the spawnId to match what the server says
			// otherwise there is going to be a bogus delete/new of the client entity in the first ClientReadFromSnapshot

			break;
		}
		case GAME_RELIABLE_MESSAGE_DELETE_ENT: {
			idEntityPtr< idEntity > entPtr;

			int coopId, spawnId;
			coopId=-1;
			if (mpGame.IsGametypeCoopBased()) {
				coopId = msg.ReadBits(32);
			}
			spawnId = msg.ReadBits( 32 );
			


			if (coopId >= 0) { //testing new netcode sync for coop

				if( !entPtr.SetCoopId( coopId ) ) {
					break;
				}
				delete entPtr.GetCoopEntity();
			} else {

				if( !entPtr.SetSpawnId( spawnId ) ) {
					break;
				}
				delete entPtr.GetEntity();
			}

			break;
		}
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
			int index = gameLocal.ClientRemapDecl( DECL_SOUND, msg.ReadInt() );
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
		case GAME_RELIABLE_MESSAGE_EVENT: {
			entityNetEvent_t *event;

			// allocate new event
			event = eventQueue.Alloc();
			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );

			if (mpGame.IsGametypeCoopBased()) {
				event->coopId = msg.ReadBits( 32 ); //new netcode sync for coop
				event->spawnId = msg.ReadBits( 32 ); //added for coop
			} else {
				event->spawnId = msg.ReadBits( 32 );
			}
			
			event->event = msg.ReadByte();
			event->time = msg.ReadInt();

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
			char voteString[ MAX_STRING_CHARS ];
			int clientNum = msg.ReadByte( );
			msg.ReadString( voteString, sizeof( voteString ) );
			mpGame.ClientStartVote( clientNum, voteString );
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
			int numPortals = msg.ReadInt();
			assert( numPortals == gameRenderWorld->NumPortals() );
			for ( int i = 0; i < numPortals; i++ ) {
				gameRenderWorld->SetPortalState( (qhandle_t) (i+1), msg.ReadBits( NUM_RENDER_PORTAL_BITS ) );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_PORTAL: {
			qhandle_t portal = msg.ReadInt();
			int blockingBits = msg.ReadBits( NUM_RENDER_PORTAL_BITS );
			if (gameLocal.mpGame.IsGametypeCoopBased()) {
				if (portal <= 0 || portal > gameRenderWorld->NumPortals()) {
					gameLocal.Warning("[FATAL] assertion avoided at GAME_RELIABLE_MESSAGE_PORTAL\n");
					return;
				}
			} else {
				assert( portal > 0 && portal <= gameRenderWorld->NumPortals() );
			}
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
		case GAME_RELIABLE_MESSAGE_FADE: {
			idVec4		fadeColor;
			idPlayer*	player;
			idVec3		color;
			float		alpha, fadeTime;
			color[0] = msg.ReadFloat();
			color[1] = msg.ReadFloat();
			color[2] = msg.ReadFloat();
			alpha = msg.ReadFloat();
			fadeTime = msg.ReadFloat();

			player = GetLocalPlayer();
			if (player) {
				fadeColor.Set(color[0], color[1], color[2], alpha);
				player->playerView.Fade(fadeColor, SEC2MS(fadeTime));
			}

			common->Printf("[COOP] Receive fade...\n");
			break;
		}
		case GAME_RELIABLE_MESSAGE_REMOVED_ENTITIES: {
			common->Printf("[COOP] Syncing map entities with server\n");
			ReadRemovedEntitiesFromEvent(msg);
			break;
		}
		case GAME_RELIABLE_MESSAGE_SDK_CHECK: {
			char serverSDK[128];
			msg.ReadString(serverSDK, sizeof(serverSDK));

			if (idStr::Icmp(serverSDK, g_mod_version.GetString())) { //invalid SDK VERSIONS
				Error("Server is using a different mod.\nClient mod: %s - Server mod: %s\n", g_mod_version.GetString(), serverSDK);
			}
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
gameReturn_t idGameLocal::ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame ) {
	idEntity *ent;
	idPlayer *player;
	gameReturn_t ret;

	ret.sessionCommand[ 0 ] = '\0';
	if (mpGame.IsGametypeCoopBased()) {
		player = static_cast<idPlayer *>( coopentities[clientNum] );
	} else {
		player = static_cast<idPlayer *>( entities[clientNum] );
	}
	
	if ( !player ) {
		return ret;
	}

	// check for local client lag
	if ( networkSystem->ClientGetTimeSinceLastPacket() >= net_clientMaxPrediction.GetInteger() ) {
		player->isLagged = true;
	} else {
		player->isLagged = false;
	}

	InitLocalClient( clientNum );

	// update the game time
	framenum++;
	previousTime = time;
	time += msec;

	// update the real client time and the new frame flag
	if ( time > realClientTime ) {
		realClientTime = time;
		isNewFrame = true;
		previousClientsideTime = clientsideTime;
		clientsideTime += msec; //added for clientside movement code and events
	} else {
		isNewFrame = false;
	}

	// set the user commands for this frame
	memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

	// run prediction on all entities from the last snapshot
	if (!mpGame.IsGametypeCoopBased()) { //non-coop original netcode
		if (isNewFrame) {
			for( ent = clientsideEntities.Next(); ent != NULL; ent = ent->clientsideNode.Next() ) {
				ent->thinkFlags |= TH_PHYSICS;
				ent->ClientPredictionThink();
			}
		}
		for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {
			ent->thinkFlags |= TH_PHYSICS;
			ent->ClientPredictionThink();
		}
	} else { //COOP netcode

	if (isNewFrame) {
		RunClientSideFrame(player, clientCmds);
	} else { //old netcode entities (mover entities mostly)
		for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {
			if ((ent->entityCoopNumber == player->entityCoopNumber) || (!ent->IsBoundToMover() && !ent->MasterUseOldNetcode())) { //maybe the entity doesn't use oldnetcode but the masters does so do this shit then
				continue;
			}

			ent->thinkFlags |= TH_PHYSICS;
			ent->ClientPredictionThink();
		}
		for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			if (isSnapshotEntity(ent) || (ent->entityCoopNumber ==  player->entityCoopNumber) || (!ent->IsBoundToMover() && !ent->MasterUseOldNetcode())) {
				continue;
			}

			ent->thinkFlags |= TH_PHYSICS;
			ent->ClientPredictionThink();
		}
	}
		//Predict only local player multiple times per frame if necessary
		player->thinkFlags  |= TH_PHYSICS;
		player->ClientPredictionThink();
	}


	// service any pending events
	idEvent::ServiceEvents();

	// show any debug info for this frame
	if ( isNewFrame ) {
		RunDebugInfo();
		D_DrawDebugLines();
	}

	if ( sessionCommand.Length() ) {
		strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
	}
	return ret;
}

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
	}

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


/*
==========================
SPECIFIC COOP METHODS
==========================
*/

/*
================
idGameLocal::RunClientSideFrame
All specific COOP player clientside logic happens here
================
*/
void	idGameLocal::RunClientSideFrame(idPlayer	*clientPlayer, const usercmd_t *clientCmds )
{
	idEntity *ent;
	int			num;
	clientEventsCount=0; //COOP DEBUG ONLY

	SetupPlayerPVS();

	for( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {
		if (ent->entityCoopNumber == clientPlayer->entityCoopNumber) {
			continue;
		}
		ent->clientSideEntity = false; //this entity is not clientside
		ent->thinkFlags |= TH_PHYSICS;
		if (ent->allowClientsideThink) {
			ent->clientSideEntity = true; //HACK!
			ent->Think();
			ent->clientSideEntity = false; //HACK!
		}
		else {
			ent->ClientPredictionThink();
		}
	}

	SortActiveEntityList();

	//Non-sync clientside think
	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if (isSnapshotEntity(ent)) {
			continue;
		}
		if (ent->entityCoopNumber == clientPlayer->entityCoopNumber) {
			//common->Printf("Ignoring player clientside\n");
			continue;
		}

		if (ent->forceNetworkSync && (ent->snapshotMissingCount[gameLocal.localClientNum] >= MAX_MISSING_SNAPSHOTS)) {
			ent->snapshotMissingCount[gameLocal.localClientNum] = MAX_MISSING_SNAPSHOTS;
			continue; //don't touch these entities here
		}

		if (!ent->fl.coopNetworkSync) {
			ent->clientSideEntity = true; //this entity is now clientside
		}
		ent->thinkFlags |= TH_PHYSICS;
		if (ent->allowClientsideThink && !ent->fl.coopNetworkSync) {
			ent->Think(); //this is maybe a mistake, but who knows
		} else {
			ent->ClientPredictionThink();
		}
		
	}

	// remove any entities that have stopped thinking
	if ( numEntitiesToDeactivate ) {
		idEntity *next_ent;
		int c = 0;
		for( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->activeNode.Next();
			if ( !ent->thinkFlags ) {
				ent->activeNode.Remove();
				c++;
			}
		}
		//assert( numEntitiesToDeactivate == c );
		numEntitiesToDeactivate = 0;
	}

	FreePlayerPVS();

	//COOP DEBUG
	if (clientEventsCount > 10) {
		common->Printf("Client sending events: %d\n", serverEventsCount);
	}
	//END COOP DEBU
}


/*
=============
idGameLocal::isSnapshotEntity
=============
*/

bool idGameLocal::isSnapshotEntity(idEntity* ent){
	return ent->snapshotNode.InList();
}

idEntity* idGameLocal::getEntityBySpawnId(int spawnId){
	idEntity *ent;

	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if (this->GetSpawnId(ent) == spawnId) {
			return ent;
		}
	}

	return NULL;
}


/*
================
idGameLocal::ClientReadSnapshotCoop
================
*/
void idGameLocal::ClientReadSnapshotCoop( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg ) {
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
	int				spawnId, coopId;
	int				numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];
	idWeapon		*weap;

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
	//clientsideTime += msec;

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
	tagRandom.SetSeed( msg.ReadInt() );
#endif

	// read all entities from the snapshot
	for ( i = msg.ReadBits( GENTITYNUM_BITS ); i != ENTITYNUM_NONE; i = msg.ReadBits( GENTITYNUM_BITS ) ) {

		base = clientEntityStates[clientNum][i]; //
		if ( base ) {
			base->state.BeginReading();
		}
		newBase = entityStateAllocator.Alloc();
		newBase->entityNumber = i; //test
		newBase->entityCoopNumber = i;
		
		newBase->next = snapshot->firstEntityState;
		snapshot->firstEntityState = newBase;
		newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
		newBase->state.BeginWriting();

		numBitsRead = msg.GetNumBitsRead();

		deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );

		coopId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS ); //test new netsync for coop
		
		typeNum = deltaMsg.ReadBits( idClass::GetTypeNumBits() );
		entityDefNumber = ClientRemapDecl( DECL_ENTITYDEF, deltaMsg.ReadBits( entityDefBits ) );

		typeInfo = idClass::GetType( typeNum );
		if ( !typeInfo ) {
			Error( "Unknown type number %d for entity %d with class number %d", typeNum, i, entityDefNumber );
		}

		ent = coopentities[i];
		
		
		// if there is no entity or an entity of the wrong type
		if ( !ent || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber || coopId != coopIds[ i ]) {

			if ( i < MAX_CLIENTS && ent ) {
				// SPAWN_PLAYER should be taking care of spawning the entity with the right spawnId
				common->Warning( "ClientReadSnapshot: recycling client entity %d\n", i );
			}

			delete ent;

			coopCount = coopId;

			args.Clear();
			args.SetInt( "coop_entnum", i );
			args.Set( "name", va( "entity%d", i ) );

			if ( entityDefNumber >= 0 ) {
				if ( entityDefNumber >= declManager->GetNumDecls( DECL_ENTITYDEF ) ) {
					Error( "server has %d entityDefs instead of %d", entityDefNumber, declManager->GetNumDecls( DECL_ENTITYDEF ) );
				}
				classname = declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
				args.Set( "classname", classname );
				if ( !SpawnEntityDef( args, &ent, true, true ) || !coopentities[i] || coopentities[i]->GetType()->typeNum != typeNum ) {
					Error( "Failed to spawn entity with classname '%s' of type '%s'", classname, typeInfo->classname );
				}
			} else {
				ent = SpawnEntityType( *typeInfo, &args, true );
				if ( !coopentities[i] || coopentities[i]->GetType()->typeNum != typeNum ) {
					Error( "Failed to spawn entity of type '%s'", typeInfo->classname );
				}
			}
			
			if ( i < MAX_CLIENTS && i >= numClients ) {
				numClients = i + 1;
			}

			ent->spawnedByServer = true; //Added  by Stradex for Coop
		}

		ent->snapshotMissingCount[clientNum] = 0;  //Added  by Stradex for Coop

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = sequence;

		// read the class specific data from the snapshot
		ent->ReadFromSnapshot( deltaMsg );

		ent->snapshotBits = msg.GetNumBitsRead() - numBitsRead;

#if ASYNC_WRITE_TAGS
		if ( msg.ReadInt() != tagRandom.RandomInt() ) {
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

	player = static_cast<idPlayer *>( coopentities[clientNum] );
	
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
		serverPVS[ i ] = msg.ReadInt();
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
		snapshot->pvs[i] = msg.ReadDeltaInt( clientPVS[clientNum][i] );
	}

	// add entities in the PVS that haven't changed since the last applied snapshot
	//for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
	for( ent = coopSyncEntities.Next(); ent != NULL; ent = ent->coopNode.Next() ) { //test for coop

		if (ent->clientsideNode.InList()) { //Stradex: ignore clientside only entities to avoid weird shit
			continue;
		}

		// if the entity is already in the snapshot
		if (!ent->forceNetworkSync) { //avoid crash related to idEntity::FindTargets coop entities
			continue; //is this really necessary for coop?
		}
		if ( ent->snapshotSequence == sequence ) {
			continue;
		}
		if (!ent->fl.coopNetworkSync) {
			continue; //ignore client-side entities in coop (IMPORTANT TO AVOID CRASHES IN COOP)
		}

		// if the entity is not in the snapshot PVS
		//is this really necessary?
		
		if ( !( snapshot->pvs[ent->entityNumber >> 5] & ( 1 << ( ent->entityNumber & 31 ) ) ) ) {
			if ( ent->PhysicsTeamInPVS( pvsHandle ) ) { //causing a fatal crash in COOP
				if ( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < mapSpawnCount ) {
					// server says it's not in PVS, client says it's in PVS
					// if that happens on map entities, most likely something is wrong
					// I can see that moving pieces along several PVS could be a legit situation though
					// this is a band aid, which means something is not done right elsewhere
					common->DWarning( "client thinks map entity 0x%x (%s) is stale, sequence 0x%x", ent->entityNumber, ent->name.c_str(), sequence );
					if (clientEntityStates[clientNum][ent->entityNumber]) {
						ent->Hide();
					}
				} else {
					ent->FreeModelDef();
					ent->UpdateVisuals();
					ent->GetPhysics()->UnlinkClip();
				}
			}
			continue;
		}
		

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = sequence;
		ent->snapshotBits = 0;

		base = clientEntityStates[clientNum][ent->entityCoopNumber]; //added to test new netcode sync for coop
		
		if ( !base ) {
			// entity has probably fl.networkSync set to false
			continue;
		}

		base->state.BeginReading();

		deltaMsg.Init( &base->state, NULL, (const idBitMsg *)NULL );

		if (mpGame.IsGametypeCoopBased()) {
			coopId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS ); //new netcode sync for coop
		} else {
			spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
		}
		
		typeNum = deltaMsg.ReadBits( idClass::GetTypeNumBits() );
		entityDefNumber = deltaMsg.ReadBits( entityDefBits );

		typeInfo = idClass::GetType( typeNum );

		// if the entity is not the right type
		if ( !typeInfo || ent->GetType()->typeNum != typeNum || ent->entityDefNumber != entityDefNumber ) {
			// should never happen - it does though. with != entityDefNumber only?
			common->DWarning( "entity '%s' is not the right type %p 0x%d 0x%x 0x%x 0x%x", ent->GetName(), typeInfo, ent->GetType()->typeNum, typeNum, ent->entityDefNumber, entityDefNumber );
			continue;
		}

		// read the class specific data from the base state
		ent->ReadFromSnapshot( deltaMsg );
	}

	// free the PVS
	pvs.FreeCurrentPVS( pvsHandle );

	//Read the cinematic data from snapshot coop
	bool serverIsInCinematic = (msg.ReadBits(1) == 1); //not used yet
	bool serverWorldNoWeapons = (msg.ReadBits(1) == 1);
	gameLocal.world->spawnArgs.SetBool("no_Weapons", serverWorldNoWeapons);
	int serverCameraEntityNumber = msg.ReadShort();
	int currentCameraEntityNumber = -1;
	if (gameLocal.GetCamera()) {
		currentCameraEntityNumber = GetCamera()->entityNumber;
	}
	if (currentCameraEntityNumber != serverCameraEntityNumber) {
		if (serverCameraEntityNumber >= MAX_CLIENTS && gameLocal.entities[serverCameraEntityNumber]->IsType(idCamera::Type)) {
			SetClientCamera(static_cast<idCamera*>(gameLocal.entities[serverCameraEntityNumber]));
		}
		else {
			SetClientCamera(NULL);
		}
	}

	// read the game and player state from the snapshot
	base = clientEntityStates[clientNum][ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game and player state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->entityCoopNumber = ENTITYNUM_NONE; //added for coop
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

	//extra by stradex for coop optimization
	for( ent = coopSyncEntities.Next(); ent != NULL; ent = ent->coopNode.Next() ) { //test for coop
		if (!ent->snapshotNode.InList()) {
			ent->snapshotMissingCount[clientNum]++; //you're not part of the snapshot so let the magic begin
		}
	}
}

// swap elements in array
void idGameLocal::snapshotsort_swap(idEntity* entities[], int lhs, int rhs) {
	idEntity* tmp;
	tmp = entities[lhs];
	entities[lhs] = entities[rhs];
	entities[rhs] = tmp;
	tmp = NULL;
};

// entities in snapshot queue <-- lower snapshot priority <-- first time in PVS <-- everything else
bool idGameLocal::snapshotsort_notInOrder(const snapshotsort_context_s &context, idEntity* lhs, idEntity* rhs) {

	//1 - inRemoteCameraPVS: optimization for idSecurityCamera
	if (!lhs->inRemoteCameraPVS[context.clientNum] && rhs->inRemoteCameraPVS[context.clientNum]) {
		return true;
	}
	else if (lhs->inRemoteCameraPVS[context.clientNum] && !rhs->inRemoteCameraPVS[context.clientNum]) {
		return false;
	}

	// 2 - elements in snapshot queue should be left
	if (lhs->inSnapshotQueue[context.clientNum] < rhs->inSnapshotQueue[context.clientNum]) {
		return true;
	} else if (lhs->inSnapshotQueue[context.clientNum] > rhs->inSnapshotQueue[context.clientNum]) {
		return false;
	}

	// either both are in snapshot queue or both are not in snapshot queue
	// 3 - lower priority should be left
	if (lhs->snapshotPriority > rhs->snapshotPriority) {
		return true;
	} else if (lhs->snapshotPriority < rhs->snapshotPriority) {
		return false;
	}

	// both are same priority
	// 4 - first time in PVS should be left
	if (!lhs->firstTimeInClientPVS[context.clientNum] && rhs->firstTimeInClientPVS[context.clientNum]) {
		return true;
	}

	// either left or both are in client PVS for first time
	return false;
}

// partition for quicksort with median-of-three pivot selection
int idGameLocal::snapshotsort_partition(const snapshotsort_context_s &context, idEntity* entities[], int low, int high) {
	int mid = idMath::Rint((low + high) / 2);
	if (snapshotsort_notInOrder(context, entities[low], entities[mid])) {
		snapshotsort_swap(entities, low, mid);
	}
	if (snapshotsort_notInOrder(context, entities[low], entities[high])) {
		snapshotsort_swap(entities, low, high);
	}
	if (snapshotsort_notInOrder(context, entities[high], entities[mid])) {
		snapshotsort_swap(entities, high, mid);
	}
	idEntity* pivot = entities[high];
	int i = low;
	for (int j = low; j < high; j++) {
		if (snapshotsort_notInOrder(context, pivot, entities[j])) {
			snapshotsort_swap(entities, i, j);
			i++;
		}
	}
	snapshotsort_swap(entities, i, high);
	return i;
};

// recursive quicksort
void idGameLocal::snapshotsort(const snapshotsort_context_s &context, idEntity* entities[], int low, int high) {
	if (low < high) {
		int p = snapshotsort_partition(context, entities, low, high);
		snapshotsort(context, entities, low, p - 1);
		snapshotsort(context, entities, p + 1, high);
	}
};

/*
================
idGameLocal::ServerWriteSnapshotCoop

  Write a snapshot of the current game state for the given client.
================
*/
void idGameLocal::ServerWriteSnapshotCoop(int clientNum, int sequence, idBitMsg& msg, byte* clientInPVS, int numPVSClients) {
	int i, j, msgSize, msgWriteBit;
	idPlayer* player, * spectated = NULL;
	idEntity* ent;
	idCamera* serverGameCamera = GetCamera();
	pvsHandle_t pvsHandle;
	pvsHandle_t remoteCameraPvsHandle[idEntity::MAX_PVS_AREAS]; //for idSecurityCamera
	idBitMsgDelta deltaMsg;
	snapshot_t* snapshot;
	entityState_t* base, * newBase;
	int numSourceAreas, sourceAreas[idEntity::MAX_PVS_AREAS];
	int remoteCameraPVSCount = 0;

	//Used by stradex for netcode optimization
	int serverSendEntitiesCount = 0;
	int serverEntitiesLimit = net_serverSnapshotLimit.GetInteger();

	player = static_cast<idPlayer*>(coopentities[clientNum]);

	if (!player) {
		return;
	}

	if (player->spectating && player->spectator != clientNum && entities[player->spectator]) {
		spectated = static_cast<idPlayer*>(entities[player->spectator]);
	}
	else {
		spectated = player;
	}

	// free too old snapshots
	FreeSnapshotsOlderThanSequence(clientNum, sequence - 64);

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->sequence = sequence;
	snapshot->firstEntityState = NULL;
	snapshot->next = clientSnapshots[clientNum];
	clientSnapshots[clientNum] = snapshot;
	memset(snapshot->pvs, 0, sizeof(snapshot->pvs));

  // get PVS for this player
  // don't use PVSAreas for networking - PVSAreas depends on animations (and md5 bounds), which are not synchronized
	if (gameLocal.inCinematic && gameLocal.GetCamera()) {
		numSourceAreas = gameRenderWorld->BoundsInAreas(gameLocal.GetCamera()->GetPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS);
	}
	else {
		numSourceAreas = gameRenderWorld->BoundsInAreas(spectated->GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS);
	}

	pvsHandle = gameLocal.pvs.SetupCurrentPVS(sourceAreas, numSourceAreas, PVS_NORMAL);

	// Prepare remoteCameraPvsHandle to send snapshots about entities in securities cameras
	for (i = 0; i < MAX_GENTITIES; i++) {
		ent = entities[i];
		if (ent && ent->cameraTarget && ent->PhysicsTeamInPVS(pvsHandle)) {
			//remoteCameraPvsHandle
			for (j = 0; j < 4; j++) { //is this necessary?
				sourceAreas[j] = 0;
			}
			numSourceAreas = gameRenderWorld->BoundsInAreas(ent->cameraTarget->GetPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS);
			remoteCameraPvsHandle[remoteCameraPVSCount] = gameLocal.pvs.SetupCurrentPVS(sourceAreas, numSourceAreas, PVS_NORMAL);
			remoteCameraPVSCount++;

			if (remoteCameraPVSCount >= idEntity::MAX_PVS_AREAS) {
				break;
			}
		}
	}

#if ASYNC_WRITE_TAGS
	idRandom tagRandom;
	tagRandom.SetSeed( random.RandomInt() );
	msg.WriteInt( tagRandom.GetSeed() );
#endif


	//Added by Stradex for netcode optimization (SORT LIST)

	//Clear the list first
	for (j=0; j < MAX_GENTITIES; j++) {
		sortsnapshotentities[j] = NULL;
	}

	int sortSnapCount=1;
	sortsnapshotentities[0] = coopentities[ clientNum ]; //ensure to always send info about our own player

	//Filtering the snapshots that are going to be processed
	for( ent = coopSyncEntities.Next(); ent != NULL; ent = ent->coopNode.Next() ) {
		ent->readByServer = false;

		if ((ent->entityCoopNumber == clientNum) || ent->clientsideNode.InList() || !ent->fl.coopNetworkSync) { //Stradex: ignore clientside only entities to avoid weird shit
			continue;
		}

		if (ent->IsType(idProjectile::Type) && static_cast<idProjectile*>(ent)->selfClientside) { //clientside self projectiles
			idEntity* tmpOwner = static_cast<idProjectile*>(ent)->GetOwner();
			if (tmpOwner && tmpOwner->entityNumber == clientNum) { //projectiles created by current player
				continue;
			}
		}

		if (gameLocal.inCinematic && ent->forceNetworkSync && ent->IsType(idAI::Type) && ent->IsActive()) {
			//dirty hack for cinematic and worldSpawn
			sortsnapshotentities[sortSnapCount++] = ent;
			continue;
		}

		if ( !ent->PhysicsTeamInPVS( pvsHandle ) ) {
			if (!ent->forceSnapshotUpdateOrigin && ent->PhysicsTeamInPVS_snapshot( pvsHandle, clientNum )) {
				//ent->ClearPVSAreas_snapshot(clientNum);
				//ent->lastSnapshotOrigin[clientNum] = ent->GetRenderEntity()->origin;
				ent->inSnapshotQueue[clientNum]++; //hack?
				ent->forceSnapshotUpdateOrigin = true;
				ent->inRemoteCameraPVS[clientNum] = false; // hack?
			} else {
				bool isInRemoteCameraPVS = false;
				for (i = 0; i < remoteCameraPVSCount; i++) {
					if (ent->PhysicsTeamInPVS(remoteCameraPvsHandle[i])) {
						ent->inRemoteCameraPVS[clientNum] = true; //Ensure that this entity is low priority in case of snapshotoverflow
						isInRemoteCameraPVS = true;
						break;
					}
				}
				if (!isInRemoteCameraPVS) {
					continue;
				}
			}
		} else {
			ent->inRemoteCameraPVS[clientNum] = false;
		}

		if (ent->firstTimeInClientPVS[clientNum] && !ent->forceSnapshotUpdateOrigin) {
			ent->forceSnapshotUpdateOrigin = true;
		}

		if (!ent->IsActive() && !ent->IsMasterActive() && !ent->firstTimeInClientPVS[clientNum] && !ent->forceNetworkSync && !ent->inSnapshotQueue[clientNum] && !ent->MasterUseOldNetcode()) { //ignore inactive entities that the player already saw before
			continue;
		}
		if (ent->IsHidden() && !ent->firstTimeInClientPVS[clientNum] && !ent->inSnapshotQueue[clientNum] && !gameLocal.inCinematic ) { //this shit is really important to improve server netcode
			continue;
		}
		//Since sorting it's a pretty expensive stuff, let's try to have this list the less filled with entities possible
		sortsnapshotentities[sortSnapCount++] = ent;
	}

	snapshotsort_context_s context; // this is to keep sorting signatures clean if isInOrder requires more game state information
	context.clientNum = clientNum;
	snapshotsort(context, sortsnapshotentities, 1, sortSnapCount - 1);

	// create the snapshot
	for (j=0, ent = sortsnapshotentities[j]; ent != NULL; ent = sortsnapshotentities[++j]) {
		
		snapshot->pvs[ ent->entityNumber >> 5 ] |= 1 << ( ent->entityNumber & 31 ); //COOP add entities to the snapshot pvs only to netsync entities STRADEX
		
		// save the write state to which we can revert when the entity didn't change at all
		msg.SaveWriteState( msgSize, msgWriteBit );
	
		msg.WriteBits( ent->entityCoopNumber, GENTITYNUM_BITS ); //test new netcode coop sync


		base = clientEntityStates[clientNum][ent->entityCoopNumber];
	
		if ( base ) {
			base->state.BeginReading();
		}
		newBase = entityStateAllocator.Alloc();
		newBase->entityCoopNumber = ent->entityCoopNumber;

		newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
		newBase->state.BeginWriting();

		deltaMsg.Init( base ? &base->state : NULL, &newBase->state, &msg );
		deltaMsg.WriteBits( coopIds[ ent->entityCoopNumber ], 32 - GENTITYNUM_BITS ); //testing netsync coop
		
		deltaMsg.WriteBits( ent->GetType()->typeNum, idClass::GetTypeNumBits() );
		deltaMsg.WriteBits( ServerRemapDecl( -1, DECL_ENTITYDEF, ent->entityDefNumber ), entityDefBits );

		// write the class specific data to the snapshot
		ent->WriteToSnapshot( deltaMsg );

		if ( !deltaMsg.HasChanged() && !ent->inSnapshotQueue[clientNum] ) { //if inSnapshotQueue, send data anyways
			msg.RestoreWriteState( msgSize, msgWriteBit );
			entityStateAllocator.Free( newBase );
		} else if (serverSendEntitiesCount >= serverEntitiesLimit) {
			msg.RestoreWriteState( msgSize, msgWriteBit );
			entityStateAllocator.Free( newBase );
			if (ent->forceSnapshotUpdateOrigin) { //Stradex: little hack
				ent->firstTimeInClientPVS[clientNum] = true; 
			}
			ent->inSnapshotQueue[clientNum]++;
		} else {
			newBase->next = snapshot->firstEntityState;
			snapshot->firstEntityState = newBase;

			//Coop specific stuff
			ent->inSnapshotQueue[clientNum] = 0;
			serverSendEntitiesCount++;
			ent->firstTimeInClientPVS[clientNum] = false; //Let the server know that this client already saw this entity for atleast one time
			ent->forceSnapshotUpdateOrigin = false;
			ent->ClearPVSAreas_snapshot(clientNum);
			ent->lastSnapshotOrigin[clientNum] = ent->GetRenderEntity()->origin;
#if ASYNC_WRITE_TAGS
			msg.WriteInt( tagRandom.RandomInt() );
#endif
		}
	}

	msg.WriteBits( ENTITYNUM_NONE, GENTITYNUM_BITS );

	// write the PVS to the snapshot
#if ASYNC_WRITE_PVS
	for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
		if ( i < numSourceAreas ) {
			msg.WriteInt( sourceAreas[ i ] );
		} else {
			msg.WriteInt( 0 );
		}
	}
	gameLocal.pvs.WritePVS( pvsHandle, msg );
#endif
	for ( i = 0; i < ENTITY_PVS_SIZE; i++ ) {
		msg.WriteDeltaInt( clientPVS[clientNum][i], snapshot->pvs[i] );
	}

	// free the PVS
	pvs.FreeCurrentPVS( pvsHandle );
	for (i = 0; i < remoteCameraPVSCount; i++) {
		pvs.FreeCurrentPVS(remoteCameraPvsHandle[i]);
	}

	//Write the cinematics and world info to the client
	msg.WriteBits(inCinematic, 1);
	msg.WriteBits(gameLocal.world->spawnArgs.GetBool("no_Weapons"), 1);

	if (serverGameCamera && serverGameCamera->isMapEntity) {
		msg.WriteShort(serverGameCamera->entityNumber);
		}
	else {
		msg.WriteShort(-1);
	}

	//Write worldspawn info to the client

	// write the game and player state to the snapshot
	base = clientEntityStates[clientNum][ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game and player state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->entityCoopNumber = ENTITYNUM_NONE; //added for coop
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
	memcpy( clientInPVS, snapshot->pvs, ( numPVSClients + 7 ) >> 3 );
	LittleRevBytes( clientInPVS, sizeof( int ), sizeof( clientInPVS ) / sizeof ( int ) );

}

/*
===============
idGameLocal::addToServerEventOverFlowList
===============
*/
void idGameLocal::addToServerEventOverFlowList(entityNetEvent_t* event, int clientNum)
{

	for (int i=0; i < SERVER_EVENTS_QUEUE_SIZE; i++) {
		if (serverOverflowEvents[i].eventId == SERVER_EVENT_NONE) {
			serverOverflowEvents[i].event = event;
			serverOverflowEvents[i].eventId = event->event;
			serverOverflowEvents[i].isEventType = true;
			serverOverflowEvents[i].excludeClient = clientNum; //FIXME Wrong var name: THIS IS NOT excludeClient when the entity is from saved queue
			return;
		}
	}
	common->DWarning("[COOP] No free slot for serverOverflowEvents\n");
}


/*
===============
idGameLocal::addToServerEventOverFlowList
===============
*/
void idGameLocal::addToServerEventOverFlowList(int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient, int eventTime, idEntity* ent, bool saveLastOnly)
{

	if (!ent) {
		common->DWarning("[COOP FATAL] Trying to add an event from an unknown entity\n");
		return;
	}

	for (int i=0; i < SERVER_EVENTS_QUEUE_SIZE; i++) {
		if (serverOverflowEvents[i].eventId == SERVER_EVENT_NONE) {
			serverOverflowEvents[i].eventEnt = ent;
			serverOverflowEvents[i].eventId = eventId;
			if ( msg ) {
				serverOverflowEvents[i].paramsSize = msg->GetSize();
				memcpy( serverOverflowEvents[i].paramsBuf, msg->GetData(), msg->GetSize() );
			} else {
				serverOverflowEvents[i].paramsSize = 0;
			}
			serverOverflowEvents[i].saveEvent = saveEvent;
			serverOverflowEvents[i].excludeClient = excludeClient;
			serverOverflowEvents[i].saveLastOnly = saveLastOnly;
			serverOverflowEvents[i].eventTime = eventTime;
			serverOverflowEvents[i].isEventType = false;
			return;
		}
	}

	common->DWarning("[COOP] No free slot for serverOverflowEvents\n");
}

/*
===============
idGameLocal::sendServerOverflowEvents
===============
*/
void idGameLocal::sendServerOverflowEvents( void )
{
	serverEventsCount=0;

	if (overflowEventCountdown > 0) overflowEventCountdown--;

	for (int i=0; i < SERVER_EVENTS_QUEUE_SIZE; i++) {
		if (serverOverflowEvents[i].eventId == SERVER_EVENT_NONE) {
			continue;
		}
		if (!serverOverflowEvents[i].eventEnt && !serverOverflowEvents[i].isEventType) {
			serverOverflowEvents[i].eventId = SERVER_EVENT_NONE;
			serverOverflowEvents[i].isEventType = false;
			continue;
		}
		if (serverOverflowEvents[i].isEventType && !getEntityBySpawnId(serverOverflowEvents[i].event->spawnId)) {
			serverOverflowEvents[i].eventId = SERVER_EVENT_NONE;
			serverOverflowEvents[i].isEventType = false;
			continue;
		}
		if ((serverEventsCount > MAX_SERVER_EVENTS_PER_FRAME) || (overflowEventCountdown > 0)) {
			continue; //don't return, continue just in case it's necessary to clean the serverOverflowEvents array
		}

		idBitMsg	outMsg;
		byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

		//event send

		if (serverOverflowEvents[i].isEventType) { //client joins and read savedqueue events

		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
		if (mpGame.IsGametypeCoopBased()) {
			outMsg.WriteBits( serverOverflowEvents[i].event->coopId, 32 ); //testing coop netsync
			outMsg.WriteBits( serverOverflowEvents[i].event->spawnId, 32 ); //added for coop
		} else {
			outMsg.WriteBits( serverOverflowEvents[i].event->spawnId, 32 );
		}
		
		outMsg.WriteByte( serverOverflowEvents[i].event->event );
		outMsg.WriteInt( serverOverflowEvents[i].event->time );
		outMsg.WriteBits( serverOverflowEvents[i].event->paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		if ( serverOverflowEvents[i].event->paramsSize ) {
			outMsg.WriteData( serverOverflowEvents[i].event->paramsBuf, serverOverflowEvents[i].event->paramsSize );
		}

		networkSystem->ServerSendReliableMessage( serverOverflowEvents[i].excludeClient, outMsg ); //FIXME: Here excludeClient == clientNum


		} else { //from Entity method: ServerSendEvent

		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
		if (gameLocal.mpGame.IsGametypeCoopBased()) {
			outMsg.WriteBits( gameLocal.GetCoopId( serverOverflowEvents[i].eventEnt ), 32 );
			outMsg.WriteBits( gameLocal.GetSpawnId( serverOverflowEvents[i].eventEnt ), 32 ); //added for coop
		} else {
			outMsg.WriteBits( gameLocal.GetSpawnId( serverOverflowEvents[i].eventEnt ), 32 );
		}
	
		outMsg.WriteByte( serverOverflowEvents[i].eventId );
		outMsg.WriteInt( gameLocal.time );
		outMsg.WriteBits( serverOverflowEvents[i].paramsSize, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		if ( serverOverflowEvents[i].paramsSize ) {
			outMsg.WriteData( serverOverflowEvents[i].paramsBuf, serverOverflowEvents[i].paramsSize );
		}

		if ( serverOverflowEvents[i].excludeClient != -1 ) {
			networkSystem->ServerSendReliableMessageExcluding( serverOverflowEvents[i].excludeClient, outMsg );
		} else {
			networkSystem->ServerSendReliableMessage( -1, outMsg );
		}
		
		if ( serverOverflowEvents[i].saveEvent ) {
			idBitMsg	saveMsg;
			byte		tmpBuf[MAX_GAME_MESSAGE_SIZE];
			saveMsg.Init( tmpBuf, sizeof( tmpBuf ) );
			saveMsg.BeginWriting();
			saveMsg.WriteData( serverOverflowEvents[i].paramsBuf, serverOverflowEvents[i].paramsSize );
			gameLocal.SaveEntityNetworkEvent( serverOverflowEvents[i].eventEnt, serverOverflowEvents[i].eventId, &saveMsg , serverOverflowEvents[i].saveLastOnly);
		}

		}

		//Remove event from the overflow queue list
		serverOverflowEvents[i].eventId = SERVER_EVENT_NONE;
		serverOverflowEvents[i].eventEnt = NULL;
		serverOverflowEvents[i].event = NULL;
		serverOverflowEvents[i].isEventType = false;
		serverEventsCount++;
	}
	if (serverEventsCount > 0) {
		common->DWarning("[COOP] Server Events overflow!, using serverOverflowEvents queue list to avoid the crash for clients\n");
		overflowEventCountdown=SERVER_EVENT_OVERFLOW_WAIT;
	}
	if (overflowEventCountdown > 0) {
		serverEventsCount=MAX_SERVER_EVENTS_PER_FRAME; //FIXME: Ugly way for doing this.  Not pretty
	}
}

/*
===============
idGameLocal::WriteRemovedEntitiesToEvent
===============
*/

void idGameLocal::WriteRemovedEntitiesToEvent(idBitMsg& msg) {
	int count = 0;
	int removedCount = CountMapSyncEntitiesRemoved(); //safe way to send data, in case another entity is deleted while writing the message
	msg.WriteInt(removedCount);
	for (int i = 0; i < num_removeSyncEntities; i++) {
		if (count >= removedCount) {
			break;
		}
		if (!removeSyncEntities[i]) {
			msg.WriteShort(i);
			count++;
		}
	}

}

/*
===============
idGameLocal::ReadRemovedEntitiesFromEvent
===============
*/
void idGameLocal::ReadRemovedEntitiesFromEvent(const idBitMsg& msg) {
	int entitiesToRemove = msg.ReadInt();
	for (int i = 0; i < entitiesToRemove; i++) {
		int entityRemoveId = msg.ReadShort();
		if (removeSyncEntities[entityRemoveId] && removeSyncEntities[entityRemoveId]->isMapEntity && removeSyncEntities[entityRemoveId]->allowRemoveSync) {
			removeSyncEntities[entityRemoveId]->CS_PostEventMS(&EV_Remove, 0);
		}

	}
}