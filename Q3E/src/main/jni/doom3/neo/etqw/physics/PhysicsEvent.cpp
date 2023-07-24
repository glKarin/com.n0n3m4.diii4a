// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PhysicsEvent.h"

/*
===============================================================================

	sdPhysicsEvent

===============================================================================
*/

/*
================
sdPhysicsEvent::sdPhysicsEvent
================
*/
sdPhysicsEvent::sdPhysicsEvent( nodeType_t& list ) : _creationTime( gameLocal.time ) {
	_node.SetOwner( this );
	_node.AddToEnd( list );
}

/*
===============================================================================

	sdPhysicsEvent_RadiusPush

===============================================================================
*/

/*
================
sdPhysicsEvent_RadiusPush::sdPhysicsEvent_RadiusPush
================
*/
sdPhysicsEvent_RadiusPush::sdPhysicsEvent_RadiusPush( nodeType_t& list, const idVec3 &origin, float radius, const sdDeclDamage* damageDecl, float push, const idEntity *inflictor, const idEntity *ignore, int flags ) : sdPhysicsEvent( list ) {
	_origin				= origin;
	_radius				= radius;
	_push				= push;
	_inflictor			= inflictor;
	_ignore				= ignore;
	_flags				= flags;
	_damageDecl			= damageDecl;
}

/*
================
sdPhysicsEvent_RadiusPush::Apply
================
*/
void sdPhysicsEvent_RadiusPush::Apply( void ) const {
	gameLocal.RadiusPush( _origin, _radius, _damageDecl, _push, _inflictor, _ignore, _flags, false );
}
