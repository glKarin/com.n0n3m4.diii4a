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



#include "EAS.h"

#define ELEVATOR_TRAVEL_FACTOR 21	// Should be 15, which worked fine for all test maps. But subsequent
									// testing with the Outpost FM required bumping it to 21 so as not to wreck
									// how the AI patrolled there.

namespace eas {

tdmEAS::tdmEAS(idAASLocal* aas) :
	_aas(aas),
	_routingIterations(0)
{}

void tdmEAS::Clear()
{
	_elevators.ClearFree();
	_clusterInfo.clear();
	_elevatorStations.clear();
}

void tdmEAS::AddElevator(CMultiStateMover* mover)
{
	_elevators.Alloc() = mover;
}

// grayman - debug cluster data

#if 0
void tdmEAS::PrintClusterInfo()
{
	// print all route counts from cluster to cluster

	int numClusters = _aas->file->GetNumClusters();
	for ( int j = 1 ; j < numClusters ; j++ )
	{
		for ( int k = 1 ; k < numClusters ; k++)
		{
			if ( k != j )
			{
				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("..... there are %d routes from %d to %d\r",_clusterInfo[j]->routeToCluster[k].size(),j,k);
			}
		}
	}
	
	// print which cluster areas live in, and show area bounds

	int numAreas = _aas->file->GetNumAreas();
	for ( int j = 0 ; j < numAreas ; j++ )
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("..... area %d (%s) is in cluster %d\r",j,_aas->file->GetArea(j).bounds.ToString(),_aas->file->GetArea(j).cluster);
	}
}
#endif

void tdmEAS::Compile()
{
	if (_aas == NULL)
	{
		gameLocal.Error("Cannot Compile EAS, no AAS available.");
	}

	// First, allocate the memory for the cluster info structures
	SetupClusterInfoStructures();

	// Then, traverse the registered elevators and assign their "stations" or floors to the clusters
	AssignElevatorsToClusters();

	// Now setup the connection information between clusters
	SetupClusterRouting();

//	PrintClusterInfo(); // grayman - for debugging cluster data
}

void tdmEAS::SetupClusterInfoStructures() 
{
	// Clear the vector and allocate a new one (one structure ptr for each cluster)
	_clusterInfo.clear();
	_clusterInfo.resize(_aas->file->GetNumClusters());

	for (std::size_t i = 0; i < _clusterInfo.size(); i++)
	{
		//const aasCluster_t& cluster = _aas->file->GetCluster(static_cast<int>(i));
		
		_clusterInfo[i] = ClusterInfoPtr(new ClusterInfo);
        _clusterInfo[i]->clusterNum = static_cast<int>(i);
		// Make sure each ClusterInfo structure can hold RouteInfo pointers to every other cluster
		_clusterInfo[i]->routeToCluster.resize(_clusterInfo.size());
		_clusterInfo[i]->visitedToCluster.assign(_clusterInfo.size(), false);
	}
}

int tdmEAS::GetAreaNumForPosition(const idVec3& position)
{
	int areaNum = _aas->file->PointReachableAreaNum(position, _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

	// If areaNum could not be determined, try again at a slightly higher position
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,16), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,32), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,48), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

	return areaNum;
}

void tdmEAS::AssignElevatorsToClusters()
{
	_elevatorStations.clear();
	std::size_t ignoredStations = 0;

	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();

		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();

		for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
		{
			CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
									
			int areaNum = GetAreaNumForPosition(positionEnt->GetPhysics()->GetOrigin());

			if (areaNum == 0)
			{
				DM_LOG(LC_AI, LT_WARNING)LOGSTRING("[%s]: Cannot assign multistatemover position to AAS area:  %s\r", _aas->name.c_str(), positionEnt->name.c_str());
				ignoredStations++;
				continue;
			}

			const aasArea_t& area = _aas->file->GetArea(areaNum);
			_clusterInfo[area.cluster]->numElevatorStations++;

			// Allocate a new ElevatorStationStructure for this station and fill in the data
			ElevatorStationInfoPtr station(new ElevatorStationInfo);

			station->elevator = elevator;
			station->elevatorPosition = positionEnt;
			station->areaNum = areaNum;
			station->clusterNum = area.cluster;
			station->elevatorNum = i; // grayman #3005 - this needs to be initialized

			_elevatorStations.push_back(station);
		}
	}

	gameLocal.Printf("[%s]: Assigned %zu multistatemover positions to AAS areas and ignored %zu.\n", _aas->name.c_str(), _elevatorStations.size(), ignoredStations);
}

