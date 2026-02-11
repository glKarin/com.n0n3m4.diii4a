/*
================
rvMonsterHarvesterDispersal.cpp

AI for the Harvester on dispersal
================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterHarvesterDispersal : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterHarvesterDispersal );

	rvMonsterHarvesterDispersal ( void );
	
	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	bool				CanTurn							( void ) const;
	virtual bool		SkipImpulse						( idEntity* ent, int id );

protected:

	rvAIAction			actionSprayScream;

	virtual bool		CheckActions			( void );
	virtual int			FilterTactical			( int availableTactical );

	int					maxShots;	
	int					minShots;
	int					shots;

	int					nextTurnTime;
	int					sweepCount;

private:

// Custom actions
	virtual bool		CheckAction_SprayScream( rvAIAction* action, int animNum );
	virtual bool		CheckAction_RangedAttack( rvAIAction* action, int animNum );

	// Torso States
	stateResult_t		State_Torso_RangedAttack( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnRight90	( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90	( const stateParms_t& parms );
	stateResult_t		State_SprayScream		( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterHarvesterDispersal );
};

CLASS_DECLARATION( idAI, rvMonsterHarvesterDispersal )
END_CLASS

/*
================
rvMonsterHarvesterDispersal::rvMonsterHarvesterDispersal
================
*/
rvMonsterHarvesterDispersal::rvMonsterHarvesterDispersal ( void ) {
}

void rvMonsterHarvesterDispersal::InitSpawnArgsVariables( void )
{
	maxShots = spawnArgs.GetInt ( "maxShots", "1" );
	minShots = spawnArgs.GetInt ( "minShots", "1" );
}
/*
================
rvMonsterHarvesterDispersal::Spawn
================
*/
void rvMonsterHarvesterDispersal::Spawn ( void ) {
	actionSprayScream.Init ( spawnArgs, "action_sprayScream", "State_SprayScream", AIACTIONF_ATTACK );
	
	InitSpawnArgsVariables();
	shots	 = 0;
}

/*
================
rvMonsterHarvesterDispersal::Save
================
*/
void rvMonsterHarvesterDispersal::Save ( idSaveGame *savefile ) const {
	actionSprayScream.Save( savefile );
	savefile->WriteInt( nextTurnTime );
	savefile->WriteInt( sweepCount );
	savefile->WriteInt ( shots );
}

/*
================
rvMonsterHarvesterDispersal::Restore
================
*/
void rvMonsterHarvesterDispersal::Restore ( idRestoreGame *savefile ) {
	actionSprayScream.Restore( savefile );
	savefile->ReadInt( nextTurnTime );
	savefile->ReadInt( sweepCount );
	savefile->ReadInt ( shots );

	InitSpawnArgsVariables();
}

/*
=====================
rvMonsterHarvesterDispersal::SkipImpulse
=====================
*/
bool rvMonsterHarvesterDispersal::SkipImpulse( idEntity* ent, int id ) {	
	return true;
}

/*
================
rvMonsterHarvesterDispersal::CanTurn
================
*/
bool rvMonsterHarvesterDispersal::CanTurn ( void ) const {
	return false;
	/*
	if ( !idAI::CanTurn ( ) ) {
		return false;
	}
	return (move.anim_turn_angles != 0.0f || move.fl.moving);
	*/
}

/*
================
rvMonsterHarvesterDispersal::CheckAction_RangedAttack
================
*/
bool rvMonsterHarvesterDispersal::CheckAction_RangedAttack ( rvAIAction* action, int animNum ) {
	return ( enemy.ent && idAI::CheckAction_RangedAttack( action, animNum ) && IsEnemyRecentlyVisible( ) );
}

/*
================
rvMonsterHarvesterDispersal::CheckAction_SprayScream
================
*/
bool rvMonsterHarvesterDispersal::CheckAction_SprayScream ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	return (!IsEnemyRecentlyVisible());
}

/*
================
rvMonsterHarvesterDispersal::CheckActions
================
*/
bool rvMonsterHarvesterDispersal::CheckActions ( void ) {

	// If not moving, try turning in place
	/*
	if ( !move.fl.moving && gameLocal.time > nextTurnTime ) {
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.8f || (turnYaw > 0 && GetEnemy() && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.8f || (turnYaw < 0 && GetEnemy() && !enemy.fl.inFov) ) {
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		}
	}
	*/

 	if ( CheckPainActions ( ) ) {
 		return true;
	}

	if ( idAI::CheckActions() ) {
		return true;
	}

	if ( PerformAction ( &actionSprayScream,  (checkAction_t)&rvMonsterHarvesterDispersal::CheckAction_SprayScream ) ) {
		return true;
	}

	return false;
}

