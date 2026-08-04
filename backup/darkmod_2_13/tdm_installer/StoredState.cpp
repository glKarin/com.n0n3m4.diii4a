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
#include "StoredState.h"
#include "StdFilesystem.h"
#include "StdString.h"
#include "Path.h"
#include <algorithm>
#include <string.h>
#include "LogUtils.h"

bool ScanState::ZipState::operator<(const ZipState &b) const {
	return name < b.name;
}
bool ScanState::ZipState::operator==(const ZipState &b) const {
	return name == b.name && timestamp == b.timestamp && size == b.size;
}

ScanState ScanState::ScanZipSet(const std::vector<std::string> &zipPaths, std::string rootDir) {
	ScanState res;
	for (const std::string &absPath : zipPaths) {
		std::string relPath = ZipSync::PathAR::FromAbs(absPath, rootDir).rel;
		ZipState zs;
		zs.name = relPath;
		zs.size = stdext::file_size(absPath);
		zs.timestamp = stdext::last_write_time(absPath);
		res._zips.push_back(zs);
	}
	std::sort(res._zips.begin(), res._zips.end());
	return res;
}

ScanState ScanState::ReadFromIni(const ZipSync::IniData &ini) {
	ScanState res;

	for (const auto &pNS : ini) {
		std::string secname = pNS.first;
		ZipSync::IniSect sec = pNS.second;

		if (stdext::starts_with(secname, "Zip ")) {
			ZipState zs;
			zs.name = secname.substr(4);
			ZipSyncAssert(sec[0].first == "modTime");
			long long unsigned temp;
			int k = sscanf(sec[0].second.c_str(), "%llu", &temp);
			ZipSyncAssert(k == 1);
			zs.timestamp = temp;
			ZipSyncAssert(sec[1].first == "size");
			k = sscanf(sec[1].second.c_str(), "%u", &zs.size);
			ZipSyncAssert(k == 1);
			res._zips.push_back(zs);
		}
		else {
			ZipSyncAssert(false);
		}
	}

	return res;
}

ZipSync::IniData ScanState::WriteToIni() const {
	ZipSync::IniData ini;

	for (const ZipState &zs : _zips) {
		std::string secname = "Zip " + zs.name;
		ZipSync::IniSect sec;
		sec.emplace_back("modTime", std::to_string(zs.timestamp));
		sec.emplace_back("size", std::to_string(zs.size));
		ini.emplace_back(secname, sec);
	}

	return ini;
}

bool ScanState::NotChangedSince(const ScanState &cleanState) const {
	//note: both must already be sorted
	return _zips == cleanState._zips;
}


InstallState::InstallState() {}
InstallState::InstallState(const std::string &version, const std::vector<std::string> &ownedZips, const std::vector<std::string> &ownedUnpacked) {
	_version = version;
	_ownedZips = ownedZips;
	_ownedUnpacked = ownedUnpacked;
}

InstallState InstallState::ReadFromIni(const ZipSync::IniData &ini) {
	InstallState res;

	for (const auto &pNS : ini) {
		std::string secname = pNS.first;
		ZipSync::IniSect sec = pNS.second;

		if (secname == "Version") {
			ZipSyncAssert(sec[0].first == "version");
			res._version = sec[0].second;
		}
		else if (stdext::starts_with(secname, "Zip ")) {
			std::string filename = secname.substr(strlen("Zip "));
			res._ownedZips.push_back(filename);
		}
		else if (stdext::starts_with(secname, "UnpackedFile ")) {
			std::string filename = secname.substr(strlen("UnpackedFile "));
			res._ownedUnpacked.push_back(filename);
		}
	}

	return res;
}

ZipSync::IniData InstallState::WriteToIni() const {
	ZipSync::IniData ini;

	ZipSync::IniSect sec = {std::make_pair("version", _version)};
	ini.emplace_back("Version", sec);

	for (const std::string &fn : _ownedZips) {
		std::string secname = "Zip " + fn;
		ini.emplace_back(secname, ZipSync::IniSect{});
	}
	for (const std::string &fn : _ownedUnpacked) {
		std::string secname = "UnpackedFile " + fn;
		ini.emplace_back(secname, ZipSync::IniSect{});
	}

	return ini;
}
