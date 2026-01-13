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

#ifndef __AI_H__
#define __AI_H__

#include "physics/Physics_Monster.h"
#include "Entity.h"
#include "Actor.h"
#include "Projectile.h"

class idFuncEmitter;

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
const float	AI_SUSPICIOUS_RANGE_Z		= 320.0f; //ff1.3
const int	DEFAULT_FLY_OFFSET			= 68;
const int	FRIEND_FLY_OFFSET			= 48; //ff1.3

#define ATTACK_IGNORE			0
#define ATTACK_ON_DAMAGE		1
#define ATTACK_ON_ACTIVATE		2
#define ATTACK_ON_SIGHT			4

//ff1.3 start
#define FRIEND_FX_NAME			"friendFx"
#define FRIEND_FX_JOINT			"origin"
#define FRIEND_FX_MODEL			"ff_evildone.prt"
//ff1.3 end

// defined in script/ai_base.script.  please keep them up to date.
typedef enum {
	MOVETYPE_DEAD,
	MOVETYPE_ANIM,
	MOVETYPE_SLIDE,
	MOVETYPE_FLY,
	MOVETYPE_STATIC,
	NUM_MOVETYPES
} moveType_t;

typedef enum {
	MOVE_NONE,
	MOVE_FACE_ENEMY,
	MOVE_FACE_ENTITY,

	// commands < NUM_NONMOVING_COMMANDS don't cause a change in position
	NUM_NONMOVING_COMMANDS,

	MOVE_TO_ENEMY = NUM_NONMOVING_COMMANDS,
	MOVE_TO_ENEMYHEIGHT,
	MOVE_TO_ENTITY,
	MOVE_OUT_OF_RANGE,
	MOVE_TO_ATTACK_POSITION,
	MOVE_TO_COVER,
	MOVE_TO_POSITION,
	MOVE_TO_POSITION_DIRECT,
	MOVE_SLIDE_TO_POSITION,
	MOVE_WANDER,
	NUM_MOVE_COMMANDS
} moveCommand_t;

typedef enum {
	TALK_NEVER,
	TALK_DEAD,
	TALK_OK,
	TALK_BUSY,
	NUM_TALK_STATES
} talkState_t;

//
// status results from move commands
// make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum {
	MOVE_STATUS_DONE,
	MOVE_STATUS_MOVING,
	MOVE_STATUS_WAITING,
	MOVE_STATUS_DEST_NOT_FOUND,
	MOVE_STATUS_DEST_UNREACHABLE,
	MOVE_STATUS_BLOCKED_BY_WALL,
	MOVE_STATUS_BLOCKED_BY_OBJECT,
	MOVE_STATUS_BLOCKED_BY_ENEMY,
	MOVE_STATUS_BLOCKED_BY_MONSTER
} moveStatus_t;

#define	DI_NODIR	-1

