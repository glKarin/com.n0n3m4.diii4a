
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterStreamProtector : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterStreamProtector );

	rvMonsterStreamProtector ( void );

	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	bool				CanTurn							( void ) const;
	virtual bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual	void		Damage							( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

protected:

	virtual bool		CheckPainActions				( void );
	virtual bool		CheckActions					( void );
	virtual bool		UpdateAnimationControllers		( void );

	jointHandle_t		jointPlasmaMuzzle;

	int					attackEndTime;
	int					attackNextTime;
	int					plasmaAttackRate;
	int					shots;

private:

	int					painConsecutive;

	rvAIAction			actionPlasmaAttack;
	rvAIAction			actionRocketAttack;
	rvAIAction			actionBlasterAttack;
	rvAIAction			actionHeavyBlasterAttack;
	rvAIAction			actionLightningActtack;
	rvAIAction			actionChaingunAttack;
	
	// Torso States
	stateResult_t		State_Killed					( const stateParms_t& parms );
	stateResult_t		State_Dead						( const stateParms_t& parms );
	
	stateResult_t		State_Torso_PlasmaAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_FinishPlasmaAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_BlasterAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_LightningAttack		( const stateParms_t& parms );

	stateResult_t		State_Torso_TurnRight90			( const stateParms_t& parms );
	stateResult_t		State_Torso_TurnLeft90			( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterStreamProtector );
};

CLASS_DECLARATION( idAI, rvMonsterStreamProtector )
END_CLASS

/*
================
rvMonsterStreamProtector::rvMonsterStreamProtector
================
*/
rvMonsterStreamProtector::rvMonsterStreamProtector ( void ) {
	painConsecutive	= 0;
}

void rvMonsterStreamProtector::InitSpawnArgsVariables ( void ) {
	jointPlasmaMuzzle = animator.GetJointHandle ( spawnArgs.GetString ( "joint_plasmaMuzzle", "NM_muzzle" ) );
	
	plasmaAttackRate = SEC2MS ( spawnArgs.GetFloat ( "attack_plasma_rate", ".15" ) );
}
/*
================
rvMonsterStreamProtector::Spawn
================
*/
void rvMonsterStreamProtector::Spawn ( void ) {
	actionPlasmaAttack.Init ( spawnArgs,		"action_plasmaAttack",			"Torso_PlasmaAttack",		AIACTIONF_ATTACK );
	actionRocketAttack.Init ( spawnArgs,		"action_rocketAttack",			NULL,						AIACTIONF_ATTACK );
	actionBlasterAttack.Init ( spawnArgs,		"action_blasterAttack",			NULL,						AIACTIONF_ATTACK );
	actionHeavyBlasterAttack.Init ( spawnArgs,	"action_heavyBlasterAttack",	"Torso_BlasterAttack",		AIACTIONF_ATTACK );
	actionLightningActtack.Init ( spawnArgs,	"action_lightningAttack",		"Torso_LightningAttack",	AIACTIONF_ATTACK );
	actionChaingunAttack.Init ( spawnArgs,		"action_chaingunAttack",		"Torso_ChaingunAttack",		AIACTIONF_ATTACK );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterStreamProtector::Save
================
*/
void rvMonsterStreamProtector::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt( attackEndTime );
	savefile->WriteInt( attackNextTime );
	savefile->WriteInt( shots );
	savefile->WriteInt( painConsecutive );

	actionPlasmaAttack.Save( savefile );
	actionRocketAttack.Save( savefile );
	actionBlasterAttack.Save( savefile );
	actionHeavyBlasterAttack.Save ( savefile );
	actionLightningActtack.Save( savefile );
	actionChaingunAttack.Save( savefile );
}

