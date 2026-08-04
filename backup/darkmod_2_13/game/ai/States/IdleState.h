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

#ifndef __AI_IDLE_STATE_H__
#define __AI_IDLE_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_IDLE "Idle"

class IdleState :
	public State
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	static StatePtr CreateInstance();

	/**
	* ishtvan: Called when targets are changed
	* Re-initializes to catch new path corners
	**/
	virtual void OnChangeTarget(idAI *owner) override;

	// grayman #3154 - forget you were sitting or sleeping at map start
	virtual void ForgetSittingSleeping() override { _startSitting = _startSleeping = false; };

protected:

	bool _startSitting;
	bool _startSleeping;

	virtual bool CheckAlertLevel(idAI* owner) override;

	// Returns the initial idle bark sound, depending on the alert level 
	// and the current state of mind
	virtual idStr GetInitialIdleBark(idAI* owner);

	virtual void InitialiseMovement(idAI* owner);
	virtual void InitialiseCommunication(idAI* owner);
};

} // namespace ai

#endif /* __AI_IDLE_STATE_H__ */
