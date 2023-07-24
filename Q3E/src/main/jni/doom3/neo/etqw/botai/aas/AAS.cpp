// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#include "AAS_local.h"
#include "../../../libs/AASLib/AASFile.h"

/*
============
idAAS::Alloc
============
*/
idAAS *idAAS::Alloc( void ) {
	return new idAASLocal;
}

/*
============
idAAS::idAAS
============
*/
idAAS::~idAAS( void ) {
}

/*
============
idAASLocal::idAASLocal
============
*/
idAASLocal::idAASLocal( void ) {
	file = NULL;

	areaCacheIndex = NULL;
	areaCacheIndexSize = 0;
	portalCacheIndex = NULL;
	portalCacheIndexSize = 0;
	areaUpdate = NULL;
	portalUpdate = NULL;
	goalAreaTravelTimes = NULL;
	areaTravelTimes = NULL;
	numAreaTravelTimes = 0;
	cacheListStart = NULL;
	cacheListEnd = NULL;
	totalCacheMemory = 0;

	groundSpeedMultiplier = 1.0f;
	waterSpeedMultiplier = 1.0f;

	obstaclePVS = NULL;
	obstaclePVSAreaNum = 0;
}

/*
============
idAASLocal::~idAASLocal
============
*/
idAASLocal::~idAASLocal( void ) {
	Shutdown();
}

/*
============
idAASLocal::Init
============
*/
bool idAASLocal::Init( const char *mapName, unsigned int mapFileCRC ) {
	if ( file && idStr::Icmp( mapName, file->GetName() ) == 0 && mapFileCRC == file->GetCRC() ) {
		gameLocal.Printf( "Keeping %s\n", file->GetName() );
		return true;
	}

	Shutdown();

	file = AASFileManager->LoadAAS( mapName, mapFileCRC );
	if ( file == NULL ) {
		gameLocal.Warning( "Couldn't load AAS file: '%s'", mapName );
		return false;
	}

	SetupRouting();

	SetupObstaclePVS();

	return true;
}

/*
============
idAASLocal::Shutdown
============
*/
void idAASLocal::Shutdown( void ) {
	if ( file == NULL ) {
		return;
	}

	ShutdownRouting();

	ShutdownObstaclePVS();

	AASFileManager->FreeAAS( file );

	file = NULL;
}

/*
============
idAASLocal::Stats
============
*/
void idAASLocal::Stats( void ) const {
	if ( file == NULL ) {
		return;
	}
	gameLocal.Printf( "[%s]\n", file->GetName() );
	gameLocal.Printf( "%6d areas\n", file->GetNumAreas() - 1 );
	gameLocal.Printf( "%6d kB file size\n", file->MemorySize() >> 10 );
	RoutingStats();
}

/*
============
idAASLocal::GetSettings
============
*/
const idAASSettings *idAASLocal::GetSettings( void ) const {
	if ( file == NULL ) {
		return NULL;
	}
	return &file->GetSettings();
}

/*
============
idAASLocal::PointAreaNum
============
*/
int idAASLocal::PointAreaNum( const idVec3 &origin ) const {
	if ( file == NULL ) {
		return 0;
	}
	return file->PointAreaNum( origin );
}

/*
============
idAASLocal::PointReachableAreaNum
============
*/
int idAASLocal::PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, int excludeTravelFlags ) const {
	if ( file == NULL ) {
		return 0;
	}

	return file->PointReachableAreaNum( origin, searchBounds, areaFlags, excludeTravelFlags );
}

/*
============
idAASLocal::BoundsReachableAreaNum
============
*/
int idAASLocal::BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, int excludeTravelFlags ) const {
	if ( file == NULL ) {
		return 0;
	}

	return file->BoundsReachableAreaNum( bounds, areaFlags, excludeTravelFlags );
}

/*
============
idAASLocal::PushPointIntoArea
============
*/
void idAASLocal::PushPointIntoArea( int areaNum, idVec3 &origin ) const {
	if ( file == NULL ) {
		return;
	}
	// FIXME: this is a hack to avoid problems with areas that have a malformed winding
	if ( ( file->GetArea( areaNum ).flags & AAS_AREA_NOPUSH ) != 0 ) {
		return;
	}
	file->PushPointIntoArea( areaNum, origin );
}

/*
============
idAASLocal::AreaCenter
============
*/
idVec3 idAASLocal::AreaCenter( int areaNum ) const {
	if ( file == NULL ) {
		return vec3_origin;
	}
	return file->AreaCenter( areaNum );
}

/*
============
idAASLocal::Trace
============
*/
bool idAASLocal::Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const {
	if ( file == NULL ) {
		trace.fraction = 0.0f;
		trace.endpos = start;
		trace.lastAreaNum = 0;
		trace.numAreas = 0;
		return true;
	}
	return file->Trace( trace, start, end );
}

/*
============
idAASLocal::TraceHeight
============
*/
bool idAASLocal::TraceHeight( aasTraceHeight_t &trace, const idVec3 &start, const idVec3 &end ) const {
	if ( file == NULL ) {
		trace.numPoints = 0;
		return false;
	}
	return file->TraceHeight( trace, start, end );
}

/*
============
idAASLocal::TraceFloor
============
*/
bool idAASLocal::TraceFloor( aasTraceFloor_t &trace, const idVec3 &start, int startAreaNum, const idVec3 &end, int travelFlags ) const {
	if ( file == NULL ) {
		trace.fraction = 0.0f;
		trace.endpos = start;
		trace.lastAreaNum = 0;
		return false;
	}
	return file->TraceFloor( trace, start, startAreaNum, end, 0, travelFlags );
}

/*
============
idAASLocal::GetEdgeVertexNumbers
============
*/
void idAASLocal::GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const {
	if ( file == NULL ) {
		verts[0] = verts[1] = 0;
		return;
	}
	const int *v = file->GetEdge( abs( edgeNum ) ).vertexNum;
	verts[0] = v[INT32_SIGNBITSET( edgeNum )];
	verts[1] = v[INT32_SIGNBITNOTSET( edgeNum )];
}

/*
============
idAASLocal::GetEdge
============
*/
void idAASLocal::GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const {
	if ( file == NULL ) {
		start.Zero();
		end.Zero();
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	start = file->GetVertex( v[INT32_SIGNBITSET(edgeNum)] );
	end = file->GetVertex( v[INT32_SIGNBITNOTSET(edgeNum)] );
}

/*
============
idAASLocal::GetAreaFlags
============
*/
int idAASLocal::GetAreaFlags( int areaNum ) const {
	if ( file == NULL ) {
		return 0;
	}
	return file->GetArea( areaNum ).flags;
}

/*
============
idAASLocal::GetEdgeFlags
============
*/
int idAASLocal::GetEdgeFlags( int edgeNum ) const {
	if ( file == NULL ) {
		return 0;
	}
	return file->GetEdge( abs( edgeNum ) ).flags;
}
