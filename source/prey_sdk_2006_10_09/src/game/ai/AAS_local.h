// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __AAS_LOCAL_H__
#define __AAS_LOCAL_H__

#include "AAS.h"
#include "../Pvs.h"


class idRoutingCache {
	friend class idAASLocal;

public:
								idRoutingCache( int size );
								~idRoutingCache( void );

	int							Size( void ) const;

private:
	int							type;					// portal or area cache
	int							size;					// size of cache
	int							cluster;				// cluster of the cache
	int							areaNum;				// area of the cache
	int							travelFlags;			// combinations of the travel flags
	idRoutingCache *			next;					// next in list
	idRoutingCache *			prev;					// previous in list
	idRoutingCache *			time_next;				// next in time based list
	idRoutingCache *			time_prev;				// previous in time based list
	unsigned short				startTravelTime;		// travel time to start with
	unsigned char *				reachabilities;			// reachabilities used for routing
	unsigned short *			travelTimes;			// travel time for every area
};


class idRoutingUpdate {
	friend class idAASLocal;

private:
	int							cluster;				// cluster number of this update
	int							areaNum;				// area number of this update
	unsigned short				tmpTravelTime;			// temporary travel time
	unsigned short *			areaTravelTimes;		// travel times within the area
	idVec3						start;					// start point into area
	idRoutingUpdate *			next;					// next in list
	idRoutingUpdate *			prev;					// prev in list
	bool						isInList;				// true if the update is in the list
};


class idRoutingObstacle {
	friend class idAASLocal;
								idRoutingObstacle( void ) { }

private:
	idBounds					bounds;					// obstacle bounds
	idList<int>					areas;					// areas the bounds are in
};

//HUMANHEAD nla 
// Helper class/data class for the near point functions
class hhNearPoint {

public:
	hhNearPoint(const idVec3 &anAllyOrigin, const idVec3 &anOurOrigin, float desiredDist) :
		allyOrigin(anAllyOrigin), ourOrigin(anOurOrigin),
		desiredDistSq(desiredDist * desiredDist),
		bestBlocked(true), bestDot(0), bestPoint(vec3_origin) { 
		
		direction = anOurOrigin - anAllyOrigin;
	}

	idVec3		getBestPoint(void) { return(bestPoint); }

	// These really should be protected with accessors, but for the sake
	//	  of speed, will leave as public access
public:
	idVec3		allyOrigin;		// Point where ally is
	idVec3		ourOrigin;      // Point where we are
	idVec3		direction;		// Vector from ally to follower
	float		desiredDistSq;	// Min distance to be, squared
	float		bestDistSq;		// Current best point distance squared
	float		bestBlocked;    // Is the best so far blocked?
	float		bestDot;		// Current best point * direction
	idVec3		bestPoint;		// Current best point

};


//HUMANHEAD nla
// Typedefs for return codes
typedef enum {
	AREA_NO_VALID_POINTS,		// No valid points were found
	AREA_MIXED,					// Points were a mixture
	AREA_ALL_DESIRED			// All points were desired points
} findAreaType_t;

typedef enum {
	POINT_NOT_VALID = 0,		// Point was on wrong side of origin
	POINT_VALID = 1,			// Point was within desired distance
	POINT_DESIRED = 2			// Point was outside desired distance
} findPointType_t;

//
// HUMANHEAD jrm
class hhPathApproach
{
public:
	hhPathApproach() {totalPathDistSqr = 0.0f; totalApproachDistSqr = 0.0f; minDistSqr = 0.0f; maxDistSqr = 0.0f;}
	float   totalPathDistSqr;		// Total path dist sqr
	float	totalApproachDistSqr;	// The total distance squared spent approaching the target 
	float   minDistSqr, maxDistSqr;	// The closest and farthest we got from the target
};


class idAASLocal : public idAAS {
public:
								idAASLocal( void );
	virtual						~idAASLocal( void );
	virtual bool				Init( const idStr &mapName, unsigned int mapFileCRC );
	virtual void				Shutdown( void );
	virtual void				Stats( void ) const;
	virtual void				Test( const idVec3 &origin );
	virtual const idAASSettings *GetSettings( void ) const;
	virtual int					PointAreaNum( const idVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const;
	virtual idVec3				AreaCenter( int areaNum ) const;
	virtual int					AreaFlags( int areaNum ) const;
	virtual int					AreaTravelFlags( int areaNum ) const;
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const;
	virtual const idPlane &		GetPlane( int planeNum ) const;
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, int *edges, int maxEdges ) const;
	virtual void				SortWallEdges( int *edges, int numEdges ) const;
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const;
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const;
	virtual bool				SetAreaState( const idBounds &bounds, const int areaContents, bool disabled );
	virtual aasHandle_t			AddObstacle( const idBounds &bounds );
	virtual void				RemoveObstacle( const aasHandle_t handle );
	virtual void				RemoveAllObstacles( void );
	virtual int					TravelTimeToGoalArea( int areaNum, const idVec3 &origin, int goalAreaNum, int travelFlags ) const;
	virtual bool				RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach ) const;
	virtual bool				WalkPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				WalkPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const;
	virtual bool				FlyPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				FlyPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const;
	virtual void				ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	virtual void				ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const;
	const char *				GetName( void ) const { return file ? file->GetName() : NULL; }

