#pragma once

#include "sys/platform.h"
#include "idlib/Str.h"

// SM: Yeah, we really should rename many functions to not have "Steam" in them
// but don't want to do a massive rename right now

class IPlatformUtilities
{
public:
	virtual			~IPlatformUtilities() { }
	virtual bool	InitializeSteam() = 0;
	virtual bool	IsSteamInitialized() = 0;

	virtual void	RunCallbacks() = 0;

	virtual bool	SetAchievement(const char* achievementName) = 0;
	virtual void	ResetAchievements() = 0;

	virtual bool	SteamCloudSave(const char* savefile, const char* descFile) = 0;
	virtual bool	SteamCloudSaveConfig(const char* configFileName) = 0;
	virtual bool	SteamCloudDeleteFile(const char* filename) = 0;
	virtual bool	SteamCloudLoad() = 0;

	virtual void	OpenSteamOverlaypage(const char* pageURL) = 0;
	virtual void	OpenSteamOverlaypageStore() = 0;

	virtual int		GetSteamWorkshopAmount() = 0;
	virtual const char* GetWorkshopPathAtIndex(int index) = 0;

	virtual const char* GetSteamLanguage() = 0;
	virtual idStr	GetSteamID() = 0;

	virtual void	SetSteamRichPresence(const char* text) = 0;
	virtual void	SetSteamTimelineEvent(const char* icon) = 0;

	virtual void	SteamShutdown() = 0;

	virtual bool	IsOnSteamDeck() = 0;
	virtual bool	IsInBigPictureMode() = 0;
	virtual bool	ShouldDefaultToController() { return IsOnSteamDeck() || IsInBigPictureMode(); }
};
