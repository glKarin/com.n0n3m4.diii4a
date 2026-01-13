//
// TODO: 
// - unarmed posture
// - relaxed posture
// - turning in place too much, rely more on look angles?

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AI_Tactical.h"

const idEventDef AI_ForcePosture	( "forcePosture", "d" );

CLASS_DECLARATION( idAI, rvAITactical )
	EVENT( AI_ForcePosture,					rvAITactical::Event_ForcePosture )
	EVENT( EV_PostSpawn,					rvAITactical::Event_PostSpawn )
END_CLASS

static const char* aiPostureString[AIPOSTURE_MAX] = { 
	"stand",				// AIPOSTURE_STAND,
	"crouch",				// AIPOSTURE_CROUCH,
	"cover_left",			// AIPOSTURE_STAND_COVER_LEFT,
	"cover_right",			// AIPOSTURE_STAND_COVER_RIGHT,
	"crouch_cover",			// AIPOSTURE_CROUCH_COVER
	"crouch_cover_left",	// AIPOSTURE_CROUCH_COVER_LEFT,
	"crouch_cover_right",	// AIPOSTURE_CROUCH_COVER_RIGHT,
	"relaxed",				// AIPOSTURE_RELAXED
	"unarmed",				// AIPOSTURE_UNARMED
	"at_attention",			// AIPOSTURE_AT_ATTENTION
};

/*
================
rvAITactical::rvAITactical
================
*/
rvAITactical::rvAITactical ( void ) {
	shots = 0;
	nextWallTraceTime = 0;
}

void rvAITactical::InitSpawnArgsVariables ( void ) 
{
	// Initialize the posture info
	InitPostureInfo ( );

	maxShots = spawnArgs.GetInt ( "maxShots", "1" );
	minShots = spawnArgs.GetInt ( "minShots", "1" );

	fireRate = SEC2MS ( spawnArgs.GetFloat ( "fireRate", "1" ) );

	healthRegen	= spawnArgs.GetInt( "healthRegen", "2" );
	healthRegenEnabled	= spawnArgs.GetBool( "healthRegenEnabled", "0" );
}

/*
================
rvAITactical::Spawn
================
*/
void rvAITactical::Spawn ( void ) {
	InitSpawnArgsVariables();

	// Force a posture?
	const char* temp;
	postureForce = AIPOSTURE_DEFAULT;
	if ( spawnArgs.GetString ( "forcePosture", "", &temp ) && *temp ) {
		for ( postureForce = AIPOSTURE_STAND; postureForce != AIPOSTURE_MAX; ((int&)postureForce)++ ) {
			if ( !idStr::Icmp ( aiPostureString[postureForce], temp ) ) {
				break;
			}
		}
		if ( postureForce >= AIPOSTURE_MAX ) {
			postureForce = AIPOSTURE_DEFAULT;
		}
	}

	UpdatePosture ( );
	postureCurrent = postureIdeal;	

	OnPostureChange ( );

	ammo	 = spawnArgs.GetInt ( "ammo", "-1" );
			
	// Initialize custom actions
	actionElbowAttack.Init		( spawnArgs, "action_elbowAttack",		NULL,	AIACTIONF_ATTACK );
	actionKillswitchAttack.Init	( spawnArgs, "action_killswitchAttack",	NULL,	AIACTIONF_ATTACK );
	
	actionTimerPeek.Init ( spawnArgs, "actionTimer_peek" );
	
//	playerFocusTime = 0;
//	playerAnnoyTime = SEC2MS(spawnArgs.GetFloat ( "annoyed", "5" ));

	healthRegenNextTime = 0;
	maxHealth = health;
}

