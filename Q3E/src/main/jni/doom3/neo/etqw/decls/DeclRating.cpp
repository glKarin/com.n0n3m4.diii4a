// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclRating.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclRating

===============================================================================
*/

/*
================
sdDeclRating::sdDeclRating
================
*/
sdDeclRating::sdDeclRating( void ) {
}

/*
================
sdDeclRating::~sdDeclRating
================
*/
sdDeclRating::~sdDeclRating( void ) {
}

/*
================
sdDeclRating::DefaultDefinition
================
*/
const char* sdDeclRating::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"}\n";
}

/*
================
sdDeclRating::Parse
================
*/
bool sdDeclRating::Parse( const char *text, const int textLength ) {
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
				src.Error( "sdDeclRating::Parse Invalid Parm For 'title'" );
				return false;
			}

			title = declHolder.declLocStrType.LocalFind( token.c_str() );

		} else if( !token.Icmp( "shortTitle" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclRating::Parse Invalid Parm For 'shortTitle'" );
				return false;
			}

			shortTitle = declHolder.declLocStrType.LocalFind( token.c_str() );

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclRating::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclRating::FreeData
================
*/
void sdDeclRating::FreeData( void ) {
	title = declHolder.declLocStrType[ "_default" ];
	shortTitle = declHolder.declLocStrType[ "_default" ];
}
