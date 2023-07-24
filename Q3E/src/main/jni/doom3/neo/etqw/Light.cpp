// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Light.h"
#include "Target.h"

/*
===============================================================================

  idLight

===============================================================================
*/

extern const idEventDef EV_TurnOn;
extern const idEventDef EV_TurnOff;

const idEventDef EV_TurnOn( "turnOn", '\0', DOC_TEXT( "Enables the object." ), 0, "See also $event:turnOff$." );
const idEventDef EV_TurnOff( "turnOff", '\0', DOC_TEXT( "Disables the object." ), 0, "See also $event:turnOn$." );

CLASS_DECLARATION( idEntity, idLight )
	EVENT( EV_Hide,					idLight::Event_Hide )
	EVENT( EV_Show,					idLight::Event_Show )
	EVENT( EV_TurnOn,				idLight::Event_On )
	EVENT( EV_TurnOff,				idLight::Event_Off )
	EVENT( EV_Activate,				idLight::Event_ToggleOnOff )
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderLight( const idDict& args, renderLight_t& renderLight ) {
	bool	gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3	color;

	memset( &renderLight, 0, sizeof( renderLight ) );

	if ( !args.GetVector("light_origin", "", renderLight.origin)) {
		args.GetVector( "origin", "", renderLight.origin );
	}

	renderLight.mapId = args.GetInt( "spawn_mapSpawnId" );

	gotTarget = args.GetVector( "light_target", "", renderLight.target );
	gotUp = args.GetVector( "light_up", "", renderLight.up );
	gotRight = args.GetVector( "light_right", "", renderLight.right );
	args.GetVector( "light_start", "0 0 0", renderLight.start );
	if ( !args.GetVector( "light_end", "", renderLight.end ) ) {
		renderLight.end = renderLight.target;
	}

	// we should have all of the target/right/up or none of them
	if ( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) ) {
		gameLocal.Warning( "Light at %s has bad target info", renderLight.origin.ToString() );
		return;
	}

	if ( !gotTarget ) {
		renderLight.flags.pointLight = true;

		// allow an optional relative center of light and shadow offset
		args.GetVector( "light_center", "0 0 0", renderLight.lightCenter );

		// create a point light
		if (!args.GetVector( "light_radius", "300 300 300", renderLight.lightRadius ) ) {
			float radius;

			args.GetFloat( "light", "300", radius );
			renderLight.lightRadius[0] = renderLight.lightRadius[1] = renderLight.lightRadius[2] = radius;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if ( !args.GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
		if ( !args.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
	   		args.GetFloat( "angle", "0", angles[ 1 ] );
   			angles[ 0 ] = 0;
			angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
	   		angles[ 2 ] = 0;
			mat = angles.ToMat3();
		}
	}

	// fix degenerate identity matrices
	mat[0].FixDegenerateNormal();
	mat[1].FixDegenerateNormal();
	mat[2].FixDegenerateNormal();

	renderLight.axis = mat;

	// check for other attributes
	args.GetVector( "_color", "1 1 1", color );
	float overBright = args.GetFloat( "_overbright", "1.0f" );

	renderLight.shaderParms[ SHADERPARM_RED ]		= color[0] * overBright;
	renderLight.shaderParms[ SHADERPARM_GREEN ]		= color[1] * overBright;
	renderLight.shaderParms[ SHADERPARM_BLUE ]		= color[2] * overBright;
	args.GetFloat( "shaderParm3", "1", renderLight.shaderParms[ SHADERPARM_TIMESCALE ] );
	if ( !args.GetFloat( "shaderParm4", "0", renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] ) ) {
		// offset the start time of the shader to sync it to the game time
		renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args.GetFloat( "shaderParm5", "0", renderLight.shaderParms[5] );
	args.GetFloat( "shaderParm6", "0", renderLight.shaderParms[6] );
	args.GetFloat( "shaderParm7", "0", renderLight.shaderParms[ SHADERPARM_MODE ] );
	renderLight.flags.noShadows = args.GetBool( "noshadows" );
	renderLight.flags.noSpecular = args.GetBool( "nospecular" );
	renderLight.flags.parallel = args.GetBool( "parallel" );
	renderLight.flags.atmosphereLight = args.GetBool( "atmosphere" );
	renderLight.flags.insideLight = args.GetBool( "inside" );

	idStr shadowSpec = args.GetString( "shadowSpec", "low" );
	if ( shadowSpec.Icmp( "high" ) == 0 ) {
		renderLight.shadowSpec = 2;
	} else if ( shadowSpec.Icmp( "med" ) == 0 || shadowSpec.Icmp( "medium" ) == 0 ) {
		renderLight.shadowSpec = 1;
	} else if ( shadowSpec.Icmp( "low" ) == 0 ) {
		renderLight.shadowSpec = 0;
	} else {
		renderLight.shadowSpec = 0;
	}

	// Gordon: uh, precaching?
	args.GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight.material = declHolder.declMaterialType[ texture ];

	renderLight.maxVisDist = args.GetInt( "maxVisDist", "0" );

	idStr mirrorParam = args.GetString( "mirror", "always" );
	if ( mirrorParam == "mirroronly" ) {
		renderLight.allowLightInViewID = MIRROR_VIEW_ID;
	} else if ( mirrorParam == "viewonly" ) {
		renderLight.suppressLightInViewID = MIRROR_VIEW_ID;
	}
}

idList< qhandle_t > idLight::s_lightHandles;

/*
================
idLight::UpdateChangeableSpawnArgs
================
*/
void idLight::UpdateChangeableSpawnArgs( const idDict *source ) {

	idEntity::UpdateChangeableSpawnArgs( source );

	if ( source ) {
		source->Print();
	}
	FreeSoundEmitter( true );
	gameEdit->ParseSpawnArgsToRefSound( source ? *source : spawnArgs, refSound );
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_ANY, 0, false, NULL );
	}

	gameEdit->ParseSpawnArgsToRenderLight( source ? *source : spawnArgs, renderLight );

	UpdateVisuals();
}

