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

/*

Invisible entities that affect other entities or the world when activated.

*/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "Objectives/MissionData.h"
#include "Missions/MissionManager.h"
#include "ai/Conversation/ConversationSystem.h"
#include "StimResponse/StimResponseCollection.h"
#include "Inventory/Inventory.h"  // SteveL #3784
#include "Inventory/WeaponItem.h" // SteveL #3784
#include "StdString.h"

/*
===============================================================================

idTarget

===============================================================================
*/

CLASS_DECLARATION( idEntity, idTarget )
END_CLASS


/*
===============================================================================

idTarget_Remove

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_Remove )
	EVENT( EV_Activate, idTarget_Remove::Event_Activate )
END_CLASS

/*
================
idTarget_Remove::Event_Activate
================
*/
void idTarget_Remove::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity	*ent;

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}

	// delete our self when done
	PostEventMS( &EV_Remove, 0 );
}


/*
===============================================================================

idTarget_Show

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_Show )
	EVENT( EV_Activate, idTarget_Show::Event_Activate )
END_CLASS

/*
================
idTarget_Show::Event_Activate
================
*/
void idTarget_Show::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity	*ent;

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->Show();
		}
	}

	// delete our self when done
	PostEventMS( &EV_Remove, 0 );
}


/*
===============================================================================

idTarget_Damage

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_Damage )
	EVENT( EV_Activate, idTarget_Damage::Event_Activate )
END_CLASS

/*
================
idTarget_Damage::Event_Activate
================
*/
void idTarget_Damage::Event_Activate( idEntity *activator ) {
	int			i;
	const char *damage;
	idEntity *	ent;

	damage = spawnArgs.GetString( "def_damage", "damage_generic" );
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->Damage( this, this, vec3_origin, damage, 1.0f, INVALID_JOINT );
		}
	}
}


/*
===============================================================================

idTarget_SessionCommand

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SessionCommand )
	EVENT( EV_Activate, idTarget_SessionCommand::Event_Activate )
END_CLASS

/*
================
idTarget_SessionCommand::Event_Activate
================
*/
void idTarget_SessionCommand::Event_Activate( idEntity *activator ) {
	gameLocal.sessionCommand = spawnArgs.GetString( "command" );
}


/*
===============================================================================

idTarget_EndLevel

Just a modified form of idTarget_SessionCommand
===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_EndLevel )
	EVENT( EV_Activate,		idTarget_EndLevel::Event_Activate )
END_CLASS

/*
================
idTarget_EndLevel::Event_Activate
================
*/
void idTarget_EndLevel::Event_Activate( idEntity *activator ) {
	idStr nextMap;

	if ( spawnArgs.GetBool( "endOfGame" ) ) {
		gameLocal.sessionCommand = "disconnect";
		return;
	}

	if ( !spawnArgs.GetString( "nextMap", "", nextMap ) ) {
		gameLocal.Printf( "idTarget_SessionCommand::Event_Activate: no nextMap key\n" );
		return;
	}

	if ( spawnArgs.GetInt( "devmap", "0" ) ) {
		gameLocal.sessionCommand = "devmap ";	// only for special demos
	} else {
		gameLocal.sessionCommand = "map ";
	}

	gameLocal.sessionCommand += nextMap;
}


/*
===============================================================================

idTarget_WaitForButton

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_WaitForButton )
	EVENT( EV_Activate, idTarget_WaitForButton::Event_Activate )
END_CLASS

/*
================
idTarget_WaitForButton::Event_Activate
================
*/
void idTarget_WaitForButton::Event_Activate( idEntity *activator ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
	} else {
		// always allow during cinematics
		cinematic = true;
		BecomeActive( TH_THINK );
	}
}

/*
================
idTarget_WaitForButton::Think
================
*/
void idTarget_WaitForButton::Think( void ) {
	idPlayer *player;

	if ( thinkFlags & TH_THINK ) {
		player = gameLocal.GetLocalPlayer();
		if ( player && ( ! ( player->oldButtons & BUTTON_ATTACK ) ) && ( player->usercmd.buttons & BUTTON_ATTACK ) ) {
			player->usercmd.buttons &= ~BUTTON_ATTACK;
			BecomeInactive( TH_THINK );
			ActivateTargets( player );
		}
	} else {
		BecomeInactive( TH_ALL );
	}
}


/*
===============================================================================

idTarget_SetGlobalShaderParm

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetGlobalShaderTime )
EVENT( EV_Activate,	idTarget_SetGlobalShaderTime::Event_Activate )
END_CLASS

/*
================
idTarget_SetGlobalShaderTime::Event_Activate
================
*/
void idTarget_SetGlobalShaderTime::Event_Activate( idEntity *activator ) {
	int parm = spawnArgs.GetInt( "globalParm" );
	float time = -MS2SEC( gameLocal.time );
	if ( parm >= 0 && parm < MAX_GLOBAL_SHADER_PARMS ) {
		gameLocal.globalShaderParms[parm] = time;
	}
}

/*
===============================================================================

idTarget_SetShaderParm

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetShaderParm )
	EVENT( EV_Activate,	idTarget_SetShaderParm::Event_Activate )
END_CLASS

/*
================
idTarget_SetShaderParm::Event_Activate
================
*/
void idTarget_SetShaderParm::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity *	ent;
	float		value;
	idVec3		color;
	int			parmnum;

	// set the color on the targets
	if ( spawnArgs.GetVector( "_color", "1 1 1", color ) ) {
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent ) {
				ent->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
			}
		}
	}

	// set any shader parms on the targets
	for( parmnum = 0; parmnum < MAX_ENTITY_SHADER_PARMS; parmnum++ ) {
		if ( spawnArgs.GetFloat( va( "shaderParm%d", parmnum ), "0", value ) ) {
			for( i = 0; i < targets.Num(); i++ ) {
				ent = targets[ i ].GetEntity();
				if ( ent ) {
					ent->SetShaderParm( parmnum, value );
				}
			}
			if (spawnArgs.GetBool("toggle") && (value == 0 || value == 1)) {
				int val = static_cast<int>(value);
				val ^= 1;
				value = val;
				spawnArgs.SetFloat(va("shaderParm%d", parmnum), value);
			}
		}
	}
}


/*
===============================================================================

idTarget_SetShaderTime

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetShaderTime )
	EVENT( EV_Activate,	idTarget_SetShaderTime::Event_Activate )
END_CLASS

/*
================
idTarget_SetShaderTime::Event_Activate
================
*/
void idTarget_SetShaderTime::Event_Activate( idEntity *activator ) {
	int			i;
	idEntity *	ent;
	float		time;

	time = -MS2SEC( gameLocal.time );
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->SetShaderParm( SHADERPARM_TIMEOFFSET, time );
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>(ent)->SetLightParm( SHADERPARM_TIMEOFFSET, time );
			}
		}
	}
}

/*
===============================================================================

idTarget_FadeEntity

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_FadeEntity )
	EVENT( EV_Activate,				idTarget_FadeEntity::Event_Activate )
END_CLASS

/*
================
idTarget_FadeEntity::idTarget_FadeEntity
================
*/
idTarget_FadeEntity::idTarget_FadeEntity( void ) {
	fadeFrom.Zero();
	fadeStart = 0;
	fadeEnd = 0;
}

/*
================
idTarget_FadeEntity::Save
================
*/
void idTarget_FadeEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteVec4( fadeFrom );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
}

/*
================
idTarget_FadeEntity::Restore
================
*/
void idTarget_FadeEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec4( fadeFrom );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
}

/*
================
idTarget_FadeEntity::Event_Activate
================
*/
void idTarget_FadeEntity::Event_Activate( idEntity *activator ) {
	idEntity *ent;
	int i;

	if ( !targets.Num() ) {
		return;
	}

	// always allow during cinematics
	cinematic = true;
	BecomeActive( TH_THINK );

	ent = this;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->GetColor( fadeFrom );
			break;
		}
	}

	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( spawnArgs.GetFloat( "fadetime" ) );
}

/*
================
idTarget_FadeEntity::Think
================
*/
void idTarget_FadeEntity::Think( void ) {
	int			i;
	idEntity	*ent;
	idVec4		color;
	idVec4		fadeTo;
	float		frac;

	if ( thinkFlags & TH_THINK ) {
		GetColor( fadeTo );
		if ( gameLocal.time >= fadeEnd ) {
			color = fadeTo;
			BecomeInactive( TH_THINK );
		} else {
			frac = ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart );
			color.Lerp( fadeFrom, fadeTo, frac );
		}

		// set the color on the targets
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent ) {
				ent->SetColor( color );
			}
		}
	} else {
		BecomeInactive( TH_ALL );
	}
}

