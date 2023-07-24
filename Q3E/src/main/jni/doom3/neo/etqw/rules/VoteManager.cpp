// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VoteManager.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../rules/GameRules.h"
#include "../rules/AdminSystem.h"
#include "../guis/UserInterfaceLocal.h"
#include "../guis/UIList.h"
#include "../botai/Bot.h"



/*
================
sdCallVote::AddVoteOption
================
*/
void sdCallVote::AddVoteOption( sdUIList* list, const wchar_t* text ) {
	AddVoteOption( list, text, list->GetNumItems() );
}

/*
================
sdCallVote::AddVoteOption
================
*/
void sdCallVote::AddVoteOption( sdUIList* list, const wchar_t* text, int dataKey ) {
	int index = sdUIList::InsertItem( list, text, -1, 0 );
	list->SetItemDataInt( dataKey, index, 0 );	
}

/*
================
sdCallVote::CreateCmdArgs
================
*/
void sdCallVote::CreateCmdArgs( idCmdArgs& args ) const {
	args.AppendArg( "callvote" );
	args.AppendArg( GetCommandName() );
}







class sdCallVoteAdminCommandFinalizer : public sdVoteFinalizer {
public:
	sdCallVoteAdminCommandFinalizer( const char* _command, const char* _suffix ) {
		command = _command;
		suffix = _suffix;
	}

	virtual void OnVoteCompleted( bool passed ) const {
		if ( !passed ) {
			return;
		}

		idCmdArgs args;
		args.AppendArg( "admin" );
		args.AppendArg( command );
		if ( suffix != NULL ) {
			args.AppendArg( suffix );
		}
		sdAdminSystem::GetInstance().PerformCommand( args, NULL );
	}

private:
	const char* command;
	const char* suffix;
};

/*
================
sdCallVoteAdminCommand::EnumerateOptions
================
*/
void sdCallVoteAdminCommand::EnumerateOptions( sdUIList* list ) const {
}

/*
================
sdCallVoteAdminCommand::Execute
================
*/
void sdCallVoteAdminCommand::Execute( int dataKey ) const {
	idCmdArgs args;
	CreateCmdArgs( args );
	sdVoteManagerLocal::Callvote_f( args );
}

/*
================
sdCallVoteAdminCommand::Execute
================
*/
sdPlayerVote* sdCallVoteAdminCommand::Execute( const idCmdArgs& args, idPlayer* player ) const {
	sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
	if ( vote != NULL ) {
		vote->MakeGlobalVote();
		vote->SetText( gameLocal.declToolTipType[ GetVoteTextKey() ] );
		vote->SetFinalizer( new sdCallVoteAdminCommandFinalizer( GetAdminCommand(), GetAdminCommandSuffix() ) );
	}

	return vote;
}




class sdCallVoteShuffleXP : public sdCallVoteAdminCommand {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/shuffleXP" ); }
	virtual const char* GetCommandName( void ) const { return "shufflexp"; }
	virtual const char* GetAdminCommand( void ) const { return "shuffleteams"; }
	virtual const char* GetAdminCommandSuffix( void ) const { return "xp"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/shuffleXP/text"; }
};

class sdCallVoteShuffleRandom : public sdCallVoteAdminCommand {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/shuffleRandom" ); }
	virtual const char* GetCommandName( void ) const { return "shufflerandom"; }
	virtual const char* GetAdminCommand( void ) const { return "shuffleteams"; }
	virtual const char* GetAdminCommandSuffix( void ) const { return "random"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/shuffleRandom/text"; }
};

class sdCallVoteSwapTeams : public sdCallVoteAdminCommand {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/swapTeams" ); }
	virtual const char* GetCommandName( void ) const { return "swapteams"; }
	virtual const char* GetAdminCommand( void ) const { return "shuffleteams"; }
	virtual const char* GetAdminCommandSuffix( void ) const { return "swap"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/swapTeams/text"; }
	virtual bool AllowedOnRankedServer( void ) const { return false; }
};

class sdCallVoteRestartMap : public sdCallVoteAdminCommand {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/restartMap" ); }
	virtual const char* GetCommandName( void ) const { return "restartmap"; }
	virtual const char* GetAdminCommand( void ) const { return "restartMap"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/restartMap/text"; }
};





class sdCallVoteCVar : public sdCallVote {
public:
	struct data_t {
		const char* text;
		int i;
	};

	class sdCallVoteCVarFinalizer : public sdVoteFinalizer {
	public:
		sdCallVoteCVarFinalizer( const char* _cvarName, data_t& _data ) {
			cvarName = _cvarName;
			data = _data;
		}

		virtual void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			cvarSystem->SetCVarInteger( cvarName, data.i );
		}

	private:
		const char* cvarName;
		data_t data;
	};

	virtual void EnumerateOptions( sdUIList* list ) const {
		for ( int i = 0; i < GetCount(); i++ ) {
			AddVoteOption( list, common->LocalizeText( GetData()[ i ].text ).c_str() );
		}
	}

	virtual void Execute( int dataKey ) const {
		idCmdArgs args;
		CreateCmdArgs( args );
		args.AppendArg( va( "%d", dataKey ) );
		sdVoteManagerLocal::Callvote_f( args );
	}

	virtual sdPlayerVote* Execute( const idCmdArgs& args, idPlayer* player ) const {
		if ( args.Argc() < 3 ) {
			return NULL;
		}

		int value = atoi( args.Argv( 2 ) );
		if ( value < 0 || value >= GetCount() ) {
			return NULL;
		}

		data_t& data = GetData()[ value ];

		sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
		if ( vote != NULL ) {
			vote->MakeGlobalVote();
			vote->SetText( gameLocal.declToolTipType[ GetVoteTextKey() ] );
			vote->AddTextParm( declHolder.declLocStrType[ data.text ] );
			vote->SetFinalizer( new sdCallVoteCVarFinalizer( GetCVarName(), data ) );
		}

		return vote;
	}

	virtual data_t* GetData( void ) const = 0;
	virtual int GetCount( void ) const = 0;

