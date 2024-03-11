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

#define TDM_INSTALLER_VERSION "1.12"

#define TDM_INSTALLER_LOG_FORMAT "tdm_installer_%lld.log"
#define TDM_INSTALLER_CONFIG_FILENAME "tdm_installer.ini"
#define TDM_INSTALLER_LOCAL_MANIFEST "manifest.iniz"
#define TDM_INSTALLER_EXECUTABLE_FILENAME "tdm_installer" TDM_INSTALLER_EXECUTABLE_FILENAME_SUFFIX

#define TDM_INSTALLER_ZIPSYNC_DIR ".zipsync"
#define TDM_INSTALLER_MANICACHE_SUBDIR "mani"
#define TDM_INSTALLER_LASTSCAN_PATH TDM_INSTALLER_ZIPSYNC_DIR "/lastscan.ini"
#define TDM_INSTALLER_LASTINSTALL_PATH TDM_INSTALLER_ZIPSYNC_DIR "/lastinstall.ini"

#define TDM_INSTALLER_CONFIG_URL "http://update.thedarkmod.com/zipsync/tdm_installer.ini"
#define TDM_INSTALLER_EXECUTABLE_URL_PREFIX "http://update.thedarkmod.com/zipsync/"
#define TDM_INSTALLER_TRUSTED_URL_PREFIX "http://update.thedarkmod.com/"

#define TDM_INSTALLER_FREESPACE_MINIMUM 100				//100 MB --- hardly enough even for differential update
#define TDM_INSTALLER_FREESPACE_RECOMMENDED (5<<10)		//5 GB --- enough for any update, since it is larger than size of TDM

#define TDM_DARKMOD_CFG_FILENAME "Darkmod.cfg"
#define TDM_DARKMOD_CFG_OLD_FORMAT "Darkmod_%y%m%d_%H%M%S.cfg"
#define TDM_DARKMOD_SHORTCUT_NAME "TheDarkMod"
#define TDM_DARKMOD_SHORTCUT_ICON "TDM_icon.ico"
#define TDM_DARKMOD_SHORTCUT_COMMENT "TheDarkMod: Stealth Gaming in a Gothic Steampunk World"
#ifdef _WIN32
#define TDM_DARKMOD_SHORTCUT_EXECUTABLES {"TheDarkModx64.exe", "TheDarkMod.exe"}
#else
#define TDM_DARKMOD_SHORTCUT_EXECUTABLES {"thedarkmod.x64", "thedarkmod.x86"}
#endif

#define TDM_INSTALLER_USERAGENT "tdm_installer/" TDM_INSTALLER_VERSION
