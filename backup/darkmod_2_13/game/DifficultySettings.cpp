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



#include "DifficultySettings.h"
#include <vector>

namespace difficulty {

#define PATTERN_DIFF "diff_%d_"
#define PREFIX_CLASS "class_"
#define PREFIX_CHANGE "change_"
#define PREFIX_CVAR "cvar_"

#define PATTERN_CLASS "diff_%d_class_%d"
#define PATTERN_CHANGE "diff_%d_change_%d"
#define PATTERN_ARG "diff_%d_arg_%d"
#define PATTERN_CVAR "diff_%d_cvar_%d"

#define APPTYPE_IGNORE "_IGNORE"

Setting::Setting() :
	isValid(false)
{}

void Setting::Save(idSaveGame* savefile)
{
	savefile->WriteBool(isValid);
	savefile->WriteString(className);
	savefile->WriteString(spawnArg);
	savefile->WriteString(argument);
	savefile->WriteInt(static_cast<int>(appType));
}

void Setting::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool(isValid);
	savefile->ReadString(className);
	savefile->ReadString(spawnArg);
	savefile->ReadString(argument);

	int temp;
	savefile->ReadInt(temp);
	appType = static_cast<EApplicationType>(temp);
}

void Setting::Apply(idDict& target)
{
	switch (appType) 
	{
		case EAssign:
			target.Set(spawnArg, argument);
			break;
		case EAdd:
			// Convert the old setting to float, add the argument, convert back to string and set as value
			target.Set(spawnArg, idStr(float(target.GetFloat(spawnArg) + atof(argument))));
			break;
		case EMultiply:
			// Convert the old setting to float, add the argument, convert back to string and set as value
			target.Set(spawnArg, idStr(float(target.GetFloat(spawnArg) * atof(argument))));
			break;
		case EIgnore:
			// Ignore => do nothing
			break;
		default:
			break;
	};
}

void Setting::ParseFromDict(const idDict& dict, int level, int index)
{
	isValid = true; // in dubio pro reo

	// Get the classname, target spawnarg and argument
	className = dict.GetString(va(PATTERN_CLASS, level, index));
	spawnArg = dict.GetString(va(PATTERN_CHANGE, level, index));
	argument = dict.GetString(va(PATTERN_ARG, level, index));

	// Parse the application type
	appType = EAssign;

	if (!argument.IsEmpty())
	{
		// Check for ignore argument
		if (argument == APPTYPE_IGNORE)
		{
			appType = EIgnore;
			argument.Clear(); // clear the argument
		}
		else if (argument.Find(' ') != -1)
		{
			// greebo: We have a space in the argument, hence it cannot be 
			// a mathematical operation. This usually applies to vector arguments
			// like '-205 10 20', which can contain a leading minus sign.
		}
		// Check for special modifiers
		else if (argument[0] == '+')
		{
			appType = EAdd;
			// Remove the first character
			argument = idStr(argument, 1, argument.Length());
		}
		else if (argument[0] == '*')
		{
			appType = EMultiply;
			// Remove the first character
			argument = idStr(argument, 1, argument.Length());
		}
		else if (argument[0] == '-')
		{
			appType = EAdd;
			// Leave the "-" sign, it will be the sign of the parsed int
		}
	}

	if (spawnArg.IsEmpty())
	{
		// Spawnarg must not be empty
		isValid = false;
	}

	// classname can be empty (this is valid for entity-specific difficulties)
}

// Static parsing function
idList<Setting> Setting::ParseSettingsFromDict(const idDict& dict, int level)
{
	idList<Setting> list;

	// Cycle through all difficulty settings (looking for "diff_0_change_*")
	idStr prefix = idStr(va(PATTERN_DIFF, level)) + PREFIX_CHANGE;
	for (const idKeyValue* keyVal = dict.MatchPrefix(prefix);
		  keyVal != NULL;
		  keyVal = dict.MatchPrefix(prefix, keyVal))
	{
		DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Parsing keyvalue: %s = %s.\r", keyVal->GetKey().c_str(), keyVal->GetValue().c_str());

		// Get the index from this keyvalue (remove the prefix and convert to int)
		idStr key = keyVal->GetKey();
		key.StripLeadingOnce(prefix);
		if (key.IsNumeric())
		{
			// Extract the index
			int index = atoi(key);

			// Parse the settings with the given index
			Setting s;
			s.ParseFromDict(dict, level, index);

			// Check for validity and insert into map
			if (s.isValid)
			{
				list.Append(s);
			}
		}
		else
		{
			gameLocal.Warning("Found invalid difficulty settings index: %s.", keyVal->GetKey().c_str());
			DM_LOG(LC_DIFFICULTY, LT_ERROR)LOGSTRING("Found invalid difficulty settings index: %s.\r", keyVal->GetKey().c_str());
		}
	}

	return list;
}