	virtual const char* GetCVarName( void ) const = 0;
	virtual const char* GetVoteTextKey( void ) const = 0;
};


sdCallVoteCVar::data_t g_sdCallVoteCVarOnOff_Data[] = {
	{ "votes/values/on", 1 },
	{ "votes/values/off", 0 },
};

class sdCallVoteCVarOnOff : public sdCallVoteCVar {
public:
	virtual data_t* GetData( void ) const { return g_sdCallVoteCVarOnOff_Data; }
	virtual int GetCount( void ) const { return _arraycount( g_sdCallVoteCVarOnOff_Data ); }
};

class sdCallVoteBalancedTeams : public sdCallVoteCVarOnOff {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/balancedTeams" ); }
	virtual const char* GetCommandName( void ) const { return "balancedteams"; }
	virtual const char* GetCVarName( void ) const { return "si_teamForceBalance"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/balancedTeams/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
	virtual bool		AllowedOnRankedServer( void ) const { return false; }
};

class sdCallVoteTeamDamage : public sdCallVoteCVarOnOff {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/teamDamage" ); }
	virtual const char* GetCommandName( void ) const { return "teamdamage"; }
	virtual const char* GetCVarName( void ) const { return "si_teamDamage"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/teamDamage/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
	virtual bool		AllowedOnRankedServer( void ) const { return false; }
};

class sdCallVoteNoXP : public sdCallVoteCVarOnOff {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/noXP" ); }
	virtual const char* GetCommandName( void ) const { return "noxp"; }
	virtual const char* GetCVarName( void ) const { return "si_noProficiency"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/noXP/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
	virtual bool		AllowedOnRankedServer( void ) const { return false; }
};




sdCallVoteCVar::data_t g_sdCallVoteTimeLimit_Data[] = {
	{ "votes/values/5", 5 },
	{ "votes/values/10", 10 },
	{ "votes/values/15", 15 },
	{ "votes/values/20", 20 },
	{ "votes/values/25", 25 },
	{ "votes/values/30", 30 },
};

class sdCallVoteTimeLimit : public sdCallVoteCVar {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/timelimit" ); }
	virtual const char* GetCommandName( void ) const { return "timelimit"; }
	virtual const char* GetCVarName( void ) const { return "si_timeLimit"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/timelimit/text"; }

	virtual data_t*		GetData( void ) const { return g_sdCallVoteTimeLimit_Data; }
	virtual int			GetCount( void ) const { return _arraycount( g_sdCallVoteTimeLimit_Data ); }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
	virtual bool		AllowedOnRankedServer( void ) const { return false; }
};






class sdCallVoteServerMode : public sdCallVote {
public:
	class sdCallVoteServerModeFinalizer : public sdVoteFinalizer {
	public:
		sdCallVoteServerModeFinalizer( const char* _config ) {
			config = _config;
		}

		virtual void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			idCmdArgs args;
			args.AppendArg( "admin" );
			args.AppendArg( "execConfig" );
			args.AppendArg( config.c_str() );
			sdAdminSystem::GetInstance().PerformCommand( args, NULL );
		}

	private:
		idStr config;
	};

	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/serverMode" ); }
	virtual const char* GetCommandName( void ) const { return "servermode"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ "votes/serverMode/text" ]; }

	virtual void EnumerateOptions( sdUIList* list ) const {
		const idDict& configs = sdUserGroupManager::GetInstance().GetConfigs();
		for ( int i = 0; i < configs.GetNumKeyVals(); i++ ) {
			AddVoteOption( list, va( L"%hs", configs.GetKeyVal( i )->GetKey().c_str() ) );
		}
	}

	virtual void Execute( int dataKey ) const {
		const idDict& configs = sdUserGroupManager::GetInstance().GetConfigs();
		if ( dataKey < 0 || dataKey >= configs.GetNumKeyVals() ) {
			return;
		}

		idCmdArgs args;
		CreateCmdArgs( args );
		args.AppendArg( configs.GetKeyVal( dataKey )->GetKey().c_str() );
		sdVoteManagerLocal::Callvote_f( args );
	}

	virtual sdPlayerVote* Execute( const idCmdArgs& args, idPlayer* player ) const {
		if ( args.Argc() < 3 ) {
			return NULL;
		}

		const char* configName = args.Argv( 2 );

		const idDict& configs = sdUserGroupManager::GetInstance().GetConfigs();
		if ( configs.FindKey( configName ) == NULL ) {
			return NULL;
		}

		sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
		if ( vote != NULL ) {
			vote->MakeGlobalVote();
			vote->SetText( gameLocal.declToolTipType[ "votes/serverMode/text" ] );
			vote->AddTextParm( va( L"%hs", configName ) ); 
			vote->SetFinalizer( new sdCallVoteServerModeFinalizer( configName ) );
		}

		return vote;
	}
};


class sdPlayerCallVote : public sdCallVote {
public:
	class sdPlayerCallVoteFinalizer : public sdVoteFinalizer {
	public:
		sdPlayerCallVoteFinalizer( idPlayer* _player, const char* _command, const char* _suffix ) {
			player = _player;
			command = _command;
			suffix = _suffix;
		}

		virtual void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			if ( !player.IsValid() ) {
				return;
			}

			idCmdArgs args;
			args.AppendArg( "admin" );
			args.AppendArg( command );
			args.AppendArg( player->userInfo.cleanName.c_str() );
			if ( suffix != NULL ) {
				args.AppendArg( suffix );
			}
			sdAdminSystem::GetInstance().PerformCommand( args, NULL );
		}

	private:
		idEntityPtr< idPlayer > player;
		const char* command;
		const char* suffix;
	};

	virtual void EnumerateOptions( sdUIList* list ) const {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL || gameLocal.IsLocalPlayer( player ) ) {
				continue;
			}

			AddVoteOption( list, va( L"%hs", player->userInfo.name.c_str() ), gameLocal.GetSpawnId( player ) );
		}
	}

	virtual void Execute( int dataKey ) const {
		idCmdArgs args;
		CreateCmdArgs( args );
		args.AppendArg( va( "%d", dataKey ) );
		sdVoteManagerLocal::Callvote_f( args );
	}

	virtual sdPlayerVote* Execute( const idCmdArgs& args, idPlayer* player ) const {
		if ( args.Argc() < 3 ) {
			return NULL;
		}

		int spawnId = atoi( args.Argv( 2 ) );
		idPlayer* other = gameLocal.EntityForSpawnId( spawnId )->Cast< idPlayer >();
		if ( other == NULL || other == player ) {
			return NULL;
		}

		sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
		if ( vote != NULL ) {
			vote->MakeGlobalVote();
			vote->SetText( gameLocal.declToolTipType[ GetVoteTextKey() ] );
			vote->AddTextParm( va( L"%hs", other->userInfo.name.c_str() ) ); 
			vote->SetFinalizer( new sdPlayerCallVoteFinalizer( other, GetAdminCommand(), GetAdminCommandSuffix() ) );
		}

		return vote;
	}

