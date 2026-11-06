/*
===============================================================================

AAS_Find.cpp

This file has all aas search classes.

===============================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AAS_Find.h"

/*
===============================================================================

	rvAASFindGoalForHide

===============================================================================
*/

/*
============
rvAASFindGoalForHide::rvAASFindGoalForHide
============
*/
rvAASFindGoalForHide::rvAASFindGoalForHide( const idVec3 &hideFromPos ) {
	int			numPVSAreas;
	idBounds	bounds( hideFromPos - idVec3( 16, 16, 0 ), hideFromPos + idVec3( 16, 16, 64 ) );

	// setup PVS
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
	hidePVS		= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
rvAASFindGoalForHide::~rvAASFindGoalForHide
============
*/
rvAASFindGoalForHide::~rvAASFindGoalForHide() {
	gameLocal.pvs.FreeCurrentPVS( hidePVS );
}

/*
rvAASFindGoalForHide
rvAASFindHide::TestArea
============
*/
bool rvAASFindGoalForHide::TestArea( class idAAS *aas, int areaNum, const aasArea_t& area ) {
	int	numPVSAreas;
	int	PVSAreas[ idEntity::MAX_PVS_AREAS ];

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( area.center ).Expand( 16.0f ).TranslateSelf(idVec3(0,0,1)), PVSAreas, idEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( hidePVS, PVSAreas, numPVSAreas ) ) {
		return true;
	}

	return false;
}

/*
===============================================================================

	rvAASFindGoalOutOfRange

===============================================================================
*/

/*
============
rvAASFindGoalOutOfRange::rvAASFindGoalOutOfRange
============
*/
rvAASFindGoalOutOfRange::rvAASFindGoalOutOfRange( idAI* _owner ) {
	owner = _owner;
}

/*
============
rvAASFindAreaOutOfRange::TestArea
============
*/
bool rvAASFindGoalOutOfRange::TestPoint ( idAAS* aas, const idVec3& pos, const float zAllow ) {
	return aiManager.ValidateDestination ( owner, pos );
}

/*
===============================================================================

rvAASFindGoalForAttack

Find a position to move to that allows the ai to at their target

===============================================================================
*/

/*
============
rvAASFindGoalForAttack::rvAASFindGoalForAttack
============
*/
rvAASFindGoalForAttack::rvAASFindGoalForAttack( idAI* _owner ) {
	owner		= _owner;
	cachedIndex = 0;
}

/*
============
rvAASFindGoalForAttack::~rvAASFindGoalForAttack
============
*/
rvAASFindGoalForAttack::~rvAASFindGoalForAttack ( void ) {
}

/*
============
rvAASFindGoalForAttack::Init
============
*/
void rvAASFindGoalForAttack::Init ( void ) {
	// setup PVS
	int	numPVSAreas;
	numPVSAreas = gameLocal.pvs.GetPVSAreas( owner->enemy.ent->GetPhysics()->GetAbsBounds(), PVSAreas, idEntity::MAX_PVS_AREAS );
	targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
	
	cachedGoals.SetGranularity ( 1024 );
}

