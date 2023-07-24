// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __BOTTHREADDATA_H__
#define __BOTTHREADDATA_H__

#include "BotAI_Main.h"
#include "BotAI_Actions.h"
#include "BotAI_Routes.h"
#include "BotAI_Obstacles.h"
#include "../misc/PlayerBody.h"
#include "Bot_Common.h"

#define MAIN_GUN		 0
#define PISTOL_SLOT		 1
#define GUN_SLOT		 2
#define GRENADE_SLOT	 3

#define MAX_CHARGES		 2	//mal: the max number of charges that will be tracked for each client, at any one time.
#define MAX_ITEMS		 8  //mal: the max number of health/ammo/supply packs we will track per client, at any one time.
#define MAX_KILLS		 4  //mal: the max number of kills we'll keep track of.
#define MAX_VEHICLES	 32 //mal: the max number of vehicles we'll keep track of. //FIXME: is this enough??!
#define MAX_CHATS		 64 //mal: the max number of chats the bots will track and react to.
#define MAX_SPAWNHOSTS	 20 //mal: the max number of spawnhosts the bots will track and use.
#define MAX_GRENADES	 4
#define MAX_MINES		 3
#define MAX_CARRYABLES	 4
#define MAX_DEPLOYABLES	 38 
#define MAX_STROYBOMBS	 32
#define MAX_TEAMS		 2
#define MAX_SHIELDS		 4

#define BOT_GUIDE_NAME_SLOT 0

#define MAX_BOT_SPECTATE_TIMER 300

#define MAX_CLIENT_CHARGES MAX_CLIENTS * MAX_CHARGES

#define SMOKE_GRENADE_LIFETIME	32000

#define MAX_TARGET_TIME			10000

#define BOT_THREAD_BUFFER_SIZE		8

#define BOT_THINK_DELAY_TIME	( bot_threadMaxFrameDelay.GetInteger() * USERCMD_MSEC ) //mal: how long the delay could be with the bots thinking with the threading system.

#define VEHICLE_BOX_EXPAND		50.0f
#define NORMAL_BOX_EXPAND		25.0f
#define NORMAL_BOUNDS_EXPAND	25.0f
#define BOT_CROSSHAIR_DIST		30.0f

#define HALF_PLAYER_BBOX		16.0f
#define NORMAL_PLAYER_BBOX		32.0f

#define MINOR_RESET_EVENT		1
#define MAJOR_RESET_EVENT		2

enum gameMapNames_t {
	UNKNOWN_MAP,
	AREA22,
	ARK,
	CANYON,
	ISLAND,
	OUTSKIRTS,
	QUARRY,
	REFINERY,
	SALVAGE,
	SEWER,
	SLIPGATE,
	VALLEY,
	VOLCANO
};

struct teamUsesSpawn_t {
	bool					teamUsesSpawn;
	int						percentage;
};

struct actorMissionInfo_t {
	bool					hasBriefedPlayer;
	bool					forwardSpawnIsAllowed;
	bool					setup;
	bool					deployableStageIsActive;
	bool					playerNeedsFinalBriefing;
	bool					playerIsOnFinalMission;
	bool					setupBotNames;
	bool					hasEnteredActorNode;
	int						chatPauseTime;
	int						actionNumber;
	int						targetClientNum;
	int						actorClientNum;
	int						goalPauseTime;
};

struct teamMineInfo_t {
	bool					isPriority;
};

struct gameLocalInfo_t {
	bool					inWarmup;
	bool					inEndGame;
	bool					heroMode;		//mal: is the game in hero mode - can update at will!
	bool					botsUseTKRevive;
	bool					botsCanStrafeJump;
	bool					botsUseVehicles;
	bool					botsUseAirVehicles;
	bool					botsStayInVehicles;
	bool					botsSillyWarmup;
	bool					botsIgnoreGoals;
	bool					botsUseSpawnHosts;
	bool					botsUseUniforms;
	bool					botsUseDeployables;
	bool					botsUseMines;
	bool					debugBots;
	bool					debugObstacleAvoidance;
	bool					friendlyFireOn;
	bool					botsPaused;
	bool					debugBotWeapons;
	bool					botKnifeOnly;
	bool					debugPersonalVehicles;
	bool					debugAltRoutes;
	bool					botsCanSuicide;
	bool					botsCanDecayObstacles;
	bool					botsSleep;
	bool					teamStroggHasHuman;
	bool					teamGDFHasHuman;
	bool					gameIsBotMatch;
	bool					botsDoObjsInTrainingMode;
	int						botFollowPlayer;
	int						botIgnoreEnemies;
	int						time;
	int						botSkill;
	int						botAimSkill;
	int						numClients;
	int						nextGDFRespawnTime;
	int						nextStroggRespawnTime;
	int						botPauseInVehicleTime;
	int						botTrainingModeObjDelayTime;

//mal: timers
	int						energyTimerTime;		// points to the time it takes to recharge the charge bar
	int						bombTimerTime;			// points to the time it takes to plant a bomb
	int						fireSupportTimerTime;	// points to the time it takes to launch fire support
	int						deviceTimerTime;		// landmines, etc.
	int						supplyTimerTime;		// health/ammo packs
	int						deployTimerTime;		// deployables

//mal: constants
	int						chargeExplodeTime;		// how long until a HE/Plasma charge explodes

