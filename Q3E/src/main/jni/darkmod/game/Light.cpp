/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "StimResponse/StimResponseCollection.h"
#include "Grabber.h"

/*
===============================================================================

  idLight

===============================================================================
*/


const idEventDef EV_Light_SetShader( "setShader", EventArgs( 's', "shader", "" ), EV_RETURNS_VOID, "Sets the shader to be used for the light." );
const idEventDef EV_Light_GetShader( "getShader", EventArgs(), 's', "Gets the shader name used by the light." ); // #3765
const idEventDef EV_Light_GetLightParm( "getLightParm", EventArgs('d', "parmNum", ""), 'f', "Gets a shader parameter." );
const idEventDef EV_Light_SetLightParm( "setLightParm", EventArgs('d', "parmNum", "", 'f', "value", ""), EV_RETURNS_VOID, "Sets a shader parameter.");
const idEventDef EV_Light_SetLightParms( "setLightParms", 
	EventArgs('f', "parm0", "", 'f', "parm1", "", 'f', "parm2", "", 'f', "parm3", ""), EV_RETURNS_VOID, 
	"Sets the red/green/blue/alpha shader parms on the light and the model.");
const idEventDef EV_Light_SetRadiusXYZ( "setRadiusXYZ", EventArgs('f', "x", "", 'f', "y", "", 'f', "z", ""), EV_RETURNS_VOID, 
	"Sets the width/length/height of the light bounding box.");
const idEventDef EV_Light_SetRadius( "setRadius", EventArgs('f', "radius", ""), EV_RETURNS_VOID, "Sets the size of the bounding box, x=y=z=radius.");
const idEventDef EV_Light_GetRadius( "getRadius", EventArgs(), 'v', "Returns the light radius." );
const idEventDef EV_Light_On( "On", EventArgs(), EV_RETURNS_VOID, "Turns the entity on.");
const idEventDef EV_Light_Off( "Off", EventArgs(), EV_RETURNS_VOID, "Turns the entity off.");
const idEventDef EV_Light_FadeOut( "fadeOutLight", EventArgs('f', "time", "in seconds"), EV_RETURNS_VOID, "Turns the light out over the given time in seconds.");
const idEventDef EV_Light_FadeIn( "fadeInLight", EventArgs('f', "time", "in seconds"), EV_RETURNS_VOID, "Turns the light on over the given time in seconds.");

// TDM Additions:
const idEventDef EV_Light_GetLightOrigin( "getLightOrigin", EventArgs(), 'v', "Get the light origin (independent of its visual model)" );
const idEventDef EV_Light_SetLightOrigin( "setLightOrigin", EventArgs('v', "pos", ""), EV_RETURNS_VOID, "Set origin of lights independent of model origin");
const idEventDef EV_Light_GetLightLevel ("getLightLevel", EventArgs(), 'f', "Get level (intensity) of a light, <= 0.0 indicates it is off");
const idEventDef EV_Light_AddToLAS("_addToLAS", EventArgs(), EV_RETURNS_VOID, "internal");
const idEventDef EV_Light_FadeToLight( "fadeToLight", EventArgs('v', "color", "", 'f', "time", ""), EV_RETURNS_VOID, "Fades the light to the given color over a given time." );
const idEventDef EV_Smoking("smoking", EventArgs('d', "state", "1 = smoking, 0 = not smoking"), EV_RETURNS_VOID, "flame is now smoking (1), or not (0)");
const idEventDef EV_SetStartedOff("setStartedOff", EventArgs(), EV_RETURNS_VOID, "no description"); // grayman #2905


CLASS_DECLARATION( idEntity, idLight )
	EVENT( EV_Light_SetShader,		idLight::Event_SetShader )
	EVENT( EV_Light_GetLightParm,	idLight::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,	idLight::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,	idLight::Event_SetLightParms )
	EVENT( EV_Light_SetRadiusXYZ,	idLight::Event_SetRadiusXYZ )
	EVENT( EV_Light_SetRadius,		idLight::Event_SetRadius )
	EVENT( EV_Light_GetRadius,		idLight::Event_GetRadius )
	EVENT( EV_Hide,					idLight::Event_Hide )
	EVENT( EV_Show,					idLight::Event_Show )
	EVENT( EV_Light_On,				idLight::Event_On )
	EVENT( EV_Light_Off,			idLight::Event_Off )
	EVENT( EV_Activate,				idLight::Event_ToggleOnOff )
	EVENT( EV_PostSpawn,			idLight::Event_SetSoundHandles )
	EVENT( EV_Light_FadeOut,		idLight::Event_FadeOut )
	EVENT( EV_Light_FadeIn,			idLight::Event_FadeIn )

	EVENT( EV_Light_SetLightOrigin, idLight::Event_SetLightOrigin )
	EVENT( EV_Light_GetLightOrigin, idLight::Event_GetLightOrigin )
	EVENT( EV_Light_GetLightLevel,	idLight::Event_GetLightLevel )
	EVENT( EV_Light_AddToLAS,		idLight::Event_AddToLAS )
	EVENT( EV_InPVS,				idLight::Event_InPVS )
	EVENT( EV_Light_FadeToLight,	idLight::Event_FadeToLight )
	EVENT( EV_Smoking,				idLight::Event_Smoking )		// grayman #2603
	EVENT( EV_SetStartedOff,		idLight::Event_SetStartedOff )	// grayman #2905
	EVENT( EV_Light_GetShader,		idLight::Event_GetShader )		// SteveL  #3765
END_CLASS

const idStrList areaLockOptions {
	"origin", "center"
}; // not sure how to make it work with char*[]  

