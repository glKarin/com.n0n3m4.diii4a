/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
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
void idAASLocal::DrawReachability( const idReachability *reach ) const 
{
	idVec4 reachColor = colorCyan;
	if (reach->travelType & TFL_DOOR)
	{
		reachColor = colorRed;
	}
	gameRenderWorld->DebugArrow( reachColor, reach->start, reach->end, 1, 10000 );

	gameRenderWorld->DebugArrow( colorLtGrey, AreaCenter(reach->fromAreaNum), AreaCenter(reach->toAreaNum), 1, 10000);

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DebugText( va( "%d", reach->edgeNum ), ( reach->start + reach->end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis, 1, 10000 );
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
		gameRenderWorld->DebugText( va( "%d", edgeNum ), ( file->GetVertex( edge->vertexNum[0] ) + file->GetVertex( edge->vertexNum[1] ) ) * 0.5f + idVec3(0,0,4), 0.1f, colorRed, gameLocal.GetLocalPlayer()->viewAxis );
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
		DrawFace( abs( file->GetFaceIndex( firstFace + i ) ), file->GetFaceIndex( firstFace + i ) < 0 );
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
		
		RouteToGoalArea( areaNum, org, aas_goalArea.GetInteger(), TFL_WALK|TFL_AIR, travelTime, &reach, NULL, NULL );
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
		if (area->flags & AREA_DOOR)
		{
			gameLocal.Printf( "AREA_DOOR " );
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
void idAASLocal::ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) {
	int i, areaNum, curAreaNum, travelTime;
	idReachability *reach;
	idVec3 org;
	aasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_WALK );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_AIR, travelTime, &reach, NULL, NULL ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorGreen, org, reach->start, 1, 10000 );
		DrawReachability( reach );

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->end;
	}

	if ( WalkPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_AIR, travelTime, NULL ) ) // grayman #3548
	{
		gameRenderWorld->DebugArrow( colorBlue, origin, path.moveGoal, 1, 10000 );
	}
}

/*
============
idAASLocal::ShowFlyPath
============
*/
void idAASLocal::ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const // grayman #4412
{
	int i, areaNum, curAreaNum, travelTime;
	idReachability *reach;
	idVec3 org;
	aasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_FLY );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_FLY|TFL_AIR, travelTime, &reach, NULL, NULL ) ) {
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

	if ( FlyPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_FLY|TFL_AIR, actor ) ) // grayman #4412
	{
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
		gameRenderWorld->DebugText( va( "%d", edges[i] ), ( start + end ) * 0.5f, 0.1f, colorWhite, player->viewAxis );
	}
}

/*
============
idAASLocal::ShowHideArea
============
*/
void idAASLocal::ShowHideArea( const idVec3 &origin, int targetAreaNum ) {
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

	idAASFindCover findCover( NULL, NULL, target );
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
bool idAASLocal::PullPlayer( const idVec3 &origin, int toAreaNum ) {
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

	int travelTime; // grayman #3548
	if ( WalkPathToGoal( path, areaNum, origin, toAreaNum, areaCenter, TFL_WALK|TFL_AIR, travelTime, NULL ) ) // grayman #3548
	{
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
void idAASLocal::RandomPullPlayer( const idVec3 &origin ) {
	int rnd, i, n;

	if ( !PullPlayer( origin, aas_pullPlayer.GetInteger() ) ) {

		rnd = gameLocal.random.RandomInt(file->GetNumAreas());

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
		ShowFlyPath( origin, aas_showFlyPath.GetInteger(), AreaCenter( aas_showFlyPath.GetInteger() ), NULL ); // grayman #4412
	}
	if ( ( aas_showHideArea.GetInteger() > 0 ) && ( aas_showHideArea.GetInteger() < file->GetNumAreas() ) ) {
		ShowHideArea( origin, aas_showHideArea.GetInteger() );
	}
	if ( aas_showAreas.GetBool() ) {
		ShowArea( origin );
	}
	if ( aas_showWallEdges.GetBool() ) {
		ShowWallEdges( origin );
	}
	if ( aas_showPushIntoArea.GetBool() ) {
		ShowPushIntoArea( origin );
	}
}

// grayman #3032 - changed from this:

// Hit the "_impulse27" key and paint AAS areas and cluster numbers
// on the screen for 1 second.

// to this:

// "_impulse27" is a toggle. When on, paint AAS areas and cluster
// numbers on the screen each frame. Now it can be left on, painting
// the local information as the player moves around. Toggle off
// when no longer needed. The colors are assigned the first time it
// toggles on, and are kept for the length of the session. They are
// not saved across savegames because they can just be redetermined
// after a restore.

void idAASLocal::DrawAreas(const idVec3& playerOrigin)
{
	if ( file == NULL )
	{
		return;
	}

	// Initialize AAS area colors if we haven't already done so

	if ( aasColors.Num() == 0 )
	{
		// Get a color for each cluster

		int numClusters = file->GetNumClusters();

		for ( int c = 0 ; c < numClusters ; c++ )
		{
			idVec4 &v = aasColors.Alloc();
			v.x = gameLocal.random.RandomFloat() + 0.1f;
			v.y = gameLocal.random.RandomFloat() + 0.1f;
			v.z = gameLocal.random.RandomFloat() + 0.1f;
			v.w = 1.0f;
		}
	}

	idMat3 playerViewMatrix(gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	
	idList<int> clusterNums;

	// Paint the AAS areas

	for ( int i = 0 ; i < file->GetNumAreas() ; i++ )
	{
		idVec3 areaCenter = AreaCenter(i);

		// angua: only draw areas near the player, no need to see them at the other end of the map
		if ( (areaCenter - playerOrigin).LengthFast() < 150 )
		{
			idBounds areaBounds = GetAreaBounds(i);
			int clusterNum = file->GetArea(i).cluster;
			clusterNums.AddUnique(clusterNum);
			idVec4 color = (clusterNum <= 0) ? colorWhite : aasColors[clusterNum];

			gameRenderWorld->DebugText(va("%d", i), areaCenter, 0.2f, color, playerViewMatrix, 1, 16);
			gameRenderWorld->DebugBox(color, idBox(areaBounds), 16);
		}
	}

	// Paint the cluster numbers

	for ( int i = 0 ; i < clusterNums.Num() ; i++ )
	{
		int area = GetAreaInCluster(clusterNums[i]);
		if ( area <= 0 )
		{
			continue;
		}

		idVec3 origin = file->GetArea(area).center;
		if ( (origin - playerOrigin).LengthFast() < 300 )
		{
			gameRenderWorld->DebugText(va("%d", clusterNums[i]), origin, 1, colorRed, playerViewMatrix, 1, 16);
		}
	}
}


void idAASLocal::DrawEASRoute( const idVec3& playerOrigin, int goalArea )
{
	idVec3 origin = playerOrigin;
	int areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );

	PushPointIntoAreaNum( areaNum, origin );

	elevatorSystem->DrawRoute(areaNum, goalArea);
}
