//----------------------------------------------------------------
// StatManager.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __STATMANAGER_H__
#define __STATMANAGER_H__

#include "StatEvent.h"
#include "StatWindow.h"

#ifndef ID_TRAFFICSTATS
	#define ID_TRAFFICSTATS 0
#endif

/*
===============================================================================

	Multiplayer game statistics

===============================================================================
*/

/*
================
endGameAward_t

End-game award IDs
================
*/
enum endGameAward_t {
	EGA_INVALID = 0,
	EGA_LEMMING,
	EGA_RAIL_MASTER,
	EGA_ROCKET_SAUCE,
	EGA_BRAWLER,
	EGA_SNIPER,
	EGA_CRITICAL_FAILURE,
	EGA_TEAM_PLAYER,
	EGA_ACCURACY,
	EGA_FRAGS,
	EGA_PERFECT,
	EGA_NUM_AWARDS
};

/*
================
endGameAwardInfo_t

end-game award info
================
*/
struct endGameAwardInfo_t {
	char* name;
};

/*
================
inGameAward_t

in-game award IDs
================
*/
enum inGameAward_t {
	IGA_INVALID = 0,
	IGA_CAPTURE,
	IGA_HUMILIATION,
	IGA_IMPRESSIVE,
	IGA_EXCELLENT,
	IGA_ASSIST,
	IGA_DEFENSE,
	IGA_COMBO_KILL,
	IGA_RAMPAGE,
	IGA_HOLY_SHIT,
	IGA_NUM_AWARDS
};

enum comboKillState_t {
	CKS_NONE,
	CKS_ROCKET_FIRED,
	CKS_ROCKET_HIT,
	CKS_RAIL_FIRED,
	CKS_RAIL_HIT,
};

/*
================
inGameAwardInfo_t

in-game award info
================
*/
struct inGameAwardInfo_t {
	char* name;
};

/*
================
rvStatAllocator

A simple block allocator for different stat objects.
Note:  this isn't a real heap; it has no delete or tracking of free blocks.
It matches the memory usage of the rvStatManager which dumps its entire
memory all at once at very specific times.  If that ever changes, this will
need to change.
================
*/

const int BLOCK_SIZE = 1024;
const int MAX_BLOCKS = 128;

class rvStatAllocator {
	protected:
		byte			blocks[ BLOCK_SIZE * MAX_BLOCKS ];
		short			currentBlock;				// current block
		size_t			placeInBlock;				// current position, offset from start of block
		size_t			totalBytesUsed;	// total bytes handed out from the allocator
		size_t			totalAllocations;	// total blocks handed out from the allocator
		size_t			totalBytesAllocated;	// total bytes in all allocated blocks
		size_t			allocationsByType[ST_COUNT]; // one spare at [0]; allocations per type of event

		void *GetBlock( size_t blockSize, int* blockNumOut = NULL );

	public:
		rvStatAllocator();
		void Report();
		void Reset();

		// object allocators
		rvStatBeginGame *AllocStatBeginGame			( int t, int* blockNumOut = NULL );
		rvStatEndGame *AllocStatEndGame				( int t, int* blockNumOut = NULL );
		rvStatClientConnect *AllocStatClientConnect	( int t, int client, int* blockNumOut = NULL );
		rvStatHit *AllocStatHit						( int t, int p, int v, int w, bool countForAccuracy, int* blockNumOut = NULL );
		rvStatKill *AllocStatKill					( int t, int p, int v, bool g, int mod, int* blockNumOut = NULL );
		rvStatDeath *AllocStatDeath					( int t, int p, int mod, int* blockNumOut = NULL );
		rvStatDamageDealt *AllocStatDamageDealt		( int t, int p, int w, int d, int* blockNumOut = NULL );
		rvStatDamageTaken *AllocStatDamageTaken		( int t, int p, int w, int d, int* blockNumOut = NULL );
		rvStatFlagDrop *AllocStatFlagDrop			( int t, int p, int a, int tm, int* blockNumOut = NULL );
		rvStatFlagReturn *AllocStatFlagReturn		( int t, int p, int tm, int* blockNumOut = NULL );
		rvStatFlagCapture *AllocStatFlagCapture		( int t, int p, int f, int tm, int* blockNumOut = NULL );

		// bookkeeping readers
		ID_INLINE size_t GetBytesLeftInBlock() const {
			assert( placeInBlock <= BLOCK_SIZE );
			return ( size_t )( BLOCK_SIZE - placeInBlock );
		}

		ID_INLINE size_t GetPlaceInBlock() const {
			return placeInBlock;
		}

		ID_INLINE size_t GetTotalBytesUsed() const { 
			return totalBytesUsed; 
		}

		ID_INLINE size_t GetTotalAllocations() const {
			return totalAllocations;
		}

		ID_INLINE size_t GetAllocationsByType( statType_t type ) const { 
			return allocationsByType[ type ]; 
		}

		ID_INLINE size_t GetTotalBytesAllocated() const {
			return totalBytesAllocated;
		}
};


/*
================
rvPlayerStat

Stores one player's stat information.
================
*/
class rvPlayerStat {
public:
	rvPlayerStat();

	void Clear( void );
	void CalculateStats( int p, const idList<rvStat*>& playerStats );

	void PackStats( idBitMsg& msg );
	void UnpackStats( const idBitMsg& msg );

	int				weaponShots[ MAX_WEAPONS ];
	int				weaponHits[ MAX_WEAPONS ];
	
	int				weaponKills[ MAX_WEAPONS ];
	
