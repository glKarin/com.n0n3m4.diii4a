#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef STEAM
#include "steamstats.h"

#include "Game_local.h"

//           index  //   stat type  // statname on admin page.
//NOTE this list has to retain the exact same order as the Steam admin page.
Stat_t g_Stats[] =
{
	


	_STAT_ID( 20, STAT_INT, "time_val_0"),
	_STAT_ID( 32, STAT_INT, "attempts_val_0"),

	_STAT_ID( 33, STAT_INT, "attempts_satsuma_0"),
	_STAT_ID( 37, STAT_INT, "attempts_satsuma_1"),
	_STAT_ID( 38, STAT_INT, "attempts_satsuma_2"),

	_STAT_ID( 48, STAT_INT, "attempts_funi_0"),
	_STAT_ID( 49, STAT_INT, "attempts_funi_1"),
	_STAT_ID( 50, STAT_INT, "attempts_funi_2"),

	_STAT_ID( 51, STAT_INT, "attempts_towers2_0"),
	_STAT_ID( 52, STAT_INT, "attempts_towers2_1"),
	_STAT_ID( 53, STAT_INT, "attempts_towers2_2"),

	_STAT_ID( 54, STAT_INT, "attempts_villa_0"),
	_STAT_ID( 55, STAT_INT, "attempts_villa_1"),
	_STAT_ID( 56, STAT_INT, "attempts_villa_2"),




	_STAT_ID( 34, STAT_INT, "time_satsuma_0"),
	_STAT_ID( 35, STAT_INT, "time_satsuma_1"),
	_STAT_ID( 36, STAT_INT, "time_satsuma_2"),

	_STAT_ID( 42, STAT_INT, "time_funi_0"),
	_STAT_ID( 43, STAT_INT, "time_funi_1"),
	_STAT_ID( 44, STAT_INT, "time_funi_2"),

	_STAT_ID( 45, STAT_INT, "time_towers2_0"),
	_STAT_ID( 46, STAT_INT, "time_towers2_1"),
	_STAT_ID( 47, STAT_INT, "time_towers2_2"),

	_STAT_ID( 39, STAT_INT, "time_villa_0"),
	_STAT_ID( 40, STAT_INT, "time_villa_1"),
	_STAT_ID( 41, STAT_INT, "time_villa_2"),



	_STAT_ID( 21, STAT_INT, "stat_deckcommands"),
	_STAT_ID( 22, STAT_INT, "stat_death_oxygen"),
	_STAT_ID( 23, STAT_INT, "stat_death_other"),
	_STAT_ID( 24, STAT_INT, "stat_launches_player"),
	_STAT_ID( 25, STAT_INT, "stat_throws"),
	_STAT_ID( 26, STAT_INT, "stat_clambers"),
	_STAT_ID( 27, STAT_INT, "stat_glassbreaks"),
	_STAT_ID( 28, STAT_INT, "stat_stickynotes"),
	_STAT_ID( 29, STAT_INT, "stat_buzzsaws"),
	_STAT_ID( 30, STAT_INT, "stat_screws"),
	_STAT_ID( 31, STAT_INT, "stat_bugreports"),
};




// Global access to Stats Object
CSteamStats* g_SteamStats = NULL;



CSteamStats::CSteamStats(Stat_t *Stats) :
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackUserStatsReceived( this, &CSteamStats::OnUserStatsReceived ),
 m_CallbackUserStatsStored( this, &CSteamStats::OnUserStatsStored )
{
	int numStats = sizeof(g_Stats) / sizeof(g_Stats[0]);

	m_iAppID = SteamUtils()->GetAppID();
	m_pStats = Stats;
	m_iNumStats = numStats;
	RequestStats();
}

CSteamStats::~CSteamStats()
{
	m_CallbackUserStatsReceived.Unregister();
	m_CallbackUserStatsStored.Unregister();
}

void CSteamStats::ResetStats()
{
	if (SteamUserStats()->ResetAllStats(true))
	{
		common->Printf("STEAM: STATS RESET.\n");
	}
}

int CSteamStats::GetNumStats()
{
	return (sizeof(g_Stats) / sizeof(g_Stats[0]));
}

bool CSteamStats::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		common->Warning("STEAM: Steam is not loaded. (RequestStats)");
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		common->Warning("STEAM: User not logged on. (RequestStats)");
		return false;
	}
	// Request user stats.
	if (!SteamUserStats()->RequestCurrentStats())
	{
		common->Warning("STEAM: problem requesting stats (RequestStats).\n");
		return false;
	}

	common->Printf("STEAM: recieved stats. (RequestStats)\n");

	return true;
}

bool CSteamStats::RequestUserStats(CSteamID id)
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		common->Warning("STEAM: Steam is not loaded. (RequestUserStats)");
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		common->Warning("STEAM: User not logged on. (RequestUserStats)");
		return false;
	}

	// Request user stats.
	if (!SteamUserStats()->RequestUserStats(id))
	{
		common->Warning("STEAM: problem requesting stats (RequestUserStats).\n");
		return false;
	}

	return true;
}


void CSteamStats::RequestGlobalStats()
{
	SteamAPICall_t hSteamAPICall = SteamUserStats()->RequestGlobalStats(0);
	m_SteamCallResultRequestGlobalStats.Set( hSteamAPICall, this, &CSteamStats::OnRequestGlobalStats );
}

void CSteamStats::OnRequestGlobalStats(GlobalStatsReceived_t *pResult, bool bIOFailure)
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: failed to request global stats. Result = %d\n", (int)pResult->m_eResult);
		return;
	}

	//success.
	/*
	int64 output;
	GetGlobalStat("m0j0_time", output);
	common->Printf("%d\n", output);*/
}

