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

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Ini.h"

//describes information read from file TDM_INSTALLER_CONFIG_FILENAME
class InstallerConfig {
	struct ProcessedUrl {
		std::string _url;
		double _weight = -1.0;
		std::string _mirrorName;
	};
	struct Mirror {
		std::string _name;
		std::string _url;
		double _weight = -1.0;
		ZipSync::IniSect _ini;
		bool _hidden = false;
	};
	struct Version {
		std::vector<ProcessedUrl> _manifestUrls;
		std::vector<std::string> _depends;
		std::vector<std::string> _folderPath;
		std::vector<std::string> _providedVersions;			//transitive closure by _depends (itself included first)
		std::string _name;
		ZipSync::IniSect _ini;
	};
	std::map<std::string, Mirror> _mirrors;
	std::vector<Version> _versions;
	std::string _defaultVersion;

	double GetUrlWeight(const ProcessedUrl &url) const;
	Version *FindVersion(const std::string &version, bool mustExist = false);
	const Version *FindVersion(const std::string &version, bool mustExist = false) const;

public:
	void Clear();

	//creates InstallerConfig object by reading specified ini file
	void InitFromIni(const ZipSync::IniData &iniData);

	//returns sequence of all known versions
	std::vector<std::string> GetAllVersions() const;

	//returns sequence of all known mirrors
	//note: "hidden" mirrors are omitted in this list
	std::vector<std::string> GetAllMirrors() const;

	//returns GUI tree pathname for specified version
	//consists of sequence of path elements, excluding the version element at the end
	std::vector<std::string> GetFolderPath(const std::string &version) const;

	//default version is installed if user has not chosen any specific version
	std::string GetDefaultVersion() const;

	//return true if URL points to trusted (main) mirror
	bool IsUrlTrusted(const std::string &url) const;

	//returns url to manifest file of the version
	//if there are several possibilities, then random one is chosen
	//if trusted is set, then only URLs starting with TDM_INSTALLER_TRUSTED_URL_PREFIX are considered
	std::string ChooseManifestUrl(const std::string &version, bool trusted = false) const;

	//returns a set of versions whose manifest must be provided for update to specified version
	//pass them ChooseManifestUrl to get set of provided manifests
	std::vector<std::string> GetProvidedVersions(const std::string &version) const;

	//specified url was unavailable, remove it from the list of available urls
	void RemoveFailingUrl(const std::string &url);
};