	int						gameTimeInMinutes;

	float					maxVehicleHeight;
	float					crouchViewHeight;
	float					proneViewHeight;
	float					normalViewHeight;

	teamUsesSpawn_t			teamUsesSpawnInfo[ MAX_TEAMS ];
	teamMineInfo_t			teamMineInfo[ MAX_TEAMS ];

	playerTeamTypes_t		winningTeam;
	gameMapNames_t			gameMap;
};

struct playerBodiesInfo_t {//mal: all values set at the end of idGameLocal::RunFrame
	bool					isValid;							
	bool					uniformStolen;
	bool					isSpawnHost;
	bool					isSpawnHostAble;
	bool					isSelectedAsSpawn;
	int						bodyOwnerClientNum;
	int						bodyNum;
	int						areaNum;				//mal: what area of the AAS this body is in.
	int						spawnID;
	playerTeamTypes_t		bodyTeam;
	idVec3					bodyOrigin;	
};

struct spawnHostInfo_t {
	bool					areaChecked;
	int						entNum;
	int						spawnID;
	int						areaNum;
	idVec3					origin;
};

struct covertToolInfo_t { //mal: flyer hives/third eye cameras
	bool					clientIsUsing;						//mal: is the client currently looking thru this tool? Always true for hives atm.
	int						entNum;
	int						spawnID;
	float					xySpeed;
	idVec3					origin;
	idMat3					axis;
};

struct forceShieldInfo_t {
	int						entNum;
	int						spawnID;
	idVec3					origin;
};

struct smokeBombInfo_t {
	int						entNum;
	int						spawnID;
	int						birthTime;
	float					xySpeed;
	idVec3					origin;
};

struct stroyBombInfo_t {
	int						entNum;
	int						spawnID;
	float					xySpeed;
	idVec3					origin;
};

struct gameAbilities_t {	//mal: track unlockable abilities that affect how/if the bots do their job.
	bool fasterMedicCharge;
	bool grenadeLauncher;
	bool stroggCovertNoFootSteps;
	bool stroggAdrenaline;
	bool gdfAdrenaline;
	bool stroggRepairDrone;
	bool gdfStealthToRadar;
	bool selfArmingMines;
	int rank;
};

#define TARGET_ARTILLERY		0
#define TARGET_ROCKETS			1
#define TARGET_SSM				2

struct artyAttackInfo_t  {
	int						birthTime;
	int						deathTime;
	int						type;
	float					radius;
	idVec3					origin;
};

struct scriptHandlers_t { 
	qhandle_t				chargeTimer;						//mal: a handle to the player's charge bar
	qhandle_t				bombTimer;							//mal: a handle to the player's charge bar
	qhandle_t				fireSupportTimer;
	qhandle_t				deviceTimer;
	qhandle_t				supplyTimer;
	qhandle_t				deployTimer;

	//mal_TODO: add more here as needed!
};

enum botVehicleWeaponInfo_t { //mal: these are the different types of weapons that are available to the bots while in a vehicle.
	NULL_VEHICLE_WEAPON = -1,
	PERSONAL_WEAPON,
	MINIGUN, // hyperblaster too
	ROCKETS,
	LAW,
	TANK_GUN, // SBC and the like. Titans, Desecrators, and goliaths all have this.
	DECOY_FLARE,
	STROY_BOMB
};

