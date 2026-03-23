#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef STEAM
#include "steamleaderboard.h"

#include "Game_local.h"

#define TEXTSIZE		131072
#define STICKYNOTEFILE	"stickynotes.txt"
//#define STICKYNOTETAG	"stickynotes"
#define MAXFRIENDLISTCOUNT	128

#define MAXMAINMENULEADERBOARDCOUNT 15

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

 //also change m_leaderboardEntries in the header file.


// Global access to object
CSteamLeaderboard* g_SteamLeaderboard = NULL;

CSteamLeaderboard::CSteamLeaderboard()
{
	downloadMax = 0;
	downloadIndex = 0;

	m_mapname = NULL;
	isSending = false;
	isLocal = false;
	uploadingScore = false;

	m_leaderboard_Notes = NULL;

	for (int i = 0; i < 3; i++)
	{
		m_leaderboard_Times[i] = NULL;
	}
}

CSteamLeaderboard::~CSteamLeaderboard()
{
}
// ======================================== INITIALIZE.

void CSteamLeaderboard::Initialize(const char *mapname, int missionAmount)
{
	idStr map = mapname;
	map.StripPath();
	map.StripFileExtension();
	map.ToLower();
	
	//find the appropriate leaderboard.
	for (int i = 0; i < missionAmount; i++)
	{
		SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(va("times_%s_%d",map.c_str(), i));
		m_InitializeFindLeaderboard[i].Set(hSteamAPICall, this, &CSteamLeaderboard::OnInitializeFindDownloadLeaderboard);
	}
}

void CSteamLeaderboard::OnInitializeFindDownloadLeaderboard(LeaderboardFindResult_t *pCallback, bool bIOFailure)
{
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		common->Warning("STEAM: failed to find leaderboard (OnInitializeFindDownloadLeaderboard).");
		isSending = false;
		return;
	}

	idStr leaderboardName = SteamUserStats()->GetLeaderboardName( pCallback->m_hSteamLeaderboard );

	idStr digits = leaderboardName.Right( 2 );

	//common->Printf("found %s\n", digits.c_str());

	if ( idStr::Icmp( digits, "_0") == 0 )
	{
		m_leaderboard_Times[0] = pCallback->m_hSteamLeaderboard;
	}
	else if ( idStr::Icmp( digits, "_1") == 0 )
	{
		m_leaderboard_Times[1] = pCallback->m_hSteamLeaderboard;
	}
	else if ( idStr::Icmp( digits, "_2") == 0 )
	{
		m_leaderboard_Times[2] = pCallback->m_hSteamLeaderboard;
	}
}




// ======================================== BEGIN SENDING.

bool CSteamLeaderboard::SendStickynotes(const char *mapname)
{
	if ( isSending )
	{
		return false;
	}

	isSending = true;

	//const char *uploadText;
	bool bFileWrite;
	int i;
	idStr output = "";

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
		{
			continue;
		}

		if (!gameLocal.entities[i]->IsType(idStaticEntity::Type) || gameLocal.entities[i]->spawnArgs.GetInt("stickynote", "0") <= 0
			|| gameLocal.entities[i]->spawnArgs.GetInt("local", "0") <= 0)
		{
			continue;
		}	

		output += va("{\nnametag %s\ntext %s\nmap %s\norigin %.2f %.2f %.2f\nrotation %s\n}\n",
			
			gameLocal.entities[i]->GetRenderEntity()->gui[0]->GetStateString("gui_parm0"),
			gameLocal.entities[i]->GetRenderEntity()->gui[0]->GetStateString("gui_parm1"),

			gameLocal.GetMapName(),
			
			gameLocal.entities[i]->GetPhysics()->GetOrigin().x,
			gameLocal.entities[i]->GetPhysics()->GetOrigin().y,
			gameLocal.entities[i]->GetPhysics()->GetOrigin().z,

			gameLocal.entities[i]->GetPhysics()->GetAxis().ToString(8)		

			);
	}

	//m_mapname = NULL;

	
	m_mapname = mapname;
	//uploadText = output;	

	bFileWrite = SteamRemoteStorage()->FileWrite( STICKYNOTEFILE, output, (int) strlen( output ) );
	//common->Printf("output: %s\n", output.c_str());

	if (!bFileWrite)
	{
		common->Warning("STEAM: filewrite fail.");
		isSending = false;
		return false;
	}

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->FileShare(STICKYNOTEFILE);
	m_OnUploadFileWrite.Set( hSteamAPICall, this, &CSteamLeaderboard::OnUploadFileWrite );
	return true;
}

