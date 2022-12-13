#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


/***********************************************************************

  hhAFEntity

***********************************************************************/

// nla - Added to allow the mappers to do huge translations w/out too much bouncing
const idEventDef EV_EnableRagdoll( "enableRagdoll", "" );
const idEventDef EV_DisableRagdoll( "disableRagdoll", "" );

CLASS_DECLARATION( idAFEntity_Generic, hhAFEntity )
	EVENT( EV_EnableRagdoll,	hhAFEntity::Event_EnableRagdoll )
	EVENT( EV_DisableRagdoll,	hhAFEntity::Event_DisableRagdoll )
END_CLASS

void hhAFEntity::Spawn( void ) {
	fl.takedamage = !spawnArgs.GetBool("noDamage");
	if (spawnArgs.FindKey("gravity")) {
		PostEventMS(&EV_ResetGravity, 100);		// Post after first think, when gravity is reset by UpdateGravity()
	}
}

void hhAFEntity::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( CheckRagdollDamage( inflictor, attacker, dir, damageDefName, location ) ) {
		return;
	}
	idAFEntity_Generic::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

//This is so ragdolls shutdown when attached to movers that are outside of the pvs.
void hhAFEntity::DormantBegin() {
	idEntity::DormantBegin();
	Event_DisableRagdoll();
}

void hhAFEntity::DormantEnd() {
	idEntity::DormantEnd();
	Event_EnableRagdoll();
}

void hhAFEntity::Event_EnableRagdoll() {
	af.GetPhysics()->Thaw();
}

void hhAFEntity::Event_DisableRagdoll() {
	af.GetPhysics()->Freeze();
}




/***********************************************************************

  hhAFEntity_WithAttachedHead

***********************************************************************/

CLASS_DECLARATION( idAFEntity_WithAttachedHead, hhAFEntity_WithAttachedHead )
	EVENT( EV_EnableRagdoll,	hhAFEntity_WithAttachedHead::Event_EnableRagdoll )
	EVENT( EV_DisableRagdoll,	hhAFEntity_WithAttachedHead::Event_DisableRagdoll )
END_CLASS

void hhAFEntity_WithAttachedHead::Spawn( void ) {
	fl.takedamage = !spawnArgs.GetBool("noDamage");
	if (spawnArgs.FindKey("gravity")) {
		PostEventMS(&EV_ResetGravity, 100);		// Post after first think, when gravity is reset by UpdateGravity()
	}
}

void hhAFEntity_WithAttachedHead::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( CheckRagdollDamage( inflictor, attacker, dir, damageDefName, location ) ) {
		return;
	}
	idAFEntity_WithAttachedHead::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

//This is so ragdolls shutdown when attached to movers that are outside of the pvs.
void hhAFEntity_WithAttachedHead::DormantBegin() {
	idEntity::DormantBegin();
	Event_DisableRagdoll();
}

void hhAFEntity_WithAttachedHead::DormantEnd() {
	idEntity::DormantEnd();
	Event_EnableRagdoll();
}

void hhAFEntity_WithAttachedHead::Event_EnableRagdoll() {
	af.GetPhysics()->Thaw();
}

void hhAFEntity_WithAttachedHead::Event_DisableRagdoll() {
	af.GetPhysics()->Freeze();
}
