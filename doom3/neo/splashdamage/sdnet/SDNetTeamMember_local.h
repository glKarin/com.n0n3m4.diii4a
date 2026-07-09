// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETTEAMMEMBER_LOCAL_H__ )
#define __SDNETTEAMMEMBER_LOCAL_H__

#if !defined( SD_DEMO_BUILD )

//===============================================================
//
//	sdNetTeamMember
//
//===============================================================

#include "SDNetMessageHistory.h"
#include "SDNetMessage_local.h"
#include "SDNet.h"

#include "SDNetTeamMember.h"

class sdNetTeamMember_Local : public sdNetTeamMember {
public:
	sdNetTeamMember_Local();
	virtual								~sdNetTeamMember_Local();

	virtual const char*					GetUsername() const;
	virtual void						GetNetClientId( sdNetClientId& netClientId ) const;

	virtual onlineState_e				GetState() const;

	virtual memberStatus_e				GetMemberStatus() const;

	virtual const sdNetMessageQueue&	GetMessageQueue() const;
	virtual sdNetMessageQueue&			GetMessageQueue();

	virtual sdNetMessageHistory&		GetHistory();

private:
	idStr username;
	onlineState_e onlineState;
	memberStatus_e memberStatus;
	sdNetMessageQueue_Local messageQueue;
	sdNetMessageHistory messageHistory;
	sdNetClientId clientId;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETTEAMMEMBER_LOCAL_H__ */
