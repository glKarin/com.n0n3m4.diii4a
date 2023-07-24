// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclDeployMask.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclDeployMask

===============================================================================
*/

/*
================
sdDeclDeployMask::sdDeclDeployMask
================
*/
sdDeclDeployMask::sdDeclDeployMask( void ) {
	FreeData();
}

/*
================
sdDeclDeployMask::~sdDeclDeployMask
================
*/
sdDeclDeployMask::~sdDeclDeployMask( void ) {
	FreeData();
}

/*
================
sdDeclDeployMask::DefaultDefinition
================
*/
const char* sdDeclDeployMask::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclDeployMask::Parse
================
*/
bool sdDeclDeployMask::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );	

	src.SkipUntilString( "{", &token );

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "mask" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclDeployMask::ParseLevel Missing Parm for 'mask'" );
				return false;
			}

			deployMask.Load( token );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclDeployMask::Parse Invalid Token '%s' in deploymask def '%s'", token.c_str(), base->GetName() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclDeployMask::FreeData
================
*/
void sdDeclDeployMask::FreeData( void ) {
	deployMask.Clear();
}

/*
================
sdDeclDeployMask::CacheFromDict
================
*/
void sdDeclDeployMask::CacheFromDict( const idDict& dict ) {
	const idKeyValue *kv;

	kv = NULL;
	while( kv = dict.MatchPrefix( "dm_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declDeployMaskType[ kv->GetValue() ];
		}
	}
}





/*
===============================================================================

	sdHeightMapInstance

===============================================================================
*/

/*
================
sdDeployMaskInstance::Init
================
*/
void sdDeployMaskInstance::Init( const char* declName, const sdBounds2D& bounds ) {
	const sdDeclDeployMask* declDeployMask = gameLocal.declDeployMaskType[ declName ];
	if ( declDeployMask == NULL ) {
		gameLocal.Error( "sdDeployMaskInstance::Init Invalid Deploy Mask '%s'", declName );
	}
	
	deployMask = &declDeployMask->GetMask();
	deployMaskData.Init( bounds );
}
