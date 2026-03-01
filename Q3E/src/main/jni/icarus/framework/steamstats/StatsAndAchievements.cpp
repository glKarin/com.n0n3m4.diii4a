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

#include "../../idlib/precompiled.h"
#pragma hdrstop

#ifdef STEAM_BUILD

//#include "stdafx.h"
#include "steam/steam_api.h"
#include "steam/isteamuserstats.h"
#include "steam/isteamremotestorage.h"
#include "steam/isteammatchmaking.h"
#include "steam/steam_gameserver.h"

#include "StatsAndAchievements.h"
#include "Inventory.h"
#include <math.h>
//#include "SpaceWarClient.h"

#define ACHDISP_FONT_HEIGHT 20
#define ACHDISP_COLUMN_WIDTH 340
#define ACHDISP_CENTER_SPACING 40
#define ACHDISP_VERT_SPACING 10
#define ACHDISP_IMG_SIZE 64
#define ACHDISP_IMG_PAD 10

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }

/*
Achievement_t g_rgAchievements[] = 
{
	_ACH_ID( ACH_WIN_ONE_GAME, "Winner" ),
	_ACH_ID( ACH_WIN_100_GAMES, "Champion" ),
	_ACH_ID( ACH_TRAVEL_FAR_ACCUM, "Interstellar" ),
	_ACH_ID( ACH_TRAVEL_FAR_SINGLE, "Orbiter" ),
};
const int NUM_STEAM_ACHIEVEMENTS = 4;
*/

Achievement_t g_rgAchievements[] = 
{
	_ACH_ID( ACH_DO_1_WARP, "A Foolish, Unreasonable, Speed!" ),
	_ACH_ID( ACH_DO_100_WARPS, "Tearing The Fabric Of Space!" ),
	_ACH_ID( ACH_GET_THE_SACRIFICE_ENDING, "Sacrifice" ),
	_ACH_ID( ACH_GET_THE_SECRET_ENDING, "Super Happy Fun Time Save The Galaxy" ),
	_ACH_ID( ACH_GET_100_FPS_KILLS, "Gunslinger" ),
	_ACH_ID( ACH_GET_10_SLOW_MO_FPS_KILLS, "Neo" ),
	_ACH_ID( ACH_VISIT_ALL_STARGRID_POSITIONS, "Explorer" ),
	_ACH_ID( ACH_DESTROY_50_STARSHIPS, "Hunter Killer" ),
	_ACH_ID( ACH_DESTROY_10_SPACE_INSECTS, "Space Exterminator" ),
	_ACH_ID( ACH_GET_A_FULL_CREW, "Full Compliment" ),
	_ACH_ID( ACH_GET_FULL_RESERVE_CREW, "Send In The Reserves!" ),
	_ACH_ID( ACH_GET_A_MAX_LEVEL_CREWMEMBER, "The Finest Crew In The Fleet" ),
	_ACH_ID( ACH_COLLECT_100000_MATERIALS, "Space Salvager" ),
	_ACH_ID( ACH_COLLECT_100000_CREDITS, "Space Banker" ),
	_ACH_ID( ACH_UPGRADE_SHIP_RESERVE_POWER_TO_MAX, "Unlimited (Reserve) Powerrrrrrr!!!!!" ),
	_ACH_ID( ACH_TAKE_COMMAND_OF_1_SHIP, "Captain" ),
	_ACH_ID( ACH_TAKE_COMMAND_OF_10_SHIPS, "Master Commandeerer" ),
	_ACH_ID( ACH_COMPLETE_THE_TUTORIAL, "Tutorial Completed" ),
	_ACH_ID( ACH_GET_THE_LAST_BARNABY_LOG, "The Adventures of Barnaby: The Complete Collection!" ),
	_ACH_ID( ACH_WATCH_THE_INTRO, "OOUAUAOOOUUAAAAAAA!" ),
	_ACH_ID( ACH_GO_INSIDE_THE_CRYSTAL_ENTITY, "Entered The Crystal Entity" ),
	_ACH_ID( ACH_BE_THE_LAST_SURVIVING_CREWMEMBER, "Sole Survivor" ),
	_ACH_ID( ACH_COMPLETE_THE_ARTIFICIAN_LABYRINTH, "The Artifician Labyrinth" ),
	_ACH_ID( ACH_BEAT_THE_GAME_IN_IRONMAN_MODE, "Nerves of Iron" ),
};
const int NUM_STEAM_ACHIEVEMENTS = 24;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#pragma warning( push )
//  warning C4355: 'this' : used in base member initializer list
//  This is OK because it's warning on setting up the Steam callbacks, they won't use this until after construction is done
#pragma warning( disable : 4355 ) 
CStatsAndAchievements::CStatsAndAchievements()
	: 
	m_pSteamUser( NULL ),
	m_pSteamUserStats( NULL ),
	m_GameID( SteamUtils()->GetAppID() ),
	m_CallbackUserStatsReceived( this, &CStatsAndAchievements::OnUserStatsReceived ),
	m_CallbackUserStatsStored( this, &CStatsAndAchievements::OnUserStatsStored ),
	m_CallbackAchievementStored( this, &CStatsAndAchievements::OnAchievementStored )
	//m_CallbackPS3TrophiesInstalled( this, &CStatsAndAchievements::OnPS3TrophiesInstalled )
{
	m_pSteamUser = SteamUser();
	m_pSteamUserStats = SteamUserStats();

	m_bRequestedStats = false;
	m_bStatsValid = false;
	m_bStoreStats = false;

	// BOYETTE STATS BEGIN
	m_nPlayerShipWarps = 0;
	m_nTimesBeatGameSacrificeEnding = 0;
	m_nTimesBeatGameSuperHappyFunTimeEnding = 0;
	m_nPlayerFPSKills = 0;
	m_nPlayerFPSKillsInSlowMotion = 0;
	m_nTimesAllStarGridPositionsVisited = 0;
	m_nPlayerStarshipKills = 0;
	m_nPlayerSpaceInsectKills = 0;
	m_nTimesAllCrewFilled = 0;
	m_nTimesAllReserveCrewFilled = 0;
	m_nTimesMaxCrewLevelAchieved = 0;
	m_nTotalMaterialsAcquired = 0;
	m_nTotalCreditsAcquired = 0;
	m_nTimesUpgradedShipToMaxPowerReserve = 0;
	m_nTimesTookCommandOfAnotherShip = 0;
	m_nTimesTutorialCompleted = 0;
	m_nTimesAllBarnabyLogsAcquired = 0;
	m_nTimesBarnabyFlewIntoSpace = 0;
	m_nTimesCrystalEntityEntered = 0;
	m_nTimesWasTheSoleSurvivor = 0;
	m_nTimesCompletedTheArtificianLabyrinth = 0;
	m_nTimesBeatTheGameInIronmanMode = 0;
	// BOYETTE STATS END

	/*
	m_flGameFeetTraveled = 0;
	m_nTotalGamesPlayed = 0;
	m_nTotalNumWins = 0;
	m_nTotalNumLosses = 0;
	m_flTotalFeetTraveled = 0;
	m_flMaxFeetTraveled = 0;

	m_flAverageSpeed = 0;
	*/

	/*
	m_hDisplayFont = pGameEngine->HCreateFont( ACHDISP_FONT_HEIGHT, FW_MEDIUM, false, "Arial" );
	if ( !m_hDisplayFont )
		OutputDebugString( "Stats font was not created properly, text won't draw\n" );
	*/

	//m_bInstalledPS3Trophies = false;
	//m_bStartedPS3TrophyInstall = false;
}
//#pragma warning( pop )

