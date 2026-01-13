
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

class idAASLocal : public idAAS {
public:
								idAASLocal( void );
	virtual						~idAASLocal( void );
	virtual bool				Init( const idStr &mapName, unsigned int mapFileCRC );
	virtual void				Shutdown( void );
// RAVEN BEGIN
// jscott: added summary flag
	virtual size_t				StatsSummary( void ) const;
// RAVEN END
	virtual void				Stats( void ) const;
	virtual void				Test( const idVec3 &origin );
	virtual const idAASSettings *GetSettings( void ) const;
	virtual int					PointAreaNum( const idVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const;
	virtual idVec3				AreaCenter( int areaNum ) const;
// RAVEN BEGIN
// bdube: added
	virtual float				AreaRadius( int areaNum ) const;
	virtual idBounds &			AreaBounds( int areaNum ) const;
	virtual float				AreaCeiling( int areaNum ) const;
// RAVEN END
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
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, float minDistance, float maxDistance, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const;
// RAVEN BEGIN 
// creed: Added Area Wall Extraction For AASTactical
	virtual idAASFile*			GetFile( void ) {return file;}
// cdr: Alternate Routes Bug
	 virtual void				SetReachabilityState( idReachability* reach, bool enable );
// rjohnson: added more debug drawing
	virtual bool				IsValid( void ) const;
	virtual void				ShowAreas( const idVec3 &origin, bool ShowProblemAreas = false ) const;
// RAVEN END


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
	void						DrawAreaBounds( int areaNum ) const;
	void						DrawArea( int areaNum ) const;
	void						DrawFace( int faceNum, bool side ) const;
	void						DrawEdge( int edgeNum, bool arrow ) const;
	void						DrawReachability( const idReachability *reach ) const;
	void						ShowArea( const idVec3 &origin ) const;
	void						ShowWallEdges( const idVec3 &origin ) const;
	void						ShowHideArea( const idVec3 &origin, int targerAreaNum ) const;
	bool						PullPlayer( const idVec3 &origin, int toAreaNum ) const;
	void						RandomPullPlayer( const idVec3 &origin ) const;
	void						ShowPushIntoArea( const idVec3 &origin ) const;

// RAVEN BEGIN 
// rjohnson: added more debug drawing
	void						DrawSimpleEdge( int edgeNum ) const;
	void						DrawSimpleFace( int faceNum, bool visited ) const;
	void						DrawSimpleArea( int areaNum ) const;
	void						ShowProblemEdge( int edgeNum ) const;
	void						ShowProblemFace( int faceNum ) const;
	void						ShowProblemArea( int areaNum ) const;
	void						ShowProblemArea( const idVec3 &origin ) const;
// RAVEN END
};

#endif /* !__AAS_LOCAL_H__ */
