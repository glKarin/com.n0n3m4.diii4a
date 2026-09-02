
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../vehicle/Vehicle.h"

class rvMonsterHeavyHoverTank : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterHeavyHoverTank );

	rvMonsterHeavyHoverTank ( void );

	void					InitSpawnArgsVariables( void );
	void					Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );
		
	virtual void			Think				( void );
	
	virtual	void			Damage				( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			OnDeath				( void );
	
protected:

	enum weaponState_t {
		WEAPONSTATE_BLASTER,
		WEAPONSTATE_ROCKET,
		WEAPONSTATE_MAX
	};

	int					blasterAnimIndex;
	int					shots;
	weaponState_t		weaponStateCurrent;
	weaponState_t		weaponStateIdeal;
	
	rvAIAction			actionRocketAttack;
	rvAIAction			actionBlasterAttack;
	rvAIAction			actionStrafe;
	
	jointHandle_t		jointHoverEffect;
	
	rvClientEffectPtr	effectDust;
	rvClientEffectPtr	effectHover;

	virtual bool			CheckActions				( void );
	virtual void			OnEnemyChange				( idEntity* oldEnemy );

//	virtual const char*		GetIdleAnimName				( void );

private:

	float					strafeSpeed;

	bool					CheckAction_Strafe				( rvAIAction* action, int animNum );

	stateResult_t			State_Torso_BlasterAttack		( const stateParms_t& parms );
	stateResult_t			State_Torso_RocketAttack		( const stateParms_t& parms );
	stateResult_t			State_Torso_ChangeWeaponState	( const stateParms_t& parms );
	stateResult_t			State_Torso_EvadeLeft			( const stateParms_t& parms );
	stateResult_t			State_Torso_EvadeRight			( const stateParms_t& parms );
	stateResult_t			State_Torso_Strafe				( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterHeavyHoverTank );
};

CLASS_DECLARATION( idAI, rvMonsterHeavyHoverTank )
END_CLASS

/*
================
rvMonsterHeavyHoverTank::rvMonsterHeavyHoverTank
================
*/
rvMonsterHeavyHoverTank::rvMonsterHeavyHoverTank ( ) {
	weaponStateIdeal   = WEAPONSTATE_BLASTER;
	weaponStateCurrent = weaponStateIdeal;
	
	effectDust  = NULL;
	effectHover = NULL;
	
	blasterAnimIndex = 0;
	shots			 = 0;

	strafeSpeed		= 0;
}

void rvMonsterHeavyHoverTank::InitSpawnArgsVariables( void )
{
	jointHoverEffect = animator.GetJointHandle ( spawnArgs.GetString("joint_hover") );
	strafeSpeed = spawnArgs.GetFloat( "strafeSpeed", "300" );
}
/*
================
rvMonsterHeavyHoverTank::Spawn
================
*/
void rvMonsterHeavyHoverTank::Spawn ( void ) {
	actionRocketAttack.Init ( spawnArgs, "action_rocketAttack", "Torso_RocketAttack", AIACTIONF_ATTACK );
	actionBlasterAttack.Init ( spawnArgs, "action_blasterAttack", "Torso_BlasterAttack", AIACTIONF_ATTACK );
	actionStrafe.Init  ( spawnArgs, "action_strafe",	"Torso_Strafe",	0 );
	
	InitSpawnArgsVariables();

	if ( jointHoverEffect != INVALID_JOINT ) {
		effectHover = PlayEffect ( "fx_hover", jointHoverEffect, true );
	}
}

/*
================
rvMonsterHeavyHoverTank::Think
================
*/
void rvMonsterHeavyHoverTank::Think ( void ) {
	idAI::Think ( );
	
	// If thinking we should play an effect on the ground under us
	if ( jointHoverEffect != INVALID_JOINT ) {
		if ( !fl.hidden && !fl.isDormant && (thinkFlags & TH_THINK ) && !aifl.dead ) {
			trace_t tr;
			idVec3	origin;
			idMat3	axis;
			
			// Project the effect 128 units down from the hover effect joint
			GetJointWorldTransform ( jointHoverEffect, gameLocal.time, origin, axis );
			
	// RAVEN BEGIN
	// ddynerman: multiple clip worlds
			gameLocal.TracePoint ( this, tr, origin, origin + axis[0] * 128.0f, CONTENTS_SOLID, this );
	// RAVEN END

			// Start the dust effect if not already started
			if ( !effectDust ) {
				effectDust = gameLocal.PlayEffect ( gameLocal.GetEffect ( spawnArgs, "fx_dust" ), tr.endpos, tr.c.normal.ToMat3(), true );
			}
			
			// If the effect is playing we should update its attenuation as well as its origin and axis
			if ( effectDust ) {
				effectDust->Attenuate ( 1.0f - idMath::ClampFloat ( 0.0f, 1.0f, (tr.endpos - origin).LengthFast ( ) / 127.0f ) );
				effectDust->SetOrigin ( tr.endpos );
				effectDust->SetAxis ( tr.c.normal.ToMat3() );
			}
			
			// If the hover effect is playing we can set its end origin to the ground
			if ( effectHover ) {
				effectHover->SetEndOrigin ( tr.endpos );
			}
		} else if ( effectDust ) {
			effectDust->Stop ( );
			effectDust = NULL;
		}
	}
}

