// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VehicleControl.h"
#include "Transport.h"
#include "TransportComponents.h"
#include "Vehicle_RigidBody.h"
#include "../ContentMask.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../../decllib/DeclSurfaceType.h"
#include "../InputMode.h"
#include "../rules/GameRules.h"

idCVar g_vehicleSteerKeyScale( "g_vehicleSteerKeyScale",	"1",	CVAR_GAME | CVAR_ARCHIVE | CVAR_FLOAT | CVAR_PROFILE, "The scale of the wheeled vehicle steering keys - 1 is standard, 2 is twice as fast, etc" );

/*
===============================================================================

	sdVehicleControlBase

===============================================================================
*/

/*
================
sdVehicleControlBase::Init
================
*/
void sdVehicleControlBase::Init( sdTransport* transport ) {
	owner = transport;
}


/*
===============================================================================

	sdVehicleScriptControl

===============================================================================
*/


/*
================
sdVehicleScriptControl::~sdVehicleScriptControl
================
*/
sdVehicleScriptControl::~sdVehicleScriptControl() {
	if ( inputThread != NULL ) {
		gameLocal.program->FreeThread( inputThread );
		inputThread = NULL;
	}
}

/*
================
sdVehicleScriptControl::Init
================
*/
void sdVehicleScriptControl::Init( sdTransport* transport ) {
	sdVehicleControlBase::Init( transport );

	inputThreadFunc = transport->GetScriptObject()->GetFunction( "InputThread" );
	if ( inputThreadFunc ) {
		inputThread = gameLocal.program->CreateThread();
		inputThread->ManualControl();
		inputThread->ManualDelete();
	}

	inputMode = 0;

	idStr modeString;
	transport->spawnArgs.GetString( "input_mode", "", modeString );
	if ( !modeString.Icmp( "car" ) ) {
		inputMode = 1;
	} else if ( !modeString.Icmp( "helicopter" ) ) {
		inputMode = 2;
	} else if ( !modeString.Icmp( "hovertank" ) ) {
		inputMode = 3;
	}
}

/*
================
sdVehicleScriptControl::Update
================
*/
void sdVehicleScriptControl::Update() {
	if ( inputThread ) {
		inputThread->CallFunction( owner->GetScriptObject(), inputThreadFunc );
		inputThread->Execute();
	}
}

/*
================
sdVehicleScriptControl::OnKeyMove
================
*/
bool sdVehicleScriptControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	return false;
}

/*
================
sdVehicleScriptControl::OnControllerMove
================
*/
void sdVehicleScriptControl::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
								 const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	// run the input for each controller
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];

		switch ( inputMode ) {
			case 1:			// car
				sdInputModeCar::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
				break;
			case 2:			// helicopter
				sdInputModeHeli::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
				break;
			case 3:			// hovertank
				sdInputModeHovertank::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
				break;
			default:
			case 0:			// player
				sdInputModePlayer::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
				break;
		}
	}
}

/*
===============================================================================

	sdVehicleControl

===============================================================================
*/

/*
================
sdVehicleControl::Init
================
*/
void sdVehicleControl::Init( sdTransport* transport ) {
	sdVehicleControlBase::Init( transport );

	state = CS_SHUTDOWN;
	newState = CS_SHUTDOWN;
	newStateTime = 0;

	overDriveSoundEndTime		= 0;		// 0 -> not playing
	overDrivePitchStart			= transport->spawnArgs.GetFloat( "overdrive_pitch_low", "1" );
	overDrivePitchEnd			= transport->spawnArgs.GetFloat( "overdrive_pitch_high", "1" );
	overDrivePitchStartSpeed	= transport->spawnArgs.GetFloat( "overdrive_speed_low", "0" );
	overDrivePitchEndSpeed		= transport->spawnArgs.GetFloat( "overdrive_speed_high", "0" );

	drownHeight = transport->spawnArgs.GetFloat( "drown_height", "0.5" );
}

/*
================
sdVehicleControl::Update
================
*/
void sdVehicleControl::Update() {
	assert( owner );
	assert( input );

	SetupInput();
	RunStateMachine();
	HandlePhysics();
}

/*
================
sdVehicleControl::HandlePhysics
================
*/
void sdVehicleControl::HandlePhysics() {
	owner->UpdateEngine( !EngineRunning() );
}

/*
================
sdVehicleControl::RunStateMachine
================
*/
void sdVehicleControl::RunStateMachine() {

	if( newStateTime ) {
		if( gameLocal.time > newStateTime ) {
			state = newState;
			newStateTime = 0;

			HandleStateChange( newState );
		} else {
			return;
		}
	}

	// only gets here if there is no state change to do
	assert( !newStateTime );
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if ( state == CS_POWERED ) {
		if ( !driver || ( owner->GetHealth() <= 0 ) || owner->IsEMPed() ) {
//			owner->StartSound( "snd_shutdown", SND_BODY, 0, NULL );
			newStateTime = gameLocal.time + 500;
			newState = CS_SHUTDOWN;
		}

	} else if ( state == CS_SHUTDOWN ) {
		if ( driver && ( owner->GetHealth() > 0 ) && !owner->IsEMPed() ) {
//			owner->StartSound( "snd_startup", SND_BODY, 0, NULL );
			newStateTime = gameLocal.time + 500;
			newState = CS_POWERED;
		}
	}
}

/*
================
sdVehicleControl::HandleStateChange
================
*/
void sdVehicleControl::HandleStateChange( controlState_t s ) {
	if( s == CS_SHUTDOWN ) {
		owner->GetPhysics()->SetComeToRest( true );
		owner->SetLightsEnabled( 0, false );
	} else if( s == CS_POWERED ) {
		owner->SetLightsEnabled( 0, true );
	}
}

/*
================
sdVehicleControl::EngineRunning
================
*/
bool sdVehicleControl::EngineRunning() {
	if ( owner->IsAmphibious() ) {
		if ( owner->IsFlipped() ) {
			return false;
		}
	} else if ( owner->GetPhysics()->InWater() > drownHeight ) {
		return false;
	}

	return ( state == CS_POWERED ) && ( newState == CS_POWERED ) && !owner->IsEMPed() && !isImmobilized;
}

/*
================
sdVehicleControl::UpdateOverdriveSound
================
*/
void sdVehicleControl::UpdateOverdriveSound( bool thrusting ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( thrusting ) {
		if ( gameLocal.isNewFrame ) {
			if ( overDriveSoundEndTime >= 0 && gameLocal.time > overDriveSoundEndTime ) {
				// freshly start the sound
				overDriveSoundEndTime = -1;
				owner->StartSound( "snd_overdrive", SND_VEHICLE_OVERDRIVE, 0, NULL );
				// fade sound in from nothing
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, -60.0f, 0.0f );
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, 0.0f, 0.01f );
			}
			if ( gameLocal.time < overDriveSoundEndTime ) {
				// fade sound in
				overDriveSoundEndTime = -1;
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, 0.0f, 0.01f );
			}
		}
		if( driver != NULL && driver == gameLocal.GetLocalPlayer() ) {
			gameLocal.SetGUIFloat( GUI_GLOBALS_HANDLE, "vehicles.overDriveFraction", 1.0f );
		}		
	} else {
		if ( gameLocal.isNewFrame ) {
			if ( overDriveSoundEndTime < 0 ) {
				owner->StartSound( "snd_overdrive_stop", SND_VEHICLE_DRIVE5, 0, NULL );
				owner->FadeSound( SND_VEHICLE_DRIVE5, -60.0f, 1.0f );
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, -60.0f, 1.0f );
				overDriveSoundEndTime = gameLocal.time + 300;
			}
		}
		if( driver != NULL && driver == gameLocal.GetLocalPlayer() ) {
			gameLocal.SetGUIFloat( GUI_GLOBALS_HANDLE, "vehicles.overDriveFraction", 0.5f );
		}		
	}

	if ( overDrivePitchEndSpeed != overDrivePitchStartSpeed && ( overDriveSoundEndTime == -1 || gameLocal.time < overDriveSoundEndTime ) ) {
		float speed = idMath::Fabs( owner->GetPhysics()->GetLinearVelocity() * owner->GetPhysics()->GetAxis()[ 0 ] );
		speed = UPSToKPH( speed );
		float pitch = idMath::ClampFloat( 0.0f, 1.0f, ( speed - overDrivePitchStartSpeed ) / ( overDrivePitchEndSpeed - overDrivePitchStartSpeed ) ); 
		pitch = idMath::Sqrt( pitch ) * ( overDrivePitchEnd - overDrivePitchStart ) + overDrivePitchStart;
		owner->SetChannelPitchShift( SND_VEHICLE_OVERDRIVE, pitch );
	}
}

/*
===============================================================================

	sdDesecratorControl

===============================================================================
*/


/*
================
sdDesecratorControl::Init
================
*/
void sdDesecratorControl::Init( sdTransport* transport ) {
	sdVehicleControl::Init( transport );

	wantsSiegeMode = true;
	inSiegeMode = false;

	lastGroundEffectsTime = 0;
}

/*
================
sdDesecratorControl::SetupInput
================
*/
void sdDesecratorControl::SetupInput() {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	bool braking = false;
	idVec3 directions = vec3_origin;
	float strafing = 0.0f;

	input->Clear();

	if ( driver != NULL && EngineRunning() ) {
		input->SetPlayer( driver );
		directions.Set( input->GetForward(), input->GetRight(), input->GetUp() );
		if ( driver->usercmd.buttons.btn.leanLeft ) {
			strafing -= 1.0f;
		}
		if ( driver->usercmd.buttons.btn.leanRight ) {
			strafing += 1.0f;
		}

		if ( directions.z > 0.0f && wantsSiegeMode ) {
			wantsSiegeMode = false;
		} else if ( directions.z < 0.0f && !wantsSiegeMode ) {
			wantsSiegeMode = true;
		}

		input->SetCollective( strafing );
		input->SetSteerAngle( directions.y );
	} else {
		braking = true;
	}

	bool handbraking = wantsSiegeMode;


	bool canThrust = false;
	if ( driver != NULL && wantsSiegeMode == false ) {
		if ( fabs( directions.x ) > 0.0f || strafing != 0.0f ) {
			canThrust = driver->usercmd.buttons.btn.sprint;
		}
	}

	input->SetForce( 1.0f );

	bool thrusting = canThrust;
	if ( thrusting && owner->IsInPlayzone() ) {
		// increase the force multiplier
		input->SetForce( 1.3f );
	}

	UpdateOverdriveSound( thrusting );

	input->SetBraking( braking );
	input->SetHandBraking( handbraking );
}

/*
================
sdDesecratorControl::OnPlayerEntered
================
*/
void sdDesecratorControl::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( driver && driver == player ) {
		wantsSiegeMode = false;
	}
	if ( oldPosition == 0 && position != 0 ) {
		wantsSiegeMode = true;
	}
}

/*
================
sdDesecratorControl::OnPlayerExited
================
*/
void sdDesecratorControl::OnPlayerExited( idPlayer* player, int position ) {
	// it was the driver that left
	if ( position == 0 ) {
		wantsSiegeMode = true;
	}
}

/*
================
sdDesecratorControl::OnKeyMove
================
*/
bool sdDesecratorControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	return false;
}

/*
================
sdDesecratorControl::OnControllerMove
================
*/
void sdDesecratorControl::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
								 const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {

	// run the input for each controller
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];
		sdInputModeHovertank::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
	}
}

void sdDesecratorControl::UpdateEffects() {
	idVec3 absMins = owner->GetPhysics()->GetAbsBounds()[ 0 ];
	idVec3 absMaxs = owner->GetPhysics()->GetAbsBounds()[ 1 ];
	idVec3 traceOrg = ( absMins + absMaxs ) * 0.5f;
	idVec3 traceEnd = traceOrg;
	traceEnd.z -= 400.0f;

	trace_t		traceObject;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS traceObject, traceOrg, traceEnd, MASK_SOLID | CONTENTS_WATER | MASK_OPAQUE, owner );
	traceEnd = traceObject.endpos;
#if 0
	if ( traceObject.fraction < 1.0f ) {
		height = traceOrg.z - traceEnd.z;
	} else {
		height = -1.0f;
	}
#endif

	bool isEmpty = owner->GetPositionManager().IsEmpty();

	if ( gameLocal.time >= ( lastGroundEffectsTime + 200 ) && !isEmpty
		&& /*groundEffects && */traceObject.fraction < 1.0f ) {

		const char* surfaceTypeName = NULL;
		if ( traceObject.c.surfaceType ) {
			surfaceTypeName = traceObject.c.surfaceType->GetName();
		}

		owner->PlayEffect( "fx_groundeffect", colorWhite.ToVec3(), surfaceTypeName, traceObject.endpos, traceObject.c.normal.ToMat3(), 0 );
		lastGroundEffectsTime = gameLocal.time;
	}
}

void sdDesecratorControl::RunStateMachine() {

	controlState_t oldNewState = newState;
	sdVehicleControl::RunStateMachine();
	if ( gameLocal.isNewFrame ) {
		UpdateEffects();
	}

	if ( state == CS_POWERED && newState == CS_SHUTDOWN && oldNewState != CS_SHUTDOWN ) {
		owner->StartSound( "snd_shutdown", SND_BODY, 0, NULL );
	} 
	
	if ( state == CS_SHUTDOWN && newState == CS_POWERED && oldNewState != CS_POWERED  ) {
		owner->StartSound( "snd_startup", SND_BODY, 0, NULL );
	}
}

/*
===============================================================================

	sdWheeledVehicleControl

===============================================================================
*/

