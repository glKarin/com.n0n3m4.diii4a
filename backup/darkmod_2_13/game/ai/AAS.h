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

#ifndef __AAS_H__
#define __AAS_H__

// Need linked list
#include "../../idlib/containers/List.h"
#include "EAS/RouteInfo.h"

class CFrobDoor;

/*
===============================================================================

	Area Awareness System

===============================================================================
*/

enum {
	PATHTYPE_WALK,
	PATHTYPE_WALKOFFLEDGE,
	PATHTYPE_BARRIERJUMP,
	PATHTYPE_JUMP,
	PATHTYPE_DOOR,
	PATHTYPE_ELEVATOR, // greebo: Added for TDM
};

typedef struct aasPath_s {
	int							type;			// path type
	idVec3						moveGoal;		// point the AI should move towards
	int							moveAreaNum;	// number of the area the AI should move towards
	idVec3						secondaryGoal;	// secondary move goal for complex navigation
	const idReachability *		reachability;	// reachability used for navigation
	eas::RouteInfoPtr			elevatorRoute;	// EAS reachability information (NULL if unused)
	CFrobDoor*					firstDoor;		// angua: when a door is in the path, this is stored here (NULL otherwise)
} aasPath_t;


typedef struct aasGoal_s {
	int							areaNum;		// area the goal is in
	idVec3						origin;			// position of goal
} aasGoal_t;


typedef struct aasObstacle_s {
	idBounds					absBounds;		// absolute bounds of obstacle
	idBounds					expAbsBounds;	// expanded absolute bounds of obstacle
} aasObstacle_t;

class idAASCallback {
public:
	virtual						~idAASCallback() {};
	virtual	bool				TestArea( const class idAAS *aas, int areaNum ) = 0;
};

typedef int aasHandle_t;

/**
* This is the typedef for a reachability tracking list
*/
typedef idList<idReachability*> TReachabilityTrackingList;
namespace eas { class tdmEAS; }

