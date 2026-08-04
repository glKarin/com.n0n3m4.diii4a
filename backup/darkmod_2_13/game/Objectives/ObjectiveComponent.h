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

#ifndef TDM_OBJECTIVE_COMPONENT_H
#define TDM_OBJECTIVE_COMPONENT_H

#include "precompiled.h"

/**
* Objective component action types
* NOTE: Any change to these must be kept up to date in gCompTypeName array, defined in ObjectiveComponent.cpp
**/
enum EComponentType
{
	// AI components - MUST BE KEPT TOGETHER IN THE ENUM because later these enums are used as an array index
	// COMP_KILL must be kept as the first one
	COMP_KILL, // also includes non-living things being destroyed
	COMP_KO,
	COMP_AI_FIND_ITEM,
	COMP_AI_FIND_BODY,
	COMP_ALERT,

	// END AI components that must be kept together
	COMP_DESTROY,		// Destroy an inanimate object
	COMP_ITEM,			// Add inventory item or imaginary loot (find object X)
	COMP_PICKPOCKET,	// Take inventory item from conscious AI
	COMP_LOCATION,		// Item X is at location Y
	COMP_CUSTOM_ASYNC,	// asynchronously updated custom objective (updated by mapper from script)

	// The following are special clocked components, updated in CMissionData::UpdateObjectives
	COMP_CUSTOM_CLOCKED,
	COMP_INFO_LOCATION, // like location, but uses existing info_location areas instead of an info_objectivelocation entity
	COMP_DISTANCE,		// distance from origin of ent X to that of ent Y

	// Readable-related
	COMP_READABLE_OPENED, // readable is opened by the player, since TDM 1.02
	COMP_READABLE_CLOSED, // readable is closed (can be considered "has read") by the player, since TDM 1.02
	COMP_READABLE_PAGE_REACHED, // readable is displaying a certain page, since TDM 1.02

	COMP_COUNT			// Dummy entry to yield the number of valid types
};

extern const char* gCompTypeName[];

/**
* Objective component specification types
**/
enum ESpecificationMethod
{
	// The following apply to both AIs and items
	SPEC_NONE,
	SPEC_NAME,
	SPEC_OVERALL,
	SPEC_GROUP,			// for inventory items, info_location groups, etc
	SPEC_CLASSNAME,		// soft/scripting classname
	SPEC_SPAWNCLASS,	// hard / SDK classname

	// Specifically for AI:
	SPEC_AI_TYPE,
	SPEC_AI_TEAM,
	SPEC_AI_INNOCENCE,
	SPEC_COUNT					// Dummy entry should not be used for anything
};

class CObjectiveComponent
{
public:
	friend class CMissionData;
	friend class CObjective;

	CObjectiveComponent();
	virtual ~CObjectiveComponent();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	/**
	* Update the state of the objective component.  
	* Returns true if the state has changed as of this call
	**/
	bool SetState( bool bState );

public:
	/**
	* Index of this component in the form of [objective num, component num]
	* NOTE: This index is that from the external scripting.
	* So it starts at 1, not at zero.
	**/
	int m_Index[2]; 

protected:

	/**
	* Set to true if the FM author has NOTted this component
	**/
	bool m_bNotted;
	
	EComponentType m_Type;

	// This could be made more general into a list, but I can't think of any component
	// types that would require more than 2 items to match.  More complicated logic
	// can be constructed out of multiple components.
	ESpecificationMethod m_SpecMethod[2];

	/**
	* Values of the specifier to match, e.g., if specmethod is group, specvalue is "beast"
	* Could be either an int or a string depending on spec type, so make room for both.
	**/
	idStr m_SpecVal[2];

	/**
	* Current component state (true/false)
	**/
	bool		m_bState;

	/**
	* Current count of the number of times this event
	* happened and the specifiers were matched.
	* Some objective components use this, others rely on
	* other counters, like overall stats or the inventory
	**/
	int			m_EventCount;

	/**
	* Set to true if this component is only satisfied when the player performs the action
	**/
	bool		m_bPlayerResponsibleOnly;

	/**
	* Whether the irreversible component has latched into a state
	**/
	bool		m_bLatched;

	idStrList	m_Args;

	// Only used by clocked objectives:
	int			m_ClockInterval; // milliseconds
	
	int			m_TimeStamp;

	/**
	* Whether the objective component latches after it changes once
	* Default is reversible.
	**/
	bool m_bReversible;

}; // CObjectiveComponent

#endif
