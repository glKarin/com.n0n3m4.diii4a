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

#ifndef __tdmAASFindEscape_H__
#define __tdmAASFindEscape_H__

#include "AAS.h"

/**
 * greebo: This evaluator is designed to find escape routes
 *         for fleeing actors. The TestArea method evaluates area
 *         candidates based on their distance to the threatening entity
 *         and the current AI location.
 */
class tdmAASFindEscape : public idAASCallback {
public:
	// Constructor
	tdmAASFindEscape(
		const idVec3& threatPosition, 
		const idVec3& selfPosition, 
		float minDistToThreat,
		float minDistToSelf,
		int   team // grayman #3548
	);

	virtual bool		TestArea(const idAAS *aas, int areaNum) override;

	// Returns the best escape goal found
	inline aasGoal_t&	GetEscapeGoal() { return _goal; };

private:
	idVec3				_threatPosition;
	idVec3				_selfPosition;
	float				_minDistThreatSqr;
	float				_minDistSelfSqr;
	int					_team; // grayman #3548

	aasGoal_t			_goal;
	float				_bestDistSqr;
	int					_bestAreaNum;
};

#endif /* __tdmAASFindEscape_H__ */
