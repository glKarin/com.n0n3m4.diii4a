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
#pragma once

#include "InstallerConfig.h"
#include "StoredState.h"
#include <map>
#include "Manifest.h"

namespace ZipSync {
	class UpdateProcess;
};

struct State {
	//contents of TDM_INSTALLER_CONFIG_FILENAME
	InstallerConfig _config;
	//describes local state of the installation dir
	ZipSync::Manifest _localManifest;
	//this is read from TDM_INSTALLER_LASTINSTALL_PATH, includes:
	//  version --- for display only
	//  owned set of files --- to be removed during update
	InstallState _lastInstall;
	//set of versions for which manifest has already been loaded
	std::map<std::string, ZipSync::Manifest> _loadedManifests;
	//which version was last evaluated with "refresh" button
	//if custom url was specified, then it is appended to based version (with " & " separator)
	std::string _versionRefreshed;
	//the update which is going to be made (or is made right now)
	//if present, then it is prepared to update to _versionRefreshed
	//action RefreshVersionInfo stores it here if plan is successfully developed
	std::unique_ptr<ZipSync::UpdateProcess> _updater;
	//name of the renamed darkmod.cfg file
	//useful if user decides to restore it
	std::string _oldConfigFilename;
	//name of preferred mirror (or empty if auto)
	std::string _preferredMirror;

	void Reset();
	~State();
};

//global state of the updater
extern State *g_state;
