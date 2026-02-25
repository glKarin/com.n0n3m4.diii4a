#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef STEAM
#include "steamremote.h"

#include "Game_local.h"

#define TEXTSIZE		131072
#define STICKYNOTEFILE	"stickynotes.txt"
#define STICKYNOTETAG	"stickynotes"





CSteamRemote* g_SteamRemote = NULL;

CSteamRemote::CSteamRemote()
{
	m_iAppID		= SteamUtils()->GetAppID();	
	bufferText		= "";
	isQuerying		= false;
	queryHandle		= NULL;
	downloadIdx		= 0;
	isSending		= false;

	//WriteFile();

	//CSteamID id = SteamUser()->GetSteamID();
	//GetUserFiles(id);
}

CSteamRemote::~CSteamRemote()
{
}


// ========================================== START SENDSTICKY CODE ==========================================

void CSteamRemote::SendStickynotes()
{
	if (isSending)
	{
		return;
	}

	isSending = true;

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

		//common->Printf("sticky output %s", output.c_str());
	}

	//common->Printf("zzz STEAM FILE CONTENT: \n%s\n", output.c_str());

	this->bufferText = output;

	//verify whether we should update an existing file.
	GetPublishedFiles();
}

// ========================================== END SENDSTICKY CODE ==========================================

// ========================================== START UPDATEFILE CODE ==========================================

void CSteamRemote::GetPublishedFiles()
{
	const char *original = STICKYNOTETAG;
	const char **pointer_to_original = &original;
	CSteamID id = SteamUser()->GetSteamID();

	SteamParamStringArray_t tags;
	tags.m_ppStrings = pointer_to_original;
	tags.m_nNumStrings = 1;

	//common->Printf("zzzz STEAM getting published files...\n");

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->EnumerateUserSharedWorkshopFiles(id, 0, &tags, NULL);
	m_SteamCallResultGetPublishedFiles.Set( hSteamAPICall, this, &CSteamRemote::OnGetPublishedFiles );
}

//Receive files that the local user has published.
void CSteamRemote::OnGetPublishedFiles(RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure )
{
	int i;

	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem getting user file (OnGetPublishedFiles()).\n");
		WriteFile(this->bufferText);
		return;
	}

	if (pResult->m_nResultsReturned <= 0)
	{
		//No results. Abort.
		WriteFile(this->bufferText);
		return;
	}

	if ((int)strlen( this->bufferText ) >= TEXTSIZE)
	{
		common->Warning("STEAM: file size too large (OnGetPublishedFiles()).\n");
		return;
	}

	bool writebool = SteamRemoteStorage()->FileWrite( STICKYNOTEFILE, this->bufferText, (int) strlen( this->bufferText ) );

	if (!writebool)
	{
		common->Warning("STEAM: problem writing file (OnGetPublishedFiles()).\n");
		WriteFile(this->bufferText);
		return;
	}

	PublishedFileUpdateHandle_t fileHandle = SteamRemoteStorage()->CreatePublishedFileUpdateRequest(pResult->m_rgPublishedFileId[0]);
	bool ret = SteamRemoteStorage()->UpdatePublishedFileFile(fileHandle, STICKYNOTEFILE);
	
	if (!ret)
	{
		common->Warning("STEAM: problem updating file (OnGetPublishedFiles()).\n");
		WriteFile(this->bufferText);
		return;
	}

	//common->Printf("zzzz STEAM updating published file....\n");

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->CommitPublishedFileUpdate(fileHandle);
	m_SteamCallResultCommitFileUpdate.Set( hSteamAPICall, this, &CSteamRemote::OnCommitFileUpdate );
}

void CSteamRemote::OnCommitFileUpdate(RemoteStorageUpdatePublishedFileResult_t  *pResult, bool bIOFailure )
{
	isSending = false;

	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem committing updated file (OnCommitFileUpdate()).\n");
		WriteFile(this->bufferText);
		return;
	}
	
	if (pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement)
	{
		gameLocal.ShowWorkshopLegalGui();
	}

	//common->Printf("zzzz STEAM done updating published file.\n");

	//common->Printf("zz STEAM UPDATING PUBLISHED FILE.\n");
}

// ========================================== END UPDATEFILE CODE ==========================================


// ========================================== START WRITE CODE ==========================================

