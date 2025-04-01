/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/


#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__


/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

class idAFEntity_Base;
class CFrobDoor;

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

#ifndef MOD_WATERPHYSICS

// waterLevel_t has been moved to Physics_Actor.h

typedef enum 
{
  WATERLEVEL_NONE,
  WATERLEVEL_FEET,
  WATERLEVEL_WAIST,
  WATERLEVEL_HEAD
} waterLevel_t;

#endif		// MOD_WATERPHYSICS


#define	MAXTOUCH					32

typedef struct playerPState_s {
	idVec3					origin;
	idVec3					velocity;
	idVec3					localOrigin;
	idVec3					pushVelocity;
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;
} playerPState_t;


// This enumreation defines the phases of the mantling movement
enum EMantlePhase
{
	notMantling_DarkModMantlePhase	= 0,
	hang_DarkModMantlePhase,	
	pull_DarkModMantlePhase,
	pullFast_DarkModMantlePhase, // STiFU #4945: Quicker pull-push medium obstacles
	shiftHands_DarkModMantlePhase,
	push_DarkModMantlePhase,
	pushNonCrouched_DarkModMantlePhase, // STiFU #4930: Quicker low obstacles
	fixClipping_DarkModMantlePhase,		
	canceling_DarkModMantlePhase, // STiFU #4509: Cancel mantle when target is clipping
	NumMantlePhases,
};

extern const float MANTLE_TEST_INCREMENT;

class CForcePush;
typedef std::shared_ptr<CForcePush> CForcePushPtr;

// The class itself
class idPhysics_Player : 
	public idPhysics_Actor
{
public:
	CLASS_PROTOTYPE( idPhysics_Player );

							idPhysics_Player();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetSpeed( const float newWalkSpeed, const float newCrouchSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	int						GetMovementType( void ); // grayman #2345
	void					SetAirAccelerate( const float newAirAccelerate );

	/**
	* Get/set the movement flags, used to force a crouch externally
	**/
	int						GetMovementFlags( void );
	void					SetMovementFlags( int );
	void					SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles );
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
							// feed back from last physics frame
#ifndef MOD_WATERPHYSICS

	waterLevel_t            GetWaterLevel( void ) const;

	int                     GetWaterType( void ) const;

#endif

	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
	bool					IsHangMantle( void ) const;
	bool					IsPullMantle( void ) const;
	int						GetLastJumpTime() const; // SteveL #3716

	/**
	* Returns the reference entity velocity.  Nonzero only when the
	* player is climbing/roping so that their reference frame
	* is determined by some ent they're attached to
	**/
	idVec3					GetRefEntVel( void ) const;

	/**
	 * greebo: Returns the rope entity or NULL if the player is not attached to any rope.
	 */
	idEntity*				GetRopeEntity();

    bool					OnRope() const;

	/**
	* True if the player is climbing on a ladder or wall
	**/
	bool					OnLadder() const;

	/**
	* Returns the surface type the player is climbing
	* Returns empty string if the player is not climbing
	**/
	idStr					GetClimbSurfaceType() const;

	/**
	* Returns the lateral world coordinates resolved into the lateral direction
	* of the climbing surface.  Used to keep track of lateral position for climb movement sounds
	**/
	float					GetClimbLateralCoord(const idVec3& origVec) const;

	/**
	* Returns the distance between climbing sounds, in horizontal and vertical coords
	**/
	int						GetClimbSndRepDistVert() { return m_ClimbSndRepDistVert; }
	int						GetClimbSndRepDistHoriz() { return m_ClimbSndRepDistHoriz; }

	const idVec3 &			PlayerGetOrigin() const;	// != GetOrigin

	/**
	* Get the view yaw and pitch changes between last frame and this frame
	* Useful for rotating items in response to yaw, rope arrow, etc
	* Returns the change in degrees
	**/
	float					GetDeltaViewYaw();
	float					GetDeltaViewPitch();

	/**
	* True if the player is mid-air
	**/
	bool					IsMidAir() const;

public:

	/**
	* Ishtvan: This variable is set when the player used crouch to detach
	* from a climbable and is still holding down the button
	* This is not saved/restored but cleared on restore, just to be safe
	**/
	bool					m_bSlideOrDetachClimb;
	bool					m_bSlideInitialized;

public:	// common physics interface

	// virtual override of base class method SetSelf() to set the push force owner
	virtual void			SetSelf( idEntity *e ) override;

	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec ) override;
	virtual void			UpdateTime( int endTimeMSec ) override;
	virtual int				GetTime( void ) const override;

	virtual void			GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const override;
	virtual void			ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) override;
	virtual bool			IsAtRest( void ) const override;
	virtual int				GetRestStartTime( void ) const override;

	virtual void			SaveState( void ) override;
	virtual void			RestoreState( void ) override;

	virtual void			SetOrigin( const idVec3 &newOrigin, int id = -1 ) override;
	virtual void			SetAxis( const idMat3 &newAxis, int id = -1 ) override;

	virtual void			Translate( const idVec3 &translation, int id = -1 ) override;
	virtual void			Rotate( const idRotation &rotation, int id = -1 ) override;

	virtual void			SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 ) override;

	virtual const idVec3 &	GetLinearVelocity( int id = 0 ) const override;

	// This is true as soon as the player's velocity is well enough above walk speed
	bool					HasRunningVelocity();

	virtual void			SetPushed( int deltaTime ) override;
	virtual const idVec3 &	GetPushedLinearVelocity( const int id = 0 ) const override;
	void					ClearPushedVelocity( void );

	virtual void			SetMaster( idEntity *master, const bool orientated = true ) override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

