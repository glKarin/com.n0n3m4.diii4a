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

#include "precompiled.h"
#pragma hdrstop



#include "tdmAASFindEscape.h"

tdmAASFindEscape::tdmAASFindEscape(
	const idVec3& threatPosition, 
	const idVec3& selfPosition, 
	float minDistToThreat,
	float minDistToSelf,
	int	  team // grayman #3548
) :
	_threatPosition(threatPosition),
	_selfPosition(selfPosition),
	_minDistThreatSqr(minDistToThreat*minDistToThreat),
	_minDistSelfSqr(minDistToSelf*minDistToSelf),
	_team(team), // grayman #3548
	_bestDistSqr(0)
{
	_goal.areaNum = -1;
}

bool tdmAASFindEscape::TestArea(const idAAS *aas, int areaNum)
{
	const idVec3 &areaCenter = aas->AreaCenter( areaNum );

	idStr numberStr(areaNum);
	idMat3 viewAngles;
	viewAngles.Identity();
	//gameRenderWorld->DebugText(numberStr.c_str(), aas->AreaCenter(areaNum), 1, colorRed, viewAngles, 1, 5000);

	float distThreatSqr(( _threatPosition.ToVec2() - areaCenter.ToVec2() ).LengthSqr());
	float distSelfSqr(( _selfPosition.ToVec2() - areaCenter.ToVec2() ).LengthSqr());

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING(
		"Testing area: %d, distThreatSqr = %f, distSelfSqr = %f, _minDistThreatSqr = %f\r", 
		areaNum, distThreatSqr, distSelfSqr, _minDistThreatSqr
	);

	// Minimum distances must be obeyed and the distance 
	// to the threat must be greater than the one of the best candidate so far
	if (distThreatSqr > _bestDistSqr && 
		distThreatSqr >= _minDistThreatSqr && 
		distSelfSqr >= _minDistSelfSqr)
	{
		// grayman #3548 - check area for hostiles
		if (!gameLocal.m_RelationsManager->CheckForHostileAI(areaCenter, _team))
		{
			// We've got a new best candidate, which is farther away than the previous candidate
			_bestDistSqr = distThreatSqr;

			// Fill the goal with the new values
			_goal.areaNum = areaNum;
			_goal.origin = areaCenter;

			// There's a 10% chance that the search is truncated here.
			if (gameLocal.random.RandomFloat() < 0.1f)
			{
				return true;
			}
		}
	}

	// Always return false, we're collecting all areas
	return false;
}