/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight ) {
	bool	gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3	color;

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
		gameLocal.Printf( "Light at (%f,%f,%f) has bad target info\n",
			renderLight->origin[0], renderLight->origin[1], renderLight->origin[2] );
		return;
	}

	if ( !gotTarget ) {
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

		// create a point light
		if (!args->GetVector( "light_radius", "300 300 300", renderLight->lightRadius ) ) {
			float radius;

			args->GetFloat( "light", "300", radius );
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
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
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "parallel", "0", renderLight->parallel );
	// stgatilov #5121: parallel light starting in all sky areas
	args->GetBool( "parallelSky", "0", renderLight->parallelSky );
	if (renderLight->parallelSky)
		renderLight->parallel = true;

	args->GetBool( "noFogBoundary", "0", renderLight->noFogBoundary ); // Stops fogs drawing and fogging their bounding boxes -- SteveL #3664
	args->GetBool( "noPortalFog", "0", renderLight->noPortalFog ); // Prevents fog from prematurely closing portals. Same as the material flag from Doom 3 -- nbohr1more #6282
	args->GetInt( "spectrum", "0", renderLight->spectrum );

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );

	if ( !args->GetBool( "ai_see", "1") ) // SteveL #4128
	{
		renderLight->suppressLightInViewID = VID_LIGHTGEM;
	} 
	renderLight->suppressInSubview = args->GetInt( "suppressInSubview" );

	const char* areaLock;
	if (args->GetString("areaLock", "", &areaLock))
		renderLight->areaLock = (renderEntity_s::areaLock_t) (areaLockOptions.FindIndex(areaLock) + 1);

	renderLight->volumetricDust = 0.0f;
	renderLight->volumetricNoshadows = renderLight->noShadows;
	if ( args->GetBool( "volumetric_light" ) ) {
		if ( renderLight->shader && renderLight->shader->IsFogLight() ) {
			common->Warning(
				"Volumetric fog lights should NOT be used!\n"
				"This feature is not finished yet and is subject to change.\n"
				"See more details in #5889."
			);
		}
		args->GetFloat( "volumetric_dust", "0.002", renderLight->volumetricDust );
		if ( args->FindKey( "volumetric_noshadows" ) )
			renderLight->volumetricNoshadows = args->GetInt( "volumetric_noshadows", "$%" );
	}
}

/*
================
idLight::UpdateChangeableSpawnArgs
================
*/
void idLight::UpdateChangeableSpawnArgs( const idDict *source ) {

	idEntity::UpdateChangeableSpawnArgs( source );

	/*if ( source ) {
		source->Print();
	}
	FreeSoundEmitter( true );
	gameEdit->ParseSpawnArgsToRefSound( source ? source : &spawnArgs, &refSound );
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}*/

	gameEdit->ParseSpawnArgsToRenderLight( source ? source : &spawnArgs, &renderLight );

	UpdateVisuals();
}

/*
================
idLight::idLight
================
*/
idLight::idLight()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX %s\r", this, __FUNCTION__);

	memset( &renderLight, 0, sizeof( renderLight ) );
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
	switchList.Clear();				// grayman #2603 - list of my switches
	beingRelit			= false;	// grayman #2603
	chanceNegativeBark	= 1.0f;		// grayman #2603
	whenTurnedOff		= 0;		// grayman #2603
	nextTimeLightOutBark = 0;		// grayman #2603
	relightAfter		= 0;		// grayman #2603
	aiBarks.Clear();				// grayman #2603

	startedOff			= false;	// grayman #2905

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
	soundWasPlaying		= false;
	m_MaxLightRadius	= 0.0f;
	m_BlendlightTexture = NULL; // SteveL #3752

	/*!
	Darkmod LAS
	*/
	LASAreaIndex = -1;
}

/*
================
idLight::~idLight
================
*/
idLight::~idLight()
{
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}

	/*!
	* Darkmod LAS
	*/
	// Remove light from LAS
	if (LASAreaIndex != -1)
	{
		LAS.removeLight(this);
	}

	/*!
	Darkmod player lighting
	*/
	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player != NULL)
	{
		player->RemoveLight(this);
	}

	// grayman #3584 - if ambient, remove from ambient list

	if (IsAmbient())
	{
		gameLocal.m_ambientLights.Remove(this);
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

	savefile->WriteInt( levels );
	savefile->WriteInt( currentLevel );

	savefile->WriteVec3( baseColor );
	savefile->WriteBool( breakOnTrigger );
	savefile->WriteInt( count );
	savefile->WriteInt( triggercount );
	savefile->WriteObject( lightParent );
	savefile->WriteBool(beingRelit);			// grayman #2603
	savefile->WriteFloat(chanceNegativeBark);	// grayman #2603
	savefile->WriteInt(whenTurnedOff);			// grayman #2603
	savefile->WriteInt(nextTimeLightOutBark);	// grayman #2603
	savefile->WriteInt(relightAfter);			// grayman #2603
	savefile->WriteFloat(nextTimeVerticalCheck);	// grayman #2603
	savefile->WriteBool(smoking);					// grayman #2603
	savefile->WriteInt(whenToDouse);				// grayman #2603
	savefile->WriteBool(startedOff);				// grayman #2905

	savefile->WriteInt(switchList.Num());	// grayman #2603
	for (int i = 0; i < switchList.Num(); i++)
	{
		switchList[i].Save(savefile);
	}

	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
	savefile->WriteBool( soundWasPlaying );

	// grayman #2603 - ai bark counts

	savefile->WriteInt(aiBarks.Num());
	for (int i = 0 ; i < aiBarks.Num() ; i++)
	{
		AIBarks barks = aiBarks[i];

		savefile->WriteInt(barks.count);
		barks.ai.Save(savefile);
	}

	savefile->WriteFloat(m_MaxLightRadius);
	savefile->WriteInt(LASAreaIndex);

	savefile->WriteMaterial(m_BlendlightTexture); // #3752

	// Don't save m_LightMaterial
}

