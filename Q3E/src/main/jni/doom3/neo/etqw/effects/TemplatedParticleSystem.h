// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_TEMPLATEDPARTICLE_SYSTEM_H__
#define __GAME_TEMPLATEDPARTICLE_SYSTEM_H__

#include "HardcodedParticleSystem.h"
#include "../Player.h"
#include "../demos/DemoManager.h"

/************************************************************************

	This is a templated particle system, it does all the list management for
	you you just give it a class that defines the behaviour of a single particle.
	Making this templated instead of using a particle class with virtual methods
	means we can have particles wich are differently sized and that all method 
	calls are statically linked or inlined.
	ParticleClass is a class wich defines the actual logic of the particles
	rendered by this system, it needs to have the following methods

	static const int NUM_INSTANCE_VERTEXES
		Number of vertexes that need to be allocated in the triangle surface for
		an instance of this particle class.

	static const int NUM_INSTANCE_INDEXES
		Number of indexes that need to be allocated in the triangle surface for
		an instance of this particle class.

	void StaticInitializeAndRender( sdTemplatedParticleSystem<class> * )
		This method will be called to initialize the particles once when
		the system is initialized. This method can write to the tirangle
		surface if it wants to setup triangle index/vertex data that 
		doesn't change.

	bool Initialize( sdTemplatedParticleSystem<class> * )
		Reinitialize a particle after it died. This can be called several times
		during the lifetime of a particle.
		Should return false if the initialization failed for some reason (init
		may be called again next frame tough)

	bool Update( sdTemplatedParticleSystem<class> * )
		Updates the "physics" associated with this particle, returns false if the 
		particle died and should be reinitialized, true to continue like normal.

	void Render( sdTemplatedParticleSystem<class> * )
		Should add the vertexes and indexes to the triangle list (GetTriSurf on
		the passed in system) if it is invisible it should just do nothing.

	ParticleClass is not used, it's just usefull to pass per system parameters
	to the individual particles

************************************************************************/

class sdAbstractTemplatedParticleSystem : public sdHardcodedParticleSystem {
public:
	// These methods can be called on any class instantiaton, setting parameters etc will be class dependent anyway
	virtual void SetMaxActiveParticles( int num ) = 0;
	virtual void Update( void ) = 0;
	virtual void Init( void ) = 0;
};

template< class ParticleClass, class ParameterClass > class sdTemplatedParticleSystem : public sdAbstractTemplatedParticleSystem { 
protected:
	int maxParticles;
	int activeParticles;
	int maxActiveParticles;		// Don't activate particles if the number of active ones is higher than this (this allows "scaling" the effect without reallocating)
	ParticleClass *particles;
	const idMaterial* material;

	idVec3	viewOrg;
	idMat3	viewAxis;
	idBounds spawnBounds; // Relative to the viewer

public:

	const idBounds &	GetSpawnBounds( void ) const { return spawnBounds; }
	void				SetSpawnBounds( const idBounds &bounds ) { spawnBounds = bounds; }
	srfTriangles_t *	GetTriSurf( void ) { return renderEntity.hModel->Surface( 0 )->geometry; }
	void				SetMaterial( const char *name ) { material = declHolder.declMaterialType.LocalFind( name ); }
	void				SetMaterial( const idMaterial* material ) { this->material = material; }
	void				SetMaxParticles( int num ) { maxParticles = num; maxActiveParticles = num; } //Need to call update after this
	virtual void		SetMaxActiveParticles( int num ) { maxActiveParticles = num; } 
	const idMat3 &		GetViewAxis( void ) { return viewAxis; }
	const idVec3 &		GetViewOrg( void ) { return viewOrg; }

	virtual void Init( void );
	virtual void Update( void );

	sdTemplatedParticleSystem( void );
	virtual ~sdTemplatedParticleSystem( void );

	ParameterClass		params;
};

template< class ParticleClass, class ParameterClass > sdTemplatedParticleSystem<ParticleClass, ParameterClass>::sdTemplatedParticleSystem( void ) {
	particles = NULL;
	material = NULL;
	particles = 0;
}

template< class ParticleClass, class ParameterClass > sdTemplatedParticleSystem<ParticleClass, ParameterClass>::~sdTemplatedParticleSystem( void ) {
	delete [] particles;
}

template< class ParticleClass, class ParameterClass > void sdTemplatedParticleSystem<ParticleClass, ParameterClass>::Init( void ) {
	delete [] particles;
	particles = new ParticleClass [maxParticles];

	SetDoubleBufferedModel();
	// Initialize as much as possible here, so we only have to set the vertex positions constantly
	ClearSurfaces();
	AddSurfaceDB( material, maxParticles * ParticleClass::NUM_INSTANCE_VERTEXES, maxParticles * ParticleClass::NUM_INSTANCE_INDEXES );

	for ( int i = 0; i<maxParticles; i++ ) {
		particles[i].StaticInitializeAndRender( this );
	}
	GetTriSurf()->numVerts = 0;
	GetTriSurf()->numIndexes = 0;

	renderEntity.origin.Zero();
	renderEntity.axis.Identity();
	renderEntity.flags.noShadow = true;
	renderEntity.flags.noSelfShadow = true;
	renderEntity.flags.noDynamicInteractions = true;

	viewOrg.Zero();
	viewAxis.Identity();

	activeParticles = 0;
	PresentRenderEntity();
}

template< class ParticleClass, class ParameterClass > void sdTemplatedParticleSystem<ParticleClass, ParameterClass>::Update( void ) {
	if ( !particles ) {
		return;
	}


	renderView_t *viewToUse = NULL;
	renderView_t view;
	if ( sdDemoManager::GetInstance().CalculateRenderView( &view ) ) {
		viewToUse = &view;
	} else {
		// If we are inside don't run the bacground effect
		idPlayer* player = gameLocal.GetLocalViewPlayer();

		if ( player == NULL ) {
			return;
		}
		viewToUse = player->GetRenderView();
	}

	int dropsCreated = 0;
	int dropsActive = 0;

	SetDoubleBufferedModel();

	//common->Printf( "%d %08x\n", gameLocal.framenum, GetTriSurf() );

	GetTriSurf()->bounds.Clear();
	GetTriSurf()->numIndexes = 0;
	GetTriSurf()->numVerts = 0;
	renderEntity.hModel->FreeVertexCache();

	viewOrg = viewToUse->vieworg;
	viewAxis = viewToUse->viewaxis;

	for ( int i = 0; i<maxParticles; i++ ) {
		ParticleClass *particle = &particles[i];
		if( !particle->Update( this ) ) {
			// No new ones if we are over budget
			if ( activeParticles > maxActiveParticles ) {
				continue;
			}
			if( !particle->Initialize( this ) ) {
				continue;
			} else {
				dropsCreated++;
				activeParticles++;
			}
		}
		particle->Render( this );
		dropsActive++;		
	}
	activeParticles = dropsActive;

	renderEntity.bounds = GetTriSurf()->bounds;
	//common->Printf( "%d %08x %d %d\n", gameLocal.framenum, GetTriSurf(), GetTriSurf()->numIndexes, GetTriSurf()->numVerts );
	PresentRenderEntity();
}

#endif //__GAME_TEMPLATEDPARTICLE_SYSTEM_H__
