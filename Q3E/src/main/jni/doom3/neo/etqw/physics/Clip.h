// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __CLIP_H__
#define __CLIP_H__

const int CLIPSECTOR_DEPTH				= 5;
const int CLIPSECTOR_WIDTH				= 1 << CLIPSECTOR_DEPTH;

/*
===============================================================================

	Handles collision detection with the world and between clip models.

===============================================================================
*/

#define CLIPMODEL_ID_TO_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id )) )
#define JOINT_HANDLE_TO_CLIPMODEL_ID( id )	( -1 - id )
#define INVALID_BODY						( -1 )

class idClipModel;
class idEntity;
class idBounds;
class sdLadderEntity;

struct clipSector_t {
	int						contents;
	struct clipLink_t *		clipLinks;
};

typedef idBlockAlloc<clipLink_t, 1024>	idClipLinkBlockAlloc;

struct clipLink_t {
	idClipModel *			clipModel;
	clipSector_t *			sector;
	clipLink_t *			prevInSector;
	clipLink_t *			nextInSector;
	clipLink_t *			nextLink;
};

//#define CLIP_DEBUG

#ifdef CLIP_DEBUG

// CLIP_DEBUG_EXTREME this enables per-frame trace logging, for spike hunting
//#define CLIP_DEBUG_EXTREME

#define CLIP_DEBUG_PARMS_DECLARATION const char* clipDebugFileName, int clipDebugLineNum,
#define CLIP_DEBUG_PARMS_DECLARATION_ONLY const char* clipDebugFileName, int clipDebugLineNum
#define CLIP_DEBUG_PARMS __FILE__, __LINE__,
#define CLIP_DEBUG_PARMS_ONLY __FILE__, __LINE__
#define CLIP_DEBUG_PARMS_PASSTHRU clipDebugFileName, clipDebugLineNum,
#define CLIP_DEBUG_PARMS_PASSTHRU_ONLY clipDebugFileName, clipDebugLineNum
#define CLIP_DEBUG_PARMS_SCRIPT gameLocal.program->GetCurrentThread()->CurrentFile(), gameLocal.program->GetCurrentThread()->CurrentLine(),
#define CLIP_DEBUG_PARMS_ENTINFO_ONLY( ent )		\
	( ent != NULL ? (								\
		ent->IsType( rvClientPhysics::Type ) ? (	\
			( ( rvClientPhysics* )ent )->GetCurrentClientEntity()->GetSpawnArgs() != NULL ? (	\
				va( "%s - %s", __FILE__, ( ( rvClientPhysics* )ent )->GetCurrentClientEntity()->GetSpawnArgs()->GetString( "classname" ) )	\
			) : va( "%s - misc rvClientEntity", __FILE__ )	\
		) : va( "%s - %s", __FILE__, ent->spawnArgs.GetString( "classname" ) )	\
	) : va( "%s - NULL", __FILE__ ) ), __LINE__

#define CLIP_DEBUG_PARMS_ENTINFO( ent )		CLIP_DEBUG_PARMS_ENTINFO_ONLY( ent ),

#define CLIP_DEBUG_PARMS_CLIENTINFO_ONLY( ent )		\
	( ent != NULL ? (								\
		ent == gameLocal.GetLocalPlayer() ?			\
			va( "%s - local client", __FILE__ )		\
			: va( "%s - non-local", __FILE__ )		\
	) : va( "%s - NULL", __FILE__ ) ), __LINE__

#define CLIP_DEBUG_PARMS_CLIENTINFO( ent )	CLIP_DEBUG_PARMS_CLIENTINFO_ONLY( ent ),


enum clipTimerMode_t {
	CTM_TRANSLATION,
	CTM_ROTATION,
	CTM_CONTACTS,
	CTM_CONTENTS,
	CTM_CLIPMODELSTOUCHINGBOUNDS,
	CTM_FINDLADDER,
	CTM_FINDWATER,
	CTM_NUM,
};

