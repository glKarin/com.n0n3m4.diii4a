#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

// RAVEN BEGIN
// bdube: client effects
#include "client/ClientEffect.h"
//mcg: need to know team for AddDamageEffects
#include "ai/AI_Manager.h"
// RAVEN END

/*
===============================================================================

	idEntity

===============================================================================
*/

// overridable events
const idEventDef EV_PostSpawn( "<postspawn>", NULL );
const idEventDef EV_FindTargets( "<findTargets>", NULL );
const idEventDef EV_Touch( "<touch>", "et" );
const idEventDef EV_GetName( "getName", NULL, 's' );
const idEventDef EV_SetName( "setName", "s" );
const idEventDef EV_Activate( "activate", "e" );
const idEventDef EV_ActivateTargets( "activateTargets", "e" );
const idEventDef EV_NumTargets( "numTargets", NULL, 'f' );
const idEventDef EV_GetTarget( "getTarget", "f", 'e' );
const idEventDef EV_RandomTarget( "randomTarget", "s", 'e' );
const idEventDef EV_Bind( "bind", "e" );
const idEventDef EV_BindPosition( "bindPosition", "e" );
const idEventDef EV_BindToJoint( "bindToJoint", "esf" );
const idEventDef EV_Unbind( "unbind", NULL );
const idEventDef EV_RemoveBinds( "removeBinds" );
const idEventDef EV_SpawnBind( "<spawnbind>", NULL );
const idEventDef EV_SetOwner( "setOwner", "e" );
const idEventDef EV_SetModel( "setModel", "s" );
const idEventDef EV_SetSkin( "setSkin", "s" );
const idEventDef EV_GetWorldOrigin( "getWorldOrigin", NULL, 'v' );
const idEventDef EV_SetWorldOrigin( "setWorldOrigin", "v" );
const idEventDef EV_GetOrigin( "getOrigin", NULL, 'v' );
const idEventDef EV_SetOrigin( "setOrigin", "v" );
const idEventDef EV_GetAngles( "getAngles", NULL, 'v' );
const idEventDef EV_SetAngles( "setAngles", "v" );
const idEventDef EV_GetLinearVelocity( "getLinearVelocity", NULL, 'v' );
const idEventDef EV_SetLinearVelocity( "setLinearVelocity", "v" );
const idEventDef EV_GetAngularVelocity( "getAngularVelocity", NULL, 'v' );
const idEventDef EV_SetAngularVelocity( "setAngularVelocity", "v" );
const idEventDef EV_GetSize( "getSize", NULL, 'v' );
const idEventDef EV_SetSize( "setSize", "vv" );
const idEventDef EV_GetMins( "getMins", NULL, 'v' );
const idEventDef EV_GetMaxs( "getMaxs", NULL, 'v' );
const idEventDef EV_IsHidden( "isHidden", NULL, 'd' );
const idEventDef EV_Hide( "hide", NULL );
const idEventDef EV_Show( "show", NULL );
const idEventDef EV_Touches( "touches", "E", 'd' );
const idEventDef EV_ClearSignal( "clearSignal", "d" );
const idEventDef EV_GetShaderParm( "getShaderParm", "d", 'f' );
const idEventDef EV_SetShaderParm( "setShaderParm", "df" );
const idEventDef EV_SetShaderParms( "setShaderParms", "ffff" );
const idEventDef EV_SetColor( "setColor", "fff" );
const idEventDef EV_GetColor( "getColor", NULL, 'v' );
const idEventDef EV_CacheSoundShader( "cacheSoundShader", "s" );
const idEventDef EV_StartSoundShader( "startSoundShader", "sd", 'f' );
const idEventDef EV_StartSound( "startSound", "sdd", 'f' );
const idEventDef EV_StopSound( "stopSound", "dd" );
const idEventDef EV_FadeSound( "fadeSound", "dff" );
const idEventDef EV_SetGuiParm( "setGuiParm", "ss" );
const idEventDef EV_SetGuiFloat( "setGuiFloat", "sf" );
const idEventDef EV_GetNextKey( "getNextKey", "ss", 's' );
const idEventDef EV_SetKey( "setKey", "ss" );
const idEventDef EV_GetKey( "getKey", "s", 's' );
const idEventDef EV_GetIntKey( "getIntKey", "s", 'f' );
const idEventDef EV_GetFloatKey( "getFloatKey", "s", 'f' );
const idEventDef EV_GetVectorKey( "getVectorKey", "s", 'v' );
const idEventDef EV_GetEntityKey( "getEntityKey", "s", 'e' );
const idEventDef EV_RestorePosition( "restorePosition" );
const idEventDef EV_UpdateCameraTarget( "<updateCameraTarget>", NULL );
const idEventDef EV_DistanceTo( "distanceTo", "E", 'f' );
const idEventDef EV_DistanceToPoint( "distanceToPoint", "v", 'f' );
const idEventDef EV_StartFx( "startFx", "s" );
const idEventDef EV_HasFunction( "hasFunction", "s", 'd' );
const idEventDef EV_CallFunction( "callFunction", "s" );
const idEventDef EV_SetNeverDormant( "setNeverDormant", "d" );

// RAVEN BEGIN
// bgeisler: go back to default skin
const idEventDef EV_ClearSkin( "clearSkin");
// kfuller: added events
const idEventDef EV_SetContents( "setContents", "d" );
const idEventDef EV_GetLastBlocker( "getLastBlocker", NULL, 'e' );
const idEventDef EV_Earthquake( "earthquake", "f" );
// we should probably try to integrate this with AI_PlayAnim
const idEventDef EV_PlayAnim("playAnimNoChannel", "s");
const idEventDef EV_PlayAnimXTimes("playAnimXTimes", "sf");
// bdube: effect events
const idEventDef EV_PlayEffect( "playEffect", "ssd" );
const idEventDef EV_StopEffect( "stopEffect", "s" );
const idEventDef EV_StopAllEffects( "stopAllEffects" );
const idEventDef EV_GetHealth ( "getHealth", NULL, 'f' );
// bdube: surface related events
const idEventDef EV_HideSurface( "hideSurface", "s" );
const idEventDef EV_ShowSurface( "showSurface", "s" );
// bdube: added gui events
const idEventDef EV_GuiEvent ( "guiEvent", "s" );
// jscott: for playback button handling
const idEventDef EV_PlaybackCallback( "playbackCallback", "ddd" );
// nmckenzie:
const idEventDef EV_GetBindMaster( "getBindMaster", NULL, 'e' );
const idEventDef EV_ApplyImpulse( "applyImpulse", "evv" );
// abahr:
const idEventDef EV_RemoveNullTargets( "removeNullTargets" );
const idEventDef EV_IsA( "isA", "s", 'f' );
const idEventDef EV_IsSameTypeAs( "isSameTypeAs", "e", 'f' );
const idEventDef EV_MatchPrefix( "matchPrefix", "ss", 's' );
const idEventDef EV_ClearTargetList( "clearTargetList", "f" );
// twhitaker:
const idEventDef EV_AppendTarget( "appendTarget", "E", 'f' );
const idEventDef EV_RemoveTarget( "removeTarget", "e" );
// mekberg:
const idEventDef EV_SetHealth( "setHealth", "f" );
// RAVEN END

ABSTRACT_DECLARATION( idClass, idEntity )
	EVENT( EV_GetName,				idEntity::Event_GetName )
	EVENT( EV_SetName,				idEntity::Event_SetName )
	EVENT( EV_FindTargets,			idEntity::Event_FindTargets )
	EVENT( EV_ActivateTargets,		idEntity::Event_ActivateTargets )
	EVENT( EV_NumTargets,			idEntity::Event_NumTargets )
	EVENT( EV_GetTarget,			idEntity::Event_GetTarget )
	EVENT( EV_RandomTarget,			idEntity::Event_RandomTarget )
	EVENT( EV_BindToJoint,			idEntity::Event_BindToJoint )
	EVENT( EV_RemoveBinds,			idEntity::Event_RemoveBinds )
	EVENT( EV_Bind,					idEntity::Event_Bind )
	EVENT( EV_BindPosition,			idEntity::Event_BindPosition )
	EVENT( EV_Unbind,				idEntity::Event_Unbind )
	EVENT( EV_SpawnBind,			idEntity::Event_SpawnBind )
	EVENT( EV_SetOwner,				idEntity::Event_SetOwner )
	EVENT( EV_SetModel,				idEntity::Event_SetModel )
	EVENT( EV_SetSkin,				idEntity::Event_SetSkin )
	EVENT( EV_GetShaderParm,		idEntity::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,		idEntity::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,		idEntity::Event_SetShaderParms )
	EVENT( EV_SetColor,				idEntity::Event_SetColor )
	EVENT( EV_GetColor,				idEntity::Event_GetColor )
	EVENT( EV_IsHidden,				idEntity::Event_IsHidden )
	EVENT( EV_Hide,					idEntity::Event_Hide )
	EVENT( EV_Show,					idEntity::Event_Show )
	EVENT( EV_CacheSoundShader,		idEntity::Event_CacheSoundShader )
	EVENT( EV_StartSoundShader,		idEntity::Event_StartSoundShader )
	EVENT( EV_StartSound,			idEntity::Event_StartSound )
	EVENT( EV_StopSound,			idEntity::Event_StopSound )
	EVENT( EV_FadeSound,			idEntity::Event_FadeSound )
	EVENT( EV_GetWorldOrigin,		idEntity::Event_GetWorldOrigin )
	EVENT( EV_SetWorldOrigin,		idEntity::Event_SetWorldOrigin )
	EVENT( EV_GetOrigin,			idEntity::Event_GetOrigin )
	EVENT( EV_SetOrigin,			idEntity::Event_SetOrigin )
	EVENT( EV_GetAngles,			idEntity::Event_GetAngles )
	EVENT( EV_SetAngles,			idEntity::Event_SetAngles )
	EVENT( EV_GetLinearVelocity,	idEntity::Event_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,	idEntity::Event_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,	idEntity::Event_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,	idEntity::Event_SetAngularVelocity )
	EVENT( EV_GetSize,				idEntity::Event_GetSize )
	EVENT( EV_SetSize,				idEntity::Event_SetSize )
	EVENT( EV_GetMins,				idEntity::Event_GetMins)
	EVENT( EV_GetMaxs,				idEntity::Event_GetMaxs )
	EVENT( EV_Touches,				idEntity::Event_Touches )
	EVENT( EV_SetGuiParm, 			idEntity::Event_SetGuiParm )
	EVENT( EV_SetGuiFloat, 			idEntity::Event_SetGuiFloat )
	EVENT( EV_GetNextKey,			idEntity::Event_GetNextKey )
	EVENT( EV_SetKey,				idEntity::Event_SetKey )
	EVENT( EV_GetKey,				idEntity::Event_GetKey )
	EVENT( EV_GetIntKey,			idEntity::Event_GetIntKey )
	EVENT( EV_GetFloatKey,			idEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,			idEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,			idEntity::Event_GetEntityKey )
	EVENT( EV_RestorePosition,		idEntity::Event_RestorePosition )
	EVENT( EV_UpdateCameraTarget,	idEntity::Event_UpdateCameraTarget )
	EVENT( EV_DistanceTo,			idEntity::Event_DistanceTo )
	EVENT( EV_DistanceToPoint,		idEntity::Event_DistanceToPoint )
	EVENT( EV_StartFx,				idEntity::Event_StartFx )
	EVENT( EV_Thread_WaitFrame,		idEntity::Event_WaitFrame )
	EVENT( EV_Thread_Wait,			idEntity::Event_Wait )
	EVENT( EV_HasFunction,			idEntity::Event_HasFunction )
	EVENT( EV_CallFunction,			idEntity::Event_CallFunction )
	EVENT( EV_SetNeverDormant,		idEntity::Event_SetNeverDormant )

// RAVEN BEGIN
// bgeisler: go back to default skin
	EVENT( EV_ClearSkin,			idEntity::Event_ClearSkin )
// kfuller: added events
	EVENT( EV_SetContents,			idEntity::Event_SetContents )
	EVENT( EV_GetLastBlocker,		idEntity::Event_GetLastBlocker)
// bdube: effect events
	EVENT( EV_PlayEffect,			idEntity::Event_PlayEffect )
	EVENT( EV_StopEffect,			idEntity::Event_StopEffect )
	EVENT( EV_StopAllEffects,		idEntity::Event_StopAllEffects )
	EVENT( EV_GetHealth,			idEntity::Event_GetHealth )
// bdube: mesh events
	EVENT( EV_HideSurface,			idEntity::Event_HideSurface )
	EVENT( EV_ShowSurface,			idEntity::Event_ShowSurface )
// bdube: gui events
	EVENT( EV_GuiEvent,				idEntity::Event_GuiEvent )
// jscott: playback callback
	EVENT( EV_PlaybackCallback,		idEntity::Event_PlaybackCallback )
// nmckenzie: Check who we're bound to.
	EVENT( EV_GetBindMaster,		idEntity::Event_GetBindMaster )
	EVENT( EV_ApplyImpulse,			idEntity::Event_ApplyImpulse )
// abahr: so we can call this from script
	EVENT( EV_RemoveNullTargets,	idEntity::Event_RemoveNullTargets )
	EVENT( EV_IsA,					idEntity::Event_IsA )
	EVENT( EV_IsSameTypeAs,			idEntity::Event_IsSameTypeAs )
	EVENT( EV_MatchPrefix,			idEntity::Event_MatchPrefix )
	EVENT( EV_ClearTargetList,		idEntity::Event_ClearTargetList )
// twhitaker: to dynamically add/remove targets in script
	EVENT( EV_AppendTarget,			idEntity::Event_AppendTarget )
	EVENT( EV_RemoveTarget,			idEntity::Event_RemoveTarget )
// mekberg: added
	EVENT( EV_SetHealth,			idEntity::Event_SetHealth )
// RAVEN END
END_CLASS

/*
================
UpdateGuiParms
================
*/
void UpdateGuiParms( idUserInterface *gui, const idDict *args ) {
	if ( gui == NULL || args == NULL ) {
		return;
	}
	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	while( kv ) {
		gui->SetStateString( kv->GetKey(), common->GetLocalizedString( kv->GetValue() ) );
		kv = args->MatchPrefix( "gui_parm", kv );
	}
	gui->SetStateBool( "noninteractive",  args->GetBool( "gui_noninteractive" ) ) ;
	gui->StateChanged( gameLocal.time );
}

/*
================
AddRenderGui
================
*/
void AddRenderGui( const char *name, idUserInterface **gui, const idDict *args ) {

	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	*gui = uiManager->FindGui( name, true, ( kv != NULL ) || args->GetBool( "gui_noninteractive" ) );
	UpdateGuiParms( *gui, args );
}

/*
================
idGameEdit::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity ) {
	int			i;
	const char	*temp;
	idVec3		color;
	float		angle;
	const idDeclModelDef *modelDef;

	memset( renderEntity, 0, sizeof( *renderEntity ) );

	temp = args->GetString( "model" );

	modelDef = NULL;
	if ( temp[0] != '\0' ) {
		if ( !strstr( temp, "." ) ) {
			modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			if ( modelDef ) {
				renderEntity->hModel = modelDef->ModelHandle();
				if ( renderEntity->hModel && !renderEntity->hModel->IsLoaded() ) {
					renderEntity->hModel->LoadModel();
				}
			}
		}

		if ( !renderEntity->hModel ) {
			renderEntity->hModel = renderModelManager->FindModel( temp );
		}
	}
	if ( renderEntity->hModel ) {
		renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
	} else {
		renderEntity->bounds.Zero();
	}

	temp = args->GetString( "skin" );
	if ( temp[0] != '\0' ) {
		renderEntity->customSkin = declManager->FindSkin( temp );
	} else if ( modelDef ) {
		renderEntity->customSkin = modelDef->GetDefaultSkin();
	}

	temp = args->GetString( "shader" );
	if ( temp[0] != '\0' ) {
		renderEntity->customShader = declManager->FindMaterial( temp );
	}		

	args->GetVector( "origin", "0 0 0", renderEntity->origin );

	// get the rotation matrix in either full form, or single angle form
	if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", renderEntity->axis ) ) {
		angle = args->GetFloat( "angle" );
// RAVEN BEGIN
// abahr: allowing up and down buttons to affect orientation
		if( angle == -1.0f ) {
			renderEntity->axis = idAngles( -90.0f, 0.0f, 0.0f ).ToMat3();
		} else if( angle == -2.0f ) {
			renderEntity->axis = idAngles( 90.0f, 0.0f, 0.0f ).ToMat3();
		} else
// RAVEN END
		if ( angle != 0.0f ) {
			renderEntity->axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
		} else {
			renderEntity->axis.Identity();
		}
	}

// RAVEN BEGIN
	renderEntity->referenceSoundHandle = -1;
// RAVEN END

	// get shader parms
	args->GetVector( "_color", "1 1 1", color );
	renderEntity->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	renderEntity->shaderParms[ 3 ]					= args->GetFloat( "shaderParm3", "1" );
	renderEntity->shaderParms[ 4 ]					= args->GetFloat( "shaderParm4", "0" );
	renderEntity->shaderParms[ 5 ]					= args->GetFloat( "shaderParm5", "0" );
	renderEntity->shaderParms[ 6 ]					= args->GetFloat( "shaderParm6", "0" );
	renderEntity->shaderParms[ 7 ]					= args->GetFloat( "shaderParm7", "0" );
	renderEntity->shaderParms[ 8 ]					= args->GetFloat( "shaderParm8", "0" );
	renderEntity->shaderParms[ 9 ]					= args->GetFloat( "shaderParm9", "0" );
	renderEntity->shaderParms[ 10 ]					= args->GetFloat( "shaderParm10", "0" );
	renderEntity->shaderParms[ 11 ]					= args->GetFloat( "shaderParm11", "0" );

	// check noDynamicInteractions flag
	renderEntity->noDynamicInteractions = args->GetBool( "noDynamicInteractions" );

	// check noshadows flag
	renderEntity->noShadow = args->GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity->noSelfShadow = args->GetBool( "noselfshadows" );

	// init any guis, including entity-specific states
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		temp = args->GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
		if ( temp[ 0 ] != '\0' ) {
			AddRenderGui( temp, &renderEntity->gui[ i ], args );
		}
	}
}

/*
================
idGameEdit::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound ) {
	const char	*temp;

	memset( refSound, 0, sizeof( *refSound ) );
	refSound->referenceSoundHandle = -1;

// RAVEN BEGIN
	refSound->parms.minDistance = args->GetFloat( "s_mindistance" );
#ifdef _QUAKE4
	refSound->parms.minDistance /= 100.0f; //k: scale to doom 3 distance
#endif
	refSound->parms.maxDistance = args->GetFloat( "s_maxdistance" );
#ifdef _QUAKE4
	refSound->parms.maxDistance /= 100.0f; //k: scale to doom 3 distance
#endif
	// WARNING: This overrides the volume; it does not modify it
	if( args->GetFloat( "s_volume" ) != 0.0f ) {
		refSound->parms.volume = idMath::dBToScale( args->GetFloat( "s_volume" ) );
	}

	if( refSound->parms.volume < 0.0f || refSound->parms.volume > 5.0f ) {
		common->Warning( "Unreasonable volume (%g) on entity \'%s\'", refSound->parms.volume, args->GetString( "name" ) );
		refSound->parms.volume = 5.0f;
	}
// RAVEN END
	refSound->parms.shakes = args->GetFloat( "s_shakes" );

	args->GetVector( "origin", "0 0 0", refSound->origin );

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is usefull to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound->diversity = args->GetFloat( "s_diversity", "-1" );
	refSound->waitfortrigger = args->GetBool( "s_waitfortrigger" );

	if ( args->GetBool( "s_omni" ) ) {
		refSound->parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args->GetBool( "s_looping" ) ) {
		refSound->parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( args->GetBool( "s_occlusion" ) ) {
		refSound->parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args->GetBool( "s_global" ) ) {
		refSound->parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args->GetBool( "s_unclamped" ) ) {
		refSound->parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	if ( args->GetBool( "s_center" ) ) {
		refSound->parms.soundShaderFlags |= SSF_CENTER;
	}

	refSound->parms.soundClass = args->GetInt( "s_soundClass" );

	temp = args->GetString( "s_shader" );
	if ( temp[0] != '\0' ) {
		refSound->shader = declManager->FindSound( temp );
	}

// RAVEN BEGIN
	if( refSound->parms.maxDistance < refSound->parms.minDistance ) {
		common->Warning( "ParseSpawnArgsToRefSound: Max distance less than min distance for entity \'%s\'", args->GetString( "name", "*unknown*" ) );
	}
// RAVEN END
}

/*
===============
idEntity::UpdateChangeableSpawnArgs

Any key val pair that might change during the course of the game ( via a gui or whatever )
should be initialize here so a gui or other trigger can change something and have it updated
properly. An optional source may be provided if the values reside in an outside dictionary and
first need copied over to spawnArgs
===============
*/
void idEntity::UpdateChangeableSpawnArgs( const idDict *source ) {
	int i;
	const char *target;

	if ( !source ) {
		source = &spawnArgs;
	}
	cameraTarget = NULL;
	target = source->GetString( "cameraTarget" );
	if ( target && target[0] ) {
// RAVEN BEGIN
// bdube: EV_UpdateCameraTarget pulls from spawnargs so we need to move the target over
		spawnArgs.Set ( "cameraTarget", target );
// RAVEN END		
		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], source );
	}
}

