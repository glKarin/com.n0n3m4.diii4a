// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SDNetProperties.h"

#include "../../sdnet/SDNet.h"
#include "../../sdnet/SDNetUser.h"
#include "../../sdnet/SDNetAccount.h"
#include "../../sdnet/SDNetFriend.h"
#include "../../sdnet/SDNetMessage.h"
#include "../../sdnet/SDNetMessageHistory.h"
#include "../../sdnet/SDNetFriendsManager.h"
#include "../../sdnet/SDNetTeamManager.h"
#include "../../sdnet/SDNetSession.h"
#include "../../sdnet/SDNetProfile.h"

#include "../proficiency/StatsTracker.h"

#include "../guis/UserInterfaceLocal.h"
#include "../guis/UserInterfaceManager.h"
#include "../guis/UIList.h"

#include "../../decllib/declLocStr.h"

 /*
================
sdNetProperties::GetFunction
================
*/
sdUIFunctionInstance* sdNetProperties::GetFunction( const char* name ) {
	return gameLocal.GetSDNet().GetFunction( name );
}

/*
================
sdNetProperties::GetProperty
================
*/
sdProperties::sdProperty* sdNetProperties::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdNetProperties::GetProperty
============
*/
sdProperties::sdProperty* sdNetProperties::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop != NULL && prop->GetValueType() != type && type != sdProperties::PT_INVALID ) {
		gameLocal.Error( "sdNetProperties::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdNetProperties::Init
================
*/
SD_UI_PUSH_CLASS_TAG( sdNetProperties )
void sdNetProperties::Init() {
	properties.RegisterProperty( "state",						state );
	properties.RegisterProperty( "disconnectReason",			disconnectReason );
	properties.RegisterProperty( "disconnectString",			disconnectString );
	properties.RegisterProperty( "connectFailed",				connectFailed );

	properties.RegisterProperty( "serverRefreshComplete",		serverRefreshComplete );
	properties.RegisterProperty( "hotServersRefreshComplete",	hotServersRefreshComplete );
	properties.RegisterProperty( "findingServers",				findingServers );
#if !defined( SD_DEMO_BUILD )
	properties.RegisterProperty( "initializingTeams",			initializingTeams );
	properties.RegisterProperty( "initializingFriends",			initializingFriends );
#endif /* !SD_DEMO_BUILD */

	properties.RegisterProperty( "hasActiveUser",				hasActiveUser );
	properties.RegisterProperty( "activeUserState",				activeUserState );
	properties.RegisterProperty( "activeUsername",				activeUsername );
#if !defined( SD_DEMO_BUILD )
	properties.RegisterProperty( "accountUsername",				accountUsername );
#endif /* !SD_DEMO_BUILD */

	properties.RegisterProperty( "taskActive",					taskActive );
	properties.RegisterProperty( "taskErrorCode",				taskErrorCode );
	properties.RegisterProperty( "taskResultMessage",			taskResultMessage );

	properties.RegisterProperty( "numAvailableDWServers",		numAvailableDWServers );
	properties.RegisterProperty( "numAvailableLANServers",		numAvailableLANServers );
	properties.RegisterProperty( "numAvailableHistoryServers",	numAvailableHistoryServers );
	properties.RegisterProperty( "numAvailableFavoritesServers",numAvailableFavoritesServers );

#if !defined( SD_DEMO_BUILD )
	properties.RegisterProperty( "numAvailableRepeaters",		numAvailableRepeaters );
	properties.RegisterProperty( "numAvailableLANRepeaters",	numAvailableLANRepeaters );
#endif /* !SD_DEMO_BUILD */

#if !defined( SD_DEMO_BUILD )
	properties.RegisterProperty( "numPendingClanInvites",		numPendingClanInvites );
	properties.RegisterProperty( "numFriends",					numFriends );
	properties.RegisterProperty( "numOnlineFriends",			numOnlineFriends );
	properties.RegisterProperty( "numClanmates",				numClanmates );
	properties.RegisterProperty( "numOnlineClanmates",			numOnlineClanmates );

	properties.RegisterProperty( "statsRequestState",			statsRequestState );
	properties.RegisterProperty( "teamName",					teamName );
	properties.RegisterProperty( "teamMemberStatus",			teamMemberStatus );

	properties.RegisterProperty( "hasAccount",					hasAccount );
#endif /* !SD_DEMO_BUILD */
	properties.RegisterProperty( "notificationText",			notificationText );
	properties.RegisterProperty( "notifyExpireTime",			notifyExpireTime );

#if !defined( SD_DEMO_BUILD )
	properties.RegisterProperty( "hasPendingFriendEvents",		hasPendingFriendEvents );
	properties.RegisterProperty( "hasPendingTeamEvents",		hasPendingTeamEvents );

	properties.RegisterProperty( "steamActive",					steamActive );
	properties.RegisterProperty( "storedLicenseCode",			storedLicenseCode );
	properties.RegisterProperty( "storedLicenseCodeChecksum",	storedLicenseCodeChecksum );
#endif /* !SD_DEMO_BUILD */

	uiManager->RegisterListEnumerationCallback( "sdnetUsers",				CreateUserList );
	uiManager->RegisterListEnumerationCallback( "sdnetServers",				CreateServerList );
	uiManager->RegisterListEnumerationCallback( "sdnetHotServers",			CreateHotServerList );
	uiManager->RegisterListEnumerationCallback( "sdnetServerClients",		CreateServerClientsList );
	uiManager->RegisterListEnumerationCallback( "sdnetUpdateServer",		UpdateServer );
#if !defined( SD_DEMO_BUILD )
	uiManager->RegisterListEnumerationCallback( "sdnetFriends",				CreateFriendsList );
	uiManager->RegisterListEnumerationCallback( "sdnetTeam",				CreateTeamList );
	uiManager->RegisterListEnumerationCallback( "sdnetTeamInvites",			CreateTeamInvitesList );
	uiManager->RegisterListEnumerationCallback( "sdnetAchievements",		CreateAchievementsList );
	uiManager->RegisterListEnumerationCallback( "sdnetAchievementTasks",	CreateAchievementTasksList );
	uiManager->RegisterListEnumerationCallback( "sdnetMessageHistory",		CreateMessageHistoryList );
	uiManager->RegisterListEnumerationCallback( "sdnetRetrievedUserNames",	CreateRetrievedUserNameList );
#endif /* !SD_DEMO_BUILD */

#define SDNET_DEFINE( definition, enumeration ) sdDeclGUI::AddDefine( va( #definition " %i", enumeration ) );

	SD_UI_PUSH_GROUP_TAG( "Service State" )

	SD_UI_ENUM_TAG( SS_DISABLED, "Network service is disabled." )
	SDNET_DEFINE( SS_DISABLED,			sdNetService::SS_DISABLED );

	SD_UI_ENUM_TAG( SS_INITIALIZED, "Network service is initialized." )
	SDNET_DEFINE( SS_INITIALIZED,		sdNetService::SS_INITIALIZED );

	SD_UI_ENUM_TAG( SS_CONNECTING, "Connecting to demonware." )
	SDNET_DEFINE( SS_CONNECTING,		sdNetService::SS_CONNECTING );

	SD_UI_ENUM_TAG( SS_ONLINE, "Conntected to demonware." )
	SDNET_DEFINE( SS_ONLINE,			sdNetService::SS_ONLINE );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "User State" )

	SD_UI_ENUM_TAG( US_INACTIVE, "No active user." )
	SDNET_DEFINE( US_INACTIVE,			sdNetUser::US_INACTIVE );

	SD_UI_ENUM_TAG( US_ACTIVE, "User is active but not online." )
	SDNET_DEFINE( US_ACTIVE, 			sdNetUser::US_ACTIVE );

	SD_UI_ENUM_TAG( US_ONLINE, "User is online." )
	SDNET_DEFINE( US_ONLINE, 			sdNetUser::US_ONLINE );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Block State" )

#if !defined( SD_DEMO_BUILD )
	SD_UI_ENUM_TAG( BS_NO_BLOCK, "Friend not blocked." )
	SDNET_DEFINE( BS_NO_BLOCK,			sdNetFriend::BS_NO_BLOCK );

	SD_UI_ENUM_TAG( BS_FULL_BLOCK, "Friend blocked from messages and invites." )
	SDNET_DEFINE( BS_FULL_BLOCK,		sdNetFriend::BS_FULL_BLOCK );

	SD_UI_ENUM_TAG( BS_INVITES_BLOCK, "Friend blocked from invites." )
	SDNET_DEFINE( BS_INVITES_BLOCK,		sdNetFriend::BS_INVITES_BLOCK );
#endif /* !SD_DEMO_BUILD */

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Find Server Source." )

	SD_UI_ENUM_TAG( FS_LAN, "Searching LAN." )
	SDNET_DEFINE( FS_LAN,				sdNetManager::FS_LAN )

	SD_UI_ENUM_TAG( FS_INTERNET, "Searching internet." )
	SDNET_DEFINE( FS_INTERNET,			sdNetManager::FS_INTERNET )

#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
	SD_UI_ENUM_TAG( FS_LAN_REPEATER, "Searching LAN repeaters." )
	SDNET_DEFINE( FS_LAN_REPEATER,		sdNetManager::FS_LAN_REPEATER )

	SD_UI_ENUM_TAG( FS_INTERNET_REPEATER, "Searching internet repeaters." )
	SDNET_DEFINE( FS_INTERNET_REPEATER,	sdNetManager::FS_INTERNET_REPEATER )
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */

	SD_UI_ENUM_TAG( FS_HISTORY, "Searching history." )
	SDNET_DEFINE( FS_HISTORY,			sdNetManager::FS_HISTORY )
	
	SD_UI_ENUM_TAG( FS_CURRENT, "Searching current selection" )
	SDNET_DEFINE( FS_CURRENT,			sdNetManager::FS_CURRENT )
	
	SD_UI_ENUM_TAG( FS_FAVORITES, "Searching favorites" )
	SDNET_DEFINE( FS_FAVORITES,			sdNetManager::FS_FAVORITES )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Server Filter State" )

	SD_UI_ENUM_TAG( SFS_MIN, "Search filter min value." )
	SDNET_DEFINE( SFS_MIN,				sdNetManager::SFS_MIN )
	
	SD_UI_ENUM_TAG( SFS_DONTCARE, "Don't care about filtering for this item." )
	SDNET_DEFINE( SFS_DONTCARE,			sdNetManager::SFS_DONTCARE )
	
	SD_UI_ENUM_TAG( SFS_HIDE, "Hide servers which filter by this item." )
	SDNET_DEFINE( SFS_HIDE, 			sdNetManager::SFS_HIDE )
	
	SD_UI_ENUM_TAG( SFS_SHOWONLY, "Only show servers that filter by this item." )
	SDNET_DEFINE( SFS_SHOWONLY, 		sdNetManager::SFS_SHOWONLY )
	
	SD_UI_ENUM_TAG( SFS_MAX, "Search filter max value." )
	SDNET_DEFINE( SFS_MAX,				sdNetManager::SFS_MAX )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Server Filter Opcode" )

	SD_UI_ENUM_TAG( SFO_EQUAL, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_EQUAL,			sdNetManager::SFO_EQUAL )
	
	SD_UI_ENUM_TAG( SFO_NOT_EQUAL, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_NOT_EQUAL,		sdNetManager::SFO_NOT_EQUAL )
	
	SD_UI_ENUM_TAG( SFO_LESS, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_LESS,				sdNetManager::SFO_LESS )
	
	SD_UI_ENUM_TAG( SFO_GREATER, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_GREATER, 			sdNetManager::SFO_GREATER )
	
	SD_UI_ENUM_TAG( SFO_CONTAINS, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_CONTAINS,			sdNetManager::SFO_CONTAINS )
	
	SD_UI_ENUM_TAG( SFO_NOT_CONTAINS, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_NOT_CONTAINS,		sdNetManager::SFO_NOT_CONTAINS )
	
	SD_UI_ENUM_TAG( SFO_NOOP, "Filtering opcode for item." )
	SDNET_DEFINE( SFO_NOOP,				sdNetManager::SFO_NOOP )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Server Filter Result" )

	SD_UI_ENUM_TAG( SFR_OR, "OR opcode." )
	SDNET_DEFINE( SFR_OR,				sdNetManager::SFR_OR )
	
	SD_UI_ENUM_TAG( SFR_AND, "AND opcode." )
	SDNET_DEFINE( SFR_AND,				sdNetManager::SFR_AND )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Server Filter" )

	SD_UI_ENUM_TAG( SF_PASSWORDED, "Server is passworded filter." )
	SDNET_DEFINE( SF_PASSWORDED,		sdNetManager::SF_PASSWORDED )
	
	SD_UI_ENUM_TAG( SF_PUNKBUSTER, "Server has punkbuster filter." )
	SDNET_DEFINE( SF_PUNKBUSTER,		sdNetManager::SF_PUNKBUSTER )
	
	SD_UI_ENUM_TAG( SF_FRIENDLYFIRE, "Server has friendly fire filter." )
	SDNET_DEFINE( SF_FRIENDLYFIRE,		sdNetManager::SF_FRIENDLYFIRE )
	
	SD_UI_ENUM_TAG( SF_AUTOBALANCE, "Server has auto balancing of teams filter." )
	SDNET_DEFINE( SF_AUTOBALANCE,		sdNetManager::SF_AUTOBALANCE )
	
	SD_UI_ENUM_TAG( SF_EMPTY, "Server is empty filter." )
	SDNET_DEFINE( SF_EMPTY,				sdNetManager::SF_EMPTY )
	
	SD_UI_ENUM_TAG( SF_FULL, "Server is  full filter." )
	SDNET_DEFINE( SF_FULL,				sdNetManager::SF_FULL )
	
	SD_UI_ENUM_TAG( SF_PING, "Server ping filter." )
	SDNET_DEFINE( SF_PING,				sdNetManager::SF_PING )
	
//	SD_UI_ENUM_TAG( SF_BOTS, "Server has bots filter." )
//	SDNET_DEFINE( SF_BOTS,				sdNetManager::SF_BOTS )

	SD_UI_ENUM_TAG( SF_MAXBOTS, "Maximum bot count filter." )
	SDNET_DEFINE( SF_MAXBOTS,				sdNetManager::SF_MAXBOTS )

	SD_UI_ENUM_TAG( SF_MODS, "Server has mods filter." )
	SDNET_DEFINE( SF_MODS,				sdNetManager::SF_MODS )
	
	SD_UI_ENUM_TAG( SF_PURE, "Server is pure filter." )
	SDNET_DEFINE( SF_PURE,				sdNetManager::SF_PURE )
	
	SD_UI_ENUM_TAG( SF_LATEJOIN, "Server has late join filter." )
	SDNET_DEFINE( SF_LATEJOIN,			sdNetManager::SF_LATEJOIN )
	
	SD_UI_ENUM_TAG( SF_FAVORITE, "Server is in favorites filter." )
	SDNET_DEFINE( SF_FAVORITE,			sdNetManager::SF_FAVORITE )

#if !defined( SD_DEMO_BUILD ) && !defined( SD_DEMO_BUILD_CONSTRUCTION )
	SD_UI_ENUM_TAG( SF_RANKED, "Server is ranked filter." )
	SDNET_DEFINE( SF_RANKED,			sdNetManager::SF_RANKED )
	
	SD_UI_ENUM_TAG( SF_FRIENDS, "Server has friends playing on it filter." )
	SDNET_DEFINE( SF_FRIENDS,			sdNetManager::SF_FRIENDS )
#endif /* !SD_DEMO_BUILD && !SD_DEMO_BUILD_CONSTRUCTION */

	SD_UI_ENUM_TAG( SF_PLAYERCOUNT, "Server player count filter." )
	SDNET_DEFINE( SF_PLAYERCOUNT,		sdNetManager::SF_PLAYERCOUNT )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Clan Member Status" )

#if !defined( SD_DEMO_BUILD )
	SD_UI_ENUM_TAG( MS_ADMIN, "Member is an admin." )
	SDNET_DEFINE( MS_ADMIN,				sdNetTeamMember::MS_ADMIN )
	
	SD_UI_ENUM_TAG( MS_MEMBER, "Member is a regular member." )
	SDNET_DEFINE( MS_MEMBER,			sdNetTeamMember::MS_MEMBER )
	
	SD_UI_ENUM_TAG( MS_OWNER, "Member is an owner." )
	SDNET_DEFINE( MS_OWNER,				sdNetTeamMember::MS_OWNER )
#endif /* !SD_DEMO_BUILD */

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Player Stat" )

	SD_UI_ENUM_TAG( PS_NAME, "For getting player name." )
	SDNET_DEFINE( PS_NAME,				sdNetManager::PS_NAME )
	
	SD_UI_ENUM_TAG( PS_XP, "For getting player XP." )
	SDNET_DEFINE( PS_XP,				sdNetManager::PS_XP )
	
	SD_UI_ENUM_TAG( PS_RANK, "For getting player rank." )
	SDNET_DEFINE( PS_RANK,				sdNetManager::PS_RANK )
	
	SD_UI_ENUM_TAG( PS_TEAM, "For getting player team." )
	SDNET_DEFINE( PS_TEAM,				sdNetManager::PS_TEAM )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Player Reward" )

	SD_UI_ENUM_TAG( PR_MOST_XP, "Player with most XP." )
	SDNET_DEFINE( PR_MOST_XP,			sdNetManager::PR_MOST_XP )
	
	SD_UI_ENUM_TAG( PR_LEAST_XP, "Player with least XP." )
	SDNET_DEFINE( PR_LEAST_XP,			sdNetManager::PR_LEAST_XP )
	
	SD_UI_ENUM_TAG( PR_BEST_SOLDIER, "Best soldier reward." )
	SDNET_DEFINE( PR_BEST_SOLDIER,		sdNetManager::PR_BEST_SOLDIER )
	
	SD_UI_ENUM_TAG( PR_BEST_MEDIC, "Best medic reward." )
	SDNET_DEFINE( PR_BEST_MEDIC,		sdNetManager::PR_BEST_MEDIC )
	
	SD_UI_ENUM_TAG( PR_BEST_ENGINEER, "Best engineer reward." )
	SDNET_DEFINE( PR_BEST_ENGINEER,		sdNetManager::PR_BEST_ENGINEER )
	
	SD_UI_ENUM_TAG( PR_BEST_FIELDOPS, "Best field ops reward." )
	SDNET_DEFINE( PR_BEST_FIELDOPS,		sdNetManager::PR_BEST_FIELDOPS )
	
	SD_UI_ENUM_TAG( PR_BEST_COVERTOPS, "Best covert ops reward." )
	SDNET_DEFINE( PR_BEST_COVERTOPS,	sdNetManager::PR_BEST_COVERTOPS )
	
	SD_UI_ENUM_TAG( PR_BEST_LIGHTWEAPONS, "Best light weapons reward." )
	SDNET_DEFINE( PR_BEST_LIGHTWEAPONS,	sdNetManager::PR_BEST_LIGHTWEAPONS )
	
	SD_UI_ENUM_TAG( PR_BEST_BATTLESENSE, "Best battlesense  reward." )
	SDNET_DEFINE( PR_BEST_BATTLESENSE,	sdNetManager::PR_BEST_BATTLESENSE )
	
	SD_UI_ENUM_TAG( PR_BEST_VEHICLE, "Best vehicle reward." )
	SDNET_DEFINE( PR_BEST_VEHICLE,		sdNetManager::PR_BEST_VEHICLE )
	
	SD_UI_ENUM_TAG( PR_ACCURACY_HIGH, "Highest accuracy reward." )
	SDNET_DEFINE( PR_ACCURACY_HIGH,		sdNetManager::PR_ACCURACY_HIGH )
	
	SD_UI_ENUM_TAG( PR_MOST_OBJECTIVES, "Most objectives completed reward." )
	SDNET_DEFINE( PR_MOST_OBJECTIVES,	sdNetManager::PR_MOST_OBJECTIVES )
	
	SD_UI_ENUM_TAG( PR_PROFICIENCY, "Most upgrades reward." )
	SDNET_DEFINE( PR_PROFICIENCY,		sdNetManager::PR_PROFICIENCY )

	SD_UI_ENUM_TAG( PR_MOST_KILLS, "Most kills reward." )
	SDNET_DEFINE( PR_MOST_KILLS,		sdNetManager::PR_MOST_KILLS )
	
	SD_UI_ENUM_TAG( PR_MOST_DAMAGE, "Most damage reward." )
	SDNET_DEFINE( PR_MOST_DAMAGE,		sdNetManager::PR_MOST_DAMAGE )
	
	SD_UI_ENUM_TAG( PR_MOST_TEAMKILLS, "Most teamkills reward." )
	SDNET_DEFINE( PR_MOST_TEAMKILLS,	sdNetManager::PR_MOST_TEAMKILLS )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Find Server Mode" )

	SD_UI_ENUM_TAG( FSM_NEW, "Getting new list of servers." )
	SDNET_DEFINE( FSM_NEW,				sdNetManager::FSM_NEW )
	
	SD_UI_ENUM_TAG( FSM_REFRESH, "Refresing current list of servers." )
	SDNET_DEFINE( FSM_REFRESH,			sdNetManager::FSM_REFRESH )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Browser Column" )

	SD_UI_ENUM_TAG( BC_IP, "IP column." )
	SDNET_DEFINE( BC_IP,				sdNetManager::BC_IP )
	
	SD_UI_ENUM_TAG( BC_FAVORITE, "Favorites column." )
	SDNET_DEFINE( BC_FAVORITE,			sdNetManager::BC_FAVORITE )
	
	SD_UI_ENUM_TAG( BC_PASSWORD, "Password column." )
	SDNET_DEFINE( BC_PASSWORD,			sdNetManager::BC_PASSWORD )
	
	SD_UI_ENUM_TAG( BC_RANKED, "Ranked column." )
	SDNET_DEFINE( BC_RANKED,			sdNetManager::BC_RANKED )
	
	SD_UI_ENUM_TAG( BC_NAME, "Server name column." )
	SDNET_DEFINE( BC_NAME,				sdNetManager::BC_NAME )
	
	SD_UI_ENUM_TAG( BC_MAP, "Server map column." )
	SDNET_DEFINE( BC_MAP,				sdNetManager::BC_MAP )
	
	SD_UI_ENUM_TAG( BC_GAMETYPE_ICON, "Gametype icon column." )
	SDNET_DEFINE( BC_GAMETYPE_ICON,		sdNetManager::BC_GAMETYPE_ICON )
	
	SD_UI_ENUM_TAG( BC_GAMETYPE, "Gametype column." )
	SDNET_DEFINE( BC_GAMETYPE,			sdNetManager::BC_GAMETYPE )
	
	SD_UI_ENUM_TAG( BC_TIMELEFT, "Time left on map column." )
	SDNET_DEFINE( BC_TIMELEFT,			sdNetManager::BC_TIMELEFT )
	
	SD_UI_ENUM_TAG( BC_PLAYERS, "Players on server column." )
	SDNET_DEFINE( BC_PLAYERS,			sdNetManager::BC_PLAYERS )
	
	SD_UI_ENUM_TAG( BC_PING, "Server ping column." )
	SDNET_DEFINE( BC_PING,				sdNetManager::BC_PING )

#undef SDNET_DEFINE

#define SDNET_DEFINE( enumeration ) sdDeclGUI::AddDefine( va( #enumeration " %i", enumeration ) );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "SDNet Error Codes" )

	SD_UI_ENUM_TAG( SDNET_NO_ERROR, "No error." )
	SDNET_DEFINE( SDNET_NO_ERROR )
	
	SD_UI_ENUM_TAG( SDNET_UNKNOWN_ERROR, "Unknown error occured." )
	SDNET_DEFINE( SDNET_UNKNOWN_ERROR )
	
	SD_UI_ENUM_TAG( SDNET_BAD_PARAMETERS, "Bad parameters." )
	SDNET_DEFINE( SDNET_BAD_PARAMETERS )
	
	SD_UI_ENUM_TAG( SDNET_CANCELLED, "Cancelled." )
	SDNET_DEFINE( SDNET_CANCELLED )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Friend Context Action" )

	SD_UI_ENUM_TAG( FCA_NONE, "No friend context action." )
	SDNET_DEFINE( FCA_NONE )
	
	SD_UI_ENUM_TAG( FCA_RESPOND_TO_PROPOSAL, "Respond to invitation proposal." )
	SDNET_DEFINE( FCA_RESPOND_TO_PROPOSAL )
	
	SD_UI_ENUM_TAG( FCA_SEND_IM, "Send instant message to friend." )
	SDNET_DEFINE( FCA_SEND_IM )
	
	SD_UI_ENUM_TAG( FCA_READ_IM, "Read instant message from friend." )
	SDNET_DEFINE( FCA_READ_IM )
	
	SD_UI_ENUM_TAG( FCA_RESPOND_TO_INVITE, "Respond to invite." )
	SDNET_DEFINE( FCA_RESPOND_TO_INVITE )
	
	SD_UI_ENUM_TAG( FCA_BLOCKED, "You have been blocked." )
	SDNET_DEFINE( FCA_BLOCKED )
	
	SD_UI_ENUM_TAG( FCA_UNBLOCKED, "You have been unblocked." )
	SDNET_DEFINE( FCA_UNBLOCKED )
	
	SD_UI_ENUM_TAG( FCA_SESSION_INVITE, "You have been invited to a game." )
	SDNET_DEFINE( FCA_SESSION_INVITE )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Clan Context Action" )

	SD_UI_ENUM_TAG( TCA_NONE, "No clan context action." )
	SDNET_DEFINE( TCA_NONE )
	
	SD_UI_ENUM_TAG( TCA_SEND_IM, "Send instant message to clan member." )
	SDNET_DEFINE( TCA_SEND_IM )
	
	SD_UI_ENUM_TAG( TCA_READ_IM, "Read instant message from clan member." )
	SDNET_DEFINE( TCA_READ_IM )
	
	SD_UI_ENUM_TAG( TCA_NOTIFY_OWNER, "Notify clan member of promotion to owner." )
	SDNET_DEFINE( TCA_NOTIFY_OWNER )
	
	SD_UI_ENUM_TAG( TCA_NOTIFY_ADMIN, "Notify clan member of promotion to admin." )
	SDNET_DEFINE( TCA_NOTIFY_ADMIN )
	
	SD_UI_ENUM_TAG( TCA_SESSION_INVITE, "Invite a clan member to a game." )
	SDNET_DEFINE( TCA_SESSION_INVITE )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Username Validation" )

	SD_UI_ENUM_TAG( UV_VALID, "Username valid. Used during the creation of an account." )
	SDNET_DEFINE( UV_VALID )
	
	SD_UI_ENUM_TAG( UV_DUPLICATE_NAME, "Username already registered. Used during the creation of an account." )
	SDNET_DEFINE( UV_DUPLICATE_NAME )
	
	SD_UI_ENUM_TAG( UV_EMPTY_NAME, "Tried registering empty name. Used during the creation of an account." )
	SDNET_DEFINE( UV_EMPTY_NAME )
	
	SD_UI_ENUM_TAG( UV_INVALID_NAME, "Used during the creation of an account." )
	SDNET_DEFINE( UV_INVALID_NAME )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Email Validation" )

	SD_UI_ENUM_TAG( EV_VALID, "Valid email address. Used during the creation of an account." )
	SDNET_DEFINE( EV_VALID )
	
	SD_UI_ENUM_TAG( EV_EMPTY_MAIL, "Empty email address supplied. Used during the creation of an account." )
	SDNET_DEFINE( EV_EMPTY_MAIL )
	
	SD_UI_ENUM_TAG( EV_INVALID_MAIL, "Invalid email address supplied. Used during the creation of an account." )
	SDNET_DEFINE( EV_INVALID_MAIL )
	
	SD_UI_ENUM_TAG( EV_CONFIRM_MISMATCH, "Confirmation mismatch of email address. Used during the creation of an account." )
	SDNET_DEFINE( EV_CONFIRM_MISMATCH )

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Stats Request State" )

	SD_UI_ENUM_TAG( SR_EMPTY, "No current stats request." )
	SDNET_DEFINE( SR_EMPTY );
	
	SD_UI_ENUM_TAG( SR_REQUESTING, "Currently requesting stats." )
	SDNET_DEFINE( SR_REQUESTING );
	
	SD_UI_ENUM_TAG( SR_COMPLETED, "Completed stats request." )
	SDNET_DEFINE( SR_COMPLETED );
	
	SD_UI_ENUM_TAG( SR_FAILED, "Failed requesting stats." )
	SDNET_DEFINE( SR_FAILED );

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Message History Source" )

	SD_UI_ENUM_TAG( MHS_FRIEND, "A friend message." )
	SDNET_DEFINE( MHS_FRIEND );
	
	SD_UI_ENUM_TAG( MHS_TEAM, "A clan message." )
	SDNET_DEFINE( MHS_TEAM );

	SD_UI_POP_GROUP_TAG

#undef SDNET_DEFINE

	notifyExpireTime = 0.0f;
	nextNotifyTime = 0;
#if !defined( SD_DEMO_BUILD )
	nextUserUpdate = 0;
#endif /* !SD_DEMO_BUILD */
	connectFailed = 0.0f;

#if !defined( SD_DEMO_BUILD )
	steamActive = networkService->IsSteamActive();

	if( steamActive ) {
		idStr fullCode = networkService->GetStoredLicenseCode();
		idStrList code;
		idSplitStringIntoList( code, fullCode.c_str(), " " );

		if( code.Num() == 2 && code[ 0 ].Length() == 16 && code[ 1 ].Length() == 2 ) {
			storedLicenseCode = code[ 0 ];
			storedLicenseCodeChecksum = code[ 1 ];
		}	
	}
#endif /* !SD_DEMO_BUILD */
}
SD_UI_POP_CLASS_TAG

/*
================
sdNetProperties::Shutdown
================
*/
void sdNetProperties::Shutdown() {
	properties.Clear();
}

idCVar gui_notificationTime( "gui_notificationTime", "8", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "length of time a user notification is on screen, in seconds" );
idCVar gui_notificationPause( "gui_notificationPause", "5", CVAR_FLOAT | CVAR_ARCHIVE | CVAR_PROFILE, "length of time between successive notifications, in seconds" );

/*
================
sdNetProperties::UpdateProperties
================
*/
void sdNetProperties::UpdateProperties() {
	sdNetUser* activeUser = networkService->GetActiveUser();

	int now = uiManager->GetLastNonGameTime();

	if ( activeUser ) {
		hasActiveUser = true;

		activeUserState = activeUser->GetState();
		activeUsername = activeUser->GetUsername();

		const sdNetAccount& account = activeUser->GetAccount();

#if !defined( SD_DEMO_BUILD )
		accountUsername = account.GetUsername();
		hasAccount = accountUsername.GetValue().IsEmpty() == false;

		if( now >= nextUserUpdate ) {
			// update friend info
			{
				sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
				const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
				numFriends = friends.Num();

				if( networkService->GetFriendsManager().GetPendingFriendsList().Num() > 0 ) {
					hasPendingFriendEvents = 1.0f;
				} else {
					int i;
					for( i = 0; i < friends.Num(); i++ ) {
						if( friends[ i ]->GetMessageQueue().GetMessages() != NULL ) {
							hasPendingFriendEvents = 1.0f;
							break;
						}
					}
					if( i >= friends.Num() ) {
						hasPendingFriendEvents = 0.0f;
					}
				}

				int numOnline = 0;
				for( int i = 0; i < friends.Num(); i++ ) {
					if( friends[ i ]->GetState() == sdNetFriend::OS_ONLINE ) {
						numOnline++;
					}
				}
				numOnlineFriends = numOnline;
			}

			// update team info
			{

				sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
				const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
				numClanmates = members.Num() > 1 ? ( members.Num() - 1 ) : 0;	// don't include yourself

				if( networkService->GetTeamManager().GetPendingInvitesList().Num() > 0 ) {
					hasPendingTeamEvents = 1.0f;
				} else {
					int i;
					for( i = 0; i < members.Num(); i++ ) {
						// skip yourself
						if( idStr::Icmp( members[ i ]->GetUsername(), activeUser->GetUsername() ) == 0 ) {
							continue;
						}

						if( members[ i ]->GetMessageQueue().GetMessages() != NULL ) {
							hasPendingTeamEvents = 1.0f;
							break;
						}
					}
					if( i >= members.Num() ) {
						hasPendingTeamEvents = 0.0f;
					}
				}

				int numOnline = 0;
				for( int i = 0; i < members.Num(); i++ ) {
					// skip yourself
					if( idStr::Icmp( members[ i ]->GetUsername(), activeUser->GetUsername() ) == 0 ) {
						continue;
					}

					if( members[ i ]->GetState() == sdNetTeamMember::OS_ONLINE ) {
						numOnline++;
					}
				}
				numOnlineClanmates = numOnline;
			}

			nextUserUpdate = now + 1000;
		}
#endif /* !SD_DEMO_BUILD */

	} else {
		hasActiveUser = 0.0f;
#if !defined( SD_DEMO_BUILD )
		hasAccount = 0.0f;
		numFriends = 0.0f;
		hasPendingTeamEvents = 0.0f;
		hasPendingFriendEvents = 0.0f;
#endif /* !SD_DEMO_BUILD */

		activeUserState = sdNetUser::US_INACTIVE;
	}

	disconnectReason = networkService->GetDisconnectReason();
	switch( networkService->GetDisconnectReason() ) {
		case sdNetService::DR_CONNECTION_RESET:
			disconnectString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/sdnetdisconnected" )->Index();
			break;
		case sdNetService::DR_DUPLICATE_AUTH:
			disconnectString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/sdnetotheruserauthed" )->Index();
			break;
		default:
			disconnectString = -1;
			break;
	}
	state = networkService->GetState();
#if !defined( SD_DEMO_BUILD )
	statsRequestState = sdGlobalStatsTracker::GetInstance().GetLocalStatsRequestState();
#endif /* !SD_DEMO_BUILD */

	serverRefreshComplete = 0.0f;
//	hotServersRefreshComplete = 0.0f;

	systemNotifications = gameLocal.GetSystemNotifications();

	if( activeUser != NULL ) {
		int numNotifications = notificationSystem->GetNumNotifications();
#if !defined( SD_DEMO_BUILD )
		for( int i = 0; i < numNotifications; i++ ) {
			const notification_t* notification = notificationSystem->GetNotification( i );

			switch( notification->id ) {
				case NID_SDNET_FRIEND_STATE_CHANGED: {
					friendStateChangedNotifications.Alloc() = *notification_cast< const sdnetFriendStateChangedNotification_t >( notification );
					break;
				}
				case NID_SDNET_TEAM_MEMBER_STATE_CHANGED: {
					teamMemberStateChangedNotifications.Alloc() = *notification_cast< const sdnetTeamMemberStateChangedNotification_t >( notification );
					break;
				}
				case NID_SDNET_TEAM_DISSOLVED: {
					teamDissolvedNotifications.Alloc() = *notification_cast< const sdnetTeamDissolvedNotification_t >( notification );
					break;
														  }
				case NID_SDNET_FRIEND_IM: {
					friendIMNotifications.Alloc() = *notification_cast< const sdnetFriendIMNotification_t >( notification );
					break;
				}
				case NID_SDNET_TEAM_INVITE: {
					teamInviteNotifications.Alloc() = *notification_cast< const sdnetTeamInviteNotification_t >( notification );
					break;
											   }
				case NID_SDNET_TEAM_MEMBER_IM: {
					teamMemberIMNotifications.Alloc() = *notification_cast< const sdnetTeamMemberIMNotification_t >( notification );
					break;
				}
				case NID_SDNET_FRIEND_SESSION_INVITE: {
					friendSessionInviteNotifications.Alloc() = *notification_cast< const sdnetFriendSessionInviteNotification_t >( notification );
					break;
				}
				case NID_SDNET_TEAM_MEMBER_SESSION_INVITE: {
					teamMemberSessionInviteNotifications.Alloc() = *notification_cast< const sdnetTeamMemberSessionInviteNotification_t >( notification );
					break;
				}
				case NID_SDNET_TEAM_KICK: {
					teamKickNotifications.Alloc() = *notification_cast< const sdnetTeamKickNotification_t >( notification );
					break;
				 }
				default:
					gameLocal.Warning( "sdNetProperties::UpdateProperties: unknown notification type '%i'", notification->id );
			}
		}
#endif /* !SD_DEMO_BUILD */
	}

	int totalPause = SEC2MS( gui_notificationPause.GetFloat() + gui_notificationTime.GetFloat() );

	// System text notifications
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < systemNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			notificationText = systemNotifications[ i ];
			systemNotifications.RemoveIndex( 0 );
			break;
		}
	}

