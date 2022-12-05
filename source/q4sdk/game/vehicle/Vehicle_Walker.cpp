#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleAnimated.h"

class rvVehicleWalker : public rvVehicleAnimated {
public:

	CLASS_PROTOTYPE( rvVehicleWalker );

	rvVehicleWalker ( void );
	
	void				Think					( void );
	void				Spawn					( void );
	void				Save					( idSaveGame *savefile ) const;
	void				Restore					( idRestoreGame *savefile );

	virtual void		UpdateState				( void );
	virtual void		SetInput				( int position, const usercmd_t& cmd, const idAngles& newAngles );

	const char*			stopAnimName;

	virtual bool		FindClearExitPoint		( int pos, idVec3& origin, idMat3& axis ) const;

private:
	void				HandleStrafing			( void );


	stateResult_t		Frame_ForwardRight		( int );
	stateResult_t		Frame_ForwardLeft		( int );
	stateResult_t		Frame_BackwardRight		( int );
	stateResult_t		Frame_BackwardLeft		( int );

	stateResult_t		State_Wait_OnlineAnim	( const stateParms_t& parms );

	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_IdleThink			( const stateParms_t& parms );
	stateResult_t		State_IdleOffline		( const stateParms_t& parms );
	stateResult_t		State_Offline			( const stateParms_t& parms );
	stateResult_t		State_Online			( const stateParms_t& parms );

	stateResult_t		State_ForwardStart		( const stateParms_t& parms );
	stateResult_t		State_Forward			( const stateParms_t& parms );
	stateResult_t		State_BackwardStart		( const stateParms_t& parms );
	stateResult_t		State_Backward			( const stateParms_t& parms );
	stateResult_t		State_Stop				( const stateParms_t& parms );
	stateResult_t		State_Turn				( const stateParms_t& parms );
	stateResult_t		State_TurnThink			( const stateParms_t& parms );
	stateResult_t		State_ScriptedAnim		( const stateParms_t& parms );

	void				Event_ScriptedAnim		( const char* animname, int blendFrames, bool loop, bool endWithIdle );
	void 				Event_ScriptedDone		( void );
	void				Event_ScriptedStop		( void );

	CLASS_STATES_PROTOTYPE ( rvVehicleWalker );
};

CLASS_DECLARATION( rvVehicleAnimated, rvVehicleWalker )
	EVENT( AI_ScriptedAnim,	rvVehicleWalker::Event_ScriptedAnim )
	EVENT( AI_ScriptedDone,	rvVehicleWalker::Event_ScriptedDone )
	EVENT( AI_ScriptedStop,	rvVehicleWalker::Event_ScriptedStop )
END_CLASS

/*
================
rvVehicleWalker::rvVehicleWalker
================
*/
rvVehicleWalker::rvVehicleWalker ( void ) {
	stopAnimName = "";
}

/*
================
rvVehicleWalker::Think
================
*/
void rvVehicleWalker::Think ( void ) {
	rvVehicleAnimated::Think();

	if ( !HasDrivers() || IsStalled() ) {
		return;
	}

	idVec3 delta;
	animator.GetDelta( gameLocal.time - gameLocal.GetMSec(), gameLocal.time, delta );

	if ( delta.LengthSqr() > 0.1f ) {
		gameLocal.RadiusDamage( GetOrigin(), this, this, this, this, spawnArgs.GetString( "def_stompDamage", "damage_Smallexplosion" ) );
	}
}

/*
================
rvVehicleWalker::Spawn
================
*/
void rvVehicleWalker::Spawn	( void ) {
	SetAnimState ( ANIMCHANNEL_LEGS, "State_IdleOffline", 0 );
}

/*
================
rvVehicleWalker::Save
================
*/
void rvVehicleWalker::Save ( idSaveGame *savefile ) const {
	savefile->WriteString( stopAnimName );
}

