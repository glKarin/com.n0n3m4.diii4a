// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"		// for cvars and debug drawing
#include "../../Player.h"
#include "../../Misc.h"
#include "../../ContentMask.h"
#include "../../botai/BotThreadData.h"

#include "AAS_local.h"
#include "AASCallback_FindCoverArea.h"
#include "AASCallback_FindFlaggedArea.h"
#include "ObstacleAvoidance.h"

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
void idAASLocal::DrawReachability( const aasReachability_t *reach, const char *name ) const {
	gameRenderWorld->DebugArrow( colorCyan, reach->GetStart(), reach->GetEnd(), 2, 0 );

	if ( name != NULL && name[0] != '\0' && gameLocal.GetLocalPlayer() != NULL ) {
		idStr flags;
		if ( reach->travelFlags & TFL_INVALID )			flags.Append( " invalid" );
		if ( reach->travelFlags & TFL_INVALID_GDF )		flags.Append( " invalidgdf" );
		if ( reach->travelFlags & TFL_INVALID_STROGG )	flags.Append( " invalidstrogg" );
		if ( reach->travelFlags & TFL_AIR )				flags.Append( " air" );
		if ( reach->travelFlags & TFL_WATER )			flags.Append( " water" );
		if ( reach->travelFlags & TFL_WALK )			flags.Append( " walk" );
		if ( reach->travelFlags & TFL_WALKOFFLEDGE )	flags.Append( " walkoffledge" );
		if ( reach->travelFlags & TFL_WALKOFFBARRIER )	flags.Append( " walloffbarrier" );
		if ( reach->travelFlags & TFL_BARRIERJUMP )		flags.Append( " barrierjump" );
		if ( reach->travelFlags & TFL_JUMP )			flags.Append( " jump" );
		if ( reach->travelFlags & TFL_LADDER )			flags.Append( " ladder" );
		if ( reach->travelFlags & TFL_SWIM )			flags.Append( " swim" );
		if ( reach->travelFlags & TFL_WATERJUMP )		flags.Append( " waterjump" );
		if ( reach->travelFlags & TFL_TELEPORT )		flags.Append( " teleport" );
		if ( reach->travelFlags & TFL_ELEVATOR )		flags.Append( " elevator" );
		gameRenderWorld->DrawText( va( "%s\nTravel Flags:%s", name, flags.c_str() ), ( reach->GetStart() + reach->GetEnd() ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->GetViewAxis(), 1, 0 );
	}
}

/*
============
idAASLocal::DrawEdge
============
*/
void idAASLocal::DrawEdge( int edgeNum, bool arrow ) const {
	const aasEdge_t *edge;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
	if ( arrow ) {
		gameRenderWorld->DebugArrow( colorRed, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ), 1, 0 );
	} else {
		gameRenderWorld->DebugLine( colorRed, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ), 0 );
	}

	if ( aas_showEdgeNums.GetBool() && gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", edgeNum ), ( file->GetVertex( edge->vertexNum[0] ) + file->GetVertex( edge->vertexNum[1] ) ) * 0.5f + idVec3(0,0,4), 0.1f, colorRed, gameLocal.GetLocalPlayer()->GetViewAxis(), 1, 0 );
	}
}

/*
============
idAASLocal::DrawArea
============
*/
void idAASLocal::DrawArea( int areaNum ) const {
	int i, j, numEdges, firstEdge;
	idVec3 mid, end;
	const aasArea_t *area;
	const aasReachability_t *reach;

	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );
	numEdges = area->numEdges;
	firstEdge = area->firstEdge;

	mid.Zero();
	for ( i = 0; i < numEdges; i++ ) {
		DrawEdge( abs( file->GetEdgeIndex( firstEdge + i ) ), true );
		j = file->GetEdgeIndex( firstEdge + i );
		mid += file->GetVertex( file->GetEdge( abs( j ) ).vertexNum[ j < 0 ] );
	}

	mid /= numEdges;
	end = mid + 5.0f * file->GetSettings().invGravityDir;
	gameRenderWorld->DebugArrow( colorGreen, mid, end, 1, 0 );

	for ( reach = area->reach; reach; reach = reach->next ) {
		DrawReachability( reach, NULL );
	}

	gameRenderWorld->DrawText( va( "%d", areaNum ), AreaCenter( areaNum ) + idVec3( 0.0f, 0.0f, 4.0f ), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->GetViewAxis(), 1, 0 );
}