	virtual const char* GetCommandName( void ) const = 0;
	virtual const char* GetAdminCommand( void ) const = 0;
	virtual const char* GetAdminCommandSuffix( void ) const { return NULL; }
	virtual const char* GetVoteTextKey( void ) const = 0;
};

class sdCallVoteKickPlayer : public sdPlayerCallVote {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/kickplayer" ); }
	virtual const char* GetCommandName( void ) const { return "kickplayer"; }
	virtual const char* GetAdminCommand( void ) const { return "kick"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/kickplayer/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
};

class sdCallVoteMutePlayer : public sdPlayerCallVote {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/mute" ); }
	virtual const char* GetCommandName( void ) const { return "muteplayer"; }
	virtual const char* GetAdminCommand( void ) const { return "playerMute"; }
	virtual const char* GetAdminCommandSuffix( void ) const { return "on"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/mute/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
};

class sdCallVoteMuteVoicePlayer : public sdPlayerCallVote {
public:
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/mutevoice" ); }
	virtual const char* GetCommandName( void ) const { return "muteplayervoip"; }
	virtual const char* GetAdminCommand( void ) const { return "playerVOIPMute"; }
	virtual const char* GetAdminCommandSuffix( void ) const { return "on"; }
	virtual const char* GetVoteTextKey( void ) const { return "votes/mutevoice/text"; }
	virtual void		PreCache( void ) const { gameLocal.declToolTipType[ GetVoteTextKey() ]; }
};

class sdCallVoteMapChange : public sdCallVote {
public:
	static const idDict* sdCallVoteMapChange::GetMapData( const char* name ) {
		const metaDataContext_t* metaData = gameLocal.mapMetaDataList->FindMetaDataContext( name );
		if ( metaData == NULL ) {
			return NULL;
		}

		if ( !gameLocal.IsMetaDataValidForPlay( *metaData, true ) ) {
			return NULL;
		}

		return metaData->meta;
	}

	static const idDict* sdCallVoteMapChange::GetMapData( int index ) {
		if( index < 0 || index > gameLocal.mapMetaDataList->GetNumMetaData() ) {
			return NULL;
		}
		const metaDataContext_t& metaData = gameLocal.mapMetaDataList->GetMetaDataContext( index );
		if ( !gameLocal.IsMetaDataValidForPlay( metaData, true ) ) {
			return NULL;
		}

		return metaData.meta;
	}

	static const sdDeclMapInfo* sdCallVoteMapChange::GetMapInfo( const char* name ) {
		const idDict* mapData = GetMapData( name );
		if ( mapData == NULL ) {
			return NULL;
		}

		const char* mapInfoName = mapData->GetString( "mapinfo" );
		if ( mapInfoName[ 0 ] == '\0' ) {
			return NULL;
		}

		return gameLocal.declMapInfoType[ mapInfoName ];
	}

	static const sdDeclMapInfo* sdCallVoteMapChange::GetMapInfo( int index ) {
		const idDict* mapData = GetMapData( index );
		if ( mapData == NULL ) {
			return NULL;
		}

		const char* mapInfoName = mapData->GetString( "mapinfo" );
		if ( mapInfoName[ 0 ] == '\0' ) {
			return NULL;
		}

		return gameLocal.declMapInfoType[ mapInfoName ];
	}

	class sdCallVoteMapChangeFinalizer : public sdVoteFinalizer {
	public:
		sdCallVoteMapChangeFinalizer( const char* _mapName, const char* _mode ) {
			mapName = _mapName;
			mode = _mode;
		}

		virtual void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			idCmdArgs args;
			args.AppendArg( "admin" );
			args.AppendArg( mode );
			args.AppendArg( mapName.c_str() );
			sdAdminSystem::GetInstance().PerformCommand( args, NULL );
		}

	private:
		idStr mapName;
		const char* mode;
	};

	virtual void sdCallVoteMapChange::EnumerateOptions( sdUIList* list ) const {
		for ( int i = 0; i < gameLocal.mapMetaDataList->GetNumMetaData(); i++ ) {
			const idDict* mapInfo = GetMapData( i );
			if ( mapInfo == NULL ) {
				continue;
			}

			const char* materialName = mapInfo->GetString( "server_shot_thumb", "levelshots/thumbs/generic.tga" );
			AddVoteOption( list, va( L"%hs\t%hs\t1.7777", mapInfo->GetString( "pretty_name", "" ), materialName ), i );
		}
	}

	virtual void sdCallVoteMapChange::Execute( int dataKey ) const {
		idCmdArgs args;
		CreateCmdArgs( args );

		const char* name = GetMapData( dataKey )->GetString( "metadata_name" );

		args.AppendArg( name );
		sdVoteManagerLocal::Callvote_f( args );
	}

	virtual sdPlayerVote* sdCallVoteMapChange::Execute( const idCmdArgs& args, idPlayer* player ) const {
		if ( args.Argc() < 3 ) {
			return NULL;
		}

		const char* metaDataName = args.Argv( 2 );

		const idDict* mapData = GetMapData( metaDataName );
		if ( mapData == NULL ) {
			return NULL;
		}

		const sdDeclMapInfo* mapInfo = GetMapInfo( metaDataName );
		if ( mapInfo == NULL ) {
			assert( false );
			return NULL;
		}

		sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
		if ( vote != NULL ) {
			vote->MakeGlobalVote();
			vote->SetText( gameLocal.declToolTipType[ GetVoteTextKey() ] );
			vote->AddTextParm( va( L"%hs", mapData->GetString( "pretty_name", "" ) ) );
			vote->SetFinalizer( new sdCallVoteMapChangeFinalizer( mapData->GetString( "metadata_name" ), GetModeName() ) );
		}

		return vote;
	}

	virtual const char* GetVoteTextKey( void ) const = 0;
	virtual const char* GetModeName( void ) const = 0;

	virtual void		PreCache( void ) const { PreCache( this ); } 

	protected:
		static void PreCache( const sdCallVoteMapChange* command ) {
			gameLocal.declToolTipType[ command->GetVoteTextKey() ];
		}
};

class sdCallVoteObjectiveMap : public sdCallVoteMapChange {
public:
	virtual const char* GetCommandName( void ) const { return "setobjectivemap"; }
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/setobjectivemap" ); }	
	virtual const char* GetVoteTextKey( void ) const { return "votes/setobjectivemap/text"; }
	virtual const char* GetModeName( void ) const { return "setObjectiveMap"; }
	virtual bool AllowedOnRankedServer( void ) const { return false; }
};

class sdCallVoteStopWatchMap : public sdCallVoteMapChange {
public:
	virtual const char* GetCommandName( void ) const { return "setstopwatchmap"; }
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/setstopwatchmap" ); }	
	virtual const char* GetVoteTextKey( void ) const { return "votes/setstopwatchmap/text"; }
	virtual const char* GetModeName( void ) const { return "setStopWatchMap"; }
	virtual bool AllowedOnRankedServer( void ) const { return false; }
};







class sdCallVoteCampaignChange : public sdCallVote {
public:
	virtual const char* GetCommandName( void ) const { return "setcampaign"; }
	virtual void GetText( idWStr& out ) const { out = common->LocalizeText( "votes/setcampaign" ); }