//-----------------------------------------------------------------------------
// Purpose: Run a frame for the CStatsAndAchievements
//-----------------------------------------------------------------------------
void CStatsAndAchievements::RunFrame()
{
/*
	// On PS3, must first install trophies before using the stats system
	if ( !m_bInstalledPS3Trophies ) {
#ifdef _PS3
		// PS3 trophies should be installed before writing to disk. Check PS3TrophiesInstalled_t to determine
		// if trophy installation failed because the machine is out of free HD space.
		if ( !m_bStartedPS3TrophyInstall )
		{
			if ( !m_pSteamUserStats->InstallPS3Trophies() )
			{
				OutputDebugString( "Failed to install PS3 trophies. This is a fatal error\n" );
				exit( 1 );
			}

			m_bStartedPS3TrophyInstall = true;
		}

		// wait for PS3TrophiesInstalled_t
#else
		m_bInstalledPS3Trophies = true;
		m_bStartedPS3TrophyInstall = true;
#endif
	}
	else if ( !m_bRequestedStats )
	{		
		// Is Steam Loaded? if no, can't get stats, done
		if ( NULL == m_pSteamUserStats || NULL == m_pSteamUser )
		{
			m_bRequestedStats = true;
			return;
		}

		LoadUserStatsOnPS3();

		// If yes, request our stats
		bool bSuccess = m_pSteamUserStats->RequestCurrentStats();
		
		// This function should only return false if we weren't logged in, and we already checked that.
		// But handle it being false again anyway, just ask again later.
		m_bRequestedStats = bSuccess;
	}
*/

// BOYETTE BEGIN - since we are not ever going to have a ps3 version this is a simplified version of what is above.
	if ( !m_bRequestedStats )
	{		
		// Is Steam Loaded? if no, can't get stats, done
		if ( NULL == m_pSteamUserStats || NULL == m_pSteamUser )
		{
			m_bRequestedStats = true;
			return;
		}

		//LoadUserStatsOnPS3();

		// If yes, request our stats
		bool bSuccess = m_pSteamUserStats->RequestCurrentStats();
		
		// This function should only return false if we weren't logged in, and we already checked that.
		// But handle it being false again anyway, just ask again later.
		m_bRequestedStats = bSuccess;
	}
// BOYETTE END
	
	if ( !m_bStatsValid ) {
		return;
	}
	//common->Printf("THE STEAM CALLBACK WORKED\n");
	// Get info from sources

	// Evaluate achievements
	for ( int iAch = 0; iAch < NUM_STEAM_ACHIEVEMENTS; ++iAch )
	{
		EvaluateAchievement( g_rgAchievements[iAch] );
	}


	// BOYETTE TESTING BEGIN: To reset all achievements AND stats
	//m_pSteamUserStats->ResetAllStats(true);
	//SteamUserStats()->ResetAllStats(true);
	// BOYETTE TESTING END: To reset all achievements AND stats

	// BOYETTE TESTING BEGIN: To reset all achievements
	//for (int iAch = 0; iAch < NUM_STEAM_ACHIEVEMENTS; ++iAch)
	//{
	//	m_pSteamUserStats->ClearAchievement(g_rgAchievements[iAch].m_pchAchievementID);
	//}
	// BOYETTE TESTING END: To reset all achievements

	// Store stats
	StoreStatsIfNecessary();
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate distance traveled
//-----------------------------------------------------------------------------
void CStatsAndAchievements::AddDistanceTraveled( float flDistance )
{
	//m_flGameFeetTraveled += SpaceWarClient()->PixelsToFeet( flDistance );
}

//-----------------------------------------------------------------------------
// Purpose: Game state has changed
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnGameStateChange()
{
	if ( !m_bStatsValid ) {
		return;
	}

	// BOYETTE MOVED THIS HERE BEGIN
	m_bStoreStats = true;
	// BOYETTE MOVED THIS HERE END

	/*
	switch ( eNewState ) {
		case k_EClientStatsAchievements:
		case k_EClientGameStartServer:
		case k_EClientGameMenu:
		case k_EClientGameQuitMenu:
		case k_EClientGameExiting:
		case k_EClientGameInstructions:
		case k_EClientGameConnecting:
		case k_EClientGameConnectionFailure:
		default:
			break;
		case k_EClientGameActive:
			// Reset per-game stats
			m_flGameFeetTraveled = 0;
			m_ulTickCountGameStart = m_pGameEngine->GetGameTickCount();
			break;
		case k_EClientFindInternetServers:
			break;	
		case k_EClientGameWinner:
			if ( SpaceWarClient()->BLocalPlayerWonLastGame() )
				m_nTotalNumWins++;
			else
				m_nTotalNumLosses++;
			// fall through
		case k_EClientGameDraw:

			// Tally games
			m_nTotalGamesPlayed++;

			// Accumulate distances
			m_flTotalFeetTraveled += m_flGameFeetTraveled;

			// New max?
			if ( m_flGameFeetTraveled > m_flMaxFeetTraveled )
				m_flMaxFeetTraveled = m_flGameFeetTraveled;

			// Calc game duration
			m_flGameDurationSeconds = ( m_pGameEngine->GetGameTickCount() - m_ulTickCountGameStart ) / 1000.0;

			// We want to update stats the next frame.
			m_bStoreStats = true;

			break;
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: see if we should unlock this achievement
//-----------------------------------------------------------------------------
void CStatsAndAchievements::EvaluateAchievement( Achievement_t &achievement )
{
	// Already have it?
	if ( achievement.m_bAchieved )
		return;

	switch ( achievement.m_eAchievementID )
	{
	case ACH_DO_1_WARP:
		if ( m_nPlayerShipWarps )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DO_100_WARPS:
		if ( m_nPlayerShipWarps >= 100 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_SACRIFICE_ENDING:
		if ( m_nTimesBeatGameSacrificeEnding )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_SECRET_ENDING:
		if ( m_nTimesBeatGameSuperHappyFunTimeEnding )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_100_FPS_KILLS:
		if ( m_nPlayerFPSKills >= 100 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_10_SLOW_MO_FPS_KILLS:
		if ( m_nPlayerFPSKillsInSlowMotion >= 10 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_VISIT_ALL_STARGRID_POSITIONS:
		if ( m_nTimesAllStarGridPositionsVisited )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DESTROY_50_STARSHIPS:
		if ( m_nPlayerStarshipKills >= 50 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DESTROY_10_SPACE_INSECTS:
		if ( m_nPlayerSpaceInsectKills >= 10 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_A_FULL_CREW:
		if ( m_nTimesAllCrewFilled )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_FULL_RESERVE_CREW:
		if ( m_nTimesAllReserveCrewFilled )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_A_MAX_LEVEL_CREWMEMBER:
		if ( m_nTimesMaxCrewLevelAchieved )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COLLECT_100000_MATERIALS:
		if ( m_nTotalMaterialsAcquired >= 100000 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COLLECT_100000_CREDITS:
		if ( m_nTotalCreditsAcquired >= 100000 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_UPGRADE_SHIP_RESERVE_POWER_TO_MAX:
		if ( m_nTimesUpgradedShipToMaxPowerReserve )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TAKE_COMMAND_OF_1_SHIP:
		if ( m_nTimesTookCommandOfAnotherShip )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TAKE_COMMAND_OF_10_SHIPS:
		if ( m_nTimesTookCommandOfAnotherShip >= 10 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COMPLETE_THE_TUTORIAL:
		if ( m_nTimesTutorialCompleted )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_LAST_BARNABY_LOG:
		if ( m_nTimesAllBarnabyLogsAcquired )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_WATCH_THE_INTRO:
		if ( m_nTimesBarnabyFlewIntoSpace )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GO_INSIDE_THE_CRYSTAL_ENTITY:
		if ( m_nTimesCrystalEntityEntered )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_BE_THE_LAST_SURVIVING_CREWMEMBER:
		if ( m_nTimesWasTheSoleSurvivor )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COMPLETE_THE_ARTIFICIAN_LABYRINTH:
		if ( m_nTimesCompletedTheArtificianLabyrinth )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_BEAT_THE_GAME_IN_IRONMAN_MODE:
		if ( m_nTimesBeatTheGameInIronmanMode )
		{
			UnlockAchievement( achievement );
		}
		break;
	}
	/*
	switch ( achievement.m_eAchievementID )
	{
	case ACH_DO_1_WARP: //XX
		if ( m_nPlayerShipWarps )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DO_100_WARPS: //XX
		if ( m_nPlayerShipWarps >= 7 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_SACRIFICE_ENDING: //XX
		if ( m_nTimesBeatGameSacrificeEnding )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_SECRET_ENDING: //XX
		if ( m_nTimesBeatGameSuperHappyFunTimeEnding )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_100_FPS_KILLS: //XX
		if ( m_nPlayerFPSKills >= 10 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_10_SLOW_MO_FPS_KILLS: //XX
		if ( m_nPlayerFPSKillsInSlowMotion >= 2 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_VISIT_ALL_STARGRID_POSITIONS:
		if ( m_nTimesAllStarGridPositionsVisited )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DESTROY_50_STARSHIPS: //XX
		if ( m_nPlayerStarshipKills >= 7 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_DESTROY_10_SPACE_INSECTS: //XX
		if ( m_nPlayerSpaceInsectKills >= 2 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_A_FULL_CREW: //XX
		if ( m_nTimesAllCrewFilled )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_FULL_RESERVE_CREW: //XX
		if ( m_nTimesAllReserveCrewFilled )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_A_MAX_LEVEL_CREWMEMBER: //XX
		if ( m_nTimesMaxCrewLevelAchieved )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COLLECT_100000_MATERIALS: //XX
		if ( m_nTotalMaterialsAcquired >= 100000 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COLLECT_100000_CREDITS: //XX
		if ( m_nTotalCreditsAcquired >= 100000 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_UPGRADE_SHIP_RESERVE_POWER_TO_MAX: //XX - retest
		if ( m_nTimesUpgradedShipToMaxPowerReserve )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TAKE_COMMAND_OF_1_SHIP: //XX
		if ( m_nTimesTookCommandOfAnotherShip )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TAKE_COMMAND_OF_10_SHIPS: //XX
		if ( m_nTimesTookCommandOfAnotherShip >= 2 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_COMPLETE_THE_TUTORIAL: //XX
		if ( m_nTimesTutorialCompleted )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GET_THE_LAST_BARNABY_LOG: //XX
		if ( m_nTimesAllBarnabyLogsAcquired )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_WATCH_THE_INTRO: //XX
		if ( m_nTimesBarnabyFlewIntoSpace )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_GO_INSIDE_THE_CRYSTAL_ENTITY: //XX
		if ( m_nTimesCrystalEntityEntered )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_BE_THE_LAST_SURVIVING_CREWMEMBER: //XX
		if ( m_nTimesWasTheSoleSurvivor )
		{
			UnlockAchievement( achievement );
		}
		break;
	}
	*/
	/*
	{
	case ACH_WIN_ONE_GAME:
		if ( m_nTotalNumWins )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_WIN_100_GAMES:
		if ( m_nTotalNumWins >= 100 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TRAVEL_FAR_ACCUM:
		if ( m_flTotalFeetTraveled >= 5280 )
		{
			UnlockAchievement( achievement );
		}
		break;
	case ACH_TRAVEL_FAR_SINGLE:
		if ( m_flGameFeetTraveled > 500 )
		{
			UnlockAchievement( achievement );
		}
		break;
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Unlock this achievement
//-----------------------------------------------------------------------------
void CStatsAndAchievements::UnlockAchievement( Achievement_t &achievement )
{
	achievement.m_bAchieved = true;

	// the icon may change once it's unlocked
	achievement.m_iIconImage = 0;

	// mark it down
	m_pSteamUserStats->SetAchievement( achievement.m_pchAchievementID );

	// Store stats end of frame
	m_bStoreStats = true;
}

//-----------------------------------------------------------------------------
// Purpose: Store stats in the Steam database
//-----------------------------------------------------------------------------
void CStatsAndAchievements::StoreStatsIfNecessary()
{
	if ( m_bStoreStats )
	{
		// already set any achievements in UnlockAchievement

		// BOYETTE STATS BEGIN
		// set stats
		m_pSteamUserStats->SetStat( "m_nPlayerShipWarps", m_nPlayerShipWarps );
		m_pSteamUserStats->SetStat( "m_nTimesBeatGameSacrificeEnding", m_nTimesBeatGameSacrificeEnding );
		m_pSteamUserStats->SetStat( "m_nTimesBeatGameSuperHappyFunTimeEnding", m_nTimesBeatGameSuperHappyFunTimeEnding );
		m_pSteamUserStats->SetStat( "m_nPlayerFPSKills", m_nPlayerFPSKills );
		m_pSteamUserStats->SetStat( "m_nPlayerFPSKillsInSlowMotion", m_nPlayerFPSKillsInSlowMotion );
		m_pSteamUserStats->SetStat( "m_nTimesAllStarGridPositionsVisited", m_nTimesAllStarGridPositionsVisited );
		m_pSteamUserStats->SetStat( "m_nPlayerStarshipKills", m_nPlayerStarshipKills );
		m_pSteamUserStats->SetStat( "m_nPlayerSpaceInsectKills", m_nPlayerSpaceInsectKills );
		m_pSteamUserStats->SetStat( "m_nTimesAllCrewFilled", m_nTimesAllCrewFilled );
		m_pSteamUserStats->SetStat( "m_nTimesAllReserveCrewFilled", m_nTimesAllReserveCrewFilled );
		m_pSteamUserStats->SetStat( "m_nTimesMaxCrewLevelAchieved", m_nTimesMaxCrewLevelAchieved );
		m_pSteamUserStats->SetStat( "m_nTotalMaterialsAcquired", m_nTotalMaterialsAcquired );
		m_pSteamUserStats->SetStat( "m_nTotalCreditsAcquired", m_nTotalCreditsAcquired );
		m_pSteamUserStats->SetStat( "m_nTimesUpgradedShipToMaxPowerReserve", m_nTimesUpgradedShipToMaxPowerReserve );
		m_pSteamUserStats->SetStat( "m_nTimesTookCommandOfAnotherShip", m_nTimesTookCommandOfAnotherShip );
		m_pSteamUserStats->SetStat( "m_nTimesTutorialCompleted", m_nTimesTutorialCompleted );
		m_pSteamUserStats->SetStat( "m_nTimesAllBarnabyLogsAcquired", m_nTimesAllBarnabyLogsAcquired );
		m_pSteamUserStats->SetStat( "m_nTimesBarnabyFlewIntoSpace", m_nTimesBarnabyFlewIntoSpace );
		m_pSteamUserStats->SetStat( "m_nTimesCrystalEntityEntered", m_nTimesCrystalEntityEntered );
		m_pSteamUserStats->SetStat( "m_nTimesWasTheSoleSurvivor", m_nTimesWasTheSoleSurvivor );
		m_pSteamUserStats->SetStat( "m_nTimesCompletedTheArtificianLabyrinth", m_nTimesCompletedTheArtificianLabyrinth );
		m_pSteamUserStats->SetStat( "m_nTimesBeatTheGameInIronmanMode", m_nTimesBeatTheGameInIronmanMode );
		// BOYETTE STATS END

		/*
		// set stats
		m_pSteamUserStats->SetStat( "NumGames", m_nTotalGamesPlayed );
		m_pSteamUserStats->SetStat( "NumWins", m_nTotalNumWins );
		m_pSteamUserStats->SetStat( "NumLosses", m_nTotalNumLosses );
		m_pSteamUserStats->SetStat( "FeetTraveled", m_flTotalFeetTraveled );
		m_pSteamUserStats->SetStat( "MaxFeetTraveled", m_flMaxFeetTraveled );
		// Update average feet / second stat
		m_pSteamUserStats->UpdateAvgRateStat( "AverageSpeed", m_flGameFeetTraveled, m_flGameDurationSeconds );
		// The averaged result is calculated for us
		m_pSteamUserStats->GetStat( "AverageSpeed", &m_flAverageSpeed );
		*/

		bool bSuccess = m_pSteamUserStats->StoreStats();
		// If this failed, we never sent anything to the server, try
		// again later.
		m_bStoreStats = !bSuccess;
	}
}


