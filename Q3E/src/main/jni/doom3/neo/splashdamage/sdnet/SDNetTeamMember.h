// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETTEAMMEMBER_H__ )
#define __SDNETTEAMMEMBER_H__

#if !defined( SD_DEMO_BUILD )

//===============================================================
//
//	sdNetTeamMember
//
//===============================================================

const int MAX_USERNAME_LENGTH = 64;
const int MAX_TEAMNAME_LENGTH = 64;

class sdNetMessageQueue;
class sdNetMessageHistory;

class sdNetTeamMember {
public:
	enum onlineState_e {
		OS_OFFLINE,
		OS_ONLINE,
		OS_GHOST
	};

	enum memberStatus_e {
		MS_MEMBER,
		MS_ADMIN,
		MS_OWNER
	};

	virtual								~sdNetTeamMember() {}

	virtual const char*					GetUsername() const = 0;
	virtual void						GetNetClientId( sdNetClientId& netClientId ) const = 0;

	virtual onlineState_e				GetState() const = 0;

	virtual memberStatus_e				GetMemberStatus() const = 0;

	virtual const sdNetMessageQueue&	GetMessageQueue() const = 0;
	virtual sdNetMessageQueue&			GetMessageQueue() = 0;

	virtual sdNetMessageHistory&		GetHistory() = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETTEAMMEMBER_H__ */
