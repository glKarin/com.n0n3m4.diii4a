/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "Player.h"
#include "ai/AI.h"

#include "Mover.h"

#include "Fx.h"

#include "bc_meta.h"
#include "bc_lever.h"
#include "bc_ventpeek.h"
#include "framework/DeclEntityDef.h"
#include "bc_doorbarricade.h"

// _D3XP : rename all gameLocal.time to gameLocal.slow.time for merge!

// a mover will update any gui entities in it's target list with
// a key/val pair of "mover" "state" from below.. guis can represent
// realtime info like this
// binary only
static const char *guiBinaryMoverStates[] = {
	"1",	// pos 1
	"2",	// pos 2
	"3",	// moving 1 to 2
	"4"		// moving 2 to 1
};

// SW: Number of milliseconds of grace time that a ragdoll can clip into a door/ventdoor without colliding with it
const int SQUEEZE_GRACE_MS = 100;

const int  PUSHAWAY_FORCE = 160;

const int VACUUMDOOR_THINKINTERVAL = 500; //BC When door is locked, how often to check whether room has been re-pressurized. We don't need to check this that often because we fictionally expect a delay between the window repair & door unlock.



const int AUTOBUTTON_UP = 60;
const int AUTOBUTTON_RIGHT = 42;
const int AUTOBUTTON_FORWARD = 16;


/*
===============================================================================

idMover

===============================================================================
*/

const idEventDef EV_FindGuiTargets( "<FindGuiTargets>", NULL );
const idEventDef EV_TeamBlocked( "<teamblocked>", "ee" );
const idEventDef EV_PartBlocked( "<partblocked>", "e" );
const idEventDef EV_ReachedPos( "<reachedpos>", NULL );
const idEventDef EV_ReachedAng( "<reachedang>", NULL );
const idEventDef EV_PostRestore( "<postrestore>", "ddddd" );
const idEventDef EV_StopMoving( "stopMoving", NULL );
const idEventDef EV_StopRotating( "stopRotating", NULL );
const idEventDef EV_Speed( "speed", "f" );
const idEventDef EV_Time( "time", "f" );
const idEventDef EV_AccelTime( "accelTime", "f" );
const idEventDef EV_DecelTime( "decelTime", "f" );
const idEventDef EV_MoveTo( "moveTo", "e" );
const idEventDef EV_MoveToPos( "moveToPos", "v" );
const idEventDef EV_Move( "move", "ff" );
const idEventDef EV_MoveAccelerateTo( "accelTo", "ff" );
const idEventDef EV_MoveDecelerateTo( "decelTo", "ff" );
const idEventDef EV_RotateDownTo( "rotateDownTo", "df" );
const idEventDef EV_RotateUpTo( "rotateUpTo", "df" );
const idEventDef EV_RotateTo( "rotateTo", "v" );
const idEventDef EV_Rotate( "rotate", "v" );
const idEventDef EV_RotateOnce( "rotateOnce", "v" );
const idEventDef EV_Bob( "bob", "ffv" );
const idEventDef EV_Sway( "sway", "ffv" );
const idEventDef EV_Mover_OpenPortal( "openPortal" );
const idEventDef EV_Mover_ClosePortal( "closePortal" );
const idEventDef EV_AccelSound( "accelSound", "s" );
const idEventDef EV_DecelSound( "decelSound", "s" );
const idEventDef EV_MoveSound( "moveSound", "s" );
const idEventDef EV_Mover_InitGuiTargets( "<initguitargets>", NULL );
const idEventDef EV_EnableSplineAngles( "enableSplineAngles", NULL );
const idEventDef EV_DisableSplineAngles( "disableSplineAngles", NULL );
const idEventDef EV_RemoveInitialSplineAngles( "removeInitialSplineAngles", NULL );
const idEventDef EV_StartSpline( "startSpline", "e" );
const idEventDef EV_StopSpline( "stopSpline", NULL );
const idEventDef EV_IsMoving( "isMoving", NULL, 'd' );
const idEventDef EV_IsRotating( "isRotating", NULL, 'd' );
const idEventDef EV_MoveToLocalPos("moveToLocalPos", "v");



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
	EVENT( EV_MoveToLocalPos,		idMover::Event_MoveToLocalPos )

	
END_CLASS

/*
================
idMover::idMover
================
*/
idMover::idMover( void ) {
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
	deceltime = 0;
	acceltime = 0;
	stopRotation = false;
	useSplineAngles = true;
	lastCommand = MOVER_NONE;
	damage = 0.0f;
	areaPortal = 0;
	fl.networkSync = true;
}

/*
================
idMover::Save
================
*/
void idMover::Save( idSaveGame *savefile ) const {
	savefile->WriteStaticObject( idMover::physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	// moveState_t move
	savefile->WriteInt( move.stage ); // moveStage_t stage
	savefile->WriteInt( move.acceleration ); // int acceleration
	savefile->WriteInt( move.movetime ); // int movetime
	savefile->WriteInt( move.deceleration ); // int deceleration
	savefile->WriteVec3( move.dir ); // idVec3 dir

	// rotationState_t rot
	savefile->WriteInt( rot.stage ); // moveStage_t stage
	savefile->WriteInt( rot.acceleration ); // int acceleration
	savefile->WriteInt( rot.movetime ); // int movetime
	savefile->WriteInt( rot.deceleration ); // int deceleration
	savefile->WriteAngles( rot.rot ); // idAngles rot

	savefile->WriteInt( move_thread ); // int move_thread
	savefile->WriteInt( rotate_thread ); // int rotate_thread
	savefile->WriteAngles( dest_angles ); // idAngles dest_angles
	savefile->WriteAngles( angle_delta ); // idAngles angle_delta
	savefile->WriteVec3( dest_position ); // idVec3 dest_position
	savefile->WriteVec3( move_delta ); // idVec3 move_delta
	savefile->WriteFloat( move_speed ); // float move_speed
	savefile->WriteInt( move_time ); // int move_time
	savefile->WriteInt( deceltime ); // int deceltime
	savefile->WriteInt( acceltime ); // int acceltime
	savefile->WriteBool( stopRotation ); // bool stopRotation
	savefile->WriteBool( useSplineAngles ); // bool useSplineAngles
	savefile->WriteObject( splineEnt ); // idEntityPtr<idEntity> splineEnt

	savefile->WriteInt( lastCommand ); // moverCommand_t lastCommand
	savefile->WriteFloat( damage ); // float damage

	savefile->WriteInt( areaPortal ); // int areaPortal
	if ( areaPortal > 0 ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}

	SaveFileWriteArray(guiTargets, guiTargets.Num(), WriteObject ); // idList< idEntityPtr<idEntity> > guiTargets

	// SM: Spline stuff from original Doom 3 code
	// https://github.com/id-Software/DOOM-3/blob/a9c49da5afb18201d31e3f0a429a037e56ce2b9a/neo/d3xp/Mover.cpp#L213-L226
	if (splineEnt.GetEntity() && splineEnt.GetEntity()->GetSpline()) {
		idCurve_Spline<idVec3>* spline = physicsObj.GetSpline();

		savefile->WriteBool(true);
		savefile->WriteInt(spline->GetTime(0));
		savefile->WriteInt(spline->GetTime(spline->GetNumValues() - 1) - spline->GetTime(0));
		savefile->WriteInt(physicsObj.GetSplineAcceleration());
		savefile->WriteInt(physicsObj.GetSplineDeceleration());
		savefile->WriteInt((int)physicsObj.UsingSplineAngles());
	}
	else {
		savefile->WriteBool(false);
	}
}

/*
================
idMover::Restore
================
*/
void idMover::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	// moveState_t move
	savefile->ReadInt( (int&)move.stage ); // moveStage_t stage
	savefile->ReadInt( move.acceleration ); // int acceleration
	savefile->ReadInt( move.movetime ); // int movetime
	savefile->ReadInt( move.deceleration ); // int deceleration
	savefile->ReadVec3( move.dir ); // idVec3 dir

	// rotationState_t rot
	savefile->ReadInt( (int&)rot.stage ); // moveStage_t stage
	savefile->ReadInt( rot.acceleration ); // int acceleration
	savefile->ReadInt( rot.movetime ); // int movetime
	savefile->ReadInt( rot.deceleration ); // int deceleration
	savefile->ReadAngles( rot.rot ); // idAngles rot

	savefile->ReadInt( move_thread ); // int move_thread
	savefile->ReadInt( rotate_thread ); // int rotate_thread
	savefile->ReadAngles( dest_angles ); // idAngles dest_angles
	savefile->ReadAngles( angle_delta ); // idAngles angle_delta
	savefile->ReadVec3( dest_position ); // idVec3 dest_position
	savefile->ReadVec3( move_delta ); // idVec3 move_delta
	savefile->ReadFloat( move_speed ); // float move_speed
	savefile->ReadInt( move_time ); // int move_time
	savefile->ReadInt( deceltime ); // int deceltime
	savefile->ReadInt( acceltime ); // int acceltime
	savefile->ReadBool( stopRotation ); // bool stopRotation
	savefile->ReadBool( useSplineAngles ); // bool useSplineAngles
	savefile->ReadObject( splineEnt ); // idEntityPtr<idEntity> splineEnt

	savefile->ReadInt( (int&)lastCommand ); // moverCommand_t lastCommand
	savefile->ReadFloat( damage ); // float damage

	savefile->ReadInt( areaPortal ); // int areaPortal
	if ( areaPortal > 0 ) {
		int portalState = 0;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}

	SaveFileReadArray(guiTargets, ReadObject ); // idList< idEntityPtr<idEntity> > guiTargets

	// SM: Spline stuff from original Doom 3 code
	// https://github.com/id-Software/DOOM-3/blob/a9c49da5afb18201d31e3f0a429a037e56ce2b9a/neo/d3xp/Mover.cpp#L286-L302
	bool hasSpline = false;
	savefile->ReadBool(hasSpline);
	if (hasSpline) {
		int starttime;
		int totaltime;
		int accel;
		int decel;
		int useAngles;

		savefile->ReadInt(starttime);
		savefile->ReadInt(totaltime);
		savefile->ReadInt(accel);
		savefile->ReadInt(decel);
		savefile->ReadInt(useAngles);

		PostEventMS(&EV_PostRestore, 0, starttime, totaltime, accel, decel, useAngles);
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
		common->Warning( "Invalid spline entity during restore\n" );
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
void idMover::Spawn( void ) {
	move_thread		= 0;
	rotate_thread	= 0;
	stopRotation	= false;
	lastCommand		= MOVER_NONE;

	acceltime		= 1000.0f * spawnArgs.GetFloat( "accel_time", "0" );
	deceltime		= 1000.0f * spawnArgs.GetFloat( "decel_time", "0" );
	move_time		= 1000.0f * spawnArgs.GetFloat( "move_time", "1" );	// safe default value
	move_speed		= spawnArgs.GetFloat( "move_speed", "0" );

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
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
	SetPhysics( &physicsObj );

	// see if we are on an areaportal
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );

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
	if ( spawnArgs.GetBool( "solid", "1" ) ) {
		physicsObj.SetContents( CONTENTS_SOLID );
	}
	SetPhysics( &physicsObj );
}

/*
============
idMover::Killed
============
*/
void idMover::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	fl.takedamage = false;
	ActivateTargets( this );
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
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.slow.time, true );
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
			renderEntity.gui[ i ]->StateChanged( gameLocal.slow.time, true );
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

	if ( lastCommand != MOVER_SPLINE ) {
		// set our final position so that we get rid of any numerical inaccuracy
		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
	}

	lastCommand	= MOVER_NONE;
	idThread::ObjectMoveDone( move_thread, this );
	move_thread = 0;

	StopSound( SND_CHANNEL_BODY, false );
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

	//common->Printf("pos %s %f %f %f \n", GetName(), GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z);

	physicsObj.GetLocalOrigin( org );

	UpdateMoveSound( move.stage );

	switch( move.stage ) {
		case ACCELERATION_STAGE: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.slow.time, move.acceleration, org, move.dir, vec3_origin );
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
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.slow.time, move.movetime, org, move.dir, vec3_origin );
			if ( move.deceleration ) {
				move.stage = DECELERATION_STAGE;
			} else {
				move.stage = FINISHED_STAGE;
			}
			break;
		}
		case DECELERATION_STAGE: {
			physicsObj.SetLinearExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.slow.time, move.deceleration, org, move.dir, vec3_origin );
			move.stage = FINISHED_STAGE;
			break;
		}
		case FINISHED_STAGE: {
			if ( g_debugMover.GetBool() ) {
				gameLocal.Printf( "%d: '%s' move done\n", gameLocal.slow.time, name.c_str() );
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
	if ( move_delta.Compare( vec3_zero ) ) {
		DoneMoving();
		return;
	}

	// scale times up to whole physics frames
	at = idPhysics::SnapTimeToPhysicsFrame( acceltime );
	move_time += at - acceltime;
	acceltime = at;
	dt = idPhysics::SnapTimeToPhysicsFrame( deceltime );
	move_time += dt - deceltime;
	deceltime = dt;

	// if we're moving at a specific speed, we need to calculate the move time
	if ( move_speed ) {
		dist = move_delta.Length();

		totalacceltime = acceltime + deceltime;

		// calculate the distance we'll move during acceleration and deceleration
		acceldist = totalacceltime * 0.5f * 0.001f * move_speed;
		if ( acceldist >= dist ) {
			// going too slow for this distance to move at a constant speed
			move_time = totalacceltime;
		} else {
			// calculate move time taking acceleration into account
			move_time = totalacceltime + 1000.0f * ( dist - acceldist ) / move_speed;
		}
	}

	// scale time up to a whole physics frames
	move_time = idPhysics::SnapTimeToPhysicsFrame( move_time );

	if ( acceltime ) {
		stage = ACCELERATION_STAGE;
	} else if ( move_time <= deceltime ) {
		stage = DECELERATION_STAGE;
	} else {
		stage = LINEAR_STAGE;
	}

	at = acceltime;
	dt = deceltime;

	if ( at + dt > move_time ) {
		// there's no real correct way to handle this, so we just scale
		// the times to fit into the move time in the same proportions
		at = idPhysics::SnapTimeToPhysicsFrame( at * move_time / ( at + dt ) );
		dt = move_time - at;
	}

	move_delta = move_delta * ( 1000.0f / ( (float) move_time - ( at + dt ) * 0.5f ) );

	move.stage			= stage;
	move.acceleration	= at;
	move.movetime		= move_time - at - dt;
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
	lastCommand	= MOVER_NONE;
	idThread::ObjectMoveDone( rotate_thread, this );
	rotate_thread = 0;

	StopSound( SND_CHANNEL_BODY, false );
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
			physicsObj.SetAngularExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.slow.time, rot.acceleration, ang, rot.rot, ang_zero );
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
				physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.slow.time, rot.movetime, ang, rot.rot, ang_zero );
			} else {
				physicsObj.SetAngularExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.slow.time, rot.movetime, ang, rot.rot, ang_zero );
			}

			if ( rot.deceleration ) {
				rot.stage = DECELERATION_STAGE;
			} else {
				rot.stage = FINISHED_STAGE;
			}
			break;
		}
		case DECELERATION_STAGE: {
			physicsObj.SetAngularExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.slow.time, rot.deceleration, ang, rot.rot, ang_zero );
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
				physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.slow.time, 0, ang, rot.rot, ang_zero );
			}

			if ( g_debugMover.GetBool() ) {
				gameLocal.Printf( "%d: '%s' rotation done\n", gameLocal.slow.time, name.c_str() );
			}

			DoneRotating();
			break;
		}
	}
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

	// rotation always uses move_time so that if a move was started before the rotation,
	// the rotation will take the same amount of time as the move.  If no move has been
	// started and no time is set, the rotation takes 1 second.
	if ( !move_time ) {
		move_time = 1;
	}

	physicsObj.GetLocalAngles( ang );
	angle_delta = dest_angles - ang;
	if ( angle_delta == ang_zero ) {
		// set our final angles so that we get rid of any numerical inaccuracy
		dest_angles.Normalize360();
		physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_angles, ang_zero, ang_zero );
		stopRotation = false;
		DoneRotating();
		return;
	}

	// scale times up to whole physics frames
	at = idPhysics::SnapTimeToPhysicsFrame( acceltime );
	move_time += at - acceltime;
	acceltime = at;
	dt = idPhysics::SnapTimeToPhysicsFrame( deceltime );
	move_time += dt - deceltime;
	deceltime = dt;
	move_time = idPhysics::SnapTimeToPhysicsFrame( move_time );

	if ( acceltime ) {
		stage = ACCELERATION_STAGE;
	} else if ( move_time <= deceltime ) {
		stage = DECELERATION_STAGE;
	} else {
		stage = LINEAR_STAGE;
	}

	at = acceltime;
	dt = deceltime;

	if ( at + dt > move_time ) {
		// there's no real correct way to handle this, so we just scale
		// the times to fit into the move time in the same proportions
		at = idPhysics::SnapTimeToPhysicsFrame( at * move_time / ( at + dt ) );
		dt = move_time - at;
	}

	angle_delta = angle_delta * ( 1000.0f / ( (float) move_time - ( at + dt ) * 0.5f ) );

	stopRotation = stopwhendone || ( dt != 0 );

	rot.stage			= stage;
	rot.acceleration	= at;
	rot.movetime		= move_time - at - dt;
	rot.deceleration	= dt;
	rot.rot				= angle_delta;

	ProcessEvent( &EV_ReachedAng );
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
		gameLocal.Printf( "%d: '%s' stopped due to team member '%s' blocked by '%s'\n", gameLocal.slow.time, name.c_str(), blockedEntity->name.c_str(), blockingEntity->name.c_str() );
	}
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
		gameLocal.Printf( "%d: '%s' blocked by '%s'\n", gameLocal.slow.time, name.c_str(), blockingEntity->name.c_str() );
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
idMover::Event_SetMoveTime
================
*/
void idMover::Event_SetMoveTime( float time ) {
	if ( time <= 0 ) {
		gameLocal.Error( "Cannot set time less than or equal to 0." );
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
idMover::Event_MoveToPos
================
*/
void idMover::Event_MoveToPos( idVec3 &pos ) {
	MoveToPos( pos );
}

/*
================
idMover::Event_MoveToLocalPos

SW: This lets the mover go to a location in local space (e.g. if the mover is bound to something else)
================
*/
void idMover::Event_MoveToLocalPos(idVec3& pos) {
	dest_position = pos;
	BeginMove(NULL);
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
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_ACCELLINEAR, gameLocal.slow.time, move.acceleration, org, dir * ( speed - v ), dir * v );
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
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_DECELLINEAR, gameLocal.slow.time, move.deceleration, org, dir * ( v - speed ), dir * speed );
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
	idAngles ang;

	if ( rotate_thread ) {
		DoneRotating();
	}

	physicsObj.GetLocalAngles( ang );
	dest_angles = ang + angles * ( move_time - ( acceltime + deceltime ) / 2 ) * 0.001f;

	BeginRotation( idThread::CurrentThread(), false );
}

