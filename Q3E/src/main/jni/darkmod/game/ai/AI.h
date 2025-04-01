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

#ifndef __AI_H__
#define __AI_H__

#include "../Relations.h"
#include "../ai/Mind.h"
#include "../ai/CommunicationSubsystem.h"
#include "../ai/MovementSubsystem.h"
#include "../HidingSpotSearchCollection.h"
#include "../darkmodHidingSpotTree.h"
#include "MoveState.h"

#include <list>
#include <set>

/*
===============================================================================

	idAI

===============================================================================
*/

const float	SQUARE_ROOT_OF_2			= 1.414213562f;
const float	AI_TURN_PREDICTION			= 0.2f;
const float	AI_TURN_SCALE				= 60.0f;
const float	AI_SEEK_PREDICTION			= 0.3f;
const float	AI_FLY_DAMPENING			= 0.15f;
const float	AI_HEARING_RANGE			= 2048.0f;
const int	DEFAULT_FLY_OFFSET			= 68;

// grayman #2414 - TEMP_THINK constants used with interleaved thinking
const int	TEMP_THINK_INTERLEAVE		= 4;	// maximum interleave think frames when
												// more interleaved thinking is needed
const int	TEMP_THINK_DISTANCE			= 200;	// increase interleave think frames
												// when door is closer than this
const int	TEMP_THINK_FACTOR			= 8;	// used to determine when to increase thinking
												// when moving to a goal position

// used to declare the Dark Mod Acuity values array.
// THIS MUST BE CHANGED if you want more than 15 acuities
static const int s_MAXACUITIES = 15;

#define ATTACK_IGNORE			0
#define ATTACK_ON_DAMAGE		1
#define ATTACK_ON_ACTIVATE		2
#define ATTACK_ON_SIGHT			4

// grayman #3820 - These must be kept in sync with their counterparts in PathSleepTask.h
// and tdm_ai.script.
// The sleep_location spawnarg can be used both on path_sleep entities and
// on the AI entities. The former has priority over the latter
#define SLEEP_LOC_UNDEFINED		-1
#define SLEEP_LOC_FLOOR			0
#define SLEEP_LOC_BED			1
#define SLEEP_LOC_CHAIR			2

typedef enum {
	TALK_NEVER,
	TALK_DEAD,
	TALK_OK,
	TALK_BUSY,
	NUM_TALK_STATES
} talkState_t;

typedef enum { // grayman #2604 - how the AI was knocked out
	KO_NOT, // default - not KO'ed
	KO_BLACKJACK,
	KO_GAS,
	KO_FALL,	// grayman #3699
	NUM_KO_STATES
} koState_t;

#define	DI_NODIR	-1

// obstacle avoidance
typedef struct obstaclePath_s {
	idVec3				seekPos;					// seek position avoiding obstacles
	idEntity *			firstObstacle;				// if != NULL the first obstacle along the path
	idVec3				startPosOutsideObstacles;	// start position outside obstacles
	idEntity *			startPosObstacle;			// if != NULL the obstacle containing the start position 
	idVec3				seekPosOutsideObstacles;	// seek position outside obstacles
	idEntity *			seekPosObstacle;			// if != NULL the obstacle containing the seek position 
	CFrobDoor*			doorObstacle;				// greebo: if != NULL, this is the door in our way
} obstaclePath_t;

// path prediction
typedef enum {
	SE_BLOCKED			= BIT(0),
	SE_ENTER_LEDGE_AREA	= BIT(1),
	SE_ENTER_OBSTACLE	= BIT(2),
	SE_FALL				= BIT(3),
	SE_LAND				= BIT(4)
} stopEvent_t;

typedef struct predictedPath_s {
	idVec3				endPos;						// final position
	idVec3				endVelocity;				// velocity at end position
	idVec3				endNormal;					// normal of blocking surface
	int					endTime;					// time predicted
	int					endEvent;					// event that stopped the prediction
	const idEntity *	blockingEntity;				// entity that blocks the movement
} predictedPath_t;

//
// events
//
extern const idEventDef AI_BeginAttack;
extern const idEventDef AI_EndAttack;
extern const idEventDef AI_MuzzleFlash;
extern const idEventDef AI_CreateMissile;
extern const idEventDef AI_CreateMissileFromDef;
extern const idEventDef AI_AttackMissile;
extern const idEventDef AI_FireMissileAtTarget;
extern const idEventDef AI_AttackMelee;
extern const idEventDef AI_DirectDamage;
extern const idEventDef AI_JumpFrame;
extern const idEventDef AI_EnableClip;
extern const idEventDef AI_DisableClip;
extern const idEventDef AI_EnableGravity;
extern const idEventDef AI_DisableGravity;
extern const idEventDef AI_TriggerParticles;
extern const idEventDef AI_RandomPath;

// DarkMod Events
extern const idEventDef AI_GetRelationEnt;
extern const idEventDef AI_GetSndDir;
extern const idEventDef AI_GetVisDir;
extern const idEventDef AI_GetTactEnt;
extern const idEventDef AI_VisScan;
extern const idEventDef AI_Alert;
extern const idEventDef AI_GetAcuity;
extern const idEventDef AI_SetAcuity;
extern const idEventDef AI_SetAudThresh;
extern const idEventDef AI_ClosestReachableEnemy;
extern const idEventDef AI_ReEvaluateArea;
extern const idEventDef AI_PlayCustomAnim;


// Darkmod: Glass Houses events
extern const idEventDef AI_SpawnThrowableProjectile;

// Darkmod AI additions
extern const idEventDef AI_GetVariableFromOtherAI;
extern const idEventDef AI_GetAlertLevelOfOtherAI;

// Set a grace period for alerts
extern const idEventDef AI_SetAlertGracePeriod;

// This event is used to get a position from which a given position can be observed
extern const idEventDef AI_GetObservationPosition;

// Darkmod communication issuing event
extern const idEventDef AI_IssueCommunication;

extern const idEventDef AI_Bark; // grayman #2816

extern const idEventDef AI_RestartPatrol; // grayman #2920

extern const idEventDef AI_OnDeadPersonEncounter; // grayman #3317
extern const idEventDef AI_OnUnconsciousPersonEncounter; // grayman #3317

extern const idEventDef AI_AllowGreetings; // grayman #3338

extern const idEventDef AI_NoisemakerDone; // grayman #3681

extern const idEventDef AI_OnHitByDoor; // grayman #3756

extern const idEventDef AI_DelayedVisualStim; // grayman #2924

extern const idEventDef AI_PickedPocketSetup1; // grayman #3559
extern const idEventDef AI_PickedPocketSetup2; // grayman #3559

extern const idEventDef AI_AlertAI; // grayman #3356

extern const idEventDef AI_GetAttacker; // grayman #3679
extern const idEventDef AI_IsPlayerResponsibleForDeath; // grayman #3679

class idPathCorner;

typedef struct particleEmitter_s {
	particleEmitter_s() {
		particle = NULL;
		time = 0;
		joint = INVALID_JOINT;
	};
	const idDeclParticle *particle;
	int					time;
	jointHandle_t		joint;
} particleEmitter_t;

class idAASFindCover : public idAASCallback {
public:
						idAASFindCover( const idActor* hidingActor, const idEntity* hideFromEnt, const idVec3 &hideFromPos );
	virtual				~idAASFindCover() override;

	virtual bool		TestArea( const idAAS *aas, int areaNum ) override;

private:
	const idActor*		hidingActor;
	const idEntity*		hideFromEnt;
	idVec3				hideFromPos;
};

class idAASFindAreaOutOfRange : public idAASCallback {
public:
						idAASFindAreaOutOfRange( const idVec3 &targetPos, float maxDist );

	virtual bool		TestArea( const idAAS *aas, int areaNum ) override;

private:
	idVec3				targetPos;
	float				maxDistSqr;
};

class idAASFindAttackPosition : public idAASCallback {
public:
						idAASFindAttackPosition(idAI *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &fireOffset);
	virtual				~idAASFindAttackPosition() override;

	virtual bool		TestArea( const idAAS *aas, int areaNum ) override;

private:
	idAI				*self;
	idEntity			*target;
	idBounds			excludeBounds;
	idVec3				targetPos;
	idVec3				fireOffset;
	idMat3				gravityAxis;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
};

class idAASFindObservationPosition : public idAASCallback {
public:
						idAASFindObservationPosition( const idAI *self, const idMat3 &gravityAxis, const idVec3 &targetPos, const idVec3 &eyeOffset, float maxDistanceFromWhichToObserve );
	virtual				~idAASFindObservationPosition() override;

	virtual bool		TestArea( const idAAS *aas, int areaNum ) override;

	// Gets the best goal result, even if it didn't meet the maximum distance
	bool getBestGoalResult
	(
		float& out_bestGoalDistance,
		aasGoal_t& out_bestGoal
	);

private:
	const idAI			*self;
	idBounds			excludeBounds;
	idVec3				targetPos;
	idVec3				eyeOffset;
	idMat3				gravityAxis;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
	float				maxObservationDistance;

	// The best goal found, even if it was greater than the maxObservationDistance
	float bestGoalDistance;
	bool b_haveBestGoal;
	aasGoal_t		bestGoal; 
};

class idAI : public idActor {
public:
	CLASS_PROTOTYPE( idAI );

							idAI();
	virtual					~idAI() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	//void					HeardSound( idEntity *ent, const char *action );
	idActor					*GetEnemy( void ) const;
	void					TalkTo( idActor *actor );
	talkState_t				GetTalkState( void ) const;