struct clipTimerInfo_t {
	int		lineNumber;
	int		count;
	double	time;
};


struct clipTimerFrameInfo_t {
	int		count;
	double	elapsedTime;
};

#define CLIP_DEBUG_MAX_FRAMES	8192

struct clipTimerInfoExtreme_t {
//	int lineNumber;
	clipTimerFrameInfo_t	frameInfo[ CLIP_DEBUG_MAX_FRAMES ];
};

#else

#define CLIP_DEBUG_PARMS_DECLARATION
#define CLIP_DEBUG_PARMS_DECLARATION_ONLY void
#define CLIP_DEBUG_PARMS
#define CLIP_DEBUG_PARMS_ONLY
#define CLIP_DEBUG_PARMS_PASSTHRU
#define CLIP_DEBUG_PARMS_PASSTHRU_ONLY
#define CLIP_DEBUG_PARMS_SCRIPT
#define CLIP_DEBUG_PARMS_ENTINFO_ONLY( ent )
#define CLIP_DEBUG_PARMS_ENTINFO( ent )
#define CLIP_DEBUG_PARMS_CLIENTINFO_ONLY( ent )
#define CLIP_DEBUG_PARMS_CLIENTINFO( ent )

#endif // CLIP_DEBUG

class idClip {
public:
							idClip( void );

	void					Init( void );
	void					Shutdown( void );

	// clip versus the rest of the world
	bool					Translation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT, float forceRadius = -1.f );
	bool					Rotation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );
	bool					Motion( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );
	int						Contacts( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );
	int						Contents( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );

	// special case translations versus the rest of the world
	bool					TracePoint( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT, float forceRadius = -1.f );

	bool					TraceBounds( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds,
								const idMat3& axis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );

	bool					TracePointExt( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								int contentMask, const idEntity *passEntity1, const idEntity *passEntity2, traceMode_t traceMode = TM_DEFAULT );

	// clip versus a specific model
	void					TranslationModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
								const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	void					RotationModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
								const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContactsModel( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
								const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContentsModel( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
								const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );


	// clip versus the world but not the entities
	void					TranslationWorld( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask );
	int						ContentsWorld( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask );

	// clip versus all entities but not the world
	bool					TranslationEntities( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode = TM_DEFAULT );

	// trace against a single render model
	void					TraceRenderModel( trace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, const idMat3 &axis, int contentMask, idClipModel *touch );

	int						EntitiesForTranslation( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start, const idVec3 &end, int contentMask, const idEntity* passEntity, idEntity** entities, int maxEntities, traceMode_t traceMode = TM_DEFAULT );

	// get a contact feature
	bool					GetModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const;

	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds( const idBounds& bounds, int contentMask, idEntity **entityList, int maxCount, bool nowarning = false ) const;
	int						EntitiesTouchingBounds( const idBounds& bounds, int contentMask, idEntity **entityList, int maxCount, const idTypeInfo& type, bool nowarning = false ) const;
	int						EntitiesTouchingRadius( const idSphere& sphere, int contentMask, idEntity **entityList, int maxCount, bool nowarning ) const;
	int						ClipModelsTouchingBounds( CLIP_DEBUG_PARMS_DECLARATION const idBounds& bounds, int contentMask, const idClipModel **clipModelList, int maxCount, const idEntity* passEntity, bool includeWorld = false ) const;

	//idClipModel *			GetWorld( void ) const { return world; }
	const idBounds&			GetWorldBounds( void ) const;
	void					GetWorldBounds( idBounds& bounds, int surfaceMask, bool inclusive ) const;
	idClipModel *			DefaultClipModel( void );

							// stats and debug drawing
	void					PrintStatistics( void );
	void					DrawWorld( float radius );
	void					DrawClipModels( const idVec3 &viewOrigin, const idMat3 &viewAxis, const float radius, const idEntity *passEntity );
	bool					DrawModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, int lifetime ) const;

	void					CoordsForBounds( int* coords, const idBounds& bounds ) const;
	void					DrawClipSectors( void ) const;
	void					DrawAreaClipSectors( float range ) const;

	idClipLinkBlockAlloc&	GetClipLinkAllocator( void ) { return clipLinkAllocator; }
	clipSector_t*			GetClipSectors( void ) { return clipSectors; }

	void					PrecacheModel( const char* clipModelName );
	bool					LoadTraceModel( const char* clipModelName, idTraceModel& trm );

	int						FindLadder( CLIP_DEBUG_PARMS_DECLARATION const idBounds& bounds, sdLadderEntity** ladders, int maxLadders );
	int						FindWater( CLIP_DEBUG_PARMS_DECLARATION const idBounds& bounds, const idClipModel** clipModelList, int maxCount );

							// each thread will have to allocate
	void					AllocThread( void );
	void					FreeThread( void );
	int						GetThreadCount( void );

							// lock linking for multi-threading
	void					Lock( void ) const { lock.Acquire(); }
	void					Unlock( void ) const { lock.Release(); }

	void					DeleteClipModel( idClipModel *clipModel );
	void					ThreadDeleteClipModels( void );
	void					ActuallyDeleteClipModels( bool force = false );

