// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetFriend_local.h"

//===============================================================
//
//	sdNetFriend_Local
//
//===============================================================

sdNetFriend_Local::sdNetFriend_Local()
    : onlineState(OS_OFFLINE),
    blockState(BS_NO_BLOCK)
{
    clientId.Invalidate();
}

sdNetFriend_Local::~sdNetFriend_Local() {
}

const char* sdNetFriend_Local::GetUsername() const {
    return username.c_str();
}

void sdNetFriend_Local::GetNetClientId( sdNetClientId& netClientId ) const {
    netClientId = clientId;
}


sdNetFriend::onlineState_e sdNetFriend_Local::GetState() const {
    return onlineState;
}


sdNetFriend::blockState_e sdNetFriend_Local::GetBlockedState() const {
    return blockState;
}


const sdNetMessageQueue& sdNetFriend_Local::GetMessageQueue() const {
    return messageQueue;
}

sdNetMessageQueue& sdNetFriend_Local::GetMessageQueue() {
    return messageQueue;
}


sdNetMessageHistory& sdNetFriend_Local::GetHistory() {
    return messageHistory;
}

