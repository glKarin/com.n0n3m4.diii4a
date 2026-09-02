
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

  idLight

===============================================================================
*/

const idEventDef EV_Light_SetShader( "setShader", "s" );
const idEventDef EV_Light_GetLightParm( "getLightParm", "d", 'f' );
const idEventDef EV_Light_SetLightParm( "setLightParm", "df" );
const idEventDef EV_Light_SetLightParms( "setLightParms", "ffff" );
const idEventDef EV_Light_SetRadiusXYZ( "setRadiusXYZ", "fff" );
const idEventDef EV_Light_SetRadius( "setRadius", "f" );
const idEventDef EV_Light_On( "On", NULL );
const idEventDef EV_Light_Off( "Off", NULL );
const idEventDef EV_Light_FadeOut( "fadeOutLight", "f" );
const idEventDef EV_Light_FadeIn( "fadeInLight", "f" );

// RAVEN BEGIN
// bdube: added
const idEventDef EV_Light_SetLightGUI ( "setLightGUI", "s" );
// jscott: added for modview
const idEventDef EV_Light_SetCurrentLightLevel( "setCurrentLightLevel", "d" );
const idEventDef EV_Light_SetMaxLightLevel( "setMaxLightLevel", "d" );
// kfuller: 8/11/03
const idEventDef EV_Light_IsOn( "isOn", NULL, 'f' );
const idEventDef EV_Light_Break( "break", "ef" );
// kfuller: lights that blink to life
const idEventDef EV_Light_DoneBlinking( "doneBlinking", NULL );
// kfuller: lights that blink off
const idEventDef EV_Light_DoneBlinkingOff( "doneBlinkingOff", NULL );
// abahr:
const idEventDef EV_Light_Timer( "<lightTimer>" );
// RAVEN END

CLASS_DECLARATION( idEntity, idLight )
	EVENT( EV_Light_SetShader,		idLight::Event_SetShader )
	EVENT( EV_Light_GetLightParm,	idLight::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,	idLight::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,	idLight::Event_SetLightParms )
	EVENT( EV_Light_SetRadiusXYZ,	idLight::Event_SetRadiusXYZ )
	EVENT( EV_Light_SetRadius,		idLight::Event_SetRadius )
	EVENT( EV_Hide,					idLight::Event_Hide )
	EVENT( EV_Show,					idLight::Event_Show )
	EVENT( EV_Light_On,				idLight::Event_On )
	EVENT( EV_Light_Off,			idLight::Event_Off )
	EVENT( EV_Activate,				idLight::Event_ToggleOnOff )
	EVENT( EV_PostSpawn,			idLight::Event_SetSoundHandles )
	EVENT( EV_Light_FadeOut,		idLight::Event_FadeOut )
	EVENT( EV_Light_FadeIn,			idLight::Event_FadeIn )

// RAVEN BEGIN
// bdube: added
	EVENT( EV_Light_SetLightGUI,				idLight::Event_SetLightGUI )
	EVENT( EV_Light_SetCurrentLightLevel,		idLight::Event_SetCurrentLightLevel )
	EVENT( EV_Light_SetMaxLightLevel,			idLight::Event_SetMaxLightLevel )
// kfuller: 8/11/03
	EVENT( EV_Light_IsOn,			idLight::Event_IsOn )
	EVENT( EV_Light_Break,			idLight::Event_Break )
// kfuller: lights that blink to life
	EVENT( EV_Light_DoneBlinking,	idLight::Event_DoneBlinking )
// kfuller: lights that blink off
	EVENT( EV_Light_DoneBlinkingOff,	idLight::Event_DoneBlinkingOff )
	EVENT( EV_Earthquake,				idLight::Event_EarthQuake )
// abahr:
	EVENT( EV_Light_Timer,				idLight::Event_Timer )