	class sdCallVoteCampaignChangeFinalizer : public sdVoteFinalizer {
	public:
		sdCallVoteCampaignChangeFinalizer( const char* _campaignName ) {
			campaignName = _campaignName;
		}

		virtual void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			idCmdArgs args;
			args.AppendArg( "admin" );
			args.AppendArg( "setCampaign" );
			args.AppendArg( campaignName.c_str() );
			sdAdminSystem::GetInstance().PerformCommand( args, NULL );
		}

	private:
		idStr campaignName;
	};

	virtual void EnumerateOptions( sdUIList* list ) const {
		for( int i = 0; i < gameLocal.campaignMetaDataList->GetNumMetaData(); i++ ) {
			const metaDataContext_t& metaData = gameLocal.campaignMetaDataList->GetMetaDataContext( i );
			if ( !gameLocal.IsMetaDataValidForPlay( metaData, true ) ) {
				continue;
			}
			const idDict& dict = *metaData.meta;
			
			const char* materialName = dict.GetString( "server_shot_thumb" );
			const char* prettyName = dict.GetString( "pretty_name" );

			AddVoteOption( list, va( L"%hs\t%hs\t1.333", prettyName, materialName ), i );
		}
	}

	virtual void Execute( int dataKey ) const {
		if( dataKey < 0 || dataKey >= gameLocal.campaignMetaDataList->GetNumMetaData() ) {
			gameLocal.Warning( "sdCallVoteCampaignChange: index '%i' out of range", dataKey );
			return;
		}
		const metaDataContext_t& metaData = gameLocal.campaignMetaDataList->GetMetaDataContext( dataKey );
		if ( !gameLocal.IsMetaDataValidForPlay( metaData, true ) ) {
			return;
		}

		const idDict& dict = *metaData.meta;
		const sdDeclCampaign* campaign = gameLocal.declCampaignType.LocalFind( dict.GetString( "metadata_name" ), false );
		if ( campaign == NULL ) {
			return;
		}

		idCmdArgs args;
		CreateCmdArgs( args );
		args.AppendArg( campaign->GetName() );

		sdVoteManagerLocal::Callvote_f( args );
	}

	virtual sdPlayerVote* Execute( const idCmdArgs& args, idPlayer* player ) const {
		if ( args.Argc() < 3 ) {
			return NULL;
		}

		const char* campaignName = args.Argv( 2 );

		const metaDataContext_t* metaData = gameLocal.campaignMetaDataList->FindMetaDataContext( campaignName );
		if ( metaData == NULL ) {
			return NULL;
		}
		if ( !gameLocal.IsMetaDataValidForPlay( *metaData, true ) ) {
			return NULL;
		}

		const sdDeclCampaign* campaign = gameLocal.declCampaignType[ campaignName ];
		if ( campaign == NULL ) {
			return NULL;
		}

		sdPlayerVote* vote = sdVoteManager::GetInstance().AllocVote();
		if ( vote != NULL ) {
			vote->MakeGlobalVote();
			vote->SetText( gameLocal.declToolTipType[ "votes/setcampaign/text" ] );
			const idDict* dict = gameLocal.campaignMetaDataList->FindMetaData( campaignName, &gameLocal.defaultMetaData );
			vote->AddTextParm( va( L"%hs", dict->GetString( "pretty_name" ) ) );
			vote->SetFinalizer( new sdCallVoteCampaignChangeFinalizer( campaignName ) );
		}

		return vote;
	}

	virtual void		PreCache( void ) const { 
		gameLocal.declToolTipType[ "votes/setcampaign/text" ];
	}
};

/*
================
sdVoteModePrivate::sdVoteModePrivate
================
*/
sdVoteModePrivate::sdVoteModePrivate( idPlayer* other ) {
	client = other;
}

/*
================
sdVoteModePrivate::CanVote
================
*/
bool sdVoteModePrivate::CanVote( idPlayer* player ) const {
	return client == player;
}


/*
================
sdVoteModeTeam::sdVoteModeTeam
================
*/
sdVoteModeTeam::sdVoteModeTeam( sdTeamInfo* _team ) {
	team = _team;
}

/*
================
sdVoteModeTeam::CanVote
================
*/
bool sdVoteModeTeam::CanVote( idPlayer* player ) const {
	return player->GetTeam() == team;
}



/*
================
sdVoteModeGlobal::CanVote
================
*/
bool sdVoteModeGlobal::CanVote( idPlayer* player ) const {
	return true;
}





/*
================
sdVoteFinalizer_Script::sdVoteFinalizer_Script
================
*/
sdVoteFinalizer_Script::sdVoteFinalizer_Script( idScriptObject* object, const char* functionName ) : scriptObjectHandle( 0 ), function( NULL ) {
	if ( !object ) {
		gameLocal.Warning( "sdVoteFinalizer_Script::sdVoteFinalizer_Script No Scriptobject" );
		return;
	}

	scriptObjectHandle	= object->GetHandle();
	function			= object->GetFunction( functionName );

	if ( !function ) {
		gameLocal.Warning( "sdVoteFinalizer_Script::sdVoteFinalizer_Script Invalid Function '%s'", functionName );
		return;
	}
}

/*
================
sdVoteFinalizer_Script::OnVoteCompleted
================
*/
void sdVoteFinalizer_Script::OnVoteCompleted( bool passed ) const {
	idScriptObject* object = gameLocal.program->GetScriptObject( scriptObjectHandle );
	if ( !object || !function ) {
		return;
	}

	sdScriptHelper h1;
	h1.Push( passed ? 1.f : 0.f );
	object->CallNonBlockingScriptEvent( function, h1 );
}


/*
================
sdPlayerVote::sdPlayerVote
================
*/
sdPlayerVote::sdPlayerVote( void ) : index( -1 ), mode( NULL ), endTime( 0 ), finalizer( NULL ), 
									yesCount( 0 ), noCount( 0 ), totalCount( 0 ), text( NULL ), voteId( VI_NONE ), displayFinishedMessage( true ) {

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		votes[ i ] = VS_CANNOT_VOTE;
	}
	activeNode.SetOwner( this );
}

/*
================
sdPlayerVote::~sdPlayerVote
================
*/
sdPlayerVote::~sdPlayerVote( void ) {
	ClearMode();
	ClearFinalizer();
}

/*
================
sdPlayerVote::AddTextParm
================
*/
void sdPlayerVote::AddTextParm( const sdDeclLocStr* text ) {
	textParm_t& parm = textParms.Alloc();
	parm.locStr = text;
}

/*
================
sdPlayerVote::AddTextParm
================
*/
void sdPlayerVote::AddTextParm( const wchar_t* text ) {
	textParm_t& parm = textParms.Alloc();
	parm.text = text;
	parm.locStr = NULL;
}

/*
================
sdPlayerVote::MakeTeamVote
================
*/
void sdPlayerVote::MakeTeamVote( sdTeamInfo* team ) {
	ClearMode();
	mode = new sdVoteModeTeam( team );
	SetVoteFlags();
}

/*
================
sdPlayerVote::MakePrivateVote
================
*/
void sdPlayerVote::MakePrivateVote( idPlayer* client ) {
	ClearMode();
	mode = new sdVoteModePrivate( client );
	SetVoteFlags();
}

/*
================
sdPlayerVote::MakeGlobalVote
================
*/
void sdPlayerVote::MakeGlobalVote( void ) {
	ClearMode();
	mode = new sdVoteModeGlobal();
	SetVoteFlags();
}

/*
================
sdPlayerVote::ClearMode
================
*/
void sdPlayerVote::ClearMode( void ) {
	delete mode;
	mode = NULL;
}

/*
================
sdPlayerVote::ClearFinalizer
================
*/
void sdPlayerVote::ClearFinalizer( void ) {
	delete finalizer;
	finalizer = NULL;
}

/*
================
sdPlayerVote::SetVoteFlags
================
*/
void sdPlayerVote::SetVoteFlags( void ) {
	if ( gameLocal.isClient ) {
		if ( voteCaller.IsValid() && gameLocal.IsLocalPlayer( voteCaller ) ) {
			votes[ gameLocal.localClientNum ] = VS_YES;
		} else {
			votes[ gameLocal.localClientNum ] = VS_NOT_VOTED;
		}
		return;
	}

	assert( mode );

	totalCount = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player || player->GetType()->IsType( idBot::Type ) || !mode->CanVote( player ) ) {
			votes[ i ] = VS_CANNOT_VOTE;
		} else {
			totalCount++;
			votes[ i ] = VS_NOT_VOTED;
		}
	}
}