/*
============
idAASLocal::DefaultSearchBounds
============
*/
const idBounds &idAASLocal::DefaultSearchBounds( void ) const {
	return file->GetSettings().boundingBox;
}

/*
============
idAASLocal::TravelFlagForTeam
============
*/
int idAASLocal::TravelFlagForTeam( void ) const {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL || player->GetGameTeam() == NULL ) {
		return TFL_VALID_GDF_AND_STROGG;
	}
	switch( player->GetGameTeam()->GetBotTeam() ) {
		case GDF: return TFL_VALID_GDF;
		case STROGG: return TFL_VALID_STROGG;
	}
	return TFL_VALID_GDF_AND_STROGG;
}

/*
============
idAASLocal::TravelFlagWalkForTeam
============
*/
int idAASLocal::TravelFlagWalkForTeam( void ) const {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL || player->GetGameTeam() == NULL ) {
		return TFL_VALID_WALK_GDF_AND_STROGG;
	}
	switch( player->GetGameTeam()->GetBotTeam() ) {
		case GDF: return TFL_VALID_WALK_GDF;
		case STROGG: return TFL_VALID_WALK_STROGG;
	}
	return TFL_VALID_WALK_GDF_AND_STROGG;
}

/*
============
idAASLocal::TravelFlagInvalidForTeam
============
*/
int idAASLocal::TravelFlagInvalidForTeam( void ) const {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL || player->GetGameTeam() == NULL ) {
		return TFL_INVALID|TFL_INVALID_GDF|TFL_INVALID_STROGG;
	}
	switch( player->GetGameTeam()->GetBotTeam() ) {
		case GDF: return TFL_INVALID|TFL_INVALID_GDF;
		case STROGG: return TFL_INVALID|TFL_INVALID_STROGG;
	}
	return TFL_INVALID|TFL_INVALID_GDF|TFL_INVALID_STROGG;
}

/*
============
idAASLocal::ShowArea
============
*/
void idAASLocal::ShowArea( const idVec3 &origin, int mode ) const {
	static int lastAreaNum;
	int areaNum;
	const aasArea_t *area;
	idVec3 org;

	org = origin;
	if ( mode == 1 ) {
		areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
		PushPointIntoArea( areaNum, org );
	} else {
		areaNum = PointAreaNum( origin );
	}

	if ( aas_showTravelTime.GetInteger() ) {
		int travelTime;
		const aasReachability_t *reach;
		
		RouteToGoalArea( areaNum, org, aas_showTravelTime.GetInteger(), TFL_WALK|TFL_AIR, travelTime, &reach );
		gameLocal.Printf( "\rtt = %4d", travelTime );
		if ( reach ) {
			gameLocal.Printf( " to area %4d", reach->toAreaNum );
			DrawArea( reach->toAreaNum );
		}
	}

	if ( areaNum != lastAreaNum ) {
		area = &file->GetArea( areaNum );
		gameLocal.Printf( "area %d:", areaNum );
		if ( area->flags & AAS_AREA_LEDGE ) {
			gameLocal.Printf( " ledge" );
		}
		if ( area->flags & AAS_AREA_CONTENTS_CLUSTERPORTAL ) {
			gameLocal.Printf( " clusterportal" );
		}
		if ( area->flags & AAS_AREA_CONTENTS_OBSTACLE ) {
			gameLocal.Printf( " obstacle" );
		}
		if ( area->flags & AAS_AREA_OUTSIDE ) {
			gameLocal.Printf( " outside" );
		}
		if ( area->flags & AAS_AREA_HIGH_CEILING ) {
			gameLocal.Printf( " highceiling" );
		}
		if ( area->travelFlags & ( TFL_INVALID | TFL_INVALID_GDF | TFL_INVALID_STROGG ) ) {
			gameLocal.Printf( " /" );
			if ( area->travelFlags & TFL_INVALID ) {
				gameLocal.Printf( " invalid" );
			}
			if ( area->travelFlags & TFL_INVALID_GDF ) {
				gameLocal.Printf( " invalidgdf" );
			}
			if ( area->travelFlags & TFL_INVALID_STROGG ) {
				gameLocal.Printf( " invalidstrogg" );
			}
		}
		gameLocal.Printf( "\n" );
		lastAreaNum = areaNum;
	}

	if ( org != origin ) {
		idBounds bnds = file->GetSettings().boundingBox;
		bnds[ 1 ].z = bnds[ 0 ].z;
		gameRenderWorld->DebugBounds( colorYellow, bnds, org );
		gameRenderWorld->DebugArrow( colorYellow, origin, org, 1 );
	}

	DrawArea( areaNum );
}

