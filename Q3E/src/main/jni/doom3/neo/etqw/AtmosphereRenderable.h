// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ATMOSPHERERENDERABLE_H__
#define __GAME_ATMOSPHERERENDERABLE_H__

#include "../framework/CVarSystem.h"

class idRenderWorld;

class sdAtmosphereRenderable {
public:
	struct parms_t {
									parms_t() :
										mapId( 0 ) {
									}

		const sdDeclAtmosphere*		atmosphere;
		idRenderModel*				boxDomeModel;
		idRenderModel*				oldDomeModel;
		idVec3						skyOrigin;

		int							mapId;
	};

								sdAtmosphereRenderable( idRenderWorld* renderWorld );
	virtual						~sdAtmosphereRenderable();

	void						UpdateAtmosphere( parms_t& parms );

	void						DrawPostProcess( const renderView_t* view, float x, float y, float w, float h ) const;

	void						FreeModelDef();
	void						FreeLightDef();

	bool						IsLightValid() const { return skyLightHandle != -1; }

public:
	static idCVar				a_sun;
	static idCVar				a_glowScale;
	static idCVar				a_glowBaseScale;
	static idCVar				a_glowThresh;
	static idCVar				a_glowLuminanceDependency;
	static idCVar				a_glowSunPower;
	static idCVar				a_glowSunScale;
	static idCVar				a_glowSunBaseScale;

private:
	void						UpdateCelestialBody( parms_t& parms );
	void						UpdateCloudLayers( parms_t& parms );

	bool _glowSpriteCB( renderEntity_t*, const renderView_s*, int& lastModifiedGameTime );


private:
	idRenderWorld*				renderWorld;
	int							Uid;

	idList< renderEntity_t >	renderEnts;
	idList< qhandle_t >			renderHandles;

	renderLight_t				skyLight;
	qhandle_t					skyLightHandle;

	renderEntity_t				skyLightSprite;
	qhandle_t					skyLightSpriteHandle;

	renderEntity_t				skyLightGlowSprite;
	qhandle_t					skyLightGlowSpriteHandle;

	qhandle_t					occtestHandle;

	const idMaterial*			postProcessMaterial;
	idRenderModel*				spriteModel;

	float						currentScale;
	float						currentAlpha;

	float						sunFlareMaxSize;
	float						sunFlareTime;

	static bool glowSpriteCB( renderEntity_t*, const renderView_s*, int& lastModifiedGameTime );
};

#endif /* __GAME_ATMOSPHERERENDERABLE_H__ */
