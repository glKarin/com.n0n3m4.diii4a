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
#include "Ini.h"

//stores size and timestamp of every managed zip file
//it is saved in TDM_INSTALLER_LASTSCAN_PATH in order to
//avoid rescanning all zips on every launch
class ScanState {
	struct ZipState {
		std::string name;
		uint64_t timestamp = 0;
		uint32_t size = 0;

		bool operator<(const ZipState &b) const;
		bool operator==(const ZipState &b) const;
	};

	std::vector<ZipState> _zips;

public:
	static ScanState ScanZipSet(const std::vector<std::string> &zipPaths, std::string rootDir);

	static ScanState ReadFromIni(const ZipSync::IniData &ini);
	ZipSync::IniData WriteToIni() const;

	bool NotChangedSince(const ScanState &cleanState) const;
};

//stores last installed version and set of "owned" files
//it is saved in TDM_INSTALLER_LASTINSTALL_PATH in order to
//  1) show "last installer version" in GUI
//  2) remove all installed files when updating to different version
class InstallState {
	std::string _version;
	std::vector<std::string> _ownedZips;
	std::vector<std::string> _ownedUnpacked;

public:
	InstallState();
	InstallState(const std::string &version, const std::vector<std::string> &ownedZips, const std::vector<std::string> &ownedUnpacked);

	const std::string &GetVersion() const { return _version; }
	const std::vector<std::string> &GetOwnedZips() const { return _ownedZips; }
	const std::vector<std::string> &GetOwnedUnpackedFiles() const { return _ownedUnpacked; }

	static InstallState ReadFromIni(const ZipSync::IniData &ini);
	ZipSync::IniData WriteToIni() const;
};
