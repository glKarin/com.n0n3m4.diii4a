// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

#include "../Game_local.h" 
#include "../misc/DefenceTurret.h"
#include "../decls/GameDeclIdentifiers.h"
#include "../../game/ContentMask.h"
#include "../misc/PlayerBody.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../rules/GameRules.h"
#include "../vehicles/Transport.h"
#include "../vehicles/VehicleControl.h"

#include "BotThread.h"
#include "BotThreadData.h"
#include "Bot.h"
#include "../Misc.h"
#include "../../sdnet/SDNetSession.h"

idBotThreadData			botThreadData;


/*
================
idBotThreadData::idBotThreadData
================
*/
idBotThreadData::idBotThreadData() {
	personalVehicleClipModel = NULL;
	genericVehicleClipModel = NULL;
	goliathVehicleClipModel = NULL;
}

/*
================
idBotThreadData::~idBotThreadData
================
*/
idBotThreadData::~idBotThreadData() {
}

/*
==================
idBotThreadData::NumAAS
==================
*/
int	idBotThreadData::NumAAS() const {
	return aasList.Num();
}

/*
==================
idBotThreadData::GetAAS
==================
*/
idAAS *idBotThreadData::GetAAS( int num ) const {
	if ( ( num >= 0 ) && ( num < aasList.Num() ) ) {
		if ( aasList[ num ] && aasList[ num ]->GetSettings() ) {
			return aasList[ num ];
		}
	}
	return NULL;
}

/*
==================
idBotThreadData::GetAAS
==================
*/
idAAS *idBotThreadData::GetAAS( const char *name ) const {
	int i;

	for ( i = 0; i < aasNames.Num(); i++ ) {
		if ( aasNames[ i ] == name ) {
			if ( !aasList[ i ]->GetSettings() ) {
				return NULL;
			} else {
				return aasList[ i ];
			}
		}
	}
	return NULL;
}

/*
==================
idBotThreadData::ShowAASStats
==================
*/
void idBotThreadData::ShowAASStats() const {
	if ( IsThreadingEnabled() ) {
		return;
	}
	for ( int i = 0; i < aasList.Num(); i++ ) {
		aasList[i]->Stats();
	}
}

/*
==================
idBotThreadData::InitClientInfo
==================
*/
void idBotThreadData::InitClientInfo( int clientNum, bool resetAll, bool leaving ) {
	clientInfo_t &client = GetGameWorldState()->clientInfo[ clientNum ];
	botAIOutput_t& botUcmd = GetBotOutputState()->botOutput[ clientNum ];

	if ( resetAll ) {
       	client.classType = NOCLASS;
		client.team = NOTEAM;
	}

	memset( &botUcmd, 0, sizeof( botUcmd ) );
	botUcmd.debugInfo.botGoalType = NULL_GOAL_TYPE;
	botUcmd.ackKillForClient = -1;
	botUcmd.ackRepairForClient = -1;
	botUcmd.desiredChat = NULL_CHAT;

	client.inGame = true;
	client.isActor = false;
	client.briefingTime = 0;

	client.escortSpawnID = -1;
	client.escortRequestTime = 0;

	client.lastRoadKillTime = 0;

	client.gpmgOrigin = vec3_zero;

	client.usingMountedGPMG = false;
	client.mountedGPMGEntNum = -1;

	client.hasTeleporterInWorld = false;
	client.hasRepairDroneInWorld = false;

	client.resetState = 0;

	client.lastShieldDroppedTime = 0;

	client.backPedalTime = 0;

	client.wantsVehicle = false;
		
	client.areaNum = 0;
	client.areaNumVehicle = 0;

	client.spectatorCounter = 0;

	client.needsParachute = false;

	client.covertWarningTime = 0;

	client.myHero = -1;
	client.mySavior = -1;
	client.missionEntNum = -1;

	client.supplyCrateRequestTime = 0;

	memset( client.lastChatTime, 0, sizeof ( client.lastChatTime ) );
	client.lastThanksTime = 0;
	client.chatDelay = 0;
	client.lastClassChangeTime = 0;
	client.lastWeapChangedTime = 0;

	memset( &client.weapInfo, 0, sizeof( client.weapInfo ) );
	memset( client.packs, 0, sizeof( client.packs ) );
	memset( client.kills, -1, sizeof( client.kills ) );
	memset( client.forceShields, 0, sizeof( client.forceShields ) );

	memset( &client.abilities, 0, sizeof( client.abilities ) );

	client.killCounter = 0;
	client.isCamper = false;
	client.favoriteKill = -1;
	client.killsSinceSpawn = 0;

	client.disguisedClient = -1;

	client.lastAttackClient = -1;
	client.lastAttacker = -1;
	client.lastAttackClientTime = -1;
	client.lastAttackerTime = -1;

	client.hasCrosshairHint = false;

	client.isInRadar = false;

	client.classChargeUsed = 0;
	client.deviceChargeUsed = 0;
	client.deployChargeUsed = 0;
	client.bombChargeUsed = 0;
	client.fireSupportChargedUsed = 0;

	client.inPlayZone = true;

	client.killTargetNum = -1;
	client.killTargetSpawnID = -1;
	client.killTargetUpdateTime = 0;
	client.killTargetNeedsChat = false;

	client.pickupRequestTime = 0;
	client.pickupTargetSpawnID = -1;
	client.commandRequestTime = 0;
	client.commandRequestChatSent = false;

	client.repairTargetNum = -1;
	client.repairTargetSpawnID = -1;
	client.repairTargetUpdateTime = 0;
	client.repairTargetNeedsChat = false;

	client.disguisedClient = -1;
	client.enemiesInArea = 0;
	client.friendsInArea = 0;
	client.hasGroundContact = false;
	client.hasJumped = false;
	client.inEnemyTerritory = false;

	client.spawnHostTargetSpawnID = -1;

	client.inLimbo = false;
	client.invulnerableEndTime = 0;
	client.inWater = false;
	client.isDisguised = false;
	client.isMovingForward = false;
	client.isNoTarget = false;

	client.posture = IS_STANDING;

	client.oldOrigin = vec3_zero;

	client.revived = false;
	client.spawnTime = 0;
	client.targetLockEntNum = -1;
	client.targetLockTime = 0;

	client.deployDelayTime = 0;

	client.proxyInfo.entNum = -1;
	client.proxyInfo.time = 0;
	client.proxyInfo.weapon = NULL_VEHICLE_WEAPON;
	client.proxyInfo.weaponIsReady = false;
	client.proxyInfo.weaponAxis = mat3_zero;
	client.proxyInfo.weaponOrigin = vec3_zero;
	client.proxyInfo.hasTurretWeapon = false;
	client.proxyInfo.clientChangedSeats = false;
	client.proxyInfo.boostCharge = 0.0f;

	client.supplyCrate.entNum = 0;
	client.supplyCrate.checkedAreaNum = false;
	client.supplyCrate.areaNum = 0;
	client.supplyCrate.origin = vec3_zero;
	client.supplyCrate.team = NOTEAM;
	client.supplyCrate.xySpeed = 0.0f;

	client.xySpeed = 0.0f;

	client.touchingItemTime = -1;
	client.justSpawned = true;
	client.weapInfo.primaryWeaponNeedsUpdate = true;
	client.crouchCounter = 0;

	if ( !leaving ) {
		client.scriptHandler.chargeTimer = gameLocal.AllocTargetTimer( "energy_timer" );
		client.scriptHandler.bombTimer = gameLocal.AllocTargetTimer( "energy_charge" );
		client.scriptHandler.fireSupportTimer = gameLocal.AllocTargetTimer( "energy_firesupport" );
		client.scriptHandler.deviceTimer = gameLocal.AllocTargetTimer( "energy_device" );
		client.scriptHandler.deployTimer = gameLocal.AllocTargetTimer( "energy_deployment" );
		client.scriptHandler.supplyTimer = gameLocal.AllocTargetTimer( "energy_supply" );
	} else {
		client.inGame = false;
		client.isBot = false;
	}
}


/*
==================
idBotThreadData::ResetClientsInfo
==================
*/
void idBotThreadData::ResetClientsInfo() {
	int i;
	idPlayer* player;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		InitClientInfo( player->entityNumber, false, false );
		GetGameWorldState()->clientInfo[ player->entityNumber ].resetState = MAJOR_RESET_EVENT;
	}
}

/*
==================
idBotThreadData::DrawActionPaths
==================
*/
void idBotThreadData::DrawActionPaths() {
	if ( bot_testPathToBotAction.IsModified() ) {
		aas_showPath.SetInteger( 0 );
		bot_testPathToBotAction.ClearModified();
	}

	if ( bot_testPathToBotAction.GetInteger() == -1 ) {
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player ) { 
		return;
	}

	idMat3 viewAxis = player->GetViewAxis();

	if ( player->GetGameTeam() == NULL ) {
		gameLocal.Warning("Must join a team before using this cvar!");
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	const playerTeamTypes_t playerTeam = player->GetGameTeam()->GetBotTeam();

	int actionNumber = bot_testPathToBotAction.GetInteger();

	if ( actionNumber <= ACTION_NULL || actionNumber > botActions.Num() ) {
		gameLocal.Warning("Invalid action set by bot_testPathToBotAction!");
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	if ( botActions[ actionNumber ] == NULL ) {
		gameLocal.Warning("Invalid action set by bot_testPathToBotAction!");
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	if ( botActions[ actionNumber ]->baseActionType != BASE_ACTION ) {
		gameLocal.Warning("Invalid action set by bot_testPathToBotAction!");
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	idAAS *aas = GetAAS( aas_test.GetInteger() );
	
	if ( aas == NULL ) {
		gameLocal.Warning("No valid aas exists for aas_type %i!", aas_test.GetInteger() );
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	int excludeTravelFlags = ( playerTeam == GDF ) ? ( TFL_INVALID | TFL_INVALID_GDF ) : ( TFL_INVALID | TFL_INVALID_STROGG );
	idVec3 actionOrg = botActions[ actionNumber ]->GetActionOrigin();
	int areaNum = aas->PointReachableAreaNum( actionOrg, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );

	if ( areaNum == 0 ) {
		gameLocal.Warning("Can't find a valid area for action %i!", bot_testPathToBotAction.GetInteger() );
		bot_testPathToBotAction.SetInteger( -1 );
		return;
	}

	aas_showPath.SetInteger( areaNum );

	DrawActionNumber( actionNumber, viewAxis, true );
}

/*
==================
idBotThreadData::DrawDefuseHints
==================
*/
void idBotThreadData::DrawDefuseHints() {
	for( int i = 0; i < botLocationRemap.Num(); i++ ) { 
		gameRenderWorld->DebugBox( colorRed, botLocationRemap[ i ]->bbox );
		gameRenderWorld->DebugArrow( colorYellow, botLocationRemap[ i ]->bbox.GetCenter(), botLocationRemap[ i ]->target, 4 );
		gameRenderWorld->DebugLine( colorGreen, botLocationRemap[ i ]->target, botLocationRemap[ i ]->target + idVec3( 0, 0, 64 ) );
	}
}



/*
==================
idBotThreadData::DrawActions
==================
*/
void idBotThreadData::DrawActions() {
	int i, j;
	idMat3 viewAxis;
	idPlayer *player;
	player = gameLocal.GetLocalPlayer();
	idVec3 end;

	if ( !player ) { 
		return;
	}

	viewAxis = player->GetViewAxis();

	if ( bot_drawIcarusActions.GetBool() ) {
		for( i = 0; i < botActions.Num(); i++ ) {
			if ( botActions[ i ] == NULL ) {
				continue;
			}

			if ( botActions[ i ]->baseActionType != BASE_ACTION ) {
				continue;
			}

			if ( botActions[ i ]->actionVehicleFlags != 0 && !( botActions[ i ]->actionVehicleFlags & PERSONAL ) || botActions[ i ]->actionVehicleFlags == -1 ) {
				continue;
			}

			if ( botActions[ i ]->areaNumVehicle == 0 ) {
				continue;
			}

			if ( botActions[ i ]->GetStroggObj() == ACTION_NULL ) {
				continue;
			}

			end = botActions[ i ]->origin - player->GetPhysics()->GetOrigin();

			if ( end.LengthSqr() > Square( bot_drawActionDist.GetFloat() ) ) {
				continue;
			}

			DrawActionNumber( i, viewAxis, false );
		}
	} else if ( bot_drawBadIcarusActions.GetBool() ) {
		for( i = 0; i < botActions.Num(); i++ ) {
			if ( botActions[ i ] == NULL ) {
				continue;
			}

			if ( botActions[ i ]->baseActionType != BASE_ACTION ) {
				continue;
			}

			if ( botActions[ i ]->actionVehicleFlags != 0 && !( botActions[ i ]->actionVehicleFlags & PERSONAL ) || botActions[ i ]->actionVehicleFlags == -1 ) {
				continue;
			}

			if ( botActions[ i ]->areaNumVehicle > 0 ) {
				continue;
			}

			if ( botActions[ i ]->GetStroggObj() == ACTION_NULL ) {
				continue;
			}

			end = botActions[ i ]->origin - player->GetPhysics()->GetOrigin();

			if ( end.LengthSqr() > Square( bot_drawActionDist.GetFloat() ) ) {
				continue;
			}

			DrawActionNumber( i, viewAxis, false );
			gameRenderWorld->DebugCircle( colorRed, botActions[ i ]->origin, idVec3( 0, 0, 1 ), botActions[ i ]->radius, 8 );
		}
	} else if ( bot_drawActions.GetBool() ) {
        for( i = 0; i < botActions.Num(); i++ ) {

			if ( botActions[ i ] == NULL ) {
				continue;
			}

			if ( botActions[ i ]->baseActionType != BASE_ACTION ) {
				continue;
			}

			if ( bot_drawActionNumber.GetInteger() != -1 ) {
				if ( i != bot_drawActionNumber.GetInteger() ) {
					continue;
				}
			}

			if ( bot_drawActionRoutesOnly.GetInteger() != -1 ) {
				if ( botActions[ i ]->routeID != bot_drawActionRoutesOnly.GetInteger() ) {
					continue;
				}
			}

			if ( bot_drawActionWithClasses.GetBool() ) {
				if ( botActions[ i ]->GetValidClasses() == 0 ) {
					continue;
				}
			}

			int actionFilter = bot_drawActiveActionsOnly.GetInteger();
			botActionGoals_t actionType = ( botActionGoals_t ) bot_drawActionTypeOnly.GetInteger();

			if ( actionType != ACTION_NULL ) {
				if ( botActions[ i ]->GetHumanObj() != actionType && botActions[ i ]->GetStroggObj() != actionType ) {
					continue;
				}
			}

			if ( actionFilter > 0 ) {
				if ( !botActions[ i ]->ActionIsActive() ) {
					continue;
				}

				if ( actionFilter == 2 ) {
					if ( botActions[ i ]->GetHumanObj() == ACTION_NULL ) {
						continue;
					}
				} else if ( actionFilter == 3 ) {
					if ( botActions[ i ]->GetStroggObj() == ACTION_NULL ) {
						continue;
					}
				}
			}

			if ( bot_drawActionGroupNum.GetInteger() > -1 ) {
				if ( botActions[ i ]->GetActionGroup() != bot_drawActionGroupNum.GetInteger() ) { 
					continue;
				}
			}

			if ( bot_drawActionVehicleType.GetInteger() > -1 ) {
				if ( botActions[ i ]->actionVehicleFlags == -1 || !( bot_drawActionVehicleType.GetInteger() & botActions[ i ]->actionVehicleFlags ) && bot_drawActionVehicleType.GetInteger() != 0 ) {
					continue;
				}
			}			

			end = botActions[ i ]->origin - player->GetPhysics()->GetOrigin();

			if ( end.LengthSqr() > Square( bot_drawActionDist.GetFloat() ) ) {
				continue;
			}

			DrawActionNumber( i, viewAxis, true );
		}
	}

//mal: now draw all of the different route nodes out there..
	if ( bot_drawRoutes.GetBool() ) {
        for( i = 0; i < botRoutes.Num(); i++ ) {

			if ( botRoutes[ i ] == NULL ) {
				continue;
			}

			if ( bot_drawActiveRoutesOnly.GetBool() ) {
				if ( !botRoutes[ i ]->active ) {
					continue;
				}
			}

			if ( bot_drawRouteGroupOnly.GetInteger() != -1 ) {
				if ( botRoutes[ i ]->GetRouteGroup() != bot_drawRouteGroupOnly.GetInteger() ) {
					continue;
				}
			}

			end = botRoutes[ i ]->origin;
			end[ 2 ] += 64.0f;
			gameRenderWorld->DebugLine( ( botRoutes[ i ]->isHeadNode == true ) ? colorPink : colorDkRed, botRoutes[ i ]->origin, end, 16 );
		
			end[ 2 ] += 8.0f;
	
			gameRenderWorld->DrawText( va( "Route %i     RouteID %i", i, botRoutes[ i ]->groupID ), end, 0.20f, colorWhite, viewAxis );

			end[ 2 ] += 8.0f;
	
			gameRenderWorld->DrawText( va( "Team %i     Active %i", botRoutes[ i ]->team, botRoutes[ i ]->active ), end, 0.20f, colorWhite, viewAxis );
	
			gameRenderWorld->DebugCircle( ( botRoutes[ i ]->isHeadNode == true ) ? colorYellow : colorGreen, botRoutes[ i ]->origin, idVec3( 0, 0, 1 ), botRoutes[ i ]->radius, 8 );

			for( j = 0; j < botRoutes[ i ]->routeLinks.Num(); j++ ) {
				gameRenderWorld->DebugArrow( colorMdGrey, botRoutes[ i ]->origin, botRoutes[ i ]->routeLinks[ j ]->origin, 16 );
			}
		}
	}
}

/*
==================
idBotThreadData::DrawDynamicObstacles
==================
*/
void idBotThreadData::DrawDynamicObstacles() {
	idMat3 viewAxis;
	idPlayer *player;
	player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	viewAxis = player->GetViewAxis();

	for( int i = 0; i < botObstacles.Num(); i++ ) {

		idVec3 vec = botObstacles[ i ]->bbox.GetCenter() - player->GetPhysics()->GetOrigin();

		if ( vec.LengthSqr() > Square( 8192.f ) ) {
			continue;
		}

		idVec4 colorType = ( botObstacles[ i ]->areaNum[ AAS_PLAYER ] == 0 || botObstacles[ i ]->areaNum[ AAS_VEHICLE ] == 0 ) ? colorRed : colorGreen;
		gameRenderWorld->DebugBox( colorType, botObstacles[ i ]->bbox );
		float radius = botObstacles[ i ]->bbox.GetExtents().ToVec2().Length();
		idVec3 center = botObstacles[ i ]->bbox.GetCenter();
		gameRenderWorld->DebugCircle( colorYellow, center, idVec3( 0, 0, 1 ), radius, 24 );
		idVec3 top = center;
		top.z += botObstacles[ i ]->bbox.GetExtents().Length() + 64.0f;
		gameRenderWorld->DrawText( va( "Origin: %.1f %.1f %.1f", center.x, center.y, center.z ), top, 0.70f, colorWhite, viewAxis );
		top.z += 32.0f;
		gameRenderWorld->DrawText( va( "Obstacle: %i", i ), top, 0.70f, colorWhite, viewAxis );
		top.z += 32.0f;
		gameRenderWorld->DrawText( va( "Player AAS: %i    Vehicle AAS: %i", botObstacles[ i ]->areaNum[ AAS_PLAYER ], botObstacles[ i ]->areaNum[ AAS_VEHICLE ] ), top, 0.70f, colorWhite, viewAxis );
	}
}

/*
==================
idBotThreadData::DrawNumbers

Used in conjunction with the bot debug hud - lets me see client numbers so I can debug the bots AI easily.
==================
*/
void idBotThreadData::DrawNumbers() {
	int i;
	idMat3 viewAxis;
	idPlayer *player;
	player = gameLocal.GetLocalPlayer();
	idVec3 end;

	if ( !player ) {
		return;
	}

	viewAxis = player->GetViewAxis();

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( GetGameWorldState()->clientInfo[ i ].inGame == false ) {
			continue;
		}

		if ( i == player->entityNumber ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team == NOTEAM ) {
			continue;
		}

		idPlayer* drawPlayer = gameLocal.GetClient( i );

		if ( drawPlayer == NULL ) {
			continue;
		}

		end = drawPlayer->GetPhysics()->GetOrigin();
		end[ 2 ] += 82;

		gameRenderWorld->DrawText( va( "%i", i ), end, 1.5f, colorWhite, viewAxis );
	}
}

/*
==================
idBotThreadData::InitAAS
==================
*/
void idBotThreadData::InitAAS( const idMapFile *mapFile ) {

	assert( !botThread->IsActive() );

	aasList.DeleteContents( true );
	aasNames.Clear();

	// get the aas settings definitions
	sdDeclWrapperTemplate< sdDeclStringMap > declStringMapType;
	declStringMapType.Init( declStringMapIdentifier );

	const sdDeclStringMap* stringMap = declStringMapType.LocalFind( "aas_types", false );
	const idKeyValue *kv = stringMap->GetDict().MatchPrefix( "type" );

	while( kv != NULL ) {
		idAAS *aas = idAAS::Alloc();
		aasList.Append( aas );
		aasNames.Append( kv->GetValue() );
		kv = stringMap->GetDict().MatchPrefix( "type", kv );
	}

	// load navigation system for all the different player/vehicle sizes
	for( int i = 0; i < aasNames.Num(); i++ ) {
		aasList[ i ]->Init( idStr( mapFile->GetName() ).SetFileExtension( aasNames[ i ] ).c_str(), mapFile->GetGeometryCRC() );
	}

//mal: now, lets do a check to make sure that at least the player AAS was loaded - if not, we're in trouble!
	if ( aasList.Num() == 0 ) {
        common->Warning( "No valid AAS file found for this map! The bots won't be able to play without one!\nConsult the manual for information on how to create an AAS file." );
	}

	pendingAreaChanges.SetNum( 0, false );
	pendingReachChanges.SetNum( 0, false );

	idVec3 mins = idVec3( 16.f, 16.f, 32.f );
	idVec3 maxs = idVec3( -16.f, -16.f, 0.f );

	personalVehicleClipModel = new idClipModel( idTraceModel( idBounds( mins, maxs ) ), false );

	mins = idVec3( 32.f, 32.f, 32.f );
	maxs = idVec3( -32.f, -32.f, 0.f );

	genericVehicleClipModel = new idClipModel( idTraceModel( idBounds( mins, maxs ) ), false );

	mins = idVec3( 64.f, 32.f, 64.f );
	maxs = idVec3( -64.f, -32.f, 0.f );

	goliathVehicleClipModel = new idClipModel( idTraceModel( idBounds( mins, maxs ) ), false );
}

/*
==================
idBotThreadData::UpdateState
==================
*/
void idBotThreadData::UpdateState() {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {

		idPlayer *player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		clientInfo_t &client = GetGameWorldState()->clientInfo[ i ];

		if ( client.inGame == false || client.team == NOTEAM ) { 
			continue;
		}

		if ( player->GetTeleportEntity() == NULL && ( ( player->GetPhysics()->HasGroundContacts() || player->GetPhysics()->InWater() ) || player->GetProxyEntity() != NULL || player->GetNoClip() ) ) {

			idVec3 aasOrigin = player->GetPhysics()->GetOrigin();

			int areaNum = Nav_GetAreaNumAndOrigin( AAS_PLAYER, client.origin, aasOrigin );

			if ( areaNum > 0 ) { //mal: be sure to cache only the last valid area
				client.areaNum = areaNum;
				client.aasOrigin = aasOrigin;
			}

			sdTransport* transport = player->GetProxyEntity()->Cast< sdTransport >();

			if ( transport == NULL ) {
				aasOrigin = player->GetPhysics()->GetOrigin();
			} else {
				aasOrigin = transport->GetPhysics()->GetOrigin() + ( transport->GetPhysics()->GetAxis() * transport->GetPhysics()->GetBounds( -1 ).GetCenter() );
			}

			areaNum = Nav_GetAreaNumAndOrigin( AAS_VEHICLE, aasOrigin, aasOrigin );

			if ( areaNum > 0 ) { //mal: be sure to cache only the last valid area
				client.areaNumVehicle = areaNum;
				client.aasVehicleOrigin = aasOrigin;
			}
		}

		client.oldOrigin = player->GetPhysics()->GetOrigin();
		client.weapInfo.sideArmHasAmmo = player->GetInventory().CheckWeaponSlotHasAmmo( PISTOL_SLOT );
		client.weapInfo.primaryWeapHasAmmo = player->GetInventory().CheckWeaponSlotHasAmmo( GUN_SLOT );
		client.weapInfo.hasNadeAmmo = player->GetInventory().CheckWeaponSlotHasAmmo( GRENADE_SLOT );

		if ( client.weapInfo.primaryWeaponNeedsUpdate ) {
			player->GetInventory().UpdatePrimaryWeapon();
			client.weapInfo.primaryWeaponNeedsUpdate = false;
		}

		if ( client.isBot ) {
			const botAIOutput_t& ucmd = botThreadData.GetGameOutputState()->botOutput[ i ];

			if ( ucmd.ackKillForClient > -1 && ucmd.ackKillForClient < MAX_CLIENTS ) {
				botThreadData.GetGameWorldState()->clientInfo[ ucmd.ackKillForClient ].killTargetNeedsChat = false;
			}

			if ( ucmd.ackRepairForClient > -1 && ucmd.ackRepairForClient < MAX_CLIENTS ) {
				botThreadData.GetGameWorldState()->clientInfo[ ucmd.ackRepairForClient ].repairTargetNeedsChat = false;
			}
		}
	}

	if ( GetGameWorldState()->botGoalInfo.isTrainingMap ) {
		if ( !MapHasAnActor() ) {
			FindBotToBeActor();
		}

		TrainingThink();
	}
	
	CheckCurrentChatRequests();

	DynamicEntity_Think(); //mal: check all dynamic entities out there.

//mal: next, run some data collection for the bot thread: get info about dead bodies around the bot...
	sdInstanceCollector< sdPlayerBody > bodies( false );

	for ( int i = 0; i < MAX_PLAYERBODIES; i++ ) {
		playerBodiesInfo_t &body = GetGameWorldState()->playerBodies[ i ];
		if ( i >= bodies.Num() ) {
			body.isValid = false;
			body.areaNum = 0;
			continue;
		}

        sdPlayerBody *stiff = bodies[ i ];

		if ( stiff ) {
			idPlayer *player = stiff->GetClient();

			if ( !player ) { //mal: they prolly got kicked/disconnected - we'll ignore those bodies.
				body.isValid = false;
				continue;
			}

			if ( stiff->GetPhysics()->InWater() > 0.0f ) {	//currently, there is no under water swimming, so ignore bodies in water. If this changes, update
				body.isValid = false;
				continue;
			}

			body.isValid = true;
			body.bodyTeam = bodies[ i ]->GetGameTeam()->GetBotTeam();
			body.bodyOwnerClientNum = player->entityNumber;
			body.bodyOrigin = bodies[ i ]->GetPhysics()->GetOrigin();
			body.bodyNum = stiff->entityNumber;
			body.spawnID = gameLocal.GetSpawnId( stiff );
	
			if ( body.areaNum == 0 ) {
				body.areaNum = Nav_GetAreaNum( AAS_PLAYER, body.bodyOrigin );
			}

			if ( body.areaNum == 0 ) {
				body.isValid = false;
			}

			if ( body.bodyTeam == GDF ) {
				sdScriptHelper h1;
				body.isSpawnHostAble = stiff->CallBooleanNonBlockingScriptEvent( stiff->isSpawnHostableFunc, h1 );

				sdScriptHelper h2;
				body.isSpawnHost = stiff->CallBooleanNonBlockingScriptEvent( stiff->isSpawnHostFunc, h2 );
			} else {

				body.isSpawnHostAble = false; //mal: can't spawnhost strogg bodies!
				body.isSpawnHost = false;
			}
	
			sdScriptHelper h3;
			body.uniformStolen = stiff->CallBooleanNonBlockingScriptEvent( stiff->hasNoUniformFunc, h3 );
		}
	}

//mal: update some info for the bot's thread.
	gameLocalInfo_t &gameLocalInfo = GetGameWorldState()->gameLocalInfo;
	gameLocalInfo.time = gameLocal.time;
	gameLocalInfo.gameTimeInMinutes = ( int ) ( ( gameLocal.rules->GetGameTime() / 1000 ) / 60 );
	gameLocalInfo.inEndGame = gameLocal.rules->IsEndGame();

	if ( gameLocalInfo.inEndGame ) {
		sdTeamInfo* winningTeam = gameLocal.rules->GetWinningTeam();
		if ( winningTeam != NULL ) {
			if ( winningTeam->GetIndex() == 0 ) {
				gameLocalInfo.winningTeam = GDF;
			} else {
				gameLocalInfo.winningTeam = STROGG;
			}
		}
	}

	gameLocalInfo.gameIsBotMatch = ( networkSystem->IsDedicated() || networkSystem->IsLANServer() ) ? false : true;
	gameLocalInfo.inWarmup = gameLocal.rules->IsWarmup();
	gameLocalInfo.botsUseTKRevive = bot_useTKRevive.GetBool();
	gameLocalInfo.numClients = gameLocal.numClients;
	gameLocalInfo.heroMode = !bot_doObjectives.GetBool();
	gameLocalInfo.botsCanStrafeJump = bot_useStrafeJump.GetBool();
	gameLocalInfo.botsSillyWarmup = bot_sillyWarmup.GetBool();
	gameLocalInfo.botsIgnoreGoals = bot_ignoreGoals.GetBool();
	gameLocalInfo.botsUseSpawnHosts = bot_useSpawnHosts.GetBool();
	gameLocalInfo.botsUseUniforms = bot_useUniforms.GetBool();
	gameLocalInfo.botSkill = bot_skill.GetInteger();

	if ( AllowDebugData() ) {
		gameLocalInfo.botFollowPlayer = bot_followMe.GetInteger();
		gameLocalInfo.botIgnoreEnemies = bot_ignoreEnemies.GetInteger();
	} else {
		gameLocalInfo.botFollowPlayer = 0;
		gameLocalInfo.botIgnoreEnemies = 0;
	}

	if ( gameLocalInfo.botSkill == BOT_SKILL_DEMO && !gameLocalInfo.gameIsBotMatch ) { //mal_HACK: another 11th hour fix. ugh.
		gameLocalInfo.botSkill = 2;
	}

	gameLocalInfo.botAimSkill = bot_aimSkill.GetInteger();
	gameLocalInfo.friendlyFireOn = si_teamDamage.GetBool();
	gameLocalInfo.botsPaused = bot_pause.GetBool() || gameLocal.IsPaused();
	gameLocalInfo.botKnifeOnly = bot_knifeOnly.GetBool();
	gameLocalInfo.debugBotWeapons = bot_debugWeapons.GetBool();
	gameLocalInfo.debugBots = bot_debug.GetBool();
	gameLocalInfo.debugObstacleAvoidance = bot_debugObstacleAvoidance.GetBool();
	gameLocalInfo.botsUseVehicles = bot_useVehicles.GetBool();
	gameLocalInfo.botsUseAirVehicles = bot_useAirVehicles.GetBool();
	gameLocalInfo.botsStayInVehicles = bot_stayInVehicles.GetBool();
	gameLocalInfo.botsUseDeployables = bot_useDeployables.GetBool();
	gameLocalInfo.botsUseMines = bot_useMines.GetBool();
	gameLocalInfo.debugPersonalVehicles = bot_debugPersonalVehicles.GetBool();
	gameLocalInfo.debugAltRoutes = bot_useAltRoutes.GetBool();
	gameLocalInfo.botsCanSuicide = bot_useSuicideWhenStuck.GetBool();
	gameLocalInfo.botsCanDecayObstacles = bot_allowObstacleDecay.GetBool();
	gameLocalInfo.botsSleep = false;
	gameLocalInfo.botPauseInVehicleTime = bot_pauseInVehicleTime.GetInteger();
	gameLocalInfo.botsDoObjsInTrainingMode = bot_doObjsInTrainingMode.GetBool();
	gameLocalInfo.botTrainingModeObjDelayTime = ( bot_doObjsDelayTimeInMins.GetInteger() * 60 );
	
	if ( bot_sleepWhenServerEmpty.GetBool() ) {
		if ( !ServerHasHumans() ) {
			gameLocalInfo.botsSleep = true;
		}
	}
	
#ifdef BOT_MOVE_LOOKUP
	// FeaRog: let the bots rebuild the lookup tables for movement if they need to 
	//			(this function won't actually do anything unless the cvars have changed)
	idBot::BuildMoveLookups( pm_runspeedforward.GetFloat(), pm_runspeedstrafe.GetFloat(), pm_runspeedback.GetFloat(),
								pm_sprintspeedforward.GetFloat(), pm_sprintspeedstrafe.GetFloat() );
#endif

//mal: update the current "humans on server" status for the "Hero" game mode.
	bool hasGDFHumans = gameLocalInfo.teamGDFHasHuman;
	bool hasSTROGGHumans = gameLocalInfo.teamStroggHasHuman;
	gameLocalInfo.teamStroggHasHuman = TeamHasHumans( STROGG );
	gameLocalInfo.teamGDFHasHuman = TeamHasHumans( GDF );
	
//mal: if the current human situation changed from last frame, update the bot AI. In hero mode, this will dictate whether they do objs or not.
	if ( hasGDFHumans != gameLocalInfo.teamGDFHasHuman ) {
		ResetBotAI( GDF );
	}

	if ( hasSTROGGHumans != gameLocalInfo.teamStroggHasHuman ) {
		ResetBotAI( STROGG );
	}

	threadingEnabled = bot_threading.GetBool();

	if ( IsThreadingEnabled() ) {

		if ( com_timeServer.GetInteger() != 0 ) {
			threadMinFrameDelay = 0;
			threadMaxFrameDelay = 0;
		} else {
			threadMinFrameDelay = Min( bot_threadMinFrameDelay.GetInteger(), bot_threadMaxFrameDelay.GetInteger() );
			threadMaxFrameDelay = bot_threadMaxFrameDelay.GetInteger();
		}

		assert( BOT_THREAD_BUFFER_SIZE >= threadMaxFrameDelay * 2 );

		// let the bot thread know there's another game frame
		botThread->SignalBotThread();

		// wait if the bot thread is falling too far behind
		while ( botThread->GetLastGameFrameNum() < gameLocal.GetFrameNum() - threadMaxFrameDelay && !botThread->IsWaiting() ) {
			common->Warning( "Waiting on BotThread\n" );
			botThread->WaitForBotThread();
		}

	} else {
		// let the bots think from here
		RunFrame();
	}

	botFPS = botThread->GetFrameRate();
}

/*
==================
idBotThreadData::RunFrame
==================
*/
void idBotThreadData::RunFrame() {
	// apply any pending changes
	ApplyPendingAreaChanges();
	ApplyPendingReachabilityChanges();

	// set latest game world state as current for the bots
	SetCurrentBotWorldState();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) { //mal: run the bots think loop
		if ( bots[i] != NULL ) {
			bots[i]->Think();
		}
	}

	// set latest bot output as current for the game
	SetCurrentBotOutputState();
}

/*
================
idBotThreadData::AddActionToHash
================
*/
void idBotThreadData::AddActionToHash( const char *actionName, int actionNum ) {
	if ( FindActionByName( actionName ) != -1 ) {
		common->Error( "Multiple bot actions named '%s'", actionName );
	}

	actionHash.Add( actionHash.GenerateKey( actionName, true ), actionNum );
}

/*
================
idBotThreadData::FindActionByOrigin
================
*/
int idBotThreadData::FindActionByOrigin( const botActionGoals_t actionType, const idVec3& origin, bool activeOnly ) {
	int i;
	int	actionNumber = -1;
	idVec3 vec;

	for( i = 0; i < botActions.Num(); i++ ) {
		if ( activeOnly ) {
			if ( !botActions[ i ]->ActionIsActive() ) {
				continue;
			}
		}

		if ( !botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( actionType != ACTION_NULL ) {
			if ( botActions[ i ]->GetHumanObj() != actionType && botActions[ i ]->GetStroggObj() != actionType ) {
				continue;
			}
		}

		if ( botActions[ i ]->actionBBox.IsCleared() ) { //mal: need a bbox type action to do this check!
			continue;
		}

		vec = botActions[ i ]->GetActionOrigin() - origin;

		if ( vec.LengthSqr() > Square( 1024.0f ) ) { //mal: too far away from our origin to matter
			continue;
		}

		if ( !botActions[ i ]->actionBBox.ContainsPoint( origin ) ) {
			continue;
		}

		actionNumber = i;
		break;
	}

	return actionNumber;
}

/*
=============
idBotThreadData::FindActionName

Returns the action whose name matches the specified string.
=============
*/
int idBotThreadData::FindActionByName( const char *actionName ) {
	int hash, i;

	hash = actionHash.GenerateKey( actionName, true );

	for ( i = actionHash.GetFirst( hash ); i != -1; i = actionHash.GetNext( i ) ) {		//mal_FIXME: optimize this loop! Originally we tested for "isvalid" first.
		if ( botActions[ i ]->name.Icmp( actionName ) == 0 ) {
			return i;
		}
	}

	return -1;

/*
	int hash, i;

	hash = actionHash.GenerateKey( actionName, true );

	for ( i = actionHash.GetFirst( hash ); i != -1; i = actionHash.GetNext( i ) ) {		//mal_FIXME: optimize this loop! Originally we tested for "isvalid" first.
		if ( botActions[ i ]->name.Icmp( actionName ) == 0 ) {
			if ( botActions[ i ]->ActionIsValid() ) {
 				return i;
			}
			return -1;
		}
	}
	return -1;
*/
}

/*
================
idBotThreadData::Clear

Clears out all of the botThreadData data - called by idGameLocal::LoadMap
================
*/
void idBotThreadData::Clear( bool clearAll ) {
	random.SetSeed( 0 );

	aasList.DeleteContents( true );
	aasNames.Clear();

	botActions.DeleteContents( true );
	botRoutes.DeleteContents( true );
	botObstacles.DeleteContents( true );
	botLocationRemap.DeleteContents( true );

	botVehicleNodes.Clear();

	actionHash.Clear();
	routeHash.Clear();

	if ( clearAll ) {
        memset( bots, 0, sizeof( bots ) );
	}

	threadingEnabled = false;

	actionsLoaded = false;
	forwardSpawnsChecked = false;

    memset( worldStateBuffer, 0, sizeof( worldStateBuffer ) );
    memset( outputStateBuffer, 0, sizeof( outputStateBuffer ) );

	currentWorldState = 0;
	currentOutputState = 0;

	gameWorldState = worldStateBuffer;
	gameOutputState = outputStateBuffer;

	botWorldState = worldStateBuffer;
	botOutputState = outputStateBuffer;

	botFPS = 0;
}

/*
================
idBotThreadData::Init
================
*/
void idBotThreadData::Init() {
	clip = &gameLocal.clip;
	GetGameWorldState()->botGoalInfo.carryableObjs.Memset( 0 );
	GetGameWorldState()->smokeGrenades.Memset( 0 );
	GetGameWorldState()->chargeInfo.Memset( 0 );
	GetGameWorldState()->spawnHosts.Memset( 0 );
	GetGameWorldState()->deployableInfo.Memset( 0 );
	GetGameWorldState()->vehicleInfo.Memset( 0 );
	GetGameWorldState()->playerBodies.Memset( 0 );
	GetGameWorldState()->stroyBombs.Memset( 0 );
	GetGameWorldState()->gameLocalInfo.maxVehicleHeight = gameLocal.flightCeilingUpper - 512.0f;
	GetGameWorldState()->gameLocalInfo.crouchViewHeight = pm_crouchviewheight.GetFloat();
	GetGameWorldState()->gameLocalInfo.proneViewHeight = pm_proneviewheight.GetFloat();
	GetGameWorldState()->gameLocalInfo.normalViewHeight = pm_normalviewheight.GetFloat();
	GetGameWorldState()->botGoalInfo.team_GDF_criticalClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.attackingTeam = NOTEAM;
	GetGameWorldState()->botGoalInfo.team_GDF_PrimaryAction = ACTION_NULL;
	GetGameWorldState()->botGoalInfo.team_STROGG_PrimaryAction = ACTION_NULL;
	GetGameWorldState()->botGoalInfo.team_GDF_SecondaryAction = ACTION_NULL;
	GetGameWorldState()->botGoalInfo.team_STROGG_SecondaryAction = ACTION_NULL;
	GetGameWorldState()->botGoalInfo.team_GDF_AttackDeployables = true;
	GetGameWorldState()->botGoalInfo.team_STROGG_AttackDeployables = true;
	GetGameWorldState()->botGoalInfo.botSightDist = ENEMY_SIGHT_DIST; //mal: nice default value
	GetGameWorldState()->botGoalInfo.botGoal_MCP_VehicleNum = -1;
	GetGameWorldState()->botGoalInfo.mapHasMCPGoal = false;
	GetGameWorldState()->botGoalInfo.mapHasMCPGoalTime = 0;
	GetGameWorldState()->botGoalInfo.gameIsOnFinalObjective = false;
	GetGameWorldState()->botGoalInfo.isTrainingMap = false;
	GetGameWorldState()->gameLocalInfo.winningTeam = NOTEAM;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].criticalClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].numCriticalClass = 0;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ GDF ].donatingClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.teamRetreatInfo[ GDF ].retreatTime = 0;
	GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].teamUsesSpawn = false;
	GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].percentage = 20;
	GetGameWorldState()->gameLocalInfo.teamMineInfo[ GDF ].isPriority = true;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].criticalClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].numCriticalClass = 0;
	GetGameWorldState()->botGoalInfo.teamNeededClassInfo[ STROGG ].donatingClass = NOCLASS;
	GetGameWorldState()->botGoalInfo.teamRetreatInfo[ STROGG ].retreatTime = 0;
	GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].teamUsesSpawn = false;
	GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].percentage = 20;
	GetGameWorldState()->gameLocalInfo.teamMineInfo[ STROGG ].isPriority = true;
	GetGameWorldState()->botGoalInfo.deliverActionNumber = FindDeliverActionNumber();

        checkedLastMapStageCovertWeapons = false;

	lastCmdDeclinedChatTime = 0;
	lastWeapChangedTime = 0;
	nextBotClassUpdateTime = 0;
	nextDeployableUpdateTime = 0;
	nextChatUpdateTime = 0;
	packUpdateTime = 0;
	teamsSwapedRecently = false;
	ignoreActionNumber = ACTION_NULL;

	actorMissionInfo.actionNumber = ACTION_NULL;
	actorMissionInfo.hasBriefedPlayer = false;
	actorMissionInfo.deployableStageIsActive = false;
	actorMissionInfo.targetClientNum = -1;
	actorMissionInfo.actorClientNum = -1;
	actorMissionInfo.setup = false;
	actorMissionInfo.chatPauseTime = 0;
	actorMissionInfo.playerNeedsFinalBriefing = false;
	actorMissionInfo.playerIsOnFinalMission = false;
	actorMissionInfo.goalPauseTime = 0;
	actorMissionInfo.setupBotNames = true;
	actorMissionInfo.forwardSpawnIsAllowed = false;
	actorMissionInfo.hasEnteredActorNode = false;
}

