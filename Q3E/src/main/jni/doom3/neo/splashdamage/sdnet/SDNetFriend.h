// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETFRIEND_H__ )
#define __SDNETFRIEND_H__

#if !defined( SD_DEMO_BUILD )

//===============================================================
//
//	sdNetFriend
//
//===============================================================

class sdNetMessageQueue;
class sdNetMessageHistory;

class sdNetFriend {
public:
	enum onlineState_e {
		OS_OFFLINE,
		OS_ONLINE,
		OS_GHOST
	};

	enum blockState_e {
		BS_NO_BLOCK = 0,
		BS_FULL_BLOCK,
		BS_INVITES_BLOCK,
		BS_END
	};

	virtual								~sdNetFriend() {}

	virtual const char*					GetUsername() const = 0;
	virtual void						GetNetClientId( sdNetClientId& netClientId ) const = 0;

	virtual onlineState_e				GetState() const = 0;

	virtual blockState_e				GetBlockedState() const = 0;

	virtual const sdNetMessageQueue&	GetMessageQueue() const = 0;
	virtual sdNetMessageQueue&			GetMessageQueue() = 0;

	virtual sdNetMessageHistory&		GetHistory() = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETFRIEND_H__ */
