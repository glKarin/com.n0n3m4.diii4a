
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterIronMaiden : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterIronMaiden );

	rvMonsterIronMaiden ( void );
	
	void				InitSpawnArgsVariables( void );
	void				Spawn				( void );
	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo		( debugInfoProc_t proc, void* userData );

	virtual int			FilterTactical		( int availableTactical );

protected:

	int					phaseTime;
	jointHandle_t		jointBansheeAttack;
	int					enemyStunTime;

	rvAIAction			actionBansheeAttack;

	virtual bool		CheckActions		( void );
	
	void				PhaseOut			( void );
	void				PhaseIn				( void );
	
	virtual void		OnDeath				( void );

private:

	// Custom actions
	bool				CheckAction_BansheeAttack		( rvAIAction* action, int animNum );
	bool				PerformAction_PhaseOut			( void );
	bool				PerformAction_PhaseIn			( void );
	
	// Global States
	stateResult_t		State_Phased					( const stateParms_t& parms );
	
	// Torso States
	stateResult_t		State_Torso_PhaseIn				( const stateParms_t& parms );
	stateResult_t		State_Torso_PhaseOut			( const stateParms_t& parms );
	stateResult_t		State_Torso_BansheeAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_BansheeAttackDone	( const stateParms_t& parms );

	// Frame commands
	stateResult_t		Frame_PhaseIn					( const stateParms_t& parms );
	stateResult_t		Frame_PhaseOut					( const stateParms_t& parms );
	stateResult_t		Frame_BansheeAttack				( const stateParms_t& parms );
	stateResult_t		Frame_EndBansheeAttack			( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterIronMaiden );
};

CLASS_DECLARATION( idAI, rvMonsterIronMaiden )
END_CLASS

/*
================
rvMonsterIronMaiden::rvMonsterIronMaiden
================
*/
rvMonsterIronMaiden::rvMonsterIronMaiden ( void ) {
	enemyStunTime		= 0;
	phaseTime			= 0;
}

void rvMonsterIronMaiden::InitSpawnArgsVariables ( void ) {
	// Cache the mouth joint
	jointBansheeAttack = animator.GetJointHandle ( spawnArgs.GetString ( "joint_bansheeAttack", "mouth_effect" ) );	
}
/*
================
rvMonsterIronMaiden::Spawn
================
*/
void rvMonsterIronMaiden::Spawn ( void ) {
	// Custom actions
	actionBansheeAttack.Init ( spawnArgs, "action_bansheeAttack", "Torso_BansheeAttack", AIACTIONF_ATTACK );
		
	InitSpawnArgsVariables();

	PlayEffect ( "fx_dress", animator.GetJointHandle ( spawnArgs.GetString ( "joint_laser", "cog_bone" ) ), true );
}

/*
================
rvMonsterIronMaiden::Save
================
*/
void rvMonsterIronMaiden::Save( idSaveGame *savefile ) const {
	savefile->WriteInt ( phaseTime );
	savefile->WriteInt ( enemyStunTime );
	
	actionBansheeAttack.Save ( savefile );
}

/*
================
rvMonsterIronMaiden::Restore
================
*/
void rvMonsterIronMaiden::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt ( phaseTime );
	savefile->ReadInt ( enemyStunTime );
	
	actionBansheeAttack.Restore ( savefile );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterIronMaiden::FilterTactical
================
*/
int rvMonsterIronMaiden::FilterTactical ( int availableTactical ) {
	// When hidden the iron maiden only uses ranged tactical
	if ( fl.hidden ) {
		availableTactical &= (AITACTICAL_RANGED_BIT | AITACTICAL_TURRET_BIT);
	}	
	return idAI::FilterTactical( availableTactical );
}

/*
================
rvMonsterIronMaiden::CheckAction_BansheeAttack
================
*/
bool rvMonsterIronMaiden::CheckAction_BansheeAttack ( rvAIAction* action, int animNum ) {
	return CheckAction_RangedAttack ( action, animNum );
}

/*
================
rvMonsterIronMaiden::PerformAction_PhaseIn
================
*/
bool rvMonsterIronMaiden::PerformAction_PhaseIn ( void ) {
	if ( !phaseTime ) {
		return false;
	}
	
	// Must be out for at least 3 seconds 
	if ( gameLocal.time - phaseTime < 3000  ) {
		return false;
	}

	// Phase in after 10 seconds or our movement is done
	if ( gameLocal.time - phaseTime > 10000	|| move.fl.done ) {
		// Make sure we arent in something
		if ( CanBecomeSolid ( ) ) {
			PerformAction ( "Torso_PhaseIn", 4, true );
			return true;
		}
	}	
	
	return false;
}