private:
	idAASFile *					file;
	idStr						name;

private:	// routing data
	idRoutingCache ***			areaCacheIndex;			// for each area in each cluster the travel times to all other areas in the cluster
	int							areaCacheIndexSize;		// number of area cache entries
	idRoutingCache **			portalCacheIndex;		// for each area in the world the travel times from each portal
	int							portalCacheIndexSize;	// number of portal cache entries
	idRoutingUpdate *			areaUpdate;				// memory used to update the area routing cache
	idRoutingUpdate *			portalUpdate;			// memory used to update the portal routing cache
	unsigned short *			goalAreaTravelTimes;	// travel times to goal areas
	unsigned short *			areaTravelTimes;		// travel times through the areas
	int							numAreaTravelTimes;		// number of area travel times
	mutable idRoutingCache *	cacheListStart;			// start of list with cache sorted from oldest to newest
	mutable idRoutingCache *	cacheListEnd;			// end of list with cache sorted from oldest to newest
	mutable int					totalCacheMemory;		// total cache memory used
	idList<idRoutingObstacle *>	obstacleList;			// list with obstacles

private:	// routing
	bool						SetupRouting( void );
	void						ShutdownRouting( void );
	unsigned short				AreaTravelTime( int areaNum, const idVec3 &start, const idVec3 &end ) const;
	void						CalculateAreaTravelTimes( void );
	void						DeleteAreaTravelTimes( void );
	void						SetupRoutingCache( void );
	void						DeleteClusterCache( int clusterNum );
	void						DeletePortalCache( void );
	void						ShutdownRoutingCache( void );
	void						RoutingStats( void ) const;
	void						LinkCache( idRoutingCache *cache ) const;
	void						UnlinkCache( idRoutingCache *cache ) const;
	void						DeleteOldestCache( void ) const;
	idReachability *			GetAreaReachability( int areaNum, int reachabilityNum ) const;
	int							ClusterAreaNum( int clusterNum, int areaNum ) const;
	void						UpdateAreaRoutingCache( idRoutingCache *areaCache ) const;
	idRoutingCache *			GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						UpdatePortalRoutingCache( idRoutingCache *portalCache ) const;
	idRoutingCache *			GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						RemoveRoutingCacheUsingArea( int areaNum );
	void						DisableArea( int areaNum );
	void						EnableArea( int areaNum );
	bool						SetAreaState_r( int nodeNum, const idBounds &bounds, const int areaContents, bool disabled );
	void						GetBoundsAreas_r( int nodeNum, const idBounds &bounds, idList<int> &areas ) const;
	void						SetObstacleState( const idRoutingObstacle *obstacle, bool enable );

private:	// pathing
	bool						EdgeSplitPoint( idVec3 &split, int edgeNum, const idPlane &plane ) const;
	bool						FloorEdgeSplitPoint( idVec3 &split, int areaNum, const idPlane &splitPlane, const idPlane &frontPlane, bool closest ) const;
	idVec3						SubSampleWalkPath( int areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, int &endAreaNum ) const;
	idVec3						SubSampleFlyPath( int areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, int &endAreaNum ) const;

private:	// debug
	const idBounds &			DefaultSearchBounds( void ) const;
	void						DrawCone( const idVec3 &origin, const idVec3 &dir, float radius, const idVec4 &color ) const;
	void						DrawArea( int areaNum ) const;
	// HUMANHEAD nla
	void						DrawBounds( int areaNum ) const;
	void						DrawBoundsEdge( const idVec3 &p0, const idVec3 &ip1, int keep, int draw ) const;
	// HUMANHEAD nla - Added color parameter
	void						DrawFace( int faceNum, bool side, idVec4 *color = &colorRed ) const;
	void						DrawEdge( int edgeNum, bool arrow, idVec4 *color = &colorRed ) const;
	void						DrawReachability( const idReachability *reach ) const;
	void						ShowArea( const idVec3 &origin ) const;
	void						ShowWallEdges( const idVec3 &origin ) const;
	void						ShowHideArea( const idVec3 &origin, int targerAreaNum ) const;
	bool						PullPlayer( const idVec3 &origin, int toAreaNum ) const;
	void						RandomPullPlayer( const idVec3 &origin ) const;
	void						ShowPushIntoArea( const idVec3 &origin ) const;
};

#endif /* !__AAS_LOCAL_H__ */
