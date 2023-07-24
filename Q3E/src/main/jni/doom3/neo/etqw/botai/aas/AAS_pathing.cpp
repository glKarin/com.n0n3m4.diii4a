// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#include "AAS_local.h"
#include "../../../libs/AASLib/AASFile.h"

#include "../BotThread.h"
#include "../BotThreadData.h"

const float	MAX_WALK_PATH_DISTANCE				= 512.0f;
const float MAX_WALK_PATH_DEST_DISTANCE			= 4096.0f;
const int	MAX_WALK_PATH_ITERATIONS			= 10;
const int	MAX_WALK_PATH_SUB_SAMPLE_ITERATIONS	= 8;

idCVar aas_optimizePaths( "aas_optimizePaths", "1", CVAR_BOOL | CVAR_CHEAT | CVAR_GAME, "set to 1 to enable path optimization" );
idCVar aas_subSampleWalkPaths( "aas_subSampleWalkPaths", "1", CVAR_BOOL | CVAR_CHEAT | CVAR_GAME, "set to 1 to enable walk path sub-sampling" );
idCVar aas_extendFlyPaths( "aas_extendFlyPaths", "1", CVAR_BOOL | CVAR_CHEAT | CVAR_GAME, "set to 1 to enable extending fly paths" );

/*
============
idAreaTrail
============
*/
class idAreaTrail {
public:
	static const int MAX_AREA_TRAIL = 4;

	idAreaTrail( int startAreaNum ) {
		for ( int i = 0; i < MAX_AREA_TRAIL; i++ ) {
			areaTrail[i] = startAreaNum;
		}
		trailIndex = 0;
	}
	void Add( int areaNum ) {
		areaTrail[trailIndex] = areaNum;
		trailIndex = ( trailIndex + 1 ) & ( MAX_AREA_TRAIL - 1 );
	}
	bool InList( int areaNum ) {
		for ( int i = 0; i < MAX_AREA_TRAIL; i++ ) {
			if ( areaNum == areaTrail[i] ) {
				return true;
			}
		}
		return false;
	}

private:
	int areaTrail[MAX_AREA_TRAIL];
	int trailIndex;
};

const int idAreaTrail::MAX_AREA_TRAIL;

/*
============
idAASLocal::WalkPathIsValid

  Returns true if one can walk in a straight line from start to goal.
============
*/
ID_INLINE bool idAASLocal::WalkPathIsValid( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, 
							    const idVec3 &goalOrigin, int travelFlags, int &endAreaNum ) const {
	aasTraceFloor_t trace;
	file->TraceFloor( trace, startOrigin, startAreaNum, goalOrigin, goalAreaNum, travelFlags );
	endAreaNum = trace.lastAreaNum;
	return ( endAreaNum == goalAreaNum );
}

/*
============
idAASLocal::SubSampleWalkPath
============
*/
void idAASLocal::SubSampleWalkPath( int startAreaNum, const idVec3 &startOrigin, int pathAreaNum, const idVec3 &pathStart, const idVec3 &pathEnd, int travelFlags, idVec3 &endPos, int &endAreaNum ) const {

	idVec3 bestEndPos = pathStart;
	int bestEndAreaNum = pathAreaNum;

	idVec3 dir = pathEnd - pathStart;
	float length = dir.LengthSqr();
	float step = 0.25f;
	float fraction = 0.5f;

	for ( int i = 0; i < MAX_WALK_PATH_SUB_SAMPLE_ITERATIONS && length * step > Square( 4.0f ); i++, step *= 0.5f ) {
		idVec3 pathPoint = pathStart + dir * fraction;

		int curEndAreaNum;

		if ( WalkPathIsValid( startAreaNum, startOrigin, pathAreaNum, pathPoint, travelFlags, curEndAreaNum ) ) {
			bestEndPos = pathPoint;
			bestEndAreaNum = curEndAreaNum;
			fraction += step;
		} else {
			fraction -= step;
		}
	}

	endPos = bestEndPos;
	endAreaNum = bestEndAreaNum;
}