void CSteamLeaderboard::OnUploadFileWrite(RemoteStorageFileShareResult_t *pResult, bool bIOFailure )
{
	idStr mapname;

	if (pResult->m_eResult != k_EResultOK || bIOFailure)
	{
		common->Warning("STEAM: problem writing file (OnUploadFileWrite()).\n");
		isSending = false;
		return;
	}

	//FileShare is successful.
	m_shareFile = pResult->m_hFile;

	mapname = m_mapname;
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	if (m_leaderboard_Notes == NULL)
	{
		//find the appropriate leaderboard.
		SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(va("notes_%s",mapname.c_str()));
		m_uploadResultFindLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnFindUploadLeaderboard);
	}
	else
	{
		//shortcut.		
		SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore( m_leaderboard_Notes, k_ELeaderboardUploadScoreMethodForceUpdate, 0, NULL, 0 );
		m_uploadScore.Set(hSteamAPICall, this, &CSteamLeaderboard::OnUploadScore);
	}
}

void CSteamLeaderboard::OnFindUploadLeaderboard( LeaderboardFindResult_t *pCallback, bool bIOFailure )
{
	// see if we encountered an error during the call
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		common->Warning("STEAM: failed to find leaderboard (upload).");
		isSending = false;
		return;
	}

	m_leaderboard_Notes = pCallback->m_hSteamLeaderboard;
	
	SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore( pCallback->m_hSteamLeaderboard, k_ELeaderboardUploadScoreMethodForceUpdate, 0, NULL, 0 );
	m_uploadScore.Set(hSteamAPICall, this, &CSteamLeaderboard::OnUploadScore);

}

void CSteamLeaderboard::OnUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure)
{
	if ( pCallback->m_bSuccess < 1 || bIOFailure )
	{
		common->Warning("STEAM: failed to upload leaderboard score. bSuccess: %d", pCallback->m_bSuccess);
		isSending = false;
		return;
	}

	SteamAPICall_t hSteamAPICall = SteamUserStats()->AttachLeaderboardUGC( pCallback->m_hSteamLeaderboard, m_shareFile );
	m_uploadAttachUGC.Set(hSteamAPICall, this, &CSteamLeaderboard::OnUploadAttachUGC);
}


void CSteamLeaderboard::OnUploadAttachUGC(LeaderboardUGCSet_t *pCallback, bool bIOFailure)
{
	if ( pCallback->m_eResult != k_EResultOK || bIOFailure )
	{
		common->Warning("STEAM: failed to attach leaderboard UGC (%d).", pCallback->m_eResult);
		isSending = false;
		return;
	}

	common->Printf("STEAM: uploaded stickynotes.\n");
	isSending = false;
}



// ====================================== DONE SENDING.




// ====================================== START RETRIEVING.

void CSteamLeaderboard::GetStickynotes(const char *mapname)
{
	idStr mapshort = mapname;
	mapshort.StripPath();
	mapshort.StripFileExtension();
	mapshort.ToLower();

	//Now find the appropriate leaderboard.
	SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(va("notes_%s",mapshort.c_str()));
	m_downloadResultFindLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnFindDownloadLeaderboard);
}

void CSteamLeaderboard::OnFindDownloadLeaderboard( LeaderboardFindResult_t *pCallback, bool bIOFailure )
{
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		common->Warning("STEAM: failed to find leaderboard (download).");
		return;
	}

	m_leaderboard_Notes = pCallback->m_hSteamLeaderboard;

	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(pCallback->m_hSteamLeaderboard,
		k_ELeaderboardDataRequestFriends, 0,0);
	m_downloadLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnDownloadLeaderboard);
}

