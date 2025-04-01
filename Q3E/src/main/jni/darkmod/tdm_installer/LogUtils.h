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
#include "Logging.h"

//reuse logging system and asserts from ZipSync
//note: initially, all logging goes to console (which is hidden on Windows)
using ZipSync::g_logger;
using ZipSync::formatMessage;
using ZipSync::LogCode;
using ZipSync::Severity;
using ZipSync::lcAssertFailed;
using ZipSync::lcCantOpenFile;
using ZipSync::lcMinizipError;
using ZipSync::assertFailedMessage;
using ZipSync::ErrorException;

//after user decides on installation directory,
//we reset logging with this implementation (writes to a file)
class LoggerTdm : public ZipSync::Logger {
public:
	LoggerTdm();
	void Init();
	virtual void Message(LogCode code, Severity severity, const char *message) override;
};

std::string FormatFilenameWithDatetime(const char *format, const char *label);
