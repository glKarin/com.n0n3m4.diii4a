
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#define	MAX_SECTOR_DEPTH				12
#define MAX_SECTORS						((1<<(MAX_SECTOR_DEPTH+1))-1)

// RAVEN BEGIN
// ddynerman: SD's clip sector code
typedef struct clipSector_s {
	int						contents;
	int						dynamicContents;
	struct clipLink_s *		clipLinks;
} clipSector_t;
// RAVEN END

typedef struct clipLink_s {
	idClipModel *			clipModel;
	struct clipSector_s *	sector;
	struct clipLink_s *		prevInSector;
	struct clipLink_s *		nextInSector;
	struct clipLink_s *		nextLink;
} clipLink_t;

typedef struct trmCache_s {
	idTraceModel			trm;
	int						refCount;
	float					volume;
	idVec3					centerOfMass;
	idMat3					inertiaTensor;
	const idMaterial *		material;
	cmHandle_t		collisionModel;
	int						hash; // only used to identify non-hashed trm's in a save.
} trmCache_t;

idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );

// RAVEN BEGIN
// jnewquist: Mark memory tags for idBlockAlloc
idBlockAlloc<clipLink_t, 1024/*, MA_PHYSICS //k*/>	clipLinkAllocator;
// RAVEN END

typedef enum {
	CPT_NONE = -1,
	CPT_TRANSLATION,
	CPT_CONTACTS,
	CPT_CONTENTS,

	CPT_MAX_TYPES
} CLIP_PROFILE_TYPES;

#if 0

static idTimer				clipProfileTimer[ CPT_MAX_TYPES ];
static CLIP_PROFILE_TYPES	currentProfileType = CPT_NONE;
static int					clipProfileDepth = 0;

static void BeginClipProfile( CLIP_PROFILE_TYPES type ) {
	clipProfileDepth++;

	if ( clipProfileDepth == 1 ) {
		clipProfileTimer[ type ].Start();
		currentProfileType = type;
	}
}

static void EndClipProfile( CLIP_PROFILE_TYPES type ) {
	clipProfileDepth--;

	if ( clipProfileDepth == 0 ) {
		clipProfileTimer[ currentProfileType ].Stop();
		currentProfileType = CPT_NONE;
	}
}

void ClearClipProfile( void ) {
	for( int i = 0; i < CPT_MAX_TYPES; i++ ) {
		clipProfileTimer[ i ].Clear();
	}
}

void DisplayClipProfile( void ) {
	for( int i = 0; i < CPT_MAX_TYPES; i++ ) {
		common->Printf( "%d:%d  ", i, ( int )clipProfileTimer[ i ].Milliseconds() );
	}
	common->Printf( "\n" );
}
#else

#define BeginClipProfile( type )

#define EndClipProfile( type )

#endif


/*
===============================================================

	idClipModel trace model cache

===============================================================
*/

static idList<trmCache_s*>		traceModelCache;
static idHashIndex				traceModelHash;
	
/*
===============
idClipModel::ClearTraceModelCache
===============
*/
void idClipModel::ClearTraceModelCache( void ) {
	int i;

	for ( i = 0; i < traceModelCache.Num(); i++ ) {
// jmarshall
#if 1
		collisionModelManager->FreeModel( traceModelCache[i]->collisionModel );
#endif
// jmarshall
		traceModelCache[i]->collisionModel = 0;
	}
	traceModelCache.DeleteContents( true );
	traceModelHash.Free();
}

/*
===============
idClipModel::CacheCollisionModels
===============
*/
void idClipModel::CacheCollisionModels( void ) {
	int i;

	for ( i = 0; i < traceModelCache.Num(); i++ ) {
		if ( traceModelCache[i]->collisionModel <= 0 ) {
			traceModelCache[i]->collisionModel = collisionModelManager->ModelFromTrm( gameLocal.GetMapName(), va( "traceModel%d", i ), traceModelCache[i]->trm, traceModelCache[i]->material );
		}
	}
}

/*
===============
idClipModel::TraceModelCacheSize
===============
*/
int idClipModel::TraceModelCacheSize( void ) {
	return traceModelCache.Num() * sizeof( idTraceModel );
}

/*
===============
idClipModel::AllocTraceModel
===============
*/
int idClipModel::AllocTraceModel( const idTraceModel &trm, const idMaterial *material, bool notHashed ) {
	int i, hashKey, traceModelIndex;
	trmCache_t *entry;
	
	if ( notHashed ) {
		hashKey = 0xffffffff;
	} else {
		hashKey = GetTraceModelHashKey( trm );
		
		for ( i = traceModelHash.First( hashKey ); i >= 0; i = traceModelHash.Next( i ) ) {
			if ( traceModelCache[i]->trm == trm ) {
				traceModelCache[i]->refCount++;
				return i;
			}
		}
	}
	

	entry = new trmCache_t;
	entry->trm = trm;
	entry->trm.GetMassProperties( 1.0f, entry->volume, entry->centerOfMass, entry->inertiaTensor );
	entry->refCount = 1;
	entry->material = material;
	entry->hash = hashKey;
	traceModelIndex = traceModelCache.Append( entry );
	
	if ( !notHashed ) {
		traceModelHash.Add( hashKey, traceModelIndex );
	}

	entry->collisionModel = collisionModelManager->ModelFromTrm( gameLocal.GetMapName(), va( "traceModel%d", traceModelIndex ), trm, material );

	return traceModelIndex;
}

/*
===============
idClipModel::Replace
===============
*/
void idClipModel::ReplaceTraceModel( int index, const idTraceModel &trm, const idMaterial *material, bool notHashed ) {
	if ( !notHashed ) {
		common->Error( "ReplaceTraceModel was misused. Replace can only be used on non-hashed models right now.\n" );
		return;
	}
	
	trmCache_t *entry = traceModelCache[ index ];
	entry->trm = trm;
	entry->trm.GetMassProperties( 1.0f, entry->volume, entry->centerOfMass, entry->inertiaTensor );
	entry->refCount = 1;
	entry->hash = 0xffffffff;
	entry->material = material;

// jmarshall
#if 1
	if(entry->collisionModel)
	{
		collisionModelManager->FreeModel( entry->collisionModel );
	}
#endif
// jmarshall
	entry->collisionModel = collisionModelManager->ModelFromTrm( gameLocal.GetMapName(), va( "traceModel%d", index ), trm, material );
}

/*
===============
idClipModel::FreeTraceModel
===============
*/
void idClipModel::FreeTraceModel( int traceModelIndex ) {
	if ( traceModelIndex < 0 || traceModelIndex >= traceModelCache.Num() || traceModelCache[traceModelIndex]->refCount <= 0 ) {
		gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model" );
		return;
	}
	traceModelCache[traceModelIndex]->refCount--;
}

