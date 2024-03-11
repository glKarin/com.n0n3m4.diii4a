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

#ifndef __AI_SEARCHING_STATE_H__
#define __AI_SEARCHING_STATE_H__

#include "State.h"

/**
* greebo: A SearchingState is handling the AI's search routines.
* 
* The routine needs "memory.alertPos" to be set as prerequisite.
*
* The boolean variable "memory.stimulusLocationItselfShouldBeSearched" can be used
* to let the AI the "memory.alertPos" position as first hiding spot.
* If the boolean is not set to TRUE, the hiding spot search is based around memory.alertPos.
*
* The actual hiding spot search algorithm is called over multiple
* frames. Once finished, the AI can use its results (unless the 
* stimulusLocationItselfShouldBeSearched bool is set to TRUE, then alertPos is used as
* first hiding spot right away.
*
* For each hiding spot, an InvestigateSpotTask is invoked which takes care of the details.
*/

namespace ai
{

#define STATE_SEARCHING "Searching"
#define DELAY_RANDOM_SPOT_GEN 3000 // grayman #2422 - don't generate random search spots more often than this (in ms)
#define LOOK_AT_AUDIO_SPOT_DURATION 2000 // grayman #3424 - how long to look at an audio alert if you do

class SearchingState :
	public State
{
public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	bool FindRadialSpot(idAI* owner, idVec3 origin, float radius, idVec3 &spot); // grayman #3857

	// Incoming events issued by the Subsystems
	virtual void OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem) override;

	// greebo: Gets called when the AI is alerted by a suspicious sound (override)
	virtual bool OnAudioAlert(idStr soundName, bool addFuzziness, idEntity* maker) override; // grayman #3847 // grayman #3857

	static StatePtr CreateInstance();

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner) override;

	/**
	 * This method is used to start a new hiding spot search. Any existing search in progress is replaced.
	 */
	virtual bool StartNewHidingSpotSearch(idAI* owner);
};

} // namespace ai

#endif /* __AI_SEARCHING_STATE_H__ */
