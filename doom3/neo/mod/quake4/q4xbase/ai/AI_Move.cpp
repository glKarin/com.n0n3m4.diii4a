/*
===============================================================================

AI_Move.cpp

This file has all movement related functions.  It and its sister H file were 
split from AI.h and AI.cpp in order to prevent merge conflicts and to make 
further changes to the system possible.

===============================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AAS_Find.h"

/*
===============================================================================

	idMoveState

===============================================================================
*/

/*
=====================
idMoveState::idMoveState
=====================
*/
idMoveState::idMoveState() {
	moveType			= MOVETYPE_ANIM;
	moveCommand			= MOVE_NONE;
	moveStatus			= MOVE_STATUS_DONE;
	moveDest.Zero();
	moveDir.Set( 1.0f, 0.0f, 0.0f );
	goalEntity			= NULL;
	goalEntityOrigin.Zero();
	toAreaNum			= 0;
	startTime			= 0;
	duration			= 0;
	speed				= 0.0f;
	range				= 0.0f;
	wanderYaw			= 0;
	nextWanderTime		= 0;
	blockTime			= 0;
	obstacle			= NULL;
	lastMoveOrigin		= vec3_origin;
	lastMoveTime		= 0;
	anim				= 0;
	travelFlags			= TFL_WALK|TFL_AIR;
	kickForce			= 2048.0f;
	fl.ignoreObstacles	= false;
	fl.allowAnimMove	= false;
	fl.allowPrevAnimMove = false;
	fl.allowHiddenMove	= false;
	blockedRadius		= 0.0f;
	blockedMoveTime		= 750;
	blockedAttackTime	= 750;
	turnRate			= 360.0f;
	turnVel				= 0.0f;
	anim_turn_yaw		= 0.0f;
	anim_turn_amount	= 0.0f;
	anim_turn_angles	= 0.0f;
	fly_offset			= 0;
	fly_seek_scale		= 1.0f;
	fly_roll_scale		= 0.0f;
	fly_roll_max		= 0.0f;
	fly_roll			= 0.0f;
	fly_pitch_scale		= 0.0f;
	fly_pitch_max		= 0.0f;
	fly_pitch			= 0.0f;
	fly_speed			= 0.0f;
	fly_bob_strength	= 0.0f;
	fly_bob_vert		= 0.0f;
	fly_bob_horz		= 0.0f;
	currentDirection	= MOVEDIR_FORWARD;
	idealDirection		= MOVEDIR_FORWARD;		
	current_yaw			= 0.0f;
	ideal_yaw			= 0.0f;
	flyTiltJoint		= INVALID_JOINT;
	addVelocity.Zero();
}



/*
=====================
idMoveState::Spawn
=====================
*/
void idMoveState::Spawn( idDict &spawnArgs ) {
	memset ( &fl, 0, sizeof(fl) );
	fl.allowAnimMove		= true;
	fl.allowPrevAnimMove	= false;
	fl.allowHiddenMove		= false;
	fl.noRun				= spawnArgs.GetBool ( "forceWalk", "0" );
	fl.noWalk				= spawnArgs.GetBool ( "forceRun", "0" );
	fl.noTurn				= spawnArgs.GetBool ( "noTurn", "0" );
	fl.noGravity			= spawnArgs.GetBool ( "animate_z", "0" );
	fl.noRangedInterrupt	= spawnArgs.GetBool ( "noRangedInterrupt", "0" );

	fl.flyTurning			= spawnArgs.GetBool ( "flyTurning", "0" );
	fl.allowDirectional		= spawnArgs.GetBool ( "directionalMovement", "0" );
	fl.allowPushMovables	= spawnArgs.GetBool( "af_push_moveables", "0" );
	fl.ignoreObstacles		= spawnArgs.GetBool( "ignore_obstacles", "0" );
	fl.disabled				= spawnArgs.GetBool ( "noMove", "0" );
	walkRange				= spawnArgs.GetFloat ( "walkRange", "100" );
	walkTurn				= spawnArgs.GetFloat ( "walkTurn", "45" );
	followRange				= spawnArgs.GetVec2 ( "followRange", "70 250" ); 
	searchRange				= spawnArgs.GetVec2 ( "searchRange", "0 1024" ); 
	attackPositionRange		= spawnArgs.GetFloat ( "attackPositionRange", "0" ); 
	turnDelta				= spawnArgs.GetFloat ( "turnDelta", "10" );
	blockTime				= 0;

	spawnArgs.GetInt(	"fly_offset",			"100",		fly_offset );
	spawnArgs.GetFloat( "fly_speed",			"100",		fly_speed );
	spawnArgs.GetFloat( "fly_bob_strength",		"50",		fly_bob_strength );
	spawnArgs.GetFloat( "fly_bob_vert",			"2",		fly_bob_horz );
	spawnArgs.GetFloat( "fly_bob_horz",			"2.7",		fly_bob_vert );
	spawnArgs.GetFloat( "fly_seek_scale",		"4",		fly_seek_scale );
	spawnArgs.GetFloat( "fly_roll_scale",		"90",		fly_roll_scale );
	spawnArgs.GetFloat( "fly_roll_max",			"60",		fly_roll_max );
	spawnArgs.GetFloat( "fly_pitch_scale",		"45",		fly_pitch_scale );
	spawnArgs.GetFloat( "fly_pitch_max",		"30",		fly_pitch_max );

	spawnArgs.GetFloat( "turn_rate",			"360",		turnRate );
	spawnArgs.GetFloat( "kick_force",			"4096",		kickForce );
	spawnArgs.GetFloat( "blockedRadius",		"-1",		blockedRadius );
	spawnArgs.GetInt( "blockedMoveTime",		"750",		blockedMoveTime );
	spawnArgs.GetInt( "blockedAttackTime",		"750",		blockedAttackTime );
}


/*
=====================
idMoveState::Save
=====================
*/
void idMoveState::Save( idSaveGame *savefile ) const {
	int i;

	savefile->Write ( &fl, sizeof(fl) );

	savefile->WriteInt( (int)moveType );
	savefile->WriteInt( (int)moveCommand );
	savefile->WriteInt( (int)moveStatus );
	savefile->WriteVec3( moveDest );
	savefile->WriteVec3( moveDir );
	
	savefile->WriteInt( toAreaNum );
	savefile->WriteInt( startTime );
	savefile->WriteInt( duration );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( range );
	savefile->WriteFloat( wanderYaw );
	savefile->WriteInt( nextWanderTime );
	savefile->WriteInt( blockTime );
	obstacle.Save( savefile );
	savefile->WriteVec3( lastMoveOrigin );
	savefile->WriteInt( lastMoveTime );
	savefile->WriteInt( anim );

	savefile->WriteInt( travelFlags );

	savefile->WriteFloat ( kickForce );
	savefile->WriteFloat ( blockedRadius );
	savefile->WriteInt ( blockedMoveTime );
	savefile->WriteInt ( blockedAttackTime );

	savefile->WriteFloat( ideal_yaw );
	savefile->WriteFloat( current_yaw );
	savefile->WriteFloat( turnRate );
	savefile->WriteFloat( turnVel );
	savefile->WriteFloat( anim_turn_yaw );
	savefile->WriteFloat( anim_turn_amount );
	savefile->WriteFloat( anim_turn_angles );

	savefile->WriteJoint( flyTiltJoint );
	savefile->WriteFloat( fly_speed );
	savefile->WriteFloat( fly_bob_strength );
	savefile->WriteFloat( fly_bob_vert );
	savefile->WriteFloat( fly_bob_horz );
	savefile->WriteInt( fly_offset );
	savefile->WriteFloat( fly_seek_scale );
	savefile->WriteFloat( fly_roll_scale );
	savefile->WriteFloat( fly_roll_max );
	savefile->WriteFloat( fly_roll );
	savefile->WriteFloat( fly_pitch_scale );
	savefile->WriteFloat( fly_pitch_max );
	savefile->WriteFloat( fly_pitch );

	savefile->WriteInt ( (int) currentDirection );
	savefile->WriteInt ( (int) idealDirection );
	savefile->WriteFloat( walkRange );
	savefile->WriteFloat( walkTurn );
	savefile->WriteVec2 ( followRange );
	savefile->WriteVec2 ( searchRange );
	savefile->WriteFloat( attackPositionRange );
	savefile->WriteFloat( turnDelta );

	savefile->WriteVec3 ( goalPos );
	savefile->WriteInt ( goalArea );
	goalEntity.Save( savefile );
	savefile->WriteVec3( goalEntityOrigin );

	savefile->WriteVec3( myPos );		// cnicholson: Added unsaved var
	savefile->WriteInt( myArea );		// cnicholson: Added unsaved var

	savefile->WriteVec3 ( seekPos );

	savefile->WriteInt( (int) MAX_PATH_LEN );	// cnicholson: Added unsaved vars
	for (i=0; i< MAX_PATH_LEN; ++i) {
		// TOSAVE: idReachability*		reach;
		savefile->WriteVec3( path[i].seekPos );
	}

	savefile->WriteInt( pathLen );		// cnicholson: Added unsaved var
	savefile->WriteInt( pathArea );		// cnicholson: Added unsaved var
	savefile->WriteInt( pathTime );		// cnicholson: Added unsaved var

	savefile->WriteVec3( addVelocity );
}

/*
=====================
idMoveState::Restore
=====================
*/
void idMoveState::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->Read ( &fl, sizeof(fl) );

	savefile->ReadInt( (int&)moveType );
	savefile->ReadInt( (int&)moveCommand );
	savefile->ReadInt( (int&)moveStatus );
	savefile->ReadVec3( moveDest );
	savefile->ReadVec3( moveDir );
	
	savefile->ReadInt( toAreaNum );
	savefile->ReadInt( startTime );
	savefile->ReadInt( duration );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( range );
	savefile->ReadFloat( wanderYaw );
	savefile->ReadInt( nextWanderTime );
	savefile->ReadInt( blockTime );
	obstacle.Restore ( savefile );
	savefile->ReadVec3( lastMoveOrigin );
	savefile->ReadInt( lastMoveTime );
	savefile->ReadInt( anim );

	savefile->ReadInt( travelFlags );

	savefile->ReadFloat ( kickForce );
	savefile->ReadFloat ( blockedRadius );
	savefile->ReadInt ( blockedMoveTime );
	savefile->ReadInt ( blockedAttackTime );

	savefile->ReadFloat( ideal_yaw );
	savefile->ReadFloat( current_yaw );
	savefile->ReadFloat( turnRate );
	savefile->ReadFloat( turnVel );
	savefile->ReadFloat( anim_turn_yaw );
	savefile->ReadFloat( anim_turn_amount );
	savefile->ReadFloat( anim_turn_angles );

	savefile->ReadJoint( flyTiltJoint );
	savefile->ReadFloat( fly_speed );
	savefile->ReadFloat( fly_bob_strength );
	savefile->ReadFloat( fly_bob_vert );
	savefile->ReadFloat( fly_bob_horz );
	savefile->ReadInt( fly_offset );
	savefile->ReadFloat( fly_seek_scale );
	savefile->ReadFloat( fly_roll_scale );
	savefile->ReadFloat( fly_roll_max );
	savefile->ReadFloat( fly_roll );
	savefile->ReadFloat( fly_pitch_scale );
	savefile->ReadFloat( fly_pitch_max );
	savefile->ReadFloat( fly_pitch );

	savefile->ReadInt ( (int&) currentDirection );
	savefile->ReadInt ( (int&) idealDirection );
	savefile->ReadFloat( walkRange );
	savefile->ReadFloat( walkTurn );
	savefile->ReadVec2 ( followRange );
	savefile->ReadVec2 ( searchRange );
	savefile->ReadFloat( attackPositionRange );
	savefile->ReadFloat( turnDelta );

	savefile->ReadVec3 ( goalPos );
	savefile->ReadInt ( goalArea );
	goalEntity.Restore ( savefile );
	savefile->ReadVec3( goalEntityOrigin );

	savefile->ReadVec3( myPos );	// cnicholson: Added unrestored var
	savefile->ReadInt( myArea );	// cnicholson: Added unrestored var

	savefile->ReadVec3 ( seekPos );

	savefile->ReadInt( num );
	for (i=0; i< num; ++i) {
		// TOSAVE: idReachability*		reach;
		savefile->ReadVec3( path[i].seekPos );
	}

	savefile->ReadInt( pathLen );		// cnicholson: Added unrestored var
	savefile->ReadInt( pathArea );		// cnicholson: Added unrestored var
	savefile->ReadInt( pathTime );		// cnicholson: Added unrestored var

	savefile->ReadVec3( addVelocity );
}


/*
============
idAI::SetMoveType
============
*/
void idAI::SetMoveType ( moveType_t moveType ) {
	if ( move.moveType == moveType ) {
		return;
	}
	
	StopSound ( SND_CHANNEL_HEART, false );
	
	switch ( moveType ) {
		case MOVETYPE_STATIC:
			move.travelFlags = 0;	
			break;
		
		case MOVETYPE_FLY:
			move.travelFlags = TFL_FLY|TFL_WALK|TFL_AIR;
			StartSound ( "snd_fly", SND_CHANNEL_HEART, 0, false, NULL );
			break;
			
		case MOVETYPE_ANIM:
			move.travelFlags = TFL_WALK|TFL_AIR;
			break;

		case MOVETYPE_CUSTOM:
			move.travelFlags = TFL_WALK|TFL_AIR|TFL_WALKOFFLEDGE;
			break;
	}	

	move.moveType = moveType;
}

/*
=====================
idAI::Event_SaveMove
=====================
*/
void idAI::Event_SaveMove( void ) {
	savedMove = move;
}

/*
=====================
idAI::Event_RestoreMove
=====================
*/
void idAI::Event_RestoreMove( void ) {
	idVec3 dest;

	switch( savedMove.moveCommand ) {
	case MOVE_NONE :
		StopMove( savedMove.moveStatus );
		break;

	case MOVE_FACE_ENEMY :
		FaceEnemy();
		break;

	case MOVE_FACE_ENTITY :
		FaceEntity( savedMove.goalEntity.GetEntity() );
		break;

	case MOVE_TO_ENEMY :
		MoveToEnemy();
		break;

	case MOVE_TO_ENTITY :
		MoveToEntity( savedMove.goalEntity.GetEntity(), savedMove.range );
		break;

	case MOVE_OUT_OF_RANGE :
		MoveOutOfRange( savedMove.goalEntity.GetEntity(), savedMove.range );
		break;

 	case MOVE_TO_ATTACK:
 		MoveToAttack ( savedMove.goalEntity.GetEntity(), savedMove.anim );
 		break;

	case MOVE_TO_COVER :
		MoveToCover ( combat.attackRange[0], combat.attackRange[1], AITACTICAL_COVER );
		break;

	case MOVE_TO_POSITION :
		MoveTo ( savedMove.moveDest, savedMove.range );
		break;

	case MOVE_SLIDE_TO_POSITION :
		SlideToPosition( savedMove.moveDest, savedMove.duration );
		break;

	case MOVE_WANDER :
		WanderAround();
		break;
	}

	if ( GetMovePos( move.seekPos ) ) {
		CheckObstacleAvoidance( move.seekPos, dest );
	}
}

/*
============
idAI::KickObstacles
============
*/
void idAI::KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick ) {
	int i, numListedClipModels;
	idBounds clipBounds;
	idEntity *obEnt;
	idClipModel *clipModel;
	idClipModel *clipModelList[ MAX_GENTITIES ];
	int clipmask;
	idVec3 org;
	idVec3 forceVec;
	idVec3 delta;
	idVec2 perpendicular;

	org = physicsObj.GetOrigin();

	// find all possible obstacles
	clipBounds = physicsObj.GetAbsBounds();
	clipBounds.TranslateSelf( dir * 32.0f );
	clipBounds.ExpandSelf( 8.0f );
	clipBounds.AddPoint( org );
	clipmask = physicsObj.GetClipMask();
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	numListedClipModels = gameLocal.ClipModelsTouchingBounds( this, clipBounds, clipmask, clipModelList, MAX_GENTITIES );
// RAVEN END
	for ( i = 0; i < numListedClipModels; i++ ) {
		clipModel = clipModelList[i];
		obEnt = clipModel->GetEntity();
		if ( obEnt == alwaysKick ) {
			// we'll kick this one outside the loop
			continue;
		}

		if ( !clipModel->IsTraceModel() ) {
			continue;
		}

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if (( obEnt->IsType( idMoveable::GetClassType() ) || obEnt->IsType( idAFAttachment::GetClassType() )) && obEnt->GetPhysics()->IsPushable() ) {
// RAVEN END
			delta = obEnt->GetPhysics()->GetOrigin() - org;
			delta.NormalizeFast();
			perpendicular.x = -delta.y;
			perpendicular.y = delta.x;
			delta.z += 0.5f;
			delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
			forceVec = delta * force * obEnt->GetPhysics()->GetMass();
			obEnt->ApplyImpulse( this, 0, obEnt->GetPhysics()->GetOrigin(), forceVec );
		}
	}

	if ( alwaysKick ) {
		delta = alwaysKick->GetPhysics()->GetOrigin() - org;
		delta.NormalizeFast();
		perpendicular.x = -delta.y;
		perpendicular.y = delta.x;
		delta.z += 0.5f;
		delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
		forceVec = delta * force * alwaysKick->GetPhysics()->GetMass();
		alwaysKick->ApplyImpulse( this, 0, alwaysKick->GetPhysics()->GetOrigin(), forceVec );
	}

}

/*
============
ValidForBounds
============
*/
bool ValidForBounds( const idAASSettings *settings, const idBounds &bounds ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( bounds[0][i] < settings->boundingBoxes[0][0][i] ) {
			return false;
		}
		if ( bounds[1][i] > settings->boundingBoxes[0][1][i] ) {
			return false;
		}
	}
	return true;
}