#if !defined( SD_DEMO_BUILD )
	// Friend session invite
	if( activeUser == NULL ) {
		friendSessionInviteNotifications.Clear();
	}

	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < friendSessionInviteNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			idWStrList args( 1 );
			args.Append( va( L"%hs", friendSessionInviteNotifications[ i ].username ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/friendsessioninvite", args );
			friendSessionInviteNotifications.RemoveIndex( 0 );
			break;
		}
	}

	// Team invite
	if( activeUser == NULL ) {
		teamInviteNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < teamInviteNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			idWStrList args( 1 );
			args.Append( va( L"%hs", teamInviteNotifications[ i ].team ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/teaminvite", args );
			teamInviteNotifications.RemoveIndex( 0 );
			break;
		}
	}

	// Team session invite
	if( activeUser == NULL ) {
		teamMemberSessionInviteNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < teamMemberSessionInviteNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			idWStrList args( 2 );
			args.Append( va( L"%hs", teamMemberSessionInviteNotifications[ i ].username ) );
			args.Append( va( L"%hs", teamMemberSessionInviteNotifications[ i ].team ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/teamsessioninvite", args );
			teamMemberSessionInviteNotifications.RemoveIndex( 0 );
			break;
		}
	}

	// Team dissolved
	if( activeUser == NULL ) {
		teamDissolvedNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < teamDissolvedNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			idWStrList args( 1 );
			args.Append( va( L"%hs", teamDissolvedNotifications[ i ].team ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/teamdissolved", args );
			teamDissolvedNotifications.RemoveIndex( 0 );
			break;
		}
	}

	// Team kick
	if( activeUser == NULL ) {
		teamKickNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		for( int i = 0; i < teamKickNotifications.Num(); i++ ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;

			idWStrList args( 1 );
			args.Append( va( L"%hs", teamKickNotifications[ i ].team ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/kickedfromteam", args );
			teamKickNotifications.RemoveIndex( 0 );
			break;
		}
	}

	// Friend IM
	if( activeUser == NULL ) {
		friendIMNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		if( friendIMNotifications.Num() >= 3 ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;
			idWStrList args( 1 );
			args.Append( va( L"%i", friendIMNotifications.Num() ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/multiple/friendims", args );
			friendIMNotifications.Clear();
		} else {
			for( int i = 0; i < friendIMNotifications.Num(); i++ ) {
				notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
				nextNotifyTime = now + totalPause;

				idWStrList args( 1 );
				args.Append( va( L"%hs", friendIMNotifications[ i ].username ) );
				notificationText = common->LocalizeText( "guis/mainmenu/notify/friendim", args );
				friendIMNotifications.RemoveIndex( 0 );
				break;
			}
		}
	}

	// Teammate IM
	if( activeUser == NULL ) {
		teamMemberIMNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		if( teamMemberIMNotifications.Num() >= 3 ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;
			idWStrList args( 1 );
			args.Append( va( L"%i", teamMemberIMNotifications.Num() ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/multiple/teamims", args );
			teamMemberIMNotifications.Clear();
		} else {
			for( int i = 0; i < teamMemberIMNotifications.Num(); i++ ) {
				notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
				nextNotifyTime = now + totalPause;

				idWStrList args( 1 );
				args.Append( va( L"%hs", teamMemberIMNotifications[ i ].username ) );
				notificationText = common->LocalizeText( "guis/mainmenu/notify/teamim", args );
				teamMemberIMNotifications.RemoveIndex( 0 );
				break;
			}
		}
	}

	// Teammate on/offline
	if( activeUser == NULL ) {
		teamMemberStateChangedNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		int numOnline = 0;
		for( int i = 0; i < teamMemberStateChangedNotifications.Num(); i++ ) {
			if( teamMemberStateChangedNotifications[ i ].state != sdNetTeamMember::OS_ONLINE ) {
				continue;
			}
			numOnline++;
		}

		if( numOnline >= 3 ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;
			idWStrList args( 1 );
			args.Append( va( L"%i", teamMemberStateChangedNotifications.Num() ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/multiple/teammembersonline", args );

			for( int i = teamMemberStateChangedNotifications.Num() - 1; i >= 0; i-- ) {
				if( teamMemberStateChangedNotifications[ i ].state == sdNetTeamMember::OS_ONLINE ) {
					teamMemberStateChangedNotifications.RemoveIndex( i );
				}
			}
		} else {
			for( int i = 0; i < teamMemberStateChangedNotifications.Num(); i++ ) {
				if( teamMemberStateChangedNotifications[ i ].state != sdNetTeamMember::OS_ONLINE ) {
					continue;
				}

				notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
				nextNotifyTime = now + totalPause;

				idWStrList args( 1 );
				args.Append( va( L"%hs", teamMemberStateChangedNotifications[ i ].username ) );
				notificationText = common->LocalizeText( "guis/mainmenu/notify/teammemberonline", args );
				teamMemberStateChangedNotifications.RemoveIndex( 0 );
				break;
			}
		}
	}

	// Friend on/offline
	if( activeUser == NULL ) {
		friendStateChangedNotifications.Clear();
	}
	if( now >= nextNotifyTime || nextNotifyTime == 0 ) {
		int numOnline = 0;
		for( int i = 0; i < friendStateChangedNotifications.Num(); i++ ) {
			if( friendStateChangedNotifications[ i ].state == sdNetFriend::OS_ONLINE ) {
				numOnline++;
			}
		}

		if( numOnline >= 3 ) {
			notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
			nextNotifyTime = now + totalPause;
			idWStrList args( 1 );
			args.Append( va( L"%i", friendStateChangedNotifications.Num() ) );
			notificationText = common->LocalizeText( "guis/mainmenu/notify/multiple/friendsonline", args );

			for( int i = friendStateChangedNotifications.Num() - 1; i >= 0; i-- ) {
				if( friendStateChangedNotifications[ i ].state == sdNetFriend::OS_ONLINE ) {
					friendStateChangedNotifications.RemoveIndex( i );
				}
			}
		} else {
			for( int i = 0; i < friendStateChangedNotifications.Num(); i++ ) {
				if( friendStateChangedNotifications[ i ].state != sdNetFriend::OS_ONLINE ) {
					continue;
				}

				notifyExpireTime = now + SEC2MS( gui_notificationTime.GetFloat() );
				nextNotifyTime = now + totalPause;

				idWStrList args( 1 );
				args.Append( va( L"%hs", friendStateChangedNotifications[ i ].username ) );
				notificationText = common->LocalizeText( "guis/mainmenu/notify/friendonline", args );
				friendStateChangedNotifications.RemoveIndex( 0 );
				break;
			}
		}
	}

#endif /* !SD_DEMO_BUILD */
}

/*
================
sdNetProperties::SetTaskActive
================
*/
void sdNetProperties::SetTaskActive( bool active ) {
	taskActive = active;
}

/*
================
sdNetProperties::SetTaskResult
================
*/
void sdNetProperties::SetTaskResult( sdNetErrorCode_e errorCode, const sdDeclLocStr* resultMessage ) {
	taskErrorCode = errorCode;
	taskResultMessage = resultMessage->Index();
}

/*
============
sdNetProperties::CreateUserList
============
*/
void sdNetProperties::CreateUserList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	int toSelect = -1;
	for ( int i = 0; i < networkService->NumUsers(); i++ ) {
		sdNetUser* user = networkService->GetUser( i );
		int defaultUser = user->GetProfile().GetProperties().GetBool( "default", "0" ) ? 1 : 0;

#if !defined( SD_DEMO_BUILD )
		const sdNetAccount& account = user->GetAccount();

		bool hasAccount = account.GetUsername()[ 0 ] != '\0';
		sdUIList::InsertItem( list, va( L"%hs\t<material = '%hs'>%i\t%hs\t%i", user->GetRawUsername(), hasAccount ? "friends/online" : "friends/offline", hasAccount ? 1 : 0, user->GetUsername(), defaultUser ), -1, 0 );
#else
		sdUIList::InsertItem( list, va( L"%hs\t<material = '%hs'>%i\t%hs\t%i", user->GetRawUsername(), "friends/online", 1, user->GetUsername(), defaultUser ), -1, 0 );
#endif /* !SD_DEMO_BUILD */
	}
}

/*
============
sdNetProperties::CreateServerList
============
*/
void sdNetProperties::CreateServerList( sdUIList* list ) {
	int iMode;
	list->GetUI()->PopScriptVar( iMode );

	int iSource;
	list->GetUI()->PopScriptVar( iSource );

	bool flagOffline;
	list->GetUI()->PopScriptVar( flagOffline );

	sdNetManager::findServerSource_e source;
	if( !sdIntToContinuousEnum< sdNetManager::findServerSource_e >( iSource, sdNetManager::FS_MIN, sdNetManager::FS_MAX, source ) ) {
		gameLocal.Error( "CreateServerList: source '%i' out of range", iSource );
		return;
	}

	sdNetManager::findServerMode_e mode;
	if( !sdIntToContinuousEnum< sdNetManager::findServerMode_e >( iMode, sdNetManager::FSM_MIN, sdNetManager::FSM_MAX, mode ) ) {
		gameLocal.Error( "CreateServerList: mode '%i' out of range", iMode );
		return;
	}

	gameLocal.GetSDNet().CreateServerList( list, source, mode, flagOffline );
}

/*
============
sdNetProperties::CreateHotServerList
============
*/
void sdNetProperties::CreateHotServerList( sdUIList* list ) {
	int iSource;
	list->GetUI()->PopScriptVar( iSource );

	sdNetManager::findServerSource_e source;
	if( !sdIntToContinuousEnum< sdNetManager::findServerSource_e >( iSource, sdNetManager::FS_MIN, sdNetManager::FS_MAX, source ) ) {
		gameLocal.Error( "CreateServerList: source '%i' out of range", iSource );
		return;
	}

	gameLocal.GetSDNet().CreateHotServerList( list, source );
}

/*
============
sdNetProperties::UpdateServer
============
*/
void sdNetProperties::UpdateServer( sdUIList* list ) {
	int iSource;
	list->GetUI()->PopScriptVar( iSource );

	sdNetManager::findServerSource_e source;
	if( !sdIntToContinuousEnum< sdNetManager::findServerSource_e >( iSource, sdNetManager::FS_MIN, sdNetManager::FS_MAX, source ) ) {
		gameLocal.Error( "CreateServerList: source '%i' out of range", iSource );
		return;
	}

	idStr sessionName;
	list->GetUI()->PopScriptVar( sessionName );

	gameLocal.GetSDNet().UpdateServer( *list, sessionName, source );
}

/*
============
sdNetProperties::CreateServerClientsList
============
*/
void sdNetProperties::CreateServerClientsList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	float fIndex;
	list->GetUI()->PopScriptVar( fIndex );
	float fSource;
	list->GetUI()->PopScriptVar( fSource );

	const sdNetSession* session = gameLocal.GetSDNet().GetSession( fSource, idMath::Ftoi( fIndex ) );
	if( session == NULL ) {
		return;
	}

	idWStr cleaned;
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		const sdNetSession::sessionClientInfo_t& client = session->GetClientInfo( i );
		if( !client.connected ) {
			continue;
		}

		cleaned = va( L"%hs", client.nickname );
		sdUIList::CleanUserInput( cleaned );
		int index = sdUIList::InsertItem( list, va( L"%hs\t%ls\t%d", client.isBot ? "<material = 'bot'>1" : "<material = 'friends/onserver'>0", cleaned.c_str(), client.ping ), -1, 0 );
	}
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetProperties::CreateFriendsList
============
*/
void sdNetProperties::CreateFriendsList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdScopedLock< true > friendsLock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& blockedFriends = networkService->GetFriendsManager().GetBlockedList();
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	for( int i = 0; i < friends.Num(); i++ ) {
		const sdNetFriend* mate = friends[ i ];

		const char*			string		= "guis/mainmenu/offline";
		const char*			material	= "friends/offline";
		statusPriority_e	status		= SP_OFFLINE;

		if( mate->GetState() == sdNetFriend::OS_ONLINE ) {
			sdNetClientId id;
			mate->GetNetClientId( id );
			const idDict* profile = networkService->GetProfileProperties( id );
			const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );

			if( idStr::Cmp( server, "0.0.0.0:0" ) == 0 ) {
				material	= "friends/online";
				string		= "guis/mainmenu/friends/online";
				status = SP_ONLINE;
			} else {
				material	= "friends/onserver";
				string		= "guis/mainmenu/friends/onserver";
				status = SP_ONSERVER;
			}
		}

		if( networkService->GetFriendsManager().FindFriend( blockedFriends, mate->GetUsername() ) ) {
			material = "friends/blocked";
			string		= "guis/mainmenu/friends/blocked";
			status = SP_BLOCKED;
		}

		idListGranularityOne< sdNetMessage* > messages;
		mate->GetMessageQueue().GetMessagesOfType( messages, sdNetMessage::MT_IM );
		if( !messages.Empty() ) {
			material	 = "friends/newmessage";
			string		= "guis/mainmenu/friends/newmessage";
			status		= SP_NEW_MESSAGE;
		} else {
			const sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if( message != NULL ) {
				switch( message->GetType() ) {
				case sdNetMessage::MT_SESSION_INVITE:
					material	= "friends/newevent";
					string		= "guis/mainmenu/friends/serverinvite";
					status		= SP_NEW_MESSAGE;
					break;
				case sdNetMessage::MT_BLOCKED:		// FALL THROUGH
					material	= "friends/newevent";
					string		= "guis/mainmenu/friends/blockedby";
					status	= SP_NEW_MESSAGE;
					break;
				case sdNetMessage::MT_UNBLOCKED:
					material	= "friends/newevent";
					string		= "guis/mainmenu/friends/unblockedby";
					status	= SP_NEW_MESSAGE;
					break;
				}
			}
		}

		sdUIList::InsertItem( list, va( L"<loc = '%hs'><material = '%hs'>%i\t%hs\t", string, material, static_cast< int >( status ), mate->GetUsername() ), 0, 0 );
	}

	const sdNetFriendsList& pendingFriends = networkService->GetFriendsManager().GetPendingFriendsList();
	for( int i = 0; i < pendingFriends.Num(); i++ ) {
		const sdNetFriend* mate = pendingFriends[ i ];
		sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/pending'><material = 'friends/pending'>%i\t%hs", static_cast< int >( SP_PENDING ), mate->GetUsername() ), 0, 0 );
	}

	const sdNetFriendsList& invitedFriends = networkService->GetFriendsManager().GetInvitedFriendsList();
	for( int i = 0; i < invitedFriends.Num(); i++ ) {
		const sdNetFriend* mate = invitedFriends[ i ];
		sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/invited'><material = 'friends/invited'>%i\t%hs", static_cast< int >( SP_INVITED ), mate->GetUsername() ), 0, 0 );
	}

	// Debug
	if( g_debugPlayerList.GetInteger() ) {
		int i;
		for( i = 0; i < 5 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/online'><material = 'friends/online'>%i\tFriend %i", static_cast< int >( SP_ONLINE ), i ), 0, 0 );
		}
		for( ; i < 10 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/newmessage'><material = 'friends/newmessage'>%i\tFriend %i", static_cast< int >( SP_NEW_MESSAGE ), i ), 0, 0 );
		}
		for( ; i < 15 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/blocked'><material = 'friends/blocked'>%i\tFriend %i", static_cast< int >( SP_BLOCKED ), i ), 0, 0 );
		}
		for( ; i < 20 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/onserver'><material = 'friends/onserver'>%i\tFriend %i", static_cast< int >( SP_ONSERVER ), i ), 0, 0 );
		}
	}
}

/*
============
sdNetProperties::CreateTeamList
============
*/
void sdNetProperties::CreateTeamList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdNetUser* activeUser = networkService->GetActiveUser();
	if( activeUser == NULL ) {
		return;
	}

	sdScopedLock< true > teamLock( networkService->GetTeamManager().GetLock() );
	sdScopedLock< true > friendLock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& blockedFriends = networkService->GetFriendsManager().GetBlockedList();
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();

	for( int i = 0; i < members.Num(); i++ ) {
		const sdNetTeamMember* mate = members[ i ];

		// skip yourself
		if( idStr::Icmp( mate->GetUsername(), activeUser->GetUsername() ) == 0 ) {
			continue;
		}

		statusPriority_e	status		= SP_OFFLINE;
		teamLevel_e			teamLevel	= TL_USER;

		const char* string	= "guis/mainmenu/friends/offline";
		const char* material = "friends/offline";

		if( mate->GetState() == sdNetTeamMember::OS_ONLINE ) {
			sdNetClientId id;
			mate->GetNetClientId( id );
			const idDict* profile = networkService->GetProfileProperties( id );
			const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );


			if( idStr::Cmp( server, "0.0.0.0:0" ) == 0 ) {
				material	= "friends/online";
				string		= "guis/mainmenu/friends/online";
				status = SP_ONLINE;
			} else {
				material	= "friends/onserver";
				string		= "guis/mainmenu/friends/onserver";
				status = SP_ONSERVER;
			}
		}

		if( networkService->GetFriendsManager().FindFriend( blockedFriends, mate->GetUsername() ) ) {
			string		= "guis/mainmenu/friends/blocked";
			material	= "friends/blocked";
			status		= SP_BLOCKED;
		}

		idListGranularityOne< sdNetMessage* > messages;
		mate->GetMessageQueue().GetMessagesOfType( messages, sdNetMessage::MT_IM );
		if( !messages.Empty() ) {
			string		= "guis/mainmenu/friends/newmessage";
			material	= "friends/newmessage";
			status		= SP_NEW_MESSAGE;
		} else {
			const sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if( message != NULL ) {
				switch( message->GetType() ) {
				case sdNetMessage::MT_SESSION_INVITE:
					string	= "guis/mainmenu/friends/serverinvite";
					material = "friends/newevent";
					status = SP_NEW_MESSAGE;
					break;
				case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_ADMIN:		// FALL THROUGH
				case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_OWNER:
					string	= "guis/mainmenu/friends/promotion";
					material = "friends/newevent";
					status = SP_NEW_MESSAGE;
					break;
				}
			}
		}

		const char* teamStatusMaterial = "teams/member";
		switch( mate->GetMemberStatus() ) {
			case sdNetTeamMember::MS_ADMIN:
				teamStatusMaterial = "teams/admin";
				teamLevel = TL_ADMIN;
				break;
			case sdNetTeamMember::MS_OWNER:
				teamStatusMaterial = "teams/owner";
				teamLevel = TL_OWNER;
				break;
		}
		sdUIList::InsertItem( list, va( L"<loc = '%hs'><material = '%hs'>%i\t%<material = '%hs'>%i\t%hs\t0",
										string, material, static_cast< int >( status ),
										teamStatusMaterial, static_cast< int >( teamLevel ),
										mate->GetUsername() ), 0, 0 );
	}

	const sdNetTeamMemberList& pending = networkService->GetTeamManager().GetPendingMemberList();
	for( int i = 0; i < pending.Num(); i++ ) {
		const sdNetTeamMember* mate = pending[ i ];
		sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/clan/invited'><material = 'friends/invited'>%i\t<material = 'nodraw'>%i\t%hs\t1",
										static_cast< int >( SP_PENDING ),
										static_cast< int >( TL_PENDING ),
										mate->GetUsername() ), 0, 0 );
	}
	// Debug
	if( g_debugPlayerList.GetInteger() ) {
		int i;
		for( i = 0; i < 5 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<loc = 'guis/mainmenu/friends/online'><material = 'friends/online'>%i\t<material = 'teams/member'>%i\tFriend %i",
											static_cast< int >( SP_ONLINE ),
											static_cast< int >( TL_USER ),
											i ), 0, 0 );
		}
		for( ; i < 10 && i < g_debugPlayerList.GetInteger(); i++ ) {
			sdUIList::InsertItem( list, va( L"<material = 'friends/newmessage'>%i\t<material = 'teams/admin'>%i\tFriend %i",
				static_cast< int >( SP_NEW_MESSAGE ),
				static_cast< int >( TL_ADMIN ),
				i ), 0, 0 );
		}
		sdUIList::InsertItem( list, va( L"<material = 'friends/online'>%i\t<material = 'teams/owner'>%i\tFriend %i",
			static_cast< int >( SP_NEW_MESSAGE ),
			static_cast< int >( TL_OWNER ),
			i ), 0, 0 );
	}
}

