// Copyright (C) 2007 Id Software, Inc.
//

/*
================================================================================================================================
================================================================================================================================
WARNING: This is included by the radiant project as well... don't try to use any gamecode stuff in here
================================================================================================================================
================================================================================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "AtmosphereRenderable.h"
//#include "Player.h"

#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/DeviceContext.h"

#include "../decllib/declAtmosphere.h"

idCVar sdAtmosphereRenderable::a_sun(						"a_sun",						"85",		CVAR_GAME | CVAR_FLOAT,	"" );
idCVar sdAtmosphereRenderable::a_glowScale(					"a_glowScale",					"0.25",		CVAR_GAME | CVAR_FLOAT,	"Blurred image contribution factor" );
idCVar sdAtmosphereRenderable::a_glowBaseScale(				"a_glowBaseScale",				"0.21",		CVAR_GAME | CVAR_FLOAT,	"Original image contribution factor" );
idCVar sdAtmosphereRenderable::a_glowThresh(				"a_glowThresh",					"0.0",		CVAR_GAME | CVAR_FLOAT,	"Threshold above which part of the scene starts glowing" );
idCVar sdAtmosphereRenderable::a_glowLuminanceDependency(	"a_glowLuminanceDependency",	"1.0",		CVAR_GAME | CVAR_FLOAT,	"Dependency of the glow on the luminance(brightness)" );
idCVar sdAtmosphereRenderable::a_glowSunPower(				"a_glowSunPower",				"16",		CVAR_GAME | CVAR_FLOAT,	"Power to raise to sun factor to" );
idCVar sdAtmosphereRenderable::a_glowSunScale(				"a_glowSunScale",				"0.0",		CVAR_GAME | CVAR_FLOAT,	"Factor to scale to sun factor with" );
idCVar sdAtmosphereRenderable::a_glowSunBaseScale(			"a_glowSunBaseScale",			"0.0",		CVAR_GAME | CVAR_FLOAT,	"Factor to scale to sun factor with" );


/*
================
sdAtmosphereRenderable::sdAtmosphereRenderable
================
*/
sdAtmosphereRenderable::sdAtmosphereRenderable( idRenderWorld* renderWorld ) {
	this->renderWorld = renderWorld;

	//
	// Setup the celestial body light source
	//
	memset( &skyLight, 0, sizeof(skyLight) );
	skyLight.flags.pointLight						= true;
	skyLight.flags.atmosphereLight					= true;
	skyLight.flags.parallel							= true;
	skyLight.axis									= mat3_identity;
	skyLight.lightRadius.Set( 50000.f, 50000.f, 50000.f );
	skyLight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.f;
	skyLight.material								= declHolder.declMaterialType.LocalFind( "_default" );

	skyLightHandle = -1;

	//
	// Setup the celestial body model
	//
	memset( &skyLightSprite, 0, sizeof( skyLightSprite ) );
	skyLightSprite.spawnID = -1;
	skyLightSprite.flags.noShadow = true;
	skyLightSprite.flags.noSelfShadow = true;

	memset( &skyLightGlowSprite, 0, sizeof( skyLightGlowSprite ) );
	skyLightGlowSprite.spawnID = -1;
	skyLightGlowSprite.flags.noShadow = true;
	skyLightGlowSprite.flags.noSelfShadow = true;

	skyLightSpriteHandle = -1;
	skyLightGlowSpriteHandle = -1;

	// Misc initialisation
	postProcessMaterial = declHolder.declMaterialType.LocalFind( "postprocess/glow" );
	spriteModel = renderModelManager->FindModel( "_SPRITE" );

	occtestHandle = -1;

	currentScale = 0.f;
	currentAlpha = 0.f;

	sunFlareMaxSize = 0.f;
	sunFlareTime = 0.f;

	Uid = renderSystem->RegisterPtr( this );
}

/*
================
sdAtmosphereRenderable::~sdAtmosphereRenderable
================
*/
sdAtmosphereRenderable::~sdAtmosphereRenderable() {
	renderSystem->UnregisterPtr( Uid );
	FreeLightDef();
	FreeModelDef();
}

