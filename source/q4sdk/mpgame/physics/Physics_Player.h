
#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__

/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

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

#define	MAXTOUCH					32

typedef struct playerPState_s {
	idVec3					origin;
	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;	
	idVec3					lastPushVelocity;
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;
	int						crouchSlideTime;
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
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
							// feed back from last physics frame
	waterLevel_t			GetWaterLevel( void ) const;
	int						GetWaterType( void ) const;
	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
	bool					IsJumping( void ) const;
	bool					OnLadder( void ) const;
	bool					OnGround( void ) const;
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

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

	bool					IsNoclip( void ) const;
	bool					IsDead( void ) const;

	void					SetClipModelNoLink( idClipModel *clip );

private:
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
	trace_t					groundTrace;
	const idMaterial *		groundMaterial;

	// ladder movement
	bool					ladder;
	idVec3					ladderNormal;

	// results of last evaluate
	waterLevel_t			waterLevel;
	int						waterType;

private:
	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction( void );
	void					WaterJumpMove( void );
	void					WaterMove( void );
	void					FlyMove( void );
	void					AirMove( void );
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	void					CheckGround( bool checkStuck );
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckWaterJump( void );
	void					SetWaterLevel( void );
	void					DropTimers( void );
	void					MovePlayer( int msec );

	// forceLength:
	// 0 => use the pm_slideVelocity / pm_powerSlide settings
	// 1 => maintain velocity
	// 2 => maintain horizontal velocity
	idVec3					AdjustVertically( const idVec3 &normal, const idVec3 &in, int forceLength = 0 ) const;

	float					Pm_Accelerate( void );
	float					Pm_AirAccelerate( void );
};

ID_INLINE bool idPhysics_Player::IsNoclip( void ) const {
	return current.movementType == PM_NOCLIP;
}

ID_INLINE bool idPhysics_Player::IsDead( void ) const {
	return current.movementType == PM_DEAD;
}

ID_INLINE bool idPhysics_Player::OnGround( void ) const {
	return groundPlane;
}

#endif /* !__PHYSICS_PLAYER_H__ */
