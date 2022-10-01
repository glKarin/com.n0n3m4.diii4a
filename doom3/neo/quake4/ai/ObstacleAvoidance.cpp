// Copyright (C) 2007 Id Software, Inc.
//



#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "ObstacleAvoidance.h"

/*
===============================================================================

	Dynamic Obstacle Avoidance

	- assumes the AI lives inside a bounding box aligned with the gravity direction
	- obstacles in proximity of the AI are gathered
	- if obstacles are found the AAS walls are also considered as obstacles
	- every obstacle is represented by an oriented bounding box (OBB)
	- an OBB is projected onto a 2D plane orthogonal to AI's gravity direction
	- the 2D windings of the projections are expanded for the AI bbox
	- a path tree is build using clockwise and counter clockwise edge walks along the winding edges
	- the path tree is pruned and optimized
	- the shortest path is chosen for navigation

===============================================================================
*/

idCVar aas_showObstacleAvoidance( "aas_showObstacleAvoidance", "0", CVAR_GAME | CVAR_INTEGER, "shows obstacles along paths" );
idCVar aas_skipObstacleAvoidance( "aas_skipObstacleAvoidance", "0", CVAR_GAME | CVAR_BOOL, "ignore all dynamic obstacles along paths" );

const float idObstacleAvoidance::PUSH_OUTSIDE_OBSTACLES		= 0.5f;
const float idObstacleAvoidance::CLIP_BOUNDS_EPSILON		= 10.0f;

/*
============
idObstacleAvoidance::pathNode_t::Init
============
*/
void idObstacleAvoidance::pathNode_t::Init( void )
{
	dir = 0;
	pos.Zero();
	delta.Zero();
	obstacle = -1;
	edgeNum = -1;
	numNodes = 0;
	parent = children[0] = children[1] = NULL;
}

/*
============
idObstacleAvoidance::LineIntersectsPath
============
*/
bool idObstacleAvoidance::LineIntersectsPath( const idVec2& start, const idVec2& end, const pathNode_t* node ) const
{
	float d0, d1, d2, d3;
	idVec3 plane1, plane2;

	plane1 = idWinding2D::Plane2DFromPoints( start, end );
	d0 = plane1.x * node->pos.x + plane1.y * node->pos.y + plane1.z;
	while( node->parent )
	{
		d1 = plane1.x * node->parent->pos.x + plane1.y * node->parent->pos.y + plane1.z;
		if( IEEE_FLT_SIGNBITSET( d0 ) ^ IEEE_FLT_SIGNBITSET( d1 ) )
		{
			plane2 = idWinding2D::Plane2DFromPoints( node->pos, node->parent->pos );
			d2 = plane2.x * start.x + plane2.y * start.y + plane2.z;
			d3 = plane2.x * end.x + plane2.y * end.y + plane2.z;
			if(   IEEE_FLT_SIGNBITSET( d2 ) ^ IEEE_FLT_SIGNBITSET( d3 ) )
			{
				return true;
			}
		}
		d0 = d1;
		node = node->parent;
	}
	return false;
}

/*
============
idObstacleAvoidance::PointInsideObstacle
============
*/
int idObstacleAvoidance::PointInsideObstacle( const idVec2& point ) const
{
	int i;

	for( i = 0; i < obstacles.Num(); i++ )
	{

		const idVec2* bounds = obstacles[i].bounds;
		if( point.x < bounds[0].x || point.y < bounds[0].y || point.x > bounds[1].x || point.y > bounds[1].y )
		{
			continue;
		}

		if( !obstacles[i].winding.PointInside( point, 0.1f ) )
		{
			continue;
		}

		return i;
	}

	return -1;
}

/*
============
idObstacleAvoidance::LineIntersectsWall
============
*/
bool idObstacleAvoidance::LineIntersectsWall( const idVec2& start, const idVec2& end ) const
{
	int edgeNums[2];
	float scale1, scale2;
	idVec2 bounds[2];
	idVec2 delta = end - start;

	// get bounds for the current movement delta
	bounds[0] = start - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[1] = start + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.x )].x += delta.x;
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.y )].y += delta.y;

	float dist = delta.Length();

	for( int i = 0; i < obstacles.Num(); i++ )
	{
		// only test walls
		if( obstacles[i].id != OBSTACLE_ID_INVALID )
		{
			continue;
		}

		if( bounds[0].x > obstacles[i].bounds[1].x || bounds[0].y > obstacles[i].bounds[1].y ||
				bounds[1].x < obstacles[i].bounds[0].x || bounds[1].y < obstacles[i].bounds[0].y )
		{
			continue;
		}

		if( obstacles[i].winding.RayIntersection( start, delta, scale1, scale2, edgeNums ) )
		{
			if( scale1 * dist > -0.01f && scale1 < 1.0f )
			{
				return true;
			}
		}
	}
	return false;
}

