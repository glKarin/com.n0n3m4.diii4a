// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PlayerProperties.h"
#include "Player.h"
#include "rules/GameRules.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleWeapon.h"
#include "vehicles/VehicleControl.h"
#include "vehicles/VehicleView.h"
#include "Weapon.h"
#include "roles/Tasks.h"
#include "roles/FireTeams.h"

#include "guis/UserInterfaceLocal.h"
#include "guis/UserInterfaceTypes.h"
#include "guis/UIIconNotification.h"
#include "guis/UserInterfaceManager.h"

#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"

#include "../decllib/declTypeHolder.h"
#include "../decllib/declLocStr.h"
#include "demos/DemoManager.h"

#include "rules/VoteManager.h"
#include "misc/WorldToScreen.h"
#include "Waypoints/LocationMarker.h"

typedef sdUIIconNotification::iconHandle_t iconHandle_t;


/*
===============================================================================

sdPlayerProperties

===============================================================================
*/

/*
================
sdPlayerProperties::sdPlayerProperties
================
*/
sdPlayerProperties::sdPlayerProperties( void ) {
	localPlayer			= NULL;	
	commandMapExpanding	= false;

	killedPlayerMessage			= NULL;
	killedByPlayerMessage		= NULL;
	killedPlayerTeamMessage		= NULL;
	killedByPlayerTeamMessage	= NULL;
}

/*
============
sdPlayerProperties::InitGUIs
============
*/
void sdPlayerProperties::InitGUIs( void ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return;
	}

	playerHud			.Reset( new sdPlayerHud() );
	playerHud->InitGui( "hud", false  );

	limboMenu			.Reset( new sdLimboMenu() );
	limboMenu->InitGui( "limbo", false );

	deployMenu			.Reset( new sdDeployMenu() );

	quickChatMenu		.Reset( new sdQuickChatMenu() );
	quickChatMenu->InitGui( "quickchat", false );

	fireTeamMenu		.Reset( new sdFireTeamMenu() );
	fireTeamMenu->InitGui( "fireteam", false );

	contextMenu		.Reset( new sdQuickChatMenu() );
	contextMenu->InitGui( "context", false );

	chatMenu			.Reset( new sdChatMenu() );
	chatMenu->InitGui( "chat", false );

	takeViewNoteMenu	.Reset( new sdTakeViewNoteMenu() );
	takeViewNoteMenu->InitGui( "takeViewNote", false );

	weaponSelectionMenu .Reset( new sdWeaponSelectionMenu() );
	weaponSelectionMenu->InitGui( "weaponSelection", false );

	postProcess			.Reset( new sdPostProcess() );
	postProcess->InitGui( "guis/postprocess" );

	gameLocal.FreeUserInterface( scoreBoard );
	scoreBoard			= gameLocal.LoadUserInterface( "scoreboard", false, false );
	
	InitGUIStates();
}

/*
============
sdPlayerProperties::InitGUIStates
============
*/
void sdPlayerProperties::InitGUIStates() {
	if ( !gameLocal.DoClientSideStuff() ) {
		return;
	}
	// set everything up sensibly
	deployMenu->Enable( false );
	deployMenu->SetSort( -1 );

	fireTeamMenu->SetSort( -2 );

	quickChatMenu->SetSort( -3 );
	contextMenu->SetSort( -3 );
	chatMenu->SetSort( -2 );

	limboMenu->SetSort( -100 );

	weaponSelectionMenu->SetSort( 0 );
	weaponSelectionMenu->Enable( true );

	playerHud->SetSort( 0 );
	playerHud->Enable( true );

	postProcess->Enable( true );
}