/*
================
idEntity::idEntity
================
*/
idEntity::idEntity() {

	entityNumber	= ENTITYNUM_NONE;
	entityDefNumber = -1;

	spawnNode.SetOwner( this );
	activeNode.SetOwner( this );

	snapshotNode.SetOwner( this );
	snapshotSequence = -1;
	snapshotBits = 0;

	thinkFlags		= 0;
	dormantStart	= 0;
	cinematic		= false;
	renderView		= NULL;
	cameraTarget	= NULL;
	health			= 0;

	physics			= NULL;
	bindMaster		= NULL;
	bindJoint		= INVALID_JOINT;
	bindBody		= -1;
	teamMaster		= NULL;
	teamChain		= NULL;
	signals			= NULL;

	memset( PVSAreas, 0, sizeof( PVSAreas ) );
	numPVSAreas		= -1;

	memset( &fl, 0, sizeof( fl ) );
	fl.neverDormant	= true;			// most entities never go dormant

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	modelDefHandle	= -1;
	memset( &refSound, 0, sizeof( refSound ) );
	refSound.referenceSoundHandle = -1;

	mpGUIState = -1;

// RAVEN BEGIN
// rjohnson: added this to persist long thinking entities
	mLastLongThinkTime = 0;
	mLastLongThinkColor.Zero();
// ddynerman: instance, clipworld
	SetInstance( 0 );
	SetClipWorld( 0 );
	fl.persistAcrossInstances = false;
// twhitaker
	forwardDamageEnt = NULL;
// ddynerman: optional preprediction
	predictTime = 0;
// RAVEN END
}

/*
================
idEntity::Spawn
================
*/
void idEntity::Spawn( void ) {
	int					i;
	const char			*temp;
	idVec3				origin;
	idMat3				axis;
	const idKeyValue	*networkSync;
	const char			*classname;
	const char			*scriptObjectName;

	gameLocal.RegisterEntity( this );

// bdube: make sure there is a classname before trying to use it
	if ( spawnArgs.GetString( "classname", NULL, &classname ) ) {
		const idDeclEntityDef *def = gameLocal.FindEntityDef( classname, false );
		if ( def ) {
			entityDefNumber = def->Index();
		}
	}

	// Persona is a set of keys that augment an entity giving it its own custom persona
	const idDict* dict;
	dict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_persona", "" ), false );
	if ( dict ) {
		spawnArgs.Copy ( *dict );
	}
// RAVEN END

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );

// RAVEN BEGIN
// bdube: added hidesurface
	const idKeyValue* kv;
	for ( kv = spawnArgs.MatchPrefix ( "hidesurface", NULL );
		  kv;
		  kv = spawnArgs.MatchPrefix ( "hidesurface", kv ) ) {
		HideSurface ( kv->GetValue() );
	}
// RAVEN END		

	renderEntity.entityNum = entityNumber;

// RAVEN BEGIN
// ddynerman: LOD code
	renderEntity.shadowLODDistance = spawnArgs.GetFloat( "shadow_lod_distance", "768.0" );
	renderEntity.shadowLODDistance *= renderEntity.shadowLODDistance;
// ddynerman: multiple clip worlds
	int spawnInstance = spawnArgs.GetInt( "instance" );
	SetInstance( spawnInstance );
// RAVEN END

	// go dormant within 5 frames so that when the map starts most monsters are dormant
	dormantStart = gameLocal.time - DELAY_DORMANT_TIME + gameLocal.msec * 5;

	origin = renderEntity.origin;
	axis = renderEntity.axis;

	// do the audio parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );

	// only play SCHANNEL_PRIVATE when sndworld->PlaceListener() is called with this listenerId
	// don't spatialize sounds from the same entity
	refSound.listenerId = entityNumber + 1;

	cameraTarget = NULL;
	temp = spawnArgs.GetString( "cameraTarget" );
	if ( temp && temp[0] ) {
		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], &spawnArgs );
	}

	fl.solidForTeam = spawnArgs.GetBool( "solidForTeam", "0" );
// RAVEN BEGIN
// bdube: usable
	fl.usable = spawnArgs.GetBool ( "usable", "0" );
// RAVEN END

	fl.neverDormant = spawnArgs.GetBool( "neverDormant", "0" );
	fl.hidden = spawnArgs.GetBool( "hide", "0" );
	if ( fl.hidden ) {
		// make sure we're hidden, since a spawn function might not set it up right
		PostEventMS( &EV_Hide, 0 );
	}
	cinematic = spawnArgs.GetBool( "cinematic", "0" );

	networkSync = spawnArgs.FindKey( "networkSync" );
	if ( networkSync ) {
		fl.networkSync = ( atoi( networkSync->GetValue() ) != 0 );
	}

	// every object will have a unique name
	temp = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber ) );
	SetName( temp );

	// if we have targets, wait until all entities are spawned to get them
	if ( spawnArgs.MatchPrefix( "target" ) || spawnArgs.MatchPrefix( "guiTarget" ) ) {
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindTargets();
		}
	}

	health = spawnArgs.GetInt( "health" );

	InitDefaultPhysics( origin, axis );

	SetOrigin( origin );
	SetAxis( axis );

	temp = spawnArgs.GetString( "model" );
	if ( temp && *temp ) {
		SetModel( temp );
	}

	if ( spawnArgs.GetString( "bind", "", &temp ) ) {
		PostEventMS( &EV_SpawnBind, 0 );
	}

	// auto-start a sound on the entity
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	// setup script object
	if ( ShouldConstructScriptObjectAtSpawn() && spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}

// RAVEN BEGIN
	fl.persistAcrossInstances = false;
// bgeisler: added
	fl.triggerAnim = spawnArgs.GetBool( "trigger_anim" );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_crush", false, false );
// RAVEN END
}

/*
================
idEntity::~idEntity
================
*/
idEntity::~idEntity( void ) {
	DeconstructScriptObject();
	scriptObject.Free();

	if ( thinkFlags ) {
		BecomeInactive( thinkFlags );
	}
	activeNode.Remove();

	Signal( SIG_REMOVED );

	// we have to set back the default physics object before unbinding because the entity
	// specific physics object might be an entity variable and as such could already be destroyed.
	SetPhysics( NULL );

	// remove any entities that are bound to me
	RemoveBinds();

	// unbind from master
	Unbind();
	QuitTeam();

	gameLocal.RemoveEntityFromHash( name.c_str(), this );

	delete renderView;
	renderView = NULL;

	delete signals;
	signals = NULL;

// RAVEN BEGIN
// bdube: make sure all sounds and attached effects are stopped
	StopSound( SCHANNEL_ANY, false );
	
	RemoveClientEntities();
// RAVEN END

	FreeModelDef();
	FreeSoundEmitter( false );

	gameLocal.UnregisterEntity( this );
}

/*
================
idEntity::Save
================
*/
void idEntity::Save( idSaveGame *savefile ) const {
	int				i, j;
	rvClientEntity* cent;

	savefile->WriteInt( entityNumber );
	savefile->WriteInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	// idLinkList<idEntity>	snapshotNode;

	savefile->WriteInt( snapshotSequence );
	savefile->WriteInt( snapshotBits );

	savefile->WriteString( name );
	savefile->WriteDict( &spawnArgs );
	scriptObject.Save( savefile );

	savefile->WriteInt( thinkFlags );
	savefile->WriteInt( dormantStart );
	savefile->WriteBool( cinematic );

	// renderView_t * renderView;

	savefile->WriteObject( cameraTarget );

	savefile->WriteInt( targets.Num() );
	for( i = 0; i < targets.Num(); i++ ) {
		targets[ i ].Save( savefile );
	}

	savefile->WriteInt( health );

	savefile->WriteInt( clientEntities.Num() );
	for( cent = clientEntities.Next(); cent; cent = cent->bindNode.Next() ) {
		savefile->WriteObject( cent );
	}

//	savefile->WriteInt( mLastLongThinkTime );			// Debug vars - don't save
//	savefile->WriteVec4( mLastLongThinkColor );			// Debug vars - don't save

	savefile->Write( &fl, sizeof( fl ) );

	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( modelDefHandle );
	savefile->WriteRefSound( refSound );

// RAVEN BEGIN
// mekberg: proper save
	forwardDamageEnt.Save ( savefile );
// RAVEN END

	savefile->WriteStaticObject( defaultPhysicsObj );

	savefile->WriteObject( bindMaster.GetEntity() );
	savefile->WriteJoint( bindJoint );
	savefile->WriteInt( bindBody );
	savefile->WriteObject( teamMaster );
	savefile->WriteObject( teamChain );

	savefile->WriteInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->WriteInt( PVSAreas[ i ] );
	}

	if ( !signals ) {
		savefile->WriteBool( false );
	} else {
		savefile->WriteBool( true );
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->WriteInt( signals->signal[ i ].Num() );
			for( j = 0; j < signals->signal[ i ].Num(); j++ ) {
				savefile->WriteInt( signals->signal[ i ][ j ].threadnum );
				savefile->WriteString( signals->signal[ i ][ j ].function->Name() );
			}
		}
	}

	savefile->WriteInt( mpGUIState );

	savefile->WriteInt( instance );
	savefile->WriteInt( clipWorld );
}

/*
================
idEntity::Restore
================
*/
void idEntity::Restore( idRestoreGame *savefile ) {
	int				i, j;
	int				num;
	rvClientEntity	*temp;
	idStr			funcname;

	savefile->ReadInt( entityNumber );
	savefile->ReadInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	// idLinkList<idEntity>	snapshotNode; 

	savefile->ReadInt( snapshotSequence );
	savefile->ReadInt( snapshotBits );

	savefile->ReadString( name );
	SetName( name );
	savefile->ReadDict( &spawnArgs );

	scriptObject.Restore( savefile );

	savefile->ReadInt( thinkFlags );
	savefile->ReadInt( dormantStart );
	savefile->ReadBool( cinematic );

	// renderView_t *			renderView;

	savefile->ReadObject( reinterpret_cast<idClass *&>( cameraTarget ) );

	targets.Clear();
	savefile->ReadInt( num );
	targets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		targets[ i ].Restore( savefile );
	}

	savefile->ReadInt( health );

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( temp ) );
		if( temp ) {
			temp->bindNode.AddToEnd( clientEntities );
		}
	}

//	savefile->ReadInt( mLastLongThinkTime );			// Debug vars - don't save
//	savefile->ReadVec4( mLastLongThinkColor );			// Debug vars - don't save

	savefile->Read( &fl, sizeof( fl ) );

// RAVEN BEGIN
	savefile->ReadRenderEntity( renderEntity, &spawnArgs );
// RAVEN END
	savefile->ReadInt( modelDefHandle );
	savefile->ReadRefSound( refSound );
	
// RAVEN BEGIN
// mekberg: proper restore
	forwardDamageEnt.Restore ( savefile );
// RAVEN END

	savefile->ReadStaticObject( defaultPhysicsObj );
	RestorePhysics( &defaultPhysicsObj );

	idEntity *templol = 0;
	savefile->ReadObject( reinterpret_cast<idClass *&>( templol ) );
	bindMaster = templol;

	savefile->ReadJoint( bindJoint );
	savefile->ReadInt( bindBody );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamMaster ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamChain ) );

	savefile->ReadInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->ReadInt( PVSAreas[ i ] );
	}

	bool readsignals;
	savefile->ReadBool( readsignals );
	if ( readsignals ) {
		signals = new signalList_t;
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->ReadInt( num );
			signals->signal[ i ].SetNum( num );
			for( j = 0; j < num; j++ ) {
				savefile->ReadInt( signals->signal[ i ][ j ].threadnum );
				savefile->ReadString( funcname );
				signals->signal[ i ][ j ].function = gameLocal.program.FindFunction( funcname );
				if ( !signals->signal[ i ][ j ].function ) {
					savefile->Error( "Function '%s' not found", funcname.c_str() );
				}
			}
		}
	}

	savefile->ReadInt( mpGUIState );

	// restore must retrieve modelDefHandle from the renderer
	if ( modelDefHandle != -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}

	savefile->ReadInt( instance );
	savefile->ReadInt( clipWorld );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_crush", false, false );
}

/*
================
idEntity::GetEntityDefName
================
*/
const char * idEntity::GetEntityDefName( void ) const {
	if ( entityDefNumber < 0 ) {
		return "*unknown*";
	}
	return declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
}

/*
================
idEntity::SetName
================
*/
void idEntity::SetName( const char *newname ) {
	if ( name.Length() ) {
		gameLocal.RemoveEntityFromHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, NULL );
	}

	name = newname;
	if ( name.Length() ) {
		if ( ( name == "NULL" ) || ( name == "null_entity" ) ) {
			gameLocal.Error( "Cannot name entity '%s'.  '%s' is reserved for script.", name.c_str(), name.c_str() );
		}
		gameLocal.AddEntityToHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, this );
	}
}

/*
================
idEntity::GetName
================
*/
const char * idEntity::GetName( void ) const {
	return name.c_str();
}


/***********************************************************************

	Thinking
	
***********************************************************************/

/*
================
idEntity::Think
================
*/
void idEntity::Think( void ) {
	RunPhysics();
	Present();
}

/*
================
idEntity::DoDormantTests

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::DoDormantTests( void ) {
	// Never go dormant?
	if ( fl.neverDormant || (gameLocal.inCinematic && cinematic) ) {
		return false;
	}

	// if the monster area is not topologically connected to a player
	if ( !gameLocal.InPlayerConnectedArea( this ) ) {
		return true;
	} else {
		// the monster area is topologically connected to a player, but if
		// the monster hasn't been woken up before, do the more precise PVS check
		if ( !fl.hasAwakened ) {
			if ( !gameLocal.InPlayerPVS( this ) ) {
				return true;
			}
		}
	}
	
	return false;
}

/*
================
idEntity::CheckDormant

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::CheckDormant( void ) {
	bool dormant;
	
	dormant = DoDormantTests();
	if ( dormant ) {
		if ( dormantStart == 0 ) {
			dormantStart = gameLocal.time;
		}
		if ( gameLocal.time - dormantStart < DELAY_DORMANT_TIME ) {
			dormant = false;
		}
	} else {
		dormantStart = 0;
		fl.hasAwakened = true;
	}		
	
	if ( dormant && !fl.isDormant ) {
		fl.isDormant = true;
		DormantBegin();
	} else if ( !dormant && fl.isDormant ) {
		fl.isDormant = false;
		DormantEnd();
	}

	return dormant;
}

/*
================
idEntity::DormantBegin

called when entity becomes dormant
================
*/
void idEntity::DormantBegin( void ) {
}

/*
================
idEntity::DormantEnd

called when entity wakes from being dormant
================
*/
void idEntity::DormantEnd( void ) {
}

/*
================
idEntity::IsActive
================
*/
bool idEntity::IsActive( void ) const {
	return activeNode.InList();
}

/*
================
idEntity::BecomeActive
================
*/
void idEntity::BecomeActive( int flags ) {
	if ( ( flags & TH_PHYSICS ) ) {
		// enable the team master if this entity is part of a physics team
		if ( teamMaster && teamMaster != this ) {
			teamMaster->BecomeActive( TH_PHYSICS );
		} else if ( !( thinkFlags & TH_PHYSICS ) ) {
			// if this is a pusher
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( physics->IsType( idPhysics_Parametric::GetClassType() ) || physics->IsType( idPhysics_Actor::GetClassType() ) ) {
// RAVEN END
				gameLocal.sortPushers = true;
			}
// RAVEN BEGIN
// abahr:
// jnewquist: Use accessor for static class type 
			if( physics->IsType( rvPhysics_Spline::GetClassType() ) ) {
				gameLocal.sortPushers = true;
			}
// RAVEN END
		}
	}

	int oldFlags = thinkFlags;
	thinkFlags |= flags;
	if ( thinkFlags ) {
		if ( !IsActive() ) {
			activeNode.AddToEnd( gameLocal.activeEntities );
		} else if ( !oldFlags ) {
			// we became inactive this frame, so we have to decrease the count of entities to deactivate
			gameLocal.numEntitiesToDeactivate--;
		}
	}
}

/*
================
idEntity::BecomeInactive
================
*/
void idEntity::BecomeInactive( int flags ) {
	if ( ( flags & TH_PHYSICS ) ) {
		// may only disable physics on a team master if no team members are running physics or bound to a joints
		if ( teamMaster == this ) {
			for ( idEntity *ent = teamMaster->teamChain; ent; ent = ent->teamChain ) {
				if ( ( ent->thinkFlags & TH_PHYSICS ) || ( ( ent->bindMaster == this ) && ( ent->bindJoint != INVALID_JOINT ) ) ) {
					flags &= ~TH_PHYSICS;
					break;
				}
			}
		}
	}

	if ( thinkFlags ) {
		thinkFlags &= ~flags;
		if ( !thinkFlags && IsActive() ) {
			gameLocal.numEntitiesToDeactivate++;
		}
	}

	if ( ( flags & TH_PHYSICS ) ) {
		// if this entity has a team master
		if ( teamMaster && teamMaster != this ) {
			// if the team master is at rest
			if ( teamMaster->IsAtRest() ) {
				teamMaster->BecomeInactive( TH_PHYSICS );
			}
		}
	}
}

