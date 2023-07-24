// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_SDNETPROPERTIES_H__
#define __GAME_SDNETPROPERTIES_H__

#include "../guis/UserInterfaceTypes.h"
#include "../../sdnet/SDNetErrorCode.h"
#include "../../sdnet/SDNetTeamMember.h"
#include "../../framework/NotificationSystem.h"

class sdDeclLocStr;

class sdNetProperties : public sdUIPropertyHolder {
public:
	enum friendContextAction_e {
		FCA_NONE,
		FCA_RESPOND_TO_PROPOSAL,
		FCA_SEND_IM,
		FCA_READ_IM,
		FCA_RESPOND_TO_INVITE,
		FCA_BLOCKED,
		FCA_UNBLOCKED,
		FCA_SESSION_INVITE
	};

	enum teamContextAction_e {
		TCA_NONE,
		TCA_SEND_IM,
		TCA_READ_IM,
		TCA_NOTIFY_ADMIN,
		TCA_NOTIFY_OWNER,
		TCA_SESSION_INVITE,
	};

	enum usernameValidation_e {
		UV_VALID,
		UV_EMPTY_NAME,
		UV_DUPLICATE_NAME,
		UV_INVALID_NAME,
	};

	enum emailValidation_e {
		EV_VALID,
		EV_EMPTY_MAIL,
		EV_INVALID_MAIL,
		EV_CONFIRM_MISMATCH,
	};

	enum statusPriority_e {
		SP_NEW_MESSAGE,
		SP_ONSERVER,
		SP_ONLINE,
		SP_PENDING,
		SP_INVITED,
		SP_OFFLINE,
		SP_BLOCKED,
	};

	enum teamLevel_e {
		TL_OWNER,
		TL_ADMIN,
		TL_USER,
		TL_PENDING,
	};

	enum messageHistorySource_e {
		MHS_MIN = -1,
		MHS_FRIEND,
		MHS_TEAM,
		MHS_MAX,
	};
												sdNetProperties() {}
												~sdNetProperties() {}

	virtual sdUIFunctionInstance*				GetFunction( const char* name );

	virtual sdProperties::sdProperty*			GetProperty( const char* name );
	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler&	GetProperties() { return properties; }
	virtual const char*							FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	virtual const char*							GetName() const { return "SDNetProperties"; }

	void										Init();
	void										Shutdown();
	void										UpdateProperties();

	void										SetTaskActive( bool active );
	void										SetTaskResult( sdNetErrorCode_e errorCode, const sdDeclLocStr* resultMessage );
	void										ConnectFailed() { connectFailed = connectFailed + 1.0f; }

#if !defined( SD_DEMO_BUILD )
	void										SetTeamName( const char* name ) { teamName = name; }
#endif /* !SD_DEMO_BUILD */

	void										SetNumAvailableDWServers( const int numAvailableServers ) { this->numAvailableDWServers = numAvailableServers; }
	void										SetNumAvailableLANServers( const int numAvailableServers ) { this->numAvailableLANServers = numAvailableServers; }
	void										SetNumAvailableLANRepeaters( const int numAvailableServers ) { this->numAvailableLANRepeaters = numAvailableServers; }
	void										SetNumAvailableRepeaters( const int numAvailableServers ) { this->numAvailableRepeaters = numAvailableServers; }
	void										SetNumAvailableHistoryServers( const int numAvailableServers ) { this->numAvailableHistoryServers = numAvailableServers; }
	void										SetNumAvailableFavoritesServers( const int numAvailableServers ) { this->numAvailableFavoritesServers = numAvailableServers; }
#if !defined( SD_DEMO_BUILD )
	void										SetNumPendingClanInvites( const int numPendingClanInvites ) { this->numPendingClanInvites = numPendingClanInvites; }
	void										SetTeamMemberStatus( sdNetTeamMember::memberStatus_e teamMemberStatus ) { this->teamMemberStatus = teamMemberStatus; }
#endif /* !SD_DEMO_BUILD */
	void										SetServerRefreshComplete( bool set ) { this->serverRefreshComplete = set ? 1.0f : 0.0f; }
	void										SetHotServersRefreshComplete( bool set ) { this->hotServersRefreshComplete = set ? 1.0f : 0.0f; }

