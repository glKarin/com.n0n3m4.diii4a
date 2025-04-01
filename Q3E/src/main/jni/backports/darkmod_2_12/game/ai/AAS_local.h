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

#ifndef __AAS_LOCAL_H__
#define __AAS_LOCAL_H__

#include "AAS.h"
#include "../Pvs.h"
#include "EAS/EAS.h"
#include <map>

class CFrobDoor;

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
								idRoutingObstacle( void ) = default;

private:
	idBounds					bounds;					// obstacle bounds
	idList<int>					areas;					// areas the bounds are in
};


class CMultiStateMover;
namespace eas { class tdmEAS; }

class idAASLocal : 
	public idAAS
{
	friend class eas::tdmEAS; // TDM's EAS is our friend

public:
								idAASLocal( void );
	virtual						~idAASLocal( void ) override;
	virtual bool				Init( const idStr &mapName, const unsigned int mapFileCRC ) override;
	void						Shutdown( void );

	virtual void				Stats( void ) const override;
	virtual void				Test( const idVec3 &origin ) override;
	virtual const idAASSettings *GetSettings( void ) const override;
	virtual int					PointAreaNum( const idVec3 &origin ) const override;
	virtual int					PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const override;
	virtual int					BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const override;
	virtual void				PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const override;
	virtual idVec3				AreaCenter( int areaNum ) const override;
	virtual int					AreaFlags( int areaNum ) const override;
	virtual int					AreaTravelFlags( int areaNum ) const override;
	virtual bool				Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const override;
	virtual const idPlane &		GetPlane( int planeNum ) const override;
	virtual int					GetWallEdges( int areaNum, const idBounds &bounds, int travelFlags, int *edges, int maxEdges ) const override;
	virtual void				SortWallEdges( int *edges, int numEdges ) const override;
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const override;
	virtual void				GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const override;
	virtual bool				SetAreaState( const idBounds &bounds, const int areaContents, bool disabled ) override;
	virtual aasHandle_t			AddObstacle( const idBounds &bounds ) override;
	virtual void				RemoveObstacle( const aasHandle_t handle ) override;
	virtual void				RemoveAllObstacles( void ) override;
	virtual int					TravelTimeToGoalArea( int areaNum, const idVec3 &origin, int goalAreaNum, int travelFlags, idActor* actor ) const override;
	virtual bool				RouteToGoalArea( int areaNum, const idVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach, CFrobDoor** firstDoor, idActor* actor ) const override;
	virtual bool				WalkPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, int &travelTime, idActor* actor ) override; // grayman #3548
	virtual bool				WalkPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum, idActor* actor) const override;
	virtual bool				FlyPathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idActor* actor ) const override; // grayman #4412
//	virtual void				PrintReachability(int index,idReachability *reach) const override;
	virtual bool				FlyPathValid( int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idVec3 &endPos, int &endAreaNum ) const override;
	virtual void				ShowWalkPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) override;
	virtual void				ShowFlyPath( const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const override; // grayman #4412
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback, unsigned short maxTravelCost=0 ) const override;
	virtual bool				FindGoalClosestToTarget( aasGoal_t &goal, int areaNum, const idVec3 origin, const idVec3 &target, int travelFlags, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const override;

	// Added for DarkMod by SophisticatedZombie(DMH)
	virtual idBounds			GetAreaBounds (int areaNum) const override;
	virtual int					GetNumAreas() const override;
	virtual idReachability*		GetAreaFirstReachability(int areaNum) const override;

	virtual void				SetAreaTravelFlag( int index, int flag ) override;

	virtual void				RemoveAreaTravelFlag( int index, int flag ) override;

	// angua: this returns the cluster number of this area
	int							GetClusterNum(int areaNum);

	virtual void				ReferenceDoor(CFrobDoor* door, int areaNum) override;
	virtual void				DeReferenceDoor(CFrobDoor* door, int areaNum) override;

	virtual CFrobDoor*			GetDoor(int areaNum) const override;

	/*!
	* See base class for interface definition
	*/
	virtual bool BuildReachabilityImpactList
	(
		TReachabilityTrackingList& inout_reachabilityList,
		idBounds impactBounds
	) const override;

	/*!
	* See base class for interface definition
	*/
	virtual bool TestIfBarrierIsolatesReachability
	(
		idReachability* p_reachability,
		int areaIndex,
		idBounds barrierBounds
	) const override;

	/**
	 * greebo: Adds the given elevator to this AAS class. This will add
	 * additional routing possibilities for AI between clusters.
	 */
	void AddElevator(CMultiStateMover* mover);

	/**
	 * grayman #3763 - retrieve cluster size
	 */
	int GetClusterSize();

	/**
	 * grayman #3857 - retrieve a list of portals around areaNum
	 */
	void GetPortals(int areaNum, idList<idVec4> &portalList, idBounds searchLimits, idAI* ai); // grayman #4238

	/**
	 * greebo: Assembles the elevator routing information.
	 */
	void CompileEAS();

	// Accessor function for the EAS
	virtual eas::tdmEAS* GetEAS() override { return elevatorSystem; }

	// Save/Restore routines
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

private:
	idAASFile *					file;
	idStr						name;

	typedef std::map<int, CFrobDoor*> DoorMap;
	DoorMap _doors;


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

	// greebo: This is TDM's EAS "Elevator Awareness System" :)
	eas::tdmEAS*				elevatorSystem;

	idList<idVec4>				aasColors;				// grayman #3032 - colors of AAS areas for debugging - no need to save/restore

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

public:
	virtual void				DisableArea( int areaNum ) override;
	virtual void				EnableArea( int areaNum ) override;

private:
	bool						SetAreaState_r( int nodeNum, const idBounds &bounds, const int areaContents, bool disabled );
	void						GetBoundsAreas_r( int nodeNum, const idBounds &bounds, idList<int> &areas ) const;
	void						SetObstacleState( const idRoutingObstacle *obstacle, bool enable );

	// returns an area within that cluster (SLOW!), returns -1 if none found
	int							GetAreaInCluster(int clusterNum);

private:	// pathing
	bool						EdgeSplitPoint( idVec3 &split, int edgeNum, const idPlane &plane ) const;
	bool						FloorEdgeSplitPoint( idVec3 &split, int areaNum, const idPlane &splitPlane, const idPlane &frontPlane, bool closest ) const;
	idVec3						SubSampleWalkPath( int areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, int &endAreaNum, idActor* actor );
	idVec3						SubSampleFlyPath( int areaNum, const idVec3 &origin, const idVec3 &start, const idVec3 &end, int travelFlags, int &endAreaNum ) const;

public:	// debug
	const idBounds &			DefaultSearchBounds( void ) const;
	void						DrawCone( const idVec3 &origin, const idVec3 &dir, float radius, const idVec4 &color ) const;
	void						DrawArea( int areaNum ) const;
	void						DrawFace( int faceNum, bool side ) const;
	void						DrawEdge( int edgeNum, bool arrow ) const;
	void						DrawReachability( const idReachability *reach ) const;
	void						ShowArea( const idVec3 &origin ) const;
	void						ShowWallEdges( const idVec3 &origin ) const;
	void						ShowHideArea( const idVec3 &origin, int targerAreaNum );
	bool						PullPlayer( const idVec3 &origin, int toAreaNum );
	void						RandomPullPlayer( const idVec3 &origin );
	void						ShowPushIntoArea( const idVec3 &origin ) const;
	void						DrawAreas( const idVec3& playerOrigin );
	void						DrawEASRoute( const idVec3& playerOrigin, int goalArea );
};

#endif /* !__AAS_LOCAL_H__ */
