// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __RAINEFFECT_H__
#define __RAINEFFECT_H__

#include "HardcodedParticleSystem.h"
#include "../Atmosphere.h"
#include "Effects.h"

class sdRainEffect : public idEntity {
public:
	CLASS_PROTOTYPE( sdRainEffect );

	void				Spawn( void );
	virtual void		Think( void );

private:
};

class sdRainPrecipitation : public sdAbstractPrecipitationSystem {
	renderEntity_t	renderEntity;
	int				renderEntityHandle;
	sdEffect		effect;
	bool			effectRunning;

	sdPrecipitationParameters parms;

	void SetupEffect( void );

public:
	sdRainPrecipitation( sdPrecipitationParameters const &_parms );
	~sdRainPrecipitation();

	renderEntity_t*	GetRenderEntity() { return &renderEntity; }

	virtual void SetMaxActiveParticles( int num );
	virtual void Update( void );
	virtual void Init( void );
	virtual void FreeRenderEntity( void );
};

#endif // __RAINEFFECT_H__