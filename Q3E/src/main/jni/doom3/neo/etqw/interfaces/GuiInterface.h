// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_INTERFACES_GUIINTERFACE_H__
#define __GAME_INTERFACES_GUIINTERFACE_H__

class sdGuiInterface {
public:
	virtual								~sdGuiInterface( void ) { ; }

	virtual void						UpdateGui( void ) = 0;
	virtual void						HandleGuiScriptMessage( idPlayer* player, const char* message ) = 0;
};

#endif // __GAME_INTERFACES_GUIINTERFACE_H__