/*
================
rvMonsterHarvesterDispersal::FilterTactical
================
*/
int rvMonsterHarvesterDispersal::FilterTactical ( int availableTactical ) {
	return availableTactical & (AITACTICAL_TURRET_BIT);
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterHarvesterDispersal )
	STATE ( "Torso_RangedAttack",	rvMonsterHarvesterDispersal::State_Torso_RangedAttack )
	STATE ( "Torso_TurnRight90",	rvMonsterHarvesterDispersal::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",		rvMonsterHarvesterDispersal::State_Torso_TurnLeft90 )
	STATE ( "State_SprayScream",	rvMonsterHarvesterDispersal::State_SprayScream )
END_CLASS_STATES

/*
================
rvMonsterHarvesterDispersal::State_Torso_RangedAttack
================
*/
stateResult_t rvMonsterHarvesterDispersal::State_Torso_RangedAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT: 
			if ( !enemy.ent ) {
				return SRESULT_DONE;	
			}
			shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_ATTACK );
		
		case STAGE_ATTACK: 
			if ( !enemy.ent )
			{
				return SRESULT_DONE;	
			}
			if ( !move.fl.moving )
			{
				DisableAnimState( ANIMCHANNEL_LEGS );
			}
			PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
			
		case STAGE_ATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				shots--;
				if ( GetEnemy() && !enemy.fl.inFov )
				{//just stop
				}
				else if ( shots > 0 || !GetEnemy() )
				{
					return SRESULT_STAGE ( STAGE_ATTACK );
				}
//				animator.ClearAllJoints ( );
//				leftChainOut = false;
//				rightChainOut = false;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterHarvesterDispersal::State_Torso_TurnRight90
================
*/
stateResult_t rvMonsterHarvesterDispersal::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_90_rt", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 ) || !strstr( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_90_rt" ) ) {
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}

			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterHarvesterDispersal::State_Torso_TurnLeft90
================
*/
stateResult_t rvMonsterHarvesterDispersal::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_90_lt", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 ) || !strstr( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "turn_90_lt" ) ) {
				AnimTurn ( 0, true );
				nextTurnTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}

			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterHarvesterDispersal::State_Torso_SprayAttack
================
*/
stateResult_t rvMonsterHarvesterDispersal::State_SprayScream ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_SWEEP,
		STAGE_END,
		STAGE_FINISH,
		STAGE_SCREAM,
		STAGE_FINISH_SCREAM
	};
	if ( parms.stage < STAGE_SCREAM && IsEnemyRecentlyVisible() ) {
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle" );
		return SRESULT_DONE;
	}
	switch ( parms.stage ) {
		case STAGE_START:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			sweepCount = 0;
			lookTarget = GetEnemy();
			PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_start", 0 );
			return SRESULT_STAGE ( STAGE_SWEEP );
		
		case STAGE_SWEEP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				sweepCount++;
				PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_loop", 0 );
				return SRESULT_STAGE ( STAGE_END );
			}
			return SRESULT_WAIT;	

		case STAGE_END:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( enemy.fl.inFov && sweepCount < 3 && !gameLocal.random.RandomInt(2) ) {
					return SRESULT_STAGE ( STAGE_SWEEP );
				} else {
					PlayAnim ( ANIMCHANNEL_TORSO, "fire_forward_spray_end", 0 );
					return SRESULT_STAGE ( STAGE_FINISH );
				}
			}
			return SRESULT_WAIT;	

		case STAGE_FINISH:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_SCREAM );
			}
			return SRESULT_WAIT;	

		case STAGE_SCREAM:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				ActivateTargets( this );//toggle vent steam on
				PlayAnim ( ANIMCHANNEL_TORSO, "dispersal_vent", 0 );
				return SRESULT_STAGE ( STAGE_FINISH_SCREAM );
			}
			return SRESULT_WAIT;	

		case STAGE_FINISH_SCREAM:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				ActivateTargets( this );//toggle vent steam off
				return SRESULT_STAGE ( STAGE_START );
			}
			return SRESULT_WAIT;	
	}
	return SRESULT_ERROR; 
}