/*
================
sdPlayerVote::Start
================
*/
void sdPlayerVote::Start( idPlayer* player ) {
	int length = text ? text->GetLength() : SEC2MS( 30.f );
	endTime = gameLocal.ToGuiTime( gameLocal.time + length );
	sdVoteManager::GetInstance().MakeActive( activeNode );

	voteCaller = player;

	if ( text ) {
		finalText.Clear();
		if ( player != NULL ) {
			const sdDeclLocStr* locStr = declHolder.declLocStrType[ "votes/playercalled" ];
			if ( locStr != NULL ) {
				idWStrList parms( 1 ); parms.Append( va( L"%hs", player->GetUserInfo().name.c_str() ) );
				finalText = common->LocalizeText( locStr, parms );
			}
		}

		sdToolTipParms parms;
		for ( int i = 0; i < textParms.Num(); i++ ) {
			if ( textParms[ i ].locStr != NULL ) {
				parms.Add( textParms[ i ].locStr->GetText() );
			} else {
				parms.Add( textParms[ i ].text.c_str() );
			}
		}

		idWStr guiText;
		text->GetMessage( &parms, guiText );
		finalText.Append( guiText );
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_VOTE );
		msg.WriteBits( sdVoteManagerLocal::VM_CREATE, idMath::BitsForInteger( sdVoteManagerLocal::VM_NUM ) );
		msg.WriteBits( index, idMath::BitsForInteger( sdVoteManagerLocal::MAX_VOTES ) );
		if ( player != NULL ) {
			msg.WriteBits( 1, 1 );
			msg.WriteByte( player->entityNumber );
		} else {
			msg.WriteBits( 0, 1 );
			
		}

		msg.WriteBits( text ? text->Index() + 1 : 0, idMath::BitsForInteger( gameLocal.declToolTipType.Num() + 1 ) );
		msg.WriteLong( textParms.Num() );
		for ( int i = 0; i < textParms.Num(); i++ ) {
			if ( textParms[ i ].locStr != NULL ) {
				msg.WriteBool( false );
				msg.WriteLong( textParms[ i ].locStr->Index() );
			} else {
				msg.WriteBool( true );
				msg.WriteString( textParms[ i ].text.c_str() );
			}
		}

		msg.WriteLong( endTime );
		SendMessage( msg );
	}
}

/*
================
sdPlayerVote::Vote
================
*/
void sdPlayerVote::Vote( bool result ) {
	if ( result ) {
		yesCount++;
	} else {
		noCount++;
	}
}

/*
================
sdPlayerVote::Vote
================
*/
void sdPlayerVote::Vote( idPlayer* client, bool result ) {
	int clientIndex = client->entityNumber;

	if ( votes[ clientIndex ] != VS_NOT_VOTED ) {
		// already voted or not eligible
		// TODO: send message
		return;
	}

	if ( result ) {
		votes[ clientIndex ] = VS_YES;
		yesCount++;
	} else {
		votes[ clientIndex ] = VS_NO;
		noCount++;
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_VOTE );
		msg.WriteBits( sdVoteManagerLocal::VM_VOTE, idMath::BitsForInteger( sdVoteManagerLocal::VM_NUM ) );
		msg.WriteBits( index, idMath::BitsForInteger( sdVoteManagerLocal::MAX_VOTES ) );
		msg.WriteBool( result );
		SendMessage( msg );
	}

	// check if there are enough people left for it to be able to fail/pass
	int votesLeft		= totalCount - ( noCount + yesCount );
	float minYesPcnt	= ( yesCount ) / ( float )totalCount;
	float maxYesPcnt	= ( yesCount + votesLeft ) / ( float )totalCount;
	float passPcnt		= GetPassPercentage();

	if ( minYesPcnt >= passPcnt || maxYesPcnt < passPcnt ) {
		endTime = 0; // this will make us be marked as complete on next check
	}
}

/*
================
sdPlayerVote::ClientVote
================
*/
void sdPlayerVote::ClientVote( bool result ) {
	assert( gameLocal.isClient );

	votes[ gameLocal.localClientNum ] = result ? VS_YES : VS_NO;

	sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_VOTE );
	msg.WriteBits( GetIndex(), idMath::BitsForInteger( sdVoteManagerLocal::MAX_VOTES ) );
	msg.WriteLong( GetEndTime() );
	msg.WriteBool( result );
	msg.Send();
}

