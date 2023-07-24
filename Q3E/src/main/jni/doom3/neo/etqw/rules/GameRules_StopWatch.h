// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_RULES_GAMERULES_STOPWATCH_H__
#define	__GAME_RULES_GAMERULES_STOPWATCH_H__

#include "GameRules.h"

class sdGameRulesStopWatchNetworkState : public sdGameRulesNetworkState {
public:
								sdGameRulesStopWatchNetworkState( void );

	void						MakeDefault( void );

	void						Write( idFile* file ) const;
	void						Read( idFile* file );

	int							progression;
	int							timeToBeat;
	sdTeamInfo*					winningTeam;
	int							winReason;
};

class sdGameRulesStopWatch : public sdGameRules {
public:
	CLASS_PROTOTYPE( sdGameRulesStopWatch );

	enum gameProgression_t {
		GP_FIRST_MATCH = 0,
		GP_RETURN_MATCH,
		GP_MAX
	};

	enum probeGameStateSW_t {
		PGS_RETURN	= BITT< sdGameRules::PGS_NEXT_BIT >::VALUE,
	};

	enum winReason_t {
		WR_NORMAL = 0,
		WR_SPEED,
		WR_XP
	};

										sdGameRulesStopWatch( void );
	virtual								~sdGameRulesStopWatch( void );

	virtual void						Clear( void );
	virtual void						EndGame( void );

	virtual sdTeamInfo*					GetWinningTeam( void ) const { return winningTeam; }
	virtual void						SetWinner( sdTeamInfo* team );

	virtual const sdDeclLocStr*			GetWinningReason( void ) const;

	virtual void						ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback );

	virtual void						ApplyNetworkState( const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( void ) const;

	virtual byte						GetProbeState( void ) const;
	virtual int							GetServerBrowserScore( const sdNetSession& session ) const;
	virtual	void						GetBrowserStatusString( idWStr& str, const sdNetSession& netSession ) const;


	virtual bool						ChangeMap( const char* mapName );
	virtual userMapChangeResult_e		OnUserStartMap( const char* text, idStr& reason, idStr& mapName );

	virtual int							GetGameTime( void ) const;

	virtual const sdDeclLocStr*			GetTypeText( void ) const;

	virtual void						Reset( void );

	virtual int							GetTimeLimit( void ) const;
	virtual void						OnTimeLimitHit( void );

	virtual void						UpdateClientFromServerInfo( const idDict& serverInfo, bool allowMedia );

	virtual bool						InhibitEntitySpawn( idDict &spawnArgs ) const;
	virtual const char*					GetKeySuffix( void ) const { return "_sw"; }

	virtual void						SetAttacker( sdTeamInfo* team );
	virtual void						SetDefender( sdTeamInfo* team );
	virtual void						OnObjectiveCompletion( int objectiveIndex );

	virtual const char*					GetDemoNameInfo( void );

protected:
	virtual void						GameState_Review( void );
	virtual void						GameState_NextGame( void );
	virtual void						GameState_Warmup( void );	
	virtual void						GameState_Countdown( void );
	virtual void						GameState_GameOn( void );
	virtual void						GameState_NextMap( void );

	virtual void						OnGameState_Review( void );
	virtual void						OnGameState_Countdown( void );
	virtual void						OnGameState_GameOn( void );
	virtual void						OnGameState_NextMap( void );

private:
	sdTeamInfo*							winningTeam;
	sdTeamInfo*							firstMatchWinningTeam;
	winReason_t							winReason;

	gameProgression_t					progression;
	int									timeToBeat;
	bool								clockBeaten;

	void								ResetTiebreakInfo( void );

	idList< int >						objectiveCompletionTimes[ GP_MAX ];
	sdTeamInfo*							attackingTeams[ GP_MAX ];
	sdTeamInfo*							defendingTeams[ GP_MAX ];
	int									attackingTeamXPs[ GP_MAX ];
};

#endif // __GAME_RULES_GAMERULES_STOPWATCH_H__
