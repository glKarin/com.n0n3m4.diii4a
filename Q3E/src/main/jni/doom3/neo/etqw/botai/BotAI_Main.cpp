// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idEntity *GetGameEntity

A thread safe way for us to get a pointer to an entity - used for traces and collision detections.
NOTE: we can't actually access any of the entity's info in the Bot AI, just the pointer. If you need
certain info, add it to the "botThreadData" client/entity info arrays.
================
*/
idEntity *GetGameEntity( int entityNumber ) {
	if ( entityNumber == -1 ) {
		return NULL;
	}

	return gameLocal.entities[ entityNumber ];
}

/*
================
idBotAI::idBotAI
================
*/
idBotAI::idBotAI() {
	ClearBotState();
}

/*
================
idBotAI::~idBotAI
================
*/
idBotAI::~idBotAI() {
}

/*
================
idBotAI::Think

This is the start of the bot's thinking process, called
every game frame by the server.
================
*/
void idBotAI::Think() {
	botInfo = &botThreadData.GetBotWorldState()->clientInfo[ botNum ];
	botUcmd = &botThreadData.GetBotOutputState()->botOutput[ botNum ];
	botWorld = botThreadData.GetBotWorldState();
	botVehicleInfo = GetBotVehicleInfo( botInfo->proxyInfo.entNum );

	BotAI_ResetUcmd();

	if ( !BotAI_CheckThinkState() ) {
		return;
	}

	BotAI_UpdateStateInfo();

	RunDebugChecks();

	UpdateAAS();

	if ( botVehicleInfo != NULL ) {
		VThink(); //mal: if bot is in a vehicle, run vehicle specific AI.
		return;
	}

	if ( ROOT_AI_NODE == NULL ) {		
		Bot_ResetState( false, false );
        ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
	}

	if ( botAAS.blockedByObstacleCounterOnFoot > MAX_FRAMES_BLOCKED_BY_OBSTACLE_ON_FOOT ) {
		Bot_ResetState( true, true );
		botAAS.blockedByObstacleCounterOnFoot = 0;
		ROOT_AI_NODE = &idBotAI::Run_LTG_Node;
	}

    CallFuncPtr( ROOT_AI_NODE ); //mal: run the bot's current think node

	Bot_Input();

	CheckBotStuckState();
}


/*
================
idBotAI::ResetUcmd

Clears out the Bot's user cmd structure at the start of every frame, so that we
can build a fresh one each bot think.
================
*/
void idBotAI::BotAI_ResetUcmd () {
	Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE ); //mal: clear out the move state at first!
//	botUcmd->viewType = VIEW_ORIGIN;
	botUcmd->turnType = SMOOTH_TURN;
	botUcmd->viewEntityNum = -1;
	botUcmd->actionEntityNum = -1;
	botUcmd->actionEntitySpawnID = -1;
	botUcmd->botCmds.ackJustSpawned = false;
	botUcmd->botCmds.ackReset = false;
	botUcmd->botCmds.activate = false;
	botUcmd->botCmds.ackSeatChange = false;
	botUcmd->botCmds.activateHeld = false;
	botUcmd->botCmds.altAttackOff = false;
	botUcmd->botCmds.honkHorn = false;
	botUcmd->botCmds.isBlocked = false;
	botUcmd->botCmds.launchDecoys = false;
	botUcmd->botCmds.launchDecoysNow = false;
	botUcmd->botCmds.altAttackOn = false;
	botUcmd->botCmds.godMode = false;
	botUcmd->botCmds.attack = false;
	botUcmd->botCmds.reload = false;
	botUcmd->botCmds.enterVehicle = false;
	botUcmd->botCmds.exitVehicle = false;
	botUcmd->botCmds.zoom = false;
	botUcmd->botCmds.lookDown = false;
	botUcmd->botCmds.lookUp = false;
	botUcmd->botCmds.constantFire = false;
	botUcmd->botCmds.throwNade = false;
	botUcmd->botCmds.hasNoGoals = false;
	botUcmd->botCmds.switchVehicleWeap = false;
	botUcmd->botCmds.dropDisguise = false;
	botUcmd->botCmds.shoveClient = false;
	botUcmd->botCmds.droppingSupplyCrate = false;
	botUcmd->botCmds.destroySupplyCrate = false;
	botUcmd->botCmds.hasMedicInFOV = false;
	botUcmd->botCmds.becomeDriver = false;
	botUcmd->botCmds.becomeGunner = false;
	botUcmd->botCmds.enterSiegeMode = false;
	botUcmd->botCmds.exitSiegeMode = false;
	botUcmd->desiredChat = NULL_CHAT;
	botUcmd->desiredChatForced = false;
	botUcmd->botCmds.useTankAim = false;
	botUcmd->botCmds.altFire = false;
	botUcmd->deployInfo.deployableType = NULL_DEPLOYABLE;
	botUcmd->deployInfo.location.Zero();
	botUcmd->deployInfo.actionNumber = ACTION_NULL;
	botUcmd->ackKillForClient = -1;
	botUcmd->ackRepairForClient = -1;
	isStrafeJumping = false;
	ignoreWeapChange = false;
	botAAS.triedToMoveThisFrame = false;
	weaponLocked = false;
	botUcmd->botCmds.launchPacks = false;
	botUcmd->botCmds.switchAwayFromSniperRifle = false;
	botUcmd->botCmds.topHat = false;
	botUcmd->tkReviveTime = 0;
	botUcmd->decayObstacleSpawnID = -1;
	botUcmd->botCmds.actorBriefedPlayer = false;
	botUcmd->botCmds.actorSurrenderStatus = false;
	botUcmd->botCmds.actorIsBriefingPlayer = false;
	botUcmd->botCmds.actorHasEnteredActorNode = false;

	botUcmd->specialMoveType = NULLMOVETYPE;
}