/*
================
idMover::Event_RotateOnce
================
*/
void idMover::Event_RotateOnce( idAngles &angles ) {
	idAngles ang;

	if ( rotate_thread ) {
		DoneRotating();
	}

	physicsObj.GetLocalAngles( ang );
	dest_angles = ang + angles;

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
	// SM: If the start time of the bob is greater than the gameLocal.time,
	// allow the bob to begin immediately but maintain the phase
	int startTime = speed * 1000 * phase;
	if ( startTime > gameLocal.time )
	{
		startTime = gameLocal.time - startTime;
	}
	physicsObj.SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), startTime, speed * 500, org, depth * 2.0f, vec3_origin );
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
	// SM: If the start time of the sway is greater than the gameLocal.time,
	// allow the sway to begin immediately but maintain the phase
	int startTime = duration * 1000.0f * phase;
	if ( startTime > gameLocal.time )
	{
		startTime = gameLocal.time - startTime;
	}
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), startTime, duration * 1000.0f, ang, angSpeed, ang_zero );
}

/*
================
idMover::Event_OpenPortal

Sets the portal associtated with this mover to be open
================
*/
void idMover::Event_OpenPortal( void ) {
	if ( areaPortal ) {
		SetPortalState( true );
	}
}

/*
================
idMover::Event_ClosePortal

Sets the portal associtated with this mover to be closed
================
*/
void idMover::Event_ClosePortal( void ) {
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
	spline->ShiftTime( gameLocal.slow.time - spline->GetTime( 0 ) );

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
bool idMover::Event_IsMoving( void ) {
	if ( physicsObj.GetLinearExtrapolationType() == EXTRAPOLATION_NONE ) {
		idThread::ReturnInt( false );
		return false;
	} else {
		idThread::ReturnInt( true );
		return true;
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



//When a mover is repaired.
void idMover::DoRepairTick(int amount)
{
	fl.takedamage = spawnArgs.GetBool("takedamage", "1");
	health = maxHealth;
	needsRepair = false;
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

idElevator

===============================================================================
*/
const idEventDef EV_PostArrival( "postArrival", NULL );
const idEventDef EV_GotoFloor( "gotoFloor", "d" );
#ifdef _D3XP
const idEventDef EV_SetGuiStates( "setGuiStates" );
#endif

CLASS_DECLARATION( idMover, idElevator )
	EVENT( EV_Activate,				idElevator::Event_Activate )
	EVENT( EV_TeamBlocked,			idElevator::Event_TeamBlocked )
	EVENT( EV_PartBlocked,			idElevator::Event_PartBlocked )
	EVENT( EV_PostArrival,			idElevator::Event_PostFloorArrival )
	EVENT( EV_GotoFloor,			idElevator::Event_GotoFloor )
	EVENT( EV_Touch,				idElevator::Event_Touch )
#ifdef _D3XP
	EVENT( EV_SetGuiStates,			idElevator::Event_SetGuiStates )
#endif
END_CLASS

/*
================
idElevator::idElevator
================
*/
idElevator::idElevator( void ) {
	state = INIT;
	floorInfo.Clear();
	currentFloor = 0;
	pendingFloor = 0;
	lastFloor = 0;
	controlsDisabled = false;
	lastTouchTime = 0;
	returnFloor = 0;
	returnTime = 0;
}

/*
================
idElevator::Save
================
*/
void idElevator::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( state ); // elevatorState_t state
	savefile->WriteInt(floorInfo.Num()); // idList<floorInfo_s>		floorInfo
	for (int idx = 0; idx < floorInfo.Num(); idx++)
	{
		savefile->WriteVec3( floorInfo[idx].pos ); // idVec3 pos
		savefile->WriteString( floorInfo[idx].door ); // idString door
		savefile->WriteInt( floorInfo[idx].floor ); // int floor
	}
	savefile->WriteInt( currentFloor ); // int currentFloor
	savefile->WriteInt( pendingFloor ); // int pendingFloor
	savefile->WriteInt( lastFloor ); // int lastFloor
	savefile->WriteBool( controlsDisabled ); // bool controlsDisabled
	savefile->WriteFloat( returnTime ); // float returnTime
	savefile->WriteInt( returnFloor ); // int returnFloor
	savefile->WriteInt( lastTouchTime ); // int lastTouchTime
}

/*
================
idElevator::Restore
================
*/
void idElevator::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int&)state ); // elevatorState_t state
	int num;
	savefile->ReadInt(num); // idList<floorInfo_s> floorInfo
	for (int idx = 0; idx < num; idx++)
	{
		savefile->ReadVec3( floorInfo[idx].pos ); // idVec3 pos
		savefile->ReadString( floorInfo[idx].door ); // idString door
		savefile->ReadInt( floorInfo[idx].floor ); // int floor
	}
	savefile->ReadInt( currentFloor ); // int currentFloor
	savefile->ReadInt( pendingFloor ); // int pendingFloor
	savefile->ReadInt( lastFloor ); // int lastFloor
	savefile->ReadBool( controlsDisabled ); // bool controlsDisabled
	savefile->ReadFloat( returnTime ); // float returnTime
	savefile->ReadInt( returnFloor ); // int returnFloor
	savefile->ReadInt( lastTouchTime ); // int lastTouchTime
}

/*
================
idElevator::Spawn
================
*/
void idElevator::Spawn( void ) {
	idStr str;
	int len1;

	lastFloor = 0;
	currentFloor = 0;
	pendingFloor = spawnArgs.GetInt( "floor", "1" );
	SetGuiStates( ( pendingFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1]);

	returnTime = spawnArgs.GetFloat( "returnTime" );
	returnFloor = spawnArgs.GetInt( "returnFloor" );

	len1 = strlen( "floorPos_" );
	const idKeyValue *kv = spawnArgs.MatchPrefix( "floorPos_", NULL );
	while( kv ) {
		str = kv->GetKey().Right( kv->GetKey().Length() - len1 );
		floorInfo_s fi;
		fi.floor = atoi( str );
		fi.door = spawnArgs.GetString( va( "floorDoor_%i", fi.floor ) );
		fi.pos = spawnArgs.GetVector( kv->GetKey() );
		floorInfo.Append( fi );
		kv = spawnArgs.MatchPrefix( "floorPos_", kv );
	}
	lastTouchTime = 0;
	state = INIT;
	BecomeActive( TH_THINK | TH_PHYSICS );
	PostEventMS( &EV_Mover_InitGuiTargets, 0 );
	controlsDisabled = false;
}

/*
==============
idElevator::Event_Touch
===============
*/
void idElevator::Event_Touch( idEntity *other, trace_t *trace ) {

	if ( gameLocal.slow.time < lastTouchTime + 2000 ) {
		return;
	}

	if ( !other->IsType( idPlayer::Type ) ) {
		return;
	}

	lastTouchTime = gameLocal.slow.time;

	if ( thinkFlags & TH_PHYSICS ) {
		return;
	}

	int triggerFloor = spawnArgs.GetInt( "triggerFloor" );
	if ( spawnArgs.GetBool( "trigger" ) && triggerFloor != currentFloor ) {
		PostEventSec( &EV_GotoFloor, 0.25f, triggerFloor );
	}
}

/*
================
idElevator::Think
================
*/
void idElevator::Think( void ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;
	idDoor *doorent = GetDoor( spawnArgs.GetString( "innerdoor" ) );
	if ( state == INIT ) {
		state = IDLE;
		if ( doorent ) {
			doorent->BindTeam( this );
			doorent->spawnArgs.Set( "snd_open", "" );
			doorent->spawnArgs.Set( "snd_close", "" );
			doorent->spawnArgs.Set( "snd_opened", "" );
		}
		for ( int i = 0; i < floorInfo.Num(); i++ ) {
			idDoor *door = GetDoor( floorInfo[i].door );
			if ( door ) {
				door->SetCompanion( doorent );
			}
		}

		Event_GotoFloor( pendingFloor );
		DisableAllDoors();
		SetGuiStates( ( pendingFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1] );
	} else if ( state == WAITING_ON_DOORS ) {
		if ( doorent ) {
			state = doorent->IsOpen() ? WAITING_ON_DOORS : IDLE;
		} else {
			state = IDLE;
		}
		if ( state == IDLE ) {
			lastFloor = currentFloor;
			currentFloor = pendingFloor;
			floorInfo_s *fi = GetFloorInfo( currentFloor );
			if ( fi ) {
				MoveToPos( fi->pos );
			}
		}
	}
	RunPhysics();
	Present();
}

/*
================
idElevator::Event_Activate
================
*/
void idElevator::Event_Activate( idEntity *activator ) {
	int triggerFloor = spawnArgs.GetInt( "triggerFloor" );
	if ( spawnArgs.GetBool( "trigger" ) && triggerFloor != currentFloor ) {
		Event_GotoFloor( triggerFloor );
	}
}

/*
================
idElevator::Event_TeamBlocked
================
*/
void idElevator::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	if ( blockedEntity == this ) {
		Event_GotoFloor( lastFloor );
	} else if ( blockedEntity && blockedEntity->IsType( idDoor::Type ) ) {
		// open the inner doors if one is blocked
		idDoor *blocked = static_cast<idDoor *>( blockedEntity );
		idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
		if ( door && blocked->GetMoveMaster() == door->GetMoveMaster() ) {
			door->SetBlocked(true);
			OpenInnerDoor();
			OpenFloorDoor( currentFloor );
		}
	}
}

