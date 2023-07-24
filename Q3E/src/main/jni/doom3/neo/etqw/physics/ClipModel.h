// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __CLIPMODEL_H__
#define __CLIPMODEL_H__

/*
===============================================================

	Collision Geometry Container.

===============================================================
*/

class idClip;
class idEntity;

struct traceModelWater_t;

class idClipModel : public idClass {
public:
							CLASS_PROTOTYPE( idClipModel );

	friend class			idClip;

							idClipModel( void );
							explicit idClipModel( const char *name );
							explicit idClipModel( idCollisionModel *model );
							explicit idClipModel( const idTraceModel &trm, bool includeBrushes );
							explicit idClipModel( qhandle_t renderEntity );
							explicit idClipModel( const idClipModel *model );

							// FIXME: make this private
	void					LoadTraceModel( const idTraceModel &trm, bool includeBrushes );

	void					Draw( const idVec3& origin, const idMat3& axis, float radius = 0.f, int lifetime = 0.f ) const;
	void					Draw( float radius = 0.f ) const;

	bool					CheckCoords( idClip &clp );
	void					Link( idClip &clp );						// must have been linked with an entity and id before
	void					Link( idClip &clp, idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, qhandle_t renderEntity = -1 );
	void					Unlink( idClip &clp );						// unlink from sectors

	void					SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis, idClip &clp );	// unlinks the clip model
	void					Translate( const idVec3 &translation, idClip &clp );						// unlinks the clip model
	void					Rotate( const idRotation &rotation, idClip &clp );							// unlinks the clip model

	const idVec3 &			GetOrigin( void ) const;					// position of clip model
	const idMat3 &			GetAxis( void ) const;						// orientation of clip model
	const idBounds &		GetBounds( void ) const;					// bounds relative to origin
	const idBounds &		GetAbsBounds( void ) const;					// absolute bounds

	void					SetContents( int newContents );				// override contents
	int						GetContents( void ) const;
	void					SetEntity( idEntity *newEntity );
	idEntity *				GetEntity( void ) const;
	int						GetEntityNumber( void ) const;
	void					SetId( int newId );
	int						GetId( void ) const;
	int						GetRealContents( void ) const { return backupContents; }

	bool					IsTraceModel( void ) const;					// returns true if this clip model contains one or more trace models
	bool					IsRenderModel( void ) const;				// returns true if this clip model contains a render model
	bool					IsLinked( void ) const;						// returns true if this clip model is linked
	bool					IsEqual( const idTraceModel &trm ) const;	// returns true if this clip model contains the given trace model

	int						GetNumCollisionModels( void ) const;		// number of contained collision models
	idCollisionModel *		GetCollisionModel( int index = 0 ) const;	// returns one of the contained collision models
	int						GetNumTraceModels( void ) const;			// number of contained trace models
	const idTraceModel *	GetTraceModel( int index = 0 ) const;		// returns one of the contained trace models
	const traceModelWater_t*GetWaterPoints( int index = 0 ) const;
	float					GetTraceModelVolume( int index = 0 ) const;
	int						GetNumRenderEntities( void ) const;			// number of contained render entities
	qhandle_t				GetRenderEntity( int index = 0 ) const;		// returns one of the contained render entities

	void					GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const;

	void					SetMaterial( const idMaterial *m );
	const idMaterial *		GetMaterial( void ) const;

	void					Enable( void ) { contents = backupContents; }
	void					Disable( void ) { contents = 0; }

private:
	idEntity *				entity;					// entity using this clip model
	int						entityNumber;			// number of the entity for use from other threads
	int						id;						// id for entities that use multiple clip models
	idVec3					origin;					// origin of clip model
	idMat3					axis;					// orientation of clip model
	idBounds				bounds;					// bounds
	idBounds				absBounds;				// absolute bounds
	const idMaterial *		material;				// material for trace models
	int						contents;				// all contents ored together
	int						backupContents;			//
	idCollisionModel *		collisionModel;			// collision model
	idList<int>				traceModels;			// trace models
	qhandle_t				renderEntity;			// render entity
	struct clipLink_t *		clipLinks;				// links into sectors
	int						lastLinkCoords[ 4 ];

	idClipModel *			nextDeleted;
	int						deleteThreadCount;

public:

	unsigned int			lastMailBox;

private:
							~idClipModel( void );

	bool					LoadCollisionModel( const char *name );
	void					LoadCollisionModel( idCollisionModel *model );
	void					LoadRenderModel( qhandle_t renderEntity );

	const char *			GetEntityName( void ) const;

	void					Init( void );			// initialize
	void					FreeModel( void );		// free any collision or trace models
	void					Link_r( struct clipSector_t *node );
};


ID_INLINE void idClipModel::Translate( const idVec3 &translation, idClip &clp ) {
	Unlink( clp );
	origin += translation;
}

ID_INLINE void idClipModel::Rotate( const idRotation &rotation, idClip &clp ) {
	Unlink( clp );
	origin *= rotation;
	axis *= rotation.ToMat3();
}

ID_INLINE const idVec3 &idClipModel::GetOrigin( void ) const {
	return origin;
}

ID_INLINE const idMat3 &idClipModel::GetAxis( void ) const {
	return axis;
}

ID_INLINE const idBounds &idClipModel::GetBounds( void ) const {
	return bounds;
}

ID_INLINE const idBounds &idClipModel::GetAbsBounds( void ) const {
	return absBounds;
}

ID_INLINE void idClipModel::SetMaterial( const idMaterial *m ) {
	material = m;
}

ID_INLINE const idMaterial * idClipModel::GetMaterial( void ) const {
	return material;
}

ID_INLINE void idClipModel::SetContents( int newContents ) {
	backupContents = contents = newContents;	
}

ID_INLINE int idClipModel::GetContents( void ) const {
	return contents;
}

ID_INLINE idEntity *idClipModel::GetEntity( void ) const {
	return entity;
}

ID_INLINE int idClipModel::GetEntityNumber( void ) const {
	return entityNumber;
}

ID_INLINE void idClipModel::SetId( int newId ) {
	id = newId;
}

ID_INLINE int idClipModel::GetId( void ) const {
	return id;
}

ID_INLINE bool idClipModel::IsRenderModel( void ) const {
	return ( renderEntity != 0 );
}

ID_INLINE bool idClipModel::IsTraceModel( void ) const {
	return ( traceModels.Num() != 0 );
}

ID_INLINE bool idClipModel::IsLinked( void ) const {
	return ( clipLinks != NULL );
}

ID_INLINE int idClipModel::GetNumCollisionModels( void ) const {
	if ( collisionModel != NULL ) {
		return 1;
	} else {
		return traceModels.Num();
	}
}

ID_INLINE int idClipModel::GetNumTraceModels( void ) const {
	return traceModels.Num();
}

ID_INLINE int idClipModel::GetNumRenderEntities( void ) const {
	if ( renderEntity != 0 ) {
		return 1;
	} else {
		return 0;
	}
}

ID_INLINE qhandle_t idClipModel::GetRenderEntity( int index ) const {
	return renderEntity;
}

#endif /* !__CLIPMODEL_H__ */