/*
================
idBotThreadData::DebugOutput

My dump heap for all things debug related.
================
*/
void idBotThreadData::DebugOutput() {

	idPlayer* player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	//mal: debug the aas!
	if ( !IsThreadingEnabled() ) {
		idAAS *aas = GetAAS( aas_test.GetInteger() );
		if ( aas != NULL ) {
			aas->Test( player->GetPhysics()->GetOrigin() );
		}
		// test an obstacle avoidance query
		if ( bot_testObstacleQuery.GetString()[0] != '\0' ) {
			idObstacleAvoidance obstacleAvoidance;
			obstacleAvoidance.TestQuery( bot_testObstacleQuery.GetString(), aas );
		}

		if ( bot_debugPersonalVehicles.GetBool() ) {
			for( int i = 0; i < botActions.Num(); i++ ) {
				botActions[ i ]->actionVehicleFlags = 1;
			}
		}
	}

	//mal: debug actions!
	DrawActions();

	if ( bot_drawObstacles.GetBool() ) {
		DrawDynamicObstacles();
	}

	if ( bot_drawDefuseHints.GetBool() ) {
		DrawDefuseHints();
	}

	if ( bot_drawRearSpawnLocations.GetBool() ) {
		DrawRearSpawns();
	}

	botThreadData.botVehicleNodes.DrawNodes();

 	if ( bot_drawClientNumbers.GetBool() ) {
		DrawNumbers();
	}

	DrawActionPaths();

//	common->Printf("Is Leaning = %i\n", player->GetPlayerPhysics().IsLeaning() );

//	common->Printf("Locked On = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].enemyHasLockon );

//	idVec3 vec = GetGameWorldState()->clientInfo[ 1 ].origin - GetGameWorldState()->clientInfo[ player->entityNumber ].origin;
//	common->Printf("Dist = %f\n", vec.LengthFast() );

//	common->Printf("Target = %i     Target Locked = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].targetLockEntNum, GetGameWorldState()->clientInfo[ player->entityNumber ].targetLocked  );
//	common->Printf("Is Scoped = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].weapInfo.isScopeUp );

//	common->Printf("Fire Support = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].fireSupportChargedUsed );

//	common->Printf("Needs Reload = %i     Can Reload = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].weapInfo.primaryWeapNeedsReload, GetGameWorldState()->clientInfo[ player->entityNumber ].weapInfo.hasAmmoForReload );

//	common->Printf("Weapon = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].weapInfo.weapon );

//	common->Printf("Smoke State = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].deviceChargeUsed );

//	common->Printf("Bomb State = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].weapInfo.bombState );

//	common->Printf("Speed = %f\n", GetGameWorldState()->clientInfo[ player->entityNumber ].xySpeed );


//	common->Printf("Weapon Ready = %i\n", GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weaponIsReady ); 

//	if ( player->GetProxyEntity() != NULL ) {
//		float proxySpeed = player->GetProxyEntity()->GetPhysics()->GetLinearVelocity() * player->GetProxyEntity()->GetRenderEntity()->axis[ 0 ];
//      common->Printf( "Forward Speed = %f\n", proxySpeed );
//		float vehicleYaw = player->GetProxyEntity()->GetRenderEntity()->axis.ToAngles().yaw;
//		float vehiclePitch = player->GetProxyEntity()->GetRenderEntity()->axis.ToAngles().pitch;
//		float vehicleRoll = player->GetProxyEntity()->GetRenderEntity()->axis.ToAngles().roll;
//		int	  cmdPitch = player->usercmd.angles[ PITCH ];
//		common->Printf("Cmd Pitch = %i\n", cmdPitch );
//		common->Printf("Pitch = %f, Yaw = %f, Roll = %f\n", vehiclePitch, vehicleYaw, vehicleRoll );
//	}

//	if ( player->GetProxyEntity() != NULL ) {
//		idBox box = idBox( player->GetProxyEntity()->GetPhysics()->GetBounds(), player->GetProxyEntity()->GetPhysics()->GetOrigin(), player->GetProxyEntity()->GetPhysics()->GetAxis() );
//		float expand = ( player->GetProxyEntity()->GetPhysics()->GetLinearVelocity() *  player->GetProxyEntity()->GetRenderEntity()->axis[ 0 ] ) * 2.0f;
//		box.ExpandSelf( idMath::Fabs( expand * 0.5f ), 0.0f, 0.0f );
//		box.TranslateSelf( box.GetAxis()[0] * expand * 0.5f );
//		gameRenderWorld->DebugBox( colorGreen, box, 16 );
//	}

/*
	idPlayer* botPlayer1 = gameLocal.GetClient( 1 );
	idPlayer* botPlayer2 = gameLocal.GetClient( 2 );

	if ( botPlayer1 != NULL && botPlayer2 != NULL ) {
		bool humanCanSeeBot = botPlayer1->IsObscuredBySmoke( player );
		bool botCanSeeHuman = player->IsObscuredBySmoke( botPlayer1 );
	
		bool nextBotCanSeeHuman = player->IsObscuredBySmoke( botPlayer2 );
		bool botCanSeeNextBot = botPlayer2->IsObscuredBySmoke( botPlayer1 );
		bool nextBotCanSeeBot = botPlayer1->IsObscuredBySmoke( botPlayer2 );
		bool humanCanSeeNextBot = botPlayer2->IsObscuredBySmoke( player );

		gameLocal.Printf("I can't see human: %i, human can't see me: %i\n", botCanSeeHuman, humanCanSeeBot );

		if ( humanCanSeeBot != botCanSeeHuman || botCanSeeNextBot != nextBotCanSeeBot || humanCanSeeNextBot != nextBotCanSeeHuman ) {
			assert( false );
		}
	}
*/
//	gameLocal.Printf("Charge: %f   Altitude: %f\n", GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.boostCharge, GetGameWorldState()->clientInfo[ player->entityNumber ].altitude );
/*
	trace_t tr;
	idVec3 end = player->firstPersonViewOrigin;
	end += ( 8092 * player->firstPersonViewAxis[ 0 ] );
	gameLocal.TracePoint( tr, player->firstPersonViewOrigin, end, MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_RENDERMODEL );
	gameLocal.Printf("Trace Entity: %i\n", tr.c.entityNum );
*/

/*
	bool isClear = true;

	for( int i = 0; i < botThreadData.botObstacles.Num(); i++ ) {
		
		idBotObstacle *obstacle = botThreadData.botObstacles[ i ];
		idVec3 vec = obstacle->bbox.GetCenter() - player->GetPhysics()->GetOrigin();

		float distSqr = vec.LengthSqr();
		distSqr -= obstacle->bbox.GetExtents().LengthSqr();

		if ( distSqr > Square( 100.0f ) ) {
			continue;
		}

		isClear = false;
		break;
	}

	common->Printf("IsClear = %i\n", isClear );
*/
}