/*
=====================
idAI::SetAAS
=====================
*/
void idAI::SetAAS( void ) {
	idStr use_aas;

	spawnArgs.GetString( "use_aas", NULL, use_aas );
	if ( !use_aas || !use_aas[0] ) {
		//don't intend to use AAS at all?
		//no warning - lack of AAS intentional
		aas = NULL;
		return;
	}
	aas = gameLocal.GetAAS( use_aas );
	if ( aas ) {
		const idAASSettings *settings = aas->GetSettings();
		if ( settings ) {
			if ( !ValidForBounds( settings, physicsObj.GetBounds() ) ) {
				gameLocal.Error( "%s cannot use use_aas %s\n", name.c_str(), use_aas.c_str() );
			}
			float height = settings->maxStepHeight;
			physicsObj.SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}
	gameLocal.Printf( "WARNING: %s has no AAS file\n", name.c_str() );
}

/*
=====================
idAI::ReachedPos
=====================
*/
bool idAI::ReachedPos( const idVec3 &pos, const aiMoveCommand_t moveCommand, float range ) const {
	// When moving towards the enemy just see if our bounding box touches the desination
	if ( moveCommand == MOVE_TO_ENEMY ) {
		if ( !enemy.ent || physicsObj.GetAbsBounds().IntersectsBounds( enemy.ent->GetPhysics()->GetAbsBounds().Expand( range ) ) ) {
			return true;
		}
		return false;
	}
	
	// Dont add vertical bias when using fly move
	if ( move.moveType == MOVETYPE_FLY ) {
		float offset;
		if ( moveCommand == MOVE_TO_ENTITY ) {
			offset = 0.0f;
		} else {
			offset = move.fly_offset;
		}
		
		idBounds bnds;
		bnds = idBounds ( idVec3(-range,-range,-range), idVec3(range,range,range+offset) );
		bnds.TranslateSelf( physicsObj.GetOrigin() );	
		return bnds.ContainsPoint( pos );
	}

	if ( moveCommand == MOVE_TO_TETHER )
	{//if you're not actually in the tether, we don't want to stop early (for compliance with ai_trigger condition_tether)
		if ( !IsWithinTether() )
		{
			return false;
		}
	}
	
	// Excluded z height when determining reached
	if ( move.toAreaNum > 0 ) {
		if ( PointReachableAreaNum( physicsObj.GetOrigin() ) == move.toAreaNum ) {
			idBounds bnds;
			bnds = idBounds ( idVec3(-range,-range,-4096.0f), idVec3(range,range,4096.0f) );
			bnds.TranslateSelf( physicsObj.GetOrigin() );	
			return bnds.ContainsPoint( pos );
		}
	}
				
	idBounds bnds;
	bnds = idBounds ( idVec3(-range,-range,-16.0f), idVec3(range,range,64.0f) );
	bnds.TranslateSelf( physicsObj.GetOrigin() );	
	return bnds.ContainsPoint( pos );
}

/*
=====================
idAI::PointReachableAreaNum
=====================
*/
int idAI::PointReachableAreaNum( const idVec3 &pos, const float boundsScale ) const {
	int areaNum;
	idVec3 size;
	idBounds bounds;

	if ( !aas ) {
		return 0;
	}

	size = aas->GetSettings()->boundingBoxes[0][1] * boundsScale;
	bounds[0] = -size;
	size.z = 32.0f;
	bounds[1] = size;

	if ( move.moveType == MOVETYPE_FLY ) {
		areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK | AREA_REACHABLE_FLY );
	} else {
		areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
	}

	return areaNum;
}

/*
=====================
idAI::PathToGoal
=====================
*/
bool idAI::PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const {
	idVec3 org;
	idVec3 goal;

	if ( !aas ) {
		return false;
	}

	org = origin;
	aas->PushPointIntoAreaNum( areaNum, org );
	if ( !areaNum ) {
		return false;
	}

	goal = goalOrigin;
	aas->PushPointIntoAreaNum( goalAreaNum, goal );
	if ( !goalAreaNum ) {
		return false;
	}
	//push goal back up if flying to a point way above an area
	if ( move.moveType == MOVETYPE_FLY ) {
		if ( areaNum == goalAreaNum ) {
			if ( goalOrigin.z > goal.z ) {
				float areaTop = aas->AreaCeiling( goalAreaNum ) - GetPhysics()->GetBounds()[1].z;
				if ( goalOrigin.z > areaTop ) {
					//crap, cap it...
					goal.z = areaTop;
				} else {
					goal.z = goalOrigin.z;
				}
			}
		}
	}


	if ( move.moveType == MOVETYPE_FLY ) {
		return aas->FlyPathToGoal( path, areaNum, org, goalAreaNum, goal, move.travelFlags );
	} else {
		return aas->WalkPathToGoal( path, areaNum, org, goalAreaNum, goal, move.travelFlags );
	}
}

