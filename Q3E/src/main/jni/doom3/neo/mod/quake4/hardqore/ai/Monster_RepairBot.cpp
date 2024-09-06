
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"

class repairBotArm_t {
public:
	jointHandle_t		joint;
	int					repairTime;
	bool				repairing;
	rvClientEffectPtr	effectRepair;
	rvClientEffectPtr	effectImpact;

    int					periodicEndTime;
	
						repairBotArm_t	() {
							periodicEndTime = -1;
						}
	void				Save			( idSaveGame* savefile ) const;
	void				Restore			( idRestoreGame* savefile );
};

class rvMonsterRepairBot : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterRepairBot );

	rvMonsterRepairBot ( void );

	void					InitSpawnArgsVariables( void );
	void					Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );

	virtual void			Think				( void );

	virtual void			GetDebugInfo		( debugInfoProc_t proc, void* userData );

protected:
	
	virtual bool			CheckActions		( void );
	virtual void			OnDeath				( void );

	int						repairEndTime;
	float					repairEffectDist;
	
	repairBotArm_t			armLeft;
	repairBotArm_t			armRight;
	
private:

	void					UpdateRepairs		( repairBotArm_t& arm );
	void					StopRepairs			( repairBotArm_t& arm );
	
	// Leg states
	stateResult_t		State_Legs_Move					( const stateParms_t& parms );
	stateResult_t		State_TorsoAction_Repair		( const stateParms_t& parms );
	stateResult_t		State_TorsoAction_RepairDone	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterRepairBot );
};

CLASS_DECLARATION( idAI, rvMonsterRepairBot )
END_CLASS

/*
================
rvMonsterRepairBot::rvMonsterRepairBot
================
*/
rvMonsterRepairBot::rvMonsterRepairBot ( ) {
}

void rvMonsterRepairBot::InitSpawnArgsVariables( void )
{
	armLeft.joint = animator.GetJointHandle ( spawnArgs.GetString ( "joint_arm_left", "l_fx" ) );
	armRight.joint = animator.GetJointHandle ( spawnArgs.GetString ( "joint_arm_right", "r_fx" ) );
	repairEffectDist = spawnArgs.GetFloat( "repairEffectDist", "64" );
}
/*
================
rvMonsterRepairBot::Spawn
================
*/
void rvMonsterRepairBot::Spawn ( void ) {
	InitSpawnArgsVariables();
}

/*
================
rvMonsterRepairBot::CheckActions
================
*/
bool rvMonsterRepairBot::CheckActions ( void ) {
	return false;
}

/*
================
rvMonsterRepairBot::OnDeath
================
*/
void rvMonsterRepairBot::OnDeath ( void ) {
	StopRepairs ( armLeft );
	StopRepairs ( armRight );
	gameLocal.PlayEffect( spawnArgs, "fx_death", GetPhysics()->GetOrigin(), viewAxis );
	idAI::OnDeath ( );
}

/*
================
rvMonsterRepairBot::Save
================
*/
void rvMonsterRepairBot::Save( idSaveGame *savefile ) const {
	savefile->WriteInt ( repairEndTime );

	armLeft.Save ( savefile );
	armRight.Save ( savefile );
}