/**
* Removes stale pointers when a rope entity is destroyed
* RopeEnt is the rope entity that's about to be destroyed.
**/
	void					RopeRemovalCleanup( idEntity *RopeEnt );

	bool					CheckPushEntity(idEntity *entity); // grayman #4603
	void					ClearPushEntity(); // grayman #4603

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

	// greebo: Used for "jump stamina"
	int						lastJumpTime;

	// player input
	usercmd_t				command;
	idAngles				viewAngles;

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	idVec3					viewForward;
	idVec3					viewRight;

	// swimming animation
	float					m_fSwimTimeStart_s;
	bool					m_bSwimSoundStarted;
	float					m_fSwimLeadInDuration_s;
	float					m_fSwimLeadOutStart_s;
	float					m_fSwimLeadOutDuration_s;
	float					m_fSwimSpeedModCompensation;

	// walk movement
	bool					walking;
	bool					groundPlane;
	trace_t					groundTrace;
	const idMaterial *		groundMaterial;

	/**
	* When the player is climbing on an entity (rope/wall, TODO: Apply to mantling)
	* This stores the velocity of the entity they're climbing on, for adding and subtracting
	* across functions if needed.
	**/
	idVec3					m_RefEntVelocity;

    // rope movement

	/**
	* Set to true if the player is moving and contacts a rope
	**/
    bool					m_bRopeContact;

	/**
	* The rope entity that the player last attached to
	**/
    idEntityPtr<idAFEntity_Base>	m_RopeEntity;

	/**
	* The rope entity that the player last touched (not necessarily attached to)
	* Used for the case where the player starts inside a rope and jumps up
	**/
	idEntityPtr<idAFEntity_Base>	m_RopeEntTouched;

	
	/**
	* toggled based on whether the player should stay attached to rope
	**/
	bool					m_bOnRope;

	/**
	* toggled on in the frame that the player first attaches to the rope
	**/
	bool					m_bJustHitRope;
	/**
	* Timer used to enforce time between kicks while on rope
	**/
	int						m_RopeKickTime;

	// ladder movement
	/**
	* Ladder is found ahead of us along view direction
	**/
	bool					m_bClimbableAhead;
	/**
	* We are currently attached to a ladder
	**/
	bool					m_bOnClimb;
	/**
	* Normal vector of the surface we're climbing on
	**/
	idVec3					m_vClimbNormal;
	/**
	* Attachment point of where the player's origin attaches to the ladder
	* In World coordinates
	* NOTE: This is the last known attachment point
	* The player is allowed to deviate from this a little to climb around corners
	**/
	idVec3					m_vClimbPoint;
	
	/**
	* Entity they're currently climbing on (used to add relative velocity)
	**/
	idEntityPtr<idEntity>	m_ClimbingOnEnt;
	/**
	* Set to true if the player has just detached from climbing this frame
	* Used to tell SlideMove to step up at the top of something
	**/
	bool					m_bClimbDetachThisFrame;

	/**
	* Set to true during the initial climb phase when the player
	* has gone from not climbing on anything to attaching to something ahead.
	* This is intended to fix a hovering-in-midair bug.
	**/
	bool					m_bClimbInitialPhase;

	/**
	* String name of the surface type the player is climbing
	**/
	idStr					m_ClimbSurfName;
	/**
	* Max horizontal climbing velocity (parsed from player.def for given surface name)
	**/
	float					m_ClimbMaxVelHoriz;
	/**
	* Max vertical climbing veocity (parsed from player.def for given surface name)
	**/
	float					m_ClimbMaxVelVert;
	/**
	* Distance between repetitions of the climbing sound, in vertical and horizontal direction
	* An integer number of doomunits
	**/
	int						m_ClimbSndRepDistVert;
	int						m_ClimbSndRepDistHoriz;

	int						m_NextAttachTime;
	
	/**
	* Timer value used as a workaround to step up curbs on high framerates.
	* Only set at a frametime below USERCMD_MSEC and decreased by the frametime each movement frame.
	**/
	
	int						m_SlopeIgnoreTimer;

	/**
	* View yaw and pitch changes between this frame and last frame
	* In degrees.
	**/
	float					m_DeltaViewYaw;
	float					m_DeltaViewPitch;

	// dragofer: Multiplier for the player's horizontal acceleration during AirMove.
	float					m_AirAccelerate;

	/**
	* Peek entity that the player is leaning into
	* Set to NULL if the player is not leaning into a peek entity
	**/
	idEntityPtr<idEntity>  m_LeanEnt;

	/**
	* Position to place the player listener when leaning
	**/
	idVec3					m_LeanListenPos;

	// greebo: The push force as applied to blocking objects
	CForcePushPtr			m_PushForce;

	// results of last evaluate