/*
================
sdWheeledVehicleControl::Init
================
*/
void sdWheeledVehicleControl::Init( sdTransport* transport ) {
	sdVehicleControl::Init( transport );

	oldSpeedKPH			= 0.0f;
	nextBrakeSoundTime	= 0;

	const char* tableName = transport->spawnArgs.GetString( "table_gearforces" );
	tableName = transport->spawnArgs.GetString( va( "table_gearforces%s", gameLocal.rules->GetKeySuffix() ), tableName );
	gearForceTable		= gameLocal.declTableType[ tableName ];
	if ( !gearForceTable ) {
		gameLocal.Error( "sdWheeledVehicleControl::Init - gear force table \'%s\' not found", tableName );
	}

	tableName = transport->spawnArgs.GetString( "table_gearspeeds" );
	tableName = transport->spawnArgs.GetString( va( "table_gearspeeds%s", gameLocal.rules->GetKeySuffix() ), tableName );
	gearSpeedTable		= gameLocal.declTableType[ tableName ];
	if ( !gearSpeedTable ) {
		gameLocal.Error( "sdWheeledVehicleControl::Init - gear speed table \'%s\' not found", tableName );
	}

	powerCurveScale		= transport->spawnArgs.GetFloat( "power_curve_scale", "0" );

	overDriveFactor		= transport->spawnArgs.GetFloat( "overdrive_factor" );

	steeringAngle			= transport->spawnArgs.GetFloat( "steering_angle" );

	forwardSteerSpeed			= transport->spawnArgs.GetFloat( "simplesteer_forward_speed", "2.0" );
	reverseSteerSpeed			= transport->spawnArgs.GetFloat( "simplesteer_reverse_speed", "-4.0" );
	minSteerCenteringSpeed		= transport->spawnArgs.GetFloat( "simplesteer_centering_speed_min", "2.0" );
	maxSteerCenteringSpeed		= transport->spawnArgs.GetFloat( "simplesteer_centering_speed_max", "15.0" );
	airSteerCenteringSpeed		= transport->spawnArgs.GetFloat( "simplesteer_centering_speed_air", "0.5" );
	maxSteerCenteringThreshold	= transport->spawnArgs.GetFloat( "simplesteer_centering_ramp_threshold", "20.0" );
	reverseSteerAngleScale		= transport->spawnArgs.GetFloat( "simplesteer_reverse_angle_scale", "-0.5" );

	keySteering				= false;

	desiredDirection		= 0.0f;
}

/*
================
sdWheeledVehicleControl::OnPlayerEntered
================
*/
void sdWheeledVehicleControl::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	StopCareening();

	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( driver && driver == player ) {
		desiredDirection = owner->GetPhysics()->GetAxis().ToAngles().yaw;
	}
}

/*
================
sdWheeledVehicleControl::OnTeleport
================
*/
void sdWheeledVehicleControl::OnTeleport( void ) {
	desiredDirection = owner->GetPhysics()->GetAxis().ToAngles().yaw;
}

/*
================
sdWheeledVehicleControl::SelectGear
================
*/
float sdWheeledVehicleControl::SelectGear( idVec3& directions, float absSpeedKPH ) {
	float min = gearSpeedTable->GetMinValue() * 0.5f;	// halving this allows the vehicle to get out of first gear
	float max = gearSpeedTable->GetMaxValue();

	float gearTableFraction;
	if ( absSpeedKPH < min ) {
		gearTableFraction = 0.f;
	} else if ( absSpeedKPH > max ) {
		gearTableFraction = 1.f;
	} else {
		gearTableFraction = ( absSpeedKPH - min ) / ( max - min );
	}

	return gearTableFraction * ( gearForceTable->NumValues() - 2 );
}

/*
================
sdWheeledVehicleControl::OnKeyMove
================
*/
bool sdWheeledVehicleControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {

	const sdVehicleInput& input = owner->GetInput();
	idPlayer* player = input.GetPlayer();
	if ( player != NULL ) {
		cmd.forwardmove = forward;
		cmd.rightmove = right;
		cmd.upmove = up;

		if ( right != 0 ) {
			keySteering = true;
		}
		return true;
	}

	return false;
}

/*
================
sdWheeledVehicleControl::OnControllerMove
================
*/
void sdWheeledVehicleControl::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
								 const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	
	char oldRightMove = cmd.rightmove;

	// run the input for each controller
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];
		sdInputModeCar::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
	}

	if ( keySteering ) {
		// if the keys are being pressed it overrides the controller
		cmd.rightmove = oldRightMove;
	}
}

/*
================
sdWheeledVehicleControl::UpdateControls
================
*/
void sdWheeledVehicleControl::UpdateControls() {
}

/*
================
sdWheeledVehicleControl::SetupInput
================
*/
void sdWheeledVehicleControl::SetupInput() {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	input->Clear();

	idVec3 vel = owner->GetPhysics()->GetLinearVelocity();

	float speedKPH = InchesToMetres( vel * owner->GetPhysics()->GetAxis()[ 0 ] ) * 3.6f;
	float absSpeedKPH = fabs( speedKPH );

	idVec3 directions = vec3_origin;
	bool braking = false;
	bool handbraking = false;
	float gearForce = 0.0f;
	float gearSpeed = 0.0f;

	if ( driver && EngineRunning() ) {
		input->SetPlayer( driver );
		directions.Set( input->GetForward(), input->GetRight(), input->GetUp() );

		UpdateCareening( directions );
		UpdateDirectionBraking( directions, vel, speedKPH, braking );
		UpdateHandbrake( directions, speedKPH, handbraking, braking );		
		UpdateControls();

		float fixedGear = SelectGear( directions, absSpeedKPH );
		// limit reverse to first gear only
		if ( directions.x < 0.0f && speedKPH < 0.0f && fixedGear > 0.5f) {
			fixedGear = 0.5f;
		}

		int intGear = fixedGear;
		float leftover = fixedGear - intGear;

		gearForce = gearForceTable->TableLookup( intGear );
		gearSpeed = gearSpeedTable->TableLookup( fixedGear );

		// adjust the gear force in a kind of torque curve
		gearForce = gearForce * ( 1.0f + leftover * powerCurveScale );

		float steerAngle = UpdateSteering( driver, directions, speedKPH, absSpeedKPH, handbraking );
		input->SetSteerAngle( steerAngle );
	} else {
		HandleEmpty( vel, absSpeedKPH, braking );
	}

	UpdateBrakingSound( speedKPH, braking );

	SetSpeed( directions, MetresToInches( gearSpeed ) / 3.6f, gearForce, braking, handbraking );

	input->SetBraking( braking );
	input->SetHandBraking( handbraking );
}

/*
================
sdWheeledVehicleControl::UpdateSteering
================
*/
float sdWheeledVehicleControl::UpdateSteering( idPlayer* driver, idVec3& directions, float speedKPH, float absSpeedKPH, bool handbraking ) {
	idAngles currentAngles = owner->GetPhysics()->GetAxis().ToAngles();
	if ( driver == NULL ) {
		idAngles currentAngles = owner->GetPhysics()->GetAxis().ToAngles();
		desiredDirection = currentAngles.yaw;
		return directions.y * steeringAngle;
	}


	// auto-steering
	if ( !handbraking ) {
		bool reversing = speedKPH < 0.0f;
		if ( !reversing ) {
			desiredDirection -= forwardSteerSpeed * directions.y * steeringAngle * MS2SEC( gameLocal.msec );
		} else {
			desiredDirection -= reverseSteerSpeed * directions.y * steeringAngle * MS2SEC( gameLocal.msec );
		}

		float yawDiff = idMath::AngleNormalize180( desiredDirection - currentAngles.yaw );

		if ( yawDiff > steeringAngle ) {
			desiredDirection = currentAngles.yaw + steeringAngle;
			yawDiff = steeringAngle;
		} else if ( yawDiff < -steeringAngle ) {
			desiredDirection = currentAngles.yaw - steeringAngle;
			yawDiff = -steeringAngle;
		} else {
			// drift the desired towards the current
			float minDriftSpeed = minSteerCenteringSpeed;
			float driftSpeed = minDriftSpeed;
			if ( !owner->GetPhysics()->HasGroundContacts() ) {
				// if in the air don't drift very fast, otherwise you can lose
				// your heading a bit fast when going over jumps badly
				driftSpeed = airSteerCenteringSpeed;
			} else {
				if ( absSpeedKPH < maxSteerCenteringThreshold ) {
					float maxDriftSpeed = 0.5f / MS2SEC( gameLocal.msec );
					maxDriftSpeed = Min( 0.5f / MS2SEC( gameLocal.msec ), maxSteerCenteringSpeed );
					driftSpeed = Lerp( maxDriftSpeed, minDriftSpeed, absSpeedKPH / maxSteerCenteringThreshold );
				}
			}
			desiredDirection -= yawDiff * driftSpeed * MS2SEC( gameLocal.msec );
		}

		if ( !reversing ) {
			return -yawDiff;
		} else {
			return -yawDiff * reverseSteerAngleScale;
		}
	} else {
		desiredDirection = currentAngles.yaw - directions.y * steeringAngle;
		return directions.y * steeringAngle;
	}
}

/*
================
sdWheeledVehicleControl::StartCareening
================
*/
void sdWheeledVehicleControl::StartCareening() {
	if ( !gameLocal.isClient ) {
		owner->SetCareening( gameLocal.time );
	}
}

/*
================
sdWheeledVehicleControl::StopCareening
================
*/
void sdWheeledVehicleControl::StopCareening() {
	if ( !gameLocal.isClient ) {
		owner->SetCareening( 0 );
	}
}

/*
================
sdWheeledVehicleControl::AutoBrake
================
*/
bool sdWheeledVehicleControl::AutoBrake() {
	idVec3 upAxis = owner->GetPhysics()->GetAxis()[ 2 ];
	return upAxis.z < 0.95f;
}

/*
================
sdWheeledVehicleControl::CanEmptyBrake
================
*/
bool sdWheeledVehicleControl::CanEmptyBrake( float speedKPH ) {
	if ( owner->IsCareening() ) {
		if ( speedKPH < 2.0f ) {
			StopCareening();
		} else {
			float t = MS2SEC( owner->GetCareeningTime() );

			// could expose these parameters, but they're ok here for now.
			float value = fmod( t * 6.0f, 4.0f );
			if ( value != 3.0f ) {
				return false;
			}
		}
	}

	return true;
}

/*
================
sdWheeledVehicleControl::UpdateCareening
================
*/
void sdWheeledVehicleControl::UpdateCareening( idVec3& directions ) {
	if ( directions.x > 0.0f || directions.x < 0.0f ) {
		StartCareening();
	} else {
		StopCareening();
	}
}

/*
================
sdWheeledVehicleControl::UpdateDirectionBraking
================
*/
void sdWheeledVehicleControl::UpdateDirectionBraking( idVec3& directions, idVec3& vel, float speedKPH, bool& braking ) {
	float absSpeedKPH = fabs( speedKPH );
	if ( directions.x && ( absSpeedKPH > 2.0f ) ) {
		if ( directions.x < 0.0f ) {
			braking = speedKPH > 0.0f;
		} else {
			braking = speedKPH < 0.0f;
		}
	} else if ( !directions.x ) {
		if ( absSpeedKPH < 10.0f ) {
			braking = true;
		} else if ( AutoBrake() && vel.z < 0.0f ) {
			braking = true;
		}
	}
}

/*
================
sdWheeledVehicleControl::UpdateHandbrake
================
*/
void sdWheeledVehicleControl::UpdateHandbrake( idVec3& directions, float speedKPH, bool& handbraking, bool& braking ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();
	if ( driver == NULL ) {
		return;
	}

	bool wantHandbrake = false;
	if ( !driver->usercmd.buttons.btn.run ) {
		wantHandbrake = true;
	}

	if ( driver->usercmd.buttons.btn.leanLeft ) {
		wantHandbrake = true;
		directions.y = -1.0f;
		directions.z = -1.0f;
	} else if ( driver->usercmd.buttons.btn.leanRight ) {
		wantHandbrake = true;
		directions.y = 1.0f;
		directions.z = -1.0f;
	}

	if ( driver->usercmd.upmove > 0 ) {
		wantHandbrake = true;
	}

	if ( wantHandbrake ) {
		if ( fabs( speedKPH ) > 5.0f ) {
			handbraking = true;
		} else {
			braking = true;
		}
	}
}

/*
================
sdWheeledVehicleControl::HandleEmpty
================
*/
void sdWheeledVehicleControl::HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking ) {
	if ( CanEmptyBrake( absSpeedKPH ) ) {
		if ( absSpeedKPH < 80.0f ) {
			braking = true;
		} else if ( AutoBrake() && vel.z < 0.0f ) {
			braking = true;
		}
	}
}

/*
================
sdWheeledVehicleControl::CanThrust
================
*/
bool sdWheeledVehicleControl::CanThrust( idVec3& directions, bool braking, bool handbraking ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if ( driver ) {
		if ( directions.x > 0 ) {
			return driver->usercmd.buttons.btn.sprint && !braking && !handbraking;
		}
	}

	return false;
}

/*
================
sdWheeledVehicleControl::SetSpeed
================
*/
void sdWheeledVehicleControl::SetSpeed( idVec3& directions, float gearSpeed, float gearForce, bool braking, bool handbraking ) {
	// do the thrusters
	bool canThrust = CanThrust( directions, braking, handbraking );
	bool thrusters = canThrust;
	if ( thrusters && owner->IsInPlayzone() ) {
		gearForce *= overDriveFactor;
		gearSpeed *= overDriveFactor;
	}

	UpdateOverdriveSound( thrusters );

	// set it up
	input->SetForce( gearForce * fabs( directions.x ) );
	float speed = gearSpeed * directions.x;
	input->SetLeftSpeed( speed );
	input->SetRightSpeed( speed );
}