/*
===============
idElevator::HandleSingleGuiCommand
===============
*/
bool idElevator::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( controlsDisabled ) {
		return false;
	}

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "changefloor" ) == 0 ) {
		if ( src->ReadToken( &token ) ) {
			int newFloor = atoi( token );
			if ( newFloor == currentFloor ) {
				// open currentFloor and interior doors
				OpenInnerDoor();
				OpenFloorDoor( currentFloor );
			} else {
				idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
				if ( door && door->IsOpen() ) {
					PostEventSec( &EV_GotoFloor, 0.5f, newFloor );
				} else {
					ProcessEvent( &EV_GotoFloor, newFloor );
				}
			}
			return true;
		}
	}

	src->UnreadToken( &token );
	return false;
}

/*
================
idElevator::OpenFloorDoor
================
*/
void idElevator::OpenFloorDoor( int floor ) {
	floorInfo_s *fi = GetFloorInfo( floor );
	if ( fi ) {
		idDoor *door = GetDoor( fi->door );
		if ( door ) {
			door->Open();
		}
	}
}

/*
================
idElevator::OpenInnerDoor
================
*/
void idElevator::OpenInnerDoor( void ) {
	idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
	if ( door ) {
		door->Open();
	}
}

/*
================
idElevator::GetFloorInfo
================
*/
floorInfo_s *idElevator::GetFloorInfo( int floor ) {
	for ( int i = 0; i < floorInfo.Num(); i++ ) {
		if ( floorInfo[i].floor == floor ) {
			return &floorInfo[i];
		}
	}
	return NULL;
}

/*
================
idElevator::Event_GotoFloor
================
*/
void idElevator::Event_GotoFloor( int floor ) {
	floorInfo_s *fi = GetFloorInfo( floor );
	if ( fi ) {
		idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
		if ( door ) {
			if ( door->IsBlocked() || door->IsOpen() ) {
				PostEventSec( &EV_GotoFloor, 0.5f, floor );
				return;
			}
		}
		DisableAllDoors();
		CloseAllDoors();
		state = WAITING_ON_DOORS;
		pendingFloor = floor;
	}
}

/*
================
idElevator::BeginMove
================
*/
void idElevator::BeginMove( idThread *thread ) {
	controlsDisabled = true;
	CloseAllDoors();
	DisableAllDoors();
	const idKeyValue *kv = spawnArgs.MatchPrefix( "statusGui" );
	while( kv ) {
		idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent ) {
			for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ j ] ) {
					ent->GetRenderEntity()->gui[ j ]->SetStateString( "floor", "" );
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.slow.time, true );
				}
			}
			ent->UpdateVisuals();
		}
		kv = spawnArgs.MatchPrefix( "statusGui", kv );
	}
	SetGuiStates( ( pendingFloor == 1 ) ? guiBinaryMoverStates[3] : guiBinaryMoverStates[2] );
	idMover::BeginMove( thread );
}

/*
================
idElevator::GetDoor
================
*/
idDoor *idElevator::GetDoor( const char *name ) {
	idEntity	*ent;
	idEntity	*master;
	idDoor		*doorEnt;

	doorEnt = NULL;
	if ( name && *name ) {
		ent = gameLocal.FindEntity( name );
		if ( ent && ent->IsType( idDoor::Type ) ) {
			doorEnt = static_cast<idDoor*>( ent );
			master = doorEnt->GetMoveMaster();
			if ( master != doorEnt ) {
				if ( master->IsType( idDoor::Type ) ) {
					doorEnt = static_cast<idDoor*>( master );
				} else {
					doorEnt = NULL;
				}
			}
		}
	}

	return doorEnt;
}

/*
================
idElevator::Event_PostFloorArrival
================
*/
void idElevator::Event_PostFloorArrival() {
	OpenFloorDoor( currentFloor );
	OpenInnerDoor();
	SetGuiStates( ( currentFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1] );
	controlsDisabled = false;
	if ( returnTime > 0.0f && returnFloor != currentFloor ) {
		PostEventSec( &EV_GotoFloor, returnTime, returnFloor );
	}
}

#ifdef _D3XP
void idElevator::Event_SetGuiStates() {
	SetGuiStates( ( currentFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1] );
}
#endif

/*
================
idElevator::DoneMoving
================
*/
void idElevator::DoneMoving( void ) {
	idMover::DoneMoving();
	EnableProperDoors();
	const idKeyValue *kv = spawnArgs.MatchPrefix( "statusGui" );
	while( kv ) {
		idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent ) {
			for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ j ] ) {
					ent->GetRenderEntity()->gui[ j ]->SetStateString( "floor", va( "%i", currentFloor ) );
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.slow.time, true );
				}
			}
			ent->UpdateVisuals();
		}
		kv = spawnArgs.MatchPrefix( "statusGui", kv );
	}
	if ( spawnArgs.GetInt( "pauseOnFloor", "-1" ) == currentFloor ) {
		PostEventSec( &EV_PostArrival, spawnArgs.GetFloat( "pauseTime" ) );
	} else {
		Event_PostFloorArrival();
	}
}

/*
================
idElevator::CloseAllDoors
================
*/
void idElevator::CloseAllDoors( void ) {
	idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
	if ( door ) {
		door->Close();
	}
	for ( int i = 0; i < floorInfo.Num(); i++ ) {
		door = GetDoor( floorInfo[i].door );
		if ( door ) {
			door->Close();
		}
	}
}

/*
================
idElevator::DisableAllDoors
================
*/
void idElevator::DisableAllDoors( void ) {
	idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
	if ( door ) {
		door->Enable( false );
	}
	for ( int i = 0; i < floorInfo.Num(); i++ ) {
		door = GetDoor( floorInfo[i].door );
		if ( door ) {
			door->Enable( false );
		}
	}
}

/*
================
idElevator::EnableProperDoors
================
*/
void idElevator::EnableProperDoors( void ) {
	idDoor *door = GetDoor( spawnArgs.GetString( "innerdoor" ) );
	if ( door ) {
		door->Enable( true );
	}
	for ( int i = 0; i < floorInfo.Num(); i++ ) {
		if ( floorInfo[i].floor == currentFloor ) {
			door = GetDoor( floorInfo[i].door );
			if ( door ) {
				door->Enable( true );
				break;
			}
		}
	}
}


/*
===============================================================================

idMover_Binary

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"

===============================================================================
*/

const idEventDef EV_Mover_ReturnToPos1( "<returntopos1>", NULL );
const idEventDef EV_Mover_MatchTeam( "<matchteam>", "dd" );
const idEventDef EV_Mover_Enable( "enable", NULL );
const idEventDef EV_Mover_Disable( "disable", NULL );
const idEventDef EV_GravityCheck( "<gravitycheck>", NULL);

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
	EVENT( EV_GravityCheck,				idMover_Binary::DoDoorGravityCheck )
END_CLASS

/*
================
idMover_Binary::idMover_Binary()
================
*/
idMover_Binary::idMover_Binary() {
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
#ifdef _D3XP
	playerOnly = false;
#endif
	fl.networkSync = true;
}

/*
================
idMover_Binary::~idMover_Binary
================
*/
idMover_Binary::~idMover_Binary() {
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
	savefile->WriteFloat( wait ); // float wait

	savefile->WriteVec3( pos1 ); // idVec3 pos1
	savefile->WriteVec3( pos2 ); // idVec3 pos2

	savefile->WriteInt( (int)moverState ); // moveState_t moverState

	savefile->WriteObject( moveMaster ); // idMover_Binary * moveMaster
	savefile->WriteObject( activateChain ); // idMover_Binary * activateChain
	savefile->WriteInt( soundPos1 ); // int soundPos1
	savefile->WriteInt( sound1to2 ); // int sound1to2
	savefile->WriteInt( sound2to1 ); // int sound2to1
	savefile->WriteInt( soundPos2 ); // int soundPos2
	savefile->WriteInt( soundLoop ); // int soundLoop
	savefile->WriteFloat( damage ); // float damage
	savefile->WriteInt( duration ); // int duration
	savefile->WriteInt( accelTime ); // int accelTime
	savefile->WriteInt( decelTime ); // int decelTime
	savefile->WriteObject( activatedBy ); // idEntityPtr<idEntity> activatedBy
	savefile->WriteInt( stateStartTime ); // int stateStartTime
	savefile->WriteString( team ); // idString team
	savefile->WriteBool( enabled ); // bool enabled
	savefile->WriteInt( move_thread ); // int move_thread
	savefile->WriteInt( updateStatus ); // int updateStatus

	SaveFileWriteArray( buddies, buddies.Num(), WriteString ); // idStrList buddies

	savefile->WriteStaticObject( idMover_Binary::physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteInt( areaPortal ); // int areaPortal
	if ( areaPortal ) {
		savefile->WriteInt( gameRenderWorld->GetPortalState( areaPortal ) );
	}

	savefile->WriteBool( blocked ); // bool blocked
	savefile->WriteBool( playerOnly ); // bool playerOnly
	SaveFileWriteArray( guiTargets, guiTargets.Num(), WriteObject ); // idList< idEntityPtr<idEntity> > guiTargets

	// SM
	savefile->WriteBool( sparksEnabled ); // bool sparksEnabled
	savefile->WriteFloat( sparksZOffset ); // float sparksZOffset
	SaveFileWriteArray( FXSparks, FXSparks.Num(), WriteObject ); // idList<idEntityPtr> FXSparks
}

/*
================
idMover_Binary::Restore
================
*/
void idMover_Binary::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( wait ); // float wait

	savefile->ReadVec3( pos1 ); // idVec3 pos1
	savefile->ReadVec3( pos2 ); // idVec3 pos2

	savefile->ReadInt( (int&)moverState ); // moveState_t moverState

	savefile->ReadObject( CastClassPtrRef(moveMaster) ); // idMover_Binary * moveMaster
	savefile->ReadObject( CastClassPtrRef(activateChain) ); // idMover_Binary * activateChain
	savefile->ReadInt( soundPos1 ); // int soundPos1
	savefile->ReadInt( sound1to2 ); // int sound1to2
	savefile->ReadInt( sound2to1 ); // int sound2to1
	savefile->ReadInt( soundPos2 ); // int soundPos2
	savefile->ReadInt( soundLoop ); // int soundLoop
	savefile->ReadFloat( damage ); // float damage
	savefile->ReadInt( duration ); // int duration
	savefile->ReadInt( accelTime ); // int accelTime
	savefile->ReadInt( decelTime ); // int decelTime
	savefile->ReadObject( activatedBy ); // idEntityPtr<idEntity> activatedBy
	savefile->ReadInt( stateStartTime ); // int stateStartTime
	savefile->ReadString( team ); // idString team
	savefile->ReadBool( enabled ); // bool enabled
	savefile->ReadInt( move_thread ); // int move_thread
	savefile->ReadInt( updateStatus ); // int updateStatus

	SaveFileReadArray( buddies, ReadString ); // idStrList buddies

	savefile->ReadStaticObject( physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadInt( areaPortal ); // int areaPortal
	if ( areaPortal ) {
		int portalState;
		savefile->ReadInt( portalState );
		gameLocal.SetPortalState( areaPortal, portalState );
	}

	savefile->ReadBool( blocked ); // bool blocked
	savefile->ReadBool( playerOnly ); // bool playerOnly
	SaveFileReadArray( guiTargets, ReadObject ); // idList< idEntityPtr<idEntity> > guiTargets

	// SM
	savefile->ReadBool( sparksEnabled ); // bool sparksEnabled
	savefile->ReadFloat( sparksZOffset ); // float sparksZOffset
	SaveFileReadListCast( FXSparks, ReadObject, idEntityPtr<idEntity>& ); // idList<idEntityPtr> FXSparks
}

/*
================
idMover_Binary::Spawn

Base class for all movers.

"wait"		wait before returning (3 default, -1 = never return)
"speed"		movement speed
================
*/
void idMover_Binary::Spawn( void ) {
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
		JoinTeam( ent );
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

	// SM
	spawnArgs.GetFloat("sparksZOffset", "-1", sparksZOffset);
	sparksEnabled = spawnArgs.GetBool("sparksEnabled", "0");
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
				StartSound( "snd_open", SND_CHANNEL_BODY2, 0, false, NULL );
				StartSound("snd_move", SND_CHANNEL_BODY, 0, false, NULL); //BC for doors.
				break;
			case MOVER_2TO1:
				StartSound( "snd_close", SND_CHANNEL_BODY2, 0, false, NULL );
				StartSound("snd_move", SND_CHANNEL_BODY, 0, false, NULL); //BC for doors.
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
	idVec3	delta;

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
			DoParticleFX_Sparks();
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
			DoParticleFX_Sparks();
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

	if (newstate == MOVER_1TO2)
	{
		//BC a door has STARTED opening. Update gravity of objects.
		PostEventMS(&EV_GravityCheck, 100);
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

Sets the portal associtated with this mover to be open
================
*/
void idMover_Binary::Event_OpenPortal( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		if ( slave->areaPortal ) {
			slave->SetPortalState( true );
		}
#ifdef _D3XP
		if ( slave->playerOnly ) {
			gameLocal.SetAASAreaState( slave->GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, false );
		}
#endif
	}
}

/*
================
idMover_Binary::Event_ClosePortal

Sets the portal associtated with this mover to be closed
================
*/
void idMover_Binary::Event_ClosePortal( void ) {
	idMover_Binary *slave;

	for ( slave = moveMaster; slave != NULL; slave = slave->activateChain ) {
		if ( !slave->IsHidden() ) {
			if ( slave->areaPortal ) {
				slave->SetPortalState( false );
			}
#ifdef _D3XP
			if ( slave->playerOnly ) {
				gameLocal.SetAASAreaState( slave->GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, true );
			}
#endif
		}
	}
}

/*
================
idMover_Binary::Event_ReturnToPos1
================
*/
void idMover_Binary::Event_ReturnToPos1( void ) {
	MatchActivateTeam( MOVER_2TO1, gameLocal.slow.time );
}

/*
================
idMover_Binary::Event_Reached_BinaryMover
================
*/
void idMover_Binary::Event_Reached_BinaryMover( void ) {
	ClearParticleFX_Sparks(); // SM

	if ( moverState == MOVER_1TO2 ) {
		// reached pos2

		//bc DOOR HAS REACHED FULLY OPEN POSITION.

		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		if ( moveMaster == this )
		{
			StartSound( "snd_opened", SND_CHANNEL_BODY3, 0, false, NULL );
		}

		DoParticleFX_Open();

		PostEventMS(&EV_StopSound, 100, SND_CHANNEL_BODY, 0);//BC stop move sound. Add delay so it is more seamless

		SetMoverState( MOVER_POS2, gameLocal.slow.time );

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

		//bc DOOR HAS REACHED FULLY CLOSED POSITION.

		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		if (moveMaster == this)
		{
			StartSound("snd_closed", SND_CHANNEL_BODY3, 0, false, NULL); //BC sound to play when door fully closes.			
		}

		DoParticleFX_Close();

		PostEventMS(&EV_StopSound, 100, SND_CHANNEL_BODY, 0);//BC stop move sound. Add delay so it is more seamless

		SetMoverState( MOVER_POS1, gameLocal.slow.time );

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


		//BC door has fully closed. update gravity of objects.
		PostEventMS(&EV_GravityCheck, 100);

	} else {
		gameLocal.Error( "Event_Reached_BinaryMover: bad moverState" );
	}
}

//TODO: This doesnt work on all angles of doors... particles face wrong way sometimes
void idMover_Binary::DoParticleFX_Open()
{
	idBounds myBounds = GetPhysics()->GetBounds();
	
	// Calculate door direction in local space
	idVec3 doorDir = pos1 - pos2;
	doorDir.Normalize();
	doorDir *= GetPhysics()->GetAxis().InverseFast();

	idVec3 center = myBounds.GetCenter();
	center.z = 0.0f;
	bool isCenterOrigin = center.LengthSqr() <= 1.0f;
	float ySize = (myBounds.Max() - myBounds.Min()).y;
	float yPos = isCenterOrigin ? doorDir.y * ySize / 2.0f : 0.0f;

	// Bottom
	idEntityFx* fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMin()));
	fx->SetAngles(idAngles(0.0f, 0.0f, -90.0f * doorDir.y));

	// Top
	fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMax()));
	fx->SetAngles(idAngles(0.0f, 0.0f, -90.0f * doorDir.y));
}

