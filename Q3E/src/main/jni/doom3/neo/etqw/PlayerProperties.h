// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PLAYERPROPERTIES_H__
#define __PLAYERPROPERTIES_H__

#include "guis/UserInterfaceTypes.h"
#include "roles/HudModule.h"
#include "roles/Tasks.h"

class sdTransport;
class sdDeclPlayerClass;
class sdVehiclePosition;
class idWeapon;

SD_UI_PUSH_CLASS_TAG( sdPlayerProperties )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"Player properties may be accessed with \"player.<propname>\". The properties are used throughout the GUIs for all " \
	"information relating to the local player or depending on the property the local view players information. " \
	"All player properties are read only."
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "player";
)
class sdPlayerProperties : public sdUIPropertyHolder {
public:
	enum positionMode_t {
		PM_SELF,
		PM_OTHER,
		PM_EMPTY,
	};
											sdPlayerProperties( void );
											~sdPlayerProperties( void );

	void									Update( idPlayer* player );
	void									PacifierUpdate( void );
	void									UpdateHudModules( void );
	void									Init( void );
	void									InitGUIs( void );
	void									InitGUIStates( void );
	void									Shutdown( void );
	void									ShutdownGUIs( void );

	void									SetContextEntity( idEntity* entity ) { contextEntity = entity; }
	idEntity*								GetContextEntity( void ) { return contextEntity; }

	void									OnActiveViewProxyChanged( idEntity* object );

	void									SetupVehiclePosition( sdUsableInterface* interface, int positionID, sdUserInterfaceLocal* ui );
	void									SetupVehiclePosition( sdUsableInterface* interface, int positionID, positionMode_t mode, sdUserInterfaceLocal* ui );

	void									EnteredObject( idPlayer* player, idEntity* object );
	void									ExitingObject( idPlayer* player, idEntity* vehicle );

	void									OnNewTask( sdPlayerTask* task );
	void									OnTaskSelected( sdPlayerTask* task );
	void									ClearTask();

	bool									ShouldShowEndGame() const;

	bool									ManualVoiceChat() const { return endGame != 0.0f; }

	void									SetCriticalClass( const playerTeamTypes_t playerTeam, const playerClassTypes_t criticalClass );

	virtual sdProperties::sdProperty*		GetProperty( const char* name );
	virtual sdProperties::sdProperty*		GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler& GetProperties()									{ return properties; }
	virtual const char*						GetName() const										{ return "playerProperties"; }
	virtual const char*						FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	void									UpdateTargetingInfo( idPlayer* player );
	void									UpdateFireTeam( idPlayer* player );
	void									UpdateTeam( sdTeamInfo* team );

	sdDeployMenu*							GetDeployMenu( void )								{ return deployMenu.Get(); }
	sdLimboMenu*							GetLimboMenu( void )								{ return limboMenu.Get(); }
	sdQuickChatMenu*						GetQuickChatMenu( void )							{ return quickChatMenu.Get(); }
	sdQuickChatMenu*						GetContextMenu( void )								{ return contextMenu.Get(); }
	sdFireTeamMenu*							GetFireTeamMenu( void )								{ return fireTeamMenu.Get(); }
	sdChatMenu*								GetChatMenu( void )									{ return chatMenu.Get(); }
	sdWeaponSelectionMenu*					GetWeaponSelectionMenu( void )						{ return weaponSelectionMenu.Get(); }
	sdPlayerHud*							GetPlayerHud( void )								{ return playerHud.Get(); }
	sdPostProcess*							GetPostProcess( void )								{ return postProcess.Get(); }
	sdTakeViewNoteMenu*						GetViewNoteMenu( void )								{ return takeViewNoteMenu.Get(); }

	guiHandle_t								GetScoreBoard( void )								{ return scoreBoard; }

	void									SetToolTipInfo( const wchar_t* text, int index, const idMaterial* icon );

	void									CloseActiveHudModules( void );
	void									ClosePassiveHudModules( void );
	void									CloseScriptHudModules( void );

	sdHudModule*							GetActiveHudModule( void )							{ return activeHudModules.Next(); }
	sdHudModule*							GetPassiveHudModule( void )							{ return passiveHudModules.Next(); }
	sdHudModule*							GetScriptHudModule( void )							{ return scriptHudModules.Next(); }
	sdHudModule*							GetDrawHudModule( void )							{ return drawHudModules.Next(); }

