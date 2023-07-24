// Copyright (C) 2007 Id Software, Inc.
//

/*
Atmosphere class

The level designer places this entity class in his level if he wants an atmosphere.
*/

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Atmosphere.h"
#include "AtmosphereRenderable.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "Player.h"
#include "Misc.h"
#include "../renderer/Material.h"
#include "../decllib/declTypeHolder.h"
#include "../decllib/declRenderBinding.h"
#include "effects/PrecipitationSystem.h"
#include "effects/RainEffect.h"
#include "effects/SnowEffect.h"
#include "demos/DemoManager.h"


/*
===============================================================================

	sdAbstractTemplatedParticlePrecipitationSystem
	
===============================================================================
*/

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::sdAbstractTemplatedParticlePrecipitationSystem
==============
*/
sdAbstractTemplatedParticlePrecipitationSystem::sdAbstractTemplatedParticlePrecipitationSystem( sdAbstractTemplatedParticleSystem *s ) : system( s ) {
}

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::~sdAbstractTemplatedParticlePrecipitationSystem
==============
*/
sdAbstractTemplatedParticlePrecipitationSystem::~sdAbstractTemplatedParticlePrecipitationSystem() {
	delete system;
}

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::SetMaxActiveParticles
==============
*/
void sdAbstractTemplatedParticlePrecipitationSystem::SetMaxActiveParticles( int num ) {
	system->SetMaxActiveParticles( num );
}

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::Update
==============
*/
void sdAbstractTemplatedParticlePrecipitationSystem::Update( void ) {
	system->Update();
}

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::Init
==============
*/
void sdAbstractTemplatedParticlePrecipitationSystem::Init( void ) {
	system->Init();
}

/*
==============
sdAbstractTemplatedParticlePrecipitationSystem::FreeRenderEntity
==============
*/
void sdAbstractTemplatedParticlePrecipitationSystem::FreeRenderEntity( void ) {
	system->FreeRenderEntity();
}

/*
===============================================================================

	sdAtmosphereInstance
	
===============================================================================
*/

static idCVar g_skipPrecipitation( "g_skipPrecipitation", "0",	CVAR_GAME | CVAR_BOOL,	"Enable/disable precipitation effects" );
idCVar g_skipLocalizedPrecipitation( "g_skipLocalizedPrecipitation", "0",	CVAR_GAME | CVAR_BOOL,	"Enable/disable precipitation effects" );
//static idCVar g_skipPrecipitation( "g_skipPrecipitation", "0",	CVAR_GAME | CVAR_BOOL,	"Enable/disable precipitation effects" );


static sdHeightMapInstance defaultHeightMapInst;
static sdHeightMap defaultHeightMap;
/*
============
sdAtmosphereInstance::sdAtmosphereInstance
============
*/
sdAtmosphereInstance::sdAtmosphereInstance( idDict &spawnArgs ) {
	renderable = NULL;
	for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
		precipitation[ i ] = NULL;
	}
	active = false;

	// Setup parameters
	atmosphereParms.atmosphere = declHolder.FindAtmosphere( spawnArgs.GetString( "atmosphereDecl", "default" ) );
	atmosphereParms.oldDomeModel = renderModelManager->FindModel( spawnArgs.GetString( "skymodel_old" ) );
	atmosphereParms.boxDomeModel = renderModelManager->FindModel( spawnArgs.GetString( "skymodel_box" ) );
	spawnArgs.GetVector( "origin" , "0 0 0", atmosphereParms.skyOrigin );

	atmosphereParms.mapId = spawnArgs.GetInt( "spawn_mapSpawnId" );

	renderable = new sdAtmosphereRenderable( gameRenderWorld );

	idBounds bb;
	bb.Zero();
	bb.ExpandSelf( 32000.0f );
	defaultHeightMap.Init( 8, 8, 0 );
	defaultHeightMapInst.Init( &defaultHeightMap, bb );
	//renderable->UpdateAtmosphere( atmosphereParms );
}

/*
============
sdAtmosphereInstance::~sdAtmosphereInstance
============
*/
sdAtmosphereInstance::~sdAtmosphereInstance() {
	delete renderable;
	for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
		delete precipitation[ i ];
	}
}

