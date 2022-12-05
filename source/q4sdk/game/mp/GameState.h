//----------------------------------------------------------------
// GameState.h
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#ifndef __GAMESTATE_H__
#define __GAMESTATE_H__

#include "../Game_local.h"
#include "../MultiplayerGame.h"
#include "Tourney.h"

/*
===============================================================================

rvGameState

Game state info common for all gametypes

===============================================================================
*/

typedef enum {
	GS_BASE,
	GS_DM,
	GS_TEAMDM,
	GS_CTF,
	GS_TOURNEY,
	GS_DZ
} gameStateType_t;

typedef enum {
	INACTIVE = 0,						// not running
	WARMUP,								// warming up
	COUNTDOWN,							// post warmup pre-game
	GAMEON,								// game is on
	SUDDENDEATH,						// game is on but in sudden death, first frag wins
	GAMEREVIEW,							// game is over, scoreboard is up. we wait si_gameReviewPause seconds (which has a min value)
	NEXTGAME,
	STATE_COUNT
} mpGameState_t;

class rvGameState {
public:

					rvGameState( bool allocPrevious = true );
	virtual			~rvGameState( void );

					// clientNum == -1 except for SendInitialState
					// when clientNum >= 0, always send the state
	virtual void	SendState( int clientNum = -1 );
	virtual void	ReceiveState( const idBitMsg& msg );

	virtual void	SendInitialState( int clientNum );

	virtual void	PackState( idBitMsg& outMsg );
	virtual	void	WriteState( idBitMsg &msg );

	virtual void	UnpackState( const idBitMsg& inMsg );

	virtual void	GameStateChanged( void );

	virtual void	Run( void );
	virtual void	NewState( mpGameState_t newState );

	virtual void	ClientDisconnect( idPlayer* player );
	virtual void	Spectate( idPlayer* player );

	virtual void	Clear( void );

	virtual	bool	IsType( gameStateType_t type ) const;
	static gameStateType_t GetClassType( void );

	mpGameState_t	GetMPGameState( void ) { return currentState; }

	mpGameState_t	GetNextMPGameState( void ) { return nextState; }
	int				GetNextMPGameStateTime( void ) { return nextStateTime; }
	void			SetNextMPGameState( mpGameState_t newState ) { nextState = newState; }
	void			SetNextMPGameStateTime( int time ) { nextStateTime = time; }

	bool			operator==( const rvGameState& rhs ) const;
	bool			operator!=( const rvGameState& rhs ) const;
	rvGameState&	operator=( const rvGameState& rhs );

	void			WriteNetworkInfo( idFile *file, int clientNum );
	void			ReadNetworkInfo( idFile *file, int clientNum );

	void			SpawnDeadZonePowerup();
protected:
	static gameStateType_t type;
	rvGameState*	previousGameState;
	bool			trackPrevious;

	mpGameState_t	currentState;
	mpGameState_t	nextState;
	int				nextStateTime;

	int				fragLimitTimeout;
};

/*
===============================================================================

rvDMGameState

Game state info for DM

===============================================================================
*/
class rvDMGameState : public rvGameState {
public:
					rvDMGameState( bool allocPrevious = true );

	virtual void	Run( void );

	virtual	bool	IsType( gameStateType_t type ) const;
	static gameStateType_t GetClassType( void );

private:
	static gameStateType_t type;
};

/*
===============================================================================

rvTeamDMGameState

Game state info for Team DM

===============================================================================
*/
class rvTeamDMGameState : public rvGameState {
public:
					rvTeamDMGameState( bool allocPrevious = true );

	virtual void	Run( void );

	virtual	bool	IsType( gameStateType_t type ) const;
	static gameStateType_t GetClassType( void );

private:
	static gameStateType_t type;
};

/*
===============================================================================

rvCTFGameState

Game state info for CTF

===============================================================================
*/

// assault point state for CTF
enum apState_t {
	AS_MARINE,
	AS_STROGG,
	AS_NEUTRAL
};

// current flag state for CTF
enum flagState_t {
	FS_AT_BASE = 0,
	FS_DROPPED,
	FS_TAKEN,
	// for one flag CTF
	FS_TAKEN_STROGG, // taken by strogg team
	FS_TAKEN_MARINE // taken by marine team
};

struct flagStatus_t {
	flagState_t state;
	int			clientNum;
};

class rvCTFGameState : public rvGameState {
public:
					rvCTFGameState( bool allocPrevious = true );
	virtual void	Clear( void );


	virtual void	SendState( int clientNum = -1 );
	virtual void	ReceiveState( const idBitMsg& msg );

	virtual void	SendInitialState( int clientNum );

	virtual void	PackState( idBitMsg& outMsg );
	virtual	void	WriteState( idBitMsg &msg );