/*
================
sdAtmosphereRenderable::UpdateAtmosphere
================
*/
void sdAtmosphereRenderable::UpdateAtmosphere( parms_t& parms ) {
	UpdateCelestialBody( parms );
	UpdateCloudLayers( parms );
}

/*
================
sdAtmosphereRenderable::DrawPostProcess

	Render a full screen quad with the vertex colors set up based on the atmospheric settings
================
*/
void sdAtmosphereRenderable::DrawPostProcess( const renderView_t* view, float x, float y, float w, float h ) const {
	deviceContext->DrawRect( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, postProcessMaterial );
}

/*
================
sdAtmosphereRenderable::FreeModelDef
================
*/
void sdAtmosphereRenderable::FreeModelDef() {
	for ( int i = 0; i < renderHandles.Num(); i++ ) {
		if ( renderHandles[i] != -1 ) {
			renderWorld->FreeEntityDef( renderHandles[i] );
			renderHandles[i] = -1;
		}
	}

	if ( skyLightSpriteHandle != -1 ) {
		renderWorld->FreeEntityDef( skyLightSpriteHandle );
		skyLightSpriteHandle = -1;
	}

	if ( skyLightGlowSpriteHandle != -1 ) {
		renderWorld->FreeEntityDef( skyLightGlowSpriteHandle );
		skyLightGlowSpriteHandle = -1;
	}
}


/*
================
sdAtmosphereRenderable::FreeLightDef
================
*/
void sdAtmosphereRenderable::FreeLightDef() {
	if ( skyLightHandle != -1 ) {
		renderWorld->FreeLightDef( skyLightHandle );
		skyLightHandle = -1;
	}
}

