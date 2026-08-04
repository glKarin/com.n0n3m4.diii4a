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
#include "LogUtils.h"
#include "OsUtils.h"
#include <time.h>
#include "Constants.h"
#include "zipsync/CommandLine.h"
#include "zipsync/StdFilesystem.h"

#ifdef _MSC_VER
#define _WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

std::string FormatFilenameWithDatetime(const char *format, const char *label) {
	static time_t timeval = time(0);
	//note: use UTC time to avoid any timezone troubles
	auto *tm = gmtime(&timeval);
	char filename[1024] = {0};
	int len = strftime(filename, sizeof(filename), format, tm);
	ZipSyncAssertF(len > 0 && len < sizeof(filename)-1, "Failed to format %s filename with datetime (%d)", label, len);
	return filename;
}

static int writeToDebugOutput = -1;
static FILE *logFile = nullptr;

LoggerTdm::LoggerTdm() {
}

//note: we need Init method, because we can't use virtual methods in constructor
void LoggerTdm::Init() {
	if (writeToDebugOutput < 0) {
		writeToDebugOutput = 0;
#ifdef _MSC_VER
		writeToDebugOutput = IsDebuggerPresent();
#endif
	}

	if (!logFile) {
		time_t timestamp = time(0);
		char filename[1024];
		sprintf(filename, TDM_INSTALLER_LOG_FORMAT, (long long)timestamp);
		logFile = fopen(filename, "wt");
		if (!logFile)
			throw ErrorException("cannot open log file", lcCantOpenFile);
		fprintf(logFile,
			"This is log file of tdm_installer application.\n"
			"Created at %s\n",
			ctime(&timestamp)
		);
		fflush(logFile);

		std::string root = OsUtils::GetCwd();
		std::vector<std::string> allPaths = ZipSync::EnumerateFilesInDirectory(root);
		for (const auto &entry : allPaths) {
			long long oldTimestamp = -1;
			if (sscanf(entry.c_str(), TDM_INSTALLER_LOG_FORMAT, &oldTimestamp) != 1 || oldTimestamp < 0)
				continue;
			long long difference = (long long)timestamp - oldTimestamp;
			if (difference <= 30 * 24*60*60)	//one month
				continue;
			infof("Removing old logfile: %s", entry.c_str());
			stdext::remove(entry);
		}
	}
}

void LoggerTdm::Message(LogCode code, Severity severity, const char *message) {
	std::string printedText = formatMessage("%s%s\n",
        severity == ZipSync::sevFatal ? "FATAL: " :
        severity == ZipSync::sevError ? "ERROR: " :
        severity == ZipSync::sevWarning ? "Warning: " : "",
        message
	);

	printf("%s", printedText.c_str());
	fflush(stdout);
#ifdef _MSC_VER
	if (writeToDebugOutput)
		OutputDebugStringA(printedText.c_str());
#endif
	if (logFile) {
		fprintf(logFile, "%s", printedText.c_str());
		fflush(logFile);
	}
}
