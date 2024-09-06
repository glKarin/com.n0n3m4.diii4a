
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterTurret : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterTurret );

	rvMonsterTurret ( void );

	void				InitSpawnArgsVariables	( void );
	void				Spawn					( void );
	void				Save					( idSaveGame *savefile ) const;
	void				Restore					( idRestoreGame *savefile );

	virtual bool		Pain					( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

protected:

	virtual bool		CheckActions			( void );

	stateResult_t		State_Combat			( const stateParms_t& parms );
	stateResult_t		State_Killed			( const stateParms_t& parms );

	int					shieldHealth;
	int					maxShots;	
	int					minShots;
	int					shots;

private:

	rvAIAction			actionBlasterAttack;

	stateResult_t		State_Torso_BlasterAttack	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterTurret );
};

CLASS_DECLARATION( idAI, rvMonsterTurret )
END_CLASS

/*
================
rvMonsterTurret::rvMonsterTurret
================
*/
rvMonsterTurret::rvMonsterTurret ( ) {
	shieldHealth = 0;
}

void rvMonsterTurret::InitSpawnArgsVariables ( void ) {
	maxShots	= spawnArgs.GetInt ( "maxShots", "1" );
	minShots	= spawnArgs.GetInt ( "minShots", "1" );
}
/*
================
rvMonsterTurret::Spawn
================
*/
void rvMonsterTurret::Spawn ( void ) {
	actionBlasterAttack.Init ( spawnArgs,	"action_blasterAttack",	"Torso_BlasterAttack",	AIACTIONF_ATTACK );

	shieldHealth = spawnArgs.GetInt ( "shieldHealth" );
	health += shieldHealth;

	InitSpawnArgsVariables();
	shots		= 0;
}

/*
================
rvMonsterTurret::Save
================
*/
void rvMonsterTurret::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( shieldHealth );
	savefile->WriteInt ( shots );
	actionBlasterAttack.Save ( savefile );
}

/*
================
rvMonsterTurret::Restore
================
*/
void rvMonsterTurret::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( shieldHealth );
	savefile->ReadInt ( shots );
	actionBlasterAttack.Restore ( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterTurret::CheckActions
================
*/
bool rvMonsterTurret::CheckActions ( void ) {
	// Attacks
	if ( PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
		return true;
	}
	return idAI::CheckActions ( );
}

/*
================
rvMonsterTurret::Pain
================
*/
bool rvMonsterTurret::Pain ( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	// Handle the shield effects
	if ( shieldHealth > 0 ) {
		shieldHealth -= damage;
		if ( shieldHealth <= 0 ) {
			PlayEffect ( "fx_shieldBreak", GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
		} else {
			PlayEffect ( "fx_shieldHit", GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
		}
	}

	return idAI::Pain ( inflictor, attacker, damage, dir, location );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterTurret )
	STATE ( "State_Combat",			rvMonsterTurret::State_Combat )
	STATE ( "State_Killed",			rvMonsterTurret::State_Killed )

	STATE ( "Torso_BlasterAttack",	rvMonsterTurret::State_Torso_BlasterAttack )
END_CLASS_STATES

/*
================
rvMonsterTurret::State_Combat
================
*/
stateResult_t rvMonsterTurret::State_Combat ( const stateParms_t& parms ) {
	// Aquire a new enemy if we dont have one
	if ( !enemy.ent ) {
		CheckForEnemy ( true );
	}

	FaceEnemy ( );
			
	// try moving, if there was no movement run then just try and action instead
	UpdateAction ( );
	
	return SRESULT_WAIT;
}

/*
================
rvMonsterTurret::State_Killed
================
*/
stateResult_t rvMonsterTurret::State_Killed ( const stateParms_t& parms ) {
	gameLocal.PlayEffect ( gameLocal.GetEffect ( spawnArgs, "fx_death" ), GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
	return idAI::State_Killed ( parms );
}
	
/*
================
rvMonsterTurret::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterTurret::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_FIRE,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_FIRE );
			
		case STAGE_FIRE:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", 2 );
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 2 ) ) {
				if ( --shots <= 0 ) {
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}
