/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/ModelManager.h"
#include "framework/DeclEntityDef.h"

#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"

#include "Fx.h"
#include "Light.h"
#include "bc_glasspiece.h"


#include "bc_meta.h"


const float BREAK_FADEOUTTIME = .4f; //BC when light breaks, fade out its targeted lights over this span of time.



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

//BC
const idEventDef EV_Light_IsOn("IsOn", NULL, 'd');
const idEventDef EV_Light_SetSpotlightTarget("setSpotlightTarget", "v", NULL);
const idEventDef EV_Light_GetLightHandle("getLightHandle", NULL, 'f');

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

	//BC
	EVENT(EV_Light_IsOn,			idLight::Event_IsOn)
	EVENT(EV_Light_SetSpotlightTarget, idLight::Event_SetSpotlightTarget)
	EVENT(EV_Light_GetLightHandle,	idLight::Event_GetLightHandle)
END_CLASS


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
	idVec3 lightOffset;

	memset( renderLight, 0, sizeof( *renderLight ) );

	
	lightOffset = args->GetVector("light_offset", "");

	//BC let you do do light offset  relative to current position.
	if (lightOffset != vec3_zero)
	{
		renderLight->origin = args->GetVector("origin") + lightOffset;
	}
	else if (!args->GetVector("light_origin", "", renderLight->origin))
	{
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
	args->GetBool("affectlightmeter", "1", renderLight->affectLightMeter);

	// <blendo> eric tag: ambient args light args
	args->GetBool("ambient", "0", renderLight->isAmbient);
	if (renderLight->isAmbient)
	{
		renderLight->noShadows = true;
		renderLight->noSpecular = true;
	}

	

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );

	// SW: Fog lights never affect the light meter, because they're... not real lights
	if (renderLight->shader && renderLight->shader->IsFogLight())
	{
		renderLight->affectLightMeter = false;		
	}

	args->GetInt("spectrum", "0", renderLight->spectrum); //darkmod

	// SM: Option for lights to not cast a player shadow
	args->GetBool( "cast_player_shadow", "1", renderLight->castPlayerShadow );
}

// SW: Parse settings from our spawnargs that let the light change when a different sky is switched in.
// This is kind of slow and messy so it doesn't happen to every light -- only the ones targeted by the sky controller.
void idLight::ParseSkySettings(const idList<const idMaterial*> skies)
{
	for (int i = 0; i < spawnArgs.GetNumKeyVals(); i++)
	{
		const idKeyValue* kv = spawnArgs.GetKeyVal(i);
		const idStr key = kv->GetKey();

		for (int j = 0; j < skies.Num(); j++)
		{
			// If the prefix of this spawnarg matches our sky name, it's good!
			const idStr name = skies[j]->GetName();
			idStr pathlessName;
			name.ExtractFileName(pathlessName);
			if (key.IcmpPrefix(pathlessName) == 0)
			{
				// Figure out what kind of setting this is -- right now we only have color and light_center
				if (key.Find("_tint") != -1)
				{
					idVec3 skyTint = spawnArgs.GetVector(key);

					if (skyTint == vec3_zero)
						gameLocal.Warning("idLight: Could not parse tint value from spawnarg '%s'", key.c_str());
					else
					{
						skyTint_t newTint;
						newTint.sky = skies[j];
						newTint.color = skyTint;
						skyTints.Append(newTint);
					}
				}
				else if (key.Find("_center") != -1 || key.Find("_centre") != -1) // SW: I'm not letting myself get tripped up by this
				{
					idVec3 skyCenter = spawnArgs.GetVector(key);
					
					if (skyCenter == vec3_zero) 
						// SW: This could theoretically be dodgy (zero is a valid offset -- what if we specifically want that?) but I can't see it ever coming up.
						// We mostly use this for the big parallel light that represents exterior sunlight.
						gameLocal.Warning("idLight: Could not parse vector value from spawnarg '%s'", key.c_str());
					else
					{
						skyCenter_t newCenter;
						newCenter.sky = skies[j];
						newCenter.center = skyCenter;
						skyCenters.Append(newCenter);
					}
				}
				else
					gameLocal.Warning("idLight: Tried to parse sky setting '%s', but it didn't resemble any known format.", key.c_str());
			}
		}
	}
}

