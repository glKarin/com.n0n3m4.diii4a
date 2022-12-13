#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//-----------------------------------------------------------------------
//
// hhSafeResurrectionVolume
//
//-----------------------------------------------------------------------

CLASS_DECLARATION( idEntity, hhSafeResurrectionVolume )
	EVENT( EV_Enable,		hhSafeResurrectionVolume::Event_Enable )
	EVENT( EV_Disable,		hhSafeResurrectionVolume::Event_Disable )
END_CLASS

/*
===============
hhSafeResurrectionVolume::Spawn
===============
*/
void hhSafeResurrectionVolume::Spawn() {
	PostEventMS( (spawnArgs.GetBool("enabled", "1")) ? &EV_Enable : &EV_Disable, 0);
}

/*
===============
hhSafeResurrectionVolume::PickRandomPoint
===============
*/
void hhSafeResurrectionVolume::PickRandomPoint( idVec3& origin, idMat3& axis ) {
	idEntity* entity = PickRandomTarget();

	if ( !entity ) { // Error message that means something to the designers - cjr
		gameLocal.Error( "hhSafeResurrectionZone::PickRandomPoint:  No targets found within resurrection zone.\n" );
	}

	origin = entity->GetOrigin();
	axis = entity->GetAxis();
}

/*
===============
hhSafeResurrectionVolume::DetermineContents
===============
*/
int hhSafeResurrectionVolume::DetermineContents() const {
	return CONTENTS_DEATHVOLUME;
}

/*
===============
hhSafeResurrectionVolume::Event_Enable
===============
*/
void hhSafeResurrectionVolume::Event_Enable() {
	GetPhysics()->EnableClip();
	GetPhysics()->SetContents( DetermineContents() );
}

/*
===============
hhSafeResurrectionVolume::Event_Disable
===============
*/
void hhSafeResurrectionVolume::Event_Disable() {
	GetPhysics()->DisableClip();
	GetPhysics()->SetContents( 0 );
}

//-----------------------------------------------------------------------
//
// hhSafeDeathVolume
//
//-----------------------------------------------------------------------

CLASS_DECLARATION( hhSafeResurrectionVolume, hhSafeDeathVolume )
	EVENT( EV_Touch,		hhSafeDeathVolume::Event_Touch )
END_CLASS

/*
===============
hhSafeDeathVolume::Spawn
===============
*/
void hhSafeDeathVolume::Spawn() {
	GetPhysics()->SetContents( DetermineContents() );
}

/*
===============
hhSafeDeathVolume::DetermineContents
===============
*/
int hhSafeDeathVolume::DetermineContents() const {
	return CONTENTS_DEATHVOLUME | CONTENTS_TRIGGER;
}

/*
===============
hhSafeDeathVolume::IsValid
===============
*/
bool hhSafeDeathVolume::IsValid( const hhPlayer* player ) {
	if( !player || player->IsDeathWalking() || player->InVehicle() ) {
		return false;
	}

	return true;
}

/*
===============
hhSafeDeathVolume::Event_Touch
===============
*/
void hhSafeDeathVolume::Event_Touch( idEntity *other, trace_t *trace ) {
	if( !other ) {
		return;
	}

	if( !other->IsType(hhPlayer::Type) ) {
		return;
	}

	hhPlayer* player = static_cast<hhPlayer*>( other );
	if( !IsValid(player) ) {
		return;
	}

	if( player->IsSpiritOrDeathwalking() ) {
		player->StopSpiritWalk();
	} else {
		player->Kill( false, false );
	}
}