/*
================
rvVehicleWalker::Restore
================
*/
void rvVehicleWalker::Restore ( idRestoreGame *savefile ) {
	//twhitaker: I just happened to see this, while going through this code which I originally wrote (at 3am or so).
	//TODO: fix this.  Make stopAnimName an idStr?
	idStr str;
	savefile->ReadString( str );
	stopAnimName = str;
}

/*
================
rvVehicleWalker::UpdateState
================
*/
void rvVehicleWalker::UpdateState ( void ) {
	rvVehiclePosition& pos = positions[0];
	usercmd_t& cmd	= pos.mInputCmd;

	vfl.driver		= pos.IsOccupied();
  	vfl.forward		= (vfl.driver && cmd.forwardmove > 0);
  	vfl.backward	= (vfl.driver && cmd.forwardmove < 0);
  	vfl.right		= (vfl.driver && cmd.rightmove < 0);
  	vfl.left		= (vfl.driver && cmd.rightmove > 0);	
	vfl.strafe		= (vfl.driver && cmd.buttons & BUTTON_STRAFE );

	if ( g_vehicleMode.GetInteger() != 0 ) {
		vfl.strafe = !vfl.strafe;
	}
}

/*
================
rvVehicleWalker::SetInput
================
*/
void rvVehicleWalker::SetInput ( int position, const usercmd_t& cmd, const idAngles& newAngles ) {
	usercmd_t* pcmd = const_cast<usercmd_t*>( &cmd );
	pcmd->rightmove *= -1;
	GetPosition(position)->SetInput ( cmd, newAngles );
}

/*
================
rvVehicleWalker::HandleStrafing
================
*/
void rvVehicleWalker::HandleStrafing ( void ) {
	if ( vfl.right ) {
		additionalDelta -= spawnArgs.GetVector( "strafe_delta", "0 0 1.2" );
	}
	if ( vfl.left ) {
		additionalDelta += spawnArgs.GetVector( "strafe_delta", "0 0 1.2" );
	}
}

