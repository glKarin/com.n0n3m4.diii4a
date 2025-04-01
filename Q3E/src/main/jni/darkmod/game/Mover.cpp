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



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "Objectives/MissionData.h"
#include "StimResponse/StimResponseCollection.h"

// a mover will update any gui entities in its target list with 
// a key/val pair of "mover" "state" from below.. guis can represent
// realtime info like this
// binary only
static const char *guiBinaryMoverStates[] = {
	"1",	// pos 1
	"2",	// pos 2
	"3",	// moving 1 to 2
	"4"		// moving 2 to 1
};


/*
===============================================================================

idMover

===============================================================================
*/

const idEventDef EV_FindGuiTargets( "<FindGuiTargets>", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_TeamBlocked( "<teamblocked>", EventArgs('e', "", "", 'e', "", ""), EV_RETURNS_VOID, "internal");
const idEventDef EV_PartBlocked( "<partblocked>", EventArgs('e', "", ""), EV_RETURNS_VOID, "internal");
const idEventDef EV_ReachedPos( "<reachedpos>", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_ReachedAng( "<reachedang>", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_PostRestore( "<postrestore>", 
	EventArgs('d', "", "", 'd', "", "", 'd', "", "", 'd', "", "", 'd', "", ""), EV_RETURNS_VOID, "internal");

const idEventDef EV_StopMoving( "stopMoving", EventArgs(), EV_RETURNS_VOID, "Stops any translational movement.");
const idEventDef EV_StopRotating( "stopRotating", EventArgs(), EV_RETURNS_VOID, "Stops any rotational movement.");
const idEventDef EV_Speed( "speed", EventArgs('f', "speed", ""), EV_RETURNS_VOID, "Sets the movement speed. Set this speed before initiating a new move.");
const idEventDef EV_Time( "time", EventArgs('f', "time", ""), EV_RETURNS_VOID, "Sets the movement time. Set this time before initiating a new move.");
const idEventDef EV_GetMoveSpeed( "getMoveSpeed", EventArgs(), 'f', "Get the movement speed.");
const idEventDef EV_GetMoveTime( "getMoveTime", EventArgs(), 'f', "Gets the movement time.");
const idEventDef EV_AccelTime( "accelTime", EventArgs('f', "time", ""), EV_RETURNS_VOID, "Sets the acceleration time. Set this acceleration time before initiating a new move.");
const idEventDef EV_DecelTime( "decelTime", EventArgs('f', "time", ""), EV_RETURNS_VOID, "Sets the deceleration time. Set this deceleration time before initiating a new move.");
const idEventDef EV_MoveTo( "moveTo", EventArgs('e', "targetEntity", ""), EV_RETURNS_VOID, 
	"Initiates a translation to the position of an entity.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_MoveToPos( "moveToPos", EventArgs('v', "pos", ""), EV_RETURNS_VOID, 
	"Initiates a translation to an absolute position.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_Move( "move", EventArgs('f', "angle", "", 'f', "distance", ""), EV_RETURNS_VOID, 
	"Initiates a translation with the given distance in the given yaw direction.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_MoveAccelerateTo( "accelTo", EventArgs('f', "speed", "", 'f', "time", ""), EV_RETURNS_VOID, "Initiates an acceleration to the given speed over the given time in seconds.");
const idEventDef EV_MoveDecelerateTo( "decelTo", EventArgs('f', "speed", "", 'f', "time", ""), EV_RETURNS_VOID, "Initiates a deceleration to the given speed over the given time in seconds.");
const idEventDef EV_RotateDownTo( "rotateDownTo", EventArgs('d', "axis", "", 'f', "angle", ""), EV_RETURNS_VOID, 
	"Initiates a rotation about the given axis by decreasing the current angle towards the given angle.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_RotateUpTo( "rotateUpTo", EventArgs('d', "axis", "", 'f', "angle", ""), EV_RETURNS_VOID, 
	"Initiates a rotation about the given axis by increasing the current angle towards the given angle.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_RotateTo( "rotateTo", EventArgs('v', "angles", ""), EV_RETURNS_VOID, 
	"Initiates a rotation towards the given Euler angles.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_Rotate( "rotate", EventArgs('v', "angleSpeed", ""), EV_RETURNS_VOID, 
	"Initiates a rotation with the given angular speed.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_RotateOnce( "rotateOnce", EventArgs('v', "angles", ""), EV_RETURNS_VOID, 
	"Initiates a rotation towards the current angles plus the given Euler angles.\n" \
	"Uses the current speed/time and acceleration and deceleration settings.");
const idEventDef EV_Bob( "bob", EventArgs('f', "speed", "", 'f', "phase", "", 'v', "distance", ""), EV_RETURNS_VOID, 
	"Initiates a translation back and forth along the given vector with the given speed and phase.");
const idEventDef EV_Sway( "sway", EventArgs('f', "speed", "", 'f', "phase", "", 'v', "angles", ""), EV_RETURNS_VOID, 
	"Initiates a rotation back and forth along the given angles with the given speed and phase.");
const idEventDef EV_Mover_OpenPortal( "openPortal", EventArgs(), EV_RETURNS_VOID, "Opens the renderer portal associated with this mover.");
const idEventDef EV_Mover_ClosePortal( "closePortal", EventArgs(), EV_RETURNS_VOID, "Closes the renderer portal associated with this mover.");
const idEventDef EV_AccelSound( "accelSound", EventArgs('s', "sound", ""), EV_RETURNS_VOID, "Sets the sound to be played when the mover accelerates.");
const idEventDef EV_DecelSound( "decelSound", EventArgs('s', "sound", ""), EV_RETURNS_VOID, "Sets the sound to be played when the mover decelerates.");
const idEventDef EV_MoveSound( "moveSound", EventArgs('s', "sound", ""), EV_RETURNS_VOID, "Sets the sound to be played when the moving.");
const idEventDef EV_Mover_InitGuiTargets( "<initguitargets>", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_EnableSplineAngles( "enableSplineAngles", EventArgs(), EV_RETURNS_VOID, "Enables aligning the mover with the spline direction.");
const idEventDef EV_DisableSplineAngles( "disableSplineAngles", EventArgs(), EV_RETURNS_VOID, "Disables aligning the mover with the spline direction.");
const idEventDef EV_RemoveInitialSplineAngles( "removeInitialSplineAngles", EventArgs(), EV_RETURNS_VOID, 
	"Subtracts the initial spline angles to maintain the initial orientation of the mover.");
const idEventDef EV_StartSpline( "startSpline", EventArgs('e', "spline", ""), EV_RETURNS_VOID, 
	"Starts moving along a spline stored on the given entity.");
const idEventDef EV_StopSpline( "stopSpline", EventArgs(), EV_RETURNS_VOID, "Stops moving along a spline.");
const idEventDef EV_IsMoving( "isMoving", EventArgs(), 'd',  "Returns true if a mover is moving" );
const idEventDef EV_IsRotating( "isRotating", EventArgs(), 'd', "Returns true if a mover is rotating" );

CLASS_DECLARATION( idEntity, idMover )
	EVENT( EV_FindGuiTargets,		idMover::Event_FindGuiTargets )
	EVENT( EV_Thread_SetCallback,	idMover::Event_SetCallback )
	EVENT( EV_TeamBlocked,			idMover::Event_TeamBlocked )
	EVENT( EV_PartBlocked,			idMover::Event_PartBlocked )
	EVENT( EV_ReachedPos,			idMover::Event_UpdateMove )
	EVENT( EV_ReachedAng,			idMover::Event_UpdateRotation )
	EVENT( EV_PostRestore,			idMover::Event_PostRestore )
	EVENT( EV_StopMoving,			idMover::Event_StopMoving )
	EVENT( EV_StopRotating,			idMover::Event_StopRotating )
	EVENT( EV_Speed,				idMover::Event_SetMoveSpeed )
	EVENT( EV_Time,					idMover::Event_SetMoveTime )
	EVENT( EV_GetMoveSpeed,				idMover::Event_GetMoveSpeed )
	EVENT( EV_GetMoveTime,				idMover::Event_GetMoveTime )
	EVENT( EV_AccelTime,			idMover::Event_SetAccellerationTime )
	EVENT( EV_DecelTime,			idMover::Event_SetDecelerationTime )
	EVENT( EV_MoveTo,				idMover::Event_MoveTo )
	EVENT( EV_MoveToPos,			idMover::Event_MoveToPos )
	EVENT( EV_Move,					idMover::Event_MoveDir )
	EVENT( EV_MoveAccelerateTo,		idMover::Event_MoveAccelerateTo )
	EVENT( EV_MoveDecelerateTo,		idMover::Event_MoveDecelerateTo )
	EVENT( EV_RotateDownTo,			idMover::Event_RotateDownTo )
	EVENT( EV_RotateUpTo,			idMover::Event_RotateUpTo )
	EVENT( EV_RotateTo,				idMover::Event_RotateTo )
	EVENT( EV_Rotate,				idMover::Event_Rotate )
	EVENT( EV_RotateOnce,			idMover::Event_RotateOnce )
	EVENT( EV_Bob,					idMover::Event_Bob )
	EVENT( EV_Sway,					idMover::Event_Sway )
	EVENT( EV_Mover_OpenPortal,		idMover::Event_OpenPortal )
	EVENT( EV_Mover_ClosePortal,	idMover::Event_ClosePortal )
	EVENT( EV_AccelSound,			idMover::Event_SetAccelSound )
	EVENT( EV_DecelSound,			idMover::Event_SetDecelSound )
	EVENT( EV_MoveSound,			idMover::Event_SetMoveSound )
	EVENT( EV_Mover_InitGuiTargets,	idMover::Event_InitGuiTargets )
	EVENT( EV_EnableSplineAngles,	idMover::Event_EnableSplineAngles )
	EVENT( EV_DisableSplineAngles,	idMover::Event_DisableSplineAngles )
	EVENT( EV_RemoveInitialSplineAngles, idMover::Event_RemoveInitialSplineAngles )
	EVENT( EV_StartSpline,			idMover::Event_StartSpline )
	EVENT( EV_StopSpline,			idMover::Event_StopSpline )
	EVENT( EV_Activate,				idMover::Event_Activate )
	EVENT( EV_IsMoving,				idMover::Event_IsMoving )
	EVENT( EV_IsRotating,			idMover::Event_IsRotating )
END_CLASS

/*
================
idMover::idMover
================
*/
idMover::idMover(void)
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);

	memset( &move, 0, sizeof( move ) );
	memset( &rot, 0, sizeof( rot ) );
	move_thread = 0;
	rotate_thread = 0;
	dest_angles.Zero();
	angle_delta.Zero();
	dest_position.Zero();
	move_delta.Zero();
	move_speed = 0.0f;
	move_time = 0;
	prevTransSpeed = 0.0f;
	deceltime = 0;
	acceltime = 0;
	stopRotation = false;
	useSplineAngles = true;
	lastCommand = MOVER_NONE;
	damage = 0.0f;
	areaPortal = 0;
	nextBounceTime = 0; // grayman #4370
	m_FrobActionScript = "frob_mover";
}

/*
================
idMover::Save
================
*/
void idMover::Save( idSaveGame *savefile ) const
{
	int i;

	savefile->WriteStaticObject( physicsObj );
	savefile->WriteInt( move.stage );
	savefile->WriteInt( move.acceleration );
	savefile->WriteInt( move.movetime );

	savefile->WriteInt( move.deceleration );
	savefile->WriteVec3( move.dir );

	savefile->WriteInt( rot.stage );
	savefile->WriteInt( rot.acceleration );
	savefile->WriteInt( rot.movetime );
	savefile->WriteInt( rot.deceleration );
	savefile->WriteAngles( rot.rot );

	savefile->WriteInt( move_thread );
	savefile->WriteInt( rotate_thread );

	savefile->WriteAngles( dest_angles );
	savefile->WriteAngles( angle_delta );
	savefile->WriteVec3( dest_position );
	savefile->WriteVec3( move_delta );

	savefile->WriteFloat( move_speed );
	savefile->WriteInt( move_time );
	savefile->WriteInt( prevMoveTime ); // grayman #3755
	savefile->WriteFloat( prevTransSpeed ); // grayman #3755
	savefile->WriteInt( deceltime );
	savefile->WriteInt( acceltime );
	savefile->WriteBool( stopRotation );
	savefile->WriteBool( useSplineAngles );
	savefile->WriteInt( lastCommand );
	savefile->WriteFloat( damage );

	savefile->WriteInt( areaPortal );
	if ( areaPortal > 0 ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}

	savefile->WriteInt(nextBounceTime); // grayman #4370

	savefile->WriteInt( guiTargets.Num() );
	for( i = 0; i < guiTargets.Num(); i++ ) {
		guiTargets[ i ].Save( savefile );
	}

	if ( splineEnt.GetEntity() && splineEnt.GetEntity()->GetSpline() ) {
		idCurve_Spline<idVec3> *spline = physicsObj.GetSpline();

		savefile->WriteBool( true );
		splineEnt.Save( savefile );
		savefile->WriteInt( static_cast<int>(spline->GetTime( 0 )) );
		savefile->WriteInt( static_cast<int>(spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 )) );
		savefile->WriteInt( physicsObj.GetSplineAcceleration() );
		savefile->WriteInt( physicsObj.GetSplineDeceleration() );
		savefile->WriteInt( (int)physicsObj.UsingSplineAngles() );

	} else {
		savefile->WriteBool( false );
	}
}

/*
================
idMover::Restore
================
*/
void idMover::Restore( idRestoreGame *savefile ) {
	int i, num;
	bool hasSpline = false;

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadInt( (int&)move.stage );
	savefile->ReadInt( move.acceleration );
	savefile->ReadInt( move.movetime );
	savefile->ReadInt( move.deceleration );
	savefile->ReadVec3( move.dir );

	savefile->ReadInt( (int&)rot.stage );
	savefile->ReadInt( rot.acceleration );
	savefile->ReadInt( rot.movetime );
	savefile->ReadInt( rot.deceleration );

	savefile->ReadAngles( rot.rot );

	savefile->ReadInt( move_thread );
	savefile->ReadInt( rotate_thread );

	savefile->ReadAngles( dest_angles );
	savefile->ReadAngles( angle_delta );
	savefile->ReadVec3( dest_position );
	savefile->ReadVec3( move_delta );

	savefile->ReadFloat( move_speed );
	savefile->ReadInt( move_time );
	savefile->ReadInt( prevMoveTime ); // grayman #3755
	savefile->ReadFloat( prevTransSpeed ); // grayman #3755
	savefile->ReadInt( deceltime );
	savefile->ReadInt( acceltime );
	savefile->ReadBool( stopRotation );
	savefile->ReadBool( useSplineAngles );
	savefile->ReadInt( (int &)lastCommand );
	savefile->ReadFloat( damage );

	savefile->ReadInt( areaPortal );
	if ( areaPortal > 0 ) {
		int portalState = 0;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}

	savefile->ReadInt(nextBounceTime); // grayman #4370

	guiTargets.Clear();
	savefile->ReadInt( num );
	guiTargets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		guiTargets[ i ].Restore( savefile );
	}

	savefile->ReadBool( hasSpline );
	if ( hasSpline ) {
		int starttime;
		int totaltime;
		int accel;
		int decel;
		int useAngles;

		splineEnt.Restore( savefile );
		savefile->ReadInt( starttime );
		savefile->ReadInt( totaltime );
		savefile->ReadInt( accel );
		savefile->ReadInt( decel );
		savefile->ReadInt( useAngles );

		PostEventMS( &EV_PostRestore, 0, starttime, totaltime, accel, decel, useAngles );
	} 
}

/*
================
idMover::Event_PostRestore
================
*/
void idMover::Event_PostRestore( int start, int total, int accel, int decel, int useSplineAng ) {
	idCurve_Spline<idVec3> *spline;

	idEntity *splineEntity = splineEnt.GetEntity();
	if ( !splineEntity ) {
		// We should never get this event if splineEnt is invalid
		common->Warning( "Invalid spline entity during restore" );
		return;
	}

	spline = splineEntity->GetSpline();

	spline->MakeUniform( total );
	spline->ShiftTime( start - spline->GetTime( 0 ) );

	physicsObj.SetSpline( spline, accel, decel, ( useSplineAng != 0 ) );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
}

/*
================
idMover::Spawn
================
*/
void idMover::Spawn( void )
{
	move_thread		= 0;
	rotate_thread	= 0;
	stopRotation	= false;
	lastCommand		= MOVER_NONE;

	acceltime		= static_cast<int>(1000.0f * spawnArgs.GetFloat( "accel_time", "0" ));
	deceltime		= static_cast<int>(1000.0f * spawnArgs.GetFloat( "decel_time", "0" ));
	move_time		= static_cast<int>(1000.0f * spawnArgs.GetFloat( "move_time", "1" ));	// safe default value
	move_speed		= spawnArgs.GetFloat( "move_speed", "0" );
	prevMoveTime	= move_time; // grayman #3755

	spawnArgs.GetFloat( "damage" , "0", damage );

	dest_position = GetPhysics()->GetOrigin();
	dest_angles = GetPhysics()->GetAxis().ToAngles();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "solid", "1" ) ) {
		physicsObj.SetContents( 0 );
	}
	if ( !renderEntity.hModel || !spawnArgs.GetBool( "nopush" ) ) {
		// greebo: Check if we should be able to push the player (default is yes)
		if (spawnArgs.GetBool("push_player", "1")) {
			physicsObj.SetPusher(0);
		}
		else {
			physicsObj.SetPusher(PUSHFL_NOPLAYER);
		}
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
	SetPhysics( &physicsObj );

	// see if we are on an areaportal
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );
	if ( areaPortal ) {
		// stgatilov #5172: consider doors to be world geometry like brushes, since they can also block light in area graph
		// imagine objects on line in this order: light, wall, unreachable area, wall with door, player;
		// player will see light leak through the door, unless some backface casts a shadow
		// wall backface does not cast shadow since it is in fully closed area, so the door should
		renderEntity.forceShadowBehindOpaque = true;
	}

	if ( spawnArgs.MatchPrefix( "guiTarget" ) ) {
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindGuiTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindGuiTargets();
		}
	}

	health = spawnArgs.GetInt( "health" );
	if ( health ) {
		fl.takedamage = true;
	}
}

