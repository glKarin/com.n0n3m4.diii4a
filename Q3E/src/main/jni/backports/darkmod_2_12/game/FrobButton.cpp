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



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "FrobButton.h"
#include "SndProp.h"

//===============================================================================
// CFrobButton
//===============================================================================

const idEventDef EV_TDM_Operate("Operate", EventArgs(), EV_RETURNS_VOID, "Call this to operate this entity.");

CLASS_DECLARATION( CBinaryFrobMover, CFrobButton )
	EVENT( EV_TDM_Operate,	CFrobButton::Operate)
END_CLASS

void CFrobButton::Save(idSaveGame *savefile) const
{
	// nothing to save (yet)
}

void CFrobButton::Restore( idRestoreGame *savefile )
{
	// nothing to restore (yet)
}

void CFrobButton::Spawn()
{
}

void CFrobButton::Operate()
{
	ToggleOpen();
}

void CFrobButton::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse )
{
	// grayman #2603 - check "noimpact" flag
	if (spawnArgs.GetBool("noimpact"))
	{
		return; // button can't be hit, so do nothing
	}

	// grayman #2603 - ignore impulse from AI

	if (ent->IsType(idAI::Type))
	{
		return;
	}

	// Check if the impulse is applied in the right direction
	if (impulse * m_Translation >= 0)
	{
		Operate();
	}
}