/*
===============
idClipModel::CopyTraceModel
===============
*/
int idClipModel::CopyTraceModel( const int traceModelIndex ) {
	if ( traceModelIndex < 0 || traceModelIndex >= traceModelCache.Num() || traceModelCache[traceModelIndex]->refCount <= 0 ) {
		gameLocal.Warning( "idClipModel::CopyTraceModel: tried to copy an uncached trace model" );
		return -1;
	}
	traceModelCache[traceModelIndex]->refCount++;
	return traceModelIndex;
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
idTraceModel *idClipModel::GetCachedTraceModel( int traceModelIndex ) {
	return &traceModelCache[traceModelIndex]->trm;
}

/*
===============
idClipModel::GetCachedTraceModel
===============
*/
cmHandle_t idClipModel::GetCachedCollisionModel( int traceModelIndex ) {
	return traceModelCache[traceModelIndex]->collisionModel;
}

/*
===============
idClipModel::GetTraceModelHashKey
===============
*/
int idClipModel::GetTraceModelHashKey( const idTraceModel &trm ) {
	const idVec3 &v = trm.bounds[0];
	return ( trm.type << 8 ) ^ ( trm.numVerts << 4 ) ^ ( trm.numEdges << 2 ) ^ ( trm.numPolys << 0 ) ^ idMath::FloatHash( v.ToFloatPtr(), v.GetDimension() );
}

/*
===============
idClipModel::SaveTraceModels
===============
*/
void idClipModel::SaveTraceModels( idSaveGame *savefile ) {
	int i;

	savefile->WriteInt( traceModelCache.Num() );
	for ( i = 0; i < traceModelCache.Num(); i++ ) {
		trmCache_t *entry = traceModelCache[i];

		savefile->Write( &entry->trm, sizeof( entry->trm ) );
		savefile->WriteFloat( entry->volume );
		savefile->WriteVec3( entry->centerOfMass );
		savefile->WriteMat3( entry->inertiaTensor );
		savefile->WriteMaterial( entry->material );
		savefile->WriteInt( entry->hash );
	}
}

/*
===============
idClipModel::RestoreTraceModels
===============
*/
void idClipModel::RestoreTraceModels( idRestoreGame *savefile ) {
	int i, num;

	ClearTraceModelCache();

	savefile->ReadInt( num );
	traceModelCache.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		trmCache_t *entry = new trmCache_t;

		savefile->Read( &entry->trm, sizeof( entry->trm ) );
		savefile->ReadFloat( entry->volume );
		savefile->ReadVec3( entry->centerOfMass );
		savefile->ReadMat3( entry->inertiaTensor );
		savefile->ReadMaterial( entry->material );
		savefile->ReadInt( entry->hash );

		entry->refCount = 0;
		entry->collisionModel = 0;

		traceModelCache[i] = entry;
		if ( entry->hash != 0xffffffff ) {
			traceModelHash.Add( GetTraceModelHashKey( entry->trm ), i );
		}
	}

	CacheCollisionModels();
}


/*
===============================================================

	idClipModel

===============================================================
*/

/*
================
idClipModel::FreeModel
================
*/
void idClipModel::FreeModel( void ) {

	if ( traceModelIndex != -1 ) {
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}

	if ( collisionModel > 0 ) {
// jmarshall
#if 1
		collisionModelManager->FreeModel( collisionModel );
#endif
// jmarshall
		collisionModel = 0;
	}

	renderModelHandle = -1;
}

// RAVEN BEGIN
// ddynerman: SD's clip sector code
/*
================
idClipModel::UpdateDynamicContents
================
*/
void idClipModel::UpdateDynamicContents( void ) {
	idClip::UpdateDynamicContents( this );
}
// RAVEN END

/*
================
idClipModel::LoadModel
================
*/
bool idClipModel::LoadModel( const char *name ) {
	FreeModel();
// jmarshall: added precache flag
#ifdef _QUAKE4
	collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), name, false );
#else
	collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), name );
#endif
// jmarshall end
	if ( collisionModel > 0 ) {
		collisionModelManager->GetModelBounds(collisionModel, bounds );
		collisionModelManager->GetModelContents(collisionModel, contents );
		return true;
	} else {
		bounds.Zero();
		return false;
	}
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const idTraceModel &trm, const idMaterial *material, bool notHashed ) {
	if ( !notHashed || traceModelIndex == -1 ) {
		FreeModel();
		traceModelIndex = AllocTraceModel( trm, material, notHashed );
	} else {
		ReplaceTraceModel( traceModelIndex, trm, material, notHashed );
	}
	
	bounds = trm.bounds;
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const int renderModelHandle ) {
	FreeModel();
	this->renderModelHandle = renderModelHandle;
	if ( renderModelHandle != -1 ) {
		const renderEntity_t *renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if ( renderEntity ) {
			bounds = renderEntity->bounds;
		}
	}
}

/*
================
idClipModel::Init
================
*/
void idClipModel::Init( void ) {
	enabled = true;
	entity = NULL;
	id = 0;
	owner = NULL;
	origin.Zero();
	axis.Identity();
	bounds.Zero();
	absBounds.Zero();
	contents = CONTENTS_BODY;
	collisionModel = 0;
	renderModelHandle = -1;
	traceModelIndex = -1;
	clipLinks = NULL;
	touchCount = -1;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	checked = false;
// RAVEN END
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( void ) {
	Init();
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const char *name ) {
	Init();
	LoadModel( name );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel &trm, const idMaterial *material ) {
	Init();
	LoadModel( trm, material );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const int renderModelHandle ) {
	Init();
	contents = CONTENTS_RENDERMODEL;
	LoadModel( renderModelHandle );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idClipModel *model ) {
	enabled = model->enabled;
	entity = model->entity;
	id = model->id;
	owner = model->owner;
	origin = model->origin;
	axis = model->axis;
	bounds = model->bounds;
	absBounds = model->absBounds;
	contents = model->contents;
	collisionModel = 0;
	if ( model->collisionModel > 0 ) {
// jmarshall - added precache flag
#ifdef _QUAKE4
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), collisionModelManager->GetModelName(model->collisionModel), false );
#else
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), model->collisionModel->GetName() );
#endif
// jmarshall end
	}
	traceModelIndex = -1;
	if ( model->traceModelIndex != -1 ) {
		traceModelIndex = CopyTraceModel( model->traceModelIndex );
	}
	renderModelHandle = model->renderModelHandle;
	clipLinks = NULL;
	touchCount = -1;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	checked = false;
// RAVEN END
}

/*
================
idClipModel::~idClipModel
================
*/
idClipModel::~idClipModel( void ) {
	// make sure the clip model is no longer linked
	Unlink();
	FreeModel();
}

/*
================
idClipModel::Save
================
*/
void idClipModel::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( enabled );
	savefile->WriteObject( entity );
	savefile->WriteInt( id );
	savefile->WriteObject( owner );
	savefile->WriteVec3( origin );
	savefile->WriteMat3( axis );
	savefile->WriteBounds( bounds );
	savefile->WriteBounds( absBounds );
	savefile->WriteInt( contents );
	if ( collisionModel > 0 ) {
		savefile->WriteString( collisionModelManager->GetModelName(collisionModel) );
	} else {
		savefile->WriteString( "" );
	}
	savefile->WriteInt( traceModelIndex );
	savefile->WriteInt( renderModelHandle );
	savefile->WriteBool( clipLinks != NULL );
	savefile->WriteInt( touchCount );

	savefile->WriteBool ( checked );	// cnicholson: Added unsaved var
}

