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
		const int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

		// Handle type-to-search in the list.
		if ((missionIndex >= 0)
		    && (missionIndex < _downloadableModList.Num())
		    && (_selectedMod != _downloadableModList[missionIndex]))
		{
			_selectedMod = _downloadableModList[missionIndex];
			SetMissionDetailsVisibility(gui, missionIndex);
		}

		// Update download progress
		if (_downloadSelectedList.Num() > 0)
		{
			UpdateSelectedGUI(gui);
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

					ResetDownloadableList();
					SortDownloadableMods();
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
		gui->SetStateString("download_search_text", "");
		gui->SetStateInt("downloadAvailableList_scroll", 0); // scroll to top
		_selectedMod = NULL;
		_downloadableModList.Clear();
		_downloadSelectedList.Clear();

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
		if (missionIndex < 0 || missionIndex >= _downloadableModList.Num()) return;

		// Update mission details
		_selectedMod = _downloadableModList[missionIndex];
		SetMissionDetailsVisibility(gui, missionIndex);
	}
	else if (cmd == "onselectmissionfordownload")
	{
		int missionIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");
		if (missionIndex < 0 || missionIndex >= _downloadableModList.Num()) return;

		_downloadSelectedList.AddUnique(_downloadableModList[missionIndex]);
		SetMissionDetailsVisibility(gui, -1);
		UpdateGUI(gui);
	}
	// #4492(Obsttorte)
	else if (cmd == "onselectallmissionsfordownload")
	{
		for (DownloadableMod* mod : _downloadableModList) {
			_downloadSelectedList.AddUnique(mod);
		}
		SetMissionDetailsVisibility(gui, -1);
		UpdateGUI(gui);
	}
	else if (cmd == "ondeselectmissionfordownload")
	{
		int index = gui->GetStateInt("downloadSelectedList_sel_0", "-1");

		if (index < 0) return;

		if (gui->GetStateBool("mission_download_in_progress") == 1){//Cancel a download in progress
			if (gameLocal.m_DownloadManager->GetDownload(_downloads[_downloadSelectedList[index]].missionDownloadId)->GetStatus() == CDownload::/*DownloadStatus::*/SUCCESS) // grayman - make linux compiler happy
				return;//this download has been completed, you won't be able to cancel it.
			if (_downloads[_downloadSelectedList[index]].l10nPackDownloadId != -1){//we were downloading a localization pack
				CDownloadPtr l10ndownload = gameLocal.m_DownloadManager->GetDownload(_downloads[_downloadSelectedList[index]].l10nPackDownloadId);
				l10ndownload->Stop(true);
				gameLocal.m_DownloadManager->RemoveDownload(_downloads[_downloadSelectedList[index]].l10nPackDownloadId);
				}
			
			CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(_downloads[_downloadSelectedList[index]].missionDownloadId);
			download->Stop(true);
			gameLocal.m_DownloadManager->RemoveDownload(_downloads[_downloadSelectedList[index]].missionDownloadId);
		}
		_downloadSelectedList.Remove(_downloadSelectedList[index]);

		UpdateSelectedGUI(gui);
	}
	else if (cmd == "ondownloadablemissionshowdetails")
	{
		// Issue a new download request
		gameLocal.m_MissionManager->StartDownloadingModDetails(_selectedMod);

		gui->HandleNamedEvent("onDownloadableMissionDetailsLoaded");
	}
	else if (cmd == "onstartdownload")
	{
		StartDownload(gui);
		UpdateSelectedGUI(gui);
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
		SortDownloadableMods();
		UpdateDownloadableGUI( gui );
	}
	else if ( cmd == "ondownloadtitlestyle" ) {
		if (cvarSystem->GetCVarInteger("tdm_download_list_sort_by") == MISSION_LIST_SORT_BY_TITLE) {
			SortDownloadableMods();
		}
		UpdateDownloadableGUI( gui );
	}
	else if (cmd == "ondownloadsearch")
	{
		Search(gui);
		SortDownloadableMods();
		gui->SetStateInt("downloadAvailableList_scroll", 0); // scroll to top
		UpdateDownloadableGUI(gui);
	}
}

void CDownloadMenu::ResetDownloadableList()
{
	_downloadableModList = gameLocal.m_MissionManager->GetPrimaryDownloadableMods();
}

