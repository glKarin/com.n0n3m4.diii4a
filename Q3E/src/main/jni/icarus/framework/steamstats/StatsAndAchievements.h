/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef STATS_AND_ACHIEVEMENTS_H
#define STATS_AND_ACHIEVEMENTS_H

//#include "SpaceWar.h"
//#include "GameEngine.h"
#include "Inventory.h"

/*
enum EAchievements
{
	ACH_WIN_ONE_GAME = 0,
	ACH_WIN_100_GAMES = 1,
	ACH_HEAVY_FIRE = 2,
	ACH_TRAVEL_FAR_ACCUM = 3,
	ACH_TRAVEL_FAR_SINGLE = 4,
};
*/

enum EAchievements
{
	ACH_DO_1_WARP = 0,							// A Foolish, Unreasonable, Speed!						// m_nPlayerShipWarps
	ACH_DO_100_WARPS = 1,						// Tearing The Fabric Of Space!							// m_nPlayerShipWarps
	ACH_GET_THE_SACRIFICE_ENDING = 2,			// Sacrifice											// m_nTimesBeatGameSacrificeEnding
	ACH_GET_THE_SECRET_ENDING = 3,				// Super Happy Fun Time Save The Galaxy					// m_nTimesBeatGameSuperHappyFunTimeEnding
	ACH_GET_100_FPS_KILLS = 4,					// Gunslinger											// m_nPlayerFPSKills
	ACH_GET_10_SLOW_MO_FPS_KILLS = 5,			// Neo													// m_nPlayerFPSKillsInSlowMotion
	ACH_VISIT_ALL_STARGRID_POSITIONS = 6,		// Explorer												// m_nTimesAllStarGridPositionsVisited
	ACH_DESTROY_50_STARSHIPS = 7,				// Hunter Killer										// m_nPlayerStarshipKills
	ACH_DESTROY_10_SPACE_INSECTS = 8,			// Space Exterminator									// m_nPlayerSpaceInsectKills
	ACH_GET_A_FULL_CREW = 9,					// Full Compliment										// m_nTimesAllCrewFilled
	ACH_GET_FULL_RESERVE_CREW = 10,				// Send In The Reserves!								// m_nTimesAllReserveCrewFilled
	ACH_GET_A_MAX_LEVEL_CREWMEMBER = 11,		// The Finest Crew In The Fleet							// m_nTimesMaxCrewLevelAchieved
	ACH_COLLECT_100000_MATERIALS = 12,			// Space Salvager										// m_nTotalMaterialsAcquired
	ACH_COLLECT_100000_CREDITS = 13,			// Space Banker											// m_nTotalCreditsAcquired
	ACH_UPGRADE_SHIP_RESERVE_POWER_TO_MAX = 14,	// Unlimited (Reserve) Powerrrrrrr!!!!!					// m_nTimesUpgradedShipToMaxPowerReserve
	ACH_TAKE_COMMAND_OF_1_SHIP = 15,			// Captain												// m_nTimesTookCommandOfAnotherShip
	ACH_TAKE_COMMAND_OF_10_SHIPS = 16,			// Master Commandeerer									// m_nTimesTookCommandOfAnotherShip
	ACH_COMPLETE_THE_TUTORIAL = 17,				// Tutorial Completed									// m_nTimesTutorialCompleted
	ACH_GET_THE_LAST_BARNABY_LOG = 18,			// The Adventures of Barnaby: The Complete Collection!	// m_nTimesAllBarnabyLogsAcquired
	ACH_WATCH_THE_INTRO = 19,					// OOUAUAOOOUUAAAAAAA!									// m_nTimesBarnabyFlewIntoSpace
	ACH_GO_INSIDE_THE_CRYSTAL_ENTITY = 20,		// Entered The Crystal Entity							// m_nTimesCrystalEntityEntered
	ACH_BE_THE_LAST_SURVIVING_CREWMEMBER = 21,	// Sole Survivor										// m_nTimesWasTheSoleSurvivor
	ACH_COMPLETE_THE_ARTIFICIAN_LABYRINTH = 22,	// The Artifician Labyrinth								// m_nTimesCompletedTheArtificianLabyrinth
	ACH_BEAT_THE_GAME_IN_IRONMAN_MODE = 23,		// Nerves of Iron										// m_nTimesBeatTheGameInIronmanMode
};

struct Achievement_t
{
	EAchievements m_eAchievementID;
	const char *m_pchAchievementID;
	char m_rgchName[128];
	char m_rgchDescription[256];
	bool m_bAchieved;
	int m_iIconImage;
};

// BOYETTE FROM stdafx.h BEGIN
// OUT_Z_ARRAY indicates an output array that will be null-terminated.
/*
#if _MSC_VER >= 1600
       // Include the annotation header file.
       #include <sal.h>
       #if _MSC_VER >= 1700
              // VS 2012+
              #define OUT_Z_ARRAY _Post_z_
       #else
              // VS 2010
              #define OUT_Z_ARRAY _Deref_post_z_
       #endif
#else
       // gcc, clang, old versions of VS
       #define OUT_Z_ARRAY
#endif
 
template <size_t maxLenInChars> void sprintf_safe(OUT_Z_ARRAY char (&pDest)[maxLenInChars], const char *pFormat, ... )
{
	va_list params;
	va_start( params, pFormat );
#ifdef POSIX
	vsnprintf( pDest, maxLenInChars, pFormat, params );
#else
	_vsnprintf( pDest, maxLenInChars, pFormat, params );
#endif
	pDest[maxLenInChars - 1] = '\0';
	va_end( params );
}
*/
// BOYETTE FROM stdafx.h END