/*
============
idAASLocal::ShowWalkPath
============
*/
void idAASLocal::ShowWalkPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const {

	if ( file == NULL ) {
		return;
	}

	int curAreaNum = startAreaNum;

	idVec3 org = startOrigin;
	for ( int i = 0; i < 100; i++ ) {

		int travelTime;
		const aasReachability_t *reach;

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorGreen, org, reach->GetStart(), 2, 0 );
		DrawReachability( reach, NULL );
		if ( aas_showAreas.GetBool() ) {
			DrawArea( curAreaNum );
		}

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->GetEnd();
	}

	idAASPath path;
	if ( !WalkPathToGoal( path, startAreaNum, startOrigin, goalAreaNum, goalOrigin, travelFlags, walkTravelFlags ) ) {
		return;
	}

	gameRenderWorld->DebugArrow( colorBlue, startOrigin, path.moveGoal, 2, 0 );

//	idObstacleAvoidance::obstaclePath_t obstaclePath;
//	idObstacleAvoidance obstacleAvoidance;
//	botThreadData.BuildObstacleList( obstacleAvoidance, startOrigin, startAreaNum, false );
//	obstacleAvoidance.FindPathAroundObstacles( file->GetSettings().boundingBox, file->GetSettings().obstaclePVSRadius, this, startOrigin, path.moveGoal, obstaclePath );

//	if ( obstaclePath.firstObstacle != idObstacleAvoidance::OBSTACLE_ID_INVALID ) {
//		path.moveGoal = obstaclePath.seekPos;
//		gameRenderWorld->DebugArrow( colorOrange, startOrigin, path.moveGoal, 2, 0 );
//	}
}

/*
============
idAASLocal::ShowWalkPath
============
*/
void idAASLocal::ShowWalkPath( const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin ) const {

	if ( file == NULL ) {
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}

	idVec3 areaOrigin = startOrigin;
	int startAreaNum = PointReachableAreaNum( areaOrigin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	PushPointIntoArea( startAreaNum, areaOrigin );

	ShowWalkPath( startAreaNum, areaOrigin, goalAreaNum, goalOrigin, TravelFlagForTeam(), TravelFlagWalkForTeam() );
}

/*
============
idAASLocal::ShowHopPath
============
*/
void idAASLocal::ShowHopPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const {

	if ( file == NULL ) {
		return;
	}

	int curAreaNum = startAreaNum;

	idVec3 org = startOrigin;
	for ( int i = 0; i < 100; i++ ) {

		int travelTime;
		const aasReachability_t *reach;

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorGreen, org, reach->GetStart(), 2, 0 );
		DrawReachability( reach, NULL );
		if ( aas_showAreas.GetBool() ) {
			DrawArea( curAreaNum );
		}

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->GetEnd();
	}

	idAASPath path;
	if ( WalkPathToGoal( path, startAreaNum, startOrigin, goalAreaNum, goalOrigin, travelFlags, walkTravelFlags ) ) {
		gameRenderWorld->DebugArrow( colorBlue, startOrigin, path.moveGoal, 2 );
		if ( ExtendHopPathToGoal( path, startAreaNum, startOrigin, goalAreaNum, goalOrigin, travelFlags, walkTravelFlags, parms ) ) {
			gameRenderWorld->DebugArrow( colorCyan, startOrigin, path.moveGoal, 2 );
		}
	}
}

