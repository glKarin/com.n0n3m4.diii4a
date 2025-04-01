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



#include "EscapePointEvaluator.h"
#include "EscapePointManager.h"
#include "ai/AAS_local.h"

EscapePointEvaluator::EscapePointEvaluator(const EscapeConditions& conditions) :
	_conditions(&conditions),
	_bestId(-1), // Set the ID to invalid
	_startAreaNum(conditions.self.GetEntity()->PointReachableAreaNum(conditions.fromPosition, 2.0f)),
	_bestTime(0),
	_distanceMultiplier((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_threatPosition(conditions.fromEntity.GetEntity() != NULL ? conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin() : conditions.threatPosition) // grayman #3317
{}

bool EscapePointEvaluator::PerformDistanceCheck(EscapePoint& escapePoint)
{
	if (_conditions->distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a valid point, we don't care about distance, end the search
	}

	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// grayman #3548 - shouldn't we also allow travel by elevator?
	idAI* ai = _conditions->self.GetEntity();
	if (ai && ai->CanUseElevators())
	{
		travelFlags |= TFL_ELEVATOR;
	}

	// Calculate the traveltime
	//idReachability* reach;

	// grayman #3100 - factor in whether the point is reachable, don't just look at distance
	// grayman #3548 - path check must include elevators. Use WalkPathToGoal() for that, because
	// RouteToGoalArea() only checks walking.

	aasPath_t path;
	bool canReachPoint = _conditions->aas->WalkPathToGoal( path, _startAreaNum, _conditions->fromPosition, escapePoint.areaNum, escapePoint.origin, travelFlags, travelTime, ai );
	//bool canReachPoint = _conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach, NULL, ai);

	if ( !canReachPoint )
	{
		return true; // can't get to it, so keep looking
	}
	
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	// Take this if no point has been found yet or if this one is better
	if (_bestId == -1 || travelTime*_distanceMultiplier < _bestTime*_distanceMultiplier)
	{
		// Either the minDistanceToThreat is negative, or the distance has to be larger
		// for the escape point to be considered as better
		if (_conditions->minDistanceToThreat < 0 || 
			(_threatPosition - escapePoint.origin).LengthFast() >= _conditions->minDistanceToThreat)
		{
			// This is a better flee point
			_bestId = escapePoint.id;
			_bestTime = travelTime;
		}
	}

	return true;
}

bool EscapePointEvaluator::PerformRelationshipCheck(EscapePoint& escapePoint, int team) // grayman #3548
{
	return gameLocal.m_RelationsManager->CheckForHostileAI(escapePoint.origin, team);
}

// grayman #3847 - Check the distance from the goal (escapePoint)
// to the threat. We don't want to send the AI to a goal that's
// near the threat. If the goal is far from the threat, return 'false'. If the
// goal is too near the threat, return 'true'. ('true' means the escape point
// isn't good, and we have to keep looking. 'false' means the escape point is good.)

bool EscapePointEvaluator::PerformProximityToThreatCheck(EscapePoint& escapePoint, idVec3 threatLoc)
{
	return ((escapePoint.origin - threatLoc).LengthFast() < 300);
}


/**
 * AnyEscapePointFinder
 */
AnyEscapePointFinder::AnyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team),
	_threatLocation(conditions.threatPosition) // grayman #3847
{}

bool AnyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	// grayman #3847 - add check for whether the point being fled from is too close
	if ( PerformProximityToThreatCheck(escapePoint,_threatLocation)) // grayman #3847
	{
		return true; // too close to threat, so keep looking
	}

	// grayman #3548 - add relationship check for hostiles at the escape point
	if ( PerformRelationshipCheck(escapePoint,_team) )
	{
		return true; // hostiles present, so keep looking
	}

	return PerformDistanceCheck(escapePoint);
}

/**
 * GuardedEscapePointFinder
 */
GuardedEscapePointFinder::GuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team),
	_threatLocation(conditions.threatPosition) // grayman #3847
{}

bool GuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded)
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Escape point %d is not guarded.\r", escapePoint.id);
		return true;
	}

	// grayman #3847 - add check for whether the point being fled from is too close
	if ( PerformProximityToThreatCheck(escapePoint,_threatLocation)) // grayman #3847
	{
		return true; // too close to threat, so keep looking
	}

	// grayman #3548 - add relationship check for hostiles at the escape point
	if ( PerformRelationshipCheck(escapePoint,_team) )
	{
		return true; // hostiles present, so keep looking
	}

	return PerformDistanceCheck(escapePoint);
}

/**
 * FriendlyEscapePointFinder
 */
FriendlyEscapePointFinder::FriendlyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team),
	_threatLocation(conditions.threatPosition) // grayman #3847
{}

bool FriendlyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not friendly, continue the search
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Escape point %d is not friendly.\r", escapePoint.id);
		return true;
	}

	// grayman #3847 - add check for whether the point being fled from is too close
	if ( PerformProximityToThreatCheck(escapePoint,_threatLocation)) // grayman #3847
	{
		return true; // too close to threat, so keep looking
	}

	// grayman #3548 - add relationship check for hostiles at the escape point
	if ( PerformRelationshipCheck(escapePoint,_team) )
	{
		return true; // hostiles present, so keep looking
	}

	return PerformDistanceCheck(escapePoint);
}

/**
 * FriendlyGuardedEscapePointFinder
 */
FriendlyGuardedEscapePointFinder::FriendlyGuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team),
	_threatLocation(conditions.threatPosition) // grayman #3847
{}

bool FriendlyGuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded || !gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not guarded or not friendly, continue the search
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Escape point %d is either not friendly or not guarded.\r", escapePoint.id);
		return true;
	}

	// grayman #3847 - add check for whether the point being fled from is too close
	if ( PerformProximityToThreatCheck(escapePoint,_threatLocation)) // grayman #3847
	{
		return true; // too close to threat, so keep looking
	}

	// grayman #3548 - add relationship check for hostiles at the escape point
	if ( PerformRelationshipCheck(escapePoint,_team) )
	{
		return true; // hostiles present, so keep looking
	}

	return PerformDistanceCheck(escapePoint);
}
