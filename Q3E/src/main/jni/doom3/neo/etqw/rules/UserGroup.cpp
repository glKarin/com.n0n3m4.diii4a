// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserGroup.h"
#include "../Player.h"
#include "../guis/UIList.h"

typedef sdPair< const char*, userPermissionFlags_t > permissionName_t;

permissionName_t permissionNames[] = {
	permissionName_t( "adminKick", PF_ADMIN_KICK ),
	permissionName_t( "adminBan", PF_ADMIN_BAN ),
	permissionName_t( "adminSetTeam", PF_ADMIN_SETTEAM ),
	permissionName_t( "adminChangeCampaign", PF_ADMIN_CHANGE_CAMPAIGN ),
	permissionName_t( "adminChangeMap", PF_ADMIN_CHANGE_MAP ),
	permissionName_t( "adminGlobalMute", PF_ADMIN_GLOBAL_MUTE ),
	permissionName_t( "adminGlobalVOIPMute", PF_ADMIN_GLOBAL_MUTE_VOIP ),
	permissionName_t( "adminPlayerMute", PF_ADMIN_PLAYER_MUTE ),
	permissionName_t( "adminPlayerVOIPMute", PF_ADMIN_PLAYER_MUTE_VOIP ),
	permissionName_t( "adminWarn", PF_ADMIN_WARN ),
	permissionName_t( "adminRestartMap", PF_ADMIN_RESTART_MAP ),
	permissionName_t( "adminRestartCampaign", PF_ADMIN_RESTART_CAMPAIGN ),
	permissionName_t( "adminStartMatch", PF_ADMIN_START_MATCH ),
	permissionName_t( "adminExecConfig", PF_ADMIN_EXEC_CONFIG ),
	permissionName_t( "adminShuffleTeams", PF_ADMIN_SHUFFLE_TEAMS ),
	permissionName_t( "adminAddBot", PF_ADMIN_ADD_BOT ),
	permissionName_t( "adminAdjustBots", PF_ADMIN_ADJUST_BOTS ),
	permissionName_t( "adminDisableProficiency", PF_ADMIN_DISABLE_PROFICIENCY ),
	permissionName_t( "adminSetTimeLimit", PF_ADMIN_SET_TIMELIMIT ),
	permissionName_t( "adminSetTeamDamage", PF_ADMIN_SET_TEAMDAMAGE ),
	permissionName_t( "adminSetTeamBalance", PF_ADMIN_SET_TEAMBALANCE ),
	permissionName_t( "noBan", PF_NO_BAN ),
	permissionName_t( "noKick", PF_NO_KICK ),
	permissionName_t( "noMute", PF_NO_MUTE ),
	permissionName_t( "noMuteVOIP", PF_NO_MUTE_VOIP ),
	permissionName_t( "noWarn", PF_NO_WARN ),
	permissionName_t( "quietLogin", PF_QUIET_LOGIN )
};

int numPermissionNames = _arraycount( permissionNames );

/*
===============================================================================

	sdUserGroup

===============================================================================
*/

/*
============
sdUserGroup::sdUserGroup
============
*/
sdUserGroup::sdUserGroup( void ) : superUser( false ), voteLevel( -1 ), needsLogin( false ) {	
}

/*
============
sdUserGroup::Write
============
*/
void sdUserGroup::Write( idBitMsg& msg ) const {
	msg.WriteString( GetName() );

	for ( int i = 0; i < flags.Size(); i++ ) {
		msg.WriteLong( flags.GetDirect()[ i ] );
	}

	msg.WriteLong( controlGroups.Num() );
	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		msg.WriteLong( controlGroups[ i ].second );
	}

	msg.WriteLong( voteLevel );
	msg.WriteBool( needsLogin );
}