/*
================
idLight::Restore

unarchives object from save game file
================
*/
void idLight::Restore( idRestoreGame *savefile ) {
	bool hadPrelightModel;

	savefile->ReadRenderLight( renderLight );

	savefile->ReadBool( hadPrelightModel );
	renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	if ( ( renderLight.prelightModel == NULL ) && hadPrelightModel ) {
		assert( 0 );
		if ( com_developer.GetBool() ) {
			// we really want to know if this happens
			gameLocal.Error( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		} else {
			// but let it slide after release
			gameLocal.Warning( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		}
	}

	savefile->ReadVec3( localLightOrigin );
	savefile->ReadMat3( localLightAxis );

	savefile->ReadInt( levels );
	savefile->ReadInt( currentLevel );

	savefile->ReadVec3( baseColor );
	savefile->ReadBool( breakOnTrigger );
	savefile->ReadInt( count );
	savefile->ReadInt( triggercount );
	savefile->ReadObject( reinterpret_cast<idClass *&>( lightParent ) );
	savefile->ReadBool( beingRelit );			// grayman #2603
	savefile->ReadFloat( chanceNegativeBark );	// grayman #2603
	savefile->ReadInt( whenTurnedOff );			// grayman #2603
	savefile->ReadInt( nextTimeLightOutBark );	// grayman #2603
	savefile->ReadInt( relightAfter );			// grayman #2603
	savefile->ReadFloat(nextTimeVerticalCheck);	// grayman #2603
	savefile->ReadBool(smoking);				// grayman #2603
	savefile->ReadInt(whenToDouse);				// grayman #2603
	savefile->ReadBool(startedOff);				// grayman #2905
	
	// grayman #2603
	switchList.Clear();
	int num;
	savefile->ReadInt(num);
	switchList.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		switchList[i].Restore(savefile);
	}

	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
	savefile->ReadBool( soundWasPlaying );

	// grayman #2603 - ai bark counts

	savefile->ReadInt(num);
	aiBarks.SetNum(num);
	for (int i = 0 ; i < num ; i++)
	{
		savefile->ReadInt(aiBarks[i].count);
		aiBarks[i].ai.Restore(savefile);
	}

	savefile->ReadFloat(m_MaxLightRadius);
	savefile->ReadInt(LASAreaIndex);

	savefile->ReadMaterial(m_BlendlightTexture); // #3752

	lightDefHandle = -1;

	SetLightLevel();
}

/*
================
idLight::Spawn
================
*/
void idLight::Spawn( void )
{
	const char *demonic_shader;

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &renderLight );

	renderLight.entityNum = entityNumber;

	// we need the origin and axis relative to the physics origin/axis
	localLightOrigin = ( renderLight.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localLightAxis = renderLight.axis * GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );

	// set the number of light levels
	spawnArgs.GetInt( "levels", "1", levels );
	currentLevel = levels;
	if ( levels <= 0 )
	{
		gameLocal.Error( "Invalid light level set on entity #%d(%s)", entityNumber, name.c_str() );
	}

	// make sure the demonic shader is cached
	if ( spawnArgs.GetString( "mat_demonic", NULL, &demonic_shader ) )
	{
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
	if ( name[ 0 ] )
	{
		// this will return 0 if not found
		renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	}

	// grayman #2905 - remember if the light started off because it's important during relighting attempts

	bool startOff = spawnArgs.GetBool( "start_off", "0" );
	if ( startOff )
	{
		Event_SetStartedOff();
		Off();
	}

	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetBool( "break", "0", breakOnTrigger );
	spawnArgs.GetInt( "count", "1", count );

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart = 0;
	fadeEnd = 0;

	// load visual and collision models
	LoadModels();

	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();

	if ( renderLight.pointLight == true )
	{
		m_MaxLightRadius = renderLight.lightRadius.Length();
	}
	else // projected light
	{
		idVec3 pos = GetPhysics()->GetOrigin();
		idVec3 max = renderLight.target + renderLight.right + renderLight.up;
		m_MaxLightRadius = max.Length();
	}
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("this: %08lX [%s] MaxLightRadius: %f\r", this, name.c_str(), m_MaxLightRadius);

	idImageAsset *pImage;
	if ( ( renderLight.shader != NULL ) && ( (pImage = renderLight.shader->LightFalloffImage()) != NULL ) )
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light has a falloff image: %08lX\r", pImage);
	}

	// grayman #2603 - set up flames for vertical check

	idStr lightType = spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);
	if (lightType == AIUSE_LIGHTTYPE_TORCH)
	{
		if (!spawnArgs.GetBool("should_be_vert","0")) // don't check verticality if it doesn't matter
		{
			nextTimeVerticalCheck = idMath::INFINITY; // never
		}
		else
		{
			nextTimeVerticalCheck = gameLocal.time + 3000 + gameLocal.random.RandomFloat()*3000; // randomize so checks are done at different times
		}
	}
	else // non-flames
	{
		nextTimeVerticalCheck = idMath::INFINITY; // never
	}
	
	smoking = false;	// grayman #2603

	whenToDouse = -1;	// grayman #2603

	// Sophisiticated Zombie (DMH)
	// Darkmod Light Awareness System: Also need to add light to LAS

	PostEventMS(&EV_Light_AddToLAS, 40);

	// grayman #3584
	if (IsAmbient())
	{
		gameLocal.m_ambientLights.Append(this); // add this light to the list of ambient lights
	}

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Spawning light: %08lX [%s] | noShadows: %u | noSpecular: %u | pointLight: %u | parallel: %u\r",
		this, name.c_str(),
		renderLight.noShadows,
		renderLight.noSpecular,
		renderLight.pointLight,
		renderLight.parallel);

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Red: %f | Green: %f | Blue: %f\r", baseColor.x, baseColor.y, baseColor.z);

	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Origin", GetPhysics()->GetOrigin());
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Radius", renderLight.lightRadius);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Center", renderLight.lightCenter);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Target", renderLight.target);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Right", renderLight.right);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Up", renderLight.up);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Start", renderLight.start);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "End", renderLight.end);


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
tels: idLight::GetLightOrigin returns the origin of the light in the world. This
is different from the physics origin, since the light can be offset.
================
void idLight::GetLightOrigin( idVec3 &out ) const {
	out[0] = renderLight.origin[0];
	out[1] = renderLight.origin[1];
	out[2] = renderLight.origin[2];
}
*/

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
void idLight::SetColor( const idVec3 &color )
{
	// Tels: If the light is currently fading, stop this:
	fadeEnd = 0;
	BecomeInactive( TH_THINK );
	baseColor = color;
	SetLightLevel();
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( const idVec4 &color ) {
	// Tels: If the light is currently fading, stop this:
	fadeEnd = 0;
	BecomeInactive( TH_THINK );
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
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::SetRadiusXYZ
================
*/
void idLight::SetRadiusXYZ( const float x, const float y, const float z ) {
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
void idLight::SetRadius( const float radius ) {
	renderLight.lightRadius[0] = renderLight.lightRadius[1] = renderLight.lightRadius[2] = radius;
	PresentLightDefChange();
}

/*
 * ================
 * Tels: idLight:GetRadius
 * ================
 * */
void idLight::GetRadius( idVec3 &out ) const {
    out.x = renderLight.lightRadius[0];
    out.y = renderLight.lightRadius[1];
    out.z = renderLight.lightRadius[2];
}

void idLight::Hide( void ) {
	idEntity::Hide();
	Off();
}

void idLight::Show( void ) {
	idEntity::Show();
	On();
}


/*
================
idLight::On
================
*/
void idLight::On( void )
{
	currentLevel = levels;
	// offset the start time of the shader to sync it to the game time
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	if ( ( soundWasPlaying || refSound.waitfortrigger ) && refSound.shader ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
		soundWasPlaying = false;
	}

	const function_t* func = scriptObject.GetFunction("LightsOn");
	if (func == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("idLight::On: 'LightsOn' not found in local space, checking for global.\r");
		func = gameLocal.program.FindFunction("LightsOn");
	}

	if (func != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("idLight::On: turning light on\r");
		idThread *pThread = new idThread(func);
		pThread->CallFunction(this,func, true);
		pThread->DelayedStart(0);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("idLight::On: 'LightsOn' not found!\r");
	}

	aiBarks.Clear(); // grayman #2603 - let the AI comment again
	
//	grayman #2603 - let script change skins, plus set the vis stim.

/*	const char *skinName;
	const idDeclSkin *skin;

	// Tels: set "skin_lit" if it is defined
	spawnArgs.GetString( "skin_lit", "", &skinName );
	skin = declManager->FindSkin( skinName );
	if (skin) {
		SetSkin( skin );
		// set the spawnarg to the current active skin
		spawnArgs.Set( "skin", skinName );
	}
 */	
	
	// SteveL #3752: blend lights need their shader restoring to turn "on"
	if ( m_BlendlightTexture != NULL )
	{
		SetShader( m_BlendlightTexture->GetName() );
		m_BlendlightTexture = NULL;
	}

	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Off
================
*/
void idLight::Off( const bool stopSound )
{
	currentLevel = 0;

	if ( stopSound && refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) {
		StopSound( SND_CHANNEL_ANY, false );
		soundWasPlaying = true;
	}

	const function_t* func = scriptObject.GetFunction("LightsOff");
	if (func == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("idLight::Off: 'LightsOff' not found in local space, checking for global.\r");
		func = gameLocal.program.FindFunction("LightsOff");
	}

	if (func != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("idLight::Off: turning light off\r");
		idThread *pThread = new idThread(func);
		pThread->CallFunction(this,func,true);
		pThread->DelayedStart(0);
		whenTurnedOff = gameLocal.time; // grayman #2603
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("idLight::Off: 'LightsOff' not found!\r");
	}

//	grayman #2603 - let script change skins, plus set the vis stim.

/*	// Tels: set "skin_unlit" if it is defined
	const char *skinName;
	const idDeclSkin *skin;
	spawnArgs.GetString( "skin_unlit", "", &skinName );
	skin = declManager->FindSkin( skinName );
	if (skin) {
		SetSkin( skin );
		// set the spawnarg to the current active skin
		spawnArgs.Set( "skin", skinName );
	}
 */	

	// SteveL #3752: blend lights need their shader removing to turn "off"
	if (IsBlend())
	{
		// Remember that this happened and what the shader was, as 
		// IsBlend() will be false after resetting shader.
		m_BlendlightTexture = renderLight.shader;
		SetShader("");
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
	// Tels: we already have the same color we should become, so we can skip this
	if (fadeFrom == to)
	{
		return;
	}
	// Tels: If the fade time is shorter than 1/60 (e.g. one frame), just set the color directly
	if (fadeTime < 0.0167f)
	{
		if (to == colorBlack)
		{
			// The fade does not happen (time too short), so Off() would not be called so do it now (#2440)
			// avoid the sound stopping, because this might be snd_extinguished
			Off( false );
		}
		else
		{
			SetColor(to);
		}
		return;
	}
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
	if (baseColor.x == 0 && baseColor.y == 0 && baseColor.z == 0)
	{
		// The fade would not happen, so Off() would not be called, so do it now (#2440)
		// avoid the sound stopping, because this might be snd_extinguished
		Off( false );
	}
	else
	{
		// Tels: Think() will call Off() once the fade is done, since we use colorBlack as fade target
		Fade( colorBlack, time );
	}
}

/*
================
idLight::FadeIn
================
*/
void idLight::FadeIn( float time )
{
	idVec3 color;
	idVec4 color4;

	currentLevel = levels;
	// restore the original light color
	spawnArgs.GetVector( "_color", "1 1 1", color );
	color4.Set( color.x, color.y, color.z, 1.0f );
	Fade( color4, time );
	On(); // grayman #3584 - turn light on, otherwise skin won't change and renderer won't paint the light volume
}

/*
================
idLight::FadeTo
================
*/
void idLight::FadeTo( idVec3 color, float time ) {

	idVec4 color4;

	// tels: TODO: this step sets the intensity of the light to the maximum,
	//		 do we really want this?
	currentLevel = levels;
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

int idLight::GetLightLevel() const
{
	return currentLevel;
}

/*
================
idLight::BecomeBroken
================
*/
void idLight::BecomeBroken( idEntity *activator ) {
	idEntity::BecomeBroken ( activator );

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
			refSound.referenceSound->StartSound( alternate, SND_CHANNEL_ANY, 0.0, 0 );
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
void idLight::PresentLightDefChange( void )
{
/*
DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s] Radius ( %0.3f / %0.3f / %03f )\r", this, name.c_str(),
		renderLight.lightRadius[0],		// x
		renderLight.lightRadius[1],		// y
		renderLight.lightRadius[2]);	// z
*/
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
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	// Clear the bounds, so idLight::PresentRenderTrigger() has a way to know
	// if idEntity::PresentRenderTrigger() added anything.
	m_renderTrigger.bounds.Clear();

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
	PresentRenderTrigger();
}

/*
================
idLight::IsVertical
*/

bool idLight::IsVertical(float degreesFromVertical)
{
	idStr lightType = spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);
	bool shouldBeVert = spawnArgs.GetBool("should_be_vert","0");

	if ((lightType == AIUSE_LIGHTTYPE_TORCH) && shouldBeVert)
	{
		const idVec3& gravityNormal = GetPhysics()->GetGravityNormal();
		idMat3 axis = GetPhysics()->GetAxis();
		idVec3 result(0,0,0);
		axis.ProjectVector(-gravityNormal,result);
		float verticality = result * (-gravityNormal);
		if (verticality < idMath::Cos(DEG2RAD(degreesFromVertical))) // > 10 degrees from vertical
		{
			return false;
		}
	}
	return true;
}

/*
================
idLight::Think
================
*/
void idLight::Think( void ) {
	idVec4 color;
	if ( thinkFlags & TH_THINK )
	{
		if ( fadeEnd > 0 )
		{
			if ( gameLocal.time < fadeEnd )
			{
				color.Lerp( fadeFrom, fadeTo, ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart ) );
			}
			else
			{
				color = fadeTo;
				fadeEnd = 0;
				// Tels: Fix issues like 2440: FadeOff() does not switch the light to the off state
				if ( ( color[0] == 0 ) && ( color[1] == 0 ) && ( color[2] == 0 ) )
				{
					// avoid the sound stopping, because this might be snd_extinguished
					Off( false );
				}
				BecomeInactive( TH_THINK );
			}
			// don't call SetColor(), as it stops the fade, instead inline the second part of it:
			baseColor = color.ToVec3();
			renderLight.shaderParms[ SHADERPARM_ALPHA ]		= color[ 3 ];
			renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
			SetLightLevel();
		}
	}

	// grayman #2603 - every so often, check if a lit flame not being held by an AI is non-vertical (> 45 degrees from vertical). if so, douse it

	if (gameLocal.time >= nextTimeVerticalCheck)
	{
		nextTimeVerticalCheck = gameLocal.time + 1000;

		if (GetLightLevel() > 0) // is it on?
		{
			bool isVertical = IsVertical(45); // w/in 45 degrees of vertical?
			if (!isVertical)
			{
				// is an AI holding this light?

				bool isHeld = false;
				idEntity* bindMaster = GetBindMaster();
				while (bindMaster != NULL) // not held when bindMaster == NULL
				{
					if (bindMaster->IsType(idAI::Type))
					{
						isHeld = true;
						break;
					}
					bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
				}

				if (!isHeld)
				{
					// Is the player holding this light?

					CGrabber* grabber = gameLocal.m_Grabber;
					if (grabber)
					{
						idEntity* heldEnt = grabber->GetSelected();
						if (heldEnt)
						{
							bindMaster = GetBindMaster();
							while (bindMaster != NULL) // not held when bindMaster == NULL
							{
								if (heldEnt == bindMaster)
								{
									isHeld = true;
									break;
								}
								bindMaster = bindMaster->GetBindMaster();
							}
						}
					}
				}

				if (!isHeld)
				{
					if (whenToDouse == -1)
					{
						whenToDouse = gameLocal.time + 5000; // douse this later if still non-vertical
						BecomeActive(TH_DOUSING); // set this so you can continue thinking after coming to rest and TH_PHYSICS shuts off
					}
					else if (gameLocal.time >= whenToDouse)
					{
						CallScriptFunctionArgs("frob_extinguish", true, 0, "e", this);
						whenToDouse = -1; // reset
						BecomeInactive(TH_DOUSING); // reset
					}
				}
				else
				{
					whenToDouse = -1; // non-vertical, but it's being held, so turn off any latched non-vertical douse
					BecomeInactive(TH_DOUSING); // reset
				}
			}
			else
			{
				whenToDouse = -1; // vertical, so turn off any latched non-vertical douse
				BecomeInactive(TH_DOUSING); // reset
			}
		}
	}
	
	idEntity::Think();
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
idLight::Event_GetShader
================
*/
void idLight::Event_GetShader( ) {
	const char * shaderName = renderLight.shader->GetName();
	if ( idStr::Cmp( shaderName, "_emptyname" ) == 0 )
	{
		shaderName = "";
	}
	idThread::ReturnString( shaderName );
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
tels: idLight::Event_GetRadius
================
*/
void idLight::Event_GetRadius( ) const {
	idThread::ReturnVector( GetRadius() );
}

/*
================
idLight::Event_Hide
================
*/
void idLight::Event_Hide( void ) {
	Hide();
	smoking = false; // grayman #2603
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

	if ( breakOnTrigger ) {
		BecomeBroken( activator );
		breakOnTrigger = false;
		return;
	}

	if ( !currentLevel )
	{
		On();
	}
	else
	{
		currentLevel--;
		if ( !currentLevel )
		{
			Off();
		}
		else
		{
			SetLightLevel();
		}
	}
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

	if ( !refSound.referenceSound ) {
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
			light->renderEntity.referenceSound = renderEntity.referenceSound;

			// update the renderEntity to the renderer
			light->UpdateVisuals();
		}
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
idLight::Event_FadeToLight
================
*/
void idLight::Event_FadeToLight( idVec3 &color, float time ) {
	FadeTo( color, time );
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
//	return false;
}


/**	Returns a bounding box surrounding the light.
 */
idBounds idLight::GetBounds()
{
	// NOTE: I need to add caching.

	idBounds b;

	if ( renderLight.pointLight )
	{
		b = idBounds( -renderLight.lightRadius, renderLight.lightRadius );
	} else {
		//gameLocal.Warning("idLight::GetBounds() not correctly implemented for projected lights.");
		// Fake a set of bounds. This might work ok for squarish spotlights with no start/stop specified.
		// FIXME: These bounds are incorrect.
		b.Zero();
		b.AddPoint( renderLight.target );
		b.AddPoint( renderLight.target + renderLight.up + renderLight.right );
		b.AddPoint( renderLight.target + renderLight.up - renderLight.right );
		b.AddPoint( renderLight.target - renderLight.up + renderLight.right );
		b.AddPoint( renderLight.target - renderLight.up - renderLight.right );
	}

	return b;
}


/**	Called to update m_renderTrigger after the render entity is modified.
 *	Only updates the render trigger if a thread is waiting for it.
 */
void idLight::PresentRenderTrigger()
{
	if ( !m_renderWaitingThread ) {
		goto Quit;
	}

	// Have the bounds been set yet?
	if ( m_renderTrigger.bounds.IsCleared() )
	{
		// No.

		// Pre-rotate our bounds and give them to m_renderTrigger.
		m_renderTrigger.origin = renderLight.origin;
		m_renderTrigger.bounds = GetBounds() * renderLight.axis;

	} else {
		// Yes.

		// Convert our light's bounds to be relative to m_renderTrigger,
		// and add them to what already exists.
		m_renderTrigger.bounds += GetBounds() * renderLight.axis + ( renderLight.origin - m_renderTrigger.origin );
	}

	// I haven't yet figured out where renderEntity.entityNum is set...
	m_renderTrigger.entityNum = entityNumber;

	// Update the renderTrigger in the render world.
	if ( m_renderTriggerHandle == -1 )
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	else
		gameRenderWorld->UpdateEntityDef( m_renderTriggerHandle, &m_renderTrigger );

	Quit:
	return;
}


int idLight::GetTextureIndex(float x, float y, int w, int h, int bpp)
{
	int rc = 0;
	float w2, h2;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Calculating texture index: x/y: %f/%f w/h: %u/%u\r", x, y, w, h);

	w2 = ((float)w)/2.0;
	h2 = ((float)h)/2.0;

	if(renderLight.pointLight)
	{
		// The lighttextures are spread out over the range of the axis. The z axis
		// can be ignored for this and we assume that the x/y is already properly
		// calculated.
		x = w2 - (w2/renderLight.lightRadius.x) * x;
		y = h2 - (h2/renderLight.lightRadius.y) * y;
	}
	else
	{
		// TODO: Just for now until the projected lights are implemented to not cause any crash.
		// x = right
		// y = up
//#pragma message(DARKMOD_NOTE "-------------------------------------------------idLight::GetTextureIndex")
//#pragma message(DARKMOD_NOTE "For projected lights this is likely not enough. As long as the light will")
//#pragma message(DARKMOD_NOTE "be straight up or down it should work, but if the cone is angled it might")
//#pragma message(DARKMOD_NOTE "give wrong results. greebo: See DarkRadiant code for starters.")
//#pragma message(DARKMOD_NOTE "-------------------------------------------------idLight::GetTextureIndex")
		x = w2 - (w2/renderLight.right.x) * x;
		y = h2 - (h2/renderLight.up.y) * y;
	}

	if(y > h)
		y = h;
	if(x > w)
		x = w;

	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	rc = ((int)y * (w*bpp)) + ((int)x*bpp);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Index result: %u   x/y: %f/%f\r", rc, x, y);

	return rc;
}

float idLight::GetDistanceColor(float fDistance, float fx, float fy)
{
	float fColVal(0), fImgVal(0);
	int fw(0), fh(0), iw(0), ih(0), i(0), fbpp(0), ibpp(0);
	const unsigned char *img = NULL;
	const unsigned char *fot = NULL;

	//stgatilov #5665: this is dead code since earlier than 2.00 
	//in reality, the pointers were always NULL here
	//see https://forums.thedarkmod.com/index.php?/topic/21002-devil-and-images-infrastructure/&do=findComment&comment=462706
#if 0
	if (m_LightMaterial == NULL)
	{
		if ( (m_LightMaterial = g_Global.GetMaterial(m_MaterialName)) != NULL )
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Material found for [%s]\r", name.c_str());
			fot = m_LightMaterial->GetFallOffTexture(fw, fh, fbpp);
			img = m_LightMaterial->GetImage(iw, ih, ibpp);
		}
	}
	else
	{
		fot = m_LightMaterial->GetFallOffTexture(fw, fh, fbpp);
		img = m_LightMaterial->GetImage(iw, ih, ibpp);
	}
#endif

	// baseColor gives the current color (intensity)

	fColVal = (baseColor.x * DARKMOD_LG_RED + baseColor.y * DARKMOD_LG_GREEN + baseColor.z * DARKMOD_LG_BLUE);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Pointlight: %u  Red: %f/%f  Green: %f/%f  Blue: %f/%f  fColVal: %f\r", renderLight.pointLight,
		baseColor.x, baseColor.x * DARKMOD_LG_RED,
		baseColor.y, baseColor.y * DARKMOD_LG_GREEN,
		baseColor.z, baseColor.z * DARKMOD_LG_BLUE,
		fColVal);

	// If we have neither falloff texture nor a projection image, we do a 
	// simple linear falloff
	if ( ( fot == NULL ) && ( img == NULL ) )
	{
		// TODO: Light falloff calculation
//#pragma message(DARKMOD_NOTE "------------------------------------------------idLight::GetDistanceColor")
//#pragma message(DARKMOD_NOTE "The lightfalloff should be calculated for ellipsoids instead of spheres")
//#pragma message(DARKMOD_NOTE "when no textures are defined. The current code will give wrong results")
//#pragma message(DARKMOD_NOTE "when a light is defined as an ellipsoid.")
//#pragma message(DARKMOD_NOTE "------------------------------------------------idLight::GetDistanceColor")
		fColVal = (fColVal / m_MaxLightRadius) * (m_MaxLightRadius - fDistance);
		fImgVal = 1;
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("No textures defined, using distance method: [%f]\r", fDistance);
	}
	else
	{
		// If we have a falloff texture ...
		if ( fot != NULL )
		{
			i = GetTextureIndex((float)fabs(fx), (float)fabs(fy), fw, fh, fbpp);
			fColVal = fColVal * (fot[i] * DARKMOD_LG_SCALE);
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Falloff: Index: %u   Value: %u [%f]\r", i, (int)fot[i], (float)(fot[i] * DARKMOD_LG_SCALE));
		}
		else
		{
			fColVal = 1;
		}

		// ... or a projection image.
		if ( img != NULL )
		{
			i = GetTextureIndex((float)fabs(fx), (float)fabs(fy), iw, ih, ibpp);
			fImgVal = img[i] * DARKMOD_LG_SCALE;
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Map: Index: %u   Value: %u [%f]\r", i, (int)img[i], (float)(img[i] * DARKMOD_LG_SCALE));
		}
		else
		{
			fImgVal = 1;
		}
	}

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Final ColVal: %f   ImgVal: %f\r", fColVal, fImgVal);

	return (fColVal*fImgVal);
}

bool idLight::CastsShadow(void)
{
	//stgatilov #5665: use idMaterial to check for ambient light
	if (renderLight.shader && renderLight.shader->IsAmbientLight())
		return false;

	return !renderLight.noShadows; 
}

bool idLight::GetLightCone(idVec3 &Origin, idVec3 &Target, idVec3 &Right, idVec3 &Up, idVec3 &Start, idVec3 &End)
{
	bool rc = false;

	Origin = GetPhysics()->GetOrigin();
	Target = renderLight.target;
	Right = renderLight.right;
	Up = renderLight.up;
	Start = renderLight.start;
	End = renderLight.end;

	return rc;
}

bool idLight::GetLightCone(idVec3 &Origin, idVec3 &Axis, idVec3 &Center)
{
	bool rc = false;

	Origin = GetPhysics()->GetOrigin();
	Axis = renderLight.lightRadius;
	// grayman #3524 - take entity rotation into account
	Center = GetPhysics()->GetAxis()*renderLight.lightCenter;
	//Center = renderLight.lightCenter;

	return rc;
}

void idLight::Event_SetLightOrigin( idVec3 &pos )
{
	localLightOrigin = pos;
	BecomeActive( TH_UPDATEVISUALS );
}

void idLight::Event_GetLightOrigin( void )
{
	idThread::ReturnVector( localLightOrigin );
}

void idLight::Event_GetLightLevel ( void )
{
	idThread::ReturnFloat( currentLevel );
}

void idLight::Event_AddToLAS()
{
	LAS.addLight(this);

	// Also register this light with the local player
	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player != NULL)
	{
		player->AddLight(this);
	}
}

/**	Returns 1 if the light is in PVS.
 *	Doesn't take into account vis-area optimizations for shadowcasting lights.
 */
void idLight::Event_InPVS()
// NOTE: Current extremely inefficent.
// Caching needs to be done.
{
	int localNumPVSAreas, localPVSAreas[32];
	idBounds modelAbsBounds;

	m_renderTrigger.bounds = GetBounds(); // currently too innefficient but I'll worry about caching later
	m_renderTrigger.origin = renderLight.origin;
	m_renderTrigger.axis = renderLight.axis;

	modelAbsBounds.FromTransformedBounds( m_renderTrigger.bounds, m_renderTrigger.origin, m_renderTrigger.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	idThread::ReturnFloat( gameLocal.pvs.InCurrentPVS( gameLocal.GetPlayerPVS(), localPVSAreas, localNumPVSAreas ) );
}

// Returns true if this is an ambient light. - J.C.Denton
bool idLight::IsAmbient(void) const
{
	if( renderLight.shader )
		return renderLight.shader->IsAmbientLight();

	return false; 
}

// Returns true if this is a fog light. - tels
bool idLight::IsFog(void) const
{
	if( renderLight.shader )
		return renderLight.shader->IsFogLight();

	return false; 
}

// Returns true if this is a blend light. - tels
bool idLight::IsBlend(void) const
{
	if( renderLight.shader )
		return renderLight.shader->IsBlendLight();

	return false; 
}

// Whether the light affects lightgem and visbility of other objects to AI #4128
bool idLight::IsSeenByAI( void ) const
{
	// For efficiency, re-use the renderLight lightgem flag rather than check the 
	// "ai_see" spawnarg in an operation that happens very often.
	return renderLight.suppressLightInViewID != VID_LIGHTGEM;
}

// grayman #2603 - keep a list of switches for this light
void idLight::AddSwitch(idEntity* newSwitch)
{
	switchList.AddUnique(newSwitch);
}

// grayman #2603 - If there are switches, return the closest one to the calling user
idEntity* idLight::GetSwitch(idAI* user)
{
	if (switchList.Num() == 0) // quick check
	{
		return NULL;
	}

	idEntity* closestSwitch = NULL;
	float shortestDistSqr = idMath::INFINITY;
	idVec3 userOrg = user->GetPhysics()->GetOrigin();
	for (int i = 0 ; i < switchList.Num() ; i++)
	{
		idEntity* e = switchList[i].GetEntity();
		if (e)
		{
			float distSqr = (e->GetPhysics()->GetOrigin() - userOrg).LengthSqr();
			if (distSqr < shortestDistSqr)
			{
				shortestDistSqr = distSqr;
				closestSwitch = e;
			}
		}
	}
	return closestSwitch;
}


// grayman #2603 - Change the flag that says if this light is being relit.

void idLight::SetBeingRelit(bool relighting)
{
	beingRelit = relighting;

	// grayman #3509 - if this light is part of a team that includes a
	// light holder and other lights, each team member that's a light
	// needs to have this flag set the same way.

	// Find topmost parent of light.
	idEntity *bindMaster = GetBindMaster();
	idEntity *parent = NULL;
	while ( bindMaster != NULL )
	{
		parent = bindMaster;
		bindMaster = parent->GetBindMaster();
	}

	// If we found a parent, mark all child lights of that parent as being relit
	if ( parent )
	{
		idList<idEntity *> children;
		parent->GetTeamChildren(&children); // gets all children
		for ( int i = 0 ; i < children.Num() ; i++ )
		{
			idEntity *child = children[i];
			if ( ( child == NULL ) || ( child == this ) ) // NULLs don't count, and we already marked ourselves
			{
				continue;
			}

			if ( child->IsType(idLight::Type) )
			{
				static_cast<idLight*>(child)->beingRelit = relighting;
			}
		}
	}
}

// grayman #2603 - Is an AI in the process or relighting this light?

bool idLight::IsBeingRelit()
{
	return beingRelit;
}

// grayman #2603 - Set the chance that this light can be barked about negatively

void idLight::SetChanceNegativeBark(float newChance)
{
	chanceNegativeBark = newChance;
}

// grayman #2603 - Time when this light was turned off

int idLight::GetWhenTurnedOff()
{
	return whenTurnedOff;
}

 // grayman #2603 - when can this light be relit

int idLight::GetRelightAfter()
{
	return relightAfter;
}

void idLight::SetRelightAfter()
{
	relightAfter = gameLocal.time + 15000;
}

 // grayman #2603 - Set when an AI can next emit a "light's out" or "won't relight" bark

int idLight::GetNextTimeLightOutBark()
{
	return nextTimeLightOutBark;
}

void idLight::SetNextTimeLightOutBark(int newNextTimeLightOutBark)
{
	nextTimeLightOutBark = newNextTimeLightOutBark;
}

// grayman #2603 - can an AI make a negative bark about this light (found off, or won't relight)

bool idLight::NegativeBark(idAI* ai)
{
	idEntityPtr<idEntity> aiPtr = ai;
	int aiBarksIndex = -1; // index of this ai in the aiBarks list
	bool barking = false;

	// Don't allow barks if the Alert Level is above 1.

	if (ai->AI_AlertLevel >= ai->thresh_1)
	{
		return false;
	}

	// Check the number of times this AI has negatively barked about this light.
	// Once they reach a certain number, the barks are disallowed until the
	// light is relit.

	for (int i = 0 ; i < aiBarks.Num() ; i++)
	{
		if (aiBarks[i].ai == aiPtr)
		{
			aiBarksIndex = i; // note for use below
			if (aiBarks[i].count > 1)
			{
				return false;
			}
			break;
		}
	}

	if ((gameLocal.time >= ai->GetMemory().nextTimeLightStimBark) && (gameLocal.time >= nextTimeLightOutBark))
	{
		if (gameLocal.random.RandomFloat() < chanceNegativeBark)
		{
			ai->GetMemory().lastTimeVisualStimBark = gameLocal.time;
			ai->GetMemory().nextTimeLightStimBark = gameLocal.time + REBARK_DELAY;
			nextTimeLightOutBark = gameLocal.time + REBARK_DELAY + 5*gameLocal.random.RandomFloat();

			// As a light receives negative barks ("light out" and "won't relight light"), the odds of emitting
			// this type of bark go down. When the light is relit, the odds are reset to 100%. This should reduce
			// the number of such barks, which can get tiresome.

			chanceNegativeBark -= 0.2f; // reduce next chance of a negative bark
			if (chanceNegativeBark < 0.0f)
			{
				chanceNegativeBark = 0.0f;
			}
			barking = true;

			// Bump the number of times this AI has negatively barked about this light.

			if (aiBarksIndex >= 0)
			{
				aiBarks[aiBarksIndex].count++;
			}
			else // create a new entry for this AI
			{
				AIBarks barks;
				barks.count = 1;
				barks.ai = aiPtr;
				aiBarks.Append(barks);
			}
		}
	}

	return barking;
}

void idLight::Event_Smoking( int state ) // grayman #2603
{
	smoking = (state == 1);	// true: smoking, false: not smoking
}

bool idLight::IsSmoking() // grayman #2603
{
	return smoking;
}

void idLight::Event_SetStartedOff() // grayman #2905 - the light was out at spawn time
{
	// Set startedOff to TRUE if this light and all of any bindmasters have shouldBeOn values == 0.

	startedOff = true; // default assumes shouldBeOn == 0

	int shouldBeOn = spawnArgs.GetInt("shouldBeOn","0");
	if ( shouldBeOn > 0 )
	{
		startedOff = false;
	}
	else // check for bindmasters
	{
		idEntity* bindMaster = GetBindMaster();
		while ( bindMaster != NULL )
		{
			shouldBeOn = bindMaster->spawnArgs.GetInt("shouldBeOn","0");
			if ( shouldBeOn > 0 )
			{
				startedOff = false;
				break;
			}
			bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
		}
	}
}

bool idLight::GetStartedOff() // grayman #2905 - was the light out at spawn time?
{
	return startedOff;
}