/***********************************************************************

	Visuals
	
***********************************************************************/

/*
================
idEntity::SetShaderParm
================
*/
void idEntity::SetShaderParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderEntity.shaderParms[ parmnum ] = value;
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( float red, float green, float blue ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= red;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= green;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= blue;
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec3 &color ) {
	SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec3 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec4 &color ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec4 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idEntity::UpdateAnimationControllers
================
*/
bool idEntity::UpdateAnimationControllers( void ) {
	// any ragdoll and IK animation controllers should be updated here
	return false;
}

/*
================
idEntity::SetModel
================
*/
void idEntity::SetModel( const char *modelname ) {
	assert( modelname );

	FreeModelDef();

	renderEntity.hModel = renderModelManager->FindModel( modelname );

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = NULL;
	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	UpdateVisuals();
}

/*
================
idEntity::SetSkin
================
*/
void idEntity::SetSkin( const idDeclSkin *skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();
}
// RAVEN BEGIN
// bgeisler: go back to default skin
/*
================
idEntity::ClearSkin
================
*/
void idEntity::ClearSkin( void )  
{
	if ( GetAnimator() && GetAnimator()->ModelDef() ) {
		renderEntity.customSkin = GetAnimator()->ModelDef()->GetDefaultSkin();		
	} else  {
		renderEntity.customSkin = NULL;
	}

	UpdateVisuals();
}
// RAVEN END

/*
================
idEntity::GetSkin
================
*/
const idDeclSkin *idEntity::GetSkin( void ) const {
	return renderEntity.customSkin;
}

/*
================
idEntity::FreeModelDef
================
*/
void idEntity::FreeModelDef( void ) {
	if ( modelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( modelDefHandle );
		modelDefHandle = -1;

		rvClientEntity* cent;

		for( cent = clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
			cent->FreeEntityDef();
		}	
	}	
}

/*
================
idEntity::FreeLightDef
================
*/
void idEntity::FreeLightDef( void ) {
}

/*
================
idEntity::IsHidden
================
*/
bool idEntity::IsHidden( void ) const {
	return fl.hidden;
}

/*
================
idEntity::Hide
================
*/
void idEntity::Hide( void ) {
	if ( !IsHidden() ) {
		fl.hidden = true;
		FreeModelDef();
		UpdateVisuals();
	}
}

/*
================
idEntity::Show
================
*/
void idEntity::Show( void ) {
	if ( IsHidden() ) {
		fl.hidden = false;
		UpdateVisuals();
	}
}

/*
================
idEntity::UpdateModelTransform
================
*/
void idEntity::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
================
idEntity::UpdateModel
================
*/
void idEntity::UpdateModel( void ) {
	UpdateModelTransform();

// RAVEN BEGIN
// abahr: moved GetAnimator call because its invalid when called from a destructor
	UpdateRenderEntityCallback();
// RAVEN END

	// set to invalid number to force an update the next time the PVS areas are retrieved
	ClearPVSAreas();

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );
}

// RAVEN BEGIN
// abahr:
/*
================
idEntity::UpdateRenderEntityCallback
================
*/
void idEntity::UpdateRenderEntityCallback() {
}
// RAVEN END

/*
================
idEntity::UpdateVisuals
================
*/
void idEntity::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound();
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( void ) {
	int localNumPVSAreas, localPVSAreas[32];
	idBounds modelAbsBounds;
	int i;

	modelAbsBounds.FromTransformedBounds( renderEntity.bounds, renderEntity.origin, renderEntity.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	// FIXME: some particle systems may have huge bounds and end up in many PVS areas
	// the first MAX_PVS_AREAS may not be visible to a network client and as a result the particle system may not show up when it should
	if ( localNumPVSAreas > MAX_PVS_AREAS ) {
		localNumPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( renderEntity.origin ).Expand( 64.0f ), localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );
	}

	for ( numPVSAreas = 0; numPVSAreas < MAX_PVS_AREAS && numPVSAreas < localNumPVSAreas; numPVSAreas++ ) {
		PVSAreas[numPVSAreas] = localPVSAreas[numPVSAreas];
	}

	for( i = numPVSAreas; i < MAX_PVS_AREAS; i++ ) {
		PVSAreas[ i ] = 0;
	}
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( const idVec3 &pos ) {
	int i;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( pos ), PVSAreas, MAX_PVS_AREAS );
	i = numPVSAreas;
	while ( i < MAX_PVS_AREAS ) {
		PVSAreas[ i++ ] = 0;
	}
}

/*
================
idEntity::GetNumPVSAreas
================
*/
int idEntity::GetNumPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return numPVSAreas;
}

/*
================
idEntity::GetPVSAreas
================
*/
const int *idEntity::GetPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return PVSAreas;
}

/*
================
idEntity::ClearPVSAreas
================
*/
void idEntity::ClearPVSAreas( void ) {
	numPVSAreas = -1;
}

/*
================
idEntity::PhysicsTeamInPVS

  FIXME: for networking also return true if any of the entity shadows is in the PVS
================
*/
bool idEntity::PhysicsTeamInPVS( pvsHandle_t pvsHandle ) {
	idEntity *part;

	if ( teamMaster ) {
		for ( part = teamMaster; part; part = part->teamChain ) {
			if ( gameLocal.pvs.InCurrentPVS( pvsHandle, part->GetPVSAreas(), part->GetNumPVSAreas() ) ) {
				return true;
			}
		}
	} else {
		return gameLocal.pvs.InCurrentPVS( pvsHandle, GetPVSAreas(), GetNumPVSAreas() );
	}
	return false;
}

/*
==============
idEntity::ProjectOverlay
==============
*/
void idEntity::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	float s, c;
	idMat3 axis, axistemp;
	idVec3 localOrigin, localAxis[2];
	idPlane localPlane[2];

	// make sure the entity has a valid model handle
	if ( modelDefHandle < 0 ) {
		return;
	}

	// only do this on dynamic md5 models
	if ( renderEntity.hModel->IsDynamicModel() != DM_CACHED ) {
		return;
	}

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	axis[2] = -dir;
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	renderEntity.axis.ProjectVector( origin - renderEntity.origin, localOrigin );
	renderEntity.axis.ProjectVector( axis[0], localAxis[0] );
	renderEntity.axis.ProjectVector( axis[1], localAxis[1] );

	size = 1.0f / size;
	localAxis[0] *= size;
	localAxis[1] *= size;

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -( localOrigin * localAxis[0] ) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -( localOrigin * localAxis[1] ) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( material );

	// project an overlay onto the model
	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	// make sure non-animating models update their overlay
	UpdateVisuals();
}

/*
================
idEntity::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idEntity::Present( void ) {

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// if there is no handle yet, go ahead and add it, ignoring the last predict frame early out
	// if not, that causes next render frame to have a bunch of spurious primitive draws ( r_showPrimitives )
	// ( we suspect this is because TH_UPDATEVISUALS doesn't get cleared? )
#if 0 //k jmarshall
	if ( !gameLocal.isLastPredictFrame && modelDefHandle != -1 ) {
		return;
	}
#endif

// RAVEN BEGIN
// ddynerman: don't render objects not in our instance (only on server)
	if ( gameLocal.isServer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != GetInstance() ) {
		FreeModelDef();
		return;
	}
// RAVEN END

	// don't render server demo stuff that's not in our instance
	if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() ) {
		if ( instance != 0 ) {
			FreeModelDef();
			return;
		}
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
// RAVEN BEGIN
// rjohnson: removed PVS check for when func_static's are not starting in your PVS
	if ( cameraTarget ) { // && gameLocal.InPlayerPVS( this ) ) {
// RAVEN END
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
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
idEntity::UpdateRenderEntity
================
*/
bool idEntity::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	idAnimator *animator = GetAnimator();
	if ( animator ) {
		return animator->CreateFrame( gameLocal.time, false );
	}

	return false;
}

/*
================
idEntity::ModelCallback

	NOTE: may not change the game state whatsoever!
================
*/
bool idEntity::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	idEntity *ent;

	ent = gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent ) {
		gameLocal.Error( "idEntity::ModelCallback: callback with NULL game entity '%d'", renderEntity->entityNum );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idEntity::GetAnimator

Subclasses will be responsible for allocating animator.
================
*/
idAnimator *idEntity::GetAnimator( void ) {
	return NULL;
}

/*
=============
idEntity::GetRenderView

This is used by remote camera views to look from an entity
=============
*/
renderView_t *idEntity::GetRenderView( void ) {
	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	renderView->vieworg = GetPhysics()->GetOrigin();
	renderView->fov_x = 120;
	renderView->fov_y = 120;
	renderView->viewaxis = GetPhysics()->GetAxis();

	// copy global shader parms
	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	renderView->globalMaterial = gameLocal.GetGlobalMaterial();

	renderView->time = gameLocal.time;

	return renderView;
}

// RAVEN BEGIN
// bdube: added convienince functions for effects

/***********************************************************************

  effects
	
***********************************************************************/

/*
================
idEntity::PlayEffect
================
*/
rvClientEffect* idEntity::PlayEffect( const idDecl *effect, jointHandle_t joint, const idVec3& originOffset, const idMat3& axisOffset, bool loop, const idVec3& endOrigin, bool broadcast, effectCategory_t category, const idVec4& effectTint ) {
	if ( joint == INVALID_JOINT ) {
		return NULL;
	}
	
	if ( !effect || !gameLocal.isNewFrame ) {
		return NULL;
	}

	if ( !gameLocal.isClient && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, effect );
		msg.WriteLong( joint );
		msg.WriteBits( loop, 1 );
		msg.WriteFloat( endOrigin.x );
		msg.WriteFloat( endOrigin.y );
		msg.WriteFloat( endOrigin.z );	
		msg.WriteByte( category );
		ServerSendInstanceEvent( EVENT_PLAYEFFECT_JOINT, &msg, false, -1 );
	}

// RAVEN BEGIN
// rjohnson: no effects on dedicated server
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		// no effects on dedicated server
		return NULL;
	}

	if( bse->Filtered( effect->GetName(), category ) ) {
		// Effect filtered out
		return NULL;
	}

	if ( gameLocal.isListenServer && gameLocal.GetLocalPlayer() ) {
		if ( GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			return NULL;
		}
	}
// RAVEN END

	RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
	rvClientEffect* clientEffect = new rvClientEffect( effect );
	RV_POP_HEAP();

	if( !clientEffect ) {
		common->Warning( "Failed to create effect \'%s\'\n", effect->GetName() );
		return NULL;
	}

	if( clientEffect->entityNumber == -1 ) {
		common->Warning( "Failed to spawn effect \'%s\'\n", effect->GetName() );
		delete clientEffect;
		return NULL;
	}

	clientEffect->SetOrigin( originOffset );
	clientEffect->SetAxis( axisOffset );
	clientEffect->Bind( this, joint );
	clientEffect->SetGravity( gameLocal.GetCurrentGravity( this ) );

	if ( !clientEffect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete clientEffect;
		return NULL;
	}
	
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_RED ]		= effectTint[ 0 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_GREEN ]	= effectTint[ 1 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_BLUE ]		= effectTint[ 2 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_ALPHA ]	= effectTint[ 3 ];

	return clientEffect;
}

rvClientEffect* idEntity::PlayEffect( const idDecl *effect, const idVec3& origin, const idMat3& axis, bool loop, const idVec3& endOrigin, bool broadcast, effectCategory_t category, const idVec4& effectTint ) {
	idVec3 localOrigin;
	idMat3 localAxis;
	
	if ( !effect || !gameLocal.isNewFrame ) {
		return NULL;
	}

	if ( entityNumber == ENTITYNUM_WORLD ) {
		return gameLocal.PlayEffect( effect, origin, axis, loop, endOrigin, broadcast, false, category, effectTint );
	}

	// Calculate the local origin and axis from the given globals
	localOrigin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
	localAxis   = axis * renderEntity.axis.Transpose();

	if ( !gameLocal.isClient && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];
		idCQuat		quat;
		
		quat = localAxis.ToCQuat();

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, effect );
		msg.WriteFloat( localOrigin.x );
		msg.WriteFloat( localOrigin.y );
		msg.WriteFloat( localOrigin.z );
		msg.WriteFloat( quat.x );
		msg.WriteFloat( quat.y );
		msg.WriteFloat( quat.z );
		msg.WriteBits( loop, 1 );
		msg.WriteFloat( endOrigin.x );
		msg.WriteFloat( endOrigin.y );
		msg.WriteFloat( endOrigin.z );	
		msg.WriteByte( category );
		ServerSendInstanceEvent( EVENT_PLAYEFFECT, &msg, false, -1 );
	}

// RAVEN BEGIN
// rjohnson: no effects on dedicated server
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		// no effects on dedicated server
		return NULL;
	}

	if( bse->Filtered( effect->GetName(), category ) ) {
		// Effect filtered out
		return( NULL );
	}
// ddynerman: a listen server might get this far re: playing effects, don't actually play out of instance effects
	if ( gameLocal.isListenServer && gameLocal.GetLocalPlayer() ) {
		if ( GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			return NULL;
		}
	}
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
	rvClientEffect* clientEffect = new rvClientEffect( effect );
	RV_POP_HEAP();
// RAVEN END

	if( !clientEffect ) {
		common->Warning( "Failed to create effect \'%s\'\n", effect->GetName() );
		return NULL;
	}

	if( clientEffect->entityNumber == -1 ) {
		common->Warning( "Failed to spawn effect \'%s\'\n", effect->GetName() );
		delete clientEffect;
		return NULL;
	}

	clientEffect->SetOrigin( localOrigin );
	clientEffect->SetAxis( localAxis );
	clientEffect->Bind( this );
	clientEffect->SetGravity( gameLocal.GetCurrentGravity( this ) );

	if ( !clientEffect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete clientEffect;
		return NULL;
	}
	
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_RED ]		= effectTint[ 0 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_GREEN ]	= effectTint[ 1 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_BLUE ]		= effectTint[ 2 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_ALPHA ]	= effectTint[ 3 ];

	return clientEffect;
}

/*
================
idEntity::StopAllEffects
================
*/
void idEntity::StopAllEffects( bool destroyParticles ) {
	rvClientEntity* cent;
	rvClientEntity* next;

	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();
		if ( cent->IsType ( rvClientEffect::GetClassType() ) ) {
			static_cast<rvClientEffect *>( cent )->Stop( destroyParticles );
		}
	}		
}

/*
================
idEntity::StopEffect
================
*/
void idEntity::StopEffect( const idDecl *effect, bool destroyParticles ) {	
	rvClientEntity*	cent;
	rvClientEntity*	next;
	
	if( !effect ) {
		return;
	}

	// Build a list of all the effects to stop	
	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();
		
		// Is this client entity an effect?
		if ( !cent->IsType( rvClientEffect::GetClassType() ) ) {
			continue;
		}
		
		// Now check to make sure its the specific effect we want to stop		
		rvClientEffect* clientEffect;
		clientEffect = static_cast<rvClientEffect *>( cent );
		if ( clientEffect->GetEffectIndex() == effect->Index() ) {
			clientEffect->Stop( destroyParticles );
		}
	}
}

void idEntity::StopEffect( const char* effectName, bool destroyParticles ) {
	StopEffect( gameLocal.GetEffect( spawnArgs, effectName ), destroyParticles );
}

// RAVEN END

/***********************************************************************

  Sound
	
***********************************************************************/

/*
================
idEntity::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool idEntity::CanPlayChatterSounds( void ) const {
	return true;
}

/*
================
idEntity::StartSound
================
*/
bool idEntity::StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	const idSoundShader *shader;
	const char *sound;

	if ( length ) {
		*length = 0;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	idStr soundNameStr = soundName;
	if( soundNameStr.CmpPrefix( "snd_" ) && soundNameStr.CmpPrefix( "lipsync_" ) ) {
		common->Warning( "Non precached sound \'%s\'", soundName );
	}

	if ( !spawnArgs.GetString( soundName, "", &sound ) ) {
		return false;
	}

	if ( *sound == '\0' ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		// don't play the sound, but don't report an error
		return true;
	}

	shader = declManager->FindSound( sound );
	return StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );
}

/*
================
idEntity::StartSoundShader
================
*/
bool idEntity::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	float diversity;
	int len;

	if ( length ) {
		*length = 0;
	}

	if ( !shader ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		return true;
	}

	if ( gameLocal.isServer && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, shader );
		msg.WriteByte( channel );
		ServerSendInstanceEvent( EVENT_STARTSOUNDSHADER, &msg, false, -1 );
	}

	// in MP, don't play sounds from other instances
	if( gameLocal.isMultiplayer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) {
		return false;
	}

	// rjohnson: don't play sounds on a dedicated server!
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		return false;
	}

	// set a random value for diversity unless one was parsed from the entity
	if ( refSound.diversity < 0.0f ) {
		diversity = gameLocal.random.RandomFloat();
	} else {
		diversity = refSound.diversity;
	}

// RAVEN BEGIN
	// if we don't have a soundEmitter allocated yet, get one now
	if ( !soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle ) ) {
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );
	}

	UpdateSound();

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
        len = emitter->StartSound( shader, channel, diversity, soundShaderFlags );
		if ( length ) {
			*length = len;
		}
	}
// RAVEN END

	// set reference to the sound for shader synced effects
	renderEntity.referenceSoundHandle = refSound.referenceSoundHandle;

	return true;
}

/*
================
idEntity::StopSound
================
*/
void idEntity::StopSound( const s_channelType channel, bool broadcast ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( gameLocal.isServer && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( channel );
		ServerSendInstanceEvent( EVENT_STOPSOUNDSHADER, &msg, false, -1 );
	}

	// in MP, don't play sounds from other instances
	if ( gameLocal.isMultiplayer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) {
		return;
	}

// RAVEN BEGIN
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->StopSound( channel );
	}
// RAVEN END
}

/*
================
idEntity::SetSoundVolume

  Must be called before starting a new sound.
================
*/
void idEntity::SetSoundVolume( float volume ) {
	refSound.parms.volume = volume;
}

/*
================
idEntity::UpdateSound
================
*/
void idEntity::UpdateSound( void ) {
// RAVEN BEGIN
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
// RAVEN END
		idVec3 origin;
		idMat3 axis;

		if ( GetPhysicsToSoundTransform( origin, axis ) ) {
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		} else {
			refSound.origin = GetPhysics()->GetOrigin();
		}

// RAVEN BEGIN
		refSound.velocity = GetPhysics()->GetLinearVelocity();
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
// RAVEN END
	}
}

/*
================
idEntity::GetListenerId
================
*/
int idEntity::GetListenerId( void ) const {
	return refSound.listenerId;
}

/*
================
idEntity::GetSoundEmitter
================
*/
// RAVEN BEGIN
int idEntity::GetSoundEmitter( void ) const {
	return( refSound.referenceSoundHandle );
// RAVEN END
}

/*
================
idEntity::FreeSoundEmitter
================
*/
void idEntity::FreeSoundEmitter( bool immediate ) {
// RAVEN BEGIN
	soundSystem->FreeSoundEmitter( SOUNDWORLD_GAME, refSound.referenceSoundHandle, immediate );
	refSound.referenceSoundHandle = -1;
// RAVEN END
}

