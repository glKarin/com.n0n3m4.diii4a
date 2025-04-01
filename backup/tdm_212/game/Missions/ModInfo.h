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

#ifndef _MOD_INFO_H_
#define _MOD_INFO_H_

#include "precompiled.h"

class CModInfoDecl;
typedef std::shared_ptr<CModInfoDecl> CModInfoDeclPtr;

class CModInfo
{
private:
	// The "internal" mod info declaration, 
	// holding the persistent information about a mod (completion status, etc.)
	CModInfoDeclPtr _decl;

	// TRUE if the underlying declaration has been altered and needs saving
	bool _declDirty;

	// The cached size of the fs_currentfm folder, in KB
	std::size_t _modFolderSize;
	bool		_modFolderSizeComputed;

	/**
	* Turn "Title:   Some  " into "Some". Example: Strip("Title:", displayName);
	*/
	void		Strip(const char* fieldname, idStr &field);

public:
	// Public Properties - these aren't stored in the mod info declaration
	// but are constructed from the text files found in the fms/mod/ folders.

	idStr modName;			// The mod name (fs_currentfm)
	idStr displayName;		// The display name of the mission
	idStr pathToFMPackage;	// path to PK4 in fms/ folder
	idStr description;		// description text
	idStr author;			// author(s)
	idStr image;			// splash image

	idList<idStr> _missionTitles; // grayman #3733 - (Campaign) mission titles

	// Required TDM version
	idStr requiredVersionStr;
	int requiredMajor;
	int requiredMinor;

	// gnartsch: flag for local presence of a localization pack 
	bool isL10NpackInstalled;

	CModInfo(const idStr& modName_, const CModInfoDeclPtr& detailsDecl) :
		_decl(detailsDecl),
		_declDirty(false),
		_modFolderSize(0),
		_modFolderSizeComputed(false),
		modName(modName_),
		requiredMajor(TDM_VERSION_MAJOR),
		requiredMinor(TDM_VERSION_MINOR)
	{}

	// Returns the size requirements of the fs_currentfm folder
	// Returns 0 if the mod has not been installed yet
	std::size_t GetModFolderSize();

	// Returns the full OS path to the mod folder
	idStr GetModFolderPath();

	void ClearModFolderSize();

	// Fast check whether the readme.txt file exists
	bool HasModNotes();

	// Retrieves the readme.txt contents (is never cached, always read live from disk)
	idStr GetModNotes();

	// Returns true if this mod has been completed
	// Pass the difficulty level to check for a specific difficulty, or -1 (default) to check
	// whether the mod has been completed on any difficulty level.
	bool ModCompleted(int difficultyLevel = -1);

	// Get the assembled mod completion string with difficulty information
	idStr GetModCompletedString();

	// Returns a human-readable format string (i.e. 1.33 GB)
	idStr	GetModFolderSizeString();

	// Returns a specific key value from the mod info declaration's dictionary
	idStr	GetKeyValue(const char* key, const char* defaultStr ="");

	// Saves a key into the internal declaration dictionary
	void	SetKeyValue(const char* key, const char* value);

	// Removes a certain keyvalue
	void	RemoveKeyValue(const char* key);

	// Removes key values matching the given prefix
	void	RemoveKeyValuesMatchingPrefix(const char* prefix);

	// Will save any persistent info to the given file
	void	SaveToFile(idFile* file);

	// Load stuff from darkmod.txt, returns FALSE if the file couldn't be read
	bool	LoadMetaData();

	// grayman #3733
	void	GetMissionTitles(idStr missionTitles);
};
typedef std::shared_ptr<CModInfo> CModInfoPtr;

#endif /* _MOD_INFO_H_ */