/*
============
sdPlayerProperties::Init
============
*/
SD_UI_PUSH_CLASS_TAG( sdPlayerProperties )
void sdPlayerProperties::Init( void ) {
	properties.RegisterProperty( "health",					health );
	properties.RegisterProperty( "maxHealth",				maxHealth );
	properties.RegisterProperty( "speed",					speed );
	properties.RegisterProperty( "name",					name );
	properties.RegisterProperty( "role",					role );
	properties.RegisterProperty( "roleTitle",				roleTitle );
	properties.RegisterProperty( "xp",						xp );
	properties.RegisterProperty( "rank",					rank );
	properties.RegisterProperty( "rankMaterial",			rankMaterial );
	properties.RegisterProperty( "yaw",						yaw );
	properties.RegisterProperty( "vehicleYaw",				vehicleYaw );
	properties.RegisterProperty( "pitch",					pitch );
	properties.RegisterProperty( "spreadFraction",			spreadFraction );
	properties.RegisterProperty( "dead",					isDead );
	properties.RegisterProperty( "viewDead",				isViewDead );
	properties.RegisterProperty( "limbo",					isInLimbo );
	properties.RegisterProperty( "spawning",				isSpawning );
	properties.RegisterProperty( "spectateClient",			spectateClient );
	properties.RegisterProperty( "location",				location );
	properties.RegisterProperty( "serverInfoChanged",		serverInfoChanged );
	properties.RegisterProperty( "voiceSendMode",			voiceSendMode );

	properties.RegisterProperty( "nextVoteTime",			nextVoteTime );
	properties.RegisterProperty( "gdfCriticalClass",		gdfCriticalClass );
	properties.RegisterProperty( "stroggCriticalClass",		stroggCriticalClass );

	properties.RegisterProperty( "inProxy",					inProxy );
	properties.RegisterProperty( "vehicleValid",			vehicleValid );
	properties.RegisterProperty( "vehicleHealth",			vehicleHealth );
	properties.RegisterProperty( "vehiclePosition",			vehiclePosition );
	properties.RegisterProperty( "vehicleDestructTime",		vehicleDestructTime );
	properties.RegisterProperty( "vehicleWrongDirection",	vehicleWrongDirection );
	properties.RegisterProperty( "vehicleKickDistance",		vehicleKickDistance );
	properties.RegisterProperty( "vehicleEMPed",			vehicleEMPed );
	properties.RegisterProperty( "vehicleWeaponEMPed",		vehicleWeaponEMPed );
	properties.RegisterProperty( "vehicleSiegeMode",		vehicleSiegeMode );
	properties.RegisterProperty( "vehicleThirdPerson",		vehicleThirdPerson );

	properties.RegisterProperty( "weaponName",				weaponName );
	properties.RegisterProperty( "weaponLookupName",		weaponLookupName );
	properties.RegisterProperty( "weaponNeedsAmmo",			weaponNeedsAmmo );
	properties.RegisterProperty( "weaponClip",				weaponClip );
	properties.RegisterProperty( "weaponTotalClip",			weaponTotalClip );
	properties.RegisterProperty( "weaponShotsPerClip",		weaponShotsPerClip );
	properties.RegisterProperty( "weaponShotsAvailable",	weaponShotsAvailable );
	properties.RegisterProperty( "weaponSlot",				weaponSlot );

	properties.RegisterProperty( "showCrosshair",			showCrosshair );
	properties.RegisterProperty( "hideDecoyInfo",			hideDecoyInfo );
	properties.RegisterProperty( "showTargetingInfo",		showTargetingInfo );
	properties.RegisterProperty( "crosshairDistance",		crosshairDistance );

	properties.RegisterProperty( "lastDamageIntensity",		lastDamageIntensity );
	properties.RegisterProperty( "lastDamageTime",			lastDamageTime );

	properties.RegisterProperty( "matchTime",				matchTime );
	properties.RegisterProperty( "warmup",					warmup );
	properties.RegisterProperty( "matchStatus",				matchStatus );
	properties.RegisterProperty( "matchType",				matchType );

	properties.RegisterProperty( "toolTipText",				toolTipText );
	properties.RegisterProperty( "toolTipMaterial",			toolTipMaterial );
	properties.RegisterProperty( "toolTipLocation",			toolTipLocation );
	properties.RegisterProperty( "toolTipIsPriority",		toolTipIsPriority );

	properties.RegisterProperty( "position",				position );

	properties.RegisterProperty( "spectating",				spectating );
	properties.RegisterProperty( "spectator",				spectator );
	properties.RegisterProperty( "localView",				localView );

	properties.RegisterProperty( "voiceSending",			voiceSending );
	properties.RegisterProperty( "voiceReceiving",			voiceReceiving );

	properties.RegisterProperty( "isChatting",				isChatting );
	properties.RegisterProperty( "isLagged",				isLagged );

	properties.RegisterProperty( "fireTeamName",			fireTeamName );
	properties.RegisterProperty( "fireTeamActive",			fireTeamActive );
	properties.RegisterProperty( "fireTeamLeader",			fireTeamLeader );
	properties.RegisterProperty( "fireTeamShow",			fireTeamShow );

	properties.RegisterProperty( "taskAddedTime",			taskAddedTime );
	properties.RegisterProperty( "taskSelectedTime",		taskSelectedTime );
	properties.RegisterProperty( "taskCompletedTime",		taskCompletedTime );
	properties.RegisterProperty( "taskExpiredTime",			taskExpiredTime );
	properties.RegisterProperty( "taskStatus",				taskStatus );
	properties.RegisterProperty( "taskIndex",				taskIndex );
	properties.RegisterProperty( "hasTask",					hasTask );

	properties.RegisterProperty( "teamName",				teamName );
	properties.RegisterProperty( "teamNameView",			teamNameView );

	properties.RegisterProperty( "spawnSelected",			spawnSelected );

	properties.RegisterProperty( "winningTeam",				winningTeam );
	properties.RegisterProperty( "winningMusic",			winningMusic );
	properties.RegisterProperty( "winningTeamTitle",		winningTeamTitle );
	properties.RegisterProperty( "winningTeamString",		winningTeamString );
	properties.RegisterProperty( "winningTeamReason",		winningTeamReason );
	properties.RegisterProperty( "endGame",					endGame );
	properties.RegisterProperty( "endGameCamera",			endGameCamera );
	properties.RegisterProperty( "nextGameStateTime",		nextGameStateTime );

	properties.RegisterProperty( "targetingCenter",			targetingCenter );
	properties.RegisterProperty( "targetingColor",			targetingColor );
	properties.RegisterProperty( "targetingPercent",		targetingPercent );

	properties.RegisterProperty( "deploymentActive",		deploymentActive );
	properties.RegisterProperty( "deployRotation",			deployRotation );
	properties.RegisterProperty( "deployPosition",			deployPosition );
	properties.RegisterProperty( "deployIsRotating",		deployIsRotating );

	properties.RegisterProperty( "voteActive",				voteActive );
	properties.RegisterProperty( "votingAllowed",			votingAllowed );
	properties.RegisterProperty( "voteText",				voteText );
	properties.RegisterProperty( "voteYesCount",			voteYesCount );
	properties.RegisterProperty( "voteNoCount",				voteNoCount );

	properties.RegisterProperty( "hitFeedback",				hitFeedback );
	properties.RegisterProperty( "lastHitFeedbackType",		lastHitFeedbackType );

	properties.RegisterProperty( "commandMapState",			commandMapState );
	properties.RegisterProperty( "lagOMeter",				lagOMeter );

//	properties.RegisterProperty( "mapLoadPercent",			mapLoadPercent );
	properties.RegisterProperty( "taskInfo",				taskInfo );	

	properties.RegisterProperty( "isReady",					isReady );	
	properties.RegisterProperty( "needsReady",				needsReady );

	properties.RegisterProperty( "lastVoiceSender",			lastVoiceSender );

	properties.RegisterProperty( "lastKillMessage",			lastKillMessage );
	properties.RegisterProperty( "lastKillMessageTime",		lastKillMessageTime );

	properties.RegisterProperty( "scoreboardActive",		scoreboardActive );

	properties.RegisterProperty( "gameTime",				gameTime );
	properties.RegisterProperty( "showFireTeam",			showFireTeam );
	properties.RegisterProperty( "isPaused",				isPaused );
	properties.RegisterProperty( "isSinglePlayer",			singlePlayerGame );
	properties.RegisterProperty( "unpauseKeyString",		unpauseKeyString );

	properties.RegisterProperty( "serverIsRepeater",		serverIsRepeater );
	properties.RegisterProperty( "isServer",				isServer );
	properties.RegisterProperty( "inLetterBox",				inLetterBox );

	killedPlayerMessage			= declHolder.declLocStrType.LocalFind( "game/obit/local/killed" );
	killedByPlayerMessage		= declHolder.declLocStrType.LocalFind( "game/obit/local/killedby" );
	killedPlayerTeamMessage		= declHolder.declLocStrType.LocalFind( "game/obit/local/killed_team" );
	killedByPlayerTeamMessage	= declHolder.declLocStrType.LocalFind( "game/obit/local/killedby_team" );

	currentMission				= declHolder.declLocStrType.LocalFind( "guis/game/scoreboard/currentmission" );

	UpdateTeam( NULL ); // Make sure we have a sensible default

	int numProficiency = gameLocal.declProficiencyTypeType.Num();

	proficiency.SetNum( numProficiency );
	proficiencyPercent.SetNum( numProficiency );

	for( int i = 0; i < numProficiency; i++ ) {
		properties.RegisterProperty( va( "proficiency%i", i ), proficiency[ i ] );
		properties.RegisterProperty( va( "proficiencyPercent%i", i ), proficiencyPercent[ i ] );
	}

	stroyentType = idWeapon::GetAmmoType( "stroyent" );

	clipIndex = 0;

	SD_UI_PUSH_GROUP_TAG( "Team Allegiance" )

	SD_UI_ENUM_TAG( TA_FRIEND, "Friendly." )
	sdDeclGUI::AddDefine( va( "TA_FRIEND %i", TA_FRIEND ) );

	SD_UI_ENUM_TAG( TA_ENEMY, "Enemy." )
	sdDeclGUI::AddDefine( va( "TA_ENEMY %i", TA_ENEMY ) );
	
	SD_UI_ENUM_TAG( TA_NEUTRAL, "Neutral." )
	sdDeclGUI::AddDefine( va( "TA_NEUTRAL %i", TA_NEUTRAL ) );

	SD_UI_POP_GROUP_TAG

	uiManager->RegisterIconEnumerationCallback( "playerUpgrades", UpdatePlayerUpgradeIcons );
	uiManager->RegisterIconEnumerationCallback( "playerAllUpgrades", PlayerUpgradeIcons );

	idPlayer::SetupCommandMapZoom();

	voipChatTimes.SetNum( MAX_ASYNC_CLIENTS );

	activateEndGame = 0;
	serverInfoChanged = 0.0f;
	showFireTeam = false;
	isPaused = false;

	voiceSendMode = VO_NUM_MODES + 1;
}
SD_UI_POP_CLASS_TAG

/*
================
sdPlayerProperties::~sdPlayerProperties
================
*/
sdPlayerProperties::~sdPlayerProperties( void ) {
}

/*
================
sdPlayerProperties::ShutdownGUIs
================
*/
void sdPlayerProperties::ShutdownGUIs( void ) {
	limboMenu.Reset();
	deployMenu.Reset();
	quickChatMenu.Reset();
	contextMenu.Reset();
	fireTeamMenu.Reset();
	chatMenu.Reset();
	takeViewNoteMenu.Reset();
	weaponSelectionMenu.Reset();	
	playerHud.Reset();
	postProcess.Reset();

	gameLocal.FreeUserInterface( scoreBoard );
	scoreBoard.Release();
}

/*
================
sdPlayerProperties::Shutdown
================
*/
void sdPlayerProperties::Shutdown( void ) {
	properties.Clear();
}