// mekberg: overloaded this because physics bounds code is significantly different
/*
=====================
rvVehicleWalker::FindClearExitPoint
=====================
*/
// FIXME: this whole function could be cleaned up
bool rvVehicleWalker::FindClearExitPoint( int pos, idVec3& origin, idMat3& axis ) const {
	trace_t		trace;
	const rvVehiclePosition*	position = GetPosition( pos );
	idActor*	driver = position->GetDriver();
	idVec3		end;
	idVec3		traceOffsetPoints[4];
	const float error = 1.1f;

	origin.Zero();
	axis.Identity();

	idMat3 driverAxis = driver->viewAxis;
	idVec3 driverOrigin = driver->GetPhysics()->GetOrigin();

	idMat3 vehicleAxis = position->GetEyeAxis();
	idVec3 vehicleOrigin = GetPhysics()->GetOrigin();

	idBounds driverBounds( driver->GetPhysics()->GetBounds() );
	idBounds vehicleBounds( GetPhysics()->GetBounds() );
	idBounds driverAbsBounds;
	idBounds vehicleAbsBounds;

	vehicleAbsBounds.FromTransformedBounds( vehicleBounds, vehicleOrigin, GetPhysics()->GetAxis() );
	if( position->fl.driverVisible ) {
		// May want to do this even if the driver isn't visible
		if( position->mExitPosOffset.LengthSqr() > VECTOR_EPSILON ) {
			axis = GetPhysics()->GetAxis() * position->mExitAxisOffset;
			origin = vehicleOrigin + position->mExitPosOffset * axis;
		} else {
			origin = driverOrigin;
			axis = (driver->IsBoundTo(this)) ? vehicleAxis : driverAxis;
		}
		return true;
	}

	// Build list
	// FIXME: try and find a cleaner way to do this
	traceOffsetPoints[ 0 ] = vehicleBounds.FindVectorToEdge( vehicleAxis[ 1 ] ) - driverBounds.FindVectorToEdge( -vehicleAxis[ 1 ] );
	traceOffsetPoints[ 1 ] = vehicleBounds.FindVectorToEdge( -vehicleAxis[ 1 ] ) - driverBounds.FindVectorToEdge( vehicleAxis[ 1 ] );
	traceOffsetPoints[ 2 ] = vehicleBounds.FindVectorToEdge( vehicleAxis[ 0 ] ) - driverBounds.FindVectorToEdge( -vehicleAxis[ 0 ] );
	traceOffsetPoints[ 3 ] = vehicleBounds.FindVectorToEdge( -vehicleAxis[ 0 ] ) - driverBounds.FindVectorToEdge( vehicleAxis[ 0 ] );

	for( int ix = 0; ix < 4; ++ix ) {
		//Try all four sides and on top if need be
		end = vehicleOrigin + traceOffsetPoints[ ix ] * error;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		gameLocal.Translation( this, trace, vehicleOrigin, end, driver->GetPhysics()->GetClipModel(), driverAxis, driver->GetPhysics()->GetClipMask(), this, driver );
// RAVEN END
		driverAbsBounds.FromTransformedBounds( driverBounds, trace.endpos, driverAxis );
		if( trace.fraction > 0.0f && !driverAbsBounds.IntersectsBounds(vehicleAbsBounds) ) {
			origin = trace.endpos;
			axis = vehicleAxis;
			return true;
		}
	}

	return false;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvVehicleWalker )
	STATE ( "ForwardLeft",			rvVehicleWalker::Frame_ForwardLeft )
	STATE ( "ForwardRight",			rvVehicleWalker::Frame_ForwardRight )
	STATE ( "BackwardLeft",			rvVehicleWalker::Frame_BackwardLeft )
	STATE ( "BackwardRight",		rvVehicleWalker::Frame_BackwardRight )

	STATE ( "Wait_OnlineAnim",		rvVehicleWalker::State_Wait_OnlineAnim )
	
	STATE ( "State_Idle",			rvVehicleWalker::State_Idle )
	STATE ( "State_IdleThink",		rvVehicleWalker::State_IdleThink )
	STATE ( "State_IdleOffline",	rvVehicleWalker::State_IdleOffline )
	STATE ( "State_Offline",		rvVehicleWalker::State_Offline )
	STATE ( "State_Online",			rvVehicleWalker::State_Online )

	STATE ( "State_ForwardStart",	rvVehicleWalker::State_ForwardStart )
	STATE ( "State_Forward",		rvVehicleWalker::State_Forward )
	STATE ( "State_BackwardStart",	rvVehicleWalker::State_BackwardStart )
	STATE ( "State_Backward",		rvVehicleWalker::State_Backward )
	STATE ( "State_Stop",			rvVehicleWalker::State_Stop )
	STATE ( "State_Turn",			rvVehicleWalker::State_Turn )	
	STATE ( "State_TurnThink",		rvVehicleWalker::State_TurnThink )
	STATE ( "State_ScriptedAnim",	rvVehicleWalker::State_ScriptedAnim )
END_CLASS_STATES

