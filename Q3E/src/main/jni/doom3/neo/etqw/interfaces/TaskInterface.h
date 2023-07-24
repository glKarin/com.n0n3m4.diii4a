// Copyright (C) 2007 Id Software, Inc.
//



#ifndef __GAME_INTERFACES_TASKINTERFACE_H__
#define __GAME_INTERFACES_TASKINTERFACE_H__

class sdTaskInterface {
public:
										sdTaskInterface( idEntity* entity );
	virtual								~sdTaskInterface( void ) { ; }

	idLinkList< idEntity >&				GetNode( void ) { return taskNode; }

private:
	idLinkList< idEntity >				taskNode;
};

#endif // __GAME_INTERFACES_TASKINTERFACE_H__
