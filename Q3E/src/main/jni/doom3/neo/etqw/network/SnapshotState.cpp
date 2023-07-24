// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SnapshotState.h"

/*
===============================================================================

	sdEntityState

===============================================================================
*/

/*
================
sdEntityState::sdEntityState
================
*/
sdEntityState::sdEntityState( idEntity* entity, networkStateMode_t mode ) : next( NULL ), data( NULL ) {
	spawnId = gameLocal.GetSpawnId( entity );
	data = entity->CreateNetworkStructure( mode );
}

/*
================
sdEntityState::~sdEntityState
================
*/
sdEntityState::~sdEntityState( void ) {
	delete data;
}

/*
===============================================================================

	sdGameState

===============================================================================
*/

/*
================
sdGameState::sdGameState
================
*/
sdGameState::sdGameState( const sdNetworkStateObject& object ) : next( NULL ), data( NULL ) {
	data = object.CreateNetworkStructure();
}

/*
================
sdGameState::~sdGameState
================
*/
sdGameState::~sdGameState( void ) {
	delete data;
}



/*
===============================================================================

	snapshot_t

===============================================================================
*/

/*
================
snapshot_t::snapshot_t
================
*/
snapshot_t::snapshot_t( void ) {
}

/*
================
snapshot_t::Init
================
*/
void snapshot_t::Init( int _sequence, int _time ) {
	sequence		= _sequence;
	time			= _time;
	next			= NULL;

	memset( firstEntityState, 0, sizeof( firstEntityState ) );
	memset( gameStates, 0, sizeof( gameStates ) );
	clientUserCommands.Clear();
}




/*
===============================================================================

	clientNetworkInfo_t

===============================================================================
*/

/*
================
clientNetworkInfo_t::clientNetworkInfo_t
================
*/
clientNetworkInfo_t::clientNetworkInfo_t( void ) {
	for ( int i = 0; i < MAX_GENTITIES; i++ ) {
		lastMarker[ i ] = -1;
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		lastUserCommand[ i ] = -1;
		lastUserCommandDelay[ i ] = -1;
	}

	snapshots = NULL;

	memset( states, 0, sizeof( states ) );
	memset( gameStates, 0, sizeof( gameStates ) );
}

/*
================
clientNetworkInfo_t::Reset
================
*/
void clientNetworkInfo_t::Reset( void ) {
	gameLocal.FreeSnapshotsOlderThanSequence( *this, 0x7FFFFFFF );

	for ( int i = 0; i < MAX_GENTITIES; i++ ) {
		for( int j = 0; j < NSM_NUM_MODES; j++ ) {
			gameLocal.FreeNetworkState( states[ i ][ j ] );
			states[ i ][ j ] = NULL;
		}

		lastMarker[ i ] = -1;
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		lastUserCommand[ i ] = -1;
		lastUserCommandDelay[ i ] = -1;
	}
	for( int i = 0; i < ENSM_NUM_MODES; i++ ) {
		gameLocal.FreeGameState( gameStates[ i ] );
		gameStates[ i ] = NULL;
	}
}






/*
===============================================================================

	sdNetworkStateObject

===============================================================================
*/

/*
================
sdNetworkStateObject::~sdNetworkStateObject
================
*/
sdNetworkStateObject::~sdNetworkStateObject( void ) {
	Clear();
}

/*
================
sdNetworkStateObject::Clear
================
*/
void sdNetworkStateObject::Clear( void ) {
	for ( sdGameState* next = freeStates; next != NULL; ) {
		sdGameState* current = next;
		next = current->next;

		gameLocal.FreeGameState( current );
	}

	freeStates = NULL;
}
