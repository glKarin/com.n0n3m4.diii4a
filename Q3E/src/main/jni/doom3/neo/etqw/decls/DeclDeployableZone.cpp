// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "DeclDeployableZone.h"
#include "../structures/TeamManager.h" // Gordon: FIXME Move this
#include "../Game_local.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclDeployableZone

===============================================================================
*/

/*
================
sdDeclDeployableZone::sdDeclDeployableZone
================
*/
sdDeclDeployableZone::sdDeclDeployableZone( void ) {
	int count = sdTeamManager::GetInstance().GetNumTeams();
	teamInfo.SetNum( count );

	FreeData();
}

/*
================
sdDeclDeployableZone::~sdDeclDeployableZone
================
*/
sdDeclDeployableZone::~sdDeclDeployableZone( void ) {
}

/*
================
sdDeclDeployableZone::DefaultDefinition
================
*/
const char* sdDeclDeployableZone::DefaultDefinition( void ) const {
	return 
		"{\n"							\
		"}\n";
}

/*
================
sdDeclDeployableZone::ParseTeamInfo
================
*/
bool sdDeclDeployableZone::ParseTeamInfo( sdTeamInfo* team, idParser& src ) {
	src.ExpectTokenString( "{" );

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}


		if ( !token.Cmp( "}" ) ) {
			break;
		}

		const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType[ token ];
		if ( !object ) {
			src.Error( "sdDeclDeployableZone::Parse Invalid Deployable Object Type '%s'", token.c_str() );
			return false;
		}

		teamInfo[ team->GetIndex() ].Alloc() = object;
	}

	return true;
}

/*
================
sdDeclDeployableZone::Parse
================
*/
bool sdDeclDeployableZone::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	teamInfo.AssureSize( sdTeamManager::GetInstance().GetNumTeams() );

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}

		if ( !token.Cmp( "team" ) ) {
			if ( !src.ReadToken( &token ) ) {
				return false;
			}

			sdTeamInfo* team = &sdTeamManager::GetInstance().GetTeam( token );
			if ( !ParseTeamInfo( team, src ) ) {
				return false;
			}

			continue;
		} else if ( !token.Cmp( "}" ) ) {
			break;
		}
	}

	return true;
}

/*
================
sdDeclDeployableZone::FreeData
================
*/
void sdDeclDeployableZone::FreeData( void ) {
	teamInfo.Clear();
}

/*
================
sdDeclDeployableZone::CacheFromDict
================
*/
void sdDeclDeployableZone::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "dz", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declDeployableZoneType[ kv->GetValue() ];
		}
	}
}

/*
================
sdDeclDeployableZone::NumOptions
================
*/
int sdDeclDeployableZone::NumOptions( const sdTeamInfo* team ) const {
	if ( !team ) {
		return 0;
	}

	return teamInfo[ team->GetIndex() ].Num();
}

/*
================
sdDeclDeployableZone::GetDeployOption
================
*/
const sdDeclDeployableObject* sdDeclDeployableZone::GetDeployOption( const sdTeamInfo* team, int index ) const {
	if ( !team ) {
		return NULL;
	}

	if ( index < 0 || index >= teamInfo[ team->GetIndex() ].Num() ) {
		return NULL;
	}

	return teamInfo[ team->GetIndex() ][ index ];
}