	void										SetFindingServers( bool set ) { this->findingServers = set ? 1.0f : 0.0f; }
#if !defined( SD_DEMO_BUILD )
	void										SetInitializingTeams( bool set ) { this->initializingTeams = set ? 1.0f : 0.0f; }
	void										SetInitializingFriends( bool set ) { this->initializingFriends = set ? 1.0f : 0.0f; }
#endif /* !SD_DEMO_BUILD */

	static void									CreateUserList( sdUIList* list );

	static void									CreateServerList( sdUIList* list );
	static void									CreateHotServerList( sdUIList* list );
	static void									CreateServerClientsList( sdUIList* list );
	static void									UpdateServer( sdUIList* list );
#if !defined( SD_DEMO_BUILD )
	static void									CreateFriendsList( sdUIList* list );
	static void									CreateTeamList( sdUIList* list );
	static void									CreateTeamInvitesList( sdUIList* list );
	static void									CreateAchievementsList( sdUIList* list );
	static void									CreateAchievementTasksList( sdUIList* list );
	static void									CreateMessageHistoryList( sdUIList* list );
	static void									CreateRetrievedUserNameList( sdUIList* list );
	static void									GenerateMessageHistoryFileName( const char* currentUser, const char* friendName, idStr& path );
	static sdNetMessageHistory*					GetMessageHistory( messageHistorySource_e source, const char* username, idStr& friendRawUserName );
#endif /* !SD_DEMO_BUILD */

	static void									FormatTimeStamp( const sysTime_t& time, idWStr& str );

private:
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/State";
	desc				= "State of network service.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					state;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/DisconnectReason";
	desc				= "Reason for disconnect where a value larger than 1 is a non graceful disconnect.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					disconnectReason;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/DisconnectString";
	desc				= "Disconnect reason string.";
	editor				= "edit";
	datatype			= "int";
	)
	sdIntProperty					disconnectString;
	// ===========================================
	
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/ServerRefreshComplete";
	desc				= "True if refresh is complete.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					serverRefreshComplete;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/HotServersRefreshComplete";
	desc				= "True if hot server refresh is complete.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					hotServersRefreshComplete;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/FindingServers";
	desc				= "True if finding list of servers (either new or refreshing).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					findingServers;
	// ===========================================

#if !defined( SD_DEMO_BUILD )
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/InitializingFriends";
	desc				= "True if initializing the friends list (as it might take some time).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					initializingFriends;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/InitializingTeams";
	desc				= "True if initializing clan list.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					initializingTeams;
	// ===========================================
#endif /* !SD_DEMO_BUILD */

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/HasActiveUser";
	desc				= "A user has logged in (but not necessarily online).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					hasActiveUser;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/ActiveUserState";
	desc				= "State of user logged on.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					activeUserState;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/ActiveUsername";
	desc				= "Name of the activer user.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty				activeUsername;
	// ===========================================

#if !defined( SD_DEMO_BUILD )
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/AccountUsername";
	desc				= "Name of the account.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty				accountUsername;
	// ===========================================
#endif /* !SD_DEMO_BUILD */

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/TaskActive";
	desc				= "True if a network service task is active. Changing password, blocking friends, creating clan, creating user, messaging friends, etc.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					taskActive;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/TaskErrorCode";
	desc				= "Result of a task once it has been completed.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					taskErrorCode;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/TaskResultMessage";
	desc				= "Result message of a task once it has been completed.";
	editor				= "edit";
	datatype			= "int";
	)
	sdIntProperty					taskResultMessage;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableDWServers";
	desc				= "Number of online servers available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableDWServers;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableLANServers";
	desc				= "Number of LAN servers available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableLANServers;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableHistoryServers";
	desc				= "Number of history servers available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableHistoryServers;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableRepeaters";
	desc				= "Number of repeaters available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableRepeaters;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableLANRepeaters";
	desc				= "Number of LAN repeaters available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableLANRepeaters;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumAvailableFavoritesServers";
	desc				= "Number of favorites servers available.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numAvailableFavoritesServers;
	// ===========================================

