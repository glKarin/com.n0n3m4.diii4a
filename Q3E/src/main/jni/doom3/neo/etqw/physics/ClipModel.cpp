// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

static idVec3 vec3_boxEpsilon( CM_BOX_EPSILON, CM_BOX_EPSILON, CM_BOX_EPSILON );

#include "../Player.h"

const float CLIP_CHECK_FACTOR = CM_CLIP_EPSILON * 0.5f;

/*
===============================================================

	idClipModel

===============================================================

*/

CLASS_DECLARATION( idClass, idClipModel )
END_CLASS

/*
================
idClipModel::Draw
================
*/
void idClipModel::Draw( const idVec3& origin, const idMat3& axis, float radius, int lifetime ) const {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer ) {
		return;
	}

	renderView_t* view = localPlayer->GetRenderView();
	if ( !view ) {
		return;
	}

	if ( collisionModel != NULL ) {
		collisionModelManager->DrawModel( collisionModel, origin, axis, view->vieworg, view->viewaxis, radius, lifetime );
	}

	for ( int i = 0; i < traceModels.Num(); i++ ) {
		idCollisionModel* cm = gameLocal.traceModelCache.GetCollisionModel( traceModels[ i ] );
		collisionModelManager->DrawModel( cm, origin, axis, view->vieworg, view->viewaxis, radius, lifetime );

/*		idBounds bounds( idVec3( -1, -1, -1 ), idVec3( 1, 1, 1 ) );

		const traceModelWater_t* waterPoints = gameLocal.traceModelCache.GetWaterPoints( traceModels[ i ] );
		int j;
		for ( j = 0; j < MAX_TRACEMODEL_WATER_POINTS; j++ ) {
			gameRenderWorld->DebugBox( colorRed, idBox( bounds, origin + ( waterPoints[ j ].xyz * axis ), mat3_identity ) );
		}*/
	}
}

/*
================
idClipModel::Draw
================
*/
void idClipModel::Draw( float radius ) const {
	Draw( GetOrigin(), GetAxis(), radius );
}

/*
================
idClipModel::FreeModel
================
*/
void idClipModel::FreeModel( void ) {
	int i;

	for ( i = 0; i < traceModels.Num(); i++ ) {
		gameLocal.traceModelCache.FreeTraceModel( traceModels[i] );
	}
	traceModels.SetNum( 0, false );

	if ( collisionModel != NULL ) {
		collisionModelManager->FreeModel( collisionModel );
		collisionModel = NULL;
	}

	renderEntity = 0;
}

/*
================
idClipModel::LoadCollisionModel
================
*/
bool idClipModel::LoadCollisionModel( const char *name ) {
	assert( !IsLinked() );
	if ( IsLinked() ) {
		gameLocal.Error( "LoadCollisionModel called while linked" );
	}

	FreeModel();

	if ( name != NULL && *name != '\0' ) {
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), name );
	}
	if ( collisionModel != NULL ) {
		bounds = collisionModel->GetBounds();
		absBounds = bounds;
		contents = collisionModel->GetContents();
		backupContents = contents;
		return true;
	} else {
		bounds.Zero();
		return false;
	}
}

/*
================
idClipModel::LoadCollisionModel
================
*/
void idClipModel::LoadCollisionModel( idCollisionModel *model ) {
	assert( !IsLinked() );
	if ( IsLinked() ) {
		gameLocal.Error( "LoadCollisionModel called while linked" );
	}

	FreeModel();

	collisionModel = model;
	bounds = collisionModel->GetBounds();
	absBounds = bounds;
	contents = collisionModel->GetContents();
	backupContents = contents;
}

/*
================
idClipModel::LoadTraceModel
================
*/
void idClipModel::LoadTraceModel( const idTraceModel &trm, bool includeBrushes ) {
	assert( !IsLinked() );
	if ( IsLinked() ) {
		gameLocal.Error( "LoadTraceModel called while linked" );
	}

	FreeModel();

	traceModels.Append( gameLocal.traceModelCache.AllocTraceModel( trm, includeBrushes ) );
	bounds = trm.bounds;
	absBounds = bounds;
}

/*
================
idClipModel::LoadRenderModel
================
*/
void idClipModel::LoadRenderModel( qhandle_t renderEntity ) {
	assert( !IsLinked() );
	if ( IsLinked() ) {
		gameLocal.Error( "LoadRenderModel called while linked" );
	}

	FreeModel();

	if ( renderEntity != -1 ) {
		this->renderEntity	= renderEntity;		
		this->bounds		= gameRenderWorld->GetRenderEntity( renderEntity )->hModel->Bounds();
	}
}