// grayman - debug route info
#if 0
void tdmEAS::PrintRoutes()
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Routing data ---------------------------------\r");
	for ( std::size_t startCluster = 1 ; startCluster < _clusterInfo.size() ; startCluster++ )
	{
		for ( std::size_t goalCluster = 1 ; goalCluster < _clusterInfo.size() ; goalCluster++ )
		{
			if ( startCluster == goalCluster )
			{
				continue;
			}

			int numRoutes = static_cast<int>(_clusterInfo[startCluster]->routeToCluster[goalCluster].size());
			if ( numRoutes == 0 )
			{
				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("No routes from %d to %d\r", startCluster, goalCluster);
				continue;
			}

			// print the routing nodes

			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("----- %d Routes from %d to %d\r", numRoutes, startCluster, goalCluster);
			RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

			int counter = 1;
			for ( RouteInfoList::const_iterator j = routes.begin() ; j != routes.end() ; ++j )
			{
				RouteType type = (*j)->routeType;
				idStr routeType = "BAD ROUTE TYPE";
				switch(type)
				{
				case ROUTE_DUMMY:
					routeType = "DUMMY";
					break;
				case ROUTE_TO_AREA:
					routeType = "AREA";
					break;
				case ROUTE_TO_CLUSTER:
					routeType = "CLUSTER";
					break;
				default:
					break;
				}
				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     ==========\r");
				DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     Route %d is type %s with routeTravelTime %d\r", counter, routeType.c_str(),(*j)->routeTravelTime);
				RouteNodeList& routeNodes = (*j)->routeNodes;
				for ( RouteNodeList::const_iterator k = routeNodes.begin() ; k != routeNodes.end() ; ++k )
				{
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     node----------\r");
					ActionType actionType = (*k)->type;
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("          ActionType = %s\r", actionType == ACTION_WALK ? "WALK" : "ELEVATOR");
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("              toArea = %d\r", (*k)->toArea);
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("           toCluster = %d\r", (*k)->toCluster);
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("            elevator = %d\r", (*k)->elevator);
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     elevatorStation = %d\r", (*k)->elevatorStation);
					DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("      nodeTravelTime = %d\r", (*k)->nodeTravelTime);
				}
				counter++;
			}
		}
	}
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("End of routing data ---------------------------------\r");
}
#endif

void tdmEAS::SetupClusterRouting()
{
	// First, find all the reachable elevator stations (for each cluster)
	SetupReachableElevatorStations();

	// At this point, all clusters know their reachable elevator stations (numReachableElevatorStations is set)
	SetupRoutesBetweenClusters();

	// Remove all dummy routes
	CondenseRouteInfo();

//	PrintRoutes(); // print condensed routes

	SumRouteTravelTimes(); // grayman #3029

	// Sort the remaining routes (travel time is the key) // grayman #3029 - change hop check to travelTime check
	SortRoutes();

//	PrintRoutes(); // print final routes
}