/*
================
idClipModel::Restore
================
*/
void idClipModel::Restore( idRestoreGame *savefile ) {
	idStr collisionModelName;
	bool linked;

	savefile->ReadBool( enabled );
	savefile->ReadObject( reinterpret_cast<idClass *&>( entity ) );
	savefile->ReadInt( id );
	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	savefile->ReadVec3( origin );
	savefile->ReadMat3( axis );
	savefile->ReadBounds( bounds );
	savefile->ReadBounds( absBounds );
	savefile->ReadInt( contents );
	savefile->ReadString( collisionModelName );
	if ( collisionModelName.Length() ) {
// jmarshall - added precache flag
#ifdef _QUAKE4
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), collisionModelName, false );
#else
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), collisionModelName );
#endif
// jmarshall end
	} else {
		collisionModel = 0;
	}
	savefile->ReadInt( traceModelIndex );
	if ( traceModelIndex >= 0 ) {
		traceModelCache[traceModelIndex]->refCount++;
	}
	savefile->ReadInt( renderModelHandle );
	savefile->ReadBool( linked );
	savefile->ReadInt( touchCount );

	savefile->ReadBool ( checked );	// cnicholson: Added unrestored var

	// the render model will be set when the clip model is linked
	renderModelHandle = -1;
	clipLinks = NULL;
	touchCount = -1;

	if ( linked ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		Link( entity, id, origin, axis, renderModelHandle );
// RAVEN END
	}
}

/*
================
idClipModel::SetPosition
================
*/
void idClipModel::SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis ) {
	if ( clipLinks ) {
		Unlink();	// unlink from old position
	}
	origin = newOrigin;
	axis = newAxis;
}

/*
================
idClipModel::GetCollisionModel
================
*/
cmHandle_t  idClipModel::GetCollisionModel( void ) const {
	assert( renderModelHandle == -1 );
	if ( collisionModel > 0 ) {
		return collisionModel;
	} else if ( traceModelIndex != -1 ) {
		return GetCachedCollisionModel( traceModelIndex );
	} else {
		// this happens in multiplayer on the combat models
		if ( entity ) {
			gameLocal.Warning( "idClipModel::GetCollisionModel: clip model %d on '%s' (%x) is not a collision or trace model", id, entity->name.c_str(), entity->entityNumber );
		}
		return 0;
	}
}

/*
================
idClipModel::GetMassProperties
================
*/
void idClipModel::GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const {
	if ( traceModelIndex == -1 ) {
		gameLocal.Error( "idClipModel::GetMassProperties: clip model %d on '%s' is not a trace model\n", id, entity->name.c_str() );
	}

	trmCache_t *entry = traceModelCache[traceModelIndex];
	mass = entry->volume * density;
	centerOfMass = entry->centerOfMass;
	inertiaTensor = density * entry->inertiaTensor;
}

/*
===============
idClipModel::Unlink
===============
*/
void idClipModel::Unlink( void ) {
	clipLink_t *link;

	for ( link = clipLinks; link; link = clipLinks ) {
		clipLinks = link->nextLink;
		if ( link->prevInSector ) {
			link->prevInSector->nextInSector = link->nextInSector;
		} else {
			link->sector->clipLinks = link->nextInSector;
		}
		if ( link->nextInSector ) {
			link->nextInSector->prevInSector = link->prevInSector;
		}
// RAVEN BEGIN
// ddynerman: SD's clip sector code
		idClip::UpdateDynamicContents( link->sector );
// RAVEN END

		clipLinkAllocator.Free( link );
	}
}

