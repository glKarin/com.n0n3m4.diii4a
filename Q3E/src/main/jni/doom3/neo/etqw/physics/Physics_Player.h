// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__

/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

#include "Physics_Actor.h"

class sdLadderEntity;

extern const idBounds playerProneLegsBounds;

// movementFlags
const int PMF_DUCKED			= 0x00000001;		// set when ducking
const int PMF_JUMPED			= 0x00000002;		// set when the player jumped this frame
const int PMF_STEPPED_UP		= 0x00000004;		// set when the player stepped up this frame
const int PMF_STEPPED_DOWN		= 0x00000008;		// set when the player stepped down this frame
const int PMF_JUMP_HELD			= 0x00000010;		// set when jump button is held down
const int PMF_TIME_LAND			= 0x00000020;		// movementTime is time before rejump
const int PMF_TIME_KNOCKBACK	= 0x00000040;		// movementTime is an air-accelerate only time
const int PMF_TIME_WATERJUMP	= 0x00000080;		// movementTime is waterjump
const int PMF_PRONE				= 0x00000100;
const int PMF_CROUCH_HELD		= 0x00000200;		// set when crouch button is held down
const int PMF_ALL_TIMES			= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK);

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP,				// flying without collision detection nor gravity
} pmtype_t;

typedef enum {
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

enum proneResult_t {
	PR_FAILED						= BITT< 0 >::VALUE,
	PR_BREAK						= BITT< 1 >::VALUE,
	PR_NOGROUND						= BITT< 2 >::VALUE,
};

#define	MAXTOUCH					32

typedef struct playerPState_s {
	idVec3					origin;
	idVec3					velocity;
	idVec3					pushVelocity;
	float					stepUp;
	float					proneOffset;
	int						movementType;
	int						movementFlags;
	int						movementTime;
} playerPState_t;

class sdPlayerPhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdPlayerPhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					origin;
	idVec3					velocity;
	int						movementTime;
	int						movementFlags;
};

class sdPlayerPhysicsBroadcastData : public sdEntityStateNetworkData {
public:
							sdPlayerPhysicsBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					pushVelocity;
	idVec3					localOrigin;
	bool					frozen;
	int						proneChangeEndTime;
	int						jumpAllowedTime;
};

class idPhysics_Player : public idPhysics_Actor {
public:
	CLASS_PROTOTYPE( idPhysics_Player );

	enum proneTimes_t { PT_STAND_TO_PRONE, PT_CROUCH_TO_PRONE, PT_PRONE_TO_CROUCH, PT_PRONE_TO_STAND, PT_MAX };

							idPhysics_Player( void );
	virtual					~idPhysics_Player( void );

							// initialisation
	void					SetSpeed( float newWalkSpeedFwd, float newWalkSpeedBack, float newWalkSpeedSide, float newCrouchSpeed, float newProneSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	void					SetPlayerInput( const usercmd_t& cmd, const idAngles &newViewAngles, bool allowMovement );
	void					SetKnockBack( const int knockBackTime );
							// feed back from last physics frame
	const idVec3&			PlayerGetOrigin( void ) const;	// != GetOrigin

	void					SetupPlayerClipModels();
	void					SetupVehicleClipModels( float playerHeight );

	virtual int				VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount );

	static void				SetupUsercmdForDirection( const idVec2 &dir, float forwardSpeed, float backwardSpeed, float sideSpeed, usercmd_t &cmd );

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					SetContents( int contents, int id = -1 );
	idClipModel*			GetClipModel( int id = 0 ) const;

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	static void				CalcSpectateBounds( idBounds& bounds );
	static void				CalcNormalBounds( idBounds& bounds );

	const idVec3&			GetGroundNormal( void ) const { return groundTrace.c.normal; }
	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	void					SetMaster( idEntity *master, const bool orientated = true );

	virtual const idVec3&	GetOrigin( int id = -1 ) const;

	virtual void			SetSelf( idEntity *e );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual float			InWater( void ) const { return waterFraction; }