void tdmEAS::SetupRoutesBetweenClusters()
{
	// grayman #3029 - this was clearing a cluster->cluster route list,
	// then filling it in using the same loop. The problem with this is
	// that some routes further down in the list may be filled in by
	// routes earlier in the list, because the further ones are subroutes
	// of the earlier ones. Then this loop would wipe out any work that
	// was already done when the later instance of the route was reached.

	// Instead, clear the routes in one loop and fill them in using a second loop

	// Clear routing lists.

	//stgatilov #4755: precompute areas for clusters to improve asymptotic time complexity
	std::vector<int> areaOfCluster(_clusterInfo.size());
	for (size_t cluster = 0; cluster < _clusterInfo.size(); cluster++)
		areaOfCluster[cluster] = _aas->GetAreaInCluster(cluster);

	for ( std::size_t startCluster = 0 ; startCluster < _clusterInfo.size() ; startCluster++ )
	{
		int startArea = areaOfCluster[startCluster];

		if (startArea <= 0)
		{
			continue;
		}
		
		for ( std::size_t goalCluster = 0 ; goalCluster < _clusterInfo[startCluster]->routeToCluster.size() ; goalCluster++ )
		{
			_clusterInfo[startCluster]->routeToCluster[goalCluster].clear();
		}
	}

	// Fill in routing lists.

	for ( std::size_t startCluster = 0 ; startCluster < _clusterInfo.size() ; startCluster++ )
	{
		int startArea = areaOfCluster[startCluster];

		if (startArea <= 0)
		{
			continue;
		}
		
		for ( std::size_t goalCluster = 0 ; goalCluster < _clusterInfo[startCluster]->routeToCluster.size() ; goalCluster++ )
		{
			if ( _clusterInfo[startCluster]->routeToCluster[goalCluster].size() > 0 )
			{
				continue; // routes already established from startCluster to goalCluster
			}

			if ( goalCluster == startCluster )
			{
				continue;
			}

			int goalArea = areaOfCluster[goalCluster];
			if ( goalArea <= 0 )
			{
				continue;
			}
			
			_routingIterations = 0;
			FindRoutesToCluster(static_cast<int>(startCluster), startArea, static_cast<int>(goalCluster), goalArea);
		}

		common->PacifierUpdate(LOAD_KEY_ROUTING_INTERIM,(int)startCluster + 1); // grayman #3763
	}
}

// grayman #3029 - sum the travel times for each node in a route and place the total in the route info

void tdmEAS::SumRouteTravelTimes()
{
	for ( std::size_t startCluster = 0 ; startCluster < _clusterInfo.size() ; startCluster++ )
	{
		for ( std::size_t goalCluster = 0 ; goalCluster < _clusterInfo[startCluster]->routeToCluster.size() ; goalCluster++ )
		{
			RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

			if ( !routes.empty() )
			{
				for ( RouteInfoList::const_iterator route = routes.begin() ; route != routes.end() ; route++ )
				{
					RouteNodeList& routeNodes = (*route)->routeNodes;
					int travelTime = 0;
					for ( RouteNodeList::const_iterator node = routeNodes.begin() ; node != routeNodes.end() ; node++ )
					{
						travelTime += (*node)->nodeTravelTime;
					}
					(*route)->routeTravelTime = travelTime;
				}
			}
		}
	}
}

void tdmEAS::SortRoutes()
{
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
            SortRoute(static_cast<int>(startCluster), static_cast<int>(goalCluster));
		}
	}
}

void tdmEAS::SortRoute(int startCluster, int goalCluster)
{
	// grayman #3029 - hop sorting can throw away the most desirable elevator
	// route. Sort by travelTime instead.

	// Use a std::map to sort the Routes by travelTime.
	typedef std::map<std::size_t, RouteInfoPtr> RouteSortMap;

	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	RouteSortMap sorted;

	// Insert the routeInfo structures, using the routeTravelTime as index.

	for (RouteInfoList::const_iterator i = routeList.begin(); i != routeList.end(); ++i)
	{
		sorted.insert(RouteSortMap::value_type(
			(*i)->routeTravelTime,	// travelTime
			(*i)					// RouteInfoPtr
		));
	}

	// Wipe the unsorted list
	routeList.clear();

	// Re-insert the items.
	// For ELEVATOR routes, keep only the shortest. It should be the first
	// ELEVATOR route in the route list, because the list was sorted above,
	// but we'll check them all just in case.

	// Pure WALK routes have only 1 route node. ELEVATOR routes have at least
	// 2 route nodes, so we'll recognize ELEVATOR routes if their node count
	// is > 1.

	int bestTime = 100000;
	for ( RouteSortMap::const_iterator i = sorted.begin() ; i != sorted.end() ; i++ )
	{
		if ( (i->second)->routeNodes.size() > 1 )
		{
			int routeTravelTime = (i->second)->routeTravelTime;
			if ( routeTravelTime > bestTime )
			{
				continue; // ignore
			}
			bestTime = routeTravelTime;
		}
		
		routeList.push_back(i->second);
	}
}

