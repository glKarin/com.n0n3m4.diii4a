
#ifndef __PHYSICS_VEHICLE_MONSTER_H__
#define __PHYSICS_VEHICLE_MONSTER_H__

/*
===================================================================================

	Vehicle Monster Physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.
	Extents particle physics with the ability to apply impulses.

===================================================================================
*/

class rvPhysics_VehicleMonster : public idPhysics_RigidBody {
public:
	CLASS_PROTOTYPE( rvPhysics_VehicleMonster );

	bool					Evaluate						( int timeStepMSec, int endTimeMSec );
	void					SetGravity						( const idVec3 & v );
};

#endif /* !__PHYSICS_VEHICLE_MONSTER_H__ */