/*
===============
idClipModel::Link
===============
*/
// RAVEN BEGIN
// ddynerman: multiple clip worlds
void idClipModel::Link( void ) {

	assert( idClipModel::entity );
	if ( !idClipModel::entity ) {
		return;
	}

	idClip* clp = gameLocal.GetEntityClipWorld( idClipModel::entity );

	if ( clipLinks ) {
		Unlink();	// unlink from old position
	}

	if ( bounds.IsCleared() ) {
		return;
	}

	// set the abs box
	if ( axis.IsRotated() ) {
		// expand for rotation
		absBounds.FromTransformedBounds( bounds, origin, axis );
	} else {
		// normal
		absBounds[0] = bounds[0] + origin;
		absBounds[1] = bounds[1] + origin;
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	absBounds[0] -= vec3_boxEpsilon;
	absBounds[1] += vec3_boxEpsilon;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	int coords[ 4 ];
	clp->CoordsForBounds( coords, absBounds );

	int x, y;
	for( x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {
			clipSector_t* sector = &clp->clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			sector->dynamicContents |= GetContents();

			clipLink_t* link = clipLinkAllocator.Alloc();
			link->clipModel = this;
			link->sector = sector;
			link->nextInSector = sector->clipLinks;
			link->prevInSector = NULL;
			if ( sector->clipLinks ) {
				sector->clipLinks->prevInSector = link;
			}
			sector->clipLinks = link;
			link->nextLink = clipLinks;
			clipLinks = link;
		}
	}
// RAVEN END
}

/*
===============
idClipModel::Link
===============
*/
// RAVEN BEGIN
// ddynerman: multiple clip worlds
void idClipModel::Link( idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int renderModelHandle ) {
	this->entity = ent;
	this->id = newId;
	this->origin = newOrigin;
	this->axis = newAxis;
	if ( renderModelHandle != -1 ) {
		this->renderModelHandle = renderModelHandle;
		const renderEntity_t *renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if ( renderEntity ) {
			this->bounds = renderEntity->bounds;
		}
	}
	this->Link();
}
// RAVEN END


/*
===============================================================

	idClip

===============================================================
*/

// RAVEN BEGIN
// ddynerman: change to static
idClipModel idClip::defaultClipModel;

idClipModel *idClip::DefaultClipModel( void ) {
	// initialize a default clip model
	if( defaultClipModel.traceModelIndex == -1 ) {
		defaultClipModel.LoadModel( idTraceModel( idBounds( idVec3( 0, 0, 0 ) ).Expand( 8 ) ), NULL );
	}

	return &defaultClipModel;
}

void idClip::FreeDefaultClipModel( void ) {
	if ( defaultClipModel.traceModelIndex != -1 ) {
		idClipModel::FreeTraceModel( defaultClipModel.traceModelIndex );
		defaultClipModel.traceModelIndex = -1;
	}
}
// RAVEN END

/*
===============
idClip::idClip
===============
*/
idClip::idClip( void ) {
	clipSectors = NULL;
	world = -1;
	worldBounds.Zero();
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::CreateClipSectors_r

Builds a uniformly subdivided tree for the given world size
===============
*/
clipSector_t *idClip::CreateClipSectors_r( const int depth, const idBounds &bounds, idVec3 &maxSector ) {
// RAVEN BEGIN
// ddynerman: SD's clip sector code
	if( clipSectors ) {
		delete[] clipSectors;
		clipSectors = NULL;
	}
	nodeOffsetVisual = bounds[ 0 ];

	int i;
	for( i = 0; i < 3; i++ ) {		
//jshepard: this crashes too often
#ifdef _DEBUG
		if( bounds[ 1 ][ i ] - bounds[ 0 ][ i ] )	{
			nodeScale[ i ] = depth / ( bounds[ 1 ][ i ] - bounds[ 0 ][ i ] );
		} else {
			gameLocal.Error("zero size bounds while creating clipsectors");
			nodeScale[ i ] = depth;
		}
		if( nodeScale[ i ] ) {	
			nodeOffset[ i ] = nodeOffsetVisual[ i ] + ( 0.5f / nodeScale[ i ] );
		} else {
			gameLocal.Error("zero size nodeScale while creating clipsectors");
			nodeOffset[ i] = nodeOffset[ i] + 0.5f;
		}
#else 
		nodeScale[ i ] = depth / ( bounds[ 1 ][ i ] - bounds[ 0 ][ i ] );
		nodeOffset[ i ] = nodeOffsetVisual[ i ] + ( 0.5f / nodeScale[ i ] );

#endif
	}

	clipSectors = new clipSector_t[ Square( depth ) ];
	memset( clipSectors, 0, Square( depth ) * sizeof( clipSector_t ) );
	return clipSectors;
// RAVEN END
}

/*
===============
idClip::Init
===============
*/
void idClip::Init( void ) {
	idVec3 size, maxSector = vec3_origin;

	// get world map bounds
// jmarshall - added precache flag
#ifdef _QUAKE4
	world = collisionModelManager->LoadModel( gameLocal.GetMapName(), WORLD_MODEL_NAME, false );
#else
	world = collisionModelManager->LoadModel( gameLocal.GetMapName(), WORLD_MODEL_NAME );
#endif
// jmarshall end
	collisionModelManager->GetModelBounds(world, worldBounds );

	// create world sectors
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// ddynerman: SD's clip sector code
	CreateClipSectors_r( CLIPSECTOR_WIDTH, worldBounds, maxSector );
	GetClipSectorsStaticContents();
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END

	size = worldBounds[1] - worldBounds[0];
	gameLocal.Printf( "map bounds are (%1.1f, %1.1f, %1.1f)\n", size[0], size[1], size[2] );
	gameLocal.Printf( "max clip sector is (%1.1f, %1.1f, %1.1f)\n", maxSector[0], maxSector[1], maxSector[2] );

	// set counters to zero
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Shutdown
===============
*/
void idClip::Shutdown( void ) {
	delete[] clipSectors;
	clipSectors = NULL;

	// free the trace model used for the temporaryClipModel
	if ( temporaryClipModel.traceModelIndex != -1 ) {
		idClipModel::FreeTraceModel( temporaryClipModel.traceModelIndex );
		temporaryClipModel.traceModelIndex = -1;
	}

	clipLinkAllocator.Shutdown();
}

/*
====================
idClip::ClipModelsTouchingBounds_r
====================
*/
typedef struct listParms_s {
	idBounds		bounds;
	int				contentMask;
	idClipModel	**	list;
	int				count;
	int				maxCount;
} listParms_t;


/*
================
idClip::ClipModelsTouchingBounds
================
*/
int idClip::ClipModelsTouchingBounds( const idBounds &bounds, int contentMask, idClipModel **clipModelList, int maxCount ) const {
	listParms_t parms;

// RAVEN BEGIN
// ddynerman: SD's clip sector code
	int clipCount = 0;
	static idClipModel* clipModels[ MAX_GENTITIES ];

	assert( maxCount <= MAX_GENTITIES );
	if( maxCount > MAX_GENTITIES ) {
		maxCount = MAX_GENTITIES;
	}
// RAVEN END

	if (	bounds[0][0] > bounds[1][0] ||
			bounds[0][1] > bounds[1][1] ||
			bounds[0][2] > bounds[1][2] ) {
		// we should not go through the tree for degenerate or backwards bounds
		assert( false );
		return 0;
	}

	parms.bounds[0] = bounds[0] - vec3_boxEpsilon;
	parms.bounds[1] = bounds[1] + vec3_boxEpsilon;
	parms.contentMask = contentMask;
	parms.list = clipModelList;
	parms.count = 0;
	parms.maxCount = maxCount;

// RAVEN BEGIN
// ddynerman: SD's clip sector code
	int coords[ 4 ];
	CoordsForBounds( coords, parms.bounds );

	int x, y;
	for( x = coords[ 0 ]; x < coords[ 2 ]; x++ ) {
		for( y = coords[ 1 ]; y < coords[ 3 ]; y++ ) {
			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			if( !( sector->dynamicContents & contentMask ) ) {
				continue;
			}

			for ( clipLink_t* link = sector->clipLinks; link && clipCount < MAX_GENTITIES; link = link->nextInSector ) {
				idClipModel* model = link->clipModel;

				if( model->checked || !model->enabled || !( model->GetContents() & contentMask ) ) {
					continue;
				}

				model->checked = true;
				clipModels[ clipCount++ ] = model;
			}
		}
	}

	for( x = 0; x < clipCount; x++ ) {
		clipModels[ x ]->checked = false;
	}

	for( x = 0; x < clipCount; x++ ) {
		idClipModel* model = clipModels[ x ];
		
		// if the bounds really do overlap
		if (	model->absBounds[0].x > parms.bounds[1].x ||
				model->absBounds[1].x < parms.bounds[0].x ||
				model->absBounds[0].y > parms.bounds[1].y ||
				model->absBounds[1].y < parms.bounds[0].y ||
				model->absBounds[0].z > parms.bounds[1].z ||
				model->absBounds[1].z < parms.bounds[0].z ) {
			continue;
		}

		if( parms.count >= parms.maxCount ) {
//			gameLocal.Warning( "idClip::ClipModelsTouchingBounds Max Count Hit\n" );
			break;
		}

		parms.list[ parms.count++ ] = model;
	}
// RAVEN END

	return parms.count;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount ) const {
	idClipModel *clipModelList[MAX_GENTITIES];
	int i, j, count, entCount;

	count = idClip::ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );
	entCount = 0;
	for ( i = 0; i < count; i++ ) {
		// entity could already be in the list because an entity can use multiple clip models
		for ( j = 0; j < entCount; j++ ) {
			if ( entityList[j] == clipModelList[i]->entity ) {
				break;
			}
		}
		if ( j >= entCount ) {
			if ( entCount >= maxCount ) {
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
				return entCount;
			}
			entityList[entCount] = clipModelList[i]->entity;
			entCount++;
		}
	}

	return entCount;
}

// RAVEN BEGIN
// ddynerman: MP helper function
/*
================
idClip::PlayersTouchingBounds
================
*/
int idClip::PlayersTouchingBounds( const idBounds &bounds, int contentMask, idPlayer **playerList, int maxCount ) const {
	idClipModel *clipModelList[MAX_GENTITIES];
	int i, j, count, playerCount;

	count = idClip::ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );
	playerCount = 0;
	for ( i = 0; i < count; i++ ) {
		// entity could already be in the list because an entity can use multiple clip models
		for ( j = 0; j < playerCount; j++ ) {
			if ( playerList[j] == clipModelList[i]->entity ) {
				break;
			}
		}
		if ( j >= playerCount ) {
			if ( playerCount >= maxCount ) {
				gameLocal.Warning( "idClip::EntitiesTouchingBounds: max count" );
				return playerCount;
			}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( clipModelList[i]->entity->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
				playerList[playerCount] = static_cast<idPlayer*>(clipModelList[i]->entity);
				playerCount++;
			}
		}
	}

	return playerCount;
}
// RAVEN END

// RAVEN BEGIN
// ddynerman: SD's clip sector code

/*
====================
idClip::DrawAreaClipSectors
====================
*/
void idClip::DrawAreaClipSectors( float range ) const {
	idClipModel* clipModels[ MAX_GENTITIES ];

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	idBounds bounds;
	bounds[0] = player->GetPhysics()->GetOrigin() - idVec3( range, range, range );
	bounds[1] = player->GetPhysics()->GetOrigin() + idVec3( range, range, range );

	int count = ClipModelsTouchingBounds( bounds, MASK_ALL, clipModels, MAX_GENTITIES );

	int i;
	for ( i = 0; i < count; i++ ) {
		idEntity* owner = clipModels[ i ]->GetEntity();

		const idVec3& org = clipModels[ i ]->GetOrigin();
		const idBounds& bounds = clipModels[ i ]->GetBounds();

		gameRenderWorld->DebugBounds( colorCyan, bounds, org );
		gameRenderWorld->DrawText( owner->GetClassname(), org, 0.5f, colorCyan, player->viewAngles.ToMat3(), 1 );
	}
}

/*
====================
idClip::DrawClipSectors
====================
*/
void idClip::DrawClipSectors( void ) const {
	idBounds bounds;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	int i;
	idVec3 inverseNodeScale;
	for( i = 0; i < 3; i++ ) {
		inverseNodeScale[ i ] = 1 / nodeScale[ i ];
	}

	const char* filter = g_showClipSectorFilter.GetString();
	idTypeInfo* type = idClass::GetClass( filter );

	int x;
	for( x = 0; x < CLIPSECTOR_WIDTH; x++ ) {
		int y;
		for( y = 0; y < CLIPSECTOR_WIDTH; y++ ) {
//			idWinding w( 4 );

			bounds[ 0 ].x = ( inverseNodeScale.x * x ) + nodeOffsetVisual.x + 1;
			bounds[ 0 ].y = ( inverseNodeScale.y * y ) + nodeOffsetVisual.y + 1;
			bounds[ 0 ].z = player->GetPhysics()->GetBounds()[0].z;

			bounds[ 1 ].x = ( inverseNodeScale.x * ( x + 1 ) ) + nodeOffsetVisual.x - 1;
			bounds[ 1 ].y = ( inverseNodeScale.y * ( y + 1 ) ) + nodeOffsetVisual.y - 1;
			bounds[ 1 ].z = player->GetPhysics()->GetBounds()[1].z;

			idVec3 point;
			point.x = ( bounds[ 0 ].x + bounds[ 1 ].x ) * 0.5f;
			point.y = ( bounds[ 0 ].y + bounds[ 1 ].y ) * 0.5f;
			point.z = 0.f;

/*			point.x = bounds[ 0 ].x;
			point.y = bounds[ 0 ].y;
			w.AddPoint( point );

			point.x = bounds[ 1 ].x;
			point.y = bounds[ 0 ].y;
			w.AddPoint( point );

			point.x = bounds[ 1 ].x;
			point.y = bounds[ 1 ].y;
			w.AddPoint( point );

			point.x = bounds[ 0 ].x;
			point.y = bounds[ 1 ].y;
			w.AddPoint( point );*/

			clipSector_t* sector = &clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ];

			clipLink_t* link = sector->clipLinks;
			while ( link ) {
				if ( type && !link->clipModel->GetEntity()->IsType( *type ) ) {
					link = link->nextInSector;
				} else {
					break;
				}
			}

			if( link ) {
				
				gameRenderWorld->DrawText( link->clipModel->GetEntity()->GetClassname(), point, 0.5f, colorCyan, player->viewAngles.ToMat3(), 1 );
				gameRenderWorld->DebugBounds( colorMagenta, bounds );
				gameRenderWorld->DebugBounds( colorYellow, link->clipModel->GetBounds(), link->clipModel->GetOrigin() );

			} else {

//				gameRenderWorld->DrawText( sector->clipLinks->clipModel->GetEntity()->GetClassname(), point, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );

			}
		}
	}
}

