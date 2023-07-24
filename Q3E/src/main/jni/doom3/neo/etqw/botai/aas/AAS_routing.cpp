// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

#include "AAS_local.h"

#include "../../../libs/AASLib/AASFile.h"

#include "../BotThread.h"
#include "../BotThreadData.h"

#define CACHETYPE_AREA				1
#define CACHETYPE_PORTAL			2

#define MAX_ROUTING_CACHE_MEMORY	(2*1024*1024)

#define LEDGE_TRAVELTIME_PANALTY	250

#ifdef MACOS_X
#include <unistd.h>
#endif

/*
============
idRoutingCache::idRoutingCache
============
*/
idRoutingCache::idRoutingCache( int size ) {
	areaNum = 0;
	cluster = 0;
	next = prev = NULL;
	time_next = time_prev = NULL;
	travelFlags = 0;
	startTravelTime = 0;
	type = 0;
	this->size = size;
	reachabilities = new byte[size];
	memset( reachabilities, 0, size * sizeof( reachabilities[0] ) );
	travelTimes = new unsigned short[size];
	memset( travelTimes, 0, size * sizeof( travelTimes[0] ) );
}

/*
============
idRoutingCache::~idRoutingCache
============
*/
idRoutingCache::~idRoutingCache( void ) {
	delete [] reachabilities;
	delete [] travelTimes;
}

/*
============
idRoutingCache::Size
============
*/
int idRoutingCache::Size( void ) const {
	return sizeof( idRoutingCache ) + size * sizeof( reachabilities[0] ) + size * sizeof( travelTimes[0] );
}

/*
============
idAASLocal::AreaTravelTime
============
*/
unsigned short idAASLocal::AreaTravelTime( int areaNum, const idVec3 &start, const idVec3 &end ) const {
	float dist = ( end - start ).Length();

	if ( file->GetArea( areaNum ).travelFlags & TFL_WATER ) {
		dist *= waterSpeedMultiplier;
	} else {
		dist *= groundSpeedMultiplier;
	}
	if ( dist < 1.0f ) {
		return 1;
	}
	return (unsigned short) idMath::Ftoi( dist );
}

/*
============
idAASLocal::CalculateAreaTravelTimes
============
*/
void idAASLocal::CalculateAreaTravelTimes(void) {
	aasReachability_t *reach, *rev_reach;

	// get total memory for all area travel times
	numAreaTravelTimes = 0;
	for ( int n = 0; n < file->GetNumAreas(); n++ ) {

		if ( ( file->GetArea( n ).flags & AAS_AREA_REACHABLE_WALK ) == 0 ) {
			continue;
		}

		int numReach = 0;
		for ( aasReachability_t *reach = file->GetArea( n ).reach; reach; reach = reach->next ) {
			numReach++;
		}

		int numRevReach = 0;
		for ( aasReachability_t *rev_reach = file->GetArea( n ).rev_reach; rev_reach; rev_reach = rev_reach->rev_next ) {
			numRevReach++;
		}
		numAreaTravelTimes += numReach * numRevReach;
	}

	areaTravelTimes = (unsigned short *) Mem_Alloc( numAreaTravelTimes * sizeof( unsigned short ) );
	int offset = 0;

	for ( int n = 0; n < file->GetNumAreas(); n++ ) {

		if ( ( file->GetArea( n ).flags & AAS_AREA_REACHABLE_WALK ) == 0 ) {
			continue;
		}

		// for each reachability that starts in this area calculate the travel time
		// towards all the reachabilities that lead towards this area
		unsigned short maxt = 0;
		int i = 0;
		for ( reach = file->GetArea( n ).reach; reach != NULL; reach = reach->next, i++ ) {

			int j = 0;
			for ( rev_reach = file->GetArea( n ).rev_reach; rev_reach != NULL; rev_reach = rev_reach->rev_next, j++ ) {
				unsigned short t = AreaTravelTime( n, reach->GetStart(), rev_reach->GetEnd() );
				areaTravelTimes[ offset + j ] = t + 1;
				if ( t > maxt ) {
					maxt = t;
				}
			}

			// make sure the area travel time offset is in range
			assert( offset < ( 1 << AAS_REACH_NUMBER_SHIFT ) );
			if ( i >= ( 1 << AAS_REACH_NUMBER_SHIFT ) ) {
				gameLocal.Error( "more than %d area travel times", ( 1 << AAS_REACH_NUMBER_SHIFT ) );
			}
			// make sure the reachability number is in range
			assert( i < AAS_REACH_MAX_PER_AREA );
			if ( i >= AAS_REACH_MAX_PER_AREA ) {
				gameLocal.Error( "area %d has more than %d reachabilities", n, AAS_REACH_MAX_PER_AREA );
			}

			// store the area travel time offset
			reach->areaTTOfsAndNumber = offset | ( i << AAS_REACH_NUMBER_SHIFT );

			offset += j;
		}

		// if this area is a portal
		if ( file->GetArea( n ).cluster < 0 ) {
			// set the maximum travel time through this portal
			file->SetPortalMaxTravelTime( -file->GetArea( n ).cluster, maxt );
		}
	}

	assert( offset <= numAreaTravelTimes );
}

