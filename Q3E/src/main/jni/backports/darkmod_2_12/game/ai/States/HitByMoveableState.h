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

#ifndef __AI_HIT_BY_MOVEABLE_H__
#define __AI_HIT_BY_MOVEABLE_H__

#include "State.h"

namespace ai
{

#define STATE_HIT_BY_MOVEABLE "HitByMoveable"

// grayman #2816 - constants for looking at objects that strike the AI
//const float HIT_DELAY	  = 1.0f; // seconds - when getting hit by something, wait this long before turning toward it
//const float HIT_DURATION  = 2.0f; // seconds - and look at it for this long (+/- a random variation)
//const float HIT_VARIATION = (HIT_DURATION/5); // seconds - max variation
const int HIT_DIST		 =  150; // pick a point this far away, back where the object came from and look at it
const int HIT_FIND_THROWER_HORZ = 300; // how far out to look for a friendly/neutral AI
const int HIT_FIND_THROWER_VERT = 150; // how far up/down to look for a friendly/neutral AI

class HitByMoveableState :
	public State
{
private:
	idVec3 _pos;				// a position to look back at
	idEntityPtr<idActor> _responsibleActor;	// who threw the object

	int _waitEndTime;			// time to wait before proceeding with a state
	float _lookAtDuration;		// how long to look at what hit you
	float _lookBackDuration;	// how long to look back at where the object came from

	enum EHitByMoveableState
	{
		EStateSittingSleeping,
		EStateStarting,
		EStateTurnToward,
		EStateLookAt,
		EStateTurnBack,
		EStateLookBack,
		EStateFinal
	} _hitByMoveableState;

public:
	// Default constructor
	HitByMoveableState();

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

#endif /* __AI_HIT_BY_MOVEABLE_H__ */