/*
================
idBotThreadData::LoadMap
================
*/
void idBotThreadData::LoadMap( const char* mapName, int randSeed ) {
	random.SetSeed( randSeed );

//mal: here, setup the various timer globals so that the bots can see and use them.
//mal_NOTE: look at "etqw/base/defs/misc.def" to find all the diff key values.

	const idDeclEntityDef* constants = gameLocal.declEntityDefType[ "globalConstants" ];

	if ( constants == NULL ) {
		gameLocal.Error( "Failed to find 'globalConstants' in /defs/misc.def" );
	}

	GetGameWorldState()->gameLocalInfo.energyTimerTime = constants->dict.GetFloat( "energy_timer_time" );
	GetGameWorldState()->gameLocalInfo.bombTimerTime = constants->dict.GetFloat( "energy_charge_time" );
	GetGameWorldState()->gameLocalInfo.fireSupportTimerTime = constants->dict.GetFloat( "energy_firesupport_time" );
	GetGameWorldState()->gameLocalInfo.deviceTimerTime = constants->dict.GetFloat( "energy_device_time" );
	GetGameWorldState()->gameLocalInfo.chargeExplodeTime = constants->dict.GetFloat( "charge_explode_time" );
	GetGameWorldState()->gameLocalInfo.supplyTimerTime = constants->dict.GetFloat( "energy_supply_time" );
	GetGameWorldState()->gameLocalInfo.deployTimerTime = constants->dict.GetFloat( "energy_deployment_time" );

	if ( idStr::Icmp( mapName, "maps/area22.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = AREA22;
	} else if ( idStr::Icmp( mapName, "maps/ark.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = ARK;
	} else if ( idStr::Icmp( mapName, "maps/canyon.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = CANYON;
	} else if ( idStr::Icmp( mapName, "maps/island.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = ISLAND;
	} else if ( idStr::Icmp( mapName, "maps/outskirts.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = OUTSKIRTS;
	} else if ( idStr::Icmp( mapName, "maps/quarry.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = QUARRY;
	} else if ( idStr::Icmp( mapName, "maps/refinery.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = REFINERY;
	} else if ( idStr::Icmp( mapName, "maps/salvage.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = SALVAGE;
	} else if ( idStr::Icmp( mapName, "maps/sewer.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = SEWER;
	} else if ( idStr::Icmp( mapName, "maps/slipgate.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = SLIPGATE;
	} else if ( idStr::Icmp( mapName, "maps/valley.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = VALLEY;
	} else if ( idStr::Icmp( mapName, "maps/volcano.entities" ) == 0 ) {
		GetGameWorldState()->gameLocalInfo.gameMap = VOLCANO;
	} else {
		GetGameWorldState()->gameLocalInfo.gameMap = UNKNOWN_MAP;
	}
}

/*
================
idBotThreadData::SetCurrentGameWorldState
================
*/
void idBotThreadData::SetCurrentGameWorldState() {
	// advance to the next world state
	int nextWorldState = ( currentWorldState + 1 ) & (BOT_THREAD_BUFFER_SIZE-1);
	memcpy( &worldStateBuffer[nextWorldState], &worldStateBuffer[currentWorldState], sizeof( worldStateBuffer[0] ) );
	currentWorldState = nextWorldState;
	gameWorldState = &worldStateBuffer[currentWorldState];
}

/*
================
idBotThreadData::SetCurrentGameOutputState
================
*/
void idBotThreadData::SetCurrentGameOutputState() {
	// set pointer to the previous bot output state
	int prevOutputState = ( currentOutputState + (BOT_THREAD_BUFFER_SIZE-1) ) & (BOT_THREAD_BUFFER_SIZE-1);
	gameOutputState = &outputStateBuffer[prevOutputState];
}

/*
================
idBotThreadData::SetCurrentBotWorldState
================
*/
void idBotThreadData::SetCurrentBotWorldState() {
	// set pointer to the previous world state
	int prevWorldState = ( currentWorldState + (BOT_THREAD_BUFFER_SIZE-1) ) & (BOT_THREAD_BUFFER_SIZE-1);
	botWorldState = &worldStateBuffer[prevWorldState];
}

/*
================
idBotThreadData::SetCurrentBotOutputState
================
*/
void idBotThreadData::SetCurrentBotOutputState() {
	// advance to the next bot output state
	int nextOutputState = ( currentOutputState + 1 ) & (BOT_THREAD_BUFFER_SIZE-1);
	memcpy( &outputStateBuffer[nextOutputState], &outputStateBuffer[currentOutputState], sizeof( outputStateBuffer[0] ) );
	currentOutputState = nextOutputState;
	botOutputState = &outputStateBuffer[currentOutputState];
}

/*
=====================
idBotThreadData::RemoveBot
=====================
*/
void idBotThreadData::RemoveBot( int entityNumber ) {
	botThread->Lock();

	idBotAI *botAI = botThreadData.bots[ entityNumber ];

	// set bot AI pointer to NULL
	botThreadData.bots[ entityNumber ] = NULL;

	if ( botAI != NULL ) {
		botAI->nextRemoved = removedBotAIs;
		removedBotAIs = botAI;
	}

	botThread->UnLock();
}

/*
================
idBotThreadData::DeleteBots
================
*/
void idBotThreadData::DeleteBots() {
	botThread->Lock();

	// remove any bots for real
	for ( idBotAI *nextBotAI = NULL, *botAI = removedBotAIs; botAI != NULL; botAI = nextBotAI ) {
		nextBotAI = botAI->nextRemoved;
		delete botAI;
	}
	removedBotAIs = NULL;

	botThread->UnLock();
}

/*
================
idBotThreadData::ApplyPendingAreaChanges
================
*/
void idBotThreadData::ApplyPendingAreaChanges() {
	botThread->Lock();

	for ( int i = 0; i < pendingAreaChanges.Num(); i++ ) {
		for ( int j = 0; j < botThreadData.NumAAS(); j++ ) {
			idAAS * aas = botThreadData.GetAAS( j );
			if ( aas == NULL ) {
				continue;
			}
			botAreaChange_t &change = pendingAreaChanges[i];
			aas->ChangeAreaTravelFlags( change.bounds, change.areaFlags, change.travelFlags, change.set );
		}
	}
	pendingAreaChanges.SetNum( 0, false );

	botThread->UnLock();
}

/*
================
idBotThreadData::EnableArea
================
*/
void idBotThreadData::EnableArea( const idBounds &bounds, int areaFlags, int team, bool enable ) {
	botThread->Lock();

	botAreaChange_t change;

	change.bounds = bounds;
	change.areaFlags = areaFlags;
	change.set = !enable;

	switch( team ) {
		case 0: change.travelFlags = TFL_INVALID_GDF; break;
		case 1: change.travelFlags = TFL_INVALID_STROGG; break;
		case 2: change.travelFlags = TFL_INVALID; break;
		default: change.travelFlags = TFL_INVALID; break;
	}

	pendingAreaChanges.Append( change );

	botThread->UnLock();
}

/*
================
idBotThreadData::ApplyPendingReachabilityChanges
================
*/
void idBotThreadData::ApplyPendingReachabilityChanges() {
	botThread->Lock();

	for ( int i = 0; i < pendingReachChanges.Num(); i++ ) {
		for ( int j = 0; j < botThreadData.NumAAS(); j++ ) {
			idAAS * aas = botThreadData.GetAAS( j );
			if ( aas == NULL ) {
				continue;
			}
			botReachChange_t &change = pendingReachChanges[i];
			aas->ChangeReachabilityTravelFlags( change.name, change.travelFlags, change.set );
		}
	}
	pendingReachChanges.SetNum( 0, false );

	botThread->UnLock();
}

/*
================
idBotThreadData::DisableAASAreaInLocation
================
*/
void idBotThreadData::DisableAASAreaInLocation( int aasType, const idVec3& location ) {
	if ( aasType < AAS_PLAYER || aasType > AAS_VEHICLE ) {
		gameLocal.DWarning( "Invalid AAS type passed to \"disableAASAreaInLocation\"\nValid values are 0 = player aas, 1 = vehicle aas" );
		return;
	}

	idAAS * aas = botThreadData.GetAAS( aasType );
	
	if ( aas == NULL ) {
		gameLocal.DWarning( "AAS type %i can't be found in call to \"disableAASAreaInLocation\"", aasType );
		return;
	}

	int areaNum = aas->PointReachableAreaNum( location, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, 0 );

	if ( areaNum == 0 ) {
		gameLocal.DWarning( "No aas area found at location X: %.0f Y: %.0f Z: %.0f in call to \"disableAASAreaInLocation\"", location.x, location.y, location.z );
		return;
	}

	aas->SetAreaTravelFlags( areaNum, TFL_INVALID );

	gameLocal.DPrintf( "Disabled areaNum %i at location X: %.0f Y: %.0f Z: %.0f\n", areaNum, location.x, location.y, location.z );
}

/*
================
idBotThreadData::EnableReachability
================
*/
void idBotThreadData::EnableReachability( const char *name, int team, bool enable ) {
	botThread->Lock();

	botReachChange_t change;

	change.name = name;
	change.set = !enable;

	switch( team ) {
		case 0: change.travelFlags = TFL_INVALID_GDF; break;
		case 1: change.travelFlags = TFL_INVALID_STROGG; break;
		case 2: change.travelFlags = TFL_INVALID; break;
		default: change.travelFlags = TFL_INVALID; break;
	}

	pendingReachChanges.Append( change );

	botThread->UnLock();
}

#define DYNAMIC_OBSTACLE_CULL_DISTANCE		4096
#define DYNAMIC_OBSTACLE_CONSIDER_DISTANCE	256

/*
================
idBotThreadData::BuildObstacleList
================
*/
bool idBotThreadData::BuildObstacleList( idObstacleAvoidance &obstacleAvoidance, const idVec3 &origin, const int areaNum, bool inVehicle ) const {

	obstacleAvoidance.ClearObstacles();

	for( int i = 0; i < botObstacles.Num(); i++ ) {
		idBotObstacle *obstacle = botObstacles[ i ];

		idVec3 vec = obstacle->bbox.GetCenter() - origin;
		float radiusSqr = obstacle->bbox.GetExtents().LengthSqr();
		float avoidObstacleRangeSqr;

		// mal: if in a vehicle, avoid better!
		if ( inVehicle ) {
			avoidObstacleRangeSqr = radiusSqr + Square( 512.0f );
		} else {
			avoidObstacleRangeSqr = radiusSqr + Square( 256.0f );
		}

		if ( vec.LengthSqr() > avoidObstacleRangeSqr ) {
			continue;
		}

		obstacleAvoidance.AddObstacle( obstacle->bbox, obstacle->num );
	}

	for( int i = 0; i < MAX_VEHICLES; i++ ) {
		const proxyInfo_t &vehicleInfo = GetBotWorldState()->vehicleInfo[ i ];
	
		if ( vehicleInfo.entNum == 0 ) {
			continue;
		}

		idVec3 vec = vehicleInfo.origin - origin;

		if ( vec.LengthSqr() > Square( DYNAMIC_OBSTACLE_CULL_DISTANCE ) ) {
			continue;
		}

		//mal: enlarge the box based on the vehicles speed.
		float expand = vehicleInfo.forwardSpeed * 2.0f;
		idBox box( vehicleInfo.bbox, vehicleInfo.origin, vehicleInfo.axis );
		box.ExpandSelf( idMath::Fabs( expand * 0.5f ), 0.0f, 0.0f );
		box.TranslateSelf( box.GetAxis()[0] * expand * 0.5f );
	
		if ( !box.Expand( DYNAMIC_OBSTACLE_CONSIDER_DISTANCE ).ContainsPoint( origin ) ) {
			continue;
		}

		obstacleAvoidance.AddObstacle( box, vehicleInfo.entNum );
	}

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		const deployableInfo_t& deployable = GetBotWorldState()->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.health <= 0 ) {
			continue;
		}

		if ( !deployable.inPlace ) {
			continue;
		}

		if ( deployable.ownerClientNum == -1 ) {
			//continue;
		}

		idVec3 vec = deployable.origin - origin;

		if ( vec.LengthSqr() > Square( 1024.0f ) ) {
			continue;
		}

		idBox box( deployable.bbox, deployable.origin, deployable.axis );

		obstacleAvoidance.AddObstacle( box, deployable.entNum );
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( gameLocal.entities[i] == NULL ) {
			continue;
		}

		// skip the local player
		if ( gameLocal.GetLocalPlayer()->entityNumber == i ) {
			continue;
		}

		const clientInfo_t& playerInfo = GetBotWorldState()->clientInfo[ i ];

		if ( playerInfo.health <= 0 ) {
			continue;
		}

		//mal: we'll test vehicles seperately.
		if ( playerInfo.proxyInfo.entNum != CLIENT_HAS_NO_VEHICLE ) {
			continue;
		}

		//mal: ignore players outside the valid AAS space
		if ( playerInfo.areaNum == 0 ) {
			continue;
		}

		idVec3 vec = playerInfo.origin - origin;

		//mal: keep the range pretty tight.
		if ( vec.LengthSqr() > Square( 1024.0f ) ) {
			continue;
		}

		idBox box = idBox( playerInfo.localBounds, playerInfo.origin, playerInfo.bodyAxis );

		obstacleAvoidance.AddObstacle( box, i );
	}

	return true;
}

/*
================
idBotThreadData::Printf
================
*/
void idBotThreadData::Printf( const char *fmt, ... ) {
	if ( !bot_debug.GetBool() || IsThreadingEnabled() ) {
		return;
	}

	va_list argptr;
	va_start( argptr, fmt );
	common->VPrintf( fmt, argptr );
	va_end( argptr );
}

/*
================
idBotThreadData::Warning
================
*/
void idBotThreadData::Warning( const char *fmt, ... ) {
	if ( !bot_debug.GetBool() || IsThreadingEnabled() ) {
		return;
	}

	va_list		argptr;
	char		msg[2048];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );
	msg[ sizeof( msg ) - 1 ] = 0;

	common->Printf( S_COLOR_YELLOW "WARNING: " S_COLOR_RED "%s\n", msg );
}

/*
================
idBotThreadData::DynamicEntity_Think

Tracks things like health/ammo/supply packs, deployables, etc.
================
*/
void idBotThreadData::DynamicEntity_Think() {

	int i, j, k;
	int areaNum;

	GetGameWorldState()->botGoalInfo.botGoal_MCP_VehicleNum = -1;

//mal: packs first....
	for( j = 0; j < MAX_CLIENTS; j++ ) {
        for( i = 0; i < MAX_ITEMS; i++ ) {
            if ( GetGameWorldState()->clientInfo[ j ].packs[ i ].entNum <= 0 ) {
				continue;
			}

			idEntity *pack = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].packs[ i ].entNum ];

			if ( pack != NULL && gameLocal.GetSpawnId( pack ) == GetGameWorldState()->clientInfo[ j ].packs[ i ].spawnID ) {
				GetGameWorldState()->clientInfo[ j ].packs[ i ].origin = pack->GetPhysics()->GetOrigin();
				GetGameWorldState()->clientInfo[ j ].packs[ i ].xySpeed = pack->GetPhysics()->GetLinearVelocity().LengthFast();
				GetGameWorldState()->clientInfo[ j ].packs[ i ].inWater = ( pack->GetPhysics()->InWater() > 0.0f ) ? true : false;

//mal: cache off the AAS areaNum data, so the bots understand if its reachable or not. Also, check if the pack is in the play zone or not.
				if ( GetGameWorldState()->clientInfo[ j ].packs[ i ].xySpeed == 0.0f ) {
					if ( GetGameWorldState()->clientInfo[ j ].packs[ i ].checkedAreaNum == false ) {
						const sdPlayZone* playZone = NULL;
						playZone = gameLocal.GetPlayZone( pack->GetPhysics()->GetOrigin(), sdPlayZone::PZF_PLAYZONE );
	
						if ( playZone != NULL ) {
							const sdDeployMaskInstance* deployMask = playZone->GetMask( gameLocal.GetPlayZoneMask() );
							if ( deployMask == NULL || deployMask->IsValid( pack->GetPhysics()->GetAbsBounds() ) == DR_CLEAR ) {
								GetGameWorldState()->clientInfo[ j ].packs[ i ].inPlayZone = true;
							}
						}

						GetGameWorldState()->clientInfo[ j ].packs[ i ].areaNum = Nav_GetAreaNum( AAS_PLAYER, pack->GetPhysics()->GetOrigin() );
						GetGameWorldState()->clientInfo[ j ].packs[ i ].checkedAreaNum = true;
					}

					if ( packUpdateTime < gameLocal.time ) {
						bool allClear = true;
						idVec3 packOrg = pack->GetPhysics()->GetOrigin();
						idVec3 end = packOrg;
						end.z += 82.0f;
						idBounds packBounds = pack->GetPhysics()->GetAbsBounds();
						packBounds.AddPoint( end );
						idEntity* entityList[ 128 ];
						int count = gameLocal.clip.EntitiesTouchingBounds( packBounds, MASK_VEHICLESOLID | CONTENTS_MONSTER | MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX, entityList, 128, true );

						for ( int f = 0; f < count; f++ ) {
							idEntity* other = entityList[ f ];

							if ( other == pack ) {
								continue;
							}
							allClear = false;
							break;
						}

						GetGameWorldState()->clientInfo[ j ].packs[ i ].available = allClear;
					}
				} else {
					GetGameWorldState()->clientInfo[ j ].packs[ i ].checkedAreaNum = false;
				}
			} else {
				GetGameWorldState()->clientInfo[ j ].packs[ i ].entNum = 0;
				GetGameWorldState()->clientInfo[ j ].packs[ i ].spawnID = -1;
				GetGameWorldState()->clientInfo[ j ].packs[ i ].available = false;
				GetGameWorldState()->clientInfo[ j ].packs[ i ].inPlayZone = false;

				if ( bot_debug.GetBool() ) {
					gameLocal.DWarning( "DynamicEntity_Think called a non-existent supply item!" );
				}
			}
		}
	}

	if ( packUpdateTime < gameLocal.time ) {
		packUpdateTime = gameLocal.time + 5000;
	}

//mal: supply crates next....
	for( j = 0; j < MAX_CLIENTS; j++ ) {
		if ( GetGameWorldState()->clientInfo[ j ].supplyCrate.entNum <= 0 ) {
			continue;
		}

		idEntity *crate = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].supplyCrate.entNum ];

		if ( crate != NULL && gameLocal.GetSpawnId( crate ) == GetGameWorldState()->clientInfo[ j ].supplyCrate.spawnID ) {
            GetGameWorldState()->clientInfo[ j ].supplyCrate.origin = crate->GetPhysics()->GetOrigin();
			GetGameWorldState()->clientInfo[ j ].supplyCrate.xySpeed = crate->GetPhysics()->GetLinearVelocity().LengthFast();
			GetGameWorldState()->clientInfo[ j ].supplyCrate.inWater = ( crate->GetPhysics()->InWater() > 0.0f ) ? true : false;
	
//mal: cache off the AAS areaNum data, so the bots understand if its reachable or not.
            if ( GetGameWorldState()->clientInfo[ j ].supplyCrate.xySpeed == 0.0f ) {
				if ( GetGameWorldState()->clientInfo[ j ].supplyCrate.checkedAreaNum == false ) {
					GetGameWorldState()->clientInfo[ j ].supplyCrate.areaNum = Nav_GetAreaNum( AAS_PLAYER, crate->GetPhysics()->GetOrigin() );
					GetGameWorldState()->clientInfo[ j ].supplyCrate.checkedAreaNum = true;
				}
			} else {
				GetGameWorldState()->clientInfo[ j ].supplyCrate.checkedAreaNum = false;
			}
		} else {
			GetGameWorldState()->clientInfo[ j ].supplyCrate.entNum = 0;
			GetGameWorldState()->clientInfo[ j ].supplyCrate.spawnID = -1;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent supply crate!" );
			}
		}
	}

//mal: track airstrikes next...
	for( j = 0; j < MAX_CLIENTS; j++ ) {
		if ( GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.entNum <= 0 ) {
			continue;
		}

		idEntity *airStrike = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.entNum ];

		if ( airStrike != NULL && gameLocal.GetSpawnId( airStrike ) == GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.spawnID ) {
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.origin = airStrike->GetPhysics()->GetOrigin();
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.xySpeed = airStrike->GetPhysics()->GetLinearVelocity().LengthFast();
		} else {
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.entNum = 0;
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.timeTilStrike = 0;
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.origin = vec3_zero;
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.xySpeed = 0.0f;
			GetGameWorldState()->clientInfo[ j ].weapInfo.airStrikeInfo.spawnID = -1;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent airstrike!" );
			}
		}
	}

//mal: track force shields next...
	for( j = 0; j < MAX_CLIENTS; j++ ) {
		for( i = 0; i < MAX_SHIELDS; i++ ) {
			if ( GetGameWorldState()->clientInfo[ j ].forceShields[ i ].entNum <= 0 ) {
				continue;
			}

			idEntity *forceShield = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].forceShields[ i ].entNum ];

			if ( forceShield != NULL && gameLocal.GetSpawnId( forceShield ) == GetGameWorldState()->clientInfo[ j ].forceShields[ i ].spawnID ) {
				GetGameWorldState()->clientInfo[ j ].forceShields[ i ].origin = forceShield->GetPhysics()->GetOrigin();
			} else {
				GetGameWorldState()->clientInfo[ j ].forceShields[ i ].entNum = 0;
				GetGameWorldState()->clientInfo[ j ].forceShields[ i ].spawnID = -1;
				GetGameWorldState()->clientInfo[ j ].forceShields[ i ].origin = vec3_zero;

				if ( bot_debug.GetBool() ) {
					gameLocal.DWarning( "DynamicEntity_Think called a non-existent force shield!" );
				}
			}
		}
	}

//mal: track stroybombs next...
	for( j = 0; j < MAX_STROYBOMBS; j++ ) {
		if ( GetGameWorldState()->stroyBombs[ j ].entNum <= 0 ) {
			continue;
		}

		idEntity *stroyBomb = gameLocal.entities[ GetGameWorldState()->stroyBombs[ j ].entNum ];

		if ( stroyBomb != NULL && gameLocal.GetSpawnId( stroyBomb ) == GetGameWorldState()->stroyBombs[ j ].spawnID ) {
			GetGameWorldState()->stroyBombs[ j ].xySpeed = stroyBomb->GetPhysics()->GetLinearVelocity().LengthFast();
			GetGameWorldState()->stroyBombs[ j ].origin = stroyBomb->GetPhysics()->GetOrigin();
		} else {
			GetGameWorldState()->stroyBombs[ j ].entNum = 0;
			GetGameWorldState()->stroyBombs[ j ].spawnID = -1;
			GetGameWorldState()->stroyBombs[ j ].origin = vec3_zero;
			GetGameWorldState()->stroyBombs[ j ].xySpeed = 0.0f;
		}
	}

//mal: track hives/3rd eye cameras next...
	for( j = 0; j < MAX_CLIENTS; j++ ) {
		if ( GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.entNum <= 0 ) {
			continue;
		}

		idEntity *covertTool = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.entNum ];

		if ( covertTool != NULL && gameLocal.GetSpawnId( covertTool ) == GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.spawnID ) {
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.origin = covertTool->GetPhysics()->GetOrigin();
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.xySpeed = covertTool->GetPhysics()->GetLinearVelocity().LengthFast();
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.axis = covertTool->GetPhysics()->GetAxis();
		} else {
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.entNum = 0;
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.origin = vec3_zero;
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.xySpeed = 0.0f;
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.clientIsUsing = false;
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.spawnID = -1;
			GetGameWorldState()->clientInfo[ j ].weapInfo.covertToolInfo.axis = mat3_identity;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent covert tool!" );
			}
		}
	}

	idAAS *aas = botThreadData.GetAAS( AAS_PLAYER );

//mal: update charges next.
	for( j = 0; j < MAX_CLIENT_CHARGES; j++ ) {
		if ( GetGameWorldState()->chargeInfo[ j ].entNum <= 0 ) {
			continue;
		}

		idEntity *charge = gameLocal.entities[ GetGameWorldState()->chargeInfo[ j ].entNum ];

		if ( charge != NULL && gameLocal.GetSpawnId( charge ) == GetGameWorldState()->chargeInfo[ j ].spawnID ) {

			if ( AllowDebugData() && GetGameWorldState()->chargeInfo[ j ].checkedAreaNum ) {
				idVec4 colorType = colorGreen;
				if ( GetGameWorldState()->chargeInfo[ j ].areaNum == 0 ) {
					colorType = colorRed;
				}

				gameRenderWorld->DebugBounds( colorType, charge->GetPhysics()->GetBounds(), charge->GetPhysics()->GetOrigin(), charge->GetPhysics()->GetAxis() );
			}

			if ( !GetGameWorldState()->chargeInfo[ j ].checkedAreaNum && aas != NULL ) {
				playerTeamTypes_t playerTeam = GetGameWorldState()->chargeInfo[ j ].team;
				int excludeTravelFlags = ( playerTeam == GDF ) ? ( TFL_INVALID | TFL_INVALID_GDF ) : ( TFL_INVALID | TFL_INVALID_STROGG );

				idVec3 chargeOrigin = charge->GetPhysics()->GetOrigin();
				botThreadData.RemapLocation( chargeOrigin );
				int areaNum = aas->PointReachableAreaNum( chargeOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );

				bool canReach = true;
				int actionAreaNum;
				int travelFlags;
				int travelTime;
				idVec3 actionOrigin;
				const aasReachability_t *reach;

				int actionNumber = FindActionByTypeForLocation( charge->GetPhysics()->GetOrigin(), ACTION_HE_CHARGE, playerTeam );

				if ( actionNumber != ACTION_NULL ) {
					actionAreaNum = botActions[ actionNumber ]->areaNum;
					travelFlags = ( playerTeam == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;
					travelTime;
					actionOrigin = botActions[ actionNumber ]->GetActionOrigin();
					canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );

					if ( botActions[ actionNumber ]->ActionIsPriority() ) {
						GetGameWorldState()->chargeInfo[ j ].isOnObjective = true;
					}
				}

				if ( areaNum == 0 || !canReach ) {
					idVec3 bombOrigin = charge->GetPhysics()->GetOrigin();
					idMat3 bombAxis = charge->GetPhysics()->GetAxis();
					idVec3 projOrigin = bombOrigin += ( HALF_PLAYER_BBOX * bombAxis[ 0 ] );
					areaNum = aas->PointReachableAreaNum( bombOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );

					if ( actionNumber != ACTION_NULL ) {
						canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );
					}

					if ( areaNum == 0 || !canReach ) { //mal: failed, now scan left and right in 2 half_bbox size steps
						bombOrigin = projOrigin;
						bombOrigin += ( HALF_PLAYER_BBOX * bombAxis[ 1 ] * -1 );
						areaNum = aas->PointReachableAreaNum( bombOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );
						
						if ( actionNumber != ACTION_NULL ) {
							canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );
						}

						if ( areaNum == 0 || !canReach ) {
							bombOrigin += ( HALF_PLAYER_BBOX * bombAxis[ 1 ] * -1 );
							areaNum = aas->PointReachableAreaNum( bombOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );
						
							if ( actionNumber != ACTION_NULL ) {
								canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );
							}

							if ( areaNum == 0 || !canReach ) {
								bombOrigin = projOrigin;
								bombOrigin += ( -HALF_PLAYER_BBOX * bombAxis[ 1 ] * -1 );
								areaNum = aas->PointReachableAreaNum( bombOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );

								if ( actionNumber != ACTION_NULL ) {
									canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );
								}

								if ( areaNum == 0 || !canReach ) {
									bombOrigin += ( -HALF_PLAYER_BBOX * bombAxis[ 1 ] * -1 );
									areaNum = aas->PointReachableAreaNum( bombOrigin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, excludeTravelFlags );

									if ( actionNumber != ACTION_NULL ) {
										canReach = aas->RouteToGoalArea( actionAreaNum, actionOrigin, areaNum, travelFlags, travelTime, &reach );
									}

									if ( areaNum == 0 || !canReach ) { //mal: we're in trouble!
										areaNum = 0;
									}
								}
							}
						}
					}
				}

				GetGameWorldState()->chargeInfo[ j ].areaNum = areaNum;
				GetGameWorldState()->chargeInfo[ j ].checkedAreaNum = true;
			}
		} else {
			GetGameWorldState()->chargeInfo[ j ].entNum = 0;
			GetGameWorldState()->chargeInfo[ j ].state = BOMB_NULL;
			GetGameWorldState()->chargeInfo[ j ].origin = vec3_zero;
			GetGameWorldState()->chargeInfo[ j ].team = NOTEAM;
			GetGameWorldState()->chargeInfo[ j ].ownerSpawnID = -1;
			GetGameWorldState()->chargeInfo[ j ].ownerEntNum = -1;
			GetGameWorldState()->chargeInfo[ j ].spawnID = -1;
			GetGameWorldState()->chargeInfo[ j ].areaNum = 0;
			GetGameWorldState()->chargeInfo[ j ].checkedAreaNum = false;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent charge!" );
			}
		}
	}

