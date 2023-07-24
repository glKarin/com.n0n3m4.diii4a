// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "AdminSystem.h"
#include "../Player.h"
#include "../../idlib/PropertiesImpl.h"
#include "../guis/UIList.h"
#include "GameRules.h"
#include "GameRules_Campaign.h"
#include "GameRules_Objective.h"
#include "GameRules_StopWatch.h"

#include "../proficiency/StatsTracker.h"

#include "../botai/Bot.h"
#include "../botai/BotThreadData.h"

/*
===============================================================================

	sdAdminSystemCommand

===============================================================================
*/

/*
============
sdAdminSystemCommand::CommandCompletion
============
*/
void sdAdminSystemCommand::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
============
sdAdminSystemCommand::Print
============
*/
void sdAdminSystemCommand::Print( idPlayer* player, const char* locStr, const idWStrList& list ) const {
	if ( player != NULL ) {
		player->SendLocalisedMessage( declHolder.declLocStrType[ locStr ], list );
	} else {
		gameLocal.Printf( va( "%ls\n", common->LocalizeText( locStr, list ).c_str() ) );
	}
}

/*
===============================================================================

	sdAdminSystemCommand_Kick

===============================================================================
*/

/*
============
sdAdminSystemCommand_Kick::PerformCommand
============
*/
bool sdAdminSystemCommand_Kick::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_KICK ) ) {
		Print( player, "guis/admin/system/nopermkickplayer" );
		return false;
	}

	const char* clientName = cmd.Argv( 2 );

	idPlayer* kickPlayer = gameLocal.GetClientByName( clientName );
	if ( !kickPlayer ) {
		gameLocal.Printf( "Client '%s' not found\n", clientName );
		return false;
	}
	
	const sdUserGroup& kickGroup = sdUserGroupManager::GetInstance().GetGroup( kickPlayer->GetUserGroup() );
	if ( kickGroup.HasPermission( PF_NO_BAN ) || gameLocal.IsLocalPlayer( kickPlayer ) ) {
		Print( player, "guis/admin/system/cannotkickplayer" );
		return false;
	}

	clientNetworkAddress_t netAddr;
	networkSystem->ServerGetClientNetworkInfo( kickPlayer->entityNumber, netAddr );

	clientGUIDLookup_t lookup;
	lookup.ip	= *( ( int* )netAddr.ip );
	lookup.pbid	= 0;
	lookup.name = kickPlayer->userInfo.rawName;

	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
		networkSystem->ServerGetClientNetId( kickPlayer->entityNumber, lookup.clientId );
	}

	sdPlayerStatEntry* stat = sdGlobalStatsTracker::GetInstance().GetStat( sdGlobalStatsTracker::GetInstance().AllocStat( "times_kicked", sdNetStatKeyValue::SVT_INT ) );
	stat->IncreaseValue( kickPlayer->entityNumber, 1 );

	gameLocal.guidFile.BanUser( lookup, ( g_kickBanLength.GetFloat() * 60 ) );

	networkSystem->ServerKickClient( kickPlayer->entityNumber, "engine/disconnect/kicked", true );
	return true;
}

/*
============
sdAdminSystemCommand_Kick::CommandCompletion
============
*/
void sdAdminSystemCommand_Kick::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdAdminSystemCommand_KickAllBots

===============================================================================
*/

/*
============
sdAdminSystemCommand_KickAllBots::PerformCommand
============
*/
bool sdAdminSystemCommand_KickAllBots::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_KICK ) ) {
		Print( player, "guis/admin/system/nopermkickplayer" );
		return false;
	}

	int count = DoKick();
	if ( count > 0 ) {
		gameLocal.Printf( "Server kicked %i bot(s)\n", count );
		return true;
	}

	gameLocal.Printf( "There are no bots on the server to kick!\n" );
	return false;
}

/*
============
sdAdminSystemCommand_KickAllBots::DoKick
============
*/
int sdAdminSystemCommand_KickAllBots::DoKick( void ) {
	int count = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient ( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->IsType( idBot::Type ) ) {
			networkSystem->ServerKickClient( i, "engine/disconnect/kicked", true );
			count++;
		}
	}
	return count;
}

/*
===============================================================================

	sdAdminSystemOnOffCommand

===============================================================================
*/

/*
============
sdAdminSystemOnOffCommand::PerformCommand
============
*/
bool sdAdminSystemOnOffCommand::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( GetRequiredPersmission() ) ) {
		Print( player, GetPermissionFailedMessage() );
		return false;
	}

	const char* value = cmd.Argv( 2 );
	if ( !*value ) {
		Print( player, "guis/admin/system/specifyonoff" );
		return false;
	}

	if ( !idStr::Icmp( value, "on" ) ) {
		OnCompleted( player, true );
		return true;
	}
	
	if ( !idStr::Icmp( value, "off" ) ) {
		OnCompleted( player, false );
		return true;
	}

	Print( player, "guis/admin/system/specifyonoff" );
	return false;
}

/*
===============================================================================

	sdAdminSystemCommand_SetTimeLimit

===============================================================================
*/

/*
============
sdAdminSystemCommand_SetTimeLimit::PerformCommand
============
*/
bool sdAdminSystemCommand_SetTimeLimit::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_SET_TIMELIMIT ) ) {
		Print( player, "guis/admin/system/nopermsettimelimit" );
		return false;
	}

	int time;
	if ( !sdProperties::sdFromString( time, cmd.Argv( 2 ) ) ) {
		Print( player, "guis/admin/system/specifytime" );
		return false;
	}

	if ( time < 0 ) {
		time = 0;
	}

	si_timeLimit.SetInteger( time );
	return true;
}

/*
============
sdAdminSystemCommand_SetTimeLimit::CommandCompletion
============
*/
void sdAdminSystemCommand_SetTimeLimit::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	int values[] = { 5, 10, 15, 20, 25, 30 };
	int count = _arraycount( values );
	for ( int i = 0; i < count; i++ ) {
		callback( va( "%s %s %d", args.Argv( 0 ), args.Argv( 1 ), values[ i ] ) );
	}
}


/*
===============================================================================

	sdAdminSystemCommand_Login

===============================================================================
*/

/*
============
sdAdminSystemCommand_Login::PerformCommand
============
*/
bool sdAdminSystemCommand_Login::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( player == NULL ) {
		player = gameLocal.GetLocalPlayer();
	}

	if ( player == NULL ) {
		gameLocal.Printf( "Server Cannot Log In\n" );
		return false;
	}

	return sdUserGroupManager::GetInstance().Login( player, cmd.Argv( 2 ) );
}

/*
===============================================================================

	sdAdminSystemCommand_Ban

===============================================================================
*/

