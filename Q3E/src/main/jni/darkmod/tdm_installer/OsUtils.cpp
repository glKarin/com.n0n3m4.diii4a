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
#include "OsUtils.h"
#include "StdFilesystem.h"
#include "Utils.h"
#include "StdString.h"
#include "LogUtils.h"
#include <string.h>
#include <FL/platform.H>

std::string OsUtils::_argv0;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atlbase.h>	//CComPtr
#include <shobjidl.h>	//shortcuts
#include <shlguid.h>	//shortcuts
#include <shlobj.h>		//getting desktop path
#include <Shobjidl.h>

CComPtr<ITaskbarList3> m_spTaskbarList;

static void ThrowWinApiError() {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
	std::string message = (LPCTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);
	g_logger->errorf("(WinAPI error) %s", message.c_str());
}
static void EnsureComInitialized() {
	auto res = CoInitialize(NULL);
	ZipSyncAssertF(res == S_OK || res == S_FALSE, "Failed to initialize COM");
}
#define CHECK_HRESULT(what) do { \
		HRESULT rc = what; \
		if (FAILED(rc)) \
			g_logger->errorf("WinAPI error %d", int(rc)); \
	} while (0)

#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


void OsUtils::InitArgs(const char *argv0) {
	g_logger->debugf("Save command line of current process: \"%s\"", argv0);
	_argv0 = argv0;
}
std::string OsUtils::GetExecutablePath() {
	ZipSyncAssert(!_argv0.empty());
	auto exePath = stdext::canonical(_argv0);
	return exePath.string();
}
std::string OsUtils::GetExecutableDir() {
	ZipSyncAssert(!_argv0.empty());
	auto exeDir = stdext::canonical(_argv0).parent_path();
	return exeDir.string();
}
std::string OsUtils::GetExecutableName() {
	ZipSyncAssert(!_argv0.empty());
	auto exeFn = stdext::path(_argv0).filename();
	return exeFn.string();
}

void OsUtils::ShowSystemProgress(const Fl_Window* window, double ratio) {
#ifdef _WIN32
	if ( !m_spTaskbarList ) {
		EnsureComInitialized();
		HRESULT hr = ::CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
			__uuidof( ITaskbarList3 ), reinterpret_cast<void**>( &m_spTaskbarList ) );

		if ( SUCCEEDED( hr ) ) {
			hr = m_spTaskbarList->HrInit();
		}
	}
	if ( !m_spTaskbarList )
		return;
	HWND h = fl_xid( window );
	if ( !h )
		return;
	if ( ratio >= 0.0 && ratio < 1.0 ) {
		m_spTaskbarList.p->SetProgressState( h, TBPF_NORMAL );
		m_spTaskbarList.p->SetProgressValue( h, int(ratio * 100.0), 100 );
	}
	else {
		m_spTaskbarList.p->SetProgressState( h, TBPF_NOPROGRESS );
	}
#endif
}

std::string OsUtils::GetCwd() {
	return stdext::current_path().string();
}

bool OsUtils::SetCwd(const std::string &newCwd) {
	g_logger->debugf("Setting CWD to \"%s\"", newCwd.c_str());
	if (!stdext::is_directory(newCwd)) {
		g_logger->warningf("Suggested CWD is not an existing directory");
		return false;
	}
	stdext::current_path(newCwd);
	auto cwd = stdext::current_path();
	if (cwd != newCwd) {
		g_logger->warningf("Failed to reset CWD");
		return false;
	}
	return true;
}

void OsUtils::MarkAsExecutable(const std::string &filePath) {
#ifndef _WIN32
	g_logger->debugf("Marking \"%s\" as executable", filePath.c_str());
	struct stat mask;
	stat(filePath.c_str(), &mask);
	mask.st_mode |= S_IXUSR|S_IXGRP|S_IXOTH;
	if (chmod(filePath.c_str(), mask.st_mode) == -1)
		g_logger->errorf("Could not mark file as executable: %s", filePath.c_str());
#endif
}

