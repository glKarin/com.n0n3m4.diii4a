/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "../Game_local.h"

ABSTRACT_DECLARATION( idClass, idPhysics )
END_CLASS


/*
================
idPhysics::~idPhysics
================
*/
idPhysics::~idPhysics( void ) {
}

/*
================
idPhysics::Save
================
*/
void idPhysics::Save( idSaveGame *savefile ) const {
}

/*
================
idPhysics::Restore
================
*/
void idPhysics::Restore( idRestoreGame *savefile ) {
}

/*
================
idPhysics::SetClipBox
================
*/
void idPhysics::SetClipBox( const idBounds &bounds, float density ) {
	SetClipModel( new idClipModel( idTraceModel( bounds ) ), density );
}

/*
================
idPhysics::SnapTimeToPhysicsFrame
================
*/
int idPhysics::SnapTimeToPhysicsFrame( int t ) {
	int s;
	s = t + USERCMD_MSEC - 1;
	return ( s - s % USERCMD_MSEC );
}

/*
================
idPhysics::AddForce
================
*/
void idPhysics::AddForce( const int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) {
	// default: forces ignored
}