/*
================
idLight::idLight
================
*/
idLight::idLight() {
	memset( &renderLight, 0, sizeof( renderLight ) );
	localLightOrigin	= vec3_zero;
	localLightAxis		= mat3_identity;
	lightDefHandle		= -1;
	levels				= 0;
	currentLevel		= 0;
	baseColor			= vec3_zero;
	count				= 0;
	triggercount		= 0;
	lightParent			= NULL;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
	soundWasPlaying		= false;
}

/*
================
idLight::~idLight
================
*/
idLight::~idLight() {
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}
}

/*
================
idLight::Spawn
================
*/
void idLight::Spawn( void ) {
	bool start_off;

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderLight( spawnArgs, renderLight );

	// we need the origin and axis relative to the physics origin/axis
	localLightOrigin = ( renderLight.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localLightAxis = renderLight.axis * GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );

	// set the number of light levels
	spawnArgs.GetInt( "levels", "1", levels );
	currentLevel = levels;
	if ( levels <= 0 ) {
		gameLocal.Error( "Invalid light level set on entity #%d(%s)", entityNumber, name.c_str() );
	}

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	// also put the light texture on the model, so light flares
	// can get the current intensity of the light
	renderEntity.referenceShader = renderLight.material;

	// see if an optimized shadow volume exists
	// the renderer will ignore this value after a light has been moved,
	// but there may still be a chance to get it wrong if the game moves
	// a light before the first present, and doesn't clear the prelight
	renderLight.numPrelightModels = 0;
	if ( name[ 0 ] ) {
		// this will return 0 if not found
		while ( renderModelManager->CheckModel( idStr( va( "_prelight_%s_%d", name.c_str(), renderLight.numPrelightModels ) ) ) ) {
			renderLight.numPrelightModels++;
		}
		if ( renderLight.numPrelightModels > MAX_PRELIGHTS ) {
			common->Warning( "Max number of prelights reached for '%s'", name.c_str() );
		}
		renderLight.numPrelightModels = Min( renderLight.numPrelightModels, MAX_PRELIGHTS );
		if ( renderLight.numPrelightModels ) {
			for (int i=0; i<renderLight.numPrelightModels; i++) {
				renderLight.prelightModels[i] = renderModelManager->CheckModel( idStr( va( "_prelight_%s_%d", name.c_str(), i ) ) );
				assert( renderLight.prelightModels[i] != NULL );
			}
		}
	}

	spawnArgs.GetBool( "start_off", "0", start_off );
	if ( start_off ) {
		Off();
	}

	spawnArgs.GetInt( "count", "1", count );

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;

	idStr gpuSpecParam = spawnArgs.GetString( "drawSpec", "low" );
	if ( gpuSpecParam.Icmp( "high" ) == 0 ) {
		renderLight.drawSpec = 2;
	} else if ( gpuSpecParam.Icmp( "med" ) == 0 || gpuSpecParam.Icmp( "medium" ) == 0 ) {
		renderLight.drawSpec = 1;
	} else if ( gpuSpecParam.Icmp( "low" ) == 0 ) {
		renderLight.drawSpec = 0;
	} else {
		renderLight.drawSpec = 0;
	}

	interior = spawnArgs.GetBool( "interior" );
}

