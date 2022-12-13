#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------------------------
//
// hhDock
//
//-----------------------------------------------------------------------
const idEventDef EV_DockLock("lockVehicleInDock", NULL);
const idEventDef EV_DockUnlock("unlockDock", NULL);

ABSTRACT_DECLARATION(idEntity, hhDock)
	EVENT( EV_DockLock,					hhDock::Event_Lock)
	EVENT( EV_DockUnlock,				hhDock::Event_Unlock)
	EVENT( EV_Activate,					hhDock::Event_Activate)
	EVENT( EV_PostSpawn,				hhDock::Event_PostSpawn )
END_CLASS

void hhDock::Spawn() {
	if ( !gameLocal.isClient ) {
		PostEventMS(&EV_PostSpawn, 0);
	}
}

void hhDock::Event_PostSpawn() {
	dockingZone = SpawnDockingZone();
}

void hhDock::Save(idSaveGame *savefile) const {
	dockingZone.Save(savefile);
}

void hhDock::Restore( idRestoreGame *savefile ) {
	dockingZone.Restore(savefile);
}

hhDockingZone *hhDock::SpawnDockingZone() {
	hhDockingZone *zone;
	idBounds localBounds;

	// Define our bounds
	localBounds[0] = spawnArgs.GetVector("dockingzonemins");
	localBounds[1] = spawnArgs.GetVector("dockingzonemaxs");

	// create a docking zone with this size
	idDict args;
	args.SetVector( "origin", GetOrigin() );
	args.SetVector( "mins", localBounds[0] );
	args.SetVector( "maxs", localBounds[1] );
	args.SetMatrix( "rotation", GetAxis() );
	zone = (hhDockingZone *)gameLocal.SpawnEntityType(hhDockingZone::Type, &args);
	assert(zone);
	zone->Bind(this, true);
	zone->RegisterDock(this);
	return zone;
}

void hhDock::Event_Lock() {
	Lock();
}

void hhDock::Event_Unlock() {
	Unlock();
}

void hhDock::Event_Activate( idEntity *activator ) {
	Unlock();
}