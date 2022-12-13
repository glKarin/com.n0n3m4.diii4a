#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/*	TODO:
	could optionally allow multiple bindcontroller bones, for coop
*/

//==========================================================================
//
//	hhRailRide
//
//==========================================================================

CLASS_DECLARATION(hhAnimated, hhRailRide)
	EVENT( EV_Activate,		hhRailRide::Event_Activate )
	EVENT( EV_BindAttach,	hhRailRide::Event_Attach )
	EVENT( EV_BindDetach,	hhRailRide::Event_Detach )
END_CLASS

void hhRailRide::Spawn() {
	fl.takedamage = spawnArgs.GetBool("dropOnPain");
	float tension = spawnArgs.GetFloat("tension", "0.01");
	float yawLimit = spawnArgs.GetFloat("yawlimit", "180");
	float pitchLimit = spawnArgs.GetFloat("pitchlimit", "0.0");
	const char *handName = spawnArgs.GetString("def_hand");
	const char *animName = spawnArgs.GetString("boundanim");

	// spawn a bind controller at railbone
	const char *bonename = spawnArgs.GetString("railbone");
	bindController = static_cast<hhBindController *>( gameLocal.SpawnObject(spawnArgs.GetString("def_bindController")) );
	assert(bindController);
	bindController->MoveToJoint(this, bonename);
	bindController->BindToJoint(this, bonename, true);
	bindController->SetTension(tension);
	bindController->SetRiderParameters(animName, handName, yawLimit, pitchLimit);
}

void hhRailRide::Save(idSaveGame *savefile) const {
	savefile->WriteObject( bindController );
}

void hhRailRide::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( bindController ) );
}

bool hhRailRide::Pain(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location) {
	bindController->Detach();

	return( true );
}

void hhRailRide::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	bindController->Detach();
}

bool hhRailRide::ValidRider(idEntity *entity) {
	return (entity && entity->spawnArgs.GetBool("RailAttachable"));
}

void hhRailRide::Attach(idEntity *rider, bool loose) {
	if (ValidRider(rider)) {
		bindController->Attach(rider, loose);
	}
}

void hhRailRide::Detach() {
	bindController->Detach();
}

void hhRailRide::Event_Activate( idEntity *activator ) {
	Attach(activator, false);
	hhAnimated::Event_Activate(activator);	// allow animation to start
}

void hhRailRide::Event_Attach(idEntity *rider, bool loose) {
	Attach(rider, loose);
}

void hhRailRide::Event_Detach() {
	Detach();
}