	bool					GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir );

	void					TouchedByFlashlight( idActor *flashlight_owner );

							// Outputs a list of all monsters to the console.
	static void				List_f( const idCmdArgs &args );

							// Finds a path around dynamic obstacles.
	static bool				FindPathAroundObstacles( const idPhysics *physics, const idAAS *aas, const idEntity *ignore, const idVec3 &startPos, const idVec3 &seekPos, obstaclePath_t &path, idActor* owner );
							// Frees any nodes used for the dynamic obstacle avoidance.
	static void				FreeObstacleAvoidanceNodes( void );
							// Predicts movement, returns true if a stop event was triggered.
	static bool				PredictPath( const idEntity *ent, const idAAS *aas, const idVec3 &start, const idVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path );
							// Return true if the trajectory of the clip model is collision free.
	static bool				TestTrajectory( const idVec3 &start, const idVec3 &end, float zVel, float gravity, float time, float max_height, const idClipModel *clip, int clipmask, const idEntity *ignore, const idEntity *targetEntity, int drawtime );
							// Finds the best collision free trajectory for a clip model.
	static bool				PredictTrajectory( const idVec3 &firePos, const idVec3 &target, float projectileSpeed, const idVec3 &projGravity, const idClipModel *clip, int clipmask, float max_height, const idEntity *ignore, const idEntity *targetEntity, int drawtime, idVec3 &aimDir );

	// Begin Dark Mod Functions:
	
	
	/**
	* Interface with Dark Mod Sound Propagation
	**/

	/**
	* Convert Sound Pressure Level from sound propagation
	* to psychoacoustic Loudness for the given AI
	* propVol is read from propParms, and 
	* loudness is set in propParms for later use.
	**/
	void SPLtoLoudness( SSprParms *propParms );
							
	/**
	* CheckHearing returns "true" if the sound is above
	* AI hearing threshold, without taking env. noise into 
	* account.
	**/
	bool CheckHearing( SSprParms *propParms );

	/**
	* Called by soundprop when AI hears a sound Assumes that CheckHearing
	* has been called and that the sound is above threshold without
	* considering environmental noise masking.
	**/
	void HearSound( SSprParms *propParms, float noise, const idVec3& origin );

	/**
	* Return the last point at which the AI heard a sound
	* Returns (0,0,0) if the AI didn't hear a sound.
	* Check AI_HEARDSOUND to see if the vector is valid.
	**/
	idVec3 GetSndDir( void );

	/**
	* Return the last point at which an AI glimpsed something suspicious.
	* Returns (0,0,0) if the AI was not visually alerted.
	* Check AI_VISALERT to see if the vector is valid.
	**/
	idVec3 GetVisDir( void );

	/**
	* Returns the entity that the AI is in tactile contact with
	**/
	idEntity *GetTactEnt( void );

	/**
	* Visual Alerts
	**/

	/**
	* Do a visibility calculation based on 2 things:
	* The lightgem value, and the distance to entity
	* NYI: take velocity into account
	*
	* The visibility can also be integrated over a number
	* of frames if we need to do that for optimization later.
	**/
	float GetVisibility( idEntity *ent ) const;

	/**
	* angua: The uncorrected linear light values [1..DARKMOD_LG_MAX] 
	* didn't produce very believable results when used 
	* in GetVisibility(). This function takes the linear values
	* and corrects them with an "empirical" correction curve.
	* 
	* @returns: a float between [0...1].It is fairly high at 
	* values above 20, fairly low below 6 and increases linearly in between.
	**/
	float GetVisFraction() const;

	/**
	* Checks enemies in the AI's FOV and calls Alert( "vis", amount )
	* The amount is calculated based on distance and the lightgem
	*
	* For now the check is only done on the player
	**/
	void PerformVisualScan( float time = 1.0f/60.0f );

	/**
	* Checks to see if the AI is being blocked by an actor when it tries to move,
	* call HadTactile this AI and if this is the case.
	*
	* greebo: Note: Only works if the AI is not Dead, KO or already engaged in combat.
	**/
	void CheckTactile();

	/**
	* Tactile Alerts:
	*
	* If no amount is entered, of the alert is defined in the global AI 
	* settings def file, and it also gets multiplied by the AI's specific
	* "acuity_tact" key in the spawnargs (defaults to 1.0)
	*
	* The amount is in alert units, so as usual 1 = barely noticible, 
	*	10 = twice as noticable, etc.
	**/

	void TactileAlert(idEntity* tactEnt, float amount = -1);

	/**
	* This is called in the frame if the AI bumped into another actor.
	* Checks the relationship to the AI, and calls TactileAlert appropriately.
	*
	* If the bumped actor is an enemy of the AI, the AI calls TactileAlert on itself
	*
	* If the bumped actor is an AI, and if this AI is an enemy of the bumped AI,
	* it calls TactileAlert on the bumped AI as well.
	**/
	void HadTactile( idActor *actor );

	// angua: ignore tactile alerts from this entity from now on
	void TactileIgnore(idEntity* tactEnt);

	bool CheckTactileIgnore(idEntity* tactEnt);

	/**
	* Generalized alerts and acuities
	**/

	/**
	* Alert the AI.  The first parameter is the alert type (same as acuity type)
	* The second parameter is the alert amount.
	* NOTE: For "alert units," an alert of 1 corresponds to just barely
	* seeing something or just barely hearing a whisper of a sound.
	**/
	void PreAlertAI(const char *type, float amount, idVec3 lookAt); // grayman #3356

	/**
	 * greebo: Sets the AI_AlertLevel of this AI and updates the AI_AlertIndex.
	 *
	 * This also updates the grace timers, alert times and checks for
	 * a valid agitatedsearching>combat transition.
	 *
	 */
	void SetAlertLevel(float newAlertLevel);


	// angua: returns true if the current alert index is higher 
	// than the previous one, false otherwise
	bool AlertIndexIncreased();

	
	void RegisterAlert(idEntity* alertedBy); // grayman #4002 - register an alert
	int ExamineAlerts(); // grayman #4002 - examine queued alerts

	/**
	* Returns the float val of the specific AI's acuity.
	* Acuity type is a char, from the same list as alert types.
	* That list is defined in DarkModGlobals.cpp.
	**/
	float GetBaseAcuity(const char *type) const; // grayman #3552

	/**
	* Returns the float val of the specific AI's acuity, accounting for
	* factors like drunkeness.
	* Acuity type is a char, from the same list as alert types.
	* That list is defined in DarkModGlobals.cpp.
	**/
	float GetAcuity( const char *type ) const;

	/**
	* Sets the AI acuity for a certain type of alert.
	**/
	void SetAcuity( const char *type, float acuity );

	/**
	* Calls objective system when the AI finds a body
	**/
	void FoundBody( idEntity *body );

	// Adds a message to the queue
	void AddMessage(const ai::CommMessagePtr& message, int msgTag); // grayman #3355

	// Removes all messages if msgTag == 0, else only removes all messages with the matching msgTag
	void ClearMessages(int msgTag); // grayman #3355

	bool CheckOutgoingMessages(ai::CommMessage::TCommType type, idActor* receiver); // grayman #3424

	/**
	* Get the volume modifier for a given movement type
	* Use the same function as idPlayer::GetMovementVolMod.
	* Unfortunately this is exactly the same as idPlayer::GetMovementVolMod
	* It's bad coding, but that's how D3 wrote the movement vars.
	**/
	virtual float GetMovementVolMod( void ) override;

	/**
	* Returns true if AI is knocked out
	**/
	virtual bool  IsKnockedOut( void ) override
	{
		return (AI_KNOCKEDOUT!=0);
	};

	/**
	* grayman #3525 - get eye position
	**/
	virtual idVec3 GetEyePosition( void ) const override;

	/** 
	* Ishtvan: Swap the AI's head CM to a larger one
	* (Used to make blackjacking easier, currently only called when a blackjack swings nearby)
	* If the argument is true, the larger CM is used, otherwise, the original CM is swapped back.
	**/
	void SwapHeadAFCM( bool bUseLargerCM );

	/**
	* Return a damage multiplier if a sneak attack has occurred
	**/
	virtual float			StealthDamageMult( void ) override;

	/**
	* Ishtvan: Overload AI target changing to re-initialize movement tasks
	**/
	virtual void			RemoveTarget(idEntity* target) override;
	virtual void			AddTarget(idEntity* target) override;

	// angua: calls the script functions for sitting down and getting up
	void SitDown();
	void FallAsleep();

	// GetUp is used both for getting up from sitting or sleeping
	void GetUp();

	// grayman #3820 - wake up w/o standing up
	void WakeUp();

	bool FitsThrough(CFrobDoor* frobDoor); // grayman #4412

public:
	/**
	* DarkMod AI Member Vars
	**/
	
	/**
	* Set to true if the AI has been alerted in this frame
	**/
	idScriptBool			AI_ALERTED;

	/**
	* Stores the actor ultimately responsible for the alert.
	* If they find a body, this gets set to whoever killed/KO'd the body.
	* If they get hit by an item, or hear it fall, this gets set to whoever 
	*	threw the item, and so on
	**/
	idEntityPtr<idActor>	m_AlertedByActor;

	int						alertTypeWeight[ai::EAlertTypeCount];
	/**
	* Set these flags so we can tell if the AI is running or creeping
	* for the purposes of playing audible sounds and propagated sounds.
	**/
	idScriptBool			AI_CROUCH;
	idScriptBool			AI_RUN;
	idScriptBool			AI_CREEP;

	// angua: this determines whether the AI should lay down to the left or to the right after sitting down
	// gets read as spawn arg from the path_sleep entity
	idScriptBool			AI_LAY_DOWN_LEFT;

	// angua: the direction the AI faces before sitting and laying down
	idScriptVector			AI_LAY_DOWN_FACE_DIR;

	// angua: the AI will turn to this direction after sitting down (this is practical for chairs next to a table)
	idScriptFloat			AI_SIT_DOWN_ANGLE;

	// angua: the AI will turn to this direction before getting up from sitting
	idScriptFloat			AI_SIT_UP_ANGLE;

	// greebo: This is to tell the scripts which idle animation should be played next in the CustomIdleAnim state
	idStr					m_NextIdleAnim;

	/****************************************************************************************
	*
	*	Added By Rich to implement AI Falling damage
	*	Called from Think with saved origin and velocity from before moving
	*   
	****************************************************************************************/
	//void CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ); // grayman #3699

	/**
	* greebo: Accessor methods for the airTicks member variable. 
	*/
	int		getAirTicks() const;
	void	setAirTicks(int airTicks);

	// greebo: Accessor methods for the array of subsystems
	ai::Subsystem* GetSubsystem(ai::SubsystemId id);

	ID_INLINE ai::MindPtr& GetMind()
	{
		return mind;
	}

	ID_INLINE ai::Memory& GetMemory() const
	{
		return mind->GetMemory();
	}

	ID_INLINE idAAS* GetAAS() const
	{
		return aas;
	}

	float GetArmReachLength();

	// Virtual override of idActor method, routes the call into the current Mind State
	virtual void NeedToUseElevator(const eas::RouteInfoPtr& routeInfo);

	// Switches this AI into conversation mode
	virtual bool SwitchToConversationState(const idStr& conversationName);


