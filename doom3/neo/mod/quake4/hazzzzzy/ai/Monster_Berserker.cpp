
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

extern const char* aiActionStatusString [ rvAIAction::STATUS_MAX ];

class rvMonsterBerserker : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterBerserker );

	rvMonsterBerserker ( void );

	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo					( debugInfoProc_t proc, void* userData );

	virtual bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
protected:

	virtual bool		CheckPainActions				( void );
	virtual bool		CheckActions					( void );
	int					FilterTactical					( int availableTactical );
	virtual void		OnTacticalChange				( aiTactical_t oldTactical );

private:

	int					standingMeleeNoAttackTime;
	int					painConsecutive;

	// Actions
	rvAIAction			actionPopupAttack;
	rvAIAction			actionChargeAttack;

	bool				Berz_CanHitEnemyFromAnim		( int animNum );
	bool				CheckAction_RangedAttack		( rvAIAction* action, int animNum );
	bool				CheckAction_ChargeAttack		( rvAIAction* action, int animNum );

	// Global States
	stateResult_t		State_Killed					( const stateParms_t& parms );

	// Torso States
	stateResult_t		State_Torso_Pain				( const stateParms_t& parms );
	stateResult_t		State_Torso_ChargeAttack		( const stateParms_t& parms );

	// Frame commands
	stateResult_t		Frame_ChargeGroundImpact		( const stateParms_t& parms );
	stateResult_t		Frame_DoBlastAttack				( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterBerserker );
};

CLASS_DECLARATION( idAI, rvMonsterBerserker )
END_CLASS

/*
================
rvMonsterBerserker::rvMonsterBerserker
================
*/
rvMonsterBerserker::rvMonsterBerserker ( ) {
	painConsecutive	= 0;
	standingMeleeNoAttackTime = 0;
}

/*
================
rvMonsterBerserker::Spawn
================
*/
void rvMonsterBerserker::Spawn ( void ) {
	actionPopupAttack.Init  ( spawnArgs, "action_popupAttack",	NULL,					AIACTIONF_ATTACK );
	actionChargeAttack.Init ( spawnArgs, "action_chargeAttack", "Torso_ChargeAttack",	AIACTIONF_ATTACK );
	PlayEffect( "fx_ambient_electricity", animator.GetJointHandle( "r_Lowerarm_Real" ), true );
	PlayEffect( "fx_ambient_electricity_mace", animator.GetJointHandle( "chain9" ), true );
}

