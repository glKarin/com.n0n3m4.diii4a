// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_FOOTPRINTS_H__
#define __GAME_FOOTPRINTS_H__

class sdFootPrintManager {
public:
	virtual void Init( void ) = 0;
	virtual void Deinit( void ) = 0;

	virtual bool AddFootPrint( const idVec3 & point, const idVec3 &forward, const idVec3 &up, bool right ) = 0;
	
	virtual void Think( void ) = 0;

	virtual renderEntity_t* GetRenderEntity( void ) = 0;
	virtual int				GetModelHandle( void ) = 0;
};

extern sdFootPrintManager *footPrintManager;

#endif //__GAME_FOOTPRINTS_H__
