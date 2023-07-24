// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "InputMode.h"

/*
===============================================================================

	sdInputMode

===============================================================================
*/

/*
=======================================
	ControllerClampDeadZone
=======================================
*/
float sdInputMode::ControllerClampDeadZone( float position, float threshold ) {
	if ( position > threshold ) {
		return ( position - threshold ) / ( 1.0f - threshold );
	} else if ( position < -threshold ) {
		return ( position + threshold ) / ( 1.0f - threshold );
	} else {
		return 0.0f;
	}
}

/*
=======================================
	ControllerEvaluateCurve
=======================================
*/
float sdInputMode::ControllerEvaluateCurve( float position, float deadZone, float offset, float power ) {
	const bool negative = position < 0.0f;
	const float x = fabs( position );

	if ( x < deadZone ) {
		return 0.0f;
	}

	const float deadZoned = ( x - deadZone ) / ( 1.0f - deadZone );
	const float powered = idMath::Pow( deadZoned, power );
	const float offseted = powered * ( 1.0f - offset ) + offset;

	if ( negative ) {
		return -offseted;
	}

	return offseted;
}

/*
=======================================
	Macros FTW!
=======================================
*/

//
// INPUTMODE_AXIS_DECLARE - declares all the parameters needed for one control axis
//
#define INPUTMODE_AXIS_DECLARE( className, modeName, axisName ) \
	idCVar className::joy_##axisName( "in_" #modeName "_" #axisName "_joy", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "the joystick number used" ); \
	idCVar className::axis_##axisName( "in_" #modeName "_" #axisName "_axis", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "which controller axis is used" ); \
	idCVar className::deadZone_##axisName( "in_" #modeName "_" #axisName "_deadZone", "0.2", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "specifies how far large the dead-zone is on the controller axis" ); \
	idCVar className::speed_##axisName( "in_" #modeName "_" #axisName "_speed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_PROFILE, "speed of the controller input" ); \
	idCVar className::invert_##axisName( "in_" #modeName "_" #axisName "_invert", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL | CVAR_PROFILE, "inverts the axis" ); \
	idCVar className::offset_##axisName( "in_" #modeName "_" #axisName "_offset", "0", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the step up the dead zone" ); \
	idCVar className::power_##axisName( "in_" #modeName "_" #axisName "_power", "1", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the power of the curve after dead zone - ie acceleration" );

//
// INPUTMODE_AXIS_DECLARE_DEF_AXIS - declares all the parameters needed for one control axis,
//									 setting up a default axis too
#define INPUTMODE_AXIS_DECLARE_DEF_AXIS( className, modeName, axisName, defaultAxis ) \
	idCVar className::joy_##axisName( "in_" #modeName "_" #axisName "_joy", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "the joystick number used" ); \
	idCVar className::axis_##axisName( "in_" #modeName "_" #axisName "_axis", #defaultAxis, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "which controller axis is used" ); \
	idCVar className::deadZone_##axisName( "in_" #modeName "_" #axisName "_deadZone", "0.2", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "specifies how far large the dead-zone is on the controller axis" ); \
	idCVar className::speed_##axisName( "in_" #modeName "_" #axisName "_speed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_PROFILE, "speed of the controller input" ); \
	idCVar className::invert_##axisName( "in_" #modeName "_" #axisName "_invert", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL | CVAR_PROFILE, "inverts the axis" ); \
	idCVar className::offset_##axisName( "in_" #modeName "_" #axisName "_offset", "0", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the step up the dead zone" ); \
	idCVar className::power_##axisName( "in_" #modeName "_" #axisName "_power", "1", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the power of the curve after dead zone - ie acceleration" );

//
// INPUTMODE_AXIS_DECLARE_DEF_INVERT - declares all the parameters needed for one control axis,
//									 setting up a default axis & invert
#define INPUTMODE_AXIS_DECLARE_DEF_INVERT( className, modeName, axisName, defaultAxis, defaultInvert ) \
	idCVar className::joy_##axisName( "in_" #modeName "_" #axisName "_joy", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "the joystick number used" ); \
	idCVar className::axis_##axisName( "in_" #modeName "_" #axisName "_axis", #defaultAxis, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "which controller axis is used" ); \
	idCVar className::deadZone_##axisName( "in_" #modeName "_" #axisName "_deadZone", "0.2", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "specifies how far large the dead-zone is on the controller axis" ); \
	idCVar className::speed_##axisName( "in_" #modeName "_" #axisName "_speed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_PROFILE, "speed of the controller input" ); \
	idCVar className::invert_##axisName( "in_" #modeName "_" #axisName "_invert", #defaultInvert, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL | CVAR_PROFILE, "inverts the axis" ); \
	idCVar className::offset_##axisName( "in_" #modeName "_" #axisName "_offset", "0", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the step up the dead zone" ); \
	idCVar className::power_##axisName( "in_" #modeName "_" #axisName "_power", "1", CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the power of the curve after dead zone - ie acceleration" );