#if !defined( SD_DEMO_BUILD )
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumFriends";
	desc				= "Number of friends in the friends list.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numFriends;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumOnlineFriends";
	desc				= "Number of friends that are online.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numOnlineFriends;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumClanmates";
	desc				= "Number of clan mates.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numClanmates;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumOnlineClanmates";
	desc				= "Number of clan mates online.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numOnlineClanmates;
	// ===========================================

#if !defined( SD_DEMO_BUILD )
	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/SteamActive";
	desc				= "Steam is active.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					steamActive;
	// ===========================================


	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/StoredLicenseCode";
	desc				= "License Code.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty				storedLicenseCode;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/StoredLicenseCodeChecksum";
	desc				= "License Code Checksum.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty				storedLicenseCodeChecksum;
	// ===========================================
#endif /* !SD_DEMO_BUILD */

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/StatsRequestState";
	desc				= "State of stats request. Either requested/completed/failed.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					statsRequestState;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NumPendingClanInvites";
	desc				= "Number of pending clan invites.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					numPendingClanInvites;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/HasPendingFriendEvents";
	desc				= "Number of pending friend events (messages, invitations, etc.).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					hasPendingFriendEvents;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/HasPendingTeamEvents";
	desc				= "Pending clan events (messages, invitations, etc.).";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					hasPendingTeamEvents;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/TeamMemberStatus";
	desc				= "Status of owner, member/admin/owner.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					teamMemberStatus;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/HasAccount";
	desc				= "Has an account (but might not be logged in). Used for disabling buttons in the main menu.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					hasAccount;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/TeamName";
	desc				= "Name of the clan the player is in.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty				teamName;
	// ===========================================
#endif /* !SD_DEMO_BUILD */

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NotificationText";
	desc				= "Notification text at bottom center of the screen.";
	editor				= "edit";
	datatype			= "wstring";
	)
	sdWStringProperty				notificationText;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/NotifyExpireTime";
	desc				= "Time at which a notification expires and should fade out.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					notifyExpireTime;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "1. SDNet/ConnectFailed";
	desc				= "True if failed to connect to the demonware network service.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty					connectFailed;
	// ===========================================

	int								nextNotifyTime;

#if !defined( SD_DEMO_BUILD )
	int								nextUserUpdate;
#endif /* !SD_DEMO_BUILD */

	sdProperties::sdPropertyHandler	properties;

	idWStrList												systemNotifications;
#if !defined( SD_DEMO_BUILD )
	idList< sdnetTeamInviteNotification_t >					teamInviteNotifications;
	idList< sdnetFriendStateChangedNotification_t >			friendStateChangedNotifications;
	idList< sdnetTeamMemberStateChangedNotification_t >		teamMemberStateChangedNotifications;
	idList< sdnetFriendIMNotification_t >					friendIMNotifications;
	idList< sdnetTeamMemberIMNotification_t >				teamMemberIMNotifications;
	idList< sdnetFriendSessionInviteNotification_t >		friendSessionInviteNotifications;
	idList< sdnetTeamMemberSessionInviteNotification_t >	teamMemberSessionInviteNotifications;
	idList< sdnetTeamDissolvedNotification_t >				teamDissolvedNotifications;
	idList< sdnetTeamKickNotification_t >					teamKickNotifications;
#endif /* !SD_DEMO_BUILD */
};

#endif /* !__GAME_SDNETPROPERTIES_H__ */
