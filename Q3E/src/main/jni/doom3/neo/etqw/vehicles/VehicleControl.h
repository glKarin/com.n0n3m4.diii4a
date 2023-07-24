// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_VEHICLES_VEHICLECONTROL_H__
#define __GAME_VEHICLES_VEHICLECONTROL_H__

#include "Walker.h"

class sdTransport;
class sdVehicleInput;
class sdVehicleDriveObject;

/*
===============================================================================

	sdVehicleControlBase - this is mostly an interface for specialised versions

	This acts like the brain of the vehicle - it handles the state machine,
	the engine & gearbox manipulation, and translation of player input to
	physics control values

===============================================================================
*/
class sdVehicleControlBase {
public:
	sdVehicleControlBase() { 
		owner = NULL; 
		input = NULL;
		isImmobilized = false;
	}
	virtual ~sdVehicleControlBase() {}

	virtual void			Init( sdTransport* transport );
	void					SetInput( sdVehicleInput* newInput ) { input = newInput; }
	virtual void			Update() = 0;

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition ) { }
	virtual void			OnPlayerExited( idPlayer* player, int position ) { }
	virtual void			OnEMPStateChanged( void ) {}
	virtual void			OnWeaponEMPStateChanged( void ) {}
	virtual void			OnTeleport( void ) {}

	virtual void			OnPostDamage( idEntity* attacker, int oldHealth, int newHealth ) { }

	virtual void			SetImmobilized( bool immobile ) { isImmobilized = immobile; }
	virtual bool			IsImmobilized( void ) const { return isImmobilized; }

	virtual bool			GetLandingGearDown() { return true; }
	virtual bool			IsGrounded( void ) const { return true; }
	virtual bool			IsLanding( void ) const { return true; }

	virtual bool			InSiegeMode( void ) const { return false; }
	virtual bool			WantsSiegeMode( void ) const { return false; }
	virtual void			SetSiegeMode( bool siege ) {}
	virtual void			CancelSiegeMode( void ) {}

	virtual int				GetLandingChangeTime( void ) const { return 0; }
	virtual int				GetLandingChangeEndTime( void ) const { return 0; }
	virtual int				GetMinDisplayHealth( void ) const { return 0; }

	virtual void			OnYawChanged( float newYaw ) { ; }

	virtual void			SetupComponents( void ) { ; }

	virtual void			OnNetworkEvent( int time, const idBitMsg& msg ) { assert( false ); }

	virtual bool			IsNetworked() { return false; }
	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) { ; }
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const { ; }
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const { ; }
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const { return false; }
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const { return NULL; }
	virtual void			ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) { ; }

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) { return false; }
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {}

	virtual void			SetDeathThroeHealth( int amount ) {}

	virtual bool			IgnoreCollisionDamage( const idVec3& direction ) const { return false; }

	// special overrides & modifiers for use when careening
	virtual float			GetCareeningCollideScale() const { return 1.0f; }
	virtual float			GetCareeningRollAmount() const { return 0.0f; }
	virtual float			GetCareeningPitchAmount() const { return 0.0f; }
	virtual float			GetCareeningYawAmount() const { return 0.0f; }
	virtual float			GetCareeningLiftScale() const { return 0.0f; }
	virtual float			GetCareeningCollectiveAmount() const { return 0.0f; }

	virtual float			GetHudSpeed( void ) const { return owner->GetPhysics()->GetLinearVelocity().Length(); }

	virtual float			GetDeadZoneFraction( void ) const { return 0.0f; }

	// physics callbacks
	virtual bool			IsSquisher( void ) const { return false; }
	virtual bool			RestNeedsGround( void ) const { return true; }

	// teleporting callbacks
	virtual bool			TeleportOntoRamp( void ) const { return false; }
	virtual float			TeleportOntoRampHeightOffset( void ) const { return 0.0f; }
	virtual bool			TeleportOntoRampOriented( void ) const { return false; }
	virtual bool			TeleportBackTraceOriented( void ) const { return true; }
	virtual bool			TeleportCanBeFragged( void ) const { return true; }


protected:

	sdTransport*			owner;
	sdVehicleInput*			input;

	bool					isImmobilized;

	float					drownHeight;
};