/*
============
sdUserGroup::Read
============
*/
void sdUserGroup::Read( const idBitMsg& msg ) {
	char buffer[ 512 ];
	msg.ReadString( buffer, sizeof( buffer ) );
	SetName( buffer );

	for ( int i = 0; i < flags.Size(); i++ ) {
		flags.GetDirect()[ i ] = msg.ReadLong();
	}

	controlGroups.SetNum( msg.ReadLong() );
	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		controlGroups[ i ].second = msg.ReadLong();
	}

	voteLevel = msg.ReadLong();
	needsLogin = msg.ReadBool();
}

/*
============
sdUserGroup::Write
============
*/
void sdUserGroup::Write( idFile* file ) const {
	file->WriteString( GetName() );

	for ( int i = 0; i < flags.Size(); i++ ) {
		file->WriteInt( flags.GetDirect()[ i ] );
	}

	file->WriteInt( controlGroups.Num() );
	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		file->WriteInt( controlGroups[ i ].second );
	}

	file->WriteInt( voteLevel );
	file->WriteBool( needsLogin );
}

/*
============
sdUserGroup::Read
============
*/
void sdUserGroup::Read( idFile* file ) {
	file->ReadString( name );

	for ( int i = 0; i < flags.Size(); i++ ) {
		file->ReadInt( flags.GetDirect()[ i ] );
	}

	int count;
	file->ReadInt( count );
	controlGroups.SetNum( count );
	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		file->ReadInt( controlGroups[ i ].second );
	}

	file->ReadInt( voteLevel );
	file->ReadBool( needsLogin );
}

/*
============
sdUserGroup::ParseControl
============
*/
bool sdUserGroup::ParseControl( idLexer& src ) {
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Unexpected end of file" );
			return false;
		}

		if ( token == "}" ) {
			break;
		}

		sdPair< idStr, int >& control = controlGroups.Alloc();

		control.first	= token;
		control.second	= -1;
	}

	return true;
}

/*
============
sdUserGroup::Parse
============
*/
bool sdUserGroup::Parse( idLexer& src ) {
	idToken token;

	if ( !src.ReadToken( &token ) ) {
		src.Warning( "No Name Specified" );
		return false;
	}

	SetName( token );

	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Unexpected end of file" );
			return false;
		}

		if ( token == "}" ) {
			break;
		}

		if ( token.Icmp( "password" ) == 0 ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Unexpected end of file" );
				return false;
			}

			// Gordon: The example file will have password set to password, this should never be a valid password
			if ( token.Icmp( "password" ) != 0 ) {
				SetPassword( token );
			}
			continue;
		}

		if ( token.Icmp( "control" ) == 0 ) {
			if ( !ParseControl( src ) ) {
				src.Warning( "Failed to parse control groups" );
				return false;
			}
			continue;
		}

		if ( token.Icmp( "voteLevel" ) == 0 ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Failed to parse voteLevel" );
				return false;
			}

			voteLevel = token.GetIntValue();
			continue;
		}

		int i;
		for ( i = 0; i < numPermissionNames; i++ ) {
			if ( idStr::Icmp( permissionNames[ i ].first, token ) ) {
				continue;
			}

			GivePermission( permissionNames[ i ].second );
			break;
		}
		if ( i != numPermissionNames ) {
			continue;
		}

		src.Warning( "Unexpected token '%s'", token.c_str() );
		return false;
	}

	return true;
}

/*
============
sdUserGroup::Parse
============
*/
void sdUserGroup::PostParseFixup( void ) {
	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();

	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		sdPair< idStr, int >& control = controlGroups[ i ];

		control.second = groupManager.FindGroup( control.first );
		if ( control.second == -1 ) {
			gameLocal.Warning( "sdUserGroup::PostParseFixup Control Group '%s' not found", control.first.c_str() );
		}
	}
}

/*
============
sdUserGroup::CanControl
============
*/
bool sdUserGroup::CanControl( const sdUserGroup& group ) const {
	if ( superUser ) {
		return true;
	}

	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();
	for ( int i = 0; i < controlGroups.Num(); i++ ) {
		int index = controlGroups[ i ].second;
		if ( index == -1 ) {
			continue;
		}

		if ( &groupManager.GetGroup( index ) == &group ) {
			return true;
		}
	}

	return false;
}

