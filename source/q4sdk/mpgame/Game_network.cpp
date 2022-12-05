#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
// RAVEN BEGIN
// bdube: client entities
#include "client/ClientEffect.h"
// shouchard:  ban list support
#define BANLIST_FILENAME "banlist.txt"
// RAVEN END

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
	#define ASYNC_WRITE_TAGS 1
#endif

idCVar net_clientShowSnapshot( "net_clientShowSnapshot", "0", CVAR_GAME | CVAR_INTEGER, "", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );
idCVar net_clientShowSnapshotRadius( "net_clientShowSnapshotRadius", "128", CVAR_GAME | CVAR_FLOAT, "" );
idCVar net_clientMaxPrediction( "net_clientMaxPrediction", "1000", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of milliseconds a client can predict ahead of server." );
idCVar net_clientLagOMeter( "net_clientLagOMeter", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | PC_CVAR_ARCHIVE, "draw prediction graph" );
idCVar net_clientLagOMeterResolution( "net_clientLagOMeterResolution", "16", CVAR_GAME | CVAR_INTEGER | CVAR_NOCHEAT | PC_CVAR_ARCHIVE, "resolution for predict ahead graph (upper part of lagometer)", 1, 100 );

// RAVEN BEGIN
// ddynerman: performance profiling
int net_entsInSnapshot;
int net_snapshotSize;

extern const int ASYNC_PLAYER_FRAG_BITS;
// RAVEN END

// message senders
idNullMessageSender nullSender;
idServerReliableMessageSender serverReliableSender;
idRepeaterReliableMessageSender repeaterReliableSender;

/*
================
idGameLocal::InitAsyncNetwork
================
*/
void idGameLocal::InitAsyncNetwork( void ) {
	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	memset( viewerEntityStates, 0, sizeof( *viewerEntityStates ) * maxViewers );
	memset( viewerPVS, 0, sizeof( *viewerPVS ) * maxViewers );
	memset( viewerSnapshots, 0, sizeof( *viewerSnapshots ) * maxViewers );
	memset( viewerUnreliableMessages, 0, sizeof (*viewerUnreliableMessages) * maxViewers );

	eventQueue.Init();

	// we used to have some special case -1 value, but we don't anymore so this is always >= 0 now
	entityDefBits = idMath::BitsForInteger( declManager->GetNumDecls( DECL_ENTITYDEF ) ) + 1;
	localClientNum = 0; // on a listen server SetLocalUser will set this right
	realClientTime = 0;
	isNewFrame = true;

	nextLagoCheck = 0;
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
	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	memset( viewerEntityStates, 0, sizeof( *viewerEntityStates ) * maxViewers );
	memset( viewerPVS, 0, sizeof( *viewerPVS ) * maxViewers );
	memset( viewerSnapshots, 0, sizeof( *viewerSnapshots ) * maxViewers );
	memset( viewerUnreliableMessages, 0, sizeof (*viewerUnreliableMessages) * maxViewers );
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

	if ( clientNum == MAX_CLIENTS && !entities[ ENTITYNUM_NONE ] ) {
		SpawnPlayer( ENTITYNUM_NONE );
		idPlayer *player = gameLocal.GetLocalPlayer();
		assert( player );
		player->SpawnFromSpawnSpot();
	}
}

/*
================
idGameLocal::ServerAllowClient
clientId is the ID of the connecting client - can later be mapped to a clientNum by calling networkSystem->ServerGetClientNum( clientId )
================
*/
allowReply_t idGameLocal::ServerAllowClient( int clientId, int numClients, const char *IP, const char *guid, const char *password, const char *privatePassword, char reason[ MAX_STRING_CHARS ] ) {
	reason[0] = '\0';

// RAVEN BEGIN
// shouchard:  ban support
	if ( IsGuidBanned( guid ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107239" );
		return ALLOW_NO;
	}
// RAVEN END

	if ( serverInfo.GetInt( "si_pure" ) && !mpGame.IsPureReady() ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107139" );
		return ALLOW_NOTYET;
	}

	if ( !serverInfo.GetInt( "si_maxPlayers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107140" );
		return ALLOW_NOTYET;
	}

	// completely full
	if ( numClients >= serverInfo.GetInt( "si_maxPlayers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107141" );
		return ALLOW_NOTYET;
	}

	// check private clients
	if( serverInfo.GetInt( "si_privatePlayers" ) > 0 ) {
		// just in case somehow we have a stale private clientId that matches a new client
		mpGame.RemovePrivatePlayer( clientId );

		const char *privatePass = cvarSystem->GetCVarString( "g_privatePassword" );
		if( privatePass[ 0 ] == '\0' ) {
			common->Warning( "idGameLocal::ServerAllowClient() - si_privatePlayers > 0 with no g_privatePassword" );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say si_privatePlayers is set but g_privatePassword is empty" );
			idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107142" );
			return ALLOW_NOTYET;
		}


		int numPrivateClients = cvarSystem->GetCVarInteger( "si_numPrivatePlayers" );

		// private clients that take up public slots are considered public clients
		numPrivateClients = idMath::ClampInt( 0, serverInfo.GetInt( "si_privatePlayers" ), numPrivateClients );

		if ( !idStr::Cmp( privatePass, privatePassword ) ) {
			// once this client spawns in, they'll be marked private
			mpGame.AddPrivatePlayer( clientId );
		} else if( (numClients - numPrivateClients) >= (serverInfo.GetInt( "si_maxPlayers" ) - serverInfo.GetInt( "si_privatePlayers" )) ) {
			// if the number of public clients is greater than or equal to the number of public slots, require a private slot
			idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107141" );
			return ALLOW_NOTYET;
		}
	} 
	

	if ( !cvarSystem->GetCVarBool( "si_usePass" ) ) {
		return ALLOW_YES;
	}

	const char *pass = cvarSystem->GetCVarString( "g_password" );
	if ( pass[ 0 ] == '\0' ) {
		common->Warning( "si_usePass is set but g_password is empty" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say si_usePass is set but g_password is empty" );
		// avoids silent misconfigured state
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107142" );
		return ALLOW_NOTYET;
	}

	if ( !idStr::Cmp( pass, password ) ) {
		return ALLOW_YES;
	}

	idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107143" );
	Printf( "Rejecting client %s from IP %s: invalid password\n", guid, IP );
	return ALLOW_BADPASS;
}

/*
================
idGameLocal::ReallocViewers
Allocate/Reallocate space for viewers
================
*/
template<typename T> static ID_INLINE void ReallocateZero( T *&var, int oldSize, int newSize ) {
	T *new_var = newSize ? new T[ newSize ] : NULL;
	SIMDProcessor->Memcpy( new_var, var, sizeof(*new_var) * Min( newSize, oldSize ) );
	delete[] var;
	var = new_var;
	if ( newSize > oldSize ) {
		SIMDProcessor->Memset( var + oldSize, 0, sizeof(*var) * (newSize - oldSize) );
	}
}

static ID_INLINE void ReallocateZeroClearInfo( viewer_t *&var, int oldSize, int newSize ) {
	int minBound = Min( newSize, oldSize ), i;
	viewer_t *new_var = newSize ? new viewer_t[ newSize ] : NULL;
	for (i = 0; i < minBound; i++ ) {
		new_var[ i ] = var[ i ];
	}
	for (i = 0; i < oldSize; i++ ) {
		var[ i ].info.Clear();
	}
	delete[] var;
	var = new_var;
	for (i = oldSize; i < newSize; i++) {
		var[i].connected = var[i].active = var[i].priv = var[i].nopvs = false;
		var[i].pvsArea = -1;
	}
}

void idGameLocal::ReallocViewers( int newMaxViewers ) {
	int i;

	if ( newMaxViewers == maxViewers ) {
		return;
	}

	if ( newMaxViewers > 0 ) { // zinx: during shutdown we clear everything, but there are still connected viewers.
		for ( i = newMaxViewers; i < maxViewers; i++ ) {
			if ( viewers[ i ].connected ) {
				// this should never happen
				Error( "Too many active viewers for ri_maxViewers change" );
			}
		}
		assert( maxViewer <= newMaxViewers );
	} else {
		maxViewer = 0;
	}

	ReallocateZeroClearInfo( viewers, maxViewers, newMaxViewers );
	ReallocateZero( viewerEntityStates, maxViewers, newMaxViewers );
	ReallocateZero( viewerPVS, maxViewers, newMaxViewers );
	ReallocateZero( viewerSnapshots, maxViewers, newMaxViewers );
	ReallocateZero( viewerUnreliableMessages, maxViewers, newMaxViewers );

	maxViewers = newMaxViewers;

	UpdateRepeaterInfo();
}

/*
================
idGameLocal::RepeaterAllowClient
clientId is the ID of the connecting client - can later be mapped to a clientNum by calling networkSystem->ServerGetClientNum( clientId )
================
*/
allowReply_t idGameLocal::RepeaterAllowClient( int clientId, int numClients, const char *IP, const char *guid, bool repeater, const char *password, const char *privatePassword, char reason[ MAX_STRING_CHARS ] ) {
	reason[0] = '\0';

	if ( IsGuidBanned( guid ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107239" );
		return ALLOW_NO;
	}

	if ( serverInfo.GetInt( "si_pure" ) != 0 && !mpGame.IsPureReady() ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107139" );
		return ALLOW_NOTYET;
	}

	if ( !cvarSystem->GetCVarInteger( "ri_maxViewers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_123000" );
		return ALLOW_NOTYET;
	}

	// completely full
	if ( numClients >= cvarSystem->GetCVarInteger( "ri_maxViewers" ) ) {
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_123000" );
		return ALLOW_NOTYET;
	}

	// check private clients
	// just in case somehow we have a stale private clientId that matches a new client
	privateViewerIds.Remove( clientId );
	nopvsViewerIds.Remove( clientId );

	const char *privatePass = cvarSystem->GetCVarString( "g_privateViewerPassword" );
	if ( privatePass[ 0 ] == '\0' && ri_privateViewers.GetInteger() > 0 ) {
		common->Warning( "idGameLocal::RepeaterAllowClient() - ri_privateViewers > 0 with no g_privateViewerPassword" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say ri_privateViewers is set but g_privateViewerPassword is empty" );
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107142" );
		return ALLOW_NOTYET;
	}

	int numPrivateClients = privateViewerIds.Num();

	// private clients that take up public slots are considered public clients
	numPrivateClients = idMath::ClampInt( 0, ri_privateViewers.GetInteger(), numPrivateClients );

	if ( repeater ) {
		const char *nopvsPass = cvarSystem->GetCVarString( "g_repeaterPassword" );

		if ( nopvsPass[ 0 ] != '\0' ) {
			if ( idStr::Cmp( nopvsPass, privatePassword ) ) {
				// password is set, and they don't match.
				idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107143" );
				Printf( "Rejecting viewer %s from IP %s: invalid password for repeater\n", guid, IP );
				return ALLOW_BADPASS;
			}

			// once this client spawns in, they'll be marked private
			privateViewerIds.Append( clientId );
		} else {
			if ( privatePass[ 0 ] != '\0' && !idStr::Cmp( privatePass, privatePassword ) ) {
				// once this client spawns in, they'll be marked private
				privateViewerIds.Append( clientId );
			}
		}

		// once this client spawns in, they'll be marked as a broadcaster
		nopvsViewerIds.Append( clientId );
	} else if ( privatePass[ 0 ] != '\0' && !idStr::Cmp( privatePass, privatePassword ) ) {
		// once this client spawns in, they'll be marked private
		privateViewerIds.Append( clientId );
	} else if( (numClients - numPrivateClients) >= ( cvarSystem->GetCVarInteger( "ri_maxViewers" ) - ri_privateViewers.GetInteger() ) ) {
		// if the number of public clients is greater than or equal to the number of public slots, require a private slot
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107141" );
		return ALLOW_NOTYET;
	}

	if ( !ri_useViewerPass.GetBool() ) {
		return ALLOW_YES;
	}

	const char *pass = cvarSystem->GetCVarString( "g_viewerPassword" );
	if ( pass[ 0 ] == '\0' ) {
		common->Warning( "ri_useViewerPass is set but g_viewerPassword is empty" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "say ri_useViewerPass is set but g_viewerPassword is empty" );
		// avoids silent misconfigured state
		idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107142" );
		return ALLOW_NOTYET;
	}

	if ( !idStr::Cmp( pass, password ) ) {
		return ALLOW_YES;
	}

	idStr::snPrintf( reason, MAX_STRING_CHARS, "#str_107143" );
	Printf( "Rejecting viewer %s from IP %s: invalid password\n", guid, IP );
	return ALLOW_BADPASS;
}

/*
================
idGameLocal::ServerClientConnect
================
*/
void idGameLocal::ServerClientConnect( int clientNum, const char *guid ) {
	// make sure no parasite entity is left
	if ( entities[ clientNum ] ) {
		common->DPrintf( "ServerClientConnect: remove old player entity\n" );
		delete entities[ clientNum ];
	}
	unreliableMessages[ clientNum ].Init( 0 );
	userInfo[ clientNum ].Clear();
	mpGame.ServerClientConnect( clientNum );
	Printf( "client %d connected.\n", clientNum );
}

/*
================
idGameLocal::RepeaterClientConnect
================
*/
void idGameLocal::RepeaterClientConnect( int clientNum ) {
	// ensure that we have enough slots for this client
	assert( cvarSystem->GetCVarInteger( "ri_maxViewers" ) > clientNum );
	ReallocViewers( cvarSystem->GetCVarInteger( "ri_maxViewers" ) );

	viewers[ clientNum ].connected = true;
	viewers[ clientNum ].active = false;

	if ( maxViewer <= clientNum ) {
		maxViewer = clientNum + 1;
	}

	// mark them as private/nopvs
	for ( int i = 0; i < privateViewerIds.Num(); i++ ) {
		int num = networkSystem->RepeaterGetClientNum( privateViewerIds[ i ] );

		// check for timed out clientids
		if ( num < 0 || (num == clientNum && viewers[ clientNum ].priv) ) {
			privateViewerIds.RemoveIndex( i );
			i--;
			continue;
		}

		if ( num == clientNum ) {
			viewers[ clientNum ].priv = true;
			continue;
		}
	}

	for ( int i = 0; i < nopvsViewerIds.Num(); i++ ) {
		int num = networkSystem->RepeaterGetClientNum( nopvsViewerIds[ i ] );

		// check for timed out clientids
		if ( num < 0 || (num == clientNum && viewers[ clientNum ].nopvs) ) {
			nopvsViewerIds.RemoveIndex( i );
			i--;
			continue;
		}

		if ( num == clientNum ) {
			viewers[ clientNum ].nopvs = true;
			continue;
		}
	}

	Printf( "viewer %d connected%s.\n", clientNum, viewers[ clientNum ].nopvs ? " (repeater)" : "" );

	// count up viewer slots so players know when servers are full
	int numClients = 0;
	for ( int i = 0; i < maxViewer; i++ ) {
		if ( !viewers[ i ].connected ) {
			continue;
		}

		++numClients;
	}

	int numPrivateClients = privateViewerIds.Num();
	// private clients that take up public slots are considered public clients
	numPrivateClients = idMath::ClampInt( 0, ri_privateViewers.GetInteger(), numPrivateClients );

	ri_numViewers.SetInteger( numClients );
	ri_numPrivateViewers.SetInteger( numPrivateClients );

	UpdateRepeaterInfo();
}

/*
================
idGameLocal::ServerClientBegin
================
*/
void idGameLocal::ServerClientBegin( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	// spawn the player
	SpawnPlayer( clientNum );
	if ( clientNum == localClientNum ) {
		mpGame.EnterGame( clientNum );
	}

	// ddynerman: connect time
	((idPlayer*)entities[ clientNum ])->SetConnectTime( time );

	// send message to spawn the player at the clients
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPAWN_PLAYER );
	outMsg.WriteByte( clientNum );
	outMsg.WriteLong( spawnIds[ clientNum ] );
	networkSystem->ServerSendReliableMessage( -1, outMsg );

	if( gameType != GAME_TOURNEY ) {
		((idPlayer*)entities[ clientNum ])->JoinInstance( 0 );
	} else {
		// instance 0 might be empty in Tourney
		((idPlayer*)entities[ clientNum ])->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( 0 ) );
	}
//RAVEN BEGIN
//asalmon: This client has finish loading and will be spawned mark them as ready.
#ifdef _XENON
	Live()->ClientReady(clientNum);
#endif
//RAVEN END
}

/*
================
idGameLocal::RepeaterClientBegin
================
*/
void idGameLocal::RepeaterClientBegin( int clientNum ) {
	viewers[ clientNum ].active = true;
}

/*
================
idGameLocal::ServerClientDisconnect
clientNum == MAX_CLIENTS for cleanup of server demo recording data
================
*/
void idGameLocal::ServerClientDisconnect( int clientNum ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( clientNum < MAX_CLIENTS ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting();
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DELETE_ENT );
		outMsg.WriteBits( ( spawnIds[ clientNum ] << GENTITYNUM_BITS ) | clientNum, 32 ); // see GetSpawnId
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}

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

	if ( clientNum == MAX_CLIENTS ) {
		return;
	}

	// only drop MP clients if we're in multiplayer and the server isn't going down
	if ( gameLocal.isMultiplayer && !(gameLocal.isListenServer && clientNum == gameLocal.localClientNum ) ) {
		// idMultiplayerGame::DisconnectClient will do the delete in MP
		mpGame.DisconnectClient( clientNum );
	} else {
		// delete the player entity
		delete entities[ clientNum ];
	}
	

}

/*
================
idGameLocal::RepeaterClientDisconnect
================
*/
void idGameLocal::RepeaterClientDisconnect( int clientNum ) {
	int			i;

	if ( clientNum >= maxViewers ) {
		// this can happen during shutdown, because repeater clients are
		// discarded when the (shared) entity states/snapshots of the other
		// clients are freed
		return;
	}

	// free snapshots stored for this client
	FreeSnapshotsOlderThanSequence( viewerSnapshots[ clientNum ], 0x7FFFFFFF );

	// free entity states stored for this client
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		if ( viewerEntityStates[ clientNum ][ i ] ) {
			entityStateAllocator.Free( viewerEntityStates[ clientNum ][ i ] );
			viewerEntityStates[ clientNum ][ i ] = NULL;
		}
	}

	// clear the client PVS
	memset( viewerPVS[ clientNum ], 0, sizeof( viewerPVS[ clientNum ] ) );

	viewerUnreliableMessages[ clientNum ].Init( 0 );

	viewers[ clientNum ].connected = false;
	viewers[ clientNum ].active = false;
	viewers[ clientNum ].priv = false;
	viewers[ clientNum ].nopvs = false;

	if ( maxViewer <= (clientNum+1) ) {
		// find the new highest client
		for ( maxViewer = clientNum; maxViewer >= 0 && !viewers[ maxViewer ].connected; maxViewer-- ) ;
		maxViewer++;
	}

	// remove them from private/nopvs lists
	for ( int i = 0; i < privateViewerIds.Num(); i++ ) {
		int num = networkSystem->RepeaterGetClientNum( privateViewerIds[ i ] );

		// check for timed out clientids
		if ( num < 0 ) {
			nopvsViewerIds.Remove( privateViewerIds[ i ] );
			privateViewerIds.RemoveIndex( i );
			i--;
			continue;
		}

		if ( num == clientNum ) {
			nopvsViewerIds.Remove( privateViewerIds[ i ] );
			privateViewerIds.RemoveIndex( i );
			i--;
			continue;
		}
	}

	// count up viewer slots so players know when servers are full
	int numClients = 0;
	for ( int i = 0; i < maxViewer; i++ ) {
		if ( !viewers[ i ].connected ) {
			continue;
		}

		++numClients;
	}

	int numPrivateClients = privateViewerIds.Num();
	// private clients that take up public slots are considered public clients
	numPrivateClients = idMath::ClampInt( 0, ri_privateViewers.GetInteger(), numPrivateClients );

	ri_numViewers.SetInteger( numClients );
	ri_numPrivateViewers.SetInteger( numPrivateClients );

	UpdateRepeaterInfo();
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
		networkSystem->ServerSendReliableMessage( clientNum, outMsg, true );
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
	networkSystem->ServerSendReliableMessage( clientNum, outMsg, true );

	mpGame.ServerWriteInitialReliableMessages( serverReliableSender.To( clientNum, true ), clientNum );
}

/*
================
idGameLocal::RepeaterWriteInitialReliableMessages

  Send reliable messages to initialize the client game up to a certain initial state.
================
*/
void idGameLocal::RepeaterWriteInitialReliableMessages( int clientNum ) {
	int			i;
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	// spawn players
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( entities[i] == NULL ) {
			continue;
		}
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.BeginWriting( );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SPAWN_PLAYER );
		outMsg.WriteByte( i );
		outMsg.WriteLong( spawnIds[ i ] );
		networkSystem->RepeaterSendReliableMessage( clientNum, outMsg );
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
	networkSystem->RepeaterSendReliableMessage( clientNum, outMsg );

	mpGame.ServerWriteInitialReliableMessages( repeaterReliableSender.To( clientNum ), ENTITYNUM_NONE );
}

