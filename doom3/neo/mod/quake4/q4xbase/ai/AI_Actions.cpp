
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

/*
===============================================================================

	rvAIActionTimer

===============================================================================
*/

/*
================
rvAIActionTimer::rvAIActionTimer
================
*/
rvAIActionTimer::rvAIActionTimer ( void ) {
	time = 0;
}

/*
================
rvAIActionTimer::Init
================
*/
bool rvAIActionTimer::Init ( const idDict& args, const char* name ) {
	rate = SEC2MS ( args.GetFloat ( va("%s_rate",name), "0" ) );
	return true;
}

/*
================
rvAIActionTimer::Save
================
*/
void rvAIActionTimer::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( rate );
	savefile->WriteInt ( time );
}

/*
================
rvAIActionTimer::Restore
================
*/
void rvAIActionTimer::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( rate );
	savefile->ReadInt ( time );
}

/*
================
rvAIActionTimer::Reset
================
*/
void rvAIActionTimer::Reset ( int currentTime, float diversity, float scale ) {
	float _rate = rate * scale;
	time = currentTime + (-gameLocal.random.RandomInt( 2.0f * _rate * diversity ) + _rate * diversity + _rate);
}

/*
================
rvAIActionTimer::Clear
================
*/
void rvAIActionTimer::Clear ( int currentTime ) {
	time = currentTime;
}

/*
================
rvAIActionTimer::Add
================
*/
void rvAIActionTimer::Add ( int _time, float diversity ) {
	time += (-gameLocal.random.RandomInt( 2.0f * _time * diversity ) + _time * diversity + _time);
}

/*
===============================================================================

	rvAIAction

===============================================================================
*/

/*
================
rvAIAction::rvAIAction
================
*/
rvAIAction::rvAIAction ( void ) {
	memset ( &fl, 0, sizeof(fl) );
	status = STATUS_UNUSED;
}

/*
================
rvAIAction::Init
================
*/
bool rvAIAction::Init ( const idDict& args, const char* name, const char* defaultState, int _flags ) {
	const idKeyValue* kv;

	if ( _flags & AIACTIONF_ATTACK ) {
		fl.isAttack = true;
	}
	if ( _flags & AIACTIONF_MELEE ) {
		fl.isMelee = true;
	}

	// Initialize timer
	timer.Init ( args, name );

	// Is this action enabled?
	fl.disabled			= !args.GetBool ( va("%s",name), "0" );
	fl.noPain			= args.GetBool ( va("%s_nopain",name), "0" );
	fl.noTurn			= args.GetBool ( va("%s_noturn",name), "1" );
	fl.overrideLegs		= args.GetBool ( va("%s_overrideLegs",name), "1" );
	fl.noSimpleThink	= args.GetBool ( va("%s_nosimplethink",name), "0" );
	
	blendFrames = args.GetInt ( va("%s_blendFrames",name), "4" );
	failRate = SEC2MS ( args.GetInt ( va("%s_failRate",name), ".1" ) );

	minRange = args.GetInt ( va("%s_minRange",name), "0" );
	maxRange = args.GetInt ( va("%s_maxRange",name), (_flags & AIACTIONF_ATTACK ) ? "-1" : "0" );
	minRange2d = args.GetInt ( va("%s_minRange2d",name), "0" );
	maxRange2d = args.GetInt ( va("%s_maxRange2d",name), "0" );

	chance = args.GetFloat ( va("%s_chance",name), "1" );
	diversity = args.GetFloat ( va("%s_diversity", name ), ".5" );
	
	// action state
	state = args.GetString ( va("%s_state",name), (!defaultState||!*defaultState) ? "Torso_Action" : defaultState );

	// allow for multiple animations
	const char* prefix = va("%s_anim",name);
	for ( kv = args.MatchPrefix ( prefix, NULL ); kv; kv = args.MatchPrefix ( prefix, kv ) ) {
		if ( kv->GetValue ( ).Length ( ) ) {
			anims.Append ( kv->GetValue ( ) );
		}
	}
			
	return true;
}