/*
============
idObstacleAvoidance::GetPointOutsideObstacles
============
*/
void idObstacleAvoidance::GetPointOutsideObstacles( idVec2& point, int* obstacle, int* edgeNum )
{
	int i, j, k, n, bestObstacle, bestEdgeNum, queueStart, queueEnd, edgeNums[2];
	float bestd, scale[2];
	idVec2 bestPoint;
	int* queue;
	bool* obstacleVisited;
	idWinding2D w1, w2;

	if( obstacle )
	{
		*obstacle = -1;
	}
	if( edgeNum )
	{
		*edgeNum = -1;
	}

	bestObstacle = PointInsideObstacle( point );
	if( bestObstacle == -1 )
	{
		return;
	}

	const idWinding2D& w = obstacles[bestObstacle].winding;
	bestd = idMath::INFINITY;
	bestEdgeNum = -1;
	for( i = 0; i < w.GetNumPoints(); i++ )
	{
		idVec3 plane = idWinding2D::Plane2DFromPoints( w[( i + 1 ) % w.GetNumPoints()], w[i], true );
		float d = plane.x * point.x + plane.y * point.y + plane.z;
		if( d < bestd )
		{
			// make sure the line from 'point' to 'newPoint' doesn't intersect any wall edges
			idVec2 newPoint = point - ( d + PUSH_OUTSIDE_OBSTACLES ) * plane.ToVec2();
			if( obstacles[bestObstacle].id == OBSTACLE_ID_INVALID || !LineIntersectsWall( point, newPoint ) )
			{
				bestd = d;
				bestPoint = newPoint;
				bestEdgeNum = i;
			}
		}
		// if this is a wall always try to pop out at the first edge
		if( obstacles[bestObstacle].id == OBSTACLE_ID_INVALID )
		{
			break;
		}
	}

	if( bestEdgeNum == -1 )
	{
		return;
	}

	if( PointInsideObstacle( bestPoint ) == -1 )
	{
		point = bestPoint;
		if( obstacle )
		{
			*obstacle = bestObstacle;
		}
		if( edgeNum )
		{
			*edgeNum = bestEdgeNum;
		}
		return;
	}

	queue = ( int* ) _alloca( obstacles.Num() * sizeof( queue[0] ) );
	obstacleVisited = ( bool* ) _alloca( obstacles.Num() * sizeof( obstacleVisited[0] ) );

	queueStart = 0;
	queueEnd = 1;
	queue[0] = bestObstacle;

	memset( obstacleVisited, 0, obstacles.Num() * sizeof( obstacleVisited[0] ) );
	obstacleVisited[bestObstacle] = true;

	bestd = idMath::INFINITY;
	for( i = queue[0]; queueStart < queueEnd; i = queue[++queueStart] )
	{
		w1 = obstacles[i].winding;
		w1.Expand( PUSH_OUTSIDE_OBSTACLES );

		for( j = 0; j < obstacles.Num(); j++ )
		{
			// if the obstacle has been visited already
			if( obstacleVisited[j] )
			{
				continue;
			}
			// if the bounds do not intersect
			if( obstacles[j].bounds[0].x > obstacles[i].bounds[1].x || obstacles[j].bounds[0].y > obstacles[i].bounds[1].y ||
					obstacles[j].bounds[1].x < obstacles[i].bounds[0].x || obstacles[j].bounds[1].y < obstacles[i].bounds[0].y )
			{
				continue;
			}

			queue[queueEnd++] = j;
			obstacleVisited[j] = true;

			w2 = obstacles[j].winding;
			w2.Expand( 0.2f );

			for( k = 0; k < w1.GetNumPoints(); k++ )
			{
				idVec2 dir = w1[( k + 1 ) % w1.GetNumPoints()] - w1[k];
				if( !w2.RayIntersection( w1[k], dir, scale[0], scale[1], edgeNums ) )
				{
					continue;
				}
				for( n = 0; n < 2; n++ )
				{
					idVec2 newPoint = w1[k] + scale[n] * dir;
					if( PointInsideObstacle( newPoint ) == -1 )
					{
						float d = ( newPoint - point ).LengthSqr();
						if( d < bestd )
						{
							// make sure the line from 'point' to 'newPoint' doesn't intersect any wall edges
							if( !LineIntersectsWall( point, newPoint ) )
							{
								bestd = d;
								bestPoint = newPoint;
								bestEdgeNum = edgeNums[n];
								bestObstacle = j;
							}
						}
					}
				}
			}
		}

		if( bestd < idMath::INFINITY )
		{
			point = bestPoint;
			if( obstacle )
			{
				*obstacle = bestObstacle;
			}
			if( edgeNum )
			{
				*edgeNum = bestEdgeNum;
			}
			return;
		}
	}
	idLib::Warning( "GetPointOutsideObstacles: no valid point found" );
}

/*
============
idObstacleAvoidance::GetFirstBlockingObstacle
============
*/
bool idObstacleAvoidance::GetFirstBlockingObstacle( int skipObstacle, const idVec2& startPos, const idVec2& delta, float& blockingScale, int& blockingObstacle, int& blockingEdgeNum )
{
	int i, edgeNums[2];
	float dist, scale1, scale2;
	idVec2 bounds[2];

	// get bounds for the current movement delta
	bounds[0] = startPos - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[1] = startPos + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.x )].x += delta.x;
	bounds[IEEE_FLT_SIGNBITNOTSET( delta.y )].y += delta.y;

	// test for obstacles blocking the path
	blockingScale = idMath::INFINITY;
	dist = delta.Length();
	for( i = 0; i < obstacles.Num(); i++ )
	{
		if( i == skipObstacle )
		{
			continue;
		}
		if( bounds[0].x > obstacles[i].bounds[1].x || bounds[0].y > obstacles[i].bounds[1].y ||
				bounds[1].x < obstacles[i].bounds[0].x || bounds[1].y < obstacles[i].bounds[0].y )
		{
			continue;
		}
		if( obstacles[i].winding.RayIntersection( startPos, delta, scale1, scale2, edgeNums ) )
		{
			if( scale1 < blockingScale && /*scale1 * dist > -0.01f && */ scale2 * dist > 0.01f )
			{
//			if ( scale1 < blockingScale && scale1 * dist > -0.01f && scale2 * dist > 0.01f ) {
				blockingScale = scale1;
				blockingObstacle = i;
				blockingEdgeNum = edgeNums[0];
			}
		}
	}
	return ( blockingScale < 1.0f );
}