/*
============
sdNetProperties::CreateTeamInvitesList
============
*/
void sdNetProperties::CreateTeamInvitesList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetPendingInvitesList();
	for( int i = 0; i < members.Num(); i++ ) {
		const sdNetTeamMember* mate = members[ i ];
		const sdNetMessage* message = mate->GetMessageQueue().GetMessages();

		if( message == NULL || message->GetType() != sdNetMessage::MT_MEMBERSHIP_PROPOSAL ) {
			continue;
		}
		const sdNetTeamInvite& teamInvite = reinterpret_cast< const sdNetTeamInvite& >( *message->GetData() );
		sdUIList::InsertItem( list, va( L"%hs\t%hs", teamInvite.text, mate->GetUsername() ), 0, 0 );
	}
}

/*
============
sdNetProperties::CreateAchievementsList
============
*/
void sdNetProperties::CreateAchievementsList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idStr playerCategory;
	list->GetUI()->PopScriptVar( playerCategory );

	gameLocal.rankInfo.CreateData( sdGlobalStatsTracker::GetInstance().GetLocalStatsHash(), sdGlobalStatsTracker::GetInstance().GetLocalStats(), gameLocal.rankScratchInfo );

	idWStr matStr;
	const idList< sdPersistentRankInfo::sdBadge >& badges = gameLocal.rankInfo.GetBadges();
	for( int i = 0; i < badges.Num(); i++ ) {
		const sdPersistentRankInfo::sdBadge& badge = badges[ i ];
		if( badge.category.Icmp( playerCategory.c_str() ) != 0 ) {
			continue;
		}

		const wchar_t* complete = gameLocal.rankScratchInfo.badges[ i ].complete ? L"" : L"desat_";
		matStr = va( L"guis/assets/icons/achieve_%ls%hs_rank%i", complete, badge.category.c_str(), badge.level );
		const wchar_t* matName = va( L"<material = '%ls'>%ls\t%hs\t%i", matStr.c_str(), matStr.c_str(), badge.category.c_str(), badge.level );
		sdUIList::InsertItem( list, matName, -1, 0 );
	}
}

