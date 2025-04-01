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



#include "DoorInfo.h"

namespace ai
{

DoorInfo::DoorInfo() :
	areaNum(-1),
	//lastTimeSeen(-1), // grayman #3755 - not used
	timeCanUseAgain(0), // grayman #2345, grayman #3755
	lastTimeTriedToOpen(-1),
	wasOpen(false),
	wasLocked(false),
	wasBlocked(false)
{}

void DoorInfo::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(areaNum);
	//savefile->WriteInt(lastTimeSeen); // grayman #3755 - not used
	savefile->WriteInt(lastTimeTriedToOpen);
	savefile->WriteBool(wasOpen);
	savefile->WriteBool(wasLocked);
	savefile->WriteBool(wasBlocked);
	savefile->WriteInt(timeCanUseAgain); // grayman #2345 grayman #3755
}

void DoorInfo::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(areaNum);
	//savefile->ReadInt(lastTimeSeen); // grayman #3755 - not used
	savefile->ReadInt(lastTimeTriedToOpen);
	savefile->ReadBool(wasOpen);
	savefile->ReadBool(wasLocked);
	savefile->ReadBool(wasBlocked);
	savefile->ReadInt(timeCanUseAgain); // grayman #2345 grayman #3755
}

} // namespace ai
