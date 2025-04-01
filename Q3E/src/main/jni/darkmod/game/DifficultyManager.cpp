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



#include "DifficultyManager.h"
#include "Game_local.h"
#include "Objectives/MissionData.h"

namespace difficulty {

// Constructor
DifficultyManager::DifficultyManager() :
	_difficulty(0)
{}

void DifficultyManager::Clear()
{
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Clear();
		_difficultyNames[i] = "";
		_cvarSettings[i].Clear();
	}
}

void DifficultyManager::Init(idMapFile* mapFile)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Searching for difficulty setting on worldspawn.\r");

	if (mapFile->GetNumEntities() <= 0) {
		return; // no entities!
	}

	// Fetch the worldspawn
	idMapEntity* mapEnt = mapFile->GetEntity(0);
	idDict spawnArgs = mapEnt->epairs;

	int mapDifficulty;
	if (spawnArgs.GetInt("difficulty", "0", mapDifficulty))
	{
		if ( mapDifficulty < 0 || mapDifficulty > 2 ) // #3928: give a sensible error instead of crash if bad value used
		{
			gameLocal.Error( "Invalid difficulty setting %d on worldspawn entity, value must be 0, 1 or 2.", mapDifficulty );
		}
		else
		{
						// We have a difficulty spawnarg set on the map's worldspawn, take it as override value
			DM_LOG( LC_DIFFICULTY, LT_DEBUG )LOGSTRING( "Found overriding difficulty setting on worldspawn entity: %d.\r", mapDifficulty );
			_difficulty = mapDifficulty;
		}
	}

	// Check for the CVAR, which might override any setting
	if (cv_tdm_difficulty.GetInteger() >= 0)
	{
		_difficulty = cv_tdm_difficulty.GetInteger();
		DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Found overriding CVAR 'tdm_difficulty': %d.\r", _difficulty);
	}

	// Clear the CVAR settings before parsing
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_cvarSettings[i].Clear();
	}

	// Load the default difficulty settings from the entityDefs
	LoadDefaultDifficultySettings();

	LoadMapDifficultySettings(mapFile);
}

void DifficultyManager::SetDifficultyLevel(const int difficulty)
{
	_difficulty = difficulty;
}

int DifficultyManager::GetDifficultyLevel() const
{
	return _difficulty;
}

idStr DifficultyManager::GetDifficultyName(int level)
{
	assert( ( level >= 0 ) && ( level < DIFFICULTY_COUNT ) );

	if ( _difficultyNames[level].Length() > 0 )
	{
		// Tels: Attempt to translate the name, in case the mapper used something like "#str_01234"
		return common->Translate( _difficultyNames[level] );
	}

	const idDecl* diffDecl = declManager->FindType(DECL_ENTITYDEF, "difficultyMenu", false);
	const idDeclEntityDef* diffDef = static_cast<const idDeclEntityDef*>(diffDecl);

	// Tels: Translate default difficulty names
	return common->Translate( diffDef->dict.GetString(va("diff%ddefault", level), "") );
}

void DifficultyManager::Save(idSaveGame* savefile)
{
	savefile->WriteInt(_difficulty);
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Save(savefile);
		_cvarSettings[i].Save(savefile);
		savefile->WriteString(_difficultyNames[i]);
	}
}

void DifficultyManager::Restore(idRestoreGame* savefile)
{
	Clear(); // clear stuff before loading

	savefile->ReadInt(_difficulty);
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].Restore(savefile);
		_cvarSettings[i].Restore(savefile);
		savefile->ReadString(_difficultyNames[i]);
	}
}

void DifficultyManager::ApplyDifficultySettings(idDict& target)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Applying difficulty settings to entity: %s.\r", target.GetString("name"));

	_globalSettings[_difficulty].ApplySettings(target);
}

void DifficultyManager::ApplyCVARDifficultySettings()
{
	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Applying CVAR difficulty settings\r");

	_cvarSettings[_difficulty].ApplySettings();
}

