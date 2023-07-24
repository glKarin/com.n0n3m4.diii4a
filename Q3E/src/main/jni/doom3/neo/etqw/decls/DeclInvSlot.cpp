// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclInvSlot.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclInvSlot

===============================================================================
*/

/*
================
sdDeclInvSlot::sdDeclInvSlot
================
*/
sdDeclInvSlot::sdDeclInvSlot( void ) {
	FreeData();
}

/*
================
sdDeclInvSlot::~sdDeclInvSlot
================
*/
sdDeclInvSlot::~sdDeclInvSlot( void ) {
	FreeData();
}

/*
================
sdDeclInvSlot::DefaultDefinition
================
*/
const char* sdDeclInvSlot::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclInvSlot::Parse
================
*/
bool sdDeclInvSlot::Parse( const char *text, const int textLength ) {
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

		if( !token.Icmp( "bank" ) ) {

			bank = src.ParseInt() - 1;

			if( bank < 0 || bank > 10 ) {
				src.Warning( "sdDeclInvSlot::Parse Invalid Parms for bank" );
				return false;
			}

		} else if( !token.Icmp( "title" ) ) {
			if( !src.ReadToken( &token )) {
				src.Error( "sdDeclInvSlot::Parse: Unexpected end of file while parsing \"title\"" );
			}
			title = token;

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclInvSlot::Parse Invalid Token %s", token.c_str() );

		}
	}

	return true;
}

/*
================
sdDeclInvSlot::FreeData
================
*/
void sdDeclInvSlot::FreeData( void ) {
	bank			= -1;
	title			= "";
}
