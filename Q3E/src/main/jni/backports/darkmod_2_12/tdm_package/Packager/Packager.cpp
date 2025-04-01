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

#include "Packager.h"

#include "../Constants.h"
#include "../ThreadPool.h"
#include "../File.h"
#include "../Zip/TdmZip.h"

namespace tdm
{

namespace packager
{

// Pass the program options to this class
Packager::Packager(const PackagerOptions& options) :
	_options(options)
{}

void Packager::LoadManifest()
{
	fs::path manifestPath = _options.Get("darkmoddir");
	manifestPath /= TDM_MANIFEST_PATH;
	manifestPath /= _options.Get("name") + TDM_MANIFEST_EXTENSION;

	TraceLog::Write(LOG_STANDARD, "Loading manifest at: " + manifestPath.string() + "...");

	_manifest.LoadFromFile(manifestPath);

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The manifest contains %d files.", _manifest.size()));
}

void Packager::SaveManifestAsOldManifest()
{
	_oldManifest = _manifest;
}

void Packager::LoadBaseManifest()
{
	fs::path baseManifestPath = _options.Get("darkmoddir");
	baseManifestPath /= TDM_MANIFEST_PATH;
	baseManifestPath /= std::string("base") + TDM_MANIFEST_EXTENSION;

	TraceLog::Write(LOG_STANDARD, "Loading manifest at: " + baseManifestPath.string() + "...");

	_baseManifest.LoadFromFile(baseManifestPath);

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The base manifest contains %d files.", _baseManifest.size()));
}

void Packager::CheckRepository()
{
	TraceLog::Write(LOG_STANDARD, "Checking if the darkmod repository is complete...");

	std::size_t missingFiles = 0;

	fs::path darkmodPath = _options.Get("darkmoddir");

	for (ReleaseManifest::iterator i = _manifest.begin(); i != _manifest.end(); ++i)
	{
		if (!fs::exists(darkmodPath / i->sourceFile))
		{
			TraceLog::WriteLine(LOG_STANDARD, stdext::format("Could not find file %s in your darkmod path: ", i->sourceFile.string()));
			missingFiles++;
		}
	}

	TraceLog::WriteLine(LOG_STANDARD, "");

	if (missingFiles > 0)
	{
		TraceLog::Error(stdext::format("The manifest contains %d files which are missing in your darkmod path.", missingFiles));
	}
}

void Packager::LoadInstructionFile()
{
	fs::path instrFile = _options.Get("darkmoddir");
	instrFile /= TDM_MANIFEST_PATH;
	instrFile /= stdext::format("%s_maps%s", _options.Get("release-name"), TDM_MANIFEST_EXTENSION); // e.g. darkmod_maps.txt

	TraceLog::WriteLine(LOG_STANDARD, "Loading package instruction file: " + instrFile.string() + "...");

	_instructionFile.LoadFromFile(instrFile);

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The package instruction file has %d entries.", _instructionFile.size()));
}

void Packager::CollectFilesForManifest()
{
	// Clear the manifest by assigning the base manifest first
	_manifest = _baseManifest;

	// Load files from the repository
	_manifest.CollectFilesFromRepository(_options.Get("darkmoddir"), _instructionFile);

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The manifest has now %d entries.", _manifest.size()));
}

void Packager::CleanupAndSortManifest()
{
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Sorting manifest..."));

	// Sort manifest
	_manifest.sort();

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Sorting done."));

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Removing duplicates..."));

	// Remove dupes using the standard algorithm (requires the list to be sorted)
	_manifest.unique();

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Removal done."));

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The manifest has now %d entries.", _manifest.size()));
}

void Packager::ShowManifestComparison()
{
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Sorting old manifest and removing duplicates..."));

	// Sort the old manifest before doing a set difference
	_oldManifest.sort();

	// Remove dupes using the standard algorithm (requires the list to be sorted)
	_oldManifest.unique();

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("done."));

	std::list<ManifestFile> removedFiles;
	std::list<ManifestFile> addedFiles;

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Calculating changes..."));

	std::set_difference(_oldManifest.begin(), _oldManifest.end(), _manifest.begin(), _manifest.end(), 
						std::back_inserter(removedFiles));

	std::set_difference(_manifest.begin(), _manifest.end(), _oldManifest.begin(), _oldManifest.end(), 
						std::back_inserter(addedFiles));

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("done."));

