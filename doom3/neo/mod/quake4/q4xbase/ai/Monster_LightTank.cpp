
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterLightTank : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterLightTank );

	rvMonsterLightTank ( void );

	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	virtual bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual int			GetDamageForLocation			( int damage, int location );
	virtual void		DamageFeedback					( idEntity *victim, idEntity *inflictor, int &damage );

protected:

	virtual void		OnStopMoving					( aiMoveCommand_t oldMoveCommand );

	virtual bool		CheckActions					( void );
	virtual void		OnTacticalChange				( aiTactical_t oldTactical );

	virtual bool		UpdateRunStatus					( void );

	virtual int			FilterTactical					( int availableTactical );

	int					flamethrowerHealth;
	int					chargeDebounce;
	void				DestroyFlamethrower				( void );

private:

	int					standingMeleeNoAttackTime;
	bool				damaged;
//	bool				damagedMove;
	int					powerUpStartTime;

	rvAIAction			actionFlameThrower;
	rvAIAction			actionPowerUp;
	rvAIAction			actionChargeAttack;

	bool				CheckAction_PowerUp				( rvAIAction* action, int animNum );
	virtual bool		CheckAction_EvadeLeft			( rvAIAction* action, int animNum );
	virtual bool		CheckAction_EvadeRight			( rvAIAction* action, int animNum );
	bool				CheckAction_ChargeAttack		( rvAIAction* action, int animNum );

	// Global States
	stateResult_t		State_Killed					( const stateParms_t& parms );
	
	// Torso States
	stateResult_t		State_Torso_FlameThrower		( const stateParms_t& parms );
	stateResult_t		State_Torso_FlameThrowerThink	( const stateParms_t& parms );
	stateResult_t		State_Torso_Pain				( const stateParms_t& parms );
	stateResult_t		State_Torso_RangedAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_PowerUp				( const stateParms_t& parms );

	rvScriptFuncUtility		mPostWeaponDestroyed;		// script to run after flamethrower is destroyed

	CLASS_STATES_PROTOTYPE ( rvMonsterLightTank );
};

CLASS_DECLARATION( idAI, rvMonsterLightTank )
END_CLASS

/*
================
rvMonsterLightTank::rvMonsterLightTank
================
*/
rvMonsterLightTank::rvMonsterLightTank ( void ) {
	damaged = false;
	standingMeleeNoAttackTime = 0;
}

/*
================
rvMonsterLightTank::Spawn
================
*/
void rvMonsterLightTank::Spawn ( void ) {
//	damagedThreshold = spawnArgs.GetInt ( "health_damagedThreshold" );
	flamethrowerHealth	= spawnArgs.GetInt ( "flamethrowerHealth", "160" );
	chargeDebounce = 0;
	
	actionFlameThrower.Init ( spawnArgs, "action_flameThrower", "Torso_FlameThrower", AIACTIONF_ATTACK );
	actionPowerUp.Init ( spawnArgs, "action_powerup", "Torso_PowerUp", 0 );
	actionChargeAttack.Init ( spawnArgs, "action_chargeAttack", NULL, AIACTIONF_ATTACK );

	const char  *func;
	if ( spawnArgs.GetString( "script_postWeaponDestroyed", "", &func ) ) 
	{
		mPostWeaponDestroyed.Init( func );
	}
}

/*
================
rvMonsterLightTank::Save
================
*/
void rvMonsterLightTank::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt( flamethrowerHealth );
	savefile->WriteInt( chargeDebounce );
	
	savefile->WriteBool( damaged );
//	savefile->WriteBool( damagedMove );
	savefile->WriteInt( powerUpStartTime );

	actionFlameThrower.Save( savefile );
	actionPowerUp.Save( savefile );
	actionChargeAttack.Save( savefile );
	mPostWeaponDestroyed.Save( savefile );
	savefile->WriteInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterLightTank::Restore
================
*/
void rvMonsterLightTank::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt( flamethrowerHealth );
	savefile->ReadInt( chargeDebounce );

	savefile->ReadBool( damaged );
//	savefile->ReadBool( damagedMove );
	savefile->ReadInt( powerUpStartTime );

	actionFlameThrower.Restore( savefile );
	actionPowerUp.Restore( savefile );
	actionChargeAttack.Restore( savefile );
	mPostWeaponDestroyed.Restore( savefile );
	savefile->ReadInt( standingMeleeNoAttackTime );
}