/*
================
sdWheeledVehicleControl::UpdateBrakingSound
================
*/
void sdWheeledVehicleControl::UpdateBrakingSound( float speedKPH, bool braking ) {
	if( braking ) {
		if( oldSpeedKPH > 50.0f && speedKPH < 50.0f ) {
			if( nextBrakeSoundTime < gameLocal.time ) {
				int time;
				owner->StartSound( "snd_brake", SND_VEHICLE_BRAKE, 0, &time );
				nextBrakeSoundTime = gameLocal.time + time;
			}
		}
	}

	oldSpeedKPH = speedKPH;
}

/*
================
sdWheeledVehicleControl::GetHudSpeed
================
*/
float sdWheeledVehicleControl::GetHudSpeed( void ) const {
	// wheeled vehicles only read the forward speed (ie from the rotation of the wheels)
	idVec3 vel = owner->GetPhysics()->GetLinearVelocity();
	return idMath::Fabs( vel * owner->GetAxis()[ 0 ] );
}

/*
================
sdWheeledVehicleControl::CreateNetworkStructure
================
*/
sdEntityStateNetworkData*	sdWheeledVehicleControl::CreateNetworkStructure( networkStateMode_t mode ) const { 
	if ( mode == NSM_VISIBLE ) {
		return new sdWheeledControlNetworkData;
	}

	return NULL;
}

/*
================
sdWheeledVehicleControl::CheckNetworkStateChanges
================
*/
bool sdWheeledVehicleControl::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdWheeledControlNetworkData );

		NET_CHECK_FIELD( desiredDirection, desiredDirection );
		if ( input != NULL && baseData.steerVisualAngle != owner->GetSteerVisualAngle() ) {
			return true;
		}

		return false;
	}

	return false;
}

/*
================
sdWheeledVehicleControl::WriteNetworkState
================
*/
void sdWheeledVehicleControl::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdWheeledControlNetworkData );

		newData.desiredDirection = desiredDirection;
		newData.steerVisualAngle = owner->GetSteerVisualAngle();

		msg.WriteDeltaFloat( baseData.desiredDirection, newData.desiredDirection );
		msg.WriteDeltaFloat( baseData.steerVisualAngle, newData.steerVisualAngle );
		return;
	}
}

/*
================
sdWheeledVehicleControl::ReadNetworkState
================
*/
void sdWheeledVehicleControl::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdWheeledControlNetworkData );

		newData.desiredDirection	= msg.ReadDeltaFloat( baseData.desiredDirection );		
		newData.steerVisualAngle	= msg.ReadDeltaFloat( baseData.steerVisualAngle );		
		return;
	}
}

/*
================
sdWheeledVehicleControl::ApplyNetworkState
================
*/
void sdWheeledVehicleControl::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdWheeledControlNetworkData );

		desiredDirection = newData.desiredDirection;
		owner->SetSteerVisualAngle( newData.steerVisualAngle );
		return;
	}
}

/*
================
sdWheeledVehicleControl::ResetNetworkState
================
*/
void sdWheeledVehicleControl::ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	ApplyNetworkState( mode, newState );
}

/*
================
sdWheeledControlNetworkData::MakeDefault
================
*/
void sdWheeledControlNetworkData::MakeDefault( void ) {
	desiredDirection = 0.0f;
	steerVisualAngle = 0.0f;
}

/*
================
sdWheeledControlNetworkData::Write
================
*/
void sdWheeledControlNetworkData::Write( idFile* file ) const {
	file->WriteFloat( desiredDirection );
	file->WriteFloat( steerVisualAngle );
}

/*
================
sdWheeledControlNetworkData::Read
================
*/
void sdWheeledControlNetworkData::Read( idFile* file ) {
	file->ReadFloat( desiredDirection );
	file->ReadFloat( steerVisualAngle );
}

/*
===============================================================================

	sdTitanControl

===============================================================================
*/

/*
================
sdTitanControl::OnKeyMove
================
*/
bool sdTitanControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	return false;
}

/*
================
sdTitanControl::UpdateCareening
================
*/
void sdTitanControl::UpdateCareening( idVec3& directions ) {
}

/*
================
sdTitanControl::UpdateDirectionBraking
================
*/
void sdTitanControl::UpdateDirectionBraking( idVec3& directions, idVec3& vel, float speedKPH, bool& braking ) {
	float absSpeedKPH = fabs( speedKPH );
	if ( directions.x && ( absSpeedKPH > 5.f ) ) {
		if ( directions.x < 0 ) {
			braking = speedKPH > 0;
		} else {
			braking = speedKPH < 0;
		}
	} else if ( directions.x == 0 && directions.y == 0 ) {
		braking = true;
	}
}

/*
================
sdTitanControl::SelectGear
================
*/
float sdTitanControl::SelectGear( idVec3& directions, float absSpeedKPH ) {
	if ( directions.x != 0.0f ) {
		return sdWheeledVehicleControl::SelectGear( directions, absSpeedKPH );
	} else {
		return 0.0f;
	}
}

/*
================
sdTitanControl::UpdateHandbrake
================
*/
void sdTitanControl::UpdateHandbrake( idVec3& directions, float speedKPH, bool& handbraking, bool& braking ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if ( driver != NULL && !driver->usercmd.buttons.btn.run ) {
		if ( fabs( speedKPH ) > 20.0f ) {
			handbraking = true;
		} else {
			braking = true;
		}
	}
}

/*
================
sdTitanControl::HandleEmpty
================
*/
void sdTitanControl::HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking ) {
	braking = true;
}

/*
================
sdTitanControl::UpdateSteering
================
*/
float sdTitanControl::UpdateSteering( idPlayer* driver, idVec3& directions, float speedKPH, float absSpeedKPH, bool handbraking ) {
//	idMat3 desiredMat;
//	idAngles::YawToMat3( desiredDirection, desiredMat );
//	gameRenderWorld->DebugArrow( colorRed, owner->GetPhysics()->GetOrigin(), owner->GetPhysics()->GetOrigin() + desiredMat[ 0 ] * 256.0f, 16 );

	idAngles currentAngles = owner->GetPhysics()->GetAxis().ToAngles();

	// auto-steering
	bool reversing = speedKPH < 0.0f && directions.x < 0.0f;
	if ( !reversing ) {
		desiredDirection -= forwardSteerSpeed * directions.y * steeringAngle * MS2SEC( gameLocal.msec );
	} else {
		desiredDirection -= reverseSteerSpeed * directions.y * steeringAngle * MS2SEC( gameLocal.msec );
	}

	float yawDiff = idMath::AngleNormalize180( desiredDirection - currentAngles.yaw );

	if ( yawDiff > steeringAngle ) {
		desiredDirection = currentAngles.yaw + steeringAngle;
		yawDiff = steeringAngle;
	} else if ( yawDiff < -steeringAngle ) {
		desiredDirection = currentAngles.yaw - steeringAngle;
		yawDiff = -steeringAngle;
	} else {
		// drift the desired towards the current
		float driftSpeed = minSteerCenteringSpeed;
		if ( !owner->GetPhysics()->HasGroundContacts() ) {
			// if in the air don't drift very fast, otherwise you can lose
			// your heading a bit fast when going over jumps badly
			driftSpeed = airSteerCenteringSpeed;
		}
		desiredDirection -= yawDiff * driftSpeed * MS2SEC( gameLocal.msec );
	}

	if ( !reversing ) {
		return -yawDiff;
	} else {
		return -yawDiff * reverseSteerAngleScale;
	}
}


/*
================
sdTitanControl::SetSpeed
================
*/
void sdTitanControl::SetSpeed( idVec3& directions, float gearSpeed, float gearForce, bool braking, bool handbraking ) {
	// do the thrusters
	bool canThrust = CanThrust( directions, braking, handbraking );
	bool thrusters = canThrust;
	if ( thrusters && owner->IsInPlayzone() ) {
		gearSpeed *= overDriveFactor;
	}

	UpdateOverdriveSound( thrusters );


	idPlayer* driver = owner->GetPositionManager().FindDriver();

	gearForce /= overDriveFactor;

	idVec3 angVelocity = owner->GetPhysics()->GetAngularVelocity();
	float yawVelocity = angVelocity * owner->GetPhysics()->GetAxis()[ 2 ];

	float angleToSteer = owner->GetInput().GetSteerAngle();
	angleToSteer += yawVelocity * 5.0f;

	float forwardMove = directions.x;
	float sideMove = angleToSteer / steeringAngle;
	sideMove = idMath::Fabs( sideMove ) < 0.01f ? 0.0f : sideMove;

	if ( sideMove > 0.0f ) {
		sideMove = idMath::Pow( sideMove, 0.25f );
	} else {
		sideMove = -idMath::Pow( -sideMove, 0.25f );
	}

	// yaw speed cancelling
	float leftSpeed = 0.0f;
	float rightSpeed = 0.0f;
	float force = gearForce * ( fabs( forwardMove ) + fabs( sideMove ) );
//	if ( driver ) {
		if( forwardMove != 0.0f ) {
			gearSpeed = gearSpeed * forwardMove;

			if( sideMove > 0.0f ) {
				if ( !thrusters ) {
					force *= 2.0f;
				}
				leftSpeed = gearSpeed * 1.0f;
				rightSpeed = gearSpeed * ( 1.0f - sideMove * 0.5f );
			} else if( directions.y < 0.0f ) {
				if ( !thrusters ) {
					force *= 2.0f;
				}
				leftSpeed = gearSpeed * ( 1.0f + sideMove * 0.5f );
				rightSpeed = gearSpeed * 1.0f;
			} else {
				leftSpeed = gearSpeed;
				rightSpeed = gearSpeed;
			}
		} else {
			if ( !thrusters ) {
				force *= 4.0f;
			}
			leftSpeed = gearSpeed * sideMove * 2.0f;
			rightSpeed = gearSpeed * sideMove * -2.0f;
		}
//	}
	
	input->SetLeftSpeed( leftSpeed );
	input->SetRightSpeed( rightSpeed );
	input->SetForce( force );
}

/*
===============================================================================

	sdTrojanControl

===============================================================================
*/

/*
================
sdTrojanControl::Init
================
*/
void sdTrojanControl::Init( sdTransport* transport ) {
	sdWheeledVehicleControl::Init( transport );

	leftProp.object		= NULL;
	leftProp.angle		= 0.f;
	leftProp.speed		= 0.f;
	leftProp.maxSpeed	= 50.f;
	leftProp.joint		= transport->GetAnimator()->GetJointHandle( transport->spawnArgs.GetString( "joint_left_thruster" ) );

	rightProp.object	= NULL;
	rightProp.angle		= 0.f;
	rightProp.speed		= 0.f;
	rightProp.maxSpeed	= 50.f;
	rightProp.joint		= transport->GetAnimator()->GetJointHandle( transport->spawnArgs.GetString( "joint_right_thruster" ) );
}

/*
================
sdTrojanControl::UpdateCareening
================
*/
void sdTrojanControl::UpdateCareening( idVec3& directions ) {
}

/*
================
sdTrojanControl::UpdateControls
================
*/
void sdTrojanControl::UpdateControls() {
	sdWheeledVehicleControl::UpdateControls();

	float leftThrust = input->GetForward();
	float rightThrust = input->GetForward();
	
	if ( input->GetRight() > 0.0f ) {
		leftThrust *= 1.75f;
		rightThrust *= -0.25f;
	} else if ( input->GetRight() < 0.0f ) {
		leftThrust *= -0.25f;
		rightThrust *= 1.75f;
	}

	if ( leftProp.object != NULL && rightProp.object != NULL ) {
		leftProp.object->SetThrust( leftThrust );
		rightProp.object->SetThrust( rightThrust );
	}
}

/*
================
sdTrojanControl::UpdatePropeller
================
*/
void sdTrojanControl::UpdatePropeller( propeller_t& prop ) {
	if ( prop.joint == INVALID_JOINT ) {
		return;
	}

	if ( prop.object != NULL ) {
		float thrust = prop.object->GetThrust();
		if ( !prop.object->IsInWater() || thrust == 0.f ) {
			if ( prop.speed < 0.f ) {
				prop.speed = prop.speed + 5.f;
				if ( prop.speed > 0.f ) {
					prop.speed = 0.f;
				}
			} else if ( prop.speed > 0.f ) {
				prop.speed = prop.speed - 5.f;
				if ( prop.speed < 0.f ) {
					prop.speed = 0.f;
				}
			}
		} else {
			if ( thrust < 0.f ) {
				prop.speed = prop.speed - 5.f;
				if ( prop.speed < -prop.maxSpeed ) {
					prop.speed = -prop.maxSpeed;
				}
			} else {
				prop.speed = prop.speed + 5.f;
				if ( prop.speed > prop.maxSpeed ) {
					prop.speed = prop.maxSpeed;
				}
			}
		}

		if ( prop.speed != 0.f ) {
			prop.angle += prop.speed;

			idMat3 temp;
			idAngles::RollToMat3( prop.angle, temp );
			prop.object->GetParent()->GetAnimator()->SetJointAxis( prop.joint, JOINTMOD_LOCAL, temp );
		}
	}
}

/*
================
sdTrojanControl::Update
================
*/
void sdTrojanControl::Update() {
	// this needs to go here since the drive objects don't exist yet in Init
	if ( !leftProp.object || !rightProp.object ) {
		leftProp.object = owner->GetDriveObject( "left_thruster" )->Cast< sdVehicleThruster >();
		if ( !leftProp.object ) {
			gameLocal.Error( "sdTrojanControl::Init - \'left_thruster\' drive object does not exist" );
		}

		rightProp.object = owner->GetDriveObject( "right_thruster" )->Cast< sdVehicleThruster >();
		if ( !rightProp.object ) {
			gameLocal.Error( "sdTrojanControl::Init - \'right_thruster\' drive object does not exist" );
		}	
	}

	sdWheeledVehicleControl::Update();

	if ( gameLocal.DoClientSideStuff() && gameLocal.isNewFrame ) {
		UpdatePropeller( leftProp );
		UpdatePropeller( rightProp );
	}
}

