
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../vehicle/Vehicle.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AAS_Find.h"

static const int 	TACTICALUPDATE_MELEEDELAY   	= 500;		// Rate to update tactical state when rushing
static const int 	TACTICALUPDATE_RANGEDDELAY 		= 2000;		// Rate to update tactical state when in ranged combat
static const int 	TACTICALUPDATE_COVERDELAY  		= 5000;		// Rate to update tactical state when in cover
static const int 	TACTICALUPDATE_HIDEDELAY   		= 5000;		// Rate to update tactical state when hiding
static const int	TACTICALUPDATE_FOLLOWDELAY 		= 500;		// Rate to update tactical state when following
static const int	TACTICALUPDATE_MOVETETHERDELAY	= 1000;		// Rate to update tactical state when moving into tether range
static const int	TACTICALUPDATE_PASSIVEDELAY		= 500;		// Rate to update tactical state when in passive mode
static const int	TACTICALUPDATE_TURRETDELAY		= 250;		// Rate to update tactical state when in turret mode

static const int 	RANGED_ENEMYDELAY				= 2000;		// Time to wait to move after losing sight of an enemy
static const int 	COVER_ENEMYDELAY				= 5000;		// Stay behind cover for 5 seconds after loosing sight of an enemy

static const float	COVER_TRIGGERRADIUS				= 64.0f;

CLASS_STATES_DECLARATION ( idAI )
	// Wait States
	STATE ( "Wait_Activated",				idAI::State_Wait_Activated )
	STATE ( "Wait_ScriptedDone",			idAI::State_Wait_ScriptedDone )
	STATE ( "Wait_Action",					idAI::State_Wait_Action )
	STATE ( "Wait_ActionNoPain",			idAI::State_Wait_ActionNoPain )

	// Global States
	STATE ( "State_WakeUp",					idAI::State_WakeUp )
	STATE ( "State_TriggerAnim",			idAI::State_TriggerAnim )
		
	// Passive states
	STATE ( "State_Passive",				idAI::State_Passive )
		
	// Combat states
	STATE ( "State_Combat",					idAI::State_Combat )
	STATE ( "State_CombatCover",			idAI::State_CombatCover )
	STATE ( "State_CombatMelee",			idAI::State_CombatMelee )
	STATE ( "State_CombatRanged",			idAI::State_CombatRanged )
	STATE ( "State_CombatTurret",			idAI::State_CombatTurret )
	STATE ( "State_CombatHide",				idAI::State_CombatHide )
	
	STATE ( "State_Wander",					idAI::State_Wander )
	STATE ( "State_MoveTether",				idAI::State_MoveTether )
	STATE ( "State_MoveFollow",				idAI::State_MoveFollow )
	STATE ( "State_MovePlayerPush",			idAI::State_MovePlayerPush )
	STATE ( "State_Killed",					idAI::State_Killed )
	STATE ( "State_Dead",					idAI::State_Dead )
	STATE ( "State_LightningDeath",			idAI::State_LightningDeath )
	STATE ( "State_Burn",					idAI::State_Burn )
	STATE ( "State_Remove",					idAI::State_Remove )
	STATE ( "State_ScriptedMove",			idAI::State_ScriptedMove )
	STATE ( "State_ScriptedFace",			idAI::State_ScriptedFace )
	STATE ( "State_ScriptedStop",			idAI::State_ScriptedStop )
	STATE ( "State_ScriptedPlaybackMove",	idAI::State_ScriptedPlaybackMove )
	STATE ( "State_ScriptedPlaybackAim",	idAI::State_ScriptedPlaybackAim )
	STATE ( "State_ScriptedJumpDown",		idAI::State_ScriptedJumpDown )

	// Torso States
	STATE ( "Torso_Idle",					idAI::State_Torso_Idle )
	STATE ( "Torso_Sight",					idAI::State_Torso_Sight )
	STATE ( "Torso_CustomCycle",			idAI::State_Torso_CustomCycle )
	STATE ( "Torso_Action",					idAI::State_Torso_Action )
	STATE ( "Torso_FinishAction",			idAI::State_Torso_FinishAction )
	STATE ( "Torso_Pain",					idAI::State_Torso_Pain )
	STATE ( "Torso_ScriptedAnim",			idAI::State_Torso_ScriptedAnim )
	STATE ( "Torso_PassiveIdle",			idAI::State_Torso_PassiveIdle )
	STATE ( "Torso_PassiveFidget",			idAI::State_Torso_PassiveFidget )

	// Leg States
	STATE ( "Legs_Idle",					idAI::State_Legs_Idle )
	STATE ( "Legs_Move",					idAI::State_Legs_Move )
	STATE ( "Legs_MoveThink",				idAI::State_Legs_MoveThink )
	STATE ( "Legs_TurnLeft",				idAI::State_Legs_TurnLeft )
	STATE ( "Legs_TurnRight",				idAI::State_Legs_TurnRight )
	STATE ( "Legs_ChangeDirection",			idAI::State_Legs_ChangeDirection )
	
	// Head States
	STATE ( "Head_Idle",					idAI::State_Head_Idle )

END_CLASS_STATES

/*
===============================================================================

	States 

===============================================================================
*/

/*
================
idAI::State_TriggerAnim
================
*/
stateResult_t idAI::State_TriggerAnim ( const stateParms_t& parms ) {
	const char* triggerAnim;
	
	// If we dont have the trigger anim, just skip it
	triggerAnim = spawnArgs.GetString ( "trigger_anim" );
	if ( !*triggerAnim || !HasAnim ( ANIMCHANNEL_TORSO, triggerAnim ) ) {
		gameLocal.Warning ( "Missing or invalid 'trigger_anim' ('%s') specified for entity '%s'", 
						    triggerAnim, GetName() );
		return SRESULT_DONE;
	}

	Show();	
	ScriptedAnim ( triggerAnim, 4, false, true );
	
	// FIXME: should do this a better way
	// Alwasy let trigger anims play out
	fl.neverDormant = true;
		
	return SRESULT_DONE;
}

/*
================
idAI::State_WakeUp
================
*/
stateResult_t idAI::State_WakeUp ( const stateParms_t& parms ) {
	const char* triggerAnim;
	
	WakeUp ( );

	// Start immeidately into a playback?
	if( spawnArgs.FindKey( "playback_intro" ) ){
		ScriptedPlaybackMove ( "playback_intro", PBFL_GET_POSITION | PBFL_GET_ANGLES_FROM_VEL, 0 );
	// Start immeidately into a scripted anim?
	} else if ( spawnArgs.GetString ( "trigger_anim", "", &triggerAnim) && *triggerAnim && HasAnim ( ANIMCHANNEL_TORSO, triggerAnim ) ) {
		PostState ( "State_TriggerAnim" );
		return SRESULT_DONE;
	} else {	
		// Either wait to be triggered or wait until they have an enemy
		if ( spawnArgs.GetBool ( "trigger" ) ) {
			// Start the legs and head in the idle position
			SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
			SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
			if ( head ) {
				SetAnimState ( ANIMCHANNEL_HEAD, "Head_Idle", 4 );
			}

			PostState ( "Wait_Activated" );
 		}
	}

	PostState ( "State_Combat" );

	return SRESULT_DONE;
}

/*
================
idAI::State_Passive
================
*/
stateResult_t idAI::State_Passive ( const stateParms_t& parms ) {
	if ( leader && !aifl.scripted ) {
		if ( !GetEnemy() ) {
			if ( combat.fl.aware && !combat.fl.ignoreEnemies ) {
				//aggressive?  I know, doesn't make sense, but becomePassive is different from State_Passive
				TurnTowardLeader( (focusType==AIFOCUS_LEADER) );
			} else if ( focusType == AIFOCUS_LEADER && leader && leader.GetEntity() ) {
				//passive
				TurnToward( leader.GetEntity()->GetPhysics()->GetOrigin() );
			}
		}
	}

	if ( UpdateAction ( ) ) {
		return SRESULT_WAIT;
	}
	if ( UpdateTactical ( TACTICALUPDATE_PASSIVEDELAY ) ) {
		return SRESULT_DONE_WAIT;
	}
	return SRESULT_WAIT;
}

/*
================
idAI::State_Combat

The base combat state is basically the clearing house for other combat states.  By calling
UpdateTactical with a timer of zero it ensures a better tactical move can be found.  
================
*/
stateResult_t idAI::State_Combat ( const stateParms_t& parms ) {	
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			combat.tacticalCurrent = AITACTICAL_NONE;
		
			// Start the legs and head in the idle position
			SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
			SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
			if ( head ) {
				SetAnimState ( ANIMCHANNEL_HEAD, "Head_Idle", 4 );
			}
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			// Make sure we keep facing our enemy
			if ( enemy.ent ) {
				TurnToward ( enemy.lastKnownPosition );
			}
			
			// Update the tactical state using all available tactical abilities and reset
			// the current tactical state since this is the generic state.
			combat.tacticalCurrent = AITACTICAL_NONE;
			if ( UpdateTactical ( 0 ) ) {
				return SRESULT_DONE_WAIT;
			}

			// Perform actions			
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}

			// If we are here then there isnt a single combat state available, thats not good
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_CombatCover

