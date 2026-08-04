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

#ifndef __AI_AGITATED_SEARCHING_STATE_H__
#define __AI_AGITATED_SEARCHING_STATE_H__

#include "State.h"
#include "SearchingState.h"

/**
* greebo: AgitatedSearchingState is one alert index above SearchingState.
*
* Apart from a few minor things this is similar to the base class SearchingState.
* 
* See the base class for documentation.
*/

namespace ai
{

#define STATE_AGITATED_SEARCHING "AgitatedSearching"

class AgitatedSearchingState :
	public SearchingState
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
	virtual bool CheckAlertLevel(idAI* owner) override;

	virtual void CalculateAlertDecreaseRate(idAI* owner);

	void DrawWeapon(idAI* owner); // grayman #3507

	void SetRepeatedBark(idAI* owner); // grayman #3857
};

} // namespace ai

#endif /* __AI_AGITATED_SEARCHING_STATE_H__ */