/*
================
rvAITactical::Think
================
*/
void rvAITactical::Think ( void ) {
	idAI::Think ( );

	// If not simple thinking and not in an action, update the posture 
	if ( !(aifl.scripted&&move.moveCommand==MOVE_NONE) && aifl.awake && !aifl.simpleThink && !aifl.action && !aifl.dead ) {
		if ( UpdatePosture ( ) ) {
			PerformAction ( "Torso_SetPosture", 4, true );
		}
	}

// FIXME: disabled for now, its annoying people
	/*
	idPlayer* localPlayer;
	localPlayer = gameLocal.GetLocalPlayer();
	
	// If the player has been standing in front of the marine and looking at him for too long he should say something
	if ( !aifl.dead && playerFocusTime && playerAnnoyTime 
		&& !aifl.scripted && focusType == AIFOCUS_PLAYER && localPlayer && !IsSpeaking() 
		&& !localPlayer->IsBeingTalkedTo() //nobody else is talking to him right now
		&& DistanceTo( localPlayer ) < 64.0f ) {
		idVec3		diff;


		diff = GetPhysics()->GetOrigin() - localPlayer->GetPhysics()->GetOrigin();
		diff.NormalizeFast();

		// Is the player looking at the marine?
		if ( diff * localPlayer->viewAxis[0] > 0.7f ) {			
			// Say something every 5 seconds
			if ( gameLocal.time - playerFocusTime > playerAnnoyTime ) {
				// Debounce it against other marines
				if ( aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_CANIHELPYOU ) ) {
					Speak ( "lipsync_canihelpyou", true );
					aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_CANIHELPYOU, 5000 );				
				}
			}
		} else {
			playerFocusTime = gameLocal.time;
		}
	} else {
		playerFocusTime = gameLocal.time;
	}
	*/

	if ( health > 0 ) {
		//alive
		if ( healthRegenEnabled && healthRegen ) {
			if ( gameLocal.GetTime() >= healthRegenNextTime ) {
				health = idMath::ClampInt( 0, maxHealth, health+healthRegen );
				healthRegenNextTime = gameLocal.GetTime() + 1000;
			}
		}
	}

	//crappy place to do this, just testing
	bool clearPrefix = true;
	bool facingWall = false;
	if ( move.fl.moving && InCoverMode() && combat.fl.aware ) {
		clearPrefix = false;
		if ( DistanceTo ( aasSensor->ReservedOrigin() ) < move.walkRange * 2.0f ) {
			facingWall = true;
		} else if ( nextWallTraceTime < gameLocal.GetTime() ) {
			//do an occasional check for solid architecture directly in front of us
			nextWallTraceTime = gameLocal.GetTime() + gameLocal.random.RandomInt(750)+750;
			trace_t	wallTrace;
			idVec3 start, end;
			idMat3 axis;
			if ( neckJoint != INVALID_JOINT ) {
				GetJointWorldTransform ( neckJoint, gameLocal.GetTime(), start, axis );
				end = start + axis[0] * 32.0f;
			} else {
				start = GetEyePosition();
				start += viewAxis[0] * 8.0f;//still inside bbox
				end = start + viewAxis[0] * 32.0f;
			}
			//trace against solid arcitecture only, don't care about other entities
			gameLocal.TracePoint ( this, wallTrace, start, end, MASK_SOLID, this );
			if ( wallTrace.fraction < 1.0f ) {
				facingWall = true;
			} else {
				clearPrefix = true;
			}
		}
	}

	if ( facingWall ) {
		if ( !animPrefix.Length() ) {
			animPrefix = "nearcover";
		}
	} else if ( clearPrefix && animPrefix == "nearcover" ) {
		animPrefix = "";
	}
}				

/*
================
rvAITactical::Save
================
*/
void rvAITactical::Save( idSaveGame *savefile ) const {
	savefile->WriteSyncId();

	savefile->WriteInt ( ammo );
	savefile->WriteInt ( shots );
	
//	savefile->WriteInt ( playerFocusTime );
//	savefile->WriteInt ( playerAnnoyTime );

	savefile->WriteInt ( (int&)postureIdeal );
	savefile->WriteInt ( (int&)postureCurrent );
	savefile->WriteInt ( (int&)postureForce );
	// TOSAVE: aiPostureInfo_t		postureInfo[AIPOSTURE_MAX]; // cnicholson:
	savefile->WriteInt ( healthRegenNextTime );
	savefile->WriteInt ( maxHealth );
	savefile->WriteInt ( nextWallTraceTime );

	
	actionElbowAttack.Save ( savefile );
	actionKillswitchAttack.Save ( savefile );

	actionTimerPeek.Save ( savefile );
}