/*
================
sdTrojanControl::HandleEmpty
================
*/
void sdTrojanControl::HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking ) {
	if ( CanEmptyBrake( absSpeedKPH ) ) {
		if ( absSpeedKPH < 48.0f ) {
			braking = true;
		} else if ( AutoBrake() && vel.z < 0.0f ) {
			braking = true;
		}

		if ( leftProp.object != NULL && rightProp.object != NULL ) {
			leftProp.object->SetThrust( 0.0f );
			rightProp.object->SetThrust( 0.0f );
		}
	}
}

/*
===============================================================================

	sdPlatypusControl

===============================================================================
*/

/*
================
sdPlatypusControl::Init
================
*/
void sdPlatypusControl::Init( sdTransport* transport ) {
	sdTrojanControl::Init( transport );

	thrustScale = owner->spawnArgs.GetFloat( "thrust_scale" );

	steeringSpeedScale		= transport->spawnArgs.GetFloat( "steering_speed_scale" );
	steeringSpeedMax		= transport->spawnArgs.GetFloat( "steering_speed_max" );
	steeringSpeedMin		= transport->spawnArgs.GetFloat( "steering_speed_min" );
	steeringReturnFactor	= transport->spawnArgs.GetFloat( "steering_return_factor" );
	steeringRampPower		= transport->spawnArgs.GetFloat( "steering_ramp_power", "1" );
	steeringRampOffset		= transport->spawnArgs.GetFloat( "steering_ramp_offset", "0" );
}

/*
================
sdPlatypusControl::CalculateSteering
================
*/
void sdPlatypusControl::CalculateSteering( idVec3& directions, float absSpeedKPH, float& desiredSteerAngle, float& steeringFactor ) {
	const sdVehicleInput& input = owner->GetInput();
	float keySteerAngle = input.GetSteerAngle();

	float steerValue;
	if ( directions.y != 0.0f && directions.y * keySteerAngle >= 0.0f ) {
		steerValue = steeringAngle;
		steeringFactor = fabs( 1.0f - ( absSpeedKPH / steeringSpeedScale ) );
	} else {
		steeringFactor = steeringReturnFactor;
		steerValue = 0.0f;
	}

	steeringFactor = steeringSpeedMax * steeringFactor;

	if ( steeringFactor < steeringSpeedMin ) {
		steeringFactor = steeringSpeedMin;
	}

	// apply further factors
	desiredSteerAngle = steerValue * directions.y;
	ApplySteeringMods( desiredSteerAngle, steeringFactor );
}

/*
================
sdPlatypusControl::ApplySteeringMods
================
*/
void sdPlatypusControl::ApplySteeringMods( float& desiredSteerAngle, float& steeringFactor ) {
	float angleDiff = idMath::AngleNormalize180( desiredSteerAngle - input->GetSteerAngle() );
	float steerRateRampFactor = angleDiff / ( steeringAngle * 2.0f );
	if ( steerRateRampFactor < 0.0f ) {
		steerRateRampFactor = -steerRateRampFactor;
	}

	steerRateRampFactor  = idMath::ClampFloat( 0.0f, 1.0f, steerRateRampFactor );
	steerRateRampFactor = idMath::Pow( steerRateRampFactor, steeringRampPower ) * ( 1.0f - steeringRampOffset ) + steeringRampOffset;
	steeringFactor *= steerRateRampFactor;
}

/*
================
sdPlatypusControl::SetupInput
================
*/
void sdPlatypusControl::SetupInput() {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	input->Clear();

	idVec3 vel = owner->GetPhysics()->GetLinearVelocity();

	float speedKPH = InchesToMetres( vel * owner->GetPhysics()->GetAxis()[ 0 ] ) * 3.6f;
	float absSpeedKPH = fabs( speedKPH );

	idVec3 directions = vec3_origin;

	bool thrusters = false;
	if ( driver && EngineRunning() ) {
		input->SetPlayer( driver );
		directions.Set( input->GetForward(), input->GetRight(), input->GetUp() );

		bool canThrust = false;
		if ( directions.x > 0.0f ) {
			canThrust = driver->usercmd.buttons.btn.sprint;
		}
		thrusters = canThrust;

		float thrust = directions.x;
		if ( thrusters ) {
			thrust *= thrustScale;
			if ( !owner->IsInPlayzone() ) {
				thrust *= 0.5f;
			}
		}
		if ( directions.x < 0.0f ) {
			thrust *= 0.25f;
		}

		if ( leftProp.object != NULL && rightProp.object != NULL ) {
			leftProp.object->SetThrust( thrust );
			rightProp.object->SetThrust( thrust );
		}

		if ( driver->usercmd.buttons.btn.leanLeft ) {
			directions.y = -1.0f;
		} else if ( driver->usercmd.buttons.btn.leanRight ) {
			directions.y = 1.0f;
		}

		float desiredSteerAngle;
		float steeringForce;
		CalculateSteering( directions, absSpeedKPH, desiredSteerAngle, steeringForce );
		input->SetSteerSpeed( steeringForce );
		input->SetSteerAngle( desiredSteerAngle );
	} else {
		if ( leftProp.object != NULL && rightProp.object != NULL ) {
			leftProp.object->SetThrust( 0.0f );
			rightProp.object->SetThrust( 0.0f );
		}
	}

	UpdateOverdriveSound( thrusters );
}


/*
===============================================================================

	sdHogControl

===============================================================================
*/

/*
================
sdHogControl::Init
================
*/
void sdHogControl::Init( sdTransport* transport ) {
	sdWheeledVehicleControl::Init( transport );

	ramming = false;
	ramDamageScale = owner->spawnArgs.GetFloat( "ram_damage_scale" );
	hitDamageScale = owner->spawnArgs.GetFloat( "hit_damage_scale" );
}

/*
================
sdHogControl::Update
================
*/
void sdHogControl::Update() {
	sdWheeledVehicleControl::Update();
	
	if ( ramming ) {
		owner->SetDamageDealtScale( ramDamageScale );
	} else {
		owner->SetDamageDealtScale( hitDamageScale );
	}

	if ( ramModel.IsValid() ) {
		if ( owner->IsHidden() ) {
			ramModel->Dispose();
			ramModel = NULL;
		} else {
			renderEntity_t *re = ramModel->GetRenderEntity();
			re->suppressSurfaceInViewID = owner->GetRenderEntity()->suppressSurfaceInViewID;
			re->suppressShadowInViewID = owner->GetRenderEntity()->suppressShadowInViewID;
			re->shaderParms[ 5 ] = owner->GetPhysics()->GetLinearVelocity().Length();
		}

		// in-cockpit shader
		idPlayer* driver = owner->GetPositionManager().FindDriver();
		if ( driver == gameLocal.GetLocalViewPlayer() ) {
			sdClientAnimated* cockpit = gameLocal.playerView.GetCockpit();
			if ( cockpit != NULL ) {
				cockpit->GetRenderEntity()->shaderParms[ 5 ] = owner->GetPhysics()->GetLinearVelocity().Length();
			}
		}
	}
}

/*
================
sdHogControl::CanThrust
================
*/
bool sdHogControl::CanThrust( idVec3& directions, bool braking, bool handbraking ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	bool canThrust = false;
	if ( driver != NULL ) {
		canThrust = driver->usercmd.buttons.btn.sprint && !braking && !handbraking && directions.x > 0.0f;
	}

	ramming = canThrust;

	if ( canThrust && !owner->IsHidden() ) {
		if ( !ramModel.IsValid() ) {
			const idDeclEntityDef* def = gameLocal.declEntityDefType[ owner->spawnArgs.GetString( "def_ram" ) ];
			if ( def != NULL ) {
				ramModel = new sdClientAnimated();
				ramModel->Create( &def->dict, gameLocal.program->GetDefaultType() );
				ramModel->Bind( owner );
			}
		}
	} else {
		if ( ramModel.IsValid() ) {
			ramModel->Dispose();
			ramModel = NULL;
		}
	}

	return canThrust;
}

/*
================
sdHogControl::IgnoreCollisionDamage
================
*/
bool sdHogControl::IgnoreCollisionDamage( const idVec3& direction ) const {
	if ( !ramming ) {
		return false;
	}

	float frontNess = direction * owner->GetPhysics()->GetAxis()[ 0 ];
	if ( frontNess > 0.7f ) {
		return true;
	}
	return false;
}


/*
===============================================================================

	sdHuskyControl

===============================================================================
*/

/*
================
sdHuskyControl::Init
================
*/
void sdHuskyControl::Init( sdTransport* transport ) {
	sdWheeledVehicleControl::Init( transport );

	handleBars = owner->GetAnimator()->GetJointHandle( owner->spawnArgs.GetString( "joint_steer" ) );
}

/*
================
sdHuskyControl::Update
================
*/
void sdHuskyControl::Update() {
	sdWheeledVehicleControl::Update();

	if ( handleBars != INVALID_JOINT ) {
		float steerAngle = owner->GetSteerVisualAngle();
		idMat3 steerAxis;
		owner->GetAnimator()->GetJointTransform( handleBars, gameLocal.time, steerAxis );
		idAngles steerAngles = steerAxis.ToAngles();

		if ( fabs( steerAngles.yaw - steerAngle ) > 0.5f ) {
			steerAngles.Zero();
			steerAngles.yaw = -steerAngle * 1.0f;
			steerAxis = steerAngles.ToMat3();
			owner->GetAnimator()->SetJointAxis( handleBars, JOINTMOD_LOCAL, steerAxis );
		}
	}
}


/*
===============================================================================

	sdAirVehicleControl

===============================================================================
*/

/*
================
sdAirVehicleControl::sdAirVehicleControl
================
*/
sdAirVehicleControl::sdAirVehicleControl() {
}

/*
================
sdAirVehicleControl::Init
================
*/
void sdAirVehicleControl::Init( sdTransport* transport ) {
	sdVehicleControlBase::Init( transport );

	idDict& args = owner->spawnArgs;

	throttling							= false;
	landingGearDown						= true;
	
	landingThresholdDistance			= MetresToInches( args.GetFloat( "landing_threshold_distance" ) );
	landingThresholdSpeed				= KPHtoUPS( args.GetFloat( "landing_threshold_speed" ) );
	deadZoneFraction					= false;

	overDriveFactor						= args.GetFloat( "overdrive_factor" );
	spiralHealth						= 0.0f;

	collective							= 1.0f;
	collectiveMin						= args.GetFloat( "collective_min" );
	collectiveMax						= args.GetFloat( "collective_max" );
	collectiveRate						= args.GetFloat( "collective_rate" );
	collectiveDefault					= 0.0f;

	lastThrusterEffectsTime				= 0;

	lastDriverTime						= 0;

	isGrounded							= true;
	isLanding							= true;
	landingGearChangeTime				= 0;
	landingGearChangeEndTime			= 0;

	leftJet								= NULL;
	rightJet							= NULL;
	airBrake							= NULL;
	
	leftThrustEffectJoint				= owner->GetAnimator()->GetJointHandle( args.GetString( "left_thrust_effect_joint" ) );
	rightThrustEffectJoint				= owner->GetAnimator()->GetJointHandle( args.GetString( "right_thrust_effect_joint" ) );

	noThrusters							= false;
	height								= -1.f;

	oldCmdAngles.Zero();

	careenHeight						= args.GetFloat( "careen_height", "512" );
	careenSpeed							= args.GetFloat( "careen_speed", "150" );
	careenYaw							= args.GetFloat( "careen_yaw", "0.5" );
	careenRoll							= args.GetFloat( "careen_roll", "50" );
	careenPitch							= args.GetFloat( "careen_pitch", "25" );
	careenPitchExtreme					= args.GetFloat( "careen_pitch_extreme", "35" );
	careenLift							= args.GetFloat( "careen_lift", "1" );
	careenLiftExtreme					= args.GetFloat( "careen_lift_extreme", "0" );
	careenCollective					= args.GetFloat( "careen_collective", "0" );
	careenCruiseTime					= ( int )( 1000.0f * args.GetFloat( "careen_cruise_time", "0.5" ) );

	owner->SetLightsEnabled( 0, false );
}

/*
================
sdAirVehicleControl::SetupComponents
================
*/
void sdAirVehicleControl::SetupComponents( void ) {
	leftJet = owner->GetDriveObject( "left_thruster" )->Cast< sdVehicleThruster >();
	rightJet = owner->GetDriveObject( "right_thruster" )->Cast< sdVehicleThruster >();

	if ( !leftJet || !rightJet ) {
		noThrusters = true;
	}

	airBrake = owner->GetDriveObject( "air_brake" )->Cast< sdVehicleAirBrake >();

	mainBounds.Clear();	
	for ( int i = 0; i < owner->GetPhysics()->GetNumClipModels(); i++ ) {
		int contents = owner->GetPhysics()->GetContents( i );
		if ( contents && contents != MASK_HURTZONE ) {
			mainBounds.AddBounds( owner->GetPhysics()->GetBounds( i ) );
		}
	}
}

/*
================
sdAirVehicleControl::Update
================
*/
void sdAirVehicleControl::Update() {
	RunStateMachine();
	HandlePhysics();
	SetupInput();
}

/*
================
sdAirVehicleControl::EngineRunning
================
*/
bool sdAirVehicleControl::EngineRunning() {
	if ( owner->GetPhysics()->InWater() > drownHeight ) {
		return false;
	}

	return ( isLanding || ( gameLocal.time < landingGearChangeEndTime ) );
}

