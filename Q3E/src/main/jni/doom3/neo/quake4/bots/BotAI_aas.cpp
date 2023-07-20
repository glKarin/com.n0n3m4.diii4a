// #ifdef MOD_BOTS
// From ai/AAS_routing.cpp, ai/AAS_pathing.cpp

#define SUBSAMPLE_WALK_PATH		1
#define SUBSAMPLE_FLY_PATH		0
#define MAX_ROUTING_CACHE_MEMORY	(4*1024*1024) // TinMan: 4Mb

extern const int		maxWalkPathIterations/*		= 10*/;
extern const float		maxWalkPathDistance/*			= 500.0f*/;
extern const float		walkPathSampleDistance/*		= 8.0f*/;

extern const int		maxFlyPathIterations/*		= 10*/;
extern const float		maxFlyPathDistance/*			= 500.0f*/;
extern const float		flyPathSampleDistance/*		= 8.0f*/;

bool botAi::RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach ) const {
	int clusterNum, goalClusterNum, portalNum, i, clusterAreaNum;
	unsigned short int t, bestTime;
	const aasPortal_t *portal;
	const aasCluster_t *cluster;
	idRoutingCache *areaCache, *portalCache, *clusterCache;
	idReachability *bestReach, *r, *nextr;

	travelTime = 0;
	*reach = NULL;

	if(!aas)
		return false;
	const idAASLocal *aasLocal = (const idAASLocal *)aas;
	idAASFile *file = aas->GetFile();
	if ( !file ) {
		return false;
	}

	if ( areaNum == goalAreaNum ) {
		return true;
	}

	if ( areaNum <= 0 || areaNum >= file->GetNumAreas() ) {
// RAVEN BEGIN
// bgeisler:
//		gameLocal.Printf( "RouteToGoalArea: areaNum %d out of range\n", areaNum );
// RAVEN END
		return false;
	}
	if ( goalAreaNum <= 0 || goalAreaNum >= file->GetNumAreas() ) {
// RAVEN BEGIN
// bgeisler:
//		gameLocal.Printf( "RouteToGoalArea: goalAreaNum %d out of range\n", goalAreaNum );
// RAVEN END
		return false;
	}

	while( aasLocal->totalCacheMemory > MAX_ROUTING_CACHE_MEMORY ) {
		aasLocal->DeleteOldestCache();
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the source area is a cluster portal, read directly from the portal cache
	if ( clusterNum < 0 ) {
		// if the goal area is a portal
		if ( goalClusterNum < 0 ) {
			// just assume the goal area is part of the front cluster
			portal = &file->GetPortal( -goalClusterNum );
			goalClusterNum = portal->clusters[0];
		}
		// get the portal routing cache
		portalCache = aasLocal->GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );
		*reach = aasLocal->GetAreaReachability( areaNum, portalCache->reachabilities[-clusterNum] );
		if(!*reach)
			return false;
		travelTime = portalCache->travelTimes[-clusterNum] + aasLocal->AreaTravelTime( areaNum, origin, (*reach)->start );
		return true;
	}

	bestTime = 0;
	bestReach = NULL;

	// check if the goal area is a portal of the source area cluster
	if ( goalClusterNum < 0 ) {
		portal = &file->GetPortal( -goalClusterNum );
		if ( portal->clusters[0] == clusterNum || portal->clusters[1] == clusterNum) {
			goalClusterNum = clusterNum;
		}
	}

	// if both areas are in the same cluster
	if ( clusterNum > 0 && goalClusterNum > 0 && clusterNum == goalClusterNum ) {
		clusterCache = aasLocal->GetAreaRoutingCache( clusterNum, goalAreaNum, travelFlags );
		clusterAreaNum = aasLocal->ClusterAreaNum( clusterNum, areaNum );
		if ( clusterCache->travelTimes[clusterAreaNum] ) {
			bestReach = aasLocal->GetAreaReachability( areaNum, clusterCache->reachabilities[clusterAreaNum] );
			if(!bestReach)
				return false;
			bestTime = clusterCache->travelTimes[clusterAreaNum] + aasLocal->AreaTravelTime( areaNum, origin, bestReach->start );
		}
		else {
			clusterCache = NULL;
		}
	}
	else {
		clusterCache = NULL;
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the goal area is a portal
	if ( goalClusterNum < 0 ) {
		// just assume the goal area is part of the front cluster
		portal = &file->GetPortal( -goalClusterNum );
		goalClusterNum = portal->clusters[0];
	}
	// get the portal routing cache
	portalCache = aasLocal->GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );

	// the cluster the area is in
	cluster = &file->GetCluster( clusterNum );
	// current area inside the current cluster
	clusterAreaNum = aasLocal->ClusterAreaNum( clusterNum, areaNum );
	// if the area is not a reachable area
	if ( clusterAreaNum >= cluster->numReachableAreas) {
		return false;
	}

	// find the portal of the source area cluster leading towards the goal area
	for ( i = 0; i < cluster->numPortals; i++ ) {
		portalNum = file->GetPortalIndex( cluster->firstPortal + i );

		// if the goal area isn't reachable from the portal
		if ( !portalCache->travelTimes[portalNum] ) {
			continue;
		}

		portal = &file->GetPortal( portalNum );
		// get the cache of the portal area
		areaCache = aasLocal->GetAreaRoutingCache( clusterNum, portal->areaNum, travelFlags );
		// if the portal is not reachable from this area
		if ( !areaCache->travelTimes[clusterAreaNum] ) {
			continue;
		}

		r = aasLocal->GetAreaReachability( areaNum, areaCache->reachabilities[clusterAreaNum] );
		if(!r)
			continue;

		if ( clusterCache ) {
			// if the next reachability from the portal leads back into the cluster
			nextr = aasLocal->GetAreaReachability( portal->areaNum, portalCache->reachabilities[portalNum] );
			if(!nextr)
				continue;
			if ( file->GetArea( nextr->toAreaNum ).cluster < 0 || file->GetArea( nextr->toAreaNum ).cluster == clusterNum ) {
				continue;
			}
		}

		// the total travel time is the travel time from the portal area to the goal area
		// plus the travel time from the source area towards the portal area
		t = portalCache->travelTimes[portalNum] + areaCache->travelTimes[clusterAreaNum];
		// NOTE:	Should add the exact travel time through the portal area.
		//			However we add the largest travel time through the portal area.
		//			We cannot directly calculate the exact travel time through the portal area
		//			because the reachability used to travel into the portal area is not known.
		t += portal->maxAreaTravelTime;

		// if the time is better than the one already found
		if ( !bestTime || t < bestTime ) {
			bestReach = r;
			bestTime = t;
		}
	}

	if ( !bestReach ) {
		return false;
	}

	*reach = bestReach;
	travelTime = bestTime;

	return true;
}