// =======================================================================

void DifficultySettings::Clear()
{
	_settings.clear();
	_inheritanceChains.clear();
	_level = 0;
}

void DifficultySettings::SetLevel(int level)
{
	_level = level;
}

int DifficultySettings::GetLevel() const
{
	return _level;
}

void DifficultySettings::LoadFromEntityDef(const idDict& defDict)
{
	// Parse all settings from the given entityDef
	idList<Setting> settings = Setting::ParseSettingsFromDict(defDict, _level);

	// Copy all settings into the SettingsMap
	for (int i = 0; i < settings.Num(); i++)
	{
		_settings.insert(SettingsMap::value_type(settings[i].className.c_str(), settings[i]));
	}
}

void DifficultySettings::LoadFromMapEntity(idMapEntity* ent)
{
	// Search the epairs for difficulty settings
	idList<Setting> settings = Setting::ParseSettingsFromDict(ent->epairs, _level);

	// greebo: Go through all found settings and remove all default
	// settings with the same class/spawnarg combination
	for (int i = 0; i < settings.Num(); i++)
	{
		// We need std::string for STL map lookups, not idStr
		std::string className = settings[i].className.c_str();

		// Search all stored settings matching this classname
		for (SettingsMap::iterator found = _settings.find(className);
			 found != _settings.upper_bound(className) && found != _settings.end();
			 /* in-loop increment */)
		{
			if (found->second.spawnArg == settings[i].spawnArg)
			{
				// Spawnarg and classname match, remove it and post-increment the iterator
				_settings.erase(found++);
			}
			else
			{
				++found; // no match, step forward
			}
		}
	}

	// Now copy all spawnargs into the SettingsMap
	for (int i = 0; i < settings.Num(); i++)
	{
		_settings.insert(SettingsMap::value_type(settings[i].className.c_str(), settings[i]));
	}
}

void DifficultySettings::ApplySettings(idDict& target)
{
	std::string eclass = target.GetString("classname");

	if (eclass.empty()) {
		return; // no classname, no rules
	}

	// greebo: First, get the list of entity-specific difficulty settings from the dictionary
	// Everything processed here will be ignored in the second run (where the default settings are applied)
	idList<Setting> entSettings = Setting::ParseSettingsFromDict(target, _level);
	DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Found %d difficulty settings on the entity %s.\r", entSettings.Num(), target.GetString("name"));

	// Apply the settings one by one
	for (int i = 0; i < entSettings.Num(); i++)
	{
		DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Applying entity-specific setting: %s => %s.\r", entSettings[i].spawnArg.c_str(), entSettings[i].argument.c_str());
		entSettings[i].Apply(target);
	}

	// Second step: apply global settings

	// Get the inheritancechain for the given target dict
	const InheritanceChain &inheritanceChain = GetInheritanceChain(target);

	// Go through the inheritance chain front to back and apply the settings
	for (InheritanceChain::const_iterator c = inheritanceChain.begin(); c != inheritanceChain.end(); ++c)
	{
		std::string className = *c;

		// Process the list of default settings that apply to this entity class,
		// but ignore all keys that have been addressed by the entity-specific settings.
		for (SettingsMap::iterator i = _settings.find(className);
			 i != _settings.upper_bound(className) && i != _settings.end();
			 ++i)
		{
			Setting& setting = i->second;
			bool settingApplicable = true;

			// Check if the spawnarg has been processed in the entity-specific settings
			for (int k = 0; k < entSettings.Num(); k++)
			{
				if (entSettings[k].spawnArg == setting.spawnArg)
				{
					// This target spawnarg has already been processed in the first run, skip it
					DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Ignoring global setting: %s => %s.\r", setting.spawnArg.c_str(), setting.argument.c_str());
					settingApplicable = false;
					break;
				}
			}

			if (settingApplicable)
			{
				// We have green light, apply the setting
				DM_LOG(LC_DIFFICULTY, LT_DEBUG)LOGSTRING("Applying global setting: %s => %s.\r", setting.spawnArg.c_str(), setting.argument.c_str());
				setting.Apply(target);
			}
		}
	}
}

