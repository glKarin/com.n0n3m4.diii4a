
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//#define DEBUG_VEHICLE_PHYSICS		1
#define VEHICLE_DEBUG if(g_vehicleDebug.GetInteger()) gameLocal.Printf
#if DEBUG_VEHICLE_PHYSICS
	#define  CheckSolid(a, b, c, d, e, f)	DoCheckSolid(a, b, c, d, e, f)
#else
	#define  CheckSolid(a, b, c, d, e, f)	0
#endif

//-------------------------------------------------------------
//
// CheckSolid: utility function for checking for collision errors
//
//-------------------------------------------------------------
bool DoCheckSolid(const char *text, idVec3 &pos, idClipModel *clipModel, int clipMask, idMat3 &axis, idEntity *pass) {
	int myContents = gameLocal.clip.Contents(pos, clipModel, axis, clipMask, pass);
	if( myContents ) {
		VEHICLE_DEBUG("%s\n", text);
		VEHICLE_DEBUG("  position:    %s\n", pos.ToString());
		VEHICLE_DEBUG("  orientation: %s\n", axis.ToAngles().ToString());
		VEHICLE_DEBUG("  contents:    %d\n", myContents);
		return true;
	}
	return false;
}


CLASS_DECLARATION( hhPhysics_RigidBodySimple, hhPhysics_Vehicle )
END_CLASS

//-------------------------------------------------------------
//
// hhPhysics_Vehicle::VehicleMotion
//
//-------------------------------------------------------------
bool hhPhysics_Vehicle::VehicleMotion(trace_t &collision, const idVec3 &start, const idVec3 &end, const idMat3 &axis) {
	trace_t translationalTrace;
	idVec3 move = end - start;
	idVec3 curPosition = start;
	idMat3 curAxis = axis;

	memset(&collision, 0, sizeof(collision));	// sanity

	collision.fraction = 1.0f;		// Assume success until proven otherwise

	//
	// try to slide move
	//
	for(int i = 0; i < 3; i++ ) {

		gameLocal.clip.Translation( translationalTrace, curPosition, curPosition + move, clipModel, curAxis, clipMask, self );
		curPosition = translationalTrace.endpos;

		CheckSolid(va("SlideMoved (%d) into something solid during slidemove!", i), curPosition, clipModel, clipMask, curAxis, self);

		// Keep the shortest frac
		if (translationalTrace.fraction < collision.fraction) {
			collision.fraction = translationalTrace.fraction;
			collision.c = translationalTrace.c;
		}

		if ( translationalTrace.fraction == 1.0f ) {
			break;
		}

		// Shorten the move by the amount moved
		move *= (1.0f - translationalTrace.fraction);

		// project movement and momentum onto the sliding surface
		move.ProjectOntoPlane( translationalTrace.c.normal, OVERCLIP );
	}

	collision.endAxis = curAxis;
	collision.endpos = curPosition;

	return collision.fraction < 1.0f;
}

//-------------------------------------------------------------
//
// hhPhysics_Vehicle::CheckForCollisions
//
// Evaluate the impulse based rigid body physics.
// in the event of a collision, tries to do a slide move
//-------------------------------------------------------------
bool hhPhysics_Vehicle::CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision ) {
	bool collided = false;

	CheckSolid("Before VehicleMotion: in something solid!", current.i.position, clipModel, clipMask, current.i.orientation, self);

	collided = VehicleMotion( collision, current.i.position, next.i.position, current.i.orientation );
	if( collided ) {
		// set the next state to the state at the moment of impact
		next.i.position = collision.endpos;
		next.i.orientation = collision.endAxis;
		collided = true;
	}

	CheckSolid("After VehicleMotion: in something solid!", next.i.position, clipModel, clipMask, next.i.orientation, self);

	return collided;
}

/*
================
hhPhysics_Vehicle::SetFriction
================
*/
void hhPhysics_Vehicle::SetFriction( const float linear, const float angular, const float contact ) {
	//don't cap the friction for the vehicle
	linearFriction = linear;
	angularFriction = angular;
	contactFriction = contact;
}

