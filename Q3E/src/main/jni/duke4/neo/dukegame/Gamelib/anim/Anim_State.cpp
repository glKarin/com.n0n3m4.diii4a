
#pragma hdrstop

#include "../Game_local.h"


/***********************************************************************

	idAnimState

***********************************************************************/

/*
=====================
idAnimState::idAnimState
=====================
*/
idAnimState::idAnimState() {
	self = NULL;
	animator = NULL;
	idleAnim = true;
	disabled = true;
	channel = ANIMCHANNEL_ALL;
	animBlendFrames = 0;
	lastAnimBlendFrames = 0;
}

/*
=====================
idAnimState::~idAnimState
=====================
*/
idAnimState::~idAnimState() {
}

/*
=====================
idAnimState::Init
=====================
*/
// RAVEN BEGIN
// bdube: converted self to entity ptr so any entity can use it
void idAnimState::Init(idEntity* owner, idAnimator* _animator, int animchannel) {
// RAVEN BEGIN
	assert(owner);
	assert(_animator);
	self = owner;
	animator = _animator;
	channel = animchannel;

	stateThread.SetName(va("%s_anim_%d", owner->GetName(), animchannel));
	stateThread.SetOwner(owner);
}

/*
=====================
idAnimState::Shutdown
=====================
*/
void idAnimState::Shutdown(void) {
	stateThread.Clear(true);
}

/*
=====================
idAnimState::PostState
=====================
*/
void idAnimState::PostState(const char* statename, int blendFrames, int delay, int flags) {
	if (SRESULT_OK != stateThread.PostState(statename, blendFrames, delay, flags)) {
		gameLocal.Error("Could not find state function '%s' for entity '%s'", statename, self->GetName());
	}
	disabled = false;
}

/*
=====================
idAnimState::SetState
=====================
*/
void idAnimState::SetState(const char* statename, int blendFrames, int flags) {
	if (SRESULT_OK != stateThread.SetState(statename, blendFrames, 0, flags)) {
		gameLocal.Error("Could not find state function '%s' for entity '%s'", statename, self->GetName());
	}

	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	disabled = false;
	idleAnim = false;
}

/*
=====================
idAnimState::StopAnim
=====================
*/
void idAnimState::StopAnim(int frames) {
	animBlendFrames = 0;
	animator->Clear(channel, gameLocal.time, FRAME2MS(frames));
}

/*
=====================
idAnimState::PlayAnim
=====================
*/
void idAnimState::PlayAnim(int anim) {
	if (anim) {
		animator->PlayAnim(channel, anim, gameLocal.time, FRAME2MS(animBlendFrames));
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::CycleAnim
=====================
*/
void idAnimState::CycleAnim(int anim) {
	if (anim) {
		animator->CycleAnim(channel, anim, gameLocal.time, FRAME2MS(animBlendFrames));
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::BecomeIdle
=====================
*/
void idAnimState::BecomeIdle(void) {
	idleAnim = true;
}

/*
=====================
idAnimState::Disabled
=====================
*/
bool idAnimState::Disabled(void) const {
	return disabled;
}

/*
=====================
idAnimState::AnimDone
=====================
*/
bool idAnimState::AnimDone(int blendFrames) const {
	int animDoneTime;

	animDoneTime = animator->CurrentAnim(channel)->GetEndTime();
	if (animDoneTime < 0) {
		// playing a cycle
		return false;
	}
	else if (animDoneTime - FRAME2MS(blendFrames) <= gameLocal.time) {
		return true;
	}
	else {
		return false;
	}
}

/*
=====================
idAnimState::IsIdle
=====================
*/
bool idAnimState::IsIdle(void) const {
	return disabled || idleAnim;
}

/*
=====================
idAnimState::GetAnimFlags
=====================
*/
animFlags_t idAnimState::GetAnimFlags(void) const {
	animFlags_t flags;

	memset(&flags, 0, sizeof(flags));
	if (!disabled && !AnimDone(0)) {
		flags = animator->GetAnimFlags(animator->CurrentAnim(channel)->AnimNum());
	}

	return flags;
}

/*
=====================
idAnimState::Enable
=====================
*/
void idAnimState::Enable(int blendFrames) {
	if (disabled) {
		disabled = false;
		animBlendFrames = blendFrames;
		lastAnimBlendFrames = blendFrames;
	}
}

/*
=====================
idAnimState::Disable
=====================
*/
void idAnimState::Disable(void) {
	disabled = true;
	idleAnim = false;
}

/*
=====================
idAnimState::UpdateState
=====================
*/
bool idAnimState::UpdateState(void) {
	if (disabled) {
		return false;
	}

	stateThread.Execute();

	return true;
}