/*
===============================================================================

	sdVehicleScriptControl
	
	this just passes the control updates through to script

===============================================================================
*/
class sdVehicleScriptControl : public sdVehicleControlBase {
public:
	sdVehicleScriptControl() { 
		inputThread = NULL; 
		inputThreadFunc = NULL;
	}

	virtual ~sdVehicleScriptControl();

	virtual void			Init( sdTransport* transport );
	virtual void			Update();

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

protected:
	int								inputMode;
	sdProgramThread*			inputThread;
	const sdProgram::sdFunction*	inputThreadFunc;
};

/*
===============================================================================

	sdVehicleControl

===============================================================================
*/
class sdVehicleControl : public sdVehicleControlBase {
public:
	virtual void			Init( sdTransport* transport );
	virtual void			Update();

protected:
	enum controlState_t {
		CS_SHUTDOWN,
		CS_POWERED
	};

	virtual void			HandlePhysics();
	virtual void			RunStateMachine();
	virtual void			HandleStateChange( controlState_t s );
	virtual bool			EngineRunning();
	virtual void			SetupInput() {}

	void					UpdateOverdriveSound( bool thrusting );

	controlState_t			state;
	controlState_t			newState;
	int						newStateTime;

	int						overDriveSoundEndTime;
	float					overDrivePitchStart;
	float					overDrivePitchEnd;
	float					overDrivePitchStartSpeed;
	float					overDrivePitchEndSpeed;
};

/*
===============================================================================

	sdDesecratorControl

===============================================================================
*/
class sdDesecratorControl : public sdVehicleControl {
public:
	virtual void			Init( sdTransport* transport );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );

	virtual bool			InSiegeMode( void ) const { return inSiegeMode; }
	virtual bool			WantsSiegeMode( void ) const { return wantsSiegeMode; }
	virtual void			SetSiegeMode( bool siege ) { inSiegeMode = siege; }
	virtual void			CancelSiegeMode( void ) { wantsSiegeMode = false; }

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

	virtual void			RunStateMachine();

	virtual bool			IsSquisher( void ) const { return true; }
	virtual bool			RestNeedsGround( void ) const { return !inSiegeMode; }

	virtual bool			TeleportOntoRamp( void ) const { return true; }
	virtual float			TeleportOntoRampHeightOffset( void ) const { return 64.0f; }
	virtual bool			TeleportOntoRampOriented( void ) const { return true; }

protected:

	virtual void			SetupInput();

	virtual void			UpdateEffects();

	int						lastGroundEffectsTime;
	bool					wantsSiegeMode;
	bool					inSiegeMode;
};

/*
===============================================================================

	sdWheeledVehicleControl

===============================================================================
*/
class sdWheeledControlNetworkData : public sdEntityStateNetworkData {
public:
							sdWheeledControlNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	float					desiredDirection;
	float					steerVisualAngle;
};

class sdWheeledVehicleControl : public sdVehicleControl {
public:
	virtual void			Init( sdTransport* transport );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnTeleport( void );

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

	virtual float			GetHudSpeed( void ) const;


	virtual bool			IsNetworked() { return true; }
	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual bool			TeleportOntoRamp( void ) const { return true; }
	virtual bool			TeleportOntoRampOriented( void ) const { return true; }

protected:
	virtual void			SetupInput();

	virtual void			UpdateControls();
	virtual void			UpdateCareening( idVec3& directions );
	virtual void			UpdateDirectionBraking( idVec3& directions, idVec3& vel, float speedKPH, bool& braking );
	virtual void			UpdateHandbrake( idVec3& directions, float speedKPH, bool& handbraking, bool& braking );
	virtual float			SelectGear( idVec3& directions, float absSpeedKPH );
	virtual void			HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking );
	virtual bool			CanThrust( idVec3& directions, bool braking, bool handbraking );
	virtual void			SetSpeed( idVec3& directions, float gearSpeed, float gearForce, bool braking, bool handbraking );

	virtual float			UpdateSteering( idPlayer* driver, idVec3& directions, float speedKPH, float absSpeedKPH, bool handbraking );
	void					UpdateBrakingSound( float speedKPH, bool braking );

	void					StartCareening();
	void					StopCareening();
	bool					AutoBrake();
	bool					CanEmptyBrake( float speedKPH );

	float					oldSpeedKPH;
	int						nextBrakeSoundTime;

	float					overDriveFactor;

	// gears
	const idDeclTable*		gearForceTable;
	const idDeclTable*		gearSpeedTable;
	float					powerCurveScale;

	// steering
	float					steeringAngle;

	// simple steering
	float					forwardSteerSpeed;
	float					reverseSteerSpeed;
	float					minSteerCenteringSpeed;
	float					maxSteerCenteringSpeed;
	float					airSteerCenteringSpeed;
	float					maxSteerCenteringThreshold;
	float					reverseSteerAngleScale;


	float					desiredDirection;

	bool					keySteering;
};

