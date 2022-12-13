
#ifndef __HH_PHYSICS_VEHICLE_H__
#define __HH_PHYSICS_VEHICLE_H__

/*
===================================================================================

	hhPhysics_Vehicle

	Simulates the motion of a vehicle through the environment. The orientation of
	the vehicle is controlled directly by the player, so the physics move the
	vehicle so that it will fit at it's current orientation.

===================================================================================
*/

class hhPhysics_Vehicle : public hhPhysics_RigidBodySimple {

public:
	CLASS_PROTOTYPE( hhPhysics_Vehicle );

	virtual void			SetFriction( const float linear, const float angular, const float contact );

	const idVec3&			GetLinearMomentum() const { return current.i.linearMomentum; }
	const idVec3&			GetAngularMomentum() const { return current.i.angularMomentum; }

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	virtual bool			CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision );

	bool					VehicleMotion( trace_t &collision, const idVec3 &start, const idVec3 &end, const idMat3 &axis );
};


#endif 