/*
================
rvMonsterBerserker::Save
================
*/
void rvMonsterBerserker::Save ( idSaveGame *savefile ) const {
	actionPopupAttack.Save ( savefile );
	actionChargeAttack.Save ( savefile );
	savefile->WriteInt( painConsecutive );
	savefile->WriteInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterBerserker::Restore
================
*/
void rvMonsterBerserker::Restore ( idRestoreGame *savefile ) {
	actionPopupAttack.Restore ( savefile );
	actionChargeAttack.Restore ( savefile );
	savefile->ReadInt( painConsecutive );
	savefile->ReadInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterBerserker::CheckAction_ChargeAttack
================
*/
bool rvMonsterBerserker::CheckAction_ChargeAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( GetEnemy() && GetEnemy()->GetPhysics()->GetOrigin().z > GetPhysics()->GetOrigin().z + 24.0f )
	{//this is a ground attack and enemy is above me, so don't even try it, stupid!
		return false;
	}
	if ( !IsEnemyRecentlyVisible ( ) || enemy.ent->DistanceTo ( enemy.lastKnownPosition ) > 128.0f ) {
		return false;
	}
	if ( animNum != -1 && !CanHitEnemyFromAnim( animNum ) ) {
		return false;
	}
	return true;
}

/*
============
rvMonsterBerserker::CanHitEnemyFromAnim
============
*/
bool rvMonsterBerserker::Berz_CanHitEnemyFromAnim( int animNum ) {
	idVec3		dir;
	idVec3		local_dir;
	idVec3		fromPos;
	idMat3		axis;
	idVec3		start;
	idEntity*	enemyEnt;

	// Need an enemy.
	if ( !enemy.ent ) {
		return false;
	}

	// Enemy actor pointer
	enemyEnt = static_cast<idEntity*>(enemy.ent.GetEntity());

	// just do a ray test if close enough
	if ( enemyEnt->GetPhysics()->GetAbsBounds().IntersectsBounds( physicsObj.GetAbsBounds().Expand( 16.0f ) ) ) {
		return CanHitEnemy();
	}

	// calculate the world transform of the launch position
  	idVec3 org = physicsObj.GetOrigin();
	idVec3 from;
  	dir = enemy.lastVisibleChestPosition - org;
  	physicsObj.GetGravityAxis().ProjectVector( dir, local_dir );
  	local_dir.z = 0.0f;
  	local_dir.ToVec2().Normalize();
  	axis = local_dir.ToMat3();
  	from = org + attackAnimInfo[ animNum ].attackOffset * axis;

	return CanSeeFrom ( from, enemy.lastVisibleEyePosition, true );
}

/*
================
rvMonsterBerserker::CheckAction_RangedAttack
================
*/
bool rvMonsterBerserker::CheckAction_RangedAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( !IsEnemyRecentlyVisible ( ) || enemy.ent->DistanceTo ( enemy.lastKnownPosition ) > 128.0f ) {
		return false;
	}
	if ( animNum != -1 && !Berz_CanHitEnemyFromAnim( animNum ) ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterBerserker::CheckActions
================
*/
bool rvMonsterBerserker::CheckActions ( void ) {
	// Pop-up attack is a forward moving melee attack that throws the enemy up in the air
	if ( PerformAction ( &actionPopupAttack, (checkAction_t)&idAI::CheckAction_LeapAttack, &actionTimerSpecialAttack ) ) {
		return true;
	}

	// Charge attack is where the berserker will charge up his spike and slam it in to the ground
	if ( PerformAction ( &actionChargeAttack, (checkAction_t)&rvMonsterBerserker::CheckAction_ChargeAttack, &actionTimerSpecialAttack ) ) {
		return true;
	}

	if ( CheckPainActions ( ) ) {
		return true;
	}

	if ( PerformAction ( &actionEvadeLeft,   (checkAction_t)&idAI::CheckAction_EvadeLeft, &actionTimerEvade )			 ||
			PerformAction ( &actionEvadeRight,  (checkAction_t)&idAI::CheckAction_EvadeRight, &actionTimerEvade )			 ||
			PerformAction ( &actionJumpBack,	 (checkAction_t)&idAI::CheckAction_JumpBack, &actionTimerEvade )			 ||
			PerformAction ( &actionLeapAttack,  (checkAction_t)&idAI::CheckAction_LeapAttack )	) {
		return true;
	} else if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack ) ) {
		standingMeleeNoAttackTime = 0;
		return true;
	} else {
		if ( actionMeleeAttack.status != rvAIAction::STATUS_FAIL_TIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_EXTERNALTIMER
			&& actionMeleeAttack.status != rvAIAction::STATUS_FAIL_CHANCE )
		{//melee attack fail for any reason other than timer?
			if ( combat.tacticalCurrent == AITACTICAL_MELEE && !move.fl.moving )
			{//special case: we're in tactical melee and we're close enough to think we've reached the enemy, but he's just out of melee range!
				//allow ranged attack
				if ( !standingMeleeNoAttackTime )
				{
					standingMeleeNoAttackTime = gameLocal.GetTime();
				}
				else if ( standingMeleeNoAttackTime + 2500 < gameLocal.GetTime() )
				{//we've been standing still and not attacking for at least 2.5 seconds, fall back to ranged attack
					actionRangedAttack.fl.disabled = false;
				}
			}
		}
		if ( PerformAction ( &actionRangedAttack,(checkAction_t)&rvMonsterBerserker::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	}
	return false;
}
/*
================
rvMonsterBerserker::FilterTactical
================
*/
int rvMonsterBerserker::FilterTactical ( int availableTactical ) {
	if ( move.moveCommand == MOVE_TO_ENEMY && move.moveStatus == MOVE_STATUS_DEST_UNREACHABLE ) {
		availableTactical |= AITACTICAL_RANGED_BIT;
	} else if ( combat.tacticalCurrent != AITACTICAL_RANGED 
		&& combat.tacticalCurrent != AITACTICAL_MELEE 
		&& (combat.tacticalMaskAvailable&AITACTICAL_RANGED_BIT) ) {
		availableTactical |= AITACTICAL_RANGED_BIT;
	} else {
		availableTactical &= ~AITACTICAL_RANGED_BIT;
	}
	
	return idAI::FilterTactical ( availableTactical );
}

/*
================
rvMonsterBerserker::OnTacticalChange

Enable/Disable the ranged attack based on whether the berzerker needs it
================
*/
void rvMonsterBerserker::OnTacticalChange ( aiTactical_t oldTactical ) {
	switch ( combat.tacticalCurrent ) {
		case AITACTICAL_MELEE:
			actionRangedAttack.fl.disabled = true;
			//once you've gone into melee once, it's okay to try ranged attacks later
			combat.tacticalMaskAvailable |= AITACTICAL_RANGED_BIT;
			break;

		default:
			actionRangedAttack.fl.disabled = false;
			break;
	}
}

/*
=====================
rvMonsterBerserker::GetDebugInfo
=====================
*/
void rvMonsterBerserker::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "action_popupAttack",		aiActionStatusString[actionPopupAttack.status], userData );
	proc ( "idAI", "action_chargeAttack",		aiActionStatusString[actionChargeAttack.status], userData );
}

/*
================
rvMonsterBerserker::Pain
================
*/
bool rvMonsterBerserker::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( pain.lastTakenTime > gameLocal.GetTime() - 500 ) {
		painConsecutive++;
	} else {
		painConsecutive = 1;
	}
	return ( idAI::Pain( inflictor, attacker, damage, dir, location ) );
}

