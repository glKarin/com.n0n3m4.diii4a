#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleStatic.h"

class rvVehicleDropPod : public rvVehicleStatic {
public:

	CLASS_PROTOTYPE( rvVehicleDropPod );

	rvVehicleDropPod ( void );
	
	void				Spawn					( void );

	virtual bool		GetPhysicsToVisualTransform ( idVec3 &origin, idMat3 &axis );

private:

	stateResult_t		State_Idle				( const stateParms_t& parms );
	stateResult_t		State_IdleThink			( const stateParms_t& parms );
	stateResult_t		State_IdleOffline		( const stateParms_t& parms );
	stateResult_t		State_ScriptedAnim		( const stateParms_t& parms );

	void				Event_ScriptedAnim		( const char* animname, int blendFrames, bool loop, bool endWithIdle );
	void 				Event_ScriptedDone		( void );
	void				Event_ScriptedStop		( void );

	CLASS_STATES_PROTOTYPE ( rvVehicleDropPod );
};

CLASS_DECLARATION( rvVehicleStatic, rvVehicleDropPod )
	EVENT( AI_ScriptedAnim,	rvVehicleDropPod::Event_ScriptedAnim )
	EVENT( AI_ScriptedDone,	rvVehicleDropPod::Event_ScriptedDone )
	EVENT( AI_ScriptedStop,	rvVehicleDropPod::Event_ScriptedStop )
END_CLASS

/*
================
rvVehicleDropPod::rvVehicleDropPod
================
*/
rvVehicleDropPod::rvVehicleDropPod ( void ) {
}

/*
================
rvVehicleDropPod::Spawn
================
*/
void rvVehicleDropPod::Spawn	( void ) {
	SetAnimState ( ANIMCHANNEL_LEGS, "State_IdleOffline", 0 );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvVehicleDropPod )
	STATE ( "State_Idle",			rvVehicleDropPod::State_Idle )
	STATE ( "State_IdleThink",		rvVehicleDropPod::State_IdleThink )
	STATE ( "State_IdleOffline",	rvVehicleDropPod::State_IdleOffline )

	STATE ( "State_ScriptedAnim",	rvVehicleDropPod::State_ScriptedAnim )
END_CLASS_STATES

/*
================
rvVehicleDropPod::State_IdleOffline
================
*/
stateResult_t rvVehicleDropPod::State_IdleOffline ( const stateParms_t& parms ) {
	vfl.frozen = true;

	PlayCycle ( ANIMCHANNEL_LEGS, "idle", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_Driver", 0 );
	PostAnimState ( ANIMCHANNEL_LEGS, "State_Idle", 0 );
	
	return SRESULT_DONE;
}

/*
================
rvVehicleDropPod::State_Idle
================
*/
stateResult_t rvVehicleDropPod::State_Idle ( const stateParms_t& parms ) {	
	if ( SRESULT_WAIT != State_IdleThink ( parms ) ) {
		return SRESULT_DONE;
	}
	
	return SRESULT_DONE;
}

/*
================
rvVehicleDropPod::State_IdleThink
================
*/
stateResult_t rvVehicleDropPod::State_IdleThink ( const stateParms_t& parms ) { 
	if ( !vfl.driver ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "State_IdleOffline", parms.blendFrames );
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

/*
================
rvVehicleDropPod::State_ScriptedAnim
================
*/
stateResult_t rvVehicleDropPod::State_ScriptedAnim ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
	Event_ScriptedStop();
	return SRESULT_DONE;
}

/*
================
rvVehicleDropPod::Event_ScriptedAnim
================
*/
void rvVehicleDropPod::Event_ScriptedAnim( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
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
rvVehicleDropPod::Event_ScriptedDone
================
*/
void rvVehicleDropPod::Event_ScriptedDone( void ) {
	idThread::ReturnInt( !vfl.scripted );
}

/*
================
rvVehicleDropPod::Event_ScriptedStop
================
*/
void rvVehicleDropPod::Event_ScriptedStop( void ) {
	vfl.scripted = false;

	if ( vfl.endWithIdle ) {
		SetAnimState ( ANIMCHANNEL_LEGS, "State_Idle", 1 );
	}
}

/*
================
rvVehicleDropPod::GetPhysicsToVisualTransform
================
*/
bool rvVehicleDropPod::GetPhysicsToVisualTransform ( idVec3 &origin, idMat3 &axis ) {
//	if ( GetBindMaster() ) {
//		axis = viewAxis;
//		origin.Zero();
//		return true;
//	}
	return false;
}

