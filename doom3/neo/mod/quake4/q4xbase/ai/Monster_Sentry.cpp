
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterSentry : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterSentry );

	rvMonsterSentry ( void );
	
	void				InitSpawnArgsVariables( void );
	void				Spawn				( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );
	
	virtual void		Think				( void );

	virtual bool		Pain				( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo		( debugInfoProc_t proc, void* userData );

protected:

	rvAIAction			actionBlasterAttack;
	rvAIAction			actionKamakaziAttack;
	rvAIAction			actionCircleStrafe;

	int					shots;
	int					minShots;
	int					maxShots;
	int					kamakaziHealth;
	int					strafeTime;
	bool				strafeRight;

	virtual bool		CheckActions		( void );
	virtual int			FilterTactical		( int availableTactical );

	virtual void		OnDeath				( void );

	void				Explode				( bool force = false );

private:

	int					nextChatterTime;

	bool				CheckAction_CircleStrafe		( rvAIAction* action, int animNum );

	// Torso States
	stateResult_t		State_Torso_BlasterAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_KamakaziAttack		( const stateParms_t& parms );
	stateResult_t		State_CircleStrafe				( const stateParms_t& parms );
	stateResult_t		State_Action_CircleStrafe		( const stateParms_t& parms );
	stateResult_t		State_Torso_EvadeLeft			( const stateParms_t& parms );
	stateResult_t		State_Torso_EvadeRight			( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterSentry );
};

CLASS_DECLARATION( idAI, rvMonsterSentry )
END_CLASS

/*
================
rvMonsterSentry::rvMonsterSentry
================
*/
rvMonsterSentry::rvMonsterSentry ( void ) {
	strafeTime	= 0;
	strafeRight = false;
}

void rvMonsterSentry::InitSpawnArgsVariables( void )
{
	minShots = spawnArgs.GetInt ( "minShots" );
	maxShots = spawnArgs.GetInt ( "maxShots" );
}
/*
================
rvMonsterSentry::Spawn
================
*/
void rvMonsterSentry::Spawn ( void ) {
	// Custom actions
	actionBlasterAttack.Init ( spawnArgs, "action_blasterAttack", "Torso_BlasterAttack", AIACTIONF_ATTACK );		
	actionKamakaziAttack.Init ( spawnArgs, "action_kamakaziAttack", "Torso_KamakaziAttack", AIACTIONF_ATTACK );
	actionCircleStrafe.Init ( spawnArgs, "action_circleStrafe", "State_Action_CircleStrafe", AIACTIONF_ATTACK );
	
	InitSpawnArgsVariables();

	kamakaziHealth = spawnArgs.GetInt ( "kamakazi_Health", va("%d", health / 2) );
	nextChatterTime = 0;
} 

/*
================
rvMonsterSentry::Save
================
*/
void rvMonsterSentry::Save ( idSaveGame *savefile ) const {
	actionBlasterAttack.Save( savefile );
	actionKamakaziAttack.Save( savefile );
	actionCircleStrafe.Save( savefile );

	savefile->WriteInt( shots );
	savefile->WriteInt( kamakaziHealth );
	savefile->WriteInt ( strafeTime	);
	savefile->WriteBool ( strafeRight );
	savefile->WriteInt ( nextChatterTime );
}

/*
================
rvMonsterSentry::Restore
================
*/
void rvMonsterSentry::Restore ( idRestoreGame *savefile ) {
	actionBlasterAttack.Restore( savefile );
	actionKamakaziAttack.Restore( savefile );
	actionCircleStrafe.Restore( savefile );

	savefile->ReadInt( shots );
	savefile->ReadInt( kamakaziHealth );
	savefile->ReadInt ( strafeTime	);
	savefile->ReadBool ( strafeRight );
	savefile->ReadInt ( nextChatterTime );

	InitSpawnArgsVariables();
}