/*
================
rvAITactical::Restore
================
*/
void rvAITactical::Restore( idRestoreGame *savefile ) {
	InitSpawnArgsVariables ( );
	savefile->ReadSyncId( "rvAITactical" );

	savefile->ReadInt ( ammo );
	savefile->ReadInt ( shots );
	
//	savefile->ReadInt ( playerFocusTime );
//	savefile->ReadInt ( playerAnnoyTime );

	savefile->ReadInt ( (int&)postureIdeal );
	savefile->ReadInt ( (int&)postureCurrent );
	savefile->ReadInt ( (int&)postureForce );
	// TORESTORE: aiPostureInfo_t		postureInfo[AIPOSTURE_MAX];
	savefile->ReadInt ( healthRegenNextTime );
	savefile->ReadInt ( maxHealth );
	savefile->ReadInt ( nextWallTraceTime );

	actionElbowAttack.Restore ( savefile );
	actionKillswitchAttack.Restore ( savefile );

	actionTimerPeek.Restore ( savefile );

	UpdateAnimPrefix ( );
}

/*
================
rvAITactical::CanTurn
================
*/
bool rvAITactical::CanTurn ( void ) const {
	if ( !move.fl.moving && !postureInfo[postureCurrent].fl.canTurn ) {
		return false;
	}	
	return idAI::CanTurn ( );
}

/*
================
rvAITactical::CanMove
================
*/
bool rvAITactical::CanMove ( void ) const {
	if ( !postureInfo[postureCurrent].fl.canMove ) {
		return false;
	}	
	return idAI::CanMove ( );
}

/*
================
rvAITactical::CheckAction_Reload
================
*/
bool rvAITactical::CheckAction_Reload ( rvAIAction* action, int animNum ) {
	if ( ammo == 0 ) {
		return true;
	}
	return false;	
}