void CSteamLeaderboard::OnDownloadLeaderboard(LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	int totalEntries;
	int index;

	if (bIOFailure)
	{
		common->Warning("STEAM: failed to download leaderboard. (OnDownloadLeaderboard)");
		return;
	}
   
	totalEntries = min(pCallback->m_cEntryCount, MAXNOTEFRIENDS);

	if (totalEntries <= 0)
	{
		common->Printf("STEAM: no stickynotes found for this map.\n");
		return;
	}

	for (index = 0; index < totalEntries; index++)
	{
		if (!SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, index, &m_leaderboardEntries[index], NULL, 0))
		{
			common->Warning("STEAM: problem downloading leaderboard entry.");
		}
	}

	//ok we got all the entries. Start downloading.

	downloadIndex = -1;
	downloadMax = totalEntries;
	isLocal = false;

	DownloadNextFile();
}

void CSteamLeaderboard::DownloadAndParseFile(UGCHandle_t handle)
{
	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->UGCDownload(handle, 0);
	m_SteamCallResultDownloadFile.Set( hSteamAPICall, this, &CSteamLeaderboard::OnDownloadFile );
}

void CSteamLeaderboard::OnDownloadFile(RemoteStorageDownloadUGCResult_t *pResult, bool bIOFailure)
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem downloading leaderboard file. Result = %d (OnDownloadFile()).\n", (int)pResult->m_eResult);
		DownloadNextFile();
		return;
	}

	char testmessage[TEXTSIZE];
	int32 cubRead = SteamRemoteStorage()->UGCRead(pResult->m_hFile, testmessage, sizeof(testmessage), 0, k_EUGCRead_ContinueReadingUntilFinished);
	testmessage[cubRead] = 0; // null-terminate

	gameLocal.ParseStickynotes(testmessage, isLocal); //parse friend note.

	DownloadNextFile();
}

void CSteamLeaderboard::DownloadNextFile( void )
{
	downloadIndex++;

	if (downloadIndex >= downloadMax)
	{
		common->Printf("STEAM: completed downloading sticky notes.\n");
		return;
	}

	isLocal = (m_leaderboardEntries[downloadIndex].m_steamIDUser.GetAccountID() == SteamUser()->GetSteamID().GetAccountID());		

	DownloadAndParseFile(m_leaderboardEntries[downloadIndex].m_hUGC);
}




// ====================================== SCORE: UPLOAD SCORE AND THEN RETRIEVE LEADERBOARD.

void CSteamLeaderboard::UploadScore(const char *mapname, int score, int missionIndex)
{
	if (uploadingScore)
	{
		common->Warning("STEAM: cannot upload new score. Currently busy.");
		return;
	}

	if (m_leaderboard_Times[missionIndex] == NULL)
	{
		common->Warning("STEAM: leaderboard variable initialization is null.");
		return;
	}

	uploadingScore = true;

	SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore( m_leaderboard_Times[missionIndex], k_ELeaderboardUploadScoreMethodKeepBest, score, NULL, 0 );
	m_scoreUploadScore.Set(hSteamAPICall, this, &CSteamLeaderboard::OnScoreUploadScore);	
}



void CSteamLeaderboard::OnScoreUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure)
{
	if ( pCallback->m_bSuccess < 1 || bIOFailure )
	{
		common->Warning("STEAM: failed to upload leaderboard score. (OnScoreUploadScore)");
		uploadingScore = false;
		return;
	}

	//successful score upload.
	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(pCallback->m_hSteamLeaderboard,
		k_ELeaderboardDataRequestFriends, 0,0);
	m_scoreDownloadLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnScoreDownloadLeaderboard);
}

