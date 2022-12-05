
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "ai/AI_Manager.h"

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
// RAVEN BEGIN
const idEventDef EV_PostRestoreExt( "<postrestore>", "ddddd" );
// RAVEN END
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
// RAVEN BEGIN
// cnicholson: added stop sound support
const idEventDef EV_StoppedSound( "stoppedSound", "s" );
// RAVEN END
const idEventDef EV_MoveSound( "moveSound", "s" );
const idEventDef EV_Mover_InitGuiTargets( "<initguitargets>", NULL );
const idEventDef EV_EnableSplineAngles( "enableSplineAngles", NULL );
const idEventDef EV_DisableSplineAngles( "disableSplineAngles", NULL );
const idEventDef EV_RemoveInitialSplineAngles( "removeInitialSplineAngles", NULL );
const idEventDef EV_StartSpline( "startSpline", "e" );
const idEventDef EV_StopSpline( "stopSpline", NULL );
const idEventDef EV_IsMoving( "isMoving", NULL, 'd' );
const idEventDef EV_IsRotating( "isRotating", NULL, 'd' );
// RAVEN BEGIN
// abahr:
const idEventDef EV_GetSplineEntity( "getSplineEntity", NULL, 'E' );
const idEventDef EV_MoveAlongVector( "moveAlongVector", "v" );
// RAVEN END

CLASS_DECLARATION( idEntity, idMover )
	EVENT( EV_FindGuiTargets,		idMover::Event_FindGuiTargets )
	EVENT( EV_Thread_SetCallback,	idMover::Event_SetCallback )
	EVENT( EV_TeamBlocked,			idMover::Event_TeamBlocked )
	EVENT( EV_PartBlocked,			idMover::Event_PartBlocked )
	EVENT( EV_ReachedPos,			idMover::Event_UpdateMove )
	EVENT( EV_ReachedAng,			idMover::Event_UpdateRotation )
// RAVEN BEGIN
	EVENT( EV_PostRestoreExt,		idMover::Event_PostRestoreExt )
// RAVEN END
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
// RAVEN BEGIN
// cnicholson: added stop sound support
	EVENT( EV_StoppedSound,			idMover::Event_SetStoppedSound )
// RAVEN END
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
// RAVEN BEGIN
// abahr:
	EVENT( EV_GetSplineEntity,		idMover::Event_GetSplineEntity )
	EVENT( EV_MoveAlongVector,		idMover::Event_MoveAlongVector )
// RAVEN END
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
	attenuate = false;
	useIdleSound = false;
	lastCommand = MOVER_NONE;
	damage = 0.0f;
	areaPortal = 0;
	maxAttenuation = 0.0f;
	attenuationScale = 0.0f;
	lastOrigin.Zero( );
	lastTime = 0;
	splineStartTime = 0;

	fl.networkSync = true;
}


idMover::~idMover( void ) {
	SetPhysics( NULL );
}



/*
================
idMover::Save
================
*/
void idMover::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteStaticObject( physicsObj );

	savefile->Write( &move, sizeof( move ) );
	savefile->Write( &rot, sizeof( rot ) );

	savefile->WriteInt( move_thread );
	savefile->WriteInt( rotate_thread );

	savefile->WriteAngles( dest_angles );
	savefile->WriteAngles( angle_delta );
	savefile->WriteVec3( dest_position );
	savefile->WriteVec3( move_delta );

	savefile->WriteFloat( move_speed );
	savefile->WriteInt( move_time );
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

	savefile->WriteInt( guiTargets.Num() );
	for( i = 0; i < guiTargets.Num(); i++ ) {
		guiTargets[ i ].Save( savefile );
	}

	if ( splineEnt.GetEntity() ) {
		idCurve_Spline<idVec3> *spline = physicsObj.GetSpline();
		if (spline) {

		savefile->WriteBool( true );
		splineEnt.Save( savefile );
		savefile->WriteInt( spline->GetTime( 0 ) );
		savefile->WriteInt( spline->GetTime( spline->GetNumValues() - 1 ) - spline->GetTime( 0 ) );
		savefile->WriteInt( physicsObj.GetSplineAcceleration() );
		savefile->WriteInt( physicsObj.GetSplineDeceleration() );
		savefile->WriteInt( (int)physicsObj.UsingSplineAngles() );
		} else {
			savefile->WriteBool( false );
		}

	} else {
		savefile->WriteBool( false );
	}

// RAVEN BEGIN
// mekberg: added for attenuation and idle sound
	savefile->WriteBool( attenuate );
	savefile->WriteFloat( maxAttenuation );
	savefile->WriteFloat( attenuationScale );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteInt( lastTime );
	savefile->WriteBool( useIdleSound );
	splineStateThread.Save( savefile );
	savefile->WriteInt( splineStartTime );
// RAVEN END
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

	savefile->Read( &move, sizeof( move ) );
	savefile->Read( &rot, sizeof( rot ) );

	savefile->ReadInt( move_thread );
	savefile->ReadInt( rotate_thread );

	savefile->ReadAngles( dest_angles );
	savefile->ReadAngles( angle_delta );
	savefile->ReadVec3( dest_position );
	savefile->ReadVec3( move_delta );

	savefile->ReadFloat( move_speed );
	savefile->ReadInt( move_time );
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

// RAVEN BEGIN
		PostEventMS( &EV_PostRestoreExt, 0, starttime, totaltime, accel, decel, useAngles );
// RAVEN END
	} 

// RAVEN BEGIN
// mekberg: added for attenuation and idle sound
	savefile->ReadBool( attenuate );
	savefile->ReadFloat( maxAttenuation );
	savefile->ReadFloat( attenuationScale );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadInt( lastTime );
	savefile->ReadBool( useIdleSound );
	splineStateThread.Restore( savefile, this );
	savefile->ReadInt( splineStartTime );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_moverCrush", false, false );
// RAVEN END
}

/*
================
idMover::Event_PostRestoreExt
================
*/
// RAVEN BEGIN
void idMover::Event_PostRestoreExt( int start, int total, int accel, int decel, bool useSplineAng ) {
// RAVEN END
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

	physicsObj.SetSpline( spline, accel, decel, useSplineAng );
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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

// RAVEN BEGIN
// abahr:
	if( spawnArgs.GetBool("removeGimbleLock") ) {
		physicsObj.SetAxisOffset( spawnArgs.GetMatrix("rotation", mat3_identity.ToString()) );
		physicsObj.SetAxis( mat3_identity );
		UpdateVisuals();
	}

// mekberg: attenuation
	attenuate = spawnArgs.GetBool( "attenuate" );
	if( attenuate ) {
		maxAttenuation		= spawnArgs.GetFloat( "maxAttenuation", "3" );
		attenuationScale	= spawnArgs.GetFloat( "attenuationScale", "100" );

		// Check for bad value/prevent divide by zero
		if ( attenuationScale == 0.0f || attenuationScale < 0.0f ) {
			attenuationScale = 100;
		}

		lastOrigin	= physicsObj.GetOrigin( );
		lastTime	= gameLocal.time;
	} else {
		maxAttenuation = 0.0f;
		attenuationScale = 1.0f;
	}

	if ( !idStr::Icmp( spawnArgs.GetString( "snd_idle", "" ), "" ) ) {
		useIdleSound = false;
	} else {
		StartSound( "snd_idle", SND_CHANNEL_BODY, 0, false, NULL );	
		useIdleSound = true;
	}	

	splineStateThread.SetName( "SplineStateThread" );
	splineStateThread.SetOwner( this );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_moverCrush", false, false );
// RAVEN END
}

