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
		Chdir,		// CHDIR
		Ignore,		// everything else
	};

	Type type;

	// The instruction value (regular expression or map file)
	std::string value;

	// The pre-compiled regex
	std::regex regex;

	// Only filled for CHDIR
	std::string second;

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
 * CHDIR <srcpath>/ => <dstpath>/
 */
class PackageInstructions :
	public std::list<PackageInstruction>
{
public:
	// Returns true if the file given by the path (relative to repository root) is included
	bool IsIncluded(const std::string& relativePath) const
	{
		bool state = false;
		for (const_iterator i = begin(); i != end(); ++i)
		{
			if (i->type == PackageInstruction::Include)
			{
				if (std::regex_search(relativePath, i->regex))
				{
					TraceLog::WriteLine(LOG_VERBOSE, 
						stdext::format("[PackageInstructions]: Relative path %s included by regex %s", relativePath, i->value));
					state = true;
				}
			}
			if (i->type == PackageInstruction::Exclude)
			{
				if (std::regex_search(relativePath, i->regex))
				{
					TraceLog::WriteLine(LOG_VERBOSE, 
						stdext::format("[PackageInstructions]: Relative path %s excluded by regex %s", relativePath, i->value));
					state = false;
				}
			}
		}

		return state;
	}

	// Returns true if the file path has to be changed during packaging (due to CHDIR)
	bool IsRenamed(const std::string& relativePath, std::string& renamedPath) const
	{
		for (const_iterator i = begin(); i != end(); ++i)
		{
			if (i->type != PackageInstruction::Chdir) continue;

			// Found an chdir instruction, check the path prefix
			if (stdext::starts_with(relativePath, i->value))
			{
				renamedPath = i->second + relativePath.substr(i->value.size());

				// collapse double-slashes
				renamedPath = stdext::replace_all_copy(renamedPath, "//", "/");
				// don't start path with slash
				if (stdext::starts_with(renamedPath, "/"))
					renamedPath = renamedPath.substr(1);

				TraceLog::WriteLine(LOG_VERBOSE, 
					stdext::format("[PackageInstructions]: Relative path %s renamed to %s", relativePath, renamedPath));
				return true;
			}
		}

		renamedPath = relativePath;
		return false; // not renamed
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
		std::size_t chdirStatements = 0;
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
			else if (stdext::starts_with(line, "CHDIR"))
			{
				std::string value = line.substr(6);
				stdext::trim(value, " \t");

				size_t pos = value.find("=>");
				if (pos == std::string::npos)
				{
					TraceLog::WriteLine(LOG_ERROR, "[PackageInstructions]: CHDIR bad format " + value);
					continue;
				}
				std::string before = value.substr(0, pos);
				std::string after = value.substr(pos + 2);
				stdext::trim(before, " \t");
				stdext::trim(after, " \t");

				PackageInstruction instruction(PackageInstruction::Chdir, before);
				instruction.second = after;

				push_back(instruction);

				chdirStatements++;
			}
			else
			{
				// Just ignore this line
				ignoredLines++;
			}
		}

		TraceLog::WriteLine(LOG_STANDARD, stdext::format("Parsed %d INCLUDEs, %d EXCLUDEs, %d CHDIRs, and ignored %d lines.", includeStatements, excludeStatements, chdirStatements, ignoredLines));
	}

	void LoadFromString(const std::string& str)
	{
		std::istringstream inputStream(str);

		LoadFromStream(inputStream);
	}
};

} // namespace
