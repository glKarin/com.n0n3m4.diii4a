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

#ifndef __AI_POCKET_PICKED_H__
#define __AI_POCKET_PICKED_H__

#include "State.h"

namespace ai
{

#define STATE_POCKET_PICKED "PocketPicked"

#define ALERT_WINDOW 4000  // how far back to look for the previous alert
#define LOOK_TIME_MIN 3000 // minimum time to look behind
#define LOOK_TIME_MAX 5000 // maximum time to look behind

class PocketPickedState :
	public State
{
private:
	int _waitEndTime;  // time to wait before proceeding with a state

	enum EPocketPickedState
	{
		EStateReact,
		EStateStopping,
		EStateStartAnim,
		EStatePlayAnim,
		EStateTurnToward,
		EStateLookAt
	} _pocketPickedState;

public:
	// Default constructor
	PocketPickedState();

	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	// This is called when a State is destroyed
	virtual void Cleanup(idAI* owner) override; // grayman #3559

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	static StatePtr CreateInstance();

private:
	void Wrapup(idAI* owner);
};

} // namespace ai

#endif /* __AI_POCKET_PICKED_H__ */
