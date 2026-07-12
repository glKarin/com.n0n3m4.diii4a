// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETFRIEND_LOCAL_H__ )
#define __SDNETFRIEND_LOCAL_H__

#if !defined( SD_DEMO_BUILD )

#include "SDNetMessage_local.h"
#include "SDNetMessageHistory.h"
#include "SDNet.h"

#include "SDNetFriend.h"

//===============================================================
//
//	sdNetFriend_Local
//
//===============================================================

class sdNetFriend_Local : public sdNetFriend {
public:
	sdNetFriend_Local();
	virtual								~sdNetFriend_Local();

	virtual const char*					GetUsername() const;
	virtual void						GetNetClientId( sdNetClientId& netClientId ) const;

	virtual onlineState_e				GetState() const;

	virtual blockState_e				GetBlockedState() const;

	virtual const sdNetMessageQueue&	GetMessageQueue() const;
	virtual sdNetMessageQueue&			GetMessageQueue();

	virtual sdNetMessageHistory&		GetHistory();

private:
	idStr username;
	onlineState_e onlineState;
	blockState_e blockState;
	sdNetMessageQueue_Local messageQueue;
	sdNetMessageHistory messageHistory;
	sdNetClientId clientId;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETFRIEND_LOCAL_H__ */