/*
============
sdNetProperties::CreateAchievementTasksList
============
*/
void sdNetProperties::CreateAchievementTasksList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idStr category;
	list->GetUI()->PopScriptVar( category );

	int level;
	list->GetUI()->PopScriptVar( level );

	gameLocal.rankInfo.CreateData( sdGlobalStatsTracker::GetInstance().GetLocalStatsHash(), sdGlobalStatsTracker::GetInstance().GetLocalStats(), gameLocal.rankScratchInfo );

	const idList< sdPersistentRankInfo::sdBadge >& badges = gameLocal.rankInfo.GetBadges();
	for( int i = 0; i < badges.Num(); i++ ) {
		const sdPersistentRankInfo::sdBadge& badge = badges[ i ];
		if( badge.category.Icmp( category.c_str() ) != 0 || badge.level != level ) {
			continue;
		}
		const sdPersistentRankInfo::sdRankInstance::sdBadge& dataBadge = gameLocal.rankScratchInfo.badges[ i ];

		for( int j = 0; j < badge.tasks.Num(); j++ ) {
			float value = idMath::ClampFloat( 0.0f, dataBadge.taskValues[ j ].max, idMath::Floor( dataBadge.taskValues[ j ].value ) );
			float percent = dataBadge.taskValues[ j ].value / dataBadge.taskValues[ j ].max;
			sdUIList::InsertItem( list, va( L"<loc = '%hs'>\t<flags customdraw>%f\t%hs\t%0.f/%0.f", badge.tasks[ j ].text.c_str(), percent, dataBadge.complete ? "1" : "0", value, dataBadge.taskValues[ j ].max ), -1, 0 );
		}
	}
}