// RAVEN BEGIN
// mekberg: added
/*
================
idMover::Think
================
*/
void idMover::Think( void ) {
	idVec3	deltaPosition;
	float	deltaTime;
	float	speed;
	float	attenuation;

	if ( physicsObj.GetSpline( ) ) {
		splineStateThread.Execute( );
	}

	if ( attenuate ) {
		deltaPosition	= physicsObj.GetOrigin( ) - lastOrigin;
		deltaTime		= gameLocal.time - lastTime;

		if ( !deltaTime ) {
			deltaTime = 1;
		}

		speed = deltaPosition.Length( ) * ( 1000.0f / float( deltaTime ) );

		if ( speed >= VECTOR_EPSILON ) {		
			soundShaderParms_t parms = refSound.parms;

			attenuation = 0.8f + 0.2f * ( speed / attenuationScale );
			parms.frequencyShift = idMath::ClampFloat( 0.0f, maxAttenuation, attenuation );

			idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
			if( emitter ) {
				emitter->ModifySound( SND_CHANNEL_BODY, &parms );
			}
		}

		lastOrigin	= physicsObj.GetOrigin( );
		lastTime	= gameLocal.time;
	}

	idEntity::Think( );
}

// abahr
/*
================
idMover::GetPhysicsToVisualTransform
================
*/
bool idMover::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin.Zero();
	axis = physicsObj.GetAxisOffset();
	return physicsObj.UseAxisOffset();
}

/*
================
idMover::MoveAlongVector
================
*/
void idMover::MoveAlongVector( const idVec3& vec ) {
	idAngles	ang;
	idVec3		org;

	physicsObj.GetLocalOrigin( org );
	physicsObj.GetLocalAngles( ang );
	dest_position = org + vec * ang.ToMat3();

	BeginMove( idThread::CurrentThread() );
}

/*
================
idMover::Event_MoveAlongVector
================
*/
void idMover::Event_MoveAlongVector( const idVec3& vec ) {
	MoveAlongVector( vec );
}
// RAVEN END

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

	if ( lastCommand != MOVER_SPLINE ) {
		// set our final position so that we get rid of any numerical inaccuracy
		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );
	}

	lastCommand	= MOVER_NONE;

// RAVEN BEGIN
// kfuller: added sig reached
	Signal(SIG_REACHED);
// RAVEN END

	idThread::ObjectMoveDone( move_thread, this );
	move_thread = 0;

// RAVEN BEGIN
// mekberg: for idle sound
	if ( !useIdleSound ) {
		StopSound( SND_CHANNEL_BODY, false );
	}
}

/*
================
idMover::UpdateMoveSound
================
*/
void idMover::UpdateMoveSound( moveStage_t stage ) {
// RAVEN BEGIN
// mekberg: Idle sound plays instead of snd_move. Don't stop idle sounds.
	switch( stage ) {
		case ACCELERATION_STAGE: {
			StartSound( "snd_accel", SND_CHANNEL_BODY2, 0, false, NULL );
			if ( !useIdleSound ) {
				StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			}
			break;
		}
		case LINEAR_STAGE: {
			if ( !useIdleSound ) {
				StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			}
			break;
		}
		case DECELERATION_STAGE: {
			if ( !useIdleSound ) {
				StopSound( SND_CHANNEL_BODY, false );	
			}
			StartSound( "snd_decel", SND_CHANNEL_BODY2, 0, false, NULL );
			break;
		}
		case FINISHED_STAGE: {
			if ( !useIdleSound ) {
				StopSound( SND_CHANNEL_BODY, false );
			}			
			// cnicholson: added stop sound support
			StartSound( "snd_stopped", SND_CHANNEL_BODY2, 0, false, NULL );
			break;
		}
	}
// RAVEN END
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

// RAVEN BEGIN
// kfuller: added reached signal
	Signal(SIG_REACHED);
// RAVEN END

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
// RAVEN BEGIN
// cnicholson: added stop sound support
			StartSound( "snd_stopped", SND_CHANNEL_BODY, 0, false, NULL );
// RAVEN END
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
	if ( !blockingEntity->fl.takedamage ) {
		if ( blockingEntity->IsType( idAI::GetClassType() ) ) {
			//burning out already
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() )
					|| (blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() )) ) {
			//moveable
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		}
	} else {
		if ( blockingEntity->IsType( idAI::GetClassType() ) && blockingEntity->health <= 0 ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() ) ) {
			//damagable movable?
            blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
			return;
		} else if ( blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() ) ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		}
	}
	if ( g_debugMover.GetBool() ) {
		gameLocal.Printf( "%d: '%s' stopped due to team member '%s' blocked by '%s'\n", gameLocal.time, name.c_str(), blockedEntity->name.c_str(), blockingEntity->name.c_str() );
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
// RAVEN BEGIN
// abahr: added return so the NULL ptr doesn't get used
		return;
// RAVEN END
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
	physicsObj.SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), speed * 1000 * phase, speed * 500, org, depth * 2.0f, vec3_origin );
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
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), duration * 1000.0f * phase, duration * 1000.0f, ang, angSpeed, ang_zero );
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

// RAVEN BEGIN
// cnicholson: added stop sound support
/*
================
idMover::Event_SetStoppedSound
================
*/
void idMover::Event_SetStoppedSound( const char *sound ) {
//	refSound.SetSound( "stopped", sound );
}
// RAVEN END

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

// RAVEN BEGIN
// bdube: movement speed
	// Use movement speed?
	if ( idMath::Fabs(move_speed) >= VECTOR_EPSILON ) {
		// Set a fixed time to determine the length from
		spline->MakeUniform( 1000 );
		spline->ShiftTime( gameLocal.GetTime() - spline->GetTime( 0 ) );

		// Calculate the move time from the speed
		move.movetime = SEC2MS( spline->GetLengthForTime(spline->GetTime(spline->GetNumValues() - 1)) / move_speed );
		move_time = move.movetime;

		spline->SetConstantSpeed( move.movetime );
		spline->ShiftTime( gameLocal.GetTime() - spline->GetTime( 0 ) );
	} else {
		spline->MakeUniform( move_time );
		spline->ShiftTime( gameLocal.GetTime() - spline->GetTime( 0 ) );
	}
// RAVEN END

	if ( acceltime + deceltime > move_time ) {
		acceltime = move_time / 2;
		deceltime = move_time - acceltime;
	}

	move.stage			= FINISHED_STAGE;
	move.acceleration	= acceltime;
	move.movetime		= move_time;
	move.deceleration	= deceltime;

	physicsObj.SetSpline( spline, move.acceleration, move.deceleration, useSplineAngles );
	physicsObj.SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, dest_position, vec3_origin, vec3_origin );

// RAVEN BEGIN
// mekberg: let the splines use a state thread instead of linear extrapolation
	if ( acceltime ) {
		splineStateThread.SetState( "Accel" );
	} else {
		splineStateThread.SetState( "Linear" );
	}	
	
	splineStartTime = gameLocal.time;
// RAVEN END
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
	msg.WriteBits( ( ( thinkFlags & TH_PHYSICS ) != 0 ), 1 );
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

	// sync down the TH_PHYSICS flag for movers, now that we skip ClientPredictionThink when thinkFlags == 0 and no longer force TH_PHYSICS on
	// movers still have prediction issues though, they predict a stop and clear TH_PHYSICS too early
	bool physics_on = ( msg.ReadBits( 1 ) != 0 );
	if ( physics_on ) {
		thinkFlags |= TH_PHYSICS;
	} else {
		thinkFlags &= ~TH_PHYSICS;
	}

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

// RAVEN BEGIN
// abahr:
void idMover::Event_GetSplineEntity() {
	idThread::ReturnEntity( splineEnt.GetEntity() );
}

CLASS_STATES_DECLARATION( idMover )
	STATE( "Accel",				idMover::State_Accel )
	STATE( "Linear",			idMover::State_Linear )
	STATE( "Decel",				idMover::State_Decel )
END_CLASS_STATES