// RAVEN END
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
bool idGameEdit::ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight ) {
	bool		gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3		color;
	bool		rv = true;

	memset( renderLight, 0, sizeof( *renderLight ) );

	if (!args->GetVector("light_origin", "", renderLight->origin)) {
		args->GetVector( "origin", "", renderLight->origin );
	}

	gotTarget = args->GetVector( "light_target", "", renderLight->target );
	gotUp = args->GetVector( "light_up", "", renderLight->up );
	gotRight = args->GetVector( "light_right", "", renderLight->right );
	args->GetVector( "light_start", "0 0 0", renderLight->start );
	if ( !args->GetVector( "light_end", "", renderLight->end ) ) {
		renderLight->end = renderLight->target;
	}

	// we should have all of the target/right/up or none of them
	if ( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) ) {
		gameLocal.Warning( "Light at (%f,%f,%f) has bad target info\n",
			renderLight->origin[0], renderLight->origin[1], renderLight->origin[2] );

		return false;
	}

	if ( !gotTarget ) {
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

// RAVEN BEGIN
// bdube: default light radius changed to 320
		// create a point light
		if (!args->GetVector( "light_radius", "320 320 320", renderLight->lightRadius ) ) {
			float radius;

			args->GetFloat( "light", "320", radius );
// RAVEN END
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
		}

		if ( renderLight->lightRadius[0] == 0 ||
			 renderLight->lightRadius[1] == 0 ||
			 renderLight->lightRadius[2] == 0 ) {
			gameLocal.Warning( "PointLight at ( %d, %d, %d ) has at least one radius component of 0!",
				( int )renderLight->origin[0], ( int )renderLight->origin[1], ( int )renderLight->origin[2] );
			rv = false;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if ( !args->GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
		if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
	   		args->GetFloat( "angle", "0", angles[ 1 ] );
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

	renderLight->axis = mat;

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderLight->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= color[2];
	args->GetFloat( "shaderParm3", "1", renderLight->shaderParms[ SHADERPARM_TIMESCALE ] );
	if ( !args->GetFloat( "shaderParm4", "0", renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] ) ) {
		// offset the start time of the shader to sync it to the game time
		renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderLight->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderLight->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderLight->shaderParms[ SHADERPARM_MODE ] );
	args->GetBool( "noshadows", "0", renderLight->noShadows );

// RAVEN BEGIN
// dluetscher: added a min light detail level setting that describes when this light is visible
	args->GetFloat( "detailLevel", "10", renderLight->detailLevel );
// RAVEN END

// RAVEN BEGIN
// ddynerman: dynamic shadows
	args->GetBool( "noDynamicShadows", "0", renderLight->noDynamicShadows );
// RAVEN END
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "parallel", "0", renderLight->parallel );

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );

	return rv;
}

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
	gameEdit->ParseSpawnArgsToRefSound( source ? source : &spawnArgs, &refSound );
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	gameEdit->ParseSpawnArgsToRenderLight( source ? source : &spawnArgs, &renderLight );

	UpdateVisuals();
}

/*
================
idLight::idLight
================
*/
idLight::idLight() {
	memset( &renderLight, 0, sizeof( renderLight ) );
// RAVEN BEGIN
// dluetscher: added a default detail level to each render light
	renderLight.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// RAVEN END
	localLightOrigin	= vec3_zero;
	localLightAxis		= mat3_identity;
	lightDefHandle		= -1;
	levels				= 0;
	currentLevel		= 0;
	baseColor			= vec3_zero;
	breakOnTrigger		= false;
	count				= 0;
	triggercount		= 0;
	lightParent			= NULL;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
	soundWasPlaying		= false;
	
// RAVEN BEGIN
// bdube: light gui
	lightGUI			= NULL;
	random				= 0.0f;
	wait				= 0.0f;
// RAVEN END	
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
idLight::Save

archives object for save game file
================
*/
void idLight::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderLight( renderLight );
	
	savefile->WriteBool( renderLight.prelightModel != NULL );

	savefile->WriteVec3( localLightOrigin );
	savefile->WriteMat3( localLightAxis );
	//qhandle_t		lightDefHandle;	// cnicholson: This wasn't here from id, so I didnt add it either. 

	savefile->WriteString( brokenModel );
	savefile->WriteInt( levels );
	savefile->WriteInt( currentLevel );

	savefile->WriteVec3( baseColor );
	savefile->WriteBool( breakOnTrigger );
	savefile->WriteInt( count );
	savefile->WriteInt( triggercount );
	savefile->WriteObject( lightParent );

	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );

	lightGUI.Save( savefile );			// cnicholson: added unsaved var

	savefile->WriteBool( soundWasPlaying );
}

