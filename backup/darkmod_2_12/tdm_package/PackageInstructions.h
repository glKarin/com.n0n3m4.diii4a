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

#include <list>
#include <fstream>

#include "TraceLog.h"

#include <regex>
#include "StdString.h"
#include "StdFormat.h"
#include "StdFilesystem.h"

namespace fs = stdext;


namespace tdm
{

struct PackageInstruction
{
	enum Type
	{
		Exclude,	// EXCLUDE
		Include,	// INCLUDE
		Mission,	// FM
		Ignore,		// everything else
	};

	Type type;

	// The instruction value (regular expression or map file)
	std::string value;

	// The pre-compiled regex
	std::regex regex;

	// Default constructor
	PackageInstruction() :
		type(Ignore)
	{}

	PackageInstruction(Type type_, const std::string& value_) :
		type(type_),
		value(value_),
		regex(value, std::regex::ECMAScript)
	{}
};

/**
 * The package instruction file is usually "darkmod_maps.txt" in the devel/manifests folder.
 * It contains the INCLUDE and EXCLUDE statements defining which files
 * of the darkmod SVN repository should be packaged and which should be left out.
 *
 * Syntax:
 *
 * # Comment
 * INCLUDE <regexp>
 * EXCLUDE <regexp>
 * FM <mapfile> - ignored for now
 * ordinary lines (e.g. "training_mission.map" or "prefabs/ \.pfb\z") - ignored for now
 */
class PackageInstructions :
	public std::list<PackageInstruction>
{
public:
	// Returns true if the file given by the path (relative to repository root) is excluded
	bool IsExcluded(const std::string& relativePath) const
	{
		for (const_iterator i = begin(); i != end(); ++i)
		{
			if (i->type != PackageInstruction::Exclude) continue;

			// Found an exclusion instruction, check the regexp
			if (std::regex_search(relativePath, i->regex))
			{
				TraceLog::WriteLine(LOG_VERBOSE, 
					stdext::format("[PackageInstructions]: Relative path %s excluded by regex %s", relativePath, i->value));
				return true;
			}
		}

		return false; // not excluded
	}

	void LoadFromFile(const fs::path& file)
	{
		// Start parsing
		std::ifstream stream(file.string().c_str());

		if (!stream)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[PackageInstructions]: Cannot open file " + file.string());
			return;
		}

		LoadFromStream(stream);
	}

	void LoadFromStream(std::istream& stream)
	{
		clear();

		std::string line;

		std::size_t includeStatements = 0;
		std::size_t excludeStatements = 0;
		std::size_t ignoredLines = 0;

		while (std::getline(stream, line, '\n'))
		{
			stdext::trim(line, " \t");

			// Skip empty lines
			if (line.empty()) continue;

			// Skip line comments
			if (line[0] == '#') continue;

			if (stdext::starts_with(line, "INCLUDE"))
			{
				std::string value = line.substr(7);
				stdext::trim(value, " \t");

				push_back(PackageInstruction(PackageInstruction::Include, value));

				includeStatements++;
			}
			else if (stdext::starts_with(line, "EXCLUDE"))
			{
				std::string value = line.substr(7);
				stdext::trim(value, " \t");

				push_back(PackageInstruction(PackageInstruction::Exclude, value));

				excludeStatements++;
			}
			else
			{
				// Just ignore this line
				ignoredLines++;
			}
		}

		TraceLog::WriteLine(LOG_STANDARD, stdext::format("Parsed %d INCLUDEs, %d EXCLUDEs and ignored %d lines.", includeStatements, excludeStatements, ignoredLines));
	}

	void LoadFromString(const std::string& str)
	{
		std::istringstream inputStream(str);

		LoadFromStream(inputStream);
	}
};

} // namespace
