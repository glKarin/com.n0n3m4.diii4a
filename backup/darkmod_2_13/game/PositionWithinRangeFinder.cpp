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



#include "PositionWithinRangeFinder.h"

PositionWithinRangeFinder::PositionWithinRangeFinder(const idAI* self, 
			const idMat3 &gravityAxis, const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistance) :
	_targetPos(targetPos),
	_eyeOffset(eyeOffset),
	_self(self),
	_gravityAxis(gravityAxis),
	_maxDistance(maxDistance),
	_haveBestGoal(false)
{
	// setup PVS

	

	idBounds bounds( targetPos - idVec3( 16, 16, 0 ), targetPos + idVec3( 16, 16, 64 ) );
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
	targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

PositionWithinRangeFinder::~PositionWithinRangeFinder() 
{
	gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

bool PositionWithinRangeFinder::TestArea( const idAAS *aas, int areaNum )
{
	idVec3 areaCenter = aas->AreaCenter( areaNum );
	areaCenter[ 2 ] += 1.0f;

	// calculate the world transform of the view position
	idVec3 dir = _targetPos - areaCenter;

	idVec3 local_dir;
	_gravityAxis.ProjectVector( dir, local_dir );
	local_dir.z = 0.0f;
	local_dir.ToVec2().Normalize();
	idMat3 axis = local_dir.ToMat3();

	idVec3 fromPos = areaCenter + _eyeOffset * axis;

	// gameRenderWorld->DebugText(va("%d", areaNum), areaCenter, 0.5f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 20000);

	float distance = (fromPos - _targetPos).LengthFast();
	if (distance > _maxDistance)
	{
		// Can't use this point, it's too far
		return false;
	}
	else
	{
		// Run trace
		trace_t results;
		if (!gameLocal.clip.TracePoint( results, fromPos, _targetPos, MASK_SOLID, _self ))
		{
			// Remember best result
			if (!_haveBestGoal || distance < bestGoalDistance)
			{
				_haveBestGoal = true;
				bestGoalDistance = distance;
				bestGoal.areaNum = areaNum;
				bestGoal.origin = areaCenter;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool PositionWithinRangeFinder::GetBestGoalResult(float& out_bestGoalDistance, aasGoal_t& out_bestGoal)
{
	if (_haveBestGoal)
	{
		out_bestGoalDistance = bestGoalDistance;
		out_bestGoal = bestGoal;
		return true;
	}
	else
	{
		return false;
	}
}