	waterLevel_t			GetWaterLevel( void ) const { return waterLevel; }
	bool					HasJumped( void ) const { return ( ( current.movementFlags & PMF_JUMPED ) != 0 ); }
	bool					HasSteppedUp( void ) const { return ( ( current.movementFlags & ( PMF_STEPPED_UP | PMF_STEPPED_DOWN ) ) != 0 ); }
	float					GetStepUp( void ) const { return current.stepUp; }
	bool					IsCrouching( void ) const { return ( ( current.movementFlags & PMF_DUCKED ) != 0 ); }
	bool					IsProne( void ) const { return ( ( current.movementFlags & PMF_PRONE ) != 0 ); }
	bool					OnLadder( void ) const;
	idVec3					GetLadderNormal( void ) const;
	void					SetFrozen( bool _frozen ) { frozen = _frozen; }
	bool					IsFrozen( void ) const { return frozen; }
	bool					IsGrounded( void ) const { return groundPlane; }

	bool					IsLeaning( void ) const { return ( leanOffset != 0.0f ) ? true : false; }

	void					EnterProne( void );
	bool					LeaveProne( bool crouch );
	void					ResetProne( void );

	void					CheckLean( bool allow );

	void					SetGroundAccel( float value ) { pm_accelerate = value; }
	void					SetAirAccel( float value ) { pm_airAccelerate = value; }
	void					SetWaterAccel( float value ) { pm_waterAccelerate = value; }
	void					SetFlyAccel( float value ) { pm_flyAccelerate = value; }

	float					GetCurrentAimSpeed( void ) const { return playerSpeed; }

	void					SetGroundFriction( float value ) { pm_friction = value; }
	void					SetAirFriction( float value ) { pm_airFriction = value; }
	void					SetWaterFriction( float value ) { pm_waterFriction = value; }
	void					SetFlyGroundFriction( float value ) { pm_flyFriction = value; }
	void					SetNoclipFriction( float value ) { pm_noclipFriction = value; }


	idVec3					GetProneLegsPos( const idVec3& startOrg, const idAngles& angles ) const;
	const idVec3&			GetProneOffset( void ) const { return proneModelOffset; }
	int						ProneCheck( const idVec3& startOrg, const idAngles& angles, float* offset = NULL, idVec3* modelOffset = NULL ) const;
	int						GetProneChangeEndTime( void ) const { return proneChangeEndTime; }

	void					SetProneTime( proneTimes_t proneTime, int valueMS ) { assert( proneTime < PT_MAX ); proneTimes[ proneTime ] = valueMS; }
	int						GetProneTime( proneTimes_t proneTime ) { assert( proneTime < PT_MAX ); return proneTimes[ proneTime ]; }

	float					GetLeanOffset( void ) const { return leanOffset; }

	bool					TryProne( void );

	void					UpdateBounds( void );

	void					UpdateCollisionMerge( void );
	void					ResetCollisionMerge( const idVec3& origin ); 

	const idClipModel*		GetNormalClipModel( void ) const { return clipModel_normal; }
	const idClipModel*		GetCrouchClipModel( void ) const { return clipModel_crouch; }
	const idClipModel*		GetProneClipModel( void ) const { return clipModel_prone; }
	const idClipModel*		GetProneLegsClipModel( void ) const { return proneLegsClipModel; }
	const idClipModel*		GetHeadClipModel( void ) const { return headClipModel; }

	void					EnableHeadClipModel( void );
	void					DisableHeadClipModel( void );

	// getters for antilag
	float					GetGroundAccel( void ) const { return pm_accelerate; }
	float					GetAirAccel( void ) const { return pm_airAccelerate; }
	float					GetGroundFriction( void ) const { return pm_friction; }
	float					GetAirFriction( void ) const { return pm_airFriction; }
	float					GetWaterFriction( void ) const { return pm_waterFriction; }
	float					GetStopSpeed( void ) const { return pm_stopSpeed; }
	float					GetSwimScale( void ) const { return pm_swimScale; }
	float					GetWalkSpeedFwd( void ) const { return walkSpeedFwd; }
	float					GetWalkSpeedBack( void ) const { return walkSpeedBack; }
	float					GetWalkSpeedSide( void ) const { return walkSpeedSide; }
	idAngles				GetViewAngles( void ) const { return viewAngles; }
	float					GetLastSnapshotTime( void ) const { return lastSnapshotTime; }

protected:
	// player physics state
	playerPState_t			current;
	playerPState_t			saved;
	idVec3					localOrigin;

