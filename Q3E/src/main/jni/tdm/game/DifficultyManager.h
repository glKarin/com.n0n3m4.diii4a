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

#ifndef DIFFICULTY_MANAGER_H
#define DIFFICULTY_MANAGER_H

#include "DifficultySettings.h"

namespace difficulty {

#define DEFAULT_DIFFICULTY_ENTITYDEF "atdm:difficulty_settings_default"
#define DIFFICULTY_ENTITYDEF "atdm:difficulty_settings"

// number of difficulty levels
#define DIFFICULTY_COUNT 3

/**
 * greebo: The Difficulty Manager provides methods to load
 * the various spawnargs into the entities based on the
 * selected mission difficulty.
 *
 * During initialisation, the manager reads the default difficulty settings 
 * from the def/ folder. The procedure is as follows:
 *
 * 1) Read global default settings from the entities matching DIFFICULTY_ENTITYDEF.
 * 2) Search the map for tdm_difficulty_settings_map entities: these settings
 *    will override any default settings found in step 1 (settings that target the same
 *    entityclass/spawnarg combination will be removed and replaced by the ones defined in the map).
 */
class DifficultyManager
{
private:
	// The selected difficulty [0 .. DIFFICULTY_COUNT-1]
	int _difficulty;

	// The global difficulty settings (parsed from the entityDefs)
	DifficultySettings _globalSettings[DIFFICULTY_COUNT];

	// The difficulty settings affecting CVARs
	CVARDifficultySettings _cvarSettings[DIFFICULTY_COUNT];

	// The name of each difficultylevel
	idStr _difficultyNames[DIFFICULTY_COUNT];

public:
	// Constructor
	DifficultyManager();

	// Clears everything associated with difficulty settings.
	void Clear();

	/**
	 * greebo: Initialises this class. This means loading the global default
	 * difficulty settings from the entityDef files and the ones
	 * from the map file (worldspawn setting, map-specific difficulty).
	 */
	void Init(idMapFile* mapFile);
	
	// Accessor methods for the currently chosen difficulty level
	void SetDifficultyLevel(const int difficulty);
	int GetDifficultyLevel() const;

	// Returns the display name for the given level, which must an integer in [0..DIFFICULTY_COUNT)
	idStr GetDifficultyName(int level);

	/**
	 * greebo: Applies the spawnarg modifiers of the currently chosen
	 * difficulty level to the given set of spawnargs.
	 */
	void ApplyDifficultySettings(idDict& target);

	/** 
	 * greebo: Applies the CVAR difficulty settings, call this once before map start.
	 */
	void ApplyCVARDifficultySettings();

	/**
	 * greebo: Checks whether the given entity (represented by "target") is allowed to spawn.
	 * 
	 * @returns: TRUE if the entity should NOT be spawned.
	 */
	bool InhibitEntitySpawn(const idDict& target);

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

private:
	// Loads the default difficulty settings from the entityDefs
	void LoadDefaultDifficultySettings();

	// Loads the map-specific difficulty settings (these will override the default ones)
	void LoadMapDifficultySettings(idMapFile* mapFile);

	// Loads the map-specific difficulty settings from the given map entity
	void LoadMapDifficultySettings(idMapEntity* ent);
};

} // namespace difficulty

#endif /* DIFFICULTY_MANAGER_H */
