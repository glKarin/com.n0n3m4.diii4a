#ifdef STEAM

#define MAXTOPFRIENDS	8    //list shown when you complete a level.
#define MAXNOTEFRIENDS	32   //max amount of friends who can populate notes in map.

#define MAXUPDATEMONITORFRIENDS	12   //max amount of friends on the hideout monitor.

class CSteamLeaderboard
{
private:

	SteamLeaderboard_t	m_leaderboard_Notes;
	SteamLeaderboard_t	m_leaderboard_Times[3];


	// INITIALIZE.
	void			OnInitializeFindDownloadLeaderboard( LeaderboardFindResult_t *pCallback,	bool bIOFailure );
	CCallResult<CSteamLeaderboard, LeaderboardFindResult_t> m_InitializeFindLeaderboard[3];




	//SENDING.
	const char *		m_mapname;
	UGCHandle_t			m_shareFile;
	bool				isSending;

	void OnUploadFileWrite(RemoteStorageFileShareResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamLeaderboard, RemoteStorageFileShareResult_t > m_OnUploadFileWrite;



	void			OnFindUploadLeaderboard( LeaderboardFindResult_t *pCallback,	bool bIOFailure );
	CCallResult<CSteamLeaderboard, LeaderboardFindResult_t> m_uploadResultFindLeaderboard;


	void			OnUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoreUploaded_t> m_uploadScore;


	void			OnUploadAttachUGC(LeaderboardUGCSet_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardUGCSet_t> m_uploadAttachUGC;
	

	//RETRIEVING.
	int					downloadMax;
	int					downloadIndex;
	bool				isLocal;
	void				DownloadLeaderboardScores(const char *mapname);
	LeaderboardEntry_t	m_leaderboardEntries[MAXNOTEFRIENDS];

	void			OnFindDownloadLeaderboard( LeaderboardFindResult_t *pCallback,	bool bIOFailure );
	CCallResult<CSteamLeaderboard, LeaderboardFindResult_t> m_downloadResultFindLeaderboard;


	void			OnDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoresDownloaded_t> m_downloadLeaderboard;


	void			DownloadAndParseFile(UGCHandle_t handle);
	CCallResult<CSteamLeaderboard, RemoteStorageDownloadUGCResult_t > m_SteamCallResultDownloadFile;

	void			OnDownloadFile( RemoteStorageDownloadUGCResult_t  *pResult, bool bIOFailure );
	void			DownloadNextFile( void );


	//SCORE: UPLOAD SCORE AND THEN RETRIEVE LEADERBOARD.
	bool			uploadingScore;

	void			OnScoreUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoreUploaded_t> m_scoreUploadScore;

	void			OnScoreDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoresDownloaded_t> m_scoreDownloadLeaderboard;


	//UPDATEMONITOR
	void			OnUpdateMonitorDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoresDownloaded_t> m_updateMonitorDownloadLeaderboard;


	//GET "STARTUP" FRIEND LIST
	void			OnFriendFindLeaderboard( LeaderboardFindResult_t *pCallback,	bool bIOFailure );
	CCallResult<CSteamLeaderboard, LeaderboardFindResult_t> m_FriendFindLeaderboard;

	void			OnFriendUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoreUploaded_t> m_friendUploadScore;

	void			OnFriendDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoresDownloaded_t> m_friendDownloadLeaderboard;


	//MAIN MENU FIND LEADERBOARDS.
	void			OnFindMainmenuLeaderboards(LeaderboardFindResult_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardFindResult_t> m_mainmenuFindLeaderboard[64];


	//MAIN MENU UPDATE LEADERBOARDS.
	void			OnDownloadMainmenuLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure);
	CCallResult<CSteamLeaderboard, LeaderboardScoresDownloaded_t> m_downloadMainmenuLeaderboard;


public:
	CSteamLeaderboard();
	~CSteamLeaderboard();

	void			Initialize( const char *mapname, int mapAmount);
	
	bool			SendStickynotes(const char *mapname);
	void			GetStickynotes(const char *mapname);

	void			UploadScore(const char *mapname, int score, int missionIndex);

	void			UpdateLeadermonitor(int missionIndex);

	void			GetFriendBoard(void);


	void			GetMainmenuLeaderboards(int index, const char *statname);
	void			UpdateMainmenuLeaderboards(SteamLeaderboard_t board);


};

extern CSteamLeaderboard *		g_SteamLeaderboard;



#endif