/*
===============================================================================

idTarget_LightFadeIn

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_LightFadeIn )
	EVENT( EV_Activate,				idTarget_LightFadeIn::Event_Activate )
END_CLASS

/*
================
idTarget_LightFadeIn::Event_Activate
================
*/
void idTarget_LightFadeIn::Event_Activate( idEntity *activator ) {
	idEntity *ent;
	idLight *light;
	int i;
	float time;

	if ( !targets.Num() ) {
		return;
	}

	time = spawnArgs.GetFloat( "fadetime" );
	ent = this;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->IsType( idLight::Type ) ) {
			light = static_cast<idLight *>( ent );
			light->FadeIn( time );
		} else {
			gameLocal.Printf( "'%s' targets non-light '%s'", name.c_str(), ent->GetName() );
		}
	}
}

/*
===============================================================================

idTarget_LightFadeOut

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_LightFadeOut )
	EVENT( EV_Activate,				idTarget_LightFadeOut::Event_Activate )
END_CLASS

/*
================
idTarget_LightFadeOut::Event_Activate
================
*/
void idTarget_LightFadeOut::Event_Activate( idEntity *activator ) {
	idEntity *ent;
	idLight *light;
	int i;
	float time;

	if ( !targets.Num() ) {
		return;
	}

	time = spawnArgs.GetFloat( "fadetime" );
	ent = this;
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->IsType( idLight::Type ) ) {
			light = static_cast<idLight *>( ent );
			light->FadeOut( time );
		} else {
			gameLocal.Printf( "'%s' targets non-light '%s'", name.c_str(), ent->GetName() );
		}
	}
}

/*
===============================================================================

idTarget_Give

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_Give )
	EVENT( EV_Activate,				idTarget_Give::Event_Activate )
END_CLASS

/*
================
idTarget_Give::Spawn
================
*/
void idTarget_Give::Spawn( void ) {
	if ( spawnArgs.GetBool( "onSpawn" ) ) {
		PostEventMS( &EV_Activate, 50 );
	}
}

/*
================
idTarget_Give::Event_Activate
================
*/
void idTarget_Give::Event_Activate( idEntity *activator ) {
	
	if ( spawnArgs.GetBool( "development" ) && developer.GetInteger() == 0 ) {
		return;
	}

	static int giveNum = 0;
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		const idKeyValue *kv = spawnArgs.MatchPrefix( "item", NULL );
		while ( kv ) {
			const idDict *dict = gameLocal.FindEntityDefDict( kv->GetValue(), false );
			if ( dict ) {
				idDict d2;
				d2.Copy( *dict );
				d2.Set( "name", va( "givenitem_%i", giveNum++ ) );
				idEntity *ent = NULL;
				if ( gameLocal.SpawnEntityDef( d2, &ent ) && ent && ent->IsType( idItem::Type ) ) {
					idItem *item = static_cast<idItem*>(ent);
					item->GiveToPlayer( gameLocal.GetLocalPlayer() );
				}
			}
			kv = spawnArgs.MatchPrefix( "item", kv );
		}
	}
}

/*
===============================================================================

idTarget_SetModel

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetModel )
	EVENT( EV_Activate,	idTarget_SetModel::Event_Activate )
END_CLASS

/*
================
idTarget_SetModel::Spawn
================
*/
void idTarget_SetModel::Spawn( void ) {
	const char *model;

	model = spawnArgs.GetString( "newmodel" );
	if ( declManager->FindType( DECL_MODELDEF, model, false ) == NULL ) {
		// precache the render model
		renderModelManager->FindModel( model );
		// precache .cm files only
		collisionModelManager->LoadModel( model, true );
	}
}

/*
================
idTarget_SetModel::Event_Activate
================
*/
void idTarget_SetModel::Event_Activate( idEntity *activator ) {
	for( int i = 0; i < targets.Num(); i++ ) {
		idEntity *ent = targets[ i ].GetEntity();
		if ( ent ) {
			ent->SetModel( spawnArgs.GetString( "newmodel" ) );
		}
	}
}


/*
===============================================================================

idTarget_SetInfluence

===============================================================================
*/

const idEventDef EV_RestoreInfluence( "<RestoreInfluece>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_GatherEntities( "<GatherEntities>", EventArgs(), EV_RETURNS_VOID, "internal"  );
const idEventDef EV_Flash( "<Flash>", EventArgs('f', "", "", 'd', "", ""), EV_RETURNS_VOID, "internal");
const idEventDef EV_ClearFlash( "<ClearFlash>", EventArgs('f', "", ""), EV_RETURNS_VOID, "internal");

CLASS_DECLARATION( idTarget, idTarget_SetInfluence )
	EVENT( EV_Activate,	idTarget_SetInfluence::Event_Activate )
	EVENT( EV_RestoreInfluence,	idTarget_SetInfluence::Event_RestoreInfluence )
	EVENT( EV_GatherEntities, idTarget_SetInfluence::Event_GatherEntities )
	EVENT( EV_Flash, idTarget_SetInfluence::Event_Flash )
	EVENT( EV_ClearFlash, idTarget_SetInfluence::Event_ClearFlash )
END_CLASS

/*
================
idTarget_SetInfluence::idTarget_SetInfluence
================
*/
idTarget_SetInfluence::idTarget_SetInfluence( void ) {
	flashIn = 0.0f;
	flashOut = 0.0f;
	delay = 0.0f;
	switchToCamera = NULL;
	soundFaded = false;
	restoreOnTrigger = false;
}

/*
================
idTarget_SetInfluence::Save
================
*/
void idTarget_SetInfluence::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( lightList.Num() );
	for( i = 0; i < lightList.Num(); i++ ) {
		savefile->WriteInt( lightList[ i ] );
	}

	savefile->WriteInt( guiList.Num() );
	for( i = 0; i < guiList.Num(); i++ ) {
		savefile->WriteInt( guiList[ i ] );
	}

	savefile->WriteInt( soundList.Num() );
	for( i = 0; i < soundList.Num(); i++ ) {
		savefile->WriteInt( soundList[ i ] );
	}

	savefile->WriteInt( genericList.Num() );
	for( i = 0; i < genericList.Num(); i++ ) {
		savefile->WriteInt( genericList[ i ] );
	}

	savefile->WriteFloat( flashIn );
	savefile->WriteFloat( flashOut );

	savefile->WriteFloat( delay );

	savefile->WriteString( flashInSound );
	savefile->WriteString( flashOutSound );

	savefile->WriteObject( switchToCamera );

	savefile->WriteFloat( fovSetting.GetStartTime() );
	savefile->WriteFloat( fovSetting.GetDuration() );
	savefile->WriteFloat( fovSetting.GetStartValue() );
	savefile->WriteFloat( fovSetting.GetEndValue() );

	savefile->WriteBool( soundFaded );
	savefile->WriteBool( restoreOnTrigger );
}

/*
================
idTarget_SetInfluence::Restore
================
*/
void idTarget_SetInfluence::Restore( idRestoreGame *savefile ) {
	int i, num;
	int itemNum;
	float set;

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( itemNum );
		lightList.Append( itemNum );
	}

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( itemNum );
		guiList.Append( itemNum );
	}

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( itemNum );
		soundList.Append( itemNum );
	}

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadInt( itemNum );
		genericList.Append( itemNum );
	}

	savefile->ReadFloat( flashIn );
	savefile->ReadFloat( flashOut );

	savefile->ReadFloat( delay );

	savefile->ReadString( flashInSound );
	savefile->ReadString( flashOutSound );

	savefile->ReadObject( reinterpret_cast<idClass *&>( switchToCamera ) );

	savefile->ReadFloat( set );
	fovSetting.SetStartTime( set );
	savefile->ReadFloat( set );
	fovSetting.SetDuration( set );
	savefile->ReadFloat( set );
	fovSetting.SetStartValue( set );
	savefile->ReadFloat( set );
	fovSetting.SetEndValue( set );

	savefile->ReadBool( soundFaded );
	savefile->ReadBool( restoreOnTrigger );
}

/*
================
idTarget_SetInfluence::Spawn
================
*/
void idTarget_SetInfluence::Spawn() {
	PostEventMS( &EV_GatherEntities, 0 );
	flashIn = spawnArgs.GetFloat( "flashIn", "0" );
	flashOut = spawnArgs.GetFloat( "flashOut", "0" );
	flashInSound = spawnArgs.GetString( "snd_flashin" );
	flashOutSound = spawnArgs.GetString( "snd_flashout" );
	delay = spawnArgs.GetFloat( "delay" );
	soundFaded = false;
	restoreOnTrigger = false;

	// always allow during cinematics
	cinematic = true;
}