	virtual void	UnpackState( const idBitMsg& inMsg );

	virtual void	GameStateChanged( void );
	virtual void	Run( void );

	void			SetAPOwner( int ap, int owner );
	int				GetAPOwner( int ap );
	
	void			SetFlagState( int flag, flagState_t newState );
	flagState_t		GetFlagState( int flag );
	int				GetFlagCarrier( int flag );

	void			SetFlagCarrier( int flag, int clientNum );

	bool			operator==( const rvCTFGameState& rhs ) const;
	rvCTFGameState&	operator=( const rvCTFGameState& rhs );

	virtual	bool	IsType( gameStateType_t type ) const;
	static gameStateType_t GetClassType( void );
private:
	flagStatus_t	flagStatus[ TEAM_MAX ];	
	apState_t		apState[ MAX_AP ];

	static gameStateType_t type;
};

ID_INLINE void rvCTFGameState::SetAPOwner( int ap, int owner ) {
	assert( (ap >= 0 && ap < MAX_AP) && (owner >= 0 && owner < TEAM_MAX) );

	apState[ ap ] = (apState_t)owner;
}

ID_INLINE int rvCTFGameState::GetAPOwner( int ap ) {
	return apState[ ap ];
}

ID_INLINE flagState_t rvCTFGameState::GetFlagState( int flag ) {
	assert( flag >= 0 && flag < TEAM_MAX );

	return flagStatus[ flag ].state;
}

ID_INLINE int rvCTFGameState::GetFlagCarrier( int flag ) {
	assert( flag >= 0 && flag < TEAM_MAX );

	return flagStatus[ flag ].clientNum;
}

ID_INLINE bool operator==( const flagStatus_t& lhs, const flagStatus_t rhs ) {
	return (lhs.state == rhs.state) && (lhs.clientNum == rhs.clientNum);
}

ID_INLINE bool operator!=( const flagStatus_t& lhs, const flagStatus_t rhs ) {
	return (lhs.state != rhs.state) || (lhs.clientNum != rhs.clientNum);
}

/*
===============================================================================

rvTourneyGameState

Game state info for tourney

===============================================================================
*/

enum tourneyState_t {
	TS_INVALID = 0,
	TS_PREMATCH,
	TS_MATCH,
	TS_END_GAME
};

class rvTourneyGameState : public rvGameState {
public:
					rvTourneyGameState( bool allocPrevious = true );

	virtual void	Clear( void );

	virtual void	Run( void );

	virtual void	Reset( void );

	virtual void	SendState( int clientNum = -1 );
	virtual void	ReceiveState( const idBitMsg& msg );

	virtual void	SendInitialState( int clientNum );

	virtual void	PackState( idBitMsg& outMsg );
	virtual	void	WriteState( idBitMsg &msg );

	virtual void	UnpackState( const idBitMsg& inMsg );

	virtual void	GameStateChanged( void );
	virtual void	NewState( mpGameState_t newState );

	virtual void	ClientDisconnect( idPlayer* player );
	virtual void	Spectate( idPlayer* player );

	void			RemovePlayer( idPlayer* player );

	bool				operator==( const rvTourneyGameState& rhs ) const;
	rvTourneyGameState&	operator=( const rvTourneyGameState& rhs );

	int				GetNumArenas( void ) const;
	bool			HasBye( idPlayer* player );

	int				GetMaxRound( void ) const;
	int				GetRound( void ) const;
	int				GetStartingRound( void ) const;

	idPlayer**		GetArenaPlayers( int arena );

	rvTourneyArena& GetArena( int arena );

	const char*		GetRoundDescription( void );
	int				GetNextActiveArena( int arena );
	int				GetPrevActiveArena( int arena );

	void			SpectateCycleNext( idPlayer* player );
	void			SpectateCyclePrev( idPlayer* player );
	int				GetTourneyCount( void );
	void			SetTourneyCount( int count ) { tourneyCount = count; }

	void			UpdateTourneyBrackets( void ); 

	void			UpdateTourneyHistory( int round );
	int				FirstAvailableArena( void );

	arenaOutcome_t*	GetArenaOutcome( int arena );

	virtual	bool	IsType( gameStateType_t type ) const;
	static gameStateType_t GetClassType( void );
private:
	void			SetupInitialBrackets( void );

	tourneyState_t			tourneyState;