/*
================
idGameLocal::FreeSnapshotsOlderThanSequence
================
*/
void idGameLocal::FreeSnapshotsOlderThanSequence( snapshot_t *&clientSnapshot, int sequence ) {
	snapshot_t *snapshot, *lastSnapshot, *nextSnapshot;
	entityState_t *state;

	for ( lastSnapshot = NULL, snapshot = clientSnapshot; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence < sequence ) {
			for ( state = snapshot->firstEntityState; state; state = snapshot->firstEntityState ) {
				snapshot->firstEntityState = snapshot->firstEntityState->next;
				entityStateAllocator.Free( state );
			}
			if ( lastSnapshot ) {
				lastSnapshot->next = snapshot->next;
			} else {
				clientSnapshot = snapshot->next;
			}
			snapshotAllocator.Free( snapshot );
		} else {
			lastSnapshot = snapshot;
		}
	}
}

/*
================
idGameLocal::FreeSnapshotsOlderThanSequence
================
*/
void idGameLocal::FreeSnapshotsOlderThanSequence( int clientNum, int sequence ) {
	FreeSnapshotsOlderThanSequence( clientSnapshots[ clientNum ], sequence );
}

/*
================
idGameLocal::ApplySnapshot
================
*/
bool idGameLocal::ApplySnapshot( snapshot_t *&clientSnapshot, entityState_t *entityStates[MAX_GENTITIES], int PVS[ENTITY_PVS_SIZE], int sequence ) {
	snapshot_t *snapshot, *lastSnapshot, *nextSnapshot;
	entityState_t *state;

	FreeSnapshotsOlderThanSequence( clientSnapshot, sequence );

	for ( lastSnapshot = NULL, snapshot = clientSnapshot; snapshot; snapshot = nextSnapshot ) {
		nextSnapshot = snapshot->next;
		if ( snapshot->sequence == sequence ) {
			for ( state = snapshot->firstEntityState; state; state = state->next ) {
				if ( entityStates[state->entityNumber] ) {
					entityStateAllocator.Free( entityStates[state->entityNumber] );
				}
				entityStates[state->entityNumber] = state;
			}
			// ~512 bytes
			memcpy( PVS, snapshot->pvs, sizeof( snapshot->pvs ) );
			if ( lastSnapshot ) {
				lastSnapshot->next = nextSnapshot;
			} else {
				clientSnapshot = nextSnapshot;
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
idGameLocal::ApplySnapshot
================
*/
bool idGameLocal::ApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( clientSnapshots[ clientNum ], clientEntityStates[ clientNum ], clientPVS[ clientNum ], sequence );
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
idGameLocal::WriteSnapshot
Write a snapshot of the current game state for the given client.
================
*/
void idGameLocal::WriteSnapshot( snapshot_t *&clientSnapshot, entityState_t *entityStates[MAX_GENTITIES], int PVS[ENTITY_PVS_SIZE], idMsgQueue &unreliable, int sequence, idBitMsg &msg, int transmitEntity, int transmitEntity2, int instance, bool doPVS, const idBounds &pvs_bounds, int lastSnapshotFrame ) {
	int i, msgSize, msgWriteBit;
	idEntity *ent;
	pvsHandle_t pvsHandle = { 0, 0 };	// shut warning
	idBitMsgDelta deltaMsg;
	snapshot_t *snapshot;
	entityState_t *base, *newBase;
	int numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];

	// free too old snapshots
	// ( that's a security, normal acking from server keeps a smaller backlog of snaps )
	FreeSnapshotsOlderThanSequence( clientSnapshot, sequence - 64 );

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->sequence = sequence;
	snapshot->firstEntityState = NULL;
	snapshot->next = clientSnapshot;
	clientSnapshot = snapshot;
	memset( snapshot->pvs, 0, sizeof( snapshot->pvs ) );

	if ( doPVS ) {
		// get PVS for this player
		// don't use PVSAreas for networking - PVSAreas depends on animations (and md5 bounds), which are not synchronized
		numSourceAreas = gameRenderWorld->BoundsInAreas( pvs_bounds, sourceAreas, idEntity::MAX_PVS_AREAS );
		pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );
	}

#if ASYNC_WRITE_TAGS
	idRandom tagRandom;
	tagRandom.SetSeed( random.RandomInt() );
	msg.WriteLong( tagRandom.GetSeed() );
#endif
	
	// write unreliable messages
	unreliable.WriteTo( msg ); // NOTE: No flush

#if ASYNC_WRITE_TAGS
	msg.WriteLong( tagRandom.RandomInt() );
#endif

	// create the snapshot
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {	
		// local fake client
		if ( ent->entityNumber == ENTITYNUM_NONE ) {
			continue;
		}

		// if the entity is not in the player PVS
		if ( doPVS && !ent->PhysicsTeamInPVS( pvsHandle ) && ent->entityNumber != transmitEntity && ent->entityNumber != transmitEntity2 ) {
			continue;
		}

		if ( ent->GetInstance() != instance ) {
			continue;
		}

		// if the entity is a map entity, mark it in PVS
		if ( isMapEntity[ ent->entityNumber ] ) {
			snapshot->pvs[ ent->entityNumber >> 5 ] |= 1 << ( ent->entityNumber & 31 );
		}

		// if that entity is not marked for network synchronization
		if ( !ent->fl.networkSync ) {
			continue;
		}

		// add the entity to the snapshot PVS
		snapshot->pvs[ ent->entityNumber >> 5 ] |= 1 << ( ent->entityNumber & 31 );

		// save the write state to which we can revert when the entity didn't change at all
		msg.SaveWriteState( msgSize, msgWriteBit );

		// write the entity to the snapshot
		msg.WriteBits( ent->entityNumber, GENTITYNUM_BITS );

		base = entityStates[ent->entityNumber];
		if ( base ) {
			base->state.BeginReading();
		}
		newBase = entityStateAllocator.Alloc();
		newBase->entityNumber = ent->entityNumber;
		newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
		newBase->state.BeginWriting();

		deltaMsg.InitWriting( base ? &base->state : NULL, &newBase->state, &msg );

		deltaMsg.WriteBits( spawnIds[ ent->entityNumber ], 32 - GENTITYNUM_BITS );
		assert( ent->entityDefNumber > 0 );
		deltaMsg.WriteBits( ent->entityDefNumber, entityDefBits );

		// write the class specific data to the snapshot
		ent->WriteToSnapshot( deltaMsg );

		// if this is a player and a transmit of the player state has been requested, write the player state
		if ( ent->entityNumber < MAX_CLIENTS ) {
			if ( transmitEntity < 0 || transmitEntity == ent->entityNumber || transmitEntity2 == ent->entityNumber ) {
				assert( ent->IsType( idPlayer::GetClassType() ) );
				deltaMsg.WriteBits( 1, 1 );
				static_cast< idPlayer * >( ent )->WritePlayerStateToSnapshot( lastSnapshotFrame, deltaMsg );
			} else {
				deltaMsg.WriteBits( 0, 1 );
			}
		}

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
	if ( doPVS ) {
		for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
			if ( i < numSourceAreas ) {
				msg.WriteLong( sourceAreas[ i ] );
			} else {
				msg.WriteLong( 0 );
			}
		}
		gameLocal.pvs.WritePVS( pvsHandle, msg );
	}
#endif
	// always write the entity pvs, even when doPVS isn't set
	for ( i = 0; i < ENTITY_PVS_SIZE; i++ ) {
		msg.WriteDeltaLong( PVS[i], snapshot->pvs[i] );
	}

	if ( doPVS ) {
		// free the PVS
		pvs.FreeCurrentPVS( pvsHandle );
	}

	// write the game state to the snapshot
	base = entityStates[ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->next = snapshot->firstEntityState;
	snapshot->firstEntityState = newBase;
	newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
	newBase->state.BeginWriting();
	deltaMsg.InitWriting( base ? &base->state : NULL, &newBase->state, &msg );

	WriteGameStateToSnapshot( deltaMsg );
}

/*
================
idGameLocal::ServerWriteSnapshot
Write a snapshot of the current game state for the given client.
================
*/
void idGameLocal::ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, int lastSnapshotFrame ) {

	int i;
	idPlayer *player, *spectated = NULL;

	player = static_cast<idPlayer *>( entities[ clientNum ] );
	assert( player );

	if ( player->spectating && player->spectator != clientNum && entities[ player->spectator ] ) {
		spectated = static_cast< idPlayer * >( entities[ player->spectator ] );
	} else {
		spectated = player;
	}

	WriteSnapshot(
		clientSnapshots[ clientNum ],
		clientEntityStates[ clientNum ],
		clientPVS[ clientNum ],
		unreliableMessages[ clientNum ],
		sequence,
		msg,
		spectated->entityNumber,
		clientNum,
		player->GetInstance(),
		true,
		spectated->GetPlayerPhysics()->GetAbsBounds(),
		lastSnapshotFrame
	);

	unreliableMessages[ clientNum ].Init( 0 ); // Flush

	// copy the client PVS string
// RAVEN BEGIN
// JSinger: Changed to call optimized memcpy
// jnewquist: Use dword array to match pvs array so we don't have endianness problems.
	const int numDwords = ( numPVSClients + 31 ) >> 5;
	for ( i = 0; i < numDwords; i++ ) {
		clientInPVS[i] = clientSnapshots[ clientNum ]->pvs[i];
	}
// RAVEN END
}

/*
===============
idGameLocal::ServerWriteServerDemoSnapshot
===============
*/
void idGameLocal::ServerWriteServerDemoSnapshot( int sequence, idBitMsg &msg, int lastSnapshotFrame ) {
	bool ret = ApplySnapshot( clientSnapshots[ MAX_CLIENTS ], clientEntityStates[ MAX_CLIENTS ], clientPVS[ MAX_CLIENTS ], sequence - 1 );
	ret = ret;	// shut the warning
	assert( ret || sequence == 1 );	// past the first snapshot of the server demo stream, there's always exactly one to clear

	idBounds dummy_bounds;

	WriteSnapshot(
		clientSnapshots[ MAX_CLIENTS ],
		clientEntityStates[ MAX_CLIENTS ],
		clientPVS[ MAX_CLIENTS ],
		unreliableMessages[ MAX_CLIENTS ],
		sequence,
		msg,
		-1,
		-1,
		0,
		false,
		dummy_bounds,
		lastSnapshotFrame
	);

	unreliableMessages[ MAX_CLIENTS ].Init( 0 ); // Flush
}

/*
===============
idGameLocal::RepeaterWriteSnapshot
===============
*/
void idGameLocal::RepeaterWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, const userOrigin_t &pvs_origin, int lastSnapshotFrame ) {
	int i;
	idBounds bounds;

	// if the client is spectating a valid player
	if ( pvs_origin.followClient >= 0 && pvs_origin.followClient < MAX_CLIENTS && entities[ pvs_origin.followClient ] && entities[ pvs_origin.followClient ]->IsType( idPlayer::GetClassType() ) ) {
		idPlayer *spectated = static_cast< idPlayer * >( entities[ pvs_origin.followClient ] );
		bounds = spectated->GetPlayerPhysics()->GetAbsBounds();
	} else {
		// zinx - FIXME - bounds, not point
		bounds = idBounds( pvs_origin.origin );
	}

	WriteSnapshot(
		viewerSnapshots[ clientNum ],
		viewerEntityStates[ clientNum ],
		viewerPVS[ clientNum ],
		viewerUnreliableMessages[ clientNum ],
		sequence,
		msg,
		viewers[ clientNum ].nopvs ? -1 : (pvs_origin.followClient >= 0 ? pvs_origin.followClient : ENTITYNUM_NONE ),
		ENTITYNUM_NONE,
		0,
		viewers[ clientNum ].nopvs ? false : true,
		bounds,
		lastSnapshotFrame
	);

	viewerUnreliableMessages[ clientNum ].Init( 0 ); // Flush

	// copy the client PVS string
// RAVEN BEGIN
// JSinger: Changed to call optimized memcpy
// jnewquist: Use dword array to match pvs array so we don't have endianness problems.
	const int numDwords = ( numPVSClients + 31 ) >> 5;
	for ( i = 0; i < numDwords; i++ ) {
		clientInPVS[i] = viewerSnapshots[ clientNum ]->pvs[i];
	}
// RAVEN END
}

/*
===============
idGameLocal::RepeaterEndSnapshots
===============
*/
void idGameLocal::RepeaterEndSnapshots( void ) {
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
idGameLocal::RepeaterApplySnapshot
================
*/
bool idGameLocal::RepeaterApplySnapshot( int clientNum, int sequence ) {
	return ApplySnapshot( viewerSnapshots[ clientNum ], viewerEntityStates[ clientNum ], viewerPVS[ clientNum ], sequence );
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
			
		if ( !entPtr.SetSpawnId( event->spawnId ) ) {
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

#ifdef _DEBUG
		entityNetEvent_t* freedEvent = eventQueue.Dequeue();
		assert( freedEvent == event );
#else
		eventQueue.Dequeue();
#endif
		eventQueue.Free( event );
	}
}

/*
================
idGameLocal::ServerSendChatMessage
================
*/
void idGameLocal::ServerSendChatMessage( int to, const char *name, const char *text, const char *parm ) {
	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CHAT );
	outMsg.WriteString( name );
	outMsg.WriteString( text );
	outMsg.WriteString( parm );
	networkSystem->ServerSendReliableMessage( to, outMsg );

	if ( to == -1 || to == localClientNum ) {
		idStr temp = va( "%s%s", common->GetLocalizedString( text ), parm );
		mpGame.AddChatLine( "%s^0: %s", name, temp.c_str() );
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
			char parm[128];

			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );
			// This parameter is ignored - it is only used when going to client from server
			msg.ReadString( parm, sizeof( parm ) );

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
		case GAME_RELIABLE_MESSAGE_CALLPACKEDVOTE: {
			mpGame.ServerCallPackedVote( clientNum, msg );
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

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();

			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_DROP );

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

// RAVEN BEGIN
// jscott: voice comms
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_TEST:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO_TEST: {
			mpGame.ReceiveAndForwardVoiceData( clientNum, msg, id - GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT );
			break;
		}
// ddynerman: stats
		case GAME_RELIABLE_MESSAGE_STAT: {
			int client = msg.ReadByte();	
			statManager->SendStat( clientNum, client );
			break;
		}
// shouchard:  voice chat
		case GAME_RELIABLE_MESSAGE_VOICECHAT_MUTING: {
			int clientDest = msg.ReadByte();
			bool mute = ( 0 != msg.ReadByte() );
			mpGame.ServerHandleVoiceMuting( clientNum, clientDest, mute );
			break;
		}
// shouchard:  server admin
		case GAME_RELIABLE_MESSAGE_SERVER_ADMIN: {
			int commandType = msg.ReadByte();
			int clientNum = msg.ReadByte();
			if ( SERVER_ADMIN_REMOVE_BAN == commandType ) {
				mpGame.HandleServerAdminRemoveBan( "" );
			} else if ( SERVER_ADMIN_KICK == commandType ) {
				mpGame.HandleServerAdminKickPlayer( clientNum );
			} else if ( SERVER_ADMIN_FORCE_SWITCH == commandType ) {
				mpGame.HandleServerAdminForceTeamSwitch( clientNum );
			} else {
				Warning( "Server admin packet with bad type %d", commandType );
			}
			break;
		}
// mekberg: get ban list for server
		case GAME_RELIABLE_MESSAGE_GETADMINBANLIST: {
			ServerSendBanList( clientNum );
			break;
		}
// RAVEN END

		case GAME_RELIABLE_MESSAGE_GETVOTEMAPS: {
			mpGame.SendMapList( clientNum );
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
idGameLocal::RepeaterProcessReliableMessage
================
*/
void idGameLocal::RepeaterProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	int id;

	id = msg.ReadByte();
	switch( id ) {
		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT: {
			if ( !g_noTVChat.GetBool() )
			{
				char name[128], text[128], parm[128];
				idStr suffixed_name, mangled_text;

				msg.ReadString( name, sizeof( name ) );
				msg.ReadString( text, sizeof( text ) );
				msg.ReadString( parm, sizeof( parm ) );

				suffixed_name = va( "^0%s^c777 (viewer)", name );
				mangled_text = va( "^c777%s", text );

				idBitMsg outMsg;
				byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
				outMsg.Init( msgBuf, sizeof( msgBuf ) );
				outMsg.BeginWriting();
				outMsg.WriteByte( GAME_RELIABLE_MESSAGE_CHAT );
				outMsg.WriteString( suffixed_name.c_str() );
				outMsg.WriteString( mangled_text.c_str() );
				outMsg.WriteString( "" );
				networkSystem->RepeaterSendReliableMessage( -1, outMsg );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_VCHAT: {
			break;
		}
		case GAME_RELIABLE_MESSAGE_KILL: {
			break;
		}
		case GAME_RELIABLE_MESSAGE_DROPWEAPON: {
			break;
		}
		case GAME_RELIABLE_MESSAGE_CALLVOTE: {
			break;
		}
		case GAME_RELIABLE_MESSAGE_CASTVOTE: {
			break;
		}
// RAVEN BEGIN
// shouchard:  multivalue votes
		case GAME_RELIABLE_MESSAGE_CALLPACKEDVOTE: {
			break;
		}
// RAVEN END
#if 0
		// uncomment this if you want to track when players are in a menu
		case GAME_RELIABLE_MESSAGE_MENU: {
			bool menuUp = ( msg.ReadBits( 1 ) != 0 );
			break;
		}
#endif
		case GAME_RELIABLE_MESSAGE_EVENT: {
			break;
		}

// RAVEN BEGIN
// jscott: voice comms
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_TEST:
		case GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO_TEST: {
			break;
		}
// ddynerman: stats
		case GAME_RELIABLE_MESSAGE_STAT: {
			break;
		}
// shouchard:  voice chat
		case GAME_RELIABLE_MESSAGE_VOICECHAT_MUTING: {
			break;
		}
// shouchard:  server admin
		case GAME_RELIABLE_MESSAGE_SERVER_ADMIN: {
			break;
		}
// mekberg: get ban list for server
		case GAME_RELIABLE_MESSAGE_GETADMINBANLIST: {
			break;
		}
// RAVEN END

		case GAME_RELIABLE_MESSAGE_GETVOTEMAPS: {
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
idGameLocal::ReadSnapshot
================
*/
void idGameLocal::ClientReadSnapshot( int clientNum, int snapshotSequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg ) {
	int						i, entityDefNumber, numBitsRead;
	idEntity				*ent;
	idPlayer				*player, *spectated;
	pvsHandle_t				pvsHandle;
	idDict					args;
	idBitMsgDelta			deltaMsg;
	snapshot_t				*snapshot;
	entityState_t			*base, *newBase;
	int						spawnId;
	int						numSourceAreas, sourceAreas[ idEntity::MAX_PVS_AREAS ];

	const idDeclEntityDef	*decl;

	if ( net_clientLagOMeter.GetBool() && renderSystem && !(GetDemoState() == DEMO_PLAYING && ( IsServerDemoPlaying() || IsTimeDemo() )) ) {
		lagometer.Update( aheadOfServer, dupeUsercmds );
	}

	InitLocalClient( clientNum );

	gameRenderWorld->DebugClear( time );

	// update the game time
	framenum = gameFrame;
	time = gameTime;
	previousTime = time - GetMSec();

	isNewFrame = true;

	// clear the snapshot entity list
	snapshotEntities.Clear();

	// allocate new snapshot
	snapshot = snapshotAllocator.Alloc();
	snapshot->sequence = snapshotSequence;
	snapshot->firstEntityState = NULL;
	snapshot->next = clientSnapshots[clientNum];
	clientSnapshots[clientNum] = snapshot;

#if ASYNC_WRITE_TAGS
	idRandom tagRandom;
	tagRandom.SetSeed( msg.ReadLong() );
#endif

	ClientReadUnreliableMessages( msg );

#if ASYNC_WRITE_TAGS
	if ( msg.ReadLong() != tagRandom.RandomInt() ) {
		Error( "error after read unreliable" );
	}
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

		deltaMsg.InitReading( base ? &base->state : NULL, &newBase->state, &msg );

		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
		entityDefNumber = deltaMsg.ReadBits( entityDefBits );

		ent = entities[i];

		// if there is no entity or an entity of the wrong type
		if ( !ent || ent->entityDefNumber != entityDefNumber || spawnId != spawnIds[ i ] ) {

			if ( i < MAX_CLIENTS && ent ) {
				// SPAWN_PLAYER should be taking care of spawning the entity with the right spawnId
				common->Warning( "ClientReadSnapshot: recycling client entity %d\n", i );
			}

			delete ent;

			spawnCount = spawnId;

			args.Clear();
			args.SetInt( "spawn_entnum", i );
			args.Set( "name", va( "entity%d", i ) );

			// assume any items spawned from a server-snapshot are in our instance
			if ( gameLocal.GetLocalPlayer() ) {
				args.SetInt( "instance", gameLocal.GetLocalPlayer()->GetInstance() );
			}
			
			assert( entityDefNumber >= 0 );
			if ( entityDefNumber >= declManager->GetNumDecls( DECL_ENTITYDEF ) ) {
				Error( "server has %d entityDefs instead of %d", entityDefNumber, declManager->GetNumDecls( DECL_ENTITYDEF ) );
			}
			decl = static_cast< const idDeclEntityDef * >( declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false ) );
			assert( decl && decl->GetType() == DECL_ENTITYDEF );
			args.Set( "classname", decl->GetName() );
			if ( !SpawnEntityDef( args, &ent ) || !entities[i] ) {
				Error( "Failed to spawn entity with classname '%s' of type '%s'", decl->GetName(), decl->dict.GetString( "spawnclass" ) );
			}

			if ( i < MAX_CLIENTS && i >= numClients ) {
				numClients = i + 1;
			}
		}

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = snapshotSequence;

// RAVEN BEGIN
// bdube: stale network entities
		// Ensure the clipmodel is relinked when transitioning from state
		if ( ent->fl.networkStale ) {
			ent->GetPhysics()->LinkClip();
		}
// RAVEN END

		// read the class specific data from the snapshot
		ent->ReadFromSnapshot( deltaMsg );

		// read the player state from player entities
		if ( ent->entityNumber < MAX_CLIENTS ) {
			if ( deltaMsg.ReadBits( 1 ) == 1 ) {
				assert( ent->IsType( idPlayer::GetClassType() ) );
				static_cast< idPlayer * >( ent )->ReadPlayerStateFromSnapshot( deltaMsg );
			}
		}

		// once we read new snapshot data, unstale the ent
		if( ent->fl.networkStale ) {
			ent->ClientUnstale();
			ent->fl.networkStale = false;
		}
		ent->snapshotBits = msg.GetNumBitsRead() - numBitsRead;

#if ASYNC_WRITE_TAGS
		if ( msg.ReadLong() != tagRandom.RandomInt() ) {
			//cmdSystem->BufferCommandText( CMD_EXEC_NOW, "writeGameState" );
			assert( entityDefNumber >= 0 );
			assert( entityDefNumber < declManager->GetNumDecls( DECL_ENTITYDEF ) );
			const char * classname = declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
			Error( "write to and read from snapshot out of sync for classname '%s'\n", classname );
		}
#endif
	}

	if ( clientNum < MAX_CLIENTS ) {
		player = static_cast<idPlayer *>( entities[ clientNum ] );
	} else {
		player = gameLocal.GetLocalPlayer();
	}

	if ( player->spectating && player->spectator != clientNum && entities[ player->spectator ] ) {
		spectated = static_cast< idPlayer * >( entities[ player->spectator ] );
	} else {
		spectated = player;
	}

	if ( spectated ) {
		// get PVS for this player
		// don't use PVSAreas for networking - PVSAreas depends on animations (and md5 bounds), which are not synchronized
		numSourceAreas = gameRenderWorld->BoundsInAreas( spectated->GetPlayerPhysics()->GetAbsBounds(), sourceAreas, idEntity::MAX_PVS_AREAS );
		pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );
	} else {
		gameLocal.Printf( "Client hasn't spawned self yet.\n" );
		numSourceAreas = gameRenderWorld->BoundsInAreas( idBounds( vec3_origin ), sourceAreas, idEntity::MAX_PVS_AREAS );
		pvsHandle = gameLocal.pvs.SetupCurrentPVS( sourceAreas, numSourceAreas, PVS_NORMAL );
	}

	// read the PVS from the snapshot
#if ASYNC_WRITE_PVS
// zinx - FIXME - this isn't written for server demos or for non-PVS'd repeaters, so we shouldn't read it.
	int serverPVS[idEntity::MAX_PVS_AREAS];
	i = numSourceAreas;
	while ( i < idEntity::MAX_PVS_AREAS ) {
		sourceAreas[ i++ ] = 0;
	}
	for ( i = 0; i < idEntity::MAX_PVS_AREAS; i++ ) {
		serverPVS[ i ] = msg.ReadLong();
	}
	if ( memcmp( sourceAreas, serverPVS, idEntity::MAX_PVS_AREAS * sizeof( int ) ) ) {
		common->Warning( "client PVS areas != server PVS areas, sequence 0x%x", snapshotSequence );
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
// RAVEN BEGIN
// ddynerman: performance profiling
	net_entsInSnapshot += snapshotEntities.Num();
	net_snapshotSize += msg.GetSize();
// RAVEN END
	// add entities in the PVS that haven't changed since the last applied snapshot
	idEntity *nextSpawnedEnt;
	for( ent = spawnedEntities.Next(); ent != NULL; ent = nextSpawnedEnt ) {
		nextSpawnedEnt = ent->spawnNode.Next();

		// local fake client
		if ( ent->entityNumber == ENTITYNUM_NONE ) {
			continue;
		}

		// if the entity is already in the snapshot
		if ( ent->snapshotSequence == snapshotSequence ) {
			continue;
		}

		// if the entity is not in the snapshot PVS
		if ( !( snapshot->pvs[ent->entityNumber >> 5] & ( 1 << ( ent->entityNumber & 31 ) ) ) ) {

			if ( !ent->fl.networkSync ) {
				// don't do stale / unstale on entities that are not marked network sync
				continue;
			}

			if ( ent->PhysicsTeamInPVS( pvsHandle ) ) {
				if ( ent->entityNumber >= MAX_CLIENTS && isMapEntity[ ent->entityNumber ] ) {
					// server says it's not in PVS, client says it's in PVS
					// if that happens on map entities, most likely something is wrong
					// I can see that moving pieces along several PVS could be a legit situation though
					// this is a band aid, which means something is not done right elsewhere
					if ( net_warnStale.GetInteger() > 1 || ( net_warnStale.GetInteger() == 1 && !ent->fl.networkStale ) ) {
						common->Warning( "client thinks map entity 0x%x (%s) is stale, sequence 0x%x", ent->entityNumber, ent->name.c_str(), snapshotSequence );
					}
				}
// RAVEN BEGIN
// bdube: hide while not in snapshot
				if ( !ent->fl.networkStale ) {
					if ( ent->ClientStale() ) {
						delete ent;
						ent = NULL;
					} else {
						ent->fl.networkStale = true;
					}
				}

			} else {
				if ( !ent->fl.networkStale ) {
					if ( ent->ClientStale() ) {
						delete ent;
						ent = NULL;
					} else {
						ent->fl.networkStale = true;
					}
				}
			}
// RAVEN END

			continue;
		}

		// add the entity to the snapshot list
		ent->snapshotNode.AddToEnd( snapshotEntities );
		ent->snapshotSequence = snapshotSequence;
		ent->snapshotBits = 0;

// RAVEN BEGIN
// bdube: hide while not in snapshot
		// Ensure the clipmodel is relinked when transitioning from state
		if ( ent->fl.networkStale ) {
			ent->GetPhysics()->LinkClip();
		}
// RAVEN END

		base = clientEntityStates[clientNum][ent->entityNumber];
		if ( !base ) {
			// entity has probably fl.networkSync set to false
			// non netsynced map entities go in and out of PVS, and may need stale/unstale calls
			if ( ent->fl.networkStale ) {
				ent->ClientUnstale();
				ent->fl.networkStale = false;
			}
			continue;
		}

		if ( !ent->fl.networkSync ) {
			// this is not supposed to happen
			// it did however, when restarting a map with a different inhibit of entities caused entity numbers to be laid differently
			// an idLight would occupy the entity number of an idItem for instance, and although it's not network-synced ( static level light ),
			// the presence of a base would cause the system to think that it is and corrupt things
			// we changed the map population so the entity numbers are kept the same no matter how things are inhibited
			// this code is left as a fall-through fixup / sanity type of thing
			// if this still happens, it's likely "client thinks map entity is stale" is happening as well, and we're still at risk of corruption
			Warning( "ClientReadSnapshot: entity %d of type %s is not networkSync and has a snapshot base", ent->entityNumber, ent->GetType()->classname );
			entityStateAllocator.Free( clientEntityStates[clientNum][ent->entityNumber] );
			clientEntityStates[clientNum][ent->entityNumber] = NULL;
			continue;
		}

		base->state.BeginReading();

		deltaMsg.InitReading( &base->state, NULL, (const idBitMsg *)NULL );
		spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
		entityDefNumber = deltaMsg.ReadBits( entityDefBits );

		// read the class specific data from the base state
		ent->ReadFromSnapshot( deltaMsg );

		// after snapshot read, notify client of unstale
		if ( ent->fl.networkStale ) {
			ent->ClientUnstale();
			ent->fl.networkStale = false;
		}
	}

// RAVEN BEGIN
// ddynerman: add the ambient lights to the snapshot entities
	for( int i = 0; i < ambientLights.Num(); i++ ) {
		ambientLights[ i ]->snapshotNode.AddToEnd( snapshotEntities );
		ambientLights[ i ]->fl.networkStale = false;
	}
// RAVEN END

	// free the PVS
	pvs.FreeCurrentPVS( pvsHandle );

	// read the game state from the snapshot
	base = clientEntityStates[clientNum][ENTITYNUM_NONE];	// ENTITYNUM_NONE is used for the game state
	if ( base ) {
		base->state.BeginReading();
	}
	newBase = entityStateAllocator.Alloc();
	newBase->entityNumber = ENTITYNUM_NONE;
	newBase->next = snapshot->firstEntityState;
	snapshot->firstEntityState = newBase;
	newBase->state.Init( newBase->stateBuf, sizeof( newBase->stateBuf ) );
	newBase->state.BeginWriting();
	deltaMsg.InitReading( base ? &base->state : NULL, &newBase->state, &msg );

	ReadGameStateFromSnapshot( deltaMsg );

	// visualize the snapshot
	if ( clientNum < MAX_CLIENTS ) {
		ClientShowSnapshot( clientNum );
	} else {
		// FIXME
	}

	// process entity events
	ClientProcessEntityNetworkEventQueue();

}

/*
===============
idGameLocal::ClientReadServerDemoSnapshot
===============
*/
void idGameLocal::ClientReadServerDemoSnapshot( int sequence, const int gameFrame, const int gameTime, const idBitMsg &msg ) {
	bool				ret = ClientApplySnapshot( MAX_CLIENTS, sequence - 1 );
	ret = ret; // shut the warning
	assert( ret || sequence == 1 ); // past the first snapshot of the server demo stream, there's always exactly one to clear

	ClientReadSnapshot( MAX_CLIENTS, sequence, gameFrame, gameTime, 0, 0, msg );
}

/*
===============
idGameLocal::ClientReadRepeaterSnapshot
===============
*/
void idGameLocal::ClientReadRepeaterSnapshot( int sequence, const int gameFrame, const int gameTime, const int aheadOfServer, const idBitMsg &msg ) {
	ClientReadSnapshot( MAX_CLIENTS, sequence, gameFrame, gameTime, 0, aheadOfServer, msg );
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

	while ( eventQueue.Start() ) {
		event = eventQueue.Start();

		// only process forward, in order
		if ( event->time > time ) {
			break;
		}

		idEntityPtr< idEntity > entPtr;
			
		if ( !entPtr.SetSpawnId( event->spawnId ) ) {
			if( !gameLocal.entities[ event->spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
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

#ifdef _DEBUG
		entityNetEvent_t* freedEvent = eventQueue.Dequeue();
		assert( freedEvent == event );
#else
		eventQueue.Dequeue();
#endif
		eventQueue.Free( event );
	}
}

// RAVEN BEGIN
// bdube: client side hitscan

/*
================
idGameLocal::ClientHitScan
================
*/
void idGameLocal::ClientHitScan( const idBitMsg &msg ) {
	int				hitscanDefIndex;
	idVec3			muzzleOrigin;
	idVec3			dir;
	idVec3			fxOrigin;
	const idDeclEntityDef *decl;
	int				num_hitscans;
	int				i;
	idEntity		*owner;

	assert( isClient );

	hitscanDefIndex = msg.ReadLong();
	decl = static_cast< const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, hitscanDefIndex ) );
	if ( !decl ) {
		common->Warning( "idGameLocal::ClientHitScan: entity def index %d not found\n", hitscanDefIndex );
		return;
	}
	num_hitscans = decl->dict.GetInt( "hitscans", "1" );

	owner = entities[ msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) ) ];	

	muzzleOrigin[0] = msg.ReadFloat();
	muzzleOrigin[1] = msg.ReadFloat();
	muzzleOrigin[2] = msg.ReadFloat();
	fxOrigin[0] = msg.ReadFloat();
	fxOrigin[1] = msg.ReadFloat();
	fxOrigin[2] = msg.ReadFloat();

	// one direction sent per hitscan
	for( i = 0; i < num_hitscans; i++ ) {
		dir = msg.ReadDir( 24 );
		gameLocal.HitScan( decl->dict, muzzleOrigin, dir, fxOrigin, owner );
	}
}

// RAVEN END

/*
================
idGameLocal::ClientProcessReliableMessage
================
*/
void idGameLocal::ClientProcessReliableMessage( int clientNum, const idBitMsg &msg ) {
	int			id;
	idDict		backupSI;
	int			toClient = -1, excludeClient = -1;
	bool		noEffect = false;

	InitLocalClient( clientNum );

	if ( IsServerDemoPlaying() ) {
		int record_type = msg.ReadByte();
		assert( record_type < DEMO_RECORD_COUNT );
		// if you need to do some special filtering:
		switch ( record_type ) {
		case DEMO_RECORD_CLIENTNUM: {
			toClient = msg.ReadChar();
			if ( toClient >= 0 && toClient < MAX_CLIENTS ) {
				// reliable was targetted
				if ( toClient != followPlayer ) {
					// we're free flying or following someone else
					if ( !isRepeater ) {
						return;
					} else {
						noEffect = true;
					}
				}
			} else {
				toClient = -1;
			}
			break;
		}
		case DEMO_RECORD_EXCLUDE: {
			excludeClient = msg.ReadChar(); // may be -1
			if ( excludeClient == followPlayer ) {
				if ( !isRepeater ) {
					return;
				} else {
					noEffect = true;
				}
			}
			toClient = -1;
			break;
		}
		}
	}

	id = msg.ReadByte();

	if ( isRepeater ) {
		bool		inhibitRepeater = false;

		switch( id ) {
			case GAME_RELIABLE_MESSAGE_SPAWN_PLAYER: {
				if ( toClient != -1 ) {
					inhibitRepeater = true;
				}
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_PORTALSTATES: {
				inhibitRepeater = true;
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_STARTSTATE: {
				if ( toClient != -1 ) {
					inhibitRepeater = true;
				}
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_GAMESTATE: {
				if ( toClient != -1 ) {
					inhibitRepeater = true;
				}
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_SET_INSTANCE: {
				inhibitRepeater = true;
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_GETVOTEMAPS: {
				inhibitRepeater = true;
				break;
			}
	
			case GAME_RELIABLE_MESSAGE_UNRELIABLE_MESSAGE: {
				inhibitRepeater = true;
				break;
			}

			default:
				break;
		}

		if ( !inhibitRepeater ) {
			// pass it along to the connected viewers
			if ( excludeClient == -1 ) {
				networkSystem->RepeaterSendReliableMessage( -1, msg, IsServerDemoPlaying(), toClient );
			} else {
				networkSystem->RepeaterSendReliableMessageExcluding( excludeClient, msg, IsServerDemoPlaying() );
			}
		}

		if ( noEffect ) {
			return;
		}
	}

	switch( id ) {
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
			if( entPtr.GetEntity() && entPtr.GetEntity()->entityNumber < MAX_CLIENTS ) {
				delete entPtr.GetEntity();
				gameLocal.mpGame.UpdatePlayerRanks();
			} else {
				delete entPtr.GetEntity();
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_CHAT:
		case GAME_RELIABLE_MESSAGE_TCHAT: {
			char name[128];
			char text[128];
			char parm[128];
			msg.ReadString( name, sizeof( name ) );
			msg.ReadString( text, sizeof( text ) );
			msg.ReadString( parm, sizeof( parm ) );
			idStr temp = va( "%s^0: %s%s\n", name, common->GetLocalizedString( text ), parm );
			mpGame.PrintChatLine( temp.c_str(), ( id == GAME_RELIABLE_MESSAGE_TCHAT && gameLocal.IsTeamGame() ) );
			break;
		}
		case GAME_RELIABLE_MESSAGE_DB: {
			msg_evt_t msg_evt = (msg_evt_t)msg.ReadByte();
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

			event->spawnId = msg.ReadBits( 32 );
			event->event = msg.ReadByte();
			event->time = msg.ReadLong();

			eventQueue.Enqueue( event, idEventQueue::OUTOFORDER_IGNORE );

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
			SetServerInfo( info );
			break;
		}
		case GAME_RELIABLE_MESSAGE_RESTART: {
			MapRestart();
			break;
		}
		case GAME_RELIABLE_MESSAGE_STARTVOTE: {
			char voteString[ MAX_STRING_CHARS ];
			int clientNum = msg.ReadByte( );
			msg.ReadString( voteString, sizeof( voteString ) );
			mpGame.ClientStartVote( clientNum, voteString );
			break;
		}
		case GAME_RELIABLE_MESSAGE_PRINT: {
			char str[ MAX_PRINT_LEN ] = { '\0' };
			msg.ReadString( str, MAX_PRINT_LEN );
			mpGame.PrintMessage( -1, str );
			break;
		}
		case GAME_RELIABLE_MESSAGE_STARTPACKEDVOTE: {
			voteStruct_t voteData;
			memset( &voteData, 0, sizeof( voteData ) );
			int clientNum = msg.ReadByte();
			voteData.m_fieldFlags = msg.ReadShort();
			char mapName[256];
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_KICK ) ) {
				voteData.m_kick = msg.ReadByte();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_MAP ) ) {
				msg.ReadString( mapName, sizeof( mapName ) );
				voteData.m_map = mapName;
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_GAMETYPE ) ) {
				voteData.m_gameType = msg.ReadByte();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TIMELIMIT ) ) {
				voteData.m_timeLimit = msg.ReadByte();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_FRAGLIMIT ) ) {
				voteData.m_fragLimit = msg.ReadShort();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TOURNEYLIMIT ) ) {
				voteData.m_tourneyLimit = msg.ReadShort();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CAPTURELIMIT ) ) {
				voteData.m_captureLimit = msg.ReadShort();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_BUYING ) ) {
				voteData.m_buying = msg.ReadByte();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_TEAMBALANCE ) ) {
				voteData.m_teamBalance = msg.ReadByte();
			}
			if ( 0 != ( voteData.m_fieldFlags & VOTEFLAG_CONTROLTIME ) ) {
				voteData.m_controlTime = msg.ReadShort();
			}
			mpGame.ClientStartPackedVote( clientNum, voteData );
			break;
		}
		case GAME_RELIABLE_MESSAGE_UPDATEVOTE: {
			int result = msg.ReadByte( );
			int yesCount = msg.ReadByte( );
			int noCount = msg.ReadByte( );
			int multiVote = msg.ReadByte( );
			voteStruct_t voteData;
			char mapNameBuffer[256];
			memset( &voteData, 0, sizeof( voteData ) );
			if ( multiVote ) {
				voteData.m_fieldFlags = msg.ReadShort();
				voteData.m_kick = msg.ReadByte();
				msg.ReadString( mapNameBuffer, sizeof( mapNameBuffer ) );
				voteData.m_map = mapNameBuffer;
				voteData.m_gameType = msg.ReadByte();
				voteData.m_timeLimit = msg.ReadByte();
				voteData.m_fragLimit = msg.ReadShort();
				voteData.m_tourneyLimit = msg.ReadShort();
				voteData.m_captureLimit = msg.ReadShort();
				voteData.m_buying = msg.ReadByte();
				voteData.m_teamBalance = msg.ReadByte();
				voteData.m_controlTime = msg.ReadShort();
			}
			mpGame.ClientUpdateVote( (idMultiplayerGame::vote_result_t)result, yesCount, noCount, voteData );
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
		case GAME_RELIABLE_MESSAGE_ITEMACQUIRESOUND:
			mpGame.PlayGlobalItemAcquireSound( msg.ReadBits ( gameLocal.entityDefBits ) );
			break;
		case GAME_RELIABLE_MESSAGE_DEATH: {
			int attackerEntityNumber = msg.ReadByte( );
			int attackerScore = -1;
			if( attackerEntityNumber >= 0 && attackerEntityNumber < MAX_CLIENTS ) {
				attackerScore = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
			}
			int victimEntityNumber = msg.ReadByte( );
			int victimScore = -1;
			if( victimEntityNumber >= 0 && victimEntityNumber < MAX_CLIENTS ) {
				victimScore = msg.ReadBits( ASYNC_PLAYER_FRAG_BITS );
			}

			idPlayer* attacker = (attackerEntityNumber != 255 ? static_cast<idPlayer*>(gameLocal.entities[ attackerEntityNumber ]) : NULL);
			idPlayer* victim = (victimEntityNumber != 255 ? static_cast<idPlayer*>(gameLocal.entities[ victimEntityNumber ]) : NULL);
			int methodOfDeath = msg.ReadByte( );
			bool quadKill = msg.ReadBits( 1 ) != 0;
		
			mpGame.ReceiveDeathMessage( attacker, attackerScore, victim, victimScore, methodOfDeath, quadKill );
			break;
		}
		case GAME_RELIABLE_MESSAGE_GAMESTATE: {
			mpGame.GetGameState()->ReceiveState( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_STAT: {
			statManager->ReceiveStat( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_ALL_STATS: {
			statManager->ReceiveAllStats( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_SET_INSTANCE: {
			if ( !IsServerDemoPlaying() ) {
				mpGame.ClientSetInstance( msg );
			}
			break;
		}
		case GAME_RELIABLE_MESSAGE_INGAMEAWARD: {
			statManager->ReceiveInGameAward( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_GETADMINBANLIST: {
			mpBanInfo_t banInfo;
			char name[MAX_STRING_CHARS];
			char guid[MAX_STRING_CHARS];

			FlushBanList( );
			while ( msg.ReadString( name, MAX_STRING_CHARS ) && msg.ReadString( guid, MAX_STRING_CHARS ) ) {
				banInfo.name = name;
				strncpy( banInfo.guid, guid, CLIENT_GUID_LENGTH );
				banList.Append( banInfo );
			}

			break;
		}
		case GAME_RELIABLE_MESSAGE_GETVOTEMAPS: {
			mpGame.ReadMapList( msg );
			break;
		}
		case GAME_RELIABLE_MESSAGE_UNRELIABLE_MESSAGE: {
			idBitMsg unreliable;
			unreliable.Init( msg.GetReadData(), msg.GetRemainingData() );
			unreliable.SetSize( msg.GetRemainingData() );
			unreliable.BeginReading();
			ProcessUnreliableMessage( unreliable );
			break;
		}
		case GAME_RELIABLE_MESSAGE_EVENT_ACK: {
			clientAckSequence = msg.ReadLong();
			// make sure the local client is waiting for a ACK
			// if he's not, this may indicate a problem
			idPlayer *p = GetLocalPlayer();
			if ( net_clientPredictWeaponSwitch.GetBool() && p != NULL && !p->IsWaitingForPredictAck() ) {
				common->Warning( "frame %d: got EVENT_ACK seq %d while not expecting one", gameLocal.framenum, clientAckSequence );
			}
			break;
		}
		default: {
			Error( "Unknown server->client reliable message: %d", id );
			break;
		}
	}
}

// RAVEN BEGIN
/*
================
idGameLocal::ClientRun
Called once each client render frame (before any ClientPrediction frames have been run)
================
*/
void idGameLocal::ClientRun( void ) {
	if( isMultiplayer ) {
		mpGame.ClientRun();
	}
}

/*
================
idGameLocal::ClientEndFrame
Called once each client render frame (after all ClientPrediction frames have been run)
================
*/
void idGameLocal::ClientEndFrame( void ) {
	if( isMultiplayer ) {
		mpGame.ClientEndFrame();
	}
}

/*
================
idGameLocal::ProcessRconReturn
================
*/
void idGameLocal::ProcessRconReturn( bool success )	{
	if( isMultiplayer )	{
		mpGame.ProcessRconReturn( success );
	}
}

/*
================
idGameLocal::ResetGuiRconStatus
================
*/
void idGameLocal::ResetRconGuiStatus( void )	{
	if( isMultiplayer )	{
		mpGame.ResetRconGuiStatus( );
	}
}

// RAVEN END

/*
================
idGameLocal::ClientPrediction
server demos: clientNum == MAX_CLIENTS
================
*/
gameReturn_t idGameLocal::ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame, ClientStats_t *cs ) {
	idEntity		*ent;
	idPlayer		*player;	// may be NULL when predicting for a server demo
	gameReturn_t	ret;

	ret.sessionCommand[ 0 ] = '\0';

	if ( g_showDebugHud.GetInteger() && net_entsInSnapshot && net_snapshotSize) {
		gameDebug.SetInt( "snap_ents", net_entsInSnapshot );
		gameDebug.SetInt( "snap_size", net_snapshotSize );

		net_entsInSnapshot = 0;
		net_snapshotSize = 0; 
	}

	if ( clientNum == localClientNum ) {
		gameDebug.BeginFrame();
		gameLog->BeginFrame( time );
	}

	isLastPredictFrame = lastPredictFrame;

	InitLocalClient( clientNum );

	// update the game time
	framenum++;
	previousTime = time;
	time += GetMSec();

	// update the real client time and the new frame flag
	if ( time > realClientTime ) {
		realClientTime = time;
		isNewFrame = true;
	} else {
		isNewFrame = false;
	}

	if ( clientNum == MAX_CLIENTS ) {
		player = gameLocal.GetLocalPlayer();
		assert( player );
	} else {
		player = static_cast<idPlayer *>( entities[clientNum] );
	}

	// check for local client lag
	if ( player ) {
		if ( networkSystem->ClientGetTimeSinceLastPacket() >= net_clientMaxPrediction.GetInteger() ) {
			player->isLagged = true;
		} else {
			player->isLagged = false;
		}
	}

	if ( cs ) {
		cs->isLastPredictFrame = isLastPredictFrame;
		cs->isLagged = player ? player->isLagged : false;
		cs->isNewFrame = isNewFrame;
	}

	// set the user commands for this frame
	usercmds = clientCmds;

	// run prediction on all entities from the last snapshot
	for ( ent = snapshotEntities.Next(); ent != NULL; ent = ent->snapshotNode.Next() ) {
		// don't force TH_PHYSICS on, only call ClientPredictionThink if thinkFlags != 0
		// it's better to synchronize TH_PHYSICS on specific entities when needed ( movers may be trouble )
		if ( ent->thinkFlags != 0 ) {
			ent->ClientPredictionThink();
		}
	}

	// run client entities
	if ( isNewFrame ) {
		// rjohnson: only run the entire logic when it is a new frame
		rvClientEntity* cent;
		for ( cent = clientSpawnedEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
			cent->Think();
		}
	}

	// freeView
	if ( clientNum == MAX_CLIENTS && player && isNewFrame && !isRepeater) {
		assert( player->IsFakeClient() );

		player->ClientPredictionThink();
		player->UpdateVisuals();

		assert( player->spectating );

		if ( player->spectator != player->entityNumber ) {
			followPlayer = player->spectator;
		} else {
			followPlayer = -1;
		}
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
		sessionCommand = "";
	}

// RAVEN BEGIN
// ddynerman: client logging/debugging
	if ( clientNum == localClientNum ) {
		gameDebug.EndFrame();
		gameLog->EndFrame();
	}
// RAVEN END

	g_simpleItems.ClearModified();
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
	} else {
		// 2: table of pak URLs
		// 3: table of pak URLs with built-in http server
		// first token is the game pak if requested, empty if not requested by the client
		// there may be empty tokens for paks the server couldn't pinpoint - the order matters
		idStr 		reply = "2;";
		idStrList	dlTable, pakList;
		bool		matchAll = false;
		int			i, j;

		if ( !idStr::Icmp( cvarSystem->GetCVarString( "net_serverDlTable" ), "*" ) ) {
			matchAll = true;
		} else {
			Tokenize( dlTable, cvarSystem->GetCVarString( "net_serverDlTable" ) );
		}
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

			idStr url = cvarSystem->GetCVarString( "net_serverDlBaseURL" );

			if ( !url.Length() ) {
				if ( cvarSystem->GetCVarInteger( "net_serverDownload" ) == 2 ) {
					common->Warning( "net_serverDownload == 2 and net_serverDlBaseURL not set" );
				} else {
					url = cvarSystem->GetCVarString( "net_httpServerBaseURL" );
				}
			}
			
			if ( matchAll ) {
				url.AppendPath( pakList[i] );
				reply += url;
				common->Printf( "download for %s: %s\n", IP, url.c_str() );
			} else {
				for ( j = 0; j < dlTable.Num(); j++ ) {
					if ( !pakList[ i ].Icmp( dlTable[ j ] ) ) {
						break;
					}
				}
				if ( j == dlTable.Num() ) {
					common->Printf( "download for %s: pak not matched: %s\n", IP, pakList[ i ].c_str() );
				} else {
					url.AppendPath( dlTable[ j ] );
					reply += url;
					common->Printf( "download for %s: %s\n", IP, url.c_str() );
				}
			}
		}

		idStr::Copynz( urls, reply, MAX_STRING_CHARS );
		return true;
	}
}

/*
===============
idGameLocal::HTTPRequest
===============
*/
bool idGameLocal::HTTPRequest( const char *IP, const char *file, bool isGamePak ) {
	idStrList	dlTable;
	int			i;

	if ( !idStr::Icmp( cvarSystem->GetCVarString( "net_serverDlTable" ), "*" ) ) {
		return true;
	}

	Tokenize( dlTable, cvarSystem->GetCVarString( "net_serverDlTable" ) );
	while ( *file == '/' ) ++file; // net_serverDlTable doesn't include the initial /

	for ( i = 0; i < dlTable.Num(); i++ ) {
		if ( !dlTable[ i ].Icmp( file ) ) {
			return true;
		}
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
	Init();
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
	if ( behaviour == OUTOFORDER_IGNORE ) {
		// warn if an event comes in with a time before the last one in queue
		// as that would cause the one we're adding to get delayed processing
		if ( end && end->time > event->time ) {
			common->Warning( "new event with id %d (time %d) is earlier than head event with id %d (time %d) - is OUTOFORDER_SORT needed?", event->event, event->time, end->event, end->time );
		}
	}

	if ( behaviour == OUTOFORDER_DROP ) {
		// go backwards through the queue and determine if there are
		// any out-of-order events
		while ( end && end->time > event->time ) {
			entityNetEvent_t *outOfOrder = RemoveLast();
			common->Warning( "new event with id %d (time %d) caused removal of event with id %d (time %d), game time = %d", event->event, event->time, outOfOrder->event, outOfOrder->time, gameLocal.time );
			Free( outOfOrder );
		}
	} else if ( behaviour == OUTOFORDER_SORT && end != NULL ) {
		entityNetEvent_t *cur = end;
		// iterate until we find a time < the new event's
		while ( cur && cur->time > event->time ) {
			cur = cur->prev;
		}
		if ( cur == end ) {
			end = event;
		}
		if ( !cur ) {
			// add to start
			event->next = start;
			event->prev = NULL;
			start->prev = event;	// start cannot be NULL because end is already != NULL
			start = event;
		} else {
			// insert
			event->prev = cur;
			event->next = cur->next;
			if ( cur->next != NULL ) {	// insert possibly at the end so have to check that
				cur->next->prev = event;
			}
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

// RAVEN BEGIN
// shouchard:  ban list stuff here

/*
================
idGameLocal::LoadBanList
================
*/
void idGameLocal::LoadBanList() {

	// open file
	idStr token;
	idFile *banFile = fileSystem->OpenFileRead( BANLIST_FILENAME );
	mpBanInfo_t banInfo;
	if ( NULL == banFile ) {
		common->DPrintf( "idGameLocal::LoadBanList:  unable to open ban list file!\n" ); // fixme:  need something better here
		return;
	}

	// parse file (read three consecutive strings per banInfo (real complex ;) ) )
	while ( banFile->ReadString( token ) > 0 ) {
		// name
		banInfo.name = token;

		// guid
		if ( banFile->ReadString( token ) > 0 && token.Length() >= 11 ) {
			idStr::Copynz( banInfo.guid, token.c_str(), CLIENT_GUID_LENGTH );

			banList.Append( banInfo );
			continue;
		}

		gameLocal.Warning( "idGameLocal::LoadBanList:  Potential curruption of banlist file (%s).", BANLIST_FILENAME );
	}
	fileSystem->CloseFile( banFile );

	banListLoaded = true;
	banListChanged = false;
}

/*
================
idGameLocal::SaveBanList
================
*/
void idGameLocal::SaveBanList() {
	if ( !banListChanged ) {
		return;
	}

	// open file
	idFile *banFile = fileSystem->OpenFileWrite( BANLIST_FILENAME );
	if ( NULL == banFile ) {
		common->DPrintf( "idGameLocal::SaveBanList:  unable to open ban list file!\n" ); // fixme:  need something better here
		return;
	}

	for ( int i = 0; i < banList.Num(); i++ ) {
		const mpBanInfo_t& banInfo = banList[ i ];
		char temp[ 16 ] = { 0, };
		banFile->WriteString( va( "%s", banInfo.name.c_str() ) );
		idStr::Copynz( temp, banInfo.guid, CLIENT_GUID_LENGTH );
		banFile->WriteString( temp );
//		idStr::Copynz( temp, (const char*)banInfo.ip, 15 );
//		banFile->WriteString( "255.255.255.255" );
	}
	fileSystem->CloseFile( banFile );
	banListChanged = false;
}

/*
================
idGameLocal::FlushBanList
================
*/
void idGameLocal::FlushBanList() {
	banList.Clear();
	banListLoaded = false;
	banListChanged = false;
}

/*
================
idGameLocal::IsPlayerBanned
================
*/
bool idGameLocal::IsPlayerBanned( const char *name ) {
	assert( name );

	if ( !banListLoaded ) {
		LoadBanList();
	}

	// check vs. each line in the list, if we found one return true
	for ( int i = 0; i < banList.Num(); i++ ) {
		if ( 0 == idStr::Icmp( name, banList[ i ].name ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idGameLocal::IsGuidBanned
================
*/
bool idGameLocal::IsGuidBanned( const char *guid ) {
	assert( guid );

	if ( !banListLoaded ) {
		LoadBanList();
	}

	// check vs. each line in the list, if we found one return true
	for ( int i = 0; i < banList.Num(); i++ ) {
		if ( 0 == idStr::Icmp( guid, banList[ i ].guid ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idGameLocal::AddGuidToBanList
================
*/
void idGameLocal::AddGuidToBanList( const char *guid ) {
	assert( guid );

	if ( !banListLoaded ) {
		LoadBanList();
	}

	mpBanInfo_t banInfo;
	char name[ 512 ];	// TODO: clean this up
	gameLocal.GetPlayerName( gameLocal.GetClientNumByGuid( guid ), name );
	banInfo.name = name;
	idStr::Copynz( banInfo.guid, guid, CLIENT_GUID_LENGTH );
//	SIMDProcessor->Memset( banInfo.ip, 0xFF, 15 );
	banList.Append( banInfo );
	banListChanged = true;
}

/*
================
idGameLocal::RemoveGuidFromBanList
================
*/
void idGameLocal::RemoveGuidFromBanList( const char *guid ) {
	assert( guid );

	if ( !banListLoaded ) {
		LoadBanList();
	}

	// check vs. each line in the list, if we find a match remove it.
	for ( int i = 0; i < banList.Num(); i++ ) {
		if ( 0 == idStr::Icmp( guid, banList[ i ].guid ) ) {
			banList.RemoveIndex( i );
			banListChanged = true;
			return;
		}
	}
}

/*
================
idGameLocal::RegisterClientGuid
================
*/
void idGameLocal::RegisterClientGuid( int clientNum, const char *guid ) {
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );
	assert( guid );
	memset( clientGuids[ clientNum ], 0, CLIENT_GUID_LENGTH ); // just in case
	idStr::Copynz( clientGuids[ clientNum ], guid, CLIENT_GUID_LENGTH );
}

/*
================
idGameLocal::GetBanListCount
================
*/
int idGameLocal::GetBanListCount() {
	if ( !banListLoaded ) {
		LoadBanList();
	}

	return banList.Num();
}

/*
================
idGameLocal::GetBanListEntry
================
*/
const mpBanInfo_t* idGameLocal::GetBanListEntry( int entry ) {
	if ( !banListLoaded ) {
		LoadBanList();
	}

	if ( entry < 0 || entry >= banList.Num() ) {
		return NULL;
	}

	return &banList[ entry ];
}

/*
================
idGameLocal::GetGuidByClientNum
================
*/
const char *idGameLocal::GetGuidByClientNum( int clientNum ) {
	assert( clientNum >= 0 && clientNum < numClients );

	return clientGuids[ clientNum ];
}

/*
================
idGameLocal::GetClientNumByGuid
================
*/
int idGameLocal::GetClientNumByGuid( const char * guid ) {
	assert( guid );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !idStr::Icmp( networkSystem->GetClientGUID( i ), guid ) ) {
			return i;
		}
	}
	return -1;
}

// mekberg: send ban list to client
/*
================
idGameLocal::ServerSendBanList
================
*/
void idGameLocal::ServerSendBanList( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GETADMINBANLIST ) ;

	if ( !banListLoaded ) {
		LoadBanList();
	}
	
	int i;
	int c = banList.Num();
	for ( i = 0; i < c; i++ ) {
		outMsg.WriteString( banList[ i ].name.c_str() );
		outMsg.WriteString( banList[ i ].guid, CLIENT_GUID_LENGTH );
	}

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );
}

// mekberg: so we can populate ban list outside of multiplayer game
/*
===================
idGameLocal::PopulateBanList
===================
*/
void idGameLocal::PopulateBanList( idUserInterface* hud ) {
	if ( !hud ) {
		return;
	}

	int bans = GetBanListCount();
	for ( int i = 0; i < bans; i++ ) {
		const mpBanInfo_t * banInfo = GetBanListEntry( i );
		hud->SetStateString( va( "sa_banList_item_%d", i ), va( "%d:  %s\t%s", i+1, banInfo->name.c_str(), banInfo->guid ) );
	}
	hud->DeleteStateVar( va( "sa_banList_item_%d", bans ) );
	hud->SetStateString( "sa_banList_sel_0", "-1" );
	// used to trigger a redraw, was slow, and doesn't seem to do anything so took it out. fixes #13675
	hud->StateChanged( gameLocal.time, false );
}
// RAVEN END

/*
================
idGameLocal::ServerSendInstanceReliableMessageExcluding
Works like networkSystem->ServerSendReliableMessageExcluding, but only sends to entities in the owner's instance
================
*/
void idGameLocal::ServerSendInstanceReliableMessageExcluding( const idEntity* owner, int excludeClient, const idBitMsg& msg ) {
	int i;
	assert( isServer );

	if ( owner == NULL ) {
		assert( false ); // does not happen in q4mp, previous revisions of the code would have crashed
		networkSystem->ServerSendReliableMessageExcluding( excludeClient, msg );	// NOTE: will also transmit through repeater
		return;
	}

	for ( i = 0; i < numClients; i++ ) {
		if ( i == excludeClient ) {
			continue;
		}

		if ( entities[ i ] == NULL ) {
			continue;
		}

		if ( entities[ i ]->GetInstance() != owner->GetInstance() ) {
			continue;
		}

		networkSystem->ServerSendReliableMessage( i, msg, true );
	}

	if ( owner->GetInstance() == 0 ) {
		// record this message into the demo client if server demo is active
		// the message will also go in the server demo data through the above individual client ServerSendReliableMessage,
		// but will be ignored on replay unless you are following that particular client
		networkSystem->ServerSendReliableMessage( MAX_CLIENTS, msg, true );

		networkSystem->RepeaterSendReliableMessageExcluding( excludeClient, msg );
	}
}

/*
================
idGameLocal::ServerSendInstanceReliableMessage
Works like networkSystem->ServerSendReliableMessage, but only sends to entities in the owner's instance
================
*/
void idGameLocal::ServerSendInstanceReliableMessage( const idEntity* owner, int clientNum, const idBitMsg& msg ) {
	int i;
	assert( isServer );

	if ( owner == NULL ) {
		networkSystem->ServerSendReliableMessage( clientNum, msg );	// NOTE: will also transmit through repeater
		return;
	}

	if ( clientNum == -1 ) {
		for ( i = 0; i < numClients; i++ ) {
			if ( entities[ i ] == NULL ) {
				continue;
			}

			if ( entities[ i ]->GetInstance() != owner->GetInstance() ) {
				continue;
			}

			networkSystem->ServerSendReliableMessage( i, msg, true );
		}

		if ( owner->GetInstance() == 0 ) {
			// see ServerSendInstanceReliableMessageExcluding
			networkSystem->ServerSendReliableMessage( MAX_CLIENTS, msg, true );
			networkSystem->RepeaterSendReliableMessage( -1, msg );
		}
	} else {
		assert( false ); // not used in q4mp
		if ( entities[ clientNum ] && entities[ clientNum ]->GetInstance() == owner->GetInstance() ) {
			networkSystem->ServerSendReliableMessage( clientNum, msg, true );

			if ( owner->GetInstance() == 0 ) {
				for ( i = 0; i < maxViewer; i++ ) {
					if ( !viewers[ i ].active ) {
						continue;
					}

					if ( viewers[ i ].origin.followClient != clientNum && !viewers[ i ].nopvs ) {
						continue;
					}

					networkSystem->RepeaterSendReliableMessage( clientNum, msg, false, i );
				}
			}
		}
	}
}

/*
===============
idGameLocal::RepeaterAppendUnreliableMessage
===============
*/
void idGameLocal::RepeaterAppendUnreliableMessage( int icl, const idBitMsg &msg, const idBitMsg *header ) {
	assert( isRepeater );

	// send the normally unreliable message as a reliable message
	if ( ( viewers[ icl ].nopvs && ( cvarSystem->GetCVarInteger( "g_repeaterReliableOnly" ) >= 1 ) ) || ( cvarSystem->GetCVarInteger( "g_repeaterReliableOnly" ) >= 2 ) ) {
		byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];
		idBitMsg reliable;

		reliable.Init( msgBuf, sizeof( msgBuf ) );
		reliable.WriteByte( GAME_RELIABLE_MESSAGE_UNRELIABLE_MESSAGE );
		if ( header ) {
			reliable.WriteData( header->GetData(), header->GetSize() );
		}
		reliable.WriteData( msg.GetData(), msg.GetSize() );

		networkSystem->RepeaterSendReliableMessage( icl, reliable );
		return;
	}

	// send the message
	if ( header ) {
		viewerUnreliableMessages[ icl ].AddConcat( header->GetData(), header->GetSize(), msg.GetData(), msg.GetSize(), false );
	} else {
		viewerUnreliableMessages[ icl ].Add( msg.GetData(), msg.GetSize(), false );
	}
}

/*
===============
idGameLocal::RepeaterUnreliableMessage
for spectating support, we have to loop through the clients and emit to the spectator client too
note that a clientNum == -1 means send to everyone
header and msg are concatenated in case a header is needed.
===============
*/
void idGameLocal::RepeaterUnreliableMessage( const idBitMsg &msg, const int clientNum, const idBitMsg *header ) {
	if ( !isRepeater ) {
		return;
	}

	int icl;

	for ( icl = 0; icl < maxViewer; icl++ ) {
		if ( !viewers[ icl ].active ) {
			continue;
		}

		if ( clientNum != -1 && viewers[ icl ].origin.followClient != clientNum && !viewers[ icl ].nopvs ) {
			// not global, not to the followed client

			// not following anyone
			if ( viewers[ icl ].origin.followClient == -1 ) {
				continue;
			}
	
			// the followed player doesn't exist
			if ( !entities[ viewers[ icl ].origin.followClient ] ) {
				continue;
			}
	
			// the followed player isn't a player
			if ( !entities[ viewers[ icl ].origin.followClient ]->IsType( idPlayer::GetClassType() ) ) {
				continue;
			}
	
			// following a valid player
	
			idPlayer *player = static_cast< idPlayer * >( entities[ viewers[ icl ].origin.followClient ] );
			if ( !player->spectating || viewers[ icl ].origin.followClient != player->spectator ) {
				// .. but not the right player
				continue;
			}

			// fallthrough
		}

		RepeaterAppendUnreliableMessage( icl, msg, header );
	}
}

/*
===============
idGameLocal::SendUnreliableMessage
for spectating support, we have to loop through the clients and emit to the spectator client too
note that a clientNum == -1 means send to everyone
===============
*/
void idGameLocal::SendUnreliableMessage( const idBitMsg &msg, const int clientNum ) {
	int icl;
	idPlayer *player;
	
	for ( icl = 0; icl < numClients; icl++ ) {
		if ( icl == localClientNum ) {
			// not to local client
			// note that if local is spectated he will still get it
			continue;
		}
		if ( !entities[ icl ] ) {
			continue;
		}
		if ( icl != clientNum ) {
			player = static_cast< idPlayer * >( entities[ icl ] );
			// drop all clients except the ones that follow the client we emit to
			if ( !player->spectating || player->spectator != clientNum ) {
				continue;
			}
		}
		unreliableMessages[ icl ].Add( msg.GetData(), msg.GetSize(), false );
	}

	if ( demoState == DEMO_RECORDING || isRepeater ) {
		// record the type and destination for remap on readback
		idBitMsg	dest;
		byte		msgBuf[ 16 ];
		
		dest.Init( msgBuf, sizeof( msgBuf ) );
		dest.WriteByte( GAME_UNRELIABLE_RECORD_CLIENTNUM );
		dest.WriteChar( clientNum );
		
		if ( demoState == DEMO_RECORDING ) {
			unreliableMessages[ MAX_CLIENTS ].AddConcat( dest.GetData(), dest.GetSize(), msg.GetData(), msg.GetSize(), false );
		}

		RepeaterUnreliableMessage( msg, clientNum, &dest );
	}
}

/*
===============
idGameLocal::RepeaterUnreliableMessagePVS
Forwards an unreliable message to viewers based on PVS.
Concats header and msg in case a header needs to be added.
===============
*/
void idGameLocal::RepeaterUnreliableMessagePVS( const idBitMsg &msg, const int *areas, int numAreas, const idBitMsg *header ) {
	if ( !isRepeater ) {
		return;
	}

	int i, icl;
	pvsHandle_t pvsHandle[ 2 ];
	assert( numAreas <= 2 );

	for ( i = 0; i < numAreas; i++ ) {
		pvsHandle[ i ] = pvs.SetupCurrentPVS( areas[ i ], PVS_NORMAL );
	}

	for ( icl = 0; icl < maxViewer; icl ++ ) {
		if ( !viewers[ icl ].active ) {
			continue;
		}

		// if no areas are given, this is a global emit; also always emit for repeaters
		if ( numAreas && !viewers[ icl ].nopvs ) {
			// only send if pvs says this client can see it
			for ( i = 0; i < numAreas; i++ ) {
				if ( pvs.InCurrentPVS( pvsHandle[ i ], viewers[ icl ].pvsArea ) ) {
					RepeaterAppendUnreliableMessage( icl, msg, header );
					break;
				}
			}
		} else {
			// global, so always send it
			RepeaterAppendUnreliableMessage( icl, msg, header );
		}
	}

	for ( i = 0; i < numAreas; i++ ) {
		pvs.FreeCurrentPVS( pvsHandle[ i ] );
	}
}

/*
===============
idGameLocal::SendUnreliableMessagePVS
instanceEnt to NULL for no instance checks
excludeClient to -1 for no exclusions
===============
*/
void idGameLocal::SendUnreliableMessagePVS( const idBitMsg &msg, const idEntity *instanceEnt, int area1, int area2 ) {
	int			icl;
	int			matchInstance = instanceEnt ? instanceEnt->GetInstance() : -1;
	idPlayer	*player;
	int			areas[ 2 ];
	int			numEvAreas;

	numEvAreas = 0;
	if ( area1 != -1 ) {
		areas[ 0 ] = area1;
		numEvAreas++;
	}
	if ( area2 != -1 ) {
		areas[ numEvAreas ] = area2;
		numEvAreas++;
	}

	for ( icl = 0; icl < numClients; icl++ ) {
		if ( icl == localClientNum ) {
			// local client is always excluded
			continue;
		}
		if ( !entities[ icl ] ) {
			continue;
		}
		if ( matchInstance >= 0 && entities[ icl ]->GetInstance() != matchInstance ) {
			continue;
		}
		if ( clientsPVS[ icl ].i < 0 ) {
			// clients for which we don't have PVS info won't get anything
			continue;
		}
		player = static_cast< idPlayer * >( entities[ icl ] );

		// if no areas are given, this is a global emit
		if ( numEvAreas ) {
			// ony send if pvs says this client can see it
			if ( !pvs.InCurrentPVS( clientsPVS[ icl ], areas, numEvAreas ) ) {
				continue;
			}
		}

		unreliableMessages[ icl ].Add( msg.GetData(), msg.GetSize(), false );
	}

	if ( demoState == DEMO_RECORDING || isRepeater ) {
		// record the target areas to the message
		idBitMsg	dest;
		byte		msgBuf[ 16 ];

		// Tourney games: only record from instance 0
		if ( !instanceEnt || instanceEnt->GetInstance() == 0 ) {
		
			dest.Init( msgBuf, sizeof( msgBuf ) );
			dest.WriteByte( GAME_UNRELIABLE_RECORD_AREAS );
			dest.WriteLong( area1 );
			dest.WriteLong( area2 );
			
			if ( demoState == DEMO_RECORDING ) {
				unreliableMessages[ MAX_CLIENTS ].AddConcat( dest.GetData(), dest.GetSize(), msg.GetData(), msg.GetSize(), false );
			}

			RepeaterUnreliableMessagePVS( msg, areas, numEvAreas, &dest );
		}
	}
}

/*
===============
idGameLocal::ClientReadUnreliableMessages
===============
*/
void idGameLocal::ClientReadUnreliableMessages( const idBitMsg &_msg ) {
	idMsgQueue	localQueue;
	int			size;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	idBitMsg	msg;

	localQueue.ReadFrom( _msg );

	msg.Init( msgBuf, sizeof( msgBuf ) );
	while ( localQueue.Get( msg.GetData(), msg.GetMaxSize(), size, false ) ) {
		msg.SetSize( size );
		msg.BeginReading();
		ProcessUnreliableMessage( msg );
		msg.BeginWriting();
	}
}

/*
===============
idGameLocal::DemoReplayInAreas
checks if our current demo replay view ( server demo ) matches the areas given
===============
*/
bool idGameLocal::IsDemoReplayInAreas( const int areas[2], int numAreas ) {
	idVec3			view;
	pvsHandle_t		handle;

	bool			ret;

	assert( IsServerDemoPlaying() );

	if ( !numAreas ) {
		return true;
	}

	if ( followPlayer == -1 ) {
		view = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	} else {
		view = entities[ followPlayer ]->GetPhysics()->GetOrigin();
	}

	// could probably factorize this, at least for processing all unreliable messages, maybe at a higher level of the loop?
	handle = pvs.SetupCurrentPVS( view );
	ret = pvs.InCurrentPVS( handle, areas, numAreas );
	pvs.FreeCurrentPVS( handle );
	return ret;   
}

/*
===============
idGameLocal::ProcessUnreliableMessage
===============
*/
void idGameLocal::ProcessUnreliableMessage( const idBitMsg &msg ) {
	if ( IsServerDemoPlaying() ) {
		int record_type = msg.ReadByte();
		assert( record_type < GAME_UNRELIABLE_RECORD_COUNT );
		switch ( record_type ) {
		case GAME_UNRELIABLE_RECORD_CLIENTNUM: {
			int client = msg.ReadChar();

			RepeaterUnreliableMessage( msg, client );

			if ( client != -1 ) {
				// unreliable was targetted
				if ( followPlayer != client ) {
					// either free flying, or following someone else
					return;
				}
			}
			break;
			}
		case GAME_UNRELIABLE_RECORD_AREAS: {
			int areas[ 2 ];
			int numEvAreas = 0;

			areas[ numEvAreas ] = msg.ReadLong();
			if ( areas[ numEvAreas ] != -1 ) ++numEvAreas;
			areas[ numEvAreas ] = msg.ReadLong();
			if ( areas[ numEvAreas ] != -1 ) ++numEvAreas;

			RepeaterUnreliableMessagePVS( msg, areas, numEvAreas );

			if ( !IsDemoReplayInAreas( areas, numEvAreas ) ) {
				return;
			}
			break;
			}
		}
	}

	int type = msg.ReadByte();
	switch ( type ) {
		case GAME_UNRELIABLE_MESSAGE_EVENT: {
			idEntityPtr<idEntity> p;
			int spawnId = msg.ReadBits( 32 );
			p.SetSpawnId( spawnId );
			
			if ( p.GetEntity() ) {
				p.GetEntity()->ClientReceiveEvent( msg.ReadByte(), time, msg );
			} else {
				Warning( "ProcessUnreliableMessage: no local entity 0x%x for event %d", spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ), msg.ReadByte() );
			}
			break;
		}
		case GAME_UNRELIABLE_MESSAGE_EFFECT: {
			idCQuat				quat;
			idVec3				origin, origin2;
			rvClientEffect*		effect;
			effectCategory_t	category;
			const idDecl		*decl;
				
			decl = idGameLocal::ReadDecl( msg, DECL_EFFECT );

			origin.x = msg.ReadFloat( );
			origin.y = msg.ReadFloat( );
			origin.z = msg.ReadFloat( );
				
			quat.x = msg.ReadFloat( );
			quat.y = msg.ReadFloat( );
			quat.z = msg.ReadFloat( );
			
			origin2.x = msg.ReadFloat( );
			origin2.y = msg.ReadFloat( );
			origin2.z = msg.ReadFloat( );

			category = ( effectCategory_t )msg.ReadByte();

			bool loop = msg.ReadBits( 1 ) != 0;
			bool predictBit = msg.ReadBits( 1 ) != 0;

			if ( bse->CanPlayRateLimited( category ) && ( !predictBit || !g_predictedProjectiles.GetBool() ) ) {
				effect = new rvClientEffect( decl );
				effect->SetOrigin( origin );
				effect->SetAxis( quat.ToMat3() );
				effect->Play( time, loop, origin2 );
			}
			
			break;
		}
		case GAME_UNRELIABLE_MESSAGE_HITSCAN: {
			ClientHitScan( msg );
			break;
		}
#ifdef _USE_VOICECHAT
		case GAME_UNRELIABLE_MESSAGE_VOICEDATA_SERVER: {
			mpGame.ReceiveAndPlayVoiceData( msg );
			break;
		}
#else
		case GAME_UNRELIABLE_MESSAGE_VOICEDATA_SERVER: {
			break;
		}
#endif
		default: {
			Error( "idGameLocal::ProcessUnreliableMessage() - Unknown unreliable message '%d'\n", type );
		}
	}
}

/*
===============
idGameLocal::WriteNetworkInfo
===============
*/
void idGameLocal::WriteNetworkInfo( idFile* file, int clientNum ) {
	int				i, j;
	snapshot_t		*snapshot;
	entityState_t	*entityState;

	if ( !IsServerDemoRecording() ) {

		// save the current states
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			entityState = clientEntityStates[clientNum][i];
			file->WriteBool( !!entityState );
			if ( entityState ) {
				file->WriteInt( entityState->entityNumber );
				file->WriteInt( entityState->state.GetSize() );
				file->Write( entityState->state.GetData(), entityState->state.GetSize() );
			}
		}

		// save the PVS states
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			for ( j = 0; j < ENTITY_PVS_SIZE; j++ ) {
				file->WriteInt( clientPVS[i][j] );
			}
		}

	}

	// players ( including local client )
	j = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !entities[i] ) {
			continue;
		}
		j++;
	}
	file->WriteInt( j );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !entities[i] ) {
			continue;
		}
		file->WriteInt( i );
		file->WriteInt( spawnIds[ i ] );
	}

	if ( !IsServerDemoRecording() ) {

		// write number of snapshots so on readback we know how many to allocate
		i = 0;
		for ( snapshot = clientSnapshots[ clientNum ]; snapshot; snapshot = snapshot->next ) {
			i++;
		}
		file->WriteInt( i );

		for ( snapshot = clientSnapshots[ clientNum ]; snapshot; snapshot = snapshot->next ) {
			file->WriteInt( snapshot->sequence );

			// write number of entity states in the snapshot
			i = 0;
			for ( entityState = snapshot->firstEntityState; entityState; entityState = entityState->next ) {
				i++;
			}
			file->WriteInt( i );
		
			for ( entityState = snapshot->firstEntityState; entityState; entityState = entityState->next ) {
				file->WriteInt( entityState->entityNumber );
				file->WriteInt( entityState->state.GetSize() );
				file->Write( entityState->state.GetData(), entityState->state.GetSize() );
			}

			file->Write( snapshot->pvs, sizeof( snapshot->pvs ) );
		}

	}

	// write the 'initial reliables' data
	mpGame.WriteNetworkInfo( file, clientNum );
}

/*
===============
idGameLocal::ReadNetworkInfo
===============
*/
void idGameLocal::ReadNetworkInfo( int gameTime, idFile* file, int clientNum ) {
	int				i, j, num, numStates, stateSize;
	snapshot_t		*snapshot, **lastSnap;
	entityState_t	*entityState, **lastState;

	assert( clientNum == MAX_CLIENTS || !IsServerDemoPlaying() );
	InitLocalClient( clientNum );

	time = gameTime;
	previousTime = gameTime;

	// force new frame
	realClientTime = 0;
	isNewFrame = true;

	// clear the snapshot entity list
	snapshotEntities.Clear();

	if ( !IsServerDemoPlaying() || IsRepeaterDemoPlaying() ) {

		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			bool isValid;
		
			file->ReadBool( isValid );
			if ( isValid ) {
				clientEntityStates[clientNum][i] = entityStateAllocator.Alloc();
				entityState = clientEntityStates[clientNum][i];
				entityState->next = NULL;
				file->ReadInt( entityState->entityNumber );
				file->ReadInt( stateSize );
				entityState->state.Init( entityState->stateBuf, sizeof( entityState->stateBuf ) );
				entityState->state.SetSize( stateSize );
				file->Read( entityState->state.GetData(), stateSize );
			} else {
				clientEntityStates[clientNum][i] = NULL;
			}
		}

		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			for ( j = 0; j < ENTITY_PVS_SIZE; j++ ) {
				file->ReadInt( clientPVS[i][j] );
			}
		}

	}

	// spawn player entities. ( numClients is not a count but the watermark of client indexes )
	file->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		int icl, spawnId;
		file->ReadInt( icl );
		file->ReadInt( spawnId );
		SpawnPlayer( icl );
		spawnIds[ icl ] = spawnId;
		numClients = icl + 1;
	}

	if ( !IsServerDemoPlaying() || IsRepeaterDemoPlaying() ) {

		file->ReadInt( num );
		lastSnap = &clientSnapshots[ localClientNum ];
		for ( i = 0; i < num; i++ ) {
			snapshot = snapshotAllocator.Alloc();
			snapshot->firstEntityState = NULL;
			snapshot->next = NULL;
			file->ReadInt( snapshot->sequence );

			file->ReadInt( numStates );
			lastState = &snapshot->firstEntityState;
			for ( j = 0; j < numStates; j++ ) {			
				entityState = entityStateAllocator.Alloc();
				file->ReadInt( entityState->entityNumber );
				file->ReadInt( stateSize );
				entityState->state.Init( entityState->stateBuf, sizeof( entityState->stateBuf ) );
				entityState->state.SetSize( stateSize );
				file->Read( entityState->state.GetData(), stateSize );
				entityState->next = NULL;
				assert( !(*lastState ) );
				*lastState = entityState;
				lastState = &entityState->next;
			}

			file->Read( snapshot->pvs, sizeof( snapshot->pvs ) );

			assert( !(*lastSnap) );
			*lastSnap = snapshot;
			lastSnap = &snapshot->next;
		}

		// spawn entities
		for ( i = 0; i < ENTITYNUM_NONE; i++ ) {
			int						spawnId, entityDefNumber;
			idBitMsgDelta			deltaMsg;
			idDict					args;
			entityState_t			*base = clientEntityStates[clientNum][i];
			idEntity				*ent = entities[i];
			const idDeclEntityDef	*decl;
		
			if ( !base ) {
				continue;
			}
			base->state.BeginReading();
			deltaMsg.InitReading( &base->state, NULL, NULL );

			spawnId = deltaMsg.ReadBits( 32 - GENTITYNUM_BITS );
			entityDefNumber = deltaMsg.ReadBits( entityDefBits );

			if ( !ent || ent->entityDefNumber != entityDefNumber || spawnId != spawnIds[ i ] ) {

				delete ent;

				spawnCount = spawnId;

				args.Clear();
				args.SetInt( "spawn_entnum", i );
				args.Set( "name", va( "entity%d", i ) );

				// assume any items spawned from a server-snapshot are in our instance
				if( gameLocal.GetLocalPlayer() ) {
					args.SetInt( "instance", gameLocal.GetLocalPlayer()->GetInstance() );
				}

				assert( entityDefNumber >= 0 );
				if ( entityDefNumber >= declManager->GetNumDecls( DECL_ENTITYDEF ) ) {
					Error( "server has %d entityDefs instead of %d", entityDefNumber, declManager->GetNumDecls( DECL_ENTITYDEF ) );
				}
				decl = static_cast< const idDeclEntityDef * >( declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false ) );
				assert( decl && decl->GetType() == DECL_ENTITYDEF );
				args.Set( "classname", decl->GetName() );					
				if ( !SpawnEntityDef( args, &ent ) || !entities[i] ) {
					Error( "Failed to spawn entity with classname '%s' of type '%s'", decl->GetName(), decl->dict.GetString("spawnclass") );
				}
			}

			// add the entity to the snapshot list
			ent->snapshotNode.AddToEnd( snapshotEntities );
		
			// read the class specific data from the snapshot
			ent->ReadFromSnapshot( deltaMsg );

			// read the player state from player entities
			if ( ent->entityNumber < MAX_CLIENTS ) {
				if ( deltaMsg.ReadBits( 1 ) == 1 ) {
					assert( ent->IsType( idPlayer::GetClassType() ) );
					static_cast< idPlayer * >( ent )->ReadPlayerStateFromSnapshot( deltaMsg );
				}
			}

			// this is useful. for instance on idPlayer, resets stuff so powerups actually appear
			ent->ClientUnstale();
		}

		{
			// specific state read for game and player state
			idBitMsgDelta	deltaMsg;
			entityState_t	*base = clientEntityStates[clientNum][ENTITYNUM_NONE];

			// it's possible to have a recording start right at CS_INGAME and not have a base for reading this yet
			if ( base ) {
				base->state.BeginReading();
				deltaMsg.InitReading( &base->state, NULL, NULL );
				ReadGameStateFromSnapshot( deltaMsg );
			}
		}

		// set self spectating state according to userinfo settings
		if ( !IsRepeaterDemoPlaying() ) {
			GetLocalPlayer()->Spectate( idStr::Icmp( userInfo[ clientNum ].GetString( "ui_spectate" ), "Spectate" ) == 0 );
		}

	}

	// read the 'initial reliables' data
	mpGame.ReadNetworkInfo( file, clientNum );
}

/*
============
idGameLocal::SetDemoState
============
*/
void idGameLocal::SetDemoState( demoState_t state, bool _serverDemo, bool _timeDemo ) {
	if ( demoState == DEMO_RECORDING && state == DEMO_NONE && serverDemo ) {
		ServerClientDisconnect( MAX_CLIENTS );
	}
	demoState = state;
	serverDemo = _serverDemo;
	timeDemo = _timeDemo;
}

/*
============
idGameLocal::SetRepeaterState
============
*/
void idGameLocal::SetRepeaterState( bool isRepeater, bool serverIsRepeater ) {
	this->isRepeater = isRepeater;
	this->serverIsRepeater = serverIsRepeater;

	if ( cvarSystem->GetCVarInteger( "net_serverDownload" ) == 3 ) {
		networkSystem->HTTPEnable( this->isServer || this->isRepeater );
	}

	ReallocViewers( isRepeater ? cvarSystem->GetCVarInteger( "ri_maxViewers" ) : 0 );
}

/*
===============
idGameLocal::ValidateDemoProtocol
===============
*/
bool idGameLocal::ValidateDemoProtocol( int minor_ref, int minor ) {
#if 0
	// 1.1 beta : 67
	// 1.1 final: 68
	// 1.2		: 69
	// 1.3		: 71

	// let 1.3 play 1.2 demos - keep a careful eye on snapshotting changes
	demo_protocol = minor;
	return ( minor_ref == minor || ( minor_ref == 71 && minor == 69 ) );
#else
	demo_protocol = minor;
	return ( minor_ref == minor );
#endif
}

/*
===============
idGameLocal::RandomSpawn
===============
*/
idPlayerStart *idGameLocal::RandomSpawn( void ) {
	return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ];
}
