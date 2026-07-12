// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetTeamMember_local.h"

//===============================================================
//
//	sdNetTeamMember_Local
//
//===============================================================

sdNetTeamMember_Local::sdNetTeamMember_Local()
    : onlineState(OS_OFFLINE),
    memberStatus(MS_MEMBER)
{
    clientId.Invalidate();
}

sdNetTeamMember_Local::~sdNetTeamMember_Local() {
}

const char* sdNetTeamMember_Local::GetUsername() const {
    return username.c_str();
}

void sdNetTeamMember_Local::GetNetClientId( sdNetClientId& netClientId ) const {
    netClientId = clientId;
}


sdNetTeamMember::onlineState_e sdNetTeamMember_Local::GetState() const {
    return onlineState;
}


sdNetTeamMember::memberStatus_e sdNetTeamMember_Local::GetMemberStatus() const {
    return memberStatus;
}


const sdNetMessageQueue& sdNetTeamMember_Local::GetMessageQueue() const {
    return messageQueue;
}

sdNetMessageQueue& sdNetTeamMember_Local::GetMessageQueue() {
    return messageQueue;
}


sdNetMessageHistory& sdNetTeamMember_Local::GetHistory() {
    return messageHistory;
}

