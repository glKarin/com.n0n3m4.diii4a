// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclProficiencyType.h"
#include "../Game_local.h"
#include "../proficiency/StatsTracker.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclProficiencyType

===============================================================================
*/

/*
================
sdDeclProficiencyType::sdDeclProficiencyType
================
*/
sdDeclProficiencyType::sdDeclProficiencyType( void ) {
	FreeData();
}

/*
================
sdDeclProficiencyType::~sdDeclProficiencyType
================
*/
sdDeclProficiencyType::~sdDeclProficiencyType( void ) {
}

/*
================
sdDeclProficiencyType::DefaultDefinition
================
*/
const char* sdDeclProficiencyType::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclProficiencyType::Parse
================
*/
bool sdDeclProficiencyType::Parse( const char *text, const int textLength ) {
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

		} else if( !token.Icmp( "name" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclProficiencyType::Parse Invalid Parm For 'name'" );
				return false;
			}

			title = token;

		} else if( !token.Icmp( "stat_name" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclProficiencyType::Parse Invalid Parm For 'stat_name'" );
				return false;
			}

			stats.name	= token;

			sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

			stats.xp		= tracker.GetStat( tracker.AllocStat( va( "%s_xp", stats.name.c_str() ), sdNetStatKeyValue::SVT_FLOAT ) );
			stats.totalXP	= tracker.GetStat( tracker.AllocStat( "total_xp", sdNetStatKeyValue::SVT_FLOAT ) );

		} else if( !token.Icmp( "level" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclProficiencyType::Parse Error Parsing Level %i", levels.Num() );
				return false;
			}

			levels.Alloc() = token.GetIntValue();

		} else if( !token.Icmp( "text" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclProficiencyType::Parse Error Parsing 'text'" );
				return false;
			}

			this->text = declHolder.FindLocStr( token );

		} else {

			src.Error( "sdDeclProficiencyType::Parse Invalid Token %s", token.c_str() );

		}
	}

	return true;
}

/*
================
sdDeclProficiencyType::FreeData
================
*/
void sdDeclProficiencyType::FreeData( void ) {
	text = NULL;
	title.Clear();
	levels.Clear();

	stats.name.Clear();
	stats.xp		= NULL;
	stats.totalXP	= NULL;
}