void idMover_Binary::DoParticleFX_Close()
{
	idBounds myBounds = GetPhysics()->GetBounds();
	
	// Calculate door direction in local space
	idVec3 doorDir = pos1 - pos2;
	doorDir.Normalize();
	doorDir *= GetPhysics()->GetAxis().InverseFast();

	idVec3 center = myBounds.GetCenter();
	center.z = 0.0f;
	bool isCenterOrigin = center.LengthSqr() <= 1.0f;
	float ySize = (myBounds.Max() - myBounds.Min()).y;
	float yPos = isCenterOrigin ? doorDir.y * ySize / 2.0f : 0.0f;

	// Bottom
	idEntityFx* fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMin()));
	fx->SetAngles(idAngles(0.0f, 90.0f, -90.0f * doorDir.y));
	fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMin()));
	fx->SetAngles(idAngles(0.0f, -90.0f, -90.0f * doorDir.y));

	// Top
	fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMax()));
	fx->SetAngles(idAngles(0.0f, 90.0f, -90.0f * doorDir.y));
	fx = idEntityFx::StartFx("fx/doorclose", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMax()));
	fx->SetAngles(idAngles(0.0f, -90.0f, -90.0f * doorDir.y));
}

void idMover_Binary::DoParticleFX_Sparks()
{
	ClearParticleFX_Sparks();
	if (!sparksEnabled) {
		return;
	}

	// Calculate door direction in local space
	idVec3 doorDir = pos1 - pos2;
	doorDir.Normalize();
	doorDir *= GetPhysics()->GetAxis().InverseFast();

	idBounds myBounds = GetPhysics()->GetBounds();

	idVec3 center = myBounds.GetCenter();
	center.z = 0.0f;
	bool isCenterOrigin = center.LengthSqr() <= 1.0f;
	float ySize = (myBounds.Max() - myBounds.Min()).y;
	float yPos = isCenterOrigin ? doorDir.y * ySize / 2.0f : 0.0f;

	myBounds.ExpandSelf(sparksZOffset);
	
	// Bottom sparks
	idEntityFx* fx = idEntityFx::StartFx("fx/doorsparks", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMin()));
	fx->SetAngles(idAngles(0.0f, 0.0f, -90.0f * doorDir.y));
	FXSparks.Append(idEntityPtr<idEntity>());
	FXSparks[ FXSparks.Num() - 1 ] = fx;
	
	// Top sparks
	fx = idEntityFx::StartFx("fx/doorsparks", NULL, NULL, this, true);
	fx->SetOrigin(idVec3(0.0f, yPos, myBounds.zMax()));
	fx->SetAngles(idAngles(0.0f, 0.0f, -90.0f * doorDir.y));
	FXSparks.Append(idEntityPtr<idEntity>());
	FXSparks[ FXSparks.Num() - 1 ] = fx;
}

void idMover_Binary::ClearParticleFX_Sparks()
{
	for (int i = 0; i < FXSparks.Num(); ++i) {
		if (FXSparks[i].IsValid()) {
			FXSparks[i].GetEntity()->PostEventMS(&EV_Fx_KillFx, 0);
		}
	}
	FXSparks.Clear();
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
		
		//MatchActivateTeam(MOVER_2TO1, gameLocal.slow.time); //BC when in open position, allow activation to close door TODO: hook up to frobabble_when_open flag

		return;
	}



	if ( moverState == MOVER_POS1 ) {
		MatchActivateTeam( MOVER_1TO2, gameLocal.slow.time );

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

	//pos1 = closed
	//pos2 = open

	//if door is CLOSED.
	if ( moverState == MOVER_POS1 ) {
		// FIXME: start moving USERCMD_MSEC later, because if this was player
		// triggered, gameLocal.time hasn't been advanced yet
		MatchActivateTeam( MOVER_1TO2, gameLocal.slow.time + USERCMD_MSEC );

		SetGuiStates( guiBinaryMoverStates[MOVER_1TO2] );
		// open areaportal
		ProcessEvent( &EV_Mover_OpenPortal );

		//if I have this spawnarg, make door unfrobbable when it's opened.
		if (!spawnArgs.GetBool("frobabble_when_open", "1"))
		{
			this->isFrobbable = false;
		}

		SpawnDoormoveInterestpoint(activator);

		return;
	}

	// if door is completely UP/OPEN
	if ( moverState == MOVER_POS2 )
	{
		idMover_Binary *slave;
		
		if ( wait == -1 ) {
			return;
		}
		
		SetGuiStates( guiBinaryMoverStates[MOVER_2TO1] );
		
		for ( slave = this; slave != NULL; slave = slave->activateChain ) {
			slave->CancelEvents( &EV_Mover_ReturnToPos1 );
			slave->PostEventSec( &EV_Mover_ReturnToPos1, spawnArgs.GetBool( "toggle" )  ? 0 : wait );
		}

		//BC when button pressed, make the door immediately close.
		if (spawnArgs.GetBool("levertoggle") && activator->IsType(idLever::Type))
		{
			for (slave = this; slave != NULL; slave = slave->activateChain) {
				slave->CancelEvents(&EV_Mover_ReturnToPos1);
				slave->PostEventSec(&EV_Mover_ReturnToPos1, 0);
			}
		}

		return;
	}

	// only partway down before reversing
	if ( moverState == MOVER_2TO1 ) {
		GotoPosition2();

		SpawnDoormoveInterestpoint(activator);

		return;
	}

	// only partway up before reversing
	if ( moverState == MOVER_1TO2 ) {
		GotoPosition1();
		return;
	}
}

void idMover_Binary::SpawnDoormoveInterestpoint(idEntity *activator)
{
	if (activator == NULL)
		return;

	if (activator->team == TEAM_ENEMY)
		return;

	//Spawn an interestpoint when the door opens up by the PLAYER.
	if (spawnArgs.GetBool("movement_interestpoint", "1"))
	{
		idVec3 doorPosition = this->GetPhysics()->GetAbsBounds().GetCenter();
		gameLocal.SpawnInterestPoint(this, doorPosition, spawnArgs.GetString("interest_open", "interest_doormove"));
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
					ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.slow.time, true );
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
	duration = idPhysics::SnapTimeToPhysicsFrame( distance * 1000 / speed );
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




//bc we need a short delay before calling this, in order for give time for the door to open/close a little bit.
void idMover_Binary::DoDoorGravityCheck()
{
	gameLocal.DoGravityCheck();
}

void idMover_Binary::SetDuration(int newTime)
{
	duration = newTime;
}

void idMover_Binary::SetPostEvents(bool value)
{	
	//BC we call this in order to disable any pending postEventMS events that make the door auto-close.
	//Ideally we only want to disable the EV_Mover_ReturnToPos1 event, but CancelEvents doesn't seem to work in this context.

	if (value)
		Event_Enable();
	else
		Event_Disable();

	//CancelEvents(&EV_Mover_ReturnToPos1);	
}


/*
===============================================================================

idDoor

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/

const idEventDef EV_Door_StartOpen( "<startOpen>", NULL );
const idEventDef EV_Door_SpawnDoorTrigger( "<spawnDoorTrigger>", NULL );
const idEventDef EV_Door_SpawnSoundTrigger( "<spawnSoundTrigger>", NULL );
const idEventDef EV_Door_SpawnDisplaceTrigger("<spawnDisplaceTrigger>", NULL);
const idEventDef EV_Door_Open( "open", NULL );
const idEventDef EV_Door_Close( "close", NULL );
const idEventDef EV_Door_Lock( "lock", "d" );
const idEventDef EV_Door_IsOpen( "isOpen", NULL, 'f' );
const idEventDef EV_Door_IsLocked( "isLocked", NULL, 'f' );

//BC
const idEventDef EV_Door_SetBarricade("setBarricaded", "d");
const idEventDef EV_Door_SetCrusher("setCrusher", "d");
const idEventDef EV_Door_ReleaseBarricade("releaseBarricade");

CLASS_DECLARATION( idMover_Binary, idDoor )
	EVENT( EV_TeamBlocked,				idDoor::Event_TeamBlocked )
	EVENT( EV_PartBlocked,				idDoor::Event_PartBlocked )
	EVENT( EV_Touch,					idDoor::Event_Touch )
	EVENT( EV_Activate,					idDoor::Event_Activate )
	EVENT( EV_Door_StartOpen,			idDoor::Event_StartOpen )
	EVENT( EV_Door_SpawnDoorTrigger,	idDoor::Event_SpawnDoorTrigger )
	EVENT( EV_Door_SpawnSoundTrigger,	idDoor::Event_SpawnSoundTrigger )
	EVENT( EV_Door_SpawnDisplaceTrigger,idDoor::Event_SpawnDisplaceTrigger)
	EVENT( EV_Door_Open,				idDoor::Event_Open )
	EVENT( EV_Door_Close,				idDoor::Event_Close )
	EVENT( EV_Door_Lock,				idDoor::Event_Lock )
	EVENT( EV_Door_IsOpen,				idDoor::Event_IsOpen )
	EVENT( EV_Door_IsLocked,			idDoor::Event_Locked )
	EVENT( EV_ReachedPos,				idDoor::Event_Reached_BinaryMover )
	EVENT( EV_SpectatorTouch,			idDoor::Event_SpectatorTouch )
	EVENT( EV_Mover_OpenPortal,			idDoor::Event_OpenPortal )
	EVENT( EV_Mover_ClosePortal,		idDoor::Event_ClosePortal )
	EVENT( EV_Door_SetBarricade,		idDoor::Event_SetBarricade)
	EVENT(EV_Door_SetCrusher,			idDoor::SetCrusher)
	EVENT(EV_Door_ReleaseBarricade,		idDoor::Event_ReleaseBarricade)
END_CLASS

/*
================
idDoor::idDoor
================
*/
idDoor::idDoor( void ) {
	triggersize = 1.0f;
	crusher = false;
	noTouch = false;
	aas_area_closed = false;
	buddyStr.Clear();
	trigger = NULL;
	sndTrigger = NULL;
	displaceTrigger = NULL;
	nextSndTriggerTime = 0;
	localTriggerOrigin.Zero();
	localTriggerAxis.Identity();
	_requires.Clear();
	removeItem = 0;
	syncLock.Clear();
	companionDoor = NULL;
	normalAxisIndex = 0;
	monster_trigger = false;
	shoveDir = idVec3(0, 0, 0);

	//BC
	blocked_stayopen = false;
	triggerDelaytime = 0;
	playerBlock = false;
	doorLocked = false;

	peekEnt = nullptr;
}

/*
================
idDoor::~idDoor
================
*/
idDoor::~idDoor( void ) {
	if ( trigger ) {
		delete trigger;
	}
	if ( sndTrigger ) {
		delete sndTrigger;
	}
	if (displaceTrigger)
	{
		delete displaceTrigger;
	}
}

/*
================
idDoor::Save
================
*/
void idDoor::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( peekEnt ); // class idVentpeek * peekEnt

	savefile->WriteFloat( triggersize ); // float triggersize
	savefile->WriteBool( crusher ); // bool crusher
	savefile->WriteBool( noTouch ); // bool noTouch
	savefile->WriteBool( aas_area_closed ); // bool aas_area_closed
	savefile->WriteString( buddyStr ); // idString buddyStr
	savefile->WriteClipModel( trigger ); // idClipModel * trigger
	savefile->WriteClipModel( sndTrigger ); // idClipModel * sndTrigger
	savefile->WriteClipModel( displaceTrigger ); // idClipModel * displaceTrigger
	savefile->WriteInt( nextSndTriggerTime ); // int nextSndTriggerTime
	savefile->WriteVec3( localTriggerOrigin ); // idVec3 localTriggerOrigin
	savefile->WriteMat3( localTriggerAxis ); // idMat3 localTriggerAxis
	savefile->WriteString( _requires ); // idString _requires
	savefile->WriteInt( removeItem ); // int removeItem
	savefile->WriteString( syncLock ); // idString syncLock
	savefile->WriteInt( normalAxisIndex ); // int normalAxisIndex
	savefile->WriteObject( companionDoor ); // idDoor * companionDoor
	savefile->WriteVec3( shoveDir ); // idVec3 shoveDir

	savefile->WriteBool( monster_trigger ); // bool monster_trigger
	savefile->WriteBool( damage_only_objects ); // bool damage_only_objects
	savefile->WriteBool( blocked_stayopen ); // bool blocked_stayopen
	savefile->WriteInt( triggerDelaytime ); // int triggerDelaytime
	savefile->WriteBool( playerBlock ); // bool playerBlock
	savefile->WriteBool( blocked_pushaway ); // bool blocked_pushaway

	savefile->WriteBool( doorLocked ); // bool doorLocked
	savefile->WriteBool( unlockdelayActive ); // bool unlockdelayActive
	savefile->WriteInt( unlockdelayTime ); // int unlockdelayTime
	savefile->WriteBool( vacuumlock ); // bool vacuumlock

	savefile->WriteInt( airlessCheckTimer ); // int airlessCheckTimer
	savefile->WriteVec3( originalPosition ); // idVec3 originalPosition
	savefile->WriteVec3( originalCenterMass ); // idVec3 originalCenterMass

	SaveFileWriteArray( gateProps, 2, WriteObject ); // idAnimatedEntity* gateProps[2]

	barricade.Save( savefile ); // idEntityPtr<class idDoorBarricade> barricade
}