#ifdef CLIP_DEBUG
	void					LogTrace( const char* fileName, int lineNumber, double time, clipTimerMode_t mode );
	void					PrintTraceTimings( void );
	void					ClearTraceTimings( void );
#endif // CLIP_DEBUG

#ifdef CLIP_DEBUG_EXTREME
	void					LogTraceExtreme( const char* fileName, int lineNumber, double time, clipTimerMode_t mode );
	void					StartTraceLogging( void );
	void					StopTraceLogging( void );
	bool					IsTraceLogging( void ) { return isTraceLogging; }
	void					UpdateTraceLogging( void );
	void					DumpTraceLog( void );
	void					DumpTraceLog( int mode );
	void					AssembleTraceLogs( void );
	void					AssembleTraceLogs( int mode );

#endif // CLIP_DEBUG_EXTREME

	const idClipModel*		GetTemporaryClipModel( const idBounds& bounds );
	
	const idClipModel*		GetThirdPersonOffsetModel( void );
	const idClipModel*		GetBigThirdPersonOffsetModel( void );
	const idClipModel*		GetLeanOffsetModel( void );

private:
	idVec3					nodeScale;
	idVec3					inverseNodeScale;
	idVec3					nodeOffset;

	idClipLinkBlockAlloc	clipLinkAllocator;

	idBounds				worldBounds;
	idList< idClipModel* >	worldClips;
	idClipModel*			temporaryClipModel;
	idClipModel*			thirdPersonOffsetModel;
	idClipModel*			bigThirdPersonOffsetModel;
	idClipModel*			leanOffsetModel;
	idClipModel*			defaultClipModel;

#ifdef CLIP_DEBUG
	idHashMap< idList< clipTimerInfo_t > > clipTimerInfo[ CTM_NUM ];
#endif // CLIP_DEBUG

#ifdef CLIP_DEBUG_EXTREME
	idHashMap< clipTimerInfoExtreme_t* > clipTimerInfoExtreme[ CTM_NUM ];
	int						clipTimerSampleTimes[ CLIP_DEBUG_MAX_FRAMES ];
	int						clipSampleUpto;
	int						logSubFileUpto;
	int						logFileUpto;
	bool					isTraceLogging;
#endif

	clipSector_t *			clipSectors;

							// statistics
	int						numTranslations;
	int						numRotations;
	int						numMotions;
	int						numRenderModelTraces;
	int						numContents;
	int						numContacts;

	idClipModel *			deletedClipModels;
	mutable sdLock			lock;				// lock for multi-threading

	static					unsigned int mailBoxID;