/*
============
sdAtmosphereInstance::Activate
============
*/
void sdAtmosphereInstance::Activate() {
	gameRenderWorld->SetAtmosphere( atmosphereParms.atmosphere );

	UpdatePrecipitationParms( true ); //may have changed

	renderable->UpdateAtmosphere( atmosphereParms );
	active = true;
}

/*
============
sdAtmosphereInstance::DeActivate
============
*/
void sdAtmosphereInstance::DeActivate() {
	active = false;
	
	// Hide stuff
	renderable->FreeModelDef();
	renderable->FreeLightDef();
	for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
		if ( precipitation[ i ] ) {
			precipitation[ i ]->FreeRenderEntity();
		}
		delete precipitation[ i ];
		precipitation[ i ] = NULL;
	}
}

/*
============
sdAtmosphereInstance::UpdatePrecipitationParms
============
*/
void sdAtmosphereInstance::UpdatePrecipitationParms( bool force ) {
	const sdHeightMapInstance* newHeightMap = &defaultHeightMapInst;

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( player != NULL ) {
		const sdPlayZone* pz = gameLocal.GetPlayZone( player->renderView.vieworg, sdPlayZone::PZF_HEIGHTMAP );
		if ( pz != NULL ) {
			newHeightMap = &pz->GetHeightMap();
		}		
	}

	if ( force || newHeightMap != cachedHeightMap ) {
		if ( newHeightMap != NULL && newHeightMap->IsValid() ) {
			SetupPrecipitation( newHeightMap, newHeightMap != cachedHeightMap );
		} else {
			for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
				delete precipitation[ i ];
				precipitation[ i ] = NULL;
			}
		}
		cachedHeightMap = newHeightMap;
	}
}

/*
============
sdAtmosphereInstance::Think
============
*/
void sdAtmosphereInstance::Think( void ) {
	UpdatePrecipitationParms( false );

	for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
		if ( precipitation[ i ] ) {
			precipitation[ i ]->Update();
		}
	}
}

/*
============
sdAtmosphereInstance::SetupPrecipitation
============
*/
void sdAtmosphereInstance::SetupPrecipitation( const sdHeightMapInstance* heightMap, bool force ) {
	for ( int i = 0; i < sdDeclAtmosphere::NUM_PRECIP_LAYERS; i++ ) {
		const sdPrecipitationParameters& newParms = atmosphereParms.atmosphere->GetPrecipitation( i );

		// If we switched it off, remove it
		if ( newParms.preType == sdPrecipitationParameters::PT_NONE || g_skipPrecipitation.GetBool() ) {
			delete precipitation[ i ];
			precipitation[ i ] = NULL;
			precParms[ i ] = newParms;
			continue;
		}

		if ( precipitation[ i ] != NULL ) {
			// Gordon: urgh.. add an operator== or something...
			// It it didn't change, do nothing
			if (
				!force &&
				precParms[ i ].preType == newParms.preType &&
				precParms[ i ].maxParticles == newParms.maxParticles &&
				precParms[ i ].heightMin == newParms.heightMin &&
				precParms[ i ].heightMax == newParms.heightMax &&
				precParms[ i ].weightMin == newParms.weightMin &&
				precParms[ i ].weightMax == newParms.weightMax &&
				precParms[ i ].timeMin == newParms.timeMin &&
				precParms[ i ].timeMax == newParms.timeMax &&
				precParms[ i ].precipitationDistance == newParms.precipitationDistance &&
				precParms[ i ].windScale == newParms.windScale &&
				precParms[ i ].gustWindScale == newParms.gustWindScale &&
				precParms[ i ].fallMin == newParms.fallMin &&
				precParms[ i ].fallMax == newParms.fallMax &&
				precParms[ i ].tumbleStrength == newParms.tumbleStrength &&
				precParms[ i ].material == newParms.material &&
				precParms[ i ].model == newParms.model &&
				precParms[ i ].effect == newParms.effect ) {	
				continue;
			}
		}
		precParms[ i ] = newParms;

		// Instantiate and setup the right class
		delete precipitation[ i ];

		switch ( precParms[ i ].preType ) {
		case sdPrecipitationParameters::PT_RAIN:
			precipitation[ i ] = new sdAbstractTemplatedParticlePrecipitationSystem( sdPrecipitationSystem< sdDrop >::SetupSystem( precParms[ i ], heightMap ) );
			break;
		case sdPrecipitationParameters::PT_SNOW:
			precipitation[ i ] = new sdAbstractTemplatedParticlePrecipitationSystem( sdPrecipitationSystem< sdFlake >::SetupSystem( precParms[ i ], heightMap ) );
			break;
		case sdPrecipitationParameters::PT_SPLASH:
			precipitation[ i ] = new sdAbstractTemplatedParticlePrecipitationSystem( sdPrecipitationSystem< sdSplash >::SetupSystem( precParms[ i ], heightMap ) );
			break;
		case sdPrecipitationParameters::PT_MODELRAIN:
			precipitation[ i ] = new sdRainPrecipitation( precParms[ i ] );
			break;
		case sdPrecipitationParameters::PT_MODELSNOW:
			precipitation[ i ] = new sdSnowPrecipitation( precParms[ i ] );
			break;
		default:
			assert( !"Unknown precip type" );
			break;
		}
	}
}

