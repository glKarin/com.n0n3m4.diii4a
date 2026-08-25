//----------------------------------------------------------------
// VehicleAnimated.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLEANIMATED_H__
#define __GAME_VEHICLEANIMATED_H__

#include "Vehicle.h"
#include "../physics/Physics_Monster.h"

class rvVehicleAnimated : public rvVehicle {
public:

	CLASS_PROTOTYPE( rvVehicleAnimated );

							rvVehicleAnimated			( void );
							~rvVehicleAnimated			( void );

	void					Spawn						( void );
	void					Think						( void );	
	void					Save						( idSaveGame *savefile ) const;
	void					Restore						( idRestoreGame *savefile );

	virtual const idMat3&	GetAxis						( int id = 0 ) const;

	void					ClientPredictionThink		( void );
	void					WriteToSnapshot				( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot			( const idBitMsgDelta &msg );

	virtual bool			GetPhysicsToVisualTransform ( idVec3 &origin, idMat3 &axis );

protected:

	// twhitaker:
	virtual void			RunPrePhysics			( void );
	virtual void			RunPostPhysics			( void );
	idVec3					storedPosition;
	// end twhitaker:

	idPhysics_Monster		physicsObj;

	idAngles				viewAngles;	
	float					turnRate;
	idVec3					additionalDelta;
};

#endif // __GAME_VEHICLEANIMATED_H__