/*
================
idTarget_SetInfluence::Event_Flash
================
*/
void idTarget_SetInfluence::Event_Flash( float flash, int out ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	player->playerView.Fade( idVec4( 1, 1, 1, 1 ), static_cast<int>(flash) );
	const idSoundShader *shader = NULL;
	if ( !out && flashInSound.Length() ){
		shader = declManager->FindSound( flashInSound );
		player->StartSoundShader( shader, SND_CHANNEL_VOICE, 0, false, NULL );
	} else if ( out && ( flashOutSound.Length() || flashInSound.Length() ) ) {
		shader = declManager->FindSound( flashOutSound.Length() ? flashOutSound : flashInSound );
		player->StartSoundShader( shader, SND_CHANNEL_VOICE, 0, false, NULL );
	}
	PostEventSec( &EV_ClearFlash, flash, flash );
}


/*
================
idTarget_SetInfluence::Event_ClearFlash
================
*/
void idTarget_SetInfluence::Event_ClearFlash( float flash ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	player->playerView.Fade( vec4_zero , static_cast<int>(flash) );
}
/*
================
idTarget_SetInfluence::Event_GatherEntities
================
*/
void idTarget_SetInfluence::Event_GatherEntities() {
	int i, listedEntities;

//	bool demonicOnly = spawnArgs.GetBool( "effect_demonic" );
	bool lights = spawnArgs.GetBool( "effect_lights" );
	bool sounds = spawnArgs.GetBool( "effect_sounds" );
	bool guis = spawnArgs.GetBool( "effect_guis" );
	bool models = spawnArgs.GetBool( "effect_models" );
	bool vision = spawnArgs.GetBool( "effect_vision" );
	bool targetsOnly = spawnArgs.GetBool( "targetsOnly" );

	lightList.Clear();
	guiList.Clear();
	soundList.Clear();

	if ( spawnArgs.GetBool( "effect_all" ) ) {
		lights = sounds = guis = models = vision = true;
	}

	idClip_EntityList entityList;
	if ( targetsOnly ) {
		listedEntities = targets.Num();
		entityList.SetNum( listedEntities );
		for ( i = 0; i < listedEntities; i++ ) {
			entityList[i] = targets[i].GetEntity();
		}
	} else {
		float radius = spawnArgs.GetFloat( "radius" );
		listedEntities = gameLocal.EntitiesWithinRadius( GetPhysics()->GetOrigin(), radius, entityList );
	}

	for( i = 0; i < listedEntities; i++ ) {
		idEntity *ent = entityList[ i ];
		if ( ent ) {
			if ( lights && ent->IsType( idLight::Type ) && ent->spawnArgs.FindKey( "color_demonic" ) ) {
				lightList.Append( ent->entityNumber );
				continue;
			}
			if ( sounds && ent->IsType( idSound::Type ) && ent->spawnArgs.FindKey( "snd_demonic" ) ) {
				soundList.Append( ent->entityNumber );
				continue;
			}
			if ( guis && ent->GetRenderEntity() && ent->GetRenderEntity()->gui[ 0 ] && ent->spawnArgs.FindKey( "gui_demonic" ) ) {
				guiList.Append( ent->entityNumber );
				continue;
			}
			if ( ent->IsType( idStaticEntity::Type ) && ent->spawnArgs.FindKey( "color_demonic" ) ) {
				genericList.Append( ent->entityNumber );
				continue;
			}
		}
	}
	idStr temp;
	temp = spawnArgs.GetString( "switchToView" );
	switchToCamera = ( temp.Length() ) ? gameLocal.FindEntity( temp ) : NULL;

}

/*
================
idTarget_SetInfluence::Event_Activate
================
*/
void idTarget_SetInfluence::Event_Activate( idEntity *activator ) {
	int i, j;
	idEntity *ent;
	idLight *light;
	idSound *sound;
	idStaticEntity *generic;
	const char *parm;
	const char *skin;
	bool update;
	idVec3 color;
	idVec4 colorTo;
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();

	if ( spawnArgs.GetBool( "triggerActivate" ) ) {
		if ( restoreOnTrigger ) {
			ProcessEvent( &EV_RestoreInfluence );
			restoreOnTrigger = false;
			return;
		}
		restoreOnTrigger = true;
	}

	float fadeTime = spawnArgs.GetFloat( "fadeWorldSounds" );

	if ( delay > 0.0f ) {
		PostEventSec( &EV_Activate, delay, activator );
		delay = 0.0f;
		// start any sound fading now
		if ( fadeTime ) {
			gameSoundWorld->FadeSoundClasses( 0, -40.0f, fadeTime );
			soundFaded = true;
		}
		return;
	} else if ( fadeTime && !soundFaded ) {
		gameSoundWorld->FadeSoundClasses( 0, -40.0f, fadeTime );
		soundFaded = true;
	}

	if ( spawnArgs.GetBool( "triggerTargets" ) ) {
		ActivateTargets( activator );
	}

	if ( flashIn ) {
		PostEventSec( &EV_Flash, 0.0f, flashIn, 0 );
	}

	parm = spawnArgs.GetString( "snd_influence" );
	if ( parm && *parm ) {
		PostEventSec( &EV_StartSoundShader, flashIn, parm, SND_CHANNEL_ANY );
	}

	if ( switchToCamera ) {
		switchToCamera->PostEventSec( &EV_Activate, flashIn + 0.05f, this );
	}

	int fov = spawnArgs.GetInt( "fov" );
	if ( fov ) {
		fovSetting.Init( gameLocal.time, SEC2MS( spawnArgs.GetFloat( "fovTime" ) ), player->DefaultFov(), fov );
		BecomeActive( TH_THINK );
	}

	for ( i = 0; i < genericList.Num(); i++ ) {
		ent = gameLocal.entities[genericList[i]];
		if ( ent == NULL ) {
			continue;
		}
		generic = static_cast<idStaticEntity*>( ent );
		color = generic->spawnArgs.GetVector( "color_demonic" );
		colorTo.Set( color.x, color.y, color.z, 1.0f );
		generic->Fade( colorTo, spawnArgs.GetFloat( "fade_time", "0.25" ) );
	}

	for ( i = 0; i < lightList.Num(); i++ ) {
		ent = gameLocal.entities[lightList[i]];
		if ( ent == NULL || !ent->IsType( idLight::Type ) ) {
			continue;
		}
		light = static_cast<idLight *>(ent);
		parm = light->spawnArgs.GetString( "mat_demonic" );
		if ( parm && *parm ) {
			light->SetShader( parm );
		}
		
		color = light->spawnArgs.GetVector( "_color" );
		color = light->spawnArgs.GetVector( "color_demonic", color.ToString() );
		colorTo.Set( color.x, color.y, color.z, 1.0f );
		light->Fade( colorTo, spawnArgs.GetFloat( "fade_time", "0.25" ) );
	}

	for ( i = 0; i < soundList.Num(); i++ ) {
		ent = gameLocal.entities[soundList[i]];
		if ( ent == NULL || !ent->IsType( idSound::Type ) ) {
			continue;
		}
		sound = static_cast<idSound *>(ent);
		parm = sound->spawnArgs.GetString( "snd_demonic" );
		if ( parm && *parm ) {
			if ( sound->spawnArgs.GetBool( "overlayDemonic" ) ) {
				sound->StartSound( "snd_demonic", SND_CHANNEL_DEMONIC, 0, false, NULL );
			} else {
				sound->StopSound( SND_CHANNEL_ANY, false );
				sound->SetSound( parm );
			}
		}
	}

	for ( i = 0; i < guiList.Num(); i++ ) {
		ent = gameLocal.entities[guiList[i]];
		if ( ent == NULL || ent->GetRenderEntity() == NULL ) {
			continue;
		}
		update = false;
		for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->GetRenderEntity()->gui[ j ] && ent->spawnArgs.FindKey( j == 0 ? "gui_demonic" : va( "gui_demonic%d", j+1 ) ) ) {
				ent->GetRenderEntity()->gui[ j ] = uiManager->FindGui( ent->spawnArgs.GetString( j == 0 ? "gui_demonic" : va( "gui_demonic%d", j+1 ) ), true );
				update = true;
			}
		}

		if ( update ) {
			ent->UpdateVisuals();
			ent->Present();
		}
	}

	player->SetInfluenceLevel( spawnArgs.GetInt( "influenceLevel" ) );

	int snapAngle = spawnArgs.GetInt( "snapAngle" );
	if ( snapAngle ) {
		idAngles ang( 0, snapAngle, 0 );
		player->SetViewAngles( ang );
		player->SetAngles( ang );
	}

	if ( spawnArgs.GetBool( "effect_vision" ) ) {
		parm = spawnArgs.GetString( "mtrVision" );
		skin = spawnArgs.GetString( "skinVision" );
		player->SetInfluenceView( parm, skin, spawnArgs.GetInt( "visionRadius" ), this ); 
	}

	parm = spawnArgs.GetString( "mtrWorld" );
	if ( parm && *parm ) {
		gameLocal.SetGlobalMaterial( declManager->FindMaterial( parm ) );
	}

	if ( !restoreOnTrigger ) {
		PostEventMS( &EV_RestoreInfluence, SEC2MS( spawnArgs.GetFloat( "time" ) ) );
	}
}