void tdmEAS::CondenseRouteInfo()
{
	// Disregard empty or invalid RouteInfo structures
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
            CleanRouteInfo(static_cast<int>(startCluster), static_cast<int>(goalCluster));
		}
	}
}

void tdmEAS::SetupReachableElevatorStations()
{
	for (std::size_t cluster = 0; cluster < _clusterInfo.size(); cluster++)
	{
		// Find an area within that cluster
		int areaNum = _aas->GetAreaInCluster(static_cast<int>(cluster));

		if (areaNum <= 0)
		{
			continue;
		}

		idList<int> elevatorStationIndices;

		// For each cluster, try to setup a route to all elevator stations
		for (std::size_t e = 0; e < _elevatorStations.size(); e++)
		{
			if (_elevatorStations[e] == NULL)
			{
				continue;
			}

			/*idBounds areaBounds = _aas->GetAreaBounds(areaNum);
			idVec3 areaCenter = _aas->AreaCenter(areaNum);

			gameRenderWorld->DebugText(va("%d", areaNum), areaCenter, 0.2f, colorRed, idAngles(0,0,0).ToMat3(), 1, 50000);
			gameRenderWorld->DebugBox(colorRed, idBox(areaBounds), 50000);

			areaBounds = _aas->GetAreaBounds(_elevatorStations[e]->areaNum);
			idVec3 areaCenter2 = _aas->AreaCenter(_elevatorStations[e]->areaNum);

			gameRenderWorld->DebugText(va("%d", _elevatorStations[e]->areaNum), areaCenter2, 0.2f, colorBlue, idAngles(0,0,0).ToMat3(), 1, 50000);
			gameRenderWorld->DebugBox(colorBlue, idBox(areaBounds), 50000);*/

			idReachability* reach;
			int travelTime = 0;
			bool routeFound = _aas->RouteToGoalArea(areaNum, _aas->AreaCenter(areaNum), 
				_elevatorStations[e]->areaNum, TFL_WALK|TFL_AIR, travelTime, &reach, NULL, NULL);

			//gameRenderWorld->DebugArrow(routeFound ? colorGreen : colorRed, areaCenter, areaCenter2, 1, 50000);
			
			if (routeFound) 
			{
				//gameLocal.Printf("Cluster %d can reach elevator station %s\n", cluster, _elevatorStations[e]->elevatorPosition.GetEntity()->name.c_str());
				// Add the elevator index to the list
				_clusterInfo[cluster]->reachableElevatorStations.push_back(_elevatorStations[e]);
			}
		}
	}
}

void tdmEAS::Save(idSaveGame* savefile) const
{
	// Elevators
	savefile->WriteInt(_elevators.Num());
	for (int i = 0; i < _elevators.Num(); i++)
	{
		_elevators[i].Save(savefile);
	}

	// ClusterInfos
	savefile->WriteInt(static_cast<int>(_clusterInfo.size()));
	for (std::size_t i = 0; i < _clusterInfo.size(); i++)
	{
		_clusterInfo[i]->Save(savefile);
	}

	// ElevatorStations
	savefile->WriteInt(static_cast<int>(_elevatorStations.size()));
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		_elevatorStations[i]->Save(savefile);
	}
}

void tdmEAS::Restore(idRestoreGame* savefile)
{
	int num;

	// Elevators
	savefile->ReadInt(num);
	_elevators.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		_elevators[i].Restore(savefile);
	}

	// Cluster Infos
	savefile->ReadInt(num);
	_clusterInfo.resize(num);
	for (int i = 0; i < num; i++)
	{
		_clusterInfo[i] = ClusterInfoPtr(new ClusterInfo);
		_clusterInfo[i]->Restore(savefile);
	}

	// ElevatorStations
	_elevatorStations.clear();
	savefile->ReadInt(num);
	_elevatorStations.resize(num);
	for (int i = 0; i < num; i++)
	{
		_elevatorStations[i] = ElevatorStationInfoPtr(new ElevatorStationInfo);
		_elevatorStations[i]->Restore(savefile);
	}
}