/*
============
sdNetProperties::CreateMessageHistoryList
============
*/
void sdNetProperties::CreateMessageHistoryList( sdUIList* list ) {
	assert( networkService->GetActiveUser() != NULL );

	sdUIList::ClearItems( list );

	int iSource;
	list->GetUI()->PopScriptVar( iSource );

	idStr name;
	list->GetUI()->PopScriptVar( name );

	messageHistorySource_e source;
	if( !sdIntToContinuousEnum< messageHistorySource_e >( iSource, MHS_MIN, MHS_MAX, source ) ) {
		gameLocal.Warning( "CreateMessageHistoryList: unknown source '%i'\n", iSource );
		return;
	}

	idStr rawUserName;
	sdNetMessageHistory* history = GetMessageHistory( source, name, rawUserName );
	if( history != NULL ) {
		idStr filename;
		GenerateMessageHistoryFileName( networkService->GetActiveUser()->GetRawUsername(), rawUserName.c_str(), filename );

		if( history->Load( filename ) ) {
			idWStr timeFormat;
			sysTime_t time;

			for( int i = 0; i < history->GetNumEntries(); i++ ) {
				const messageHistoryEntry_t& entry = history->GetEntry( i );
				sys->SecondsToTime( entry.timeStamp, time, true );
				FormatTimeStamp( time, timeFormat );

				sdUIList::InsertItem( list, va( L"^3%ls^0\n%ls", timeFormat.c_str(), entry.message.c_str() ), -1, 0 );
			}
		}
	}
}

