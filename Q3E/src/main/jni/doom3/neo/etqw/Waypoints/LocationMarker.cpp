// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "LocationMarker.h"
#include "../Player.h"
#include "../CommandMapInfo.h"
#include "../WorldSpawn.h"
#include "../roles/WayPointManager.h"

/*
===============================================================================

	sdLocationMarker

===============================================================================
*/

idCVar g_showCommandMapNames( "g_showCommandMapNames", "0", CVAR_BOOL | CVAR_GAME, "Show/hide location text on the commandmap" );
idCVar g_showLocationWayPoints( "g_showLocationWayPoints", "2", CVAR_INTEGER | CVAR_GAME, "Show/hide location waypoints in the world" );

CLASS_DECLARATION( idEntity, sdLocationMarker )
END_CLASS

idStaticList< locationInfo_t, sdLocationMarker::MAX_LOCATIONS >		sdLocationMarker::s_locations;
idStaticList< locationInfo_t*, sdLocationMarker::MAX_LOCATIONS >	sdLocationMarker::s_exteriorLocations;
idStaticList< locationInfo_t*, sdLocationMarker::MAX_LOCATIONS >	sdLocationMarker::s_interiorLocations;
idList< int >														sdLocationMarker::s_areaCollapse;
idList< locationInfo_t* >											sdLocationMarker::s_areaLocations;
const sdDeclLocStr*													sdLocationMarker::s_locationTextMissing;
const sdDeclLocStr*													sdLocationMarker::s_locationTextRange;
sdLocationMarker::sdLocationCVarCallback							sdLocationMarker::s_callback;
sdLocationMarker::sdWayPointCVarCallback							sdLocationMarker::s_callback2;
compassDirection_t													sdLocationMarker::s_compassDirections[ 8 ];

/*
==============
sdLocationMarker::sdLocationMarker
==============
*/
sdLocationMarker::sdLocationMarker( void ) {
}

/*
==============
sdLocationMarker::Spawn
==============
*/
void sdLocationMarker::Spawn( void ) {
	locationInfo_t& info = *s_locations.Alloc();
	if ( &info == NULL ) {
		gameLocal.Error( "sdLocationMarker::Spawn No Free Locations" );
	}

	info.origin = GetPhysics()->GetOrigin();
	if ( spawnArgs.GetBool( "interior" ) ) {
		s_interiorLocations.Append( &info );
	} else {
		s_exteriorLocations.Append( &info );
	}

	if ( spawnArgs.GetBool( "commandmap" ) ) {
		info.commandMapName = declHolder.declLocStrType.LocalFind( spawnArgs.GetString( "commandmap_name" ) );
	} else {
		info.commandMapName = NULL;
	}

	if ( spawnArgs.GetBool( "waypoint" ) ) {
		info.waypointMaterial	= gameLocal.declMaterialType[ spawnArgs.GetString( "mtr_waypoint" ) ];
	} else {
		info.waypointMaterial	= NULL;
	}

	info.locationName		= declHolder.declLocStrType.LocalFind( spawnArgs.GetString( "location_name" ) );
	info.minRange			= spawnArgs.GetFloat( "range_min" );
	info.maxRange			= spawnArgs.GetFloat( "range_max" );
	info.nextInArea			= NULL;
	info.commandMapHandle	= -1;
	info.font				= spawnArgs.GetString( "font" );
	info.textScale			= spawnArgs.GetFloat( "text_scale" );
	info.wayPoint			= NULL;

	PostEventMS( &EV_Remove, 0 );
}

/*
==============
sdLocationMarker::OnNewMapLoad
==============
*/
void sdLocationMarker::OnNewMapLoad( void ) {
	Clear();

	g_showCommandMapNames.RegisterCallback( &s_callback );
	g_showLocationWayPoints.RegisterCallback( &s_callback2 );
}