// RAVEN BEGIN
// bdube: client entities

/***********************************************************************

  client entities
	
***********************************************************************/

/*
================
idEntity::RemoveClientEntities
================
*/
void idEntity::RemoveClientEntities( void ) {
	rvClientEntity* cent;
	// Unbinding should remove the node from the list so keep using the head until
	// there are no more entities
	for( cent = clientEntities.Next(); cent != NULL; cent = clientEntities.Next() ) {
		cent->Unbind( );
		delete cent;
	}		
	clientEntities.Clear( );
}
// RAVEN END

/***********************************************************************

  entity binding
	
***********************************************************************/

/*
================
idEntity::PreBind
================
*/
void idEntity::PreBind( void ) {
}

/*
================
idEntity::PostBind
================
*/
void idEntity::PostBind( void ) {
}

/*
================
idEntity::PreUnbind
================
*/
void idEntity::PreUnbind( void ) {
}

/*
================
idEntity::PostUnbind
================
*/
void idEntity::PostUnbind( void ) {
}

/*
================
idEntity::InitBind
================
*/
bool idEntity::InitBind( idEntity *master ) {

	if ( master == this ) {
		gameLocal.Error( "Tried to bind an object to itself." );
		return false;
	}

	if ( this == gameLocal.world ) {
		gameLocal.Error( "Tried to bind world to another entity" );
		return false;
	}

	// unbind myself from my master
	Unbind();

	// add any bind constraints to an articulated figure
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( master && IsType( idAFEntity_Base::GetClassType() ) ) {
// RAVEN END
		static_cast<idAFEntity_Base *>(this)->AddBindConstraints();
	}

	if ( !master || master == gameLocal.world ) {
		// this can happen in scripts, so safely exit out.
		return false;
	}

	return true;
}

/*
================
idEntity::FinishBind
================
*/
void idEntity::FinishBind( void ) {

	// set the master on the physics object
	physics->SetMaster( bindMaster, fl.bindOrientated );

	// We are now separated from our previous team and are either
	// an individual, or have a team of our own.  Now we can join
	// the new bindMaster's team.  Bindmaster must be set before
	// joining the team, or we will be placed in the wrong position
	// on the team.
	JoinTeam( bindMaster );

	// if our bindMaster is enabled during a cinematic, we must be, too
// RAVEN BEGIN
// rjohnson: players should always have cinematic turned on, no matter what
	if ( !IsType ( idPlayer::GetClassType() ) ) {
		cinematic = bindMaster->cinematic;
	}
// RAVEN END

	// make sure the team master is active so that physics get run
	teamMaster->BecomeActive( TH_PHYSICS );
}

/*
================
idEntity::Bind

  bind relative to the visual position of the master
================
*/
void idEntity::Bind( idEntity *master, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind( );
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, const char *jointname, bool orientated ) {
	jointHandle_t	jointnum;
	idAnimator		*masterAnimator;

	if ( !InitBind( master ) ) {
		return;
	}

	masterAnimator = master->GetAnimator();
	if ( !masterAnimator ) {
		gameLocal.Warning( "idEntity::BindToJoint: entity '%s' cannot support skeletal models.", master->GetName() );
		return;
	}

	jointnum = masterAnimator->GetJointHandle( jointname );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::BindToJoint: joint '%s' not found on entity '%s'.", jointname, master->GetName() );
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToBody

  bind relative to a collision model used by the physics of the master
================
*/
void idEntity::BindToBody( idEntity *master, int bodyId, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	if ( bodyId < 0 ) {
		gameLocal.Warning( "idEntity::BindToBody: body '%d' not found.", bodyId );
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = bodyId;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::Unbind
================
*/
void idEntity::Unbind( void ) {
	idEntity *	prev;
	idEntity *	next;
	idEntity *	last;
	idEntity *	ent;

	// remove any bind constraints from an articulated figure
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( IsType( idAFEntity_Base::GetClassType() ) ) {
// RAVEN END
		static_cast<idAFEntity_Base *>(this)->RemoveBindConstraints();
	}

	if ( !bindMaster ) {
		return;
	}

	if ( !teamMaster ) {
		// Teammaster already has been freed
		bindMaster = NULL;
		return;
	}

	PreUnbind();

	if ( physics ) {
		physics->SetMaster( NULL, fl.bindOrientated );
	}

	// We're still part of a team, so that means I have to extricate myself
	// and any entities that are bound to me from the old team.
	// Find the node previous to me in the team
	prev = teamMaster;
	for( ent = teamMaster->teamChain; ent && ( ent != this ); ent = ent->teamChain ) {
		prev = ent;
	}

	assert( ent == this ); // If ent is not pointing to this, then something is very wrong.

	// Find the last node in my team that is bound to me.
	// Also find the first node not bound to me, if one exists.
	last = this;
	for( next = teamChain; next != NULL; next = next->teamChain ) {
		if ( !next->IsBoundTo( this ) ) {
			break;
		}

		// Tell them I'm now the teamMaster
		next->teamMaster = this;
		last = next;
	}

	// disconnect the last member of our team from the old team
	last->teamChain = NULL;

	// connect up the previous member of the old team to the node that
	// follow the last node bound to me (if one exists).
	if ( teamMaster != this ) {
		prev->teamChain = next;
		if ( !next && ( teamMaster == prev ) ) {
			prev->teamMaster = NULL;
		}
	} else if ( next ) {
		// If we were the teamMaster, then the nodes that were not bound to me are now
		// a disconnected chain.  Make them into their own team.
		for( ent = next; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamMaster = next;
		}
		next->teamMaster = next;
	}

	// If we don't have anyone on our team, then clear the team variables.
	if ( teamChain ) {
		// make myself my own team
		teamMaster = this;
	} else {
		// no longer a team
		teamMaster = NULL;
	}

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = NULL;

	PostUnbind();
}

/*
================
idEntity::RemoveBinds
================
*/
void idEntity::RemoveBinds( void ) {
	idEntity *ent;
	idEntity *next;

	for( ent = teamChain; ent != NULL; ent = next ) {
		next = ent->teamChain;
		if ( ent->bindMaster == this ) {
			ent->Unbind();
			ent->PostEventMS( &EV_Remove, 0 );
			next = teamChain;
		}
	}
}

/*
================
idEntity::IsBound
================
*/
bool idEntity::IsBound( void ) const {
	if ( bindMaster ) {
		return true;
	}
	return false;
}

/*
================
idEntity::IsBoundTo
================
*/
// RAVEN BEGIN
// abahr: added const so it can be called from const functions
bool idEntity::IsBoundTo( const idEntity *master ) const {
// RAVEN END
	idEntity *ent;

	if ( !bindMaster ) {
		return false;
	}

	for ( ent = bindMaster; ent != NULL; ent = ent->bindMaster ) {
		if ( ent == master ) {
			return true;
		}
	}

	return false;
}

/*
================
idEntity::GetBindMaster
================
*/
idEntity *idEntity::GetBindMaster( void ) const {
	return bindMaster;
}

/*
================
idEntity::GetBindJoint
================
*/
jointHandle_t idEntity::GetBindJoint( void ) const {
	return bindJoint;
}

/*
================
idEntity::GetBindBody
================
*/
int idEntity::GetBindBody( void ) const {
	return bindBody;
}

/*
================
idEntity::GetTeamMaster
================
*/
idEntity *idEntity::GetTeamMaster( void ) const {
	return teamMaster;
}

/*
================
idEntity::GetNextTeamEntity
================
*/
idEntity *idEntity::GetNextTeamEntity( void ) const {
	return teamChain;
}

/*
=====================
idEntity::ConvertLocalToWorldTransform
=====================
*/
void idEntity::ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis ) {
	UpdateModelTransform();

	offset = renderEntity.origin + offset * renderEntity.axis;
	axis *= renderEntity.axis;
}

/*
================
idEntity::GetLocalVector

Takes a vector in worldspace and transforms it into the parent
object's localspace.

Note: Does not take origin into acount.  Use getLocalCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetLocalVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetLocalCoordinates

Takes a vector in world coordinates and transforms it into the parent
object's local coordinates.
================
*/
idVec3 idEntity::GetLocalCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec - masterOrigin, pos );

	return pos;
}

// RAVEN BEGIN
// kfuller: added method

/*
================
idEntity::DistanceTo2d
================
*/
float idEntity::DistanceTo2d ( const idVec3& pos ) const {
	idVec3 pos1;
	idVec3 pos2;
	pos1 = pos - (pos * GetPhysics()->GetGravityNormal ( )) * GetPhysics()->GetGravityNormal ( );
	pos2 = GetPhysics()->GetOrigin ( );
	pos2 = pos2 - (pos2 * GetPhysics()->GetGravityNormal ( )) * GetPhysics()->GetGravityNormal ( );
	return (pos2 - pos1).LengthFast ( );
}

/*
================
idEntity::GetLocalAngles
================
*/
void idEntity::GetLocalAngles(idAngles &localAng) 
{
	idVec3 localVec = GetPhysics()->GetAxis()[0];

	GetLocalVector(localVec);
	localAng = localVec.ToAngles();
}
// RAVEN END

/*
================
idEntity::GetWorldVector

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.

Note: Does not take origin into acount.  Use getWorldCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetWorldVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetWorldCoordinates

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.
================
*/
idVec3 idEntity::GetWorldCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );
	pos += masterOrigin;

	return pos;
}

/*
================
idEntity::GetMasterPosition
================
*/
bool idEntity::GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	idVec3		localOrigin;
	idMat3		localAxis;
	idAnimator	*masterAnimator;

	if ( bindMaster ) {
		// if bound to a joint of an animated model
		if ( bindJoint != INVALID_JOINT ) {
			masterAnimator = bindMaster->GetAnimator();
			if ( !masterAnimator ) {
				masterOrigin = vec3_origin;
				masterAxis = mat3_identity;
				return false;
			} else {
				masterAnimator->GetJointTransform( bindJoint, gameLocal.time, masterOrigin, masterAxis );
				masterAxis *= bindMaster->renderEntity.axis;
				masterOrigin = bindMaster->renderEntity.origin + masterOrigin * bindMaster->renderEntity.axis;
			}
		} else if ( bindBody >= 0 && bindMaster->GetPhysics() ) {
			masterOrigin = bindMaster->GetPhysics()->GetOrigin( bindBody );
			masterAxis = bindMaster->GetPhysics()->GetAxis( bindBody );
		} else {
			masterOrigin = bindMaster->renderEntity.origin;
			masterAxis = bindMaster->renderEntity.axis;
		}
		return true;
	} else {
		masterOrigin = vec3_origin;
		masterAxis = mat3_identity;
		return false;
	}
}

// RAVEN BEGIN
// abahr: needed so client get the correct position
/*
================
idEntity::GetPosition
================
*/
void idEntity::GetPosition( idVec3& origin, idMat3& axis ) const {
	origin = renderEntity.origin;
	axis = renderEntity.axis;
}
// RAVEN END

/*
================
idEntity::GetWorldVelocities
================
*/
void idEntity::GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const {

	linearVelocity = physics->GetLinearVelocity();
	angularVelocity = physics->GetAngularVelocity();

	if ( bindMaster ) {
		idVec3 masterOrigin, masterLinearVelocity, masterAngularVelocity;
		idMat3 masterAxis;

		// get position of master
		GetMasterPosition( masterOrigin, masterAxis );

		// get master velocities
		bindMaster->GetWorldVelocities( masterLinearVelocity, masterAngularVelocity );

		// linear velocity relative to master plus master linear and angular velocity
		linearVelocity = linearVelocity * masterAxis + masterLinearVelocity +
								masterAngularVelocity.Cross( GetPhysics()->GetOrigin() - masterOrigin );
	}
}

/*
================
idEntity::JoinTeam
================
*/
void idEntity::JoinTeam( idEntity *teammember ) {
	idEntity *ent;
	idEntity *master;
	idEntity *prev;
	idEntity *next;

	// if we're already on a team, quit it so we can join this one
	if ( teamMaster && ( teamMaster != this ) ) {
		QuitTeam();
	}

	assert( teammember );

	if ( teammember == this ) {
		teamMaster = this;
		return;
	}

	// check if our new team mate is already on a team
	master = teammember->teamMaster;
	if ( !master ) {
		// he's not on a team, so he's the new teamMaster
		master = teammember;
		teammember->teamMaster = teammember;
		teammember->teamChain = this;

		// make anyone who's bound to me part of the new team
		for( ent = teamChain; ent != NULL; ent = ent->teamChain ) {
			ent->teamMaster = master;
		}
	} else {
		// skip past the chain members bound to the entity we're teaming up with
		prev = teammember;
		next = teammember->teamChain;
		if ( bindMaster ) {
			// if we have a bindMaster, join after any entities bound to the entity
			// we're joining
			while( next && next->IsBoundTo( teammember ) ) {
				prev = next;
				next = next->teamChain;
			}
		} else {
			// if we're not bound to someone, then put us at the end of the team
			while( next ) {
				prev = next;
				next = next->teamChain;
			}
		}

		// make anyone who's bound to me part of the new team and
		// also find the last member of my team
		for( ent = this; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamChain->teamMaster = master;
		}

    	prev->teamChain = this;
		ent->teamChain = next;
	}

	teamMaster = master;

	// reorder the active entity list 
	gameLocal.sortTeamMasters = true;
}

/*
================
idEntity::QuitTeam
================
*/
void idEntity::QuitTeam( void ) {
	idEntity *ent;

	if ( !teamMaster ) {
		return;
	}

	// check if I'm the teamMaster
	if ( teamMaster == this ) {
		// do we have more than one teammate?
		if ( !teamChain->teamChain ) {
			// no, break up the team
			teamChain->teamMaster = NULL;
		} else {
			// yes, so make the first teammate the teamMaster
			for( ent = teamChain; ent; ent = ent->teamChain ) {
				ent->teamMaster = teamChain;
			}
		}
	} else {
		assert( teamMaster );
		assert( teamMaster->teamChain );

		// find the previous member of the teamChain
		ent = teamMaster;
		while( ent->teamChain != this ) {
			assert( ent->teamChain ); // this should never happen
			ent = ent->teamChain;
		}

		// remove this from the teamChain
		ent->teamChain = teamChain;

		// if no one is left on the team, break it up
		if ( !teamMaster->teamChain ) {
			teamMaster->teamMaster = NULL;
		}
	}

	teamMaster = NULL;
	teamChain = NULL;
}

/***********************************************************************

  Physics.
	
***********************************************************************/

/*
================
idEntity::InitDefaultPhysics
================
*/
void idEntity::InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis ) {
	const char *temp;
	idClipModel *clipModel = NULL;

	// check if a clipmodel key/value pair is set
	if ( spawnArgs.GetString( "clipmodel", "", &temp ) ) {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_HEAP_MEM(this);
// RAVEN END
		clipModel = new idClipModel( temp );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_POP_HEAP();
// RAVEN END
	}

	if ( !spawnArgs.GetBool( "noclipmodel", "0" ) ) {

		// check if mins/maxs or size key/value pairs are set
		if ( !clipModel ) {
			idVec3 size;
			idBounds bounds;
			bool setClipModel = false;

			if ( spawnArgs.GetVector( "mins", NULL, bounds[0] ) &&
				spawnArgs.GetVector( "maxs", NULL, bounds[1] ) ) {
				setClipModel = true;
				if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] ) {
					gameLocal.Error( "Invalid bounds '%s'-'%s' on entity '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
				}
			} else if ( spawnArgs.GetVector( "size", NULL, size ) ) {
				if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) ) {
					gameLocal.Error( "Invalid size '%s' on entity '%s'", size.ToString(), name.c_str() );
				}
				bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
				bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
				setClipModel = true;
			}

			if ( setClipModel ) {
				int numSides;
				idTraceModel trm;

				if ( spawnArgs.GetInt( "cylinder", "0", numSides ) && numSides > 0 ) {
					trm.SetupCylinder( bounds, numSides < 3 ? 3 : numSides );
				} else if ( spawnArgs.GetInt( "cone", "0", numSides ) && numSides > 0 ) {
					trm.SetupCone( bounds, numSides < 3 ? 3 : numSides );
// RAVEN BEGIN
// bdube: added dodecahedron
				} else if ( spawnArgs.GetInt( "dodecahedron", "0", numSides ) && numSides > 0 ) {
					trm.SetupDodecahedron ( bounds );
// RAVEN END
				} else {
					trm.SetupBox( bounds );
				}
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
				RV_PUSH_HEAP_MEM(this);
// RAVEN END
				clipModel = new idClipModel( trm );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
				RV_POP_HEAP();
// RAVEN END
			}
		}

		// check if the visual model can be used as collision model
		if ( !clipModel ) {
			temp = spawnArgs.GetString( "model" );
			if ( ( temp != NULL ) && ( *temp != 0 ) ) {
// RAVEN BEGIN
// jscott:slash problems
				idStr canonical = temp;
				canonical.BackSlashesToSlashes();
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
				RV_PUSH_HEAP_MEM(this);
// RAVEN END
				clipModel = new idClipModel();
				if ( !clipModel->LoadModel( canonical ) ) {
					delete clipModel;
					clipModel = NULL;
				}
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
				RV_POP_HEAP();
// RAVEN END
			}
		}
	}

	defaultPhysicsObj.SetSelf( this );
	defaultPhysicsObj.SetClipModel( clipModel, 1.0f );
	defaultPhysicsObj.SetOrigin( origin );
	defaultPhysicsObj.SetAxis( axis );

	physics = &defaultPhysicsObj;
}

/*
================
idEntity::SetPhysics
================
*/
void idEntity::SetPhysics( idPhysics *phys ) {
	// clear any contacts the current physics object has
	if ( physics ) {
		physics->ClearContacts();
	}
	// set new physics object or set the default physics if NULL
	if ( phys != NULL ) {
		defaultPhysicsObj.SetClipModel( NULL, 1.0f );
		physics = phys;
		physics->Activate();
	} else {
		physics = &defaultPhysicsObj;
	}
	physics->UpdateTime( gameLocal.time );
	physics->SetMaster( bindMaster, fl.bindOrientated );
}

/*
================
idEntity::RestorePhysics
================
*/
void idEntity::RestorePhysics( idPhysics *phys ) {
	assert( phys != NULL );
	// restore physics pointer
	physics = phys;
}