/*
================
sdAirVehicleControl::RunStateMachine
================
*/
void sdAirVehicleControl::RunStateMachine() {
	bool emped = owner->IsEMPed();
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	idVec3 absMins = owner->GetPhysics()->GetAbsBounds()[ 0 ];
	idVec3 absMaxs = owner->GetPhysics()->GetAbsBounds()[ 1 ];
	idVec3 traceOrg = ( absMins + absMaxs ) * 0.5f;
	idVec3 traceEnd = traceOrg;
	traceEnd.z -= 4096.0f;

	trace_t		traceObject;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS traceObject, traceOrg, traceEnd, MASK_SOLID | CONTENTS_WATER | MASK_OPAQUE, owner );
	traceEnd = traceObject.endpos;

	if ( traceObject.fraction < 1.0f ) {
		height = traceOrg.z - traceEnd.z;
	} else {
		height = -1.0f;
	}

	bool contact = IsContacting( absMins, traceObject );

	if( landingGearDown ) {
		isGrounded = contact;
		deadZoneFraction = ( absMins.z - traceObject.endpos.z ) / landingThresholdDistance;
		deadZoneFraction = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, deadZoneFraction );
	} else {
		isGrounded = false;
		deadZoneFraction = 0.0f;
	}

	if ( landingGearChangeEndTime < gameLocal.time ) {
		if ( !isLanding && ( owner->GetPositionManager().IsEmpty() || emped ) ) {
			isLanding = true;
			landingGearChangeTime = gameLocal.time;
			if ( owner->GetHealth() > 0 ) {
				owner->StartSound( "snd_engine_stop", SND_VEHICLE_DRIVE, 0, NULL );
				landingGearChangeEndTime = gameLocal.time + 2000;
			} else {
				landingGearChangeEndTime = gameLocal.time + 500;
			}
			owner->SetLightsEnabled( 0, false );
		} else if ( isLanding && ( driver && !emped ) ) {
			isLanding = false;
			landingGearChangeTime = gameLocal.time;
			owner->StartSound( "snd_engine_start", SND_VEHICLE_DRIVE, 0, NULL );
			landingGearChangeEndTime = gameLocal.time + 50;
			owner->SetLightsEnabled( 0, true );
		}
	}

	UpdateEffects( absMins, traceObject );
}

/*
================
sdAirVehicleControl::HandlePhysics
================
*/
void sdAirVehicleControl::HandlePhysics() {
	owner->UpdateEngine( EngineRunning() );
}

/*
================
sdAirVehicleControl::OnKeyMove
================
*/
bool sdAirVehicleControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	return false;
}

/*
================
sdAirVehicleControl::OnControllerMove
================
*/
void sdAirVehicleControl::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
								 const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {

	// run the input for each controller
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];
		sdInputModeHeli::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
	}
}

/*
================
sdAirVehicleControl::SetupInput
================
*/
void sdAirVehicleControl::SetupInput() {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	input->Clear();
	idVec3 directions = vec3_origin;
	idAngles cmdAngles = ang_zero;
	if ( driver != NULL ) {
		input->SetPlayer( driver );
		directions.Set( input->GetForward(), input->GetRight(), input->GetUp() );
		cmdAngles = input->GetCmdAngles();
	}

	idAngles angleDelta = cmdAngles - oldCmdAngles;
	angleDelta.Normalize180();
	oldCmdAngles = cmdAngles;

	bool simpleControls = driver && !driver->GetUserInfo().advancedFlightControls;
	float pitchInput = -angleDelta.pitch;
	float rollInput = -angleDelta.yaw;
	float yawInput = directions[ 1 ];
	if ( driver != NULL ) {
		if ( driver->GetUserInfo().swapFlightYawAndRoll ) {
			Swap( rollInput, yawInput );
			yawInput *= 2.0f;
		}
		yawInput = idMath::ClampFloat( -2.0f, 2.0f, yawInput );
	}

	if ( simpleControls ) {
		// Newbie mode!
		// Harvest data
		float timeDelta = MS2SEC( gameLocal.msec );
		idMat3 axis = owner->GetPhysics()->GetAxis();
		idAngles angles = axis.ToAngles();
		idVec3 origin = owner->GetPhysics()->GetOrigin();
		idVec3 linearVelocity = owner->GetPhysics()->GetLinearVelocity();
		float linearSpeed = linearVelocity.Length();
		idVec3 angVelocity = owner->GetPhysics()->GetAngularVelocity();

		idMat3 yawAxis;
		idAngles::YawToMat3( angles.yaw, yawAxis );
		float pitchVelocity = angVelocity * yawAxis[ 1 ];
		float rollVelocity = angVelocity * axis[ 0 ];


		// Hooray for magic numbers!
		// TODO - move these magic numbers into the def files. It doesn't seem necessary at the moment
		//        as these values work well for all current flying vehicles, but one day
		//        modders may make some crazy vehicle that makes this need tweaking

		// calculate if we should autolevel or not
		bool autoLevellingRoll = false;
		bool autoLevellingPitch = false;

		float autoLevelPitchScale = 1.0f;
		float autoLevelRollScale = 1.0f;
		if ( idMath::Fabs( rollInput ) < 0.1f && idMath::Fabs( pitchInput ) < 0.1f && idMath::Fabs( yawInput ) < 0.1f ) {
			if ( simpleControls || deadZoneFraction ) {
				autoLevellingRoll = true;
				autoLevellingPitch = true;
			}
		}

		// scale the autolevel using speed
		float autoLevelScale = ( linearSpeed - 200.0f ) / 1000.0f;
		autoLevelScale = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, autoLevelScale );
		autoLevelScale = autoLevelScale * autoLevelScale;
		autoLevelPitchScale *= autoLevelScale;

		if ( idMath::Fabs( yawInput ) < 0.1f ) {
			autoLevelScale = autoLevelScale * 0.5f + 0.5f;
		} else {
			autoLevelScale = autoLevelScale * 0.75f + 0.25f;
		}

		autoLevelRollScale *= autoLevelScale;

		// don't let the player get too out of control!
		float minPitch = -1000.0f;
		float maxPitch = 1000.0f;
		float minRoll = -1000.0f;
		float maxRoll = 1000.0f;

		// don't allow them to tilt over too far
		if ( angles.pitch > 45.0f || angles.pitch < -30.0f ) {
			float desiredPitch = idMath::ClampFloat( -30.0f, 45.0f, angles.pitch );
			
			// calculate the pitching input needed to get back towards the limit
			float levellingPitch = ( angles.pitch - desiredPitch ) * timeDelta + pitchVelocity * 2.0f;
			if ( desiredPitch < 0.0f ) { 
				maxPitch = levellingPitch < 0.0f ? levellingPitch : 0.0f;
			} else {
				minPitch = levellingPitch > 0.0f ? levellingPitch : 0.0f;
			}
		}

		if ( idMath::Fabs( angles.roll ) > 30.0f ) {
			float desiredRoll = idMath::ClampFloat( -30.0f, 30.0f, angles.roll );
			
			// calculate the rolling input needed to get back towards the limit
			float levellingRoll = ( desiredRoll - angles.roll ) * timeDelta - rollVelocity;
			if ( desiredRoll > 0.0f ) { 
				maxRoll = levellingRoll < 0.0f ? levellingRoll : 0.0f;
			} else {
				minRoll = levellingRoll > 0.0f ? levellingRoll : 0.0f;
			}
		}

		// stop them from giving so much input they start to spin out of control
		if ( pitchVelocity > 0.8f ) {
			minPitch = minPitch < 1.0f ? 1.0f : minPitch;
		} else if ( pitchVelocity < -0.8f ) {
			maxPitch = maxPitch > -1.0f ? -1.0f : maxPitch;
		}

		if ( rollVelocity > 0.8f ) {
			maxRoll = maxRoll > -1.0f ? -1.0f : maxRoll;
		} else if ( rollVelocity < -0.8f ) {
			minRoll = minRoll < 1.0f ? 1.0f : minRoll;
		}

		rollInput = idMath::ClampFloat( minRoll, maxRoll, rollInput );
		pitchInput = idMath::ClampFloat( minPitch, maxPitch, pitchInput );

		// perform the autolevelling
		if ( autoLevellingRoll ) {
			float autoLevelRoll = -angles.roll * 0.15f - rollVelocity * 10.0f;
			rollInput += autoLevelRoll * autoLevelRollScale;
		}

		if ( autoLevellingPitch ) {
			float autoLevelPitch = ( angles.pitch * 0.15f + pitchVelocity * 10.0f ) * 0.15f;
			rollInput += autoLevelPitch * autoLevelPitchScale;
		}
	}

	input->SetPitch( pitchInput );
	input->SetRoll( rollInput );
	input->SetYaw( yawInput );

	float frameTime = MS2SEC( gameLocal.msec );
	if ( directions.x != 0.0f ) {
		if ( !isGrounded ) {
			collective = collective + ( directions.x * frameTime * collectiveRate );
		} else {
			collective = directions.x;
		}

		collectiveDefault = 0.0f;
	} else {
		float diff = collective - collectiveDefault;
		float rate = frameTime * collectiveRate;

		if ( fabs( diff ) < rate ) {
			collective = collectiveDefault;
		} else {
			if ( diff > 0.0f ) {
				collective = collective - rate;
			} else {
				collective = collective + rate;
			}
		}
	}

	if ( collective < collectiveMin ) {
		collective = collectiveMin;
	} else if ( collective > collectiveMax ) {
		collective = collectiveMax;
	}

	bool canThrust = false;
	bool run = false;
	bool sprint = false;
	if ( driver ) {
		run = driver->usercmd.buttons.btn.run;
		sprint = driver->usercmd.buttons.btn.sprint;
		canThrust = ( sprint | !run ) && !owner->IsEMPed();
		lastDriverTime = gameLocal.time;
	} else {
		if ( gameLocal.time - lastDriverTime > SEC2MS( 5 ) || owner->GetPositionManager().IsEmpty() ) {
			collectiveDefault = collectiveMin;
		}
	}

	UpdateLandingGear( directions );
	
	float thrust = 0.0f;
	bool thrusters = canThrust && !owner->IsEMPed();
	bool careening = owner->IsCareening() && !owner->InDeathThroes();
	if ( careening ) {
		thrusters = true;
		run = true;
	}

	if ( thrusters ) {
		if ( !run ) {
			thrust = -overDriveFactor;
			if( driver == gameLocal.GetLocalPlayer() && driver ) {
				gameLocal.SetGUIFloat( GUI_GLOBALS_HANDLE, "vehicles.overDriveFraction", 0.0f );
			}			
		} else {
			thrust = overDriveFactor;
			if( driver == gameLocal.GetLocalPlayer() && driver ) {
				gameLocal.SetGUIFloat( GUI_GLOBALS_HANDLE, "vehicles.overDriveFraction", 1.0f );
			}			
		}

		if ( !owner->IsInPlayzone() ) {
			thrust *= 0.5f;
		}

		if ( gameLocal.isNewFrame ) {
			if ( !overDrivePlayingSound ) {
				owner->StartSound( "snd_overdrive", SND_VEHICLE_OVERDRIVE, 0, NULL );
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, 0.0f, 0.01f );
				overDrivePlayingSound = true;
			}
		}
	} else {
		if ( gameLocal.isNewFrame ) {
			if ( overDrivePlayingSound ) {
				owner->FadeSound( SND_VEHICLE_OVERDRIVE, -60.f, 1.f );
				owner->StartSound( "snd_overdrive_stop", SND_VEHICLE_OVERDRIVE, 0, NULL );
				overDrivePlayingSound = false;
			}
		}
		if( driver == gameLocal.GetLocalPlayer() && driver ) {
			gameLocal.SetGUIFloat( GUI_GLOBALS_HANDLE, "vehicles.overDriveFraction", 0.5f );
		}		
	}

	if ( thrusters && gameLocal.time >= ( lastThrusterEffectsTime + 1000 ) )  {
		idVec3 white( 1.0f, 1.0f, 1.0f );
		bool played = false;
		if ( run ) {
			if ( leftThrustEffectJoint != INVALID_JOINT ) {
				played |= owner->PlayEffect( "fx_thruster_left", white, NULL, leftThrustEffectJoint ) != NULL;
			}
			if ( rightThrustEffectJoint != INVALID_JOINT ) {
				played |= owner->PlayEffect( "fx_thruster_right", white, NULL, rightThrustEffectJoint ) != NULL;
			}
		} else {
			if ( leftThrustEffectJoint != INVALID_JOINT ) {
				played |= owner->PlayEffect( "fx_thruster_reverse_left", white, NULL, leftThrustEffectJoint ) != NULL;
			}
			if ( rightThrustEffectJoint != INVALID_JOINT ) {
				played |= owner->PlayEffect( "fx_thruster_reverse_right", white, NULL, rightThrustEffectJoint ) != NULL;
			}
		}

		if ( played ) {
			lastThrusterEffectsTime = gameLocal.time;
		}
	}

	if ( airBrake != NULL ) {
		if ( thrust < 0.f ) {
			airBrake->Enable();
			thrust = 0.f;
		} else {
			airBrake->Disable();
		}
	}

	if ( careening ) {
		thrust *= 0.5f;
	}

	if ( !noThrusters ) {
		leftJet->SetThrust( thrust );
		rightJet->SetThrust( thrust );
	}

	input->SetCollective( collective );
	
	// hack so that careening vehicles blow up when contacting ground even if it the suspension touching
	if ( !gameLocal.isClient ) {
		if ( ( owner->IsCareening() || owner->InDeathThroes() ) && owner->GetPhysics()->HasGroundContacts() ) {
			sdVehicle_RigidBody* rbOwner = owner->Cast< sdVehicle_RigidBody >();

			if ( rbOwner != NULL ) {
				// kill self
				const sdDeclDamage* collideDamage = rbOwner->GetCollideDamage();
				if ( collideDamage != NULL ) {
					owner->Damage( owner, owner, idVec3( 0.0f, 0.0f, 1.0f ), collideDamage, 10000.0f, NULL );
				}
			}
		}
	}
}

