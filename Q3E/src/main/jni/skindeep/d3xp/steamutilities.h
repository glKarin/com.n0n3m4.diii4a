#pragma once

// SM: We need to include platform.h here so that STEAM can be defined
#include "sys/platform.h"
#include "idlib/Str.h"

#include "platformutilties.h"

#ifdef STEAM
//#define _STAT_ID( id,type,name ) { id, type, name, 0, 0, 0, 0 }
#include "steam/steam_api.h"

#define MAX_WORKSHOP_ITEMS 256

class CSteamUtilities : public IPlatformUtilities
{
public:
	CSteamUtilities();
	~CSteamUtilities();
	
	bool		InitializeSteam() override;
	bool		IsSteamInitialized() override;

	void		RunCallbacks() override;

	bool		SetAchievement(const char* achievementName) override;
	void		ResetAchievements() override;

	bool		SteamCloudSave(const char* savefile, const char* descFile) override;
	bool		SteamCloudSaveConfig(const char* configFileName) override;
	bool		SteamCloudDeleteFile(const char* filename) override;
	bool		SteamCloudLoad() override;

	void		OpenSteamOverlaypage(const char* pageURL) override;
	void		OpenSteamOverlaypageStore() override;

	int			GetSteamWorkshopAmount() override;
	const char* GetWorkshopPathAtIndex(int index) override;

	const char* GetSteamLanguage() override;
	idStr		GetSteamID() override;

	void		SetSteamRichPresence(const char* text) override;
	void		SetSteamTimelineEvent(const char* icon) override;

	void		SteamShutdown() override;

	bool		IsOnSteamDeck() override;
	bool		IsInBigPictureMode() override;
	
private:

	STEAM_CALLBACK(CSteamUtilities, OnGameOverlayActivated, GameOverlayActivated_t);
	
	bool 				steamInitSuccessful;

	PublishedFileId_t	modFileIDs[MAX_WORKSHOP_ITEMS];
	bool				cloudEnabled = false;
};



#endif