/*
================
sdPlayerVote::SendMessage
================
*/
void sdPlayerVote::SendMessage( sdReliableServerMessage& msg ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( votes[ i ] == VS_CANNOT_VOTE ) {
			continue;
		}

		msg.Send( sdReliableMessageClientInfo( i ) );
	}
}

/*
================
sdPlayerVote::Tag
================
*/
void sdPlayerVote::Tag( voteId_t id, idEntity* object ) {
	voteId		= id;
	voteObject	= object;
}

/*
================
sdPlayerVote::Match
================
*/
bool sdPlayerVote::Match( voteId_t id, idEntity* object ) {
	return voteId == id && voteObject == object;
}

idCVar g_votePassPercentage( "g_votePassPercentage", "51",	CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE | CVAR_RANKLOCKED,	"Percentage of yes votes required for a vote to pass", 0.f, 100.f );
idCVar g_minAutoVotePlayers( "g_minAutoVotePlayers", "0",	CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE,					"If this number of players has voted, then the vote will be considered valid even if the required % of yes/no votes has not been met" );

/*
================
sdPlayerVote::GetPassPercentage
================
*/
float sdPlayerVote::GetPassPercentage( void ) const {
	return g_votePassPercentage.GetFloat() / 100.f;
}

/*
================
sdPlayerVote::Complete
================
*/
void sdPlayerVote::Complete( void ) {
	bool passed;
	if ( g_minAutoVotePlayers.GetInteger() > 0 && ( yesCount + noCount ) >= g_minAutoVotePlayers.GetInteger() ) {
		passed = yesCount > noCount;
	} else {
		float frac = ( yesCount / ( float )totalCount );
		passed = frac >= GetPassPercentage();
	}

	if ( displayFinishedMessage ) {
		const sdDeclLocStr* finishedMessage = NULL;
		if ( passed ) {
			finishedMessage = declHolder.declLocStrType[ "votes/finished/success" ];
		} else {
			finishedMessage = declHolder.declLocStrType[ "votes/finished/failed" ];
		}

		if ( finishedMessage == NULL ) {
			gameLocal.Warning( "sdPlayerVote::Complete Missing Vote Complete Mission" );
		} else {
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( votes[ i ] == VS_CANNOT_VOTE ) {
					continue;
				}

				idPlayer* player = gameLocal.GetClient( i );
				if ( !player ) {
					continue;
				}

				player->SendLocalisedMessage( finishedMessage, idWStrList() );
			}
		}
	}

	if ( finalizer == NULL ) {
		gameLocal.Warning( "sdPlayerVote::Complete Vote completed with no Finalizer" );
		return;
	}

	finalizer->OnVoteCompleted( passed );
}

/*
================
sdPlayerVote::GetPlayerCanVote
================
*/
bool sdPlayerVote::GetPlayerCanVote( idPlayer* player ) const {
	return votes[ player->entityNumber ] == VS_NOT_VOTED;
}






/*
================
sdVoteManagerLocal::sdVoteManagerLocal
================
*/
sdVoteManagerLocal::sdVoteManagerLocal( void ) {
	memset( votes, 0, sizeof( votes ) );
}

/*
================
sdVoteManagerLocal::~sdVoteManagerLocal
================
*/
sdVoteManagerLocal::~sdVoteManagerLocal( void ) {
	globalCallVotes.DeleteContents( true );
}

/*
================
sdVoteManagerLocal::AllocVote
================
*/
sdPlayerVote* sdVoteManagerLocal::AllocVote( int index ) {
	if ( index == -1 ) {
		if ( gameLocal.isClient ) {
			gameLocal.Error( "sdVoteManagerLocal::AllocVote Tried to Create a Vote on a Network Client" );
			return NULL;
		}

		for ( int i = 0; i < MAX_VOTES; i++ ) {
			if ( votes[ i ] != NULL ) {
				continue;
			}
			index = i;
			break;
		}
	}

	if ( index == -1 ) {
		gameLocal.Error( "sdVoteManagerLocal::AllocVote Max Votes Hit" );
		return NULL;
	}
	
	votes[ index ] = new sdPlayerVote();
	votes[ index ]->Init( index );
	return votes[ index ];
}

/*
================
sdVoteManagerLocal::Think
================
*/
void sdVoteManagerLocal::Think( void ) {
	sdPlayerVote* next = activeVotes.Next();
	for ( sdPlayerVote* vote = next; vote; vote = next ) {
		next = vote->GetNextActiveVote();

		int endTime = vote->GetEndTime();
		if ( gameLocal.ToGuiTime( gameLocal.time ) < endTime ) {
			continue;
		}

		int index = vote->GetIndex();

		vote->Complete();

		FreeVote( index );
	}
}

/*
================
sdVoteManagerLocal::FreeVote
================
*/
void sdVoteManagerLocal::FreeVote( int index ) {
	if ( index < 0 || index > MAX_VOTES ) {
		assert( false );
		return;
	}

	sdPlayerVote* vote = votes[ index ];
	if( vote == NULL ) {
		return;
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_VOTE );
		msg.WriteBits( VM_FREE, idMath::BitsForInteger( VM_NUM ) );
		msg.WriteBits( index, idMath::BitsForInteger( sdVoteManagerLocal::MAX_VOTES ) );			
		vote->SendMessage( msg );
	}

	delete votes[ index ];
	votes[ index ] = NULL;
}

/*
================
sdVoteManagerLocal::AllocVote
================
*/
void sdVoteManagerLocal::FreeVotes( void ) {
	for ( int i = 0; i < MAX_VOTES; i++ ) {
		FreeVote( i );
	}
}

/*
================
sdVoteManagerLocal::Init
================
*/
void sdVoteManagerLocal::Init( void ) {
	globalCallVotes.Alloc() = new sdCallVoteObjectiveMap();
	globalCallVotes.Alloc() = new sdCallVoteStopWatchMap();
	globalCallVotes.Alloc() = new sdCallVoteCampaignChange();
	globalCallVotes.Alloc() = new sdCallVoteShuffleXP();
	globalCallVotes.Alloc() = new sdCallVoteShuffleRandom();
	globalCallVotes.Alloc() = new sdCallVoteSwapTeams();
	globalCallVotes.Alloc() = new sdCallVoteBalancedTeams();
	globalCallVotes.Alloc() = new sdCallVoteTeamDamage();
	globalCallVotes.Alloc() = new sdCallVoteNoXP();
	globalCallVotes.Alloc() = new sdCallVoteTimeLimit();
	globalCallVotes.Alloc() = new sdCallVoteServerMode();
	globalCallVotes.Alloc() = new sdCallVoteKickPlayer();
	globalCallVotes.Alloc() = new sdCallVoteMutePlayer();
	globalCallVotes.Alloc() = new sdCallVoteMuteVoicePlayer();
	for( int i = 0; i < globalCallVotes.Num(); i++ ) {
		globalCallVotes[ i ]->PreCache();
	}
}

