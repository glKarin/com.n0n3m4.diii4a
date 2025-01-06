// Player_Anim.cpp
//


#pragma hdrstop

#include "../Game_local.h"

/*
================
idPlayer::Torso_Idle
================
*/
stateResult_t idPlayer::Torso_Idle(stateParms_t* parms) {
	if (SRESULT_WAIT == Torso_IdleThink(parms)) {
		Event_PlayCycle(ANIMCHANNEL_TORSO, "idle", /* parms->blendFrames */ 0);
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_IdleThink", /* parms->blendFrames */ 0);
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_IdleThink
================
*/
stateResult_t idPlayer::Torso_IdleThink(stateParms_t* parms) {
	if (AI_TELEPORT) {
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_Teleport", 0);
		return SRESULT_DONE;
	}
	if (AI_WEAPON_FIRED) {
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_Fire", 0);
		return SRESULT_DONE;
	}

	//if (AI_ATTACK_HELD && weapon && weapon->wfl.hasWindupAnim) {
	//	PostAnimState(ANIMCHANNEL_TORSO, "Torso_Fire_Windup", 0);
	//	return SRESULT_DONE;
	//}

	if (AI_ATTACK_HELD && HasAnim(ANIMCHANNEL_TORSO, "startfire", false)) {
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_Fire_StartFire", 2);
		return SRESULT_DONE;
	}
	if (AI_PAIN) {
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_Pain", 0);
		return SRESULT_DONE;
	}
	//if (emote != PE_NONE) {
	//	PostAnimState(ANIMCHANNEL_TORSO, "Torso_Emote", 5);
	//	return SRESULT_DONE;
	//}

	return SRESULT_WAIT;
}

/*
================
idPlayer::Torso_Teleport
================
*/
stateResult_t idPlayer::Torso_Teleport(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		AI_TELEPORT = false;
		Event_PlayAnim(ANIMCHANNEL_TORSO, "teleport", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, 4)) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Torso_RaiseWeapon
================
*/
stateResult_t idPlayer::Torso_RaiseWeapon(stateParms_t* parms) {
	Event_PlayAnim(ANIMCHANNEL_TORSO, "raise", /* parms->blendFrames */ 0);
	PostAnimState(ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 3);
	PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 3);
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_LowerWeapon
================
*/
stateResult_t idPlayer::Torso_LowerWeapon(stateParms_t* parms) {
	Event_PlayAnim(ANIMCHANNEL_TORSO, "lower", /* parms->blendFrames */ 0);
	PostAnimState(ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 3);
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_Fire
================
*/
stateResult_t idPlayer::Torso_Fire(stateParms_t* parms) {
	enum {
		TORSO_FIRE_INIT,
		TORSO_FIRE_WAIT,
		TORSO_FIRE_AIM,
		TORSO_FIRE_AIMWAIT
	};

	switch (parms->stage) {
		// Start the firing sequence
	case TORSO_FIRE_INIT:
		Event_PlayAnim(ANIMCHANNEL_TORSO, "fire", /* parms->blendFrames */ 0);
		AI_WEAPON_FIRED = false;
		SRESULT_STAGE(TORSO_FIRE_WAIT);

		// Wait for the firing animation to be finished
	case TORSO_FIRE_WAIT:
		if (AI_WEAPON_FIRED) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Fire", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		if (AnimDone(ANIMCHANNEL_TORSO, /* parms->blendFrames */ 0)) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;

		// Keep the gun aimed but dont shoot
	case TORSO_FIRE_AIM:
		Event_PlayAnim(ANIMCHANNEL_TORSO, "aim", 3);
		SRESULT_STAGE(TORSO_FIRE_AIMWAIT);

		// Keep the gun aimed as long as the attack button is held and nothing is firing
	case TORSO_FIRE_AIMWAIT:
		if (AI_WEAPON_FIRED) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Fire", 3);
			return SRESULT_DONE;
		}
		else if (!AI_ATTACK_HELD) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_Fire_Windup
================
*/
stateResult_t idPlayer::Torso_Fire_Windup(stateParms_t* parms) {
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

	switch (parms->stage) {
		// Start the firing sequence
	case TORSO_FIRE_INIT:
		//jshepard: HACK for now we're blending here, but we need to support charge up anims here
		Event_PlayAnim(ANIMCHANNEL_TORSO, "fire", 4);
		AI_WEAPON_FIRED = false;
		SRESULT_STAGE(TORSO_WINDUP_WAIT);

		// wait for the windup anim to end, or the attackHeld to be false.
	case TORSO_WINDUP_WAIT:
		if (!AI_ATTACK_HELD) {
			SRESULT_STAGE(TORSO_WINDDOWN_START);
		}
		if (AnimDone(ANIMCHANNEL_TORSO, /* parms->blendFrames */ 0)) {
			SRESULT_STAGE(TORSO_FIRE_LOOP);
		}
		return SRESULT_WAIT;

		// play the firing loop
	case TORSO_FIRE_LOOP:
		if (!AI_ATTACK_HELD) {
			SRESULT_STAGE(TORSO_WINDDOWN_START);
		}
		Event_PlayAnim(ANIMCHANNEL_TORSO, "fire", /* parms->blendFrames */ 0);
		SRESULT_STAGE(TORSO_FIRE_WAIT);

		// loop the fire anim
	case TORSO_FIRE_WAIT:
		if (!AI_ATTACK_HELD) {
			SRESULT_STAGE(TORSO_WINDDOWN_START);
		}
		//loop the attack anim
		if (AnimDone(ANIMCHANNEL_TORSO, /* parms->blendFrames */ 0)) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "fire", /* parms->blendFrames */ 0);
			SRESULT_STAGE(TORSO_FIRE_WAIT);
		}
		return SRESULT_WAIT;

		//wind down
	case TORSO_WINDDOWN_START:
		//jshepard: HACK just blend back into idle for now, we could support winddown anims here.
		PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		AI_WEAPON_FIRED = false;
		return SRESULT_DONE;

	}
	return SRESULT_DONE;
}
/*
================
idPlayer::Torso_Reload
================
*/
stateResult_t idPlayer::Torso_Reload(stateParms_t* parms) {
	enum {
		TORSO_RELOAD_START,
		TORSO_RELOAD_STARTWAIT,
		TORSO_RELOAD_LOOP,
		TORSO_RELOAD_LOOPWAIT,
		TORSO_RELOAD_WAIT,
		TORSO_RELOAD_END
	};
	switch (parms->stage) {
		// Start the reload by either playing the reload animation or the reload_start animation
	case TORSO_RELOAD_START:
		if (HasAnim(ANIMCHANNEL_TORSO, "reload_start", false)) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "reload_start", /* parms->blendFrames */ 0);
			SRESULT_STAGE(TORSO_RELOAD_STARTWAIT);
		}

		Event_PlayAnim(ANIMCHANNEL_TORSO, "reload", /* parms->blendFrames */ 0);
		SRESULT_STAGE(TORSO_RELOAD_WAIT);

		// Wait for the reload_start animation to finish and transition to reload_loop
	case TORSO_RELOAD_STARTWAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			SRESULT_STAGE(TORSO_RELOAD_LOOP);
		}
		else if (AI_WEAPON_FIRED) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 3);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;

		// Play a single reload from the reload loop
	case TORSO_RELOAD_LOOP:
		if (!AI_RELOAD) {
			SRESULT_STAGE(TORSO_RELOAD_END);
		}
		Event_PlayAnim(ANIMCHANNEL_TORSO, "reload_loop", 0);
		SRESULT_STAGE(TORSO_RELOAD_LOOPWAIT);

		// Wait for the looping reload to finish and either start a new one or end the reload
	case TORSO_RELOAD_LOOPWAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, 0)) {
			SRESULT_STAGE(TORSO_RELOAD_LOOP);
		}
		else if (AI_WEAPON_FIRED) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 3);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;

		// End the reload
	case TORSO_RELOAD_END:
		if (AI_WEAPON_FIRED) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 3);
			return SRESULT_DONE;
		}

		Event_PlayAnim(ANIMCHANNEL_TORSO, "reload_end", 3);
		SRESULT_STAGE(TORSO_RELOAD_WAIT);

		// Wait for reload to finish (called by both reload_end and reload)
	case TORSO_RELOAD_WAIT:
		if (AI_WEAPON_FIRED || AnimDone(ANIMCHANNEL_TORSO, 3)) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 3);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}

	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_Pain