/*
============
idObstacleAvoidance::ProjectTopDown
============
*/
void idObstacleAvoidance::ProjectTopDown( idVec3& point ) const
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}
	idMat3 viewAxis;
	idVec3 viewOrigin;
	player->GetViewPos( viewOrigin, viewAxis );
	idMat3 playerAxis = idAngles( 0.0f, -player->viewAngles.yaw, 0.0f ).ToMat3();
	float radius = ( lastQuery.radius * 2.0f );

	//point = ( point - viewOrigin ) * playerAxis;
	point = ( point - lastQuery.startPos ) * playerAxis;
	point = viewOrigin + radius * viewAxis[0] + point.y * viewAxis[1] + point.x * viewAxis[2];
}

/*
============
idObstacleAvoidance::DrawBox
============
*/
void idObstacleAvoidance::DrawBox() const
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}

	//const idVec3 &origin = player->GetPhysics()->GetOrigin();
	const idVec3& origin = lastQuery.startPos;
	float radius = lastQuery.radius;

	idVec3 box[7] = { origin, origin, origin, origin, origin, origin, origin };

	box[0][0] += radius;
	box[0][1] += radius;

	box[1][0] += radius;
	box[1][1] -= radius;

	box[2][0] -= radius;
	box[2][1] -= radius;

	box[3][0] -= radius;
	box[3][1] += radius;

	box[4][1] += radius;

	box[5][0] += radius * 0.1f;
	box[5][1] += radius - radius * 0.1f;

	box[6][0] -= radius * 0.1f;
	box[6][1] += radius - radius * 0.1f;

	for( int i = 0; i < 7; i++ )
	{
		ProjectTopDown( box[i] );
	}
	for( int i = 0; i < 4; i++ )
	{
		gameRenderWorld->DebugLine( colorCyan, box[i], box[( i + 1 ) & 3], 0 );
	}
	gameRenderWorld->DebugLine( colorCyan, box[4], box[5], 0 );
	gameRenderWorld->DebugLine( colorCyan, box[4], box[6], 0 );
}

/*
============
idObstacleAvoidance::GetObstacles
============
*/
int idObstacleAvoidance::GetObstacles( const idBounds& bounds, float radius, const idAAS* aas, int areaNum, const idVec3& startPos, const idVec3& seekPos, idBounds& clipBounds )
{
	int i, j, numVerts, blockingObstacle, blockingEdgeNum;
	int wallEdges[MAX_AAS_WALL_EDGES], numWallEdges, verts[2], lastVerts[2], nextVerts[2];
	float stepHeight, headHeight, blockingScale;
	float halfBoundsSize;
	idVec3 seekDelta, silVerts[32], start, end, nextStart, nextEnd;
	idVec2 expBounds[2], edgeDir, edgeNormal, nextEdgeDir, nextEdgeNormal, lastEdgeNormal;

	seekDelta = seekPos - startPos;

	expBounds[0] = bounds[0].ToVec2() - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	expBounds[1] = bounds[1].ToVec2() + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
	halfBoundsSize = ( expBounds[ 1 ].x - expBounds[ 0 ].x ) * 0.5f + CM_BOX_EPSILON;

	idVec3 invGravity( 0, 0, 1 );

	bounds.AxisProjection( invGravity, stepHeight, headHeight );
	if( aas != NULL )
	{
		stepHeight += aas->GetSettings()->maxStepHeight;
	}

	clipBounds[0] = clipBounds[1] = startPos;
	clipBounds.ExpandSelf( radius );

	for( i = 0; i < obstacles.Num(); i++ )
	{
		obstacle_t& obstacle = obstacles[i];

		numVerts = obstacle.box.GetParallelProjectionSilhouetteVerts( -invGravity, silVerts );

		// create a 2D winding for the obstacle;
		obstacle.winding.Clear();
		for( j = 0; j < numVerts; j++ )
		{
			obstacle.winding.AddPoint( silVerts[j].ToVec2() );
		}

		if( aas_showObstacleAvoidance.GetInteger() != 0 )
		{
			for( j = 0; j < numVerts; j++ )
			{
				silVerts[j].z = startPos.z;
				if( aas_showObstacleAvoidance.GetInteger() > 1 )
				{
					ProjectTopDown( silVerts[j] );
				}
			}
			for( j = 0; j < numVerts; j++ )
			{
				gameRenderWorld->DebugLine( colorWhite, silVerts[j], silVerts[( j + 1 ) % numVerts] );
			}
		}

		// expand the 2D winding for collision with a 2D box
		obstacle.winding.ExpandForAxialBox( expBounds );
		obstacle.winding.GetBounds( obstacle.bounds );
	}

	// if the current path doesn't intersect any dynamic obstacles the path should be through valid AAS space
	if( PointInsideObstacle( startPos.ToVec2() ) == -1 )
	{
		if( !GetFirstBlockingObstacle( -1, startPos.ToVec2(), seekDelta.ToVec2(), blockingScale, blockingObstacle, blockingEdgeNum ) )
		{
			return 0;
		}
	}

	// create obstacles for AAS walls
	if( aas != NULL )
	{
		numWallEdges = aas->GetWallEdges( areaNum, clipBounds, TFL_WALK, wallEdges, MAX_AAS_WALL_EDGES );
		//numWallEdges = aas->GetObstaclePVSWallEdges( areaNum, wallEdges, MAX_AAS_WALL_EDGES );

		lastVerts[0] = lastVerts[1] = 0;
		lastEdgeNormal.Zero();
		nextVerts[0] = nextVerts[1] = 0;

		for( i = 0; i < numWallEdges; i++ )
		{
			aas->GetEdge( wallEdges[i], start, end );
			aas->GetEdgeVertexNumbers( wallEdges[i], verts );

			idBounds bounds( start );
			bounds.AddPoint( end );
			if( !bounds.IntersectsBounds( clipBounds ) )
			{
				continue;
			}

			edgeDir = end.ToVec2() - start.ToVec2();
			edgeDir.Normalize();
			edgeNormal.x = edgeDir.y;
			edgeNormal.y = -edgeDir.x;
			if( i < numWallEdges - 1 )
			{
				aas->GetEdge( wallEdges[i + 1], nextStart, nextEnd );
				aas->GetEdgeVertexNumbers( wallEdges[i + 1], nextVerts );
				nextEdgeDir = nextEnd.ToVec2() - nextStart.ToVec2();
				nextEdgeDir.Normalize();
				nextEdgeNormal.x = nextEdgeDir.y;
				nextEdgeNormal.y = -nextEdgeDir.x;
			}

			obstacle_t& obstacle = obstacles.Alloc();
			obstacle.winding.Clear();
			obstacle.winding.AddPoint( end.ToVec2() );
			obstacle.winding.AddPoint( start.ToVec2() );
			obstacle.winding.AddPoint( start.ToVec2() - edgeDir - edgeNormal * halfBoundsSize );
			obstacle.winding.AddPoint( end.ToVec2() + edgeDir - edgeNormal * halfBoundsSize );
			if( lastVerts[1] == verts[0] )
			{
				obstacle.winding[2] -= lastEdgeNormal * halfBoundsSize;
			}
			else
			{
				obstacle.winding[1] -= edgeDir;
			}
			if( verts[1] == nextVerts[0] )
			{
				obstacle.winding[3] -= nextEdgeNormal * halfBoundsSize;
			}
			else
			{
				obstacle.winding[0] += edgeDir;
			}
			obstacle.winding.GetBounds( obstacle.bounds );
			obstacle.id = OBSTACLE_ID_INVALID;

			memcpy( lastVerts, verts, sizeof( lastVerts ) );
			lastEdgeNormal = edgeNormal;
		}
	}

	// show obstacles
	if( aas_showObstacleAvoidance.GetInteger() != 0 )
	{
		for( i = 0; i < obstacles.Num(); i++ )
		{
			obstacle_t& obstacle = obstacles[i];
			for( j = 0; j < obstacle.winding.GetNumPoints(); j++ )
			{
				silVerts[j].ToVec2() = obstacle.winding[j];
				silVerts[j].z = startPos.z;
				if( aas_showObstacleAvoidance.GetInteger() > 1 )
				{
					ProjectTopDown( silVerts[j] );
				}
			}
			if( obstacle.id == OBSTACLE_ID_INVALID )
			{
				gameRenderWorld->DebugLine( colorRed, silVerts[0], silVerts[1] );
			}
			else
			{
				for( j = 0; j < obstacle.winding.GetNumPoints(); j++ )
				{
					gameRenderWorld->DebugLine( colorGreen, silVerts[j], silVerts[( j + 1 ) % obstacle.winding.GetNumPoints()] );
				}
			}
		}
	}

	return obstacles.Num();
}