/*
================
sdVoteManagerLocal::MakeActive
================
*/
void sdVoteManagerLocal::MakeActive( idLinkList< sdPlayerVote >& node ) {
	node.AddToEnd( activeVotes );
}

idCVar g_voteKeepVote( "g_voteKeepVote", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "Keep vote on the HUD after having voted" );

/*
================
sdVoteManagerLocal::GetActiveVote
================
*/
sdPlayerVote* sdVoteManagerLocal::GetActiveVote( idPlayer* p ) {
	if ( p == NULL ) {
		return NULL;
	}

	// get first vote player hasn't voted on
	for ( sdPlayerVote* vote = activeVotes.Next(); vote != NULL; vote = vote->GetNextActiveVote() ) {
		if ( !vote->GetPlayerCanVote( p ) ) {
			continue;
		}

		return vote;
	}

	// get first vote player is part of
	if ( g_voteKeepVote.GetBool() ) {
		for ( sdPlayerVote* vote = activeVotes.Next(); vote != NULL; vote = vote->GetNextActiveVote() ) {
			if ( vote->GetVoteState( p ) != sdPlayerVote::VS_CANNOT_VOTE ) {
				return vote;
			}
		}
	}

	return NULL;
}

/*
================
sdVoteManagerLocal::ClientReadNetworkMessage
================
*/
void sdVoteManagerLocal::ClientReadNetworkMessage( const idBitMsg& msg ) {
	idPlayer* player = NULL;

	voteMessage_t message = ( voteMessage_t )msg.ReadBits( idMath::BitsForInteger( VM_NUM ) );
	switch ( message ) {
		case VM_CREATE: {
			int index = msg.ReadBits( idMath::BitsForInteger( MAX_VOTES ) );
			FreeVote( index );

			if ( msg.ReadBits( 1 ) != 0 ) {
				int client = msg.ReadByte();
				if ( client >= 0 || client < MAX_CLIENTS ) {
					player = gameLocal.GetClient( client );
				}
			}

			sdPlayerVote* vote = AllocVote( index );

			int toolTipIndex = msg.ReadBits( idMath::BitsForInteger( gameLocal.declToolTipType.Num() ) );
			const sdDeclToolTip* toolTip = toolTipIndex ? gameLocal.declToolTipType[ toolTipIndex - 1 ] : NULL;
			vote->SetText( toolTip );

			int textCount = msg.ReadLong();
			for ( int i = 0; i < textCount; i++ ) {
				if ( !msg.ReadBool() ) {
					vote->AddTextParm( declHolder.declLocStrType.SafeIndex( msg.ReadLong() ) );
				} else {
					wchar_t buffer[ 128 ];
					msg.ReadString( buffer, sizeof( buffer ) / sizeof( wchar_t ) );
					vote->AddTextParm( buffer );
				}
			}

			vote->Start( player );
			vote->SetEndTime( msg.ReadLong() );
			vote->SetVoteFlags();
			
			break;
		}
		case VM_FREE: {
			int index = msg.ReadBits( idMath::BitsForInteger( MAX_VOTES ) );
			FreeVote( index );
			break;
		}
		case VM_VOTE: {
			int index = msg.ReadBits( idMath::BitsForInteger( MAX_VOTES ) );
			sdPlayerVote* v = votes[ index ];
			if ( v ) {
				v->Vote( msg.ReadBool() );
			}
			break;
		}
	}
}

/*
================
sdVoteManagerLocal::ServerReadNetworkMessage
================
*/
void sdVoteManagerLocal::ServerReadNetworkMessage( idPlayer* client, const idBitMsg& msg ) {
	int index = msg.ReadBits( idMath::BitsForInteger( MAX_VOTES ) );
	int endTime = msg.ReadLong();
	bool result = msg.ReadBool();

	sdPlayerVote* v = votes[ index ];
	if ( !v ) {
		return;
	}

	if ( v->GetEndTime() != endTime ) {
		return; // try to make sure we are talking about the same vote
	}

	v->Vote( client, result );
}

/*
============
sdVoteManagerLocal::PerformCommand
============
*/
void sdVoteManagerLocal::PerformCommand( const idCmdArgs& args, idPlayer* player ) {
	const char* cmd = args.Argv( 1 );

	sdCallVote* vote = NULL;

	for ( int i = 0; i < globalCallVotes.Num(); i++ ) {
		if ( idStr::Icmp( globalCallVotes[ i ]->GetCommandName(), cmd ) != 0 ) {
			continue;
		}

		vote = globalCallVotes[ i ];
		break;
	}

	if ( vote == NULL && gameLocal.rules != NULL ) {
		const idList< sdCallVote* >& ruleVotes = gameLocal.rules->GetCallVotes();
		for ( int i = 0; i < ruleVotes.Num(); i++ ) {
			if ( idStr::Icmp( ruleVotes[ i ]->GetCommandName(), cmd ) != 0 ) {
				continue;
			}

			vote = ruleVotes[ i ];
			break;
		}
	}

	if ( vote != NULL ) {
		if ( !CanUseVote( vote, player ) ) {
			return;
		}
		sdPlayerVote* playerVote = vote->Execute( args, player );
		if ( playerVote == NULL ) {
			return;
		}
		player->SetNextCallVoteTime( gameLocal.ToGuiTime( gameLocal.time + MINS2MS( g_voteWait.GetFloat() ) ) );
		playerVote->Start( player );
		if ( player != NULL ) {
			playerVote->Vote( player, true );
		}
	}
}

/*
============
sdVoteManagerLocal::CancelVote
============
*/
void sdVoteManagerLocal::CancelVote( voteId_t id, idEntity* object ) {
	while ( sdPlayerVote* vote = FindVote( id, object ) ) {
		FreeVote( vote->GetIndex() );
	}
}

/*
============
sdVoteManagerLocal::CancelVote
============
*/
void sdVoteManagerLocal::CancelVote( sdPlayerVote* vote ) {
	FreeVote( vote->GetIndex() );
}

/*
============
sdVoteManagerLocal::CancelFireTeamVotesForPlayer
============
*/
void sdVoteManagerLocal::CancelFireTeamVotesForPlayer( idPlayer *player ) {
	CancelVote( VI_FIRETEAM_CREATE, player );
	CancelVote( VI_FIRETEAM_JOIN, player );
	CancelVote( VI_FIRETEAM_REQUEST, player );
	CancelVote( VI_FIRETEAM_PROPOSE, player );
	CancelVote( VI_FIRETEAM_INVITE, player );
}

