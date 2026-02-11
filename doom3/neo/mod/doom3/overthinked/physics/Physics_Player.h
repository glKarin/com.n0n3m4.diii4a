/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

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
typedef enum
{
    PM_NORMAL,				// normal physics
    PM_DEAD,				// no acceleration or turning, but free falling
    PM_SPECTATOR,			// flying without gravity but with collision detection
    PM_FREEZE,				// stuck in place without control
    PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

typedef enum
{
    WATERLEVEL_NONE,
    WATERLEVEL_FEET,
    WATERLEVEL_WAIST,
    WATERLEVEL_HEAD
} waterLevel_t;

#define	MAXTOUCH					32

typedef struct playerPState_s
{
    idVec3					origin;
    idVec3					velocity;
    idVec3					localOrigin;
    idVec3					pushVelocity;
    float					stepUp;
    int						movementType;
    int						movementFlags;
    int						movementTime;
} playerPState_t;

class idPhysics_Player : public idPhysics_Actor
{

public:
    CLASS_PROTOTYPE(idPhysics_Player);

    idPhysics_Player(void);

    void					Save(idSaveGame *savefile) const;
    void					Restore(idRestoreGame *savefile);

    // initialisation
    void					SetSpeed(const float newWalkSpeed, const float newCrouchSpeed);
    void					SetMaxStepHeight(const float newMaxStepHeight);
    float					GetMaxStepHeight(void) const;
    void					SetMaxJumpHeight(const float newMaxJumpHeight);
    void					SetMovementType(const pmtype_t type);
    void					SetPlayerInput(const usercmd_t &cmd, const idAngles &newViewAngles);
    void					SetKnockBack(const int knockBackTime);
    void					SetDebugLevel(bool set);
    // feed back from last physics frame
    waterLevel_t			GetWaterLevel(void) const;
    int						GetWaterType(void) const;
    bool					HasJumped(void) const;
    bool					HasSteppedUp(void) const;
    float					GetStepUp(void) const;
    bool					IsCrouching(void) const;
    bool					OnLadder(void) const;
    const idVec3 			&PlayerGetOrigin(void) const;	// != GetOrigin

public:	// common physics interface
    bool					Evaluate(int timeStepMSec, int endTimeMSec);
    void					UpdateTime(int endTimeMSec);
    int						GetTime(void) const;

    void					GetImpactInfo(const int id, const idVec3 &point, impactInfo_t *info) const;
    void					ApplyImpulse(const int id, const idVec3 &point, const idVec3 &impulse);
    bool					IsAtRest(void) const;
    int						GetRestStartTime(void) const;

    void					SaveState(void);
    void					RestoreState(void);

    void					SetOrigin(const idVec3 &newOrigin, int id = -1);
    void					SetAxis(const idMat3 &newAxis, int id = -1);

    void					Translate(const idVec3 &translation, int id = -1);
    void					Rotate(const idRotation &rotation, int id = -1);

    void					SetLinearVelocity(const idVec3 &newLinearVelocity, int id = 0);

    const idVec3 			&GetLinearVelocity(int id = 0) const;

    void					SetPushed(int deltaTime);
    const idVec3 			&GetPushedLinearVelocity(const int id = 0) const;
    void					ClearPushedVelocity(void);

    void					SetMaster(idEntity *master, const bool orientated = true);

    void					WriteToSnapshot(idBitMsgDelta &msg) const;
    void					ReadFromSnapshot(const idBitMsgDelta &msg);
    /**
      * Get the view yaw and pitch changes between last frame and this frame
      * Useful for rotating items in response to yaw, rope arrow, etc
      * Returns the change in degrees
      **/
    float                   GetDeltaViewYaw();
    float                   GetDeltaViewPitch();
    /**
       * Door that the player is leaning into
       * Set to NULL if the player is not leaning into a door
       **/
    //idEntityPtr<CFrobDoor>  m_LeanDoorEnt;

    /**
    * Position to place the player listener beyond the door when door leaning
    **/
    idVec3                  m_LeanDoorListenPos;

    //#####################################################
    // Leaning handler
    // by SophisticatedZombie (Damon Hill)
    //
    //#####################################################

protected:

    /**
    * Set to true if the player is leaning at all from the vertical
    **/
    bool m_bIsLeaning;

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

    /**
    * Current lean stretch fraction.  When this is 1.0, the player is at full stretch, at 0.0, not stretched
    **/
    float m_CurrentLeanStretch;

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

    /**
    * Maximum lean stretch (set dynamically depending on forward/sideways leans)
    **/
    float m_leanMoveMaxStretch;

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
    * Convert a lean angle and stretch into a point in space, in world coordinates
    **/
    idVec3 LeanParmsToPoint( float AngTilt, float Stretch );

    /*!
    * This method updates the lean by as much of the delta amount given
    * that does not result in a clip model trace collision.
    *
    * This is an internal method called by LeanMove.
    * UpdateLeanPhysics must be called after this.
    **/
    void UpdateLeanAngle(float deltaLeanAngle, float deltaLeanStretch);

    /**
    * Takes the currently set m_CurrentLeanTiltDegrees and m_CurrentLeanStretch
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
    *   starts testing at this point, extending along lean yaw axis.
    *
    * If a point is found, it sets m_DoorListenPos and returns true
    * If door is too thick to listen through, returns false
    **/
//    bool FindLeanDoorListenPos(const idVec3& incidencePoint, CFrobDoor* door);
//
//    /**
//    * Tests whether the player is still leaning against the door.
//    * If not, clears m_LeanDoorEnt
//    **/
//    void UpdateLeanDoor();

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
    * Returns true if the player is leaning against a door
    **/
    bool        IsDoorLeaning();
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
    const idMaterial 		*groundMaterial;

    // ladder movement
    bool					ladder;
    idVec3					ladderNormal;

    // results of last evaluate
    waterLevel_t			waterLevel;
    int						waterType;
    /**
      * View yaw and pitch changes between this frame and last frame
      * In degrees.
      **/
    float                   m_DeltaViewYaw;
    float                   m_DeltaViewPitch;


private:
    float					CmdScale(const usercmd_t &cmd) const;
    void					Accelerate(const idVec3 &wishdir, const float wishspeed, const float accel);
    bool					SlideMove(bool gravity, bool stepUp, bool stepDown, bool push);
    void					Friction(void);
    void					WaterJumpMove(void);
    void					WaterMove(void);
    void					FlyMove(void);
    void					AirMove(void);
    void					WalkMove(void);
    void					DeadMove(void);
    void					NoclipMove(void);
    void					SpectatorMove(void);
    void					LadderMove(void);
    void					CorrectAllSolid(trace_t &trace, int contents);
    void					CheckGround(void);
    void					CheckDuck(void);
    void					CheckLadder(void);
    bool					CheckJump(void);
    bool					CheckWaterJump(void);
    void					SetWaterLevel(void);
    void					DropTimers(void);
    void					MovePlayer(int msec);
};

#endif /* !__PHYSICS_PLAYER_H__ */