/*
============
idObstacleAvoidance::FreePathTree_r
============
*/
void idObstacleAvoidance::FreePathTree_r( pathNode_t* node )
{
	if( node->children[0] )
	{
		FreePathTree_r( node->children[0] );
	}
	if( node->children[1] )
	{
		FreePathTree_r( node->children[1] );
	}
	pathNodeAllocator.Free( node );
}

/*
============
idObstacleAvoidance::DrawPathTree
============
*/
void idObstacleAvoidance::DrawPathTree( const pathNode_t* root, const float height )
{
	int i;
	idVec3 start, end;
	const pathNode_t* node;

	for( node = root; node; node = node->queueNode.GetNext() )
	{
		for( i = 0; i < 2; i++ )
		{
			if( node->children[i] )
			{
				start.ToVec2() = node->pos;
				start.z = height;
				end.ToVec2() = node->children[i]->pos;
				end.z = height;
				if( aas_showObstacleAvoidance.GetInteger() > 1 )
				{
					ProjectTopDown( start );
					ProjectTopDown( end );
				}
				gameRenderWorld->DebugArrow( node->edgeNum == -1 ? colorYellow : i ? colorBlue : colorRed, start, end, 1 );
				break;
			}
		}
	}
}

/*
============
idObstacleAvoidance::GetPathNodeDelta
============
*/
bool idObstacleAvoidance::GetPathNodeDelta( pathNode_t* node, const idVec2& seekPos, bool blocked )
{
	int numPoints, edgeNum;
	bool facing;
	idVec2 seekDelta, dir;
	pathNode_t* n;

	numPoints = obstacles[node->obstacle].winding.GetNumPoints();

	// get delta along the current edge
	while( 1 )
	{
		edgeNum = ( node->edgeNum + node->dir ) % numPoints;
		node->delta = obstacles[node->obstacle].winding[edgeNum] - node->pos;
		if( node->delta.LengthSqr() > 0.01f )
		{
			break;
		}
		node->edgeNum = ( node->edgeNum + numPoints + ( 2 * node->dir - 1 ) ) % numPoints;
	}

	// if not blocked
	if( !blocked )
	{

		// test if the current edge faces the goal
		seekDelta = seekPos - node->pos;
		facing = ( ( 2 * node->dir - 1 ) * ( node->delta.x * seekDelta.y - node->delta.y * seekDelta.x ) ) >= 0.0f;

		// if the current edge faces goal and the line from the current
		// position to the goal does not intersect the current path
		if( facing && !LineIntersectsPath( node->pos, seekPos, node->parent ) )
		{
			node->delta = seekPos - node->pos;
			node->edgeNum = -1;
		}
	}

	// if the delta is along the obstacle edge
	if( node->edgeNum != -1 )
	{
		// if the edge is found going from this node to the root node
		for( n = node->parent; n; n = n->parent )
		{

			if( node->obstacle != n->obstacle || node->edgeNum != n->edgeNum )
			{
				continue;
			}

			// test whether or not the edge segments actually overlap
			if( n->pos * node->delta > ( node->pos + node->delta ) * node->delta )
			{
				continue;
			}
			if( node->pos * node->delta > ( n->pos + n->delta ) * node->delta )
			{
				continue;
			}

			break;
		}
		if( n )
		{
			return false;
		}
	}
	return true;
}