//mal: track carryable objectives next
	if( aas != NULL ) {
		for( i = 0; i < MAX_CARRYABLES; i++ ) {
			if ( GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum <= 0 ) {
				continue;
			}

			idEntity *carryableObjective = gameLocal.entities[ GetGameWorldState()->botGoalInfo.carryableObjs[ i ].entNum ];

			if ( carryableObjective != NULL && gameLocal.GetSpawnId( carryableObjective ) == GetGameWorldState()->botGoalInfo.carryableObjs[ i ].spawnID ) {
				GetGameWorldState()->botGoalInfo.carryableObjs[ i ].origin = carryableObjective->GetPhysics()->GetOrigin();
				GetGameWorldState()->botGoalInfo.carryableObjs[ i ].areaNum = aas->PointReachableAreaNum( carryableObjective->GetPhysics()->GetOrigin(), aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, 0 );

				if ( bot_debug.GetBool() ) {
					idVec4 colorType = ( GetGameWorldState()->botGoalInfo.carryableObjs[ i ].areaNum == 0 ) ? colorRed : colorGreen;
					gameRenderWorld->DebugBounds( colorType, carryableObjective->GetPhysics()->GetAbsBounds() );
				}
			} else {
				if ( bot_debug.GetBool() ) {
					gameLocal.DWarning( "DynamicEntity_Think called a non-existent carryable objective!" );
				}
			}
		}
	}

//mal: track smoke grenades next
	for( j = 0; j < MAX_CLIENTS; j++ ) {

		if ( GetGameWorldState()->smokeGrenades[ j ].entNum <= 0 ) {
			continue;
		}

		idEntity *smokeNade = gameLocal.entities[ GetGameWorldState()->smokeGrenades[ j ].entNum ];

		if ( smokeNade != NULL && gameLocal.GetSpawnId( smokeNade ) == GetGameWorldState()->smokeGrenades[ j ].spawnID ) {
			GetGameWorldState()->smokeGrenades[ j ].origin = smokeNade->GetPhysics()->GetOrigin();
			GetGameWorldState()->smokeGrenades[ j ].xySpeed = smokeNade->GetPhysics()->GetLinearVelocity().LengthFast();
		} else {
			GetGameWorldState()->smokeGrenades[ j ].entNum = 0;
			GetGameWorldState()->smokeGrenades[ j ].spawnID = -1;

			if ( bot_debug.GetBool() ) {
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent smoke grenade!" );
			}
		}
	}

//mal: track grenades next
	for( j = 0; j < MAX_CLIENTS; j++ ) {
        for( i = 0; i < MAX_GRENADES; i++ ) {
            if ( GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].entNum <= 0 ) {
				continue;
			}

			idEntity *nade = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].entNum ];

			if ( nade != NULL && gameLocal.GetSpawnId( nade ) == GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].spawnID ) {
				GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].origin = nade->GetPhysics()->GetOrigin();
				GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].xySpeed = nade->GetPhysics()->GetLinearVelocity().LengthFast();
			} else {
				GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].entNum = 0;
				GetGameWorldState()->clientInfo[ j ].weapInfo.grenades[ i ].spawnID = -1;

				if ( bot_debug.GetBool() ) {
					gameLocal.DWarning( "DynamicEntity_Think called a non-existent grenade!\n" );
				}
			}
		}
	}

//mal: track spawnhosts next
	for( j = 0; j < MAX_SPAWNHOSTS; j++ ) {
		if ( GetGameWorldState()->spawnHosts[ j ].entNum <= 0 ) {
			continue;
		}

		idEntity *spawnHost = gameLocal.entities[ GetGameWorldState()->spawnHosts[ j ].entNum ];

		if ( spawnHost != NULL && gameLocal.GetSpawnId( spawnHost ) == GetGameWorldState()->spawnHosts[ j ].spawnID ) {
			if ( !GetGameWorldState()->spawnHosts[ j ].areaChecked ) {
				GetGameWorldState()->spawnHosts[ j ].areaNum = Nav_GetAreaNum( AAS_PLAYER, GetGameWorldState()->spawnHosts[ j ].origin );
			}
		} else {
			GetGameWorldState()->spawnHosts[ j ].entNum = 0;
			GetGameWorldState()->spawnHosts[ j ].spawnID = -1;
			GetGameWorldState()->spawnHosts[ j ].origin = vec3_zero;
			GetGameWorldState()->spawnHosts[ j ].areaChecked = false;
			GetGameWorldState()->spawnHosts[ j ].areaNum = 0;

			if ( bot_debug.GetBool() ) { 
				gameLocal.DWarning( "DynamicEntity_Think called a non-existent spawnhost!\n" );
			}
		}
	}

//mal: track landmines next
	for( j = 0; j < MAX_CLIENTS; j++ ) {
        for( i = 0; i < MAX_MINES; i++ ) {
			if ( GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum <= 0 ) {
				continue;
			}

			idEntity *mine = gameLocal.entities[ GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum ];

			if ( mine != NULL && gameLocal.GetSpawnId( mine ) == GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID ) {
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].origin = mine->GetPhysics()->GetOrigin();
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].xySpeed = mine->GetPhysics()->GetLinearVelocity().LengthFast();
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].bbox = mine->GetPhysics()->GetAbsBounds();

				if ( AllowDebugData() ) {
					idBox bbox = idBox( mine->GetPhysics()->GetAbsBounds() );
				}
			} else {
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].entNum = 0;
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spawnID = -1;
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].state = BOMB_NULL;
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].spotted = false;
				GetGameWorldState()->clientInfo[ j ].weapInfo.landMines[ i ].xySpeed = 0.0f;

				if ( bot_debug.GetBool() ) {
					gameLocal.DWarning( "DynamicEntity_Think called a non-existent landmine!\n" );
				}
			}
		}
	}

//mal: track deployables next.
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
	int numTeam0 = 0;
	idList< int >	deployableActions;

	if ( nextDeployableUpdateTime < gameLocal.time ) {
		nextDeployableUpdateTime = gameLocal.time + 5000;

		for( i = 0; i < botActions.Num(); i++ ) {
			if ( botActions[ i ]->GetBaseActionType() != BASE_ACTION ) {
				continue;
			}

			if ( botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_DROP_DEPLOYABLE && botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_DROP_DEPLOYABLE && 
				botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_DROP_PRIORITY_DEPLOYABLE && botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_DROP_PRIORITY_DEPLOYABLE ) {
				continue;
			}

			deployableActions.Append( i );
		}
	}

	for( j = 0; j < MAX_DEPLOYABLES; j++ ) {
		deployableInfo_t &deployableInfo = GetGameWorldState()->deployableInfo[ j ];
		deployableInfo.entNum = 0;
		deployableInfo.inPlace = false;

		const idList< idEntityPtr< idEntity > > *deployableList = &manager.GetTeamByIndex( 0 ).GetDeployables();
		if ( j >= deployableList->Num() ) {
			deployableList = &manager.GetTeamByIndex( 1 ).GetDeployables();
			numTeam0 = manager.GetTeamByIndex( 0 ).GetDeployables().Num();
		}

		if ( ( j - numTeam0 ) >= deployableList->Num() ) {
			continue;
		}

		const idEntityPtr< idEntity >& deployablePtr = ( *deployableList )[ j - numTeam0 ];

		idEntity *deployable = deployablePtr;
		if ( deployable == NULL ) {
			continue;
		}

		deployableInfo.spawnID = deployablePtr.GetSpawnId();

		sdDefenceTurret* turret = deployable->Cast< sdDefenceTurret >();
		if ( turret != NULL ) {
			idEntity* turretTarget = turret->GetTargetEntity();
			idEntity* turretOwner = turret->GetTurretOwner();
			sdTeamInfo* turretTeam = turret->GetGameTeam();

			deployableInfo.disabled = turret->IsDisabled();
			deployableInfo.inPlace = turret->IsDeployed();  
			deployableInfo.health = turret->GetHealth();
			deployableInfo.maxHealth = turret->GetMaxHealth();
			deployableInfo.origin = turret->GetPhysics()->GetOrigin();
			deployableInfo.entNum = turret->entityNumber;
			deployableInfo.enemyEntNum = turretTarget == NULL ? -1 : turretTarget->entityNumber;
			deployableInfo.axis = turret->GetPhysics()->GetAxis();
			deployableInfo.maxAttackRange = turret->GetTurretMaxRange();
			deployableInfo.minAttackRange = turret->GetTurretMinRange();
			deployableInfo.ownerClientNum = turretOwner == NULL ? -1 : turretOwner->entityNumber;
			deployableInfo.team = turretTeam == NULL ? NOTEAM : turretTeam->GetBotTeam();
			deployableInfo.type = turret->GetTurretType();
			deployableInfo.bbox = turret->GetPhysics()->GetBounds();

			if ( deployableInfo.inPlace ) {
				if ( deployableInfo.ownerClientNum != -1 ) {
					deployableInfo.areaNum = Nav_GetAreaNum( AAS_PLAYER, deployableInfo.origin );
					deployableInfo.areaNumVehicle =	Nav_GetAreaNum( AAS_VEHICLE, deployableInfo.origin );

					if ( deployableInfo.type == APT ) {
						deployableInfo.dangerAreaBox = idBox( deployableInfo.bbox, deployableInfo.origin, deployableInfo.axis );
						deployableInfo.dangerAreaBox.ExpandSelf( 1500.0f, 900.0f, 500.0f );
						deployableInfo.dangerAreaBox.TranslateSelf( deployableInfo.dangerAreaBox.GetAxis()[0] * 1700.0f );
					}
					
					int actionNumber = GetDeployableActionNumber( deployableActions, deployableInfo.origin, deployableInfo.team, deployableInfo.type );
					if ( actionNumber != ACTION_NULL && deployableInfo.ownerClientNum != -1 ) {
						const clientInfo_t& deployableOwner = GetGameWorldState()->clientInfo[ deployableInfo.ownerClientNum ];
						
						if ( !botActions[ actionNumber ]->ActionIsActive() && deployableOwner.isBot ) {
							idEntity *deployableEnt = gameLocal.entities[ deployableInfo.entNum ];

							if ( deployableEnt != NULL ) {
								deployableEnt->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_deployable_destruct" ), 1.0f, NULL );
							}
						}
					}
				}
			}
		} else { //mal: hammers/DMC/etc 

			sdScriptEntity* miscDeployable = deployable->Cast< sdScriptEntity >();
			if ( miscDeployable != NULL ) {
				sdTeamInfo* deployableTeam = miscDeployable->GetGameTeam();
				idEntity* deployableOwner = miscDeployable->GetOwner();


				deployableInfo.origin = miscDeployable->GetPhysics()->GetOrigin();
				deployableInfo.entNum = miscDeployable->entityNumber;
				deployableInfo.health = miscDeployable->GetHealth();
				deployableInfo.maxHealth = miscDeployable->GetMaxHealth();
				deployableInfo.axis = miscDeployable->GetPhysics()->GetAxis();
				deployableInfo.enemyEntNum = -1;
				deployableInfo.minAttackRange = 0.0f;
				deployableInfo.maxAttackRange = miscDeployable->GetDeployableRange();
				deployableInfo.bbox = miscDeployable->GetPhysics()->GetBounds();
				deployableInfo.disabled = miscDeployable->GetIsDisabled();
				deployableInfo.inPlace = miscDeployable->GetIsDeployed();  
				deployableInfo.ownerClientNum = deployableOwner == NULL ? -1 : deployableOwner->entityNumber;
				deployableInfo.team = deployableTeam == NULL ? NOTEAM : deployableTeam->GetBotTeam();

				deployableInfo.type = miscDeployable->GetDeployableType();

				if ( deployableInfo.inPlace ) {
					if ( deployableInfo.ownerClientNum != -1 ) {
						deployableInfo.areaNum = Nav_GetAreaNum( AAS_PLAYER, deployableInfo.origin );
						deployableInfo.areaNumVehicle =	Nav_GetAreaNum( AAS_VEHICLE, deployableInfo.origin );

						int actionNumber = GetDeployableActionNumber( deployableActions, deployableInfo.origin, deployableInfo.team, deployableInfo.type );
						if ( actionNumber != ACTION_NULL && deployableInfo.ownerClientNum != -1 ) {
							const clientInfo_t& deployableOwner = GetGameWorldState()->clientInfo[ deployableInfo.ownerClientNum ];
						
							if ( !botActions[ actionNumber ]->ActionIsActive() && deployableOwner.isBot ) {
								idEntity *deployableEnt = gameLocal.entities[ deployableInfo.entNum ];

								if ( deployableEnt != NULL ) {
									deployableEnt->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_deployable_destruct" ), 1.0f, NULL );
								}
							}
						}
					}
				}
			}
		}

		if ( AllowDebugData() ) {
			idBox bbox = idBox( deployableInfo.bbox, deployableInfo.origin, deployableInfo.axis );
			gameRenderWorld->DebugBox( colorWhite, bbox );

			if ( turret != NULL && turret->GetTurretType() == APT ) {
				gameRenderWorld->DebugBox( colorRed, deployableInfo.dangerAreaBox );
			}
		}
	}

//mal: vehicles last
    sdInstanceCollector< sdTransport > transports( true );

	i = 0;

	idVec3 aasOrigin;

	const float VEHICLE_OWNERSHIP_DIST = 1500.0f;

	for ( j = 0; j < MAX_VEHICLES; j++ ) {

		GetGameWorldState()->vehicleInfo[ j ].entNum = 0;
		GetGameWorldState()->vehicleInfo[ j ].isDeployed = false;

		if ( j >= transports.Num() ) {
			continue;
		}

        if ( sdTransport* transport = transports[ j ] ) { 
            if ( idPhysics* physics = transport->GetPhysics() ) {
				proxyInfo_t &vehicleInfo = GetGameWorldState()->vehicleInfo[ i ];
				vehicleInfo.bbox = physics->GetBounds( 0 );

				for( k = 1; k < physics->GetNumClipModels(); k++ ) {
					//mal: if the vehicle has multiple bounding box, add them all together for the overall bounds.
					// Gordon: passing -1 should do this
					vehicleInfo.bbox.AddBounds( physics->GetBounds( k ) );
				}

				vehicleInfo.origin = transport->GetRenderEntity()->origin;
				vehicleInfo.type = transport->GetVehicleType();
				vehicleInfo.team = transport->GetVehicleTeam();
				vehicleInfo.flags = transport->GetVehicleFlags();
				vehicleInfo.isEmpty = transport->GetPositionManager().IsEmpty();
				vehicleInfo.isEMPed = transport->IsEMPed();
				vehicleInfo.isFlipped = transport->IsFlipped();
				vehicleInfo.isAmphibious = transport->IsAmphibious();
				vehicleInfo.inWater = ( physics->InWater() > 0.0f ) ? true: false;
				vehicleInfo.health = transport->GetHealth();
				vehicleInfo.maxHealth = transport->GetMaxHealth();
				vehicleInfo.axis = transport->GetRenderEntity()->axis;
				vehicleInfo.isAirborneVehicle = ( vehicleInfo.type >= ICARUS ) ? true : false;
				vehicleInfo.isAirborneAttackVehicle = ( vehicleInfo.type == ANANSI || vehicleInfo.type == HORNET ) ? true : false;
				vehicleInfo.inPlayZone = transport->IsInPlayzone();
				vehicleInfo.hasFreeSeat = transport->GetPositionManager().HasFreePosition();
				vehicleInfo.inSiegeMode =  ( transport->GetVehicleControl() && transport->GetVehicleControl()->InSiegeMode() ) ? 1 : 0;
				vehicleInfo.hasGroundContact = physics->HasGroundContacts();
				vehicleInfo.damagedPartsCount = transport->GetDestroyedCriticalDriveParts();
				vehicleInfo.isCareening = transport->IsCareening();
                vehicleInfo.isImmobilized = ( transport->GetVehicleControl() && transport->GetVehicleControl()->IsImmobilized() ) ? 1 : 0;
				vehicleInfo.wheelsAreOnGround = transport->AreWheelsOnGround();
				vehicleInfo.canRotateInPlace = false;
				vehicleInfo.actionRouteNumber = transport->GetRouteActionNumber();
				vehicleInfo.isBoobyTrapped = false;
				
				idEntity* next = NULL;
				playerTeamTypes_t ignoreTeam = transport->GetVehicleTeam();
				
				if ( si_teamDamage.GetBool() ) {
					ignoreTeam = NOTEAM;
				}

				for ( idEntity* other = transport->GetNextTeamEntity(); other; other = next ) {
					next = other->GetNextTeamEntity();

					if ( other == transport ) {
						continue;
					}

					if ( EntityIsExplosiveCharge( other->entityNumber, true, ignoreTeam ) ) {
						vehicleInfo.isBoobyTrapped = true;
					}
				}

				if ( vehicleInfo.type == GOLIATH || vehicleInfo.type == MCP || vehicleInfo.type == TITAN || vehicleInfo.type == DESECRATOR ) {
					vehicleInfo.canRotateInPlace = true;
				}

				idPlayer* driver = transport->GetPositionManager().FindDriver();

				if ( driver != NULL ) {
                    vehicleInfo.driverEntNum = driver->entityNumber;
				} else {
					vehicleInfo.driverEntNum = -1;
				}

				vehicleInfo.isOwned = false;
				vehicleInfo.neverDriven = false;

				vehicleInfo.spawnID = gameLocal.GetSpawnId( transport );

				idPlayer* player = transport->GetLastOccupant();
				if ( player != NULL ) {
					if ( player->ownsVehicle != false && ( player->GetProxyEntity() != NULL && player->GetProxyEntity()->entityNumber == transport->entityNumber ) ) { //mal: if player exited vehicle and was killed, or is too far away, bot can take it.
                        idVec3 vec;
                        
						vec = player->GetPhysics()->GetOrigin() - vehicleInfo.origin;

						if ( vec.LengthSqr() < Square( VEHICLE_OWNERSHIP_DIST ) ) {
							if ( player->GetProxyEntity() != NULL ) {
								if ( vehicleInfo.driverEntNum == player->entityNumber ) {
									vehicleInfo.isOwned = true;
									vehicleInfo.ownerNum = player->entityNumber; //mal: if the player is in the vehicle, but NOT the driver, bot can take control!
								}
							} else {
                                vehicleInfo.isOwned = true;
                                vehicleInfo.ownerNum = player->entityNumber;
							}
						}
					} else if ( player->ownsVehicle != false && gameLocal.GetSpawnId( transport ) == player->lastOwnedVehicleSpawnID ) {
						idVec3 vec;                        
						vec = player->GetPhysics()->GetOrigin() - vehicleInfo.origin;

						if ( vec.LengthSqr() < Square( VEHICLE_OWNERSHIP_DIST ) ) {
							vehicleInfo.isOwned = true;
							vehicleInfo.ownerNum = player->entityNumber; //mal: if the player is in the vehicle, but NOT the driver, bot can take control!
						}
					}
				} else {
                    vehicleInfo.neverDriven = true;
				} //mal: dont steal vehicles from other players ( bots and humans ) Will ignore this for important vehicles ( MCP, etc ).

				vehicleInfo.entNum = transport->entityNumber;
				vehicleInfo.xyspeed = physics->GetLinearVelocity().LengthFast();
				vehicleInfo.forwardSpeed = transport->GetRenderEntity()->axis[ 0 ] * physics->GetLinearVelocity();

				if ( vehicleInfo.type == MCP ) {
					vehicleInfo.isDeployed = transport->IsDeployed();
				}

				aasOrigin = physics->GetOrigin();
				
				areaNum = Nav_GetAreaNumAndOrigin( AAS_PLAYER, physics->GetOrigin(), aasOrigin );

				if ( areaNum > 0 ) { //mal: cache last valid area
					vehicleInfo.areaNum = areaNum;
					vehicleInfo.aasOrigin = aasOrigin;
				}

				aasOrigin = physics->GetOrigin();

				areaNum = Nav_GetAreaNumAndOrigin( AAS_VEHICLE, physics->GetOrigin(), aasOrigin );

				if ( areaNum > 0 ) {
					vehicleInfo.areaNumVehicle = areaNum;
					vehicleInfo.aasVehicleOrigin = aasOrigin;
				}
				
				if ( vehicleInfo.type == MCP ) {
					GetGameWorldState()->botGoalInfo.botGoal_MCP_VehicleNum = vehicleInfo.entNum;
				}

				i++;
			}
		}
	}

	if ( !forwardSpawnsChecked ) {
		forwardSpawnsChecked = true;
		bool foundSpawnPoint = true;
		sdInstanceCollector< sdSpawnController > forwardSpawns( false );
		if ( forwardSpawns.Num() > 0 ) {
			idList< int > forwardSpawnActions;

			for( i = 0; i < botActions.Num(); i++ ) {
				if ( botActions[ i ]->GetBaseActionType() != BASE_ACTION ) {
					continue;
				}

				if ( botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_DENY_SPAWNPOINT && botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_DENY_SPAWNPOINT && 
					botActions[ i ]->GetBaseObjForTeam( GDF ) != ACTION_FORWARD_SPAWN && botActions[ i ]->GetBaseObjForTeam( STROGG ) != ACTION_FORWARD_SPAWN ) {
					continue;
				}

				forwardSpawnActions.Append( i );
			}

			if ( forwardSpawnActions.Num() > 0 ) {
				foundSpawnPoint = false; //mal: only report error if spawns and actions exist.
				for( i = 0; i < forwardSpawnActions.Num(); i++ ) {
					for( k = 0; k < forwardSpawns.Num(); k++ ) {
						const sdSpawnController* spawnPoint = forwardSpawns[ k ];

						if ( spawnPoint == NULL ) {
							continue;
						}

						idBox actionBox = botActions[ forwardSpawnActions[ i ] ]->actionBBox;
						actionBox.ExpandSelf( ACTION_BBOX_EXPAND_BIG );

						if ( !actionBox.ContainsPoint( spawnPoint->GetPhysics()->GetOrigin() ) ) {
							continue;
						}

						botActions[ forwardSpawnActions[ i ] ]->spawnControllerEntityNum = spawnPoint->entityNumber;
						foundSpawnPoint = true;
						break;
					}
				}
			}
		}

		if ( !foundSpawnPoint ) {
			if ( botThreadData.AllowDebugData() ) {
				gameLocal.Warning("No sdSpawnController entity found thats matches a bot foward_spawn action.\nThis could be because there are no forward spawn actions setup for the bots\nOr that the forward spawn action's bbox isn't correctly setup.");
			}
		}
	}
}

