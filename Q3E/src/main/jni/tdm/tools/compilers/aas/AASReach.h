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

#ifndef __AASREACH_H__
#define __AASREACH_H__

/*
===============================================================================

	Reachabilities

===============================================================================
*/

class idAASReach {

public:
	bool					Build( const idMapFile *mapFile, idAASFileLocal *file );

private:
	const idMapFile *		mapFile;
	idAASFileLocal *		file;
	int						numReachabilities;
	bool					allowSwimReachabilities;
	bool					allowFlyReachabilities;

private:	// reachability
	void					FlagReachableAreas( idAASFileLocal *file );
	bool					ReachabilityExists( int fromAreaNum, int toAreaNum );
	bool					CanSwimInArea( int areaNum );
	bool					AreaHasFloor( int areaNum );
	bool					AreaIsClusterPortal( int areaNum );
	void					AddReachabilityToArea( idReachability *reach, int areaNum );
	void					Reachability_Fly( int areaNum );
	void					Reachability_Swim( int areaNum );
	void					Reachability_EqualFloorHeight( int areaNum );
	bool					Reachability_Step_Barrier_WaterJump_WalkOffLedge( int fromAreaNum, int toAreaNum );
	void					Reachability_WalkOffLedge( int areaNum );

};

#endif /* !__AASREACH_H__ */
