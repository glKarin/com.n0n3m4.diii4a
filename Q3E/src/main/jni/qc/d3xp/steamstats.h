#ifdef STEAM
#define _STAT_ID( id,type,name ) { id, type, name, 0, 0, 0, 0 }

enum EStatTypes
{
	STAT_INT = 0,
	STAT_FLOAT = 1,
	STAT_AVGRATE = 2,
};

struct Stat_t
{
	int m_ID;						//MAKE SURE index number matches steam admin page.
	EStatTypes m_eStatType;
	const char *m_pchStatName;		//name string.
	int m_iValue;					//integer value.
	float m_flValue;				//float value.
	float m_flAvgNumerator;
	float m_flAvgDenominator;
};

class CSteamStats
{
private:
	int64 m_iAppID; // Our current AppID
	Stat_t *m_pStats; // Stats data
	int m_iNumStats; // The number of Stats
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamStats(Stat_t *Stats);
	~CSteamStats();
	
	int GetNumStats();

	bool RequestStats();
	bool StoreStats();
	bool SetStat( const char *statname, int value );
	bool GetStat( const char *statname, int &value );
	bool SetStatDelta( const char *statname, int delta);

	bool RequestUserStats(CSteamID id);
	bool GetUserStat(CSteamID id, const char *statname, int &value);


	void RequestGlobalStats( void );
	void OnRequestGlobalStats( GlobalStatsReceived_t *pResult, bool bIOFailure );
	CCallResult<CSteamStats, GlobalStatsReceived_t> m_SteamCallResultRequestGlobalStats;
	
	bool GetGlobalStat(const char *statname, int &output);

	void ResetStats( void );

	bool SetAchievement(const char *achievementName);

	STEAM_CALLBACK( CSteamStats, OnUserStatsReceived, UserStatsReceived_t, 
		m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CSteamStats, OnUserStatsStored, UserStatsStored_t, 
		m_CallbackUserStatsStored );
};

extern CSteamStats *		g_SteamStats;
extern Stat_t				g_Stats[];


#endif