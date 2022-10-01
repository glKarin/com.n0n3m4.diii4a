//----------------------------------------------------------------
// VehicleStatic.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLESTATIC_H__
#define __GAME_VEHICLESTATIC_H__

#ifndef __GAME_VEHICLE_H__
#include "Vehicle.h"
#endif

class rvVehicleStatic : public rvVehicle
{
public:

	CLASS_PROTOTYPE( rvVehicleStatic );

							rvVehicleStatic		( void );
							~rvVehicleStatic	( void );

	void					Spawn				( void );

	virtual int				AddDriver			( int position, idActor* driver );
	virtual bool			RemoveDriver		( int position, bool force = false );

	virtual void			UpdateHUD			( idActor* driver, idUserInterface* gui );

	void					Event_ScriptedAnim	( const char* animname, int blendFrames, bool loop, bool endWithIdle );
	void 					Event_ScriptedDone	( void );
	void					Event_ScriptedStop	( void );
};

#endif // __GAME_VEHICLESTATIC_H__