/*
================
idTarget_SetInfluence::Think
================
*/
void idTarget_SetInfluence::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		idPlayer *player = gameLocal.GetLocalPlayer();
		player->SetInfluenceFov( fovSetting.GetCurrentValue( gameLocal.time ) );
		if ( fovSetting.IsDone( gameLocal.time ) ) {
			if ( !spawnArgs.GetBool( "leaveFOV" ) ) {
				player->SetInfluenceFov( 0 );
			}
			BecomeInactive( TH_THINK );
		}
	} else {
		BecomeInactive( TH_ALL );
	}
}


/*
================
idTarget_SetInfluence::Event_RestoreInfluence
================
*/
void idTarget_SetInfluence::Event_RestoreInfluence() {
	int i, j;
	idEntity *ent;
	idLight *light;
	idSound *sound;
	idStaticEntity *generic;
	bool update;
	idVec3 color;
	idVec4 colorTo;

	if ( flashOut ) {
		PostEventSec( &EV_Flash, 0.0f, flashOut, 1 );
	}

	if ( switchToCamera ) {
		switchToCamera->PostEventMS( &EV_Activate, 0, this );
	}

	for ( i = 0; i < genericList.Num(); i++ ) {
		ent = gameLocal.entities[genericList[i]];
		if ( ent == NULL ) {
			continue;
		}
		generic = static_cast<idStaticEntity*>( ent );
		colorTo.Set( 1.0f, 1.0f, 1.0f, 1.0f );
		generic->Fade( colorTo, spawnArgs.GetFloat( "fade_time", "0.25" ) );
	}

	for ( i = 0; i < lightList.Num(); i++ ) {
		ent = gameLocal.entities[lightList[i]];
		if ( ent == NULL || !ent->IsType( idLight::Type ) ) {
			continue;
		}
		light = static_cast<idLight *>(ent);
		if ( !light->spawnArgs.GetBool( "leave_demonic_mat" ) ) {
			const char *texture = light->spawnArgs.GetString( "texture", "lights/squarelight1" );
			light->SetShader( texture );
		}
		color = light->spawnArgs.GetVector( "_color" );
		colorTo.Set( color.x, color.y, color.z, 1.0f );
		light->Fade( colorTo, spawnArgs.GetFloat( "fade_time", "0.25" ) );
	}

	for ( i = 0; i < soundList.Num(); i++ ) {
		ent = gameLocal.entities[soundList[i]];
		if ( ent == NULL || !ent->IsType( idSound::Type ) ) {
			continue;
		}
		sound = static_cast<idSound *>(ent);
		sound->StopSound( SND_CHANNEL_ANY, false );
		sound->SetSound( sound->spawnArgs.GetString( "s_shader" ) );
	}

	for ( i = 0; i < guiList.Num(); i++ ) {
		ent = gameLocal.entities[guiList[i]];
		if ( ent == NULL || GetRenderEntity() == NULL ) {
			continue;
		}
		update = false;
		for( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->GetRenderEntity()->gui[ j ] ) {
				ent->GetRenderEntity()->gui[ j ] = uiManager->FindGui( ent->spawnArgs.GetString( j == 0 ? "gui" : va( "gui%d", j+1 ) ) );
				update = true;
			}
		}
		if ( update ) {
			ent->UpdateVisuals();
			ent->Present();
		}
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	player->SetInfluenceLevel( 0 );
	player->SetInfluenceView( NULL, NULL, 0.0f, NULL );
	player->SetInfluenceFov( 0 );
	gameLocal.SetGlobalMaterial( NULL );
	float fadeTime = spawnArgs.GetFloat( "fadeWorldSounds" );
	if ( fadeTime ) {
		gameSoundWorld->FadeSoundClasses( 0, 0.0f, fadeTime / 2.0f );
	}

}

/*
===============================================================================

idTarget_SetKeyVal

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetKeyVal )
	EVENT( EV_Activate,	idTarget_SetKeyVal::Event_Activate )
END_CLASS

/*
================
idTarget_SetKeyVal::Event_Activate
================
*/
void idTarget_SetKeyVal::Event_Activate( idEntity *activator ) {
	int i;
	idStr key, val;
	idEntity *ent;
	const idKeyValue *kv;
	int n;

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent ) {
			kv = spawnArgs.MatchPrefix("keyval");
			while ( kv ) {
				n = kv->GetValue().Find( ";" );
				if ( n > 0 ) {
					key = kv->GetValue().Left( n );
					val = kv->GetValue().Right( kv->GetValue().Length() - n - 1 );
					ent->spawnArgs.Set( key, val );
					for ( int j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
						if ( ent->GetRenderEntity()->gui[ j ] ) {
							if ( idStr::Icmpn( key, "gui_", 4 ) == 0 ) {
								ent->GetRenderEntity()->gui[ j ]->SetStateString( key, val );
								ent->GetRenderEntity()->gui[ j ]->StateChanged( gameLocal.time );
							}
						}
					}
				}
				kv = spawnArgs.MatchPrefix( "keyval", kv );
			}
			ent->UpdateChangeableSpawnArgs( NULL );
			ent->UpdateVisuals();
			ent->Present();
		}
	}
}

/*
===============================================================================

idTarget_SetFov

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_SetFov )
	EVENT( EV_Activate,	idTarget_SetFov::Event_Activate )
END_CLASS


/*
================
idTarget_SetFov::Save
================
*/
void idTarget_SetFov::Save( idSaveGame *savefile ) const {

	savefile->WriteFloat( fovSetting.GetStartTime() );
	savefile->WriteFloat( fovSetting.GetDuration() );
	savefile->WriteFloat( fovSetting.GetStartValue() );
	savefile->WriteFloat( fovSetting.GetEndValue() );
}

/*
================
idTarget_SetFov::Restore
================
*/
void idTarget_SetFov::Restore( idRestoreGame *savefile ) {
	float setting;

	savefile->ReadFloat( setting );
	fovSetting.SetStartTime( setting );
	savefile->ReadFloat( setting );
	fovSetting.SetDuration( setting );
	savefile->ReadFloat( setting );
	fovSetting.SetStartValue( static_cast<int>(setting) );
	savefile->ReadFloat( setting );
	fovSetting.SetEndValue( static_cast<int>(setting) );

	fovSetting.GetCurrentValue( gameLocal.time );
}

/*
================
idTarget_SetFov::Event_Activate
================
*/
void idTarget_SetFov::Event_Activate( idEntity *activator ) {
	// always allow during cinematics
	cinematic = true;

	idPlayer *player = gameLocal.GetLocalPlayer();
	fovSetting.Init( gameLocal.time, SEC2MS( spawnArgs.GetFloat( "time" ) ), static_cast<int>(player ? player->DefaultFov() : g_fov.GetFloat()), static_cast<int>(spawnArgs.GetFloat( "fov" )) );
	BecomeActive( TH_THINK );
}

/*
================
idTarget_SetFov::Think
================
*/
void idTarget_SetFov::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		idPlayer *player = gameLocal.GetLocalPlayer();
		player->SetInfluenceFov( fovSetting.GetCurrentValue( gameLocal.time ) );
		if ( fovSetting.IsDone( gameLocal.time ) ) {
			player->SetInfluenceFov( 0.0f );
			BecomeInactive( TH_THINK );
		}
	} else {
		BecomeInactive( TH_ALL );
	}
}