/*
================
idBotThreadData::LoadActions

Parse the bot actions and load them up.
================
*/
void idBotThreadData::LoadActions( const idMapFile *mapFile ) {
	int obstacleCounter = MAX_CLIENTS + MAX_GENTITIES; // MAX_GENTITIES -> MAX_GENTITIES + MAX_CLIENTS is used for arty obstacles.
	idToken	token;
	idVec3 extents;
	idAngles angles;
	idVec3 center;

	for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {

		idMapEntity* mapEnt = mapFile->GetEntity( i );
		const char* classname = mapEnt->epairs.GetString( "classname" );

		if ( idStr::Icmp( classname, "bot_action_bbox" ) == 0 ) {
			
			idBotActions* actionLoader = new idBotActions;

			actionLoader->name = mapEnt->epairs.GetString( "name", "" );
			
			mapEnt->epairs.GetVector( "box_center", "0 0 0", center );
			mapEnt->epairs.GetVector( "box_extents", "0 0 0", extents );
			mapEnt->epairs.GetAngles( "box_angles", "0 0 0", angles );

			actionLoader->actionBBox = idBox( center, extents, angles.ToMat3() );

			actionLoader->targetAction = mapEnt->epairs.GetString( "target", "" );

			if ( actionLoader->targetAction.Length() == 0 ) {
				gameLocal.Error( "idBotThreadData::LoadActions - action %s doesn't point to a valid bot_action\nHalting action loading!!", actionLoader->name.c_str() );
				return;
			}

			actionLoader->baseActionType = BBOX_ACTION;

			botActions.Append( actionLoader );

			continue;
		}

		if ( idStr::Icmp( classname, "bot_action_target" ) == 0 ) {

			idBotActions* actionLoader = new idBotActions;

			actionLoader->name = mapEnt->epairs.GetString( "name", "" );
			
			mapEnt->epairs.GetVector( "origin", "", actionLoader->origin );
			actionLoader->radius = mapEnt->epairs.GetInt( "radius", "70.0f" );

			actionLoader->targetAction = mapEnt->epairs.GetString( "target", "" );

			if ( actionLoader->targetAction.Length() == 0 ) {
				gameLocal.Error( "idBotThreadData::LoadActions - action %s doesn't point to a valid bot_action\nHalting action loading!!", actionLoader->name.c_str() );
				return;
			}

			actionLoader->baseActionType = TARGET_ACTION;

			botActions.Append( actionLoader );

			continue;
		}

		if ( idStr::Icmp( classname, "bot_action" ) == 0 ) {

			idBotActions* actionLoader = new idBotActions;

            actionLoader->name = mapEnt->epairs.GetString( "name", "" );

			actionLoader->targetAction = mapEnt->epairs.GetString( "target", "" );

			if ( actionLoader->targetAction.Length() != 0 ) {
				gameLocal.Error( "idBotThreadData::LoadActions - action %s can not target other actions. \"BBox\" or \"Target\" actions must target \"bot_action\" entities instead!\nHalting action loading!!", actionLoader->name.c_str() );
				return; //mal: make sure someone isn't being silly.
			}

			actionLoader->activeForever = mapEnt->epairs.GetBool( "activeForever", "0" );
			actionLoader->validClasses = mapEnt->epairs.GetBool( "validClasses", "0" );
			actionLoader->active = mapEnt->epairs.GetBool( "active", "0" );
			actionLoader->radius = mapEnt->epairs.GetInt( "radius", "120.0f" );

			actionLoader->deployableType = mapEnt->epairs.GetInt( "deployableType", "0" );

			actionLoader->requiresVehicleType = mapEnt->epairs.GetBool( "requiresVehicle", "0" );

			actionLoader->routeID = mapEnt->epairs.GetInt( "targetRouteID", "-1" );

			actionLoader->blindFire = mapEnt->epairs.GetBool( "blindFire", "0" );

			actionLoader->priority = mapEnt->epairs.GetBool( "priority", "1" );

			actionLoader->noHack = mapEnt->epairs.GetBool( "noHack", "0" );

			actionLoader->VOChatFlag = ( botChatTypes_t ) mapEnt->epairs.GetInt( "VOChat", "-1" );

			mapEnt->epairs.GetVector( "origin", "", actionLoader->origin );
		
			actionLoader->humanObj = ( botActionGoals_t ) mapEnt->epairs.GetInt( "humanGoal", "-1" );
			actionLoader->stroggObj = ( botActionGoals_t ) mapEnt->epairs.GetInt( "stroggGoal", "-1" );
			actionLoader->baseHumanObj = actionLoader->humanObj;
			actionLoader->baseStroggObj = actionLoader->stroggObj;
			actionLoader->teamOwner = ( playerTeamTypes_t ) mapEnt->epairs.GetInt( "teamOwner", "-1" );

			actionLoader->leanDir = ( leanTypes_t ) mapEnt->epairs.GetInt( "leanDir", "-1" );

			actionLoader->baseActionType  = BASE_ACTION;		

			actionLoader->groupID = mapEnt->epairs.GetInt( "groupID", "0" );
			actionLoader->posture = ( botMoveFlags_t ) mapEnt->epairs.GetInt( "posture", "2" ); //stand
			actionLoader->actionTimeInSeconds = mapEnt->epairs.GetInt( "actionTime", "30" );

			actionLoader->actionVehicleFlags = mapEnt->epairs.GetInt( "vehicleType", "-1" );
			
			int tempWeap = mapEnt->epairs.GetInt( "weaponType", "-1" );

			if ( tempWeap == 0 ) {
				actionLoader->weapType = HEAVY_MG;
			} else if ( tempWeap == 1 ) {
				actionLoader->weapType = ROCKET;
			} else {
				actionLoader->weapType = NULL_WEAP;
			}

			actionLoader->areaNum = Nav_GetAreaNum( AAS_PLAYER, actionLoader->origin );
			actionLoader->areaNumVehicle = Nav_GetAreaNum( AAS_VEHICLE, actionLoader->origin );

			if ( actionLoader->humanObj == ACTION_MG_NEST_BUILD || actionLoader->stroggObj == ACTION_MG_NEST_BUILD ) {
				actionLoader->actionState = ACTION_STATE_GUN_DESTROYED;
			} else {
				actionLoader->actionState = ACTION_STATE_NORMAL;
			}

			actionLoader->hasObj = false;

			AddActionToHash( actionLoader->name, botActions.Append( actionLoader ) );
			continue;
		}

		if ( idStr::Icmp( classname, "bot_dynamic_obstacle" ) == 0 ) {

			idBotObstacle* obstacle = new idBotObstacle;

			obstacle->num = obstacleCounter++;

			mapEnt->epairs.GetVector( "box_center", "0 0 0", center );
			mapEnt->epairs.GetVector( "box_extents", "0 0 0", extents );
			mapEnt->epairs.GetAngles( "box_angles", "0 0 0", angles );

			obstacle->bbox = idBox( center, extents, angles.ToMat3() );
			obstacle->areaNum[AAS_PLAYER] = Nav_GetAreaNum( AAS_PLAYER, center );
			obstacle->areaNum[AAS_VEHICLE] = Nav_GetAreaNum( AAS_VEHICLE, center );

			if ( obstacle->areaNum[AAS_PLAYER] == 0 ) {
				//gameLocal.Warning( "bot obstacle at '%1.0f %1.0f %1.0f' is not in a valid player AAS area", center.x, center.y, center.z );
			}
			if ( obstacle->areaNum[AAS_VEHICLE] == 0 ) {
				//gameLocal.Warning( "bot obstacle at '%1.0f %1.0f %1.0f' is not in a valid vehicle AAS area", center.x, center.y, center.z );
			}

			botObstacles.Append( obstacle );
			continue;
		}

		if ( idStr::Icmp( classname, "bot_locationremap" ) == 0 ) {
			
			botLocationRemap_t *remap = new botLocationRemap_t;

			mapEnt->epairs.GetVector( "box_center", "0 0 0", center );
			mapEnt->epairs.GetVector( "box_extents", "0 0 0", extents );
			mapEnt->epairs.GetAngles( "box_angles", "0 0 0", angles );

			remap->bbox = idBox( center, extents, angles.ToMat3() );

			idMapEntity *targetEnt = mapFile->FindEntity( mapEnt->epairs.GetString( "target", "" ) );
			if ( targetEnt == NULL ) {
				gameLocal.Error( "idBotThreadData::LoadActions - bot_locationremap %s doesn't point to a valid bot_locationremap_target!!", mapEnt->epairs.GetString( "name" ) );
				return;
			}
			targetEnt->epairs.GetVector( "origin", "0 0 0", remap->target );

			botLocationRemap.Append( remap );

			continue;
		}
	}

//mal: lets go thru all the actions, and find the actions that are targeting base actions ( if any ).
	for ( int i = 0; i < botActions.Num(); i++ ) {

		if ( botActions[ i ]->baseActionType == BASE_ACTION ) {
			continue;
		}

		if ( botActions[ i ]->baseActionType == BBOX_ACTION ) {
			
			int t = FindActionByName( botActions[ i ]->targetAction );

			if ( t == -1 ) {
				gameLocal.Error( "idBotThreadData::LoadActions - bbox action %s doesn't point to a valid bot_action\nAction %s either doesn't exist, or isn't a valid primary action\nHalting action loading!!", botActions[ i ]->name.c_str(), botActions[ i ]->targetAction.c_str() );
				return;
			}

			if ( botActions[ t ]->baseActionType != BASE_ACTION ) {
				gameLocal.Error( "idBotThreadData::LoadActions - base action %s isn't a valid bot_action\nHalting action loading!!", botActions[ t ]->name.c_str() );
				return;
			}

			botActions[ t ]->actionBBox = botActions[ i ]->actionBBox;
		}

		if ( botActions[ i ]->baseActionType == TARGET_ACTION ) {

			bool foundSlot = false;
			
			int t = FindActionByName( botActions[ i ]->targetAction );

			if ( t == -1 ) {
				gameLocal.Error( "idBotThreadData::LoadActions - target action %s doesn't point to a valid bot_action\nAction %s either doesn't exist, or isn't a valid primary action\nHalting action loading!!", botActions[ i ]->name.c_str(), botActions[ i ]->targetAction.c_str() );
				return;
			}

			if ( botActions[ t ]->baseActionType != BASE_ACTION ) {
				gameLocal.Error( "idBotThreadData::LoadActions - base action %s isn't a valid bot_action\nHalting action loading!!", botActions[ t ]->name.c_str() );
				return;
			}

			for ( int j = 0; j < MAX_LINKACTIONS; j++ ) {
				
				if ( botActions[ t ]->actionTargets[ j ].inuse != false ) {
					continue;
				}

				botActions[ t ]->actionTargets[ j ].inuse = true;
				botActions[ t ]->actionTargets[ j ].origin = botActions[ i ]->origin;
				botActions[ t ]->actionTargets[ j ].radius = botActions[ i ]->radius;
				foundSlot = true;
				break;
			}

			if ( foundSlot == false ) {
				gameLocal.Error( "idBotThreadData::LoadActions - too many target actions pointing to base action %s\nMax target actions that can point to one bot_action is %i\nHalting action loading!!", botActions[ t ]->name.c_str(), MAX_LINKACTIONS );
				return;
			}
		}
	}

	if ( botActions.Num() > 0 ) {
		actionsLoaded = true;
	}

	DeactivateBadMapActions();
}

/*
================
idBotThreadData::RemapLocation
================
*/
bool idBotThreadData::RemapLocation( idVec3 &origin ) const {
	for ( int i = 0; i < botLocationRemap.Num(); i++ ) {
		if ( botLocationRemap[i]->bbox.ContainsPoint( origin ) ) {
			origin = botLocationRemap[i]->target;
			return true;
		}
	}
	return false;
}

/*
================
idBotThreadData::LoadRoutes

Parse the bot routes and load them up.
================
*/
void idBotThreadData::LoadRoutes( const idMapFile *mapFile ) {
	
	int i, j, k;
	idBotRoutes *routeLoader;

	for( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		const idDict &epairs = mapFile->GetEntity( i )->epairs;

		if ( idStr::Icmp( epairs.GetString( "classname" ), "bot_route_start" ) == 0 ) {
			routeLoader = new idBotRoutes;

			epairs.GetVector( "origin", "", routeLoader->origin );
			routeLoader->radius = epairs.GetInt( "radius", "120.0f" );
			routeLoader->team = ( playerTeamTypes_t ) epairs.GetInt( "team", "-1" );
			routeLoader->isHeadNode = true;
			routeLoader->active = epairs.GetBool( "active", "1" );
			routeLoader->groupID = epairs.GetInt( "targetRouteID", "-1" );
			routeLoader->name = epairs.GetString( "name", "" );
			routeLoader->num = botRoutes.Num();
			AddRouteToHash( routeLoader->name, botRoutes.Append( routeLoader ) );
			continue;
		}

		if ( idStr::Icmp( epairs.GetString( "classname" ), "bot_route_link" ) == 0 ) {
			routeLoader = new idBotRoutes;

			epairs.GetVector( "origin", "", routeLoader->origin );
			routeLoader->radius = epairs.GetInt( "radius", "120.0f" );
			routeLoader->team = ( playerTeamTypes_t ) epairs.GetInt( "team", "-1" );
			routeLoader->isHeadNode = false;
			routeLoader->active = epairs.GetBool( "active", "1" );
			routeLoader->groupID = epairs.GetInt( "targetRouteID", "-1" );
			routeLoader->name = epairs.GetString( "name", "" );
			routeLoader->num = botRoutes.Num();
			AddRouteToHash( routeLoader->name, botRoutes.Append( routeLoader ) );
			continue;
		}
	}

//mal: now go thru again and find the routes that are linked together...
	for( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		const idDict &epairs = mapFile->GetEntity( i )->epairs;

		if ( idStr::Icmp( epairs.GetString( "classname" ), "bot_route_start" ) != 0 && idStr::Icmp( epairs.GetString( "classname" ), "bot_route_link" ) != 0 ) {
			continue;
		}

		const idKeyValue* kv = epairs.MatchPrefix( "target" );
		const char* routeName = epairs.GetString( "name", "" );

		while ( kv != NULL ) {
			for( j = 0; j < botRoutes.Num(); j++ ) {
				if ( idStr::Icmp( botRoutes[ j ]->name, routeName ) == 0 ) {
					break;
				}
			}

			assert( j < botRoutes.Num() );

			for( k = 0; k < botRoutes.Num(); k++ ) {
				if ( k == j ) {
					continue;
				}					

				if ( idStr::Icmp( botRoutes[ k ]->name, kv->GetValue().c_str() ) != 0 ) {
					continue;
				}

				botRoutes[ j ]->routeLinks.Append( botRoutes[ k ] );
				break;
			}

			kv = epairs.MatchPrefix( "target", kv );
		}
	}
}

/*
================
idBotThreadData::VOChat

A safe way to say chats from anywhere in the game. If forceChat == true, will say the chat 
no matter what.


//mal_FIXME: loop thru all the chats at startup and store off the indexes in a hash.

================
*/
void idBotThreadData::VOChat( const botChatTypes_t chatType, int clientNum, bool forceChat ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		return;
	}

#ifdef _XENON
	if ( gameLocal.rules->IsWarmup() ) {
		return;
	}
#endif

	const sdDeclQuickChat* quickChat;
	idPlayer* player;
	const char* quickChatName;

	if ( bot_noChat.GetBool() ) { //mal: shaddup you mouthy bot!
		return;
	}

	if ( GetGameWorldState()->clientInfo[ clientNum ].chatDelay > gameLocal.time ) {
		return;
	} //mal: dont allow overlapping chats!

	if ( chatType == NULL_CHAT ) {
		return;
	}

	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ clientNum ];

	quickChat = NULL;
	player = gameLocal.GetClient( clientNum );

	if ( player == NULL ) {
		return;
	}

	if ( player->GetGameTeam() == NULL ) {
		return;
	}

	const sdDeclPlayerClass* pc = player->GetInventory().GetClass();

	if( pc == NULL ) {
		return ;
	}

	if ( chatType == THANKS ) {
		if ( playerInfo.lastChatTime[ THANKS ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/thanks" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastThanksTime = gameLocal.time;
			playerInfo.lastChatTime[ THANKS ] = gameLocal.time + 30000;
		}
	} else if ( chatType == HEAL_ME ) {
		if ( playerInfo.lastChatTime[ HEAL_ME ] < gameLocal.time || forceChat ) {
			if ( playerInfo.team == GDF ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/context/health" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/context/stroyent" );
			}
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ HEAL_ME ] = gameLocal.time + 15000;
		}
	} else if ( chatType == NEED_BACKUP ) {
		if ( playerInfo.lastChatTime[ NEED_BACKUP ] < gameLocal.time || forceChat ) {
			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
                quickChatName = pc->BuildQuickChatDeclName( "quickchat/need/backup" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/need/coveringfire" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ NEED_BACKUP ] = gameLocal.time + 120000; //mal: REALLY dont want to do this too much!
		}
	} else if ( chatType == HOLDFIRE ) {
        if ( playerInfo.lastChatTime[ HOLDFIRE ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/commands/holdfire" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ HOLDFIRE ] = gameLocal.time + 60000;
		}
	} else if ( chatType == MOVE ) {
        if ( playerInfo.lastChatTime[ MOVE ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/commands/move" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ MOVE ] = gameLocal.time + 30000;
		}
	} else if ( chatType == FOLLOW_ME ) {
        if ( playerInfo.lastChatTime[ FOLLOW_ME ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/commands/followme" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ FOLLOW_ME ] = gameLocal.time + 5000;
		}
	} else if ( chatType == NEED_LIFT ) {
        if ( playerInfo.lastChatTime[ NEED_LIFT ] < gameLocal.time || forceChat ) {

			if ( random.RandomInt( 100 ) > 50 ) {
                quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/needalift" );
			} else {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/needaride" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/needtransport" );
				}
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ NEED_LIFT ] = gameLocal.time + 10000; //mal: dont do for a while
		}
	} else if ( chatType == CMD_DECLINED && ( lastCmdDeclinedChatTime < gameLocal.time || forceChat ) ) {
        if ( playerInfo.lastChatTime[ CMD_DECLINED ] < gameLocal.time || forceChat ) {
			if ( random.RandomInt( 100 ) > 50 ) {
                quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/declined" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/unabletoassist" );
			}
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ CMD_DECLINED ] = gameLocal.time + REQUEST_CONSIDER_TIME; //mal: dont do for a while
			lastCmdDeclinedChatTime = gameLocal.time + 20000; //REQUEST_CONSIDER_TIME;
		}
	} else if ( chatType == MY_CLASS ) {
        if ( playerInfo.lastChatTime[ MY_CLASS ] < gameLocal.time || forceChat ) {
			if ( playerInfo.classType == MEDIC ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/immedic" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imtechnician" );
				}
			} else if ( playerInfo.classType == SOLDIER ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imsoldier" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imaggressor" );
				}
			} else if ( playerInfo.classType == ENGINEER ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imengineer" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imconstructor" );
				}
			} else if ( playerInfo.classType == FIELDOPS ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imfieldops" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imoppressor" );
				}
			} else {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imcovertops" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/iminfiltrator" );
				}
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ MY_CLASS ] = gameLocal.time + 30000;
		}
	} else if ( chatType == REARM_ME ) {
        if ( playerInfo.lastChatTime[ REARM_ME ] < gameLocal.time || forceChat ) {
			if ( playerInfo.team == GDF ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/need/ammo" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/context/stroyent" );
			}
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ REARM_ME ] = gameLocal.time + 15000;
		}
	} else if ( chatType == REVIVE_ME ) {
        if ( playerInfo.lastChatTime[ REVIVE_ME ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/context/revive" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ REVIVE_ME ] = gameLocal.time + 30000;
		}
	} else if ( chatType == ENEMY_DISGUISED ) {
        if ( playerInfo.lastChatTime[ ENEMY_DISGUISED ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/enemy/indisguise" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ ENEMY_DISGUISED ] = gameLocal.time + 20000;
		}
	} else if ( chatType == KILLED_TAUNT && !bot_noTaunt.GetBool() ) {
        if ( playerInfo.lastChatTime[ KILLED_TAUNT ] < gameLocal.time || forceChat ) {

			int n = random.RandomInt( 12 );

			if ( n == 0 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "global/taunts/grr" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "global/taunts/rrr" );
				}
			} else if ( n == 1 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/greatshot" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/awesome" );
				}
			} else if ( n == 2 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/thathurt" );
			} else if ( n == 3 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ihadworse" );
			} else if ( n == 4 )  {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/cough" );
			} else if ( n == 5 )  {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/meh" );
			} else if ( n == 6 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/awkward" );
			} else if ( n == 7 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/awyeahohno" );
			} else if ( n == 8 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ohdear" );
			} else if ( n == 9 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/oops" );
			} else if ( n == 10 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/bullseye" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/accurate" );
				}
			} else {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/wellplayed" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/impressive" );
				}
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ KILLED_TAUNT ] = gameLocal.time + 30000;
		}
	} else if ( chatType == WARMUP_TAUNT && !bot_noTaunt.GetBool() ) {
        if ( playerInfo.lastChatTime[ WARMUP_TAUNT ] < gameLocal.time || forceChat ) {

			int n = random.RandomInt( 6 );

			if ( n == 0 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/gdf" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/strogg" );
				}
			} else if ( n == 1 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killalienscum" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killhumanfood" );
				}
			} else if ( n == 2 ) {
                quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ohdear" );
			} else if ( n == 3 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/enemyweakened" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theycrumble" );
				}
			} else if ( n == 4 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theyrunningaway" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theyfleeterror" );
				}
			} else {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/freedomofearth" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/forthemakron" );
				}
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ WARMUP_TAUNT ] = gameLocal.time + 10000;
		}
	} else if ( chatType == GENERAL_TAUNT && ( !bot_noTaunt.GetBool() || forceChat ) ) { //mal: make an exception for this chat type - since bots need to taunt when humiliating enemy.
        if ( playerInfo.lastChatTime[ GENERAL_TAUNT ] < gameLocal.time || forceChat ) {

			int n = random.RandomInt( 6 );

			if ( n == 0 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/freedomofearth" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/forthemakron" );
				}
			} else if ( n == 1 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killalienscum" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killhumanfood" );
				}
			} else if ( n == 2 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/eatthatstrogg" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/eatthathuman" );
				}
			} else if ( n == 3 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/gdf" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/strogg" );
				}
			} else if ( n == 4 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/denied" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/thwarted" );
				}
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/owned" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ GENERAL_TAUNT ] = gameLocal.time + 10000;
		}
	} else if ( chatType == WILL_FIX_RIDE ) {
        if ( playerInfo.lastChatTime[ WILL_FIX_RIDE ] < gameLocal.time || forceChat ) {
			if ( gameLocal.random.RandomInt( 100 ) > 50 ) {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/vehicle/letmerepair" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/vehicle/iwillfixride" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ WILL_FIX_RIDE ] = gameLocal.time + 15000;
		}
	} else if ( chatType == STOP_WILL_FIX_RIDE ) {
        if ( playerInfo.lastChatTime[ STOP_WILL_FIX_RIDE ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/vehicle/stopiwillfixride" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ STOP_WILL_FIX_RIDE ] = gameLocal.time + 15000;
		}
	} else if ( chatType == YOURWELCOME ) {
        if ( playerInfo.lastChatTime[ YOURWELCOME ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/youwelcome" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ YOURWELCOME ] = gameLocal.time + 15000;
		}
	} else if ( chatType == ACKNOWLEDGE_YES ) {
        if ( playerInfo.lastChatTime[ ACKNOWLEDGE_YES ] < gameLocal.time || forceChat ) {
            int n = random.RandomInt( 3 );

			if ( n == 0 ) {
                quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/acknowledged" );
			} else if ( n == 1 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/onit" );
			} else {
                quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/onmyway" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ ACKNOWLEDGE_YES ] = gameLocal.time + 15000;
		}
	} else if ( chatType == MEDIC_ACK ) {
        if ( playerInfo.lastChatTime[ MEDIC_ACK ] < gameLocal.time || forceChat ) {
			int n = random.RandomInt( 3 );

			if ( n == 0 ) {
                if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/holdonimamedic" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/holdonimatechnician" );
				}
			} else if ( n == 1 ) {
				if ( playerInfo.team == GDF ) {
                    quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/medicenroute" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/technicianenroute" );
				}
			} else {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/immedic" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/imtechnician" );
				}				
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ MEDIC_ACK ] = gameLocal.time + 15000;
		}
	} else if ( chatType == AMMO_ACK ) {
        if ( playerInfo.lastChatTime[ AMMO_ACK ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/gotammo" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ AMMO_ACK ] = gameLocal.time + 15000; 
		}
	} else if ( chatType == TK_REVIVE_CHAT ) {
        if ( playerInfo.lastChatTime[ TK_REVIVE_CHAT ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/tkrevive" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ TK_REVIVE_CHAT ] = gameLocal.time + 15000; //mal: dont go again for a while.
		}
	} else if ( chatType == HELLO ) {
        if ( playerInfo.lastChatTime[ HELLO ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/hi" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ HELLO ] = gameLocal.time + 60000; //mal: dont go again for a while.
		}
	} else if ( chatType == GOT_YOUR_BACK ) {
        if ( playerInfo.lastChatTime[ GOT_YOUR_BACK ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/gotyourback" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ GOT_YOUR_BACK ] = gameLocal.time + 5000; //mal: dont go again for a while.
		}
	} else if ( chatType == MOVE_OUT ) {
        if ( playerInfo.lastChatTime[ MOVE_OUT ] < gameLocal.time || forceChat ) {
            quickChatName = pc->BuildQuickChatDeclName( "quickchat/commands/letsgo" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ MOVE_OUT ] = gameLocal.time + 5000; //mal: dont go again for a while.
		}
	} else if ( chatType == LETS_GO ) {
        if ( playerInfo.lastChatTime[ LETS_GO ] < gameLocal.time || forceChat ) {
			int n = random.RandomInt( 5 );

			if ( n == 0 ) {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/youlead" );
			} else if ( n == 1 ) {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/gotyourback" );
			} else if ( n == 2 ) {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/leadway" );
			} else if ( n == 3 ) {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/generic/whereto" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/commands/letsgo" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ LETS_GO ] = gameLocal.time + 60000; //mal: dont go again for a while.
		}
	} else if ( chatType == GENERIC_PLANT ) {
        if ( playerInfo.lastChatTime[ GENERIC_PLANT ] < gameLocal.time || forceChat ) {
			if ( playerInfo.team == GDF ) {
                quickChatName = pc->BuildQuickChatDeclName( "botchat/objectives/planthecharge" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "botchat/objectives/plantplasmacharge" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ GENERIC_PLANT ] = gameLocal.time + 60000; //mal: dont go again for a while.
		}
	} else if ( chatType == INCOMING_VEHICLE ) {
        if ( playerInfo.lastChatTime[ INCOMING_VEHICLE ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/enemy/vehiclespotted" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ INCOMING_VEHICLE ] = gameLocal.time + 30000;
		}
	} else if ( chatType == INCOMING_AIRCRAFT ) {
        if ( playerInfo.lastChatTime[ INCOMING_AIRCRAFT ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/enemy/aircraftspotted" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ INCOMING_AIRCRAFT ] = gameLocal.time + 30000;
		}
	}else if ( chatType == INCOMING_INFANTRY ) {
        if ( playerInfo.lastChatTime[ INCOMING_INFANTRY ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/enemy/infantryspotted" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ INCOMING_INFANTRY ] = gameLocal.time + 30000;
		}
	} else if ( chatType == IM_DISGUISED ) {
        if ( playerInfo.lastChatTime[ IM_DISGUISED ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/disguise/imindisguise" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ IM_DISGUISED ] = gameLocal.time + 30000;
		}
	} else if ( chatType == MINES_SPOTTED ) {
        if ( playerInfo.lastChatTime[ MINES_SPOTTED ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/enemy/minesspotted" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ MINES_SPOTTED ] = gameLocal.time + 45000;
		}
	} else if ( chatType == HOLD_VEHICLE ) {
        if ( playerInfo.lastChatTime[ HOLD_VEHICLE ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/vehicles/holdvehicle" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ HOLD_VEHICLE ] = gameLocal.time + 30000;
		}
	} else if ( chatType == ENEMY_DISGUISED_AS_ME ) {
        if ( playerInfo.lastChatTime[ ENEMY_DISGUISED_AS_ME ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/self/disguise/enemydisguisedasme" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ ENEMY_DISGUISED_AS_ME ] = gameLocal.time + 30000;

			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( GetGameWorldState()->clientInfo[ i ].team == playerInfo.team ) {
					continue;
				}

				if ( GetGameWorldState()->clientInfo[ i ].classType != COVERTOPS ) {
					continue;
				}

				if ( !GetGameWorldState()->clientInfo[ i ].isDisguised ) {
					continue;
				}

				if ( GetGameWorldState()->clientInfo[ i ].disguisedClient != clientNum ) {
					continue;
				}

				GetGameWorldState()->clientInfo[ i ].covertWarningTime = gameLocal.time;
				break;
			}
		}
	} else if ( chatType == ENDGAME_WIN && !bot_noTaunt.GetBool() ) {
		if ( playerInfo.lastChatTime[ ENDGAME_WIN ] < gameLocal.time || forceChat ) {
			int n = random.RandomInt( 11 );

			if ( n == 0 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/gdf" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/strogg" );
				}
			} else if ( n == 1 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killalienscum" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/killhumanfood" );
				}
			} else if ( n == 2 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ohdear" );
			} else if ( n == 3 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/enemyweakened" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theycrumble" );
				}
			} else if ( n == 4 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theyrunningaway" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/theyfleeterror" );
				}
			} else if ( n == 5 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/freedomofearth" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/forthemakron" );
				}
			} else if ( n == 6 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/eatthatstrogg" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/eatthathuman" );
				}
			} else if ( n == 7 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/denied" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/thwarted" );
				}
			} else if ( n == 8 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/owned" );
			} else if ( n == 9 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/goodgame" );
			} else {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/goodgame" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/excellent" );
				}
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ ENDGAME_WIN ] = gameLocal.time + 15000;
		}
	} else if ( chatType == ENDGAME_LOSE && !bot_noTaunt.GetBool() ) {
		if ( playerInfo.lastChatTime[ ENDGAME_LOSE ] < gameLocal.time || forceChat ) {

			int n = random.RandomInt( 13 );

			if ( n == 0 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/gdf" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/strogg" );
				}
			} else if ( n == 1 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/awkward" );
			} else if ( n == 2 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ohdear" );
			} else if ( n == 3 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/oops" );
			} else if ( n == 4 ) {
				if ( playerInfo.team == GDF ) {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/grr" );
				} else {
					quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/rrr" );
				}	
			} else if ( n == 5 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/cough" );
			} else if ( n == 6 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/thathurt" );
			} else if ( n == 7 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/meh" );
			} else if ( n == 8 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/awyeahohno" );
			} else if ( n == 9 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/taunts/ihadworse" );
			} else if ( n == 10 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/quiet" );
			} else if ( n == 11 ) {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/cheers/goodgame" );
			} else {
				quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/quiet" );
			}

			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ ENDGAME_LOSE ] = gameLocal.time + 15000;
		}
	} else if ( chatType == SORRY ) {
        if ( playerInfo.lastChatTime[ SORRY ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/responses/sorry" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ SORRY ] = gameLocal.time + 15000;
		}
	} else if ( chatType == GLOBAL_SORRY ) {
        if ( playerInfo.lastChatTime[ GLOBAL_SORRY ] < gameLocal.time || forceChat ) {
			quickChatName = pc->BuildQuickChatDeclName( "quickchat/global/sorry" );
			quickChat = gameLocal.declQuickChatType.LocalFind( quickChatName, false );
			playerInfo.lastChatTime[ GLOBAL_SORRY ] = gameLocal.time + 5000;
		}
	}

	if ( quickChat == NULL ) {
		return;
	}
	
	gameLocal.ServerSendQuickChatMessage( player, quickChat, NULL, NULL );
	playerInfo.chatDelay = gameLocal.time + 1000;
}

