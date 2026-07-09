// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( SD_DEMO_BUILD )

#include "idlib/precompiled.h"

#include "SDNetTeamManager_local.h"

#include "SDNetTask_local.h"

//===============================================================
//
//	sdNetTeamManager
//
//===============================================================

sdNetTeamManager_Local::sdNetTeamManager_Local()
	: teamMember(false),
	memberStatus(sdNetTeamMember::MS_ADMIN)
{
}

sdNetTeamManager_Local::~sdNetTeamManager_Local() {
	
}

bool sdNetTeamManager_Local::IsTeamMember() const {
	return teamMember;
}

const char* sdNetTeamManager_Local::GetTeamName() const {
	return teamName.c_str();
}

const sdNetTeamMemberList& sdNetTeamManager_Local::GetMemberList() const {
	return memberList;
}

const sdNetTeamMemberList& sdNetTeamManager_Local::GetPendingMemberList() const {
	return pendingMemberList;
}

const sdNetTeamMemberList& sdNetTeamManager_Local::GetPendingInvitesList() const {
	return pendingInvitesList;
}

const sdNetTeamMember::memberStatus_e sdNetTeamManager_Local::GetMemberStatus() const {
	return memberStatus;
}

sdNetTeamMember* sdNetTeamManager_Local::FindMember( const sdNetTeamMemberList& list, const char* username ) {
	for (sdNetTeamMemberList::ConstIterator itor = list.Begin(); itor != list.End(); ++itor) {
		if (!idStr::Cmp((*itor)->GetUsername(), username))
			return *itor;
	}
	return NULL;
}

int sdNetTeamManager_Local::FindMemberIndex( const sdNetTeamMemberList& list, const char* username ) {
	for (int i = 0; i < list.Size(); ++i) {
		const sdNetTeamMember *item = list[i];
		if (!idStr::Cmp(item->GetUsername(), username))
			return i;
	}
	return -1;
}

sdLock& sdNetTeamManager_Local::GetLock() {
	return lock;
}

	//
	// Online functionality
	//

	// Initialize team list
sdNetTask* sdNetTeamManager_Local::Init() {
	return new sdNetTask_TeamManager_Init;
}

	// Create a team
sdNetTask* sdNetTeamManager_Local::CreateTeam( const char* teamname ) {
	return new sdNetTask_CreateTeam(teamname);
}

	// Invite a user to become a team member
sdNetTask* sdNetTeamManager_Local::ProposeMembership( const char* username, const wchar_t* reason ) {
	return new sdNetTask_ProposeMembership(username, reason);
}

	// Withdraw a membership proposal
sdNetTask* sdNetTeamManager_Local::WithdrawMembership( const char* username ) {
	return new sdNetTask_WithdrawMembership(username);
}

	// Accept a membership proposal
sdNetTask* sdNetTeamManager_Local::AcceptMembership( const char* username, const sdNetTeamId& teamId ) {
	return new sdNetTask_AcceptMembership(username, teamId);
}

	// Reject a membership proposal
sdNetTask* sdNetTeamManager_Local::RejectMembership( const char* username, const sdNetTeamId& teamId ) {
	return new sdNetTask_RejectMembership(username, teamId);
}

	// Remove a member from the team
sdNetTask* sdNetTeamManager_Local::RemoveMember( const char* username ) {
	return new sdNetTask_RemoveMember(username);
}

	// Send a message to a member
sdNetTask* sdNetTeamManager_Local::SendMessage( const char* username, const wchar_t* text ) {
	return new sdNetTask_TeamSendMessage(username, text);
}

	// Send a message to the whole team
sdNetTask* sdNetTeamManager_Local::BroadcastMessage( const wchar_t* text ) {
	return new sdNetTask_BroadcastMessage(text);
}

	// Invite a member to a session
sdNetTask* sdNetTeamManager_Local::Invite( const char* username, const netadr_t& sessionAddress ) {
	return new sdNetTask_TeamInvite(username, sessionAddress);
}

	// Promote a user to administrator
sdNetTask* sdNetTeamManager_Local::PromoteMember( const char* username ) {
	return new sdNetTask_PromoteMember(username);
}

	// Demote an administrator to a normal member
sdNetTask* sdNetTeamManager_Local::DemoteMember( const char* username ) {
	return new sdNetTask_DemoteMember(username);
}

	// Transfer ownership to another member
sdNetTask* sdNetTeamManager_Local::TransferOwnership( const char* username ) {
	return new sdNetTask_TransferOwnership(username);
}

	// Disband the team
sdNetTask* sdNetTeamManager_Local::DisbandTeam() {
	return new sdNetTask_DisbandTeam;
}

	// Leave the team, must specify a new member to be the owner if this member is the current owner
sdNetTask* sdNetTeamManager_Local::LeaveTeam( const char* username ) {
	return new sdNetTask_LeaveTeam(username);
}

#endif /* !SD_DEMO_BUILD */
