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



#include "DownloadMenu.h"
#include "Missions/MissionManager.h"
#include "Missions/Download.h"
#include "Missions/DownloadManager.h"

namespace
{
	inline const char* GetPlural(int num, const char* singular, const char* plural)
	{
		return (num == 1) ? singular : plural;
	}
}



void CDownloadMenu::HandleCommands(const idStr& cmd, idUserInterface* gui)
{
	if (cmd == "mainmenu_heartbeat")
	{
		// Update download progress
		if (_selectedMods.Num() > 0)
		{
			UpdateDownloadProgress(gui);
			UpdateGUI(gui);
		}
		
		// Do we have a pending mission list request?
		if (gameLocal.m_MissionManager->IsDownloadableModsRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessReloadDownloadableModsRequest();
			
			switch (status)
			{
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = common->Translate( "#str_02140" );	// Unable to contact Mission Archive
					msg.message = common->Translate( "#str_02007" );	// Cannot connect to server.
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::MALFORMED:
				{
					gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

					// Issue a failure message
					gameLocal.Printf("Server response is incorrect\n");

					GuiMessage msg;
					msg.title = common->Translate( "#str_02147" );	// Malformed response
					msg.message = common->Translate( "#str_02138" );	// The server returned wrong information.\nPlease report to maintainers.
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

					
					UpdateDownloadProgress(gui);
					UpdateGUI(gui);
				}
				break;

				default: break;
			};
		}

		// Process pending details download request
		if (gameLocal.m_MissionManager->IsModDetailsRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessReloadModDetailsRequest();

			switch (status)
			{
				case CMissionManager::MALFORMED:
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onDownloadableMissionDetailsDownloadFailed"); // hide progress dialog

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = common->Translate( "#str_02008" );	// Mission Details Download Failed
					msg.message = common->Translate( "#str_02009" );	// Failed to download the details XML file.
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					gui->HandleNamedEvent("onDownloadableMissionDetailsLoaded"); // hide progress dialog

					UpdateModDetails(gui);
					UpdateScreenshotItemVisibility(gui);
				}
				break;

				default: break;
			};
		}

		// Process pending screenshot download request
		if (gameLocal.m_MissionManager->IsMissionScreenshotRequestInProgress())
		{
			CMissionManager::RequestStatus status = 
				gameLocal.m_MissionManager->ProcessMissionScreenshotRequest();

			switch (status)
			{
				case CMissionManager::MALFORMED:
				case CMissionManager::FAILED:
				{
					gui->HandleNamedEvent("onFailedToDownloadScreenshot");

					// Issue a failure message
					gameLocal.Printf("Connection Error.\n");

					GuiMessage msg;
					msg.title = common->Translate( "#str_02002" ); // "Connection Error"
					msg.message = common->Translate( "#str_02139" ); // "Failed to download the screenshot file."
					msg.type = GuiMessage::MSG_OK;
					msg.okCmd = "close_msg_box";

					gameLocal.AddMainMenuMessage(msg);
				}
				break;

				case CMissionManager::SUCCESSFUL:
				{
					// Load data into GUI
					// Get store "next" number from the GUI
					int nextScreenNum = gui->GetStateInt("av_mission_next_screenshot_num");

					UpdateNextScreenshotData(gui, nextScreenNum);

					const char* sNext = gui->GetStateString("av_mission_next_screenshot");
					DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Download Finished, Fading: %s.", sNext);

					// Ready to fade
					gui->HandleNamedEvent("onStartFadeToNextScreenshot");
				}
				break;

				default: break;
			};
		}
	}
	else if (cmd == "refreshavailablemissionlist")
	{
		if (!cv_tdm_allow_http_access.GetBool() || gameLocal.m_HttpConnection == NULL)
		{
			gui->HandleNamedEvent("onAvailableMissionsRefreshed"); // hide progress dialog

			// HTTP Access disallowed, display message
			gameLocal.Printf("HTTP requests disabled, cannot check for available missions.\n");

			GuiMessage msg;
			msg.type = GuiMessage::MSG_OK;
			msg.okCmd = "close_msg_box";
			msg.title = common->Translate( "#str_02140" ); // "Unable to contact Mission Archive"
			msg.message = common->Translate( "#str_02141" ); // "HTTP Requests have been disabled,\n cannot check for available missions."

			gameLocal.AddMainMenuMessage(msg);

			return;
		}

		// Clear data before updating the list
		gui->DeleteStateVar("downloadSelectedList_item_0");
		
		_selectedMods.Clear();
		UpdateDownloadProgress(gui);
		UpdateGUI(gui);
		

		// Start refreshing the list, will be handled in mainmenu_heartbeat
		if (gameLocal.m_MissionManager->StartReloadDownloadableMods() == -1)
		{
			gameLocal.Error("No URLs specified to download the mission list XML.");
		}
	}
	else if (cmd == "ondownloadablemissionselected")
	{
		int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

		// Update mission details
		const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

		if (missionIndex >= mods.Num() || missionIndex < 0) return;

		gui->SetStateString("av_mission_title", mods[missionIndex]->title);
		gui->SetStateString("av_mission_author", mods[missionIndex]->author);
		gui->SetStateString("av_mission_release_date", mods[missionIndex]->releaseDate);
		gui->SetStateString("av_mission_type", mods[missionIndex]->type == DownloadableMod::Multi ? 
			common->Translate("#str_04353") : // Campaign
			common->Translate("#str_04352")); // Single Mission
		gui->SetStateString("av_mission_version", va("%d", mods[missionIndex]->version));
		gui->SetStateString("av_mission_size", va("%0.1f %s", mods[missionIndex]->sizeMB, common->Translate( "#str_02055" )));	// MB

		gui->SetStateBool("av_mission_details_visible", true);
	}
	else if (cmd == "onselectmissionfordownload")
	{
		int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

    if (missionIndex < 0) return;

		_selectedMods.AddUnique(missionIndex);

		gui->SetStateBool("av_mission_details_visible", false);

		UpdateGUI(gui);
	}
	// #4492(Obsttorte)
	else if (cmd == "onselectallmissionsfordownload")
	{
		for (int i = 0; i < gameLocal.m_MissionManager->GetDownloadableMods().Num(); i++)
		{
			_selectedMods.AddUnique(i);
		}
		gui->SetStateBool("av_mission_details_visible", false);
		UpdateGUI(gui);
	}
	else if (cmd == "ondeselectmissionfordownload")
	{
		int index = gui->GetStateInt("downloadSelectedList_sel_0", "-1");

		if (index < 0) return;

		if (gui->GetStateBool("mission_download_in_progress") == 1){//Cancel a download in progress
			if (gameLocal.m_DownloadManager->GetDownload(_downloads[_selectedMods[index]].missionDownloadId)->GetStatus() == CDownload::/*DownloadStatus::*/SUCCESS) // grayman - make linux compiler happy
				return;//this download has been completed, you won't be able to cancel it.
			if (_downloads[_selectedMods[index]].l10nPackDownloadId != -1){//we were downloading a localization pack
				CDownloadPtr l10ndownload = gameLocal.m_DownloadManager->GetDownload(_downloads[_selectedMods[index]].l10nPackDownloadId);
				l10ndownload->Stop(true);
				gameLocal.m_DownloadManager->RemoveDownload(_downloads[_selectedMods[index]].l10nPackDownloadId);
				}
			
			CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(_downloads[_selectedMods[index]].missionDownloadId);
			download->Stop(true);
			gameLocal.m_DownloadManager->RemoveDownload(_downloads[_selectedMods[index]].missionDownloadId);
		}
		_selectedMods.Remove(_selectedMods[index]);

		
		if (_selectedMods.Num()<1 ) UpdateDownloadProgress(gui); // Last one
		UpdateGUI(gui);
	}
	else if (cmd == "ondownloadablemissionshowdetails")
	{
		int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

		// Issue a new download request
		gameLocal.m_MissionManager->StartDownloadingModDetails(missionIndex);

		gui->HandleNamedEvent("onDownloadableMissionDetailsLoaded");
	}
	else if (cmd == "onstartdownload")
	{
		StartDownload(gui);
		UpdateDownloadProgress(gui); // do this first
		UpdateGUI(gui);
	}
	else if (cmd == "ondownloadcompleteconfirm")
	{
		// Let the GUI request another refresh of downloadable missions (with delay)
		gui->HandleNamedEvent("QueueDownloadableMissionListRefresh");
	}
	else if (cmd == "ongetnextscreenshotforavailablemission")
	{
		PerformScreenshotStep(gui, +1);
		UpdateScreenshotItemVisibility(gui);
	}
	else if (cmd == "ongetprevscreenshotforavailablemission")
	{
		PerformScreenshotStep(gui, -1);
		UpdateScreenshotItemVisibility(gui);
	}
	else if ( cmd == "onsortmissions" ) {
		auto mm = gameLocal.m_MissionManager;
		mm->sortByDate = !mm->sortByDate;
		mm->SortDownloadableMods();
		UpdateGUI( gui );
	}
}