/*
============
sdAdminSystemCommand_Ban::PerformCommand
============
*/
bool sdAdminSystemCommand_Ban::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_BAN ) ) {
		Print( player, "guis/admin/system/cannotban" );
		return false;
	}

	const char* playerName = cmd.Argv( 2 );
	idPlayer* banPlayer = gameLocal.GetClientByName( playerName );
	if ( !banPlayer ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	if( banPlayer->IsType( idBot::Type ) ) {
		Print( player, "guis/admin/system/cannotbanplayer" );
		return false;
	}

	const sdUserGroup& banGroup = sdUserGroupManager::GetInstance().GetGroup( banPlayer->GetUserGroup() );
	if ( banGroup.HasPermission( PF_NO_BAN ) || gameLocal.IsLocalPlayer( banPlayer ) ) {
		Print( player, "guis/admin/system/cannotbanplayer" );
		return false;
	}

	int length;
	if ( cmd.Argc() >= 4 ) {
		sdProperties::sdFromString( length, cmd.Argv( 3 ) );
		length *= 60;
	} else {
		length = -1;
	}

	clientNetworkAddress_t netAddr;
	networkSystem->ServerGetClientNetworkInfo( banPlayer->entityNumber, netAddr );

	clientGUIDLookup_t lookup;
	lookup.ip	= *( ( int* )netAddr.ip );
	lookup.pbid	= 0;
	lookup.name = banPlayer->userInfo.rawName;

	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
		networkSystem->ServerGetClientNetId( banPlayer->entityNumber, lookup.clientId );
	}

	sdPlayerStatEntry* stat = sdGlobalStatsTracker::GetInstance().GetStat( sdGlobalStatsTracker::GetInstance().AllocStat( "times_banned", sdNetStatKeyValue::SVT_INT ) );
	stat->IncreaseValue( banPlayer->entityNumber, 1 );

	gameLocal.guidFile.BanUser( lookup, length );

	networkSystem->ServerKickClient( banPlayer->entityNumber, "engine/disconnect/banned", true );
	return true;
}

/*
============
sdAdminSystemCommand_Ban::CommandCompletion
============
*/
void sdAdminSystemCommand_Ban::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* clientName = args.Argv( 2 );
	int len = idStr::Length( clientName );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}	
}

/*
===============================================================================

	sdAdminSystemCommand_ListBans

===============================================================================
*/

/*
============
sdAdminSystemCommand_ListBans::PerformCommand
============
*/
bool sdAdminSystemCommand_ListBans::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_BAN ) ) {
		Print( player, "guis/admin/system/cannotban" );
		return false;
	}

	gameLocal.StartSendingBanList( player );
	return true;
}

/*
===============================================================================

	sdAdminSystemCommand_UnBan

===============================================================================
*/

/*
============
sdAdminSystemCommand_UnBan::PerformCommand
============
*/
bool sdAdminSystemCommand_UnBan::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_BAN ) ) {
		Print( player, "guis/admin/system/cannotban" );
		return false;
	}

	int banIndex = atoi( cmd.Argv( 2 ) );
	gameLocal.guidFile.UnBan( banIndex );
	return true;
}

/*
===============================================================================

	sdAdminSystemCommand_SetTeam

===============================================================================
*/

/*
============
sdAdminSystemCommand_SetTeam::PerformCommand
============
*/
bool sdAdminSystemCommand_SetTeam::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_SETTEAM ) ) {
		Print( player, "guis/admin/system/nopermsetnumteamplayers" );
		return false;
	}

	const char* playerName = cmd.Argv( 2 );

	idPlayer* otherPlayer = gameLocal.GetClientByName( playerName );
	if ( !otherPlayer ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	int teamIndex = sdGameRules::TeamIndexForName( cmd.Argv( 3 ) );
	if ( teamIndex == -1 ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", cmd.Argv( 3 ) ) );
		Print( player, "guis/admin/system/cannotfindteam", list );
		return false;
	}

	idWStrList list( 1 );
	list.Append( va( L"%hs", player ? player->userInfo.cleanName.c_str() : "the server" ) );
	Print( otherPlayer, "guis/admin/system/forcedchangeteam", list );
	gameLocal.rules->SetClientTeam( otherPlayer, teamIndex, true, "" );
	return true;
}

/*
============
sdAdminSystemCommand_SetTeam::CommandCompletion
============
*/
void sdAdminSystemCommand_SetTeam::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* clientName = args.Argv( 2 );

	sdTeamManagerLocal& teamManager = sdTeamManager::GetInstance();

	int len = idStr::Length( clientName );
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		if ( idStr::Icmp( player->userInfo.cleanName, clientName ) ) {			
			callback( va( "%s %s \"%s\" \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str(), "spectator" ) );
			for ( int j = 0; j < teamManager.GetNumTeams(); j++ ) {
				sdTeamInfo& team = teamManager.GetTeamByIndex( j );
					callback( va( "%s %s \"%s\" \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str(), team.GetLookupName() ) );				
			}
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}	
}

/*
===============================================================================

	sdAdminSystemCommand_SetCampaign

===============================================================================
*/

/*
============
sdAdminSystemCommand_SetCampaign::PerformCommand
============
*/
bool sdAdminSystemCommand_SetCampaign::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_CHANGE_CAMPAIGN ) ) {
		Print( player, "guis/admin/system/nopermchangecampaign" );
		return false;
	}

	const char* campaignName = cmd.Argv( 2 );

	const metaDataContext_t* metaData = gameLocal.campaignMetaDataList->FindMetaDataContext( campaignName );
	if ( metaData == NULL || !gameLocal.IsMetaDataValidForPlay( *metaData, false ) ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", campaignName ) );
		Print( player, "guis/admin/system/notfoundcampaign", list );
		return false;
	}

	sdAdminSystemCommand_KickAllBots::DoKick();

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "set si_rules sdGameRulesCampaign\n" );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "spawnServer %s\n", campaignName ) );

	return true;
}

/*
============
sdAdminSystemCommand_SetCampaign::CommandCompletion
============
*/
void sdAdminSystemCommand_SetCampaign::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* cmd = args.Argv( 2 );
	int len = idStr::Length( cmd );

	int num = gameLocal.campaignMetaDataList->GetNumMetaData();
	for ( int i = 0; i < num; i++ ) {
		const metaDataContext_t& metaData = gameLocal.campaignMetaDataList->GetMetaDataContext( i );
		if ( !gameLocal.IsMetaDataValidForPlay( metaData, false ) ) {
			continue;
		}
		const idDict& meta = *metaData.meta;
		
		const char* metaName = meta.GetString( "metadata_name" );
		if ( idStr::Icmpn( metaName, cmd, len ) ) {
			continue;
		}

		callback( va( "%s %s %s", args.Argv( 0 ), args.Argv( 1 ), metaName ) );
	}	
}

/*
===============================================================================

	sdAdminSystemCommand_SetObjectiveMap

===============================================================================
*/

/*
============
sdAdminSystemCommand_SetObjectiveMap::PerformCommand
============
*/
bool sdAdminSystemCommand_SetObjectiveMap::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_CHANGE_MAP ) ) {
		Print( player, "guis/admin/system/nopermchangemap" );
		return false;
	}

	sdAdminSystemCommand_KickAllBots::DoKick();

	idStr mapName = cmd.Argv( 2 );
	sdGameRules_SingleMapHelper::SanitizeMapName( mapName, false );

#if defined( SD_PUBLIC_BUILD )
	const metaDataContext_t* metaData = gameLocal.mapMetaDataList->FindMetaDataContext( mapName.c_str() );
	if ( metaData == NULL || !gameLocal.IsMetaDataValidForPlay( *metaData, false ) ) {
		Print( player, "guis/admin/system/cannotchangethatmap" );
		return false;
	}
#endif // SD_PUBLIC_BUILD

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "set si_rules sdGameRulesObjective\n" );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "spawnServer %s\n", mapName.c_str() ) );

	return true;
}

/*
============
sdAdminSystemCommand_SetObjectiveMap::CommandCompletion
============
*/
void sdAdminSystemCommand_SetObjectiveMap::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdAdminSystemCommand_SetStopWatchMap

