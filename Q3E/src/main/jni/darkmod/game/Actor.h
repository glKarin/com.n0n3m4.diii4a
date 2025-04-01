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
#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

#include "MultiStateMoverPosition.h"
#include "ai/EAS/RouteInfo.h"
#include <set>

// greebo: Keep the values in scripts/tdm_defs.script in sync with these
enum ECombatType
{
	COMBAT_NONE,
	COMBAT_MELEE, 
	COMBAT_RANGED,
	NUM_COMBAT_TYPES
};

/** Melee Attack/Parry types **/
typedef enum
{
	MELEETYPE_OVER,
	MELEETYPE_LR, // slashing (attacker's) left to right
	MELEETYPE_RL, // slashing (attacker's) right to left
	MELEETYPE_THRUST,
	MELEETYPE_UNBLOCKABLE, // unblockable attacks (e.g. animal claws)
	MELEETYPE_BLOCKALL, // blocks all attacks except unblockable (e.g., shield)
	NUM_MELEE_TYPES
} EMeleeType;

/** Melee Overall Action States **/
typedef enum
{
	MELEEACTION_READY, // ready to attack or parry
	MELEEACTION_ATTACK,
	MELEEACTION_PARRY
} EMeleeActState;

/** Melee phases of each action (applies to both attack and parry) **/
typedef enum
{
	MELEEPHASE_PREPARING, // Moving to backswing or parry position
	MELEEPHASE_HOLDING, // holding a backswing or parry
	MELEEPHASE_EXECUTING, // executing a threatening attack (doesn't apply to parries)
	MELEEPHASE_RECOVERING // recovering the current attack/parry back to guard position
} EMeleeActPhase;

/** Possible outcomes of a melee action (includes attacks and parries) **/
enum EMeleeResult
{
	MELEERESULT_IN_PROGRESS, // no result yet, still in progress
	MELEERESULT_AT_HIT,
	MELEERESULT_AT_MISSED,
	MELEERESULT_AT_PARRIED,
	MELEERESULT_PAR_BLOCKED, // successfully parried the attack
	MELEERESULT_PAR_FAILED, // got hit while parrying (TODO: Does not catch all cases, only catches parry type mismatch)
	MELEERESULT_PAR_ABORTED, // gave up parrying

	NUM_MELEE_RESULTS
};

/** class for storing current melee combat status **/
class CMeleeStatus
{
public:	
	CMeleeStatus( void );
	virtual ~CMeleeStatus( void );
	
	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	// clears the current action
	void ClearAction( void );

	EMeleeActState	m_ActionState; // current action
	EMeleeActPhase	m_ActionPhase; // phase within that action
	EMeleeType		m_ActionType; // type of attack/parry

	// time the phase of our action last changed
	int				m_PhaseChangeTime;
	// time that our most recent action ended
	int				m_LastActTime;

	// Result of most recent action
	// Will be considered "in progress" until back in the "Ready" state
	EMeleeResult	m_ActionResult;

	// were we just hit by a melee attack?  may take longer to perform next action
	// NYI
	bool			m_bWasHit;

	/** What attack type were we last hit by? (used by trainers to give feedback) **/
	EMeleeType		m_LastHitByType;

	// melee capabilities of weapon
	bool				m_bCanParry;
	bool				m_bCanParryAll;
	idList<EMeleeType>	m_attacks; // possible attacks with current weapon

}; // CMeleeStatus


/*
===============================================================================

	idActor

===============================================================================
*/

// grayman #2816 - constants for moveables striking AI
const float	MIN_MASS_FOR_KO = 50.0f;

extern const idEventDef AI_EnableEyeFocus;
extern const idEventDef AI_DisableEyeFocus;
extern const idEventDef EV_Footstep;
extern const idEventDef EV_FootstepLeft;
extern const idEventDef EV_FootstepRight;
extern const idEventDef EV_EnableWalkIK;
extern const idEventDef EV_DisableWalkIK;
extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetAnimPrefix;
extern const idEventDef AI_PlayAnim;
extern const idEventDef AI_PauseAnim;
extern const idEventDef AI_AnimIsPaused;
extern const idEventDef AI_PlayCycle;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;