// obstacle avoidance
typedef struct obstaclePath_s {
	idVec3				seekPos;					// seek position avoiding obstacles
	idEntity *			firstObstacle;				// if != NULL the first obstacle along the path
	idVec3				startPosOutsideObstacles;	// start position outside obstacles
	idEntity *			startPosObstacle;			// if != NULL the obstacle containing the start position
	idVec3				seekPosOutsideObstacles;	// seek position outside obstacles
	idEntity *			seekPosObstacle;			// if != NULL the obstacle containing the seek position
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
extern const idEventDef AI_AttackMissile;
extern const idEventDef AI_FireMissileAtTarget;
#ifdef _D3XP
extern const idEventDef AI_LaunchProjectile;
//extern const idEventDef AI_TriggerFX; //ff1.1 - moved to idAnimatedEntity
extern const idEventDef AI_StartEmitter;
extern const idEventDef AI_StopEmitter;
#endif
extern const idEventDef AI_AttackMelee;
extern const idEventDef AI_DirectDamage;
extern const idEventDef AI_JumpFrame;
extern const idEventDef AI_EnableClip;
extern const idEventDef AI_DisableClip;
extern const idEventDef AI_EnableGravity;
extern const idEventDef AI_DisableGravity;
extern const idEventDef AI_TriggerParticles;
extern const idEventDef AI_RandomPath;
extern const idEventDef AI_ClearEnemy; //ff1.3
extern const idEventDef AI_StartRiding; //ff1.3
extern const idEventDef AI_StopRiding; //ff1.3
extern const idEventDef AI_MeleeAttackToJoint; //ff1.3
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

#ifdef _D3XP
typedef struct funcEmitter_s {
	char				name[64];
	idFuncEmitter*		particle;
	jointHandle_t		joint;
} funcEmitter_t;
#endif

class idMoveState {
public:
							idMoveState();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	moveType_t				moveType;
	moveCommand_t			moveCommand;
	moveStatus_t			moveStatus;
	idVec3					moveDest;
	idVec3					moveDir;			// used for wandering and slide moves
	idEntityPtr<idEntity>	goalEntity;
	idVec3					goalEntityOrigin;	// move to entity uses this to avoid checking the floor position every frame
	int						toAreaNum;
	int						startTime;
	int						duration;
	float					speed;				// only used by flying creatures
	float					range;
	float					wanderYaw;
	int						nextWanderTime;
	int						blockTime;
	idEntityPtr<idEntity>	obstacle;
	idVec3					lastMoveOrigin;
	int						lastMoveTime;
	int						anim;
};

class idAASFindCover : public idAASCallback {
public:
						idAASFindCover( const idVec3 &hideFromPos );
						~idAASFindCover();

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	pvsHandle_t			hidePVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
};

class idAASFindAreaOutOfRange : public idAASCallback {
public:
						idAASFindAreaOutOfRange( const idVec3 &targetPos, float maxDist );

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	idVec3				targetPos;
	float				maxDistSqr;
};

class idAI;

class idAASFindAttackPosition : public idAASCallback {
public:
						idAASFindAttackPosition( const idAI *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &fireOffset );
						~idAASFindAttackPosition();

	virtual bool		TestArea( const idAAS *aas, int areaNum );

private:
	const idAI			*self;
	idEntity			*target;
	idBounds			excludeBounds;
	idVec3				targetPos;
	idVec3				fireOffset;
	idMat3				gravityAxis;
	pvsHandle_t			targetPVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
};

class idAI : public idActor {
public:
	CLASS_PROTOTYPE( idAI );

							idAI();
							~idAI();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					HeardSound( idEntity *ent, const char *action );
	idActor					*GetEnemy( void ) const;
	void					TalkTo( idActor *actor );
	talkState_t				GetTalkState( void ) const;

	bool					GetAimDir( const idVec3 &firePos, idEntity *aimAtEnt, const idEntity *ignore, idVec3 &aimDir, bool useLastVisiblePos=true ) const; //ff1.3 - useLastVisiblePos added

	void					TouchedByFlashlight( idActor *flashlight_owner );

							// Outputs a list of all monsters to the console.
	static void				List_f( const idCmdArgs &args );

							// Finds a path around dynamic obstacles.
	static bool				FindPathAroundObstacles( const idPhysics *physics, const idAAS *aas, const idEntity *ignore, const idVec3 &startPos, const idVec3 &seekPos, obstaclePath_t &path );
							// Frees any nodes used for the dynamic obstacle avoidance.
	static void				FreeObstacleAvoidanceNodes( void );
							// Predicts movement, returns true if a stop event was triggered.
	static bool				PredictPath( const idEntity *ent, const idAAS *aas, const idVec3 &start, const idVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path );
							// Return true if the trajectory of the clip model is collision free.
	static bool				TestTrajectory( const idVec3 &start, const idVec3 &end, float zVel, float gravity, float time, float max_height, const idClipModel *clip, int clipmask, const idEntity *ignore, const idEntity *targetEntity, int drawtime );
							// Finds the best collision free trajectory for a clip model.
	static bool				PredictTrajectory( const idVec3 &firePos, const idVec3 &target, float projectileSpeed, const idVec3 &projGravity, const idClipModel *clip, int clipmask, float max_height, const idEntity *ignore, const idEntity *targetEntity, int drawtime, idVec3 &aimDir );

#ifdef _D3XP
	virtual void			Gib( const idVec3 &dir, const char *damageDefName );
#endif

	//ff1.3 start
	bool					CanBeRidden( void );
	float					GetProjectileDamageScale( void ) const;
	//ff1.3 end

protected:
	// navigation
	idAAS *					aas;
	int						travelFlags;

	idMoveState				move;
	idMoveState				savedMove;

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
	bool					af_push_moveables;			// allow the articulated figure to push moveable objects

	// weapon/attack vars
	bool					lastHitCheckResult;
	int						lastHitCheckTime;
	int						lastAttackTime;
	float					melee_range;
	float					projectile_height_to_distance_ratio;	// calculates the maximum height a projectile can be thrown
	idList<idVec3>			missileLaunchOffset;

	const idDict *			projectileDef;
	mutable idClipModel		*projectileClipModel;
	float					projectileRadius;
	float					projectileSpeed;
	idVec3					projectileVelocity;
	idVec3					projectileGravity;
	idEntityPtr<idProjectile> projectile;
	idStr					attack;

	// chatter/talking
	const idSoundShader		*chat_snd;
	int						chat_min;
	int						chat_max;
	int						chat_time;
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

	// enemy variables
	idEntityPtr<idActor>	enemy;
	idVec3					lastVisibleEnemyPos;
	idVec3					lastVisibleEnemyEyeOffset;
	idVec3					lastVisibleReachableEnemyPos;
	idVec3					lastReachableEnemyPos;
	bool					wakeOnFlashlight;

#ifdef _D3XP
	bool					spawnClearMoveables;

	idHashTable<funcEmitter_t> funcEmitters;

	idEntityPtr<idHarvestable>	harvestEnt;
#endif

	//ff1.3 start
	int						lastScriptUpdate;
	bool					activateTargetsOnDeath;
	bool					focusPosOnlyInRange; //completely ignore focus position if out of look_min/look_max range
	bool					helltimeMode;
	bool					helltimeAllowed;
	bool					canTouchTriggers;
	bool					forcedMovements;
	float					meleeDamageScale;
	float					projectileDamageScale;
	const idDeclSkin *		helltimeSkin;
	const idDeclSkin *		postHelltimeSkin;
	const idDeclSkin *		postHelltimeHeadSkin;
	//ff1.3 end

	// script variables
	idScriptBool			AI_TALK;
	idScriptBool			AI_DAMAGE;
	idScriptBool			AI_PAIN;
	idScriptFloat			AI_SPECIAL_DAMAGE;
	idScriptBool			AI_DEAD;
	idScriptBool			AI_ENEMY_VISIBLE;
	idScriptBool			AI_ENEMY_IN_FOV;
	idScriptBool			AI_ENEMY_DEAD;
	idScriptBool			AI_MOVE_DONE;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ACTIVATED;
	idScriptBool			AI_FORWARD;
	idScriptBool			AI_JUMP;
	//idScriptBool			AI_ENEMY_REACHABLE;
	idScriptBool			AI_BLOCKED;
	idScriptBool			AI_OBSTACLE_IN_PATH;
	idScriptBool			AI_DEST_UNREACHABLE;
	idScriptBool			AI_HIT_ENEMY;
	idScriptBool			AI_PUSHED;

	//
	// ai/ai.cpp
	//
	void					SetAAS( void );
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	void					Think( void );
	void					Activate( idEntity *activator );
	int						ReactionTo( const idActor *ent );
	bool					CheckForEnemy( void );
	void					EnemyDead( void );
	virtual bool			CanPlayChatterSounds( void ) const;
	void					SetChatSound( void );
	void					PlayChatter( void );
	virtual void			Hide( void );
	virtual void			Show( void );
	idVec3					FirstVisiblePointOnPath( const idVec3 origin, const idVec3 &target, int travelFlags ) const;
	void					CalculateAttackOffsets( void );
	void					PlayCinematic( void );

	// movement
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
	void					GetMoveDelta( const idMat3 &oldaxis, const idMat3 &axis, idVec3 &delta );
	void					CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos );
	void					DeadMove( void );
	void					AnimMove( void );
	void					SlideMove( void );
	void					AdjustFlyingAngles( void );
	void					AddFlyBob( idVec3 &vel );
	void					AdjustFlyHeight( idVec3 &vel, const idVec3 &goalPos );
	void					FlySeekGoal( idVec3 &vel, idVec3 &goalPos );
	void					AdjustFlySpeed( idVec3 &vel );
	void					FlyTurn( void );
	void					FlyMove( void );
	void					StaticMove( void );

	// damage
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	// navigation
	void					KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick );
	bool					ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const;
	float					TravelDistance( const idVec3 &start, const idVec3 &end ) const;
	int						PointReachableAreaNum( const idVec3 &pos, const float boundsScale = 2.0f ) const;
	bool					PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	void					DrawRoute( void ) const;
	bool					GetMovePos( idVec3 &seekPos );
	bool					MoveDone( void ) const;
	bool					EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos );
	void					BlockedFailSafe( void );

	// movement control
	void					StopMove( moveStatus_t status );
	bool					FaceEnemy( void );
	bool					FaceEntity( idEntity *ent );
	bool					DirectMoveToPosition( const idVec3 &pos );
	bool					MoveToEnemyHeight( void );
	bool					MoveOutOfRange( idEntity *entity, float range );
	bool					MoveToAttackPosition( idEntity *ent, int attack_anim );
	bool					MoveToEnemy( void );
	bool					MoveToEntity( idEntity *ent );
	bool					MoveToPosition( const idVec3 &pos );
	bool					MoveToCover( idEntity *entity, const idVec3 &pos );
	bool					SlideToPosition( const idVec3 &pos, float time );
	bool					WanderAround( void );
	bool					StepDirection( float dir );
	bool					NewWanderDir( const idVec3 &dest );

	// effects
	const idDeclParticle	*SpawnParticlesOnJoint( particleEmitter_t &pe, const char *particleName, const char *jointName );
	void					SpawnParticles( const char *keyName );
	bool					ParticlesActive( void );

	// turning
	bool					FacingIdeal( void );
	void					Turn( void );
	bool					TurnToward( float yaw );
	bool					TurnToward( const idVec3 &pos );

	// enemy management
	void					ClearEnemy( void );
	bool					EnemyPositionValid( void ) const;
	void					SetEnemyPosition( void );
	void					UpdateEnemyPosition( void );
	void					SetEnemy( idActor *newEnemy );

	// attacks
	void					CreateProjectileClipModel( void ) const;
	idProjectile			*CreateProjectile( const idVec3 &pos, const idVec3 &dir );
	void					RemoveProjectile( void );
	idProjectile			*LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone );
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
	void					DirectDamage( const char *meleeDefName, idEntity *ent );
	void					DamageEffect( const char *meleeDefName, idEntity *ent, const idVec3 &start, float range ); //ff1.3
	bool					TestMelee( void ) const;
	virtual bool			AttackMelee( const char *meleeDefName ); //ff1.3 - virtual added
	void					BeginAttack( const char *name );
	void					EndAttack( void );
	void					PushWithAF( void );
	//void					CanHitEnemyFromAnim( const char *animname, bool visibleRequired ); //ff1.3

	// special effects
	void					GetMuzzle( const char *jointname, idVec3 &muzzle, idMat3 &axis );
	void					InitMuzzleFlash( void );
	void					TriggerWeaponEffects( const idVec3 &muzzle );
	void					UpdateMuzzleFlash( void );
	virtual bool			UpdateAnimationControllers( void );
	void					UpdateParticles( void );
	void					TriggerParticles( const char *jointName );

