// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Sound.h"

/*
===============================================================================

  sdSoundBroadcastData

===============================================================================
*/

/*
==============
sdSoundBroadcastData::MakeDefault
==============
*/
void sdSoundBroadcastData::MakeDefault( void ) {
	isOn = true;
}

/*
==============
sdSoundBroadcastData::Write
==============
*/
void sdSoundBroadcastData::Write( idFile* file ) const {
	file->WriteBool( isOn );
}

/*
==============
sdSoundBroadcastData::Read
==============
*/
void sdSoundBroadcastData::Read( idFile* file ) {
	file->ReadBool( isOn );
}


/*
===============================================================================

  idSound

===============================================================================
*/

idList< idSoundEmitter* > idSound::s_soundEmitters;

extern const idEventDef EV_TurnOn;
extern const idEventDef EV_TurnOff;

const idEventDefInternal EV_Speaker_Timer( "internal_timer", NULL );

CLASS_DECLARATION( idEntity, idSound )
	EVENT( EV_Activate,				idSound::Event_Trigger )
	EVENT( EV_TurnOn,				idSound::Event_On )
	EVENT( EV_TurnOff,				idSound::Event_Off )
	EVENT( EV_Speaker_Timer,		idSound::Event_Timer )
END_CLASS


/*
================
idSound::idSound
================
*/
idSound::idSound( void ) {
	lastSoundVol = 0.0f;
	soundVol = 0.0f;
	random = 0.0f;
	wait = 0.0f;
	timerOn = false;
	soundOn = false;
	playingUntilTime = 0;
}

/*
================
idSound::OnNewMapLoad
================
*/
void idSound::OnNewMapLoad( void ) {
	FreeMapSounds();
}

/*
================
idSound::OnMapClear
================
*/
void idSound::OnMapClear( void ) {
	FreeMapSounds();
}

/*
================
idSound::FreeMapSounds
================
*/
void idSound::FreeMapSounds( void ) {
	for ( int i = 0; i < s_soundEmitters.Num(); i++ ) {
		s_soundEmitters[ i ]->Free( true );
	}
	s_soundEmitters.Clear();
}

