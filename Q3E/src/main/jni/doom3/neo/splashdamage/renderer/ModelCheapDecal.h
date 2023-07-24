// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __MODELCHEAPDECAL_H__
#define __MODELCHEAPDECAL_H__

/*
===============================================================================

	These are very simple models they are not clipped against the underlying
	geometry like normal decals so only small bits like bullet holes etc. should
	bee realized with this system.

===============================================================================
*/

#include "Model.h"

class idRenderModelStatic;

class sdRenderModelCheapDecal : public idRenderModelStatic {

	int numUsedJoints;
	short jointsUsage[16];
	short jointsIdx[16];
	int maxDecals;
	bool useJoints;

	bool UpdateSurface( idRenderEntityLocal *def, modelSurface_t &surf, float time );

public:
	sdRenderModelCheapDecal( int maxDecals, bool useJoints );
	~sdRenderModelCheapDecal( void );

	void AddDecal( idRenderEntityLocal *def, const cheapDecalParameters_t &params, float time );
	void AddDecalDrawSurfs( struct viewEntity_s *space );
	void AddDecalDrawSurfs( void );

	// Cleans up unused memory (will happen after all decails died for example)
	// returns true if the "this" should be deleted because all decails are gone
	bool CleanUp( void );
	void Clear( void );

};

#endif /* !__MODELCHEAPDECAL_H__ */
