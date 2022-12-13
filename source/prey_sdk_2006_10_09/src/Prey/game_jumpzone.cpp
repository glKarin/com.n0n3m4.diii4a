// Game_JumpZone.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


const idEventDef EV_ResetSlopeCheck("<resetslopecheck>", "d");

CLASS_DECLARATION(hhTrigger, hhJumpZone)
	EVENT( EV_Touch,			hhJumpZone::Event_Touch )
	EVENT( EV_Enable,			hhJumpZone::Event_Enable )
	EVENT( EV_Disable,			hhJumpZone::Event_Disable )
	EVENT( EV_ResetSlopeCheck,	hhJumpZone::Event_ResetSlopeCheck )
END_CLASS

void hhJumpZone::Spawn(void) {

	velocity = spawnArgs.GetVector("velocity");
	pitchDegrees = spawnArgs.GetFloat("jumpPitch");
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
}

void hhJumpZone::Save(idSaveGame *savefile) const {
	savefile->WriteVec3( velocity );
	savefile->WriteFloat( pitchDegrees );
}

void hhJumpZone::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( velocity );
	savefile->ReadFloat( pitchDegrees );
}


// Given a pitch angle, calculate a speed to get us to destination
static float JumpBallistics( const idVec3 &start, const idVec3 &end, float pitch, float gravity ) {
/*
	speed = sqrt(
					-0.5f * gravity * ( x / cos(pitch) )^2
					-----------------------------------------
					y - x * tan(pitch)
				);
*/
	float pitchRadians = DEG2RAD(pitch);
	float speed = 0.0f;
	idVec3 toTarget = end - start;
	float dist = toTarget.Length();
	float a = dist / idMath::Cos(pitchRadians);
	float num = -0.5f * gravity * a*a;
	float den = toTarget.z - dist * idMath::Tan(pitchRadians);
	if (den != 0.0f) {
		speed = idMath::Sqrt( num / den );
	}
	return speed;
}

idVec3 hhJumpZone::CalculateJumpVelocity() {
	idVec3 destination = GetOrigin() + idVec3(0,0,200);
	if (targets.Num() == 0 || !targets[0].IsValid()) {
		// Use explicit velocity
		return velocity;
	}

	destination = targets[0]->GetOrigin();

	idVec3 toTarget = destination - GetOrigin();

	// Given the angle, calculate a speed to get us to destination
	float speed = JumpBallistics(GetOrigin(), destination, pitchDegrees, DEFAULT_GRAVITY);
	
	idAngles ang;
	ang.Set(-pitchDegrees, toTarget.ToYaw(), 0.0f);
	return ang.ToForward() * speed;
}

/*
================
hhJumpZone::Event_Enable
================
*/
void hhJumpZone::Event_Enable( void ) {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
}

/*
================
hhJumpZone::Event_Disable
================
*/
void hhJumpZone::Event_Disable( void ) {
	GetPhysics()->SetContents( 0 );
}


/*
================
hhJumpZone::Event_Touch
================
*/
void hhJumpZone::Event_Touch( idEntity *other, trace_t *trace ) {
	if (other) {
		// Enable slope checks on player in case it was turned off by gravity zone.  Need it on to
		// recognize getting thrown off the ground
		if (other->IsType( hhPlayer::Type ) && other->GetPhysics()->IsType(hhPhysics_Player::Type) ) {
			static_cast<hhPhysics_Player*>(other->GetPhysics())->SetSlopeCheck(true);
			// Post an event to turn it back off?
			other->PostEventMS(&EV_ResetSlopeCheck, 200, other->entityNumber);
		}
		other->GetPhysics()->SetLinearVelocity( CalculateJumpVelocity() );
	}
}

void hhJumpZone::Event_ResetSlopeCheck(int entNum) {
	idEntity *entityList[100];

	// If the player is still encroaching on an inward gravity zone, reset slope check (off)
	idEntity *player = gameLocal.entities[entNum];
	if (player && player->IsType(hhPlayer::Type) && player->GetPhysics()->IsType(hhPhysics_Player::Type)) {
		int num = gameLocal.clip.EntitiesTouchingBounds(player->GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER, entityList, 100);
		for (int ix=0; ix<num; ix++) {
			if (entityList[ix] && entityList[ix]->IsType(hhGravityZoneInward::Type)) {
				static_cast<hhPhysics_Player*>(player->GetPhysics())->SetSlopeCheck(false);
				break;
			}
		}
	}
}