================
*/
stateResult_t idPlayer::Torso_Pain(stateParms_t* parms) {
	Event_PlayAnim(ANIMCHANNEL_TORSO, painAnim.Length() ? painAnim : "pain", /* parms->blendFrames */ 0);
	PostAnimState(ANIMCHANNEL_TORSO, "Wait_TorsoAnim", 4);
	PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_Dead
================
*/
stateResult_t idPlayer::Torso_Dead(stateParms_t* parms) {
	PostAnimState(ANIMCHANNEL_TORSO, "Wait_Alive", 0);
	PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Torso_Emote
================
*/
stateResult_t idPlayer::Torso_Emote(stateParms_t* parms) {
#if 0
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		if (emote == PE_GRAB_A) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "grab_a", /* parms->blendFrames */ 0);
		}
		else if (emote == PE_GRAB_B) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "grab_b", /* parms->blendFrames */ 0);
		}
		else if (emote == PE_SALUTE) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "salute", /* parms->blendFrames */ 0);
		}
		else if (emote == PE_CHEER) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "cheer", /* parms->blendFrames */ 0);
		}
		else if (emote == PE_TAUNT) {
			Event_PlayAnim(ANIMCHANNEL_TORSO, "taunt", /* parms->blendFrames */ 0);
		}

		emote = PE_NONE;

		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (AnimDone(ANIMCHANNEL_TORSO, /* parms->blendFrames */ 0)) {
			PostAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
#else 
	gameLocal.Error("Not implemented!");
	return SRESULT_ERROR;
#endif
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
bool idPlayer::IsLegsIdle(bool crouching) const {
	return ((AI_CROUCH == crouching) && AI_ONGROUND && (AI_FORWARD == AI_BACKWARD) && (AI_STRAFE_LEFT == AI_STRAFE_RIGHT));
}

/*
================
idPlayer::Legs_Idle
================
*/
stateResult_t idPlayer::Legs_Idle(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		if (AI_CROUCH) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		Event_IdleAnim(ANIMCHANNEL_LEGS, "idle");
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		// If now crouching go back to idle so we can transition to crouch 
		if (AI_CROUCH) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch", 4);
			return SRESULT_DONE;
		}
		else if (AI_JUMP) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Jump", 4);
			return SRESULT_DONE;
		}
		else if (!AI_ONGROUND) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Fall", 4);
			return SRESULT_DONE;
		}
		else if (AI_FORWARD && !AI_BACKWARD) {
			if (usercmd.buttons & BUTTON_RUN) {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "run_forward", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Forward", /* parms->blendFrames */ 0);
			}
			else {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "walk", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Forward", /* parms->blendFrames */ 0);
			}

			return SRESULT_DONE;
		}
		else if (AI_BACKWARD && !AI_FORWARD) {
			if (usercmd.buttons & BUTTON_RUN) {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "run_backwards", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Backward", /* parms->blendFrames */ 0);
			}
			else {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_backwards", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Backward", /* parms->blendFrames */ 0);
			}

			return SRESULT_DONE;
		}
		else if (AI_STRAFE_LEFT && !AI_STRAFE_RIGHT) {
			if (usercmd.buttons & BUTTON_RUN) {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "run_strafe_left", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Left", /* parms->blendFrames */ 0);
			}
			else {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_left", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Left", /* parms->blendFrames */ 0);
			}

			return SRESULT_DONE;
		}
		else if (AI_STRAFE_RIGHT && !AI_STRAFE_LEFT) {
			if (usercmd.buttons & BUTTON_RUN) {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "run_strafe_right", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Right", /* parms->blendFrames */ 0);
			}
			else {
				Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_right", /* parms->blendFrames */ 0);
				PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Right", /* parms->blendFrames */ 0);
			}

			return SRESULT_DONE;
		}
		return SRESULT_WAIT;

	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Crouch_Idle
