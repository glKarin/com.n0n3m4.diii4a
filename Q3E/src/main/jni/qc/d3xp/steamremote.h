#ifdef STEAM

class CSteamRemote
{
private:
	int64 m_iAppID; // Our current AppID

	
	void	WriteFile(const char *text);	
	void	FileUpdateRequest();
	void	GetPublishedFiles();
	void	DownloadAndParseFile(UGCHandle_t handle);
	void	DownloadNextFile();
	void	GetFriendFiles();

	

	bool				isQuerying;
	UGCQueryHandle_t	queryHandle;
	int					downloadMax;
	int					downloadIdx;
	bool				isSending;
	

	void OnGetSelfFiles( RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageEnumerateUserSharedWorkshopFilesResult_t> m_SteamCallResultGetSelfFiles;	



	void OnGetFilesToDelete( RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageEnumerateUserSharedWorkshopFilesResult_t> m_SteamCallResultGetFilesToDelete;	


	void OnGetPublishedFiles( RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageEnumerateUserSharedWorkshopFilesResult_t> m_SteamCallResultGetPublishedFiles;	


	void OnWriteFile( RemoteStorageFileShareResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageFileShareResult_t > m_SteamCallResultWriteFile;

	void OnPublishFile( RemoteStoragePublishFileResult_t  *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStoragePublishFileResult_t > m_SteamCallResultPublishFile;

	void OnGetFileDetails( RemoteStorageGetPublishedFileDetailsResult_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageGetPublishedFileDetailsResult_t > m_SteamCallResultGetFileDetails;

	void OnDownloadFile( RemoteStorageDownloadUGCResult_t  *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageDownloadUGCResult_t > m_SteamCallResultDownloadFile;

	void OnDownloadSelfFile( RemoteStorageDownloadUGCResult_t  *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageDownloadUGCResult_t > m_SteamCallResultDownloadSelfFile;

	void OnCommitFileUpdate( RemoteStorageUpdatePublishedFileResult_t   *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, RemoteStorageUpdatePublishedFileResult_t  > m_SteamCallResultCommitFileUpdate;
	
	void OnUGCQueryCompleted( SteamUGCQueryCompleted_t *pResult, bool bIOFailure );
	CCallResult<CSteamRemote, SteamUGCQueryCompleted_t > m_SteamCallResultUGCQueryCompleted;
	
	const char *		bufferText;

		

public:
	CSteamRemote();
	~CSteamRemote();

	void	SendStickynotes( void );
	void	GetAllUserFiles( void );
	void	DeleteAll( void );

	void	DownloadMods( int& amount, idStrList& workshopModNames );


	//CLOUD SAVEGAMES.
	void	CloudLoad();

};

extern CSteamRemote *		g_SteamRemote;

#endif