struct weaponInfo_t { //mal_TODO: keep adding more info for weapons, as its needed, ALSO - setup where this are placed
	bool					primaryWeaponNeedsUpdate;
	bool					isReady;							//mal: is in the up and ready to fire position
	bool					primaryWeapHasAmmo;					//mal: does the weapon in this bank have ammo?
	bool					sideArmHasAmmo;
	bool					primaryWeapNeedsAmmo;				//mal: does our main gun need ammo?
	bool					isIronSightsEnabled;
	bool					isScopeUp;
	bool					primaryWeapClipEmpty;
	bool					hasNadeAmmo;
	bool					isReloading;						//mal: is the client busy reloading their weapon?
	bool					isFiringWeap;						//mal: is this client firing their weapon?
	bool					primaryWeapNeedsReload;				//mal: does our main gun need a reload?
	bool					hasAmmoForReload;
	bool					hasNadeLauncher;
	int						grenadeFuseStart;					//mal: the timer on the bots nade. -1 if bot has no active nade
	plantedMineInfo_t		landMines[ MAX_MINES ];
	grenadeInfo_t			grenades[ MAX_GRENADES ];
	playerWeaponTypes_t		weapon;								//mal: set in idWeapon::GetWeaponDef - current weapon client has
	playerWeaponTypes_t		primaryWeapon;						//mal: set in idWeapon::GetWeaponDef - this is the "main" gun for this client
	airstrikeInfo_t			airStrikeInfo;
	covertToolInfo_t		covertToolInfo;
	artyAttackInfo_t		artyAttackInfo;
};

struct carryableObjInfo_t {
	bool					onGround;	//mal: is the docs/brain/gold/flag/parts/etc on the ground.
	int						entNum;
	int						spawnID;
	int						areaNum;
	int						carrierEntNum; // if exists
	int						parentActionNum;
	playerTeamTypes_t		ownerTeam;
	idVec3					origin;
};

struct clientProxyInfo_t {
	bool					weaponIsReady;						//mal: is the current weapon ready?'
	bool					clientChangedSeats;				//mal: did the client move seats in the vehicle recently?
	bool					hasTurretWeapon;
	int						time;						//mal: how long since the player entered this vehicle
	int						entNum;						//mal: set in idPlayer::Think
	float					boostCharge;				//mal: for vehicles with limited boost charge.
	botVehicleWeaponInfo_t	weapon;								//mal: does the seat the bots in atm have a weapon available?
	idVec3					weaponOrigin;
	idMat3					weaponAxis;
};

struct gameNeededClassInfo_t {
	int						numCriticalClass;
	playerClassTypes_t		criticalClass;
	playerClassTypes_t		donatingClass;
};

struct teamRetreatInfo_t {
	int						retreatTime;
};

struct botGoalInfo_t { //mal_TODO: keep adding more info for goals, as its needed, ALSO - setup where these are placed
	bool					objOnGround;						//mal: is the docs/key/gold/whatever on the ground ATM?
	bool					mapHasMCPGoal;
	bool					team_GDF_AttackDeployables;
	bool					team_STROGG_AttackDeployables;
	bool					gameIsOnFinalObjective;
	bool					isTrainingMap;
	int						deliverActionNumber;
	int						mapHasMCPGoalTime;
	int						team_GDF_PrimaryAction;
	int						team_GDF_SecondaryAction;
	int						team_STROGG_PrimaryAction;
	int						team_STROGG_SecondaryAction;
	int						botGoal_MCP_VehicleNum;
	float					botSightDist;
	gameNeededClassInfo_t	teamNeededClassInfo[ MAX_TEAMS ];
	teamRetreatInfo_t		teamRetreatInfo[ MAX_TEAMS ];
	playerTeamTypes_t		attackingTeam;
	playerClassTypes_t		team_GDF_criticalClass;					//mal: the critical class currently, so bots know who to escort, defend, etc for the GDF.
	playerClassTypes_t		team_STROGG_criticalClass;				//mal: the critical class currently, so bots know who to escort, defend, etc for the Strogg.
	sdSafeArray< carryableObjInfo_t, MAX_CARRYABLES >		carryableObjs;
};

