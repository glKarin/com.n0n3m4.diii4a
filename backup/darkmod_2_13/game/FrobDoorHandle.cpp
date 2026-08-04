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

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"

//===============================================================================
// CFrobDoorHandle
//===============================================================================
const idEventDef EV_TDM_Handle_GetDoor( "GetDoor", EventArgs(), 'e', "Returns the associated door entity for this handle." );

CLASS_DECLARATION( CFrobHandle, CFrobDoorHandle )
	EVENT( EV_TDM_Handle_GetDoor,		CFrobDoorHandle::Event_GetDoor )
END_CLASS

CFrobDoorHandle::CFrobDoorHandle() :
	m_Door(NULL)
{}

void CFrobDoorHandle::Save(idSaveGame *savefile) const
{
	savefile->WriteObject(m_Door);
}

void CFrobDoorHandle::Restore( idRestoreGame *savefile )
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Door));
}

void CFrobDoorHandle::Spawn()
{}

CFrobDoor* CFrobDoorHandle::GetDoor()
{
	return m_Door;
}

void CFrobDoorHandle::SetDoor(CFrobDoor* door)
{
	m_Door = door;

	// Set the frob master accordingly
	SetFrobMaster(m_Door);
}

void CFrobDoorHandle::Event_GetDoor()
{
	return idThread::ReturnEntity(m_Door);
}

void CFrobDoorHandle::OnOpenPositionReached()
{
	// The handle is "opened", trigger the door, but only if this is the master handle
	if (IsMasterHandle() && m_Door != NULL && !m_Door->IsOpen())
	{
		m_Door->OpenDoor(false);
	}

	// Let the handle return to its initial position
	Close(true);
}

void CFrobDoorHandle::Tap()
{
	// Invoke the base class first
	CFrobHandle::Tap();
	
	// Only the master handle is allowed to trigger sounds
	if (IsMasterHandle() && m_Door != NULL)
	{
		// Start the appropriate sound
		FrobMoverStartSound(m_Door->IsLocked() ? "snd_tap_locked" : "snd_tap_default");
	}
}

bool CFrobDoorHandle::DoorIsLocked()
{
	return m_Door ? m_Door->IsLocked() : IsLocked();
}