// SW: Instruct the light to swap out properties for ones that are relevant to the current sky.
// If we don't have any such properties, we switch ourselves off -- we aren't participating in this sky.
void idLight::ApplySkySetting(const idMaterial* sky)
{
	bool hasSettings = false;
	for (int i = 0; i < skyTints.Num(); i++)
	{
		if (skyTints[i].sky == sky)
		{
			this->SetColor(skyTints[i].color.x, skyTints[i].color.y, skyTints[i].color.z);
			hasSettings = true;
		}
	}
	for (int i = 0; i < skyCenters.Num(); i++)
	{
		if (skyCenters[i].sky == sky)
		{
			this->localLightOrigin = skyCenters[i].center;
			this->renderLight.lightCenter = this->localLightOrigin;
			PresentLightDefChange();
			hasSettings = true;
		}
	}

	if (hasSettings)
	{
		this->On();
	}
		
	else
		this->Off();
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

	fxBreak.Clear();

	// blendo eric: default repairable, but check if disabled when spawning
	//				so the repair list is set on creation, not spawn (savegame doesn't use spawn)
	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);

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

	if (spawnArgs.GetBool("repairable", "1"))
	{
		repairNode.Remove();
	}
}

/*
================
idLight::Save

archives object for save game file
================
*/
void idLight::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderLight( renderLight ); // renderLight_t renderLight
	savefile->WriteInt( lightDefHandle ); // int lightDefHandle

	savefile->WriteBool( renderLight.prelightModel != NULL ); // regen below

	savefile->WriteVec3( localLightOrigin ); // idVec3 localLightOrigin
	savefile->WriteMat3( localLightAxis ); // idMat3 localLightAxis
	savefile->WriteString( brokenModel ); // idString brokenModel
	savefile->WriteInt( levels ); // int levels
	savefile->WriteInt( currentLevel ); // int currentLevel
	savefile->WriteVec3( baseColor ); // idVec3 baseColor
	savefile->WriteBool( breakOnTrigger ); // bool breakOnTrigger
	savefile->WriteInt( count ); // int count
	savefile->WriteInt( triggercount ); // int triggercount
	savefile->WriteObject( lightParent ); // idEntity * lightParent
	savefile->WriteVec4( fadeFrom ); // idVec4 fadeFrom
	savefile->WriteVec4( fadeTo ); // idVec4 fadeTo
	savefile->WriteInt( fadeStart ); // int fadeStart
	savefile->WriteInt( fadeEnd ); // int fadeEnd
	savefile->WriteBool( soundWasPlaying ); // bool soundWasPlaying

	savefile->WriteInt( skyTints.Num() ); // idList<skyTint_t> skyTints
	for (int idx = 0; idx < skyTints.Num(); idx++)
	{
		savefile->WriteMaterial( skyTints[idx].sky ); // const idMaterial* sky
		savefile->WriteVec3( skyTints[idx].color ); // idVec3 color
	}

	savefile->WriteInt( skyCenters.Num() ); // idList<skyCenter_t> skyCenters
	for (int idx = 0; idx < skyCenters.Num(); idx++)
	{
		savefile->WriteMaterial( skyCenters[idx].sky ); // const idMaterial* sky
		savefile->WriteVec3( skyCenters[idx].center ); // idVec3 color
	}

	savefile->WriteDict( &shardDict ); // idDict shardDict
	savefile->WriteString( fxBreak ); // idString fxBreak
	savefile->WriteVec4( originalColor ); // idVec4 originalColor
	savefile->WriteBool( originalAffectLightmeter ); // bool originalAffectLightmeter
}