#ifdef _D3XP
	//void					TriggerFX( const char* joint, const char* fx ); // ff1.1 - moved to idAnimatedEntity
	idFuncEmitter			*StartEmitter( const char* name, const char* joint, const char* particle, /*ff1.3*/ bool orientated );
	idFuncEmitter			*GetEmitter( const char* name );
	void					StopEmitter( const char* name );
	//ff1.3 start
	//void					SetEmitterVisible( const char* name, bool on );
	void					ChangeTeam( int newTeam );
	void					UpdateFriendFx( void );
	//ff1.3 end
#endif

	//ff1.3 start
	void					UpdateHelltimeMode( bool on );
	virtual void			UpdateAttack( void ){};
	void					FadeOut( void );
	//ff1.3 end

	//ff1.4 start
	void					UnbindEntityOnDeath( void );
	//ff1.4 end

	// AI script state management
	virtual void			LinkScriptVariables( void ); //ff1.3 - virtual added
	void					UpdateAIScript( void );

	//
	// ai/ai_events.cpp
	//
	void					Event_Activate( idEntity *activator );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_FindEnemy( int useFOV );
	void					Event_FindEnemyAI( int useFOV );
	void					Event_FindEnemyInCombatNodes( void );
	void					Event_ClosestReachableEnemyOfEntity( idEntity *team_mate );
	void					Event_HeardSound( int ignore_team );
	void					Event_SetEnemy( idEntity *ent );
	void					Event_ClearEnemy( void );
	void					Event_MuzzleFlash( const char *jointname );
	void					Event_CreateMissile( const char *jointname );
	void					Event_AttackMissile( const char *jointname );
	void					Event_FireMissileAtTarget( const char *jointname, const char *targetname );
	void					Event_LaunchMissile( const idVec3 &muzzle, const idAngles &ang );
