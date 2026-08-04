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

#include "TraceLog.h"

namespace tdm
{

void TraceLog::Register(const ILogWriterPtr& logWriter)
{
	_writers.insert(logWriter);
}

void TraceLog::Unregister(const ILogWriterPtr& logWriter)
{
	_writers.erase(logWriter);
}

void TraceLog::Write(LogClass lc, const std::string& output)
{
	TraceLog& log = Instance();

	for (LogWriters::const_iterator i = log._writers.begin(); i != log._writers.end(); ++i)
	{
		(*i)->WriteLog(lc, output);
	}
}

void TraceLog::WriteLine(LogClass lc, const std::string& output)
{
	TraceLog& log = Instance();

	std::string outputWithNewLine = output + "\n";

	for (LogWriters::const_iterator i = log._writers.begin(); i != log._writers.end(); ++i)
	{
		(*i)->WriteLog(lc, outputWithNewLine);
	}
}

TraceLog& TraceLog::Instance()
{
	static TraceLog _instance;
	return _instance;
}

} // namespace
