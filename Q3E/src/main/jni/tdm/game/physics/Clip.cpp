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

#include "precompiled.h"
#pragma hdrstop



#include "math/Line.h"
#include "../Game_local.h"

//stgatilov: record some information into trace events for idClip calls
//unfortunately, it adds considerable overhead time, so is disabled by default
#define TRACE_CLIP_INFO 0

typedef struct trmCache_s {
	idTraceModel			trm;
	int						refCount;
	float					volume;
	idVec3					centerOfMass;
	idMat3					inertiaTensor;
} trmCache_t;

idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );



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
	traceModelCache.DeleteContents( true );
	traceModelHash.ClearFree();
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
int idClipModel::AllocTraceModel( const idTraceModel &trm ) {
	int i, hashKey, traceModelIndex;
	trmCache_t *entry;

	hashKey = GetTraceModelHashKey( trm );
	for ( i = traceModelHash.First( hashKey ); i >= 0; i = traceModelHash.Next( i ) ) {
		if ( traceModelCache[i]->trm == trm ) {
			traceModelCache[i]->refCount++;
			return i;
		}
	}

	entry = new trmCache_t;
	entry->trm = trm;

	// If density is 1 the volume has the same size as the mass (m = d*v). The calling code wants to know the volume,
	// and with density equal to 1 it's allowed to use the mass value returned by idTraceModel::GetMassProperties().
	// That's what's happening here.
	entry->trm.GetMassProperties( 1.0f, entry->volume, entry->centerOfMass, entry->inertiaTensor );
	entry->refCount = 1;

	traceModelIndex = traceModelCache.Append( entry );
	traceModelHash.Add( hashKey, traceModelIndex );
	return traceModelIndex;
}

/*
===============
idClipModel::FreeTraceModel
===============
*/
void idClipModel::FreeTraceModel( const int traceModelIndex ) {
	if ( traceModelIndex < 0 || traceModelIndex >= traceModelCache.Num() ) {
		gameLocal.Warning( "idClipModel::FreeTraceModel: traceModelIndex %i out of range (0..%i)", traceModelIndex, traceModelCache.Num() );
		return;
	}
	if ( traceModelCache[traceModelIndex]->refCount <= 0 ) {
		gameLocal.Warning( "idClipModel::FreeTraceModel: tried to free uncached trace model (index=%i)", traceModelIndex );
		return;
	}
	traceModelCache[traceModelIndex]->refCount--;
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
		
		savefile->WriteTraceModel( entry->trm );
		savefile->WriteFloat( entry->volume );
		savefile->WriteVec3( entry->centerOfMass );
		savefile->WriteMat3( entry->inertiaTensor );
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
		
		savefile->ReadTraceModel( entry->trm );

		savefile->ReadFloat( entry->volume );
		savefile->ReadVec3( entry->centerOfMass );
		savefile->ReadMat3( entry->inertiaTensor );
		entry->refCount = 0;

		traceModelCache[i] = entry;
		traceModelHash.Add( GetTraceModelHashKey( entry->trm ), i );
	}
}


/*
===============================================================

	idClipModel

===============================================================
*/

/*
================
idClipModel::LoadModel
================
*/
bool idClipModel::LoadModel( const char *name ) {
	return LoadModel( name, (const idDeclSkin*)NULL );
}

