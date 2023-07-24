// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ATMOSPHERE_H__
#define __GAME_ATMOSPHERE_H__

#include "AtmosphereRenderable.h"

class sdAbstractTemplatedParticleSystem;

/***********************************************************************
* If there are several atmosphere entities in a map this is what the
* game code keeps around, all entities but the first one will be deleted
* after map load
***********************************************************************/

/*
===============================================================================

	sdAtmosphereInstance

	If there are several atmosphere entities in a map, this is what the
	game code keeps around. All entities but the first one will be deleted
	after map load.

===============================================================================
*/

class sdAbstractPrecipitationSystem {
public:
	virtual ~sdAbstractPrecipitationSystem() {}
	// These methods can be called on any class instantiaton, setting parameters etc will be class dependent anyway
	virtual void SetMaxActiveParticles( int num ) = 0;
	virtual void Update( void ) = 0;
	virtual void Init( void ) = 0;

	virtual void FreeRenderEntity( void ) = 0;
};

class sdAbstractTemplatedParticlePrecipitationSystem : public sdAbstractPrecipitationSystem {
	sdAbstractTemplatedParticleSystem *system;
public:
	sdAbstractTemplatedParticlePrecipitationSystem( sdAbstractTemplatedParticleSystem *s );

	~sdAbstractTemplatedParticlePrecipitationSystem();

	virtual void SetMaxActiveParticles( int num );
	virtual void Update( void );
	virtual void Init( void );
	virtual void FreeRenderEntity( void );
};


class sdAtmosphereInstance {
public:
										sdAtmosphereInstance( idDict &spawnArgs );
										~sdAtmosphereInstance();

	void								Activate();		// Enable this atmosphere as the current active one
	void								DeActivate();	// Do cleanup when another atmosphere is about to become active

	void								Think();

	void								SetDecl( const sdDeclAtmosphere* atmosphereDecl ) { atmosphereParms.atmosphere = atmosphereDecl; }
	const sdDeclAtmosphere*				GetDecl() const { return atmosphereParms.atmosphere; }
	void								SetFloodOrigin( const idVec3 &orig ) { origin = orig; };
	const idVec3&						GetFloodOrigin() const { return origin; }

private:
	void								SetupPrecipitation( const sdHeightMapInstance* heightMap, bool force );
	void								UpdatePrecipitationParms( bool force );

private:
	sdAtmosphereRenderable::parms_t		atmosphereParms;
	sdAtmosphereRenderable*				renderable;
	sdAbstractPrecipitationSystem*		precipitation[2];
	sdPrecipitationParameters			precParms[2];
	bool								active;
	idVec3								origin;

	const sdHeightMapInstance*			cachedHeightMap;
};

class sdAtmosphere : public idEntity {
public:
	CLASS_PROTOTYPE( sdAtmosphere );

								sdAtmosphere( void );
	virtual						~sdAtmosphere( void );

	void						Spawn( void );
	void						InitCommands( void );
	virtual void				Think( void );

	void						FreeModelDef( void );
	void						FreeLightDef( void );

	static void					GetAtmosphereLightDetails_f( const idCmdArgs &args );	

	static void					DrawPostProcess( sdUserInterfaceLocal* ui, float x, float y, float w, float h );

	idVec3						GetFogColor();

	const idVec3&				GetWindVector() const { return windVector; }
	static sdAtmosphere*		GetAtmosphereInstance() { return currentAtmosphere; }
	
	static void					SetAtmosphere_f( const idCmdArgs &args );
	static void					FloodAmbientCubeMap( const idVec3 &origin,  const sdDeclAmbientCubeMap *ambientCubeMap );

public:
	static idCVar				a_windTimeScale;
	static sdAtmosphere*		currentAtmosphere;

private:
	void						Event_ResetPostProcess();

	void						Event_GetDefaultPostProcessSaturation();
	void						Event_GetDefaultPostProcessGlareSourceBrightness();
	void						Event_GetDefaultPostProcessGlareBlurBrightness();
	void						Event_GetDefaultPostProcessGlareBrightnessThreshold();
	void						Event_GetDefaultPostProcessGlareThresholdDependency();

	void						Event_SetPostProcessTint( const idVec3& tint );
	void						Event_SetPostProcessSaturation( float saturation );
	void						Event_SetPostProcessContrast( float contrast );
	void						Event_SetPostProcessGlareParms( float sourceBrightness, float blurBrightness, float brightnessThreshold, float thresholdDep );

	void						UpdateWeather();

	void						Event_IsNight( void );
private:
	const idMaterial*			glowPostProcessMaterial;

	float						windAngle;
	float						windStrength;
	idVec3						windVector;

	const sdProgram::sdFunction*	updatePostProcessFunction;

	sdAtmosphereInstance*			currentAtmosphereInstance;
	idList< sdAtmosphereInstance* > instances;

	sdAtmosphereInstance**			areaAtmospheres;

	const sdDeclRenderBinding *		windVectorRB;

	bool						forceUpdate;
};


class sdAmbientLight : public idEntity {
public:
	CLASS_PROTOTYPE( sdAmbientLight );

	sdAmbientLight( void );
	virtual						~sdAmbientLight( void );

	void						Spawn( void );
};

#endif /* !__GAME_ATMOSPHERE_H__ */