/*
================
rvMonsterLightTank::FilterTactical
================
*/
int rvMonsterLightTank::FilterTactical ( int availableTactical ) {
	if ( flamethrowerHealth > 0 ) { 
		// Only let the light tank use ranged tactical when he is really far from his enemy
		if ( !enemy.range || enemy.range < combat.attackRange[1] ) {
			availableTactical &= ~(AITACTICAL_RANGED_BITS);
		}
	}
	if ( chargeDebounce > gameLocal.GetTime() )
	{//don't charge again any time soon
		availableTactical &= ~(AITACTICAL_MELEE_BIT);
	}
	
	return idAI::FilterTactical( availableTactical );
}

/*
================
rvMonsterLightTank::OnTacticalChange

Enable/Disable the ranged attack based on whether the grunt needs it
================
*/
void rvMonsterLightTank::OnTacticalChange ( aiTactical_t oldTactical ) {
	switch ( combat.tacticalCurrent ) {
		case AITACTICAL_MELEE:
			actionFlameThrower.fl.disabled = true;
			actionRangedAttack.fl.disabled = true;
			break;

		default:
			actionFlameThrower.fl.disabled = false;
			actionRangedAttack.fl.disabled = false;
			break;
	}
}

/*
================
rvMonsterLightTank::UpdateRunStatus
================
*/
bool rvMonsterLightTank::UpdateRunStatus ( void ) {
	// If rushing, run
	if ( combat.tacticalCurrent == AITACTICAL_MELEE ) 
	{
		move.fl.idealRunning = true;
	}
	else
	{
		move.fl.idealRunning = false;
	}

	return move.fl.running != move.fl.idealRunning;
}

/*
================
rvMonsterLightTank::DestroyFlamethrower
================
*/
void rvMonsterLightTank::DestroyFlamethrower ( void ) {

	StopEffect ( "fx_flame_muzzle" );
	//HideSurface ( "models/monsters/light_tank/flamethrower" );
	//GetAFPhysics()->GetBody ( "b_right_forearm" )->SetClipMask ( 0 );
	animator.CollapseJoint( animator.GetJointHandle( "r_smallShield_nadeLauncher" ), animator.GetJointHandle( "r_elbo" ) );
	animator.CollapseJoint( animator.GetJointHandle( "r_bigShield_nadeLauncher" ), animator.GetJointHandle( "r_elbo" ) );
	animator.CollapseJoint( animator.GetJointHandle( "r_gun_effect" ), animator.GetJointHandle( "r_elbo" ) );
	flamethrowerHealth = -1;
	
	
	pain.takenThisFrame = pain.threshold;
	pain.lastTakenTime = gameLocal.time;
//	flamethrowerDestroyedTime = gameLocal.GetTime();

	// Tweak-out the AI to be more aggressive and more likely to charge?
	
	PlayEffect ( "fx_destroy_arm", animator.GetJointHandle("r_elbo") );
	PlayEffect ( "fx_destroy_arm_trail", animator.GetJointHandle("r_elbo"), true );

	DisableAnimState( ANIMCHANNEL_LEGS );
	painAnim = "damaged";
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Pain" );
	PostAnimState( ANIMCHANNEL_TORSO, "Torso_Idle" );

	chargeDebounce = 0;
	damaged = true;
	animPrefix = "damage";

	actionFlameThrower.fl.disabled = true;
	actionRangedAttack.fl.disabled = true;

	combat.attackRange[1] = 200;
	combat.aggressiveRange = 400;
	
	spawnArgs.SetFloat( "action_meleeAttack_rate", 0.3f );
	actionMeleeAttack.Init( spawnArgs, "action_meleeAttack", NULL, AIACTIONF_ATTACK );
	actionMeleeAttack.failRate = 200;
	actionMeleeAttack.chance = 1.0f;

	combat.tacticalMaskAvailable &= ~(AITACTICAL_RANGED_BITS);

	actionMeleeAttack.timer.Reset( actionTime );
	actionChargeAttack.timer.Reset( actionTime );

	ExecScriptFunction( mPostWeaponDestroyed );
}

