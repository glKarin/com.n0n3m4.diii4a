/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __AI_FLEE_DONE_H__
#define __AI_FLEE_DONE_H__

#include "State.h"

namespace ai
{

#define STATE_FLEE_DONE "FleeDone"

class FleeDoneState :
	public State
{
private:
	float _oldTurnRate;
	int _turnEndTime;
	bool _searchForFriendDone;

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	void WrapUp(idAI* owner); // grayman #3848

	virtual void OnActorEncounter(idEntity* stimSource, idAI* owner) override;

	// grayman #3848 - reactions when fleeing
	virtual void OnVisualStimWeapon(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimSuspicious(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimRope( idEntity* stimSource, idAI* owner, idVec3 ropeStimSource ) override;
	virtual void OnVisualStimBlood(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimLightSource(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimMissingItem(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimBrokenItem(idEntity* stimSource, idAI* owner) override;
	virtual void OnVisualStimDoor(idEntity* stimSource, idAI* owner) override;
	virtual void OnHitByMoveable(idAI* owner, idEntity* tactEnt) override;

	virtual bool CheckAlertLevel(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_FLEE_DONE_H__ */
