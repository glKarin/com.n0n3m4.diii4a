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

#ifndef __AI_BLINDED_STATE_H__
#define __AI_BLINDED_STATE_H__

#include "../AI.h"
#include "State.h"

namespace ai
{

#define STATE_BLINDED "Blinded"

class BlindedState :
	public State
{
private:
	int   _endTime;
	float _oldVisAcuity; // to restore visual acuity
	float _oldAudAcuity; // Smoke #2829 - to restore audio acuity
	bool  _staring;		 // grayman #3431 (true = staring at ground)
	bool  _initialized;  // grayman #4270

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
};

} // namespace ai

#endif /* __AI_BLINDED_STATE_H__ */