/*
================
idMover::Hide
================
*/
void idMover::Hide( void ) {
	idEntity::Hide();
	physicsObj.SetContents( 0 );
}

/*
================
idMover::Show
================
*/
void idMover::Show( void ) {
	idEntity::Show();
	physicsObj.SetContents( m_preHideContents );
	SetPhysics( &physicsObj );
}

/*
============
idMover::Killed
============
*/
void idMover::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) 
{
	bool bPlayerResponsible(false);

	fl.takedamage = false;
	ActivateTargets( this );

	if ( attacker && attacker->IsType( idPlayer::Type ) )
		bPlayerResponsible = ( attacker == gameLocal.GetLocalPlayer() );
	else if( attacker && attacker->m_SetInMotionByActor.GetEntity() )
		bPlayerResponsible = ( attacker->m_SetInMotionByActor.GetEntity() == gameLocal.GetLocalPlayer() );

	gameLocal.m_MissionData->MissionEvent( COMP_DESTROY, this, bPlayerResponsible );
}


/*
================
idMover::Event_SetCallback
================
*/
void idMover::Event_SetCallback( void ) {
	if ( ( lastCommand == MOVER_ROTATING ) && !rotate_thread ) {
		lastCommand	= MOVER_NONE;
		rotate_thread = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	} else if ( ( lastCommand == MOVER_MOVING || lastCommand == MOVER_SPLINE ) && !move_thread ) {
		lastCommand	= MOVER_NONE;
		move_thread = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
================
idMover::VectorForDir
================
*/
void idMover::VectorForDir( float angle, idVec3 &vec ) {
	idAngles ang;

	switch( ( int )angle ) {
	case DIR_UP :
		vec.Set( 0, 0, 1 );
		break;

	case DIR_DOWN :
		vec.Set( 0, 0, -1 );
		break;

	case DIR_LEFT :
		physicsObj.GetLocalAngles( ang );
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		+= 90;
		vec			= ang.ToForward();
		break;

	case DIR_RIGHT :
		physicsObj.GetLocalAngles( ang );
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		-= 90;
		vec			= ang.ToForward();
		break;

	case DIR_FORWARD :
		physicsObj.GetLocalAngles( ang );
		ang.pitch	= 0;
		ang.roll	= 0;
		vec			= ang.ToForward();
		break;

	case DIR_BACK :
		physicsObj.GetLocalAngles( ang );
		ang.pitch	= 0;
		ang.roll	= 0;
		ang.yaw		+= 180;
		vec			= ang.ToForward();
		break;

	case DIR_REL_UP :
		vec.Set( 0, 0, 1 );
		break;

	case DIR_REL_DOWN :
		vec.Set( 0, 0, -1 );
		break;

	case DIR_REL_LEFT :
		physicsObj.GetLocalAngles( ang );
		ang.ToVectors( NULL, &vec );
		vec *= -1;
		break;

	case DIR_REL_RIGHT :
		physicsObj.GetLocalAngles( ang );
		ang.ToVectors( NULL, &vec );
		break;

	case DIR_REL_FORWARD :
		physicsObj.GetLocalAngles( ang );
		vec = ang.ToForward();
		break;

	case DIR_REL_BACK :
		physicsObj.GetLocalAngles( ang );
		vec = ang.ToForward() * -1;
		break;

	default:
		ang.Set( 0, angle, 0 );
		vec = GetWorldVector( ang.ToForward() );
		break;
	}
}

/*
================
idMover::FindGuiTargets
================
*/
void idMover::FindGuiTargets( void ) {
   	gameLocal.GetTargets( spawnArgs, guiTargets, "guiTarget" );
}

/*
==============================
idMover::SetGuiState

key/val will be set to any renderEntity->gui's on the list
==============================
*/
void idMover::SetGuiState( const char *key, const char *val ) const {
	gameLocal.Printf( "Setting %s to %s\n", key, val );
	for( int i = 0; i < guiTargets.Num(); i++ ) {
		idEntity *ent = guiTargets[ i ].GetEntity();
		if ( ent ) {
			for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ j ] ) {
					ent->GetRenderEntity()->gui[ j ]->SetStateString( key, val );
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.time, true );
				}
			}
			ent->UpdateVisuals();
		}
	}
}

/*
================
idMover::Event_InitGuiTargets
================
*/
void idMover::Event_FindGuiTargets( void ) {
	FindGuiTargets();
}

