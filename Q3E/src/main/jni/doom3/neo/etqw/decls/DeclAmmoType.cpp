// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclAmmoType.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclAmmoType

===============================================================================
*/

/*
================
sdDeclAmmoType::sdDeclAmmoType
================
*/
sdDeclAmmoType::sdDeclAmmoType( void ) {
}

/*
================
sdDeclAmmoType::~sdDeclAmmoType
================
*/
sdDeclAmmoType::~sdDeclAmmoType( void ) {
}

/*
================
sdDeclAmmoType::DefaultDefinition
================
*/
const char* sdDeclAmmoType::DefaultDefinition( void ) const {
	return 
		"{\n"							\
		"}\n";
}

/*
================
sdDeclAmmoType::Parse
================
*/
bool sdDeclAmmoType::Parse( const char *text, const int textLength ) {
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

		if( !token.Cmp( "}" ) ) {
			break;
		}
	}

	return true;
}

/*
================
sdDeclAmmoType::FreeData
================
*/
void sdDeclAmmoType::FreeData( void ) {
}

/*
================
sdDeclAmmoType::GetType
================
*/
ammoType_t sdDeclAmmoType::GetAmmoType( void ) const {
	return static_cast< ammoType_t >( base->Index() );
}