bool tdmEAS::InsertUniqueRouteInfo(int startCluster, int goalCluster, RouteInfoPtr route)
{
	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	for (RouteInfoList::iterator i = routeList.begin(); i != routeList.end(); ++i)
	{
		RouteInfoPtr& existing = *i;
		
		if (*route == *existing)
		{
			return false; // Duplicate
		}
	}

	routeList.push_back(route);
	return true;
}

void tdmEAS::CleanRouteInfo(int startCluster, int goalCluster)
{
	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	for (RouteInfoList::iterator i = routeList.begin(); i != routeList.end(); /* in-loop increment */)
	{
		if ((*i)->routeNodes.empty() || (*i)->routeType == ROUTE_DUMMY)
		{
			routeList.erase(i++);
		}
		else
		{
			++i;
		}
	}
}

ElevatorStationInfoPtr tdmEAS::GetElevatorStationInfo(int index)
{
	if ( (index >= 0) && (index < static_cast<int>(_elevatorStations.size()))) // grayman #4229
	//if (index >= 0 || index < static_cast<int>(_elevatorStations.size())) // very bad
	{
		return _elevatorStations[static_cast<std::size_t>(index)];
	}

	return ElevatorStationInfoPtr();
}

RouteInfoList tdmEAS::FindRoutesToCluster(int startCluster, int startArea, int goalCluster, int goalArea)
{
	_routingIterations++;

	/**
	 * greebo: Pseudo-Code:
	 *
	 * 1. Check if we already have routing information to the target cluster, exit if yes
	 * 2. Check if we can walk right up to the target cluster, if yes: fill in the RouteNode and exit
	 * 3. Check all reachable elevator stations and all clusters reachable from there. Go to 1. for each of those.
	 */

	if (startCluster == goalCluster)
	{
		// Do nothing for start == goal
	}
	else if (_clusterInfo[startCluster]->visitedToCluster[goalCluster])
	{
		// Routing information to the goal cluster is right there, return it
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("We already tried to find a route from cluster %d to %d.\r", startCluster, goalCluster);
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Route from cluster %d to %d doesn't exist yet, check walk path.\r", startCluster, goalCluster);

		// Remember that we have computed this info in _clusterInfo matrix, so that we don't come here again
		_clusterInfo[startCluster]->visitedToCluster[goalCluster] = true;

		// No routing information, check walk path to the goal cluster
		idReachability* reach;
		int travelTime = 0;
		// Workaround: Include the TFL_INVALID flag to include deactivated AAS areas
		bool routeFound = _aas->RouteToGoalArea(startArea, _aas->AreaCenter(startArea), 
			goalArea, TFL_WALK|TFL_AIR|TFL_INVALID, travelTime, &reach, NULL, NULL);

		if (routeFound) 
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Can walk from cluster %d to %d.\r", startCluster, goalCluster);

			// Walk path possible, allocate a new RouteInfo with a WALK node
			RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));

			RouteNodePtr node(new RouteNode(ACTION_WALK, goalArea, goalCluster, -1, -1, travelTime)); // grayman #3029 - add the travelTime for a WALK node
			info->routeNodes.push_back(node);

			// Save this WALK route into the cluster
			InsertUniqueRouteInfo(startCluster, goalCluster, info);
		}

		// grayman #3029 - whether we found a walking route or not,
		// let's see if there's an elevator route.

		// Check all elevator stations that are reachable from this cluster
		for (ElevatorStationInfoList::const_iterator station = _clusterInfo[startCluster]->reachableElevatorStations.begin();
				station != _clusterInfo[startCluster]->reachableElevatorStations.end(); ++station)
		{
			const ElevatorStationInfoPtr& elevatorInfo = *station;
			int startStationIndex = GetElevatorStationIndex(elevatorInfo);

			// Get all stations reachable via this elevator
			const idList<MoverPositionInfo>& positionList = elevatorInfo->elevator.GetEntity()->GetPositionInfoList();

			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found %d elevator stations reachable from cluster %d (goal cluster = %d).\r", positionList.Num(), startCluster, goalCluster);

			// Now look at all elevator floors and route from there
			for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
			{
				CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();

				if (positionEnt == elevatorInfo->elevatorPosition.GetEntity())
				{
					// grayman #3029 - if we comment this line, does this let AI walk
					// across a parked elevator to get to the hallway beyond? (Answer: Yes in one direction, NO in the other)
					// Also, does it screw up normal elevator use? (Yes)
					continue; // this is the same position we are starting from, skip that
				}

				int targetStationIndex = GetElevatorStationIndex(positionEnt);

				int nextArea = GetAreaNumForPosition(positionEnt->GetPhysics()->GetOrigin());

				if (nextArea <= 0)
				{
					continue;
				}

				int nextCluster = _aas->file->GetArea(nextArea).cluster;

				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Checking elevator station %d reachable from cluster %d (goal cluster = %d).\r", startStationIndex, startCluster, goalCluster);

				if (nextCluster == goalCluster)
				{
					// Hooray, the elevator leads right to the goal cluster, write that down
					RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));

					// Walk to the elevator station and use it

					// grayman #3029 - Need to get travelTime for that walk

					int t = 0; // travelTime
					if ( startArea != elevatorInfo->areaNum )
					{
						idReachability* r;
						// Workaround: Include the TFL_INVALID flag to include deactivated AAS areas
						_aas->RouteToGoalArea(startArea, _aas->AreaCenter(startArea),elevatorInfo->areaNum, TFL_WALK|TFL_AIR|TFL_INVALID, t, &r, NULL, NULL);
					}

					RouteNodePtr walkNode(new RouteNode(ACTION_WALK, elevatorInfo->areaNum, elevatorInfo->clusterNum, elevatorInfo->elevatorNum, startStationIndex, t)); // grayman #3029
						
					// Use the elevator to reach the elevator station in the next cluster
					int elevatorTravelTime = ( positionEnt->GetPhysics()->GetOrigin() - elevatorInfo->elevatorPosition.GetEntity()->GetPhysics()->GetOrigin() ).LengthFast()/(elevatorInfo->elevator.GetEntity()->GetMoveSpeed());
					RouteNodePtr useNode(new RouteNode(ACTION_USE_ELEVATOR,
														nextArea,
														nextCluster,
														elevatorInfo->elevatorNum,
														targetStationIndex,
														ELEVATOR_TRAVEL_FACTOR*elevatorTravelTime)); // grayman #3029 - factor needed to normalize with horizontal traveltimes

					info->routeNodes.push_back(walkNode);
					info->routeNodes.push_back(useNode);

					// Save this USE_ELEVATOR route into this startCluster
					InsertUniqueRouteInfo(startCluster, goalCluster, info);

					DM_LOG(LC_AI, LT_INFO)LOGSTRING("Elevator leads right to the target cluster %d.\r", goalCluster);
				}
				else 
				{
					DM_LOG(LC_AI, LT_INFO)LOGSTRING("Investigating route to target cluster %d, starting from station cluster %d.\r", goalCluster, nextCluster);
					// The elevator station does not start in the goal cluster, find a way from there
					RouteInfoList routes = FindRoutesToCluster(nextCluster, nextArea, goalCluster, goalArea);

					for (RouteInfoList::iterator i = routes.begin(); i != routes.end(); ++i)
					{
						RouteInfoPtr& foundRoute = *i;

						// Evaluate the suggested route (check for redundancies)
						if (!EvaluateRoute(startCluster, goalCluster, elevatorInfo->elevatorNum, foundRoute))
						{
							continue;
						}

						// Route was accepted, copy it
						RouteInfoPtr newRoute(new RouteInfo(*foundRoute));

						// Append the valid route objects to the existing chain, but add a "walk to elevator station" to the front
						int elevatorTravelTime = ( positionEnt->GetPhysics()->GetOrigin() - elevatorInfo->elevatorPosition.GetEntity()->GetPhysics()->GetOrigin() ).LengthFast()/(elevatorInfo->elevator.GetEntity()->GetMoveSpeed());
						RouteNodePtr useNode(new RouteNode(ACTION_USE_ELEVATOR,
															nextArea,
															nextCluster,
															elevatorInfo->elevatorNum,
															targetStationIndex,
															ELEVATOR_TRAVEL_FACTOR*elevatorTravelTime)); // // grayman #3029 - factor needed to normalize with horizontal traveltimes

						// grayman #3029 - Need to get travelTime for the walk node

						int t = 0; // travelTime
						if ( startArea != elevatorInfo->areaNum )
						{
							idReachability* r;
							// Workaround: Include the TFL_INVALID flag to include deactivated AAS areas
							_aas->RouteToGoalArea(startArea, _aas->AreaCenter(startArea),elevatorInfo->areaNum, TFL_WALK|TFL_AIR|TFL_INVALID, t, &r, NULL, NULL);
						}

						RouteNodePtr walkNode(new RouteNode(ACTION_WALK, elevatorInfo->areaNum, elevatorInfo->clusterNum, elevatorInfo->elevatorNum, startStationIndex, t)); // grayman #3029

						newRoute->routeNodes.push_front(useNode);
						newRoute->routeNodes.push_front(walkNode);
							
						// Add the compiled information to our repository
						InsertUniqueRouteInfo(startCluster, goalCluster, newRoute);
					}
				}
			}
		}
	}

	// Purge all empty RouteInfo nodes
	CleanRouteInfo(startCluster, goalCluster);

	assert(_routingIterations > 0);
	_routingIterations--;

	return _clusterInfo[startCluster]->routeToCluster[goalCluster];
}