/*
===============================================================================

	sdAtmosphere
	
===============================================================================
*/

const idEventDef EV_Atmosphere_resetPostProcess( "resetPostProcess", '\0', DOC_TEXT( "Resets post process settings back to their default settings." ), 0, NULL );
const idEventDef EV_Atmosphere_getDefaultPostProcessSaturation( "getDefaultPostProcessSaturation", 'f', DOC_TEXT( "Returns the default saturation value for the current atmosphere." ), 0, NULL );
const idEventDef EV_Atmosphere_getDefaultPostProcessGlareSourceBrightness( "getDefaultPostProcessGlareSourceBrightness", 'f', DOC_TEXT( "Returns the default glare source brightness value for the current atmosphere." ), 0, NULL );
const idEventDef EV_Atmosphere_getDefaultPostProcessGlareBlurBrightness( "getDefaultPostProcessGlareBlurBrightness", 'f', DOC_TEXT( "Returns the default glare blur value for the current atmosphere." ), 0, NULL );
const idEventDef EV_Atmosphere_getDefaultPostProcessGlareBrightnessThreshold( "getDefaultPostProcessGlareBrightnessThreshold", 'f', DOC_TEXT( "Returns the default glare brightness threshold for the current atmosphere." ), 0, NULL );
const idEventDef EV_Atmosphere_getDefaultPostProcessGlareThresholdDependency( "getDefaultPostProcessGlareThresholdDependency", 'f', DOC_TEXT( "Returns the default glare threshold dependency for the current atmosphere." ), 0, NULL );
const idEventDef EV_Atmosphere_setPostProcessTint( "setPostProcessTint", '\0', DOC_TEXT( "Overrides the post process tint value." ), 1, NULL, "v", "tint", "Tint value to override with." );
const idEventDef EV_Atmosphere_setPostProcessSaturation( "setPostProcessSaturation", '\0', DOC_TEXT( "Overrides the post process saturation value." ), 1, NULL, "f", "saturation", "Saturation value to override with." );
const idEventDef EV_Atmosphere_setPostProcessContrast( "setPostProcessContrast", '\0', DOC_TEXT( "Overrides the post process constrast value." ), 1, NULL, "f", "cotrast", "Cotrast value to override with." );
const idEventDef EV_Atmosphere_setPostProcessGlareParms( "setPostProcessGlareParms", '\0', DOC_TEXT( "Overries the post process glare parms." ), 4, NULL, "f", "sourceBrightness", "Source brightness to override with.", "f", "blurBrightness", "Blur brightness to override with.", "f", "brightnessThreshold", "Brightness threshold to override with.", "f", "thresholdDependency", "Threshold dependency to override with." );
const idEventDef EV_Atmosphere_isNight( "isNight", 'b', DOC_TEXT( "Returns whether the current $decl:atmosphere$ is marked as being at night" ), 0, NULL );

