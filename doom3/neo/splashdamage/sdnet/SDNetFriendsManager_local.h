// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETFRIENDSMANAGER_LOCAL_H__ )
#define __SDNETFRIENDSMANAGER_LOCAL_H__

#if !defined( SD_DEMO_BUILD )

#include "SDNetFriendsManager.h"
#include "sys/sys_public.h"
#include "threading/Lock.h"

//===============================================================
//
//	sdNetFriendsManager
//
//===============================================================

class sdNetFriendsManager_Local : public sdNetFriendsManager {
public:
	sdNetFriendsManager_Local();
	virtual							~sdNetFriendsManager_Local();

	virtual const sdNetFriendsList&	GetFriendsList() const;

	// friends who have invited you to be their friend
	virtual const sdNetFriendsList&	GetPendingFriendsList() const;

	// friends that you have invited to be your friend
	virtual const sdNetFriendsList&	GetInvitedFriendsList() const;

	virtual const sdNetFriendsList&	GetBlockedList() const;

	virtual sdNetFriend*			FindFriend( const sdNetFriendsList& list, const char* username );
	virtual int						FindFriendIndex( const sdNetFriendsList& list, const char* username );

	virtual sdLock&					GetLock();

	//
	// Online functionality
	//

	// Initialize friends list
	virtual sdNetTask*				Init();

	// Invite a user to become a friend
	virtual sdNetTask*				ProposeFriendship( const char* username, const wchar_t* reason );

	// Withdraw a friendship proposal
	virtual sdNetTask*				WithdrawProposal( const char* username );

	// Accept a friendship proposal
	virtual sdNetTask*				AcceptProposal( const char* username );

	// Reject a friendship proposal
	virtual sdNetTask*				RejectProposal( const char* username );

	// Remove a user from the friends list
	virtual sdNetTask*				RemoveFriend( const char* username );

	// Set the blocked status of a user
	virtual sdNetTask*				SetBlockedStatus( const char* username, const sdNetFriend::blockState_e blockState );

	// Send a messsage to a friend
	virtual sdNetTask*				SendMessage( const char* username, const wchar_t* text );

	// Invite a friend to a session
	virtual sdNetTask*				Invite( const char* username, const netadr_t& sessionAddress );

private:
	sdNetFriendsList friendsList;
	sdNetFriendsList pendingFriendsList;
	sdNetFriendsList invitedFriendsList;
	sdNetFriendsList blockedFriendsList;
	sdLock lock;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETFRIENDSMANAGER_LOCAL_H__ */
