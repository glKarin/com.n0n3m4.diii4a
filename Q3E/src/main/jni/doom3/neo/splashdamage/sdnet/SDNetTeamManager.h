// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETTEAMMANAGER_H__ )
#define __SDNETTEAMMANAGER_H__

#if !defined( SD_DEMO_BUILD )

#include "SDNetTeamManager.h"

#include "SDNet.h"
#include "SDNetTeamMember.h"
#include "SDNetSession.h"

//===============================================================
//
//	sdNetTeamManager
//
//===============================================================

struct sdNetTeamId : public sdNetEntityId {};

struct sdNetTeamInvite {
	sdNetTeamId netTeamId;
	char text[ MAX_TEAMNAME_LENGTH ];
};

typedef idList< sdNetTeamMember* > sdNetTeamMemberList; 

class sdNetTeamManager {
public:
	virtual											~sdNetTeamManager() {}

	virtual bool									IsTeamMember() const = 0;

	virtual const char*								GetTeamName() const = 0;

	virtual const sdNetTeamMemberList&				GetMemberList() const = 0;

	virtual const sdNetTeamMemberList&				GetPendingMemberList() const = 0;

	virtual const sdNetTeamMemberList&				GetPendingInvitesList() const = 0;

	virtual const sdNetTeamMember::memberStatus_e	GetMemberStatus() const = 0;

	virtual sdNetTeamMember*						FindMember( const sdNetTeamMemberList& list, const char* username ) = 0;
	virtual int										FindMemberIndex( const sdNetTeamMemberList& list, const char* username ) = 0;

	virtual sdLock&									GetLock() = 0;

	//
	// Online functionality
	//

	// Initialize team list
	virtual sdNetTask*					Init() = 0;

	// Create a team
	virtual sdNetTask*					CreateTeam( const char* teamname ) = 0;

	// Invite a user to become a team member
	virtual sdNetTask*					ProposeMembership( const char* username, const wchar_t* reason ) = 0;

	// Withdraw a membership proposal
	virtual sdNetTask*					WithdrawMembership( const char* username ) = 0;

	// Accept a membership proposal
	virtual sdNetTask*					AcceptMembership( const char* username, const sdNetTeamId& teamId ) = 0;

	// Reject a membership proposal
	virtual sdNetTask*					RejectMembership( const char* username, const sdNetTeamId& teamId ) = 0;

	// Remove a member from the team
	virtual sdNetTask*					RemoveMember( const char* username ) = 0;

	// Send a message to a member
	virtual sdNetTask*					SendMessage( const char* username, const wchar_t* text ) = 0;

	// Send a message to the whole team
	virtual sdNetTask*					BroadcastMessage( const wchar_t* text ) = 0;

	// Invite a member to a session
	virtual sdNetTask*					Invite( const char* username, const netadr_t& sessionAddress ) = 0;

	// Promote a user to administrator
	virtual sdNetTask*					PromoteMember( const char* username ) = 0;

	// Demote an administrator to a normal member
	virtual sdNetTask*					DemoteMember( const char* username ) = 0;

	// Transfer ownership to another member
	virtual sdNetTask*					TransferOwnership( const char* username ) = 0;

	// Disband the team
	virtual sdNetTask*					DisbandTeam() = 0;

	// Leave the team, must specify a new member to be the owner if this member is the current owner
	virtual sdNetTask*					LeaveTeam( const char* username = NULL ) = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETTEAMMANAGER_H__ */
