#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION(hhSound, hhJukeBoxSpeaker)
END_CLASS


const idEventDef EV_SetNumTracks("setNumTracks", "d");
const idEventDef EV_SetTrack("setTrack", "d");
const idEventDef EV_PlaySelected("playSelected", NULL);
const idEventDef EV_TrackOver("<trackover>", NULL);
const idEventDef EV_SetJukeboxVolume("setvolume", "f");

CLASS_DECLARATION(hhConsole, hhJukeBox)
	EVENT( EV_SetNumTracks,			hhJukeBox::Event_SetNumTracks)
	EVENT( EV_SetTrack,				hhJukeBox::Event_SetTrack)
	EVENT( EV_PlaySelected,			hhJukeBox::Event_PlaySelected)
	EVENT( EV_TrackOver,			hhJukeBox::Event_TrackOver)
	EVENT( EV_SetJukeboxVolume,		hhJukeBox::Event_SetVolume)
END_CLASS


void hhJukeBox::Spawn() {
	track = 1;
	currentHistorySample = 0;
	numTracks = spawnArgs.GetInt("numtracks", "1");
	volume = spawnArgs.GetFloat("volume");
	UpdateView();
}

void hhJukeBox::Save(idSaveGame *savefile) const {
	int i;

	savefile->WriteFloat( volume );
	savefile->WriteInt( track );
	savefile->WriteInt( numTracks );
	savefile->WriteInt( currentHistorySample );

	savefile->WriteInt( speakers.Num() );		// Saving of idList<idEntity*>
	for( i = 0; i < speakers.Num(); i++ ) {
		savefile->WriteObject(speakers[i]);
	}
}

void hhJukeBox::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadFloat( volume );
	savefile->ReadInt( track );
	savefile->ReadInt( numTracks );
	savefile->ReadInt( currentHistorySample );

	speakers.Clear();
	savefile->ReadInt( num );
	speakers.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>(speakers[i]) );
	}

	UpdateVolume();
}

void hhJukeBox::ConsoleActivated() {
	BecomeActive(TH_MISC3);
}

void hhJukeBox::UpdateView() {
	idUserInterface *gui = renderEntity.gui[0];

	if (gui) {
		gui->SetStateInt("track", track);
		gui->SetStateFloat("volume", volume);
	}
}

void hhJukeBox::ClearSpectrum() {
	idUserInterface *gui = renderEntity.gui[0];
	if (gui) {
		gui->SetStateFloat("amplitude", 0.0f);

		for (int ix=0; ix<10; ix++) {
			gui->SetStateFloat(va("amplitude%d", ix), 0.0f);
		}
		gui->StateChanged(gameLocal.time);
	}
}

void hhJukeBox::SetTrack(int newTrack) {
	track = idMath::ClampInt(1, numTracks, newTrack);
}

void hhJukeBox::PlayCurrentTrack() {
	const char *shaderName = spawnArgs.GetString(va("snd_song%d", track), NULL);
	const idSoundShader *shader = declManager->FindSound(shaderName);
	int time = 0;

	// Clear up any previous state, even if using another mixer at the time
	StopCurrentTrack();

	// In OpenAL, samples that are out of range pause instead of mute so the targetted speakers can get out of sync.
	if (shader && targets.Num() && !cvarSystem->GetCVarBool("s_useOpenAL")) {
		for (int ix=0; ix<targets.Num(); ix++) {
			if (targets[ix].IsValid()) {
				targets[ix]->StartSoundShader(shader, SND_CHANNEL_VOICE, 0, true, &time);
			}
		}
	}
	else {
		StartSound(va("snd_song%d", track), SND_CHANNEL_VOICE, 0, true, &time);
	}
	UpdateVolume();
	CancelEvents(&EV_TrackOver);
	PostEventMS(&EV_TrackOver, time + 500);
}

