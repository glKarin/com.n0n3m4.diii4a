#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

CLASS_STATES_DECLARATION ( idPlayer )

	// Wait States
	STATE ( "Wait_Alive",					idPlayer::State_Wait_Alive )
	STATE ( "Wait_ReloadAnim",				idPlayer::State_Wait_ReloadAnim )

	// Torso States
	STATE ( "Torso_Idle",					idPlayer::State_Torso_Idle )
	STATE ( "Torso_IdleThink",				idPlayer::State_Torso_IdleThink )
	STATE ( "Torso_Teleport",				idPlayer::State_Torso_Teleport )
	STATE ( "Torso_RaiseWeapon",			idPlayer::State_Torso_RaiseWeapon )
	STATE ( "Torso_LowerWeapon",			idPlayer::State_Torso_LowerWeapon )
	STATE ( "Torso_Fire",					idPlayer::State_Torso_Fire )
	STATE ( "Torso_Fire_Windup",			idPlayer::State_Torso_Fire_Windup )
	STATE ( "Torso_Reload",					idPlayer::State_Torso_Reload )
	STATE ( "Torso_Pain",					idPlayer::State_Torso_Pain )
	STATE ( "Torso_Dead",					idPlayer::State_Torso_Dead )
	STATE ( "Torso_Emote",					idPlayer::State_Torso_Emote )

	// Leg States
	STATE ( "Legs_Idle",					idPlayer::State_Legs_Idle )
	STATE ( "Legs_Crouch",					idPlayer::State_Legs_Crouch )
	STATE ( "Legs_Uncrouch",				idPlayer::State_Legs_Uncrouch )
	STATE ( "Legs_Run_Forward",				idPlayer::State_Legs_Run_Forward )
	STATE ( "Legs_Run_Backward",			idPlayer::State_Legs_Run_Backward )
	STATE ( "Legs_Run_Left",				idPlayer::State_Legs_Run_Left )
	STATE ( "Legs_Run_Right",				idPlayer::State_Legs_Run_Right )
	STATE ( "Legs_Walk_Forward",			idPlayer::State_Legs_Walk_Forward )
	STATE ( "Legs_Walk_Backward",			idPlayer::State_Legs_Walk_Backward )
	STATE ( "Legs_Walk_Left",				idPlayer::State_Legs_Walk_Left )
	STATE ( "Legs_Walk_Right",				idPlayer::State_Legs_Walk_Right )
	STATE ( "Legs_Crouch_Idle",				idPlayer::State_Legs_Crouch_Idle )
	STATE ( "Legs_Crouch_Forward",			idPlayer::State_Legs_Crouch_Forward )
	STATE ( "Legs_Crouch_Backward",			idPlayer::State_Legs_Crouch_Backward )
	STATE ( "Legs_Fall",					idPlayer::State_Legs_Fall )
	STATE ( "Legs_Jump",					idPlayer::State_Legs_Jump )
	STATE ( "Legs_Fall",					idPlayer::State_Legs_Fall )
	STATE ( "Legs_Land",					idPlayer::State_Legs_Land )
	STATE ( "Legs_Dead",					idPlayer::State_Legs_Dead )

END_CLASS_STATES