/*
================
idLight::Restore

unarchives object from save game file
================
*/
void idLight::Restore( idRestoreGame *savefile ) {
	savefile->ReadRenderLight( renderLight ); // renderLight_t renderLight
	savefile->ReadInt( lightDefHandle ); // int lightDefHandle
	if ( lightDefHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	}

	bool hadPrelightModel;
	savefile->ReadBool( hadPrelightModel );
	// renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	// blendo eric: this is loaded by renderlight now
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

	savefile->ReadVec3( localLightOrigin ); // idVec3 localLightOrigin
	savefile->ReadMat3( localLightAxis ); // idMat3 localLightAxis
	savefile->ReadString( brokenModel ); // idString brokenModel
	savefile->ReadInt( levels ); // int levels
	savefile->ReadInt( currentLevel ); // int currentLevel
	savefile->ReadVec3( baseColor ); // idVec3 baseColor
	savefile->ReadBool( breakOnTrigger ); // bool breakOnTrigger
	savefile->ReadInt( count ); // int count
	savefile->ReadInt( triggercount ); // int triggercount
	savefile->ReadObject( lightParent ); // idEntity * lightParent
	savefile->ReadVec4( fadeFrom ); // idVec4 fadeFrom
	savefile->ReadVec4( fadeTo ); // idVec4 fadeTo
	savefile->ReadInt( fadeStart ); // int fadeStart
	savefile->ReadInt( fadeEnd ); // int fadeEnd
	savefile->ReadBool( soundWasPlaying ); // bool soundWasPlaying

	int num;
	savefile->ReadInt( num ); // idList<skyTint_t> skyTints
	skyTints.SetNum( num );
	for (int idx = 0; idx < skyTints.Num(); idx++)
	{
		savefile->ReadMaterial( skyTints[idx].sky ); // const idMaterial* sky
		savefile->ReadVec3( skyTints[idx].color ); // idVec3 color
	}

	savefile->ReadInt( num ); // idList<skyCenter_t> skyCenters
	skyCenters.SetNum( num );
	for (int idx = 0; idx < skyCenters.Num(); idx++)
	{
		savefile->ReadMaterial( skyCenters[idx].sky ); // const idMaterial* sky
		savefile->ReadVec3( skyCenters[idx].center ); // idVec3 color
	}

	savefile->ReadDict( &shardDict ); // idDict shardDict
	savefile->ReadString( fxBreak ); // idString fxBreak
	savefile->ReadVec4( originalColor ); // idVec4 originalColor
	savefile->ReadBool( originalAffectLightmeter ); // bool originalAffectLightmeter

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

	bool hasBrokenmodel; //bc

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &renderLight );

	// we need the origin and axis relative to the physics origin/axis
	localLightOrigin = ( renderLight.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localLightAxis = renderLight.axis * GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );

	this->GetColor(originalColor);

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

#ifdef CTF
	// Midnight CTF
	if ( gameLocal.mpGame.IsGametypeFlagBased() && gameLocal.serverInfo.GetBool("si_midnight") && !spawnArgs.GetBool("midnight_override") ) {
		Off();
	}
