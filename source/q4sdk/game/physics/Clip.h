
#ifndef __CLIP_H__
#define __CLIP_H__

// RAVEN BEGIN
// ddynerman: SD's clip sector code
const int CLIPSECTOR_DEPTH				= 6;
const int CLIPSECTOR_WIDTH				= 1 << CLIPSECTOR_DEPTH;
// RAVEN END

/*
===============================================================================

  Handles collision detection with the world and between physics objects.

===============================================================================
*/

#define CLIPMODEL_ID_TO_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id )) )
#define JOINT_HANDLE_TO_CLIPMODEL_ID( id )	( -1 - id )

class idClip;
class idClipModel;
class idEntity;


//===============================================================
//
//	idClipModel
//
//===============================================================

class idClipModel {

	friend class idClip;

public:
							idClipModel( void );
							explicit idClipModel( const char *name );
							explicit idClipModel( const idTraceModel &trm, const idMaterial *material = NULL );
							explicit idClipModel( const int renderModelHandle );
							explicit idClipModel( const idClipModel *model );
							~idClipModel( void );
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	void					UpdateDynamicContents( void );
// RAVEN END

	bool					LoadModel( const char *name );
	void					LoadModel( const idTraceModel &trm, const idMaterial *material, bool notHashed = false );
	void					LoadModel( const int renderModelHandle );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	void					Link( void );				// must have been linked with an entity and id before
	void					Link( idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int renderModelHandle = -1 );
// RAVEN END
	void					Unlink( void );						// unlink from sectors
	void					SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis );	// unlinks the clip model
	void					Translate( const idVec3 &translation );							// unlinks the clip model
	void					Rotate( const idRotation &rotation );							// unlinks the clip model
	void					Enable( void );						// enable for clipping
	void					Disable( void );					// keep linked but disable for clipping
	void					SetContents( int newContents );		// override contents
	int						GetContents( void ) const;
	void					SetEntity( idEntity *newEntity );
	idEntity *				GetEntity( void ) const;
	void					SetId( int newId );
	int						GetId( void ) const;
	void					SetOwner( idEntity *newOwner );
	idEntity *				GetOwner( void ) const;
	const idBounds &		GetBounds( void ) const;
	const idBounds &		GetAbsBounds( void ) const;
	const idVec3 &			GetOrigin( void ) const;
	const idMat3 &			GetAxis( void ) const;
	bool					IsTraceModel( void ) const;			// returns true if this is a trace model
	bool					IsRenderModel( void ) const;		// returns true if this is a render model
	bool					IsLinked( void ) const;				// returns true if the clip model is linked
	bool					IsEnabled( void ) const;			// returns true if enabled for collision detection
	bool					IsEqual( const idTraceModel &trm ) const;
	idCollisionModel *		GetCollisionModel( void ) const;	// returns handle used to collide vs this model
	const idTraceModel *	GetTraceModel( void ) const;
	void					GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const;

	static void				ClearTraceModelCache( void );
	static int				TraceModelCacheSize( void );
	static void				SaveTraceModels( idSaveGame *savefile );
	static void				RestoreTraceModels( idRestoreGame *savefile );

private:
	bool					enabled;				// true if this clip model is used for clipping
// RAVEN BEGIN
	bool					checked;				// Splash's clip model code
// RAVEN END
	idEntity *				entity;					// entity using this clip model
	int						id;						// id for entities that use multiple clip models
	idEntity *				owner;					// owner of the entity that owns this clip model
	idVec3					origin;					// origin of clip model
	idMat3					axis;					// orientation of clip model
	idBounds				bounds;					// bounds
	idBounds				absBounds;				// absolute bounds
	int						contents;				// all contents ored together
	idCollisionModel *		collisionModel;			// handle to collision model
	int						traceModelIndex;		// trace model used for collision detection
	int						renderModelHandle;		// render model def handle

	struct clipLink_s *		clipLinks;				// links into sectors
	int						touchCount;

	void					Init( void );			// initialize
	void					FreeModel( void );
	void					Link_r( struct clipSector_s *node );

	static void				CacheCollisionModels( void );
	static int				AllocTraceModel( const idTraceModel &trm, const idMaterial *material, bool notHashed = false );
	static void				ReplaceTraceModel( int index, const idTraceModel &trm, const idMaterial *material, bool notHashed = false );
	static void				FreeTraceModel( int traceModelIndex );
	static int				CopyTraceModel( const int traceModelIndex );
	static idTraceModel *	GetCachedTraceModel( int traceModelIndex );
	static idCollisionModel*GetCachedCollisionModel( int traceModelIndex );
	static int				GetTraceModelHashKey( const idTraceModel &trm );
};