class idAAS {
public:
	static idAAS *				Alloc( void );
	virtual						~idAAS( void ) = 0;
								// Initialize for the given map.
	virtual bool				Init( const idStr &mapName, const unsigned int mapFileCRC ) = 0;
								// Print AAS stats.
	virtual void				Stats( void ) const = 0;
								// Test from the given origin.
	virtual void				Test( const idVec3 &origin ) = 0;
								// Get the AAS settings.
	virtual const idAASSettings *GetSettings( void ) const = 0;
								// Returns the number of the area the origin is in.
	virtual int					PointAreaNum( const idVec3 &origin ) const = 0;
								// Returns the number of the nearest reachable area for the given point.
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &bounds, const int areaFlags ) const = 0;
								// Returns the number of the first reachable area in or touching the bounds.
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const = 0;
	
	// Push the point into the area.
	/**
	 * greebo: Copied from d3world: this is what brian from id said about PushPointIntoAreaNum:
	 * 
	 * Quote: "If the point is already in the specified area, it does nothing, otherwise it 'pushes' 
	 *         the point into the area by moving it along the surface normal of all the planes that make up the area.
	 *         Imagine if an area were a box and the point were outside the bottom right side of the box,
	 *         it would first push the point along the right normal so it would be below the box, then it 
	 *         would push the point along the bottom normal so it would be inside the box."
	 *
	 * greebo: So basically, this alters the given idVec3 <origin>, so that it ends up being in the area box.
	 */
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const = 0;

								// Returns a reachable point inside the given area.
	virtual idVec3				AreaCenter( int areaNum ) const = 0;
								// Returns the area flags.
	virtual int					AreaFlags( int areaNum ) const = 0;
								// Returns the travel flags for traveling through the area.
	virtual int					AreaTravelFlags( int areaNum ) const = 0;
								// Trace through the areas and report the first collision.
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const = 0;
								// Get a plane for a trace.
	virtual const idPlane &		GetPlane( int planeNum ) const = 0;
								// Get wall edges.
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, int *edges, int maxEdges ) const = 0;
								// Sort the wall edges to create continuous sequences of walls.
	virtual void				SortWallEdges( int *edges, int numEdges ) const = 0;
								// Get the vertex numbers for an edge.
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const = 0;
								// Get an edge.
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const = 0;
								// Find all areas within or touching the bounds with the given contents and disable/enable them for routing.
	virtual bool				SetAreaState( const idBounds &bounds, const int areaContents, bool disabled ) = 0;

	// angua: disable / enable a specific area
	virtual void				DisableArea( int areanum ) = 0;
	virtual void				EnableArea( int areanum ) = 0;

								// Add an obstacle to the routing system.
	virtual aasHandle_t			AddObstacle( const idBounds &bounds ) = 0;
								// Remove an obstacle from the routing system.
	virtual void				RemoveObstacle( const aasHandle_t handle ) = 0;
								// Remove all obstacles from the routing system.
	virtual void				RemoveAllObstacles( void ) = 0;
								// Returns the travel time towards the goal area in 100th of a second.
	virtual int					TravelTimeToGoalArea( int areaNum, const idVec3 &origin, int goalAreaNum, int travelFlags, idActor* actor ) const = 0;

	/**
	 * greebo: Tries to set up a reachability between <areaNum/origin> and <goalAreaNum>. Uses the local routingcache.
	 *
	 * @areaNum/origin: the starting position and AAS area number.
	 * @goalAreaNum: The AAS area number of the destination
	 * @travelFlags: The allowed travelflags (used to determine which routing cache should be used).
	 * 
	 * @travelTime: Will hold the total traveltime of the best reachability or 0 if no route is found.
	 * @reach: A reference to a idReachability* pointer. The pointer is set to NULL if no route is found, otherwise it contains the best reachability.
	 * @door: if there is a door in the path, this stores a pointer to in (NULL otherwise)
	 * @actor: the calling actor (optional). Is used to determine whether walk paths are valid (through locked doors, for instance).
	 *
	 * @returns TRUE if a route is available, FALSE otherwise. 
	 *
	 * Note: A route is usually available if 
	 *       a) the goal area and the starting area are in the same cluster (no portals in between)
	 *       b) the clusters are connected via a portal.
	 */
	// Get the travel time and first reachability to be used towards the goal, returns true if there is a path.
	virtual bool				RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach, CFrobDoor** firstDoor, idActor* actor ) const = 0;

	/**
	 * greebo: Tries to set up a walk path from areaNum/origin to goalAreaNum/goalOrigin for the given travel flags.
	 *
	 * @path: The path structure which will contain the resulting path information. Will contain the starting point if no path is found.
	 * @areaNum/origin: the AAS area number and origin of the starting point.
	 * @goalAreaNum/goalOrigin: the AAS area number and origin of the destination.
	 * @travelFlags: the allowed travelflags (e.g. TFL_WALK|TFL_DOOR)
	 * @actor: The calling actor (optional), this is used to identify locked doors, which the actor is aware of.
	 *
	 * @returns: TRUE if a walk path could be found, FALSE otherwise.
	 */
	// Creates a walk path towards the goal.
	virtual bool				WalkPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int &travelTime, idActor* actor ) = 0; // grayman #3548

								/** 
								 * Returns true if one can walk along a straight line from the origin to the goal origin.
								 * angua: actor is used to handle AI-specific pathing, such as forbidden areas (e.g. locked doors)
								 * actor can be NULL
								 */
	virtual bool				WalkPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum, idActor* actor ) const = 0;
								// Creates a fly path towards the goal.
	virtual bool				FlyPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idActor* actor ) const = 0; // grayman #4412
								// Returns true if one can fly along a straight line from the origin to the goal origin.
	virtual bool				FlyPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const = 0;
								// Show the walk path from the origin towards the area.
	virtual void				ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) = 0;
								// Show the fly path from the origin towards the area.
	virtual void				ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const = 0; // grayman #4412
								// Find the nearest goal which satisfies the callback.
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback, unsigned short maxDistance=0 ) const = 0;

								// Find the goal cloest to the target which satisfies the callback.
	virtual bool				FindGoalClosestToTarget( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const = 0;

								// Added for DarkMod by SophisticatedZombie(DMH)
	virtual idBounds			GetAreaBounds (int areaNum) const = 0;
	virtual int					GetNumAreas() const = 0;
	virtual idReachability*		GetAreaFirstReachability(int areaNum) const = 0;

	virtual void				SetAreaTravelFlag( int index, int flag ) = 0;
	virtual void				RemoveAreaTravelFlag( int index, int flag ) = 0;

	virtual void				ReferenceDoor(CFrobDoor* door, int areaNum) = 0;
	virtual void				DeReferenceDoor(CFrobDoor* door, int areaNum) = 0;

	virtual CFrobDoor*			GetDoor(int areaNum) const = 0;

	/**
	* This function fills a reachability list
	* given an aas and the bounds for which any
	* intersecting reachabilities should be considered
	* impacted.
	*
	* @param inout_reachbilityList The list which will be
	*	initialized to only the idReachability pointers
	*	which are intersected by the impactBounds parameter
	*
	* @param impactBounds The bounds we check against
	*	reachabilities for intersection to determine impact.
	*
	* @return true on success
	* 
	* @return false on failure
	*/
	virtual bool BuildReachabilityImpactList
	(
		TReachabilityTrackingList& inout_reachabilityList,
		idBounds impactBounds
	) const = 0;

	/**
	* This method tests if a reachability is cut off from all
	* other reachabilities in the same area by the bounds
	* given. Note that iff there are no other reachabilities on 
	* then the reachability is NOT considered isolated.
	* 
	* @param p_reachability The reachability that we are testing
	*		to see if the given barrierBounds isolate it from any
	*		other reachabilities on the same area. 
	*
	* @param areaIndex The index of the area to which the reachability belongs
	*
	* @param barrierBounds The bounds of the barrier we are considering
	*
	* @return true if the reachability is isolated by the given bounds
	*	from all other reachbailities on the same area 
	*
	* @return false otherwise
	*/
	virtual bool TestIfBarrierIsolatesReachability
	(
		idReachability* p_reachability,
		int areaIndex,
		idBounds barrierBounds
	) const = 0;

	// Accessor function for the EAS
	virtual eas::tdmEAS* GetEAS() = 0;

	// Save/Restore routines
	virtual void Save(idSaveGame* savefile) const = 0;
	virtual void Restore(idRestoreGame* savefile) = 0;
};

#endif /* !__AAS_H__ */