// Obsttorte #0540
extern const idEventDef EV_getAnimRate;
extern const idEventDef EV_setAnimRate;
extern const idEventDef EV_getAnimList;

extern const idEventDef AI_MeleeAttackStarted;
extern const idEventDef AI_MeleeParryStarted;
extern const idEventDef AI_MeleeActionHeld;
extern const idEventDef AI_MeleeActionReleased;
extern const idEventDef AI_MeleeActionFinished;
extern const idEventDef AI_GetMeleeActionState;
extern const idEventDef AI_GetMeleeActionPhase;
extern const idEventDef AI_GetMeleeActionType;
extern const idEventDef AI_GetMeleeLastActTime;
extern const idEventDef AI_GetMeleeResult;
extern const idEventDef AI_GetMeleeLastHitByType;
extern const idEventDef AI_MeleeBestParry;
extern const idEventDef AI_MeleeNameForNum;

extern const idEventDef AI_ShowAttachment;
extern const idEventDef AI_ShowAttachmentInd;

extern const idEventDef AI_SyncAnimChannels; // SteveL #3800
extern const idEventDef AI_OverrideAnim; // SteveL #4012: Expose so IdleAnimationTask can use it. 

class idDeclParticle;

class idAnimState {
public:
	bool					idleAnim;
	idStr					state;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
							idAnimState();
							~idAnimState();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Init( idActor *owner, idAnimator *_animator, int animchannel );
	void					Shutdown( void );
	void					SetState( const char *name, int blendFrames );
	void					SetFrame( int anim, int frame );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					PauseAnim( int channel, bool bPause );
	bool					AnimIsPaused( int channel );
	void					CycleAnim( int anim );
	void					BecomeIdle( void );
	bool					UpdateState( void );
	bool					Disabled( void ) const;
	void					Enable( int blendFrames );
	void					Disable( void );
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle( void ) const;
	animFlags_t				GetAnimFlags( void ) const;
	idAnimator*				GetAnimator();
	void					FinishAction(const idStr& actionname);
	const char *			WaitState( void ) const;
	void					SetWaitState( const char *_waitstate );


private:
	idStr					waitState;
	idActor *				self;
	idAnimator *			animator;
	idThread *				thread;
	int						channel;
	bool					disabled;
};

