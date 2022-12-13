#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_StartDelayedSoundShader( "<startDelayedSoundShader>", "sddd" );
const idEventDef EV_ResetTargetHandles( "resetTargetHandles" );
const idEventDef EV_SubtitleOff( "<subtitleOff>" );

CLASS_DECLARATION( idSound, hhSound )
	EVENT( EV_PostSpawn,					hhSound::Event_SetTargetHandles )
	EVENT( EV_ResetTargetHandles,			hhSound::Event_SetTargetHandles )
	EVENT( EV_SubtitleOff,					hhSound::Event_SubtitleOff )
END_CLASS


/*
================
hhSound::hhSound
================
*/
hhSound::hhSound( void ) {
	positionOffset.Zero();
}

/*
================
hhSound::Spawn
================
*/
void hhSound::Spawn() {
	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
hhSound::StartDelayedSoundShader
================
*/
void hhSound::StartDelayedSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast ) {
	CancelEvents( &EV_StartDelayedSoundShader );
	PostEventSec( &EV_StartDelayedSoundShader, RandomRange(spawnArgs.GetFloat("s_minDelay"), spawnArgs.GetFloat("s_maxDelay")), (const char *)shader->GetName(), (int)channel, (int)soundShaderFlags, (int)broadcast );
}

/*
================
hhSound::StopDelayedSound
================
*/
void hhSound::StopDelayedSound( const s_channelType channel, bool broadcast ) {
	CancelEvents( &EV_StartDelayedSoundShader );
	idSound::StopSound( channel, broadcast );
}

/*
================
hhSound::StartSoundShader
================
*/
bool hhSound::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int* length ) {
	bool result = true;
	float volume = 0.0f;

	positionOffset = DeterminePositionOffset();
	if( !spawnArgs.GetBool("s_useRandomDelay") ) {
		StopDelayedSound( channel, broadcast );

		result = idSound::StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );

		const char *subtitleText = spawnArgs.GetString( "subtitle", NULL );
		if ( result && subtitleText ) {
			idPlayer *player = gameLocal.GetLocalPlayer();
			if ( length > 0 && player && player->hud ) {
				player->hud->SetStateInt("subtitlefadetime", 0);
				player->hud->SetStateInt("subtitlex", 0 );
				player->hud->SetStateInt("subtitley", 400 );
				player->hud->SetStateInt("subtitlecentered", true);
				player->hud->SetStateString("subtitletext", common->GetLanguageDict()->GetString(subtitleText));
				player->hud->StateChanged(gameLocal.time);
				player->hud->HandleNamedEvent("DisplaySubtitle");
				PostEventMS( &EV_SubtitleOff, *length );
			}
		}

		if( DetermineVolume(volume) ) {
			HH_SetSoundVolume( volume, channel );
		}
	} else {
		StartDelayedSoundShader( shader, channel, soundShaderFlags, broadcast );
	}

	//trigger targets when sound ends
	if ( length && targets.Num() && spawnArgs.GetInt("trigger_targets","0") ) { 
		float delay = *length * 0.001 + spawnArgs.GetFloat("target_delay", "0");
		if ( delay < 0 ) {
			delay = 0;
		}
		PostEventSec( &EV_ActivateTargets, delay , this );
	}

	return result;
}

/*
================
hhSound::StopSound
================
*/
void hhSound::StopSound( const s_channelType channel, bool broadcast ) {
	StopDelayedSound( channel, broadcast );
}

/*
================
hhSound::RandomRange
================
*/
float hhSound::RandomRange( const float min, const float max ) {
	return hhMath::Lerp( min, max, gameLocal.random.RandomFloat() );
}

/*
===============
hhSound::DetermineVolume
===============
*/
bool hhSound::DetermineVolume( float& volume ) {
	if( !spawnArgs.GetBool("s_useRandomVolume") ) {
		return false;
	}

	volume = hhMath::dB2Scale( RandomRange(spawnArgs.GetInt("s_minVolume"), spawnArgs.GetInt("s_maxVolume")) );
	return true;
}

/*
===============
hhSound::DeterminePositionOffset
===============
*/
idVec3 hhSound::DeterminePositionOffset() {
	if( !spawnArgs.GetBool("s_useRandomPosition") ) {
		return vec3_origin;
	}

	float radius = RandomRange( spawnArgs.GetFloat("s_minRadius"), spawnArgs.GetFloat("s_maxRadius") );
	return hhUtils::RandomVector() * radius;
}

/*
================
hhSound::GetCurrentAmplitude
================
*/
float hhSound::GetCurrentAmplitude(const s_channelType channel) {
	if (refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying()) {
		return refSound.referenceSound->CurrentAmplitude(channel);
	}
	return 0.0f;
}

/*
================
hhSound::GetPhysicsToSoundTransform
================
*/
bool hhSound::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	origin = positionOffset;
	axis.Identity();
	return true;
}

/*
================
hhSound::Event_StartDelayedSoundShader
================
*/
void hhSound::Event_StartDelayedSoundShader( const char *shaderName, const s_channelType channel, int soundShaderFlags, bool broadcast ) {
	int soundLength = 0;
	float volume = 0.0f;

	const idSoundShader *shader = declManager->FindSound( shaderName );
	assert( shader );
	if( !shader ) {
		return;
	}

	if( !GetSoundEmitter() || !GetSoundEmitter()->CurrentlyPlaying() ) {
		soundLength = idSound::StartSoundShader( shader, channel, soundShaderFlags, broadcast );//Not sure if we should broadcast
		
		positionOffset = DeterminePositionOffset();
		if( DetermineVolume(volume) ) {
			HH_SetSoundVolume( volume, channel );
		}
	}

	CancelEvents( &EV_StartDelayedSoundShader );
	PostEventSec( &EV_StartDelayedSoundShader, MS2SEC(soundLength) + RandomRange(spawnArgs.GetFloat("s_minDelay"), spawnArgs.GetFloat("s_maxDelay")), shaderName, (int)channel, (int)soundShaderFlags, (int)broadcast );
}

void hhSound::Event_SubtitleOff( void ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player && player->hud ) {
		player->hud->HandleNamedEvent("RemoveSubtitleInstant");
	}
}

/*
================
hhSound::Event_SetTargetHandles

Copied from hhLight
================
*/
void hhSound::Event_SetTargetHandles( void ) {
	int i;
	idEntity *targetEnt = NULL;

	if ( !refSound.referenceSound ) {
		refSound.referenceSound = gameSoundWorld->AllocSoundEmitter();
	}

	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( targetEnt ) {
			if( targetEnt->IsType(idLight::Type) ) {
				static_cast<idLight*>(targetEnt)->SetLightParent( this );
			}

			targetEnt->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			targetEnt->GetRenderEntity()->referenceSound = refSound.referenceSound;

			// update the renderEntity to the renderer
			targetEnt->UpdateVisuals();
		}
	}
}

/*
================
hhSound::Save
================
*/
void hhSound::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( positionOffset );
}

/*
================
hhSound::Restore
================
*/
void hhSound::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( positionOffset );
}