/*
============
idObstacleAvoidance::BuildPathTree
============
*/
idObstacleAvoidance::pathNode_t* idObstacleAvoidance::BuildPathTree( const idBounds& clipBounds,
		const idVec2& startPos,
		const idVec2& seekPos,
		obstaclePath_t& path )
{
	int blockingEdgeNum, blockingObstacle, obstaclePoints, bestNumNodes;
	float blockingScale;
	pathNode_t* root, *node, *child;
	idQueue<pathNode_t, &pathNode_t::queueNode> pathNodeQueue, treeQueue;

	// make sure the tree is never more than MAX_OBSTACLE_PATH nodes deep
	bestNumNodes = MAX_OBSTACLE_PATH / 2;

	root = pathNodeAllocator.Alloc();
	root->Init();
	root->pos = startPos;

	root->delta = seekPos - root->pos;
	root->numNodes = 1;
	pathNodeQueue.Add( root );

	for( node = pathNodeQueue.RemoveFirst(); node && pathNodeAllocator.GetAllocCount() < MAX_PATH_NODES; node = pathNodeQueue.RemoveFirst() )
	{

		treeQueue.Add( node );

		// if this path has more than twice the number of nodes than the best path so far
		if( node->numNodes >= bestNumNodes * 2 )
		{
			continue;
		}

		// don't move outside of the clip bounds
		idVec2 endPos = node->pos + node->delta;
		if( endPos.x - CLIP_BOUNDS_EPSILON < clipBounds[0].x || endPos.x + CLIP_BOUNDS_EPSILON > clipBounds[1].x ||
				endPos.y - CLIP_BOUNDS_EPSILON < clipBounds[0].y || endPos.y + CLIP_BOUNDS_EPSILON > clipBounds[1].y )
		{
			continue;
		}

		// if an obstacle is blocking the path
		if( GetFirstBlockingObstacle( node->obstacle, node->pos, node->delta, blockingScale, blockingObstacle, blockingEdgeNum ) )
		{

			if( path.firstObstacle == OBSTACLE_ID_INVALID )
			{
				path.firstObstacle = obstacles[blockingObstacle].id;
			}

			node->delta *= blockingScale;

			if( node->edgeNum == -1 )
			{
				node->children[0] = pathNodeAllocator.Alloc();
				node->children[0]->Init();
				node->children[1] = pathNodeAllocator.Alloc();
				node->children[1]->Init();
				node->children[0]->dir = 0;
				node->children[1]->dir = 1;
				node->children[0]->parent = node->children[1]->parent = node;
				node->children[0]->pos = node->children[1]->pos = node->pos + node->delta;
				node->children[0]->obstacle = node->children[1]->obstacle = blockingObstacle;
				node->children[0]->edgeNum = node->children[1]->edgeNum = blockingEdgeNum;
				node->children[0]->numNodes = node->children[1]->numNodes = node->numNodes + 1;
				if( GetPathNodeDelta( node->children[0], seekPos, true ) )
				{
					pathNodeQueue.Add( node->children[0] );
				}
				if( GetPathNodeDelta( node->children[1], seekPos, true ) )
				{
					pathNodeQueue.Add( node->children[1] );
				}
			}
			else
			{
				node->children[node->dir] = child = pathNodeAllocator.Alloc();
				child->Init();
				child->dir = node->dir;
				child->parent = node;
				child->pos = node->pos + node->delta;
				child->obstacle = blockingObstacle;
				child->edgeNum = blockingEdgeNum;
				child->numNodes = node->numNodes + 1;
				if( GetPathNodeDelta( child, seekPos, true ) )
				{
					pathNodeQueue.Add( child );
				}
			}
		}
		else
		{
			node->children[node->dir] = child = pathNodeAllocator.Alloc();
			child->Init();
			child->dir = node->dir;
			child->parent = node;
			child->pos = node->pos + node->delta;
			child->numNodes = node->numNodes + 1;

			// there is a free path towards goal
			if( node->edgeNum == -1 )
			{
				if( node->numNodes < bestNumNodes )
				{
					bestNumNodes = node->numNodes;
				}
				continue;
			}

			child->obstacle = node->obstacle;
			obstaclePoints = obstacles[node->obstacle].winding.GetNumPoints();
			child->edgeNum = ( node->edgeNum + obstaclePoints + ( 2 * node->dir - 1 ) ) % obstaclePoints;

			if( GetPathNodeDelta( child, seekPos, false ) )
			{
				pathNodeQueue.Add( child );
			}
		}
	}

	return root;
}

