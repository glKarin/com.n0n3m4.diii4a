
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "AAS_local.h"
#include "../Game_local.h"		// for cvars and debug drawing
#include "AI.h"
#include "AAS_Find.h"


/*
============
idAASLocal::DrawCone
============
*/
void idAASLocal::DrawCone( const idVec3 &origin, const idVec3 &dir, float radius, const idVec4 &color ) const {
	int i;
	idMat3 axis;
	idVec3 center, top, p, lastp;

	axis[2] = dir;
	axis[2].NormalVectors( axis[0], axis[1] );
	axis[1] = -axis[1];

	center = origin + dir;
	top = center + dir * (3.0f * radius);
	lastp = center + radius * axis[1];

	for ( i = 20; i <= 360; i += 20 ) {
		p = center + idMath::Sin( DEG2RAD(i) ) * radius * axis[0] + idMath::Cos( DEG2RAD(i) ) * radius * axis[1];
		gameRenderWorld->DebugLine( color, lastp, p, 0 );
		gameRenderWorld->DebugLine( color, p, top, 0 );
		lastp = p;
	}
}

/*
============
idAASLocal::DrawReachability
============
*/
void idAASLocal::DrawReachability( const idReachability *reach ) const {
	gameRenderWorld->DebugArrow( colorCyan, reach->start, reach->end, 2 );

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", reach->edgeNum ), ( reach->start + reach->end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis );
	}

	switch( reach->travelType ) {
		case TFL_WALK: {
//			const idReachability_Walk *walk = static_cast<const idReachability_Walk *>(reach);
			break;
		}
		default: {
			break;
		}
	}
}

/*
============
idAASLocal::DrawEdge
============
*/
void idAASLocal::DrawEdge( int edgeNum, bool arrow ) const {
	const aasEdge_t *edge;
	idVec4 *color;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
	color = &colorRed;
	if ( arrow ) {
		gameRenderWorld->DebugArrow( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ), 1 );
	} else {
		gameRenderWorld->DebugLine( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ) );
	}

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", edgeNum ), ( file->GetVertex( edge->vertexNum[0] ) + file->GetVertex( edge->vertexNum[1] ) ) * 0.5f + idVec3(0,0,4), 0.1f, colorRed, gameLocal.GetLocalPlayer()->viewAxis );
	}
}

/*
============
idAASLocal::DrawFace
============
*/
void idAASLocal::DrawFace( int faceNum, bool side ) const {
	int i, j, numEdges, firstEdge;
	const aasFace_t *face;
	idVec3 mid, end;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	if ( !numEdges )
	{//wtf?  A face with no edges?!
		return;
	}

	mid = vec3_origin;
	for ( i = 0; i < numEdges; i++ ) {
		DrawEdge( abs( file->GetEdgeIndex( firstEdge + i ) ), ( face->flags & FACE_FLOOR ) != 0 );
		j = file->GetEdgeIndex( firstEdge + i );
		mid += file->GetVertex( file->GetEdge( abs( j ) ).vertexNum[ j < 0 ] );
	}

	mid /= numEdges;
	if ( side ) {
		end = mid - 5.0f * file->GetPlane( file->GetFace( faceNum ).planeNum ).Normal();
	} else {
		end = mid + 5.0f * file->GetPlane( file->GetFace( faceNum ).planeNum ).Normal();
	}
	gameRenderWorld->DebugArrow( colorGreen, mid, end, 1 );
}

/*
============
idAASLocal::DrawAreaBounds
============
*/
void idAASLocal::DrawAreaBounds( int areaNum ) const {
	const aasArea_t *area;
	if ( !file ) {
		return;
	}
	area = &file->GetArea( areaNum );

	idVec3 points[8];
	bool	drawn[8][8];
	memset( drawn, false, sizeof( drawn ) );
	
	area->bounds.ToPoints( points );
	for ( int p1 = 0; p1 < 8; p1++ ) {
		for ( int p2 = 0; p2 < 8; p2++ ) {
			if ( !drawn[p2][p1] ) {
				if ( (points[p1].x == points[p2].x && (points[p1].y == points[p2].y||points[p1].z == points[p2].z))
					|| (points[p1].y == points[p2].y && (points[p1].x == points[p2].x||points[p1].z == points[p2].z))
					|| (points[p1].z == points[p2].z && (points[p1].x == points[p2].x||points[p1].y == points[p2].y)) ) {
					//an edge
					gameRenderWorld->DebugLine( colorRed, points[p1], points[p2] );
					drawn[p1][p2] = true;
				}
			}
		}
	}
}

