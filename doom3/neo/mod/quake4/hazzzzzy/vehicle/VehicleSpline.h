//----------------------------------------------------------------
// VehicleSpline.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLESPLINECOUPLING_H__
#define __GAME_VEHICLESPLINECOUPLING_H__

class rvVehicleSpline : public rvVehicle {
public:
	CLASS_PROTOTYPE( rvVehicleSpline );

							rvVehicleSpline					( void );
							~rvVehicleSpline				( void );
	
	void					Spawn							( void );
	void					Save							( idSaveGame *savefile ) const;
	void					Restore							( idRestoreGame *savefile );

	void					Think							( void );

	void					Event_PostSpawn					( void );
	void					Event_SetSpline					( idEntity * spline );
	void					Event_DoneMoving				( void );

protected:
	rvPhysics_Spline		physicsObj;
	idMat3					angleOffset;
	float					idealSpeed;
	float					accelWithStrafe;
};

#endif // __GAME_VEHICLESPLINECOUPLING_H__