struct clientInfo_t {
	bool					inGame;								//mal: set true in idPlayer::Spawn, set false in idGameLocal::ServerClientDisconnect
	bool					isTryingToMove;
	bool					hasCrosshairHint;					//mal: set in idPlayer::GetCrosshairInfo - just checks if bot has a crosshair hint
	bool					revived;							//mal: set in idPlayer::SpawnToPoint
	bool					justSpawned;						//mal: set in idPlayer::SpawnToPoint
	bool					inLimbo;							//mal: set in idPlayer::Think
	bool					isPanting;							//mal: set in idPlayer::Think
	bool					hasGroundContact;					//mal: set in idPlayer::Think
	bool					hasJumped;							//mal: set in idPlayer::Think
	bool					inWater;							//mal: set in idPlayer::Think
	bool					wantsVehicle;
	bool					hasRepairDroneInWorld;				
	bool					isTeleporting;
	bool					usingMountedGPMG;
	bool					isNoTarget;							//mal: set in idPlayer::Think
	bool					onLadder;							//mal: set in idPlayer::Think
	bool					isInRadar;							//mal: set in idPlayer::Think
	bool					needsParachute;						//mal: set in idPlayer::Think
	bool					isBot;								//mal: set in idGameLocal::SpawnPlayer
	bool					enemyHasLockon;						//mal: does an enemy have a locked on us - if we're in a vehicle or turret of some kind.
	bool					isCamper;
	bool					inPlayZone;							//mal: set in idPlayer::Think - is the player in a valid play zone?
	bool					targetLocked;						//mal: set in idPlayer::Think - is our target locked on?
	bool					isMovingForward;					//mal: set in idPlayer::Think
	bool					isLeaning;							//mal: set in idPlayer::Think
	bool					isDisguised;						//mal: set in idPlayer::Think
	bool					inEnemyTerritory;					//mal: set in idPlayer::Think
	bool					hasTeleporterInWorld;
	
	bool					isActor;							//mal: this client is an actor, presenting info to the player.
	int						briefingTime;
	
	int						resetState;							//mal: should the bot reset its AI, and how much of a priority is it to reset?	
	int						touchingItemTime;					//mal: set in idItem::OnTouch - is this client touching an item? Useful for finding spies!
	int						friendsInArea;						//mal: set in idBotAI::RunFrame
	int						enemiesInArea;						//mal: set in idBotAI::RunFrame
	int						disguisedClient;					//mal: set in idPlayer::Think - client bot is disguised to look like.

	int						tkReviveTime;

	int						lastOwnedVehicleSpawnID;
	int						lastOwnedVehicleTime;
	
	int						classChargeUsed;					//mal: set in idPlayer::Think -
	int						supplyChargeUsed;					//mal: set in idPlayer::Think - supply packs
	int						bombChargeUsed;						//mal: set in idPlayer::Think - HE charges
	int						fireSupportChargedUsed;				//mal: set in idPlayer::Think - arty and airstrikes
	int						deviceChargeUsed;					//mal: set in idPlayer::Think - landmines, etc.
	int						deployChargeUsed;					//mal: set in idPlayer::Think - deployables.

	int						mountedGPMGEntNum;

	int						escortSpawnID;
	int						escortRequestTime;

	int						lastRoadKillTime;

	int						spawnID;

	int						spawnHostEntNum;

	int						killTargetNum;						//mal: keep track of entities that have been marked for death
	int						killTargetSpawnID;
	int						killTargetUpdateTime;
	bool					killTargetNeedsChat;

	int						pickupRequestTime;
	int						pickupTargetSpawnID;
	int						commandRequestTime;
	bool					commandRequestChatSent;

	int						spawnHostTargetSpawnID;
	
	int						repairTargetNum;					//mal: keep track of entities that have been marked for repair
	int						repairTargetSpawnID;
	int						repairTargetUpdateTime;
	bool					repairTargetNeedsChat;

	int						deployDelayTime;					//mal: how long til we can try to drop a new deployable.

