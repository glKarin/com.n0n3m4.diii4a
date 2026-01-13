#include "../../idlib/precompiled.h"
#pragma hdrstop

// RAVEN BEGIN
#include "../Game_local.h"
// RAVEN END
#include "AAS_local.h"

/*
============
idAAS::Alloc
============
*/
idAAS *idAAS::Alloc( void ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_AAS);
// RAVEN END
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
bool idAASLocal::Init( const idStr &mapName, unsigned int mapFileCRC ) {
	if ( file && mapName.Icmp( file->GetName() ) == 0 && mapFileCRC == file->GetCRC() ) {
		gameLocal.Printf( "Keeping %s\n", file->GetName() );
		RemoveAllObstacles();
	}
	else {
		Shutdown();

		file = AASFileManager->LoadAAS( mapName, mapFileCRC );
		if ( !file ) {
			common->DWarning( "Couldn't load AAS file: '%s'", mapName.c_str() );
			return false;
		}
// RAVEN BEGIN
// rhummer: Check if this is a dummy file, since it really has no valid data dump it.
		else if ( file->IsDummyFile( mapFileCRC ) ) {
			AASFileManager->FreeAAS( file );
			file = NULL;
			return false;
		}
// RAVEN END
		SetupRouting();
	}
	return true;
}

/*
============
idAASLocal::Shutdown
============
*/
void idAASLocal::Shutdown( void ) {
	if ( file ) {
		ShutdownRouting();
		RemoveAllObstacles();
		AASFileManager->FreeAAS( file );
		file = NULL;
	}
}

/*
============
idAASLocal::Stats
============
*/
void idAASLocal::Stats( void ) const {
	if ( !file ) {
		return;
	}
	common->Printf( "[%s]\n", file->GetName() );
	file->PrintInfo();
	RoutingStats();
}

// RAVEN BEGIN
// jscott: added
/*
============
idAASLocal::StatsSummary
============
*/
size_t idAASLocal::StatsSummary( void ) const {

	int		size;

	if( !file ) {

		return( 0 );
	}

	size = ( numAreaTravelTimes * sizeof( unsigned short ) ) 
			+ ( areaCacheIndexSize * sizeof( idRoutingCache * ) ) 
			+ ( portalCacheIndexSize * sizeof( idRoutingCache * ) );

	return( file->GetMemorySize() + size );
}
// RAVEN END

/*
============
idAASLocal::GetSettings
============
*/
const idAASSettings *idAASLocal::GetSettings( void ) const {
	if ( !file ) {
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
	if ( !file ) {
		return 0;
	}
	return file->PointAreaNum( origin );
}

/*
============
idAASLocal::PointReachableAreaNum
============
*/
int idAASLocal::PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}

	return file->PointReachableAreaNum( origin, searchBounds, areaFlags, TFL_INVALID );
}

/*
============
idAASLocal::BoundsReachableAreaNum
============
*/
int idAASLocal::BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}
	
	return file->BoundsReachableAreaNum( bounds, areaFlags, TFL_INVALID );
}

/*
============
idAASLocal::PushPointIntoAreaNum
============
*/
void idAASLocal::PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const {
	if ( !file ) {
		return;
	}
	file->PushPointIntoAreaNum( areaNum, origin );
}

/*
============
idAASLocal::AreaCenter
============
*/
idVec3 idAASLocal::AreaCenter( int areaNum ) const {
	if ( !file ) {
		return vec3_origin;
	}
	return file->GetArea( areaNum ).center;
}

// RAVEN BEGIN
// bdube: added
/*
============
idAASLocal::AreaRadius
============
*/
float idAASLocal::AreaRadius( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).bounds.GetRadius();
}
// mcg: added
/*
============
idAASLocal::AreaBounds
============
*/
idBounds & idAASLocal::AreaBounds( int areaNum ) const {
	return file->GetArea( areaNum ).bounds;
}
/*
============
idAASLocal::AreaCeiling
============
*/
float idAASLocal::AreaCeiling( int areaNum ) const {
	return file->GetArea( areaNum ).ceiling;
}
// RAVEN END

/*
============
idAASLocal::AreaFlags
============
*/
int idAASLocal::AreaFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).flags;
}

/*
============
idAASLocal::AreaTravelFlags
============
*/
int idAASLocal::AreaTravelFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).travelFlags;
}

/*
============
idAASLocal::Trace
============
*/
bool idAASLocal::Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const {
	if ( !file ) {
		trace.fraction = 0.0f;
		trace.lastAreaNum = 0;
		trace.numAreas = 0;
		return true;
	}
	return file->Trace( trace, start, end );
}

/*
============
idAASLocal::GetPlane
============
*/
const idPlane &idAASLocal::GetPlane( int planeNum ) const {
	if ( !file ) {
		static idPlane dummy;
		return dummy;
	}
	return file->GetPlane( planeNum );
}