/*
============
sdPlayerProperties::GetProperty
============
*/
sdProperties::sdProperty* sdPlayerProperties::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && prop->GetValueType() != type && type != sdProperties::PT_INVALID ) {
		gameLocal.Error( "sdPlayerProperties::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdPlayerProperties::UpdateTeam
================
*/
void sdPlayerProperties::UpdateTeam( sdTeamInfo* team ) {
	teamName = team ? team->GetLookupName() : "spectating";
	
	if ( teamNameView.GetValue().Length() <= 0 ) {
		teamNameView = teamName;
	}
}

/*
================
sdPlayerProperties::Update
================
*/ 
void sdPlayerProperties::PacifierUpdate( void ) {
//	mapLoadPercent = static_cast< float >( fileSystem->GetReadCount() ) / gameLocal.GetBytesNeededForMapLoad();
}


/*
================
sdPlayerProperties::Update
================
*/ 
void sdPlayerProperties::Update( idPlayer* player ) {
	using namespace sdProperties;

	gameTime						= gameLocal.time;
	serverIsRepeater				= gameLocal.serverIsRepeater;

	localView						= localPlayer == player ? 1.0f : 0.0f;

	if ( localPlayer != NULL ) {
		assert( player != NULL );

		spectator					= localPlayer->IsSpectator() ? 1.0f : 0.0f;
		spectating					= localPlayer->IsSpectating() ? 1.0f : 0.0f;

		isDead						= localPlayer->IsDead();
		isInLimbo					= localPlayer->IsInLimbo();
		isSpawning					= localPlayer->WantRespawn();		
		isReady						= localPlayer->IsReady();
		needsReady					= gameLocal.rules->IsWarmup() && ( !gameLocal.serverInfoData.adminStart && !gameLocal.rules->IsCountDown() );
		isLagged					= localPlayer->GetIsLagged() ? 1.0f : 0.0f;

		float distance = localPlayer->GetCrosshairInfoDirect().GetDistance();
		crosshairDistance			= distance < localPlayer->targetIdRangeNormal ? va( "%.2fm",  InchesToMetres( distance ) ) : "-1";

		name						= localPlayer->userInfo.name;
		singlePlayerGame			= gameLocal.isServer ? 1.0f : 0.0f;

		toolTipIsPriority			= localPlayer->IsSinglePlayerToolTipPlaying();
	} else {
		spectator					= 1.f;
		spectating					= 1.f;

		isDead						= 0.f;

		isInLimbo					= 0.f;
		isSpawning					= 0.f;
		isReady						= 0.f;
		needsReady					= 0.f;

		crosshairDistance			= "";
		isLagged					= 0.0f;

		name						= "";

		singlePlayerGame			= 0.0f;
		toolTipIsPriority			= 0.0f;

		UpdateTeam( NULL );
	}


	const char* newViewTeamName = teamName.GetValue().c_str();
	if ( player != NULL ) {
		sdTeamInfo* spectateTeam = player->GetGameTeam();
		if ( spectateTeam != NULL ) {
			newViewTeamName = spectateTeam->GetLookupName();
		}
		isViewDead					= player->IsDead() ? 1.f : 0.f;
	} else {
		isViewDead					= 0.f;
	}

	teamNameView					= newViewTeamName;

	isPaused = gameLocal.isPaused;
	isServer = gameLocal.isServer;

	sdUserInterfaceLocal* scoreboardUI = gameLocal.GetUserInterface( scoreBoard );
	if ( scoreboardUI != NULL ){
		scoreboardActive = scoreboardUI->IsActive() ? 1.0f : 0.0f;
	} else {
		scoreboardActive = 0.0f;
	}

	if ( player != NULL ) {
		if ( g_hitBeep.GetInteger() > 0 && g_hitBeep.GetInteger() != 2 ) {
			hitFeedback = player->NewDamageDealt();
			player->ClearDamageDealt();
			lastHitFeedbackType = player->GetLastDamageDealtType();
		}

		const idPlayer::damageEvent_t& event = player->GetLastDamageEvent();
		lastDamageTime = event.hitTime;
		lastDamageIntensity = event.hitDamage;
	}

	lagOMeter		= ( net_clientLagOMeter.GetBool() && gameLocal.isClient ) ? 1.0f : 0.0f;

	commandMapState	= GetCommandMapExpanding() ? 1.0f : 0.0f;

	const sdDeclPlayerClass* playerClass = NULL;
	sdInventory* inv = NULL;

	if ( player != NULL ) {
		inv				= &player->GetInventory();
		playerClass		= inv->GetClass();

		// player
		spectateClient	= player->userInfo.name;
		health			= player->GetHealth();
		maxHealth		= player->GetMaxHealth();
		
		role			= ( playerClass != NULL ) ? playerClass->GetName() : "spec";
		roleTitle		= ( playerClass != NULL ) ? playerClass->GetTitle()->Index() : declHolder.FindLocStr( "spec" )->Index();

		idWStr locationText;
		sdLocationMarker::GetLocationText( player->GetPhysics()->GetOrigin(), locationText );

		location		= locationText;

		if( player->GetProxyEntity() == NULL && player->weapon && ( localPlayer && !( localPlayer->IsInLimbo() || gameLocal.rules->IsEndGame() ) ) ) {
			float fraction = idMath::Sqrt( player->weapon->GetSpreadValueNormalized() );
			float min = player->weapon->GetCrosshairSpreadMin();
			float max = player->weapon->GetCrosshairSpreadMax();
			float scale = player->weapon->GetCrosshairSpreadScale();

			assert( !FLOAT_IS_NAN( fraction ) );

			fraction = min + fraction * scale;
			if ( fraction > max ) {
				fraction = max;
			}

			spreadFraction = fraction;
		} else {
			spreadFraction = 0.f;
		}
	} else {
		// player
		spectateClient	= "";
		health			= 0.f;
		maxHealth		= 0.f;
		
		role			= "spec";
		roleTitle		= declHolder.FindLocStr( "spec" )->Index();

		location		= L"";

		spreadFraction	= 0.f;
	}

	if( localPlayer != NULL ) {
		int time = localPlayer->GetNextCallVoteTime();
		nextVoteTime	= time >= gameLocal.time ? time - gameLocal.time : 0.0f;
	}

	warmup			= gameLocal.rules->IsWarmup() ? 1.0f : 0.0f;

	matchStatus		= gameLocal.rules->GetStatusText();
	matchType		= gameLocal.rules->GetTypeText()->GetText();

	matchTime		= gameLocal.rules->GetGameTime();

	idEntity* proxy = player != NULL ? player->GetProxyEntity() : NULL;
	sdTransport* vehicle = proxy ? proxy->Cast< sdTransport >() : NULL;	
	sdUsableInterface* iface = proxy ? proxy->GetUsableInterface() : NULL;

	// speed
	float localSpeed = 0.0f;
	float localEMPed = 0.0f;
	float localWeaponEMPed = 0.0f;
	float localSiegeMode = 0.0f;
	float localThirdPerson = 0.0f;
	if ( proxy != NULL ) {
		if ( vehicle != NULL ) {
			localEMPed = vehicle->GetRemainingEMP();
			if ( vehicle->GetActiveWeapon( player ) != NULL ) {
				localWeaponEMPed = vehicle->GetRemainingWeaponEMP();
			}

			const sdVehicleControlBase* control = vehicle->GetVehicleControl();
			if ( control != NULL ) {
				localSpeed = control->GetHudSpeed();
				localSiegeMode = control->InSiegeMode() == true ? 1.0f : 0.0f;
			}

			sdVehicleView& view = vehicle->GetViewForPlayer( player );
			localThirdPerson = view.GetViewParms().thirdPerson;

		} else {
			localSpeed = proxy->GetPhysics()->GetLinearVelocity().Length();
		}
	} else if ( player != NULL ) {
		localSpeed = player->GetPhysics()->GetLinearVelocity().Length();
	} else {
		localSpeed = 0.f;
	}

	speed					= idMath::FtoiFast( UPSToMPH( localSpeed ) );
	vehicleEMPed			= localEMPed;
	vehicleWeaponEMPed		= localWeaponEMPed;
	vehicleSiegeMode		= localSiegeMode;
	vehicleThirdPerson		= localThirdPerson;

	if ( player != NULL ) {
		// angles
		idAngles viewAngles;

		viewAngles = player->renderView.viewaxis.ToAngles();
		viewAngles.Normalize360();

		yaw			= viewAngles.yaw;
		pitch		= viewAngles.pitch;

		spawnSelected			= ( player->GetSpawnPoint() == NULL ) ? 0.f : 1.f;
	} else {
		// angles
		idAngles viewAngles = gameLocal.playerView.GetRepeaterViewInfo().viewAngles;
		viewAngles.Normalize360();

		yaw			= viewAngles.yaw;
		pitch		= viewAngles.pitch;

		spawnSelected			= 0.f;
	}

	idWeapon* pWeapon = player == NULL ? NULL : player->GetWeapon();

	bool showWeaponInfo = true;
	if ( vehicle != NULL ) {
		const sdVehiclePosition& pos = vehicle->GetPositionManager().PositionForPlayer( player );

		// e.g. the buffalo allows passengers to fire their weapons
		showWeaponInfo = pos.GetAllowWeapon();		
	} else if( proxy != NULL || inv == NULL ) {
		showWeaponInfo = false;
		vehicleYaw = 0.0f;
	} else {
		vehicleYaw = 0.0f;
	}

	if ( showWeaponInfo && ( pWeapon != NULL ) && ( playerClass != NULL ) ) {
		assert( inv != NULL );

		if ( pWeapon->IsClipBased() ) {
			weaponClip = player->GetClip( clipIndex );
			weaponShotsPerClip = pWeapon->GetClipSize( clipIndex );
		} else {
			ammoType_t type = pWeapon->GetAmmoType( clipIndex );
			if ( type != -1 ) {
				weaponClip			= inv->GetAmmo( type );
				weaponShotsPerClip	= inv->GetMaxAmmo( type );
			} else {
				weaponClip			= -1;
				weaponShotsPerClip	= -1;
			}
		}

		if ( weaponShotsPerClip == 0 ) {
			weaponShotsPerClip = -1;
		}

		weaponTotalClip			= player->AmmoForWeapon( clipIndex );
		weaponNeedsAmmo			= pWeapon->AmmoRequired( clipIndex ) != 0 ? 1.0f : 0.0f;
		weaponShotsAvailable	= pWeapon->ShotsAvailable( clipIndex );

		if ( inv->IsSwitchActive() ) {
			weaponSlot			= inv->GetSwitchingSlot() + 1;
		} else {
			weaponSlot			= inv->GetCurrentSlot() + 1;
		}
	}
	inProxy = proxy != NULL ? 1.0f : 0.0f;

	if ( proxy != NULL ) {
		int minDisplayHealth = proxy->GetMinDisplayHealth();
		vehicleHealth = ( proxy->GetHealth() - minDisplayHealth ) / static_cast< float >( proxy->GetMaxHealth() - minDisplayHealth );

		sdUsableInterface* iface = proxy->GetUsableInterface();

		vehicleDestructTime = iface->GetDestructionEndTime() != 0 ? MS2SEC( iface->GetDestructionEndTime() - gameLocal.time < 0 ? 0 : iface->GetDestructionEndTime() - gameLocal.time ) : 0.0f;
		vehicleWrongDirection = iface->GetDirectionWarning() ? 1.f : 0.f;
		vehicleKickDistance = iface->GetRouteKickDistance();

		showCrosshair = iface->GetShowCrosshair( player ) ? 1.f : 0.f;
		hideDecoyInfo = iface->GetHideDecoyInfo( player ) ? 1.f : 0.f;
		showTargetingInfo = iface->GetShowTargetingInfo( player ) ? 1.f : 0.f;
		vehicleYaw = proxy->GetRenderEntity() != NULL ? proxy->GetRenderEntity()->axis.ToAngles().yaw : 0.0f;
		inLetterBox = 0.0f;
	} else {
		vehicleDestructTime = 0.0f;
		vehicleWrongDirection = 0.0f;
		vehicleKickDistance = -1;
		if ( player != NULL ) {
			showCrosshair = player->IsBeingBriefed() ? 0.0f : 1.0f;
			inLetterBox = ( player->IsBeingBriefed() ) ? 1.0f : 0.0f;
		} else {
			showCrosshair = 1.0f;
			inLetterBox = 0.0f;
		}
		hideDecoyInfo = 0.0f;
		showTargetingInfo = 0.0f;
	}

	if ( inv == NULL ) {
		weaponNeedsAmmo		= 0.f;
		weaponName			= -1;
		weaponLookupName.Set( "" );
	} else if ( iface != NULL && !showWeaponInfo ) { // if showWeaponInfo is true then we need to prevent the values being overwritten
		weaponNeedsAmmo = 0.0f;
		const sdDeclLocStr* weaponTitle = iface->GetWeaponName( player );
		weaponName = weaponTitle != NULL ? weaponTitle->Index() : -1;
		weaponLookupName.Set( iface->GetWeaponLookupName( player ) );
	} else {
		weaponName = inv->GetWeaponTitle() != NULL ? inv->GetWeaponTitle()->Index() : -1;
 		weaponLookupName.Set( inv->GetWeaponName() );
	}

	sdTransport* viewVehicle = activeObjectView->Cast< sdTransport >();
	if ( viewVehicle && localPlayer ) {
		if ( proxyOverlay.Get() != NULL ) {			
			if( proxyOverlay->GetGuiHandle().IsValid() ) {
				viewVehicle->GetUsableInterface()->UpdateHud( localPlayer, proxyOverlay->GetGuiHandle() );
			}			
		}
	}

	// vehicles
	vehicleValid			= vehicle != NULL ? 1.0f : 0.0f;

	UpdateFireTeam( player );

	if ( player != NULL ) {

		// latitude/longitude/altitude
		position = InchesToMetres( player->GetPhysics()->GetOrigin() );

		// experience
		xp = idMath::Floor( player->GetProficiencyTable().GetXP() );

		const sdDeclRank* playerRank = player->GetProficiencyTable().GetRank();
		if ( playerRank != NULL ) {
			rank			= playerRank->GetTitle() != NULL ? playerRank->GetTitle()->Index() : -1;
			rankMaterial	= playerRank->GetMaterial();
		} else {
			rank = -1;
			rankMaterial = "";
		}
	} else {
		position = vec3_origin;
		xp = 0.f;
		rank = -1;
		rankMaterial = "";
	}

	// voice chat
	int currentTime = sys->Milliseconds();
	voiceSending	= ( currentTime - networkSystem->GetLastVoiceSentTime() ) < SEC2MS( 0.5f ) ? 1.0f : 0.0f;

	bool receivingNow = false;
	int bestDiffIndex = -1;
	int bestDiff = -1;
	for ( int i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		int diff = ( currentTime - networkSystem->GetLastVoiceReceivedTime( i ) );
		receivingNow |= diff < SEC2MS( 0.5f );

		voipChatTimes[ i ] = gameLocal.time - diff;

		if ( bestDiff == -1 || diff < bestDiff ) {
			bestDiffIndex = i;
			bestDiff = diff;
		}
	}
	voiceReceiving = receivingNow ? 1.f : 0.f;
	if ( bestDiffIndex != -1 ) {
		const idPlayer* client = gameLocal.GetClient( bestDiffIndex );
		if ( client != NULL ) {
			lastVoiceSender = client->GetUserInfo().name;
		}
	}

	switch( networkSystem->GetVoiceMode() ) {
		case VO_GLOBAL:
			voiceSendMode = sdUserInterfaceLocal::VOIPC_GLOBAL;
			break;
		case VO_TEAM:
			voiceSendMode = sdUserInterfaceLocal::VOIPC_TEAM;
			break;
		case VO_FIRETEAM:
			voiceSendMode = sdUserInterfaceLocal::VOIPC_FIRETEAM;
			break;
		default:
			voiceSendMode = sdUserInterfaceLocal::VOIPC_DISABLE;
			break;
	}

	isChatting		= 0.0f;	

	// Campaign/Match status

	if( gameLocal.rules->IsEndGame() ) {
		sdTeamInfo* winner	= gameLocal.rules->GetWinningTeam();
		const sdDeclLocStr* reason = gameLocal.rules->GetWinningReason();
		winningTeam			= ( winner != NULL ) ? winner->GetLookupName() : "";
		winningTeamTitle	= ( winner != NULL && winner->GetTitle() != NULL ) ? winner->GetTitle()->Index() : -1;
		winningTeamString	= ( winner != NULL && winner->GetWinString() != NULL ) ? winner->GetWinString()->Index() : -1;
		winningTeamReason	= ( reason != NULL ) ? reason->Index() : -1;
		winningMusic		= ( winner != NULL ) ? winner->GetWinMusic() : "";
		if( activateEndGame == 0 ) {
			activateEndGame = gameLocal.time + SEC2MS( gameLocal.GetMapInfo().GetData().GetFloat( va( "%s_endgame_pause", winningTeam.GetValue().c_str() ), "3.0" ) );
		}
	} else {
		winningTeam			= "";
		winningTeamTitle	= -1;
		winningTeamString	= -1;
		winningMusic		= "";
		activateEndGame = 0;
	}
	if( gameLocal.time >= activateEndGame && gameLocal.rules->IsEndGame() ) {
		endGame	= 1.0f;
	} else {
		endGame = 0.0f;
	}

	endGameCamera = gameLocal.rules->IsEndGame();

	nextGameStateTime	= gameLocal.rules->GetNextStateTime();

	// targeting display
	UpdateTargetingInfo( player );

	// deployment feedback
	deploymentActive = gameLocal.GetDeploymentRequest( player ) != NULL ? 1.0f : 0.0f;

	if( deployMenu.Get() ) {
		deployIsRotating = deployMenu->GetDeployMode() ? 1.0f : 0.0f;
		deployRotation = deployMenu->GetRotation();
		deployPosition = deployMenu->GetPosition();
	}

	votingAllowed = gameLocal.serverInfoData.votingDisabled == false;
	if ( sdPlayerVote* vote = sdVoteManager::GetInstance().GetActiveVote( player ) ) {
		voteActive		= 1.0f;
		voteText		= vote->GetText();
		voteYesCount	= vote->GetYesCount();
		voteNoCount		= vote->GetNoCount();
	} else {
		voteActive		= 0.0f;
		voteText		= L"";
		voteYesCount	= 0.0f;
		voteNoCount		= 0.0f;
	}

	// update proficiency statistics
	if ( playerClass ) {
		const sdProficiencyTable& table = player->GetProficiencyTable();
		for( int i = 0; i < playerClass->GetNumProficiencies(); i++ ) {
			const sdDeclPlayerClass::proficiencyCategory_t& category = playerClass->GetProficiency( i );

			int profIndex = category.index;
			proficiency[ i ] = static_cast< float >( table.GetLevel( profIndex ) );

			const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType.LocalFindByIndex( profIndex );
			float currentCost = 0.0f;			
			int maxLevel = Min( table.GetLevel( profIndex ) + 1, prof->GetNumLevels() );
			int levelIndex;
			for( levelIndex = 0; levelIndex < maxLevel; levelIndex++ ) {
				currentCost += static_cast< float >( prof->GetLevel( levelIndex ) );
			}

			float baseCost = 0.0f;
			for( levelIndex = 0; levelIndex < table.GetLevel( profIndex ); levelIndex++ ) {
				baseCost += static_cast< float >( prof->GetLevel( levelIndex ) );
			}

			if( currentCost > idMath::FLT_EPSILON ) {
				proficiencyPercent[ i ]	= ( table.GetPoints( profIndex ) - baseCost ) / ( currentCost - baseCost );
			} else {
				proficiencyPercent[ i ]	= 0.0f;
			}
		}
	}
}

/*
============
sdPlayerProperties::SetToolTipInfo
============
*/
void sdPlayerProperties::SetToolTipInfo( const wchar_t* text, int index, const idMaterial* icon ) {
	toolTipText = text;
	toolTipLocation = index;
	toolTipMaterial = icon ? icon->GetName() : "";
}



/*
============
FindItemWithData
============
*/
static iconHandle_t FindItemWithData( sdUIIconNotification* icons, int data ) {	
	iconHandle_t handle = icons->GetFirstItem();
	while( handle.IsValid() ) {
		int itemData = icons->GetItemDataInt( handle );
		if( data == itemData ) {
			break;
		}
		handle = icons->GetNextItem( handle );
	}
	return handle;
}

/*
============
sdPlayerProperties::UpdatePlayerUpgradeIcons
============
*/
void sdPlayerProperties::UpdatePlayerUpgradeIcons( sdUIIconNotification* icons ) {
	int desiredCategory;
	icons->GetUI()->PopScriptVar( desiredCategory );

	idStr playerClassName;
	icons->GetUI()->PopScriptVar( playerClassName );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		icons->Clear();
		return;
	}

	const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType.LocalFind( playerClassName, false );
	if ( pc == NULL ) {
		icons->Clear();
		return;
	}

	idStaticList< iconHandle_t, 32 > iconsToRemove;
	iconHandle_t start = icons->GetFirstItem();
	while( start.IsValid() ) {
		iconsToRemove.Append( start );
		start = icons->GetNextItem( start );
	}

	const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

	for ( int i = 0; i < pc->GetNumProficiencies(); i++ ) {
		const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( i );

		if( desiredCategory != -1 && category.index != desiredCategory ) {
			continue;
		}

		int profIndex = category.index;
		int playerLevel = idMath::Ftoi( table.GetLevel( profIndex ) );

		// add all icons from the current level and below
		for ( int level = 0; level < playerLevel && level < category.upgrades.Num(); level++ ) {
			const sdDeclPlayerClass::proficiencyUpgrade_t& upgrade = category.upgrades[ level ];
			iconHandle_t handle = FindItemWithData( icons, category.index + level );
			if( !handle.IsValid() ) {
				iconHandle_t newItem = icons->AddIcon( upgrade.materialInfo );
				icons->SetItemDataInt( newItem, category.index + level );
				icons->SetItemText( newItem, upgrade.text->GetText() );
			} else {
				verify( iconsToRemove.Remove( handle ) );
			}
		}
	}

	for( int i = 0; i < iconsToRemove.Num(); i++ ) {
		icons->RemoveIcon( iconsToRemove[ i ] );
	}
}

/*
============
sdPlayerProperties::PlayerUpgradeIcons
============
*/
void sdPlayerProperties::PlayerUpgradeIcons( sdUIIconNotification* icons ) {
	int desiredCategory;
	icons->GetUI()->PopScriptVar( desiredCategory );

	idStr playerClassName;
	icons->GetUI()->PopScriptVar( playerClassName );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		icons->Clear();
		return;
	}

	const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType.LocalFind( playerClassName, false );
	if ( pc == NULL ) {
		icons->Clear();
		return;
	}

	const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

	for ( int i = pc->GetNumProficiencies() - 1; i >= 0 ; i-- ) {
		const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( i );

		if( desiredCategory != -1 && category.index != desiredCategory ) {
			continue;
		}

		int profIndex = category.index;

		// add all icons from the current level and below
		for ( int level = 0; level < category.upgrades.Num(); level++ ) {
			const sdDeclPlayerClass::proficiencyUpgrade_t& upgrade = category.upgrades[ level ];
			iconHandle_t newItem = icons->AddIcon( upgrade.materialInfo );
			icons->SetItemText( newItem, upgrade.text->GetText() );
		}
	}
}