void CDownloadMenu::UpdateScreenshotItemVisibility(idUserInterface* gui)
{
  int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	int numScreens = mods[missionIndex]->screenshots.Num();

	gui->SetStateBool("av_no_screens_available", numScreens == 0);
	gui->SetStateBool("av_mission_screenshot_prev_visible", numScreens > 1);
	gui->SetStateBool("av_mission_screenshot_next_visible", numScreens > 1);
}

void CDownloadMenu::UpdateNextScreenshotData(idUserInterface* gui, int nextScreenshotNum)
{
  int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	if (mods[missionIndex]->screenshots.Num() == 0)
	{
		return; // no screenshots for this mission
	}

	MissionScreenshot& screenshotInfo = *mods[missionIndex]->screenshots[nextScreenshotNum];

	// Update the current screenshot number
	gui->SetStateInt("av_mission_cur_screenshot_num", nextScreenshotNum);

	// Load next screenshot path, remove image extension
	idStr path = screenshotInfo.filename;
	path.StripFileExtension();

	gui->SetStateString("av_mission_next_screenshot", path);
}

void CDownloadMenu::PerformScreenshotStep(idUserInterface* gui, int step)
{
  int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

	// Check if the screenshot is downloaded already
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	assert(missionIndex >= 0 && missionIndex < mods.Num());

	int numScreens = mods[missionIndex]->screenshots.Num();

	if (numScreens == 0)
	{
		return; // no screenshots for this mission
	}

	int curScreenNum = gui->GetStateInt("av_mission_cur_screenshot_num");
	int nextScreenNum = (curScreenNum + step + numScreens) % numScreens; // ensure index is always positive

	assert(nextScreenNum >= 0);

	// Store the next number in the GUI
	gui->SetStateInt("av_mission_next_screenshot_num", nextScreenNum);

	if (nextScreenNum != curScreenNum || curScreenNum == 0)
	{
		MissionScreenshot& screenshotInfo = *mods[missionIndex]->screenshots[nextScreenNum];

		if (screenshotInfo.filename.IsEmpty())
		{
			// No local file yet, start downloading it
			gui->HandleNamedEvent("onStartDownloadingNextScreenshot");

			// New request
			gameLocal.m_MissionManager->StartDownloadingMissionScreenshot(missionIndex, nextScreenNum);
		}
		else
		{
			// Load data necessary to fade into the GUI
			UpdateNextScreenshotData(gui, nextScreenNum);

			const char* sNext = gui->GetStateString("av_mission_next_screenshot");
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("PerformScreenshotStep: %s.", sNext);

			gui->HandleNamedEvent("onStartFadeToNextScreenshot");
		}
	}
}

