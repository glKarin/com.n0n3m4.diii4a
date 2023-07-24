// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTDEBUG_H__
#define __BOTDEBUG_H__

#include "Bot_Common.h"

struct botDebugInfo_t {
	bool					inUse;
	int						botClientNum;
	int						tacticalActionNum;
	int						aasAreaNum;
	int						enemy;
	int						actionNumber;
	int						target;
	int						routeNum;
	int						vehicleNodeNum;
	int						botGoalTypeTarget;
	float					entDist;
	const char*				aiState;
	const char*				aiNode;
	const char*				moveNode;
	botGoalTypes_t			botGoalType;
};

#endif /* !__BOTDEBUG_H__ */
