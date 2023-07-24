// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "WayPointManager.h"

#include "../misc/WorldToScreen.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"

/*
===============================================================================

	sdWayPoint

===============================================================================
*/

/*
================
sdWayPoint::sdWayPoint
================
*/
sdWayPoint::sdWayPoint( void ) {
	_activeNode.SetOwner( this );

	Clear();
}

/*
================
sdWayPoint::Clear
================
*/
void sdWayPoint::Clear( void ) {
	_flags					= 0;
	_activeTime				= 0;
	_highlightActiveTime	= 0;
	_fixedBounds.GetMins()	= idVec3( -8.f, -8.f, -8.f );
	_fixedBounds.GetMaxs()	= idVec3( 8.f, 8.f, 8.f );
	_text					= L"";
	_fixedLocation			= vec3_origin;
	_iconOffset				= vec3_origin;
	_material				= NULL;
	_offScreenMaterial		= NULL;
	_owner					= NULL;
	_range					= 1024.f;

	_shouldCheckLineOfSight	= true;
	_isVisible				= true;
	_selected				= -1;
	_flashEndTime			= -1;

	_activeNode.Remove();
	sdWayPointManager::GetInstance().RemoveWayPoint( this );
}

/*
================
sdWayPoint::Init
================
*/
void sdWayPoint::Init( void ) {
}

/*
================
sdWayPoint::Update
================
*/
void sdWayPoint::Update( void ) {
	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	_isVisible = true;
}

/*
================
sdWayPoint::GetBounds
================
*/
const idBounds& sdWayPoint::GetBounds( void ) const {
	static idBounds emptyBounds = idBounds( vec3_zero );
	if ( _flags & WF_FIXEDSIZE ) {
		return _fixedBounds;
	}
	if ( _owner.IsValid() ) {
		if ( _flags & WF_RENDERMODEL ) {
			return _owner->GetRenderEntity()->bounds;
		}
		return _owner->GetWayPointBounds();
	}
	return emptyBounds;
}

/*
================
sdWayPoint::GetPosition
================
*/
const idVec3 sdWayPoint::GetPosition( void ) const {
	if ( _flags & WF_FIXEDPOSITION ) {
		return _fixedLocation + _iconOffset;
	}

	// ao: avoid prediction jerkiness
	if ( _owner->GetRenderEntity() != NULL ) {
		return _owner->GetLastPushedOrigin() + _iconOffset;
	}

	return _owner->GetWayPointOrigin() + _iconOffset;
}

/*
================
sdWayPoint::GetBracketPosition
================
*/
const idVec3 sdWayPoint::GetBracketPosition( void ) const {
	if ( _flags & WF_FIXEDPOSITION ) {
		return _fixedLocation;
	}

	// ao: avoid prediction jerkiness
	if ( _owner->GetRenderEntity() != NULL ) {
		return _owner->GetLastPushedOrigin();
	}

	return _owner->GetWayPointOrigin();
}

/*
================
sdWayPoint::GetOrientation
================
*/
const idMat3& sdWayPoint::GetOrientation( void ) const {
	if ( _flags & WF_FIXEDPOSITION ) {
		return mat3_identity;
	}

	// ao: avoid prediction jerkiness
	const renderEntity_t* rEnt = _owner->GetRenderEntity();
	if ( rEnt != NULL ) {
		return rEnt->axis;
	}

	return _owner->GetWayPointAxis();
}

/*
================
sdWayPoint::SetBounds
================
*/
void sdWayPoint::SetBounds( const idBounds& bounds ) {
	_fixedBounds = bounds;
	_flags |= WF_FIXEDSIZE;
}

/*
================
sdWayPoint::SetOrigin
================
*/
void sdWayPoint::SetOrigin( const idVec3& org ) {
	_fixedLocation = org;
	_flags |= WF_FIXEDPOSITION;
}

/*
================
sdWayPoint::SetIconOffset
================
*/
void sdWayPoint::SetIconOffset( const idVec3& offset ) {
	_iconOffset = offset;
}

/*
================
sdWayPoint::SetOwner
================
*/
void sdWayPoint::SetOwner( idEntity* owner ) {
	_owner = owner;

	if ( owner == NULL ) {
		gameLocal.Warning( "sdWayPoint::SetOwner NULL owner" );
		return;
	}

	idVec3 offset = owner->spawnArgs.GetVector( "waypoint_offset", "0 0 64" );
	SetIconOffset( offset );
}

