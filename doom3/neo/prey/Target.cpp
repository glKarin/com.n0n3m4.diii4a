// Copyright (C) 2004 Id Software, Inc.
//
/*

Invisible entities that affect other entities or the world when activated.

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

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

#ifdef ID_DEMO_BUILD
	//HUMANHEAD jsh - if the next map is DEMO_END_LEVEL, demo is over
	//this check is done to avoid having to change map assets
	//if ( spawnArgs.GetBool( "endOfGame" ) ) {
	if ( idStr::Icmp( spawnArgs.GetString( "nextMap" ), DEMO_END_LEVEL ) == 0 ) {
		cvarSystem->SetCVarBool( "g_nightmare", true );
		gameLocal.sessionCommand = "endofDemo";
		return;
	}
#else
	if ( spawnArgs.GetBool( "endOfGame" ) ) {
		cvarSystem->SetCVarBool( "g_nightmare", true );
		gameLocal.sessionCommand = "disconnect";	// HUMANHEAD pdm: changed from disconnect
		return;
	}
#endif
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
		if ( player && ( !player->oldButtons & BUTTON_ATTACK ) && ( player->usercmd.buttons & BUTTON_ATTACK ) ) {
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
				int val = value;
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
	fovSetting.SetStartValue( setting );
	savefile->ReadFloat( setting );
	fovSetting.SetEndValue( setting );

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
	fovSetting.Init( gameLocal.time, SEC2MS( spawnArgs.GetFloat( "time" ) ), player ? player->DefaultFov() : g_fov.GetFloat(), spawnArgs.GetFloat( "fov" ) );
}

/*
===============================================================================

idTarget_CallObjectFunction

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

	funcName = spawnArgs.GetString( "call" );
	for( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( ent && ent->scriptObject.HasObject() ) {
			func = ent->scriptObject.GetFunction( funcName );
			if ( !func ) {
				gameLocal.Error( "Function '%s' not found on entity '%s' for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			if ( func->type->NumParameters() != 1 ) {
				gameLocal.Error( "Function '%s' on entity '%s' has the wrong number of parameters for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			if ( !ent->scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
				gameLocal.Error( "Function '%s' on entity '%s' is the wrong type for function call from '%s'", funcName, ent->name.c_str(), name.c_str() );
			}
			// create a thread and call the function
			thread = new idThread();
			thread->CallFunction( ent, func, true );
			thread->Start();
		}
	}
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

idTarget_Tip
	HUMANHEAD pdm: rewritten
===============================================================================
*/

const idEventDef EV_CheckPos( "<checkplayerpos>" );

CLASS_DECLARATION( idTarget, idTarget_Tip )
	EVENT( EV_Activate,		idTarget_Tip::Event_Activate )
	EVENT( EV_CheckPos,		idTarget_Tip::Event_CheckPlayerPos )
END_CLASS

idTarget_Tip::idTarget_Tip() {
	bDisabled = false;
}

void idTarget_Tip::Spawn( void ) {
}

void idTarget_Tip::Save( idSaveGame *savefile ) const {
	savefile->WriteBool(bDisabled);
}

void idTarget_Tip::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(bDisabled);
}

void idTarget_Tip::Disable() {
	CancelEvents( &EV_CheckPos);
	if (!spawnArgs.GetBool("tip_repeatable")) {
		bDisabled = true;
	}
}

void idTarget_Tip::Event_Activate( idEntity *activator ) {

	idPlayer *player = gameLocal.GetLocalPlayer();
	if( player ) {

		if (!g_tips.GetBool()) {
			player->HideTip();
			return;
		}

		if( spawnArgs.GetBool( "tip_remove" ) ) {
			// Disable all targets
			for( int i = 0; i < targets.Num(); i++ ) {
				idEntity *target = targets[ i ].GetEntity();
				if ( target && target->IsType(idTarget_Tip::Type) ) {
					static_cast<idTarget_Tip*>(target)->Disable();
				}
			}
			player->HideTip();
			return;
		}

		if( bDisabled || player->IsTipVisible() ) {
			return;
		}

		// Determine key material to display
		idStr keyMaterial, key;
		bool keyWide = false;
		if ( !spawnArgs.GetString("mtr_override", "", keyMaterial) ) {
			const char *keyBinding = spawnArgs.GetString("tip_key", NULL);
			if (keyBinding) {
				gameLocal.GetTip(keyBinding, keyMaterial, key, keyWide);
			}
		}

		if (keyMaterial.Length()) {
			int xOffset = spawnArgs.GetInt("tipOffsetX");
			int yOffset = spawnArgs.GetInt("tipOffsetY");
			player->ShowTip( keyMaterial.c_str(), spawnArgs.GetString("text_tip"), key.c_str(), spawnArgs.GetString("mtr_top", NULL), keyWide, xOffset, yOffset );
			PostEventMS( &EV_CheckPos, 1000 );
		}
	}
}

void idTarget_Tip::Event_CheckPlayerPos( void ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		idVec3 v = player->GetOrigin() - GetOrigin();
		if ( v.Length() > spawnArgs.GetFloat( "tip_distance" ) || !g_tips.GetBool() ) {
			player->HideTip();
		} else {
			PostEventMS( &EV_CheckPos, 1000 );
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

idTarget_LevelTrigger

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_LevelTrigger )
EVENT( EV_Activate,	idTarget_LevelTrigger::Event_Activate )
END_CLASS

/*
================
idTarget_LevelTrigger::Event_Activate
================
*/
void idTarget_LevelTrigger::Event_Activate( idEntity *activator ) {
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] ) {
			idPlayer *player = static_cast< idPlayer* >( gameLocal.entities[i] );
			player->SetLevelTrigger( spawnArgs.GetString( "levelName" ), spawnArgs.GetString( "triggerName" ) );
		}
	}
}


/*
===============================================================================

idTarget_EnableStamina

===============================================================================
*/

CLASS_DECLARATION( idTarget, idTarget_EnableStamina )
EVENT( EV_Activate,	idTarget_EnableStamina::Event_Activate )
END_CLASS

/*
================
idTarget_EnableStamina::Event_Activate
================
*/
void idTarget_EnableStamina::Event_Activate( idEntity *activator ) {
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		if ( gameLocal.entities[ i ] ) {
			idPlayer *player = static_cast< idPlayer* >( gameLocal.entities[i] );
			if ( spawnArgs.GetBool( "enable" ) ) {
				pm_stamina.SetFloat( player->spawnArgs.GetFloat( "pm_stamina" ) );
			} else {
				pm_stamina.SetFloat( 0.0f );
			}
		}
	}
}

/*
===============================================================================

idTarget_FadeSoundClass

===============================================================================
*/

const idEventDef EV_RestoreVolume( "<RestoreVolume>" );
CLASS_DECLARATION( idTarget, idTarget_FadeSoundClass )
	EVENT( EV_Activate,			idTarget_FadeSoundClass::Event_Activate )
	EVENT( EV_RestoreVolume,	idTarget_FadeSoundClass::Event_RestoreVolume )
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
	if ( fadeTime >= 0 ) { // HUMANHEAD rdr - was if ( fadeTime ), changed to allow 0 for instant fade
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
	float restoreTime = spawnArgs.GetFloat( "restoreTime" ); // HUMANHEAD rdr - added to allow different restoreTime
	int fadeClass = spawnArgs.GetInt( "fadeClass" );
	// restore volume
	gameSoundWorld->FadeSoundClasses( fadeClass, 0, restoreTime ); // HUMANHEAD rdr - changed from ->FadeSoundClasses( 0, fadeDB, fadeTime );
}

