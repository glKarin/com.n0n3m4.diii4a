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

#ifndef TDM_OBJECTIVE_LOCATION_H
#define TDM_OBJECTIVE_LOCATION_H

#include "precompiled.h"

// Helper entity for objective locations
class CObjectiveLocation : public idEntity
{
public:
	CLASS_PROTOTYPE( CObjectiveLocation );
	
	CObjectiveLocation();

	virtual ~CObjectiveLocation() override;

	virtual void Think( void ) override;
	void Spawn( void );

	// Called by ~idEntity to catch entity destructions
	void OnEntityDestroyed(idEntity* ent);

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

protected:
	/**
	* Clock interval [seconds]
	**/
	int m_Interval;

	int m_TimeStamp;

	/**
	* List of entity names that intersected bounds in previous clock tick
	**/
	idList< idEntityPtr<idEntity> >	m_EntsInBounds;
	/**
	* Objective system: Location's objective group name for objective checks
	**/
	idStr		m_ObjectiveGroup;

private:
	idClipModel *		clipModel;

}; // CObjectiveLocation

#endif
