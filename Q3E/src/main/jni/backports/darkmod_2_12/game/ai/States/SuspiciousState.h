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

#ifndef __AI_SUSPICIOUS_STATE_H__
#define __AI_SUSPICIOUS_STATE_H__

#include "State.h"

namespace ai
{

#define STATE_SUSPICIOUS "Suspicious"

class SuspiciousState :
	public State
{

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	static StatePtr CreateInstance();

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner) override;
};

} // namespace ai

#endif /* __AI_SUSPICIOUS_STATE_H__ */