/*
================
rvMonsterIronMaiden::PerformAction_PhaseOut
================
*/
bool rvMonsterIronMaiden::PerformAction_PhaseOut ( void ) {
	// Little randomization 
	if ( gameLocal.random.RandomFloat ( ) > 0.5f ) {
		return false;
	}
	if ( !enemyStunTime || gameLocal.time - enemyStunTime > 1500 ) {
		return false;
	}
	
	PerformAction ( "Torso_PhaseOut", 4, true );
	return true;		
}

/*
================
rvMonsterIronMaiden::CheckActions
================
*/
bool rvMonsterIronMaiden::CheckActions ( void ) {
	// When phased the only available action is phase in
	if ( phaseTime ) {
		if ( PerformAction_PhaseIn ( ) ) {
			return true;
		}
		return false;
	}

	if ( PerformAction ( &actionBansheeAttack, (checkAction_t)&rvMonsterIronMaiden::CheckAction_BansheeAttack, &actionTimerRangedAttack ) ) {
		return true;
	}
	
	if ( PerformAction_PhaseOut ( ) ) {
		return true;
	}
	
	return idAI::CheckActions ( );
}

/*
================
rvMonsterIronMaiden::PhaseOut
================
*/
void rvMonsterIronMaiden::PhaseOut ( void ) {
	if ( phaseTime ) {
		return;
	}

	rvClientEffect* effect;
	effect = PlayEffect ( "fx_phase", animator.GetJointHandle("cog_bone") );
	if ( effect ) {
		effect->Unbind ( );
	}
	
	Hide ( );
	
	move.fl.allowHiddenMove = true;

	ProcessEvent ( &EV_BecomeNonSolid );
	
	StopMove ( MOVE_STATUS_DONE );
	SetState ( "State_Phased" );
	
	// Move away from here, to anywhere
	MoveOutOfRange ( this, 500.0f, 150.0f );

	SetShaderParm ( 5, MS2SEC ( gameLocal.time ) );
		
	phaseTime = gameLocal.time;
}

/*
================
rvMonsterIronMaiden::PhaseIn
================
*/
void rvMonsterIronMaiden::PhaseIn ( void ) {
	if ( !phaseTime ) {
		return;
	}

	rvClientEffect* effect;
	effect = PlayEffect ( "fx_phase", animator.GetJointHandle("cog_bone") );
	if ( effect ) {
		effect->Unbind ( );
	}
	
	if ( enemy.ent ) {
		TurnToward ( enemy.lastKnownPosition );
	}

	ProcessEvent ( &AI_BecomeSolid );

	Show ( );

//	PlayEffect ( "fx_dress", animator.GetJointHandle ( "cog_bone" ), true );

	phaseTime = 0;
	// Wait for the action that started the phase in to finish, then go back to combat
	SetState ( "Wait_Action" );
	PostState ( "State_Combat" );
	
	SetShaderParm ( 5, MS2SEC ( gameLocal.time ) );
}

/*
================
rvMonsterIronMaiden::OnDeath
================
*/
void rvMonsterIronMaiden::OnDeath ( void ) {
	StopSound ( SND_CHANNEL_ITEM, false );

	// Stop looping effects
	StopEffect ( "fx_banshee" );
	StopEffect ( "fx_dress" );
	
	idAI::OnDeath( );
}

/*
================
rvMonsterIronMaiden::GetDebugInfo
================
*/
void rvMonsterIronMaiden::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo( proc, userData );
	
	proc ( "idAI", "action_BansheeAttack",	aiActionStatusString[actionBansheeAttack.status], userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterIronMaiden )
	STATE ( "State_Phased",				rvMonsterIronMaiden::State_Phased )

	STATE ( "Torso_PhaseOut",			rvMonsterIronMaiden::State_Torso_PhaseOut )
	STATE ( "Torso_PhaseIn",			rvMonsterIronMaiden::State_Torso_PhaseIn )
	STATE ( "Torso_BansheeAttack",		rvMonsterIronMaiden::State_Torso_BansheeAttack )
	STATE ( "Torso_BansheeAttackDone",	rvMonsterIronMaiden::State_Torso_BansheeAttackDone )
	
	STATE ( "Frame_PhaseIn",			rvMonsterIronMaiden::Frame_PhaseIn )
	STATE ( "Frame_PhaseOut",			rvMonsterIronMaiden::Frame_PhaseOut )
	STATE ( "Frame_BansheeAttack",		rvMonsterIronMaiden::Frame_BansheeAttack )
	STATE ( "Frame_EndBansheeAttack",	rvMonsterIronMaiden::Frame_EndBansheeAttack )
END_CLASS_STATES

