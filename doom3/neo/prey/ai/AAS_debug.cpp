// Copyright (C) 2004 Id Software, Inc.
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "AAS_local.h"
#include "../Game_local.h"		// for cvars and debug drawing


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
		p = center + sin( DEG2RAD(i) ) * radius * axis[0] + cos( DEG2RAD(i) ) * radius * axis[1];
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
#ifdef HUMANHEAD
	idVec3 dir = (reach->end - reach->start).ToNormal() * 10;
	gameRenderWorld->DebugArrow( colorCyan, reach->start - dir, reach->end + dir, 2 );
	gameRenderWorld->DrawText( va( "%d", reach->toAreaNum ), ( reach->start + reach->end ) * 0.5f, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
#else
	gameRenderWorld->DebugArrow( colorCyan, reach->start, reach->end, 2 );

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", reach->edgeNum ), ( reach->start + reach->end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis );
	}
#endif

	switch( reach->travelType ) {
		case TFL_WALK: {
			const idReachability_Walk *walk = static_cast<const idReachability_Walk *>(reach);
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
// HUMANHEAD nla - Added color parameterv
void idAASLocal::DrawEdge( int edgeNum, bool arrow, idVec4 *color ) const {
	const aasEdge_t *edge;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
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
// HUMANHEAD nla - Added color parameter
void idAASLocal::DrawFace( int faceNum, bool side, idVec4 *color ) const {
	int i, j, numEdges, firstEdge;
	const aasFace_t *face;
	idVec3 mid, end;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	mid = vec3_origin;
	for ( i = 0; i < numEdges; i++ ) {
		DrawEdge( abs( file->GetEdgeIndex( firstEdge + i ) ), ( face->flags & FACE_FLOOR ) != 0, color );
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
	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for ( i = 0; i < numFaces; i++ ) {
		// HUMANHEAD nla - added color parameter
		DrawFace( abs( file->GetFaceIndex( firstFace + i ) ), file->GetFaceIndex( firstFace + i ) < 0,
			&colorRed );
	}

	for ( reach = area->reach; reach; reach = reach->next ) {
		DrawReachability( reach );
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

	idAASFindCover findCover( target );
	if ( FindNearestGoal( goal, areaNum, origin, target, TFL_WALK|TFL_AIR, obstacles, numObstacles, findCover ) ) {
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
idAASLocal::DrawBounds
============
*/
// HUMANHEAD nla
void idAASLocal::DrawBounds( int areaNum ) const {
	const aasArea_t	*area;
	const idBounds	*bounds;
	const idVec3		*p0, *p1;

	area = &file->GetArea(areaNum);
	if (area->numFaces < -1) {	// Not a valid area
		return;
	}
	
	bounds = &area->bounds;
	
	for (int i = 0; i < 2; ++i) {
		p0 = &(*bounds)[i];
		p1 = &(*bounds)[(i + 1) % 2];
		for (int j = 0; j < 3; ++j) {
			DrawBoundsEdge(*p0, *p1, j, 0);
		}
	}
}


/*
============
idAASLocal::DrawBoundsEdge
============
*/
// HUMANHEAD nla
void idAASLocal::DrawBoundsEdge( const idVec3 &p0, const idVec3 &ip1, int keep, int draw ) const {
	idVec3 p1((*(keep == 0 ? &p0 : &ip1))[0],
			  (*(keep == 1 ? &p0 : &ip1))[1], 
			  (*(keep == 2 ? &p0 : &ip1))[2]);


	if (draw) {
		gameRenderWorld->DebugLine( colorWhite, p0, p1);
	}
	else {
		for (int i = 1; i < 3; ++i) {
			DrawBoundsEdge(p0, p1, (keep + i) % 3, true);
		}
	}
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
	if ( ai_debugPath.GetBool() ) {
		int i, areaNum, numEdges, edges[1024];
		idVec3 start, end;
		if ( gameLocal.GetLocalPlayer() ) {
			areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
			numEdges = GetWallEdges( areaNum, idBounds( origin ).Expand( 256.0f ), TFL_WALK, edges, 1024 );
			for ( i = 0; i < numEdges; i++ ) {
				GetEdge( edges[i], start, end );
				gameRenderWorld->DebugLine( colorWhite, start, end );
			}
		}
	}
	if ( aas_showAreas.GetBool() ) {
		//HUMANHEAD nla		
		if (aas_showAreas.GetInteger() > 2) {
			int counter;

			for (counter = 0; counter < file->GetNumAreas(); counter++) {
				if (aas_showAreas.GetInteger() > 3) {
					DrawBounds(counter);
				}
				else {
					DrawArea(counter);
				}
			}
		}
		else {
			ShowArea( origin );
		}
	}
	if ( aas_showWallEdges.GetBool() ) {
		ShowWallEdges( origin );
	}
	if ( aas_showPushIntoArea.GetBool() ) {
		ShowPushIntoArea( origin );
	}
}
