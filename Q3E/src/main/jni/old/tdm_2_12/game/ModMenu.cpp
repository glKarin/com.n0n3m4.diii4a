/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop

#include <string>



#include "ModMenu.h"
#include "Shop/Shop.h"
#include "Objectives/MissionData.h"
#include "declxdata.h"
#include "ZipLoader/ZipLoader.h"
#include "Missions/MissionManager.h"

#ifdef _WINDOWS
#include <process.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

CModMenu::CModMenu() :
	_modTop(0)
{}

#include <StdFilesystem.h>
namespace fs = stdext;

// Handle mainmenu commands
void CModMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "refreshMissionList")
	{
		int numNewMods = gameLocal.m_MissionManager->GetNumNewMods();

		if (numNewMods > 0)
		{
			// Update mission DB records
			gameLocal.m_MissionManager->RefreshMetaDataForNewFoundMods();

			gui->SetStateString("newFoundMissionsText", common->Translate( "#str_02143" ) ); // New missions available
			gui->SetStateString("newFoundMissionsList", gameLocal.m_MissionManager->GetNewFoundModsText());
			gui->HandleNamedEvent("OnNewMissionsFound");

			gameLocal.m_MissionManager->ClearNewModList();
		}

		gameLocal.m_MissionManager->ReloadModList();

		// Update the GUI state
		UpdateGUI(gui);
	}
	else if (cmd == "mainMenuStartup")
	{
		gui->SetStateBool("curModIsCampaign", gameLocal.m_MissionManager->CurrentModIsCampaign());
		//stgatilov: we need to set cvar early, otherwise state switching code will "play nosound"
		gui->SetStateBool("menu_bg_music", cv_tdm_menu_music.GetBool());
	}
	else if (cmd == "loadModNotes")
	{
		// Get selected mod
    int modIndex = gui->GetStateInt("missionList_sel_0", "-1");

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		// Load the readme.txt contents, if available
		gui->SetStateString("ModNotesText", info != NULL ? info->GetModNotes() : "");
	}
	else if (cmd == "onMissionSelected")
	{
		UpdateSelectedMod(gui);
	}
	/*else if (cmd == "eraseSelectedModFromDisk")
	{
		int modIndex = gui->GetStateInt("missionList_sel_0", "-1");

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		if (info != NULL)
		{
			gameLocal.m_MissionManager->CleanupModFolder(info->modName);
		}

		gui->HandleNamedEvent("OnSelectedMissionErasedFromDisk");

		// Update the selected mission
		UpdateSelectedMod(gui);
	}*/
	else if (cmd == "update")
	{
		gameLocal.Error("Deprecated update method called by main menu.");
	}
	else if (cmd == "onClickInstallSelectedMission")
	{
		// Get selected mod
		int modIndex = gui->GetStateInt("missionList_sel_0", "-1");

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		if (info == NULL) return; // sanity check

		// Issue the named command to the GUI
		gui->SetStateString("modInstallProgressText", common->Translate( "#str_02504" ) + info->displayName); // "Installing Mission Package\n\n"
	}
	else if (cmd == "installSelectedMission")
	{
		// Get selected mod
    int modIndex = gui->GetStateInt("missionList_sel_0", "-1");

		CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

		if (info == NULL) return; // sanity check

		if (!PerformVersionCheck(info, gui))
		{
			return; // version check failed
		}

		InstallMod(info, gui);
	}
	else if (cmd == "darkmodRestart")
	{
		RestartEngine();
	}
	else if (cmd == "briefing_show")
	{
		// Display the briefing text
		_briefingPage = 1;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "briefing_scroll_down_request")
	{
		// Display the next page of briefing text
		_briefingPage++;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "briefing_scroll_up_request")
	{
		// Display the previous page of briefing text
		_briefingPage--;
		DisplayBriefingPage(gui);
	}
	else if (cmd == "uninstallMod")
	{
		UninstallMod(gui);
	}
	else if (cmd == "startSelect") // grayman #2933 - save mission start position
	{
		gameLocal.m_StartPosition = gui->GetStateString("startSelect", "");
	}
}

