// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "../structures/TeamManager.h"

#include "../../decllib/declTypeHolder.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclPlayerTask

===============================================================================
*/

#include "DeclPlayerTask.h"

/*
================
sdDeclPlayerTask::sdDeclPlayerTask
================
*/
sdDeclPlayerTask::sdDeclPlayerTask( void ) {
	waypointData.SetGranularity( 1 );
	FreeData();
}

/*
================
sdDeclPlayerTask::~sdDeclPlayerTask
================
*/
sdDeclPlayerTask::~sdDeclPlayerTask( void ) {
}

/*
================
sdDeclPlayerTask::DefaultDefinition
================
*/
const char* sdDeclPlayerTask::DefaultDefinition( void ) const {
	return 
		"{\n"							\
		"}\n";
}

/*
================
sdDeclPlayerTask::ParseFromDict
================
*/
void sdDeclPlayerTask::ParseFromDict( const idDict& dict ) {

	eligibility.Load( dict, "require_eligible" );

	if ( GetState() == DS_PARSED ) {
		title			= declHolder.FindLocStr( dict.GetString( "title" ) );

		friendlyTitle	= declHolder.FindLocStr( dict.GetString( "title_friend" ), false );
		if( friendlyTitle == NULL ) {
			friendlyTitle = title;
		}
		completedTitle	= declHolder.FindLocStr( dict.GetString( "title_completed" ), false );
		if( completedTitle == NULL ) {
			completedTitle = title;
		}
		completedFriendlyTitle	= declHolder.FindLocStr( dict.GetString( "title_friend_completed" ), false );
		if( completedFriendlyTitle == NULL ) {
			completedFriendlyTitle = completedTitle;
		}
	} else {
		title					= declHolder.declLocStrType.LocalFind( "_default" );
		friendlyTitle			= title;
		completedTitle			= title;
		completedFriendlyTitle	= title;
	}

	team			= sdTeamManager::GetInstance().GetTeamSafe( dict.GetString( "team" ) );

	if ( dict.GetBool( "task" ) ) {
		type = PTT_TASK;
	} else if ( dict.GetBool( "objective" ) ) {
		type = PTT_OBJECTIVE;
	}

	noOcclusion = dict.GetBool( "los_check", "1" ) == 0;

	xpBonus			= dict.GetFloat( "xp_bonus" );
	xpString		= common->LocalizeText( dict.GetString( "xp_string", "game/tasks/empty" ) );
	priority		= dict.GetInt( "priority" );
	scriptObject	= dict.GetString( "scriptobject", "task" );
	showEligibleWayPoints = dict.GetBool( "show_eligible" );

	botTaskType		= dict.GetInt( "botTaskType", "-1" );

	float time		= dict.GetFloat( "time_limit" );
	if ( time <= 0 ) {
		timeLimit	= -1;
	} else {
		timeLimit	= SEC2MS( time );
	}
}

/*
================
sdDeclPlayerTask::Parse
================
*/
bool sdDeclPlayerTask::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "sdDeclPlayerTask::Parse Unexpected End of File" );
			return false;
		}

		if ( !token.Icmp( "data" ) ) {
			idDict dict;
			if ( !dict.Parse( src ) ) {
				src.Error( "sdDeclPlayerTask::Parse Error Reading Task Info Dictionary" );
				return false;
			}

			info.Copy( dict );			

		} else if ( !token.Icmp( "waypoint" ) ) {
			idDict data;			

			if ( !data.Parse( src ) ) {
				src.Error( "sdDeclPlayerTask::Parse Error Reading WayPoint Info Dictionary" );
				return false;
			}

			waypointData.Alloc() = data;
			waypointIcons.Alloc() = declHolder.declMaterialType.LocalFind( data.GetString( "mtr_icon", "" ), false );

		} else if ( !token.Cmp( "}" ) ) {
			break;
		} else {
			src.Error( "sdDeclPlayerTask::Parse Invalid Token '%s'", token.c_str() );
			return false;
		}
	}

	game->CacheDictionaryMedia( info );
	for ( int i = 0; i < waypointData.Num(); i++ ) {
		game->CacheDictionaryMedia( waypointData[ i ] );
	}

	ParseFromDict( info );

	return true;
}

/*
================
sdDeclPlayerTask::FreeData
================
*/
void sdDeclPlayerTask::FreeData( void ) {
	eligibility.Clear();
	waypointData.Clear();
	type			= PTT_MISSION;
	title			= NULL;
	friendlyTitle	= NULL;
	timeLimit		= -1;
	priority		= 0;
	scriptObject	= "task";
	info.Clear();
	showEligibleWayPoints = false;
	botTaskType		= -1;
}

/*
================
sdDeclPlayerTask::CacheFromDict
================
*/
void sdDeclPlayerTask::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "task", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declPlayerTaskType[ kv->GetValue() ];
		}
	}
}
