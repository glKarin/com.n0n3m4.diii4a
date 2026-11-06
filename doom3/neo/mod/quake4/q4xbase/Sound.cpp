
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

  SOUND

===============================================================================
*/

const idEventDef EV_Speaker_On( "On", NULL );
const idEventDef EV_Speaker_Off( "Off", NULL );
const idEventDef EV_Speaker_Timer( "<timer>", NULL );

CLASS_DECLARATION( idEntity, idSound )
	EVENT( EV_Activate,				idSound::Event_Trigger )
	EVENT( EV_Speaker_On,			idSound::Event_On )
	EVENT( EV_Speaker_Off,			idSound::Event_Off )
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
	shakeTranslate.Zero();
	shakeRotate.Zero();
	random = 0.0f;
	wait = 0.0f;
	timerOn = false;
	playingUntilTime = 0;
}

/*
================
idSound::Save
================
*/
void idSound::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( lastSoundVol );
	savefile->WriteFloat( soundVol );
	savefile->WriteFloat( random );
	savefile->WriteFloat( wait );
	savefile->WriteBool( timerOn );
	savefile->WriteVec3( shakeTranslate );
	savefile->WriteAngles( shakeRotate );
	savefile->WriteInt( playingUntilTime );
}

/*
================
idSound::Restore
================
*/
void idSound::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( lastSoundVol );
	savefile->ReadFloat( soundVol );
	savefile->ReadFloat( random );
	savefile->ReadFloat( wait );
	savefile->ReadBool( timerOn );
	savefile->ReadVec3( shakeTranslate );
	savefile->ReadAngles( shakeRotate );
	savefile->ReadInt( playingUntilTime );
}

/*
================
idSound::Spawn
================
*/
void idSound::Spawn( void ) {
	spawnArgs.GetVector( "move", "0 0 0", shakeTranslate );
	spawnArgs.GetAngles( "rotate", "0 0 0", shakeRotate );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "wait", "0", wait );

	if ( ( wait > 0.0f ) && ( random >= wait ) ) {
		random = wait - 0.001;
		gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	soundVol		= 0.0f;
	lastSoundVol	= 0.0f;

	if ( ( shakeRotate != ang_zero ) || ( shakeTranslate != vec3_zero ) ) {
		BecomeActive( TH_THINK );
	}

	if ( !refSound.waitfortrigger && ( wait > 0.0f ) ) {
		timerOn = true;
		PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
	} else {
		timerOn = false;
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
		if ( gameLocal.isMultiplayer ) {
// RAVEN BEGIN
			idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
			if ( emitter && ( gameLocal.time < playingUntilTime ) ) {
// RAVEN END
				DoSound( false );
			} else {
				DoSound( true );
			}
		} else {
// RAVEN BEGIN
			idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
			if ( emitter && emitter->CurrentlyPlaying() ) {
// RAVEN END
				DoSound( false );
			} else {
				DoSound( true );
			}
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
	idAngles	ang;

	// run physics
	RunPhysics();

	// clear out our update visuals think flag since we never call Present
	BecomeInactive( TH_UPDATEVISUALS );
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
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );

		spawnArgs.Copy( *source );
		gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );

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
			random = wait - 0.001;
			gameLocal.Warning( "speaker '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
		}

		if ( !refSound.waitfortrigger && ( wait > 0.0f ) ) {
			timerOn = true;
			DoSound( false );
			CancelEvents( &EV_Speaker_Timer );
			PostEventSec( &EV_Speaker_Timer, wait + gameLocal.random.CRandomFloat() * random );
// RAVEN BEGIN
		} else  if ( !refSound.waitfortrigger ) {

			idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
			if( !( emitter && emitter->CurrentlyPlaying() ) ) {
// RAVEN END
				// start it if it isn't already playing, and we aren't waitForTrigger
				DoSound( true );
				timerOn = false;
			}
		}
	}
}

/*
===============
idSound::SetSound
===============
*/
void idSound::SetSound( const char *sound, int channel ) {
	const idSoundShader *shader = declManager->FindSound( sound );
	if ( shader != refSound.shader ) {
		FreeSoundEmitter( true );
	}
	gameEdit->ParseSpawnArgsToRefSound(&spawnArgs, &refSound);
	refSound.shader = shader;
// RAVEN BEGIN
	// start it if it isn't already playing, and we aren't waitForTrigger
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
    if ( !refSound.waitfortrigger && !( emitter && emitter->CurrentlyPlaying() ) ) {
// RAVEN END
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
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, refSound.parms.soundShaderFlags, true, &playingUntilTime );
		playingUntilTime += gameLocal.time;
	} else {
		StopSound( SND_CHANNEL_ANY, true );
		playingUntilTime = 0;
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

// RAVEN BEGIN
// abahr: so we only set the referenceSounds on our targets once
/*
================
idSound::FindTargets
================
*/
void idSound::FindTargets() {
	idEntity::FindTargets();

	if( !targets.Num() ) {// I don't think we ever get in here unless we have targets
		return;
	}

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( !emitter ) { 
		// if we have targets lets get an emitter
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );
	}

	SetTargetSoundHandles();
}
// RAVEN END

/*
===============
idSound::ShowEditingDialog
===============
*/
void idSound::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_SOUND, &spawnArgs );
}

// RAVEN BEGIN
// jshepard: Allow speakers to target lights and have those lights use this speaker's refSound
/*
===============
idSound::SetSoundHandles
===============
*/
void idSound::SetTargetSoundHandles( void ) {
//this code is mostly boosted from a similar function in light.cpp
	int i;
	idEntity *targetEnt;

	//is this check really necessary? We are a speaker after all...
	if ( !soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle ) ) {
		return;
	}
	
	for ( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( targetEnt && targetEnt->IsType( idLight::GetClassType() ) ) {
// RAVEN END
			idLight	*light = static_cast<idLight*>(targetEnt);

			//no need to make this speaker a lightparent....
			//light->lightParent = this;

			// explicitly delete any sounds on the entity
			light->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			light->SetRefSound(refSound.referenceSoundHandle);

			// update the renderEntity to the renderer
			light->UpdateVisuals();
		}
	}	
}

// abahr:
/*
================
idSound::GetPhysicsToSoundTransform
================
*/
bool idSound::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	origin = shakeTranslate.Random( spawnArgs.GetVector("move_random_delta"), gameLocal.random );
	axis = shakeRotate.Random( spawnArgs.GetVector("shake_random_delta"), gameLocal.random ).ToMat3();
	return true;
}
// RAVEN END