/*
============
idAASLocal::DrawArea
============
*/
void idAASLocal::DrawArea( int areaNum ) const {
	int i, numFaces, firstFace;
	const aasArea_t *area;
	idReachability *reach;
	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );

	if ( aas_showAreaBounds.GetBool() ) {
		if ( (area->flags&AREA_FLOOR)  ) {
			idVec3 areaTop = area->center;
			areaTop.z = area->ceiling;
			gameRenderWorld->DebugArrow( colorCyan, area->center, areaTop, 1 );
			gameRenderWorld->DrawText( va( "%4.2f", floor(area->ceiling-area->center.z) ), ( area->center + areaTop ) * 0.5f, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
		} else {//air area
			DrawAreaBounds( areaNum );
		}
	}

	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for ( i = 0; i < numFaces; i++ ) {
		DrawFace( abs( file->GetFaceIndex( firstFace + i ) ), file->GetFaceIndex( firstFace + i ) < 0 );
	}

	if (aas_showRevReach.GetInteger())
 	{
 		for ( reach = area->rev_reach; reach; reach = reach->rev_next ) {
 			DrawReachability( reach );
 		}
 	}
 	else
 	{
 		for ( reach = area->reach; reach; reach = reach->next ) {
 			DrawReachability( reach );
 		}
	}
}

/*
============
idAASLocal::DefaultSearchBounds
============
*/
const idBounds &idAASLocal::DefaultSearchBounds( void ) const {
	return file->GetSettings().boundingBoxes[0];
}

/*
============
idAASLocal::ShowArea
============
*/
void idAASLocal::ShowArea( const idVec3 &origin ) const {
	static int lastAreaNum;
	int areaNum;
	const aasArea_t *area;
	idVec3 org;

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	org = origin;
	PushPointIntoAreaNum( areaNum, org );

	if ( aas_goalArea.GetInteger() ) {
		int travelTime;
		idReachability *reach;
		
		RouteToGoalArea( areaNum, org, aas_goalArea.GetInteger(), TFL_WALK|TFL_AIR, travelTime, &reach );
		gameLocal.Printf( "\rtt = %4d", travelTime );
		if ( reach ) {
			gameLocal.Printf( " to area %4d", reach->toAreaNum );
			DrawArea( reach->toAreaNum );
		}
	}

	if ( areaNum != lastAreaNum ) {
		area = &file->GetArea( areaNum );
		gameLocal.Printf( "area %d: ", areaNum );
		if ( area->flags & AREA_LEDGE ) {
			gameLocal.Printf( "AREA_LEDGE " );
		}
		if ( area->flags & AREA_REACHABLE_WALK ) {
			gameLocal.Printf( "AREA_REACHABLE_WALK " );
		}
		if ( area->flags & AREA_REACHABLE_FLY ) {
			gameLocal.Printf( "AREA_REACHABLE_FLY " );
		}
		if ( area->contents & AREACONTENTS_CLUSTERPORTAL ) {
			gameLocal.Printf( "AREACONTENTS_CLUSTERPORTAL " );
		}
		if ( area->contents & AREACONTENTS_OBSTACLE ) {
			gameLocal.Printf( "AREACONTENTS_OBSTACLE " );
		}
		gameLocal.Printf( "\n" );
		lastAreaNum = areaNum;
	}

	if ( org != origin ) {
		idBounds bnds = file->GetSettings().boundingBoxes[ 0 ];
		bnds[ 1 ].z = bnds[ 0 ].z;
		gameRenderWorld->DebugBounds( colorYellow, bnds, org );
	}

	DrawArea( areaNum );
}

// RAVEN BEGIN 
// rjohnson: added more debug drawing
/*
============
idAASLocal::DrawSimpleEdge
============
*/
void idAASLocal::DrawSimpleEdge( int edgeNum ) const {
	const aasEdge_t *edge;
	const idVec4	*color;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
	color = &file->GetSettings().debugColor;

	gameRenderWorld->DebugLine( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ) );
}

/*
============
idAASLocal::DrawSimpleFace
============
*/
const int 	MAX_AAS_WALL_EDGES			= 256;

