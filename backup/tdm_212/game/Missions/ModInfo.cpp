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

#include "precompiled.h"
#pragma hdrstop



#include "ModInfo.h"
#include "ModInfoDecl.h"

#include <StdFilesystem.h>
namespace fs = stdext;

std::size_t CModInfo::GetModFolderSize()
{
	if (_modFolderSizeComputed)
	{
		return _modFolderSize;
	}

	_modFolderSizeComputed = true;
	_modFolderSize = 0;

	fs::path modPath = g_Global.GetModPath(modName.c_str());

	if (fs::exists(modPath))
	{
		// Iterate over all files in the mod folder
		auto recPaths = fs::recursive_directory_enumerate(modPath);
		for (const auto &path : recPaths)
		{
			if (fs::is_directory(path)) continue;
			_modFolderSize += fs::file_size(path);
		}
	}

	return _modFolderSize;
}

void CModInfo::ClearModFolderSize()
{
	_modFolderSize = 0;
	_modFolderSizeComputed = false;
}

idStr CModInfo::GetModFolderSizeString()
{
	float size = static_cast<float>(GetModFolderSize());

	idStr str;
	if (size < 1024)
	{
		str = va("%0.2f %s", size, common->Translate( "#str_02050" ));	// Bytes
	}
	else if (size < 1024*1024)
	{
		str = va("%0.0f %s", size/1024.0f, common->Translate( "#str_02054" ));	// kB
	}
	else if (size < 1024.0f*1024.0f*1024.0f)
	{
		str = va("%0.0f %s", size/(1024.0f*1024.0f), common->Translate( "#str_02055" ));	// MB
	}
	else if (size < 1024.0f*1024.0f*1024.0f*1024.0f)
	{
		str = va("%0.2f %s", size/(1024.0f*1024.0f*1024.0f), common->Translate( "#str_02056" ));	// GB
	}

	return str;
}

bool CModInfo::ModCompleted(int difficultyLevel)
{
	bool anyCompleted = false;

	for (int i = 0; i < DIFFICULTY_COUNT; ++i)
	{
		bool diffCompleted = GetKeyValue(va("mission_completed_%d", i)) == "1";

		if (difficultyLevel == i)
		{
			return diffCompleted;
		}

		// Accumulate the information
		anyCompleted |= diffCompleted;
	}

	return anyCompleted;
}

idStr CModInfo::GetModCompletedString()
{
	if (modName == "training_mission")
	{
		return common->Translate( "#str_02511" );	// Not completable
	}

	idStr diffStr;

	bool anyCompleted = false;

	for (int i = 0; i < DIFFICULTY_COUNT; ++i)
	{
		bool diffCompleted = GetKeyValue(va("mission_completed_%d", i)) == "1";

		if (diffCompleted)
		{
			diffStr += diffStr.Length() > 0 ? ", " : "";
			diffStr += gameLocal.m_DifficultyManager.GetDifficultyName(i);

			anyCompleted = true;
		}
	}

	if (anyCompleted)
	{
		return va( common->Translate( "#str_02513" ), diffStr.c_str());	// "Yes (%s)"
	}
	else
	{
		return common->Translate( "#str_02512" );	// Not yet
	}
}

idStr CModInfo::GetKeyValue(const char* key, const char* defaultStr)
{
	if (_decl == NULL) return defaultStr;

	return _decl->data.GetString(key, defaultStr);
}

void CModInfo::SetKeyValue(const char* key, const char* value)
{
	if (_decl == NULL) return;

	_declDirty = true;

	_decl->data.Set(key, value);
}

void CModInfo::RemoveKeyValue(const char* key)
{
	if (_decl == NULL) return;

	_declDirty = true;

	_decl->data.Delete(key);
}

void CModInfo::RemoveKeyValuesMatchingPrefix(const char* prefix)
{
	if (_decl == NULL) return;

	for (const idKeyValue* kv = _decl->data.MatchPrefix(prefix, NULL); 
		 kv != NULL; 
		 kv = _decl->data.MatchPrefix(prefix, NULL))
	{
		_decl->data.Delete(kv->GetKey());
	}
}

void CModInfo::SaveToFile(idFile* file)
{
	if (_decl == NULL) return;

	_decl->Update(modName);
	_decl->SaveToFile(file);
}

idStr CModInfo::GetModFolderPath()
{
	return g_Global.GetModPath(modName.c_str()).c_str();
}

bool CModInfo::HasModNotes()
{
	// Check for the readme.txt file
	idStr notesFileName = cv_tdm_fm_path.GetString() + modName + "/" + cv_tdm_fm_notes_file.GetString();

	return fileSystem->ReadFile(notesFileName, NULL) != -1;
}

idStr CModInfo::GetModNotes()
{
	// Check for the readme.txt file
	idStr notesFileName = cv_tdm_fm_path.GetString() + modName + "/" + cv_tdm_fm_notes_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(notesFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not found
		return "";
	}

	idStr modNotes(buffer);
	fileSystem->FreeFile(buffer);

	return modNotes;
}