/*
================
idBotThreadData::NumClassOnTeam

Looks for all clients that are of class "classType" on team "playerTeam" and returns the number of clients.
================
*/
int idBotThreadData::GetNumClassOnTeam( const playerTeamTypes_t playerTeam, const playerClassTypes_t classType ) {

	int n = 0;

	if( !GetBotWorldState() ) {
		return 0;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetBotWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].classType != NOCLASS ) {
			if ( GetBotWorldState()->clientInfo[ i ].classType != classType ) {
				continue;
			}
		} else {
			if ( GetBotWorldState()->clientInfo[ i ].cachedClassType != classType ) {
				continue;
			}
		}

		n++;
	}

	return n;
}

/*
================
idBotThreadData::GetNumBotsInServer

Looks for the number of bots on the server. If playerTeam != NOTEAM, then only bots
playing on playerTeam.
================
*/
int idBotThreadData::GetNumBotsInServer( const playerTeamTypes_t playerTeam ) {

	int n = 0;
	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team == NOTEAM ) {
			continue;
		}

		if ( playerTeam != NOTEAM ) {
			if ( GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
				continue;
			}
		}

		if ( !GetGameWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		n++;
	}

	return n;
}

/*
================
idBotThreadData::TeamHasHumans

Does this team have any humans on it?
================
*/
bool idBotThreadData::TeamHasHumans( const playerTeamTypes_t playerTeam ) {

	bool hasHumans = false;
	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team == NOTEAM ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].isBot == true ) {
			continue;
		}


		if ( GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		hasHumans = true;
		break;
	}

	return hasHumans;
}

/*
================
idBotThreadData::ResetBotAI

Do a reset of the bot's AI.
================
*/
void idBotThreadData::ResetBotAI( const playerTeamTypes_t playerTeam ) {

	int i;

	for( i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team == NOTEAM ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].isBot == false ) {
			continue;
		}


		if ( GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		GetGameWorldState()->clientInfo[ i ].resetState = MAJOR_RESET_EVENT; //mal: give it total priority!
	}
}

/*
================
idBotThreadData::UpdateChats;

If a human chats, see what they chatted, and if its something the bot's should worry about.
================
*/
void idBotThreadData::UpdateChats( int clientNum, const sdDeclQuickChat* quickChat ) {

	int i;

    if ( quickChat->GetType() == THANKS ) {

		botThreadData.GetGameWorldState()->clientInfo[ clientNum ].lastThanksTime = gameLocal.time;

	} else if ( quickChat->GetType() == ENEMY_DISGUISED_AS_ME ) {

		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( botThreadData.GetGameWorldState()->clientInfo[ i ].team == botThreadData.GetGameWorldState()->clientInfo[ clientNum ].team ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->clientInfo[ i ].classType != COVERTOPS ) {
				continue;
			}

			if ( !botThreadData.GetGameWorldState()->clientInfo[ i ].isDisguised ) {
				continue;
			}

			if ( botThreadData.GetGameWorldState()->clientInfo[ i ].disguisedClient != clientNum ) {
				continue;
			}

			botThreadData.GetGameWorldState()->clientInfo[ i ].covertWarningTime = gameLocal.time;
			break;
		}
	}

    botThreadData.GetGameWorldState()->clientInfo[ clientNum ].lastChatTime[ quickChat->GetType() ] = gameLocal.time;
}

/*
================
idBotThreadData::FindCurrentWeaponInVehicle

Returns what kind of weapon "player" has available to it in the vehicle position its in.
================
*/
void idBotThreadData::FindCurrentWeaponInVehicle( idPlayer *player, sdTransport* transport ) {

	int weapIndex;

	if ( transport->GetPositionManager().PositionForPlayer( player ).FindDefaultWeapon() == -1 ) {
		if ( transport->GetPositionManager().PositionForPlayer( player ).GetAllowWeapon() ) {
			botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = PERSONAL_WEAPON; //mal: we can use our main gun, pistols, and nades.
		} else {
            botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = NULL_VEHICLE_WEAPON;
		}

		return;
	}

	weapIndex = transport->GetPositionManager().PositionForPlayer( player ).GetWeaponIndex(); 

	if( weapIndex == -1 ) { 
		botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = NULL_VEHICLE_WEAPON;
		return;
	}

	switch( transport->GetVehicleType() ) {
		case BADGER:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;

		case TITAN:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = TANK_GUN;
			} else if ( weapIndex == 1 ) {
				botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;

		case HOG:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;

		case GOLIATH:
            botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = TANK_GUN;
			break;

		case DESECRATOR:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = TANK_GUN;
			} else if ( weapIndex == 1 ) {
				botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;

		case MCP:
		case BUFFALO:
			if ( weapIndex >= 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;

		case ICARUS:
			botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = STROY_BOMB;
			break;

		case PLATYPUS:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			} 

			break;

		case TROJAN:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			} else if ( weapIndex == 1 ) {
				botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = LAW;
			}

			break;

		case ANANSI:
		case HORNET:
			if ( weapIndex == 0 ) {
                botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = LAW;
			} else if ( weapIndex == 1 ) {
				botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = ROCKETS;
			} else if ( weapIndex == 2 ) {
				botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weapon = MINIGUN;
			}

			break;
	}

//	common->Printf("WeapIndex = %i\n", weapIndex );
//	common->Printf("Weapon Ready = %i\n", botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].proxyInfo.weaponIsReady );
}

/*
================
idBotThreadData::AddActionToHash
================
*/
void idBotThreadData::AddRouteToHash( const char *routeName, int routeNum ) {
	if ( FindRouteByName( routeName ) != -1 ) {
		gameLocal.Error( "Multiple bot routes named '%s'", routeName );
	}

	routeHash.Add( routeHash.GenerateKey( routeName, true ), routeNum );
}

/*
=============
idBotThreadData::FindRouteByName

Returns the route whose name matches the specified string.
=============
*/
int idBotThreadData::FindRouteByName( const char *routeName ) {
	int hash, i;

	hash = routeHash.GenerateKey( routeName, true );

	for ( i = routeHash.GetFirst( hash ); i != -1; i = routeHash.GetNext( i ) ) {	
		if ( botRoutes[ i ]->name.Icmp( routeName ) == 0 ) {
			return i;
		}
	}

	return -1;
}

/*
=============
idBotThreadData::CheckCrossHairInfo

Give the bots a break, and let them easily use things in the world.
=============
*/
void idBotThreadData::CheckCrossHairInfo( idEntity *bot, sdCrosshairInfo &botCrossHairInfo ) {
	int i;
	float entDist = 350.0f;
	idBounds entBounds;
	idBox playerBox, entBox;
	idVec3 vec;

	playerBox = idBox( bot->GetPhysics()->GetBounds(), bot->GetPhysics()->GetOrigin(), bot->GetPhysics()->GetAxis() );

	idEntity* botCrossHairEntity = botCrossHairInfo.GetEntity();

	if ( botCrossHairEntity != NULL ) {
		if ( botCrossHairEntity->IsType( sdTransport::Type ) ) {
			entDist = 750.0f;
		}

		vec = botCrossHairEntity->GetPhysics()->GetOrigin() - bot->GetPhysics()->GetOrigin();
		
		if ( vec.LengthSqr() < Square( entDist ) ) {
			botCrossHairInfo.SetStartTime( gameLocal.time );
			botCrossHairInfo.SetDistance( BOT_CROSSHAIR_DIST );
			botCrossHairInfo.Validate();
		}
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ bot->entityNumber ].actionEntityNum != -1 ) {
		idEntity* actionEnt = gameLocal.entities[ botThreadData.GetGameOutputState()->botOutput[ bot->entityNumber ].actionEntityNum ];

		if ( actionEnt != NULL && gameLocal.GetSpawnId( actionEnt ) == botThreadData.GetGameOutputState()->botOutput[ bot->entityNumber ].actionEntitySpawnID ) { //mal: our actionEnt will override our crosshair entity, in the off chance they dont match.    
			if ( actionEnt != botCrossHairEntity ) {
				entBounds = actionEnt->GetPhysics()->GetBounds( 0 );

				for( i = 1; i < actionEnt->GetPhysics()->GetNumClipModels(); i++ ) { //mal: if the entity has multiple bounding boxs, add them all together for the overall bounds.
					entBounds.AddBounds( actionEnt->GetPhysics()->GetBounds( i ) );
				}

				entBox = idBox( entBounds, actionEnt->GetPhysics()->GetOrigin(), actionEnt->GetPhysics()->GetAxis() );
			    entBox.ExpandSelf( VEHICLE_BOX_EXPAND );

				if ( playerBox.IntersectsBox( entBox ) ) { 
					botCrossHairInfo.SetEntity( actionEnt );
					botCrossHairInfo.SetStartTime( gameLocal.time );
					botCrossHairInfo.SetDistance( BOT_CROSSHAIR_DIST );
					botCrossHairInfo.Validate();
				}
			}
		}
	}

//mal: flag this bot as having a crosshair hint of some kind.
	if ( botCrossHairInfo.IsUseValid() ) {
		botThreadData.GetGameWorldState()->clientInfo[ bot->entityNumber ].hasCrosshairHint = true;
	} else {
		botThreadData.GetGameWorldState()->clientInfo[ bot->entityNumber ].hasCrosshairHint = false;
	}
}

/*
===============
idBotThreadData::Nav_GetAreaNum
===============
*/
int idBotThreadData::Nav_GetAreaNum( int aasType, const idVec3 &origin ) {
	idAAS *aas = GetAAS( aasType );

	if ( aas == NULL ) {
		return 0;
	}

	return aas->PointReachableAreaNum( origin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, 0 );
}

/*
===============
idBotThreadData::Nav_GetAreaNumAndOrigin
===============
*/
int idBotThreadData::Nav_GetAreaNumAndOrigin( int aasType, const idVec3& origin, idVec3& aasOrigin ) {
	idAAS *aas = GetAAS( aasType );

	if ( aas == NULL ) {
		return 0;
	}

	int areaNum = aas->PointReachableAreaNum( origin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, 0 );

	if ( areaNum > 0 ) {
		aas->PushPointIntoArea( areaNum, aasOrigin );
	}

	return areaNum;
}

/*
===============
idBotThreadData::Nav_IsDirectPath
===============
*/
bool idBotThreadData::Nav_IsDirectPath( int aasType, const playerTeamTypes_t& playerTeam, int areaNum, const idVec3& origin, const idVec3& end ) {
	idAAS *aas = GetAAS( aasType );

	if ( aas == NULL ) {
		return false;
	}

	if ( botThreadData.AllowDebugData() ) {
		gameRenderWorld->DebugLine( colorGreen, origin, end, 16 );
	}

	int travelFlags = ( playerTeam == GDF ) ? TFL_VALID_GDF : TFL_VALID_STROGG;

	if ( areaNum == NULL_AREANUM ) {
		areaNum = aas->PointReachableAreaNum( origin, aas->GetSettings()->boundingBox, AAS_AREA_REACHABLE_WALK, 0 );
	}

	aasTraceFloor_t trace;

	travelFlags &= ~TFL_WALKOFFLEDGE;

	if ( aasType == AAS_VEHICLE ) {
		travelFlags &= ~TFL_BARRIERJUMP;
	}

	aas->TraceFloor( trace, origin, areaNum, end, travelFlags );

	return trace.fraction == 1.0f;
}

/*
===============
idBotThreadData::LoadBotNames
===============
*/
void idBotThreadData::LoadBotNames() {

	botNames.Clear();

	idLexer src( LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT );

	idStr botFileName = "bots/";
	botFileName += "botnames";

	botFileName.SetFileExtension( "dat" );

	src.LoadFile( botFileName );

	if ( !src.IsLoaded() ) {
		return;
	}

	idToken token;

	while( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token.Length() > sdNetSession::MAX_NICKLEN ) {
			gameLocal.Warning("Skipping invalid name found in botnames.dat! The name ^7\"%s^7\" ^1is too long! Max name length is %i characters!", token.c_str(), sdNetSession::MAX_NICKLEN );
			continue;
		}

		botNames.Append( token.c_str() );
	}
}

/*
===============
idBotThreadData::LoadTrainingBotNames
===============
*/
void idBotThreadData::LoadTrainingBotNames() {

	bool inGDF = true;

	trainingGDFBotNames.Clear();
	trainingSTROGGBotNames.Clear();

	idLexer src( LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT );

	idStr botFileName = "bots/";
	botFileName += "training_botnames";

	botFileName.SetFileExtension( "dat" );

	src.LoadFile( botFileName );

	if ( !src.IsLoaded() ) {
		return;
	}

	idToken token;

	while( true ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token.Icmp( "[GDF]" ) == 0 ) {
			inGDF = true;
			continue;
		} else if ( token.Icmp( "[STROGG]" ) == 0 ) {
			inGDF = false;
			continue;
		}

		if ( token.Length() > sdNetSession::MAX_NICKLEN ) {
			gameLocal.Warning("Skipping invalid name found in training_botnames.dat! The name ^7\"%s^7\" ^1is too long! Max name length is %i characters!", token.c_str(), sdNetSession::MAX_NICKLEN );
			continue;
		}

		if ( inGDF ) {
			trainingGDFBotNames.Append( token.c_str() );
		} else {
			trainingSTROGGBotNames.Append( token.c_str() );
		}
	}
}

/*
================
idBotThreadData::FindDeclIndexForDeployable
================
*/
int idBotThreadData::FindDeclIndexForDeployable( const playerTeamTypes_t team, int deployableNum ) {

	const char* name;
	int index;
	const idDecl* decl;

	if ( team < GDF || team > STROGG ) {
		gameLocal.DWarning( "Invalid team passed to \"FindDeclIndexForDeployable\"!\n" );
		return -1;
	}

	if ( deployableNum == ARTILLERY ) {
		if ( team == STROGG ) {
			name = "deployobject_railhowitzer";
		} else {
			name = "deployobject_artillery";
		}
	} else if ( deployableNum == ROCKET_ARTILLERY ) {
		if ( team == STROGG ) {
			name = "deployobject_plasmamortar";
		} else {
			name = "deployobject_rockets";
		}
	} else if ( deployableNum == NUKE ) {
		if ( team == STROGG ) {
			name = "deployobject_ssg";
		} else {
			name = "deployobject_ssm";
		}
	} else if ( deployableNum == APT ) {
		if ( team == STROGG ) {
			name = "deployobject_antipersonnel_strogg";
		} else {
			name = "deployobject_antipersonnel_gdf";
		}
	} else if ( deployableNum == AVT ) {
		if ( team == STROGG ) {
			name = "deployobject_antiarmour_strogg";
		} else {
			name = "deployobject_antiarmour_gdf";
		}
	} else if ( deployableNum == RADAR ) {
		if ( team == STROGG ) {
			name = "deployobject_psi";
		} else {
			name = "deployobject_radar";
		}
	} else if ( deployableNum == AIT ) {
		if ( team == STROGG ) {
			name = "deployobject_shield_generator";
		} else {
			name = "deployobject_amt_gdf";
		}
	} else {
		gameLocal.DWarning( "Invalid deployable type passed to \"FindDeclIndexForDeployable\"!\n" );
		return -1;
	}

	decl = gameLocal.declDeployableObjectType[ name ];
	index = ( decl ) ? decl->Index() : -1;

	return index;
}

/*
================
idBotThreadData::RequestDeployableAtLocation
================
*/
bool idBotThreadData::RequestDeployableAtLocation( int clientNum, bool& needPause ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		return false;
	}

	bool success;
	int actionNumber = GetGameOutputState()->botOutput[ clientNum ].deployInfo.actionNumber;
	idVec3 deployPoint = GetGameOutputState()->botOutput[ clientNum ].deployInfo.location;
	idVec3 aimPoint = GetGameOutputState()->botOutput[ clientNum ].deployInfo.aimPoint;
	idVec3 vec;

	needPause = false;

	idPlayer* player = gameLocal.GetClient( clientNum );

	if ( player == NULL ) {
		gameLocal.DWarning( "Invalid player trying to request a deployable!" );
		return false;
	}

	int index = FindDeclIndexForDeployable( GetGameWorldState()->clientInfo[ clientNum ].team, GetGameOutputState()->botOutput[ clientNum ].deployInfo.deployableType );

	if ( index == -1 ) {
		gameLocal.DWarning( "Can't find deployable type %i in \"RequestDeployableAtLocation\"! ", GetGameOutputState()->botOutput[ clientNum ].deployInfo.deployableType );
		return false;
	}

	const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType.SafeIndex( index );

	if ( object == NULL ) {
		gameLocal.DWarning( "Client %i tried to drop a deployable that doesn't exist!", clientNum );
		return false;
	}

	if ( player->CheckDeployPosition( deployPoint, object ) != DR_CLEAR ) { //mal: oops - hint origin is not on target.
		gameLocal.DWarning( "Client %i can't deploy at action number %i!", clientNum, actionNumber );
		return false;
	}

	if ( botThreadData.DestroyClientDeployable( clientNum ) ) {
		needPause = true;
		return false;
	}

	if ( aimPoint == vec3_zero ) {
		vec = player->GetPhysics()->GetOrigin() - deployPoint;
	} else {
		vec = aimPoint - deployPoint;
	}

	float rotation = vec.ToAngles()[ YAW ];

	success = gameLocal.RequestDeployment( player, object, deployPoint, rotation, 0 );

	if ( success ) {
		GetGameWorldState()->clientInfo[ clientNum ].deployDelayTime = gameLocal.time + 60000; //mal: dont try this again for a while.
		gameLocal.SetTargetTimer( GetGameWorldState()->clientInfo[ clientNum ].scriptHandler.deployTimer, player, gameLocal.time + 60000 );
	} else {
		return false;
	}
	
	return true;
}

/*
================
idBotThreadData::CheckTargetLockInfo
================
*/
void idBotThreadData::CheckTargetLockInfo( int clientNum, float& boundsScale ) {
	int i;
	proxyInfo_t vehicleInfo;

	if ( GetBotAimSkill() < 1 || GetBotSkill() < BOT_SKILL_NORMAL ) {
		return;
	}

	if ( GetGameWorldState()->clientInfo[ clientNum ].proxyInfo.entNum == CLIENT_HAS_NO_VEHICLE ) {
		boundsScale = 2.0f;
		return;
	}

	vehicleInfo.entNum = 0;

	for( i = 0; i < MAX_VEHICLES; i++ ) {
		if ( GetGameWorldState()->vehicleInfo[ i ].entNum != GetGameWorldState()->clientInfo[ clientNum ].proxyInfo.entNum ) {
			continue;
		}

		vehicleInfo = GetGameWorldState()->vehicleInfo[ i ];
		break;
	}

	if ( vehicleInfo.entNum == 0 ) {
		return;
	}

	if ( vehicleInfo.type == ANANSI ) {
		boundsScale = 4.0f;
	} else {
		boundsScale = 2.0f;
	}
}

/*
================
idBotThreadData::GetNumClientsOnTeam

Looks for all clients that are on team "playerTeam" and returns the number of clients.
================
*/
int idBotThreadData::GetNumClientsOnTeam( const playerTeamTypes_t playerTeam ) {
	int n = 0;

	if( !GetBotWorldState() ) {
		return 0;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetBotWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].team == NOTEAM ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		n++;
	}
	return n;
}

/*
================
idBotThreadData::GetNumTeamBotsSpawningAtRearBase
================
*/
int idBotThreadData::GetNumTeamBotsSpawningAtRearBase( const playerTeamTypes_t playerTeam ) {
	int n = 0;
	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( !GetGameWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].wantsVehicle == false ) {
			continue;
		}

		n++;
	}

	return n;
}

/*
================
idBotThreadData::GetNumWeaponsOnTeam

Looks for all clients that are on team "playerTeam" and returns how many of them have "weapType".
================
*/
int idBotThreadData::GetNumWeaponsOnTeam( const playerTeamTypes_t playerTeam, const playerWeaponTypes_t weapType ) {
	int n = 0;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetGameWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		if ( GetGameWorldState()->clientInfo[ i ].weapInfo.primaryWeapon != weapType ) {
			continue;
		}

		n++;
	}

	return n;
}

/*
================
idBotThreadData::GetNumBotsOnTeam

Looks for all bots that are on team "playerTeam" and returns the number of bots.
================
*/
int idBotThreadData::GetNumBotsOnTeam( const playerTeamTypes_t playerTeam ) {
	int n = 0;

	if( !GetBotWorldState() ) {
		return 0;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		if ( !GetBotWorldState()->clientInfo[ i ].isBot ) {
			continue;
		}

		if ( !GetBotWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].team != playerTeam ) {
			continue;
		}

		n++;
	}
	return n;
}

/*
================
idBotThreadData::GetBotDebugInfo

Send the bot's debug data to the game.
================
*/
botDebugInfo_t idBotThreadData::GetBotDebugInfo( int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		botDebugInfo_t botInfo;
		memset( &botInfo, 0, sizeof( botInfo ) );
		return botInfo;
	} 

	idPlayer *player = gameLocal.GetClient( clientNum );
	botDebugInfo_t botInfo;
	memset( &botInfo, 0, sizeof( botInfo ) );

	if ( player != NULL && bot_debug.GetBool() ) {
		if ( GetGameWorldState()->clientInfo[ clientNum ].isBot ) {
			botInfo = botThreadData.GetGameOutputState()->botOutput[ clientNum ].debugInfo;
			botInfo.inUse = true;
		}
	}

	if ( botInfo.inUse ) {
		bot_showPath.SetInteger( clientNum );
//		aas_showObstacleAvoidance.SetInteger( 1 );
	}

	return botInfo;
}

/*
================
idBotThreadData::ChangeBotName
================
*/
void idBotThreadData::ChangeBotName( idPlayer* bot ) {
	idStr playerName;

	if ( !FindRandomBotName( bot->entityNumber, playerName ) ) {
		const char * basePlayerName = "Bot";

		if ( GetGameWorldState()->clientInfo[ bot->entityNumber ].team == GDF ) {
			basePlayerName = "GDF Bot";
		} else {
			basePlayerName = "Strogg Bot";
		}	

		sprintf( playerName, "%s %d", basePlayerName, bot->entityNumber );
	}

	networkSystem->ServerSetBotUserName( bot->entityNumber, playerName );
}

/*
============
idBotThreadData::FindRandomBotName

Make sure we dont have a duplicate name from a human player, or another bot.
Randomize the name selection.
============
*/
bool idBotThreadData::FindRandomBotName( int clientNum, idStr& botName ) {
	if ( botThreadData.botNames.Num() == 0 ) {
		return false;
	}

	if ( botThreadData.GetGameWorldState()->botGoalInfo.isTrainingMap ) {
		botName = "";//"Player";
		return true;
	}

	bool foundName = false;
	int i;
	int k;
	idStrList tempBotNames = botThreadData.botNames;

	while( tempBotNames.Num() > 0 ) {
		k = botThreadData.random.RandomInt( tempBotNames.Num() );
		botName = tempBotNames[ k ];
		gameLocal.CleanName( botName );
		
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( i == clientNum ) {
				continue;
			}

			idPlayer* player = gameLocal.GetClient( i );
			
			if ( !player ) {
				continue;
			}

			if ( !idStr::Icmp( botName, player->userInfo.cleanName ) ) {
				break;
			}
		}

		if ( i == MAX_CLIENTS ) {
			botName = tempBotNames[ k ];
			foundName = true; 
			break;
		}

		tempBotNames.RemoveIndexFast( k );
	}

	return foundName;
}