// Save/Restore methods
void DifficultySettings::Save(idSaveGame* savefile)
{
	savefile->WriteInt(static_cast<int>(_settings.size()));
	for (SettingsMap::iterator i = _settings.begin(); i != _settings.end(); ++i)
	{
		idStr className(i->second.className.c_str());
		// Save the key and the value
		savefile->WriteString(className); // key
		i->second.Save(savefile); // value
	}
	savefile->WriteInt(static_cast<int>(_inheritanceChains.size()));
	for (InheritanceChainsMap::iterator i = _inheritanceChains.begin(); i != _inheritanceChains.end(); ++i)
	{	//save the whole map
		savefile->WriteString(i->first.c_str());
		const InheritanceChain &chain = i->second;
		savefile->WriteInt(static_cast<int>(chain.size()));
		for (InheritanceChain::const_iterator j = chain.begin(); j != chain.end(); j++)
			savefile->WriteString(j->c_str());
	}
	savefile->WriteInt(_level);
}

void DifficultySettings::Restore(idRestoreGame* savefile)
{
	Clear(); // always clear before loading

	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		idStr className;
		savefile->ReadString(className);

		// Insert an empty structure into the map
		SettingsMap::iterator inserted = _settings.insert(
			SettingsMap::value_type(className.c_str(), Setting())
		);

		// Now restore the struct itself
		inserted->second.Restore(savefile);
	}
	savefile->ReadInt(num);
	for (int i = 0; i<num; i++)
	{	//restore the whole map
		idStr className;
		savefile->ReadString(className);

		int k;
		savefile->ReadInt(k);
		InheritanceChain chain;
		for (int j = 0; j<k; j++) {
			idStr str;
			savefile->ReadString(str);
			chain.push_back(std::string(str.c_str()));
		}

		_inheritanceChains[std::string(className.c_str())] = chain;
	}
	savefile->ReadInt(_level);
}

DifficultySettings::InheritanceChain DifficultySettings::GetInheritanceChain(const idDict& dict)
{
	std::string className = dict.GetString("classname");

	// stgatilov: Look the class name up in the chains cache
	InheritanceChainsMap::iterator it = _inheritanceChains.find(className);
	if (it != _inheritanceChains.end())
		return it->second;

	InheritanceChain inheritanceChain;

	// Add the classname itself to the end of the list
	inheritanceChain.push_back(className);

	// greebo: Extract the inherit value from the raw declaration text, 
	// as the "inherit" key has been removed in the given "dict"
	for (std::string inherit = GetInheritValue(className); 
		!inherit.empty();
		inherit = GetInheritValue(inherit))
	{
		// Has parent, add to list
		inheritanceChain.push_back(inherit);
	}

	// stgatilov: reverse the chain so that parents go first
	std::reverse(inheritanceChain.begin(), inheritanceChain.end());

	// stgatilov: save the chain in cache
	_inheritanceChains[className] = inheritanceChain;

	return inheritanceChain;
}

std::string DifficultySettings::GetInheritValue(const std::string& className)
{
	// Get the raw declaration, in the parsed entitydefs, all "inherit" keys have been remoed
	const idDecl* decl = declManager->FindType(DECL_ENTITYDEF, className.c_str(), false);

	if (decl == NULL)
	{
		return ""; // no declaration found...
	}

	// Get the raw text from the declaration manager
	std::string buffer;
	buffer.resize(decl->GetTextLength()+1);
	decl->GetText(&buffer[0]);

	// Find the "inherit" key and parse the value
	std::size_t pos = buffer.find("\"inherit\"");
	if (pos == std::string::npos) {
		return ""; // not found
	}

	pos += 9; // skip "inherit"

	while (buffer[pos] != '"' && pos < buffer.size()) {
		pos++; // skip everything till first "
	}

	pos++; // skip "

	std::string inherit;

	while (buffer[pos] != '"' && pos < buffer.size()) {
		inherit += buffer[pos];
		pos++;
	}
	
	return inherit;
}