#ifndef MOD_WATERPHYSICS

  waterLevel_t            waterLevel;

  int                     waterType;

#endif


private:
	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, const float velocityLimit = -1.0);
	void					Friction( const idVec3 &wishdir = idVec3(), const float forceFriction = -1 );
	void					WaterMove( void );
	void					FlyMove( void );
	void					AirMove( void );
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					RopeMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
	void					CheckGround( void );
	void					CheckDuck( void );
	bool					CheckJump( void );
	bool					CheckRopeJump( void );
	/**
	* Read in the entity's velocity and store it to m_RefEntVelocity
	* If the ent is a static, the bindmaster velocity is read off instead
	* bodID is the clipmodel ID for AFs, defaults to zero
	**/
	void					SetRefEntVel( idEntity *ent, int bodID = 0 );

/**
* The following are used in wall/ladder climbing
**/
	void					CheckClimbable( void );

#ifndef MOD_WATERPHYSICS
	void					SetWaterLevel( void );
#endif
	void					DropTimers( void );
	void					MovePlayer( int msec );

	void					PlaySwimBurstSound();

public:
	void					ClimbDetach( bool bStepUp = false );
	void					RopeDetach( void );

	// angua: player can not attach to rope/ladder before this time is over
	void					SetNextAttachTime(int time)
	{
		m_NextAttachTime = time;
	}



	//#####################################################
	// Mantling handler
	// by SophisticatedZombie (Damon Hill)
	//
	//#####################################################