/*
=====================
idAI::TravelDistance

Returns the approximate travel distance from one position to the goal, or if no AAS, the straight line distance.

This is feakin' slow, so it's not good to do it too many times per frame.  It also is slower the further you
are from the goal, so try to break the goals up into shorter distances.
=====================
*/
float idAI::TravelDistance( const idVec3 &start, const idVec3 &end ) const {
	int			fromArea;
	int			toArea;
	float		dist;
	idVec2		delta;
	aasPath_t	path;

	if ( !aas ) {
		// no aas, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( DebugFilter(ai_debugMove) ) {	// Blue = Travel Distance
			gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
			gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	fromArea = PointReachableAreaNum( start );
	toArea = PointReachableAreaNum( end );

	if ( !fromArea || !toArea ) {
		// can't seem to get there
		return -1;
	}

	if ( fromArea == toArea ) {
		// same area, so just take the straight line distance
		delta = end.ToVec2() - start.ToVec2();
		dist = delta.LengthFast();

		if ( DebugFilter(ai_debugMove) ) { // Blue = Travel Distance
			gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
			gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
		}

		return dist;
	}

	idReachability *reach;
	int travelTime;
	if ( !aas->RouteToGoalArea( fromArea, start, toArea, move.travelFlags, travelTime, &reach ) ) {
		return -1;
	}

	if ( DebugFilter(ai_debugMove) ) { // Travel Distance, Fly Path & Walk Path
		if ( move.moveType == MOVETYPE_FLY ) {
			aas->ShowFlyPath( start, toArea, end );
		} else {
			aas->ShowWalkPath( start, toArea, end );
		}
	}

	return travelTime;
}

float idAI::TravelDistance ( idEntity *ent ) const {
	return TravelDistance ( physicsObj.GetOrigin(), ent->GetPhysics()->GetOrigin() );
}

float idAI::TravelDistance( idEntity* start, idEntity* end ) const {
	assert( start );
	assert( end );
	return TravelDistance( start->GetPhysics()->GetOrigin(), end->GetPhysics()->GetOrigin() );
}

float idAI::TravelDistance( const idVec3 &pos ) const {
	return TravelDistance( physicsObj.GetOrigin(), pos );
}

/*
============
idAI::ScriptedPlaybackMove
============
*/
void idAI::ScriptedPlaybackMove ( const char* playback, int flags, int numFrames ) {
	// Start the scripted sequence
	if ( !ScriptedBegin ( false ) ) {
		return;
	}

	// Start the playback
	if ( !mPlayback.Start( spawnArgs.GetString ( playback ), this, flags, numFrames ) ) {
		ScriptedEnd ( );
		return;
	}
	
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_RV_PLAYBACK;
	move.moveType		= MOVETYPE_PLAYBACK;
	move.fl.done		= false;

	// Wait till its done and mark it finished
	SetState ( "State_ScriptedPlaybackMove" );
	PostState ( "State_ScriptedStop" );
}

/*
=====================
idAI::FaceEnemy

Continually face the enemy's last known position.  MoveDone is always true in this case.
=====================
*/
bool idAI::FaceEnemy( void ) {
 	idEntity *enemyEnt = enemy.ent;
	if ( !enemyEnt ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	TurnToward( enemy.lastKnownPosition );
	move.goalEntity			= enemyEnt;
	move.moveDest			= physicsObj.GetOrigin();
	move.moveCommand		= MOVE_FACE_ENEMY;
	move.moveStatus			= MOVE_STATUS_WAITING;
	move.startTime			= gameLocal.time;
	move.speed				= 0.0f;
	
	move.fl.done			= true;
	move.fl.moving			= false;
	move.fl.goalUnreachable	= false;
	aifl.simpleThink		= false;

	return true;
}

/*
=====================
idAI::FaceEntity

Continually face the entity position.  MoveDone will never be true in this case.
=====================
*/
bool idAI::FaceEntity( idEntity *ent ) {
	if ( !ent ) {
		StopMove( MOVE_STATUS_DEST_NOT_FOUND );
		return false;
	}

	idVec3 entityOrg = ent->GetPhysics()->GetOrigin();
	TurnToward( entityOrg );
	move.goalEntity		= ent;
	move.moveDest		= physicsObj.GetOrigin();
	move.moveCommand	= MOVE_FACE_ENTITY;
	move.moveStatus		= MOVE_STATUS_WAITING;
	move.startTime		= gameLocal.time;
	move.speed			= 0.0f;
	
	move.fl.done			= false;
	move.fl.moving			= false;
	move.fl.goalUnreachable	= false;
	aifl.simpleThink		= false;

	return true;
}

/*
=====================
idAI::StartMove

Initialize a new movement by setting up the movement structure
=====================
*/
bool idAI::StartMove ( aiMoveCommand_t command, const idVec3& goalOrigin, int goalArea, idEntity* goalEntity, aasFeature_t* feature, float range ) {
	// If we are already there then we are done
	if ( ReachedPos( goalOrigin, command ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.lastMoveOrigin		= physicsObj.GetOrigin ( );

	move.seekPos = move.goalPos = move.moveDest	= goalOrigin;
	move.toAreaNum			= goalArea;
	move.goalEntity			= goalEntity;
	move.moveCommand		= command;
	move.moveStatus			= MOVE_STATUS_MOVING;
	move.speed				= move.fly_speed;
	move.startTime			= gameLocal.time;
	move.range				= range;
	
	move.fl.done			= false;
	move.fl.goalUnreachable	= false;
	move.fl.moving			= true;

	aasSensor->Reserve ( feature );

	OnStartMoving ( );
	
	return true;
}

/*
=====================
idAI::StopMove
=====================
*/
void idAI::StopMove( moveStatus_t status ) {
	aiMoveCommand_t oldCommand = move.moveCommand;
	float saveZ = 0.0f;

	move.fl.done			= true;
	move.fl.moving			= false;
	move.fl.goalUnreachable	= false;
	move.fl.obstacleInPath	= false;
	move.fl.blocked			= false;

	if ( move.moveType == MOVETYPE_FLY ) {
		if ( move.moveCommand == MOVE_TO_ENEMY
			|| move.moveCommand == MOVE_TO_ATTACK ) {
			saveZ = (move.moveDest.z-physicsObj.GetOrigin().z);
		}
	}
	move.moveCommand		= MOVE_NONE;
	move.moveStatus			= status;
	move.toAreaNum			= 0;
	move.goalEntity			= NULL;
	move.moveDest			= physicsObj.GetOrigin();
	move.moveDest.z			+= saveZ;
	move.startTime			= gameLocal.time;
	move.duration			= 0;
	move.range				= 0.0f;
	move.speed				= 0.0f;
 	move.anim				= 0;
 	
	move.moveDir.Zero();
	move.lastMoveOrigin.Zero();
	move.lastMoveTime	= gameLocal.time;
	
	// Callback for handling stopping
	if ( oldCommand != MOVE_NONE ) {
		OnStopMoving ( oldCommand );
	}
}

/*
=====================
idAI::MoveToTether
=====================
*/
bool idAI::MoveToTether ( rvAITether* tether ) {
	aasGoal_t	goal;

	// find a goal using the currently active tether
	if ( !aas || !tether ) {
		return false;
	}
	
	if ( !tether->FindGoal ( this, goal ) ) {		
		//This is extremely bad - if 2 guys both try to get to the center of a tether, they get hosed.
		return MoveTo ( tether->GetPhysics()->GetOrigin ( ), tether->GetOriginReachedRange() );
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		//Hmm... we shouldn't have to clamp as the area wouldn't have been valid unless the tether was in its bounds and height, but...?
		//float areaTop = aas->AreaCeiling( goal.areaNum );
		goal.origin.z = tether->GetPhysics()->GetOrigin().z;
	}
	
	return StartMove ( MOVE_TO_TETHER, goal.origin, goal.areaNum, NULL, NULL, AI_TETHER_MINRANGE );
}

/*
=====================
idAI::MoveToAttack 
=====================
*/
bool idAI::MoveToAttack ( idEntity *ent, int attack_anim ) {
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idBounds		bounds; 
	idVec3			pos;

	if ( !aas || !ent ) {
		return false;
	}
	
	const idVec3 &org  = physicsObj.GetOrigin();
	obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();
	pos				   = LastKnownPosition ( ent );

	// if we havent started a find yet or the current find is something other than a attack find then start a new one
	if ( !aasFind || !dynamic_cast<rvAASFindGoalForAttack*>(aasFind) ) {
		// Allocate the new aas find
		delete aasFind;
		aasFind = new rvAASFindGoalForAttack ( this );

		// Find a goal that gives us a viable attack position on our enemy
		aas->FindNearestGoal( goal, PointReachableAreaNum( org ), org, pos, move.travelFlags, IsTethered()?0:move.searchRange[0], move.searchRange[1], &obstacle, 1, *aasFind );
	}

	assert ( aasFind );
	
	// Test some more points with the existing find
	rvAASFindGoalForAttack* aasFindAttack = static_cast<rvAASFindGoalForAttack*>(aasFind);
	if ( !aasFindAttack->TestCachedGoals ( aifl.simpleThink ? 1 : 4, goal ) ) {
		delete aasFind;
		aasFind = NULL;
		return false;
	}

	// Havent found a goal yet but exhaused our trace count
	if ( !goal.areaNum ) { 
		return false;
	}
		
	// Dont need the find anymore
	delete aasFind;
	aasFind = NULL;
	
	if ( move.moveType == MOVETYPE_FLY ) {
		//float up above the ground?
		if ( aas && aas->GetFile() ) {
			//we have AAS
			float areaTop = aas->AreaCeiling(goal.areaNum)-GetPhysics()->GetBounds()[1][2];
			if ( pos.z > areaTop ) {
				goal.origin.z = areaTop;
			} else if ( pos.z > goal.origin.z+1.0f ) {
				goal.origin.z = pos.z;
			} else if ( move.fly_offset > 0 ) {
				if ( (goal.origin.z+move.fly_offset) > areaTop ) {
					goal.origin.z = areaTop;
				} else {
					goal.origin.z += move.fly_offset;
				}
			}
		}
	}

	return StartMove ( MOVE_TO_ATTACK, goal.origin, goal.areaNum, ent, NULL, move.attackPositionRange?move.attackPositionRange:8.0f );
}

/*
=====================
idAI::MoveToEnemy
=====================
*/
bool idAI::MoveToEnemy( void ) {
	int			areaNum;
	aasPath_t	path;
	idVec3		pos;

	if ( !enemy.ent ) {
		return false;
	}

//	pos = LastKnownPosition ( enemy.ent );
	pos = enemy.ent->GetPhysics()->GetOrigin ( );
	
	// If we are already moving to the entity and its position hasnt changed then we are done
	if ( move.moveCommand == MOVE_TO_ENEMY && move.goalEntity == enemy.ent && move.goalEntityOrigin == pos ) {
		return true;
	}

	// Early out if we are already there.
	if ( ReachedPos( pos, MOVE_TO_ENEMY, 8.0f ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	// See if its posible to get where we want to go
	areaNum = 0;
	if ( aas ) {
		areaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( areaNum, pos );

		if ( !PathToGoal( path, PointReachableAreaNum( physicsObj.GetOrigin() ), physicsObj.GetOrigin(), areaNum, pos ) ) {
			return false;
		}
	}

	move.goalEntityOrigin = pos;

	if ( move.moveType == MOVETYPE_FLY ) {
	//float up above the ground?
		if ( aas && aas->GetFile() ) {
		//we have AAS
			float areaTop = aas->AreaCeiling(areaNum)-GetPhysics()->GetBounds()[1][2];
			if ( move.fly_offset > 0 ) {
				if ( (pos.z+move.fly_offset) > areaTop ) {
					pos.z = areaTop;
					move.goalEntityOrigin.z = areaTop;
				} else {
					pos.z += move.fly_offset;
					move.goalEntityOrigin.z += move.fly_offset;
				}
			}
		}
	}

	// If we are already moving towards the given enemy then we have updated enough
	if ( move.moveCommand == MOVE_TO_ENEMY && move.goalEntity == enemy.ent ) {
		move.moveDest	= pos;
		move.toAreaNum	= areaNum;
		return true;
	}

	return StartMove ( MOVE_TO_ENEMY, pos, areaNum, enemy.ent, NULL, 8.0f );
}

/*
=====================
idAI::MoveToEntity
=====================
*/
bool idAI::MoveToEntity( idEntity *ent, float range ) {
	int			areaNum;
	aasPath_t	path;
	idVec3		pos;

	if ( !ent ) {
		return false;
	}

	// Where do we want to go?
	pos = ent->GetPhysics()->GetOrigin();

	// If we are already moving to the entity and its position hasnt changed then we are done
	if ( move.moveCommand == MOVE_TO_ENTITY && move.goalEntity == ent && move.goalEntityOrigin == pos ) {
		return true;
	}

	// If we arent flying we should move to a position on the floor
	if ( move.moveType != MOVETYPE_FLY ) {
		ent->GetFloorPos( 64.0f, pos );
	}

	// Early out if we are already there.
	if ( ReachedPos( pos, MOVE_TO_ENTITY, range ) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	// See if its posible to get where we want to go
	areaNum = 0;
	if ( aas ) {
		areaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( areaNum, pos );

		if ( !PathToGoal( path, PointReachableAreaNum( physicsObj.GetOrigin() ), physicsObj.GetOrigin(), areaNum, pos ) ) {
			return false;
		}
	}

	move.goalEntityOrigin	= ent->GetPhysics()->GetOrigin();

	// If we are already moving towards the given enemy then we have updated enough
	if ( move.moveCommand == MOVE_TO_ENTITY && move.goalEntity == ent ) {
		move.moveDest	= pos;
		move.range		= range <= 0.0f ? 8.0f : range;
		move.toAreaNum	= areaNum;
		return true;
	}

	return StartMove ( MOVE_TO_ENTITY, pos, areaNum, ent, NULL, range <= 0.0f ? 8.0f : range );
}

/*
=====================
idAI::MoveOutOfRange
=====================
*/
bool idAI::MoveOutOfRange( idEntity *ent, float range, float minRange ) {
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idBounds		bounds;
	idVec3			pos;
	int				obstacles;

	if ( !aas || !ent ) {
		return false;
	}

	const idVec3 &org = physicsObj.GetOrigin();

	// consider the entity the monster is getting close to as an obstacle
	if ( ent != this ) {
		obstacles = 1;
		obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();
	} else {
		obstacles = 0;
	}

	pos = LastKnownPosition ( ent );

	// Find a goal out of range of where we are
	rvAASFindGoalOutOfRange findGoal( this );
	if ( !aas->FindNearestGoal( goal, PointReachableAreaNum( org ), org, pos, move.travelFlags, minRange, range, &obstacle, obstacles, findGoal ) ) {
		return false;
	}

	return StartMove ( MOVE_OUT_OF_RANGE, goal.origin, goal.areaNum, ent, NULL, 8.0f );
}

/*
=====================
idAI::MoveTo
=====================
*/
bool idAI::MoveTo ( const idVec3 &pos, float range ) {
	idVec3		org;
	int			areaNum;
	aasPath_t	path;

	if ( !aas ) {
		return false;
	}

	org     = pos;
	areaNum = PointReachableAreaNum( org );
	if ( !areaNum ) {
		return false;
	}

	// Can we get to where we want to go?
	aas->PushPointIntoAreaNum( areaNum, org );
	if ( !PathToGoal( path, areaNum, physicsObj.GetOrigin(), PointReachableAreaNum( physicsObj.GetOrigin() ), org ) ) {
		return false;
	}
	
	// Start moving
	return StartMove ( MOVE_TO_POSITION, org, areaNum, NULL, NULL, range );
}

/*
=====================
idAI::MoveToCover
=====================
*/
bool idAI::MoveToCover( float minRange, float maxRange, aiTactical_t coverType ) {
	idVec3				org;
	int					areaNum;
	aasPath_t			path;
	aasFeature_t*		feature = 0;
	idVec3				featureOrigin;

	if ( !aas ) {
		return false;
	}
			
	// Look for nearby cover
	switch ( coverType ) {
		case AITACTICAL_HIDE:			aasSensor->SearchHide();	break;
		case AITACTICAL_COVER_FLANK:	aasSensor->SearchFlank();	break;
		case AITACTICAL_COVER_ADVANCE:	aasSensor->SearchAdvance();	break;
		case AITACTICAL_COVER_RETREAT:	aasSensor->SearchRetreat();	break;
		case AITACTICAL_COVER_AMBUSH:	aasSensor->SearchAmbush();	break;
		case AITACTICAL_COVER:
		default:
			aasSensor->SearchCover();
			break;
	}
				
	if ( !aasSensor->FeatureCount ( ) ) {
		return false;
	}	

	feature		  = aasSensor->Feature ( 0 );
	featureOrigin = feature->Origin ( );
	org			  = featureOrigin;
	areaNum		  = 0;
	
	// Find the aas area our cover point is in
	areaNum = PointReachableAreaNum( org );
	if ( !areaNum ) {
		return false;
	}
	
	// See if there is a path to our goal or not
	aas->PushPointIntoAreaNum( areaNum, org );
/*
	if ( !PathToGoal( path, PointReachableAreaNum( physicsObj.GetOrigin() ), physicsObj.GetOrigin(), areaNum, org ) ) {
		return false;
	}
*/

	combat.coverValidTime = gameLocal.time;

	// Start the move
	return StartMove ( MOVE_TO_COVER, org, areaNum, enemy.ent, feature, AI_COVER_MINRANGE / 2.0f );
}

/*
=====================
idAI::MoveToHide
=====================
*/
bool idAI::MoveToHide ( void ) {
	aasObstacle_t	obstacle;
	aasGoal_t		goal;
	idBounds		bounds;
	idVec3			pos;

	// Need an enemy to hide from
	if ( !aas || !enemy.ent ) {
		return false;
	}

	const idVec3& org  = physicsObj.GetOrigin();	
	obstacle.absBounds = enemy.ent->GetPhysics()->GetAbsBounds();
	pos				   = LastKnownPosition ( enemy.ent );

	// Search aas for a suitable goal
	rvAASFindGoalForHide findGoal( pos );
	if ( !aas->FindNearestGoal( goal, PointReachableAreaNum( org ), org, pos, move.travelFlags, 0.0f, 0.0f, &obstacle, 1, findGoal ) ) {
		return false;
	}

	// Start the movement
	return StartMove ( MOVE_TO_HIDE, goal.origin, goal.areaNum, enemy.ent, NULL, 0.0f );
}

/*
=====================
idAI::SlideToPosition
=====================
*/
bool idAI::SlideToPosition( const idVec3 &pos, float time ) {
	StopMove( MOVE_STATUS_DONE );

	move.moveDest		= pos;
	move.goalEntity		= NULL;
	move.moveCommand	= MOVE_SLIDE_TO_POSITION;
	move.moveStatus		= MOVE_STATUS_MOVING;
	move.startTime		= gameLocal.time;
	move.duration		= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( time ) );
	
	move.fl.done			= false;
	move.fl.goalUnreachable	= false;
	move.fl.moving			= false;
	aifl.simpleThink		= false;
		
	move.fl.allowAnimMove				= false;

	if ( move.duration > 0 ) {
		move.moveDir = ( pos - physicsObj.GetOrigin() ) / MS2SEC( move.duration );
		if ( move.moveType != MOVETYPE_FLY ) {
			move.moveDir.z = 0.0f;
		}
		move.speed = move.moveDir.LengthFast();
	}

	return true;
}

/*
=====================
idAI::WanderAround
=====================
*/
bool idAI::WanderAround( void ) {
	idVec3 dest;
	
	StopMove( MOVE_STATUS_DONE );
	
	dest = physicsObj.GetOrigin() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;
	if ( !NewWanderDir( dest ) ) {
		StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		move.fl.goalUnreachable = true;
		return false;
	}

	return StartMove ( MOVE_WANDER, dest, 0, NULL, NULL, 0.0f );
}

/*
================
idAI::StepDirection
================
*/
bool idAI::StepDirection( float dir ) {
	predictedPath_t path;
	idVec3 org;

	move.wanderYaw = dir;
	move.moveDir = idAngles( 0, move.wanderYaw, 0 ).ToForward();

	org = physicsObj.GetOrigin();

	idAI::PredictPath( this, aas, org, move.moveDir * 48.0f, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( path.blockingEntity && ( ( move.moveCommand == MOVE_TO_ENEMY ) || ( move.moveCommand == MOVE_TO_ENTITY ) ) && ( path.blockingEntity == move.goalEntity.GetEntity() ) ) {
		// don't report being blocked if we ran into our goal entity
		return true;
	}

	if ( ( move.moveType == MOVETYPE_FLY ) && ( path.endEvent == SE_BLOCKED ) ) {
		float z;

		move.moveDir = path.endVelocity * 1.0f / 48.0f;

		// trace down to the floor and see if we can go forward
		idAI::PredictPath( this, aas, org, idVec3( 0.0f, 0.0f, -1024.0f ), 1000, 1000, SE_BLOCKED, path );

		idVec3 floorPos = path.endPos;
		idAI::PredictPath( this, aas, floorPos, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
		if ( !path.endEvent ) {
			move.moveDir.z = -1.0f;
			return true;
		}

		// trace up to see if we can go over something and go forward
		idAI::PredictPath( this, aas, org, idVec3( 0.0f, 0.0f, 256.0f ), 1000, 1000, SE_BLOCKED, path );

		idVec3 ceilingPos = path.endPos;

		for( z = org.z; z <= ceilingPos.z + 64.0f; z += 64.0f ) {
			idVec3 start;
			if ( z <= ceilingPos.z ) {
				start.x = org.x;
				start.y = org.y;
                start.z = z;
			} else {
				start = ceilingPos;
			}
			idAI::PredictPath( this, aas, start, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
			if ( !path.endEvent ) {
				move.moveDir.z = 1.0f;
				return true;
			}
		}
		return false;
	}

	return ( path.endEvent == 0 );
}

/*
================
idAI::NewWanderDir
================
*/
bool idAI::NewWanderDir( const idVec3 &dest ) {
	float	deltax, deltay;
	float	d[ 3 ];
	float	tdir, olddir, turnaround;

	move.nextWanderTime = gameLocal.time + ( gameLocal.random.RandomFloat() * 500 + 500 );

	olddir = idMath::AngleNormalize360( ( int )( move.current_yaw / 45 ) * 45 );
	turnaround = idMath::AngleNormalize360( olddir - 180 );

	idVec3 org = physicsObj.GetOrigin();
	deltax = dest.x - org.x;
	deltay = dest.y - org.y;
	if ( deltax > 10 ) {
		d[ 1 ]= 0;
	} else if ( deltax < -10 ) {
		d[ 1 ] = 180;
	} else {
		d[ 1 ] = DI_NODIR;
	}

	if ( deltay < -10 ) {
		d[ 2 ] = 270;
	} else if ( deltay > 10 ) {
		d[ 2 ] = 90;
	} else {
		d[ 2 ] = DI_NODIR;
	}

	// try direct route
	if ( d[ 1 ] != DI_NODIR && d[ 2 ] != DI_NODIR ) {
		if ( d[ 1 ] == 0 ) {
			tdir = d[ 2 ] == 90 ? 45 : 315;
		} else {
			tdir = d[ 2 ] == 90 ? 135 : 215;
		}

		if ( tdir != turnaround && StepDirection( tdir ) ) {
			return true;
		}
	}

	// try other directions
	if ( ( gameLocal.random.RandomInt() & 1 ) || abs( deltay ) > abs( deltax ) ) {
		tdir = d[ 1 ];
		d[ 1 ] = d[ 2 ];
		d[ 2 ] = tdir;
	}

	if ( d[ 1 ] != DI_NODIR && d[ 1 ] != turnaround && StepDirection( d[1] ) ) {
		return true;
	}

	if ( d[ 2 ] != DI_NODIR && d[ 2 ] != turnaround	&& StepDirection( d[ 2 ] ) ) {
		return true;
	}

	// there is no direct path to the player, so pick another direction
	if ( olddir != DI_NODIR && StepDirection( olddir ) ) {
		return true;
	}

	 // randomly determine direction of search
	if ( gameLocal.random.RandomInt() & 1 ) {
		for( tdir = 0; tdir <= 315; tdir += 45 ) {
			if ( tdir != turnaround && StepDirection( tdir ) ) {
                return true;
			}
		}
	} else {
		for ( tdir = 315; tdir >= 0; tdir -= 45 ) {
			if ( tdir != turnaround && StepDirection( tdir ) ) {
				return true;
			}
		}
	}

	if ( turnaround != DI_NODIR && StepDirection( turnaround ) ) {
		return true;
	}

	// can't move
	StopMove( MOVE_STATUS_DEST_UNREACHABLE );
	return false;
}

/*
=====================
idAI::GetMovePos
=====================
*/
bool idAI::GetMovePos( idVec3 &seekPos, idReachability** seekReach ) {
	int			areaNum;
	aasPath_t	path;
	bool		result;
	idVec3		org;

	org = physicsObj.GetOrigin();
	seekPos = org;

 	// RAVEN BEGIN
 	// cdr: Alternate Routes Bug
 	if (seekReach) {
 		(*seekReach) = 0;
 	}
 	// RAVEN END

	switch( move.moveCommand ) {
		case MOVE_NONE :
			seekPos = move.moveDest;
			return false;

		case MOVE_FACE_ENEMY :
		case MOVE_FACE_ENTITY :
			seekPos = move.moveDest;
			return false;

		case MOVE_TO_POSITION_DIRECT :
			seekPos = move.moveDest;
			if ( ReachedPos( move.moveDest, move.moveCommand ) ) {
				StopMove( MOVE_STATUS_DONE );
			}
			return false;
	
		case MOVE_SLIDE_TO_POSITION :
			seekPos = org;
			return false;
			
		case MOVE_TO_ENTITY:
			MoveToEntity( move.goalEntity.GetEntity(), move.range );
			break;
			
		case MOVE_TO_ENEMY:
			if ( !MoveToEnemy() && combat.tacticalCurrent == AITACTICAL_MELEE ) {
				StopMove( MOVE_STATUS_DEST_UNREACHABLE );
				return false;
			}
			break;
	}

	if ( move.moveType == MOVETYPE_FLY && move.moveCommand >= NUM_NONMOVING_COMMANDS ) {
		//flying
		if ( DistanceTo( move.moveDest ) < 1024.0f ) {
			//less than huge translation dist, which is actually 4096, but, just to be safe...
			trace_t moveTrace;
			gameLocal.Translation( this, moveTrace, org, move.moveDest, physicsObj.GetClipModel(), mat3_identity, MASK_MONSTERSOLID, this, move.goalEntity.GetEntity() );
			if ( moveTrace.fraction >= 1.0f ) {
				//can head straight for it, so do it.
				seekPos = move.moveDest;
				return false;
			}
		}
	}

	move.moveStatus = MOVE_STATUS_MOVING;
	result = false;
	
	if ( move.moveCommand == MOVE_WANDER ) {
		move.moveDest = org + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 256.0f;
	} else {
		if ( ReachedPos( move.moveDest, move.moveCommand, move.range ) ) {
			StopMove( MOVE_STATUS_DONE );
			seekPos	= org;
			return false;
		}
	}

	if ( aas && move.toAreaNum ) {
		areaNum	= PointReachableAreaNum( org );
		if ( PathToGoal( path, areaNum, org, move.toAreaNum, move.moveDest ) ) {
			seekPos = path.moveGoal;
			if ( aas->GetFile() ) {
				//we have AAS
				if ( move.fly_offset > 0 ) {
					if ( (seekPos-move.moveDest).LengthSqr() > 10.0f ) {
						//not heading to final dest, that already has offset in it
						float areaTop = aas->AreaCeiling(path.moveAreaNum)-GetPhysics()->GetBounds()[1][2];
						if ( (seekPos.z+move.fly_offset) > areaTop ) {
							seekPos.z = areaTop;
						} else {
							seekPos.z += move.fly_offset;
						}
					}
				}
			}

 			// RAVEN BEGIN
 			// cdr: Alternate Routes Bug
 			if (seekReach) {
 				(*seekReach) = (idReachability*)(path.reachability);
 			}
 			// RAVEN END

			result = true;
			move.nextWanderTime = 0;
		} else {
			move.fl.goalUnreachable = true;
		}
	}


	if ( !result ) {
		// wander around
		if ( ( gameLocal.time > move.nextWanderTime ) || !StepDirection( move.wanderYaw ) ) {
			result = NewWanderDir( move.moveDest );
			if ( !result ) {
				StopMove( MOVE_STATUS_DEST_UNREACHABLE );
				move.fl.goalUnreachable = true;
				seekPos	= org;

				return false;
			}
		} else {
			result = true;
		}

		seekPos = org + move.moveDir * 2048.0f;

	} else {
		move.fl.goalUnreachable = false;
	}

	if (  DebugFilter(ai_debugMove) ) { // YELLOW = seekPos
		gameRenderWorld->DebugLine( colorYellow, physicsObj.GetOrigin(), seekPos );
	}

	return result;
}



/*
=====================
idAI::BlockedFailSafe
=====================
*/
void idAI::BlockedFailSafe( void ) {
/*	move.fl.blocked = false;

	if ( !ai_blockedFailSafe.GetBool() || move.blockedRadius < 0.0f ) {
		return;
	}
	if ( !physicsObj.OnGround() || enemy.GetEntity() == NULL ||
			( physicsObj.GetOrigin() - move.lastMoveOrigin ).LengthSqr() > Square( move.blockedRadius ) ) {
		move.lastMoveOrigin = physicsObj.GetOrigin();
		move.lastMoveTime = gameLocal.time;
	}
	if ( move.lastMoveTime < gameLocal.time - move.blockedMoveTime ) {
		if ( lastAttackTime < gameLocal.time - move.blockedAttackTime ) {
			move.fl.blocked = true;
			move.lastMoveTime = gameLocal.time;
		}
	}*/
}

/***********************************************************************

	turning

***********************************************************************/

/*
=====================
idAI::TurnToward
=====================
*/
bool idAI::TurnToward( float yaw ) {
	move.ideal_yaw = idMath::AngleNormalize180( yaw );
	bool result = FacingIdeal();
	return result;
}

/*
=====================
idAI::TurnToward
=====================
*/
bool idAI::TurnToward( const idVec3 &pos ) {
	idVec3 dir;
	idVec3 local_dir;
	float lengthSqr;

	dir = pos - physicsObj.GetOrigin();
	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	lengthSqr = local_dir.LengthSqr();
	if ( lengthSqr > Square( 2.0f ) || ( lengthSqr > Square( 0.1f ) && enemy.ent  == NULL ) ) {
		move.ideal_yaw = idMath::AngleNormalize180( local_dir.ToYaw() );
	}

	return FacingIdeal();
}

/*
=====================
idAI::TurnTowardLeader
=====================
*/
bool idAI::TurnTowardLeader( bool faceLeaderByDefault ) {
	if ( !leader.GetEntity() ) {
		return false;
	}
	//see if there's a wall in the direction the player's looking
	trace_t tr;

	idVec3 leaderLookDest = leader->GetPhysics()->GetOrigin() + (( (leader.GetEntity() && leader.GetEntity()->IsType( idPlayer::GetClassType() )) ? ((idPlayer*)leader.GetEntity())->intentDir:leader->viewAxis[0])*300.0f);
	idVec3 myLookDir = leaderLookDest-GetPhysics()->GetOrigin();
	myLookDir.Normalize();
	idVec3 start = GetPhysics()->GetOrigin();
	start.z += EyeHeight();
	idVec3 end = start + (myLookDir*128.0f);
	end.z = start.z;
	idVec3 currentLookDir = viewAxis[0];
	currentLookDir.Normalize();
	
	if ( !GetEnemy() && (!leader->IsType(idPlayer::GetClassType()) || !((idPlayer*)leader.GetEntity())->IsFlashlightOn()) ) {
		//Not in combat and leader isn't looking around with flashlight
		if ( myLookDir*currentLookDir > 0.666f ) {
			//new dir isn't different enough from current dir for me to care
			return true;
		}
	}
	gameLocal.TracePoint( this, tr, start, end, MASK_OPAQUE, this );
	idEntity* traceEnt = gameLocal.entities[ tr.c.entityNum ];
	if ( tr.fraction < 1.0f 
		&& (tr.fraction<0.5f||!traceEnt||!traceEnt->IsType(idDoor::GetClassType())) ) {
		//wall there - NOTE: okay to look at doors
		if ( faceLeaderByDefault//want to face leader by default
			|| leader->viewAxis[0].ToYaw() == move.ideal_yaw ) {//a wall must have moved in front of us?
			//face the leader
			return TurnToward( leader->GetPhysics()->GetOrigin() );
		}
		//just keep looking in the last valid dir
		if ( FacingIdeal() ) {
			//make sure it's still valid
			idVec3 end = start + (viewAxis[0]*128.0f);
			end.z = start.z;
			gameLocal.TracePoint( this, tr, start, end, MASK_OPAQUE, this );
			traceEnt = gameLocal.entities[ tr.c.entityNum ];
			if ( tr.fraction < 1.0f
				&& (tr.fraction<0.5f||!traceEnt||!traceEnt->IsType(idDoor::GetClassType())) ) {
				//a wall right in front of us - NOTE: okay to look at doors
				//face the leader
				return TurnToward( leader->GetPhysics()->GetOrigin() );
			}
		}
		return true;
	}
	//opening, face leader's dir
	return TurnToward( myLookDir.ToYaw() );
}

/*
============
idAI::DirectionalTurnToward

Turn toward the given point using directional movement
============
*/
bool idAI::DirectionalTurnToward ( const idVec3 &pos ) {
	static float moveDirOffset [ MOVEDIR_MAX ] = {
		0.0f, 180.0f, -90.0f, 90.0f
	};

	// Issue standard TurnToward if we are not currently eligible for directional movement
	if ( combat.tacticalCurrent != AITACTICAL_MOVE_PLAYERPUSH ) {
		//always move directionally when getting out of the player's way
		if( !combat.fl.aware || focusType < AIFOCUS_USE_DIRECTIONAL_MOVE || !move.fl.moving || move.moveCommand == MOVE_TO_ENEMY || !move.fl.allowDirectional ) {
			move.idealDirection = MOVEDIR_FORWARD;		
			return TurnToward ( pos );		
		}
	}

	// Turn towards our desination
	float moveYaw;
	TurnToward ( pos );
	moveYaw = move.ideal_yaw;

	// Turn towards the goal entity and determine the angle difference 
	TurnToward ( currentFocusPos );

	// Check for a direction change only when we can no longer see 
	// where we need to look and where we are looking is greater than 3/4ths the maximum look
	if ( (pos - GetPhysics()->GetOrigin()).LengthFast ( ) > 8.0f ) 
	if ( !FacingIdeal ( ) ) 
	if ( fabs ( idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ) >= fabs(lookMax[YAW]) || 
	     fabs ( idMath::AngleNormalize180 ( idMath::AngleNormalize180 ( moveDirOffset[move.idealDirection] + moveYaw ) - move.current_yaw ) ) >= 80.0f ) {

		float diffYaw;
		diffYaw = idMath::AngleNormalize180 ( move.ideal_yaw - moveYaw );

		if ( diffYaw > -45.0f && diffYaw < 45.0f ) {
			move.idealDirection = MOVEDIR_FORWARD;
		} else if ( diffYaw < -135.0f || diffYaw > 135.0f ) {
			move.idealDirection = MOVEDIR_BACKWARD;
		} else if ( diffYaw < 0.0f ) { 
			move.idealDirection = MOVEDIR_LEFT;
		} else {
			move.idealDirection = MOVEDIR_RIGHT;
		} 
	}

	return TurnToward( idMath::AngleNormalize180 ( moveDirOffset[move.idealDirection] + moveYaw ) );
}

/*
=====================
idAI::Turn
=====================
*/
void idAI::Turn( void ) {
	float diff;
	float diff2;
	float turnAmount;
	animFlags_t animflags;

	// If cant turn or turning is disabled just bail
	if ( !CanTurn ( ) ) {
		return;
	}

	// check if the animator has marked this anim as non-turning
	if ( !legsAnim.Disabled() && !legsAnim.AnimDone( 0 ) ) {
		animflags = legsAnim.GetAnimFlags();
	} else {
		animflags = torsoAnim.GetAnimFlags();
	}
	if ( animflags.ai_no_turn ) {
		return;
	}

	if ( move.anim_turn_angles && animflags.anim_turn ) {
		idMat3 rotateAxis;

		// set the blend between no turn and full turn
		float frac = move.anim_turn_amount / move.anim_turn_angles;
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, 1.0f - frac );
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, frac );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, 1.0f - frac );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, frac );

		// get the total rotation from the start of the anim
		animator.GetDeltaRotation( 0, gameLocal.time, rotateAxis );
		move.current_yaw = idMath::AngleNormalize180( move.anim_turn_yaw + rotateAxis[ 0 ].ToYaw() );
	} else {
		diff = idMath::AngleNormalize180( move.ideal_yaw - move.current_yaw );

		if ( move.currentDirection != move.idealDirection ) {
			move.turnVel += (AI_TURN_SCALE * 2.0f)* diff * MS2SEC( gameLocal.msec );
 		} else if ( move.fl.running ) {
 			move.turnVel += (AI_TURN_SCALE * 3.0f)* diff * MS2SEC( gameLocal.msec );
 		} else if ( move.fl.moving ) {
 			move.turnVel += (AI_TURN_SCALE * 1.5f)* diff * MS2SEC( gameLocal.msec );
		} else {
			move.turnVel += AI_TURN_SCALE * diff * MS2SEC( gameLocal.msec );
		}
		
		if ( move.turnVel > move.turnRate ) {
			move.turnVel = move.turnRate;
		} else if ( move.turnVel < -move.turnRate ) {
			move.turnVel = -move.turnRate;
		}
		turnAmount = move.turnVel * MS2SEC( gameLocal.msec );
		if ( ( diff >= 0.0f ) && ( turnAmount >= diff ) ) {
			move.turnVel = diff / MS2SEC( gameLocal.msec );
			turnAmount = diff;
		} else if ( ( diff <= 0.0f ) && ( turnAmount <= diff ) ) {
			move.turnVel = diff / MS2SEC( gameLocal.msec );
			turnAmount = diff;
		}
		move.current_yaw = idMath::AngleNormalize180( move.current_yaw + turnAmount );
		diff2 = idMath::AngleNormalize180( move.ideal_yaw - move.current_yaw );

		if ( idMath::Fabs( diff2 ) < 0.1f ) {
			move.current_yaw = move.ideal_yaw;
		}
	}

	viewAxis = idAngles( 0, move.current_yaw, 0 ).ToMat3();

//	if ( DebugFilter(ai_debugMove) ) { // RED = ideal_yaw, GREEN = current_yaw,  YELLOW = current+velocity
//		const idVec3 &org = physicsObj.GetOrigin();
//		gameRenderWorld->DebugLine( colorRed, org, org + idAngles( 0, move.ideal_yaw, 0 ).ToForward() * 64, gameLocal.msec );
//		gameRenderWorld->DebugLine( colorGreen, org, org + idAngles( 0, move.current_yaw, 0 ).ToForward() * 48, gameLocal.msec );
//		gameRenderWorld->DebugLine( colorYellow, org, org + idAngles( 0, move.current_yaw + move.turnVel, 0 ).ToForward() * 32, gameLocal.msec );
//		if ( move.anim_turn_angles && animflags.anim_turn ) {
//			gameRenderWorld->DebugLine( colorOrange, org, org + idAngles( 0, move.anim_turn_yaw, 0 ).ToForward() * 32, gameLocal.msec );
//		}
//	}
}

/*
=====================
idAI::FacingIdeal
=====================
*/
bool idAI::FacingIdeal( void ) {
	float diff;

	if ( !move.turnRate ) {
		return true;
	}

	diff = idMath::AngleDelta ( move.current_yaw, move.ideal_yaw );
	if ( idMath::Fabs( diff ) < 0.01f ) {
		// force it to be exact
		move.current_yaw = move.ideal_yaw;
		return true;
	}

	return false;
}

/*
================
idAI::AnimTurn
================
*/
void idAI::AnimTurn ( float angles, bool force ) {
	move.turnVel = 0.0f;
	move.anim_turn_angles = angles;
	if ( angles ) {
		move.anim_turn_yaw = move.current_yaw;

		if ( force ) {
			move.anim_turn_amount = angles;
		} else { 
			move.anim_turn_amount = idMath::Fabs( idMath::AngleNormalize180( move.current_yaw - move.ideal_yaw ) );
			if ( move.anim_turn_amount > move.anim_turn_angles ) {
				move.anim_turn_amount = move.anim_turn_angles;
			}
		}
	} else {
		move.anim_turn_amount = 0.0f;
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, 0.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, 1.0f );
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, 0.0f );
	}
}

/***********************************************************************

	Movement Helper Functions

***********************************************************************/

/*
================
idAI::ApplyImpulse
================
*/
void idAI::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash ) {
	// FIXME: Jim take a look at this and see if this is a reasonable thing to do
	// instead of a spawnArg flag.. Sabaoth is the only slide monster ( and should be the only one for D3 )
	// and we don't want him taking physics impulses as it can knock him off the path
	if ( move.moveType != MOVETYPE_STATIC && move.moveType != MOVETYPE_SLIDE ) {
		idActor::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
=====================
idAI::GetAnimMoveDelta
=====================
*/
void idAI::GetAnimMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta ) {
	idVec3 oldModelOrigin;
	idVec3 modelOrigin;

	animator.GetDelta( gameLocal.time - gameLocal.msec, gameLocal.time, delta );
	delta = axis * delta;

	if ( modelOffset != vec3_zero ) {
		// the pivot of the monster's model is around its origin, and not around the bounding
		// box's origin, so we have to compensate for this when the model is offset so that
		// the monster still appears to rotate around it's origin.
		oldModelOrigin = modelOffset * oldaxis;
		modelOrigin = modelOffset * axis;
		delta += oldModelOrigin - modelOrigin;
	}

	delta *= physicsObj.GetGravityAxis();
}



/*
=====================
TestTeammateCollisions
=====================
*/

void TestTeammateCollisions(idAI* owner) {
	idActor*		teammate;
	idVec3			teammateDirection;
	idVec3			teammateVelocity;
	float			teammateDirectionDot;
	float			teammateDistance;
	float			teammateSpeed;
	const idVec3&	myOrigin	= owner->GetPhysics()->GetOrigin();
	const idBounds&	myBounds	= owner->GetPhysics()->GetBounds();
	idVec3			myVelocity	= owner->GetPhysics()->GetLinearVelocity();
	float			mySpeed		= myVelocity.NormalizeFast();

	// Don't Bother, We're Blocked Or Not Moving
	//---------------------------------------------
	if (owner->move.blockTime>gameLocal.GetTime() || !owner->move.fl.moving || mySpeed<1.0f) {
		return;
	}


	for (teammate = aiManager.GetAllyTeam((aiTeam_t)owner->team); teammate; teammate = teammate->teamNode.Next()) {
		if (teammate->fl.hidden || teammate == owner || teammate->health <= 0) {
			continue;
		}

		// If On Same Floor
		//------------------
		teammateDirection	= teammate->GetPhysics()->GetOrigin() - myOrigin;
		if (fabsf(teammateDirection[2])>myBounds.Size().z) {
			continue;
		}

		// If Close Enough
		//-----------------
		teammateDistance	= teammateDirection.NormalizeFast();
		if (teammateDistance>128.0f) {
			continue;
		}

		// Completely Ignore Dudes Directly Behind Me
		//--------------------------------------------
		teammateDirectionDot	= teammateDirection*myVelocity;
		if (teammateDirectionDot<-0.5f) {
			continue;
		}

		// Switch To Walk If I'm Heading For A Teammate
		//----------------------------------------------
		if (teammateDirectionDot>0.85f) {
			owner->move.fl.obstacleInPath = true;	// make him slow to a walk

			if ( owner->DebugFilter(ai_debugMove) ) {	// WHITE = Walk Teammate Near
				gameRenderWorld->DebugArrow( colorWhite, myOrigin, teammate->GetPhysics()->GetOrigin(), 4, 250 );
			}
		}

		if (teammateDistance<48.0f) {

			teammateVelocity	= teammate->GetPhysics()->GetLinearVelocity();
			teammateSpeed		= teammateVelocity.NormalizeFast();

			if (teammateSpeed>50.0f) {

				// If I'm Following Him, And I'm RIGHT Behind Him, Stop And Let Him Go A Bit Farther Ahead
				//------------------------------------------------------------------------------------------
				if (teammateDirectionDot>0.85f && teammateVelocity*myVelocity>0.4f) {
					owner->move.blockTime = gameLocal.time + 1000;	// Set Blocktime Timer
					owner->move.fl.blocked = true;

					if ( owner->DebugFilter(ai_debugMove) ) {	// RED = Stop Teammate Near
						gameRenderWorld->DebugArrow( colorRed, myOrigin, teammate->GetPhysics()->GetOrigin(), 8, 1000);
					}

				}

				// Stop moving if my leader or a guy with a higher entity number is headed straight for me
				//-----------------------------------------------------------------------------------------
				else if (teammate->entityNumber<owner->entityNumber && teammateDirection*teammateVelocity<0.5f) {
					owner->move.blockTime = gameLocal.time + 1500;	// Set Blocktime Timer
					owner->move.fl.blocked = true;

					if ( owner->DebugFilter(ai_debugMove) ) {	// RED = Stop Teammate Near
						gameRenderWorld->DebugArrow( colorRed, myOrigin, teammate->GetPhysics()->GetOrigin(), 8, 1500);
					}
				}
			}
		}
	}
}






/*
=====================
idAI::CheckObstacleAvoidance
=====================
*/
void idAI::CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &seekPos, idReachability* goalReach ) {

	move.fl.blocked				= false;	// Makes Character Stop Moving
	move.fl.obstacleInPath		= false;	// Makes Character Walk
	move.obstacle				= NULL;
	seekPos						= goalPos;

	if (move.fl.ignoreObstacles) {
		return;
	}
	if ( g_perfTest_aiNoObstacleAvoid.GetBool() ) {
		return;
	}

	// Test For Path Around Obstacles
	//--------------------------------
	obstaclePath_t	path;
	move.fl.blocked				= !FindPathAroundObstacles( &physicsObj, aas, move.moveCommand == MOVE_TO_ENEMY ? enemy.ent : NULL, physicsObj.GetOrigin(), goalPos, path );
	move.fl.obstacleInPath		= (path.firstObstacle || path.seekPosObstacle || path.startPosObstacle);
	move.obstacle				= (path.firstObstacle)?(path.firstObstacle):(path.seekPosObstacle);
	seekPos						= path.seekPos;

	// Don't Worry About Obstacles Out Of Walk Range
	//-----------------------------------------------
	if (move.obstacle && DistanceTo(move.obstacle)>155.0f) {
		move.fl.blocked			= false;
		move.fl.obstacleInPath	= false;
		move.obstacle			= 0;
		seekPos					= goalPos;
	}

	// cdr: Alternate Routes Bug
	// If An Obstacle Remains, And The Seek Pos Is Fairly Farr Off Of The Straight Line Path, Then Mark The Reach As Blocked
	//-----------------------------------------------------------------------------------------------------------------------
	if (move.fl.obstacleInPath && goalReach && !(goalReach->travelType&TFL_INVALID) && seekPos.Dist2XY(goalPos)>100.0f) {
		float scale;
		float dist;

		dist = seekPos.DistToLineSeg(physicsObj.GetOrigin(), goalPos, scale);
		if (scale<0.95f && dist>50.0f) {
			aiManager.MarkReachBlocked(aas, goalReach, path.allObstacles);
		}
 	}

	TestTeammateCollisions(this);



	if ( DebugFilter(ai_showObstacleAvoidance) ) {
		gameRenderWorld->DebugLine( colorBlue, goalPos + idVec3( 1.0f, 1.0f, 0.0f ), goalPos + idVec3( 1.0f, 1.0f, 64.0f ), gameLocal.msec );
		gameRenderWorld->DebugLine( !move.fl.blocked ? colorYellow : colorRed, path.seekPos, path.seekPos + idVec3( 0.0f, 0.0f, 64.0f ), gameLocal.msec );
	}
}

/*
============
idAI::TestAnimMove
============
*/
bool idAI::TestAnimMove ( int animNum, idEntity *ignore, idVec3 *pMoveVec ) {
	const idAnim*	anim;
	predictedPath_t path;
	idVec3			moveVec;
	
	anim = GetAnimator()->GetAnim ( animNum );
	assert ( anim );

//	moveVec = anim->TotalMovementDelta() * idAngles( 0.0f, move.ideal_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	moveVec = anim->TotalMovementDelta() * idAngles( 0.0f, move.current_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	idAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec / MS2SEC( anim->Length() ), anim->Length(), 200, SE_BLOCKED | SE_ENTER_LEDGE_AREA, path, ignore );

	if ( DebugFilter(ai_debugMove) ) { // TestAnimMove
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, anim->Length() );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, anim->Length() );
	}
	if ( pMoveVec ) {
		*pMoveVec = moveVec;
	}

	return ( path.endEvent == 0 );
}