/*
==============
sdLocationMarker::OnMapClear
==============
*/
void sdLocationMarker::OnMapClear( bool all ) {
	Clear();

	if ( !all ) {
		g_showCommandMapNames.RegisterCallback( &s_callback );
		g_showLocationWayPoints.RegisterCallback( &s_callback2 );
	}
}

/*
==============
sdLocationMarker::Clear
==============
*/
void sdLocationMarker::Clear( void ) {
	g_showCommandMapNames.UnRegisterCallback( &s_callback );
	g_showLocationWayPoints.UnRegisterCallback( &s_callback2 );

	FreeCommandMapIcons();
	FreeWayPoints();

	s_locations.SetNum( 0 );
	s_exteriorLocations.SetNum( 0 );
	s_interiorLocations.SetNum( 0 );
	s_areaCollapse.SetNum( 0, false );
	s_areaLocations.SetNum( 0, false );
}

/*
==============
sdLocationMarker::FreeCommandMapIcon
==============
*/
void sdLocationMarker::FreeCommandMapIcon( locationInfo_t& info ) {
	if ( info.commandMapHandle == -1 ) {
		return;
	}

	sdCommandMapInfoManager::GetInstance().Free( info.commandMapHandle );
	info.commandMapHandle = -1;
}

/*
==============
sdLocationMarker::CreateCommandMapIcon
==============
*/
void sdLocationMarker::CreateCommandMapIcon( locationInfo_t& info ) {
	if ( info.commandMapHandle != -1 ) {
		return;
	}

	if ( info.commandMapName == NULL ) {
		return;
	}

	info.commandMapHandle = sdCommandMapInfoManager::GetInstance().Alloc( gameLocal.world, 100 );
	sdCommandMapInfo* cm = sdCommandMapInfoManager::GetInstance().GetInfo( info.commandMapHandle );
	if ( cm != NULL ) {
		cm->SetOrigin( info.origin.ToVec2() );
		cm->SetDrawMode( sdCommandMapInfo::DM_TEXT );
		cm->SetColor( colorWhite );
		cm->SetFlag( sdCommandMapInfo::CMF_ALWAYSKNOWN | sdCommandMapInfo::CMF_ONLYSHOWONFULLVIEW );
		cm->SetPositionMode( sdCommandMapInfo::PM_FIXED );
		cm->SetColorMode( sdCommandMapInfo::CM_NORMAL );
		cm->SetFont( info.font.c_str() );
		cm->SetTextScale( info.textScale );
		cm->SetText( info.commandMapName->GetText() );
	}
}

