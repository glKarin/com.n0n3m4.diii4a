// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __SMOKEPARTICLES_H__
#define __SMOKEPARTICLES_H__

/*
===============================================================================

	Smoke systems are for particles that are emitted off of things that are
	constantly changing position and orientation, like muzzle smoke coming
	from a bone on a weapon, blood spurting from a wound, or particles
	trailing from a monster limb.

	The smoke particles are always evaluated and rendered each tic, so there
	is a performance cost with using them for continuous effects. The general
	particle systems are completely parametric, and have no performance
	overhead when not in view.

	All smoke systems share the same shaderparms, so any coloration must be
	done in the particle definition.

	Each particle model has its own shaderparms, which can be used by the
	particle materials.

===============================================================================
*/

typedef struct singleSmoke_s {
	struct singleSmoke_s	 *	next;
	int							privateStartTime;	// start time for this particular particle
	int							index;				// particle index in system, 0 <= index < stage->totalParticles
	idRandom					random;
	idVec3						origin;
	idMat3						axis;
} singleSmoke_t;

typedef struct {
	const idParticleStage *		stage;
	singleSmoke_t *				smokes;
} activeSmokeStage_t;


class idSmokeParticles {
public:
								idSmokeParticles( void );

	// creats an entity covering the entire world that will call back each rendering
	void						Init( void );
	void						Shutdown( void );

	// spits out a particle, returning false if the system will not emit any more particles in the future
	bool						EmitSmoke( const idDeclParticle *smoke, const int startTime, const float diversity,
											const idVec3 &origin, const idMat3 &axis );

	// free old smokes
	void						FreeSmokes( void );

private:
	bool						initialized;

	renderEntity_t				renderEntity;			// used to present a model to the renderer
	int							renderEntityHandle;		// handle to static renderer model

	static const int			MAX_SMOKE_PARTICLES = 10000;
	singleSmoke_t				smokes[MAX_SMOKE_PARTICLES];

	idList<activeSmokeStage_t>	activeStages;
	singleSmoke_t *				freeSmokes;
	int							numActiveSmokes;
	int							currentParticleTime;	// don't need to recalculate if == view time

	bool						UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool					ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
};

#endif /* !__SMOKEPARTICLES_H__ */
