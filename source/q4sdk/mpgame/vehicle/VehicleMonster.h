//----------------------------------------------------------------
// VehicleMonster.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLEMONSTER_H__
#define __GAME_VEHICLEMONSTER_H__

#ifndef __GAME_VEHICLE_H__
#include "Vehicle.h"
#endif

class rvVehicleAI;

class rvVehicleMonster : public rvVehicle {
	friend class rvVehicleAI;
public:

	CLASS_PROTOTYPE( rvVehicleMonster );

							rvVehicleMonster	( void );
							~rvVehicleMonster	( void );

	void					Spawn				( void );
	void					Think				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );

	bool					SkipImpulse			( idEntity* ent, int id );

protected:
	
	void					SetClipModel		( idPhysics & physicsObj );

	const idVec3 &			GetTargetOrigin		( void );
	idVec3					GetVectorToTarget	( void );
	const idVec3 &			GetEnemyOrigin		( void );
	idVec3					GetVectorToEnemy	( void );
	void					LookAtEntity		( idEntity *ent, float duration );

	idEntityPtr<rvVehicleAI>	driver;
};

#endif // __GAME_VEHICLEMONSTER_H__