typedef struct {
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

struct CrashLandResult
{
	int damageDealt;	// contains the damage done to the actor
	bool hasLanded;		// true if the actor hit the ground after a fall this frame
};

enum GreetingState
{
	ECannotGreet = 0,		// actor is not able to greet (e.g. spiders)
	ECannotGreetYet,		// grayman #3338 - can greet, but not yet allowed to
	ENotGreetingAnybody,	// actor is currently not greeting anybody (free)
	EWaitingForGreeting,	// actor is receiving a greeting from somebody else
	EGoingToGreet,			// actor is about to greet somebody 
	EIsGreeting,			// actor is currently playing its greeting sound
	EAfterGreeting,			// actor is in the small pause after greeting
	ENumAIGreetingStates,	// invalid state
};

// Hold information about a warning you have given or received
struct WarningEvent
{
	int eventID;					// id of the suspicious event warned about
	idEntityPtr<idEntity> entity;	// giver if you received this; receiver if you sent it
};

// grayman #3857 - Hold information about a suspicious event and whether you searched it
struct KnownSuspiciousEvent
{
	int eventID;	// id of the suspicious event
	bool searched;	// whether you have searched it
};

#define TDM_HEAD_ENTITYDEF "atdm:ai_head_base"

class idActor : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idActor );

	int						rank; // monsters don't fight back if the attacker's rank is higher
	/**
	* TDM: Defines the type of the AI (human, beast, undead, bot, etc)
	**/
	int						m_AItype;
	/**
	* TDM: Whether this actor is considered a non-combatant
	**/
	bool					m_Innocent;
	idMat3					viewAxis;			// view axis of the actor

	idLinkList<idActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	idLinkList<idActor>		enemyList;			// list of characters that have targeted the player as their enemy

	// The greeting state this actor is currently in (to coordinate greeting barks in between actors)
	GreetingState			greetingState;

	/**
	* TDM: Moved up from idAI
	* These ranges are the best guess for the max range at which the AI can hit
	* an enemy.  melee_range_unarmed is the range with no weapon, read from
	* the melee_range spawnarg on the AI.
	* If the AI has no weapon, melee_range = melee_range_unarmed.
	* melee_range includes the reach of the current weapon, updated when weapons are attached
	**/
	float					melee_range_unarmed; // potential
	float					melee_range; // includes reach of current weapon
	/**
	* grayman #2655 - some AI need a different vertical melee_range, i.e. the werebeast crawler
	**/
	float					melee_range_vert;
	/**
	* This should roughly correspond to the time it takes them to swing the weapon
	* In seconds (read from spawnarg in milliseconds and converted)
	* Currently it is used for AI predicting whether they will be able to hit
	* their enemy in the future if they start our attack now
	* It may be used for other things later
	**/	
	float					m_MeleePredictedAttTime;
	// and without a weapon (need to store in case they get disarmed somehow)
	float					m_MeleePredictedAttTimeUnarmed;


	/**
	* Stores what this actor is currently doing in melee combat
	* ishtvan: Made public to avoid lots of gets/sets
	**/
	CMeleeStatus			m_MeleeStatus;
	/** 
	* Number representing the ability of this actor to inflict melee damage
	* multiplies the baseline melee damage set on the weapon by this amount 
	**/
	float					m_MeleeDamageMult;
	/**
	* Melee timing: Time between the backswing and forward swing
	* Actual number is random, between this min and max
	* Number is in milliseconds
	**/
	int						m_MeleeHoldTimeMin;
	int						m_MeleeHoldTimeMax;
	int						m_MeleeCurrentHoldTime;
	/**
	* Melee timing: Max time that we will hold a parry, waiting for the attack
	**/
	int						m_MeleeParryHoldMin;
	int						m_MeleeParryHoldMax;
	int						m_MeleeCurrentParryHold;
	/**
	* Melee timing: Time between subsequent attacks (in milliseconds)
	**/
	int						m_MeleeAttackRecoveryMin;
	int						m_MeleeAttackRecoveryMax;
	int						m_MeleeCurrentAttackRecovery;
	/**
	* Melee timing: Time after being parried or getting hit that we can attack (longer than the normal time)
	**/
	int						m_MeleeAttackLongRecoveryMin;
	int						m_MeleeAttackLongRecoveryMax;
	int						m_MeleeCurrentAttackLongRecovery;
	/**
	* Melee timing: Time after last action that we can parry (fairly short)
	* TODO: the name of this could be confusing, we're not recovering FROM a parry
	**/
	int						m_MeleeParryRecoveryMin;
	int						m_MeleeParryRecoveryMax;
	int						m_MeleeCurrentParryRecovery;
	/**
	* Melee timing: Time after a successful (or aborted) parry that we can attack
	**/
	int						m_MeleeRiposteRecoveryMin;
	int						m_MeleeRiposteRecoveryMax;
	int						m_MeleeCurrentRiposteRecovery;
	/**
	* Melee timing: Time delay between when we decide to parry and when the anim starts
	* Acts as a human reaction time to an oncoming attack
	**/
	int						m_MeleePreParryDelayMin;
	int						m_MeleePreParryDelayMax;
	int						m_MeleeCurrentPreParryDelay;
	/**
	* Same as above, but use a shorter delay when responding to repeated attacks
	* along the same direction
	**/
	int						m_MeleeRepeatedPreParryDelayMin;
	int						m_MeleeRepeatedPreParryDelayMax;
	int						m_MeleeCurrentRepeatedPreParryDelay;
	/**
	* Controls how many times the attack has to be repeated in a row
	* in order to use the faster response time
	**/
	int						m_MeleeNumRepAttacks;
	/**
	* Time duration over which the same attacks in a row have to occur
	* in order to be anticipated and use faster response
	**/
	int						m_MeleeRepAttackTime;
	/**
	* Melee timing: Time delay between when we decide to stop parrying and when the animation goes
	* Acts as a human reaction time to the cessation of an attack
	**/
	int						m_MeleePostParryDelayMin;
	int						m_MeleePostParryDelayMax;
	int						m_MeleeCurrentPostParryDelay;
	/**
	* Same as above, but use a shorter delay when responding to repeated attacks
	* along the same direction
	**/
	int						m_MeleeRepeatedPostParryDelayMin;
	int						m_MeleeRepeatedPostParryDelayMax;
	int						m_MeleeCurrentRepeatedPostParryDelay;

	/**
	* Correspondence between melee type and string name suffix of the action
	**/
	static const char *		MeleeTypeNames[ NUM_MELEE_TYPES ];

	/**
	* grayman #2345 - rank for path-finding
	**/
	int						m_pathRank;

	/**
	* grayman #2728 - next time this actor can be kicked by another actor
	**/
	int						m_nextKickTime;

	/**
	* grayman #3317 - when we were killed or KO'ed
	**/
	int						m_timeFellDown;

	/**
	* grayman #3202 - whether actor is mute or not
	**/
	bool					m_isMute;

	/**
	* Offset relative to the head origin when the head is separate, or the eye 
	* when the head is attached. Used to locate the mouth.
	**/
	idVec3					m_MouthOffset; // grayman #1104

	/**
	* Offset relative to the head origin when the head is separate, or the body origin 
	* when the head is attached.
	**/
	idVec3					m_EyeOffset; // grayman #3525

	/**
	* grayman #3848 - true when a combat victor has knealt by my body
	**/
	bool					m_victorHasKnealt;

	/**
	* grayman #3848 - who killed us
	**/
	idEntityPtr<idEntity>	m_killedBy;

	/**
	* grayman #3424 - List of warnings this actor has either given or received.
	* Use the id as in index into gameLocal.m_suspiciousEvents, which is a list.
	**/
	idList<WarningEvent>	m_warningEvents;

	/**
	* grayman #3857 - List of suspicious events this actor knows about, and
	* whether he's searched them or not.
	**/
	idList<KnownSuspiciousEvent>	m_knownSuspiciousEvents;