public:
	idLinkList<idAI>		aiNode;				// for being linked into gameLocal.spawnedAI list

	// greebo: When this AI has last re-evaluated a forbidden area (game time)
	int						lastAreaReevaluationTime;
	// The minimum time that needs to pass by before the AI re-evaluates a forbidden area (msec)
	int						maxAreaReevaluationInterval;
	// The time that needs to pass before locked doors are enabled for another try (msec)
	int						doorRetryTime;

protected:
	// navigation
	idAAS *					aas;
	int						travelFlags;

	idMoveState				move;
	idMoveState				savedMove;

	// A stack of movestates, used for saving and restoring moves in PushMove() and PopMove()
	std::list<idMoveState>	moveStack;

	float					kickForce;
	bool					ignore_obstacles;
	float					blockedRadius;
	int						blockedMoveTime;
	int						blockedAttackTime;

	// turning
	float					ideal_yaw;
	float					current_yaw;
	float					turnRate;
	float					turnVel;
	float					anim_turn_yaw;
	float					anim_turn_amount;
	float					anim_turn_angles;

	// angua: the offset of the pivot for turning while seated
	idVec3					sitting_turn_pivot;

	// This expands the AABB a bit when the AI is checking for reached positions.
	float					reachedpos_bbox_expansion;

	// greebo: Defines the maximum vertical tolerance within wich a point above an AAS area is still considered reachable.
	// This is used by PathToGoal to judge whether an area is reachable or not.
	float					aas_reachability_z_tolerance;

	// physics
	idPhysics_Monster		physicsObj;

	// flying
	jointHandle_t			flyTiltJoint;
	float					fly_speed;
	float					fly_bob_strength;
	float					fly_bob_vert;
	float					fly_bob_horz;
	int						fly_offset;					// prefered offset from player's view
	float					fly_seek_scale;
	float					fly_roll_scale;
	float					fly_roll_max;
	float					fly_roll;
	float					fly_pitch_scale;
	float					fly_pitch_max;
	float					fly_pitch;

	bool					allowMove;					// disables any animation movement
	bool					allowHiddenMovement;		// allows character to still move around while hidden
	bool					disableGravity;				// disables gravity and allows vertical movement by the animation
	
	// weapon/attack vars
	bool					lastHitCheckResult;
	int						lastHitCheckTime;
	int						lastAttackTime;
	float					fire_range;
	float					projectile_height_to_distance_ratio;	// calculates the maximum height a projectile can be thrown
	idList<idVec3>			missileLaunchOffset;

	struct ProjectileInfo
	{
		const idDict*	def;
		idStr			defName;
		idClipModel*	clipModel;
		float			radius;
		float			speed;
		idVec3			velocity;
		idVec3			gravity;

		ProjectileInfo() :
			def(NULL),
			clipModel(NULL),
			radius(-1),
			speed(-1),
			velocity(0),
			gravity(0)
		{}

		~ProjectileInfo()
		{
			// Take care of any allocated clipmodels on destruction
			delete clipModel;
		}
	};

	/**
	 * greebo: An AI can have multiple projectile defs in their entityDef declaration.
	 * For some routines the properties of each projectile need to be cached, like
	 * radius, clipmodel and gravity, this is stored in the ProjectileInfo structure.
	 *
	 * After each CreateProjectile() call the "current projectile" index is recalculated
	 * and might point to a different projectile for the next round of firing.
	 */
	idList<ProjectileInfo>	projectileInfo;

	// The index of the "current projectile", is recalculated each time CreateProjectile() is called.
	int						curProjectileIndex;

	/**
	 * The currently active projectile. It holds the spawned
	 * projectile entity, which can either be generated from the
	 * list of possible ProjectileInfo structures above or a custom
	 * one which has been spawned directly via script or frame commands.
	 *
	 * The ActiveProjectile structure is filled either by calling CreateProjectile()
	 * or by methods like Event_CreateThrowableProjectile()
	 */
	struct ActiveProjectile
	{
		// The projectile parameter structure
		ProjectileInfo info;
		
		// The currently active projectile entity (might be NULL)
		idEntityPtr<idProjectile> projEnt;
	};

	// The currently active projectile, might be an empty structure
	ActiveProjectile		activeProjectile;

	idStr					attack;

	/**
	 * grayman #2603 - Per-stim info for relighting lights. This allows AI to
	 * keep track of delays for responding to visual stims. The delays are
	 * determined by conditions surrounding a relight and the probabilities
	 * associated with relighting.
	 */
	struct DelayedStim
	{
		// The next time to consider the stim
		int nextTimeToConsider;
		
		// The stim being delayed
		idEntityPtr<idEntity> stim;
	};

	idList<DelayedStim>		delayedStims; // grayman #2603

	// chatter/talking
	talkState_t				talk_state;
	idEntityPtr<idActor>	talkTarget;

	// cinematics
	int						num_cinematics;
	int						current_cinematic;

	bool					allowJointMod;
	idEntityPtr<idEntity>	focusEntity;
	idVec3					currentFocusPos;
	int						focusTime;
	int						alignHeadTime;
	int						forceAlignHeadTime;
	idAngles				eyeAng;
	idAngles				lookAng;
	idAngles				destLookAng;
	idAngles				lookMin;
	idAngles				lookMax;
	idList<jointHandle_t>	lookJoints;
	idList<idAngles>		lookJointAngles;
	idList<jointHandle_t>	lookJointsCombat;
	idList<idAngles>		lookJointAnglesCombat;
	float					eyeVerticalOffset;
	float					eyeHorizontalOffset;
	float					eyeFocusRate;
	float					headFocusRate;
	int						focusAlignTime;

	// special fx
	float					shrivel_rate;
	int						shrivel_start;
	
	bool					restartParticles;			// should smoke emissions restart
	bool					useBoneAxis;				// use the bone vs the model axis
	idList<particleEmitter_t> particles;				// particle data

	renderLight_t			worldMuzzleFlash;			// positioned on world weapon bone
	int						worldMuzzleFlashHandle;
	jointHandle_t			flashJointWorld;
	int						muzzleFlashEnd;
	int						flashTime;

	// joint controllers
	idAngles				eyeMin;
	idAngles				eyeMax;
	jointHandle_t			focusJoint;
	jointHandle_t			orientationJoint;

	typedef std::set<CBinaryFrobMover*> FrobMoverList;
	FrobMoverList			unlockableDoors;

	typedef std::set<idEntity*> TactileIgnoreList;
	TactileIgnoreList		tactileIgnoreEntities;

public: // greebo: Made these public
	// enemy variables
	idEntityPtr<idActor>	enemy;
	idVec3					lastVisibleEnemyPos;
	idVec3					lastVisibleEnemyEyeOffset;
	idVec3					lastVisibleReachableEnemyPos;
	idVec3					lastReachableEnemyPos;
	bool					enemyReachable;
	bool					wakeOnFlashlight;
	int						lastUpdateEnemyPositionTime;

	// grayman #2887 - next 2 are used to determine amount of "busted" time

	int						lastTimePlayerSeen;
	int						lastTimePlayerLost;

	// grayman #3317 - if this is TRUE, we're fleeing an event (like witnessing a murder),
	// and don't necessarily have an enemy.

	bool					fleeingEvent;

	// grayman #3848 - where you're fleeing from
	idVec3					fleeingFrom;

	// grayman #3847 - who you're fleeing from
	idEntityPtr<idActor>	fleeingFromPerson;

	// grayman #3474 - if this is TRUE, emit fleeing barks, otherwise don't

	bool					emitFleeBarks;
	
	idVec3					lastSearchedSpot; // grayman #4220 - most recently searched spot