public:


	// This method returns
	// true if the player is mantling, false otherwise
	bool IsMantling() const;
	
	// This returns the current mantling phase
	EMantlePhase GetMantlePhase() const;

	// Cancels any current mantle
	void CancelMantle();

	// Checks to see if there is a mantleable target within reach
	// of the player's view. If so, starts the mantle... 
	// If the player is already mantling, this does nothing.
	void PerformMantle();

protected:

	/*!
	* The current mantling phase
	*/
	EMantlePhase m_mantlePhase;

	/**
	 * greebo: Set to TRUE if the next mantling can start. Set to FALSE at the 
	 *         beginning of a mantle process - the jump button has to be released
	 *         again during a non-mantling phase to set this to TRUE again.
	 */
	bool m_mantleStartPossible;

	/*!
	* Points along the mantle path
	*/
	idVec3 m_mantlePullStartPos;
	idVec3 m_mantlePullEndPos;
	idVec3 m_mantlePushEndPos;
	
	/*!
	* STiFU #4509: Mantle canceling animation
	*/
	float m_mantleCancelStartRoll;
	float m_fmantleCancelDist;
	idVec3 m_mantleCancelStartPos;
	idVec3 m_mantleCancelEndPos;
	idVec3 m_mantleStartPosWorld;

	/*!
	* Pointer to the entity being mantled.
	* This is undefined if m_mantlePhase == notMantling_DarkModMantlePhase
	*/
	idEntity* m_p_mantledEntity;

	/*!
	* ID number of the entity being mantled
	* This is 0 if m_mantlePhase == notMantling_DarkModMantlePhase
	*/
	int m_mantledEntityID;

	/*!
	* How long will the current phase of the mantle operation take?
	* Uses milliseconds and counts down to 0.
	*/
	float m_mantleTime;

	/*!
	* Tracks, in milliseconds, how long jump button has been held down
	* Counts upwards from 0.
	*/ 
	float m_jumpHeldDownTime;

	/*!
	* This method determines the mantle time required for each phase of the mantle.
	* I made this a function so you could implement things such as carry-weight,
	* tiredness, length of lift....
	* @param[in] mantlePhase The mantle phase for which the duration is to be retrieved
	*/
	float GetMantleTimeForPhase(EMantlePhase mantlePhase);

	/*!
	*
	* Internal method to start the mantle operation
	*
	* @param[in] initialMantlePhase The mantle phase in which the mantle starts.
	* @param[in] eyePos The position of the player's eyes in the world
	* @param[in] startPos The position of the player's feet at the start of the mantle
	* @param[in] endPos The position of the player's feet at the end of the mantle
	*/
	void StartMantle
	(
		EMantlePhase initialMantlePhase,
		idVec3 eyePos,
		idVec3 startPos,
		idVec3 endPos
	);

	/*!
	* Internal method which determines the maximum vertical
	* and horizontal distances for mantling
	*
	* @param[out] out_maxVerticalReachDistance The distance that the player can reach vertically, from their current origin
	* @param[out] out_maxHorizontalReachDistance The distance that the player can reach horizontally, from their current origin
	* @param[out] out_maxMantleTraceDistance The maximum distance that the traces should look in front of the player for a mantle target
	*/
	void GetCurrentMantlingReachDistances
	(
		float& out_maxVerticalReachDistance,
		float& out_maxHorizontalReachDistance,
		float& out_maxMantleTraceDistance
	);

	/*!
	* This method runs the trace to find a mantle target
	* It first attempts to raycast along the player's gaze
	* direction to determine a target. If it doesn't find one,
	* then it tries a collision test along a vertical plane
	* from the players feet to their height, out in the direction
	* the player is facing.
	*
	* @param[in] maxMantleTraceDistance The maximum distance from the player that should be used in the traces
	* @param[in] eyePos The position of the player's eyes, used for the beginning of the gaze trace
	* @param[in] forwardVec The vector gives the direction that the player is facing
	* @param[out] out_trace This trace structure will hold the result of whichever trace succeeds. If both fail, the trace fraction will be 1.0
	*/
	void MantleTargetTrace
	(
		float maxMantleTraceDistance,
		const idVec3& eyePos,
		const idVec3& forwardVec,
		trace_t& out_trace
	);

	enum EMantleable
	{
		EMantleable_No = 0,
		EMantleable_YesCrouched = 1,
		EMantleable_YesUpstraight = 2,
	};

	/*!
	* 
	* This function checks the collision target of the mantle 
	* trace to see if there is a surface within reach distance
	* upon which the player will fit.
	*
	* @param[in] maxVerticalReachDistance The maximum distance that the player can reach vertically from their current origin
	* @param[in] maxHorizontalReachDistance The maximum distance that the player can reach horizontally from their current origin
	* @param[in] in_targetTraceResult The trace which found the mantle target
	* @param[out] out_mantleEndPoint If the return code is true, this out paramter specifies the position of the player's origin at the end of the mantle move.
	*
	* @return the result of the test
	* @retval EMantleable_No if the mantel target does not have a mantleable surface
	* @retval EMantleable_YesCrouched if the mantle target has a mantleable surface that can be reached in crouched state
	* @retval EMantleable_YesUpstraight if the mantle target has a mantleable surface
	*
	*/
	EMantleable DetermineIfMantleTargetHasMantleableSurface
	(
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		trace_t& in_targetTraceResult,
		idVec3& out_mantleEndPoint
	);		

	/*!
	* Call this method to test whether or not the path
	* along the mantle movement is free of obstructions.
	*
	* @param[in] maxVerticalReachDistance The maximum distance that the player can reach vertically from their current origin
	* @param[in] maxHorizontalReachDistance The maximum distance that the player can reach horizontally from their current origin
	* @param[in] testCrouched Perform this test with a crouched clipmodel (true) or standing up straight (false)
	* @param[in] eyePos The position of the player's eyes in the world
	* @param[in] mantleStartPoint The player's origin at the start of the mantle movement
	* @param[in] mantleEndPoint The player's origin at the end of the mantle movement
	*
	* @return the result of the test
	* @retval true if the path is clear
	* @retval false if the path is blocked
	*/
	bool DetermineIfPathToMantleSurfaceIsPossible
	(
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		bool  testCrouched,
		const idVec3& eyePos,
		const idVec3& mantleStartPoint,
		const idVec3& mantleEndPoint
	);

	/*!
	* Given a trace which resulted in a detected mantle
	* target, this method checks to see if the target
	* is mantleable.  It looks for a surface up from gravity
	* on which the player can fit while crouching. It also
	* checks that the distance does not violate horizontal
	* and vertical displacement rules for mantling. Finally
	* it checks that the path to the mantleable surface is
	* not blocked by obstructions.
	*
	* This method calls DetermineIfMantleTargetHasMantleableSurface and
	* also DetermineIfPathToMantleSurfaceIsPossible
	*
	* @param[in] maxVerticalReachDistance The maximum distance from the player's origin that the player can reach vertically
	* @param[in] maxHorizontalReachDistance The maximum distance from the player's origin that the player can reach horizontally
	* @param[in] eyePos The position of the player's eyes (camera point) in the world coordinate system
	* @param[in] in_targetTraceResult Pass in the trace result from MantleTargetTrace
	* @param[out] out_mantleEndPoint If the return value is true, this passes back out what the player's origin will be at the end of the mantle
	*
	* @returns the result of the test
	* @retval EMantleable_No if the mantel target does not have a mantleable surface
	* @retval EMantleable_YesCrouched if the mantle target has a mantleable surface that can be reached in crouched state
	* @retval EMantleable_YesUpstraight if the mantle target has a mantleable surface
	*
	*/	
	EMantleable ComputeMantlePathForTarget
	(	
		float maxVerticalReachDistance,
		float maxHorizontalReachDistance,
		const idVec3& eyePos,
		trace_t& in_targetTraceResult,
		idVec3& out_mantleEndPoint
	);
		
	/*!
	* This handles the reduction of the mantle timer by the
	* number of milliseconds between animation frames. It
	* uses the timer results to update the mantle timers.
	*/
	void UpdateMantleTimers();
    
	/*!
	* This handles the movement of the the player during
	* the phases of the mantle.  It performs the translation
	* of the player along the mantle path, the camera movement
	* that creates the visual effect, and the placing of the
	* player into a crouch during the end phase of the mantle.
	* 
	*/
	void MantleMove();

	
	/** @brief	  	Test if the mantle end position intersects with world geomtry
	  * @param 		pPhysicsMantledEntity Pointer to the mantled entity
	  * @return   	True, if the player is clipping into world geometry at the 
	  *				mantle endposition
	  * @author		STiFU #4509: Test entposition for clipping, cancel in that case */
	const bool IsMantleEndPosClipping(idPhysics* pPhysicsMantledEntity);

	// Tests if player is holding down jump while already jumping
	// (can be used to trigger mantle)
	bool CheckJumpHeldDown();

	//#################################################
	// End mantling handler
	//#################################################

	//#####################################################
	// Leaning handler
	// by SophisticatedZombie (Damon Hill)
	//
	//#####################################################