/*
====================
idClip::GetClipSectorsStaticContents
====================
*/
void idClip::GetClipSectorsStaticContents( void ) {
	idBounds bounds;

	bounds[ 0 ].x = 0;
	bounds[ 0 ].y = 0;
	bounds[ 0 ].z = worldBounds[ 0 ].z;

	bounds[ 1 ].x = 1 / nodeScale.x;
	bounds[ 1 ].y = 1 / nodeScale.y;
	bounds[ 1 ].z = worldBounds[ 1 ].z;

	idTraceModel* trm = new idTraceModel( bounds );

	idVec3 org;
	org.z = 0;

	int x;
	for( x = 0; x < CLIPSECTOR_WIDTH; x++ ) {
		int y;
		for( y = 0; y < CLIPSECTOR_WIDTH; y++ ) {
			org.x = ( x / nodeScale.x ) + nodeOffset.x;
			org.y = ( y / nodeScale.y ) + nodeOffset.y;

			int contents = collisionModelManager->Contents( org, trm, mat3_identity, -1, world, vec3_origin, mat3_default );
			clipSectors[ x + ( y << CLIPSECTOR_DEPTH ) ].contents = contents;
		}
	}
	// mwhitlock: Fix leak in SD's code.
	delete trm;
}

// RAVEN END

/*
====================
idClip::GetTraceClipModels

  an ent will be excluded from testing if:
  cm->entity == passEntity ( don't clip against the pass entity )
  cm->entity == passOwner ( missiles don't clip with owner )
  cm->owner == passEntity ( don't interact with your own missiles )
  cm->owner == passOwner ( don't interact with other missiles from same owner )
====================
*/
// RAVEN BEGIN
// nmckenzie: had to add a second pass entity so we can safely ignore both a guy and his target in some cases
int idClip::GetTraceClipModels( const idBounds &bounds, int contentMask, const idEntity *passEntity, idClipModel **clipModelList, const idEntity *passEntity2 ) const {
// RAVEN END
	int i, num;
	idClipModel	*cm;
	idEntity *passOwner;

	num = ClipModelsTouchingBounds( bounds, contentMask, clipModelList, MAX_GENTITIES );

	if ( !passEntity ) {
		return num;
	}

	if ( passEntity->GetPhysics()->GetNumClipModels() > 0 ) {
		passOwner = passEntity->GetPhysics()->GetClipModel()->GetOwner();
	} else {
		passOwner = NULL;
	}

	for ( i = 0; i < num; i++ ) {

		cm = clipModelList[i];

		// check if we should ignore this entity
		if ( cm->entity == passEntity ) {
			clipModelList[i] = NULL;			// don't clip against the pass entity
		}
// RAVEN BEGIN
// nmckenzie: we have cases where both a guy and his target need to be ignored by a translation
		else if ( cm->entity == passEntity2 ){
			clipModelList[i] = NULL;
// RAVEN END
		} else if ( cm->entity == passOwner ) {
			clipModelList[i] = NULL;			// missiles don't clip with their owner
		} else if ( cm->owner ) {
			if ( cm->owner == passEntity ) {
				clipModelList[i] = NULL;		// don't clip against own missiles
			} else if ( cm->owner == passOwner ) {
				clipModelList[i] = NULL;		// don't clip against other missiles from same owner
			}
		}
	}

	return num;
}