/*
=====================
Seek
=====================
*/
idVec3 Seek( idVec3 &vel, const idVec3 &org, const idVec3 &goal, float prediction ) {
	idVec3 predictedPos;
	idVec3 goalDelta;
	idVec3 seekVel;

	// predict our position
	predictedPos = org + vel * prediction;
	goalDelta = goal - predictedPos;
//	goalDelta = goal - org;
	seekVel = goalDelta * MS2SEC( gameLocal.msec );

	return seekVel;
}

/***********************************************************************

	Movement

***********************************************************************/

/*
=====================
idAI::DeadMove
=====================
*/
void idAI::DeadMove( void ) {
	idVec3				delta;
	monsterMoveResult_t	moveResult;

	DeathPush ( );

	idVec3 org = physicsObj.GetOrigin();

	GetAnimMoveDelta( viewAxis, viewAxis, delta );
	physicsObj.SetDelta( delta );

	RunPhysics();

	moveResult = physicsObj.GetMoveResult();
	move.fl.onGround = physicsObj.OnGround();
}

/*
=====================
idAI::AdjustFlyingAngles
=====================
*/
void idAI::AdjustFlyingAngles( void ) {
	idVec3	vel;
	float 	speed;
	float 	roll;
	float 	pitch;

	vel = physicsObj.GetLinearVelocity();

	speed = vel.Length();
	if ( speed < 5.0f ) {
		roll = 0.0f;
		pitch = 0.0f;
	} else {
		roll = vel * viewAxis[ 1 ] * -move.fly_roll_scale / move.fly_speed;
		if ( roll > move.fly_roll_max ) {
			roll = move.fly_roll_max;
		} else if ( roll < -move.fly_roll_max ) {
			roll = -move.fly_roll_max;
		}

		pitch = vel * viewAxis[ 0 ] * -move.fly_pitch_scale / move.fly_speed;
		if ( pitch > move.fly_pitch_max ) {
			pitch = move.fly_pitch_max;
		} else if ( pitch < -move.fly_pitch_max ) {
			pitch = -move.fly_pitch_max;
		}
	}

	move.fly_roll = move.fly_roll * 0.95f + roll * 0.05f;
	move.fly_pitch = move.fly_pitch * 0.95f + pitch * 0.05f;

	if ( move.flyTiltJoint != INVALID_JOINT ) {
		animator.SetJointAxis( move.flyTiltJoint, JOINTMOD_WORLD, idAngles( move.fly_pitch, 0.0f, move.fly_roll ).ToMat3() );
	} else {
		viewAxis = idAngles( move.fly_pitch, move.current_yaw, move.fly_roll ).ToMat3();
	}
}