/*
================
idDoor::Restore
================
*/
void idDoor::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( CastClassPtrRef(peekEnt) ); // class idVentpeek * peekEnt

	savefile->ReadFloat( triggersize ); // float triggersize
	savefile->ReadBool( crusher ); // bool crusher
	savefile->ReadBool( noTouch ); // bool noTouch
	savefile->ReadBool( aas_area_closed ); // bool aas_area_closed
	savefile->ReadString( buddyStr ); // idString buddyStr
	savefile->ReadClipModel( trigger ); // idClipModel * trigger
	savefile->ReadClipModel( sndTrigger ); // idClipModel * sndTrigger
	savefile->ReadClipModel( displaceTrigger ); // idClipModel * displaceTrigger
	savefile->ReadInt( nextSndTriggerTime ); // int nextSndTriggerTime
	savefile->ReadVec3( localTriggerOrigin ); // idVec3 localTriggerOrigin
	savefile->ReadMat3( localTriggerAxis ); // idMat3 localTriggerAxis
	savefile->ReadString( _requires ); // idString _requires
	savefile->ReadInt( removeItem ); // int removeItem
	savefile->ReadString( syncLock ); // idString syncLock
	savefile->ReadInt( normalAxisIndex ); // int normalAxisIndex
	savefile->ReadObject( CastClassPtrRef(companionDoor) ); // idDoor * companionDoor
	savefile->ReadVec3( shoveDir ); // idVec3 shoveDir

	savefile->ReadBool( monster_trigger ); // bool monster_trigger
	savefile->ReadBool( damage_only_objects ); // bool damage_only_objects
	savefile->ReadBool( blocked_stayopen ); // bool blocked_stayopen
	savefile->ReadInt( triggerDelaytime ); // int triggerDelaytime
	savefile->ReadBool( playerBlock ); // bool playerBlock
	savefile->ReadBool( blocked_pushaway ); // bool blocked_pushaway

	savefile->ReadBool( doorLocked ); // bool doorLocked
	savefile->ReadBool( unlockdelayActive ); // bool unlockdelayActive
	savefile->ReadInt( unlockdelayTime ); // int unlockdelayTime
	savefile->ReadBool( vacuumlock ); // bool vacuumlock

	savefile->ReadInt( airlessCheckTimer ); // int airlessCheckTimer
	savefile->ReadVec3( originalPosition ); // idVec3 originalPosition
	savefile->ReadVec3( originalCenterMass ); // idVec3 originalCenterMass

	SaveFileReadArrayCast( gateProps, ReadObject, idClass*& ); // idAnimatedEntity* gateProps[2]

	barricade.Restore( savefile ); // idEntityPtr<class idDoorBarricade> barricade
}

/*
================
idDoor::Spawn
================
*/
void idDoor::Spawn( void ) {
	idVec3		abs_movedir;
	float		distance;
	idVec3		size;
	idVec3		movedir;
	float		dir;
	float		lip;
	bool		start_open;
	float		time;
	float		speed;

	// get the direction to move
	if (!spawnArgs.GetBool("autodir", "0"))
	{
		if (!spawnArgs.GetFloat("movedir", "0", dir))
		{
			// no movedir, so angle defines movement direction and not orientation,
			// a la oldschool Quake
			spawnArgs.GetFloat("angle", "0", dir);
		}
	}
	else
	{
		//BC auto select direction based on model orientation.
		idVec3 right;
		const char * modelname = spawnArgs.GetString("model");
		int textIdx = idStr::FindText( modelname, "right.ase", false);
		
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &right, NULL);
		
		dir = right.ToYaw();

		if (textIdx >= 0)
			dir += 180;
	}

	GetMovedir( dir, movedir );

	// default speed of 400
	spawnArgs.GetFloat( "speed", "400", speed );

	// default wait of 2 seconds
	spawnArgs.GetFloat( "wait", "3", wait );

	// default lip of 8 units
	spawnArgs.GetFloat( "lip", "8", lip );

	// by default no damage
	spawnArgs.GetFloat( "damage", "0", damage );

	// trigger size
	spawnArgs.GetFloat( "triggersize", "120", triggersize );

	spawnArgs.GetBool( "crusher", "0", crusher );
	spawnArgs.GetBool( "start_open", "0", start_open );
	spawnArgs.GetBool( "no_touch", "0", noTouch );
#ifdef _D3XP
	spawnArgs.GetBool( "player_only", "0", playerOnly );
#endif

	// expects syncLock to be a door that must be closed before this door will open
	spawnArgs.GetString( "syncLock", "", syncLock );

	spawnArgs.GetString( "buddy", "", buddyStr );

	spawnArgs.GetString( "requires", "", _requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );

	//BC
	spawnArgs.GetBool("monster_trigger", "1", monster_trigger);
	spawnArgs.GetBool("damage_only_objects", "1", damage_only_objects);
	spawnArgs.GetBool("blocked_stayopen", "1", blocked_stayopen);
	spawnArgs.GetBool("player_block", "0", playerBlock);
	spawnArgs.GetBool("blocked_pushaway", "1", blocked_pushaway);

	// ever separate piece of a door is considered solid when other team mates push entities
	fl.solidForTeam = true;
	 
	// SW: Gets the direction (relative to the DOOR MODEL, WITHOUT ROTATIONS) that potential blockages should be shoved.
	// You shouldn't need to change this on an individual entity unless you're doing something very wacky.
	spawnArgs.GetVector("shoveDir", "0 0 1", shoveDir);

	// first position at start
	pos1 = GetPhysics()->GetOrigin();

	// calculate second position
	abs_movedir[0] = idMath::Fabs( movedir[ 0 ] );
	abs_movedir[1] = idMath::Fabs( movedir[ 1 ] );
	abs_movedir[2] = idMath::Fabs( movedir[ 2 ] );
	size = GetPhysics()->GetAbsBounds()[1] - GetPhysics()->GetAbsBounds()[0];
	distance = ( abs_movedir * size ) - lip;
	pos2 = pos1 + distance * movedir;

	// if "start_open", reverse position 1 and 2
	if ( start_open ) {
		// post it after EV_SpawnBind
		PostEventMS( &EV_Door_StartOpen, 1 );
	}

	if ( spawnArgs.GetFloat( "time", "1", time ) ) {
		InitTime( pos1, pos2, time, 0, 0 );
	} else {
		InitSpeed( pos1, pos2, speed, 0, 0 );
	}


	if ( moveMaster == this ) {
		if ( health ) {
			fl.takedamage = true;
		}
		if ( (noTouch || health) && !spawnArgs.GetBool("forceDoorTrigger", "0")) {
			// non touch/shoot doors
			PostEventMS( &EV_Mover_MatchTeam, 0, moverState, gameLocal.slow.time );

			const char *sndtemp = spawnArgs.GetString( "snd_locked" );
			if ( spawnArgs.GetInt( "locked" ) && sndtemp && *sndtemp ) {
				PostEventMS( &EV_Door_SpawnSoundTrigger, 0 );
			}
		} else {
			// spawn triggers
			PostEventMS( &EV_Door_SpawnDoorTrigger, 0 );
		}
	}

	// Create displace trigger regardless of whether the interaction trigger is created
	// (automatic doors should still push things aside!)
	// SW 26th Feb 2025: adding spawnarg control for certain tricky scenarios where this trigger doesn't really work
	if (spawnArgs.GetBool("displaceTrigger", "1"))
		PostEventMS(&EV_Door_SpawnDisplaceTrigger, 0);

	// see if we are on an areaportal
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );
	if ( !start_open ) {
		// start closed
		ProcessEvent( &EV_Mover_ClosePortal );

#ifdef _D3XP
		if ( playerOnly ) {
			gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, true );
		}
#endif
	}

	int locked = spawnArgs.GetInt( "locked" );
	if ( locked ) {
		// make sure all members of the team get locked
		PostEventMS( &EV_Door_Lock, 0, locked );
	}

	if ( spawnArgs.GetBool( "continuous" ) ) {
		PostEventSec( &EV_Activate, spawnArgs.GetFloat( "delay" ), this );
	}

	// sounds have a habit of stuttering when portals close, so make them unoccluded
	refSound.parms.soundShaderFlags |= SSF_NO_OCCLUSION;

	companionDoor = NULL;

	enabled = true;
	blocked = false;

	//BC
	doorLocked = false;
	unlockdelayActive = false;
	unlockdelayTime = 0;
	airlessCheckTimer = 0;

	spawnArgs.GetBool( "vacuumlock", "0", vacuumlock );

	SetColor(idVec4(0, 1, 0, 1));
	UpdateVisuals();

	originalPosition = GetPhysics()->GetOrigin();
	originalCenterMass = GetPhysics()->GetAbsBounds().GetCenter();


	health = spawnArgs.GetInt("health");
	if (health) {
		fl.takedamage = true;
	}
	else
	{
		fl.takedamage = false;
	}



	if (spawnArgs.GetBool("auto_button", "0"))
	{
		//Spawn buttons on the doorframe.
		idVec3 forward, up, right;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

		for (int i = 0; i < 2; i++)
		{
			idVec3 spawnPos;
			int yaw;

			if (i == 0)
			{
				spawnPos = GetPhysics()->GetOrigin() + (forward * AUTOBUTTON_FORWARD) + (right * -AUTOBUTTON_RIGHT) + (up * AUTOBUTTON_UP);
				yaw = GetPhysics()->GetAxis().ToAngles().yaw;				
			}
			else
			{
				spawnPos = GetPhysics()->GetOrigin() + (forward * -AUTOBUTTON_FORWARD) + (right * AUTOBUTTON_RIGHT) + (up * AUTOBUTTON_UP);
				yaw = GetPhysics()->GetAxis().ToAngles().yaw + 180;
			}

			idEntity *buttonEnt;
			idDict args;			
			args.Set("classname", "env_lever_c");
			args.SetVector("origin", spawnPos);
			//args.Set("target", GetName());
			args.Set("frobtarget", GetName());
			args.SetInt("angle", yaw);
			gameLocal.SpawnEntityDef(args, &buttonEnt);


		}
	}

    //if door does vacuum lock, then spawn lockdown gate models. The logic for this lives in idDoor::Lock()
    if (spawnArgs.GetBool("vacuumlock"))
    {
        #define DOORRADIUS 8
        #define GATEOFFSET .2f

        for (int i = 0; i < 2; i++)
        {
            idAngles gateAngle = GetPhysics()->GetAxis().ToAngles();
            if (i <= 0)
                gateAngle.yaw += 180;

            idVec3 forward;
            GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL);
            idVec3 gatePos = GetPhysics()->GetOrigin();
            if (i <= 0)
            {
                gatePos += forward * (-DOORRADIUS - GATEOFFSET);
            }
            else
            {
                gatePos += forward * (DOORRADIUS + GATEOFFSET);
            }

            idDict args;
            args.Clear();
            args.SetVector("origin", gatePos);
            args.SetFloat("angle", gateAngle.yaw);
            args.SetBool("hide", true);
            args.Set("model", this->spawnArgs.GetString("model_lockdowngate"));


			//inspection point for lockdown gate.
			args.SetBool("zoominspect", true);
			args.Set("displayName", "#str_def_gameplay_100049"); // "Lockdown Gate"
			args.SetInt("dyna_inspectable", 1); // Allows inspection point to talk back to dynatips
			args.Set("name", va("%s_gate%d", this->GetName(), i + 1)); // Gives the lockdown gate a predictable name that we can refer to in scripts
			args.SetVector("zoominspect_angle", idVec3(0, 179, 0));
			args.SetVector("zoominspect_campos", idVec3(20, 0, 75));
			args.SetVector("zoominspect_selpos", idVec3( GATEOFFSET, 0, 75));
			args.Set("loc_inspectiontext", "#str_label_doorgate");

            gateProps[i] = (idAnimatedEntity *)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
            gateProps[i]->Event_PlayAnim("opened", 0);


			
        }
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            gateProps[i] = NULL;
        }
    }

	// SM: Add vent option
	if ( spawnArgs.GetBool( "has_ventpeek" ) )
	{
		idBounds doorBounds = this->GetPhysics()->GetBounds();
		idVec3 pos = GetPhysics()->GetOrigin();
		idVec3 forwardDir, upDir;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors( &forwardDir, NULL, &upDir );
		pos += upDir * idMath::Abs( doorBounds[1].z - doorBounds[0].z ) / 2.0f;
		SpawnVentPeek( "env_ventpeek_door_single", pos, 180.0f );
	}

	if ( spawnArgs.GetBool( "barricaded" ) )
	{
		AddBarricade();
	}


	BecomeActive( TH_THINK );
}

bool idDoor::DoFrob(int index, idEntity * frobber)
{
	if (doorLocked)
	{
		//if (this->refSound.referenceSound != NULL)
		//{
		//	if (!this->refSound.referenceSound->CurrentlyPlaying())
		//		StartSound("snd_locked", SND_CHANNEL_ANY, 0, false, NULL);
		//}

		if (gameLocal.slow.time > nextSndTriggerTime)
		{
			StartSound("snd_locked", SND_CHANNEL_VOICE, 0, false, NULL);
			nextSndTriggerTime = gameLocal.slow.time + 500;
		}

		return false;
	}
	else if ( barricade.IsValid() )
	{
		//check if player has key.
		bool playCancelNoise = true;

		if (frobber == gameLocal.GetLocalPlayer())
		{
			//If door is barricaded, frob the barricade entity. This routes all of the key/lock/barricade logic all into the barricade.
			if (barricade.GetEntity()->DoFrob(0, gameLocal.GetLocalPlayer())) //see if player has key.
			{
				playCancelNoise = false;
			}
		}
		else if (barricade.GetEntity()->DoFrob(0, frobber)) //check if correct key is being thrown against door.
		{			
			playCancelNoise = false;
		}		

		if ( gameLocal.slow.time > nextSndTriggerTime && playCancelNoise)
		{
			StartSound("snd_cancel", SND_CHANNEL_VOICE, 0, false, NULL);
			nextSndTriggerTime = gameLocal.slow.time + 500;
		}

		return false;
	}

	if (IsOpen())
	{
		bool doDoorClose = true;

		//if a monster frobs a door, we only want them to OPEN it, never close it.
		if (frobber != NULL)
		{
			if (frobber->IsType(idActor::Type) && frobber != gameLocal.GetLocalPlayer())
			{
				doDoorClose = false;
			}
		}

		if (doDoorClose)
		{
			Close();
		}
	}
	else
	{
		Open();

		if (spawnArgs.GetBool("frob_colorchange"))
		{
			SetColor(idVec4(1, 0, 0, 1));
		}
	}

	return true;
}