	int						targetLockEntNum;					//mal: set in idPlayer::SetTargetEntity - this will be set whether client has target lock or not.
	int						spectatorCounter;
	int						targetLockTime;
	int						spawnTime;							//mal: set in idPlayer::SpawnToPoint
	int						lastKilledTime;						//mal: set in idPlayer::Think - when client last died.
	int						health;								//mal: set in idPlayer::Think
	int						maxHealth;							//mal: set in idPlayer::Think
	int						lastAttacker;						//mal: set in idPlayer::Pain - who last attacked this client
	int						lastAttackerTime;					//mal: set in idPlayer::Pain - when this client was last attacked
	int						lastAttackClient;					//mal: set in idPlayer::Pain - who we last attacked
	int						lastAttackClientTime;				//mal: set in idPlayer::Pain - when we last attacked them
	int						lastAttackedEntity;					//mal: last entity ( deployables, etc ), we attacked.
	int						lastAttackedEntityTime;				//mal: last entity attacked time.
	int						areaNum;							//mal: set in idBotThread::RunFrame - what AAS area this client is in.
	int						areaNumVehicle;
	int						invulnerableEndTime;				//mal: set in idPlayer::Think - is this client invulnerable ( i.e. just spawned)?	
	int						crouchCounter;						//mal: set in idPlayer::Think - how long has this player been crouching.
	int						killCounter;						//mal: cycles thru the list of "kills".
	int						favoriteKill;						//mal: set in idPlayer::UpdatePlayerKills - the client we love to kill!
	int						chatDelay;
	int						covertWarningTime;
	int						lastChatTime[ MAX_CHATS ];			//mal: when did this client last chat 
	int						lastThanksTime;
	int						lastClassChangeTime;
	int						lastWeapChangedTime;
	int						myHero;								//mal: client who gave this bot health/ammo recently.
	int						mySavior;							//mal: client who revived us recently
	int						killsSinceSpawn;					//mal: how many ppl have I killed since being spawned?
	int						backPedalTime;
	int						missionEntNum;						//mal: entity number of the entity we have a mission for
	int						supplyCrateRequestTime;				//mal: when this client last requested a supply crate.
	int						lastShieldDroppedTime;
	float					xySpeed;							//mal: set in idPlayer::Think
	clientProxyInfo_t		proxyInfo;
	weaponInfo_t			weapInfo;							//mal: the state of this clients weapon
	playerTeamTypes_t		team;								//mal: set in idPlayer::SetGameTeam
	playerClassTypes_t		classType;							//mal: set in sdInventory::SetPlayerClass
	playerClassTypes_t		cachedClassType;					
	scriptHandlers_t		scriptHandler;						//mal: used to get certain, hard to get, script only stuff.
	botPostureFlags_t		posture;							//mal: set in idPlayer::Think
	killedPlayersInfo_t		kills[ MAX_KILLS ];	
	supplyPackInfo_t		packs[ MAX_ITEMS ];
	supplyPackInfo_t		supplyCrate;
	gameAbilities_t			abilities;
	forceShieldInfo_t		forceShields[ MAX_SHIELDS ];
	idVec3					altitude;
	idVec3					origin;								//mal: set in idPlayer::Think
	idVec3					oldOrigin;
	idVec3					aasOrigin;
	idVec3					aasVehicleOrigin;
	idVec3					gpmgOrigin;
	idVec3					viewOrigin;							//mal: set in idPlayer::Think - a copy of idPlayer:: origin + eyeOffset
	idAngles				viewAngles;							//mal: set in idPlayer::Think - a copy of idPlayer::viewAngles
	idMat3					viewAxis;							//mal: set in idPlayer::Think - a copy of idPlayer::viewAxis
	idMat3					bodyAxis;
	idBounds				localBounds;						//mal: set in idPlayer::Think - a copy of the players bbox
	idBounds				absBounds;							//mal: set in idPlayer::Think - a copy of the players bbox
};

struct botCommands_t {
	bool					enterVehicle;		// impulse cmd
	bool					exitVehicle;
	bool					reload;				// impulse cmd
	bool					ackJustSpawned;
	bool					ackReset;
	bool					ackSeatChange;
	bool					dropDisguise;

	bool					shoveClient;

	bool					droppingSupplyCrate;

	bool					attack;				// mal: normal cmds 
	bool					zoom; 
	bool					activate;
	bool					activateHeld;		// hold down the activate key
	bool					altAttackOn;
	bool					altAttackOff;

	bool					destroySupplyCrate;

	bool					altFire;

	bool					launchPacks;		// specific to health/ammo packs

	bool					switchAwayFromSniperRifle;

	bool					topHat;

	bool					enterSiegeMode;
	bool					exitSiegeMode;

	bool					honkHorn;

	bool					launchDecoys;
	bool					launchDecoysNow;

	bool					useTankAim;			// if true, use the Titan's aiming code to look at this location.

	bool					isBlocked;
	
	bool					lookUp;				// these 2 are just used by bots resupplying themselves.
	bool					lookDown;

	bool					throwNade;			// looks up just a bit if bot throws nade/airstrike/violater.
	
	bool					suicide;			// bots wants to end it all.

	bool					constantFire;		// we want the bot to fire constantly

	bool					hasNoGoals;			// a special flag that lets the waypointer know the bot has no goals yet, so the bots are just gonna take a smoke break.

	bool					hasMedicInFOV;		// if dead, does this client have a medic in their view?

	bool					becomeDriver;
	bool					becomeGunner;

	bool					godMode;

	bool					switchVehicleWeap;

	bool					actorBriefedPlayer;
	bool					actorSurrenderStatus;
	bool					actorIsBriefingPlayer;
	bool					actorHasEnteredActorNode;
};