class ISteamUser;
class CSteamClient;

class CStatsAndAchievements
{
public:
	// Constructor
	CStatsAndAchievements();

	// Run a frame
	void RunFrame();

	// Display the stats and achievements
	void Render();

	// Game state changed
	void OnGameStateChange();

	// Accumulators
	void AddDistanceTraveled( float flDistance );

	// accessors
	float GetGameFeetTraveled() { return m_flGameFeetTraveled; }
	double GetGameDurationSeconds() { return m_flGameDurationSeconds; }

	// BOYETTE NOTE BEGIN: original these were private begin
	// Current Stat details
	float m_flGameFeetTraveled;
	uint64 m_ulTickCountGameStart;
	double m_flGameDurationSeconds;

	// BOYETTE STATS BEGIN
	// Persisted Stat details
	int m_nPlayerShipWarps; // A Foolish, Unreasonable, Speed! - Do 1 Warp // Force of Nature! - Do 100 Warps
	int m_nTimesBeatGameSacrificeEnding; // Sacrifice! - Get The Sacrifice Ending
	int m_nTimesBeatGameSuperHappyFunTimeEnding; // Super Happy Fun Time Save The Galaxy - Get The Secret Ending
	int m_nPlayerFPSKills; // Gunslinger! - Get 100 kills
	int m_nPlayerFPSKillsInSlowMotion; // Neo! - Get 10 Slow Mo Kills
	int m_nTimesAllStarGridPositionsVisited; // Explorer! - Visit All Stargrid Positions
	int m_nPlayerStarshipKills; // Hunter Killer! - Destroy 50 Starships
	int m_nPlayerSpaceInsectKills; // Space Exterminator! - Destroy 10 Space Insects
	int m_nTimesAllCrewFilled; // Full Compliment! - Get Full Crew
	int m_nTimesAllReserveCrewFilled; // Send In The Reserves! - Max Out The Reserve Crew
	int m_nTimesMaxCrewLevelAchieved; // The Finest Crew In The Fleet! - Get A Max Level Crewmember
	int m_nTotalMaterialsAcquired; // Space Salvager! - Collect 100,000 Materials
	int m_nTotalCreditsAcquired; // Mucho Crédito! or Mucho Dinero! - Collect 100,000 Credits
	int m_nTimesUpgradedShipToMaxPowerReserve; // Unlimited (Reserve) Powerrrrrrr!!!!! // or maybe: That Wrinkly Old Guy Would Be Proud! - Max out the power reserve
	int m_nTimesTookCommandOfAnotherShip; // I've Taken Command! or maybe: KHAAAAAAN! first name: Genghis! Captain! - Take Command of One Vessel // Master Commandeerer! - Take command of 10 vessels
	int m_nTimesTutorialCompleted; // Tutorial Completed! - Complete the tutorial
	int m_nTimesAllBarnabyLogsAcquired; // The Adventures of Barnaby: The Complete Collection! - Get the last barnaby log
	int m_nTimesBarnabyFlewIntoSpace; // OOUAUAOOOUUAAAAAAA! - Watch the intro
	int m_nTimesCrystalEntityEntered; // Entered The Crystal Entity! - Go inside the crystal entity
	int m_nTimesWasTheSoleSurvivor; // Sole Survivor! - Be the last surviving crewmember
	int m_nTimesCompletedTheArtificianLabyrinth; // The Artifician Labyrinth - Complete the Artifician labyrinth.
	int m_nTimesBeatTheGameInIronmanMode; // Nerves of Iron - Beat the game in Ironman mode.
	// BOYETTE STATS END

	/*
	// Persisted Stat details
	int m_nTotalGamesPlayed;
	int m_nTotalNumWins;
	int m_nTotalNumLosses;
	float m_flTotalFeetTraveled;
	float m_flMaxFeetTraveled;
	float m_flAverageSpeed;
	*/

	// PS3 specific
	//bool m_bStartedPS3TrophyInstall;
	//bool m_bInstalledPS3Trophies;
	// BOYETTE NOTE END: original these were private begin

	STEAM_CALLBACK( CStatsAndAchievements, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CStatsAndAchievements, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
	STEAM_CALLBACK( CStatsAndAchievements, OnAchievementStored, UserAchievementStored_t, m_CallbackAchievementStored );
	//STEAM_CALLBACK( CStatsAndAchievements, OnPS3TrophiesInstalled, PS3TrophiesInstalled_t, m_CallbackPS3TrophiesInstalled );
	

private:

	// Determine if we get this achievement now
	void EvaluateAchievement( Achievement_t &achievement );
	void UnlockAchievement( Achievement_t &achievement );

	// Store stats
	void StoreStatsIfNecessary();

	// Render helpers
	void DrawAchievementInfo();
	void DrawStatInfo();
	void DrawInventory();

	// PS3 specific
	//bool LoadUserStatsOnPS3();
	//bool SaveUserStatsOnPS3();

	// our GameID
	CGameID m_GameID;

	// Engine
	//IGameEngine *m_pGameEngine;

	// Steam User interface
	ISteamUser *m_pSteamUser;

	// Steam UserStats interface
	ISteamUserStats *m_pSteamUserStats;

	// Display font
	//HGAMEFONT m_hDisplayFont;

	// Did we get the stats from Steam?
	bool m_bRequestedStats;
	bool m_bStatsValid;

	// Should we store stats this frame?
	bool m_bStoreStats;
};

#endif // STATS_AND_ACHIEVEMENTS_H