/*
================
idClipModel::LoadModel
================
*/
bool idClipModel::LoadModel( const char *name, const idDeclSkin* skin ) 
{
	renderModelHandle = -1;
	if ( traceModelIndex != -1 ) {
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
	}
	collisionModelHandle = collisionModelManager->LoadModel( name, false, skin );
	if ( collisionModelHandle >= 0 ) {
		collisionModelManager->GetModelBounds( collisionModelHandle, bounds );
		collisionModelManager->GetModelContents( collisionModelHandle, contents );
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
void idClipModel::LoadModel( const idTraceModel &trm ) {
	collisionModelHandle = 0;
	renderModelHandle = -1;
	if ( traceModelIndex != -1 ) {
		FreeTraceModel( traceModelIndex );
	}
	traceModelIndex = AllocTraceModel( trm );
	bounds = trm.bounds;
}

/*
================
idClipModel::LoadModel
================
*/
void idClipModel::LoadModel( const int renderModelHandle ) {
	collisionModelHandle = 0;
	this->renderModelHandle = renderModelHandle;
	if ( renderModelHandle != -1 ) {
		const renderEntity_t *renderEntity = gameRenderWorld->GetRenderEntity( renderModelHandle );
		if ( renderEntity ) {
			bounds = renderEntity->bounds;
		}
	}
	if ( traceModelIndex != -1 ) {
		FreeTraceModel( traceModelIndex );
		traceModelIndex = -1;
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
	material = NULL;
	contents = CONTENTS_BODY;
	collisionModelHandle = 0;
	renderModelHandle = -1;
	traceModelIndex = -1;
	touchCount = -1;
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
idClipModel::idClipModel( const char *name, const idDeclSkin* skin ) {
	Init();
	LoadModel( name, skin );
}


/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel &trm ) {
	Init();
	LoadModel( trm );
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
	material = model->material;
	contents = model->contents;
	collisionModelHandle = model->collisionModelHandle;
	traceModelIndex = -1;
	if ( model->traceModelIndex != -1 ) {
		LoadModel( *GetCachedTraceModel( model->traceModelIndex ) );
	}
	renderModelHandle = model->renderModelHandle;
	touchCount = -1;
}

/*
================
idClipModel::~idClipModel
================
*/
idClipModel::~idClipModel( void ) {
	// make sure the clip model is no longer linked
	Unlink();
	if ( traceModelIndex != -1 ) {
		FreeTraceModel( traceModelIndex );
	}
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
	savefile->WriteMaterial( material );
	savefile->WriteInt( contents );
	if ( collisionModelHandle >= 0 ) {
		savefile->WriteString( collisionModelManager->GetModelName( collisionModelHandle ) );
	} else {
		savefile->WriteString( "" );
	}
	savefile->WriteInt( traceModelIndex );
	savefile->WriteBool( IsLinked() );
	savefile->WriteInt( touchCount );
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
	savefile->ReadMaterial( material );
	savefile->ReadInt( contents );
	savefile->ReadString( collisionModelName );
	if ( collisionModelName.Length() ) {
		// Cater for skinned models with name in format "modelName skinName" delimiter is chr(1) #4232
		const int splitPos = idStr::FindChar(collisionModelName, '\1');
		if ( splitPos != -1 ) {
			const idDeclSkin* skin = declManager->FindSkin( collisionModelName.c_str() + splitPos + 1, false );
			collisionModelHandle = collisionModelManager->LoadModel( idStr(collisionModelName.c_str(), 0, splitPos), false, skin );
		} else {
			collisionModelHandle = collisionModelManager->LoadModel( collisionModelName, false );
		}
	} else {
		collisionModelHandle = -1;
	}
	savefile->ReadInt( traceModelIndex );
	if ( traceModelIndex >= 0 ) {
		traceModelCache[traceModelIndex]->refCount++;
	}
	savefile->ReadBool( linked );
	savefile->ReadInt( touchCount );

	// the render model will be set when the clip model is linked, so do not restore it
	renderModelHandle = -1;
	touchCount = -1;

	if ( linked ) {
		Link( gameLocal.clip, entity, id, origin, axis, renderModelHandle );
	}
}

/*
================
idClipModel::SetPosition
================
*/
void idClipModel::SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis ) {
	if ( IsLinked() ) {
		Unlink();	// unlink from old position
	}
	origin = newOrigin;
	axis = newAxis;
}

/*
================
idClipModel::Handle
================
*/
cmHandle_t idClipModel::Handle( void ) const {
	assert( renderModelHandle == -1 );
	if ( collisionModelHandle ) {
		return collisionModelHandle;
	} else if ( traceModelIndex != -1 ) {
		return collisionModelManager->SetupTrmModel( *GetCachedTraceModel( traceModelIndex ), material );
	} else {
		// this happens in multiplayer on the combat models
		gameLocal.Warning( "idClipModel::Handle: clip model %d on '%s' (%x) is not a collision or trace model", id, entity->name.c_str(), entity->entityNumber );
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
void idClipModel::Unlink() {
	// stgatilov: this is a bit hacky,
	// but we only ever use one instance of idClip,
	// and storing additional pointer would be unnecessary waste of memory
	idClip &clp = gameLocal.clip;

	clp.octree.Remove( this );

	assert( !octreeHandle.IsLinked() );
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip &clp ) {

	assert( idClipModel::entity );
	if ( !idClipModel::entity ) {
		return;
	}


	if ( bounds.IsCleared() ) {
		Unlink();	// unlink from old position
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

	clp.octree.Update( this, absBounds );
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip &clp, idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int renderModelHandle ) {

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
	this->Link( clp );
}

/*
============
idClipModel::CheckModel
============
*/
cmHandle_t idClipModel::CheckModel( const char *name, const idDeclSkin* skin ) // skin added #4232 SteveL
{
	return collisionModelManager->LoadModel( name, false, skin );
}


/*
===============================================================

	idClip

===============================================================
*/

/*
===============
idClip::idClip
===============
*/
idClip::idClip( void ) {
	worldBounds.Zero();
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::idClip
===============
*/
idClip::~idClip( void ) {
}

/*
===============
idClip::Init
===============
*/
void idClip::Init( void ) {
	// get world map bounds
	//stgatilov: name of collision model equals "name" spawnarg of worldspawn entity (and "worldMap" if not specified)
	//however, it is certain that world collision model is always the first one (that's how it worked before rev 9592)
	cmHandle_t h = 0;	//collisionModelManager->LoadModel( "worldMap", false );
	const char *cmname = collisionModelManager->GetModelName( h );	//should be "worldMap" or whatever worldspawn's name is
	collisionModelManager->GetModelBounds( h, worldBounds );

	idVec3 size = worldBounds[1] - worldBounds[0];
	gameLocal.Printf( "map bounds are (%1.1f, %1.1f, %1.1f)\n", size[0], size[1], size[2] );

	// expand world bounds to cubic shape (same size by all coords) with same center
	idVec3 worldCenter = worldBounds.GetCenter();
	float worldRadius = size.Max() * 0.5f;
	idBounds worldCube(worldCenter);
	worldCube.ExpandSelf(worldRadius);

	// initialize octree
	touchCount = -1;
	octree.Init(worldCube, [](idBoxOctree::Pointer ptr) -> idBoxOctreeHandle& {
		return ((idClipModel*)ptr)->GetOctreeHandle();
	});

	// initialize a default clip model
	defaultClipModel.LoadModel( idTraceModel( idBounds( idVec3( 0, 0, 0 ) ).Expand( 8 ) ) );

	// set counters to zero
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
===============
idClip::Optimize
===============
*/
void idClip::Optimize( void ) {
	octree.Condense();
}

/*
===============
idClip::Shutdown
===============
*/
void idClip::Shutdown( void ) {
	octree.Clear();

	// free the trace model used for the temporaryClipModel
	if ( temporaryClipModel.traceModelIndex != -1 ) {
		idClipModel::FreeTraceModel( temporaryClipModel.traceModelIndex );
		temporaryClipModel.traceModelIndex = -1;
	}

	// free the trace model used for the defaultClipModel
	if ( defaultClipModel.traceModelIndex != -1 ) {
		idClipModel::FreeTraceModel( defaultClipModel.traceModelIndex );
		defaultClipModel.traceModelIndex = -1;
	}
}

/*
================
idClip::ClipModelsTouchingBounds
================
*/
int idClip::ClipModelsTouchingBounds( const idBounds &bounds, int contentMask, idClip_ClipModelList &clipModelList ) const {
	if ( bounds.IsBackwards() ) {
		// we should not go through the tree for degenerate or backwards bounds
		assert( false );
		return 0;
	}

	idBounds queryBox = bounds;
	queryBox.ExpandSelf(vec3_boxEpsilon);

	idBoxOctree::QueryResult res;
	octree.QueryInBox(queryBox, res);

	clipModelList.Clear();
	touchCount++;

	for ( int i = 0; i < res.Num(); i++ ) {
		auto chunk = res[i];

		for ( int j = 0; j < chunk->num; j++ ) {
			idClipModel *check = (idClipModel*)chunk->arr[j].object;
			const idBounds &absBounds = chunk->arr[j].bounds;
			assert(absBounds == check->absBounds);

			// if the bounds really do overlap
			if ( !absBounds.IntersectsBounds(queryBox) ) {
				continue;
			}

			// if the clip model does not have any contents we are looking for
			if ( !( check->contents & contentMask ) ) {
				continue;
			}

			// if the clip model is enabled
			if ( !check->enabled ) {
				continue;
			}

			// avoid duplicates in the list
			if ( check->touchCount == touchCount ) {
				continue;
			}

			check->touchCount = touchCount;
			clipModelList.AddGrow(check);
		}
	}

	return clipModelList.Num();
}

/*
================
idClip::ClipModelsTouchingMovingBounds
================
*/
int idClip::ClipModelsTouchingMovingBounds(
			const idBounds &absBounds, const idBounds &stillBounds, const idVec3 &start, const idVec3 &end,
			int contentMask,
			idClip_ClipModelList &clipModelList, idClip_FloatList &fractionLowers ) const
{
	if ( absBounds.IsBackwards() ) {
		// we should not go through the tree for degenerate or backwards bounds
		assert( false );
		return 0;
	}

	idBounds queryBox = absBounds;
	queryBox.ExpandSelf(vec3_boxEpsilon);

	idVec3 queryStart = start + stillBounds.GetCenter();
	idVec3 queryExtent = stillBounds.GetSize() * 0.5f + vec3_boxEpsilon;
	idVec3 queryInvDir = GetInverseMovementVelocity(start, end);

	idBoxOctree::QueryResult res;
	octree.QueryInMovingBox(queryBox, queryStart, queryInvDir, queryExtent, res);

	clipModelList.Clear();
	fractionLowers.Clear();
	touchCount++;

	for ( int i = 0; i < res.Num(); i++ ) {
		auto chunk = res[i];

		for ( int j = 0; j < chunk->num; j++ ) {
			idClipModel *check = (idClipModel*)chunk->arr[j].object;
			const idBounds &absBounds = chunk->arr[j].bounds;
			assert(absBounds == check->absBounds);

			// if the bounds really do overlap
			if ( !absBounds.IntersectsBounds(queryBox) ) {
				continue;
			}

			// if moving bounds intersect with entity bounds
			float range[2] = {0.0f, 1.0f};
			if ( !MovingBoundsIntersectBounds(queryStart, queryInvDir, queryExtent, absBounds, range) ) {
				continue;
			}

			// if the clip model does not have any contents we are looking for
			if ( !( check->contents & contentMask ) ) {
				continue;
			}

			// if the clip model is enabled
			if ( !check->enabled ) {
				continue;
			}

			// avoid duplicates in the list
			if ( check->touchCount == touchCount ) {
				continue;
			}

			check->touchCount = touchCount;
			clipModelList.AddGrow(check);
			fractionLowers.AddGrow(range[0]);
		}
	}

	// sort clip models by lower bound on intersection time
	int n = clipModelList.Num();
	assert(n == fractionLowers.Num());
	for (int i = 0; i < n; i++)
		for (int j = i+1; j < n; j++)
			if (fractionLowers[i] > fractionLowers[j]) {
				idSwap(fractionLowers[i], fractionLowers[j]);
				idSwap(clipModelList[i], clipModelList[j]);
			}

	return n;
}

/*
================
idClip::EntitiesTouchingBounds
================
*/
int idClip::EntitiesTouchingBounds( const idBounds &bounds, int contentMask, idClip_EntityList &entityList ) const {
	idClip_ClipModelList clipModelList;
	idClip::ClipModelsTouchingBounds( bounds, contentMask, clipModelList );
	FilterEntities( entityList, clipModelList );
	return entityList.Num();
}
//stgatilov: filtering part of EntitiesTouchingBounds refactored into this internal method
void idClip::FilterEntities( idClip_EntityList &entityList, idClip_ClipModelList &clipModelList ) const {
	entityList.Clear();
	for ( int i = 0; i < clipModelList.Num(); i++ ) {
		// entity could already be in the list because an entity can use multiple clip models
		int j;
		for ( j = 0; j < entityList.Num(); j++ ) {
			if ( entityList[j] == clipModelList[i]->entity ) {
				break;
			}
		}
		if ( j >= entityList.Num() ) {
			entityList.AddGrow(clipModelList[i]->entity);
		}
	}
}

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
int idClip::GetTraceClipModels( const idBounds &bounds, int contentMask, const idEntity *passEntity, idClip_ClipModelList &clipModelList ) const {
	ClipModelsTouchingBounds( bounds, contentMask, clipModelList );
	FilterClipModels(passEntity, clipModelList);
	return clipModelList.Num();
}
int idClip::GetTraceClipModels( const idBounds &absBounds, const idBounds &stillBounds, const idVec3 &start, const idVec3 &end,
	int contentMask, const idEntity *passEntity, idClip_ClipModelList &clipModelList, idClip_FloatList &fractionLowers ) const 
{
	ClipModelsTouchingMovingBounds( absBounds, stillBounds, start, end, contentMask, clipModelList, fractionLowers );
	FilterClipModels(passEntity, clipModelList);
	return clipModelList.Num();
}
//stgatilov: filtering part of GetTraceClipModels refactored into this internal method
void idClip::FilterClipModels(const idEntity *passEntity, idClip_ClipModelList &clipModelList ) const {
	if ( !passEntity ) {
		return;
	}

	idEntity *passOwner;
	if ( passEntity->GetPhysics()->GetNumClipModels() > 0 ) {
		passOwner = passEntity->GetPhysics()->GetClipModel()->GetOwner();
	} else {
		passOwner = NULL;
	}

	for ( int i = 0; i < clipModelList.Num(); i++ ) {
		idClipModel	*cm = clipModelList[i];

		// check if we should ignore this entity
		if ( cm->entity == passEntity ) {
			clipModelList[i] = NULL;			// don't clip against the pass entity
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
//		assert( 0 );

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
void idClip::TranslationEntities( trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	//stgatilov: I refactored this method because it was complete copy/paste of Translation
	//except for the fact that collision with World is ignored
	Translation(results, start, end, mdl, trmAxis, contentMask, passEntity, true);
}

/*
============
idClip::Translation
============
*/
bool idClip::Translation( trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity,
						bool ignoreWorld ) {
	int i, num;
	idClipModel *touch;
	idBounds traceBounds;
	float radius;
	trace_t trace;
	const idTraceModel *trm;

	if ( TestHugeTranslation( results, mdl, start, end, trmAxis ) ) {
		return true;
	}
	TRACE_CPU_SCOPE("Clip:Translate");

	trm = TraceModelForClipModel( mdl );

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("(%0.2f %0.2f %0.2f) -> (%0.2f %0.2f %0.2f)\nContMask: 0x%x\nTrm: V%d\nIgnore: %s %s\n",
		start.x, start.y, start.z, end.x, end.y, end.z,
		contentMask, 
		(trm ? trm->numVerts: -1),
		(ignoreWorld ? "ignoreWorld" : ""),
		(passEntity ? passEntity->name.c_str() : "")
	)
#endif

	if ( !ignoreWorld && (!passEntity || passEntity->entityNumber != ENTITYNUM_WORLD) ) {
		// test world
		idClip::numTranslations++;
		collisionModelManager->Translation( &results, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		results.c.entityNum = results.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
		if ( results.fraction == 0.0f ) {
			return true;		// blocked immediately by the world
		}
	} else {
		memset( &results, 0, sizeof( results ) );
		results.fraction = 1.0f;
		results.endpos = end;
		results.endAxis = trmAxis;
	}

	//stgatilov: first obtain bounds with rotation applied (relative to start point)
	idBounds localBounds;
	if ( trm ) {
		if (trmAxis.IsRotated())
			localBounds.FromTransformedBounds(trm->bounds, idVec3(0.0f), trmAxis);
		else
			localBounds = trm->bounds;
		radius = trm->bounds.GetRadius();
	}
	else {
		localBounds.Zero();
		radius = 0.0f;
	}
	//then obtain bounds in global coordinates during the whole movement
	traceBounds.FromBoundsTranslation(localBounds, start, results.endpos - start);

	idClip_ClipModelList clipModelList;
	idClip_FloatList fractionLowers;

	idVec3 globalSize = traceBounds.GetSize(), localSizeCap = localBounds.GetSize() * 2.0;
	float totalMovement = (results.endpos - start).Length();
	bool movingClipCheck = (totalMovement > CM_BOX_EPSILON && (globalSize.x > localSizeCap.x || globalSize.y > localSizeCap.y || globalSize.z > localSizeCap.z));
	if (movingClipCheck) {
		//box moved significantly: clip with moving bounds
		num = GetTraceClipModels( traceBounds, localBounds, start, results.endpos, contentMask, passEntity, clipModelList, fractionLowers );
		//note: convert fraction bounds to [start..end] range
		float partOfWhole = totalMovement * idMath::InvSqrt((end - start).LengthSqr());
		for (i = 0; i < num; i++)
			fractionLowers[i] *= partOfWhole;
	} else {
		//box almost stands still: clip with bounds of whole movement
		num = GetTraceClipModels( traceBounds, contentMask, passEntity, clipModelList );
	}

	for ( i = 0; i < num; i++ ) {
		touch = clipModelList[i];

		if ( !touch ) {
			continue;
		}

		if (movingClipCheck && fractionLowers[i] > results.fraction) {
			//stgatilov: judging from bounds, we can only obtain higher fractions for other models
			for (int t = i; t < num; t++)
				assert(fractionLowers[t] > results.fraction);	//were sorted in ClipModelsTouchingMovingBounds
			break;
		}

		if ( touch->renderModelHandle != -1 ) {
			idClip::numRenderModelTraces++;
			TraceRenderModel( trace, start, end, radius, trmAxis, touch );
		} else {
			idClip::numTranslations++;
			collisionModelManager->Translation( &trace, start, end, trm, trmAxis, contentMask,
									touch->Handle(), touch->origin, touch->axis );
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

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("%s#cand: %d\nFrac: %0.3f\nHitEnt: %s\nCont:0x%d",
		movingClipCheck ? "Moving check\n" : "",
		num,
		results.fraction,
		(results.c.entityNum == ENTITYNUM_NONE || !gameLocal.entities[results.c.entityNum] ? "[none]" : gameLocal.entities[results.c.entityNum]->name.c_str()),
		results.c.contents
	)
#endif
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
	idClipModel *touch;
	idBounds traceBounds;
	trace_t trace;
	const idTraceModel *trm;

	TRACE_CPU_SCOPE("Clip:Rotate");

	trm = TraceModelForClipModel( mdl );

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numRotations++;
		collisionModelManager->Rotation( &results, start, rotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
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

	idClip_ClipModelList clipModelList;
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
							touch->Handle(), touch->origin, touch->axis );

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
	idClipModel *touch;
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

	TRACE_CPU_SCOPE("Clip:Motion");

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
		collisionModelManager->Translation( &translationalTrace, start, end, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
		translationalTrace.c.entityNum = translationalTrace.fraction != 1.0f ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	} else {
		memset( &translationalTrace, 0, sizeof( translationalTrace ) );
		translationalTrace.fraction = 1.0f;
		translationalTrace.endpos = end;
		translationalTrace.endAxis = trmAxis;
	}

	idClip_ClipModelList clipModelList;

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
										touch->Handle(), touch->origin, touch->axis );
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
		collisionModelManager->Rotation( &rotationalTrace, endPosition, endRotation, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
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
								touch->Handle(), touch->origin, touch->axis );

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
	int i, j, num, n, numContacts;
	idClipModel *touch;
	idBounds traceBounds;
	const idTraceModel *trm;

	TRACE_CPU_SCOPE("Clip:Contacts");

	trm = TraceModelForClipModel( mdl );

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("(%0.2f %0.2f %0.2f): (%0.2f %0.2f %0.2f  %0.2f %0.2f %0.2f) D%0.2f\nContMask: 0x%x\nTrm: V%d\nIgnore: %s\n",
		start.x, start.y, start.z, dir[0], dir[1], dir[2], dir[3], dir[4], dir[5], depth,
		contentMask, 
		(trm ? trm->numVerts: -1),
		(passEntity ? passEntity->name.c_str() : "")
	)
#endif

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numContacts++;
		numContacts = collisionModelManager->Contacts( contacts, maxContacts, start, dir, depth, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
	} else {
		numContacts = 0;
	}

	for ( i = 0; i < numContacts; i++ ) {
		contacts[i].entityNum = ENTITYNUM_WORLD;
		contacts[i].id = 0;
	}

	if ( numContacts >= maxContacts ) {
		return numContacts;
	}

	if ( !trm ) {
		traceBounds = idBounds( start ).Expand( depth );
	} else {
		traceBounds.FromTransformedBounds( trm->bounds, start, trmAxis );
		traceBounds.ExpandSelf( depth );
	}

	idClip_ClipModelList clipModelList;
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
									touch->Handle(), touch->origin, touch->axis );

		for ( j = 0; j < n; j++ ) {
			contacts[numContacts].entityNum = touch->entity->entityNumber;
			contacts[numContacts].id = touch->id;
			numContacts++;
		}

		if ( numContacts >= maxContacts ) {
			break;
		}
	}

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("#cand: %d\n#contacts: %d\n",
		num,
		numContacts
	)
#endif
	return numContacts;
}

/*
============
idClip::Contents
============
*/
int idClip::Contents( const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	int i, num, contents;
	idClipModel *touch;
	idBounds traceBounds;
	const idTraceModel *trm;

	TRACE_CPU_SCOPE("Clip:Contents");

	trm = TraceModelForClipModel( mdl );

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("(%0.2f %0.2f %0.2f)\nContMask: 0x%x\nTrm: V%d\nIgnore: %s\n",
		start.x, start.y, start.z,
		contentMask, 
		(trm ? trm->numVerts: -1),
		(passEntity ? passEntity->name.c_str() : "")
	)
#endif

	if ( !passEntity || passEntity->entityNumber != ENTITYNUM_WORLD ) {
		// test world
		idClip::numContents++;
		contents = collisionModelManager->Contents( start, trm, trmAxis, contentMask, 0, vec3_origin, mat3_default );
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

	idClip_ClipModelList clipModelList;
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
		if ( collisionModelManager->Contents( start, trm, trmAxis, contentMask, touch->Handle(), touch->origin, touch->axis ) ) {
			contents |= ( touch->contents & contentMask );
		}
	}

#if TRACE_CLIP_INFO
	TRACE_ATTACH_FORMAT("#cand: %d\n#contents: 0x%x\n",
		num,
		contents
	)
#endif
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
	cmHandle_t handle;
	idVec3 start, end;

	handle = -1;
	winding.Clear();

	if ( clipModel == NULL ) {
		handle = 0;
	} else {
		if ( clipModel->renderModelHandle != -1 ) {
			winding += contact.point;
			return true;
		} else if ( clipModel->traceModelIndex != -1 ) {
			handle = collisionModelManager->SetupTrmModel( *idClipModel::GetCachedTraceModel( clipModel->traceModelIndex ), clipModel->material );
		} else {
			handle = clipModel->collisionModelHandle;
		}
	}

	// if contact with a collision model
	if ( handle != -1 ) {
		switch( contact.type ) {
			case CONTACT_EDGE: {
				// the model contact feature is a collision model edge
				collisionModelManager->GetModelEdge( handle, contact.modelFeature, start, end );
				winding += start;
				winding += end;
				break;
			}
			case CONTACT_MODELVERTEX: {
				// the model contact feature is a collision model vertex
				collisionModelManager->GetModelVertex( handle, contact.modelFeature, start );
				winding += start;
				break;
			}
			case CONTACT_TRMVERTEX: {
				// the model contact feature is a collision model polygon
				collisionModelManager->GetModelPolygon( handle, contact.modelFeature, winding );
				break;
			}
			default: break;
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
	gameLocal.Printf( "t = %-3d, r = %-3d, m = %-3d, render = %-3d, contents = %-3d, contacts = %-3d\n",
					numTranslations, numRotations, numMotions, numRenderModelTraces, numContents, numContacts );
	numRotations = numTranslations = numMotions = numRenderModelTraces = numContents = numContacts = 0;
}

/*
============
idClip::DrawClipModel
============
*/
void idClip::DrawClipModel( const idClipModel *clipModel, const idVec3 &eye, const float radius ) const {
	if ( clipModel->renderModelHandle != -1 ) {
		gameRenderWorld->DebugBounds( colorCyan, clipModel->GetAbsBounds() );
	} else {
		collisionModelManager->DrawModel( clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis(), eye, radius );
	}
}

/*
============
idClip::DrawClipModels
============
*/
void idClip::DrawClipModels( const idVec3 &eye, const float radius, const idEntity *passEntity ) {
	int				i, num;
	idBounds		bounds;
	idClipModel		*clipModel;

	bounds = idBounds( eye ).Expand( radius );

	idClip_ClipModelList clipModelList;
	num = idClip::ClipModelsTouchingBounds( bounds, -1, clipModelList );

	for ( i = 0; i < num; i++ ) {
		clipModel = clipModelList[i];
		if ( clipModel->GetEntity() == passEntity ) {
			continue;
		}
		DrawClipModel(clipModel, eye, radius);
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
	gameRenderWorld->DebugText( contact.material->GetName(), winding.GetCenter() - 4.0f * axis[2], 0.1f, colorWhite, axis, 1, 5000 );

	return true;
}

void idClipModel::TranslateOrigin( const idVec3 &translation )
{
	if( IsTraceModel() )
	{
		// Copy the tracemodel
		idTraceModel trm = *(idClipModel::GetCachedTraceModel( traceModelIndex ));
		trm.Translate( translation );
		
		LoadModel( trm );
	}
}