private:
	clipSector_t*			CreateClipSectors( const int depth, const idBounds &bounds );
	int						GetTraceClipModels( const idVec3& start, const idVec3& end, int contentMask, const idEntity* passEntity, idClipModel** clipModelList, traceMode_t traceMode ) const;
	int						GetTraceClipModels( const idBounds &bounds, int contentMask, const idEntity *passEntity, idClipModel** clipModelList, traceMode_t traceMode ) const;
	int						GetTraceClipModelsExt( const idVec3& start, const idVec3& end, int contentMask, const idEntity* passEntity1, const idEntity *passEntity2, idClipModel** clipModelList, traceMode_t traceMode ) const;
	void					GetClipSectorsStaticContents( void );
	bool					TestHugeTranslation( trace_t &results, const idClipModel *mdl, const idVec3 &start, const idVec3 &end, const idMat3 &trmAxis ) const;

	bool					TranslationClipModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl );
	bool					RotationClipModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl );
	int						ContactsClipModel( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec3 *dir, const float depth,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl );
	int						ContentsClipModel( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
									const idTraceModel *trm, const idMat3 &trmAxis, int contentMask, const idClipModel *mdl );

	// the raw versions of these functions - they don't attempt to correct non-orthonormal axes
	bool					RotationInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode );
	bool					MotionInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode );
	void					RotationModelInternal( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
								const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
};


ID_INLINE bool idClip::TracePoint( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity, traceMode_t traceMode, float forceRadius ) {
	Translation( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, NULL, mat3_identity, contentMask, passEntity, traceMode, forceRadius );
	return ( results.fraction < 1.0f );
}

ID_INLINE idClipModel *idClip::DefaultClipModel( void ) {
	return defaultClipModel;
}

ID_INLINE void idClip::CoordsForBounds( int* coords, const idBounds& bounds ) const {
	float fCoords[ 4 ];

	fCoords[ 0 ] = ( bounds[ 0 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 1 ] = ( bounds[ 0 ].y - nodeOffset.y ) * nodeScale.y;
	fCoords[ 2 ] = ( bounds[ 1 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 3 ] = ( bounds[ 1 ].y - nodeOffset.y ) * nodeScale.y;

	coords[ 0 ] = idMath::ClampInt( 0, CLIPSECTOR_WIDTH - 1, idMath::Ftoi( fCoords[ 0 ] ) );
	coords[ 1 ] = idMath::ClampInt( 0, CLIPSECTOR_WIDTH - 1, idMath::Ftoi( fCoords[ 1 ] ) );
	coords[ 2 ] = idMath::ClampInt( 0, CLIPSECTOR_WIDTH - 1, idMath::Ftoi( fCoords[ 2 ] ) ) + 1;
	coords[ 3 ] = idMath::ClampInt( 0, CLIPSECTOR_WIDTH - 1, idMath::Ftoi( fCoords[ 3 ] ) ) + 1;
}

ID_INLINE bool idClip::Rotation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
							const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {

	if ( RotationInternal( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, mdl, trmAxis, contentMask, passEntity, traceMode ) ) {
		// ensure that non-orthonormal axes don't creep their way into the physics
		results.endAxis.OrthoNormalizeSelf();
		return true;
	}
	return false;
}

ID_INLINE bool idClip::Motion( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
							const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, traceMode_t traceMode ) {

	if ( MotionInternal( CLIP_DEBUG_PARMS_PASSTHRU results, start, end, rotation, mdl, trmAxis, contentMask, passEntity, traceMode ) ) {
		// ensure that non-orthonormal axes don't creep their way into the physics
		results.endAxis.OrthoNormalizeSelf();
		return true;
	}
	return false;
}

ID_INLINE void idClip::RotationModel( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
							const idClipModel *trm, const idMat3 &trmAxis, int contentMask,
							const idClipModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {

	RotationModelInternal( CLIP_DEBUG_PARMS_PASSTHRU results, start, rotation, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
	if ( results.fraction < 1.0f ) {
		// ensure that non-orthonormal axes don't creep their way into the physics
		results.endAxis.OrthoNormalizeSelf();
	}
}

#endif /* !__CLIP_H__ */