/*
===============================================================================

	sdUserGroupManagerLocal

===============================================================================
*/

/*
============
sdUserGroupManagerLocal::sdUserGroupManagerLocal
============
*/
sdUserGroupManagerLocal::sdUserGroupManagerLocal( void ) {
	consoleGroup.SetName( "<CONSOLE>" ); // users coming from the console will use this user group which has full permissions
	consoleGroup.MakeSuperUser();

	defaultGroup = -1;
}

/*
============
sdUserGroupManagerLocal::sdUserGroupManagerLocal
============
*/
sdUserGroupManagerLocal::~sdUserGroupManagerLocal( void ) {
}

/*
============
sdUserGroupManagerLocal::Init
============
*/
void sdUserGroupManagerLocal::Init( void ) {
	idStr groupNames[ MAX_CLIENTS ];
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		nextLoginTime[ i ] = 0;

		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		groupNames[ i ] = GetGroup( player->GetUserGroup() ).GetName();
	}

	userGroups.Clear();
	userGroups.Alloc().SetName( "<DEFAULT>" );

	configs.Clear();
	votes.Clear();

	// load user groups
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );

	src.LoadFile( "usergroups.dat" );

	if ( !src.IsLoaded() ) {
		return;
	}

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "group" ) ) {
			sdUserGroup& group = userGroups.Alloc();
			if ( !group.Parse( src ) ) {
				src.Warning( "Error Parsing Group" );
				break;
			}
		} else if ( !token.Icmp( "configs" ) ) {
			if ( !configs.Parse( src ) ) {
				src.Warning( "Error Parsing configs" );
				break;
			}
		} else if ( !token.Icmp( "votes" ) ) {
			if ( !votes.Parse( src ) ) {
				src.Warning( "Error Parsing votes" );
				break;
			}
		} else {
			src.Warning( "Invalid Token '%s'", token.c_str() );
			break;
		}
	}

	defaultGroup = FindGroup( "default" );
	if ( defaultGroup == -1 ) {
		defaultGroup = 0;
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		player->SetUserGroup( FindGroup( groupNames[ i ] ) );
	}

	for ( int i = 0; i < userGroups.Num(); i++ ) {
		userGroups[ i ].PostParseFixup();
	}

	if ( gameLocal.isServer ) {
		WriteNetworkData( sdReliableMessageClientInfoAll() );
	}
}

/*
============
sdUserGroupManagerLocal::FindGroup
============
*/
int sdUserGroupManagerLocal::FindGroup( const char* name ) {
	for ( int i = 0; i < userGroups.Num(); i++ ) {
		if ( idStr::Icmp( userGroups[ i ].GetName(), name ) ) {
			continue;
		}

		return i;
	}
	return -1;
}

/*
============
sdUserGroupManagerLocal::Login
============
*/
bool sdUserGroupManagerLocal::Login( idPlayer* player, const char* password ) {
	int now = MS2SEC( sys->Milliseconds() );
	if ( now < nextLoginTime[ player->entityNumber ] ) {
		return false;
	}
	nextLoginTime[ player->entityNumber ] = now + 5; // Gordon: limit password brute forcing

	for ( int i = 0; i < userGroups.Num(); i++ ) {
		if ( !userGroups[ i ].CheckPassword( password ) ) {
			continue;
		}

		player->SetUserGroup( i );
		return true;
	}

	return false;
}


/*
============
sdUserGroupManagerLocal::CanLogin
============
*/
bool sdUserGroupManagerLocal::CanLogin() const {
	for ( int i = 0; i < userGroups.Num(); i++ ) {
		if( userGroups[ i ].HasPassword() ) {
			return true;
		}
	}
	return false;
}