/*
============
idAASLocal::ShowHopPath
============
*/
void idAASLocal::ShowHopPath( const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin ) const {

	if ( file == NULL ) {
		return;
	}

	idVec3 areaOrigin = startOrigin;
	int startAreaNum = PointReachableAreaNum( areaOrigin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	PushPointIntoArea( startAreaNum, areaOrigin );

	ShowHopPath( startAreaNum, areaOrigin, goalAreaNum, goalOrigin, TravelFlagForTeam(), TravelFlagWalkForTeam(), idAASHopPathParms() );
}

/*
============
ProjectTopDown
============
*/
void ProjectTopDown( idVec3 &point, const idVec3 &viewOrigin, const idMat3 &viewAxis, const idMat3 &playerAxis, float distance ) {
	point = ( point - viewOrigin ) * playerAxis;
	point = viewOrigin + distance * viewAxis[0] + point.y * viewAxis[1] + point.x * viewAxis[2];
}

/*
============
idAASLocal::ShowWallEdges
============
*/
void idAASLocal::ShowWallEdges( const idVec3 &origin, int mode, bool showNumbers ) const {
	const int MAX_WALL_EDGES = 1024;
	int edges[MAX_WALL_EDGES];
	float textSize;

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}

	idMat3 viewAxis = player->GetViewAxis();
	idVec3 viewOrigin = player->GetViewPos();
	idMat3 playerAxis = idAngles( 0.0f, -player->GetViewAngles().yaw, 0.0f ).ToMat3();

	if ( mode == 3 ) {
		textSize = 0.2f;
	} else {
		textSize = 0.1f;
	}

	float radius = file->GetSettings().obstaclePVSRadius;

	int areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	//int numEdges = GetWallEdges( areaNum, idBounds( origin ).Expand( radius ), TFL_WALK, 64.0f, edges, MAX_WALL_EDGES );
	int numEdges = GetObstaclePVSWallEdges( areaNum, edges, MAX_WALL_EDGES );

	// move the wall edges to the start of the list
	int numWallEdges = 0;
	for ( int i = 0; i < numEdges; i++ ) {
		if ( ( file->GetEdge( abs( edges[i] ) ).flags & AAS_EDGE_WALL ) != 0 ) {
			idSwap( edges[numWallEdges++], edges[i] );
		}
	}

	for ( int i = 0; i < numEdges; i++ ) {
		idVec3 start, end;

		GetEdge( edges[i], start, end );

		if ( mode == 2 ) {

			start.z = end.z = origin.z;

		} else if ( mode == 3 ) {

			ProjectTopDown( start, viewOrigin, viewAxis, playerAxis, radius * 2.0f );
			ProjectTopDown( end, viewOrigin, viewAxis, playerAxis, radius * 2.0f );
		}

		if ( ( file->GetEdge( abs( edges[i] ) ).flags & AAS_EDGE_WALL ) != 0 ) {
			gameRenderWorld->DebugLine( colorRed, start, end, 0 );
		} else {
			gameRenderWorld->DebugLine( colorGreen, start, end, 0 );
		}
		if ( showNumbers ) {
			gameRenderWorld->DrawText( va( "%d", edges[i] ), ( start + end ) * 0.5f, textSize, colorWhite, viewAxis, 1, 0 );
		}
	}

	if ( mode == 3 ) {
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

		for ( int i = 0; i < 7; i++ ) {
			ProjectTopDown( box[i], viewOrigin, viewAxis, playerAxis, radius * 2.0f );
		}
		for ( int i = 0; i < 4; i++ ) {
			gameRenderWorld->DebugLine( colorCyan, box[i], box[(i+1)&3], 0 );
		}
		gameRenderWorld->DebugLine( colorCyan, box[4], box[5], 0 );
		gameRenderWorld->DebugLine( colorCyan, box[4], box[6], 0 );
	}
}

