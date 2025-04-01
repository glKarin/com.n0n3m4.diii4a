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

#include <stdio.h>
#include <string>
#include <memory>

namespace tdm
{

class FileLogWriter :
	public tdm::ILogWriter
{
private:
	FILE* _logFile;
public:
	FileLogWriter(const std::string& path) :
		_logFile(fopen(path.c_str(), "w"))
	{
		time_t rawtime;
		time(&rawtime);

		fputs("TDM Package Logfile created: ", _logFile);
		fputs(asctime(localtime(&rawtime)), _logFile);
		fputs("\n", _logFile);
	}

	~FileLogWriter()
	{
		fclose(_logFile);
		fflush(_logFile);
	}

	void WriteLog(LogClass lc, const std::string& str)
	{
		fputs(str.c_str(), _logFile);
		fflush(_logFile);
	}
};

class ConsoleLogWriter :
	public tdm::ILogWriter
{
public:
	void WriteLog(LogClass lc, const std::string& str)
	{
		if (lc >= LOG_STANDARD)
		{
			std::cout << str;
		}
	}
};

void RegisterLogWriters()
{
	std::shared_ptr<tdm::FileLogWriter> logWriter(new tdm::FileLogWriter("tdm_package.log"));
	std::shared_ptr<tdm::ConsoleLogWriter> consoleWriter(new tdm::ConsoleLogWriter);

	tdm::TraceLog::Instance().Register(logWriter);
	tdm::TraceLog::Instance().Register(consoleWriter);
}

} // namespace