/*
================
rvMonsterHeavyHoverTank::Save
================
*/
void rvMonsterHeavyHoverTank::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( blasterAnimIndex );
	savefile->WriteInt ( shots );
	savefile->WriteInt ( (int)weaponStateCurrent );
	savefile->WriteInt ( (int)weaponStateIdeal );
	
	actionRocketAttack.Save ( savefile );
	actionBlasterAttack.Save ( savefile );
	actionStrafe.Save ( savefile );
	
	effectDust.Save ( savefile );
	effectHover.Save ( savefile );
}

/*
================
rvMonsterHeavyHoverTank::Restore
================
*/
void rvMonsterHeavyHoverTank::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( blasterAnimIndex );
	savefile->ReadInt ( shots );
	savefile->ReadInt ( (int&)weaponStateCurrent );
	savefile->ReadInt ( (int&)weaponStateIdeal );
	
	actionRocketAttack.Restore ( savefile );
	actionBlasterAttack.Restore ( savefile );
	actionStrafe.Restore ( savefile );
	
	effectDust.Restore ( savefile );
	effectHover.Restore ( savefile );

	InitSpawnArgsVariables();
}

void rvMonsterHeavyHoverTank::Damage ( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location )
{
	if ( damageScale > 0.0f ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
		if ( damageDef && damageDef->GetBool( "vehicle_collision" ) ) {
			//push me hard!
			float push = idMath::ClampFloat( 250.0f, 500.0f, damageScale*1000.0f );
			idVec3 vel = GetPhysics()->GetLinearVelocity();
			vel += dir * push;
			physicsObj.UseVelocityMove( true );
			GetPhysics()->SetLinearVelocity( vel );
		}
	}

	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
================
rvMonsterHeavyHoverTank::OnDeath
================
*/
void rvMonsterHeavyHoverTank::OnDeath ( void ) {
	// Stop the dust effect
	if ( effectDust ) {
		effectDust->Stop ( );
		effectDust = NULL;
	}

	// Stop the hover effect
	if ( effectHover ) {
		effectHover->Stop ( );
		effectHover = NULL;
	}
	
	idAI::OnDeath ( );
}

/*
================
rvMonsterHeavyHoverTank::CheckAction_Strafe
================
*/
bool rvMonsterHeavyHoverTank::CheckAction_Strafe ( rvAIAction* action, int animNum ) {
	if ( !enemy.fl.visible ) {
		return false;
	}

	if ( !enemy.fl.inFov ) {
		return false;
	}

	if ( !move.fl.done ) {
		return false;
	}

	if ( animNum != -1 && !TestAnimMove ( animNum ) ) {
		//well, at least try a new attack position
		if ( combat.tacticalCurrent == AITACTICAL_RANGED ) {
			combat.tacticalUpdateTime = 0;
		}
		return false;
	}
	return true;
}

/*
================
rvMonsterHeavyHoverTank::Spawn
================
*/
bool rvMonsterHeavyHoverTank::CheckActions ( void ) {
	if ( weaponStateIdeal != weaponStateCurrent ) {
		PerformAction ( "Torso_ChangeWeaponState", 4 );
		return true;
	}

	if ( PerformAction ( &actionRocketAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ) {
		return true;
	}

	if ( PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
		return true;
	}

	if ( idAI::CheckActions( ) ) {
		return true;
	}

	if ( PerformAction ( &actionStrafe,  (checkAction_t)&rvMonsterHeavyHoverTank::CheckAction_Strafe ) )
	{
		return true;
	}
	return false;
}

/*
================
rvMonsterHeavyHoverTank::OnEnemyChange
================
*/
void rvMonsterHeavyHoverTank::OnEnemyChange ( idEntity* oldEnemy ) {
	idAI::OnEnemyChange ( oldEnemy );
	
	if ( !enemy.ent ) {
		return;
	}

	if ( enemy.ent->IsType ( rvVehicle::GetClassType() ) ) {
		weaponStateIdeal = WEAPONSTATE_ROCKET;
	}
}

/*
================
rvMonsterHeavyHoverTank::GetIdleAnimName
================
*/
/*
const char* rvMonsterHeavyHoverTank::GetIdleAnimName ( void ) {
	if ( weaponStateCurrent == WEAPONSTATE_ROCKET ) {
		return "rocket_idle";
	}
	
	return idAI::GetIdleAnimName ( );
}	
*/

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterHeavyHoverTank )
	STATE ( "Torso_BlasterAttack",		rvMonsterHeavyHoverTank::State_Torso_BlasterAttack )
	STATE ( "Torso_RocketAttack",		rvMonsterHeavyHoverTank::State_Torso_RocketAttack )
	STATE ( "Torso_ChangeWeaponState",	rvMonsterHeavyHoverTank::State_Torso_ChangeWeaponState )
	STATE ( "Torso_EvadeLeft",			rvMonsterHeavyHoverTank::State_Torso_EvadeLeft )
	STATE ( "Torso_EvadeRight",			rvMonsterHeavyHoverTank::State_Torso_EvadeRight )
	STATE ( "Torso_Strafe",				rvMonsterHeavyHoverTank::State_Torso_Strafe )
END_CLASS_STATES

/*
================
rvMonsterHeavyHoverTank::State_Torso_BlasterAttack
================
*/
stateResult_t rvMonsterHeavyHoverTank::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			weaponStateIdeal = WEAPONSTATE_BLASTER;
			if ( weaponStateIdeal != weaponStateCurrent  ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_ChangeWeaponState", parms.blendFrames );				
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_BlasterAttack", parms.blendFrames );				
				return SRESULT_DONE;
			}
			shots = gameLocal.random.RandomInt ( 8 ) + 4;
			blasterAnimIndex = (blasterAnimIndex + 1) % 2;
			PlayAnim ( ANIMCHANNEL_TORSO, va("blaster_%d_preshoot", blasterAnimIndex + 1 ), parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, va("blaster_%d_fire", blasterAnimIndex + 1 ), 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 ) {
					PlayAnim ( ANIMCHANNEL_TORSO, va("blaster_%d_postshoot", blasterAnimIndex + 1 ), 0 );
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
rvMonsterHeavyHoverTank::State_Torso_RocketAttack
================
*/
stateResult_t rvMonsterHeavyHoverTank::State_Torso_RocketAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			weaponStateIdeal = WEAPONSTATE_ROCKET;
			if ( weaponStateIdeal != weaponStateCurrent  ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_ChangeWeaponState", parms.blendFrames );				
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_RocketAttack", parms.blendFrames );				
				return SRESULT_DONE;
			}
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}		

	return SRESULT_ERROR;
}