//
// INPUTMODE_AXIS_DECLARE_DEFAULTS - declares all the parameters needed for one control axis,
//									 setting up defaults for all values
#define INPUTMODE_AXIS_DECLARE_DEFAULTS( className, modeName, axisName, defaultAxis, defaultDeadZone, defaultSpeed, defaultInvert, defaultOffset, defaultPower ) \
	idCVar className::joy_##axisName( "in_" #modeName "_" #axisName "_joy", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "the joystick number used" ); \
	idCVar className::axis_##axisName( "in_" #modeName "_" #axisName "_axis", #defaultAxis, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER | CVAR_PROFILE, "which controller axis is used" ); \
	idCVar className::deadZone_##axisName( "in_" #modeName "_" #axisName "_deadZone", #defaultDeadZone, CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "specifies how far large the dead-zone is on the controller axis" ); \
	idCVar className::speed_##axisName( "in_" #modeName "_" #axisName "_speed", #defaultSpeed, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_PROFILE, "speed of the controller input" ); \
	idCVar className::invert_##axisName( "in_" #modeName "_" #axisName "_invert", #defaultInvert, CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL | CVAR_PROFILE, "inverts the axis" ); \
	idCVar className::offset_##axisName( "in_" #modeName "_" #axisName "_offset", #defaultOffset, CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the step up the dead zone" ); \
	idCVar className::power_##axisName( "in_" #modeName "_" #axisName "_power", #defaultPower, CVAR_SYSTEM | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "the power of the curve after dead zone - ie acceleration" );

//
// INPUTMODE_AXIS_EVALUATE - Evaulates an axis that has been defined using the above macros
//
#define INPUTMODE_AXIS_EVALUATE( className, axisName, axes ) \
	float axisName = 0.0f; \
	{ \
		int joystickNumber = className::joy_##axisName.GetInteger(); \
		if ( joystickNumber == controllerNum ) {\
			const int axisNumber = className::axis_##axisName.GetInteger(); \
			if ( axisNumber != -1 ) { \
				const float deadZone = className::deadZone_##axisName.GetFloat(); \
				const float speed = className::speed_##axisName.GetFloat(); \
				const float offset = className::offset_##axisName.GetFloat(); \
				const float power = className::power_##axisName.GetFloat(); \
				const bool invert = className::invert_##axisName.GetBool(); \
				axisName = invert ? -axes[ axisNumber ] : axes[ axisNumber ]; \
				axisName = ControllerEvaluateCurve( axisName, deadZone, offset, power ); \
				axisName *= speed; \
			} \
		} \
	}

/*
===============================================================================

	sdInputModePlayer

===============================================================================
*/

/*
=======================================
	CVars
=======================================
*/

INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModePlayer, player, pitch , 3, 0.2, 230, 1, 0, 4 );
INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModePlayer, player, yaw, 2, 0.2, 230, 1, 0, 4 );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModePlayer, player, forward, 1, 1 );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModePlayer, player,  side, 0, 0 );

/*
=======================================
	ControllerMove
=======================================
*/
void sdInputModePlayer::ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	INPUTMODE_AXIS_EVALUATE( sdInputModePlayer, pitch, controllerAxis );	
	INPUTMODE_AXIS_EVALUATE( sdInputModePlayer, forward, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModePlayer, yaw, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModePlayer, side, controllerAxis );

	float	angleSpeedYaw;
	float	angleSpeedPitch;
	angleSpeedYaw = angleSpeedPitch = MS2SEC( USERCMD_MSEC );

	if ( doGameCallback ) {
		gameLocal.GetSensitivity( angleSpeedYaw, angleSpeedPitch );
	}

	yaw *= angleSpeedYaw;
	pitch *= angleSpeedPitch;

	viewAngles[ ROLL ] = 0.0f;
	viewAngles[ YAW ] += yaw;
	viewAngles[ PITCH ] += pitch;

	if ( forward || side ) {
//		cmd.buttons.btn.run = true;
		cmd.rightmove = idMath::ClampChar( cmd.rightmove + side );
		cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + forward );
	}
}

/*
===============================================================================

	sdInputModeCar

===============================================================================
*/

/*
=======================================
	CVars
=======================================
*/

INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeCar, car, throttle, 1, 1  );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeCar, car, steering, 0, 0 );
INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModeCar, car, pitch , 3, 0.2, 230, 1, 0, 4 );
INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModeCar, car, yaw, 2, 0.2, 230, 1, 0, 4 );

/*
=======================================
	ControllerMove
=======================================
*/
void sdInputModeCar::ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	INPUTMODE_AXIS_EVALUATE( sdInputModeCar, throttle, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeCar, steering, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeCar, pitch, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeCar, yaw, controllerAxis );

	float	angleSpeedYaw;
	float	angleSpeedPitch;
	angleSpeedYaw = angleSpeedPitch = MS2SEC( USERCMD_MSEC );

	if ( doGameCallback ) {
		gameLocal.GetSensitivity( angleSpeedYaw, angleSpeedPitch );
	}

	yaw *= angleSpeedYaw;
	pitch *= angleSpeedPitch;

	viewAngles[ ROLL ] = 0.0f;
	viewAngles[ YAW ] += yaw;
	viewAngles[ PITCH ] += pitch;

	if ( throttle || steering ) {
	//	cmd.buttons.btn.run = true;
		cmd.rightmove = idMath::ClampChar( steering );
		cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + throttle );
	}
}


/*
===============================================================================

	sdInputModeHeli

===============================================================================
*/

/*
=======================================
	CVars
=======================================
*/

INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHeli, heli, throttle, 1, 1  );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHeli, heli, yaw, 0, 0 );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHeli, heli, forward, 3, 0  );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHeli, heli, side, 2, 1 );

/*
=======================================
	ControllerMove
=======================================
*/
void sdInputModeHeli::ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	INPUTMODE_AXIS_EVALUATE( sdInputModeHeli, throttle, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHeli, yaw, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHeli, forward, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHeli, side, controllerAxis );

	float	angleSpeedYaw;
	float	angleSpeedPitch;
	angleSpeedYaw = angleSpeedPitch = MS2SEC( USERCMD_MSEC );

	if ( doGameCallback ) {
		gameLocal.GetSensitivity( angleSpeedYaw, angleSpeedPitch );
	}

	side *= angleSpeedYaw;
	forward *= angleSpeedPitch;

	viewAngles[ ROLL ] = 0.0f;
	viewAngles[ YAW ] += side;
	viewAngles[ PITCH ] += forward;

	if ( throttle || yaw ) {
		cmd.buttons.btn.run = true;
		cmd.rightmove = idMath::ClampChar( cmd.rightmove + yaw );
		cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + throttle );
	}
}


/*
===============================================================================

	sdInputModeHovertank

===============================================================================
*/

/*
=======================================
	CVars
=======================================
*/

INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHovertank, hovertank, forward, 1, 1  );
INPUTMODE_AXIS_DECLARE_DEF_INVERT( sdInputModeHovertank, hovertank, turn, 0, 0 );
INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModeHovertank, hovertank, pitch , 3, 0.2, 230, 1, 0, 4 );
INPUTMODE_AXIS_DECLARE_DEFAULTS( sdInputModeHovertank, hovertank, yaw, 2, 0.2, 230, 1, 0, 4 );
INPUTMODE_AXIS_DECLARE_DEF_AXIS( sdInputModeHovertank, hovertank, side, -1 );


/*
=======================================
	ControllerMove
=======================================
*/
void sdInputModeHovertank::ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	INPUTMODE_AXIS_EVALUATE( sdInputModeHovertank, forward, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHovertank, side, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHovertank, turn, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHovertank, pitch, controllerAxis );
	INPUTMODE_AXIS_EVALUATE( sdInputModeHovertank, yaw, controllerAxis );

	float	angleSpeedYaw;
	float	angleSpeedPitch;
	angleSpeedYaw = angleSpeedPitch = MS2SEC( USERCMD_MSEC );

	if ( doGameCallback ) {
		gameLocal.GetSensitivity( angleSpeedYaw, angleSpeedPitch );
	}

	yaw *= angleSpeedYaw;
	pitch *= angleSpeedPitch;

	viewAngles[ ROLL ] = 0.0f;
	viewAngles[ YAW ] += yaw;
	viewAngles[ PITCH ] += pitch;

	if ( forward || turn ) {
		cmd.buttons.btn.run = true;
		cmd.rightmove = idMath::ClampChar( cmd.rightmove + turn );
		cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + forward );
	}

	cmd.buttons.btn.leanLeft |= side < 0.0f;
	cmd.buttons.btn.leanRight |= side > 0.0f;
}