// =======================================================================

CVARSetting::CVARSetting()
{}

void CVARSetting::Save(idSaveGame* savefile)
{
	Setting::Save(savefile);

	savefile->WriteString(cvar);
}

void CVARSetting::Restore(idRestoreGame* savefile)
{
	Setting::Restore(savefile);

	savefile->ReadString(cvar);
}

void CVARSetting::Apply()
{
	idCVar* cv = cvarSystem->Find(cvar);

	if (cv == NULL) 
	{
		gameLocal.Warning("Difficulty setting references unknown CVAR %s", cvar.c_str());
		return;
	}

	switch (appType) 
	{
		case EAssign:
			if (argument.IsNumeric())
			{
				cv->SetFloat(atof(argument));
			}
			else
			{
				cv->SetString(argument);
			}
			break;
		case EAdd:
			// Convert the old setting to float, add the argument, convert back to string and set as value
			cv->SetFloat(cv->GetFloat() + atof(argument));
			break;
		case EMultiply:
			// Convert the old setting to float, add the argument, convert back to string and set as value
			cv->SetFloat(cv->GetFloat() * atof(argument));
			break;
		case EIgnore:
			// Ignore => do nothing
			break;
		default:
			break;
	};
}

void CVARSetting::ParseFromDict(const idDict& dict, int level, int index)
{
	// Call the base class first to parse the arguments and stuff
	Setting::ParseFromDict(dict, level, index);

	// Parse the CVAR name
	cvar = dict.GetString(va(PATTERN_CVAR, level, index));

	// CVAR must not be empty, everything else is not that important
	isValid = !cvar.IsEmpty();
}

// =======================================================================

void CVARDifficultySettings::Clear()
{
	_settings.ClearFree();
	_level = -1;
}

// Sets the level of these settings
void CVARDifficultySettings::SetLevel(int level)
{
	_level = level;
}

int CVARDifficultySettings::GetLevel() const
{
	return _level;
}

void CVARDifficultySettings::LoadFromEntityDef(const idDict& dict)
{
	// Cycle through all difficulty settings (looking for "diff_0_cvar_*")
	idStr prefix = idStr(va(PATTERN_DIFF, _level)) + PREFIX_CVAR;

	for (const idKeyValue* keyVal = dict.MatchPrefix(prefix);
		  keyVal != NULL;
		  keyVal = dict.MatchPrefix(prefix, keyVal))
	{
		DM_LOG(LC_DIFFICULTY, LT_INFO)LOGSTRING("Parsing cvar keyvalue: %s = %s.\r", keyVal->GetKey().c_str(), keyVal->GetValue().c_str());

		// Get the index from this keyvalue (remove the prefix and convert to int)
		idStr key = keyVal->GetKey();
		key.StripLeadingOnce(prefix);

		if (key.IsNumeric())
		{
			// Extract the index
			int index = atoi(key);

			// Parse the setting with the given index
			CVARSetting s;
			s.ParseFromDict(dict, _level, index);

			// Check for validity and insert into map
			if (s.isValid)
			{
				_settings.Append(s);
			}
		}
		else
		{
			gameLocal.Warning("Found invalid cvar difficulty settings index: %s.", keyVal->GetKey().c_str());
			DM_LOG(LC_DIFFICULTY, LT_ERROR)LOGSTRING("Found invalid cvar difficulty settings index: %s.\r", keyVal->GetKey().c_str());
		}
	}
}

void CVARDifficultySettings::LoadFromMapEntity(idMapEntity* ent)
{
	LoadFromEntityDef(ent->epairs);
}

void CVARDifficultySettings::ApplySettings()
{
	for (int i = 0; i < _settings.Num(); ++i)
	{
		_settings[i].Apply();
	}
}

void CVARDifficultySettings::Save(idSaveGame* savefile)
{
	savefile->WriteInt(_settings.Num());

	for (int i = 0; i < _settings.Num(); ++i)
	{
		_settings[i].Save(savefile);
	}
}

void CVARDifficultySettings::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);

	_settings.SetNum(num);
	for (int i = 0; i < _settings.Num(); ++i)
	{
		_settings[i].Restore(savefile);
	}
}

} // namespace difficulty