/*
================
idMover::SetGuiStates
================
*/
void idMover::SetGuiStates( const char *state ) {
	int i;
	if ( guiTargets.Num() ) {
		SetGuiState( "movestate", state );
	}
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			renderEntity.gui[ i ]->SetStateString( "movestate", state );
			renderEntity.gui[ i ]->StateChanged( gameLocal.time, true );
		}
	}
}

/*
================
idMover::Event_InitGuiTargets
================
*/
void idMover::Event_InitGuiTargets( void ) {
	SetGuiStates( guiBinaryMoverStates[MOVER_POS1] );
}

/***********************************************************************

	Translation control functions
	
***********************************************************************/

/*
================
idMover::Event_StopMoving
================
*/
void idMover::Event_StopMoving( void ) {
	physicsObj.GetLocalOrigin( dest_position );
	DoneMoving();
}

/*
================
idMover::DoneMoving
================
*/
void idMover::DoneMoving( void ) {

	if (lastCommand != MOVER_SPLINE) {
		// set our final position so that we get rid of any numerical inaccuracy
		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
	}

	lastCommand	= MOVER_NONE;
	idThread::ObjectMoveDone( move_thread, this );
	move_thread = 0;

	StopSound( SND_CHANNEL_BODY, false );

	// grayman #3755 - if this is a door, stop any hard pushing an AI might have done
	if (IsType(CFrobDoor::Type))
	{
		static_cast<CFrobDoor*>(this)->StopPushingDoorHard();
	}
}

/*
================
idMover::UpdateMoveSound
================
*/
void idMover::UpdateMoveSound( moveStage_t stage ) {
	switch( stage ) {
		case ACCELERATION_STAGE: {
			StartSound( "snd_accel", SND_CHANNEL_BODY2, 0, false, NULL );
			StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			break;
		}
		case LINEAR_STAGE: {
			StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			break;
		}
		case DECELERATION_STAGE: {
			StopSound( SND_CHANNEL_BODY, false );
			StartSound( "snd_decel", SND_CHANNEL_BODY2, 0, false, NULL );
			break;
		}
		case FINISHED_STAGE: {
			StopSound( SND_CHANNEL_BODY, false );
			break;
		}
	}
}

/*
================
idMover::Event_UpdateMove
================
*/
void idMover::Event_UpdateMove( void ) {
	idVec3	org;

	physicsObj.GetLocalOrigin( org );

	UpdateMoveSound( move.stage );

	switch( move.stage ) {
		case ACCELERATION_STAGE: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.time, move.acceleration, org, move.dir, vec3_origin );
			if ( move.movetime > 0 ) {
				move.stage = LINEAR_STAGE;
			} else if ( move.deceleration > 0 ) {
				move.stage = DECELERATION_STAGE;
			} else {
				move.stage = FINISHED_STAGE;
			}
			break;
		}
		case LINEAR_STAGE: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.time, move.movetime, org, move.dir, vec3_origin );
			if ( move.deceleration ) {
				move.stage = DECELERATION_STAGE;
			} else {
				move.stage = FINISHED_STAGE;
			}
			break;
		}
		case DECELERATION_STAGE: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.time, move.deceleration, org, move.dir, vec3_origin );
			move.stage = FINISHED_STAGE;
			break;
		}
		case FINISHED_STAGE: {
			if ( g_debugMover.GetBool() ) {
				gameLocal.Printf( "%d: '%s' move done\n", gameLocal.time, name.c_str() );
			}
			DoneMoving();
			break;
		}
	}
}

/*
================
idMover::BeginMove
================
*/
void idMover::BeginMove( idThread *thread ) {
	moveStage_t stage;
	idVec3		org;
	float		dist;
	float		acceldist;
	int			totalacceltime;
	int			at;
	int			dt;

	lastCommand	= MOVER_MOVING;
	move_thread = 0;

	physicsObj.GetLocalOrigin( org );

	move_delta = dest_position - org;
	if ( move_delta.Compare( vec3_zero ) )
	{
		DoneMoving();
		return;
	}

	// grayman #3711 - Calculate the moveTime fraction according to the current rotation state
	// this is overridden by BinaryFrobMovers to achieve a flexible rotation move time.
	float moveTimeFraction = GetMoveTimeTranslationFraction();

	int moveTime = static_cast<int>(move_time*moveTimeFraction);

	// scale times up to whole physics frames
	at = idPhysics::SnapTimeToPhysicsFrame( acceltime );
	moveTime += at - acceltime;
	acceltime = at;
	dt = idPhysics::SnapTimeToPhysicsFrame( deceltime );
	moveTime += dt - deceltime;
	deceltime = dt;

	// if we're moving at a specific speed, we need to calculate the move time
	if ( move_speed )
	{
		dist = move_delta.Length();

		totalacceltime = acceltime + deceltime;

		// calculate the distance we'll move during acceleration and deceleration
		acceldist = totalacceltime * 0.5f * 0.001f * move_speed;
		if ( acceldist >= dist )
		{
			// going too slow for this distance to move at a constant speed
			moveTime = totalacceltime;
		} else
		{
			// calculate move time taking acceleration into account
			moveTime = static_cast<int>(totalacceltime + 1000.0f * ( dist - acceldist ) / move_speed);
		}
	}

	// scale time up to a whole physics frames
	moveTime = idPhysics::SnapTimeToPhysicsFrame( moveTime );

	if ( acceltime ) {
		stage = ACCELERATION_STAGE;
	} else if ( moveTime <= deceltime ) {
		stage = DECELERATION_STAGE;
	} else {
		stage = LINEAR_STAGE;
	}

	at = acceltime;
	dt = deceltime;

	if ( at + dt > moveTime ) {
		// there's no real correct way to handle this, so we just scale
		// the times to fit into the move time in the same proportions
		at = idPhysics::SnapTimeToPhysicsFrame( at * moveTime / ( at + dt ) );
		dt = moveTime - at;
	}

	move_delta = move_delta * ( 1000.0f / ( (float) moveTime - ( at + dt ) * 0.5f ) );

	move.stage			= stage;
	move.acceleration	= at;
	move.movetime		= moveTime - at - dt;
	move.deceleration	= dt;
	move.dir			= move_delta;

	ProcessEvent( &EV_ReachedPos );
}

/***********************************************************************

	Rotation control functions
	
***********************************************************************/

/*
================
idMover::Event_StopRotating
================
*/
void idMover::Event_StopRotating( void ) {
	physicsObj.GetLocalAngles( dest_angles );
	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
	DoneRotating();
}

/*
================
idMover::DoneRotating
================
*/
void idMover::DoneRotating( void ) {
	lastCommand = MOVER_NONE;
	idThread::ObjectMoveDone( rotate_thread, this );
	rotate_thread = 0;

	StopSound( SND_CHANNEL_BODY, false );

	// grayman #3967 - if this is a door, stop any hard pushing an AI might have done
	if (IsType(CFrobDoor::Type))
	{
		static_cast<CFrobDoor*>(this)->StopPushingDoorHard();
	}
}

/*
================
idMover::UpdateRotationSound
================
*/
void idMover::UpdateRotationSound( moveStage_t stage ) {
	switch( stage ) {
		case ACCELERATION_STAGE: {
			StartSound( "snd_accel", SND_CHANNEL_BODY2, 0, false, NULL );
			StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			break;
		}
		case LINEAR_STAGE: {
			StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			break;
		}
		case DECELERATION_STAGE: {
			StopSound( SND_CHANNEL_BODY, false );
			StartSound( "snd_decel", SND_CHANNEL_BODY2, 0, false, NULL );
			break;
		}
		case FINISHED_STAGE: {
			StopSound( SND_CHANNEL_BODY, false );
			break;
		}
	}
}

/*
================
idMover::Event_UpdateRotation
================
*/
void idMover::Event_UpdateRotation( void ) {
	idAngles	ang;

	physicsObj.GetLocalAngles( ang );

	UpdateRotationSound( rot.stage );

	switch( rot.stage ) {
		case ACCELERATION_STAGE: {
			physicsObj.SetAngularExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.time, rot.acceleration, ang, rot.rot, ang_zero );
			if ( rot.movetime > 0 ) {
				rot.stage = LINEAR_STAGE;
			} else if ( rot.deceleration > 0 ) {
				rot.stage = DECELERATION_STAGE;
			} else {
				rot.stage = FINISHED_STAGE;
			}
			break;
		}
		case LINEAR_STAGE: {
			if ( !stopRotation && !rot.deceleration ) {
				physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, rot.movetime, ang, rot.rot, ang_zero );
			} else {
				physicsObj.SetAngularExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.time, rot.movetime, ang, rot.rot, ang_zero );
			}

			if ( rot.deceleration ) {
				rot.stage = DECELERATION_STAGE;
			} else {
				rot.stage = FINISHED_STAGE;
			}
			break;
		}
		case DECELERATION_STAGE: {
			physicsObj.SetAngularExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.time, rot.deceleration, ang, rot.rot, ang_zero );
			rot.stage = FINISHED_STAGE;
			break;
		}
		case FINISHED_STAGE: {
			lastCommand	= MOVER_NONE;
			if ( stopRotation ) {
				// set our final angles so that we get rid of any numerical inaccuracy
				dest_angles.Normalize360();
				physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
				stopRotation = false;
			} else if ( physicsObj.GetAngularExtrapolationType() == EXTRAPOLATION_ACCELLINEAR ) {
				// keep our angular velocity constant
				physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, ang, rot.rot, ang_zero );
			}

			if ( g_debugMover.GetBool() ) {
				gameLocal.Printf( "%d: '%s' rotation done\n", gameLocal.time, name.c_str() );
			}

			DoneRotating();
			break;
		}
	}
}

float idMover::GetMoveTimeRotationFraction() // grayman #3711
{
	return 1.0f;
}

float idMover::GetMoveTimeTranslationFraction() // grayman #3711
{
	return 1.0f;
}

/*
================
idMover::BeginRotation
================
*/
void idMover::BeginRotation( idThread *thread, bool stopwhendone ) {
	moveStage_t stage;
	idAngles	ang;
	int			at;
	int			dt;

	lastCommand	= MOVER_ROTATING;
	rotate_thread = 0;

	int moveTime = move_time;

	// rotation always uses moveTime so that if a move was started before the rotation,
	// the rotation will take the same amount of time as the move.  If no move has been
	// started and no time is set, the rotation takes 1 second.
	if ( !moveTime ) {
		moveTime = 1;
	}

	physicsObj.GetLocalAngles( ang );
	angle_delta = dest_angles - ang;
	if ( angle_delta.Compare(ang_zero, VECTOR_EPSILON) ) {
		// set our final angles so that we get rid of any numerical inaccuracy
		dest_angles.Normalize360();
		physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
		stopRotation = false;
		DoneRotating();
		return;
	}

	// greebo: Calculate the moveTime fraction according to the current rotation state
	// this is overridden by BinaryFrobMovers to achieve a flexible rotation move time.
	float moveTimeFraction = GetMoveTimeRotationFraction(); // grayman #3711

	moveTime = static_cast<int>(moveTime*moveTimeFraction);

	// scale times up to whole physics frames
	at = idPhysics::SnapTimeToPhysicsFrame( acceltime );
	moveTime += at - acceltime;
	acceltime = at;
	dt = idPhysics::SnapTimeToPhysicsFrame( deceltime );
	moveTime += dt - deceltime;
	deceltime = dt;
	moveTime = idPhysics::SnapTimeToPhysicsFrame( moveTime );

	if ( acceltime ) {
		stage = ACCELERATION_STAGE;
	} else if ( moveTime <= deceltime ) {
		stage = DECELERATION_STAGE;
	} else {
		stage = LINEAR_STAGE;
	}

	at = acceltime;
	dt = deceltime;

	if ( at + dt > moveTime ) {
		// there's no real correct way to handle this, so we just scale
		// the times to fit into the move time in the same proportions
		at = idPhysics::SnapTimeToPhysicsFrame( at * moveTime / ( at + dt ) );
		dt = moveTime - at;
	}

	angle_delta = angle_delta * ( 1000.0f / ( (float) moveTime - ( at + dt ) * 0.5f ) );

	stopRotation = stopwhendone || ( dt != 0 );

	rot.stage			= stage;
	rot.acceleration	= at;
	rot.movetime		= moveTime - at - dt;
	rot.deceleration	= dt;
	rot.rot				= angle_delta;

	ProcessEvent( &EV_ReachedAng );
}

