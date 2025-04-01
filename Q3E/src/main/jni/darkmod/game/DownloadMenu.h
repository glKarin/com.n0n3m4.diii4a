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

#ifndef _DOWNLOAD_MENU_H_
#define	_DOWNLOAD_MENU_H_

#include <map>
#include "Missions/MissionManager.h"

// Handles mainmenu that displays list of downloadable mods/PK4 files
class CDownloadMenu
{
private:
	idList<DownloadableMod*> _downloadableModList;
	idList<DownloadableMod*> _downloadSelectedList;
	DownloadableMod* _selectedMod;

	/**
	 * greebo: Since mission l10n packs are stored separately, a mission
	 * transfer can consist of several downloads.
	 */
	struct MissionDownload
	{
		int missionDownloadId;	// Download ID of the actual mission pack
		int l10nPackDownloadId;	// Download ID of the L10n Pack, is -1 by default

		MissionDownload() :
			missionDownloadId(-1),
			l10nPackDownloadId(-1)
		{}

		MissionDownload(int missionDownloadId_, int l10nPackDownloadId_ = -1) :
			missionDownloadId(missionDownloadId_),
			l10nPackDownloadId(l10nPackDownloadId_)
		{}
	};

	// A mapping "selected mod" => download info
	typedef std::map<DownloadableMod*, MissionDownload> ActiveDownloads;
	ActiveDownloads _downloads;

public:

	// handles main menu commands
	void HandleCommands(const idStr& cmd, idUserInterface* gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);
	void UpdateDownloadableGUI(idUserInterface* gui, bool redraw = true);
	void UpdateSelectedGUI(idUserInterface* gui, bool redraw = true);

private:
	void ResetDownloadableList();

	void SortDownloadableMods();

	void Search(idUserInterface* gui);

	void UpdateScreenshotItemVisibility(idUserInterface* gui);
	void UpdateNextScreenshotData(idUserInterface* gui, int nextScreenshotNum);

	void PerformScreenshotStep(idUserInterface* gui, int step);

	void StartDownload(idUserInterface* gui);

	void SetMissionDetailsVisibility(idUserInterface* gui, int index);
	void UpdateModDetails(idUserInterface* gui);

	void UpdateDownloadProgress(idUserInterface* gui);
	idStr GetMissionDownloadProgressString(DownloadableMod* mod);
	void ShowDownloadResult(idUserInterface* gui);
};
typedef std::shared_ptr<CDownloadMenu> CDownloadMenuPtr;

#endif	/* _DOWNLOAD_MENU_H_ */