#endif

	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetString( "broken", "", brokenModel );
	spawnArgs.GetBool( "break", "0", breakOnTrigger );
	spawnArgs.GetInt( "count", "1", count );
	spawnArgs.GetBool("hasbrokenmodel", "0", hasBrokenmodel); //BC

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
		if (hasBrokenmodel)
		{
			if (!renderModelManager->CheckModel(brokenModel)) {
				if (needBroken) {
					gameLocal.Error("Model '%s' not found for entity %d(%s)", brokenModel.c_str(), entityNumber, name.c_str());
				}
				else {
					brokenModel = "";
				}
			}

			// make sure the collision model gets cached
			idClipModel::CheckModel(brokenModel);
		}
	}

	GetPhysics()->SetContents(spawnArgs.GetBool("solid") ? CONTENTS_SOLID : (CONTENTS_CORPSE | CONTENTS_RENDERMODEL));

	const idDeclEntityDef *shardDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_glasspiece"), false);
	if (shardDef)
	{
		shardDict = shardDef->dict;
	}
	else
	{
		shardDict.Clear();
	}

	fxBreak = spawnArgs.GetString("fx_break");
	
	


	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();

	if (!spawnArgs.GetBool("repairable", "0")) //BC 4-24-2025: now removes repairable node if light is not repairable.
	{
		repairNode.Remove();  // blendo eric: disable instead of enable here, so the list is set on creation, not spawn
	}

	
	originalAffectLightmeter = renderLight.affectLightMeter;
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
void idLight::SetColor( const idVec3 &color ) {
	baseColor = color;
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
	currentLevel = 0;
	// kill any sound it was making
	if ( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) {
		StopSound( SND_CHANNEL_ANY, false );
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
idLight::Killed
================
*/
void idLight::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	//BC called when health is zero
	BecomeBroken( attacker );
}

/*
================
idLight::BecomeBroken
================
*/
void idLight::BecomeBroken( idEntity *activator ) {
	const char *damageDefName;
	const char *breaksound;

	fl.takedamage = false;

	if ( brokenModel.Length() && spawnArgs.GetBool("hasbrokenmodel", "0"))
	{
		//Switch to broken model.
		SetModel( brokenModel );

		if ( !spawnArgs.GetBool( "nonsolid" ) ) {
			GetPhysics()->SetClipModel( new idClipModel( brokenModel.c_str() ), 1.0f );
			GetPhysics()->SetContents( CONTENTS_SOLID );
		}
	}
	else if ( spawnArgs.GetBool( "hideModelOnBreak" ) )
	{
		SetModel( "" );
		GetPhysics()->SetContents( 0 );
	}

	if ( gameLocal.isServer )
	{

		ServerSendEvent( EVENT_BECOMEBROKEN, NULL, true, -1 );

		if ( spawnArgs.GetString( "def_damage", "", &damageDefName ) ) {
			idVec3 origin = renderEntity.origin + renderEntity.bounds.GetCenter() * renderEntity.axis;
			gameLocal.RadiusDamage( origin, activator, activator, this, this, damageDefName );
		}

	}

	//ActivateTargets( activator );

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

	//BC
	parm = spawnArgs.GetString("skin_broken");
	if (parm && *parm) {
		SetSkin(declManager->FindSkin(parm));
	}

	
	parm = spawnArgs.GetString("gui_onbreak");
	if (parm && *parm)
	{
		Event_GuiNamedEvent(1, parm);
	}



	if (spawnArgs.GetString("snd_break", "", &breaksound))
	{
		StartSound("snd_break", SND_CHANNEL_ANY, 0, false, NULL);
	}
	

	if (spawnArgs.GetBool("kill_light"))
	{
		Fade(vec4_zero, BREAK_FADEOUTTIME);
	}

	if (shardDict.GetFloat("fuse") > 0)
	{
		int i;

        idVec3 breakFlyDest = vec3_zero;
        bool lightAirless = gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin());
        if (lightAirless)
        {
            if (targets.Num() > 0)
            {
                breakFlyDest = targets[0].GetEntity()->GetPhysics()->GetOrigin();
            }
        }

		idBounds lightBounds = GetPhysics()->GetAbsBounds();
		lightBounds.Expand(-1);


		#define GLASSPIECES_COUNT 16
		for (i = 0; i < GLASSPIECES_COUNT; i++)
		{
			idEntity *glassPiece;
			gameLocal.SpawnEntityDef(shardDict, &glassPiece, false);
			if (glassPiece && glassPiece->IsType(idGlassPiece::Type))
			{
				idGlassPiece *debris = static_cast<idGlassPiece *>(glassPiece);
				
				//Find a suitable position to spawn the glasspiece. Spawn within light model's bounds.
				idVec3 spawnPos;
				spawnPos.x = lightBounds[0].x + gameLocal.random.RandomInt(lightBounds[1].x - lightBounds[0].x);
				spawnPos.y = lightBounds[0].y + gameLocal.random.RandomInt(lightBounds[1].y - lightBounds[0].y);
				spawnPos.z = lightBounds[0].z + gameLocal.random.RandomInt(lightBounds[1].z - lightBounds[0].z);

				//gameRenderWorld->DebugArrowSimple(spawnPos);				

				debris->Create(spawnPos, mat3_identity);
				debris->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 4, gameLocal.random.CRandomFloat() * 4, gameLocal.random.CRandomFloat() * 4));

                if (lightAirless && breakFlyDest != vec3_zero)
                {
                    debris->GetPhysics()->SetGravity(vec3_zero);

                    //no gravity. make the glass fly toward the targeted entity.
                    idVec3 adjustedFlyDest = breakFlyDest + idVec3(-8 + (gameLocal.random.CRandomFloat() * 16), -8 + (gameLocal.random.CRandomFloat() * 16), -8 + (gameLocal.random.CRandomFloat() * 16));
                    idVec3 flyDir = (adjustedFlyDest - debris->GetPhysics()->GetOrigin());
                    flyDir.NormalizeFast();
                    debris->GetPhysics()->SetLinearVelocity(flyDir * 32);
                }
                

				//debris->Create(NULL, shards[i]->physicsObj.GetOrigin(), shards[i]->physicsObj.GetAxis(0));
				//debris->Launch();
			}
		}
	}

	if (fxBreak.Length())
	{
		idVec3 fxPos = GetPhysics()->GetOrigin() + idVec3(0,0,-16);
		idMat3 fxAngle = mat3_identity;
		idEntityFx::StartFx(fxBreak, &fxPos, &fxAngle, this, true);		
	}

	//BC turn off any lights it is targeting.
	ToggleTargetedLights(false);


	UpdateVisuals();

	//Spawn an interestpoint.
	gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_brokenlight");



	//repair system.
	if (spawnArgs.GetBool("repairable", "1"))
	{
		this->health = 0;
		needsRepair = true;
		repairrequestTimestamp = gameLocal.time;
	}
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

		assert(renderEntity.referenceSound != (void*)0x00000001);
	}
	else {

		assert(refSound.referenceSound != (void*)0x00000001);
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
				// SM: If we've faded to black, also set to current level of 0
				if (color.Compare(colorBlack))
					currentLevel = 0;
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
			assert(renderLight.referenceSound != (void*)0x00000001);

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
	msg.WriteInt( PackColor( baseColor ) );
	// msg.WriteBits( lightParent.GetEntityNum(), GENTITYNUM_BITS );

/*	// only helps prediction
	msg.WriteInt( PackColor( fadeFrom ) );
	msg.WriteInt( PackColor( fadeTo ) );
	msg.WriteInt( fadeStart );
	msg.WriteInt( fadeEnd );
*/

	// FIXME: send renderLight.shader
	msg.WriteFloat( renderLight.lightRadius[0], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[1], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[2], 5, 10 );

	msg.WriteInt( PackColor( idVec4( renderLight.shaderParms[SHADERPARM_RED],
									  renderLight.shaderParms[SHADERPARM_GREEN],
									  renderLight.shaderParms[SHADERPARM_BLUE],
									  renderLight.shaderParms[SHADERPARM_ALPHA] ) ) );

	msg.WriteFloat( renderLight.shaderParms[SHADERPARM_TIMESCALE], 5, 10 );
	msg.WriteInt( renderLight.shaderParms[SHADERPARM_TIMEOFFSET] );
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
	UnpackColor( msg.ReadInt(), baseColor );
	// lightParentEntityNum = msg.ReadBits( GENTITYNUM_BITS );

/*	// only helps prediction
	UnpackColor( msg.ReadInt(), fadeFrom );
	UnpackColor( msg.ReadInt(), fadeTo );
	fadeStart = msg.ReadInt();
	fadeEnd = msg.ReadInt();
*/

	// FIXME: read renderLight.shader
	renderLight.lightRadius[0] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[1] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[2] = msg.ReadFloat( 5, 10 );

	UnpackColor( msg.ReadInt(), shaderColor );
	renderLight.shaderParms[SHADERPARM_RED] = shaderColor[0];
	renderLight.shaderParms[SHADERPARM_GREEN] = shaderColor[1];
	renderLight.shaderParms[SHADERPARM_BLUE] = shaderColor[2];
	renderLight.shaderParms[SHADERPARM_ALPHA] = shaderColor[3];

	renderLight.shaderParms[SHADERPARM_TIMESCALE] = msg.ReadFloat( 5, 10 );
	renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadInt();
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
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

void idLight::SetLightTarget(idVec3 newTarget) //BC for trash cuber
{
	//common->Printf("%f %f %f\n", renderLight.target.x, renderLight.target.y, renderLight.target.z);

	renderLight.target = newTarget;
	renderLight.end = newTarget;
}

bool idLight::GetNoShadow()
{
	return renderLight.noShadows;
}

void idLight::SetNoShadow(bool value)
{
	renderLight.noShadows = value;
	UpdateVisuals();
}

void idLight::ToggleTargetedLights(bool value)
{
	idEntity	*ent;

	for (int i = 0; i < targets.Num(); i++)
	{
		ent = targets[i].GetEntity();
		if (!ent) { continue; }

		if (ent->RespondsTo(EV_Activate) || ent->HasSignal(SIG_TRIGGER))
		{
			//check if it's a light.
			if (ent->IsType(idLight::Type))
			{
				if (!value)
					static_cast<idLight *>(ent)->FadeOut(BREAK_FADEOUTTIME); //turn OFF light.
				else
					static_cast<idLight *>(ent)->FadeIn(BREAK_FADEOUTTIME); //turn ON light.
			}
		}
	}
}


bool idLight::Event_IsOn(void)
{
	if (currentLevel > 0)
	{
		idThread::ReturnInt(1);
		return true;
	}

	idThread::ReturnInt(0);
	return false;
}

bool idLight::Event_IsAmbient(void)
{
	if (renderLight.isAmbient)
	{
		idThread::ReturnInt(1);
		return true;
	}

	idThread::ReturnInt(0);
	return false;
}

void idLight::Event_SetAmbient(bool value)
{
	renderLight.isAmbient = value;	
	//renderLight.noShadows = true; //... this isn't great. it will always turns shadows off. 
	//renderLight.noSpecular = true;

	UpdateVisuals();
}

void idLight::GetBaseColor(idVec3 &out)
{
	out[0] = baseColor.x;
	out[1] = baseColor.y;
	out[2] = baseColor.z;
}

idVec3 idLight::GetRadius()
{
	return renderLight.lightRadius;
}


//BC
void idLight::DoRepairTick(int amount)
{
	const char * skinName;

	fl.takedamage = true;
	health = maxHealth;
	needsRepair = false;

	ToggleTargetedLights(true);

	StopSound(SND_CHANNEL_ANY, false); //Stop the broken buzz sound.

	skinName = spawnArgs.GetString("skin", "");
	if (skinName[0])
	{
		SetSkin(declManager->FindSkin(skinName));
	}
	else
	{
		SetSkin(NULL);
	}

	SetColor(spawnArgs.GetVector("_color", "1 1 1"));

	StartSound("snd_repaired", SND_CHANNEL_ANY, 0, false, NULL);

	const char *parm = spawnArgs.GetString("gui_onrepaired");
	if (parm && *parm)
	{
		Event_GuiNamedEvent(1, parm);
	}

	UpdateVisuals();

}

//Return the original color the light had at game start.
idVec4 idLight::GetOriginalColor()
{
	return originalColor;
}

bool idLight::GetOriginalLightmeter()
{
	return originalAffectLightmeter;
}

bool idLight::IsFog()
{
	return (this->renderLight.shader && this->renderLight.shader->IsFogLight());
}

void idLight::SetAffectLightmeter(bool value)
{
	renderLight.affectLightMeter = value;
}

void idLight::Event_SetSpotlightTarget(const idVec3 &vec)
{
	SetLightTarget(vec);

	BecomeActive(TH_UPDATEVISUALS);
}

void idLight::Event_GetLightHandle(void)
{
	idThread::ReturnFloat(lightDefHandle);
}
