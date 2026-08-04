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

#include "../ProgramOptions.h"

namespace tdm
{

namespace packager
{

class PackagerOptions :
	public ProgramOptions
{
public:
	// Construct options from command line arguments
	PackagerOptions(int argc, char* argv[])
	{
		SetupDescription();
		ParseFromCommandLine(argc, argv);

		for (int i = 1; i < argc; ++i)
		{
			_cmdLineArgs.push_back(argv[i]);
		}
	}

	void PrintHelp()
	{
		ProgramOptions::PrintHelp();

		// Add some custom examples
		TraceLog::WriteLine(LOG_STANDARD, "");
		TraceLog::WriteLine(LOG_STANDARD, "Command: --create-manifest --darkmoddir=c:/games/tdm/darkmod [--release-name=darkmod]");
		TraceLog::WriteLine(LOG_STANDARD, " The parameter 'darkmoddir' is mandatory, the optional --release-name parameter specifies the name of the package (defaults to 'darkmod').");
		TraceLog::WriteLine(LOG_STANDARD, "");
		TraceLog::WriteLine(LOG_STANDARD, "Examples:");
		TraceLog::WriteLine(LOG_STANDARD, "-------------------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " tdm_package --create-package [--name=darkmod] --darkmoddir=c:/games/tdm/darkmod --outputdir=d:/temp/package");
		TraceLog::WriteLine(LOG_STANDARD, " This will create a full release PK4 set in the specified output directory. The name argument is optional and will default to 'darkmod'. The darkmoddir parameter specifies the darkmod SVN repository. The manifest will be taken from <darkmoddir>/devel/manifests/<releasename>.txt");
		TraceLog::WriteLine(LOG_STANDARD, "");
		TraceLog::WriteLine(LOG_STANDARD, "-------------------------------------------------------------------------------------");
		TraceLog::WriteLine(LOG_STANDARD, " tdm_package --check-repository --darkmoddir=c:/games/tdm/darkmod");
		TraceLog::WriteLine(LOG_STANDARD, " This will check your repository for completeness.");
		TraceLog::WriteLine(LOG_STANDARD, "");
	}

private:
	void SetupDescription()
	{
		_desc.push_back(Option::Flag("create-manifest", "Create a list of files to be released, needs the darkmod SVN folder as argument.\n"));
		_desc.push_back(Option::Flag("create-package", "Create a full release package, needs outputdir and darkmoddir as argument.\n"));
		_desc.push_back(Option::Flag("check-repository", "Checks the repository for completeness.\n"));
		_desc.push_back(Option::Str("outputdir", "The folder the update package PK4 should be saved to.\n"));
		_desc.push_back(Option::Str("darkmoddir", "The folder the darkmod SVN repository is checked out to.\n"));
		_desc.push_back(Option::Str("name", "The name of the release/manifest, as used by the --create-package option. Defaults to 'darkmod'.\n", "darkmod"));
		_desc.push_back(Option::Str("release-name", "The name of the release package to generate the manifest for, e.g. --release-name=saintlucia \n", "darkmod"));
		_desc.push_back(Option::Flag("allow-unversioned-files", "Skips the 'is under SVN version control' check when creating the manifest (use this only if you actually exported your SVN working copy for packaging.)\n"));
		_desc.push_back(Option::Flag("use-singlethread-compression", "Processes one file after the other during packaging, to avoid threading issues on the TDM server.\n"));
		_desc.push_back(Option::Flag("help", "Display this help page"));
	}
};

}

}
