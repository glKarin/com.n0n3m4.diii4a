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

#include <cstdlib>
#include <iostream>
#include <functional>
#include <map>

#include "TraceLog.h"
#include "LogWriters.h"
#include "StdFormat.h"
#include "Packager/Packager.h"
#include "Packager/PackagerOptions.h"


using namespace tdm;
using namespace packager;

int main(int argc, char* argv[])
{
	// Start logging
	RegisterLogWriters();

	TraceLog::WriteLine(LOG_STANDARD, "TDM Packager v2.00 (c) 2016-2022 by greebo & Tels & stgatilov.");
	TraceLog::WriteLine(LOG_STANDARD, "Part of The Dark Mod (http://www.thedarkmod.com).");
	TraceLog::WriteLine(LOG_STANDARD, "");

	// Parse the command line
	PackagerOptions options(argc, argv);

	if (options.Empty() || options.IsSet("help"))
	{
		options.PrintHelp();
		return EXIT_SUCCESS;
	}

	try
	{
		if (options.IsSet("create-manifest"))
		{
			if (options.Get("darkmoddir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Load old manifest if existing
			packager.LoadManifest();

			// Store it for later reference
			packager.SaveManifestAsOldManifest();

			// Read release base manifest (files to be released in any case)
			packager.LoadBaseManifest();

			// TODO Analyse script files
			// TODO Parse declarations
			// TODO Add hardcoded entityDefs

			// Load darkmod_maps.txt (INCLUDE/EXCLUDE/FM)
			packager.LoadInstructionFile();

			// TODO ? Add stuff from maps/prefabs (this is redundant I think)
			// TODO Parse GUIs
			// TODO Parse MD5 Meshes

			// Add versioned files as specified by INCLUDE 
			packager.CollectFilesForManifest();

			// Sort manifest and remove duplicates
			packager.CleanupAndSortManifest();

			// Print the manifest differences
			packager.ShowManifestComparison();

			// Write manifest to disk
			packager.SaveManifest();
		}
		else if (options.IsSet("create-package"))
		{
			if (options.Get("darkmoddir").empty() || options.Get("outputdir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.LoadManifest();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.CheckRepository();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.LoadPk4Mapping();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.SortFilesIntoPk4s();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.CreatePackage();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			TraceLog::WriteLine(LOG_STANDARD, "Done.");
		}
		else if (options.IsSet("check-repository"))
		{
			if (options.Get("darkmoddir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.LoadManifest();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.CheckRepository();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			TraceLog::WriteLine(LOG_STANDARD, "Done.");
		}
		else
		{
			options.PrintHelp();
		}
	}
	catch (std::runtime_error& ex)
	{
		TraceLog::Error(ex.what());
		return EXIT_FAILURE;
	}

	
	return EXIT_SUCCESS;
}