/*
================
idBotAI::BotAI_UpdateStateInfo

Updates some important info that the bots will need every frame.
================
*/
void idBotAI::BotAI_UpdateStateInfo () {
	const_cast<int &>( botInfo->friendsInArea ) = ClientsInArea( botNum, botInfo->origin, 1200.0f, botInfo->team, NOCLASS, false, false, false, true, false );
	const_cast<int &>( botInfo->enemiesInArea ) = ClientsInArea( botNum, botInfo->origin, 1500.0f, ( botInfo->team == GDF ) ? STROGG : GDF, NOCLASS, false, false, false, false, false );
}

/*
================
idBotAI::Bot_Input

A housekeeping function, called at the end of every bot think.
Sends the bot's user cmds to the server, updates bots cmd timers, etc.
================
*/
void idBotAI::Bot_Input() {

	//mal_TODO: add more stuff here as needed!

	if ( aiState == LTG && !botWorld->gameLocalInfo.inWarmup ) {
		botUcmd->botCmds.reload = true;			
	} //mal: just auto-reload in LTG.

	if ( weaponLocked ) {
		botUcmd->idealWeaponSlot = NO_WEAPON;	
		botUcmd->idealWeaponNum = NULL_WEAP;
	} else {
		botUcmd->idealWeaponSlot = botIdealWeapSlot;	
		botUcmd->idealWeaponNum = botIdealWeapNum;
	}

	if ( !hasGreetedServer ) {
		if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
			Bot_AddDelayedChat( botNum, HELLO, botThreadData.random.RandomInt( 3 ) ); //mal: say a nice hello to the folks on the server.
		}
		hasGreetedServer = true; //mal: never do this again. 
	}

	Bot_CheckDelayedChats();
}

/*
================
idBotAI::Bot_ResetState

Totally resets the bot's AI states and important values. Used in the case where the bot needs to "forget"
what it was doing, and needs to find a new goal, and on every spawn into the world (and revive).
================
*/
void idBotAI::Bot_ResetState( bool resetEnemy, bool resetStack ) {

	if ( resetEnemy ) {
        Bot_ResetEnemy();
	}

	if ( resetStack ) {
		Bot_ClearAIStack();
	}

	classAbilityDelay = -1;

	wantsVehicle = false;

	chasingEnemy = false;

	ignoreSpyTime = 0;

	vehicleGunnerTime = 0;
	vehicleDriverTime = 0;
	vehicleUpdateTime = 0;

	supplySelfTime = 0;

	resetEnemy = false;

	shotIsBlockedCounter = 0;

	vehicleEnemyWasInheritedFromFootCombat = false;

	nodeTimeOut = 0;
	newPathTime = 0;

	combatMoveTime = -1;
	combatMoveFailedCount = 0;

//mal: save off what the last action we did was, so we can make sure we don't repeat ourselves!
	actionNum = ACTION_NULL;

	ignoreDangersTime = 0;

	useNadeOnEnemy		= false;	// does this bot want to use a nade on its enemy?
	useAircanOnEnemy	= false;

	ResetRandomLook();

	framesVehicleStuck = 0;

	isStrafeJumping = false;

	stayInPosition = false;

	badMoveTime = 0;
	testFireShot = false;

	ResetRandomLook();

	routeNode = NULL;
	vehicleNode = NULL;

	skipStrafeJumpTime = 0;
	pistolTime = 0;

	COMBAT_MOVEMENT_STATE = NULL;
	VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
	VEHICLE_COMBAT_AI_SUB_NODE = NULL;

	botIdealWeapSlot = GUN;

	botIdealWeapNum = NULL_WEAP;

	aiState = LTG;

	nbgTargetType = NOTYPE;
	ltgUseVehicle = false;

	botExitTime = 0;

	ROOT_AI_NODE = NULL;
	V_ROOT_AI_NODE = NULL;
	V_LTG_AI_SUB_NODE = NULL;
	NBG_AI_SUB_NODE = NULL;
	LTG_AI_SUB_NODE = NULL;
    COMBAT_AI_SUB_NODE = NULL;

	combatNBGType = NO_COMBAT_TYPE;

	botVehiclePathList.Clear();
}

