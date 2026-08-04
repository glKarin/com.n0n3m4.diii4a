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

#ifndef __AI_PATH_CORNER_TASK_H__
#define __AI_PATH_CORNER_TASK_H__

#include "PathTask.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_CORNER "PathCorner"

// grayman #2414 - For path prediction
const int PATH_PREDICTION_MOVES = 2; // Number of moves to look ahead
const float PATH_PREDICTION_CONSTANT = 26.0; // Empirically determined for smooth
											 // turns at any m_maxInterleaveThinkFrames

class PathCornerTask;
typedef std::shared_ptr<PathCornerTask> PathCornerTaskPtr;

class PathCornerTask :
	public PathTask
{
private:
	bool _moveInitiated;
	bool _movePaused; // grayman #3647

	// Position last time this task was executed, used for path prediction
	idVec3 _lastPosition;

	// Frame this task was last executed
	int _lastFrameNum;

	// Whether to anticipate the AI reaching path corners
	bool _usePathPrediction;

	PathCornerTask();

public:
	PathCornerTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static PathCornerTaskPtr CreateInstance();

};

} // namespace ai

#endif /* __AI_PATH_CORNER_TASK_H__ */