//-----------------------------------------------------------------------------
// Purpose: We have stats data from Steam. It is authoritative, so update
//			our data with those results now.
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	//common->Printf("THE STEAM CALLBACK WORKED\n");
	if ( !m_pSteamUserStats )
		return;

	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			//OutputDebugString( "Received stats and achievements from Steam\n" );

			m_bStatsValid = true;

			// load achievements
			common->Printf( "--- Fetching Steam Achievements BEGIN ---\n" );
			for ( int iAch = 0; iAch < NUM_STEAM_ACHIEVEMENTS; ++iAch )
			{
				Achievement_t &ach = g_rgAchievements[iAch];
				m_pSteamUserStats->GetAchievement( ach.m_pchAchievementID, &ach.m_bAchieved );
				//sprintf_safe( ach.m_rgchName, "%s", m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "name" ) );
				common->Printf( "Name: %s \n", m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "name" ) );
				//sprintf_safe( ach.m_rgchDescription, "%s", m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "desc" ) );
				common->Printf( "Description: %s \n", m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "desc" ) );
			}
			common->Printf( "--- Fetching Steam Achievements END ---\n" );

			// BOYETTE STATS BEGIN
			// load stats
			m_pSteamUserStats->GetStat( "m_nPlayerShipWarps", &m_nPlayerShipWarps );
			m_pSteamUserStats->GetStat( "m_nTimesBeatGameSacrificeEnding", &m_nTimesBeatGameSacrificeEnding );
			m_pSteamUserStats->GetStat( "m_nTimesBeatGameSuperHappyFunTimeEnding", &m_nTimesBeatGameSuperHappyFunTimeEnding );
			m_pSteamUserStats->GetStat( "m_nPlayerFPSKills", &m_nPlayerFPSKills );
			m_pSteamUserStats->GetStat( "m_nPlayerFPSKillsInSlowMotion", &m_nPlayerFPSKillsInSlowMotion );
			m_pSteamUserStats->GetStat( "m_nTimesAllStarGridPositionsVisited", &m_nTimesAllStarGridPositionsVisited );
			m_pSteamUserStats->GetStat( "m_nPlayerStarshipKills", &m_nPlayerStarshipKills );
			m_pSteamUserStats->GetStat( "m_nPlayerSpaceInsectKills", &m_nPlayerSpaceInsectKills );
			m_pSteamUserStats->GetStat( "m_nTimesAllCrewFilled", &m_nTimesAllCrewFilled );
			m_pSteamUserStats->GetStat( "m_nTimesAllReserveCrewFilled", &m_nTimesAllReserveCrewFilled );
			m_pSteamUserStats->GetStat( "m_nTimesMaxCrewLevelAchieved", &m_nTimesMaxCrewLevelAchieved );
			m_pSteamUserStats->GetStat( "m_nTotalMaterialsAcquired", &m_nTotalMaterialsAcquired );
			m_pSteamUserStats->GetStat( "m_nTotalCreditsAcquired", &m_nTotalCreditsAcquired );
			m_pSteamUserStats->GetStat( "m_nTimesUpgradedShipToMaxPowerReserve", &m_nTimesUpgradedShipToMaxPowerReserve );
			m_pSteamUserStats->GetStat( "m_nTimesTookCommandOfAnotherShip", &m_nTimesTookCommandOfAnotherShip );
			m_pSteamUserStats->GetStat( "m_nTimesTutorialCompleted", &m_nTimesTutorialCompleted );
			m_pSteamUserStats->GetStat( "m_nTimesAllBarnabyLogsAcquired", &m_nTimesAllBarnabyLogsAcquired );
			m_pSteamUserStats->GetStat( "m_nTimesBarnabyFlewIntoSpace", &m_nTimesBarnabyFlewIntoSpace );
			m_pSteamUserStats->GetStat( "m_nTimesCrystalEntityEntered", &m_nTimesCrystalEntityEntered );
			m_pSteamUserStats->GetStat( "m_nTimesWasTheSoleSurvivor", &m_nTimesWasTheSoleSurvivor );
			m_pSteamUserStats->GetStat( "m_nTimesCompletedTheArtificianLabyrinth", &m_nTimesCompletedTheArtificianLabyrinth );
			m_pSteamUserStats->GetStat( "m_nTimesBeatTheGameInIronmanMode", &m_nTimesBeatTheGameInIronmanMode );
			// BOYETTE STATS END

			/*
			// load stats
			m_pSteamUserStats->GetStat( "NumGames", &m_nTotalGamesPlayed );
			m_pSteamUserStats->GetStat( "NumWins", &m_nTotalNumWins );
			m_pSteamUserStats->GetStat( "NumLosses", &m_nTotalNumLosses );
			m_pSteamUserStats->GetStat( "FeetTraveled", &m_flTotalFeetTraveled );
			m_pSteamUserStats->GetStat( "MaxFeetTraveled", &m_flMaxFeetTraveled );
			m_pSteamUserStats->GetStat( "AverageSpeed", &m_flAverageSpeed );
			*/

			//SaveUserStatsOnPS3();
		}
		else
		{
			//char buffer[128];
			//sprintf_safe( buffer, "RequestStats - failed, %d\n", pCallback->m_eResult );
			common->Printf( "RequestStats - failed, %d\n", pCallback->m_eResult );
			//buffer[ sizeof(buffer) - 1 ] = 0;
			//OutputDebugString( buffer );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Our stats data was stored!
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			//OutputDebugString( "StoreStats - success\n" );
			common->Printf("STEAM: StoreStats - success\n");
			//SaveUserStatsOnPS3();
		}
		else if ( k_EResultInvalidParam == pCallback->m_eResult )
		{
			// One or more stats we set broke a constraint. They've been reverted,
			// and we should re-iterate the values now to keep in sync.
			//OutputDebugString( "StoreStats - some failed to validate\n" );
			common->Warning("STEAM: StoreStats - some failed to validate\n");
			// Fake up a callback here so that we re-load the values.
			UserStatsReceived_t callback;
			callback.m_eResult = k_EResultOK;
			callback.m_nGameID = m_GameID.ToUint64();
			OnUserStatsReceived( &callback );
		}
		else
		{
			//char buffer[128];
			//sprintf_safe( buffer, "StoreStats - failed, %d\n", pCallback->m_eResult );
			common->Warning("STEAM: StoreStats - failed, %d\n", pCallback->m_eResult);
			//buffer[ sizeof(buffer) - 1 ] = 0;
			//OutputDebugString( buffer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: An achievement was stored
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( 0 == pCallback->m_nMaxProgress )
		{
			//char buffer[128];
			//sprintf_safe( buffer, "Achievement '%s' unlocked!", pCallback->m_rgchAchievementName );
			common->Printf("STEAM: Achievement '%s' unlocked!\n", pCallback->m_rgchAchievementName);
			//buffer[ sizeof(buffer) - 1 ] = 0;
			//OutputDebugString( buffer );
		}
		else
		{
			//char buffer[128];
			//sprintf_safe( buffer, "Achievement '%s' progress callback, (%d,%d)\n", pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress );
			common->Printf("STEAM: Achievement '%s' progress callback, (%d,%d)\n", pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress);
			//buffer[ sizeof(buffer) - 1 ] = 0;
			//OutputDebugString( buffer );
		}
	}
}