protected:

	/*!
	* An axis which is perpendicular to the gravity normal and
	* rotated by the given yaw angle clockwise (when looking down in direction of
	* gravity) around it. The player always leans to positive angles
	* around this axis.
	*/
	float m_leanYawAngleDegrees;

	/*! 
	* The current lean angle
	*/
	float m_CurrentLeanTiltDegrees;

	/*!
	* The start (roll) angle of this lean movement
	*/
	float m_leanMoveStartTilt;

	/*!
	* The end (roll) angle of this lean movement
	*/
	float m_leanMoveEndTilt;

	/**
	* Max lean angle (set dynamically depending on forward/sideways leans)
	**/
	float m_leanMoveMaxAngle;

	/*!
	* Is the lean finished
	*/
	bool m_b_leanFinished;

	/*!
	* How long will the current phase of the leaning operation take?
	* Uses milliseconds and counts down to 0
	*/
	float m_leanTime;

	/*!
	* The last recorded view angles
	* These are updated each animation frame and are used
	* to track player rotations to due rotation collision detection.
	* The original Doom3 code did not need to due collision tests
	* during player view rotation, but if the player is leaning, it is
	* necessary to prevent passing through solid objects
	*/
	idAngles m_lastPlayerViewAngles;

	float m_lastCommandViewYaw;
	float m_lastCommandViewPitch;
	
	/*!
	* The current resulting view lean angles
	*/
	idAngles m_viewLeanAngles;

	/*!
	* The current resulting view lean translation
	* In local/player coordinates (along player model axes, not world axes)
	*/
	idVec3 m_viewLeanTranslation;

	/**
	* Lean bounds for collision testing.  Same as most view testing bounds, 10x10x10 box
	* Also extends 15 units downwards.  Initialized once at spawn.
	**/
	idBounds m_LeanViewBounds;

