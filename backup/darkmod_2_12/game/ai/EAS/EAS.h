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

#ifndef __AI_EAS_H__
#define __AI_EAS_H__

#include "precompiled.h"
#include "../../MultiStateMover.h"
#include "../../../game/ai/AAS_local.h"
#include <set>
#include <vector>
#include <list>

#include "RouteNode.h"
#include "RouteInfo.h"
#include "ElevatorStationInfo.h"
#include "ClusterInfo.h"

class idAASLocal;

namespace eas {

/**
 * greebo: The EAS ("Elevator Awareness System") provides some extended
 * routing functionality between AAS clusters for all AI which are able
 * to handle elevators (i.e. travelFlag TFL_ELEVATOR set to 1).
 *
 * This class is tightly bound to its owning idAASLocal class and is always
 * constructed and destructed along with it.
 */
class tdmEAS
{
private:
	// The owning AAS
	idAASLocal* _aas;

	// The list of elevators
	idList< idEntityPtr<CMultiStateMover> > _elevators;

	// The array of ClusterInfoStructures
	typedef std::vector<ClusterInfoPtr> ClusterInfoVector;
	ClusterInfoVector _clusterInfo;

	// An array of dynamically allocated elevator stations
	typedef std::vector<ElevatorStationInfoPtr> ElevatorStationVector;
	ElevatorStationVector _elevatorStations;

	// Temporary calculation variables, don't need to be saved
	mutable int _routingIterations;

public:
	// Initialise the EAS with a valid AAS reference
	tdmEAS(idAASLocal* aas);

	/**
	 * greebo: Computes the routing tables for the elevators.
	 * All elevators must have been added beforehand using AddElevator().
	 */
	void Compile();

	// Clears all data
	void Clear();

	/**
	 * greebo: Adds a new elevator to the EAS.
	 */
	void AddElevator(CMultiStateMover* mover);

	// This is the analogous method to idAAS::RouteToGoal. The path variable will contain the right pathing information if a goal was found.
	// returns TRUE if a route was found, FALSE otherwise.
	bool FindRouteToGoal(aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idActor* actor, int &elevatorTravelTime); // grayman #3029

	/** 
	 * greebo: Returns the elevator station with the given index.
	 */
	ElevatorStationInfoPtr GetElevatorStationInfo(int index);

	// Gets the index of the given elevator station
	int GetElevatorStationIndex(ElevatorStationInfoPtr info);
	int GetElevatorStationIndex(CMultiStateMoverPosition* positionEnt);

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	void DrawRoute(int startArea, int goalArea);

	// grayman #3548 - find closest elevator
	// grayman #4229 - abandoned
//	CMultiStateMover* GetNearbyElevator(idVec3 pos, float maxDist, float maxVertDist);
	
	// grayman - for debugging cluster data
	//void PrintClusterInfo();

private:
	void SetupClusterInfoStructures();
	void AssignElevatorsToClusters();

	//void PrintRoutes(); // grayman - debug routes

	void SetupClusterRouting();
	void SetupReachableElevatorStations();
	void SetupRoutesBetweenClusters();

	// Removes all empty and dummy routes
	void CondenseRouteInfo();

	// Sorts the existing routes (fastest first)
	void SortRoutes();
	void SortRoute(int startCluster, int goalCluster);
	void SumRouteTravelTimes(); // grayman #3029 - stamps routes with overall travel time

	// Retrieves the internal index of the given mover (or -1 if the mover is not registered)
	int GetElevatorIndex(CMultiStateMover* mover);

	// Calculates all possible routes from the startCluster/startArea to goalCluster/goalArea 
	RouteInfoList FindRoutesToCluster(int startCluster, int startArea, int goalCluster, int goalArea);

	// Returns the AAS area number for the given position
	int GetAreaNumForPosition(const idVec3& position);

	// Inserts the routeInfo for startCluster => goalCluster, but checks for duplicates, returns TRUE if inserted
	bool InsertUniqueRouteInfo(int startCluster, int goalCluster, RouteInfoPtr route);

	// Removes empty routeInfo structures
	void CleanRouteInfo(int startCluster, int goalCluster);

	// Checks the route for redundancies, returns TRUE if the route is accepted
	bool EvaluateRoute(int startCluster, int goalCluster, int forbiddenElevator, RouteInfoPtr route);
};

} // namespace eas

#endif /* __AI_EAS_H__ */
