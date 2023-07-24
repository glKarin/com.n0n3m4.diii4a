// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "HardcodedParticleSystem.h"

static const char *hardcodedParticleSystem_SnapshotName0 = "_HardcodedParticleSystem_Snapshot0_";
static const char *hardcodedParticleSystem_SnapshotName1 = "_HardcodedParticleSystem_Snapshot1_";

/*
================
sdHardcodedParticleSystem::sdHardcodedParticleSystem
================
*/
sdHardcodedParticleSystem::sdHardcodedParticleSystem( void ) {

	lastGameFrame = -1;
	currentDB = 0;

	renderEntityHandle = -1;
	Uid = renderSystem->RegisterPtr( this );

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	hModel[0] = renderModelManager->AllocModel();
	hModel[1] = renderModelManager->AllocModel();
	hModel[0]->InitEmpty( hardcodedParticleSystem_SnapshotName0 );
	hModel[1]->InitEmpty( hardcodedParticleSystem_SnapshotName1 );

	renderEntity.hModel = hModel[0];
	renderEntity.callback = sdHardcodedParticleSystem::ModelCallback;
#pragma warning( push )
#pragma warning( disable: 4312 )
	renderEntity.callbackData = (void*)Uid;
#pragma warning( pop )

}
/*
================
sdHardcodedParticleSystem::~sdHardcodedParticleSystem
================
*/
sdHardcodedParticleSystem::~sdHardcodedParticleSystem( void ) {
	// make sure the render entity is freed before the model is freed
	renderSystem->UnregisterPtr( Uid );
	FreeRenderEntity();
	renderModelManager->FreeModel( hModel[0] );
	renderModelManager->FreeModel( hModel[1] );
}

/*
================
sdHardcodedParticleSystem::ModelCallback
================
*/
bool sdHardcodedParticleSystem::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime ) {
#pragma warning( push )
#pragma warning( disable: 4311 )
	sdHardcodedParticleSystem* me = (sdHardcodedParticleSystem*)renderSystem->PtrForUID( (int)renderEntity->callbackData );
#pragma warning( pop )

	if ( !me ) {
		return false;
	}

	if ( me->RenderEntityCallback( renderEntity, renderView, lastGameModifiedTime ) ) {
		return true;
	}

	return false;
}

/*
================
sdHardcodedParticleSystem::PresentRenderEntity
================
*/
void sdHardcodedParticleSystem::PresentRenderEntity( void ) {
	if ( renderEntityHandle == -1 ) {
		renderEntityHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( renderEntityHandle, &renderEntity );
	}
}

/*
================
sdHardcodedParticleSystem::PresentEntityDef
================
*/
void sdHardcodedParticleSystem::FreeRenderEntity( void ) {
	if ( renderEntityHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
}	

/*
================
sdHardcodedParticleSystem::AddSurface
================
*/
int sdHardcodedParticleSystem::AddSurface( const idMaterial *material, int numVerts, int numIndexes ) {
	modelSurface_t surface;
	memset( &surface, 0, sizeof( surface ) );
	surface.geometry = renderEntity.hModel->AllocSurfaceTriangles(  numVerts, numIndexes );
	surface.material = material;
	surface.id = renderEntity.hModel->NumSurfaces();
	renderEntity.hModel->AddSurface( surface );
	return surface.id;
}

/*
================
sdHardcodedParticleSystem::AddSurface
================
*/
void sdHardcodedParticleSystem::AddSurfaceDB( const idMaterial *material, int numVerts, int numIndexes ) {
	modelSurface_t surface;
	memset( &surface, 0, sizeof( surface ) );
	surface.geometry = hModel[0]->AllocSurfaceTriangles(  numVerts, numIndexes );
	surface.material = material;
	surface.id = hModel[0]->NumSurfaces();
	hModel[0]->AddSurface( surface );

	memset( &surface, 0, sizeof( surface ) );
	surface.geometry = hModel[1]->AllocSurfaceTriangles(  numVerts, numIndexes );
	surface.material = material;
	surface.id = hModel[1]->NumSurfaces();
	hModel[1]->AddSurface( surface );
}

/*
================
sdHardcodedParticleSystem::ClearSurfaces
================
*/
void sdHardcodedParticleSystem::ClearSurfaces( void ) {
	renderEntity.hModel->InitEmpty( hardcodedParticleSystem_SnapshotName0 );
}

/*
================
sdHardcodedParticleSystem::SetThreadingHModel
================
*/
void sdHardcodedParticleSystem::SetDoubleBufferedModel( void ) {
	if ( lastGameFrame != renderSystem->GetSyncNum() ) {
		currentDB = 1 - currentDB;
		lastGameFrame = renderSystem->GetSyncNum();
	}
	//int db = renderSystem->GetDoubleBufferIndex();
	renderEntity.hModel = hModel[ currentDB ];
}