/*
================
rvAITactical::CheckActions
================
*/
bool rvAITactical::CheckActions ( void ) {
	// Pain?
	if ( CheckPainActions ( ) ) {
		return true;
	}
	
	// If we are pressed, fight-- do not break melee combat until you or the enemy is dead.
	if ( IsMeleeNeeded ( ))	{
		if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack )							 || 
			 PerformAction ( &actionElbowAttack, (checkAction_t)&idAI::CheckAction_LeapAttack )									) {
			return true;
		}
		//take no actions other than fighting
		return false;
	}

	// Handle any posture changes
	if ( postureIdeal != postureCurrent ) {
		PerformAction ( "Torso_SetPosture", 4, true );
		return true;
	}

	// Reload takes precedence
	if ( !move.fl.moving && postureInfo[postureCurrent].fl.canReload && ammo == 0 ) {
		PerformAction ( "Torso_Reload", 4, false );
		return true;
	}

	if ( IsBehindCover ( ) ) {
		// If we have no enemy try peeking	
		if ( !IsEnemyRecentlyVisible ( ) ) {
			if ( aiManager.CheckTeamTimer ( team, AITEAMTIMER_ACTION_PEEK ) ) {
				if ( actionTimerPeek.IsDone ( actionTime ) ) {
					actionTimerPeek.Reset ( actionTime, 0.5f );
					aiManager.SetTeamTimer ( team, AITEAMTIMER_ACTION_PEEK, 2000 );
					PerformAction ( "Torso_Cover_Peek", 4, false );
					return true;
				}
			}
		}
		
		// Attacks from cover
		if ( postureInfo[postureCurrent].fl.canShoot && (ammo > 0 || ammo == -1) ) {
			// Kill switch attack from cover?
			if ( postureInfo[postureCurrent].fl.canKillswitch && IsEnemyRecentlyVisible ( ) ) {
				if ( PerformAction ( &actionKillswitchAttack, NULL, &actionTimerRangedAttack ) ) {
					return true;
				}
			}

			if ( PerformAction ( &actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
				return true;
			}			
		}
		
		return false;
	}

	// Standard attacks
	if ( PerformAction ( &actionMeleeAttack, (checkAction_t)&idAI::CheckAction_MeleeAttack )							 || 
		 PerformAction ( &actionElbowAttack, (checkAction_t)&idAI::CheckAction_LeapAttack )									) {
		return true;
	}

	// Ranged attack only if there is ammo
	if ( postureInfo[postureCurrent].fl.canShoot && (ammo > 0 || ammo == -1) ) {
		if ( PerformAction ( &actionRangedAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
			return true;
		}
	}

	return false;
}

/*
================
rvAITactical::CheckRelaxed

Returns true if the marine should currently be in a relaxed state
================
*/
bool rvAITactical::CheckRelaxed ( void ) const {
//	if ( forceRelaxed ) {
//		return true;
//	}

/*
	// If we have a leader, no enemy, and havent had an enemy for over 5 seconds go to relaxed
	if ( leader && !enemy.ent && !tether && gameLocal.time - enemy.changeTime > 5000 ) {
		return true;
	}
*/
	
	// Alwasy relaxed when ignoring enemies
	if ( !combat.fl.aware ) {
		return true;
	}

	if ( enemy.ent || focusType != AIFOCUS_PLAYER || move.fl.moving || move.fl.crouching || talkState == TALK_OK ) {
		return false;
	}
	
	if ( gameLocal.time >= focusTime ) {
		return false;
	} 
		
	return true;
}

/*
================
rvAITactical::GetIdleAnimName
================
*/
const char* rvAITactical::GetIdleAnimName ( void ) {
	return "idle";
}	

/*
================
rvAITactical::UpdateAnimPrefix
================
*/
void rvAITactical::UpdateAnimPrefix ( void ) {
	if ( postureCurrent == AIPOSTURE_STAND ) {
		animPrefix = "";
	} else {
		animPrefix = aiPostureString[postureCurrent];
	}
}

/*
================
rvAITactical::InitPostureInfo
================
*/
void rvAITactical::InitPostureInfo ( void ) {
	int posture;
	for ( posture = AIPOSTURE_DEFAULT + 1; posture < AIPOSTURE_MAX; posture ++ ) {
		aiPostureInfo_t& info = postureInfo[(aiPosture_t)posture];

		postureCurrent = (aiPosture_t)posture;
		UpdateAnimPrefix ( );
	
		info.fl.canMove   		= HasAnim ( ANIMCHANNEL_TORSO, "run", true );
		info.fl.canPeek   		= HasAnim ( ANIMCHANNEL_TORSO, "peek", true );
		info.fl.canReload 		= HasAnim ( ANIMCHANNEL_TORSO, "reload", true );
		info.fl.canShoot  		= HasAnim ( ANIMCHANNEL_TORSO, "range_attack", true );
		info.fl.canKillswitch	= HasAnim ( ANIMCHANNEL_TORSO, "killswitch", true );
		info.fl.canTurn			= false;
	}
	
	// FIXME: this should be based on the availablity of turn anims
	postureInfo[AIPOSTURE_STAND].fl.canTurn = true;
	postureInfo[AIPOSTURE_RELAXED].fl.canTurn = true;
	postureInfo[AIPOSTURE_UNARMED].fl.canTurn = true;
}

/*
================
rvAITactical::UpdatePosture
================
*/
bool rvAITactical::UpdatePosture ( void ) {
	// If the posture is being forced then use that until its no longer forced
	if ( postureForce != AIPOSTURE_DEFAULT ) {
		postureIdeal = postureForce;
	// Not forcing posture, determine it from our current state
	} else {
		postureIdeal = AIPOSTURE_STAND;

		// Behind cover?
		if ( IsBehindCover ( ) ) {
			bool left;
			if ( enemy.ent ) {
				left = (aasSensor->Reserved()->Normal().Cross ( physicsObj.GetGravityNormal ( ) ) * (enemy.lastVisibleEyePosition - physicsObj.GetOrigin())) > 0.0f;
			} else if ( tether ) {
				left = (aasSensor->Reserved()->Normal().Cross ( physicsObj.GetGravityNormal ( ) ) * tether->GetPhysics()->GetAxis()[0] ) > 0.0f;
			} else {
				left = false;
			}
			// Should be crouching behind cover?
			if ( InCrouchCoverMode ( ) ) {
				if ( (aasSensor->Reserved()->flags & FEATURE_LOOK_LEFT) && left ) {
					postureIdeal = AIPOSTURE_CROUCH_COVER_LEFT;
				} else if ( (aasSensor->Reserved()->flags & FEATURE_LOOK_RIGHT) && !left ) {
					postureIdeal = AIPOSTURE_CROUCH_COVER_RIGHT;
				} else {
					postureIdeal = AIPOSTURE_CROUCH_COVER;
				}
			} else {
				if ( (aasSensor->Reserved()->flags & FEATURE_LOOK_LEFT) && left ) {
					postureIdeal = AIPOSTURE_STAND_COVER_LEFT;
				} else if ( (aasSensor->Reserved()->flags & FEATURE_LOOK_RIGHT) && !left ) {
					postureIdeal = AIPOSTURE_STAND_COVER_RIGHT;
				} else if ( (aasSensor->Reserved()->flags & FEATURE_LOOK_LEFT) ) {				
					postureIdeal = AIPOSTURE_STAND_COVER_LEFT;
				} else {
					postureIdeal = AIPOSTURE_STAND_COVER_RIGHT;
				}
			}
		} else if ( combat.fl.aware //aggressive
			&& (FacingIdeal ( ) || CheckFOV ( currentFocusPos )) //looking in desired direction
			&& ((leader && leader->IsCrouching()) || combat.fl.crouchViewClear) ) {//leader is crouching or we can crouch-look in this direction here 
			//we crouch only if leader is
			postureIdeal = AIPOSTURE_CROUCH;
		} else if ( CheckRelaxed ( ) ) {
			postureIdeal = AIPOSTURE_RELAXED;
		}

		//never crouch in melee!
		if( IsMeleeNeeded() )	{
			postureIdeal = AIPOSTURE_STAND;
		}
	}

	// Default the posture if trying to move with one that doesnt support it
	if ( move.fl.moving && !postureInfo[postureIdeal].fl.canMove ) {
		postureIdeal = AIPOSTURE_STAND;
	// Default the posture if trying to turn and we cant in the posture we chose
	} else if ( (move.moveCommand == MOVE_FACE_ENEMY || move.moveCommand == MOVE_FACE_ENTITY) && !postureInfo[postureIdeal].fl.canTurn ) {
		postureIdeal = AIPOSTURE_STAND;
	}
		
	return (postureIdeal != postureCurrent);
}

/*
================
rvAITactical::OnPostureChange
================
*/
void rvAITactical::OnPostureChange ( void ) {
	UpdateAnimPrefix ( );
}

/*
============
rvAITactical::OnSetKey
============
*/
void rvAITactical::OnSetKey	( const char* key, const char* value ) {
	idAI::OnSetKey ( key, value );
	
/*
	if ( !idStr::Icmp ( key, "annoyed" ) ) {
		playerAnnoyTime = SEC2MS( atof ( value ) );
	}
*/
}

/*
================
rvAITactical::OnStopMoving
================
*/
void rvAITactical::OnStopMoving	( aiMoveCommand_t oldMoveCommand ) {
	// Ensure the peek doesnt happen immedately every time we stop at a cover
	if ( IsBehindCover ( ) ){
		actionTimerPeek.Clear ( actionTime );
		actionTimerPeek.Add ( 2000, 0.5f );

		actionKillswitchAttack.timer.Reset ( actionTime, actionKillswitchAttack.diversity );

		// We should be looking fairly close to the right direction, so just snap it
		TurnToward ( GetPhysics()->GetOrigin() + aasSensor->Reserved()->Normal() * 64.0f );
		move.current_yaw = move.ideal_yaw;
	}
	
	idAI::OnStopMoving ( oldMoveCommand );
}

/*
================
rvAITactical::CalculateShots
================
*/
void rvAITactical::CalculateShots ( const char* fireAnim ) {
	// Random number of shots ( scale by aggression range)
	shots = (minShots + gameLocal.random.RandomInt(maxShots-minShots+1)) * combat.aggressiveScale;
	if ( shots > ammo ) {
		shots = ammo;
	}
	
	// Update the firing animation playback rate
	int	animNum;	
	animNum = GetAnim( ANIMCHANNEL_TORSO, fireAnim );
	if ( animNum != 0 ) {
		const idAnim* anim = GetAnimator()->GetAnim ( animNum );
		if ( anim ) {			
			GetAnimator()->SetPlaybackRate ( animNum, ((float)anim->Length() * combat.aggressiveScale) / fireRate );
		}
	}
}

/*
================
rvAITactical::UseAmmo 
================
*/
void rvAITactical::UseAmmo ( int amount ) {
	if ( ammo <= 0 ) {
		return;
	}
	
	shots--;
	ammo-=amount;
	if ( ammo < 0 ) {
		ammo = 0;
		shots = 0;
	}
}

/*
================
rvAITactical::GetDebugInfo
================
*/
void rvAITactical::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "rvAITactical", "postureIdeal",		aiPostureString[postureIdeal], userData );
	proc ( "rvAITactical", "postureCurrent",	aiPostureString[postureCurrent], userData );
	proc ( "rvAITactical", "healthRegen",		va("%d",healthRegen), userData );
	proc ( "rvAITactical", "healthRegenEnabled",healthRegenEnabled?"true":"false", userData );
	proc ( "rvAITactical", "healthRegenNextTime",va("%d",healthRegenNextTime), userData );
	proc ( "rvAITactical", "maxHealth",			va("%d",maxHealth), userData );

	proc ( "rvAITactical", "nextWallTraceTime",	va("%d",nextWallTraceTime), userData );
	
	
	proc ( "idAI", "action_killswitchAttack",	aiActionStatusString[actionKillswitchAttack.status], userData );	
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvAITactical )
	STATE ( "Torso_RangedAttack",			rvAITactical::State_Torso_RangedAttack )
	STATE ( "Torso_MovingRangedAttack",		rvAITactical::State_Torso_MovingRangedAttack )

	STATE ( "Torso_Cover_LeanAttack",		rvAITactical::State_Torso_Cover_LeanAttack )
	STATE ( "Torso_Cover_LeanLeftAttack",	rvAITactical::State_Torso_Cover_LeanLeftAttack )
	STATE ( "Torso_Cover_LeanRightAttack",	rvAITactical::State_Torso_Cover_LeanRightAttack )
	STATE ( "Torso_Cover_Peek",				rvAITactical::State_Torso_Cover_Peek )

	STATE ( "Torso_Reload",					rvAITactical::State_Torso_Reload )
	
	STATE ( "Torso_SetPosture",				rvAITactical::State_Torso_SetPosture )
	
	STATE ( "Frame_Peek",					rvAITactical::State_Frame_Peek )