/*
================
idEntity::RunPhysics
================
*/
bool idEntity::RunPhysics( void ) {
	int			i, reachedTime, startTime, endTime;
	idEntity *	part, *blockedPart, *blockingEntity = NULL;
	trace_t		results;
	bool		moved;

	moved = false;

// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_PHYSICS);
// RAVEN END

	// don't run physics if not enabled
	if ( !( thinkFlags & TH_PHYSICS ) ) {
		// however do update any animation controllers
		if ( UpdateAnimationControllers() ) {
			BecomeActive( TH_ANIMATE );
		}
// RAVEN BEGIN
// kfuller: we want to be able to debug draw the bbox regardless
		physics->DebugDraw();
// RAVEN END
		return false;
	}

	// if this entity is a team slave don't do anything because the team master will handle everything
	if ( teamMaster && teamMaster != this ) {
		return false;
	}

	startTime = gameLocal.previousTime;
	endTime = gameLocal.time;

	gameLocal.push.InitSavingPushedEntityPositions();
	blockedPart = NULL;

	// save the physics state of the whole team and disable the team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->DisableClip();
			}
			part->physics->SaveState();
		}
	}

	// move the whole team
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics ) {

			// run physics
// RAVEN BEGIN
// ddynerman: optional pre-prediction
			moved = part->physics->Evaluate( endTime - startTime + part->predictTime, endTime );
			part->predictTime = 0;
// RAVEN END

			// check if the object is blocked
			blockingEntity = part->physics->GetBlockingEntity();
			if ( blockingEntity ) {
				blockedPart = part;
				break;
			}

			// if moved or forced to update the visual position and orientation from the physics
			if ( moved || part->fl.forcePhysicsUpdate ) {
				part->UpdateFromPhysics( false );
			}

			// update any animation controllers here so an entity bound
			// to a joint of this entity gets the correct position
			if ( part->UpdateAnimationControllers() ) {
				part->BecomeActive( TH_ANIMATE );
			}
		}
	}

	// enable the whole team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->EnableClip();
			}
		}
	}

	// cdr: Obstacle Avoidance
	if (ai_useRVMasterMove.GetBool() && moved && fl.isAIObstacle) {
		AI_EntityMoved(this);
	}

	// if one of the team entities is a pusher and blocked
	if ( blockedPart ) {
		// move the parts back to the previous position
		for ( part = this; part != blockedPart; part = part->teamChain ) {

			if ( part->physics ) {

				// restore the physics state
				part->physics->RestoreState();

				// move back the visual position and orientation
				part->UpdateFromPhysics( true );
			}
		}
		for ( part = this; part != NULL; part = part->teamChain ) {
			if ( part->physics ) {
				// update the physics time without moving
				part->physics->UpdateTime( endTime );
			}
		}

		// restore the positions of any pushed entities
		gameLocal.push.RestorePushedEntityPositions();

		if ( gameLocal.isClient ) {
			return false;
		}

		// if the master pusher has a "blocked" function, call it
		Signal( SIG_BLOCKED );
		ProcessEvent( &EV_TeamBlocked, blockedPart, blockingEntity );
		// call the blocked function on the blocked part
		blockedPart->ProcessEvent( &EV_PartBlocked, blockingEntity );
		return false;
	}

	// set pushed
	for ( i = 0; i < gameLocal.push.GetNumPushedEntities(); i++ ) {
		idEntity *ent = gameLocal.push.GetPushedEntity( i );
		ent->physics->SetPushed( endTime - startTime );
	}

	if ( gameLocal.isClient ) {
		return true;
	}

	// post reached event if the current time is at or past the end point of the motion
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics ) {

			reachedTime = part->physics->GetLinearEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedPos );
			}
			reachedTime = part->physics->GetAngularEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedAng );
			}
		}
	}

	return true;
}

/*
================
idEntity::UpdateFromPhysics
================
*/
void idEntity::UpdateFromPhysics( bool moveBack ) {

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( IsType( idActor::GetClassType() ) ) {
// RAVEN END
		idActor *actor = static_cast<idActor *>( this );

		// set master delta angles for actors
		if ( GetBindMaster() ) {
			idAngles delta = actor->GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			} else {
				delta.yaw += static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			}
			actor->SetDeltaViewAngles( delta );
		}
	}

	UpdateVisuals();
}

/*
================
idEntity::SetOrigin
================
*/
void idEntity::SetOrigin( const idVec3 &org ) {

	GetPhysics()->SetOrigin( org );

	UpdateVisuals();
}

/*
================
idEntity::SetAxis
================
*/
void idEntity::SetAxis( const idMat3 &axis ) {

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( GetPhysics()->IsType( idPhysics_Actor::GetClassType() ) ) {
// RAVEN END
		static_cast<idActor *>(this)->viewAxis = axis;
	} else {
		GetPhysics()->SetAxis( axis );
	}

	UpdateVisuals();
}

/*
================
idEntity::SetAngles
================
*/
void idEntity::SetAngles( const idAngles &ang ) {
	SetAxis( ang.ToMat3() );
}

/*
================
idEntity::GetFloorPos
================
*/
bool idEntity::GetFloorPos( float max_dist, idVec3 &floorpos ) const {
	trace_t result;

	if ( !GetPhysics()->HasGroundContacts() ) {
		GetPhysics()->ClipTranslation( result, GetPhysics()->GetGravityNormal() * max_dist, NULL );
		if ( result.fraction < 1.0f ) {
			floorpos = result.endpos;
			return true;
		} else {
			floorpos = GetPhysics()->GetOrigin();
			return false;
		}
	} else {
		floorpos = GetPhysics()->GetOrigin();
		return true;
	}
}

/*
================
idEntity::GetPhysicsToVisualTransform
================
*/
bool idEntity::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	return false;
}

/*
================
idEntity::GetPhysicsToSoundTransform
================
*/
bool idEntity::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	// by default play the sound at the center of the bounding box of the first clip model
	if ( GetPhysics()->GetNumClipModels() > 0 ) {
		origin = GetPhysics()->GetBounds().GetCenter();
		axis.Identity();
		return true;
	}
	return false;
}

/*
================
idEntity::Collide
================
*/
bool idEntity::Collide( const trace_t &collision, const idVec3 &velocity ) {
	// this entity collides with collision.c.entityNum
	return false;
}

/*
================
idEntity::GetImpactInfo
================
*/
void idEntity::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	GetPhysics()->GetImpactInfo( id, point, info );
}

/*
================
idEntity::ApplyImpulse
================
*/
void idEntity::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash ) {
	if( SkipImpulse(ent, id) ) {
		return;
	}

	GetPhysics()->ApplyImpulse( id, point, impulse );
}

/*
================
idEntity::AddForce
================
*/
void idEntity::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	GetPhysics()->AddForce( id, point, force );
}

/*
================
idEntity::ActivatePhysics
================
*/
void idEntity::ActivatePhysics( idEntity *ent ) {
	GetPhysics()->Activate();
}

/*
================
idEntity::IsAtRest
================
*/
bool idEntity::IsAtRest( void ) const {
	return GetPhysics()->IsAtRest();
}

/*
================
idEntity::GetRestStartTime
================
*/
int idEntity::GetRestStartTime( void ) const {
	return GetPhysics()->GetRestStartTime();
}

/*
================
idEntity::AddContactEntity
================
*/
void idEntity::AddContactEntity( idEntity *ent ) {
	GetPhysics()->AddContactEntity( ent );
}

/*
================
idEntity::RemoveContactEntity
================
*/
void idEntity::RemoveContactEntity( idEntity *ent ) {
// RAVEN BEGIN
	if( GetPhysics() ) {

		GetPhysics()->RemoveContactEntity( ent );
	}
// RAVEN END
}



/***********************************************************************

	Damage
	
***********************************************************************/

/*
============
idEntity::CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
// RAVEN BEGIN
// bdube: added ignore entity
bool idEntity::CanDamage( const idVec3 &origin, idVec3 &damagePoint, idEntity* ignoreEnt ) const {
// RAVEN END
	idVec3 	dest;
	trace_t	tr;
	idVec3 	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin at 0,0,0
	midpoint = ( GetPhysics()->GetAbsBounds()[0] + GetPhysics()->GetAbsBounds()[1] ) * 0.5;

	dest = midpoint;
// RAVEN BEGIN
// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN END
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	// this should probably check in the plane of projection, rather than in world coordinate
	dest = midpoint;
	dest[0] += 15.0;
	dest[1] += 15.0;
// RAVEN BEGIN
// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN ENE
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] += 15.0;
	dest[1] -= 15.0;
// RAVEN BEGIN
// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN END
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] += 15.0;
// RAVEN BEGIN
// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN END
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] -= 15.0;
// RAVEN BEGIN
// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN EN
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] += 15.0;
// RAVEN BEGIN
// ddynerman: multiple collision worlds
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, NULL );
// RAVEN END
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] -= 15.0;
// RAVEN BEGIN
// ddynerman: multiple collision worlds
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, NULL );
// RAVEN END
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	return false;
}

/*
================
idEntity::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idEntity::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	// implemented in subclasses
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and point can be NULL for environmental effects

============
*/
void idEntity::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location ) {
	if ( forwardDamageEnt.IsValid() ) {
		forwardDamageEnt->Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
		return;
	}

	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage ) {
		// do the damage
		//jshepard: this is kinda important, no?
		health -= damage;

		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	}
}

/*
============
idEntity::SkipImpulse
============
*/
// RAVEN BEGIN
// abahr: push stuff
bool idEntity::SkipImpulse( idEntity *ent, int id ) {
	return false;//ent == this;
}

/*
============
idEntity::ApplyImpulse
============
*/
void idEntity::ApplyImpulse( idEntity* ent, int id, const idVec3& point, const idVec3& dir, const idDict* damageDef ) {
	ApplyImpulse( ent, id, point, dir * damageDef->GetFloat("push", "5000") );
}
// RAVEN END

/*
================
idEntity::AddDamageEffect
================
*/
void idEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	const char *sound, *decal, *key;

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
// RAVEN BEGIN
// bdube: impact_blood is now in the damage def
	if ( def == NULL || !def->dict.GetBool ( "bleed" ) ) {
// RAVEN END
		return;
	}

	const char *materialType = gameLocal.sufaceTypeNames[ collision.c.material->GetSurfaceType() ];

	// start impact sound based on material type
	key = va( "snd_%s", materialType );
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}

	if ( g_decals.GetBool() ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", materialType );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			idVec3 dir = velocity;
			dir.Normalize();
			ProjectOverlay( collision.c.point, dir, 20.0f, decal );
		}
	}
}

/*
================
idEntity::CanPlayImpactEffect
================
*/
bool idEntity::CanPlayImpactEffect ( idEntity* owner, idEntity* ent ) {
	if( gameLocal.isMultiplayer ) {
		if( gameLocal.IsTeamGame() && !cvarSystem->GetCVarBool("si_teamDamage") && owner->IsType( idPlayer::GetClassType() ) && ent->IsType( idPlayer::GetClassType() ) && ((idPlayer*)owner)->team == ((idPlayer*)ent)->team ) {
			return false;		 
		}
 
		// default to blood
		return true;
	} else {
		idActor* actorOwner;
		idAI* aiEnt;
		actorOwner = dynamic_cast<idActor*>( owner );

		if ( ent->IsType ( idAFAttachment::GetClassType ( ) ) ) {
			aiEnt = dynamic_cast<idAI*>( static_cast<idAFAttachment*>( ent )->GetBody ( )  );
		} else {
			aiEnt = dynamic_cast<idAI*>( ent );
		}

		if ( !actorOwner || !aiEnt ) {
			return true;
		}

		return (actorOwner->team != aiEnt->team);
	}
}

/*
============
idEntity::Pain

Called whenever an entity recieves damage.  Returns whether the entity responds to the pain.
This is a virtual function that subclasses are expected to implement.
============
*/
bool idEntity::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	return false;
}

/*
============
idEntity::Killed

Called whenever an entity's health is reduced to 0 or less.
This is a virtual function that subclasses are expected to implement.
============
*/
void idEntity::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
}

/***********************************************************************

  Script functions
	
***********************************************************************/

/*
================
idEntity::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idEntity::ShouldConstructScriptObjectAtSpawn( void ) const {
	return true;
}

/*
================
idEntity::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idEntity::ConstructScriptObject( void ) {
	idThread *thread;
	const function_t *constructor;

	// init the script object's data
	scriptObject.ClearObject();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( constructor ) {
		// start a thread that will initialize after Spawn is done being called
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_HEAP_MEM(this);
// RAVEN END
		thread = new idThread();
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_POP_HEAP();
// RAVEN END
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, constructor, true );
		thread->DelayedStart( 0 );
	} else {
		thread = NULL;
	}

	// clear out the object's memory
	scriptObject.ClearObject();

	return thread;
}

/*
================
idEntity::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idEntity::DeconstructScriptObject( void ) {
	idThread		*thread;
	const function_t *destructor;

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and be destroyed
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_TEMPORARY);
// RAVEN END
		thread = new idThread();
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_POP_HEAP();
// RAVEN END
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		delete thread;
	}
}

/*
================
idEntity::HasSignal
================
*/
bool idEntity::HasSignal( signalNum_t signalnum ) const {
	if ( !signals ) {
		return false;
	}
	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );
	return ( signals->signal[ signalnum ].Num() > 0 );
}

/*
================
idEntity::SetSignal
================
*/
void idEntity::SetSignal( signalNum_t signalnum, idThread *thread, const function_t *function ) {
	int			i;
	int			num;
	signal_t	sig;
	int			threadnum;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		signals = new signalList_t;
	}

	assert( thread );
	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ][ i ].function = function;
			return;
		}
	}

	if ( num >= MAX_SIGNAL_THREADS ) {
		thread->Error( "Exceeded maximum number of signals per object" );
	}

	sig.threadnum = threadnum;
	sig.function = function;
	signals->signal[ signalnum ].Append( sig );
}

/*
================
idEntity::ClearSignal
================
*/
void idEntity::ClearSignal( idThread *thread, signalNum_t signalnum ) {
	assert( thread );
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	signals->signal[ signalnum ].Clear();
}

/*
================
idEntity::ClearSignalThread
================
*/
void idEntity::ClearSignalThread( signalNum_t signalnum, idThread *thread ) {
	int	i;
	int	num;
	int	threadnum;

	assert( thread );

	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ].RemoveIndex( i );
			return;
		}
	}
}

/*
================
idEntity::Signal
================
*/
void idEntity::Signal( signalNum_t signalnum ) {
	int			i;
	int			num;
	signal_t	sigs[ MAX_SIGNAL_THREADS ];
	idThread	*thread;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		return;
	}

	// we copy the signal list since each thread has the potential
	// to end any of the threads in the list.  By copying the list
	// we don't have to worry about the list changing as we're
	// processing it.
	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		sigs[ i ] = signals->signal[ signalnum ][ i ];
	}

	// clear out the signal list so that we don't get into an infinite loop
	signals->signal[ signalnum ].Clear();

	for( i = 0; i < num; i++ ) {
		thread = idThread::GetThread( sigs[ i ].threadnum );
		if ( thread ) {
			thread->CallFunction( this, sigs[ i ].function, true );
			thread->Execute();
		}
	}
}

/*
================
idEntity::SignalEvent
================
*/
void idEntity::SignalEvent( idThread *thread, signalNum_t signalnum ) {
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	Signal( signalnum );
}

/***********************************************************************

  Guis.
	
***********************************************************************/


/*
================
idEntity::TriggerGuis
================
*/
void idEntity::TriggerGuis( void ) {
	int i;
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			renderEntity.gui[ i ]->Trigger( gameLocal.time );
		}
	}
}

/*
================
idEntity::HandleGuiCommands
================
*/
bool idEntity::HandleGuiCommands( idEntity *entityGui, const char *cmds ) {
	idEntity *targetEnt;
	bool ret = false;
	if ( entityGui && cmds && *cmds ) {
		idLexer src;
		idToken token, token2, token3, token4;
		src.LoadMemory( cmds, strlen( cmds ), "guiCommands" );
		while( 1 ) {

			if ( !src.ReadToken( &token ) ) {
				return ret;
			}

			if ( token == ";" ) {
				continue;
			}

			if ( token.Icmp( "activate" ) == 0 ) {
				bool targets = true;
				if ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
					} else {
						targets = false;
					}
				}

				if ( targets ) {
					entityGui->ActivateTargets( this );
				} else {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->Signal( SIG_TRIGGER );
						ent->PostEventMS( &EV_Activate, 0, this );
					}
				}

				entityGui->renderEntity.shaderParms[ SHADERPARM_MODE ] = 1.0f;
				continue;
			}


			if ( token.Icmp( "runScript" ) == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
  					while( src.CheckTokenString( "::" ) ) {
  						idToken token3;
  						if ( !src.ReadToken( &token3 ) ) {
  							gameLocal.Error( "Expecting function name following '::' in gui for entity '%s'", entityGui->name.c_str() );
  						}
  						token2 += "::" + token3;
  					}
				}
// RAVEN BEGIN
// abahr: allow parms to be passed in
// For some reason the semi colon is used as a delimeter so we need the above code
				rvScriptFuncUtility utility;
				if( utility.Init(token2) > SFU_ERROR ) {
					utility.InsertEntity( entityGui, 0 );
					utility.CallFunc( &entityGui->spawnArgs );
				}
// RAVEN END
				continue;
			}

			if ( token.Icmp("play") == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					const idSoundShader *shader = declManager->FindSound(token2);
					entityGui->StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, NULL );
				}
				continue;
			}

			if ( token.Icmp( "setkeyval" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) && src.ReadToken( &token4 ) ) {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->spawnArgs.Set( token3, token4 );
						ent->UpdateChangeableSpawnArgs( NULL );
						ent->UpdateVisuals();
					}
				}
				continue;
			}

			if ( token.Icmp( "setshaderparm" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) ) {
					entityGui->SetShaderParm( atoi( token2 ), atof( token3 ) );
					entityGui->UpdateVisuals();
				}
				continue;
			}

			if ( token.Icmp("close") == 0 ) {
				ret = true;
				continue;
			}

			// handy for debugging GUI stuff
			if ( !token.Icmp( "print" ) ) {
				idStr msg;
				while ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
						break;
					}
					msg += token2.c_str();
				}
				common->Printf( "ent gui 0x%x '%s': %s\n", entityNumber, name.c_str(), msg.c_str() );
				continue;
			}

			// if we get to this point we don't know how to handle it
			src.UnreadToken(&token);
			if ( !HandleSingleGuiCommand( entityGui, &src ) ) {
				// not handled there see if entity or any of its targets can handle it
				// this will only work for one target atm
				if ( entityGui->HandleSingleGuiCommand( entityGui, &src ) ) {
					continue;
				}

				int c = entityGui->targets.Num();
				int i;
				for ( i = 0; i < c; i++) {
					targetEnt = entityGui->targets[ i ].GetEntity();
					if ( targetEnt && targetEnt->HandleSingleGuiCommand( entityGui, &src ) ) {
						break;
					}
				}

				if ( i == c ) {
					// not handled
					common->DPrintf( "idEntity::HandleGuiCommands: '%s' not handled\n", token.c_str() );
					src.ReadToken( &token );
				}
			}

		}
	}
	return ret;
}

/*
================
idEntity::HandleSingleGuiCommand
================
*/
bool idEntity::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	return false;
}

/***********************************************************************

  Targets
	
***********************************************************************/

/*
===============
idEntity::FindTargets

We have to wait until all entities are spawned
Used to build lists of targets after the entity is spawned.  Since not all entities
have been spawned when the entity is created at map load time, we have to wait
===============
*/
void idEntity::FindTargets( void ) {
	int			i;

	// targets can be a list of multiple names
	gameLocal.GetTargets( spawnArgs, targets, "target" );

	// ensure that we don't target ourselves since that could cause an infinite loop when activating entities
	for( i = 0; i < targets.Num(); i++ ) {
		if ( targets[ i ].GetEntity() == this ) {
			gameLocal.Error( "Entity '%s' is targeting itself", name.c_str() );
		}
	}
}