bool tdmEAS::EvaluateRoute(int startCluster, int goalCluster, int forbiddenElevator, RouteInfoPtr route)
{
	// Don't regard empty or dummy routes
	if (route == NULL || route->routeType == ROUTE_DUMMY || route->routeNodes.empty()) 
	{
		return false;
	}

	// Does the route come across our source cluster?
	for (RouteNodeList::const_iterator node = route->routeNodes.begin() ; node != route->routeNodes.end(); ++node)
	{
		if ((*node)->toCluster == startCluster || (*node)->elevator == forbiddenElevator)
		{
			// Route is coming across the same cluster or elevator since we started, so this is a loop. Reject.
			return false;
		}
	}
	
	return true; // accepted
}

int tdmEAS::GetElevatorIndex(CMultiStateMover* mover)
{
	for (int i = 0; i < _elevators.Num(); i++)
	{
		if (_elevators[i].GetEntity() == mover)
		{
			return i; // found!
		}
	}

	return -1; // not found
}

int tdmEAS::GetElevatorStationIndex(ElevatorStationInfoPtr info)
{
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		if (_elevatorStations[i] == info)
		{
			return static_cast<int>(i); // found!
		}
	}

	return -1;
}

int tdmEAS::GetElevatorStationIndex(CMultiStateMoverPosition* positionEnt)
{
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		if (_elevatorStations[i] != NULL && _elevatorStations[i]->elevatorPosition.GetEntity() == positionEnt)
		{
			return static_cast<int>(i); // found!
		}
	}

	return -1;
}

