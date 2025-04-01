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

#ifndef __AI_EAS_ROUTEINFO_H__
#define __AI_EAS_ROUTEINFO_H__

#include <list>
#include <vector>
#include "RouteNode.h"

namespace eas {

enum RouteType {
	ROUTE_DUMMY = 0,	// placeholder
	ROUTE_TO_AREA,		// a route to an AAS area
	ROUTE_TO_CLUSTER,	// a route to an AAS cluster
	NUM_ROUTE_TYPES,
};

// A route info contains information of how to get to a specific target
struct RouteInfo
{
	RouteType routeType;		// ROUTE_TO_AREA or ROUTE_TO_CLUSTER, ...
	int target;					// either the target AREA or the target CLUSTER number, depending on routeType
	RouteNodeList routeNodes;	// contains the actual route node chain (WALK, USE_ELEVATOR, WALK, etc.)
	int routeTravelTime;		// grayman #3029 - sum of individual node travel times

	// Default constructor
	RouteInfo();

	// Specialised constructor
	RouteInfo(RouteType type, int targetNum);

	// Copy constructor
	RouteInfo(const RouteInfo& other);

	bool operator==(const RouteInfo& other) const;
	bool operator!=(const RouteInfo& other) const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};
typedef std::shared_ptr<RouteInfo> RouteInfoPtr;
typedef std::list<RouteInfoPtr> RouteInfoList;
typedef std::vector<RouteInfoList> RouteInfoListVector;

} // namespace eas

#endif /* __AI_EAS_ROUTEINFO_H__ */
