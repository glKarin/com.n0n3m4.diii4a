// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __AAS_LOCAL_H__
#define __AAS_LOCAL_H__

#include "AAS.h"

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
	idRoutingObstacle( void ) { travelFlags = TFL_INVALID_GDF | TFL_INVALID_STROGG; }

private:
	int							travelFlags;
	idBounds					bounds;					// obstacle bounds
	idList<int>					areas;					// areas the bounds are in
};


class idAASLocal : public idAAS {
public:
								idAASLocal( void );
	virtual						~idAASLocal( void );

	virtual bool				Init( const char *mapName, unsigned int mapFileCRC );
	virtual void				Shutdown( void );
	virtual void				Stats( void ) const;
	virtual void				Test( const idVec3 &origin );
	virtual const idAASSettings *GetSettings( void ) const;
	virtual int					PointAreaNum( const idVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, int excludeTravelFlags ) const;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags, int excludeTravelFlags ) const;
	virtual void				PushPointIntoArea( int areaNum, idVec3 &origin ) const;
	virtual idVec3				AreaCenter( int areaNum ) const;
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const;
	virtual bool				TraceHeight( aasTraceHeight_t &trace, const idVec3 &start, const idVec3 &end ) const;
	virtual bool				TraceFloor( aasTraceFloor_t &trace, const idVec3 &start, int startAreaNum, const idVec3 &end, int travelFlags ) const;
	virtual const byte *		GetObstaclePVS( int areaNum ) const;
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, float height, int *edges, int maxEdges ) const;
	virtual int					GetObstaclePVSWallEdges( int areaNum, int *edges, int maxEdges ) const;
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const;
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const;
	virtual int					GetEdgeFlags( int areaNum ) const;
	virtual int					GetAreaFlags( int areaNum ) const;
	virtual void				SetAreaTravelFlags( int areaNum, int travelFlags );
	virtual bool				ChangeAreaTravelFlags( const idBounds &bounds, const int areaFlags, int travelFlags, bool set );
	virtual bool				ChangeReachabilityTravelFlags( const char *name, int travelFlags, bool set );
	virtual int					TravelTimeToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags ) const;
	virtual bool				RouteToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags, int &travelTime, const aasReachability_t **reach ) const;
	virtual bool				WalkPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const;
	virtual bool				ExtendHopPathToGoal( idAASPath &path, int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const;
	virtual bool				FindNearestGoal( idAASGoal &goal, int startAreaNum, const idVec3 &startOrigin, int travelFlags, idAASCallback &callback ) const;
	virtual void				ShowWalkPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags ) const;
	virtual void				ShowHopPath( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int walkTravelFlags, const idAASHopPathParms &parms ) const;
	virtual void				DrawArea( int areaNum ) const;

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

	float						groundSpeedMultiplier;
	float						waterSpeedMultiplier;

	int							numObstaclePVSBytes;
	mutable byte *				obstaclePVS;
	mutable int					obstaclePVSAreaNum;

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
	aasReachability_t *			GetAreaReachability( int areaNum, int reachabilityNum ) const;
	int							ClusterAreaNum( int clusterNum, int areaNum ) const;
	void						UpdateAreaRoutingCache( idRoutingCache *areaCache ) const;
	idRoutingCache *			GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						UpdatePortalRoutingCache( idRoutingCache *portalCache ) const;
	idRoutingCache *			GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	bool						GetClusterRoute( int startAreaNum, const idVec3 &startOrigin, int startClusterNum, int goalAreaNum, int travelFlags, int &travelTime, const aasReachability_t **reach ) const;
	void						RemoveRoutingCacheUsingArea( int areaNum );
	void						RemoveAreaTravelFlags( int areaNum, int travelFlags );
	bool						ChangeAreaTravelFlags_r( int nodeNum, const idBounds &bounds, const int areaFlags, int travelFlags, bool set );

private:	// pathing
	bool						FloorEdgeSplitPoint( idVec3 &split, int areaNum, const idPlane &splitPlane, const idPlane &frontPlane, bool closest ) const;
	void						SubSampleWalkPath( int startAreaNum, const idVec3 &startOrigin, int pathAreaNum, const idVec3 &pathStart, const idVec3 &pathEnd, int travelFlags, idVec3 &endPos, int &endAreaNum ) const;
	bool						WalkPathIsValid( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int &endAreaNum ) const;
	float						HorizontalDistanceSquare( const idVec3 &start, const idVec3 &end ) const;
	bool						HopPathIsValid( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin, const idAASHopPathParms &parms ) const;
	void						SortWallEdges( int *edges, int numEdges ) const;
	bool						EdgesIntersect2D( const int edge1, const int edge2, const idBounds &b1, const idBounds &b2, const float height ) const;
	void						SetupObstaclePVS();
	void						ShutdownObstaclePVS();

private:	// debug
	const idBounds &			DefaultSearchBounds( void ) const;
	void						DrawCone( const idVec3 &origin, const idVec3 &dir, float radius, const idVec4 &color ) const;
	void						DrawEdge( int edgeNum, bool arrow ) const;
	void						DrawReachability( const aasReachability_t *reach, const char *name ) const;
	int							TravelFlagForTeam( void ) const;
	int							TravelFlagWalkForTeam( void ) const;
	int							TravelFlagInvalidForTeam( void ) const;
	void						ShowArea( const idVec3 &origin, int mode ) const;
	void						ShowWalkPath( const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	void						ShowHopPath( const idVec3 &startOrigin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	void						ShowWallEdges( const idVec3 &origin, int mode, bool showNumbers ) const;
	void						ShowNearestCoverArea( const idVec3 &origin, int targerAreaNum ) const;
	void						ShowNearestInsideArea( const idVec3 &origin ) const;
	bool						PullPlayer( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int &startAreaNum, int &travelTime ) const;
	void						UnFreezePlayer() const;
	void						RandomPullPlayer( const idVec3 &origin, int mode ) const;
	void						ShowPushIntoArea( const idVec3 &origin ) const;
	void						ShowFloorTrace( const idVec3 &origin ) const;
	void						ShowObstaclePVS( const int areaNum ) const;
	void						ShowManualReachabilities() const;
	void						ShowAASObstacles() const;
	void						ShowAASBadAreas( int mode ) const;
	bool						GetAreaNumAndLocation( idCVar &cvar, const idVec3 &origin, int &areaNum, idVec3 &location ) const;
};

#endif /* !__AAS_LOCAL_H__ */