/*
================
sdAtmosphereRenderable::UpdateCelestialBody
================
*/
void sdAtmosphereRenderable::UpdateCelestialBody( parms_t& parms ) {

	//
	// Update lightsource
	//

	float sunIntensity							= Min( 1.f, ( 0.3f + ( ( a_sun.GetFloat() / 80.f ) ) ) );
	idVec3 sunColor								= parms.atmosphere->GetSunColor() * sunIntensity;
	skyLight.lightCenter						= parms.atmosphere->GetSunDirection() /** 100000.f*/;
	skyLight.minSpecShadowColor					= sdColor4::PackColor( parms.atmosphere->GetMinSpecShadowColor(), 1.f );
	skyLight.shaderParms[ SHADERPARM_RED ]		= sunColor[ 0 ];
	skyLight.shaderParms[ SHADERPARM_GREEN ]	= sunColor[ 1 ];
	skyLight.shaderParms[ SHADERPARM_BLUE ]		= sunColor[ 2 ];
	skyLight.material							= parms.atmosphere->GetSunMaterial();
	skyLight.origin								= parms.skyOrigin;
	skyLight.atmosLightProjection				= renderWorld->FindAtmosLightProjection( parms.mapId );

	if ( parms.mapId != 0 ) {
		skyLight.mapId = parms.mapId;
		skyLight.numPrelightModels = 0;
		while ( renderModelManager->CheckModel( idStr( va( "_prelightatmosphere_%d_%d", parms.mapId, skyLight.numPrelightModels ) ) ) ) {
			skyLight.numPrelightModels++;
		}
		if ( skyLight.numPrelightModels > MAX_PRELIGHTS ) {
			common->Warning( "Max number of prelights reached for atmosphere against areas" );
		}
		skyLight.numPrelightModels = Min( skyLight.numPrelightModels, MAX_PRELIGHTS );
		for (int i=0; i<skyLight.numPrelightModels; i++) {
			skyLight.prelightModels[i] = renderModelManager->CheckModel( idStr( va( "_prelightatmosphere_%d_%d", parms.mapId, i ) ) );
			assert( skyLight.prelightModels[i] != NULL );
		}
	}

	skyLight.flags.noShadows = ( sunColor.Length() <= 2/255.f );

	if ( skyLightHandle != -1 ) {
		renderWorld->UpdateLightDef( skyLightHandle, &skyLight );
	} else {
		skyLightHandle = renderWorld->AddLightDef( &skyLight );
	}

	//
	// Update model
	//

	skyLightSprite.origin = parms.skyOrigin;

	idVec3 dir = parms.atmosphere->GetSunDirection();
	//-dir.Normalize();
	skyLightSprite.axis = dir.ToMat3();

	// Setup model & shader
	skyLightSprite.hModel = spriteModel;

	skyLightSprite.customShader = parms.atmosphere->GetSunSpriteMaterial();

	// Set shader parms
	skyLightSprite.shaderParms[ SHADERPARM_RED ] = parms.atmosphere->GetSunColor()[0];
	skyLightSprite.shaderParms[ SHADERPARM_GREEN ] = parms.atmosphere->GetSunColor()[1];
	skyLightSprite.shaderParms[ SHADERPARM_BLUE ] = parms.atmosphere->GetSunColor()[2];
	skyLightSprite.shaderParms[ SHADERPARM_ALPHA ] = 1.f;
	skyLightSprite.shaderParms[4] = parms.atmosphere->GetSunZenith();

	// Set model parms (position and size)
	skyLightSprite.shaderParms[ SHADERPARM_SPRITE_OFFSET ] = 60000.0f;
	skyLightSprite.shaderParms[ SHADERPARM_SPRITE_WIDTH ] = parms.atmosphere->GetSunSpriteSize();
	skyLightSprite.shaderParms[ SHADERPARM_SPRITE_HEIGHT ] = parms.atmosphere->GetSunSpriteSize();

	skyLightSprite.bounds = skyLightSprite.hModel->Bounds( &skyLightSprite );

	skyLightSprite.flags.pushIntoConnectedOutsideAreas = true;

	if ( skyLightSpriteHandle != -1 ) {
		renderWorld->UpdateEntityDef( skyLightSpriteHandle, &skyLightSprite );
	} else {
		skyLightSpriteHandle = renderWorld->AddEntityDef( &skyLightSprite );
	}

	//
	// Update glow model
	//

	skyLightGlowSprite.origin = parms.skyOrigin;

	if ( parms.atmosphere->EnableSunFlareAziZen() ) {
		idVec3 sunFlareDir;
		float azs, azc;
		float zes, zec;

		idMath::SinCos( DEG2RAD( parms.atmosphere->GetSunFlareAzi() ), azs, azc );
		idMath::SinCos( DEG2RAD( parms.atmosphere->GetSunFlareZen() ), zes, zec );

		sunFlareDir.x = azs * zec;
		sunFlareDir.y = azc * zec;
		sunFlareDir.z = zes;

		skyLightGlowSprite.axis = sunFlareDir.ToMat3();
	} else {
		skyLightGlowSprite.axis = dir.ToMat3();
	}

	// Setup model & shader
	skyLightGlowSprite.hModel = spriteModel;

	skyLightGlowSprite.callback = glowSpriteCB;
#pragma warning( push )
#pragma warning( disable: 4312 )
	skyLightGlowSprite.callbackData = (void*)Uid;
#pragma warning( pop )

	skyLightGlowSprite.customShader = parms.atmosphere->GetSunFlareMaterial();

	// Set shader parms
	skyLightGlowSprite.shaderParms[ SHADERPARM_RED ] = parms.atmosphere->GetSunColor()[0];
	skyLightGlowSprite.shaderParms[ SHADERPARM_GREEN ] = parms.atmosphere->GetSunColor()[1];
	skyLightGlowSprite.shaderParms[ SHADERPARM_BLUE ] = parms.atmosphere->GetSunColor()[2];
	skyLightGlowSprite.shaderParms[ SHADERPARM_ALPHA ] = 1.f;
	skyLightGlowSprite.shaderParms[4] = parms.atmosphere->GetSunZenith();

	// Set model parms (position and size)
	skyLightGlowSprite.shaderParms[ SHADERPARM_SPRITE_OFFSET ] = 60000.0f;
	skyLightGlowSprite.shaderParms[ SHADERPARM_SPRITE_WIDTH ] = parms.atmosphere->GetSunFlareSize();
	skyLightGlowSprite.shaderParms[ SHADERPARM_SPRITE_HEIGHT ] = parms.atmosphere->GetSunFlareSize();
	sunFlareMaxSize = parms.atmosphere->GetSunFlareSize();
	sunFlareTime = parms.atmosphere->GetSunFlareTime();

	skyLightGlowSprite.bounds = skyLightGlowSprite.hModel->Bounds( &skyLightGlowSprite );

	skyLightGlowSprite.flags.pushIntoConnectedOutsideAreas = true;

	if ( skyLightGlowSpriteHandle != -1 ) {
		renderWorld->UpdateEntityDef( skyLightGlowSpriteHandle, &skyLightGlowSprite );
	} else {
		skyLightGlowSpriteHandle = renderWorld->AddEntityDef( &skyLightGlowSprite );
	}

}