/*
================
idSound::Spawn
================
*/
void idSound::Spawn( void ) {
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "wait", "0", wait );

	if ( ( wait > 0.0f ) && ( random >= wait ) ) {
		random = wait - 0.001f;
		gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	soundVol		= 0.0f;
	lastSoundVol	= 0.0f;

	if ( !refSound.waitfortrigger && ( wait > 0.0f ) ) {
		timerOn = true;
		PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
	} else {
		timerOn = false;
		soundOn = true;
	}

	if ( g_removeStaticEntities.GetBool() && !StartSynced() && !timerOn ) {
		if ( refSound.referenceSound != NULL ) {
			s_soundEmitters.Alloc() = refSound.referenceSound;
			refSound.referenceSound = NULL;
		}
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idSound::Event_Trigger

this will toggle the idle idSound on and off
================
*/
void idSound::Event_Trigger( idEntity *activator ) {
	if ( wait > 0.0f ) {
		if ( timerOn ) {
			timerOn = false;
			CancelEvents( &EV_Speaker_Timer );
		} else {
			timerOn = true;
			DoSound( true );
			PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
		}
	} else {
		if ( refSound.referenceSound && ( gameLocal.time < playingUntilTime ) ) {
			DoSound( false );
		} else {
			DoSound( true );
		}
	}
}

/*
================
idSound::Event_Timer
================
*/
void idSound::Event_Timer( void ) {
	DoSound( true );
	PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
}

/*
================
idSound::Think
================
*/
void idSound::Think( void ) {
}

/*
===============
idSound::UpdateChangableSpawnArgs
===============
*/
void idSound::UpdateChangeableSpawnArgs( const idDict *source ) {

	idEntity::UpdateChangeableSpawnArgs( source );

	if ( source ) {
		FreeSoundEmitter( true );
		spawnArgs.Copy( *source );
		idSoundEmitter *saveRef = refSound.referenceSound;
		gameEdit->ParseSpawnArgsToRefSound( spawnArgs, refSound );
		refSound.referenceSound = saveRef;

		idVec3 origin;
		idMat3 axis;

		if ( GetPhysicsToSoundTransform( origin, axis ) ) {
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		} else {
			refSound.origin = GetPhysics()->GetOrigin();
		}

		spawnArgs.GetFloat( "random", "0", random );
		spawnArgs.GetFloat( "wait", "0", wait );

		if ( ( wait > 0.0f ) && ( random >= wait ) ) {
			random = wait - 0.001f;
			gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
		}

		if ( !refSound.waitfortrigger && ( wait > 0.0f ) ) {
			timerOn = true;
			DoSound( false );
			CancelEvents( &EV_Speaker_Timer );
			PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
		} else  if ( !refSound.waitfortrigger && !(refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) ) {
			// start it if it isn't already playing, and we aren't waitForTrigger
			DoSound( true );
			timerOn = false;
		}
	}
}

/*
===============
idSound::SetSound
===============
*/
void idSound::SetSound( const char *sound, int channel ) {
	const idSoundShader *shader = declHolder.declSoundShaderType.LocalFind( sound );
	if ( shader != refSound.shader ) {
		FreeSoundEmitter( true );
	}
	gameEdit->ParseSpawnArgsToRefSound( spawnArgs, refSound );
	refSound.shader = shader;
	// start it if it isn't already playing, and we aren't waitForTrigger
	if ( !refSound.waitfortrigger && !(refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) ) {
		DoSound( true );
	}
}

/*
================
idSound::DoSound
================
*/
void idSound::DoSound( bool play ) {
	if ( play ) {
		StartSoundShader( refSound.shader, SND_ANY, refSound.parms.soundShaderFlags, &playingUntilTime );
		playingUntilTime += gameLocal.time;
		soundOn = true;
	} else {
		StopSound( SND_ANY );
		playingUntilTime = 0;
		soundOn = false;
	}
}

/*
================
idSound::Event_On
================
*/
void idSound::Event_On( void ) {
	if ( wait > 0.0f ) {
		timerOn = true;
		PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
	}
	DoSound( true );
}

/*
================
idSound::Event_Off
================
*/
void idSound::Event_Off( void ) {
	if ( timerOn ) {
		timerOn = false;
		CancelEvents( &EV_Speaker_Timer );
	}
	DoSound( false );
}

/*
===============
idSound::ShowEditingDialog
===============
*/
void idSound::ShowEditingDialog( void ) {
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "selectSpeaker '%s'", name.c_str() ) );
}

/*
================
idSound::ApplyNetworkState
================
*/
void idSound::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdSoundBroadcastData );

		DoSound( newData.isOn );
	}

	idEntity::ApplyNetworkState( mode, newState );
}

/*
==============
idSound::ReadNetworkState
==============
*/
void idSound::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdSoundBroadcastData );

		newData.isOn = msg.ReadBool();
	}

	idEntity::ReadNetworkState( mode, baseState, newState, msg );
}

/*
==============
idSound::WriteNetworkState
==============
*/
void idSound::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdSoundBroadcastData );

		newData.isOn = soundOn;
		msg.WriteBool( newData.isOn );
	}

	idEntity::WriteNetworkState( mode, baseState, newState, msg );
}

/*
==============
idSound::CheckNetworkStateChanges
==============
*/
bool idSound::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdSoundBroadcastData );

		if ( baseData.isOn != soundOn ) {
			return true;
		}
	}
	return idEntity::CheckNetworkStateChanges( mode, baseState );
}

/*
==============
idSound::CreateNetworkStructure
==============
*/
sdEntityStateNetworkData* idSound::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdSoundBroadcastData();
	}
	return idEntity::CreateNetworkStructure( mode );
}
