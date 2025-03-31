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

#include <vector>
#include <list>
#include <fstream>
#include "StdFilesystem.h"
#include <functional>
#include <regex>

namespace tdm
{

// Example of a PK4 mapping:
// tdm_player01.pk4: ^models/md5/weapons, ^models/md5/chars/thief, ^dds/models/md5/chars/thief

// The list of regex patterns for each PK4
typedef std::vector<std::string> Patterns;

// A pair associating a PK4 file name with a pattern list
typedef std::pair<std::string, Patterns> Pk4Mapping;

/**
 * The information loaded from the package PK4 mapping (e.g. "darkmod_pk4s.txt")
 * is stored in this class. The order the mappings are defined in is important,
 * so this class derives from a sequential list of file => patternlist pairs.
 */
class Pk4Mappings :
	public std::list<Pk4Mapping>
{
public:
	void LoadFromFile(const fs::path& mappingFile)
	{
		// Start parsing
		std::ifstream file(mappingFile.string().c_str());

		if (!file)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[Pk4Mappings]: Cannot open file " + mappingFile.string());
			return;
		}

		LoadFromStream(file);
	}

	void LoadFromStream(std::istream& stream)
	{
		// Read the whole stream into a string
		std::string buffer(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

		LoadFromString(buffer);
	}

	void LoadFromString(const std::string& str)
	{
		clear();
		auto npos = std::string::npos;

		//split file into non-empty lines
		std::vector<std::string> lines;
		stdext::split(lines, str, "\r\n");

		for (std::string line : lines) {
			//remove comment (if present)
			size_t commentStart = line.find_first_of("#");
			if (commentStart != npos)
				line.resize(commentStart);

			//remove spaces at both ends
			stdext::trim(line);
			if (line.empty())
				continue;
			
			//get position of colon
			auto colon = line.find(':');
			if (colon == npos) {
				TraceLog::WriteLine(LOG_VERBOSE, "Unknown line in mappings file: " + line);
				continue;
			}
			//break into pk4 name and regexes
			AddFile(line.substr(0, colon));
			std::string remains = line.substr(colon + 1);

			//split remaining part into individual regexes (by spaces and commas)
			std::vector<std::string> regexes;
			stdext::split(regexes, remains, " ,");
			for (const std::string &s : regexes)
				AddPattern(s);
		}

		TraceLog::WriteLine(LOG_VERBOSE, "Parsed the mapping file.");
	}

	static bool SearchString(const Patterns& patterns, const std::string &haystack)
	{
		for (const std::string& pat : patterns)
		{
			if (SearchOne(pat, haystack))
				return true;
		}
		return false;
	}

private:
	void AddFile(const std::string &token)
	{
		push_back(Pk4Mapping(token, Patterns()));
	}

	void AddPattern(const std::string &token)
	{
		if (token.empty()) return;

		// Set the destination on the last element
		assert(!empty());

        back().second.push_back(token);
	}

	static bool SearchOne(const std::string& needle, const std::string& haystack)
	{
		//simple regex = only literal characters, possibly with ^ at beginning
		bool isNeedleSimple = true;
		for (int i = 0; i < needle.size(); i++)
		{
			char c = needle[i];

			if (c >= '0' && c <= '9')
				continue;
			if (c >= 'a' && c <= 'z')
				continue;
			if (c >= 'A' && c <= 'Z')
				continue;
			if (c == '_' || c == '-' || c == '/')
				continue;
			if (c == '^'&& i == 0)
				continue;

			isNeedleSimple = false;
			break;
		}

		if (isNeedleSimple)
		{
			if (!needle.empty() && needle[0] == '^')
			{
				const char *prefix = needle.c_str() + 1;
				int prefixLen = needle.size() - 1;
				return (haystack.size() >= prefixLen && memcmp(haystack.c_str(), prefix, prefixLen) == 0);
			}
			else
				return haystack.find(needle) != std::string::npos;
		}
		else
		{
			std::regex regex(needle, std::regex::ECMAScript);
			return std::regex_search(haystack, regex);
		}
	}
};

} // namespace