/*
================
rvVehicleWalker::State_IdleOffline
================
*/
stateResult_t rvVehicleWalker::State_IdleOffline ( const stateParms_t& parms ) {
	vfl.frozen = true;

	PlayCycle ( ANIMCHANNEL_LEGS, "idle_offline", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_Driver", 2 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Online", 2 );
	
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_Online
================
*/
stateResult_t rvVehicleWalker::State_Online ( const stateParms_t& parms ) {	
	vfl.frozen = false;
	
	PlayAnim ( ANIMCHANNEL_LEGS, "start", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_OnlineAnim", 4 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Idle", 4 );

	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_Offline
================
*/
stateResult_t rvVehicleWalker::State_Offline ( const stateParms_t& parms ) {	
	PlayAnim ( ANIMCHANNEL_LEGS, "stop", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_TorsoAnim", 4 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_IdleOffline", 4 );
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_Idle
================
*/
stateResult_t rvVehicleWalker::State_Idle ( const stateParms_t& parms ) {	
	if ( SRESULT_WAIT != State_IdleThink ( parms ) ) {
		return SRESULT_DONE;
	}
	
	PlayCycle( ANIMCHANNEL_LEGS, "idle", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_IdleThink", 2 );
	
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_Idle
================
*/
stateResult_t rvVehicleWalker::State_IdleThink ( const stateParms_t& parms ) { 
	if ( !vfl.driver || vfl.stalled ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "State_Offline", parms.blendFrames );
		return SRESULT_DONE;
	}

	if ( IsMovementEnabled ( ) ) {
		if ( vfl.forward ) {
			PostAnimState ( ANIMCHANNEL_LEGS, "State_ForwardStart", 2 );
			return SRESULT_DONE;
		}
		
		if ( vfl.backward ) {
			PostAnimState ( ANIMCHANNEL_LEGS, "State_BackwardStart", 2 );
			return SRESULT_DONE;
		}
		
		if ( vfl.right || vfl.left ) {
			PostAnimState ( ANIMCHANNEL_LEGS, "State_Turn", 2 );
			return SRESULT_DONE;
		}
	}
	
	return SRESULT_WAIT;
}

/*
================
rvVehicleWalker::State_ForwardStart
================
*/
stateResult_t rvVehicleWalker::State_ForwardStart ( const stateParms_t& parms ) {
	PlayAnim ( ANIMCHANNEL_LEGS, "forward_start", 2 );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_TorsoAnim", 2 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Forward", 2 );
	return SRESULT_DONE;
}

stateResult_t rvVehicleWalker::State_Forward ( const stateParms_t& parms ) {
	// If not moving anymore by the time we get here just play the stop anim
	if ( !vfl.forward ) {
		stopAnimName = "forward_stop_leftmid";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 4 );
		return SRESULT_DONE;
	}

	if ( !parms.stage ) {
		PlayCycle( ANIMCHANNEL_LEGS, "forward", 2 );
		return SRESULT_STAGE(parms.stage + 1);
	}

	if ( AnimDone( ANIMCHANNEL_LEGS, 2 ) ) {
		return SRESULT_DONE;
	}

	HandleStrafing();
	return SRESULT_WAIT;
}

/*
================
rvVehicleWalker::State_BackwardStart
================
*/
stateResult_t rvVehicleWalker::State_BackwardStart ( const stateParms_t& parms ) {
	PlayAnim ( ANIMCHANNEL_LEGS, "backward_start", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_TorsoAnim", 2 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Backward", 2 );
	return SRESULT_DONE;
}

stateResult_t rvVehicleWalker::State_Backward ( const stateParms_t& parms ) {
	// If not moving anymore by the time we get here just play the stop anim
	if ( !vfl.backward ) {
		stopAnimName = "backward_stop_leftmid";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 2 );
		return SRESULT_DONE;
	}
		
	if ( !parms.stage ) {
	PlayCycle( ANIMCHANNEL_LEGS, "backward", 2 );
		return SRESULT_STAGE(parms.stage + 1);
	}

	if ( AnimDone( ANIMCHANNEL_LEGS, 2 ) ) {
		return SRESULT_DONE;
	}

	HandleStrafing();
	return SRESULT_WAIT;
}

/*
================
rvVehicleWalker::State_Stop
================
*/
stateResult_t rvVehicleWalker::State_Stop ( const stateParms_t& parms ) {	
	PlayAnim ( ANIMCHANNEL_LEGS, stopAnimName, parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_TorsoAnim", 2 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Idle", 2 );
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_Turn
================
*/
stateResult_t rvVehicleWalker::State_Turn ( const stateParms_t& parms ) {
	if ( vfl.left ) {
		PlayAnim ( ANIMCHANNEL_LEGS, "turn_left", parms.blendFrames );
	} else if ( vfl.right ) {
		PlayAnim ( ANIMCHANNEL_LEGS, "turn_right", parms.blendFrames );
	} else {
		PostAnimState ( ANIMCHANNEL_LEGS, "State_Idle", parms.blendFrames );
		return SRESULT_DONE;
	}
	
	PostAnimState ( ANIMCHANNEL_LEGS, "State_TurnThink", 16 );
	
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::State_TurnThink
================
*/
stateResult_t rvVehicleWalker::State_TurnThink ( const stateParms_t& parms ) {
	// If moving again bail on the turn, reguardless of whether its in mid animation
	if ( vfl.forward || vfl.backward ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "State_Idle", parms.blendFrames );		
		return SRESULT_DONE;
	}
	// If the animation is done then repeat
	if ( AnimDone ( ANIMCHANNEL_LEGS, 8 ) ){
		PostAnimState ( ANIMCHANNEL_LEGS, "State_Turn", 8 );
		return SRESULT_DONE;
	}

	HandleStrafing();

	return SRESULT_WAIT;
}

/*
================
rvVehicleWalker::Frame_ForwardLeft
================
*/
stateResult_t rvVehicleWalker::Frame_ForwardLeft ( int ) {
	if ( !vfl.forward ) {
		stopAnimName = "forward_stop_left";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 2 );
	}
	return SRESULT_OK;
}

/*
================
rvVehicleWalker::Frame_ForwardRight
================
*/
stateResult_t rvVehicleWalker::Frame_ForwardRight ( int ) {
	if ( !vfl.forward ) {
		stopAnimName = "forward_stop_right";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 2 );
	}
	return SRESULT_OK;
}