void CDownloadMenu::StartDownload(idUserInterface* gui)
{
	
	int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");
	// Add a new download for each selected mission
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	for (int i = 0; i < _selectedMods.Num(); ++i)
	{
		int missionIndex = _selectedMods[i];

		if (missionIndex < 0 || missionIndex > mods.Num()) continue;

		const DownloadableMod& mod = *mods[missionIndex];

		// The filename is deduced from the mod name found on the website
		idStr targetPath = g_Global.GetDarkmodPath().c_str();
		targetPath += "/";
		targetPath += cv_tdm_fm_path.GetString();

		// Final path to the FM file
		idStr missionPath = targetPath + mod.modName + ".pk4";

		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Will download the mission PK4 to %s (modName %s).", missionPath.c_str(), mod.modName.c_str());

		// log all the URLs
		for (int u = 0; u < mod.missionUrls.Num(); u++)
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING(" URL: '%s'", mod.missionUrls[u].c_str());
		}

		CDownloadPtr download(new CDownload(mod.missionUrls, missionPath, true));
		download->VerifySha256Checksum(mod.missionSha256);
        // gnartsch: In case only the language pack needs to be downloaded, do not add the mission itself to the download list.
		//           In that case we did not add any urls for the mission itself anyway.
        int id = -1;
		if (mod.missionUrls.Num() > 0)
        {
        	// Check for valid PK4 files after download
			id = gameLocal.m_DownloadManager->AddDownload(download);
		}

		int l10nId = -1;

		// Check if there is a Localisation pack available
		if (mod.l10nPackUrls.Num() > 0)
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("There are l10n pack URLs listed for this FM.");

			for (int u = 0; u < mod.l10nPackUrls.Num(); u++)
			{
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING(" l10n pack URL: '%s'", mod.l10nPackUrls[u].c_str());
			}
			idStr l10nPackPath = targetPath + mod.modName + "_l10n.pk4";

			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Will download the l10n pack to %s.", l10nPackPath.c_str());

			CDownloadPtr l10nDownload(new CDownload(mod.l10nPackUrls, l10nPackPath, true));
			l10nDownload->VerifySha256Checksum(mod.l10nPackSha256);

			l10nId = gameLocal.m_DownloadManager->AddDownload(l10nDownload);

			// Relate these two downloads, so that they might be handled as pair
			// gnartsch: In case only the language pack needs to be downloaded, we can ignore the mission itself
			if (id > 0) {
				download->SetRelatedDownloadId(l10nId);
			} else {
				id = l10nId;
				l10nId = -1;
			}
		}

		// Store these IDs for later reference
		_downloads[missionIndex] = MissionDownload(id, l10nId);
	}

	// Let the download manager start its downloads
	gameLocal.m_DownloadManager->ProcessDownloads();
}