Seek out a cover point that has a vantage point on the enemy.  Stay there firing at the 
enemy until the cover point is invalidated.
================
*/
stateResult_t idAI::State_CombatCover ( const stateParms_t& parms ) {	
	enum {
		STAGE_MOVE,
		STAGE_ATTACK,
	};	
	switch ( parms.stage ) {
		case STAGE_MOVE:
			// Attack when we have either stopped moving or are within melee range
			if ( move.fl.done && aasSensor->Reserved ( ) ) {
				StopMove ( MOVE_STATUS_DONE );
				TurnToward ( GetPhysics()->GetOrigin() + aasSensor->Reserved()->Normal() * 64.0f );
				return SRESULT_STAGE ( STAGE_ATTACK );
			}

			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_COVERDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			// Perform actions on the way to the enemy
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}
			return SRESULT_WAIT;

		case STAGE_ATTACK:
			// If we dont have a cover point anymore then just bail out
			if ( !aasSensor->Reserved ( ) ) {
				ForceTacticalUpdate ( );
				UpdateTactical ( 0 );
				return SRESULT_DONE_WAIT;
			}		
			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_COVERDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			// If we have moved off our cover point, move back
			if ( DistanceTo ( aasSensor->ReservedOrigin ( ) ) > 8.0f ) {
				if ( UpdateTactical ( 0 ) ) {
					return SRESULT_DONE_WAIT;
				}			
			}
			// Dont do any cover checks until they are facing towards the wall
			if ( !FacingIdeal ( ) ) {
				return SRESULT_WAIT;
			}
			// Perform cover point actions
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}
			
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}


/*
================
idAI::State_CombatMelee

Head directly towards our enemy but allow ranged actions along the way
================
*/
stateResult_t idAI::State_CombatMelee ( const stateParms_t& parms ) {	
	enum {
		STAGE_MOVE,			// Move towards the enemy
		STAGE_ATTACK,		// Keep attacking until no longer in melee range
	};	
	switch ( parms.stage ) {
		case STAGE_MOVE:
			// If we can no longer get to our enemy, give up on this and do something else!
			if ( move.moveStatus == MOVE_STATUS_DEST_UNREACHABLE ) {
				ForceTacticalUpdate ( );
				UpdateTactical ( 0 );
				return SRESULT_DONE_WAIT;
			}
			// Attack when we have either stopped moving or are within melee range
			if ( move.fl.done ) {
				StopMove ( MOVE_STATUS_DONE );
				return SRESULT_STAGE ( STAGE_ATTACK );
			}
			// Perform actions on the way to the enemy
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}
			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_MELEEDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			return SRESULT_WAIT;
	
		case STAGE_ATTACK:
			// If we are out of melee range or lost sight of our enemy then start moving again			
			if ( !IsEnemyVisible ( ) ) {
				ForceTacticalUpdate ( );
				UpdateTactical ( 0 );
				return SRESULT_DONE_WAIT;
			}
			// Always face enemy when in melee range
			TurnToward ( enemy.lastKnownPosition );

			// Perform actions while standing still
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}

			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_MELEEDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_CombatRanged