ID_INLINE void idClipModel::Translate( const idVec3 &translation ) {
	Unlink();
	origin += translation;
}

ID_INLINE void idClipModel::Rotate( const idRotation &rotation ) {
	Unlink();
	origin *= rotation;
	axis *= rotation.ToMat3();
}

ID_INLINE void idClipModel::Enable( void ) {
	enabled = true;
}

ID_INLINE void idClipModel::Disable( void ) {
	enabled = false;
}

ID_INLINE void idClipModel::SetContents( int newContents ) {
	contents = newContents;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	UpdateDynamicContents();
// RAVEN END
}

ID_INLINE int idClipModel::GetContents( void ) const {
	return contents;
}

ID_INLINE void idClipModel::SetEntity( idEntity *newEntity ) {
	entity = newEntity;
}

ID_INLINE idEntity *idClipModel::GetEntity( void ) const {
	return entity;
}

ID_INLINE void idClipModel::SetId( int newId ) {
	id = newId;
}

ID_INLINE int idClipModel::GetId( void ) const {
	return id;
}

ID_INLINE void idClipModel::SetOwner( idEntity *newOwner ) {
	owner = newOwner;
}

ID_INLINE idEntity *idClipModel::GetOwner( void ) const {
	return owner;
}

ID_INLINE const idBounds &idClipModel::GetBounds( void ) const {
	return bounds;
}

ID_INLINE const idBounds &idClipModel::GetAbsBounds( void ) const {
	return absBounds;
}

ID_INLINE const idVec3 &idClipModel::GetOrigin( void ) const {
	return origin;
}

ID_INLINE const idMat3 &idClipModel::GetAxis( void ) const {
	return axis;
}

ID_INLINE bool idClipModel::IsRenderModel( void ) const {
	return ( renderModelHandle != -1 );
}

ID_INLINE bool idClipModel::IsTraceModel( void ) const {
	return ( traceModelIndex != -1 );
}

ID_INLINE bool idClipModel::IsLinked( void ) const {
	return ( clipLinks != NULL );
}

ID_INLINE bool idClipModel::IsEnabled( void ) const {
	return enabled;
}

ID_INLINE bool idClipModel::IsEqual( const idTraceModel &trm ) const {
	return ( traceModelIndex != -1 && *GetCachedTraceModel( traceModelIndex ) == trm );
}

ID_INLINE const idTraceModel *idClipModel::GetTraceModel( void ) const {
	if ( !IsTraceModel() ) {
		return NULL;
	}
	return idClipModel::GetCachedTraceModel( traceModelIndex );
}


//===============================================================
//
//	idClip
//
//===============================================================

class idClip {

	friend class idClipModel;

public:
							idClip( void );

	void					Init( void );
	void					Shutdown( void );

