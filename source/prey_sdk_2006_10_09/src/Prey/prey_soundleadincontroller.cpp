#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idClass, hhSoundLeadInController )
END_CLASS

/*
================
hhSoundLeadInController::hhSoundLeadInController

HUMANHEAD: aob
================
*/
hhSoundLeadInController::hhSoundLeadInController() {
	leadInShader = NULL;
	loopShader = NULL;
	leadOutShader = NULL;

	startTime = 0;
	endTime = 0;

	owner = NULL;

	//rww - networking-related variables
	lastLeadChannel = SND_CHANNEL_ANY;
	lastLoopChannel = SND_CHANNEL_ANY;
	bPlaying = false;
	iLoopOnlyOnLocal = -1;
}

/*
================
hhSoundLeadInController::SetOwner

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::SetOwner( idEntity* ent ) {
	owner = ent;
}

/*
================
hhSoundLeadInController::WriteToSnapshot

HUMANHEAD: rww
================
*/
void hhSoundLeadInController::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits(lastLeadChannel, 8);
	msg.WriteBits(lastLoopChannel, 8);

	msg.WriteBits(owner.GetSpawnId(), 32);

	msg.WriteBits(bPlaying, 1);
}

/*
================
hhSoundLeadInController::ReadFromSnapshot

HUMANHEAD: rww
note that the methods used here are not failsafe given the range of functionality
within this class, and this logic may need to be adjusted on a per-case basis if
more instances are to be sync'd over the net using this method.
================
*/
void hhSoundLeadInController::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	lastLeadChannel = msg.ReadBits(8);
	lastLoopChannel = msg.ReadBits(8);

	owner.SetSpawnId(msg.ReadBits(32));

	bool nowPlaying = !!msg.ReadBits(1);
	if (nowPlaying != bPlaying) {
		if (nowPlaying) {
			StartSound(lastLeadChannel, lastLoopChannel);
		}
		else {
			StopSound(lastLeadChannel, lastLoopChannel);
		}
	}
}

/*
================
hhSoundLeadInController::SetLeadIn

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::SetLeadIn( const char* soundname ) {
	leadInShader = declManager->FindSound( soundname, false );
}

/*
================
hhSoundLeadInController::SetLoop

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::SetLoop( const char* soundname ) {
	loopShader = declManager->FindSound( soundname, false );
}

/*
================
hhSoundLeadInController::SetLeadOut

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::SetLeadOut( const char* soundname ) {
	leadOutShader = declManager->FindSound( soundname, false );
}

/*
================
hhSoundLeadInController::StartSound

HUMANHEAD: aob
================
*/
int hhSoundLeadInController::StartSound( const s_channelType leadChannel, const s_channelType loopChannel, int soundShaderFlags, bool broadcast ) {
	int length = 0;

	if( !owner.IsValid() ) {
		return 0;
	}

	if( loopShader ) {
		owner->StopSound( loopChannel, broadcast );
	}

	if( leadOutShader ) {
		owner->StopSound( leadChannel, broadcast );
	}

	if( leadInShader ) {
		owner->StartSoundShader( leadInShader, leadChannel, 0, broadcast, &length );
		StartFade( leadInShader, leadChannel, endTime, startTime, leadInShader->GetVolume(), length );

		startTime = gameLocal.GetTime();
		endTime = startTime + length;
	}

	if (iLoopOnlyOnLocal == -1 || iLoopOnlyOnLocal == gameLocal.localClientNum) { //rww - for spirit music and whatever else needs it
		owner->StartSoundShader( loopShader, loopChannel, 0, broadcast, NULL );
		StartFade( loopShader, loopChannel, startTime, endTime, loopShader->GetVolume(), length );
	}

	//rww - for networking
	lastLeadChannel = leadChannel;
	lastLoopChannel = loopChannel;
	bPlaying = true;

	return length;
}

/*
================
hhSoundLeadInController::StopSound

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::StopSound( const s_channelType leadChannel, const s_channelType loopChannel, bool broadcast ) {
	int length = 0;

	if( !owner.IsValid() ) {
		return;
	}

	if( leadInShader ) {
		owner->StopSound( leadChannel, broadcast );
	}

	if( loopShader ) {
		owner->StopSound( loopChannel, broadcast );
	}

	if( leadOutShader ) {
		owner->StartSoundShader( leadOutShader, leadChannel, 0, broadcast, &length );
		StartFade( leadOutShader, leadChannel, startTime, endTime, hhMath::Scale2dB(0.0f), length );

		startTime = gameLocal.GetTime();
		endTime = startTime + length;
	}

	//rww - for networking
	lastLeadChannel = leadChannel;
	lastLoopChannel = loopChannel;
	bPlaying = false;
}

/*
================
hhSoundLeadInController::CalculateScale

HUMANHEAD: aob
================
*/
float hhSoundLeadInController::CalculateScale( const float value, const float min, const float max ) {
	return hhUtils::CalculateScale( value, min, max );
}

/*
================
hhSoundLeadInController::StartFade

HUMANHEAD: aob
================
*/
void hhSoundLeadInController::StartFade( const idSoundShader* shader, const s_channelType channel, int start, int end, int finaldBVolume, int duration ) {
/*
	float scale = CalculateScale( gameLocal.GetTime(), start, end );

	scale *= hhMath::dB2Scale( shader->GetVolume() );
	owner->HH_SetSoundVolume( scale, channel );
	owner->FadeSoundShader( finaldBVolume, duration, channel );
*/
}

/*
================
hhSoundLeadInController::Save
================
*/
void hhSoundLeadInController::Save( idSaveGame *savefile ) const {
	savefile->WriteSoundShader( leadInShader );
	savefile->WriteSoundShader( loopShader );
	savefile->WriteSoundShader( leadOutShader );
	savefile->WriteInt( startTime );
	savefile->WriteInt( endTime );

	owner.Save( savefile );
}

/*
================
hhSoundLeadInController::Restore
================
*/
void hhSoundLeadInController::Restore( idRestoreGame *savefile ) {
	savefile->ReadSoundShader( leadInShader );
	savefile->ReadSoundShader( loopShader );
	savefile->ReadSoundShader( leadOutShader );
	savefile->ReadInt( startTime );
	savefile->ReadInt( endTime );

	owner.Restore( savefile );
}

/*
================
hhSoundLeadInController::SetLoopOnlyOnLocal
================
*/
void hhSoundLeadInController::SetLoopOnlyOnLocal(int loopOnlyOnLocal) {
	iLoopOnlyOnLocal = loopOnlyOnLocal;
}