/*
================
sdAirVehicleControl::IsContacting
================
*/
bool sdAirVehicleControl::IsContacting( const idVec3& absMins, const trace_t& traceObject ) {
	return ( absMins.z - traceObject.endpos.z ) < landingThresholdDistance;
}

/*
================
sdAirVehicleControl::OnPlayerEntered
================
*/
void sdAirVehicleControl::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	if ( !gameLocal.isClient ) {
		owner->SetCareening( 0 );
	}
}

/*
================
sdAirVehicleControl::OnPlayerExited
================
*/
void sdAirVehicleControl::OnPlayerExited( idPlayer* player, int position ) {
	if ( !gameLocal.isClient && owner->GetPositionManager().IsEmpty() ) {
		// check careening conditions
		const idVec3& velocity = owner->GetPhysics()->GetLinearVelocity();
		if ( velocity.LengthSqr() > careenSpeed*careenSpeed ) {
			owner->SetCareening( gameLocal.time );
		} else {
			idBounds bounds = mainBounds;
			const idVec3& origin = owner->GetPhysics()->GetOrigin();
			const idMat3& axis = owner->GetPhysics()->GetAxis();
			bounds.Rotate( axis );

			trace_t trace;
			gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, origin, origin - idVec3( 0.0f, 0.0f, careenHeight ), bounds, mat3_identity, CONTENTS_SOLID, owner );
			if ( trace.fraction >= 1.0f ) {
				owner->SetCareening( gameLocal.time );
			}
		}
	}
}

/*
================
sdAirVehicleControl::OnPostDamage
================
*/
void sdAirVehicleControl::OnPostDamage( idEntity* attacker, int oldHealth, int newHealth ) {
	if ( spiralHealth > 0 && newHealth <= spiralHealth && oldHealth > spiralHealth ) {
		owner->GetPhysics()->Activate();
		owner->SetDeathThroes( true );
		owner->GetPositionManager().EjectAllPlayers( EF_KILL_PLAYERS );
	} else if ( newHealth > spiralHealth ) {
		owner->SetDeathThroes( false );
	}
}

/*
================
sdAirVehicleControl::GetCareeningCollideScale
================
*/
float sdAirVehicleControl::GetCareeningCollideScale() const {
	return 1000.0f;
}

/*
================
sdAirVehicleControl::GetCareeningRollAmount
================
*/
float sdAirVehicleControl::GetCareeningRollAmount() const {
	float angle = 0.0f;
	int time = owner->GetCareeningTime();
	if ( time > careenCruiseTime ) {
		float bankSpeed = owner->GetPhysics()->GetAngularVelocity() * owner->GetPhysics()->GetAxis()[ 0 ];
		if ( bankSpeed <= 0.0f ) {
			angle = -careenRoll;
		} else {
			angle = careenRoll;
		}
	}

	return angle;
}

/*
================
sdAirVehicleControl::GetCareeningPitchAmount
================
*/
float sdAirVehicleControl::GetCareeningPitchAmount() const {
	int time = owner->GetCareeningTime();
	if ( time < careenCruiseTime ) {
		float pitchSpeed = owner->GetPhysics()->GetAngularVelocity() * owner->GetPhysics()->GetAxis()[ 1 ];
		if ( pitchSpeed <= 0.0f ) {
			return careenPitch;
		} else {
			return -careenPitch;
		}
	} else {
		return -careenPitchExtreme;
	}
}

/*
================
sdAirVehicleControl::GetCareeningYawAmount
================
*/
float sdAirVehicleControl::GetCareeningYawAmount() const {
	// calculate the direction this should be spinning in
	float angle = 0.0f;
	int time = owner->GetCareeningTime();
	if ( time > careenCruiseTime ) {
		float yawSpeed = owner->GetPhysics()->GetAngularVelocity() * owner->GetPhysics()->GetAxis()[ 2 ];
		if ( yawSpeed <= 0.0f ) {
			angle = -careenYaw;
		} else {
			angle = careenYaw;
		}
	}

	return angle;
}

/*
================
sdAirVehicleControl::GetCareeningLiftScale
================
*/
float sdAirVehicleControl::GetCareeningLiftScale() const {
	int time = owner->GetCareeningTime();
	if ( time < careenCruiseTime * 2.0f ) {
		return careenLift;
	}
	return careenLiftExtreme;
}

/*
================
sdAirVehicleControl::GetCareeningCollectiveAmount
================
*/
float sdAirVehicleControl::GetCareeningCollectiveAmount() const {
	return careenCollective;
}

/*
===============================================================================

	sdHornetControl

===============================================================================
*/

/*
================
sdHornetControl::Init
================
*/
void sdHornetControl::Init( sdTransport* transport ) {
	sdAirVehicleControl::Init( transport );

	owner->StartSound( "snd_throttle", SND_VEHICLE_IDLE, 0, NULL );
	owner->FadeSound( SND_VEHICLE_IDLE, -60.0f, 0.0f );

	groundEffects						= false;
	groundEffectsThreshhold				= owner->spawnArgs.GetFloat( "groundeffects_threshhold" );
	lastGroundEffectsTime				= 0;

	landingAnimEndTime = 0;
}

/*
================
sdHornetControl::OnPlayerEntered
================
*/
void sdHornetControl::OnPlayerEntered( idPlayer* player, int position, int oldPosition ) {
	sdAirVehicleControl::OnPlayerEntered( player, position, oldPosition );

	if ( position == 0 ) {
		const char *sparks = owner->spawnArgs.GetString( "joints_up_sparks" );
		if ( sparks && *sparks ) {
			idStrList placement;
			idSplitStringIntoList( placement, sparks, ";" );
			for (int i=0; i<placement.Num(); i++) {
				jointHandle_t jh = owner->GetAnimator()->GetJointHandle( placement[i] );
				if ( jh != INVALID_JOINT ) {
					idVec3 white(1.f,1.f,1.f);
					owner->PlayEffect( "fx_up_sparks", white, NULL, jh );
				}
			}
		}
	}
}

/*
================
sdHornetControl::OnPlayerEntered
================
*/
void sdHornetControl::OnEMPStateChanged( void ) {
	if ( !owner->IsEMPed() ) {
		owner->StartSound( "snd_engine_start", SND_VEHICLE_DRIVE, 0, NULL );
		owner->StartSound( "snd_throttle", SND_VEHICLE_IDLE, 0, NULL );
		owner->FadeSound( SND_VEHICLE_IDLE, -60.0f, 0.0f );
	} else {
		owner->StopSound( SND_VEHICLE_IDLE );
		owner->StartSound( "snd_engine_stop", SND_VEHICLE_DRIVE, 0, NULL );
	}
}

/*
================
sdHornetControl::UpdateEffects
================
*/
void sdHornetControl::UpdateEffects( const idVec3& absMins, const trace_t& traceObject ) {
	idPlayer* driver = owner->GetPositionManager().FindDriver();

	if( driver && landingGearChangeEndTime < gameLocal.time ) {
		const idVec3& physics = owner->GetPhysics()->GetLinearVelocity();
		owner->FadeSound( SND_VEHICLE_IDLE, ( collective * 30.0f ) - 10.0f, collectiveRate * 0.1f );
		owner->SetChannelPitchShift( SND_VEHICLE_IDLE, physics.Length() * 0.0005f + 1.0f );
	} else {
		owner->FadeSound( SND_VEHICLE_IDLE, -60.0f, 0.5f );
		owner->SetChannelPitchShift( SND_VEHICLE_IDLE, 1.0f );
	}

	groundEffects = ( absMins.z - traceObject.endpos.z ) < groundEffectsThreshhold;

	idVec3 white( 1.0f, 1.0f, 1.0f );
	bool isEmpty = owner->GetPositionManager().IsEmpty();
/*	if ( !downdraftPlaying && !isEmpty ) {

		if ( mainJoint != INVALID_JOINT && leftJetJoint != INVALID_JOINT && rightJetJoint != INVALID_JOINT ) {
			owner->PlayEffect( "fx_downdraft", white, NULL, mainJoint, true );
			owner->PlayEffect( "fx_thruster_base_right", white, NULL, leftJetJoint, true );
			owner->PlayEffect( "fx_thruster_base_left", white, NULL, rightJetJoint, true );
		}

		downdraftPlaying = true;
	} else if ( downdraftPlaying && isEmpty ) {
		owner->StopAllEffects();
		downdraftPlaying = false;
	}*/

	if ( gameLocal.time >= ( lastGroundEffectsTime + 100 ) && !isEmpty
		&& groundEffects && traceObject.fraction < 1.0f 
		&& gameLocal.time > landingGearChangeEndTime ) {

		const char* surfaceTypeName = NULL;
		if ( traceObject.c.surfaceType ) {
			surfaceTypeName = traceObject.c.surfaceType->GetName();
		}

		owner->PlayEffect( "fx_groundeffect", white, surfaceTypeName, traceObject.endpos, traceObject.c.normal.ToMat3(), 0 );
		lastGroundEffectsTime = gameLocal.time;
	}

}

/*
================
sdHornetControl::IsContacting
================
*/
bool sdHornetControl::IsContacting( const idVec3& absMins, const trace_t& traceObject ) {
	if ( owner->IsAtRest() && owner->GetPhysics()->HasGroundContacts() ) {
		return true;
	} 

	return ( absMins.z - traceObject.endpos.z ) < landingThresholdDistance;
}

/*
================
sdHornetControl::UpdateLandingGear
================
*/
void sdHornetControl::UpdateLandingGear( const idVec3& directions ) {
	if ( landingAnimEndTime > gameLocal.time ) {
		return;
	}

	float speed = owner->GetPhysics()->GetLinearVelocity() * owner->GetPhysics()->GetAxis()[ 0 ];
	float absSpeed = fabs( speed );

	int anim = 0; 
	if ( height < landingThresholdDistance && absSpeed < landingThresholdSpeed && directions.z <= 0.0f  ) {
		if( !landingGearDown ) {
			landingGearDown = true;
			anim = owner->GetAnimator()->GetAnim( "gear_down" );

			sdScriptHelper h1;
			owner->GetScriptObject()->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnLandingGearDown" ), h1 );
		}
	} else {
		if ( landingGearDown ) {
			landingGearDown = false;
			anim = owner->GetAnimator()->GetAnim( "gear_up" );

			sdScriptHelper h1;
			owner->GetScriptObject()->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnLandingGearUp" ), h1 );
		}
	}

	if ( anim ) {
		owner->GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
		landingAnimEndTime = gameLocal.time + owner->GetAnimator()->AnimLength( anim );
	}
}

/*
===============================================================================

	sdHovercopterControl

===============================================================================
*/

/*
================
sdHovercopterControl::Init
================
*/
void sdHovercopterControl::Init( sdTransport* transport ) {
	sdAirVehicleControl::Init( transport );

	downdraftPlaying					= false;
	groundEffects						= false;
	groundEffectsThreshhold				= owner->spawnArgs.GetFloat( "groundeffects_threshhold" );
	lastGroundEffectsTime				= 0;

	mainJoint							= owner->GetAnimator()->GetJointHandle( "main" );
	leftJetJoint						= owner->GetAnimator()->GetJointHandle( "left_thruster" );
	rightJetJoint						= owner->GetAnimator()->GetJointHandle( "right_thruster" );
}

/*
================
sdHovercopterControl::UpdateEffects
================
*/
void sdHovercopterControl::UpdateEffects( const idVec3& absMins, const trace_t& traceObject ) {
	groundEffects = ( absMins.z - traceObject.endpos.z ) < groundEffectsThreshhold;

	idVec3 white( 1.0f, 1.0f, 1.0f );
	bool isEmpty = owner->GetPositionManager().IsEmpty();
	if ( !downdraftPlaying && !isEmpty ) {

		if ( mainJoint != INVALID_JOINT && leftJetJoint != INVALID_JOINT && rightJetJoint != INVALID_JOINT ) {
			owner->PlayEffect( "fx_downdraft", white, NULL, mainJoint, true );
			owner->PlayEffect( "fx_thruster_base_right", white, NULL, leftJetJoint, true );
			owner->PlayEffect( "fx_thruster_base_left", white, NULL, rightJetJoint, true );
		}

		downdraftPlaying = true;
	} else if ( downdraftPlaying && isEmpty ) {
		owner->StopEffect( "fx_downdraft" );
		owner->StopEffect( "fx_thruster_base_right" );
		owner->StopEffect( "fx_thruster_base_left" );
		downdraftPlaying = false;
	}

	if ( gameLocal.time >= ( lastGroundEffectsTime + 100 ) && !isEmpty
		&& groundEffects && traceObject.fraction < 1.0f 
		&& gameLocal.time > landingGearChangeEndTime ) {

		const char* surfaceTypeName = NULL;
		if ( traceObject.c.surfaceType ) {
			surfaceTypeName = traceObject.c.surfaceType->GetName();
		}

		owner->PlayEffect( "fx_groundeffect", white, surfaceTypeName, traceObject.endpos, traceObject.c.normal.ToMat3(), false, vec3_origin, false );
		lastGroundEffectsTime = gameLocal.time;
	}
}

/*
===============================================================================

	sdAnansiControl

===============================================================================
*/

/*
================
sdAnansiControl::Init
================
*/
void sdAnansiControl::Init( sdTransport* transport ) {
	sdHovercopterControl::Init( transport );

	landingAnimEndTime = 0;
}


