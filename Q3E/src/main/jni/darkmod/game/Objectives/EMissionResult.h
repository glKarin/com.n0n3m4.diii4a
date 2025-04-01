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

#ifndef EMISSIONRESULT_H_
#define EMISSIONRESULT_H_

enum EMissionResult {
	MISSION_NOTEVENSTARTED = 0,	// before any map is loaded (at game startup, for instance)
	MISSION_INPROGRESS = 1,		// mission not yet accomplished (in-game)
	MISSION_FAILED = 2,			// mission failed
	MISSION_COMPLETE = 3,		// mission completed
};

#endif /*EMISSIONRESULT_H_*/
