#pragma once

#include "platformutilties.h"

#ifdef EPICSTORE

#include <functional>
#include "eos_Windows_base.h"
#include "eos_sdk.h"
#include "eos_achievements.h"

class CEpicUtilities : public IPlatformUtilities
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
private:
	// static since called from callbacks
	static void EOS_CALL UnlockAchievementsReceivedCallbackFn(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* Data);
	static const char* ProductUserIDToString(EOS_ProductUserId InAccountId);
		
	void ConnectLogin(EOS_EpicAccountId UserId);

	bool 					bEpicInitSuccessful=false;
	bool					bEpicOverlayEnabeled = true;
	EOS_InitializeOptions	epicInitOptions = {};
	EOS_Platform_Options	platformOptions = {};
	static const uint		maxEpicCLALength = 64;
	char					epicUserId[maxEpicCLALength];
	char					epicLocale[maxEpicCLALength];
	char					epicAUTH_PASSWORD[maxEpicCLALength];
	
};

#endif // EPICSTORE
