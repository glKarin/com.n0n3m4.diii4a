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

#ifndef _MOD_MENU_H_
#define	_MOD_MENU_H_


class CModInfo;
typedef std::shared_ptr<CModInfo> CModInfoPtr;

// Handles mainmenu that displays list of mods (FMs) and lets user
// chose which one to load. Also handles display of briefing page
class CModMenu
{
private:
	// The index of the first displayed mod
	int _modTop;

	int _briefingPage;

public:
	CModMenu();

	// handles main menu commands
	void HandleCommands(const idStr& cmd, idUserInterface* gui);

	// updates the GUI variables
	void UpdateGUI(idUserInterface* gui);

	// displays the current briefing page
	void DisplayBriefingPage(idUserInterface* gui);

private:

	// Performs the version check and returns TRUE if positive,
	// returns FALSE otherwise (and issues failure calls to the given GUI)
	bool PerformVersionCheck(const CModInfoPtr& mission, idUserInterface* gui);

	void UpdateSelectedMod(idUserInterface* gui);

	// Installs the given mod (doesn't accept NULL pointers);
	void InstallMod(const CModInfoPtr& mod, idUserInterface* gui);

	// Uninstalls the current FM
	void UninstallMod(idUserInterface* gui);

	// Restarts the engine after mod installation
	void RestartEngine();
};

#endif	/* _MOD_MENU_H_ */