bool DifficultyManager::InhibitEntitySpawn(const idDict& target) {
	bool isAllowed(true);

	// Construct the key ("diff_0_spawn")
	idStr key = va("diff_%d_nospawn", _difficulty);

	// The entity is allowed to spawn by default, must be set to 1 by the mapper
	isAllowed = !target.GetBool(key, "0");

	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Entity %s is allowed to spawn on difficulty %i: %s.\r", target.GetString("name"), _difficulty, isAllowed ? "YES" : "NO");

	// Return false if the entity is allowed to spawn
	return !isAllowed;
}

void DifficultyManager::LoadDefaultDifficultySettings()
{
	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Trying to load default difficulty settings from entityDefs.\r");

	// Construct the entityDef name (e.g. atdm:difficulty_settings_default)
	idStr defName(DEFAULT_DIFFICULTY_ENTITYDEF);

	const idDict* difficultyDict = gameLocal.FindEntityDefDict(defName, true); // grayman #3391 - don't create a default 'difficultyDict'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message

	if (difficultyDict != NULL)
	{
		DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Found difficulty settings: %s.\r", defName.c_str());

		// greebo: Try to lookup the entityDef for each difficulty level and load the settings
		for (int i = 0; i < DIFFICULTY_COUNT; i++)
		{
			// Let the setting structure know which level it is referring to
			_globalSettings[i].SetLevel(i);
			// And load the settings
			_globalSettings[i].LoadFromEntityDef(*difficultyDict);

			// Load the CVAR settings too
			_cvarSettings[i].SetLevel(i);
			_cvarSettings[i].LoadFromEntityDef(*difficultyDict);
		}
	}
	else
	{
		for (int i = 0; i < DIFFICULTY_COUNT; i++)
		{
			_globalSettings[i].Clear();
			_cvarSettings[i].Clear();
		}
		gameLocal.Warning("DifficultyManager: Could not find default difficulty entityDef!");
	}
}

void DifficultyManager::LoadMapDifficultySettings(idMapFile* mapFile)
{
	DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Trying to load map-specific difficulty settings.\r");

	if (mapFile == NULL) return;

	for (int i = 0; i < mapFile->GetNumEntities(); i++)
	{
		idMapEntity* ent = mapFile->GetEntity(i);

		if (idStr::Icmp(ent->epairs.GetString("classname"), DIFFICULTY_ENTITYDEF) == 0)
		{
			LoadMapDifficultySettings(ent);
		}
	}

	// greebo: Find out the names of the difficulty settings
	idMapEntity* worldSpawn = mapFile->GetEntity(0);
	const idDict& mapDict = worldSpawn->epairs;

	// Determine the difficulty level string. The defaults are the "difficultyMenu" entityDef.
	// Maps can override these values by use of the difficulty#Name value on the spawnargs of 
	// the worldspawn.
	const idDecl* diffDecl = declManager->FindType(DECL_ENTITYDEF, "difficultyMenu", false);
	const idDeclEntityDef* diffDef = static_cast<const idDeclEntityDef*>(diffDecl);
	for (int diffLevel = 0; diffLevel < DIFFICULTY_COUNT; diffLevel++)
	{
		// Tels: #3411 if the name is a hard-coded english name, turn it into a #str_12345 template if possible
		_difficultyNames[diffLevel] = common->GetI18N()->TemplateFromEnglish(
			mapDict.GetString(
				va("difficulty%dName",diffLevel),
				diffDef->dict.GetString(va("diff%ddefault",diffLevel), "")
			)
		);
	}
	gameLocal.m_MissionData->SetDifficultyNames(_difficultyNames); // grayman #3292
}

void DifficultyManager::LoadMapDifficultySettings(idMapEntity* ent)
{
	if (ent == NULL) return;

	// greebo: Let each global settings structure investigate the settings 
	// on this entity.
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		_globalSettings[i].LoadFromMapEntity(ent);
		_cvarSettings[i].LoadFromMapEntity(ent);
	}
}

} // namespace difficulty
