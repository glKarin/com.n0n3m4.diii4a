// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FLARES_H__
#define __FLARES_H__

#include "HardcodedParticleSystem.h"

struct sdFlareParameters {
	float depth;
	float end_height;
	float end_width;
	float height;
	float width;
	int	  slices;
	int	  surfid;
};

class sdHardcodedParticleFlare : public sdHardcodedParticleSystem {
public:
	static void SetupParams( sdFlareParameters &params, renderEntity_t &renderEntity, const idDict& args );
	static void SetupRenderEntity( sdFlareParameters const &params, renderEntity_t *renderEntity, const renderView_t *renderView );

	bool	RenderEntityCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime );
	sdFlareParameters params;
};

class sdMiscFlare : public idEntity {
public:
	CLASS_PROTOTYPE( sdMiscFlare );

	void				Spawn( void );
	virtual void		Think( void );

	sdHardcodedParticleFlare& GetFlareSystem( void ) { return system; }

private:

	sdHardcodedParticleFlare system;
};


#endif // __FLARES_H__