#ifdef _D3XP
	void					Event_LaunchProjectile( const char *entityDefName );
#endif
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
	void					Event_SetHealth( float newHealth );
	void					Event_GetHealth( void );
	void					Event_AllowDamage( void );
	void					Event_IgnoreDamage( void );
	void					Event_GetCurrentYaw( void );
	void					Event_TurnTo( float angle );
	void					Event_TurnToPos( const idVec3 &pos );
	void					Event_TurnToEntity( idEntity *ent );
	void					Event_MoveStatus( void );
	void					Event_StopMove( void );
	void					Event_MoveToCover( void );
	void					Event_MoveToEnemy( void );
	void					Event_MoveToEnemyHeight( void );
	void					Event_MoveOutOfRange( idEntity *entity, float range );
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
	void					Event_CanHitNotVisibleEnemyFromAnim( const char *animname ); //ff1.3
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
	void					Event_Disintegrate( void ); //ff1.3
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
	void					Event_CanReachPosition( const idVec3 &pos );
	void					Event_CanReachEntity( idEntity *ent );
	void					Event_CanReachEnemy( void );
	void					Event_GetReachableEntityPosition( idEntity *ent );
#ifdef _D3XP
	void					Event_MoveToPositionDirect( const idVec3 &pos );
	void					Event_AvoidObstacles( int ignore);
	//void					Event_TriggerFX( const char* joint, const char* fx ); // ff1.1 - moved to idAnimatedEntity

	void					Event_StartEmitter( const char* name, const char* joint, const char* particle );
	void					Event_GetEmitter( const char* name );
	void					Event_StopEmitter( const char* name );
