// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclHeightMap.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclHeightMap

===============================================================================
*/

/*
================
sdDeclHeightMap::sdDeclHeightMap
================
*/
sdDeclHeightMap::sdDeclHeightMap( void ) {
	FreeData();
}

/*
================
sdDeclHeightMap::~sdDeclHeightMap
================
*/
sdDeclHeightMap::~sdDeclHeightMap( void ) {
	FreeData();
}

/*
================
sdDeclHeightMap::DefaultDefinition
================
*/
const char* sdDeclHeightMap::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclHeightMap::Parse
================
*/
bool sdDeclHeightMap::Parse( const char *text, const int textLength ) {
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

		if ( !token.Icmp( "heightmap" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclHeightMap::ParseLevel Missing Parm for 'heightmap'" );
				return false;
			}

			heightMap.Load( token );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclHeightMap::Parse Invalid Token '%s' in heightmap def '%s'", token.c_str(), base->GetName() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclHeightMap::FreeData
================
*/
void sdDeclHeightMap::FreeData( void ) {
	heightMap.Clear();
}

/*
================
sdDeclHeightMap::CacheFromDict
================
*/
void sdDeclHeightMap::CacheFromDict( const idDict& dict ) {
	const idKeyValue *kv;

	kv = NULL;
	while( kv = dict.MatchPrefix( "hm_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declHeightMapType[ kv->GetValue() ];
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
sdHeightMapInstance::Init
================
*/
void sdHeightMapInstance::Init( const char* declName, const idBounds& bounds ) {
	const sdDeclHeightMap* declHeightMap = gameLocal.declHeightMapType[ declName ];
	if ( declHeightMap == NULL ) {
		gameLocal.Error( "sdHeightMapInstance::Init Invalid Heightmap '%s'", declName );
	}
	
	heightMap = &declHeightMap->GetHeightMap();
	heightMapData.Init( bounds );
}

void sdHeightMapInstance::Init( const sdHeightMap *map, const idBounds& bounds ) {
	heightMap = map;
	heightMapData.Init( bounds );
}
