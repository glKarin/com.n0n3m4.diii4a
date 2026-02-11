#pragma once

#include "platformutilties.h"


class CNullUtilities : public IPlatformUtilities
{
public:	
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
	
};
