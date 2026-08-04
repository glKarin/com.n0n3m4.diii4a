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

#ifndef __CLIP_H__
#define __CLIP_H__

#include "containers/FlexList.h"
#include "bv/BoxOctree.h"

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
struct listParmsMoving;

//stgatilov: size of automatic storage in returned arrays of entities
#define CLIPARRAY_AUTOSIZE 128
typedef idFlexList<idEntity*, CLIPARRAY_AUTOSIZE> idClip_EntityList;
typedef idFlexList<idClipModel*, CLIPARRAY_AUTOSIZE> idClip_ClipModelList;
typedef idFlexList<float, CLIPARRAY_AUTOSIZE> idClip_FloatList;


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
							explicit idClipModel( const char *name, const idDeclSkin* skin ); // #4232 SteveL
							explicit idClipModel( const idTraceModel &trm );
							explicit idClipModel( const int renderModelHandle );
							explicit idClipModel( const idClipModel *model );
							~idClipModel( void );

	bool					LoadModel( const char *name );
	bool					LoadModel( const char *name, const idDeclSkin* skin ); // #4232 SteveL
	void					LoadModel( const idTraceModel &trm );
	void					LoadModel( const int renderModelHandle );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Link( idClip &clp );				// must have been linked with an entity and id before
	void					Link( idClip &clp, idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int renderModelHandle = -1 );
	void					Unlink( void );						// unlink from sectors
	void					SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis );	// unlinks the clip model
	/**
	* Translates the origin of the clip model relative to the clipmodel itself
	* Unlinks the clip model
	**/
	void					TranslateOrigin( const idVec3 &translation );
	void					Translate( const idVec3 &translation );							// unlinks the clip model
	void					Scale( const idVec3 &scale );								// unlinks the clip model
	void					Rotate( const idRotation &rotation );							// unlinks the clip model
	void					Enable( void );						// enable for clipping
	void					Disable( void );					// keep linked but disable for clipping
	void					SetMaterial( const idMaterial *m );
	const idMaterial *		GetMaterial( void ) const;
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
	cmHandle_t				Handle( void ) const;				// returns handle used to collide vs this model
	idBoxOctreeHandle&		GetOctreeHandle( void ) { return octreeHandle; }
	const idTraceModel *	GetTraceModel( void ) const;
	void					GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const;

	static cmHandle_t		CheckModel( const char *name, const idDeclSkin* skin = NULL ); // skin added #4232 SteveL
	static void				ClearTraceModelCache( void );
	static int				TraceModelCacheSize( void );

	static void				SaveTraceModels( idSaveGame *savefile );
	static void				RestoreTraceModels( idRestoreGame *savefile );

private:
	bool					enabled;				// true if this clip model is used for clipping
	idEntity *				entity;					// entity using this clip model
	int						id;						// id for entities that use multiple clip models
	idEntity *				owner;					// owner of the entity that owns this clip model
	idVec3					origin;					// origin of clip model
	idMat3					axis;					// orientation of clip model
	idBounds				bounds;					// bounds
	idBounds				absBounds;				// absolute bounds
	const idMaterial *		material;				// material for trace models
	int						contents;				// all contents ored together
	cmHandle_t				collisionModelHandle;	// handle to collision model
	int						traceModelIndex;		// trace model used for collision detection
	int						renderModelHandle;		// render model def handle

	idBoxOctreeHandle		octreeHandle;			// links back to the octree containing the model
	int						touchCount;				// mutable counter to avoid double-reporting clipmodel

	void					Init( void );			// initialize

	static int				AllocTraceModel( const idTraceModel &trm );
	static void				FreeTraceModel( const int traceModelIndex );
	static idTraceModel *	GetCachedTraceModel( int traceModelIndex );
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

ID_INLINE void idClipModel::Scale( const idVec3 &scale ) {
	if( IsTraceModel() )
	{
		// copy & scale the tracemodel
		// Tels: Is the copy nec.?
		idTraceModel trm = *(idClipModel::GetCachedTraceModel( traceModelIndex ));
		trm.Scale( scale );
		
		LoadModel( trm );
	}
}

ID_INLINE void idClipModel::Enable( void ) {
	enabled = true;
}

ID_INLINE void idClipModel::Disable( void ) {
	enabled = false;
}

ID_INLINE void idClipModel::SetMaterial( const idMaterial *m ) {
	material = m;
}

