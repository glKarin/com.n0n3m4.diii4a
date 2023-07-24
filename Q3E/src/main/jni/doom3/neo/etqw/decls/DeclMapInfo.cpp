// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclMapInfo.h"
#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclMapInfo

===============================================================================
*/

/*
================
sdDeclMapInfo::sdDeclMapInfo
================
*/
sdDeclMapInfo::sdDeclMapInfo( void ) {
}

/*
================
sdDeclMapInfo::~sdDeclMapInfo
================
*/
sdDeclMapInfo::~sdDeclMapInfo( void ) {
	FreeData();
}

/*
================
sdDeclMapInfo::DefaultDefinition
================
*/
const char* sdDeclMapInfo::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclMapInfo::Parse
================
*/
bool sdDeclMapInfo::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	idDict temp;

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "data" ) ) {

			if ( !temp.Parse( src ) ) {
				return false;
			}
			game->CacheDictionaryMedia( temp );

			data.Copy( temp );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclMapInfo::Parse Invalid Token %s", token.c_str() );
			return false;

		}
	}

	heightMap					= data.GetString( "heightmap" );
	location					= data.GetVec2( "location", idVec2( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f ).ToString() );
	serverShot					= data.GetString( "mtr_serverShot", "levelshots/generic" );

	const idKeyValue* kv = NULL;
	while ( kv = data.MatchPrefix( "megatexture", kv ) ) {
		if ( kv->GetValue().Length() ) {
			megatextureMaterials.Append( kv->GetValue() );
		}
	}

	return true;
}

/*
================
sdDeclMapInfo::FreeData
================
*/
void sdDeclMapInfo::FreeData( void ) {
	megatextureMaterials.Clear();
	heightMap.Clear();
	data.Clear();
	location.Set( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f );
	serverShot = "levelshots/generic";
}

/*
============
sdDeclMapInfo::GetServerShot
============
*/
const idMaterial* sdDeclMapInfo::GetServerShot( void ) const {
	return gameLocal.declMaterialType.LocalFind(  serverShot.c_str() );
}