/*
================
idBotAI::Bot_ClearAIStack
================
*/
void idBotAI::Bot_ClearAIStack() {
	AIStack.STACK_AI_NODE = false;
	AIStack.VEHICLE_STACK_AI_NODE = false;
	AIStack.stackClientNum = -1;
	AIStack.stackClientSpawnID = -1;
	AIStack.stackEntNum = -1;
	AIStack.stackTimeLimit = -1;
	AIStack.stackActionNum = ACTION_NULL;
	AIStack.stackLTGType = NO_LTG;
	AIStack.vehicleStackLTGType = NO_VEHICLE_LTG;
	AIStack.isPriority = false;
	AIStack.useVehicle = false;
	AIStack.routeNode = NULL;
}

/*
================
idBotAI::Bot_ClearVehicleOffAIStack
================
*/
void idBotAI::Bot_ClearVehicleOffAIStack() {
	AIStack.useVehicle = false;
}

/*
================
idBotAI::PushAINodeOntoStack
================
*/
void idBotAI::PushAINodeOntoStack( int clientNum, int entNum, int actionNumber, int timeLimit, bool isPriority, bool useVehicle, bool useRoute ) {
	AIStack.STACK_AI_NODE = LTG_AI_SUB_NODE;
	AIStack.VEHICLE_STACK_AI_NODE = V_LTG_AI_SUB_NODE;
	AIStack.stackLTGType = ltgType;
	AIStack.vehicleStackLTGType = vLTGType;

	if ( ClientIsValid( clientNum, -1 ) ) {
		AIStack.stackClientNum = clientNum;
		AIStack.stackClientSpawnID = botWorld->clientInfo[ clientNum ].spawnID;
	}

	AIStack.stackEntNum = entNum;
	AIStack.stackTimeLimit = botWorld->gameLocalInfo.time + timeLimit;
	AIStack.stackActionNum = actionNumber;
	AIStack.isPriority = isPriority;
	AIStack.useVehicle = useVehicle;

	if ( useRoute ) {
		if ( routeNode != NULL ) {
			AIStack.routeNode = routeNode;
		} else {
            assert( false );
		}
	} else {
		AIStack.routeNode = NULL;
	}
}

/*
================
idBotAI::PopAINodeOffStack
================
*/
bool idBotAI::PopAINodeOffStack() {

	if ( AIStack.STACK_AI_NODE == false ) {
		return false;
	}

	LTG_AI_SUB_NODE = AIStack.STACK_AI_NODE;
	V_LTG_AI_SUB_NODE = AIStack.VEHICLE_STACK_AI_NODE;
	
	if ( AIStack.stackClientNum != -1 ) {
		ltgTarget = AIStack.stackClientNum;
		ltgTargetSpawnID = AIStack.stackClientSpawnID;
	} else if ( AIStack.stackEntNum != -1 ) {
		ltgTarget = AIStack.stackEntNum;
	} else {
		ltgTarget = -1;
	}

	ltgUseVehicle = AIStack.useVehicle;

	routeNode = AIStack.routeNode;

	ltgTime = botWorld->gameLocalInfo.time + AIStack.stackTimeLimit;
	actionNum = AIStack.stackActionNum;

	if ( AIStack.stackLTGType != FOLLOW_TEAMMATE && AIStack.stackLTGType != FOLLOW_TEAMMATE_BY_REQUEST ) {
        Bot_ClearAIStack(); //mal: only use this once - else bot may get stuck in loop - UNLESS following mate!
	}

	return true;
}

/*
================
idBotAI::Bot_ResetEnemy
================
*/
void idBotAI::Bot_ResetEnemy() {
    numVisEnemies = 0;
	enemy = -1;
	fastAwareness = false;
	timeTilAttackEnemy = -1;
	enemyInfo.enemyDist = -1;
	enemyInfo.enemyLastVisTime = -1;
	enemyInfo.enemyVisible = false;
	enemyInfo.enemy_LS_Pos = vec3_zero;
	COMBAT_AI_SUB_NODE = NULL;
	VEHICLE_COMBAT_AI_SUB_NODE = NULL;
	hammerTime = false;
	hammerVehicle = false;
	hammerLocation = vec3_zero;
	COMBAT_MOVEMENT_STATE = NULL;
	VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
	combatMoveType = COMBAT_MOVE_NULL;
	combatMoveTime = -1;
	combatMoveFailedCount = 0;
	chasingEnemy = false;
	chaseEnemy = false;
	vehicleGunnerTime = 0;
	vehicleDriverTime = 0;
	vehicleUpdateTime = 0;
	shotIsBlockedCounter = 0;
	resetEnemy = false;
}