/*
================
rvVehicleWalker::Frame_BackwardLeft
================
*/
stateResult_t rvVehicleWalker::Frame_BackwardLeft ( int ) {
	if ( !vfl.backward ) {
		stopAnimName = "backward_stop_left";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 2 );
	}
	return SRESULT_OK;
}

/*
================
rvVehicleWalker::Frame_BackwardRight
================
*/
stateResult_t rvVehicleWalker::Frame_BackwardRight ( int ) {
	if ( !vfl.backward ) {
		stopAnimName = "backward_stop_right";
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Stop", 2 );
	}
	return SRESULT_OK;
}

/*
================
rvVehicleWalker::State_Wait_OnlineAnim
================
*/
stateResult_t rvVehicleWalker::State_Wait_OnlineAnim ( const stateParms_t& parms ) {	
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) && vfl.driver ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}	

/*
================
rvVehicleWalker::State_ScriptedAnim
================
*/
stateResult_t rvVehicleWalker::State_ScriptedAnim ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
	Event_ScriptedStop();
	return SRESULT_DONE;
}

/*
================
rvVehicleWalker::Event_ScriptedAnim
================
*/
void rvVehicleWalker::Event_ScriptedAnim( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
	vfl.endWithIdle = endWithIdle;
	if ( loop ) {
		PlayCycle ( ANIMCHANNEL_LEGS, animname, blendFrames );
	} else {
		PlayAnim ( ANIMCHANNEL_LEGS, animname, blendFrames );
	}
	SetAnimState ( ANIMCHANNEL_LEGS, "State_ScriptedAnim", blendFrames );
	vfl.scripted = true;
}

/*
================
rvVehicleWalker::Event_ScriptedDone
================
*/
void rvVehicleWalker::Event_ScriptedDone( void ) {
	idThread::ReturnInt( !vfl.scripted );
}

/*
================
rvVehicleWalker::Event_ScriptedStop
================
*/
void rvVehicleWalker::Event_ScriptedStop( void ) {
	vfl.scripted = false;

	if ( vfl.endWithIdle ) {
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Idle", 2 );
	}
}