protected:
	/**
	* This uses the other internal mtehods to coordiante the lean
	* lean movement.
	**/
	void LeanMove();

	/**
	* Test clipping for the current eye position, plus delta in the lean direction
	**/
	bool TestLeanClip();

	/**
	* Convert a lean angle into a point in space, in world coordinates
	**/
	idVec3 LeanParmsToPoint( float AngTilt );

	/**
	* Start and maintain a peeking state until exited
	**/
	void ProcessPeek(idEntity *peekEntity, idEntity* door, idVec3 normal); // grayman #4882
	
	/*!
	* This method updates the lean by as much of the delta amount given
	* that does not result in a clip model trace collision.
	*
	* This is an internal method called by LeanMove.
	* UpdateLeanPhysics must be called after this.
	**/
	void UpdateLeanAngle(float deltaLeanAngle);

	/**
	* Takes the currently set m_CurrentLeanTiltDegrees
	* And updates m_LeanTranslation and m_ViewLeanAngles
	* Should be called after changing these member vars.
	**/
	void UpdateLeanPhysics();

	/**
	* This method gets called when the leaned player view is found to be clipping something
	* It un-leans them in increments until they are outside the solid object
	**/
	void UnleanToValidPosition();

	/**
	* Calculates the listener position when leaning against a given door
	* Tests points in space along the lean yaw axis until they come out the other
	* side of the door into empty space.
	*
	* Takes the point of contact (where the trace hit the door), 
	*	starts testing at this point, extending along lean yaw axis.
	*
	* If a point is found, it sets m_DoorListenPos and returns true
	* If door is too thick to listen through, returns false
	**/
	bool FindLeanListenPos(const idVec3& incidencePoint); // grayman #4882

	/**
	* Tests whether the player is still leaning against a peek entity.
	* If not, clears m_LeanEnt
	**/
	void UpdateLean();