/*
================
sdPlayerProperties::UpdateHudModules
================
*/
void sdPlayerProperties::UpdateHudModules( void ) {
	sdHudModule* module = GetActiveHudModule();
	if ( module ) {
		module->Activate();
		module->Update();
		sdUserInterfaceLocal* ui = module->GetGui();
		if ( ui && !ui->IsActive() ) {
			module->Enable( false );
		}
	}

	for ( module = GetPassiveHudModule(); module; module = module->GetNode().Next() ) {
		module->Update();
	}
}

/*
================
sdPlayerProperties::UpdateTargetingInfo
================
*/
void sdPlayerProperties::UpdateTargetingInfo( idPlayer* player ) {
	idEntity* targetEnt = NULL;

	if ( player != NULL ) {
		idEntity* proxy = player->GetProxyEntity();	
		targetEnt = player->targetEntity;

		if ( targetEnt ) {
			targetingColor = player->GetTargetLocked() ? colorRed : colorGreen;
		} else {
			targetEnt = player->targetEntityPrevious;
			if ( targetEnt ) {
				targetingColor = idVec4( colorGreen.x, colorGreen.y, colorGreen.z, 0.5f );
			}
		}
	}

	if ( targetEnt ) {
		idVec2 point;
		sdWorldToScreenConverter converter( gameLocal.playerView.GetCurrentView() );

		idPhysics* targetPhysics = targetEnt->GetPhysics();

		if ( targetEnt->GetSelectionBounds() != NULL && !targetEnt->GetSelectionBounds()->IsCleared() ) {
			converter.Transform( targetEnt->GetLastPushedOrigin() + ( targetEnt->GetSelectionBounds()->GetCenter() * targetEnt->GetLastPushedAxis() ), point );
		} else {
			converter.Transform( targetEnt->GetLastPushedOrigin() + ( targetPhysics->GetBounds().GetCenter() * targetEnt->GetLastPushedAxis() ), point );
		}
		targetingCenter = point;

		if( player->targetLockEndTime ) {	
			float f = 1.0f - static_cast< float >( player->targetLockEndTime - gameLocal.time ) / player->targetLockDuration;
			targetingPercent = idMath::ClampFloat( 0.0f, 1.0f, f );
		} else if( player->targetLockLastTime != 0 ) {
			float f = static_cast< float >( player->targetLockLastTime - gameLocal.time ) / 1000;
			targetingPercent = idMath::ClampFloat( 0.0f, 1.0f, f );
		} else {
			targetingPercent = 0.0f;
		}
	} else {
		targetingPercent = 0.0f;
	}
}