// RAVEN BEGIN
// cdr: added visited check
void idAASLocal::DrawSimpleFace( int faceNum, bool visited ) const {
// RAVEN END
	int				i, numEdges, firstEdge, edgeNum;
	const			aasFace_t *face;
	idVec3			sides[MAX_AAS_WALL_EDGES];
	const aasEdge_t *edge, *nextEdge;
	idVec4			color2;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	for ( i = 0; i < numEdges; i++ ) {
		DrawSimpleEdge( abs( file->GetEdgeIndex( firstEdge + i ) ) );
	}

	if ( numEdges >= 2 && numEdges <= MAX_AAS_WALL_EDGES ) {
		edgeNum = abs( file->GetEdgeIndex( firstEdge ) );
		edge = &file->GetEdge( abs( edgeNum ) );
		edgeNum = abs( file->GetEdgeIndex( firstEdge + 1 ) );
		nextEdge = &file->GetEdge( abs( edgeNum ) );

		// need to find the first common edge so that we go form the polygon in the right direction
		if ( file->GetVertex( edge->vertexNum[0] ) == file->GetVertex( nextEdge->vertexNum[0] ) || 
			file->GetVertex( edge->vertexNum[0] ) == file->GetVertex( nextEdge->vertexNum[1] ) ) {
			sides[ 0 ] = file->GetVertex( edge->vertexNum[0] );
		} else {
			sides[ 0 ] = file->GetVertex( edge->vertexNum[1] );
		}

		for ( i = 1; i < numEdges; i++ ) {
			edgeNum = abs( file->GetEdgeIndex( firstEdge + i ) );
			edge = &file->GetEdge( abs( edgeNum ) );

			if ( sides[ i-1 ] == file->GetVertex( edge->vertexNum[0] ) ) {
				sides[ i ] = file->GetVertex( edge->vertexNum[1] );
			} else {
				sides[ i ] = file->GetVertex( edge->vertexNum[0] );
			}
		}

		color2 = file->GetSettings().debugColor;
		color2[3] = 0.20f;

		// RAVEN BEGIN
		// cdr: added visited check
		if (!visited)
		{
			color2[3] = 0.05f;
		}
		// RAVEN END
		idWinding winding( sides, numEdges ); 
		gameRenderWorld->DebugPolygon( color2, winding, 0, true );
	}
}


/*
============
idAASLocal::DrawSimpleArea
============
*/
void idAASLocal::DrawSimpleArea( int areaNum ) const {
	int				i, numFaces, firstFace;
	const aasArea_t *area;

	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );

	if ( aas_showAreaBounds.GetBool() ) {
		if ( (area->flags&AREA_FLOOR) ) {
			idVec3 areaTop = area->center;
			areaTop.z = area->ceiling;
			gameRenderWorld->DebugArrow( colorCyan, area->center, areaTop, 1 );
			gameRenderWorld->DrawText( va( "%4.2f", floor(area->ceiling-area->center.z) ), ( area->center + areaTop ) * 0.5f, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
		} else {//air area
			DrawAreaBounds( areaNum );
		}
	}

	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for ( i = 0; i < numFaces; i++ ) {
		DrawSimpleFace( abs( file->GetFaceIndex( firstFace + i )), true );
	}
}

/*
============
idAASLocal::IsValid
============
*/
bool idAASLocal::IsValid( void ) const {
	if ( !file ) {
		return false;
	}

	return true;
}

/*
============
idAASLocal::ShowAreas
============
*/
void idAASLocal::ShowAreas( const idVec3 &origin, bool ShowProblemAreas ) const {
	int				i, areaNum;
	idPlayer		*player;
	int				*areaQueue, curArea, queueStart, queueEnd;
	byte			*areasVisited;
	const aasArea_t *area;
	idReachability	*reach;
	int				travelFlags = (TFL_WALK);
	const idBounds	bounds = idBounds( origin ).Expand( 256.0f );

	if ( !file ) {
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );

	areasVisited = (byte *) _alloca16( file->GetNumAreas() );
	memset( areasVisited, 0, file->GetNumAreas() * sizeof( byte ) );
	areaQueue = (int *) _alloca16( file->GetNumAreas() * sizeof( int ) );

	queueStart = 0;
	queueEnd = 1;
	areaQueue[0] = areaNum;
	areasVisited[areaNum] = true;

	for ( curArea = areaNum; queueStart < queueEnd; curArea = areaQueue[++queueStart] ) {
		area = &file->GetArea( curArea );

		// add new areas to the queue
		for ( reach = area->reach; reach; reach = reach->next ) {
			if ( reach->travelType & travelFlags ) {
				// if the area the reachability leads to hasn't been visited yet and the area bounds touch the search bounds
				if ( !areasVisited[reach->toAreaNum] && bounds.IntersectsBounds( file->GetArea( reach->toAreaNum ).bounds ) ) {
					areaQueue[queueEnd++] = reach->toAreaNum;
					areasVisited[reach->toAreaNum] = true;
				}
			}
		}
	}

	if ( ShowProblemAreas ) {
		for ( i = 0; i < queueEnd; i++ ) {
			ShowProblemArea( areaQueue[ i ] );
		}
	} else {
		for ( i = 0; i < queueEnd; i++ ) {
			DrawSimpleArea( areaQueue[ i ] );
		}
	}
}

