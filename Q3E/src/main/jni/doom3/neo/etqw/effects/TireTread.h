// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_TIRETREADS_H__
#define __GAME_TIRETREADS_H__

class sdTireTreadManager {

public:
	virtual void Init( void ) = 0;
	virtual void Deinit( void ) = 0;

	virtual unsigned int  StartSkid( bool isStrogg ) = 0;
	virtual bool AddSkidPoint( unsigned int handle, const idVec3 & point, const idVec3 &forward, const idVec3 &up, const class sdDeclSurfaceType *surface ) = 0;
	virtual void StopSkid( unsigned int handle ) = 0;
	
	virtual void Think( void ) = 0;
};

extern sdTireTreadManager *tireTreadManager;

#endif //__GAME_TIRETREADS_H__