/*
================
idLight::Restore

unarchives object from save game file
================
*/
void idLight::Restore( idRestoreGame *savefile ) {
// RAVEN BEGIN
// jscott: constants can be read from the spawnargs
	wait = spawnArgs.GetFloat( "wait" );
	random = spawnArgs.GetFloat( "random" );
// RAVEN END

	bool hadPrelightModel;

	savefile->ReadRenderLight( renderLight );

	savefile->ReadBool( hadPrelightModel );
	renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	if ( ( renderLight.prelightModel == NULL ) && hadPrelightModel ) {
		assert( 0 );
		if ( developer.GetBool() ) {
			// we really want to know if this happens
			gameLocal.Error( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		} else {
			// but let it slide after release
			gameLocal.Warning( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		}
	}

	savefile->ReadVec3( localLightOrigin );
	savefile->ReadMat3( localLightAxis );

	savefile->ReadString( brokenModel );
	savefile->ReadInt( levels );
	savefile->ReadInt( currentLevel );

	savefile->ReadVec3( baseColor );
	savefile->ReadBool( breakOnTrigger );
	savefile->ReadInt( count );
	savefile->ReadInt( triggercount );
	savefile->ReadObject( reinterpret_cast<idClass *&>( lightParent ) );

	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
// RAVEN BEGIN
// bdube: light gui
	lightGUI.Restore ( savefile );
// RAVEN END	

	savefile->ReadBool( soundWasPlaying );
	
	lightDefHandle = -1;

	SetLightLevel();
}

/*
================
idLight::Spawn
================
*/
void idLight::Spawn( void ) {
	bool start_off;
	bool needBroken;
	const char *demonic_shader;

	// do the parsing the same way dmap and the editor do
	if ( !gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &renderLight ) ) {
		gameLocal.Warning( "Removing invalid light named: %s", GetName() );
		PostEventMS( &EV_Remove, 0 );
		return;
	}

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

	// make sure the demonic shader is cached
	if ( spawnArgs.GetString( "mat_demonic", NULL, &demonic_shader ) ) {
		declManager->FindType( DECL_MATERIAL, demonic_shader );
	}

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	// also put the light texture on the model, so light flares
	// can get the current intensity of the light
	renderEntity.referenceShader = renderLight.shader;

	lightDefHandle = -1;		// no static version yet

	// see if an optimized shadow volume exists
	// the renderer will ignore this value after a light has been moved,
	// but there may still be a chance to get it wrong if the game moves
	// a light before the first present, and doesn't clear the prelight
	renderLight.prelightModel = 0;
	if ( name[ 0 ] ) {
		// this will return 0 if not found
		renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	}

	spawnArgs.GetBool( "start_off", "0", start_off );
	if ( start_off ) {
		Off();
	}

	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetString( "broken", "", brokenModel );
	spawnArgs.GetBool( "break", "0", breakOnTrigger );
	spawnArgs.GetInt( "count", "1", count );

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;

	// if we have a health make light breakable
	if ( health ) {
		idStr model = spawnArgs.GetString( "model" );		// get the visual model
		if ( !model.Length() ) {
			gameLocal.Error( "Breakable light without a model set on entity #%d(%s)", entityNumber, name.c_str() );
		}

		fl.takedamage	= true;

		// see if we need to create a broken model name
		needBroken = true;
		if ( model.Length() && !brokenModel.Length() ) {
			int	pos;

			needBroken = false;
		
			pos = model.Find( "." );
			if ( pos < 0 ) {
				pos = model.Length();
			}
			if ( pos > 0 ) {
				model.Left( pos, brokenModel );
			}
			brokenModel += "_broken";
			if ( pos > 0 ) {
				brokenModel += &model[ pos ];
			}
		}
	
		// make sure the model gets cached
		if ( !renderModelManager->CheckModel( brokenModel ) ) {
			if ( needBroken ) {
				gameLocal.Error( "Model '%s' not found for entity %d(%s)", brokenModel.c_str(), entityNumber, name.c_str() );
			} else {
				brokenModel = "";
			}
		}

		GetPhysics()->SetContents( spawnArgs.GetBool( "nonsolid" ) ? 0 : CONTENTS_SOLID );
	}

	PostEventMS( &EV_PostSpawn, 0 );
	
// RAVEN BEGIN
// bdube: light guis	
	const char* lightGUI;
	if ( spawnArgs.GetString ( "light_gui", "", &lightGUI ) ) {	
		PostEventMS( &EV_Light_SetLightGUI, 0, lightGUI );
	}

// abahr:
	wait = spawnArgs.GetFloat( "wait" );
	random = spawnArgs.GetFloat( "random" );
// AReis: Minor light optimization stuff.
	spawnArgs.GetBool( "globalLight", "0", renderLight.globalLight );
// RAVEN END

	UpdateVisuals();

// RAVEN BEGIN
// ddynerman: ambient lights added clientside
 	if( renderLight.shader && renderLight.shader->IsAmbientLight() ) {
 		if ( !gameLocal.ambientLights.Find( static_cast<idEntity*>(this) ) ) {
 			gameLocal.ambientLights.Append( static_cast<idEntity*>(this) );
 		}
 		fl.networkSync = false; // don't transmit ambient lights
 	}
// RAVEN END
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
idLight::SetShader
================
*/
void idLight::SetShader( const char *shadername ) {
	// allow this to be NULL
	renderLight.shader = declManager->FindMaterial( shadername, false );
	PresentLightDefChange();
}

/*
================
idLight::SetLightParm
================
*/
void idLight::SetLightParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	renderLight.shaderParms[ parmnum ] = value;
	PresentLightDefChange();
}

/*
================
idLight::SetLightParms
================
*/
void idLight::SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderLight.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderLight.shaderParms[ SHADERPARM_GREEN ]		= parm1;
	renderLight.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= parm3;
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	baseColor.Set( parm0, parm1, parm2 );
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::SetRadiusXYZ
================
*/
void idLight::SetRadiusXYZ( float x, float y, float z ) {
	renderLight.lightRadius[0] = x;
	renderLight.lightRadius[1] = y;
	renderLight.lightRadius[2] = z;
	PresentLightDefChange();
}

/*
================
idLight::SetRadius
================
*/
void idLight::SetRadius( float radius ) {
	renderLight.lightRadius[0] = renderLight.lightRadius[1] = renderLight.lightRadius[2] = radius;
	PresentLightDefChange();
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

// RAVEN BEGIN
	idStr	blinkOnSound;
	if (spawnArgs.GetString("snd_blinkOn", "", blinkOnSound))
	{
		refSound.shader = declManager->FindSound(blinkOnSound);
		int howLongInMS = StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
		PostEventMS(&EV_Light_DoneBlinking, howLongInMS);
		soundWasPlaying = false;
		idStr	blinkOnTexture;
		if (spawnArgs.GetString( "mtr_blinkOn", "", blinkOnTexture ))
		{
			renderLight.shader = declManager->FindMaterial( blinkOnTexture, false );
			UpdateVisuals();
			Present();
		}
	}
	else
// RAVEN END

	if ( ( soundWasPlaying || refSound.waitfortrigger ) && refSound.shader ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
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
// RAVEN BEGIN
// kfuller: lights can flicker off
	idStr	blinkOffSound;
	if (spawnArgs.GetString("snd_blinkOff", "", blinkOffSound))
	{
		refSound.shader = declManager->FindSound(blinkOffSound);
		int howLongInMS = StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );//*1000;
		PostEventMS(&EV_Light_DoneBlinkingOff, howLongInMS);
		soundWasPlaying = false;
		idStr	blinkOffTexture;
		if (spawnArgs.GetString( "mtr_blinkOff", "", blinkOffTexture ))
		{
			renderLight.shader = declManager->FindMaterial( blinkOffTexture, false );
			UpdateVisuals();
			Present();
		}
	}
	else
	{
		currentLevel = 0;
		// kill any sound it was making
		idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
		if ( emitter && emitter->CurrentlyPlaying() ) {
			StopSound( SND_CHANNEL_ANY, false );
			soundWasPlaying = true;
		}
	}
// RAVEN END
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
idLight::Killed
================
*/
void idLight::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	BecomeBroken( attacker );
}

/*
================
idLight::BecomeBroken
================
*/
void idLight::BecomeBroken( idEntity *activator ) {
	const char *damageDefName;

	fl.takedamage = false;

	if ( brokenModel.Length() ) {
		SetModel( brokenModel );

		if ( !spawnArgs.GetBool( "nonsolid" ) ) {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_PUSH_HEAP_MEM(this);
// RAVEN END
			GetPhysics()->SetClipModel( new idClipModel( brokenModel.c_str() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_POP_HEAP();
// RAVEN END
			GetPhysics()->SetContents( CONTENTS_SOLID );
		}
	} else if ( spawnArgs.GetBool( "hideModelOnBreak" ) ) {
		SetModel( "" );
		GetPhysics()->SetContents( 0 );
	}

	if ( gameLocal.isServer ) {

		ServerSendInstanceEvent( EVENT_BECOMEBROKEN, NULL, true, -1 );

		if ( spawnArgs.GetString( "def_damage", "", &damageDefName ) ) {
			idVec3 origin = renderEntity.origin + renderEntity.bounds.GetCenter() * renderEntity.axis;
			gameLocal.RadiusDamage( origin, activator, activator, this, this, damageDefName );
		}

	}

	ActivateTargets( activator );

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	// set the state parm
	renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;
	renderLight.shaderParms[ SHADERPARM_MODE ] = 1;

	// if the light has a sound, either start the alternate (broken) sound, or stop the sound
	const char *parm = spawnArgs.GetString( "snd_broken" );
	if ( refSound.shader || ( parm && *parm ) ) {
		StopSound( SND_CHANNEL_ANY, false );
		const idSoundShader *alternate = refSound.shader ? refSound.shader->GetAltSound() : declManager->FindSound( parm );
		if ( alternate ) {
			// start it with no diversity, so the leadin break sound plays
			idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
			if( emitter ) {
				emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
				emitter->StartSound( alternate, SND_CHANNEL_ANY, 0.0, 0 );
			}
		}
	}

	parm = spawnArgs.GetString( "mtr_broken" );
	if ( parm && *parm ) {
		SetShader( parm );
	}

	UpdateVisuals();
}

/*
================
idLight::PresentLightDefChange
================
*/
void idLight::PresentLightDefChange( void ) {
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
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG( tag, MA_RENDER );
// RAVEN END
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
// RAVEN BEGIN
		renderLight.referenceSoundHandle = lightParent->GetSoundEmitter();
		renderEntity.referenceSoundHandle = lightParent->GetSoundEmitter();
	}
	else {
		renderLight.referenceSoundHandle = refSound.referenceSoundHandle;
		renderEntity.referenceSoundHandle = refSound.referenceSoundHandle;
// RAVEN END
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

// RAVEN BEGIN
// bdube: gui controlled lights		
		if ( lightGUI ) {
			SetColor ( lightGUI->GetRenderEntity()->gui[0]->GetLightColor ( ) );
		}
// RAVEN END
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
idLight::SaveState
================
*/
void idLight::SaveState( idDict *args ) {
	int i, c = spawnArgs.GetNumKeyVals();
	for ( i = 0; i < c; i++ ) {
		const idKeyValue *pv = spawnArgs.GetKeyVal(i);
		if ( pv->GetKey().Find( "editor_", false ) >= 0 || pv->GetKey().Find( "parse_", false ) >= 0 ) {
			continue;
		}
		args->Set( pv->GetKey(), pv->GetValue() );
	}
}

/*
===============
idLight::ShowEditingDialog
===============
*/
void idLight::ShowEditingDialog( void ) {
	if ( g_editEntityMode.GetInteger() == 1 ) {
		common->InitTool( EDITOR_LIGHT, &spawnArgs );
	} else {
		common->InitTool( EDITOR_SOUND, &spawnArgs );
	}
}

/*
================
idLight::Event_SetShader
================
*/
void idLight::Event_SetShader( const char *shadername ) {
	SetShader( shadername );
}

/*
================
idLight::Event_GetLightParm
================
*/
void idLight::Event_GetLightParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( renderLight.shaderParms[ parmnum ] );
}

/*
================
idLight::Event_SetLightParm
================
*/
void idLight::Event_SetLightParm( int parmnum, float value ) {
	SetLightParm( parmnum, value );
}

/*
================
idLight::Event_SetLightParms
================
*/
void idLight::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	SetLightParms( parm0, parm1, parm2, parm3 );
}

/*
================
idLight::Event_SetRadiusXYZ
================
*/
void idLight::Event_SetRadiusXYZ( float x, float y, float z ) {
	SetRadiusXYZ( x, y, z );
}

/*
================
idLight::Event_SetRadius
================
*/
void idLight::Event_SetRadius( float radius ) {
	SetRadius( radius );
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
// RAVEN BEGIN
// abahr:
	if( wait > 0 ) {
		if( EventIsPosted(&EV_Light_Timer) ) {
			CancelEvents( &EV_Light_Timer );
		} else {
			ProcessEvent( &EV_Light_Timer );
		}
	} else {
// RAVEN END
	triggercount++;
	if ( triggercount < count ) {
		return;
	}

	// reset trigger count
	triggercount = 0;

	if ( breakOnTrigger ) {
		BecomeBroken( activator );
		breakOnTrigger = false;
		return;
	}

	if ( !currentLevel ) {
		On();
	}
	else {
		currentLevel--;
		if ( !currentLevel ) {
			Off();
		}
		else {
			SetLightLevel();
		}
	}
// RAVEN BEGIN
// abahr:
	}
// RAVEN END
}

// RAVEN BEGIN
// abahr:
/*
================
idSound::Event_Timer
================
*/
void idLight::Event_Timer( void ) {
// FIXME: think about putting this logic in helper function so we don't have cut and pasted code
	if ( !currentLevel ) {
		On();
	}
	else {
		currentLevel--;
		if ( !currentLevel ) {
			Off();
		}
		else {
			SetLightLevel();
		}
	}

	PostEventSec( &EV_Light_Timer, wait + gameLocal.random.CRandomFloat() * random );
}

/*
================
idLight::Event_SetSoundHandles

  set the same sound def handle on all targeted lights
================
*/
void idLight::Event_SetSoundHandles( void ) {
	int i;
	idEntity *targetEnt;

	if ( !soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle ) ) {
		return;
	}

	for ( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( targetEnt && targetEnt->IsType( idLight::Type ) ) {
			idLight	*light = static_cast<idLight*>(targetEnt);
			light->lightParent = this;

			// explicitly delete any sounds on the entity
			light->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			light->renderEntity.referenceSoundHandle = renderEntity.referenceSoundHandle;

			// update the renderEntity to the renderer
			light->UpdateVisuals();
		}
// RAVEN BEGIN
// rjohnson: func_static's can now have their color parms affected by lights
		else if ( targetEnt && targetEnt->IsType( idStaticEntity::GetClassType() ) ) {
			targetEnt->GetRenderEntity()->referenceShader = renderLight.shader;
			targetEnt->GetRenderEntity()->referenceSoundHandle = renderEntity.referenceSoundHandle;
		}
// RAVEN END
	}	
}

/*
================
idLight::Event_FadeOut
================
*/
void idLight::Event_FadeOut( float time ) {
	FadeOut( time );
}

/*
================
idLight::Event_FadeIn
================
*/
void idLight::Event_FadeIn( float time ) {
	FadeIn( time );
}

/*
================
idLight::ClientPredictionThink
================
*/
void idLight::ClientPredictionThink( void ) {
	Think();
}

/*
================
idLight::WriteToSnapshot
================
*/
void idLight::WriteToSnapshot( idBitMsgDelta &msg ) const {

	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );

	msg.WriteByte( currentLevel );
	msg.WriteLong( PackColor( baseColor ) );
	// msg.WriteBits( lightParent.GetEntityNum(), GENTITYNUM_BITS );

/*	// only helps prediction
	msg.WriteLong( PackColor( fadeFrom ) );
	msg.WriteLong( PackColor( fadeTo ) );
	msg.WriteLong( fadeStart );
	msg.WriteLong( fadeEnd );
*/

	// FIXME: send renderLight.shader
	msg.WriteFloat( renderLight.lightRadius[0], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[1], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[2], 5, 10 );

	msg.WriteLong( PackColor( idVec4( renderLight.shaderParms[SHADERPARM_RED],
									  renderLight.shaderParms[SHADERPARM_GREEN],
									  renderLight.shaderParms[SHADERPARM_BLUE],
									  renderLight.shaderParms[SHADERPARM_ALPHA] ) ) );

	msg.WriteFloat( renderLight.shaderParms[SHADERPARM_TIMESCALE], 5, 10 );
	msg.WriteLong( renderLight.shaderParms[SHADERPARM_TIMEOFFSET] );
	//msg.WriteByte( renderLight.shaderParms[SHADERPARM_DIVERSITY] );
	msg.WriteShort( renderLight.shaderParms[SHADERPARM_MODE] );

	WriteColorToSnapshot( msg );
}

