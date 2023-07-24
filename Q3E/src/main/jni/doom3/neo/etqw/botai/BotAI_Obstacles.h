// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTOBSTACLES_H__
#define __BOTOBSTACLES_H__

class idBotObstacle {
public:

	friend class idBotAI;
	friend class idBotThreadData; //mal_FIXME: when get map loading finished, take this out!

							idBotObstacle();
	virtual					~idBotObstacle() {}


private:
	int					num;			//mal: this will be a # > ( MAX_ENTITIES + MAX_CLIENTS ).
	int					areaNum[2];		// area number in 
	idBox				bbox;
};

#endif /* __BOTOBSTACLES_H__ */