/*
================
rvMonsterSentry::Pain
================
*/
bool rvMonsterSentry::Pain ( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( kamakaziHealth > 0 && health < kamakaziHealth ) {
		kamakaziHealth = 0;		
		move.speed = move.fly_speed = spawnArgs.GetFloat ( "kamakazi_fly_speed", va("%g", move.fly_speed ) );
		move.turnRate = spawnArgs.GetFloat ( "kamakazi_turn_rate", va("%g", move.turnRate ) );
		move.fly_pitch_max = spawnArgs.GetFloat ( "kamakazi_fly_pitch_max" );
		move.fly_pitch_scale = spawnArgs.GetFloat ( "kamakazi_fly_pitch_scale" );
	}
		
	return idAI::Pain ( inflictor, attacker, damage, dir, location );
}

/*
================
rvMonsterSentry::Think
================
*/
void rvMonsterSentry::Think ( void ) {
	// Explode whenever we hit something 
	if ( kamakaziHealth == 0 && physicsObj.GetSlideMoveEntity () ) {
		Explode ( );
	} else {
		/*
		if ( nextChatterTime < gameLocal.GetTime() ) {
			if ( GetEnemy() ) {
				Speak( "lipsync_chatter_combat" );
				nextChatterTime = speakTime + 2000 + gameLocal.random.RandomInt( 2000 );
			} else {
				Speak( "lipsync_chatter_idle" );
				nextChatterTime = speakTime + 3000 + gameLocal.random.RandomInt( 3000 );
			}
		}
		*/

		idAI::Think ( );
	}
}

/*
================
rvMonsterSentry::CheckAction_CircleStrafe
================
*/
bool rvMonsterSentry::CheckAction_CircleStrafe ( rvAIAction* action, int animNum ) {
	if ( !enemy.fl.visible ) {
		return false;
	}

	if ( !enemy.fl.inFov ) {
		return false;
	}

	if ( !move.fl.done ) {
		return false;
	}

	return true;
}

/*
================
rvMonsterSentry::CheckActions
================
*/
bool rvMonsterSentry::CheckActions ( void ) {
	if ( kamakaziHealth != 0 ) {
		if ( PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
		if ( PerformAction ( &actionCircleStrafe, (checkAction_t)&rvMonsterSentry::CheckAction_CircleStrafe ) ) {
			return true;
		}
	} else {
		if ( PerformAction ( &actionKamakaziAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack, NULL ) ) {
			return true;
		}
	}
	
	if ( idAI::CheckActions ( ) ) {
		return true;
	}

	return false;
}

/*
================
rvMonsterSentry::FilterTactical
================
*/
int rvMonsterSentry::FilterTactical ( int availableTactical ) {
	// Kamakazi Health will be set to 0 when its time to kamakazi
	if ( !kamakaziHealth ) {
		return availableTactical & (AITACTICAL_MELEE_BIT|AITACTICAL_TURRET_BIT);
	// No rush when not in kamakazi mode
	} else {
		availableTactical &= ~(AITACTICAL_MELEE_BIT);
	}
		
	return idAI::FilterTactical ( availableTactical );
}

/*
================
rvMonsterSentry::OnDeath
================
*/
void rvMonsterSentry::OnDeath ( void ) {
	idAI::OnDeath ( );
	
	Explode ( true );
}

/*
================
rvMonsterSentry::Explode
================
*/
void rvMonsterSentry::Explode ( bool force ) {
	if ( health > 0 || force ) {
		gameLocal.RadiusDamage ( GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString ( "def_kamakazi_damage" ), 1.0f );
		
		// Kill ourselves
		Damage ( this, this, viewAxis[0], "damage_gib", 1.0f, 0 );
	}
}

/*
================
rvMonsterSentry::GetDebugInfo
================
*/
void rvMonsterSentry::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "action_blasterAttack",	aiActionStatusString[actionBlasterAttack.status], userData );
	proc ( "idAI", "action_kamakaziAttack",	aiActionStatusString[actionKamakaziAttack.status], userData );	
	proc ( "idAI", "action_circleStrafe",	aiActionStatusString[actionCircleStrafe.status], userData );	
	proc ( "rvMonsterSentry", "strafeRight",strafeRight?"true":"false", userData );
	proc ( "rvMonsterSentry", "strafeTime",	va("%d",strafeTime), userData );
	proc ( "rvMonsterSentry", "nextChatterTime",	va("%d",nextChatterTime), userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterSentry )
	STATE ( "Torso_BlasterAttack",		rvMonsterSentry::State_Torso_BlasterAttack )
	STATE ( "Torso_KamakaziAttack",		rvMonsterSentry::State_Torso_KamakaziAttack )
	STATE ( "State_CircleStrafe",		rvMonsterSentry::State_CircleStrafe )
	STATE ( "State_Action_CircleStrafe",rvMonsterSentry::State_Action_CircleStrafe )
	STATE ( "Torso_EvadeLeft",			rvMonsterSentry::State_Torso_EvadeLeft )
	STATE ( "Torso_EvadeRight",			rvMonsterSentry::State_Torso_EvadeRight )
