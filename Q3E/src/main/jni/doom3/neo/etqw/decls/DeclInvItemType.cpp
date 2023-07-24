// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclInvItemType.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclInvItemType

===============================================================================
*/

/*
================
sdDeclInvItemType::sdDeclInvItemType
================
*/
sdDeclInvItemType::sdDeclInvItemType( void ) {
}

/*
================
sdDeclInvItemType::~sdDeclInvItemType
================
*/
sdDeclInvItemType::~sdDeclInvItemType( void ) {
}

/*
================
sdDeclInvItemType::DefaultDefinition
================
*/
const char* sdDeclInvItemType::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclInvItemType::Parse
================
*/
bool sdDeclInvItemType::Parse( const char *text, const int textLength ) {
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

		if( !token.Icmp( "equipable" ) ) {

			flags.equipable = true;

		} else if( !token.Icmp( "vehicleEquipable" ) ) {

			flags.vehicleEquipable = true;

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclInvItemType::Parse Invalid Token %s", token.c_str() );

		}
	}

	return true;
}

/*
================
sdDeclInvItemType::FreeData
================
*/
void sdDeclInvItemType::FreeData( void ) {
	memset( &flags, 0, sizeof( flags ) );
}