END_CLASS_STATES

/*
================
rvAITactical::State_Torso_SetPosture 
================
*/
stateResult_t rvAITactical::State_Torso_SetPosture ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT_RELAXED,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT: {
			idStr transAnim = va("%s_to_%s", aiPostureString[postureCurrent], aiPostureString[postureIdeal] );
			if ( !HasAnim ( ANIMCHANNEL_TORSO, transAnim ) ) {
				postureCurrent = postureIdeal;
				OnPostureChange ( );
				return SRESULT_DONE;
			}

			if ( postureCurrent < AIPOSTURE_STAND_COVER_LEFT
				|| postureCurrent > AIPOSTURE_CROUCH_COVER_RIGHT
				|| (postureIdeal != AIPOSTURE_STAND && postureIdeal != AIPOSTURE_RELAXED && postureIdeal != AIPOSTURE_CROUCH) )
			{
				// FIXME: TEMPORARY UNTIL ANIM IS FIXED TO NOT HAVE ORIGIN TRANSLATION
				move.fl.allowAnimMove = false;
			} else {
				//no need to play cover-to-stand/relaxed transition if:
					//scripted...
					//or we're moving already...
					//or turning away from our old cover direction...
				if ( aifl.scripted
					/*|| (move.fl.moving&&!move.fl.blocked)
					|| (fabs(move.current_yaw-move.ideal_yaw) > 30.0f && (move.moveCommand == MOVE_FACE_ENEMY||move.moveCommand == MOVE_FACE_ENTITY))*/ ) {
					postureCurrent = postureIdeal;
					OnPostureChange ( );
					return SRESULT_DONE;
				}
			}

			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, transAnim, parms.blendFrames );
			if ( postureCurrent >= AIPOSTURE_STAND_COVER_LEFT
				&& postureCurrent <= AIPOSTURE_CROUCH_COVER_RIGHT 
				&& postureIdeal == AIPOSTURE_RELAXED ) {
				//we need to also play stand_to_relaxed at the end...
				return SRESULT_STAGE ( STAGE_WAIT_RELAXED );
			}
			return SRESULT_STAGE ( STAGE_WAIT );
		}
					
		case STAGE_WAIT_RELAXED:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				if ( HasAnim ( ANIMCHANNEL_TORSO, "stand_to_relaxed" ) ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "stand_to_relaxed", parms.blendFrames );
				}
				return SRESULT_STAGE ( STAGE_WAIT );
			}
			return SRESULT_WAIT;

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				postureCurrent = postureIdeal;
				OnPostureChange ( );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvAITactical::State_Torso_RangedAttack