//Write file.
void CSteamRemote::WriteFile(const char *text)
{
	char testmessage1[TEXTSIZE];

	strncpy(testmessage1, text, sizeof(testmessage1) - 1);
	testmessage1[sizeof(testmessage1) - 1] = 0;

	if ((int)strlen( testmessage1 ) >= TEXTSIZE)
	{
		common->Warning("STEAM: file size too large (OnGetPublishedFiles()).\n");
		return;
	}

	bool bRet = SteamRemoteStorage()->FileWrite( STICKYNOTEFILE, testmessage1, (int) strlen( testmessage1 ) );

	if (!bRet)
	{
		common->Warning("STEAM: problem writing file (WriteFile()).\n");
		isSending = false;
		return;
	}

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->FileShare(STICKYNOTEFILE);
	m_SteamCallResultWriteFile.Set( hSteamAPICall, this, &CSteamRemote::OnWriteFile );

	//common->Printf("zz STEAM WRITING NEW FILE.");
}

void CSteamRemote::OnWriteFile( RemoteStorageFileShareResult_t *pResult, bool bIOFailure )
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem writing file (OnWriteFile()).\n");
		isSending = false;
		return;
	}

	const char *original = STICKYNOTETAG;
	const char **pointer_to_original = &original;

	SteamParamStringArray_t tags;
	tags.m_ppStrings = pointer_to_original;
	tags.m_nNumStrings = 1;

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->PublishWorkshopFile(STICKYNOTEFILE, NULL, m_iAppID, NULL, NULL,
		k_ERemoteStoragePublishedFileVisibilityFriendsOnly, &tags, k_EWorkshopFileTypeCommunity);
		m_SteamCallResultPublishFile.Set( hSteamAPICall, this, &CSteamRemote::OnPublishFile );
}

void CSteamRemote::OnPublishFile( RemoteStoragePublishFileResult_t *pResult, bool bIOFailure )
{
	isSending = false;

	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem publishing file (OnPublishFile()).\n");
		return;
	}

	if (pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement)
	{
		gameLocal.ShowWorkshopLegalGui();
	}

	common->Printf("STEAM: completed uploading sticky notes.\n");
}

// ========================================== END WRITE CODE ==========================================







// ========================================== START DELETION CODE ==========================================

void CSteamRemote::DeleteAll( void )
{
	CSteamID id = SteamUser()->GetSteamID();
	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->EnumerateUserSharedWorkshopFiles(id, 0, NULL, NULL);
	m_SteamCallResultGetFilesToDelete.Set( hSteamAPICall, this, &CSteamRemote::OnGetFilesToDelete );
}

void CSteamRemote::OnGetFilesToDelete( RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure )
{
	int i;

	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem getting files to delete (OnGetFilesToDelete()).\n");
		return;
	}

	if (pResult->m_nResultsReturned <= 0)
	{
		//No results. Abort.
		common->Warning("STEAM: there are no files to delete (OnGetFilesToDelete()).\n");
		return;
	}

	for (i = 0; i < pResult->m_nResultsReturned; i++)
	{
		SteamRemoteStorage()->DeletePublishedFile(pResult->m_rgPublishedFileId[i]);
	}

	common->Printf("STEAM: deleted %d files.\n", pResult->m_nResultsReturned);

	//delete all existing sticky notes in the world.

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
		{
			continue;
		}

		if (!gameLocal.entities[i]->IsType(idStaticEntity::Type) || gameLocal.entities[i]->spawnArgs.GetInt("stickynote", "0") <= 0)
		{
			continue;
		}

		gameLocal.entities[i]->PostEventMS( &EV_Remove, 0 );
	}
}

// ========================================== END DELETION CODE ==========================================





void CSteamRemote::GetAllUserFiles( void )
{
	const char *original = STICKYNOTETAG;
	const char **pointer_to_original = &original;
	CSteamID selfID = SteamUser()->GetSteamID();

	SteamParamStringArray_t tags;
	tags.m_ppStrings = pointer_to_original;
	tags.m_nNumStrings = 1;	

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->EnumerateUserSharedWorkshopFiles(selfID, 0, &tags, NULL);
	m_SteamCallResultGetSelfFiles.Set( hSteamAPICall, this, &CSteamRemote::OnGetSelfFiles );
}

void CSteamRemote::OnGetSelfFiles( RemoteStorageEnumerateUserSharedWorkshopFilesResult_t *pResult, bool bIOFailure )
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem getting self files (OnGetSelfFiles()).\n");
		GetFriendFiles();
		return;
	}

	if (pResult->m_nResultsReturned <= 0)
	{
		GetFriendFiles();
		return;
	}

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->GetPublishedFileDetails(pResult->m_rgPublishedFileId[0], 0);
	m_SteamCallResultGetFileDetails.Set( hSteamAPICall, this, &CSteamRemote::OnGetFileDetails );
}