/*
=====================
idAI::AddFlyBob
=====================
*/
void idAI::AddFlyBob( idVec3 &vel ) {
	idVec3	fly_bob_add;
	float	t;

	if ( move.fly_bob_strength ) {
		t = MS2SEC( gameLocal.time + entityNumber * 497 );
		fly_bob_add = ( viewAxis[ 1 ] * idMath::Sin16( t * move.fly_bob_horz ) + viewAxis[ 2 ] * idMath::Sin16( t * move.fly_bob_vert ) ) * move.fly_bob_strength;
		vel += fly_bob_add * MS2SEC( gameLocal.msec );
		if ( DebugFilter(ai_debugMove) ) { // FlyBob
			const idVec3 &origin = physicsObj.GetOrigin();
			gameRenderWorld->DebugArrow( colorOrange, origin, origin + fly_bob_add, 0 );
		}
	}
}

/*
=====================
idAI::AdjustFlyHeight
=====================
*/
void idAI::AdjustFlyHeight( idVec3 &vel, const idVec3 &goalPos ) {
	const idVec3	&origin = physicsObj.GetOrigin();
	predictedPath_t path;
	idVec3			end;
	idVec3			dest;
	trace_t			trace;
	bool			goLower;

	// make sure we're not flying too high to get through doors
	// FIXME: with move to enemy this thinks we're going to hit
	//			the enemy and then says we need to go lower... but that's not quite right...
	//			if we're about to hit our enemy, then maintaining our height should be desirable..?
	goLower = false;
	if ( origin.z > goalPos.z ) {
		dest = goalPos;
		dest.z = origin.z + 128.0f;
		idAI::PredictPath( this, aas, goalPos, dest - origin, 1000, 1000, SE_BLOCKED, path, move.goalEntity.GetEntity() );
		if ( path.endPos.z < origin.z ) {

			//Hmm, should we make sure the path.endPos is high enough off the ground?
			if ( move.fly_offset && (move.moveCommand == MOVE_TO_ENEMY || move.moveCommand == MOVE_TO_ATTACK) ) {
				int pathArea = PointReachableAreaNum( path.endPos );
				if ( aas && (aas->AreaFlags( pathArea )&AREA_FLOOR) ) {
					path.endPos.z = aas->AreaBounds( pathArea )[0][2];
					float areaTop = aas->AreaCeiling( pathArea ) - GetPhysics()->GetBounds()[1].z;
					if ( path.endPos.z + move.fly_offset > areaTop ) {
						path.endPos.z = areaTop;
					} else {
						path.endPos.z += move.fly_offset;
					}
				}
			}

			idVec3 addVel = Seek( vel, origin, path.endPos, AI_SEEK_PREDICTION );
			vel.z += addVel.z;
			goLower = true;
		}
        
		if ( DebugFilter(ai_debugMove) ) { // Fly Height
			gameRenderWorld->DebugBounds( goLower ? colorRed : colorGreen, physicsObj.GetBounds(), path.endPos, gameLocal.msec );
		}
	}

	if ( !goLower ) {
		// make sure we don't fly too low
		end = origin;
		if ( move.moveCommand == MOVE_TO_ENEMY ) {
			end.z = enemy.lastKnownPosition.z + move.fly_offset;
		} else if ( move.moveCommand == MOVE_TO_ENTITY ) {
			end.z = goalPos.z;
		} else {
			end.z = goalPos.z;// + move.fly_offset;
		}

// RAVEN BEGIN
// ddynerman: multiple collision world
		idVec3 cappedEnd = (end-origin);
		if ( cappedEnd.LengthFast() > 1024.0f ) {
			//don't do translation predictions over 1024
			cappedEnd.Normalize();
			cappedEnd *= 1024.0f;
		}
		cappedEnd += origin;
		gameLocal.Translation( this, trace, origin, cappedEnd, physicsObj.GetClipModel(), mat3_identity, MASK_MONSTERSOLID, this );
// RAVEN END
		vel += Seek( vel, origin, trace.endpos, AI_SEEK_PREDICTION );
	}
}

/*
=====================
idAI::FlySeekGoal
=====================
*/
void idAI::FlySeekGoal( idVec3 &vel, idVec3 &goalPos ) {
	idVec3 seekVel;
	
	// seek the goal position
	seekVel = Seek( vel, physicsObj.GetOrigin(), goalPos, AI_SEEK_PREDICTION );
	seekVel *= move.fly_seek_scale;
	//seekVel.Normalize();
	//vel = seekVel*move.speed;
	vel += seekVel;
}

/*
=====================
idAI::AdjustFlySpeed
=====================
*/
void idAI::AdjustFlySpeed( idVec3 &vel ) {
	float goalSpeed;

	// Slow down movespeed when we close to goal (this is similar to how AnimMove ai will 
	// switch to walking when they are close, it allows for more fine control of movement)
	if ( move.walkRange > 0.0f ) {
		float distSqr;
		distSqr	  = (physicsObj.GetOrigin ( ) - move.moveDest).LengthSqr ( );
		goalSpeed = move.speed * idMath::ClampFloat ( 0.1f, 1.0f, distSqr / Square ( move.walkRange ) );		
	} else {
		goalSpeed = move.speed;
	}

	// apply dampening as long as we arent within the walk range
//	if ( goalSpeed != move.speed) {
		float speed;

		vel -= vel * AI_FLY_DAMPENING * MS2SEC( gameLocal.msec );

		// gradually speed up/slow down to desired speed
		speed = vel.Normalize();
		speed += ( goalSpeed - speed ) * MS2SEC( gameLocal.msec );
		if ( speed < 0.0f ) {
			speed = 0.0f;
		} else if (goalSpeed && ( speed > goalSpeed ) ) {
			speed = goalSpeed;
		}

		vel *= speed;
//	} else {
//		vel.Normalize ( );
//		vel *= goalSpeed;
//	}
}

/*
=====================
idAI::FlyTurn
=====================
*/
void idAI::FlyTurn( void ) {
	if ( move.moveCommand == MOVE_FACE_ENEMY || ForceFaceEnemy() ) {
		TurnToward( enemy.lastKnownPosition );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( focusType != AIFOCUS_NONE && move.fl.allowDirectional ) {
		DirectionalTurnToward ( currentFocusPos );
	} else if ( move.speed > 0.0f ) {
		const idVec3 &vel = physicsObj.GetLinearVelocity();
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			TurnToward( vel.ToYaw() );
		}
	}
	
	Turn();
}

