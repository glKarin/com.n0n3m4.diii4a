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

#ifndef TDM_OBJECTIVE_CONDITION_H
#define TDM_OBJECTIVE_CONDITION_H

#include "precompiled.h"

#include "Objective.h"

class CMissionData;

class ObjectiveCondition
{
private:
	// Possible effect types 
	enum Type
	{
		CHANGE_STATE,		// changes state of target objective
		CHANGE_VISIBILITY,	// changes visibility of target objective
		CHANGE_MANDATORY,	// changes mandatory flag of target objetive
		INVALID_TYPE,		// not a valid type
	};

	Type _type;

	int _value;
	int _srcMission;
	EObjCompletionState _srcState;
	int _srcObj;	
	int _targetObj;

public:
	// Default constructor
	ObjectiveCondition();

	// Construct from a given dictionary
	ObjectiveCondition(const idDict& dict, int index);

	// Returns TRUE if the condition has enough valid parameters to be functional
	bool IsValid() const;

	// Applies this conditional action to the given objectives
	// Returns TRUE if the condition was applicable, FALSE otherwise
	bool Apply(CMissionData& missionData);

private:
	void ParseFromSpawnargs(const idDict& dict, int index);
};

#endif 