void CSteamLeaderboard::OnScoreDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	int					totalEntries;
	int					i;
	LeaderboardEntry_t	m_scoreEntries[MAXTOPFRIENDS];

	if (bIOFailure)
	{
		common->Warning("STEAM: failed to download leaderboard. (OnScoreDownloadLeaderboard)");
		uploadingScore = false;
		return;
	}

	totalEntries = pCallback->m_cEntryCount;

	if (totalEntries <= 0)
	{
		common->Warning("STEAM: empty leaderboard. (OnScoreDownloadLeaderboard)");
		uploadingScore = false;
		return;
	}
	
	//ready to populate the list.
	gameLocal.GetLocalPlayer()->topfriendnameList->Clear();
	gameLocal.GetLocalPlayer()->topfriendtimeList->Clear();

	idListGUI * guiList_people = uiManager->AllocListGUI();
	guiList_people->Clear();
	guiList_people->Config( gameLocal.GetLocalPlayer()->hud, "topfriendnames" );

	idListGUI * guiList_times = uiManager->AllocListGUI();
	guiList_times->Clear();
	guiList_times->Config( gameLocal.GetLocalPlayer()->hud, "topfriendtimes" );	

	totalEntries = min(totalEntries, MAXTOPFRIENDS);

	for (i = 0; i < totalEntries; i++)
	{
		if (SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, i, &m_scoreEntries[i], NULL, 0))
		{
			//m_scoreEntries[i].m_nScore
			guiList_people->Push( va("%s", SteamFriends()->GetFriendPersonaName( m_scoreEntries[i].m_steamIDUser) ));
			guiList_times->Push( va("%s", gameLocal.ParseTimeMS(m_scoreEntries[i].m_nScore)  ) );
		}
		else
		{
			//common->Warning("STEAM: problem downloading leaderboard entry. (OnScoreDownloadLeaderboard)");
		}
	}

	uiManager->FreeListGUI( guiList_people );
	uiManager->FreeListGUI( guiList_times );

	uploadingScore = false;
}

void CSteamLeaderboard::UpdateLeadermonitor(int missionIndex)
{
	idEntity *targetScreen;

	if (m_leaderboard_Times[missionIndex] == NULL)
	{
		common->Warning("STEAM: failed to find initialized leaderboard. (UpdateLeadermonitor)");
		return;
	}

	
	targetScreen = gameLocal.FindEntity("screen_timelimit");
	if (!targetScreen)
	{
		common->Warning("STEAM: failed to find 'screen_timelimit' monitor. (UpdateLeadermonitor)");
		return;
	}

	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(m_leaderboard_Times[missionIndex], k_ELeaderboardDataRequestFriends, 0, 0);
	m_updateMonitorDownloadLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnUpdateMonitorDownloadLeaderboard);
}



void CSteamLeaderboard::OnUpdateMonitorDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	int					totalEntries;
	int					i;
	LeaderboardEntry_t	m_scoreEntries[MAXUPDATEMONITORFRIENDS];
	idEntity			*targetScreen;

	if (bIOFailure)
	{
		common->Warning("STEAM: failed to download leaderboard. (OnUpdateMonitorDownloadLeaderboard)");
		return;
	}

	totalEntries = pCallback->m_cEntryCount;

	if (totalEntries <= 0)
	{
		common->Warning("STEAM: empty leaderboard. (OnUpdateMonitorDownloadLeaderboard)");
		return;
	}

	targetScreen = gameLocal.FindEntity("screen_timelimit");
	if (!targetScreen)
	{
		common->Warning("STEAM: failed to find 'screen_timelimit' monitor. (OnUpdateMonitorDownloadLeaderboard)");
		return;
	}

	idUserInterface *screenGui = targetScreen->GetRenderEntity()->gui[0]; //get the gui surface.
	


	gameLocal.GetLocalPlayer()->topfriendnameList->Clear();
	gameLocal.GetLocalPlayer()->topfriendtimeList->Clear();

	

	idListGUI * guiList_people = uiManager->AllocListGUI();
	guiList_people->Clear();
	guiList_people->Config( screenGui, "topfriendnames" );

	idListGUI * guiList_times = uiManager->AllocListGUI();
	guiList_times->Clear();
	guiList_times->Config( screenGui, "topfriendtimes" );	

	totalEntries = min(totalEntries, MAXUPDATEMONITORFRIENDS);








	for (i = 0; i < MAXUPDATEMONITORFRIENDS; i++)
	{
		if ( i <= totalEntries)
		{
			if (SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, i, &m_scoreEntries[i], NULL, 0))
			{
				//common->Printf("friend monitor %s\n", SteamFriends()->GetFriendPersonaName( m_scoreEntries[i].m_steamIDUser));

				guiList_people->Push( va("%s", SteamFriends()->GetFriendPersonaName( m_scoreEntries[i].m_steamIDUser) ));
				guiList_times->Push( va("%s", gameLocal.ParseTimeMS(m_scoreEntries[i].m_nScore)  ) );
				continue;
			}
			else
			{
				//common->Warning("STEAM: problem downloading leaderboard entry. (OnUpdateMonitorDownloadLeaderboard)");
			}
		}
		else
		{
			//empty out the rest of the entries.
			guiList_people->Push( " " );
			guiList_times->Push( " " );
		}
	}

	uiManager->FreeListGUI( guiList_people );
	uiManager->FreeListGUI( guiList_times );
}