	void									AddDrawHudModule( sdHudModule& module );

	void									PushPassiveHudModule( sdHudModule& module )			{ module.GetNode().AddToEnd( passiveHudModules ); }
	void									PushScriptHudModule( sdHudModule& module )			{ module.GetNode().AddToEnd( scriptHudModules ); }
	void									PushActiveHudModule( sdHudModule& module )			{ module.GetNode().AddToEnd( activeHudModules ); }
	void									PushActiveHudModuleUrgent( sdHudModule& module )	{ module.GetNode().AddToFront( activeHudModules ); }

	void									SetTaskIndex( const wchar_t* text )					{ taskIndex = text; }

	void									OnServerInfoChanged() { serverInfoChanged = serverInfoChanged + 1.0f; }

	idEntity*								GetActiveObjectView( void ) const					{ return activeObjectView; }

	guiHandle_t								AllocHudModule( const char* name, int sort, bool allowInhibit );
	void									FreeHudModule( guiHandle_t handle );

	void									SetClipIndex( int index ) { clipIndex = index; }
	bool									GetCommandMapExpanding( void ) const { return commandMapExpanding; }
	void									ToggleCommandMap( void ) { ForceCommandMap( !commandMapExpanding ); }
	void									ForceCommandMap( bool on ) { commandMapExpanding = on; }

	void									OnTeamChanged( sdTeamInfo* oldTeam, sdTeamInfo* newTeam );
	void									OnDefaultSpawnChanged( sdTeamInfo* team, idEntity* newSpawn );
	void									OnSpawnChanged( void );
	void									OnSetActiveSpawn( idEntity* newSpawn );
	void									OnEntityDestroyed( idEntity* entity );
	void									SetLocalPlayer( idPlayer* player ) { localPlayer = player; }
	void									OnTaskExpired( sdPlayerTask* task );
	void									OnTaskCompleted( sdPlayerTask* task );

	void									SetActiveCamera( idEntity* camera );
	void									SetActiveWeapon( idWeapon* weapon );
	void									SetActivePlayer( idPlayer* player );
	void									SetActiveTask( sdPlayerTask* task );

	idPlayer*								GetActivePlayer( void );

	void									OnObituary( idPlayer* self, idPlayer* other );

	void									SetHasTask( bool value ) { hasTask = value; }

	void									SetShowFireTeam( bool keyDown ) { showFireTeam = static_cast< float >( keyDown ); }
	bool									GetShowFireTeam() const { return showFireTeam != 0.0f; }

	void									SetPaused( bool value ) { isPaused = value; }
	void									SetUnpauseKeyString( const wchar_t* key ) { unpauseKeyString = key; }

private:
	static void								UpdatePlayerUpgradeIcons( sdUIIconNotification* icons );
	static void								PlayerUpgradeIcons( sdUIIconNotification* icons );
	static void								UpdatePlayerNotifyIcons( sdUIIconNotification* icons );

private:
	idList< sdFloatProperty	> proficiency;
	idList< sdFloatProperty	> proficiencyPercent;

	ammoType_t			stroyentType;

	sdProperties::sdPropertyHandler			properties;

	sdAutoPtr< sdLimboMenu >				limboMenu;
	sdAutoPtr< sdDeployMenu >				deployMenu;
	sdAutoPtr< sdQuickChatMenu >			quickChatMenu;
	sdAutoPtr< sdQuickChatMenu >			contextMenu;
	sdAutoPtr< sdFireTeamMenu >				fireTeamMenu;
	sdAutoPtr< sdChatMenu >					chatMenu;	
	sdAutoPtr< sdWeaponSelectionMenu >		weaponSelectionMenu;
	sdAutoPtr< sdPlayerHud >				playerHud;
	sdAutoPtr< sdPostProcess >				postProcess;
	sdAutoPtr< sdTakeViewNoteMenu >			takeViewNoteMenu;
	sdAutoPtr< sdPassiveHudModule >			proxyOverlay;

	idLinkList< sdHudModule >				activeHudModules;
	idLinkList< sdHudModule >				passiveHudModules;
	idLinkList< sdHudModule >				scriptHudModules;
	idLinkList< sdHudModule >				drawHudModules;