	if (!removedFiles.empty())
	{
		TraceLog::WriteLine(LOG_STANDARD, stdext::format("The following %d files were removed:", removedFiles.size()));

		for (std::list<ManifestFile>::const_iterator i = removedFiles.begin(); i != removedFiles.end(); ++i)
		{
			TraceLog::WriteLine(LOG_STANDARD, stdext::format("  Removed: %s", i->sourceFile.string()));
		}
	}
	else
	{
		TraceLog::WriteLine(LOG_STANDARD, stdext::format("No files were removed."));
	}

	if (!addedFiles.empty())
	{
		TraceLog::WriteLine(LOG_STANDARD, stdext::format("The following %d files were added:", addedFiles.size()));

		for (std::list<ManifestFile>::const_iterator i = addedFiles.begin(); i != addedFiles.end(); ++i)
		{
			TraceLog::WriteLine(LOG_STANDARD, stdext::format("  Added: %s", i->sourceFile.string()));
		}
	}
	else
	{
		TraceLog::WriteLine(LOG_STANDARD, stdext::format("No files were added."));
	}
}

void Packager::SaveManifest()
{
	fs::path manifestPath = _options.Get("darkmoddir");
	manifestPath /= TDM_MANIFEST_PATH;
	manifestPath /= stdext::format("%s%s", _options.Get("release-name"), TDM_MANIFEST_EXTENSION); // e.g. darkmod.txt

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Writing manifest to %s...", manifestPath.string()));

	_manifest.WriteToFile(manifestPath);

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Done writing manifest file."));
}

void Packager::LoadPk4Mapping()
{
	fs::path mappingFile = _options.Get("darkmoddir");
	mappingFile /= TDM_MANIFEST_PATH;
	mappingFile /= stdext::format("%s_pk4s%s", _options.Get("name"), TDM_MANIFEST_EXTENSION); // e.g. darkmod_pk4s.txt

	TraceLog::Write(LOG_STANDARD, "Loading PK4 mapping file: " + mappingFile.string() + "...");

	_pk4Mappings.LoadFromFile(mappingFile);

	TraceLog::WriteLine(LOG_STANDARD, "");
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("The mapping file defines %d PK4s.", _pk4Mappings.size()));
}

void Packager::SortFilesIntoPk4s()
{
	TraceLog::WriteLine(LOG_STANDARD, "Sorting files into PK4s...");

	_package.clear();

	fs::path darkmodPath = _options.Get("darkmoddir");

	// Go through each file in the manifest and check which pattern applies
	for (ReleaseManifest::iterator i = _manifest.begin(); i != _manifest.end(); /* in-loop increment */)
	{
		const std::string& destFilename = i->destFile.string();

		bool matched = false;

		// The patterns are applied in the order they appear in the darkmod_pk4s.txt file
		for (Pk4Mappings::const_iterator m = _pk4Mappings.begin(); m != _pk4Mappings.end(); ++m)
		{
			const std::string& pk4name = m->first;
			const Patterns& patterns = m->second;

			// Does this filename match any of the patterns of this PK4 file?
			if (Pk4Mappings::SearchString(patterns, destFilename))
			{
				// Match
				ManifestPak& pak = _package[pk4name];
				pak.name = pk4name;

				//TraceLog::WriteLine(LOG_STANDARD, "Putting file " + i->destFile.string() + " => " + pk4name);

				// Copy that file into the release package
				pak.files.push_back(*i);

				if (fs::is_regular_file(darkmodPath / i->sourceFile))
					pak.contentsSize += fs::file_size(darkmodPath / i->sourceFile);

				// Remove the file from our manifest
				_manifest.erase(i++);

				matched = true;
				break;
			}
		}

		if (!matched) 
		{
			// If the file is a PK4 file itself, just add it to the list
			if (File::IsArchive(i->destFile))
			{
				const std::string& pk4name = i->sourceFile.string();

				_package[pk4name].name = pk4name;
				_package[pk4name].contentsSize += fs::file_size(darkmodPath / pk4name);

				_manifest.erase(i++);
			}
			else if (fs::is_directory(darkmodPath / i->destFile)) // Check if this is a folder
			{
				// Unmatched folders can be removed
				_manifest.erase(i++);
			}
			else
			{
				TraceLog::WriteLine(LOG_STANDARD, "Could not match file: " + i->destFile.string());
				++i;
			}
		}
	}

	TraceLog::WriteLine(LOG_STANDARD, "done");

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("%d entries in the manifest could not be matched, check the logs.", _manifest.size()));
}