public: // greebo: Made these public for now, I didn't want to write an accessor for EVERYTHING
	// script variables
	idScriptBool			AI_TALK;
	idScriptBool			AI_DAMAGE;
	idScriptBool			AI_PAIN;
	idScriptFloat			AI_SPECIAL_DAMAGE;
	//idScriptBool			AI_DEAD; // is defined on idActor now
	idScriptBool			AI_KNOCKEDOUT;
	idScriptBool			AI_ENEMY_VISIBLE;
	idScriptBool			AI_ENEMY_TACTILE;
	idScriptBool			AI_ENEMY_IN_FOV;
	idScriptBool			AI_MOVE_DONE;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ACTIVATED;
	idScriptBool			AI_FORWARD;
	idScriptBool			AI_JUMP;
	idScriptBool			AI_BLOCKED;
	idScriptBool			AI_OBSTACLE_IN_PATH;
	idScriptBool			AI_DEST_UNREACHABLE;
	idScriptBool			AI_HIT_ENEMY;
	idScriptBool			AI_PUSHED;

	/**
	* The following variables are set as soon as the AI
	* gets a certain type of alert, and never unset by the
	* game code.  They are only unset in scripting.  This is
	* to facilitate different script reactions to different kinds
	* of alerts.
	*
	* It's also done this way so that the AI will know if it has been
	* alerted even if it happened in a frame that the script did not check.
	*
	* This is to facilitate optimization by having the AI check for alerts
	* every N frames rather than every frame.
	**/


	/**
	* Set to true if the AI heard a suspicious sound.
	**/
	idScriptBool			AI_HEARDSOUND;

	/**
	* Set to true if the AI saw something suspicious.
	**/
	idScriptBool			AI_VISALERT;

	/**
	* Set to true if the AI was pushed by or bumped into an enemy.
	**/
	idScriptBool			AI_TACTALERT;

	/**
	* The current alert number of the AI.
	* This is checked to see if the AI should
	* change alert indices.  This var is very important!
	* NOTE: Don't change this directly. Instead, call Event_SetAlertLevel
	* to change it.
	**/
	idScriptFloat			AI_AlertLevel;
	
	/**
	* Current alert index of the AI. Is set based on AI_AlertLevel and the alert threshold values:
	* 	ai::ERelaxed           == 0 if AI_AlertLevel < thresh_1
	* 	ai::EObservant         == 1 if thresh_1 <= AI_AlertLevel < thresh_2
	* 	ai::ESuspicious        == 2 if thresh_2 <= AI_AlertLevel < thresh_3
	* 	ai::ESearching     == 3 if thresh_3 <= AI_AlertLevel < thresh_4
	* 	ai::EAgitatedSearching == 4 if thresh_4 <= AI_AlertLevel < thresh_5
	*   ai::ECombat            == 5 if thresh_5 <= AI_AlertLevel (and an enemy is known)
	**/
	idScriptFloat			AI_AlertIndex;

	/**
	* Whether the AI is sleeping on the floor (0), on a bed (1) or in a chair (2)
	**/
	idScriptFloat			AI_SleepLocation; // grayman #3820
	
	/**
	* Stores the amount alerted in this frame
	* Used to compare simultaneous alerts, the smaller one is ignored
	* Should be cleared at the start of each frame.
	**/
	float					m_AlertLevelThisFrame;

	// grayman #3520 - an alert has occurred, and depending on State,
	// the AI might want to look at the alert spot
	bool					m_lookAtAlertSpot;
	idVec3					m_lookAtPos; // the spot to look at

	// angua: stores the previous alert index at alert index changes
	int						m_prevAlertIndex;

	// angua: the highest alert level/index the AI reached already 
	float						m_maxAlertLevel;
	int							m_maxAlertIndex;

	// angua: the alert level the AI had after the last alert level increase
	// grayman #3472 - to remove ambiguity, and confusion with m_maxAlertLevel, change the name to reflect the
	// variable's true meaning: when rising from Idle or AlertIdle into the
	// higher alert states, and returning back to Idle or AlertIdle, m_recentHighestAlertLevel
	// holds the highest alert level achieved. When returning to Idle or Alert Idle,
	// and after emitting a relevant rampdown bark, this variable is reset to 0, the
	// lowest alert level.
	float						m_recentHighestAlertLevel;

	/**
	* If true, the AI ignores alerts during all actions
	**/
	bool					m_bIgnoreAlerts;

	/**
	* Is AI drunk?
	* Same as spawnargs.GetBool("drunk"), but a bit faster.
	**/
	bool					m_drunk;

	/**
	* Array containing the various AI acuities (visual, aural, tactile, etc)
	**/
	idList<float>			m_Acuities;


	float					m_oldVisualAcuity;	// Tels: fixes 2408

	float					m_sleepFloorZ;  // grayman #2416
	int						m_getupEndTime; // grayman #2416

	/**
	* Audio detection threshold (in dB of Sound Pressure Level)
	* Sounds heard below this volume will be ignored (default is 20 dB)
	* Soundprop only goes down to 15 dB, although setting it lower than
	* this will still have some effect, since alert = volume - threshold
	**/
	float					m_AudThreshold;

	/**
	* The loudest direction for the last suspicious sound the AI heard
	* is set to NULL if the AI has not yet heard a suspicious sound
	* Note suspicious sounds that are omnidirectional do not set this.
	* If no sound has been propagated it will be (0,0,0).
	**/
	idVec3					m_SoundDir;

	/**
	* Position of the last visual alert
	**/
	idVec3					m_LastSight;

	/**
	* The entity that last issued a tactile alert
	**/
	idEntityPtr<idEntity>	m_TactAlertEnt;

	/**
	* Alert Grace period variables :
	* Actor that the alert grace period applies to:
	**/
	idEntityPtr<idActor>	m_AlertGraceActor;

	/**
	* Time of the grace period start [ms]
	**/
	int						m_AlertGraceStart;

	/**
	* Duration of the grace period [ms]
	**/
	int						m_AlertGraceTime;

	/**
	* Alert number below which alerts are ignored during the grace period
	**/
	float					m_AlertGraceThresh;

	/**
	* Number of alerts ignored in this grace period
	**/
	int						m_AlertGraceCount;

	/**
	* Number of alerts it takes to override the grace period via sheer number
	* (This is needed to make AI rapidly come up to alert due to visual alert,
	* because visual alerts do not increase in magnitude but just come in more rapidly
	**/
	int						m_AlertGraceCountLimit;

	/**
	 * greebo: This is the message "outbox" of an AI. During sound propagation
	 * the messages are traversed and delivered to the "recipient" AI.
	 * Once delivered, messages are automatically removed from this list.
	 *
	 * Use AddMessage() to store new messages here.
	 */
	ai::MessageList			m_Messages;

	/**
	* The spots resulting from the current search or gotten from
	* another AI.
	*/
	//CDarkmodHidingSpotTree	m_hidingSpots; // grayman #3857

	// An array of random numbers serving as indexes into the hiding spot list.
	//std::vector< int >		m_randomHidingSpotIndexes; // grayman #3857

	/**
	* Used for drowning
	**/
	int						m_AirCheckTimer;

	bool					m_bCanDrown;

	/**
	* Head body ID on the AF, used by drowning
	**/
	int						m_HeadBodyID;

	/**
	* Head joint ID on the living AI (used by FOV and KOing)
	**/
	jointHandle_t			m_HeadJointID;

	/**
	* Some AI have a different head CM while conscious.  
	* This variable stores the original head CM from the AF, 
	* to set it back when they die/get ko'd.
	**/
	idClipModel				*m_OrigHeadCM;
	/** If true, head was swapped when alive and needs to be swapped back when ragdolled **/
	bool					m_bHeadCMSwapped;

	/**
	* Knockout Data
	* Name of damage location within which a KO is possible
	**/
	idStr					m_KoZone;
	/**
	* Alert state above which their KO Immunity changes (if any)
	**/
	int					m_KoAlertImmuneState;
	/**
	* Alert state above which their KO behavior changes (if any)
	**/
	int					m_KoAlertState;
	/**
	* True if AI is completely immune to KO when alerted above the given alert state
	**/
	bool					m_bKoAlertImmune;
	/**
	* Cosines of the vertical and horizontal angles, and the same when alert
	**/
	float					m_KoDotVert;
	float					m_KoDotHoriz;
	float					m_KoAlertDotVert;
	float					m_KoAlertDotHoriz;
	/**
	* Rotates the KO cone relative to the head joint
	**/
	idMat3					m_KoRot;

	/**
	* Current number of air ticks left for drowning
	**/
	int						m_AirTics;

	/**
	* Max number of air ticks for drowning
	**/
	int						m_AirTicksMax;

	/**
	* number of seconds between air checks
	**/
	int						m_AirCheckInterval;

	/**
	* Set to true if the AI can be KO'd (defaults to true)
	**/
	bool					m_bCanBeKnockedOut;

	/**
	* Set to true if the AI can be gassed (defaults to true)
	**/
	bool					m_bCanBeGassed; // grayman #2468

	/**
	* How the AI was knocked out
	**/
	koState_t				m_koState;		// grayman #2604

	/**
	* Keep track of initial thinking frame count - important when a cinematic starts the mission
	**/
	int						m_earlyThinkCounter;	// grayman #2654

	/**
	* grayman #3063 - flag for ignoring a player sighting prior to entering Combat mode. For mission statistics purposes.
	**/
	bool					m_ignorePlayer;

	/**
	* grayman #2603 - if TRUE, don't allow movement extrication
	**/

	bool					m_bCanExtricate;

	/**
	* grayman #2422 - search volume when searching
	 **/
	 
	//idBounds				m_searchLimits; // grayman #3857 - no longer used; limits are kept in the search assignment
	
	/**
	 * greebo: Is set to TRUE if the AI is able to open/close doors at all.
	 */
	bool					m_bCanOperateDoors;

	/**
	 * Daft Mugi #6460: Is set to TRUE if the AI is able to close doors.
	 */
	bool					m_bCanCloseDoors;

	/**
	 * angua: is set true while the AI is handling the door.
	 */
	bool					m_HandlingDoor;

	 
	/**
	 * grayman #3647 - is set true if a door handling task has been queued but not yet started
	 */
	bool					m_DoorQueued;
	
 	/**
	 * grayman #3647 - is set true if an elevator handling task has been queued but not yet started
	 */
	bool					m_ElevatorQueued;

	/**
	 * grayman #3559 - is set true if the AI is in a conversation
	 */
	bool					m_InConversation;

	/**
	 * grayman #5164 - the next time a warning can be issued about an AI not being able to reach its starting sit/sleep location
	 */
	int						m_nextWarningTime;

	/**
	 * grayman #2706: is set true when the move prior to door handling is saved
	 */
	bool					m_RestoreMove;

	/**
	 * grayman #2603: is set true when a search needs to be set up after relighting a light
	 */
	bool					m_LatchedSearch;

	/**
	 * grayman #2603: list of doused lights seen
	 */
	idList< idEntityPtr<idEntity> >		m_dousedLightsSeen;

	/**
	 * grayman #3681: list of noisemakers heard
	 */
	idList< idEntityPtr<idEntity> >		m_noisemakersHeard;

	/**
	 * angua: is set true while the AI is handling an elevator.
	 */
	bool					m_HandlingElevator;

	/**
	 * grayman #3029: is set true if the AI can set up to handle a door while handling an elevator,
	 * allowed only during the approach states of elevator handling
	 */
	bool					m_CanSetupDoor;

	/**
	 * grayman #2603: is set true while the AI is relighting a light.
	 */
	bool					m_RelightingLight;

	/**
	 * grayman #2872: is set to true while the AI is examining a rope
	 */
	bool					m_ExaminingRope;

	/**
	 * grayman #2603: is set true while the AI is dropping a torch.
	 */
	bool					m_DroppingTorch;

	/**
	 * grayman #3559: is set true while the AI is reacting to having something stolen from him.
	 */
	bool					m_ReactingToPickedPocket;

	/**
	* Head center offset in head joint coordinates, relative to head joint
	* When this offset is added to the head joint, we should be at the head center
	**/
	idVec3					m_HeadCenterOffset;

	/**
	* Rotates the FOV cone relative to the head joint
	**/
	idMat3					m_FOVRot;

	/**
	* Set to true if we want this AI to push off the player when the
	* player ends up standing on top of them
	* Applies only when the AI is alive
	**/
	bool					m_bPushOffPlayer;

	/**
	* Ishtvan: Flat-footedness, disallows movement while maitaining ability to turn
	* Currently only implemented in melee
	* The following are timers for being flat-footed in all situations
	**/
	bool					m_bCanBeFlatFooted;
	bool					m_bFlatFooted;
	int						m_FlatFootedTimer;
	int						m_FlatFootedTime;
	/**
	* The following apply to being flat-footed as a result of a parry only
	**/
	int						m_FlatFootParryNum; // runtime tracking
	int						m_FlatFootParryMax; // spawnarg cap
	int						m_FlatFootParryTimer; // runtime tracking
	int						m_FlatFootParryTime; // spawnarg cap [ms]
	/**
	* Melee: Chance to counter attack into an enemy attack instead of parrying:
	**/
	float					m_MeleeCounterAttChance;
	/**
	* Does this AI try to predict when the enemy will be close enough
	* in the future and start their melee swing in advance?
	**/
	bool					m_bMeleePredictProximity;
	
	// AI_AlertLevel thresholds for each alert level
	// Alert levels are: 1=slightly suspicious, 2=aroused, 3=investigating, 4=agitated investigating, 5=hunting
	float thresh_1, thresh_2, thresh_3, thresh_4, thresh_5;
	// Grace period info for each alert level
	float m_gracetime_1, m_gracetime_2, m_gracetime_3, m_gracetime_4;
	float m_gracefrac_1, m_gracefrac_2, m_gracefrac_3, m_gracefrac_4;
	int m_gracecount_1, m_gracecount_2, m_gracecount_3, m_gracecount_4;
	// De-alert times for each alert level
	float atime1, atime2, atime3, atime4, atime_fleedone;

	bool m_canSearch; // grayman #3069

	float atime1_fuzzyness, atime2_fuzzyness, atime3_fuzzyness, atime4_fuzzyness, atime_fleedone_fuzzyness;

	// angua: Random head turning
	int m_timeBetweenHeadTurnChecks;
	float m_headTurnChanceIdle;
	float m_headTurnFactorAlerted;
	float m_headTurnMaxYaw;
	float m_headTurnMaxPitch;
	int m_headTurnMinDuration;
	int m_headTurnMaxDuration;

	idEntity* m_tactileEntity;	// grayman #2345 - something we bumped into this frame, not necessarily an enemy
	bool m_canResolveBlock;		// grayman #2345 - whether we can resolve a block if asked
	bool m_leftQueue;			// grayman #2345 - if we timed out waiting in a door queue
	bool m_performRelight;		// grayman #2603 - set to TRUE by a script function when it's time to relight a light
	idEntityPtr<idEntity> m_bloodMarker;	// grayman #3075
	bool m_ReactingToHit;		// grayman #2816 - reaction after being hit by something
	idEntityPtr<idActor> m_lastKilled; // grayman #2816 - the last enemy we killed
	bool m_justKilledSomeone;	// grayman #2816 - remember just killing someone so correct bark is emitted when alert level comes down
	bool m_deckedByPlayer;		// grayman #3314 - TRUE if the player killed or KO'ed me (for mission statistics "bodies found")
	bool m_allowAudioAlerts;	// grayman #3424 - latch to prevent audio alert processing

	// grayman #3857 - search manager
	int m_searchID;				// which search he's assigned to; index into the Search Manager's list of active searches

	/**
	* grayman #4046 - saved _endtime for interrupted path_wait task
	**/
	float m_pathWaitTaskEndtime;

	/**
	 * grayman #4002 - Alert time information per entity.
	 */
	struct EntityAlert
	{
		// The last time an entity raised the alert index
		int timeAlerted;
		
		// The alert index reached
		int alertIndex;

		// The entity responsible
		idEntityPtr<idEntity> entityResponsible;
		//int entityNumber;

		// Alert entry should be ignored?
		bool ignore;
	};

	idList<EntityAlert> alertQueue; // grayman #4002

	// The mind of this AI
	ai::MindPtr mind;

	// The array of subsystems of this AI
	ai::SubsystemPtr senseSubsystem;
	ai::MovementSubsystemPtr movementSubsystem;
	ai::SubsystemPtr actionSubsystem;
	ai::CommunicationSubsystemPtr commSubsystem;
	ai::SubsystemPtr searchSubsystem; // grayman #3857

	// greebo: The names of the backbone states, one for each alert state
	// e.g. ECombat => "Combat"
	typedef std::map<ai::EAlertState, idStr> BackboneStateMap;
	BackboneStateMap backboneStates;

	void					SetAAS( void );
	virtual	void			DormantBegin( void ) override;	// called when entity becomes dormant
	virtual	void			DormantEnd( void ) override;	// called when entity wakes from being dormant
	virtual void			Think( void ) override;
	virtual void			Activate( idEntity *activator ) override;
	int						ReactionTo( const idEntity *ent );
	bool					CheckForEnemy( void );
	void					EnemyDead( void );

	
	/** 
	 * angua: Interleaved thinking optimization
	 * AI will only think once in a certain number of frames
	 * depending on player distance and whether the AI is in the player view
	 * (this includes movement, pathing, physics and the states and tasks)
	 */

	// This checks whether the AI should think in this frame
	bool					ThinkingIsAllowed();

	// Sets time moment when the AI should think next time
	void					SetNextThinkFrame();

	int						GetMaxInterleaveThinkFrames() const;
	// returns interleave think frames
	// the AI will only think once in this number of frames (note: frame = 16 ms)
	int						GetThinkInterleave() const; // grayman 2414 - add 'const'
	int						m_nextThinkTime;

	// Below min dist, the AI thinks normally every frame.
	// Above max dist, the thinking frequency is given by max interleave think frames.
	// The thinking frequency increases linearly between min and max dist.
	int						m_maxInterleaveThinkFrames;
	float					m_minInterleaveThinkDist;
	float					m_maxInterleaveThinkDist;

	// the last time where the AI did its thinking (used for physics)
	int						m_lastThinkTime;

	// grayman #2691 - this checks if a doorway is large enough to fit through when the door is fully open
	bool					CanPassThroughDoor(CFrobDoor* frobDoor);

	// grayman #2603 - am I carrying a torch?
	idEntity*				GetTorch();

	// grayman #3559 - am I carrying a lantern?
	idEntity*				GetLantern();

	bool					IsSearching(); // grayman #2603

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;
	virtual bool			CanBecomeSolid();
	idVec3					FirstVisiblePointOnPath( const idVec3 origin, const idVec3 &target, int travelFlags );
	void					CalculateAttackOffsets( void );
	void					PlayCinematic( void );

	// movement
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) override;
	void					GetMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta );
	void					CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos );
	void					DeadMove( void );
	void					AnimMove( void );
	void					SlideMove( void );
	void					SittingMove();
	void					NoTurnMove();
	void					SleepingMove();
	void					AdjustFlyingAngles( void );
	void					AddFlyBob( idVec3 &vel );
	void					AdjustFlyHeight( idVec3 &vel, const idVec3 &goalPos );
	void					FlySeekGoal( idVec3 &vel, idVec3 &goalPos );
	void					AdjustFlySpeed( idVec3 &vel );
	void					FlyTurn( void );
	void					FlyMove( void );
	void					StaticMove( void );

	// greebo: Overrides idActor::PlayFootStepSound()
	virtual void			PlayFootStepSound() override;

	// greebo: Plays the given bark sound (will override any sounds currently playing)
	virtual void			Bark(const idStr& soundName);

	// damage
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const idDict* damageDef ) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale, const int location,
								trace_t *collision = NULL) override;

	void					DropBlood(idEntity *inflictor);
	void					SpawnBloodMarker(const idStr& splat, const idStr& splatFading, float size);

	void					PostDeath();

	/**
	* Copy knockout spawnargs from the head entity (assumes head exists)
	**/
	void					CopyHeadKOInfo(void);

	/**
	* Parse knockout spawnargs
	**/
	void					ParseKnockoutInfo( void );

	/**
	* TestKnockOutBlow is called when the AI is hit with a weapon with knockout capability.
	* This function tests the location hit, angle of the blow, and alert state of the AI.
	*
	* The "dir" vector is from the knockback of the weapon.  It is not used for now.
	*
	* tr is the trace from the weapon collision, location is the joint handle that was hit
	* bIsPowerBlow is set if the BJ was powered up
	*
	* Returns false if BJ attempt failed, or if already knocked out
	*
	* Note: overrides idActor::TestKnockoutBlow.
	**/
	virtual bool			TestKnockoutBlow( idEntity* attacker, const idVec3& dir, trace_t *tr, int location, bool bIsPowerBlow, bool performAttack = true ) override;
	
	/**
	* Can the AI greet someone?
	**/
	virtual bool			CanGreet() override; // grayman #3338

	/**
	* grayman #3559 - react to having had something stolen
	**/
	void					PocketPicked();

	/**
	* grayman #3559 - setup methods for picking a pocket
	**/
	void					Event_PickedPocketSetup1();
	void					Event_PickedPocketSetup2();

	/**
	* Tells the AI to go unconscious.  Called by TestKnockoutBlow if successful,
	* also can be called by itself and is called by scriptevent AI_KO_Knockout.
	*
	* @inflictor: This is the entity causing the knockout, can be NULL for "unknown originator".
	**/
	void					Event_KO_Knockout( idEntity* inflictor );

	/**
	* grayman #2468 - Tells the AI to go unconscious.  Called by scriptevent AI_Gas_Knockout.
	*
	* @inflictor: This is the entity causing the knockout, can be NULL for "unknown originator".
	**/
	void					Event_Gas_Knockout( idEntity* inflictor );

	void					Fall_Knockout( idEntity* inflictor ); // grayman #3699

	/**
	* Tells the AI to go unconscious.
	*
	* @inflictor: This is the entity causing the knockout, can be NULL for "unknown originator".
	**/
	void					Knockout( idEntity* inflictor );

	/**
	 * greebo: Does a few things after the knockout animation has finished.
	 *         This includes setting the model, dropping attachments and starting ragdoll mode.
	 *         Note: Gets called by the Mind's KnockOutState.
	 */
	void					PostKnockOut();

	/**
	* Drop certain attachments and def_drop items when transitioning into a ragdoll state
	* Called by idActor::Killed and idActor::KnockedOut
	**/
	void					DropOnRagdoll( void );

	// navigation
	void					KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick );

	// greebo: For Documentation, see idActor class.
	virtual bool			ReEvaluateArea(int areaNum) override;

	/**
	 * greebo: ReachedPos checks whether we the AI has reached the given target position <pos>.
	 *         The check is a bounds check ("bounds contain point?"), the bounds radius is 
	 *         depending on the given <moveCommand>.
	 *
	 * @returns: TRUE when the position has been reached, FALSE otherwise.
	 */
	bool					ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand) const;

	// greebo: Bounding box check to see if the AI has reached the given position. Returns TRUE if position reached.
	bool					ReachedPosAABBCheck(const idVec3& pos) const;

	float					TravelDistance( const idVec3 &start, const idVec3 &end );

	/**
	 * greebo: Returns the number of the nearest reachable area for the given point. 
	 *         Depending on the move type, the call is routed to the AAS->PointReachableAreaNum 
	 *         function. The bounds are modified before submission to the AAS function.
	 *
	 * @returns: the areanumber of the given point, 0 == no area found.
	 */
	int						PointReachableAreaNum(const idVec3 &pos, const float boundsScale = 2.0f, const idVec3& offset = idVec3(0,0,0)) const;

	bool					PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, idActor* actor ) const;
	void					DrawRoute( void ) const;
	bool					GetMovePos( idVec3 &seekPos );
	bool					MoveDone( void ) const;

	ID_INLINE moveType_t	GetMoveType() const
	{
		return move.moveType;
	}

	ID_INLINE int			GetMoveStartTime() const // grayman #3492
	{
		return move.startTime;
	}

	void					SetMoveType( int moveType );
	void					SetMoveType( idStr moveType );

	void					SetMoveAccuracy(float accuracy); // grayman #4039
	
	/**
	* This is a virtual override of the idActor method.  It takes lighting levels into consideration
	* additional to the ordinary FOV/trace check in idActor.
	*/
	virtual bool			CanSee( idEntity *ent, bool useFOV ) const override;


	/**
	* This version can optionally use or not use lighting and fov
	*/
	bool					CanSeeExt ( idEntity* ent, const bool useFOV, const bool useLighting ) const;

	/**
	* This tests if a position is visible.  it can optionally use lighting and fov.
	*/
	bool					CanSeePositionExt( idVec3 position, const bool useFOV, const bool useLighting );

	bool					EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos );

	bool					CanSeeTargetPoint( idVec3 point, idEntity* target , bool checkLighting) const; // grayman #2859 & #2959

	idVec3					CanSeeRope( idEntity *ent ) const; // grayman #2872

	float					GetReachTolerance(); // grayman #3029

	// angua: if the focusTime > gameLocal.time, the AI is currently looking at a specified entity or location
	ID_INLINE int			GetFocusTime()
	{
		return focusTime;
	}

	// grayman #3473
	ID_INLINE void			SetFocusTime(int t)
	{
		focusTime = t;
	}

	void					BlockedFailSafe( void );
	/**
	* Overloaded idActor::CheckFOV with FOV check that depends on head joint orientation
	**/
	virtual bool			CheckFOV( const idVec3 &pos ) const override;

	/**
	* Darkmod enemy tracking: Is an entity shrouded in darkness?
	* @author: SophisticatedZombie, tels
	*
	* The sightThreshold goes from 0.0f to 1.0f. Uses FOV, distance and lighting of the
	* entity to determine visibility.
	*
	* @return true if the entity is in darkness
	* @return false if not
	*/
	bool IsEntityHiddenByDarkness (idEntity* p_entity, const float sightThreshold) const;

	/**
	 * greebo: Returns TRUE if the entity is within the "attack_cone".
	 */
	bool EntityInAttackCone(idEntity* entity);

	/**
	 * Returns TRUE or FALSE, depending on the distance to the 
	 * given entity and the weapons attached to this AI.
	 * Melee AI normally perform a bounding box expansion check,
	 * ranged AI implement a visual test incl. lighting.
	 */
	bool CanHitEntity(idActor* entity, ECombatType combatType = COMBAT_NONE);
	/**
	* Will we be able to hit an enemy that is not currently in reach but coming into reach
	* Similar to CanHitEntity, but uses predicted future positions based on velocity
	* and a given prediction time.  Only applies to melee combat
	**/
	bool WillBeAbleToHitEntity(idActor* entity, ECombatType combatType = COMBAT_NONE);

	/**
	 * Returns TRUE or FALSE, depending on the distance to the 
	 * given entity and the weapons attached to it.
	 * May include other factors such as relative velocity
	 * Ranged combat NYI (may overlap with existing "take cover" algorithms)
	 */
	bool CanBeHitByEntity(idActor* entity, ECombatType combatType = COMBAT_NONE);

	/**
	 * greebo: This updates the weapon attachment's "solid" status.
	 * By design, AI have solid weapons only when in searching states.
	 */
	void UpdateAttachmentContents(bool makeSolid);


	// movement control
	void					SetStartTime(idVec3 pos); // grayman #3993
	void					StopMove( moveStatus_t status );
	bool					FaceEnemy( void );
	bool					FaceEntity( idEntity *ent );
	bool					DirectMoveToPosition( const idVec3 &pos );
	bool					MoveToEnemyHeight( void );
	bool					MoveOutOfRange( idEntity *entity, float range );
	const idVec3&			GetMoveDest() const;
	idEntity*				GetTactileEntity(void); // grayman #2345

	/**
	 * greebo: Flee from the given entity. Pass the maximum distance this AI should search escape areas in.
	 * grayman #3317 - can also flee a murder or KO, w/o having an enemy
	 */
	bool					Flee(idEntity* entityToFleeFrom, bool fleeingEvent, int algorithm, int distanceOption); // grayman #3317
	bool					FindAttackPosition(int pass, idActor* enemy, idVec3& targetPoint, ECombatType type); // grayman #3507
	aasGoal_t				GetPositionWithinRange(const idVec3& targetPos);
	idVec3					GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne, const unsigned short maxCost); // grayman #4347
	bool					MoveToAttackPosition( idEntity *ent, int attack_anim );
	bool					MoveToEnemy( void );
	bool					MoveToEntity( idEntity *ent );
	bool					IsAfraid( void ); // grayman #3848

	virtual CMultiStateMover* OnElevator(bool mustBeMoving) const override;

	/**
	 * greebo: This moves the entity to the given point.
	 *
	 * @returns: FALSE, if the position is not reachable (AI_DEST_UNREACHABLE && AI_MOVE_DONE == true)
	 * @returns: TRUE, if the position is reachable and the AI is moving (AI_MOVE_DONE == false) 
	 *                 OR the position is already reached (AI_MOVE_DONE == true).
	 */
	bool					MoveToPosition( const idVec3 &pos, float accuracy = -1 );

	/**
	 * angua: This looks for a suitable position for taking cover
	 *
	 * @returns: FALSE, if no suitable position is found
	 * @returns: TRUE, if the position is reachable 
	 * The position is stored in <hideGoal>.           
	 */	
	bool					LookForCover(aasGoal_t& hideGoal, idEntity *entity, const idVec3 &pos );
	bool					MoveToCover( idEntity *entity, const idVec3 &pos );
	bool					SlideToPosition( const idVec3 &pos, float time );
	bool					WanderAround( void );
	/**
	* Ish : Move AI along a vector without worrying about AAS or obstacles
	* Can be used for direct control of an AI
	* Applies finite turn speed toward the direction
	**/
	bool					MoveAlongVector( float yaw );
	bool					StepDirection( float dir );
	bool					NewWanderDir( const idVec3 &dest );

	// greebo: These two Push/Pop commands can be used to save the current move state in a stack and restore it later on
	void					PushMove();
	void					PopMove();

	void					PopMoveNoRestoreMove(); // grayman #3647

	// Local helper function, will restore the current movestate from the given saved one
	void					RestoreMove(const idMoveState& saved);

	// effects
	const idDeclParticle	*SpawnParticlesOnJoint( particleEmitter_t &pe, const char *particleName, const char *jointName );
	void					SpawnParticles( const char *keyName );
	bool					ParticlesActive( void );

	// turning
	bool					FacingIdeal( void );
	void					Turn(const idVec3& pivotOffset = idVec3(0,0,0));
	bool					TurnToward( float yaw );
	bool					TurnToward( const idVec3 &pos );
	ID_INLINE float			GetCurrentYaw() { return current_yaw; }
	ID_INLINE float			GetIdealYaw() { return ideal_yaw; } // grayman #2345
	ID_INLINE float			GetTurnRate() { return turnRate; }
	ID_INLINE void			SetTurnRate(float newTurnRate) { turnRate = newTurnRate; }

	// enemy management
	void					ClearEnemy( void );
	bool					EnemyPositionValid( void ) const;

	/**
	 * greebo: SetEnemyPos() tries to determine the enemy location and to setup a path
	 *         to the enemy, depending on the AI's move type (FLY/WALK).
	 *
	 * If this method succeeds in setting up a path to the enemy, the following members are
	 * set: lastVisibleReachableEnemyPos, lastVisibleReachableEnemyAreaNum
	 * and the movecommands move.toAreaNum and move.dest are updated, but the latter two 
	 * ONLY if the movecommand is set to MOVE_TO_ENEMY beforehand.
	 *
	 * The AI_DEST_UNREACHABLE is updated if the movecommand is currently set to MOVE_TO_ENEMY. 
	 *
	 * It is TRUE (enemy unreachable) in the following cases:
	 * - Enemy is not on ground (OnLadder) for non-flying AI.
	 * - The entity area number could not be determined.
	 * - PathToGoal failed, no path to the enemy could be found.
	 * 
	 * Note: This overwrites a few lastVisibleReachableEnemyPos IN ANY CASE with the 
	 * lastReachableEnemyPos, so this is kind of cheating if the enemy is not visible before calling this.
	 * Also, lastVisibleEnemyPos is overwritten by the enemy's origin IN ANY CASE.
	 *
	 * Basically, this method relies on "lastReachableEnemyPos" being set beforehand.
	 */
	void					SetEnemyPosition();

	/**
	 * greebo: This is pretty similar to SetEnemyPosition, but not the same.
	 *
	 * First, this tries to locate the current enemy position (disregards visibility!) and
	 * to set up a path to the entity's origin/floorposition. If this succeeds,
	 * the "lastReachableEnemyPos" member is updated, but ONLY if the enemy is on ground.
	 *
	 * Second, if the enemy is visible or heard, SetEnemyPosition is called, which updates
	 * the lastVisibleReachableEnemyPosition.
	 */
	void					UpdateEnemyPosition();

	/**
	 * greebo: Updates the enemy pointer and tries to set up a path to the enemy by
	 *         using SetEnemyPosition(), but ONLY if the enemy has actually changed.
	 *
	 * AI_ENEMY_DEAD is updated at any rate.
	 *
	 * @returns: TRUE if the enemy has been set and is non-NULL, FALSE if the enemy
	 *           is dead or has been cleared by anything else.
	 */
	bool					SetEnemy(idActor *newEnemy);

	/**
	* DarkMod: Ishtvan note:
	* Before I added this, this code was only called in
	* Event_FindEnemy, so it could be used by scripting, but
	* not by the SDK.  I just moved the code to a new function,
	* and made Event_FindEnemy call this.
	*
	* This was because I needed to use FindEnemy in the visibility
	* calculation.
	**/
	idActor * FindEnemy( bool useFOV ) ;

	idActor* FindEnemyAI(bool useFOV);

	idActor* FindFriendlyAI(int requiredTeam);


	/**
	* Similarly to FindEnemy, this was previously only an Event_ scripting
	* function.  I moved it over to a new SDK function and had the Event_ 
	* call it, in case we want to use this later.  It returns the closest
	* AI or Player enemy.
	*
	* It was originally used to get tactile alerts, but is no longer used for that
	* IMO we should leave it in though, as we might use it for something later,
	* like determining what targets to engage with ranged weapons.
	**/	
	idActor * FindNearestEnemy( bool useFOV = true );

	/**
	 * angua: this returns if the AI has seen evidence of an intruder already 
	 * (the enemy, a body, missing loot...)
	 */
	bool HasSeenEvidence() const;

	/**
	 * grayman #3857 - register that evidence has been seen that
	 * causes the AI to use Alert Idle instead of Idle
	 */
	void HasEvidence( EventType type );

	/**
	* Draw the debug cone representing valid knockout area
	* Called every frame when cvar cv_ai_ko_show is set to true.
	**/
	void KnockoutDebugDraw( void );

	/**
	* Draw the debug cone representing the FOV
	* Called every frame when cvar cv_ai_fov_show is set to true.
	**/
	void FOVDebugDraw( void );

	/**
	* This method calculates the maximum distance from a given
	* line segment that the segment is visible due to current light conditions
	* at the segment. Does not accept NULL pointers!
	*/
	float GetMaximumObservationDistanceForPoints(const idVec3& p1, const idVec3& p2) const;

	// Returns the maximum distance where this AI can still see the given entity from
	float GetMaximumObservationDistance(idEntity* entity) const;

	/**
	* The point of this function is to determine the visual stimulus level caused
	* by addition of the CVAR tdm_ai_sight_mag. The current alert level is taken 
	* as reference value and the difference in logarithmic alert units is returned.
	*/