void idMover::OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity)
{
	// empty default implementation, overridden by subclasses
}

/***********************************************************************

	Script callable routines  
	
***********************************************************************/

/*
===============
idMover::Event_TeamBlocked
===============
*/
void idMover::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	if ( g_debugMover.GetBool() ) {
		gameLocal.Printf( "%d: '%s' stopped due to team member '%s' blocked by '%s'\n", gameLocal.time, name.c_str(), blockedEntity->name.c_str(), blockingEntity->name.c_str() );
	}

	// greebo: Pass the call to the virtual function
	OnTeamBlocked(blockedEntity, blockingEntity);
}

/*
===============
idMover::Event_PartBlocked
===============
*/
void idMover::Event_PartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
	if ( g_debugMover.GetBool() ) {
		gameLocal.Printf( "%d: '%s' blocked by '%s'\n", gameLocal.time, name.c_str(), blockingEntity->name.c_str() );
	}
}

/*
================
idMover::Event_SetMoveSpeed
================
*/
void idMover::Event_SetMoveSpeed( float speed ) {
	if ( speed <= 0 ) {
		gameLocal.Error( "Cannot set speed less than or equal to 0." );
	}

	move_speed = speed;
	move_time = 0;			// move_time is calculated for each move when move_speed is non-0
}

/*
================
idMover::Event_GetMoveSpeed
================
*/
void idMover::Event_GetMoveSpeed( void ) const {
	// tels #3209
	idThread::ReturnFloat( move_speed );
}

/*
================
idMover::Event_GetMoveTime
================
*/
void idMover::Event_GetMoveTime( void ) const {
	// tels #3209
	idThread::ReturnFloat( move_time );
}

// grayman #3029 - EAS needs this for station-to-station travel times
/*
================
idMover::GetMoveSpeed
================
*/
float idMover::GetMoveSpeed()
{
	return move_speed;
}

/*
================
idMover::Event_SetMoveTime
================
*/
void idMover::Event_SetMoveTime( float time ) {
	if ( time <= 0 ) {
		gameLocal.Error( "idMover::Event_SetMoveTime - '%s' - Cannot set time less than or equal to 0.",GetName());
	}

	move_speed = 0;
	move_time = SEC2MS( time );
}

/*
================
idMover::Event_SetAccellerationTime
================
*/
void idMover::Event_SetAccellerationTime( float time ) {
	if ( time < 0 ) {
		gameLocal.Error( "Cannot set acceleration time less than 0." );
	}

	acceltime = SEC2MS( time );
}

/*
================
idMover::Event_SetDecelerationTime
================
*/
void idMover::Event_SetDecelerationTime( float time ) {
	if ( time < 0 ) {
		gameLocal.Error( "Cannot set deceleration time less than 0." );
	}

	deceltime = SEC2MS( time );
}

/*
================
idMover::Event_MoveTo
================
*/
void idMover::Event_MoveTo( idEntity *ent ) {
	if ( !ent ) {
		gameLocal.Warning( "Entity not found" );
	}

	dest_position = GetLocalCoordinates( ent->GetPhysics()->GetOrigin() );
	BeginMove( idThread::CurrentThread() );
}

/*
================
idMover::MoveToPos
================
*/
void idMover::MoveToPos( const idVec3 &pos ) {
	dest_position = GetLocalCoordinates( pos );
	BeginMove( NULL );
}

/*
================
idMover::MoveToLocalPos
================
*/
void idMover::MoveToLocalPos( const idVec3 &pos ) {
	dest_position = pos;
	BeginMove( NULL );
}
/*
================
idMover::Event_MoveToPos
================
*/
void idMover::Event_MoveToPos( idVec3 &pos ) {
	MoveToPos( pos );
}

/*
================
idMover::Event_MoveDir
================
*/
void idMover::Event_MoveDir( float angle, float distance ) {
	idVec3 dir;
	idVec3 org;

	physicsObj.GetLocalOrigin( org );
	VectorForDir( angle, dir );
	dest_position = org + dir * distance;
	BeginMove( idThread::CurrentThread() );
}

/*
================
idMover::Event_MoveAccelerateTo
================
*/
void idMover::Event_MoveAccelerateTo( float speed, float time ) {
	float v;
	idVec3 org, dir;
	int at;

	if ( time < 0 ) {
		gameLocal.Error( "idMover::Event_MoveAccelerateTo: cannot set acceleration time less than 0." );
	}

	dir = physicsObj.GetLinearVelocity();
	v = dir.Normalize();

	// if not moving already
	if ( v == 0.0f ) {
		gameLocal.Error( "idMover::Event_MoveAccelerateTo: not moving." );
	}

	// if already moving faster than the desired speed
	if ( v >= speed ) {
		return;
	}

	at = idPhysics::SnapTimeToPhysicsFrame( SEC2MS( time ) );

	lastCommand	= MOVER_MOVING;

	physicsObj.GetLocalOrigin( org );

	move.stage			= ACCELERATION_STAGE;
	move.acceleration	= at;
	move.movetime		= 0;
	move.deceleration	= 0;

	StartSound( "snd_accel", SND_CHANNEL_BODY2, 0, false, NULL );
	StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.time, move.acceleration, org, dir * ( speed - v ), dir * v );
}

/*
================
idMover::Event_MoveDecelerateTo
================
*/
void idMover::Event_MoveDecelerateTo( float speed, float time ) {
	float v;
	idVec3 org, dir;
	int dt;

	if ( time < 0 ) {
		gameLocal.Error( "idMover::Event_MoveDecelerateTo: cannot set deceleration time less than 0." );
	}

	dir = physicsObj.GetLinearVelocity();
	v = dir.Normalize();

	// if not moving already
	if ( v == 0.0f ) {
		gameLocal.Error( "idMover::Event_MoveDecelerateTo: not moving." );
	}

	// if already moving slower than the desired speed
	if ( v <= speed ) {
		return;
	}

	dt = idPhysics::SnapTimeToPhysicsFrame( SEC2MS( time ) );

	lastCommand	= MOVER_MOVING;

	physicsObj.GetLocalOrigin( org );

	move.stage			= DECELERATION_STAGE;
	move.acceleration	= 0;
	move.movetime		= 0;
	move.deceleration	= dt;

	StartSound( "snd_decel", SND_CHANNEL_BODY2, 0, false, NULL );
	StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.time, move.deceleration, org, dir * ( v - speed ), dir * speed );
}

/*
================
idMover::Event_RotateDownTo
================
*/
void idMover::Event_RotateDownTo( int axis, float angle ) {
	idAngles ang;

	if ( ( axis < 0 ) || ( axis > 2 ) ) {
		gameLocal.Error( "Invalid axis" );
	}

	physicsObj.GetLocalAngles( ang );

	dest_angles[ axis ] = angle;
	if ( dest_angles[ axis ] > ang[ axis ] ) {
		dest_angles[ axis ] -= 360;
	}

	BeginRotation( idThread::CurrentThread(), true );
}

/*
================
idMover::Event_RotateUpTo
================
*/
void idMover::Event_RotateUpTo( int axis, float angle ) {
	idAngles ang;

	if ( ( axis < 0 ) || ( axis > 2 ) ) {
		gameLocal.Error( "Invalid axis" );
	}

	physicsObj.GetLocalAngles( ang );

	dest_angles[ axis ] = angle;
	if ( dest_angles[ axis ] < ang[ axis ] ) {
		dest_angles[ axis ] += 360;
	}

	BeginRotation( idThread::CurrentThread(), true );
}

/*
================
idMover::Event_RotateTo
================
*/
void idMover::Event_RotateTo( idAngles &angles ) {
	dest_angles = angles;
	BeginRotation( idThread::CurrentThread(), true );
}

/*
================
idMover::Event_Rotate
================
*/
void idMover::Event_Rotate( idAngles &angles ) {
	if ( rotate_thread ) {
		DoneRotating();
	}

	dest_angles = physicsObj.GetLocalAngles() + angles * ( move_time - ( acceltime + deceltime ) / 2 ) * 0.001f;

	BeginRotation( idThread::CurrentThread(), false );
}

/*
================
idMover::Event_RotateOnce
================
*/
void idMover::Event_RotateOnce( idAngles &angles ) {
	if ( rotate_thread ) {
		DoneRotating();
	}

	dest_angles = physicsObj.GetLocalAngles() + angles;

	BeginRotation( idThread::CurrentThread(), true );
}

/*
================
idMover::Event_Bob
================
*/
void idMover::Event_Bob( float speed, float phase, idVec3 &depth ) {
	idVec3 org;

	physicsObj.GetLocalOrigin( org );
	physicsObj.SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), static_cast<int>(speed * 1000 * phase), static_cast<int>(speed * 500), org, depth * 2.0f, vec3_origin );
}

/*
================
idMover::Event_Sway
================
*/
void idMover::Event_Sway( float speed, float phase, idAngles &depth ) {
	idAngles ang, angSpeed;
	float duration;

	physicsObj.GetLocalAngles( ang );
	assert ( speed > 0.0f );
	duration = idMath::Sqrt( depth[0] * depth[0] + depth[1] * depth[1] + depth[2] * depth[2] ) / speed;
	angSpeed = depth / ( duration * idMath::SQRT_1OVER2 );
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), static_cast<int>(duration * 1000.0f * phase), static_cast<int>(duration * 1000.0f), ang, angSpeed, ang_zero );
}

/*
================
idMover::Event_OpenPortal

Sets the portal associated with this mover to be open
================
*/
void idMover::Event_OpenPortal( void ) 
{
	// angua: call the virtual function
	OpenPortal();
}

/*
================
idMover::Event_ClosePortal

Sets the portal associatated with this mover to be closed
================
*/
void idMover::Event_ClosePortal( void ) 
{
	// angua: call the virtual function
	ClosePortal();
}


/*
================
idMover::OpenPortal

Sets the portal associated with this mover to be open
================
*/
void idMover::OpenPortal() {
	if ( areaPortal ) {
		SetPortalState( true );
	}
}

/*
================
idMover::ClosePortal

Sets the portal associated with this mover to be closed
================
*/
void idMover::ClosePortal() {
	if ( areaPortal ) {
		SetPortalState( false );
	}
}




