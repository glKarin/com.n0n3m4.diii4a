// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "CrosshairInfo.h"
#include "Entity.h"

/*
===============================================================================

	sdCrosshairInfo

===============================================================================
*/

const int FADETIME = SEC2MS( 0.5 );
const int FULLTIME = SEC2MS( 0.25f );

/*
================
sdCrosshairInfo::sdCrosshairInfo
================
*/
sdCrosshairInfo::sdCrosshairInfo( void ) {
	time = 0;
}

/*
================
sdCrosshairInfo::IsValid
================
*/
bool sdCrosshairInfo::IsValid( void ) const {
	return time >= gameLocal.time;
}

/*
================
sdCrosshairInfo::IsValid
================
*/
bool sdCrosshairInfo::IsUseValid( void ) const {
	return useTime >= gameLocal.time;
}

/*
================
sdCrosshairInfo::Invalidate
================
*/
void sdCrosshairInfo::Invalidate( void ) {
	time = 0;
	useTime = 0;
}

/*
================
sdCrosshairInfo::GetEntity
================
*/
idEntity* sdCrosshairInfo::GetEntity( void ) const {
	return owner.GetEntity();
}

/*
================
sdCrosshairInfo::SetEntity
================
*/
void sdCrosshairInfo::SetEntity( idEntity* entity ) {
	owner = entity;
}


/*
================
sdCrosshairInfo::Validate
================
*/
void sdCrosshairInfo::Validate( void ) {
	time = gameLocal.time + FADETIME + FULLTIME;
	useTime = gameLocal.time + gameLocal.msec;
}

/*
================
sdCrosshairInfo::GetAlpha
================
*/
float sdCrosshairInfo::GetAlpha( void ) const {
	int timeleft = time - gameLocal.time;

	if( timeleft > FADETIME ) {
		return 1.f;
	}

	return timeleft / static_cast< float >( FADETIME );
}

