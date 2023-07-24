// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclCampaign.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclCampaign

===============================================================================
*/

/*
================
sdDeclCampaign::sdDeclCampaign
================
*/
sdDeclCampaign::sdDeclCampaign( void ) {
}

/*
================
sdDeclCampaign::~sdDeclCampaign
================
*/
sdDeclCampaign::~sdDeclCampaign( void ) {
}


/*
================
sdDeclCampaign::DefaultDefinition
================
*/
const char* sdDeclCampaign::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclCampaign::Parse
================
*/
bool sdDeclCampaign::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );	

	src.SkipUntilString( "{", &token );

	idDict dict;

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Icmp( "map" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				return false;
			}

			maps.Alloc() = token;

		} else if ( !token.Icmp( "data" ) ) {

			if ( !dict.Parse( src ) ) {
				return false;
			}

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclInvItem::Parse Invalid Token %s", token.c_str() );

		}
	}

	backdrop = dict.GetString( "mtr_backdrop", "guis/assets/black" ) ;

	return true;
}

/*
================
sdDeclCampaign::FreeData
================
*/
void sdDeclCampaign::FreeData( void ) {
	maps.Clear();
	backdrop = "guis/assets/black";
}

/*
============
sdDeclCampaign::GetBackdrop
============
*/
const idMaterial* sdDeclCampaign::GetBackdrop( void ) const {
	return declHolder.FindMaterial( backdrop.c_str() );
}