================
*/
stateResult_t idPlayer::Legs_Crouch_Idle(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		if (!AI_CROUCH) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Uncrouch", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		Event_PlayCycle(ANIMCHANNEL_LEGS, "crouch", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!AI_CROUCH || AI_JUMP) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Uncrouch", 4);
			return SRESULT_DONE;
		}
		else if ((AI_FORWARD && !AI_BACKWARD) || (AI_STRAFE_LEFT != AI_STRAFE_RIGHT)) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch_Forward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		else if (AI_BACKWARD && !AI_FORWARD) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch_Backward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Crouch
================
*/
stateResult_t idPlayer::Legs_Crouch(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		Event_PlayAnim(ANIMCHANNEL_LEGS, "crouch_down", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!IsLegsIdle(true) || AnimDone(ANIMCHANNEL_LEGS, 4)) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Uncrouch
================
*/
stateResult_t idPlayer::Legs_Uncrouch(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		Event_PlayAnim(ANIMCHANNEL_LEGS, "crouch_up", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!IsLegsIdle(false) || AnimDone(ANIMCHANNEL_LEGS, 4)) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Run_Forward
================
*/
stateResult_t idPlayer::Legs_Run_Forward(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && !AI_BACKWARD && AI_FORWARD) {
		if (usercmd.buttons & BUTTON_RUN) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "walk", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Forward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Run_Backward
================
*/
stateResult_t idPlayer::Legs_Run_Backward(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && !AI_FORWARD && AI_BACKWARD) {
		if (usercmd.buttons & BUTTON_RUN) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_backwards", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Backward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Run_Left
================
*/
stateResult_t idPlayer::Legs_Run_Left(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && (AI_FORWARD == AI_BACKWARD) && AI_STRAFE_LEFT && !AI_STRAFE_RIGHT) {
		if (usercmd.buttons & BUTTON_RUN) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_left", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Left", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Run_Right
================
*/
stateResult_t idPlayer::Legs_Run_Right(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && (AI_FORWARD == AI_BACKWARD) && AI_STRAFE_RIGHT && !AI_STRAFE_LEFT) {
		if (usercmd.buttons & BUTTON_RUN) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "walk_right", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Walk_Right", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Walk_Forward
================
*/
stateResult_t idPlayer::Legs_Walk_Forward(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && !AI_BACKWARD && AI_FORWARD) {
		if (!(usercmd.buttons & BUTTON_RUN)) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "run_forward", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Forward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Walk_Backward
================
*/
stateResult_t idPlayer::Legs_Walk_Backward(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && !AI_FORWARD && AI_BACKWARD) {
		if (!(usercmd.buttons & BUTTON_RUN)) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "run_backwards", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Backward", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Walk_Left
================
*/
stateResult_t idPlayer::Legs_Walk_Left(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && (AI_FORWARD == AI_BACKWARD) && AI_STRAFE_LEFT && !AI_STRAFE_RIGHT) {
		if (!(usercmd.buttons & BUTTON_RUN)) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "run_strafe_left", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Left", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Walk_Right
================
*/
stateResult_t idPlayer::Legs_Walk_Right(stateParms_t* parms) {
	if (!AI_JUMP && AI_ONGROUND && !AI_CROUCH && (AI_FORWARD == AI_BACKWARD) && AI_STRAFE_RIGHT && !AI_STRAFE_LEFT) {
		if (!(usercmd.buttons & BUTTON_RUN)) {
			return SRESULT_WAIT;
		}
		else {
			Event_PlayCycle(ANIMCHANNEL_LEGS, "run_strafe_right", /* parms->blendFrames */ 0);
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Run_Right", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
	}
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Crouch_Forward
================
*/
stateResult_t idPlayer::Legs_Crouch_Forward(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		Event_PlayCycle(ANIMCHANNEL_LEGS, "crouch_walk", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!AI_JUMP && AI_ONGROUND && AI_CROUCH && ((!AI_BACKWARD && AI_FORWARD) || (AI_STRAFE_LEFT != AI_STRAFE_RIGHT))) {
			return SRESULT_WAIT;
		}
		PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", 2);
		return SRESULT_DONE;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Crouch_Backward
================
*/
stateResult_t idPlayer::Legs_Crouch_Backward(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		Event_PlayCycle(ANIMCHANNEL_LEGS, "crouch_walk_backward", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (!AI_JUMP && AI_ONGROUND && AI_CROUCH && !AI_FORWARD && AI_BACKWARD) {
			return SRESULT_WAIT;
		}
		PostAnimState(ANIMCHANNEL_LEGS, "Legs_Crouch_Idle", /* parms->blendFrames */ 0);
		return SRESULT_DONE;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Jump
================
*/
stateResult_t idPlayer::Legs_Jump(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		// prevent infinite recursion
		AI_JUMP = false;
		if (AI_RUN) {
			Event_PlayAnim(ANIMCHANNEL_LEGS, "run_jump", /* parms->blendFrames */ 0);
		}
		else {
			Event_PlayAnim(ANIMCHANNEL_LEGS, "jump", /* parms->blendFrames */ 0);
		}
		SRESULT_STAGE(STAGE_WAIT);

	case STAGE_WAIT:
		if (AI_ONGROUND) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Land", 4);
			return SRESULT_DONE;
		}
		if (AnimDone(ANIMCHANNEL_LEGS, 4)) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Fall", 4);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Fall
================
*/
stateResult_t idPlayer::Legs_Fall(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		if (AI_ONGROUND) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Land", 2);
			return SRESULT_DONE;
		}
		Event_PlayCycle(ANIMCHANNEL_LEGS, "fall", /* parms->blendFrames */ 0);
		SRESULT_STAGE(STAGE_WAIT);
	case STAGE_WAIT:
		if (AI_ONGROUND) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Land", 2);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::Legs_Land
================
*/
stateResult_t idPlayer::Legs_Land(stateParms_t* parms) {
	enum {
		STAGE_INIT,
		STAGE_WAIT
	};
	switch (parms->stage) {
	case STAGE_INIT:
		if (IsLegsIdle(false) && (AI_HARDLANDING || AI_SOFTLANDING)) {
			if (AI_HARDLANDING) {
				Event_PlayAnim(ANIMCHANNEL_LEGS, "hard_land", /* parms->blendFrames */ 0);
			}
			else {
				Event_PlayAnim(ANIMCHANNEL_LEGS, "soft_land", /* parms->blendFrames */ 0);
			}
			SRESULT_STAGE(STAGE_WAIT);
		}
		PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
		return SRESULT_DONE;

	case STAGE_WAIT:
		if (!IsLegsIdle(false) || AnimDone(ANIMCHANNEL_LEGS, /* parms->blendFrames */ 0)) {
			PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", /* parms->blendFrames */ 0);
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
idPlayer::Legs_Dead
================
*/
stateResult_t idPlayer::Legs_Dead(stateParms_t* parms) {
	PostAnimState(ANIMCHANNEL_LEGS, "Wait_Alive", 0);
	PostAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
	return SRESULT_DONE;
}

/*
================
idPlayer::Wait_Alive

Waits until the player is alive again.
================
*/
stateResult_t idPlayer::Wait_Alive(stateParms_t* parms) {
	if (AI_DEAD) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idPlayer::Wait_ReloadAnim
================
*/
stateResult_t idPlayer::Wait_ReloadAnim(stateParms_t* parms) {
	// The gun firing can cancel any of the relod animations
	if (AI_WEAPON_FIRED) {
		SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", /* parms->blendFrames */ 0);
		return SRESULT_DONE;
	}

	// wait for the animation to finish
	if (AnimDone(ANIMCHANNEL_TORSO, /* parms->blendFrames */ 0)) {
		return SRESULT_DONE;
	}

	return SRESULT_WAIT;
}
