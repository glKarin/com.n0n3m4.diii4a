// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_INTERFACES_NETWORKINTERFACE_H__
#define __GAME_INTERFACES_NETWORKINTERFACE_H__

class sdNetworkInterface {
public:
	virtual								~sdNetworkInterface( void ) { ; }

	virtual void						HandleNetworkMessage( idPlayer* player, const char* message ) = 0;
	virtual void						HandleNetworkEvent( const char* message ) = 0;
};

#endif // __GAME_INTERFACES_NETWORKINTERFACE_H__