CLASS_DECLARATION( idEntity, sdAtmosphere )
	EVENT( EV_Atmosphere_resetPostProcess,			sdAtmosphere::Event_ResetPostProcess )

	EVENT( EV_Atmosphere_getDefaultPostProcessSaturation,				sdAtmosphere::Event_GetDefaultPostProcessSaturation )
	EVENT( EV_Atmosphere_getDefaultPostProcessGlareSourceBrightness,	sdAtmosphere::Event_GetDefaultPostProcessGlareSourceBrightness )
	EVENT( EV_Atmosphere_getDefaultPostProcessGlareBlurBrightness,		sdAtmosphere::Event_GetDefaultPostProcessGlareBlurBrightness )
	EVENT( EV_Atmosphere_getDefaultPostProcessGlareBrightnessThreshold,	sdAtmosphere::Event_GetDefaultPostProcessGlareBrightnessThreshold )
	EVENT( EV_Atmosphere_getDefaultPostProcessGlareThresholdDependency,	sdAtmosphere::Event_GetDefaultPostProcessGlareThresholdDependency )

	EVENT( EV_Atmosphere_setPostProcessTint,		sdAtmosphere::Event_SetPostProcessTint )
	EVENT( EV_Atmosphere_setPostProcessSaturation,	sdAtmosphere::Event_SetPostProcessSaturation )
	EVENT( EV_Atmosphere_setPostProcessContrast,	sdAtmosphere::Event_SetPostProcessContrast )
	EVENT( EV_Atmosphere_setPostProcessGlareParms,	sdAtmosphere::Event_SetPostProcessGlareParms )
	
	EVENT( EV_Atmosphere_isNight, sdAtmosphere::Event_IsNight )
END_CLASS

idCVar sdAtmosphere::a_windTimeScale(			"a_windTimeScale",				"0.00005",	CVAR_GAME | CVAR_FLOAT,	"Speed at which wind effects change" );

sdAtmosphere* sdAtmosphere::currentAtmosphere = NULL;

/*
================
sdAtmosphere::FloodAmbientCubeMap
================
*/
void sdAtmosphere::FloodAmbientCubeMap( const idVec3 &origin, const sdDeclAmbientCubeMap *ambientCubeMap ) {
	int areaNum = gameRenderWorld->PointInArea( origin );

	// flood to all connected areas
	for ( int i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
		if ( i == areaNum || gameRenderWorld->AreasAreConnected( areaNum, i, PORTAL_BLOCKAMBIENT ) ) {
			gameRenderWorld->SetAreaAmbientCubeMap( i, ambientCubeMap );
		}
	}
}

/*
================
sdAtmosphere::GetAtmosphereLightDetails_f
================
*/
void sdAtmosphere::GetAtmosphereLightDetails_f( const idCmdArgs &args ) {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();

	if ( atm == NULL ) {
		gameLocal.Printf( "No atmosphere present.\n" );
		return;
	}

	idVec3 sunDir = atm->GetSunDirection();	// direction from origin TO sun, not direction sun is shining in
	idVec3 sunColor = atm->GetSunColor();

	if ( atm->GetSunDirection().z > 0.000001f ) {
		sunDir.Normalize();

		gameLocal.Printf( "Sun Direction: %s\n", sunDir.ToString() );
		gameLocal.Printf( "Sun Color: %s\n", sunColor.ToString() );

		// ugh
		sunColor.x *= .9f;
		sunColor.y *= .9f;
		gameLocal.Printf( "WARNING: sun light material modulates color: %s\n", sunColor.ToString() );
	}
}

/*
================
sdAtmosphere::SetAtmosphere_f
================
*/
void sdAtmosphere::SetAtmosphere_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: SetAtmosphere [Atmosphere Name]\n" );
		return;
	}

	const sdDeclAtmosphere *atm = declHolder.FindAtmosphere( args.Argv( 1 ), false );

	if ( atm == NULL ) {
		return;
	}
	if ( currentAtmosphere == NULL ) {
		return;
	}

	// I don't trust renderworld updates from the command system, just reset the timestamp so the gamecode picks it up later...
	currentAtmosphere->currentAtmosphereInstance->SetDecl( atm );

	const sdDeclAmbientCubeMap* ambientCubeMap = atm->GetAmbientCubeMap();	
	FloodAmbientCubeMap( currentAtmosphere->currentAtmosphereInstance->GetFloodOrigin(), ambientCubeMap );

	currentAtmosphere->forceUpdate = true;
}