END_CLASS_STATES

/*
================
rvMonsterSentry::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterSentry::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_ATTACK,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = (gameLocal.random.RandomInt ( maxShots - minShots ) + minShots) * combat.aggressiveScale;
			return SRESULT_STAGE ( STAGE_ATTACK );

		case STAGE_ATTACK:			
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", parms.blendFrames );
			shots--;
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (IsEnemyVisible() && !enemy.fl.inFov)  ) {
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_ATTACK );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterSentry::State_Torso_KamakaziAttack
================
*/
stateResult_t rvMonsterSentry::State_Torso_KamakaziAttack ( const stateParms_t& parms ) {
	Explode ( );
	return SRESULT_DONE;
}

stateResult_t  rvMonsterSentry::State_CircleStrafe ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_CIRCLE
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( strafeTime ) {
				//already strafing, just change dir?
				strafeTime = (strafeTime<gameLocal.GetTime()+2000)?gameLocal.GetTime()+2000:strafeTime;
				strafeRight = !strafeRight;
			} else {
				strafeTime = gameLocal.GetTime() + 8000;
				//FIXME: try to see which side it clear?
				strafeRight = (gameLocal.random.RandomFloat()>0.5f);
			}
			return SRESULT_STAGE ( STAGE_CIRCLE );
		case STAGE_CIRCLE:
			if ( !GetEnemy() || strafeTime < gameLocal.GetTime() || !enemy.fl.visible || !enemy.fl.inFov ) {
				//FIXME: also stop if I bump into something
				strafeTime = 0;
				SetState( "State_Combat" );
				return SRESULT_DONE;
			}
			idVec3 vel = GetPhysics()->GetLinearVelocity();
			vel += viewAxis[1] * (strafeRight?-200:200);
			vel.Normalize();
			vel *= 200.0f;
			physicsObj.UseVelocityMove( true );
			GetPhysics()->SetLinearVelocity( vel );
			TurnToward( enemy.lastKnownPosition );

			// Perform actions
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}

			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t rvMonsterSentry::State_Action_CircleStrafe ( const stateParms_t& parms ) {
	SetState( "State_CircleStrafe" );
	return SRESULT_DONE;
}

stateResult_t rvMonsterSentry::State_Torso_EvadeLeft ( const stateParms_t& parms ) {
	if ( strafeTime ) {
		strafeRight = false;
	} else {
		idVec3 vel = GetPhysics()->GetLinearVelocity();
		vel += viewAxis[1] * 400;
		physicsObj.UseVelocityMove( true );
		GetPhysics()->SetLinearVelocity( vel );
	}
	return SRESULT_DONE;
}

stateResult_t rvMonsterSentry::State_Torso_EvadeRight ( const stateParms_t& parms ) {
	if ( strafeTime ) {
		strafeRight = true;
	} else {
		idVec3 vel = GetPhysics()->GetLinearVelocity();
		vel += viewAxis[1] * -400;
		physicsObj.UseVelocityMove( true );
		GetPhysics()->SetLinearVelocity( vel );
	}
	return SRESULT_DONE;
}