bool idDoor::DoFrobHold( int index /*= 0*/, idEntity * frobber /*= NULL */ )
{
	bool result = false;

	if ( peekEnt )
	{
		result = peekEnt->DoFrob( index, frobber );
	}

	return result;
}


bool idDoor::IsFrobHoldable() const
{
	return peekEnt != NULL;
}

// SW: Unlocks the door's barricade, if applicable.
// This is distinct from the 'setBarricaded' event, which just instantly removes the barricade outside of the normal game flow
void idDoor::Event_ReleaseBarricade(void)
{
	if (barricade.IsValid())
	{
		barricade.GetEntity()->DoFrob(0, this);
	}
}

/*
================
idDoor::Think
================
*/
void idDoor::Think( void ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	idMover_Binary::Think();

	if ( thinkFlags & TH_PHYSICS ) {
		// update trigger position
		if ( GetMasterPosition( masterOrigin, masterAxis ) ) {
			if ( trigger ) {
				trigger->Link( gameLocal.clip, this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
			}
			if ( sndTrigger ) {
				sndTrigger->Link( gameLocal.clip, this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
			}
		}
	}

	if (unlockdelayActive)
	{
		if (gameLocal.time >= unlockdelayTime)
		{
			unlockdelayActive = false;
			//BecomeInactive(TH_THINK);

			if (IsLocked())
			{
				Lock(0);
			}
		}
	}

	if (vacuumlock && gameLocal.time > airlessCheckTimer)
	{
		//BC Important: when the door is LOCKED, we're currently assuming it is locked because of a vacuum breach.
		//If we ever lock a door NOT because of a vacuum breach, then we'll need to change this code.

		airlessCheckTimer = gameLocal.time + VACUUMDOOR_THINKINTERVAL; //how often to check airless status.

		idVec3 forward, up, checkPos1, checkPos2;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		checkPos1 = originalPosition + (forward * 12) + (up * 1);
		checkPos2 = originalPosition + (forward * -12) + (up * 1);

		if (!doorLocked && (gameLocal.GetAirlessAtPoint(checkPos1) || gameLocal.GetAirlessAtPoint(checkPos2)))
		{
			//Lock
			Lock(1);
		}
		else if (doorLocked && !gameLocal.GetAirlessAtPoint(checkPos1) && !gameLocal.GetAirlessAtPoint(checkPos2))
		{
			//Unlock.
			Lock(0);
			//BecomeInactive(TH_THINK);
		}
	}

	
	if (moverState == MOVER_2TO1 && displaceTrigger != NULL)
	{
		displaceTrigger->Link(gameLocal.clip, this, 253, GetPhysics()->GetOrigin(), mat3_identity);
	}
}
/*
================
idDoor::DisplacePotentialBlockage

	SW: We want doors to close as seamlessly and effectively as possible, preferably without destroying whatever gets in their way.
	
	While it's possible to detect blockages with Event_PartBlocked, I believe this is already a worst-case scenario.
	Ideally, we want to detect and displace blockages before they happen, while the door is still closing.
	If the door is closing and an object enters the displace trigger, this method will try to shove it aside.
================
*/
void idDoor::DisplacePotentialBlockage(idPhysics* blockage)
{
	idVec3 shoveDir = this->shoveDir * this->GetPhysics()->GetAxis();
	idVec3 directionToBlockage = (blockage->GetOrigin() - this->GetPhysics()->GetOrigin());
	directionToBlockage.NormalizeFast();

	if (DotProduct(shoveDir, directionToBlockage) < 0)
	{
		// We're shoving in the wrong direction!
		// (directionToBlockage tells us which side of the door it's closest to,
		// and the dot product should be > 0 if there's an acute angle between these vectors)
		shoveDir *= -1;
	}

	// Special case: if we're shoving something straight up into the air, 
	// add a random offset so we don't accidentally 'juggle' it in place
	if (shoveDir.z == 1 && shoveDir.x == 0 && shoveDir.y == 0)
	{
		shoveDir.x = (float)rand() / (float)RAND_MAX;
		shoveDir.y = (float)rand() / (float)RAND_MAX;

	}

	// Ragdolls need all their components shoved at once, 
	// or the components that aren't moving will eat up the momentum of the root joint
	if (blockage->IsType(idPhysics_AF::Type))
	{
		idPhysics_AF* ragdoll = static_cast<idPhysics_AF*>(blockage);
		for (int i = 0; i < ragdoll->GetNumBodies(); i++)
		{
			ragdoll->SetLinearVelocity(shoveDir * PUSHAWAY_FORCE, i);
		}

		// Ragdolls will also add the door to their 'squeeze grace' list, preventing collision for a short time
		ragdoll->AddSqueezeGraceEnt(this, SQUEEZE_GRACE_MS);
	}
	else
	{
		shoveDir.z += .2f; //BC add a little upward push to help it move better.
		blockage->SetLinearVelocity(shoveDir * PUSHAWAY_FORCE);
	}
}

/*
================
idDoor::PreBind
================
*/
void idDoor::PreBind( void ) {
	idMover_Binary::PreBind();
}

/*
================
idDoor::PostBind
================
*/
void idDoor::PostBind( void ) {
	idMover_Binary::PostBind();
	GetLocalTriggerPosition( trigger ? trigger : sndTrigger );
}

/*
================
idDoor::SetAASAreaState
================
*/
void idDoor::SetAASAreaState( bool closed ) {
	aas_area_closed = closed;
	idBounds bounds = physicsObj.GetAbsBounds();
	bounds.ExpandSelf( 8.0f );
	gameLocal.SetAASAreaState( bounds, AREACONTENTS_CLUSTERPORTAL|AREACONTENTS_OBSTACLE, closed );
}

/*
================
idDoor::Hide
================
*/
void idDoor::Hide( void ) {
	idMover_Binary *slave;
	idMover_Binary *master;
	idDoor *slaveDoor;
	idDoor *companion;

	master = GetMoveMaster();
	if ( this != master ) {
		master->Hide();
	} else {
		for ( slave = this; slave != NULL; slave = slave->GetActivateChain() ) {
			if ( slave->IsType( idDoor::Type ) ) {
				slaveDoor = static_cast<idDoor *>( slave );
				companion = slaveDoor->companionDoor;
				if ( companion && ( companion != master ) && ( companion->GetMoveMaster() != master ) ) {
					companion->Hide();
				}
				if ( slaveDoor->trigger ) {
					slaveDoor->trigger->Disable();
				}
				if ( slaveDoor->sndTrigger ) {
					slaveDoor->sndTrigger->Disable();
				}
				if ( slaveDoor->areaPortal ) {
					slaveDoor->SetPortalState( true );
				}
				slaveDoor->SetAASAreaState( false );
			}
			slave->GetPhysics()->GetClipModel()->Disable();
			slave->idMover_Binary::Hide();
		}
	}
}

/*
================
idDoor::Show
================
*/
void idDoor::Show( void ) {
	idMover_Binary *slave;
	idMover_Binary *master;
	idDoor *slaveDoor;
	idDoor *companion;

	master = GetMoveMaster();
	if ( this != master ) {
		master->Show();
	} else {
		for ( slave = this; slave != NULL; slave = slave->GetActivateChain() ) {
			if ( slave->IsType( idDoor::Type ) ) {
				slaveDoor = static_cast<idDoor *>( slave );
				companion = slaveDoor->companionDoor;
				if ( companion && ( companion != master ) && ( companion->GetMoveMaster() != master ) ) {
					companion->Show();
				}
				if ( slaveDoor->trigger ) {
					slaveDoor->trigger->Enable();
				}
				if ( slaveDoor->sndTrigger ) {
					slaveDoor->sndTrigger->Enable();
				}
				if ( slaveDoor->areaPortal && ( slaveDoor->moverState == MOVER_POS1 ) ) {
					slaveDoor->SetPortalState( false );
				}
				slaveDoor->SetAASAreaState( IsLocked() || IsNoTouch() );
			}
			slave->GetPhysics()->GetClipModel()->Enable();
			slave->idMover_Binary::Show();
		}
	}
}

/*
================
idDoor::GetLocalTriggerPosition
================
*/
void idDoor::GetLocalTriggerPosition( const idClipModel *trigger ) {
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
================
idDoor::Use
================
*/
void idDoor::Use( idEntity *other, idEntity *activator ) {
	if ( barricade.IsValid() )
	{
		return;
	}
	if ( gameLocal.RequirementMet( activator, _requires, removeItem ) ) {
		if ( syncLock.Length() ) {
			idEntity *sync = gameLocal.FindEntity( syncLock );
			if ( sync && sync->IsType( idDoor::Type ) ) {
				if ( static_cast<idDoor *>( sync )->IsOpen() ) {
					return;
				}
			}
		}
		ActivateTargets( activator );
		Use_BinaryMover( activator );
		if ( peekEnt != NULL )
		{
			peekEnt->Hide();
		}

		
	}
}

/*
================
idDoor::Open
================
*/
void idDoor::Open( void ) {

	GotoPosition2();
	SpawnDoormoveInterestpoint(gameLocal.GetLocalPlayer());

	if (!spawnArgs.GetBool("frobabble_when_open", "1"))
	{
		this->isFrobbable = false;
	}

	if ( peekEnt != NULL )
	{
		peekEnt->Hide();
	}
}

/*
================
idDoor::Close
================
*/
void idDoor::Close( void ) {
	GotoPosition1();

	if (!spawnArgs.GetBool("frobabble_when_open", "1") && spawnArgs.GetBool("frobbable", "1"))
	{
		this->isFrobbable = true;
	}
}

/*
================
idDoor::Lock
================
*/
void idDoor::Lock( int lockActive ) {
	idMover_Binary *other;

	// lock all the doors on the team
	for( other = moveMaster; other != NULL; other = other->GetActivateChain() ) {
		if ( other->IsType( idDoor::Type ) )
		{
			idDoor *door = static_cast<idDoor *>( other );
			if ( other == moveMaster ) {
				if ( door->sndTrigger == NULL ) {
					// in this case the sound trigger never got spawned
					const char *sndtemp = door->spawnArgs.GetString( "snd_locked" );
					if ( sndtemp && *sndtemp ) {
						door->PostEventMS( &EV_Door_SpawnSoundTrigger, 0 );
					}
				}
				if ( !lockActive && ( door->spawnArgs.GetInt( "locked" ) != 0 ) )
				{
					door->StartSound( "snd_unlocked", SND_CHANNEL_ANY, 0, false, NULL );
				}
			}
			door->spawnArgs.SetInt( "locked", lockActive);
			if ( (lockActive == 0 ) || ( !IsHidden() && ( door->moverState == MOVER_POS1 ) ) ) {
				door->SetAASAreaState(lockActive != 0 || barricade.IsValid());
			}
		}
	}

	if (lockActive)
	{
        //Activate the lockdown gate models.
        if (!doorLocked)
        {
            for (int i = 0; i < 2; i++)
            {
                gateProps[i]->Show();
                gateProps[i]->Event_PlayAnim("close", 0);


            }
        }

		//Close the door, lock it.
		doorLocked = true;
		SetColor(idVec4(1, 0, 0, 1)); //set button color.
		renderEntity.shaderParms[7] = 1; //make the vacuum breach icon appear.
		UpdateVisuals();

		PostEventMS(&EV_Door_Close, 100); //We add a delay because we need the volumetriggers to recognize the doorLocked variable, THEN we close the door.

		//BecomeActive(TH_THINK);
	}
	else
	{
        if (doorLocked)
        {
            for (int i = 0; i < 2; i++)
            {
                gateProps[i]->Event_PlayAnim("open", 0);
            }
        }

		//Unlock the door.
		doorLocked = false;
		SetColor(idVec4(.2f, 1, .2f, 1)); //set button color.
		renderEntity.shaderParms[7] = 0; //make the vacuum breach icon hide.
		UpdateVisuals();
	}
}

/*
================
idDoor::IsLocked
================
*/
int idDoor::IsLocked( void ) {
	return spawnArgs.GetInt( "locked" ) || doorLocked;
}

/*
================
idDoor::IsOpen
================
*/
bool idDoor::IsOpen( void ) {
	return ( moverState != MOVER_POS1 );
}

/*
================
idDoor::IsNoTouch
================
*/
bool idDoor::IsNoTouch( void ) {
	return noTouch;
}

#ifdef _D3XP
/*
================
idDoor::AllowPlayerOnly
================
*/
bool idDoor::AllowPlayerOnly( idEntity *ent ) {
	if ( playerOnly && !ent->IsType(idPlayer::Type) ) {
		return false;
	}

	return true;
}
#endif

/*
======================
idDoor::CalcTriggerBounds

Calcs bounds for a trigger.
======================
*/
void idDoor::CalcTriggerBounds( float size, idBounds &bounds ) {
	idMover_Binary	*other;
	int				i;
	int				best;

	// find the bounds of everything on the team
	bounds = GetPhysics()->GetAbsBounds();

	if (spawnArgs.GetInt("health") > 0)
		fl.takedamage = true;

	for( other = activateChain; other != NULL; other = other->GetActivateChain() ) {
		if ( other->IsType( idDoor::Type ) ) {
			// find the bounds of everything on the team
			bounds.AddBounds( other->GetPhysics()->GetAbsBounds() );

			// set all of the slaves as shootable
			if (other->spawnArgs.GetInt("health") > 0)
				other->fl.takedamage = true;
		}
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( bounds[1][ i ] - bounds[0][ i ] < bounds[1][ best ] - bounds[0][ best ] ) {
			best = i;
		}
	}
	normalAxisIndex = best;
	bounds[0][ best ] -= size;
	bounds[1][ best ] += size;
	bounds[0] -= GetPhysics()->GetOrigin();
	bounds[1] -= GetPhysics()->GetOrigin();
}

/*
======================
idDoor::Event_StartOpen

if "start_open", reverse position 1 and 2
======================
*/
void idDoor::Event_StartOpen( void ) {
	float time;
	float speed;

	// if "start_open", reverse position 1 and 2
	pos1 = pos2;
	pos2 = GetPhysics()->GetOrigin();

	spawnArgs.GetFloat( "speed", "400", speed );

	if ( spawnArgs.GetFloat( "time", "1", time ) ) {
		InitTime( pos1, pos2, time, 0, 0 );
	} else {
		InitSpeed( pos1, pos2, speed, 0, 0 );
	}
}

/*
======================
idDoor::Event_SpawnDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them.
======================
*/
void idDoor::Event_SpawnDoorTrigger( void ) {
	idBounds		bounds;
	idMover_Binary	*other;
	bool			toggle;
	idVec3			relativeoffset;
	idVec3			finaloffset = vec3_zero;

	if ( trigger ) {
		// already have a trigger, so don't spawn a new one.
		return;
	}

	// check if any of the doors are marked as toggled
	toggle = false;
	for( other = moveMaster; other != NULL; other = other->GetActivateChain() ) {
		if ( other->IsType( idDoor::Type ) && other->spawnArgs.GetBool( "toggle" ) ) {
			toggle = true;
			break;
		}
	}

	if ( toggle ) {
		// mark them all as toggled
		for( other = moveMaster; other != NULL; other = other->GetActivateChain() ) {
			if ( other->IsType( idDoor::Type ) ) {
				other->spawnArgs.Set( "toggle", "1" );
			}
		}
		// don't spawn trigger
		return;
	}

	const char *sndtemp = spawnArgs.GetString( "snd_locked" );
	if ( spawnArgs.GetInt( "locked" ) && sndtemp && *sndtemp ) {
		PostEventMS( &EV_Door_SpawnSoundTrigger, 0 );
	}

	CalcTriggerBounds( triggersize, bounds );

	relativeoffset = spawnArgs.GetVector("trigger_relativeoffset", "0 0 0");

	// create a trigger clip model
	trigger = new idClipModel( idTraceModel( bounds ) );

	if (relativeoffset != vec3_zero)
	{
		//BC for things like ventdoors, give offset to trigger so that door stays open when player is trying to exit.
		idVec3 upDir, forwardDir, rightDir;
		idVec3 offset;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);
		finaloffset += (forwardDir * relativeoffset.x) + (rightDir * relativeoffset.y) + (upDir * relativeoffset.z);		
	}

	trigger->Link(gameLocal.clip, this, 255, GetPhysics()->GetOrigin() + finaloffset, mat3_identity);
	trigger->SetContents( CONTENTS_TRIGGER );

	//gameRenderWorld->DebugBounds(colorGreen, trigger->GetAbsBounds(), vec3_zero, 60000);

	GetLocalTriggerPosition( trigger );

	MatchActivateTeam( moverState, gameLocal.slow.time );
}

/*
======================
idDoor::Event_SpawnSoundTrigger

Spawn a sound trigger to activate locked sound if it exists.
======================
*/
void idDoor::Event_SpawnSoundTrigger( void ) {
	idBounds bounds;

	if ( sndTrigger ) {
		return;
	}

	CalcTriggerBounds( triggersize * 0.5f, bounds );

	// create a trigger clip model
	sndTrigger = new idClipModel( idTraceModel( bounds ) );
	sndTrigger->Link( gameLocal.clip, this, 254, GetPhysics()->GetOrigin(), mat3_identity );
	sndTrigger->SetContents( CONTENTS_TRIGGER );

	GetLocalTriggerPosition( sndTrigger );
}

/*
======================
idDoor::Event_SpawnDisplaceTrigger

	SW: Spawns a trigger that sits slightly in front of the door's pinch-point. 
	Objects that enter this trigger are shoved aside as the door closes.
======================
*/
void idDoor::Event_SpawnDisplaceTrigger(void) {
	if (!displaceTrigger)
	{
		// Move our trigger forward into the pinch point a little,
		// and shrink it a tiny bit so it's not coplanar with the door sides
		idBounds bounds = this->GetPhysics()->GetBounds()
			.Rotate(this->GetPhysics()->GetAxis())
			.Translate((pos1 - pos2) * 0.25)
			.Expand(-1.0f);

		displaceTrigger = new idClipModel(idTraceModel(bounds));
		displaceTrigger->Link(gameLocal.clip, this, 253, GetPhysics()->GetOrigin(), mat3_identity);
		displaceTrigger->SetContents(CONTENTS_TRIGGER);
		//gameRenderWorld->DebugBounds(idVec4(1, 0, 0, 1), displaceTrigger->GetBounds(), displaceTrigger->GetOrigin(), 10000);
	}
	else
	{
		gameLocal.Warning("idDoor::Event_SpawnDisplaceTrigger: displace trigger already exists!");
	}
}


void idDoor::SpawnVentPeek( const char* name, const idVec3& position, float extraYaw )
{
	//spawn a ventpeek.
	const idDeclEntityDef *peekDef;
	peekDef = gameLocal.FindEntityDef( name, false );
	if ( !peekDef )
	{
		gameLocal.Error( "idVentdoor %s failed to spawn peek ent.\n", GetName() );
	}

	idEntity *tempEnt;
	gameLocal.SpawnEntityDef( peekDef->dict, &tempEnt, false );
	peekEnt = static_cast< idVentpeek* >( tempEnt );
	if ( !peekEnt )
	{
		gameLocal.Error( "ventdoor '%s' failed to spawn ventpeek.\n", GetName() );
	}

	peekEnt->GetPhysics()->SetOrigin( position );

	idAngles finalAng = this->GetPhysics()->GetAxis().ToAngles();
	finalAng.yaw += extraYaw;
	peekEnt->GetPhysics()->SetAxis( finalAng.ToMat3() );
	peekEnt->UpdateVisuals(); //gotta do this! or else the position doesn't update correctly.
	peekEnt->ownerEnt = this;

	// Setup the frob for it
	peekEnt->isFrobbable = false;
	peekEnt->forVentDoor = true;
}

void idDoor::AddBarricade()
{
	SetAASAreaState( true );

	idBounds doorBounds = this->GetPhysics()->GetBounds();
	idVec3 pos = GetPhysics()->GetOrigin();
	idVec3 forwardDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors( &forwardDir, NULL, &upDir );
	//pos += upDir * idMath::Abs( doorBounds[1].z - doorBounds[0].z ) / 2.0f;
	barricade = SpawnBarricade( spawnArgs.GetString("def_barricade", "func_door_barricade"), pos, 0.0f );
}

void idDoor::RemoveBarricade()
{
	if ( barricade.IsValid() )
	{
		barricade.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

	SetAASAreaState( false || doorLocked );	
	
	//Update the infostation map. Make the appropriate lock icons disappear.	
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->InfostationsLockdisplayUpdate();

	if (barricade.IsValid())
	{
		idStr barricadeAnimatedModelname = barricade.GetEntity()->spawnArgs.GetString("model_animated");
		if (barricadeAnimatedModelname.Length() > 0)
		{
			idAngles animatedAngles = barricade.GetEntity()->GetPhysics()->GetAxis().ToAngles();
			animatedAngles.yaw += 180;

			idDict args;
			args.Clear();
			args.SetVector("origin", barricade.GetEntity()->GetPhysics()->GetOrigin());
			args.SetMatrix("rotation", animatedAngles.ToMat3());
			args.Set("model", barricadeAnimatedModelname.c_str());
			args.SetBool("hide", false);
			args.Set("snd_open", "shutter_2sec");
			idAnimatedEntity *animatedEnt = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
			if (animatedEnt)
			{
				animatedEnt->Event_PlayAnim("open", 0);
				animatedEnt->PostEventMS(&EV_Remove, 2000);
			}

			animatedEnt->StartSound("snd_open", SND_CHANNEL_ANY);
		}
	}
}

idDoorBarricade* idDoor::SpawnBarricade( const char* name, const idVec3& position, float extraYaw /*= 0.0f */ )
{
	const idDeclEntityDef *barricadeDef;
	barricadeDef = gameLocal.FindEntityDef( name, false );
	if ( !barricadeDef )
	{
		gameLocal.Error( "Door '%s' has invalid def_barricade '%s'\n", GetName(), name );
	}

	idEntity *tempEnt;
	gameLocal.SpawnEntityDef( barricadeDef->dict, &tempEnt, false );
	idDoorBarricade* barEnt = static_cast< idDoorBarricade* >( tempEnt );
	if ( !barEnt )
	{
		gameLocal.Error( "door '%s' failed to spawn barricade.\n", GetName() );
	}

	//set position with offset defined by 'barricadeoffset'
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idVec3 barricadeOffset = barEnt->spawnArgs.GetVector("barricadeoffset");
	idVec3 barricadePosition = position + (forward * barricadeOffset[0]) + (right * barricadeOffset[1]) + (up * barricadeOffset[2]);
	barEnt->GetPhysics()->SetOrigin(barricadePosition);

	idAngles finalAng = this->GetPhysics()->GetAxis().ToAngles();
	finalAng.yaw += extraYaw;
	barEnt->GetPhysics()->SetAxis( finalAng.ToMat3() );
	barEnt->UpdateVisuals();

	barEnt->owningDoor = this;

	return barEnt;
}

bool idDoor::IsBarricaded()
{
	if (!barricade.IsValid())
		return false;

	//TODO: this needs to be more robust later when the barricade doesn't just magically disappear.
	return true;
}

idVec3 idDoor::GetBarricadeColor()
{
	if (!barricade.IsValid())
		return vec3_zero;

	idVec3 barricadeColor;
	barricade.GetEntity()->GetColor(barricadeColor);

	return barricadeColor;
}

void idDoor::Event_SetBarricade(int value)
{
	//This is the script call to toggle barricade.

	if (!value)
	{
		//want to remove barricade.
		if (IsBarricaded())
		{
			RemoveBarricade();
			return;
		}
	}
	else
	{
		//want to add barricade.
		if (!IsBarricaded())
		{
			barricade = SpawnBarricade(spawnArgs.GetString("def_barricade", "func_door_barricade"), GetPhysics()->GetOrigin(), 0.0f);
			return;
		}
	}
}

/*
================
idDoor::Event_Reached_BinaryMover
================
*/
void idDoor::Event_Reached_BinaryMover( void ) {
	if ( moverState == MOVER_2TO1 ) {

		//BC called when door is fully closed.

		SetBlocked( false );
		const idKeyValue *kv = spawnArgs.MatchPrefix( "triggerClosed" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 0, moveMaster->GetActivator() );
			}
			kv = spawnArgs.MatchPrefix( "triggerClosed", kv );
		}


		//BC when door closes, make the door frobbable again.
		if (!spawnArgs.GetBool("frobabble_when_open", "1") && spawnArgs.GetBool("frobbable", "1"))
		{
			this->isFrobbable = true;
		}

		if ( moverState == MOVER_2TO1 && peekEnt != NULL )
		{
			peekEnt->Show();
			peekEnt->GetPhysics()->SetContents( CONTENTS_RENDERMODEL ); //We do this because the clipmodel frob box will only reappear if this line is included.
		}

	} else if ( moverState == MOVER_1TO2 ) {

		//BC called when door is fully open.

		const idKeyValue *kv = spawnArgs.MatchPrefix( "triggerOpened" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 0, moveMaster->GetActivator() );
			}
			kv = spawnArgs.MatchPrefix( "triggerOpened", kv );
		}
	}
	idMover_Binary::Event_Reached_BinaryMover();
}