/*
============
idClip::TraceRenderModel
============
*/
void idClip::TraceRenderModel( trace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, const idMat3 &axis, idClipModel *touch ) const {
	trace.fraction = 1.0f;

	// if the trace is passing through the bounds
	if ( touch->absBounds.Expand( radius ).LineIntersection( start, end ) ) {
		modelTrace_t modelTrace;

		// test with exact render model and modify trace_t structure accordingly
		if ( gameRenderWorld->ModelTrace( modelTrace, touch->renderModelHandle, start, end, radius ) ) {
			trace.fraction = modelTrace.fraction;
			trace.endAxis = axis;
			trace.endpos = modelTrace.point;
			trace.c.normal = modelTrace.normal;
			trace.c.dist = modelTrace.point * modelTrace.normal;
			trace.c.point = modelTrace.point;
			trace.c.type = CONTACT_TRMVERTEX;
			trace.c.modelFeature = 0;
			trace.c.trmFeature = 0;
			trace.c.contents = modelTrace.material->GetContentFlags();
			trace.c.material = modelTrace.material;

// RAVEN BEGIN
// jscott: for material types
			trace.c.materialType = modelTrace.materialType;
// RAVEN END

			// NOTE: trace.c.id will be the joint number
			touch->id = JOINT_HANDLE_TO_CLIPMODEL_ID( modelTrace.jointNumber );
		}
	}
}

/*
============
idClip::TraceModelForClipModel
============
*/
const idTraceModel *idClip::TraceModelForClipModel( const idClipModel *mdl ) const {
	if ( !mdl ) {
		return NULL;
	} else {
		if ( !mdl->IsTraceModel() ) {
			if ( mdl->GetEntity() ) {
				gameLocal.Error( "TraceModelForClipModel: clip model %d on '%s' is not a trace model\n", mdl->GetId(), mdl->GetEntity()->name.c_str() );
			} else {
				gameLocal.Error( "TraceModelForClipModel: clip model %d is not a trace model\n", mdl->GetId() );
			}
		}
		return idClipModel::GetCachedTraceModel( mdl->traceModelIndex );
	}
}

/*
============
idClip::TestHugeTranslation
============
*/
ID_INLINE bool TestHugeTranslation( trace_t &results, const idClipModel *mdl, const idVec3 &start, const idVec3 &end, const idMat3 &trmAxis ) {
	if ( mdl != NULL && ( end - start ).LengthSqr() > Square( CM_MAX_TRACE_DIST ) ) {
		assert( 0 );

		results.fraction = 0.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		memset( &results.c, 0, sizeof( results.c ) );
		results.c.point = start;
		results.c.entityNum = ENTITYNUM_WORLD;

		if ( mdl->GetEntity() ) {
			gameLocal.Printf( "huge translation for clip model %d on entity %d '%s'\n", mdl->GetId(), mdl->GetEntity()->entityNumber, mdl->GetEntity()->GetName() );
		} else {
			gameLocal.Printf( "huge translation for clip model %d\n", mdl->GetId() );
		}
		return true;
	}
	return false;
}

/*
============
idClip::TranslationEntities
============
*/
// RAVEN BEGIN
// nmckenzie: had to add a second pass entity so we can safely ignore both a guy and his target in some cases
void idClip::TranslationEntities( trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 ) {
// RAVEN END
	int i, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel *trm;

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return;
	}

	trm = TraceModelForClipModel( mdl );

	results.fraction = 1.0f;
	results.endpos = end;
	results.endAxis = trmAxis;

	if ( !trm ) {
		traceBounds.FromPointTranslation( start, end - start );
		radius = 0.0f;
	} else {
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, end - start );
		radius = trm->bounds.GetRadius();
	}

// RAVEN BEGIN
// nmckenzie: had to add a second pass entity so we can safely ignore both a guy and his target in some cases
	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, passEntity2 );
// RAVEN END

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		if ( touch->renderModelHandle != -1 ) {
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		} else {
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
									touch->GetCollisionModel(), touch->origin, touch->axis );
		}

		if ( trace.fraction < results.fraction ) {
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if ( results.fraction == 0.0f ) {
				break;
			}
		}
	}
}

/*
============
idClip::Translation
============
*/
// RAVEN BEGIN
// nmckenzie: we have cases where both a guy and his target need to be ignored by a translation
bool idClip::Translation( trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 ) {
// RAVEN END
	BeginClipProfile( CPT_TRANSLATION );

	int i, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel *trm;

// RAVEN BEGIN
// rjohnson: added debug line drawing for traces
	if ( g_showCollisionTraces.GetInteger() >= 2 && !g_stopTime.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, start, end, 1000 );
	}
// RAVEN END

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		EndClipProfile( CPT_TRANSLATION );
		return true;
	}

	trm = TraceModelForClipModel( mdl );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numTranslations++;
		collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if ( results.fraction == 0.0f ) {
			EndClipProfile( CPT_TRANSLATION );
			return true;		// blocked immediately by the world
		}
	} else {
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = end;
		results.endAxis = trmAxis;
	}

	if ( !trm ) {
		traceBounds.FromPointTranslation( start, results.endpos - start );
		radius = 0.0f;
	} else {
		traceBounds.FromBoundsTranslation( trm->bounds, start, trmAxis, results.endpos - start );
		radius = trm->bounds.GetRadius();
	}

// RAVEN BEGIN
// nmckenzie: we have cases where both a guy and his target need to be ignored by a translation
	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList, passEntity2 );
// RAVEN END

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		if ( touch->renderModelHandle != -1 ) {
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		} else {
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
									touch->GetCollisionModel(), touch->origin, touch->axis );
		}

		if ( trace.fraction < results.fraction ) {
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;

// RAVEN BEGIN
// jscott: for material types
			results.c.materialType = trace.c.materialType;

// mekberg: copy contents
			if ( touch->IsTraceModel( ) ) {
				results.c.contents = touch->GetContents( );
			}
// RAVEN END

			if ( results.fraction == 0.0f ) {
				break;
			}
		}
	}

	EndClipProfile( CPT_TRANSLATION );

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Rotation
============
*/
bool idClip::Rotation( trace_t &results, const idVec3 &start, const idRotation &rotation,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	int i, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	trace_t trace;
	const idTraceModel *trm;

	trm = TraceModelForClipModel( mdl );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numRotations++;
		collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if ( results.fraction == 0.0f ) {
			return true;		// blocked immediately by the world
		}
	} else {
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis * rotation.ToMat3();
	}

	if ( !trm ) {
		traceBounds.FromPointRotation( start, rotation );
	} else {
		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		// no rotational collision with render models
		if ( touch->renderModelHandle != -1 ) {
			continue;
		}

		idClip::numRotations++;
		collisionModelManager->Rotation( &trace, start, rotation, trm, trmAxis, contentMask,
							touch->GetCollisionModel(), touch->origin, touch->axis );

		if ( trace.fraction < results.fraction ) {
			results = trace;
			results.c.entityNum = touch->entity->entityNumber;
			results.c.id = touch->id;
			if ( results.fraction == 0.0f ) {
				break;
			}
		}
	}

	return ( results.fraction < 1.0f );
}