/*
============
idObstacleAvoidance::PrunePathTree
============
*/
void idObstacleAvoidance::PrunePathTree( pathNode_t* root, const idVec2& seekPos )
{
	int i;
	float bestDist;
	pathNode_t* node, *lastNode, *n, *bestNode;

	node = root;
	while( node )
	{

		node->dist = ( seekPos - node->pos ).LengthSqr();

		if( node->children[0] )
		{
			node = node->children[0];
		}
		else if( node->children[1] )
		{
			node = node->children[1];
		}
		else
		{

			// find the node closest to the goal along this path
			bestDist = idMath::INFINITY;
			bestNode = node;
			for( n = node; n; n = n->parent )
			{
				if( n->children[0] && n->children[1] )
				{
					break;
				}
				if( n->dist < bestDist )
				{
					bestDist = n->dist;
					bestNode = n;
				}
			}

			// free tree down from the best node
			for( i = 0; i < 2; i++ )
			{
				if( bestNode->children[i] )
				{
					FreePathTree_r( bestNode->children[i] );
					bestNode->children[i] = NULL;
				}
			}

			for( lastNode = bestNode, node = bestNode->parent; node; lastNode = node, node = node->parent )
			{
				if( node->children[1] && ( node->children[1] != lastNode ) )
				{
					node = node->children[1];
					break;
				}
			}
		}
	}
}

/*
============
OptimizePath
============
*/
int idObstacleAvoidance::OptimizePath( const pathNode_t* root,
									   const pathNode_t* leafNode,
									   idVec2 optimizedPath[MAX_OBSTACLE_PATH] )
{
	int i, numPathPoints, edgeNums[2];
	const pathNode_t* curNode, *nextNode;
	idVec2 curPos, curDelta, bounds[2];
	float scale1, scale2, curLength;

	optimizedPath[0] = root->pos;
	numPathPoints = 1;

	for( nextNode = curNode = root; curNode != leafNode; curNode = nextNode )
	{

		for( nextNode = leafNode; nextNode->parent != curNode; nextNode = nextNode->parent )
		{

			// can only take shortcuts when going from one object to another
			if( nextNode->obstacle == curNode->obstacle )
			{
				continue;
			}

			curPos = curNode->pos;
			curDelta = nextNode->pos - curPos;
			curLength = curDelta.Length();

			// get bounds for the current movement delta
			bounds[0] = curPos - idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
			bounds[1] = curPos + idVec2( CM_BOX_EPSILON, CM_BOX_EPSILON );
			bounds[IEEE_FLT_SIGNBITNOTSET( curDelta.x )].x += curDelta.x;
			bounds[IEEE_FLT_SIGNBITNOTSET( curDelta.y )].y += curDelta.y;

			// test if the shortcut intersects with any obstacles
			for( i = 0; i < obstacles.Num(); i++ )
			{
				if( bounds[0].x > obstacles[i].bounds[1].x || bounds[0].y > obstacles[i].bounds[1].y ||
						bounds[1].x < obstacles[i].bounds[0].x || bounds[1].y < obstacles[i].bounds[0].y )
				{
					continue;
				}
				if( obstacles[i].winding.RayIntersection( curPos, curDelta, scale1, scale2, edgeNums ) )
				{
					if( scale1 >= 0.0f && scale1 <= 1.0f && ( i != nextNode->obstacle || scale1 * curLength < curLength - 0.5f ) )
					{
						break;
					}
					if( scale2 >= 0.0f && scale2 <= 1.0f && ( i != nextNode->obstacle || scale2 * curLength < curLength - 0.5f ) )
					{
						break;
					}
				}
			}
			if( i >= obstacles.Num() )
			{
				break;
			}
		}

		assert( numPathPoints < MAX_OBSTACLE_PATH );

		// store the next position along the optimized path
		optimizedPath[numPathPoints++] = nextNode->pos;
	}

	return numPathPoints;
}

/*
============
PathLength
============
*/
float idObstacleAvoidance::PathLength( const idVec2 optimizedPath[MAX_OBSTACLE_PATH],
									   int numPathPoints, const idVec2& curDir )
{
	int i;
	float pathLength;

	// calculate the path length
	pathLength = 0.0f;
	for( i = 0; i < numPathPoints - 1; i++ )
	{
		pathLength += ( optimizedPath[i + 1] - optimizedPath[i] ).LengthFast();
	}

	// add penalty if this path does not go in the current direction
	if( curDir * ( optimizedPath[1] - optimizedPath[0] ) < 0.0f )
	{
		pathLength += 100.0f;
	}
	return pathLength;
}