/*
================
idLight::ReadFromSnapshot
================
*/
void idLight::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idVec4	shaderColor;
	int		oldCurrentLevel = currentLevel;
	idVec3	oldBaseColor = baseColor;

	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	currentLevel = msg.ReadByte();
	if ( currentLevel != oldCurrentLevel ) {
		// need to call On/Off for flickering lights to start/stop the sound
		// while doing it this way rather than through events, the flickering is out of sync between clients
		// but at least there is no question about saving the event and having them happening globally in the world
		if ( currentLevel ) {
			On();
		} else {
			Off();
		}
	}
	UnpackColor( msg.ReadLong(), baseColor );
	// lightParentEntityNum = msg.ReadBits( GENTITYNUM_BITS );	

/*	// only helps prediction
	UnpackColor( msg.ReadLong(), fadeFrom );
	UnpackColor( msg.ReadLong(), fadeTo );
	fadeStart = msg.ReadLong();
	fadeEnd = msg.ReadLong();
*/

	// FIXME: read renderLight.shader
	renderLight.lightRadius[0] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[1] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[2] = msg.ReadFloat( 5, 10 );

	UnpackColor( msg.ReadLong(), shaderColor );
	renderLight.shaderParms[SHADERPARM_RED] = shaderColor[0];
	renderLight.shaderParms[SHADERPARM_GREEN] = shaderColor[1];
	renderLight.shaderParms[SHADERPARM_BLUE] = shaderColor[2];
	renderLight.shaderParms[SHADERPARM_ALPHA] = shaderColor[3];

	renderLight.shaderParms[SHADERPARM_TIMESCALE] = msg.ReadFloat( 5, 10 );
	renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadLong();
	//renderLight.shaderParms[SHADERPARM_DIVERSITY] = msg.ReadFloat();
	renderLight.shaderParms[SHADERPARM_MODE] = msg.ReadShort();

	ReadColorFromSnapshot( msg );

	if ( msg.HasChanged() ) {
		if ( ( currentLevel != oldCurrentLevel ) || ( baseColor != oldBaseColor ) ) {
			SetLightLevel();
		} else {
			PresentLightDefChange();
			PresentModelDefChange();
		}
	}
}