/*
============
idClip::Motion
============
*/
bool idClip::Motion( trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	int i, num;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idVec3 dir, endPosition;
	idBounds traceBounds;
	float radius;
	trace_t translationalTrace, rotationalTrace, trace;
	idRotation endRotation;
	const idTraceModel *trm;

	assert( rotation.GetOrigin() == start );

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return true;
	}

	if ( mdl != NULL && rotation.GetAngle() != 0.0f && rotation.GetVec() != vec3_origin ) {
		// if no translation
		if ( start == end ) {
			// pure rotation
			return Rotation( results, start, rotation, mdl, trmAxis, contentMask, passEntity );
		}
	} else if ( start != end ) {
		// pure translation
		return Translation( results, start, end, mdl, trmAxis, contentMask, passEntity );
	} else {
		// no motion
		results.fraction = 1.0f;
		results.endpos = start;
		results.endAxis = trmAxis;
		return false;
	}

	trm = TraceModelForClipModel( mdl );

	radius = trm->bounds.GetRadius();

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// translational collision with world
		idClip::numTranslations++;
		collisionModelManager->Translation( &translationalTrace, start, end, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
		translationalTrace.c.entityNum = translationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	} else {
		memset( &translationalTrace, 0, sizeof( translationalTrace ) );
		translationalTrace.fraction = 1.0f;
		translationalTrace.endpos = end;
		translationalTrace.endAxis = trmAxis;
	}

	if ( translationalTrace.fraction != 0.0f ) {

		traceBounds.FromBoundsRotation( trm->bounds, start, trmAxis, rotation );
		dir = translationalTrace.endpos - start;
		for ( i = 0; i < 3; i++ ) {
			if ( dir[i] < 0.0f ) {
				traceBounds[0][i] += dir[i];
			}
			else {
				traceBounds[1][i] += dir[i];
			}
		}

		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

		for ( i = 0; i < num; i++ ) {
			touch = clipModelList[i];

			if ( !touch ) {
				continue;
			}

			if ( touch->renderModelHandle != -1 ) {
				idClip::numRenderModelTraces++;
				TraceRenderModel( trace, start, end, radius, trmAxis, touch );
			} else {
				idClip::numTranslations++;
				collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
										touch->GetCollisionModel(), touch->origin, touch->axis );
			}

			if ( trace.fraction < translationalTrace.fraction ) {
				translationalTrace = trace;
				translationalTrace.c.entityNum = touch->entity->entityNumber;
				translationalTrace.c.id = touch->id;
				if ( translationalTrace.fraction == 0.0f ) {
					break;
				}
			}
		}
	} else {
		num = -1;
	}

	endPosition = translationalTrace.endpos;
	endRotation = rotation;
	endRotation.SetOrigin( endPosition );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// rotational collision with world
		idClip::numRotations++;
		collisionModelManager->Rotation( &rotationalTrace, endPosition, endRotation, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
		rotationalTrace.c.entityNum = rotationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	} else {
		memset( &rotationalTrace, 0, sizeof( rotationalTrace ) );
		rotationalTrace.fraction = 1.0f;
		rotationalTrace.endpos = endPosition;
		rotationalTrace.endAxis = trmAxis * rotation.ToMat3();
	}

	if ( rotationalTrace.fraction != 0.0f ) {

		if ( num == -1 ) {
			traceBounds.FromBoundsRotation( trm->bounds, endPosition, trmAxis, endRotation );
			num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );
		}

		for ( i = 0; i < num; i++ ) {
			touch = clipModelList[i];

			if ( !touch ) {
				continue;
			}

			// no rotational collision detection with render models
			if ( touch->renderModelHandle != -1 ) {
				continue;
			}

			idClip::numRotations++;
			collisionModelManager->Rotation( &trace, endPosition, endRotation, trm, trmAxis, contentMask,
								touch->GetCollisionModel(), touch->origin, touch->axis );

			if ( trace.fraction < rotationalTrace.fraction ) {
				rotationalTrace = trace;
				rotationalTrace.c.entityNum = touch->entity->entityNumber;
				rotationalTrace.c.id = touch->id;
				if ( rotationalTrace.fraction == 0.0f ) {
					break;
				}
			}
		}
	}

	if ( rotationalTrace.fraction < 1.0f ) {
		results = rotationalTrace;
	} else {
		results = translationalTrace;
		results.endAxis = rotationalTrace.endAxis;
	}

	results.fraction = Max( translationalTrace.fraction, rotationalTrace.fraction );

	return ( translationalTrace.fraction < 1.0f || rotationalTrace.fraction < 1.0f );
}

/*
============
idClip::Contacts
============
*/
int idClip::Contacts( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
					 const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	BeginClipProfile( CPT_CONTACTS );

	int i, j, num, n, numContacts;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel *trm;

	trm = TraceModelForClipModel( mdl );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numContacts++;
		numContacts = collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
	} else {
		numContacts = 0;
	}

	for ( i = 0; i < numContacts; i++ ) {
		contacts[i].entityNum = ENTITYNUM_WORLD;
		contacts[i].id = 0;
	}

	if ( numContacts >= maxContacts ) {
		EndClipProfile( CPT_CONTACTS );
		return numContacts;
	}

	if ( !trm ) {
		traceBounds = idBounds( start ).Expand( depth );
	} else {
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
		traceBounds.ExpandSelf( depth );
	}

	num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		// no contacts with render models
		if ( touch->renderModelHandle != -1 ) {
			continue;
		}

		idClip::numContacts++;
		n = collisionModelManager->Contacts( contacts + numContacts, maxContacts - numContacts,
								start, dir, depth, trm, trmAxis, contentMask,
									touch->GetCollisionModel(), touch->origin, touch->axis );

		for ( j = 0; j < n; j++ ) {
			contacts[numContacts].entityNum = touch->entity->entityNumber;
			contacts[numContacts].id = touch->id;
			numContacts++;
		}

		if ( numContacts >= maxContacts ) {
			break;
		}
	}

	EndClipProfile( CPT_CONTACTS );

	return numContacts;
}

/*
============
idClip::Contents
============
*/
// RAVEN BEGIN
// AReis: Added ability to get the entity that was touched as well.
int idClip::Contents( const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, idEntity **touchedEntity ) {
// RAVEN END
	BeginClipProfile( CPT_CONTENTS );

	int i, num, contents;
	idClipModel *touch, *clipModelList[MAX_GENTITIES];
	idBounds traceBounds;
	const idTraceModel *trm;

	trm = TraceModelForClipModel( mdl );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numContents++;
		contents = collisionModelManager->Contents( start, trm, trmAxis, contentMask, world, vec3_origin, mat3_default );
	} else {
		contents = 0;
	}

	if ( !trm ) {
		traceBounds[0] = start;
		traceBounds[1] = start;
	} else if ( trmAxis.IsRotated() ) {
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
	} else {
		traceBounds[0] = trm->bounds[0] + start;
		traceBounds[1] = trm->bounds[1] + start;
	}

	num = GetTraceClipModels( traceBounds, -1, passEntity, clipModelList );

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		// no contents test with render models
		if ( touch->renderModelHandle != -1 ) {
			continue;
		}

		// if the entity does not have any contents we are looking for
		if ( ( touch->contents & contentMask ) == 0 ) {
			continue;
		}

		// if the entity has no new contents flags
		if ( ( touch->contents & contents ) == touch->contents ) {
			continue;
		}

		idClip::numContents++;
		if ( collisionModelManager->Contents( start, trm, trmAxis, contentMask, touch->GetCollisionModel(), touch->origin, touch->axis ) ) {
			contents |= ( touch->contents & contentMask );
		}

// RAVEN BEGIN
		// Only sends back one entity for now. Ahh well, if this is a problem, have it send back a list...
		if ( touchedEntity )
		{
			*touchedEntity = touch->GetEntity();
		}
// RAVEN END
	}

	EndClipProfile( CPT_CONTENTS );

	return contents;
}

