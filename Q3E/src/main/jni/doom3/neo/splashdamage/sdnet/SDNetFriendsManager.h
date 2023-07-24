// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETFRIENDSMANAGER_H__ )
#define __SDNETFRIENDSMANAGER_H__

#if !defined( SD_DEMO_BUILD )

#include "SDNetFriend.h"
#include "SDNetSession.h"

//===============================================================
//
//	sdNetFriendsManager
//
//===============================================================

class sdNetTask;

typedef idList< sdNetFriend* > sdNetFriendsList; 

class sdNetFriendsManager {
public:
	virtual							~sdNetFriendsManager() {}

	virtual const sdNetFriendsList&	GetFriendsList() const = 0;

	// friends who have invited you to be their friend
	virtual const sdNetFriendsList&	GetPendingFriendsList() const = 0;

	// friends that you have invited to be your friend
	virtual const sdNetFriendsList&	GetInvitedFriendsList() const = 0;

	virtual const sdNetFriendsList&	GetBlockedList() const = 0;

	virtual sdNetFriend*			FindFriend( const sdNetFriendsList& list, const char* username ) = 0;
	virtual int						FindFriendIndex( const sdNetFriendsList& list, const char* username ) = 0;

	virtual sdLock&					GetLock() = 0;

	//
	// Online functionality
	//

	// Initialize friends list
	virtual sdNetTask*				Init() = 0;

	// Invite a user to become a friend
	virtual sdNetTask*				ProposeFriendship( const char* username, const wchar_t* reason ) = 0;

	// Withdraw a friendship proposal
	virtual sdNetTask*				WithdrawProposal( const char* username ) = 0;

	// Accept a friendship proposal
	virtual sdNetTask*				AcceptProposal( const char* username ) = 0;

	// Reject a friendship proposal
	virtual sdNetTask*				RejectProposal( const char* username ) = 0;

	// Remove a user from the friends list
	virtual sdNetTask*				RemoveFriend( const char* username ) = 0;

	// Set the blocked status of a user
	virtual sdNetTask*				SetBlockedStatus( const char* username, const sdNetFriend::blockState_e blockState ) = 0;

	// Send a messsage to a friend
	virtual sdNetTask*				SendMessage( const char* username, const wchar_t* text ) = 0;

	// Invite a friend to a session
	virtual sdNetTask*				Invite( const char* username, const netadr_t& sessionAddress ) = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETFRIENDSMANAGER_H__ */