/*
//-----------------------------------------------------------------------------
// Purpose: An achievement was stored
//-----------------------------------------------------------------------------
void CStatsAndAchievements::OnPS3TrophiesInstalled( PS3TrophiesInstalled_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them	 
	if ( m_GameID.ToUint64() == pCallback->m_nGameID )
	{
		if ( pCallback->m_eResult != k_EResultOK )
		{
			// this is a fatal error. Usually the PS3 would have already displayed a fatal error to the user and forced the game to exit. If the system
			// could not display that error, you should display the appropriate message to the user then exit.
			char buffer[256];
			if ( pCallback->m_eResult == k_EResultDiskFull )
				sprintf_safe( buffer, "Failed to install PS3 trophies because the HD is full (required space=%llu)\n", pCallback->m_ulRequiredDiskSpace );
			else
				sprintf_safe( buffer, "Failed to install PS3 trophies (%d)", pCallback->m_eResult );

			buffer[ sizeof(buffer) - 1 ] = 0;
			//OutputDebugString( buffer );
			exit( 1 );
		}

		m_bInstalledPS3Trophies = true;
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: Display the user's stats and achievements
//-----------------------------------------------------------------------------
void CStatsAndAchievements::Render()
{
	/*
	const int32 width = m_pGameEngine->GetViewportWidth();
	const int32 height = m_pGameEngine->GetViewportHeight();

	const int32 pxColumn1Left = width / 2 - ACHDISP_COLUMN_WIDTH - ACHDISP_CENTER_SPACING / 2;
	const int32 pxColumn2Left = width / 2 + ACHDISP_CENTER_SPACING / 2;

	RECT rect;

	if ( !m_bStatsValid )
	{
		rect.top = 0;
		rect.bottom = m_pGameEngine->GetViewportHeight();
		rect.left = 0;
		rect.right = width;

		char rgchBuffer[256];
		sprintf_safe( rgchBuffer, "Unable to retrieve data from Steam\n" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_CENTER|TEXTPOS_VCENTER, rgchBuffer );


		rect.left = 0;
		rect.right = width;
		rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.7);
		rect.bottom = m_pGameEngine->GetViewportHeight();

		sprintf_safe( rgchBuffer, "Press ESC to return to the Main Menu" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_CENTER|TEXTPOS_TOP, rgchBuffer );

	}
	else
	{
		// COLUMN 1
		// Achievements above the midline 
		int32 pxVertOffset = height / 2 - 3 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );
		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		rect.left = pxColumn1Left;
		rect.right = rect.left + ACHDISP_COLUMN_WIDTH;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[0] );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[1] );

		// Stats below the midline
		pxVertOffset = height / 2 + ACHDISP_VERT_SPACING - 1 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;
		
		DrawStatInfo( rect, "Games Played", static_cast<float>( m_nTotalGamesPlayed ) );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Games Won", static_cast<float>( m_nTotalNumWins ) );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Games Lost", static_cast<float>( m_nTotalNumLosses ) );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_LEFT|TEXTPOS_VCENTER, "Inventory" );

		std::list<CSteamItem *>::const_iterator iter;
		for ( iter = SpaceWarLocalInventory()->GetItemList().begin(); iter != SpaceWarLocalInventory()->GetItemList().end(); ++iter )
		{
			rect.top = pxVertOffset;
			rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
			pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

			DrawInventory( rect, ( *iter )->GetItemId() );
		}


		// COLUMN 2
		// Achievements above the midline
		pxVertOffset = height / 2 - 3 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );
		
		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		rect.left = pxColumn2Left;
		rect.right = rect.left + ACHDISP_COLUMN_WIDTH;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[2] );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_IMG_SIZE;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawAchievementInfo( rect, g_rgAchievements[3] );

		// Stats below the midline
		pxVertOffset = height / 2 + ACHDISP_VERT_SPACING - 1 * ( ACHDISP_IMG_SIZE + ACHDISP_VERT_SPACING );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Feet Traveled", m_flTotalFeetTraveled );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Max Feet Traveled", m_flMaxFeetTraveled );

		rect.top = pxVertOffset;
		rect.bottom = rect.top + ACHDISP_FONT_HEIGHT;
		pxVertOffset = rect.bottom + ACHDISP_VERT_SPACING;

		DrawStatInfo( rect, "Average Inches / Second", m_flAverageSpeed * 12.0f );
		
		// Footer
		rect.left = 0;
		rect.right = width;
		rect.top = LONG(m_pGameEngine->GetViewportHeight() * 0.8);
		rect.bottom = m_pGameEngine->GetViewportHeight();

		char rgchBuffer[256];
		sprintf_safe( rgchBuffer, "Press ESC to return to the Main Menu" );
		m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_CENTER|TEXTPOS_TOP, rgchBuffer );

	}
	*/
}