/*
===============================================================================

idTarget_CallObjectFunction

Tels:

If "pass_self" is true, the first argument of the called object function is the
triggered entity. This is useful for teleportTo(target), for instance.

If "pass_activator" is true, the first argument of the called object function
is the entity that caused the trigger.
This is useful for atdm:voice, for instance.

If "pass_self" and "pass_activator" are true, then the order of arguments 
is first "target", then as second argument "activator".

Note that the method you want to call needs to have exactly the number
of arguments, e.g. zero (pass_self and pass_activator false), 1 (pass_self
or pass_activator true) or two (both are true).

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_CallObjectFunction )
	EVENT( EV_Activate,	idTarget_CallObjectFunction::Event_Activate )
END_CLASS

/*
================
idTarget_CallObjectFunction::Event_Activate
================
*/
void idTarget_CallObjectFunction::Event_Activate( idEntity *activator ) {
	int					i;
	idEntity			*ent;
	const function_t	*func;
	const char			*funcName;
	idThread			*thread;

	funcName  = spawnArgs.GetString( "call" );
	// we delay each post by: delay +  (wait + (wait_add * numberOfTarget)) * numberOfTarget
	float wait			= spawnArgs.GetFloat ( "wait", "0");
	float wait_add		= spawnArgs.GetFloat ( "wait_add", "0");
	float wait_mul		= spawnArgs.GetFloat ( "wait_mul", "0");
	float delay			= spawnArgs.GetFloat ( "delay", "0");
	bool pass_self		= spawnArgs.GetBool  ( "pass_self", "0");
	bool pass_activator	= spawnArgs.GetBool  ( "pass_activator", "0");

	// avoid a negative delay
	if (wait < 0) { wait = 0; }
	if (delay < 0) { delay = 0; }
	// wait_add can be negative, we will later make sure that wait is never negative
	//
	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("%s: Calling object function %s on %i targets.\r", name.c_str(), funcName, targets.Num() );

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && ent->scriptObject.HasObject() ) {
			func = ent->scriptObject.GetFunction( funcName );
			if ( !func ) {
				gameLocal.Error( "Function '%s' not found on entity '%s' for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			int numParams = 1;
			if (pass_self) { numParams ++; }
			if (pass_activator) { numParams ++; }

			if ( func->type->NumParameters() != numParams ) {
				gameLocal.Error( "Function '%s' on entity '%s' has the wrong number of parameters for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			if ( !ent->scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
				gameLocal.Error( "Function '%s' on entity '%s' is the wrong type for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Starting call for target #%i\r", i);
			// create a thread and call the function
			thread = new idThread();
			if (numParams == 1) {
				thread->CallFunction( ent, func, true );
			} else if (numParams == 2) {
				if (pass_self) {
					thread->CallFunctionArgs( func, true, "ee", ent, this );
				} else {
					thread->CallFunctionArgs( func, true, "ee", ent, activator );
				}
			} else { // if (numParams == 3) {
				thread->CallFunctionArgs( func, true, "eee", ent, this, activator );
			}
			thread->DelayedStart( delay );

			delay += wait;
			wait += wait_add;
			wait *= wait_mul;
			// avoid a negative delay
			if (wait < 0) { wait = 0; }
		}
		else
		{
			DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("No entity or no script object on target #%i.\r", i );
		}
	}
}

/*
===============================================================================

Tels idTarget_PostScriptEvent

This locks up the named event and then posts it with the specified "delay"
for each target. The delay is increased for each target as specified with
the "wait" spawnarg.

If "pass_self" is true, the first argument of the posted event is the
triggered entity. This is useful for target->teleportTo(triggeredEntity).

If "pass_activator" is true, the first argument of the posted event is the
entity that activated the trigger. This is useful for target->AddItemToInv(activator).

If "propagate_to_team" is true, then the event will be posted to all members
of the team that the target is a member of. Useful for posting events
to entites that have other entities bound to them like holders with lights.

In case the named event cannot be found, this tries to call the script
object function on each target entity, provided the entity in question
has a script object (if not, a warning is issued) and the method
there exists (if not, an error is thrown).
===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_PostScriptEvent )
	EVENT( EV_Activate,	idTarget_PostScriptEvent::Event_Activate )
END_CLASS

void idTarget_PostScriptEvent::TryPostOrCall( idEntity *ent, idEntity *activator, const idEventDef *ev, const char* funcName, const bool pass_self, const bool pass_activator, const float delay)
{
	const function_t	*func;

	if (ev) {
		if (pass_self) {
			ent->PostEventSec( ev, delay, this );
		}
		else if (pass_activator && !pass_self) {
			ent->PostEventSec( ev, delay, activator );
		} else {
			// both pass_self and pass_activator
			ent->PostEventSec( ev, delay, this, activator );
		}
	} else {
		// try the script object function
		if ( ent->scriptObject.HasObject() ) {
			func = ent->scriptObject.GetFunction( funcName );
			if ( !func ) {
				gameLocal.Error( "Function '%s' not found on entity '%s' for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			int numParams = 1;
			if (pass_self || pass_activator) { numParams = 2; }
			if (pass_self && pass_activator) { numParams = 3; }

			if ( func->type->NumParameters() != numParams ) {
				gameLocal.Error( "Function '%s' on entity '%s' has the wrong number of parameters for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}

			if ( !ent->scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
				gameLocal.Error( "Function '%s' on entity '%s' is the wrong type for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}

			// create a thread and call the function
			idThread *thread = new idThread();
			if (numParams == 1) {
				thread->CallFunction( ent, func, true );
			} else if (numParams == 2) {
				thread->CallFunctionArgs( func, true, "ee", ent, this );
			} else {
				thread->CallFunctionArgs( func, true, "eee", ent, this, activator );
			}
			thread->DelayedStart( delay );
		}
		else
		{
			DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("No script object on target %s.\r", ent->name.c_str() );
		}
	}
}

/*
================
idTarget_PostScriptEvent::Event_Activate
================
*/

void idTarget_PostScriptEvent::Event_Activate( idEntity *activator ) {
	int					i;
	idEntity			*ent;
	idEntity			*NextEnt;

	// we delay each post by: delay +  (wait + (wait_add * numberOfTarget)) * numberOfTarget
	float wait			= spawnArgs.GetFloat ( "wait", "0");
	float wait_add		= spawnArgs.GetFloat ( "wait_add", "0");
	float wait_mul		= spawnArgs.GetFloat ( "wait_mul", "0");
	float delay			= spawnArgs.GetFloat ( "delay", "0");
	const char* evName	= spawnArgs.GetString( "event" );
	bool pass_self		= spawnArgs.GetBool  ( "pass_self", "0");
	bool pass_activator	= spawnArgs.GetBool  ( "pass_activator", "0");
	bool do_team		= spawnArgs.GetBool  ( "propagate_to_team", "0");

	// avoid a negative delay
	if (wait < 0) { wait = 0; }
	if (delay < 0) { delay = 0; }
	// wait_add can be negative, we will later make sure that wait is never negative

	DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("%s: Posting event %s on %i targets (team: %i, pass_self: %i).\r", name.c_str(), evName, targets.Num(), do_team, pass_self );
	const idEventDef *ev = idEventDef::FindEvent( evName );

	if ( !ev ) {
		gameLocal.Error( "Unknown event '%s' on entity '%s'", evName, name.c_str() );
	}

	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();

		if (!ent) { continue; }

		// DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Posting event on target #%i.\r", i);
		if (do_team) {
			NextEnt = ent;
			idEntity* bind_master = ent->GetBindMaster();
			if (bind_master) {
				NextEnt = bind_master;
			}
    
			while ( NextEnt != NULL ) {	
				DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING(" Posting event on team member %s of target #%i.\r", NextEnt->GetName(), i);
				TryPostOrCall( NextEnt, activator, ev, evName, pass_self, pass_activator, delay);
				/* get next Team member */
				NextEnt = NextEnt->GetNextTeamEntity();
			}

		} else {
			// we should post only to the target directly
			TryPostOrCall( ent, activator, ev, evName, pass_self, pass_activator, delay);
		}

		delay += wait;
		wait += wait_add;
		wait *= wait_mul;
		// avoid a negative delay
		if (wait < 0) { wait = 0; }
	}	// end for all targets
}


/*
===============================================================================

idTarget_EnableLevelWeapons

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_EnableLevelWeapons )
	EVENT( EV_Activate,	idTarget_EnableLevelWeapons::Event_Activate )
END_CLASS

/*
================
idTarget_EnableLevelWeapons::Event_Activate
================
*/
void idTarget_EnableLevelWeapons::Event_Activate( idEntity *activator ) {
	int i;
	const char *weap;

	gameLocal.world->spawnArgs.SetBool( "no_Weapons", spawnArgs.GetBool( "disable" ) );

	if ( spawnArgs.GetBool( "disable" ) ) {
		for( i = 0; i < gameLocal.numClients; i++ ) {
			if ( gameLocal.entities[ i ] ) {
				gameLocal.entities[ i ]->ProcessEvent( &EV_Player_DisableWeapon );
			}
		}
	} else {
		weap = spawnArgs.GetString( "weapon" );
		for( i = 0; i < gameLocal.numClients; i++ ) {
			if ( gameLocal.entities[ i ] ) {
				gameLocal.entities[ i ]->ProcessEvent( &EV_Player_EnableWeapon );
				if ( weap && weap[ 0 ] ) {
					gameLocal.entities[ i ]->PostEventSec( &EV_Player_SelectWeapon, 0.5f, weap );
				}
			}
		}
	}
}

/*
===============================================================================

idTarget_RemoveWeapons

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_RemoveWeapons )
EVENT( EV_Activate,	idTarget_RemoveWeapons::Event_Activate )
END_CLASS

/*
================
idTarget_RemoveWeapons::Event_Activate
================
*/
void idTarget_RemoveWeapons::Event_Activate( idEntity *activator ) {
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] ) {
			idPlayer *player = static_cast< idPlayer* >( gameLocal.entities[i] );
			const idKeyValue *kv = spawnArgs.MatchPrefix( "weapon", NULL );
			while ( kv ) {
				player->RemoveWeapon( kv->GetValue() );
				kv = spawnArgs.MatchPrefix( "weapon", kv );
			}
			player->SelectWeapon( player->weapon_fists, true );
		}
	}
}