	idList< sdHudModule* >					allocedHudModules;

	guiHandle_t								scoreBoard;

	int										clipIndex;
	bool									commandMapExpanding;

	int										activateEndGame;

	const sdDeclLocStr*						killedPlayerMessage;
	const sdDeclLocStr*						killedByPlayerMessage;
	const sdDeclLocStr*						killedPlayerTeamMessage;
	const sdDeclLocStr*						killedByPlayerTeamMessage;
	const sdDeclLocStr*						currentMission;

	idEntityPtr< idEntity >					currentSpawnPoint;
	idEntityPtr< idEntity >					activeObjectView;
	idEntityPtr< idEntity >					activeCameraEntity;
	idEntityPtr< idWeapon >					activeWeaponEntity;
	idEntityPtr< idPlayer >					activePlayerEntity;
	taskHandle_t							activeTask;

	idPlayer*								localPlayer;
	idEntityPtr< idEntity >					contextEntity;

	idStaticList< int, MAX_ASYNC_CLIENTS >	voipChatTimes;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Health";
	desc				= "Local view player health.";
	datatype			= "float";
	)
	sdFloatProperty		health;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/MaxHealth";
	desc				= "Local view player max health.";
	datatype			= "float";
	)
	sdFloatProperty		maxHealth;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Speed";
	desc				= "Local view player or vehicle velocity.";
	datatype			= "float";
	)
	sdFloatProperty		speed;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Yaw";
	desc				= "View player yaw.";
	datatype			= "float";
	)
	sdFloatProperty		yaw;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleYaw";
	desc				= "Vehicle yaw.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleYaw;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Pitch";
	desc				= "View player pitch.";
	datatype			= "float";
	)
	sdFloatProperty		pitch;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Role";
	desc				= "View player class name or \"spec\" if spectating.";
	datatype			= "string";
	)
	sdStringProperty	role;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/RoleTitle";
	desc				= "View player class title or spectator title if spectating.";
	datatype			= "int";
	)
	sdIntProperty		roleTitle;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskInfo";
	desc				= "TODO: not used.";
	datatype			= "wstring";
	)
	sdWStringProperty	taskInfo;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Location";
	desc				= "Location name for view players current location.";
	datatype			= "wstring";
	)
	sdWStringProperty	location;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/GdfCriticalClass";
	desc				= "Class name for the class that can complete the current objective.";
	datatype			= "string";
	)
	sdStringProperty	gdfCriticalClass;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/StroggCriticalClass";
	desc				= "Class name for the class that can complete the current objective.";
	datatype			= "string";
	)
	sdStringProperty	stroggCriticalClass;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Xp";
	desc				= "Local View player XP.";
	datatype			= "float";
	)
	sdFloatProperty		xp;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Rank";
	desc				= "View player's rank or invalid handle if there's no rank.";
	datatype			= "int";
	)
	sdIntProperty		rank;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/RankMaterial";
	desc				= "View player's rank material name.";
	datatype			= "string";
	)
	sdStringProperty	rankMaterial;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsReady";
	desc				= "Player is ready.";
	datatype			= "float";
	)
	sdFloatProperty		isReady;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/NeedsReady";
	desc				= "Server is in warmup mode waiting for players to ready up.";
	datatype			= "float";
	)
	sdFloatProperty		needsReady;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/SpreadFraction";
	desc				= "View player weapon spread.";
	datatype			= "float";
	)
	sdFloatProperty		spreadFraction;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsDead";
	desc				= "Player is unconcious waiting for revive or respawn.";
	datatype			= "float";
	alias				= "dead";
	)
	sdFloatProperty		isDead;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsViewDead";
	desc				= "Player is unconcious waiting for revive or respawn.";
	datatype			= "float";
	alias				= "viewDead";
	)
	sdFloatProperty		isViewDead;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsInLimbo";
	desc				= "Player is in the limbo menu and might be waiting for respawn.";
	datatype			= "float";
	alias				= "limbo";
	)
	sdFloatProperty		isInLimbo;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsSpawning";
	desc				= "Player waiting to spawn.";
	datatype			= "float";
	alias				= "spawning";
	)
	sdFloatProperty		isSpawning;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/CommandMapState";
	desc				= "True if expanding.";
	datatype			= "float";
	)
	sdFloatProperty		commandMapState;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/MapLoadPercent";
	desc				= "Map loading percent.";
	datatype			= "float";
	)