// ======================================================================================================================

//												FRIENDLIST

// ======================================================================================================================




//upload to the startup board. Then populate list with friends.
void CSteamLeaderboard::GetFriendBoard()
{
	//UserHasLicenseForApp
	SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard("startup");
	m_FriendFindLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnFriendFindLeaderboard);
}

void CSteamLeaderboard::OnFriendFindLeaderboard( LeaderboardFindResult_t *pCallback,	bool bIOFailure )
{
	if (!pCallback->m_bLeaderboardFound || bIOFailure)
	{
		common->Warning("STEAM: problem finding leaderboard (OnFriendFindLeaderboard).\n");
		isSending = false;
		return;
	}


	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(pCallback->m_hSteamLeaderboard, k_ELeaderboardDataRequestFriends, 0,0);
	m_friendDownloadLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnFriendDownloadLeaderboard);
}


void CSteamLeaderboard::OnFriendDownloadLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	int					totalEntries;
	int					i;
	idListGUI *			guiList_people;
	idUserInterface *	mainGui;
	LeaderboardEntry_t	m_leaderboardEntries[MAXFRIENDLISTCOUNT];

	if (bIOFailure)
	{
		common->Warning("STEAM: failed to download leaderboard. (OnFriendDownloadLeaderboard)");
		return;
	}

	totalEntries = pCallback->m_cEntryCount;

	if (totalEntries <= 0)
	{
		common->Warning("STEAM: empty leaderboard. (OnFriendDownloadLeaderboard)");
		return;
	}

	mainGui = uiManager->FindGui( "guis/mainmenu.gui", true, false, true );

	guiList_people = uiManager->AllocListGUI();
	guiList_people->Config( mainGui, "friendlist" );


	//brute-force jam the local player to the top of the list.
	guiList_people->Push(SteamFriends()->GetPersonaName());
	gameLocal.friendIDs.Append(SteamUser()->GetSteamID());


	for (i = 0; i < totalEntries; i++)
	{
		if (SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, i, &m_leaderboardEntries[i], NULL, 0))
		{
			//player is already at the top of the list. Therefore, skip local player.
			if (SteamUser()->GetSteamID() == m_leaderboardEntries[i].m_steamIDUser)
			{
				continue;
			}

			//name.
			idStr playername = SteamFriends()->GetFriendPersonaName( m_leaderboardEntries[i].m_steamIDUser);
			//common->Printf("STEAM: adding #%d %s\n", i, playername.c_str());
			guiList_people->Push(  playername.c_str()  );
			gameLocal.friendIDs.Append(m_leaderboardEntries[i].m_steamIDUser);
			continue;
		}
	}

	uiManager->FreeListGUI( guiList_people );
	//common->Printf("STEAM: retrieved friendlist. (%d entries)\n", totalEntries);


	//now attempt to upload.
	SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore( pCallback->m_hSteamLeaderboard, k_ELeaderboardUploadScoreMethodKeepBest, 1, NULL, 0 );
	m_friendUploadScore.Set(hSteamAPICall, this, &CSteamLeaderboard::OnFriendUploadScore);		
}