Move towards enemy until within firing range then continue to shoot at the enemy
================
*/
stateResult_t idAI::State_CombatRanged ( const stateParms_t& parms ) {	
	enum {
		STAGE_MOVE,			// Move to an attack position
		STAGE_ATTACK,		// Perform actions until the enemy is out of range or not visible
	};	
	switch ( parms.stage ) {
		case STAGE_MOVE:
			// if we lost our enemy we are done here
			if ( !enemy.ent ) {
				ForceTacticalUpdate ( );
				if ( UpdateTactical ( 0 ) ) {
					return SRESULT_DONE_WAIT;
				}
				return SRESULT_WAIT;
			}			
			// If done moving or within range of a visible enemy we can stop
			if ( move.fl.done ) {
				StopMove ( MOVE_STATUS_DONE );
				return SRESULT_STAGE ( STAGE_ATTACK );
			}			
			// Stop early because we have a shot on our enemy?
			if ( !move.fl.noRangedInterrupt &&
				 !aifl.simpleThink && enemy.fl.visible
				 && gameLocal.time - enemy.lastVisibleChangeTime > 500
				 && enemy.range >= combat.attackRange[0]
				 && enemy.range <= combat.attackRange[1]
				 && ( DistanceTo ( move.moveDest ) < DistanceTo ( move.lastMoveOrigin ) )
				 && (!tether || tether->ValidateDestination ( this, physicsObj.GetOrigin( ) ) )
				 && aiManager.ValidateDestination ( this, physicsObj.GetOrigin ( ) ) ) {
				StopMove ( MOVE_STATUS_DONE );
				return SRESULT_STAGE ( STAGE_ATTACK );
			}			
			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_RANGEDDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			// Perform actions along the way
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}
			return SRESULT_WAIT;
		
		case STAGE_ATTACK:
			// If the enemy is visible but not in fov then rotate
			if ( !enemy.fl.inFov && enemy.ent ) {
				TurnToward ( enemy.lastKnownPosition );
			}			
			// Update tactical state occasionally
			if ( UpdateTactical ( TACTICALUPDATE_RANGEDDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			// Perform actions
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_CombatTurret

Stay put and shoot at the enemy when nearby
================
*/
stateResult_t idAI::State_CombatTurret ( const stateParms_t& parms ) {
	// Turn toward the enemy if visible but not in fov
	if ( IsEnemyVisible ( ) && !enemy.fl.inFov ) {
		TurnToward ( enemy.lastKnownPosition );
	// Turn towards tether direction if we have no enemy
	} else if ( !enemy.ent && tether ) {
		TurnToward ( physicsObj.GetOrigin() + tether->GetPhysics()->GetAxis()[0] * 64.0f );
	} else if ( leader && !aifl.scripted ) {
		if ( !GetEnemy() ) {
			if ( combat.fl.aware && !combat.fl.ignoreEnemies ) {
				//aggressive?  I know, doesn't make sense, but becomePassive is different from State_Passive
				TurnTowardLeader( (focusType==AIFOCUS_LEADER) );
			} else if ( focusType == AIFOCUS_LEADER && leader && leader.GetEntity() ) {
				//passive
				TurnToward( leader.GetEntity()->GetPhysics()->GetOrigin() );
			}
		}
	}

	// Perform actions
	if ( UpdateAction ( ) ) {
		return SRESULT_WAIT;
	}	

	// If there are other available tactical states besides turret then try to
	// go to one of them
	if ( combat.tacticalMaskAvailable & ~(1<<AITACTICAL_TURRET) ) {
		if ( UpdateTactical ( TACTICALUPDATE_TURRETDELAY ) ) {
			return SRESULT_DONE_WAIT;
		}
	}

	return SRESULT_WAIT;
}

/*
================
idAI::State_CombatHide
================
*/
stateResult_t idAI::State_CombatHide ( const stateParms_t& parms ) {
	// Turn toward the enemy if visible but not in fov
	if ( IsEnemyVisible ( ) ) {
		if ( !move.fl.moving ) {
			TurnToward ( enemy.lastKnownPosition );
		}
	}

	if ( UpdateTactical ( TACTICALUPDATE_HIDEDELAY ) ) {
		return SRESULT_DONE_WAIT;
	}

	// Perform actions
	if ( UpdateAction ( ) ) {
		return SRESULT_WAIT;
	}	

	return SRESULT_WAIT;
}

/*
================
idAI::State_Wander
================
*/
stateResult_t idAI::State_Wander ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( !WanderAround ( ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_STAGE ( STAGE_WAIT );
			
		case STAGE_WAIT:
			if ( enemy.ent ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;	
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_MoveTether

Move into tether range
================
*/
stateResult_t idAI::State_MoveTether ( const stateParms_t& parms ) {
	enum {
		STAGE_MOVE,
		STAGE_DONE,
	};

	switch ( parms.stage ) {
		case STAGE_MOVE:
			// If we lost our tether then let UpdateTactical figure out what to do next
			if ( !tether ) {
				if ( UpdateTactical ( 0 ) ) {
					return SRESULT_DONE_WAIT;
				}
				return SRESULT_WAIT;
			}		
			// Stopped moving?
			if ( move.fl.done ) {
				StopMove ( MOVE_STATUS_DONE );				
				if ( tether ) {
					idVec3 toPos = physicsObj.GetOrigin( ) + 64.0f * tether->GetPhysics()->GetAxis()[0];
					TurnToward ( toPos );
					if ( !IsBehindCover() ) {
						//Do a trace *once* to see if I can crouch-look in the direction of the tether at this point
						trace_t tr;
						idVec3 crouchEye = physicsObj.GetOrigin( );
						crouchEye.z += 32.0f;
						gameLocal.TracePoint( this, tr, crouchEye, toPos, MASK_SHOT_BOUNDINGBOX, this );
						if ( tr.fraction >= 1.0f ) {
							combat.fl.crouchViewClear = true;
						}
					}
				}
				ForceTacticalUpdate ( );
				return SRESULT_STAGE ( STAGE_DONE );
			}			
			// Update tactical state occasionally to see if there is something better to do
			if ( UpdateTactical ( TACTICALUPDATE_MOVETETHERDELAY ) ) {
				return SRESULT_DONE_WAIT;
			}
			// Perform actions on the way to the enemy
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}

		case STAGE_DONE:
			if ( UpdateTactical ( ) ) {
				return SRESULT_DONE_WAIT;
			}
			return SRESULT_WAIT;
	}
	
	return SRESULT_ERROR;
}

/*
================
idAI::State_MoveFollow
================
*/
stateResult_t idAI::State_MoveFollow ( const stateParms_t& parms ) {
	enum {
		STAGE_MOVE,
		STAGE_DONE,
	};

	switch ( parms.stage ) {
		case STAGE_MOVE:
			// If we lost our leader we are done
			if ( !leader ) {
				move.fl.done = true;
			// Can we just stop here?
			} else if ( DistanceTo ( leader ) < (move.followRange[0]+move.followRange[1])*0.5f && aiManager.ValidateDestination ( this, physicsObj.GetOrigin( ), false, leader ) ) {
				move.fl.done = true;
				/*
				idEntity* leaderGroundElevator = leader->GetGroundElevator();
				if ( leaderGroundElevator && GetGroundElevator(leaderGroundElevator) != leaderGroundElevator ) {
					move.fl.done = false;
				}
				*/
			}
			// Are we done moving?	
			if ( move.fl.done ) {
				StopMove ( MOVE_STATUS_DONE );
				if ( leader ) {
					TurnToward ( leader->GetPhysics()->GetOrigin() );
				}
				ForceTacticalUpdate ( );
				return SRESULT_STAGE ( STAGE_DONE );
			} else {
				if ( UpdateTactical ( TACTICALUPDATE_FOLLOWDELAY ) ) {
					return SRESULT_DONE_WAIT;
				}
			}

			// Perform actions on the way to the enemy
			UpdateAction ( );

			return SRESULT_WAIT;
			
		case STAGE_DONE:
			if ( UpdateTactical ( ) ) {
				return SRESULT_DONE_WAIT;
			}
			return SRESULT_WAIT;			
	}

	return SRESULT_ERROR;
}

/*
================
idAI::State_MovePlayerPush

Move out of the way when the player is pushing us
================
*/
stateResult_t idAI::State_MovePlayerPush ( const stateParms_t& parms ) {
	enum {
		STAGE_MOVE,
		STAGE_DONE,
	};

	switch ( parms.stage ) {
		case STAGE_MOVE:
			if ( move.fl.done ) {
				StopMove(MOVE_STATUS_DONE);
				ForceTacticalUpdate ( );
				return SRESULT_STAGE ( STAGE_DONE );
			} else if ( gameLocal.GetTime() - move.startTime > 2000 ) {
				if ( UpdateTactical ( ) ) {
					return SRESULT_DONE_WAIT;
				}
			}
			return SRESULT_WAIT;

		case STAGE_DONE:
			if ( UpdateTactical ( ) ) {
				return SRESULT_DONE_WAIT;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_Killed
================
*/
stateResult_t idAI::State_Killed ( const stateParms_t& parms ) {
	disablePain = true;

	//quickburning subjects skip all this jazz
	if( fl.quickBurn )	{
		PostState ( "State_Dead" );
		return SRESULT_DONE;
	}

	// Make sure all animation stops
	StopAnimState ( ANIMCHANNEL_TORSO );
	StopAnimState ( ANIMCHANNEL_LEGS );
	if ( head ) {
		StopAnimState ( ANIMCHANNEL_HEAD );
	}

	// Make sure all animations stop
	animator.ClearAllAnims ( gameLocal.time, 0 );

	if( spawnArgs.GetBool ( "remove_on_death" )  ){
		PostState ( "State_Remove" );
	} else { 
		PostState ( "State_Dead" );
	}
	
	return SRESULT_DONE;
}


/*
================
idAI::State_Dead
================
*/
stateResult_t idAI::State_Dead ( const stateParms_t& parms ) {
	if ( !fl.hidden ) {
		float burnDelay = spawnArgs.GetFloat ( "burnaway" );
		if ( burnDelay > 0.0f ) {
			if( fl.quickBurn )	{
				StopRagdoll();
				PostState ( "State_Burn", SEC2MS(0.05f) );
			} else if ( spawnArgs.GetString( "fx_burn_lightning", NULL ) ) {
				lightningNextTime = 0;
				lightningEffects = 0;
				PostState ( "State_LightningDeath", SEC2MS(burnDelay) );
			} else {
				PostState ( "State_Burn", SEC2MS(burnDelay) );
			}
		}
		float removeDelay = SEC2MS ( spawnArgs.GetFloat ( "removeDelay" ) );
		if ( removeDelay >= 0.0f ) {
			PostState ( "State_Remove", removeDelay );
		}
	} else {
		PostState ( "State_Remove" );
	}
		
	return SRESULT_DONE;
}

/*
================
idAI::State_LightningDeath
================
*/
stateResult_t idAI::State_LightningDeath ( const stateParms_t& parms ) {
	if ( gameLocal.time > lightningNextTime ) {
		if ( !lightningEffects )
		{
			StartSound ( "snd_burn_lightning", SND_CHANNEL_BODY, 0, false, NULL );
		}
		rvClientCrawlEffect* effect;
		effect = new rvClientCrawlEffect ( gameLocal.GetEffect ( spawnArgs, "fx_burn_lightning" ), this, 100 );
		effect->Play ( gameLocal.time, false );
		lightningNextTime = gameLocal.time + 100;
		lightningEffects++;
	}
	if ( lightningEffects < 10 )
	{
		return SRESULT_WAIT;
	}

	/*
	if ( spawnArgs.GetString( "fx_burn_lightning" ) )
	{
		for ( int i = GetAnimator()->NumJoints() - 1; i > 0; i -- ) {
			if ( i != GetAnimator()->GetFirstChild ( (jointHandle_t)i ) ) {
				if ( !gameLocal.random.RandomInt(1) ) {
					PlayEffect( "fx_burn_lightning", (jointHandle_t)i );
				}
			}
		}
	}
	*/
	float burnDelay = spawnArgs.GetFloat ( "burnaway" );
	if ( burnDelay > 0.0f ) {
		PostState ( "State_Burn", 0 );
	}
		
	return SRESULT_DONE;
}

/*
================
idAI::State_Burn
================
*/
stateResult_t idAI::State_Burn ( const stateParms_t& parms ) {
	if( fl.hidden ){
		return SRESULT_DONE;
	}
	

	renderEntity.noShadow = true;

	// Dont let the articulated figure be shot once they start burning away
	SetCombatContents ( false );
	fl.takedamage = false;
	
	renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	idEntity *head = GetHead();
	if ( head ) {
		head->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
	}

	UpdateVisuals();

	if ( spawnArgs.GetString( "fx_burn_particles", NULL ) )
	{
		int i = GetAnimator()->GetJointHandle( spawnArgs.GetString("joint_chestOffset","chest") );
		if ( i != INVALID_JOINT )
		{
			PlayEffect( "fx_burn_particles_chest", (jointHandle_t)i );
		}

		for ( i = GetAnimator()->NumJoints() - 1; i > 0; i -- ) {
			if ( i != GetAnimator()->GetFirstChild ( (jointHandle_t)i ) ) {
				PlayEffect( "fx_burn_particles", (jointHandle_t)i );
			}
		}
		//FIXME: head, too?
		//FIXME: from joint to parent joint?
		//FIXME: not small joints...
	}

	StartSound ( "snd_burn", SND_CHANNEL_BODY, 0, false, NULL ); 				

	return SRESULT_DONE;
}

/*
================
idAI::State_Remove
================
*/
stateResult_t idAI::State_Remove ( const stateParms_t& parms ) {
	PostEventMS( &EV_Remove, 0 );
	return SRESULT_DONE;
}

/*
================
idAI::State_Torso_ScriptedAnim
================
*/
stateResult_t idAI::State_Torso_ScriptedAnim ( const stateParms_t& parms ) {
	if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
		aifl.scripted = false;
		return SRESULT_DONE;
	}
	return SRESULT_WAIT;
}

/*
================
idAI::State_ScriptedMove
================
*/
stateResult_t idAI::State_ScriptedMove ( const stateParms_t& parms ) {
	if ( !aifl.scripted || move.fl.done ) {
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

/*
================
idAI::State_ScriptedFace
================
*/
stateResult_t idAI::State_ScriptedFace ( const stateParms_t& parms ) {
	if ( !aifl.scripted || FacingIdeal() ) {
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}

/*
================
idAI::State_ScriptedPlaybackMove
================
*/
stateResult_t idAI::State_ScriptedPlaybackMove ( const stateParms_t& parms ) {
	if ( mPlayback.IsActive() ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idAI::State_ScriptedPlaybackAim
================
*/
stateResult_t idAI::State_ScriptedPlaybackAim ( const stateParms_t& parms ) {
	if ( mLookPlayback.IsActive() ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idAI::State_ScriptedStop
================
*/
stateResult_t idAI::State_ScriptedStop ( const stateParms_t& parms ) {
	ScriptedEnd ( );

	// If ending in an idle animation move the legs back to idle and 
	// revert back to normal combat
	if ( aifl.scriptedEndWithIdle ) {
		ForceTacticalUpdate ( );
		UpdateTactical ( 0 );
	}
	
	return SRESULT_DONE;
}

/*
===============================================================================

	AI Torso States

===============================================================================
*/

/*
================
idAI::State_Torso_Idle
================
*/
stateResult_t idAI::State_Torso_Idle ( const stateParms_t& parms ) {
	// Custom passive idle?
	if ( combat.tacticalCurrent == AITACTICAL_PASSIVE && passive.animIdlePrefix.Length() ) {
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_PassiveIdle", parms.blendFrames );
		return SRESULT_DONE;
	}

	// If the legs were disabled then get them back into idle again
	if ( legsAnim.Disabled ( ) ) {
		SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	}
	
	// Start idle animation
	const char* idleAnim = GetIdleAnimName ();
	if ( !torsoAnim.IsIdle () || idStr::Icmp ( idleAnim, animator.CurrentAnim ( ANIMCHANNEL_TORSO )->AnimName ( ) ) ) {
		IdleAnim ( ANIMCHANNEL_TORSO, idleAnim, parms.blendFrames );
	}
	
	return SRESULT_DONE;
}

/*
================
idAI::State_Torso_PassiveIdle
================
*/
stateResult_t idAI::State_Torso_PassiveIdle ( const stateParms_t& parms ) {
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_LOOP,
		STAGE_LOOP_WAIT,
		STAGE_END,
		STAGE_END_WAIT,
		STAGE_TALK_WAIT,
	};
	
	idStr animName;
	
	switch ( parms.stage ) {
		case STAGE_START:
			// Talk animation?
			if ( passive.talkTime > gameLocal.time ) {
				idStr postfix;
				if ( talkMessage >= TALKMSG_LOOP ) {
					postfix = aiTalkMessageString[TALKMSG_LOOP];
					postfix += va("%d", (int)(talkMessage - TALKMSG_LOOP+1) );
				} else {
					postfix = aiTalkMessageString[talkMessage];
				}
				// Find their talk animation
				animName = spawnArgs.GetString ( va("%s_%s", passive.animTalkPrefix.c_str(), postfix.c_str() ) );
				if ( !animName.Length ( ) ) {
					animName = spawnArgs.GetString ( passive.animTalkPrefix );
				}

				if ( animName.Length ( ) ) {
					DisableAnimState ( ANIMCHANNEL_LEGS );			
					PlayCycle ( ANIMCHANNEL_TORSO, animName, parms.blendFrames );
					return SRESULT_STAGE ( STAGE_TALK_WAIT );
				}
			}

			// If we have a fidget animation then see if its time to play it
			if ( passive.animFidgetPrefix.Length() ) {			
				if ( passive.fidgetTime == 0 ) {					
					passive.fidgetTime = gameLocal.time + SEC2MS ( spawnArgs.GetInt ( "fidget_rate", "20" ) );
				} else if ( gameLocal.time > passive.fidgetTime ) {				
					PostAnimState ( ANIMCHANNEL_TORSO, "Torso_PassiveFidget", 4 );
					PostAnimState ( ANIMCHANNEL_TORSO, "Torso_PassiveIdle", 4 );
					return SRESULT_DONE;
				}
			}
			
			// Do we have a custom idle animation?
			passive.idleAnim		   = spawnArgs.RandomPrefix ( passive.animIdlePrefix, gameLocal.random );			
			passive.idleAnimChangeTime = passive.fl.multipleIdles ? SEC2MS ( spawnArgs.GetFloat ( "idle_change_rate", "10" ) ) : 0;
			
			// Is there a start animation for the idle?
			animName = va("%s_start", passive.idleAnim.c_str() );
			if ( HasAnim ( ANIMCHANNEL_TORSO, animName ) ) {							
				PlayAnim ( ANIMCHANNEL_TORSO, animName, parms.blendFrames );
				return SRESULT_STAGE ( STAGE_START_WAIT );
			}
			
			PlayAnim ( ANIMCHANNEL_TORSO, passive.idleAnim, parms.blendFrames  );
			return SRESULT_STAGE ( STAGE_LOOP_WAIT );
			
		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, passive.idleAnim, 0 );
			return SRESULT_STAGE ( STAGE_LOOP_WAIT );
			
		case STAGE_LOOP_WAIT:
			// Talk animation interrupting?
			if ( passive.animTalkPrefix.Length() && passive.talkTime > gameLocal.time ) {
				return SRESULT_STAGE ( STAGE_END );
			}
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				// If its time to play a new idle animation then do so
				if ( passive.idleAnimChangeTime && gameLocal.time > passive.idleAnimChangeTime ) {
					return SRESULT_STAGE ( STAGE_END );
				}				
				// If its time to fidget then do so
				if ( passive.fidgetTime && gameLocal.time > passive.fidgetTime ) {
					return SRESULT_STAGE ( STAGE_END );
				}
				// Loop the idle again
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
			
		case STAGE_END:
			animName = va("%s_end", passive.idleAnim.c_str() );
			if ( HasAnim ( ANIMCHANNEL_TORSO, animName ) ) {							
				PlayAnim ( ANIMCHANNEL_TORSO, animName, parms.blendFrames );
				return SRESULT_STAGE ( STAGE_END_WAIT );
			}
			return SRESULT_STAGE ( STAGE_START );

		case STAGE_END_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_START );
			}
			return SRESULT_WAIT;
			
		case STAGE_TALK_WAIT:
			if ( gameLocal.time > passive.talkTime ) { 
				return SRESULT_STAGE ( STAGE_START );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_Torso_PassiveFidget
================
*/
stateResult_t idAI::State_Torso_PassiveFidget ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	
	idStr animName;
	
	switch ( parms.stage ) {
		case STAGE_INIT:
			passive.fidgetTime = 0;
			animName = spawnArgs.RandomPrefix ( va("anim_%sfidget", passive.prefix.c_str() ), gameLocal.random );
			if ( !animName.Length ( ) ) {
				animName = spawnArgs.RandomPrefix ( "anim_fidget", gameLocal.random );
			}
			if ( !animName.Length ( ) ) {
				return SRESULT_DONE;
			}
			if ( !PlayAnim ( ANIMCHANNEL_TORSO, animName, parms.blendFrames ) ) {
				return SRESULT_DONE;
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

/*
================
idAI::State_ScriptedJumpDown

Face edge, walk off
================
*/
stateResult_t idAI::State_ScriptedJumpDown( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_FACE_WAIT,
		STAGE_ANIM,
		STAGE_JUMP_WAIT,
		STAGE_INAIR,
		STAGE_WAIT_LAND,
		STAGE_ANIM_WAIT,
		STAGE_FINISH
	};

	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				aifl.scripted = true;
				Event_SaveMove();
				StopMove(MOVE_STATUS_DONE);
				//move.current_yaw = move.ideal_yaw;
				move.moveCommand = MOVE_NONE;
				move.fl.allowDirectional = false;
			}
			if ( !move.fl.onGround ) {
				return SRESULT_STAGE ( STAGE_INAIR );
			}
			return SRESULT_STAGE ( STAGE_FACE_WAIT );
			break;
		case STAGE_FACE_WAIT:
			if ( FacingIdeal() )
			{
				return SRESULT_STAGE ( STAGE_ANIM );
			}
			return SRESULT_WAIT;
			break;
		case STAGE_ANIM:
			{
				DisableAnimState ( ANIMCHANNEL_LEGS );
				torsoAnim.StopAnim ( 0 );
				legsAnim.StopAnim ( 0 );

				if ( HasAnim( ANIMCHANNEL_TORSO, "jumpdown_start" ) ) {
					PlayAnim( ANIMCHANNEL_TORSO, "jumpdown_start", 4 );
					move.fl.allowAnimMove = true;
					move.fl.noGravity = true;
					return SRESULT_STAGE ( STAGE_JUMP_WAIT );
				}
				return SRESULT_STAGE ( STAGE_INAIR );
			}
			break;
		case STAGE_JUMP_WAIT:
			if ( AnimDone( ANIMCHANNEL_TORSO, 2 ) )	{
				return SRESULT_STAGE ( STAGE_INAIR );
			}
			return SRESULT_WAIT;
			break;
		case STAGE_INAIR:
			{
				idVec3 vel = viewAxis[0] * 200.0f;
				vel.z = -50.0f;
				physicsObj.SetLinearVelocity( vel );
				PlayAnim( ANIMCHANNEL_TORSO, "jumpdown_loop", 4 );
				move.fl.allowAnimMove = false;
				move.fl.noGravity = false;
				return SRESULT_STAGE ( STAGE_WAIT_LAND );
			}
			break;
		case STAGE_WAIT_LAND:
			if ( physicsObj.OnGround() )//GetPhysics()->HasGroundContacts() )
			{
				PlayAnim( ANIMCHANNEL_TORSO, "jumpdown_end", 0 );
				move.fl.allowAnimMove = true;
				return SRESULT_STAGE ( STAGE_ANIM_WAIT );
			}
			return SRESULT_WAIT;
			break;
		case STAGE_ANIM_WAIT:
			if ( AnimDone( ANIMCHANNEL_TORSO, 0 ) )
			{
				return SRESULT_STAGE ( STAGE_FINISH );
			}
			return SRESULT_WAIT;
			break;
		case STAGE_FINISH:
			Event_RestoreMove();
			SetState( "State_Combat" );
			aifl.scripted = false;
			return SRESULT_DONE;
			break;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_Torso_CustomCycle
================
*/
stateResult_t idAI::State_Torso_CustomCycle ( const stateParms_t& parms ) {
	return SRESULT_DONE;
}

/*
================
idAI::State_Torso_Sight
================
*/
stateResult_t idAI::State_Torso_Sight ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT: {
			// Dont let everyone on the team play their sight sounds at the same time
			if ( aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SIGHT ) ) {
				Speak ( "lipsync_sight", true );
				aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SIGHT, 1000 );
			}

			// Execute sighted scripts
			if( !enemy.fl.sighted ) {
				enemy.fl.sighted = true;
				ExecScriptFunction ( funcs.first_sight, this );
			}
			ExecScriptFunction ( funcs.sight );

			idStr animName = spawnArgs.GetString ( "anim_sight", spawnArgs.GetString ( "sight_anim" ) );
			if ( HasAnim ( ANIMCHANNEL_TORSO, animName ) ) {
				DisableAnimState ( ANIMCHANNEL_LEGS );
				PlayAnim ( ANIMCHANNEL_TORSO, animName, parms.blendFrames );
				return SRESULT_STAGE ( STAGE_WAIT );
			}
			return SRESULT_DONE;
		}
			
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idAI::State_Torso_Action
================
*/
stateResult_t idAI::State_Torso_Action ( const stateParms_t& parms ) {
	// Invalid animation so dont bother with running the action
	if ( actionAnimNum == -1 ) {
		return SRESULT_DONE_WAIT;
	}

	// Play the action animation
	PlayAnim ( ANIMCHANNEL_TORSO, animator.GetAnim ( actionAnimNum )->FullName ( ), parms.blendFrames );
		
	// Wait till animation is finished
	PostAnimState ( ANIMCHANNEL_TORSO, "Wait_TorsoAnim", parms.blendFrames );
	
	return SRESULT_DONE_WAIT;
}

/*
================
idAI::State_Torso_FinishAction
================
*/
stateResult_t idAI::State_Torso_FinishAction ( const stateParms_t& parms ) {

	RestoreFlag ( AIFLAGOVERRIDE_DISABLEPAIN );
	RestoreFlag ( AIFLAGOVERRIDE_DAMAGE );
	RestoreFlag ( AIFLAGOVERRIDE_NOTURN );
	RestoreFlag ( AIFLAGOVERRIDE_NOGRAVITY );

	enemy.fl.lockOrigin	 = false;
	aifl.action			 = false;
	
	OnStopAction ( );
	
	return SRESULT_DONE;
}

/*
================
idAI::State_Torso_Pain
================
*/
stateResult_t idAI::State_Torso_Pain ( const stateParms_t& parms ) {
	enum {
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_LOOP,
		STAGE_LOOP_WAIT,
		STAGE_END,
		STAGE_END_WAIT
	};
	
	idStr animName;
	
	switch ( parms.stage ) {
		case STAGE_START:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			
			pain.loopEndTime = gameLocal.time + SEC2MS ( spawnArgs.GetFloat ( "pain_maxLoopTime", "1" ) );

			// Just in case the pain anim wasnt set before we got here.
			if ( !painAnim.Length ( ) ) {
				painAnim = "pain";
			}
			
			animName = va( "%s_start", painAnim.c_str() );
			if ( HasAnim ( ANIMCHANNEL_TORSO, animName ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, animName, parms.blendFrames );
				return SRESULT_STAGE ( STAGE_START_WAIT );
			}
			PlayAnim ( ANIMCHANNEL_TORSO, painAnim, parms.blendFrames );
			return SRESULT_STAGE ( STAGE_END_WAIT );
		
		case STAGE_START_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 1 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
			
		case STAGE_LOOP:
			PlayCycle ( ANIMCHANNEL_TORSO, painAnim, 1 );
			return SRESULT_STAGE ( STAGE_LOOP_WAIT );
	
		case STAGE_LOOP_WAIT:
			if ( gameLocal.time - pain.lastTakenTime > AI_PAIN_LOOP_DELAY ) { 
				return SRESULT_STAGE ( STAGE_END );
			}
			if ( !pain.loopEndTime || gameLocal.time > pain.loopEndTime ) {
				return SRESULT_STAGE ( STAGE_END );
			}
			return SRESULT_WAIT;
			
		case STAGE_END: 
			animName = va( "%s_end", painAnim.c_str() );
			PlayAnim ( ANIMCHANNEL_TORSO, animName, 1 );
			return SRESULT_STAGE ( STAGE_END_WAIT );
		
		case STAGE_END_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
===============================================================================

	Head States

===============================================================================
*/

/*
================
idAI::State_Head_Idle
================
*/
stateResult_t idAI::State_Head_Idle ( const stateParms_t& parms ) {
	return SRESULT_DONE;
}

/*
===============================================================================

	AI Leg States

===============================================================================
*/

/*
================
idAI::State_Legs_Idle
================
*/
stateResult_t idAI::State_Legs_Idle ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT: {
			const char* idleAnim;
			move.fl.allowAnimMove = false;
			idleAnim = GetIdleAnimName ( );
			if ( !legsAnim.IsIdle () || idStr::Icmp ( idleAnim, animator.CurrentAnim ( ANIMCHANNEL_LEGS )->AnimName ( ) ) ) {
				IdleAnim ( ANIMCHANNEL_LEGS, idleAnim, parms.blendFrames );
			}
			return SRESULT_STAGE ( STAGE_WAIT );			
		}
		
		case STAGE_WAIT:
			// Dont let the legs do anything from idle until they are done blending
			// the animations they have (this is to prevent pops when an action finishes
			// and blends to idle then immediately the legs move to walk)
			if ( animator.IsBlending ( ANIMCHANNEL_LEGS, gameLocal.time ) ) {
				return SRESULT_WAIT;
			}
		
			if ( move.fl.moving && CanMove() ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Move", 2 );
				return SRESULT_DONE;
			} 
			return SRESULT_WAIT;			
	}	
	return SRESULT_ERROR;
}

/*
================
idAI::State_Legs_TurnLeft
================
*/
stateResult_t idAI::State_Legs_TurnLeft ( const stateParms_t& parms ) {
	// Go back to idle if we are done here
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
		
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idAI::State_Legs_TurnRight
================
*/
stateResult_t idAI::State_Legs_TurnRight ( const stateParms_t& parms ) {
	// Go back to idle if we are done here
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
	
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idAI::State_Legs_Move
================
*/
stateResult_t idAI::State_Legs_Move ( const stateParms_t& parms ) {
	idStr	animName;
	
	move.fl.allowAnimMove = true;
	move.fl.allowPrevAnimMove = false;
	
	// If not moving forward just go back to idle
	if ( !move.fl.moving || !CanMove() ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
		return SRESULT_DONE;
	}

	// Movement direction changed?
	if ( move.idealDirection != move.currentDirection ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "Legs_ChangeDirection", parms.blendFrames );
	}		

	// Make sure run status is up to date when legs start moving
	UpdateRunStatus ( );
	
	// Run or walk?
	animName = "run";
	if ( !move.fl.idealRunning && HasAnim ( ANIMCHANNEL_TORSO, "walk" ) ) { 
		animName = "walk";
	}
	
	// Append the run direction to the animation name
	if ( move.idealDirection == MOVEDIR_LEFT ) {
		animName = animName + "_left";
	} else if ( move.idealDirection == MOVEDIR_RIGHT ) {
		animName = animName + "_right";
	} else if ( move.idealDirection == MOVEDIR_BACKWARD ) {
		animName = animName + "_backwards";
	} else {
/*		
		// When moving to cover the walk animation is special
		if ( move.moveCommand == MOVE_TO_COVER && !move.fl.idealRunning && move.fl.running ) {
			animName = "run_slowdown";
		}
*/			
	}

	// Update running flag with ideal state
	move.fl.running = move.fl.idealRunning;
	move.currentDirection = move.idealDirection;

	PlayCycle ( ANIMCHANNEL_LEGS, animName, parms.blendFrames );

	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_MoveThink", parms.blendFrames );
	
	return SRESULT_DONE;
}

/*
================
idAI::State_Legs_MoveThink
================
*/
stateResult_t idAI::State_Legs_MoveThink ( const stateParms_t& parms ) {
	move.fl.allowAnimMove = true;
	move.fl.allowPrevAnimMove = false;

	// If not moving anymore then go back to idle
	if ( !move.fl.moving || !CanMove() ) {
		PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
		return SRESULT_DONE;
	}
	
	// If the run state has changed restart the leg movement
	if ( UpdateRunStatus ( ) || (move.idealDirection != move.currentDirection) ) { 
		PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Move", 4 );
		return SRESULT_DONE;
	}
		
	// Just wait a frame before doing anything else
	return SRESULT_WAIT;
}

/*
================
idAI::State_Legs_ChangeDirection
================
*/
stateResult_t idAI::State_Legs_ChangeDirection ( const stateParms_t& parms ) {
	if ( FacingIdeal ( ) || (move.idealDirection != move.currentDirection) ) {
		return SRESULT_DONE;
	}

	move.fl.allowAnimMove = false;
	move.fl.allowPrevAnimMove = true;
	return SRESULT_WAIT;
}

/*
===============================================================================

	State helpers

===============================================================================
*/

/*
================
idAI::UpdateRunStatus
================
*/
bool idAI::UpdateRunStatus ( void ) {
	// Get new run status
	if ( !move.fl.moving || !CanMove() ) {
		move.fl.idealRunning = false;		
	} else if ( move.walkRange && !ai_useRVMasterMove.GetBool() && move.moveCommand != MOVE_TO_ENEMY && (DistanceTo ( move.moveDest ) - move.range) < move.walkRange ) {
		move.fl.idealRunning = false;
	} else if ( IsTethered() && tether->IsRunForced ( ) ) {
		move.fl.idealRunning = true;
	} else if ( IsTethered() && tether->IsWalkForced ( ) ) {
		move.fl.idealRunning = false;
	} else if ( move.fl.noWalk ) {
		move.fl.idealRunning = true;
	} else if ( move.fl.noRun ) {
		move.fl.idealRunning = false;
	} else if ( move.walkRange ) {
		if ( move.fl.obstacleInPath ) {
			move.fl.idealRunning = false;
		} else if ( InLookAtCoverMode() ) {
			move.fl.idealRunning = false;
		} else if ( !ai_useRVMasterMove.GetBool() && move.walkTurn && !move.fl.allowDirectional && fabs(GetTurnDelta ( )) > move.walkTurn ) {
			move.fl.idealRunning = false;
		} else {
			move.fl.idealRunning = true;
		}
	} else {
		move.fl.idealRunning = true;
	}
	
	return move.fl.running != move.fl.idealRunning;
}

/*
================
idAI::UpdateTactical

This method is a recursive method that will determine the best tactical state to be in
from the given available states.
================
*/
bool idAI::UpdateTactical ( int delay ) {	
	// Update tactical cannot be called while performing an action and it must be called from the main state loop
	assert ( !aifl.action );
	
	// No movement updating on simple think (if the update is forced do it anyway)
	if ( combat.tacticalUpdateTime && aifl.simpleThink && !combat.tacticalMaskUpdate ) {
		return false;
	}
	// Don't let tactical updates pre-empt pain actions
	if ( pain.takenThisFrame ) {
		return false;
	}

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerFindEnemy.Start ( );
	}

	// Keep the enemy status up to date
	if ( !combat.fl.ignoreEnemies ) {
		// If we dont have an enemy or havent seen our enemy for a while just find a new one entirely
		if ( gameLocal.time - enemy.checkTime > 250 ) {
			CheckForEnemy ( true, true );
		} else if ( !IsEnemyRecentlyVisible ( ) ) {
			CheckForEnemy ( true );
		}
	}

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerFindEnemy.Stop ( );
	}

	// We have sighted an enemy so execute the sight state
	if ( IsEnemyVisible() && !enemy.fl.sighted ) {
		PerformAction ( "Torso_Sight", 4, true );
		return true;
	}

	// continue with the last updatetactical if we still have bits left to check in our update mask
	if ( !combat.tacticalMaskUpdate ) {				
		// Ignore the tactical delay if we are taking too much damage here.
		// TODO: use skipcurrentdestination instead
		if ( !combat.tacticalPainThreshold || combat.tacticalPainTaken < combat.tacticalPainThreshold ) {
			// Delay tactical updating?
			if ( combat.tacticalUpdateTime && move.moveStatus != MOVE_STATUS_BLOCKED_BY_ENEMY && delay && (gameLocal.time - combat.tacticalUpdateTime) < delay ) {
				return false;
			}
		}

		// handle Auto break tethers
		if ( IsTethered ( ) && tether->IsAutoBreak ( ) && IsWithinTether ( ) ) {
			tether = NULL;
		}

		// Filter the tactical
		combat.tacticalMaskUpdate = FilterTactical ( combat.tacticalMaskAvailable );
	} else {
		// Make sure all cached tactical updates are still valid
		combat.tacticalMaskUpdate &= FilterTactical ( combat.tacticalMaskAvailable );
		if ( !combat.tacticalMaskUpdate ) {
			return false;
		}
	}

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerTactical.Start ( );
	}

	// Recursively look for a better tactical state
	bool result = UpdateTactical_r ( );

	// AI Speeds
	if ( ai_speeds.GetBool ( ) ) {
		aiManager.timerTactical.Stop ( );
	}
	
	return result;
}

bool idAI::UpdateTactical_r ( void ) {
	// Mapping of tactical types to combat states
	static const char* tacticalState [ ] = { 
		"State_Combat",				// AITACTICAL_NONE
		"State_CombatMelee",		// AITACTICAL_MELEE
		"State_MoveFollow",			// AITACTICAL_MOVE_FOLLOW
		"State_MoveTether",			// AITACTICAL_MOVE_TETHER
		"State_MovePlayerPush",		// AITACTICAL_MOVE_PLAYERPUSH
		"State_CombatCover",		// AITACTICAL_COVER
		"State_CombatCover",		// AITACTICAL_COVER_FLANK
		"State_CombatCover",		// AITACTICAL_COVER_ADVANCE
		"State_CombatCover",		// AITACTICAL_COVER_RETREAT
		"State_CombatCover",		// AITACTICAL_COVER_AMBUSH
		"State_CombatRanged",		// AITACTICAL_RANGED
		"State_CombatTurret",		// AITACTICAL_TURRET
		"State_CombatHide",			// AITACTICAL_HIDE
		"State_Passive",			// AITACTICAL_PASSIVE
	};

	if ( g_perfTest_aiStationary.GetBool() ) {
		return false;
	}
	// Determine the new tactical state
	aiTactical_t newTactical = AITACTICAL_TURRET;
	
	if ( combat.tacticalMaskUpdate & AITACTICAL_MOVE_PLAYERPUSH_BIT ) {
		newTactical = AITACTICAL_MOVE_PLAYERPUSH;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_MOVE_FOLLOW_BIT ) {
		newTactical = AITACTICAL_MOVE_FOLLOW;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_COVER_ADVANCE_BIT ) { 
		newTactical = AITACTICAL_COVER_ADVANCE;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_COVER_FLANK_BIT ) { 
		newTactical = AITACTICAL_COVER_FLANK;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_COVER_RETREAT_BIT ) { 
		newTactical = AITACTICAL_COVER_RETREAT;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_COVER_AMBUSH_BIT ) { 
		newTactical = AITACTICAL_COVER_AMBUSH;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_COVER_BIT ) { 
		newTactical = AITACTICAL_COVER;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_RANGED_BIT ) {
		newTactical = AITACTICAL_RANGED;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_MELEE_BIT ) { 
		newTactical = AITACTICAL_MELEE;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_MOVE_TETHER_BIT ) {
		newTactical = AITACTICAL_MOVE_TETHER;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_HIDE_BIT ) { 
		newTactical = AITACTICAL_HIDE;
	} else if ( combat.tacticalMaskUpdate & AITACTICAL_PASSIVE_BIT ) { 
		newTactical = AITACTICAL_PASSIVE;
	}

	// Remove the new tactical from the available in case we have to recursively call ourself
	combat.tacticalMaskUpdate &= ~(1<<newTactical);

	// Check the tactical state and see if we need to move or not
	aiCTResult_t checkResult = CheckTactical ( newTactical );
	
	// Skip the tactial state if necessary
	if ( checkResult == AICTRESULT_SKIP ) {
		return UpdateTactical_r ( );
	}

	// Check to see if we need to move for the new tactical tate
	if ( checkResult == AICTRESULT_OK ) {
		bool result = true;
		switch ( newTactical ) {
			case AITACTICAL_MOVE_TETHER:
				result = MoveToTether ( tether );
				break;
				
			case AITACTICAL_MOVE_FOLLOW:
				//FIXME: if leader is on an elevator, we need to find a position on the elevator...
				result = MoveToEntity ( leader );
				break;
				
			case AITACTICAL_MOVE_PLAYERPUSH:
				result = MoveOutOfRange ( pusher, (move.followRange[0] + move.followRange[1]) / 2.0f );
				break;				
				
			case AITACTICAL_COVER:
			case AITACTICAL_COVER_FLANK:
			case AITACTICAL_COVER_ADVANCE:
			case AITACTICAL_COVER_RETREAT:
			case AITACTICAL_COVER_AMBUSH:
				result = MoveToCover ( combat.attackRange[0], combat.attackRange[1], newTactical );
				break;				
				
			case AITACTICAL_RANGED:
				result = MoveToAttack ( enemy.ent, animator.GetAnim ( "ranged_attack" ) );
				// Found nothing but still have a find going
				if ( !result && aasFind ) {
					// turn ranged back on for next update on next frame
					combat.tacticalMaskUpdate |= AITACTICAL_RANGED_BIT;
					return false;
				}
				break;
				
			case AITACTICAL_MELEE:
				result = MoveToEnemy ( );
				break;

			case AITACTICAL_PASSIVE:
			case AITACTICAL_TURRET:
				StopMove ( MOVE_STATUS_DONE );
				break;

			case AITACTICAL_HIDE:
				if ( combat.tacticalMaskAvailable & (1<<AITACTICAL_COVER) ) {
					result = MoveToCover ( combat.attackRange[1], 5000.0f, AITACTICAL_COVER );
				} else { 
					result = false;
				}
				if ( !result ) {
					result = MoveToHide ( );
				}
				if ( !result && enemy.ent ) {
					result = MoveOutOfRange ( enemy.ent, combat.hideRange[1], combat.hideRange[0] );
				}
				break; 
		}

		// Move impossible?
		if ( !result ) {		
			return UpdateTactical_r ( );
		}

		// Look to see if we are already there
		if ( ReachedPos ( move.moveDest, move.moveCommand, move.range ) ) {
			StopMove ( MOVE_STATUS_DONE );
		}
	} else if ( newTactical == combat.tacticalCurrent ) {
		// We are staying here, so update the time so the AI doesnt constantly keep checking
		combat.tacticalUpdateTime = gameLocal.time;
		combat.tacticalMaskUpdate = 0;
		return false;
	} else {
		// Stop moving since we are changing state and dont need to move
		StopMove ( MOVE_STATUS_DONE );
	}

	aiTactical_t oldTactical = combat.tacticalCurrent;

	// If we had any saved aas find we dont need it anymore
	delete aasFind;
	aasFind = NULL;
		
	// Set tactical update time to current time.  This time is used to delay tactical updates 
	// in a generic fashion.
	combat.tacticalUpdateTime = gameLocal.time;
	combat.tacticalCurrent    = newTactical;
	
	// Allow handling of tactical changes
	if ( newTactical != oldTactical ) {
		OnTacticalChange ( newTactical );
	}

	// Reset tactical counters since we are probably going somewhere else
 	combat.tacticalPainTaken	= 0;
 	combat.tacticalFlinches		= 0;
 	combat.tacticalMaskUpdate	= 0;

	// Start the legs and head in the idle position
	if ( legsAnim.Disabled ( ) ) {
		SetAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
	}
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
	if ( head ) {
		SetAnimState ( ANIMCHANNEL_HEAD, "Head_Idle", 4 );
	}

	// Go to the new state		
	PostState ( tacticalState [ newTactical ] );
	return true;
}

/*
================
idAI::FilterTactical

Filters out tactical types from the given available list.
================
*/
int idAI::FilterTactical ( int availableTactical ) {
	// No passive if we are agressive
	if ( enemy.ent || combat.fl.aware ) {
		availableTactical &= ~(AITACTICAL_PASSIVE_BIT);
	}

	// If we cant move then there are not many options	
	if ( move.moveType == MOVETYPE_STATIC || move.fl.disabled ) {
		availableTactical &= AITACTICAL_NONMOVING_BITS;
		return availableTactical;
	}

	// Only need push player
	if ( aifl.scripted ) {
		availableTactical &= ~(AITACTICAL_MOVE_PLAYERPUSH_BIT);
	} else {
		if ( !pusher || !pusher->IsType ( idPlayer::GetClassType ( ) ) ) {
			//no current pusher
			if ( combat.tacticalCurrent == AITACTICAL_MOVE_PLAYERPUSH ) {
				//in the middle of a push move, just continue it
				availableTactical = AITACTICAL_NONE_BIT;
				return availableTactical;
			}
			//stop pushing away
			availableTactical &= ~(AITACTICAL_MOVE_PLAYERPUSH_BIT);
		} else {
			//have a pusher
			if ( combat.tacticalCurrent == AITACTICAL_MOVE_PLAYERPUSH ) {
				//in the middle of a push move, only allow it to continue playerpush, but can re-calc, if needbe
				availableTactical = AITACTICAL_MOVE_PLAYERPUSH_BIT;
				return availableTactical;
			}
			if ( ai_playerPushAlways.GetBool() ) {
				switch ( combat.tacticalCurrent ) {
					case AITACTICAL_MELEE:
					case AITACTICAL_MOVE_FOLLOW:
					case AITACTICAL_MOVE_TETHER:
					case AITACTICAL_MOVE_PLAYERPUSH:
					case AITACTICAL_COVER:
					case AITACTICAL_COVER_FLANK:
					case AITACTICAL_COVER_ADVANCE:
					case AITACTICAL_COVER_RETREAT:
					case AITACTICAL_COVER_AMBUSH:
					case AITACTICAL_RANGED:
					case AITACTICAL_HIDE:
						//okay to be pushed by players
						break;
					default:
						if ( !leader ) {
							//no leader and not in a moving tactical state, don't be pushed by players
							availableTactical &= ~(AITACTICAL_MOVE_PLAYERPUSH_BIT);
						}
						break;
				}
			} else if ( !leader || (move.fl.moving&&move.goalEntity!=leader) ) {
				availableTactical &= ~(AITACTICAL_MOVE_PLAYERPUSH_BIT);
			}
		}
	}

	// No tether move if not tethered
	if ( !IsTethered ( ) ) {
		availableTactical &= ~(AITACTICAL_MOVE_TETHER_BIT);

		// Filter out any tactical states that require a leader
		if ( !leader || (enemy.ent && IsEnemyVisible() && enemy.range < move.followRange[1] * 2.0f ) ) {
			availableTactical &= ~(AITACTICAL_MOVE_FOLLOW_BIT);
		} else if ( leader && !enemy.ent ) {
			availableTactical &= ~(AITACTICAL_COVER_BITS);
		}
	// When tethered disable actions that dont adhear to the tether.
	} else {
		availableTactical &= ~(AITACTICAL_MELEE_BIT|AITACTICAL_MOVE_FOLLOW_BIT|AITACTICAL_HIDE_BIT|AITACTICAL_COVER_ADVANCE_BIT|AITACTICAL_COVER_FLANK_BIT|AITACTICAL_COVER_RETREAT_BIT|AITACTICAL_COVER_AMBUSH_BIT);
	}

	// If we dont have an enemy we can filter out pure combat states
	if ( !enemy.ent ) {
		availableTactical &= ~(AITACTICAL_RANGED_BIT|AITACTICAL_MELEE_BIT|AITACTICAL_COVER_FLANK_BIT|AITACTICAL_COVER_ADVANCE_BIT|AITACTICAL_COVER_RETREAT_BIT|AITACTICAL_COVER_AMBUSH_BIT|AITACTICAL_HIDE_BIT);
		
		// If we arent aware either then we cant take cover
		if ( !combat.fl.aware ) {
			availableTactical &= ~(AITACTICAL_COVER_BITS);
		}			
	} else { 
		// Dont advance or flank our enemy if we have seen them recently
		if ( IsEnemyRecentlyVisible ( 0.5f ) ) {
			availableTactical &= ~(AITACTICAL_COVER_ADVANCE_BIT|AITACTICAL_COVER_FLANK_BIT);
		}
		
		// FIXME: need conditions for these cover states
		availableTactical &= ~(AITACTICAL_COVER_AMBUSH_BIT|AITACTICAL_COVER_RETREAT_BIT);
	}
					
	// Filter out all cover states if cover is disabled
	if ( ai_disableCover.GetBool ( ) ) {
		availableTactical &= ~(AITACTICAL_COVER_BITS);
	}
	
	//if we need to fight in melee then fight in melee!
	if( IsMeleeNeeded( ) )	{
		availableTactical &= ~(AITACTICAL_RANGED_BIT|AITACTICAL_COVER_FLANK_BIT|AITACTICAL_COVER_ADVANCE_BIT|AITACTICAL_COVER_RETREAT_BIT|AITACTICAL_COVER_AMBUSH_BIT|AITACTICAL_HIDE_BIT);
	}

	return availableTactical;
}

/*
================
idAI::CheckTactical

Returns 'AICHECKTACTICAL_MOVE' if we should use the given tactical state and execute a movement to do so
Returns 'AICHECKTACTICAL_SKIP' if the given tactical state should be skipped
Returns 'AICHECKTACTICAL_NOMOVE' if the given tactical state should be used but no movement is required
================
*/
aiCTResult_t idAI::CheckTactical ( aiTactical_t tactical ) {
	// Handle non movement tactical states first
	if ( Q4BIT(tactical) & AITACTICAL_NONMOVING_BITS ) {
		// If we are moving 
		if ( move.fl.moving ) {
			return AICTRESULT_OK;
		}
		return AICTRESULT_NOMOVE;
	}

	// If we are tethered always check that first
	if ( IsTethered ( ) ) {
		if ( !move.fl.moving || tactical != combat.tacticalCurrent ) {
			// We stopped out of tether range so try a new move to get back in
			if ( !tether->ValidateDestination ( this, physicsObj.GetOrigin ( ) ) ) {
				return AICTRESULT_OK;
			}
		} else {
			// Move is out of tether range, so look for another one
			if ( !tether->ValidateDestination ( this, move.moveDest ) ) {
				return AICTRESULT_OK;
			}
		}
	}

	// Anything wrong with our current destination, if so pick a new move
	if ( SkipCurrentDestination ( ) ) {
		return AICTRESULT_OK;
	}
	
	switch ( tactical ) {
		case AITACTICAL_MOVE_FOLLOW:
			// If already moving to our leader dont move again
			if ( move.fl.moving && move.moveCommand == MOVE_TO_ENTITY && move.goalEntity == leader ) {
				return AICTRESULT_NOMOVE;
			}
			// If not moving and within follow range we can skip the follow state
			if ( !move.fl.moving ) {
				if ( DistanceTo ( leader ) < move.followRange[1] ) {
					//unless the leader is on an elevator we should be standing on...
					/*
					idEntity* leaderGroundElevator = leader->GetGroundElevator();
					if ( leaderGroundElevator && GetGroundElevator(leaderGroundElevator) != leaderGroundElevator ) {
						return AICTRESULT_OK;
					}
					*/
					return AICTRESULT_SKIP;
				}
			}
			return AICTRESULT_OK;
			
		case AITACTICAL_MOVE_TETHER:
			// If not moving we dont want to idle in the move state so skip it
			if ( !move.fl.moving ) {
				return AICTRESULT_SKIP;
			}
			// We are still heading towards our tether so dont reissue a move
			return AICTRESULT_NOMOVE;
		
		case AITACTICAL_RANGED:
			// Currently issuing a find
			if ( dynamic_cast<rvAASFindGoalForAttack*>(aasFind) ) {
				return AICTRESULT_OK;
			}
		
			// If set to zero a tactical update was forced so lets force a move too
			if ( !combat.tacticalUpdateTime ) {
				return AICTRESULT_OK;
			}
			// If the enemy is visible then no movement is required for ranged combat
			if ( !IsEnemyRecentlyVisible ( ) ) {
				return AICTRESULT_OK;
			}
			if ( !move.fl.moving ) {
				// If not within the attack range we need to move
				if ( enemy.range > combat.attackRange[1] || enemy.range < combat.attackRange[0] ) {
					return AICTRESULT_OK;
				}

				// If we havent seen our enemy for a while and arent moving, then get moving!
				if ( enemy.lastVisibleTime && gameLocal.time - enemy.lastVisibleTime > RANGED_ENEMYDELAY ) {
					return AICTRESULT_OK;
				}
			} else {
				// Is our destination out of range?
				if ( (move.moveDest - enemy.lastKnownPosition).LengthSqr ( ) > Square ( combat.attackRange[1] ) ) {
					return AICTRESULT_OK;
				}
			}	
			// Our position is fine so just stay in ranged combat
			return AICTRESULT_NOMOVE;
			
		case AITACTICAL_MELEE:
			// IF we are already moving towards our enemy then we dont need a state change
			if ( move.fl.moving && move.moveCommand == MOVE_TO_ENEMY && move.goalEntity == enemy.ent ) {
				return AICTRESULT_NOMOVE;
			}
			return AICTRESULT_OK;
			
		case AITACTICAL_COVER:
		case AITACTICAL_COVER_FLANK:
		case AITACTICAL_COVER_ADVANCE:
		case AITACTICAL_COVER_RETREAT:
		case AITACTICAL_COVER_AMBUSH:
			// If set to zero a tactical update was forced so lets force a move too
			if ( !combat.tacticalUpdateTime ) {
				return AICTRESULT_OK;
			}
			if ( enemy.ent && !IsEnemyRecentlyVisible ( ) ) {
				return AICTRESULT_OK;
			}
			// No longer behind cover? 
			if ( !IsBehindCover ( ) ) {
				return AICTRESULT_OK;
			}
		 	// Move if there have been too many close calls
		 	if ( combat.tacticalFlinches && combat.tacticalFlinches > gameLocal.random.RandomInt(12) + 3 ) {
		 		return AICTRESULT_OK;
		 	}
			// Move if cover destination isnt valid anymore
			if ( !IsCoverValid() ) {
				return AICTRESULT_OK;
			}
			return AICTRESULT_NOMOVE;			
	}
	
	return AICTRESULT_OK;
}

/*
================
idAI::WakeUpTargets
================
*/
void idAI::WakeUpTargets ( void ) {
	const idKeyValue* kv;
		
	for ( kv = spawnArgs.MatchPrefix ( "wakeup_target" ); kv; kv = spawnArgs.MatchPrefix ( "wakeup_target", kv ) ) {
		idEntity* ent;
		ent = gameLocal.FindEntity ( kv->GetValue ( ) );
		if ( !ent ) {
			gameLocal.Warning ( "Unknown wakeup_target '%s' on entity '%s'", kv->GetValue().c_str(), GetName() );
		} else {
			ent->Signal( SIG_TRIGGER );
			ent->PostEventMS( &EV_Activate, 0, this );
			ent->TriggerGuis ( );
		}
	}

	// Find all the tether entities we target
	const char* target;	
	if ( spawnArgs.GetString ( "tether_target", "", &target ) && *target ) {
		idEntity* ent;
		ent = gameLocal.FindEntity ( target );
		if ( ent && ent->IsType ( rvAITether::GetClassType ( ) ) ) {
			ProcessEvent ( &EV_Activate, ent );
		}
	}
}

/*
===============================================================================

	Wait States 

===============================================================================
*/

/*
================
idAI::State_Wait_Activated

Stop the state thread until the ai is activated
================
*/
stateResult_t idAI::State_Wait_Activated ( const stateParms_t& parms ) {
	if ( (aifl.activated || aifl.pain ) && CanBecomeSolid ( ) ) {
		return SRESULT_DONE;
	}	
	return SRESULT_WAIT;
}

/*
================
idAI::State_Wait_Action

Stop the state thread as long as a torso action is running and allow a pain action to interrupt
================
*/
stateResult_t idAI::State_Wait_Action ( const stateParms_t& parms ) {
	if ( CheckPainActions ( ) ) {
		// Our current action is being interrupted by pain so make sure the action can be performed
		// immediately after coming out of the pain.
		actionTime = actionSkipTime;
		return SRESULT_DONE;
	}

	if ( !aifl.action ) {
		return SRESULT_DONE;
	}
	return SRESULT_WAIT;
}

/*
================
idAI::State_Wait_ActionNoPain

Stop the state thread as long as a torso action is running
================
*/
stateResult_t idAI::State_Wait_ActionNoPain ( const stateParms_t& parms ) {
	if ( !aifl.action ) {
		return SRESULT_DONE;
	}
	return SRESULT_WAIT;
}

/*
================
idAI::State_Wait_ScriptedDone

Stop the state thread as a scripted sequence is active
================
*/
stateResult_t idAI::State_Wait_ScriptedDone ( const stateParms_t& parms ) {
	if ( aifl.scripted ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}
