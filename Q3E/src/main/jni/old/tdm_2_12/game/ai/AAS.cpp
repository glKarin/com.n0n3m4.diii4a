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



#include "AAS_local.h"


/*
============
idAAS::Alloc
============
*/
idAAS *idAAS::Alloc( void ) {
	return new idAASLocal;
}

/*
============
idAAS::idAAS
============
*/
idAAS::~idAAS( void ) {
}

/*
============
idAASLocal::idAASLocal
============
*/
idAASLocal::idAASLocal( void )
{
	elevatorSystem = new eas::tdmEAS(this);
	file = NULL;
}

/*
============
idAASLocal::~idAASLocal
============
*/
idAASLocal::~idAASLocal( void ) {
	Shutdown();

	if (elevatorSystem != NULL)
	{
		delete elevatorSystem;
	}
}

/*
============
idAASLocal::Init
============
*/
bool idAASLocal::Init( const idStr &mapName, const unsigned int mapFileCRC ) {
	// Clear the elevator system before reloading
	elevatorSystem->Clear();

	if ( file && mapName.Icmp( file->GetName() ) == 0 && mapFileCRC == file->GetCRC() ) {
		common->Printf( "Keeping %s\n", file->GetName() );
		RemoveAllObstacles();
	}
	else {
		Shutdown();

		file = AASFileManager->LoadAAS( mapName, mapFileCRC );
		if ( !file ) {
			//common->DWarning( "Couldn't load AAS file: '%s'", mapName.c_str() );
			return false;
		}
		mapName.ExtractFileExtension(name);

		SetupRouting();
	}
	return true;
}

/*
============
idAASLocal::Shutdown
============
*/
void idAASLocal::Shutdown( void ) {
	if ( file ) {
		elevatorSystem->Clear();
		ShutdownRouting();
		RemoveAllObstacles();
		AASFileManager->FreeAAS( file );
		file = NULL;
	}
}

/*
============
idAASLocal::TestIfBarrierIsolatesReachability
============
*/
bool idAASLocal::TestIfBarrierIsolatesReachability
(
	idReachability* p_reachability,
	int areaIndex,
	idBounds barrierBounds
) const
{
	/*
	* Test params
	*/
	if ( p_reachability == NULL)
	{
		return false;
	}

	// Test the paths from the reachability to all other reachabilities leaving
	// the area.  If a reachability has no path to another reachability that does not 
	// intersect the barrier, then return true. Also if there are no other reachbilities
	// return true.  Otherwise return false;

	// Iterate the other reachabilities
	bool b_hadPath = false;
	bool b_foundClearPath = false;
	idReachability* p_reach2 = GetAreaFirstReachability(areaIndex);

	while (p_reach2 != NULL)
	{
		if (p_reach2 != p_reachability)
		{
			b_hadPath = true;

			// Test if path between the reachabilities is blocked by the barrier bounds
			if (barrierBounds.LineIntersection (p_reachability->start, p_reach2->start))
			{
				// Blocked
				return true;
			}
			/*
				// Its not blocked
				b_foundClearPath = true;
			}
			*/

		} // Not same reachability

		// Is it blocked?
		if (b_foundClearPath)
		{
			// End iteration early if we already found a clear path
			p_reach2 = NULL;
		}
		else
		{
			p_reach2 = p_reach2->next;
		}
	
	} // Next other reachability on same area

	return false;

	/*

	// Return result of test
	if ( (b_hadPath) && (!b_foundClearPath) )
	{
		// Its isolated by the bounds given
		return true;
	}
	else
	{
		// Its not isolated by the bounds given
		return false;
	}
	*/

}


/*
============
idAASLocal::Stats
============
*/
void idAASLocal::Stats( void ) const {
	if ( !file ) {
		return;
	}
	common->Printf( "[%s]\n", file->GetName() );
	file->PrintInfo();
	RoutingStats();
}

/*
============
idAASLocal::GetSettings
============
*/
const idAASSettings *idAASLocal::GetSettings( void ) const {
	if ( !file ) {
		return NULL;
	}
	return &file->GetSettings();
}

/*
============
idAASLocal::PointAreaNum
============
*/
int idAASLocal::PointAreaNum( const idVec3 &origin ) const {
	if ( !file ) {
		return 0;
	}
	return file->PointAreaNum( origin );
}

/*
============
idAASLocal::PointReachableAreaNum
============
*/
int idAASLocal::PointReachableAreaNum( const idVec3 &origin, const idBounds &searchBounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}

	return file->PointReachableAreaNum( origin, searchBounds, areaFlags, TFL_INVALID );
}

/*
============
idAASLocal::BoundsReachableAreaNum
============
*/
int idAASLocal::BoundsReachableAreaNum( const idBounds &bounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}
	
	return file->BoundsReachableAreaNum( bounds, areaFlags, TFL_INVALID );
}

/*
============
idAASLocal::PushPointIntoAreaNum
============
*/
void idAASLocal::PushPointIntoAreaNum( int areaNum, idVec3 &origin ) const {
	if ( !file ) {
		return;
	}
	file->PushPointIntoAreaNum( areaNum, origin );
}

