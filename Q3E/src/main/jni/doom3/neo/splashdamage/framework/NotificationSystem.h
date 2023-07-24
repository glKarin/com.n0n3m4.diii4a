// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __NOTIFICATIONSYSTEM_H__
#define __NOTIFICATIONSYSTEM_H__

#include "../sdnet/SDNetFriend.h"
#include "../sdnet/SDNetTeamMember.h"

enum notificationID_e {
	// SDNet
#if !defined( SD_DEMO_BUILD )
	NID_SDNET_FRIEND_STATE_CHANGED,
	NID_SDNET_TEAM_MEMBER_STATE_CHANGED,
	NID_SDNET_FRIEND_IM,	
	NID_SDNET_TEAM_MEMBER_IM,
	NID_SDNET_FRIEND_SESSION_INVITE,
	NID_SDNET_TEAM_MEMBER_SESSION_INVITE,
	NID_SDNET_TEAM_INVITE,
	NID_SDNET_TEAM_DISSOLVED,
	NID_SDNET_TEAM_KICK,
#endif /* !SD_DEMO_BUILD */
};

struct notification_t {
	notificationID_e				id;
};

#if !defined( SD_DEMO_BUILD )
struct sdnetFriendStateChangedNotification_t {
	notificationID_e				id;
	char							username[ MAX_USERNAME_LENGTH ];
	sdNetFriend::onlineState_e		state;

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamMemberStateChangedNotification_t {
	notificationID_e				id;
	char							username[ MAX_USERNAME_LENGTH ];
	sdNetTeamMember::onlineState_e	state;

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetFriendIMNotification_t {
	notificationID_e				id;
	char							username[ MAX_USERNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamMemberIMNotification_t {
	notificationID_e				id;
	char							username[ MAX_USERNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetFriendSessionInviteNotification_t {
	notificationID_e				id;
	char							username[ MAX_USERNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamMemberSessionInviteNotification_t {
	notificationID_e				id;
	const char*						username;
	char							team[ MAX_TEAMNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamInviteNotification_t {
	notificationID_e				id;
	char							team[ MAX_TEAMNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamDissolvedNotification_t {
	notificationID_e				id;
	char							team[ MAX_TEAMNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};

struct sdnetTeamKickNotification_t {
	notificationID_e				id;
	char							team[ MAX_TEAMNAME_LENGTH ];

	notification_t*					Cast() { return reinterpret_cast< notification_t* >( this ); }
};
#endif /* !SD_DEMO_BUILD */

template< typename T >
T* notification_cast( const notification_t* notification ) {
	return ( reinterpret_cast< const T* >( notification ) );
}

class sdNotificationSystem {
public:
	static const int				MAX_NOTIFICATIONS = 16;

public:
	virtual int						GetNumNotifications() const = 0;
	virtual const notification_t*	GetNotification( const int index ) const = 0;
};

extern sdNotificationSystem* notificationSystem;

#endif /* !__NOTIFICATIONSYSTEM_H__ */