/*
================
sdWayPoint::GetActiveFraction
================
*/
float sdWayPoint::GetActiveFraction( void ) const {
	if ( IsActive() ) {
		if ( gameLocal.time > _activeTime ) {
			return 1.f;
		}
		return 1.f - ( ( _activeTime - gameLocal.time ) / ( float )ACTIVATE_TIME );
	}
	int timeSince = gameLocal.time - _activeTime;
	if ( timeSince > ACTIVATE_TIME ) {
		return 0.f;
	}
	return 1.f - ( ( timeSince ) / ( float )ACTIVATE_TIME );
}

/*
================
sdWayPoint::MakeActive
================
*/
void sdWayPoint::MakeActive( void ) {
	float active = GetActiveFraction();
	_activeTime = gameLocal.time + ( ( 1.f - active ) * ACTIVATE_TIME );
	_flags |= WF_ACTIVE;
}

/*
================
sdWayPoint::MakeInActive
================
*/
void sdWayPoint::MakeInActive( void ) {
	float active = GetActiveFraction();
	_activeTime = gameLocal.time - ( ( 1.f - active ) * ACTIVATE_TIME );
	_flags &= ~WF_ACTIVE;
}

/*
================
sdWayPoint::GetHighLightActiveFraction
================
*/
float sdWayPoint::GetHighLightActiveFraction( void ) const {
	if ( IsHighlightActive() ) {
		if ( gameLocal.time > _highlightActiveTime ) {
			return 1.f;
		}
		return 1.f - ( ( _highlightActiveTime - gameLocal.time ) / ( float )ACTIVATE_TIME );
	}
	int timeSince = gameLocal.time - _highlightActiveTime;
	if ( timeSince > ACTIVATE_TIME ) {
		return 0.f;
	}
	return 1.f - ( ( timeSince ) / ( float )ACTIVATE_TIME );
}

/*
================
sdWayPoint::MakeHighlightActive
================
*/
void sdWayPoint::MakeHighlightActive( void ) {
	float active = GetHighLightActiveFraction();
	_highlightActiveTime = gameLocal.time + ( ( 1.f - active ) * ACTIVATE_TIME );
	_flags |= WF_HIGHLIGHTACTIVE;
}

/*
================
sdWayPoint::MakeHighlightInActive
================
*/
void sdWayPoint::MakeHighlightInActive( void ) {
	float active = GetHighLightActiveFraction();
	_highlightActiveTime = gameLocal.time - ( ( 1.f - active ) * ACTIVATE_TIME );
	_flags &= ~WF_HIGHLIGHTACTIVE;
}

/*
================
sdWayPoint::Event_Free
================
*/
void sdWayPoint::Event_Free( void ) {
	sdWayPointManager::GetInstance().FreeWayPoint( this );
}

/*
================
sdWayPoint::Event_SetBounds
================
*/
void sdWayPoint::Event_SetBounds( const idVec3& mins, const idVec3& maxs ) {
	SetBounds( idBounds( mins, maxs ) );
}

/*
================
sdWayPoint::Event_SetOrigin
================
*/
void sdWayPoint::Event_SetOrigin( const idVec3& org ) {
	SetOrigin( org );
}

/*
================
sdWayPoint::Event_SetOwner
================
*/
void sdWayPoint::Event_SetOwner( idEntity* owner ) {
	if ( !owner ) {
		gameLocal.Error( "sdWayPoint::Event_SetOwner Cannot Take a NULL entity" );
	}

	SetOwner( owner );
}

/*
================
sdWayPoint::SetRange
================
*/
void sdWayPoint::SetRange( float range ) {
	_range = range;
}

/*
================
sdWayPoint::SetMaterial
================
*/
void sdWayPoint::SetMaterial( const idMaterial* material ) {
	_material = material;
}

/*
================
sdWayPoint::SetOffScreenMaterial
================
*/
void sdWayPoint::SetOffScreenMaterial( const idMaterial* material ) {
	_offScreenMaterial = material;
}

/*
================
sdWayPoint::UseRenderModel
================
*/
void sdWayPoint::UseRenderModel( void ) {
	_flags |= WF_RENDERMODEL;
}

/*
================
sdWayPoint::SetBracketed
================
*/
void sdWayPoint::SetBracketed( bool bracketed ) {
	if ( bracketed ) {
		_flags |= WF_BRACKETED;
	} else {
		_flags &= ~WF_BRACKETED;
	}
}

/*
===============================================================================

	sdWayPointManagerLocal

===============================================================================
*/

/*
================
sdWayPointManagerLocal::sdWayPointManagerLocal
================
*/
sdWayPointManagerLocal::sdWayPointManagerLocal( void ) {
	_forceShowWayPoints = false;
}

/*
================
sdWayPointManagerLocal::~sdWayPointManagerLocal
================
*/
sdWayPointManagerLocal::~sdWayPointManagerLocal( void ) {
}

