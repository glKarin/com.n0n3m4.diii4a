#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "bc_cassetteplayer.h"


CLASS_DECLARATION(idStaticEntity, idCassettePlayer)
END_CLASS

#define MAXTRACKS 10

#define IMPULSE_STOP 60
#define IMPULSE_PLAY 70

idCassettePlayer::idCassettePlayer(void)
{
	isFrobbable = false;
	soundshaderList.Clear();

	tracklist = uiManager->AllocListGUI();
}

idCassettePlayer::~idCassettePlayer(void)
{
	uiManager->FreeListGUI(tracklist);
}

void idCassettePlayer::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	//isFrobbable = true;
	fl.takedamage = false; //let's make our lives easier and make cassette tapes invincible

	// SM: Don't do this at 0 or it will call this before the player is created
	PostEventMS(&EV_PostSpawn, 100);
}

int CassetteSort(const int* a, const int* b)
{
	return *a >= *b;
}

void idCassettePlayer::Event_PostSpawn(void)
{
	//Populate the list with cassette tapes the player has obtained.
	tracklist->Config(renderEntity.gui[0], "tracklist");
	tracklist->Clear();

	//tapesCollected is a list of integers.
	//these integers are the index numbers of the tapes the player has collected.
	static idList<int> tapesCollected;
	tapesCollected = gameLocal.GetLocalPlayer()->tapesCollected;
	tapesCollected.Sort(CassetteSort);

	//TODO: sort the "tapesCollected" list so that the tracks are in order.
	//TODO: remove any duplicate tape indexes???

	//Iterate over all collected cassettes.
	int tracklistIndex = 0;
	for (int i = 0; i < tapesCollected.Num(); i++)
	{
		int cassetteIndexNumber = tapesCollected[i];
		idStr cassetteDefName = idStr::Format("info_cassettetape_%d", cassetteIndexNumber);
		const idDeclEntityDef* cassetteDef = gameLocal.FindEntityDef(cassetteDefName.c_str());

		if (!cassetteDef)
		{
			common->Warning("Couldn't find cassette definition '%s'\n", cassetteDefName.c_str());
			continue; //Couldn't find it. Skip it..
		}

		//Iterate over how many tracks does this cassette include.
		for (int k = 1; k < MAXTRACKS; k++)
		{
			idStr trackShaderKey = idStr::Format("snd_track_%d", k);
			idStr trackShader = cassetteDef->dict.GetString(trackShaderKey.c_str());

			idStr trackNameKey = idStr::Format("name_track_%d", k);
			idStr trackName = cassetteDef->dict.GetString(trackNameKey.c_str());

			if (trackShader.Length() <= 0 || trackName.Length() <= 0)
				break; //no more tracks left. exit

			tracklist->Add(tracklistIndex, common->GetLanguageDict()->GetString(trackName.c_str()));
			soundshaderList.Append(trackShader);
			tracklistIndex++;
		}
	}


	Event_SetGuiInt("tapescollected", tapesCollected.Num());
	Event_SetGuiInt("tapestotal", spawnArgs.GetInt("totaltapes")); //total tapes is just manually set via a spawnarg


	if (tracklist->Num() <= 0)
	{
		Event_GuiNamedEvent(1, "no_tracks");
	}
}

void idCassettePlayer::Save(idSaveGame *savefile) const
{
	tracklist->Save( savefile ); //  idListGUI* tracklist
	SaveFileWriteArray( soundshaderList, soundshaderList.Num(), WriteString ); //  idList<idStr> soundshaderList
}

void idCassettePlayer::Restore(idRestoreGame *savefile)
{
	tracklist->Restore( savefile ); //  idListGUI* tracklist
	SaveFileReadArray( soundshaderList, ReadString ); //  idList<idStr> soundshaderList
}

void idCassettePlayer::Think(void)
{
	idStaticEntity::Think();
}

//bool idCassettePlayer::DoFrob(int index, idEntity * frobber)
//{
//	return true;
//}

void idCassettePlayer::DoGenericImpulse(int index)
{
	if (index == IMPULSE_PLAY)
	{
		int selNum = tracklist->GetSelection(nullptr, 0);

		//if player has tracks but hasn't selected anything, then select the first track.
		if (tracklist->Num() > 0 && selNum <= -1)
		{
			tracklist->SetSelection(0);
			selNum = 0;
		}

		if (selNum <= -1 || selNum >= soundshaderList.Num())
		{
			//If no track is selected, then error out.
			StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			Event_GuiNamedEvent(1, "no_tracks");

			return;
		}

		idStr shaderToPlay = soundshaderList[selNum];
		if (!StartSoundShader(declManager->FindSound(shaderToPlay.c_str()), SND_CHANNEL_BODY3, 0, false, NULL))
		{
			StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
			return;
		}

		char listItem[2048];
		tracklist->GetSelection(listItem, 2048);
		Event_SetGuiParm("currenttrack", listItem);

		Event_GuiNamedEvent(1, "startplaying");
	}
	else if (index == IMPULSE_STOP)
	{
		StopSound(SND_CHANNEL_BODY3);
	}
}