void CDownloadMenu::UpdateModDetails(idUserInterface* gui)
{
	// Get the selected mod index
  int modIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	if (modIndex < 0 || modIndex >= mods.Num())
	{
		return;
	}

	if (!mods[modIndex]->detailsLoaded)
	{
		GuiMessage msg;
		msg.type = GuiMessage::MSG_OK;
		msg.okCmd = "close_msg_box";
		msg.title = common->Translate( "#str_02003" ); // "Code Logic Error"
		msg.message = common->Translate( "#str_02004" ); // "No mission details loaded."

		gameLocal.AddMainMenuMessage(msg);

		return;
	}

	gui->SetStateString("av_mission_title", mods[modIndex]->title);
	gui->SetStateString("av_mission_author", mods[modIndex]->author);
	gui->SetStateString("av_mission_release_date", mods[modIndex]->releaseDate);
	gui->SetStateString("av_mission_version", va("%d", mods[modIndex]->version));
	gui->SetStateString("av_mission_size", va("%0.1f %s", mods[modIndex]->sizeMB, common->Translate( "#str_02055" )));	// MB

	gui->SetStateString("av_mission_description", mods[modIndex]->description);
}

void CDownloadMenu::UpdateGUI(idUserInterface* gui)
{
	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	gui->SetStateBool("av_no_download_available", mods.Num() == 0);
	auto title = idStr::Fmt( "Sorted By %s", gameLocal.m_MissionManager->sortByDate ? "Date" : "Title" );
	gui->SetStateString( "sort_btn_caption", title );

	bool downloadInProgress = gui->GetStateBool("mission_download_in_progress");

	bool updateInList = false;
	
	for (int index = 0; index < mods.Num(); ++index)
	{
		idStr title = mods[index]->title;

		if (mods[index]->isUpdate)
		{
			//title += "*";
			// Obsttorte #5842
			title = "*" + title;
			updateInList = true;
		}
          // gnartsch: check if a localization pack needs to be downloaded
		else if (mods[index]->needsL10NpackDownload && mods[index]->l10nPackUrls.Num() > 0)
		{
			//title += "#";
			// Obsttorte #5842
			title = "#" + title;
			updateInList = true;
		}

		gui->SetStateString(va("downloadAvailableList_item_%d", index), title);
	}

	gui->SetStateBool("av_mission_update_in_list", updateInList);

	// Missions in the download queue
	for (int i = 0; i < _selectedMods.Num(); ++i)
	{
	
		   int modIndex = _selectedMods[i];
idStr lineContents;
    if (modIndex < 0 || modIndex >= mods.Num()) continue;
    lineContents = mods[modIndex]->title;
    lineContents += "\t";
    lineContents += GetMissionDownloadProgressString(_selectedMods[i]);
    gui->SetStateString(va("downloadSelectedList_item_%d", i), lineContents);
	}

  // Cap list by clearing elements
  gui->DeleteStateVar(va("downloadSelectedList_item_%d", _selectedMods.Num()));

	gui->SetStateInt("dl_mission_count", _selectedMods.Num());
	gui->SetStateBool("dl_button_available", _selectedMods.Num() > 0 && !downloadInProgress);
	gui->SetStateBool("dl_button_visible", !downloadInProgress);

  gui->StateChanged(gameLocal.time);
}