/*
============
idClip::TranslationModel
============
*/
void idClip::TranslationModel( trace_t &results, const idVec3 &start, const idVec3 &end,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
					cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	const idTraceModel *trm = TraceModelForClipModel( mdl );
	idClip::numTranslations++;
	collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::RotationModel
============
*/
void idClip::RotationModel( trace_t &results, const idVec3 &start, const idRotation &rotation,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
					cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	const idTraceModel *trm = TraceModelForClipModel( mdl );
	idClip::numRotations++;
	collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContactsModel
============
*/
int idClip::ContactsModel( contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
					cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	const idTraceModel *trm = TraceModelForClipModel( mdl );
	idClip::numContacts++;
	return collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::ContentsModel
============
*/
int idClip::ContentsModel( const idVec3 &start,
					const idClipModel *mdl, const idMat3 &trmAxis, int contentMask,
					cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	const idTraceModel *trm = TraceModelForClipModel( mdl );
	idClip::numContents++;
	return collisionModelManager->Contents( start, trm, trmAxis, contentMask, model, modelOrigin, modelAxis );
}

/*
============
idClip::GetModelContactFeature
============
*/
bool idClip::GetModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const {
	int i;
	cmHandle_t model;
	idVec3 start, end;

	model = -1;
	winding.Clear();

	if ( clipModel == NULL ) {
		model = 0;
	} else {
		if ( clipModel->renderModelHandle != -1 ) {
			winding += contact.point;
			return true;
		} else if ( clipModel->traceModelIndex != -1 ) {
			model = idClipModel::GetCachedCollisionModel( clipModel->traceModelIndex );
		} else {
			model = clipModel->collisionModel;
		}
	}

	// if contact with a collision model
	if ( model >= 0 ) {
		switch( contact.type ) {
			case CONTACT_EDGE: {
				// the model contact feature is a collision model edge
				collisionModelManager->GetModelEdge(model, contact.modelFeature, start, end );
				winding += start;
				winding += end;
				break;
			}
			case CONTACT_MODELVERTEX: {
				// the model contact feature is a collision model vertex
				collisionModelManager->GetModelVertex(model, contact.modelFeature, start );
				winding += start;
				break;
			}
			case CONTACT_TRMVERTEX: {
				// the model contact feature is a collision model polygon
				collisionModelManager->GetModelPolygon(model, contact.modelFeature, winding );
				break;
			}
		}
	}

	// transform the winding to world space
	if ( clipModel ) {
		for ( i = 0; i < winding.GetNumPoints(); i++ ) {
			winding[i].ToVec3() *= clipModel->axis;
			winding[i].ToVec3() += clipModel->origin;
		}
	}

	return true;
}

/*
============
idClip::PrintStatistics
============
*/
void idClip::PrintStatistics( void ) {
// RAVEN BEGIN
// rjohnson: added trace model cache size
	gameLocal.Printf( "t=%-3d, r=%-3d, m=%-3d, render=%-3d, contents=%-3d, contacts=%-3d, cache=%d\n",
						numTranslations, numRotations, numMotions, numRenderModelTraces, numContents, numContacts, traceModelCache.Num() * sizeof( idTraceModel ) );
// RAVEN END
}

/*
============
idClip::DrawClipModels
============
*/
void idClip::DrawClipModels( const idVec3 &eye, const float radius, const idEntity *passEntity, const idTypeInfo* type ) {
	int				i, num;
	idBounds		bounds;
	idClipModel		*clipModelList[MAX_GENTITIES];
	idClipModel		*clipModel;

	bounds = idBounds( eye ).Expand( radius );

	num = idClip::ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		clipModel = clipModelList[i];
		if ( clipModel->GetEntity() == passEntity ) {
			continue;
		}
		if ( type && !clipModel->GetEntity()->IsType( *type ) ) {
			continue;
		}
		if ( clipModel->renderModelHandle != -1 ) {
			gameRenderWorld->DebugBounds( colorCyan, clipModel->GetAbsBounds() );
			continue;
		}

		cmHandle_t model = clipModel->GetCollisionModel();
		if ( model >= 0 ) {
			collisionModelManager->DrawModel( model, clipModel->GetOrigin(), clipModel->GetAxis(), eye, mat3_identity, radius );
		}
	}
}

/*
============
idClip::DrawModelContactFeature
============
*/
bool idClip::DrawModelContactFeature( const contactInfo_t &contact, const idClipModel *clipModel, int lifetime ) const {
	int i;
	idMat3 axis;
	idFixedWinding winding;

	if ( !GetModelContactFeature( contact, clipModel, winding ) ) {
		return false;
	}

	axis = contact.normal.ToMat3();

	if ( winding.GetNumPoints() == 1 ) {
		gameRenderWorld->DebugLine( colorCyan, winding[0].ToVec3(), winding[0].ToVec3() + 2.0f * axis[0], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[1], winding[0].ToVec3() + 1.0f * axis[1], lifetime );
		gameRenderWorld->DebugLine( colorWhite, winding[0].ToVec3() - 1.0f * axis[2], winding[0].ToVec3() + 1.0f * axis[2], lifetime );
	} else {
		for ( i = 0; i < winding.GetNumPoints(); i++ ) {
			gameRenderWorld->DebugLine( colorCyan, winding[i].ToVec3(), winding[(i+1)%winding.GetNumPoints()].ToVec3(), lifetime );
		}
	}

	axis[0] = -axis[0];
	axis[2] = -axis[2];
	gameRenderWorld->DrawText( contact.material->GetName(), winding.GetCenter() - 4.0f * axis[2], 0.1f, colorWhite, axis, 1, 5000 );

	return true;
}

// RAVEN BEGIN
// rjohnson: added debug hud support

void idClip::DebugHudStatistics( void )
{
	gameDebug.SetInt( "physics_translations", numTranslations );
	gameDebug.SetInt( "physics_rotations", numRotations );
	gameDebug.SetInt( "physics_motions", numMotions );
	gameDebug.SetInt( "physics_render_model_traces", numRenderModelTraces );
	gameDebug.SetInt( "physics_contents", numContents );
	gameDebug.SetInt( "physics_contacts", numContacts );
}

void idClip::ClearStatistics( void )
{
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

// RAVEN END

// RAVEN BEGIN
// ddynerman: SD's clip sector code
/*
============
idClip::UpdateDynamicContents
============
*/
void idClip::UpdateDynamicContents( clipSector_t* sector ) {
	sector->dynamicContents = 0;

	clipLink_t* link;
	for( link = sector->clipLinks; link; link = link->nextInSector ) {
		sector->dynamicContents |= link->clipModel->GetContents();
	}
}

/*
============
idClip::UpdateDynamicContents
============
*/
void idClip::UpdateDynamicContents( idClipModel* clipModel ) {
	clipLink_s* link = clipModel->clipLinks;
	while ( link ) {
		idClip::UpdateDynamicContents( link->sector );
		link = link->nextLink;
	}
}
// RAVEN END

#ifdef _QUAKE4
/*
============
idClip::PointContents
============
*/
int idClip::PointContents(const idVec3 p)
{
	int contents = -1;

	contents = collisionModelManager->PointContents(p, world);
	if (contents > 0)
	{
		return contents;
	}

	//for (int i = 0; i < collisionModelManager->GetNumInlinedProcClipModels(); i++)
	//{
	//	idCollisionModel* cm = collisionModelManager->GetCollisionModel(i + 1);
	//	if (cm == NULL)
	//		continue;
	//
	//	contents = collisionModelManager->PointContents(p, cm);
	//}

	return contents;
}
#endif