//	sdFloatProperty		mapLoadPercent;	

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Position";
	desc				= "View player latitude/longitude/altitude.";
	datatype			= "vec3";
	)
	sdVec3Property		position;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/InProxy";
	desc				= "View player in proxy (vehicle or mountable weapon).";
	datatype			= "float";
	)
	sdFloatProperty		inProxy;

	SD_UI_PUSH_GROUP_TAG( "Vehicle Properties" )

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleValid";
	desc				= "View player is in vehicle.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleValid;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleHealth";
	desc				= "View player vehicle health.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleHealth;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehiclePosition";
	desc				= "View players vehicle position.";
	datatype			= "int";
	)
	sdIntProperty		vehiclePosition;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleDestructionTime";
	desc				= "MCP destruction time if MCP moves outside the valid path.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleDestructTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleWrongDirection";
	desc				= "True if MCP is driving in the wrong direction.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleWrongDirection;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleKickDistance";
	desc				= "Distance before being kicked while driving the MCP.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleKickDistance;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleEMPed";
	desc				= "Time the vehicle is EMPed.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleEMPed;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleWeaponEMPed";
	desc				= "Time the vehicle weapon is EMPed.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleWeaponEMPed;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleSiegeMode";
	desc				= "True if vehicle is in siege mode.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleSiegeMode;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VehicleThirdPerson";
	desc				= "Players vehicle camera mode is third person.";
	datatype			= "float";
	)
	sdFloatProperty		vehicleThirdPerson;

	SD_UI_POP_GROUP_TAG
	SD_UI_PUSH_GROUP_TAG( "Weapon/Damage Properties" )

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponName";
	desc				= "View players weapon title handle.";
	datatype			= "int";
	)
	sdIntProperty		weaponName;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponLookupName";
	desc				= "Lookup name for view players weapon.";
	datatype			= "string";
	)
	sdStringProperty	weaponLookupName;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponClip";
	desc				= "Ammo in clip.";
	datatype			= "float";
	)
	sdFloatProperty		weaponClip;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponTotalClip";
	desc				= "Total ammo in weapon.";
	datatype			= "float";
	)
	sdFloatProperty		weaponTotalClip;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponNeedsAmmo";
	desc				= "Weapon needs ammo to shoot.";
	datatype			= "float";
	)
	sdFloatProperty		weaponNeedsAmmo;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponShotsPerClip";
	desc				= "Total shots per clip.";
	datatype			= "float";
	)
	sdFloatProperty		weaponShotsPerClip;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponShotsAvailable";
	desc				= "Shots left.";
	datatype			= "float";
	)
	sdFloatProperty		weaponShotsAvailable;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WeaponSlot";
	desc				= "Slot for item.";
	datatype			= "float";
	)
	sdFloatProperty		weaponSlot;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastDamageTime";
	desc				= "Last time damage was dealt to view player.";
	datatype			= "float";
	)
	sdFloatProperty		lastDamageTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastDamageIntensity";
	desc				= "Last damage amount dealt to the view player.";
	datatype			= "float";
	)
	sdFloatProperty		lastDamageIntensity;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/HitFeedback";
	desc				= "True if view player dealt damage to another player.";
	datatype			= "float";
	)
	sdFloatProperty		hitFeedback;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastHitFeedbackType";
	desc				= "Team allegiance for the player the view player dealt damage to.";
	datatype			= "float";
	)
	sdFloatProperty		lastHitFeedbackType;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastKillMessage";
	desc				= "Last kill message (you killed X/you were killed by X messages).";
	datatype			= "wstring";
	)
	sdWStringProperty	lastKillMessage;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastKillMessageTime";
	desc				= "Last time the kill message was changed.";
	datatype			= "float";
	)
	sdFloatProperty		lastKillMessageTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TargetingCenter";
	desc				= "Targeting position on HUD for target lock rectangle.";
	datatype			= "vec2";
	)
	sdVec2Property 		targetingCenter;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TargetingColor";
	desc				= "Color of target lock materials to be drawn by the GUI.";
	datatype			= "vec4";
	)
	sdVec4Property 		targetingColor;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TargetingPercent";
	desc				= "Fraction if locking on. 0 If not locking on.";
	datatype			= "float";
	)
	sdFloatProperty		targetingPercent;

	SD_UI_POP_GROUP_TAG

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ShowCrosshair";
	desc				= "Show crosshair, used to hide crosshair in vehicles/mounts.";
	datatype			= "float";
	)
	sdFloatProperty		showCrosshair;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ShowTargetingInfo";
	desc				= "Show targeting info, used in vehicle weapon GUIs.";
	datatype			= "float";
	)
	sdFloatProperty		showTargetingInfo;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/HideDecoyInfo";
	desc				= "TODO: unused.";
	datatype			= "float";
	)
	sdFloatProperty		hideDecoyInfo;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/CrosshairDistance";
	desc				= "Distance form view player to point crosshair is pointing in meters. Used for range indicators in weapons.";
	datatype			= "string";
	)
	sdStringProperty	crosshairDistance;

	SD_UI_PUSH_GROUP_TAG( "Misson System Properties" )
	
	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/FireTeamName";
	desc				= "Fire team name.";
	datatype			= "wstring";
	)
	sdWStringProperty	fireTeamName;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/FireTeamActive";
	desc				= "View player is in a fire team.";
	datatype			= "float";
	)
	sdFloatProperty		fireTeamActive;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/FireTeamLeader";
	desc				= "True if view player is fireteam leader.";
	datatype			= "float";
	)
	sdFloatProperty		fireTeamLeader;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/FireTeamShow";
	desc				= "Show fireteam list.";
	datatype			= "float";
	)
	sdFloatProperty		fireTeamShow;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskStatus";
	desc				= "New mission available to the player.";
	datatype			= "wstring";
	)
	sdWStringProperty	taskStatus;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskAddedTime";
	desc				= "Time the new mission became available to the player.";
	datatype			= "float";
	)
	sdFloatProperty		taskAddedTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskSelectedTime";
	desc				= "Last time the player selected a new mission.";
	datatype			= "float";
	)
	sdFloatProperty		taskSelectedTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskCompletedTime";
	desc				= "Task a mission was completed.";
	datatype			= "float";
	)
	sdFloatProperty		taskCompletedTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskExpiredTime";
	desc				= "Time a task expired.";
	datatype			= "float";
	)
	sdFloatProperty		taskExpiredTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TaskIndex";
	desc				= "TODO: unused.";
	datatype			= "wstring";
	)
	sdWStringProperty	taskIndex;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/HasTask";
	desc				= "True if player finished cycling tasks.";
	datatype			= "float";
	)
	sdFloatProperty		hasTask;

	SD_UI_POP_GROUP_TAG

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LagOMeter";
	desc				= "True if lagOMeter should be visible.";
	datatype			= "float";
	)
	sdFloatProperty		lagOMeter;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Name";
	desc				= "Players name (including color codes).";
	datatype			= "string";
	)
	sdStringProperty	name;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Spectating";
	desc				= "Player is spectating (true if in the limbo menu, waiting to respawn or at the end of the game).";
	datatype			= "float";
	)
	sdFloatProperty		spectating;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Spectator";
	desc				= "True if a spectator (not on any teams).";
	datatype			= "float";
	)
	sdFloatProperty		spectator;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LocalView";
	desc				= "False if spectating others.";
	datatype			= "float";
	)
	sdFloatProperty		localView;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/SpectateClient";
	desc				= "View players name (including color codes).";
	datatype			= "string";
	)
	sdStringProperty	spectateClient;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoiceSending";
	desc				= "True if player sent a voice message within the last 0.5 seconds.";
	datatype			= "float";
	)
	sdFloatProperty		voiceSending;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoiceReceiving";
	desc				= "True if player received a voice message within the last 0.5 seconds..";
	datatype			= "float";
	)
	sdFloatProperty		voiceReceiving;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsChatting";
	desc				= "TODO: unused.";
	datatype			= "float";
	)
	sdFloatProperty		isChatting;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsLagged";
	desc				= "True if player is lagged.";
	datatype			= "float";
	)
	sdFloatProperty		isLagged;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/SpawnSelected";
	desc				= "TODO: unused in the GUIS. True if the view player has selected a spawn point.";
	datatype			= "float";
	)
	sdFloatProperty		spawnSelected;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ToolTipText";
	desc				= "Tooltip text to display.";
	datatype			= "wstring";
	)
	sdWStringProperty	toolTipText;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ToolTipIsPriority";
	desc				= "Tooltip is an important tooltip.";
	datatype			= "float";
	)
	sdFloatProperty		toolTipIsPriority;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ToolTipMaterial";
	desc				= "Icon name appropriate for the tooltip.";
	datatype			= "string";
	)
	sdStringProperty	toolTipMaterial;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ToolTipLocation";
	desc				= "TODO: unused in the GUIs. Tooltip location title handle.";
	datatype			= "int";
	)
	sdIntProperty		toolTipLocation;

	SD_UI_PUSH_GROUP_TAG( "Match/Team Properties" )

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TeamName";
	desc				= "Team lookup name or \"spectating\" if player is spectator.";
	datatype			= "string";
	)
	sdStringProperty	teamName;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/TeamNameView";
	desc				= "Team lookup name for view player or equal to teamName if local player isn't spectating.";
	datatype			= "string";
	)
	sdStringProperty	teamNameView;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/NextVoteTime";
	desc				= "Next time the local player can call a vote. 0 if allowed immediately.";
	datatype			= "float";
	)
	sdFloatProperty		nextVoteTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WinningTeam";
	desc				= "Used during the end game. Lookup name for the winning team or \"\" if there is no winner.";
	datatype			= "string";
	)
	sdStringProperty	winningTeam;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WinningMusic";
	desc				= "Sound shader for musing to play during end game.";
	datatype			= "string";
	)
	sdStringProperty	winningMusic;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WinningTeamTitle";
	desc				= "Title handle for winning team.";
	datatype			= "int";
	)
	sdIntProperty		winningTeamTitle;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/WinningTeamString";
	desc				= "Win text handle to display, different depending on the winning team.";
	datatype			= "int";
	)
	sdIntProperty		winningTeamString;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/winningTeamReason";
	desc				= "Win text handle to display, gives the reason for the team winning (if a tiebreak situation).";
	datatype			= "int";
	)
	sdIntProperty		winningTeamReason;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/EndGameCamera";
	desc				= "TODO: redundant. True if at the end game.";
	datatype			= "float";
	)
	sdFloatProperty		endGameCamera;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/EndGame";
	desc				= "True if at the end game.";
	datatype			= "float";
	)
	sdFloatProperty		endGame;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/NextGameStateTime";
	desc				= "Time when changing game state (endgame -> next map load for example).";
	datatype			= "float";
	)
	sdFloatProperty		nextGameStateTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/MathTime";
	desc				= "Match time in milliseconds. Counts down.";
	datatype			= "float";
	)
	sdFloatProperty		matchTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/Warmup";
	desc				= "Players are in warmup.";
	datatype			= "float";
	)
	sdFloatProperty		warmup;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/MatchStatus";
	desc				= "Match status string for warmup and scoreboard.";
	datatype			= "wstring";
	)
	sdWStringProperty	matchStatus;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/MatchType";
	desc				= "One of the three match types.";
	datatype			= "wstring";
	)
	sdWStringProperty	matchType;

	SD_UI_POP_GROUP_TAG

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ScoreboardActive";
	desc				= "True if the view player is viewing the scoreboard.";
	datatype			= "float";
	)
	sdFloatProperty		scoreboardActive;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastHitTime";
	desc				= "TODO: unused.";
	datatype			= "float";
	)
	sdFloatProperty		lastHitTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/DeploymentActive";
	desc				= "True if view player is requesting a deployment.";
	datatype			= "float";
	)
	sdFloatProperty		deploymentActive;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/DeployRotation";
	desc				= "Deployment yaw. Rotation of the last deplyable placed is saved for next time.";
	datatype			= "float";
	)
	sdFloatProperty		deployRotation;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/DeployPosition";
	desc				= "Current deploy position.";
	datatype			= "vec3";
	)
	sdVec3Property		deployPosition;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/DeployIsRotating";
	desc				= "View player is in the rotation mode.";
	datatype			= "float";
	)
	sdFloatProperty		deployIsRotating;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VotingAllowed";
	desc				= "False if voting has been disabled by the server.";
	datatype			= "float";
	)
	sdFloatProperty		votingAllowed;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoteActive";
	desc				= "True if a vote is in progress.";
	datatype			= "float";
	)
	sdFloatProperty		voteActive;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoteText";
	desc				= "The vote text if a vote is in progress.";
	datatype			= "wstring";
	)
	sdWStringProperty	voteText;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoteYesCount";
	desc				= "Yes votes for current vote.";
	datatype			= "float";
	)
	sdFloatProperty		voteYesCount;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoteNoCount";
	desc				= "No votes for current vote.";
	datatype			= "float";
	)
	sdFloatProperty		voteNoCount;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ServerInfoChanged";
	desc				= "Incremented when serverinfo has changed.";
	datatype			= "float";
	)
	sdFloatProperty		serverInfoChanged;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/LastVoiceSender";
	desc				= "Name of the last voice sender.";
	datatype			= "string";
	)
	sdStringProperty	lastVoiceSender;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/VoiceSendMode";
	desc				= "Channel that the local player is sending voice to.";
	datatype			= "float";
	)
	sdFloatProperty		voiceSendMode;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/GameTime";
	desc				= "Real game time elapsed, does not include any time the game was paused for.";
	datatype			= "float";
	)
	sdFloatProperty		gameTime;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ShowFireTeam";
	desc				= "Local player wants to see the fireteam list on the HUD.";
	datatype			= "float";
	)
	sdFloatProperty		showFireTeam;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsPaused";
	desc				= "Game is paused.";
	datatype			= "float";
	)
	sdFloatProperty		isPaused;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/SinglePlayerGame";
	desc				= "Is a single player game.";
	datatype			= "float";
	alias				= "isSinglePlayer"
	)
	sdFloatProperty		singlePlayerGame;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/UnpauseKeyString";
	desc				= "Optional weapon key to unpause tooltip.";
	datatype			= "wstring";
	)
	sdWStringProperty	unpauseKeyString;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/ServerIsRepeater";
	desc				= "Is the server you are connected to a TV server.";
	datatype			= "float";
	alias				= "serverIsRepeater"
	)
	sdFloatProperty		serverIsRepeater;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/IsServer";
	desc				= "True if the server.";
	datatype			= "float";
	)
	sdFloatProperty		isServer;

	SD_UI_PROPERTY_TAG(
	title				= "PlayerProperties/InLetterBox";
	desc				= "Local view player letterbox state.";
	datatype			= "float";
	)
	sdFloatProperty		inLetterBox;
};

