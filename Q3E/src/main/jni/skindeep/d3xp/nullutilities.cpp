//If STEAM and EPICSTORE are not defined, then this "null" platform is the fallback.
//See: epicutilities.cpp , steamutilities.cpp

#pragma hdrstop

#include "nullutilities.h"


//Try to initialize steam.
bool CNullUtilities::InitializeSteam()
{
	return false;
}

//Returns whether steam has successfully initialized.
bool CNullUtilities::IsSteamInitialized()
{
	return false;
}

//Call this when shutting down the game.
void CNullUtilities::SteamShutdown()
{
	
}


bool CNullUtilities::IsOnSteamDeck()
{
	return false;
}

bool CNullUtilities::IsInBigPictureMode()
{
	return false;
}

//This should be called every frame for steam callbacks.
void CNullUtilities::RunCallbacks()
{

}

//Save savegame file to steam cloud.
bool CNullUtilities::SteamCloudSave(const char* savefile, const char* descFile)
{
#ifndef DEMO
	return false;
#else
	//if it's the demo build, don't do the cloud save.
	return false;
#endif // DEMO
}

bool CNullUtilities::SteamCloudSaveConfig(const char* configFileName)
{
#ifndef DEMO
	return false;
#else
	//if it's demo, don't do cloud save.
	return false;
#endif // DEMO
}

//Delete a file from the steam cloud.
bool CNullUtilities::SteamCloudDeleteFile(const char* filename)
{
	return false;
}

bool CNullUtilities::SteamCloudLoad()
{
#ifndef DEMO
	return false;
#else
	//if it's demo, don't do cloud load.
	return false;
#endif // DEMO
}

//Opens a steam overlay page URL.
void CNullUtilities::OpenSteamOverlaypage(const char* pageURL)
{

}

//Open store page of current game.
void CNullUtilities::OpenSteamOverlaypageStore()
{

}

//Set a Steam achievement.
bool CNullUtilities::SetAchievement(const char *achievementName)
{
	return false;
}

void CNullUtilities::ResetAchievements()
{

}

int CNullUtilities::GetSteamWorkshopAmount()
{
	return 0;
}

const char* CNullUtilities::GetWorkshopPathAtIndex(int index)
{
	return "";
}

//Note: This returns the language set up in the Steam game properties (NOT the Steam interface language).
//In Steam, this is accessed via right-click game > Properties > General > Language
//In the Steam backend, the languages are set in Steampipe > Depots > Managing Base Languages
const char* CNullUtilities::GetSteamLanguage()
{
	return "english";
}

idStr CNullUtilities::GetSteamID()
{
	return "12456";
}

//NOTE: this requires localized text in the steam backend. The text should be "#nameOfLocString"
void CNullUtilities::SetSteamRichPresence(const char* text)
{

}

void CNullUtilities::SetSteamTimelineEvent(const char* icon)
{

}
