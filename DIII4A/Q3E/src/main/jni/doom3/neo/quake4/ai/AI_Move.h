/*
===============================================================================

AI_Move.h

This file has all movement related typedefs, enums, flags, and structures.  It 
and its sister CPP file were split from AI.h and AI.cpp in order to prevent 
merge conflicts and to make further changes to the system possible.

===============================================================================
*/

#ifndef __AI_MOVE_H__
#define __AI_MOVE_H__


typedef idEntityPtr<idEntity>			entityPointer_t;

#define MAX_PATH_LEN	48

/*
=====================
aiMoveDir_t - used by directional movement code
=====================
*/
typedef enum {
	MOVEDIR_FORWARD,
	MOVEDIR_BACKWARD,
	MOVEDIR_LEFT,
	MOVEDIR_RIGHT,
	MOVEDIR_MAX
} aiMoveDir_t;
extern const char* aiMoveDirectionString [ MOVEDIR_MAX ];

/*
=====================
moveType_t - determines which movement function to call
=====================
*/
typedef enum {
	MOVETYPE_DEAD,
	MOVETYPE_ANIM,
	MOVETYPE_SLIDE,
	MOVETYPE_FLY,
	MOVETYPE_STATIC,
	MOVETYPE_PLAYBACK,
	//twhitaker: added custom move type
	MOVETYPE_CUSTOM,
	NUM_MOVETYPES
} moveType_t;

/*
=====================
aiMoveCommand_t - tells the AI how to get there
=====================
*/
typedef enum {
	MOVE_NONE,
	MOVE_FACE_ENEMY,
	MOVE_FACE_ENTITY,

	// commands < NUM_NONMOVING_COMMANDS don't cause a change in position
	NUM_NONMOVING_COMMANDS,

	MOVE_TO_ENEMY = NUM_NONMOVING_COMMANDS,
	MOVE_TO_ENTITY, 
	MOVE_TO_ATTACK,
	MOVE_TO_HELPER,
	MOVE_TO_TETHER,
	MOVE_TO_COVER,
	MOVE_TO_HIDE,
	MOVE_TO_POSITION,
	MOVE_TO_POSITION_DIRECT,
	MOVE_OUT_OF_RANGE,
	MOVE_SLIDE_TO_POSITION,
	MOVE_WANDER,
	MOVE_RV_PLAYBACK,
	NUM_MOVE_COMMANDS
	
} aiMoveCommand_t;

/*
=====================
moveStatus_t - status results from move commands

make sure to change script/doom_defs.script if you add any, or change their order
=====================
*/
typedef enum {
	MOVE_STATUS_DONE,
	MOVE_STATUS_MOVING,
	MOVE_STATUS_WAITING,
	MOVE_STATUS_DEST_NOT_FOUND,
	MOVE_STATUS_DEST_UNREACHABLE,
	MOVE_STATUS_BLOCKED_BY_WALL,
	MOVE_STATUS_BLOCKED_BY_OBJECT,
	MOVE_STATUS_BLOCKED_BY_ENEMY,
	MOVE_STATUS_BLOCKED_BY_MONSTER,
	MOVE_STATUS_BLOCKED_BY_PLAYER,
	MOVE_STATUS_DISABLED,
	NUM_MOVE_STATUS,
} moveStatus_t;
extern const char* aiMoveStatusString[ NUM_MOVE_STATUS ];

/*
=====================
stopEvent_t - used by path prediction
=====================
*/
typedef enum {
	SE_BLOCKED			= BIT(0),
	SE_ENTER_LEDGE_AREA	= BIT(1),
	SE_ENTER_OBSTACLE	= BIT(2),
	SE_FALL				= BIT(3),
	SE_LAND				= BIT(4)
} stopEvent_t;

/*
=====================
obstaclePath_s
=====================
*/
typedef struct obstaclePath_s {
	idVec3				seekPos;					// seek position avoiding obstacles
	idEntity *			firstObstacle;				// if != NULL the first obstacle along the path
	idVec3				startPosOutsideObstacles;	// start position outside obstacles
	idEntity *			startPosObstacle;			// if != NULL the obstacle containing the start position 
	idVec3				seekPosOutsideObstacles;	// seek position outside obstacles
	idEntity *			seekPosObstacle;			// if != NULL the obstacle containing the seek position 
	// RAVEN BEGIN
	// cdr: Alternate Routes Bug
	idList<idEntity*>	allObstacles;
	// RAVEN END
} obstaclePath_t;

