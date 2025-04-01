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

#include <cstdint>
#include <string>
#include <vector>

namespace ZipSync {
	class ProgressIndicator;
}

class Actions {
public:
	//called when user clicks "Restart" button with custom install dir
	static void RestartWithInstallDir(const std::string &installDir);

	//checks that we can write files at installation directory, and for free space
	//errors are thrown as exceptions, but warnings are returned as array of strings
	static std::vector<std::string> CheckSpaceAndPermissions(const std::string &installDir);

	//called when we are sure user won't change install dir any more
	static void StartLogFile();

	//check if central server offers different executable
	static bool NeedsSelfUpdate(ZipSync::ProgressIndicator *progress);
	//update and restart installer
	//must be called immediately after NeedsSelfUpdate returns true
	static void DoSelfUpdate();

	//read g_config from file in install dir
	//if download = true, then the file is downloaded from TDM server first
	static void ReadConfigFile(bool download, ZipSync::ProgressIndicator *progress);

	//called when user clicks "Next" button on settings page
	//generates manifest in the install directory
	static void ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress);

	struct VersionInfo {
		uint64_t currentSize = 0;
		uint64_t finalSize = 0;
		uint64_t addedSize = 0;
		uint64_t removedSize = 0;
		uint64_t downloadSize = 0;
		uint64_t missingSize = 0;		//only possible with custom manifest
	};
	//user wants to know stats about possible update to specified version
	//this action can trigger downloading manifests (note: they are cached in g_state)
	//if customManifestUrl is nonempty, then it overrides target manifest location
	static VersionInfo RefreshVersionInfo(const std::string &version, const std::string &customManifestUrl, bool bitwiseExact, ZipSync::ProgressIndicator *progress);

	//perform prepared update: download all data
	static void PerformInstallDownload(ZipSync::ProgressIndicator *progressDownload, ZipSync::ProgressIndicator *progressVerify, bool blockMultipart);

	//perform prepared update: repack installation
	static void PerformInstallRepack(ZipSync::ProgressIndicator *progress);

	//finalize update (cleanup, manifest, unpacking, etc.)
	static void PerformInstallFinalize(ZipSync::ProgressIndicator *progress);

	//did we rename darkmod.cfg to some file?
	static bool CanRestoreOldConfig();
	//restores old version of darkmod.cfg which was renamed automatically
	static void DoRestoreOldConfig();

	//return true if TDM shortcut is already present on Desktop
	static bool IfShortcutExists();
	//creates or overwrites Desktop shortcut for TDM
	static void CreateShortcut();
};
