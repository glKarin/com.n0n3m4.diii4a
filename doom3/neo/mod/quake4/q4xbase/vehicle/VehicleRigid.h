//----------------------------------------------------------------
// VehicleRigid.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLERIGID_H__
#define __GAME_VEHICLERIGID_H__

#ifndef __GAME_VEHICLE_H__
#include "Vehicle.h"
#endif

class rvVehicleRigid : public rvVehicle
{
public:

	CLASS_PROTOTYPE( rvVehicleRigid );

							rvVehicleRigid		( void );
							~rvVehicleRigid		( void );

	void					Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );

	void					WriteToSnapshot		( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot	( const idBitMsgDelta &msg );

	bool					SkipImpulse			( idEntity* ent, int id );

protected:
	
	void					SetClipModel	( void );
	
	// twhitaker:
	virtual void			RunPrePhysics			( void );
	virtual void			RunPostPhysics			( void );
	idVec3					storedVelocity;
	// end twhitaker:

	idPhysics_RigidBody		physicsObj;				// physics object
};

#endif // __GAME_VEHICLERIGID_H__