/*
================
idDoor::Blocked_Door
================
*/
void idDoor::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity )
{
	//BC this is called when door is blocked.

	SetBlocked( true );

	if ( crusher )
	{
		return;		// crushers don't reverse
	}

	if (blockingEntity && moverState == MOVER_1TO2)
	{
		if (!blockingEntity->IsType(idAI::Type))
		{
			//door is OPENING and movement is blocked.

			//When door is opening, we want to INFLICT MASSIVE DAMAGE on any object that's blocking the door's
			//movement. This is primarily for situations where an object gets stuck INSIDE the doorframe
			//and cannot be pushed out/away.

			blockingEntity->Damage(this, this, vec3_origin, "damage_gib", 1.0f, INVALID_JOINT);
			return;
		}
	}


	// reverse direction
	Use_BinaryMover( moveMaster->GetActivator() );

	if ( companionDoor )
	{
		companionDoor->ProcessEvent( &EV_TeamBlocked, blockedEntity, blockingEntity );
	}

	//BC
	if (blockingEntity)
	{		
		idVec3 upDir;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
		blockingEntity->GetPhysics()->SetLinearVelocity(upDir * 128);		
	}
}

/*
===============
idDoor::SetCompanion
===============
*/
void idDoor::SetCompanion( idDoor *door ) {
	companionDoor = door;
}

/*
===============
idDoor::Event_PartBlocked
===============
*/
void idDoor::Event_PartBlocked( idEntity *blockingEntity )
{
	//BC This gets called when door movement is blocked by an object.

	if ( damage > 0.0f )
	{
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 1.0f, INVALID_JOINT );
	}
	else if (damage_only_objects)
	{
		if ((blockingEntity->IsType(idPlayer::Type) || (blockingEntity->IsType(idAI::Type) && blockingEntity->health > 0))) //stay open for actors IF they're alive.
			return;

		blockingEntity->Damage(this, this, vec3_origin, "damage_moverCrush", 1.0f, INVALID_JOINT);
	}

	if (blocked_pushaway)
	{
		DisplacePotentialBlockage(blockingEntity->GetPhysics());
	}
}

bool idDoor::AllowMonsterOnly(idEntity *ent)
{
	if (monster_trigger && ent->IsType(idAI::Type) && ent->health > 0)
	{
		//if I'm a monster-only trigger and the thing hitting me is a monster, then activate.
		return true;
	}

	//if (!monster_trigger && (ent->IsType(idAI::Type) || ent->IsType(idPlayer::Type)))
	//{
	//	//If I am NOT a monster trigger and an actor is hitting me, then ignore.
	//	return false;
	//}
	//
	//return true;

	return false;
}