/*
===============================================================================

	sdTitanControl

===============================================================================
*/
class sdTitanControl : public sdWheeledVehicleControl {
public:
	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual bool			IsSquisher( void ) const { return true; }

protected:
	virtual void			UpdateCareening( idVec3& directions );
	virtual void			UpdateDirectionBraking( idVec3& directions, idVec3& vel, float speedKPH, bool& braking );
	virtual void			UpdateHandbrake( idVec3& directions, float speedKPH, bool& handbraking, bool& braking );
	virtual float			SelectGear( idVec3& directions, float absSpeedKPH );
	virtual void			HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking );
	virtual void			SetSpeed( idVec3& directions, float gearSpeed, float gearForce, bool braking, bool handbraking );
	virtual float			UpdateSteering( idPlayer* driver, idVec3& directions, float speedKPH, float absSpeedKPH, bool handbraking );
};

/*
===============================================================================

	sdTrojanControl

===============================================================================
*/

class sdVehicleThruster;
class sdVehicleAirBrake;

class sdTrojanControl : public sdWheeledVehicleControl {
public:
	virtual void			Init( sdTransport* transport );

protected:
	struct propeller_t {
		sdVehicleThruster*	object;
		jointHandle_t		joint;
		float				speed;
		float				angle;
		float				maxSpeed;
	};

	virtual void			Update();

	virtual void			HandleEmpty( idVec3& vel, float absSpeedKPH, bool& braking );
	virtual void			UpdateControls();
	virtual void			UpdateCareening( idVec3& directions );

	void					UpdatePropeller( propeller_t& prop );
	
	propeller_t				leftProp;
	propeller_t				rightProp;
};

/*
===============================================================================

	sdPlatypusControl

===============================================================================
*/
class sdPlatypusControl : public sdTrojanControl {
public:
	virtual void			Init( sdTransport* transport );

protected:
	virtual void			CalculateSteering( idVec3& directions, float absSpeedKPH, float& desiredSteerAngle, float& steeringFactor );
	virtual void			ApplySteeringMods( float& desiredSteerAngle, float& steeringFactor );

	virtual void			SetupInput();

	float					thrustScale;

	// steering
	float					steeringSpeedScale;
	float					steeringSpeedMin;
	float					steeringSpeedMax;
	float					steeringReturnFactor;
	float					steeringRampPower;
	float					steeringRampOffset;
};

/*
===============================================================================

	sdHogControl

===============================================================================
*/
class sdHogControl : public sdWheeledVehicleControl {
public:
	virtual void			Init( sdTransport* transport );
	virtual void			Update();

	virtual bool			IgnoreCollisionDamage( const idVec3& direction ) const;

protected:
	virtual bool			CanThrust( idVec3& directions, bool braking, bool handbraking );

	bool					ramming;
	float					ramDamageScale;
	float					hitDamageScale;

	rvClientEntityPtr< sdClientAnimated > ramModel;
};

/*
===============================================================================

	sdHuskyControl

===============================================================================
*/
class sdHuskyControl : public sdWheeledVehicleControl {
public:
	virtual void			Init( sdTransport* transport );
	virtual void			Update();

protected:

	jointHandle_t			handleBars;
};

/*
===============================================================================

	sdMCPControl

===============================================================================
*/
class sdMCPControl : public sdTitanControl {

	virtual bool			TeleportCanBeFragged( void ) const { return false; }

protected:
	virtual bool			CanThrust( idVec3& directions, bool braking, bool handbraking ) { return false; }
};

/*
===============================================================================

	sdAirVehicleControl
	
	Base class for air vehicle control

===============================================================================
*/
class sdAirVehicleControl : public sdVehicleControlBase {
public:
	sdAirVehicleControl();