public:
							idActor( void );
	virtual					~idActor( void ) override;

	void					Spawn( void );
	virtual void			Restart( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;
	virtual int				GetDefaultSurfaceType( void ) const override;
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material, bool save = true ) override;

	virtual bool			LoadAF( void ) override;
	void					SetupBody( void );

	void					CheckBlink( void );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) override;

							// script state management
	void					ShutdownThreads( void );
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const override;
	virtual idThread *		ConstructScriptObject( void ) override;
	virtual void			UpdateScript();
	const function_t		*GetScriptFunction( const char *funcname );
	void					SetState( const function_t *newState );
	void					SetState( const char *statename );
	
							// vision testing
	void					SetEyeHeight( float height );
	float					EyeHeight( void ) const;
	idVec3					EyeOffset( void ) const;
	virtual idVec3			GetEyePosition( void ) const;
	virtual void			GetViewPos( idVec3 &origin, idMat3 &axis ) const;
	/**
	* Sets the actor's field of view (doesn't apply to players)
	* Input is horizontal FOV, vertical FOV.
	* If no vertical is provided, it is assumed the same as the vertical
	*
	* Sets the FOV dot products to those of the half-angles of the full fov
	**/
	void					SetFOV( float fovHoriz, float fovVert = -1 );
	virtual bool			CheckFOV( const idVec3 &pos ) const;

	/**
	 * greebo: This method returns TRUE, if the given entity <ent> is visible by this actor.
	 *         For actor entities, the eye position is taken into account, for ordinary entities,
	 *         the origin is taken.
	 *         First, the origin/eye position is checked against the FOV of this actor, and second,
	 *         a trace is performed from eye (this) to origin/eye (other entity). If the trace is
	 *         blocked, the entity is considered hidden and the method returns FALSE.
	 */
	virtual bool			CanSee( idEntity *ent, bool useFOV ) const;
	bool					PointVisible( const idVec3 &point ) const;
	virtual void			GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos );

							// damage
	void					SetupDamageGroups( void );
	int						GetDamageLocation(idStr name); // Obsttorte
	// DarkMod: Added trace reference to damage
	virtual	void			Damage
							( 
								idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName, const float damageScale, const int location,
								trace_t *collision = NULL
							) override;

	/**
	* Return the stealth damage multiplier
	* Only used by derived class idAI
	**/
	virtual float	StealthDamageMult( void ) { return 1.0; };

	/**
	* Melee callbacks so the melee system can let the actor know what happened
	* And to keep the melee status up to date
	**/
	void MeleeAttackMissed( void );
	void MeleeAttackHit( idEntity *ent );
	void MeleeAttackParried( idEntity *owner, idEntity *weapon = NULL );
	void MeleeParrySuccess( idEntity *other );

	// grayman #4882 - get which side of a door a point is on
	int  GetDoorSide(CFrobDoor* frobDoor, idVec3 pos);

	/****************************************************************************************
	=====================
	idActor::CrashLand
	handle collision (Falling) damage to AI/Players
	Added by Richard Day
	=====================
	// greebo: Changed return type: the amount of damage points is contained in the struct
	****************************************************************************************/
	CrashLandResult			CrashLand( const idPhysics_Actor& physicsObj, const idVec3 &oldOrigin, const idVec3 &oldVelocity );

	int						GetDamageForLocation( int damage, int location );
	const char *			GetDamageGroup( int location );
	void					ClearPain( void );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const idDict* damageDef ) override;

	// greebo: Sets the "player is pushing something heavy" state to the given bool.
	virtual void			SetIsPushing(bool isPushing);
	// Returns whether the player is currently pushing something heavy
	virtual bool			IsPushing();

							// model/combat model/ragdoll
	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void ) override;
	virtual void			UnlinkCombat( void ) override;
	bool					StartRagdoll( void );
	void					StopRagdoll( void );
	virtual bool			UpdateAnimationControllers( void ) override;

							// delta view angles to allow movers to rotate the view of the actor
	const idAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const idAngles &delta );

	bool					HasEnemies( void ) const;
	idActor *				ClosestEnemyToPoint( const idVec3 &pos );
	idActor *				EnemyWithMostHealth();
	/**
	* Get closest enemy who is in the process of launching a melee attack
	**/
	idActor *				ClosestAttackingEnemy( bool bUseFOV );
	/**
	* Returns the enum of the best parry given the attacks at the time
	* If no attacking enemy is found, returns default of "RL"
	* See dActor::MeleeTypeNames for the list of melee attack/parry names
	**/
	EMeleeType				GetBestParry( void );

	virtual bool			OnLadder( void ) const;
	// Returns the elevator entity if the actor is standing on an elevator
	// angua: if mustBeMoving is true, the elevator is only returned when it is moving
	virtual CMultiStateMover* OnElevator(bool mustBeMoving) const;

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;

	/**
	* Called when the given ent is about to be bound/attached to this actor.
	**/
	virtual void			BindNotify( idEntity *ent, const char *jointName ) override; // grayman #3074
	
	/**
	* Called to determine if an actor can exchange greetings with another actor.
	**/
	virtual bool			CanGreet(); // grayman #3338

	/**
	* Retrieve head, if any
	**/
	idAFAttachment*			GetHead(); // grayman #1104

	/**
	* Suspicious events
	* grayman #3424
	**/
	bool					KnowsAboutSuspiciousEvent( int eventID );
	void					AddSuspiciousEvent( int eventID );
	int						LogSuspiciousEvent( EventType type, idVec3 loc, idEntity* entity, bool forceLog ); // grayman #3857 
	void					AddWarningEvent( idActor* other, int eventID );
	bool					HasBeenWarned( idActor* other, int eventID );
	bool					HasSearchedEvent( int eventID );
	bool					HasSearchedEvent( int eventID, EventType type, idVec3 location ); // grayman #3857 - TODO: apply where applicable
	void					MarkEventAsSearched( int eventID );

	/**
	* Called when the given ent is about to be unbound/detached from this actor.
	**/
	virtual void			UnbindNotify( idEntity *ent ) override;

	/**
	* Attach an entity.  Entity spawnArgs checked for attachments are:
	* "origin", "angles", and "joint".
	* AttName is the optional name of the attachment for indexing purposes (e.g., "melee_weapon")
	* Ent is the entity being attached
	* PosName is the optional position name to attach to.
	**/
	virtual void			Attach( idEntity *ent, const char *PosName = NULL, const char *AttName = NULL ) override;

	/**
	 * greebo: Returns the number of attached melee weapons. These are the entities
	 *         with the according spawnarg "is_weapon_melee" etc. set to "1".
	 */
	int GetNumMeleeWeapons();
	int GetNumRangedWeapons();

	ID_INLINE float GetMeleeRange() const
	{
		return melee_range;
	}

	/**
	 * greebo: Returns TRUE whether combat is allowed for the given type.
	 * Even though an actor has a weapon attached, it might still be sheathed and 
	 * combat is "disabled" therefore, which is represented by the AttackFlag.
	 */
	bool GetAttackFlag(ECombatType type) const;
	void SetAttackFlag(ECombatType type, bool enabled);

	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) override;

	virtual	renderView_t *	GetRenderView() override;
	
							// animation state control
	int						GetAnim( int channel, const char *name );
	idAnimator*				GetAnimatorForChannel(int channel);

	// greebo: Searches the given dictionary for animation replacement spawnargs and applies them to this actor
	void					LoadReplacementAnims(const idDict& spawnArgs, const char *jointName); // grayman #3074

	const char*				LookupReplacementAnim( const char *name );

	// greebo: Replaces the given animToReplace by the animation replacementAnim
	void					SetReplacementAnim(const idStr& animToReplace, const idStr& replacementAnim);

	// greebo: Removes any replacement for the given anim.
	void					RemoveReplacementAnim(const idStr& replacedAnim);
	void					StopAnim(int channel, int frames);
	void					UpdateAnimState( void );
	void					SetAnimState( int channel, const char *name, int blendFrames );
	const char *			GetAnimState( int channel ) const;
	bool					InAnimState( int channel, const char *name ) const;
	const char *			WaitState( void ) const;
	const char *			WaitState( int channel ) const;
	void					SetWaitState( const char *_waitstate );
	void					SetWaitState( int channel, const char *_waitstate );
	bool					AnimDone( int channel, int blendFrames ) const;
	virtual void			SpawnGibs( const idVec3 &dir, const char *damageDefName ) override;

	/**
	* Returns the modification to movement volume based on the movement type
	* (crouch walk, creep, run, etc)
	* Called in derived classes idPlayer and idAI.
	**/
	virtual float			GetMovementVolMod( void ) { return 0; };

	virtual bool			IsKnockedOut( void ) { return false; };

	virtual bool			CanUseElevators() const { return canUseElevators; }

	/** 
	 * greebo: This gets called by the pathing routine to let the actor
	 * reconsider the "forbidden" status of the given area. After some time
	 * AI should be able to re-try opening locked doors otherwise the door's AAS area
	 * stays flagged as "forbidden" for all times.
	 *
	 * Note: This method is overriden by the AI class.
	 *
	 * @returns: TRUE if the area is now considered as "allowed", FALSE otherwise.
	 */
	virtual bool			ReEvaluateArea(int areaNum);

