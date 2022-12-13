#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION( hhMoveable, hhBarrel )
END_CLASS

/*
================
hhBarrel::hhBarrel
================
*/
hhBarrel::hhBarrel() {
	radius = 1.0f;
	barrelAxis = 0;
	lastOrigin.Zero();
	lastAxis.Identity();
	additionalRotation = 0.0f;
	additionalAxis.Identity();
	fl.networkSync = true;
}

/*
================
hhBarrel::Save
================
*/
void hhBarrel::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( radius );
	savefile->WriteInt( barrelAxis );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteMat3( lastAxis );
	savefile->WriteFloat( additionalRotation );
	savefile->WriteMat3( additionalAxis );
}

/*
================
hhBarrel::Restore
================
*/
void hhBarrel::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( radius );
	savefile->ReadInt( barrelAxis );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadMat3( lastAxis );
	savefile->ReadFloat( additionalRotation );
	savefile->ReadMat3( additionalAxis );
}

/*
================
hhBarrel::BarrelThink
================
*/
void hhBarrel::BarrelThink( void ) {
	bool wasAtRest, onGround;
	float movedDistance, rotatedDistance, angle;
	idVec3 curOrigin, gravityNormal, dir;
	idMat3 curAxis, axis;

	wasAtRest = IsAtRest();

	// run physics
	RunPhysics();

	// only need to give the visual model an additional rotation if the physics were run
	if ( !wasAtRest ) {

		// current physics state
		onGround = GetPhysics()->HasGroundContacts();
		curOrigin = GetPhysics()->GetOrigin();
		curAxis = GetPhysics()->GetAxis();

		// if the barrel is on the ground
		if ( onGround ) {
			gravityNormal = GetPhysics()->GetGravityNormal();

			dir = curOrigin - lastOrigin;
			dir -= gravityNormal * dir * gravityNormal;
			movedDistance = dir.LengthSqr();

			// if the barrel moved and the barrel is not aligned with the gravity direction
			if ( movedDistance > 0.0f && idMath::Fabs( gravityNormal * curAxis[barrelAxis] ) < 0.7f ) {

				// barrel movement since last think frame orthogonal to the barrel axis
				movedDistance = idMath::Sqrt( movedDistance );
				dir *= 1.0f / movedDistance;
				movedDistance = ( 1.0f - idMath::Fabs( dir * curAxis[barrelAxis] ) ) * movedDistance;

				// get rotation about barrel axis since last think frame
				angle = lastAxis[(barrelAxis+1)%3] * curAxis[(barrelAxis+1)%3];
				angle = idMath::ACos( angle );
				// distance along cylinder hull
				rotatedDistance = angle * radius;

				// if the barrel moved further than it rotated about it's axis
				if ( movedDistance > rotatedDistance ) {

					// additional rotation of the visual model to make it look
					// like the barrel rolls instead of slides
					angle = 180.0f * (movedDistance - rotatedDistance) / (radius * idMath::PI);
					if ( gravityNormal.Cross( curAxis[barrelAxis] ) * dir < 0.0f ) {
						additionalRotation += angle;
					} else {
						additionalRotation -= angle;
					}
					dir = vec3_origin;
					dir[barrelAxis] = 1.0f;
					additionalAxis = idRotation( vec3_origin, dir, additionalRotation ).ToMat3();
				}
			}
		}

		// save state for next think
		lastOrigin = curOrigin;
		lastAxis = curAxis;
	}

	Present();
}

/*
================
hhBarrel::Think
================
*/
void hhBarrel::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( !FollowInitialSplinePath() ) {
			BecomeInactive( TH_THINK );
		}
	}

	BarrelThink();
}

/*
================
hhBarrel::GetPhysicsToVisualTransform
================
*/
bool hhBarrel::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = additionalAxis;
	return true;
}

/*
================
hhBarrel::Spawn
================
*/
void hhBarrel::Spawn( void ) {
	const idBounds &bounds = GetPhysics()->GetBounds();

	// radius of the barrel cylinder
	radius = ( bounds[1][0] - bounds[0][0] ) * 0.5f;

	// always a vertical barrel with cylinder axis parallel to the z-axis
	barrelAxis = 2;

	lastOrigin = GetPhysics()->GetOrigin();
	lastAxis = GetPhysics()->GetAxis();

	additionalRotation = 0.0f;
	additionalAxis.Identity();
}

/*
================
hhBarrel::ClientPredictionThink
================
*/
void hhBarrel::ClientPredictionThink( void ) {
	Think();
}


