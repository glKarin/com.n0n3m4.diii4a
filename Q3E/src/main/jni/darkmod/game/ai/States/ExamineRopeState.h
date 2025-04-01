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

#ifndef __AI_EXAMINE_ROPE_H__
#define __AI_EXAMINE_ROPE_H__

#include "State.h"

namespace ai
{

#define STATE_EXAMINE_ROPE "ExamineRope"

class ExamineRopeState :
	public State
{
private:
	// Default constructor
	ExamineRopeState();

	idEntityPtr<idAFEntity_Generic> _rope;
	idVec3 _point; // the interesting point on the rope

	// time to wait before proceeding with a state
	int _waitEndTime;

	idVec3 _examineSpot;	// where to stand to examine the rope

	enum EExamineRopeState
	{
		EStateSitting,
		EStateStarting,
		EStateApproaching,
		EStateTurningToward,
		EStateExamineTop,
		EStateExamineBottom,
		EStateFinal
	} _examineRopeState;

public:
	// Constructor using rope and examination point as input
	ExamineRopeState(idAFEntity_Generic* rope, idVec3 point);

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
	// Look at top of rope
	void StartExaminingTop(idAI* owner);

	// Look at bottom of rope
	void StartExaminingBottom(idAI* owner);

	void Wrapup(idAI* owner);
};

} // namespace ai

#endif /* __AI_EXAMINE_ROPE_H__ */
