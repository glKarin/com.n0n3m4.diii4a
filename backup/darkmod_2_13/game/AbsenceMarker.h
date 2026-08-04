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

#ifndef ABSENCEMARKER_H
#define ABSENCEMARKER_H

#pragma hdrstop

#include "Entity.h"

/**
* The purpose of this entity subclass is to act as a marker for other entities that have
* been moved or destroyed. An instance of this class can be spawned when the other entity
* is removed, destroyed, moved or re-oriented in a noticeable fashion.  If an instance
* of this class has a stim, it can signal entities to notice it, and then provide them
* with a copy of the missing entities spawn args so that a script will konw how to
* react to the absence.
*
* @author SophisticatedZombie
* @project The Dark Mod
* @copyright 2006 The Dark Mod team
*
*/
class CAbsenceMarker : public idEntity
{
public:
	CLASS_PROTOTYPE( CAbsenceMarker );

	int ownerTeam;

protected:

	// Defines the spawnargs etc.. for this entity's script type
	idStr referenced_entityDefName;
	int referenced_entityDefNumber;

	// The name of the entity being referenced
	idStr referenced_entityName;

	// The spawn args of the entity being referenced
	idDict referenced_spawnArgs;


public:

	CAbsenceMarker(void);

	const idDict&			GetRefSpawnargs() const;

	// Save and restore
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	* Call this method to set the information about the entity
	* which this references as "missing" from its normal location.
	*
	* @param absetEntity idEntityPtr indicating the entity who's
	*	absence we are marking.
	*/
	bool initAbsenceReference(idEntity* owner, idBounds& startBounds);
};


// End of header wrapper
#endif
