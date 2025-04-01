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

#ifndef _AGITATED_SEARCHING_STATE_LANTERN_BOT_H_
#define _AGITATED_SEARCHING_STATE_LANTERN_BOT_H_

#include "../AI.h"
#include "AgitatedSearchingState.h"

namespace ai
{

#define STATE_AGITATED_SEARCHING_LANTERN_BOT "AgitatedSearchingLanternBot"

class AgitatedSearchingStateLanternBot :
	public AgitatedSearchingState
{
protected:
	idVec3 _curAlertPos;

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	static StatePtr CreateInstance();

	// Override base class signal
	virtual void OnMovementBlocked(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner) override;

	void MoveTowardAlertPos(idAI* owner);
};

} // namespace

#endif