void CDownloadMenu::SortDownloadableMods()
{
	if (cvarSystem->GetCVarInteger("tdm_download_list_sort_by") == MISSION_LIST_SORT_BY_DATE) {
		_downloadableModList.Sort(DownloadableMod::SortCompareDate);
	} else {
		_downloadableModList.Sort(DownloadableMod::SortCompareTitle);
	}

	if (cvarSystem->GetCVarInteger("tdm_download_list_sort_direction") == MISSION_LIST_SORT_DIRECTION_DESC) {
		_downloadableModList.Reverse();
	}
}

// Daft Mugi #6449: Add Search
void CDownloadMenu::Search(idUserInterface* gui)
{
	idStr searchText = gui->GetStateString("download_search_text", "");
	searchText.StripLeadingWhitespace(); // remove leading space; otherwise, lock up occurs.

	// If there's no search text, repopulate mod list using primary downloadable list and bail.
	if (searchText.IsEmpty()) {
		ResetDownloadableList();
		return;
	}

	// Clear downloadable list.
	_downloadableModList.Clear();

	// Populate mod list with search matches.
	for (DownloadableMod* mod : gameLocal.m_MissionManager->GetPrimaryDownloadableMods()) {
		bool matched = false;
		idStr title = mod->title;

		// Try to match on title
		int matchedPos = 0;
		while (!matched && matchedPos >= 0) {
			matchedPos = title.Find(searchText, false, matchedPos);
			if (matchedPos >= 0) {
				if (matchedPos == 0 || title[matchedPos - 1] == ' ') {
					// Only match at the start of a word.
					matched = true;
				} else {
					// Not a word; try next match.
					matchedPos++;
				}
			} // else: no match anywhere; bail.
		}

		// Try to match on author
		if (!matched) {
			idList<idStr> authors;

			// Split on common separators to capture each author.
			authors = mod->author.Split({",", "&", " and ", "&amp;"}, true);
			for (idStr& a : authors) {
				a.StripWhitespace();
			}

			for (idStr& a : authors) {
				if (a.IstartsWith(searchText)) {
					matched = true;
					break;
				}
			}
		}

		// Add match
		if (matched) {
			_downloadableModList.Append(mod);
		}
	}
}

void CDownloadMenu::UpdateScreenshotItemVisibility(idUserInterface* gui)
{
	// Check if the screenshot is downloaded already
	int numScreens = _selectedMod->screenshots.Num();

	gui->SetStateBool("av_no_screens_available", numScreens == 0);
	gui->SetStateBool("av_mission_screenshot_prev_visible", numScreens > 1);
	gui->SetStateBool("av_mission_screenshot_next_visible", numScreens > 1);
}

void CDownloadMenu::UpdateNextScreenshotData(idUserInterface* gui, int nextScreenshotNum)
{
	// Check if the screenshot is downloaded already
	if (_selectedMod->screenshots.Num() == 0)
	{
		return; // no screenshots for this mission
	}

	MissionScreenshot& screenshotInfo = *_selectedMod->screenshots[nextScreenshotNum];

	// Update the current screenshot number
	gui->SetStateInt("av_mission_cur_screenshot_num", nextScreenshotNum);

	// Load next screenshot path, remove image extension
	idStr path = screenshotInfo.filename;
	path.StripFileExtension();

	gui->SetStateString("av_mission_next_screenshot", path);
}