/*
================
sdAtmosphere::Spawn
================
*/
void sdAtmosphere::Spawn( void ) {
	sdAtmosphereInstance* instance = new sdAtmosphereInstance( spawnArgs );
	const sdDeclAtmosphere* atmosphere = instance->GetDecl();

	if ( currentAtmosphere ) {
		// There is already an atmosphere, just creates an instance and delete yourself
		currentAtmosphere->instances.Append( instance );
		PostEventMS( &EV_Remove, 0 );
	} else {
		// This will be the actual atmosphere entity that remains in the map
		InitCommands();

		instances.Append( instance );
		
		currentAtmosphereInstance = instance;
		currentAtmosphereInstance->Activate();

		updatePostProcessFunction = GetScriptObject()->GetFunction( "OnUpdatePostProcess" );

		BecomeActive( TH_THINK );
	}

	int areaNum = gameRenderWorld->PointInArea( GetPhysics()->GetOrigin() );
	instance->SetFloodOrigin( GetPhysics()->GetOrigin() );
	
	const sdDeclAmbientCubeMap* ambientCubeMap = atmosphere->GetAmbientCubeMap();

	gameRenderWorld->SetCubemapSunProperties( ambientCubeMap, atmosphere->GetSunDirection(), atmosphere->GetSunColor() );

	FloodAmbientCubeMap( GetPhysics()->GetOrigin(), ambientCubeMap );

	// The currentAtmosphere is the single entity that will remain so we need to add it tho the areas of that entity!
	if ( currentAtmosphere == NULL ) {
		// default globally to the first atmosphere found.
		areaAtmospheres = new sdAtmosphereInstance*[ gameRenderWorld->NumAreas() ];
		for ( int i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
			areaAtmospheres[ i ] = instance;
		}
	} else {
		// flood to all connected areas
		for ( int i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
			if ( i == areaNum || gameRenderWorld->AreasAreConnected( areaNum, i ) ) {
				currentAtmosphere->areaAtmospheres[ i ] = instance;
			}
		}
	}

	if ( !currentAtmosphere ) {
		currentAtmosphere = this;
	}
}

/*
================
sdAtmosphere::InitCommands
================
*/
void sdAtmosphere::InitCommands() {
	cmdSystem->AddCommand( "getAtmosphereLightDetails",	GetAtmosphereLightDetails_f, CMD_FL_GAME | CMD_FL_CHEAT, "Get atmosphere light details" );
	cmdSystem->AddCommand( "setAtmosphere", SetAtmosphere_f, CMD_FL_GAME | CMD_FL_CHEAT, "Set the atmosphere for the current area", idCmdSystem::ArgCompletion_AtmosphereName );
}

/*
================
sdAtmosphere::sdAtmosphere
================
*/
sdAtmosphere::sdAtmosphere() :
	forceUpdate( false ) {
	currentAtmosphereInstance = NULL;
	glowPostProcessMaterial = NULL;
	areaAtmospheres = NULL;
	windVector.Zero();
	windVectorRB = declHolder.FindRenderBinding( "windWorld", false );
} 

/*
================
sdAtmosphere::~sdAtmosphere
================
*/
sdAtmosphere::~sdAtmosphere() {
	FreeLightDef();
	FreeModelDef();

	instances.DeleteContents( true );

	delete [] areaAtmospheres;

	if ( currentAtmosphere == this ) {
		currentAtmosphere = NULL;
	}
}

/*
================
sdAtmosphere::FreeModelDef
================
*/
void sdAtmosphere::FreeModelDef() {
	idEntity::FreeModelDef();
	if ( currentAtmosphereInstance ) {
		currentAtmosphereInstance->DeActivate();
	}
}