/*
================
idLight::SetLightLevel
================
*/
void idLight::SetLightLevel( void ) {
	idVec3	color;
	float	intensity;

	intensity = ( float )currentLevel / ( float )levels;
	color = baseColor * intensity;
	renderLight.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderLight.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderLight.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec3 &out ) const {
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec4 &out ) const {
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderLight.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( float red, float green, float blue ) {
	baseColor.Set( red, green, blue );
	SetLightLevel();
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( const idVec4 &color ) {
	baseColor = color.ToVec3();
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= color[ 3 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	SetLightLevel();
}

/*
================
idLight::On
================
*/
void idLight::On( void ) {
	currentLevel = levels;
	// offset the start time of the shader to sync it to the game time
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	if ( ( soundWasPlaying || refSound.waitfortrigger ) && refSound.shader ) {
		StartSoundShader( refSound.shader, SND_ANY, 0, false, NULL );
		soundWasPlaying = false;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Off
================
*/
void idLight::Off( void ) {
	currentLevel = 0;
	// kill any sound it was making
	if ( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) {
		StopSound( SND_ANY );
		soundWasPlaying = true;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Fade
================
*/
void idLight::Fade( const idVec4 &to, float fadeTime ) {
	GetColor( fadeFrom );
	fadeTo = to;
	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( fadeTime );
	BecomeActive( TH_THINK );
}

/*
================
idLight::FadeOut
================
*/
void idLight::FadeOut( float time ) {
	Fade( colorBlack, time );
}

/*
================
idLight::FadeIn
================
*/
void idLight::FadeIn( float time ) {
	idVec3 color;
	idVec4 color4;

	currentLevel = levels;
	spawnArgs.GetVector( "_color", "1 1 1", color );
	color4.Set( color.x, color.y, color.z, 1.0f );
	Fade( color4, time );
}

/*
================
idLight::PresentLightDefChange
================
*/
void idLight::PresentLightDefChange( void ) {
	if ( currentLevel == 0 ) {
		FreeLightDef();
		return;
	}

	// let the renderer apply it to the world
	if ( ( lightDefHandle != -1 ) ) {
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	} else {
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
}

/*
================
idLight::PresentModelDefChange
================
*/
void idLight::PresentModelDefChange( void ) {

	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
================
idLight::Present
================
*/
void idLight::Present( void ) {

	// Arnout: CHECKME: this wasn't here, if it is causing problems, remove it again
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	// add the model
	idEntity::Present();

	// current transformation
	renderLight.axis	= localLightAxis * GetPhysics()->GetAxis();
	renderLight.origin  = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * localLightOrigin;

	// reference the sound for shader synced effects
	if ( lightParent ) {
		renderLight.referenceSound = lightParent->GetSoundEmitter();
		renderEntity.referenceSound = lightParent->GetSoundEmitter();
	}
	else {
		renderLight.referenceSound = refSound.referenceSound;
		renderEntity.referenceSound = refSound.referenceSound;
	}

	// update the renderLight and renderEntity to render the light and flare
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::Think
================
*/
void idLight::Think( void ) {
	idVec4 color;

	if ( thinkFlags & TH_THINK ) {
		if ( fadeEnd > 0 ) {
			if ( gameLocal.time < fadeEnd ) {
				color.Lerp( fadeFrom, fadeTo, ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart ) );
			} else {
				color = fadeTo;
				fadeEnd = 0;
				BecomeInactive( TH_THINK );
			}
			SetColor( color );
		}
	}

	RunPhysics();
	Present();
}

/*
================
idLight::GetPhysicsToSoundTransform
================
*/
bool idLight::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	origin = localLightOrigin + renderLight.lightCenter;
	axis = localLightAxis * GetPhysics()->GetAxis();
	return true;
}

/*
================
idLight::FreeLightDef
================
*/
void idLight::FreeLightDef( void ) {
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
		lightDefHandle = -1;
	}
}

/*
================
idLight::Event_Hide
================
*/
void idLight::Event_Hide( void ) {
	Hide();
	PresentModelDefChange();
	Off();
}

/*
================
idLight::Event_Show
================
*/
void idLight::Event_Show( void ) {
	Show();
	PresentModelDefChange();
	On();
}

/*
================
idLight::Event_On
================
*/
void idLight::Event_On( void ) {
	On();
}

/*
================
idLight::Event_Off
================
*/
void idLight::Event_Off( void ) {
	Off();
}

/*
================
idLight::Event_ToggleOnOff
================
*/
void idLight::Event_ToggleOnOff( idEntity *activator ) {
	triggercount++;
	if ( triggercount < count ) {
		return;
	}

	// reset trigger count
	triggercount = 0;

	if ( !currentLevel ) {
		On();
	} else {
		currentLevel--;
		if ( !currentLevel ) {
			Off();
		} else {
			SetLightLevel();
		}
	}
}

/*
================
idLight::Event_SetAtmosphere
================
*/
void idLight::Event_SetAtmosphere( float value ) {
	renderLight.flags.atmosphereLight = ( value != 0 );
	PresentLightDefChange();
}

/*
================
idLight::PostMapSpawn
================
*/
void idLight::PostMapSpawn( void ) {
	idEntity::PostMapSpawn();

	SetLightAreas();
	UpdateVisuals();
	Present();

	if ( g_removeStaticEntities.GetBool() && !IsDynamicLight() ) {
		if ( lightDefHandle != -1 ) {
			PushMapLight( lightDefHandle );
			lightDefHandle = -1;
		}
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idLight::PushMapLight
================
*/
void idLight::PushMapLight( int handle ) {
	s_lightHandles.Alloc() = handle;
}

/*
================
idLight::FreeMapLights
================
*/
void idLight::OnNewMapLoad( void ) {
	FreeMapLights();
}

/*
================
idLight::OnMapClear
================
*/
void idLight::OnMapClear( void ) {
	FreeMapLights();
}

/*
================
idLight::FreeMapLights
================
*/
void idLight::FreeMapLights( void ) {
	for ( int i = 0; i < s_lightHandles.Num(); i++ ) {
		gameRenderWorld->FreeLightDef( s_lightHandles[ i ] );
	}
	s_lightHandles.Clear();
}

/*
================
idLight::SetLightAreas
================
*/
void idLight::SetLightAreas() {
	if ( !interior ) {
		return;
	}

	// parse potentially visible areas
	int pvAreas[ MAX_LIGHT_AREAS ];
	int numPVAreas = 0;
	
	const char *areas;

	if ( spawnArgs.GetString( "areas", "", &areas ) ) {
		idStrList areaList;
		idSplitStringIntoList( areaList, areas, " " );
		if ( areaList.Num() ) {
			for ( int i = 0; i < areaList.Num(); i++ ) {
				if ( numPVAreas == MAX_LIGHT_AREAS ) {
					gameLocal.Warning( "light '%s' pvs contains more than %d areas", GetName(), MAX_LIGHT_AREAS );
					break;
				}

				pvAreas[ numPVAreas++ ] = atoi( areaList[i].c_str() );
			}
		}
	}

	// find targetted areas
	int			areaNum, i, j;
	idEntity*	targetEnt;

	renderLight.numAreas = 0;

	// add light origin
	idVec3 globalLightOrigin = renderLight.origin + renderLight.axis * renderLight.lightCenter;

	areaNum = gameRenderWorld->PointInArea( globalLightOrigin );
	if ( areaNum == -1 ) {
		areaNum = gameRenderWorld->PointInArea( renderLight.origin );
	}
	if ( areaNum >= 0 ) {
		renderLight.areas[ renderLight.numAreas++ ] = areaNum;
	}

	// add targets
	for ( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[i].GetEntity();

		if ( targetEnt && targetEnt->IsType( idTarget_Null::Type ) ) {
			if ( renderLight.numAreas == MAX_LIGHT_AREAS ) {
				gameLocal.Warning( "light '%s' ( %s ) is targetting more than %d areas", GetName(), globalLightOrigin.ToString(), MAX_LIGHT_AREAS );
				break;
			}

			areaNum = gameRenderWorld->PointInArea( targetEnt->GetPhysics()->GetOrigin() );
			if ( areaNum >= 0 ) {
				// check for duplicates
				for ( j = 0; j < renderLight.numAreas; j++ ) {
					if ( renderLight.areas[ j ] == areaNum ) {
						break;
					}
				}
				if ( j != renderLight.numAreas ) {
					gameLocal.Warning( "light '%s' ( %s ), duplicate reference to area %d by '%s' ( %s )", GetName(), globalLightOrigin.ToString(), areaNum, targetEnt->GetName(), targetEnt->GetPhysics()->GetOrigin().ToString() );
					continue;
				}

				if ( numPVAreas > 0 ) {
					// check if it is in the pvs
					for ( j = 0; j < numPVAreas; j++ ) {
						if ( pvAreas[ j ] == areaNum ) {
							break;
						}
					}
					if ( j == numPVAreas ) {
						gameLocal.Warning( "light '%s' ( %s ), reference to not visible area %d by '%s' ( %s )", GetName(), globalLightOrigin.ToString(), areaNum, targetEnt->GetName(), targetEnt->GetPhysics()->GetOrigin().ToString() );
						continue;
					}
				}

				// add
				renderLight.areas[ renderLight.numAreas++ ] = areaNum;
			}
		}
	}

	PresentLightDefChange();
}