const float	VEH_VELOCITY_MAX				= 16000;
const int	VEH_VELOCITY_TOTAL_BITS		= 16;
const int	VEH_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( VEH_VELOCITY_MAX ) ) + 1;
const int	VEH_VELOCITY_MANTISSA_BITS	= VEH_VELOCITY_TOTAL_BITS - 1 - VEH_VELOCITY_EXPONENT_BITS;
const float	VEH_MOMENTUM_MAX				= 1e20f;
const int	VEH_MOMENTUM_TOTAL_BITS		= 16;
const int	VEH_MOMENTUM_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( VEH_MOMENTUM_MAX ) ) + 1;
const int	VEH_MOMENTUM_MANTISSA_BITS	= VEH_MOMENTUM_TOTAL_BITS - 1 - VEH_MOMENTUM_EXPONENT_BITS;
const float	VEH_FORCE_MAX				= 1e20f;
const int	VEH_FORCE_TOTAL_BITS			= 16;
const int	VEH_FORCE_EXPONENT_BITS		= idMath::BitsForInteger( idMath::BitsForFloat( VEH_FORCE_MAX ) ) + 1;
const int	VEH_FORCE_MANTISSA_BITS		= VEH_FORCE_TOTAL_BITS - 1 - VEH_FORCE_EXPONENT_BITS;

/*
================
hhPhysics_Vehicle::WriteToSnapshot
================
*/
void hhPhysics_Vehicle::WriteToSnapshot( idBitMsgDelta &msg ) const {
	idCQuat quat, localQuat;

	quat = current.i.orientation.ToCQuat();
	localQuat = current.localAxis.ToCQuat();

	msg.WriteFloat( gravityVector.x );
	msg.WriteFloat( gravityVector.y );
	msg.WriteFloat( gravityVector.z );

	msg.WriteBits(dropToFloor, 1);

	msg.WriteLong( current.atRest );
	msg.WriteFloat( current.i.position[0] );
	msg.WriteFloat( current.i.position[1] );
	msg.WriteFloat( current.i.position[2] );
	msg.WriteFloat( quat.x );
	msg.WriteFloat( quat.y );
	msg.WriteFloat( quat.z );
	msg.WriteFloat( current.i.linearMomentum[0], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[1], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[2], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[0], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[1], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[2], VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.i.position[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.i.position[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.i.position[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( quat.x, localQuat.x );
	msg.WriteDeltaFloat( quat.y, localQuat.y );
	msg.WriteDeltaFloat( quat.z, localQuat.z );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[0], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[1], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[2], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[0], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[1], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[2], VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
}

/*
================
hhPhysics_Vehicle::ReadFromSnapshot
================
*/
void hhPhysics_Vehicle::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idCQuat quat, localQuat;

	idVec3 newGrav;
	newGrav.x = msg.ReadFloat();
	newGrav.y = msg.ReadFloat();
	newGrav.z = msg.ReadFloat();

	SetGravity(newGrav);

	dropToFloor = !!msg.ReadBits(1);

	current.atRest = msg.ReadLong();
	current.i.position[0] = msg.ReadFloat();
	current.i.position[1] = msg.ReadFloat();
	current.i.position[2] = msg.ReadFloat();
	quat.x = msg.ReadFloat();
	quat.y = msg.ReadFloat();
	quat.z = msg.ReadFloat();
	current.i.linearMomentum[0] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[1] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[2] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[0] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[1] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[2] = msg.ReadFloat( VEH_MOMENTUM_EXPONENT_BITS, VEH_MOMENTUM_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.i.position[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.i.position[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.i.position[2] );
	localQuat.x = msg.ReadDeltaFloat( quat.x );
	localQuat.y = msg.ReadDeltaFloat( quat.y );
	localQuat.z = msg.ReadDeltaFloat( quat.z );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, VEH_VELOCITY_EXPONENT_BITS, VEH_VELOCITY_MANTISSA_BITS );
	current.externalForce[0] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	current.externalForce[1] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	current.externalForce[2] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	current.externalTorque[0] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	current.externalTorque[1] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );
	current.externalTorque[2] = msg.ReadDeltaFloat( 0.0f, VEH_FORCE_EXPONENT_BITS, VEH_FORCE_MANTISSA_BITS );

	current.i.orientation = quat.ToMat3();
	current.localAxis = localQuat.ToMat3();

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, clipModel->GetId(), current.i.position, current.i.orientation );
	}
}
