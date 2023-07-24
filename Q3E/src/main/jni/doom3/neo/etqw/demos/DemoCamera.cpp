// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DemoCamera.h"

//===============================================================
//
//	sdDemoCamera
//
//===============================================================

/*
============
sdDemoCamera::ParseKey
============
*/
bool sdDemoCamera::ParseKey( const idToken& key, idParser& src ) {
	idToken token;

	if ( !key.Icmp( "name" ) ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}

		name = token;
		return true;
	}

	return false;		
}

//===============================================================
//
//	sdDemoCamera_Fixed
//
//===============================================================

/*
============
sdDemoCamera_Fixed::Parse
============
*/
bool sdDemoCamera_Fixed::Parse( idParser& src ) {
	
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;

	while( true ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		} else if ( !token.Icmp( "origin" ) ) {
			if ( !src.Parse1DMatrix( 3, origin.ToFloatPtr() ) ) {
				return false;
			}
		} else if ( !token.Icmp( "axis" ) ) {
			if ( !src.Parse2DMatrix( 3, 3, axis.ToFloatPtr() ) ) {
				return false;
			}
		} else if ( !token.Icmp( "angles" ) ) {
			idAngles angles;
			if ( !src.Parse1DMatrix( 3, angles.ToFloatPtr() ) ) {
				return false;
			}
			axis = angles.ToMat3();
		} else if ( !token.Icmp( "fov" ) ) {
			fov = src.ParseFloat();
		} else if ( !sdDemoCamera::ParseKey( token, src ) ) {
			src.Error( "sdDemoCamera_Fixed::Parse : Unknown keyword '%s'", token.c_str() );
			return false;
		}
	}

	return true;
}

//===============================================================
//
//	sdDemoCamera_Anim
//
//===============================================================

/*
============
sdDemoCamera_Anim::Parse
============
*/
bool sdDemoCamera_Anim::Parse( idParser& src ) {
	
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;
	int cycle = 1;
	idVec3 offset( vec3_origin );

	while( true ) {
		if ( !src.ExpectAnyToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		} else if ( !token.Icmp( "anim" ) ) {
			if ( !src.ExpectAnyToken( &token ) ) {
				return false;
			}

			if ( !cameraMD5.LoadAnim( token ) ) {
				return false;
			}
		} else if ( !token.Icmp( "cycle" ) ) {
			cycle = src.ParseInt();
		} else if ( !token.Icmp( "offset" ) ) {
			if ( !src.Parse1DMatrix( 3, offset.ToFloatPtr() ) ) {
				return false;
			}
		} else if ( !sdDemoCamera::ParseKey( token, src ) ) {
			src.Error( "sdDemoCamera_Anim::Parse : Unknown keyword '%s'", token.c_str() );
			return false;
		}
	}

	if ( !cycle ) {
		cycle = 1;
	}
	cameraMD5.SetCycle( cycle );

	cameraMD5.SetOffset( offset );

	return true;
}

/*
============
sdDemoCamera_Anim::Start
============
*/
void sdDemoCamera_Anim::Start() {
	cameraMD5.SetStartTime( gameLocal.time );
}

/*
============
sdDemoCamera_Anim::RunFrame
============
*/
void sdDemoCamera_Anim::RunFrame() {
	cameraMD5.Evaluate( origin, axis, fov, gameLocal.time );
}