std::string OsUtils::GetBatchForReplaceAndRestartExecutable(const std::string &targetPath) {
	return targetPath + "__temp__.cmd";
}
void OsUtils::ReplaceAndRestartExecutable(const std::string &targetPath, const std::string &temporaryPath, const std::vector<std::string> &cmdArgs) {
	std::string allArgs = stdext::join(cmdArgs, " ");
	std::string batchFilePath = GetBatchForReplaceAndRestartExecutable(targetPath);
	g_logger->infof("Restarting executable \"%s\" from \"%s\" (arguments: %s)",
		targetPath.c_str(), temporaryPath.c_str(), allArgs.c_str()
	);

	{
		//sadly batch commands like "copy" don't understand slashes =(
		std::string winTargetPath = targetPath;
		std::string winTemporaryPath = temporaryPath;
		for (char &c : winTargetPath)
			if (c == '/')
				c = '\\';
		for (char &c : winTemporaryPath)
			if (c == '/')
				c = '\\';
		g_logger->infof("Creating updating batch/shell file \"%s\"", batchFilePath.c_str());
		ZipSync::StdioFileHolder batchFile(batchFilePath.c_str(), "wt");
#ifdef _WIN32
		fprintf(batchFile, "@ping 127.0.0.1 -n 6 -w 1000 > nul\n"); //hack equivalent to Wait 5
		if (!temporaryPath.empty()) {
			fprintf(batchFile, "@copy \"%s\" \"%s\" >nul\n", winTemporaryPath.c_str(), winTargetPath.c_str());
			fprintf(batchFile, "@del \"%s\"\n", winTemporaryPath.c_str());
			fprintf(batchFile, "@echo Executable has been replaced.\n");
		}
		fprintf(batchFile, "@echo Re-launching executable.\n\n");
		//when quoting argument for @start, we have to add one more ""
		//  https://superuser.com/a/239572
		fprintf(batchFile, "@start \"\" \"%s\" %s\n", winTargetPath.c_str(), allArgs.c_str());
#else //POSIX
		fprintf(batchFile, "#!/bin/bash\n");
		fprintf(batchFile, "sleep 5s\n");
		if (!temporaryPath.empty()) {
			fprintf(batchFile, "mv -f \"%s\" \"%s\"\n", temporaryPath.c_str(), targetPath.c_str());
			fprintf(batchFile, "chmod +x \"%s\"\n", targetPath.c_str());
			fprintf(batchFile, "echo \"Executable has been updated.\"\n");
		}
		fprintf(batchFile, "echo \"Re-launching executable.\"\n");
		fprintf(batchFile, "\"%s\" %s", targetPath.c_str(), allArgs.c_str());
#endif
	}

	//mark the shell script as executable in *nix
	MarkAsExecutable(batchFilePath);

	{
		g_logger->infof("Running updating script");
#ifdef _WIN32
		STARTUPINFO siStartupInfo = {0};
		PROCESS_INFORMATION piProcessInfo = {0};
		siStartupInfo.cb = sizeof(siStartupInfo);
		//start new process
		BOOL success = CreateProcess(
			NULL, (LPSTR)batchFilePath.c_str(), NULL, NULL, false, 0, NULL,
			NULL, &siStartupInfo, &piProcessInfo
		);
		if (!success)
			ThrowWinApiError();
#else //POSIX	
		//perform the system command in a fork
		int code = fork();
		if (code == -1)
			g_logger->errorf("Failed to fork process");
		if (code == 0) {
			//don't wait for the subprocess to finish
			system((batchFilePath + " &").c_str());
			exit(0);
		}
#endif
		g_logger->infof("Updating script started successfully");
	}

	//terminate this process
	exit(0);
}

uint64_t OsUtils::GetAvailableDiskSpace(const std::string &path) {
	stdext::space_info info = stdext::space(path);
	if (info.available == UINT64_MAX)	//e.g. invalid path
		memset(&info, 0, sizeof(info));
	return uint64_t(info.available);
}

bool OsUtils::HasElevatedPrivilegesWindows() {
	bool underAdmin = false;
#ifdef _WIN32
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hToken;
	if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
		//get the Integrity level
		DWORD dwLengthNeeded;
		if (!GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded)) {
			PTOKEN_MANDATORY_LABEL pTIL = (PTOKEN_MANDATORY_LABEL)malloc(dwLengthNeeded);
			if (GetTokenInformation(hToken, TokenIntegrityLevel, pTIL, dwLengthNeeded, &dwLengthNeeded)) {
				DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid, (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid)-1));
				if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID) {
					underAdmin = true;
				}
			}
			free(pTIL);
		}
		CloseHandle(hToken);
	}
#endif
	return underAdmin;
}

std::string OsUtils::CanModifyFiles(const std::vector<std::string> &filePaths, bool skipMissing) {
	char message[1024] = {0};

	for (int i = 0; i < filePaths.size(); i++) {
		std::string path = filePaths[i];

		if (FILE *f = fopen(path.c_str(), "rb"))
			fclose(f);
		else {
			if (skipMissing)
				continue;
			sprintf(message, "File %s does not exist", path.c_str());
			return message;
		}

#ifdef _WIN32
		//note: in my tests, this does not change "modification datetime"
		if (FILE *f = _fsopen(path.c_str(), "ab", _SH_DENYRW))
#else
		//TODO: is there better way?
		if (FILE *f = fopen(path.c_str(), "ab"))
#endif
			fclose(f);
		else {
			sprintf(message, "File %s cannot be modified", path.c_str());
			return message;
		}
	}

	return message;
}