struct botDeployInfo_t {
	int						deployableType;
	int						actionNumber;		 //mal: for debugging.
	idVec3					location;
	idVec3					aimPoint;
};

struct botLocationRemap_t {
	idBox					bbox;
	idVec3					target;
};

struct botAreaChange_t {
	idBounds				bounds;
	int						areaFlags;
	int						travelFlags;
	bool					set;
};

struct botReachChange_t {
	idStr					name;
	int						travelFlags;
	bool					set;
};

struct botAIOutput_t {
	botTurnTypes_t			turnType;						//mal: should the bot turn to its viewAngle goal right away, aim, or just move to angles slowly?

	botChatTypes_t			desiredChat;					//mal: what the bot would like to be saying
	bool					desiredChatForced;
	int						actionEntityNum;				//mal: entity the bot is trying to interact with - could be a player, flag, vehicle, etc.
	int						actionEntitySpawnID;

	int						ackRepairForClient;
	int						ackKillForClient;

	int						tkReviveTime;

	int						decayObstacleSpawnID;

	botDeployInfo_t			deployInfo;

	idVec3					moveGoal;						//mal: area the bot is heading to.
	idVec3					moveGoal2;						//mal: for jumps, hal strafing, etc - where its heading after it reaches moveGoal.

	botViewTypes_t			viewType;						// type of bot view to use, defines which of the variables below are actually used
	int						viewEntityNum;					//mal: entity the bot is trying to look at - could be vehicle, a flag, an enemy, etc.
	idAngles				moveViewAngles;					//mal: where the bot would like to be looking - can be independent of movement direction.
	idVec3					moveViewOrigin;					//mal: only used for vehicles - a point in space the bot wants to look at. The vehicle itself will find the angles.

	botCommands_t			botCmds;						//mal: the user cmds the bot would like to execute this frame.

	weaponBankTypes_t		idealWeaponSlot;				//mal: the weapon this bot would like to have in its hands.
	playerWeaponTypes_t		idealWeaponNum;					// the specific weapon the bot wants to use.

	botMoveFlags_t			moveFlag;					    //mal: crouch, sprint, run, walk, etc.
	
	botMoveTypes_t			specialMoveType;
	botMoveTypes_t			moveType;						//mal: mostly used for combat, but can be used for jumping, etc.

	botDebugInfo_t			debugInfo;
};

struct sharedWorldState_t {
	sdSafeArray< clientInfo_t, MAX_CLIENTS >			clientInfo;			//mal: store info for all the clients.
	sdSafeArray< playerBodiesInfo_t, MAX_PLAYERBODIES >	playerBodies;	// store info about all the dead bodies in the world.
	sdSafeArray< spawnHostInfo_t, MAX_SPAWNHOSTS >		spawnHosts;
	gameLocalInfo_t			gameLocalInfo;						// store info about the current game
	botGoalInfo_t			botGoalInfo;
	sdSafeArray< proxyInfo_t, MAX_VEHICLES >			vehicleInfo;
	sdSafeArray< deployableInfo_t, MAX_DEPLOYABLES >	deployableInfo;
	sdSafeArray< smokeBombInfo_t, MAX_CLIENTS >			smokeGrenades;
	sdSafeArray< plantedChargeInfo_t, MAX_CLIENT_CHARGES >	chargeInfo;	//mal: its possible for one client to 2 charges in the world.
	sdSafeArray< stroyBombInfo_t, MAX_STROYBOMBS >		stroyBombs;
};

struct sharedOutputState_t {
	sdSafeArray< botAIOutput_t, MAX_CLIENTS > botOutput;
};

class idBotThreadData {
public:							// the following routines are called from the game thread
		
								idBotThreadData();
								~idBotThreadData();

	void						LoadMap( const char* mapName, int randSeed );
	void						InitAAS( const idMapFile *mapFile );

	int							NumAAS() const;
	idAAS *						GetAAS( int num ) const;
	idAAS *						GetAAS( const char *name ) const;
	void						ShowAASStats() const;

	int							FindActionByName( const char *actionName );
	int							FindActionByOrigin( const botActionGoals_t actionType, const idVec3& origin, bool activeOnly );
	int							FindRouteByName( const char *routeName );

	void						CheckCrossHairInfo( idEntity *bot, sdCrosshairInfo &botCrossHairInfo );
	int							FindDeclIndexForDeployable( const playerTeamTypes_t team, int deployableNum );
	bool						RequestDeployableAtLocation( int clientNum, bool& needPause );

	void						InitClientInfo( int clientNum, bool resetAll, bool leaving );
	void						UpdateState();
	void						DebugOutput();