void CSteamRemote::OnGetFileDetails( RemoteStorageGetPublishedFileDetailsResult_t *pResult, bool bIOFailure )
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem getting published file details (OnGetFileDetails()).\n");
		GetFriendFiles();
		return;
	}

	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->UGCDownload(pResult->m_hFile, 0);
	m_SteamCallResultDownloadSelfFile.Set( hSteamAPICall, this, &CSteamRemote::OnDownloadSelfFile );
}

void CSteamRemote::OnDownloadSelfFile(RemoteStorageDownloadUGCResult_t *pResult, bool bIOFailure)
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem downloading file (OnDownloadSelfFile()).\n");
		GetFriendFiles();
		return;
	}

	if (pResult->m_nSizeInBytes + 1 > TEXTSIZE)
	{
		common->Warning("STEAM: file size too large (OnDownloadSelfFile()).\n");
		GetFriendFiles();
		return;
	}

	char testmessage[TEXTSIZE];
	int32 cubRead = SteamRemoteStorage()->UGCRead(pResult->m_hFile, testmessage, sizeof(testmessage), 0, k_EUGCRead_ContinueReadingUntilFinished);
	testmessage[cubRead] = 0; // null-terminate

	//common->Printf("zzzz SELF contents \"%s\"\n",  testmessage);
	gameLocal.ParseStickynotes(testmessage, true); //parse local file.

	GetFriendFiles();
}




void CSteamRemote::GetFriendFiles( void )
{
	if (isQuerying)
	{
		//common->Printf("ZZZZ STEAM already querying. Restarting query...\n");
		SteamUGC()->ReleaseQueryUGCRequest(queryHandle);
		isQuerying = false;

		GetFriendFiles();
		return;
	}

	//common->Printf("ZZZZ Steam getting friend files...\n");

	isQuerying		= true;	
	downloadIdx		= 0;
	downloadMax		= 0;

	queryHandle = SteamUGC()->CreateQueryAllUGCRequest(
		k_EUGCQuery_CreatedByFriendsRankedByPublicationDate,
		k_EUGCMatchingUGCType_Items,
		m_iAppID, m_iAppID, 1);

	//SteamUGC()->AddRequiredTag(queryHandle, STICKYNOTETAG);
	
	SteamAPICall_t hSteamAPICall = SteamUGC()->SendQueryUGCRequest(queryHandle);
	m_SteamCallResultUGCQueryCompleted.Set(hSteamAPICall, this, &CSteamRemote::OnUGCQueryCompleted );
}

void CSteamRemote::OnUGCQueryCompleted( SteamUGCQueryCompleted_t *pResult, bool bIOFailure )
{
	int i;

	//common->Printf("ZZZZ Steam friend query complete...\n");

	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem completing UGC query. Result = %d (OnUGCQueryCompleted()).\n", (int)pResult->m_eResult);
		SteamUGC()->ReleaseQueryUGCRequest(queryHandle);
		isQuerying = false;
		return;
	}

	//common->Warning("zzzzz STEAM: found %d friend files.\n", pResult->m_unNumResultsReturned );

	if (pResult->m_unNumResultsReturned <= 0)
	{		
		SteamUGC()->ReleaseQueryUGCRequest(queryHandle);
		isQuerying = false;
		return;
	}

	downloadMax = pResult->m_unNumResultsReturned;

	SteamUGCDetails_t details;
	SteamUGC()->GetQueryUGCResult(queryHandle, 0, &details);

	//common->Printf("zzzz steam attempt download. Owner name: %s  Filename: %s  id: %d\n",		SteamFriends()->GetFriendPersonaName(details.m_ulSteamIDOwner),		details.m_pchFileName,		details.m_hFile);	

	DownloadAndParseFile(details.m_hFile);	
}

void CSteamRemote::DownloadAndParseFile(UGCHandle_t handle)
{
	SteamAPICall_t hSteamAPICall = SteamRemoteStorage()->UGCDownload(handle, 0);
	m_SteamCallResultDownloadFile.Set( hSteamAPICall, this, &CSteamRemote::OnDownloadFile );
}

