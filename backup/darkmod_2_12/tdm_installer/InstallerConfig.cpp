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
#include "InstallerConfig.h"
#include <random>
#include <functional>
#include <set>
#include <time.h>
#include "LogUtils.h"
#include "StdString.h"
#include "Path.h"
#include "Constants.h"
#include "State.h"

void InstallerConfig::Clear() {
	_mirrors.clear();
	_versions.clear();
	_defaultVersion.clear();
}

InstallerConfig::Version *InstallerConfig::FindVersion(const std::string &versionName, bool mustExist) {
	for (Version &ver : _versions) {
		if (ver._name == versionName)
			return &ver;
	}
	ZipSyncAssertF(!mustExist, "Can't find version %s", versionName.c_str());
	return nullptr;
}
const InstallerConfig::Version *InstallerConfig::FindVersion(const std::string &versionName, bool mustExist) const {
	return const_cast<InstallerConfig*>(this)->FindVersion(versionName, mustExist);
}

void InstallerConfig::InitFromIni(const ZipSync::IniData &iniData) {
	Clear();

	//pass 1: read everything to our structures
	for (const auto &pNS : iniData) {
		const std::string &secHeader = pNS.first;
		const ZipSync::IniSect &secData = pNS.second;

		//split into section class and name
		int pos = (int)secHeader.find(' ');
		ZipSyncAssertF(pos > 0, "No space in INI section header: \"%s\"", secHeader.c_str());
		std::string secClass = secHeader.substr(0, pos);
		std::string secName = secHeader.substr(pos+1);

		if (secClass == "Mirror") {
			ZipSyncAssertF(secName.size() > 0, "Mirror with empty name");
			ZipSyncAssertF(_mirrors.count(secName) == 0, "Mirror %s: described in INI twice", secName.c_str());

			Mirror mirror;
			mirror._ini = secData;
			mirror._name = secName;
			for (const auto &pKV : secData) {
				const std::string &key = pKV.first;
				const std::string &value = pKV.second;
				if (key == "url")
					mirror._url = value;
				else if (key == "weight")
					mirror._weight = stod(value);
				else if (key == "hidden")
					mirror._hidden = true;
				else {
					ZipSyncAssertF(false, "Mirror %s: unexpected key \"%s\"", secName.c_str(), key.c_str());
				}
			}
			ZipSyncAssertF(!mirror._url.empty(), "Mirror %s: url not set or empty", secName.c_str());
			ZipSyncAssertF(mirror._weight >= 0.0, "Mirror %s: weight < 0 or not set", secName.c_str());
			_mirrors[mirror._name] = std::move(mirror);
		}
		else if (secClass == "Version") {
			ZipSyncAssertF(!FindVersion(secName), "Version %s: described in INI twice", secName.c_str());
			Version ver;
			ver._name = secName;
			ver._ini = secData;
			for (const auto &pKV : secData) {
				const std::string &key = pKV.first;
				const std::string &value = pKV.second;
				if (key == "folder") {
					ZipSyncAssertF(ver._folderPath.empty(), "Version %s: folder described twice", ver._name.c_str());
					stdext::split(ver._folderPath, value, "/");
				}
				else if (key == "default") {
					ZipSyncAssertF(_defaultVersion.empty(), "Two versions marked as default");
					_defaultVersion = ver._name;
				}
				else if (key == "manifestUrl" || stdext::starts_with(key, "manifestUrl_")) {
					ProcessedUrl addUrl;
					addUrl._url = value;
					addUrl._weight = 1.0;
					ver._manifestUrls.push_back(addUrl);
				}
				else if (key == "depends" || stdext::starts_with(key, "depends_")) {
					ver._depends.push_back(value);
				}
				else {
					ZipSyncAssertF(false, "Version %s: unexpected key \"%s\"", ver._name.c_str(), key.c_str());
				}
			}
			ZipSyncAssertF(!ver._folderPath.empty(), "Version %s: folder not specified", ver._name.c_str());
			ZipSyncAssertF(!ver._manifestUrls.empty(), "Version %s: no manifestUrl-s", ver._name.c_str());
			_versions.push_back(std::move(ver));
		}
		else {
			ZipSyncAssertF(false, "Unknown INI section class \"%s\"", secClass.c_str());
		}
	}
	ZipSyncAssertF(!_defaultVersion.empty(), "No default version specified in INI");

	//pass 2: resolve manifestUrls and verify depends
	for (Version &ver : _versions) {
		//process mirrors
		std::vector<ProcessedUrl> newUrls;
		for (const ProcessedUrl &url : ver._manifestUrls) {
			//syntax: ${MIRROR} at the beginning of URL
			if (stdext::starts_with(url._url, "${MIRROR}")) {
				std::string tail = url._url.substr(9);
				for (const auto &pKV : _mirrors) {
					const Mirror &m = pKV.second;
					ProcessedUrl addUrl;
					addUrl._url = m._url + tail;
					addUrl._weight = url._weight;
					addUrl._mirrorName = m._name;
					newUrls.push_back(addUrl);
				}
			}
			else {
				newUrls.push_back(url);
			}
		}
		for (const ProcessedUrl &url : newUrls) {
			ZipSyncAssertF(ZipSync::PathAR::IsHttp(url._url), "Version %s: manifest URL is not recognized as HTTP", ver._name.c_str());
		}
		ver._manifestUrls = std::move(newUrls);

		int trustedCnt = 0;
		for (const ProcessedUrl &url : ver._manifestUrls) {
			if (IsUrlTrusted(url._url))
				trustedCnt++;
		}
		ZipSyncAssertF(trustedCnt > 0, "Version %s: has no manifest URL at trusted location", ver._name.c_str());

		for (const std::string &dep : ver._depends) {
			ZipSyncAssertF(FindVersion(dep), "Version %s: depends on missing Version %s", ver._name.c_str(), dep.c_str());
		}
	}

	//pass 3: compute provided manifests for every version, check for dependency cycles
	for (Version &ver : _versions) {
		std::vector<std::string> providedVersions;
		std::set<std::string> inRecursion;
		std::function<void(const std::string &)> TraverseDependencies = [&](const std::string &version) -> void {
			ZipSyncAssertF(inRecursion.count(version) == 0, "Version %s: at dependency cycle", version.c_str());
			inRecursion.insert(version);

			if (std::count(providedVersions.begin(), providedVersions.end(), version))
				return;
			providedVersions.push_back(version);

			const auto &depends = FindVersion(version, true)->_depends;
			for (const std::string &dep : depends) {
				TraverseDependencies(dep);
			}

			inRecursion.erase(version);
		};
		TraverseDependencies(ver._name);

		ver._providedVersions = std::move(providedVersions);
	}
}

