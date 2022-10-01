/*
game_worldspawn.cpp

Worldspawn class.  Each map has one worldspawn which handles global spawnargs.

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
================
idWorldspawn

Every map should have exactly one worldspawn.
================
*/
CLASS_DECLARATION( idEntity, idWorldspawn )
	EVENT( EV_Remove,				idWorldspawn::Event_Remove )
	EVENT( EV_SafeRemove,			idWorldspawn::Event_Remove )
END_CLASS

/*
================
idWorldspawn::Spawn
================
*/
void idWorldspawn::Spawn( void ) {
	idStr				scriptname;
	idThread			*thread;
	const function_t	*func;
	const idKeyValue	*kv;

	assert( gameLocal.world == NULL );
	gameLocal.world = this;

	// load script
	scriptname = gameLocal.GetMapName();
	scriptname.SetFileExtension( ".script" );
	if ( fileSystem->ReadFile( scriptname, NULL, NULL ) > 0 ) {
		gameLocal.program.CompileFile( scriptname );

		// call the main function by default
		func = gameLocal.program.FindFunction( "main" );
		if ( func != NULL ) {
			thread = new idThread( func );
			thread->DelayedStart( 0 );
		}
	}

	// call any functions specified in worldspawn
	kv = spawnArgs.MatchPrefix( "call" );
	while( kv != NULL ) {
		func = gameLocal.program.FindFunction( kv->GetValue() );
		if ( func == NULL ) {
			gameLocal.Error( "Function '%s' not found in script for '%s' key on worldspawn", kv->GetValue().c_str(), kv->GetKey().c_str() );
		}

		thread = new idThread( func );
		thread->DelayedStart( 0 );
		kv = spawnArgs.MatchPrefix( "call", kv );
	}
}

/*
=================
idWorldspawn::Save
=================
*/
void idWorldspawn::Save( idRestoreGame *savefile ) {
}

/*
=================
idWorldspawn::Restore
=================
*/
void idWorldspawn::Restore( idRestoreGame *savefile ) {
	assert( gameLocal.world == this );

// RAVEN BEGIN
// bdube: gravity change
	g_gravity.SetFloat( spawnArgs.GetFloat( "gravity", va( "%f", DEFAULT_GRAVITY) ) );
// RAVEN END
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

/*
================
idWorldspawn::Event_Remove
================
*/
void idWorldspawn::Event_Remove( void ) {
	gameLocal.Error( "Tried to remove world" );
}