/*
================
sdPlayerProperties::UpdateFireTeam
================
*/
void sdPlayerProperties::UpdateFireTeam( idPlayer* player ) {	
	sdFireTeam* fireTeam = player != NULL ? gameLocal.rules->GetPlayerFireTeam( player->entityNumber ) : NULL;

	if ( fireTeam != NULL ) {
		fireTeamName	= va( L"%hs", fireTeam->GetName() );
		fireTeamActive	= 1.f;
		fireTeamLeader	= fireTeam->GetCommander() == player;
		fireTeamShow	= 1.f;
		return;
	}
	
	fireTeamActive	= 0.f;
	fireTeamLeader	= 0.f;
	fireTeamShow	= sdObjectiveManager::GetInstance().GetNumObjectives() > 0 ? 1.0f : 0.0f;
	if( fireTeamShow > 0.0f ) {
		fireTeamName	= currentMission->GetText();
	} else {
		fireTeamName	= L"";
	}
}

/*
================
sdPlayerProperties::GetProperty
================
*/
sdProperties::sdProperty* sdPlayerProperties::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdPlayerProperties::SetupVehiclePosition
============
*/
void sdPlayerProperties::SetupVehiclePosition( sdUsableInterface* interface, int positionID, sdUserInterfaceLocal* ui ) {
	positionMode_t mode = PM_EMPTY;

	idPlayer* other = interface->GetPlayerAtPosition( positionID );
	if ( other ) {
		if ( other == gameLocal.GetLocalViewPlayer() ) {
			mode = PM_SELF;
		} else {
			mode = PM_OTHER;
		}
	} else {
		mode = PM_EMPTY;
	}

	SetupVehiclePosition( interface, positionID, mode, ui );
}

