// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SDNETTASK_LOCAL_H__
#define __SDNETTASK_LOCAL_H__

#include "SDNetTask.h"
#include "SDNetTeamManager.h"
#include "SDNetStatsManager.h"
#include "SDNetSessionManager.h"
#include "SDNetFriend.h"

//===============================================================
//
//	sdNetTask
//
//===============================================================

class sdNetTask_Local : public sdNetTask {
public:
	sdNetTask_Local(const char *name = "");
	virtual						~sdNetTask_Local();

	virtual void				Cancel( bool blocking = false );

	virtual taskStatus_e		GetState() const;
	virtual sdNetErrorCode_e	GetErrorCode() const;

	virtual void				AcquireLock();
	virtual void				ReleaseLock();

	virtual void				RunFrame();

protected:
	virtual void				OnStateChanged(taskStatus_e st);

private:
	taskStatus_e taskStatus;
	sdNetErrorCode_e errorCode;

	idStr name;
	int startTime;
	int updateTime;
};

class sdNetTask_Connect : public sdNetTask_Local {
public:
	sdNetTask_Connect() : sdNetTask_Local("sdNetTask_Connect") {}
};
class sdNetTask_SignInDedicated : public sdNetTask_Local {
public:
	sdNetTask_SignInDedicated() : sdNetTask_Local("sdNetTask_SignInDedicated") {}
};
class sdNetTask_SignOutDedicated : public sdNetTask_Local {
public:
	sdNetTask_SignOutDedicated() : sdNetTask_Local("sdNetTask_SignOutDedicated") {}
};
class sdNetTask_GetAccountsForLicense : public sdNetTask_Local {
public:
	sdNetTask_GetAccountsForLicense() : sdNetTask_Local("sdNetTask_GetAccountsForLicense") {}
};

class sdNetTask_CreateAccount : public sdNetTask_Local {
public:
	sdNetTask_CreateAccount(const char* username, const char* password, const char* key) : sdNetTask_Local(va("sdNetTask_CreateAccount::%s %s %s", username, password, key)), username(username), password(password), key(key) {}
private:
	idStr username;
	idStr password;
	idStr key;
};
class sdNetTask_ChangePassword : public sdNetTask_Local {
public:
	sdNetTask_ChangePassword(const char* password, const char* newPassword) : sdNetTask_Local(va("sdNetTask_ChangePassword::%s %s", password, newPassword)), password(password), newPassword(newPassword) {}
private:
	idStr password;
	idStr newPassword;
};
class sdNetTask_ResetPassword : public sdNetTask_Local {
public:
	sdNetTask_ResetPassword(const char* key, const char* newPassword) : sdNetTask_Local(va("sdNetTask_ResetPassword::%s %s", key, newPassword)), key(key), newPassword(newPassword) {}
private:
	idStr key;
	idStr newPassword;
};
class sdNetTask_DeleteAccount : public sdNetTask_Local {
public:
	sdNetTask_DeleteAccount() : sdNetTask_Local("sdNetTask_DeleteAccount") {}
};
class sdNetTask_SignIn : public sdNetTask_Local {
public:
	sdNetTask_SignIn() : sdNetTask_Local("sdNetTask_SignIn") {}
};
class sdNetTask_SignOut : public sdNetTask_Local {
public:
	sdNetTask_SignOut() : sdNetTask_Local("sdNetTask_SignOut") {}
};