// mekberg: spline states
/*
================
idMover::State_Accel
================
*/
stateResult_t idMover::State_Accel( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch( parms.stage ) {
		case STAGE_INIT:
			StartSound( "snd_accel", SND_CHANNEL_BODY2, 0, false, NULL );
			if ( !useIdleSound ) {
				StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			}			
			return SRESULT_STAGE( STAGE_WAIT );

		case STAGE_WAIT:
			if ( gameLocal.time >= splineStartTime + acceltime ) {
				splineStateThread.SetState( "Linear" );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idMover::State_Linear
================
*/
stateResult_t idMover::State_Linear( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch( parms.stage ) {
		case STAGE_INIT:
			if ( !useIdleSound ) {
				StartSound( "snd_move", SND_CHANNEL_BODY, 0, false, NULL );
			}
			return SRESULT_STAGE( STAGE_WAIT );			

		case STAGE_WAIT:
			if ( gameLocal.time >= splineStartTime + move_time - deceltime ) {
				if ( deceltime ) {
					splineStateThread.SetState( "Decel" );
				}
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idMover::State_Decel
================
*/
stateResult_t idMover::State_Decel( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};

	switch( parms.stage ) {
		case STAGE_INIT:
			if ( !useIdleSound ) {
				StopSound( SND_CHANNEL_BODY, false );	
			}
			StartSound( "snd_decel", SND_CHANNEL_BODY2, 0, false, NULL );
			return SRESULT_STAGE( STAGE_WAIT );

		case STAGE_WAIT:
			if ( gameLocal.time >= splineStartTime + move_time ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
// RAVEN END

/*
===============================================================================

	idSplinePath, holds a spline path to be used by an idMover

===============================================================================
*/

// RAVEN BEGIN
// abahr: so we can toggle activated state via script or trigger
const idEventDef EV_IsActive( "isActive", "", 'd' );
// RAVEN END

CLASS_DECLARATION( idEntity, idSplinePath )
// RAVEN BEGIN
// abahr: so we can toggle activated state via script or trigger
	EVENT( EV_Activate,			idSplinePath::Event_Toggle )
	EVENT( EV_IsActive,			idSplinePath::Event_IsActive )
// RAVEN END
END_CLASS

/*
================
idSplinePath::idSplinePath
================
*/
idSplinePath::idSplinePath() {
	sampledTimes	= NULL;
	numSamples	= 0;
}

/*
================
idSplinePath::idSplinePath
================
*/
idSplinePath::~idSplinePath() {
	if ( sampledTimes ) {
		delete [] sampledTimes;
		sampledTimes = NULL;
	}
}

/*
================
idSplinePath::Spawn
================
*/
void idSplinePath::Spawn( void ) {
// RAVEN BEGIN
// abahr:
	SetActive( spawnArgs.GetBool("start_active", "1") );
	SampleSpline ( );
// RAVEN END
}

// RAVEN BEGIN
// abahr:
/*
================
idSplinePath::FindTargets
================
*/
void idSplinePath::FindTargets() {
	idEntity::FindTargets();

	gameLocal.GetTargets( spawnArgs, backwardPathTargets, "target_reverse" );

	// This alows us to seperate forward targets from backward targets
	for( int ix = backwardPathTargets.Num() - 1; ix >= 0; --ix ) {
		targets.Remove( backwardPathTargets[ix] );
	}
}

/*
================
idSplinePath::SortTargets
================
*/
int rvSortByActiveState( const void* a, const void* b ) {
	idEntityPtr<idSplinePath>	splineA;
	idEntityPtr<idSplinePath>	splineB;

	splineA	= *(idEntityPtr<idSplinePath>*)a;
	splineB = *(idEntityPtr<idSplinePath>*)b;

	return splineB->IsActive() - splineA->IsActive();
}
int idSplinePath::SortTargets( idList< idEntityPtr<idEntity> >& list ) {
	int numActive = 0;
	idSplinePath* target = NULL;

	RemoveNullTargets();

	qsort( list.Ptr(), list.Num(), list.TypeSize(), rvSortByActiveState );
	for( int ix = list.Num() - 1; ix >= 0; --ix ) {
		target = static_cast<idSplinePath*>( list[ix].GetEntity() );
		if( target->IsActive() ) {
			numActive++;
		}
	}

	return numActive;
}

int idSplinePath::SortTargets() {
	return SortTargets( targets );
}

int idSplinePath::SortBackwardsTargets() {
	return SortTargets( backwardPathTargets );
}

/*
================
idSplinePath::RemoveNullTargets
================
*/
void idSplinePath::RemoveNullTargets( idList< idEntityPtr<idEntity> >& list ) {
	int i;

	for( i = list.Num() - 1; i >= 0; i-- ) {
		if ( !list[ i ].GetEntity() ) {
			list.RemoveIndex( i );
		}
	}
}

/*
================
idSplinePath::RemoveNullTargets
================
*/
void idSplinePath::ActivateTargets( idEntity *activator, const idList< idEntityPtr<idEntity> >& list ) const {
	idEntity	*ent;
	int			i, j;
	
	for( i = 0; i < list.Num(); i++ ) {
		ent = list[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
			ent->Signal( SIG_TRIGGER );
			ent->ProcessEvent( &EV_Activate, activator );
		} 		
		for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->GetRenderEntity()->gui[ j ] ) {
				ent->GetRenderEntity()->gui[ j ]->Trigger( gameLocal.time );
			}
		}
	}
}

/*
================
idSplinePath::RemoveNullTargets
================
*/
void idSplinePath::RemoveNullTargets( void ) {
	RemoveNullTargets( targets );
	RemoveNullTargets( backwardPathTargets );
}

/*
==============================
idSplinePath::ActivateTargets

"activator" should be set to the entity that initiated the firing.
==============================
*/
void idSplinePath::ActivateTargets( idEntity *activator ) const {
	ActivateTargets( activator, targets );
	ActivateTargets( activator, backwardPathTargets );
}

/*
================
idSplinePath::Save
================
*/
void idSplinePath::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( active );
}

/*
================
idSplinePath::Restore
================
*/
void idSplinePath::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( active );
}

/*
================
idSplinePath::Event_IsActive
================
*/
void idSplinePath::Event_IsActive() {
	idThread::ReturnInt( IsActive() );
}

/*
================
idSplinePath::SampleSpline
================
*/
void idSplinePath::SampleSpline ( void ) {
	int i;
	float splineLength;
	idCurve_Spline<idVec3>*	tempSpline = GetSpline ( );
	splineLength = tempSpline->GetLengthForTime( tempSpline->GetTime(tempSpline->GetNumValues() - 1) );

	if ( splineLength > SPLINE_SAMPLE_RATE ) {
		numSamples		= int( splineLength / SPLINE_SAMPLE_RATE );
		sampledTimes	= new float[ numSamples ];
		float stepSize	= splineLength / ( numSamples - 1 );

		for ( i = 0; i < numSamples; i++ ) {
			float time = float( i * stepSize );
			if ( time >= splineLength ) {
				time = splineLength;
			}
			sampledTimes[ i ] = tempSpline->GetTimeForLength ( time, 0.01f );
		}
	}
	SAFE_DELETE_PTR( tempSpline );
}

/*
================
idSplinePath::GetSampledTime
================
*/
float idSplinePath::GetSampledTime ( float distance ) const {
	if ( sampledTimes && distance  >= 0.0f ) {
		int lowIndex, highIndex;
		float lerp = distance / SPLINE_SAMPLE_RATE;
		int actualLowIndex = int( lerp );
		lowIndex = idMath::ClampInt ( 0, numSamples - 2, actualLowIndex );
		highIndex = lowIndex + 1;
		lerp = lerp - idMath::Floor ( lerp );
		return ( actualLowIndex != lowIndex ) ? sampledTimes[ highIndex ] : idMath::Lerp( sampledTimes[ lowIndex ], sampledTimes[ highIndex ], lerp );
	}
	return -1.0f;
}

// RAVEN END


/*
===============================================================================

idElevator

===============================================================================
*/
const idEventDef EV_PostArrival( "postArrival", NULL );
const idEventDef EV_GotoFloor( "gotoFloor", "d" );
const idEventDef EV_UpdateFloorInfo ( "updateFloorInfo", NULL );

CLASS_DECLARATION( idMover, idElevator )
	EVENT( EV_Activate,				idElevator::Event_Activate )
	EVENT( EV_TeamBlocked,			idElevator::Event_TeamBlocked )
	EVENT( EV_PostArrival,			idElevator::Event_PostFloorArrival )
	EVENT( EV_GotoFloor,			idElevator::Event_GotoFloor )
	EVENT( EV_Touch,				idElevator::Event_Touch )
	EVENT( EV_UpdateFloorInfo,		idElevator::Event_UpdateFloorInfo )
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
	int i;

	savefile->WriteInt( (int)state );

	savefile->WriteInt( floorInfo.Num() );
	for ( i = 0; i < floorInfo.Num(); i++ ) {
		savefile->WriteVec3( floorInfo[ i ].pos );
		savefile->WriteString( floorInfo[ i ].door );
		savefile->WriteInt( floorInfo[ i ].floor );
	}

	savefile->WriteInt( currentFloor );
	savefile->WriteInt( pendingFloor );
	savefile->WriteInt( lastFloor );
	savefile->WriteBool( controlsDisabled );
//	savefile->WriteBool( waitingForPlayerFollowers );
	savefile->WriteFloat( returnTime );
	savefile->WriteInt( returnFloor );
	savefile->WriteInt( lastTouchTime );
}

/*
================
idElevator::Restore
================
*/
void idElevator::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadInt( (int &)state );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		floorInfo_s floor;

		savefile->ReadVec3( floor.pos );
		savefile->ReadString( floor.door );
		savefile->ReadInt( floor.floor );

		floorInfo.Append( floor );
	}

	savefile->ReadInt( currentFloor );
	savefile->ReadInt( pendingFloor );
	savefile->ReadInt( lastFloor );
	savefile->ReadBool( controlsDisabled );
//	savefile->ReadBool( waitingForPlayerFollowers );
	savefile->ReadFloat( returnTime );
	savefile->ReadInt( returnFloor );
	savefile->ReadInt( lastTouchTime );
}

/*
================
idElevator::Spawn
================
*/
void idElevator::Spawn( void ) {
	lastFloor = 0;
	currentFloor = 0;
	pendingFloor = spawnArgs.GetInt( "floor", "1" );
	SetGuiStates( ( pendingFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1]);

	returnTime = spawnArgs.GetFloat( "returnTime" );
	returnFloor = spawnArgs.GetInt( "returnFloor" );

	UpdateFloorInfo ( );

	lastTouchTime = 0;
	state = INIT;
	BecomeActive( TH_THINK | TH_PHYSICS );
	PostEventMS( &EV_Mover_InitGuiTargets, 0 );
	controlsDisabled = false;
//	waitingForPlayerFollowers = false;
}

/*
==============
idElevator::UpdateFloorInfo
===============
*/
void idElevator::UpdateFloorInfo ( void ) {
	int		len1;
	idStr	str;

	floorInfo.Clear ( );

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
}

/*
==============
idElevator::Event_UpdateFloorInfo
===============
*/
void idElevator::Event_UpdateFloorInfo ( void ) {
	UpdateFloorInfo ( );
}

/*
==============
idElevator::Event_Touch
===============
*/
void idElevator::Event_Touch( idEntity *other, trace_t *trace ) {
	
	if ( gameLocal.time < lastTouchTime + 2000 ) {
		return;
	}

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !other->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		return;
	}

	lastTouchTime = gameLocal.time;

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

// RAVEN BEGIN
// bdube: provide floor information to status guis
		if ( floorInfo.Num ( ) > 0 ) {
			int j;
			// Guis on the elevator are considered status guis
			for ( j = 0; j < MAX_RENDERENTITY_GUI && renderEntity.gui[j]; j++ ) {
				InitStatusGui ( renderEntity.gui[j] );
			}
			
			// Initialize all the status guis of the elevator
			const idKeyValue* kv;
			for ( kv = spawnArgs.MatchPrefix( "statusGui" ); kv; kv = spawnArgs.MatchPrefix( "statusGui", kv ) ) {
				idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
				if ( !ent || !ent->GetRenderEntity() ) {
					continue;
				}
				for ( j = 0; j < MAX_RENDERENTITY_GUI && ent->GetRenderEntity()->gui[j]; j++ ) {
					InitStatusGui ( ent->GetRenderEntity()->gui[j] );
				}
			}
		}
// RAVEN END

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
	if ( !blockingEntity->fl.takedamage ) {
		if ( blockingEntity->IsType( idAI::GetClassType() ) ) {
			//burning out already
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() )
					|| (blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() )) ) {
			//moveable
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		}
	} else {
		if ( blockingEntity->IsType( idAI::GetClassType() ) && blockingEntity->health <= 0 ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() ) ) {
			//damagable movable?
            blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
			return;
		} else if ( blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() ) ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		}
	}
	if ( blockedEntity == this ) {
		Event_GotoFloor( lastFloor );
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	} else if ( blockedEntity && blockedEntity->IsType( idDoor::GetClassType() ) ) {
// RAVEN END
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
// RAVEN BEGIN
// bdube: up and down floor commands
			int newFloor;
			if (!token.Cmp("up")) {
				newFloor = currentFloor + 1;
			} else if (!token.Cmp("down")) {
				newFloor = currentFloor - 1;
			} else {
				newFloor = atoi( token );
			}
// RAVEN END
			
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
		/*
		if ( !gameLocal.isMultiplayer ) {
			//FIXME: make sure player is my activator?
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( player ) {
				if ( !player->GetGroundEntity() ) {
					//player in air
					PostEventSec( &EV_GotoFloor, 0.5f, floor );
					return;
				}
				if ( player->GetGroundElevator() == this ) {
					idActor* actor = NULL;
					// Iterate through all teammates
					for( actor = aiManager.GetAllyTeam ( (aiTeam_t)player->team ); actor; actor = actor->teamNode.Next() ) {
						if ( !actor->IsHidden() && actor->health > 0 && actor->IsType( idAI::GetClassType() ) ) {
							if ( ((idAI*)(actor))->leader == player && !((idAI*)(actor))->move.fl.disabled && !((idAI*)(actor))->aifl.scripted ) {
								if ( actor->GetGroundElevator( this ) != this ) {
									waitingForPlayerFollowers = true;
									//follower of player is not standing on me, don't move!
									PostEventSec( &EV_GotoFloor, 0.5f, floor );
									return;
								}
							}
						}
					}
				} else if ( waitingForPlayerFollowers ) {
					//player got off, cancel
					waitingForPlayerFollowers = false;
					SetGuiStates( ( currentFloor == 1 ) ? guiBinaryMoverStates[0] : guiBinaryMoverStates[1] );
					UpdateStatusGuis ( );
					return;
				}
			}
		}
		waitingForPlayerFollowers = false;
		*/
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
	SetGuiStates( ( pendingFloor == 1 ) ? guiBinaryMoverStates[3] : guiBinaryMoverStates[2] );

// RAVEN BEGIN
// bdube: replaced with function
	SetAASAreaState ( true );

	idMover::BeginMove( thread );

	UpdateStatusGuis ( );	
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent && ent->IsType( idDoor::GetClassType() ) ) {
// RAVEN END
			doorEnt = static_cast<idDoor*>( ent );
			master = doorEnt->GetMoveMaster();
			if ( master != doorEnt ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
				if ( master->IsType( idDoor::GetClassType() ) ) {
// RAVEN END
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

/*
================
idElevator::DoneMoving
================
*/
void idElevator::DoneMoving( void ) {
	idMover::DoneMoving();
	EnableProperDoors();
// RAVEN BEGIN
// bdube: factored into a function
	UpdateStatusGuis ( );
// RAVEN END	
	if ( spawnArgs.GetInt( "pauseOnFloor", "-1" ) == currentFloor ) {
		PostEventSec( &EV_PostArrival, spawnArgs.GetFloat( "pauseTime" ) );
	} else {
		Event_PostFloorArrival();
	}

// RAVEN BEGIN
// kfuller: we want an elevator to fire its targets when it reaches a floor
	SetAASAreaState( false );

	// Floor targets
	if ( lastFloor != 0 ) {
		const char* floorTarget = spawnArgs.GetString ( va("floorTarget_%d", currentFloor ) );
		if ( floorTarget && *floorTarget ) {
			idEntity* ent = gameLocal.FindEntity ( floorTarget );
			if ( ent ) {
				ent->ProcessEvent ( &EV_Activate, this );
			}
		}
	}

	if (spawnArgs.GetInt("fireTargetsAtFloor") == currentFloor) {
		ActivateTargets(gameLocal.entities[ENTITYNUM_WORLD]);
		spawnArgs.SetInt("fireTargetsAtFloor", -1);
	}
// RAVEN END
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

// RAVEN BEGIN
// bdube: more advanced status gui control

/*
================
idElevator::SetAASAreaState
================
*/
void idElevator::SetAASAreaState ( bool enable ) {
	idEntity* ents[16];
	int		  numEnts;
	numEnts = gameLocal.EntitiesTouchingBounds ( this, physicsObj.GetAbsBounds(), CONTENTS_AAS_OBSTACLE, ents, 16 );
	for ( numEnts--; numEnts >= 0; numEnts -- ) {
		idFuncAASObstacle* obstacle = dynamic_cast<idFuncAASObstacle*>(ents[numEnts]);
		if ( obstacle ) {
			obstacle->SetState ( enable );
		}
	}
}

/*
================
idElevator::InitStatusGui
================
*/
void idElevator::InitStatusGui ( idUserInterface* gui ) {
	int floor;
	int topFloor;
	int bottomFloor;
	
	topFloor = -1;
	bottomFloor = 9999;
	for ( floor = 0; floor < floorInfo.Num(); floor ++ ) {
		topFloor = Max( floorInfo[floor].floor, topFloor );
		bottomFloor = Min( floorInfo[floor].floor, bottomFloor );
	}

	gui->SetStateInt ( "topFloor", topFloor );
	gui->SetStateInt ( "bottomFloor", bottomFloor );

	for ( floor = 0; floor < floorInfo.Num(); floor ++ ) {
		idStr keySrc;
		idStr keyDest;
		gui->SetStateInt ( va("floorNumber_%d", floor ), floorInfo[floor].floor );
		keySrc = va("floorName_%d", floorInfo[floor].floor );
		keyDest = va("floorName_%d", floor );
		gui->SetStateString ( keyDest, spawnArgs.GetString ( keySrc, va("%d", floorInfo[floor].floor ) ) );
	}

	gui->HandleNamedEvent ( "updateFloor" );
}

/*
================
idElevator::UpdateStatusGui
================
*/
void idElevator::UpdateStatusGui ( idUserInterface* gui ) {
	idStr floorName;
	floorName = va("floorName_%d", currentFloor );
	gui->SetStateInt ( "floor", (physicsObj.GetLinearExtrapolationType() == EXTRAPOLATION_NONE) ? currentFloor : lastFloor );
	gui->SetStateInt ( "floorNext", currentFloor );
	gui->SetStateString( "floorName", spawnArgs.GetString ( floorName, va("%d", currentFloor ) ) );
	gui->StateChanged( gameLocal.time, true );

	// mekberg: trigger all status guis if we moved the elevator from another gui.
	if ( lastFloor && !( physicsObj.GetLinearExtrapolationType() == EXTRAPOLATION_NONE ) ) {
		gui->HandleNamedEvent( "triggerGui" );
	}

	gui->HandleNamedEvent ( "updateFloor" );
}

/*
================
idElevator::UpdateStatusGuis
================
*/
void idElevator::UpdateStatusGuis ( void ) {
	int j;
	
	// Treat the guis on the elevator as status guis
	for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
		if ( renderEntity.gui[ j ] ) {
			UpdateStatusGui ( renderEntity.gui[ j ] );
		}
	}

	// All entities linked as status guis should get updated
	const idKeyValue *kv = spawnArgs.MatchPrefix( "statusGui" );
	while( kv ) {
		idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
		if ( ent ) {
			for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
				if ( ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ j ] ) {
					UpdateStatusGui ( ent->GetRenderEntity()->gui[j] );
				}
			}
			ent->UpdateVisuals();
		}
		kv = spawnArgs.MatchPrefix( "statusGui", kv );
	}
}
// RAVEN END

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
	deferedOpen = false;
	move_thread = 0;
	updateStatus = 0;
	areaPortal = 0;
	blocked = false;
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
	
	SetPhysics( NULL );
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
	savefile->WriteBool( deferedOpen );

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
	savefile->ReadBool( deferedOpen );

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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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
	idVec3 	delta;

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
	deferedOpen = false;
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

Sets the portal associtated with this mover to be open
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

// RAVEN BEGIN
// jdischler: this wasn't actually doing anything, anyway
//		UpdateBuddies( 1 );
// RAVEN END

		if ( enabled && wait >= 0 && !spawnArgs.GetBool( "toggle" ) ) {
			// return to pos1 after a delay
			PostEventSec( &EV_Mover_ReturnToPos1, wait );
		}

		// fire targets
		ActivateTargets( moveMaster->GetActivator() );
		
		SetBlocked ( false );
	} else if ( moverState == MOVER_2TO1 ) {
		// reached pos1
		idThread::ObjectMoveDone( move_thread, this );
		move_thread = 0;

		SetMoverState( MOVER_POS1, gameLocal.time );

		SetGuiStates( guiBinaryMoverStates[MOVER_POS1] );

// RAVEN BEGIN
// jdischler: this wasn't actually doing anything, anyway
//		UpdateBuddies( 0 );
// RAVEN END

		// close areaportals
		if ( moveMaster == this ) {
// RAVEN BEGIN
// kfuller: added "snd_closed"
			StartSound( "snd_closed", SND_CHANNEL_ANY, 0, false, NULL );
// RAVEN END
			ProcessEvent( &EV_Mover_ClosePortal );
		}

		if ( enabled && wait >= 0 && spawnArgs.GetBool( "continuous" ) ) {
			PostEventSec( &EV_Activate, wait, this );
		}

		SetBlocked ( false );
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
void idMover_Binary::UpdateBuddies( int val ) 
{
// RAVEN BEGIN
// jdischler: was using update status but that was never getting set anyway.
// Additionally, shaderparm_mode was never getting set on the mover itself, which creates
// extra work for the designers.
	int c = buddies.Num();
	for ( int i = 0; i < c; i++ ) {
		idEntity *buddy = gameLocal.FindEntity( buddies[i] );
		if ( buddy ) {
			buddy->SetShaderParm( SHADERPARM_MODE, val );
			buddy->UpdateVisuals();
		}
	}
	// Update the mover itself, too.
	SetShaderParm( SHADERPARM_MODE, val );
	UpdateVisuals();
// RAVEN END
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

	if ( moverState == MOVER_POS1 && !deferedOpen) {

		float openWait = spawnArgs.GetFloat( "openWait", "0" );
		deferedOpen = true;
		PostEventMS( &EV_Mover_MatchTeam, SEC2MS(openWait), MOVER_1TO2, SEC2MS(openWait) + gameLocal.time );

		// TODO: might want to delay these as well if openWait is present?
		SetGuiStates( guiBinaryMoverStates[MOVER_1TO2] );
		// open areaportal
		ProcessEvent( &EV_Mover_OpenPortal );

		// any things we should trigger when the door is first asked to open?
		//	NOTE: with openWait, it's possible (and desirable as per aweldon's request)
		//		that this will be called before the door actually opens.  This is specifically
		//		used by the rotate/lock doors...the triggering below starts another entity rotating
		//		and onWait is used to offset the actual open so the rotating piece has time to work...
		const idKeyValue *kv = spawnArgs.MatchPrefix( "triggerOnOpen" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 0, moveMaster->GetActivator() );
			}
			kv = spawnArgs.MatchPrefix( "triggerOnOpen", kv );
		}

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
const idEventDef EV_Door_Open( "open", NULL );
const idEventDef EV_Door_Close( "close", NULL );
const idEventDef EV_Door_Lock( "lock", "d" );
const idEventDef EV_Door_IsOpen( "isOpen", NULL, 'f' );
const idEventDef EV_Door_IsLocked( "isLocked", NULL, 'f' );

CLASS_DECLARATION( idMover_Binary, idDoor )
	EVENT( EV_TeamBlocked,				idDoor::Event_TeamBlocked )
	EVENT( EV_PartBlocked,				idDoor::Event_PartBlocked )
	EVENT( EV_Touch,					idDoor::Event_Touch )
	EVENT( EV_Activate,					idDoor::Event_Activate )
	EVENT( EV_Door_StartOpen,			idDoor::Event_StartOpen )
	EVENT( EV_Door_SpawnDoorTrigger,	idDoor::Event_SpawnDoorTrigger )
	EVENT( EV_Door_SpawnSoundTrigger,	idDoor::Event_SpawnSoundTrigger )
	EVENT( EV_Door_Open,				idDoor::Event_Open )
	EVENT( EV_Door_Close,				idDoor::Event_Close )
	EVENT( EV_Door_Lock,				idDoor::Event_Lock )
	EVENT( EV_Door_IsOpen,				idDoor::Event_IsOpen )
	EVENT( EV_Door_IsLocked,			idDoor::Event_Locked )
	EVENT( EV_ReachedPos,				idDoor::Event_Reached_BinaryMover )
	EVENT( EV_SpectatorTouch,			idDoor::Event_SpectatorTouch )
	EVENT( EV_Mover_OpenPortal,			idDoor::Event_OpenPortal )
	EVENT( EV_Mover_ClosePortal,		idDoor::Event_ClosePortal )
// RAVEN BEGIN
// abahr:
	EVENT( EV_Mover_ReturnToPos1,		idDoor::Event_ReturnToPos1 )
// RAVEN END
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
	nextSndTriggerTime = 0;
	localTriggerOrigin.Zero();
	localTriggerAxis.Identity();
	requires.Clear();
	removeItem = 0;
	syncLock.Clear();
	companionDoor = NULL;
	normalAxisIndex = 0;
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
}