/*
============
sdPlayerProperties::SetupVehiclePosition
============
*/
void sdPlayerProperties::SetupVehiclePosition( sdUsableInterface* interface, int positionID, positionMode_t mode, sdUserInterfaceLocal* ui ) {

	const char* eventName;
	switch ( mode ) {
		case PM_OTHER:
			eventName = va( "otherEnterPosition%i", positionID );
			break;
		case PM_SELF:
			eventName = va( "selfEnterPosition%i", positionID );
			vehiclePosition = interface->GetPositionTitle( positionID )->Index();
			break;
		case PM_EMPTY:
			eventName = va( "exitPosition%i", positionID );
			break;
	}

	ui->PostNamedEvent( eventName, true );
}

/*
============
sdPlayerProperties::OnEntityDestroyed
============
*/
void sdPlayerProperties::OnEntityDestroyed( idEntity* entity ) {
	if ( gameLocal.playerView.GetActiveViewProxy() == entity ) {
		gameLocal.playerView.SetActiveProxyView( NULL, NULL ); // make sure this gets cleared out
	}

	if ( entity == currentSpawnPoint ) {
		OnSetActiveSpawn( NULL );
		gameLocal.limboProperties.OnSetActiveSpawn( NULL );
	}
}

/*
============
sdPlayerProperties::OnNewTask
============
*/
void sdPlayerProperties::OnNewTask( sdPlayerTask* task ) {
	taskStatus = common->LocalizeText( "guis/game/new_tasks_available" );
	taskAddedTime = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );

	sdScriptHelper h1;
	h1.Push( task->GetScriptObject() );
	h1.Push( task->IsMission() );
	localPlayer->CallNonBlockingScriptEvent( localPlayer->GetScriptFunction( "OnNewTask" ), h1 );
}

/*
============
sdPlayerProperties::OnTaskSelected
============
*/
void sdPlayerProperties::OnTaskSelected( sdPlayerTask* task ) {
/*
	if( localPlayer == NULL ) {
		return;
	}
	if( task != NULL ) {
		if( taskIndex.GetValue().IsEmpty() ) {
			taskInfo	= va( L"%ls - %ls", task->GetTitle(), task->GetInfo()->GetXPString() );
		} else {
			taskInfo	= va( L"%ls - %ls (%ls)", task->GetTitle(), task->GetInfo()->GetXPString(), taskIndex.GetValue().c_str() );
		}
	} else {
		// see if there's an objective to show		
		sdTeamInfo* team = localPlayer->GetGameTeam();
		if ( team != NULL ) {
			const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( team );
			sdPlayerTask* objectiveTask = objectiveTasks.Next();
			if ( objectiveTask != NULL ) {
				if( taskIndex.GetValue().IsEmpty() ) {
					taskInfo	= va( L"%ls - %ls", objectiveTask->GetTitle(), objectiveTask->GetInfo()->GetXPString() );
				} else {
					taskInfo	= va( L"%ls - %ls (%ls)", objectiveTask->GetTitle(), objectiveTask->GetInfo()->GetXPString(), taskIndex.GetValue().c_str() );					
				}
			} else {
				taskInfo	= common->LocalizeText( "guis/game/scoreboard/nomission" ).c_str();
			}
		} else {
			taskInfo	= L"";
		}	
	}
*/
	sdTeamInfo* team = localPlayer->GetGameTeam();
	if ( team != NULL ) {
		const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( team );
		sdPlayerTask* objectiveTask = objectiveTasks.Next();
		if ( objectiveTask != NULL ) {
			if( task == NULL ) {
				objectiveTask->SelectWayPoints( gameLocal.time );
			} else {
				objectiveTask->SelectWayPoints( -1 );
			}
		}
	}

	taskSelectedTime = gameLocal.ToGuiTime( gameLocal.time );

}

/*
============
sdPlayerProperties::ClearTask
============
*/
void sdPlayerProperties::ClearTask() {
	taskSelectedTime = 0;
	taskInfo = L"";
}

/*
============
sdPlayerProperties::OnObituary
============
*/
void sdPlayerProperties::OnObituary( idPlayer* self, idPlayer* other ) {
	if ( self == other || other == NULL || !( gameLocal.rules != NULL && gameLocal.rules->IsGameOn() ) ) {
		return;
	}

	bool sameTeam = self->GetTeam() == other->GetTeam();
	if ( self == localPlayer ) {
		idWStrList parms;
		parms.Append( other->userInfo.wideName );

		if ( sameTeam ) {
			lastKillMessage		= common->LocalizeText( killedByPlayerTeamMessage, parms );
		} else {
			lastKillMessage		= common->LocalizeText( killedByPlayerMessage, parms );
		}
		lastKillMessageTime = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );
	} else if ( other == localPlayer ) {
		idWStrList parms;
		parms.Append( self->userInfo.wideName );

		if ( sameTeam ) {
			lastKillMessage		= common->LocalizeText( killedPlayerTeamMessage, parms );
		} else {
			lastKillMessage		= common->LocalizeText( killedPlayerMessage, parms );
		}
		lastKillMessageTime = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );
	}
}

/*
============
sdPlayerProperties::OnActiveViewProxyChanged
============
*/
void sdPlayerProperties::OnActiveViewProxyChanged( idEntity* object ) {	
	proxyOverlay.Reset();

	activeObjectView = object;
	if ( object != NULL ) {
		sdUsableInterface* iface = object->GetUsableInterface();
		if ( iface != NULL ) {
			const sdDeclGUI* gui = iface->GetOverlayGUI();
			if ( gui != NULL ) {
				proxyOverlay.Reset( new sdPassiveHudModule );
				proxyOverlay->InitGui( gui->GetName(), false );				
				proxyOverlay->SetSort( object->spawnArgs.GetInt( "hud_sort", "0" ) );
			}
		}
	}

	if ( proxyOverlay.Get() != NULL && proxyOverlay->GetGuiHandle().IsValid() ) {
		sdUserInterfaceLocal* ui = proxyOverlay->GetGui();
		if ( ui != NULL ) {
			proxyOverlay->Enable( true );
			ui->Activate();

			sdUsableInterface* iface = object->GetUsableInterface();

			if ( iface != NULL ) {
				for ( int i = 0; i < iface->GetNumPositions(); i++ ) {
					SetupVehiclePosition( iface, i, ui );
				}
			}
		}
	}
}

/*
============
sdPlayerProperties::EnteredObject
============
*/
void sdPlayerProperties::EnteredObject( idPlayer* player, idEntity* object ) {
	if ( activeObjectView != object ) {
		return;
	}

	if ( proxyOverlay.Get() != NULL && proxyOverlay->GetGuiHandle().IsValid() ) {
		sdUserInterfaceLocal* ui = proxyOverlay->GetGui();
		if ( ui != NULL ) {
			sdUsableInterface* iface = object->GetUsableInterface();

			if ( iface != NULL ) {
				SetupVehiclePosition( iface, player->GetProxyPositionId(), ui );
			}
		}
	}
}

/*
============
sdPlayerProperties::ExitingObject
============
*/
void sdPlayerProperties::ExitingObject( idPlayer* player, idEntity* object ) {
	if ( activeObjectView != object ) {
		return;
	}

	if ( proxyOverlay.Get() != NULL && proxyOverlay->GetGuiHandle().IsValid() ) {
		sdUserInterfaceLocal* ui = proxyOverlay->GetGui();
		if ( ui != NULL ) {
			sdUsableInterface* iface = object->GetUsableInterface();

			if ( iface != NULL ) {
				SetupVehiclePosition( iface, player->GetProxyPositionId(), ui );
			}
		}
	}
}

/*
============
sdPlayerProperties::CloseActiveHudModules
============
*/
void sdPlayerProperties::CloseActiveHudModules( void ) {
	while ( sdHudModule* module = activeHudModules.Next() ) {
		module->Enable( false );
	}
}

/*
============
sdPlayerProperties::ClosePassiveHudModules
============
*/
void sdPlayerProperties::ClosePassiveHudModules( void ) {
	while ( sdHudModule* module = passiveHudModules.Next() ) {
		module->Enable( false );
	}
}