// grayman #3029 - look at the first route to see if it's an ELEVATOR route
bool tdmEAS::FindRouteToGoal(aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idActor* actor, int &elevatorTravelTime) // grayman #3029
{
	assert(_aas != NULL);
	int startCluster = _aas->file->GetArea(areaNum).cluster;
	int goalCluster = _aas->file->GetArea(goalAreaNum).cluster;

	bool result = false; // grayman #3029 - assume failure

	// Check if we are starting from a portal
	if (startCluster < 0)
	{
		startCluster = _aas->file->GetPortal(-startCluster).clusters[0];
	}

	// Check if we are going to a portal
	if (goalCluster < 0)
	{
		goalCluster = _aas->file->GetPortal(-goalCluster).clusters[0];
	}

	const RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	// Check all routes to the target area
	if ( !routes.empty() )
	{
		RouteInfoList::const_iterator route = routes.begin();

		if ( (*route)->routeNodes.size() < 2 ) // Valid elevator routes have at least two nodes
		{
			return false; // we were looking for an ELEVATOR route as the first route in the list and didn't find one
		}

		// grayman #4466 - if the elevator is disabled, we can't go that way

		RouteNodeList& routeNodes = (*route)->routeNodes;

		for ( RouteNodeList::const_iterator node = routeNodes.begin(); node != routeNodes.end(); node++ )
		{
			if ( (*node)->type == ACTION_USE_ELEVATOR )
			{
				int elevatorNumber = (*node)->elevator;
				if ( (elevatorNumber >= 0) && (elevatorNumber < _elevators.Num() ))
				{
					CMultiStateMover* elevator = _elevators[elevatorNumber].GetEntity();
					if ( !elevator->spawnArgs.GetBool("enabled", "1") )
					{
						return false;
					}
				}

				break;
			}
		}

		// We have a valid ELEVATOR route, set the elevator flag on the path type

		path.type = PATHTYPE_ELEVATOR;
		path.moveGoal = goalOrigin;
		path.moveAreaNum = goalAreaNum;

#if 0
		// grayman - for debugging, print the nodes for this route
		RouteType type = (*route)->routeType;
		DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     type = %s for route 1\r", type == ROUTE_TO_AREA ? "AREA" : "CLUSTER");
		routeNodes = (*route)->routeNodes;

		for ( RouteNodeList::const_iterator node = routeNodes.begin() ; node != routeNodes.end() ; node++ )
		{
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     node:\r");
			ActionType actionType = (*node)->type;
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("          ActionType = %s\r", actionType == ACTION_WALK ? "WALK" : "ELEVATOR");
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("              toArea = %d\r", (*node)->toArea);
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("           toCluster = %d\r", (*node)->toCluster);
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("            elevator = %d\r", (*node)->elevator);
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("     elevatorStation = %d\r", (*node)->elevatorStation);
			DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING("      nodeTravelTime = %d\r", (*node)->nodeTravelTime);
		}
#endif
		path.elevatorRoute = *route;
		elevatorTravelTime = (*route)->routeTravelTime;
		result = true;
	}

	return result;
}