	int				kills;
	int				deaths;
	int				suicides;
	float			damageRatio;

// asalmon: added for Xenon
// ddynerman: also used on PC
	int				damageGiven;
	int				damageTaken;

	idList<endGameAward_t>	endGameAwards;
// asalmon: changed this to just an array of counts
	int				inGameAwards[IGA_NUM_AWARDS];

	int				lastUpdateTime;
};

/*
================
rvStatManager

The statistics management interface.  This is the entrypoint into 
statistics from the game.

The game calls into rvStatManager at appropriate times (i.e. 'FlagCaptured')

rvStatManager creates stat events (see StatEvent.h) with the appropriate
information and append them to the statQueue.

The statQueue is parsed at the end of a game to tabulate statistics, give 
end-game awards.

================
*/
class rvStatManager {
public:
								rvStatManager();

	void						Init( void );
	void						Shutdown( void );
	
	// generic events
	void						BeginGame( void );
	void						EndGame( void );
	void						ClientConnect( int clientNum );
	void						ClientDisconnect( int clientNum );
	void						WeaponFired( const idPlayer* player, int weapon, int num );
	void						WeaponHit( const idActor* attacker, const idEntity* victim, int weapon, bool countForAccuracy = true );
	void						Damage( const idEntity* attacker, const idEntity* victim, int weapon, int damage );


	// ctf-specific events
	void						FlagCaptured( const idPlayer* player, int flagTeam );
	void						FlagDropped( const idPlayer* player, const idEntity* attacker );
	void						FlagReturned( const idPlayer* player );


	// shared events
	void						Kill( const idPlayer* victim, const idEntity* killer, int methodOfDeath );

	void						DebugPrint( void );

	rvPlayerStat*				GetPlayerStats( void ) { return playerStats; };
	rvPlayerStat*				GetPlayerStat( int clientNum );

	void						CalculateEndGameStats( void );

	void						GivePlayerCashForAward( idPlayer* player, inGameAward_t award );
	void						GiveInGameAward( inGameAward_t award, int clientNum );

	// network transmission
	void						SendStat( int toClient, int statClient );
	void						ReceiveStat( const idBitMsg& msg );

	void						SendInGameAward( inGameAward_t award, int clientNum );	
	void						ReceiveInGameAward( const idBitMsg& msg );
	void						CheckAwardQueue();

	void						ClearStats( void );

	int							DamageGiven( int playerNum, int lowerBound = idMath::INT_MIN, int upperBound = idMath::INT_MAX );
	int							DamageTaken( int playerNum, int lowerBound = idMath::INT_MIN, int upperBound = idMath::INT_MAX );

	void						UpdateInGameHud( idUserInterface* statHud, bool visible );
	void						UpdateEndGameHud( idUserInterface* statHud, int clientNum );
	void						SetupEndGameHud( idUserInterface* statHud );

	rvStat*						GetLastClientStat( int clientNum, statType_t type, int time );
	void						GetLastClientStats( int clientNum, statType_t type, int time, int num, rvStat** results );
	rvStatTeam*					GetLastTeamStat( int team, statType_t type, int time );
	rvStat*						GetStat( int i );

	void						GetAccuracyLeaders( int accuracyLeaders[ MAX_WEAPONS ] );
	
	void						SetupStatWindow( idUserInterface* statHud );
	void						SelectStatWindow( int selectionIndex, int selectionTeam );
	int							GetSelectedClientNum( int* selectionIndexOut = NULL, int* selectionTeamOut = NULL );

	//asalmon: Sends all stats to all clients.  For Xenon periodic update of stats.
	void						SendAllStats( int clientNum = -1, bool full = true );
	void						ReceiveAllStats( const idBitMsg& msg );

	void						SetDamageGiven(int client, int damage) { playerStats[client].damageGiven = damage; }
	void						SetDamageTaken(int client, int damage) { playerStats[client].damageTaken = damage; }

	int							FreeEvents( int blockNum );

	static comboKillState_t		comboKillState[ MAX_CLIENTS ];
	static int					lastRailShot[ MAX_CLIENTS ];
	static int					lastRailShotHits[ MAX_CLIENTS ];

private:
	idList<rvPair<rvStat*, int> >	statQueue;
	idList<rvPair<int, bool> >		awardQueue;
//	idList<rvStatInGame>		inGameStats;
//	rvStatInGame				previousInGameStats[ MAX_CLIENTS ];
//	rvStatInGame				localClientInGameStats;

	// local hud support
	int							localInGameAwards[ IGA_NUM_AWARDS ];
	int							inGameAwardHudTime;

	rvPlayerStat				playerStats[ MAX_CLIENTS ];

	bool						endGameSetup;

	rvStatWindow				statWindow;

	// shouchard:  stat allocator to avoid lots of little news
	rvStatAllocator				statAllocator;

#ifdef _XENON
	int							lastFullUpdate;
#endif

#if ID_TRAFFICSTATS
	int							startSent;
	int							startPacketsSent;
	int							startReceived;
	int							startPacketsReceived;
	int							startTime;
#endif
};

ID_INLINE rvStat* rvStatManager::GetStat( int i ) {
	if( i < 0 || i >= statQueue.Num() ) { 
		return NULL; 
	} 

	return statQueue[ i ].First();
}

extern rvStatManager* statManager;
extern inGameAwardInfo_t inGameAwardInfo[ IGA_NUM_AWARDS ];
extern endGameAwardInfo_t endGameAwardInfo[ EGA_NUM_AWARDS ];

#endif /* !__STATMANAGER_H__ */