/*
================
idBotAI::UpdateBotsInformation

Anytime the bot spawns fresh into the world, start with a clean slate.
This could be setup to detect if the bot changes its weapon/class/team, so that we can support
that sort of thing in the scripts.
================
*/
void idBotAI::UpdateBotsInformation( const playerClassTypes_t playerClass, const playerTeamTypes_t playerTeam, bool botInit ) {

	if ( !botInit ) {
        Bot_ResetState( true, true );
	}

//mal: dont do this if haven't had time to get all the bot's info setup yet!
	if ( playerClass == NOCLASS || playerTeam == NOTEAM ) {
		return;
	}

	LTG_CHECK_FOR_CLASS_NBG = NULL;
	NBG_CHECK_FOR_CLASS_NBG = NULL;

//mal: check what kind of class this bot is, and set its short term goal checks accordingly....
	if ( playerClass == MEDIC ) {
		LTG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_MedicCheckNBGState_InLTG;
		NBG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_MedicCheckNBGState_InNBG;
	}

	if ( playerClass == FIELDOPS ) { //mal: strogg medics will do the same job as Fops.
		LTG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_FopsCheckNBGState;
		NBG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_FopsCheckNBGState;
	}

	if ( playerClass == ENGINEER ) {
		LTG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_EngCheckNBGState;
		NBG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_EngCheckNBGState;
	}

	if ( playerClass == COVERTOPS ) {
		LTG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_CovertOpsCheckNBGState;
		NBG_CHECK_FOR_CLASS_NBG = &idBotAI::Bot_CovertOpsCheckNBGState;
	}

//mal_TODO: add short term goal checks for all the classes (at least the ones who need them) - also setup team specific stuff here!


	if ( !botInit ) { //must be run very last!
        botUcmd->botCmds.ackJustSpawned = true;
	}
}

/*
===============
idBotAI::InitAAS
===============
*/
void idBotAI::InitAAS() {

	botAAS.aas = botThreadData.GetAAS( AAS_PLAYER ); //mal: start out with player AAS at spawn
	botAAS.aasType = AAS_PLAYER;

	if ( !botAAS.aas ) {
		return;
		// give some kind of error msg here?
	}
}

/*
===============
idBotAI::Spawn

just setups all of the bots basic vars/strucs with default values.
===============
*/
void idBotAI::Spawn( int playerClass, int playerTeam ) {

	InitAAS();

	ClearBotUcmd( botNum );	

	playerTeamTypes_t botTeam = ( playerTeamTypes_t ) playerTeam;
	playerClassTypes_t botClass = ( playerClassTypes_t ) playerClass;

	UpdateBotsInformation( botClass, botTeam, true );
}

/*
================
idBotAI::CheckBotStuckState
================
*/
void idBotAI::CheckBotStuckState() {

	if ( !botInfo->hasGroundContact && botInfo->invulnerableEndTime > botWorld->gameLocalInfo.time ) {
		framesStuck = 0;
		framesFlipped = 0;
		vehicleFramesStuck = 0;
		overallFramesStuck = 0;
        return; //mal: if we're spawning in, dont count a lack of a valid AAS area against us
	}

	if ( stuckRandomMoveTime > botWorld->gameLocalInfo.time ) {
		BotAI_ResetUcmd();
		Bot_MoveToGoal( vec3_zero, vec3_zero, SPRINT, RANDOM_DIR_JUMP );
		return;
	}

	if ( !botAAS.triedToMoveThisFrame ) {
		botAAS.blockedByObstacleCounterOnFoot = 0;
	}

	if ( botInfo->areaNum == 0 ) {
		noAASCounter++;

		if ( noAASCounter >= 100 ) {
			if ( botVehicleInfo == NULL ) {
                botUcmd->botCmds.suicide = true;
				noAASCounter = 0;
			} else {
                Bot_ExitVehicle();
			}
		}

		return;
	} else {
		noAASCounter = 0;
	}

	if ( moveErrorCounter.moveErrorTime < botWorld->gameLocalInfo.time ) {
		moveErrorCounter.moveErrorTime = botWorld->gameLocalInfo.time + 5000;
		if ( moveErrorCounter.moveErrorCount >= MAX_MOVE_ERRORS ) {
			moveErrorCounter.moveErrorCount = 0;
			botUcmd->botCmds.suicide = true;
		} else {
			moveErrorCounter.moveErrorCount = 0;
		}
	}

	if ( botVehicleInfo == NULL ) {
		idVec3 vec = botInfo->origin - botInfo->oldOrigin;

		if ( vec.LengthSqr() < Square( 10.0f ) && botInfo->isTryingToMove ) {
			framesStuck++;

//			if ( framesStuck > MAX_FRAMES_BLOCKED_BY_OBSTACLE_ON_FOOT ) {
//				botAAS.blockedByObstacleCounterOnFoot = MAX_FRAMES_BLOCKED_BY_OBSTACLE_ON_FOOT + 1;
//				framesStuck = 0;
//				stuckRandomMoveTime = botWorld->gameLocalInfo.time + 500;
//				overallFramesStuck++;
//			}

			if ( framesStuck >= 300 ) {
				stuckRandomMoveTime = botWorld->gameLocalInfo.time + 500;
				framesStuck = 0;
				overallFramesStuck++;
			}

			if ( overallFramesStuck > 6 && botWorld->gameLocalInfo.botsCanSuicide ) {
				botUcmd->botCmds.suicide = true;
			}

			return;
		}
	}

	framesStuck = 0;
	overallFramesStuck = 0;

	if ( botVehicleInfo != NULL ) {
		if ( botVehicleInfo->isFlipped ) {
			framesFlipped++;

			if ( framesFlipped >= 100 ) { //mal: give us time to recover, just in case we can.
                Bot_ExitVehicle( false);
			}
			return;		
		}

		if ( botVehicleInfo->isCareening && botVehicleInfo->isAirborneVehicle ) { //mal: out NOW! This thing is lost!
			Bot_ExitVehicle();
		}

		if ( botVehicleInfo->inWater && !( botVehicleInfo->flags & WATER ) ) {
			Bot_ExitVehicle( false );
		}
	}

	framesFlipped = 0;

/*
	if ( botVehicleInfo != NULL ) { //mal: this sucks.
		idVec3 vec = botInfo->origin - botInfo->oldOrigin;
		vec.z = 0.0f;

		if ( vec.LengthSqr() < Square( 10.0f ) && botInfo->isTryingToMove ) {
			vehicleFramesStuck++;

			if ( vehicleFramesStuck >= 300 ) {
				vehicleReverseTime = botWorld->gameLocalInfo.time + 900;
				botUcmd->specialMoveType = REVERSEMOVE;
				vehicleFramesStuck = 0;
			}
		} else {
			vehicleFramesStuck = 0;
		}
	}
	*/
}

