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

#include "PackagerOptions.h"
#include "../ReleaseManifest.h"
#include "../Pk4Mappings.h"
#include "../PackageInstructions.h"

namespace tdm
{

namespace packager
{

/**
 * A class containing logic and algorithms for creating TDM release packages.
 */
class Packager
{
public:
	// Thrown on any errors during method execution
	class FailureException :
		public std::runtime_error
	{
	public:
		FailureException(const std::string& message) :
			std::runtime_error(message)
		{}
	};

private:
	// Program parameters
	const PackagerOptions& _options;

	// ---- Manifest creation ---

	// The pre-existing manifest before we're starting to create a new one
	ReleaseManifest _oldManifest;

	// The manifest as defined by "base.txt"
	ReleaseManifest _baseManifest;

	// The file containing the include/exclude statements
	PackageInstructions _instructionFile;

	// ---- Manifest creation End ----

	// ---- Package creation ----

	// The manifest of a specific release
	ReleaseManifest _manifest;
	Pk4Mappings _pk4Mappings;

	// Manifest files distributed into Pk4s
	struct ManifestPak {
		std::string name;
		std::vector<ManifestFile> files;
		uint64_t contentsSize = 0;
	};
	typedef std::map<std::string, ManifestPak> Package;
	Package _package;

	// ---- Package creation End ----

public:
	
	// Pass the program options to this class
	Packager(const PackagerOptions& options);

	// Loads the manifest information from the options - needs darkmoddir set to something
	void LoadManifest();

	// Copies the loaded manifest as "old" manifest for later comparison
	void SaveManifestAsOldManifest();

	// Loads the base manifest ("base.txt") from the darkmoddir specified in the options
	void LoadBaseManifest();

	// Load the file containing the INCLUDE/EXCLUDE/FM statements
	void LoadInstructionFile();

	// Traverse the repository to collect all files that should go into the manifest
	void CollectFilesForManifest();

	// Sorts the manifest and removes any duplicates
	void CleanupAndSortManifest();

	// Compares the "old" manifest with the new one and prints a summary
	void ShowManifestComparison();

	// Saves the manifest to devel/manifests/<name>.txt, including the base manifest
	void SaveManifest();

	// Checks if all files in the manifest are existing
	void CheckRepository();

	// Loads the darkmod_pk4s.txt file from the devel folder.
	void LoadPk4Mapping();

	// Sorts files into Pk4s, resulting in a ReleaseFileset
	void SortFilesIntoPk4s();

	// Creates the package at the given output folder
	void CreatePackage();

private:
	// Worker thread for creating a release archive
	void ProcessPackageElement(const ManifestPak& pak);
};

} // namespace

} // namespace