/*
================
sdAnansiControl::UpdateLandingGear
================
*/
void sdAnansiControl::UpdateLandingGear( const idVec3& directions ) {
	if ( landingAnimEndTime > gameLocal.time ) {
		return;
	}

	float speed = owner->GetPhysics()->GetLinearVelocity() * owner->GetPhysics()->GetAxis()[ 0 ];
	float absSpeed = fabs( speed );

	int anim = 0; 
	if ( height < landingThresholdDistance && absSpeed < landingThresholdSpeed && directions.z <= 0.0f ) {
		if( !landingGearDown ) {
			landingGearDown = true;
			anim = owner->GetAnimator()->GetAnim( "gear_down" );

			sdScriptHelper h1;
			owner->GetScriptObject()->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnLandingGearDown" ), h1 );
		}
	} else {
		if ( landingGearDown ) {
			landingGearDown = false;
			anim = owner->GetAnimator()->GetAnim( "gear_up" );

			sdScriptHelper h1;
			owner->GetScriptObject()->CallNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnLandingGearUp" ), h1 );
		}
	}

	if ( anim ) {
		owner->GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
		landingAnimEndTime = gameLocal.time + owner->GetAnimator()->AnimLength( anim );
	}
}


/*
===============================================================================

  sdWalkerNetworkData

===============================================================================
*/

/*
================
sdWalkerNetworkData::MakeDefault
================
*/
void sdWalkerNetworkData::MakeDefault( void ) {
	idealYaw	= 0.f;
	currentYaw	= 0.f;
	turnScale	= 0.f;
	crouching	= false;
}

/*
================
sdWalkerNetworkData::Write
================
*/
void sdWalkerNetworkData::Write( idFile* file ) const {
	file->WriteFloat( idealYaw );
	file->WriteFloat( currentYaw );
	file->WriteFloat( turnScale );
	file->WriteBool( crouching );
}

/*
================
sdWalkerNetworkData::Read
================
*/
void sdWalkerNetworkData::Read( idFile* file ) {
	file->ReadFloat( idealYaw );
	file->ReadFloat( currentYaw );
	file->ReadFloat( turnScale );
	file->ReadBool( crouching );
}

/*
===============================================================================

	sdWalkerControl

===============================================================================
*/

#define IsTurningOnSpot() ( ( newState == CS_TURN_LEFT ) || ( newState == CS_TURN_RIGHT ) )
#define MovingBackwards() ( ( newState == CS_WALK_BACK_LEFT_LEG ) || ( newState == CS_WALK_BACK_RIGHT_LEG ) || ( newState == CS_WALK_BACK_LEFT_LEG_START ) )

/*
================
sdWalkerControl::Init
================
*/
void sdWalkerControl::Init( sdTransport* transport ) {
	sdVehicleControlBase::Init( transport );

	walker = owner->Cast< sdWalker >();
	if ( !walker ) {
		gameLocal.Error( "sdWalkerControl::Init Walker control can only be used on sdWalker based entities" );
	}

	owner->SetLightsEnabled( 0, false );
	
	owner->SetLightsEnabled( 1, true );
	walker->SetCompressionScale( 0.9f, 0.1f );

	idAnimator* animator = transport->GetAnimator();

	for ( int i = 0; i < CS_NUM_STATES; i++ ) {
		stateAnims[ i ] = animator->GetAnim( owner->spawnArgs.GetString( va( "anim_state_%i", i ) ) );
	}

	dynamicTurnRate		= 0.f;
	currentYaw			= 0;
	idealYaw			= owner->GetRenderEntity()->axis.ToAngles().yaw;
	turnScale			= 0.f;
	turnRate			= owner->spawnArgs.GetFloat( "turn_rate" );

	currentState		= CS_SPAWN;
	newState			= CS_SPAWN;
	newStateTime		= 0;

	leftFootJoint		= walker->GetAnimator()->GetJointHandle( walker->spawnArgs.GetString( "joint_foot_left" ) );
	rightFootJoint		= walker->GetAnimator()->GetJointHandle( walker->spawnArgs.GetString( "joint_foot_right" ) );
	leftFootOnGround	= false;
	rightFootOnGround	= false;
	lastLeftFootEffectTime	= 0;
	lastRightFootEffectTime	= 0;
	lastLeftFootGroundOrg.Zero();
	lastRightFootGroundOrg.Zero();
	lastLeftFootOrg.Zero();
	lastRightFootOrg.Zero();

	fallStartTime		= 0;

	flags.powered			= false;
	flags.turnOnSpot		= owner->spawnArgs.GetBool( "turn_on_spot" );
	flags.playStopAnim		= owner->spawnArgs.GetBool( "play_stop_anim", "1" );
	flags.manualCrouch		= false;
	flags.startOnLeftLeg	= owner->spawnArgs.GetBool( "start_on_left", "1" );

	walkingGroundPoundForce			= owner->spawnArgs.GetFloat( "ground_pound_walk_force", "10000000" );	
	walkingGroundPoundDamageScale	= owner->spawnArgs.GetFloat( "ground_pound_walk_damage_scale", "0.25" );	
	walkingGroundPoundRange			= owner->spawnArgs.GetFloat( "ground_pound_walk_range", "384" );	
}

/*
================
sdWalkerControl::Update
================
*/
void sdWalkerControl::Update() {
	RunStateMachine();

	owner->UpdateEngine( !flags.powered );

	Move();

	if ( gameLocal.isNewFrame ) {
		// no point spawning sliding foot effects in reprediction
		UpdateSliding();
	}
}

/*
================
sdWalkerControl::TurnToward
================
*/
void sdWalkerControl::TurnToward( float yaw ) {
	idealYaw = idMath::AngleNormalize180( yaw );
}

/*
================
sdWalkerControl::Turn
================
*/
void sdWalkerControl::Turn( void ) {
	if ( idMath::Fabs( idealYaw - currentYaw ) < 0.01f ) {
		return;
	}

	float rate;
	if ( IsTurningOnSpot() ) {
		rate = dynamicTurnRate;
	} else {
		rate = turnRate;
	}

	float ang = idMath::AngleNormalize180( idealYaw - currentYaw );

	float maxTurn = rate * gameLocal.msec;
	
	float newAngles;
	if ( ang < -maxTurn ) {
		newAngles = currentYaw - maxTurn;
	} else if( ang > maxTurn ) {
		newAngles = currentYaw + maxTurn;
	} else {
		newAngles = idealYaw;
	}
 
	SetYaw( newAngles );
}

/*
================
sdWalkerControl::OnYawChanged
================
*/
void sdWalkerControl::OnYawChanged( float newYaw ) {
	currentYaw	= newYaw;
	idealYaw	= newYaw;
}

/*
================
sdWalkerControl::SetPowered
================
*/
void sdWalkerControl::SetPowered( bool value ) {
	if ( flags.powered == value ) {
		return;
	}

	flags.powered = value;

	if ( flags.powered ) {
		owner->StartSound( "snd_powerup", SND_ANY, 0, NULL );
		owner->StartSound( "snd_engine_start_interior", SND_VEHICLE_INTERIOR_IDLE, 0, NULL );
		owner->StartSound( "snd_engine_start", SND_VEHICLE_IDLE, 0, NULL );
		owner->SetLightsEnabled( 0, true );
		
		const char *sparks = owner->spawnArgs.GetString( "joints_up_sparks" );
		if ( sparks && *sparks ) {
			idStrList placement;
			idSplitStringIntoList( placement, sparks, ";" );
			for (int i=0; i<placement.Num(); i++) {
				jointHandle_t jh = owner->GetAnimator()->GetJointHandle( placement[i] );
				if ( jh != INVALID_JOINT ) {
					idVec3 white(1.f,1.f,1.f);
					owner->PlayEffect( "fx_up_sparks", white, NULL, jh );
				}
			}
		}

		walker->SetCompressionScale( 0.3f, 1.f );
	} else {
		owner->StartSound( "snd_shutdown", SND_ANY, 0, NULL );
		owner->StartSound( "snd_engine_stop_interior", SND_VEHICLE_INTERIOR_IDLE, 0, NULL );
		owner->StartSound( "snd_engine_stop", SND_VEHICLE_IDLE, 0, NULL );
		owner->SetLightsEnabled( 0, false );

		walker->SetCompressionScale( 0.9f, 1.f );
	}
}

/*
================
sdWalkerControl::OnNewStateBegun
================
*/
void sdWalkerControl::OnNewStateBegun( controlState_t state ) {
	switch ( state ) {
		case CS_SHUTDOWN:
			SetPowered( false );
			break;
		case CS_POSED:
			SetPowered( true );
			break;
	}
}

/*
================
sdWalkerControl::OnNewStateCompleted
================
*/
void sdWalkerControl::OnNewStateCompleted( controlState_t state ) {
	switch ( state ) {
		case CS_WALK_LEFT_LEG:
		case CS_WALK_LEFT_LEG_START:
		case CS_WALK_BACK_LEFT_LEG:
		case CS_WALK_BACK_LEFT_LEG_START:
		case CS_WALK_RIGHT_LEG:
		case CS_WALK_RIGHT_LEG_START:
		case CS_WALK_BACK_RIGHT_LEG: 
		case CS_WALK_BACK_RIGHT_LEG_START: {
			walker->GroundPound( walkingGroundPoundForce, walkingGroundPoundDamageScale, walkingGroundPoundRange );
			break;
		}
	}
}

/*
================
sdWalkerControl::OnOldStateFinished
================
*/
void sdWalkerControl::OnOldStateFinished( controlState_t state ) {
}

/*
================
sdWalkerControl::SetupNextState
================
*/
bool sdWalkerControl::SetupNextState( controlState_t state, int time, int blendTime ) {
	if ( !stateAnims[ state ] ) {
		return false;
	}

	idAnimator* animator = owner->GetAnimator();

	const idAnim* anim = animator->GetAnim( stateAnims[ state ] );

	int length = anim->Length();

	if ( newState == state && newStateTime == time + length ) {
		return true;
	}

	newState			= state;
	newStateTime		= time + length;
	newStateStartTime	= time;
	
	animator->PlayAnim( ANIMCHANNEL_LEGS, stateAnims[ state ], time, blendTime );

	// Gordon: Need to test with and without this, as i'd rather not do this if we don't "have" to
/*	if ( gameLocal.isServer ) {
		sdEntityBroadcastEvent msg( owner, sdTransport::EVENT_CONTROLMESSAGE );
		msg.WriteBits( state, idMath::BitsForInteger( CS_NUM_STATES ) );
		msg.WriteLong( blendTime );
		msg.Send( false, false );
	}*/

	float turnScale = 0.f;
	switch ( state ) {
		case CS_TURN_RIGHT:
			turnScale = -1.f;
			break;
		case CS_TURN_LEFT:
			turnScale = 1.f;
			break;
	}

	if ( turnScale != 0.f ) {
		TurnToward( currentYaw + ( turnScale * turnRate ) );		
		dynamicTurnRate = turnRate / length;
	} else {
		TurnToward( currentYaw );
	}

	OnNewStateBegun( state );

	return true;
}

/*
================
sdWalkerControl::OnKeyMove
================
*/
bool sdWalkerControl::OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	return false;
}

/*
================
sdWalkerControl::OnControllerMove
================
*/
void sdWalkerControl::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers, const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	// run the input for each controller
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];
		sdInputModePlayer::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
	}
}

idCVar g_walkerTraceDistance( "g_walkerTraceDistance", "128", CVAR_GAME | CVAR_FLOAT | CVAR_NETWORKSYNC | CVAR_RANKLOCKED, "distance to check for space for the walker to move" );

/*
================
sdWalkerControl::CheckWalk
================
*/
bool sdWalkerControl::CheckWalk( float direction ) {
	const idMat3& walkerAxes = walker->GetAxis();

	float distance = g_walkerTraceDistance.GetFloat();
	if ( distance <= 0.f ) {
		return true;
	}

	idVec3 forward = walkerAxes[ 0 ] * direction * distance;

	idVec3 up( 0.f, 0.f, walker->GetMonsterPhysics().GetMaxStepHeight() );

	idClipModel* cm = walker->GetPhysics()->GetClipModel();

	const idVec3& origin = walker->GetPhysics()->GetOrigin();
	
	trace_t trace;

	gameLocal.clip.TranslationWorld( CLIP_DEBUG_PARMS trace, origin, origin + forward, cm, mat3_identity, walker->GetPhysics()->GetClipMask() );
	if ( trace.fraction == 1.f ) {
		return true;
	}

	gameLocal.clip.TranslationWorld( CLIP_DEBUG_PARMS trace, origin + up, origin + up + forward, cm, mat3_identity, walker->GetPhysics()->GetClipMask() );
	if ( trace.fraction == 1.f ) {
		return true;
	}

	return false;
}

