// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNet_local.h"

#include "SDNetTask_local.h"

static idCVar harm_com_autoLogin("harm_com_autoLogin", "", CVAR_SYSTEM | CVAR_ARCHIVE, "login username automatic when game start");

//===============================================================
//
//	sdNetService
//
//===============================================================

sdNetService_Local::sdNetService_Local()
	: isInitialized(false),
	serviceState(SS_DISABLED),
	disconnectReason(DR_NONE),
	dedicatedState(DS_OFFLINE),
	lastError(SDNET_NO_ERROR),
	activeUser(NULL)
{
}

sdNetService_Local::~sdNetService_Local() {
}

bool sdNetService_Local::Init() {
	LoadOfflineUsers();
	serviceState = SS_INITIALIZED;
	dedicatedState = DS_ONLINE;
	isInitialized = true;
	if(harm_com_autoLogin.GetString() && harm_com_autoLogin.GetString()[0])
	{
		bool found = false;
		for(int i = 0; i < userList.Num(); i++) {
			sdNetUser *user = userList[i];
			if(!idStr::Cmp(user->GetRawUsername(), harm_com_autoLogin.GetString()))
			{
				user->Activate();
				found = true;
				common->Printf("Login user '%s' automatic\n", user->GetRawUsername());
				break;
			}
		}
		if(!found)
		{
			common->Warning("Login user '%s' not found", harm_com_autoLogin.GetString());
			harm_com_autoLogin.SetString("");
		}
	}
	return isInitialized;
}

void sdNetService_Local::Shutdown() {
}

void sdNetService_Local::RunFrame() {
	for (int i = 0; i < taskPools.Num(); ++i) {
		((sdNetTask_Local *)taskPools[i])->RunFrame();
	}
}

sdNetService::serviceState_e sdNetService_Local::GetState() const {
	return serviceState;
}

sdNetService::disconnectReason_e sdNetService_Local::GetDisconnectReason() const {
	return disconnectReason;
}

sdNetService::dedicatedState_e sdNetService_Local::GetDedicatedServerState() const {
	return dedicatedState;
}

const sdNetService::motdList_t& sdNetService_Local::GetMotD() const {
	return motdList;
}

	//
	// Key Code
	//
bool sdNetService_Local::CheckKey( const char* key, bool noChecksum ) const {
	return true;
}

const char* sdNetService_Local::GetStoredLicenseCode() const {
	return "";
}

bool sdNetService_Local::IsSteamActive() const {
	return false;
}

	//
	// User management
	//
sdNetErrorCode_e sdNetService_Local::CreateUser( sdNetUser** user, const char* username ) {
	*user = NULL;
	for(int i = 0; i < userList.Num(); i++)
	{
		if(!idStr::Cmp(userList[i]->GetUsername(), username))
		{
			*user = userList[i];
			return SDNET_USER_USERNAME_EXISTS;
		}
	}
	if(!*user)
	{
		sdNetUser_Local *newUser = new sdNetUser_Local;
		newUser->SetRawUsername(username);
		newUser->Create();
		userList.Append(newUser);
		*user = newUser;
	}
	return SDNET_NO_ERROR;
}

void sdNetService_Local::DeleteUser( sdNetUser* user ) {
	if(!user)
		return;
	// deactivate user first
	if(activeUser == user)
		user->Deactivate();

	const char *username = user->GetUsername();
	for(int i = 0; i < userList.Num(); i++)
	{
		if(!idStr::Cmp(userList[i]->GetUsername(), username))
		{
			userList[i]->Remove();
			delete userList[i];
			userList.RemoveIndex(i);
			break;
		}
	}
}

int sdNetService_Local::NumUsers() const {
	return userList.Num();
}

sdNetUser* sdNetService_Local::GetUser( const int index ) {
	if(index < 0 || index >= userList.Num())
		return NULL;
	return userList[index];
}

sdNetUser* sdNetService_Local::GetActiveUser() {
	return activeUser;
}

	//
	// Session management - deferred to Session Manager
	//
sdNetSessionManager& sdNetService_Local::GetSessionManager() {
	return sessionManager;
}

#if !defined( SD_DEMO_BUILD )
	//
	// Stats management - deferred to Stats Manager
	//
sdNetStatsManager& sdNetService_Local::GetStatsManager() {
	return statsManager;
}

	//
	// Friends management - deferred to Friends Manager
	//
sdNetFriendsManager& sdNetService_Local::GetFriendsManager() {
	return friendsManager;
}

	//
	// Friends management - deferred to Team Manager
	//
sdNetTeamManager& sdNetService_Local::GetTeamManager() {
	return teamManager;
}
#endif /* !SD_DEMO_BUILD */

	//
	// Task management
	//
void sdNetService_Local::FreeTask( sdNetTask* task ) {
	taskPools.Remove(task);
	delete task;
}

	//
	// Online Services
	//

sdNetErrorCode_e sdNetService_Local::GetLastError() const {
	return lastError;
}

	// Start online service and connect to auth system
sdNetTask* sdNetService_Local::Connect() {
	serviceState = SS_ONLINE;
#if 0
	sdNetUser *user;
	CreateUser(&user, "test");
	activeUser = user;
#endif
	return new sdNetTask_Connect;
}

	// Authorize a dedicated server
sdNetTask* sdNetService_Local::SignInDedicated() {
	return new sdNetTask_SignInDedicated;
}

	// De-authorize a dedicated server
sdNetTask* sdNetService_Local::SignOutDedicated() {
	return new sdNetTask_SignOutDedicated;
}

#if !defined( SD_DEMO_BUILD )
	// Get a list of account names for a license code
sdNetTask* sdNetService_Local::GetAccountsForLicense( idStrList& accountNames, const char* licenseCode ) {
	return new sdNetTask_GetAccountsForLicense;
}

	// Get a user's profile
const idDict* sdNetService_Local::GetProfileProperties( sdNetClientId userID ) const {
	for(int i = 0; i < userList.Num(); i++) {
		sdNetClientId id;
		sdNetUser *user = userList[i];
		user->GetAccount().GetNetClientId(id);
		if (id == userID) {
			return &user->GetProfile().GetProperties();
		}
	}
	return NULL;
}
#endif /* !SD_DEMO_BUILD */

idList<sdNetTask *> sdNetService_Local::taskPools;

void sdNetService_Local::AddTask(sdNetTask *task) {
	taskPools.Append( task );
}

void sdNetService_Local::LoadOfflineUsers(void)
{
	common->Printf("Load local offline users...");
	activeUser = NULL;
	userList.DeleteContents(true);

	idStr path = fileSystem->GetUserPath();
	path.AppendPath("sdnet");
	idStrList list;
	int size = Sys_ListFiles(path, "/", list);
	common->Printf("%d\n", size);
	for(int i = 0; i < size; i++)
	{
		if (!list[i].Cmp(".") || !list[i].Cmp("..")) { // only for Windows
			continue;
		}
		sdNetUser_Local *user = new sdNetUser_Local;
		common->Printf("Load offline user: %s\n", list[i].c_str());
		user->SetRawUsername(list[i].c_str());
		user->Init();
		userList.Append(user);
	}
}

void sdNetService_Local::SetActiveUser(sdNetUser_Local *user) {
	activeUser = user;
}

void sdNetService_Local::SaveUserModified(void)
{
	if(activeUser)
		activeUser->SaveModified();
}

sdNetService_Local networkServiceLocal;
sdNetService* networkService = &networkServiceLocal;