/*
=====================
predictedPath_s
=====================
*/
typedef struct predictedPath_s {
	idVec3				endPos;						// final position
	idVec3				endVelocity;				// velocity at end position
	idVec3				endNormal;					// normal of blocking surface
	int					endTime;					// time predicted
	int					endEvent;					// event that stopped the prediction
	const idEntity *	blockingEntity;				// entity that blocks the movement
} predictedPath_t;


/*
=====================
pathSeek_s
=====================
*/
typedef struct pathSeek_s {
	idReachability*		reach;
	idVec3				seekPos;
} pathSeek_t;



/*
=====================
idMoveState
=====================
*/
class idMoveState {
public:
							idMoveState();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Spawn( idDict &spawnArgs );


	struct movementFlags_s {

		// current state
		bool		done				:1;
		bool		moving				:1;
		bool		crouching			:1;		// is currently crouching?
		bool		running				:1;		// is currently running?
		bool		blocked				:1;
		bool		obstacleInPath		:1;
		bool		goalUnreachable		:1;
		bool		onGround			:1;
		bool		flyTurning			:1;		// lean into turns when flying

		// ideal state
		bool		idealRunning		:1;		// what we want running to be


		// various enable / disable flags
		bool		disabled			:1;		// is movement disabled?  (this includes turning)
		bool		ignoreObstacles		:1;		// don't check for obsticles in path

		bool		allowDirectional	:1;		// allows directional movement (ie. running sideways, backwards)
		bool		allowAnimMove		:1;		// allows any animation movement
		bool		allowPrevAnimMove	:1;		// allows slide move if current animmove has no motion extraction on it (smooth transitions)
		bool		allowHiddenMove		:1;		// allows character to still move around while hidden
		bool		allowPushMovables	:1;		// allows the articulated figure to push moveable objects
		bool		allowSlideToGoal	:1;		// allows the AI to excactly slide to the goal position if close enough

		bool		noRun				:1;		// force the actor to walk
		bool		noWalk				:1;		// force the actor to run
		bool		noTurn				:1;		// can the ai turn?
		bool		noGravity			:1;		// dont use gravity
		bool		noRangedInterrupt	:1;		// dont stop halfway to attack position just because you have a clear shot
	} fl;


	moveType_t				moveType;
	aiMoveCommand_t			moveCommand;
	moveStatus_t			moveStatus;
	idVec3					moveDest;
	idVec3					moveDir;				// used for wandering and slide moves

	int						toAreaNum;
	int						startTime;
	int						duration;
	float					speed;					// only used by flying creatures
	float					range;
	float					wanderYaw;
	int						nextWanderTime;
	int						blockTime;
	idEntityPtr<idEntity>	obstacle;
	idVec3					lastMoveOrigin;
	int						lastMoveTime;
	int						anim;

	int						travelFlags;

	float					kickForce;
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

	aiMoveDir_t				currentDirection;		// Direction currently moving in
	aiMoveDir_t				idealDirection;			// Direction we want to be moving in
	float					walkRange;				// Distance to target before starting to walk
	float					walkTurn;				// Turn delta threshold for walking when turning
	idVec2					followRange;			// Min and max range for AI following their leader	
	idVec2					searchRange;			// Min and max range to use when searching for a new place to move to
	float					attackPositionRange;	// Override for how close you have to get to your attackPosition before stopping
	float					turnDelta;				// Amount to turn when turning

	idVec3					goalPos;
	int						goalArea;
	entityPointer_t			goalEntity;
	idVec3					goalEntityOrigin;		// move to entity uses this to avoid checking the floor position every frame

	idVec3					myPos;
	int						myArea;

	idVec3					seekPos;

	pathSeek_t				path[MAX_PATH_LEN];
	int						pathLen;
	int						pathArea;
	int						pathTime;

	idVec3					addVelocity;
};

// cdr: Obstacle Avoidance
void AI_EntityMoved(idEntity* ent);
void AI_MoveInitialize();


#endif /* !__AI_MOVE_H__ */