	virtual void			Init( sdTransport* transport );
	virtual void			Update();
	virtual void			SetupComponents( void );

	virtual bool			GetLandingGearDown() { return landingGearDown; }

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

	virtual void			SetDeathThroeHealth( int amount ) { spiralHealth = amount; }
	virtual int				GetMinDisplayHealth( void ) const { return spiralHealth; }

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnPlayerExited( idPlayer* player, int position );
	virtual void			OnPostDamage( idEntity* attacker, int oldHealth, int newHealth );

	virtual float			GetCareeningCollideScale() const;
	virtual float			GetCareeningRollAmount() const;
	virtual float			GetCareeningPitchAmount() const;
	virtual float			GetCareeningYawAmount() const;
	virtual float			GetCareeningLiftScale() const;
	virtual float			GetCareeningCollectiveAmount() const;

	virtual bool			IsGrounded( void ) const { return isGrounded; }
	virtual bool			IsLanding( void ) const { return isLanding; }
	virtual int				GetLandingChangeTime( void ) const { return landingGearChangeTime; }
	virtual int				GetLandingChangeEndTime( void ) const { return landingGearChangeEndTime; }

	virtual float			GetDeadZoneFraction( void ) const { return deadZoneFraction; }

protected:

	void					SetupInput();

	void					RunStateMachine();
	void					HandlePhysics();
	bool					EngineRunning();

	virtual void			UpdateEffects( const idVec3& absMins, const trace_t& traceObject ) {}
	virtual bool			IsContacting( const idVec3& absMins, const trace_t& traceObject );
	virtual void			UpdateLandingGear( const idVec3& directions ) {}

	bool					throttling;
	bool					landingGearDown;
	
	float					landingThresholdDistance;
	float					landingThresholdSpeed;
	
	float					overDriveFactor;
	bool					overDrivePlayingSound;

	float					collective;
	float					collectiveMin;
	float					collectiveMax;
	float					collectiveRate;
	float					collectiveDefault;

	int						lastThrusterEffectsTime;

	int						lastDriverTime;

	bool					isGrounded;
	bool					isLanding;
	int						landingGearChangeTime;
	int						landingGearChangeEndTime;
	float					deadZoneFraction;
	
	sdVehicleThruster*		leftJet;
	sdVehicleThruster*		rightJet;
	sdVehicleAirBrake*		airBrake;
	
	jointHandle_t			leftThrustEffectJoint;
	jointHandle_t			rightThrustEffectJoint;

	int						spiralHealth;

	bool					noThrusters;

	float					height;

	idBounds				mainBounds;

	// new steering stuff
	idAngles				oldCmdAngles;

	// careening input parms
	float					careenHeight;
	float					careenSpeed;
	float					careenYaw;
	float					careenRoll;
	float					careenPitch;
	float					careenPitchExtreme;
	float					careenLift;
	float					careenLiftExtreme;
	float					careenCollective;
	int						careenCruiseTime;
};

/*
===============================================================================

	sdHornetControl
	
===============================================================================
*/
class sdHornetControl : public sdAirVehicleControl {
public:
	virtual void			Init( sdTransport* transport );

	virtual void			OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void			OnEMPStateChanged( void );
	virtual void			OnWeaponEMPStateChanged( void ) {}

protected:
	virtual void			UpdateEffects( const idVec3& absMins, const trace_t& traceObject );
	virtual bool			IsContacting( const idVec3& absMins, const trace_t& traceObject );
	virtual void			UpdateLandingGear( const idVec3& directions );

	bool					groundEffects;
	float					groundEffectsThreshhold;
	int						lastGroundEffectsTime;

	int						landingAnimEndTime;
};

/*
===============================================================================

	sdHovercopterControl
	
===============================================================================
*/
class sdHovercopterControl : public sdAirVehicleControl {
public:
	virtual void			Init( sdTransport* transport );

protected:
	virtual void			UpdateEffects( const idVec3& absMins, const trace_t& traceObject );

	bool	 				downdraftPlaying;
	bool					groundEffects;
	float					groundEffectsThreshhold;
	int						lastGroundEffectsTime;
	jointHandle_t			mainJoint;
	jointHandle_t			leftJetJoint;
	jointHandle_t			rightJetJoint;
};