/*
================
idDoor::Event_Touch
================
*/
void idDoor::Event_Touch( idEntity *other, trace_t *trace ) {
	idVec3		contact, translate;
	idVec3		planeaxis1, planeaxis2, normal;
	idBounds	bounds;

	if ( !enabled  || triggerDelaytime > gameLocal.time ) //triggerDelaytime = when player enters vent, we need to force the door to ignore the triggervolume for a short time, so that the ventdoor can completely close shut.
	{
		return;
	}


	//BC this is logic for the TRIGGERVOLUME associated with this door.
	if ( trigger && trace->c.id == trigger->GetId() && !doorLocked && !crusher)
	{
		if ( !IsNoTouch() && !IsLocked() && GetMoverState() != MOVER_1TO2 )
		{
			if (AllowMonsterOnly(other))
			{
				Use(this, other);
			}

			if (blocked_stayopen && moverState == MOVER_POS2)
			{
				Use(this, other);
			}

			if (AllowPlayerBlock(other) && moverState == MOVER_POS2)
			{
				Use(this, other);
			}

			/*
			if ( AllowPlayerOnly( other ) )
			{
				Use( this, other );
			}*/
		}		
	}
	else if ( sndTrigger && trace->c.id == sndTrigger->GetId() ) {

		//BC this makes the lock-sound play when the player touches a locked door. I don't think we want this to happen.......

		//if ( other && other->IsType( idPlayer::Type ) && IsLocked() && gameLocal.slow.time > nextSndTriggerTime )
		//{
		//	StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
		//	nextSndTriggerTime = gameLocal.slow.time + 10000;
		//}
	}
	else if (displaceTrigger && trace->c.id == displaceTrigger->GetId()) {
		// SW: If the door is closing, try to shove objects with a physics component out of the way
		if ((moverState == MOVER_2TO1 || moverState == MOVER_POS1) && other && other->GetPhysics())
		{
			DisplacePotentialBlockage(other->GetPhysics());
		}
	}
}

/*
================
idDoor::Event_SpectatorTouch
================
*/
void idDoor::Event_SpectatorTouch( idEntity *other, trace_t *trace ) {
	idVec3		contact, translate, normal;
	idBounds	bounds;
	idPlayer	*p;

	assert( other && other->IsType( idPlayer::Type ) && static_cast< idPlayer * >( other )->spectating );

	p = static_cast< idPlayer * >( other );
	// avoid flicker when stopping right at clip box boundaries
	if ( p->lastSpectateTeleport > gameLocal.hudTime - 500 ) { //BC was 1000
		return;
	}

	//For spectator players: teleport to other side of door if you press against door.

	//if ( trigger && !IsOpen() )
	//{
	//	// teleport to the other side, center to the middle of the trigger brush
	//	bounds = trigger->GetAbsBounds();
	//	contact = trace->endpos - bounds.GetCenter();
	//	translate = bounds.GetCenter();
	//	normal.Zero();
	//	normal[ normalAxisIndex ] = 1.0f;
	//	if ( normal * contact > 0 ) {
	//		translate[ normalAxisIndex ] += ( bounds[ 0 ][ normalAxisIndex ] - translate[ normalAxisIndex ] ) * 0.5f;
	//	} else {
	//		translate[ normalAxisIndex ] += ( bounds[ 1 ][ normalAxisIndex ] - translate[ normalAxisIndex ] ) * 0.5f;
	//	}
	//
	//
	//	//Ok, do an extra check here to make sure the resulting destination is in a valid spot. Ventdoors have a trigger only on ONE side of the door,
	//	//so we need to make sure we catch that situation and handle it accordingly.
	//	//TODO: do the check here.
	//
	//
	//	
	//
	//	p->SetOrigin( translate );
	//	p->lastSpectateTeleport = gameLocal.hudTime;
	//
	//	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
	//}
	//else if (!IsOpen())
	if (!IsOpen())
	{
		SpectatorDoorNoTriggerTouch(p);
	}
}

void idDoor::SpectatorDoorNoTriggerTouch(idPlayer *p)
{
	//BC Ok, we do NOT have a trigger. This is for things like airlock doors.
	//Figure out where to teleport player to.

	//TODO: figure out if this handles ventdoors & ventdoor non-ortho orientations

	idVec3 forward, candidatePos1, candidatePos2;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

	//get positions on both sides of the door.
	candidatePos1 = this->GetPhysics()->GetOrigin() + (forward * 32);
	candidatePos2 = this->GetPhysics()->GetOrigin() + (forward * -32);

	idAngles modelAngle = GetPhysics()->GetAxis().ToAngles();
	if (modelAngle.pitch == 0)
	{
		//Vent is vertical, attached to wall. Lock destination to current player altitude.
		candidatePos1.z = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z;
		candidatePos2.z = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z;
	}

	//figure out which side is farther from player. Teleport player there.
	idVec3 finalDestination = vec3_zero;
	idVec3 playerPos = gameLocal.GetLocalPlayer()->GetEyePosition();

	if ((playerPos - candidatePos1).LengthFast() > (playerPos - candidatePos2).LengthFast())
	{
		finalDestination = candidatePos1;
	}
	else
	{
		finalDestination = candidatePos2;
	}
	

	p->SetOrigin(finalDestination);
	p->lastSpectateTeleport = gameLocal.hudTime;
	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
}





/*
================
idDoor::Event_Activate
================
*/
void idDoor::Event_Activate( idEntity *activator ) {
	//int old_lock;
	//
	////If door is locked.......
	//if ( spawnArgs.GetInt( "locked" ) ) {
	//	if ( !trigger ) {
	//		PostEventMS( &EV_Door_SpawnDoorTrigger, 0 );
	//	}
	//	if ( buddyStr.Length() ) {
	//		idEntity *buddy = gameLocal.FindEntity( buddyStr );
	//		if ( buddy ) {
	//			buddy->SetShaderParm( SHADERPARM_MODE, 1 );
	//			buddy->UpdateVisuals();
	//		}
	//	}
	//
	//	old_lock = spawnArgs.GetInt( "locked" );
	//	Lock( 0 );
	//	if ( old_lock == 2 ) {
	//		return;
	//	}
	//}

	//BC this function used to be called when the door's doorframe buttonw was pressed. However, that
	//is now routed through the lever's frobTarget, and now calls the door's dofrob() function.
	//To the best of my knowledge door::Event_Activate is no longer used, but I may be wrong!!!!!

	if (doorLocked)
	{
		if (gameLocal.slow.time > nextSndTriggerTime)
		{
			StartSound("snd_locked", SND_CHANNEL_VOICE, 0, false, NULL);
			nextSndTriggerTime = gameLocal.slow.time + 500;
		}
		return;
	}
	else if (barricade.IsValid())
	{
		if (gameLocal.slow.time > nextSndTriggerTime)
		{
			StartSound("snd_cancel", SND_CHANNEL_VOICE, 0, false, NULL);
			nextSndTriggerTime = gameLocal.slow.time + 500;
		}

		return;
	}



	if ( syncLock.Length() ) {
		idEntity *sync = gameLocal.FindEntity( syncLock );
		if ( sync && sync->IsType( idDoor::Type ) ) {
			if ( static_cast<idDoor *>( sync )->IsOpen() ) {
				return;
			}
		}
	}

	ActivateTargets( activator );

	//renderEntity.shaderParms[ SHADERPARM_MODE ] = 1; //BC what is this for????????? I am going to comment this out......... it is messin with the vacuumbreach icon logic
	UpdateVisuals();

	Use_BinaryMover( activator );

	if ( peekEnt != NULL )
	{
		peekEnt->Hide();
	}
}

/*
================
idDoor::Event_Open
================
*/
void idDoor::Event_Open( void ) {
	Open();
}

/*
================
idDoor::Event_Close
================
*/
void idDoor::Event_Close( void ) {
	Close();
}

/*
================
idDoor::Event_Lock
================
*/
void idDoor::Event_Lock( int f ) {
	Lock( f );
}

/*
================
idDoor::Event_IsOpen
================
*/
void idDoor::Event_IsOpen( void ) {
	bool state;

	state = IsOpen();
	idThread::ReturnFloat( state );
}

/*
================
idDoor::Event_Locked
================
*/
void idDoor::Event_Locked( void ) {
	idThread::ReturnFloat( spawnArgs.GetInt("locked") );
}

/*
================
idDoor::Event_OpenPortal

Sets the portal associtated with this door to be open
================
*/
void idDoor::Event_OpenPortal( void ) {
	idMover_Binary *slave;
	idDoor *slaveDoor;

	for ( slave = this; slave != NULL; slave = slave->GetActivateChain() ) {
		if ( slave->IsType( idDoor::Type ) ) {
			slaveDoor = static_cast<idDoor *>( slave );
			if ( slaveDoor->areaPortal ) {
				slaveDoor->SetPortalState( true );
			}
			slaveDoor->SetAASAreaState( false );
		}
	}
}

/*
================
idDoor::Event_ClosePortal

Sets the portal associtated with this door to be closed
================
*/
void idDoor::Event_ClosePortal( void ) {
	idMover_Binary *slave;
	idDoor *slaveDoor;

	for ( slave = this; slave != NULL; slave = slave->GetActivateChain() ) {
		if ( !slave->IsHidden() ) {
			if ( slave->IsType( idDoor::Type ) ) {
				slaveDoor = static_cast<idDoor *>( slave );
				if ( slaveDoor->areaPortal ) {
					slaveDoor->SetPortalState( false );
				}
				slaveDoor->SetAASAreaState( IsLocked() || IsNoTouch() );
			}
		}
	}
}


//Force the door's triggervolume to disable for xx millisec.
void idDoor::ForceTriggerDelay(int millisec)
{
	triggerDelaytime = gameLocal.time + millisec;
}

bool idDoor::AllowPlayerBlock(idEntity *ent)
{
	if (playerBlock && ent->IsType(idPlayer::Type))
	{
		return true;
	}

	return false;
}

//This was used when windowseal repairs were on a fixed timer.
void idDoor::SetUnlockDelay(int millisec)
{
	//Make door unlock itself after XX millisec.
	unlockdelayActive = true;
	unlockdelayTime = gameLocal.time + millisec;
	//BecomeActive(TH_THINK);
}

void idDoor::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	//Door was killed.
	if (areaPortal)
	{
		//Open the portal.
		gameLocal.SetPortalState(areaPortal, PS_BLOCK_NONE);
	}

	//BC 5-8-2025: destruction fx when a door is blown up
	idStr destructionFx = spawnArgs.GetString("fx_destruction", "");
	if (destructionFx.Length() > 0)
	{
		idEntityFx::StartFx(destructionFx.c_str(), GetPhysics()->GetAbsBounds().GetCenter(), mat3_identity);
	}

	gameLocal.SetDebrisBurst("moveable_metalgib1", originalCenterMass, 24, 48, 256, dir);

	PostEventMS(&EV_Remove, 0);
}

void idDoor::SetCrusher(bool value)
{
	crusher = value;
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
	savefile->WriteClipModel( trigger ); // idClipModel * trigger
	savefile->WriteVec3( localTriggerOrigin ); // idVec3 localTriggerOrigin
	savefile->WriteMat3( localTriggerAxis ); // idMat3 localTriggerAxis
}

/*
===============
idPlat::Restore
===============
*/
void idPlat::Restore( idRestoreGame *savefile ) {
	savefile->ReadClipModel( trigger ); // idClipModel * trigger
	savefile->ReadVec3( localTriggerOrigin ); // idVec3 localTriggerOrigin
	savefile->ReadMat3( localTriggerAxis ); // idMat3 localTriggerAxis
}

/*
===============
idPlat::Spawn
===============
*/
void idPlat::Spawn( void ) {
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

	SetMoverState( MOVER_POS1, gameLocal.slow.time );
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
	savefile->WriteStaticObject( idMover_Periodic::physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhysics = &physicsObj == GetPhysics();
	savefile->WriteBool( restorePhysics );

	savefile->WriteFloat( damage ); // float damage
}

/*
===============
idMover_Periodic::Restore
===============
*/
void idMover_Periodic::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj ); // idPhysics_Parametric physicsObj
	bool restorePhys;
	savefile->ReadBool( restorePhys );
	if (restorePhys)
	{
		RestorePhysics( &physicsObj );
	}

	savefile->ReadFloat( damage );// float damage
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
idRotater::idRotater( void ) {
	activatedBy = this;
}

/*
===============
idRotater::Spawn
===============
*/
void idRotater::Spawn( void ) {
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	if ( !spawnArgs.GetBool( "nopush" ) ) {
		physicsObj.SetPusher( 0 );
	}
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, gameLocal.slow.time, 0, GetPhysics()->GetOrigin(), vec3_origin, vec3_origin );
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.slow.time, 0, GetPhysics()->GetAxis().ToAngles(), ang_zero, ang_zero );
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
void idRotater::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( activatedBy ); // idEntityPtr<idEntity> activatedBy
}

/*
===============
idRotater::Restore
===============
*/
void idRotater::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( activatedBy ); // idEntityPtr<idEntity> activatedBy
}

/*
===============
idRotater::Event_Activate
===============
*/
void idRotater::Event_Activate( idEntity *activator ) {
	float		speed;
	bool		x_axis;
	bool		y_axis;
	idAngles	delta;

	activatedBy = activator;

	delta.Zero();

	if ( !spawnArgs.GetBool( "rotate" ) ) {
		spawnArgs.Set( "rotate", "1" );
		spawnArgs.GetFloat( "speed", "100", speed );
		spawnArgs.GetBool( "x_axis", "0", x_axis );
		spawnArgs.GetBool( "y_axis", "0", y_axis );

		// set the axis of rotation
		if ( x_axis ) {
			delta[2] = speed;
		} else if ( y_axis ) {
			delta[0] = speed;
		} else {
			delta[1] = speed;
		}
	} else {
		spawnArgs.Set( "rotate", "0" );
	}

	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.slow.time, 0, physicsObj.GetAxis().ToAngles(), delta, ang_zero );
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
void idBobber::Spawn( void ) {
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
	physicsObj.SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), phase * 1000, speed * 500, GetPhysics()->GetOrigin(), delta * 2.0f, vec3_origin );
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
void idPendulum::Spawn( void ) {
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
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), phase * 1000, 500/freq, GetPhysics()->GetAxis().ToAngles(), idAngles( 0, 0, speed * 2.0f ), ang_zero );
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
void idRiser::Spawn( void ) {
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

		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.slow.time, time * 1000, physicsObj.GetOrigin(), delta, vec3_origin );
	}
}
