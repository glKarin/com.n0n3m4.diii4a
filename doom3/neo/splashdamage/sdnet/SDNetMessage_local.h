// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETMESSAGE_LOCAL_H__ )
#define __SDNETMESSAGE_LOCAL_H__

#include "SDNetMessage.h"

//===============================================================
//
//	sdNetMessage
//
//===============================================================

class sdNetMessage_Local : public sdNetMessage {
public:
	sdNetMessage_Local();

	virtual messageType_e		GetType() const;
	virtual const wchar_t*		GetText() const;
	virtual const byte*			GetData() const;
	virtual size_t				GetDataSize() const;	
	virtual sdNetMessageQueue*	GetSenderQueue() const;
	virtual const sysTime_t& 	GetTimeStamp() const;

private:
	messageType_e messageType;
	idWStr text;
	byte* data;
	int dataSize;
	sdNetMessageQueue *messageQueue;
	sysTime_t timeStamp;
};

//===============================================================
//
//	sdNetMessageQueue
//
//===============================================================

class sdNetMessageQueue_Local : public sdNetMessageQueue {
public:
	sdNetMessageQueue_Local();

	virtual const sdNetMessage*	GetMessages() const;
	virtual sdNetMessage*		GetMessages();
	virtual void				RemoveMessage( sdNetMessage* message );
	virtual void				GetMessagesOfType( idList< sdNetMessage* >& messages, const sdNetMessage::messageType_e type ) const;
	virtual bool				AnyMessagesOfType( const sdNetMessage::messageType_e type ) const;
};

#endif /* !__SDNETMESSAGE_LOCAL_H__ */