/*
===============================================================================

	sdAnansiControl
	
===============================================================================
*/
class sdAnansiControl : public sdHovercopterControl {
public:
	virtual void			Init( sdTransport* transport );

protected:
	virtual void			UpdateLandingGear( const idVec3& directions );

	int						landingAnimEndTime;
};

/*
===============================================================================

	sdWalkerControl
	
===============================================================================
*/
class sdWalkerNetworkData : public sdEntityStateNetworkData {
public:
										sdWalkerNetworkData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

public:
	float								idealYaw;
	float								currentYaw;
	float								turnScale;
	bool								crouching;
};

class sdWalkerControl : public sdVehicleControlBase {
public:
	virtual void			Init( sdTransport* transport );
	virtual void			Update();

	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
										const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

	enum controlState_t {
		CS_SPAWN,
		CS_SHUTDOWN,
		CS_POSED,
		CS_WALK_LEFT_LEG_START,
		CS_WALK_RIGHT_LEG_START,
		CS_WALK_LEFT_LEG,
		CS_WALK_RIGHT_LEG,
		CS_WALK_LEFT_LEG_STOP,
		CS_WALK_RIGHT_LEG_STOP,
		CS_WALK_BACK_LEFT_LEG_START,
		CS_WALK_BACK_RIGHT_LEG_START,
		CS_WALK_BACK_LEFT_LEG,
		CS_WALK_BACK_RIGHT_LEG,
		CS_WALK_BACK_LEFT_LEG_STOP,
		CS_WALK_BACK_RIGHT_LEG_STOP,
		CS_TURN_RIGHT,
		CS_TURN_LEFT,
		CS_STOMP_START,
		CS_STOMP_END,
		CS_STAMP,
		CS_STAND,
		CS_FALL,
		CS_LAND,
		CS_NUM_STATES,
	};

	bool					SetupNextState( controlState_t state, int time, int blendTime );
	bool					CheckStateExit( void );

	void					RunStateMachine( void );
	void					Move( void );

	bool					CheckWalk( float direction );

	void					SetPowered( bool value );

	void					OnNewStateBegun( controlState_t state );
	void					OnNewStateCompleted( controlState_t state );
	void					OnOldStateFinished( controlState_t state );

	void					SetYaw( float yaw );
	void					TurnToward( float yaw );
	void					Turn( void );
	virtual void			OnYawChanged( float newYaw );

	virtual bool			IsNetworked() { return true; }

	virtual void			OnNetworkEvent( int time, const idBitMsg& msg );

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual bool			InSiegeMode( void ) const { return !flags.powered; }

	virtual bool			TeleportOntoRamp( void ) const { return true; }
	virtual bool			TeleportBackTraceOriented( void ) const { return false; }

protected:

	void					UpdateSliding();
	void					UpdateSlidingFoot( jointHandle_t joint, bool& footOnGround, idVec3& lastFootOrg, 
												idVec3& lastFootGroundOrg, int& lastFootEffectTime );

	jointHandle_t			leftFootJoint;
	jointHandle_t			rightFootJoint;
	bool					leftFootOnGround;
	bool					rightFootOnGround;
	idVec3					lastLeftFootOrg;
	idVec3					lastRightFootOrg;
	idVec3					lastLeftFootGroundOrg;
	idVec3					lastRightFootGroundOrg;

	int						lastLeftFootEffectTime;
	int						lastRightFootEffectTime;

	int						fallStartTime;


	struct controlFlags_t {
		bool				powered			: 1;
		bool				turnOnSpot		: 1;
		bool				playStopAnim	: 1;
		bool				manualCrouch	: 1;
		bool				canStamp		: 1;
		bool				startOnLeftLeg	: 1;
	};

	float					turnRate;
	float					dynamicTurnRate;
	float					currentYaw;
	float					idealYaw;
	float					turnScale;

	sdWalker*				walker;

	controlFlags_t			flags;

	controlState_t			currentState;

	controlState_t			newState;
	int						newStateTime;
	int						newStateStartTime;

	float					walkingGroundPoundForce;
	float					walkingGroundPoundDamageScale;
	float					walkingGroundPoundRange;

	int						stateAnims[ CS_NUM_STATES ];
};

#endif // __GAME_VEHICLES_VEHICLECONTROL_H__