class sdGlobalPropertiesNameSpace : public sdUIWritablePropertyHolder {
public:
													~sdGlobalPropertiesNameSpace( void );

	void											Init( const idDict& dict );
	void											Shutdown( void );

	virtual sdProperties::sdProperty*				GetProperty( const char* name );
	virtual sdProperties::sdProperty*				GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler&		GetProperties() { return properties; }
	virtual const sdProperties::sdPropertyHandler&	GetProperties() const { return properties; }
	virtual const char*								GetName() const { return ""; }
	virtual const char*								FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	virtual sdUserInterfaceScope*					GetSubScope( const char* name );

	virtual int										NumSubScopes() const { return namespaces.Num(); }
	virtual const char*								GetSubScopeNameForIndex( int index ) { return namespaces.GetKey( index ).c_str(); }
	virtual sdUserInterfaceScope*					GetSubScopeForIndex( int index ) { return *namespaces.GetIndex( index ); }

	int												GetNumNamespaces() const;
	int												GetNumProperties() const;

private:
	sdProperties::sdPropertyHandler					properties;

	idHashMap< sdGlobalPropertiesNameSpace* >		namespaces;

	idList< sdProperties::sdPropertyValueBase* >	propertyValues;
};

class sdGlobalProperties : public sdGlobalPropertiesNameSpace {
public:
													sdGlobalProperties( void );
	virtual											~sdGlobalProperties( void );

	void											Init( void );
	void											Shutdown( void );

private:
};

#endif // __PLAYERPROPERTIES_H__
