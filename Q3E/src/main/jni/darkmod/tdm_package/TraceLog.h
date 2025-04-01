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
#include <set>
#include <memory>

namespace tdm
{

enum LogClass
{
	LOG_VERBOSE,		// print every little thing
	LOG_STANDARD,		// mirror info, file checks, etc.
	LOG_PROGRESS,		// download/file operation progress
	LOG_ERROR,			// program errors
};

/**
 * An abstract class that digests log messages.
 *
 * Client code can implement this class and register it in 
 * the library's TraceLog instance.
 */
class ILogWriter
{
public:
	virtual ~ILogWriter() {}

	/**
	 * A Log writer must implement this method such that it can receive
	 * text messages sent by the library.
	 */
	virtual void WriteLog(LogClass lc, const std::string& str) = 0;
};
typedef std::shared_ptr<ILogWriter> ILogWriterPtr;

/**
 * The tracelog singleton class. Register any LogWriters here, to have
 * the library's log output being sent to them.
 */
class TraceLog
{
private:
	typedef std::set<ILogWriterPtr> LogWriters;
	LogWriters _writers;

public:
	// Add a new logwriter to this instance. All future logging output will be sent
	// to this log writer too.
	void Register(const ILogWriterPtr& logWriter);

	// Remove a logwriter, no more logging will be sent to it.
	void Unregister(const ILogWriterPtr& logWriter);

	// Write a string to the trace log, this is broadcast to all registered writers.
	static void Write(LogClass lc, const std::string& output);

	// Write a line to the trace log, this is broadcast to all registered writers.
	// A line break is appended automatically at the end of the given string.
	static void WriteLine(LogClass lc, const std::string& output);

	// Convenience method, wraps to WriteLine(LOG_ERROR, ...)
	static void Error(const std::string& output)
	{
		WriteLine(LOG_ERROR, output);
	}

	// Accessor to the singleton instance
	static TraceLog& Instance();
};

} // namespace
