
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterGunner : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterGunner );

	rvMonsterGunner ( void );

	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo					( debugInfoProc_t proc, void* userData );

	virtual bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

protected:

	int					shots;
	int					shotsFired;
	idStr				nailgunPrefix;
	int					nailgunMinShots;
	int					nailgunMaxShots;
	int					nextShootTime;
	int					attackRate;
	jointHandle_t		attackJoint;

	virtual void		OnStopMoving					( aiMoveCommand_t oldMoveCommand );
	virtual bool		UpdateRunStatus					( void );

	virtual int			FilterTactical					( int availableTactical );

	virtual bool		CheckActions					( void );
	virtual void		OnTacticalChange				( aiTactical_t oldTactical );

private:

	// Actions
	rvAIAction			actionGrenadeAttack;
	rvAIAction			actionNailgunAttack;

	rvAIAction			actionSideStepLeft;
	rvAIAction			actionSideStepRight;
	rvAIActionTimer		actionTimerSideStep;

	bool				CheckAction_SideStepLeft		( rvAIAction* action, int animNum );
	bool				CheckAction_SideStepRight		( rvAIAction* action, int animNum );

	// Torso States
	stateResult_t		State_Torso_NailgunAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_MovingRangedAttack	( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterGunner );
};

CLASS_DECLARATION( idAI, rvMonsterGunner )
END_CLASS

/*
================
rvMonsterGunner::rvMonsterGunner
================
*/
rvMonsterGunner::rvMonsterGunner ( ) {
	nextShootTime = 0;
}


void rvMonsterGunner::InitSpawnArgsVariables( void )
{
	nailgunMinShots = spawnArgs.GetInt ( "action_nailgunAttack_minshots", "5" );
	nailgunMaxShots = spawnArgs.GetInt ( "action_nailgunAttack_maxshots", "20" );
	attackRate = SEC2MS( spawnArgs.GetFloat( "attackRate", "0.3" ) );
	attackJoint = animator.GetJointHandle( spawnArgs.GetString( "attackJoint", "muzzle" ) );
}
/*
================
rvMonsterGunner::Spawn
================
*/
void rvMonsterGunner::Spawn ( void ) {
	actionGrenadeAttack.Init ( spawnArgs, "action_grenadeAttack", NULL, AIACTIONF_ATTACK );
	actionNailgunAttack.Init ( spawnArgs, "action_nailgunAttack", "Torso_NailgunAttack", AIACTIONF_ATTACK );
	actionSideStepLeft.Init ( spawnArgs, "action_sideStepLeft", NULL, 0 );
	actionSideStepRight.Init ( spawnArgs, "action_sideStepRight", NULL, 0 );
	actionTimerSideStep.Init ( spawnArgs, "actionTimer_sideStep" );
	
	InitSpawnArgsVariables();
}

/*
================
rvMonsterGunner::Save
================
*/
void rvMonsterGunner::Save ( idSaveGame *savefile ) const {
	actionGrenadeAttack.Save ( savefile );
	actionNailgunAttack.Save ( savefile );
	actionSideStepLeft.Save ( savefile );
	actionSideStepRight.Save ( savefile );
	actionTimerSideStep.Save ( savefile );
	
	savefile->WriteInt ( shots );
	savefile->WriteInt ( shotsFired );
	savefile->WriteString ( nailgunPrefix );
	savefile->WriteInt ( nextShootTime );
}

/*
================
rvMonsterGunner::Restore
================
*/
void rvMonsterGunner::Restore ( idRestoreGame *savefile ) {
	actionGrenadeAttack.Restore ( savefile );
	actionNailgunAttack.Restore ( savefile );
	actionSideStepLeft.Restore ( savefile );
	actionSideStepRight.Restore ( savefile );
	actionTimerSideStep.Restore ( savefile );
	
	savefile->ReadInt ( shots );
	savefile->ReadInt ( shotsFired );
	savefile->ReadString ( nailgunPrefix );
	savefile->ReadInt ( nextShootTime );

	InitSpawnArgsVariables();
}

/*
============
rvMonsterGunner::OnStopMoving
============
*/
void rvMonsterGunner::OnStopMoving ( aiMoveCommand_t oldMoveCommand ) {
	//MCG - once you get to your position, attack immediately (no pause)
	//FIXME: Restrict this some?  Not after animmoves?  Not if move was short?  Only in certain tactical states?
	if ( GetEnemy() )
	{
		if ( combat.tacticalCurrent == AITACTICAL_HIDE )
		{//hiding
		}
		else if ( combat.tacticalCurrent == AITACTICAL_MELEE || enemy.range <= combat.meleeRange )
		{//in melee state or in melee range
			actionMeleeAttack.timer.Clear( actionTime );
		}
		else if ( (!actionNailgunAttack.timer.IsDone(actionTime) || !actionTimerRangedAttack.IsDone(actionTime))
			&& (!actionGrenadeAttack.timer.IsDone(actionTime) || !actionTimerSpecialAttack.IsDone(actionTime)) )
		{//no attack is ready
			//Ready at least one of them
			if ( gameLocal.random.RandomInt(3) )
			{
				actionNailgunAttack.timer.Clear( actionTime );
				actionTimerRangedAttack.Clear( actionTime );
			}
			else
			{
				actionGrenadeAttack.timer.Clear( actionTime );
				actionTimerSpecialAttack.Clear( actionTime );
			}
		}
	}
}