/*
================
idClipModel::Init
================
*/
void idClipModel::Init( void ) {
	entity = NULL;
	entityNumber = ENTITYNUM_NONE;
	id = 0;
	origin.Zero();
	axis.Identity();
	bounds.Zero();
	absBounds.Zero();
	material = NULL;
	contents = CONTENTS_BODY;
	backupContents = contents;
	collisionModel = NULL;
	renderEntity = 0;
	traceModels.Clear();
	clipLinks = NULL;
	lastLinkCoords[0] = 0;
	lastLinkCoords[1] = 0;
	lastLinkCoords[2] = 0;
	lastLinkCoords[3] = 0;
	nextDeleted = NULL;
	deleteThreadCount = 0;
	lastMailBox = 0xffffffff;
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
	LoadCollisionModel( name );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( idCollisionModel *model ) {
	Init();
	LoadCollisionModel( model );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idTraceModel &trm, bool includeBrushes ) {
	Init();
	LoadTraceModel( trm, includeBrushes );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( qhandle_t renderEntity ) {
	Init();
	contents = CONTENTS_RENDERMODEL;
	backupContents = contents;
	LoadRenderModel( renderEntity );
}

/*
================
idClipModel::idClipModel
================
*/
idClipModel::idClipModel( const idClipModel *model ) {
	int i;

	SetEntity( model->entity );
	id = model->id;
	origin = model->origin;
	axis = model->axis;
	bounds = model->bounds;
	absBounds = model->absBounds;
	material = model->material;
	contents = model->contents;
	backupContents = contents;
	if ( model->collisionModel != NULL ) {
		collisionModel = collisionModelManager->LoadModel( gameLocal.GetMapName(), model->collisionModel->GetName() );
	} else {
		collisionModel = NULL;
	}
	traceModels.SetNum( model->traceModels.Num() );
	for ( i = 0; i < model->traceModels.Num(); i++ ) {
		traceModels[i] = gameLocal.traceModelCache.CopyTraceModel( model->traceModels[i] );
	}
	renderEntity = model->renderEntity;
	clipLinks = NULL;
}

/*
================
idClipModel::~idClipModel
================
*/
idClipModel::~idClipModel( void ) {
	// make sure the clip model is no longer linked
	FreeModel();
}

/*
================
idClipModel::SetEntity
================
*/
void idClipModel::SetEntity( idEntity *newEntity ) {
	entity = newEntity;
	entityNumber = ( newEntity != NULL ) ? newEntity->entityNumber : ENTITYNUM_WORLD;
}

/*
================
idClipModel::GetEntityName
================
*/
const char *idClipModel::GetEntityName( void ) const {
	if ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID && GetEntity() != NULL ) {
		return GetEntity()->GetName();
	}
	return "";
}

/*
================
idClipModel::SetPosition
================
*/
void idClipModel::SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis, idClip &clp ) {
	origin = newOrigin;
	axis = newAxis;

	if ( CheckCoords( clp ) ) {
		Unlink( clp );	// unlink from old position
	}
}

/*
================
idClipModel::GetMassProperties
================
*/
void idClipModel::GetMassProperties( const float density, float &mass, idVec3 &centerOfMass, idMat3 &inertiaTensor ) const {
	if ( traceModels.Num() == 0 ) {

		gameLocal.Error( "idClipModel::GetMassProperties: clip model %d on '%s' is not a trace model", id, entity->GetName() );

	} else if ( traceModels.Num() == 1 ) {

		gameLocal.traceModelCache.GetMassProperties( traceModels[0], density, mass, centerOfMass, inertiaTensor );

	} else {

		int i;
		float trmMass;
		idVec3 trmCenterOfMass;
		idMat3 trmInertiaTensor;

		mass = 0.0f;
		centerOfMass.Zero();
		inertiaTensor.Zero();

		for ( i = 0; i < traceModels.Num(); i++ ) {
			gameLocal.traceModelCache.GetMassProperties( traceModels[i], density, trmMass, trmCenterOfMass, trmInertiaTensor );
			mass += trmMass;
			centerOfMass += trmMass * trmCenterOfMass;
		}
		centerOfMass /= mass;

		for ( i = 0; i < traceModels.Num(); i++ ) {
			gameLocal.traceModelCache.GetMassProperties( traceModels[i], density, trmMass, trmCenterOfMass, trmInertiaTensor );
			trmInertiaTensor.InertiaTranslateSelf( trmMass, trmCenterOfMass, centerOfMass - trmCenterOfMass );
			inertiaTensor += trmInertiaTensor;
		}
	}
}