/*
================
idMover::Event_SetAccelSound
================
*/
void idMover::Event_SetAccelSound( const char *sound ) {
//	refSound.SetSound( "accel", sound );
}

/*
================
idMover::Event_SetDecelSound
================
*/
void idMover::Event_SetDecelSound( const char *sound ) {
//	refSound.SetSound( "decel", sound );
}

/*
================
idMover::Event_SetMoveSound
================
*/
void idMover::Event_SetMoveSound( const char *sound ) {
//	refSound.SetSound( "move", sound );
}

/*
================
idMover::Event_EnableSplineAngles
================
*/
void idMover::Event_EnableSplineAngles( void ) {
	useSplineAngles = true;
}

/*
================
idMover::Event_DisableSplineAngles
================
*/
void idMover::Event_DisableSplineAngles( void ) {
	useSplineAngles = false;
}

/*
================
idMover::Event_RemoveInitialSplineAngles
================
*/
void idMover::Event_RemoveInitialSplineAngles( void ) {
	idCurve_Spline<idVec3> *spline;
	idAngles ang;

	spline = physicsObj.GetSpline();
	if ( !spline ) {
		return;
	}
	ang = spline->GetCurrentFirstDerivative( 0 ).ToAngles();
	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, -ang, ang_zero, ang_zero );
}

/*
================
idMover::Event_StartSpline
================
*/
void idMover::Event_StartSpline( idEntity *splineEntity ) {
	idCurve_Spline<idVec3> *spline;

	if ( !splineEntity ) {
		return;
	}

	// Needed for savegames
	splineEnt = splineEntity;

	spline = splineEntity->GetSpline();
	if ( !spline ) {
		return;
	}

	lastCommand = MOVER_SPLINE;
	move_thread = 0;

	if ( acceltime + deceltime > move_time ) {
		acceltime = move_time / 2;
		deceltime = move_time - acceltime;
	}
	move.stage			= FINISHED_STAGE;
	move.acceleration	= acceltime;
	move.movetime		= move_time;
	move.deceleration	= deceltime;

	spline->MakeUniform( move_time );
	spline->ShiftTime( gameLocal.time - spline->GetTime( 0 ) );

	physicsObj.SetSpline( spline, move.acceleration, move.deceleration, useSplineAngles );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
}

/*
================
idMover::Event_StopSpline
================
*/
void idMover::Event_StopSpline( void ) {
	physicsObj.SetSpline( NULL, 0, 0, useSplineAngles );
	splineEnt = NULL;
}

/*
================
idMover::Event_Activate
================
*/
void idMover::Event_Activate( idEntity *activator ) {
	Show();
	Event_StartSpline( this );
}

/*
================
idMover::Event_IsMoving
================
*/
void idMover::Event_IsMoving( void ) {
	if ( physicsObj.GetLinearExtrapolationType() == EXTRAPOLATION_NONE ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idMover::Event_IsRotating
================
*/
void idMover::Event_IsRotating( void ) {
	if ( physicsObj.GetAngularExtrapolationType() == EXTRAPOLATION_NONE ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}

/*
================
idMover::WriteToSnapshot
================
*/
void idMover::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	msg.WriteBits( move.stage, 3 );
	msg.WriteBits( rot.stage, 3 );
	WriteBindToSnapshot( msg );
	WriteGUIToSnapshot( msg );
}

/*
================
idMover::ReadFromSnapshot
================
*/
void idMover::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	moveStage_t oldMoveStage = move.stage;
	moveStage_t oldRotStage = rot.stage;

	physicsObj.ReadFromSnapshot( msg );
	move.stage = (moveStage_t) msg.ReadBits( 3 );
	rot.stage = (moveStage_t) msg.ReadBits( 3 );
	ReadBindFromSnapshot( msg );
	ReadGUIFromSnapshot( msg );

	if ( msg.HasChanged() ) {
		if ( move.stage != oldMoveStage ) {
			UpdateMoveSound( oldMoveStage );
		}
		if ( rot.stage != oldRotStage ) {
			UpdateRotationSound( oldRotStage );
		}
		UpdateVisuals();
	}
}

/*
================
idMover::SetPortalState
================
*/
void idMover::SetPortalState( bool open ) {
	assert( areaPortal );
	gameLocal.SetPortalState( areaPortal, open ? PS_BLOCK_NONE : PS_BLOCK_ALL );
}

/*
================
idMover::IsBlocked
================
*/
bool idMover::IsBlocked( void )
{
	const trace_t* trace = physicsObj.GetBlockingInfo();
	return (trace != NULL);
}

/*
================
idMover::SetCanPushPlayer
================
*/
bool idMover::SetCanPushPlayer( bool newSetting )
{
	// flag has opposite meaning to parameter. NOT the param and return value.
	return !(physicsObj.pushFlagOverride( PUSHFL_NOPLAYER, !newSetting ));
}


/*
===============================================================================

	idSplinePath, holds a spline path to be used by an idMover

===============================================================================
*/

CLASS_DECLARATION( idEntity, idSplinePath )
END_CLASS

/*
================
idSplinePath::idSplinePath
================
*/
idSplinePath::idSplinePath() {
}

/*
================
idSplinePath::Spawn
================
*/
void idSplinePath::Spawn( void ) {
}


/*
===============================================================================

idMover_Binary

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"

===============================================================================
*/

const idEventDef EV_Mover_ReturnToPos1( "<returntopos1>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Mover_MatchTeam( "<matchteam>", EventArgs('d', "", "", 'd', "", ""), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Mover_Enable( "enable", EventArgs(), EV_RETURNS_VOID, "Enables the mover/trigger" );
const idEventDef EV_Mover_Disable( "disable", EventArgs(), EV_RETURNS_VOID, "Disables the mover/trigger" );

CLASS_DECLARATION( idEntity, idMover_Binary )
	EVENT( EV_FindGuiTargets,			idMover_Binary::Event_FindGuiTargets )
	EVENT( EV_Thread_SetCallback,		idMover_Binary::Event_SetCallback )
	EVENT( EV_Mover_ReturnToPos1,		idMover_Binary::Event_ReturnToPos1 )
	EVENT( EV_Activate,					idMover_Binary::Event_Use_BinaryMover )
	EVENT( EV_ReachedPos,				idMover_Binary::Event_Reached_BinaryMover )
	EVENT( EV_Mover_MatchTeam,			idMover_Binary::Event_MatchActivateTeam )
	EVENT( EV_Mover_Enable,				idMover_Binary::Event_Enable )
	EVENT( EV_Mover_Disable,			idMover_Binary::Event_Disable )
	EVENT( EV_Mover_OpenPortal,			idMover_Binary::Event_OpenPortal )
	EVENT( EV_Mover_ClosePortal,		idMover_Binary::Event_ClosePortal )
	EVENT( EV_Mover_InitGuiTargets,		idMover_Binary::Event_InitGuiTargets )
END_CLASS

/*
================
idMover_Binary::idMover_Binary()
================
*/
idMover_Binary::idMover_Binary()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);

	pos1.Zero();
	pos2.Zero();
	moverState = MOVER_POS1;
	moveMaster = NULL;
	activateChain = NULL;
	soundPos1 = 0;
	sound1to2 = 0;
	sound2to1 = 0;
	soundPos2 = 0;
	soundLoop = 0;
	wait = 0.0f;
	damage = 0.0f;
	duration = 0;
	accelTime = 0;
	decelTime = 0;
	activatedBy = this;
	stateStartTime = 0;
	team.Clear();
	enabled = false;
	move_thread = 0;
	updateStatus = 0;
	areaPortal = 0;
	blocked = false;
	m_FrobActionScript = "frob_binary_mover";
}

/*
================
idMover_Binary::~idMover_Binary
================
*/
idMover_Binary::~idMover_Binary()
{
	idMover_Binary *mover;

	// if this is the mover master
	if ( this == moveMaster ) {
		// make the next mover in the chain the move master
		for ( mover = moveMaster; mover; mover = mover->activateChain ) {
			mover->moveMaster = this->activateChain;
		}
	}
	else {
		// remove mover from the activate chain
		for ( mover = moveMaster; mover; mover = mover->activateChain ) {
			if ( mover->activateChain == this ) {
				mover->activateChain = this->activateChain;
				break;
			}
		}
	}
}

/*
================
idMover_Binary::Save
================
*/
void idMover_Binary::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteVec3( pos1 );
	savefile->WriteVec3( pos2 );
	savefile->WriteInt( (moverState_t)moverState );

	savefile->WriteObject( moveMaster );
	savefile->WriteObject( activateChain );

	savefile->WriteInt( soundPos1 );
	savefile->WriteInt( sound1to2 );
	savefile->WriteInt( sound2to1 );
	savefile->WriteInt( soundPos2 );
	savefile->WriteInt( soundLoop );

	savefile->WriteFloat( wait );
	savefile->WriteFloat( damage );

	savefile->WriteInt( duration );
	savefile->WriteInt( accelTime );
	savefile->WriteInt( decelTime );

	activatedBy.Save( savefile );

	savefile->WriteInt( stateStartTime );
	savefile->WriteString( team );
	savefile->WriteBool( enabled );

	savefile->WriteInt( move_thread );
	savefile->WriteInt( updateStatus );

	savefile->WriteInt( buddies.Num() );
	for ( i = 0; i < buddies.Num(); i++ ) {
		savefile->WriteString( buddies[ i ] );
	}

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteInt( areaPortal );
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}
	savefile->WriteBool( blocked );

	savefile->WriteInt( guiTargets.Num() );
	for( i = 0; i < guiTargets.Num(); i++ ) {
		guiTargets[ i ].Save( savefile );
	}
}

/*
================
idMover_Binary::Restore
================
*/
void idMover_Binary::Restore( idRestoreGame *savefile ) {
	int		i, num, portalState;
	idStr	temp;

	savefile->ReadVec3( pos1 );
	savefile->ReadVec3( pos2 );
	savefile->ReadInt( (int &)moverState );

	savefile->ReadObject( reinterpret_cast<idClass *&>( moveMaster ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( activateChain ) );

	savefile->ReadInt( soundPos1 );
	savefile->ReadInt( sound1to2 );
	savefile->ReadInt( sound2to1 );
	savefile->ReadInt( soundPos2 );
	savefile->ReadInt( soundLoop );

	savefile->ReadFloat( wait );
	savefile->ReadFloat( damage );

	savefile->ReadInt( duration );
	savefile->ReadInt( accelTime );
	savefile->ReadInt( decelTime );

	activatedBy.Restore( savefile );

	savefile->ReadInt( stateStartTime );

	savefile->ReadString( team );
	savefile->ReadBool( enabled );

	savefile->ReadInt( move_thread );
	savefile->ReadInt( updateStatus );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadString( temp );
		buddies.Append( temp );
	}

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadInt( areaPortal );
	if ( areaPortal ) {
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}
	savefile->ReadBool( blocked );

	guiTargets.Clear();
	savefile->ReadInt( num );
	guiTargets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		guiTargets[ i ].Restore( savefile );
	}
}