std::vector<std::string> InstallerConfig::GetAllVersions() const {
	std::vector<std::string> res;
	for (const Version &ver : _versions)
		res.push_back(ver._name);
	return res;
}

std::vector<std::string> InstallerConfig::GetAllMirrors() const {
	std::vector<std::string> res;
	for (const auto &pNV : _mirrors) {
		if (pNV.second._hidden)
			continue;	//omit mirrors marked as "hidden" in config file
		res.push_back(pNV.first);
	}
	return res;
}

std::vector<std::string> InstallerConfig::GetFolderPath(const std::string &version) const {
	return FindVersion(version, true)->_folderPath;
}

std::string InstallerConfig::GetDefaultVersion() const {
	ZipSyncAssertF(!_defaultVersion.empty(), "Cannot return default version: %s was not read", TDM_INSTALLER_CONFIG_FILENAME);
	return _defaultVersion;
}

bool InstallerConfig::IsUrlTrusted(const std::string &url) const {
	return stdext::starts_with(url, TDM_INSTALLER_TRUSTED_URL_PREFIX);
}

double InstallerConfig::GetUrlWeight(const ProcessedUrl &url) const {
	double weight = url._weight;
	if (!url._mirrorName.empty()) {
		const Mirror &mirror = _mirrors.at(url._mirrorName);
		weight *= mirror._weight;
	}
	//check if this mirror is selected as "preferred"
	if (g_state->_preferredMirror == url._mirrorName)
		weight *= 1e+9;	//take this one whenever possible!
	return weight;
}

std::string InstallerConfig::ChooseManifestUrl(const std::string &version, bool trusted) const {
	static std::mt19937 MirrorChoosingRandom((int)time(0) ^ clock());	//RNG here!!!

	if (ZipSync::PathAR::IsHttp(version))
		return version;	//this is custom manifest, not a version

	const Version &ver = *FindVersion(version, true);
	std::vector<ProcessedUrl> candidates;
	if (trusted) {
		for (const ProcessedUrl &url : ver._manifestUrls)
			if (IsUrlTrusted(url._url))
				candidates.push_back(url);
	}
	else {
		candidates = ver._manifestUrls;
	}

	double sum = 0.0;
	for (const ProcessedUrl &url : candidates)
		sum += GetUrlWeight(url);
	sum = std::max(sum, 1e-6);
	ZipSyncAssertF(candidates.size(), "No candidates to choose url from (version %s)", version.c_str());

	double param = (MirrorChoosingRandom() % 1000000) * 1e-6 * sum;
	const ProcessedUrl *chosen = &candidates[0];
	for (const ProcessedUrl &url : candidates) {
		param -= GetUrlWeight(url);
		if (param < 0.0) {
			chosen = &url;
			break;
		}
	}
	return chosen->_url;
}

std::vector<std::string> InstallerConfig::GetProvidedVersions(const std::string &version) const {
	return FindVersion(version, true)->_providedVersions;
}

void InstallerConfig::RemoveFailingUrl(const std::string &badUrl) {
	int cntRemoved = 0;

	for (Version &ver : _versions) {
		std::vector<ProcessedUrl> remainUrls;
		for (ProcessedUrl maniUrl : ver._manifestUrls) {
			if (maniUrl._url == badUrl) {
				g_logger->warningf("Removed manifest URL %s from version %s", maniUrl._url.c_str(), ver._name.c_str());
				cntRemoved++;
			}
			else
				remainUrls.push_back(maniUrl);
		}
		ver._manifestUrls = std::move(remainUrls);
	}

	ZipSyncAssertF(cntRemoved > 0, "Cannot handle nonworking URL %s", badUrl.c_str());
}