/*
================
sdAtmosphereRenderable::UpdateCloudLayers

	This should not be called at run time, just at init or when the atmosphere editor is changing stuff
================
*/
void sdAtmosphereRenderable::UpdateCloudLayers( parms_t& parms ) {
	const idList< sdCloudLayer >& cloudLayers = parms.atmosphere->GetCloudLayers();

	// If number of layers has changed free render ents and realloc
	if ( renderEnts.Num() != cloudLayers.Num() ) {
		for ( int i = 0; i < renderHandles.Num(); i++ ) {
			if ( renderHandles[i] >= 0 ) {
				renderWorld->FreeEntityDef( renderHandles[i] );
				renderHandles[i] = -1;
			}
		}

		renderEnts.SetNum( cloudLayers.Num() );
		renderHandles.SetNum( cloudLayers.Num() );

		for ( int i = 0; i < cloudLayers.Num(); i++ ) {
			renderHandles[i] = -1;
		}
	}

	for ( int i = 0; i < cloudLayers.Num(); i++ ) {
		renderEntity_t* skyModel = &renderEnts[i];

		memset( skyModel, 0, sizeof( *skyModel ) );

		skyModel->axis.Identity();
		skyModel->hModel								= ( cloudLayers[i].style ) ? parms.boxDomeModel : parms.oldDomeModel;
		skyModel->bounds								= skyModel->hModel->Bounds( skyModel );
		skyModel->origin.x								= parms.skyOrigin.x;
		skyModel->origin.y								= parms.skyOrigin.y;
		skyModel->origin.z								= parms.skyOrigin.z + ( (cloudLayers.Num()-1-i) * 1.f );	// offset each layer
		skyModel->suppressSurfaceInViewID				= FAST_MIRROR_VIEW_ID;
		skyModel->customShader							= cloudLayers[i].material;
		skyModel->flags.pushIntoConnectedOutsideAreas	= true;
		skyModel->sortOffset							= ( i * 0.001f );	// offset each layer

		memcpy( skyModel->shaderParms, cloudLayers[i].parms, sizeof( float ) * NUM_CLOUD_LAYER_PARAMETERS );

		if ( renderHandles[i] != -1 ) {
			renderWorld->UpdateEntityDef( renderHandles[i], &renderEnts[i] );
		} else {
			renderHandles[i] = renderWorld->AddEntityDef( &renderEnts[i] );
		}
	}
}


idCVar a_glowSpriteMin(	"a_glowSpriteMin",				"0",		CVAR_GAME,	"" );
idCVar a_glowSpriteSize( "a_glowSpriteSize",				"100",		CVAR_GAME | CVAR_FLOAT,	"" );
//idCVar a_glowSpriteMaxSize( "a_glowSpriteMaxSize",			"2",		CVAR_GAME | CVAR_FLOAT,	"" );
//idCVar a_glowSpriteAlpha( "a_glowSpriteAlpha",				"100",		CVAR_GAME | CVAR_FLOAT,	"" );
//idCVar a_glowSpriteMaxAlpha( "a_glowSpriteMaxAlpha",		"0.1",		CVAR_GAME | CVAR_FLOAT,	"" );
//idCVar a_glowSpriteSizeSpeed( "a_glowSpriteSizeSpeed",				"0.5",		CVAR_GAME | CVAR_FLOAT,	"" );
//idCVar a_glowSpriteAlphaSpeed( "a_glowSpriteAlphaSpeed",				"0.99",		CVAR_GAME | CVAR_FLOAT,	"" );