void CStatsAndAchievements::DrawAchievementInfo()
{
	/*
	if ( ach.m_iIconImage == 0 )
	{
		ach.m_iIconImage = m_pSteamUserStats->GetAchievementIcon( ach.m_pchAchievementID );
	}

	HGAMETEXTURE hTexture = SpaceWarClient()->GetSteamImageAsTexture( ach.m_iIconImage );

	// don't modify the caller's rect, they may use it later to locate something else
	RECT rect2 = rect;

	// could still be zero if the image isn't downloaded yet
	if (hTexture )
	{
		m_pGameEngine->BDrawTexturedRect( (float)rect2.left, (float)rect2.top, (float)rect2.left+ACHDISP_IMG_SIZE, (float)rect2.bottom, 
			0.0f, 0.0f, 1.0, 1.0, D3DCOLOR_ARGB( 255, 255, 255, 255 ), hTexture );

		rect2.left += ACHDISP_IMG_SIZE + ACHDISP_IMG_PAD;
	}

	// todo: divide up so can draw image
	char rgchBuffer[256];
	sprintf_safe( rgchBuffer, "%s: %s\n%s", 
		ach.m_rgchName,
		ach.m_bAchieved ? "Unlocked" : "Locked",
		ach.m_rgchDescription );

	m_pGameEngine->BDrawString( m_hDisplayFont, rect2, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_LEFT|TEXTPOS_VCENTER, rgchBuffer );
	*/
}

