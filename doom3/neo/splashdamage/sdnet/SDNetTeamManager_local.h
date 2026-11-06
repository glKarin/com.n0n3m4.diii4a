// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETTEAMMANAGER_LOCAL_H__ )
#define __SDNETTEAMMANAGER_LOCAL_H__

#if !defined( SD_DEMO_BUILD )

#include "SDNetTeamManager.h"

//===============================================================
//
//	sdNetTeamManager
//
//===============================================================

class sdNetTeamManager_Local : public sdNetTeamManager {
public:
	sdNetTeamManager_Local();
	virtual											~sdNetTeamManager_Local();

	virtual bool									IsTeamMember() const;

	virtual const char*								GetTeamName() const;

	virtual const sdNetTeamMemberList&				GetMemberList() const;

	virtual const sdNetTeamMemberList&				GetPendingMemberList() const;

	virtual const sdNetTeamMemberList&				GetPendingInvitesList() const;

	virtual const sdNetTeamMember::memberStatus_e	GetMemberStatus() const;

	virtual sdNetTeamMember*						FindMember( const sdNetTeamMemberList& list, const char* username );
	virtual int										FindMemberIndex( const sdNetTeamMemberList& list, const char* username );

	virtual sdLock&									GetLock();

	//
	// Online functionality
	//

	// Initialize team list
	virtual sdNetTask*					Init();

	// Create a team
	virtual sdNetTask*					CreateTeam( const char* teamname );

	// Invite a user to become a team member
	virtual sdNetTask*					ProposeMembership( const char* username, const wchar_t* reason );

	// Withdraw a membership proposal
	virtual sdNetTask*					WithdrawMembership( const char* username );

	// Accept a membership proposal
	virtual sdNetTask*					AcceptMembership( const char* username, const sdNetTeamId& teamId );

	// Reject a membership proposal
	virtual sdNetTask*					RejectMembership( const char* username, const sdNetTeamId& teamId );

	// Remove a member from the team
	virtual sdNetTask*					RemoveMember( const char* username );

	// Send a message to a member
	virtual sdNetTask*					SendMessage( const char* username, const wchar_t* text );

	// Send a message to the whole team
	virtual sdNetTask*					BroadcastMessage( const wchar_t* text );

	// Invite a member to a session
	virtual sdNetTask*					Invite( const char* username, const netadr_t& sessionAddress );

	// Promote a user to administrator
	virtual sdNetTask*					PromoteMember( const char* username );

	// Demote an administrator to a normal member
	virtual sdNetTask*					DemoteMember( const char* username );

	// Transfer ownership to another member
	virtual sdNetTask*					TransferOwnership( const char* username );

	// Disband the team
	virtual sdNetTask*					DisbandTeam();

	// Leave the team, must specify a new member to be the owner if this member is the current owner
	virtual sdNetTask*					LeaveTeam( const char* username = NULL );

private:
	bool teamMember;
	idStr teamName;
	sdNetTeamMemberList memberList;
	sdNetTeamMemberList pendingMemberList;
	sdNetTeamMemberList pendingInvitesList;
	sdNetTeamMember::memberStatus_e memberStatus;
	sdLock lock;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETTEAMMANAGER_LOCAL_H__ */
