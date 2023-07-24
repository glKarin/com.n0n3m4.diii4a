// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LIMBOPROPERTIES_H__
#define __LIMBOPROPERTIES_H__

#include "guis/UserInterfaceTypes.h"

SD_UI_PUSH_CLASS_TAG( sdLimboProperties )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"Limbo properties are used by the limbo menu. Use \"limbo.\" to access the properties. " \
	"Never use \"player.<propname>\" properties in the limbo menu. Many \"player.\" properties represent the local player OR " \
	"the player that the local player is spectating. Always use the limbo.<propname> equivalents as they only ever " \
	"refer to the local player. All player properties are read only."
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "limbo";
)
class sdLimboProperties : public sdUIPropertyHolder {
public:
											sdLimboProperties( void );
											~sdLimboProperties( void );

	virtual sdProperties::sdProperty*		GetProperty( const char* name );
	virtual sdProperties::sdProperty*		GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler& GetProperties() { return properties; }
	virtual const char*						GetName() const { return "limboProperties"; }
	virtual const char*						FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	void									Update( void );
	void									Init( void );
	void									Shutdown( void );	
	void									SetProficiencySource( const char* className );
	void									OnSetActiveSpawn( idEntity* newSpawn );

private:
	void									UpdateProficiency( idPlayer* player, const sdDeclPlayerClass* pc );

private:
	const sdDeclPlayerClass*				proficiencySource;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/Role";
	desc				= "Selected role lookup name.";
	datatype			= "string";
	)
	sdStringProperty						role;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/Name";
	desc				= "Players current name with color codes and clan tag.";
	datatype			= "string";
	)
	sdStringProperty						name;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/TeamName";
	desc				= "Players current team lookup name.";
	datatype			= "string";
	)
	sdStringProperty						teamName;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/WeaponIndex";
	desc				= "Current weapon selection index.";
	datatype			= "float";
	)
	sdFloatProperty							weaponIndex;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/Rank";
	desc				= "Players current rank text handle.";
	datatype			= "int";
	)
	sdIntProperty							rank;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/Xp";
	desc				= "Players XP.";
	datatype			= "float";
	)
	sdFloatProperty							xp;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/RankIcon";
	desc				= "Players current rank icon.";
	datatype			= "string";
	)
	sdStringProperty						rankMaterial;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/MatchTime";
	desc				= "Current match time.";
	datatype			= "float";
	)
	sdFloatProperty							matchTime;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/AvailablePlayZones";
	desc				= "Number of play zones, usually one.";
	datatype			= "float";
	)
	sdFloatProperty							availablePlayZones;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/DefaultPlayZone";
	desc				= "The play zone the player currently is in.";
	datatype			= "float";
	)
	sdFloatProperty							defaultPlayZone;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/SpawnLocation";
	desc				= "Localized text of the players current spawn location.";
	datatype			= "wstring";
	)
	sdWStringProperty						spawnLocation;

	SD_UI_PUSH_GROUP_TAG( "Player Proficiency Properties" )

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/Proficiency";
	desc				= "Proficiency level for \"proficiencyX\" where X is from 0 to 4.";
	datatype			= "float";
	)
	idList< sdFloatProperty	>				proficiency;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyPercent";
	desc				= "Percent complete for \"proficiencyPercentX\" where X is from 0 to 4.";
	datatype			= "float";
	)
	idList< sdFloatProperty	>				proficiencyPercent;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyLevels";
	desc				= "Number of proficiency levels for \"proficiencyLevelsX\" where X is from 0 to 4.";
	datatype			= "float";
	)
	idList< sdFloatProperty	>				proficiencyLevels;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyTitle";
	desc				= "Proficiency title for \"proficiencyTitleX\" where X is from 0 to 4.";
	datatype			= "int";
	)
	idList< sdIntProperty >					proficiencyTitle;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyID";
	desc				= "Proficiency ID (an index) for \"proficiencyIDX\" where X is from 0 to 4.";
	datatype			= "float";
	)
	idList< sdFloatProperty >				proficiencyID;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyXP";
	desc				= "Proficiency XP gained for \"proficiencyXPX\" where X is from 0 to 4.";
	datatype			= "float";
	)
	idList< sdFloatProperty >				proficiencyXP;

	SD_UI_PROPERTY_TAG(
	title				= "LimboProperties/ProficiencyName";
	desc				= "Proficiency Name for \"proficiencyNameX\" where X is from 0 to 4.";
	datatype			= "string";
	)
	idList< sdStringProperty >				proficiencyName;

	SD_UI_POP_GROUP_TAG

	sdProperties::sdPropertyHandler			properties;
};

#endif // __LIMBOPROPERTIES_H__