/*
================
idBotAI::RunDebugChecks
================
*/
void idBotAI::RunDebugChecks() { 
	SetupDebugHud();

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_breakPoint.GetBool() ) { 
			bot_breakPoint.SetBool( false );
			assert( false ); // stop everything!
		}
	}
}

/*
===============
idBotAI::ResetBotsAI

just resets the AI, but keeps any enemies the bot has!
===============
*/
void idBotAI::ResetBotsAI( bool critical ) {

	if ( !critical ) {
		if ( aiState == LTG && ( ltgType == FOLLOW_TEAMMATE || ltgType == FOLLOW_TEAMMATE_BY_REQUEST ) && Client_IsCriticalForCurrentObj( ltgTarget, -1 ) ) {
			botUcmd->botCmds.ackReset = true;
			return;
		}
	} //mal: if we're escorting someone doing the obj, DON'T reset our AI state!

	bool fastEnemyAwareness = false;

	if ( enemy != -1 ) {
		fastEnemyAwareness = true;
	}

	Bot_ResetState( true, true );

	botUcmd->botCmds.ackReset = true;

	fastAwareness = fastEnemyAwareness; //mal: if we has an enemy when we reset, we'll re-aquire them very quickly.

	lastCheckActionTime = 0; //mal: we won't ignore a 2nd plant msg right after the first one.
}

/*
===============
idBotAI::Bot_AddDelayedChat

sets a chat for the bot to use later in time. Not recommended to be for too long ( 5 - 10 seconds tops ).
Its possible a chat will be dropped, which is fine because a bot's chats aren't considered to be too critical.
Has been extended, so that ANY bot can have their chat state set in the botThread.
===============
*/
void idBotAI::Bot_AddDelayedChat( int botClientNum, const botChatTypes_t chatType, int delayTimeInSecs, bool forceChat ) {
	bool skipChat = false;

	for( int i = 0; i < MAX_DELAYED_CHATS; i++ ) {
		if ( botThreadData.bots[ botClientNum ]->delayedChats[ i ].delayTime != 0 && botThreadData.bots[ botClientNum ]->delayedChats[ i ].delayTime > botWorld->gameLocalInfo.time ) {
			continue;
		} //mal: this chat is currently tagged inuse, so find another.

		for( int j = 0; j < MAX_DELAYED_CHATS; j++ ) {
			if ( botThreadData.bots[ botClientNum ]->delayedChats[ j ].delayTime == 0 || botThreadData.bots[ botClientNum ]->delayedChats[ i ].delayTime < botWorld->gameLocalInfo.time ) {
				continue;
			}

			if ( botThreadData.bots[ botClientNum ]->delayedChats[ j ].chat == chatType ) {
				skipChat = true;
				break;
			}
		}

		if ( skipChat ) {
			break;
		}

		botThreadData.bots[ botClientNum ]->delayedChats[ i ].chat = chatType;
		botThreadData.bots[ botClientNum ]->delayedChats[ i ].delayTime = ( delayTimeInSecs * 1000 ) + botWorld->gameLocalInfo.time;
		botThreadData.bots[ botClientNum ]->delayedChats[ i ].forceChat = forceChat;
		break;
	}
}

