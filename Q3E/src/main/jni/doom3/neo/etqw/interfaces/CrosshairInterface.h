// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_INTERFACES_CROSSHAIRINTERFACE_H__
#define __GAME_INTERFACES_CROSSHAIRINTERFACE_H__

class sdCrosshairInterface {
public:
	virtual								~sdCrosshairInterface( void ) { ; }

	virtual bool						UpdateCrosshairInfo( idPlayer* player ) const = 0;

private:
};

#endif // __GAME_INTERFACES_CROSSHAIRINTERFACE_H__

