// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclDamageFilter.h"
#include "../Game_local.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclDamageFilter

===============================================================================
*/

/*
================
sdDeclDamageFilter::sdDeclDamageFilter
================
*/
sdDeclDamageFilter::sdDeclDamageFilter( void ) {
}

/*
================
sdDeclDamageFilter::~sdDeclDamageFilter
================
*/
sdDeclDamageFilter::~sdDeclDamageFilter( void ) {
}

/*
================
sdDeclDamageFilter::DefaultDefinition
================
*/
const char* sdDeclDamageFilter::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"}\n";
}

/*
================
sdDeclDamageFilter::ParseFilter
================
*/
bool sdDeclDamageFilter::ParseFilter( damageFilter_t& filter, idParser& src ) {
	idToken token;

	if( !src.ReadToken( &token ) || token.Cmp( "{" ) ) {
		return false;
	}

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		if( !token.Icmp( "damage" ) ) {

			bool error;
			filter.damage = src.ParseFloat( &error );
			if ( error ) {
				src.Error( "sdDeclDamageFilter::ParseLevel Invalid Parm for 'damage'" );
				return false;
			}

			if ( src.PeekTokenString( "%" ) ) {
				src.ReadToken( &token );
				filter.mode = DFM_PERCENT;
			} else {
				filter.mode = DFM_NORMAL;
			}

		} else if( !token.Icmp( "target" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclDamageFilter::ParseLevel Missing Parm for 'target'" );
				return false;
			}

			filter.target = gameLocal.declTargetInfoType.LocalFind( token, false );
			if ( !filter.target ) {
				src.Error( "sdDeclDamageFilter::ParseLevel Invalid Target '%s'", token.c_str() );
				return false;
			}

		} else if( !token.Icmp( "noScale" ) ) {

			filter.noScale = true;

		} else {

			src.Error( "sdDeclDamageFilter::ParseLevel Unknown Parameter %s", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
================
sdDeclDamageFilter::Parse
================
*/
bool sdDeclDamageFilter::Parse( const char *text, const int textLength ) {
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

		if( !token.Icmp( "type" ) ) {

			damageFilter_t& filter		= filters.Alloc();
			filter.target				= NULL;
			filter.damage				= 0.f;
			filter.mode					= DFM_NORMAL;
			filter.noScale				= false;

			if ( !ParseFilter( filter, src ) ) {
				src.Error( "sdDeclDamageFilter::Parse Error Parsing Filter %i", filters.Num() );
				return false;
			}

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclDamageFilter::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclDamageFilter::FreeData
================
*/
void sdDeclDamageFilter::FreeData( void ) {
	filters.Clear();
}
