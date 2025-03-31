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

#ifndef BLOODMARKER_H
#define BLOODMARKER_H

#pragma hdrstop

#include "Entity.h"

class CBloodMarker : public idEntity
{
public:
	CLASS_PROTOTYPE( CBloodMarker );
	CBloodMarker();

protected:
	idStr					_bloodSplat;
	idStr					_bloodSplatFading;
	float					_angle;
	float					_size;

	// True if this bloodsplat is in the process of disappearing
	bool					_isFading;

	idEntityPtr<idAI>		_spilledBy; // grayman #3075

public:
	void					Init(const idStr& splat, const idStr& splatFading, float size, idAI* bleeder); // grayman #3075 - note who bled
	void					Event_GenerateBloodSplat();

	/**
	 * greebo: Overrides the OnStim method of the base class to check
	 * for water stims.
	 */
	virtual void			OnStim(const CStimPtr& stim, idEntity* stimSource) override;

	/**
	 * grayman #3075: get the AI that spilled this blood
	 */
	idAI*					GetSpilledBy(void);

	// Save and restore
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
};

// End of header wrapper
#endif
