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

#include "PackageInstructions.h"
#include "SvnClient.h"

namespace tdm
{

struct ManifestFile
{
	// The source file path relative to darkmod/
	fs::path sourceFile;

	// The destination path in the release, is almost always the same as sourceFile
	// except for some special cases (config.spec, DoomConfig.cfg, etc.)
	fs::path destFile;

	ManifestFile(const fs::path& sourceFile_) :
		sourceFile(sourceFile_),
		destFile(sourceFile_)
	{}

	ManifestFile(const fs::path& sourceFile_, const fs::path& destFile_) :
		sourceFile(sourceFile_),
		destFile(destFile_)
	{}

	// Comparison operator for sorting
	bool operator<(const ManifestFile& other) const
	{
		return this->sourceFile < other.sourceFile;
	}

	// Comparison operator for removing duplicates
	bool operator==(const ManifestFile& other) const
	{
		return this->sourceFile == other.sourceFile && this->destFile == other.destFile;
	}
};

/**
 * The manifest contains all the files which should go into a TDM release.
 * The list is unsorted, it doesn't matter if files are duplicate, as they
 * are re-sorted into PK4 mappings anyways, which resolves any double entries.
 *
 * File format:
 * Each line contains a single file, its path relative to darkmod/.
 * Comments lines start with the # character.
 * It's possible to move files to a different location by using the => syntax: 
 * e.g. ./devel/release/config.spec => ./config.spec
 */
class ReleaseManifest :
	public std::list<ManifestFile>
{
public:
	void LoadFromFile(const fs::path& manifestFile)
	{
		// Start parsing
		std::ifstream file(manifestFile.string().c_str());

		if (!file)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[ReleaseManifest]: Cannot open file " + manifestFile.string());
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

			//try to split into filename and replacement
			auto arrow = line.find("=>");
			if (arrow == npos) {
				AddSourceFile(line);
			}
			else {
				std::string leftS = line.substr(0, arrow);
				std::string rightS = line.substr(arrow + 2);
				stdext::trim(leftS);
				stdext::trim(rightS);
				AddSourceFile(leftS);
				AddDestFile(rightS);
			}
		}

		TraceLog::WriteLine(LOG_VERBOSE, "Parsed the manifest.");
	}

	/**
	 * greebo: Collects all files from the SVN repository if they are meant to be included by
	 * the instructions given (which were loaded from darkmod_maps.txt).
	 * If @skipVersioningCheck is true no SVN client checks are performed. Still, .svn folders won't be included.
	 */
	void CollectFilesFromRepository(const fs::path& repositoryRoot, const PackageInstructions& instructions, 
									bool skipVersioningCheck = false)
	{
		// Acquire an SVN client implementation
		SvnClient svnClient;

		if (!skipVersioningCheck)
		{
			svnClient.SetActive(repositoryRoot);
		}

		// Process the inclusion commands and exclude all files as instructed
		for (PackageInstructions::const_iterator instr = instructions.begin(); instr != instructions.end(); ++instr)
		{
			if (instr->type == PackageInstruction::Include)
			{
				ProcessInclusion(repositoryRoot, instructions, *instr, svnClient);
			}
		}
	}