/*
============
idObstacleAvoidance::FindOptimalPath

  Returns true if there is a path all the way to the goal.
============
*/
void idObstacleAvoidance::FindOptimalPath( const pathNode_t* root, const float height, const idVec3& curDir, idVec3& seekPos, float& targetDist, float& pathLength )
{
	int bestNumPathPoints;
	const pathNode_t* node, *lastNode, *bestNode;
	idVec2 optimizedPath[MAX_OBSTACLE_PATH];
	float bestPathLength;
	bool optimizedPathCalculated;

	seekPos.Zero();
	seekPos.z = height;

	optimizedPathCalculated = false;

	bestNode = root;
	bestNumPathPoints = 0;
	bestPathLength = idMath::INFINITY;

	node = root;
	while( node )
	{

		if( node->dist <= bestNode->dist )
		{

			if( idMath::Fabs( node->dist - bestNode->dist ) < 1.0f )
			{

				if( !optimizedPathCalculated )
				{
					bestNumPathPoints = OptimizePath( root, bestNode, optimizedPath );
					bestPathLength = PathLength( optimizedPath, bestNumPathPoints, curDir.ToVec2() );
					if( bestNumPathPoints > 1 )
					{
						seekPos.ToVec2() = optimizedPath[1];
					}
					else
					{
						seekPos.ToVec2() = optimizedPath[0];
					}
				}

				int numPathPoints = OptimizePath( root, node, optimizedPath );
				float curPathLength = PathLength( optimizedPath, numPathPoints, curDir.ToVec2() );

				if( curPathLength < bestPathLength )
				{
					bestNode = node;
					bestNumPathPoints = numPathPoints;
					bestPathLength = curPathLength;
					seekPos.ToVec2() = optimizedPath[1];
				}
				optimizedPathCalculated = true;

			}
			else
			{

				bestNode = node;
				optimizedPathCalculated = false;
			}
		}

		if( node->children[0] )
		{
			node = node->children[0];
		}
		else if( node->children[1] )
		{
			node = node->children[1];
		}
		else
		{
			for( lastNode = node, node = node->parent; node; lastNode = node, node = node->parent )
			{
				if( node->children[1] && node->children[1] != lastNode )
				{
					node = node->children[1];
					break;
				}
			}
		}
	}

	if( !optimizedPathCalculated )
	{
		int numPathPoints = OptimizePath( root, bestNode, optimizedPath );
		pathLength = PathLength( optimizedPath, numPathPoints, curDir.ToVec2() );
		seekPos.ToVec2() = optimizedPath[1];
	}
	else
	{
		pathLength = bestPathLength;
	}

	targetDist = idMath::Sqrt( bestNode->dist );

	if( aas_showObstacleAvoidance.GetBool() )
	{
		idVec3 start, end;
		start.z = end.z = height + 4.0f;
		int numPathPoints = OptimizePath( root, bestNode, optimizedPath );
		for( int i = 0; i < numPathPoints - 1; i++ )
		{
			start.ToVec2() = optimizedPath[ i ];
			end.ToVec2() = optimizedPath[ i + 1];
			if( aas_showObstacleAvoidance.GetInteger() > 1 )
			{
				ProjectTopDown( start );
				ProjectTopDown( end );
			}
			gameRenderWorld->DebugArrow( colorCyan, start, end, 1 );
		}
	}
}

/*
============
idObstacleAvoidance::ClearObstacles
============
*/
void idObstacleAvoidance::ClearObstacles( void )
{
	obstacles.SetNum( 0 );
}

/*
============
idObstacleAvoidance::AddObstacle
============
*/
void idObstacleAvoidance::AddObstacle( const idBox& box, int id )
{
	obstacle_t& obstacle = obstacles.Alloc();
	obstacle.box = box;
	obstacle.id = id;
}

/*
============
idObstacleAvoidance::RemoveObstacle
============
*/
void idObstacleAvoidance::RemoveObstacle( int id )
{
	for( int i = 0; i < obstacles.Num(); i++ )
	{
		if( obstacles[i].id == id )
		{
			obstacles.RemoveIndex( i );
			return;
		}
	}
}

/*
============
idObstacleAvoidance::FindPathAroundObstacles

  Finds a path around dynamic obstacles using a path tree with clockwise and counter clockwise edge walks.
  If aas != NULL a path is constructed inside valid AAS space.
============
*/
bool idObstacleAvoidance::FindPathAroundObstacles( const idBounds& bounds, const float radius, const idAAS* aas, const idVec3& startPos, const idVec3& seekPos, obstaclePath_t& path, bool alwaysAvoidObstacles )
{
	int areaNum = -1;
	int startInsideObstacle, destInsideObstacle;
	idBounds clipBounds;
	pathNode_t* root;

	lastQuery.bounds = bounds;
	lastQuery.radius = radius;
	lastQuery.startPos = startPos;
	lastQuery.seekPos = seekPos;

//mal: debug code.
	bool save = false;
	if( save )
	{
		SaveLastQuery( "test" );
	}

	// cap the seek position within the obstacle radius
	idVec3 cappedSeekPos = seekPos;
	if( aas != NULL )
	{
		idVec3 moveDir = cappedSeekPos - startPos;
		float moveLength = moveDir.Normalize();
// jmarshall - check this on a per obstancle basis
		//if ( moveLength > aas->GetSettings()->obstaclePVSRadius ) {
		//	cappedSeekPos = startPos + moveDir * aas->GetSettings()->obstaclePVSRadius * 0.9f;
		//}
		if( moveLength > 1024 )
		{
			cappedSeekPos = startPos + moveDir * 1024.0f * 0.9f;
		}
// jmarshall end
	}

	// initialize the obstacle path in case there are no obstacles
	path.seekPos = cappedSeekPos;
	path.originalSeekPos = seekPos;
	path.firstObstacle = OBSTACLE_ID_INVALID;
	path.startPosOutsideObstacles = startPos;
	path.startPosObstacle = OBSTACLE_ID_INVALID;
	path.seekPosOutsideObstacles = cappedSeekPos;
	path.seekPosObstacle = OBSTACLE_ID_INVALID;
	path.pathLength = idMath::INFINITY;
	path.hasValidPath = true;

	// get the AAS area number and a valid point inside that area
	if( aas != NULL )
	{
		areaNum = aas->PointReachableAreaNum( path.startPosOutsideObstacles, bounds, AREA_REACHABLE_WALK );
		aas->PushPointIntoAreaNum( areaNum, path.startPosOutsideObstacles );
	}

	if( aas_skipObstacleAvoidance.GetBool() )
	{
		return true;
	}

	// if there are no dynamic obstacles the path should be through valid AAS space
	if( obstacles.Num() == 0 )
	{
		return true;
	}

	// get all the nearby obstacles
	GetObstacles( bounds, radius, aas, areaNum, path.startPosOutsideObstacles, path.seekPosOutsideObstacles, clipBounds );

	// get a source position outside the obstacles
	GetPointOutsideObstacles( path.startPosOutsideObstacles.ToVec2(), &startInsideObstacle, NULL );
	if( startInsideObstacle != -1 )
	{
		path.startPosObstacle = obstacles[startInsideObstacle].id;
	}

	// get a goal position outside the obstacles
	GetPointOutsideObstacles( path.seekPosOutsideObstacles.ToVec2(), &destInsideObstacle, NULL );
	if( destInsideObstacle != -1 )
	{
		path.seekPosObstacle = obstacles[destInsideObstacle].id;
	}

	// if start and destination are pushed to the same point, we don't have a path around the obstacle
	if( ( path.seekPosOutsideObstacles.ToVec2() - path.startPosOutsideObstacles.ToVec2() ).LengthSqr() < Square( 1.0f ) )
	{
		if( startInsideObstacle != -1 )
		{
			path.startPosObstacle = obstacles[ startInsideObstacle ].id;
		}
		if( destInsideObstacle != -1 )
		{
			path.seekPosObstacle = obstacles[ destInsideObstacle ].id;
		}
		if( path.startPosObstacle != OBSTACLE_ID_INVALID )
		{
			path.firstObstacle = path.startPosObstacle;
		}
		else
		{
			path.firstObstacle = path.seekPosObstacle;
		}

		if( alwaysAvoidObstacles )
		{
			path.seekPos = path.startPosOutsideObstacles;
		}

		if( ( cappedSeekPos.ToVec2() - startPos.ToVec2() ).LengthSqr() > Square( 2.0f ) )
		{
			return false;
		}
		return true;
	}

	// build a path tree
	root = BuildPathTree( clipBounds, path.startPosOutsideObstacles.ToVec2(), path.seekPosOutsideObstacles.ToVec2(), path );

	// draw the path tree
	if( aas_showObstacleAvoidance.GetInteger() != 0 )
	{
		DrawPathTree( root, startPos.z );
		if( aas_showObstacleAvoidance.GetInteger() > 1 )
		{
			DrawBox();
		}
	}

	// prune the tree
	PrunePathTree( root, path.seekPosOutsideObstacles.ToVec2() );

	// find the optimal path
	FindOptimalPath( root, startPos.z, cappedSeekPos - startPos, path.seekPos, path.targetDist, path.pathLength );

	// free the tree
	FreePathTree_r( root );

	// a valid path is found if the path gets close enough to the seekPos
	path.hasValidPath = ( path.targetDist < 1.0f );

	return path.hasValidPath;
}