/*
===============
idBotAI::Bot_CheckDelayedChats

Checks to see if there are any delayed chats ready to play.
===============
*/
void idBotAI::Bot_CheckDelayedChats() {

	bool hasChat = false;
	int i;
	idVec3 vec;

	for( i = 0; i < MAX_DELAYED_CHATS; i++ ) {

		if ( delayedChats[ i ].delayTime == 0 || delayedChats[ i ].delayTime > botWorld->gameLocalInfo.time ) {
			continue;
		} //mal: this chat is currently tagged released or not ready to be sent, so find another.

		botUcmd->desiredChat = delayedChats[ i ].chat;
		botUcmd->desiredChatForced = delayedChats[ i ].forceChat;
		delayedChats[ i ].delayTime = 0;
		hasChat = true;
		break;
	}

//mal: if we dont have anything important to say, lets see if we should say "Your Welcome!" to someone who thanked us.
	if ( hasChat == false && botInfo->health > 0 && botInfo->lastChatTime[ YOURWELCOME ] < botWorld->gameLocalInfo.time ) {
		if ( ( botInfo->classType == MEDIC || botInfo->classType == FIELDOPS ) && aiState == LTG || aiState == NBG ) {
			for( i = 0; i < MAX_CLIENTS; i++ ) {

				if ( !ClientIsValid( i, -1 ) ) {
					continue;
				}

				if ( i == botNum ) {
					continue;
				}

				const clientInfo_t& playerInfo = botThreadData.GetBotWorldState()->clientInfo[ i ];

				if ( playerInfo.health <= 0 ) { //mal: seems retared to say "Bitte Sehr!" to someones whos' dead!
					continue;
				}

				if ( playerInfo.myHero != botNum ) {
					continue;
				}

				if ( playerInfo.lastThanksTime + 5000 < botWorld->gameLocalInfo.time || playerInfo.lastThanksTime + 1500 > botWorld->gameLocalInfo.time ) {
					continue;
				} 

				vec = playerInfo.origin - botInfo->origin;

				if ( vec.LengthSqr() > Square( 900.0f ) ) {
					continue;
				}

				botUcmd->desiredChat = YOURWELCOME;
			}
		}
	}
}

/*
===============
idBotAI::UpdateAAS

NOTE: The icarus is the only vehicle that will use the player AAS.
===============
*/
void idBotAI::UpdateAAS() {
	if ( botVehicleInfo != NULL ) {
		if ( botAAS.aasType != AAS_VEHICLE ) {
			botAAS.aas = botThreadData.GetAAS( AAS_VEHICLE );
			if ( botAAS.aas == NULL ) {
				botAAS.aas = botThreadData.GetAAS( AAS_PLAYER );
				assert( false ); //mal: this should NEVER happen!
			}
			botAAS.aasType = AAS_VEHICLE;
		}
	} else {
		if ( botAAS.aasType != AAS_PLAYER ) {
			botAAS.aas = botThreadData.GetAAS( AAS_PLAYER );
			botAAS.aasType = AAS_PLAYER;
		}
	}

	botAAS.blockedByObstacleCounterOnFoot = 0;
	botAAS.blockedByObstacleCounterInVehicle = 0;
}

/*
===============
idBotAI::BotAI_CheckThinkState

Should the bot think this frame?
===============
*/
bool idBotAI::BotAI_CheckThinkState() {
	if ( botWorld->gameLocalInfo.botsPaused ) {
		return false;
	}

	if ( botWorld->gameLocalInfo.botsSleep ) {
		return false;
	}

	if ( botInfo->team == NOTEAM || botInfo->classType == NOCLASS ) {
		if ( botThreadData.AllowDebugData() ) {
//			assert( false );
		}
		return false;
	}

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_skipThinkClient.GetInteger() == botNum ) {
			botUcmd->botCmds.godMode = true;
			return false;
		}

		if ( bot_godMode.GetInteger() == botNum ) {
			botUcmd->botCmds.godMode = true;
		}
	}

	if ( botInfo->isTeleporting ) {
		return false;
	}

	if ( botAAS.aas == NULL ) {
		if ( botThreadData.random.RandomInt( 100 ) > 98 ) {
			botUcmd->botCmds.hasNoGoals = true; //mal: pass a warning to the player to let them know we have no goals
		}
		return false;
	}

	if ( !botInfo->hasGroundContact && botInfo->invulnerableEndTime > botWorld->gameLocalInfo.time ) {
		return false; 
	} //mal: dont start thinking  until we've actually touched the ground after spawning in.

	if ( botInfo->justSpawned ) {
		UpdateBotsInformation( botInfo->classType, botInfo->team, false );
		return false; //mal: dont think til we have some team/class stuff to think about!
	}

	if ( botWorld->gameLocalInfo.debugBotWeapons ) {
		botUcmd->botCmds.lookDown = true;
		botUcmd->idealWeaponSlot = GUN;
		botUcmd->botCmds.attack = true;
		if ( !botInfo->weapInfo.primaryWeapHasAmmo && !botInfo->weapInfo.sideArmHasAmmo && !botInfo->justSpawned ) {
			botUcmd->botCmds.suicide = true;
		}
		Bot_MoveToGoal( botInfo->origin + idVec3( 5.0f, 5.0f, 0.0f ), vec3_zero, RUN, ( botThreadData.random.RandomInt( 100 ) > 80 ) ? RANDOM_JUMP : NULLMOVETYPE );
		return false;
	}

	if ( botInfo->resetState > 0 ) { 
		ResetBotsAI( ( botInfo->resetState == MINOR_RESET_EVENT ) ? false : true );
		return false;
	}

	return true;
}

