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

#ifndef __AI_MOVESTATE_H__
#define __AI_MOVESTATE_H__

// defined in script/ai_base.script.  please keep them up to date.
// greebo: also be sure to update the ai::setMoveTypeFromSpawnargs() script method in the same file.
// angua: and don't forget to update idAI::SetMoveType( idStr moveType ) 

enum moveType_t {
	MOVETYPE_DEAD,
	MOVETYPE_ANIM,
	MOVETYPE_SLIDE,
	MOVETYPE_FLY,
	MOVETYPE_STATIC,
	MOVETYPE_SIT,
	MOVETYPE_SIT_DOWN,
	MOVETYPE_SLEEP,
	MOVETYPE_FALL_ASLEEP, // grayman #3820 - was MOVETYPE_LAY_DOWN
	MOVETYPE_GET_UP,
	MOVETYPE_WAKE_UP, // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	NUM_MOVETYPES
};

enum moveCommand_t {
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
	MOVE_VECTOR, // (TDM)
	MOVE_FLEE, // (TDM)
	NUM_MOVE_COMMANDS
};

//
// status results from move commands
// make sure to change script/doom_defs.script if you add any, or change their order
//
enum moveStatus_t {
	MOVE_STATUS_DONE,
	MOVE_STATUS_MOVING,
	MOVE_STATUS_WAITING,
	MOVE_STATUS_DEST_NOT_FOUND,
	MOVE_STATUS_DEST_UNREACHABLE,
	MOVE_STATUS_BLOCKED_BY_WALL,
	MOVE_STATUS_BLOCKED_BY_OBJECT,
	MOVE_STATUS_BLOCKED_BY_ENEMY,
	MOVE_STATUS_BLOCKED_BY_MONSTER
};

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
	// angua: the distance at which the AI will see a position as reached
	// if < 0, the AI will use their bounding box for this check as before
	float					accuracy;
};

#endif /* __AI_MOVESTATE_H__ */