/*
=====================
rvMonsterLightTank::Pain
=====================
*/
bool rvMonsterLightTank::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	bool didPain = idAI::Pain( inflictor, attacker, damage, dir, location );
	if ( move.fl.moving && move.fl.running )
	{
		painAnim = "pain_charge";
	}
	return didPain;
}
	/*
=====================
rvMonsterLightTank::GetDamageForLocation
=====================
*/
int rvMonsterLightTank::GetDamageForLocation( int damage, int location ) {
	// If the flamethrower was hit only do damage to it
	if( !damaged && !aifl.dead ) 
	{
		if ( idStr::Icmp ( GetDamageGroup ( location ), "flamethrower" ) == 0 ) {
//			pain.takenThisFrame = damage;
			if ( flamethrowerHealth > 0 ){
				flamethrowerHealth -= damage;
				if ( flamethrowerHealth <= 0 ) {
					DestroyFlamethrower();
				}
			}
			return 0;
		}
	}
	 
	return idAI::GetDamageForLocation ( damage, location );
}

/*
================
rvMonsterLightTank::DamageFeedback

callback function for when another entity recieved damage from this entity
================
*/
void rvMonsterLightTank::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	if ( !damaged )
	{
		if ( victim == GetEnemy() && inflictor == this )
		{
			if ( combat.tacticalCurrent == AITACTICAL_MELEE )
			{//okay, get out of melee state for now
				chargeDebounce = gameLocal.GetTime() + gameLocal.random.RandomInt(3000) + 3000;
			}
		}
	}

	idAI::DamageFeedback( victim, inflictor, damage );
}

/*
============
rvMonsterLightTank::OnStopMoving
============
*/
void rvMonsterLightTank::OnStopMoving ( aiMoveCommand_t oldMoveCommand ) {
	//MCG - once you get to your position, attack immediately (no pause)
	//FIXME: Restrict this some?  Not after animmoves?  Not if move was short?  Only in certain tactical states?
	if ( GetEnemy() )
	{
		if ( combat.tacticalCurrent == AITACTICAL_RANGED )
		{
			actionRangedAttack.timer.Clear( actionTime );
			actionTimerRangedAttack.Clear( actionTime );
			actionFlameThrower.timer.Clear( actionTime );
		}
		else if ( combat.tacticalCurrent == AITACTICAL_MELEE )
		{//so we don't stand there and look stupid
			actionMeleeAttack.timer.Clear( actionTime );
			actionChargeAttack.timer.Clear( actionTime );
		}
	}
}