void CStatsAndAchievements::DrawStatInfo()
{
	/*
	// todo: divide up so can draw image
	char rgchBuffer[256];
	sprintf_safe( rgchBuffer, "%s: %.1f", pchName, flValue );
	m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_LEFT|TEXTPOS_VCENTER, rgchBuffer );
	*/
}

void CStatsAndAchievements::DrawInventory()
{
	/*
	const CSteamItem *pItem = SpaceWarLocalInventory()->GetItem( itemid );
	if ( !pItem )
		return;

	// todo: divide up so can draw image
	char rgchBuffer[256];
	sprintf_safe( rgchBuffer, "%s", pItem->GetLocalizedName().c_str() );
	m_pGameEngine->BDrawString( m_hDisplayFont, rect, D3DCOLOR_ARGB( 255, 25, 200, 25 ), TEXTPOS_LEFT|TEXTPOS_VCENTER, rgchBuffer );
	*/
}
/*
bool CStatsAndAchievements::LoadUserStatsOnPS3()
{
#ifdef _PS3
	// On PS3, we need to load the user's stats & achievement information from the save container. In this example, we are simply
	// reading the data from a known location on disk
	FILE *file = fopen( GetUserSaveDataPath(), "rb" );
	if ( !file )
	{
		// we need to tell Steam that there is no data
		SteamUserStats()->SetUserStatsData( NULL, 0 );
		return true;
	}

	fseek( file, 0, SEEK_END );
	long nSize = ftell( file );
	fseek( file, 0, SEEK_SET );

	byte *buffer = new byte[nSize];
	fread( buffer, 1, nSize, file );
	fclose( file );

	bool bRet = SteamUserStats()->SetUserStatsData( buffer, nSize );
	delete [] buffer;

	return bRet;
#else
	return true;
#endif
}

bool CStatsAndAchievements::SaveUserStatsOnPS3()
{
#ifdef _PS3
	// On PS3, we need to save the user's stats & achievement information into the save container. In this example, we are simply
	// saving the data to a known location on disk

	// get required buffer size
	uint32 unSize = 0;
	if ( !SteamUserStats()->GetUserStatsData( NULL, 0, &unSize ) && unSize == 0 )
		return false;

	// get data
	byte *buffer = new byte[unSize];
	uint32 unWritten = 0;
	bool bRet = SteamUserStats()->GetUserStatsData( buffer, unSize, &unWritten );
	if ( bRet )
	{
		FILE *file = fopen( GetUserSaveDataPath(), "wb" );
		if ( file )
		{
			bRet = (fwrite( buffer, 1, unWritten, file ) == unWritten);
			fclose( file );
		}
		else
		{
			bRet = false;
		}
	}

	delete [] buffer;

	return bRet;
#else
	return true;
#endif
}
*/

#endif