/*
================
idPlayer::State_Torso_Idle
================
*/
stateResult_t idPlayer::State_Torso_Idle ( const stateParms_t& parms ) {
	if ( SRESULT_WAIT == State_Torso_IdleThink ( parms ) ) {
		PlayCycle ( ANIMCHANNEL_TORSO, "idle", parms.blendFrames  );
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_IdleThink", parms.blendFrames );
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_IdleThink
================
*/
stateResult_t idPlayer::State_Torso_IdleThink ( const stateParms_t& parms ) {
	if ( pfl.teleport ) {
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Teleport", 0 );
		return SRESULT_DONE;
	}
	if ( pfl.weaponFired ) {
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Fire", 0 );
		return SRESULT_DONE;
	}
	
	if( pfl.attackHeld && weapon && weapon->wfl.hasWindupAnim)	{
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Fire_Windup", 0 );
		return SRESULT_DONE;
	}
	
	if ( pfl.attackHeld && HasAnim ( ANIMCHANNEL_TORSO, "startfire" ) ) {
   		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Fire_StartFire", 2 );
   		return SRESULT_DONE;
   	}
   	if ( pfl.pain ) {
  		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Pain", 0 );
  		return SRESULT_DONE;
   	}
	if ( emote != PE_NONE ) {
		PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Emote", 5 );
		return SRESULT_DONE;
	}

   	
   	return SRESULT_WAIT;
}

/*
================
idPlayer::State_Torso_Teleport
================
*/
stateResult_t idPlayer::State_Torso_Teleport ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:		
			pfl.teleport = false;
			PlayAnim ( ANIMCHANNEL_TORSO, "teleport", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Torso_RaiseWeapon
================
*/
stateResult_t idPlayer::State_Torso_RaiseWeapon ( const stateParms_t& parms ) {
	PlayAnim( ANIMCHANNEL_TORSO, "raise", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 3 );
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 3 );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_LowerWeapon
================
*/
stateResult_t idPlayer::State_Torso_LowerWeapon ( const stateParms_t& parms ) {	
	PlayAnim( ANIMCHANNEL_TORSO, "lower", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 3 );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_Fire
================
*/
stateResult_t idPlayer::State_Torso_Fire ( const stateParms_t& parms ) {
	enum {
		TORSO_FIRE_INIT,
		TORSO_FIRE_WAIT,
		TORSO_FIRE_AIM,
		TORSO_FIRE_AIMWAIT
	};

	switch ( parms.stage ) {
		// Start the firing sequence
		case TORSO_FIRE_INIT:
 			PlayAnim ( ANIMCHANNEL_TORSO, "fire", parms.blendFrames );
			pfl.weaponFired = false;
			return SRESULT_STAGE(TORSO_FIRE_WAIT);
		
		// Wait for the firing animation to be finished
		case TORSO_FIRE_WAIT:
			if ( pfl.weaponFired ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Fire", parms.blendFrames );
				return SRESULT_DONE;
			}
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {	
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
			
		// Keep the gun aimed but dont shoot
		case TORSO_FIRE_AIM:
			PlayAnim ( ANIMCHANNEL_TORSO, "aim", 3 );
			return SRESULT_STAGE(TORSO_FIRE_AIMWAIT);
			
		// Keep the gun aimed as long as the attack button is held and nothing is firing
		case TORSO_FIRE_AIMWAIT:
			if ( pfl.weaponFired ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Fire", 3 );
				return SRESULT_DONE;
			} else if ( !pfl.attackHeld ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}		

/*
================
idPlayer::State_Torso_Fire_Windup
================
*/
stateResult_t idPlayer::State_Torso_Fire_Windup ( const stateParms_t& parms ) {
	enum {
		TORSO_FIRE_INIT,
		TORSO_WINDUP_WAIT,
		TORSO_FIRE_LOOP,
		TORSO_FIRE_WAIT,
		TORSO_WINDDOWN_START,
		TORSO_WINDDOWN_WAIT,
		TORSO_FIRE_AIM,
		TORSO_FIRE_AIMWAIT
	};

	switch ( parms.stage ) {
		// Start the firing sequence
		case TORSO_FIRE_INIT:
			//jshepard: HACK for now we're blending here, but we need to support charge up anims here
 			PlayAnim ( ANIMCHANNEL_TORSO, "fire", 4 );
			pfl.weaponFired = false;
			return SRESULT_STAGE(TORSO_WINDUP_WAIT);

		// wait for the windup anim to end, or the attackHeld to be false.
		case TORSO_WINDUP_WAIT:
			if( !pfl.attackHeld )	{
				return SRESULT_STAGE( TORSO_WINDDOWN_START );
			}
			if( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames )) {
				return SRESULT_STAGE( TORSO_FIRE_LOOP );								
			}
			return SRESULT_WAIT;
		
		// play the firing loop
		case TORSO_FIRE_LOOP:
			if( !pfl.attackHeld )	{
				return SRESULT_STAGE( TORSO_WINDDOWN_START );
			}
			PlayAnim ( ANIMCHANNEL_TORSO, "fire", parms.blendFrames );
			return SRESULT_STAGE( TORSO_FIRE_WAIT );
			
		// loop the fire anim
		case TORSO_FIRE_WAIT:
			if( !pfl.attackHeld )	{
				return SRESULT_STAGE( TORSO_WINDDOWN_START );
			}
			//loop the attack anim
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {	
				PlayAnim ( ANIMCHANNEL_TORSO, "fire", parms.blendFrames );
				return SRESULT_STAGE( TORSO_FIRE_WAIT );
			}
			return SRESULT_WAIT;

		//wind down
		case TORSO_WINDDOWN_START:
			//jshepard: HACK just blend back into idle for now, we could support winddown anims here.
			PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
			pfl.weaponFired = false;
			return SRESULT_DONE;
		
	}
	return SRESULT_DONE;
}		
/*
================
idPlayer::State_Torso_Reload
================
*/
stateResult_t idPlayer::State_Torso_Reload ( const stateParms_t& parms ) {
	enum {
		TORSO_RELOAD_START,
		TORSO_RELOAD_STARTWAIT,
		TORSO_RELOAD_LOOP,
		TORSO_RELOAD_LOOPWAIT,
		TORSO_RELOAD_WAIT,
		TORSO_RELOAD_END
	};
	switch ( parms.stage ) {
		// Start the reload by either playing the reload animation or the reload_start animation
		case TORSO_RELOAD_START:
			if ( HasAnim ( ANIMCHANNEL_TORSO, "reload_start" ) ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "reload_start", parms.blendFrames );
				return SRESULT_STAGE(TORSO_RELOAD_STARTWAIT);
			}
			
			PlayAnim( ANIMCHANNEL_TORSO, "reload", parms.blendFrames );
			return SRESULT_STAGE(TORSO_RELOAD_WAIT);
			
		// Wait for the reload_start animation to finish and transition to reload_loop
		case TORSO_RELOAD_STARTWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE(TORSO_RELOAD_LOOP);
			} else if ( pfl.weaponFired ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 3 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
			
		// Play a single reload from the reload loop
		case TORSO_RELOAD_LOOP:
			if ( !pfl.reload ) {
				return SRESULT_STAGE(TORSO_RELOAD_END);
			}
			PlayAnim ( ANIMCHANNEL_TORSO, "reload_loop", 0 );
			return SRESULT_STAGE(TORSO_RELOAD_LOOPWAIT);

		// Wait for the looping reload to finish and either start a new one or end the reload
		case TORSO_RELOAD_LOOPWAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE(TORSO_RELOAD_LOOP);
			} else if ( pfl.weaponFired ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 3 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;

		// End the reload
		case TORSO_RELOAD_END:
			if ( pfl.weaponFired ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 3 );
				return SRESULT_DONE;
			}

			PlayAnim( ANIMCHANNEL_TORSO, "reload_end", 3 );
			return SRESULT_STAGE(TORSO_RELOAD_WAIT);
		
		// Wait for reload to finish (called by both reload_end and reload)
		case TORSO_RELOAD_WAIT:
			if ( pfl.weaponFired || AnimDone ( ANIMCHANNEL_TORSO, 3 ) ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 3 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_Pain
================
*/
stateResult_t idPlayer::State_Torso_Pain ( const stateParms_t& parms ) {
	PlayAnim ( ANIMCHANNEL_TORSO, painAnim.Length()?painAnim:"pain", parms.blendFrames );
	PostAnimState ( ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 4  );
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_Dead
================
*/
stateResult_t idPlayer::State_Torso_Dead ( const stateParms_t& parms ) {
	PostAnimState ( ANIMCHANNEL_TORSO, "Wait_Alive", 0 );
	PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Torso_Emote
================
*/
stateResult_t idPlayer::State_Torso_Emote ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if( emote == PE_GRAB_A ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "grab_a", parms.blendFrames );
			} else if( emote == PE_GRAB_B ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "grab_b", parms.blendFrames );
			} else if( emote == PE_SALUTE ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "salute", parms.blendFrames );
			} else if( emote == PE_CHEER ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "cheer", parms.blendFrames );
			} else if( emote == PE_TAUNT ) {
				PlayAnim ( ANIMCHANNEL_TORSO, "taunt", parms.blendFrames );
			}

			emote = PE_NONE;
			
			return SRESULT_STAGE ( STAGE_WAIT );

		case STAGE_WAIT:
			if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
				PostAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
===============================================================================

	AI Leg States

===============================================================================
*/

/*
================
idPlayer::IsLegsIdle
================
*/
bool idPlayer::IsLegsIdle ( bool crouching ) const {
	return ( (pfl.crouch == crouching) && pfl.onGround && (pfl.forward==pfl.backward) && (pfl.strafeLeft==pfl.strafeRight) );
}

/*
================
idPlayer::State_Legs_Idle
================
*/
stateResult_t idPlayer::State_Legs_Idle ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( pfl.crouch ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch", parms.blendFrames );
				return SRESULT_DONE;
			}
			IdleAnim ( ANIMCHANNEL_LEGS, "idle", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			// If now crouching go back to idle so we can transition to crouch 
			if ( pfl.crouch ) {
 				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch", 4 );
				return SRESULT_DONE;
			} else if ( pfl.jump ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Jump", 4 );
				return SRESULT_DONE;
			} else if ( !pfl.onGround ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Fall", 4 );
				return SRESULT_DONE;
			}else if ( pfl.forward && !pfl.backward ) {
				if( usercmd.buttons & BUTTON_RUN ) {
 					PlayCycle( ANIMCHANNEL_LEGS, "run_forward", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Forward", parms.blendFrames );
				} else {
					PlayCycle( ANIMCHANNEL_LEGS, "walk_forward", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Forward", parms.blendFrames );
				}
				
				return SRESULT_DONE;
			} else if ( pfl.backward && !pfl.forward ) {
				if( usercmd.buttons & BUTTON_RUN ) {
					PlayCycle( ANIMCHANNEL_LEGS, "run_backwards", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Backward", parms.blendFrames );
				} else {
					PlayCycle( ANIMCHANNEL_LEGS, "walk_backwards", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Backward", parms.blendFrames );
				}
			
				return SRESULT_DONE;
			} else if ( pfl.strafeLeft && !pfl.strafeRight ) {
				if( usercmd.buttons & BUTTON_RUN ) {
					PlayCycle( ANIMCHANNEL_LEGS, "run_strafe_left", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Left", parms.blendFrames );
				} else {
					PlayCycle( ANIMCHANNEL_LEGS, "walk_left", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Left", parms.blendFrames );
				}
				
				return SRESULT_DONE;
			} else if ( pfl.strafeRight && !pfl.strafeLeft ) {
				if( usercmd.buttons & BUTTON_RUN ) {
					PlayCycle( ANIMCHANNEL_LEGS, "run_strafe_right", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Right", parms.blendFrames );
				} else {
					PlayCycle( ANIMCHANNEL_LEGS, "walk_right", parms.blendFrames );
					PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Right", parms.blendFrames );
				}

				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
		
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Crouch_Idle
================
*/
stateResult_t idPlayer::State_Legs_Crouch_Idle ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:			
			if ( !pfl.crouch ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Uncrouch", parms.blendFrames );
				return SRESULT_DONE;
			}
			PlayCycle ( ANIMCHANNEL_LEGS, "crouch", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( !pfl.crouch || pfl.jump ) {
 				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Uncrouch", 4 );
				return SRESULT_DONE;
			} else if ( (pfl.forward && !pfl.backward) || (pfl.strafeLeft != pfl.strafeRight) ) {				
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch_Forward", parms.blendFrames );
				return SRESULT_DONE;
			} else if ( pfl.backward && !pfl.forward ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch_Backward", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Crouch
================
*/
stateResult_t idPlayer::State_Legs_Crouch ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			PlayAnim ( ANIMCHANNEL_LEGS, "crouch_down", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( !IsLegsIdle ( true ) || AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Uncrouch
================
*/
stateResult_t idPlayer::State_Legs_Uncrouch ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			PlayAnim ( ANIMCHANNEL_LEGS, "crouch_up", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( !IsLegsIdle ( false ) || AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Run_Forward
================
*/
stateResult_t idPlayer::State_Legs_Run_Forward ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && !pfl.backward && pfl.forward ) {
		if( usercmd.buttons & BUTTON_RUN ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "walk_forward", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Forward", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Run_Backward
================
*/
stateResult_t idPlayer::State_Legs_Run_Backward ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && !pfl.forward && pfl.backward ) {
		if( usercmd.buttons & BUTTON_RUN ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "walk_backwards", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Backward", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Run_Left
================
*/
stateResult_t idPlayer::State_Legs_Run_Left ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && (pfl.forward == pfl.backward) && pfl.strafeLeft && !pfl.strafeRight ) {
		if( usercmd.buttons & BUTTON_RUN ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "walk_left", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Left", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Run_Right
================
*/
stateResult_t idPlayer::State_Legs_Run_Right ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && (pfl.forward == pfl.backward) && pfl.strafeRight && !pfl.strafeLeft ) {
		if( usercmd.buttons & BUTTON_RUN ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "walk_right", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Walk_Right", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Walk_Forward
================
*/
stateResult_t idPlayer::State_Legs_Walk_Forward ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && !pfl.backward && pfl.forward ) {
		if( !(usercmd.buttons & BUTTON_RUN) ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "run_forward", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Forward", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Walk_Backward
================
*/
stateResult_t idPlayer::State_Legs_Walk_Backward ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && !pfl.forward && pfl.backward ) {
		if( !(usercmd.buttons & BUTTON_RUN) ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "run_backwards", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Backward", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Walk_Left
================
*/
stateResult_t idPlayer::State_Legs_Walk_Left ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && (pfl.forward == pfl.backward) && pfl.strafeLeft && !pfl.strafeRight ) {
		if( !(usercmd.buttons & BUTTON_RUN) ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "run_strafe_left", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Left", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Walk_Right
================
*/
stateResult_t idPlayer::State_Legs_Walk_Right ( const stateParms_t& parms ) {
	if ( !pfl.jump && pfl.onGround && !pfl.crouch && (pfl.forward == pfl.backward) && pfl.strafeRight && !pfl.strafeLeft ) {
		if( !(usercmd.buttons & BUTTON_RUN) ) {
			return SRESULT_WAIT;
		} else {
			PlayCycle( ANIMCHANNEL_LEGS, "run_strafe_right", parms.blendFrames );
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Run_Right", parms.blendFrames );
			return SRESULT_DONE;
		}
	}
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Crouch_Forward
================
*/
stateResult_t idPlayer::State_Legs_Crouch_Forward ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			PlayCycle( ANIMCHANNEL_LEGS, "crouch_walk", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( !pfl.jump && pfl.onGround && pfl.crouch && ((!pfl.backward && pfl.forward) || (pfl.strafeLeft != pfl.strafeRight)) ) {
				return SRESULT_WAIT;
			}
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", 2 );
			return SRESULT_DONE;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Crouch_Backward
================
*/
stateResult_t idPlayer::State_Legs_Crouch_Backward ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			PlayCycle( ANIMCHANNEL_LEGS, "crouch_walk_backward", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( !pfl.jump && pfl.onGround && pfl.crouch && !pfl.forward && pfl.backward ) {
				return SRESULT_WAIT;
			}
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", parms.blendFrames );
			return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Jump
================
*/
stateResult_t idPlayer::State_Legs_Jump ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			// prevent infinite recursion
			pfl.jump = false;
			if ( pfl.run ) {
				PlayAnim ( ANIMCHANNEL_LEGS, "run_jump", parms.blendFrames );
			} else {
				PlayAnim ( ANIMCHANNEL_LEGS, "jump", parms.blendFrames );
			}
			return SRESULT_STAGE ( STAGE_WAIT );
		
		case STAGE_WAIT:
			if ( pfl.onGround ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Land", 4 );
				return SRESULT_DONE;
			}
			if ( AnimDone ( ANIMCHANNEL_LEGS, 4 ) ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Fall", 4 );
				return SRESULT_DONE;
			}			
			return SRESULT_WAIT;
	}
 	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Fall
================
*/
stateResult_t idPlayer::State_Legs_Fall ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( pfl.onGround ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Land", 2 );
				return SRESULT_DONE;
			}
			PlayCycle ( ANIMCHANNEL_LEGS, "fall", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAIT );			
		case STAGE_WAIT:
			if ( pfl.onGround ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Land", 2 );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Legs_Land
================
*/
stateResult_t idPlayer::State_Legs_Land ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			if ( IsLegsIdle ( false ) && ( pfl.hardLanding || pfl.softLanding ) ) {					
				if ( pfl.hardLanding ) {
					PlayAnim ( ANIMCHANNEL_LEGS, "hard_land", parms.blendFrames );
				} else {
					PlayAnim ( ANIMCHANNEL_LEGS, "soft_land", parms.blendFrames );
				}
				return SRESULT_STAGE ( STAGE_WAIT );
			}
			PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 4 );
			return SRESULT_DONE;
			
		case STAGE_WAIT:
			if ( !IsLegsIdle ( false ) || AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
				PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", parms.blendFrames );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::State_Legs_Dead
================
*/
stateResult_t idPlayer::State_Legs_Dead ( const stateParms_t& parms ) {
	PostAnimState ( ANIMCHANNEL_LEGS, "Wait_Alive", 0 );
	PostAnimState ( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Wait_Alive

Waits until the player is alive again.
================
*/
stateResult_t idPlayer::State_Wait_Alive ( const stateParms_t& parms ) {
	if ( pfl.dead ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::State_Wait_ReloadAnim
================
*/
stateResult_t idPlayer::State_Wait_ReloadAnim ( const stateParms_t& parms ) {
	// The gun firing can cancel any of the relod animations
	if ( pfl.weaponFired ) {		
		SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Idle", parms.blendFrames );
		return SRESULT_DONE;
	}
	
	// wait for the animation to finish
	if ( AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
		return SRESULT_DONE;
	}
	
	return SRESULT_WAIT;
}
