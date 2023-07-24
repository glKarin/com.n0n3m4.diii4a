// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETMESSAGE_H__ )
#define __SDNETMESSAGE_H__

//===============================================================
//
//	sdNetMessage
//
//===============================================================

const int MAX_IM_TEXT_CHARACTERS = 512;

class sdNetMessageQueue;

class sdNetMessage {
public:
	enum messageType_e {
		MT_INVALID,
		MT_FRIENDSHIP_PROPOSAL,
		MT_MEMBERSHIP_PROPOSAL,
		MT_MEMBERSHIP_PROMOTION_TO_ADMIN,
		MT_MEMBERSHIP_PROMOTION_TO_OWNER,
		MT_IM,
		MT_SESSION_INVITE,
		MT_BLOCKED,
		MT_UNBLOCKED,
	};

	virtual messageType_e		GetType() const = 0;
	virtual const wchar_t*		GetText() const = 0;
	virtual const byte*			GetData() const = 0;
	virtual size_t				GetDataSize() const = 0;	
	virtual sdNetMessageQueue*	GetSenderQueue() const = 0;
	virtual const sysTime_t& 	GetTimeStamp() const = 0;
};

//===============================================================
//
//	sdNetMessageQueue
//
//===============================================================

class sdNetMessageQueue {
public:
	virtual const sdNetMessage*	GetMessages() const = 0;
	virtual sdNetMessage*		GetMessages() = 0;
	virtual void				RemoveMessage( sdNetMessage* message ) = 0;
	virtual void				GetMessagesOfType( idList< sdNetMessage* >& messages, const sdNetMessage::messageType_e type ) const = 0;
	virtual bool				AnyMessagesOfType( const sdNetMessage::messageType_e type ) const = 0;
};

#endif /* !__SDNETMESSAGE_H__ */
