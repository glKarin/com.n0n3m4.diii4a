// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetMessage_local.h"

//===============================================================
//
//	sdNetMessage
//
//===============================================================

sdNetMessage_Local::sdNetMessage_Local()
	: messageType(MT_INVALID),
	messageQueue(NULL),
	data(NULL),
	dataSize(0)
{
	memset(&timeStamp, 0, sizeof(timeStamp));
}

sdNetMessage::messageType_e sdNetMessage_Local::GetType() const {
	return messageType;
}

const wchar_t* sdNetMessage_Local::GetText() const {
	return text.c_str();
}

const byte* sdNetMessage_Local::GetData() const {
	return data;
}

size_t sdNetMessage_Local::GetDataSize() const {
	return dataSize;
}
	
sdNetMessageQueue* sdNetMessage_Local::GetSenderQueue() const {
	return messageQueue;
}

const sysTime_t& sdNetMessage_Local::GetTimeStamp() const {
	return timeStamp;
}


//===============================================================
//
//	sdNetMessageQueue
//
//===============================================================

sdNetMessageQueue_Local::sdNetMessageQueue_Local() {
}


const sdNetMessage* sdNetMessageQueue_Local::GetMessages() const {
	return NULL;
}

sdNetMessage* sdNetMessageQueue_Local::GetMessages() {
	return NULL;
}

void sdNetMessageQueue_Local::RemoveMessage( sdNetMessage* message ) {
}

void sdNetMessageQueue_Local::GetMessagesOfType( idList< sdNetMessage* >& messages, const sdNetMessage::messageType_e type ) const {
}

bool sdNetMessageQueue_Local::AnyMessagesOfType( const sdNetMessage::messageType_e type ) const {
	return false;
}