	// startingRound		- The starting round, with the current # of players
	int						startingRound;
	// round				- current round ( 1-indexed )
	int						round;
	// maxRound				- the most rounds we'll run (based on MAX_ARENAS)
	int						maxRound;
	// arenas				- current brackets, an extra arena to serve as the waiting room
	rvTourneyArena			arenas[ MAX_ARENAS + 1 ];
	// tourneyHistory		- past winners
	arenaOutcome_t			tourneyHistory[ MAX_ROUNDS ][ MAX_ARENAS ];
	// byeArena				- the arena that natural (non disconnection) bye players go in
	int						byeArena;
	// packTourneyHistory	- whether or not we should transmit tourney history to one client through SendState
	bool					packTourneyHistory;
	// forceTourneyHistory	- if true will sync down tourney history for all clients
	bool					forceTourneyHistory;
	// tourneyCount			- how many tourneys to run per map
	int						tourneyCount;
	// roundTimeout			- a delay between all arenas finishing and starting the next round
	int						roundTimeout;

	static gameStateType_t type;
};

ID_INLINE int rvTourneyGameState::GetMaxRound( void ) const {
	assert( type == GS_TOURNEY );
	return maxRound;
}

ID_INLINE int rvTourneyGameState::GetRound( void ) const {
	assert( type == GS_TOURNEY );
	return round;
}

ID_INLINE int rvTourneyGameState::GetStartingRound( void ) const {
	assert( type == GS_TOURNEY );
	return startingRound;
}

ID_INLINE idPlayer** rvTourneyGameState::GetArenaPlayers( int arena ) {
	assert( type == GS_TOURNEY );
	return arenas[ arena ].GetPlayers();
}

ID_INLINE rvTourneyArena& rvTourneyGameState::GetArena( int arena ) {
	assert( type == GS_TOURNEY );
	return arenas[ arena ]; 
}

ID_INLINE const char* rvTourneyGameState::GetRoundDescription( void ) {
	assert( type == GS_TOURNEY );

	if( round == maxRound ) {
		return common->GetLocalizedString( "#str_107720" );
	} else if( round == maxRound - 1 ) {
		return common->GetLocalizedString( "#str_107719" );
	} else if( round == maxRound - 2 ) {
		return common->GetLocalizedString( "#str_107718" );
	} else if( round == maxRound - 3 ) {
		return common->GetLocalizedString( "#str_107717" );
	} else {
		// shouldn't happen in production
		return va( "ROUND %d", round );
	}
}

ID_INLINE int rvTourneyGameState::GetTourneyCount( void ) {
	assert( type == GS_TOURNEY );
	return tourneyCount;
}

/*
===============================================================================

riDZGameState

Game state info for Dead Zone

===============================================================================
*/

// current control zone state
enum dzState_t {
	DZ_NONE = 0,
	DZ_TAKEN,
	DZ_LOST,
	DZ_DEADLOCK,
	DZ_MARINES_TAKEN,
	DZ_MARINES_LOST,
	DZ_STROGG_TAKEN,
	DZ_STROGG_LOST,
	DZ_MARINE_TO_STROGG,
	DZ_STROGG_TO_MARINE,
	DZ_MARINE_DEADLOCK,
	DZ_STROGG_DEADLOCK,
	DZ_MARINE_REGAIN,
	DZ_STROGG_REGAIN
};

struct dzStatus_t {
	dzState_t	state;
	int			clientNum;
};

class riDZGameState : public rvGameState {
public:
					riDZGameState( bool allocPrevious = true );
					~riDZGameState( void );
	virtual void	Clear( void );


	virtual void	SendState( int clientNum = -1 );
	virtual void	ReceiveState( const idBitMsg& msg );

	virtual void	SendInitialState( int clientNum );

	virtual void	PackState( idBitMsg& outMsg );
	virtual void	WriteState( idBitMsg &msg );

	virtual void	UnpackState( const idBitMsg& inMsg );

	virtual void	GameStateChanged( void );
	virtual void	Run( void );
	
	void			SetDZState( int dz, dzState_t newState );
	dzState_t		GetDZState( int dz );

	bool			operator==( const riDZGameState& rhs ) const;
	riDZGameState&	operator=( const riDZGameState& rhs );

	int				dzTriggerEnt;
	int				dzShaderParm;

private:
	dzStatus_t		dzStatus[ TEAM_MAX ];	

	void ControlZoneStateChanged( int team );
};


ID_INLINE dzState_t riDZGameState::GetDZState( int dz ) {
	assert( dz >= 0 && dz < TEAM_MAX );

	return dzStatus[ dz ].state;
}

ID_INLINE bool operator==( const dzStatus_t& lhs, const dzStatus_t rhs ) {
	return (lhs.state == rhs.state) && (lhs.clientNum == rhs.clientNum);
}

ID_INLINE bool operator!=( const dzStatus_t& lhs, const dzStatus_t rhs ) {
	return (lhs.state != rhs.state) || (lhs.clientNum != rhs.clientNum);
}
#endif