public:

	/*!
	* Starts or ends a lean around and axis which is perpendicular to the gravity normal and
	* rotated by the given yaw angle clockwise (when looking down in direction of
	* gravity) around it. 
	* 
	* @param[in] leanYawAngleDegrees The angle of the axis around which the player will
	* lean clockwise, itself rotated clockwise from straight forward.
	* 0.0 leans right, 180.0 leans left, 90.0 leans forward, 270.0 leans backward
	*
	*/
	void ToggleLean(float leanYawAngleDegrees);
	void UnLean(float leanYawAngleDegrees);

	/*!
	* This method tests if the player is in the middle of a leaning
	* movement.
	*
	* @return the result of the test
	* @retval true if the player is changing lean
	* @retval false if the player is not changing lean
	*/
	bool IsLeaning();

	/**
	* Returns true if the player is leaning near a peeking entity (keyhole or crack)
	**/
	bool IsPeekLeaning();

	/*!
	* This is called from idPlayer to adjust the camera before
	* rendering
	* Returns current view lean translation in world axes
	*/
	const idAngles& GetViewLeanAngles() const;

	/*
	* This is called from idPlayer to adjust the camera before
	* rendering
	*/
	idVec3 GetViewLeanTranslation();

	/**
	* Takes proposed new view angles InputAngles,
	* tests whether the change in yaw will cause a collision
	* of the leaned camera with a wall.  If so, clamps the yaw before the collision.
	* Overwrites InputAngles with the new vetted angles.
	* Does nothing if the player is not leaned
	**/
	void UpdateLeanedInputYaw( idAngles &InputAngles );

	//#####################################################
	// Shouldering viewport animation (#3607)
	// by STiFU
	//#####################################################
public: 
		
	/** @brief	  	Initialize and schedule shouldering animation (#3607)
	  * @param 		pBody Pointer to the body to be shouldered
	  * @author		STiFU */
	void StartShouldering(idEntity const * const pBody);
	
	/** @brief	  	Check if shouldering animation plays out (#3607)
	  * @return   	True, if animation is active
	  * @author		STiFU */
	bool IsShouldering() const;

	bool					m_bMidAir;
private:
		
	/** @brief	  	Try to start the shouldering animation (#3607)
	  * @author		STiFU */
	void StartShoulderingAnim();

	
	/** @brief	  	Perform the shouldering animation (#3607)
	  * @author		STiFU */
	void ShoulderingMove();

	enum eShoulderingAnimation
	{
		eShoulderingAnimation_NotStarted = 0,
		eShoulderingAnimation_Initialized = 1,
		eShoulderingAnimation_Scheduled = 2,
		eShoulderingAnimation_Active = 3,
	};

	eShoulderingAnimation	m_eShoulderAnimState;
	float					m_fShoulderingTime;
	float					m_fPrevShoulderingPitchOffset;
	idVec3					m_PrevShoulderingPosOffset;
	idVec3					m_ShoulderingStartPosRelative;
	idVec3					m_ShoulderingCurrentPosRelative;
	idEntity*				m_pShoulderingGroundEntity;
	bool					m_bShouldering_SkipDucking;
	float					m_fShouldering_TimeToNextSound;
};

#endif /* !__PHYSICS_PLAYER_H__ */