/*
============
sdVoteManagerLocal::FindVote
============
*/
sdPlayerVote* sdVoteManagerLocal::FindVote( voteId_t id ) {
	for ( sdPlayerVote* vote = activeVotes.Next(); vote; vote = vote->GetNextActiveVote() ) {
		if ( vote->GetVoteId() != id ) {
			continue;
		}
		return vote;
	}
	return NULL;
}

/*
============
sdVoteManagerLocal::FindVote
============
*/
sdPlayerVote* sdVoteManagerLocal::FindVote( voteId_t id, idEntity* object ) {
	for ( sdPlayerVote* vote = activeVotes.Next(); vote; vote = vote->GetNextActiveVote() ) {
		if ( !vote->Match( id, object ) ) {
			continue;
		}
		return vote;
	}
	return NULL;
}

/*
============
sdVoteManagerLocal::CreateCallVoteList
============
*/
void sdVoteManagerLocal::CreateCallVoteList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdVoteManagerLocal& manager = sdVoteManager::GetInstance();
	manager.EnumerateCallVotes( list );
}

/*
============
sdVoteManagerLocal::CreateCallVoteOptionList
============
*/
void sdVoteManagerLocal::CreateCallVoteOptionList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	int index;
	list->GetUI()->PopScriptVar( index );

	sdCallVote* vote = sdVoteManager::GetInstance().CallVoteForIndex( index );
	if ( vote == NULL ) {
		return;
	}

	vote->EnumerateOptions( list );
}

/*
============
sdVoteManagerLocal::EnumerateCallVotes
============
*/
void sdVoteManagerLocal::EnumerateCallVotes( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	int voteIndex = 0;
	for ( int i = 0; i < globalCallVotes.Num(); i++, voteIndex++ ) {
		if( !globalCallVotes[ i ]->AllowedOnRankedServer() && networkSystem->IsRankedServer() ) {
			continue;
		}
		PushVoteItem( list, globalCallVotes[ i ], voteIndex );
	}

	if ( gameLocal.rules != NULL ) {
		const idList< sdCallVote* >& ruleVotes = gameLocal.rules->GetCallVotes();
		for ( int i = 0; i < ruleVotes.Num(); i++, voteIndex++ ) {
			if( !globalCallVotes[ i ]->AllowedOnRankedServer() && networkSystem->IsRankedServer() ) {
				continue;
			}
			PushVoteItem( list, ruleVotes[ i ], voteIndex );			
		}
	}
}

/*
============
sdVoteManagerLocal::ExecVote
============
*/
void sdVoteManagerLocal::ExecVote( sdUserInterfaceLocal* ui ) {
	int voteIndex;
	ui->PopScriptVar( voteIndex );

	int voteKey;
	ui->PopScriptVar( voteKey );

	sdCallVote* vote = CallVoteForIndex( voteIndex );
	if ( vote == NULL ) {
		return;
	}

	vote->Execute( voteKey );
}

/*
============
sdVoteManagerLocal::CanUseVote
============
*/
bool sdVoteManagerLocal::CanUseVote( sdCallVote* vote, idPlayer* player ) {
	if ( gameLocal.serverInfoData.votingDisabled ) {
		return false;
	}

	if ( !vote->AllowedOnRankedServer() ) {
		if ( networkSystem->IsRankedServer() ) {
			return false;
		}
	}

	if( player == NULL ) {
		return false;
	}

	const sdUserGroup& group = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
	int voteLevel = sdUserGroupManager::GetInstance().GetVoteLevel( vote->GetCommandName() );
	return gameLocal.ToGuiTime( gameLocal.time ) > player->GetNextCallVoteTime() && group.CanCallVote( voteLevel );
}

/*
============
sdVoteManagerLocal::NumActiveVotes
============
*/
int sdVoteManagerLocal::NumActiveVotes( voteId_t id, idEntity* player ) const {
	assert( gameLocal.isServer );

	int count = 0;

	sdPlayerVote* next = activeVotes.Next();
	for ( sdPlayerVote* vote = next; vote; vote = next ) {
		next = vote->GetNextActiveVote();

		if ( vote->GetVoteCaller() == player && vote->GetVoteId() == id ) {
			count++;
		}
	}

	return count;
}
/*
============
sdVoteManagerLocal::PushVoteItem
============
*/
void sdVoteManagerLocal::PushVoteItem( sdUIList* list, sdCallVote* vote, int voteIndex ) {
	bool canUse = CanUseVote( vote, gameLocal.GetLocalPlayer() );
	if( !canUse ) {
		return;
	}

	idWStr temp;
	vote->GetText( temp );
	int index = sdUIList::InsertItem( list, temp.c_str(), -1, 0 );
	list->SetItemDataInt( voteIndex, index, 0 );
}

/*
============
sdVoteManagerLocal::CallVoteForIndex
============
*/
sdCallVote* sdVoteManagerLocal::CallVoteForIndex( int index ) {
	if ( index < 0 ) {
		return NULL;
	}

	if ( index < globalCallVotes.Num() ) {
		return globalCallVotes[ index ];
	}
	index -= globalCallVotes.Num();

	if ( gameLocal.rules != NULL ) {
		const idList< sdCallVote* >& ruleVotes = gameLocal.rules->GetCallVotes();
		if ( index < ruleVotes.Num() ) {
			return ruleVotes[ index ];
		}
		index -= ruleVotes.Num();
	}

	return NULL;
}

/*
==================
sdVoteManagerLocal::Callvote_f
==================
*/
void sdVoteManagerLocal::Callvote_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_CALLVOTE );
		msg.WriteLong( args.Argc() - 1 );
		for ( int i = 1; i < args.Argc(); i++ ) {
			msg.WriteString( args.Argv( i ) );
		}
		msg.Send();
		return;
	}

	sdVoteManager::GetInstance().PerformCommand( args, gameLocal.GetLocalPlayer() );
}

/*
==================
sdVoteManagerLocal::ListVotes
==================
*/
void sdVoteManagerLocal::ListVotes( void ) {
	int index = 0;
	while ( true ) {
		sdCallVote* vote = CallVoteForIndex( index++ );
		if ( vote == NULL ) {
			break;
		}

		const char* cmdName = vote->GetCommandName();
		int voteLevel = sdUserGroupManager::GetInstance().GetVoteLevel( cmdName );

		gameLocal.Printf( "\"%s\"\t\"%d\"\n", cmdName, voteLevel );
	}
}