/*
============
sdAtmosphereRenderable::_glowSpriteCB
============
*/
bool sdAtmosphereRenderable::_glowSpriteCB( renderEntity_t *re, const renderView_s *v, int& lastModifiedGameTime )
{
	occlusionTest_t def;
	def.view = v->viewID;
	def.bb.Zero();
	def.bb.ExpandSelf( 1000.f );
	def.bb.TranslateSelf( idVec3( re->shaderParms[ SHADERPARM_SPRITE_OFFSET ], 0.0f, 0.0f ) );
	def.origin = re->origin;
	def.axis = re->axis;
	if ( occtestHandle < 0 ) {
		occtestHandle = renderWorld->AddOcclusionTestDef(&def );
	} else {
		renderWorld->UpdateOcclusionTestDef( occtestHandle, &def );
	}

	//bool recentVisible = renderWorld->IsVisibleOcclusionTestDef( occtestHandle );
	int recentVisibleCount = renderWorld->CountVisibleOcclusionTestDef( occtestHandle ) - a_glowSpriteMin.GetInteger();

	float scale;
	float alpha;
	if ( recentVisibleCount > 0 ) {
		scale = recentVisibleCount / a_glowSpriteSize.GetFloat();
		if ( scale > 1.f ) {
			scale = 1.f;
		}
		/*alpha = recentVisibleCount / a_glowSpriteAlpha.GetFloat();
		if ( alpha > a_glowSpriteMaxAlpha.GetFloat() )
		{
			alpha = a_glowSpriteMaxAlpha.GetFloat();
		}*/
	} else {
		scale = 0.f;
		alpha = 0.f;
	}
	float dtscale = sunFlareTime * 0.001f;
	if ( dtscale > 0.f ) {
		if ( scale < currentScale ) {
			float inc = (1.f / dtscale) * 1.f/30.f;
			currentScale -= inc;
			if ( currentScale < scale ) {
				currentScale = scale;
			}
		} else {
			float inc = (1.f / dtscale) * 1.f/30.f;
			currentScale += inc;
			if ( currentScale > scale ) {
				currentScale = scale;
			}
		}
	} else {
		currentScale = scale;
	}
/*	float dtalpha = a_glowSpriteAlphaSpeed.GetFloat();
	if ( alpha < currentAlpha ) {
		float inc = (1.f / dtalpha) * 1.f/30.f;
		currentAlpha -= inc;
		if ( currentAlpha < alpha ) {
			currentAlpha = alpha;
		}
	} else {
		float inc = (1.f / dtalpha) * 1.f/30.f;
		currentAlpha += inc;
		if ( currentAlpha > alpha ) {
			currentAlpha = alpha;
		}
	}*/
	alpha = 1.0;
	re->shaderParms[ SHADERPARM_SPRITE_WIDTH ] = sunFlareMaxSize * currentScale;
	re->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] = sunFlareMaxSize * currentScale;
	re->shaderParms[ SHADERPARM_ALPHA ] = currentScale;

	return false;
}


/*
============
sdAtmosphereRenderable::glowSpriteCB
============
*/
bool sdAtmosphereRenderable::glowSpriteCB( renderEntity_t *re, const renderView_s *v, int& lastModifiedGameTime ) {
	if ( v ) {
#pragma warning( push )
#pragma warning( disable: 4311 )
		sdAtmosphereRenderable *atmos = static_cast<sdAtmosphereRenderable *>(renderSystem->PtrForUID( (int)re->callbackData ));
#pragma warning( pop )
		if ( atmos != NULL ) {
			return atmos->_glowSpriteCB( re, v, lastModifiedGameTime );
		} else {
			return false;
		}
	} else {
		return false;
	}
}