/*
===============================================================================

idTarget_FadeSoundClass

===============================================================================
*/

const idEventDef EV_RestoreVolume( "<RestoreVolume>", EventArgs(), EV_RETURNS_VOID, "internal" );
CLASS_DECLARATION( idTarget, idTarget_FadeSoundClass )
EVENT( EV_Activate,	idTarget_FadeSoundClass::Event_Activate )
EVENT( EV_RestoreVolume, idTarget_FadeSoundClass::Event_RestoreVolume )
END_CLASS

/*
================
idTarget_FadeSoundClass::Event_Activate
================
*/
void idTarget_FadeSoundClass::Event_Activate( idEntity *activator ) {
	float fadeTime = spawnArgs.GetFloat( "fadeTime" );
	float fadeDB = spawnArgs.GetFloat( "fadeDB" );
	float fadeDuration = spawnArgs.GetFloat( "fadeDuration" );
	int fadeClass = spawnArgs.GetInt( "fadeClass" );
	// start any sound fading now
	if ( fadeTime ) {
		gameSoundWorld->FadeSoundClasses( fadeClass, spawnArgs.GetBool( "fadeIn" ) ? fadeDB : 0.0f - fadeDB, fadeTime );
		if ( fadeDuration ) {
			PostEventSec( &EV_RestoreVolume, fadeDuration );
		}
	}
}

/*
================
idTarget_FadeSoundClass::Event_RestoreVolume
================
*/
void idTarget_FadeSoundClass::Event_RestoreVolume() {
	float fadeTime = spawnArgs.GetFloat( "fadeTime" );
	float fadeDB = spawnArgs.GetFloat( "fadeDB" );
//	int fadeClass = spawnArgs.GetInt( "fadeClass" );
	// restore volume
	gameSoundWorld->FadeSoundClasses( 0, fadeDB, fadeTime );
}

/*
===============================================================================

CTarget_AddObjectives

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_AddObjectives )
	EVENT( EV_Activate,	CTarget_AddObjectives::Event_Activate )
END_CLASS

void CTarget_AddObjectives::Spawn( void )
{
	if( !spawnArgs.GetBool( "wait_for_trigger" ) )
		PostEventMS( &EV_Activate, 0, this );
}

void CTarget_AddObjectives::Event_Activate( idEntity *activator )
{
	int SetVal(-1);

	if( gameLocal.m_MissionData )
	{
		SetVal = gameLocal.m_MissionData->AddObjsFromEnt( this );
	}

	// greebo: If the entity is not the world entity, notify the player
	if (spawnArgs.GetBool("wait_for_trigger") && activator != gameLocal.world)
	{
		gameLocal.m_MissionData->Event_NewObjective();
	}

	spawnArgs.Set( "obj_num_offset", va("%d", SetVal) );
}

/*
===============================================================================

CTarget_SetObjectiveState

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_SetObjectiveState )
	EVENT( EV_Activate,	CTarget_SetObjectiveState::Event_Activate )
END_CLASS

void CTarget_SetObjectiveState::Spawn( void )
{
	if( !spawnArgs.GetBool( "wait_for_trigger" ) )
	{
		// Immediately fire the activate event, as we 
		// don't need to wait for a trigger event
		PostEventMS(&EV_Activate, 0, this);
	}
}

void CTarget_SetObjectiveState::Event_Activate( idEntity *activator )
{
	// greebo: Get the state we should set the objectives to
	int state = spawnArgs.GetInt("obj_state", "0");

	// Find all values that match the given prefix
	const idKeyValue* keyVal = spawnArgs.MatchPrefix("obj_id");
	
	// greebo: Cycle through all matching spawnargs
	while (keyVal != NULL) {
		int objId = atoi(keyVal->GetValue().c_str());

		if (objId > 0) {
			// "Unlock" the objective first, if desired
			if (spawnArgs.GetBool("unlatch_irreversible_objectives", "1"))
			{
				gameLocal.m_MissionData->UnlatchObjective(objId-1);
			}

			// Now set the completion state
			gameLocal.m_MissionData->SetCompletionState(objId-1, state);
		}
		else {
			gameLocal.Warning("Invalid objective ID %s on CTarget_SetObjectiveState %s", keyVal->GetValue().c_str(), name.c_str());
		}

		// greebo: Lookup the next matching spawnarg
		keyVal = spawnArgs.MatchPrefix("obj_id", keyVal);
	}
}

/*
===============================================================================

CTarget_SetObjectiveVisibility

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_SetObjectiveVisibility )
	EVENT( EV_Activate,	CTarget_SetObjectiveVisibility::Event_Activate )
END_CLASS

void CTarget_SetObjectiveVisibility::Spawn( void )
{
	if( !spawnArgs.GetBool( "wait_for_trigger" ) )
	{
		// Immediately fire the activate event, as we 
		// don't need to wait for a trigger event
		PostEventMS(&EV_Activate, 0, this);
	}
}

void CTarget_SetObjectiveVisibility::Event_Activate( idEntity *activator )
{
	// Get the visibility we should set the objectives to
	bool bVisible = spawnArgs.GetBool("obj_visibility", "0");

	// Cycle through all matching spawnargs
	for (const idKeyValue* keyVal = spawnArgs.MatchPrefix("obj_id"); keyVal != NULL; 
		 keyVal = spawnArgs.MatchPrefix("obj_id", keyVal)) 
	{
		int objId = atoi(keyVal->GetValue().c_str());

		if (objId > 0)
		{
			gameLocal.m_MissionData->SetObjectiveVisibility(objId - 1, bVisible);
		}
		else
		{
			gameLocal.Warning("Invalid objective ID %s on CTarget_SetObjectiveState %s", keyVal->GetValue().c_str(), name.c_str());
			DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("Invalid objective ID %s on CTarget_SetObjectiveState %s\n", keyVal->GetValue().c_str(), name.c_str());
		}
	}
}

/*
===============================================================================

CTarget_SetObjectiveComponentState

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_SetObjectiveComponentState )
	EVENT( EV_Activate,	CTarget_SetObjectiveComponentState::Event_Activate )
END_CLASS

void CTarget_SetObjectiveComponentState::Spawn( void )
{
	if( !spawnArgs.GetBool( "wait_for_trigger" ) )
	{
		// Immediately fire the activate event, as we 
		// don't need to wait for a trigger event
		PostEventMS(&EV_Activate, 0, this);
	}
}

void CTarget_SetObjectiveComponentState::Event_Activate( idEntity *activator )
{
	// Get the state we should set the objectives to
	bool state = spawnArgs.GetBool("comp_state", "0");

	// Find all values that match the given prefix
	const idKeyValue* keyVal = spawnArgs.MatchPrefix("comp_id");
	
	// Cycle through all matching spawnargs
	while (keyVal != NULL) 
	{
		bool success = false; // whether we have a valid key value string
							  // of the form "O,C", where O and C are numbers
							  // greater than zero

		idStr StringID = keyVal->GetValue();
		
		// grayman #3554 - split the obj/comp string
		// For the record, StripTrailing(",") removes all trailing commas,
		// which is not what we want. StripLeading(",") strips all leading
		// commas, which is also not what we want.
		//objIDStr.StripTrailing(",");
		//compIDStr.StripLeading(",");

		int index = idStr::FindChar(StringID.c_str(),',');
		if ( index > 0 )
		{
			std::vector<std::string> substrings; // will hold the separated substrings
			std::string ids = StringID.c_str();
			stdext::split(substrings, ids, ",");
			const char* objective = idStr(substrings[0].c_str());
			const char* component = idStr(substrings[1].c_str());
		
			int objId = atoi( objective );
			int compId = atoi( component );

			if ( ( objId > 0 ) && ( compId > 0) )
			{
				gameLocal.m_MissionData->SetComponentState(objId - 1, compId - 1, state); // SteveL #3741: decrement indexes
				success = true;
			}
		}
		
		if (!success)
		{
			gameLocal.Warning("Invalid objective component ID %s on CTarget_SetObjectiveState %s", StringID.c_str(), name.c_str());
		}

		// Get the next matching spawnarg
		keyVal = spawnArgs.MatchPrefix("comp_id", keyVal);
	}
}

/*
===============================================================================

CTarget_StartConversation

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_StartConversation )
	EVENT( EV_Activate,	CTarget_StartConversation::Event_Activate )
END_CLASS

void CTarget_StartConversation::Spawn( void )
{
	if (spawnArgs.FindKey("conversation") == NULL) 
	{
		gameLocal.Warning("Target %s has no 'conversation' spawnarg set!", name.c_str());
		return;
	}
}

void CTarget_StartConversation::Event_Activate( idEntity *activator )
{
	idStr conversationName = spawnArgs.GetString("conversation");

	if (conversationName.IsEmpty()) 
	{
		gameLocal.Printf("Target %s has no 'conversation' spawnarg set!\n", name.c_str());
		return;
	}

	// Try to find the conversation
	int convIndex = gameLocal.m_ConversationSystem->GetConversationIndex(conversationName);

	if (convIndex == -1)
	{
		gameLocal.Printf("Target %s references non-existent conversation %s!\n", name.c_str(), conversationName.c_str());
		return;
	}

	// Pass the torch to the conversationsystem to do the rest
	gameLocal.m_ConversationSystem->StartConversation(convIndex);
}

/*
===============================================================================

CTarget_SetFrobable

===============================================================================
*/
CLASS_DECLARATION( idTarget, CTarget_SetFrobable )
	EVENT( EV_Activate,	CTarget_SetFrobable::Event_Activate )
