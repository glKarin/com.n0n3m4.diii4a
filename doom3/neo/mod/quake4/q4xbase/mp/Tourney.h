//----------------------------------------------------------------
// Tourney.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __TOURNEY_H__
#define __TOURNEY_H__

#include "../Game_local.h"

enum arenaState_t {
	AS_INACTIVE = 0,
	AS_WARMUP,
	AS_ROUND,
	AS_SUDDEN_DEATH,
	AS_DONE
};

#define MAX_TOURNEY_HISTORY_NAME_LEN 32

typedef struct arenaOutcome_s {
	// for clients that have disconnected, copy their names for history purposes
	char	playerOne[ MAX_TOURNEY_HISTORY_NAME_LEN ];
	char	playerTwo[ MAX_TOURNEY_HISTORY_NAME_LEN ];

	// for currently connected clients, use clientnum to get current name
	int		playerOneNum;
	int		playerTwoNum;

	int playerOneScore;
	int playerTwoScore;
} arenaOutcome_t;

// shouldn't exceed MAX_INSTANCES from idMultiplayerGame
const int MAX_ARENAS = 8;

const int MAX_ROUNDS = 4;

class rvTourneyArena {
public:
	rvTourneyArena();
	
	void			AddPlayers( idPlayer* playerOne, idPlayer* playerTwo );
	void			ClearPlayers( idPlayer* clearPlayer = NULL );
	void			Clear( bool respawnPlayers = true );
	void			Ready( void );

	idPlayer*		GetLeader( void );
	idPlayer*		GetLoser( void );
	idPlayer*		GetWinner( void ) { return winner; }
	void			UpdateState( void );
	void			NewState( arenaState_t newState );
	
	idPlayer**		GetPlayers( void );

	void			SetArenaID( int id );
	int				GetArenaID( void ) const;

	arenaState_t	GetState( void ) const;
	void			SetState( arenaState_t newState );
	void			SetNextStateTime( int time );
	int				GetNextStateTime( void );
	int				GetMatchStartTime( void ) { return matchStartTime; }

	void			PackState( idBitMsg& outMsg );
	void			UnpackState( const idBitMsg& inMsg );

	void			RemovePlayer( idPlayer* player );
	bool			TimeLimitHit( void );
	bool			IsPlaying( idPlayer* player ) { return ( arenaState != AS_INACTIVE && arenaState != AS_DONE && ( player == players[ 0 ] || player == players[ 1 ] ) ); }
	bool			HasPlayer( idPlayer* player ) { return ( player == players[0] || player == players[1] ); }
	bool			IsPlaying( void ) { return ( arenaState != AS_INACTIVE && arenaState != AS_DONE ); }

	const char*		GetPlayerName( int player );
	int				GetPlayerScore( int player );
	int				GetFraglimitTimeout( void ) { return fragLimitTimeout; }
	bool			operator==( const rvTourneyArena& rhs ) const;
	bool			operator!=( const rvTourneyArena& rhs ) const;
	rvTourneyArena& operator=( const rvTourneyArena& rhs );

private:
	// players			- players in arena
	idPlayer*			players[ 2 ];	
	// arenaID			- this arena's ID
	int					arenaID;
	// arenaState		- state of the arena
	arenaState_t		arenaState;
	// nextStateTime	- transition time to next state
	int					nextStateTime;
	// winner			- the winner of the arena
	idPlayer*			winner;
	// fragLimitTimeout	- timeout to let death anims play
	int					fragLimitTimeout;
	// matchStartTime	- time arena started
	int					matchStartTime;
};

ID_INLINE idPlayer** rvTourneyArena::GetPlayers( void ) {
	return players;
}

ID_INLINE void rvTourneyArena::SetArenaID( int id ) {
	arenaID = id;
}

ID_INLINE int rvTourneyArena::GetArenaID( void ) const {
	return arenaID;
}

ID_INLINE bool rvTourneyArena::operator==( const rvTourneyArena& rhs ) const {
	return ( arenaState == rhs.arenaState && players[ 0 ] == rhs.players[ 0 ] && players[ 1 ] == rhs.players[ 1 ] && nextStateTime == rhs.nextStateTime && arenaID == rhs.arenaID && fragLimitTimeout == rhs.fragLimitTimeout && matchStartTime == rhs.matchStartTime );
}

ID_INLINE bool rvTourneyArena::operator!=( const rvTourneyArena& rhs ) const {
	return ( arenaState != rhs.arenaState || players[ 0 ] != rhs.players[ 0 ] || players[ 1 ] != rhs.players[ 1 ] || nextStateTime != rhs.nextStateTime || arenaID != rhs.arenaID || fragLimitTimeout != rhs.fragLimitTimeout || matchStartTime != rhs.matchStartTime );
}

ID_INLINE rvTourneyArena& rvTourneyArena::operator=( const rvTourneyArena& rhs ) {
	players[ 0 ] = rhs.players[ 0 ];
	players[ 1 ] = rhs.players[ 1 ];
	arenaState = rhs.arenaState;
	nextStateTime = rhs.nextStateTime;
	arenaID = rhs.arenaID;
	fragLimitTimeout = rhs.fragLimitTimeout;
	matchStartTime = rhs.matchStartTime;
	return (*this);
}

typedef enum {
	TGH_BRACKET,
	TGH_PLAYER_ONE,
	TGH_PLAYER_TWO
} tourneyGUIHighlight_t;

class rvTourneyGUI {
public:
			rvTourneyGUI();
	void	SetupTourneyGUI( idUserInterface* newTourneyGUI, idUserInterface* newTourneyScoreboard );

	void	RoundStart( void );
	void	ArenaStart( int arena );
	void	ArenaDone( int arena );
	void	ArenaSelect( int arena, tourneyGUIHighlight_t highlightType );
	void	UpdateScores( void );
	void	PreTourney( void );
	void	TourneyStart( void );
	//void	UpdateByePlayer( void );
	void	SetupTourneyHistory( int startHistory, int endHistory, arenaOutcome_t tourneyHistory[ MAX_ROUNDS ][ MAX_ARENAS ] );

private:
	idUserInterface*	tourneyGUI;
	idUserInterface*	tourneyScoreboard;
};

#endif
