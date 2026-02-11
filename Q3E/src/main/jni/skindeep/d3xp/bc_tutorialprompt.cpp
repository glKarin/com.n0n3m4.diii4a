#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "bc_tutorialprompt.h"

//Handle the logic for the tutorial button prompts.

#define MAX_KEYPROMPTS 8


//Script calls
const idEventDef EV_Tut_SetText("SetText", "ss");



CLASS_DECLARATION(idEntity, idTutorialPrompt)
EVENT(EV_Tut_SetText, idTutorialPrompt::SetText)
END_CLASS

idTutorialPrompt::idTutorialPrompt(void)
{
	promptActive = false;
	currentText = "";
}

idTutorialPrompt::~idTutorialPrompt(void)
{
}

void idTutorialPrompt::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( promptActive ); // bool promptActive
	savefile->WriteString( currentText ); // idString currentText
}

void idTutorialPrompt::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( promptActive ); // bool promptActive
	savefile->ReadString( currentText ); // idString currentText
}


void idTutorialPrompt::Spawn(void)
{
	//BecomeActive(TH_THINK);
}

//void idTutorialPrompt::Think(void)
//{
//}


//Set the text on the tutorial prompt
void idTutorialPrompt::SetText(const char* text, const char* keys)
{
	idStr _newText = text;

	if (_newText.Length() <= 0)
	{
		//Empty text. Turn off the prompt.
		if (promptActive)
		{
			gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("onTutorialpromptOff");
			promptActive = false;
		}
		return;
	}

	
	if (!promptActive)
	{
		//prompt is OFF. So, activate it.
		DoPopup(text, keys);
		return;
	}

	//The prompt is currently ON. We do an extra check: if the prompt is already displaying the requested text, then
	//we do nothing. (so that we avoid doing a fanfare animation that just displays the same text)

	if (idStr::Icmp(text, currentText.c_str()) == 0)
	{
		//we're already displaying this text. So, don't do anything.
		return;
	}

	DoPopup(text, keys);
}

void idTutorialPrompt::DoPopup(const char* text, const char* keys)
{
	idStr displayText;
	idStr keysText = keys;
	if (keysText.Length() <= 0)
	{
		//No key prompts. Just display the text normally.
		displayText = common->GetLanguageDict()->GetString(text);

		//Event_SetGuiParm("tutorialbind", "");
		gameLocal.GetLocalPlayer()->hud->SetStateString("tutorialbind", "");
	}
	else
	{
		//There are key prompts in this. Parse it.
		//idStrList keyList = keysText.Split(',', true);
		//
		//for (int i = 0; i < keyList.Num(); i++)
		//{
		//	//Get the keybind name.
		//	keyList[i] = gameLocal.GetKeyFromBinding(keyList[i].c_str());
		//}
		//
		//if (keyList.Num() < MAX_KEYPROMPTS)
		//{
		//	int keypromptsToAdd = MAX_KEYPROMPTS - keyList.Num();
		//	for (int i = 0; i < keypromptsToAdd; i++)
		//	{
		//		//Just fill it up with empty entries, so that we can safely call idStr::Format
		//		keyList.Append("");
		//	}
		//}
		//
		//idStr localizedText = common->GetLanguageDict()->GetString(text);
		//displayText = idStr::Format(localizedText.c_str(), keyList[0].c_str(), keyList[1].c_str(), keyList[2].c_str(), keyList[3].c_str(), keyList[4].c_str(), keyList[5].c_str(), keyList[6].c_str(), keyList[7].c_str());

		displayText = common->GetLanguageDict()->GetString(text);

		idStrList keyList = keysText.Split(',', true);
// 		for (int i = 0; i < keyList.Num(); i++)
// 		{
// 			//Get the keybind name.
// 			keyList[i] = gameLocal.GetKeyFromBinding(keyList[i].c_str());
// 		}

//		Event_SetGuiParm("tutorialbind", keyList[0].c_str());
		gameLocal.GetLocalPlayer()->hud->SetStateString("tutorialbind", keyList[0].c_str());
	}

	gameLocal.GetLocalPlayer()->hud->SetStateString("tutorialprompttext", displayText.c_str()); //set the prompt text.
	gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("onTutorialpromptOn"); //make the fanfare happen.
	currentText = text;
	promptActive = true;

	StartSound("snd_appear", SND_CHANNEL_ANY);
}