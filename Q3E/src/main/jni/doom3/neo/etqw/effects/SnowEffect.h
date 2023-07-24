// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SNOWEFFECT_H__
#define __SNOWEFFECT_H__

#include "HardcodedParticleSystem.h"
#include "../Atmosphere.h"
#include "Effects.h"

class sdSnowEffect : public idEntity {
public:
	CLASS_PROTOTYPE( sdSnowEffect );

	void				Spawn( void );
	virtual void		Think( void );

private:

	struct grouping {
		idVec3 axis;
		float rotate;
		float rotateSpeed;
		idVec3 rotationPoint;
		idVec3 worldPos;
		float time;
		float alpha;
	};

	static const int MAX_GROUPS = 32;

	grouping groups[ MAX_GROUPS ];
};


class sdSnowPrecipitation : public sdAbstractPrecipitationSystem {
	renderEntity_t	renderEntity;
	int				renderEntityHandle;

	struct grouping {
		idVec3 axis;
		float rotate;
		float rotateSpeed;
		idVec3 rotationPoint;
		idVec3 worldPos;
		float time;
		float alpha;
	};

	static const int MAX_GROUPS = 32;

	grouping groups[ MAX_GROUPS ];

	sdEffect		effect;
	bool			effectRunning;

	sdPrecipitationParameters parms;

	void SetupEffect( void );

public:
	sdSnowPrecipitation( sdPrecipitationParameters const &parms );
	~sdSnowPrecipitation();

	renderEntity_t*	GetRenderEntity() { return &renderEntity; }

	virtual void SetMaxActiveParticles( int num );
	virtual void Update( void );
	virtual void Init( void );
	virtual void FreeRenderEntity( void );
};


#endif // __SNOWEFFECT_H__