#include "../../framework/CompressionParameters.h"

void Packager::ProcessPackageElement(const ManifestPak& pak)
{
	fs::path outputDir = _options.Get("outputdir");

	if (!fs::exists(outputDir))
	{
		fs::create_directories(outputDir);
	}

	fs::path darkmodPath = _options.Get("darkmoddir");

	// Target file
	fs::path pk4Path = outputDir / pak.name;

	// Make sure all folders exist
	fs::path pk4Folder = pk4Path;
	pk4Folder.remove_filename();

	if (!fs::exists(pk4Folder))
	{
		fs::create_directories(pk4Folder);
	}

	// Remove destination file before writing
	File::Remove(pk4Path);

	// Copy-only switch for PK4 files mentioned in the manifest (those have 0 members to compress, like tdm_game01.pk4)
	if (File::IsArchive(pak.name) && pak.files.empty())
	{
		TraceLog::WriteLine(LOG_STANDARD, stdext::format("Copying file: %s", pk4Path.string()));

		if (!File::Copy(darkmodPath / pak.name, pk4Path))
		{
			TraceLog::Error(stdext::format("Could not copy file: %s", pk4Path.string()));
		}

		return;
	}

	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Compressing package: %s", pk4Path.string()));
	
	ZipFileWritePtr pk4 = Zip::OpenFileWrite(pk4Path, Zip::CREATE);

	if (pk4 == NULL)
	{
		throw FailureException("Failed to process element: " + pak.name);
	}

	for (auto m = pak.files.begin(); m != pak.files.end(); ++m)
	{
		fs::path sourceFile = darkmodPath / m->sourceFile;
		const fs::path& targetFile = m->destFile;

		// Make sure folders get added as such
		if (fs::is_directory(sourceFile))
		{
			continue;
		}

		//stgatilov: some files must be stored uncompressed, e.g. ROQ and OGG
		ZipFileWrite::CompressionMethod method = ZipFileWrite::DEFLATE_MAX;
		std::string ext = fs::extension(sourceFile);
		for (int i = 0; i < PK4_UNCOMPRESSED_EXTENSIONS_COUNT; i++)
			if (stdext::to_lower_copy(ext) == stdext::to_lower_copy(std::string(".") + PK4_UNCOMPRESSED_EXTENSIONS[i]))
				method = ZipFileWrite::STORE;

		TraceLog::WriteLine(LOG_VERBOSE,
			stdext::format("%s file %s.", (method == ZipFileWrite::STORE ? "Storing" : "Deflating"), sourceFile.string())
		);

		pk4->DeflateFile(sourceFile, targetFile.string(), method);
	}
}

void Packager::CreatePackage()
{
	// Get list of pk4 files to process
	std::vector<ManifestPak> paks;
	for (auto& p : _package)
		paks.push_back(p.second);

	// Create worker threads to compress stuff into the target PK4s
	unsigned int numHardwareThreads = std::thread::hardware_concurrency();
	if (numHardwareThreads == 0 || _options.IsSet("use-singlethread-compression")) 
		numHardwareThreads = 1;
	TraceLog::WriteLine(LOG_STANDARD, stdext::format("Using %d threads to compress files.", numHardwareThreads));

	if (numHardwareThreads > 1) {
		// Sort paks by size decreasing
		// This should reduces wall time
		std::sort(paks.begin(), paks.end(), [](const ManifestPak& a, const ManifestPak& b) {
			return std::tie(a.contentsSize, a.name) > std::tie(b.contentsSize, b.name);
		});

		ThreadPool pool(numHardwareThreads);

		std::vector<std::future<void>> futures;
		for (const ManifestPak& pak : paks) {
			futures.push_back(pool.enqueue([this, pak]() {
				ProcessPackageElement(pak);
			}));
		}

		// rethrow exceptions (if any)
		for (auto& f : futures)
			f.get();
	}
	else {
		for (const ManifestPak& pak : paks)
			ProcessPackageElement(pak);
	}

	TraceLog::WriteLine(LOG_STANDARD, "Done.");
}

} // namespace 

} // namespace