/*
============
idAASLocal::WalkPathToGoal
============
*/
bool idAASLocal::WalkPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin,
								 int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const {

	int travelTime, endAreaNum;
	const aasReachability_t *reach = NULL;

	path.type = PATHTYPE_WALK;
	path.moveGoal = startOrigin;
	path.moveAreaNum = startAreaNum;
	path.reachability = NULL;
	path.viewGoal = startOrigin;
	path.travelTime = 1;

	if ( file == NULL ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	// if the start and goal are in the same area
	if ( startAreaNum == goalAreaNum ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	// test for a direct path to the destination
	if ( ( goalOrigin - startOrigin ).LengthSqr() < Square( MAX_WALK_PATH_DEST_DISTANCE ) ) {
		if ( WalkPathIsValid( startAreaNum, startOrigin, goalAreaNum, goalOrigin, walkTravelFlags, endAreaNum ) ) {
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}
	}

	idAreaTrail areaTrail( startAreaNum );

	int badTravelFlags = ~walkTravelFlags;
	int curAreaNum = startAreaNum;

	for ( int i = 0; i < MAX_WALK_PATH_ITERATIONS; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			return false;
		}

		if ( reach == NULL ) {
			return false;
		}

		if ( i == 0 ) {
			path.travelTime = travelTime;
		}	

		// no need to check through the first area
		if ( curAreaNum != startAreaNum ) {
			// only optimize a limited distance ahead
			if ( ( reach->GetStart() - startOrigin ).LengthSqr() > Square( MAX_WALK_PATH_DISTANCE ) ) {
				if ( aas_subSampleWalkPaths.GetBool() ) {
					SubSampleWalkPath( startAreaNum, startOrigin, curAreaNum, path.moveGoal, reach->GetStart(), walkTravelFlags, path.moveGoal, path.moveAreaNum );
				}
				return true;
			}
			// if we can't walk directly to the start of the reachability, stop routing
			// and get a sub-sampled path for as far as we can directly walk.
			if ( !WalkPathIsValid( startAreaNum, startOrigin, curAreaNum, reach->GetStart(), walkTravelFlags, endAreaNum ) ) {
				if ( aas_subSampleWalkPaths.GetBool() ) {
					SubSampleWalkPath( startAreaNum, startOrigin, curAreaNum, path.moveGoal, reach->GetStart(), walkTravelFlags, path.moveGoal, path.moveAreaNum );
				}
				return true;
			}
		}

		path.moveGoal = reach->GetStart();
		path.moveAreaNum = curAreaNum;

		if ( ( reach->travelFlags & badTravelFlags ) != 0 ) {
			break;
		}

		path.moveGoal = reach->GetEnd();
		path.moveAreaNum = reach->toAreaNum;

		if ( !aas_optimizePaths.GetBool() ) {
			return true;
		}

		if ( reach->toAreaNum == goalAreaNum ) {
			if ( !WalkPathIsValid( startAreaNum, startOrigin, goalAreaNum, goalOrigin, walkTravelFlags, endAreaNum ) ) {
				if ( aas_subSampleWalkPaths.GetBool() ) {
					SubSampleWalkPath( startAreaNum, startOrigin, goalAreaNum, path.moveGoal, goalOrigin, walkTravelFlags, path.moveGoal, path.moveAreaNum );
				}
				return true;
			}
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}

		curAreaNum = reach->toAreaNum;

		if ( areaTrail.InList( curAreaNum ) ) {
			botThreadData.Warning( "idAASLocal::WalkPathToGoal: local routing minimum at area %d while going from area %d to area %d", curAreaNum, startAreaNum, goalAreaNum );
			break;
		}
		areaTrail.Add( curAreaNum );
	}

	switch( reach->travelFlags ) {
		case TFL_WALKOFFLEDGE:
			path.type = PATHTYPE_WALKOFFLEDGE;
			path.reachability = reach;
			path.moveGoal = reach->GetEnd();
			path.moveAreaNum = reach->toAreaNum;
			break;
		case TFL_WALKOFFBARRIER: {
			path.type = PATHTYPE_WALKOFFBARRIER;
			path.reachability = reach;
			path.moveGoal = reach->GetEnd();
			path.moveAreaNum = reach->toAreaNum;
			break;
		}
		case TFL_BARRIERJUMP:
			path.type = PATHTYPE_BARRIERJUMP;
			path.reachability = reach;
			break;
		case TFL_JUMP:
			path.type = PATHTYPE_JUMP;
			path.reachability = reach;
			break;
		case TFL_LADDER:
			path.type = PATHTYPE_LADDER;
			path.reachability = reach;
			path.moveGoal = reach->GetEnd();
			path.moveAreaNum = reach->toAreaNum;
			break;
		case TFL_TELEPORT:
			path.type = PATHTYPE_TELEPORT;
			path.reachability = reach;
			break;
		case TFL_ELEVATOR:
			path.type = PATHTYPE_ELEVATOR;
			path.reachability = reach;
			break;
		default:
			break;
	}

	return true;
}

/*
============
idAASLocal::HorizontalDistanceSquare
============
*/
ID_INLINE float idAASLocal::HorizontalDistanceSquare( const idVec3 &start, const idVec3 &end ) const {
	idVec3 dir = end - start;
	dir -= file->GetSettings().invGravityDir * dir * file->GetSettings().invGravityDir;
	return dir.LengthSqr();
}

/*
============
idAASLocal::HopPathIsValid
============
*/
bool idAASLocal::HopPathIsValid( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, const idAASHopPathParms &parms ) const {
	if ( ( file->GetArea( goalAreaNum ).flags & AAS_AREA_OUTSIDE ) == 0 ) {
		return false;
	}

	if ( HorizontalDistanceSquare( startOrigin, goalOrigin ) > Square( parms.maxDistance ) ) {
		return false;
	}

	const int MAX_POINTS = 512;
	idVec3 points[MAX_POINTS];

	aasTraceHeight_t trace;

	trace.maxPoints = MAX_POINTS;
	trace.points = points;

	file->TraceHeight( trace, startOrigin, goalOrigin );

	if ( trace.numPoints >= MAX_POINTS ) {
		return false;
	}

	float baseHeight = startOrigin * file->GetSettings().invGravityDir;

	// skip the first point which is in the start area
	for ( int i = 1; i < trace.numPoints; i++ ) {

		float height = trace.points[i] * file->GetSettings().invGravityDir - baseHeight;
		float maxHeight = idMath::ClampFloat( parms.minHeight, parms.maxHeight, parms.maxSlope * idMath::Sqrt( HorizontalDistanceSquare( startOrigin, trace.points[i] ) ) );

		if ( height > maxHeight ) {
			return false;
		}
	}
	return true;
}

/*
============
idAASLocal::ExtendHopPathToGoal
============
*/
bool idAASLocal::ExtendHopPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const {
	int travelTime;
	const aasReachability_t *reach = NULL;

	if ( !aas_extendFlyPaths.GetBool() ) {
		return false;
	}

	assert( path.type != MAX_PATHTYPE );

	if ( ( file->GetArea( startAreaNum ).flags & AAS_AREA_OUTSIDE ) == 0 ) {
		return false;
	}

	if ( path.moveGoal == goalOrigin ) {
		return HopPathIsValid( startAreaNum, startOrigin, path.moveAreaNum, path.moveGoal, parms );
	}

	if ( ( file->GetArea( goalAreaNum ).flags & AAS_AREA_OUTSIDE ) != 0 ) {
		if ( HopPathIsValid( startAreaNum, startOrigin, goalAreaNum, goalOrigin, parms ) ) {
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}
	}

	idAreaTrail areaTrail( path.moveAreaNum );

	bool canHop = HopPathIsValid( startAreaNum, startOrigin, path.moveAreaNum, path.moveGoal, parms );

	for ( int i = 0; i < MAX_WALK_PATH_ITERATIONS; i++ ) {

		if ( !RouteToGoalArea( path.moveAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			return false;
		}

		if ( reach == NULL ) {
			return false;
		}

		if ( !HopPathIsValid( startAreaNum, startOrigin, reach->toAreaNum, reach->GetEnd(), parms ) ) {
			break;
		}

		path.moveGoal = reach->GetEnd();
		path.moveAreaNum = reach->toAreaNum;
		canHop = true;

		if ( reach->toAreaNum == goalAreaNum ) {
			return canHop;
		}

		if ( areaTrail.InList( path.moveAreaNum ) ) {
			botThreadData.Warning( "idAASLocal::HopPathToGoal: local routing minimum at area %d while going from area %d to area %d", path.moveAreaNum, startAreaNum, goalAreaNum );
			break;
		}
		areaTrail.Add( path.moveAreaNum );
	}

	return canHop;
}

/*
============
idAASLocal::SetupObstaclePVS
============
*/
void idAASLocal::SetupObstaclePVS() {
	numObstaclePVSBytes = ( file->GetNumAreas() + 7 ) / 8;
	obstaclePVS = (byte *) Mem_AllocAligned( numObstaclePVSBytes, ALIGN_16 );
}

/*
============
idAASLocal::ShutdownObstaclePVS
============
*/
void idAASLocal::ShutdownObstaclePVS() {
	Mem_FreeAligned( obstaclePVS );
	numObstaclePVSBytes = 0;
	obstaclePVS = NULL;
	obstaclePVSAreaNum = 0;
}

/*
============
idAASLocal::GetObstaclePVS
============
*/
const byte * idAASLocal::GetObstaclePVS( int areaNum ) const {
	if ( areaNum == obstaclePVSAreaNum ) {
		return obstaclePVS;
	}

	memset( obstaclePVS, 0, numObstaclePVSBytes );

	const aasArea_t &area = file->GetArea( areaNum );

	for ( int offset = area.obstaclePVSOffset, index = 0; index < file->GetNumAreas(); ) {
		int rleBits = file->GetObstaclePVS( offset++ );

		if ( ( rleBits & AAS_PVS_RLE_RUN_BIT ) != 0 ) {

			// short run-length code
			int run = rleBits & ( ( 1 << AAS_PVS_RLE_1ST_COUNT_BITS ) - 1 );

			// run-length mode
			if ( ( rleBits & AAS_PVS_RLE_RUN_LONG_BIT ) != 0 ) {
				// additional bits for long run-length code
				run |= file->GetObstaclePVS( offset++ ) << AAS_PVS_RLE_1ST_COUNT_BITS;
			}

			index += ( run + 1 ) * AAS_PVS_RLE_RUN_GRANULARITY;

		} else {

			// immediate mode
			for ( int i = 0; i < AAS_PVS_RLE_IMMEDIATE_BITS; i++ ) {
				obstaclePVS[index >> 3] |= ( ( rleBits >> i ) & 1 ) << ( index & 7 );
				index++;
			}
		}
	}

	obstaclePVSAreaNum = areaNum;

	return obstaclePVS;
}

/*
============
idAASLocal::GetObstaclePVSWallEdges
============
*/
int idAASLocal::GetObstaclePVSWallEdges( int areaNum, int *edges, int maxEdges ) const {

	int numEdges = 0;

	if ( file == NULL ) {
		return numEdges;
	}

	const aasArea_t &area = file->GetArea( areaNum );

	for ( int offset = area.obstaclePVSOffset, index = 0; index < file->GetNumAreas(); ) {
		int rleBits = file->GetObstaclePVS( offset++ );

		if ( ( rleBits & AAS_PVS_RLE_RUN_BIT ) != 0 ) {

			// short run-length code
			int run = rleBits & ( ( 1 << AAS_PVS_RLE_1ST_COUNT_BITS ) - 1 );

			// run-length mode
			if ( ( rleBits & AAS_PVS_RLE_RUN_LONG_BIT ) != 0 ) {
				// additional bits for long run-length code
				run |= file->GetObstaclePVS( offset++ ) << AAS_PVS_RLE_1ST_COUNT_BITS;
			}

			index += ( run + 1 ) * AAS_PVS_RLE_RUN_GRANULARITY;

		} else {

			// immediate mode
			for ( int i = 0; i < AAS_PVS_RLE_IMMEDIATE_BITS; i++ ) {
				if ( ( rleBits & ( 1 << i ) ) == 0 ) {
					index++;
					continue;
				}
				const aasArea_t &pvsArea = file->GetArea( index++ );

				for ( int j = 0; j < pvsArea.numEdges; j++ ) {
					int edgeNum = file->GetEdgeIndex( pvsArea.firstEdge + j );
					const aasEdge_t &edge = file->GetEdge( abs( edgeNum ) );

					if ( ( edge.flags & ( AAS_EDGE_WALL | AAS_EDGE_LEDGE ) ) != 0 ) {
						edges[numEdges++] = edgeNum;
						if ( numEdges >= maxEdges ) {
							return numEdges;
						}
					}
				}
			}
		}
	}

	// FIXME: create short sequences per area above and inline sorting of sequences here

	SortWallEdges( edges, numEdges );

	return numEdges;
}

/*
============
idAASLocal::SortWallEdges
============
*/
struct wallEdge_t {
	int				edgeNum;
	int				verts[2];
	wallEdge_t *	next;
};

void idAASLocal::SortWallEdges( int *edges, int numEdges ) const {
	int i, j, k, numSequences;
	wallEdge_t **sequenceFirst, **sequenceLast, *wallEdges, *wallEdge;

	wallEdges = (wallEdge_t *) _alloca16( numEdges * sizeof( wallEdge_t ) );
	sequenceFirst = (wallEdge_t **)_alloca16( numEdges * sizeof( wallEdge_t * ) );
	sequenceLast = (wallEdge_t **)_alloca16( numEdges * sizeof( wallEdge_t * ) );

	for ( i = 0; i < numEdges; i++ ) {
		int edgeNum = edges[i];
		const int *v = file->GetEdge( abs( edgeNum ) ).vertexNum;

		wallEdges[i].edgeNum = edgeNum;
		wallEdges[i].verts[0] = v[INT32_SIGNBITSET( edgeNum )];
		wallEdges[i].verts[1] = v[INT32_SIGNBITNOTSET( edgeNum )];
		wallEdges[i].next = NULL;
		sequenceFirst[i] = &wallEdges[i];
		sequenceLast[i] = &wallEdges[i];
	}
	numSequences = numEdges;

	for ( i = 0; i < numSequences; i++ ) {
		for ( j = i+1; j < numSequences; j++ ) {
			if ( sequenceFirst[i]->verts[0] == sequenceLast[j]->verts[1] ) {
				sequenceLast[j]->next = sequenceFirst[i];
				sequenceFirst[i] = sequenceFirst[j];
				break;
			}
			if ( sequenceLast[i]->verts[1] == sequenceFirst[j]->verts[0] ) {
				sequenceLast[i]->next = sequenceFirst[j];
				sequenceLast[i] = sequenceLast[j];
				break;
			}
		}
		if ( j < numSequences ) {
			numSequences--;
			for ( k = j; k < numSequences; k++ ) {
				sequenceFirst[k] = sequenceFirst[k+1];
				sequenceLast[k] = sequenceLast[k+1];
			}
			i--;
		}
	}

	k = 0;
	for ( i = 0; i < numSequences; i++ ) {
		for ( wallEdge = sequenceFirst[i]; wallEdge; wallEdge = wallEdge->next ) {
			edges[k++] = wallEdge->edgeNum;
		}
	}
}

/*
============
idAASLocal::EdgesIntersect2D
============
*/
bool idAASLocal::EdgesIntersect2D( const int edge1, const int edge2, const idBounds &b1, const idBounds &b2, const float height ) const {
	const float EDGE_INTERSECTION_EPSILON	= 0.1f;

	const int *e0 = file->GetEdge( abs(edge1) ).vertexNum;
	const int *e1 = file->GetEdge( abs(edge2) ).vertexNum;
	const aasVertex_t &v0 = file->GetVertex( e0[0] );
	const aasVertex_t &v1 = file->GetVertex( e0[1] );
	const aasVertex_t &v2 = file->GetVertex( e1[0] );
	const aasVertex_t &v3 = file->GetVertex( e1[1] );

	idVec3 plane1;
	plane1.x = v0.y - v1.y;
	plane1.y = v1.x - v0.x;
	plane1.z = - ( v0.x * plane1.x + v0.y * plane1.y );

	float d0 = plane1.x * v2.x + plane1.y * v2.y + plane1.z;
	float d1 = plane1.x * v3.x + plane1.y * v3.y + plane1.z;

	// if the edges are in the same plane
	if ( fabs( d0 ) < EDGE_INTERSECTION_EPSILON && fabs( d1 ) < EDGE_INTERSECTION_EPSILON ) {
		// edges must be at least 'height' units apart vertically
		if ( b1[0].z - b2[1].z > height || b2[0].z - b1[1].z > height ) {
			// the edges intersect because the 2D bounds overlap
			return true;
		}
		return false;
	}

	// if both points of the second edge are at the same side of the vertical plane through the first edge
	if ( !( IEEE_FLT_SIGNBITSET( d0 ) ^ IEEE_FLT_SIGNBITSET( d1 ) ) ) {
		return false;
	}

	idVec3 plane2;
	plane2.x = v2.y - v3.y;
	plane2.y = v3.x - v2.x;
	plane2.z = - ( v2.x * plane2.x + v2.y * plane2.y );

	float d2 = plane2.x * v0.x + plane2.y * v0.y + plane2.z;
	float d3 = plane2.x * v1.x + plane2.y * v1.y + plane2.z;

	// if both points of the first edge are at the same side of the vertical plane through the second edge
	if ( !( IEEE_FLT_SIGNBITSET( d2 ) ^ IEEE_FLT_SIGNBITSET( d3 ) ) ) {
		return false;
	}

	d0 = d0 / ( d0 - d1 );
	float z0 = v2.z + d0 * ( v3.z - v2.z );

	d2 = d2 / ( d2 - d3 );
	float z1 = v0.z + d2 * ( v1.z - v0.z );

	// at the crossing point the height difference should be at least 'height' units
	if ( fabs( z0 - z1 ) > height ) {
		return true;
	}

	return false;
}

/*
============
idAASLocal::GetWallEdges
============
*/
int idAASLocal::GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, float height, int *edges, int maxEdges ) const {
	int j, k, numEdges;
	int *areaQueue, curArea, queueStart, queueEnd;
	byte *areasVisited;
	idBounds *edgeBounds;

	if ( file == NULL ) {
		return 0;
	}

	numEdges = 0;

	areasVisited = (byte *) _alloca16( file->GetNumAreas() );
	memset( areasVisited, 0, file->GetNumAreas() * sizeof( byte ) );
	areaQueue = (int *) _alloca16( file->GetNumAreas() * sizeof( int ) );
	edgeBounds = (idBounds *) _alloca16( maxEdges * sizeof( edgeBounds[0] ) );

	queueStart = -1;
	queueEnd = 0;
	areaQueue[0] = areaNum;
	areasVisited[areaNum] = true;

	for ( curArea = areaNum; queueStart < queueEnd; curArea = areaQueue[++queueStart] ) {

		const aasArea_t &area = file->GetArea( curArea );
		bool edgeIntersection = false;
		int withoutAreaNumEdges = numEdges;

		idBounds areaBounds;
		areaBounds.Clear();

		for ( j = 0; j < area.numEdges && !edgeIntersection; j++ ) {
			int edgeNum = file->GetEdgeIndex( area.firstEdge + j );
			const aasEdge_t &edge = file->GetEdge( abs( edgeNum ) );

			edgeBounds[numEdges][0] = edgeBounds[numEdges][1] = file->GetVertex( edge.vertexNum[0] );
			edgeBounds[numEdges].AddPoint( file->GetVertex( edge.vertexNum[1] ) );

			areaBounds.AddPoint( file->GetVertex( edge.vertexNum[0] ) );

			if ( ( edge.flags & ( AAS_EDGE_WALL | AAS_EDGE_LEDGE ) ) == 0 ) {
				continue;
			}

			// test if the edge is already in the list or intersects an existing edge
			for ( k = 0; k < numEdges; k++ ) {
				if ( edgeNum == edges[k] ) {
					break;
				}

				// edges must have the same flags
				const aasEdge_t &otherEdge = file->GetEdge( abs( edges[k] ) );
				if ( ( edge.flags & ( AAS_EDGE_WALL | AAS_EDGE_LEDGE ) ) != ( otherEdge.flags & ( AAS_EDGE_WALL | AAS_EDGE_LEDGE ) ) ) {
					continue;
				}

				// edges must overlap in 2D
				if ( !edgeBounds[numEdges].IntersectsBounds2D( edgeBounds[k] ) ) {
					continue;
				}

				if ( height > 0.0f && EdgesIntersect2D( edgeNum, edges[k], edgeBounds[numEdges], edgeBounds[k], height ) ) {
					edgeIntersection = true;
					break;
				}
			}
			if ( k < numEdges ) {
				continue;
			}

			// add the edge to the list
			edges[numEdges++] = edgeNum;
			if ( numEdges >= maxEdges ) {
				return numEdges;
			}
		}

		// if edges from this area intersected already collected edges
		if ( edgeIntersection ) {
			numEdges = withoutAreaNumEdges;
			continue;
		}

		// if this area is already outside the search bounds
		if ( !bounds.IntersectsBounds( areaBounds ) ) {
			continue;
		}

		// add new areas to the queue
		for ( aasReachability_t *reach = area.reach; reach; reach = reach->next ) {
			if ( ( reach->travelFlags & travelFlags ) != 0 ) {
				// if the area the reachability leads to hasn't been visited yet
				if ( !areasVisited[reach->toAreaNum] ) {
					areaQueue[queueEnd++] = reach->toAreaNum;
					areasVisited[reach->toAreaNum] = true;
				}
			}
		}
	}

	SortWallEdges( edges, numEdges );

	return numEdges;
}