class sdNetTask_TeamManager_Init : public sdNetTask_Local {
public:
	sdNetTask_TeamManager_Init() : sdNetTask_Local("sdNetTask_TeamManager_Init") {}
};
class sdNetTask_CreateTeam : public sdNetTask_Local {
public:
	sdNetTask_CreateTeam(const char* teamname) : sdNetTask_Local(va("sdNetTask_CreateTeam::%s", teamname)), teamname(teamname) {}
private:
	idStr teamname;
};
class sdNetTask_ProposeMembership : public sdNetTask_Local {
	public:
	sdNetTask_ProposeMembership(const char* username, const wchar_t* reason) : sdNetTask_Local(va("sdNetTask_ProposeMembership::%s %ls", username, reason)), username(username), reason(reason) {}
	private:
	idStr username;
	idWStr reason;
};
class sdNetTask_WithdrawMembership : public sdNetTask_Local {
public:
	sdNetTask_WithdrawMembership(const char* username) : sdNetTask_Local(va("sdNetTask_WithdrawMembership::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_AcceptMembership : public sdNetTask_Local {
public:
	sdNetTask_AcceptMembership(const char* username, const sdNetTeamId& teamId) : sdNetTask_Local(va("sdNetTask_AcceptMembership::%s", username)), username(username), teamId(teamId) {}
private:
	idStr username;
	sdNetTeamId teamId;
};
class sdNetTask_RejectMembership : public sdNetTask_Local {
public:
	sdNetTask_RejectMembership(const char* username, const sdNetTeamId& teamId) : sdNetTask_Local(va("sdNetTask_RejectMembership::%s", username)), username(username), teamId(teamId) {}
private:
	idStr username;
	sdNetTeamId teamId;
};
class sdNetTask_RemoveMember : public sdNetTask_Local {
public:
	sdNetTask_RemoveMember(const char* username) : sdNetTask_Local(va("sdNetTask_RemoveMember::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_TeamSendMessage : public sdNetTask_Local {
public:
	sdNetTask_TeamSendMessage(const char* username, const wchar_t* text) : sdNetTask_Local(va("sdNetTask_TeamSendMessage::%s %ls", username, text)), username(username), text(text) {}
private:
	idStr username;
	idWStr text;
};
class sdNetTask_BroadcastMessage : public sdNetTask_Local {
public:
	sdNetTask_BroadcastMessage(const wchar_t* text) : sdNetTask_Local(va("sdNetTask_BroadcastMessage::%ls", text)), text(text) {}
private:
	idWStr text;
};
class sdNetTask_TeamInvite : public sdNetTask_Local {
public:
	sdNetTask_TeamInvite(const char* username, const netadr_t& sessionAddress) : sdNetTask_Local(va("sdNetTask_TeamInvite::%s", username)), username(username), sessionAddress(sessionAddress) {}
private:
	idStr username;
	netadr_t sessionAddress;
};
class sdNetTask_PromoteMember : public sdNetTask_Local {
public:
	sdNetTask_PromoteMember(const char* username) : sdNetTask_Local(va("sdNetTask_PromoteMember::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_DemoteMember : public sdNetTask_Local {
public:
	sdNetTask_DemoteMember(const char* username) : sdNetTask_Local(va("sdNetTask_DemoteMember::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_TransferOwnership : public sdNetTask_Local {
public:
	sdNetTask_TransferOwnership(const char* username) : sdNetTask_Local(va("sdNetTask_TransferOwnership::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_DisbandTeam : public sdNetTask_Local {
public:
	sdNetTask_DisbandTeam() : sdNetTask_Local("sdNetTask_DisbandTeam") {}
};
class sdNetTask_LeaveTeam : public sdNetTask_Local {
public:
	sdNetTask_LeaveTeam(const char* username) : sdNetTask_Local(va("sdNetTask_LeaveTeam::%s", username)), username(username) {}
private:
	idStr username;
};


class sdNetTask_AssureExists : public sdNetTask_Local {
public:
	sdNetTask_AssureExists() : sdNetTask_Local("sdNetTask_AssureExists") {}
};
class sdNetTask_Store : public sdNetTask_Local {
public:
	sdNetTask_Store(bool publicProfile, bool privateProfile) : sdNetTask_Local(va("sdNetTask_Store::%d %d", publicProfile, privateProfile)), publicProfile(publicProfile), privateProfile(privateProfile) {}
private:
	bool publicProfile;
	bool privateProfile;
};
class sdNetTask_Restore : public sdNetTask_Local {
public:
	sdNetTask_Restore() : sdNetTask_Local("sdNetTask_Restore") {}
};



class sdNetTask_Flush : public sdNetTask_Local {
public:
	sdNetTask_Flush() : sdNetTask_Local("sdNetTask_Flush") {}
};
class sdNetTask_ReadDictionary : public sdNetTask_Local {
public:
	sdNetTask_ReadDictionary(const sdNetClientId& clientId, sdNetStatKeyValList& stats) : sdNetTask_Local("sdNetTask_ReadDictionary"), clientId(clientId), stats(stats) {}
private:
	sdNetClientId clientId;
	sdNetStatKeyValList &stats;
};



class sdNetTask_CreateSession : public sdNetTask_Local {
public:
	sdNetTask_CreateSession(sdNetSession& session) : sdNetTask_Local("sdNetTask_CreateSession"), session(session) {}
private:
	sdNetSession &session;
};
class sdNetTask_UpdateSession : public sdNetTask_Local {
public:
	sdNetTask_UpdateSession(sdNetSession& session) : sdNetTask_Local("sdNetTask_UpdateSession"), session(session) {}
private:
	sdNetSession &session;
};
class sdNetTask_DeleteSession : public sdNetTask_Local {
public:
	sdNetTask_DeleteSession(sdNetSession& session) : sdNetTask_Local("sdNetTask_DeleteSession"), session(session) {}
private:
	sdNetSession &session;
};
class sdNetTask_FindSessions : public sdNetTask_Local {
public:
	sdNetTask_FindSessions(idList< sdNetSession* >& sessions, sdNetSessionManager::sessionSource_e source) : sdNetTask_Local(va("sdNetTask_FindSessions::%d", source)), sessions(sessions), source(source) {}
private:
	idList< sdNetSession* >& sessions;
	sdNetSessionManager::sessionSource_e source;
};
class sdNetTask_RefreshSessions : public sdNetTask_Local {
public:
	sdNetTask_RefreshSessions(idList< sdNetSession* >& sessions) : sdNetTask_Local("sdNetTask_RefreshSessions"), sessions(sessions) {}
private:
	idList< sdNetSession* >& sessions;
};
class sdNetTask_RefreshSession : public sdNetTask_Local {
public:
	sdNetTask_RefreshSession(sdNetSession& session) : sdNetTask_Local("sdNetTask_RefreshSession"), session(session) {}
private:
	sdNetSession &session;
};


class sdNetTask_FriendsManager_Init : public sdNetTask_Local {
public:
	sdNetTask_FriendsManager_Init() : sdNetTask_Local("sdNetTask_FriendsManager_Init") {}
};
class sdNetTask_ProposeFriendship : public sdNetTask_Local {
public:
	sdNetTask_ProposeFriendship(const char* username, const wchar_t* reason) : sdNetTask_Local(va("sdNetTask_ProposeFriendship::%s %ls", username, reason)), username(username), reason(reason) {}
private:
	idStr username;
	idWStr reason;
};
class sdNetTask_WithdrawProposal : public sdNetTask_Local {
public:
	sdNetTask_WithdrawProposal(const char* username) : sdNetTask_Local(va("sdNetTask_WithdrawProposal::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_AcceptProposal : public sdNetTask_Local {
public:
	sdNetTask_AcceptProposal(const char* username) : sdNetTask_Local(va("sdNetTask_AcceptProposal::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_RejectProposal : public sdNetTask_Local {
public:
	sdNetTask_RejectProposal(const char* username) : sdNetTask_Local(va("sdNetTask_RejectProposal::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_RemoveFriend : public sdNetTask_Local {
public:
	sdNetTask_RemoveFriend(const char* username) : sdNetTask_Local(va("sdNetTask_RemoveFriend::%s", username)), username(username) {}
private:
	idStr username;
};
class sdNetTask_SetBlockedStatus : public sdNetTask_Local {
public:
	sdNetTask_SetBlockedStatus(const char* username, const sdNetFriend::blockState_e blockState) : sdNetTask_Local(va("sdNetTask_SetBlockedStatus::%s %d", username, blockState)), username(username), blockState(blockState) {}
private:
	idStr username;
	sdNetFriend::blockState_e blockState;
};
class sdNetTask_FriendSendMessage : public sdNetTask_Local {
public:
	sdNetTask_FriendSendMessage(const char* username, const wchar_t* text) : sdNetTask_Local(va("sdNetTask_FriendSendMessage::%s %ls", username, text)), username(username), text(text) {}
private:
	idStr username;
	idWStr text;
};
class sdNetTask_FriendInvite : public sdNetTask_Local {
public:
	sdNetTask_FriendInvite(const char* username, const netadr_t& sessionAddress) : sdNetTask_Local(va("sdNetTask_FriendInvite::%s", username)), username(username), sessionAddress(sessionAddress) {}
private:
	idStr username;
	netadr_t sessionAddress;
};

#endif /* !__SDNETTASK_LOCAL_H__ */
