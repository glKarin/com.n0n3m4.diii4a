// DnAI.h
//

#pragma once

class idPlayer;

const float	SQUARE_ROOT_OF_2 = 1.414213562f;
const float	AI_TURN_PREDICTION = 0.2f;
const float	AI_TURN_SCALE = 60.0f;
const float	AI_SEEK_PREDICTION = 0.3f;
const float	AI_FLY_DAMPENING = 0.15f;
const float	AI_HEARING_RANGE = 2048.0f;
const int	DEFAULT_FLY_OFFSET = 68;


// defined in script/ai_base.script.  please keep them up to date.
typedef enum
{
	MOVETYPE_DEAD,
	MOVETYPE_ANIM,
	MOVETYPE_SLIDE,
	MOVETYPE_FLY,
	MOVETYPE_STATIC,
	NUM_MOVETYPES
} moveType_t;

typedef enum
{
	MOVE_NONE,
	MOVE_FACE_ENEMY,
	MOVE_FACE_ENTITY,

	// commands < NUM_NONMOVING_COMMANDS don't cause a change in position
	NUM_NONMOVING_COMMANDS,

	MOVE_TO_ENEMY = NUM_NONMOVING_COMMANDS,
	MOVE_TO_ENEMYHEIGHT,
	MOVE_TO_ENTITY,
	MOVE_OUT_OF_RANGE,
	MOVE_TO_COVER,
	MOVE_TO_POSITION,
	MOVE_TO_POSITION_DIRECT,
	MOVE_SLIDE_TO_POSITION,
	MOVE_WANDER,
	NUM_MOVE_COMMANDS
} moveCommand_t;

typedef enum
{
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
typedef enum
{
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


class idMoveState
{
public:
	idMoveState()
	{
		moveType = MOVETYPE_ANIM;
		moveCommand = MOVE_NONE;
		moveStatus = MOVE_STATUS_DONE;
		moveDest.Zero();
		moveDir.Set(1.0f, 0.0f, 0.0f);
		goalEntity = NULL;
		goalEntityOrigin.Zero();
		toAreaNum = 0;
		startTime = 0;
		duration = 0;
		speed = 0.0f;
		range = 0.0f;
		wanderYaw = 0;
		nextWanderTime = 0;
		blockTime = 0;
		obstacle = NULL;
		lastMoveOrigin = vec3_origin;
		lastMoveTime = 0;
		anim = 0;
	}

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

//
// DnRand 
// Basically a wrapper for krand from Duke3D.
//
class DnRand
{
public:
	DnRand()
	{
		randomseed = 17;
	}

	unsigned int GetRand()
	{
		randomseed = (randomseed * 27584621) + 1;
		return(((unsigned int)randomseed) >> 16);
	}

	bool ifrnd(int val1)
	{
		unsigned int r = ((GetRand() >> 8));

		return (r >= 255 - val1);
	}
private:
	int randomseed;
};

extern DnRand dnRand;

//
// DnAI
//
class DnAI : public idActor
{
	CLASS_PROTOTYPE(DnAI);
public:
	void					Spawn(void);
	void					Think(void);

	// damage
	virtual bool			Pain(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location) override;
	virtual void			Killed(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location) override;

	bool					CurrentlyPlayingSound();

	bool					FacingIdeal();
	bool					Event_TurnToward(float yaw);
	bool					Event_TurnToward(const idVec3& pos);

	bool					IsOnGround() { return AI_ONGROUND; }
	bool					MoveToPosition(const idVec3& pos);

	idEntity*				GetEnemy() { return target; }
protected:
	void					Event_SetAnimation(const char* anim, bool loop);
	void					Event_Hitscan(const char *damage_type, const idVec3& muzzleOrigin, const idVec3& dir, int num_hitscans, float spread, float power);
	void					Event_UpdatePathToPosition(idVec3 position);
	idPlayer*				Event_FindNewTarget();
	void					Event_ResetAnimation();
	void					Event_StopMove();

	idStr					GetCurrentAnimation() { return currentAnimation; }
protected:
	void					Event_SetHandWeapon(const char* modelName);

	idActor*				target;
	idVec3					targetLastSeenLocation;
	bool					isTargetVisible;

	idPhysics_Monster		physicsObj;

	float					ideal_yaw;
	float					current_yaw;
	float					turnRate;
	float					turnVel;

	idList<idVec3>			pathWaypoints;
	int						waypointId;
	bool					AI_PAIN;
private:
	void					StopMove(moveStatus_t status);

	idMoveState				move;

	void					Turn();
	void					SetupPhysics(void);
	void					SlideMove();
	bool					ReachedPos(const idVec3& pos, const moveCommand_t moveCommand) const;

	bool					AI_ONGROUND;
	bool					AI_BLOCKED;

	idStr					currentAnimation;
protected:
	int						EgoKillValue;
	bool					startedDeath;
};

#include "../Monsters/Monster_Pigcop.h"
#include "../Monsters/Monster_Liztroop.h"
#include "../Civilians/Civilian.h"