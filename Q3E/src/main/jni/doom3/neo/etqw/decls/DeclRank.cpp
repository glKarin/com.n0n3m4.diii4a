// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclRank.h"

#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclRank

===============================================================================
*/

/*
================
sdDeclRank::sdDeclRank
================
*/
sdDeclRank::sdDeclRank( void ) {
}

/*
================
sdDeclRank::~sdDeclRank
================
*/
sdDeclRank::~sdDeclRank( void ) {
}

/*
================
sdDeclRank::DefaultDefinition
================
*/
const char* sdDeclRank::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"}\n";
}

/*
================
sdDeclRank::Parse
================
*/
bool sdDeclRank::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if( !token.Icmp( "title" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclRank::Parse Invalid Parm For 'title'" );
				return false;
			}

			title = declHolder.FindLocStr( token.c_str() );

			if ( title->GetState() == DS_DEFAULTED ) {
				src.Warning( "Defaulted string for rank title" );
			}

		} else if( !token.Icmp( "shortTitle" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclRank::Parse Invalid Parm For 'shortTitle'" );
				return false;
			}

			shortTitle = declHolder.FindLocStr( token.c_str() );

		} else if( !token.Icmp( "cost" ) ) {

			bool error;
			cost = src.ParseFloat( &error );

			if( error ) {
				src.Error( "sdDeclRank::Parse Invalid Parm For 'cost'" );
				return false;
			}

		} else if( !token.Icmp( "level" ) ) {

			level = src.ParseInt();

		} else if( !token.Icmp( "material" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclRank::Parse Invalid Parm For 'material'" );
				return false;
			}

			material = token;

		}else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclRank::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclRank::FreeData
================
*/
void sdDeclRank::FreeData( void ) {
	cost = 0;
	level = -1;
	title = declHolder.declLocStrType[ "_default" ];
	shortTitle = declHolder.declLocStrType[ "_default" ];
}