/*
================
rvMonsterGunner::UpdateRunStatus
================
*/
bool rvMonsterGunner::UpdateRunStatus ( void ) {
	move.fl.idealRunning = false;

	return move.fl.running != move.fl.idealRunning;
}

/*
================
rvMonsterGunner::FilterTactical
================
*/
int rvMonsterGunner::FilterTactical ( int availableTactical ) {
	if ( !move.fl.moving && enemy.range > combat.meleeRange )
	{//keep moving!
		if ( (!actionNailgunAttack.timer.IsDone(actionTime+500) || !actionTimerRangedAttack.IsDone(actionTime+500))
			&& (!actionGrenadeAttack.timer.IsDone(actionTime+500) || !actionTimerSpecialAttack.IsDone(actionTime+500)) )
		{//won't be attacking in the next 1 second
			combat.tacticalUpdateTime = 0;
			availableTactical |= (AITACTICAL_MELEE_BIT);
			if ( !gameLocal.random.RandomInt(2) )
			{
				availableTactical &= ~(AITACTICAL_RANGED_BITS);
			}
		}
	}
	
	return idAI::FilterTactical ( availableTactical );
}

/*
================
rvMonsterGunner::OnTacticalChange

Enable/Disable the ranged attack based on whether the grunt needs it
================
*/
void rvMonsterGunner::OnTacticalChange ( aiTactical_t oldTactical ) {
	switch ( combat.tacticalCurrent ) {
		case AITACTICAL_MELEE:
			//walk for at least 2 seconds (default update time of 500 is too short)
			combat.tacticalUpdateTime = gameLocal.GetTime() + 2000 + gameLocal.random.RandomInt(1000);
			break;
	}
}

/*
================
rvMonsterGunner::CheckAction_SideStepLeft
================
*/
bool rvMonsterGunner::CheckAction_SideStepLeft ( rvAIAction* action, int animNum ) {
	if ( animNum == -1 ) {
		return false;
	}
	idVec3 moveVec;
	TestAnimMove( animNum, NULL, &moveVec );
	//NOTE: should we care if we can't walk all the way to the left?
	int attAnimNum = -1;
	if ( actionNailgunAttack.anims.Num ( ) ) {
		// Pick a random animation from the list
		attAnimNum = GetAnim ( ANIMCHANNEL_TORSO, actionNailgunAttack.anims[gameLocal.random.RandomInt(actionNailgunAttack.anims.Num())] );
	}
	if ( attAnimNum != -1 && !CanHitEnemyFromAnim( attAnimNum, moveVec ) ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterGunner::CheckAction_SideStepRight
================
*/
bool rvMonsterGunner::CheckAction_SideStepRight ( rvAIAction* action, int animNum ) {
	if ( animNum == -1 ) {
		return false;
	}
	idVec3 moveVec;
	TestAnimMove ( animNum, NULL, &moveVec );
	//NOTE: should we care if we can't walk all the way to the right?
	int attAnimNum = -1;
	if ( actionNailgunAttack.anims.Num ( ) ) {
		// Pick a random animation from the list
		attAnimNum = GetAnim ( ANIMCHANNEL_TORSO, actionNailgunAttack.anims[gameLocal.random.RandomInt(actionNailgunAttack.anims.Num())] );
	}
	if ( attAnimNum != -1 && !CanHitEnemyFromAnim( attAnimNum, moveVec ) ) {
		return false;
	}
	return true;
}

/*
================
rvMonsterGunner::CheckActions
================
*/
bool rvMonsterGunner::CheckActions ( void ) {
	// Fire a grenade?
	if ( PerformAction ( &actionGrenadeAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ||
		 PerformAction ( &actionNailgunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack )     )  {
		return true;
	}

	bool action = idAI::CheckActions( );
	if ( !action ) {
		//try a strafe
		if ( GetEnemy() && enemy.fl.visible && gameLocal.GetTime()-lastAttackTime > actionTimerRangedAttack.GetRate()+1000 ) {
			//we can see our enemy but haven't been able to shoot him in a while...
			if ( PerformAction ( &actionSideStepLeft, (checkAction_t)&rvMonsterGunner::CheckAction_SideStepLeft, &actionTimerSideStep )
				|| PerformAction ( &actionSideStepRight, (checkAction_t)&rvMonsterGunner::CheckAction_SideStepRight, &actionTimerSideStep ) ) {
				return true;
			}
		}
	}
	return action;
}

/*
=====================
rvMonsterGunner::GetDebugInfo
=====================
*/
void rvMonsterGunner::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "idAI", "action_grenadeAttack",		aiActionStatusString[actionGrenadeAttack.status], userData );
	proc ( "idAI", "action_nailgunAttack",		aiActionStatusString[actionNailgunAttack.status], userData );
	proc ( "idAI", "actionSideStepLeft",		aiActionStatusString[actionSideStepLeft.status], userData );
	proc ( "idAI", "actionSideStepRight",		aiActionStatusString[actionSideStepRight.status], userData );
}

bool rvMonsterGunner::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	actionTimerRangedAttack.Clear( actionTime );
	actionNailgunAttack.timer.Clear( actionTime );
	return (idAI::Pain( inflictor, attacker, damage, dir, location ));
}
/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterGunner )
	STATE ( "Torso_NailgunAttack",		rvMonsterGunner::State_Torso_NailgunAttack )
	STATE ( "Torso_MovingRangedAttack",	rvMonsterGunner::State_Torso_MovingRangedAttack )