ID_INLINE const idMaterial * idClipModel::GetMaterial( void ) const {
	return material;
}

ID_INLINE void idClipModel::SetContents( int newContents ) {
	contents = newContents;
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
	return octreeHandle.IsLinked();
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
							~idClip( void );

	void					Init( void );
	void					Shutdown( void );

	// stgatilov: should be called after all clipmodels got into their normal position
	// when clipmodels are first linked, they are located at origin, which results in suboptimal octree
	void					Optimize( void );

	// clip versus the rest of the world
	bool					Translation( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, bool ignoreWorld = false );
	bool					Rotation( trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	bool					Motion( trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	int						Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	int						Contents( const idVec3 &start,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );

	// special case translations versus the rest of the world
	bool					TracePoint( trace_t &results, const idVec3 &start, const idVec3 &end,
								int contentMask, const idEntity *passEntity );
	bool					TraceBounds( trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds,
								int contentMask, const idEntity *passEntity );

	// clip versus a specific model
	void					TranslationModel( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	void					RotationModel( trace_t &results, const idVec3 &start, const idRotation &rotation,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContactsModel( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContentsModel( const idVec3 &start,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
								cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );

	// clip versus all entities but not the world
	void					TranslationEntities( trace_t &results, const idVec3 &start, const idVec3 &end,
								const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );

	// get a contact feature
	bool					GetModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const;

	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idClip_EntityList &entityList ) const;
	int						ClipModelsTouchingBounds( const idBounds &bounds, int contentMask, idClip_ClipModelList &clipModelList ) const;

	const idBounds &		GetWorldBounds( void ) const;
	idClipModel *			DefaultClipModel( void );

							// stats and debug drawing
	void					PrintStatistics( void );
	void					DrawClipModels( const idVec3 &eye, const float radius, const idEntity *passEntity );
	bool					DrawModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, int lifetime ) const;
	void					DrawClipModel( const idClipModel *clipModel, const idVec3 &eye, const float radius ) const;

private:
	idBoxOctree				octree;
	idBounds				worldBounds;
	idClipModel				temporaryClipModel;
	idClipModel				defaultClipModel;
	mutable int				touchCount;
							// statistics
	int						numTranslations;
	int						numRotations;
	int						numMotions;
	int						numRenderModelTraces;
	int						numContents;
	int						numContacts;

private:
	void					ClipModelsTouchingBounds_r( const struct clipSector_s *node, struct listParms_s &parms ) const;
	const idTraceModel *	TraceModelForClipModel( const idClipModel *mdl ) const;
	int						GetTraceClipModels( const idBounds &bounds, int contentMask, const idEntity *passEntity, idClip_ClipModelList &clipModelList ) const;
	void					TraceRenderModel( trace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, const idMat3 &axis, idClipModel *touch ) const;

	void					FilterClipModels(const idEntity *passEntity, idClip_ClipModelList &clipModelList ) const;
	void					FilterEntities( idClip_EntityList &entityList, idClip_ClipModelList &clipModelList ) const;

	void					ClipModelsTouchingMovingBounds_r( const clipSector_s *node, idBounds &nodeBounds, listParmsMoving &parms ) const;
	int						ClipModelsTouchingMovingBounds( const idBounds &absBounds, const idBounds &stillBounds, const idVec3 &start, const idVec3 &end,
								int contentMask, idClip_ClipModelList &clipModelList, idClip_FloatList &fractionLowers ) const;
	int						GetTraceClipModels( const idBounds &absBounds, const idBounds &stillBounds, const idVec3 &start, const idVec3 &end,
								int contentMask, const idEntity *passEntity, idClip_ClipModelList &clipModelList, idClip_FloatList &fractionLowers ) const;
};


ID_INLINE bool idClip::TracePoint( trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity ) {
	Translation( results, start, end, NULL, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE bool idClip::TraceBounds( trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds, int contentMask, const idEntity *passEntity ) {
	temporaryClipModel.LoadModel( idTraceModel( bounds ) );
	Translation( results, start, end, &temporaryClipModel, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ID_INLINE const idBounds & idClip::GetWorldBounds( void ) const {
	return worldBounds;
}

ID_INLINE idClipModel *idClip::DefaultClipModel( void ) {
	return &defaultClipModel;
}

#endif /* !__CLIP_H__ */