#ifdef _WIN32
static CComBSTR ComposeShortcutPath(const std::string &name) {
	g_logger->infof("Composing path to shortcut file");
	CComHeapPtr<WCHAR> desktopPath;
	CHECK_HRESULT(SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_DEFAULT, NULL, &desktopPath));
	char appended[256];
	sprintf(appended, "\\%s.lnk", name.c_str());
	CComBSTR shortcutPath = desktopPath;
	shortcutPath += appended;
	return shortcutPath;
}
static void OpenShortcutComInterfaces(CComPtr<IShellLink> &pShellLink, CComPtr<IPersistFile> &pPersistFile) {
	g_logger->infof("Opening COM interfaces");
	EnsureComInitialized();
	CHECK_HRESULT(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink));
	CHECK_HRESULT(pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile));
}
#else
static std::string ComposeShortcutPath(const std::string &name) {
	g_logger->infof("Composing path to shortcut file");
	const char *envHome = getenv("HOME");
	ZipSyncAssertF(envHome, "Environment variable HOME not defined");
	std::string shortcutPath = (stdext::path(envHome) / "Desktop" / (name + ".desktop")).string();
	return shortcutPath;
}
#endif
bool OsUtils::IfShortcutExists(const std::string &name) {
#ifdef _WIN32
	CComBSTR shortcutPath = ComposeShortcutPath(name);

	CComPtr<IShellLink> pShellLink;
	CComPtr<IPersistFile> pPersistFile;
	OpenShortcutComInterfaces(pShellLink, pPersistFile);

	HRESULT res = pPersistFile->Load(shortcutPath, STGM_READ);
	return (res == S_OK);
#else	//_WIN32
	std::string shortcutPath = ComposeShortcutPath(name);
	return (stdext::is_regular_file(shortcutPath));
#endif
}
void OsUtils::CreateShortcut(ShortcutInfo info) {
	g_logger->infof("Shortcut to %s is going to be created", info.executablePath.c_str());
	//rewrite paths as absolute
	std::string cwd = GetCwd();
	if (!stdext::path(info.workingDirPath).is_absolute())
		info.workingDirPath = (stdext::path(cwd) / info.workingDirPath).string();
	if (!stdext::path(info.executablePath).is_absolute())
		info.executablePath = (stdext::path(info.workingDirPath) / info.executablePath).string();
	if (!stdext::path(info.iconPath).is_absolute())
		info.iconPath = (stdext::path(info.workingDirPath) / info.iconPath).string();

#ifdef _WIN32
	//make paths native
	info.workingDirPath = stdext::replace_all_copy(info.workingDirPath, "/", "\\");
	info.executablePath = stdext::replace_all_copy(info.executablePath, "/", "\\");
	info.iconPath = stdext::replace_all_copy(info.iconPath, "/", "\\");

	CComPtr<IShellLink> pShellLink;
	CComPtr<IPersistFile> pPersistFile;
	OpenShortcutComInterfaces(pShellLink, pPersistFile);

	CComBSTR shortcutPath = ComposeShortcutPath(info.name);

	g_logger->infof("Setting shortcut properties");
	//set all properties
	CHECK_HRESULT(pShellLink->SetWorkingDirectory(info.workingDirPath.c_str()));
	CHECK_HRESULT(pShellLink->SetPath(info.executablePath.c_str()));
	if (info.arguments.size())
		CHECK_HRESULT(pShellLink->SetArguments(info.arguments.c_str()));
	if (info.iconPath.size())
		CHECK_HRESULT(pShellLink->SetIconLocation(info.iconPath.c_str(), 0));
	if (info.comment.size())
		CHECK_HRESULT(pShellLink->SetDescription(info.comment.c_str()));

	g_logger->infof("Saving shortcut to %ls", shortcutPath.m_str);
	CHECK_HRESULT(pPersistFile->Save(shortcutPath, FALSE));
#else	//_WIN32

	std::string shortcutPath = ComposeShortcutPath(info.name);
	{
		g_logger->infof("Saving shortcut to %s", shortcutPath.c_str());
		ZipSync::StdioFileHolder f(shortcutPath.c_str(), "wt");
		fprintf(f, "[Desktop Entry]\n");
		fprintf(f, "Version=1.1\n");
		fprintf(f, "Type=Application\n");
		fprintf(f, "Name=%s\n", info.name.c_str());
		fprintf(f, "Comment=%s\n", info.comment.c_str());
		fprintf(f, "TryExec=%s\n", info.executablePath.c_str());
		fprintf(f, "Exec=%s\n", info.executablePath.c_str());
		fprintf(f, "Path=%s\n", info.workingDirPath.c_str());
		fprintf(f, "Icon=%s\n", info.iconPath.c_str());
		fprintf(f, "Categories=Game;ActionGame\n");
		fprintf(f, "Terminal=false\n");
		fprintf(f, "PrefersNonDefaultGPU=true\n");
	}
	MarkAsExecutable(shortcutPath);

#endif
}