/*
================
idDoor::Save
================
*/
void idDoor::Save( idSaveGame *savefile ) const {

	savefile->WriteFloat( triggersize );
	savefile->WriteBool( crusher );
	savefile->WriteBool( noTouch );
	savefile->WriteBool( aas_area_closed );
	savefile->WriteString( buddyStr );

	savefile->WriteClipModel( trigger );
	savefile->WriteClipModel( sndTrigger );
	savefile->WriteInt( nextSndTriggerTime );

	savefile->WriteVec3( localTriggerOrigin );
	savefile->WriteMat3( localTriggerAxis );

	savefile->WriteString( requires );
	savefile->WriteInt( removeItem );
	savefile->WriteString( syncLock );
	savefile->WriteInt( normalAxisIndex );

	savefile->WriteObject( companionDoor );

// RAVEN BEGIN
// abahr:
	doorFrameController.Save( savefile );
// RAVEN END
}

/*
================
idDoor::Restore
================
*/
void idDoor::Restore( idRestoreGame *savefile ) {

	savefile->ReadFloat( triggersize );
	savefile->ReadBool( crusher );
	savefile->ReadBool( noTouch );
	savefile->ReadBool( aas_area_closed );
	SetAASAreaState( aas_area_closed );
	savefile->ReadString( buddyStr );

	savefile->ReadClipModel( trigger );
	savefile->ReadClipModel( sndTrigger );
	savefile->ReadInt( nextSndTriggerTime );

	savefile->ReadVec3( localTriggerOrigin );
	savefile->ReadMat3( localTriggerAxis );

	savefile->ReadString( requires );
	savefile->ReadInt( removeItem );
	savefile->ReadString( syncLock );
	savefile->ReadInt( normalAxisIndex );

	savefile->ReadObject( reinterpret_cast<idClass *&>( companionDoor ) );

// RAVEN BEGIN
// abahr:
	doorFrameController.Restore( savefile );
// RAVEN END
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
	if ( !spawnArgs.GetFloat( "movedir", "0", dir ) ) {
		// no movedir, so angle defines movement direction and not orientation,
		// a la oldschool Quake
		SetAngles( ang_zero );
		spawnArgs.GetFloat( "angle", "0", dir );
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

	// expects syncLock to be a door that must be closed before this door will open
	spawnArgs.GetString( "syncLock", "", syncLock );

	spawnArgs.GetString( "buddy", "", buddyStr );

	spawnArgs.GetString( "requires", "", requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );

	// ever separate piece of a door is considered solid when other team mates push entities
	fl.solidForTeam = true;

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
		if ( noTouch || health ) {
			// non touch/shoot doors
			PostEventMS( &EV_Mover_MatchTeam, 0, moverState, gameLocal.time );

			const char *sndtemp = spawnArgs.GetString( "snd_locked" );
			if ( spawnArgs.GetInt( "locked" ) && sndtemp && *sndtemp ) {
				PostEventMS( &EV_Door_SpawnSoundTrigger, 0 );
			}
		} else {
			// spawn trigger
			PostEventMS( &EV_Door_SpawnDoorTrigger, 0 );
		}
	}

	// see if we are on an areaportal
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );
	if ( !start_open ) {
		// start closed
		ProcessEvent( &EV_Mover_ClosePortal );
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
	
// RAVEN BEGIN
// bdube: added
	// Instruct ai to avoid standing in doors
	if ( !spawnArgs.GetBool ( "noavoid" ) ) {
		aiManager.AddAvoid ( GetPhysics()->GetAbsBounds().GetCenter(), GetPhysics()->GetBounds().GetRadius(), -1 );
	}
// RAVEN END
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
				trigger->Link( this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
// RAVEN END
			}
			if ( sndTrigger ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
				sndTrigger->Link( this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
// RAVEN END
			}
		}
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
	gameLocal.SetAASAreaState( physicsObj.GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL|AREACONTENTS_OBSTACLE, closed );
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
	if ( gameLocal.RequirementMet( activator, requires, removeItem ) ) {
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
	} 
}