================
*/
stateResult_t rvAITactical::State_Torso_RangedAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_START,
		STAGE_START_WAIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
		STAGE_END,
		STAGE_END_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_START:
			// If moving switch to the moving ranged attack (torso only)
			if ( move.fl.moving && FacingIdeal() ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_MovingRangedAttack", parms.blendFrames );
				return SRESULT_DONE;
			}

			// Full body animations						
			DisableAnimState ( ANIMCHANNEL_LEGS );

			CalculateShots ( "range_attack" );

			// Attack lead in animation?
			if ( HasAnim ( ANIMCHANNEL_TORSO, "range_attack_start", true ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_start", parms.blendFrames );
				return SRESULT_STAGE ( STAGE_START_WAIT );
			}

			return SRESULT_STAGE ( STAGE_SHOOT );
			
		case STAGE_START_WAIT:
			// When the pre shooting animation is done head over to shooting
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_SHOOT );
			}
			return SRESULT_WAIT;
		
		case STAGE_SHOOT:
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack", 0 );
			UseAmmo ( 1 );
			return SRESULT_STAGE ( STAGE_SHOOT_WAIT );
		
		case STAGE_SHOOT_WAIT:
			// When the shoot animation is done either play another shot animation
			// or finish up with post_shooting
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				// If our enemy is no longer in our fov we can stop shooting
				if ( !enemy.fl.inFov ) { 
					return SRESULT_STAGE ( STAGE_END );
				} else if ( enemy.fl.dead ) {
					//if enemy is dead, stop shooting soon
					if ( shots > 5 ) {
						shots = gameLocal.random.RandomInt(6);
					}
				}
				if ( shots <= 0 ) {
					return SRESULT_STAGE ( STAGE_END );
				}
				return SRESULT_STAGE ( STAGE_SHOOT);
			}
			return SRESULT_WAIT;
			
		case STAGE_END:
			// Attack lead in animation?
			if ( HasAnim ( ANIMCHANNEL_TORSO, "range_attack_end", true ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_end", parms.blendFrames );
				return SRESULT_STAGE ( STAGE_END_WAIT );
			}			
			return SRESULT_DONE;
			
		case STAGE_END_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvAITactical::State_Torso_MovingRangedAttack
================
*/
stateResult_t rvAITactical::State_Torso_MovingRangedAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_SHOOT,
		STAGE_SHOOT_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			CalculateShots ( "range_attack_torso" );
			return SRESULT_STAGE ( STAGE_SHOOT );
			
		case STAGE_SHOOT:
			UseAmmo ( 1 );
			PlayAnim ( ANIMCHANNEL_TORSO, "range_attack_torso", 0 );
			return SRESULT_STAGE ( STAGE_SHOOT_WAIT );
		
		case STAGE_SHOOT_WAIT:
			// When the shoot animation is done either play another shot animation
			// or finish up with post_shooting
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( enemy.fl.dead ) {
					//if enemy is dead, stop shooting soon
					if ( shots > 5 ) {
						shots = gameLocal.random.RandomInt(6);
					}
				}
				if ( shots <= 0 || !enemy.fl.inFov ) {
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
rvAITactical::State_Torso_Reload
================
*/
stateResult_t rvAITactical::State_Torso_Reload ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:			
			DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "reload", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 2 ) ) {
				ammo = spawnArgs.GetInt ( "ammo" );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;			
	}
	return SRESULT_ERROR;
}