/*
============
idAASLocal::ShowProblemEdge
============
*/
void idAASLocal::ShowProblemEdge( int edgeNum ) const {
	const aasEdge_t	*edge;
	const idVec4	*color;
	idVec4			color2;
	idTraceModel	trm;
	idClipModel		mdl;
	idVec3			sides[4];
	trace_t			results;
	idVec3			forward, forwardNormal, left, down, start, end;
	float			hullSize, hullHeight;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );

	start = (idVec3)file->GetVertex( edge->vertexNum[0] );
	end = (idVec3)file->GetVertex( edge->vertexNum[1] );
	forward =  end - start ;
	forward.Normalize();
	forwardNormal = forward;
	forward.NormalVectors( left, down );

	hullSize = ( ( file->GetSettings().boundingBoxes[0][1][0] - file->GetSettings().boundingBoxes[0][0][0] ) / 2.0f ) - 1.0f;	// assumption that x and y are the same size
	hullHeight = ( file->GetSettings().boundingBoxes[0][1][2] - file->GetSettings().boundingBoxes[0][0][2] );	

	left *= hullSize;
	forward *= hullSize;

	sides[0] = -left + idVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][0][2] + 1.0f );
	sides[1] = left + idVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][0][2] + 1.0f );
	sides[2] = left + idVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][1][2] - 1.0f );
	sides[3] = -left + idVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][1][2] - 1.0f );
	trm.SetupPolygon( sides, 4 );
	mdl.LoadModel( trm, NULL );
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.Translation( gameLocal.GetLocalPlayer(), results, start, end, &mdl, mat3_identity, MASK_MONSTERSOLID, gameLocal.GetLocalPlayer() );
// RAVEN END

	if ( results.fraction != 1.0f ) {
		color = &file->GetSettings().debugColor;
		gameRenderWorld->DebugLine( *color, start - left, end - left );
		gameRenderWorld->DebugLine( *color, start + left, end + left );

		color = &colorYellow;
		gameRenderWorld->DebugLine( *color, start, end );

		color2 = colorOrange;
		color2[3] = 0.25f;
		sides[0] += results.endpos;
		sides[1] += results.endpos;
		sides[2] += results.endpos;
		sides[3] += results.endpos;
		idWinding winding( sides, 4 );
		gameRenderWorld->DebugPolygon( color2, winding );
	}
}

/*
============
idAASLocal::ShowProblemFace
============
*/
void idAASLocal::ShowProblemFace( int faceNum ) const {
	int		i, numEdges, firstEdge;
	const	aasFace_t *face;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	for ( i = 0; i < numEdges; i++ ) {
		ShowProblemEdge( abs( file->GetEdgeIndex( firstEdge + i ) ) );
	}
}

/*
============
idAASLocal::ShowProblemArea
============
*/
void idAASLocal::ShowProblemArea( int areaNum ) const {
	int				i, numFaces, firstFace;
	const aasArea_t *area;
	idEntity *		entityList[ MAX_GENTITIES ];
	int				numListedEntities;
	idBounds		bounds;
	float			hullSize;

	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );
	numFaces = area->numFaces;
	firstFace = area->firstFace;

	hullSize = ( ( file->GetSettings().boundingBoxes[0][1][0] - file->GetSettings().boundingBoxes[0][0][0] ) / 2.0f );	// assumption that x and y are the same size

	bounds = area->bounds;
	bounds.Expand( hullSize );
