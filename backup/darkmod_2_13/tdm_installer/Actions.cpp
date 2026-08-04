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
#include "Actions.h"
#include "StdFilesystem.h"
#include "StdString.h"
#include "LogUtils.h"
#include "OsUtils.h"
#include "CommandLine.h"
#include "ChecksummedZip.h"
#include "Downloader.h"
#include "Utils.h"
#include "ZipSync.h"
#include "Constants.h"
#include "StoredState.h"
#include "State.h"


//small wrapper to share common initialization
struct Downloader : public ZipSync::Downloader {
	Downloader(ZipSync::ProgressIndicator *progress = nullptr) {
		SetUserAgent(TDM_INSTALLER_USERAGENT);
		if (progress)
			SetProgressCallback(progress->GetDownloaderCallback());
	}
};

//=======================================================================================

static std::vector<std::string> CollectTdmZipPaths(const std::string &installDir) {
	std::vector<std::string> lastInstallZips = g_state->_lastInstall.GetOwnedZips();

	std::vector<std::string> res;
	std::vector<std::string> allPaths = ZipSync::EnumerateFilesInDirectory(installDir);
	for (const auto &entry : allPaths) {
		std::string relPath = entry;
		std::string absPath = (stdext::path(installDir) / entry).string();
		bool managed = false;

		//common categories:
		if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. tdm_ai_base01.pk4
		if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".zip"))
			managed = true;		//e.g. tdm_shared_stuff.zip
		if (stdext::istarts_with(relPath, "fms/tdm_") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. fms/tdm_training_mission/tdm_training_mission.pk4

		//hardcoded prepackaged FMs:
		if (stdext::istarts_with(relPath, "fms/newjob/") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. fms/newjob/newjob.pk4
		if (stdext::istarts_with(relPath, "fms/stlucia/") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. fms/stlucia/stlucia.pk4
		if (stdext::istarts_with(relPath, "fms/saintlucia/") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. fms/saintlucia/saintlucia.pk4
		if (stdext::istarts_with(relPath, "fms/training_mission/") && stdext::iends_with(relPath, ".pk4"))
			managed = true;		//e.g. fms/training_mission/training_mission.pk4

		for (const auto &s : lastInstallZips)
			if (relPath == s)
				managed = true;	//managed by last install

		if (managed)
			res.push_back(absPath);
	}
	return res;
}

static std::vector<std::string> CollectFilesInList(const std::string &installDir, const std::vector<std::string> &filenames) {
	std::vector<std::string> res;
	std::vector<std::string> allPaths = ZipSync::EnumerateFilesInDirectory(installDir);
	for (const auto &entry : allPaths) {
		std::string relPath = entry;
		std::string absPath = (stdext::path(installDir) / entry).string();

		bool matches = false;
		for (const auto &fn : filenames)
			if (relPath == fn)
				matches = true;

		if (matches)
			res.push_back(absPath);
	}
	return res;
}
std::vector<std::string> WrapStringList(const char *strlist[]) {
	std::vector<std::string> res;
	for (int i = 0; strlist[i]; i++)
		res.push_back(strlist[i]);
	return res;
}

static const char *TDM_DELETE_ON_INSTALL[] = {
	//Windows executables
	"TheDarkMod.exe",
	"TheDarkModx64.exe",
	//Windows DLLs (2.06)
	"ExtLibs.dll",
	"ExtLibsx64.dll",
	//Linux executables
	"thedarkmod.x86",
	"thedarkmod.x64",
	//game DLLs (2.05 and before)
	"gamex86.dll",
	"gamex86.so",
	//old tdm_update (not supported since 2.09a)
	"tdm_update.exe",
	"tdm_update.linux",
	"tdm_update.linux64",
	nullptr
};
static std::vector<std::string> CollectTdmUnpackedFilesToDelete(const std::string &installDir) {
	auto alwaysDeleted = WrapStringList(TDM_DELETE_ON_INSTALL);
	auto lastInstallDeleted = g_state->_lastInstall.GetOwnedUnpackedFiles();
	auto all = alwaysDeleted;
	all.insert(all.end(), lastInstallDeleted.begin(), lastInstallDeleted.end());
	return CollectFilesInList(installDir, all);
}

static const char *TDM_MARK_EXECUTABLE[] = {
	//Linux executables
	"thedarkmod.x86",
	"thedarkmod.x64",
	nullptr
};
static std::vector<std::string> CollectTdmUnpackedFilesMarkExecutable(const std::string &installDir) {
	return CollectFilesInList(installDir, WrapStringList(TDM_MARK_EXECUTABLE));
}


static const char *ZIPS_TO_UNPACK[] = {"tdm_shared_stuff.zip"};
static int ZIPS_TO_UNPACK_NUM = sizeof(ZIPS_TO_UNPACK) / sizeof(ZIPS_TO_UNPACK[0]);

//=======================================================================================

void Actions::RestartWithInstallDir(const std::string &installDir) {
	g_logger->infof("Restarting TDM installer in directory: %s", installDir.c_str());

	if (!stdext::is_directory(installDir)) {
		g_logger->debugf("Creating missing directories for restart");
		if (!stdext::create_directories(installDir) || !stdext::is_directory(installDir))
			g_logger->errorf("Failed to create missing directories for restart");
	}

	std::string oldExePath = OsUtils::GetExecutablePath();
	std::string newExePath = (stdext::path(installDir) / OsUtils::GetExecutableName()).string();
	if (stdext::equivalent(oldExePath, newExePath))
		g_logger->debugf("Old and new paths are equivalent: \"%s\" === \"%s\"", oldExePath.c_str(), newExePath.c_str());
	else {
		g_logger->debugf("Copying updater to new install directory: \"%s\" -> \"%s\"", oldExePath.c_str(), newExePath.c_str());
		if (stdext::is_regular_file(newExePath))
			stdext::remove(newExePath);
		stdext::copy_file(oldExePath, newExePath);
	}

	OsUtils::ReplaceAndRestartExecutable(newExePath, "");
}

std::vector<std::string> Actions::CheckSpaceAndPermissions(const std::string &installDir) {
	std::vector<std::string> warnings;

	stdext::path currPath = installDir;
	while (!stdext::is_directory(currPath)) {
		//note: this is possible is user entered arbitrary path and clicked "Create and Restart"
		auto parentPath = currPath.parent_path();
		ZipSyncAssertF(parentPath != currPath, "Invalid installation directory %s: no ancestor directory exists", installDir.c_str());
		currPath = parentPath;
	};
	std::string lastExistingDir = currPath.string();

	std::string checkPath = (stdext::path(lastExistingDir) / "__permissions_test").string();
	g_logger->infof("Test writing file %s", checkPath.c_str());
	FILE *f = fopen(checkPath.c_str(), "wt");
	ZipSyncAssertF(f,
		"Cannot write anything in installation directory \"%s\".\n"
		"Please choose a directory where files can be created without admin rights.\n"
		"Directory \"C:\\Games\\TheDarkMod\" is a usually good.\n"
		"Do NOT try to install TheDarkMod into Program Files! ",
		installDir.c_str()
	);
	fclose(f);
	stdext::remove(checkPath);

	//filesystem::space returns free space module 2^32 bytes on 32-bit Linux
	//so we have to disable free space check for it
	//see also: https://forums.thedarkmod.com/index.php?/topic/20460-new-tdm_installer-and-dev-builds/&do=findComment&comment=456147
	bool isLinux32 = false;
#ifndef _WIN32
	if (sizeof(void*) == 4)
		isLinux32 = true;
#endif

	if (!isLinux32) {
		g_logger->infof("Checking for free space at %s", lastExistingDir.c_str());
		uint64_t freeSpace = OsUtils::GetAvailableDiskSpace(lastExistingDir) >> 20;
		ZipSyncAssertF(freeSpace >= TDM_INSTALLER_FREESPACE_MINIMUM, 
			"Only %0.0lf MB of free space is available in installation directory.\n"
			"Installer surely won't work without at least %0.0lf MB of free space!",
			freeSpace / 1.0, TDM_INSTALLER_FREESPACE_MINIMUM / 1.0
		);
		if (freeSpace < TDM_INSTALLER_FREESPACE_RECOMMENDED) {
			warnings.push_back(ZipSync::formatMessage(
				"Only %0.2lf GB of free space is available in installation directory.\n"
				"Installation or update can fail due to lack of space.\n"
				"Better free at least %0.2lf GB and restart updater.\n",
				freeSpace / 1024.0, TDM_INSTALLER_FREESPACE_RECOMMENDED / 1024.0
			));
		}
	}

	return warnings;
}

void Actions::StartLogFile() {
	//from now on, write logs to a logfile in CWD
	delete g_logger;
	auto myLogger = new LoggerTdm();
	g_logger = myLogger;
	myLogger->Init();
	g_logger->infof("Install directory: %s", OsUtils::GetCwd().c_str());
}

bool Actions::NeedsSelfUpdate(ZipSync::ProgressIndicator *progress) {
	std::string exePath = OsUtils::GetExecutablePath();
	std::string exeTempPath = OsUtils::GetExecutablePath() + ".__temp__";
	std::string exeZipPath = OsUtils::GetExecutablePath() + ".__temp__.zip";
	std::string exeUrl = std::string(TDM_INSTALLER_EXECUTABLE_URL_PREFIX) + TDM_INSTALLER_EXECUTABLE_FILENAME + ".zip";

	ZipSync::HashDigest myHash;
	{
		g_logger->infof("Computing hash of myself at %s...", exePath.c_str());
		std::vector<uint8_t> selfExeData = ZipSync::ReadWholeFile(exePath);
		myHash = ZipSync::Hasher().Update(selfExeData.data(), selfExeData.size()).Finalize();
		g_logger->infof("My hash is %s", myHash.Hex().c_str());
	}

	//fast pass: download hash of updater
	g_logger->infof("Checking installer executable at %s...", exeUrl.c_str());
	Downloader downloaderPreliminary(progress);
	ZipSync::HashDigest desiredHash = ZipSync::GetHashesOfRemoteChecksummedZips(downloaderPreliminary, {exeUrl})[0];
	g_logger->infof("Downloaded bytes: %lld", downloaderPreliminary.TotalBytesDownloaded());
	g_logger->infof("Hash of installer on server is %s", desiredHash.Hex().c_str());

	if (myHash == desiredHash) {
		g_logger->infof("Hashes match, update not needed");
		return false;
	}
	else {
		//second pass: download full manifests when necessary
		g_logger->infof("Downloading installer executable from %s...", exeUrl.c_str());
		Downloader downloaderFull(progress);
		std::vector<std::string> outPaths = {exeZipPath};
		ZipSync::DownloadChecksummedZips(downloaderFull, {exeUrl}, {desiredHash}, {}, outPaths);
		g_logger->infof("Downloaded bytes: %lld", downloaderFull.TotalBytesDownloaded());
		ZipSyncAssert(exeZipPath == outPaths[0]);

		g_logger->infof("Unpacking data from %s to temporary file %s", exeZipPath.c_str(), exeTempPath.c_str());
		std::vector<uint8_t> data = ZipSync::ReadChecksummedZip(exeZipPath.c_str(), TDM_INSTALLER_EXECUTABLE_FILENAME);
		{
			ZipSync::StdioFileHolder f(exeTempPath.c_str(), "wb");
			int wr = fwrite(data.data(), 1, data.size(), f);
			ZipSyncAssert(wr == data.size());
		}
		stdext::remove(exeZipPath);

		g_logger->infof("");
		return true;
	}
}
void Actions::DoSelfUpdate() {
	std::string exePath = OsUtils::GetExecutablePath();
	std::string exeTempPath = OsUtils::GetExecutablePath() + ".__temp__";
	//replace executable and rerun it
	g_logger->infof("Replacing and restarting myself...");
	OsUtils::ReplaceAndRestartExecutable(exePath, exeTempPath);
}


void Actions::ReadConfigFile(bool download, ZipSync::ProgressIndicator *progress) {
	g_state->_config.Clear();

	if (download) {
		g_logger->infof("Downloading config file from %s...", TDM_INSTALLER_CONFIG_URL);
		Downloader downloader(progress);
		auto DataCallback = [](const void *data, int len) {
			ZipSync::StdioFileHolder f(TDM_INSTALLER_CONFIG_FILENAME, "wb");
			int res = fwrite(data, 1, len, f);
			ZipSyncAssert(res == len);
		};
		downloader.EnqueueDownload(ZipSync::DownloadSource(TDM_INSTALLER_CONFIG_URL), DataCallback);
		downloader.DownloadAll();
		g_logger->infof("Downloaded bytes: %lld", downloader.TotalBytesDownloaded());
	}

	g_logger->infof("Reading INI file %s", TDM_INSTALLER_CONFIG_FILENAME);
	//read the file (throws exception if not present)
	auto iniData = ZipSync::ReadIniFile(TDM_INSTALLER_CONFIG_FILENAME);
	//analyze and check it
	try {
		g_logger->infof("Loading installer config from it");
		g_state->_config.InitFromIni(iniData);
	}
	catch(...) {
		//make sure corrupted config does not remain after error
		g_state->_config.Clear();
		throw;
	}
	g_logger->infof("");
}

static void ReadLastInstallInfo() {
	g_state->_lastInstall = InstallState();
	try {
		//read last scan state, also last installed version
		g_logger->infof("Trying to read %s", TDM_INSTALLER_LASTINSTALL_PATH);
		ZipSync::IniData ini = ZipSync::ReadIniFile(TDM_INSTALLER_LASTINSTALL_PATH);
		g_state->_lastInstall = InstallState::ReadFromIni(ini);
		g_logger->infof("Last install state read successfully");
	}
	catch(const std::exception &) {
		g_logger->infof("Failed to read it: last install unknown");
	}
}
static bool IfChangedSinceLastScan(const std::string &root, const std::vector<std::string> &scanSet) {
	bool doScan = true;
	try {
		//read last scan state, also last installed version
		g_logger->infof("Trying to read %s", TDM_INSTALLER_LASTSCAN_PATH);
		ZipSync::IniData ini = ZipSync::ReadIniFile(TDM_INSTALLER_LASTSCAN_PATH);
		ScanState lastScan = ScanState::ReadFromIni(ini);
		g_logger->infof("Last scan state read successfully");

		//generate current scan state
		ScanState nowScan = ScanState::ScanZipSet(scanSet, root);

		if (nowScan.NotChangedSince(lastScan)) {
			doScan = false;
			g_logger->infof("Zips seem to have not changed since last time");
		}
		else {
			g_logger->infof("Perform scan because something changed since last run");
		}
	}
	catch(const std::exception &) {
		g_logger->infof("Failed to read it, fallback to scanning");
		g_state->_localManifest.Clear();
		doScan = true;
	}
	return doScan;
}
void Actions::ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress) {
	g_state->_localManifest.Clear();

	std::string root = OsUtils::GetCwd();
	g_logger->infof("Cleaning temporary files");	//possibly remained from previous run
	ZipSync::DoClean(root);

	ReadLastInstallInfo();

	g_logger->infof("Collecting set of already existing TDM-owned zips");
	std::vector<std::string> managedZips = CollectTdmZipPaths(root);
	std::vector<std::string> managedZipsAndMani = managedZips;
	managedZipsAndMani.push_back((stdext::path(root) / TDM_INSTALLER_LOCAL_MANIFEST).string());

	bool doScan = IfChangedSinceLastScan(root, managedZipsAndMani);
	if (force) {
		g_logger->infof("Do scanning because forced by user");
		doScan = true;
	}

	if (!doScan) {
		//read existing local manifest
		g_logger->infof("Trying to read local %s", TDM_INSTALLER_LOCAL_MANIFEST);
		ZipSync::IniData ini = ZipSync::ReadIniFile(TDM_INSTALLER_LOCAL_MANIFEST);
		g_state->_localManifest.ReadFromIni(ini, OsUtils::GetCwd());
		g_logger->infof("Local manifest read successfully");

		g_logger->infof("");
	}
	else {
		g_logger->infof("Installation currently contains of %d TDM-owned zips", managedZips.size());
		uint64_t totalSize = 0;
		for (const std::string &mzip : managedZips) {
			g_logger->infof("  %s", mzip.c_str());
			totalSize += ZipSync::SizeOfFile(mzip);
		}
		g_logger->infof("Total size of managed zips: %0.0lf MB", totalSize * 1e-6);

		g_logger->infof("Analysing the archives");
		ZipSync::Manifest manifest = ZipSync::DoAnalyze(root, managedZips, true, 1, progress);
		g_logger->infof("Saving results of analysis to manifest file");
		ZipSync::WriteIniFile((root + "/manifest.iniz").c_str(), manifest.WriteToIni());
		g_state->_localManifest = std::move(manifest);

		g_logger->infof("Saving current scan at %s", TDM_INSTALLER_LASTSCAN_PATH);
		ScanState nowScan = ScanState::ScanZipSet(managedZipsAndMani, root);
		ZipSync::IniData ini = nowScan.WriteToIni();
		stdext::create_directories(stdext::path(TDM_INSTALLER_LASTSCAN_PATH).parent_path());
		ZipSync::WriteIniFile(TDM_INSTALLER_LASTSCAN_PATH, ini);

		g_logger->infof("");
	}

	std::string message = OsUtils::CanModifyFiles(managedZipsAndMani, false);
	ZipSyncAssertF(message == "", "%s.\nPlease make sure no TheDarkMod-related programs are running and try again.", message.c_str());
}

Actions::VersionInfo Actions::RefreshVersionInfo(const std::string &targetVersion, const std::string &customManifestUrl, bool bitwiseExact, ZipSync::ProgressIndicator *progress) {
	std::string root = OsUtils::GetCwd();
	g_logger->infof("Evaluating version %s", targetVersion.c_str());
	if (!customManifestUrl.empty())
		g_logger->infof("With custom manifest URL: %s", customManifestUrl.c_str());
	if (!g_state->_preferredMirror.empty())
		g_logger->infof("Preferred mirror: %s", g_state->_preferredMirror.c_str());

	//these arrays are same-indexed
	//0-th manifest is "target" one, all the others are "provided"
	std::vector<std::string> versions;
	std::vector<bool> areLoaded;
	std::vector<std::string> urls;

	//set target version first
	if (customManifestUrl.empty()) {
		//note: target manifest always comes from trusted source
		versions.push_back(targetVersion + "$$" + "trusted");
		urls.push_back(g_state->_config.ChooseManifestUrl(targetVersion, true));
	}
	else {
		versions.push_back(customManifestUrl);
		urls.push_back(customManifestUrl);
	}
	//append provided versions them
	if (!customManifestUrl.empty())
		versions.push_back(customManifestUrl);
	for (std::string ver : g_state->_config.GetProvidedVersions(targetVersion))
		versions.push_back(ver);
	int n = versions.size();
	urls.resize(n);

	//see which manifests were not loaded in this updater session
	for (int i = 0; i < n; i++) {
		bool isLoaded = g_state->_loadedManifests.count(versions[i]);
		areLoaded.push_back(isLoaded);
	}
		
	g_logger->infof("Target manifest at %s", urls[0].c_str());
	g_logger->infof("Version %s needs files from %d versions", targetVersion.c_str(), int(versions.size() - 1));
	for (int i = 1; i < versions.size(); i++)
		g_logger->debugf("  %s %s", versions[i].c_str(), (areLoaded[i] ? "(loaded)" : ""));

	while (int downloadCnt = std::count(areLoaded.begin(), areLoaded.end(), false)) {
		//inspect local cache of manifests
		std::string cacheDir = TDM_INSTALLER_ZIPSYNC_DIR "/" TDM_INSTALLER_MANICACHE_SUBDIR;
		stdext::create_directories(cacheDir);

		//detect existing manifests and names for new ones
		g_logger->infof("Looking into manifests cache");
		std::vector<std::string> cachedManiNames, newManiNames;
		for (int id = 0; id < 1000 || newManiNames.size() < downloadCnt; id++) {
			std::string filename = cacheDir + "/" + std::to_string(id) + ".iniz";
			bool valid = false;
			if (stdext::is_regular_file(filename)) {
				try {
					//open file to check if it is valid
					ZipSync::UnzFileHolder zf(filename.c_str());
					valid = true;
					cachedManiNames.push_back(filename);
				} catch(ZipSync::ErrorException &e) {
					g_logger->infof("Removing %s from manifest cache since it is not a valid zip", filename.c_str());
					stdext::remove(filename);
				}
			}
			if (!valid)
				newManiNames.push_back(filename);
		}
		g_logger->infof("Detected %d manifests in cache", (int)cachedManiNames.size());

		//get list of urls we need to download from
		std::vector<std::string> downloadUrls;
		g_logger->infof("Need to download manifests:");
		for (int i = 0; i < n; i++) if (!areLoaded[i]) {
			if (i > 0)
				urls[i] = g_state->_config.ChooseManifestUrl(versions[i]);
			downloadUrls.push_back(urls[i]);
			g_logger->infof("  %s at %s", versions[i].c_str(), urls[i].c_str());
		}

		//fast pass: download hashes of all manifests
		g_logger->infof("Downloading hashes of remote manifests");
		Downloader downloaderPreliminary(progress);
		downloaderPreliminary.SetErrorMode(true);	//skip url on download error!
		std::vector<ZipSync::HashDigest> allHashes = ZipSync::GetHashesOfRemoteChecksummedZips(downloaderPreliminary, downloadUrls);
		g_logger->infof("Downloaded bytes: %lld", downloaderPreliminary.TotalBytesDownloaded());

		//filter only manifests which were downloaded successfully
		//remove urls which failed to download from config
		ZipSync::HashDigest zeroHash;
		zeroHash.Clear();
		int failCnt = std::count(allHashes.begin(), allHashes.end(), zeroHash);
		if (failCnt > 0) {
			g_logger->infof("Failed to download %d manifests", failCnt);
			int k = 0;
			for (int i = 0; i < allHashes.size(); i++) {
				if (allHashes[i] == zeroHash) {
					//failed to download hash, consider it to be "mirror is down" situation
					//throw away failing url for future retries
					g_state->_config.RemoveFailingUrl(downloadUrls[i]);
				}
				else {
					allHashes[k] = allHashes[i];
					downloadUrls[k] = downloadUrls[i];
					k++;
				}
			}
			allHashes.resize(k);
			downloadUrls.resize(k);
		}

		g_logger->infof("Downloading actual remote manifests");
		for (int i = 0; i < allHashes.size(); i++) {
			ZipSyncAssert(!(allHashes[i] == zeroHash));
			g_logger->infof("  (%s) %s -> %s", allHashes[i].Hex().c_str(), downloadUrls[i].c_str(), newManiNames[i].c_str());
		}

		//second pass: download full manifests when necessary
		Downloader downloaderFull(progress);
		newManiNames.resize(downloadUrls.size());
		std::vector<int> matching = ZipSync::DownloadChecksummedZips(downloaderFull, downloadUrls, allHashes, cachedManiNames, newManiNames);
		g_logger->infof("Downloaded bytes: %lld", downloaderFull.TotalBytesDownloaded());

		g_logger->infof("Downloaded manifests successfully");
		for (int i = 0; i < allHashes.size(); i++)
			g_logger->infof("  (%s) %s -> %s", allHashes[i].Hex().c_str(), downloadUrls[i].c_str(), newManiNames[i].c_str());

		g_logger->infof("Loading downloaded manifests");
		for (int i = 0; i < allHashes.size(); i++) {
			if (progress)
				progress->Update(double(i) / allHashes.size(), ZipSync::formatMessage("Loading manifest \"%s\"...", newManiNames[i].c_str()));
			int pos = -1;
			for (int j = 0; j < n; j++)
				if (!areLoaded[j] && urls[j] == downloadUrls[i])
					pos = j;
			ZipSyncAssert(pos >= 0);
			ZipSync::IniData ini = ZipSync::ReadIniFile(newManiNames[i].c_str());
			ZipSync::Manifest mani;
			mani.ReadFromIni(std::move(ini), root);
			mani.ReRoot(ZipSync::GetDirPath(urls[pos]));
			g_state->_loadedManifests[versions[pos]] = std::move(mani);
			areLoaded[pos] = true;
			if (progress)
				progress->Update(double(i+1) / allHashes.size(), ZipSync::formatMessage("Manifest \"%s\" loaded", newManiNames[i].c_str()));
		}
		if (progress)
			progress->Update(1.0, "Manifests loaded");
	}

	//look at locally present zips
	std::vector<std::string> ownedZips = CollectTdmZipPaths(root);
	g_logger->infof("There are %d TDM-owned zips locally", int(ownedZips.size()));
	for (int i = 0; i < ownedZips.size(); i++)
		g_logger->debugf("  %s", ownedZips[i].c_str());

	//gather full manifests for update
	ZipSync::Manifest targetMani = g_state->_loadedManifests.at(versions[0]);
	ZipSync::Manifest providMani = g_state->_localManifest;
	for (int i = 1; i < versions.size(); i++) {
		const std::string &ver = versions[i];
		const ZipSync::Manifest &mani = g_state->_loadedManifests.at(ver);
		ZipSync::Manifest added = mani.Filter([](const ZipSync::FileMetainfo &mf) -> bool {
			return mf.location != ZipSync::FileLocation::Nowhere;
		});
		providMani.AppendManifest(added);
	}
	g_logger->infof("Collected full manifest: target (%d files) and provided (%d files)", (int)targetMani.size(), (int)providMani.size());

	//develop update plan
	g_logger->infof("Developing update plan");
	std::unique_ptr<ZipSync::UpdateProcess> updater(new ZipSync::UpdateProcess());
	updater->Init(targetMani, providMani, root);
	for (const std::string &path : ownedZips)
		updater->AddManagedZip(path);
	auto updateType = (bitwiseExact ? ZipSync::UpdateType::SameCompressed : ZipSync::UpdateType::SameContents);
	bool ok = updater->DevelopPlan(updateType);

	//compute stats
	Actions::VersionInfo info;
	for (int i = 0; i < ownedZips.size(); i++)
		info.currentSize += ZipSync::SizeOfFile(ownedZips[i]);
	for (int i = 0; i < updater->MatchCount(); i++) {
		auto m = updater->GetMatch(i);
		uint32_t sz = m.target->props.compressedSize;
		info.finalSize += sz;
		if (m.provided) {
			if (m.provided->location == ZipSync::FileLocation::RemoteHttp)
				info.downloadSize += sz;
			if (m.provided->location != ZipSync::FileLocation::Inplace)
				info.addedSize += sz;
		}
		else {
			info.missingSize += sz;
		}
	}
	ZipSyncAssert(ok == (info.missingSize == 0));	//always so by construction
	uint64_t remainsSize = info.finalSize - info.addedSize - info.missingSize;
	info.removedSize = info.currentSize - remainsSize;
	g_logger->infof("Update statistics:");
	g_logger->infof("  current: %llu", info.currentSize);
	g_logger->infof("  final: %llu", info.finalSize);
	g_logger->infof("  download: %llu", info.downloadSize);
	g_logger->infof("  added: %llu", info.addedSize);
	g_logger->infof("  removed: %llu", info.removedSize);
	g_logger->infof("  missing: %llu", info.missingSize);
	if (!ok) {
		ZipSyncAssert(!customManifestUrl.empty());
		updater.reset();
	}

	//save updater --- perhaps it would be used to actually perform the update
	g_state->_updater = std::move(updater);
	g_logger->infof("");
	return info;
}

void Actions::PerformInstallDownload(ZipSync::ProgressIndicator *progressDownload, ZipSync::ProgressIndicator *progressVerify, bool blockMultipart) {
	g_logger->infof("Starting installation: download");
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);

	auto callback1 = (progressDownload ? progressDownload->GetDownloaderCallback() : ZipSync::GlobalProgressCallback());
	auto callback2 = (progressVerify ? progressVerify->GetDownloaderCallback() : ZipSync::GlobalProgressCallback());
	uint64_t totalBytesDownloaded = updater->DownloadRemoteFiles(callback1, callback2, TDM_INSTALLER_USERAGENT, blockMultipart);
	g_logger->infof("Downloaded bytes: %lld", totalBytesDownloaded);

	g_logger->infof("");
}

