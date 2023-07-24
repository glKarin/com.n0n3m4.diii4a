// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __TRACEMODELCACHE_H__
#define __TRACEMODELCACHE_H__

/*
===============================================================

	idTraceModelCache

===============================================================
*/

#define MAX_TRACEMODEL_WATER_POINTS			64
#define MAX_TRACEMODEL_WATER_POINTS_POOL	( MAX_TRACEMODEL_WATER_POINTS * 3 )

struct traceModelWater_t {
	idVec3				xyz;
	float				weight;
};

class idTraceModelCache {
public:
	void							ClearTraceModelCache( void );
	size_t							TraceModelCacheSize( void );

	int								FindTraceModel( const char* fileName, bool includeBrushes );
	int								FindTraceModel( const idTraceModel &trm, bool includeBrushes );
	int								PrecacheTraceModel( const idTraceModel &trm, const char* fileName );
	int								AllocTraceModel( const idTraceModel& trm, bool includeBrushes );
	void							FreeTraceModel( const int traceModelIndex );
	int								CopyTraceModel( const int traceModelIndex );

	const idTraceModel *			GetTraceModel( const int traceModelIndex ) const;
	const traceModelWater_t*		GetWaterPoints( const int traceModelIndex ) const;
	idCollisionModel *				GetCollisionModel( const int traceModelIndex ) const;
	float							GetVolume( const int traceModelIndex ) const;
	void							GetMassProperties( const int traceModelIndex, const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const;

	static const idMaterial*		TrmMaterialForName( const char* name );
	static const char*				TrmNameForMaterial( const idMaterial* material );

	void							Write( int index, idFile* fp );
	void							Read( idTraceModel& trm, idFile* fp );

private:
	void							AllocFileEntry( const char* fileName, int traceModelIndex );
	int								FindFileEntry( const char* fileName, bool includeBrushes );

	// stuff for figuring out the water points
	struct polyPoint_t {
		idVec3						xyz;
		float						weight;
		float						squareWeight;
		idLinkList< polyPoint_t >	node;
	};

	typedef polyPoint_t*			polyPointPtr_t;

	static polyPoint_t				polyPointPool[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
	static polyPoint_t*				freePolyPoints[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
	static int						numFreePolyPoints;
	static bool						polyPointPoolValid;

	static polyPoint_t*				NewPolyPoint( void );
	static void						DeletePolyPoint( polyPoint_t* point );
	static void						DeletePointList( idLinkList< polyPoint_t >& points );
	static void						FindClosestPoints( idLinkList< polyPoint_t >& points, polyPointPtr_t& closePoint1, polyPointPtr_t& closePoint2 );

	struct trmCache_t {
		idTraceModel				trm;
		int							refCount;
		float						volume;				// volume of trace model
		idVec3						centerOfMass;		// center of mass
		idMat3						inertiaTensor;		// inertia tensor
		idCollisionModel *			collisionModel;		// trace model converted to a collision model
		bool						hasWater;
		bool						includesBrushes;
		traceModelWater_t			waterPoints[ MAX_TRACEMODEL_WATER_POINTS ];
	};

	struct trmFileCache_t {
		idStr						fileName;
		int							entryIndex;
	};

	idList< trmCache_t* >			cache;
	idBlockAlloc< trmCache_t, 64 >	allocator;

	idList< trmFileCache_t >		fileCache;
	idHashIndex						nameHash;

	idHashIndex						hash;

	void							SetupWaterPoints( trmCache_t& entry );
	int								GetTraceModelHashKey( const idTraceModel &trm );
};

ID_INLINE const idTraceModel *idTraceModelCache::GetTraceModel( const int traceModelIndex ) const {
	return &cache[ traceModelIndex ]->trm;
}

ID_INLINE idCollisionModel *idTraceModelCache::GetCollisionModel( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->collisionModel;
}

ID_INLINE const traceModelWater_t* idTraceModelCache::GetWaterPoints( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->hasWater ? cache[ traceModelIndex ]->waterPoints : NULL;
}

ID_INLINE float idTraceModelCache::GetVolume( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->volume;
}

#endif /* !__TRACEMODELCACHE_H__ */