===============================================================================
*/

/*
============
sdAdminSystemCommand_SetStopWatchMap::PerformCommand
============
*/
bool sdAdminSystemCommand_SetStopWatchMap::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_CHANGE_MAP ) ) {
		Print( player, "guis/admin/system/nopermchangemap" );
		return false;
	}

	sdAdminSystemCommand_KickAllBots::DoKick();

	idStr mapName = cmd.Argv( 2 );
	sdGameRules_SingleMapHelper::SanitizeMapName( mapName, false );

#if defined( SD_PUBLIC_BUILD )
	const metaDataContext_t* metaData = gameLocal.mapMetaDataList->FindMetaDataContext( mapName.c_str() );
	if ( metaData == NULL || !gameLocal.IsMetaDataValidForPlay( *metaData, false ) ) {
		Print( player, "guis/admin/system/cannotchangethatmap" );
		return false;
	}
#endif // SD_PUBLIC_BUILD

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "set si_rules sdGameRulesStopWatch\n" );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "spawnServer %s\n", mapName.c_str() ) );

	return true;
}

/*
============
sdAdminSystemCommand_SetStopWatchMap::CommandCompletion
============
*/
void sdAdminSystemCommand_SetStopWatchMap::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdAdminSystemCommand_GlobalMute

===============================================================================
*/

/*
============
sdAdminSystemCommand_GlobalMute::PerformCommand
============
*/
bool sdAdminSystemCommand_GlobalMute::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_GLOBAL_MUTE ) ) {
		Print( player, "guis/admin/system/nopermglobalchatmute" );
		return false;
	}

	const char* value = cmd.Argv( 2 );
	if ( !*value ) {
		Print( player, "guis/admin/system/specifyonoff" );
		return false;
	}

	if ( !idStr::Icmp( value, "on" ) ) {
		si_disableGlobalChat.SetBool( true );
		return true;
	}
	
	if ( !idStr::Icmp( value, "off" ) ) {
		si_disableGlobalChat.SetBool( false );
		return true;
	}

	idWStrList list( 1 ); list.Append( va( L"%hs", value ) );
	Print( player, "guis/admin/system/unknownmutemode" );
	return false;
}

/*
============
sdAdminSystemCommand_GlobalMute::CommandCompletion
============
*/
void sdAdminSystemCommand_GlobalMute::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	callback( va( "%s %s on", args.Argv( 0 ), args.Argv( 1 ) ) );
	callback( va( "%s %s off", args.Argv( 0 ), args.Argv( 1 ) ) );
}

/*
===============================================================================

	sdAdminSystemCommand_GlobalVOIPMute

===============================================================================
*/

/*
============
sdAdminSystemCommand_GlobalVOIPMute::PerformCommand
============
*/
bool sdAdminSystemCommand_GlobalVOIPMute::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_GLOBAL_MUTE_VOIP ) ) {
		Print( player, "guis/admin/system/nopermchangeglobalvoipmute" );
		return false;
	}

	const char* value = cmd.Argv( 2 );
	if ( !*value ) {
		Print( player, "guis/admin/system/specifyonoff" );
		return false;
	}

	if ( !idStr::Icmp( value, "on" ) ) {
		g_disableGlobalAudio.SetBool( true );
		return true;
	}
	
	if ( !idStr::Icmp( value, "off" ) ) {
		g_disableGlobalAudio.SetBool( false );
		return true;
	}

	idWStrList list( 1 );
	list.Append( va( L"%hs", value ) );
	Print( player, "guis/admin/system/unknownmutemode", list );
	return false;
}

/*
============
sdAdminSystemCommand_GlobalVOIPMute::CommandCompletion
============
*/
void sdAdminSystemCommand_GlobalVOIPMute::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	callback( va( "%s %s on", args.Argv( 0 ), args.Argv( 1 ) ) );
	callback( va( "%s %s off", args.Argv( 0 ), args.Argv( 1 ) ) );
}



/*
===============================================================================

	sdAdminSystemCommand_PlayerMute

===============================================================================
*/

/*
============
sdAdminSystemCommand_PlayerMute::PerformCommand
============
*/
bool sdAdminSystemCommand_PlayerMute::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_PLAYER_MUTE ) ) {
		Print( player, "guis/admin/system/nopermchangeplayermute" );
		return false;
	}

	const char* playerName = cmd.Argv( 2 );
	idPlayer* mutePlayer = gameLocal.GetClientByName( playerName );
	if ( !mutePlayer ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	const char* value = cmd.Argv( 3 );
	if ( !*value ) {
		Print( player, "guis/admin/system/specifyonoff" );
		return false;
	}

	if ( !idStr::Icmp( value, "on" ) ) {
		const sdUserGroup& muteGroup = sdUserGroupManager::GetInstance().GetGroup( mutePlayer->GetUserGroup() );
		if ( muteGroup.HasPermission( PF_NO_MUTE ) || gameLocal.IsLocalPlayer( mutePlayer ) ) {
			Print( player, "guis/admin/system/cannotmuteplayer" );
			return false;
		}

		Print( mutePlayer, "rules/messages/textmuted", idWStrList() );
		gameLocal.rules->Mute( mutePlayer, MF_CHAT );
		return true;
	}
	
	if ( !idStr::Icmp( value, "off" ) ) {
		Print( mutePlayer, "rules/messages/textunmuted", idWStrList() );
		gameLocal.rules->UnMute( mutePlayer, MF_CHAT );
		return true;
	}

	idWStrList list( 1 );
	list.Append( va( L"%hs", value ) );
	Print( player, "guis/admin/system/unknownmutemode", list );
	return false;
}

/*
============
sdAdminSystemCommand_PlayerMute::CommandCompletion
============
*/
void sdAdminSystemCommand_PlayerMute::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* name = args.Argv( 2 );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		

		if ( idStr::Icmp( name, player->userInfo.cleanName ) ) {
			continue;
		}

		callback( va( "%s %s \"%s\" on", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		
		callback( va( "%s %s \"%s\" off", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		
	}
}

/*
===============================================================================

	sdAdminSystemCommand_PlayerVOIPMute

===============================================================================
*/

/*
============
sdAdminSystemCommand_PlayerVOIPMute::PerformCommand
============
*/
bool sdAdminSystemCommand_PlayerVOIPMute::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_PLAYER_MUTE_VOIP ) ) {
		Print( player, "guis/admin/system/nopermchangeplayervoip" );
		return false;
	}

	const char* playerName = cmd.Argv( 2 );
	idPlayer* mutePlayer = gameLocal.GetClientByName( playerName );
	if ( !mutePlayer ) {
		idWStrList list( 1 ); list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	const char* value = cmd.Argv( 3 );
	if ( !*value ) {
		Print( player, "guis/admin/system/specifyonoff" );
		return false;
	}

	if ( !idStr::Icmp( value, "on" ) ) {
		const sdUserGroup& muteGroup = sdUserGroupManager::GetInstance().GetGroup( mutePlayer->GetUserGroup() );
		if ( muteGroup.HasPermission( PF_NO_MUTE_VOIP ) || gameLocal.IsLocalPlayer( mutePlayer ) ) {
			Print( player, "guis/admin/system/cannotmuteplayer" );
			return false;
		}

		Print( mutePlayer, "rules/messages/voipmuted", idWStrList() );
		gameLocal.rules->Mute( mutePlayer, MF_AUDIO );
		return true;
	}
	
	if ( !idStr::Icmp( value, "off" ) ) {
		Print( mutePlayer, "rules/messages/voipunmuted", idWStrList() );
		gameLocal.rules->UnMute( mutePlayer, MF_AUDIO );
		return true;
	}

	idWStrList list( 1 );
	list.Append( va( L"%hs", value ) );
	Print( player, "guis/admin/system/unknownmutemode", list );
	return false;
}

