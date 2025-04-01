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

CLASS_DECLARATION(idEntity, idListener)
	EVENT(EV_Activate, idListener::Event_Activate)
	EVENT(EV_PostSpawn, idListener::PostSpawn)
END_CLASS

/*
=====================
idListener::Spawn
=====================
*/
void idListener::Spawn(void)
{
	// keep track during cinematics
	cinematic = true;

	mode = spawnArgs.GetInt("mode", "1");	// 1 = hear what's at the Listener plus what's around the player
											// 2 = hear what's at the Listner only

	loss = 0;	// Volume loss through the Listener. Only affects sounds the player hears.
				// Schedule a post-spawn event to setup other spawnargs

	PostEventMS(&EV_PostSpawn, 1);
}

void idListener::PostSpawn()
{
}

/*
===============
idListener::Event_Activate
================
*/
void idListener::Event_Activate(idEntity *_activator)
{
	idPlayer* player = gameLocal.GetLocalPlayer();

	// If the current listener is this listener, turn off this listener.
	// If the current listener is NOT this listener, then switch to this listener.
	// If there's no current listener, then turn on this listener.

	idListener* currentListener = player->m_Listener.GetEntity();

	if ( currentListener )
	{
		if ( currentListener == this )
		{
			player->m_Listener = NULL; // turn off listener
			player->SetSecondaryListenerLoc(vec3_zero);
		}
		else
		{
			player->m_Listener = this; // turn on listener
		}
	}
	else // no current listener
	{
		player->m_Listener = this; // turn on listener
	}
}

/*
===============
idListener::Save
================
*/
void idListener::Save(idSaveGame *savefile) const {
	savefile->WriteInt(mode);
	savefile->WriteInt(mode);
}

/*
===============
idListener::Restore
================
*/
void idListener::Restore(idRestoreGame *savefile) {
	savefile->ReadInt(mode);
	savefile->ReadInt(loss);
}

idListener::~idListener(void)
{
}
