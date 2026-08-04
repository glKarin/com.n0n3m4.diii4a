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

#ifndef __AI_ALERT_IDLE_STATE_H__
#define __AI_ALERT_IDLE_STATE_H__

#include "State.h"
#include "IdleState.h"

namespace ai
{

#define STATE_ALERT_IDLE "AlertIdle"

/**
 * angua: This is a specialisation of the IdleState. If the AI
 * has been highly alerted during its lifetime, it doesn't return
 * into the regular IdleState, but this one.
 */
class AlertIdleState :
	public IdleState
{

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	// Note: we do not call IdleState::Init
	virtual void Init(idAI* owner) override;

	// Think is inherited from IdleState::Think

	static StatePtr CreateInstance();

	virtual void ForgetSittingSleeping() override { _startSitting = _startSleeping = false; };   // grayman #3154

protected:
	// Returns the initial idle bark sound, depending on the alert level 
	// and the current state of mind
	virtual idStr GetInitialIdleBark(idAI* owner) override;
};

} // namespace ai

#endif /* __AI_ALERT_IDLE_STATE_H__ */
