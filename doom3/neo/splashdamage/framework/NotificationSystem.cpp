// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "NotificationSystem.h"

class sdNotificationSystemLocal : public sdNotificationSystem {
public:
	static const int				MAX_NOTIFICATIONS = 16;

public:
	virtual int						GetNumNotifications() const;
	virtual const notification_t*	GetNotification( const int index ) const;
};

int sdNotificationSystemLocal::GetNumNotifications() const {
	return 0;
}

const notification_t* sdNotificationSystemLocal::GetNotification( const int index ) const {
	return NULL;
}

static sdNotificationSystemLocal sdNotificationSystemLocal;

sdNotificationSystem *notificationSystem = &sdNotificationSystemLocal;