/*
================
sdWayPointManagerLocal::AllocWayPoint
================
*/
sdWayPoint* sdWayPointManagerLocal::AllocWayPoint( void ) {
	if ( networkSystem->IsDedicated() ) {
		return NULL;
	}

	sdWayPoint* wayPoint = _allocator.Alloc();

	AddWayPoint( wayPoint );
	wayPoint->Init();

	return wayPoint;
}

/*
================
sdWayPointManagerLocal::Think
================
*/
void sdWayPointManagerLocal::Think( void ) {
	UpdateActive();
	UpdateWayPoints();
}

/*
================
sdWayPointManagerLocal::Think
================
*/
void sdWayPointManagerLocal::UpdateActive( void ) {
	if ( ( gameLocal.time - _lastActiveUpdate ) < SEC2MS( 0.25f ) ) {
		return;
	}
	_lastActiveUpdate = gameLocal.time;

	idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
	if ( !viewPlayer ) {
		return;
	}

	renderView_t& view = viewPlayer->renderView;

	int worldZoneIndex = gameLocal.GetWorldPlayZoneIndex( view.vieworg );

	for ( int i = 0; i < _wayPoints.Num(); i++ ) {
		sdWayPoint* wayPoint = _wayPoints[ i ];

		bool shouldBeActive = true;

		do {
			if ( !wayPoint->IsValid() ) {
				shouldBeActive = false;
				break;
			}		
			if ( !wayPoint->IsAlwaysActive() ) {
				if ( ( wayPoint->GetPosition() - view.vieworg ).LengthSqr() > Square( wayPoint->GetRange() ) ) {
					shouldBeActive = false;
					break;
				}
			}

			if ( gameLocal.GetWorldPlayZoneIndex( wayPoint->GetPosition() ) != worldZoneIndex ) {
				shouldBeActive = false;
				break;
			}

		} while ( false );

		if ( shouldBeActive ) {
			if ( !wayPoint->IsActive() ) {
				wayPoint->MakeActive();
				wayPoint->GetActiveNode().AddToEnd( _activeWayPoints );
			}
		} else {
			if ( wayPoint->IsActive() ) {
				wayPoint->MakeInActive();
			}

			if ( wayPoint->GetActiveFraction() <= 0.f ) {
				wayPoint->GetActiveNode().Remove();
			}
		}
	}
}

/*
================
sdWayPointManagerLocal::UpdateWayPoints
================
*/
void sdWayPointManagerLocal::UpdateWayPoints( void ) {
	if ( _wayPoints.Num() == 0 ) {
		return;
	}

	// update 2
	int firstUpdateIndex = ( _lastUpdatedWayPointIndex + 1 ) % _wayPoints.Num();
	int numUpdated = 0;
	sdWayPoint* firstUpdated = NULL;
	for ( int i = firstUpdateIndex; numUpdated < 2; i = ( i + 1 ) % _wayPoints.Num(), numUpdated++ ) {

		sdWayPoint* wayPoint = _wayPoints[ i ];
		if ( firstUpdated == NULL ) {
			firstUpdated = wayPoint;
		} else if ( numUpdated > 0 && firstUpdated == wayPoint ) {
			// got back around to the first one
			break;
		}

		wayPoint->Update();
		_lastUpdatedWayPointIndex = i;

		if ( _wayPoints.Num() == 0 ) {
			break;
		}

		int newI = i % _wayPoints.Num();

		sdWayPoint* newWayPoint = _wayPoints[ newI ];
		if ( newWayPoint != wayPoint && newI == i ) {
			// hit this index again - it has changed!
			i--;
		}
	}
}

/*
================
sdWayPointManagerLocal::Init
================
*/
void sdWayPointManagerLocal::Init( void ) {
	_lastActiveUpdate = 0;
	_lastUpdatedWayPointIndex = -1;

	while ( _wayPoints.Num() > 0 ) {
		FreeWayPoint( _wayPoints[ 0 ] );
	}
}

/*
================
sdWayPointManagerLocal::FreeWayPoint
================
*/
void sdWayPointManagerLocal::FreeWayPoint( sdWayPoint* wayPoint ) {
	wayPoint->Clear();
	_allocator.Free( wayPoint );
}

/*
================
sdWayPointManagerLocal::AddWayPoint
================
*/
void sdWayPointManagerLocal::AddWayPoint( sdWayPoint* way ) {
	assert( _wayPoints.FindElement( way ) == NULL );

	_wayPoints.Append( way );
}

/*
================
sdWayPointManagerLocal::RemoveWayPoint
================
*/
void sdWayPointManagerLocal::RemoveWayPoint( sdWayPoint* way ) {
	_wayPoints.RemoveFast( way );
}