public:
	// Moved from player/AI to here
	idScriptBool			AI_DEAD;

protected:

	/*	CrashLand variables	Added by Richard Day	*/

	float m_damage_thresh_min;		// min damage. anything at or below this does 0 damage
	float m_damage_thresh_hard;		// any damage above this is considered "hard"
	float m_delta_scale; ///< scale the damage based on this. delta is divide by this
	idVec3 m_savedVelocity; // grayman #3699
	

	friend class			idAnimState;

	float					m_fovDotHoriz;		// cos( Horizontal fov [degrees] )
	float					m_fovDotVert;		// cos( Vertical fov [degrees] )
	idVec3					eyeOffset;			// offset of eye relative to physics origin
	idVec3					modelOffset;		// offset of visual model relative to the physics origin
	idVec3					mHeadModelOffset;	// offset for the head

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound
	int						pain_threshold;		// how much damage monster can take at any one time before playing pain animation

	idStrList				damageGroups;		// body damage groups
	idList<float>			damageScale;		// damage scale per damage group

	// greebo: If not -1, the actor will fire a script when its health falls below this value
	int						lowHealthThreshold;
	idStr					lowHealthScript;

	/**
	* Alertnum threshold above which sneak attacks won't work,
	**/
	float					m_SneakAttackThresh;
	/**
	* Damage multiplier applied for sneak attack damage
	**/
	float					m_SneakAttackMult;

	bool						use_combat_bbox;	// whether to use the bounding box for combat collision
	idEntityPtr<idAFAttachment>	head;
	idList<copyJoints_t>		copyJoints;			// copied from the body animation to the head model

	// state variables
	const function_t		*state;
	const function_t		*idealState;
	