END_CLASS

CTarget_SetFrobable::CTarget_SetFrobable( void )
{
	m_bCurFrobState = false;
	m_EntsSetUnfrobable.Clear();
}

void CTarget_SetFrobable::Spawn( void )
{
	spawnArgs.GetBool( "start_frobable", "0", m_bCurFrobState );

	// Set the contents to a useless trigger so that the collision model will be loaded
	// FLASHLIGHT_TRIGGER seems to be the only one that doesn't do anything else we don't want
	GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );

	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	// Disable the clipmodel for now, only enable when needed
	GetPhysics()->DisableClip();

	// If we don't start frobable, activate once and set everything to not frobable
	// This saves the mapper the time of manually setting everything inside not frobable to start out with
	if( !m_bCurFrobState )
	{
		m_bCurFrobState = true;
		PostEventMS( &EV_Activate, 0, this );
	}
}

void CTarget_SetFrobable::Event_Activate( idEntity *activator )
{
	bool bOnList(false);

	// Contents mask:
	int cm = CONTENTS_SOLID | CONTENTS_CORPSE | CONTENTS_RENDERMODEL | CONTENTS_BODY | CONTENTS_FROBABLE;

	// bounding box test to get entities inside
	GetPhysics()->EnableClip();
	idClip_EntityList Ents;
	int numEnts = gameLocal.clip.EntitiesTouchingBounds(GetPhysics()->GetAbsBounds(), cm, Ents);
	GetPhysics()->DisableClip();

	// toggle frobability
	m_bCurFrobState = !m_bCurFrobState;
	
	for (int i = 0; i < numEnts; i++)
	{
		idEntity* ent = Ents[i];

		// Don't set self or the world, or the player frobable
		if (ent == NULL || ent == this || 
			ent == gameLocal.world || ent == gameLocal.GetLocalPlayer())
		{
			continue;
		}

		if (ent->spawnArgs.GetBool("immune_to_target_setfrobable", "0")) 
		{
			continue; // greebo: per-entity exclusion
		}

		if( m_bCurFrobState )
		{
			// Before setting something frobable, check if it is on the
			// list of things we set un-frobable earlier
			bOnList = false;

			for( int k=0; k < m_EntsSetUnfrobable.Num(); k++ )
			{
				if( m_EntsSetUnfrobable[k] == ent->name )
				{
					bOnList = true;
					break;
				}
			}

			if ( bOnList )
			{
				ent->SetFrobable(m_bCurFrobState);
			}
		}
		else 
		{	
			// setting unfrobable
			if( ent->m_bFrobable )
			{
				m_EntsSetUnfrobable.AddUnique( ent->name );
				ent->SetFrobable( m_bCurFrobState );
			}
		}

/* Uncomment for debugging

		idStr frobnofrob = "not frobable.";
		if( m_bCurFrobState )
			frobnofrob = "frobable.";

		DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_SetFrobable: Set entity %s to frob state: %s\r", Ents[i]->name.c_str(), frobnofrob.c_str() );
*/
	}
}

void CTarget_SetFrobable::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool( m_bCurFrobState );

	savefile->WriteInt( m_EntsSetUnfrobable.Num() );
	for( int i=0;i < m_EntsSetUnfrobable.Num(); i++ )
		savefile->WriteString( m_EntsSetUnfrobable[i] );
}

void CTarget_SetFrobable::Restore( idRestoreGame *savefile )
{
	int num;

	savefile->ReadBool( m_bCurFrobState );

	m_EntsSetUnfrobable.Clear();
	savefile->ReadInt( num );
	m_EntsSetUnfrobable.SetNum( num );
	for ( int i = 0; i < num; i++ )
		savefile->ReadString(m_EntsSetUnfrobable[i]);
}

/*
================
CTarget_CallScriptFunction
================
*/
CLASS_DECLARATION( idTarget, CTarget_CallScriptFunction )
	EVENT( EV_Activate,	CTarget_CallScriptFunction::Event_Activate )
END_CLASS

void CTarget_CallScriptFunction::Event_Activate( idEntity *activator )
{
	// Get the function name
	idStr funcName = spawnArgs.GetString("call");
	if (funcName.IsEmpty())
	{
		gameLocal.Warning("Target %s has no script function to call!", name.c_str());
		return;
	}

	// Get the function
	const function_t* scriptFunction = gameLocal.program.FindFunction(funcName);
	if (scriptFunction == NULL)
	{
		// script function not found!
		gameLocal.Warning("Target '%s' specifies non-existent script function '%s'!", name.c_str(),funcName.c_str());
		return;
	}

	bool forEach = spawnArgs.GetBool("foreach","0");
	
	if (forEach)
	{
		// we delay each call by: delay +  (wait + (wait_add * numberOfTarget)) * numberOfTarget
		float wait			= spawnArgs.GetFloat ( "wait", "0");
		float wait_add		= spawnArgs.GetFloat ( "wait_add", "0");
		float wait_mul		= spawnArgs.GetFloat ( "wait_mul", "0");
		float delay			= spawnArgs.GetFloat ( "delay", "0");

		// avoid a negative delay
		if (wait < 0) { wait = 0; }
		if (delay < 0) { delay = 0; }
		// wait_add can be negative, we will later make sure that wait is never negative

		for(int i = 0; i < targets.Num(); i++ )
		{
			idEntity* ent = targets[ i ].GetEntity();
			if (!ent)
			{
				DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("No entity for script call on target #%i.\r", i );
				continue;
			}

			// each call in its own thread so they can run in parallel
			idThread* thread = new idThread();
			// Call "Foo(target, activator, triggred_entity);"
			thread->CallFunctionArgs( scriptFunction, true, "eee", ent, activator, this );
			// and finally start the new thread after "delay" ms:
			thread->DelayedStart(delay);

			delay += wait;
			wait += wait_add;
			wait *= wait_mul;
			// avoid a negative delay
			if (wait < 0) { wait = 0; }
		}
	}
	else
	{
		idThread* thread = new idThread(scriptFunction);
		thread->DelayedStart(0);
	}
}

/*
================
CTarget_ChangeLockState
================
*/
CLASS_DECLARATION( idTarget, CTarget_ChangeLockState )
	EVENT( EV_Activate,	CTarget_ChangeLockState::Event_Activate )
END_CLASS