/*
============
sdNetProperties::GenerateMessageHistoryFileName
============
*/
void sdNetProperties::GenerateMessageHistoryFileName( const char* currentUser, const char* friendName, idStr& path ) {
	path = va( "%s/logs/%s.messageStore", currentUser, friendName );
}

/*
============
sdNetProperties::GetMessageHistory
============
*/
sdNetMessageHistory* sdNetProperties::GetMessageHistory( messageHistorySource_e source, const char* username, idStr& friendRawUserName ) {
	friendRawUserName = "";

	sdNetMessageHistory* history = NULL;
	if( source == MHS_FRIEND ) {
		const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
		sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, username );
		if( mate != NULL ) {
			history = &mate->GetHistory();
			sdNetUser::MakeRawUsername( mate->GetUsername(), friendRawUserName );
		}
	} else if( source == MHS_TEAM ) {
		const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
		sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, username );
		if( mate != NULL ) {
			history = &mate->GetHistory();
			sdNetUser::MakeRawUsername( mate->GetUsername(), friendRawUserName );
		}
	}

	return history;
}

#endif /* !SD_DEMO_BUILD */

/*
============
sdNetProperties::FormatTimeStamp
============
*/
void sdNetProperties::FormatTimeStamp( const sysTime_t& time, idWStr& str ) {
	str = L"";

	// ensure we always return to the default locale
	try {
		sys->SetSystemLocale();

		sysTime_t now;
		sys->RealTime( &now );

		if( now.tm_yday == time.tm_yday && now.tm_year == time.tm_year ) {
			// Today
			idWStrList args( 1 );
			args.Append( va( L"%hs", sys->TimeToSystemStr( time ) ) );
			str = common->LocalizeText( "guis/today", args ).c_str();
		} else {
			str = va( L"%hs", sys->TimeAndDateToSystemStr( time ) );
		}

		sys->SetDefaultLocale();
	} catch( ... ) {
		sys->SetDefaultLocale();
		throw;
	}
}

/*
============
sdNetProperties::CreateRetrievedUserNameList
============
*/

#if !defined( SD_DEMO_BUILD )
void sdNetProperties::CreateRetrievedUserNameList( sdUIList* list ) {
	gameLocal.GetSDNet().CreateRetrievedUserNameList( list );
}
#endif /* !SD_DEMO_BUILD */
