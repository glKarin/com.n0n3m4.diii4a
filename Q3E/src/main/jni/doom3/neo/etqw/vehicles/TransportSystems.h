// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_SYSTEMS_H__
#define __GAME_VEHICLES_SYSTEMS_H__

#include "../decls/declVehicleScript.h"

class sdTransport;
class idPlayer;
class sdVehicleInput;

/*
===============================================================================

  sdTransportEngine

===============================================================================
*/

typedef struct engineSound_s {
	idInterpolate< float >			pitchLerp;
	sdInterpolateBandPass< float >	volumeLerp;
	jointHandle_t					joint;
	idStr							soundFile;
	bool							enabled;
} engineSound_t;

class sdTransportEngine {
public:
							sdTransportEngine( void );
							~sdTransportEngine( void );
	void					Init( idEntity* _parent, int _startIndex, int _max );
	void					AddSound( const engineSoundInfo_t& info );
	void					Update( bool off );
	void					Clear( void );
	bool					GetState( void ) const { return state; }

protected:
	sdTransport*			parent;
	float					previousSpeed;
	int						startIndex;
	int						max;
	idList< engineSound_t >	sounds;
	bool					state;
};

/*
===============================================================================

  sdVehicleLightSystem

===============================================================================
*/

typedef struct vehicleLightData_s {
	int							group;
	qhandle_t					lightHandle;
	jointHandle_t				joint;
	renderLight_t				renderLight;
	int							lightType;
	idVec3						offset;
	bool						on;
	bool						enabled;
} vehicleLightData_t;

class sdVehicleLightSystem {
public:
								sdVehicleLightSystem( void );
								~sdVehicleLightSystem( void );

	void						Init( sdTransport* _parent );
	void						Clear( void );
	void						UpdateLightPresentation( vehicleLightData_t& light );
	void						UpdateLight( vehicleLightData_t& light, const sdVehicleInput& input );
	bool						NightLightIsOn() const;
	void						AddLight( const vehicleLightInfo_t& info );
	void						UpdatePresentation( void );
	void						Update( const sdVehicleInput& input );
	void						SetEnabled( int group, bool enabled );

protected:
	sdTransport*					parent;
	idList< vehicleLightData_t >	lights;
	int								brakeStartTime;
	int								lastBrakeTime;
};

#endif // __GAME_VEHICLES_SYSTEMS_H__

