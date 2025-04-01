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

#include "Entity.h"

class idListener : public idEntity
{
public:
	CLASS_PROTOTYPE(idListener);

	virtual ~idListener(void) override;

	int		mode;	// 1 (default) = hear what's at the Listener plus what's around the player
					// 2 = hear only what's at the Listener

	int		loss;	// Volume loss through the Listener. Only affects sounds the player hears.

	void	Spawn();
	void	PostSpawn();
	void	Save(idSaveGame *savefile) const;
	void	Restore(idRestoreGame *savefile);

private:
	void	Event_Activate(idEntity *activator);
};