/*
================
idLight::ClientReceiveEvent
================
*/
bool idLight::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_BECOMEBROKEN: {
			BecomeBroken( NULL );
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//unreachable
//	return false;
}

// RAVEN BEGIN
// kfuller: 8/11/03
void idLight::Event_IsOn()
{
	// not entirely sure this is the best way to check for offness
	if (currentLevel == 0)
	{
		idThread::ReturnFloat( false );
	}
	else
	{
		idThread::ReturnFloat( true );
	}
}

void idLight::Event_Break(idEntity *activator, float turnOff)
{
	BecomeBroken(activator);
	if (turnOff)
	{
		Off();
	}
}

void idLight::Event_DoneBlinking()
{
	// switch to a new (possibly non-blinking) shader for the light as well as a new looping sound
	idStr	blinkedOn;
	if (spawnArgs.GetString( "mtr_doneBlinking", "", blinkedOn ))
	{
		renderLight.shader = declManager->FindMaterial( blinkedOn, false );
		UpdateVisuals();
		Present();
	}
	idStr	doneBlinkingSound;
	if (spawnArgs.GetBool("doneBlinkingNoSound"))
	{
		StopSound( SCHANNEL_ANY, false );
	}
	else if (spawnArgs.GetString("snd_doneBlinking", "", doneBlinkingSound))
	{
		StopSound( SCHANNEL_ANY, false );
		if (doneBlinkingSound.Icmp("none"))
		{
			refSound.shader = declManager->FindSound(doneBlinkingSound);
			StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
			soundWasPlaying = false;
		}
	}
}