/*
================
sdAtmosphere::FreeLightDef
================
*/
void sdAtmosphere::FreeLightDef() {
	if ( currentAtmosphereInstance ) {
		currentAtmosphereInstance->DeActivate();
	}
}

/*
================
sdAtmosphere::Think
================
*/
void sdAtmosphere::Think( void ) {

	renderView_t* useView = NULL;
	
	renderView_t view;
	if ( sdDemoManager::GetInstance().CalculateRenderView( &view ) ) {
		useView = &view;
	} else {
		idPlayer* player = gameLocal.GetLocalViewPlayer();
		if ( player != NULL ) {
			useView = player->GetRenderView();
		}
	}

	// Switch atmospheres if needed
	if ( useView != NULL && areaAtmospheres != NULL ) {
		int areaNum = gameRenderWorld->PointInArea( useView->vieworg );
		if ( areaNum >= 0 && areaNum < gameRenderWorld->NumAreas() ) {
			sdAtmosphereInstance* instance = areaAtmospheres[ areaNum ];
			if ( currentAtmosphereInstance != instance ) {
				currentAtmosphereInstance->DeActivate();

				if ( instance ) {
					instance->Activate();
				}

				currentAtmosphereInstance = instance;
			}
		}
	}

	const sdDeclAtmosphere* atm = currentAtmosphereInstance->GetDecl();
	// only update if it changed
	if ( atm != NULL && atm->IsModified() || g_skipPrecipitation.IsModified() || forceUpdate ) {
		forceUpdate = false;

		currentAtmosphereInstance->Activate();
		g_skipPrecipitation.ClearModified();
	}

	UpdateWeather();
	currentAtmosphereInstance->Think();
}

/*
================
sdAtmosphere::UpdateWeather
================
*/
void sdAtmosphere::UpdateWeather() {
	const sdDeclAtmosphere* atm = currentAtmosphereInstance->GetDecl();

	float lerp = idMath::Sin( gameLocal.time * a_windTimeScale.GetFloat() );
	windAngle = atm->GetWindAngle() + lerp * atm->GetWindAngleDev();

	float s, c;
	idMath::SinCos( DEG2RAD( windAngle ), s, c );

	windVector.Set( c, s, 0.0f );
	lerp = idMath::Sin( gameLocal.time * a_windTimeScale.GetFloat() + 1.23f );
	windStrength = atm->GetWindStrength() + lerp * atm->GetWindStrengthDev();
	windVector *= windStrength;

	if ( windVectorRB != NULL ) {
		windVectorRB->Set( windVector );
	}
}

/*
================
sdAtmosphere::GetFogColor
================
*/
idVec3 sdAtmosphere::GetFogColor() {
	if ( currentAtmosphereInstance != NULL ) {
		return currentAtmosphereInstance->GetDecl()->GetFogColor();
	} else {
		return colorBlack.ToVec3();
	}
}


/*
================
sdAtmosphere::Event_ResetPostProcess
================
*/
void sdAtmosphere::Event_ResetPostProcess() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		atm->GetPostProcessParms() = atm->GetDefaultPostProcessParms();
	}
}

/*
================
sdAtmosphere::Event_GetDefaultPostProcessSaturation
================
*/
void sdAtmosphere::Event_GetDefaultPostProcessSaturation() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		sdProgram::ReturnFloat( atm->GetDefaultPostProcessParms().saturation );
	}
}

/*
================
sdAtmosphere::Event_GetDefaultPostProcessGlareSourceBrightness
================
*/
void sdAtmosphere::Event_GetDefaultPostProcessGlareSourceBrightness() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		sdProgram::ReturnFloat( atm->GetDefaultPostProcessParms().glareParms[ 0 ] );
	}
}

/*
================
sdAtmosphere::Event_GetDefaultPostProcessGlareBlurBrightness
================
*/
void sdAtmosphere::Event_GetDefaultPostProcessGlareBlurBrightness() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		sdProgram::ReturnFloat( atm->GetDefaultPostProcessParms().glareParms[ 1 ] );
	}
}

