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
#ifndef POSITION_WITHIN_RANGE_FINDER__H
#define POSITION_WITHIN_RANGE_FINDER__H

#include "precompiled.h"

class PositionWithinRangeFinder : 
	public idAASCallback
{
private:
	idVec3 _targetPos;
	idVec3 _eyeOffset;
	const idAI* _self;
	idMat3 _gravityAxis;
	float _maxDistance;
	bool _haveBestGoal;

	
	idBounds			excludeBounds;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
	int	numPVSAreas;

	aasGoal_t		bestGoal; 
	float bestGoalDistance;

	
public:
	PositionWithinRangeFinder(const idAI *self, const idMat3 &gravityAxis, 
			const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistance);

	virtual ~PositionWithinRangeFinder() override;

	virtual bool TestArea( const idAAS *aas, int areaNum ) override;

	bool GetBestGoalResult(float& out_bestGoalDistance, aasGoal_t& out_bestGoal);
};

#endif /* POSITION_WITHIN_RANGE_FINDER__H */