END_CLASS_STATES

/*
================
rvMonsterGunner::State_Torso_MovingRangedAttack
================
*/
stateResult_t rvMonsterGunner::State_Torso_MovingRangedAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = (gameLocal.random.RandomInt ( nailgunMaxShots - nailgunMinShots ) + nailgunMinShots) * combat.aggressiveScale;
			shotsFired = 0;
			return SRESULT_STAGE ( STAGE_SHOOT );
			
		case STAGE_SHOOT:
			shots--;
			shotsFired++;
			nextShootTime = gameLocal.GetTime() + attackRate;
			if ( attackJoint != INVALID_JOINT ) {
				Attack( "nail", attackJoint, GetEnemy() );
				PlayEffect( "fx_nail_flash", attackJoint );
			}
			StartSound( "snd_nailgun_fire", SND_CHANNEL_WEAPON, 0, false, 0 );
			/*
			switch ( move.currentDirection )
			{
			case MOVEDIR_RIGHT:
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_right", 0 );
				break;
			case MOVEDIR_LEFT:
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_left", 0 );
				break;
			case MOVEDIR_BACKWARD:
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso_back", 0 );
				break;
			default:
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso", 0 );
				break;
			}
			*/
			return SRESULT_STAGE ( STAGE_SHOOT_WAIT );
		
		case STAGE_SHOOT_WAIT:
			// When the shoot animation is done either play another shot animation
			// or finish up with post_shooting
			if ( gameLocal.GetTime() >= nextShootTime ) {
				if ( shots <= 0 || (!enemy.fl.inFov && shotsFired >= nailgunMinShots) || !move.fl.moving ) {
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_SHOOT);
			}
			return SRESULT_WAIT;	
	}
	return SRESULT_ERROR; 
}


/*
================
rvMonsterGunner::State_Torso_NailgunAttack
================
*/
stateResult_t rvMonsterGunner::State_Torso_NailgunAttack ( const stateParms_t& parms ) {
	static const char* nailgunAnims [ ] = { "nailgun_short", "nailgun_long" };
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			// If moving switch to the moving ranged attack (torso only)
			if ( move.fl.moving && !actionNailgunAttack.fl.overrideLegs && FacingIdeal() && !gameLocal.random.RandomInt(1) ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_MovingRangedAttack", parms.blendFrames );
				return SRESULT_DONE;
			}

			shots = (gameLocal.random.RandomInt ( nailgunMaxShots - nailgunMinShots ) + nailgunMinShots) * combat.aggressiveScale;
			DisableAnimState ( ANIMCHANNEL_LEGS );
			shotsFired = 0;
			nailgunPrefix = nailgunAnims[shots%2];
			if ( !CanHitEnemyFromAnim( GetAnim( ANIMCHANNEL_TORSO, va("%s_loop", nailgunPrefix.c_str() ) ) ) )
			{//this is hacky, but we really need to test the attack anim first since they're so different
				//can't hit with this one, just use the other one...
				nailgunPrefix = nailgunAnims[(shots+1)%2];
			}
			PlayAnim ( ANIMCHANNEL_TORSO, va("%s_start", nailgunPrefix.c_str() ), parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, va("%s_loop", nailgunPrefix.c_str() ), 0 );
			shotsFired++;
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 || (shotsFired >= nailgunMinShots && !enemy.fl.inFov) || aifl.damage ) {
					PlayAnim ( ANIMCHANNEL_TORSO, va("%s_end", nailgunPrefix.c_str() ), 0 );
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