/*
================
rvAITactical::State_Torso_Cover_LeanLeftAttack
================
*/
stateResult_t rvAITactical::State_Torso_Cover_LeanLeftAttack ( const stateParms_t& parms ) {
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_RangedAttack", parms.blendFrames );
	return SRESULT_DONE;
}
	
/*
================
rvAITactical::State_Torso_Cover_LeanRightAttack
================
*/
stateResult_t rvAITactical::State_Torso_Cover_LeanRightAttack ( const stateParms_t& parms ) {
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_RangedAttack", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
rvAITactical::State_Torso_Cover_LeanAttack
================
*/
stateResult_t rvAITactical::State_Torso_Cover_LeanAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_OUT,
		STAGE_OUTWAIT,
		STAGE_FIRE,
		STAGE_FIREWAIT,
		STAGE_IN,
		STAGE_INWAIT,
	};
	switch ( parms.stage ) {
		case STAGE_OUT:
			DisableAnimState ( ANIMCHANNEL_LEGS );
			
			// The lean out animation cannot blend with any other animations since
			// it is essential that the movement delta out match the one back in.  Therefore
			// we force the legs and torso to be stoped before playing any animations
			torsoAnim.StopAnim ( 0 );
			legsAnim.StopAnim ( 0 );
			
			PlayAnim ( ANIMCHANNEL_TORSO, "lean_out", 0 );
			return SRESULT_STAGE ( STAGE_OUTWAIT );
	
		case STAGE_OUTWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				// Random number of shots
				CalculateShots ( "lean_attack" );
				return SRESULT_STAGE ( STAGE_FIRE );
			}
			return SRESULT_WAIT;
		
		case STAGE_FIRE:
			UseAmmo ( 1 );
			PlayAnim ( ANIMCHANNEL_TORSO, "lean_attack", 0 );
			return SRESULT_STAGE ( STAGE_FIREWAIT );
		
		case STAGE_FIREWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( enemy.fl.dead ) {
					//if enemy is dead, stop shooting soon
					if ( shots > 5 ) {
						shots = gameLocal.random.RandomInt(6);
					}
				}
				if ( shots > 0 ) {
					return SRESULT_STAGE ( STAGE_FIRE );
				}
				return SRESULT_STAGE ( STAGE_IN );
			}
			return SRESULT_WAIT;

		case STAGE_IN:
			PlayAnim ( ANIMCHANNEL_TORSO, "lean_in", 0 );
			return SRESULT_STAGE ( STAGE_INWAIT );
		
		case STAGE_INWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;	
}