/*
================
idEntity::RemoveNullTargets
================
*/
void idEntity::RemoveNullTargets( void ) {
	int i;

	for( i = targets.Num() - 1; i >= 0; i-- ) {
		if ( !targets[ i ].GetEntity() ) {
			targets.RemoveIndex( i );
		}
	}
}

/*
==============================
idEntity::ActivateTargets

"activator" should be set to the entity that initiated the firing.
==============================
*/
void idEntity::ActivateTargets( idEntity *activator ) const {
	idEntity	*ent;
	int			i, j;
	
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
			ent->Signal( SIG_TRIGGER );
			ent->ProcessEvent( &EV_Activate, activator );
		} 		
		for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->renderEntity.gui[ j ] ) {
				ent->renderEntity.gui[ j ]->Trigger( gameLocal.time );
			}
		}
	}
}

// RAVEN BEGIN
// twhitaker: added (meant to be used from script)
/*
================
idEntity::AppendTarget
================
*/
int idEntity::AppendTarget( idEntity *appendMe ) {

	int index = -1;
	// silently fail if they pass in null
	if ( appendMe )	{
		index = targets.Append( appendMe );
		RemoveNullTargets();
	}
	return index;
}
/*
================
idEntity::RemoveTarget
================
*/
void idEntity::RemoveTarget( idEntity *removeMe ) {

	targets.Remove( removeMe );
	RemoveNullTargets();
}
/*
================
idEntity::RemoveTargets
================
*/
void idEntity::RemoveTargets( bool destroyContents ) {
	if( destroyContents ) {
		targets.RemoveContents( true );
	} else {
		targets.Clear();
	}
}

// jshepard: added
/*
================
idEntity::UnbindTargets
================
*/
void idEntity::UnbindTargets( idEntity *activator ) const {
	idEntity	*ent;
	int			i;
	
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		ent->Unbind();		

	}
}

// bdube: added
/*
================
idEntity::Event_SetContents
================
*/
void idEntity::Event_SetContents( int contents ) {
	GetPhysics()->SetContents( contents );
}

/*
================
idEntity::Event_GetLastBlocker
================
*/
void idEntity::Event_GetLastBlocker(idThread *thread) {
	int	whichEntNum = GetLastBlocker();

	if (whichEntNum < 0 || whichEntNum == ENTITYNUM_WORLD) {
		thread->ReturnEntity(this);
		return;
	}
	thread->ReturnEntity(gameLocal.entities[whichEntNum]);
}

/*
================
idEntity::ShowSurface
================
*/
void idEntity::ShowSurface ( const char* surface ) {
	if ( !renderEntity.hModel || !surface || !*surface ) {
		return;
	}

#if 0 //k: not implament
	renderEntity.suppressSurfaceMask &= (~renderEntity.hModel->GetSurfaceMask ( surface ));
#endif
}

/*
================
idEntity::Event_ShowSurface
================
*/
void idEntity::Event_ShowSurface ( const char* surface ) {
	ShowSurface ( surface );
}

/*
================
idEntity::HideSurface
================
*/
void idEntity::HideSurface ( const char* surface ) {
	if ( !renderEntity.hModel || !surface || !*surface ) {
		return;
	}

#if 0 //k: not implament
	renderEntity.suppressSurfaceMask |= renderEntity.hModel->GetSurfaceMask ( surface ) ;
#endif
}

/*
================
idEntity::Event_HideSurface
================
*/
void idEntity::Event_HideSurface ( const char* surface ) {
	HideSurface ( surface );
}

/*
================
idEntity::Event_GuiEvent
================
*/
void idEntity::Event_GuiEvent ( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent ( eventName );
	} 
}

/*
================
idEntity::Event_clearSkin
================
*/
void idEntity::Event_ClearSkin( void ) {
	ClearSkin();
}

/*
================
idEntity::Event_StopAllEffects
================
*/
void idEntity::Event_StopAllEffects ( void ) {
	StopAllEffects ( );
}

/*
================
idEntity::Event_GetHealth
================
*/
void idEntity::Event_GetHealth ( void ) {
	idThread::ReturnFloat( health );
}

// jscott:
/*
================
idEntity::Event_PlaybackCallback
================
*/
void idEntity::Event_PlaybackCallback ( int type, int changed, int impulse ) {
	common->Printf( "Playback callback type %d - %d/%d\n", type, changed, impulse );
}

// nmckenzie: Check who we're bound to.
/*
================
idEntity::Event_GetBindMaster
================
*/

void idEntity::Event_GetBindMaster ( void ) {
	idThread::ReturnEntity( GetBindMaster() );
}

/*
================
idEntity::Event_ApplyImpulse
================
*/

void idEntity::Event_ApplyImpulse( idEntity *source, const idVec3 &point, const idVec3 &impulse ){
	ApplyImpulse( source, 0, point, impulse );
}

/*
================
idEntity::Event_PlayEffect
================
*/
void idEntity::Event_PlayEffect( const char *effectName, const char* jointName, bool loop ) {
	jointHandle_t joint;
	joint = GetAnimator ( ) ? GetAnimator()->GetJointHandle ( jointName ) : INVALID_JOINT;
	if ( joint != INVALID_JOINT ) {
		PlayEffect ( effectName, joint, loop );
	} else {
		PlayEffect ( effectName, renderEntity.origin, renderEntity.axis, loop );
	}	
}

/*
================
idEntity::Event_StopEffect
================
*/
void idEntity::Event_StopEffect( const char *effectName ) {
	StopEffect ( effectName );
}

// END RAVEN

/***********************************************************************

  Misc.
	
***********************************************************************/

/*
================
idEntity::Teleport
================
*/
void idEntity::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin );
	GetPhysics()->SetAxis( angles.ToMat3() );

	UpdateVisuals();
}

/*
============
idEntity::TouchTriggers

  Activate all trigger entities touched at the current position.

  Optionally only activate triggers of ownerType
============
*/
#ifdef _QUAKE4
static idCVar harm_g_useSimpleTriggerClipForce( "harm_g_useSimpleTriggerClipForce", "0", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "Force to Using simple trigger clip for player/AI/vehicle touching triggers, it can optimize some solutions for doors can' open in Quake4Doom(not need enable no longer, collision has been fixed). "
		);
#endif
bool idEntity::TouchTriggers( const idTypeInfo* ownerType ) const {
	int				i, numClipModels, numEntities;
	idClipModel *	cm;
	idClipModel *	clipModels[ MAX_GENTITIES ];
	idEntity *		ent;
	trace_t			trace;

	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = GetPhysics()->GetOrigin();
	trace.endAxis = GetPhysics()->GetAxis();

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	numClipModels = gameLocal.ClipModelsTouchingBounds( this, GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER, clipModels, MAX_GENTITIES );
// RAVEN END
	numEntities = 0;

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[ i ];

		// don't touch it if we're the owner
		if ( cm->GetOwner() == this ) {
			continue;
		}

		ent = cm->GetEntity();

		if ( !ent->RespondsTo( EV_Touch ) && !ent->HasSignal( SIG_TOUCH ) ) {
			continue;
		}

		if( ownerType && !ent->IsType( *ownerType ) ) {
			continue;
		}

#ifdef _QUAKE4 //k: force using simple clip, so has touched now
		if(!harm_g_useSimpleTriggerClipForce.GetBool()
				|| ent->IsType(idTrigger_Hurt::GetClassType()) //k: some hurt trigger is danger
				)
		{
#endif
// RAVEN BEGIN
// abahr: needed so tram car can has collision model and touch triggers
		bool useSimpleClip = spawnArgs.GetBool("useSimpleTriggerClip");
		if ( !useSimpleClip && !GetPhysics()->ClipContents( cm ) ) {
// RAVEN END
			continue;
		}
#ifdef _QUAKE4
		}
#endif

		numEntities++;

		trace.c.contents = cm->GetContents();
		trace.c.entityNum = cm->GetEntity()->entityNumber;
		trace.c.id = cm->GetId();

		ent->Signal( SIG_TOUCH );
		ent->ProcessEvent( &EV_Touch, this, &trace );

		if ( !gameLocal.entities[ entityNumber ] ) {
			gameLocal.Printf( "entity was removed while touching triggers\n" );
			return true;
		}
	}

	return ( numEntities != 0 );
}

/*
================
idEntity::GetSpline
================
*/
idCurve_Spline<idVec3> *idEntity::GetSpline( void ) const {
	int i, numPoints, t;
	const idKeyValue *kv;
	idLexer lex;
	idVec3 v;
	idCurve_Spline<idVec3> *spline;
	const char *curveTag = "curve_";

	kv = spawnArgs.MatchPrefix( curveTag );
	if ( !kv ) {
		return NULL;
	}

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	idStr str = kv->GetKey().Right( kv->GetKey().Length() - strlen( curveTag ) );
	if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
		spline = new idCurve_CatmullRomSpline<idVec3>();
	} else if ( str.Icmp( "nubs" ) == 0 ) {
		spline = new idCurve_NonUniformBSpline<idVec3>();
	} else if ( str.Icmp( "nurbs" ) == 0 ) {
		spline = new idCurve_NURBS<idVec3>();
	} else {
		spline = new idCurve_BSpline<idVec3>();
	}
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END

	spline->SetBoundaryType( idCurve_Spline<idVec3>::BT_CLAMPED );

	lex.LoadMemory( kv->GetValue(), kv->GetValue().Length(), curveTag );
	numPoints = lex.ParseInt();
	lex.ExpectTokenString( "(" );
	for ( t = i = 0; i < numPoints; i++, t += 100 ) {
		v.x = lex.ParseFloat();
		v.y = lex.ParseFloat();
		v.z = lex.ParseFloat();
		spline->AddValue( t, v );
	}
	lex.ExpectTokenString( ")" );

	return spline;
}

/*
===============
idEntity::ShowEditingDialog
===============
*/
void idEntity::ShowEditingDialog( void ) {
}

/***********************************************************************

   Events
	
***********************************************************************/

/*
================
idEntity::Event_GetName
================
*/
void idEntity::Event_GetName( void ) {
	idThread::ReturnString( name.c_str() );
}

/*
================
idEntity::Event_SetName
================
*/
void idEntity::Event_SetName( const char *newname ) {
	SetName( newname );
}

/*
===============
idEntity::Event_FindTargets
===============
*/
void idEntity::Event_FindTargets( void ) {
	FindTargets();
}

/*
============
idEntity::Event_ActivateTargets

Activates any entities targeted by this entity.  Mainly used as an
event to delay activating targets.
============
*/
void idEntity::Event_ActivateTargets( idEntity *activator ) {
	ActivateTargets( activator );
}

// RAVEN BEGIN
// jshepard: added
/*
============
idEntity::Event_UnbindTargets

Unbinds all targets of this entity. Useful to make held or clamped items
drop when shot, and for breakable walls.
============
*/
void idEntity::Event_UnbindTargets( idEntity *activator ) {
	UnbindTargets( activator );
}

// RAVEN END


/*
================
idEntity::Event_NumTargets
================
*/
void idEntity::Event_NumTargets( void ) {
	idThread::ReturnFloat( targets.Num() );
}

/*
================
idEntity::Event_GetTarget
================
*/
void idEntity::Event_GetTarget( float index ) {
	int i;

	i = ( int )index;
	if ( ( i < 0 ) || i >= targets.Num() ) {
		idThread::ReturnEntity( NULL );
	} else {
		idThread::ReturnEntity( targets[ i ].GetEntity() );
	}
}

/*
================
idEntity::Event_RandomTarget
================
*/
void idEntity::Event_RandomTarget( const char *ignore ) {
	int			num;
	idEntity	*ent;
	int			i;
	int			ignoreNum;

	RemoveNullTargets();
	if ( !targets.Num() ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ignoreNum = -1;
	if ( ignore && ( ignore[ 0 ] != 0 ) && ( targets.Num() > 1 ) ) {
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent && ( ent->name == ignore ) ) {
				ignoreNum = i;
				break;
			}
		}
	}

	if ( ignoreNum >= 0 ) {
		num = gameLocal.random.RandomInt( targets.Num() - 1 );
		if ( num >= ignoreNum ) {
			num++;
		}
	} else {
		num = gameLocal.random.RandomInt( targets.Num() );
	}

	ent = targets[ num ].GetEntity();
	idThread::ReturnEntity( ent );
}

// RAVEN BEGIN
// abahr: so we can call this from script
/*
================
idEntity::Event_RemoveNullTargets
================
*/
void idEntity::Event_RemoveNullTargets() {
	RemoveNullTargets();
}

// twhitaker: So targets can be added from script
/*
================
idEntity::Event_AppendTarget
================
*/
void idEntity::Event_AppendTarget( idEntity *appendMe ) {
	idThread::ReturnFloat( AppendTarget( appendMe ) );
}

/*
================
idEntity::Event_RemoveTarget
================
*/
void idEntity::Event_RemoveTarget( idEntity *removeMe ) {
	RemoveTarget( removeMe );
}

/*
================
idEntity::Event_ClearTargetList
================
*/
void idEntity::Event_ClearTargetList( float destroyContents ) {
	RemoveTargets( destroyContents != 0.0f );
}

/*
================
idEntity::Event_MatchPrefix
================
*/
void idEntity::Event_MatchPrefix( const char *prefix, const char* previousKey ) {
	const idKeyValue* kv = (previousKey[0]) ? spawnArgs.FindKey(previousKey) : NULL;

	kv = spawnArgs.MatchPrefix( prefix, kv );
	if( !kv || !kv->GetValue() ) {
		idThread::ReturnString( "" );
		return;
	}

	idThread::ReturnString( kv->GetKey() );
}

/*
================
idEntity::Event_IsA
================
*/
void idEntity::Event_IsA( const char* entityDefName ) {
	const idDict* dict = gameLocal.FindEntityDefDict( entityDefName );
	if( !dict ) {
		idThread::ReturnFloat( false );
		return;
	}

	idTypeInfo* info = idClass::GetClass( dict->GetString("spawnclass") );
	if( !info ) {
		idThread::ReturnFloat( false );
		return;
	}

	idThread::ReturnFloat( IsType(*info) );
}

/*
================
idEntity::Event_IsSameTypeAs
================
*/
void idEntity::Event_IsSameTypeAs( const idEntity* ent ) {
	assert( ent );

	idThread::ReturnFloat( IsType(ent->Type) );
}

// mekberg: allow sethealth on all entities.
// jshepard: removed clamping
/*
================
idEntity::Event_SetHealth
================
*/
void idEntity::Event_SetHealth( float newHealth ) {
	health =  newHealth;
}
// RAVEN END

/*
================
idEntity::Event_BindToJoint
================
*/
void idEntity::Event_BindToJoint( idEntity *master, const char *jointname, float orientated ) {
	BindToJoint( master, jointname, ( orientated != 0.0f ) );
}

/*
================
idEntity::Event_RemoveBinds
================
*/
void idEntity::Event_RemoveBinds( void ) {
	RemoveBinds();
}

/*
================
idEntity::Event_Bind
================
*/
void idEntity::Event_Bind( idEntity *master ) {
	Bind( master, true );
}

/*
================
idEntity::Event_BindPosition
================
*/
void idEntity::Event_BindPosition( idEntity *master ) {
	Bind( master, false );
}

/*
================
idEntity::Event_Unbind
================
*/
void idEntity::Event_Unbind( void ) {
	Unbind();
}

/*
================
idEntity::Event_SpawnBind
================
*/
void idEntity::Event_SpawnBind( void ) {
	idEntity		*parent;
	const char		*bind, *joint, *bindanim;
	jointHandle_t	bindJoint;
	bool			bindOrientated;
	int				id;
	const idAnim	*anim;
	int				animNum;
	idAnimator		*parentAnimator;
	
	if ( spawnArgs.GetString( "bind", "", &bind ) ) {
		if ( idStr::Icmp( bind, "worldspawn" ) == 0 ) {
			//FIXME: Completely unneccessary since the worldspawn is called "world"
			parent = gameLocal.world;
		} else {
			parent = gameLocal.FindEntity( bind );
		}
		bindOrientated = spawnArgs.GetBool( "bindOrientated", "1" );
		if ( parent ) {
			// bind to a joint of the skeletal model of the parent
			if ( spawnArgs.GetString( "bindToJoint", "", &joint ) && *joint ) {
				parentAnimator = parent->GetAnimator();
				if ( !parentAnimator ) {
					gameLocal.Error( "Cannot bind to joint '%s' on '%s'.  Entity does not support skeletal models.", joint, name.c_str() );
				}
				bindJoint = parentAnimator->GetJointHandle( joint );
				if ( bindJoint == INVALID_JOINT ) {
					gameLocal.Error( "Joint '%s' not found for bind on '%s'", joint, name.c_str() );
				}

				// bind it relative to a specific anim
				if ( ( parent->spawnArgs.GetString( "bindanim", "", &bindanim ) || parent->spawnArgs.GetString( "anim", "", &bindanim ) ) && *bindanim ) {
					animNum = parentAnimator->GetAnim( bindanim );
					if ( !animNum ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}
					anim = parentAnimator->GetAnim( animNum );
					if ( !anim ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}

					// make sure parent's render origin has been set
					parent->UpdateModelTransform();

					//FIXME: need a BindToJoint that accepts a joint position
					parentAnimator->CreateFrame( gameLocal.time, true );
					idJointMat *frame = parent->renderEntity.joints;
					gameEdit->ANIM_CreateAnimFrame( parentAnimator->ModelHandle(), anim->MD5Anim( 0 ), parent->renderEntity.numJoints, frame, 0, parentAnimator->ModelDef()->GetVisualOffset(), parentAnimator->RemoveOrigin() );
					BindToJoint( parent, joint, bindOrientated );
					parentAnimator->ForceUpdate();
				} else {
					BindToJoint( parent, joint, bindOrientated );
				}
			}
			// bind to a body of the physics object of the parent
			else if ( spawnArgs.GetInt( "bindToBody", "0", id ) ) {
				BindToBody( parent, id, bindOrientated );
			}
			// bind to the parent
			else {
				Bind( parent, bindOrientated );
			}
		}
	}
}

/*
================
idEntity::Event_SetOwner
================
*/
void idEntity::Event_SetOwner( idEntity *owner ) {
	int i;

	for ( i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		GetPhysics()->GetClipModel( i )->SetOwner( owner );
	}
}

/*
================
idEntity::Event_SetModel
================
*/
void idEntity::Event_SetModel( const char *modelname ) {
	SetModel( modelname );
}

/*
================
idEntity::Event_SetSkin
================
*/
void idEntity::Event_SetSkin( const char *skinname ) {
	renderEntity.customSkin = declManager->FindSkin( skinname );
	UpdateVisuals();
}

/*
================
idEntity::Event_GetShaderParm
================
*/
void idEntity::Event_GetShaderParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( renderEntity.shaderParms[ parmnum ] );
}

/*
================
idEntity::Event_SetShaderParm
================
*/
void idEntity::Event_SetShaderParm( int parmnum, float value ) {
	SetShaderParm( parmnum, value );
}

/*
================
idEntity::Event_SetShaderParms
================
*/
void idEntity::Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	UpdateVisuals();
}


/*
================
idEntity::Event_SetColor
================
*/
void idEntity::Event_SetColor( float red, float green, float blue ) {
	SetColor( red, green, blue );
}