/*
============
sdUserGroupManagerLocal::ReadNetworkData
============
*/
void sdUserGroupManagerLocal::ReadNetworkData( const idBitMsg& msg ) {
	userGroups.Clear();
	userGroups.Alloc().SetName( "<DEFAULT>" );

	int count = msg.ReadLong();
	for ( int i = 0; i < count; i++ ) {
		sdUserGroup& group = userGroups.Alloc();
		group.Read( msg );
	}

	msg.ReadDeltaDict( configs, NULL );
	msg.ReadDeltaDict( votes, NULL );

	defaultGroup = FindGroup( "default" );
	if ( defaultGroup == -1 ) {
		defaultGroup = 0;
	}
}

/*
============
sdUserGroupManagerLocal::WriteNetworkData
============
*/
void sdUserGroupManagerLocal::WriteNetworkData( const sdReliableMessageClientInfoBase& target ) {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_USERGROUPS );

	msg.WriteLong( userGroups.Num() - 1 );
	for ( int i = 1; i < userGroups.Num(); i++ ) {
		sdUserGroup& group = userGroups[ i ];
		group.Write( msg );
	}

	msg.WriteDeltaDict( configs, NULL );
	msg.WriteDeltaDict( votes, NULL );

	msg.Send( target );
}

/*
============
sdUserGroupManagerLocal::CreateUserGroupList
============
*/
void sdUserGroupManagerLocal::CreateUserGroupList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer ) {
		return;
	}

	const sdUserGroup& localGroup = gameLocal.isClient ? GetGroup( localPlayer->GetUserGroup() ) : GetConsoleGroup();

	int index = 0;
	for ( int i = 1; i < userGroups.Num(); i++ ) {
		if ( !localGroup.CanControl( userGroups[ i ] ) ) {
			continue;
		}

		sdUIList::InsertItem( list, va( L"%hs", userGroups[ i ].GetName() ), index, 0 );
		index++;
	}
	
	/*
	for ( int i = 1; i < userGroups.Num(); i++ ) {
		sdUIList::InsertItem( list, va( L"%hs", userGroups[ i ].GetName() ), i - 1, 0 );
	}
	*/
}

/*
============
sdUserGroupManagerLocal::CreateServerConfigList
============
*/
void sdUserGroupManagerLocal::CreateServerConfigList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	for ( int i = 0; i < configs.GetNumKeyVals(); i++ ) {
		const idKeyValue* kv = configs.GetKeyVal( i );
		sdUIList::InsertItem( list, va( L"%hs", kv->GetKey().c_str() ), i, 0 );
	}
}

/*
============
sdUserGroupManagerLocal::Write
============
*/
void sdUserGroupManagerLocal::Write( idFile* file ) const {
	file->WriteInt( userGroups.Num() - 1 );
	for ( int i = 1; i < userGroups.Num(); i++ ) {
		const sdUserGroup& group = userGroups[ i ];
		group.Write( file );
	}

	configs.WriteToFileHandle( file );
	votes.WriteToFileHandle( file );
}

/*
============
sdUserGroupManagerLocal::Read
============
*/
void sdUserGroupManagerLocal::Read( idFile* file ) {
	userGroups.Clear();
	userGroups.Alloc().SetName( "<DEFAULT>" );

	int count;
	file->ReadInt( count );
	for ( int i = 0; i < count; i++ ) {
		sdUserGroup& group = userGroups.Alloc();
		group.Read( file );
	}

	configs.ReadFromFileHandle( file );
	votes.ReadFromFileHandle( file );

	defaultGroup = FindGroup( "default" );
	if ( defaultGroup == -1 ) {
		defaultGroup = 0;
	}
}

/*
============
sdUserGroupManagerLocal::GetVoteLevel
============
*/
int sdUserGroupManagerLocal::GetVoteLevel( const char* voteName ) {
	return votes.GetInt( voteName, "-1" );
}
