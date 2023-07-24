// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __INPUT_MODE_H__
#define __INPUT_MODE_H__



class sdInputMode {
public:
	static void				ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, 
											idVec3& viewAngles, usercmd_t& cmd ) {};

protected:
	static float			ControllerClampDeadZone( float position, float threshold );
	static float			ControllerEvaluateCurve( float position, float deadZone, float offset, float power );
};

#define INPUTMODE_AXIS_DEFINE( axisName ) \
	static idCVar			joy_##axisName; \
	static idCVar			axis_##axisName; \
	static idCVar			deadZone_##axisName; \
	static idCVar			offset_##axisName; \
	static idCVar			speed_##axisName; \
	static idCVar			invert_##axisName; \
	static idCVar			power_##axisName;

class sdInputModePlayer : public sdInputMode {
public:
	static void			ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, 
											idVec3& viewAngles, usercmd_t& cmd );

	INPUTMODE_AXIS_DEFINE( pitch );
	INPUTMODE_AXIS_DEFINE( yaw );
	INPUTMODE_AXIS_DEFINE( forward );
	INPUTMODE_AXIS_DEFINE( side );
};

class sdInputModeCar : public sdInputMode {
public:
	static void			ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, 
											idVec3& viewAngles, usercmd_t& cmd );

	INPUTMODE_AXIS_DEFINE( throttle );
	INPUTMODE_AXIS_DEFINE( steering );
	INPUTMODE_AXIS_DEFINE( pitch );
	INPUTMODE_AXIS_DEFINE( yaw );
};

class sdInputModeHeli : public sdInputMode {
public:
	static void			ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, 
											idVec3& viewAngles, usercmd_t& cmd );

	INPUTMODE_AXIS_DEFINE( throttle );
	INPUTMODE_AXIS_DEFINE( yaw );
	INPUTMODE_AXIS_DEFINE( forward );
	INPUTMODE_AXIS_DEFINE( side );
};

class sdInputModeHovertank : public sdInputMode {
public:
	static void			ControllerMove( bool doGameCallback, const int controllerNum, const float* controllerAxis, 
											idVec3& viewAngles, usercmd_t& cmd );

	INPUTMODE_AXIS_DEFINE( forward );
	INPUTMODE_AXIS_DEFINE( side );
	INPUTMODE_AXIS_DEFINE( turn );
	INPUTMODE_AXIS_DEFINE( pitch );
	INPUTMODE_AXIS_DEFINE( yaw );
};

#endif	// __INPUT_MODE_H__