/*
================
rvMonsterStreamProtector::Restore
================
*/
void rvMonsterStreamProtector::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt( attackEndTime );
	savefile->ReadInt( attackNextTime );
	savefile->ReadInt( shots );
	savefile->ReadInt( painConsecutive );

	actionPlasmaAttack.Restore( savefile );
	actionRocketAttack.Restore( savefile );
	actionBlasterAttack.Restore( savefile );
	actionHeavyBlasterAttack.Restore ( savefile );
	actionLightningActtack.Restore( savefile );
	actionChaingunAttack.Restore( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterStreamProtector::Spawn
================
*/
bool rvMonsterStreamProtector::UpdateAnimationControllers ( void ) {
	// TODO: Target enemies behind us?  (doesnt need to be the same enemy that we are targetting)

	return idAI::UpdateAnimationControllers ( );
}

/*
================
rvMonsterStreamProtector::CheckPainActions
================
*/
bool rvMonsterStreamProtector::CheckPainActions ( void ) {
	if ( !pain.takenThisFrame || !actionTimerPain.IsDone ( actionTime ) ) {
		return false;
	}
	
	if ( !pain.threshold || pain.takenThisFrame < pain.threshold ) {
		if ( painConsecutive < 10 ) {
			return false;
		} else {
			painConsecutive = 0;
		}
	}
	
	PerformAction ( "Torso_Pain", 2, true );
	actionTimerPain.Reset ( actionTime );
	
	return true;	
}

/*
================
rvMonsterStreamProtector::CheckActions
================
*/
bool rvMonsterStreamProtector::CheckActions ( void ) {
	// If not moving, try turning in place
	if ( !move.fl.moving && gameLocal.time > combat.investigateTime ) {
		float turnYaw = idMath::AngleNormalize180 ( move.ideal_yaw - move.current_yaw ) ;
		if ( turnYaw > lookMax[YAW] * 0.75f ) {
			PerformAction ( "Torso_TurnRight90", 4, true );
			return true;
		} else if ( turnYaw < -lookMax[YAW] * 0.75f ) {
			PerformAction ( "Torso_TurnLeft90", 4, true );
			return true;
		}
	}

	if ( PerformAction ( &actionPlasmaAttack,		(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )  ||
		 PerformAction ( &actionRocketAttack,		(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )  ||
		 PerformAction ( &actionLightningActtack,	(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )  ||	
		 PerformAction ( &actionHeavyBlasterAttack,	(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )  ||	
		 PerformAction ( &actionBlasterAttack,		(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )      ) {
		return true;
	}
	return idAI::CheckActions ( );
}

/*
================
rvMonsterStreamProtector::CanTurn
================
*/
bool rvMonsterStreamProtector::CanTurn ( void ) const {
	if ( !idAI::CanTurn ( ) ) {
		return false;
	}
	return move.anim_turn_angles != 0.0f || move.fl.moving;
}

/*
================
rvMonsterStreamProtector::Pain
================
*/
bool rvMonsterStreamProtector::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( pain.lastTakenTime > gameLocal.GetTime() - 500 ) {
		painConsecutive++;
	} else {
		painConsecutive = 1;
	}
	return ( idAI::Pain( inflictor, attacker, damage, dir, location ) );
}

/*
================
rvMonsterStreamProtector::Damage
================
*/
void rvMonsterStreamProtector::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								  const char *damageDefName, const float damageScale, const int location ) {
	if ( attacker && attacker->IsType( rvMonsterStreamProtector::GetClassType() ) ) {
		//don't take damage from ourselves or other stream protectors
		return;
	}
	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterStreamProtector )
	STATE ( "State_Killed",				rvMonsterStreamProtector::State_Killed )
	STATE ( "State_Dead",				rvMonsterStreamProtector::State_Dead )

	STATE ( "Torso_PlasmaAttack",		rvMonsterStreamProtector::State_Torso_PlasmaAttack )
	STATE ( "Torso_FinishPlasmaAttack",	rvMonsterStreamProtector::State_Torso_FinishPlasmaAttack )
	STATE ( "Torso_BlasterAttack",		rvMonsterStreamProtector::State_Torso_BlasterAttack )
	STATE ( "Torso_LightningAttack",	rvMonsterStreamProtector::State_Torso_LightningAttack )
	
	STATE ( "Torso_TurnRight90",		rvMonsterStreamProtector::State_Torso_TurnRight90 )
	STATE ( "Torso_TurnLeft90",			rvMonsterStreamProtector::State_Torso_TurnLeft90 )
END_CLASS_STATES

/*
================
rvMonsterStreamProtector::State_Killed
================
*/
stateResult_t rvMonsterStreamProtector::State_Killed ( const stateParms_t& parms ) {
	// Make sure all animation stops
	StopAnimState ( ANIMCHANNEL_TORSO );
	StopAnimState ( ANIMCHANNEL_LEGS );
	if ( head ) {
		StopAnimState ( ANIMCHANNEL_HEAD );
	}

	DisableAnimState ( ANIMCHANNEL_LEGS );
	PlayAnim ( ANIMCHANNEL_TORSO, "death", parms.blendFrames );
	PostState ( "State_Dead" );
	return SRESULT_DONE;	
}

/*
================
rvMonsterStreamProtector::State_Dead
================
*/
stateResult_t rvMonsterStreamProtector::State_Dead ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
		return SRESULT_WAIT;
	}
	return idAI::State_Dead ( parms );
}

/*
================
rvMonsterStreamProtector::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blaster_start", parms.blendFrames );
			shots = (gameLocal.random.RandomInt ( 8 ) + 4) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_blaster_fire", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "range_blaster_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStreamProtector::State_Torso_PlasmaAttack
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_PlasmaAttack ( const stateParms_t& parms ) {	
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_FIRE,
		STAGE_INITIALFIRE_WAIT,
		STAGE_FIRE_WAIT,
		STAGE_END,
		STAGE_END_WAIT,
	};	
	switch ( parms.stage ) {
		case STAGE_START:
			DisableAnimState ( ANIMCHANNEL_LEGS );

			// Loop the flame animation
			PlayAnim( ANIMCHANNEL_TORSO, "range_plasma_start", parms.blendFrames );

			// Make sure we clean up some things when this state is finished (effects for one)			
			PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishPlasmaAttack", 0, 0, SFLAG_ONCLEAR );
			
			return SRESULT_STAGE ( STAGE_START_WAIT );
		
		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			return SRESULT_WAIT;

		case STAGE_FIRE:
			attackEndTime = gameLocal.time + 500;
			attackNextTime = gameLocal.time;
			
			// Flame effect
			PlayEffect ( "fx_plasma_muzzle", jointPlasmaMuzzle, true );
			
			PlayCycle ( ANIMCHANNEL_TORSO, "range_plasma_fire", 0 );
			
			return SRESULT_STAGE ( STAGE_INITIALFIRE_WAIT );

		case STAGE_INITIALFIRE_WAIT:
			if ( gameLocal.time > attackEndTime ) {
				attackEndTime = gameLocal.time + SEC2MS ( 1.0f + gameLocal.random.RandomFloat ( ) * 4.0f );
				return SRESULT_STAGE ( STAGE_FIRE_WAIT );
			}
			// Launch another attack?
			if ( gameLocal.time >= attackNextTime ) {
				Attack ( "plasma", jointPlasmaMuzzle, enemy.ent );
				attackNextTime = gameLocal.time + plasmaAttackRate;
			}
			return SRESULT_WAIT;
			
		case STAGE_FIRE_WAIT:
			// If we have been using plasma too long or havent seen our enemy for at least half a second then
			// stop now.
			if ( gameLocal.time > attackEndTime || gameLocal.time - enemy.lastVisibleTime > 500 || (IsEnemyVisible() && !enemy.fl.inFov) ) {
				StopEffect ( "fx_plasma_muzzle" );
				return SRESULT_STAGE ( STAGE_END );
			}
			// Launch another attack?
			if ( gameLocal.time >= attackNextTime  ) {
				Attack ( "plasma", jointPlasmaMuzzle, enemy.ent );
				attackNextTime = gameLocal.time + plasmaAttackRate;
			}
			return SRESULT_WAIT;
				
		case STAGE_END:
			// End animations
			PlayAnim( ANIMCHANNEL_TORSO, "range_plasma_end", parms.blendFrames );			
			return SRESULT_STAGE ( STAGE_END_WAIT );
		
		case STAGE_END_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;						
	}

	return SRESULT_ERROR;
}

/*
================
rvMonsterStreamProtector::State_Torso_FinishPlasmaAttack
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_FinishPlasmaAttack ( const stateParms_t& parms ) {	
	StopEffect ( "fx_plasma_muzzle" );
	return SRESULT_DONE;
}

/*
================
rvMonsterStreamProtector::State_Torso_LightningAttack
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_LightningAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_START:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			attackEndTime = gameLocal.time + 5000;
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_lightning_start", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_lightning_fire", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( gameLocal.time > attackEndTime || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_lightning_end", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_lightning_fire", 0 );
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStreamProtector::State_Torso_TurnRight90
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_TurnRight90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_right_90", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 )) {
				AnimTurn ( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStreamProtector::State_Torso_TurnLeft90
================
*/
stateResult_t rvMonsterStreamProtector::State_Torso_TurnLeft90 ( const stateParms_t& parms ) {	
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "turn_left_90", parms.blendFrames );
			AnimTurn ( 90.0f, true );
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( move.fl.moving || AnimDone ( ANIMCHANNEL_TORSO, 0 )) {
				AnimTurn ( 0, true );
				combat.investigateTime = gameLocal.time + 250;
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}