void hhJukeBox::StopCurrentTrack() {
	CancelEvents(&EV_TrackOver);
	ClearSpectrum();

	// Stop speakers and jukebox so no mixer switches can screw us up
	StopSound(SND_CHANNEL_VOICE, true);
	for (int ix=0; ix<targets.Num(); ix++) {
		if (targets[ix].IsValid()) {
			targets[ix]->StopSound(SND_CHANNEL_VOICE, true);
		}
	}
}

bool hhJukeBox::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("prevtrack") == 0) {
		SetTrack(track-1);
		UpdateView();
	}
	else if (token.Icmp("nexttrack") == 0) {
		SetTrack(track+1);
		UpdateView();
	}
	else if (token.Icmp("selecttrack") == 0) {
		BecomeActive(TH_MISC3);
		PlayCurrentTrack();
		UpdateView();
	}
	else if (token.Icmp("turnoff") == 0) {
		StopCurrentTrack();
		BecomeInactive(TH_MISC3);
		UpdateView();
	}
	else if (token.Icmp("turnon") == 0) {
		BecomeActive(TH_MISC3);
		UpdateView();
	}
	else if (token.Icmp("volumeup") == 0) {
		volume = idMath::ClampFloat(0.0f, 1.0f, volume + 0.02f);
		UpdateVolume();
		UpdateView();
	}
	else if (token.Icmp("volumedown") == 0) {
		volume = idMath::ClampFloat(0.0f, 1.0f, volume - 0.02f);
		UpdateVolume();
		UpdateView();
	}
	else {
		src->UnreadToken(&token);
		return false;
	}

	return true;
}

void hhJukeBox::UpdateEntityVolume(idEntity *ent) {
	ent->HH_SetSoundVolume(volume, SND_CHANNEL_VOICE);
}

void hhJukeBox::UpdateVolume() {
	if (targets.Num() && !cvarSystem->GetCVarBool("s_useOpenAL")) {
		for (int ix=0; ix<targets.Num(); ix++) {
			if (targets[ix].IsValid()) {
				UpdateEntityVolume(targets[ix].GetEntity());
			}
		}
	}
	else {
		UpdateEntityVolume(this);
	}
}

void hhJukeBox::Think() {

	hhConsole::Think();

	if (thinkFlags & TH_MISC3) {
		float amplitude = 0.0f;

		//TODO: This sampling really only needs to take place once/render
		if (refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying()) {
			amplitude = refSound.referenceSound->CurrentAmplitude();
		}
		else if (targets.Num() && targets[0].IsValid() && targets[0].GetEntity()->IsType(hhSound::Type) ) {
			amplitude = static_cast<hhSound*>(targets[0].GetEntity())->GetCurrentAmplitude(SND_CHANNEL_VOICE);
		}

		amplitude = idMath::ClampFloat(0.0f, 1.0f, amplitude);
		idUserInterface *gui = renderEntity.gui[0];		// Interface area
		if (gui) {
			gui->SetStateFloat("amplitude", amplitude);

			currentHistorySample = (currentHistorySample+1)%10;
			gui->SetStateFloat(va("amplitude%d", currentHistorySample), amplitude);
			gui->StateChanged(gameLocal.time);
		}
		gui = renderEntity.gui[1];						// Outer jukebox
		if (gui) {
			gui->SetStateFloat("amplitude", amplitude);
		}
	}
}

void hhJukeBox::Event_SetNumTracks(int newNumTracks) {
	numTracks = newNumTracks;
}

void hhJukeBox::Event_SetTrack(int newTrack) {
	SetTrack(newTrack);
	UpdateView();
}

void hhJukeBox::Event_PlaySelected() {
	PlayCurrentTrack();
	BecomeActive(TH_MISC3);
	UpdateView();
}

void hhJukeBox::Event_TrackOver() {
	ClearSpectrum();
	//loop to beginning
	if ( track+1 > numTracks ) {
		SetTrack(1);
	} else {
		SetTrack(track+1);
	}
	PlayCurrentTrack();
	UpdateView();
}

void hhJukeBox::Event_SetVolume(float vol) {
	volume = idMath::ClampFloat(0.0f, 1.0f, vol);
	UpdateVolume();
	UpdateView();
}