void CSteamLeaderboard::OnFriendUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure)
{
	if ( pCallback->m_bSuccess < 1 || bIOFailure )
	{
		common->Warning("STEAM: failed to upload leaderboard score. (OnFriendUploadScore)");
		uploadingScore = false;
		return;
	}

	common->Printf("STEAM: friendlist update complete.\n");	
}












// ======================================================================================================================

//												MAIN MENU GET LEADERBOARD HANDLES

// ======================================================================================================================


void CSteamLeaderboard::GetMainmenuLeaderboards(int index, const char *statname)
{
	common->Printf("searching for %d: %s\n", index, statname);
	SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(statname);
	m_mainmenuFindLeaderboard[index].Set(hSteamAPICall, this, &CSteamLeaderboard::OnFindMainmenuLeaderboards);	
}

void CSteamLeaderboard::OnFindMainmenuLeaderboards(LeaderboardFindResult_t *pCallback, bool bIOFailure)
{
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		common->Warning("STEAM: failed to find leaderboard (OnFindMainmenuLeaderboards).");
		return;
	}

	idStr leaderboardName = SteamUserStats()->GetLeaderboardName( pCallback->m_hSteamLeaderboard );

	gameLocal.steamLeaderboardHandles.AddUnique(pCallback->m_hSteamLeaderboard);
	gameLocal.steamLeaderboardNames.AddUnique(leaderboardName.c_str());

	gameLocal.OnClickLeaderboard();
}

// ======================================================================================================================

//												MAIN MENU RETRIEVE LEADERBOARD NAMES/TIMES

// ======================================================================================================================

void CSteamLeaderboard::UpdateMainmenuLeaderboards(SteamLeaderboard_t board)
{
	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(board, k_ELeaderboardDataRequestFriends, 0,0);
	m_downloadMainmenuLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboard::OnDownloadMainmenuLeaderboard);
}

void CSteamLeaderboard::OnDownloadMainmenuLeaderboard( LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	int					totalEntries;
	int					i;
	LeaderboardEntry_t	m_scoreEntries[MAXMAINMENULEADERBOARDCOUNT];
	//idUserInterface *	mainGui;

	if (bIOFailure)
	{
		common->Warning("STEAM: failed to download leaderboard. (OnDownloadMainmenuLeaderboard)");
		return;
	}

	gameLocal.mainmenuLeaderboardNames->Clear();
	gameLocal.mainmenuLeaderboardTimes->Clear();

	totalEntries = pCallback->m_cEntryCount;
	if (totalEntries <= 0)
	{
		common->Warning("STEAM: empty leaderboard. (OnDownloadMainmenuLeaderboard)");
		return;
	}

	//mainGui = uiManager->FindGui( "guis/mainmenu.gui", true, false, true );

	
	

	totalEntries = min(totalEntries, MAXMAINMENULEADERBOARDCOUNT);

	for (i = 0; i < totalEntries; i++)
	{
		if (SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, i, &m_scoreEntries[i], NULL, 0))
		{
			//m_scoreEntries[i].m_nScore
			gameLocal.mainmenuLeaderboardNames->Push( va("%s", SteamFriends()->GetFriendPersonaName( m_scoreEntries[i].m_steamIDUser) ));
			gameLocal.mainmenuLeaderboardTimes->Push( va("%s", gameLocal.ParseTimeMS(m_scoreEntries[i].m_nScore)  ) );
		}
		else
		{
			//common->Warning("STEAM: problem downloading leaderboard entry. (OnScoreDownloadLeaderboard)");
		}
	}

	//uiManager->FreeListGUI( guiList_people );
	//uiManager->FreeListGUI( guiList_times );
}



#endif