/*
================
rvAIAction::Save
================
*/
void rvAIAction::Save ( idSaveGame *savefile ) const {
	int i;

	savefile->Write( &fl, sizeof( fl ) );

	savefile->WriteInt ( anims.Num ( ) );
	for ( i = 0; i < anims.Num(); i ++ ) {
		savefile->WriteString ( anims[i] );
	}	
	savefile->WriteString ( state );

	timer.Save ( savefile );

	savefile->WriteInt ( blendFrames );
	savefile->WriteInt ( failRate );

	savefile->WriteFloat ( minRange );
	savefile->WriteFloat ( maxRange );
	savefile->WriteFloat ( minRange2d );
	savefile->WriteFloat ( maxRange2d );

	savefile->WriteFloat ( chance );
	savefile->WriteFloat ( diversity );

	savefile->WriteInt ( (int)status );
}

/*
================
rvAIAction::Restore
================
*/
void rvAIAction::Restore ( idRestoreGame *savefile ) {
	int num;

	savefile->Read( &fl, sizeof( fl ) );

	savefile->ReadInt ( num );
	anims.Clear ( );
	anims.SetNum ( num );
	for ( num--; num >= 0; num -- ) {
		savefile->ReadString ( anims[num] );
	}	
	savefile->ReadString ( state );

	timer.Restore ( savefile );

	savefile->ReadInt ( blendFrames );
	savefile->ReadInt ( failRate );

	savefile->ReadFloat ( minRange );
	savefile->ReadFloat ( maxRange );
	savefile->ReadFloat ( minRange2d );
	savefile->ReadFloat ( maxRange2d );

	savefile->ReadFloat ( chance );
	savefile->ReadFloat ( diversity );

	savefile->ReadInt ( (int&)status );
}

/*
===============================================================================

	Actions

===============================================================================
*/

/*
================
idAI::CheckAction_EvadeLeft
================
*/
bool idAI::CheckAction_EvadeLeft ( rvAIAction* action, int animNum ) {
	if( combat.shotAtAngle >= 0 || gameLocal.time - combat.shotAtTime > 100 ) {
		return false;
	}
	// TODO: dont evade unless it was coming from directly in front of us
	if ( animNum != -1 && !TestAnimMove ( animNum ) ) {
		return false;
	}
	return true;
}

/*
================
idAI::CheckAction_EvadeRight
================
*/
bool idAI::CheckAction_EvadeRight ( rvAIAction* action, int animNum ) {
	if( combat.shotAtAngle < 0 || gameLocal.time - combat.shotAtTime > 100 ){
		return false;
	}
	// TODO: Dont eveade unless it was coming from directly in front of us
	if ( animNum != -1 && !TestAnimMove ( animNum ) ) {
		return false;
	}
	return true;
}

/*
================
idAI::CheckAction_JumpBack
================
*/
bool idAI::CheckAction_JumpBack ( rvAIAction* action, int animNum ) {
	// Jump back after taking damage
	if ( !aifl.damage ) {
		return false;
	}
	// TODO: enemy must be in front to jump backwards
	
	// Can we actually move backwards?
	if ( !TestAnimMove ( animNum ) ) {
		return false;
	}
	return true;
}

/*
================
idAI::CheckAction_RangedAttack
================
*/
bool idAI::CheckAction_RangedAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
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
================
idAI::CheckAction_MeleeAttack
================
*/
bool idAI::CheckAction_MeleeAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( !CheckFOV ( enemy.ent->GetPhysics()->GetOrigin(), 10 ) ) {
		return false;
	}
	return true;
}

/*
================
idAI::CheckAction_LeapAttack
================
*/
bool idAI::CheckAction_LeapAttack ( rvAIAction* action, int animNum ) {
	if ( !enemy.ent || !enemy.fl.inFov ) {
		return false;
	}
	if ( enemy.range > 64.0f && !TestAnimMove ( animNum, enemy.ent ) ) {
		return false;
	}
	// Must be looking right at the enemy to leap
	if ( !CheckFOV ( enemy.ent->GetPhysics()->GetOrigin(), 4 ) ) {
		return false;
	}
	return true;
}

/*
================
idAI::UpdateAction
================
*/
bool idAI::UpdateAction ( void ) {
	// Update action MUST be called from the main state loop
	assert ( stateThread.IsExecuting ( ) );

	// If an action is already running then dont let another start
	if ( aifl.action ) {
		return false;
	}
		
	return CheckActions ( );
}