/*
=====================
idAI::FlyMove
=====================
*/
void idAI::FlyMove( void ) {
	idVec3	oldorigin;
	idVec3	newDest;

	move.fl.blocked = false;
	if ( ( move.moveCommand >= NUM_NONMOVING_COMMANDS ) && ReachedPos( move.moveDest, move.moveCommand, move.range ) ) {
		StopMove( MOVE_STATUS_DONE );
	}

	if ( DebugFilter(ai_debugMove) ) { // Fly Move
		gameLocal.Printf( "%d: %s: %s, vel = %.2f, sp = %.2f, maxsp = %.2f\n", gameLocal.time, name.c_str(), aiMoveCommandString[ move.moveCommand ], physicsObj.GetLinearVelocity().Length(), move.speed, move.fly_speed );
	}

	// Dont move when movement is disabled
	if ( !CanMove() || legsAnim.Disabled ( ) ) {
		// Still allow turning though
		FlyTurn ( );

		if ( (aifl.action || aifl.scripted ) && legsAnim.Disabled () && move.fl.allowAnimMove ) {
			idMat3 oldaxis = viewAxis;
			idVec3 delta;
			GetAnimMoveDelta( oldaxis, viewAxis, delta );
			physicsObj.UseFlyMove( false );
			if ( spawnArgs.GetBool( "alwaysBob" ) ) {
				AddFlyBob( delta );
			}
			physicsObj.SetDelta( delta );
			physicsObj.ForceDeltaMove( true );

			RunPhysics();
		} else if ( spawnArgs.GetBool( "alwaysBob" ) ) {
			idVec3 vel = physicsObj.GetLinearVelocity();
			AddFlyBob( vel );
			physicsObj.SetLinearVelocity( vel );

			// run the physics for this frame
			oldorigin = physicsObj.GetOrigin();
			physicsObj.UseFlyMove( true );
			physicsObj.UseVelocityMove( false );
			physicsObj.SetDelta( vec3_zero );
			physicsObj.ForceDeltaMove( move.fl.noGravity );
			RunPhysics();
		}

		UpdateAnimationControllers ( );
		return;
	}

	if ( move.moveCommand != MOVE_TO_POSITION_DIRECT ) {
		idVec3 vel = physicsObj.GetLinearVelocity();

		if ( GetMovePos( move.seekPos ) ) {
			CheckObstacleAvoidance( move.seekPos, newDest );
			move.seekPos.x = newDest.x;
			move.seekPos.y = newDest.y;
		}

		if ( move.speed	) {
			FlySeekGoal( vel, move.seekPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if ( ( move.moveCommand != MOVE_TO_POSITION ) ) {
			AdjustFlyHeight( vel, move.seekPos );
		}

		AdjustFlySpeed( vel );

		vel += move.addVelocity;
		move.addVelocity.Zero();

		physicsObj.SetLinearVelocity( vel );
	}

	// turn
	FlyTurn();

	// run the physics for this frame
	oldorigin = physicsObj.GetOrigin();
	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( move.fl.noGravity );
	RunPhysics();

	monsterMoveResult_t	moveResult = physicsObj.GetMoveResult();
	idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
	if ( blockEnt && blockEnt->IsType( idMoveable::GetClassType() ) && blockEnt->GetPhysics()->IsPushable() ) {
		KickObstacles( viewAxis[ 0 ], move.kickForce, blockEnt );
	} else if ( moveResult == MM_BLOCKED ) {
		move.blockTime = gameLocal.time + 500;
		move.fl.blocked = true;
	}

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( DebugFilter(ai_debugMove) ) { // Fly Move
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 4000 );
		gameRenderWorld->DebugBounds( colorOrange, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, org, org + physicsObj.GetLinearVelocity(), gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorBlue, org, move.seekPos, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, GetEyePosition ( ), GetEyePosition ( ) + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

/*
============
idAI::UpdatePlayback
============
*/
void idAI::UpdatePlayback ( idVec3 &goalPos, idVec3 &delta, idVec3 &oldorigin, idMat3 &oldaxis ) {
	rvDeclPlaybackData	pbd;
	bool				atDest;
	
	// New playback stuff
	if( !mPlayback.IsActive() ) {
		return;
	}
			
	atDest = mPlayback.UpdateFrame( this, pbd );

	goalPos = pbd.GetPosition();
	SetOrigin( goalPos );
	viewAxis = pbd.GetAngles().ToMat3();

	// Keep the yaw updated
	idVec3 local_dir;
	physicsObj.GetGravityAxis().ProjectVector( viewAxis[ 0 ], local_dir );
	move.current_yaw		= local_dir.ToYaw();
	move.ideal_yaw		= idMath::AngleNormalize180( move.current_yaw );	

	OnUpdatePlayback ( pbd );
}

/*
============
idAI::
============
*/

void idAI::PlaybackMove( void ){
	idVec3				goalPos;
	idVec3				delta;
	idVec3				goalDelta;
	monsterMoveResult_t	moveResult;
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	move.fl.blocked = false;

	move.obstacle = NULL;

	goalPos = oldorigin;

	UpdatePlayback( goalPos, delta, oldorigin, oldaxis );

	if ( DebugFilter(ai_debugMove) ) { // Playback Move
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetLinearVelocity( vec3_zero );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( true );
	RunPhysics();

	moveResult = physicsObj.GetMoveResult();
	idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( blockEnt && blockEnt->IsType( idMoveable::GetClassType() ) && blockEnt->GetPhysics()->IsPushable() ) {
// RAVEN END
		KickObstacles( viewAxis[ 0 ], move.kickForce, blockEnt );
	} else {
		move.fl.blocked = true;
	}

	BlockedFailSafe();

	move.fl.onGround = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( DebugFilter(ai_debugMove) ) { // Playback Move
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest );
		gameRenderWorld->DebugLine( colorYellow, GetEyePosition(), GetEyePosition() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

/*
=====================
idAI::StaticMove
=====================
*/
void idAI::StaticMove( void ) {
	idEntity* enemyEnt = enemy.ent;

	if ( aifl.dead ) {
		return;
	}

	if ( ( move.moveCommand == MOVE_FACE_ENEMY || ForceFaceEnemy() ) && enemyEnt ) {
		TurnToward( enemy.lastKnownPosition );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.moveCommand != MOVE_NONE ) {
		TurnToward( move.moveDest );
	}
	Turn();

	physicsObj.ForceDeltaMove( true ); // disable gravity
	RunPhysics();

	move.fl.onGround = false;

	if ( DebugFilter(ai_debugMove) ) { // Static Move
		const idVec3 &org = physicsObj.GetOrigin();
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugLine( colorBlue, org, move.moveDest, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, GetEyePosition(), GetEyePosition() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
	}
}

/*
=====================
idAI::SlideMove
=====================
*/
void idAI::SlideMove( void ) {
	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	monsterMoveResult_t	moveResult;
	idVec3				newDest;

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	move.fl.blocked = false;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){ 
		move.lastMoveOrigin.Zero();
		move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( ( move.moveCommand == MOVE_FACE_ENEMY || ForceFaceEnemy() ) && enemy.ent ) {
		TurnToward( enemy.lastKnownPosition );
		move.seekPos = move.moveDest;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		move.seekPos = move.moveDest;
	} else if ( GetMovePos( move.seekPos ) ) {
		CheckObstacleAvoidance( move.seekPos, newDest );
		TurnToward( newDest );
		move.seekPos = newDest;
	}

	// FIXME: this stuff should really move to GetMovePos (Steering)
	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			move.seekPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
		} else {
			move.seekPos = move.moveDest;
			move.fl.allowAnimMove = true;
			move.fl.allowPrevAnimMove = false;
			StopMove( MOVE_STATUS_DONE );
		}
	}

	if ( move.moveCommand == MOVE_TO_POSITION ) {
		goalDelta = move.moveDest - oldorigin;
		goalDist = goalDelta.LengthFast();
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}

	idVec3 vel = physicsObj.GetLinearVelocity();
	float z = vel.z;
	idVec3  predictedPos = oldorigin + vel * AI_SEEK_PREDICTION;

	// seek the goal position
	goalDelta = move.seekPos - predictedPos;
	vel -= vel * AI_FLY_DAMPENING * MS2SEC( gameLocal.msec );
	vel += goalDelta * MS2SEC( gameLocal.msec );

	// cap our speed
	vel.Truncate( move.fly_speed );
	vel.z = z;
	physicsObj.SetLinearVelocity( vel );
	physicsObj.UseVelocityMove( true );
	RunPhysics();

	if ( ( move.moveCommand == MOVE_FACE_ENEMY || ForceFaceEnemy()  ) && enemy.ent ) {
		TurnToward( enemy.lastKnownPosition );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else {
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			TurnToward( vel.ToYaw() );
		}
	}
	Turn();

	if ( DebugFilter(ai_debugMove) ) { // Slide Move
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 5000 );
	}

	moveResult = physicsObj.GetMoveResult();
	idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( blockEnt && blockEnt->IsType( idMoveable::GetClassType() ) && blockEnt->GetPhysics()->IsPushable() ) {
// RAVEN END
		KickObstacles( viewAxis[ 0 ], move.kickForce, blockEnt );
	}

	BlockedFailSafe();

	move.fl.onGround = physicsObj.OnGround();

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( DebugFilter(ai_debugMove) ) { // SlideMove
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorYellow, GetEyePosition(), GetEyePosition() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

/*
=====================
idAI::AnimMove
=====================
*/
void idAI::AnimMove( void ) {

	if ( ai_useRVMasterMove.GetBool ( ) ) {
		RVMasterMove();
		return;
	}

	idVec3				delta;
	idVec3				goalDelta;
	float				goalDist;
	monsterMoveResult_t	moveResult;
	idVec3				newDest;
	// RAVEN BEGIN
	// cdr: Alternate Routes Bug
	idReachability*		goalReach;
	// RAVEN END

	idVec3 oldorigin = physicsObj.GetOrigin();
	idMat3 oldaxis = viewAxis;

	if ( move.moveCommand < NUM_NONMOVING_COMMANDS ){ 
		move.lastMoveOrigin.Zero();
		//move.lastMoveTime = gameLocal.time;
	}

	move.obstacle = NULL;
	if ( move.moveCommand == MOVE_FACE_ENEMY && enemy.ent ) {
		TurnToward( enemy.lastKnownPosition );
		move.goalPos = oldorigin;
		move.seekPos = oldorigin;
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
		move.goalPos = oldorigin;
		move.seekPos = oldorigin;
	} else if ( move.moveCommand >= NUM_NONMOVING_COMMANDS ) {
		if ( ReachedPos( move.moveDest, move.moveCommand, move.range ) ) {
			StopMove( MOVE_STATUS_DONE );
		} else { 
			move.moveStatus = MOVE_STATUS_MOVING;

			// Otherwise, Update The Seek Pos
			if ( !aifl.simpleThink && GetMovePos( move.goalPos, &goalReach ) ) {
				if ( move.moveCommand != MOVE_WANDER ) {
					CheckObstacleAvoidance( move.goalPos, move.seekPos, goalReach );
				} else {
					move.seekPos = move.goalPos;
				}
				DirectionalTurnToward ( move.seekPos );
			}
		}
	}

	Turn();

	goalDelta = move.seekPos - oldorigin;
	goalDist = goalDelta.LengthFast();

	// FIXME: this stuff should really move to GetMovePos (Steering)
	if ( move.moveCommand == MOVE_SLIDE_TO_POSITION ) {
		if ( gameLocal.time < move.startTime + move.duration ) {
			move.goalPos = move.moveDest - move.moveDir * MS2SEC( move.startTime + move.duration - gameLocal.time );
			delta = move.goalPos - oldorigin;
			delta.z = 0.0f;
		} else {
			delta = move.moveDest - oldorigin;
			delta.z = 0.0f;
			move.fl.allowAnimMove = true;
			move.fl.allowPrevAnimMove = false;
			StopMove( MOVE_STATUS_DONE );
		}
	} else if ( move.fl.allowAnimMove ) {
		GetAnimMoveDelta( oldaxis, viewAxis, delta );
	} else if ( move.fl.allowPrevAnimMove ) {
		GetAnimMoveDelta( oldaxis, viewAxis, delta );
		float speed = delta.LengthFast();
		delta = goalDelta;
		delta.Normalize();
		delta *= speed;
	} else {
		delta.Zero();
	}

	if ( move.moveCommand > NUM_NONMOVING_COMMANDS ) { 
		//actually *trying* to move to a goal
		if ( goalDist < delta.LengthFast() ) {
			delta = goalDelta;
		}
	}

	physicsObj.SetDelta( delta );
	physicsObj.ForceDeltaMove( move.fl.noGravity );

	RunPhysics();

	
	moveResult = physicsObj.GetMoveResult();
	idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( blockEnt && blockEnt->IsType( idMoveable::GetClassType() ) && blockEnt->GetPhysics()->IsPushable() ) {
// RAVEN END
		KickObstacles( viewAxis[ 0 ], move.kickForce, blockEnt );
	}

	BlockedFailSafe();

	move.fl.onGround = physicsObj.OnGround();

	const idVec3& org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( DebugFilter(ai_debugMove) ) { // AnimMove : GREEN / RED Bounds & Move Dest
		gameRenderWorld->DebugLine(		colorCyan, oldorigin, org, 5000 );
		gameRenderWorld->DebugBounds(	(team==0)?(colorGreen):(colorRed), physicsObj.GetBounds(), org, gameLocal.msec );
		if (!ReachedPos( move.moveDest, move.moveCommand, move.range )) {
			gameRenderWorld->DebugBounds(	(team==0)?(colorGreen):(colorRed), physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
			gameRenderWorld->DebugArrow(	(team==0)?(colorGreen):(colorRed), org, move.moveDest, 4, gameLocal.msec );
		}
		DrawRoute();
	}
}

/*
=====================
idAI::CustomMove
=====================
*/
void idAI::CustomMove( void ) {
	// derived class must implement this
}



/*
======================================================================================
									NEW MOVE CODE BEGIN
======================================================================================
*/

struct rvObstacle;

const float		REACHED_RADIUS			= 4.0f;
const float		REACHED_RADIUS_SQUARE	= REACHED_RADIUS*REACHED_RADIUS;






/*
=====================
LineIntersection2D
=====================
*/	
bool  LineIntersection2D(const idVec3& A, const idVec3& B, const idVec3& C, const idVec3& D, idVec3& contactPoint) {

	// Test If Parallel
	//------------------
	float q = (((B.x-A.x)*(D.y-C.y))-((B.y-A.y)*(D.x-C.x)));
	if (fabsf(q)<0.01f) {
		return false;
	}

	// Test CD Edge
	//--------------
	float s = (((A.y-C.y)*(B.x-A.x))-((A.x-C.x)*(B.y-A.y))) / q;
	if (s<0.0f) {
		return false;
	}
	if (s>1.0f) {
		return false;
	}

	// Test AB Edge
	//--------------
	float r = (((A.y-C.y)*(D.x-C.x))-((A.x-C.x)*(D.y-C.y))) / q;
	if (r<0.0f) {
		return false;
	}
	if (r>1.0f) {
		return false;
	}

	contactPoint = A + r*(B-A);
	return true;
}


/*
=====================
rvWindingBox

Clockwise winding with a simple line intersection test

[1]---->[2]
 ^       |
 | verts |
 |       V
[0]<----[3]

=====================
*/
struct rvWindingBox {
	idVec3				verts[4];
	aasArea_t*			areas[4];
	rvObstacle*			obstacles[4];	// CDR_TODO: Should be a list of obstacles for each vertex


	/*
	=====================
	Initialize
	=====================
	*/	
	void Initialize() {
		for (int i=0; i<4; i++) {
			verts[i]		= vec3_zero;
			areas[i]		= NULL;
			obstacles[i]	= NULL;
		}
	}

	/*
	=====================
	FromBounds
	=====================
	*/	
	void FromBounds(const idBounds& b) {
		verts[0].x	= b[0].x;
		verts[0].y	= b[0].y;
		verts[0].z	= b[0].z;

		verts[1].x	= b[0].x;
		verts[1].y	= b[1].y;
		verts[1].z	= b[0].z;

		verts[2].x	= b[1].x;
		verts[2].y	= b[1].y;
		verts[2].z	= b[0].z;

		verts[3].x	= b[1].x;
		verts[3].y	= b[0].y;
		verts[3].z	= b[0].z;
	}

	/*
	=====================
	LineIntersection
	=====================
	*/	
	bool  LineIntersection(const idVec3& start, const idVec3& end, idVec3& contactPoint, int& v1, int& v2) const  {
		for (int i=0; i<4; i++) {
			v1 = i;
			v2 = (i<3)?(i+1):(0);

			if (start.IsLeftOf(verts[v1], verts[v2]) && LineIntersection2D(start, end, verts[v1], verts[v2], contactPoint)) {
				return true;
			}
		}
		return false;
	}

	/*
	=====================
	PointInside
	=====================
	*/	
	bool PointInside(const idVec3& point) const {
		for (int i=0; i<4; i++) {
			const idVec3& vert1 = verts[i];
			const idVec3& vert2 = verts[(i<3)?(i+1):(0)];

			if (point.IsLeftOf(vert1, vert2)) {
				return false;
			}
		}
		return true;
	}

	/*
	=====================
	DrawDebugGraphics
	=====================
	*/	
	bool DrawDebugGraphics() const  {
		for (int i=0; i<4; i++) {
			const idVec3& vert1 = verts[i];
			const idVec3& vert2 = verts[(i<3)?(i+1):(0)];

			gameRenderWorld->DebugLine(colorYellow, vert1, vert2, gameLocal.msec);
			if (!areas[i] || obstacles[i]) {
				gameRenderWorld->DebugLine(colorRed, vert1, vert1+idVec3(0.0f,0.0f,16.0f), gameLocal.msec);
			} else if (areas[i]) {
			//	gameRenderWorld->DebugLine(colorYellow, vert1, areas[i]->center, gameLocal.msec);
			}
		}
		return true;
	}
};




/*
=====================
rvMarker

A marker represents an obstacle within 
an area.  Any single obstacle can have
any number of markers in any number of
areas that it touches
=====================
*/
struct rvMarker {
	rvObstacle*			obstacle;
	aasArea_t*			area;
	rvMarker*			prev;
	rvMarker*			next;
};
rvPool<rvMarker, 255> markerPool;

/*
=====================
rvObstacle
=====================
*/
struct rvObstacle {
	static idAASFile*	searchFile;
	static idBounds		searchBounds;

	entityPointer_t		entity;
	idVec3				origin;
	idVec3				originFuture;
	int					lastTimeMoved;
	bool				pendingUpdate;

	rvWindingBox		windings[3];
	idList<rvMarker*>	markers;
	idList<aasArea_t*>	areas;

	/*
	============
	Initialize
	============
	*/
	void Initialize(idEntity* ent) {
		entity			= ent;
		origin			= vec3_zero;
		originFuture	= vec3_zero;
		lastTimeMoved	= 0;
		pendingUpdate	= true;

		for (int i=0; i<3; i++) {
			windings[i].Initialize();
		}
		markers.Clear();
		areas.Clear();
	}

	/*
	============
	DrawDebugGraphics
	============
	*/
	void DrawDebugGraphics() {
 		if ((gameLocal.time - lastTimeMoved) < 1000) {
			gameRenderWorld->DebugArrow(colorBrown, origin, originFuture, 3, gameLocal.msec);
		}

		windings[0].DrawDebugGraphics();
	}

	/*
	============
	RemoveMarkers
	============
	*/
	void RemoveMarkers() {
		static rvMarker*	marker;

		for (int i=0; i<markers.Num(); i++) {
			marker = markers[i];
			assert(marker->obstacle==this);

			if (marker->area->firstMarker==marker) {
				marker->area->firstMarker = marker->next;
			}
			if (marker->prev) {
				marker->prev->next = marker->next;
			}
			if (marker->next) {
				marker->next->prev = marker->prev;
			}

			marker->obstacle	= NULL;

			markerPool.free(marker);
		}
		markers.Clear();
	}
	/*
	============
	SearchAreas_r
	============
	*/
	void SearchAreas_r( int nodeNum ) {
		int side;
		const aasNode_t *node;

		while( nodeNum != 0 ) {

			// Negative nodeNum signifies a Leaf Area
			if ( nodeNum < 0 ) {
				if ( searchFile->GetArea(-nodeNum).flags&AREA_REACHABLE_WALK ) {
					areas.AddUnique( &searchFile->GetArea(-nodeNum) );
				}
				break;
			}
			node = &searchFile->GetNode( nodeNum );
			side =  searchBounds.PlaneSide( searchFile->GetPlane( node->planeNum ) );
			if ( side == PLANESIDE_BACK ) {
				nodeNum = node->children[1];
			}
			else if ( side == PLANESIDE_FRONT ) {
				nodeNum = node->children[0];
			}
			else {
				SearchAreas_r( node->children[1] );
				nodeNum = node->children[0];
			}
		}
	}

	/*
	============
	PointInsideArea
	============
	*/
	bool PointInsideArea( const idVec3& point, aasArea_t* area ) {
		int i, faceNum;

		for ( i = 0; i < area->numFaces; i++ ) {
			faceNum = searchFile->GetFaceIndex(area->firstFace + i);

			const aasFace_t& face = searchFile->GetFace(abs( faceNum ));
			if (!(face.flags & FACE_FLOOR)) {
				const idPlane& plane = searchFile->GetPlane(face.planeNum ^ INTSIGNBITSET( faceNum ));
				if (plane.Side(point) == PLANESIDE_BACK) {
					return false;
				}
			}
		}
		return true;
	}

	/*
	============
	AddMarkers
	============
	*/
	void AddMarkers() {
		static rvMarker*	marker;

		for (int i=0; i<areas.Num() && !markerPool.full(); i++) {

			// Allocate A Marker
			//-------------------
			marker	= markerPool.alloc();
		//	assert(marker->obstacle==NULL);

			// Setup The New Marker
			//----------------------
			marker->obstacle	= this;
			marker->area		= areas[i];
			marker->next		= NULL;
			marker->prev		= NULL;


			// Fixup Any Existing Linked List (First Marker)
			//-----------------------------------------------
			if (marker->area->firstMarker) {
				marker->area->firstMarker->prev = marker;
				marker->next			= marker->area->firstMarker;
			}

			// Add This Marker At THe Area's Head
			//------------------------------------
			marker->area->firstMarker	= marker;

			markers.Append(marker);
		}
	}



	/*
	============
	Update
	============
	*/
	bool Update() {
		static rvMarker*			marker;
		static rvObstacle*			obstacle;
		static aasArea_t*			area;
		static idVec3				expand;
		static float				speed;
		static float				distance;
		static idVec3				direction;
		static int					aasFileNum, t, v, a;
		static idList<rvObstacle*>	touched;
		static rvWindingBox*		myWinding;
		

		pendingUpdate = false;
		idEntity* ent = entity.GetEntity();
		if (!ent || ent->health<=0 || ent->fl.hidden || !ent->GetPhysics()) {
			RemoveMarkers();
			return false;	// Means This Obstacle Structure Should Be Retired
		}


		// Only Update If We've Moved Far Enough
		//---------------------------------------
		idPhysics* physics = ent->GetPhysics();
		if (origin.Dist2XY(physics->GetOrigin())<100.0f) {	// CDR_TODO: Scale This By Distance To Player, Other Obstacles Near...
			return true;
		}

		// Get The Obstacle's Seek Direction & Speed
		//-------------------------------------------
		if (ent->IsType(idAI::GetClassType())) {
			speed				= (((idAI*)ent)->move.fl.done)?(0.0f):(physics->GetLinearVelocity().LengthFast()) * 2.5f;
			direction			= (((idAI*)ent)->move.seekPos - physics->GetOrigin());
			distance			= direction.NormalizeFast();

			// Cap The Projection To The Seek Position
			if (speed > distance) {
				speed = distance;
			}
		} else {
			direction			= physics->GetLinearVelocity();
			speed				= direction.NormalizeFast() * 2.5f;
		}

		origin			= physics->GetOrigin();
		originFuture	= origin + (direction*speed);		// 2.5 seconds into the future predicion
		lastTimeMoved	= gameLocal.time;


		// Remove Old Markers
		//--------------------
		RemoveMarkers();


		// Get The Entity's Bounds And Areas That These Bounds Cover
		//-----------------------------------------------------------
		for (aasFileNum=0; aasFileNum<gameLocal.GetNumAAS() && aasFileNum<3; aasFileNum++) {
			if (!gameLocal.GetAAS(aasFileNum)) {
				continue;
			}

			areas.Clear();
			touched.Clear();
	
			myWinding		= &windings[aasFileNum];
			searchFile		= gameLocal.GetAAS(aasFileNum)->GetFile();
			searchBounds	= physics->GetAbsBounds();
			expand			= searchFile->GetSettings().boundingBoxes[0].Size();
			expand[0]		*= 0.5f;
			expand[1]		*= 0.5f;
			expand[2]		= 0;
			expand[0]		+= REACHED_RADIUS;
			expand[1]		+= REACHED_RADIUS;

			searchBounds.ExpandSelf(expand);
			SearchAreas_r(1);

			// Setup The Winding From The Bounds
			//-----------------------------------
			myWinding->FromBounds(searchBounds);

			// Setup Each Vertex On The Winding
			//----------------------------------
			for (v=0; v<4; v++) {
				const idVec3& myVertex = myWinding->verts[v];

				myWinding->areas[v] = NULL;
				for (a=0; a<areas.Num(); a++) {
					aasArea_t* area = areas[a];
					if (PointInsideArea(myVertex, area)) {
						myWinding->areas[v] = area;


						// Search This Area For All Obstacles That May Contain This Vertex
						//-----------------------------------------------------------------
						obstacle = NULL;
						for (marker=area->firstMarker; marker; marker=marker->next) {
							if (marker->obstacle->windings[aasFileNum].PointInside(myVertex)) {
								obstacle = marker->obstacle;
								break;
							}
						}

						// Update The Obstacle On The Winding
						//------------------------------------
						if (myWinding->obstacles[v] && myWinding->obstacles[v]!=obstacle) {
							touched.AddUnique(myWinding->obstacles[v]);
						}
						if (obstacle) {
							touched.AddUnique(obstacle);
						}

						myWinding->obstacles[v] = obstacle;
						break;
					}
				}
			}

			// Touched Obstacles Need To Test Their Verts Against This Winding
			//-----------------------------------------------------------------
			for (t=0; t<touched.Num(); t++) {
				obstacle = touched[t];
				for (v=0; v<4; v++) {
					if (myWinding->PointInside(obstacle->windings[aasFileNum].verts[v])) {
						obstacle->windings[aasFileNum].obstacles[v] = this;
					} else if (obstacle->windings[aasFileNum].obstacles[v]==this) {
						obstacle->windings[aasFileNum].obstacles[v] = NULL;
					}
				}
			}

			// AddMarkers In Each Found Area
			//-------------------------------
			AddMarkers();
		}

		return true;
	}

	/*
	=====================
	VertexValid
	=====================
	*/	
	static bool VertexValid(const rvWindingBox& bounds, int v, const aasArea_t* inArea, const idEntity* ignore)  {
		return (bounds.areas[v] && (bounds.obstacles[v]==NULL || bounds.obstacles[v]->entity.GetEntity()==ignore));
	}

	/*
	============
	MovedRecently
	============
	*/
	bool MovedRecently(int time=800) {
 		if ((gameLocal.time - lastTimeMoved) < time) {
			return true;
		}
	//	idEntity* ent = entity.GetEntity();
	//	if (ent && ent->IsType(idAI::GetClassType())) {
	//		return !((idAI*)ent)->move.fl.done;
	//	}
		return false;
	}
};
rvIndexPool<rvObstacle, 50, MAX_GENTITIES> obstaclePool;

idAASFile*		rvObstacle::searchFile;
idBounds		rvObstacle::searchBounds;






/*
=====================
rvObstacleFinder
=====================
*/
struct rvObstacleFinder {
	idList<rvObstacle*>	obstaclesPendingUpdate;
	int					obstaclesUpdateTime;


	struct traceResult_t {
		rvObstacle*		obstacle;
		idVec3			endPoint;
		float			distance;
		int				v1;
		int				v2;
		bool			v1Valid;
		bool			v2Valid;
	};
	traceResult_t		contact;




	/*
	============
	Initialize
	============
	*/
	void Initialize() {
		markerPool.clear();
		obstaclePool.clear();

		obstaclesPendingUpdate.Clear();
		obstaclesUpdateTime = 0;
		memset(&contact, 0, sizeof(contact));

		// Clear All The Marker Pointers In Any Areas
		//--------------------------------------------
		for (int i=0; i<gameLocal.GetNumAAS(); i++) {
			if (gameLocal.GetAAS(i)) {
				idAASFile* file = gameLocal.GetAAS(i)->GetFile();
				for (int a=0; a<file->GetNumAreas(); a++) {
					file->GetArea(a).firstMarker = NULL;
				}
			}
		}
	}

	/*
	============
	DrawDebugGraphics
	============
	*/
	void DrawDebugGraphics() {
		static int			nextDrawTime=0;
		static rvObstacle*	obstacle;
		if (nextDrawTime>=gameLocal.time) {
			return;
		}
		nextDrawTime = gameLocal.time;

		for (int i=0; i<MAX_GENTITIES; i++) {
			if (obstaclePool.valid(i)) {
				obstaclePool[i]->DrawDebugGraphics();
			}
		}
	}

	/*
	============
	UpdateObstacles
	============
	*/
	void UpdateObstacles() {
		static rvObstacle*	obstacle;
		static float		entityNum;

		if (obstaclesUpdateTime < gameLocal.time && obstaclesPendingUpdate.Num()) {
			obstaclesUpdateTime = gameLocal.time + 50;

			// CDR_TODO: Priority Queue Of Updates?  Track Last Update Time Perhaps?
			while (obstaclesPendingUpdate.Num()) {
				obstacle = obstaclesPendingUpdate.StackTop();
				obstaclesPendingUpdate.StackPop();

				if (!obstacle->Update()) {
					obstaclePool.free(obstacle->entity.GetEntityNum());
				}
			}
		}
	}

	/*
	============
	MarkEntityForUpdate
	============
	*/
	void MarkEntityForUpdate(idEntity* ent) {

		// Ignore Non Physics Entities
		//-----------------------------
		if (!ent || !ent->GetPhysics()) {
			return;
		}

		// Ignore Obstacles That Are Already Pending
		//--------------------------------------------
		if (obstaclePool.valid(ent->entityNumber) && obstaclePool[ent->entityNumber]->pendingUpdate) {
			return;
		}

		if (!obstaclePool.valid(ent->entityNumber)) {

			// If No More Obstacles Are Available, Ignore This One
			//-----------------------------------------------------
			if (obstaclePool.full()) {
				return;
			}

			obstaclePool.alloc(ent->entityNumber)->Initialize(ent);
		}


		obstaclePool[ent->entityNumber]->pendingUpdate	= true;
		obstaclesPendingUpdate.Append(obstaclePool[ent->entityNumber]);
	}


	/*
	============
	RecordContact
	============
	*/
	void RecordContact(float maxDistance, const idVec3& start, rvObstacle* obstacle, const idVec3& point, int vert1=-1, int vert2=-1) {
		static float distance;
		distance = point.DistXY(start);
		if ((maxDistance==0.0f || distance<maxDistance) && (contact.distance > distance || !contact.obstacle)) {
			contact.obstacle	= obstacle;
			contact.endPoint	= point;
			contact.distance	= distance;
			contact.v1			= vert1;
			contact.v2			= vert2;
		}
	}

	/*
	============
	RayTrace
	============
	*/
	bool RayTrace(float maxDistance, const aasArea_t* area, const idVec3& start, const idVec3& stop, int aasNum, const idEntity* ignore1, const idEntity* ignore2=NULL, const idEntity* ignore3=NULL) {
		static rvMarker*	marker;
		static idVec3		p;
		static int			v1;
		static int			v2;
		static rvObstacle*	ignoreA;
		static rvObstacle*	ignoreB;
		static rvObstacle*	ignoreC;
		static idEntity*	ent;
		static idVec3		startBack;
		static idVec3		direction;
		static bool			pulledBack;

		ignoreA = (ignore1 && obstaclePool.valid(ignore1->entityNumber))?(obstaclePool[ignore1->entityNumber]):(NULL);
		ignoreB = (ignore2 && obstaclePool.valid(ignore2->entityNumber))?(obstaclePool[ignore2->entityNumber]):(NULL);
		ignoreC = (ignore3 && obstaclePool.valid(ignore3->entityNumber))?(obstaclePool[ignore3->entityNumber]):(NULL);

		contact.obstacle	= NULL;
		pulledBack			= false;


		for (marker=area->firstMarker; marker; marker=marker->next) {
			if (marker->obstacle==NULL || marker->obstacle==ignoreA || marker->obstacle==ignoreB || marker->obstacle==ignoreC) {
				continue;
			}

			// Handle Moving Obstacles Differently
			//-------------------------------------
			if (marker->obstacle->MovedRecently()) {

				// Ignore Moving Actors With Higher Entity Numbers
				//-------------------------------------------------
				ent = marker->obstacle->entity.GetEntity();
				if (ent && ignore1 && ent->entityNumber>ignore1->entityNumber && ent->IsType(idActor::GetClassType())) {
					continue;
				}

				// Pull The Start Back To Avoid Starting "In Solid"
				//--------------------------------------------------
				if (!pulledBack) {
					pulledBack = true;
					direction = stop - start;
					direction.Normalize();
					startBack = start - (direction*12.0f);	// CDR_TODO: Use Radius
				}

				// Record Contact With The Moving Obstacle's Predicted Path
				//----------------------------------------------------------
				if (LineIntersection2D(startBack, stop, marker->obstacle->origin, marker->obstacle->originFuture, p)) {
					RecordContact(maxDistance, startBack, marker->obstacle, p);
				}


			// Stationary Obstacles
			//----------------------
			} else {

				// CDR_TODO: Special Line Intersection Test For Player Forward Aim And On Same Team
				//----------------------------------------------------------------------------------


				// Pull The Start Back To Avoid Starting "In Solid"
				//--------------------------------------------------
				if (!pulledBack) {
					pulledBack = true;
					direction = stop - start;
					direction.Normalize();
					startBack = start - (direction*12.0f);	// CDR_TODO: Use Radius
				}

				// Record Contact With The Stationary Obstacle's Position
				//--------------------------------------------------------
				if (marker->obstacle->windings[aasNum].LineIntersection(startBack, stop, p, v1, v2)) {
					RecordContact(maxDistance, startBack, marker->obstacle, p, v1, v2);
				}
			}
		}

		if (contact.obstacle) {
			if (contact.obstacle->MovedRecently()) {
				contact.v1Valid = true;
				contact.v2Valid = true;
			} else {
				contact.v1Valid = rvObstacle::VertexValid(contact.obstacle->windings[aasNum], contact.v1, area, ignore1);
				contact.v2Valid = rvObstacle::VertexValid(contact.obstacle->windings[aasNum], contact.v2, area, ignore1);
			}

			return true;
		}
		return false;
	}
};
rvObstacleFinder	obstacleFinder;





/*
=====================
rvPathFinder
=====================
*/
class rvPathFinder {
public:
	/*
	=====================
	visitNode
	=====================
	*/
	struct visitNode {
		float				costToGoal;
		float				travelCost;
		bool				closed;
		int					vertexNum;
		idReachability*		reach;
		visitNode*			from;

		float cost() {
			return travelCost + costToGoal;
		}
	};

	static int visitSort( const void *a, const void *b ) {
		return (int)( (*((visitNode**)b))->cost() - (*((visitNode**)a))->cost() );
	}



	enum {
		MAX_VISITED			= 512,
		MAX_OPEN			= 255,
		MAX_PENDING			= 60,
	};

	idAAS*				myAAS;
	float				myRadius;
	idMoveState*		myMove;
	int					myTeam;
	const idEntity*		myIgnoreEntity;
	const idEntity*		myIgnoreEntity2;
	bool				drawVisitTree;

	visitNode*			next;

	visitNode*			open[MAX_OPEN];
	int					openCount;
	bool				openListUpdate;

	visitNode*			pending[MAX_PENDING];
	visitNode*			pendingBest;
	int					pendingCount;

	visitNode			visited[MAX_VISITED];
	int					visitedCount;
	idHashIndex			visitedIndexReach;
	idHashIndex			visitedIndexVert;





	/*
	=====================
	Initialize
	=====================
	*/
	void Initialize() {
		openCount = 0;
		openListUpdate = false;
		pendingCount = 0;
		pendingBest = NULL;
		visitedCount = 0;
		visitedIndexReach.Clear();
		visitedIndexVert.Clear();
		next = NULL;
	}


	/*
	=====================
	Close
	=====================
	*/
	void	Close(visitNode* node) {
		node->closed = true;
	}

	/*
	=====================
	DrawVisitTree
	=====================
	*/
	void	DrawVisitTree() {
	 	for (int i=0; i<visitedCount; i++) {
			const idVec3&	start		= (visited[i].from)?(GetSeekPosition(visited[i].from)):(myMove->myPos);
			const idVec3&	stop		= (GetSeekPosition(&visited[i]));
			const idVec4&	color		= (visited[i].closed)?(colorOrange):(colorYellow);
			const int		duration	= (visited[i].closed)?(3000):(500);

			gameRenderWorld->DebugArrow(color, start, stop, 3, duration);
		}
	}

	/*
	=====================
	XYLineIntersection
	=====================
	*/
	bool  XYLineIntersection(const idVec3& A, const idVec3& B, const idVec3& C, const idVec3& D, idVec3& P) {
		float q = (((B.x-A.x)*(D.y-C.y))-((B.y-A.y)*(D.x-C.x)));
		if (fabsf(q)>0.01f) {
			float s = (((A.y-C.y)*(B.x-A.x))-((A.x-C.x)*(B.y-A.y))) / q;
			if (s<0.0f || s>1.0f) {
				return false;
			}

			float r = (((A.y-C.y)*(D.x-C.x))-((A.x-C.x)*(D.y-C.y))) / q;
			if (r>1.0f) {
				P = B;
				return false;
			}
			if (r<0.0f) {
				P = A;
				return false;
			}

			P = A + r*(B-A);
			return true;
		}

		// Lines Are Parallel, No Intersection
		return false;
	}

	/*
	=====================
	GetSeekPosition
	=====================
	*/
	const idVec3& GetSeekPosition(idReachability* reach, int vertexNum) {
		return ((vertexNum)?(myAAS->GetFile()->GetVertex(vertexNum)):(reach->start));
	}

	/*
	=====================
	GetSeekPosition
	=====================
	*/
	const idVec3& GetSeekPosition(visitNode* node) {
		return  GetSeekPosition(node->reach, node->vertexNum);
	}

	/*
	=====================
	Success
	=====================
	*/
	bool	Success(visitNode* node) {
		static idVec3		edgeA;
		static idVec3		edgeB;
		static idVec3		intersect;
		static idVec3		direction;
		static idVec3		smoothedPos;
		static int			at;
		static int			count;
		static rvMarker*	marker;
		static bool			isCorner;

		// Always Add The Goal Pos At The End Of The Path
		//------------------------------------------------
		myMove->path[myMove->pathLen].reach		= NULL;
		myMove->path[myMove->pathLen].seekPos	= myMove->goalPos;
		myMove->pathLen ++;

		// Build The Path
		//----------------
		while (node && myMove->pathLen<MAX_PATH_LEN) {
			myMove->path[myMove->pathLen].reach		= node->reach;
			myMove->path[myMove->pathLen].seekPos	= GetSeekPosition(node);

			myMove->pathLen ++;
			node = node->from;
		}


		// Additional Path Point Modifications
		//-------------------------------------
		count = myMove->pathLen-1;
		for (at=count; at>0; at--) {
			pathSeek_t& pathPrev	= myMove->path[at+1];
			pathSeek_t& pathAt		= myMove->path[at];
			pathSeek_t& pathNext	= myMove->path[at-1];
			
			myAAS->GetEdge(pathAt.reach->edgeNum, edgeA, edgeB);

			// Smooth The Path One Pass
			//--------------------------
			if (pathAt.reach->travelType==TFL_WALK) {
				const idVec3& walkA = (at==count) ?		(myMove->myPos)		:(pathPrev.seekPos);
				const idVec3& walkB = (at==0) ?			(myMove->goalPos)	:(pathNext.seekPos);

				isCorner = !XYLineIntersection(edgeA, edgeB, walkA, walkB, smoothedPos);


				// If The Smoothed Position Is Not Blocked By An Obstacle
				//--------------------------------------------------------
				aasArea_t* area = &myAAS->GetFile()->GetArea(pathAt.reach->toAreaNum);
				for (marker=area->firstMarker; marker; marker=marker->next) {
					if (!marker->obstacle || marker->obstacle->entity.GetEntity()==myIgnoreEntity || marker->obstacle->entity.GetEntity()==myIgnoreEntity2) {
						continue;
					}
					if (marker->obstacle->windings[0/*CDR_TODO: use aasNum*/].PointInside(smoothedPos)) {
						break;
					}
				}
				if (!marker) {
					pathAt.seekPos = smoothedPos;

					// Push Away From The Corner A Bit
					//---------------------------------
					if (isCorner) {
						smoothedPos.ProjectToLineSeg(walkA, walkB);
						direction = pathAt.seekPos - smoothedPos;
						direction.NormalizeFast();
						pathAt.seekPos += direction * (REACHED_RADIUS+1.0f);
					}
				}
			}
		}

		// CDR_TODO: Record Statistics Here
		if (drawVisitTree) {
			DrawVisitTree();
		}
		return true;
	}

	/*
	=====================
	Failure
	=====================
	*/
	bool	Failure() {

		// CDR_TODO: Record Statistics Here
		if (drawVisitTree) {
			DrawVisitTree();
		}
		return false;
	}


	/*
	=====================
	ErrorCondition
	=====================
	*/
	bool	ErrorCondition() {
		assert(0);
		// Stats?
		return false;
	}

	/*
	=====================
	SelectNextVisitedNode
	=====================
	*/
	visitNode*	SelectNextVisitedNode() {
		if (pendingBest && (!openCount || open[openCount-1]->cost() > pendingBest->cost())) {
			next = pendingBest;
			pendingBest = NULL;
			openListUpdate = true;
			return next;
		}

		if (openCount) {
			openCount--;
		    return open[openCount];
		}
		return NULL;
	}

	/*
	=====================
	Open
	=====================
	*/
	void	Open(visitNode* node) {
		node->closed = false;

		// If This Node Is Cheaper Than The Existing Best Open Node, Add It To The End Of The Open List
		//----------------------------------------------------------------------------------------------
		if (openCount<MAX_OPEN) {
			if (!openCount || (node->cost() < open[openCount-1]->cost())) {
				open[openCount] = node;
				openCount++;
				return;
			}
		}

		// Otherwise, Add It To The Pending List
		//---------------------------------------
		if (pendingCount<MAX_PENDING) {
			pending[pendingCount] = node;
			pendingCount++;

			if (!pendingBest || (node->cost() < pendingBest->cost())) {
				pendingBest = node;
			}
		}
	}

	/*
	=====================
	UpdateOpenList
	=====================
	*/
	void	UpdateOpenList() {

		// List Is Updated Now, So Remove The Flag
		//-----------------------------------------
		openListUpdate = false;

		// Add All Pending Nodes To The Open List
		//----------------------------------------
		for (int i=0; i<pendingCount; i++) {
			if (openCount<MAX_OPEN && pending[i] && !pending[i]->closed) {
				open[openCount] = pending[i];
				openCount ++;
			}
		}
		pendingCount = 0;
		pendingBest = NULL;


		// Sort The Open List
		//--------------------
		qsort( (void*)open, (size_t)openCount, (size_t)sizeof(visitNode*), visitSort );
	}

	/*
	=====================
	WasVisited
	=====================
	*/
	visitNode*	WasVisited(idReachability* reach) {
		for (int i=visitedIndexReach.First(abs(reach->edgeNum)); i>=0; i=visitedIndexReach.Next(i)) {
			if (visited[i].reach==reach) {
				return &visited[i];
			}
		}
		return NULL;
	}

	/*
	=====================
	WasVisited
	=====================
	*/
	visitNode*	WasVisited(int vertexNum) {
		for (int i=visitedIndexVert.First(vertexNum); i>=0; i=visitedIndexVert.Next(i)) {
			if (visited[i].vertexNum==vertexNum) {
				return &visited[i];
			}
		}
		return NULL;
	}


	/*
	=====================
	TravelCost
	=====================
	*/
	float	TravelCost(idReachability* reach, int vertexNum, visitNode* from) {
		static float		distance;

		const idVec3&		start	= (from)?(GetSeekPosition(from)):(myMove->myPos);
		const idVec3&		stop	= GetSeekPosition(reach, vertexNum);


		// Test For Any Obstacles In The Way
		//-----------------------------------
		const aasArea_t*	area	= &myAAS->GetFile()->GetArea((from)?(from->reach->toAreaNum):(myMove->myArea));
		if (obstacleFinder.RayTrace(0.0f, area, start, stop, 0/*CDR_TODO: Get myAASNum*/, myIgnoreEntity, myIgnoreEntity2)) {

			// If It Is Not Possible To Steer Around, Then This Edge Is Completely Invalid
			//-----------------------------------------------------------------------------
			if (!obstacleFinder.contact.v1Valid && !obstacleFinder.contact.v2Valid) {
				return 0.0f;
			}

			// Completely Disable Any Points Completely Covered By An Obstacle
			//-----------------------------------------------------------------
			if (obstacleFinder.contact.obstacle->windings[0/*CDR_TODO: Get myAASNum*/].PointInside(stop)) {
				return 0.0f;
			}
			distance += 128.0f;
		}


		// Compute Standard Distance
		//---------------------------
		distance = start.Dist(stop); 
		if (from) {
			distance += from->travelCost + from->reach->travelTime;
		}

		return distance;
	}


	/*
	=====================
	Visit
	=====================
	*/
	void	Visit(idReachability* reach, int vertexNum, visitNode* from) {
		static visitNode*	visit;
		static float		travelCost;

		// If Full, Stop Visiting Anything
		//---------------------------------
		if (visitedCount>=MAX_VISITED) {
			return;
		}

		// Compute Travel Cost, And Test To See If This Edge Is Blocked
		//--------------------------------------------------------------
		travelCost	= TravelCost(reach, vertexNum, from);
		if (travelCost==0.0f) {
			return;
		}

		// If The Visited Version Is Already Less Costly, Then Ignore This Reach
		//-----------------------------------------------------------------------
		visit		= (vertexNum)?(WasVisited(vertexNum)):(WasVisited(reach));
		if (visit && visit->travelCost<=travelCost) {
			return;
		}



		// Reopen Any Nodes That Were Closed
		//-----------------------------------
		if (visit && visit->closed) {
			Open(visit);

		// Otherwise, If The Node Is Already In The Open List, Just Change The Cost And Mark The List For Resorting
		//----------------------------------------------------------------------------------------------------------
		} else if (visit) {
			visit->from			= from;
			visit->travelCost	= travelCost;
			openListUpdate		= true;

		// Must Never Have Visited This Node Before, So Make A Whole New One
		//-------------------------------------------------------------------
		} else {

			const idVec3& pos	= GetSeekPosition(reach, vertexNum);

			// Constant Data (Will Never Change)
			//-----------------------------------
			visit				= &visited[visitedCount];
			visit->reach		= reach;
			visit->vertexNum	= vertexNum;
			visit->costToGoal	= pos.Dist(myMove->goalPos); 

			// Temporary Data
			//----------------
			visit->from			= from;
			visit->travelCost	= travelCost;
			visit->closed		= false;

			// Add It To The Hash Table To Be Found Later
			//--------------------------------------------
			if (!vertexNum) {
				visitedIndexReach.Add(abs(reach->edgeNum),	visitedCount);
			} else {
				visitedIndexVert.Add(vertexNum,				visitedCount);
			}
			visitedCount++;

			// Mark It As Open
			//-----------------
			Open(visit);
		}
	}

	/*
	=====================
	VisitReach

	This function first visits the center of the reachability, and then if
	the edge is long enough and close enough to the start or end of the
	path, it visits the verts as well
	=====================
	*/
	void	VisitReach(idReachability *reach, visitNode* from) {
		static int			verts[2];
		static int			vertexNum;
		static idVec3		start;
		static idVec3		stop;


		// If Full, Stop Visiting Anything
		//---------------------------------
		if (visitedCount>=MAX_VISITED) {
			return;
		}

		// Ignore Any Reach To The Area We Came From
		//-------------------------------------------
		if (from && from->reach->fromAreaNum==reach->toAreaNum) {
			return;
		}

		// Ignore Any Reach That Does Not Match Our Travel Flags
		//-------------------------------------------------------
		if (reach->travelType&TFL_INVALID || !(reach->travelType&myMove->travelFlags)) {
			return;
		}

		// Visit The Center Of The Reach
		//-------------------------------
		Visit(reach, 0, from);


		// If Running Low On Visit Space, Stop Adding Verts
		//--------------------------------------------------
		if (visitedCount>=(int)((float)MAX_VISITED * 0.85f)) {
			return;
		}

		// If Edge Is Far From Start and Goal, Don't Add Verts
		//-----------------------------------------------------
		if (!reach->fromAreaNum!=myMove->myArea && 
			!reach->toAreaNum!=myMove->myArea && 
			!reach->fromAreaNum!=myMove->goalArea && 
			!reach->toAreaNum!=myMove->goalArea && 
			reach->start.Dist2XY(myMove->myPos)>22500.0f/*(250*250)*/ && reach->start.Dist2XY(myMove->goalPos)>22500.0f/*(250*250)*/) {
			return;
		}

		// If This Edge Is Small Enough, Just Skip The Verts
		//---------------------------------------------------
		myAAS->GetEdge(reach->edgeNum, start, stop);
		if (start.Dist2XY(stop)<6400.0f/*(80*80)*/) {
			return;
		}

		// Ok, So Visit The Verts Too
		//----------------------------
		myAAS->GetEdgeVertexNumbers(reach->edgeNum, verts);
		for (int i=0; i<2 && visitedCount<MAX_VISITED; i++) {
 			Visit(reach, verts[i], from);
		}
	}




	/*
	=====================
	VisitArea
	=====================
	*/
	void	VisitArea(int areaNum, visitNode* from=NULL) {
		idReachability*		reach;
		const aasArea_t&	area = myAAS->GetFile()->GetArea(areaNum);

		// If Visiting From Another Node, Close That Node Now
		//----------------------------------------------------
		if (from) {

			// If Already Closed
			//-------------------
			if (from->closed) {
				if (openListUpdate) {
					UpdateOpenList();	// this is kind of hacky...
				}

				// Don't Bother To Look At These Reaches
				//---------------------------------------
				return;

			// Otherwise, Close It
			//---------------------
			} else {
				Close(from);
			}
		}

		// Ok, Iterate All Reachabilities Within This Area
		//-------------------------------------------------
		for (reach=area.reach; reach && visitedCount<MAX_VISITED; reach=reach->next) {
			VisitReach(reach, from);
		}

		// Finally, If Necessary, Update The Open List Now
		//-------------------------------------------------
		if (openListUpdate) {
			UpdateOpenList();
		}
	}


	/*
	=====================
	FindPath
	=====================
	*/
	bool	FindPath(idAAS* aas, idMoveState& move, float radius, bool inDebugMode, idEntity* ignoreEntity, idEntity* ignoreEntity2) {
		myAAS				= aas;
		myMove				= &move;
		myRadius			= radius;
		drawVisitTree		= inDebugMode;
		myIgnoreEntity		= ignoreEntity;
		myIgnoreEntity2		= ignoreEntity2;

		myMove->pathArea	= myMove->goalArea;
		myMove->pathTime	= gameLocal.GetTime();
		myMove->pathLen		= 0;

		openCount			= 0;
		openListUpdate		= false;
		pendingCount		= 0;
		pendingBest			= NULL;
		visitedCount		= 0;
		visitedIndexReach.Clear();
		visitedIndexVert.Clear();



		if (!myMove->myArea) {
			return ErrorCondition();
		}
		const aasArea_t*	myArea = &myAAS->GetFile()->GetArea(myMove->myArea);
		const aasArea_t*	goalArea = &myAAS->GetFile()->GetArea(myMove->goalArea);


		// Special Case For Starting In The Goal Area
		//--------------------------------------------
		if (myMove->myArea==myMove->goalArea) {
			
			// Test For Any Obstacles In The Way
			//-----------------------------------
			if (!obstacleFinder.RayTrace(0.0f, myArea, myMove->myPos, myMove->goalPos, 0/*CDR_TODO: Get myAASNum*/, myIgnoreEntity, myIgnoreEntity2)) {
				return Success(NULL);
			}

			// If There Is An Obstacle But We Think We Can Steer Around It, Then We've Still Succeeded
			//-----------------------------------------------------------------------------------------
			if (obstacleFinder.contact.v1Valid || obstacleFinder.contact.v2Valid) {
				return Success(NULL);
			}
		}



		// Start With My Area
		//--------------------
		VisitArea(myMove->myArea);


		// While Reachabilities Are Still Pending
		//----------------------------------------
		while (openCount || pendingCount) {

			// Select Next Visited Node
			//--------------------------
			next = SelectNextVisitedNode();
			if (!next) {
				return ErrorCondition();
			}

			// If This Node Reaches Our Target Destination, We've Succeeded
			//--------------------------------------------------------------
			if (next->reach->toAreaNum==myMove->goalArea) {
				next->closed = true;

				// Test For Any Obstacles In The Way
				//-----------------------------------
				if (!obstacleFinder.RayTrace(0.0f, goalArea, GetSeekPosition(next), myMove->goalPos, 0/*CDR_TODO: Get myAASNum*/, myIgnoreEntity, myIgnoreEntity2)) {
					return Success(next);
				}

				// Or If There Is An Obstacle, But One Of The Verts Is Valid, Then This Is Still A Safe Course
				//---------------------------------------------------------------------------------------------
				if (obstacleFinder.contact.v1Valid || obstacleFinder.contact.v2Valid) {
					return Success(next);
				}

			// Visit All Reaches In The Next Area
			//-------------------------------------
			} else {
				VisitArea(next->reach->toAreaNum, next);
			}
		}

		return Failure();
	}
};

rvPathFinder	pathFinder;



void AI_EntityMoved(idEntity* ent) {
	obstacleFinder.MarkEntityForUpdate(ent);
}
void AI_MoveInitialize() {
	pathFinder.Initialize();
	obstacleFinder.Initialize();
}







/*
=====================
idAI::NewCombinedMove
=====================
*/
void idAI::RVMasterMove( void ) {
	static idVec3				mySeekDelta;
	static idVec3				mySeekDirection;
	static float				mySeekDistance;

	static float				moveDistance;
	static idVec3				moveDelta;
	static idEntity*			moveBlockEnt;
	static monsterMoveResult_t	moveResult;


	//====================================================================
	// UPDATE DATA
	//   A simple first step for movement is to get data from the command
	//   and goal arguments and compute changes if necessary.
	//====================================================================
	bool						seekMove	= CanMove() && move.moveCommand>=NUM_NONMOVING_COMMANDS;
	bool						seekTurn	= CanTurn() && move.moveCommand> MOVE_NONE;

	idMat3						myAxis		= viewAxis;
	const idVec3&				myPos		= physicsObj.GetOrigin();
	const idBounds&				myBounds	= physicsObj.GetBounds();
	float						myRadius	= myBounds.Size().x / 2.0f;
	idVec3						myPosOld	= move.myPos;	// only used for debug graphics
	bool						myPosMoved	= false;		// only used for debug graphics

	const idEntity*				goalEntity	= move.goalEntity.GetEntity();
	const idVec3&				goalPos		= (goalEntity)?(LastKnownPosition(goalEntity)):(move.moveDest);




	// Update My Position And Area
	//-----------------------------
	if (move.myArea==0 || move.myPos.Dist2XY(myPos)>20.0f) {
		move.myPos			= myPos;
		move.myArea			= PointReachableAreaNum(move.myPos);
		myPosMoved			= true;
	}
	aasArea_t*					myArea		= &aas->GetFile()->GetArea(move.myArea);

	// Update Goal Position And Area
	//-------------------------------
	if ((seekMove || seekTurn) && move.goalPos.Dist2XY(goalPos)>20.0f) {
		move.goalPos		= goalPos;
		move.goalArea		= PointReachableAreaNum(move.goalPos);
	}

	// If Reached The Goal Position, Then Stop Moving
	//------------------------------------------------
	if (seekMove && ReachedPos( move.goalPos, move.moveCommand, move.range )) {
		StopMove( MOVE_STATUS_DONE );
		seekMove = false;
		move.pathLen = 0;
	}

	// Update The Obstacle Markers
	//-----------------------------
	obstacleFinder.UpdateObstacles();



	//====================================================================
	// PATH FINDING
	//   If the goal area is not the same as myArea, then we need to run
	//   a pathfinding search to get to the goal area.  This operation
	//   will alter the seek position
	//====================================================================
	if (seekMove) {

		// If No Path Exists, Find One
		//-----------------------------
		if (move.pathArea!=move.goalArea || (!move.pathLen && move.pathTime < (gameLocal.time-10000))) {
			pathFinder.FindPath(aas, move, myRadius, DebugFilter(ai_debugMove), this, move.goalEntity.GetEntity());
		}


		// Set The SeekPos
		//-----------------
		if (move.pathLen) {
			move.seekPos = move.path[move.pathLen-1].seekPos;

			// If We've Reached The Next Path Pos, Pop It Off The List And Continue
			//----------------------------------------------------------------------
 			if (move.seekPos.Dist2XY(move.myPos)<REACHED_RADIUS_SQUARE) {
				// CDR_TODO: Reached Callback Here

				move.pathLen--;
				if (move.pathLen) {
					move.seekPos = move.path[move.pathLen-1].seekPos;
				}
			}

		} else {

			// Pathfinding Failure!  Stand Here And Retry In A Few Seconds
			//-------------------------------------------------------------
			seekMove		= false;
			move.seekPos	= move.myPos;
			move.blockTime	= gameLocal.time + 1600;
		}

	} else {
		move.seekPos = move.goalPos;
	}




	//====================================================================
	// STEERING
	//   The seek target is where AI will attempt to turn and move toward
	//   With simple commands the seek origin is just the goal origin, 
	//   however with pathfinding and obsticle avoidance the seek origin
	//   will usually be somewhere between the actor and his goal.
	//====================================================================
	move.fl.obstacleInPath		= false;	// Makes Character Walk
	move.fl.blocked				= false;	// Makes Character Stop Moving

	if (seekMove) {
		mySeekDelta				= move.seekPos - move.myPos;
		mySeekDirection			= mySeekDelta;
		mySeekDistance			= mySeekDirection.NormalizeFast();


		// Test For Obstacles In The Path
		//--------------------------------
		move.fl.obstacleInPath	= obstacleFinder.RayTrace(128.0f, myArea, move.myPos, move.seekPos, 0/*CDR_TODO: Get actual aasNumber*/, this, goalEntity);
		if (move.fl.obstacleInPath) {
			rvObstacleFinder::traceResult_t& tr = obstacleFinder.contact;

			move.obstacle				= tr.obstacle->entity;
			const rvWindingBox& bounds	= tr.obstacle->windings[0/*CDR_TODO: Get actual aasNumber*/]; 

			// Is The Obstacle Standing On Seek Position
			//-------------------------------------------
			if (bounds.PointInside(move.seekPos)) {
				if (move.myArea!=move.goalArea) {
					move.fl.blocked		= true;
					move.blockTime		= gameLocal.time + 1150;
					move.pathArea		= 0;	// force a refind path next update
				} else {

					// Otherwise, Stop And Wait For It To Move
					//-----------------------------------------
					move.fl.blocked		= true;
					move.blockTime		= gameLocal.time + 1050;
					// CDR_TODO: Issue MoveDestInvalid() Callback Here
				}
			} 

			// Is The Obstacle About To Cross My Path?
			//-----------------------------------------
			else if (tr.v1==-1) {	// CDR_TODO:  -1 is an obtouse way to detect this
				move.fl.blocked			= true;
				move.blockTime			= gameLocal.time + 1500;
			}

			// Ok, So Let's Try To Steer Around The Obstacle
			//-----------------------------------------------
			else {

				// If Neither Vertex Is Valid, Need To Refind Path
				//-------------------------------------------------
				if (!tr.v1Valid && !tr.v2Valid) {
					move.fl.blocked		= true;
					move.blockTime		= gameLocal.time + 3250;
					move.pathArea		= 0;	// force a refind path next update

				} else {

					int vertex;

					// Otherwise, Choose The Best Valid Vertex
					//-----------------------------------------
					if (!tr.v2Valid || bounds.areas[tr.v2]!=myArea) {
						vertex	= tr.v1;
					} else if (!tr.v1Valid || bounds.areas[tr.v2]!=myArea) {
						vertex	= tr.v2;
					} else {
						// CDRTODO: Record clockwise / counter clockwise and only test this once.
						vertex	= (tr.obstacle->origin.IsLeftOf(move.myPos, move.seekPos))?(tr.v2):(tr.v1);
					}


					// Get Close The Contact Point if The Choosen Vertex Is Not In This Area, Before Going Toward The Vertex
					//-------------------------------------------------------------------------------------------------------
					if (bounds.areas[vertex]!=myArea && move.myPos.Dist2XY(tr.endPoint)>400.0f /*20*20*/) {
						move.seekPos		= tr.endPoint;
					} else {
						move.seekPos		= bounds.verts[vertex];
					}


					// And Recompute The Seek Vectors
					//--------------------------------
					mySeekDelta			= move.seekPos - move.myPos;
					mySeekDirection		= mySeekDelta;
					mySeekDistance		= mySeekDirection.NormalizeFast();
				}
			}
		}
	}





	//====================================================================
	// TURNING
	//   Having finialized our seek position, it is now time to turn
	//   toward it.
	//====================================================================
	if (seekTurn) {
		DirectionalTurnToward(move.seekPos);
	}
	Turn();



	//====================================================================
	// VELOCITY
	//   Now we need to get the instantanious velocity vector, which will
	//   usually come right from the animation
	//====================================================================
	if (move.moveType == MOVETYPE_ANIM) {
		if ( move.fl.allowAnimMove || move.fl.allowPrevAnimMove ) {
			GetAnimMoveDelta( myAxis, viewAxis, moveDelta );
		} else {
			moveDelta.Zero();
		}
	} else {
		// CDR_TODO: Other movetypes here
	}

	// If Doing Seek Move, Cap The Delta To Avoid Overshooting The Seek Position
	//---------------------------------------------------------------------------
	if (seekMove && moveDelta!=vec3_zero) {
		moveDistance = moveDelta.LengthFast();

		if (mySeekDistance<0.5f) {
			moveDelta = vec3_zero;
		} else if (mySeekDistance<moveDistance) {
			moveDelta = mySeekDelta;
		} else if (mySeekDistance<64.0f || (!move.fl.allowAnimMove && move.fl.allowPrevAnimMove)) {
			moveDelta = mySeekDelta * (moveDistance/mySeekDistance);
		}
	}



	//====================================================================
	// PHYSICS SIMULATION
	//   Now we need to get the instantanious velocity vector, which will
	//   usually come right from the animation
	//====================================================================
	physicsObj.SetDelta( moveDelta );
	physicsObj.ForceDeltaMove( move.fl.noGravity );
	RunPhysics();

	// Record The Results
	//--------------------
	move.fl.onGround	= physicsObj.OnGround();
	moveResult			= physicsObj.GetMoveResult();
	moveBlockEnt		= physicsObj.GetSlideMoveEntity();

	// Push Things Out Of The Way
	//----------------------------
	if (moveBlockEnt && moveBlockEnt->IsType( idMoveable::GetClassType() ) && moveBlockEnt->GetPhysics()->IsPushable()) {
		KickObstacles( viewAxis[ 0 ], move.kickForce, moveBlockEnt );
	}

	// Touch Triggers
	//----------------
	if (moveDelta!=vec3_zero ) {
		if (moveResult!=MM_BLOCKED) {
			TouchTriggers();
		}
	}



	//====================================================================
	// DEBUG GRAPHICS
	//====================================================================
	idVec3 origin	= physicsObj.GetOrigin();
	if (DebugFilter(ai_debugMove)) {
		static const idVec3	upPole(0.0f, 0.0f, 60.0f);
		static const idVec3	upSeek(0.0f, 0.0f, 3.0f);

		gameRenderWorld->DebugBounds(colorMagenta,		physicsObj.GetBounds(), origin,				gameLocal.msec);	// Bounds: MAGENTA
		gameRenderWorld->DebugArrow(colorGreen,			origin+upSeek,	move.seekPos + upSeek,	5,	gameLocal.msec);	// Seek: GREEN
		gameRenderWorld->DebugLine(colorPurple,			move.goalPos,	move.goalPos + upPole,		gameLocal.msec);	// Goal: PURPLE

		if (myPosMoved) {
			gameRenderWorld->DebugLine(colorCyan,		myPosOld,		move.myPos,					800);				// Trail: CYAN
		}

		if (move.pathLen) {
			for (int i=move.pathLen-1; i>=0; i--) {
				gameRenderWorld->DebugLine(colorBlue,	origin,			move.path[i].seekPos,		gameLocal.msec);	// FoundPath: BLUE
				origin = move.path[i].seekPos;
			}
		}
																														// PathVisited: ORANGE
																														// PathOpened: YELLOW
	}

	if (DebugFilter(ai_showObstacleAvoidance)) {
		static const idVec3	upSeek(0.0f, 0.0f, 3.0f);
		const idVec3& obstaclePos = (move.obstacle.GetEntity())?(move.obstacle->GetPhysics()->GetOrigin()):(move.seekPos);

		if (!DebugFilter(ai_debugMove)) {
			gameRenderWorld->DebugArrow(colorGreen,		origin+upSeek,	move.seekPos + upSeek,	5,	gameLocal.msec);	// Seek: GREEN
		}

 		if (move.blockTime>gameLocal.time) {
			gameRenderWorld->DebugArrow(colorRed,		move.myPos,		obstaclePos,			3,	gameLocal.msec);	// Blocked Obstacle: RED
		} else if (move.fl.obstacleInPath) {
			gameRenderWorld->DebugArrow(colorWhite,		move.myPos,		obstaclePos,			3,	gameLocal.msec);	// Walk Obstacle: WHITE
		}
																														// ObstacleBox: YELLOW
																														// VertexInvalid: RED
																														// FuturePosition: BROWN
		obstacleFinder.DrawDebugGraphics();
	}
}

/*
=====================
idAI::Move
=====================
*/
void idAI::Move	( void ) {
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerMove.Start ( );
	}

	switch( move.moveType ) {
	case MOVETYPE_DEAD:
		DeadMove();
		break;
	case MOVETYPE_FLY :
		FlyMove();
		break;
	case MOVETYPE_STATIC :
		StaticMove();
		break;
	case MOVETYPE_ANIM :
		AnimMove();
		break;
	case MOVETYPE_SLIDE :
		SlideMove();
		break;
	case MOVETYPE_PLAYBACK:
		PlaybackMove();
		break;
	case MOVETYPE_CUSTOM:
		CustomMove();
		break;
	}

	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerMove.Stop ( );
	}
}