	void						LoadBotNames();
	void						LoadTrainingBotNames();

	void						TrainingThink();

	void						CheckCurrentChatRequests();

	void						FindBotToBeActor();
	bool						MapHasAnActor();

	bool						DestroyClientDeployable( int clientNum );

	void						CheckTargetLockInfo( int clientNum, float& boundsScale );

	int							GetBotSkill() { return GetBotWorldState()->gameLocalInfo.botSkill; }
	int							GetBotAimSkill() { return GetBotWorldState()->gameLocalInfo.botAimSkill; }

	void						UpdateClientAbilities( int clientNum, int rankLevel );

	void						VehicleRouteThink( bool setup, int& routeActionNumber, const idVec3& vehicleOrg );

	void						Clear( bool clearAll );

	void						FindCurrentWeaponInVehicle( idPlayer *player, sdTransport* transport ); //mal: find which weapon is currently available to the bot in a vehicle position.

	void						SetCurrentGameWorldState();
	void						SetCurrentGameOutputState();

	static void					Cmd_KillAllBots_f( const idCmdArgs &args );
	static void					Cmd_ResetAllBots_f( const idCmdArgs & args );

	void						ManageBotClassesOnSmallServer();

	bool						ServerHasHumans();
	bool						ActionIsLinkedToAction( int curActionNum, int testActionNum );
	bool						CheckActionIsValid( int actionNumber );

	sharedWorldState_t *		GetGameWorldState() { return gameWorldState; }
	const sharedOutputState_t *	GetGameOutputState() const { return gameOutputState; }

	void						VOChat( const botChatTypes_t chatType, int clientNum, bool forceChat );
	void						UpdateChats( int clientNum, const sdDeclQuickChat* quickChat );

	void 						LoadActions( const idMapFile *mapFile );
	void						LoadRoutes( const idMapFile *mapFile );
	void						Init();
	void						ResetClientsInfo();

	void						SetupBotInfo( int clientNum );
	float						GetVehicleTargetOffset( const playerVehicleTypes_t vehicleType );

	void						ChangeBotName( idPlayer* bot );
	static bool					FindRandomBotName( int clientNum, idStr& botName );

	int							GetNumClassOnTeam( const playerTeamTypes_t playerTeam, const playerClassTypes_t classType );
	int							GetNumBotsInServer( const playerTeamTypes_t playerTeam );
	bool						TeamHasHumans( const playerTeamTypes_t playerTeam );
	void						ResetBotAI( const playerTeamTypes_t playerTeam );
	int							GetNumClientsOnTeam( const playerTeamTypes_t playerTeam );
	int							GetNumBotsOnTeam( const playerTeamTypes_t playerTeam );
	int							GetNumWeaponsOnTeam( const playerTeamTypes_t playerTeam, const playerWeaponTypes_t weapType );
	int							GetNumTeamBotsSpawningAtRearBase( const playerTeamTypes_t playerTeam );

	void						CheckBotSpawnLocation( idPlayer* player );

	void						DeactivateBadMapActions();

	int							FindHeavyVehicleNearLocation( idPlayer* player, const idVec3& org, float range );

	int							FindDeliverActionNumber();

	bool						IsThreadingEnabled() const { return threadingEnabled; }
	int							GetThreadMinFrameDelay() const { return threadMinFrameDelay; }
	int							GetThreadMaxFrameDelay() const { return threadMaxFrameDelay; }

	bool						AllowDebugData() const { return !threadingEnabled && bot_debug.GetBool(); }
	bool						PlayerIsBeingBriefed();

	int							FindActionByTypeForLocation( const idVec3& loc, const botActionGoals_t& obj, const playerTeamTypes_t& team );

	bool						IsActionsLoaded() { return actionsLoaded; }
	bool						IsForwardSpawnsChecked() { return forwardSpawnsChecked; }

	bool						ClientIsValid( int clientNum );

	void						ResetBotsInfo();

	void						ClearClientBoundEntities( int clientNum );

	void						DrawNumbers();

	void						DrawRearSpawns();

	void						DrawActionPaths();

	void						DrawActions(); //mal: displays the map's actions - useful for debugging!
	void						DrawDefuseHints();
	void						DrawDynamicObstacles();
	void						DrawActionNumber( int actionNumber, const idMat3& viewAxis, bool drawAllInfo );
	
	int							GetBotFPS() const { return botFPS; }
	botDebugInfo_t				GetBotDebugInfo( int clientNum );

	void						RemoveBot( int entityNumber );

