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

#ifndef __AI_EAS_ELEVATOR_STATION_INFO_H__
#define __AI_EAS_ELEVATOR_STATION_INFO_H__

#include <list>
#include "RouteNode.h"
#include "../../Game_local.h"

namespace eas {

struct ElevatorStationInfo
{
	idEntityPtr<CMultiStateMover> elevator;					// The elevator this station is belonging to
	idEntityPtr<CMultiStateMoverPosition> elevatorPosition;	// The elevator position entity
	int areaNum;											// The area number of this elevator station
	int clusterNum;											// The cluster number of this elevator station
	int elevatorNum;										// The elevator number this position is belonging to

	ElevatorStationInfo() :
		areaNum(-1),
		clusterNum(-1),
		elevatorNum(-1)
	{
		elevator = NULL;
		elevatorPosition = NULL;
	}

	void Save(idSaveGame* savefile) const
	{
		elevator.Save(savefile);
		elevatorPosition.Save(savefile);

		savefile->WriteInt(areaNum);
		savefile->WriteInt(clusterNum);
		savefile->WriteInt(elevatorNum);
	}

	void Restore(idRestoreGame* savefile)
	{
		elevator.Restore(savefile);
		elevatorPosition.Restore(savefile);

		savefile->ReadInt(areaNum);
		savefile->ReadInt(clusterNum);
		savefile->ReadInt(elevatorNum);
	}
};
typedef std::shared_ptr<ElevatorStationInfo> ElevatorStationInfoPtr;
typedef std::list<ElevatorStationInfoPtr> ElevatorStationInfoList;

} // namespace eas

#endif /* __AI_EAS_ELEVATOR_STATION_INFO_H__ */