/*
================
rvAITactical::State_Torso_Cover_Peek
================
*/
stateResult_t rvAITactical::State_Torso_Cover_Peek ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			DisableAnimState ( ANIMCHANNEL_LEGS );						
			if ( !PlayAnim ( ANIMCHANNEL_TORSO, "peek", parms.blendFrames ) ) {
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
rvAITactical::State_Frame_Peek
================
*/
stateResult_t rvAITactical::State_Frame_Peek ( const stateParms_t& parms ) {
	CheckForEnemy ( true, true );
	return SRESULT_OK;
}

/*
================
rvAITactical::Event_ForcePosture
================
*/
void rvAITactical::Event_ForcePosture ( int posture ) {
	postureForce = (aiPosture_t)posture;
}

/*
===================
rvAITactical::IsCrouching
===================
*/
bool rvAITactical::IsCrouching( void ) const {
	if ( postureCurrent == AIPOSTURE_CROUCH
		|| postureCurrent == AIPOSTURE_CROUCH_COVER
		|| postureCurrent == AIPOSTURE_CROUCH_COVER_LEFT
		|| postureCurrent == AIPOSTURE_CROUCH_COVER_RIGHT ) {
		return true;
	}
	return idAI::IsCrouching();
}

/*
================
rvAITactical::Event_PostSpawn
================
*/
void rvAITactical::Event_PostSpawn( void ) {
	idAI::Event_PostSpawn();
	if ( team == AITEAM_MARINE && healthRegenEnabled )
	{//regen-enabled buddy marine
		if ( CheckDeathCausesMissionFailure() )
		{//who is important to a mission
			if ( g_skill.GetInteger() > 2 )
			{//on impossible
				health *= 1.5f;
				healthRegen *= 1.5f;
			}
			else if ( g_skill.GetInteger() > 1 )
			{//on hard
				health *= 1.2f;
				healthRegen *= 1.25f;
			}
		}
	}
}