/*
================
idDoor::Open
================
*/
void idDoor::Open( void ) {
	GotoPosition2();
}

/*
================
idDoor::Close
================
*/
void idDoor::Close( void ) {
	GotoPosition1();
}

/*
================
idDoor::Lock
================
*/
void idDoor::Lock( int f ) {
	idMover_Binary *other;

	// lock all the doors on the team
	for( other = moveMaster; other != NULL; other = other->GetActivateChain() ) {
		if ( other->IsType( idDoor::Type ) ) {
			idDoor *door = static_cast<idDoor *>( other );
			if ( other == moveMaster ) {
				if ( door->sndTrigger == NULL ) {
					// in this case the sound trigger never got spawned
					const char *sndtemp = door->spawnArgs.GetString( "snd_locked" );
					if ( sndtemp && *sndtemp ) {
						door->PostEventMS( &EV_Door_SpawnSoundTrigger, 0 );
					}
				}
				if ( !f && ( door->spawnArgs.GetInt( "locked" ) != 0 ) ) {
					door->StartSound( "snd_unlocked", SND_CHANNEL_ANY, 0, false, NULL );
				}
			}
			door->spawnArgs.SetInt( "locked", f );
// RAVEN BEGIN
// jdischler
			// locking/unlocking doors should update the shaderparm7 of any buddies
			door->UpdateBuddies( !f );
// RAVEN END

			if ( ( f == 0 ) || ( !IsHidden() && ( door->moverState == MOVER_POS1 ) ) ) {
				door->SetAASAreaState( f != 0 );
			}
		}
	}

	if ( f ) {
		Close();
	}
}