/*
============
sdAdminSystemCommand_PlayerMute::CommandCompletion
============
*/
void sdAdminSystemCommand_PlayerVOIPMute::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* name = args.Argv( 2 );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		

		if ( idStr::Icmp( name, player->userInfo.cleanName ) != 0 ) {
			continue;
		}

		callback( va( "%s %s \"%s\" on", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		
		callback( va( "%s %s \"%s\" off", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );		
	}
}

/*
===============================================================================

	sdAdminSystemCommand_Warn

===============================================================================
*/

/*
============
sdAdminSystemCommand_Warn::PerformCommand
============
*/
bool sdAdminSystemCommand_Warn::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_WARN ) ) {
		Print( player, "guis/admin/system/nopermwarn" );
		return false;
	}

	const char* playerName = cmd.Argv( 2 );
	idPlayer* warnPlayer = gameLocal.GetClientByName( playerName );
	if ( !warnPlayer ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	const sdUserGroup& warnUserGroup = sdUserGroupManager::GetInstance().GetGroup( warnPlayer->GetUserGroup() );
	if ( warnUserGroup.HasPermission( PF_NO_WARN ) || gameLocal.IsLocalPlayer( warnPlayer ) ) {
		Print( player, "guis/admin/system/cannotwarnplayer" );
		return false;
	}

	gameLocal.rules->Warn( warnPlayer );
	return true;
}

/*
============
sdAdminSystemCommand_Warn::CommandCompletion
============
*/
void sdAdminSystemCommand_Warn::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdAdminSystemCommand_RestartMap

===============================================================================
*/

/*
============
sdAdminSystemCommand_RestartMap::PerformCommand
============
*/
bool sdAdminSystemCommand_RestartMap::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_RESTART_MAP ) ) {
		Print( player, "guis/admin/system/nopermrestartmap" );
		return false;
	}

	if ( gameLocal.GameState() != GAMESTATE_ACTIVE ) {
		return false;
	}

	sdProficiencyManager::GetInstance().ResetToBasePoints();
	gameLocal.LocalMapRestart();
	gameLocal.rules->MapRestart();
	return true;
}

/*
============
sdAdminSystemCommand_RestartMap::CommandCompletion
============
*/
void sdAdminSystemCommand_RestartMap::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdAdminSystemCommand_RestartCampaign

===============================================================================
*/

/*
============
sdAdminSystemCommand_RestartCampaign::PerformCommand
============
*/
bool sdAdminSystemCommand_RestartCampaign::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_RESTART_CAMPAIGN ) ) {
		Print( player, "guis/admin/system/nopermrestartcampaign" );
		return false;
	}

	sdGameRulesCampaign* campaignRules = gameLocal.rules->Cast< sdGameRulesCampaign >();
	if ( !campaignRules ) {
		Print( player, "guis/admin/system/servernocampaign" );
		return false;
	}

	const sdDeclCampaign* campaignDecl = campaignRules->GetCampaign();
	if ( !campaignDecl ) {
		Print( player, "guis/admin/system/nocampaignrunning" );
		return false;
	}

	campaignRules->SetCampaign( campaignDecl );
	campaignRules->StartMap();
	return true;
}

/*
============
sdAdminSystemCommand_RestartCampaign::CommandCompletion
============
*/
void sdAdminSystemCommand_RestartCampaign::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdAdminSystemCommand_ChangeUserGroup

===============================================================================
*/

/*
============
sdAdminSystemCommand_ChangeUserGroup::PerformCommand
============
*/
bool sdAdminSystemCommand_ChangeUserGroup::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	const char* playerName = cmd.Argv( 2 );
	idPlayer* changePlayer = gameLocal.GetClientByName( playerName );
	if ( !changePlayer ) {
		idWStrList list( 1 );
		list.Append( va( L"%hs", playerName ) );
		Print( player, "guis/admin/system/cannotfindplayer", list );
		return false;
	}

	const sdUserGroup& oldGroup = sdUserGroupManager::GetInstance().GetGroup( changePlayer->GetUserGroup() );
	if ( !userGroup.CanControl( oldGroup ) ) {
		Print( player, "guis/admin/system/cannotchangeplayergroup" );
		return false;
	}

	const char* groupName = cmd.Argv( 3 );
	int index = sdUserGroupManager::GetInstance().FindGroup( groupName );
	if ( index == -1 ) {
		idWStrList list( 1 ); list.Append( va( L"%hs", groupName ) );
		Print( player, "guis/admin/system/cannotfindgroup", list );
		return false;
	}

	const sdUserGroup& group = sdUserGroupManager::GetInstance().GetGroup( index );
	if ( !userGroup.CanControl( group ) ) {
		Print( player, "guis/admin/system/cannotplaceplayergroup" );
		return false;
	}

	changePlayer->SetUserGroup( index );
	return true;
}

/*
============
sdAdminSystemCommand_ChangeUserGroup::CommandCompletion
============
*/
void sdAdminSystemCommand_ChangeUserGroup::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	const char* clientName = args.Argv( 2 );

	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		if ( !idStr::Icmp( clientName, player->userInfo.cleanName ) ) {
			for ( int j = 1; j < groupManager.NumGroups(); j++ ) {
				callback( va( "%s %s \"%s\" \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str(), groupManager.GetGroup( j ).GetName() ) );
			}
		}

		callback( va( "%s %s \"%s\"", args.Argv( 0 ), args.Argv( 1 ), player->userInfo.cleanName.c_str() ) );
	}
}

/*
===============================================================================

	sdAdminSystemCommand_StartMatch

===============================================================================
*/

/*
============
sdAdminSystemCommand_StartMatch::PerformCommand
============
*/
bool sdAdminSystemCommand_StartMatch::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_START_MATCH ) ) {
		Print( player, "guis/admin/system/nopermstartmatch" );
		return false;
	}

	gameLocal.rules->AdminStart();
	return true;
}

/*
============
sdAdminSystemCommand_StartMatch::CommandCompletion
============
*/
void sdAdminSystemCommand_StartMatch::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
}

/*
===============================================================================

	sdAdminSystemCommand_ExecConfig

===============================================================================
*/

/*
============
sdAdminSystemCommand_ExecConfig::PerformCommand
============
*/
bool sdAdminSystemCommand_ExecConfig::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_EXEC_CONFIG ) ) {
		Print( player, "guis/admin/system/nopermexecconfig" );
		return false;
	}

	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();
	const idDict& configs = groupManager.GetConfigs();

	const char* configName = cmd.Argv( 2 );

	const idKeyValue* kv = configs.FindKey( configName );
	if ( !kv ) {
		idWStrList list( 1 ); list.Append( va( L"%hs", configName ) );
		Print( player, "guis/admin/system/cannotfindconfig", list );
		return false;
	}

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "exec %s", kv->GetValue().c_str() ) );
	return true;
}