/*
============
idBotThreadData::FindActionByTypeForLocation
============
*/
int	idBotThreadData::FindActionByTypeForLocation( const idVec3& loc, const botActionGoals_t& obj, const playerTeamTypes_t& team ) {
	int actionNumber = ACTION_NULL;

	for( int i = 0; i < botThreadData.botActions.Num(); i++ ) {

        if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( team == GDF ) {
			if ( botThreadData.botActions[ i ]->GetHumanObj() != obj ) {
				continue;
			}
		} else {
			if ( botThreadData.botActions[ i ]->GetStroggObj() != obj ) {
				continue;
			}
		}

		if ( !botThreadData.botActions[ i ]->actionBBox.Expand( ACTION_BBOX_EXPAND_BIG ).ContainsPoint( loc ) ) {
			continue;
		}

		actionNumber = i;
		break;
	}

	return actionNumber;
}

/*
============
idBotThreadData::CheckCurrentChatRequests
============
*/
void idBotThreadData::CheckCurrentChatRequests() {

	if ( nextChatUpdateTime > gameLocal.time ) { 
		return;
	}

	nextChatUpdateTime = gameLocal.time + 1000;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientInfo_t& player = GetGameWorldState()->clientInfo[ i ];

		if ( player.inGame == false || player.team == NOTEAM ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.repairTargetNum == -1 ) {
			continue;
		}

		if ( player.repairTargetUpdateTime + MAX_TARGET_TIME > gameLocal.time || player.repairTargetUpdateTime == 0 ) {
			continue;
		}

		if ( player.repairTargetNeedsChat == false ) {
			continue;
		}

		for( int j = 0; j < MAX_CLIENTS; j++ ) { //mal: find a bot eng to respond in the negative.
			clientInfo_t& bot = GetGameWorldState()->clientInfo[ j ];

			if ( bot.inGame == false || bot.team == NOTEAM ) {
				continue;
			}
	
			if ( !bot.isBot ) {
				continue;
			}

			if ( bot.team != player.team ) {
				continue;
			}

			if ( bot.classType != ENGINEER ) {
				continue;
			}

			VOChat( CMD_DECLINED, j, true );
			break;
		}

		player.repairTargetNeedsChat = false; //mal: whether we responded or not, dont check this client again.
		break;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientInfo_t& player = GetGameWorldState()->clientInfo[ i ];

		if ( player.inGame == false || player.team == NOTEAM ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.killTargetNum == -1 ) {
			continue;
		}

		if ( player.killTargetUpdateTime + MAX_TARGET_TIME > gameLocal.time || player.killTargetUpdateTime == 0 ) {
			continue;
		}

		if ( player.killTargetNeedsChat == false ) {
			continue;
		}

		for( int j = 0; j < MAX_CLIENTS; j++ ) { //mal: find a bot to respond in the negative.
			clientInfo_t& bot = GetGameWorldState()->clientInfo[ j ];

			if ( bot.inGame == false || bot.team == NOTEAM ) {
				continue;
			}
	
			if ( !bot.isBot ) {
				continue;
			}

			if ( bot.team != player.team ) {
				continue;
			}

			VOChat( CMD_DECLINED, j, true );
			break;
		}

		player.killTargetNeedsChat = false; //mal: dont check this client again.
		break;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientInfo_t& player = GetGameWorldState()->clientInfo[ i ];

		if ( player.inGame == false || player.team == NOTEAM ) {
			continue;
		}

		if ( player.isBot ) {
			continue;
		}

		if ( player.commandRequestTime + MAX_TARGET_TIME > gameLocal.time || player.commandRequestTime == 0 ) {
			continue;
		}

		if ( player.commandRequestChatSent == true ) {
			continue;
		}

		for( int j = 0; j < MAX_CLIENTS; j++ ) { //mal: find a critical class bot to respond in the affirmative.
			clientInfo_t& bot = GetGameWorldState()->clientInfo[ j ];

			if ( bot.inGame == false || bot.team == NOTEAM ) {
				continue;
			}
	
			if ( !bot.isBot ) {
				continue;
			}

			if ( bot.team != player.team ) {
				continue;
			}

			if ( player.team == GDF ) {
				if ( bot.classType != GetGameWorldState()->botGoalInfo.team_GDF_criticalClass ) {
					continue;
				}
			} else {
				if ( bot.classType != GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass ) {
					continue;
				}
			}

			if ( !GetGameWorldState()->gameLocalInfo.heroMode && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {//mal: this is just for show anyhow, but if human has "Hero" mode on, the bot can't do the obj - so let the human know.
				VOChat( ACKNOWLEDGE_YES, j, true );
			} else {
				VOChat( CMD_DECLINED, j, true );
			}

			break;
		}

		player.commandRequestChatSent = true; //mal: whether we responded or not, dont check this client again.
		break;
	}
}

/*
============
idBotThreadData::ResetBotsInfo
============
*/
void idBotThreadData::ResetBotsInfo() {
	for( int i = 0; i < MAX_CLIENTS; i++ ) {

		idPlayer *player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		if ( !player->IsType( idBot::Type ) ) {
			continue;
		}

		clientInfo_t& client = GetGameWorldState()->clientInfo[ i ];
		client.isBot = true;
		client.inGame = true;

		idBotAI *botAI = new idBotAI;
		botAI->ClearBotState();
		botAI->ClearBotUcmd( i );
		botAI->SetBotClientNum( i );
		botAI->InitAAS();
		botThreadData.bots[ i ] = botAI;
	}
}

/*
============
idBotThreadData::SetupBotInfo
============
*/
void idBotThreadData::SetupBotInfo( int clientNum ) {
	clientInfo_t& client = GetGameWorldState()->clientInfo[ clientNum ];
	client.isBot = true;
	client.inGame = true;

	idBotAI *botAI = new idBotAI;
	botAI->ClearBotState();
	botAI->ClearBotUcmd( clientNum );
	botAI->SetBotClientNum( clientNum );
	botAI->InitAAS();
	botThreadData.bots[ clientNum ] = botAI;
}

/*
============
idBotThreadData::UpdateClientAbilities
============
*/
void idBotThreadData::UpdateClientAbilities( int clientNum, int rankLevel ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		return;
	}

	clientInfo_t& client = GetGameWorldState()->clientInfo[ clientNum ];

	client.abilities.rank = rankLevel;

	if ( client.abilities.rank == 1 ) {
		client.abilities.fasterMedicCharge = true;
		client.abilities.grenadeLauncher = true;
		client.abilities.stroggCovertNoFootSteps = true;
	} else if ( client.abilities.rank == 2 ) {
		client.abilities.stroggAdrenaline = true;
		client.abilities.stroggRepairDrone = true;
	} else if ( client.abilities.rank == 3 ) {
		client.abilities.gdfAdrenaline = true;
	} else {
		client.abilities.selfArmingMines = true;
		client.abilities.gdfStealthToRadar = true;
	}
}

/*
============
idBotThreadData::ClientIsValid
============
*/
bool idBotThreadData::ClientIsValid( int clientNum ) { 
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		if ( botThreadData.AllowDebugData() ) {
			assert( false );
		}
		return false;
	}
	return true;
}

/*
============
idBotThreadData::ClearClientBoundEntities
============
*/
void idBotThreadData::ClearClientBoundEntities( int clientNum ) {
	if ( !ClientIsValid( clientNum ) ) {
		return;
	}

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {

		deployableInfo_t& deployable = GetGameWorldState()->deployableInfo[ i ];

		if ( deployable.entNum == 0 ) {
			continue;
		}

		if ( deployable.ownerClientNum != clientNum ) {
			continue;
		}

		idEntity *deployableEnt = gameLocal.entities[ deployable.entNum ];

		if ( deployableEnt == NULL ) {
			continue;
		}

		deployableEnt->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_deployable_destruct" ), 1.0f, NULL );
		break;
	}

	clientInfo_t& client = GetGameWorldState()->clientInfo[ clientNum ];

	for( int i = 0; i < MAX_MINES; i++ ) {

		if ( client.weapInfo.landMines[ i ].entNum == 0 ) {
			continue;
		}

		idEntity *landMine = gameLocal.entities[ client.weapInfo.landMines[ i ].entNum ];

		if ( landMine == NULL ) {
			continue;
		}

		landMine->ProcessEvent( &EV_Remove ); //mal: fizzle, don't explode.
	}

	if ( client.weapInfo.covertToolInfo.entNum != 0 ) {

		idEntity *covertTool = gameLocal.entities[ client.weapInfo.covertToolInfo.entNum ];

		if ( covertTool != NULL ) {
			covertTool->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_generic" ), 999.0f, NULL );
		}
	}

	if ( client.supplyCrate.entNum != 0 ) {
		idEntity *supplyCrate = gameLocal.entities[ client.supplyCrate.entNum ];

		if ( supplyCrate != NULL ) {
			supplyCrate->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_grenade_frag_splash" ), 999.0f, NULL );
		}
	}
}

/*
============
idBotThreadData::CheckBotClassSpread
============
*/
void idBotThreadData::CheckBotClassSpread() { 
	if( !gameLocal.isServer ) {
		return;
	}

	if ( !bot_balanceCriticalClass.GetBool() ) {
		return;
	}

	if ( !bot_allowClassChanges.GetBool() ) {
		return;
	}

	if ( gameLocal.GameState() != GAMESTATE_ACTIVE ) {
		return;
	}

	if( !GetBotWorldState() ) {
		return;
	}

	if ( !CheckIfClientsInGameYet() ) {
		return;
	}

	if ( GetGameWorldState()->botGoalInfo.isTrainingMap ) {
		return;
	}

	if ( GetNumBotsInServer( NOTEAM ) == 0 ) {
		return;
	}

	if ( gameLocal.numClients < bot_minClients.GetInteger() ) { //mal: game is still in process of adding bots, so leave.
		return;
	}

	//mal: disabling for now, if have time - will come back to later....
/*
	for( int i = 0; i < MAX_CLIENTS; i++ ) { //mal: make sure any engs that have the nade launcher upgrade, switch to it.
		clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

		if ( !playerInfo.inGame ) {
			continue;
		}

		if ( playerInfo.classType != ENGINEER ) {
			continue;
		}

		if ( !playerInfo.isBot ) {
			continue;
		}

		if ( playerInfo.health > 0 ) {
			continue;
		}

		if ( playerInfo.weapInfo.primaryWeapon == SHOTGUN ) {
			continue;
		}

		if ( !playerInfo.abilities.grenadeLauncher ) {
			continue;
		}

		if ( ( playerInfo.lastWeapChangedTime + MIN_WEAPON_CHANGE_DELAY ) > gameLocal.time ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		const sdDeclPlayerClass* pc;

		if ( playerInfo.team == GDF ) {
			pc = gameLocal.declPlayerClassType[ "engineer" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "constructor" ];
		}

		player->ChangeClass( pc, 2 ); //mal: switch to the nade launcher.
		playerInfo.lastWeapChangedTime = gameLocal.time;
		playerInfo.weapInfo.hasNadeLauncher = true;
	}
*/

	if ( nextBotClassUpdateTime > gameLocal.time ) {
		return;
	}

	ManageBotClassesOnSmallServer();

	if ( !gameLocal.rules->IsWarmup() ) {
		nextBotClassUpdateTime = gameLocal.time + 5000; //mal: this isn't super critical.
	} else {
		nextBotClassUpdateTime = gameLocal.time + 1000;
	}

	return;
}

/*
============
idBotThreadData::CheckBotSpawnLocation
============
*/
void idBotThreadData::CheckBotSpawnLocation( idPlayer* player ) {
	if ( player == NULL ) {
		return;
	}

	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ];
	playerInfo.wantsVehicle = false;

	if ( playerInfo.team == STROGG ) {
		idEntity* spawnHost = player->GetSpawnPoint();
		
		if ( spawnHost != NULL && spawnHost->IsType( sdDynamicSpawnPoint::Type ) ) {
			return;
		} else {
			player->SetSpawnPoint( NULL );
		}
	} else {
		player->SetSpawnPoint( NULL );
	}

	if ( !bot_useRearSpawn.GetBool() ) {
		return;
	}

	sdTeamInfo* team = player->GetGameTeam();

	if ( team == NULL ) {
		return;
	}

	const playerTeamTypes_t botTeam = team->GetBotTeam();

	if ( botTeam == GDF && !botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].teamUsesSpawn ) {
		return;
	}

	if ( botTeam == STROGG && !botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].teamUsesSpawn ) {
		return;
	}

	idEntity* homeBaseSpawn = team->GetHomeBaseSpawn();

	if ( homeBaseSpawn != NULL && homeBaseSpawn->GetPhysics() ) {
		bool useRearSpawn = false;
		int percentage;

		if ( botTeam == GDF ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.team_GDF_criticalClass != playerInfo.classType ) {
				percentage = botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].percentage;
				useRearSpawn = true;
			}
		} else if ( botTeam == STROGG ) {
			if ( botThreadData.GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass != playerInfo.classType ) {
				percentage = botThreadData.GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].percentage;
				useRearSpawn = true;
			}
		}

		if ( useRearSpawn ) {
			idVec3 homeOrg = homeBaseSpawn->GetPhysics()->GetOrigin();

			int vehicleNum = FindHeavyVehicleNearLocation( player, homeOrg, MAX_VEHICLE_RANGE ); //mal: make sure have worthwhile vehicles before spawn back there.

			if ( vehicleNum == -1 ) {
				useRearSpawn = false;
			}				
		}
			
		if ( useRearSpawn && gameLocal.random.RandomInt( 100 ) < percentage && GetNumTeamBotsSpawningAtRearBase( botTeam ) < MAX_REAR_SPAWN_NUMBER ) {	
			player->SetSpawnPoint( team->GetHomeBaseSpawn() );
			playerInfo.wantsVehicle = true;
		}
	}
}

/*
==================
idBotThreadData::FindHeavyVehicleNearLocation

Expanded to make an intelligent choice as to whether or not the bot should spawn back to grab a vehicle. If no vehicle matches its current potential goals, wont spawn back.
==================
*/
int idBotThreadData::FindHeavyVehicleNearLocation( idPlayer* player, const idVec3& org, float range ) {
	bool hasAirVehicleGoal = false;
	bool hasArmorGoal = false;	
	int entNum = -1;
	float closest = idMath::INFINITY;
	sdTeamInfo* playerTeam = player->GetTeam();

	if ( playerTeam == NULL ) {
		assert( false );
		return entNum;
	}

	for( int i = 0; i < botActions.Num(); i++ ) {
		if ( !botThreadData.botActions[ i ]->ActionIsActive() ) {
			continue;
		}

		if ( !botThreadData.botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botActions[ i ]->GetBaseObjForTeam( playerTeam->GetBotTeam() ) != ACTION_VEHICLE_CAMP && botActions[ i ]->GetBaseObjForTeam( playerTeam->GetBotTeam() ) != ACTION_VEHICLE_ROAM ) {
			continue;
		}

		if ( botActions[ i ]->GetActionVehicleFlags( playerTeam->GetBotTeam() ) & AIR || botActions[ i ]->GetActionVehicleFlags( playerTeam->GetBotTeam() ) == NULL_VEHICLE_FLAGS ) {
			hasAirVehicleGoal = true;
		}

		if ( botActions[ i ]->GetActionVehicleFlags( playerTeam->GetBotTeam() ) & ARMOR || botActions[ i ]->GetActionVehicleFlags( playerTeam->GetBotTeam() ) == NULL_VEHICLE_FLAGS ) {
			hasArmorGoal = true;
		}

		if ( hasArmorGoal && hasAirVehicleGoal ) { //mal: only check for as long as need to.
			break;
		}
	}

	for( int i = 0; i < MAX_VEHICLES; i++ ) {

		const proxyInfo_t &vehicle = GetGameWorldState()->vehicleInfo[ i ];

		if ( vehicle.entNum == 0 ) {
			continue;
		}

		if ( vehicle.team != playerTeam->GetBotTeam() ) {
			continue;
		}

		if ( vehicle.inWater ) { //mal: someone drove it into the ocean.
			continue;
		}

		if ( vehicle.isFlipped ) { //mal: its on its back.
			continue;
		}

		if ( !vehicle.hasGroundContact && vehicle.type != DESECRATOR ) {
			continue;
		}

		if ( !( vehicle.flags & ARMOR ) && vehicle.type <= ICARUS ) {
			continue;
		}

		if ( !hasAirVehicleGoal && vehicle.type > ICARUS ) {
			continue;
		}

		if ( !hasArmorGoal && vehicle.flags & ARMOR ) {
			continue;
		}

		if ( vehicle.damagedPartsCount > 0 ) { //mal: dont get a vehicle that has a missing wheel,
			continue;
		}

		if ( vehicle.isOwned && vehicle.ownerNum != player->entityNumber ) {
			continue;
		}

	    if ( !vehicle.isEmpty ) {
			continue;
		}
	
		if ( !vehicle.inPlayZone ) {
			continue;
		}

		idVec3 vec = vehicle.origin - org;

		if ( vec.LengthSqr() > Square( range ) ) {
			continue;
		}

		entNum = vehicle.entNum;
		break;
	}

	return entNum;
}

/*
================
idBotThreadData::GetVehicleTargetOffset

TODO: add additional offsets for any other vehicles needed.
================
*/
float idBotThreadData::GetVehicleTargetOffset( const playerVehicleTypes_t vehicleType ) {
	float offset = 0.80f;

	if ( vehicleType == HOG ) {
		offset = 0.50f; // was .60
	} else if ( vehicleType == MCP ) {
		offset = 0.40f;
	} else if ( vehicleType == GOLIATH ) {
		offset = 0.50f;
	}

	return offset;
}

/*
================
idBotThreadData::GetVehicleTestBounds
================
*/
const idClipModel* idBotThreadData::GetVehicleTestBounds( const playerVehicleTypes_t vehicleType ) {
	if ( vehicleType == ICARUS || vehicleType == HUSKY ) {
		return personalVehicleClipModel;
	} else if ( vehicleType == GOLIATH ) {
		return goliathVehicleClipModel;
	} else {
		return genericVehicleClipModel;
	}
}

/*
================
idBotThreadData::GetDeployableActionNumber
================
*/
int idBotThreadData::GetDeployableActionNumber( idList< int >& deployableActions, const idVec3& deployableOrigin, const playerTeamTypes_t deployableTeam, int deployableType ) {
	if ( deployableActions.Num() == 0 ) {
		return ACTION_NULL;
	}

	int actionNumber = ACTION_NULL;

	for( int i = 0; i < deployableActions.Num(); i++ ) {
		if ( botActions[ deployableActions[ i ] ]->GetBaseObjForTeam( deployableTeam ) != ACTION_DROP_DEPLOYABLE && botActions[ deployableActions[ i ] ]->GetBaseObjForTeam( deployableTeam ) != ACTION_DROP_PRIORITY_DEPLOYABLE ) {
			continue;
		}

		int actionNum = deployableActions[ i ];

		if ( deployableType != -1 ) {
			if ( !( deployableType & botActions[ actionNum ]->GetDeployableType() ) ) {
				continue;
			}
		}

		float radius = botActions[ actionNum ]->GetRadius();
		idVec3 vec = botActions[ actionNum ]->GetActionOrigin() - deployableOrigin;

		if ( vec.LengthSqr() > Square( botActions[ actionNum ]->GetRadius() ) ) {
			continue;
		}

		actionNumber = actionNum;
		break;
	}

	return actionNumber;
}

/*
================
idBotThreadData::OnPlayerKilled
================
*/
void idBotThreadData::OnPlayerKilled( int clientNum ) {
	idPlayer* player = gameLocal.GetClient( clientNum );

	if ( player == NULL ) {
		return;
	}

	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ clientNum ];
	playerInfo.isCamper = false;
	playerInfo.wantsVehicle = false;
	playerInfo.killsSinceSpawn = 0;
	playerInfo.killCounter = 0;
	playerInfo.favoriteKill = -1;
	memset( playerInfo.kills, -1, sizeof( playerInfo.kills ) );
}

/*
================
idBotThreadData::Cmd_KillAllBots_f
================
*/
void idBotThreadData::Cmd_KillAllBots_f( const idCmdArgs &args ) {
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		if ( !player->IsType( idBot::Type ) ) {
			continue;
		}

		player->Kill( NULL );
	}
}

/*
================
idBotThreadData::Cmd_ResetAllBots_f
================
*/
void idBotThreadData::Cmd_ResetAllBots_f( const idCmdArgs & args ) {
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		if ( !player->IsType( idBot::Type ) ) {
			continue;
		}

		clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ i ];

		playerInfo.resetState = MAJOR_RESET_EVENT;
		botThreadData.bots[ i ]->ResetLastActionNum();
	}
}

/*
================
idBotThreadData::ServerHasHumans
================
*/
bool idBotThreadData::ServerHasHumans() {
	bool hasHumans = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );

		if ( player == NULL ) {
			continue;
		}

		if ( player->IsType( idBot::Type ) ) {
			continue;
		}

		hasHumans = true;
		break;
	}

	return hasHumans;
}

/*
================
idBotThreadData::ActionIsLinkedToAction
================
*/
bool idBotThreadData::ActionIsLinkedToAction( int curActionNum, int testActionNum ) {
	if ( !CheckActionIsValid( curActionNum ) || !CheckActionIsValid( testActionNum ) ) {
		return false;
	}

	if ( botActions[ curActionNum ]->GetActionGroup() != botActions[ testActionNum ]->GetActionGroup() ) {
		return false;
	}

	if ( ( botActions[ curActionNum ]->GetHumanObj() != botActions[ testActionNum ]->GetHumanObj() ) && ( botActions[ curActionNum ]->GetStroggObj() != botActions[ testActionNum ]->GetStroggObj() ) ) {
		return false;
	}

	return true;
}

/*
==================
idBotThreadData::CheckActionIsValid
==================
*/
bool idBotThreadData::CheckActionIsValid( int actionNumber ) {
	if ( actionNumber <= ACTION_NULL || actionNumber > botActions.Num() ) {
		assert( false );
		return false;
	}

	return true;
}

/*
==================
idBotThreadData::DestroyClientDeployable
==================
*/
bool idBotThreadData::DestroyClientDeployable( int clientNum ) {
	bool hasDeployable = false;

	for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
		if ( GetGameWorldState()->deployableInfo[ i ].entNum == 0 ) {
			continue;
		}

		if ( GetGameWorldState()->deployableInfo[ i ].ownerClientNum != clientNum ) {
			continue;
		}

		idEntity *deployable = gameLocal.entities[ GetGameWorldState()->deployableInfo[ i ].entNum ];

		if ( deployable == NULL || gameLocal.GetSpawnId( deployable ) != GetGameWorldState()->deployableInfo[ i ].spawnID ) {
			if ( bot_debug.GetBool() ) {
				gameLocal.Warning( "Bot has a deployable in the world it can't find!" );
			}
			break;
		}

		deployable->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_deployable_destruct" ), 1.0f, NULL );
		hasDeployable = true;
		break;
	}

	return hasDeployable;
}

/*
==================
idBotThreadData::DrawActionNumber
==================
*/
void idBotThreadData::DrawActionNumber( int actionNumber, const idMat3& viewAxis, bool drawAllInfo ) {
	idVec3 end = botActions[ actionNumber ]->origin;
	
	end.z += 64.0f;

	gameRenderWorld->DebugLine( colorGreen, botActions[ actionNumber ]->origin, end, 16 );
		
	end.z += 8.0f;

	gameRenderWorld->DrawText( va( "Action: %i", actionNumber ), end, bot_drawActionSize.GetFloat(), colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Name: %s", botActions[ actionNumber ]->name.c_str() ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "GDF Goal: %i     Strogg Goal: %i", botActions[ actionNumber ]->GetHumanObj(), botActions[ actionNumber ]->GetStroggObj() ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Active: %i        Group ID: %i", botActions[ actionNumber ]->ActionIsActive(), botActions[ actionNumber ]->GetActionGroup() ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Vehicle: %i       Deployable: %i", botActions[ actionNumber ]->GetActionVehicleType(), botActions[ actionNumber ]->GetDeployableType() ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "TargetRouteID: %i     BlindFire: %i", botActions[ actionNumber ]->routeID, botActions[ actionNumber ]->blindFire ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Valid Class: %i     Goal Time: %i", botActions[ actionNumber ]->validClasses, botActions[ actionNumber ]->actionTimeInSeconds ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Team Owner: %i     Has Obj: %i", botActions[ actionNumber ]->GetTeamOwner(), botActions[ actionNumber ]->hasObj ), end, 0.2f, colorWhite, viewAxis );
			
	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Action State: %i     Disguise Safe: %i", botActions[ actionNumber ]->actionState, botActions[ actionNumber ]->disguiseSafe ), end, 0.2f, colorWhite, viewAxis );
	
	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "No Hack: %i     Requires Vehicle Type: %i", botActions[ actionNumber ]->noHack, botActions[ actionNumber ]->requiresVehicleType ), end, 0.2f, colorWhite, viewAxis );

	end.z -= 16.0f;

	gameRenderWorld->DrawText( va( "Is Priority: %i", botActions[ actionNumber ]->priority ), end, 0.2f, colorWhite, viewAxis );

	if ( drawAllInfo ) {
		if ( !botActions[ actionNumber ]->actionBBox.IsCleared() ) {
			gameRenderWorld->DebugBox( colorRed, botActions[ actionNumber ]->actionBBox );
			gameRenderWorld->DebugArrow( colorLtBlue, botActions[ actionNumber ]->actionBBox.GetCenter(), botActions[ actionNumber ]->origin, 16 );
		}
	
		gameRenderWorld->DebugCircle( colorBlue, botActions[ actionNumber ]->origin, idVec3( 0, 0, 1 ), botActions[ actionNumber ]->radius, 8 );

		for( int j = 0; j < MAX_LINKACTIONS; j++ ) {
			if ( botActions[ actionNumber ]->actionTargets[ j ].inuse == false ) {
				continue;
			}

			end = botActions[ actionNumber ]->actionTargets[ j ].origin;
			end.z += 64.0f;

			gameRenderWorld->DebugLine( colorYellow, botActions[ actionNumber ]->actionTargets[ j ].origin, end, 16 );
			gameRenderWorld->DebugCircle( colorOrange, botActions[ actionNumber ]->actionTargets[ j ].origin, idVec3( 0, 0, 1 ), botActions[ actionNumber ]->actionTargets[ j ].radius, 8 );
			gameRenderWorld->DebugArrow( colorLtBlue, botActions[ actionNumber ]->actionTargets[ j ].origin, botActions[ actionNumber ]->origin, 16 );
		}
	}
}