void CDownloadMenu::PerformScreenshotStep(idUserInterface* gui, int step)
{
	// Check if the screenshot is downloaded already
	int numScreens = _selectedMod->screenshots.Num();
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
		MissionScreenshot& screenshotInfo = *_selectedMod->screenshots[nextScreenNum];

		if (screenshotInfo.filename.IsEmpty())
		{
			// No local file yet, start downloading it
			gui->HandleNamedEvent("onStartDownloadingNextScreenshot");

			// New request
			gameLocal.m_MissionManager->StartDownloadingMissionScreenshot(_selectedMod, nextScreenNum);
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
	// Add a new download for each selected mission
	for (int i = 0; i < _downloadSelectedList.Num(); ++i)
	{
		DownloadableMod* selectedMod = _downloadSelectedList[i];

		if (selectedMod == NULL) continue;

		const DownloadableMod& mod = *selectedMod;

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
		_downloads[selectedMod] = MissionDownload(id, l10nId);
	}

	// Let the download manager start its downloads
	gameLocal.m_DownloadManager->ProcessDownloads();
}

void CDownloadMenu::SetMissionDetailsVisibility(idUserInterface* gui, int index)
{
	gui->SetStateBool("av_mission_details_visible", index != -1);
	gui->SetStateInt("downloadAvailableList_sel_0", index);

	if (index == -1) {
		_selectedMod = NULL;
		return;
	}

	gui->SetStateString("av_mission_title", _selectedMod->title);
	gui->SetStateString("av_mission_author", _selectedMod->author);
	gui->SetStateString("av_mission_release_date", _selectedMod->releaseDate);
	gui->SetStateString("av_mission_type", _selectedMod->type == DownloadableMod::Multi ?
		common->Translate("#str_04353") : // Campaign
		common->Translate("#str_04352")); // Single Mission
	gui->SetStateString("av_mission_version", va("%d", _selectedMod->version));
	gui->SetStateString("av_mission_size", va("%0.1f %s", _selectedMod->sizeMB, common->Translate( "#str_02055" )));	// MB
}

void CDownloadMenu::UpdateModDetails(idUserInterface* gui)
{
	// Get the selected mod index
	int modIndex = gui->GetStateInt("downloadAvailableList_sel_0", "-1");

	const DownloadableModList& mods = _downloadableModList;

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
	UpdateSelectedGUI(gui, false);
	UpdateDownloadableGUI(gui, true);
}

void CDownloadMenu::UpdateDownloadableGUI(idUserInterface* gui, bool redraw)
{
	const int available_mods_num = gameLocal.m_MissionManager->GetPrimaryDownloadableMods().Num();
	const DownloadableModList& mods = _downloadableModList;
	const int num_mods = mods.Num();
	bool updateInList = false;

	gui->SetStateBool("av_no_download_available", available_mods_num == 0);

	for (int index = 0; index < num_mods; ++index)
	{
		idStr title = mods[index]->title;

		if (cvarSystem->GetCVarInteger("tdm_mission_list_title_style") == MISSION_LIST_TITLE_STYLE_CMOS) {
			CModInfo::StyleAsCMOS(title);
		}

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

		// Workaround: Create a tabstop in order to keep text from overflowing rectangle.
		title += "\t";

		gui->SetStateString(va("downloadAvailableList_item_%d", index), title);
	}

	// Ensure that only the desired items are displayed in ListWindow.
	// NOTE: A ListWindow list is populated until an item is not found.
	gui->DeleteStateVar(va("downloadAvailableList_item_%d", num_mods));

	// Ensure correct selection (or no selection) after the list is modified.
	int selectedIndex = _selectedMod ? mods.FindIndex(_selectedMod) : -1;
	SetMissionDetailsVisibility(gui, selectedIndex);

	// Ensure update label is shown even when search results do not include updates.
	if (updateInList) {
		gui->SetStateBool("av_mission_update_in_list", updateInList);
	}

	if (redraw) {
		gui->StateChanged(gameLocal.time);
	}
}

void CDownloadMenu::UpdateSelectedGUI(idUserInterface* gui, bool redraw)
{
	UpdateDownloadProgress(gui);

	// Missions in the download queue
	for (int i = 0; i < _downloadSelectedList.Num(); ++i)
	{
		idStr lineContents;
		idStr title = _downloadSelectedList[i]->title;

		if (cvarSystem->GetCVarInteger("tdm_mission_list_title_style") == MISSION_LIST_TITLE_STYLE_CMOS) {
			CModInfo::StyleAsCMOS(title);
		}

		lineContents = title;
		lineContents += "\t";
		lineContents += GetMissionDownloadProgressString(_downloadSelectedList[i]);
		gui->SetStateString(va("downloadSelectedList_item_%d", i), lineContents);
	}

	// Ensure that only the desired items are displayed in ListWindow.
	// NOTE: A ListWindow list is populated until an item is not found.
	gui->DeleteStateVar(va("downloadSelectedList_item_%d", _downloadSelectedList.Num()));

	bool downloadInProgress = gui->GetStateBool("mission_download_in_progress");

	gui->SetStateInt("dl_mission_count", _downloadSelectedList.Num());
	gui->SetStateBool("dl_button_available", _downloadSelectedList.Num() > 0 && !downloadInProgress);
	gui->SetStateBool("dl_button_visible", !downloadInProgress);

	if (redraw) {
		gui->StateChanged(gameLocal.time);
	}
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

idStr CDownloadMenu::GetMissionDownloadProgressString(DownloadableMod* mod)
{
	// Agent Jones #4254
	if ( _downloads.empty() )
	{
		return "";
	}

	ActiveDownloads::const_iterator it = _downloads.find(mod);

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

	for (ActiveDownloads::iterator i = _downloads.begin(); i != _downloads.end(); ++i)
	{
		CDownloadPtr download = gameLocal.m_DownloadManager->GetDownload(i->second.missionDownloadId);

		if (download == NULL) continue;

		const DownloadableMod& mod = *i->first;

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
