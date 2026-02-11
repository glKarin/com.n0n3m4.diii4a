// Copyright (C) 2004 Id Software, Inc.
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

//HUMANHEAD START: All these moved here from physics_player.cpp
// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 3.0f;
const float PM_NOCLIPFRICTION	= 12.0f;

const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float OVERCLIP			= 1.001f;

// movementFlags
const int PMF_DUCKED			= 1;		// set when ducking
const int PMF_JUMPED			= 2;		// set when the player jumped this frame
const int PMF_STEPPED_UP		= 4;		// set when the player stepped up this frame
const int PMF_STEPPED_DOWN		= 8;		// set when the player stepped down this frame
const int PMF_JUMP_HELD			= 16;		// set when jump button is held down
const int PMF_TIME_LAND			= 32;		// movementTime is time before rejump
const int PMF_TIME_KNOCKBACK	= 64;		// movementTime is an air-accelerate only time
const int PMF_TIME_WATERJUMP	= 128;		// movementTime is waterjump
const int PMF_ALL_TIMES			= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK);
//HUMANHEAD END

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

typedef enum {
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

typedef struct playerPState_s {
	idVec3					origin;

	//HUMANHEAD: aob - needed for saving state and vehicles
	idMat3					axis;
	idMat3					localAxis;
	//HUMANHEAD END

	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;
} playerPState_t;

class idPhysics_Player : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( idPhysics_Player );

							idPhysics_Player( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetSpeed( const float newWalkSpeed, const float newCrouchSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	void					SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles );
	virtual	//HUMANHEAD: Made virtual
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
							// feed back from last physics frame
	waterLevel_t			GetWaterLevel( void ) const;
	int						GetWaterType( void ) const;
	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
	bool					OnLadder( void ) const;
	const idVec3 &			PlayerGetOrigin( void ) const;	// != GetOrigin

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

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	virtual	//HUMANHEAD: Made virtual
	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

protected:	//HUMANHEAD
	// player physics state
	playerPState_t			current;
	playerPState_t			saved;

	// properties
	float					walkSpeed;
	float					crouchSpeed;
	float					maxStepHeight;
	float					maxJumpHeight;
	int						debugLevel;				// if set, diagnostic output will be printed

	// player input
	usercmd_t				command;
	idAngles				viewAngles;

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	idVec3					viewForward;
	idVec3					viewRight;

	// walk movement
	bool					walking;
	bool					groundPlane;
//HUMANHEAD: aob - moved to idPhysics_Actor
//	trace_t					groundTrace;
//HUMANHEAD END
	const idMaterial *		groundMaterial;

	// ladder movement
	bool					ladder;
	idVec3					ladderNormal;

	// results of last evaluate
	waterLevel_t			waterLevel;
	int						waterType;

	idVec3		wishdir;	//HUMANHEAD

protected://HUMANHEAD
	// HUMANHEAD 
	virtual	idVec3			DetermineJumpVelocity();
	// HUMANHEAD END

	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction( void );
	void					WaterJumpMove( void );
	void					WaterMove( void );
	void					FlyMove( void );
	virtual	//HUMANHEAD: Made virtual
	void					AirMove( void );
	virtual	//HUMANHEAD: Made virtual
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	virtual	//HUMANHEAD: Made virtual
	void					CheckGround( void );
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckWaterJump( void );
	void					SetWaterLevel( void );
	void					DropTimers( void );
	void					MovePlayer( int msec );
};

#endif /* !__PHYSICS_PLAYER_H__ */