/*
==================
idBotThreadData::DrawRearSpawns
==================
*/
void idBotThreadData::DrawRearSpawns() {
	idPlayer *player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	idMat3 viewAxis = player->GetViewAxis();
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
	sdTeamInfo* gdfTeam = &manager.GetTeamByIndex( GDF );
	sdTeamInfo* stroggTeam = &manager.GetTeamByIndex( STROGG );

	if ( gdfTeam == NULL || stroggTeam == NULL ) {
		return;
	}

	idEntity* gdfHomeBaseSpawn = gdfTeam->GetHomeBaseSpawn();
	idEntity* stroggHomeBaseSpawn = stroggTeam->GetHomeBaseSpawn();

	if ( ( gdfHomeBaseSpawn == NULL || gdfHomeBaseSpawn->GetPhysics() == NULL ) && ( stroggHomeBaseSpawn == NULL || stroggHomeBaseSpawn->GetPhysics() == NULL ) ) {
		return;
	}

	if ( gdfHomeBaseSpawn != NULL && gdfHomeBaseSpawn->GetPhysics() ) {
		gameRenderWorld->DebugCircle( colorGreen, gdfHomeBaseSpawn->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), 64.0f, 32 );
		gameRenderWorld->DebugCircle( colorBlue, gdfHomeBaseSpawn->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), MAX_VEHICLE_RANGE, 32 );
		gameRenderWorld->DrawText( "GDF Rear Spawn", gdfHomeBaseSpawn->GetPhysics()->GetOrigin() + idVec3( 0, 0, 82 ), 0.60f, colorWhite, viewAxis );
		gameRenderWorld->DrawText( va( "Percent: %i    Used: %i", GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].percentage, GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ GDF ].teamUsesSpawn ), gdfHomeBaseSpawn->GetPhysics()->GetOrigin(), 0.20f, colorWhite, viewAxis );
	}

	if ( stroggHomeBaseSpawn != NULL && stroggHomeBaseSpawn->GetPhysics() ) {
		gameRenderWorld->DebugCircle( colorGreen, stroggHomeBaseSpawn->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), 64.0f, 32 );
		gameRenderWorld->DebugCircle( colorBlue, stroggHomeBaseSpawn->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), MAX_VEHICLE_RANGE, 32 );
		gameRenderWorld->DrawText( "STROGG Rear Spawn", stroggHomeBaseSpawn->GetPhysics()->GetOrigin() + idVec3( 0, 0, 82 ), 0.60f, colorWhite, viewAxis );
		gameRenderWorld->DrawText( va( "Percent: %i    Used: %i", GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].percentage, GetGameWorldState()->gameLocalInfo.teamUsesSpawnInfo[ STROGG ].teamUsesSpawn ), stroggHomeBaseSpawn->GetPhysics()->GetOrigin(), 0.20f, colorWhite, viewAxis );
	}
}

/*
==================
idBotThreadData::FindDeliverActionNumber
==================
*/
int idBotThreadData::FindDeliverActionNumber() {
	int deliverActionNum = ACTION_NULL;

	for( int i = 0; i < botActions.Num(); i++ ) {
		if ( !botActions[ i ]->ActionIsValid() ) {
			continue;
		}

		if ( botActions[ i ]->GetObjForTeam( STROGG ) != ACTION_DELIVER && botActions[ i ]->GetObjForTeam( GDF ) != ACTION_DELIVER ) {
			continue;
		}

		deliverActionNum = i;
		break;
	}

	return deliverActionNum;
}

/*
==================
idBotThreadData::VehicleRouteThink
==================
*/
void idBotThreadData::VehicleRouteThink( bool setup, int& routeActionNumber, const idVec3& vehicleOrg ) {
	if ( setup ) {
		mcpRoutes.Clear();

		for( int i = 0; i < botActions.Num(); i++ ) {
			if ( !botActions[ i ]->ActionIsValid() ) {
				continue;
			}

			if ( botActions[ i ]->GetObjForTeam( GDF ) == ACTION_MCP_ROUTE_MARKER ) {
				mcpRoutes.Append( i );
			}
		}

		if ( mcpRoutes.Num() > 0 ) {
			int i = gameLocal.random.RandomInt( mcpRoutes.Num() );
			routeActionNumber = mcpRoutes[ i ];
		} else {
			routeActionNumber = ACTION_NULL;
		}
	} else {
		for( int i = 0; i < mcpRoutes.Num(); i++ ) {
			if ( !CheckActionIsValid( mcpRoutes[ i ] ) ) {
				continue;
			}

			idVec3 vec = botThreadData.botActions[ mcpRoutes[ i ] ]->GetActionOrigin() - vehicleOrg;
			
			if ( vec.LengthSqr() < Square( botThreadData.botActions[ mcpRoutes[ i ] ]->GetRadius() ) ) {
				routeActionNumber = ACTION_NULL;
				break;
			}
		}
	}
}

/*
==================
idBotThreadData::ManageBotClassesOnSmallServer
==================
*/
void idBotThreadData::ManageBotClassesOnSmallServer() {
	bool changedClass = false;

	for( int t = 0; t < MAX_TEAMS; t++ ) {
		int weaponLoadOut = gameLocal.random.RandomInt( 2 );
		int numDesiredCriticalClass;
		int	numDesiredMedicClass;
		playerTeamTypes_t botTeam;
		playerClassTypes_t criticalClass = NOCLASS;
		playerClassTypes_t desiredBotClass = NOCLASS;
		const sdDeclPlayerClass* pc;
		
		if ( t == GDF ) {
			botTeam = GDF;
			criticalClass = GetGameWorldState()->botGoalInfo.team_GDF_criticalClass;
			numDesiredCriticalClass = ( GetNumClientsOnTeam( GDF ) >= 7 ) ? 3 : 2;
			numDesiredMedicClass = ( GetNumClientsOnTeam( GDF ) >= 7 ) ? 2 : 1;
		} else if ( t == STROGG ) {
			botTeam = STROGG;
			criticalClass = GetGameWorldState()->botGoalInfo.team_STROGG_criticalClass;
			numDesiredCriticalClass = ( GetNumClientsOnTeam( STROGG ) >= 7 ) ? 3 : 2;
			numDesiredMedicClass = ( GetNumClientsOnTeam( STROGG ) >= 7 ) ? 2 : 1;
		}

		int numDesiredSoldierClass = ( criticalClass == MEDIC ) ? 2 : 1; //mal: if have lots of medics, then need more soldiers.

		int numMedic = GetNumClassOnTeam( botTeam, MEDIC );
		int numEng = GetNumClassOnTeam( botTeam, ENGINEER );
		int numFOps = GetNumClassOnTeam( botTeam, FIELDOPS );
		int numCovert = GetNumClassOnTeam( botTeam, COVERTOPS );
		int numSoldier = GetNumClassOnTeam( botTeam, SOLDIER );
		int numRLOnTeam = GetNumWeaponsOnTeam( botTeam, ROCKET );
		int	numHeavyOnTeam = GetNumWeaponsOnTeam( botTeam, HEAVY_MG ); 
		int numCritical;

		if ( criticalClass != NOCLASS && bot_doObjectives.GetBool() ) {
			numCritical = botThreadData.GetNumClassOnTeam( botTeam, criticalClass );
		} else {
			numCritical = -1;
		}

		//mal: make sure we always have a couple bots of the critical class first, unless the human wants to do the obj.
		if ( numCritical < numDesiredCriticalClass && numCritical != -1 ) {
			desiredBotClass = criticalClass;
		} else if ( numMedic < numDesiredMedicClass ) { //mal: medics are incredibly useful. On a full server - have more.
			desiredBotClass = MEDIC;
		} else if ( numSoldier < numDesiredSoldierClass ) {
			desiredBotClass = SOLDIER;
		} else if ( numEng == 0 ) {
			desiredBotClass = ENGINEER;
		} else if ( numCovert == 0 ) {
			desiredBotClass = COVERTOPS;
		} else if ( numFOps == 0 ) {
			desiredBotClass = FIELDOPS;
		}

		if ( desiredBotClass == NOCLASS ) {
			continue;
		}

		playerClassTypes_t sampleClass = NOCLASS;

		if ( numMedic > numDesiredMedicClass && ( criticalClass != MEDIC || numMedic > numDesiredCriticalClass ) ) {
			sampleClass = MEDIC;
		} else if ( numSoldier > numDesiredSoldierClass && ( criticalClass != SOLDIER || numSoldier > numDesiredCriticalClass ) ) {
			sampleClass = SOLDIER;
		} else if ( numEng > 1 && ( criticalClass != ENGINEER || numEng > numDesiredCriticalClass ) ) {
			sampleClass = ENGINEER;
		} else if ( numCovert > 1 && ( criticalClass != COVERTOPS || numCovert > numDesiredCriticalClass ) ) {
			sampleClass = COVERTOPS;
		} else if ( numFOps > 1 && ( criticalClass != FIELDOPS || numFOps > numDesiredCriticalClass ) ) {
			sampleClass = FIELDOPS;
		}

		if ( sampleClass == NOCLASS ) { //mal: dont have anyone to spare.
			continue;
		}

		if ( t == GDF ) {
			if ( desiredBotClass == MEDIC ) {
				pc = gameLocal.declPlayerClassType[ "medic" ];
			} else if ( desiredBotClass == SOLDIER ) {
				pc = gameLocal.declPlayerClassType[ "soldier" ];

				if ( numRLOnTeam == 0 ) {
					weaponLoadOut = 1;
				} else if ( numHeavyOnTeam == 0 ) {
					weaponLoadOut = 2;
				} else {
					weaponLoadOut = gameLocal.random.RandomInt( 4 );
				}
			} else if ( desiredBotClass == ENGINEER ) {
				pc = gameLocal.declPlayerClassType[ "engineer" ];
			} else if ( desiredBotClass == FIELDOPS ) {
				pc = gameLocal.declPlayerClassType[ "fieldops" ];
				weaponLoadOut = 0;
			} else if ( desiredBotClass == COVERTOPS ) {
				pc = gameLocal.declPlayerClassType[ "covertops" ];
			} else {
				continue;
			}
		} else if ( t == STROGG ) {
			if ( desiredBotClass == MEDIC ) {
				pc = gameLocal.declPlayerClassType[ "technician" ];
			} else if ( desiredBotClass == SOLDIER ) {
				pc = gameLocal.declPlayerClassType[ "aggressor" ];

				if ( numRLOnTeam == 0 ) {
					weaponLoadOut = 1;
				} else if ( numHeavyOnTeam == 0 ) {
					weaponLoadOut = 2;
				} else {
					weaponLoadOut = gameLocal.random.RandomInt( 4 );
				}
			} else if ( desiredBotClass == ENGINEER ) {
				pc = gameLocal.declPlayerClassType[ "constructor" ];
			} else if ( desiredBotClass == FIELDOPS ) {
				pc = gameLocal.declPlayerClassType[ "oppressor" ];
				weaponLoadOut = 0;
			} else if ( desiredBotClass == COVERTOPS ) {
				pc = gameLocal.declPlayerClassType[ "infiltrator" ];
			} else {
				continue;
			}
		}

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

			if ( !playerInfo.inGame ) {
				continue;
			}

			if ( playerInfo.team != botTeam ) {
				continue;
			}

			if ( playerInfo.classType != sampleClass ) {
				continue;
			}

			if ( !playerInfo.isBot ) {
				continue;
			}

			if ( playerInfo.isActor ) {
				continue;
			}

			if ( !gameLocal.rules->IsWarmup() && playerInfo.health > 0 ) {
				if ( ( playerInfo.lastClassChangeTime + MIN_CLASS_CHANGE_DELAY ) > gameLocal.time ) {
					continue;
				}

			if ( bots[ i ]->GetAIState() != LTG ) { //mal: now, make sure the bot isn't doing something important.
				continue;
			}

			if ( playerInfo.classType != criticalClass ) {
				if ( bots[ i ]->GetLTGType() != ROAM_GOAL && bots[ i ]->GetLTGType() != CAMP_GOAL ) {
					if ( bots[ i ]->GetLTGType() == DEFENSE_CAMP_GOAL && random.RandomInt( 100 ) > 50 ) { //mal: if defense camping, a random chance we might do the switch.
						continue;
					} else {
						continue;
					}
				}
			}
			}

			idPlayer* player = gameLocal.GetClient( i );

			if ( player == NULL ) {
				continue;
			}

			player->ChangeClass( pc, weaponLoadOut );
					
			if ( player->GetHealth() > 0 ) {
			player->Kill( NULL ); //mal: hurry up and suicide, and get back into the game as our new class!
			}
			playerInfo.lastClassChangeTime = gameLocal.time;
			changedClass = true;
			break;
		}
	}

	if ( !gameLocal.rules->IsWarmup() ) {
	if ( ( lastWeapChangedTime + MIN_WEAPON_CHANGE_DELAY ) > gameLocal.time ) {
		return;
	}
	}

	if ( changedClass == false || gameLocal.rules->IsWarmup() ) { //mal: make sure there is at least 1 RL soldier on each team.
		for( int t = 0; t < MAX_TEAMS; t++ ) {
			playerTeamTypes_t botTeam;
			const sdDeclPlayerClass* pc;

			if ( t == GDF ) {
				botTeam = GDF;
			} else if ( t == STROGG ) {
				botTeam = STROGG;
			}

			int numSoldier = GetNumClassOnTeam( botTeam, SOLDIER );

			if ( numSoldier == 0 ) {
				continue;
			}

			int numRLOnTeam = GetNumWeaponsOnTeam( botTeam, ROCKET );

			bool skipRLSoldiers = true;
			int switchWeaponNumber = 1;

			if ( numRLOnTeam > 0 ) {
				if ( numSoldier > 1 ) { //mal: with 1 or more soldiers, make sure have at least one heavy weapon.
					int numHeavyOnTeam = GetNumWeaponsOnTeam( botTeam, HEAVY_MG );

					if ( numHeavyOnTeam > 0 ) {
						continue;
					}

					if ( numRLOnTeam > 1 ) {
						skipRLSoldiers = false; //mal: have enough RLs on the map to borrow from them.
					}
					switchWeaponNumber = 2;
				} else {
				continue;
			}
			}
	
			if ( t == GDF ) {
				pc = gameLocal.declPlayerClassType[ "soldier" ];
			} else if ( t == STROGG ) {
				pc = gameLocal.declPlayerClassType[ "aggressor" ];
			}

			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

				if ( !playerInfo.inGame ) {
					continue;
				}

				if ( playerInfo.team != botTeam ) {
					continue;
				}

				if ( playerInfo.classType != SOLDIER ) {
					continue;
				}

				if ( !playerInfo.isBot ) {
					continue;
				}

				if ( skipRLSoldiers ) {
					if ( playerInfo.weapInfo.primaryWeapon == ROCKET ) {
						continue;
					}
				}

				idPlayer* player = gameLocal.GetClient( i );

				if ( player == NULL ) {
					continue;
				}

				player->ChangeClass( pc, switchWeaponNumber );
				lastWeapChangedTime = gameLocal.time;
				
				if ( gameLocal.rules->IsWarmup() ) {
					player->Kill( NULL ); //mal: hurry up and suicide, and get back into the game with our new weapon!
				}

				break;
			}
		}
	}

	if ( !checkedLastMapStageCovertWeapons && GetGameWorldState()->botGoalInfo.gameIsOnFinalObjective ) { //mal: on the last map obj, if theres no spots for snipers, turn in our sniper rifles.
		bool gdfSnipersExist = false;
		bool stroggSnipersExist = false;
		checkedLastMapStageCovertWeapons = true;

		for( int i = 0; i < botActions.Num(); i++ ) {
			if ( !botActions[ i ]->ActionIsActive() ) {
				continue;
			}

			if ( !botActions[ i ]->ActionIsValid() ) {
				continue;
			}

			if ( botActions[ i ]->GetHumanObj() == ACTION_SNIPE ) {
				gdfSnipersExist = true;
			}

			if ( botActions[ i ]->GetStroggObj() == ACTION_SNIPE ) {
				stroggSnipersExist = true;
			}

			if ( gdfSnipersExist && stroggSnipersExist ) {
				break;
			}
		}

		for( int t = 0; t < MAX_TEAMS; t++ ) {
			playerTeamTypes_t botTeam;

			if ( t == GDF ) {
				if ( gdfSnipersExist ) {
					continue;
				}

				botTeam = GDF;
			} else if ( t == STROGG ) {
				if ( stroggSnipersExist ) {
					continue;
				}

				botTeam = STROGG;
			}

			int numCoverts = GetNumClassOnTeam( botTeam, COVERTOPS );

			if ( numCoverts == 0 ) {
				continue;
			}

			int numSnipersOnTeam = GetNumWeaponsOnTeam( botTeam, SNIPERRIFLE );

			if ( numSnipersOnTeam == 0 ) {
				continue;
			}

			const sdDeclPlayerClass* pc;
	
			if ( t == GDF ) {
				pc = gameLocal.declPlayerClassType[ "covertops" ];
			} else if ( t == STROGG ) {
				pc = gameLocal.declPlayerClassType[ "infiltrator" ];
			}

			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

				if ( !playerInfo.inGame ) {
					continue;
				}

				if ( playerInfo.team != botTeam ) {
					continue;
				}

				if ( playerInfo.classType != COVERTOPS ) {
					continue;
				}

				if ( !playerInfo.isBot ) {
					continue;
				}

				idPlayer* player = gameLocal.GetClient( i );

				if ( player == NULL ) {
					continue;
				}

				player->ChangeClass( pc, 0 );
			}
		}
	}
}

/*
==================
idBotThreadData::EntityIsExplosiveCharge
==================
*/
bool idBotThreadData::EntityIsExplosiveCharge( int entNum, bool armedOnly, const playerTeamTypes_t& ignoreTeam ) {
	bool isCharge = false;

	for( int i = 0; i < MAX_CHARGES; i++ ) {
		if ( GetGameWorldState()->chargeInfo[ i ].entNum != entNum ) {
			continue;
		}

		if ( armedOnly ) {
			if ( GetGameWorldState()->chargeInfo[ i ].state != BOMB_ARMED ) {
				continue;
			}
		}

		if ( ignoreTeam != NOTEAM ) {
			if ( GetGameWorldState()->chargeInfo[ i ].team == ignoreTeam ) {
				continue;
			}
		}

		isCharge = true;
		break;
	}

	return isCharge;
}

/*
============
idBotThreadData::MapHasAnActor
============
*/
bool idBotThreadData::MapHasAnActor() {
	bool hasActor = false;

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i ) ) {
			continue;
		}

		const clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

		if ( !playerInfo.inGame ) {
			continue;
		}

		if ( !playerInfo.isActor ) {
			continue;
		}

		hasActor = true;
		break;
	}

	return hasActor;
}

/*
============
idBotThreadData::FindBotToBeActor
============
*/
void idBotThreadData::FindBotToBeActor() {
	if ( !GetGameWorldState()->gameLocalInfo.teamGDFHasHuman ) {
		return;
	}

	if ( actorMissionInfo.playerIsOnFinalMission && !actorMissionInfo.playerNeedsFinalBriefing ) {
		return;
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !ClientIsValid( i ) ) {
			continue;
		}
				
		clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

		if ( !playerInfo.inGame ) {
			continue;
		}

		if ( !playerInfo.isBot ) {
			continue;
		}

		if ( playerInfo.team != GDF ) {
			continue;
		}

		if ( playerInfo.classType != MEDIC ) {
			continue;
		}

		playerInfo.isActor = true;
		actorMissionInfo.actorClientNum = i;

		idPlayer* player = gameLocal.GetClient( i );

		if ( player != NULL ) {
			player->SetGodMode( true );
			networkSystem->ServerSetBotUserName( i, trainingGDFBotNames[ BOT_GUIDE_NAME_SLOT ] );
		}

		break;
	}
}

/*
============
idBotThreadData::TrainingThink

Quick and dirty func to track the progress of the training
============
*/
void idBotThreadData::TrainingThink() {
	if ( !actorMissionInfo.setup ) {
		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !ClientIsValid( i ) ) {
				continue;
			}

			const clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

			if ( !playerInfo.inGame ) {
				continue;
			}

			if ( playerInfo.isBot ) {
				continue;
			}

			if ( playerInfo.team != GDF ) {
				continue;
			}

			actorMissionInfo.setup = true;
			actorMissionInfo.targetClientNum = i;
			break;
		}

		int actionNum = botThreadData.FindActionByName( "hut_avt_deploy" );

		if ( actionNum != -1 ) {
			botThreadData.ignoreActionNumber = actionNum;
		}
	}

	if ( actorMissionInfo.deployableStageIsActive ) {
		for( int i = 0; i < MAX_DEPLOYABLES; i++ ) {
			if ( GetGameWorldState()->deployableInfo[ i ].entNum == 0 ) {
				continue;
			}

			if ( GetGameWorldState()->deployableInfo[ i ].ownerClientNum != actorMissionInfo.targetClientNum ) {
				continue;
			}

			sdObjectiveManager::GetInstance().PlayerDeployableDeployed();
			actorMissionInfo.deployableStageIsActive = false;
			break;
		}
	}

	if ( actorMissionInfo.targetClientNum > -1 && actorMissionInfo.targetClientNum < MAX_CLIENTS ) {
		idPlayer* player = gameLocal.GetClient( actorMissionInfo.targetClientNum );

		idWeapon* gun = player->weapon.GetEntity(); //mal: get some info on our gun

		if ( gun ) {
			gun->UpdateVisibility();
		}
	}

	if ( actorMissionInfo.actorClientNum > -1 && actorMissionInfo.actorClientNum < MAX_CLIENTS ) {
		idPlayer* player = gameLocal.GetClient( actorMissionInfo.actorClientNum );

		if ( player != NULL ) {
			player->SetGodMode( true );

			sdInventory& inventory = player->GetInventory();

			for( int i = 0; i < gameLocal.declAmmoTypeType.Num(); i++ ) {
				inventory.SetAmmo( i, inventory.GetMaxAmmo( i ) );
			}
		}		
	}

	if ( gameLocal.numClients == bot_minClients.GetInteger() && actorMissionInfo.setupBotNames ) {
		int gdfCounter = BOT_GUIDE_NAME_SLOT + 1;
		int stroggCounter = 0;

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !ClientIsValid( i ) ) {
				continue;
			}

			const clientInfo_t& playerInfo = GetGameWorldState()->clientInfo[ i ];

			if ( !playerInfo.inGame ) {
				continue;
			}

			if ( !playerInfo.isBot ) {
				continue;
			}

			if ( playerInfo.isActor ) { //mal: this is setup when we first set the actor.
				continue;
			}

			if ( playerInfo.team == GDF && gdfCounter < trainingGDFBotNames.Num() ) {
				networkSystem->ServerSetBotUserName( i, trainingGDFBotNames[ gdfCounter ] );
				gdfCounter++;
			} else if ( playerInfo.team == STROGG && stroggCounter < trainingSTROGGBotNames.Num() ) {
				networkSystem->ServerSetBotUserName( i, trainingSTROGGBotNames[ stroggCounter ] );
				stroggCounter++;
			}

			actorMissionInfo.setupBotNames = false;
		}
	}

	bot_doObjsInTrainingMode.SetBool( false );
}

/*
============
idBotThreadData::PlayerIsBeingBriefed
============
*/
bool idBotThreadData::PlayerIsBeingBriefed() {
	if ( GetGameWorldState()->botGoalInfo.isTrainingMap == false ) {
		return false;
	}

	if ( actorMissionInfo.targetClientNum > -1 && actorMissionInfo.targetClientNum < MAX_CLIENTS ) {
		idPlayer* player = gameLocal.GetClient( actorMissionInfo.targetClientNum );

		if ( player != NULL ) {
			if ( player->IsBeingBriefed() ) {
				return true;
			}
		}
	}

	return false;
}

/*
============
idBotThreadData::DeactivateBadMapActions

This is a nasty, 11th hour, 59th minute, 59th second hack! No time to remove these actions ingame, so just disable them.
============
*/
void idBotThreadData::DeactivateBadMapActions() { 
	if ( GetGameWorldState()->gameLocalInfo.gameMap == QUARRY ) { //mal: these actions face the GDF spawn, and their APTs - leading to MANY Strogg deaths.
		int actionNumber = botThreadData.FindActionByName( "strogg_jammer_3" );

		if ( actionNumber != -1 ) {
			botActions[ actionNumber ]->SetActive( false );
			botActions[ actionNumber ]->SetHumanObj( ACTION_NULL );
			botActions[ actionNumber ]->SetStroggObj( ACTION_NULL );
			botActions[ actionNumber ]->SetActionGroupNum( -1 );
		} else {
			Warning( "No valid bot action found by the name strogg_jammer_3!" );
		}

		actionNumber = botThreadData.FindActionByName( "strogg_jammer_4" );

		if ( actionNumber != -1 ) {
			botActions[ actionNumber ]->SetActive( false );
			botActions[ actionNumber ]->SetHumanObj( ACTION_NULL );
			botActions[ actionNumber ]->SetStroggObj( ACTION_NULL );
			botActions[ actionNumber ]->SetActionGroupNum( -1 );
		} else {
			Warning( "No valid bot action found by the name strogg_jammer_4!" );
		}
	}
}

/*
============
idBotThreadData::CheckIfClientsInGameYet
============
*/
bool idBotThreadData::CheckIfClientsInGameYet() {
	bool hasPlayers = false;
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		
		if ( !GetBotWorldState()->clientInfo[ i ].inGame ) {
			continue;
		}

		if ( GetBotWorldState()->clientInfo[ i ].classType == NOCLASS ) {
			continue;
		}

		idPlayer* player = gameLocal.GetClient( i );
		
		if ( player == NULL ) {
			continue;
		}

		if ( !player->CanPlay() ) {
			continue;
		}

		hasPlayers = true;
		break;
	}

	return hasPlayers;
}