//	float GetPlayerVisualStimulusAmount() const;

	// attacks

	// greebo: Ensures that the projectile info of the currently active projectile 
	// is holding a valid clipmodel and returns the info structure for convenience.
	ProjectileInfo&			EnsureActiveProjectileInfo();

	// Called at spawn time to ensure valid projectile properties in the info structures (except clipmodel)
	// activeProjectile.projEnt will be NULL after this call
	void					InitProjectileInfo();

	// Specialised routine to initialise the given projectile info structure from the named or given dictionary
	// Won't touch the activeProjectile structure, so it's safe to use that as helper.
	void					InitProjectileInfoFromDict(ProjectileInfo& info, const char* entityDef) const;
	void					InitProjectileInfoFromDict(ProjectileInfo& info, const idDict* dict) const;

	/**
	 * greebo: Changed this method to support multiple projectile definitions (D3 suppoerted exactly one).
	 * The index argument can be set to anything within [0..projectileInfo.Num()) to spawn a specific
	 * projectile from the list of possible ones, or leave that argument at the default of -1 to
	 * create a projectile using the "current projectile index". That index is re-rolled each
	 * time after this method has been called, so the AI will seemingly use random projectiles.
	 *
	 * Note: doesn't do anything if the currently active projectile is not NULL.
	 */
	idProjectile*			CreateProjectile(const idVec3 &pos, const idVec3 &dir, int index = -1);