protected:

	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;

	idIK_Walk				walkIK;

	idStr					animPrefix;
	idStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	// script variables
	idThread *				scriptThread;
	idStr					waitState;
	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

	bool					allowPain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;

	// greebo: Is set to TRUE if this actor can use elevators.
	bool					canUseElevators;

//	idList<CAttachInfo>	m_Attachments;
	
	// Maps animation names to the names of their replacements
	idDict					m_replacementAnims;

	/**
	 * This is the set of attack flags. If the corresponding enum value ECombatType
	 * is present in this set, the actor is ready for attacking with that attack type.
	 */
	std::set<int>			m_AttackFlags;

	/**
	* Movement volume modifiers.  Ones for the player are taken from 
	* cvars (for now), ones for AI are taken from spawnargs.
	* Walking and not crouching is the default volume.
	**/

	float					m_stepvol_walk;
	float					m_stepvol_run;
	float					m_stepvol_creep;

	float					m_stepvol_crouch_walk;
	float					m_stepvol_crouch_creep;
	float					m_stepvol_crouch_run;

	virtual void			Gib( const idVec3 &dir, const char *damageDefName ) override;

							// removes attachments with "remove" set for when character dies
	void					RemoveAttachments( void );

							// copies animation from body to head joints
	void					CopyJointsFromBodyToHead( void );

	/**
	* Updates the volume offsets for various movement modes
	* (eg walk, run, creep + crouch ).  
	* Used by derived classes idPlayer and idAI.
	**/
	virtual void			UpdateMoveVolumes( void ) {};

	/**
	* TestKnockoutBlow, only defined in derived classes
	* Returns true if going from conscious to unconscious
	**/
	virtual bool TestKnockoutBlow( idEntity* attacker, const idVec3& dir, trace_t *tr, int location, bool bIsPowerBlow, bool performAttack = true)
	{
		return false;
	};

	/**
	 * greebo: Plays the footstep sound according to the current movement type.
	 *         Note: AI and Player are overriding this method.
	 */
	virtual void PlayFootStepSound(); // empty default implementation

	// Links script variables (is overridden by idAI and idPlayer)
	virtual void LinkScriptVariables();