bool CModInfo::LoadMetaData()
{
	if (modName.IsEmpty()) 
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Cannot load mission information from darkmod.txt without mod name.\r");
		return false;
	}

	idStr fmPath = cv_tdm_fm_path.GetString() + modName + "/";

	// Check for the darkmod.txt file
	idStr descFileName = fmPath + cv_tdm_fm_desc_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(descFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not found
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Couldn't find darkmod.txt for mod %s.\r", modName.c_str());
		return false;
	}

	idStr modFileContent(buffer);
	fileSystem->FreeFile(buffer);

	if (modFileContent.IsEmpty())
	{
		// Failed to find info
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Empty darkmod.txt for mod %s.\r", modName.c_str());
		return false;
	}

	pathToFMPackage = fmPath;
	
	int titlePos = modFileContent.Find("Title:", false);
	int descPos = modFileContent.Find("Description:", false);
	int authorPos = modFileContent.Find("Author:", false);
	int versionPos = modFileContent.Find("Required TDM Version:", false);
	int missionTitlesPos = modFileContent.Find("Mission 1 Title:", false); // grayman #3733

	int len = modFileContent.Length();

	// FIX: Incorrect order in darkmod.txt or leaving out specific fields was causing crashes or incorrect data display
	std::set<int> pos;
	pos.insert(titlePos);
	pos.insert(descPos);
	pos.insert(authorPos);
	pos.insert(versionPos);
	pos.insert(missionTitlesPos);
	pos.insert(len);
	std::set<int>::iterator iter;

	if (titlePos >= 0)
	{
		//displayName = idStr(modFileContent, titlePos, (missionTitlesPos != -1) ? missionTitlesPos : (descPos != -1) ? descPos : len); // grayman #3733
		//displayName = idStr(modFileContent, titlePos, (descPos != -1) ? descPos : len);
		iter = pos.find(titlePos);
		iter++;
		displayName = idStr(modFileContent, titlePos, *iter);
		Strip("Title:", displayName);
	}

	_missionTitles.Clear();
	_missionTitles.Append(displayName); // [0] is the display name

	// grayman #3733 - read mission titles if they exist
	//idStr missionTitles = idStr(modFileContent, missionTitlesPos, (descPos != -1) ? descPos : len);
	iter = pos.find(missionTitlesPos);
	iter++;
	idStr missionTitles = idStr(modFileContent, missionTitlesPos, *iter);
	if (missionTitlesPos >= 0)
	{
		GetMissionTitles(missionTitles); // fills in _missionTitles
	}

	if (descPos >= 0)
	{
		//description = idStr(modFileContent, descPos, (authorPos != -1) ? authorPos : len);
		iter = pos.find(descPos);
		iter++;
		description = idStr(modFileContent, descPos, *iter);
		Strip("Description:", description);
	}

	if (authorPos >= 0)
	{
		//author = idStr(modFileContent, authorPos, (versionPos != -1) ? versionPos : len);
		iter = pos.find(authorPos);
		iter++;
		author = idStr(modFileContent, authorPos, *iter);
		Strip("Author:", author);
	}

	if (versionPos >= 0)
	{
		//requiredVersionStr = idStr(modFileContent, versionPos, len);
		iter = pos.find(versionPos);
		iter++;
		requiredVersionStr = idStr(modFileContent, versionPos, *iter);
		Strip("Required TDM Version:", requiredVersionStr);
		requiredVersionStr.StripLeadingOnce('v');	// tels: "v1.07" => "1.07"		(#3170)

		// Parse version
		int dotPos = requiredVersionStr.Find('.');

		if (dotPos != -1)
		{
			requiredMajor = atoi(requiredVersionStr.Mid(0, dotPos));
			requiredMinor = atoi(requiredVersionStr.Mid(dotPos + 1, requiredVersionStr.Length() - dotPos));
		}
	}

	// Check for mod image
	if (fileSystem->ReadFile(pathToFMPackage + cv_tdm_fm_splashimage_file.GetString(), NULL) != -1)
	{
		idStr splashImageName = cv_tdm_fm_splashimage_file.GetString();
		splashImageName.StripFileExtension();

		image = pathToFMPackage + splashImageName;
	}
    
	// gnartsch: Check for local localization pack
	isL10NpackInstalled = (fileSystem->FindFile(pathToFMPackage + modName + "_l10n.pk4") == FIND_YES);

	return true;
}

void CModInfo::Strip( const char *fieldname, idStr &input) {
	input.StripLeadingOnce(fieldname);
	input.StripWhitespace();
}

// grayman #3733 - read mission titles, if this is a campaign

void CModInfo::GetMissionTitles(idStr missionTitles)
{
	if (modName.IsEmpty())
	{
		return;
	}

	int startIndex = 0;
	idStr start = "Mission 1 Title: ";
	idStr end   = "Mission 2 Title: ";
	int endIndex = missionTitles.Find(end.c_str(), true); // grayman #3733
	bool finished = false;
	for ( int i = 1 ; ; i++ )
	{
		idStr title = idStr(missionTitles, startIndex, endIndex); // grayman #3733
		Strip(start.c_str(), title);
		_missionTitles.Append(title);
		start = end;
		startIndex = endIndex;
		if (finished)
		{
			break;
		}
		end = va("Mission %d Title: ",i+2);
		endIndex = missionTitles.Find(end.c_str(), true, startIndex); // grayman #3733
		if (endIndex < 0)
		{
			endIndex = missionTitles.Length();
			finished = true;
		}
	}
}