private:
	/**
	 * greebo: Specialised method similar to the CreateProjectile() variant taking an index.
	 * This method doesn't use the projectileInfo list, but spawns a new active projectile
	 * directly from the given dictionary. This can be used to spawn projectiles out of the usual
	 * loop of known projectile defs.
	 *
	 * Note: doesn't do anything if the currently active projectile is not NULL.
	 */
	idProjectile*			CreateProjectileFromDict(const idVec3 &pos, const idVec3 &dir, const idDict* dict);

	// Helper routine to spawn an actual projectile with safety checks. Will throw gameLocal.Error on failure.
	// The result will always be non-NULL
	idProjectile*			SpawnProjectile(const idDict* dict) const;

	void					RemoveProjectile( void );

	
	virtual void			SwapLODModel( const char *modelname ) override; // SteveL #3770

public:
	idProjectile*			LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone );
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) override;
	void					DirectDamage( const char *meleeDefName, idEntity *ent );
	bool					TestMelee( void ) const;
	/** ishtvan: test melee anticipating future positions **/
	bool					TestMeleeFuture( void ) const;
	bool					TestRanged( void );
	bool					AttackMelee( const char *meleeDefName );
	void					BeginAttack( const char *name );
	void					EndAttack( void );
	void					PushWithAF( void );

	// special effects
	void					GetMuzzle( const char *jointname, idVec3 &muzzle, idMat3 &axis );
	void					InitMuzzleFlash( void );
	void					TriggerWeaponEffects( const idVec3 &muzzle );
	void					UpdateMuzzleFlash( void );
	virtual bool			UpdateAnimationControllers( void ) override;
	void					UpdateParticles( void );
	void					TriggerParticles( const char *jointName );

	// AI script state management
	virtual void			LinkScriptVariables( void ) override;
	virtual void			UpdateScript() override;

	// Returns true if the current enemy can be reached
	bool					CanReachEnemy();

	// Returns the current move status (MOVE_STATUS_MOVING, for instance).
	moveStatus_t			GetMoveStatus() const;

	/**
	* Returns true if AI's mouth is underwater
	**/
	bool MouthIsUnderwater( void );

	/**
	* Checks for drowning, damages if drowning.
	*
	* greebo: Only enabled if the entity is able to drown and the 
	* interleave timer is elapsed (is not checked each frame).
	**/
	void					UpdateAir();

	/**
	* Halts lipsync
	**/
	void					StopLipSync();

	/**
	 * Plays and lipsyncs the given sound name, returns the duration in msecs.
	 */
	int						PlayAndLipSync(const char *soundName, const char *animName, int msgTag); // grayman #3355

	// Lip sync stuff
	bool					m_lipSyncActive; /// True iff we're currently lip syncing
	int						m_lipSyncAnim; /// The number of the animation that we are lipsyncing to
	int						m_lipSyncEndTimer; /// Time at which to stop lip syncing

	// grayman #3857 - bark stuff
	idStr					m_barkName; // The name of the bark
	int						m_barkEndTime; // When the bark will end

	bool					DrawWeapon(ECombatType type); // grayman #3331 // grayman #3775
	void					SheathWeapon();

	// angua: this is used to check whether the AI is able to unlock a specific door
	bool					CanUnlock(CBinaryFrobMover *frobMover);

	// angua: this checks whether the AI should close the door after passing through
	bool					ShouldCloseDoor(CFrobDoor *frobMover, bool wasLocked); // grayman #3523

	// greebo: Contains all the checks for CVAR-dependent debug info
	void					ShowDebugInfo();

	const idStr&			GetNextIdleAnim() const;
	void					SetNextIdleAnim(const idStr& nextIdleAnim);

	// grayman #2603 - work with the list of delayed visual stims

	void					SetDelayedStimExpiration(idEntityPtr<idEntity> stimPtr);
	int						GetDelayedStimExpiration(idEntityPtr<idEntity> stimPtr);

	// grayman #3075 - set and get an AI's blood marker
	void					SetBlood(idEntity *marker);
	idEntity*				GetBlood(void) const;

	// grayman #2816 - get last enemy killed
	void					SetLastKilled(idActor *killed);
		
	// grayman #3643 - setup a suspicious door
	void					SetUpSuspiciousDoor(CFrobDoor* door);

	// grayman #4238 - is an AI standing at a point?
	bool					PointObstructed(idVec3 p);

	// for debugging circling problems
	//void					PrintGoalData(idVec3 goal, int tag);

	//
	// ai/ai_events.cpp
	//
	// The post-spawn event parses the spawnargs which refer to other entities
	// that might not be available at spawn time.
	void					Event_PostSpawn();
	void					Event_Activate( idEntity *activator );