bool botAi::WalkPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const {
	int i, travelTime, curAreaNum, lastAreas[4], lastAreaIndex, endAreaNum;
	idReachability *reach = NULL;
	idVec3 endPos;

	path.type = PATHTYPE_WALK;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = NULL;

	if(!aas) {
		path.moveGoal = goalOrigin;
		return true;
	}
	const idAASLocal *aasLocal = (const idAASLocal *)aas;
	const idAASFile *file = aas->GetFile();

	if ( file == NULL || areaNum == goalAreaNum ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	lastAreas[0] = lastAreas[1] = lastAreas[2] = lastAreas[3] = areaNum;
	lastAreaIndex = 0;

	curAreaNum = areaNum;

	for ( i = 0; i < maxWalkPathIterations; i++ ) {

		if ( !botAi::RouteToGoalArea( curAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			return false;
		}

 		// RAVEN BEGIN
 		// cdr: Alternate Routes Bug
 		path.reachability = reach;
 		// RAVEN END

		// no need to check through the first area
		if ( areaNum != curAreaNum ) {
			// only optimize a limited distance ahead
			if ( (reach->start - origin).LengthSqr() > Square( maxWalkPathDistance ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = aasLocal->SubSampleWalkPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}

			if ( !aasLocal->WalkPathValid( areaNum, origin, 0, reach->start, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = aasLocal->SubSampleWalkPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
		}

		path.moveGoal = reach->start;
		path.moveAreaNum = curAreaNum;

		if ( reach->travelType != TFL_WALK ) {
			break;
		}

		if ( !aasLocal->WalkPathValid( areaNum, origin, 0, reach->end, travelFlags, endPos, endAreaNum ) ) {
			return true;
		}

		path.moveGoal = reach->end;
		path.moveAreaNum = reach->toAreaNum;

		if ( reach->toAreaNum == goalAreaNum ) {
			if ( !aasLocal->WalkPathValid( areaNum, origin, 0, goalOrigin, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = aasLocal->SubSampleWalkPath( areaNum, origin, path.moveGoal, goalOrigin, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}

		lastAreas[lastAreaIndex] = curAreaNum;
		lastAreaIndex = ( lastAreaIndex + 1 ) & 3;

		curAreaNum = reach->toAreaNum;

		if ( curAreaNum == lastAreas[0] || curAreaNum == lastAreas[1] ||
				curAreaNum == lastAreas[2] || curAreaNum == lastAreas[3] ) {
			//k common->Warning( "idAASLocal::WalkPathToGoal: local routing minimum going from area %d to area %d", areaNum, goalAreaNum );
			break;
		}
	}

	if ( !reach ) {
		return false;
	}

	switch( reach->travelType ) {
		case TFL_WALKOFFLEDGE:
			path.type = PATHTYPE_WALKOFFLEDGE;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		case TFL_BARRIERJUMP:
			path.type |= PATHTYPE_BARRIERJUMP;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		case TFL_JUMP:
			path.type |= PATHTYPE_JUMP;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		// MOD_BOTS begin
		case TFL_ELEVATOR:
            /* custom3: i took a look at this. it seems many of the plats, especially on ctf1,
            go through a realm of areaNum=0 on the way up. this is a problem. lol.
            what i started doing as a hack is something like this.
            */
            int time;
            idReachability *next;
            path.secondaryGoal = reach->end;
            this->RouteToGoalArea(reach->toAreaNum, reach->end, goalAreaNum, travelFlags, time, &next);
            while ( next && next->travelType == TFL_ELEVATOR )
            {
                path.secondaryGoal = next->end;
                this->RouteToGoalArea( next->toAreaNum, next->end, goalAreaNum, travelFlags, time, &next);
            }

            path.type |= PATHTYPE_ELEVATOR;
            path.reachability = reach;
            break;
        // MOD_BOTS end
		default:
			break;
	}

	return true;
}

bool botAi::FlyPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const {
	int i, travelTime, curAreaNum, lastAreas[4], lastAreaIndex, endAreaNum;
	idReachability *reach = NULL;
	idVec3 endPos;

	path.type = PATHTYPE_WALK;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = NULL;

	if(!aas) {
		path.moveGoal = goalOrigin;
		return true;
	}
	const idAASLocal *aasLocal = (const idAASLocal *)aas;
	const idAASFile *file = aas->GetFile();

	if ( file == NULL || areaNum == goalAreaNum ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	lastAreas[0] = lastAreas[1] = lastAreas[2] = lastAreas[3] = areaNum;
	lastAreaIndex = 0;

	curAreaNum = areaNum;

	for ( i = 0; i < maxFlyPathIterations; i++ ) {

		if ( !botAi::RouteToGoalArea( curAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			return false;
		}

		// no need to check through the first area
		if ( areaNum != curAreaNum ) {
			if ( (reach->start - origin).LengthSqr() > Square( maxFlyPathDistance ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = aasLocal->SubSampleFlyPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}

			if ( !aasLocal->FlyPathValid( areaNum, origin, 0, reach->start, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = aasLocal->SubSampleFlyPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
		}

		path.moveGoal = reach->start;
		path.moveAreaNum = curAreaNum;

		if ( !aasLocal->FlyPathValid( areaNum, origin, 0, reach->end, travelFlags, endPos, endAreaNum ) ) {
			return true;
		}

		path.moveGoal = reach->end;
		path.moveAreaNum = reach->toAreaNum;

		if ( reach->toAreaNum == goalAreaNum ) {
			if ( !aasLocal->FlyPathValid( areaNum, origin, 0, goalOrigin, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = aasLocal->SubSampleFlyPath( areaNum, origin, path.moveGoal, goalOrigin, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}

		lastAreas[lastAreaIndex] = curAreaNum;
		lastAreaIndex = ( lastAreaIndex + 1 ) & 3;

		curAreaNum = reach->toAreaNum;

		if ( curAreaNum == lastAreas[0] || curAreaNum == lastAreas[1] ||
				curAreaNum == lastAreas[2] || curAreaNum == lastAreas[3] ) {
            //k common->Warning( "idAASLocal::FlyPathToGoal: local routing minimum going from area %d to area %d", areaNum, goalAreaNum );
			break;
		}
	}

	if ( !reach ) {
		return false;
	}

	return true;
}

// #endif
