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
#include "AbsenceMarker.h"

CLASS_DECLARATION( idEntity, CAbsenceMarker )
END_CLASS

//##################################################################
// Constructor, Init
//##################################################################

CAbsenceMarker::CAbsenceMarker()
{
	ownerTeam = -1;
	referenced_entityDefNumber = -1;
	referenced_entityDefName.Clear();
	referenced_entityName.Clear();
}

//-----------------------------------------------------------------------------------

bool CAbsenceMarker::initAbsenceReference(idEntity* owner, idBounds& startBounds)
{
	if (owner == NULL)
	{
		return false;
	}
    
	referenced_entityDefNumber = owner->entityDefNumber;
	referenced_entityName = owner->name;
	referenced_entityDefName = owner->GetEntityDefName();

	GetPhysics()->SetClipBox( owner->GetPhysics()->GetBounds(), 1.0f ); // grayman #2853 - set marker's bounds to missing item's bounds

	// Fill with spawnargs of referenced entity
	referenced_spawnArgs = owner->spawnArgs;

	// grayman #3663 - pay attention to solid/non-solid spawnarg

	bool solid = spawnArgs.GetBool( "solid" );
	if ( !solid )
	{
		GetPhysics()->SetContents(0);
	}

	// Move to position where missing entity should have been
	// angua: place marker at the center of the original bounds
	idVec3 markerOrg = startBounds.GetCenter();
	SetOrigin (markerOrg);

	team = owner->team;
	
	// Done
	return true;
}


const idDict& CAbsenceMarker::GetRefSpawnargs() const
{
	return referenced_spawnArgs;
}


//-----------------------------------------------------------------------------------

void CAbsenceMarker::Save( idSaveGame *savefile ) const
{
	// Get current position and orientation of the marker
	idVec3 refPosition = GetPhysics()->GetOrigin();
	idMat3 refOrientation = GetPhysics()->GetAxis();

	savefile->WriteDict( &referenced_spawnArgs );

	// Additional fields
	savefile->WriteInt (ownerTeam);
	savefile->WriteInt (referenced_entityDefNumber);
	savefile->WriteString (referenced_entityName);
	savefile->WriteString (referenced_entityDefName);
	savefile->WriteVec3 (refPosition);
	savefile->WriteMat3 (refOrientation);
}

//-----------------------------------------------------------------------------------

void CAbsenceMarker::Restore( idRestoreGame *savefile )
{
	idVec3 refPosition;
	idMat3 refOrientation;

	savefile->ReadDict(&referenced_spawnArgs);

	// Get the referenced entity def number
	savefile->ReadInt (ownerTeam);
	savefile->ReadInt (referenced_entityDefNumber);
	savefile->ReadString (referenced_entityName);
	savefile->ReadString (referenced_entityDefName);
	savefile->ReadVec3 (refPosition);
	savefile->ReadMat3 (refOrientation);

	// Set position and orientation of marker
	SetOrigin(refPosition);
	SetAxis(refOrientation);
}