void CTarget_ChangeLockState::Event_Activate(idEntity *activator)
{
	// Find all targetted frobmovers
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* ent = targets[i].GetEntity();

		if (ent == NULL) continue;

		if (ent->IsType(CBinaryFrobMover::Type))
		{
			CBinaryFrobMover* frobMover = static_cast<CBinaryFrobMover*>(ent);

			if (spawnArgs.GetBool("toggle", "0"))
			{
				DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_ChangeLockState: Toggling lock state of entity %s\r", ent->name.c_str());
				frobMover->ToggleLock();
			}
			else if (spawnArgs.GetBool("unlock", "1"))
			{
				DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_ChangeLockState: Unlocking entity %s\r", ent->name.c_str());
				frobMover->Unlock();
			}
			else
			{
				DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_ChangeLockState: Locking entity %s\r", ent->name.c_str());
				frobMover->Lock();
			}
		}
	}
}

/*
================
CTarget_ChangeTarget
================
*/
CLASS_DECLARATION( idTarget, CTarget_ChangeTarget )
	EVENT( EV_Activate,	CTarget_ChangeTarget::Event_Activate )
END_CLASS

void CTarget_ChangeTarget::Event_Activate(idEntity *activator)
{
	// Get the targetted entities
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* ent = targets[i].GetEntity();

		if (ent == NULL) continue;

		// Let's check if we should remove a target
		idEntity* removeEnt = gameLocal.FindEntity(spawnArgs.GetString("remove"));

		if (removeEnt != NULL)
		{
			DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_ChangeTarget: Removing target %s from %s\r", removeEnt->name.c_str(), ent->name.c_str());
			ent->RemoveTarget(removeEnt);
		}

		// Let's check if we should add a target (happens after the removal)
		idEntity* addEnt = gameLocal.FindEntity(spawnArgs.GetString("add"));

		if (addEnt != NULL)
		{
			DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Target_ChangeTarget: Adding target %s to %s\r", addEnt->name.c_str(), ent->name.c_str());
			ent->AddTarget(addEnt);
		}
	}
}

CLASS_DECLARATION( idTarget, CTarget_InterMissionTrigger )
	EVENT( EV_Activate,	CTarget_InterMissionTrigger::Event_Activate )
END_CLASS

void CTarget_InterMissionTrigger::Event_Activate(idEntity* activator)
{
	// greebo: Get the target mission number, defaults to the next mission number (which is current+2 to get the 1-based index, see comment below)
	// We don't care if this is the last mission.
	int missionNum = spawnArgs.GetInt("mission");

	if (missionNum == 0)
	{
		missionNum = gameLocal.m_MissionManager->GetCurrentMissionIndex() + 2;
	}

	// The mission number is 0-based but the mapper can use 1-based indices for convenience => subtract 1 after reading the spawnarg.
	missionNum--;

	if (missionNum < 0)
	{
		return;
	}
	
	// Get the name of the activating entity, can be overridden by the spawnarg, otherwise defaults to the activator passed in
	idStr activatorName = spawnArgs.GetString("activator");
	
	if (activatorName.IsEmpty() && activator != NULL)
	{
		activatorName = activator->name;
	}

	// Now register an inter-mission trigger for each target spawnarg we find on this entity
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("target"); kv != NULL; kv = spawnArgs.MatchPrefix("target", kv))
	{
		const idStr& targetName = kv->GetValue();

		DM_LOG(LC_MISC,LT_DEBUG)LOGSTRING("Registering Inter-Mission trigger for mission %d, from %s to %s\r", missionNum, activatorName.c_str(), targetName.c_str());

		gameLocal.AddInterMissionTrigger(missionNum, activatorName, targetName);
	}
}

/* ************************************** Target setTeam ********************************* */

// Tels: Can be targetted from a trigger and changes the team of all of its targets to the
// 		 team according to the spawnarg "team". Will also cause the affected actors to re-
//		 evaluate their targets (so if they "see" someone, they might consider them an
//		 enemy, or friend now):

CLASS_DECLARATION( idEntity, CTarget_SetTeam )
	EVENT( EV_Activate,	CTarget_SetTeam::Event_Activate )
END_CLASS

void CTarget_SetTeam::Event_Activate(idEntity* activator)
{
	float newTeam = spawnArgs.GetFloat("team", 0);

	// for all targets
	int t = targets.Num();
	for( int i = 0; i < t; i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();
		if ( ent )
		//if ( ent &&  ent->IsType(idActor::Type) )
		{
		//	idActor* actor = static_cast<idActor*>(ent);
			ent->Event_SetTeam( newTeam );
		}
	}
}	


/*
* =============================
*
*  CTarget_ItemRemove
*
* =============================
*/

CLASS_DECLARATION( idEntity, CTarget_ItemRemove )
	EVENT( EV_Activate,	CTarget_ItemRemove::Event_Activate )
END_CLASS

void CTarget_ItemRemove::RespawnItem( const char* classname, const char* itemname, const int quantity, const bool ammo )
{
	const idPlayer* player = gameLocal.GetLocalPlayer();
	idEntity* ent = NULL;
	idDict args;

	if ( quantity < 1 )
	{
		return;
	} 
	else if ( quantity > 1 ) 
	{
		const char* amountArg = ammo ? "inv_ammo_amount" : "inv_count";
		args.SetInt( amountArg, quantity );
	}

	if ( itemname )
	{
		args.Set( "name", itemname );
	}

	args.Set("classname", classname );
	args.SetVector("origin", RespawnPosition(player) );
	args.Set("inv_map_start", "0"); // Don't go straight back into inventory
	gameLocal.SpawnEntityDef( args, &ent );
	if ( ent )
	{
		ent->PostEventMS( &EV_Activate, 16, player ); // Touch to make it drop
	}
}

void CTarget_ItemRemove::Event_Activate(idEntity* activator)
{
	const bool	dropInWorld		= spawnArgs.GetBool("drop_in_world", "0");
	const char* uniqueItem		= spawnArgs.GetString("unique_item", "-");
	const char* stackableClass	= spawnArgs.GetString("stackable_class", "-");
	const int	stackableCount	= spawnArgs.GetInt("stackable_count", "0");
	const char* ammoType		= spawnArgs.GetString("ammo_type", "-");
	const int	ammoCount		= spawnArgs.GetInt("ammo_count", "0");
	idPlayer* player = gameLocal.GetLocalPlayer();
	const CInventoryPtr& inv = player->Inventory();

	// Items not found could well be correct operation for the map, so issue messages instead of warnings.	

	// Unique items
	if ( uniqueItem && *uniqueItem != '-' )
	{
		idEntity* ent = gameLocal.FindEntity( uniqueItem );
		bool success = false;
		if ( ent )
		{
			const char* classname = ent->spawnArgs.GetString("classname");
			success = player->ReplaceInventoryItem( ent, NULL ); 
			if ( success && dropInWorld )
			{
				// Unique entities are usually still in the world, just hidden.
				if ( gameLocal.FindEntity( uniqueItem ) )
				{
					ent->SetOrigin( RespawnPosition(player) );
					ent->PostEventMS( &EV_Activate, 0, player ); // unhides too
				}
				else
				{
					RespawnItem( classname, uniqueItem, 1, false );
				}
			}
		}
		if ( !success ) 
		{
			gameLocal.Printf( "Couldn't remove entity '%s' specified in 'unique_item' key in entity '%s'\n", uniqueItem, name.c_str() );
		}
	}

	// Stackables
	if ( stackableClass && *stackableClass != '-' )
	{
		CInventoryItemPtr item = inv->GetItem( stackableClass );
		if ( item && item->IsStackable() )
		{
			const int n = item->GetCount();
			const int toRemove = stackableCount <= 0 ? n : idMath::Imin(n, stackableCount);
			const char* classname = item->GetItemEntity()->spawnArgs.GetString("classname");
			item->SetCount( n - toRemove );
			if ( dropInWorld )
			{
				RespawnItem( classname, NULL, toRemove, false );
			}
		}
		else
		{
			gameLocal.Printf( "Couldn't remove inv_name '%s' specified in 'stackable_class' key in entity '%s'\n", stackableClass, name.c_str() );
		}
	}

	// Ammo
	if ( ammoType && *ammoType != '-' )
	{
		CInventoryWeaponItemPtr weap = player->GetWeaponItem(ammoType);
		if ( weap )
		{
			const int n = weap->GetAmmo();
			const int toRemove = ammoCount <= 0 ? n : idMath::Imin(n, ammoCount);
			const idStr classname = idStr("atdm:ammo_") + ammoType;
			weap->SetAmmo( n - toRemove );
			if ( dropInWorld )
			{
				RespawnItem( classname.c_str(), NULL, toRemove, true);
			}
		}
		else
		{
			gameLocal.Printf( "Couldn't remove ammo '%s' specified in 'ammo_type' key in entity '%s'\n", ammoType, name.c_str() );
		}
	}
}