/*
================
idMover_Binary::Spawn

Base class for all movers.

"wait"		wait before returning (3 default, -1 = never return)
"speed"		movement speed
================
*/
void idMover_Binary::Spawn( void )
{
	idEntity	*ent;
	const char	*temp;

	move_thread		= 0;
	enabled			= true;
	areaPortal		= 0;

	activateChain = NULL;

	spawnArgs.GetFloat( "wait", "0", wait );

	spawnArgs.GetInt( "updateStatus", "0", updateStatus );

	const idKeyValue *kv = spawnArgs.MatchPrefix( "buddy", NULL );
	while( kv ) {
		buddies.Append( kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "buddy", kv );
	}

	spawnArgs.GetString( "team", "", &temp );
	team = temp;

	if ( !team.Length() ) {
		ent = this;
	} else {
		// find the first entity spawned on this team (which could be us)
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( idMover_Binary::Type ) && !idStr::Icmp( static_cast<idMover_Binary *>(ent)->team.c_str(), temp ) ) {
				break;
			}
		}
		if ( !ent ) {
			ent = this;
		}
	}
	moveMaster = static_cast<idMover_Binary *>(ent);

	// create a physics team for the binary mover parts
	if ( ent != this ) {
		gameLocal.Error("Mover teams support dropped (entity %s)", ent->name.c_str());
	}

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "solid", "1" ) ) {
		physicsObj.SetContents( 0 );
	}
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetAxis().ToAngles(), ang_zero, ang_zero );
	SetPhysics( &physicsObj );

	if ( moveMaster != this ) {
		JoinActivateTeam( moveMaster );
	}

	idBounds soundOrigin;
	idMover_Binary *slave;

	soundOrigin.Clear();
	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		soundOrigin += slave->GetPhysics()->GetAbsBounds();
	}
	moveMaster->refSound.origin = soundOrigin.GetCenter();

	if ( spawnArgs.MatchPrefix( "guiTarget" ) ) {
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindGuiTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindGuiTargets();
		}
	}
}

/*
===============
idMover_Binary::GetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void idMover_Binary::GetMovedir( float angle, idVec3 &movedir ) {
	if ( angle == -1 ) {
		movedir.Set( 0, 0, 1 );
	} else if ( angle == -2 ) {
		movedir.Set( 0, 0, -1 );
	} else {
		movedir = idAngles( 0, angle, 0 ).ToForward();
	}
}

/*
================
idMover_Binary::Event_SetCallback
================
*/
void idMover_Binary::Event_SetCallback( void ) {
	if ( ( moverState == MOVER_1TO2 ) || ( moverState == MOVER_2TO1 ) ) {
		move_thread = idThread::CurrentThreadNum();
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idMover_Binary::UpdateMoverSound
===============
*/
void idMover_Binary::UpdateMoverSound( moverState_t state ) {
	if ( moveMaster == this ) {
		switch( state ) {
			case MOVER_POS1:
				break;
			case MOVER_POS2:
				break;
			case MOVER_1TO2:
				StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
				break;
			case MOVER_2TO1:
				StartSound( "snd_close", SND_CHANNEL_ANY, 0, false, NULL );
				break;
		}
	}
}

/*
===============
idMover_Binary::SetMoverState
===============
*/
void idMover_Binary::SetMoverState( moverState_t newstate, int time ) {
	moverState = newstate;
	move_thread = 0;

	UpdateMoverSound( newstate );

	stateStartTime = time;
	switch( moverState ) {
		case MOVER_POS1: {
			Signal( SIG_MOVER_POS1 );
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, time, 0, pos1, vec3_origin, vec3_origin );
			break;
		}
		case MOVER_POS2: {
			Signal( SIG_MOVER_POS2 );
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, time, 0, pos2, vec3_origin, vec3_origin );
			break;
		}
		case MOVER_1TO2: {
			Signal( SIG_MOVER_1TO2 );
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, time, duration, pos1, ( pos2 - pos1 ) * 1000.0f / duration, vec3_origin );
			if ( accelTime != 0 || decelTime != 0 ) {
				physicsObj.SetLinearInterpolation( time, accelTime, decelTime, duration, pos1, pos2 );
			} else {
				physicsObj.SetLinearInterpolation( 0, 0, 0, 0, pos1, pos2 );
			}
			break;
		}
		case MOVER_2TO1: {
			Signal( SIG_MOVER_2TO1 );
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, time, duration, pos2, ( pos1 - pos2 ) * 1000.0f / duration, vec3_origin );
			if ( accelTime != 0 || decelTime != 0 ) {
				physicsObj.SetLinearInterpolation( time, accelTime, decelTime, duration, pos2, pos1 );
			} else {
				physicsObj.SetLinearInterpolation( 0, 0, 0, 0, pos1, pos2 );
			}
			break;
		}
	}
}

/*
================
idMover_Binary::MatchActivateTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void idMover_Binary::MatchActivateTeam( moverState_t newstate, int time ) {
	idMover_Binary *slave;

	for ( slave = this; slave != NULL; slave = slave->activateChain ) {
		slave->SetMoverState( newstate, time );
	}
}

/*
================
idMover_Binary::Enable
================
*/
void idMover_Binary::Enable( bool b ) {
	enabled = b;
}

/*
================
idMover_Binary::Event_MatchActivateTeam
================
*/
void idMover_Binary::Event_MatchActivateTeam( moverState_t newstate, int time ) {
	MatchActivateTeam( newstate, time );
}

/*
================
idMover_Binary::BindTeam

All entities in a mover team will be bound 
================
*/
void idMover_Binary::BindTeam( idEntity *bindTo ) {
	idMover_Binary *slave;

	for ( slave = this; slave != NULL; slave = slave->activateChain ) {
		slave->Bind( bindTo, true );
	}
}

/*
================
idMover_Binary::JoinActivateTeam

Set all entities in a mover team to be enabled
================
*/
void idMover_Binary::JoinActivateTeam( idMover_Binary *master ) {
	this->activateChain = master->activateChain;
	master->activateChain = this;
}

/*
================
idMover_Binary::Event_Enable

Set all entities in a mover team to be enabled
================
*/
void idMover_Binary::Event_Enable( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->Enable( false );
	}
}

/*
================
idMover_Binary::Event_Disable

Set all entities in a mover team to be disabled
================
*/
void idMover_Binary::Event_Disable( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->Enable( false );
	}
}

/*
================
idMover_Binary::Event_OpenPortal

Sets the portal associated with this mover to be open
================
*/
void idMover_Binary::Event_OpenPortal( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		if ( slave->areaPortal ) {
			slave->SetPortalState( true );
		}
	}
}

/*
================
idMover_Binary::Event_ClosePortal

Sets the portal associated with this mover to be closed
================
*/
void idMover_Binary::Event_ClosePortal( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		if ( !slave->IsHidden() ) {
			if ( slave->areaPortal ) {
				slave->SetPortalState( false );
			}
		}
	}
}

/*
================
idMover_Binary::Event_ReturnToPos1
================
*/
void idMover_Binary::Event_ReturnToPos1( void ) {
	MatchActivateTeam( MOVER_2TO1, gameLocal.time );
}

/*
================
idMover_Binary::Event_Reached_BinaryMover
================
*/
void idMover_Binary::Event_Reached_BinaryMover( void ) {

	if ( moverState == MOVER_1TO2 ) {
		// reached pos2
		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		if ( moveMaster == this ) {
			StartSound( "snd_opened", SND_CHANNEL_ANY, 0, false, NULL );
		}

		SetMoverState( MOVER_POS2, gameLocal.time );

		SetGuiStates( guiBinaryMoverStates[MOVER_POS2] );

		UpdateBuddies( 1 );

		if ( enabled && wait >= 0 && !spawnArgs.GetBool( "toggle" ) ) {
			// return to pos1 after a delay
			PostEventSec( &EV_Mover_ReturnToPos1, wait );
		}
		
		// fire targets
		ActivateTargets( moveMaster->GetActivator() );


		SetBlocked(false);

	} else if ( moverState == MOVER_2TO1 ) {
		// reached pos1
		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		SetMoverState( MOVER_POS1, gameLocal.time );

		SetGuiStates( guiBinaryMoverStates[MOVER_POS1] );

		UpdateBuddies( 0 );

		// close areaportals
		if ( moveMaster == this ) {
			ProcessEvent( &EV_Mover_ClosePortal );
		}

		if ( enabled && wait >= 0 && spawnArgs.GetBool( "continuous" ) ) {
			PostEventSec( &EV_Activate, wait, this );
		}


		SetBlocked(false);

	} else {
		gameLocal.Error( "Event_Reached_BinaryMover: bad moverState" );
	}
}

/*
================
idMover_Binary::GotoPosition1
================
*/
void idMover_Binary::GotoPosition1( void ) {
	idMover_Binary *slave;
	int	partial;

	// only the master should control this
	if ( moveMaster != this ) {
		moveMaster->GotoPosition1();
		return;
	}

	SetGuiStates( guiBinaryMoverStates[MOVER_2TO1] );

	if ( ( moverState == MOVER_POS1 ) || ( moverState == MOVER_2TO1 ) ) {
		// already there, or on the way
		return;
	}

	if ( moverState == MOVER_POS2 ) {
		for ( slave = this; slave != NULL; slave = slave->activateChain ) {
			slave->CancelEvents( &EV_Mover_ReturnToPos1 );
		}
		if ( !spawnArgs.GetBool( "toggle" ) ) {
			ProcessEvent( &EV_Mover_ReturnToPos1 );
		}
		return;
	}

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		assert( partial >= 0 );
		if ( partial < 0 ) {
			partial = 0;
		}
		MatchActivateTeam( MOVER_2TO1, physicsObj.GetTime() - partial );
		// if already at at position 1 (partial == duration) execute the reached event
		if ( partial >= duration ) {
			Event_Reached_BinaryMover();
		}
	}
}

/*
================
idMover_Binary::GotoPosition2
================
*/
void idMover_Binary::GotoPosition2( void ) {
	int	partial;

	// only the master should control this
	if ( moveMaster != this ) {
		moveMaster->GotoPosition2();
		return;
	}

	SetGuiStates( guiBinaryMoverStates[MOVER_1TO2] );

	if ( ( moverState == MOVER_POS2 ) || ( moverState == MOVER_1TO2 ) ) {
		// already there, or on the way
		return;
	}

	if ( moverState == MOVER_POS1 ) {
		MatchActivateTeam( MOVER_1TO2, gameLocal.time );

		// open areaportal
		ProcessEvent( &EV_Mover_OpenPortal );
		return;
	}


	// only partway up before reversing
	if ( moverState == MOVER_2TO1 ) {
		// use the physics times because this might be executed during the physics simulation
		partial = physicsObj.GetLinearEndTime() - physicsObj.GetTime();
		assert( partial >= 0 );
		if ( partial < 0 ) {
			partial = 0;
		}
		MatchActivateTeam( MOVER_1TO2, physicsObj.GetTime() - partial );
		// if already at at position 2 (partial == duration) execute the reached event
		if ( partial >= duration ) {
			Event_Reached_BinaryMover();
		}
	}
}

/*
================
idMover_Binary::UpdateBuddies
================
*/
void idMover_Binary::UpdateBuddies( int val ) {
	int i, c;

	if ( updateStatus == 2 ) {
		 c = buddies.Num();
		for ( i = 0; i < c; i++ ) {
			idEntity *buddy = gameLocal.FindEntity( buddies[i] );
			if ( buddy ) {
				buddy->SetShaderParm( SHADERPARM_MODE, val );
				buddy->UpdateVisuals();
			}
		}
	}
}