void idLight::Event_DoneBlinkingOff()
{
	// switch light and sound off
	currentLevel = 0;
	SetLightLevel();
// RAVEN BEGIN
	// kill any sound it was making
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter && emitter->CurrentlyPlaying ( ) ) {
// RAVEN END
		StopSound( SCHANNEL_ANY, false );
		soundWasPlaying = true;
	}
}

// kfuller: want fx entities to be able to respond to earthquakes
void idLight::Event_EarthQuake(float requiresLOS)
{
	// does this entity even care about earthquakes?
	float	quakeChance = 0;

	if (!spawnArgs.GetFloat("quakeChance", "0", quakeChance))
	{
		return;
	}
	if (rvRandom::flrand(0, 1.0f) > quakeChance)
	{
		// failed its activation roll
		return;
	}

	if (requiresLOS)
	{
		bool inPVS = gameLocal.InPlayerPVS( this );

		// for lights, a line-of-sight check doesn't make as much sense, so if the quake requires an LOS check
		//we'll actually perform a PVS check
		if (!inPVS)
		{
			return;
		}
	}
	// do something with this light
	if (spawnArgs.GetBool("quakeBreak"))
	{
		spawnArgs.SetBool("quakeBreak", false);
		BecomeBroken(gameLocal.entities[ENTITYNUM_WORLD]);
		return;
	}

	float	offTime = spawnArgs.GetFloat("quakeOffTime", "1.0");
	
	Off();
	PostEventMS(&EV_Light_On, offTime*1000.0f);
}

/*
================
idLight::Event_SetLightGUI
================
*/
void idLight::Event_SetLightGUI ( const char* gui ) {
	lightGUI = gameLocal.FindEntity( gui );
	if ( lightGUI && lightGUI->GetRenderEntity() && lightGUI->GetRenderEntity()->gui[0] ) {
		BecomeActive( TH_THINK );
	} else {
		lightGUI = NULL;
	}
}

/*
================
idLight::Event_SetCurrentLightLevel
================
*/
void idLight::Event_SetCurrentLightLevel( int in ) { 
	currentLevel = in;
	SetLightLevel();
}

/*
================
idLight::Event_SetMaxLightLevel
================
*/
void idLight::Event_SetMaxLightLevel( int in ) { 
	levels = in; 
	SetLightLevel();
}

// RAVEN END
