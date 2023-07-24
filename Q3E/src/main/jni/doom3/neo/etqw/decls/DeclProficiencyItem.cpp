// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclProficiencyItem.h"
#include "../Game_local.h"
#include "../proficiency/StatsTracker.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclProficiencyItem

===============================================================================
*/

/*
================
sdDeclProficiencyItem::sdDeclProficiencyItem
================
*/
sdDeclProficiencyItem::sdDeclProficiencyItem( void ) {
}

/*
================
sdDeclProficiencyItem::~sdDeclProficiencyItem
================
*/
sdDeclProficiencyItem::~sdDeclProficiencyItem( void ) {
}

/*
================
sdDeclProficiencyType::DefaultDefinition
================
*/
const char* sdDeclProficiencyItem::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclProficiencyItem::Parse
================
*/
bool sdDeclProficiencyItem::Parse( const char *text, const int textLength ) {
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

		} else if( !token.Icmp( "type" ) ) {

			if( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclProficiencyItem::Parse Missing Parm For 'type'" );
				return false;
			}

			type = gameLocal.declProficiencyTypeType.LocalFind( token, false );
			if ( type == NULL ) {
				src.Error( "sdDeclProficiencyItem::Parse Invalid Parm '%s' For 'type'", token.c_str() );
				return false;
			}

		} else if( !token.Icmp( "count" ) ) {

			count = src.ParseFloat();

		} else if( !token.Icmp( "stat_name" ) ) {

			if( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclProficiencyItem::Parse Missing Parm For 'stat_name'" );
				return false;
			}

			sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

			stat = tracker.GetStat( tracker.AllocStat( token.c_str(), sdNetStatKeyValue::SVT_FLOAT ) );

		} else {

			src.Error( "sdDeclProficiencyItem::Parse Invalid Token %s", token.c_str() );

		}
	}

	return true;
}

/*
================
sdDeclProficiencyItem::FreeData
================
*/
void sdDeclProficiencyItem::FreeData( void ) {
	type	= NULL;
	count	= 0;
	stat	= NULL;
}