/*
============
sdAdminSystemCommand_ExecConfig::CommandCompletion
============
*/
void sdAdminSystemCommand_ExecConfig::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();

	const idDict& configs = groupManager.GetConfigs();

	int count = configs.GetNumKeyVals();
	for ( int i = 0; i < count; i++ ) {
		const idKeyValue* kv = configs.GetKeyVal( i );
		callback( va( "%s %s %s", args.Argv( 0 ), args.Argv( 1 ), kv->GetKey().c_str() ) );
	}
}

/*
===============================================================================

	sdAdminSystemCommand_ShuffleTeams

===============================================================================
*/

/*
============
sdAdminSystemCommand_ShuffleTeams::PerformCommand
============
*/
bool sdAdminSystemCommand_ShuffleTeams::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {	
	if ( !userGroup.HasPermission( PF_ADMIN_SHUFFLE_TEAMS ) ) {
		Print( player, "guis/admin/system/nopermshuffleteams" );
		return false;
	}
	
	const char* shuffleModeName = cmd.Args( 2 );

	if ( !idStr::Icmp( shuffleModeName, "xp" ) ) {
		botThreadData.teamsSwapedRecently = true;
		gameLocal.rules->ShuffleTeams( sdGameRules::SM_XP );
		return true;
	} else if ( !idStr::Icmp( shuffleModeName, "random" ) ) {
		botThreadData.teamsSwapedRecently = true;
		gameLocal.rules->ShuffleTeams( sdGameRules::SM_RANDOM );
		return true;
	} else if ( !networkSystem->IsRankedServer() && !idStr::Icmp( shuffleModeName, "swap" ) ) {
		botThreadData.teamsSwapedRecently = true;
		gameLocal.rules->ShuffleTeams( sdGameRules::SM_SWAP );
		return true;
	}

	idWStrList list( 1 );
	list.Append( va( L"%hs", shuffleModeName ) );
	Print( player, "guis/admin/system/unknownshufflemode", list );
	return false;
}


/*
============
sdAdminSystemCommand_ShuffleTeams::CommandCompletion
============
*/
void sdAdminSystemCommand_ShuffleTeams::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	callback( va( "%s %s xp", args.Argv( 0 ), args.Argv( 1 ) ) );
	callback( va( "%s %s random", args.Argv( 0 ), args.Argv( 1 ) ) );
	if ( !networkSystem->IsRankedServer() ) {
		callback( va( "%s %s swap", args.Argv( 0 ), args.Argv( 1 ) ) );
	}
}

/*
============
sdAdminSystemCommand_AddBot::PerformCommand
============
*/
bool sdAdminSystemCommand_AddBot::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_ADD_BOT ) ) {
		Print( player, "guis/admin/system/nopermaddbot" );
		return false;
	}

	int clientNum;
	int botTeam = NOTEAM;
	int botClass = MEDIC;
	int botGun = -1; // random weapon layout

	if ( !bot_enable.GetBool() ) {
		gameLocal.Warning( "Can't add bot because bots are disabled on this server!\nSet \"bot_enable\" to 1 to enable them and try again." );
		return false;
	}

	if ( cmd.Argc() > 2 ) {
		if ( idStr::Icmp( cmd.Argv( 2 ), "strogg" ) == 0 || idStr::Icmp( cmd.Argv( 2 ), "s" ) == 0 ) {
			botTeam = STROGG;
		} else if ( idStr::Icmp( cmd.Argv( 2 ), "gdf" ) == 0 || idStr::Icmp( cmd.Argv( 2 ), "g" ) == 0 ) {
			botTeam = GDF;
		} else {
			gameLocal.Warning( "Invalid team value passed to Addbot! ^1Valid values are: gdf, strogg" );
			return false;
		}
	} else {
		int numGDF = botThreadData.GetNumClientsOnTeam( GDF );
		int numStrogg = botThreadData.GetNumClientsOnTeam( STROGG );
		int limitStrogg = bot_uiNumStrogg.GetInteger();
		int limitGDF = bot_uiNumGDF.GetInteger();

		if ( numGDF > numStrogg && ( limitStrogg == -1 || numStrogg < limitStrogg ) ) {
			botTeam = STROGG;
		} else if ( numStrogg > numGDF && ( limitGDF == -1 || numGDF < limitGDF ) ) {
			botTeam = GDF;
		} else {
			if( ( limitStrogg == -1 || numStrogg < limitStrogg ) && ( limitGDF == -1 || numGDF < limitGDF ) ) {
				botTeam = gameLocal.random.RandomInt( 2 );
			} else if( numStrogg < limitStrogg || limitStrogg == -1 ) {
				botTeam = STROGG;
			} else if( numGDF < limitGDF || limitGDF == -1 ) {
				botTeam = GDF;
			}
		}
	}

	if( botTeam == NOTEAM ) {
		return false;
	}

	int maxPlayers = si_maxPlayers.GetInteger();
	int maxPrivatePlayers = gameLocal.GetMaxPrivateClients();
	if ( maxPrivatePlayers > maxPlayers ) {
		maxPrivatePlayers = maxPlayers;
	}
	int maxRegularPlayers = maxPlayers - maxPrivatePlayers;

	clientNum = networkSystem->AllocateClientSlotForBot( maxRegularPlayers );

	if ( clientNum == -1 ) {
		gameLocal.DWarning( "Can't find open client slot for bot - start server with more client slots!\nCheck the value of \"si_maxPlayers\"" );
		return false;
	}

	playerTeamTypes_t team = ( botTeam == 0 ) ? GDF : STROGG;

	if ( cmd.Argc() > 3 ) {
		const char* className = cmd.Argv( 3 );
		if ( idStr::Icmp( className, "medic" ) == 0 || idStr::Icmp( className, "technician" ) == 0 || idStr::Icmp( className, "med" ) == 0 || idStr::Icmp( className, "tech" ) == 0 ) {
			botClass = MEDIC;
		} else if ( idStr::Icmp( className, "soldier" ) == 0 || idStr::Icmp( className, "aggressor" ) == 0 || idStr::Icmp( className, "sol" ) == 0 || idStr::Icmp( className, "agg" ) == 0 ) {
			botClass = SOLDIER;
		} else if ( idStr::Icmp( className, "engineer" ) == 0 || idStr::Icmp( className, "constructor" ) == 0 || idStr::Icmp( className, "eng" ) == 0 || idStr::Icmp( className, "con" ) == 0 ) {
			botClass = ENGINEER;
		} else if ( idStr::Icmp( className, "fieldops" ) == 0 || idStr::Icmp( className, "oppressor" ) == 0 || idStr::Icmp( className, "fop" ) == 0 || idStr::Icmp( className, "opp" ) == 0 ) {
			botClass = FIELDOPS;
		} else if ( idStr::Icmp( className, "covertops" ) == 0 || idStr::Icmp( className, "infiltrator" ) == 0 || idStr::Icmp( className, "covert" ) == 0 || idStr::Icmp( className, "cov" ) == 0 || idStr::Icmp( className, "inf" ) == 0  ) {
			botClass = COVERTOPS;
		} else {
			gameLocal.Warning( "Invalid class value passed to Addbot! ^1Valid values are: medic, soldier, engineer, fieldops, covertops, technician, aggressor, constructor, oppressor, infiltrator" );
		}
	} else {

		playerClassTypes_t criticalClass;
		int numDesiredCriticalClass;
		int	numDesiredMedicClass;

		if ( botTeam == GDF ) {
			criticalClass = botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_criticalClass;
			numDesiredCriticalClass = ( botThreadData.GetNumClientsOnTeam( GDF ) >= 6 ) ? 3 : 2;
			numDesiredMedicClass = ( botThreadData.GetNumClientsOnTeam( GDF ) >= 7 ) ? 2 : 1;
		} else {
			criticalClass = botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass;
			numDesiredCriticalClass = ( botThreadData.GetNumClientsOnTeam( STROGG ) >= 6 ) ? 3 : 2;
			numDesiredMedicClass = ( botThreadData.GetNumClientsOnTeam( STROGG ) >= 7 ) ? 2 : 1;
		}

		int numMedic = botThreadData.GetNumClassOnTeam( team, MEDIC );
		int numEng = botThreadData.GetNumClassOnTeam( team, ENGINEER );
		int numFOps = botThreadData.GetNumClassOnTeam( team, FIELDOPS );
		int numCovert = botThreadData.GetNumClassOnTeam( team, COVERTOPS );
		int numSoldier = botThreadData.GetNumClassOnTeam( team, SOLDIER );
		int numCritical;

		if ( criticalClass != NOCLASS && bot_doObjectives.GetBool() ) {
			numCritical = botThreadData.GetNumClassOnTeam( team, criticalClass );
		} else {
			numCritical = -1;
		}

		//mal: make sure we always have a couple bots of the critical class first, unless the human wants to do the obj.
		if ( numCritical < numDesiredCriticalClass && numCritical != -1 ) {
			botClass = criticalClass;
		} else if ( numMedic < numDesiredMedicClass ) {
			botClass = MEDIC;
		} else if ( numSoldier == 0 ) {
			botClass = SOLDIER;
		} else if ( numEng == 0 ) {
			botClass = ENGINEER;
		} else if ( numCovert == 0 ) {
			botClass = COVERTOPS;
		} else if ( numFOps == 0 ) {
			botClass = FIELDOPS;
		} else {
			botClass = gameLocal.random.RandomInt( 5 );
		}
	}

	int maxGuns = 1;

	if ( bot_useShotguns.GetBool() ) { //mal: this is mainly for debugging, but may be a popular choice since the shotgun is so range limited.
		maxGuns = 2;
		if ( botClass == SOLDIER ) {
			maxGuns = 4;
		} else if ( botClass == FIELDOPS ) {
			maxGuns = 1;
		}
	} else {
		if ( botClass == SOLDIER ) {
			maxGuns = 3;
		} else if ( botClass == COVERTOPS ) {
			maxGuns = 2;
		}
	}

	if ( !bot_useSniperWeapons.GetBool() ) {
		if ( botClass == COVERTOPS ) {
			maxGuns = 1;
		}
	}

	if ( cmd.Argc() > 4 ) {
		if ( idStr::Icmp( cmd.Argv( 4 ), "random" ) == 0 ) {
			botGun = -1;
		} else {
			botGun = atoi( cmd.Argv( 4 ) );
			if ( botGun < 0 || botGun >= maxGuns ) {
				common->Warning( "Weapon value is out of range. Valid values are 'random' or 0-%d", maxGuns-1 );
				botGun = -1;
			}
		}
	}

	if ( botClass == SOLDIER && botThreadData.GetNumClassOnTeam( team, SOLDIER ) == 0 ) { //mal: if haven't spawned a RL soldier yet, do so now.
		botGun = 1;
	}

	if ( botGun < 0 ) {
		botGun = gameLocal.random.RandomInt( maxGuns );
	}

	idPlayer* newPlayer = gameLocal.GetClient( clientNum );

	idStr playerName = newPlayer->userInfo.baseName;

	if ( playerName.IsEmpty() ) { //mal: this shouldn't ever happen, but just in case...
		const char * basePlayerName = "Bot";

		if ( botTeam == GDF ) {
			basePlayerName = "GDF Bot";
		} else {
			basePlayerName = "Strogg Bot";
		}	

		sprintf( playerName, "%s %d", basePlayerName, clientNum );

		networkSystem->ServerSetBotUserName( clientNum, playerName );
	}


	gameLocal.rules->SetClientTeam( newPlayer, botTeam + 1, true, "" );

	idBot::Bot_SetClassType( newPlayer, botClass, botGun );

	return true;
}