/*
================
rvMonsterRepairBot::Restore
================
*/
void rvMonsterRepairBot::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt ( repairEndTime );

	armLeft.Restore ( savefile );
	armRight.Restore ( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterRepairBot::Think
================
*/
void rvMonsterRepairBot::Think ( void ) {
	idAI::Think ( );
	
	// Update repair effects? (dont worry about stopping them, the state ending will do so)
	UpdateRepairs ( armLeft );
	UpdateRepairs ( armRight );
}

/*
================
rvMonsterRepairBot::UpdateRepairs
================
*/
void rvMonsterRepairBot::UpdateRepairs ( repairBotArm_t& arm ) {
	trace_t tr;
	idVec3	origin;
	idMat3	axis;

	if ( arm.joint == INVALID_JOINT ) {
		return;
	}

	if ( gameLocal.time > repairEndTime ) {
		StopRepairs ( arm ) ;
		return;
	}

	// If the repair time has been crossed we need to start/stop the repairs
	if ( gameLocal.time > arm.repairTime ) {
		if ( arm.repairing ) {
			StopRepairs ( arm );
			arm.repairTime = gameLocal.time + gameLocal.random.RandomInt ( 500 );
			arm.repairing  = false;
		} else {
			arm.repairTime = gameLocal.time + gameLocal.random.RandomInt ( 2500 );
			arm.repairing  = true;
		}				
	} 
	
	if ( !arm.repairing ) {
		return;
	}
			
	// Left repair effect
	GetJointWorldTransform ( arm.joint, gameLocal.time, origin, axis );
	gameLocal.TracePoint ( this, tr, origin, origin + axis[0] * repairEffectDist, MASK_SHOT_RENDERMODEL, this );

	if ( tr.fraction >= 1.0f ) {		
		StopRepairs ( arm );
	} else {
		// Start the repair effect if not already started
		if ( !arm.effectRepair ) {
			arm.effectRepair = PlayEffect ( "fx_repair", arm.joint, true );
		}
		// If the repair effect is running then set its end origin
		if ( arm.effectRepair ) {	
			arm.effectRepair->SetEndOrigin ( tr.endpos );
		}
		// Start the impact effect
		if ( !arm.effectImpact ) {
			arm.effectImpact = PlayEffect ( "fx_repair_impact", tr.endpos, tr.c.normal.ToMat3(), true );

		} else {
			// Calculate the local origin and axis from the given globals
			idVec3 localOrigin = (tr.endpos - renderEntity.origin) * renderEntity.axis.Transpose ( );
			idMat3 localAxis   = tr.c.normal.ToMat3 ();// * renderEntity.axis.Transpose();
			arm.effectImpact->SetOrigin ( localOrigin );
			arm.effectImpact->SetAxis ( localAxis );
		}

		if( gameLocal.GetTime() > arm.periodicEndTime ) {// FIXME: Seems dumb to keep banging on this if the fx isn't defined.
			gameLocal.PlayEffect( spawnArgs, "fx_repair_impact_periodic", tr.endpos, tr.c.normal.ToMat3() );
			arm.periodicEndTime = gameLocal.GetTime() + SEC2MS( rvRandom::flrand(spawnArgs.GetVec2("impact_fx_delay_range", "1 1")) );
		}
	}
}

/*
================
rvMonsterRepairBot::StopRepairs
================
*/
void rvMonsterRepairBot::StopRepairs ( repairBotArm_t& arm ) {
	if ( arm.effectImpact ) {
		arm.effectImpact->Stop ( );
		arm.effectImpact = NULL;
	}
	if ( arm.effectRepair ) {
		arm.effectRepair->Stop ( );
		arm.effectRepair = NULL;
	}

	arm.periodicEndTime = -1;
}

/*
================
rvMonsterRepairBot::GetDebugInfo
================
*/
void rvMonsterRepairBot::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterRepairBot )
	STATE ( "Legs_Move",				rvMonsterRepairBot::State_Legs_Move )
	STATE ( "TorsoAction_Repair",		rvMonsterRepairBot::State_TorsoAction_Repair )
	STATE ( "TorsoAction_RepairDone",	rvMonsterRepairBot::State_TorsoAction_RepairDone )
END_CLASS_STATES

/*
================
rvMonsterRepairBot::State_Legs_Move
================
*/
stateResult_t rvMonsterRepairBot::State_Legs_Move ( const stateParms_t& parms ) {
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_MOVE,
		STAGE_MOVE_WAIT,
		STAGE_STOP,
		STAGE_STOP_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_START:
			PlayAnim ( ANIMCHANNEL_LEGS, "idle_to_run", 4 );
			return SRESULT_STAGE ( STAGE_START_WAIT );
		
		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				return SRESULT_STAGE ( STAGE_MOVE );
			}
			return SRESULT_WAIT;
			
		case STAGE_MOVE:
			PlayCycle (  ANIMCHANNEL_LEGS, "run", 4 );
			return SRESULT_STAGE ( STAGE_MOVE_WAIT );
		
		case STAGE_MOVE_WAIT:
			if ( !move.fl.moving || !CanMove() ) {
				return SRESULT_STAGE ( STAGE_STOP );
			}
			return SRESULT_WAIT;
				
		case STAGE_STOP:
			PlayAnim ( ANIMCHANNEL_LEGS, "run_to_idle", 4 );
			return SRESULT_STAGE ( STAGE_STOP_WAIT );
		
		case STAGE_STOP_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterRepairBot::State_TorsoAction_Repair
================
*/
stateResult_t rvMonsterRepairBot::State_TorsoAction_Repair ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_REPAIR,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayCycle ( ANIMCHANNEL_TORSO, "repair", 4 );
			repairEndTime = gameLocal.time + SEC2MS(scriptedActionEnt->spawnArgs.GetInt ( "duration", "5" ) );
			PostAnimState ( ANIMCHANNEL_TORSO, "TorsoAction_RepairDone", 0, 0, SFLAG_ONCLEAR );
			return SRESULT_STAGE(STAGE_REPAIR);
			
		case STAGE_REPAIR:
			if ( gameLocal.time > repairEndTime ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterRepairBot::State_TorsoAction_RepairDone
================
*/
stateResult_t rvMonsterRepairBot::State_TorsoAction_RepairDone ( const stateParms_t& parms ) {
	StopRepairs ( armLeft );
	StopRepairs ( armRight );
	
	return SRESULT_DONE;
}

/*
================
repairBotArm_t::Save
================
*/
void repairBotArm_t::Save ( idSaveGame* savefile ) const {
	savefile->WriteInt ( repairTime );
	savefile->WriteBool ( repairing );
	effectRepair.Save ( savefile );
	effectImpact.Save ( savefile );
	
    savefile->WriteInt ( periodicEndTime );
}

/*
================
repairBotArm_t::Restore
================
*/
void repairBotArm_t::Restore ( idRestoreGame* savefile ) {
	savefile->ReadInt ( repairTime );
	savefile->ReadBool ( repairing );
	effectRepair.Restore ( savefile );
	effectImpact.Restore ( savefile );

    savefile->ReadInt ( periodicEndTime );
}