/*
==============
sdLocationMarker::OnMapStart
==============
*/
void sdLocationMarker::OnMapStart( void ) {
	int numAreas = gameRenderWorld->NumAreas();

	s_areaCollapse.SetNum( numAreas, false );

	for ( int i = 0; i < numAreas; i++ ) {
		int j;
		for ( j = 0; j < i; j++ ) {
			if ( gameRenderWorld->AreasAreConnected( j, i, PORTAL_OUTSIDE ) ) {
				s_areaCollapse[ i ] = s_areaCollapse[ j ];
				break;
			}
		}
		if ( j == i ) {
			s_areaCollapse[ i ] = s_areaLocations.Num();
			s_areaLocations.Alloc() = NULL;
		}
	}

	for ( int i = 0; i < s_interiorLocations.Num(); i++ ) {
		locationInfo_t& loc = *s_interiorLocations[ i ];

		int baseArea = gameRenderWorld->PointInArea( loc.origin );
		if ( baseArea == -1 ) {
			gameLocal.Warning( "sdLocationMarker::OnMapStart Interior Location '%ls' Is Not in an Area", loc.locationName->GetText() );
			continue;
		}
		int indirect = s_areaCollapse[ baseArea ];

		loc.nextInArea = s_areaLocations[ indirect ];
		s_areaLocations[ indirect ] = &loc;
	}

	OnShowMarkersChanged();
	OnShowWayPointsChanged();

	s_locationTextMissing	= declHolder.declLocStrType.LocalFind( "game/location/missing" );
	s_locationTextRange		= declHolder.declLocStrType.LocalFind( "game/location/range" );

	float angle = idMath::Sqrt( 0.5f );

	s_compassDirections[ 0 ].dir = idVec2( 0.f, 1.f );
	s_compassDirections[ 0 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/N" );
	s_compassDirections[ 1 ].dir = idVec2( angle, angle );
	s_compassDirections[ 1 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/NE" );
	s_compassDirections[ 2 ].dir = idVec2( 1.f, 0.f );
	s_compassDirections[ 2 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/E" );
	s_compassDirections[ 3 ].dir = idVec2( angle, -angle );
	s_compassDirections[ 3 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/SE" );
	s_compassDirections[ 4 ].dir = idVec2( 0.f, -1.f );
	s_compassDirections[ 4 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/S" );
	s_compassDirections[ 5 ].dir = idVec2( -angle, -angle );
	s_compassDirections[ 5 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/SW" );
	s_compassDirections[ 6 ].dir = idVec2( -1.f, 0.f );
	s_compassDirections[ 6 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/W" );
	s_compassDirections[ 7 ].dir = idVec2( -angle, angle );
	s_compassDirections[ 7 ].name = declHolder.declLocStrType.LocalFind( "game/location/direction/NW" );
}

/*
==============
sdLocationMarker::LocationForPosition
==============
*/
locationInfo_t* sdLocationMarker::LocationForPosition( const idVec3& position ) {
	int baseArea = gameRenderWorld->PointInArea( position );
	if ( baseArea != -1 ) {
		int indirect = s_areaCollapse[ baseArea ];

		locationInfo_t* loc = s_areaLocations[ indirect ];
		if ( loc != NULL ) {
			locationInfo_t* bestLoc = NULL;
			float bestDist = -1;

			for ( locationInfo_t* testLoc = loc; testLoc != NULL; testLoc = testLoc->nextInArea ) {
				float dist = ( testLoc->origin - position ).LengthSqr();
				if ( dist > Square( testLoc->maxRange ) ) {
					continue;
				}

				if ( dist < bestDist || bestLoc == NULL ) {
					bestLoc = testLoc;
					bestDist = dist;
				}
			}

			if ( bestLoc != NULL ) {
				return bestLoc;
			}
		}
	}

	locationInfo_t* bestLoc = NULL;
	float bestDist = -1;

	for ( int i = 0; i < s_exteriorLocations.Num(); i++ ) {
		locationInfo_t* testLoc = s_exteriorLocations[ i ];
		
		float dist = ( testLoc->origin - position ).LengthSqr();
		if ( dist > Square( testLoc->maxRange ) ) {
			continue;
		}

		if ( dist < bestDist || bestLoc == NULL ) {
			bestLoc = testLoc;
			bestDist = dist;
		}
	}

	return bestLoc;
}

/*
==============
sdLocationMarker::DebugDraw
==============
*/
void sdLocationMarker::DebugDraw( const idVec3& position ) {
	locationInfo_t* loc = LocationForPosition( position );	

	for ( int i = 0; i < s_locations.Num(); i++ ) {
		gameRenderWorld->DebugBounds( loc == &s_locations[ i ] ? colorGreen : colorRed, idBounds( idSphere( s_locations[ i ].origin, 16 ) ) );
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		if ( loc != NULL ) {
			idWStr text;
			GetLocationText( position, text );

			gameRenderWorld->DrawText( va( "%ls", text.c_str() ), loc->origin, 0.25f, colorWhite, localPlayer->renderView.viewaxis );
		}
	}
}

/*
==============
sdLocationMarker::GetLocationText
==============
*/
void sdLocationMarker::GetLocationText( const idVec3& position, idWStr& text ) {
	locationInfo_t* info = LocationForPosition( position );
	if ( info == NULL ) {
		text = s_locationTextMissing->GetText();
		return;
	}

	idVec3 diff = position - info->origin;

	float range = diff.LengthFast();
	if ( range < info->minRange ) {
		text = info->locationName->GetText();
	} else {
		idVec2 diff2D = diff.ToVec2();

		int best = -1;
		float bestDist = 0.f;
		for ( int i = 0; i < 8; i++ ) {
			float temp = diff2D * s_compassDirections[ i ].dir;
			if ( temp > bestDist || best == -1 ) {
				bestDist = temp;
				best = i;
			}
		}

		idWStrList list;
		list.Append( va( L"%d", ( int )InchesToMetres( bestDist ) ) );
		list.Append( s_compassDirections[ best ].name->GetText() );
		list.Append( info->locationName->GetText() );

		text = common->LocalizeText( s_locationTextRange, list );
	}
}

/*
==============
sdLocationMarker::OnShowMarkersChanged
==============
*/
void sdLocationMarker::OnShowMarkersChanged( void ) {
	if ( g_showCommandMapNames.GetBool() ) {
		CreateCommandMapIcons();
	} else {
		FreeCommandMapIcons();
	}
}

/*
==============
sdLocationMarker::OnShowWayPointsChanged
==============
*/
void sdLocationMarker::OnShowWayPointsChanged( void ) {
	if ( g_showLocationWayPoints.GetInteger() == 1 ) {
		CreateWayPoints();
	} else {
		FreeWayPoints();
	}
}

/*
==============
sdLocationMarker::CreateWayPoint
==============
*/
void sdLocationMarker::CreateWayPoint( locationInfo_t& info ) {
	if ( info.wayPoint != NULL ) {
		return;
	}

	if ( info.waypointMaterial == NULL ) {
		return;
	}

	info.wayPoint = sdWayPointManager::GetInstance().AllocWayPoint();
	info.wayPoint->SetBracketed( false );
	info.wayPoint->SetMinRange( info.minRange );
	info.wayPoint->SetRange( info.maxRange );
	info.wayPoint->SetMaterial( info.waypointMaterial );
	info.wayPoint->SetOrigin( info.origin );
	info.wayPoint->SetText( info.locationName->GetText() );
}

/*
==============
sdLocationMarker::FreeWayPoint
==============
*/
void sdLocationMarker::FreeWayPoint( locationInfo_t& info ) {
	if ( info.wayPoint == NULL ) {
		return;
	}

	sdWayPointManager::GetInstance().FreeWayPoint( info.wayPoint );
	info.wayPoint = NULL;
}

/*
==============
sdLocationMarker::FreeCommandMapIcons
==============
*/
void sdLocationMarker::FreeCommandMapIcons( void ) {
	for ( int i = 0; i < s_locations.Num(); i++ ) {
		FreeCommandMapIcon( s_locations[ i ] );
	}
}

/*
==============
sdLocationMarker::FreeWayPoints
==============
*/
void sdLocationMarker::FreeWayPoints( void ) {
	for ( int i = 0; i < s_locations.Num(); i++ ) {
		FreeWayPoint( s_locations[ i ] );
	}
}

/*
==============
sdLocationMarker::CreateCommandMapIcons
==============
*/
void sdLocationMarker::CreateCommandMapIcons( void ) {
	for ( int i = 0; i < s_locations.Num(); i++ ) {
		CreateCommandMapIcon( s_locations[ i ] );
	}
}

/*
==============
sdLocationMarker::CreateWayPoints
==============
*/
void sdLocationMarker::CreateWayPoints( void ) {
	for ( int i = 0; i < s_locations.Num(); i++ ) {
		CreateWayPoint( s_locations[ i ] );
	}
}

/*
==============
sdLocationMarker::ShowLocations
==============
*/
void sdLocationMarker::ShowLocations( bool value ) {
	if ( g_showLocationWayPoints.GetInteger() == 2 ) {
		if ( value ) {
			CreateWayPoints();
		} else {
			FreeWayPoints();
		}
	}
}