void CModMenu::UpdateSelectedMod(idUserInterface* gui)
{
	// Get selected mod
  int modIndex = gui->GetStateInt("missionList_sel_0", "-1");

	CModInfoPtr info = gameLocal.m_MissionManager->GetModInfo(modIndex);

	if (info != NULL)
	{
		bool missionIsCurrentlyInstalled = gameLocal.m_MissionManager->GetCurrentModName() == info->modName;

    idStr name = common->Translate( info != NULL ? info->displayName : "");
    gui->SetStateString("mod_name", name);
    gui->SetStateString("mod_desc", common->Translate( info != NULL ? info->description : ""));
    gui->SetStateString("mod_author", info != NULL ? info->author : "");
    gui->SetStateString("mod_image", info != NULL ? info->image : "");
		
		// Don't display the install button if the mod is already installed
		gui->SetStateBool("installModButtonVisible", !missionIsCurrentlyInstalled);
		gui->SetStateBool("hasModNoteButton", info->HasModNotes());

		// Set the mod size info
		std::size_t missionSize = info->GetModFolderSize();
		idStr missionSizeStr = info->GetModFolderSizeString();
		gui->SetStateString("selectedModSize", missionSize > 0 ? missionSizeStr : "-");

		gui->SetStateBool("eraseSelectedModButtonVisible", missionSize > 0 && !missionIsCurrentlyInstalled);
		
		// 07208: "You're about to delete the gameplay contents of the mission folder from your disk (mainly savegames and screenshots):"
		// 07209: "Note that the mission PK4 itself in your darkmod/fms/ folder will not be removed by this operation, you'll still able to play the mission."
		idStr eraseMissionText = va( idStr( common->Translate( "#str_07208" ) ) + "\n\n%s\n\n" +
					     common->Translate( "#str_07209" ), info->GetModFolderPath().c_str() );
		gui->SetStateString("eraseMissionText", eraseMissionText);

		gui->SetStateString("selectedModCompleted", info->GetModCompletedString());
		gui->SetStateString("selectedModLastPlayDate", info->GetKeyValue("last_play_date", "-"));
		gui->SetStateBool("modToInstallVisible", true);
	}
	else
	{
		gui->SetStateBool("modToInstallVisible", false);
		gui->SetStateBool("installModButtonVisible", false);
		gui->SetStateString("selectedModSize", "0 Bytes");
		gui->SetStateBool("eraseSelectedModButtonVisible", false);
		gui->SetStateBool("hasModNoteButton", false);
	}
}

// Displays the current page of briefing text
void CModMenu::DisplayBriefingPage(idUserInterface* gui)
{
	// look up the briefing xdata, which is in "maps/<map name>/mission_briefing"
	idStr briefingData = idStr("maps/") + gameLocal.m_MissionManager->GetCurrentStartingMap() + "/mission_briefing";

	gameLocal.Printf("DisplayBriefingPage: briefingData is %s\n", briefingData.c_str());

	// Load the XData declaration
	const tdmDeclXData* xd = static_cast<const tdmDeclXData*>(
		declManager->FindType(DECL_XDATA, briefingData, false)
	);

	const char* briefing = "";
	bool scrollDown = false;
	bool scrollUp = false;

	if (xd != NULL)
	{
		gameLocal.Printf("DisplayBriefingPage: xdata found.\n");

		// get page count from xdata (tels: and if nec., translate it #3193)
		idStr strNumPages = common->Translate( xd->m_data.GetString("num_pages") );
		if (!strNumPages.IsNumeric())
		{
			gameLocal.Warning("DisplayBriefingPage: num_pages '%s' is not numeric!", strNumPages.c_str());
		}
		else
		{
			int numPages = atoi(strNumPages.c_str());

			gameLocal.Printf("DisplayBriefingPage: numPages is %d\n", numPages);

			// ensure current page is between 1 and page count, inclusive
			_briefingPage = idMath::ClampInt(1, numPages, _briefingPage);

			// load up page text
			idStr page = va("page%d_body", _briefingPage);

			gameLocal.Printf("DisplayBriefingPage: current page is %d\n", _briefingPage);

			// Tels: Translate it properly
			briefing = common->Translate( xd->m_data.GetString(page) );

			// set scroll button visibility
			scrollDown = numPages > _briefingPage;
			scrollUp = _briefingPage > 1;
		}
	}
	else
	{
		gameLocal.Warning("DisplayBriefingPage: Could not find briefing xdata: %s", briefingData.c_str());
	}

	// update GUI
	gui->SetStateString("BriefingText", briefing);
	gui->SetStateBool("ScrollDownVisible", scrollDown);
	gui->SetStateBool("ScrollUpVisible", scrollUp);

}