//GLOBAL STATS.
bool CSteamStats::GetGlobalStat(const char *statname, int &output)
{
	if (!gameLocal.steamInitialized)
	{
		common->Warning("STEAM: Steam is not initialized. (GetGlobalStat)");
		return false;
	}


	



	int64 value;
	if (!SteamUserStats()->GetGlobalStat(statname, &value))
	{
		common->Warning("STEAM: failed to get global stat (GetGlobalStat): %s\n", statname);
		return false;
	}

	//success.
	output = value;
	return true;
}

bool CSteamStats::GetStat(const char *statname, int &value )
{
	if (!gameLocal.steamInitialized)
	{
		return false;
	}

	if (!SteamUserStats()->GetStat(statname, &value))
	{
		common->Warning("STEAM: failed to get stat.\n");
		return false;
	}

	return true;
}

bool CSteamStats::SetStatDelta(const char *statname, int delta )
{
	if (!gameLocal.steamInitialized)
	{
		return false;
	}

	if (!m_bInitialized)
	{
		common->Warning("STEAM: cannot set stat before stat initialization. (SetStatDelta)");
		return false;
	}

	for (int i = 0; i < m_iNumStats; i++)
	{
		if ( idStr::Icmp(g_Stats[i].m_pchStatName, statname) == 0)
		{
			g_Stats[i].m_iValue += delta;
			return SetStat(statname, g_Stats[i].m_iValue);
		}
	}

	return false;
}

bool CSteamStats::SetStat(const char *statname, int value )
{
	if (!gameLocal.steamInitialized)
	{
		return false;
	}

	if (!m_bInitialized)
	{
		common->Warning("STEAM: cannot set stat before stat initialization. (SetStat)");
		return false;
	}

	if (!SteamUserStats()->SetStat(statname, value))
	{
		common->Warning("STEAM: failed to set stat: %s (SetStat)", statname);
		return false;
	}

	if (!SteamUserStats()->StoreStats())
	{
		common->Warning("STEAM: failed to store stat. (SetStat)\n");
		return false;
	}

	return true;
}

//Store user stats.
bool CSteamStats::StoreStats()
{
	if ( !m_bInitialized )
	{
		common->Warning("STEAM: Steam is not initialized. (StoreStats)");
		return false;
	}

	// load stats
	int m_NumStats = sizeof(g_Stats) / sizeof(g_Stats[0]);

	for ( int iStat = 0; iStat < m_NumStats; ++iStat )
	{
		Stat_t &stat = m_pStats[iStat];

		switch (stat.m_eStatType)
		{
			case STAT_INT:
				SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_iValue);
				break;

			case STAT_FLOAT:
				SteamUserStats()->SetStat(stat.m_pchStatName, stat.m_flValue);
				break;

			case STAT_AVGRATE:
				SteamUserStats()->UpdateAvgRateStat(stat.m_pchStatName, stat.m_flAvgNumerator, stat.m_flAvgDenominator );
				SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue );
				break;

			default:
				break;
		}
	}
   
	if (!SteamUserStats()->StoreStats())
	{
		common->Warning("STEAM: problem storing stats (StoreStats).\n");
		return false;
	}

	return true;	
}


//Retrieve user stats.
void CSteamStats::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	if (m_iAppID != pCallback->m_nGameID)
	{
		//app id doesn't match.
		return;
	}

	if ( k_EResultOK != pCallback->m_eResult )
	{
		common->Warning("STEAM: Failed to receive stats. (OnUserStatsReceived)");
		return;
	}

	for (int i = 0; i < m_iNumStats; i++)
	{
		Stat_t &stat = g_Stats[i];

		switch (stat.m_eStatType)
		{
			case STAT_INT:
				SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_iValue);
				//common->Printf("STEAM: Stat received (int): %s = %d\n", stat.m_pchStatName, stat.m_iValue);
				break;

			case STAT_FLOAT:
			case STAT_AVGRATE:
				SteamUserStats()->GetStat(stat.m_pchStatName, &stat.m_flValue);
				//common->Printf("STEAM: Stat received (float): %s = %f\n",stat.m_pchStatName, stat.m_flValue);
				break;

			default:
				break;
		}
	}

	//common->Printf("STEAM: Received stats.\n");
	m_bInitialized = true;
}



void CSteamStats::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	if ( m_iAppID != pCallback->m_nGameID )
	{
		return;
	}

	if ( k_EResultInvalidParam == pCallback->m_eResult )
	{
		// One or more stats we set broke a constraint. They've been reverted,
		// and we should re-iterate the values now to keep in sync.
		common->Warning("STEAM: StoreStats - some failed to validate\n" );
		// Fake up a callback here so that we re-load the values.
		UserStatsReceived_t callback;
		callback.m_eResult = k_EResultOK;
		callback.m_nGameID = m_iAppID;
		OnUserStatsReceived( &callback );
	}
}

bool CSteamStats::GetUserStat(CSteamID id, const char *statname, int &value)
{
	return SteamUserStats()->GetUserStat(id, statname, &value);
}

bool CSteamStats::SetAchievement(const char *achievementName)
{
	if (g_skill.GetInteger() <= 0)
	{
		return false;
	}

	idPlayer	*player;
	player = gameLocal.GetLocalPlayer();

	if (!player)
	{
		return false;
	}

	if ( player->godmode || player->noclip )
	{
		return false;
	}


	if (!SteamUserStats()->SetAchievement(achievementName))
	{
		common->Warning("STEAM: failed to set achievement. (SetAchievement)\n");
		return false;
	}

	if (!SteamUserStats()->StoreStats())
	{
		common->Warning("STEAM: failed to store stat. (SetAchievement)\n");
		return false;
	}

	return true;
}

#endif