/*
============
rvAASFindGoalForAttack::Finish
============
*/
void rvAASFindGoalForAttack::Finish ( void ) {
	gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
rvAASFindGoalForAttack::TestArea
============
*/
bool rvAASFindGoalForAttack::TestArea( class idAAS *aas, int areaNum, const aasArea_t& area ) {
	int	numPVSAreas;
	int	PVSAreas[ idEntity::MAX_PVS_AREAS ];

	cachedAreaNum = areaNum;

	// If the whole area is out of range then skip it
	float range;
	range = area.bounds.ShortestDistance ( owner->enemy.lastKnownPosition );
	if ( range > owner->combat.attackRange[1] ) {
		return false;
	}

	// Out of pvs?
	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( area.center ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
	return gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas );
}

/*
============
rvAASFindGoalForAttack::TestPoint
============
*/
bool rvAASFindGoalForAttack::TestPoint ( class idAAS *aas, const idVec3& point, const float zAllow ) {
	float dist;
	
	idVec3 localPoint = point;
	float bestZ = owner->enemy.ent->GetPhysics()->GetOrigin().z;
	if ( bestZ > localPoint.z ) {
		if ( bestZ > zAllow ) {
			localPoint.z = zAllow;
		} else {
			localPoint.z = bestZ;
		}
	}

	// Out of attack range?
	dist = (localPoint - owner->enemy.ent->GetPhysics()->GetOrigin ( )).LengthFast ( );
	if ( dist < owner->combat.attackRange[0] || dist > owner->combat.attackRange[1] ) {
		return false;
	}

	// If tethered make sure the point is within the tether range
	if ( owner->tether ) {
		idVec3 localPoint = point;
		float bestZ = owner->tether.GetEntity()->GetPhysics()->GetOrigin().z;
		if ( bestZ > localPoint.z ) {
			if ( bestZ > zAllow ) {
				localPoint.z = zAllow;
			} else {
				localPoint.z = bestZ;
			}
		}
		if ( !owner->tether->ValidateDestination ( owner, localPoint ) ) {
			return false;
		}
	}
	
	aasGoal_t& goal = cachedGoals.Alloc ( );
	goal.areaNum = cachedAreaNum;
	goal.origin  = localPoint;
	
	return false;
}

/*
============
rvAASFindGoalForAttack::TestCachedGoal
============
*/
bool rvAASFindGoalForAttack::TestCachedGoal ( int index ) {
	const aasGoal_t& goal = cachedGoals[index];
	
	// Out of attack range?
	float dist = (goal.origin - owner->enemy.ent->GetPhysics()->GetOrigin ( ) ).LengthFast ( );
	if ( dist < owner->combat.attackRange[0] || dist > owner->combat.attackRange[1] ) {
		return false;
	}	

	// Someone already there?
	if ( !aiManager.ValidateDestination ( owner, goal.origin, true ) ) {
		return false;
	}

	// Can we see the enemy from this position?
	if ( !owner->CanSeeFrom ( goal.origin - owner->GetPhysics()->GetGravityNormal ( ) * owner->combat.visStandHeight, owner->GetEnemy(), false ) ) {
		return false;
	}

	return true;
}

/*
============
rvAASFindGoalForAttack::TestCachedPoints
============
*/
bool rvAASFindGoalForAttack::TestCachedGoals ( int count, aasGoal_t& goal ) {
	int i;
	
	goal.areaNum = 0;
	
	// Test as many points as we are allowed to test
	for ( i = 0; i < count && cachedIndex < cachedGoals.Num(); cachedIndex ++, i ++ ) {
		// Retest simple checks
		if ( TestCachedGoal( cachedIndex ) ) {
			goal = cachedGoals[cachedIndex];
			return true;
		}
	}
	
	return !(cachedIndex >= cachedGoals.Num());
}

/*
===============================================================================

rvAASFindGoalForTether
	
Find a goal to move to that is within the given tether.

===============================================================================
*/

/*
============
rvAASFindGoalForTether::rvAASFindGoalForTether
============
*/
rvAASFindGoalForTether::rvAASFindGoalForTether( idAI* _owner, rvAITether* _tether ) {
	owner  = _owner;
	tether = _tether;
}

/*
============
rvAASFindGoalForTether::rvAASFindGoalForTether
============
*/
rvAASFindGoalForTether::~rvAASFindGoalForTether( void ) {
}

/*
============
rvAASFindGoalForTether::TestArea
============
*/
bool rvAASFindGoalForTether::TestArea( class idAAS *aas, int areaNum, const aasArea_t& area ) {
	// Test super class first
	if ( !idAASCallback::TestArea ( aas, areaNum, area ) ) {
		return false;
	}

	// Make sure the area bounds is remotely valid for the tether
	idBounds tempBounds = area.bounds;
	tempBounds[1].z = area.ceiling;
	if ( !tether->ValidateBounds ( tempBounds ) ) {
		return false;
	}
	return true;
}

/*
============
rvAASFindGoalForTether::TestPoint
============
*/
bool rvAASFindGoalForTether::TestPoint ( class idAAS* aas, const idVec3& point, const float zAllow ) {
	if ( !tether ) {
		return false;
	}	
	
	idVec3 localPoint = point;
	float bestZ = tether->GetPhysics()->GetOrigin().z;
	if ( bestZ > localPoint.z )	{
		if ( bestZ > zAllow ) {
			localPoint.z = zAllow;
		} else {
			localPoint.z = bestZ;
		}
	}
	if ( !tether->ValidateDestination ( owner, localPoint ) ) {
		return false;
	}
	
	if ( !aiManager.ValidateDestination ( owner, localPoint ) ) {
		return false;
	}
	
	return true;
}