void CDownloadMenu::UpdateDownloadProgress(idUserInterface* gui)
{
	bool downloadsInProgress = false;

	// Check if we have any mission downloads pending.
	// Don't use DownloadManager::DownloadInProgress(), as there might be different kind of downloads in progress
	for (ActiveDownloads::const_iterator it = _downloads.begin(); it != _downloads.end(); ++it)
	{
		int missionDownloadId = it->second.missionDownloadId;
		int l10nPackDownloadId = it->second.l10nPackDownloadId;

		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(missionDownloadId);

		CDownloadPtr l10nDownload = l10nPackDownloadId != -1 ? gameLocal.m_DownloadManager->GetDownload(missionDownloadId) : CDownloadPtr();

		if (!download && !l10nDownload) continue;

		if (download->GetStatus() == CDownload::IN_PROGRESS || 
			(l10nDownload && l10nDownload->GetStatus() == CDownload::IN_PROGRESS))
		{
			downloadsInProgress = true;
			break;
		}
	}


	// Update the "in progress" state flag 
	bool prevDownloadsInProgress = gui->GetStateBool("mission_download_in_progress");
	
	gui->SetStateBool("mission_download_in_progress", downloadsInProgress);

	if (prevDownloadsInProgress != downloadsInProgress)
	{
		if (downloadsInProgress == false)
		{
			// Fire the "finished downloaded" event
			ShowDownloadResult(gui);
		}
	}
}

idStr CDownloadMenu::GetMissionDownloadProgressString(int modIndex)
{
	// Agent Jones #4254
	if ( _downloads.empty() )
	{
		return "";
	}

	ActiveDownloads::const_iterator it = _downloads.find(modIndex);

	if (it == _downloads.end())
	{
		return common->Translate( "#str_02180" );
	}

	CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(it->second.missionDownloadId);
	CDownloadPtr l10nDownload;
	
	if (it->second.l10nPackDownloadId != -1)
	{
		l10nDownload = gameLocal.m_DownloadManager->GetDownload(it->second.l10nPackDownloadId);
	}

	if (!download && !l10nDownload) return idStr();

	switch (download->GetStatus())
	{
	case CDownload::NOT_STARTED_YET:
		return common->Translate( "#str_02180" );	// "queued "
	case CDownload::FAILED:
		return common->Translate( "#str_02181" );	// "failed "
	case CDownload::MALFORMED:
		return common->Translate( "#str_02518" );	// "malformed "
	case CDownload::IN_PROGRESS:
	{
		double totalFraction = download->GetProgressFraction(); 

		if (l10nDownload)
		{
			// We assume the L10n pack to consume 10% of the whole download 
			// This is just a rough guess, for some missions the l10n pack
			// is bigger than the mission, for others it is only 5%
			totalFraction *= 0.90f;
			totalFraction += 0.10f * l10nDownload->GetProgressFraction();
		}

		return va("%0.1f%s", totalFraction*100, "% ");
	}
	case CDownload::SUCCESS:
		return "100% ";
	default:
		return "??";
	};
}