#endif

	//ff1.3 start
	void					Event_SetSkin( const char *skinname );
	void					Event_EnemyIsDriving( void );
	void					Event_EnemyIsPlayer( void );
	//void					Event_AllowSlowmoMode( int allow );
	void					Event_SetActivateTargetsOnDeath( int activate );
	void					Event_SetTeam( int newTeam );
	void					Event_RestoreTeam( void );
	//void					Event_SetTemporaryTeam( int newTeam, float timeSec );
	//void					Event_ResetTemporaryTeam( void );
	void					Event_EnemyInFov( void );
	void					Event_EnemyInFovDegrees( float fov );
	void					Event_HeardSuspiciousSound( float focusFov, float focusDistance, float fovDistance, float minDistance );
	void					Event_EnemyInSuspiciousPosition( float focusFov, float focusDistance, float fovDistance, float minDistance );
	void					Event_StartRiding( idEntity* driver, int checkSkippedTriggers );
	void					Event_NoTarget( int enable );

	bool					SuspiciousPosition( const idVec3 &pos, float focusFov, float maxFocusDistance, float maxFovDistance, float minDistance );
	//ff1.3 end
};

//ff1.3 start
ID_INLINE float idAI::GetProjectileDamageScale( void ) const {
	return projectileDamageScale;
}
//ff1.3 end

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