/*
================
idAI::CheckPainActions
================
*/
bool idAI::CheckPainActions ( void ) {
	if ( !pain.takenThisFrame || !actionTimerPain.IsDone ( actionTime ) ) {
		return false;
	}
	
	if ( !pain.threshold || pain.takenThisFrame < pain.threshold ) {
		return false;
	}
	
	PerformAction ( "Torso_Pain", 2, true );
	actionTimerPain.Reset ( actionTime );
	
	return true;	
}

/*
================
idAI::CheckActions
================
*/
bool idAI::CheckActions ( void ) {

	// Pain?
	if ( CheckPainActions ( ) ) {
		return true;
	}

	// Actions are limited at a cover position to shooting and leaning
	if ( IsBehindCover ( ) ) {
		// Test ranged attack first
		if ( PerformAction ( &actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	} else {
		if ( PerformAction ( &actionEvadeLeft,   (checkAction_t)&idAI::CheckAction_EvadeLeft, &actionTimerEvade )			 ||
			 PerformAction ( &actionEvadeRight,  (checkAction_t)&idAI::CheckAction_EvadeRight, &actionTimerEvade )			 ||
			 PerformAction ( &actionJumpBack,	 (checkAction_t)&idAI::CheckAction_JumpBack, &actionTimerEvade )			 ||
			 PerformAction ( &actionLeapAttack,  (checkAction_t)&idAI::CheckAction_LeapAttack )	) {
			return true;
		} else if ( PerformAction ( &actionRangedAttack,(checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) || 
					PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack )							    ) {
			return true;
		}
	}
	
	return false;
}

/*
================
idAI::PerformAction
================
*/
void idAI::PerformAction ( const char* stateName, int blendFrames, bool noPain ) {
	// Allow movement in actions
	move.fl.allowAnimMove = true;

	// Start the action
	if ( legsAnim.Disabled() ) {
		//MCG: Hmmm... I hope this doesn't break anything, but if an action happens *right* 
		//		at the end of a trigger_anim, then the legs will be enabled (by the SetAnimState
		//		on the torso) with no state!  The actor will then be stuck in place until 
		//		something actually sets the legsAnim state... so let's check for disabled and
		//		set a default state right here...?
		SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
	}
	SetAnimState ( ANIMCHANNEL_TORSO, stateName, blendFrames );

	// Always call finish action when the action is done, it will clear the action flag
	aifl.action = true;
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_FinishAction", 0, 0, SFLAG_ONCLEAR );

	// Go back to idle when done
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", blendFrames );

	// Main state will wait until action is finished before continuing
	if ( noPain ) {
		InterruptState ( "Wait_ActionNoPain" );
	} else {
		InterruptState ( "Wait_Action" );
	}

	OnStartAction ( );
}

bool idAI::PerformAction ( rvAIAction* action, bool (idAI::*condition)(rvAIAction*,int), rvAIActionTimer* timer ) {
	// If we arent ignoring simple think then dont perform this action on a simple think frame
	if ( !action->fl.noSimpleThink && aifl.simpleThink ) {
		return false;
	}

	// Is the action disabled?
	if ( action->fl.disabled ) {
		action->status = rvAIAction::STATUS_FAIL_DISABLED;
		return false;
	}
	
	// Action timers still running?
	if ( !action->timer.IsDone ( actionTime ) ) {
		action->status = rvAIAction::STATUS_FAIL_TIMER;
		return false;
	}

	if ( timer && !timer->IsDone ( actionTime ) ) {
		action->status = rvAIAction::STATUS_FAIL_EXTERNALTIMER;
		return false;
	}

	// Special code for attacks	
	if ( action->fl.isAttack ) {
		// Attacks disabled?
		if ( ai_disableAttacks.GetBool() ) {
			action->status = rvAIAction::STATUS_FAIL_DISABLED;
			return false;
		}
		// No attack actions if we have no enemy or our enemy cant be hurt
		if ( !enemy.ent || enemy.ent->health <= 0 ) {
			action->status = rvAIAction::STATUS_FAIL_NOENEMY;
			return false;
		}
	}		

	// Min Range check	
	if ( action->minRange ) {
		if ( !enemy.ent || !enemy.range || enemy.range < action->minRange ) {
			action->status = rvAIAction::STATUS_FAIL_MINRANGE;
			return false;
		}
	}
	if ( action->minRange2d ) {
		if ( !enemy.ent || !enemy.range2d || enemy.range2d < action->minRange2d ) {
			action->status = rvAIAction::STATUS_FAIL_MINRANGE;
			return false;
		}
	}

	// Max Range check	
	if ( action->maxRange != 0 ) {
		float maxrange = action->maxRange == -1 ? combat.attackRange[1] : action->maxRange;
		if ( !enemy.ent || !enemy.range || enemy.range > maxrange ) {
			if ( action->fl.isMelee && GetEnemy() ) {
				//FIXME: make work with gravity vector
				idVec3 org = physicsObj.GetOrigin();
				const idBounds &myBounds = physicsObj.GetBounds();
				idBounds bounds;

				// expand the bounds out by our melee range
				bounds[0][0] = -combat.meleeRange;
				bounds[0][1] = -combat.meleeRange;
				bounds[0][2] = myBounds[0][2] - 4.0f;
				bounds[1][0] = combat.meleeRange;
				bounds[1][1] = combat.meleeRange;
				bounds[1][2] = myBounds[1][2] + 4.0f;
				bounds.TranslateSelf( org );

				idVec3 enemyOrg = GetEnemy()->GetPhysics()->GetOrigin();
				idBounds enemyBounds = GetEnemy()->GetPhysics()->GetBounds();
				enemyBounds.TranslateSelf( enemyOrg );

				if ( !bounds.IntersectsBounds( enemyBounds ) ) {
					action->status = rvAIAction::STATUS_FAIL_MAXRANGE;
					return false;
				}
			} else {
				action->status = rvAIAction::STATUS_FAIL_MAXRANGE;
				return false;
			}
		}
	}
	if ( action->maxRange2d ) {
		if ( !enemy.ent || !enemy.range2d || enemy.range2d > action->maxRange2d ) {
			action->status = rvAIAction::STATUS_FAIL_MAXRANGE;
			return false;
		}
	}
	
	int animNum;
	if ( action->anims.Num ( ) ) {
		// Pick a random animation from the list
		animNum = GetAnim ( ANIMCHANNEL_TORSO, action->anims[gameLocal.random.RandomInt(action->anims.Num())] );
		if ( !animNum ) {
			action->status = rvAIAction::STATUS_FAIL_ANIM;
			return false;
		}
	} else { 
		animNum = -1;
	}
	
	// Random chance?
	if ( action->chance < 1.0f && gameLocal.random.RandomFloat ( ) > action->chance ) {
		action->status = rvAIAction::STATUS_FAIL_CHANCE;
		action->timer.Clear ( actionTime );
		action->timer.Add ( 100 );
		return false;
	}
	
	// Check the condition
	if ( condition && !(this->*(condition)) ( action, animNum ) ) {
		action->status = rvAIAction::STATUS_FAIL_CONDITION;
		action->timer.Clear ( actionTime );
		action->timer.Add ( action->failRate );
		return false;		
	}

	// Disallow turning during action?
	if ( action->fl.noTurn ) {	
		OverrideFlag ( AIFLAGOVERRIDE_NOTURN, true );
	}

	// Perform the raw action
	PerformAction ( action->state, action->blendFrames, action->fl.noPain );

	// Override legs for this state?
	if ( action->fl.overrideLegs ) {
		DisableAnimState ( ANIMCHANNEL_LEGS );
	}

	// When attacking scale the time by the aggression scale
	float scale;
	if ( action->fl.isAttack ) {
		scale = 2.0f - combat.aggressiveScale;
	} else {
		scale = 1.0f;
	}

	// Restart the action timer using the length of the animation being played
	action->timer.Reset ( actionTime, action->diversity, scale );

	// If the action gets interrupted it will be skipped, if it is then move the action timers forward
	// by half of its normal delay to allow it to be performed again quicker than usual.  This also allows
	// other actions that may be still pending to be performed and thus cause less of a pause after taking pain.
	actionSkipTime = (action->timer.GetTime ( ) + actionTime) / 2;

	// Restart the global action timer using the length of the animation being played
	if ( timer ) {
		timer->Reset ( actionTime, action->diversity, scale );
	}

	action->status = rvAIAction::STATUS_OK;

	actionAnimNum  = animNum;
	     	
	return true;
}