/*
================
idEntity::Event_GetColor
================
*/
void idEntity::Event_GetColor( void ) {
	idVec3 out;

	GetColor( out );
	idThread::ReturnVector( out );
}

/*
================
idEntity::Event_IsHidden
================
*/
void idEntity::Event_IsHidden( void ) {
	idThread::ReturnInt( fl.hidden );
}

/*
================
idEntity::Event_Hide
================
*/
void idEntity::Event_Hide( void ) {
	Hide();
}

/*
================
idEntity::Event_Show
================
*/
void idEntity::Event_Show( void ) {
	Show();
}

/*
================
idEntity::Event_CacheSoundShader
================
*/
void idEntity::Event_CacheSoundShader( const char *soundName ) {
	declManager->FindSound( soundName );
}

/*
================
idEntity::Event_StartSoundShader
================
*/
void idEntity::Event_StartSoundShader( const char *soundName, int channel ) {
	int length;

	StartSoundShader( declManager->FindSound( soundName ), (s_channelType)channel, 0, false, &length );
	idThread::ReturnFloat( MS2SEC( length ) );
}

/*
================
idEntity::Event_StopSound
================
*/
void idEntity::Event_StopSound( int channel, int netSync ) {
	StopSound( channel, ( netSync != 0 ) );
}

/*
================
idEntity::Event_StartSound 
================
*/
void idEntity::Event_StartSound( const char *soundName, int channel, int netSync ) {
	int time;
	
	StartSound( soundName, ( s_channelType )channel, 0, ( netSync != 0 ), &time );
	idThread::ReturnFloat( MS2SEC( time ) );
}

/*
================
idEntity::Event_FadeSound
================
*/
void idEntity::Event_FadeSound( int channel, float to, float over ) {
// RAVEN BEGIN
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->FadeSound( channel, to, over );
	}
// RAVEN END
}

/*
================
idEntity::Event_GetWorldOrigin
================
*/
void idEntity::Event_GetWorldOrigin( void ) {
	idThread::ReturnVector( GetPhysics()->GetOrigin() );
}

/*
================
idEntity::Event_SetWorldOrigin
================
*/
void idEntity::Event_SetWorldOrigin( idVec3 const &org ) {
	idVec3 neworg = GetLocalCoordinates( org );
	SetOrigin( neworg );
}

/*
================
idEntity::Event_SetOrigin
================
*/
void idEntity::Event_SetOrigin( idVec3 const &org ) {
	SetOrigin( org );
}

/*
================
idEntity::Event_GetOrigin
================
*/
void idEntity::Event_GetOrigin( void ) {
	idThread::ReturnVector( GetLocalCoordinates( GetPhysics()->GetOrigin() ) );
}

/*
================
idEntity::Event_SetAngles
================
*/
void idEntity::Event_SetAngles( idAngles const &ang ) {
	SetAngles( ang );
}

/*
================
idEntity::Event_GetAngles
================
*/
void idEntity::Event_GetAngles( void ) {
	idAngles ang = GetPhysics()->GetAxis().ToAngles();
	idThread::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idEntity::Event_SetLinearVelocity
================
*/
void idEntity::Event_SetLinearVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetLinearVelocity( velocity );
}

/*
================
idEntity::Event_GetLinearVelocity
================
*/
void idEntity::Event_GetLinearVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetLinearVelocity() );
}

/*
================
idEntity::Event_SetAngularVelocity
================
*/
void idEntity::Event_SetAngularVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetAngularVelocity( velocity );
}

/*
================
idEntity::Event_GetAngularVelocity
================
*/
void idEntity::Event_GetAngularVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetAngularVelocity() );
}

/*
================
idEntity::Event_SetSize
================
*/
void idEntity::Event_SetSize( idVec3 const &mins, idVec3 const &maxs ) {
	GetPhysics()->SetClipBox( idBounds( mins, maxs ), 1.0f );
}

/*
================
idEntity::Event_GetSize
================
*/
void idEntity::Event_GetSize( void ) {
	idBounds bounds;

	bounds = GetPhysics()->GetBounds();
	idThread::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
idEntity::Event_GetMins
================
*/
void idEntity::Event_GetMins( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[0] );
}

/*
================
idEntity::Event_GetMaxs
================
*/
void idEntity::Event_GetMaxs( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[1] );
}

/*
================
idEntity::Event_Touches
================
*/
void idEntity::Event_Touches( idEntity *ent ) {
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	const idBounds &myBounds = GetPhysics()->GetAbsBounds();
	const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

	idThread::ReturnInt( myBounds.IntersectsBounds( entBounds ) );
}

/*
================
idEntity::Event_SetGuiParm
================
*/
void idEntity::Event_SetGuiParm( const char *key, const char *val ) {
// RAVEN BEGIN
// mekberg: added
	idStr temp = key;
	for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			if ( idStr::Icmpn( key, "gui_", 4 ) ) {
				temp.Insert( "gui_", 0 );	
			}
			spawnArgs.Set( temp.c_str(), val );
// RAVEN END
			
			renderEntity.gui[ i ]->SetStateString( key, val );
			renderEntity.gui[ i ]->StateChanged( gameLocal.time );
		}
	}
}

/*
================
idEntity::Event_SetGuiParm
================
*/
void idEntity::Event_SetGuiFloat( const char *key, float f ) {
	for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			renderEntity.gui[ i ]->SetStateString( key, va( "%f", f ) );
			renderEntity.gui[ i ]->StateChanged( gameLocal.time );
		}
	}
}

/*
================
idEntity::Event_GetNextKey
================
*/
void idEntity::Event_GetNextKey( const char *prefix, const char *lastMatch ) {
	const idKeyValue *kv;
	const idKeyValue *previous;

	if ( *lastMatch ) {
		previous = spawnArgs.FindKey( lastMatch );
	} else {
		previous = NULL;
	}

	kv = spawnArgs.MatchPrefix( prefix, previous );
	if ( !kv ) {
		idThread::ReturnString( "" );
	} else {
		idThread::ReturnString( kv->GetKey() );
	}
}

/*
================
idEntity::Event_SetKey
================
*/
void idEntity::Event_SetKey( const char *key, const char *value ) {
	spawnArgs.Set( key, value );
}

/*
================
idEntity::Event_GetKey
================
*/
void idEntity::Event_GetKey( const char *key ) {
	const char *value;

	spawnArgs.GetString( key, "", &value );
	idThread::ReturnString( value );
}

/*
================
idEntity::Event_GetIntKey
================
*/
void idEntity::Event_GetIntKey( const char *key ) {
	int value;

	spawnArgs.GetInt( key, "0", value );

	// scripts only support floats
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetFloatKey
================
*/
void idEntity::Event_GetFloatKey( const char *key ) {
	float value;

	spawnArgs.GetFloat( key, "0", value );
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetVectorKey
================
*/
void idEntity::Event_GetVectorKey( const char *key ) {
	idVec3 value;

	spawnArgs.GetVector( key, "0 0 0", value );
	idThread::ReturnVector( value );
}

/*
================
idEntity::Event_GetEntityKey
================
*/
void idEntity::Event_GetEntityKey( const char *key ) {
	idEntity *ent;
	const char *entname;

	if ( !spawnArgs.GetString( key, NULL, &entname ) ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key in entity '%s'", entname, key, name.c_str() );
	}

	idThread::ReturnEntity( ent );
}

/*
================
idEntity::Event_RestorePosition
================
*/
void idEntity::Event_RestorePosition( void ) {
	idVec3		org;
	idAngles	angles;
	idMat3		axis;
	idEntity *	part;

	spawnArgs.GetVector( "origin", "0 0 0", org );

	// get the rotation matrix in either full form, or single angle form
	if ( spawnArgs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) ) {
		angles = axis.ToAngles();
	} else {
   		angles[ 0 ] = 0;
   		angles[ 1 ] = spawnArgs.GetFloat( "angle" );
   		angles[ 2 ] = 0;
	}

	Teleport( org, angles, NULL );

	for ( part = teamChain; part != NULL; part = part->teamChain ) {
		if ( part->bindMaster != this ) {
			continue;
		}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( part->GetPhysics()->IsType( idPhysics_Parametric::GetClassType() ) ) {
			if ( static_cast<idPhysics_Parametric *>(part->GetPhysics())->IsPusher() ) {
				gameLocal.Warning( "teleported '%s' which has the pushing mover '%s' bound to it\n", GetName(), part->GetName() );
			}
		} else if ( part->GetPhysics()->IsType( idPhysics_AF::GetClassType() ) ) {
// RAVEN END
			gameLocal.Warning( "teleported '%s' which has the articulated figure '%s' bound to it\n", GetName(), part->GetName() );
		}
	}
}

/*
================
idEntity::Event_UpdateCameraTarget
================
*/
void idEntity::Event_UpdateCameraTarget( void ) {
	const char *target;
	const idKeyValue *kv;
	idVec3 dir;

	target = spawnArgs.GetString( "cameraTarget" );

	cameraTarget = gameLocal.FindEntity( target );

	if ( cameraTarget ) {
		kv = cameraTarget->spawnArgs.MatchPrefix( "target", NULL );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent && idStr::Icmp( ent->GetEntityDefName(), "target_null" ) == 0) {
				dir = ent->GetPhysics()->GetOrigin() - cameraTarget->GetPhysics()->GetOrigin();
				dir.Normalize();
				cameraTarget->SetAxis( dir.ToMat3() );
// RAVEN BEGIN
// rjohnson: if you have a func_cameraview pointing to an info_null via "cameratarget" and
//			 you have a func_static pointing to the func_cameraview via a "cameratarget" then
//			 the func_static evaluates 'target' to the func_cameraview and its target it the info null
//			 the SexAxis() is then applied to the the func_static rather than the func_cameraview
//				SetAxis(dir.ToMat3());
// RAVEN END
				break;						
			}
			kv = cameraTarget->spawnArgs.MatchPrefix( "target", kv );
		}
	}
	UpdateVisuals();
}

/*
================
idEntity::Event_DistanceTo
================
*/
void idEntity::Event_DistanceTo( idEntity *ent ) {
	if ( !ent ) {
		// just say it's really far away
		idThread::ReturnFloat( MAX_WORLD_SIZE );
	} else {
		float dist = ( GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin() ).LengthFast();
		idThread::ReturnFloat( dist );
	}
}

/*
================
idEntity::Event_DistanceToPoint
================
*/
void idEntity::Event_DistanceToPoint( const idVec3 &point ) {
	float dist = ( GetPhysics()->GetOrigin() - point ).LengthFast();
	idThread::ReturnFloat( dist );
}

/*
================
idEntity::Event_StartFx
================
*/
void idEntity::Event_StartFx( const char *fx ) {
// RAVEN BEGIN
// bdube: not used
//	idEntityFx::StartFx( fx, NULL, NULL, this, true );
// RAVEN END
}

/*
================
idEntity::Event_WaitFrame
================
*/
void idEntity::Event_WaitFrame( void ) {
	idThread *thread;
	
	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->WaitFrame();
	}
}

/*
=====================
idEntity::Event_Wait
=====================
*/
void idEntity::Event_Wait( float time ) {
	idThread *thread = idThread::CurrentThread();

	if ( !thread ) {
		gameLocal.Error( "Event 'wait' called from outside thread" );
	}

	thread->WaitSec( time );
}

/*
=====================
idEntity::Event_HasFunction
=====================
*/
void idEntity::Event_HasFunction( const char *name ) {
	const function_t *func;

	func = scriptObject.GetFunction( name );
	if ( func ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idEntity::Event_CallFunction
=====================
*/
void idEntity::Event_CallFunction( const char *funcname ) {
// RAVEN BEGIN
// bdube: states
	stateParms_t parms = {0};
	if ( ProcessState ( funcname, parms ) != SRESULT_ERROR ) {
		return;
	}
	gameLocal.CallObjectFrameCommand ( this, funcname );
// RAVEN END
}

/*
================
idEntity::Event_SetNeverDormant
================
*/
void idEntity::Event_SetNeverDormant( int enable ) {
	fl.neverDormant	= ( enable != 0 );
	dormantStart = 0;
}

/***********************************************************************

   Network
	
***********************************************************************/

/*
================
idEntity::ClientPredictionThink
================
*/
void idEntity::ClientPredictionThink( void ) {
	RunPhysics();
	Present();
}

/*
================
idEntity::WriteBindToSnapshot
================
*/
void idEntity::WriteBindToSnapshot( idBitMsgDelta &msg ) const {
	int bindInfo;

	if ( bindMaster ) {
		bindInfo = bindMaster->entityNumber;
		bindInfo |= ( fl.bindOrientated & 1 ) << GENTITYNUM_BITS;
		if ( bindJoint != INVALID_JOINT ) {
			bindInfo |= 1 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindJoint << ( 3 + GENTITYNUM_BITS );
		} else if ( bindBody != -1 ) {
			bindInfo |= 2 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindBody << ( 3 + GENTITYNUM_BITS );
		}
	} else {
		bindInfo = ENTITYNUM_NONE;
	}
	msg.WriteBits( bindInfo, GENTITYNUM_BITS + 3 + 9 );
}

/*
================
idEntity::ReadBindFromSnapshot
================
*/
void idEntity::ReadBindFromSnapshot( const idBitMsgDelta &msg ) {
	int bindInfo, bindEntityNum, bindPos;
	bool bindOrientated;
	idEntity *master;

	bindInfo = msg.ReadBits( GENTITYNUM_BITS + 3 + 9 );
	bindEntityNum = bindInfo & ( ( 1 << GENTITYNUM_BITS ) - 1 );

	if ( bindEntityNum != ENTITYNUM_NONE ) {
		master = gameLocal.entities[ bindEntityNum ];

		bindOrientated = ( bindInfo >> GENTITYNUM_BITS ) & 1;
		bindPos = ( bindInfo >> ( GENTITYNUM_BITS + 3 ) );
		switch( ( bindInfo >> ( GENTITYNUM_BITS + 1 ) ) & 3 ) {
			case 1: {
				BindToJoint( master, (jointHandle_t) bindPos, bindOrientated );
				break;
			}
			case 2: {
				BindToBody( master, bindPos, bindOrientated );
				break;
			}
			default: {
				Bind( master, bindOrientated );
				break;
			}
		}
	} else if ( bindMaster ) {
		Unbind();
	}
}

/*
================
idEntity::WriteColorToSnapshot
================
*/
void idEntity::WriteColorToSnapshot( idBitMsgDelta &msg ) const {
	idVec4 color;

	color[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	color[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	color[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	color[3] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
	msg.WriteLong( PackColor( color ) );
}

/*
================
idEntity::ReadColorFromSnapshot
================
*/
void idEntity::ReadColorFromSnapshot( const idBitMsgDelta &msg ) {
	idVec4 color;

	UnpackColor( msg.ReadLong(), color );
	renderEntity.shaderParms[ SHADERPARM_RED ] = color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = color[3];
}

/*
================
idEntity::WriteGUIToSnapshot
================
*/
void idEntity::WriteGUIToSnapshot( idBitMsgDelta &msg ) const {
	// no need to loop over MAX_RENDERENTITY_GUI at this time
	if ( renderEntity.gui[ 0 ] ) {
		msg.WriteByte( renderEntity.gui[ 0 ]->State().GetInt( "networkState" ) );
	} else {
		msg.WriteByte( 0 );
	}
}

/*
================
idEntity::ReadGUIFromSnapshot
================
*/
void idEntity::ReadGUIFromSnapshot( const idBitMsgDelta &msg ) {
	int state;
	idUserInterface *gui;
	state = msg.ReadByte( );
	gui = renderEntity.gui[ 0 ];
	if ( gui && state != mpGUIState ) {
		mpGUIState = state;
		gui->SetStateInt( "networkState", state );
		gui->HandleNamedEvent( "networkState" );
	}
}

/*
================
idEntity::WriteToSnapshot
================
*/
void idEntity::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
idEntity::ReadFromSnapshot
================
*/
void idEntity::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
idEntity::ServerSendEvent

   Saved events are also sent to any client that connects late so all clients
   always receive the events nomatter what time they join the game.
   ================
   */
void idEntity::ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isServer ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );	
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	networkSystem->ServerSendReliableMessageExcluding( excludeClient, outMsg );

	if ( saveEvent ) {
		gameLocal.Error( "Unsupported saveEvent == true in idEntity::ServerSendEvent" );
	}
}

/*
================
idEntity::ServerSendInstanceEvent
================
*/
void idEntity::ServerSendInstanceEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isServer ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );	
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	gameLocal.ServerSendInstanceReliableMessageExcluding( this, excludeClient, outMsg );

	if ( saveEvent ) {
		gameLocal.Error( "Unsupported saveEvent == true in idEntity::ServerSendEvent" );
	}
}

/*
================
idEntity::ClientSendEvent
================
*/
void idEntity::ClientSendEvent( int eventId, const idBitMsg *msg ) const {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isClient ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, idMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
idEntity::ServerReceiveEvent
================
*/
bool idEntity::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch( event ) {
		case 0: {
		}
		default: {
			return false;
		}
	}
}

/*
================
idEntity::ClientReceiveEvent
================
*/
bool idEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	const idSoundShader	*shader;
	s_channelType		channel;

	switch( event ) {
		case EVENT_STARTSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			if ( time < gameLocal.realClientTime - 300 ) {
				// too old, skip it
				common->DPrintf( "ent 0x%x: start sound shader too old (%d ms)\n", entityNumber, gameLocal.realClientTime - time );
				return true;
			}
			shader = static_cast< const idSoundShader* >( idGameLocal::ReadDecl( msg, DECL_SOUND ) );
			channel = (s_channelType)msg.ReadByte();
			StartSoundShader( shader, channel, 0, false, NULL );
			return true;
		}
		case EVENT_STOPSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			channel = (s_channelType)msg.ReadByte();
			StopSound( channel, false );
			return true;
		}
// RAVEN BEGIN
// bdube: new events
		case EVENT_PLAYEFFECT_JOINT: {
			const idDecl*		effect;
			idCQuat				quat;
			idVec3				origin;
			rvClientEffect*		clientEffect;
			effectCategory_t	category;
			jointHandle_t		jointHandle;
			bool				loop;

			// TMP - not quite sure this is still used for anything
			common->Warning( "FIXME: idEntity::PlayEffect happens" );
			assert( false );
		
			effect = idGameLocal::ReadDecl( msg, DECL_EFFECT );
			jointHandle = ( jointHandle_t )msg.ReadLong();
			loop = ( msg.ReadBits ( 1 ) != 0 );
			origin.x = msg.ReadFloat( );
			origin.y = msg.ReadFloat( );
			origin.z = msg.ReadFloat( );
			category = ( effectCategory_t )msg.ReadByte();

			if ( bse->CanPlayRateLimited( category ) ) {
			// mwhitlock: Dynamic memory consolidation
				RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
				clientEffect = new rvClientEffect( effect );
				RV_POP_HEAP();

				clientEffect->SetOrigin ( vec3_origin );
				clientEffect->SetAxis ( mat3_identity );
				clientEffect->Bind( this, jointHandle );

				clientEffect->Play( time, loop, origin );
			}
			return true;
		}
		
		case EVENT_PLAYEFFECT: {
			const idDecl*		effect;
			idCQuat				quat;
			idVec3				origin, origin2;
			rvClientEffect*		clientEffect;
			effectCategory_t	category;
			bool				loop;
			
			effect = idGameLocal::ReadDecl( msg, DECL_EFFECT );

			origin.x = msg.ReadFloat( );
			origin.y = msg.ReadFloat( );
			origin.z = msg.ReadFloat( );
			
			quat.x = msg.ReadFloat( );
			quat.y = msg.ReadFloat( );
			quat.z = msg.ReadFloat( );

			loop = ( msg.ReadBits( 1 ) != 0 );

			origin2.x = msg.ReadFloat( );
			origin2.y = msg.ReadFloat( );
			origin2.z = msg.ReadFloat( );
			category = ( effectCategory_t )msg.ReadByte();

			if ( bse->CanPlayRateLimited( category ) ) {
				// mwhitlock: Dynamic memory consolidation
				RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
				clientEffect = new rvClientEffect( effect );
				RV_POP_HEAP();

				clientEffect->SetOrigin ( origin );
				clientEffect->SetAxis ( quat.ToMat3() );
				clientEffect->Bind ( this );

				clientEffect->Play ( time, loop, origin2 );
			}
			return true;
		}