/*
============
idAASLocal::AreaCenter
============
*/
idVec3 idAASLocal::AreaCenter( int areaNum ) const {
	if ( !file ) {
		return vec3_origin;
	}
	return file->GetArea( areaNum ).center;
}

/*
============
idAASLocal::AreaFlags
============
*/
int idAASLocal::AreaFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).flags;
}

/*
============
idAASLocal::AreaTravelFlags
============
*/
int idAASLocal::AreaTravelFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).travelFlags;
}

/*
============
idAASLocal::Trace
============
*/
bool idAASLocal::Trace( aasTrace_t &trace, const idVec3 &start, const idVec3 &end ) const {
	if ( !file ) {
		trace.fraction = 0.0f;
		trace.lastAreaNum = 0;
		trace.numAreas = 0;
		return true;
	}
	return file->Trace( trace, start, end );
}

/*
============
idAASLocal::GetPlane
============
*/
const idPlane &idAASLocal::GetPlane( int planeNum ) const {
	if ( !file ) {
		static idPlane dummy;
		return dummy;
	}
	return file->GetPlane( planeNum );
}

/*
============
idAASLocal::GetEdgeVertexNumbers
============
*/
void idAASLocal::GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const {
	if ( !file ) {
		verts[0] = verts[1] = 0;
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	verts[0] = v[INTSIGNBITSET(edgeNum)];
	verts[1] = v[INTSIGNBITNOTSET(edgeNum)];
}

/*
============
idAASLocal::GetEdge
============
*/
void idAASLocal::GetEdge( int edgeNum, idVec3 &start, idVec3 &end ) const {
	if ( !file ) {
		start.Zero();
		end.Zero();
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	start = file->GetVertex( v[INTSIGNBITSET(edgeNum)] );
	end = file->GetVertex( v[INTSIGNBITNOTSET(edgeNum)] );
}


/*
**********************************************************8
Added for Darkmod by SophisticatedZombie
**********************************************************8
*/


/*
============
idAASLocal::GetAreaBounds
============
*/
idBounds idAASLocal::GetAreaBounds( int areaNum ) const {
	if ( !file ) 
	{
		assert(false);
		idBounds emptyBounds(vec3_zero);
		return emptyBounds;
	}
	return file->AreaBounds(areaNum);
}

/*
============
idAASLocal::GetNumAreas
============
*/
int	idAASLocal::GetNumAreas() const
{
	if (!file)
	{
		return -1;
	}
	else
	{
		return file->GetNumAreas();
	}

}

/*
============
idAASLocal::GetAreaFirstReachability
============
*/

idReachability* idAASLocal::GetAreaFirstReachability(int areaNum) const
{
	if ( !file ) 
	{
		return NULL;
	}
	aasArea_t area = file->GetArea (areaNum);
	return area.reach;
	
}

void idAASLocal::SetAreaTravelFlag( int index, int flag )
{
	if (file != NULL)
	{
		file->SetAreaTravelFlag(index, flag);
	}
}

void idAASLocal::RemoveAreaTravelFlag( int index, int flag )
{
	if (file != NULL)
	{
		file->RemoveAreaTravelFlag(index, flag);
	}
}

int idAASLocal::GetClusterNum(int areaNum)
{
	return file->GetArea( areaNum ).cluster;
}

void idAASLocal::ReferenceDoor(CFrobDoor* door, int areaNum)
{
	_doors[areaNum] = door;
}

void idAASLocal::DeReferenceDoor(CFrobDoor* door, int areaNum)
{
	DoorMap::iterator found = _doors.find(areaNum);

	if (found != _doors.end())
	{
		_doors.erase(found);
	}
}

CFrobDoor* idAASLocal::GetDoor(int areaNum) const
{
	DoorMap::const_iterator found = _doors.find(areaNum);
	if (found != _doors.end())
	{
		return found->second;
	}
	return NULL;
}

void idAASLocal::Save(idSaveGame* savefile) const
{
	elevatorSystem->Save(savefile);
}

void idAASLocal::Restore(idRestoreGame* savefile)
{
	elevatorSystem->Restore(savefile);
}

/*
============
idAASLocal::BuildReachbilityImpactList
============
*/

bool idAASLocal::BuildReachabilityImpactList
(
	TReachabilityTrackingList& inout_reachabilityList,
	idBounds impactBounds
) const
{

	// Start with empty list
	inout_reachabilityList.Clear();

	// For each area
	int numAreas = GetNumAreas();
	for (int areaIndex = 0; areaIndex < numAreas; areaIndex ++)
	{
		// Test this area's reachabilties
		idReachability* p_reach = GetAreaFirstReachability (areaIndex);

		while (p_reach != NULL)
		{
			// If this reachability is isolated by the impact bounds, then ad it to the list
			if (TestIfBarrierIsolatesReachability (p_reach, areaIndex, impactBounds))
			{
				inout_reachabilityList.Append (p_reach);
			}

			// Next reachability
			p_reach = p_reach->next;
		
		} // Reachability intersects given bounds


	} // Next area

	// Done
	return true;

}