/*
================
idDoor::IsLocked
================
*/
int idDoor::IsLocked( void ) {
	return spawnArgs.GetInt( "locked" );
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
	
	if ( health > 0 ) {
		fl.takedamage = true;
	}
	for( other = activateChain; other != NULL; other = other->GetActivateChain() ) {
		if ( other->IsType( idDoor::Type ) ) {
			// find the bounds of everything on the team
			bounds.AddBounds( other->GetPhysics()->GetAbsBounds() );

			// set all of the slaves as shootable
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

// RAVEN BEGIN
// abahr:
/*
==============================
idDoor::SetDoorFrameController
==============================
*/
void idDoor::SetDoorFrameController( idEntity* controller ) {
	doorFrameController = controller;
}

/*
==============================
idDoor::ActivateTargets

If we have a frame controller we activate its targets
==============================
*/
void idDoor::ActivateTargets( idEntity *activator ) const {
	if ( doorFrameController.IsValid() && static_cast< const idMover_Binary *>( GetMoveMaster() ) == this && moverState == MOVER_POS1 ) {
		doorFrameController->ActivateTargets( activator );
	}

	idMover_Binary::ActivateTargets( activator );
}
// RAVEN END

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

	// create a trigger clip model
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	trigger = new idClipModel( idTraceModel( bounds ) );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	trigger->Link( this, 255, GetPhysics()->GetOrigin(), mat3_identity );
// RAVEN END
	trigger->SetContents( CONTENTS_TRIGGER );

	GetLocalTriggerPosition( trigger );

	MatchActivateTeam( moverState, gameLocal.time );
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	sndTrigger = new idClipModel( idTraceModel( bounds ) );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	sndTrigger->Link( this, 254, GetPhysics()->GetOrigin(), mat3_identity );
// RAVEN END
	sndTrigger->SetContents( CONTENTS_TRIGGER );

	GetLocalTriggerPosition( sndTrigger );
}

/*
================
idDoor::Event_Reached_BinaryMover
================
*/
void idDoor::Event_Reached_BinaryMover( void ) {
	if ( moverState == MOVER_2TO1 ) {
		SetBlocked( false );
		const idKeyValue *kv = spawnArgs.MatchPrefix( "triggerClosed" );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 0, moveMaster->GetActivator() );
			}
			kv = spawnArgs.MatchPrefix( "triggerClosed", kv );
		}
	} else if ( moverState == MOVER_1TO2 ) {
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
void idDoor::Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity ) {
	if ( !blockingEntity->fl.takedamage ) {
		if ( blockingEntity->IsType( idAI::GetClassType() ) ) {
			//burning out already
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() )
					|| (blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() )) ) {
			//moveable
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		}
	} else {
		if ( blockingEntity->IsType( idAI::GetClassType() ) && blockingEntity->health <= 0 ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() ) ) {
			//damagable movable?
            blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
			return;
		} else if ( blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() ) ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		}
	}
	SetBlocked( true );

	if ( crusher ) {
		return;		// crushers don't reverse
	}

	// reverse direction
	Use_BinaryMover( moveMaster->GetActivator() );

	if ( companionDoor ) {
		companionDoor->ProcessEvent( &EV_TeamBlocked, blockedEntity, blockingEntity );
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
void idDoor::Event_PartBlocked( idEntity *blockingEntity ) {
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
}

// RAVEN BEGIN
// abahr:
/*
================
idDoor::Event_ReturnToPos1
================
*/
void idDoor::Event_ReturnToPos1( void ) {
	idMover_Binary::Event_ReturnToPos1();

	if( doorFrameController.IsValid() ) {
		doorFrameController->ProcessEvent( &EV_CloseGate );
	}
}
// RAVEN END

/*
================
idDoor::Event_Touch
================
*/
void idDoor::Event_Touch( idEntity *other, trace_t *trace ) {
	idVec3		contact, translate;
	idVec3		planeaxis1, planeaxis2, normal;
	idBounds	bounds;

	if ( !enabled ) {
		return;
	}

	if ( trigger && trace->c.id == trigger->GetId() ) {
		if ( !IsNoTouch() && !IsLocked() && GetMoverState() != MOVER_1TO2 ) {
// RAVEN BEGIN
// abahr: allowing animated door frames
			if( doorFrameController.IsValid() && doorFrameController != other ) {
				doorFrameController->ProcessEvent( &EV_Touch, other, trace );
			} else {
				Use( this, other );
			}
// RAVEN END
		}
	} else if ( sndTrigger && trace->c.id == sndTrigger->GetId() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( other && other->IsType( idPlayer::GetClassType() ) && IsLocked() && gameLocal.time > nextSndTriggerTime ) {
// RAVEN END
			StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
			nextSndTriggerTime = gameLocal.time + 10000;
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

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	assert( other && other->IsType( idPlayer::GetClassType() ) && static_cast< idPlayer * >( other )->spectating );
// RAVEN END

	p = static_cast< idPlayer * >( other );
	// avoid flicker when stopping right at clip box boundaries
	if ( p->lastSpectateTeleport > gameLocal.time - 1000 ) {
		return;
	}
	if ( trigger && !IsOpen() ) {
		// teleport to the other side, center to the middle of the trigger brush
		bounds = trigger->GetAbsBounds();
		contact = trace->endpos - bounds.GetCenter();
		translate = bounds.GetCenter();
		normal.Zero();
		normal[ normalAxisIndex ] = 1.0f;
		if ( normal * contact > 0 ) {
			translate[ normalAxisIndex ] += ( bounds[ 0 ][ normalAxisIndex ] - translate[ normalAxisIndex ] ) * 0.5f;
		} else {
			translate[ normalAxisIndex ] += ( bounds[ 1 ][ normalAxisIndex ] - translate[ normalAxisIndex ] ) * 0.5f;
		}
		p->SetOrigin( translate );
		p->lastSpectateTeleport = gameLocal.time;
	}
}

/*
================
idDoor::Event_Activate
================
*/
void idDoor::Event_Activate( idEntity *activator ) {
	int old_lock;

	if ( spawnArgs.GetInt( "locked" ) ) {
		if ( !trigger ) {
			PostEventMS( &EV_Door_SpawnDoorTrigger, 0 );
		}
		UpdateBuddies( 1 );

		old_lock = spawnArgs.GetInt( "locked" );
		Lock( 0 );
		if ( old_lock == 2 ) {
			return;
		}
	}

  	if ( syncLock.Length() ) {
		idEntity *sync = gameLocal.FindEntity( syncLock );
		if ( sync && sync->IsType( idDoor::Type ) ) {
			if ( static_cast<idDoor *>( sync )->IsOpen() ) {
  				return;
  			}
  		}
	}

// RAVEN BEGIN
// abahr:
	if( doorFrameController.IsValid() && doorFrameController != activator ) {
		doorFrameController->ProcessEvent( &EV_Activate, activator );
	} else {
		ActivateTargets( activator );

		renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;
		UpdateVisuals();

		Use_BinaryMover( activator );	
	}
//RAVEN END
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
				trigger->Link( this, 0, masterOrigin + localTriggerOrigin * masterAxis, localTriggerAxis * masterAxis );
// RAVEN END
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	trigger = new idClipModel( idTraceModel( idBounds( tmin, tmax ) ) );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	trigger->Link( this, 255, GetPhysics()->GetOrigin(), mat3_identity );
// RAVEN END
	trigger->SetContents( CONTENTS_TRIGGER );
}

/*
==============
idPlat::Event_Touch
===============
*/
void idPlat::Event_Touch( idEntity *other, trace_t *trace ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !other->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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

	Use_BinaryMover( this );
	
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

idMover_Periodic::~idMover_Periodic( void ) {
	SetPhysics( NULL );
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
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteFloat( damage );
}

/*
===============
idMover_Periodic::Restore
===============
*/
void idMover_Periodic::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	savefile->ReadFloat( damage );

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
	if ( !blockingEntity->fl.takedamage ) {
		if ( blockingEntity->IsType( idAI::GetClassType() ) ) {
			//burning out already
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() )
					|| (blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() )) ) {
			//moveable
			blockingEntity->ProcessEvent( &EV_Remove );
			return;
		}
	} else {
		if ( blockingEntity->IsType( idAI::GetClassType() ) && blockingEntity->health <= 0 ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		} else if ( blockingEntity->IsType( idMoveable::GetClassType() ) ||  blockingEntity->IsType( idMoveableItem::GetClassType() ) ) {
			//damagable movable?
            blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
			return;
		} else if ( blockingEntity->IsType( idAFEntity_Base::GetClassType() ) && !blockingEntity->IsType( idActor::GetClassType() ) ) {
			if ( blockingEntity->spawnArgs.GetBool( "gib" ) ) {
                blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", 20, INVALID_JOINT );
				return;
			} else {
				blockingEntity->ProcessEvent( &EV_Remove );
				return;
			}
		}
	}
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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
void idRotater::Save( idSaveGame *savefile ) const {
	activatedBy.Save( savefile );
}

/*
===============
idRotater::Restore
===============
*/
void idRotater::Restore( idRestoreGame *savefile ) {
	activatedBy.Restore( savefile );
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

	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, physicsObj.GetAxis().ToAngles(), delta, ang_zero );
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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

		if( gameLocal.isMultiplayer ) {
			freq = 1 / ( idMath::TWO_PI ) * idMath::Sqrt( g_mp_gravity.GetFloat() / ( 3 * length ) );
		} else {
			freq = 1 / ( idMath::TWO_PI ) * idMath::Sqrt( g_gravity.GetFloat() / ( 3 * length ) );
		}
	}

	physicsObj.SetSelf( this );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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

		physicsObj.SetLinearExtrapolation( EXTRAPOLATION_LINEAR, gameLocal.time, time * 1000, physicsObj.GetOrigin(), delta, vec3_origin );
	}
}