/*****
* DarkMod: Event_Touch was modified to issue a tactile alert.
*
* Note: Event_Touch checks ReactionTo, which checks our DarkMod Relations.
* So it will only go off if the AI is bumped by an enemy that moves into it.
* This is NOT called when an AI moves into an enemy.
*
* AI bumping by inanimate objects is handled separately by CheckTactile.
****/
	void					Event_Touch( idEntity *other, trace_t *trace );

	void					Event_FindEnemy( int useFOV );
	void					Event_FindEnemyAI( int useFOV );
	void					Event_FindEnemyInCombatNodes( void );

	/**
	 * greebo: Finds the nearest friendly and visible AI. Used to look for allies.
	 *         The <team> argument is optional and can be used to limit the search to a given team.
	 *         Set <team> to -1 to disable the team search.
	 *
	 *         Returns the best candidate (can be the nullentity) to the script thread.
	 */
	void					Event_FindFriendlyAI(int requiredTeam);

	void					Event_ClosestReachableEnemyOfEntity( idEntity *team_mate );
	//void					Event_HeardSound( int ignore_team );
	void					Event_SetEnemy( idEntity *ent );
	void					Event_ClearEnemy( void );
	void					Event_FoundBody( idEntity *body );
	void					Event_MuzzleFlash( const char *jointname );
	void					Event_CreateMissile( const char *jointname );
	void					Event_CreateMissileFromDef(const char* defName, const char* jointname);
	void					Event_AttackMissile( const char *jointname );
	void					Event_FireMissileAtTarget( const char *jointname, const char *targetname );
	void					Event_LaunchMissile( const idVec3 &muzzle, const idAngles &ang );
	void					Event_AttackMelee( const char *meleeDefName );
	void					Event_DirectDamage( idEntity *damageTarget, const char *damageDefName );
	void					Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName );
	void					Event_BeginAttack( const char *name );
	void					Event_EndAttack( void );
	void					Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName );
	void					Event_RandomPath( void );
	void					Event_CanBecomeSolid( void );
	void					Event_BecomeSolid( void );
	void					Event_BecomeNonSolid( void );
	void					Event_BecomeRagdoll( void );
	void					Event_StopRagdoll( void );
	void					Event_AllowDamage( void );
	void					Event_IgnoreDamage( void );
	void					Event_GetCurrentYaw( void );
	void					Event_TurnTo( float angle );
	void					Event_TurnToPos( const idVec3 &pos );
	void					Event_TurnToEntity( idEntity *ent );
	void					Event_MoveStatus( void );
	void					Event_StopMove( void );
	void					Event_MoveToCover( void );
	void					Event_MoveToCoverFrom( idEntity *enemyEnt );
	void					Event_MoveToEnemy( void );
	void					Event_MoveToEnemyHeight( void );
	void					Event_MoveOutOfRange( idEntity *entity, float range );
	void					Event_Flee(idEntity* entityToFleeFrom);
	void					Event_FleeFromPoint( const idVec3 &pos );
	void					Event_MoveToAttackPosition( idEntity *entity, const char *attack_anim );
	void					Event_MoveToEntity( idEntity *ent );
	void					Event_MoveToPosition( const idVec3 &pos );
	void					Event_SlideTo( const idVec3 &pos, float time );
	void					Event_Wander( void );
	void					Event_FacingIdeal( void );
	void					Event_FaceEnemy( void );
	void					Event_FaceEntity( idEntity *ent );
	void					Event_WaitAction( const char *waitForState );
	void					Event_GetCombatNode( void );
	void					Event_EnemyInCombatCone( idEntity *ent, int use_current_enemy_location );
	void					Event_WaitMove( void );
	void					Event_GetJumpVelocity( const idVec3 &pos, float speed, float max_height );
	void					Event_EntityInAttackCone( idEntity *ent );
	void					Event_CanSeeEntity( idEntity *ent );
	void					Event_CanSeeEntityExt( idEntity *ent, const int useFOV, const int useLighting);
	void					Event_IsEntityHidden( idEntity *ent, const float sightThreshold);
	void					Event_CanSeePositionExt( const idVec3& position, int useFOV, int useLighting);
	void					Event_SetTalkTarget( idEntity *target );
	void					Event_GetTalkTarget( void );
	void					Event_SetTalkState( int state );
	void					Event_EnemyRange( void );
	void					Event_EnemyRange2D( void );
	void					Event_GetEnemy( void );
	void					Event_GetEnemyPos( void );
	void					Event_GetEnemyEyePos( void );
	void					Event_PredictEnemyPos( float time );
	void					Event_CanHitEnemy( void );
	void					Event_CanHitEnemyFromAnim( const char *animname );
	void					Event_CanHitEnemyFromJoint( const char *jointname );
	void					Event_EnemyPositionValid( void );
	void					Event_ChargeAttack( const char *damageDef );
	void					Event_TestChargeAttack( void );
	void					Event_TestAnimMoveTowardEnemy( const char *animname );
	void					Event_TestAnimMove( const char *animname );
	void					Event_TestMoveToPosition( const idVec3 &position );
	void					Event_TestMeleeAttack( void );
	void					Event_TestAnimAttack( const char *animname );
	void					Event_Shrivel( float shirvel_time );
	void					Event_Burn( void );
	void					Event_PreBurn( void );
	void					Event_ClearBurn( void );
	void					Event_SetSmokeVisibility( int num, int on );
	void					Event_NumSmokeEmitters( void );
	void					Event_StopThinking( void );
	void					Event_GetTurnDelta( void );
	void					Event_GetMoveType( void );
	void					Event_SetMoveType( int moveType );
	void					Event_SaveMove( void );
	void					Event_RestoreMove( void );
	void					Event_AllowMovement( float flag );
	void					Event_JumpFrame( void );
	void					Event_EnableClip( void );
	void					Event_DisableClip( void );
	void					Event_EnableGravity( void );
	void					Event_DisableGravity( void );
	void					Event_EnableAFPush( void );
	void					Event_DisableAFPush( void );
	void					Event_SetFlySpeed( float speed );
	void					Event_SetFlyOffset( int offset );
	void					Event_ClearFlyOffset( void );
	void					Event_GetClosestHiddenTarget( const char *type );
	void					Event_GetRandomTarget( const char *type );
	void					Event_TravelDistanceToPoint( const idVec3 &pos );
	void					Event_TravelDistanceToEntity( idEntity *ent );
	void					Event_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest );
	void					Event_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_LookAtEnemy( float duration );
	void					Event_LookAtPosition (const idVec3& lookAtWorldPosition, float duration);
	void					Event_LookAtAngles (float yawAngleClockwise, float pitchAngleUp, float rollAngle, float durationInSeconds);
	void					Event_SetJointMod( int allowJointMod );
	void					Event_ThrowMoveable( void );
	void					Event_ThrowAF( void );
	void					Event_SetAngles( idAngles const &ang );
	void					Event_GetAngles( void );
	void					Event_RealKill( void );
	void					Event_Kill( void );
	void					Event_WakeOnFlashlight( int enable );
	void					Event_LocateEnemy( void );
	void					Event_KickObstacles( idEntity *kickEnt, float force );
	void					Event_GetObstacle( void );
	void					Event_PushPointIntoAAS( const idVec3 &pos );
	void					Event_GetTurnRate( void );
	void					Event_SetTurnRate( float rate );
	void					Event_AnimTurn( float angles );
	void					Event_AllowHiddenMovement( int enable );
	void					Event_TriggerParticles( const char *jointName );
	void					Event_FindActorsInBounds( const idVec3 &mins, const idVec3 &maxs );
	void 					Event_CanReachPosition( const idVec3 &pos );
	void 					Event_CanReachEntity( idEntity *ent );
	void					Event_CanReachEnemy( void );
	void					Event_GetReachableEntityPosition( idEntity *ent );
	void					Event_ReEvaluateArea(int areanum);

	// Script interface for state manipulation
	void					Event_PushState(const char* state);
	void					Event_SwitchState(const char* state);
	void					Event_EndState();

	void					Event_PlayAndLipSync( const char *soundName, const char *animName );
	
	/**
	* Frontend scripting functions for Dark Mod Relations Manager
	* See CRelations class definition for descriptions
	**/
	void					Event_GetRelationEnt( idEntity *ent );
	
	void					Event_SetAlertLevel( float newAlertLevel );

	/**
	* Script frontend for idAI::GetAcuity and idAI::SetAcuity
	* and idAI::AlertAI
	**/
	void Event_Alert( const char *type, float amount );
	void Event_GetAcuity( const char *type );
	void Event_SetAcuity( const char *type, float val );
	void Event_GetAudThresh( void );
	void Event_SetAudThresh( float val );

	/**
	* Get the actor that alerted the AI this frame
	**/
	void Event_GetAlertActor( void );

	/**
	* Set an alert grace period
	* First argument is the fraction of the current alert this frame to ignore
	* Second argument is the number of SECONDS the grace period lasts.
	* Third argument is the number of events it takes to override the grace period
	**/
	void Event_SetAlertGracePeriod( float frac, float duration, int count );

	void Event_GetObservationPosition (const idVec3& pointToObserve, const float visualAcuityZeroToOne );

	/**
	* Gets the alert number of an entity that is another AI.
	* Will return 0.0 to script if other entity is NULL or not an AI.
	*/
	void Event_GetAlertLevelOfOtherAI (idEntity* p_otherEntity);

	void Event_ProcessBlindStim(idEntity* stimSource, int skipVisibilityCheck);
	/**
	 * greebo: Script event for processing a visual stim coming from the entity <stimSource>
	 */
	void Event_ProcessVisualStim(idEntity* stimSource);

	/*!
	* Spawns a new stone projectile that the AI can throw
	*
	* @param pstr_projectileName Name of the projectile type to
	*	be spawned (as given in .def file)
	*
	* @param pstr_jointName Name of the model joint to which the
	*	stone projectile will be bound until thrown.
	*/
	void Event_SpawnThrowableProjectile ( const char* pstr_projectileName, const char* pstr_jointName );

	/**
	* Scan for the player in FOV, and cause a visual alert if found
	* Currently only checks the player.
	* Will return the entity that caused the visual alert for 
	* scripting purposes.
	**/
	void Event_VisScan( void );
	
	/**
	* Return the last suspicious sound direction
	**/
	void Event_GetSndDir( void );

	/**
	* Return the last visual alert position
	**/
	void Event_GetVisDir( void );

	/**
	* Return the entity that the AI is in tactile contact with
	**/
	void Event_GetTactEnt( void );

	/**
	* This is needed for accurate AI-AI combat
	* It just calls the vanilla D3 event:
	* Event_ClosestReachableEnemyToEntity( idEntity *ent )
	* with the "this" pointer.
	*
	* For some reason this was left out of D3.
	**/
	void Event_ClosestReachableEnemy( void );

	// Scripts query this to retrieve the name of the next idle anim
	void Event_GetNextIdleAnim();

	void Event_HasSeenEvidence();

	// grayman #2603 - relighting lights
	void Event_PerformRelight();

	// grayman #2603 - drop torch
	void Event_DropTorch();

	// grayman #2816 - bark
	void Event_Bark(const char* soundName);

	// grayman #3154
	void Event_EmptyHand(const char* hand);

	// grayman #2920
	void Event_RestartPatrol();

	// grayman #5056
	void Event_StopPatrol();

	void Event_OnDeadPersonEncounter(idActor* person); // grayman #3317
	void Event_OnUnconsciousPersonEncounter(idActor* person); // grayman #3317

	void Event_AllowGreetings(); // grayman #3338

	void Event_DelayedVisualStim(idEntity* stimSource); // grayman #2924

	void Event_AlertAI(const char *type, float amount, idActor* actor); // grayman #3356 & #3258

	void Event_GetAttacker();	// grayman #3679
	void Event_IsPlayerResponsibleForDeath(); // grayman #3679

	void Event_NoisemakerDone(idEntity* maker); // grayman #3681

	void Event_HitByDoor(idEntity* door); // grayman #3756

	void Event_PlayCustomAnim( const char* animName ); // SteveL #3597

	void Event_GetVectorToIdealOrigin(); // grayman #3989

#ifdef TIMING_BUILD
private:
	int aiThinkTimer;
	int aiMindTimer;
	int aiAnimationTimer;
	int aiPushWithAFTimer;
	int aiUpdateEnemyPositionTimer;
	int aiScriptTimer;
	int aiAnimMoveTimer;
	int aiObstacleAvoidanceTimer;
	int aiPhysicsTimer;
	int aiGetMovePosTimer;
	int aiPathToGoalTimer;
	int aiGetFloorPosTimer;
	int aiPointReachableAreaNumTimer;
	int aiCanSeeTimer;


#endif
};

class idCombatNode : public idEntity {
public:
	CLASS_PROTOTYPE( idCombatNode );

						idCombatNode();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	bool				IsDisabled( void ) const;
	bool				EntityInView( idActor *actor, const idVec3 &pos );
	static void			DrawDebugInfo( void );

private:
	float				min_dist;
	float				max_dist;
	float				cone_dist;
	float				min_height;
	float				max_height;
	idVec3				cone_left;
	idVec3				cone_right;
	idVec3				offset;
	bool				disabled;

	void				Event_Activate( idEntity *activator );
	void				Event_MarkUsed( void );
};

#endif /* !__AI_H__ */