/*
================
sdWalkerControl::CheckStateExit
================
*/
bool sdWalkerControl::CheckStateExit( void ) {
	if ( newState == CS_FALL ) {
		if ( walker->GetMonsterPhysics().OnGround() ) {
			SetupNextState( CS_LAND, gameLocal.time, 50 );
			return true;
		}
		return false;
	}

	if ( gameLocal.time - newStateStartTime < 100 ) { // we've only jsut started
		return false;
	}
	if ( newStateTime - gameLocal.time < 100 ) { // we're close to the end
		return false;
	}

	switch ( newState ) {
		case CS_WALK_RIGHT_LEG_START:
		case CS_WALK_LEFT_LEG:
		case CS_WALK_LEFT_LEG_START:
		case CS_WALK_RIGHT_LEG: {
			if ( input->GetForward() <= 0.f ) {
				SetupNextState( CS_STAND, gameLocal.time, 250 );
				return true;
			}
			break;
		}
		case CS_WALK_BACK_LEFT_LEG_START:
		case CS_WALK_BACK_LEFT_LEG:
		case CS_WALK_BACK_RIGHT_LEG_START:
		case CS_WALK_BACK_RIGHT_LEG: {
			if ( input->GetForward() >= 0.f ) {
				SetupNextState( CS_STAND, gameLocal.time, 250 );
				return true;
			}
			break;
		}
		case CS_TURN_LEFT: {
			if ( input->GetRight() >= 0.f ) {
				SetupNextState( CS_STAND, gameLocal.time, 250 );
				return true;
			}
			break;
		}
		case CS_TURN_RIGHT: {
			if ( input->GetRight() <= 0.f ) {
				SetupNextState( CS_STAND, gameLocal.time, 250 );
				return true;
			}
			break;
		}
	}
	return false;
}

/*
================
sdWalkerControl::RunStateMachine
================
*/
void sdWalkerControl::RunStateMachine( void ) {
	idPlayer* driver	= owner->GetPositionManager().FindDriver();

	input->Clear();
	input->SetPlayer( driver );

	if ( newStateTime != 0 ) {
		if ( gameLocal.time < newStateTime ) {
			CheckStateExit();
			return;
		}

		OnOldStateFinished( currentState );

		currentState	= newState;
		newState		= CS_SPAWN;
		newStateTime	= 0;

		OnNewStateCompleted( currentState );
	}

	bool alive			= owner->GetHealth() > 0;

	bool wantsToCrouch	= !driver || flags.manualCrouch || !alive || owner->IsEMPed();

	switch ( currentState ) {
		case CS_SPAWN:
			SetupNextState( CS_SHUTDOWN, gameLocal.time, 50 );
			break;

		case CS_SHUTDOWN:
			if ( !wantsToCrouch ) {
				SetupNextState( CS_POSED, gameLocal.time, 50 );
			}
			break;

		case CS_STAMP:
		case CS_STOMP_END:
		case CS_TURN_LEFT:
		case CS_TURN_RIGHT:
		case CS_WALK_RIGHT_LEG_STOP:
		case CS_WALK_LEFT_LEG_STOP:
		case CS_POSED:
		case CS_STAND:
		case CS_WALK_BACK_LEFT_LEG_STOP:
		case CS_WALK_BACK_RIGHT_LEG_STOP:
		case CS_LAND:
			if ( walker->GetMonsterPhysics().OnGround() ) {
				fallStartTime = 0;

				if ( wantsToCrouch ) {
					SetupNextState( CS_SHUTDOWN, gameLocal.time, 50 );
				} else {
					float forward = input->GetForward();
					if ( forward > 0.f && CheckWalk( 1.f ) ) {
						if ( flags.startOnLeftLeg ) {
							SetupNextState( CS_WALK_LEFT_LEG_START, gameLocal.time, 50 );
						} else {
							SetupNextState( CS_WALK_RIGHT_LEG_START, gameLocal.time, 50 );
						}
					} else if ( forward < 0.f && CheckWalk( -1.f ) ) {
						if ( flags.startOnLeftLeg ) {
							SetupNextState( CS_WALK_BACK_LEFT_LEG_START, gameLocal.time, 50 );
						} else {
							SetupNextState( CS_WALK_BACK_RIGHT_LEG_START, gameLocal.time, 50 );
						}
					} else if ( flags.turnOnSpot ) {
						float right = input->GetRight();
						if ( right > 0.f ) {
							SetupNextState( CS_TURN_RIGHT, gameLocal.time, 50 );
						} else if ( right < 0.f ) {
							SetupNextState( CS_TURN_LEFT, gameLocal.time, 50 );
						}
					}

					if ( input->GetUp() > 0.f ) {
						SetupNextState( CS_STAMP, gameLocal.time, 50 );
					}
				}
			} else {
				if ( fallStartTime == 0 ) {
					fallStartTime = gameLocal.time;
				}

				if ( ( gameLocal.time - fallStartTime ) > SEC2MS( 0.33f ) ) {
					fallStartTime = 0;
					SetupNextState( CS_FALL, gameLocal.time, 50 );
				}
			}
			break;

		case CS_FALL:
			if ( walker->GetMonsterPhysics().OnGround() ) {
				SetupNextState( CS_LAND, gameLocal.time, 50 );
			}
			break;

		case CS_WALK_RIGHT_LEG_START:
		case CS_WALK_LEFT_LEG: {
			float forward = input->GetForward();
			if ( wantsToCrouch || forward <= 0.f || !CheckWalk( 1.f ) ) {
				SetupNextState( CS_WALK_RIGHT_LEG_STOP, gameLocal.time, 50 );
			} else {
				SetupNextState( CS_WALK_RIGHT_LEG, gameLocal.time, 50 );
			}
			break;
		}

		case CS_WALK_LEFT_LEG_START:
		case CS_WALK_RIGHT_LEG: {
			float forward = input->GetForward();
			if ( wantsToCrouch || forward <= 0.f || !CheckWalk( 1.f ) ) {
				SetupNextState( CS_WALK_LEFT_LEG_STOP, gameLocal.time, 50 );
			} else {
				SetupNextState( CS_WALK_LEFT_LEG, gameLocal.time, 50 );
			}
			break;
		}

		case CS_WALK_BACK_LEFT_LEG_START:
		case CS_WALK_BACK_LEFT_LEG: {
			float forward = input->GetForward();
			if ( wantsToCrouch || forward >= 0.f || !CheckWalk( -1.f ) ) {
				SetupNextState( CS_WALK_BACK_RIGHT_LEG_STOP, gameLocal.time, 50 );
			} else {
				SetupNextState( CS_WALK_BACK_RIGHT_LEG, gameLocal.time, 50 );
			}
			break;
		}

		case CS_WALK_BACK_RIGHT_LEG_START:
		case CS_WALK_BACK_RIGHT_LEG: {
			float forward = input->GetForward();
			if ( wantsToCrouch || forward >= 0.f || !CheckWalk( -1.f ) ) {
				SetupNextState( CS_WALK_BACK_LEFT_LEG_STOP, gameLocal.time, 50 );
			} else {
				SetupNextState( CS_WALK_BACK_LEFT_LEG, gameLocal.time, 50 );
			}
			break;
		}

		case CS_STOMP_START: {
			SetupNextState( CS_STOMP_END, gameLocal.time, 50 );
			break;
		}
	}
}

/*
================
sdWalkerControl::Move
================
*/
void sdWalkerControl::Move( void ) {
	if ( flags.powered && !IsTurningOnSpot() ) {
		float lerpScale;
		if ( walker->GetMonsterPhysics().OnGround() ) {
			 lerpScale = 0.05f;
		} else {
			 lerpScale = 0.1f;
		}

		float turning = 0.f;
		if ( input->GetRight() ) {
			turning = MS2SEC( gameLocal.msec ) * turnRate;

			if ( input->GetRight() > 0.f ) {
				turning *= -1;
			}

			if ( MovingBackwards() ) {
				turning *= -1;
			}
		}
		turnScale = turnScale + ( ( turning - turnScale ) * lerpScale );

		TurnToward( currentYaw + turnScale );
	} else {
		turnScale = 0.f;
	}

	if ( input->GetUp() > 0.f ) {
		flags.manualCrouch = false;
	} else if( input->GetUp() < 0.f ) {
		flags.manualCrouch = true;
	}

	idVec3 delta;
	walker->GetMoveDelta( delta );
	walker->SetDelta( delta );

	Turn();
}

/*
================
sdWalkerControl::SetYaw
================
*/
void sdWalkerControl::SetYaw( float yaw ) {
	float oldIdealYaw = idealYaw;
	idMat3 temp;
	idAngles::YawToMat3( yaw, temp );
	owner->SetAxis( temp );
	idealYaw = oldIdealYaw;
}

/*
================
sdWalkerControl::UpdateSlidingFoot
================
*/
void sdWalkerControl::UpdateSlidingFoot( jointHandle_t joint, bool& footOnGround, idVec3& lastFootOrg, 
										   idVec3& lastFootGroundOrg, int& lastFootEffectTime ) {

	bool wasOnGround = footOnGround;
	footOnGround = false;

	idVec3 footOrg;
	walker->GetWorldOrigin( joint, footOrg );

	// check if the foot is on the ground
	idVec3 traceStart = footOrg;
	idVec3 traceEnd = footOrg;
	traceStart.z += 20.0f;
	traceEnd.z -= 10.0f;

	trace_t		traceObject;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS traceObject, traceStart, traceEnd, MASK_SOLID | CONTENTS_WATER | MASK_OPAQUE, owner );
	traceEnd = traceObject.endpos;

	if ( traceObject.fraction < 1.0f ) {
		float traceDist = traceObject.fraction * 30.0f - 20.0f;

		idBounds checkBounds( traceEnd );
		checkBounds.ExpandSelf( 8.0f );
		const idClipModel* waterModel;
		int found = gameLocal.clip.FindWater( CLIP_DEBUG_PARMS checkBounds, &waterModel, 1 );
		int cont = 0;
		if ( found ) {
			cont = gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS traceEnd, NULL, mat3_identity, CONTENTS_WATER, waterModel, waterModel->GetOrigin(), waterModel->GetAxis() );
		}

		if ( !cont ) {
			idVec3 footGroundOrg = traceEnd;
			bool playEffect = false;

			// on the ground and not in water
			const idVec3& groundNormal = walker->GetMonsterPhysics().GetGroundNormal();
			if ( wasOnGround ) {
				// find the movement on the ground plane
				idVec3 groundDelta = footGroundOrg - lastFootGroundOrg;
				groundDelta -= ( groundDelta * groundNormal ) * groundNormal;

				if ( groundDelta.LengthSqr() > 25.0f ) {
					playEffect = true;
				}
			} else {
				// find the movement towards the ground plane
				float towardsGround = ( lastFootOrg - footOrg ) * groundNormal;
				if ( towardsGround > 4.0f && towardsGround < 100.0f ) {
					playEffect = true;
				}
			}

			if ( playEffect && lastFootEffectTime < gameLocal.time - 150 ) {
				const char* surfaceTypeName = NULL;
				if ( traceObject.c.surfaceType ) {
					surfaceTypeName = traceObject.c.surfaceType->GetName();
				}

				idVec3 colorWhite(1.f,1.f,1.f);
				idVec3 xaxis(-1.f, 0.f, 0.f);
				walker->PlayEffect( "fx_ground_walk", colorWhite, surfaceTypeName, traceEnd, xaxis.ToMat3() );
				lastFootEffectTime = gameLocal.time;
			}

			footOnGround = true;
			lastFootGroundOrg = footGroundOrg;
		}
	}

	lastFootOrg = footOrg;
}

/*
================
sdWalkerControl::UpdateSliding
================
*/
void sdWalkerControl::UpdateSliding() {
	UpdateSlidingFoot( leftFootJoint, leftFootOnGround, lastLeftFootOrg, lastLeftFootGroundOrg, lastLeftFootEffectTime );
	UpdateSlidingFoot( rightFootJoint, rightFootOnGround, lastRightFootOrg, lastRightFootGroundOrg, lastRightFootEffectTime );
}

/*
================
sdWalkerControl::ApplyNetworkState
================
*/
void sdWalkerControl::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdWalkerNetworkData );

		idPlayer* localViewer = gameLocal.GetLocalViewPlayer();
		if ( localViewer == NULL || walker->GetPositionManager().FindDriver() != localViewer ) {
			idealYaw			= newData.idealYaw;
		}

		SetYaw( newData.currentYaw );

		turnScale			= newData.turnScale;
		flags.manualCrouch	= newData.crouching;
	}
}

/*
================
sdWalkerControl::ReadNetworkState
================
*/
void sdWalkerControl::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdWalkerNetworkData );

		newData.idealYaw	= msg.ReadDeltaFloat( baseData.idealYaw );
		newData.currentYaw	= msg.ReadDeltaFloat( baseData.currentYaw );
		newData.turnScale	= msg.ReadDeltaFloat( baseData.turnScale );
		newData.crouching	= msg.ReadBool();
	}
}

/*
================
sdWalkerControl::WriteNetworkState
================
*/
void sdWalkerControl::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdWalkerNetworkData );

		newData.idealYaw	= idealYaw;
		newData.currentYaw	= currentYaw;
		newData.turnScale	= turnScale;
		newData.crouching	= flags.manualCrouch;

		msg.WriteDeltaFloat( baseData.idealYaw, newData.idealYaw );
		msg.WriteDeltaFloat( baseData.currentYaw, newData.currentYaw );
		msg.WriteDeltaFloat( baseData.turnScale, newData.turnScale );
		msg.WriteBool( newData.crouching );
	}
}

/*
================
sdWalkerControl::CheckNetworkStateChanges
================
*/
bool sdWalkerControl::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdWalkerNetworkData );

		if ( baseData.idealYaw != idealYaw ) {
			return true;
		}
		if ( baseData.currentYaw != currentYaw ) {
			return true;
		}
		if ( baseData.turnScale != turnScale ) {
			return true;
		}
		if ( baseData.crouching != flags.manualCrouch ) {
			return true;
		}
	}
	return false;
}

/*
================
sdWalkerControl::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdWalkerControl::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdWalkerNetworkData;
	}
	return NULL;
}

/*
================
sdWalkerControl::OnNetworkEvent
================
*/
void sdWalkerControl::OnNetworkEvent( int time, const idBitMsg& msg ) {
	controlState_t state	= ( controlState_t )msg.ReadBits( idMath::BitsForInteger( CS_NUM_STATES ) );
	SetupNextState( state, time, msg.ReadLong() );
}
