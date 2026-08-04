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

#ifndef __DECLPARTICLE_H__
#define __DECLPARTICLE_H__

class idImage;

/*
===============================================================================

	idDeclParticle

===============================================================================
*/

#include "renderer/resources/ParticleSystem.h"

//
// single particle stage
//
class idParticleStage : public idPartStageData {
public:
							idParticleStage( void );
	virtual					~idParticleStage( void ) {}

	void					Default();
	virtual int				NumQuadsPerParticle() const;	// includes trails and cross faded animations

	const char *			GetCustomPathName();
	const char *			GetCustomPathDesc();
	int						NumCustomPathParms();
	void					SetCustomPathType( const char *p );

	//stgatilov #4957: single place which finds and loads collisionStatic images
	static const char *		GetCollisionStaticDirectory();
	static idStr			GetCollisionStaticImagePath(const idPartSysEmitterSignature &signature);
	static idImageAsset *	LoadCutoffTimeMap(const char *imagePath, bool defer);

	//------------------------------

	const idMaterial *		material;

	bool					hidden;				// for editor use

	//-----------------------------------

	//bounding box of particles in "standard" coordinate system --- transformation and world gravity not applied yet
	idBounds				stdBounds;			// derived

	/* Soft particles -- SteveL #3878
	-2.0 is the value at initialization, meaning no user specification: "auto".
	-1.0 means no change to old system: suppress soft particles, but allow modelDepthhack if specified.
	 0   means disable all softening for this stage, including modelDepthHack.
	 +ve value means apply soft particle effect, allowing overdraw up to the specified depth. 
	This is more flexible even when not using soft particles, as modelDepthHack 
	can be turned off for specific stages to stop them poking through walls.
	*/
	float					softeningRadius;


	// stgatilov: Particle "time" is multiplied by the number from this map (in range [0..1])
	// If this is less than "1" at tex coords where particle emits, then its death moment happens earlier
	// Note: fade-out time moment is not changed, since this is collision (TODO: do we need to support both cases?)
	idImageAsset *			cutoffTimeMap;
};


//
// group of particle stages
//
class idDeclParticle : public idDecl {
public:

	virtual size_t			Size( void ) const override;
	virtual const char *	DefaultDefinition( void ) const override;
	virtual bool			Parse( const char *text, const int textLength ) override;
	virtual void			FreeData( void ) override;

	//note: bounds are returned in entity space
	//they are correct when particle origin/axis are trivial
	//do NOT apply them in particle deform case!
	static idBounds			GetStageBounds( const struct renderEntity_s *ent, idParticleStage *stage );
	idBounds				GetFullBounds( const struct renderEntity_s *ent ) const;

	bool					Save( const char *fileName = NULL );

	idList<idParticleStage *>stages;
	float					depthHack;

private:
	bool					RebuildTextSource( void );
	idParticleStage *		ParseParticleStage( idLexer &src );
	void					ParseParms( idLexer &src, float *parms, int maxParms );
	void					ParseParametric( idLexer &src, idParticleParm *parm );
	void					WriteStage( idFile *f, idParticleStage *stage );
	void					WriteParticleParm( idFile *f, idParticleParm *parm, const char *name );
};

#endif /* !__DECLPARTICLE_H__ */