/*
============
idAASLocal::DeleteAreaTravelTimes
============
*/
void idAASLocal::DeleteAreaTravelTimes( void ) {
	Mem_Free( areaTravelTimes );
	areaTravelTimes = NULL;
	numAreaTravelTimes = 0;
}

/*
============
idAASLocal::SetupRoutingCache
============
*/
void idAASLocal::SetupRoutingCache( void ) {
	int i;
	byte *bytePtr;

	areaCacheIndexSize = 0;
	for ( i = 0; i < file->GetNumClusters(); i++ ) {
		areaCacheIndexSize += file->GetCluster( i ).numReachableAreas;
	}
	areaCacheIndex = (idRoutingCache ***) Mem_ClearedAlloc( file->GetNumClusters() * sizeof( idRoutingCache ** ) +
													areaCacheIndexSize * sizeof( idRoutingCache *) );
	bytePtr = ((byte *)areaCacheIndex) + file->GetNumClusters() * sizeof( idRoutingCache ** );
	for ( i = 0; i < file->GetNumClusters(); i++ ) {
		areaCacheIndex[i] = ( idRoutingCache ** ) bytePtr;
		bytePtr += file->GetCluster( i ).numReachableAreas * sizeof( idRoutingCache * );
	}

	portalCacheIndexSize = file->GetNumAreas();
	portalCacheIndex = (idRoutingCache **) Mem_ClearedAlloc( portalCacheIndexSize * sizeof( idRoutingCache * ) );

	areaUpdate = (idRoutingUpdate *) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( idRoutingUpdate ) );
	portalUpdate = (idRoutingUpdate *) Mem_ClearedAlloc( (file->GetNumPortals()+1) * sizeof( idRoutingUpdate ) );

	goalAreaTravelTimes = (unsigned short *) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( unsigned short ) );

	cacheListStart = cacheListEnd = NULL;
	totalCacheMemory = 0;
}