private:
	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup( void );

	/**
	 * greebo: This loads the vocal set (the snd_* spawnargs) into this entity's spawnargs.
	 */
	void					LoadVocalSet();
	
	/**
	* ishtvan: Load a set of melee difficulty options
	**/
	void					LoadMeleeSet();

	void					SetupHead( void );

public:
	void					Event_EnableEyeFocus( void );
	void					Event_DisableEyeFocus( void );
	void					Event_Footstep( void );
	void					Event_EnableWalkIK( void );
	void					Event_DisableWalkIK( void );
	void					Event_EnableLegIK( int num );
	void					Event_DisableLegIK( int num );
	void					Event_SetAnimPrefix( const char *name );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_PreventPain( float duration );
	void					Event_DisablePain( void );
	void					Event_EnablePain( void );
	void					Event_GetPainAnim( void );
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( int channel, const char *name );
	void					Event_PauseAnim( int channel, bool bPause );
	void					Event_AnimIsPaused( int channel );
	void					Event_PlayCycle( int channel, const char *name );
	void					Event_IdleAnim( int channel, const char *name );
	void					Event_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Event_SyncAnimChannels(int fromChannel, int toChannel, float blendFrames);
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_DisableAnimchannel( int channel );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_AnimState( int channel, const char *name, int blendFrames );
	void					Event_GetAnimState( int channel );
	void					Event_InAnimState( int channel, const char *name );
	void					Event_FinishAction( const char *name );
	void					Event_ReloadTorchReplacementAnims( void ); // grayman #3166
	void					Event_FinishChannelAction(int channel, const char *name );
	void					Event_AnimDone( int channel, int blendFrames );
	void					Event_HasAnim( int channel, const char *name );
	void					Event_CheckAnim( int channel, const char *animname );
	void					Event_ChooseAnim( int channel, const char *animname );
	void					Event_AnimLength( int channel, const char *animname );
	void					Event_AnimDistance( int channel, const char *animname );
	void					Event_HasEnemies( void );
	void					Event_NextEnemy( idEntity *ent );
	void					Event_ClosestEnemyToPoint( const idVec3 &pos );
	void					Event_StopSound( int channel, int netsync );
	void					Event_SetNextState( const char *name );
	void					Event_SetState( const char *name );
	void					Event_GetState( void );
	void					Event_GetHead( void );
	void					Event_GetEyePos( void );

	// Obsttorte: #0540
	void					Event_getAnimRate(int channel, const char* animName);
	void					Event_setAnimRate(int channel, const char* animName, float animRate);
	void					Event_getAnimList(int channel);

	// greebo: Moved these from idAI to here
	void					Event_SetHealth( float newHealth );
	void					Event_GetHealth( void );

	void					Event_SetMaxHealth( float newMaxHealth );

	/**
	* Attaches the entity and gives it the given attachment name
	**/
	void					Event_Attach( idEntity *ent, const char *AttName );
	/**
	* Attaches the entity to the named position PosName and gives it the attachment name AttName
	**/
	void					Event_AttachToPos( idEntity *ent, const char *PosName, const char *AttName );
	void					Event_GetAttachment( const char *AttName );
	void					Event_GetAttachmentInd( int ind );
	void					Event_GetNumAttachments( void );

	// Returns the number of ranged/melee weapons attached to the calling script
	void					Event_GetNumMeleeWeapons();
	void					Event_GetNumRangedWeapons();
	/**
	* Registers the start of a given melee attack/parry
	* Intended to be called from a script that also starts the animation
	**/
	void					Event_MeleeAttackStarted( int AttType );
	void					Event_MeleeParryStarted( int ParType );
	/** Called when the melee action reaches the "hold" point **/
	void					Event_MeleeActionHeld( void );
	/** Called when the melee action is released from the hold point **/
	void					Event_MeleeActionReleased( void );
	/** Called when the animation for the melee action has finished **/
	void					Event_MeleeActionFinished( void );
	/** Get the current melee action state **/
	void					Event_GetMeleeActionState( void );
	/** Get the current melee action phase **/
	void					Event_GetMeleeActionPhase( void );
	/** Get the current melee action type **/
	void					Event_GetMeleeActionType( void );
	/** Get the time at which the previous melee action finished **/
	void					Event_GetMeleeLastActTime( void );
	/** Called by script to get result of last melee action **/
	void					Event_GetMeleeResult( void );
	/** Get the type of the last melee attack we were hit with (defaults to MELEETYPE_UNBLOCKABLE if we were not hit) **/
	void					Event_GetMeleeLastHitByType( void );

	/**
	* Returns the melee type integer of the optimal melee parry given current attackers
	* If no attackers are found within the FOV, 
	* returns a default of MELEETYPE_RL (sabre parry #4)
	**/
	void					Event_MeleeBestParry();
	/**
	* Convert a melee type integer to the corresponding string name
	**/
	void					Event_MeleeNameForNum( int num );

	// Script interface for replacing anims with different ones
	void					Event_SetReplacementAnim(const char* animToReplace, const char* replacementAnim);
	void					Event_RemoveReplacementAnim(const char* animName);
	void					Event_LookupReplacementAnim(const char* animName);

	void					Event_GetAttackFlag(int combatType);
	void					Event_SetAttackFlag(int combatType, int enabled);

#ifdef TIMING_BUILD
public:
	int actorGetObstaclesTimer;
	int actorGetPointOutsideObstaclesTimer;
	int actorGetWallEdgesTimer;
	int actorSortWallEdgesTimer;
	int actorBuildPathTreeTimer;
	int actorPrunePathTreeTimer;
	int actorFindOptimalPathTimer;
	int actorRouteToGoalTimer;
	int actorSubSampleWalkPathTimer;
	int actorWalkPathValidTimer;
#endif
};

#endif /* !__GAME_ACTOR_H__ */
