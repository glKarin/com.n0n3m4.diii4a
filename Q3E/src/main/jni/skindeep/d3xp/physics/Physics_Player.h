/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "physics/Physics_Actor.h"
#include "Mover.h"

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

// SW: Number of units of clearance to leave between the camera and the edge of the aperture
// when lerping into a ventdoor aperture
#define APERTURE_MARGIN				12

// SW: Number of units that that a ventdoor aperture should be offset 'out' when performing zipping calculations.
// This is used to account for very skew angles (and the lip of the ventdoor itself)
#define APERTURE_OFFSET_DISTANCE	4

//BC enums
enum
{
	CLAMBERSTATE_NONE,
	CLAMBERSTATE_RAISING,
	CLAMBERSTATE_SETTLING,
	CLAMBERSTATE_ACRO
};

enum
{
	ZIPPINGSTATE_NONE,
	ZIPPINGSTATE_PAUSE,
	ZIPPINGSTATE_DASHING
};

enum
{
	SWOOPSTATE_NONE,
	SWOOPSTATE_MOVETOSTART,
	SWOOPSTATE_MOVETOEND
};



enum
{
	FALLEN_NONE,
	FALLEN_HEADONGROUND,
	FALLEN_RISING,
	FALLEN_IDLE,
	FALLEN_GETUP_KICKUP

};

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

class idPhysics_Player : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( idPhysics_Player );

							idPhysics_Player( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
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

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

	//BC PUBLIC

	int						GetAcroType();
	int						GetClamberState();
	void					GetAcroAngleRestrictions(float& baseAngle, int& arcSize);
	float					GetAcroAngle();

	void					SetFallState(bool hardFall);
	int						GetFallState();
	void					SetImmediateExitFallState();
	void					BeginGetUp();

	void					ForceDuck(int howlong);
	void					ForceUnduck();
	bool					CheckUnduckOverheadClear(void); // blendo eric: check if player has overhead clearance to unduck, this is probably slow
	void					ZippingTo(idVec3 destination, idWinding* aperture, float forceDuckDuration);
	bool					GetZippingState();

	void					CableswoopTo(idEntity * startPoint, idEntity * endPoint);
	void					SpaceCableswoopTo(idEntity * startPoint, idEntity * endPoint);
	bool					GetSwoopState();

	void					SetHideState(idEntity * hideEnt, int verticalOffset);
	void					SetAcroState(idEntity * hideEnt);

	void					StartMovelerp(idVec3 destination, int moveTime);

	int						hideType;

	void					StartGrabRing(idVec3 grabPosition, idEntity *grabring);
	bool					GetGrabringState();

	// SW 17th Feb 2025
	void					SetVacuumSplineMover(idMover* mover);
	idMover*				GetVacuumSplineMover(void);

	//END BC PUBLIC

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
	void					Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel, const idVec3 &clampVector );
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
	void					CheckGround( void );
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckWaterJump( void );
	void					SetWaterLevel( void );
	void					DropTimers( void );
	void					MovePlayer( int msec );


	//BC PRIVATE

	//-------- functions --------
	void					UpdateClamber(void);
public:
	bool					TryClamber(bool checkFromCrouch = false, int numIterations = 4);
	bool					TryClamberOutOfCubby(bool checkInCubby); // blendo eric: clambers out of cubby if there's clearance nearby to stand