	// collision merging state & flags
	idVec3					lastClippedOrigin;
	int						numMergedFrames;
	bool					mergeThisFrame;
	bool					skipContactsThisFrame;

	// properties
	float					walkSpeedFwd;
	float					walkSpeedBack;
	float					walkSpeedSide;
	float					crouchSpeed;
	float					proneSpeed;
	float					maxStepHeight;
	float					maxJumpHeight;

	idClipModel*			proneLegsClipModel;
	idClipModel*			shotClipModel;
	idClipModel*			headClipModel;

	void					BuildClipModel( const idBounds& bounds, bool useCylinder, idClipModel*& model );
	idClipModel*			clipModel_normal;
	idClipModel*			clipModel_normalShot;
	idClipModel*			clipModel_crouch;
	idClipModel*			clipModel_crouchShot;
	idClipModel*			clipModel_prone;
	idClipModel*			clipModel_proneShot;
	idClipModel*			clipModel_dead;
	idClipModel*			clipModel_deadShot;
	idClipModel*			clipModel_vehicle;
	idClipModel*			clipModel_vehicleShot;
	idClipModel*			groundCheckModel;

	idPlayer*				playerSelf;

	int						proneChangeEndTime;

	// player input
	usercmd_t				command;
	idAngles				viewAngles;
	bool					movementAllowed;

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	idVec3					viewForward;
	idVec3					viewRight;
	float					leanFraction;
	float					leanOffset;

	// walk movement
	bool					walking;
	bool					groundPlane;
	trace_t					groundTrace;
	const idMaterial*		groundMaterial;

	idVec3					proneModelOffset;

	// ladder movement
	idEntityPtr< sdLadderEntity > ladder;

	// results of last evaluate
	waterLevel_t			waterLevel;
	float					waterFraction;
	float					waterHeightAboveGround;

	//
	bool					frozen;

	float 					pm_stopSpeed;
	float 					pm_swimScale;
	float 					pm_ladderSpeed;
	float 					pm_stepScale;

	float 					pm_accelerate;
	float 					pm_airAccelerate;
	float 					pm_waterAccelerate;
	float 					pm_flyAccelerate;

	float 					pm_friction;
	float 					pm_airFriction;
	float 					pm_waterFriction;
	float 					pm_flyFriction;
	float 					pm_noclipFriction;
	
	int						proneTimes[ PT_MAX ];

	int						jumpAllowedTime;

	int						lastSnapshotTime;

public:
	static idVec3			AdjustVertically( const idVec3 &normal, const idVec3 &_in );

	static void				CalcDesiredWalkMove( int fwd, int right, float walkSpeedFwd, float walkSpeedSide, float walkSpeedBack, idVec2& output );

protected:
	bool					CanCrouch( void );
	bool					CanProne( void );
	float					CmdScale( const usercmd_t& cmd, bool noVertical ) const;
	idVec2					CalcDesiredWalkMove( const usercmd_t& cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, int vehiclePush );
	void					Friction( void );
	void					WaterJumpMove( void );
	virtual void			WaterMove( void );
	virtual void			FlyMove( void );
	virtual void			AirMove( bool allowLean );
	virtual void			WalkMove( bool allowLean );
	virtual void			DeadMove( void );
	virtual void			NoclipMove( void );
	virtual void			SpectatorMove( void );
	virtual void			LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	void					FinishProneChange( void );
	virtual void			CheckGround( void );
	virtual void			CheckStance( void );
	sdLadderEntity*			FindLadder( const idVec3& origin, sdLadderEntity** ladders, int numLadders, const idVec3& direction );
	virtual void			CheckLadder( void );
	virtual bool			CheckJump( void );
	virtual bool			CheckWaterJump( void );
	void					SetWaterLevel( void );
	void					DropTimers( void );
	void					MovePlayer( int msec );
};

#endif /* !__PHYSICS_PLAYER_H__ */