/*
============
idAASLocal::GetEdgeVertexNumbers
============
*/
void idAASLocal::GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const {
	if ( !file ) {
		verts[0] = verts[1] = 0;
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	verts[0] = v[INTSIGNBITSET(edgeNum)];
	verts[1] = v[INTSIGNBITNOTSET(edgeNum)];
}

/*
============
idAASLocal::GetEdge
============
*/
void idAASLocal::GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const {
	if ( !file ) {
		start.Zero();
		end.Zero();
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	start = file->GetVertex( v[INTSIGNBITSET(edgeNum)] );
	end = file->GetVertex( v[INTSIGNBITNOTSET(edgeNum)] );
}


/*
===============================================================================

	idAASCallback

===============================================================================
*/

/*
============
idAASCallback::~idAASCallback
============
*/
idAASCallback::~idAASCallback ( void ) {
}

/*
============
idAASCallback::Test
============
*/
idAASCallback::testResult_t idAASCallback::Test ( class idAAS *aas, int areaNum, const idVec3& origin, float minDistance, float maxDistance, const idVec3* point, aasGoal_t& goal ) {
	// Get AAS file
	idAASFile* file = ((idAAS&)*aas).GetFile ( );
	if ( !file ) {
		return TEST_BADAREA;
	}
	
	// Get area for edges
	aasArea_t& area = file->GetArea ( areaNum );

	if ( ai_debugTactical.GetInteger ( ) > 1 ) {
		gameRenderWorld->DebugLine ( colorYellow, area.center, area.center + idVec3(0,0,80.0f), 10000 );
	}
	
	// Make sure the area itself is valid
	if ( !TestArea ( aas, areaNum, area ) ) {
		return TEST_BADAREA;
	}

	if ( ai_debugTactical.GetInteger ( ) > 1 && point ) {
		gameRenderWorld->DebugLine ( colorMagenta, *point, *point + idVec3(0,0,64.0f), 10000 );
	}
	
	// Test the original origin first
	if ( point && TestPointDistance ( origin, *point, minDistance, maxDistance) && TestPoint ( aas, *point ) ) {
		goal.areaNum = areaNum;
		goal.origin  = *point;
		return TEST_OK;
	}

	if ( ai_debugTactical.GetInteger ( ) > 1 ) {
		gameRenderWorld->DebugLine ( colorCyan, area.center, area.center + idVec3(0,0,64.0f), 10000 );
	}
	
	// Test the center of the area
	if ( TestPointDistance ( origin, area.center, minDistance, maxDistance) && TestPoint ( aas, area.center, area.ceiling ) ) {
		goal.areaNum = areaNum;
		goal.origin  = area.center;
		return TEST_OK;
	}
	
	// For each face test all available edges
	int	f;
	int	e;
	for ( f = 0; f < area.numFaces; f ++ ) {
		aasFace_t& face = file->GetFace ( abs ( file->GetFaceIndex (area.firstFace + f ) ) );
		
		// for each edge test a point between the center of the edge and the center
		for ( e = 0; e < face.numEdges; e ++ ) {
			idVec3 edgeCenter = file->EdgeCenter ( abs( file->GetEdgeIndex( face.firstEdge + e ) ) ); 	
			idVec3 dir        = area.center - edgeCenter;
			float  dist;
			for ( dist = dir.Normalize() - 64.0f; dist > 0.0f; dist -= 64.0f ) {				
				idVec3 testPoint = edgeCenter + dir * dist;
				if ( ai_debugTactical.GetInteger ( ) > 1 ) {
					gameRenderWorld->DebugLine ( colorPurple, testPoint, testPoint + idVec3(0,0,64.0f), 10000 );
				}

				if ( TestPointDistance ( origin, testPoint, minDistance, maxDistance) && TestPoint ( aas, testPoint, area.ceiling ) ) {
					goal.areaNum = areaNum;
					goal.origin  = testPoint;
					return TEST_OK;
				}
			}
		}
	}
	
	return TEST_BADPOINT;
}

/*
============
idAASCallback::Init
============
*/
bool idAASCallback::TestPointDistance ( const idVec3& origin, const idVec3& point, float minDistance, float maxDistance ) {
	float dist = (origin - point).LengthFast ( );
	if ( minDistance > 0.0f && dist < minDistance ) {
		return false;
	}
	if ( maxDistance > 0.0f && dist > maxDistance ) {
		return false;
	}
	return true;
}

/*
============
idAASCallback::Init
============
*/
void idAASCallback::Init ( void ) {
}

/*
============
idAASCallback::Finish
============
*/
void idAASCallback::Finish ( void ) {
}

/*
============
idAASCallback::TestArea
============
*/
bool idAASCallback::TestArea ( class idAAS *aas, int areaNum, const aasArea_t& area ) {
	return true;
}

/*
============
idAASCallback::TestPoint
============
*/
bool idAASCallback::TestPoint ( class idAAS *aas, const idVec3& pos, const float zAllow ) {
	return true;
}

// RAVEN END
