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



#include "../Game_local.h"

CLASS_DECLARATION( idClass, idForce )
END_CLASS

idList<idForce*> idForce::forceList;

/*
================
idForce::idForce
================
*/
idForce::idForce( void ) {
	forceList.Append( this );
}

/*
================
idForce::~idForce
================
*/
idForce::~idForce( void ) {
	forceList.Remove( this );
}

/*
================
idForce::DeletePhysics
================
*/
void idForce::DeletePhysics( const idPhysics *phys ) {
	int i;

	for ( i = 0; i < forceList.Num(); i++ ) {
		forceList[i]->RemovePhysics( phys );
	}
}

/*
================
idForce::ClearForceList
================
*/
void idForce::ClearForceList( void ) {
	forceList.Clear();
}

/*
================
idForce::Evaluate
================
*/
void idForce::Evaluate( int time ) {
}

/*
================
idForce::RemovePhysics
================
*/
void idForce::RemovePhysics( const idPhysics *phys ) {
}

/*
================
idForceApplication::Save
================
*/
void idForceApplication::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( force );
	savefile->WriteVec3( point );
}

/*
================
idForceApplication::Restore
================
*/
void idForceApplication::Restore(idRestoreGame *savefile) {
	// NOTE: we cannot reliably serialize/deserialize pointers, so we reset IDs to empty ones
	// this can probably cause duplicate application of some force for one game tic...
	// does not sound like a big trouble
	id = idForceApplicationId();
	savefile->ReadVec3( force );
	savefile->ReadVec3( point );
}

/*
================
idForceApplicationList::Add
================
*/
void idForceApplicationList::Add(const idVec3 &point, const idVec3 &force, const idForceApplicationId &id) {
	// find force application with same ID
	// if found, then replace its contents with new info
	int q = -1;
	for ( int i = 0; i < Num(); i++ )
		if ( (*this)[i].id.Same( id ) )
			q = i;

	// if not found, add new force application entry
	if ( q < 0 )
		q = Append(idForceApplication());

	// write or overwrite data
	idForceApplication &appl = (*this)[q];
	appl.point = point;
	appl.force = force;
	appl.id = id;
}

/*
================
idForceApplicationList::ComputeTotal
================
*/
void idForceApplicationList::ComputeTotal(const idVec3 &center, idVec3 *force, idVec3 *torque, idVec3 *point) const {
	idVec3 totalForce = idVec3(0);
	idVec3 totalTorque = idVec3(0);
	idVec3 lastPoint = idVec3(0);
	for ( int i = 0; i < Num(); i++ ) {
		const idForceApplication &appl = (*this)[i];
		totalForce += appl.force;
		totalTorque += ( appl.point - center ).Cross( appl.force );
		lastPoint = appl.point;	// assign last point (no sense if several forces)
	}
	if (force)
		*force = totalForce;
	if (torque)
		*torque = totalTorque;
	if (point)
		*point = lastPoint;
}

/*
================
idForceApplicationList::Save
================
*/
void idForceApplicationList::Save(idSaveGame *savefile) const {
	savefile->WriteInt( Num() );
	for ( int i = 0; i < Num(); i++ )
		(*this)[i].Save( savefile );
}

/*
================
idForceApplicationList::Restore
================
*/
void idForceApplicationList::Restore(idRestoreGame *savefile) {
	int num;
	savefile->ReadInt( num );
	SetNum( num );
	for ( int i = 0; i < num; i++ )
		(*this)[i].Restore( savefile );
}
