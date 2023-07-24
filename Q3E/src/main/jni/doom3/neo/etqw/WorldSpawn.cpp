// Copyright (C) 2007 Id Software, Inc.
//
/*
game_worldspawn.cpp

Worldspawn class.  Each map has one worldspawn which handles global spawnargs.

*/

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "WorldSpawn.h"

/*
================
idWorldspawn

Every map should have exactly one worldspawn.
================
*/
CLASS_DECLARATION( idEntity, idWorldspawn )
END_CLASS

/*
================
idWorldspawn::Spawn
================
*/
void idWorldspawn::Spawn( void ) {
	assert( gameLocal.world == NULL );
	gameLocal.world = this;

	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY ) ) );

	gameLocal.flightCeilingLower = spawnArgs.GetFloat( "flight_ceiling_lower", "2048" );
	gameLocal.flightCeilingUpper = spawnArgs.GetFloat( "flight_ceiling_upper", "2304" );
}

/*
================
idWorldspawn::~idWorldspawn
================
*/
idWorldspawn::~idWorldspawn() {
	if ( gameLocal.world == this ) {
		gameLocal.world = NULL;
	}
}