// RAVEN BEGIN
// ddynerman: multiple clip world
	numListedEntities = gameLocal.EntitiesTouchingBounds( gameLocal.GetLocalPlayer(), bounds, -1, entityList, MAX_GENTITIES );
// RAVEN END
	for( i = 0; i < numListedEntities; i++) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( entityList[ i ]->IsType( idMoveable::GetClassType() )  ||
			 entityList[ i ]->IsType( idMover::GetClassType() ) ||
			 entityList[ i ]->IsType( idAI::GetClassType() ) ) {
// RAVEN END
			entityList[ i ]->GetPhysics()->DisableClip();
			continue;
		}

		entityList[ i ] = NULL;
	}

	for ( i = 0; i < numFaces; i++ ) {
		ShowProblemFace( abs( file->GetFaceIndex( firstFace + i ) ) );
	}

	for( i = 0; i < numListedEntities; i++) {
		if ( entityList[ i ] ) {
			entityList[ i ]->GetPhysics()->EnableClip();
		}
	}
}

/*
============
idAASLocal::ShowProblemArea
============
*/
void idAASLocal::ShowProblemArea( const idVec3 &origin ) const {
	static int	lastAreaNum;
	int			areaNum;
	idVec3		org;

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );

	ShowProblemArea( areaNum );
}

// RAVEN END

/*
============
idAASLocal::ShowWalkPath
============
*/
void idAASLocal::ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const {
	int i, areaNum, curAreaNum, travelTime;
	idReachability *reach;
	idVec3 org, areaCenter;
	aasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_WALK );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_AIR, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorGreen, org, reach->start, 2 );
		DrawReachability( reach );

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->end;
	}

	if ( WalkPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_AIR ) ) {
		gameRenderWorld->DebugArrow( colorBlue, origin, path.moveGoal, 2 );
	}
}

/*
============
idAASLocal::ShowFlyPath
============
*/
void idAASLocal::ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const {
	int i, areaNum, curAreaNum, travelTime;
	idReachability *reach;
	idVec3 org, areaCenter;
	aasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_FLY );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_FLY|TFL_AIR, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorPurple, org, reach->start, 2 );
		DrawReachability( reach );

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->end;
	}

	if ( FlyPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_FLY|TFL_AIR ) ) {
		gameRenderWorld->DebugArrow( colorBlue, origin, path.moveGoal, 2 );
	}
}

/*
============
idAASLocal::ShowWallEdges
============
*/
void idAASLocal::ShowWallEdges( const idVec3 &origin ) const {
	int i, areaNum, numEdges, edges[1024];
	idVec3 start, end;
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	numEdges = GetWallEdges( areaNum, idBounds( origin ).Expand( 256.0f ), TFL_WALK, edges, 1024 );
	for ( i = 0; i < numEdges; i++ ) {
		GetEdge( edges[i], start, end );
		gameRenderWorld->DebugLine( colorRed, start, end );
		gameRenderWorld->DrawText( va( "%d", edges[i] ), ( start + end ) * 0.5f, 0.1f, colorWhite, player->viewAxis );
	}
}

/*
============
idAASLocal::ShowHideArea
============
*/
void idAASLocal::ShowHideArea( const idVec3 &origin, int targetAreaNum ) const {
	int areaNum, numObstacles;
	idVec3 target;
	aasGoal_t goal;
	aasObstacle_t obstacles[10];

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	target = AreaCenter( targetAreaNum );

	// consider the target an obstacle
	obstacles[0].absBounds = idBounds( target ).Expand( 16 );
	numObstacles = 1;

	DrawCone( target, idVec3(0,0,1), 16.0f, colorYellow );

	rvAASFindGoalForHide findHide ( target );
	if ( FindNearestGoal( goal, areaNum, origin, target, TFL_WALK|TFL_AIR, 0.0f, 0.0f, obstacles, numObstacles, findHide ) ) {
		DrawArea( goal.areaNum );
		ShowWalkPath( origin, goal.areaNum, goal.origin );
		DrawCone( goal.origin, idVec3(0,0,1), 16.0f, colorWhite );
	}
}