/*
================
rvMonsterBerserker::CheckPainActions
================
*/
bool rvMonsterBerserker::CheckPainActions ( void ) {
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
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterBerserker )
	STATE ( "State_Killed",					rvMonsterBerserker::State_Killed )

	STATE ( "Torso_ChargeAttack",			rvMonsterBerserker::State_Torso_ChargeAttack )
	STATE ( "Torso_Pain",					rvMonsterBerserker::State_Torso_Pain )
	
	STATE ( "Frame_ChargeGroundImpact",		rvMonsterBerserker::Frame_ChargeGroundImpact )
	STATE ( "Frame_DoBlastAttack",			rvMonsterBerserker::Frame_DoBlastAttack )
END_CLASS_STATES

/*
================
rvMonsterBerserker::State_Torso_ChargeAttack
================
*/
stateResult_t rvMonsterBerserker::State_Torso_ChargeAttack ( const stateParms_t& parms ) {
	enum { 
		TORSO_CHARGEATTACK_INIT,
		TORSO_CHARGEATTACK_WAIT,
		TORSO_CHARGEATTACK_RECOVER,
		TORSO_CHARGEATTACK_RECOVERWAIT
	};

	switch ( parms.stage ) {
		// Start the charge attack animation
		case TORSO_CHARGEATTACK_INIT:			
			// Full body animations
			DisableAnimState ( ANIMCHANNEL_LEGS );

			// Play the ground strike
			PlayAnim ( ANIMCHANNEL_TORSO, "ground_strike", parms.blendFrames );	
			return SRESULT_STAGE ( TORSO_CHARGEATTACK_WAIT );
			
		// Wait for charge attack animation to finish
		case TORSO_CHARGEATTACK_WAIT:
			if ( AnimDone ( ANIMCHANNEL_LEGS, 0 ) ) {
				return SRESULT_STAGE ( TORSO_CHARGEATTACK_RECOVER );
			}
			return SRESULT_WAIT;
	
		// Play recover animation
		case TORSO_CHARGEATTACK_RECOVER:
			PlayAnim ( ANIMCHANNEL_TORSO, "ground_strike_recover", parms.blendFrames );
			return SRESULT_STAGE ( TORSO_CHARGEATTACK_RECOVERWAIT );

		// Wait for recover animation to finish
		case TORSO_CHARGEATTACK_RECOVERWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 2 ) ) {
				return SRESULT_DONE;			
			}
			return SRESULT_WAIT;
	}	
	return SRESULT_ERROR;
}

/*
================
rvMonsterBerserker::State_Torso_Pain
================
*/
stateResult_t rvMonsterBerserker::State_Torso_Pain ( const stateParms_t& parms ) {
	StopEffect ( "fx_charge_up" );
	return idAI::State_Torso_Pain ( parms );
}

/*
================
rvMonsterBerserker::State_Killed
================
*/
stateResult_t rvMonsterBerserker::State_Killed	( const stateParms_t& parms ) {
	StopEffect ( "fx_charge_up" );
	StopEffect ( "fx_ambient_electricity" );
	StopEffect ( "fx_ambient_electricity_mace" );
	return idAI::State_Killed ( parms );
}

/*
================
rvMonsterBerserker::Frame_ChargeGroundImpact
================
*/
stateResult_t rvMonsterBerserker::Frame_ChargeGroundImpact ( const stateParms_t& parms ) {
	idVec3			start;
	idVec3			end;
	idMat3			axis;
	trace_t			tr;

	GetJointWorldTransform ( animator.GetJointHandle ( "R_lowerArm_Real" ), gameLocal.time, start, axis );
	
	end    = start;
	end.z -= 128;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	gameLocal.TracePoint ( this, tr, start, end, MASK_SHOT_BOUNDINGBOX, this );
// RAVEN END
	
	gameLocal.PlayEffect ( gameLocal.GetEffect( spawnArgs, "fx_ground_impact" ), tr.endpos, idVec3(0,0,1).ToMat3() ); 
	
	return SRESULT_OK;
}

/*
================
rvMonsterBerserker::Frame_DoBlastAttack
================
*/
stateResult_t rvMonsterBerserker::Frame_DoBlastAttack ( const stateParms_t& parms ) {
	float			i;
	idVec3			start;
	idMat3			axis;
	idAngles		angles ( 0.0f, move.current_yaw, 0.0f );
	const idDict*	blastDict;

	blastDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_attack_spike" ), false );
	if ( !blastDict ) {
		gameLocal.Error ( "missing projectile on spike attack for AI entity '%s'", GetName ( ) ) ;
		return SRESULT_OK;
	}

	GetJointWorldTransform ( animator.GetJointHandle ( "end_spike" ), gameLocal.time, start, axis );

	for( i = 0; i < 32; i++ ) {
		angles.yaw += (360.0f / 32.0f);
		AttackProjectile ( blastDict, start, angles );
	}
	
	return SRESULT_OK;
}