/*
============
idAASLocal::DeleteClusterCache
============
*/
void idAASLocal::DeleteClusterCache( int clusterNum ) {
	int i;
	idRoutingCache *cache;

	for ( i = 0; i < file->GetCluster( clusterNum ).numReachableAreas; i++ ) {
		for ( cache = areaCacheIndex[clusterNum][i]; cache; cache = areaCacheIndex[clusterNum][i] ) {
			areaCacheIndex[clusterNum][i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
idAASLocal::DeletePortalCache
============
*/
void idAASLocal::DeletePortalCache( void ) {
	int i;
	idRoutingCache *cache;

	for ( i = 0; i < file->GetNumAreas(); i++ ) {
		for ( cache = portalCacheIndex[i]; cache; cache = portalCacheIndex[i] ) {
			portalCacheIndex[i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
idAASLocal::ShutdownRoutingCache
============
*/
void idAASLocal::ShutdownRoutingCache( void ) {
	int i;

	for ( i = 0; i < file->GetNumClusters(); i++ ) {
		DeleteClusterCache( i );
	}

	DeletePortalCache();

	Mem_Free( areaCacheIndex );
	areaCacheIndex = NULL;
	areaCacheIndexSize = 0;
	Mem_Free( portalCacheIndex );
	portalCacheIndex = NULL;
	portalCacheIndexSize = 0;
	Mem_Free( areaUpdate );
	areaUpdate = NULL;
	Mem_Free( portalUpdate );
	portalUpdate = NULL;
	Mem_Free( goalAreaTravelTimes );
	goalAreaTravelTimes = NULL;

	cacheListStart = cacheListEnd = NULL;
	totalCacheMemory = 0;
}

/*
============
idAASLocal::SetupRouting
============
*/
bool idAASLocal::SetupRouting( void ) {

	groundSpeedMultiplier = 100.0f / file->GetSettings().groundSpeed;
	waterSpeedMultiplier = 100.0f / file->GetSettings().waterSpeed;

	CalculateAreaTravelTimes();

	SetupRoutingCache();

	return true;
}

/*
============
idAASLocal::ShutdownRouting
============
*/
void idAASLocal::ShutdownRouting( void ) {
	DeleteAreaTravelTimes();
	ShutdownRoutingCache();
}

/*
============
idAASLocal::RoutingStats
============
*/
void idAASLocal::RoutingStats( void ) const {
	idRoutingCache *cache;
	int numAreaCache, numPortalCache;
	int totalAreaCacheMemory, totalPortalCacheMemory;

	numAreaCache = numPortalCache = 0;
	totalAreaCacheMemory = totalPortalCacheMemory = 0;
	for ( cache = cacheListStart; cache; cache = cache->time_next ) {
		if ( cache->type == CACHETYPE_AREA ) {
			numAreaCache++;
			totalAreaCacheMemory += sizeof( idRoutingCache ) + cache->size * (sizeof( unsigned short ) + sizeof( byte ));
		} else {
			numPortalCache++;
			totalPortalCacheMemory += sizeof( idRoutingCache ) + cache->size * (sizeof( unsigned short ) + sizeof( byte ));
		}
	}

	gameLocal.Printf( "%6d area cache (%d kB)\n", numAreaCache, totalAreaCacheMemory >> 10 );
	gameLocal.Printf( "%6d portal cache (%d kB)\n", numPortalCache, totalPortalCacheMemory >> 10 );
	gameLocal.Printf( "%6d total cache (%d kB)\n", numAreaCache + numPortalCache, totalCacheMemory >> 10 );
	gameLocal.Printf( "%6d area travel times (%d kB)\n", numAreaTravelTimes, ( numAreaTravelTimes * sizeof( unsigned short ) ) >> 10 );
	gameLocal.Printf( "%6d area cache entries (%d kB)\n", areaCacheIndexSize, ( areaCacheIndexSize * sizeof( idRoutingCache * ) ) >> 10 );
	gameLocal.Printf( "%6d portal cache entries (%d kB)\n", portalCacheIndexSize, ( portalCacheIndexSize * sizeof( idRoutingCache * ) ) >> 10 );
}

/*
============
idAASLocal::RemoveRoutingCacheUsingArea
============
*/
void idAASLocal::RemoveRoutingCacheUsingArea( int areaNum ) {
	int clusterNum;

	clusterNum = file->GetArea( areaNum ).cluster;
	if ( clusterNum > 0 ) {
		// remove all the cache in the cluster the area is in
		DeleteClusterCache( clusterNum );
	} else {
		// if this is a portal remove all cache in both the front and back cluster
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[0] );
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[1] );
	}
	DeletePortalCache();
}

/*
============
idAASLocal::SetAreaTravelFlags
============
*/
void idAASLocal::SetAreaTravelFlags( int areaNum, int travelFlags ) {
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if ( ( file->GetArea( areaNum ).travelFlags & travelFlags ) != 0 ) {
		return;
	}

	file->SetAreaTravelFlag( areaNum, travelFlags );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
idAASLocal::RemoveAreaTravelFlags
============
*/
void idAASLocal::RemoveAreaTravelFlags( int areaNum, int travelFlags ) {
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if ( ( file->GetArea( areaNum ).travelFlags & travelFlags ) == 0 ) {
		return;
	}

	file->RemoveAreaTravelFlag( areaNum, travelFlags );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
idAASLocal::ChangeAreaTravelFlags_r
============
*/
bool idAASLocal::ChangeAreaTravelFlags_r( int nodeNum, const idBounds &bounds, const int areaFlags, int travelFlags, bool set ) {
	int res;
	const aasNode_t *node;
	bool foundArea = false;

	while( nodeNum != 0 ) {
		if ( nodeNum < 0 ) {
			// if this area is a cluster portal
			if ( file->GetArea( -nodeNum ).flags & areaFlags ) {
				if ( set ) {
					SetAreaTravelFlags( -nodeNum, travelFlags );
				} else {
					RemoveAreaTravelFlags( -nodeNum, travelFlags );
				}
				foundArea |= true;
			}
			break;
		}
		node = &file->GetNode( nodeNum );
		res = bounds.PlaneSide( file->GetPlane( node->planeNum ) );
		if ( res == PLANESIDE_BACK ) {
			nodeNum = node->children[1];
		} else if ( res == PLANESIDE_FRONT ) {
			nodeNum = node->children[0];
		} else {
			foundArea |= ChangeAreaTravelFlags_r( node->children[1], bounds, areaFlags, travelFlags, set );
			nodeNum = node->children[0];
		}
	}

	return foundArea;
}

/*
============
idAASLocal::ChangeAreaTravelFlags
============
*/
bool idAASLocal::ChangeAreaTravelFlags( const idBounds &bounds, const int areaFlags, int travelFlags, bool set ) {
	idBounds expBounds;

	if ( file == NULL ) {
		return false;
	}

	expBounds[0] = bounds[0] - file->GetSettings().boundingBox[1];
	expBounds[1] = bounds[1] - file->GetSettings().boundingBox[0];

	// find all areas within or touching the bounds with the given contents and disable/enable them for routing
	return ChangeAreaTravelFlags_r( 1, expBounds, areaFlags, travelFlags, set );
}

/*
============
idAASLocal::ChangeReachabilityTravelFlags
============
*/
bool idAASLocal::ChangeReachabilityTravelFlags( const char *name, int travelFlag, bool set ) {
	if ( file == NULL ) {
		return false;
	}

	int index = file->FindReachabilityByName( name );
	if ( index < 0 ) {
		return false;
	}

	if ( set ) {
		file->SetReachabilityTravelFlag( index, travelFlag );
	} else {
		file->RemoveReachabilityTravelFlag( index, travelFlag );
	}

	RemoveRoutingCacheUsingArea( file->GetReachability( index ).fromAreaNum );
	RemoveRoutingCacheUsingArea( file->GetReachability( index ).toAreaNum );

	return true;
}

/*
============
idAASLocal::LinkCache

  link the cache in the cache list sorted from oldest to newest cache
============
*/
void idAASLocal::LinkCache( idRoutingCache *cache ) const {

	// if the cache is already linked
	if ( cache->time_next != NULL || cache->time_prev != NULL || cacheListStart == cache ) {
		UnlinkCache( cache );
	}

	totalCacheMemory += cache->Size();

	// add cache to the end of the list
	cache->time_next = NULL;
	cache->time_prev = cacheListEnd;
	if ( cacheListEnd != NULL ) {
		cacheListEnd->time_next = cache;
	}
	cacheListEnd = cache;
	if ( cacheListStart == NULL ) {
		cacheListStart = cache;
	}
}

/*
============
idAASLocal::UnlinkCache
============
*/
void idAASLocal::UnlinkCache( idRoutingCache *cache ) const {

	totalCacheMemory -= cache->Size();

	// unlink the cache
	if ( cache->time_next != NULL ) {
		cache->time_next->time_prev = cache->time_prev;
	} else {
		cacheListEnd = cache->time_prev;
	}
	if ( cache->time_prev != NULL ) {
		cache->time_prev->time_next = cache->time_next;
	} else {
		cacheListStart = cache->time_next;
	}
	cache->time_next = cache->time_prev = NULL;
}

/*
============
idAASLocal::DeleteOldestCache
============
*/
void idAASLocal::DeleteOldestCache( void ) const {
	idRoutingCache *cache;

	assert( cacheListStart != NULL );

	// unlink the oldest cache
	cache = cacheListStart;
	UnlinkCache( cache );

	// unlink the oldest cache from the area or portal cache index
	if ( cache->next != NULL ) {
		cache->next->prev = cache->prev;
	}
	if ( cache->prev != NULL ) {
		cache->prev->next = cache->next;
	} else if ( cache->type == CACHETYPE_AREA ) {
		areaCacheIndex[cache->cluster][ClusterAreaNum( cache->cluster, cache->areaNum )] = cache->next;
	} else if ( cache->type == CACHETYPE_PORTAL ) {
		portalCacheIndex[cache->areaNum] = cache->next;
	}

	delete cache;
}

/*
============
idAASLocal::GetAreaReachability
============
*/
aasReachability_t *idAASLocal::GetAreaReachability( int areaNum, int reachabilityNum ) const {
	for ( aasReachability_t *reach = file->GetArea( areaNum ).reach; reach != NULL; reach = reach->next ) {
		if ( --reachabilityNum < 0 ) {
			return reach;
		}
	}
	return NULL;
}

/*
============
idAASLocal::ClusterAreaNum
============
*/
ID_INLINE int idAASLocal::ClusterAreaNum( int clusterNum, int areaNum ) const {
	int side, areaCluster;

	areaCluster = file->GetArea( areaNum ).cluster;
	if ( areaCluster > 0 ) {
		return file->GetArea( areaNum ).clusterAreaNum;
	} else {
		side = file->GetPortal( -areaCluster ).clusters[0] != clusterNum;
		return file->GetPortal( -areaCluster ).clusterAreaNum[side];
	}
}

/*
============
idAASLocal::UpdateAreaRoutingCache
============
*/
void idAASLocal::UpdateAreaRoutingCache( idRoutingCache *areaCache ) const {
	int i, nextAreaNum, cluster, badTravelFlags, clusterAreaNum, numReachableAreas;
	unsigned short t, startAreaTravelTimes[AAS_REACH_MAX_PER_AREA];
	idRoutingUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	aasReachability_t *reach;
	const aasArea_t *nextArea;

	// number of reachability areas within this cluster
	numReachableAreas = file->GetCluster( areaCache->cluster ).numReachableAreas;

	// number of the start area within the cluster
	clusterAreaNum = ClusterAreaNum( areaCache->cluster, areaCache->areaNum );
	if ( clusterAreaNum >= numReachableAreas ) {
		return;
	}

	areaCache->travelTimes[clusterAreaNum] = areaCache->startTravelTime;
	badTravelFlags = ~areaCache->travelFlags;
	memset( startAreaTravelTimes, 0, sizeof( startAreaTravelTimes ) );

	// initialize first update
	curUpdate = &areaUpdate[clusterAreaNum];
	curUpdate->areaNum = areaCache->areaNum;
	curUpdate->areaTravelTimes = startAreaTravelTimes;
	curUpdate->tmpTravelTime = areaCache->startTravelTime;
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the list
	while( updateListStart != NULL ) {

		curUpdate = updateListStart;
		if ( curUpdate->next != NULL ) {
			curUpdate->next->prev = NULL;
		} else {
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		for ( i = 0, reach = file->GetArea( curUpdate->areaNum ).rev_reach; reach; reach = reach->rev_next, i++ ) {

			// if the reachability uses an undesired travel type
			if ( reach->travelFlags & badTravelFlags ) {
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->fromAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if ( nextArea->travelFlags & badTravelFlags ) {
				continue;
			}

			// get the cluster number of the area
			cluster = nextArea->cluster;
			// don't leave the cluster, however do flood into cluster portals
			if ( cluster > 0 && cluster != areaCache->cluster ) {
				continue;
			}

			// get the number of the area in the cluster
			clusterAreaNum = ClusterAreaNum( areaCache->cluster, nextAreaNum );
			if ( clusterAreaNum >= numReachableAreas ) {
				continue;	// should never happen
			}

			assert( clusterAreaNum < areaCache->size );

			// time already travelled plus the traveltime through the current area
			// plus the travel time of the reachability towards the next area
			t = curUpdate->tmpTravelTime + curUpdate->areaTravelTimes[i] + reach->travelTime;

			if ( !areaCache->travelTimes[clusterAreaNum] || t < areaCache->travelTimes[clusterAreaNum] ) {

				areaCache->travelTimes[clusterAreaNum] = t;
				areaCache->reachabilities[clusterAreaNum] = ( reach->areaTTOfsAndNumber >> AAS_REACH_NUMBER_SHIFT ) & AAS_REACH_NUMBER_MASK;
				nextUpdate = &areaUpdate[clusterAreaNum];
				nextUpdate->areaNum = nextAreaNum;
				nextUpdate->tmpTravelTime = t;
				nextUpdate->areaTravelTimes = areaTravelTimes + ( reach->areaTTOfsAndNumber & AAS_AREA_TRAVEL_TIME_OFFSET_MASK );

				// avoid areas near ledges
				if ( file->GetArea( nextAreaNum ).flags & AAS_AREA_LEDGE ) {
					nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
				}

				if ( !nextUpdate->isInList ) {
					nextUpdate->next = NULL;
					nextUpdate->prev = updateListEnd;
					if ( updateListEnd != NULL ) {
						updateListEnd->next = nextUpdate;
					} else {
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
idAASLocal::GetAreaRoutingCache
============
*/
idRoutingCache *idAASLocal::GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const {
	int clusterAreaNum;
	idRoutingCache *cache, *clusterCache;

	// number of the area in the cluster
	clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
	// pointer to the cache for the area in the cluster
	clusterCache = areaCacheIndex[clusterNum][clusterAreaNum];
	// check if cache without undesired travel flags already exists
	for ( cache = clusterCache; cache; cache = cache->next ) {
		if ( cache->travelFlags == travelFlags ) {
			break;
		}
	}
	// if no cache found
	if ( cache == NULL ) {
		cache = new idRoutingCache( file->GetCluster( clusterNum ).numReachableAreas );
		cache->type = CACHETYPE_AREA;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = NULL;
		cache->next = clusterCache;
		if ( clusterCache ) {
			clusterCache->prev = cache;
		}
		areaCacheIndex[clusterNum][clusterAreaNum] = cache;
		UpdateAreaRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
idAASLocal::UpdatePortalRoutingCache
============
*/
void idAASLocal::UpdatePortalRoutingCache( idRoutingCache *portalCache ) const {
	int i, portalNum, clusterAreaNum;
	unsigned short t;
	const aasPortal_t *portal;
	const aasCluster_t *cluster;
	idRoutingCache *cache;
	idRoutingUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;

	curUpdate = &portalUpdate[ file->GetNumPortals() ];
	curUpdate->cluster = portalCache->cluster;
	curUpdate->areaNum = portalCache->areaNum;
	curUpdate->tmpTravelTime = portalCache->startTravelTime;

	//put the area to start with in the current read list
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the current list
	while( updateListStart != NULL ) {

		curUpdate = updateListStart;
		// remove the current update from the list
		if ( curUpdate->next != NULL ) {
			curUpdate->next->prev = NULL;
		} else {
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;
		// current update is removed from the list
		curUpdate->isInList = false;

		cluster = &file->GetCluster( curUpdate->cluster );
		cache = GetAreaRoutingCache( curUpdate->cluster, curUpdate->areaNum, portalCache->travelFlags );

		// take all portals of the cluster
		for ( i = 0; i < cluster->numPortals; i++ ) {
			portalNum = file->GetPortalIndex( cluster->firstPortal + i );
			assert( portalNum < portalCache->size );
			portal = &file->GetPortal( portalNum );

			clusterAreaNum = ClusterAreaNum( curUpdate->cluster, portal->areaNum );
			if ( clusterAreaNum >= cluster->numReachableAreas ) {
				continue;
			}

			t = cache->travelTimes[clusterAreaNum];
			if ( t == 0 ) {
				continue;
			}
			t += curUpdate->tmpTravelTime;

			if ( !portalCache->travelTimes[portalNum] || t < portalCache->travelTimes[portalNum] ) {

				portalCache->travelTimes[portalNum] = t;
				portalCache->reachabilities[portalNum] = cache->reachabilities[clusterAreaNum];
				nextUpdate = &portalUpdate[portalNum];
				if ( portal->clusters[0] == curUpdate->cluster ) {
					nextUpdate->cluster = portal->clusters[1];
				} else {
					nextUpdate->cluster = portal->clusters[0];
				}
				nextUpdate->areaNum = portal->areaNum;
				// add travel time through the actual portal area for the next update
				nextUpdate->tmpTravelTime = t + portal->maxAreaTravelTime;

				if ( !nextUpdate->isInList ) {

					nextUpdate->next = NULL;
					nextUpdate->prev = updateListEnd;
					if ( updateListEnd != NULL ) {
						updateListEnd->next = nextUpdate;
					} else {
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
idAASLocal::GetPortalRoutingCache
============
*/
idRoutingCache *idAASLocal::GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const {
	idRoutingCache *cache;

	// check if cache without undesired travel flags already exists
	for ( cache = portalCacheIndex[areaNum]; cache; cache = cache->next ) {
		if ( cache->travelFlags == travelFlags ) {
			break;
		}
	}
	// if no cache found
	if ( cache == NULL ) {
		cache = new idRoutingCache( file->GetNumPortals() );
		cache->type = CACHETYPE_PORTAL;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = NULL;
		cache->next = portalCacheIndex[areaNum];
		if ( portalCacheIndex[areaNum] ) {
			portalCacheIndex[areaNum]->prev = cache;
		}
		portalCacheIndex[areaNum] = cache;
		UpdatePortalRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
idAASLocal::GetClusterRoute
============
*/
bool idAASLocal::GetClusterRoute( int startAreaNum, const idVec3 &startOrigin, int startClusterNum, int goalAreaNum, int travelFlags, int &travelTime, const aasReachability_t **reach ) const {
	idRoutingCache *clusterCache = GetAreaRoutingCache( startClusterNum, goalAreaNum, travelFlags );
	int startClusterAreaNum = ClusterAreaNum( startClusterNum, startAreaNum );
	if ( clusterCache->travelTimes[startClusterAreaNum] == 0 ) {
		return false;
	}

	*reach = GetAreaReachability( startAreaNum, clusterCache->reachabilities[startClusterAreaNum] );
	travelTime = clusterCache->travelTimes[startClusterAreaNum] + AreaTravelTime( startAreaNum, startOrigin, (*reach)->GetStart() );

#if 1
	// loop over the reachabilities in the startAreaNum and pick the best one
	for ( const aasReachability_t *r = file->GetArea( startAreaNum ).reach; r != NULL; r = r->next ) {

		if ( r == *reach ) {
			continue;
		}

		if ( ( r->travelFlags & travelFlags ) == 0 ) { //mal: if travel flags don't match, don't use.
			continue;
		}

		if ( file->GetArea( r->toAreaNum ).cluster != startClusterNum ) {
			continue;
		}

		int nextAreaNum = r->toAreaNum;
		int nextClusterAreaNum = ClusterAreaNum( startClusterNum, nextAreaNum );
		if ( clusterCache->travelTimes[nextClusterAreaNum] == 0 ) {
			continue;
		}

		aasReachability_t *nextr = GetAreaReachability( nextAreaNum, clusterCache->reachabilities[nextClusterAreaNum] );

		// never go back to the starting area.  we may wind up trying to go back to the current area because
		// we calculate travel times from reachability to reachability rather than to the center points of the area
		if ( nextr->toAreaNum == startAreaNum ) {
			continue;
		}

		// travel time from the next area
		int t = clusterCache->travelTimes[nextClusterAreaNum];
		// travel time through the next area
		t += AreaTravelTime( nextAreaNum, r->GetEnd(), nextr->GetStart() );
		// travel time for the reachability from the start area to the next area
		t += r->travelTime;
		// travel time through the start area
		t += AreaTravelTime( startAreaNum, startOrigin, r->GetStart() );

		if ( t < travelTime ) {
			travelTime = t;
			*reach = r;
		}
	}
#endif
	return true;
}

/*
============
idAASLocal::RouteToGoalArea
============
*/
bool idAASLocal::RouteToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags, int &travelTime, const aasReachability_t **reach ) const {
	int startClusterNum, goalClusterNum;
	const aasPortal_t *portal;

	travelTime = 0;
	*reach = NULL;

	if ( !file ) {
		return false;
	}

	if ( startAreaNum == goalAreaNum ) {
		return true;
	}

	if ( startAreaNum <= 0 || startAreaNum >= file->GetNumAreas() ) {
		botThreadData.Warning( "RouteToGoalArea: startAreaNum %d out of range.\nStart Origin: X: %0.f, Y: %0.f, Z: %0.f", startAreaNum, startOrigin.x, startOrigin.y, startOrigin.z );
		return false;
	}
	if ( goalAreaNum <= 0 || goalAreaNum >= file->GetNumAreas() ) {
		botThreadData.Warning( "RouteToGoalArea: goalAreaNum %d out of range", goalAreaNum );
		return false;
	}

	while( totalCacheMemory > MAX_ROUTING_CACHE_MEMORY ) {
		DeleteOldestCache();
	}

	startClusterNum = file->GetArea( startAreaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the source area is a cluster portal, read directly from the portal cache
	if ( startClusterNum < 0 ) {
		// if the goal area is a portal
		if ( goalClusterNum < 0 ) {
			// just assume the goal area is part of the front cluster
			portal = &file->GetPortal( -goalClusterNum );
			goalClusterNum = portal->clusters[0];
		}
		// get the portal routing cache
		idRoutingCache *portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );
		*reach = GetAreaReachability( startAreaNum, portalCache->reachabilities[-startClusterNum] );
		travelTime = portalCache->travelTimes[-startClusterNum] + AreaTravelTime( startAreaNum, startOrigin, (*reach)->GetStart() );
		return true;
	}

	int bestTime = 0;
	const aasReachability_t *bestReach = NULL;

	// check if the goal area is a portal of the source area cluster
	if ( goalClusterNum < 0 ) {
		portal = &file->GetPortal( -goalClusterNum );
		if ( portal->clusters[0] == startClusterNum || portal->clusters[1] == startClusterNum) {
			goalClusterNum = startClusterNum;
		}
	}

	// if both areas are in the same cluster consider a direct route within the cluster first
	if ( startClusterNum > 0 && goalClusterNum > 0 && startClusterNum == goalClusterNum ) {
		GetClusterRoute( startAreaNum, startOrigin, startClusterNum, goalAreaNum, travelFlags, bestTime, &bestReach );
	}

	startClusterNum = file->GetArea( startAreaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the goal area is a portal
	if ( goalClusterNum < 0 ) {
		// just assume the goal area is part of the front cluster
		portal = &file->GetPortal( -goalClusterNum );
		goalClusterNum = portal->clusters[0];
	}

	// get the portal routing cache
	idRoutingCache *portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );

	// the cluster the area is in
	const aasCluster_t *startCluster = &file->GetCluster( startClusterNum );
	// current area inside the current cluster
	int startClusterAreaNum = ClusterAreaNum( startClusterNum, startAreaNum );
	// if the area is not a reachable area
	if ( startClusterAreaNum >= startCluster->numReachableAreas) {
		return false;
	}

	// find the portal of the source area cluster leading towards the goal area
	for ( int i = 0; i < startCluster->numPortals; i++ ) {
		int portalNum = file->GetPortalIndex( startCluster->firstPortal + i );

		// if the goal area isn't reachable from the portal
		if ( portalCache->travelTimes[portalNum] == 0 ) {
			continue;
		}

		portal = &file->GetPortal( portalNum );

		int t = 0;
		const aasReachability_t *r = NULL;

		// get the route to the cluster portal
		if ( !GetClusterRoute( startAreaNum, startOrigin, startClusterNum, portal->areaNum, travelFlags, t, &r ) ) {
			continue;
		}

		// The total travel time is the travel time from the portal area to the goal area
		// plus the travel time from the source area towards the portal area.
		t += portalCache->travelTimes[portalNum];
		// Should add the exact travel time through the portal area. However, we add the
		// largest travel time through the portal area. We cannot directly calculate the
		// exact travel time through the portal area because the reachability used to
		// travel into the portal area is not known.
		t += portal->maxAreaTravelTime;

		// if the time is better than the one already found
		if ( bestTime == 0 || t < bestTime ) {
			bestReach = r;
			bestTime = t;
		}
	}

	if ( bestReach == NULL ) {
		return false;
	}

	*reach = bestReach;
	travelTime = bestTime;

	return true;
}

/*
============
idAASLocal::TravelTimeToGoalArea
============
*/
int idAASLocal::TravelTimeToGoalArea( int startAreaNum, const idVec3 &startOrigin, int goalAreaNum, int travelFlags ) const {
	int travelTime;
	const aasReachability_t *reach;

	if ( !file ) {
		return 0;
	}

	if ( !RouteToGoalArea( startAreaNum, startOrigin, goalAreaNum, travelFlags, travelTime, &reach ) ) {
		return 0;
	}
	return travelTime;
}

/*
============
idAASLocal::FindNearestGoal
============
*/
bool idAASLocal::FindNearestGoal( idAASGoal &goal, int startAreaNum, const idVec3 &startOrigin, int travelFlags, idAASCallback &callback ) const {
	int badTravelFlags, nextAreaNum, bestAreaNum;
	unsigned short t, bestTravelTime;
	idRoutingUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	aasReachability_t *reach;
	const aasArea_t *nextArea;

	if ( file == NULL || startAreaNum <= 0 ) {
		goal.areaNum = startAreaNum;
		goal.origin = startOrigin;
		return false;
	}

	// if the first area is valid goal, just return the origin
	if ( callback.AreaIsGoal( this, startAreaNum ) ) {
		goal.areaNum = startAreaNum;
		goal.origin = startOrigin;
		return true;
	}

	badTravelFlags = ~travelFlags;
	SIMDProcessor->Memset( goalAreaTravelTimes, 0, file->GetNumAreas() * sizeof( unsigned short ) );

	// initialize first update
	curUpdate = &areaUpdate[startAreaNum];
	curUpdate->areaNum = startAreaNum;
	curUpdate->tmpTravelTime = 0;
	curUpdate->start = startOrigin;
	curUpdate->next = NULL;
	curUpdate->prev = NULL;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	bestTravelTime = 0;
	bestAreaNum = 0;

	// while there are updates in the list
	while ( updateListStart ) {

		curUpdate = updateListStart;
		if ( curUpdate->next ) {
			curUpdate->next->prev = NULL;
		} else {
			updateListEnd = NULL;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		// if we already found a closer location
		if ( bestTravelTime && curUpdate->tmpTravelTime >= bestTravelTime ) {
			continue;
		}

		for ( reach = file->GetArea( curUpdate->areaNum ).reach; reach != NULL; reach = reach->next ) {

			// if the reachability uses an undesired travel type
			if ( ( reach->travelFlags & badTravelFlags ) != 0 ) {
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->toAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if ( ( nextArea->travelFlags & badTravelFlags ) != 0 ) {
				continue;
			}

			t = curUpdate->tmpTravelTime +
					AreaTravelTime( curUpdate->areaNum, curUpdate->start, reach->GetStart() ) +
						reach->travelTime;

			t += callback.AdditionalTravelTimeForPath( this, curUpdate->start, reach->GetEnd() );

			// if we already found a closer location
			if ( bestTravelTime && t >= bestTravelTime ) {
				continue;
			}

			// if this is not the best path towards the next area
			if ( goalAreaTravelTimes[nextAreaNum] && t >= goalAreaTravelTimes[nextAreaNum] ) {
				continue;
			}

			if ( !callback.PathValid( this, curUpdate->start, reach->GetEnd() ) ) {
				continue;
			}

			goalAreaTravelTimes[nextAreaNum] = t;
			nextUpdate = &areaUpdate[nextAreaNum];
			nextUpdate->areaNum = nextAreaNum;
			nextUpdate->tmpTravelTime = t;
			nextUpdate->start = reach->GetEnd();

			// avoid areas near ledges
			if ( file->GetArea( nextAreaNum ).flags & AAS_AREA_LEDGE ) {
				nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
			}

			if ( !nextUpdate->isInList ) {
				nextUpdate->next = NULL;
				nextUpdate->prev = updateListEnd;
				if ( updateListEnd ) {
					updateListEnd->next = nextUpdate;
				} else {
					updateListStart = nextUpdate;
				}
				updateListEnd = nextUpdate;
				nextUpdate->isInList = true;
			}

			// don't put goal near a ledge
			if ( !( nextArea->flags & AAS_AREA_LEDGE ) ) {

				// add travel time through the area
				t += AreaTravelTime( reach->toAreaNum, reach->GetEnd(), AreaCenter( nextAreaNum ) );
	
				if ( !bestTravelTime || t < bestTravelTime ) {
					// test the area
					if ( callback.AreaIsGoal( this, reach->toAreaNum ) ) {
						bestTravelTime = t;
						bestAreaNum = reach->toAreaNum;
					}
				}
			}
		}
	}

	if ( bestAreaNum ) {
		goal.areaNum = bestAreaNum;
		goal.origin = AreaCenter( bestAreaNum );
		return true;
	}

	return false;
}