void CSteamRemote::OnDownloadFile(RemoteStorageDownloadUGCResult_t *pResult, bool bIOFailure)
{
	if (pResult->m_eResult != k_EResultOK)
	{
		common->Warning("STEAM: problem downloading file. Result = %d (OnDownloadFile()).\n", (int)pResult->m_eResult);
		DownloadNextFile();
		return;
	}

	if (pResult->m_nSizeInBytes + 1 > TEXTSIZE)
	{
		common->Warning("STEAM: file size too large (OnDownloadFile()).\n");
		DownloadNextFile();
		return;
	}

	char testmessage[TEXTSIZE];
	int32 cubRead = SteamRemoteStorage()->UGCRead(pResult->m_hFile, testmessage, sizeof(testmessage), 0, k_EUGCRead_ContinueReadingUntilFinished);
	testmessage[cubRead] = 0; // null-terminate

	//common->Printf("zzzz steam contents \"%s\"\n",  testmessage);
	gameLocal.ParseStickynotes(testmessage, false); //parse friend note.

	//const char *name = SteamFriends()->GetFriendPersonaName(	pResult->m_ulSteamIDOwner);
	//common->Printf("getfriend %s  \n\n%s\n",name, testmessage);

	DownloadNextFile();
}

void CSteamRemote::DownloadNextFile( void )
{
	downloadIdx++;

	if (downloadIdx >= downloadMax)
	{
		//common->Printf("zzzz STEAM: done downloading file handles. Count = %d\n", downloadMax);
		SteamUGC()->ReleaseQueryUGCRequest(queryHandle);
		isQuerying = false;

		common->Printf("STEAM: completed downloading sticky notes (%d total).\n", downloadMax);

		return;
	}

	SteamUGCDetails_t details; 
	SteamUGC()->GetQueryUGCResult(queryHandle, downloadIdx, &details);
	DownloadAndParseFile(details.m_hFile);
}



// ========================================== START WORKSHOP DOWNLOAD CODE ==========================================

//obsolete
void CSteamRemote::DownloadMods( int& amount, idStrList& workshopModNames )
{
}



// ========================================== CLOUD LOADING ==========================================

void CSteamRemote::CloudLoad()
{
	if (!steam_cloudsave.GetBool())
		return;

	ISteamRemoteStorage* remote_storage = SteamRemoteStorage();

	if (!remote_storage)
	{
		common->Printf("STEAM: cloud load failed to initialize.\n");
		return;
	}

	int cloudFileCount = remote_storage->GetFileCount();

	common->Printf("STEAM: found %d cloud files.\n", cloudFileCount);

	if (cloudFileCount <= 0)
	{	
		return;
	}

	idStr basegame = cvarSystem->GetCVarString( "fs_game" );

	if (basegame.IsEmpty())
	{
		basegame = "base/";
	}
	else
	{
		basegame += "/";
	}

	for (int i = 0; i < cloudFileCount; i++)
	{
		int32 size;
		idStr fileName = remote_storage->GetFileNameAndSize(i, &size);

		if ( idStr::FindText( fileName, basegame,  false ) != 0 )
		{
			//savegame's mod folder does NOT match. skip it.
			continue;
		}

		bool shouldUpdate = false;

		//strip out the mod folder name.
		idStr strippedFileName = fileName;
		int amountToStrip = fileName.Length() - basegame.Length();
		strippedFileName = strippedFileName.Right(amountToStrip);

		if (strippedFileName.Length() <= 0)
			continue;

		if (fileSystem->ReadFile( strippedFileName, NULL ) <= 0)
		{
			//file does NOT exist locally. Flag it for cloud update.
			shouldUpdate = true;
		}
		else
		{
			//file exists locally. compare last modified date.
			ID_TIME_T localTimestamp;
			fileSystem->ReadFile( strippedFileName, NULL, &localTimestamp );

			int cloudDate = remote_storage->GetFileTimestamp(fileName.c_str());
			
			if (localTimestamp >= cloudDate)
				continue; //local file is newer/same as cloud file. skip it.

			shouldUpdate = true;
		}

		if (!shouldUpdate)
			continue;

		//retrieve the cloud file and write the file to the local drive.

		int32 cloudfileSize = remote_storage->GetFileSize(fileName);

		if (cloudfileSize <= 0)
		{
			common->Printf("STEAM: cloud file size is <= 0 (%s)\n", fileName.c_str());
			continue;
		}

		void	*buffer;
		buffer = (byte *)Mem_Alloc( cloudfileSize );
		uint bytesRead = remote_storage->FileRead(fileName.c_str(), buffer, cloudfileSize);

		if (bytesRead <= 0)
		{
			common->Printf("STEAM: cloud file bytes read is <= 0 (%s)\n", fileName.c_str());
			Mem_Free(buffer);
			continue;
		}

		fileSystem->WriteFile(strippedFileName.c_str(), buffer, cloudfileSize);
		//fileSystem->FreeFile( buffer );
		common->Printf("STEAM: writing local file %s\n", strippedFileName.c_str());

		Mem_Free(buffer);
	}
}


#endif