void Actions::PerformInstallRepack(ZipSync::ProgressIndicator *progress) {
	g_logger->infof("Starting installation: repack");
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);

	updater->RepackZips(progress->GetDownloaderCallback());
	g_logger->infof("Repacking finished");

	g_logger->infof("");
}

static std::vector<std::string> UnpackZip(unzFile zf) {
	std::vector<std::string> unpackedFiles;
	SAFE_CALL(unzGoToFirstFile(zf));
	while (1) {
		char currFilename[4096];
		SAFE_CALL(unzGetCurrentFileInfo(zf, NULL, currFilename, sizeof(currFilename), NULL, 0, NULL, 0));
		ZipSync::StdioFileHolder outfile(currFilename, "wb");
		unpackedFiles.push_back(currFilename);

		SAFE_CALL(unzOpenCurrentFile(zf));
		char buffer[64<<10];
		while (1) {
			int read = unzReadCurrentFile(zf, buffer, sizeof(buffer));
			ZipSyncAssert(read >= 0);
			if (read == 0)
				break;
			int wr = fwrite(buffer, 1, read, outfile);
			ZipSyncAssert(read == wr);
		}
		SAFE_CALL(unzCloseCurrentFile(zf));

		int res = unzGoToNextFile(zf);
		if (res == UNZ_END_OF_LIST_OF_FILE)
			break;   //finished
		SAFE_CALL(res);
	}
	return unpackedFiles;
}
void Actions::PerformInstallFinalize(ZipSync::ProgressIndicator *progress) {
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);
	std::string root = OsUtils::GetCwd();

	if (progress)
		progress->Update(0.0, "Saving resulting manifest...");
	ZipSync::Manifest mani = updater->GetProvidedManifest().Filter([](const ZipSync::FileMetainfo &mf) -> bool {
		return mf.location == ZipSync::FileLocation::Inplace;
	});
	ZipSync::WriteIniFile(TDM_INSTALLER_LOCAL_MANIFEST, mani.WriteToIni());
	if (progress)
		progress->Update(0.5, "Manifest saved");

	if (progress)
		progress->Update(0.5, "Cleaning temporary zips...");
	ZipSync::DoClean(root);
	if (progress)
		progress->Update(0.6, "Cleaning finished");

	if (progress)
		progress->Update(0.6, "Deleting old files...");
	std::vector<std::string> delFiles = CollectTdmUnpackedFilesToDelete(root);
	for (const std::string &fn : delFiles)
		stdext::remove(fn);
	if (progress)
		progress->Update(0.7, "Deleting finished");

	std::vector<std::string> unpackedFiles;
	for (int i = 0; i < ZIPS_TO_UNPACK_NUM; i++) {
		const char *fn = ZIPS_TO_UNPACK[i];
		ZipSync::UnzFileHolder zf(unzOpen(fn));
		if (!zf)
			continue;
		if (progress)
			progress->Update(0.7 + 0.1 * (i+0)/ZIPS_TO_UNPACK_NUM, formatMessage("Unpacking %s...", fn).c_str());
		auto filelist = UnpackZip(zf);
		unpackedFiles.insert(unpackedFiles.end(), filelist.begin(), filelist.end());
		if (progress)
			progress->Update(0.7 + 0.1 * (i+1)/ZIPS_TO_UNPACK_NUM, "Unpacking finished");
	}

	//---------------------------------------------------
	std::set<std::string> zipPaths;
	for (int i = 0; i < mani.size(); i++)
		zipPaths.insert(mani[i].zipPath.rel);
	// overwrite install state in memory
	g_state->_lastInstall = InstallState(g_state->_versionRefreshed, std::vector<std::string>(zipPaths.begin(), zipPaths.end()), unpackedFiles);
	//---------------------------------------------------

	if (progress)
		progress->Update(0.8, "Saving " TDM_INSTALLER_LASTSCAN_PATH "...");
	std::vector<std::string> managedZipsAndMani = CollectTdmZipPaths(root);
	managedZipsAndMani.push_back((stdext::path(root) / TDM_INSTALLER_LOCAL_MANIFEST).string());
	ScanState nowScan = ScanState::ScanZipSet(managedZipsAndMani, root);
	ZipSync::IniData ini = nowScan.WriteToIni();
	stdext::create_directories(stdext::path(TDM_INSTALLER_LASTSCAN_PATH).parent_path());
	ZipSync::WriteIniFile(TDM_INSTALLER_LASTSCAN_PATH, ini);
	if (progress)
		progress->Update(0.85, "Saved properly");

	if (progress)
		progress->Update(0.85, "Saving " TDM_INSTALLER_LASTINSTALL_PATH "...");
	ini = g_state->_lastInstall.WriteToIni();
	stdext::create_directories(stdext::path(TDM_INSTALLER_LASTINSTALL_PATH).parent_path());
	ZipSync::WriteIniFile(TDM_INSTALLER_LASTINSTALL_PATH, ini);
	if (progress)
		progress->Update(0.9, "Saved properly");

	if (progress)
		progress->Update(0.9, "Mark as executable...");
	std::vector<std::string> execFiles = CollectTdmUnpackedFilesMarkExecutable(root);
	for (const std::string &fn : execFiles)
		OsUtils::MarkAsExecutable(fn);
	if (progress)
		progress->Update(0.95, "Executables marked");

	if (progress)
		progress->Update(0.95, "Renaming config file...");
	if (stdext::is_regular_file(TDM_DARKMOD_CFG_FILENAME)) {
		std::string filename = FormatFilenameWithDatetime(TDM_DARKMOD_CFG_OLD_FORMAT, "TDM config");
		g_logger->infof("Renaming %s to %s...", TDM_DARKMOD_CFG_FILENAME, filename.c_str());
		if (stdext::is_regular_file(filename))
			g_logger->infof("Failed to rename: destination file already exists.");
		else
			stdext::rename(TDM_DARKMOD_CFG_FILENAME, filename);
		g_state->_oldConfigFilename = filename;
	}
	if (progress)
		progress->Update(1.0, "Renamed config file");

	progress->Update(1.0, "Finalization complete");
	g_logger->infof("");
}