	void WriteToFile(const fs::path& manifestPath)
	{
		std::ofstream manifest(manifestPath.string().c_str());

		time_t rawtime;
		time(&rawtime);

		manifest << "#" << std::endl;
		manifest << "# This file was generated automatically:" << std::endl;
		manifest << "#   by tdm_package.exe" << std::endl;
		manifest << "#   at " << asctime(localtime(&rawtime)) << std::endl;
		manifest << "# Any modifications to this file will be lost, instead modify:" << std::endl;
		manifest << "#   devel/manifests/base.txt" << std::endl;
		manifest << "#" << std::endl;
		manifest << std::endl;

		for (const_iterator i = begin(); i != end(); ++i)
		{
			manifest << "./" << i->sourceFile.string();

			// Do we have a redirection?
			if (i->destFile != i->sourceFile)
			{
				manifest << " => ./" << i->destFile.string();
			}

			manifest << std::endl;
		}
	}

private:
	void ProcessInclusion(const fs::path& repositoryRoot, const PackageInstructions& instructions, 
						  const PackageInstruction& inclusion, SvnClient& svn)
	{
		TraceLog::WriteLine(LOG_STANDARD, "Processing inclusion: " + inclusion.value);

		// Construct the starting path
		fs::path inclusionPath = repositoryRoot;
		inclusionPath /= stdext::trim_copy(inclusion.value);

		// Add the inclusion path itself
		std::string relativeInclusionPath = inclusionPath.string().substr(repositoryRoot.string().length() + 1);

		// Cut off the trailing slash
		if (stdext::ends_with(relativeInclusionPath, "/"))
		{
			relativeInclusionPath = relativeInclusionPath.substr(0, relativeInclusionPath.length() - 1);
		}

		TraceLog::WriteLine(LOG_VERBOSE, "Adding inclusion path: " + relativeInclusionPath);
		push_back(ManifestFile(relativeInclusionPath));

		// Check for a file
		if (fs::is_regular_file(inclusionPath))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Investigating file: " + inclusionPath.string());

			// Add this item, if versioned
			if (!svn.FileIsUnderVersionControl(inclusionPath))
			{
				TraceLog::WriteLine(LOG_STANDARD, "Skipping unversioned file mentioned in INCLUDE statements: " + inclusionPath.string());
				return;
			}

			// Cut off the repository path to receive a relative path (cut off the trailing slash too)
			std::string relativePath = inclusionPath.string().substr(repositoryRoot.string().length() + 1);

			if (!instructions.IsExcluded(relativePath))
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Adding file: " + relativePath);
				push_back(ManifestFile(relativePath));
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "File is excluded: " + relativePath);
				return;
			}
		}
		
		//TraceLog::WriteLine(LOG_PROGRESS, "Inclusion path: " + inclusionPath.string() + "---");

		if (!fs::is_directory(inclusionPath))
		{
			TraceLog::WriteLine(LOG_PROGRESS, "Skipping non-file and non-folder: " + inclusionPath.string());
			return;
		}

		ProcessDirectory(repositoryRoot, inclusionPath, instructions, svn);
	}

	// Adds stuff in the given directory (absolute path), adds stuff only if not excluded, returns the number of added items
	std::size_t ProcessDirectory(const fs::path& repositoryRoot, const fs::path& dir, 
								 const PackageInstructions& instructions, SvnClient& svn)
	{
		assert(fs::exists(dir));

		std::size_t itemsAdded = 0;

		// Traverse this folder
		for (fs::path file : fs::directory_enumerate(dir))
		{
			if (stdext::ends_with(file.string(), ".svn"))
			{
				// Prevent adding .svn folders
				continue;
			}

			// Ensure this item is under version control
			if (!svn.FileIsUnderVersionControl(file))
			{
				TraceLog::WriteLine(LOG_PROGRESS, "Skipping unversioned item: " + file.string());
				continue;
			}

			TraceLog::WriteLine(LOG_VERBOSE, "Investigating item: " + file.string());

			// Cut off the repository path to receive a relative path (cut off the trailing slash too)
			std::string relativePath = file.string().substr(repositoryRoot.string().length() + 1);

			// Consider the exclusion list
			if (instructions.IsExcluded(relativePath))
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Item is excluded: " + relativePath);
				continue;
			}
			
			if (fs::is_directory(file))
			{
				// Versioned folder, enter recursion
				std::size_t folderItems = ProcessDirectory(repositoryRoot, file, instructions, svn);

				if (folderItems > 0)
				{
					// Add an entry for this folder itself, it's not empty
					push_back(ManifestFile(relativePath));
				}

				itemsAdded += folderItems;
			}
			else
			{
				// Versioned file, add it
				TraceLog::WriteLine(LOG_VERBOSE, "Adding file: " + relativePath);
				push_back(ManifestFile(relativePath));

				itemsAdded++;
			}
		}

		return itemsAdded;
	}

	void AddSourceFile(std::string token)
	{
		if (token.size() < 2) return; // skip strings smaller than 2 chars

		if (token[0] == '.' && token[1] == '/') 
		{
			token.erase(0, 2); // skip leading ./
		}

		push_back(ManifestFile(stdext::trim_copy(token)));
	}

	void AddDestFile(std::string token)
	{
		if (token.size() < 2) return; // skip strings smaller than 2 chars

		if (token[0] == '.' && token[1] == '/') 
		{
			token.erase(0, 2); // skip leading ./
		}

		// Set the destination on the last element
		assert(!empty());

		back().destFile = stdext::trim_copy(token);
	}

};

} // namespace