private:
	idVec3					GetPossibleClamberPos_Ledge(int numIterations, idVec3 & startingPosOut);
	idVec3					GetPossibleClamberPos_Ledgecheck(bool backwardsCheck, int numIterations, idVec3 & startingPosOut);
	idVec3					GetPossibleClamberPos_Cubby(idVec3 eyePos, int numIterations);
	float					GetClamberLerp(void);
	idVec3					CheckClamberBounds(idVec3 basePos, idVec3 sweepStart, bool doExtraChecks = true);
	idVec3					CheckClamberBounds(idVec3 basePos, bool doExtraChecks = true){ return CheckClamberBounds(basePos,basePos,doExtraChecks); }
	void					StartClamber(idVec3 targetPos, idVec3 shiftBack, float raiseTimeScale, float settleTimeScale, float verticalMix, float horiztonalMix, bool forceDuck, bool ignoreLipCheck = false);
	void					StartClamber(idVec3 targetPos, idVec3 shiftBack = vec3_zero);
	void					StartClamberQuick(idVec3 targetPos, idVec3  shiftBack = vec3_zero);
	int						GetClamberMaxHeightLocal();
	int						GetClamberMaxHeightWorld();
	int						CheckClamberHeight(float z);
	bool					ClamberCheckSteppable(idVec3& newClamberPos); // blendo eric: check if the landing spot should be stepped rather than clambered
	bool					ClamberCheckCubbyLowLedge(idVec3 cubbyPos); // blendo eric: check if there's a low ledge above a ground cubby spot, i.e. benches
	idVec3					CheckClamberExitCubby(int sideTestCount = 8); // blendo eric: check if space next to player the overhead is clear to stand

	void					UpdateAcro(void);

	bool					DoPrejumpLogic(void);
	void					DoJump(void);

	void					CheckLadderDismount(void);

	void					CheckAutocrouch(void);

	//-------- variables --------
	int						clamberState;
	idVec3					clamberOrigin; // start point
	idVec3					clamberTransition; // transition point between states
	idVec3					clamberDestination; // final point
	int						clamberStartTime; // start of state time
	int						clamberStateTime; // time until next state
	float					clamberTotalMoveTime;
	bool					clamberForceDuck; // force duck during clamber animation
	idVec3					clamberGroundPosInitial; // position at initial jump
	idVec3					clamberGroundPosCurrent; // last checked ground pos, capped

	
	int						acroUpdateTimer;
	
	int						acroType;
	float					acroAngle; //angle of the acro entity, so we can do things like restrict view angle, etc.
	idVec3					lastgoodAcroPosition;
	float					acroViewAngleArc;
	int						dashStartTime;
	int						dashWarningTimer;
	float					dashSlopeScale;
	

	bool					inJump;
	int						coyotetimeTimer;

	void					UpdateLadderLerp(void);	
	int						ladderLerpTimer; //when we need to teleport player into a ladder mounting position.
	bool					ladderLerpActive;
	idVec3					ladderLerpTarget;
	idVec3					ladderLerpStart;
	int						ladderTimer; //times the intervals between ladder steps/dashes
	
	void					UpdateFallState();
	int						fallenState;
	int						fallenTimer;

	int						forceduckTimer;


	void					UpdateZipping();
	int						zippingState;
	int						zippingTimer;
	idVec3					zippingOrigin;
	idVec3					zippingCameraStart;
	idVec3					zippingCameraEnd;
	idVec3					zippingCameraMidpoint;
	float					zipForceDuckDuration;


	
	void					UpdateSwooping();
	int						swoopState;
	int						swoopTimer;
	idVec3					swoopStartPoint;
	idVec3					swoopDestinationPoint;
	idEntity *				swoopStartEnt; //TODO: make these pointers instead
	idEntity *				swoopEndEnt;
	int						swoopParticletype;
	enum					{ SWOOP_VERTICALZIPCORD, SWOOP_SPACECABLE };

	idEntityPtr<idEntity>	cargohideEnt;

	
	void					UpdateMovelerp();
	idVec3					movelerpStartPoint;
	idVec3					movelerpDestinationPoint;
	int						movelerpTimer;
	bool					movelerping;
	int						movelerp_Duration;


	int						nextAutocrouchChecktime;


	//Grabring stuff.	
	void					UpdateGrabRing();
	idVec3					grabringStartPos;
	idVec3					grabringDestinationPos;
	int						grabringTimer;
	int						grabringState;
	enum					{GR_NONE, GR_GRABSTART, GR_GRABIDLE};
	idEntityPtr<idEntity>	grabringEnt;

	bool					canFlymoveUp;

	int						lastAirlessMoveTime;
	int						spacenudgeState;
	enum                    { SN_NONE, SN_RAMPINGUP, SN_NUDGING };
	int						spacenudgeRampTimer;

	// SW 17th Feb 2025
	void					UpdateVacuumSplineMoving(void);
	idEntityPtr<idMover>	vacuumSplineMover;

	//BC PRIVATE END

};

#endif /* !__PHYSICS_PLAYER_H__ */