/*
============
idAASLocal::PullPlayer
============
*/
bool idAASLocal::PullPlayer( const idVec3 &origin, int toAreaNum ) const {
	int areaNum;
	idVec3 areaCenter, dir, vel;
	idAngles delta;
	aasPath_t path;
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return true;
	}

	idPhysics *physics = player->GetPhysics();
	if ( !physics ) {
		return true;
	}

	if ( !toAreaNum ) {
		return false;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	areaCenter = AreaCenter( toAreaNum );
	if ( player->GetPhysics()->GetAbsBounds().Expand( 8 ).ContainsPoint( areaCenter ) ) {
		return false;
	}
	if ( WalkPathToGoal( path, areaNum, origin, toAreaNum, areaCenter, TFL_WALK|TFL_AIR ) ) {
		dir = path.moveGoal - origin;
		dir[2] *= 0.5f;
		dir.Normalize();
		delta = dir.ToAngles() - player->cmdAngles - player->GetDeltaViewAngles();
		delta.Normalize180();
		player->SetDeltaViewAngles( player->GetDeltaViewAngles() + delta * 0.1f );
		dir[2] = 0.0f;
		dir.Normalize();
		dir *= 100.0f;
		vel = physics->GetLinearVelocity();
		dir[2] = vel[2];
		physics->SetLinearVelocity( dir );
		return true;
	}
	else {
		return false;
	}
}

/*
============
idAASLocal::RandomPullPlayer
============
*/
void idAASLocal::RandomPullPlayer( const idVec3 &origin ) const {
	int rnd, i, n;

	if ( !PullPlayer( origin, aas_pullPlayer.GetInteger() ) ) {

		rnd = gameLocal.random.RandomFloat() * file->GetNumAreas();

		for ( i = 0; i < file->GetNumAreas(); i++ ) {
			n = (rnd + i) % file->GetNumAreas();
			if ( file->GetArea( n ).flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) ) {
				aas_pullPlayer.SetInteger( n );
			}
		}
	} else {
		ShowWalkPath( origin, aas_pullPlayer.GetInteger(), AreaCenter( aas_pullPlayer.GetInteger() ) );
	}
}

/*
============
idAASLocal::ShowPushIntoArea
============
*/
void idAASLocal::ShowPushIntoArea( const idVec3 &origin ) const {
	int areaNum;
	idVec3 target;

	target = origin;
	areaNum = PointReachableAreaNum( target, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	PushPointIntoAreaNum( areaNum, target );
	gameRenderWorld->DebugArrow( colorGreen, origin, target, 1 );
}

/*
============
idAASLocal::Test
============
*/
void idAASLocal::Test( const idVec3 &origin ) {

	if ( !file ) {
		return;
	}

	if ( aas_randomPullPlayer.GetBool() ) {
		RandomPullPlayer( origin );
	}
	if ( ( aas_pullPlayer.GetInteger() > 0 ) && ( aas_pullPlayer.GetInteger() < file->GetNumAreas() ) ) {
		ShowWalkPath( origin, aas_pullPlayer.GetInteger(), AreaCenter( aas_pullPlayer.GetInteger() ) );
		PullPlayer( origin, aas_pullPlayer.GetInteger() );
	}
	if ( ( aas_showPath.GetInteger() > 0 ) && ( aas_showPath.GetInteger() < file->GetNumAreas() ) ) {
		ShowWalkPath( origin, aas_showPath.GetInteger(), AreaCenter( aas_showPath.GetInteger() ) );
	}
	if ( ( aas_showFlyPath.GetInteger() > 0 ) && ( aas_showFlyPath.GetInteger() < file->GetNumAreas() ) ) {
		ShowFlyPath( origin, aas_showFlyPath.GetInteger(), AreaCenter( aas_showFlyPath.GetInteger() ) );
	}
	if ( ( aas_showHideArea.GetInteger() > 0 ) && ( aas_showHideArea.GetInteger() < file->GetNumAreas() ) ) {
		ShowHideArea( origin, aas_showHideArea.GetInteger() );
	}
// RAVEN BEGIN 
// rjohnson: added more debug drawing
	if ( aas_showAreas.GetInteger() == 1 ) {
		ShowArea( origin );
	}
	else if ( aas_showAreas.GetInteger() == 2 ) {
		ShowAreas( origin );
	}
	if ( aas_showProblemAreas.GetInteger() == 1 ) {
		ShowProblemArea( origin );
	}
	else if ( aas_showProblemAreas.GetInteger() == 2 ) {
		ShowAreas( origin, true );
	}
// RAVEN END
	if ( aas_showWallEdges.GetBool() ) {
		ShowWallEdges( origin );
	}
	if ( aas_showPushIntoArea.GetBool() ) {
		ShowPushIntoArea( origin );
	}
}