/*
================
sdAtmosphere::Event_GetDefaultPostProcessGlareBrightnessThreshold
================
*/
void sdAtmosphere::Event_GetDefaultPostProcessGlareBrightnessThreshold() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		sdProgram::ReturnFloat( atm->GetDefaultPostProcessParms().glareParms[ 2 ] );
	}
}

/*
================
sdAtmosphere::Event_GetDefaultPostProcessGlareThresholdDependency
================
*/
void sdAtmosphere::Event_GetDefaultPostProcessGlareThresholdDependency() {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		sdProgram::ReturnFloat( atm->GetDefaultPostProcessParms().glareParms[ 3 ] );
	}
}

/*
================
sdAtmosphere::Event_SetPostProcessTint
================
*/
void sdAtmosphere::Event_SetPostProcessTint( const idVec3& tint ) {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		atm->GetPostProcessParms().tint = tint;
	}
}

/*
================
sdAtmosphere::Event_SetPostProcessSaturation
================
*/
void sdAtmosphere::Event_SetPostProcessSaturation( float saturation ) {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		atm->GetPostProcessParms().saturation = saturation;
	}
}

/*
================
sdAtmosphere::Event_SetPostProcessContrast
================
*/
void sdAtmosphere::Event_SetPostProcessContrast( float contrast ) {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		atm->GetPostProcessParms().contrast = contrast;
	}
}

/*
================
sdAtmosphere::Event_SetPostProcessGlareParms
================
*/
void sdAtmosphere::Event_SetPostProcessGlareParms( float sourceBrightness, float blurBrightness, float brightnessThreshold, float thresholdDep ) {
	const sdDeclAtmosphere* atm = gameRenderWorld->GetAtmosphere();
	if ( atm != NULL ) {
		atm->GetPostProcessParms().glareParms[ 0 ] = sourceBrightness;
		atm->GetPostProcessParms().glareParms[ 1 ] = blurBrightness;
		atm->GetPostProcessParms().glareParms[ 2 ] = brightnessThreshold;
		atm->GetPostProcessParms().glareParms[ 3 ] = thresholdDep;
	}
}

/*
================
sdAtmosphere::Event_IsNight
================
*/
void sdAtmosphere::Event_IsNight( void ) {
	sdProgram::ReturnBoolean( currentAtmosphereInstance->GetDecl()->IsNight() );
}

/*
================
sdAtmosphere::DrawPostProcess
================
*/
void sdAtmosphere::DrawPostProcess( sdUserInterfaceLocal* ui, float x, float y, float w, float h ) {
	if ( currentAtmosphere == NULL ) {
		return;
	}

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( !player ) {
		return;
	}

	renderView_t* view = player->GetRenderView();
	if ( !view ) {
		return;
	}

	if ( currentAtmosphere->updatePostProcessFunction ) {
		sdScriptHelper helper;
		helper.Push( player->GetScriptObject() );
		currentAtmosphere->CallNonBlockingScriptEvent( currentAtmosphere->updatePostProcessFunction, helper );
	}
}

//========================================================================================================================================

/*
================
sdAmbientLight

	This floods an ambient cubemap into connected areas
================
*/

CLASS_DECLARATION( idEntity, sdAmbientLight )
END_CLASS

/*
================
sdAmbientLight::sdAmbientLight
================
*/
sdAmbientLight::sdAmbientLight() {
}

/*
================
sdAmbientLight::~sdAmbientLight
================
*/
sdAmbientLight::~sdAmbientLight() {
}

/*
================
sdAmbientLight::Spawn
================
*/
void sdAmbientLight::Spawn() {
	const char* temp = spawnArgs.GetString( "ambientCubeMap" );
	if ( *temp != '\0' ) {
		const sdDeclAmbientCubeMap* ambientCubeMap = declHolder.FindAmbientCubeMap( temp );

		sdAtmosphere::FloodAmbientCubeMap( GetPhysics()->GetOrigin(), ambientCubeMap );
	} else {
		gameLocal.Warning( "sdAmbientLight::Spawn : no ambientCubeMap specified on entity '%s'", GetName() );
	}

	PostEventMS( &EV_Remove, 0 );
}