//ff1.3 start
#define RIDE_STATUS_NOTREADY	0
#define RIDE_STATUS_READY		1
#define RIDE_STATUS_STARTED		2
#define RIDE_STATUS_STOPPED		3

#define RIDE_DRIVER_OFFSET		4.0f

//hud pulse flags
const int HUD_RIDE_PULSE_WEAPON			= BIT(0);
//const int HUD_RIDE_PULSE_EXIT_DISABLED	= BIT(1);
//const int HUD_RIDE_PULSE_EXIT_1MORE		= BIT(2);

class idAI_Rideable : public idAI {
public:
	CLASS_PROTOTYPE( idAI_Rideable );

						idAI_Rideable();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				UnbindDriver( bool driverKilled );
	void				PreStopRiding( void );

	void				NextWeapon( void );
	void				PrevWeapon( void );
	bool				PerformImpulse( int impulse );
	void				SelectWeapon( int num );

	void				InitHudStats( idUserInterface *hud, idUserInterface *cursor );
	void				UpdateHudStats( idUserInterface *hud );
	
	//bool				IsTimeMode( void ) const;
	idPlayer *			GetDriver( void ) const;
	void				GetCameraPos( idVec3 &origin, idMat3 &axis );
	float				GetThirdPersonRange( void ) const;
	float				GetThirdPersonHeight( void ) const;

	virtual bool		AttackMelee( const char *meleeDefName );

	virtual bool		IgnoreDamage( idEntity *inflictor, idEntity *attacker, const idDict *damageDef, float *additionalDamageScale );

	virtual void		Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

protected:
	bool				timeMode;
	int					rideEndTime;
	int					rideStatus;
	int					lastStopRequestTime;
	idPlayer *			driver;
	idVec3				cameraOffset;
	idVec3				aimPos;
	int					currentFireMode;
	int					weaponBarSlot;
	int					numFireModes;
	int					hudPulseFlags;
	float				thirdPersonRange;
	float				thirdPersonHeight;
	idScriptBool		AI_ATTACK;
	idScriptBool		AI_SEC_ATTACK;
	idScriptBool		AI_RELOAD;

	void				SetDriverInValidPosition( void );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void		Show( void );
	void				UpdateAttack( void );
	void				UpdateAim( void );
	idProjectile		*LaunchProjectile( const char *jointname, bool clampToAttackCone );
	void				TriggerSkippedTriggers( const idVec3 &startPos );
	
	void				LinkScriptVariables( void );

	void				Event_GetDriver( void );
	void				Event_GetFireMode( void );
	void				Event_GetAimAngles( const idVec3 &firePos );
	void				Event_BindDriver( idEntity* newDriver );
	void				Event_UnbindDriver( void );
	void				Event_FaceIdeal( void );
	void				Event_FaceIdealOffset( float offset );
	void				Event_GetIdealAngles( float offset );
	void				Event_GetMove( void );
	void				Event_AttackMissile( const char *jointname );
	void				Event_LookAtAimPos( float duration );
	void				Event_LookAtEnemyOrAimPos( float duration );
	void				Event_StartRiding( idEntity* driver, int checkSkippedTriggers );
	void				Event_StopRiding( void );
	void				Event_EnableClip( void );
	void				Event_Remove( void );
	//void				Event_MeleeAttackToJoint( const char *jointname, const char *meleeDefName );
};

ID_INLINE idPlayer * idAI_Rideable::GetDriver( void ) const {
	return driver;
}

ID_INLINE float idAI_Rideable::GetThirdPersonRange( void ) const {
	return thirdPersonRange;
}

ID_INLINE float idAI_Rideable::GetThirdPersonHeight( void ) const {
	return thirdPersonHeight;
}
//ff1.3 end

#endif /* !__AI_H__ */
