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

#ifndef __DOOR_INFO_H__
#define __DOOR_INFO_H__


namespace ai
{

struct DoorInfo
{
	// The AAS area number
	int areaNum;

	// When this door was seen the last time (-1 == never)
	//int lastTimeSeen; // grayman #3755 - not used

	// When this door can be used again by the same AI
	int timeCanUseAgain; // grayman #2345 grayman #3755

	// The last time this door was attempted to be opened (-1 == never)
	int lastTimeTriedToOpen;

	// Whether this door was open the last time it was seen
	bool wasOpen;

	// Whether this door was locked at the last open attempt
	bool wasLocked;

	// Whether this door was blocked at the last open attempt
	bool wasBlocked;

	DoorInfo();

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};
typedef std::shared_ptr<DoorInfo> DoorInfoPtr;

} // namespace ai

#endif /* __DOOR_INFO_H__ */