/*
================
rvMonsterLightTank::CheckAction_PowerUp
================
*/
bool rvMonsterLightTank::CheckAction_PowerUp ( rvAIAction* action, int animNum )
{
	if ( !damaged && combat.tacticalCurrent == AITACTICAL_MELEE )
	{
		return false;
	}
	if ( health > 20 || gameLocal.time - pain.lastTakenTime < 500 ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterLightTank::CheckAction_EvadeLeft
================
*/
bool rvMonsterLightTank::CheckAction_EvadeLeft ( rvAIAction* action, int animNum ) {
	if ( damaged || combat.tacticalCurrent == AITACTICAL_MELEE )
	{
		return false;
	}
	return idAI::CheckAction_EvadeLeft( action, animNum );
}

/*
================
rvMonsterLightTank::CheckAction_EvadeRight
================
*/
bool rvMonsterLightTank::CheckAction_EvadeRight ( rvAIAction* action, int animNum ) {
	if ( damaged || combat.tacticalCurrent == AITACTICAL_MELEE )
	{
		return false;
	}
	return idAI::CheckAction_EvadeRight( action, animNum );
}

/*
================
rvMonsterLightTank::CheckAction_ChargeAttack
================
*/
bool rvMonsterLightTank::CheckAction_ChargeAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( !CheckFOV ( enemy.ent->GetPhysics()->GetOrigin(), 10 ) ) {
		return false;
	}
	if ( damaged || idStr::Icmp( "run", animator.CurrentAnim(ANIMCHANNEL_TORSO)->AnimName() ) ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterLightTank::CheckActions
================
*/
bool rvMonsterLightTank::CheckActions ( void ) {
	if ( PerformAction ( &actionFlameThrower, (checkAction_t)&idAI::CheckAction_RangedAttack ) ) {
		return true;
	}
	if ( PerformAction ( &actionChargeAttack, (checkAction_t)&rvMonsterLightTank::CheckAction_ChargeAttack ) ) {
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
					actionFlameThrower.fl.disabled = false;
					actionRangedAttack.fl.disabled = false;
				}
			}
		}
		if ( PerformAction ( &actionRangedAttack,(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	}
	if ( PerformAction ( &actionPowerUp, (checkAction_t)&rvMonsterLightTank::CheckAction_PowerUp ) ) {
		return true;
	}
	return false;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterLightTank )
	STATE ( "State_Killed",				rvMonsterLightTank::State_Killed )
		
	STATE ( "Torso_FlameThrower",		rvMonsterLightTank::State_Torso_FlameThrower )
	STATE ( "Torso_FlameThrowerThink",	rvMonsterLightTank::State_Torso_FlameThrowerThink )
	STATE ( "Torso_Pain",				rvMonsterLightTank::State_Torso_Pain )
	STATE ( "Torso_RangedAttack",		rvMonsterLightTank::State_Torso_RangedAttack )
	STATE ( "Torso_PowerUp",			rvMonsterLightTank::State_Torso_PowerUp )
	
END_CLASS_STATES

/*
================
rvMonsterLightTank::State_Killed
================
*/
stateResult_t rvMonsterLightTank::State_Killed	( const stateParms_t& parms ) {
	StopEffect ( "fx_destroy_arm_trail" );
	StopEffect ( "fx_flame_muzzle" );
	return idAI::State_Killed ( parms );
}

/*
================
rvMonsterLightTank::State_Torso_FlameThrower
================
*/
stateResult_t rvMonsterLightTank::State_Torso_FlameThrower ( const stateParms_t& parms ) {	
	DisableAnimState ( ANIMCHANNEL_LEGS );

	// Flame effect
	PlayEffect ( "fx_flame_muzzle", animator.GetJointHandle ( "gun_effect" ), true );
	
	// Loop the flame animation
	PlayAnim( ANIMCHANNEL_TORSO, "flamethrower", parms.blendFrames );

	// Delay start the flame thrower think to ensure he flames for a minimum time
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FlameThrowerThink", 0, 500 );	

	return SRESULT_DONE;
}

/*
================
rvMonsterLightTank::State_Torso_FlameThrowerThink
================
*/
stateResult_t rvMonsterLightTank::State_Torso_FlameThrowerThink ( const stateParms_t& parms ) {
	if ( !enemy.fl.inFov || AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
		StopEffect ( "fx_flame_muzzle" );
		return SRESULT_DONE;
	}
	
	return SRESULT_WAIT;
}

/*
================
rvMonsterLightTank::State_Torso_Pain
================
*/
stateResult_t rvMonsterLightTank::State_Torso_Pain ( const stateParms_t& parms ) {

	StopEffect ( "fx_flame_muzzle" );

	// Default pain animation
	return idAI::State_Torso_Pain ( parms );
}

/*
================
rvMonsterLightTank::State_Torso_RangedAttack
================
*/
stateResult_t rvMonsterLightTank::State_Torso_RangedAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_FINISH,
	};
	switch ( parms.stage ) {
		case STAGE_START:
			// If moving switch to the moving ranged attack (torso only)
			if ( !move.fl.moving || !FacingIdeal() ) {
				// Full body animations						
				DisableAnimState ( ANIMCHANNEL_LEGS );
				PlayAnim ( ANIMCHANNEL_TORSO, "range_megaattack", parms.blendFrames );
			}
			else
			{
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso", parms.blendFrames );
			}

			return SRESULT_STAGE ( STAGE_FINISH );

		case STAGE_FINISH:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterLightTank::State_Torso_PowerUp
================
*/
stateResult_t rvMonsterLightTank::State_Torso_PowerUp ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_LOOP,
		STAGE_FINISH,
	};
	switch ( parms.stage ) {
		case STAGE_START:
			// If moving switch to the moving ranged attack (torso only)
			//fl.takedamage = false;
			powerUpStartTime = gameLocal.GetTime();
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "powerup_start", parms.blendFrames );

			return SRESULT_STAGE ( STAGE_START_WAIT );

		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				PlayCycle( ANIMCHANNEL_TORSO, "powerup_loop", parms.blendFrames ); 
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;

		case STAGE_LOOP:
			health++;
			if ( health >= spawnArgs.GetInt( "health" )
				|| gameLocal.GetTime() - powerUpStartTime > 3875 )
			{//full health or been charging up for 3 full anim loops
				PlayAnim ( ANIMCHANNEL_TORSO, "powerup_end", parms.blendFrames );
				return SRESULT_STAGE ( STAGE_FINISH );
			}
			return SRESULT_WAIT;

		case STAGE_FINISH:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}