/*
================
rvMonsterHeavyHoverTank::State_Torso_ChangeWeaponState
================
*/
stateResult_t rvMonsterHeavyHoverTank::State_Torso_ChangeWeaponState ( const stateParms_t& parms ) {
	static const char* stateAnims [ WEAPONSTATE_MAX ] [ WEAPONSTATE_MAX ] = {
		{ NULL, "blaster_to_rocket" },			// WEAPONSTATE_BLASTER
		{ "rocket_to_blaster", NULL },			// WEAPONSTATE_ROCKET
	};
		
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			// No anim for that transition?
			if ( stateAnims [ weaponStateCurrent ] [ weaponStateIdeal ] == NULL ) {
				return SRESULT_DONE;
			}	
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, stateAnims [ weaponStateCurrent ] [ weaponStateIdeal ], parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				weaponStateCurrent = weaponStateIdeal;
				switch ( weaponStateCurrent ) {
					case WEAPONSTATE_ROCKET:
						animPrefix = "rocket";
						break;
					
					default:
						animPrefix = "";
						break;
				}
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t rvMonsterHeavyHoverTank::State_Torso_EvadeLeft ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				idVec3 vel = GetPhysics()->GetLinearVelocity();
				vel += viewAxis[1] * strafeSpeed;
				physicsObj.UseVelocityMove( true );
				GetPhysics()->SetLinearVelocity( vel );
				PlayAnim ( ANIMCHANNEL_TORSO, "evade_left", parms.blendFrames );
			}
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t rvMonsterHeavyHoverTank::State_Torso_EvadeRight ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				idVec3 vel = GetPhysics()->GetLinearVelocity();
				vel += viewAxis[1] * -strafeSpeed;
				physicsObj.UseVelocityMove( true );
				GetPhysics()->SetLinearVelocity( vel );
				PlayAnim ( ANIMCHANNEL_TORSO, "evade_right", parms.blendFrames );
			}
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

stateResult_t rvMonsterHeavyHoverTank::State_Torso_Strafe ( const stateParms_t& parms ) {
	//fixme: trace first for visibility & obstruction?
	if ( gameLocal.random.RandomFloat() > 0.5f ) {
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_EvadeRight", parms.blendFrames );
	} else {
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_EvadeLeft", parms.blendFrames );
	}
	return SRESULT_DONE;
}