/*
===============================================================================

rvConveyor

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvConveyor )
	EVENT( EV_FindTargets,	rvConveyor::Event_FindTargets )
END_CLASS

/*
================
rvConveyor::rvConveyor
================
*/
rvConveyor::rvConveyor ( void ) {
}

/*
================
rvConveyor::Spawn
================
*/
void rvConveyor::Spawn ( void ) {
	spawnArgs.GetFloat ( "speed", "100", moveSpeed );
	
	float angle;
	if ( spawnArgs.GetFloat ( "moveAngle", "0", angle ) ) {
		moveDir = idAngles ( 0, angle, 0 ).ToMat3()[0];
	} else { 
		moveDir = GetPhysics()->GetAxis()[0];
	}
	
	GetPhysics()->SetContents ( CONTENTS_SOLID );
	GetPhysics()->SetClipMask( MASK_SOLID );
	GetPhysics()->EnableClip ( );
	
	BecomeActive ( TH_THINK|TH_PHYSICS );
}

/*
================
rvConveyor::Think
================
*/
void rvConveyor::Think ( void ) {
	trace_t pushResults;	
	idVec3	newOrigin;
	idMat3	newAxis;
	idVec3	oldOrigin;
	idMat3	oldAxis;
	
	oldOrigin = GetPhysics()->GetOrigin ( );
	oldAxis   = GetPhysics()->GetAxis ( );
	
	newOrigin = GetPhysics()->GetOrigin() + moveDir * moveSpeed * MS2SEC ( gameLocal.GetMSec() );
	newAxis   = oldAxis;
	gameLocal.push.ClipPush( pushResults, this, 0, 
							 GetPhysics()->GetOrigin(), oldAxis, newOrigin, newAxis );

	GetPhysics()->SetOrigin ( oldOrigin );
	GetPhysics()->SetAxis ( oldAxis );
	
	idEntity::Think();	
}