/*
============
idAASLocal::ShowNearestCoverArea
============
*/
void idAASLocal::ShowNearestCoverArea( const idVec3 &origin, int targetAreaNum ) const {
	int areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	idVec3 target = AreaCenter( targetAreaNum );

	DrawCone( target, idVec3(0,0,1), 16.0f, colorYellow );

	idAASCallback_FindCoverArea findCover( target );
	idAASGoal goal;
	if ( FindNearestGoal( goal, areaNum, origin, TravelFlagInvalidForTeam(), findCover ) ) {
		DrawArea( goal.areaNum );
		ShowWalkPath( areaNum, origin, goal.areaNum, goal.origin, TravelFlagForTeam(), TravelFlagWalkForTeam() );
		DrawCone( goal.origin, idVec3( 0, 0, 1 ), 16.0f, colorWhite );
	}
}

/*
============
idAASLocal::ShowNearestInsideArea
============
*/
void idAASLocal::ShowNearestInsideArea( const idVec3 &origin ) const {
	int areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );

	idAASCallback_FindFlaggedArea findInside( AAS_AREA_OUTSIDE, false );
	idAASGoal goal;
	if ( FindNearestGoal( goal, areaNum, origin, TravelFlagForTeam(), findInside ) ) {
		DrawArea( goal.areaNum );
		ShowWalkPath( areaNum, origin, goal.areaNum, goal.origin, TravelFlagForTeam(), TravelFlagWalkForTeam() );
		DrawCone( goal.origin, idVec3( 0, 0, 1 ), 16.0f, colorWhite );
	}
}

struct idPullPlayerState {
	idPullPlayerState() {
		jumpNow = false;
		ladderDir = vec3_zero;
		ladderTime = 0;
	}

	bool		jumpNow;
	idVec3		ladderDir;
	int			ladderTime;
} pullPlayerState;

/*
============
idAASLocal::PullPlayer
============
*/
bool idAASLocal::PullPlayer( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int &startAreaNum, int &travelTime ) const {

	startAreaNum = 0;
	travelTime = 0;

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return true;
	}

	if ( goalAreaNum == 0 ) {
		return false;
	}

	if ( player->GetNoClip() ) {
		player->aasPullPlayer = false;
		return false;
	}

	idVec3 dir = goalOrigin - origin;
	float height = idMath::Fabs( dir.z );
	float dist = dir.ToVec2().Length();

	if ( dist < 32.0f && height < 128.0f ) {
		return false;
	}

	idVec3 org = origin;
	startAreaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	PushPointIntoArea( startAreaNum, org );

	const aasReachability_t *reach;
	RouteToGoalArea( startAreaNum, org, goalAreaNum, TravelFlagForTeam(), travelTime, &reach );

	ShowWalkPath( startAreaNum, org, goalAreaNum, goalOrigin, TravelFlagForTeam(), TravelFlagWalkForTeam() );

	idAASPath path;
	if ( !WalkPathToGoal( path, startAreaNum, org, goalAreaNum, goalOrigin, TravelFlagForTeam(), TravelFlagWalkForTeam() ) ) {
		return false;
	}

	idObstacleAvoidance::obstaclePath_t obstaclePath;
	idObstacleAvoidance obstacleAvoidance;
	botThreadData.BuildObstacleList( obstacleAvoidance, org, startAreaNum, false );
	obstacleAvoidance.FindPathAroundObstacles( file->GetSettings().boundingBox, file->GetSettings().obstaclePVSRadius, this, org, path.moveGoal, obstaclePath );
	path.moveGoal = obstaclePath.seekPos;

	player->aasPullPlayer = true;

	usercmd_t usercmd;
	memset( &usercmd, 0, sizeof( usercmd ) );

	usercmd.forwardmove = 127;
	usercmd.buttons.btn.run = true;
	usercmd.buttons.btn.sprint = false;

	idVec3 moveDir = path.moveGoal - org;
	idVec3 horizontalDir( moveDir.x, moveDir.y, 0.0f );
	float horizontalDist = horizontalDir.Normalize();
	moveDir.Normalize();

	switch( path.type ) {
		case PATHTYPE_WALKOFFLEDGE:
		case PATHTYPE_WALKOFFBARRIER: {
			if ( horizontalDist < 80.0f ) {
				usercmd.buttons.btn.run = false;
				usercmd.forwardmove = 16 + horizontalDist;
			}
			break;
		}
		case PATHTYPE_BARRIERJUMP:
		case PATHTYPE_JUMP: {
			if ( horizontalDist < 24.0f ) {
				pullPlayerState.jumpNow = !pullPlayerState.jumpNow;
				if ( pullPlayerState.jumpNow ) {
					usercmd.upmove = 127;
				}
			}
			if ( player->GetPlayerPhysics().IsGrounded() ) {
				if ( horizontalDist < 100.0f ) {
					usercmd.buttons.btn.run = false;
					usercmd.forwardmove = 8 + horizontalDist;
				}
			} else {
				moveDir = path.reachability->GetEnd() - org;
				moveDir.Normalize();
			}
			break;
		}
		case PATHTYPE_LADDER: {
			pullPlayerState.ladderDir = moveDir;
			pullPlayerState.ladderTime = gameLocal.time;
#if 0
			// test ladder physics code
			if ( !player->GetPlayerPhysics().IsGrounded() ) {
				trace_t trace;
				bool onLadder = false;
				if ( gameLocal.clip.Translation( CLIP_DEBUG_PARMS_CLIENTINFO( self ) trace, org, org + horizontalDir * 32.0f, NULL, mat3_identity, MASK_PLAYERSOLID, player ) ) {
					onLadder = ( gameLocal.entities[ trace.c.entityNum ]->Cast< sdLadderEntity >() != NULL );
				}
				if ( onLadder && !player->GetPlayerPhysics().OnLadder() ) {
					usercmd.forwardmove = 0;
				}
			}
#endif
			break;
		}
	}

	if ( player->GetPlayerPhysics().OnLadder() || pullPlayerState.ladderTime > gameLocal.time - 500 ) {
		moveDir = pullPlayerState.ladderDir;
	}

	idAngles viewAngles = moveDir.ToAngles();

	player->Move( usercmd, viewAngles );

	return true;
}