void CDownloadMenu::ShowDownloadResult(idUserInterface* gui)
{
	// greebo: Let the mod list be refreshed
	// We need the information from darkmod.txt later down this road
	gameLocal.m_MissionManager->ReloadModList();

	int successfulDownloads = 0;
	int failedDownloads = 0;
	int malformedDownloads = 0;
	int canceledDownloads = 0; //Agent Jones
	bool updateCurrentFm = false;

	const DownloadableModList& mods = gameLocal.m_MissionManager->GetDownloadableMods();

	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(i->second.missionDownloadId);

		if (download == NULL) continue;

		if (i->first > mods.Num()) continue;

		const DownloadableMod& mod = *mods[i->first];

		switch (download->GetStatus())
		{
		case CDownload::NOT_STARTED_YET:
			gameLocal.Warning("Some downloads haven't been processed?");
			break;
		case CDownload::FAILED:
			failedDownloads++;
			break;
		case CDownload::MALFORMED:
			malformedDownloads++;
			break;
		case CDownload::CANCELED:
			canceledDownloads++;//Agent Jones Test
			break;
		case CDownload::IN_PROGRESS:
			gameLocal.Warning("Some downloads still in progress?");
			break;
		case CDownload::SUCCESS:
		{
			// gnartsch
			bool l10nPackDownloaded = false;
			// In case of success, check l10n download status
			if (i->second.l10nPackDownloadId != -1)
			{
				CDownloadPtr l10nDownload = gameLocal.m_DownloadManager->GetDownload(i->second.l10nPackDownloadId);

				CDownload::DownloadStatus l10nStatus = l10nDownload->GetStatus();

				if (l10nStatus == CDownload::NOT_STARTED_YET || l10nStatus == CDownload::IN_PROGRESS)
				{
					gameLocal.Warning("Localisation pack download not started or still in progress?");
				}
				else if (l10nStatus == CDownload::FAILED)
				{
					gameLocal.Warning("Failed to download localisation pack!");

					// Turn this download into a failed one
					failedDownloads++;
				}
				else if (l10nStatus == CDownload::SUCCESS)
				{
					// both successfully downloaded
					successfulDownloads++;
					// gnartsch
					l10nPackDownloaded = true;
				}
			}
			else // regular download without l10n ... or l10n download only (gnartsch)
			{
				successfulDownloads++;
				// gnartsch: Consider Localization pack having been dealt with as well
				l10nPackDownloaded = true;
			}

			// Save the mission version into the MissionDB for later use
			CModInfoPtr missionInfo = gameLocal.m_MissionManager->GetModInfo(mod.modName);
			missionInfo->SetKeyValue("downloaded_version", idStr(mod.version).c_str());
			// gnartsch: Mark l10n pack as present, so that the mission may disappear from the list of 'Available Downloads'
			missionInfo->isL10NpackInstalled = l10nPackDownloaded;

			// stgatilov #5661: check if we have downloaded update to the currently installed FM
			if (mod.modName == gameLocal.m_MissionManager->GetCurrentModName())
			{
				updateCurrentFm = true;
			}
		}
		break;
		};
	}

	// stgatilov: save missions.tdminfo right now!
	// otherwise the information about downloaded version will be lost
	// if the game crashes before proper exit/restart
	gameLocal.m_MissionManager->SaveDatabase();

	gameLocal.Printf("Successful downloads: %d\nFailed downloads: %d\n", successfulDownloads, failedDownloads);
	// Display the popup box
	GuiMessage msg;
	msg.type = GuiMessage::MSG_OK;
	msg.okCmd = "close_msg_box;onDownloadCompleteConfirm";
	msg.title = common->Translate("#str_02142"); // "Mission Download Result"
	msg.message = "";

	if (successfulDownloads > 0)
	{
		msg.message += va(
			// "%d mission/missions successfully downloaded. You'll find it/them in the 'New Mission' page."
			GetPlural(successfulDownloads, common->Translate("#str_02144"), common->Translate("#str_02145")),
			successfulDownloads);
	}
	if (failedDownloads > 0)
	{
		// "\n%d mission(s) couldn't be downloaded. Please check your disk space (or maybe some file is write protected) and try again."
		msg.message += va(common->Translate("#str_02146"),
			failedDownloads);
	}
	if (malformedDownloads > 0)
	{
		// "\n%d mission(s) are wrong on mirrors.\nPlease report to maintainers."
		msg.message += va(common->Translate("#str_02693"), malformedDownloads);
	}

	if (failedDownloads > 0 || successfulDownloads > 0 || malformedDownloads > 0)
	{
		gameLocal.AddMainMenuMessage(msg);

		if (updateCurrentFm)
		{
			GuiMessage msgRestart;
			// "Current mission was updated.\nGame restart is required!"
			msgRestart.message = common->Translate("#str_menu_updatedcurrentfm");
			msgRestart.type = GuiMessage::MSG_CUSTOM;
			msgRestart.positiveCmd = "close_msg_box;darkmodRestart";
			msgRestart.negativeCmd = "close_msg_box;onDownloadCompleteConfirm";
			msgRestart.positiveLabel = common->Translate("#str_menu_restart");			//"Restart";
			msgRestart.negativeLabel = common->Translate("#str_menu_later");			//"Later"
			gameLocal.AddMainMenuMessage(msgRestart);
		}
	}
	else
	{
		UpdateGUI(gui);
	}

	// Remove all downloads
	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		gameLocal.m_DownloadManager->RemoveDownload(i->second.missionDownloadId);

		if (i->second.l10nPackDownloadId != -1)
		{
			gameLocal.m_DownloadManager->RemoveDownload(i->second.l10nPackDownloadId);
		}
	}

	_downloads.clear();
}