/*
============
sdAdminSystemCommand_AddBot::CommandCompletion
============
*/
void sdAdminSystemCommand_AddBot::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	if( args.Argc() < 3 ) {
		callback( va( "%s %s gdf", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s strogg", args.Argv( 0 ), args.Argv( 1 ) ) );
		return;
	}

	if( args.Argc() < 4 ) {
		if( !idStr::Icmp( args.Argv( 2 ), "gdf" ) ) {
			callback( va( "%s %s %s soldier", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s medic", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s engineer", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s fieldops", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s covertops", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			return;
		}
		if( !idStr::Icmp( args.Argv( 2 ), "strogg" ) ) {
			callback( va( "%s %s %s aggressor", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s technician", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s constructor", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s oppressor", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			callback( va( "%s %s %s infiltrator", args.Argv( 0 ), args.Argv( 1 ), args.Argv( 2 ) ) );
			return;
		}
		return;
	}
}

/*
============
sdAdminSystemCommand_AdjustBots::PerformCommand
============
*/
bool sdAdminSystemCommand_AdjustBots::PerformCommand( const idCmdArgs& cmd, const sdUserGroup& userGroup, idPlayer* player ) const {
	if ( !userGroup.HasPermission( PF_ADMIN_ADJUST_BOTS ) ) {
		Print( player, "guis/admin/system/nopermadjustbots" );
		return false;
	}
	
	if( cmd.Argc() < 4 ) {
		return false;
	}

	if( idStr::Icmp( cmd.Argv( 2 ), "minClients" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( -1, num );
		num = Min( num, MAX_CLIENTS );
		bot_minClients.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "numStrogg" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( -1, num );
		num = Min( num, MAX_CLIENTS );
		bot_uiNumStrogg.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "numGDF" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( -1, num );
		num = Min( num, MAX_CLIENTS );
		bot_uiNumGDF.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "aimSkill" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( 0, num );
		num = Min( num, 3 );
		bot_aimSkill.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "skill" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( 0, num );
		num = Min( num, 3 );
		bot_skill.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "uiSkill" ) == 0 ) {
		int num = atoi( cmd.Argv( 3 ) );
		num = Max( 0, num );
		num = Min( num, 4 );
		bot_uiSkill.SetInteger( num );
	} else if( idStr::Icmp( cmd.Argv( 2 ), "doObjectives" ) == 0 ) {
		int value = atoi( cmd.Argv( 3 ) );
		bot_doObjectives.SetBool( value != 0 );
	}

	return true;
}

/*
============
sdAdminSystemCommand_AdjustBots::CommandCompletion
============
*/
void sdAdminSystemCommand_AdjustBots::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) const {
	if( args.Argc() < 3 ) {
		callback( va( "%s %s minClients", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s numStrogg", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s numGDF", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s aimSkill", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s skill", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s uiSkill", args.Argv( 0 ), args.Argv( 1 ) ) );
		callback( va( "%s %s doObjectives", args.Argv( 0 ), args.Argv( 1 ) ) );
		return;
	}
}

/*
===============================================================================

	sdAdminSystemLocal

===============================================================================
*/

/*
============
sdAdminSystemLocal::sdAdminSystemLocal
============
*/
sdAdminSystemLocal::sdAdminSystemLocal( void ) :
	commandState( CS_NOCOMMAND ),
	commandStateTime( 0 ) {
}

/*
============
sdAdminSystemLocal::sdAdminSystemLocal
============
*/
sdAdminSystemLocal::~sdAdminSystemLocal( void ) {
	commands.DeleteContents( true );
}

/*
============
sdAdminSystemLocal::Init
============
*/
void sdAdminSystemLocal::Init( void ) {
	commands.PreAllocate( 25 );		// NOTE: if you add more commands increase this number to reflect that
	commands.Alloc() = new sdAdminSystemCommand_Kick();
	commands.Alloc() = new sdAdminSystemCommand_Login();
	commands.Alloc() = new sdAdminSystemCommand_Ban();
	commands.Alloc() = new sdAdminSystemCommand_SetTeam();
	commands.Alloc() = new sdAdminSystemCommand_KickAllBots();

	commands.Alloc() = new sdAdminSystemCommand_SetCampaign();
	commands.Alloc() = new sdAdminSystemCommand_SetStopWatchMap();	
	commands.Alloc() = new sdAdminSystemCommand_SetObjectiveMap();

	commands.Alloc() = new sdAdminSystemCommand_GlobalMute();
	commands.Alloc() = new sdAdminSystemCommand_GlobalVOIPMute();
	commands.Alloc() = new sdAdminSystemCommand_PlayerMute();
	commands.Alloc() = new sdAdminSystemCommand_PlayerVOIPMute();
	commands.Alloc() = new sdAdminSystemCommand_Warn();
	commands.Alloc() = new sdAdminSystemCommand_RestartMap();
	commands.Alloc() = new sdAdminSystemCommand_RestartCampaign();
	commands.Alloc() = new sdAdminSystemCommand_ChangeUserGroup();
	commands.Alloc() = new sdAdminSystemCommand_StartMatch();
	commands.Alloc() = new sdAdminSystemCommand_ExecConfig();
	commands.Alloc() = new sdAdminSystemCommand_ShuffleTeams();
	commands.Alloc() = new sdAdminSystemCommand_AddBot();
	commands.Alloc() = new sdAdminSystemCommand_AdjustBots();
	commands.Alloc() = new sdAdminSystemCommand_DisableProficiency();
	commands.Alloc() = new sdAdminSystemCommand_SetTimeLimit();
	commands.Alloc() = new sdAdminSystemCommand_SetTeamDamage();
	commands.Alloc() = new sdAdminSystemCommand_SetTeamBalance();

	commands.Alloc() = new sdAdminSystemCommand_ListBans();
	commands.Alloc() = new sdAdminSystemCommand_UnBan();

	properties.RegisterProperties();
}

/*
============
sdAdminSystemLocal::PerformCommand
============
*/
void sdAdminSystemLocal::PerformCommand( const idCmdArgs& cmd, idPlayer* player ) {
	assert( idStr::Icmp( "admin", cmd.Argv( 0 ) ) == 0 );

	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();

	const sdUserGroup& userGroup = player != NULL ? groupManager.GetGroup( player->GetUserGroup() ) : groupManager.GetConsoleGroup();

	const char* command = cmd.Argv( 1 );
	for ( int i = 0; i < commands.Num(); i++ ) {
		if ( idStr::Icmp( command, commands[ i ]->GetName() ) ) {
			continue;
		}

		bool success = true;
		if ( networkSystem->IsRankedServer() ) {
			if ( !commands[ i ]->AllowedOnRankedServer() ) {
				success = false;
			}
		}
		if ( success ) {
			success = commands[ i ]->PerformCommand( cmd, userGroup, player );
		}

		if ( player == NULL || player == gameLocal.GetLocalPlayer() ) {
			SetCommandState( success ? CS_SUCCESS : CS_FAILED );
		} else {
			sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_ADMINFEEDBACK );
			msg.WriteBool( success );
			msg.Send( sdReliableMessageClientInfo( player->entityNumber ) );
		}
		break;
	}
}

/*
============
sdAdminSystemLocal::CommandCompletion
============
*/
void sdAdminSystemLocal::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) {
	sdAdminSystemLocal& admin = sdAdminSystem::GetInstance();

	const char* cmd = args.Argv( 1 );

	for ( int i = 0; i < admin.commands.Num(); i++ ) {
		if ( idStr::Icmp( cmd, admin.commands[ i ]->GetName() ) ) {
			continue;
		}

		if ( networkSystem->IsRankedServer() ) {
			if ( !admin.commands[ i ]->AllowedOnRankedServer() ) {
				continue;
			}
		}

		admin.commands[ i ]->CommandCompletion( args, callback );
	}

	int cmdLength = idStr::Length( cmd );
	for ( int i = 0; i < admin.commands.Num(); i++ ) {
		if ( idStr::Icmpn( cmd, admin.commands[ i ]->GetName(), cmdLength ) ) {
			continue;
		}

		if ( networkSystem->IsRankedServer() ) {
			if ( !admin.commands[ i ]->AllowedOnRankedServer() ) {
				continue;
			}
		}

		callback( va( "%s %s", args.Argv( 0 ), admin.commands[ i ]->GetName() ) );
	}
}

/*
============
sdAdminSystemLocal::CreatePlayerAdminList
============
*/
void sdAdminSystemLocal::CreatePlayerAdminList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdUserGroupManagerLocal& groupManager = sdUserGroupManager::GetInstance();

	idWStr cleanPlayerName;
	int index = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			if( g_debugPlayerList.GetInteger() ) {
				sdUIList::InsertItem( list, va( L"Player %i\tuser", i ), index, 0 );
			}
			continue;
		}

		const sdUserGroup* userGroup = NULL;
		if( ( gameLocal.isServer && gameLocal.IsLocalPlayer( player ) ) || ( !gameLocal.isClient && gameLocal.IsLocalPlayer( player ) ) ) {
			userGroup = &groupManager.GetConsoleGroup();
		} else {		
			userGroup = &groupManager.GetGroup( player->GetUserGroup() );
		}	

		cleanPlayerName = va( L"%hs", player->userInfo.cleanName.c_str() );
		sdUIList::CleanUserInput( cleanPlayerName );
		sdUIList::InsertItem( list, cleanPlayerName.c_str(), index, 0 );

		if( userGroup->IsSuperUser() ) {
			sdUIList::SetItemText( list, L"<loc = 'guis/admin/superuser'>", index, 1 );
		} else {
			sdUIList::SetItemText( list, va( L"%hs", userGroup->GetName() ), index, 1 );
		}		

		index++;
	}
}

/*
===============================================================================

	sdAdminProperties

===============================================================================
*/

/*
============
sdAdminProperties::GetProperty
============
*/
sdProperties::sdProperty* sdAdminProperties::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdAdminProperties::GetProperty
============
*/
sdProperties::sdProperty* sdAdminProperties::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && type != sdProperties::PT_INVALID && prop->GetValueType() != type ) {
		gameLocal.Error( "sdAdminProperties::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
============
sdAdminProperties::Update
============
*/
void sdAdminProperties::Update( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdUserGroupManagerLocal& userGroupManager = sdUserGroupManager::GetInstance();

	const sdUserGroup& playerGroup = gameLocal.isClient ? userGroupManager.GetGroup( localPlayer->GetUserGroup() ) : userGroupManager.GetConsoleGroup();
	
	isSuperUser			= playerGroup.IsSuperUser() ? 1.0f : 0.0f;
	if( isSuperUser != 0.0f ) {		
		userGroup			= superUserString->GetText();
	} else {
		userGroup			= va( L"%hs", playerGroup.GetName() );
	}

	commandState			= sdAdminSystem::GetInstance().GetCommandState();
	commandStateTime		= sdAdminSystem::GetInstance().GetCommandStateTime();

	canLogin				= userGroupManager.CanLogin();
	canAdjustBots			= playerGroup.HasPermission( PF_ADMIN_ADJUST_BOTS ) && !networkSystem->IsRankedServer();
	canAddBot				= playerGroup.HasPermission( PF_ADMIN_ADD_BOT ) && !networkSystem->IsRankedServer();
	canDisableProficiency	= playerGroup.HasPermission( PF_ADMIN_DISABLE_PROFICIENCY ) && !networkSystem->IsRankedServer();
	canKick					= playerGroup.HasPermission( PF_ADMIN_KICK );
	canBan					= playerGroup.HasPermission( PF_ADMIN_BAN );
	canSetTeam				= playerGroup.HasPermission( PF_ADMIN_SETTEAM );
	canChangeCampaign		= playerGroup.HasPermission( PF_ADMIN_CHANGE_CAMPAIGN );
	canChangeMap			= playerGroup.HasPermission( PF_ADMIN_CHANGE_MAP ) && !networkSystem->IsRankedServer();
	canGlobalMute			= playerGroup.HasPermission( PF_ADMIN_GLOBAL_MUTE ) && !networkSystem->IsRankedServer();
	canGlobalMuteVoip		= playerGroup.HasPermission( PF_ADMIN_GLOBAL_MUTE_VOIP );
	canPlayerMute			= playerGroup.HasPermission( PF_ADMIN_PLAYER_MUTE );
	canPlayerMuteVoip		= playerGroup.HasPermission( PF_ADMIN_PLAYER_MUTE_VOIP );
	canWarn					= playerGroup.HasPermission( PF_ADMIN_WARN );
	canRestartMap			= playerGroup.HasPermission( PF_ADMIN_RESTART_MAP );
	canRestartCampaign		= playerGroup.HasPermission( PF_ADMIN_RESTART_CAMPAIGN );
	canStartMatch 			= playerGroup.HasPermission( PF_ADMIN_START_MATCH ) && !networkSystem->IsRankedServer();
	canExecConfig 			= playerGroup.HasPermission( PF_ADMIN_EXEC_CONFIG );
	canShuffleTeams			= playerGroup.HasPermission( PF_ADMIN_SHUFFLE_TEAMS );
	canSetTimelimit			= playerGroup.HasPermission( PF_ADMIN_SET_TIMELIMIT ) && !networkSystem->IsRankedServer();
	canSetTeamDamage		= playerGroup.HasPermission( PF_ADMIN_SET_TEAMDAMAGE ) && !networkSystem->IsRankedServer();
	canSetTeamBalance		= playerGroup.HasPermission( PF_ADMIN_SET_TEAMBALANCE ) && !networkSystem->IsRankedServer();
}

/*
============
sdAdminProperties::RegisterProperties
============
*/
SD_UI_PUSH_CLASS_TAG( sdAdminProperties )
void sdAdminProperties::RegisterProperties( void ) {
	properties.RegisterProperty( "userGroup",				userGroup );
	properties.RegisterProperty( "isSuperUser",				isSuperUser );
	properties.RegisterProperty( "commandState",			commandState );
	properties.RegisterProperty( "commandStateTime",		commandStateTime );
	properties.RegisterProperty( "canLogin",				canLogin );

	properties.RegisterProperty( "canAddBot",				canAddBot );
	properties.RegisterProperty( "canAdjustBots",			canAdjustBots );
	properties.RegisterProperty( "canDisableProficiency",	canDisableProficiency );
	properties.RegisterProperty( "canKick",					canKick );
	properties.RegisterProperty( "canBan",					canBan );
	properties.RegisterProperty( "canSetTeam",				canSetTeam );
	properties.RegisterProperty( "canChangeCampaign",		canChangeCampaign );
	properties.RegisterProperty( "canChangeMap",			canChangeMap );
	properties.RegisterProperty( "canGlobalMute",			canGlobalMute );
	properties.RegisterProperty( "canGlobalMuteVoip",		canGlobalMuteVoip  );
	properties.RegisterProperty( "canPlayerMute",			canPlayerMute );
	properties.RegisterProperty( "canPlayerMuteVoip",		canPlayerMuteVoip );
	properties.RegisterProperty( "canWarn",					canWarn );
	properties.RegisterProperty( "canRestartMap",			canRestartMap );
	properties.RegisterProperty( "canRestartCampaign",		canRestartCampaign );
	properties.RegisterProperty( "canStartMatch",			canStartMatch );
	properties.RegisterProperty( "canExecConfig",			canExecConfig );
	properties.RegisterProperty( "canShuffleTeams",			canShuffleTeams );
	properties.RegisterProperty( "canSetTimelimit",			canSetTimelimit );
	properties.RegisterProperty( "canSetTeamDamage",		canSetTeamDamage );
	properties.RegisterProperty( "canSetTeamBalance",		canSetTeamBalance );

	superUserString = declHolder.declLocStrType.LocalFind( "guis/admin/superuser" );

	isSuperUser = 0.0f;
	commandState = 0.0f;
	commandStateTime = 0.0f;
	canLogin = 0.0f;

	canAddBot = 0.0f;
	canAdjustBots = 0.0f;
	canDisableProficiency = 0.0f;
	canKick = 0.0f;
	canBan = 0.0f;
	canSetTeam = 0.0f;
	canChangeCampaign = 0.0f;
	canChangeMap = 0.0f;
	canGlobalMute = 0.0f;
	canGlobalMuteVoip  = 0.0f;
	canPlayerMute = 0.0f;
	canPlayerMuteVoip = 0.0f;
	canWarn = 0.0f;
	canRestartMap = 0.0f;
	canRestartCampaign = 0.0f;
	canStartMatch = 0.0f;
	canExecConfig = 0.0f;
	canShuffleTeams = 0.0f;
	canSetTimelimit = 0.0f;
	canSetTeamDamage = 0.0f;
	canSetTeamBalance = 0.0f;

	SD_UI_PUSH_GROUP_TAG( "Admin Command State" )

	SD_UI_ENUM_TAG( CS_NOCOMMAND, "No command has been issued to the admin system." )
	sdDeclGUI::AddDefine( va( "CS_NOCOMMAND %i",	sdAdminSystemLocal::CS_NOCOMMAND ) );

	SD_UI_ENUM_TAG( CS_SUCCESS, "Admin command succeded." )
	sdDeclGUI::AddDefine( va( "CS_SUCCESS %i",		sdAdminSystemLocal::CS_SUCCESS ) );

	SD_UI_ENUM_TAG( CS_FAILED, "Admin command failed." )
	sdDeclGUI::AddDefine( va( "CS_FAILED %i",		sdAdminSystemLocal::CS_FAILED ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG

/*
============
sdAdminSystemLocal::SetCommandState
============
*/
void sdAdminSystemLocal::SetCommandState( commandState_e state ) {
	commandState = state;
	commandStateTime = gameLocal.time;
}