/*
============
idAASLocal::UnFreezePlayer
============
*/
void idAASLocal::UnFreezePlayer() const {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}
	player->aasPullPlayer = false;
}

/*
============
idAASLocal::RandomPullPlayer
============
*/
void idAASLocal::RandomPullPlayer( const idVec3 &origin, int mode ) const {
	int areaNum, travelTime;
	static int bestTravelTime = 0;
	static int startAreaNum = 0;
	static int goalAreaNum = 0;
	static int failedCount;

	if ( goalAreaNum > file->GetNumAreas() ) {
		goalAreaNum = 0;
	}

	if ( !PullPlayer( origin, goalAreaNum, AreaCenter( goalAreaNum ), areaNum, travelTime ) ) {

		startAreaNum = 0;
		bestTravelTime = INT_MAX;

		int rnd = idMath::Ftoi( gameLocal.random.RandomFloat() * file->GetNumAreas() );

		for ( int i = 0; i < file->GetNumAreas(); i++ ) {
			int n = ( rnd + i ) % file->GetNumAreas();
			if ( ( file->GetArea( n ).flags & AAS_AREA_REACHABLE_WALK ) != 0 ) {
				goalAreaNum = n;
			}
		}
	} else if ( startAreaNum == 0 ) {
		startAreaNum = areaNum;
		bestTravelTime = travelTime;
	}

	if ( travelTime < bestTravelTime ) {
		bestTravelTime = travelTime;
		failedCount = 0;
	} else {
		failedCount++;
	}

	if ( failedCount > 10 * 1000 / USERCMD_MSEC ) {
		failedCount = 0;
		if ( mode > 1 ) {
			common->Warning( "failed to go from area %d to area %d at area %d", startAreaNum, goalAreaNum, areaNum );
			goalAreaNum = 0;
		}
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
	areaNum = PointReachableAreaNum( target, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	PushPointIntoArea( areaNum, target );
	gameRenderWorld->DebugArrow( colorGreen, origin, target, 1, 0 );
}

/*
============
idAASLocal::ShowFloorTrace
============
*/
void idAASLocal::ShowFloorTrace( const idVec3 &origin ) const {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}
	idMat3 playerAxis = idAngles( 0.0f, player->GetViewAngles().yaw, 0.0f ).ToMat3();

	idVec3 org = origin;
	int areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
	PushPointIntoArea( areaNum, org );

	aasTraceFloor_t trace;
	TraceFloor( trace, org, areaNum, origin + playerAxis[0] * 1024, TFL_WALK|TFL_AIR );

	gameRenderWorld->DebugArrow( colorCyan, org, trace.endpos, 1, 0 );
	idVec3 up = trace.endpos + playerAxis[2] * 64;
	const idVec4 &color = ( trace.fraction >= 1.0f ) ? colorGreen : colorRed;
	gameRenderWorld->DebugArrow( color, trace.endpos, up + playerAxis[1] * 32, 1, 0, true );
	gameRenderWorld->DebugArrow( color, trace.endpos, up - playerAxis[1] * 32, 1, 0, true );
}

/*
============
idAASLocal::ShowObstaclePVS
============
*/
void idAASLocal::ShowObstaclePVS( const int areaNum ) const {
	const byte *pvs = GetObstaclePVS( areaNum );

	for ( int i = 0; i < file->GetNumAreas(); i++ ) {
		if ( !IsInObstaclePVS( pvs, i ) ) {
			continue;
		}
		DrawArea( i );
	}
}

/*
============
idAASLocal::ShowManualReachabilities
============
*/
void idAASLocal::ShowManualReachabilities() const {
	for ( int i = 0; i < file->GetNumReachabilityNames(); i++ ) {
		const aasName_t &name = file->GetReachabilityName( i );
		int index = file->FindReachabilityByName( name.name );
		if ( index >= 0 ) {
			DrawReachability( &file->GetReachability( index ), name.name );
		}
	}
}

/*
==================
idAASLocal::ShowAASObstacles
==================
*/
void idAASLocal::ShowAASObstacles() const {

	idPlayer *player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	idMat3 viewAxis = player->GetViewAxis();

	for( int i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {

		idEntity* ent = gameLocal.entities[ i ];

		if ( ent == NULL ) {
			continue;
		}

		idAASObstacleEntity* obstacle = ent->Cast< idAASObstacleEntity >();

		if ( obstacle == NULL ) {
			continue;
		}

		idVec4 colorType = colorGreen;

		if ( !obstacle->IsEnabled() ) {
			colorType = colorRed;
		}

		idVec3 org = obstacle->GetPhysics()->GetAbsBounds().GetCenter();

		gameRenderWorld->DebugBounds( colorType, obstacle->GetPhysics()->GetAbsBounds() );
		gameRenderWorld->DrawText( va( "Team: %i", obstacle->GetTeam() ), org, 0.20f, colorWhite, viewAxis );
	}
}

/*
============
idAASLocal::ShowAASBadAreas
============
*/
void idAASLocal::ShowAASBadAreas( int mode ) const {

	if ( file == NULL ) {
		return;
	}

	float height = file->GetSettings().boundingBox[1][2] - file->GetSettings().boundingBox[0][2];

	for ( int i = 0; i < file->GetNumAreas(); i++ ) {
		const aasArea_t &area = file->GetArea( i );

		idVec3 mid;
		mid.Zero();

		for ( int j = 0; j < area.numEdges; j++ ) {
			int edgeNum = file->GetEdgeIndex( area.firstEdge + j );
			const aasEdge_t &edge = file->GetEdge( abs( edgeNum ) );
			mid += file->GetVertex( edge.vertexNum[ INT32_SIGNBITSET( edgeNum ) ] );
		}

		mid /= area.numEdges;

		bool bad = false;

		if ( mode == 1 || mode == 3 ) {
			int j;
			for ( j = 0; j < 4; j++ ) {
				int areaNum = file->PointAreaNum( mid + file->GetSettings().invGravityDir * ( j * height )  );
				if ( areaNum == i ) {
					break;
				}
			}
			if ( j >= 4 ) {
				bad = true;
			}
		}

		if ( mode == 2 || mode == 3 ) {
#if 1
			if ( ( area.flags & AAS_AREA_NOPUSH ) != 0 ) {
				bad = true;
			}
#else
			idVec3 pushed = mid;
			if ( file->PushPointIntoArea( i, pushed ) ) {
				bad = true;
			}
#endif
		}

		if ( bad ) {
			DrawArea( i );
		}
	}
}

/*
============
idAASLocal::GetAreaNumAndLocation
============
*/
bool idAASLocal::GetAreaNumAndLocation( idCVar &cvar, const idVec3 &origin, int &areaNum, idVec3 &location ) const {

	areaNum = 0;
	location.Zero();

	if ( cvar.GetString()[0] == '\0' ) {
		return false;
	}

	if ( idStr::Icmp( cvar.GetString(), "memory" ) == 0 ) {
		cvar.SetString( aas_locationMemory.GetString() );
	}

	if ( idStr::Icmp( cvar.GetString(), "current" ) == 0 ) {
		cvar.SetString( origin.ToString() );
	}

	idLexer src( LEXFL_NOERRORS|LEXFL_NOWARNINGS );
	src.LoadMemory( cvar.GetString(), idStr::Length( cvar.GetString() ), "areaNum" );

	bool error = false;
	location.x = src.ParseFloat( &error );
	location.y = src.ParseFloat( &error );
	location.z = src.ParseFloat( &error );

	if ( !error ) {
		areaNum = PointReachableAreaNum( location, DefaultSearchBounds(), AAS_AREA_REACHABLE_WALK, TravelFlagInvalidForTeam() );
		PushPointIntoArea( areaNum, location );
		return true;
	}

	src.Reset();

	areaNum = src.ParseInt();

	if ( ( areaNum > 0 ) && ( areaNum < file->GetNumAreas() ) ) {
		location = AreaCenter( areaNum );
		return true;
	}

	return false;
}

/*
============
idAASLocal::Test
============
*/
void idAASLocal::Test( const idVec3 &origin ) {
	int areaNum;
	idVec3 location;

	if ( file == NULL ) {
		return;
	}

	if ( GetAreaNumAndLocation( aas_locationMemory, origin, areaNum, location ) ) {
	}
	if ( GetAreaNumAndLocation( aas_showPath, origin, areaNum, location ) ) {
		ShowWalkPath( origin, areaNum, location );
	}
	if ( GetAreaNumAndLocation( aas_showHopPath, origin, areaNum, location ) ) {
		ShowHopPath( origin, areaNum, location );
	}
	if ( GetAreaNumAndLocation( aas_pullPlayer, origin, areaNum, location ) ) {
		int startAreaNum, travelTime;
		ShowWalkPath( origin, areaNum, location );
		PullPlayer( origin, areaNum, location, startAreaNum, travelTime );
	} else {
		UnFreezePlayer();
	}
	if ( aas_randomPullPlayer.GetInteger() != 0 ) {
		RandomPullPlayer( origin, aas_randomPullPlayer.GetInteger() );
	}
	if ( aas_showAreas.GetInteger() != 0 ) {
		ShowArea( origin, aas_showAreas.GetInteger() );
	}

	if ( aas_showAreaNumber.GetInteger() != 0 ) {
		DrawArea( aas_showAreaNumber.GetInteger() );
	}

	if ( ( aas_showNearestCoverArea.GetInteger() > 0 ) && ( aas_showNearestCoverArea.GetInteger() < file->GetNumAreas() ) ) {
		ShowNearestCoverArea( origin, aas_showNearestCoverArea.GetInteger() );
	}
	if ( aas_showNearestInsideArea.GetBool() ) {
		ShowNearestInsideArea( origin );
	}
	if ( aas_showWallEdges.GetInteger() != 0 ) {
		ShowWallEdges( origin, aas_showWallEdges.GetInteger(), aas_showWallEdgeNums.GetBool() );
	}
	if ( aas_showPushIntoArea.GetBool() ) {
		ShowPushIntoArea( origin );
	}
	if ( aas_showFloorTrace.GetBool() ) {
		ShowFloorTrace( origin );
	}
	if ( GetAreaNumAndLocation( aas_showObstaclePVS, origin, areaNum, location ) ) {
		ShowObstaclePVS( areaNum );
	}
	if ( aas_showManualReachabilities.GetBool() ) {
		ShowManualReachabilities();
	}
	if ( aas_showFuncObstacles.GetBool() ) {
		ShowAASObstacles();
	}
	if ( aas_showBadAreas.GetInteger() != 0 ) {
		ShowAASBadAreas( aas_showBadAreas.GetInteger() );
	}
}