void CModMenu::UpdateGUI(idUserInterface* gui)
{
  const int num_mods = gameLocal.m_MissionManager->GetNumMods();
  CModInfoPtr info;
  for (int index = 0; index < num_mods; ++index)
  {
    info = gameLocal.m_MissionManager->GetModInfo(index);
  
    idStr name = common->Translate( info != NULL ? info->displayName : "");
    // alexdiru #4499
		/*
		// grayman #3110
		idStr prefix = "";
		idStr suffix = "";
		common->GetI18N()->MoveArticlesToBack( name, prefix, suffix );
		if ( !suffix.IsEmpty() )
		{
			// found, remove prefix and append suffix
			name.StripLeadingOnce( prefix.c_str() );
			name += suffix;
		}
		*/

    name += "\t";
    if (info->ModCompleted())
    {
      name += "mtr_complete";
    }
    gui->SetStateString( va("missionList_item_%d", index), name);
  }

  CModInfoPtr curModInfo = gameLocal.m_MissionManager->GetCurrentModInfo();
  gui->SetStateBool("hasCurrentMod", curModInfo != NULL);
  gui->SetStateString("currentModName", common->Translate( curModInfo != NULL ? curModInfo->displayName : "#str_02189" )); // <No Mission Installed>
  gui->SetStateString("currentModDesc", common->Translate( curModInfo != NULL ? curModInfo->description : "" )); 
  gui->StateChanged(gameLocal.time);
}

bool CModMenu::PerformVersionCheck(const CModInfoPtr& mission, idUserInterface* gui)
{
	// Check the required TDM version of this FM
	if (CompareVersion(TDM_VERSION_MAJOR, TDM_VERSION_MINOR, mission->requiredMajor, mission->requiredMinor) == OLDER)
	{
		gui->SetStateString("requiredVersionCheckFailText", 
			// "Cannot install this mission, as it requires\n%s v%d.%02d.\n\nYou are running v%d.%02d. Please run the tdm_update application to update your installation.",
			va( common->Translate( "#str_07210" ),
			GAME_VERSION, mission->requiredMajor, mission->requiredMinor, TDM_VERSION_MAJOR, TDM_VERSION_MINOR));

		gui->HandleNamedEvent("OnRequiredVersionCheckFail");

		return false;
	}

	return true; // version check passed
}

void CModMenu::InstallMod(const CModInfoPtr& mod, idUserInterface* gui)
{
	assert(mod != NULL);
	assert(gui != NULL);

	// Perform the installation
	CMissionManager::InstallResult result = gameLocal.m_MissionManager->InstallMod(mod->modName);

	if (result != CMissionManager::INSTALLED_OK)
	{
		idStr msg;

		switch (result)
		{
		case CMissionManager::COPY_FAILURE:
			msg = common->Translate( "#str_02010" ); // Could not copy files...
			break;
		default:
			msg = common->Translate( "#str_02011" ); // No further explanation available. Well, this was kind of unexpected.
		};

		// Feed error messages to GUI
		gui->SetStateString("modInstallationFailedText", msg);
		gui->HandleNamedEvent("OnModInstallationFailed");
	}
	else
	{
		gui->HandleNamedEvent("OnModInstallationFinished");
	}
}

void CModMenu::UninstallMod(idUserInterface* gui)
{
	gameLocal.m_MissionManager->UninstallMod();

	gui->HandleNamedEvent("OnModUninstallFinished");
}

void CModMenu::RestartEngine()
{
	// We restart the game by issuing a restart engine command only, this activates any newly installed mod
	cmdSystem->SetupReloadEngine(idCmdArgs());
}