/* grayman #4229
// grayman #3548
CMultiStateMover* tdmEAS::GetNearbyElevator(idVec3 pos, float maxDist, float maxVertDist)
{
	CMultiStateMover* elevator = NULL;
	for ( int j = 0 ; (j < _elevators.Num()) && (elevator == NULL) ; j++ )
	{
		CMultiStateMover* ent = _elevators[ j ].GetEntity();
		if (ent)
		{
			const idList<MoverPositionInfo>& positionList = ent->GetPositionInfoList();

			for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
			{
				CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();

				// grayman #4229 - verify that this position entity belongs to an elevator
				// that the AI can use (as opposed to, for example, dumbwaiters)
		
				int stationIndex = GetElevatorStationIndex(positionEnt);

				// If stationIndex is < 0, the AI can't use the position entity.

				if ( stationIndex < 0 )
				{
					continue;
				}

				idVec3 entOrigin = positionEnt->GetPhysics()->GetOrigin();
				float dist = (pos - entOrigin).LengthFast();
				if (dist < maxDist)
				{
					float vertDist = idMath::Abs(pos.z - entOrigin.z);
					if (vertDist < maxVertDist)
					{
						elevator = ent;
						break;
					}
				}
			}
		}
	}

	return elevator;
}
*/

void tdmEAS::DrawRoute(int startArea, int goalArea)
{
	int startCluster = _aas->file->GetArea(startArea).cluster;
	int goalCluster = _aas->file->GetArea(goalArea).cluster;

	if (startCluster < 0 || goalCluster < 0)
	{
		gameLocal.Warning("Cannot draw route, cluster numbers < 0.");
		return;
	}

	const RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	// Draw all routes to the target area
	for (RouteInfoList::const_iterator r = routes.begin(); r != routes.end(); ++r)
	{
		const RouteInfoPtr& route = *r;

		RouteNodePtr prevNode(new RouteNode(ACTION_WALK, startArea, startCluster));
		
		for (RouteNodeList::const_iterator n = route->routeNodes.begin(); n != route->routeNodes.end(); ++n)
		{
			RouteNodePtr node = *n;

			if (prevNode != NULL)
			{
				idVec4 colour = (node->type == ACTION_WALK) ? colorBlue : colorCyan;
				idVec3 start = _aas->file->GetArea(node->toArea).center;
				idVec3 end = _aas->file->GetArea(prevNode->toArea).center;
				gameRenderWorld->DebugArrow(colour, start, end, 1, 5000);
			}

			prevNode = node;
		}
	}
}

} // namespace eas