/*
============
sdPlayerProperties::CloseScriptHudModules
============
*/
void sdPlayerProperties::CloseScriptHudModules( void ) {
	while ( sdHudModule* module = scriptHudModules.Next() ) {
		module->Enable( false );
	}
}

/*
============
sdPlayerProperties::AllocHudModule
============
*/
guiHandle_t sdPlayerProperties::AllocHudModule( const char* name, int sort, bool allowInhibit ) {
	sdHudModule* module = new sdHudModule();
	module->SetSort( sort );
	module->InitGui( name, false );
	module->SetAllowInhibit( allowInhibit );
	sdUserInterfaceLocal* ui = module->GetGui();
	if ( ui != NULL ) {
		ui->Activate();
	}
	allocedHudModules.Alloc() = module;
	PushScriptHudModule( *module );
	AddDrawHudModule( *module );
	return module->GetGuiHandle();
}

/*
============
sdPlayerProperties::FreeHudModule
============
*/
void sdPlayerProperties::FreeHudModule( guiHandle_t handle ) {
	int i;
	for ( i = 0; i < allocedHudModules.Num(); i++ ) {
		if ( allocedHudModules[ i ]->GetGuiHandle() == handle ) {
			sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
			if ( ui ) {
				ui->Deactivate( true );
			}

			delete allocedHudModules[ i ];
			allocedHudModules.RemoveIndex( i );
			return;
		}
	}
}

/*
============
sdPlayerProperties::OnTeamChanged
============
*/
void sdPlayerProperties::OnTeamChanged( sdTeamInfo* oldTeam, sdTeamInfo* newTeam ) {
	if ( GetQuickChatMenu() != NULL && quickChatMenu->Enabled() ) {
		quickChatMenu->Enable( false );
	}
	if ( fireTeamMenu.Get() != NULL && fireTeamMenu->Enabled() ) {
		fireTeamMenu->Enable( false );
	}
	UpdateTeam( newTeam );
	OnSpawnChanged();
}

/*
============
sdPlayerProperties::OnDefaultSpawnChanged
============
*/
void sdPlayerProperties::OnDefaultSpawnChanged( sdTeamInfo* team, idEntity* newSpawn ) {
	OnSpawnChanged();
}

/*
============
sdPlayerProperties::OnSetActiveSpawn
============
*/
void sdPlayerProperties::OnSetActiveSpawn( idEntity* newSpawn ) {
	if ( !localPlayer ) {
		return;
	}

	bool isDefault = false;
	if ( !newSpawn ) {
		isDefault = true;

		sdTeamInfo* team = localPlayer->GetTeam();
		if ( team ) {
			newSpawn = team->GetDefaultSpawn();
		}
	}

	if ( newSpawn ) {
		currentSpawnPoint = newSpawn;

		idScriptObject* obj = newSpawn->GetScriptObject();
		if ( obj ) {
			sdScriptHelper h2;
			h2.Push( isDefault ? 1.f : 0.f );
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnSpawnPointSelected" ), h2 );
		}
	}
}

/*
============
sdPlayerProperties::SetCurrentSpawn
============
*/
void sdPlayerProperties::OnSpawnChanged( void ) {
	idEntity* current = currentSpawnPoint;

	if ( current != NULL && localPlayer != NULL && current == localPlayer->GetSpawnPoint() ) {
		return;
	}

	if ( current ) {
		idScriptObject* obj = current->GetScriptObject();
		if ( obj ) {
			sdScriptHelper h1;
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnSpawnPointDeselected" ), h1 );
		}
	}

	currentSpawnPoint = NULL;

	if ( localPlayer ) {
		OnSetActiveSpawn( localPlayer->GetSpawnPoint() );
		gameLocal.limboProperties.OnSetActiveSpawn( localPlayer->GetSpawnPoint() );
	}
}

/*
============
sdPlayerProperties::OnTaskExpired
============
*/
void sdPlayerProperties::OnTaskExpired( sdPlayerTask* task ) {
	taskExpiredTime = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );

	idWStrList args( 1 );
	args.Append( task->GetTitle() );

	taskStatus = common->LocalizeText( "guis/game/task_removed", args );
}

/*
============
sdPlayerProperties::OnTaskCompleted
============
*/
void sdPlayerProperties::OnTaskCompleted( sdPlayerTask* task ) {
	taskCompletedTime = MS2SEC( gameLocal.ToGuiTime( gameLocal.time ) );

	idWStrList args( 1 );
	args.Append( task->GetCompletedTitle() );
	taskStatus = common->LocalizeText( "guis/game/task_completed", args );
}

/*
===============================================================================

sdGlobalProperties

===============================================================================
*/

/*
================
sdGlobalProperties::sdGlobalProperties
================
*/
sdGlobalProperties::sdGlobalProperties( void ) {
}

/*
================
sdGlobalProperties::~sdGlobalProperties
================
*/
sdGlobalProperties::~sdGlobalProperties( void ) {
}

/*
================
sdGlobalProperties::Init
================
*/
void sdGlobalProperties::Init( void ) {
	const sdDeclStringMap* stringMap = gameLocal.declStringMapType[ "guiGlobals" ];
	if ( !stringMap ) {
		return;
	}

	gameLocal.Printf( "Initializing global UI namespaces\n" ); 
	sdGlobalPropertiesNameSpace::Init( stringMap->GetDict() );	
	gameLocal.Printf( "...%i namespaces\n...%i properties\n", GetNumNamespaces(), GetNumProperties() );
}


/*
============
sdGlobalProperties::Shutdown
============
*/
void sdGlobalProperties::Shutdown() {
	sdGlobalPropertiesNameSpace::Shutdown();
}



/*
===============================================================================

sdGlobalPropertiesNameSpace

===============================================================================
*/

/*
================
sdGlobalPropertiesNameSpace::~sdGlobalPropertiesNameSpace
================
*/
sdGlobalPropertiesNameSpace::~sdGlobalPropertiesNameSpace( void ) {
	namespaces.DeleteContents();
	propertyValues.DeleteContents( true );
	properties.Clear();
}

/*
================
sdGlobalPropertiesNameSpace::Shutdown
================
*/
void sdGlobalPropertiesNameSpace::Shutdown( void ) {
	int i;
	for ( i = 0; i < namespaces.Num(); i++ ) {
		sdGlobalPropertiesNameSpace* ns = *namespaces.GetIndex( i );
		ns->Shutdown();
	}
	propertyValues.DeleteContents( true );
	properties.Clear();
}

/*
================
sdGlobalPropertiesNameSpace::Init
================
*/
void sdGlobalPropertiesNameSpace::Init( const idDict& dict ) {
	int i;
	for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const idKeyValue* kv = dict.GetKeyVal( i );		

		const char* typeName = kv->GetValue();

		if ( properties.GetProperty( kv->GetKey(), sdProperties::PT_INVALID, false ) ) {
			continue;
		}

		if ( !idStr::Icmpn( "namespace_", kv->GetKey(), 10 ) ) {
			const sdDeclStringMap* info = gameLocal.declStringMapType[ kv->GetValue() ];
			if ( !info ) {
				gameLocal.Error( "sdGlobalPropertiesNameSpace::Init Invalid Namespace Data '%s'", kv->GetKey().c_str() );
			}

			sdGlobalPropertiesNameSpace* ns = new sdGlobalPropertiesNameSpace();
			ns->Init( info->GetDict() );

			idStr name = kv->GetKey().Right( kv->GetKey().Length() - 10 );
			name.ToLower();
			namespaces.Set( name, ns );
			continue;
		}

		if ( !idStr::Icmp( typeName, "float" ) ) {
			sdFloatProperty* property = new sdFloatProperty;
			*property = 0.f;
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "vec2" ) ) {
			sdVec2Property* property = new sdVec2Property;
			*property = vec2_origin;
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "vec3" ) ) {
			sdVec3Property* property = new sdVec3Property;
			*property = vec3_origin;
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "vec4" ) || !idStr::Icmp( typeName, "rect" ) || !idStr::Icmp( typeName, "color" ) ) {
			sdVec4Property* property = new sdVec4Property;
			*property = vec4_origin;
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "string" ) ) {
			sdStringProperty* property = new sdStringProperty;
			*property = "";
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "wstring" ) ) {
			sdWStringProperty* property = new sdWStringProperty;
			*property = L"";
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else if ( !idStr::Icmp( typeName, "handle" ) ) {
			sdIntProperty* property = new sdIntProperty;
			*property = -1;
			properties.RegisterProperty( kv->GetKey(), *property );
			propertyValues.Append( property );
		} else {
			gameLocal.Error( "sdGlobalPropertiesNameSpace::Init Invalid Type '%s'", typeName );
		}
	}
}