	// clip versus the rest of the world
// RAVEN BEGIN
// nmckenzie: we have cases where both a guy and his target need to be ignored by a translation
	bool					Translation( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 = 0 );
// RAVEN END
	bool					Rotation( trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	bool					Motion( trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	int						Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
// RAVEN BEGIN
// AReis: Added ability to get the entity that was touched as well.
	int						Contents( const idVec3 &start, 
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, idEntity **touchedEntity = NULL );
// RAVEN END

	// special case translations versus the rest of the world
	bool					TracePoint( trace_t &results, const idVec3 &start, const idVec3 &end,
								int contentMask, const idEntity *passEntity );
	bool					TraceBounds( trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds,
								int contentMask, const idEntity *passEntity );

	// clip versus a specific model
	void					TranslationModel( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	void					RotationModel( trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContactsModel( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContentsModel( const idVec3 &start,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis );

	// clip versus all entities but not the world
// RAVEN BEGIN
// nmckenzie: had to add a second pass entity so we can safely ignore both a guy and his target in some cases
	void					TranslationEntities( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 = 0 );
// RAVEN END

	// get a contact feature
	bool					GetModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const;

	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount ) const;
	int						ClipModelsTouchingBounds( const idBounds &bounds, int contentMask, idClipModel **clipModelList, int maxCount ) const;

// RAVEN BEGIN
// ddynerman: another helper function, useful in MP
	int						PlayersTouchingBounds( const idBounds &bounds, int contentMask, idPlayer **entityList, int maxCount ) const;
// RAVEN END

	const idBounds &		GetWorldBounds( void ) const;
	idCollisionModel *		GetWorldCollisionModel( void ) const { return world; }

// RAVEN BEGIN
// ddynerman: change to static
	static idClipModel *	DefaultClipModel( void );
	static void				FreeDefaultClipModel( void );
// RAVEN END

							// stats and debug drawing
	void					PrintStatistics( void );
	void					DrawClipModels( const idVec3 &eye, const float radius, const idEntity *passEntity, const idTypeInfo* type = NULL );
	bool					DrawModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, int lifetime ) const;

// RAVEN BEGIN
// rjohnson: added debug hud support
	void					DebugHudStatistics( void );
	void					ClearStatistics( void );
// RAVEN END

// RAVEN BEGIN
// ddynerman: SD's clip sector code
	void					CoordsForBounds( int* coords, idBounds& bounds ) const;
	void					DrawClipSectors( void ) const;
	void					DrawAreaClipSectors( float range ) const;	
	static void				UpdateDynamicContents( struct clipSector_s* sector );
	static void				UpdateDynamicContents( idClipModel* clipModel );
// RAVEN END

private:
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	idVec3					nodeScale;
	idVec3					nodeOffset;
	idVec3					nodeOffsetVisual;
// ddynerman: change to static
	static idClipModel		defaultClipModel;
// RAVEN END
	struct clipSector_s *	clipSectors;
	idCollisionModel *		world;
	idBounds				worldBounds;
	idClipModel				temporaryClipModel;

	mutable int				touchCount;
							// statistics
	int						numTranslations;
	int						numRotations;
	int						numMotions;
	int						numRenderModelTraces;
	int						numContents;
	int						numContacts;

private:
	struct clipSector_s *	CreateClipSectors_r( const int depth, const idBounds &bounds, idVec3 &maxSector );
	void					ClipModelsTouchingBounds_r( const struct clipSector_s *node, struct listParms_s &parms ) const;
	const idTraceModel *	TraceModelForClipModel( const idClipModel *mdl ) const;
// RAVEN BEGIN
// nmckenzie: had to add a second pass entity so we can safely ignore both a guy and his target in some cases
	int						GetTraceClipModels( const idBounds &bounds, int contentMask, const idEntity *passEntity, idClipModel **clipModelList, const idEntity *passEntity2 = 0 ) const;
// RAVEN END
	void					TraceRenderModel( trace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, const idMat3 &axis, idClipModel *touch ) const;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	void					GetClipSectorsStaticContents( void );
// RAVEN END
};


ID_INLINE bool idClip::TracePoint( trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity ) {
	Translation( results, start, end, NULL, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE bool idClip::TraceBounds( trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds, int contentMask, const idEntity *passEntity ) {
	temporaryClipModel.LoadModel( idTraceModel( bounds ), NULL, true );
	Translation( results, start, end, &temporaryClipModel, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE const idBounds & idClip::GetWorldBounds( void ) const {
	return worldBounds;
}

// RAVEN BEGIN
// ddynerman: SD's clip sector code
ID_INLINE void idClip::CoordsForBounds( int* coords, idBounds& bounds ) const {
	float fCoords[ 4 ];

	fCoords[ 0 ] = ( bounds[ 0 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 1 ] = ( bounds[ 0 ].y - nodeOffset.y ) * nodeScale.y;
	fCoords[ 2 ] = ( bounds[ 1 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 3 ] = ( bounds[ 1 ].y - nodeOffset.y ) * nodeScale.y;

	int i;
	for( i = 0; i < 4; i++ ) {

		coords[ i ] = idMath::FtoiFast( fCoords[ i ] );

		if( coords[ i ] < 0 ) {
			coords[ i ] = 0;
		} else if( coords[ i ] > CLIPSECTOR_WIDTH - 1 ) {
			coords[ i ] = CLIPSECTOR_WIDTH - 1;
		}
	}
	coords[ 2 ]++; coords[ 3 ]++;
}
// RAVEN END

#endif /* !__CLIP_H__ */
