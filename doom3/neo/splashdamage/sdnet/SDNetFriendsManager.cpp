// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( SD_DEMO_BUILD )

#include "idlib/precompiled.h"
#include "SDNetTask_local.h"
#include "SDNetFriend_local.h"

#include "SDNetFriendsManager_local.h"

//===============================================================
//
//	sdNetFriendsManager
//
//===============================================================

sdNetFriendsManager_Local::sdNetFriendsManager_Local() {
}

sdNetFriendsManager_Local::~sdNetFriendsManager_Local() {
}


const sdNetFriendsList&	sdNetFriendsManager_Local::GetFriendsList() const {
	return friendsList;
}


	// friends who have invited you to be their friend
const sdNetFriendsList&	sdNetFriendsManager_Local::GetPendingFriendsList() const {
	return pendingFriendsList;
}


	// friends that you have invited to be your friend
const sdNetFriendsList&	sdNetFriendsManager_Local::GetInvitedFriendsList() const {
	return invitedFriendsList;
}


const sdNetFriendsList&	sdNetFriendsManager_Local::GetBlockedList() const {
	return blockedFriendsList;
}


sdNetFriend* sdNetFriendsManager_Local::FindFriend( const sdNetFriendsList& list, const char* username ) {
	for (sdNetFriendsList::ConstIterator itor = list.Begin(); itor != list.End(); ++itor) {
		if (!idStr::Cmp((*itor)->GetUsername(), username))
			return *itor;
	}
	return NULL;
}

int	sdNetFriendsManager_Local::FindFriendIndex( const sdNetFriendsList& list, const char* username ) {
	for (int i = 0; i < list.Size(); ++i) {
		const sdNetFriend *item = list[i];
		if (!idStr::Cmp(item->GetUsername(), username))
			return i;
	}
	return -1;
}


sdLock&	sdNetFriendsManager_Local::GetLock() {
	return lock;
}


	//
	// Online functionality
	//

	// Initialize friends list
sdNetTask* sdNetFriendsManager_Local::Init() {
	return new sdNetTask_FriendsManager_Init;
}


	// Invite a user to become a friend
sdNetTask* sdNetFriendsManager_Local::ProposeFriendship( const char* username, const wchar_t* reason ) {
	return new sdNetTask_ProposeFriendship(username, reason);
}


	// Withdraw a friendship proposal
sdNetTask* sdNetFriendsManager_Local::WithdrawProposal( const char* username ) {
	return new sdNetTask_WithdrawProposal(username);
}


	// Accept a friendship proposal
sdNetTask* sdNetFriendsManager_Local::AcceptProposal( const char* username ) {
	return new sdNetTask_AcceptProposal(username);
}


	// Reject a friendship proposal
sdNetTask* sdNetFriendsManager_Local::RejectProposal( const char* username ) {
	return new sdNetTask_RejectProposal(username);
}


	// Remove a user from the friends list
sdNetTask* sdNetFriendsManager_Local::RemoveFriend( const char* username ) {
	return new sdNetTask_RemoveFriend(username);
}


	// Set the blocked status of a user
sdNetTask* sdNetFriendsManager_Local::SetBlockedStatus( const char* username, const sdNetFriend::blockState_e blockState ) {
	return new sdNetTask_SetBlockedStatus(username, blockState);
}


	// Send a messsage to a friend
sdNetTask* sdNetFriendsManager_Local::SendMessage( const char* username, const wchar_t* text ) {
	return new sdNetTask_FriendSendMessage(username, text);
}


	// Invite a friend to a session
sdNetTask* sdNetFriendsManager_Local::Invite( const char* username, const netadr_t& sessionAddress ) {
	return new sdNetTask_FriendInvite(username, sessionAddress);
}


#endif /* !SD_DEMO_BUILD */