/*
===============
idClipModel::Unlink
===============
*/
void idClipModel::Unlink( idClip &clp ) {
	clipLink_t *link;

	clp.Lock();

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

		clp.GetClipLinkAllocator().Free( link );
	}

	clp.Unlock();
}

/*
===============
idClipModel::CheckCoords
===============
*/
bool idClipModel::CheckCoords( idClip& clp ) {
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

	int coords[ 4 ];
	clp.CoordsForBounds( coords, absBounds );

	if ( IsLinked() ) {
		if ( ( lastLinkCoords[ 0 ] == coords[ 0 ] ) &&
			( lastLinkCoords[ 1 ] == coords[ 1 ] ) &&
			( lastLinkCoords[ 2 ] == coords[ 2 ] ) &&
			( lastLinkCoords[ 3 ] == coords[ 3 ] ) ) {
			return false;
		}
	}

	lastLinkCoords[ 0 ] = coords[ 0 ];
	lastLinkCoords[ 1 ] = coords[ 1 ];
	lastLinkCoords[ 2 ] = coords[ 2 ];
	lastLinkCoords[ 3 ] = coords[ 3 ];

	return true;
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip &clp ) {
	if ( !entity ) {
		assert( false );
		return;
	}

	if ( !clp.GetClipSectors() || entity->fl.forceDisableClip ) {
		return; 
	}

	if ( bounds.IsCleared() ) {
		return;
	}

	if ( !CheckCoords( clp ) ) {
		return;
	}

	Unlink( clp );	// unlink from old position

	clp.Lock();

	int x, y;
	for( x = lastLinkCoords[ 0 ]; x < lastLinkCoords[ 2 ]; x++ ) {
		for( y = lastLinkCoords[ 1 ]; y < lastLinkCoords[ 3 ]; y++ ) {
			clipSector_t* sector = &clp.GetClipSectors()[ x + ( y << CLIPSECTOR_DEPTH ) ];

			clipLink_t* link = clp.GetClipLinkAllocator().Alloc();
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

	clp.Unlock();
}

/*
===============
idClipModel::Link
===============
*/
void idClipModel::Link( idClip &clp, idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int _renderModelHandle ) {
	SetEntity( ent );
	id = newId;

	origin = newOrigin;
	axis = newAxis;

	if ( _renderModelHandle != -1 ) {
		renderEntity = _renderModelHandle;
		const renderEntity_t* renderEnt = gameRenderWorld->GetRenderEntity( renderEntity );
		if ( renderEnt ) {
			bounds = renderEnt->bounds;
		}
	}

	Link( clp );
}

/*
===============
idClipModel::GetTraceModel
===============
*/
const idTraceModel *idClipModel::GetTraceModel( int index ) const {
	if ( index < traceModels.Num() ) {
		return gameLocal.traceModelCache.GetTraceModel( traceModels[index] );
	} else {
		return NULL;
	}
}

/*
===============
idClipModel::GetWaterPoints
===============
*/
const traceModelWater_t* idClipModel::GetWaterPoints( int index ) const {
	if ( index < traceModels.Num() ) {
		return gameLocal.traceModelCache.GetWaterPoints( traceModels[ index ] );
	} else {
		return NULL;
	}
}

/*
===============
idClipModel::GetTraceModelVolume
===============
*/
float idClipModel::GetTraceModelVolume( int index ) const {
	if ( index < traceModels.Num() ) {
		return gameLocal.traceModelCache.GetVolume( traceModels[ index ] );
	} else {
		return 0.f;
	}
}

/*
===============
idClipModel::IsEqual
===============
*/
bool idClipModel::IsEqual( const idTraceModel &trm ) const {
	return ( traceModels.Num() != 0 && *gameLocal.traceModelCache.GetTraceModel( traceModels[0] ) == trm );
}

/*
===============
idClipModel::GetCollisionModel
===============
*/
idCollisionModel *idClipModel::GetCollisionModel( int index ) const {
	if ( collisionModel != NULL ) {
		return collisionModel;
	} else {
		if ( index >= 0 && index < traceModels.Num() ) {
			return gameLocal.traceModelCache.GetCollisionModel( traceModels[index] );
		}
	}
	if ( collisionModelManager->GetThreadId() == MAIN_THREAD_ID ) {
		if ( entity != NULL ) {
			gameLocal.Warning( "idClipModel::GetCollisionModel: clip model %d on '%s' (%i) is not a collision or trace model", id, entity->GetName(), entity->entityNumber );
		} else {		
			assert( false );
			gameLocal.Warning( "idClipModel::GetCollisionModel: clip model %d on NULL entity is not a collision or trace model", id );
		}
	}
	return NULL;
}