// RAVEN END		
		default: {
			return false;
		}
	}
//unreachable
//	return false;
}

// RAVEN BEGIN
// bdube: added
/*
================
idEntity::ClientStale
================
*/
bool idEntity::ClientStale( void ) {
	FreeModelDef();
	UpdateVisuals();
	GetPhysics()->UnlinkClip();
	return false;
}

/*
================
idEntity::ClientUnstale
================
*/
void idEntity::ClientUnstale( void ) {
}

/*
================
idEntity::GetDamageEntity

Returns the entity that should take damage in place of this entity.  The default is the
entity itself.
================
*/
idEntity* idEntity::GetDamageEntity( void ) { 
	return forwardDamageEnt.IsValid() ? forwardDamageEnt.GetEntity() : this;
}

// rjohnson: moved entity info out of idGameLocal into its own function
/*
================
idEntity::DrawDebugEntityInfo
================
*/
void idEntity::DrawDebugEntityInfo( idBounds *viewBounds, idBounds *viewTextBounds, idVec4 *overrideColor ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	idMat3 axis = player->viewAngles.ToMat3();
	idVec3 up = axis[ 2 ] * 5.0f;

	// skip if the entity is very far away
	if ( viewBounds && !viewBounds->IntersectsBounds( GetPhysics()->GetAbsBounds() ) ) {
		return;
	}

	const idBounds &entBounds = GetPhysics()->GetAbsBounds();

	if (overrideColor) {
		if ( !entBounds.GetVolume() ) {
			gameRenderWorld->DebugBounds( *overrideColor, entBounds.Expand( 8.0f ), vec3_origin );
		} else {
			gameRenderWorld->DebugBounds( *overrideColor, entBounds, vec3_origin );
		}
	} else {
		int contents = GetPhysics()->GetContents();
		if ( contents & CONTENTS_BODY ) {
			gameRenderWorld->DebugBounds ( colorCyan, entBounds, vec3_origin );
		} else if ( contents & CONTENTS_TRIGGER ) {
			gameRenderWorld->DebugBounds( colorOrange, entBounds, vec3_origin );
		} else if ( contents & CONTENTS_SOLID ) {
			gameRenderWorld->DebugBounds( colorGreen, entBounds, vec3_origin );
		} else {
			if ( !entBounds.GetVolume() ) {
				gameRenderWorld->DebugBounds( colorMdGrey, entBounds.Expand( 8.0f ), vec3_origin );
			} else {
				gameRenderWorld->DebugBounds( colorMdGrey, entBounds, vec3_origin );
			}
		}
	}

	if ( !viewTextBounds || viewTextBounds->IntersectsBounds( entBounds ) ) {
		gameRenderWorld->DrawText( name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
		gameRenderWorld->DrawText( va( "#%d", entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );

#ifdef __ANDROID__
		gameLocal.Printf( "[Entity]: %s#%d\n", name.c_str(), entityNumber );
#endif

		if ( gameLocal.GetLocalPlayer() && this != gameLocal.GetLocalPlayer() && teamMaster != gameLocal.GetLocalPlayer() ) {
			gameRenderWorld->DebugLine ( colorRed, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[0] );
			gameRenderWorld->DebugLine ( colorGreen, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[1] );
			gameRenderWorld->DebugLine ( colorBlue, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[2] );
		}
	}
}

/*
=====================
idEntity::SetInstance
=====================
*/
void idEntity::SetInstance( int newInstance ) {
	instance = newInstance;

	if( gameLocal.isServer ) {
		SetClipWorld( newInstance );
	}
}

/*
=====================
idEntity::InstanceJoin
Gets called when the local player joins the same instance as this entity
=====================
*/
void idEntity::InstanceJoin( void ) {
	assert( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() == instance );

	BecomeActive( TH_UPDATEVISUALS );
	Present();
}

/*
=====================
idEntity::InstanceLeave
Gets called when the local player leaves the same instance as this entity
=====================
*/
void idEntity::InstanceLeave( void ) {
	assert( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance );

	FreeLightDef();
	FreeModelDef();
	//RemoveClientEntities();
	BecomeInactive( TH_UPDATEVISUALS );
}

/*
=====================
idEntity::GetDebugInfo
=====================
*/
void idEntity::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idClass::GetDebugInfo ( proc, userData );

	proc ( "idEntity", "health",		va("%d",health), userData );
	proc ( "idEntity", "name",			name, userData );
	proc ( "idEntity", "entityNumber",  va("%d",entityNumber), userData );
	proc ( "idEntity", "origin",		renderEntity.origin.ToString ( ), userData );

	proc ( "idEntity", "notarget",		fl.notarget?"true":"false", userData );
	proc ( "idEntity", "takedamage",	fl.takedamage?"true":"false", userData );
	proc ( "idEntity", "hidden",		fl.hidden?"true":"false", userData );
	proc ( "idEntity", "bindOrientated",fl.bindOrientated?"true":"false", userData );
	proc ( "idEntity", "isDormant",		fl.isDormant?"true":"false", userData );
	proc ( "idEntity", "neverDormant",	fl.neverDormant?"true":"false", userData );
	proc ( "idEntity", "isAIObstacle",	fl.isAIObstacle?"true":"false", userData );

	proc ( "idEntity", "forwardDamageEnt",forwardDamageEnt.GetEntity() ? forwardDamageEnt.GetEntity()->GetName() : "<none>", userData );

	proc ( "idEntity", "bindMaster",	bindMaster ? bindMaster->GetName() : "<none>", userData );
	proc ( "idEntity", "bindJoint",		va("%d",((int)bindJoint)), userData );
	proc ( "idEntity", "bindBody",		va("%d",bindBody), userData );

	proc ( "idEntity", "teamMaster",	teamMaster ? teamMaster->GetName() : "<none>", userData );
	proc ( "idEntity", "teamChain",		teamChain ? teamChain->GetName() : "<none>", userData );
}

// mwhitlock: memory profiling
/*
=====================
idEntity::Size()

Returns memory size of an idEntity instance
=====================
*/

size_t idEntity::Size( void ) const
{
	// TODO: more crap needs to go here!
	return sizeof (idEntity);
}
// RAVEN END



/*
===============================================================================

  idAnimatedEntity

===============================================================================
*/

const idEventDef EV_GetJointHandle( "getJointHandle", "s", 'd' );
const idEventDef EV_ClearAllJoints( "clearAllJoints" );
const idEventDef EV_ClearJoint( "clearJoint", "d" );
const idEventDef EV_SetJointPos( "setJointPos", "ddv" );
const idEventDef EV_SetJointAngle( "setJointAngle", "ddv" );
const idEventDef EV_GetJointPos( "getJointPos", "d", 'v' );
const idEventDef EV_GetJointAngle( "getJointAngle", "d", 'v' );


// RAVEN BEGIN
// bdube: programmer controlled joint events
const idEventDef EV_SetJointAngularVelocity ( "setJointAngularVelocity", "sfffd" );
const idEventDef EV_CollapseJoints ( "collapseJoints", "ss" );
// jshepard: clear out all animations still running on the model
const idEventDef EV_ClearAnims( "clearAnims" );

// RAVEN END

CLASS_DECLARATION( idEntity, idAnimatedEntity )
	EVENT( EV_GetJointHandle,		idAnimatedEntity::Event_GetJointHandle )
	EVENT( EV_ClearAllJoints,		idAnimatedEntity::Event_ClearAllJoints )
	EVENT( EV_ClearJoint,			idAnimatedEntity::Event_ClearJoint )
	EVENT( EV_SetJointPos,			idAnimatedEntity::Event_SetJointPos )
	EVENT( EV_SetJointAngle,		idAnimatedEntity::Event_SetJointAngle )
	EVENT( EV_GetJointPos,			idAnimatedEntity::Event_GetJointPos )
	EVENT( EV_GetJointAngle,		idAnimatedEntity::Event_GetJointAngle )

// RAVEEN BEGIN
// bdube: programmer controlled joint events
	EVENT( EV_SetJointAngularVelocity,	idAnimatedEntity::Event_SetJointAngularVelocity )
	EVENT( EV_CollapseJoints,			idAnimatedEntity::Event_CollapseJoints )
// RAVEN END
END_CLASS

/*
================
idAnimatedEntity::idAnimatedEntity
================
*/
idAnimatedEntity::idAnimatedEntity() {
	animator.SetEntity( this );
	damageEffects = NULL;
}

/*
================
idAnimatedEntity::~idAnimatedEntity
================
*/
idAnimatedEntity::~idAnimatedEntity() {
	damageEffect_t	*de;

	for ( de = damageEffects; de; de = damageEffects ) {
		damageEffects = de->next;
		delete de;
	}
}

/*
================
idAnimatedEntity::Save

archives object for save game file
================
*/
void idAnimatedEntity::Save( idSaveGame *savefile ) const {
	animator.Save( savefile );

	// Wounds are very temporary, ignored at this time
	//damageEffect_t			*damageEffects;
}

/*
================
idAnimatedEntity::Restore

unarchives object from save game file
================
*/
void idAnimatedEntity::Restore( idRestoreGame *savefile ) {
	animator.Restore( savefile );

	// check if the entity has an MD5 model
	if ( animator.ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if ( modelDefHandle != -1 ) {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
}

/*
================
idAnimatedEntity::ClientPredictionThink
================
*/
void idAnimatedEntity::ClientPredictionThink( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::Think
================
*/
void idAnimatedEntity::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::UpdateAnimation
================
*/
void idAnimatedEntity::UpdateAnimation( void ) {
	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

// RAVEN BEGIN
// bgeisler: for triggered anims
	// call any frame commands that have happened in the past frame
	if ( !fl.hidden || fl.triggerAnim )  {
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}
// RAVEN END

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if ( renderEntity.bounds.IsCleared() && !fl.hidden ) {
		gameLocal.DPrintf( "idAnimatedEntity %s %d: inside out bounds - %d\n", GetName(), entityNumber, gameLocal.time );
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
idAnimatedEntity::GetAnimator
================
*/
idAnimator *idAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
================
idAnimatedEntity::SetModel
================
*/
void idAnimatedEntity::SetModel( const char *modelname ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		idEntity::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = idEntity::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateVisuals();
}

/*
=====================
idAnimatedEntity::GetJointWorldTransform
=====================
*/
bool idAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = GetPhysics()->GetCenterMass();
		axis = renderEntity.axis;
		return true;
	}

	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		return false;
	}

	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset, idMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = GetPhysics()->GetCenterMass() - GetPhysics()->GetOrigin();
		axis = renderEntity.axis;
		return true;
	}

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( 0 );
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( 0 );
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();
	
	return true;
}

// RAVEN BEGIN
// ddynerman: removed/merged AddLocalDamageEffect() (redundant math)
/*
==============
idAnimatedEntity::AddDamageEffect

  Dammage effects track the animating impact position, spitting out particles.
==============
*/
void idAnimatedEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	// ddynerman: note, on client the collision struct is incomplete.  Only contains impact point and material
	const char *splat, *decal, *key;
	idVec3 dir;

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == NULL || !def->dict.GetBool ( "bleed" ) ) {
		return;
	}

	if ( !spawnArgs.GetBool( "bleed" ) ) {
		return;
	}

	dir = velocity;
	dir.Normalize();

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( collision.c.point[0] );
		msg.WriteFloat( collision.c.point[1] );
		msg.WriteFloat( collision.c.point[2] );
		msg.WriteDir( dir, 24 );
		idGameLocal::WriteDecl( msg, def );
		idGameLocal::WriteDecl( msg, collision.c.material );
		ServerSendInstanceEvent( EVENT_ADD_DAMAGE_EFFECT, &msg, false, -1 );
	}

	if ( !g_decals.GetBool() ) {
		return;
	}

	if ( gameLocal.GetLocalPlayer() && GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
		return; // no blood from other instances
	}

	// blood splats are thrown onto nearby surfaces
	splat = NULL;
	if ( collision.c.material->GetMaterialType() ) {
		key = va( "mtr_splat_%s", collision.c.material->GetMaterialType()->GetName() );
		splat = spawnArgs.RandomPrefix( key, gameLocal.random );
	}
	if ( !splat || !*splat ) {
		splat = spawnArgs.RandomPrefix( "mtr_splat", gameLocal.random );
	}
	if ( splat && *splat ) {
		//jshepard original 64.0f
		// dluetscher: changed from 64. to 48. for performance reasons
		gameLocal.BloodSplat( this, collision.c.point, dir, 48.0f, splat );
	}

	// can't see wounds on the player model in single player mode
	if ( !( IsType( idPlayer::GetClassType() ) && !gameLocal.isMultiplayer ) ) {
		//If this is a buddy marine, no wound decals until they're actually dead unless it's mp.
		if ( gameLocal.isMultiplayer 
			|| !IsType( idAI::GetClassType() ) 
			|| this->health <= 0
			|| ((idAI*)this)->team != AITEAM_MARINE ) {
			// place a wound overlay on the model
			decal = NULL;
			if ( collision.c.material->GetMaterialType() ) {
				key = va( "mtr_wound_%s", collision.c.material->GetMaterialType()->GetName() );
				decal = spawnArgs.RandomPrefix( key, gameLocal.random );
			}
			if ( !decal || !*decal ) {
				decal = spawnArgs.RandomPrefix( "mtr_wound", gameLocal.random );
			}
			if ( decal && *decal ) {
				ProjectOverlay( collision.c.point, dir, 20.0f, decal );
				if( IsType( idPlayer::GetClassType() ) ) {
					ProjectHeadOverlay( collision.c.point, dir, 20.0f, decal );
				}
			}
		}
	}
}
// RAVEN END

/*
==============
idAnimatedEntity::GetDefaultSurfaceType
==============
*/
int	idAnimatedEntity::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_METAL;
}

/*
================
idAnimatedEntity::ClientReceiveEvent
================
*/
bool idAnimatedEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	idVec3 origin, dir;

	switch( event ) {
		case EVENT_ADD_DAMAGE_EFFECT: {
			origin[0] = msg.ReadFloat();
			origin[1] = msg.ReadFloat();
			origin[2] = msg.ReadFloat();
			dir = msg.ReadDir( 24 );
			const idDeclEntityDef *damageDef = static_cast< const idDeclEntityDef* >( idGameLocal::ReadDecl( msg, DECL_ENTITYDEF ) );
			const idMaterial *collisionMaterial = static_cast< const idMaterial* >( idGameLocal::ReadDecl( msg, DECL_MATERIAL ) );
// RAVEN BEGIN
// ddynerman: removed redundant AddLocalDamageEffect()
			trace_t collision;
			collision.c.point = origin;
			collision.c.material = collisionMaterial;
			AddDamageEffect( collision, dir, damageDef->GetName(), NULL );
// RAVEN END
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
// abahr: so we don't crash if UpdateModel is called from a destructor
/*
================
idAnimatedEntity::UpdateRenderEntityCallback
================
*/
void idAnimatedEntity::UpdateRenderEntityCallback() {
	// check if the entity has an MD5 model
	idAnimator *animator = GetAnimator();
	if ( animator && animator->ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
	}
}
// RAVEN END

/*
================
idAnimatedEntity::Event_GetJointHandle

looks up the number of the specified joint.  returns INVALID_JOINT if the joint is not found.
================
*/
void idAnimatedEntity::Event_GetJointHandle( const char *jointname ) {
	jointHandle_t joint;

	joint = animator.GetJointHandle( jointname );
	idThread::ReturnInt( joint );
}

/*
================
idAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void idAnimatedEntity::Event_ClearAllJoints( void ) {
	animator.ClearAllJoints();
}

/*
================
idAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void idAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum ) {
	animator.ClearJoint( jointnum );
}

/*
================
idAnimatedEntity::Event_ClearAnims

Clears any animation running on the animated entity
================
*/
void idAnimatedEntity::Event_ClearAnims( void ) {
	animator.Clear( ANIMCHANNEL_ALL, gameLocal.GetTime(), gameLocal.GetTime() ); 
}

/*
================
idAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
idAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles ) {
	idMat3 mat;

	mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idThread::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idAngles ang = axis.ToAngles();
	idVec3 vec( ang[ 0 ], ang[ 1 ], ang[ 2 ] );
	idThread::ReturnVector( vec );
}

// RAVEN BEGIN
// bdube: moved to idAnimatedEntity
/*
================
idAnimatedEntity::Event_SetJointAngularVelocity
================
*/
void idAnimatedEntity::Event_SetJointAngularVelocity ( const char* jointName, float pitch, float yaw, float roll, int blendTime ) {
	jointHandle_t joint = animator.GetJointHandle ( jointName );
	if ( joint == INVALID_JOINT ) {
		return;
	}
	
	animator.SetJointAngularVelocity ( joint, idAngles(pitch,yaw,roll), gameLocal.time, blendTime );
}

/*
================
idAnimatedEntity::Event_CollapseJoints
================
*/
void idAnimatedEntity::Event_CollapseJoints ( const char* jointnames, const char* collapseTo ) {
	jointHandle_t collapseToJoint = animator.GetJointHandle ( collapseTo );
	if ( collapseToJoint == INVALID_JOINT ) {
		return;
	}

	animator.CollapseJoints ( jointnames, collapseToJoint );	
}
// RAVEN END

#ifdef _QUAKE4
// jmarshall
/*
==============
idEntity::GetKey
==============
*/
const char* idEntity::GetKey(const char* key)
{
	const char* value;

	spawnArgs.GetString(key, "", &value);

	return value;
}

/*
==============
idEntity::GetFloat
==============
*/

float idEntity::GetFloat(const char* key)
{
	return spawnArgs.GetFloat(key, "0");
}

/*
==============
idEntity::GetInt
==============
*/
int idEntity::GetInt(const char* key)
{
	return spawnArgs.GetInt(key, "0");
}

/*
==============
idEntity::GetBool
==============
*/
bool idEntity::GetBool(const char* key)
{
	return spawnArgs.GetBool(key, "0");
}

// jmarshall
/*
================
idEntity::GetOrigin
================
*/
idVec3 idEntity::GetOrigin(void)
{
	return GetLocalCoordinates(GetPhysics()->GetOrigin());
}
// jmarshall end
#endif