	bool						CheckIfClientsInGameYet();

	void						EnableArea( const idBounds &bounds, int areaFlags, int team, bool enable );
	void						EnableReachability( const char *name, int team, bool enable );

	bool						BuildObstacleList( idObstacleAvoidance &obstacleAvoidance, const idVec3 &origin, const int areaNum, bool inVehicle ) const;

public:							// the following routines are called from the bot thread

	void						RunFrame();
	void						CheckBotClassSpread();
	void						DeleteBots();
	void						ApplyPendingAreaChanges();
	void						ApplyPendingReachabilityChanges();
	void						DisableAASAreaInLocation( int aasType, const idVec3& location );

	int							GetDeployableActionNumber( idList< int >& deployableActions, const idVec3& deployableOrigin, const playerTeamTypes_t deployableTeam, int deployableType );

	void						OnPlayerKilled( int clientNum );

	const idClipModel*			GetVehicleTestBounds( const playerVehicleTypes_t vehicleType );

	void						SetCurrentBotWorldState();
	void						SetCurrentBotOutputState();

	const sharedWorldState_t *	GetBotWorldState() const { return botWorldState; }
	sharedOutputState_t *		GetBotOutputState() { return botOutputState; }

	void						Printf( const char *fmt, ... );
	void						Warning( const char *fmt, ... );

	// aasType is either AAS_PLAYER or AAS_VEHICLE
	int							Nav_GetAreaNum( int aasType, const idVec3 & origin );
	int							Nav_GetAreaNumAndOrigin( int aasType, const idVec3& origin, idVec3& aasOrigin );
	bool						Nav_IsDirectPath( int aasType, const playerTeamTypes_t& playerTeam, int areaNum, const idVec3 & start, const idVec3 & end );

	bool						RemapLocation( idVec3 &origin ) const;

	bool						EntityIsExplosiveCharge( int entNum, bool armedOnly, const playerTeamTypes_t& ignoreTeam );

public:
	idList< idAAS* >			aasList;					// area system
	idStrList					aasNames;
	idClip *					clip;
	idRandom					random;

	idList<idBotActions*>		botActions;	
	idList<idBotRoutes*>		botRoutes;
	idList<idBotObstacle*>		botObstacles;
	idList<botLocationRemap_t*>	botLocationRemap;

	idBotNodeGraph				botVehicleNodes;
	
	idHashIndex					actionHash;					// hash table to quickly find actions by name
	idHashIndex					routeHash;					// hash table to quickly find routes by name

	idBotAI *					bots[ MAX_CLIENTS ];

	idStrList					botNames;

	idStrList					trainingGDFBotNames;
	idStrList					trainingSTROGGBotNames;

	idList< int >				mcpRoutes;

	actorMissionInfo_t			actorMissionInfo;

	bool						teamsSwapedRecently;

	int							ignoreActionNumber;

private:
	bool						threadingEnabled;
	bool						actionsLoaded;
	bool						forwardSpawnsChecked;

	sharedWorldState_t			worldStateBuffer[BOT_THREAD_BUFFER_SIZE];
	sharedOutputState_t			outputStateBuffer[BOT_THREAD_BUFFER_SIZE];

	int							currentWorldState;
	int							currentOutputState;

	sharedWorldState_t *		gameWorldState;
	sharedOutputState_t *		gameOutputState;

	sharedWorldState_t *		botWorldState;
	sharedOutputState_t *		botOutputState;

	idBotAI *					removedBotAIs;
	idList<botAreaChange_t>		pendingAreaChanges;
	idList<botReachChange_t>	pendingReachChanges;

	idClipModel*				personalVehicleClipModel;		// icarus and husky
	idClipModel*				genericVehicleClipModel;		// everything but goliath
	idClipModel*				goliathVehicleClipModel;		// 
	
	int							threadMinFrameDelay;
	int							threadMaxFrameDelay;
	int							botFPS;

	int							nextBotClassUpdateTime;
	int							packUpdateTime;

	int							nextDeployableUpdateTime;

	int							lastCmdDeclinedChatTime;
	int							nextChatUpdateTime;
	int							lastWeapChangedTime;

	bool						checkedLastMapStageCovertWeapons;

private:
	void						AddActionToHash( const char *actionName, int actionNum );
	void						AddRouteToHash( const char *routeName, int routeNum );

	void						DynamicEntity_Think(); //mal: run thru all the tracked dynamic entities, and update their info.
};

extern idBotThreadData			botThreadData;

#endif /* !__BOTTHREADDATA_H__ */
