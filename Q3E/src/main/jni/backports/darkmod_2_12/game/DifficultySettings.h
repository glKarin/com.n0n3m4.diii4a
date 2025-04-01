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

#ifndef __DIFFICULTY_SETTINGS_H__
#define __DIFFICULTY_SETTINGS_H__

#include <map>
#include <list>
#include <string>

namespace difficulty {

/**
 * greebo: A Setting represents a spawnarg change.
 * This can be an assignment, addition or multiplication.
 */
class Setting
{
public:
	enum EApplicationType {
		EAssign,
		EAdd,
		EMultiply,
		EIgnore,
	};

	// The classname this setting applies to
	idStr className;

	// The target spawnarg to be changed
	idStr spawnArg;
	
	// The parsed argument (the specifier (+/*) has already been removed)
	idStr argument;

	// How the argument should be applied
	EApplicationType appType;

	// Whether this setting is valid
	bool isValid;

	// Default constructor
	Setting();

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile);
	virtual void Restore(idRestoreGame* savefile);

	// Applies this setting to the given spawnargs
	void Apply(idDict& target);

	// Load the settings with the given <index> matching the given <level> from the given dict.
	virtual void ParseFromDict(const idDict& dict, int level, int index);

	// Factory function: get all Settings from the given dict (matching the given <level>)
	// The returned list is guaranteed to contain only valid settings.
	static idList<Setting> ParseSettingsFromDict(const idDict& dict, int level);
};

/**
 * greebo: This class encapsulates the difficulty settings
 * for a given difficulty level (easy/medium/hard/etc).
 *
 * Use the ApplySettings() method to apply the settings on a set of spawnargs.
 */
class DifficultySettings
{
private:
	// The settings map associates classnames with spawnarg change records.
	// Multiple settings can be made for a single classname.
	typedef std::multimap<std::string, Setting> SettingsMap;
	SettingsMap _settings;

	// The inheritance chain class stored in vector
	typedef std::vector<std::string> InheritanceChain;
	// This data structure maps each classname to its inheritance chain
	typedef std::map<std::string, InheritanceChain> InheritanceChainsMap;
	InheritanceChainsMap _inheritanceChains;

	// the difficulty level these settings are referring to
	int _level; 

public:
	// Wipes the contents of this class
	void Clear();

	// Sets the level of these settings
	void SetLevel(int level);
	int GetLevel() const;

	/**
	 * greebo: Loads the difficulty settings from the given entityDef.
	 * This considers only settings matching the level of this class.
	 */
	void LoadFromEntityDef(const idDict& defDict);

	/**
	 * greebo: This loads the difficulty settings from the given map entity.
	 * Settings loaded from the entity will replace settings with the same 
	 * classname/spawnarg combination found in the default entityDefs.
	 */
	void LoadFromMapEntity(idMapEntity* ent);

	/**
	 * greebo: Applies the contained difficulty settings on the given set of spawnargs.
	 */
	void ApplySettings(idDict& target);

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

private:
	// greebo: Returns the value of the "inherit" spawnarg for the given classname
	// This parses the raw declaration text on a char-per-char basis, this is 
	// necessary because the "inherit" key gets removed by the entityDef parser after loading.
	std::string GetInheritValue(const std::string& className);

	// Returns the inheritance chain for the given dict
	InheritanceChain GetInheritanceChain(const idDict& dict);
};

/**
 * greebo: A CVARSetting represents a CVAR change.
 * This can be an assignment, addition or multiplication.
 */
class CVARSetting :
	public Setting
{
public:
	// The cvar name this setting applies to
	idStr cvar;
	
	// Default constructor
	CVARSetting();

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Applies this setting
	void Apply();

	// Load the settings with the given <index> matching the given <level> from the given dict.
	virtual void ParseFromDict(const idDict& dict, int level, int index) override;
};

/**
 * greebo: This class encapsulates the difficulty settings affecting CVARs
 * for a given difficulty level (easy/medium/hard/etc).
 *
 * Use the ApplySettings() method before map start to activate the settings.
 */
class CVARDifficultySettings
{
private:
	// The settings list
	typedef idList<CVARSetting> SettingsMap;
	SettingsMap _settings;

	// the difficulty level these settings are referring to
	int _level;

public:
	// Wipes the contents of this class
	void Clear();

	// Sets the level of these settings
	void SetLevel(int level);
	int GetLevel() const;

	/**
	 * greebo: Loads the difficulty settings from the given entityDef.
	 * This considers only settings matching the level of this class.
	 */
	void LoadFromEntityDef(const idDict& defDict);

	/**
	 * greebo: This loads the difficulty settings from the given map entity.
	 * Settings loaded from the entity will replace settings with the same 
	 * classname/spawnarg combination found in the default entityDefs.
	 */
	void LoadFromMapEntity(idMapEntity* ent);

	/**
	 * greebo: Applies the contained difficulty settings.
	 */
	void ApplySettings();

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);
};

} // namespace difficulty

#endif /* __DIFFICULTY_SETTINGS_H__ */