/*
===============
idObstacleAvoidance::PointInObstacles

  Returns true if the point is inside an obstacle.
  If aas != NULL the point is first pushed into valid AAS space.
===============
*/
bool idObstacleAvoidance::PointInObstacles( const idBounds& bounds, const float radius, const idAAS* aas, const idVec3& pos )
{
	int			areaNum = -1;
	int			insideObstacle;
	idBounds	clipBounds;
	idVec3		posOutsideObstacles;

	posOutsideObstacles = pos;

	// get the AAS area number and a valid point inside that area
	if( aas != NULL )
	{
		areaNum = aas->PointReachableAreaNum( posOutsideObstacles, bounds, AREA_REACHABLE_WALK );
		aas->PushPointIntoAreaNum( areaNum, posOutsideObstacles );
	}

	// get all the nearby obstacles
	GetObstacles( bounds, radius, aas, areaNum, posOutsideObstacles, posOutsideObstacles, clipBounds );

	// get a source position outside the obstacles
	GetPointOutsideObstacles( posOutsideObstacles.ToVec2(), &insideObstacle, NULL );
	if( insideObstacle != -1 )
	{
		return true;
	}

	return false;
}

#define OBSTACLE_AVOIDANCE_QUERY_ID		"ObstacleAvoidanceQuery"

/*
===============
idObstacleAvoidance::SaveLastQuery
===============
*/
bool idObstacleAvoidance::SaveLastQuery( const char* fileName )
{
	idFile* file = fileSystem->OpenFileWrite( fileName, "fs_savepath" );
	if( file == NULL )
	{
		return false;
	}

	file->WriteString( OBSTACLE_AVOIDANCE_QUERY_ID );
	file->WriteVec3( lastQuery.bounds[0] );
	file->WriteVec3( lastQuery.bounds[1] );
	file->WriteFloat( lastQuery.radius );
	file->WriteVec3( lastQuery.startPos );
	file->WriteVec3( lastQuery.seekPos );
	file->WriteInt( obstacles.Num() );
	file->Write( &obstacles[0], obstacles.Num() * sizeof( obstacles[0] ) );

	fileSystem->CloseFile( file );

	return true;
}

/*
===============
idObstacleAvoidance::TestQuery
===============
*/
bool idObstacleAvoidance::TestQuery( const char* fileName, const idAAS* aas )
{
	idFile* file = fileSystem->OpenFileRead( fileName );
	if( file == NULL )
	{
		return false;
	}

	idStr id;
	file->ReadString( id );
	if( id != OBSTACLE_AVOIDANCE_QUERY_ID )
	{
		fileSystem->CloseFile( file );
		return false;
	}
	file->ReadVec3( lastQuery.bounds[0] );
	file->ReadVec3( lastQuery.bounds[1] );
	file->ReadFloat( lastQuery.radius );
	file->ReadVec3( lastQuery.startPos );
	file->ReadVec3( lastQuery.seekPos );
	int num;
	file->ReadInt( num );
	obstacles.SetNum( num );
	file->Read( &obstacles[0], obstacles.Num() * sizeof( obstacles[0] ) );

	fileSystem->CloseFile( file );

	obstaclePath_t path;

	bool result = FindPathAroundObstacles( lastQuery.bounds, lastQuery.radius, aas, lastQuery.startPos, lastQuery.seekPos, path );

	gameRenderWorld->DebugBounds( colorOrange, lastQuery.bounds, lastQuery.startPos );

	return true;
}