/*
===============
idBotAI::ClearBotState
===============
*/
void idBotAI::ClearBotState() {
	nextRemoved = NULL;

	crateGoodTime = 0;

	LTG_CHECK_FOR_CLASS_NBG = NULL;
	NBG_CHECK_FOR_CLASS_NBG = NULL;
	COMBAT_MOVEMENT_STATE = NULL;
	ROOT_AI_NODE = NULL;
	V_ROOT_AI_NODE = NULL;
	NBG_AI_SUB_NODE = NULL;
	LTG_AI_SUB_NODE = NULL;
	V_LTG_AI_SUB_NODE = NULL;
	COMBAT_AI_SUB_NODE = NULL;
	VEHICLE_COMBAT_MOVEMENT_STATE = NULL;
	VEHICLE_COMBAT_AI_SUB_NODE = NULL;

	noAASCounter = 0;
	vehicleEnemyWasInheritedFromFootCombat = false;

	wantsVehicle = false;

	ignoreNewEnemiesTime = 0;
	ignoreNewEnemiesWhileInVehicleTime = 0;

	botTeleporterAttempts = 0;

	botPathFailedCounter = 0;

	botInfo = NULL;
	botUcmd = NULL;
	botWorld = NULL;
	botVehicleInfo = NULL;

	AIStack.STACK_AI_NODE = NULL;
	AIStack.stackClientNum = -1;
	AIStack.stackClientSpawnID = -1;
	AIStack.stackEntNum = -1;
	AIStack.stackTimeLimit = -1;
	skipStrafeJumpTime = 0;
	pistolTime = 0;

	hopMoveGoal = vec3_zero;

	ignoreMCPRouteAction = false;

	turretDangerExists = false;
	turretDangerEntNum = -1;

	warmupActionMoveGoal = ACTION_NULL;
	warmupUseActionMoveGoalTime = 0;

	stuckRandomMoveTime = 0;

	lastVehicleSpawnID = 0;
	lastVehicleTime = 0;

	actionNumInsideDanger = ACTION_NULL;
	lastNearbySafeActionToMoveTowardActionNum = ACTION_NULL;

	shotIsBlockedCounter = 0;

	hasGreetedServer = false;

	reachedPatrolPointTime = 0;

	strafeToAvoidDangerTime = 0;
	strafeToAvoidDangerDir = NULL_DIR;

	timeOnTarget = 0;

	nextObjChargeCheckTime = 0;

	routeNode = NULL;

	vehicleNode = NULL;

	weaponLocked = false;

	vehicleSurrenderTime = 0;
	vehicleSurrenderClient = -1;
	vehicleSurrenderChatSent = false;

	vehicleCheckForPossibleHumanPassengerChatTime = 0;

	weapSwitchTime = 0;

	randomLookDelay = 0;

	resetEnemy = false;

	botWantsVehicleBackTime = 0;

	checkRoadKillTime = 0;

	ignoreIcarusTime = 0;

	pathNodeTimer.ignorePathNode = NULL;
	pathNodeTimer.ignorePathNodeTime = 0;
	pathNodeTimer.ignorePathNodeUpdateTime = 0;

	vehicleGunnerTime = 0;
	vehicleDriverTime = 0;
	vehicleUpdateTime = 0;

	memset( &botAAS, 0, sizeof( botAAS ) );
	memset( &enemyInfo, 0, sizeof( enemyInfo ) );

	memset( ignoreClients, 0, sizeof( ignoreClients ) );
	memset( ignoreEnemies, 0, sizeof( ignoreEnemies ) );
	memset( ignoreBodies, 0, sizeof( ignoreBodies ) );
	memset( currentDangers, 0, sizeof( currentDangers ) );
	memset( ignoreActions, 0, sizeof( ignoreActions ) );
	memset( ignoreItems, 0, sizeof( ignoreItems ) );

	memset( &currentArtyDanger, 0, sizeof( currentArtyDanger ) );

	memset( delayedChats, 0, sizeof( delayedChats ) );

	randomLookOrg.Zero();

	botExitTime		= 0;

	botIsBlocked = 0;
	botIsBlockedTime = 0;
	vehicleWheelsAreOffGroundCounter = 0.0f;

	ignoreDangersTime		= 0;

	lastGrenadeTime			= 0;
	
	fdaUpdateTime			= 0;

	selfShockTime			= 0;

	useNadeOnEnemy			= false;
	useAircanOnEnemy		= false;

	safeGrenade				= false;
	chaseEnemy				= false;
	timeTilAttackEnemy		= -1;
	ignoreSpyTime			= 0;
	framesStuck				= 0;
	framesVehicleStuck		= 0;
	framesFlipped			= 0;
	randomLookRange			= 0.0f;
	isStrafeJumping			= false;
	fastAwareness			= false;
	skipStrafeJumpTime		= 0;
	badMoveTime				= 0;
	vehiclePauseTime		= 0;
	vehicleReverseTime		= 0;
	chasingEnemy			= false;
	testFireShot			= false;

	isBoosting				= false;


	chaseEnemyTime			= 0;

	stayInPosition			= false;

	actionNum				= ACTION_NULL;
	lastActionNum			= ACTION_NULL;
	tacticalActionNum		= ACTION_NULL;
    tacticalActionTime		= -1;
	lastCheckActionTime		= -1;

	supplySelfTime			= 0;

	bugForSuppliesDelay		= 0;

	tacticalActionTime		= 0;

	botNum					= -1;

	aiState					= LTG;

	botCanMoveGoal.Zero();
	botBackStabMoveGoal.Zero();

	actionNum				= 0;

	ltgType					= NO_LTG;
	ltgTarget				= 0;
	ltgTime					= 0;
	ltgReached				= false;
	ltgTimer				= 0;
	ltgTargetType			= NOTYPE;
	ltgUseVehicle			= false;

	rocketTime				= 0;

	nbgType					= NO_NBG;
	nbgTarget				= 0;
	nbgTime					= 0;
	nbgReached				= false;
	nbgTimer				= 0;
	nbgTargetType			= NOTYPE;

	combatNBGTarget			= 0;
	combatNBGTime			= 0;
	combatNBGReached		= false;
	combatNBGType			= NO_COMBAT_TYPE;
	combatMoveFlag			= NULLMOVEFLAG;

	bot_FS_Enemy_Pos.Zero();
	bot_LS_Enemy_Pos.Zero();
	numVisEnemies			= 0;
	enemy					= -1;
	heardClient				= -1;

	nextMissionUpdateTime	= 0;

	nodeTimeOut				= 0;
	newPathTime				= 0;

	hammerTime				= false;
	hammerLocation			= vec3_zero;
	hammerVehicle			= false;

	combatMoveDir			= FORWARD;
	combatMoveType			= COMBAT_MOVE_NULL;
	combatMoveTime			= -1;
	combatMoveFailedCount	= 0;
	gunTargetEntNum			= 0;

	classAbilityDelay		= 0;

	bugForSuppliesDelay		= 0;

	vehicleObstacleSpawnID	= 0;
	vehicleObstacleTime		= 0;

	combatDangerExists		= false;

	lefty = false;

	enemy = -1;
	combatMoveTime = -1;
	ResetRandomLook();
	botIdealWeapSlot = GUN;
	botIdealWeapNum = NULL_WEAP;
	badMoveTime = 0;
	testFireShot = false;

	vehicleAINodeSwitch.nodeSwitchCount = 0;
	vehicleAINodeSwitch.nodeSwitchTime = 0;

	moveErrorCounter.moveErrorTime = 0;
	moveErrorCounter.moveErrorCount = 0;
	
	// FIXME: initialize all variables

	lastAINode = "NO NODE";
	lastMoveNode = "NO NODE";

	botVehiclePathList.Clear();

#ifdef _DEBUG
	debugVar1 = -1;
	debugVar2 = -1;
#endif
}

