// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTROUTES_H__
#define __BOTROUTES_H__

#include "Bot_Common.h"

class idBotRoutes {
public:

	friend class idBotAI;
	friend class idBotThreadData; //mal_FIXME: when get map loading finished, take this out!

							idBotRoutes();
	virtual					~idBotRoutes() {}

	void					SetActive( bool isActive ) { active = isActive; }
	int						GetRouteGroup() { return groupID; }


private:

	bool					active;
	bool					isHeadNode;
	int						groupID;
	int						num;
	float					radius;
	playerTeamTypes_t		team;
	idVec3					origin;
	idStr					name;
	idList< idBotRoutes* >	routeLinks;

	//mal_TODO: add more stuff as needed


};

#endif /* __BOTROUTES_H__ */