/*
================
rvConveyor::Save
================
*/
void rvConveyor::Save( idSaveGame *savefile ) const
{
	savefile->WriteVec3( moveDir );
	savefile->WriteFloat( moveSpeed );
}

/*
================
rvConveyor::Restore
================
*/
void rvConveyor::Restore( idRestoreGame *savefile )
{
	savefile->ReadVec3( moveDir );
	savefile->ReadFloat( moveSpeed );
}

/*
================
rvConveyor::Event_FindTargets
================
*/
void rvConveyor::Event_FindTargets ( void ) {
	FindTargets ( );

	moveDir = GetPhysics()->GetAxis ( )[0];
	if ( !targets.Num ( ) || !targets[0] ) {
		return;
	}
	
	idEntity* path;
	idEntity* next;
	path = targets[0];
	path->FindTargets ( );
	next = path->targets.Num() ? path->targets[0].GetEntity() : NULL;
	if ( !next ) {
		return;
	}

	moveDir = next->GetPhysics()->GetOrigin() - path->GetPhysics()->GetOrigin();
	moveDir.Normalize ( );

	path->PostEventMS ( &EV_Remove, 0 );
}


CLASS_DECLARATION( idMover, rvPusher )
END_CLASS

/*
================
rvPusher::rvPusher
================
*/
rvPusher::rvPusher( void ) {
	parent = 0;
}

/*
================
rvPusher::~rvPusher
================
*/
rvPusher::~rvPusher( void ) {
}

/*
================
rvPusher::Spawn
================
*/
void rvPusher::Spawn( void ) {
	parent = 0;
}

/*
================
rvPusher::Think
================
*/
void rvPusher::Think( void ) {
	
	// Total hack, but it gets the job done
	BecomeActive( TH_ALL );
	
	if ( parent ) {
		idAnimator *parentAnimator = parent->GetAnimator();
		if ( parentAnimator ) {
		
			idStr jointName;
			if ( spawnArgs.GetString( "attachedBone", "", jointName ) ) {	
				bindJointHandle = parentAnimator->GetJointHandle( jointName );
				
				trace_t pushResults;	
				idVec3	oldOrigin;
				idMat3	oldAxis;
				
				oldOrigin = GetPhysics()->GetOrigin();
				oldAxis = GetPhysics()->GetAxis();

				parentAnimator->CreateFrame( gameLocal.time, true );
				parentAnimator->ServiceAnims( gameLocal.previousTime, gameLocal.time );
				parentAnimator->GetJointTransform( bindJointHandle, gameLocal.time, pusherOrigin, pusherAxis );
				pusherAxis *= parent->GetRenderEntity()->axis;
				pusherOrigin = parent->GetRenderEntity()->origin + pusherOrigin * parent->GetRenderEntity()->axis;
				MoveToPos( pusherOrigin );
				gameLocal.push.ClipTranslationalPush(pushResults, this, 0, pusherOrigin, pusherOrigin - oldOrigin );
				GetPhysics()->SetOrigin ( pusherOrigin );
			}

		}
	} else {
		idStr bindEntName;
		parent = 0;
		if ( spawnArgs.GetString( "attachedEntity", "", bindEntName ) ) {
			parent = gameLocal.FindEntity( bindEntName );
		}
	}

	idMover::Think();
}



// RAVEN END
