// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_RULES_GAMERULES_CAMPAIGN_H__
#define	__GAME_RULES_GAMERULES_CAMPAIGN_H__

class sdDeclCampaign;

#include "GameRules.h"

class sdGameRulesCampaignNetworkState : public sdGameRulesNetworkState {
public:
								sdGameRulesCampaignNetworkState( void );

	void						MakeDefault( void );

	void						Write( idFile* file ) const;
	void						Read( idFile* file );

	sdTeamInfo*					winningTeam;
	sdTeamInfo*					campaignWinningTeam;
};

class sdGameRulesCampaign : public sdGameRules {
public:
	CLASS_PROTOTYPE( sdGameRulesCampaign );

	enum networkEvent_t {
		EVENT_MAPSTATS = sdGameRules::MAX_NET_EVENTS,
		EVENT_SETCAMPAIGN,
		MAX_NET_EVENTS,
	};

							sdGameRulesCampaign( void );
	virtual					~sdGameRulesCampaign( void );

	virtual void			Clear( void );
	virtual void			EndGame( void );
	virtual void			SetWinner( sdTeamInfo* team );
	
	void					SetCampaignWinner( sdTeamInfo* team ) { campaignWinningTeam = team; }

	virtual void			GetBrowserStatusString( idWStr& str, const sdNetSession& netSession ) const;

	virtual int				GetGameTime( void ) const;

	virtual sdTeamInfo*		GetWinningTeam( void ) const { return winningTeam; }
	sdTeamInfo*				GetCampaignWinner( void ) const { return campaignWinningTeam; }

	virtual void			WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target );

	virtual void			ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback );

	virtual void						ApplyNetworkState( const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( void ) const;

	virtual bool			ParseNetworkMessage( int msgType, const idBitMsg& msg );

	virtual userMapChangeResult_e	OnUserStartMap( const char* text, idStr& reason, idStr& mapName );

	virtual int					GetNumMaps( void ) const		{ return campaignMapData.Num(); }
	virtual const mapData_t*	GetMapData( int index ) const	{ return &campaignMapData[ index ]; }
	virtual const mapData_t*	GetCurrentMapData() const;

	virtual teamMapData_t*	GetTeamData( int team, int data ) { return NULL; }

	virtual void			InitVotes( void );
	virtual const sdDeclLocStr*	GetTypeText( void ) const;

	void					SendCampaignInfo( const sdReliableMessageClientInfoBase& target );
	virtual void			ReadCampaignInfo( const idBitMsg& msg );

	void					SetCampaign( const sdDeclCampaign* newCampaign );
	const sdDeclCampaign*	GetCampaign( void ) const { return campaignDecl; }
	void					StartMap( void );

	void					OnMapStatsReceived( int index );

	virtual int				GetServerBrowserScore( const sdNetSession& session ) const;

	virtual void			UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia );

	virtual const char*		GetDemoNameInfo( void ) { return "campaign"; }

protected:
	void					SendMapStats( int index, const sdReliableMessageClientInfoBase& target );
	void					ReadMapStats( const idBitMsg& msg );
	bool					SetCampaign( const char* text, idStr& mapName );

protected:
	virtual void			GameState_Review( void );
	virtual void			GameState_NextGame( void );
	virtual void			GameState_Warmup( void );	
	virtual void			GameState_Countdown( void );
	virtual void			GameState_GameOn( void );
	virtual void			GameState_NextMap( void );

	virtual void			OnGameState_Review( void );
	virtual void			OnGameState_Countdown( void );
	virtual void			OnGameState_GameOn( void );
	virtual void			OnGameState_NextMap( void );

protected:
	int						currentMapIndex;
	const sdDeclCampaign*	campaignDecl;
	sdTeamInfo*				winningTeam;
	sdTeamInfo*				campaignWinningTeam;

	idList< mapData_t >		campaignMapData;
};

#endif // __GAME_RULES_GAMERULES_CAMPAIGN_H__