/*
================
idMover_Binary::SetGuiStates
================
*/
void idMover_Binary::SetGuiStates( const char *state ) {
	if ( guiTargets.Num() ) {
		SetGuiState( "movestate", state );
	}

	idMover_Binary *mb = activateChain;
	while( mb ) {
		if ( mb->guiTargets.Num() ) {
			mb->SetGuiState( "movestate", state );
		}
		mb = mb->activateChain;
	}
}

/*
================
idMover_Binary::Use_BinaryMover
================
*/
void idMover_Binary::Use_BinaryMover( idEntity *activator ) {
	// only the master should be used
	if ( moveMaster != this ) {
		moveMaster->Use_BinaryMover( activator );
		return;
	}

	if ( !enabled ) {
		return;
	}

	activatedBy = activator;

	if ( moverState == MOVER_POS1 ) {
		// FIXME: start moving USERCMD_MSEC later, because if this was player
		// triggered, gameLocal.time hasn't been advanced yet
		MatchActivateTeam( MOVER_1TO2, gameLocal.time + USERCMD_MSEC );

		SetGuiStates( guiBinaryMoverStates[MOVER_1TO2] );
		// open areaportal
		ProcessEvent( &EV_Mover_OpenPortal );
		return;
	}

	// if all the way up, just delay before coming down
	if ( moverState == MOVER_POS2 ) {
		idMover_Binary *slave;

		if ( wait == -1 ) {
			return;
		}

		SetGuiStates( guiBinaryMoverStates[MOVER_2TO1] );

		for ( slave = this; slave != NULL; slave = slave->activateChain ) {
			slave->CancelEvents( &EV_Mover_ReturnToPos1 );
			slave->PostEventSec( &EV_Mover_ReturnToPos1, spawnArgs.GetBool( "toggle" ) ? 0 : wait );
		}
		return;
	}

	// only partway down before reversing
	if ( moverState == MOVER_2TO1 ) {
		GotoPosition2();
		return;
	}

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		GotoPosition1();
		return;
	}
}

/*
================
idMover_Binary::Event_Use_BinaryMover
================
*/
void idMover_Binary::Event_Use_BinaryMover( idEntity *activator ) {
	Use_BinaryMover( activator );
}

/*
================
idMover_Binary::PreBind
================
*/
void idMover_Binary::PreBind( void ) {
	pos1 = GetWorldCoordinates( pos1 );
	pos2 = GetWorldCoordinates( pos2 );
}

/*
================
idMover_Binary::PostBind
================
*/
void idMover_Binary::PostBind( void ) {
	pos1 = GetLocalCoordinates( pos1 );
	pos2 = GetLocalCoordinates( pos2 );
}

/*
================
idMover_Binary::FindGuiTargets
================
*/
void idMover_Binary::FindGuiTargets( void ) {
   	gameLocal.GetTargets( spawnArgs, guiTargets, "guiTarget" );
}

/*
==============================
idMover_Binary::SetGuiState

key/val will be set to any renderEntity->gui's on the list
==============================
*/
void idMover_Binary::SetGuiState( const char *key, const char *val ) const {
	int i;

	for( i = 0; i < guiTargets.Num(); i++ ) {
		idEntity *ent = guiTargets[ i ].GetEntity();
		if ( ent ) {
			for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ j ] ) {
					ent->GetRenderEntity()->gui[ j ]->SetStateString( key, val );
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.time, true );
				}
			}
			ent->UpdateVisuals();
		}
	}
}

/*
================
idMover_Binary::Event_InitGuiTargets
================
*/
void idMover_Binary::Event_FindGuiTargets( void ) {
	FindGuiTargets();
}

/*
================
idMover_Binary::Event_InitGuiTargets
================
*/
void idMover_Binary::Event_InitGuiTargets( void ) {
	if ( guiTargets.Num() ) {
		SetGuiState( "movestate", guiBinaryMoverStates[MOVER_POS1] );
	}
}

/*
================
idMover_Binary::InitSpeed

pos1, pos2, and speed are passed in so the movement delta can be calculated
================
*/
void idMover_Binary::InitSpeed( idVec3 &mpos1, idVec3 &mpos2, float mspeed, float maccelTime, float mdecelTime ) {
	idVec3		move;
	float		distance;
	float		speed;

	pos1		= mpos1;
	pos2		= mpos2;

	accelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( maccelTime ) );
	decelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mdecelTime ) );

	speed		= mspeed ? mspeed : 100;

	// calculate time to reach second position from speed
	move = pos2 - pos1;
	distance = move.Length();
	duration = idPhysics::SnapTimeToPhysicsFrame( static_cast<int>(distance * 1000 / speed) );
	if ( duration <= 0 ) {
		duration = 1;
	}

	moverState = MOVER_POS1;

	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, pos1, vec3_origin, vec3_origin );
	physicsObj.SetLinearInterpolation( 0, 0, 0, 0, vec3_origin, vec3_origin );
	SetOrigin( pos1 );

	PostEventMS( &EV_Mover_InitGuiTargets, 0 );
}

/*
================
idMover_Binary::InitTime

pos1, pos2, and time are passed in so the movement delta can be calculated
================
*/
void idMover_Binary::InitTime( idVec3 &mpos1, idVec3 &mpos2, float mtime, float maccelTime, float mdecelTime ) {

	pos1		= mpos1;
	pos2		= mpos2;

	accelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( maccelTime ) );
	decelTime	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mdecelTime ) );

	duration	= idPhysics::SnapTimeToPhysicsFrame( SEC2MS( mtime ) );
	if ( duration <= 0 ) {
		duration = 1;
	}

	moverState = MOVER_POS1;

	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, pos1, vec3_origin, vec3_origin );
	physicsObj.SetLinearInterpolation( 0, 0, 0, 0, vec3_origin, vec3_origin );
	SetOrigin( pos1 );

	PostEventMS( &EV_Mover_InitGuiTargets, 0 );
}

/*
================
idMover_Binary::SetBlocked
================
*/
void idMover_Binary::SetBlocked( bool b ) {
	for ( idMover_Binary *slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		slave->blocked = b;
		if ( b ) {
			const idKeyValue *kv = slave->spawnArgs.MatchPrefix( "triggerBlocked" );
			while( kv ) {
				idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
				if ( ent ) {
					ent->PostEventMS( &EV_Activate, 0, moveMaster->GetActivator() );
				}
				kv = slave->spawnArgs.MatchPrefix( "triggerBlocked", kv );
			}
		}
	}
}

/*
================
idMover_Binary::IsBlocked
================
*/
bool idMover_Binary::IsBlocked( void ) {
	return blocked;
}

/*
================
idMover_Binary::GetActivator
================
*/
idEntity *idMover_Binary::GetActivator( void ) const {
	return activatedBy.GetEntity();
}

/*
================
idMover_Binary::WriteToSnapshot
================
*/
void idMover_Binary::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	msg.WriteBits( moverState, 3 );
	WriteBindToSnapshot( msg );
}

/*
================
idMover_Binary::ReadFromSnapshot
================
*/
void idMover_Binary::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	moverState_t oldMoverState = moverState;

	physicsObj.ReadFromSnapshot( msg );
	moverState = (moverState_t) msg.ReadBits( 3 );
	ReadBindFromSnapshot( msg );

	if ( msg.HasChanged() ) {
		if ( moverState != oldMoverState ) {
			UpdateMoverSound( moverState );
		}
		UpdateVisuals();
	}
}

/*
================
idMover_Binary::SetPortalState
================
*/
void idMover_Binary::SetPortalState( bool open ) {
	assert( areaPortal );
	gameLocal.SetPortalState( areaPortal, open ? PS_BLOCK_NONE : PS_BLOCK_ALL );
}

/*
===============================================================================

idPlat

===============================================================================
*/

CLASS_DECLARATION( idMover_Binary, idPlat )
	EVENT( EV_Touch,			idPlat::Event_Touch )
	EVENT( EV_TeamBlocked,		idPlat::Event_TeamBlocked )
	EVENT( EV_PartBlocked,		idPlat::Event_PartBlocked )
END_CLASS

/*
===============
idPlat::idPlat
===============
*/
idPlat::idPlat( void ) {
	trigger = NULL;
	localTriggerOrigin.Zero();
	localTriggerAxis.Identity();
}

/*
===============
idPlat::~idPlat
===============
*/
idPlat::~idPlat( void ) {
	if ( trigger ) {
		delete trigger;
	}
}

/*
===============
idPlat::Save
===============
*/
void idPlat::Save( idSaveGame *savefile ) const {
	savefile->WriteClipModel( trigger );
	savefile->WriteVec3( localTriggerOrigin );
	savefile->WriteMat3( localTriggerAxis );
}

/*
===============
idPlat::Restore
===============
*/
void idPlat::Restore( idRestoreGame *savefile ) {
	savefile->ReadClipModel( trigger );
	savefile->ReadVec3( localTriggerOrigin );
	savefile->ReadMat3( localTriggerAxis );
}

/*
===============
idPlat::Spawn
===============
*/
void idPlat::Spawn( void )
{
	float	lip;
	float	height;
	float	time;
	float	speed;
	float	accel;
	float	decel;
	bool	noTouch;

	spawnArgs.GetFloat( "speed", "100", speed );
	spawnArgs.GetFloat( "damage", "0", damage );
	spawnArgs.GetFloat( "wait", "1", wait );
	spawnArgs.GetFloat( "lip", "8", lip );
	spawnArgs.GetFloat( "accel_time", "0.25", accel );
	spawnArgs.GetFloat( "decel_time", "0.25", decel );

	// create second position
	if ( !spawnArgs.GetFloat( "height", "0", height ) ) {
		height = ( GetPhysics()->GetBounds()[1][2] - GetPhysics()->GetBounds()[0][2] ) - lip;
	}

	spawnArgs.GetBool( "no_touch", "0", noTouch );

	// pos1 is the rest (bottom) position, pos2 is the top
	pos2 = GetPhysics()->GetOrigin();
	pos1 = pos2;
	pos1[2] -= height;

	if ( spawnArgs.GetFloat( "time", "1", time ) ) {
		InitTime( pos1, pos2, time, accel, decel );
	} else {
		InitSpeed( pos1, pos2, speed, accel, decel );
	}

	SetMoverState( MOVER_POS1, gameLocal.time );
	UpdateVisuals();

	// spawn the trigger if one hasn't been custom made
	if ( !noTouch ) {
		// spawn trigger
		SpawnPlatTrigger( pos1 );
	}
}