/*
================
rvMonsterIronMaiden::State_Phased
================
*/
stateResult_t rvMonsterIronMaiden::State_Phased ( const stateParms_t& parms ) {
	// If done moving and cant become solid here, move again
	if ( move.fl.done ) {
		if ( !CanBecomeSolid ( ) ) {
			MoveOutOfRange ( this, 300.0f, 150.0f );
		}
	}

	// Keep the enemy status up to date
	if ( !enemy.ent ) {
		CheckForEnemy ( true );
	}

	// Make sure we keep facing our enemy
	if ( enemy.ent ) {
		TurnToward ( enemy.lastKnownPosition );
	}

	// Make sure we are checking actions if we have no tactical move
	UpdateAction ( );
	
	return SRESULT_WAIT;
}

/*
================
rvMonsterIronMaiden::State_Torso_PhaseIn
================
*/
stateResult_t rvMonsterIronMaiden::State_Torso_PhaseIn ( const stateParms_t& parms ) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
		STAGE_PHASE,
	};
	switch ( parms.stage ) {
		case STAGE_ANIM:
			if ( enemy.ent ) {
				TurnToward ( enemy.lastKnownPosition );
			}
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "phase_in", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_ANIM_WAIT );
	
		case STAGE_ANIM_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;			
	}						
	return SRESULT_ERROR;
}

/*
================
rvMonsterIronMaiden::State_Torso_PhaseOut
================
*/
stateResult_t rvMonsterIronMaiden::State_Torso_PhaseOut ( const stateParms_t& parms ) {
	enum {
		STAGE_ANIM,
		STAGE_ANIM_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ANIM:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "phase_out", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_ANIM_WAIT );
	
		case STAGE_ANIM_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}						
	return SRESULT_ERROR;
}

/*
================
rvMonsterIronMaiden::State_Torso_BansheeAttack
================
*/
stateResult_t rvMonsterIronMaiden::State_Torso_BansheeAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_ATTACK,
		STAGE_ATTACK_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_ATTACK:
			PlayAnim ( ANIMCHANNEL_TORSO, "banshee", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_TORSO, "Torso_BansheeAttackDone", 0, 0, SFLAG_ONCLEAR );
			return SRESULT_STAGE ( STAGE_ATTACK_WAIT );
			
		case STAGE_ATTACK_WAIT:
			if ( enemy.ent ) {
				TurnToward ( enemy.lastKnownPosition );
			}			
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
rvMonsterIronMaiden::State_Torso_BansheeAttackDone

To ensure the movement is enabled after the banshee attack and that the effect is stopped this state
will be posted to be run after the banshe attack finishes.
================
*/
stateResult_t rvMonsterIronMaiden::State_Torso_BansheeAttackDone ( const stateParms_t& parms ) {
	Frame_EndBansheeAttack ( parms );
	return SRESULT_DONE;
}

/*
================
rvMonsterIronMaiden::Frame_PhaseIn
================
*/
stateResult_t rvMonsterIronMaiden::Frame_PhaseIn ( const stateParms_t& parms ) {
	PhaseIn ( );
	return SRESULT_OK;
}


/*
================
rvMonsterIronMaiden::Frame_PhaseOut
================
*/
stateResult_t rvMonsterIronMaiden::Frame_PhaseOut ( const stateParms_t& parms ) {
	PhaseOut ( );
	return SRESULT_OK;
}

/*
================
rvMonsterIronMaiden::Frame_BansheeAttack
================
*/
stateResult_t rvMonsterIronMaiden::Frame_BansheeAttack ( const stateParms_t& parms ) {
	idVec3		origin;
	idMat3		axis;
	idEntity*	entities[ 1024 ];
	int			count;
	int			i;
	
	// Get mouth origin
	GetJointWorldTransform ( jointBansheeAttack, gameLocal.time, origin, axis );
	
	// Find all entities within the banshee attacks radius 
	count = gameLocal.EntitiesWithinRadius ( origin, actionBansheeAttack.maxRange, entities, 1024 );
	for ( i = 0; i < count; i ++ ) {
		idEntity* ent = entities[i];		
		if ( !ent || ent == this ) {
			continue;
		}

		// Must be an actor that takes damage to be affected		
		if ( !ent->fl.takedamage || !ent->IsType ( idActor::GetClassType() ) ) {
			continue;
		}
				
		// Has to be within fov
		if ( !CheckFOV ( ent->GetEyePosition ( ) ) ) {
			continue;
		}

		// Do some damage		
		idVec3 dir;
		dir = origin = ent->GetEyePosition ( );
		dir.NormalizeFast ( );
		ent->Damage ( this, this, dir, spawnArgs.GetString ( "def_banshee_damage" ), 1.0f, 0 );
		
		// Cache the last time we stunned our own enemy for the phase out
		if ( ent == enemy.ent ) {
			enemyStunTime = gameLocal.time;
		}
	}
	
	return SRESULT_OK;
}

/*
================
rvMonsterIronMaiden::Frame_EndBansheeAttack
================
*/
stateResult_t rvMonsterIronMaiden::Frame_EndBansheeAttack ( const stateParms_t& parms ) {
	StopEffect ( "fx_banshee" );
	return SRESULT_OK;
}