bool Actions::CanRestoreOldConfig() {
	return g_state->_oldConfigFilename.size() > 0;
}
void Actions::DoRestoreOldConfig() {
	ZipSyncAssert(CanRestoreOldConfig());
	g_logger->infof("Restoring %s...", g_state->_oldConfigFilename.c_str());
	if (stdext::is_regular_file(TDM_DARKMOD_CFG_FILENAME)) {
		stdext::remove(TDM_DARKMOD_CFG_FILENAME);
		g_logger->infof("Removed %s", TDM_DARKMOD_CFG_FILENAME);
	}
	stdext::rename(g_state->_oldConfigFilename.c_str(), TDM_DARKMOD_CFG_FILENAME);
	g_state->_oldConfigFilename.clear();
	g_logger->infof("Finished.");
	g_logger->infof("");
}

bool Actions::IfShortcutExists() {
	bool res = OsUtils::IfShortcutExists(TDM_DARKMOD_SHORTCUT_NAME);
	g_logger->infof("");
	return res;
}
void Actions::CreateShortcut() {
	//choose best executable which exists
	static const char *CANDIDATES[] = TDM_DARKMOD_SHORTCUT_EXECUTABLES;
	static const int k = sizeof(CANDIDATES) / sizeof(CANDIDATES[0]);
	std::string exePath;
	for (int i = 0; i < k; i++)
		if (stdext::is_regular_file(CANDIDATES[i])) {
			exePath = CANDIDATES[i];
			break;
		}
	ZipSyncAssertF(!exePath.empty(), "Cannot find any TDM executable");

	OsUtils::ShortcutInfo info;
	info.name = TDM_DARKMOD_SHORTCUT_NAME;
	info.executablePath = exePath;
	info.workingDirPath = OsUtils::GetCwd();
	info.iconPath = TDM_DARKMOD_SHORTCUT_ICON;
	info.comment = TDM_DARKMOD_SHORTCUT_COMMENT;
	OsUtils::CreateShortcut(info);
	g_logger->infof("");
}