/*
================
idPlat::Think
================
*/
void idPlat::Think( void ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	idMover_Binary::Think();

	if ( thinkFlags & TH_PHYSICS ) {
		// update trigger position
		if ( GetMasterPosition( masterOrigin, masterAxis ) ) {
			if ( trigger ) {
				trigger->Link( gameLocal.clip, this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
			}
		}
	}
}

/*
================
idPlat::PreBind
================
*/
void idPlat::PreBind( void ) {
	idMover_Binary::PreBind();
}

/*
================
idPlat::PostBind
================
*/
void idPlat::PostBind( void ) {
	idMover_Binary::PostBind();
	GetLocalTriggerPosition( trigger );
}

/*
================
idPlat::GetLocalTriggerPosition
================
*/
void idPlat::GetLocalTriggerPosition( const idClipModel *trigger ) {
	idVec3 origin;
	idMat3 axis;

	if ( !trigger ) {
		return;
	}

	GetMasterPosition( origin, axis );
	localTriggerOrigin = ( trigger->GetOrigin() - origin ) * axis.Transpose();
	localTriggerAxis = trigger->GetAxis() * axis.Transpose();
}

/*
==============
idPlat::SpawnPlatTrigger
===============
*/
void idPlat::SpawnPlatTrigger( idVec3 &pos ) {
	idBounds		bounds;
	idVec3			tmin;
	idVec3			tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position

	bounds = GetPhysics()->GetBounds();

	tmin[0] = bounds[0][0] + 33;
	tmin[1] = bounds[0][1] + 33;
	tmin[2] = bounds[0][2];

	tmax[0] = bounds[1][0] - 33;
	tmax[1] = bounds[1][1] - 33;
	tmax[2] = bounds[1][2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ( bounds[0][0] + bounds[1][0] ) * 0.5f;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ( bounds[0][1] + bounds[1][1] ) * 0.5f;
		tmax[1] = tmin[1] + 1;
	}
	
	trigger = new idClipModel( idTraceModel( idBounds( tmin, tmax ) ) );
	trigger->Link( gameLocal.clip, this, 255, GetPhysics()->GetOrigin(), mat3_identity );
	trigger->SetContents( CONTENTS_TRIGGER );
}

/*
==============
idPlat::Event_Touch
===============
*/
void idPlat::Event_Touch( idEntity *other, trace_t *trace ) {
	if ( !other->IsType( idPlayer::Type ) ) {
		return;
	}

	if ( ( GetMoverState() == MOVER_POS1 ) && trigger && ( trace->c.id == trigger->GetId() ) && ( other->health > 0 ) ) {
		Use_BinaryMover( other );
	}
}

/*
================
idPlat::Event_TeamBlocked
================
*/
void idPlat::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	// reverse direction
	Use_BinaryMover( activatedBy.GetEntity() );
}

/*
===============
idPlat::Event_PartBlocked
===============
*/
void idPlat::Event_PartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
}


/*
===============================================================================

idMover_Periodic

===============================================================================
*/

CLASS_DECLARATION( idEntity, idMover_Periodic )
	EVENT( EV_TeamBlocked,		idMover_Periodic::Event_TeamBlocked )
	EVENT( EV_PartBlocked,		idMover_Periodic::Event_PartBlocked )
END_CLASS

/*
===============
idMover_Periodic::idMover_Periodic
===============
*/
idMover_Periodic::idMover_Periodic( void ) {
	damage = 0.0f;
	fl.neverDormant	= false;
}

/*
===============
idMover_Periodic::Spawn
===============
*/
void idMover_Periodic::Spawn( void ) {
	spawnArgs.GetFloat( "damage", "0", damage );
	if ( !spawnArgs.GetBool( "solid", "1" ) ) {
		GetPhysics()->SetContents( 0 );
	}
}

/*
===============
idMover_Periodic::Save
===============
*/
void idMover_Periodic::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( damage );
	savefile->WriteStaticObject( physicsObj );
}

/*
===============
idMover_Periodic::Restore
===============
*/
void idMover_Periodic::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( damage );
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
}

/*
================
idMover_Periodic::Think
================
*/
void idMover_Periodic::Think( void ) {
	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() ) {
		return;
	}

	RunPhysics();
	Present();
}

/*
===============
idMover_Periodic::Event_TeamBlocked
===============
*/
void idMover_Periodic::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
}

/*
===============
idMover_Periodic::Event_PartBlocked
===============
*/
void idMover_Periodic::Event_PartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
}

/*
================
idMover_Periodic::WriteToSnapshot
================
*/
void idMover_Periodic::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
}

/*
================
idMover_Periodic::ReadFromSnapshot
================
*/
void idMover_Periodic::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

idRotater

===============================================================================
*/

CLASS_DECLARATION( idMover_Periodic, idRotater )
	EVENT( EV_Activate,			idRotater::Event_Activate )
END_CLASS

/*
===============
idRotater::idRotater
===============
*/
idRotater::idRotater( void ) :
	nextTriggerDirectionIsForward(true)
{
	activatedBy = this;
}

/*
===============
idRotater::Spawn
===============
*/
void idRotater::Spawn( void )
{
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, gameLocal.time, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, GetPhysics()->GetAxis().ToAngles(), ang_zero, ang_zero );
	SetPhysics( &physicsObj );

	if ( spawnArgs.GetBool( "start_on" ) ) {
		ProcessEvent( &EV_Activate, this );
	}
}

/*
===============
idRotater::Save
===============
*/
void idRotater::Save( idSaveGame *savefile ) const
{
	activatedBy.Save( savefile );
	savefile->WriteBool(nextTriggerDirectionIsForward);
}

/*
===============
idRotater::Restore
===============
*/
void idRotater::Restore( idRestoreGame *savefile )
{
	activatedBy.Restore( savefile );
	savefile->ReadBool(nextTriggerDirectionIsForward);
}

/*
===============
idRotater::Event_Activate
===============
*/
void idRotater::Event_Activate( idEntity *activator )
{
	activatedBy = activator;

	bool isRotating = spawnArgs.GetBool("rotate");

	// greebo: If rotate direction inversion is activated, toggle the boolean, else leave it alone
	if (!isRotating && spawnArgs.GetBool("invert_on_trigger", "0"))
	{
		nextTriggerDirectionIsForward = !nextTriggerDirectionIsForward;
	}

	// greebo: Invert the "rotate" spawnarg, this toggles the rotation itself
	spawnArgs.Set("rotate", isRotating ? "0" : "1");

	// Start or stop the rotation, based on the spawnargs (forward direction)
	SetRotationFromSpawnargs(nextTriggerDirectionIsForward);
}

void idRotater::SetRotationFromSpawnargs(bool forward)
{
	idAngles delta(0,0,0);

	if (spawnArgs.GetBool("rotate"))
	{
		// We should be rotating, read the parameters
		float speed = spawnArgs.GetFloat("speed", "100");
		bool x_axis = spawnArgs.GetBool("x_axis", "0");
		bool y_axis = spawnArgs.GetBool("y_axis", "0");
		bool z_axis = !x_axis && !y_axis;
		bool applyDirectionFix = spawnArgs.GetBool("apply_direction_fix", "0");

		// Invert the speed if the direction boolean is false
		if (!forward) 
		{
			speed *= -1;
		}

		if (applyDirectionFix)
		{
			idAngles curAngles = physicsObj.GetAxis().ToAngles().Normalize360();
			idAngles spawnAngles = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().Normalize360();

			// greebo: Euler angles are ambiguous - it might happen that a positive speed value
			// applied to "roll" means something different than it would have meant at spawn time
			// To yield the same orientation with a different set of angles one usually has 
			// to employ two 180 degree rotations - check this. If anybody has a more elegant way of checking that, let me know!
			if (x_axis && fabs(spawnAngles[1] - curAngles[1]) == 180 && fabs(spawnAngles[0] - curAngles[0]) == 180)
			{
				speed *= -1;
			}
			else if (y_axis && fabs(spawnAngles[1] - curAngles[1]) == 180 && fabs(spawnAngles[2] - curAngles[2]) == 180)
			{
				speed *= -1;
			}
			else if (z_axis && fabs(spawnAngles[0] - curAngles[0]) == 180 && fabs(spawnAngles[2] - curAngles[2]) == 180)
			{
				speed *= -1;
			}
		}
		
		// Set the axis of rotation
		if (x_axis) // roll
		{
			delta[2] = speed;
		}
		else if (y_axis) // pitch
		{
			delta[0] = speed;
		}
		else // z_axis == yaw
		{
			delta[1] = speed;
		}
	}

	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, physicsObj.GetAxis().ToAngles().Normalize360(), delta, ang_zero );
}

void idRotater::SetDirection(bool forward)
{
	// Read the rotation parameters again, but consider the direction
	SetRotationFromSpawnargs(forward);
}

/*
===============================================================================

idBobber

===============================================================================
*/

CLASS_DECLARATION( idMover_Periodic, idBobber )
END_CLASS

/*
===============
idBobber::idBobber
===============
*/
idBobber::idBobber( void ) {
}

/*
===============
idBobber::Spawn
===============
*/
void idBobber::Spawn( void )
{
	float	speed;
	float	height;
	float	phase;
	bool	x_axis;
	bool	y_axis;
	idVec3	delta;

	spawnArgs.GetFloat( "speed", "4", speed );
	spawnArgs.GetFloat( "height", "32", height );
	spawnArgs.GetFloat( "phase", "0", phase );
	spawnArgs.GetBool( "x_axis", "0", x_axis );
	spawnArgs.GetBool( "y_axis", "0", y_axis );

	// set the axis of bobbing
	delta = vec3_origin;
	if ( x_axis ) {
		delta[ 0 ] = height;
	} else if ( y_axis ) {
		delta[ 1 ] = height;
	} else {
		delta[ 2 ] = height;
	}

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), static_cast<int>(phase * 1000), static_cast<int>(speed * 500), GetPhysics()->GetOrigin(), delta * 2.0f, vec3_origin );
	SetPhysics( &physicsObj );
}


/*
===============================================================================

idPendulum

===============================================================================
*/

CLASS_DECLARATION( idMover_Periodic, idPendulum )
END_CLASS

/*
===============
idPendulum::idPendulum
===============
*/
idPendulum::idPendulum( void ) {
}

/*
===============
idPendulum::Spawn
===============
*/
void idPendulum::Spawn( void )
{
	float	speed;
	float	freq;
	float	length;
	float	phase;

	spawnArgs.GetFloat( "speed", "30", speed );
	spawnArgs.GetFloat( "phase", "0", phase );

	if ( spawnArgs.GetFloat( "freq", "", freq ) ) {
		if ( freq <= 0.0f ) {
			gameLocal.Error( "Invalid frequency on entity '%s'", GetName() );
		}
	} else {
		// find pendulum length
		length = idMath::Fabs( GetPhysics()->GetBounds()[0][2] );
		if ( length < 8 ) {
			length = 8;
		}

		freq = 1 / ( idMath::TWO_PI ) * idMath::Sqrt( g_gravity.GetFloat() / ( 3 * length ) );
	}

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), static_cast<int>(phase * 1000), static_cast<int>(500/freq), GetPhysics()->GetAxis().ToAngles(), idAngles( 0, 0, speed * 2.0f ), ang_zero );
	SetPhysics( &physicsObj );
}


/*
===============================================================================

idBobber

===============================================================================
*/

CLASS_DECLARATION( idMover_Periodic, idRiser )
EVENT( EV_Activate,				idRiser::Event_Activate )
END_CLASS

/*
===============
idRiser::idRiser
===============
*/
idRiser::idRiser( void ) {
}

/*
===============
idRiser::Spawn
===============
*/
void idRiser::Spawn( void )
{
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );

	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "solid", "1" ) ) {
		physicsObj.SetContents( 0 );
	}
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
	SetPhysics( &physicsObj );
}

/*
================
idRiser::Event_Activate
================
*/
void idRiser::Event_Activate( idEntity *activator ) {

	if ( !IsHidden() && spawnArgs.GetBool("hide")  ) {
		Hide();
	} else {
		Show();
		float	time;
		float	height;
		idVec3	delta;

		spawnArgs.GetFloat( "time", "4", time );
		spawnArgs.GetFloat( "height", "32", height );

		delta = vec3_origin;
		delta[ 2 ] = height;

		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.time, static_cast<int>(time * 1000), physicsObj.GetOrigin(), delta, vec3_origin );
	}
}