/*
================
sdGlobalPropertiesNameSpace::GetProperty
================
*/
sdProperties::sdProperty* sdGlobalPropertiesNameSpace::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
================
sdGlobalPropertiesNameSpace::GetProperty
================
*/
sdProperties::sdProperty* sdGlobalPropertiesNameSpace::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && ( prop->GetValueType() != type && type != sdProperties::PT_INVALID ) ) {
		gameLocal.Error( "sdGlobalPropertiesNameSpace::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdGlobalPropertiesNameSpace::GetSubScope
================
*/
sdUserInterfaceScope* sdGlobalPropertiesNameSpace::GetSubScope( const char* name ) {
	sdGlobalPropertiesNameSpace** ns;

	idStr lower = name;
	lower.ToLower();
	if ( !namespaces.Get( lower.c_str(), &ns ) ) {
		return NULL;
	}

	return *ns;
}

/*
============
sdGlobalPropertiesNameSpace::GetNumProperties
============
*/
int	sdGlobalPropertiesNameSpace::GetNumProperties() const {
	int total = GetProperties().Num();
	for( int i = 0; i < namespaces.Num(); i++ ) {
		total += (*namespaces.GetIndex( i ))->GetNumProperties();
	}
	return total;
}

/*
============
sdGlobalPropertiesNameSpace::GetNumNamespaces
============
*/
int sdGlobalPropertiesNameSpace::GetNumNamespaces() const {
	int total = 1;
	for( int i = 0; i < namespaces.Num(); i++ ) {
		total += (*namespaces.GetIndex( i ))->GetNumNamespaces();
	}
	return total;
}

/*
============
sdPlayerProperties::AddDrawHudModule
smaller values are drawn after larger values
============
*/
void sdPlayerProperties::AddDrawHudModule( sdHudModule& module ) {	
	int localSort = module.GetSort();

	sdHudModule* other;
	for ( other = GetDrawHudModule(); other; other = other->GetDrawNode().Next() ) {
		if ( localSort >= other->GetSort() ) {
			module.GetDrawNode().InsertBefore( other->GetDrawNode() );
			return;
		}
	}

	module.GetDrawNode().AddToEnd( drawHudModules );
}

/*
============
sdPlayerProperties::ShouldShowEndGame
============
*/
bool sdPlayerProperties::ShouldShowEndGame() const {
	return activateEndGame != 0 && gameLocal.time >= activateEndGame;
}

/*
============
sdPlayerProperties::SetActiveCamera
============
*/
void sdPlayerProperties::SetActiveCamera( idEntity* camera ) {
	if ( camera == activeCameraEntity ) {
		return;
	}

	idEntity* cam = activeCameraEntity;
	if ( cam != NULL ) {
		idScriptObject* obj = cam->GetScriptObject();
		if ( obj != NULL ) {
			sdScriptHelper h1;
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnFinishRemoteCamera" ), h1 );
		}
	}

	activeCameraEntity = camera;

	if ( camera != NULL ) {
		idScriptObject* obj = camera->GetScriptObject();
		if ( obj != NULL ) {
			sdScriptHelper h1;
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnBecomeRemoteCamera" ), h1 );
		}
	}
}

/*
============
sdPlayerProperties::SetActiveTask
============
*/
void sdPlayerProperties::SetActiveTask( sdPlayerTask* task ) {
	sdPlayerTask* currentTask = NULL;
	if ( activeTask.IsValid() ) {
		currentTask = sdTaskManager::GetInstance().TaskForHandle( activeTask );
	}

	if ( currentTask == task ) {
		return;
	}

	if ( currentTask != NULL ) {
		if ( !currentTask->IsObjective() ) {
			currentTask->HideWayPoint(); // Gordon: Objective waypoints are always active
		}

		idScriptObject* obj = currentTask->GetScriptObject();
		if ( obj != NULL ) {
			obj->CallEvent( "OnTaskDeselected" );
		}
		currentTask->SelectWayPoints( -1 );
	}

	if ( task == NULL ) {
		activeTask = taskHandle_t();
		return;
	}

	activeTask = task->GetHandle();

	task->ShowWayPoint();

	idScriptObject* obj = task->GetScriptObject();
	if ( obj != NULL ) {
		obj->CallEvent( "OnTaskSelected" );
	}
	task->SelectWayPoints( gameLocal.time );
}

/*
============
sdPlayerProperties::SetActiveWeapon
============
*/
void sdPlayerProperties::SetActiveWeapon( idWeapon* weapon ) {
	if ( weapon == activeWeaponEntity ) {
		return;
	}

	idWeapon* weap = activeWeaponEntity;
	if ( weap != NULL ) {
		idScriptObject* obj = weap->GetScriptObject();
		if ( obj != NULL ) {
			sdScriptHelper h1;
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnFinishViewWeapon" ), h1 );
		}

		idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
		if ( viewPlayer != NULL ) {
			sdTransport* transport = viewPlayer->GetProxyEntity()->Cast<sdTransport>();
			if ( transport != NULL ) {
				sdVehicleWeapon* vehicleWeap = transport->GetActiveWeapon( viewPlayer );
				if ( vehicleWeap != NULL ) {
					idScriptObject* obj = vehicleWeap->GetScriptObject();
					if ( obj != NULL ) {
						sdScriptHelper h1;
						obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnFinishViewWeapon" ), h1 );
					}
				}
			}
		}
	}

	activeWeaponEntity = weapon;

	if ( weapon != NULL ) {
		idScriptObject* obj = weapon->GetScriptObject();
		if ( obj != NULL ) {
			sdScriptHelper h1;
			obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnBecomeViewWeapon" ), h1 );
		}

		idPlayer* viewPlayer = gameLocal.GetLocalViewPlayer();
		if ( viewPlayer != NULL ) {
			sdTransport* transport = viewPlayer->GetProxyEntity()->Cast<sdTransport>();
			if ( transport != NULL ) {
				sdVehicleWeapon* vehicleWeap = transport->GetActiveWeapon( viewPlayer );
				if ( vehicleWeap != NULL ) {
					idScriptObject* obj = vehicleWeap->GetScriptObject();
					if ( obj != NULL ) {
						sdScriptHelper h1;
						obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnBecomeViewWeapon" ), h1 );
					}
				}
			}
		}
	}
}

/*
============
sdPlayerProperties::GetActivePlayer
============
*/
idPlayer* sdPlayerProperties::GetActivePlayer( void ) {
	return activePlayerEntity.GetEntity();
}

/*
============
sdPlayerProperties::SetActivePlayer
============
*/
void sdPlayerProperties::SetActivePlayer( idPlayer* player ) {
	if ( player == activePlayerEntity ) {
		return;
	}

	idPlayer* p = activePlayerEntity;
	if ( p != NULL ) {
		sdScriptHelper h1;
		p->CallNonBlockingScriptEvent( p->GetScriptFunction( "OnExitView" ), h1 );
	}

	activePlayerEntity = player;

	if ( activePlayerEntity != NULL ) {
		sdScriptHelper h1;
		activePlayerEntity->CallNonBlockingScriptEvent( activePlayerEntity->GetScriptFunction( "OnEnterView" ), h1 );
	}
}

/*
============
sdPlayerProperties::SetCriticalClass
jrad - ack...
============
*/
void sdPlayerProperties::SetCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass ) {
	sdStringProperty* prop = NULL;
	switch( playerTeam ) {
		case GDF:
			switch( criticalClass ) {
				case SOLDIER:
					gdfCriticalClass = "soldier";
					break;
				case MEDIC:
					gdfCriticalClass = "medic";
					break;
				case ENGINEER:
					gdfCriticalClass = "engineer";
					break;
				case FIELDOPS:
					gdfCriticalClass = "fieldops";
					break;
				case COVERTOPS:
					gdfCriticalClass = "covertops";
					break;
				default:
					gdfCriticalClass = "";
					break;
			}
			break;
		case STROGG:
			switch( criticalClass ) {
			case SOLDIER:
				stroggCriticalClass = "aggressor";
				break;
			case MEDIC:
				stroggCriticalClass = "technician";
				break;
			case ENGINEER:
				stroggCriticalClass = "constructor";
				break;
			case FIELDOPS:
				stroggCriticalClass = "oppressor";
				break;
			case COVERTOPS:
				stroggCriticalClass = "infiltrator";
				break;
			default:
				stroggCriticalClass = "";
				break;
			}
			break;
	}
}