/*
===============
idBotAI::ClearBotUcmd
===============
*/
void idBotAI::ClearBotUcmd( int clientNum ) {
	botAIOutput_t &output = botThreadData.GetBotOutputState()->botOutput[ clientNum ];

	output.botCmds.ackJustSpawned = false;
	output.botCmds.ackReset = false;
	output.turnType = INSTANT_TURN;
	output.desiredChat = NULL_CHAT;
	output.desiredChatForced = false;
	output.deployInfo.deployableType = NULL_DEPLOYABLE;
	output.deployInfo.location.Zero();
	output.deployInfo.actionNumber = ACTION_NULL;
	output.viewType = VIEW_ORIGIN;
	output.viewEntityNum = -1;
	output.actionEntitySpawnID = -1;
	output.botCmds.activate = false;
	output.botCmds.activateHeld = false;
	output.botCmds.ackSeatChange = false;
	output.botCmds.honkHorn = false;
	output.botCmds.isBlocked = false;
	output.botCmds.launchDecoys = false;
	output.botCmds.launchDecoysNow = false;
	output.botCmds.altAttackOff = false;
	output.botCmds.altAttackOn = false;
	output.botCmds.attack = false;
	output.botCmds.reload = false;
	output.botCmds.enterVehicle = false;
	output.botCmds.zoom = false;
	output.botCmds.lookDown = false;
	output.botCmds.lookUp = false;
	output.botCmds.constantFire = false;
	output.botCmds.throwNade = false;
	output.botCmds.suicide = false;
	output.botCmds.hasNoGoals = false;
	output.botCmds.dropDisguise = false;
	output.botCmds.shoveClient = false;
	output.botCmds.hasMedicInFOV = false;
	output.botCmds.enterSiegeMode = false;
	output.botCmds.exitSiegeMode = false;
	output.moveFlag = NULLMOVEFLAG;
	output.specialMoveType = NULLMOVETYPE;
	output.moveType = NULLMOVETYPE;
	output.moveGoal = vec3_zero;
	output.moveGoal2 = vec3_zero;
	output.moveViewAngles = ang_zero;
	output.moveViewOrigin = vec3_zero;
	output.idealWeaponNum = NULL_WEAP;
	output.idealWeaponSlot = GUN;
	output.ackKillForClient = -1;
	output.ackRepairForClient = -1;
	output.tkReviveTime = 0;
	output.decayObstacleSpawnID = -1;
	output.botCmds.launchPacks = false;
	output.botCmds.topHat = false;
}

