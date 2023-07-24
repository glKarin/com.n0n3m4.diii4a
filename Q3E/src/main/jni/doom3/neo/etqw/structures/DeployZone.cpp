// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DeployZone.h"

/*
==============
sdDeployZoneBroadcastData::MakeDefault
==============
*/
void sdDeployZoneBroadcastData::MakeDefault( void ) {
	sdScriptEntityBroadcastData::MakeDefault();

	active = false;
}

/*
==============
sdDeployZoneBroadcastData::Write
==============
*/
void sdDeployZoneBroadcastData::Write( idFile* file ) const {
	sdScriptEntityBroadcastData::Write( file );

	file->WriteBool( active );
}

/*
==============
sdDeployZoneBroadcastData::Read
==============
*/
void sdDeployZoneBroadcastData::Read( idFile* file ) {
	sdScriptEntityBroadcastData::Read( file );

	file->ReadBool( active );
}

/*
===============================================================================

	sdPlayZoneMarker

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdPlayZoneMarker )
END_CLASS

/*
==============
sdPlayZoneMarker::sdPlayZoneMarker
==============
*/
sdPlayZoneMarker::sdPlayZoneMarker( void ) {
}

/*
==============
sdPlayZoneMarker::sdPlayZoneMarker
==============
*/
sdPlayZoneMarker::~sdPlayZoneMarker( void ) {
}

/*
==============
sdPlayZoneMarker::Spawn
==============
*/
void sdPlayZoneMarker::Spawn( void ) {
	int areaNum = gameRenderWorld->PointInArea( GetPhysics()->GetOrigin() );
	if ( areaNum != -1 ) {
		for ( int i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
			if ( gameRenderWorld->AreasAreConnected( areaNum, i, PORTAL_PLAYZONE ) ) {
				gameLocal.SetPlayZoneAreaName( i, GetName() );
			}
		}
	}

	PostEventMS( &EV_Remove, 0 );
}

/*
===============================================================================

	sdPlayZoneEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdPlayZoneEntity )
END_CLASS

/*
==============
sdPlayZoneEntity::sdPlayZoneEntity
==============
*/
sdPlayZoneEntity::sdPlayZoneEntity( void ) {
}

/*
==============
sdPlayZoneEntity::sdPlayZoneEntity
==============
*/
sdPlayZoneEntity::~sdPlayZoneEntity( void ) {
}

/*
==============
sdPlayZoneEntity::Spawn
==============
*/
void sdPlayZoneEntity::Spawn( void ) {
	idBounds absBounds;
	absBounds.FromTransformedBounds( GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	gameLocal.CreatePlayZone( spawnArgs, absBounds );

	// Everyone needs to remove this!
	PostEventMS( &EV_Remove, 0 );
}

/*
===============================================================================

	sdDeployZone

===============================================================================
*/

const idEventDef EV_SetActive( "setActive", '\0', DOC_TEXT( "Sets whether the territory is considered active or not." ), 1, NULL, "b", "state", "Whether to set it as active or not." );

CLASS_DECLARATION( sdScriptEntity, sdDeployZone )
	EVENT( EV_SetActive,				sdDeployZone::Event_SetActive )
END_CLASS

/*
==============
sdDeployZone::sdDeployZone
==============
*/
sdDeployZone::sdDeployZone( void ) {
}

/*
==============
sdDeployZone::sdDeployZone
==============
*/
sdDeployZone::~sdDeployZone( void ) {
}

/*
==============
sdDeployZone::Spawn
==============
*/
void sdDeployZone::Spawn( void ) {
	_active = false;

	GetPhysics()->UnlinkClip();
}

/*
================
sdDeployZone::ApplyNetworkState
================
*/
void sdDeployZone::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdDeployZoneBroadcastData );

		_active = newData.active;
	}

	sdScriptEntity::ApplyNetworkState( mode, newState );
}

/*
==============
sdDeployZone::ReadNetworkState
==============
*/
void sdDeployZone::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdDeployZoneBroadcastData );

		// read state
		newData.active = msg.ReadBool();
	}

	sdScriptEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdDeployZone::WriteNetworkState
==============
*/
void sdDeployZone::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdDeployZoneBroadcastData );

		// update state
		newData.active = _active;

		// write state
		msg.WriteBool( newData.active );
	}

	sdScriptEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
==============
sdDeployZone::CheckNetworkStateChanges
==============
*/
bool sdDeployZone::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdDeployZoneBroadcastData );

		if ( baseData.active != _active ) {
			return true;
		}
	}
	return sdScriptEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
==============
sdDeployZone::CreateNetworkStructure
==============
*/
sdEntityStateNetworkData* sdDeployZone::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdDeployZoneBroadcastData();
	}
	return sdScriptEntity::CreateNetworkStructure( mode );
}
