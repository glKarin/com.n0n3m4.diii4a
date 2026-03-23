/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_UpdateBeamVisibility( "<updateBeamVisibility>", NULL );
const idEventDef EV_UpdateShieldEntityVisibility( "<updateShieldEntityVisibility>", NULL );
const idEventDef EV_CheckTorpedoStatus( "<checkTorpedoStatus>", NULL );
const idEventDef EV_SetTargetEntityInSpace( "<setTargetEntityInSpace>", NULL );
const idEventDef EV_EngageWarp( "<engageWarp>", NULL );
const idEventDef EV_InitiateTransporter( "<initiateTransporter>" );
const idEventDef EV_InitiateRetrievalTransport( "<initiateRetrievalTransport>" );
const idEventDef EV_InitiateOffPadRetrievalTransport( "<initiateOffPadRetrievalTransport>", "e" );
const idEventDef EV_InitiateOffPadRetrievalTransportToReserveCrew( "<initiateOffPadRetrievalTransportToReserveCrew>", "e" );
const idEventDef EV_DisplayStoryWindow( "<displayStoryWindow>" );
const idEventDef EV_StartSynchdRedAlertFX( "<startSynchdRedAlertFX>" );
const idEventDef EV_TestScriptFunction( "testScriptFunction" );
const idEventDef EV_SetMinimumModulePowers( "setMinimumModulePowers" );
const idEventDef EV_HandleBeginningShipDormancy( "handleBeginningShipDormancy" );
const idEventDef EV_ConcludeShipDestructionSequence( "<concludeShipDestructionSequence>" );
const idEventDef EV_EvaluateShipRepairModeCycle( "<evaluateShipRepairModeCycle>" );
const idEventDef EV_UpdateViewscreenCamera( "<updateViewscreenCamera>" );
const idEventDef EV_DoWarpInVisualEffects( "<doWarpInVisualEffects>" );
const idEventDef EV_FinishWarpInVisualEffects( "<finishWarpInVisualEffects>" );
// SHIP AI EVENTS BEGIN
const idEventDef SHIP_AI_IsPlayerShip( "isPlayerShip", NULL,'d' );
const idEventDef SHIP_AI_IsAtPlayerSGPosition( "isAtPlayerSGPosition", NULL,'d' );
const idEventDef SHIP_AI_IsAtPlayerShipSGPosition( "isAtPlayerShipSGPosition", NULL,'d' );
const idEventDef SHIP_AI_SpaceEntityIsAtMySGPosition( "spaceEntityIsAtMySGPosition", "E",'d' );
const idEventDef SHIP_AI_IsDerelict( "isDerelict", NULL,'d' );
const idEventDef SHIP_AI_IsDerelictShip(  "isDerelictShip", "E",'d' );
const idEventDef SHIP_AI_IsFriendlyShip(  "isFriendlyShip", "E",'d' );
const idEventDef SHIP_AI_IsHostileShip(  "isHostileShip", "E",'d' );

const idEventDef SHIP_AI_SetMainGoal( "setMainGoal", "dE" );
const idEventDef SHIP_AI_AddMiniGoal( "addMiniGoal", "dE" );
const idEventDef SHIP_AI_RemoveMiniGoal( "removeMiniGoal", "dE" );
const idEventDef SHIP_AI_RemoveMiniGoalAction( "removeMiniGoalAction", "d" );
const idEventDef SHIP_AI_ClearMiniGoals( "clearMiniGoals" );
const idEventDef SHIP_AI_NextMiniGoal( "nextMiniGoal" );
const idEventDef SHIP_AI_PreviousMiniGoal( "previousMiniGoal" );
const idEventDef SHIP_AI_PrioritizeMiniGoal( "prioritizeMiniGoal", "dE" );

const idEventDef SHIP_AI_ReturnCurrentGoalAction( "returnCurrentGoalAction", NULL, 'd' );
const idEventDef SHIP_AI_ReturnCurrentGoalEntity( "returnCurrentGoalEntity", NULL, 'e' );
const idEventDef SHIP_AI_ReturnHostileTargetingMyGoalEntity( "returnHostileTargetingMyGoalEntity", NULL, 'e' );

const idEventDef SHIP_AI_SetTargetShipInSpace(  "setTargetShipInSpace", "E" );
const idEventDef SHIP_AI_ClearTargetShipInSpace(  "clearTargetShipInSpace" );
const idEventDef SHIP_AI_ReturnTargetShipInSpace( "returnTargetShipInSpace", NULL, 'e' );
const idEventDef SHIP_AI_GetATargetShipInSpace( "getATargetShipInSpace" );

const idEventDef SHIP_AI_ReturnBestFriendlyToProtect( "returnBestFriendlyToProtect", NULL, 'e' );
const idEventDef SHIP_AI_WeAreAProtector( "weAreAProtector", NULL,'d' );

const idEventDef SHIP_AI_ShipAIAggressiveness( "shipAIAggressiveness", NULL,'d' );

const idEventDef SHIP_AI_CrewAllAboard( "crewAllAboard", NULL,'d' );
const idEventDef SHIP_AI_CrewIsAboard( "crewIsAboard", "E",'d' );
const idEventDef SHIP_AI_SpareCrewIsNotOnTransporterPad( "spareCrewIsNotOnTransporterPad", NULL,'d' );
const idEventDef SHIP_AI_OrderSpareCrewToMoveToTransporterPad( "orderSpareCrewToMoveToTransporterPad" );
const idEventDef SHIP_AI_OrderSpareCrewToReturnToBattlestations( "orderSpareCrewToReturnToBattlestations" );
const idEventDef SHIP_AI_InitiateShipTransporter( "initiateShipTransporter" );
const idEventDef SHIP_AI_ShipIsSuitableForBoarding( "shipIsSuitableForBoarding", "E",'d' );

const idEventDef SHIP_AI_ShipShieldsAreLowEnoughForBoarding( "shipShieldsAreLowEnoughForBoarding", "E",'d' );

const idEventDef SHIP_AI_IsCrewRetreivableFrom( "isCrewRetreivableFrom", "E",'d' );
const idEventDef SHIP_AI_ShipWithOurCrewAboardIt( "shipWithOurCrewAboardIt", NULL, 'e' );
const idEventDef SHIP_AI_CrewOnBoardShipIsNotOnTransporterPad( "crewOnBoardShipIsNotOnTransporterPad", "E",'d' );
const idEventDef SHIP_AI_OrderCrewOnBoardShipToMoveToTransporterPad(  "orderCrewOnBoardShipToMoveToTransporterPad", "E" );
const idEventDef SHIP_AI_InitiateShipRetrievalTransporter( "initiateShipRetrievalTransporter" );

const idEventDef SHIP_AI_ActivateAutoModeForCrewAboardShip(  "activateAutoModeForCrewAboardShip", "E" );
const idEventDef SHIP_AI_DeactivateAutoModeForCrewAboardShip(  "deactivateAutoModeForCrewAboardShip", "E" );

const idEventDef SHIP_AI_PrioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue( "prioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue" );
const idEventDef SHIP_AI_PrioritizeEnginesInMyAutoPowerQueue( "prioritizeEnginesInMyAutoPowerQueue" );
const idEventDef SHIP_AI_PrioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue( "prioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue" );
const idEventDef SHIP_AI_TurnAutoFireOn( "turnAutoFireOn" );
const idEventDef SHIP_AI_CeaseFire( "ceaseFire" );

const idEventDef SHIP_AI_CanAttemptWarp( "canAttemptWarp", NULL,'d' );
const idEventDef SHIP_AI_AttemptWarpTowardsEntityAvoidingHostilesIfPossible( "attemptWarpTowardsEntityAvoidingHostilesIfPossible", "E" );
const idEventDef SHIP_AI_AttemptWarpAwayFromEntityAvoidingHostilesIfPossible( "attemptWarpAwayFromEntityAvoidingHostilesIfPossible", "E" );

const idEventDef SHIP_AI_ShouldHailThePlayerShip( "shouldHailThePlayerShip", NULL,'d' );
const idEventDef SHIP_AI_ReturnWaitToHailOrderNum( "returnWaitToHailOrderNum", NULL, 'f' );
const idEventDef SHIP_AI_PlayerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail( "playerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail", NULL,'d' );
const idEventDef SHIP_AI_AttemptToHailThePlayerShip( "attemptToHailThePlayerShip" );
const idEventDef SHIP_AI_InNoActionHailMode( "inNoActionHailMode", NULL,'d' );
const idEventDef SHIP_AI_PutAllShipsAtTheSameSGPosIntoNoActionHailMode( "putAllShipsAtTheSameSGPosIntoNoActionHailMode" );
const idEventDef SHIP_AI_ExitAllShipsAtTheSameSGPosFromNoActionHailMode( "exitAllShipsAtTheSameSGPosFromNoActionHailMode" );

const idEventDef SHIP_AI_GoToRedAlert( "goToRedAlert" );
const idEventDef SHIP_AI_CancelRedAlert( "cancelRedAlert" );
const idEventDef SHIP_AI_RaiseShields( "raiseShields" );
const idEventDef SHIP_AI_LowerShields( "lowerShields" );
const idEventDef SHIP_AI_BattleStations( "battleStations" );

const idEventDef SHIP_AI_HasFledFromShip( "hasFledFromShip", "E",'d' );
const idEventDef SHIP_AI_HasEscapedFromUs( "hasEscapedFromUs", "E",'d' );

const idEventDef SHIP_AI_ShouldAlwaysMoveToPrioritySpaceEntityToTarget( "shouldAlwaysMoveToPrioritySpaceEntityToTarget", NULL,'d' );
const idEventDef SHIP_AI_ReturnPrioritySpaceEntityToTarget( "returnPrioritySpaceEntityToTarget", NULL, 'e' );

const idEventDef SHIP_AI_ShouldAlwaysMoveToSpaceEntityToProtect( "shouldAlwaysMoveToSpaceEntityToProtect", NULL,'d' );
const idEventDef SHIP_AI_ShouldAlwaysMoveToPrioritySpaceEntityToProtect( "shouldAlwaysMoveToPrioritySpaceEntityToProtect", NULL,'d' );
const idEventDef SHIP_AI_ReturnPrioritySpaceEntityToProtect( "returnPrioritySpaceEntityToProtect", NULL, 'e' );

const idEventDef SHIP_AI_OptimizeModuleQueuesForIdleness( "optimizeModuleQueuesForIdleness" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForFleeing( "optimizeModuleQueuesForFleeing" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForDefending( "optimizeModuleQueuesForDefending" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForAttacking( "optimizeModuleQueuesForAttacking" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForSeekingAndDestroying( "optimizeModuleQueuesForSeekingAndDestroying" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForMoving( "optimizeModuleQueuesForMoving" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForBoarding( "optimizeModuleQueuesForBoarding" );
const idEventDef SHIP_AI_OptimizeModuleQueuesForRetrievingCrew( "optimizeModuleQueuesForRetrievingCrew" );

const idEventDef SHIP_AI_ReturnExtraShipAIWaitTimeForLowSensorsModuleEfficiency( "returnExtraShipAIWaitTimeForLowSensorsModuleEfficiency", NULL, 'f' );

const idEventDef SHIP_AI_IsDormantShip( "isDormantShip", NULL,'d' );
const idEventDef SHIP_AI_ShouldBecomeDormantShip( "shouldBecomeDormantShip", NULL,'d' );
const idEventDef SHIP_AI_BecomeDormantShip( "becomeDormantShip" );
const idEventDef SHIP_AI_BecomeNonDormantShip( "becomeNonDormantShip" );

const idEventDef SHIP_AI_ActivateShipAutoPower( "activateShipAutoPower" );
const idEventDef SHIP_AI_AttemptWarpTowardsShipPlayerIsOnBoard( "attemptWarpTowardsShipPlayerIsOnBoard" );

const idEventDef SHIP_AI_DetermineDefensiveActionsForSpareCrewOnBoard( "determineDefensiveActionsForSpareCrewOnBoard" );
const idEventDef SHIP_AI_AllModulesAreFullyRepaired( "allModulesAreFullyRepaired", NULL,'d' );
const idEventDef SHIP_AI_ActivateAutoModeForAllCrewAboardShip( "activateAutoModeForAllCrewAboardShip" );
const idEventDef SHIP_AI_DeactivateAutoModeForAllCrewAboardShip( "deactivateAutoModeForAllCrewAboardShip" );
const idEventDef SHIP_AI_SendCrewToStations( "sendCrewToStations" );

const idEventDef SHIP_AI_DoPhenomenonActions( "doPhenomenonActions", NULL,'d' );

const idEventDef SHIP_AI_ShipShouldFlee( "shipShouldFlee", NULL,'d' );
// SHIP AI EVENTS END

/*
Here is the heirarchy for the space entities
	-sbSpaceEntity
		--sbShip
			---Various unnecessary classes
		--sbStationarySpaceEntity
			---Various unnecessary classes
*/

CLASS_DECLARATION(idEntity, sbSpaceEntity)
END_CLASS

CLASS_DECLARATION(sbSpaceEntity, sbShip)

EVENT( EV_UpdateBeamVisibility,			sbShip::Event_UpdateBeamVisibility )
EVENT( EV_UpdateShieldEntityVisibility,	sbShip::Event_UpdateShieldEntityVisibility )
EVENT( EV_CheckTorpedoStatus,			sbShip::Event_CheckTorpedoStatus )
EVENT( EV_SetTargetEntityInSpace,		sbShip::Event_SetTargetEntityInSpace )
EVENT( EV_EngageWarp,					sbShip::Event_EngageWarp )

EVENT( EV_InitiateTransporter,			sbShip::Event_InitiateTransporter )
EVENT( EV_InitiateRetrievalTransport,	sbShip::Event_InitiateRetrievalTransport )

EVENT( EV_InitiateOffPadRetrievalTransport,			sbShip::Event_InitiateOffPadRetrievalTransport )
EVENT( EV_InitiateOffPadRetrievalTransportToReserveCrew,	sbShip::Event_InitiateOffPadRetrievalTransportToReserveCrew )

EVENT( EV_DisplayStoryWindow,			sbShip::Event_DisplayStoryWindow )
EVENT( EV_StartSynchdRedAlertFX,		sbShip::Event_StartSynchdRedAlertFX )
EVENT( EV_TestScriptFunction,			sbShip::Event_TestScriptFunction )
EVENT( EV_SetMinimumModulePowers,			sbShip::Event_SetMinimumModulePowers )
EVENT( EV_HandleBeginningShipDormancy,	sbShip::Event_HandleBeginningShipDormancy )
EVENT( EV_ConcludeShipDestructionSequence,	sbShip::Event_ConcludeShipDestructionSequence )
EVENT( EV_EvaluateShipRepairModeCycle,	sbShip::Event_EvaluateShipRepairModeCycle )
EVENT( EV_UpdateViewscreenCamera,		sbShip::Event_UpdateViewscreenCamera )
EVENT( EV_DoWarpInVisualEffects,		sbShip::Event_DoWarpInVisualEffects )
EVENT( EV_FinishWarpInVisualEffects,		sbShip::Event_FinishWarpInVisualEffects )
// SHIP AI EVENTS BEGIN
EVENT( SHIP_AI_IsPlayerShip,						sbShip::Event_IsPlayerShip )
EVENT( SHIP_AI_IsAtPlayerSGPosition,				sbShip::Event_IsAtPlayerSGPosition )
EVENT( SHIP_AI_IsAtPlayerShipSGPosition,			sbShip::Event_IsAtPlayerShipSGPosition )
EVENT( SHIP_AI_SpaceEntityIsAtMySGPosition,			sbShip::Event_SpaceEntityIsAtMySGPosition )
EVENT( SHIP_AI_IsDerelict,							sbShip::Event_IsDerelict )
EVENT( SHIP_AI_IsDerelictShip,						sbShip::Event_IsDerelictShip )
EVENT( SHIP_AI_IsFriendlyShip,						sbShip::Event_IsFriendlyShip )
EVENT( SHIP_AI_IsHostileShip,						sbShip::Event_IsHostileShip )

EVENT( SHIP_AI_SetMainGoal,							sbShip::Event_SetMainGoal )
EVENT( SHIP_AI_AddMiniGoal,							sbShip::Event_AddMiniGoal )
EVENT( SHIP_AI_RemoveMiniGoal,						sbShip::Event_RemoveMiniGoal )
EVENT( SHIP_AI_RemoveMiniGoalAction,				sbShip::Event_RemoveMiniGoalAction )
EVENT( SHIP_AI_ClearMiniGoals,						sbShip::Event_ClearMiniGoals )
EVENT( SHIP_AI_NextMiniGoal,						sbShip::Event_NextMiniGoal )
EVENT( SHIP_AI_PreviousMiniGoal,					sbShip::Event_PreviousMiniGoal )
EVENT( SHIP_AI_PrioritizeMiniGoal,					sbShip::Event_PrioritizeMiniGoal )

EVENT( SHIP_AI_ReturnCurrentGoalAction,				sbShip::Event_ReturnCurrentGoalAction )
EVENT( SHIP_AI_ReturnCurrentGoalEntity,				sbShip::Event_ReturnCurrentGoalEntity )
EVENT( SHIP_AI_ReturnHostileTargetingMyGoalEntity,	sbShip::Event_ReturnHostileTargetingMyGoalEntity )

EVENT( SHIP_AI_SetTargetShipInSpace,				sbShip::Event_SetTargetShipInSpace )
EVENT( SHIP_AI_ClearTargetShipInSpace,				sbShip::Event_ClearTargetShipInSpace )
EVENT( SHIP_AI_ReturnTargetShipInSpace,				sbShip::Event_ReturnTargetShipInSpace )
EVENT( SHIP_AI_GetATargetShipInSpace,				sbShip::Event_GetATargetShipInSpace )

EVENT( SHIP_AI_ReturnBestFriendlyToProtect,			sbShip::Event_ReturnBestFriendlyToProtect )
EVENT( SHIP_AI_WeAreAProtector,						sbShip::Event_WeAreAProtector )

EVENT( SHIP_AI_ShipAIAggressiveness,				sbShip::Event_ShipAIAggressiveness )

EVENT( SHIP_AI_CrewAllAboard,						sbShip::Event_CrewAllAboard )
EVENT( SHIP_AI_CrewIsAboard,						sbShip::Event_CrewIsAboard )
EVENT( SHIP_AI_SpareCrewIsNotOnTransporterPad,			sbShip::Event_SpareCrewIsNotOnTransporterPad )
EVENT( SHIP_AI_OrderSpareCrewToMoveToTransporterPad,	sbShip::Event_OrderSpareCrewToMoveToTransporterPad )
EVENT( SHIP_AI_OrderSpareCrewToReturnToBattlestations,	sbShip::Event_OrderSpareCrewToReturnToBattlestations )
EVENT( SHIP_AI_InitiateShipTransporter,				sbShip::Event_InitiateShipTransporter )
EVENT( SHIP_AI_ShipIsSuitableForBoarding,			sbShip::Event_ShipIsSuitableForBoarding )

EVENT( SHIP_AI_ShipShieldsAreLowEnoughForBoarding,			sbShip::Event_ShipShieldsAreLowEnoughForBoarding )

EVENT( SHIP_AI_IsCrewRetreivableFrom,				sbShip::Event_IsCrewRetreivableFrom )
EVENT( SHIP_AI_ShipWithOurCrewAboardIt,				sbShip::Event_ShipWithOurCrewAboardIt )
EVENT( SHIP_AI_CrewOnBoardShipIsNotOnTransporterPad,		sbShip::Event_CrewOnBoardShipIsNotOnTransporterPad )
EVENT( SHIP_AI_OrderCrewOnBoardShipToMoveToTransporterPad,	sbShip::Event_OrderCrewOnBoardShipToMoveToTransporterPad )
EVENT( SHIP_AI_InitiateShipRetrievalTransporter,	sbShip::Event_InitiateShipRetrievalTransporter )

EVENT( SHIP_AI_ActivateAutoModeForCrewAboardShip,	sbShip::Event_ActivateAutoModeForCrewAboardShip )
EVENT( SHIP_AI_DeactivateAutoModeForCrewAboardShip,	sbShip::Event_DeactivateAutoModeForCrewAboardShip )

EVENT( SHIP_AI_PrioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue,						sbShip::Event_PrioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue )
EVENT( SHIP_AI_PrioritizeEnginesInMyAutoPowerQueue,									sbShip::Event_PrioritizeEnginesInMyAutoPowerQueue )
EVENT( SHIP_AI_PrioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue,									sbShip::Event_PrioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue )
EVENT( SHIP_AI_TurnAutoFireOn,																sbShip::Event_TurnAutoFireOn )
EVENT( SHIP_AI_CeaseFire,																	sbShip::Event_CeaseFire )

EVENT( SHIP_AI_CanAttemptWarp,																sbShip::Event_CanAttemptWarp )
EVENT( SHIP_AI_AttemptWarpTowardsEntityAvoidingHostilesIfPossible,							sbShip::Event_AttemptWarpTowardsEntityAvoidingHostilesIfPossible )
EVENT( SHIP_AI_AttemptWarpAwayFromEntityAvoidingHostilesIfPossible,							sbShip::Event_AttemptWarpAwayFromEntityAvoidingHostilesIfPossible )

EVENT( SHIP_AI_ShouldHailThePlayerShip,									sbShip::Event_ShouldHailThePlayerShip )
EVENT( SHIP_AI_ReturnWaitToHailOrderNum,								sbShip::Event_ReturnWaitToHailOrderNum )
EVENT( SHIP_AI_PlayerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail,	sbShip::Event_PlayerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail )
EVENT( SHIP_AI_AttemptToHailThePlayerShip,								sbShip::Event_AttemptToHailThePlayerShip )
EVENT( SHIP_AI_InNoActionHailMode,										sbShip::Event_InNoActionHailMode )
EVENT( SHIP_AI_PutAllShipsAtTheSameSGPosIntoNoActionHailMode,		sbShip::Event_PutAllShipsAtTheSameSGPosIntoNoActionHailMode )
EVENT( SHIP_AI_ExitAllShipsAtTheSameSGPosFromNoActionHailMode,		sbShip::Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode )

EVENT( SHIP_AI_GoToRedAlert,						sbShip::Event_GoToRedAlert )
EVENT( SHIP_AI_CancelRedAlert,						sbShip::Event_CancelRedAlert )
EVENT( SHIP_AI_RaiseShields,						sbShip::Event_RaiseShields )
EVENT( SHIP_AI_LowerShields,						sbShip::Event_LowerShields )
EVENT( SHIP_AI_BattleStations,						sbShip::Event_BattleStations )

EVENT( SHIP_AI_HasFledFromShip,						sbShip::Event_HasFledFromShip )
EVENT( SHIP_AI_HasEscapedFromUs,						sbShip::Event_HasEscapedFromUs )

EVENT( SHIP_AI_ShouldAlwaysMoveToPrioritySpaceEntityToTarget,	sbShip::Event_ShouldAlwaysMoveToPrioritySpaceEntityToTarget )
EVENT( SHIP_AI_ReturnPrioritySpaceEntityToTarget,				sbShip::Event_ReturnPrioritySpaceEntityToTarget )

EVENT( SHIP_AI_ShouldAlwaysMoveToSpaceEntityToProtect,			sbShip::Event_ShouldAlwaysMoveToSpaceEntityToProtect )
EVENT( SHIP_AI_ShouldAlwaysMoveToPrioritySpaceEntityToProtect,	sbShip::Event_ShouldAlwaysMoveToPrioritySpaceEntityToProtect )
EVENT( SHIP_AI_ReturnPrioritySpaceEntityToProtect,				sbShip::Event_ReturnPrioritySpaceEntityToProtect )

EVENT( SHIP_AI_OptimizeModuleQueuesForIdleness,					sbShip::Event_OptimizeModuleQueuesForIdleness )
EVENT( SHIP_AI_OptimizeModuleQueuesForFleeing,					sbShip::Event_OptimizeModuleQueuesForFleeing )
EVENT( SHIP_AI_OptimizeModuleQueuesForDefending,				sbShip::Event_OptimizeModuleQueuesForDefending )
EVENT( SHIP_AI_OptimizeModuleQueuesForAttacking,				sbShip::Event_OptimizeModuleQueuesForAttacking )
EVENT( SHIP_AI_OptimizeModuleQueuesForSeekingAndDestroying,		sbShip::Event_OptimizeModuleQueuesForSeekingAndDestroying )
EVENT( SHIP_AI_OptimizeModuleQueuesForMoving,					sbShip::Event_OptimizeModuleQueuesForMoving )
EVENT( SHIP_AI_OptimizeModuleQueuesForBoarding,					sbShip::Event_OptimizeModuleQueuesForBoarding )
EVENT( SHIP_AI_OptimizeModuleQueuesForRetrievingCrew,			sbShip::Event_OptimizeModuleQueuesForRetrievingCrew )

EVENT( SHIP_AI_ReturnExtraShipAIWaitTimeForLowSensorsModuleEfficiency,	sbShip::Event_ReturnExtraShipAIWaitTimeForLowSensorsModuleEfficiency )

EVENT( SHIP_AI_IsDormantShip,				sbShip::Event_IsDormantShip )
EVENT( SHIP_AI_ShouldBecomeDormantShip,		sbShip::Event_ShouldBecomeDormantShip )
EVENT( SHIP_AI_BecomeDormantShip,			sbShip::Event_BecomeDormantShip )
EVENT( SHIP_AI_BecomeNonDormantShip,		sbShip::Event_BecomeNonDormantShip )

EVENT( SHIP_AI_ActivateShipAutoPower,					sbShip::Event_ActivateShipAutoPower )
EVENT( SHIP_AI_AttemptWarpTowardsShipPlayerIsOnBoard,	sbShip::Event_AttemptWarpTowardsShipPlayerIsOnBoard )

EVENT( SHIP_AI_DetermineDefensiveActionsForSpareCrewOnBoard,	sbShip::Event_DetermineDefensiveActionsForSpareCrewOnBoard )
EVENT( SHIP_AI_AllModulesAreFullyRepaired,						sbShip::Event_AllModulesAreFullyRepaired )
EVENT( SHIP_AI_ActivateAutoModeForAllCrewAboardShip,			sbShip::Event_ActivateAutoModeForAllCrewAboardShip )
EVENT( SHIP_AI_DeactivateAutoModeForAllCrewAboardShip,			sbShip::Event_DeactivateAutoModeForAllCrewAboardShip )
EVENT( SHIP_AI_SendCrewToStations,								sbShip::Event_SendCrewToStations )

EVENT( SHIP_AI_DoPhenomenonActions,								sbShip::Event_DoPhenomenonActions )

EVENT( SHIP_AI_ShipShouldFlee,				sbShip::Event_ShipShouldFlee )
// SHIP AI EVENTS END

END_CLASS

sbShip::sbShip() {

	TargetEntityInSpace = NULL;

	captaintestnumber = 0;
	stargridpositionx = 0;
	stargridpositiony = 0;
	stargriddestinationx = 0;
	stargriddestinationy = 0;

	track_on_stargrid = false;

	maximum_power_reserve = 0;
	current_power_reserve = 0;

	set_as_playership_on_player_spawn = false;

	ShipDiagramDisplayNode = NULL; // This will be used to calculate the position of entities on the ship diagrams.
	TestHideShipEntity = NULL;
	SelectedCrewMember	= NULL;
	TransporterBounds	= NULL;
	TransporterPad		= NULL;

	TransporterParticleEntityDef = NULL;
	TransporterParticleEntityFX	= NULL;
	TransporterParticleEntitySpawnMarker = NULL;

	MySkyPortalEnt		= NULL;
	alway_snap_to_my_sky_portal_entity = false;

	// Reserve Crew
	reserve_Crew.clear();
	max_reserve_crew = 0;

	// Crew Member Roles
	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		crew[i] = NULL;
	}

	// Ship Consoles (each one has the abilty to be assigned a module in its def).
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		consoles[i] = NULL;
	}

	// Ship Room Nodes
	for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		room_node[i] = NULL;
	}

	// beam
	beam = NULL;
	beamTarget = NULL;
	weapons_shot_missed = false;

	// torpedo
	projectileDef	= NULL;
	projectile		= NULL;
	torpedo_shot_missed = false;

	// star grid icon
	ShipStargridIcon = NULL;

	// ship image visual
	ShipImageVisual = NULL;

	// star grid artifact icon
	ShipStargridArtifactIcon = NULL;

	// module targeting
	//CurrentTargetModule	= NULL; // this is not used currently 03 18 13

	// module selection
	SelectedModule	= NULL;

	//shields, hull, structure
	shieldStrength = 0;
	max_shieldStrength = 0;
	hullStrength = 0;
	max_hullStrength = 0;
	health = 0;
	entity_max_health = 0;
	ShieldEntity = NULL;
	shields_repair_per_cycle = 0;

	// damage modifiers
	weapons_damage_modifier = 0;
	torpedos_damage_modifier = 0;

	// weapons and torpedos queues
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		WeaponsTargetQueue[i] = 0;
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		TorpedosTargetQueue[i] = 0;
	}
	weapons_autofire_on = false;
	torpedos_autofire_on = false;

	TempTargetEntityInSpace = NULL;
	ship_is_firing_weapons = false;
	ship_is_firing_torpedo = false;

	player_ceased_firing_on_this_ship = false;

	// ship dialogue system.
	hail_dialogue_gui_file = NULL;
	friendlinessWithPlayer = 0;
	has_forgiven_player = false;
	is_ignoring_player = false;
	currently_in_hail = false;

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		ModulesPowerQueue[i] = 0;
	}
	modules_power_automanage_on = false;
	current_automanage_power_reserve = 0;

	// necessary for warp effects display.
	verified_warp_stargrid_postion_x = 0;
	verified_warp_stargrid_postion_y = 0;

	// module charges timer
	update_module_charges_timer = 0;

	// stargrid ship discovery
	discovered_by_player = false;

	// show the artifact locations on the stargrid map
	has_artifact_aboard = false;

	// oxygen system
	current_oxygen_level = 0;
	evaluate_current_oxygen_timer = 0;
	infinite_oxygen = false;

	// red alert light system
	red_alert = false;

	// ship doors
	old_security_module_efficiency = 0;

	// minimum required shields percentage to prevent foreign transporters from beaming aboard your ship
	min_shields_percent_for_blocking_foreign_transporters = 0;

	// captain chair
	CaptainChair = NULL;

	// the view screen
	ViewScreenEntity = NULL;
	ship_that_just_fired_at_us = NULL;

	// ship is derelict status
	is_derelict = false;
	never_derelict = false;

	do_red_alert_when_derelict = false;

	//original name to deal with derelict string
	original_name = NULL;

	// scrap and interstellar credits
	current_materials_reserves = 0;
	current_currency_reserves = 0;

	// ship repair mode
	in_repair_mode = false;

	// to enable deconstruction effects
	ship_deconstruction_sequence_initiated = false;
	can_be_deconstructed = false;
	ship_beam_active = false;
	ShipBeam.target = NULL;

	// ready room captain chair
	ReadyRoomCaptainChair = NULL;

	// derelict ships and uninhabited planets and maybe some other types of space entities should always be neutral
	always_neutral = false;

	// for various special effects on the ship
	fx_color_theme	= vec3_origin;

	// for entity description display after ship sensors scan it
	was_sensor_scanned = false;

	// for raising/lowering shields - they wil default to being raised - sometimes it will be helpful in dialogue to raise/lower shields
	shields_raised = true;
	shieldStrength_copy = 0;

	// for ship light gui toggle button
	ship_lights_on = true;

	// for finding a suitable torpedo launchpoint without obstacles in the way
	suitable_torpedo_launchpoint_offset = vec3_origin;

	// ship ai
	current_goal_iterator = 0;
	we_are_a_protector = false;
	ai_always_targets_random_module = false;
	ship_ai_aggressiveness = 0;
	max_spare_crew_size = 0;
	max_modules_to_take_spare_crew_from = 0;
	ignore_boarding_problems = false;
	min_hullstrength_percent_required_for_boarding = 0.0f;
	min_environment_module_efficiency_required_for_boarding = 0;
	min_oxygen_percent_required_for_boarding = 0.0f;

	boarders_should_target_player = false;
	boarders_should_target_random_module = false;
	boarders_should_target_module_id = 0;

	in_no_action_hail_mode = false;
	should_hail_the_playership = false;
	// boyette today begin
	wait_to_hail_order_num = 100.0f;
	// boyette today end
	should_go_into_no_action_hail_mode_on_hail = false;
	battlestations = false;
	is_attempting_warp = false;

	should_warp_in_when_first_encountered = false;

	successful_flee_distance = 0;

	priority_space_entity_to_target = NULL;
	priority_space_entity_to_protect = NULL;
	prioritize_playership_as_space_entity_to_target = false;
	prioritize_playership_as_space_entity_to_protect = false;

	// ship ai become dormant conditions
	ship_begin_dormant = false;
	ship_is_never_dormant = false;
	ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos = false;
	ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos = false;
	ship_is_dormant_until_awoken_by_player_shiponboard = false;
	ship_is_dormant_until_awoken_by_an_active_ship = false;

	ship_modules_must_be_repaired_to_go_dormant = false;

	try_to_be_dormant = false;

	flee_hullstrength_percentage = 0.0f;

	// hail conditionals
	hail_conditionals_met = false;
	hail_conditional_hull_below_this_percentage = 0.0f;
	hail_conditional_no_hostiles_at_my_stargrid_position = false;
	hail_conditional_hostiles_at_my_stargrid_position = false;
	hail_conditional_no_friendlies_at_my_stargrid_position = false;
	hail_conditional_friendlies_at_my_stargrid_position = false;
	hail_conditional_captain_officer_killed = false;
	hail_conditional_player_is_aboard_playership = false;
	hail_conditional_not_at_player_shiponboard_position = false;
	hail_conditional_is_playership_target = false;

	//stargrid random location team and variables
	stargridstartpos_random_team = "";
	stargridstartpos_try_to_be_alone = false;
	stargridstartpos_avoid_entities_of_same_class = false;

	// phenomenon actions variables begin
	phenomenon_show_damage_or_disable_beam = false;
	phenomenon_should_do_ship_damage = false;
	phenomenon_should_damage_modules = false;
		phenomenon_module_damage_amount = 0;
	phenomenon_should_damage_random_module = false;
	phenomenon_should_disable_modules = false;
	phenomenon_should_disable_random_module = false;
	phenomenon_should_set_oxygen_level = false;
		phenomenon_oxygen_level_to_set = 0;
	phenomenon_should_set_ship_shields_to_zero = false;
	phenomenon_should_spawn_entity_def_on_playership = false;
		phenomenon_number_of_entity_defs_to_spawn = 0;
		phenomenon_entity_def_to_spawn = NULL;
	phenomenon_should_change_random_playership_crewmember_team = false;
	phenomenon_should_make_everything_go_slowmo = false;
	phenomenon_should_toggle_slowmo_on_and_off = false;
	phenomenon_should_ignore_the_rest_of_the_ship_ai_loop = false;
	// phenomenon actions variables end

	// for the low oxygen alert sound - so it only plays when we first take low oxygen damage - resets when oxygen is at a safe level
	play_low_oxygen_alert_sound = true;
	show_low_oxygen_alert_display = true;

	// entity specific stargrid story window
	story_window_satisfied = true;
}
sbShip::~sbShip( void ) {
	if ( beam ) {
		delete beam;
	}
	if ( beamTarget ) {
		delete beamTarget;
	}
}
void sbShip::Spawn() {
	regenAmount = spawnArgs.GetInt("regenAmount"); //the amount to regen each entity, each frame

	ShipStargridIcon = spawnArgs.GetString("ship_stargrid_icon", "textures/images_used_in_source/default_ship_stargrid_icon.tga");
	ShipImageVisual = spawnArgs.GetString("ship_image_visual", "textures/images_used_in_source/default_ship_image_visual.tga");

	ShipStargridArtifactIcon = spawnArgs.GetString("ship_stargrid_artifact_icon", "textures/images_used_in_source/default_ship_stargrid_icon.tga");

	// entity specific stargrid story window BEGIN
	if ( spawnArgs.GetString( "story_window_to_display" , NULL ) != NULL ) {
		story_window_satisfied = false;
	}
	// entity specific stargrid story window END

	track_on_stargrid = spawnArgs.GetBool( "track_on_stargrid", "1" );

	weapons_damage_modifier = spawnArgs.GetInt("weapons_damage_modifier", "100");
	torpedos_damage_modifier = spawnArgs.GetInt("torpedos_damage_modifier", "100");

	shields_repair_per_cycle = spawnArgs.GetInt("shields_repair_per_cycle", "10");

	max_shieldStrength = spawnArgs.GetInt("max_shieldStrength", "1000");
	max_shieldStrength = idMath::ClampInt(0,MAX_MAX_SHIELDSTRENGTH,max_shieldStrength);
	shieldStrength = spawnArgs.GetInt("shieldStrength", "1000");
	if ( shieldStrength > max_shieldStrength ) {
		shieldStrength = max_shieldStrength;
	}
	shieldStrength_copy = shieldStrength;
	if ( !shields_raised ) {
		shieldStrength = 0;
	}

	max_hullStrength = spawnArgs.GetInt("max_hullStrength", "1000");
	max_hullStrength = idMath::ClampInt(0,MAX_MAX_HULLSTRENGTH,max_hullStrength);
	hullStrength = spawnArgs.GetInt("hullStrength", "1000");
	if ( hullStrength > max_hullStrength ) {
		hullStrength = max_hullStrength;
	}
	SetShaderParm( 10, 1.0f -( (float)hullStrength / (float)max_hullStrength ) ); // set the initial damage decal alpha

	captaintestnumber = 5;

	stargridpositionx = spawnArgs.GetInt("stargridstartposx", "1");
	stargridpositiony = spawnArgs.GetInt("stargridstartposy", "1");
	if ( stargridpositionx > MAX_STARGRID_X_POSITIONS ) {
		spawnArgs.SetBool("stargridstartpos_random", "1");
	}
	if ( stargridpositionx <= 0 ) {
		spawnArgs.SetBool("stargridstartpos_random", "1");
	}
	if ( stargridpositiony > MAX_STARGRID_Y_POSITIONS ) {
		spawnArgs.SetBool("stargridstartpos_random", "1");
	}
	if ( stargridpositiony <= 0 ) {
		spawnArgs.SetBool("stargridstartpos_random", "1");
	}

	stargriddestinationx = MAX_STARGRID_X_POSITIONS + 10; // to get the targeting reticule off screen and prevent warps(can't warp if destination is not valid)
	stargriddestinationy = MAX_STARGRID_Y_POSITIONS + 10; // to get the targeting reticule off screen and prevent warps(can't warp if destination is not valid)
	
	maximum_power_reserve = spawnArgs.GetInt("maximum_power_reserve", "6");
	current_power_reserve = maximum_power_reserve;

	// BOYETTE GAMEPLAY BALANCING BEGIN
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		module_max_powers[i] = spawnArgs.GetInt( module_description[i] + "_module_max_power", "1" );
	}
	// BOYETTE GAMEPLAY BALANCING END

	set_as_playership_on_player_spawn = spawnArgs.GetBool("set_as_playership_on_player_spawn", "0");

	// RESERVE CREW BEGIN
	max_reserve_crew = spawnArgs.GetInt("max_reserve_crew","20");
	max_reserve_crew = idMath::ClampInt(0,MAX_RESERVE_CREW_ON_SHIPS,max_reserve_crew);
	max_reserve_crew = max_reserve_crew - (max_reserve_crew % 10); // clamp to lowest ten - will not go lower than 10
	// RESERVE CREW END

// boyette map entites link up begin
	// wait until all entities are spawned to get them
	idStr TestHideShip = spawnArgs.GetString("test_hide_ship",NULL);

	idStr TransporterBoundsName = spawnArgs.GetString("transporter_bounds",NULL);
	idStr TransporterPadName = spawnArgs.GetString("transporter_pad",NULL);
	idStr TransporterParticleEntitySpawnMarkerName = spawnArgs.GetString("transporter_particle_marker",NULL);

	idStr ShipDiagramDisplayNodeName = spawnArgs.GetString("ship_diagram_display_node",NULL);

	idStr ShieldEntityName = spawnArgs.GetString("ship_shield_entity",NULL);

	idStr CaptainChairName = spawnArgs.GetString("captain_chair",NULL);

	idStr ViewScreenName = spawnArgs.GetString("ship_viewscreen_entity",NULL);

	idStr ReadyRoomCaptainChairName = spawnArgs.GetString("ready_room_captain_chair",NULL);

	fx_color_theme = spawnArgs.GetVector("ship_fx_color_theme","0.0 0.7 0.7"); // cyan will be the default

	was_sensor_scanned = spawnArgs.GetBool("was_sensor_scanned","0"); // we will assume that entities have not been scanned - some well known planets and ships will not need to be scanned to display their description.
	if ( set_as_playership_on_player_spawn ) {
		was_sensor_scanned = true;
	}

	should_warp_in_when_first_encountered = spawnArgs.GetBool("should_warp_in_when_first_encountered","0");

	// SHIP AI VARIABLES BEGIN
	Event_SetMainGoal(SHIP_AI_IDLE,NULL); // we should start off idle

	ai_always_targets_random_module = spawnArgs.GetBool("ai_always_targets_random_module","1"); // for ship AI

	ship_ai_aggressiveness = spawnArgs.GetInt("ship_ai_aggressiveness","1"); // for ship AI

	we_are_a_protector = spawnArgs.GetBool("we_are_a_protector","0"); // for ship AI

	max_spare_crew_size = spawnArgs.GetInt("max_spare_crew_size","3"); // for ship AI
	max_modules_to_take_spare_crew_from = spawnArgs.GetInt("max_modules_to_take_spare_crew_from","3"); // for ship AI

	ignore_boarding_problems = spawnArgs.GetBool("ignore_boarding_problems","0"); // for ship AI
	min_hullstrength_percent_required_for_boarding = spawnArgs.GetFloat("min_hullstrength_percent_required_for_boarding","0.4"); // for ship AI
	min_environment_module_efficiency_required_for_boarding = spawnArgs.GetFloat("min_environment_module_efficiency_required_for_boarding","50"); // for ship AI // is 50%
	min_oxygen_percent_required_for_boarding = spawnArgs.GetFloat("min_oxygen_percent_required_for_boarding","0.75"); // for ship AI

	should_hail_the_playership = spawnArgs.GetBool("should_hail_the_playership", "0"); // for ship AI
	wait_to_hail_order_num = spawnArgs.GetFloat("wait_to_hail_order_num","100.0"); // for ship AI
	should_go_into_no_action_hail_mode_on_hail = spawnArgs.GetBool("should_go_into_no_action_hail_mode_on_hail", "0"); // for ship AI // purpose: some ships should go into NoActionHailMode upon successfully hailing the player ship - other ships should not - and should hail and attack the playership at the same time.

	successful_flee_distance = spawnArgs.GetInt("successful_flee_distance","1"); // for ship AI. Anything >= than this is a successful flee

	// ship ai become dormant conditions
	ship_begin_dormant = spawnArgs.GetBool("ship_begin_dormant", "1"); // for ship AI
	if ( set_as_playership_on_player_spawn ) {
		ship_begin_dormant = false;
	}
	ship_is_never_dormant = spawnArgs.GetBool("ship_is_never_dormant", "0"); // for ship AI
	ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos = spawnArgs.GetBool("ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos", "1"); // for ship AI
	ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos = spawnArgs.GetBool("ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos", "0"); // for ship AI
	ship_is_dormant_until_awoken_by_player_shiponboard = spawnArgs.GetBool("ship_is_dormant_until_awoken_by_player_shiponboard", "0"); // for ship AI
	ship_is_dormant_until_awoken_by_an_active_ship = spawnArgs.GetBool("ship_is_dormant_until_awoken_by_an_active_ship", "0"); // for ship AI

	ship_modules_must_be_repaired_to_go_dormant = spawnArgs.GetBool("ship_modules_must_be_repaired_to_go_dormant", "0"); // for ship AI

	flee_hullstrength_percentage = spawnArgs.GetFloat("flee_hullstrength_percentage", "0.0"); // for ship AI
		
		// hail conditions begin
		hail_conditional_hull_below_this_percentage = spawnArgs.GetFloat("hail_conditional_hull_below_this_percentage", "0.0"); // for ship AI
		hail_conditional_no_hostiles_at_my_stargrid_position = spawnArgs.GetBool("hail_conditional_no_hostiles_at_my_stargrid_position", "0"); // for ship AI
		hail_conditional_hostiles_at_my_stargrid_position = spawnArgs.GetBool("hail_conditional_hostiles_at_my_stargrid_position", "0"); // for ship AI
		hail_conditional_no_friendlies_at_my_stargrid_position = spawnArgs.GetBool("hail_conditional_no_friendlies_at_my_stargrid_position", "0"); // for ship AI
		hail_conditional_friendlies_at_my_stargrid_position = spawnArgs.GetBool("hail_conditional_friendlies_at_my_stargrid_position", "0"); // for ship AI
		hail_conditional_captain_officer_killed = spawnArgs.GetBool("hail_conditional_captain_officer_killed", "0"); // for ship AI
		hail_conditional_player_is_aboard_playership = spawnArgs.GetBool("hail_conditional_player_is_aboard_playership", "0"); // for ship AI
		hail_conditional_not_at_player_shiponboard_position = spawnArgs.GetBool("hail_conditional_not_at_player_shiponboard_position", "0"); // for ship AI
		hail_conditional_is_playership_target = spawnArgs.GetBool("hail_conditional_is_playership_target", "0"); // for ship AI
		// hail conditions end

		// phenomenon actions variables begin
		phenomenon_show_damage_or_disable_beam = spawnArgs.GetBool("phenomenon_show_damage_or_disable_beam", "0"); // for ship phenomenon AI
		phenomenon_should_do_ship_damage = spawnArgs.GetBool("phenomenon_should_do_ship_damage", "0"); // for ship phenomenon AI - used in conjuction with phenomenon_should_damage_modules
		phenomenon_should_damage_modules = spawnArgs.GetBool("phenomenon_should_damage_modules", "0"); // for ship phenomenon AI
			phenomenon_module_damage_amount = spawnArgs.GetInt( "phenomenon_module_damage_amount", "100" ); // for ship phenomenon AI - this still has to go through the shields like a laser weapon
			phenomenon_module_ids_to_damage = SplitStringToInts( spawnArgs.GetString( "phenomenon_module_ids_to_damage",""), ',' );
		phenomenon_should_damage_random_module = spawnArgs.GetBool("phenomenon_should_damage_random_module", "0"); // for ship phenomenon AI
		phenomenon_should_disable_modules = spawnArgs.GetBool("phenomenon_should_disable_modules", "0"); // for ship phenomenon AI
			phenomenon_module_ids_to_disable = SplitStringToInts( spawnArgs.GetString( "phenomenon_module_ids_to_disable",""), ',' );
		phenomenon_should_disable_random_module = spawnArgs.GetBool("phenomenon_should_disable_random_module", "0"); // for ship phenomenon AI
		phenomenon_should_set_oxygen_level = spawnArgs.GetBool("phenomenon_should_set_oxygen_level", "0"); // for ship phenomenon AI
			phenomenon_oxygen_level_to_set = spawnArgs.GetInt( "phenomenon_oxygen_level_to_set", "50" ); // for ship phenomenon AI - this gets clamped to between 0 to 100
		phenomenon_should_set_ship_shields_to_zero = spawnArgs.GetBool("phenomenon_should_set_ship_shields_to_zero", "0"); // for ship phenomenon AI
		phenomenon_should_spawn_entity_def_on_playership = spawnArgs.GetBool("phenomenon_should_spawn_entity_def_on_playership", "0"); // for ship phenomenon AI
			phenomenon_number_of_entity_defs_to_spawn = spawnArgs.GetInt( "phenomenon_number_of_entity_defs_to_spawn", "1" ); // for ship phenomenon AI - this gets clamped to between 0 to 5
			phenomenon_entity_def_to_spawn = spawnArgs.GetString("phenomenon_entity_def_to_spawn",NULL);
		phenomenon_should_change_random_playership_crewmember_team = spawnArgs.GetBool("phenomenon_should_change_random_playership_crewmember_team", "0"); // for ship phenomenon AI
		phenomenon_should_make_everything_go_slowmo = spawnArgs.GetBool("phenomenon_should_make_everything_go_slowmo", "0"); // for ship phenomenon AI
		phenomenon_should_toggle_slowmo_on_and_off = spawnArgs.GetBool("phenomenon_should_toggle_slowmo_on_and_off", "0"); // for ship phenomenon AI
		phenomenon_should_ignore_the_rest_of_the_ship_ai_loop = spawnArgs.GetBool("phenomenon_should_ignore_the_rest_of_the_ship_ai_loop", "0"); // for ship phenomenon AI
		// phenomenon actions variables end
	// SHIP AI VARIABLES END

	// stargrid random location team and variables
	stargridstartpos_random_team = spawnArgs.GetString("stargridstartpos_random_team", "");
	stargridstartpos_try_to_be_alone = spawnArgs.GetBool("stargridstartpos_try_to_be_alone", "0");
	stargridstartpos_avoid_entities_of_same_class = spawnArgs.GetBool("stargridstartpos_avoid_entities_of_same_class", "0");

	projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_spaceship_torpedo", "projectile_spaceship_torpedo_default"), false );

// boyette - ensure that we don't target ourselves since that could cause an infinite loop when referencing entities and interacting with them.
// boyette - we should probably eventually have a function called CheckForSelfReferencing() that runs whenever entity references are changed to make sure an entity never references itself. It will check all references.
	if ( TestHideShipEntity == this ) {
		gameLocal.Error( "Entity '%s' is referencing itself in it's def", name.c_str() );
	}
// boyette map entites link up end
// this makes the entity think in every frame. gameLocal cycles through all entities with this flag set to !=0 and runs the think() function on them.
	//thinkFlags = TH_UPDATEVISUALS; // TH_UPDATEVISUALS - entities with this flag only think when their updatevisuals function is run.
	//thinkFlags = TH_ANIMATE; // TH_ANIMATE - entities with this flag update their animations every frame.
	//thinkFlags = TH_PHYSICS; // TH_PHYSICS - entities with this flag update their physics every frame.
	thinkFlags = TH_THINK; // TH_THINK - entities with this flag will run their think function every frame but will not run their physics or animations.
	//thinkFlags = TH_ALL; // TH_ALL - entities with this flag will run everything. - This is the most expensive.
	//thinkFlags = 0; // 0 - entities with this flag set to 0 will not think at all - they are unthinking entities - this is the default for idEntity. - This is the least expensive.
	RunPhysics(); // this is called at spawn so it doesn't have to be called every frame in ::Think() now that this is a thinking entity
	Present(); // this is called at spawn so it doesn't have to be called every frame in ::Think() now that this is a thinking entity

	// To Delete
	if ( !gameLocal.isMultiplayer ) {
		idDict args;

		if ( !beamTarget ) {
			args.SetVector( "origin", vec3_origin );
			args.SetBool( "start_off", true );
			beamTarget = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		}

		if ( !beam ) {
			args.Clear();
			args.Set( "target", beamTarget->name.c_str() );
			args.SetVector( "origin", vec3_origin );
			args.SetBool( "start_off", true );
			args.Set( "width", "6" );
			//args.Set( "skin", "textures/smf/flareSizeable" );
			//args.Set( "skin", "models/particles/phaser_burst/phaser_burst_1" );
			//args.Set( "skin", "models/particles/phaser_burst/phaser_burst_ship_to_ship_wild" );
			//args.Set( "skin", "models/particles/phaser_burst/phaser_burst_ship_to_ship_plain" );
			//args.Set( "skin", "models/particles/phaser_burst/phaser_burst_ship_to_ship_variable" );

			//args.Set( "skin", "textures/ship_to_ship_weapons/phaser_burst_ship_to_ship_variable" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/green_generic_phaser_burst" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/red_generic_phaser_burst" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/blue_generic_phaser_burst" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/purple_generic_phaser_burst" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/yellow_generic_phaser_burst" );
			//args.Set( "skin", "textures/ship_to_ship_weapons/orange_generic_phaser_burst" );
			args.Set( "skin", spawnArgs.GetString("ship_to_ship_weapons_material", "textures/ship_to_ship_weapons/generic_phaser_burst") );
			//args.Set( "_color", "0.0235 0.843 0.969 0.2" );
			//args.Set( "_color", spawnArgs.GetString("ship_to_ship_weapons_color_theme", "1.0 1.0 1.0 1.0") ); // BOYETTE NOTE - this doesn't seem to have any effect. Maybe because the skin overrides it. SHould just use different skins.
			//args.Set( "_color", spawnArgs.GetString("ship_to_ship_torpedos_color_theme", "1.0 1.0 1.0 1.0") ); // BOYETTE NOTE - this doesn't seem to have any effect. Maybe because the skin overrides it. SHould just use different skins.
			beam = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
			beam->SetShaderParm( 6, 1.0f );
		}
	}

	beam->SetOrigin( GetPhysics()->GetOrigin() );
	beamTarget->SetOrigin( GetPhysics()->GetOrigin() );

	// weapons and torpedos queues
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		WeaponsTargetQueue[i] = i;
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		TorpedosTargetQueue[i] = i;
	}
	weapons_autofire_on = true;
	torpedos_autofire_on = true;

	// ship dialogue system.
	hailDialogueBranchTracker.set(0,1); // so the first bit will spawn as set to true - all others will be false. Because we always want the dialogue to start with the first branch. When the dialoge is finished, set the last bit to true. In our logic for a hail we will check the last bit. That way hailing will just produce a "no reponse" or something like that screen if they don't want to talk. The ship will have a damage tolerance, if it goes below that amount they will give up and hail you to continue the dialogue(so the last bit will be set to false).
	hail_dialogue_gui_file = spawnArgs.GetString("hail_dialogue_gui_file","guis/steve_space_command/hail_guis/default_hail_no_response.gui");
	currently_in_hail = false;

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		ModulesPowerQueue[i] = i;
	}
	modules_power_automanage_on = false;
	current_automanage_power_reserve = maximum_power_reserve;
	AutoManageModulePowerlevels();

	discovered_by_player = spawnArgs.GetBool("initially_discovered_by_player","0");

	has_artifact_aboard = spawnArgs.GetBool("has_artifact_aboard","0");

	// oxygen system
	current_oxygen_level = 100;
	infinite_oxygen = spawnArgs.GetBool("infinite_oxygen","0");

	// red alert light system
	red_alert = false;

	// ship doors
	old_security_module_efficiency = 0; // the beginning charge of the security module is 0 so when the map loads all the doors won't be set to zero health since the this number is not different than the current module efficiency. But from here on out, whenever the module_efficiency changes the doors health's will be updated.

	// minimum required shields percentage to prevent foreign transporters from beaming aboard your ship
	min_shields_percent_for_blocking_foreign_transporters = spawnArgs.GetFloat("min_shields_percent_for_blocking_foreign_transporters","0.57"); // was 0.5 for a long time

	// These are teams that this ship (and should make sure other ships on the same team) team has a non-hostility agreement with.
	neutral_teams = SplitStringToInts( spawnArgs.GetString( "neutral_teams",""), ',' );
	/*
	gameLocal.Printf( "The size of the neutral teams of the " + name + " is" + idStr(neutral_teams.size()) + "\n");
	gameLocal.Printf( "The neutral teams of the " + name + " are:\n");
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		gameLocal.Printf( idStr(neutral_teams[i]) + "\n");
	}
	*/

	// we are assuming ships don't start off as derelict
	is_derelict = false;
	// for planets and such
	never_derelict = spawnArgs.GetBool( "never_derelict", "0" );

	do_red_alert_when_derelict = spawnArgs.GetBool( "do_red_alert_when_derelict", "0" );

	// ship destruction
	ship_destruction_sequence_initiated = false;


	// ship self destruct sequence
	ship_self_destruct_sequence_initiated = false;
	ship_self_destruct_sequence_timer = 0;


	//original name to deal with derelict strings
	original_name = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber ) );

	// scrap and interstellar credits
	current_materials_reserves = 0;
	current_currency_reserves = 0;

	// to enable deconstruction effects
	ship_deconstruction_sequence_initiated = false;
	// only asteroids and other salvageable things can be deconstructed
	can_be_deconstructed = spawnArgs.GetBool( "can_be_deconstructed", "0" );
	ship_beam_active = false;
	ShipBeam.target = NULL;

	// to make sure these sounds are cached for quick retrieval the first time
	declManager->FindSound( spawnArgs.GetString( "snd_weapons_launch", "spaceship_weapons_launch_snd_default" ) );
	declManager->FindSound( spawnArgs.GetString( "snd_weapons_launch_in_space", "spaceship_weapons_launch_in_space_snd_default" ) );

	declManager->FindSound( spawnArgs.GetString( "snd_torpedos_launch", "spaceship_torpedos_launch_snd_default") );
	declManager->FindSound( spawnArgs.GetString( "snd_torpedos_launch_in_space", "spaceship_torpedos_launch_in_space_snd_default") );
	
	declManager->FindSound( "spaceship_weapons_impact_light" );
	declManager->FindSound( "spaceship_weapons_impact_medium" );
	declManager->FindSound( "spaceship_weapons_impact_heavy" );
	declManager->FindSound( "spaceship_weapons_impact_critical" );

	declManager->FindSound( "spaceship_torpedos_impact_light" );
	declManager->FindSound( "spaceship_torpedos_impact_medium" );
	declManager->FindSound( "spaceship_torpedos_impact_heavy" );
	declManager->FindSound( "spaceship_torpedos_impact_critical" );

	declManager->FindSound( "spaceship_weapons_impact_in_space" );

	declManager->FindSound( "spaceship_torpedos_impact_in_space" );

	declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress", "destruction_in_progress_snd_default") );
	declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress_in_space", "destruction_in_progress_in_space_snd_default") );

	declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion", "destruction_conclusion_snd_default") );
	declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion_in_space", "destruction_conclusion_in_space_snd_default") );
}

// Utility function for converting delimited strings into vectors of ints.
std::vector<int> sbShip::SplitStringToInts(const std::string &s, char delim) {
    std::vector<int> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(std::stoi(item));
    }
    return elems;
}

void sbShip::EndNeutralityWithTeam( int team_variable ) {
	// BOYETTE NOTE TODO MAYBE: every other ship/planet/station with the same team as this one should have the team_variable removed from its neutral teams vector. - this could cause problems with random ships attacking each other though so maybe it is fine how it is. - we just need to update the attacking neutral team warning to just mean this ship.
	bool neutrality_ended = false;
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		if ( team_variable == neutral_teams[i] ) { 
			neutral_teams.erase(neutral_teams.begin() + i);
			neutrality_ended = true;
		}
	}
	if ( neutrality_ended ) {
		SyncUpOurBrokenNeutralityWithThisTeam(team_variable);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == team_variable ) {
			friendlinessWithPlayer = 0;
		}
	}
	if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony && neutrality_ended /*&& ( gameLocal.GetLocalPlayer()->PlayerShip == this || gameLocal.GetLocalPlayer()->PlayerShip->HasNeutralityWithTeam(team_variable) )*/ ) {
		gameLocal.GetLocalPlayer()->PopulateShipList(); // needed to update the colors of the ship name text in the shiplist.
	}
}
void sbShip::EndNeutralityWithShip( sbShip* ship_to_not_be_neutral ) {
	if ( ship_to_not_be_neutral ) {
		EndNeutralityWithTeam(ship_to_not_be_neutral->team);
		ship_to_not_be_neutral->EndNeutralityWithTeam(team);
	}
}

void sbShip::StartNeutralityWithTeam( int team_variable ) {
	bool neutrality_started = true;
	// BOYETTE NOTE TODO MAYBE: every other ship/planet/station with the same team as this one should have the team_variableadded to its neutral teams vector. - this could cause problems with random ships forgiving each other though so maybe it is fine how it is. - we just need to update the attacking neutral team warning to just mean this ship.
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		if ( team_variable == neutral_teams[i] ) { 
			neutrality_started = false; // they already have neutrality
		}
	}
	if ( neutrality_started ) {
		neutral_teams.push_back(team_variable);
		SyncUpOurNeutralityWithThisTeam(team_variable);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == team_variable ) {
			friendlinessWithPlayer = idMath::ClampInt(1,10,friendlinessWithPlayer);
		}
	}
	if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony && ( gameLocal.GetLocalPlayer()->PlayerShip == this || gameLocal.GetLocalPlayer()->PlayerShip->HasNeutralityWithTeam(team_variable) ) ) {
		gameLocal.GetLocalPlayer()->PopulateShipList(); // needed to update the colors of the ship name text in the shiplist.
	}
}
void sbShip::StartNeutralityWithShip( sbShip* ship_to_be_neutral ) {
	if ( ship_to_be_neutral ) {
		StartNeutralityWithTeam(ship_to_be_neutral->team);
		ship_to_be_neutral->StartNeutralityWithTeam(team);
	}
}

bool sbShip::HasNeutralityWithTeam( int team_variable ) {
	if ( team_variable == ALWAYS_NEUTRAL_TEAM || team == ALWAYS_NEUTRAL_TEAM ) {
		return true;
	}
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		if ( team_variable == neutral_teams[i] ) { 
			return true;
		}
	}
	return false;
}

bool sbShip::HasNeutralityWithShip( sbShip* ship_to_test ) {
	if ( ship_to_test->team == ALWAYS_NEUTRAL_TEAM || team == ALWAYS_NEUTRAL_TEAM ) {
		return true;
	} else if ( HasNeutralityWithTeam( ship_to_test->team ) && ship_to_test->HasNeutralityWithTeam( team ) ) {
		return true;
	} else {
		return false;
	}
}

bool sbShip::HasNeutralityWithAI( idAI* ai_to_test ) {
	if ( ai_to_test->team == ALWAYS_NEUTRAL_TEAM || team == ALWAYS_NEUTRAL_TEAM ) {
		return true;
	} else if ( ai_to_test && HasNeutralityWithTeam( ai_to_test->team ) && ( (ai_to_test->ParentShip && ai_to_test->ParentShip->HasNeutralityWithTeam( team )) || !ai_to_test->ParentShip ) ) {
		return true;
	} else {
		return false;
	}
}

void sbShip::BecomeASpacePirateShip() {
	// get rid of all neutrality agreements
	neutral_teams.clear();

	// BOYETTE NOTE TODO: might want to make sure that the unique team is not in enyone's neutral_teams vector. It shouldn't be, but it could be. - not really necessary to do this though.
	int unique_team;
	bool found_unique_team = false;
	// check to make sure no other ships use this team number
	while ( found_unique_team == false ) {
		unique_team = gameLocal.random.RandomInt( idRandom::MAX_RAND );
		found_unique_team = true;
		for ( int i = 0; i < gameLocal.num_entities; i++ ) {
			if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {
				if ( gameLocal.entities[ i ] && dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->is_derelict ) {
					neutral_teams.push_back( gameLocal.entities[ i ]->team ); // derelict space entities should always be neutral.
				} else {
					if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
						static_cast<sbShip*>( gameLocal.entities[ i ] )->friendlinessWithPlayer = 0;
					}
				}
				if ( gameLocal.entities[ i ]->team == unique_team && gameLocal.entities[ i ] != this ) {
					found_unique_team = false;
					break;
				}
			}
		}
		if ( found_unique_team ) {
			break;
		}
	}

	// if this is the playership - we also need to make sure that the player's team is changed.
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->team = unique_team; gameLocal.GetLocalPlayer()->spawnArgs.SetInt("team",unique_team);
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1You and your crew have been ^1branded space pirates.^0" );
	}

	ChangeTeam(unique_team);
}

void sbShip::RedeemFromBeingASpacePirateShip( sbShip* redeemer_ship ) {
	int redeemer_team = redeemer_ship->team;

	// inherit the neutrality agreements of the redeemer ship.
	neutral_teams = redeemer_ship->neutral_teams;

	// if this is the playership - we also need to make sure that the player's team is changed.
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->team = redeemer_team; gameLocal.GetLocalPlayer()->spawnArgs.SetInt("team",redeemer_team);
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1You and your crew have been ^1redeemed from being space pirates.^0" );
	}

	ChangeTeam(redeemer_team);
}

void sbShip::ChangeTeam(int new_team) {
	// the ship itself
	team = new_team; spawnArgs.SetInt("team",new_team);

	// the officers
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] ) {
			crew[i]->team = new_team; crew[i]->spawnArgs.SetInt("team",new_team);
		}
	}

	// the consoles
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] ) {
			consoles[i]->team = new_team; consoles[i]->spawnArgs.SetInt("team",new_team);
			// the console's modules
			if ( consoles[i]->ControlledModule ) {
				consoles[i]->ControlledModule->team = new_team; consoles[i]->ControlledModule->spawnArgs.SetInt("team",new_team);
			}
		}
	}

	// the ship's doors
	for( int i = 0; i < shipdoors.Num(); i++ ) {
		if ( shipdoors[ i ].GetEntity() ) {
			dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SetDoorGroupsidEntityTeam(new_team);
		}
	}

	// the captain chair - this might not be necessary - it shouldn't effect anything. - maybe whether or not the Captain can sit in it.
	if ( CaptainChair ) CaptainChair->team = new_team; if ( CaptainChair ) CaptainChair->spawnArgs.SetInt("team",new_team);

	// the ready room captain chair - this might not be necessary - it shouldn't effect anything. - maybe whether or not the Captain can sit in it.
	if ( ReadyRoomCaptainChair ) ReadyRoomCaptainChair->team = new_team; if ( ReadyRoomCaptainChair ) ReadyRoomCaptainChair->spawnArgs.SetInt("team",new_team);

	if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony && ( gameLocal.GetLocalPlayer()->PlayerShip == this || gameLocal.GetLocalPlayer()->PlayerShip->HasNeutralityWithTeam(new_team) ) ) {
		gameLocal.GetLocalPlayer()->PopulateShipList(); // needed to update the colors of the ship name text in the shiplist.
	}
}

void sbShip::SyncUpOurNeutralityWithThisTeam( int new_neutral_team ) {
	for ( int i = 0; i < gameLocal.num_entities; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {

			sbShip* ShipToTeamSync = static_cast<sbShip*>( gameLocal.entities[ i ] );

			 // make sure all our old enemies know about our neutrality
			if ( ShipToTeamSync->team == new_neutral_team ) {
				for ( int x = 0; x < ShipToTeamSync->neutral_teams.size(); x++ ) {
					if ( team == ShipToTeamSync->neutral_teams[x] ) {
						if ( !ShipToTeamSync->HasNeutralityWithTeam(team) ) {
							ShipToTeamSync->neutral_teams.push_back(team);

							if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == team ) {
								ShipToTeamSync->friendlinessWithPlayer = idMath::ClampInt(1,10,ShipToTeamSync->friendlinessWithPlayer);
							}
						}
					}
				}
			}

			// make sure all our allies know about our neutrality
			if ( ShipToTeamSync->team == team ) {
				for ( int x = 0; x < ShipToTeamSync->neutral_teams.size(); x++ ) {
					if ( new_neutral_team == ShipToTeamSync->neutral_teams[x] ) { // remove the specified team from our allies neutral teams
						if ( !ShipToTeamSync->HasNeutralityWithTeam(new_neutral_team) ) {
							ShipToTeamSync->neutral_teams.push_back(new_neutral_team);

							if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == new_neutral_team ) {
								ShipToTeamSync->friendlinessWithPlayer = idMath::ClampInt(1,10,ShipToTeamSync->friendlinessWithPlayer);
							}
						}
					}
				}
			}

		}
	}
}
void sbShip::SyncUpOurBrokenNeutralityWithThisTeam( int new_enemy_team ) {
	bool this_is_the_playership = ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this );

	for ( int i = 0; i < gameLocal.num_entities; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {

			sbShip* ShipToTeamSync = static_cast<sbShip*>( gameLocal.entities[ i ] );

			 // make sure all our new enemies know our neutrality is broken
			if ( ShipToTeamSync->team == new_enemy_team ) {
				for ( int x = 0; x < ShipToTeamSync->neutral_teams.size(); x++ ) {
					if ( team == ShipToTeamSync->neutral_teams[x] ) {
						ShipToTeamSync->neutral_teams.erase(ShipToTeamSync->neutral_teams.begin() + x);

						if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == team ) {
							ShipToTeamSync->friendlinessWithPlayer = 0;
						}
					}
				}
			}

			// make sure all our allies know our neutrality is broken
			if ( ShipToTeamSync->team == team ) {
				for ( int x = 0; x < ShipToTeamSync->neutral_teams.size(); x++ ) {
					if ( new_enemy_team == ShipToTeamSync->neutral_teams[x] ) { // remove the specified team from our allies neutral teams
						ShipToTeamSync->neutral_teams.erase(ShipToTeamSync->neutral_teams.begin() + x);

						if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->team == new_enemy_team ) {
							ShipToTeamSync->friendlinessWithPlayer = 0;
						}
					}
				}
			}

		}
	}
}

void sbShip::Save( idSaveGame *savefile ) const {
	// BOYETTE SAVE BEGIN
	int num = 0;

	savefile->WriteObject( TargetEntityInSpace );

	savefile->WriteObject( MySkyPortalEnt );
	savefile->WriteBool( alway_snap_to_my_sky_portal_entity );

	savefile->WriteObject( TestHideShipEntity );

	savefile->WriteObject( ShipDiagramDisplayNode );
	savefile->WriteDict( &spawnArgs_adjusted_ShipDiagramDisplayNode );

	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		savefile->WriteObject( crew[i] );
	}
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_crew[i] );
	}

	savefile->WriteInt( reserve_Crew.size() );
	for ( int i = 0; i < reserve_Crew.size(); i++ ) {
		savefile->WriteDict( &reserve_Crew[i] );
	}
	savefile->WriteInt( max_reserve_crew );

	savefile->WriteInt( AIsOnBoard.size() );
	for ( int i = 0; i < AIsOnBoard.size(); i++ ) {
		savefile->WriteObject( AIsOnBoard[i] );
	}
	savefile->WriteInt( spawnArgs_adjusted_AIsOnBoard.size() );
	for ( int i = 0; i < spawnArgs_adjusted_AIsOnBoard.size(); i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_AIsOnBoard[i] );
	}

	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		savefile->WriteObject( room_node[i] );
	}
	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_room_node[i] );
	}

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteObject( consoles[i] );
	}
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_consoles[i] );
	}
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_consoles_ControlledModule[i] );
	}

	savefile->WriteObject( CaptainChair );
	savefile->WriteDict( &spawnArgs_adjusted_CaptainChair );

	savefile->WriteObject( ReadyRoomCaptainChair );
	savefile->WriteDict( &spawnArgs_adjusted_ReadyRoomCaptainChair );

	savefile->WriteObject( ShieldEntity );
	savefile->WriteDict( &spawnArgs_adjusted_ShieldEntity );

	savefile->WriteObject( TransporterBounds );
	savefile->WriteDict( &spawnArgs_adjusted_TransporterBounds );
	savefile->WriteObject( TransporterPad );
	savefile->WriteDict( &spawnArgs_adjusted_TransporterPad );
	savefile->WriteObject( TransporterParticleEntitySpawnMarker );
	savefile->WriteDict( &spawnArgs_adjusted_TransporterParticleEntitySpawnMarker );

	savefile->WriteObject( ViewScreenEntity );
	savefile->WriteDict( &spawnArgs_adjusted_ViewScreenEntity );

	savefile->WriteInt( shipdoors.Num() );
	for( int i = 0; i < shipdoors.Num(); i++ ) {
		shipdoors[ i ].Save( savefile );
	}
	savefile->WriteInt( spawnArgs_adjusted_shipdoors.size() );
	for ( int i = 0; i < spawnArgs_adjusted_shipdoors.size(); i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_shipdoors[i] );
	}
	savefile->WriteInt( spawnArgs_adjusted_shipdoors_partners.size() );
	for ( int i = 0; i < spawnArgs_adjusted_shipdoors_partners.size(); i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_shipdoors_partners[i] );
	}

	savefile->WriteInt( shiplights.Num() );
	for( int i = 0; i < shiplights.Num(); i++ ) {
		shiplights[ i ].Save( savefile );
	}
	savefile->WriteInt( spawnArgs_adjusted_shiplights.size() );
	for ( int i = 0; i < spawnArgs_adjusted_shiplights.size(); i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_shiplights[i] );
	}
	savefile->WriteInt( spawnArgs_adjusted_shiplights_targets.size() );
	for ( int i = 0; i < spawnArgs_adjusted_shiplights_targets.size(); i++ ) {
		savefile->WriteDict( &spawnArgs_adjusted_shiplights_targets[i] );
	}

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteInt( module_max_powers[i] );
	}

	idStr TransporterParticleEntityDefName;
	spawnArgs.GetString("transporter_pad_particle_def","module_transporter_circular_pad_particle_default", TransporterParticleEntityDefName );
	savefile->WriteString( TransporterParticleEntityDefName );

	TransporterParticleEntityFX.Save( savefile );

	savefile->WriteFloat( min_shields_percent_for_blocking_foreign_transporters );

	savefile->WriteInt( verified_warp_stargrid_postion_x );
	savefile->WriteInt( verified_warp_stargrid_postion_y );

	savefile->WriteBool( story_window_satisfied );

	savefile->WriteBool( should_warp_in_when_first_encountered );

	savefile->WriteObject( SelectedCrewMember );

	savefile->WriteInt( SelectedCrewMembers.size() );
	for ( int i = 0; i < SelectedCrewMembers.size(); i++ ) {
		savefile->WriteObject( SelectedCrewMembers[i] );
	}

	savefile->WriteObject( SelectedModule );

	savefile->WriteInt( maximum_power_reserve );
	savefile->WriteInt( current_power_reserve );

	savefile->WriteObject( beam );
	savefile->WriteObject( beamTarget );
	savefile->WriteBool( weapons_shot_missed );

	idStr projectileDefName;
	spawnArgs.GetString("def_spaceship_torpedo", "projectile_spaceship_torpedo_default", projectileDefName );
	savefile->WriteString( projectileDefName );
	projectile.Save( savefile );
	savefile->WriteBool( torpedo_shot_missed );

	for ( int i = 0; i < 9; i++ ) {
		projectilearray[i].Save( savefile );
	}

	idStr damageFXDefName;
	damageFXDefName = "module_disabled_particle";
	savefile->WriteString( damageFXDefName );
	damageFX.Save( savefile );

	savefile->WriteBool( set_as_playership_on_player_spawn );

	savefile->WriteString( ShipStargridIcon );
	savefile->WriteString( ShipImageVisual );

	savefile->WriteString( ShipStargridArtifactIcon );

	savefile->WriteBool( track_on_stargrid );

	savefile->WriteInt( shieldStrength );
	savefile->WriteInt( max_shieldStrength );
	savefile->WriteInt( hullStrength );
	savefile->WriteInt( max_hullStrength );

	savefile->WriteInt( weapons_damage_modifier );
	savefile->WriteInt( torpedos_damage_modifier );

	savefile->WriteInt( shields_repair_per_cycle );

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteInt( WeaponsTargetQueue[i] );
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteInt( TorpedosTargetQueue[i] );
	}

	savefile->WriteBool( weapons_autofire_on );
	savefile->WriteBool( torpedos_autofire_on );

	savefile->WriteBool( ship_is_firing_weapons );
	savefile->WriteBool( ship_is_firing_torpedo );

	savefile->WriteObject( TempTargetEntityInSpace );

	for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
		savefile->WriteBool( hailDialogueBranchTracker.test(i) );
	}
	savefile->WriteBool( currently_in_hail );

	savefile->WriteString( hail_dialogue_gui_file );

	savefile->WriteInt( friendlinessWithPlayer );
	savefile->WriteBool( has_forgiven_player );
	savefile->WriteBool( has_forgiven_player );

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->WriteInt( ModulesPowerQueue[i] );
	}
	savefile->WriteBool( modules_power_automanage_on );

	savefile->WriteInt( current_automanage_power_reserve );

	savefile->WriteInt( update_module_charges_timer );

	savefile->WriteBool( discovered_by_player );

	savefile->WriteBool( has_artifact_aboard );

	savefile->WriteInt( current_oxygen_level );
	savefile->WriteInt( evaluate_current_oxygen_timer );
	savefile->WriteBool( infinite_oxygen );

	savefile->WriteBool( player_ceased_firing_on_this_ship );

	savefile->WriteBool( red_alert );

	savefile->WriteInt( old_security_module_efficiency );

	savefile->WriteInt( neutral_teams.size() );
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		savefile->WriteInt( neutral_teams[i] );
	}

	savefile->WriteBool( is_derelict );
	savefile->WriteBool( never_derelict );

	savefile->WriteBool( do_red_alert_when_derelict );

	savefile->WriteBool( ship_destruction_sequence_initiated );

	savefile->WriteBool( ship_self_destruct_sequence_initiated );
	savefile->WriteInt( ship_self_destruct_sequence_timer );
	savefile->WriteString( original_name );

	savefile->WriteInt( current_materials_reserves );
	savefile->WriteInt( current_currency_reserves );

	savefile->WriteBool( in_repair_mode );

	savefile->WriteBool( ship_deconstruction_sequence_initiated );
	savefile->WriteBool( can_be_deconstructed );

	if ( ShipBeam.target.GetEntity() && ShipBeam.modelDefHandle >= 0 ) {
		savefile->WriteBool( true );
		ShipBeam.target.Save( savefile );
		savefile->WriteRenderEntity( ShipBeam.renderEntity );
	} else {
		savefile->WriteBool( false );
	}
	savefile->WriteBool( ship_beam_active );

	savefile->WriteObject( ship_that_just_fired_at_us );

	savefile->WriteBool( always_neutral );

	savefile->WriteVec3( fx_color_theme );

	savefile->WriteBool( was_sensor_scanned );

	savefile->WriteBool( shields_raised );
	savefile->WriteInt( shieldStrength_copy );

	savefile->WriteBool( ship_lights_on );

	savefile->WriteVec3( suitable_torpedo_launchpoint_offset );

	savefile->WriteInt( ships_at_my_stargrid_position.size() );
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		savefile->WriteObject( ships_at_my_stargrid_position[i] );
	}
	savefile->WriteString( stargridstartpos_random_team );
	savefile->WriteBool( stargridstartpos_try_to_be_alone );
	savefile->WriteBool( stargridstartpos_avoid_entities_of_same_class );

	savefile->WriteInt( main_goal.goal_action );
	savefile->WriteObject( main_goal.goal_entity );
	savefile->WriteInt( mini_goals.size() );
	for ( int i = 0; i < mini_goals.size(); i++ ) {
		savefile->WriteInt( mini_goals[i].goal_action );
		savefile->WriteObject( mini_goals[i].goal_entity );
	}
	savefile->WriteInt( current_goal_iterator );

	savefile->WriteBool( we_are_a_protector );

	savefile->WriteBool( ai_always_targets_random_module );

	savefile->WriteInt( ship_ai_aggressiveness );

	savefile->WriteInt( max_spare_crew_size );
	savefile->WriteInt( max_modules_to_take_spare_crew_from );

	savefile->WriteBool( ignore_boarding_problems );
	savefile->WriteFloat( min_hullstrength_percent_required_for_boarding );
	savefile->WriteInt( min_environment_module_efficiency_required_for_boarding );
	savefile->WriteFloat( min_oxygen_percent_required_for_boarding );

	savefile->WriteBool( boarders_should_target_player );
	savefile->WriteBool( boarders_should_target_random_module );
	savefile->WriteInt( boarders_should_target_module_id );

	savefile->WriteBool( in_no_action_hail_mode );
	savefile->WriteBool( should_hail_the_playership );

	savefile->WriteFloat( wait_to_hail_order_num );

	savefile->WriteBool( should_go_into_no_action_hail_mode_on_hail );

	savefile->WriteBool( battlestations );
	savefile->WriteBool( is_attempting_warp );

	savefile->WriteInt( successful_flee_distance );

	savefile->WriteObject( priority_space_entity_to_target );
	savefile->WriteBool( prioritize_playership_as_space_entity_to_target );

	savefile->WriteObject( priority_space_entity_to_protect );
	savefile->WriteBool( prioritize_playership_as_space_entity_to_protect );

	savefile->WriteBool( ship_begin_dormant );
	savefile->WriteBool( ship_is_never_dormant );
	savefile->WriteBool( ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos );
	savefile->WriteBool( ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos );
	savefile->WriteBool( ship_is_dormant_until_awoken_by_player_shiponboard );
	savefile->WriteBool( ship_is_dormant_until_awoken_by_an_active_ship );

	savefile->WriteBool( ship_modules_must_be_repaired_to_go_dormant );

	savefile->WriteBool( try_to_be_dormant );

	savefile->WriteFloat( flee_hullstrength_percentage );

	savefile->WriteBool( hail_conditionals_met );
	savefile->WriteFloat( hail_conditional_hull_below_this_percentage );
	savefile->WriteBool( hail_conditional_no_hostiles_at_my_stargrid_position );
	savefile->WriteBool( hail_conditional_hostiles_at_my_stargrid_position );
	savefile->WriteBool( hail_conditional_no_friendlies_at_my_stargrid_position );
	savefile->WriteBool( hail_conditional_friendlies_at_my_stargrid_position );
	savefile->WriteBool( hail_conditional_captain_officer_killed );
	savefile->WriteBool( hail_conditional_player_is_aboard_playership );
	savefile->WriteBool( hail_conditional_not_at_player_shiponboard_position );
	savefile->WriteBool( hail_conditional_is_playership_target );

	savefile->WriteBool( phenomenon_show_damage_or_disable_beam );
	savefile->WriteBool( phenomenon_should_do_ship_damage );
	savefile->WriteBool( phenomenon_should_damage_modules );
	savefile->WriteInt( phenomenon_module_damage_amount );

	savefile->WriteInt( phenomenon_module_ids_to_damage.size() );
	for ( int i = 0; i < phenomenon_module_ids_to_damage.size(); i++ ) {
		savefile->WriteInt( phenomenon_module_ids_to_damage[i] );
	}
	savefile->WriteBool( phenomenon_should_damage_random_module );
	savefile->WriteBool( phenomenon_should_disable_modules );

	savefile->WriteInt( phenomenon_module_ids_to_disable.size() );
	for ( int i = 0; i < phenomenon_module_ids_to_disable.size(); i++ ) {
		savefile->WriteInt( phenomenon_module_ids_to_disable[i] );
	}

	savefile->WriteBool( phenomenon_should_disable_random_module );
	savefile->WriteBool( phenomenon_should_set_oxygen_level );
	savefile->WriteInt( phenomenon_oxygen_level_to_set );
	savefile->WriteBool( phenomenon_should_set_ship_shields_to_zero );
	savefile->WriteBool( phenomenon_should_spawn_entity_def_on_playership );
	savefile->WriteInt( phenomenon_number_of_entity_defs_to_spawn );
	savefile->WriteString( phenomenon_entity_def_to_spawn );

	savefile->WriteBool( phenomenon_should_change_random_playership_crewmember_team );
	savefile->WriteBool( phenomenon_should_make_everything_go_slowmo );
	savefile->WriteBool( phenomenon_should_toggle_slowmo_on_and_off );
	savefile->WriteBool( phenomenon_should_ignore_the_rest_of_the_ship_ai_loop );

	savefile->WriteBool( play_low_oxygen_alert_sound );
	savefile->WriteBool( show_low_oxygen_alert_display );

	savefile->WriteInt( regenAmount );
	// BOYETTE SAVE END
}

void sbShip::Restore( idRestoreGame *savefile ) {
	// BOYETTE RESTORE BEGIN
	int num = 0;
	bool hasObject;

	savefile->ReadObject( reinterpret_cast<idClass *&>( TargetEntityInSpace ) );

	savefile->ReadObject( reinterpret_cast<idClass *&>( MySkyPortalEnt ) );
	savefile->ReadBool( alway_snap_to_my_sky_portal_entity );

	savefile->ReadObject( reinterpret_cast<idClass *&>( TestHideShipEntity ) );


	savefile->ReadObject( reinterpret_cast<idClass *&>( ShipDiagramDisplayNode ) );
	savefile->ReadDict( &spawnArgs_adjusted_ShipDiagramDisplayNode );

	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( crew[i] ) );
	}
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_crew[i] );
	}

	savefile->ReadInt( num );
	reserve_Crew.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &reserve_Crew[i] );
	}
	savefile->ReadInt( max_reserve_crew );

	savefile->ReadInt( num );
	AIsOnBoard.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( AIsOnBoard[i] ) );
	}
	savefile->ReadInt( num );
	spawnArgs_adjusted_AIsOnBoard.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_AIsOnBoard[i] );
	}

	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( room_node[i] ) );
	}
	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_room_node[i] );
	}

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( consoles[i] ) );
	}
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_consoles[i] );
	}
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_consoles_ControlledModule[i] );
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( CaptainChair ) );
	savefile->ReadDict( &spawnArgs_adjusted_CaptainChair );

	savefile->ReadObject( reinterpret_cast<idClass *&>( ReadyRoomCaptainChair ) );
	savefile->ReadDict( &spawnArgs_adjusted_ReadyRoomCaptainChair );

	savefile->ReadObject( reinterpret_cast<idClass *&>( ShieldEntity ) );
	savefile->ReadDict( &spawnArgs_adjusted_ShieldEntity );

	savefile->ReadObject( reinterpret_cast<idClass *&>( TransporterBounds ) );
	savefile->ReadDict( &spawnArgs_adjusted_TransporterBounds );
	savefile->ReadObject( reinterpret_cast<idClass *&>( TransporterPad ) );
	savefile->ReadDict( &spawnArgs_adjusted_TransporterPad );
	savefile->ReadObject( reinterpret_cast<idClass *&>( TransporterParticleEntitySpawnMarker ) );
	savefile->ReadDict( &spawnArgs_adjusted_TransporterParticleEntitySpawnMarker );

	savefile->ReadObject( reinterpret_cast<idClass *&>( ViewScreenEntity ) );
	savefile->ReadDict( &spawnArgs_adjusted_ViewScreenEntity );

	shipdoors.Clear();
	savefile->ReadInt( num );
	shipdoors.SetNum( num );
	for( int i = 0; i < num; i++ ) {
		shipdoors[ i ].Restore( savefile );
	}
	savefile->ReadInt( num );
	spawnArgs_adjusted_shipdoors.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_shipdoors[i] );
	}
	savefile->ReadInt( num );
	spawnArgs_adjusted_shipdoors_partners.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_shipdoors_partners[i] );
	}

	shiplights.Clear();
	savefile->ReadInt( num );
	shiplights.SetNum( num );
	for( int i = 0; i < num; i++ ) {
		shiplights[ i ].Restore( savefile );
	}
	savefile->ReadInt( num );
	spawnArgs_adjusted_shiplights.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_shiplights[i] );
	}
	savefile->ReadInt( num );
	spawnArgs_adjusted_shiplights_targets.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadDict( &spawnArgs_adjusted_shiplights_targets[i] );
	}

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadInt( module_max_powers[i] );
	}

	idStr TransporterParticleEntityDefName;
	savefile->ReadString( TransporterParticleEntityDefName );
	if ( TransporterParticleEntityDefName.Length() ) {
		TransporterParticleEntityDef = gameLocal.FindEntityDefDict( TransporterParticleEntityDefName );
	} else {
		TransporterParticleEntityDef = NULL;
	}

	TransporterParticleEntityFX.Restore( savefile );

	savefile->ReadFloat( min_shields_percent_for_blocking_foreign_transporters );

	savefile->ReadInt( verified_warp_stargrid_postion_x );
	savefile->ReadInt( verified_warp_stargrid_postion_y );

	savefile->ReadBool( story_window_satisfied );

	savefile->ReadBool( should_warp_in_when_first_encountered );

	savefile->ReadObject( reinterpret_cast<idClass *&>( SelectedCrewMember ) );

	savefile->ReadInt( num );
	SelectedCrewMembers.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( SelectedCrewMembers[i] ) );
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( SelectedModule ) );

	savefile->ReadInt( maximum_power_reserve );
	savefile->ReadInt( current_power_reserve );

	savefile->ReadObject( reinterpret_cast<idClass *&>( beam ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( beamTarget ) );
	savefile->ReadBool( weapons_shot_missed );

	idStr projectileDefName;
	savefile->ReadString( projectileDefName );
	if ( projectileDefName.Length() ) {
		projectileDef = gameLocal.FindEntityDefDict( projectileDefName );
	} else {
		projectileDef = NULL;
	}
	projectile.Restore( savefile );
	savefile->ReadBool( torpedo_shot_missed );

	for ( int i = 0; i < 9; i++ ) {
		projectilearray[i].Restore( savefile );
	}

	idStr damageFXDefName;
	savefile->ReadString( damageFXDefName );
	if ( damageFXDefName.Length() ) {
		damageFXDef = gameLocal.FindEntityDefDict( damageFXDefName );
	} else {
		damageFXDef = NULL;
	}
	damageFX.Restore( savefile );

	savefile->ReadBool( set_as_playership_on_player_spawn );

	savefile->ReadString( ShipStargridIcon );
	savefile->ReadString( ShipImageVisual );
	
	savefile->ReadString( ShipStargridArtifactIcon );

	savefile->ReadBool( track_on_stargrid );

	savefile->ReadInt( shieldStrength );
	savefile->ReadInt( max_shieldStrength );
	savefile->ReadInt( hullStrength );
	savefile->ReadInt( max_hullStrength );

	savefile->ReadInt( weapons_damage_modifier );
	savefile->ReadInt( torpedos_damage_modifier );

	savefile->ReadInt( shields_repair_per_cycle );

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadInt( WeaponsTargetQueue[i] );
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadInt( TorpedosTargetQueue[i] );
	}

	savefile->ReadBool( weapons_autofire_on );
	savefile->ReadBool( torpedos_autofire_on );

	savefile->ReadBool( ship_is_firing_weapons );
	savefile->ReadBool( ship_is_firing_torpedo );

	savefile->ReadObject( reinterpret_cast<idClass *&>( TempTargetEntityInSpace ) );

	for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
		bool bitset_value = false;
		savefile->ReadBool( bitset_value );
		hailDialogueBranchTracker.set(i,bitset_value);
	}
	savefile->ReadBool( currently_in_hail );

	savefile->ReadString( hail_dialogue_gui_file );

	savefile->ReadInt( friendlinessWithPlayer );
	savefile->ReadBool( has_forgiven_player );
	savefile->ReadBool( has_forgiven_player );

	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		savefile->ReadInt( ModulesPowerQueue[i] );
	}
	savefile->ReadBool( modules_power_automanage_on );

	savefile->ReadInt( current_automanage_power_reserve );

	savefile->ReadInt( update_module_charges_timer );

	savefile->ReadBool( discovered_by_player );

	savefile->ReadBool( has_artifact_aboard );

	savefile->ReadInt( current_oxygen_level );
	savefile->ReadInt( evaluate_current_oxygen_timer );
	savefile->ReadBool( infinite_oxygen );

	savefile->ReadBool( player_ceased_firing_on_this_ship );

	savefile->ReadBool( red_alert );

	savefile->ReadInt( old_security_module_efficiency );

	savefile->ReadInt( num );
	neutral_teams.resize(num);
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		savefile->ReadInt( neutral_teams[i] );
	}

	savefile->ReadBool( is_derelict );
	savefile->ReadBool( never_derelict );

	savefile->ReadBool( do_red_alert_when_derelict );

	savefile->ReadBool( ship_destruction_sequence_initiated );

	savefile->ReadBool( ship_self_destruct_sequence_initiated );
	savefile->ReadInt( ship_self_destruct_sequence_timer );
	savefile->ReadString( original_name );

	savefile->ReadInt( current_materials_reserves );
	savefile->ReadInt( current_currency_reserves );

	savefile->ReadBool( in_repair_mode );

	savefile->ReadBool( ship_deconstruction_sequence_initiated );
	savefile->ReadBool( can_be_deconstructed );

	savefile->ReadBool( hasObject );
	if ( hasObject ) {
		ShipBeam.target.Restore( savefile );
		savefile->ReadRenderEntity( ShipBeam.renderEntity );
		ShipBeam.modelDefHandle = gameRenderWorld->AddEntityDef( &ShipBeam.renderEntity );
	} else {
		ShipBeam.target = NULL;
		memset( &ShipBeam.renderEntity, 0, sizeof( renderEntity_t ) );
		ShipBeam.modelDefHandle = -1;
	}
	savefile->ReadBool( ship_beam_active );

	savefile->ReadObject( reinterpret_cast<idClass *&>( ship_that_just_fired_at_us ) );

	savefile->ReadBool( always_neutral );

	savefile->ReadVec3( fx_color_theme );

	savefile->ReadBool( was_sensor_scanned );

	savefile->ReadBool( shields_raised );
	savefile->ReadInt( shieldStrength_copy );

	savefile->ReadBool( ship_lights_on );

	savefile->ReadVec3( suitable_torpedo_launchpoint_offset );

	savefile->ReadInt( num );
	ships_at_my_stargrid_position.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( ships_at_my_stargrid_position[i] ) );
	}
	savefile->ReadString( stargridstartpos_random_team );
	savefile->ReadBool( stargridstartpos_try_to_be_alone );
	savefile->ReadBool( stargridstartpos_avoid_entities_of_same_class );

	savefile->ReadInt( main_goal.goal_action );
	savefile->ReadObject( reinterpret_cast<idClass *&>( main_goal.goal_entity ) );
	savefile->ReadInt( num );
	mini_goals.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadInt( mini_goals[i].goal_action );
		savefile->ReadObject( reinterpret_cast<idClass *&>( mini_goals[i].goal_entity ) );
	}
	savefile->ReadInt( current_goal_iterator );

	savefile->ReadBool( we_are_a_protector );

	savefile->ReadBool( ai_always_targets_random_module );

	savefile->ReadInt( ship_ai_aggressiveness );

	savefile->ReadInt( max_spare_crew_size );
	savefile->ReadInt( max_modules_to_take_spare_crew_from );

	savefile->ReadBool( ignore_boarding_problems );
	savefile->ReadFloat( min_hullstrength_percent_required_for_boarding );
	savefile->ReadInt( min_environment_module_efficiency_required_for_boarding );
	savefile->ReadFloat( min_oxygen_percent_required_for_boarding );

	savefile->ReadBool( boarders_should_target_player );
	savefile->ReadBool( boarders_should_target_random_module );
	savefile->ReadInt( boarders_should_target_module_id );

	savefile->ReadBool( in_no_action_hail_mode );
	savefile->ReadBool( should_hail_the_playership );

	savefile->ReadFloat( wait_to_hail_order_num );

	savefile->ReadBool( should_go_into_no_action_hail_mode_on_hail );

	savefile->ReadBool( battlestations );
	savefile->ReadBool( is_attempting_warp );

	savefile->ReadInt( successful_flee_distance );

	savefile->ReadObject( reinterpret_cast<idClass *&>( priority_space_entity_to_target ) );
	savefile->ReadBool( prioritize_playership_as_space_entity_to_target );

	savefile->ReadObject( reinterpret_cast<idClass *&>( priority_space_entity_to_protect ) );
	savefile->ReadBool( prioritize_playership_as_space_entity_to_protect );

	savefile->ReadBool( ship_begin_dormant );
	savefile->ReadBool( ship_is_never_dormant );
	savefile->ReadBool( ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos );
	savefile->ReadBool( ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos );
	savefile->ReadBool( ship_is_dormant_until_awoken_by_player_shiponboard );
	savefile->ReadBool( ship_is_dormant_until_awoken_by_an_active_ship );

	savefile->ReadBool( ship_modules_must_be_repaired_to_go_dormant );

	savefile->ReadBool( try_to_be_dormant );

	savefile->ReadFloat( flee_hullstrength_percentage );

	savefile->ReadBool( hail_conditionals_met );
	savefile->ReadFloat( hail_conditional_hull_below_this_percentage );
	savefile->ReadBool( hail_conditional_no_hostiles_at_my_stargrid_position );
	savefile->ReadBool( hail_conditional_hostiles_at_my_stargrid_position );
	savefile->ReadBool( hail_conditional_no_friendlies_at_my_stargrid_position );
	savefile->ReadBool( hail_conditional_friendlies_at_my_stargrid_position );
	savefile->ReadBool( hail_conditional_captain_officer_killed );
	savefile->ReadBool( hail_conditional_player_is_aboard_playership );
	savefile->ReadBool( hail_conditional_not_at_player_shiponboard_position );
	savefile->ReadBool( hail_conditional_is_playership_target );

	savefile->ReadBool( phenomenon_show_damage_or_disable_beam );
	savefile->ReadBool( phenomenon_should_do_ship_damage );
	savefile->ReadBool( phenomenon_should_damage_modules );
	savefile->ReadInt( phenomenon_module_damage_amount );

	savefile->ReadInt( num );
	phenomenon_module_ids_to_damage.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadInt( phenomenon_module_ids_to_damage[i] );
	}
	savefile->ReadBool( phenomenon_should_damage_random_module );
	savefile->ReadBool( phenomenon_should_disable_modules );

	savefile->ReadInt( num );
	phenomenon_module_ids_to_disable.resize(num);
	for ( int i = 0; i < num; i++ ) {
		savefile->ReadInt( phenomenon_module_ids_to_disable[i] );
	}

	savefile->ReadBool( phenomenon_should_disable_random_module );
	savefile->ReadBool( phenomenon_should_set_oxygen_level );
	savefile->ReadInt( phenomenon_oxygen_level_to_set );
	savefile->ReadBool( phenomenon_should_set_ship_shields_to_zero );
	savefile->ReadBool( phenomenon_should_spawn_entity_def_on_playership );
	savefile->ReadInt( phenomenon_number_of_entity_defs_to_spawn );
	savefile->ReadString( phenomenon_entity_def_to_spawn );

	savefile->ReadBool( phenomenon_should_change_random_playership_crewmember_team );
	savefile->ReadBool( phenomenon_should_make_everything_go_slowmo );
	savefile->ReadBool( phenomenon_should_toggle_slowmo_on_and_off );
	savefile->ReadBool( phenomenon_should_ignore_the_rest_of_the_ship_ai_loop );

	savefile->ReadBool( play_low_oxygen_alert_sound );
	savefile->ReadBool( show_low_oxygen_alert_display );

	savefile->ReadInt( regenAmount );
	// BOYETTE RESTORE END
}

bool sbShip::ValidEntity(idEntity *ent) {
	if (!ent || !ent->IsType(idPlayer::Type) || ent->health <= 0) {
		return false;
	}

	return true;
}

void sbShip::EntityEncroaching(idEntity *ent) {
	if ((ent->health+regenAmount) > 90) {
		return; //let's not go over 90 health
	}
	ent->health += regenAmount;
}

int sbShip::GetCaptainTestNumber() {
	return captaintestnumber;
}

void sbShip::RecieveVolley() {
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		consoles[ENGINESMODULEID]->ControlledModule->Event_Remove();
		consoles[ENGINESMODULEID]->ControlledModule = NULL;
	}
	return;
	//SplitStringToInts( spawnArgs.GetString( "neutral_teams",""), ',', neutral_teams );		
	for ( int i = 0; i < neutral_teams.size(); i++ ) {
		gameLocal.Printf( idStr(neutral_teams[i]) + ".\n" );
	}
	gameLocal.Printf( idStr("The ") + name + idStr("'s team is: ") + idStr(team) + ".\n" );
	gameLocal.Printf( idStr("The Player") + idStr("'s team is: ") + idStr(gameLocal.GetLocalPlayer()->team) + ".\n" );
	captaintestnumber = 6;
	//gameLocal.GetLocalPlayer()->playerView.Flash(idVec4(.5,0,.7,.5),2000);
	idEntityFx::StartFx( "fx/sparks", &(GetPhysics()->GetOrigin()), 0, this, true );
	// we'll do something like this when the ship warps. We should also freeze the player in place while this is happening. End it with an event that get's sent off that includes the FreeWarp(0) function. Should only last a few seconds.

	/* // this is how we would do the slowmo if we could fix it's effect on the games overall time after it is done.
	gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "player_bfg_explode" ), SND_CHANNEL_ANY, 0, false, NULL );
	gameLocal.GetLocalPlayer()->playerView.AddWarp( idVec3( 0, 0, 0 ), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 1000 );
	gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(1,1,1,.5),1400);
	gameLocal.GetLocalPlayer()->weapon.GetEntity()->DetermineTimeGroup( true );
	gameLocal.GetLocalPlayer()->DetermineTimeGroup( true );
	g_enableSlowmo.SetBool(true); // need to either make a seperate cvar or set this to false when the gamelocal spawns. Because this doesn't get reset currently when you reload the level.
	*/

	// this is what we need to do to reset the screen color from the white fade.
	//gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),100);
	
	
	//gameLocal.GetLocalPlayer()->playerView.FreeWarp(0);
	//gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),100);
	//gameLocal.GetLocalPlayer()->weapon.GetEntity()->DetermineTimeGroup( false );
	//g_enableSlowmo.SetBool(false); // need to either make a seperate cvar or set this to false when the gamelocal spawns. Because this doesn't get reset currently when you reload the level.

	if ( crew[COMPUTERCREWID] ) {
		crew[COMPUTERCREWID]->TriggerHealFX(spawnArgs.GetString("heal_actor_emitter_def", "heal_actor_emitter_def_default"));
		crew[COMPUTERCREWID]->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
	}
	
	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule ) {
		//consoles[MEDICALMODULEID]->ControlledModule->renderEntity.gui[0];
			// this works great. But we need to do the GUi idea instead, as this doesn't look very good. We could spawn this at the beginning, and then just hide/activate it as needed. That would be more efficient.
			if ( !damageFX.GetEntity() || !damageFX.GetEntity()->IsActive() ) {
				idEntity *ent;
				const char *clsname;
				if ( !damageFX.GetEntity() ) {
					damageFXDef = gameLocal.FindEntityDefDict( "module_disabled_particle" ); // we probably need a spawnarg for the projectile def here. // we need to make this the sparkly particle projecile that spawns fires on impact.
					gameLocal.SpawnEntityDef( *damageFXDef, &ent, false );
					damageFX = ( idFuncEmitter * )ent; // boyette note: this is c-style cast that is converting(or designating) an idEntity into an idProjectile. A dynamic_cast or possibly a static_cast would probably work fine here instead but I haven't tried it yet.
				}
				damageFX.GetEntity()->SetOrigin(consoles[MEDICALMODULEID]->ControlledModule->GetPhysics()->GetOrigin() + idVec3(0,0,27));
				damageFX.GetEntity()->Event_Activate(this);
			} else if ( damageFX.GetEntity() && damageFX.GetEntity()->IsActive() ) {
				damageFX.GetEntity()->Event_Activate(this);
			}
	}

	// THIS IS GREAT - the skyportal entity will automatically update it's view. Everything on the forums was unnecessary - it works fine.
	/*
	MySkyPortalEnt->SetOrigin(MySkyPortalEnt->GetPhysics()->GetOrigin() + idVec3(200,200,200));
	*/


	// we'll do something like this to damage all idAI's and the player if the oxygen(or "Life Support") module health is zero or it is not powered:
	
	idEntity	*ent;
	idAI		*CrewMemberToOxygenDamage;
	int i;
	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType(idAI::Type) ) {
			CrewMemberToOxygenDamage = dynamic_cast<idAI*>(ent);
			if ( CrewMemberToOxygenDamage->ShipOnBoard && CrewMemberToOxygenDamage->ShipOnBoard == this ) {
				CrewMemberToOxygenDamage->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",3,0);
			}
		}
	}
	if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",3,0);
	}
	
	//gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(.5,0,.7,.5),2000);

	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule && TargetEntityInSpace ) { // we need to add a bunch of random numbers and/or numbers based on the damage to the module. All this has to be done on the module and when the module recieves ship to ship damage.
		for ( int i = 0; i < 10 ; i++ ) { // need to figure out why some of the projectiles move really slow.
			float randx;
			float randy;
			randx = gameLocal.random.CRandomFloat();
			randy = gameLocal.random.CRandomFloat();
			//CreateProjectile(consoles[MEDICALMODULEID]->ControlledModule->GetPhysics()->GetOrigin(),idVec3(randx,randy,0));
			idEntity *ent;
			const char *clsname;
			if ( !projectilearray[i].GetEntity() ) {
				//gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
				if ( !projectileDef ) {
					projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "recieve_damage_projectile_def", "projectile_module_ship_damage_default" ), false );
				}
				gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
				if ( !ent ) {
					clsname = projectileDef->GetString( "classname" );
					gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
				}
				if ( !ent->IsType( idProjectile::Type ) ) {
					clsname = ent->GetClassname();
					gameLocal.Error( "'%s' is not an idProjectile", clsname );
				}
				projectilearray[i] = ( idProjectile * )ent; // boyette note: this is c-style cast that is converting(or designating) an idEntity into an idProjectile. A dynamic_cast or possibly a static_cast would probably work fine here instead but I haven't tried it yet.
			}
			//projectilearray[i].GetEntity()->Create(consoles[MEDICALMODULEID]->ControlledModule, consoles[MEDICALMODULEID]->ControlledModule->GetPhysics()->GetOrigin(), idVec3(randx,randy,0) );
			projectilearray[i].GetEntity()->Create( consoles[MEDICALMODULEID]->ControlledModule, consoles[MEDICALMODULEID]->ControlledModule->GetPhysics()->GetOrigin() + idVec3(0,0,27), idVec3(randx,randy,0) ); // the 27 is just to get it off the gorund.
			//char buffer [33]; // good for 32 bit systems and above.
			//projectilearray[i].GetEntity()->spawnArgs.Set("gravity",(const char*)itoa(gameLocal.random.RandomInt(700),buffer,10)); // converts the int to a const char*. OR YOU COULD ADD A GRAVITY PARAMETER TO THE Projectile->Launch() FUNCTION.
			//projectilearray[i].GetEntity()->UpdateChangeableSpawnArgs(NULL);
			projectilearray[i].GetEntity()->Launch(consoles[MEDICALMODULEID]->ControlledModule->GetPhysics()->GetOrigin() + idVec3(0,0,27),idVec3(randx,randy,0), vec3_origin,0.0f,1.0f,1.0f,gameLocal.random.RandomInt(700)); // we need a spawnarg here for 700. // THIS WORKS GREAT. // the 27 is just to get it off the gorund.
		}
			//CreateProjectile(GetPhysics()->GetOrigin(),TargetEntityInSpace->GetPhysics()->GetOrigin());
			//projectile.GetEntity()->Create( this, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0] );
			//projectile.GetEntity()->Launch(GetPhysics()->GetOrigin(),(TargetEntityInSpace->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).ToAngles().ToForward(), vec3_origin); // THIS WORKS GREAT.
	}
	if ( TestHideShipEntity ) {
		TestHideShipEntity->Hide();
	}
	if ( crew[MEDICALCREWID] && crew[MEDICALCREWID]->health > 0 && room_node[MEDICALROOMID] ) {
		//crew[MEDICALCREWID]->Hide();
		// You need to make a public idEntity on idAI called EntityCommandedToMoveTo. You need to make a function on idAI called MoveToEntityCommandedTo that is the same as MoveToEntity but moves to the EntityCommandedToMoveTo (unless it is NULL in which case it does nothing).
		// You need to make a function called Event_MoveToEntityCommandedTo.
		// You need to put this event in the wait_for_enemy function on the AI script of the entity.
		// You need to make an event called Event_ReturnEntityCommandedToMoveTo that returns an entity. Then in the AI scripts you can make logic to move to that entity.
		//crew[MEDICALCREWID]->MoveToEntity( room_node[MEDICALROOMID] );
		crew[MEDICALCREWID]->SetEntityToMoveToByCommand( room_node[MEDICALROOMID] );
	}
	// boyette map entites link up end
}

void sbShip::GiveCrewMoveCommand(idEntity* MoveToEntity, sbShip* ShipToBeAboard) {
	if ( SelectedCrewMember && SelectedCrewMember->health > 0 && MoveToEntity && ShipToBeAboard && SelectedCrewMember->ShipOnBoard == ShipToBeAboard ) { // boyette future note: && !SelectedCrewMember->was_killed
		SelectedCrewMember->crew_auto_mode_activated = false;
		SelectedCrewMember->player_follow_mode_activated = false;
		SelectedCrewMember->handling_emergency_oxygen_situation = false;
		SelectedCrewMember->SetEntityToMoveToByCommand(MoveToEntity);
		battlestations = false;
	}
	for ( int i = 0; i < SelectedCrewMembers.size(); i++ ) {
		if ( SelectedCrewMembers[i] && SelectedCrewMembers[i]->health > 0 && MoveToEntity && ShipToBeAboard && SelectedCrewMembers[i]->ShipOnBoard == ShipToBeAboard ) { // boyette future note: && !SelectedCrewMember->was_killed
			SelectedCrewMembers[i]->crew_auto_mode_activated = false;
			SelectedCrewMembers[i]->player_follow_mode_activated = false;
			SelectedCrewMembers[i]->handling_emergency_oxygen_situation = false;
			SelectedCrewMembers[i]->SetEntityToMoveToByCommand(MoveToEntity);
			battlestations = false;
		}
	}
}

void sbShip::SetSelectedCrewMember(idAI* SelectedAI) {
	ClearCrewMemberSelection();
	if ( SelectedAI ) {
		SelectedCrewMember = SelectedAI;
		SelectedCrewMember->IsSelectedCrew = true;
	}
}

void sbShip::AddCrewMemberToSelection(idAI* SelectedAI) {
	if ( SelectedCrewMember ) {
		SelectedCrewMember->IsSelectedCrew = false;
	}
	SelectedCrewMember = NULL;

	// need to clear the crew members that were in the previous multi-selection.
	bool crewmember_already_selected = false;
	for ( int i = 0; i < SelectedCrewMembers.size(); i++ ) {
		if ( SelectedCrewMembers[i] && SelectedCrewMembers[i] == SelectedAI ) {
			return; // crewmember is already selected
		}
	}

	SelectedCrewMembers.push_back( SelectedAI );
	SelectedAI->IsSelectedCrew = true;
}

void sbShip::ClearCrewMemberSelection() {
	if ( SelectedCrewMember ) {
		SelectedCrewMember->IsSelectedCrew = false;
	}
	SelectedCrewMember = NULL;

	for ( int i = 0; i < SelectedCrewMembers.size(); i++ ) {
		SelectedCrewMembers[i]->IsSelectedCrew = false;
	}
	SelectedCrewMembers.clear();
}

void sbShip::ClearCrewMemberMultiSelection() {
	for ( int i = 0; i < SelectedCrewMembers.size(); i++ ) {
		SelectedCrewMembers[i]->IsSelectedCrew = false;
	}
	SelectedCrewMembers.clear();
}

bool sbShip::IsThisAIACrewmember( idAI* ai_to_check ) {
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] && crew[i] == ai_to_check) {
			return true;
		}
	}
	return false;
}


void sbShip::SetSelectedModule(sbModule* SelectedsbModule) {
	ClearCrewMemberSelection();
	if ( SelectedsbModule ) {
		SelectedModule = SelectedsbModule;
		SelectedModule->IsSelectedModule = true;
	}
}

void sbShip::ClearModuleSelection() {
	if ( SelectedModule ) {
		SelectedModule->IsSelectedModule = false;
	}
	SelectedModule = NULL;
}

int sbShip::ReturnOnBoardEntityDiagramPositionX(idEntity *ent) {
	int positionX;
	// ent->health > 0 && 
	if ( ent && ShipDiagramDisplayNode && !ent->was_killed ) { // boyette note - you probably want to reinsert this after development is done to hide hidden world entities from the gui(or maybe not for room node move commands): && !ent->IsHidden()
		positionX = (ent->GetPhysics()->GetOrigin().x - ShipDiagramDisplayNode->GetPhysics()->GetOrigin().x);
		return positionX;
	} else {
		return -400;
	}
}

int sbShip::ReturnOnBoardEntityDiagramPositionY(idEntity *ent) {
	int positionY;

	if ( ent && ShipDiagramDisplayNode && !ent->was_killed ) {
		positionY = (ent->GetPhysics()->GetOrigin().y - ShipDiagramDisplayNode->GetPhysics()->GetOrigin().y);
		return positionY;
	} else {
		return 400;
	}
}

int sbShip::ReturnOnBoardEntityDiagramPositionZ(idEntity *ent) {
	int positionZ;

	if ( ent && ShipDiagramDisplayNode && !ent->was_killed ) {
		positionZ = (ent->GetPhysics()->GetOrigin().z - ShipDiagramDisplayNode->GetPhysics()->GetOrigin().z);
		return positionZ;
	} else {
		return -400;
	}
}

void sbShip::IncreaseModulePower(sbModule *module) {
	if (current_power_reserve > 0 && (module->power_allocated < module->max_power) && (module->power_allocated < module->damage_adjusted_max_power) ) {
		module->power_allocated++;
		current_power_reserve--;
	}
	module->RecalculateModuleEfficiency();
}

void sbShip::DecreaseModulePower(sbModule *module) {
	if (module->power_allocated > 0) {
		module->power_allocated--;
		current_power_reserve++;
	}
	module->RecalculateModuleEfficiency();
}

/* // more efficient but more complex version(assigns pointer tbounds)
void sbShip::InitiateTransporter() {
	idEntity	*ent;
	int i;
	const idBounds* tbounds;
	if ( TransporterBounds && TargetEntityInSpace && TargetEntityInSpace->TransporterBounds ) {
		tbounds = &TransporterBounds->GetPhysics()->GetAbsBounds();
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && (ent->IsType( idAI::Type ) || ent->IsType( idPlayer::Type ) ) && tbounds->ContainsPoint(ent->GetPhysics()->GetOrigin()) ) {
				ent->SetOrigin( TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() + (ent->GetPhysics()->GetOrigin() - TransporterBounds->GetPhysics()->GetOrigin()) );
			}
		}
	}
	return;
}
*/

void sbShip::HandleTransporterEventsOnPlayerGuis() {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipTransporterActivated");
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
		gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipTransporterActivated");
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("ShipOnBoardTransporterActivated");
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardTransporterActivated");
	}
}

// slower but simpler version(doesn't assign pointer tbounds)
void sbShip::BeginTransporterSequence() {
	if ( !consoles[COMPUTERMODULEID] || !consoles[COMPUTERMODULEID]->ControlledModule ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship has no computer. It can't operate a transporter pad.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}
		return;
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->health <= 0 ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship's computer is disabled. It can't handle a transport sequence.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}			
		return;
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount < consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship's computer hasn't finished calculating the transport sequence.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}	
		return;
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount >= consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount && TargetEntityInSpace->shieldStrength >= ( TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters * TargetEntityInSpace->max_shieldStrength ) && TargetEntityInSpace->team != team ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "The target ship's shield is currently too strong to transport through it.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}	
		return;
	}

	idEntity	*ent;
	int i;
	if ( TransporterBounds && TargetEntityInSpace && TransporterBounds && TargetEntityInSpace->TransporterBounds ) {
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ((ent->IsType( idAI::Type ) && dynamic_cast<idAI*>( ent )->ai_can_be_transported) || ent->IsType( idPlayer::Type ) ) && TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(ent->GetPhysics()->GetAbsBounds()) ) {
				if ( ent && ent->IsType( idAI::Type ) ) {
					dynamic_cast<idAI*>( ent )->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
					ent->BeginTransporterMaterialShaderEffect(fx_color_theme);
					ent->ScheduleEndTransporterMaterialShaderEffect( 2500 );
				}
				if ( ent && ent->IsType( idPlayer::Type ) ) {
					if ( TargetEntityInSpace->red_alert ) {
						TargetEntityInSpace->SynchronizeRedAlertSpecialFX(); // start the red alert effects on the targetship if they are at red alert.
					}
					dynamic_cast<idPlayer*>( ent )->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
				}
			}
		}
	}
	// if ( succussful_transport ) {
	// EMPTY THE COMPUTER CHARGE AMOUNT
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
		consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount = 0;
		consoles[COMPUTERMODULEID]->ControlledModule->current_charge_percentage = 0;
	}
	TriggerShipTransporterPadFX();
	TargetEntityInSpace->TriggerShipTransporterPadFX();
	HandleTransporterEventsOnPlayerGuis();
	PostEventMS( &EV_InitiateTransporter, 2500 );
	return;
}

// slower but simpler version(doesn't assign pointer tbounds)
void sbShip::InitiateTransporter() {
	idEntity	*ent;
	int i;
	bool		playership_intruder_alert = false;
	bool		nothing_was_transported = true;
	bool		player_was_transported = false;
	if ( TransporterBounds && TargetEntityInSpace && TransporterBounds && TargetEntityInSpace->TransporterBounds /*&& TransporterPad && TargetEntityInSpace->TransporterPad*/ ) {
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ((ent->IsType( idAI::Type ) && dynamic_cast<idAI*>( ent )->ai_can_be_transported) || ent->IsType( idPlayer::Type ) ) && TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(ent->GetPhysics()->GetAbsBounds()) ) {

				idVec3 destination_point = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() + (ent->GetPhysics()->GetOrigin() - TransporterBounds->GetPhysics()->GetOrigin());
				idBounds current_location_bounds = TransporterBounds->GetPhysics()->GetAbsBounds(); // GetAbsBounds is apparently correct - GetBounds is not
				idBounds destination_bounds = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetAbsBounds(); // GetAbsBounds is apparently correct - GetBounds is not
				//idBounds destination_bounds = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetBounds();

				if ( destination_bounds.GetVolume() < current_location_bounds.GetVolume() ) { // see if we need to adjust the destination point for a smaller transporter pad or area.
					//gameLocal.Printf( "The target bounds was smaller. \n" );
					// BOYETTE NOTE: destination_bounds.GetRadius() doesn't seem to be giving me the radius I want - instead I used destination_bounds[1][0] - destination_bounds.GetCenter().x  //destination_point = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() + ( (ent->GetPhysics()->GetOrigin() - TransporterBounds->GetPhysics()->GetOrigin() ) * (destination_bounds.GetRadius() / current_location_bounds.GetRadius()) );
					destination_point = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() + ( (ent->GetPhysics()->GetOrigin() - TransporterBounds->GetPhysics()->GetOrigin() ) * ( (destination_bounds[1][0] - destination_bounds.GetCenter().x) / (current_location_bounds[1][0] - current_location_bounds.GetCenter().x)) );
				
					destination_point.x = idMath::ClampFloat(destination_bounds[0][0],destination_bounds[1][0],destination_point.x); // just for good measure now
					destination_point.y = idMath::ClampFloat(destination_bounds[0][1],destination_bounds[1][1],destination_point.y); // just for good measure now
					//destination_point.z = idMath::ClampFloat(destination_bounds[0][2],destination_bounds[1][2],destination_point.z); // just for good measure now
					destination_point.z = destination_bounds[0][2]; // we don't want the AI to be up in the air.
							//left//x-min//1//destination_bounds[0][0]
							//right//x-max//2//destination_bounds[1][0]
							//down//y-min//3//destination_bounds[0][2])
							//up//y-max//4//destination_bounds[1][2])
							//back//z-min//5//destination_bounds[0][1]
							//front//z-max//6//destination_bounds[1][1]
				}

				if ( ent->IsType( idPlayer::Type ) ) {
					if ( g_enableWeirdTextureThrashingFix.GetBool() ) {
						gameLocal.GetLocalPlayer()->StartWeirdTextureThrashFix(destination_point,1);
					} else {
						ent->SetOrigin( destination_point );
					}
				} else {
					ent->SetOrigin( destination_point );
				}
				//gameLocal.Printf( ent->name + " has a gravity of: " + idStr(ent->GetPhysics()->GetGravity().x) + "," + idStr(ent->GetPhysics()->GetGravity().y) + "," + idStr(ent->GetPhysics()->GetGravity().z)  );
				ent->TemporarilySetThisEntityGravityToZero(200); // BOYETTE NOTE TODO IMPORTANT: MAYBE TODO: we could make the 200 a factor a the z distance the entity is being transported. BOYETTE NOTE: this is so they go immediately to the z value and are affected by gravity while transporting.
				nothing_was_transported = false;
				//TargetEntityInSpace->MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
				if ( ent && ent->IsType( idAI::Type ) ) {

					for ( int i = 0; i < AIsOnBoard.size() ; i++ ) { // make sure nobody tries to keep moving to them
						if ( AIsOnBoard[i] && AIsOnBoard[i]->EntityCommandedToMoveTo == ent ) {
							AIsOnBoard[i]->EntityCommandedToMoveTo == NULL;
						}
					}

					AIsOnBoard.erase(std::remove(AIsOnBoard.begin(), AIsOnBoard.end(), dynamic_cast<idAI*>( ent )), AIsOnBoard.end()); // remove the ai from the list of AI's on board this ship
					TargetEntityInSpace->AIsOnBoard.push_back(dynamic_cast<idAI*>( ent )); // add the ai to the list of AI's on board the targetentityinspace
					dynamic_cast<idAI*>( ent )->ShipOnBoard = TargetEntityInSpace;
					if ( ent->team != TargetEntityInSpace->team ) TargetEntityInSpace->red_alert = true; // if an enemy transports aboard a ship, the ship will go to red alert.
					if ( ent->team != TargetEntityInSpace->team && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) playership_intruder_alert = true;
					if ( !TargetEntityInSpace->is_derelict ) {
						EndNeutralityWithTeam( TargetEntityInSpace->team );
						TargetEntityInSpace->EndNeutralityWithTeam( team );
					}
					ent->EndTransporterMaterialShaderEffect();
				}
				if ( ent && ent->IsType( idPlayer::Type ) ) {
					StopRedAlertSpecialFX(); // We don't need them to be playing on the ship we are transporting off of.
					g_enableSlowmo.SetBool( false );
					gameLocal.GetLocalPlayer()->blurEnabled = false;
					gameLocal.GetLocalPlayer()->desaturateEnabled = false;
					gameLocal.GetLocalPlayer()->bloomEnabled = false;
					gameLocal.GetLocalPlayer()->playerView.FreeWarp(0);
					gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),3200);
					if ( ent->team != TargetEntityInSpace->team && ( !TargetEntityInSpace->is_derelict || TargetEntityInSpace->do_red_alert_when_derelict ) ) TargetEntityInSpace->red_alert = true; // if an enemy transports aboard a ship, the ship will go to red alert.
					/* disabled 02/26/2017 to allow player to transport aboard the transitorian station to get the artifact.
					if ( !TargetEntityInSpace->is_derelict ) {
						EndNeutralityWithTeam( TargetEntityInSpace->team );
						TargetEntityInSpace->EndNeutralityWithTeam( team );
					}
					*/
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity->Hide();
					}
					dynamic_cast<idPlayer*>( ent )->ShipOnBoard = TargetEntityInSpace;
					player_was_transported = true;
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity->Show();
					}
					gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
					gameLocal.GetLocalPlayer()->UpdateSpaceCommandTabletGUIOnce();
					gameLocal.GetLocalPlayer()->player_just_transported = true;
					if ( TargetEntityInSpace->red_alert ) {
						TargetEntityInSpace->SynchronizeRedAlertSpecialFX(); // this is just for good measure. The one above in the event scheduling function should have been successful. //  start the red alert effects on the targetship if they are at red alert. // BOYETTE TODO: We might need to put a cleanup function here to make sure that any other ships that we aren't on board don't have any effects playing.
					}
					if ( TargetEntityInSpace->is_derelict && !TargetEntityInSpace->do_red_alert_when_derelict ) {
						TargetEntityInSpace->DimRandomShipLightsOff();
					}
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->MySkyPortalEnt ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
					}
					// since the skyportal is inside the ship we want to hide it.
					gameLocal.UpdateSpaceEntityVisibilities();
					gameLocal.UpdateSpaceCommandViewscreenCamera();

					// BOYETTE MUSIC BEGIN
					if ( s_in_game_music_volume.GetFloat() >= -35.0f ) {
						if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
						} else {
							idStr potential_song_to_play;
							if ( TargetEntityInSpace->is_derelict ) {
								potential_song_to_play = "derelict_music_long";
							} else if ( TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) {
								potential_song_to_play = TargetEntityInSpace->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
							} else {
								potential_song_to_play = TargetEntityInSpace->spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" );
							}
							if ( gameLocal.GetLocalPlayer()->music_shader_is_playing && gameLocal.time < gameLocal.GetLocalPlayer()->currently_playing_music_shader_end_time && gameLocal.GetLocalPlayer()->currently_playing_music_shader == potential_song_to_play ) {
							} else {
								gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_song_to_play;
								int song_length = 0;
								//gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound("bellumturan_battle_music_short"), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
								gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound(gameLocal.GetLocalPlayer()->currently_playing_music_shader), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
								//gameLocal.GetLocalPlayer()->currently_playing_music_shader = this->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
								gameLocal.GetLocalPlayer()->currently_playing_music_shader_begin_time = gameLocal.time;
								gameLocal.GetLocalPlayer()->currently_playing_music_shader_end_time = gameLocal.time + song_length;
								gameLocal.GetLocalPlayer()->music_shader_is_playing = true;
								gameLocal.GetLocalPlayer()->BeginMonitoringMusic();
								//gameSoundWorld->PlayShaderDirectly("bellumturan_battle_music_short",SND_CHANNEL_ANY);
								gameSoundWorld->FadeSoundClasses(2,-100.0f,0.0f); // BOYETTE NOTE: so it can fade up
								gameSoundWorld->FadeSoundClasses(2,s_in_game_music_volume.GetFloat(),3.0f); // BOYETTE NOTE: music sounds are soundclass 2. 0.0f db is the default level.
								gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f); // fade the alarm sounds down while music is playing
							}
						}
					}
					// BOYETTE MUSIC END
				}
			}
		}

		//if ( !player_was_transported && !red_alert ) {
		//	TargetEntityInSpace->StopRedAlertSpecialFX();
		//	gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false);
		//}
		if ( playership_intruder_alert && gameLocal.GetLocalPlayer()->ShipOnBoard == gameLocal.GetLocalPlayer()->PlayerShip ) {
			gameLocal.GetLocalPlayer()->PlayerShip->SynchronizeRedAlertSpecialFX();
			gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_intruder_alert", "ship_alert_intruder_default" ) ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
			if ( gameLocal.GetLocalPlayer()->guiOverlay ) {
				gameLocal.GetLocalPlayer()->guiOverlay->HandleNamedEvent("IntruderAlert");
			} else if ( gameLocal.GetLocalPlayer()->hud ) {
				gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("IntruderAlert");
			}
		}
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			if ( nothing_was_transported ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("^1Nothing Was Transported^0");
			} else if ( !nothing_was_transported && !playership_intruder_alert ) {
				if ( player_was_transported && !gameLocal.GetLocalPlayer()->hud_map_visible ) {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("^5Transport Successful:^0 \n ^5Press -" + idStr(common->KeysFromBinding("_impulse42")) + "- to toggle a map overlay on/off^0" );
				} else {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("^5Transport Successful^0" );
				}
			}
		}
		TargetEntityInSpace->HandleTransporterEventsOnPlayerGuis();
	}
	return;
}

void sbShip::BeginRetrievalTransportSequence() {
	if ( !consoles[COMPUTERMODULEID] || !consoles[COMPUTERMODULEID]->ControlledModule ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship has no computer. It can't operate a transporter pad.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}
		return;
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->health <= 0 ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship's computer is disabled. It can't handle a transport sequence.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}			
		return;
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount < consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "Your ship's computer hasn't finished calculating the transport sequence.");
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Transport Sequence Unsuccessful^0");
		}	
		return;
	}
	idEntity	*ent;
	int i;
	if ( TransporterBounds && TargetEntityInSpace && TransporterBounds && TargetEntityInSpace->TransporterBounds ) {
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ((ent->IsType( idAI::Type ) && dynamic_cast<idAI*>( ent )->ai_can_be_transported) || ent->IsType( idPlayer::Type ) ) && TargetEntityInSpace->TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(ent->GetPhysics()->GetAbsBounds()) ) {
				if ( ent && ent->IsType( idAI::Type ) ) {
					dynamic_cast<idAI*>( ent )->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
					ent->BeginTransporterMaterialShaderEffect(TargetEntityInSpace->fx_color_theme);
				}
				if ( ent && ent->IsType( idPlayer::Type ) ) {
					if ( red_alert ) {
						SynchronizeRedAlertSpecialFX(); // start the red alert effects on the targetship if they are at red alert.
					}
					dynamic_cast<idPlayer*>( ent )->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
				}
			}
		}
	}
	// if ( succussful_transport ) {
	// EMPTY THE COMPUTER CHARGE AMOUNT
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
		consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount = 0;
		consoles[COMPUTERMODULEID]->ControlledModule->current_charge_percentage = 0;
	}
	TriggerShipTransporterPadFX();
	TargetEntityInSpace->TriggerShipTransporterPadFX();
	TargetEntityInSpace->HandleTransporterEventsOnPlayerGuis();
	PostEventMS( &EV_InitiateRetrievalTransport, 2500 );
	return;
}

void sbShip::InitiateRetrievalTransport() {
	idEntity	*ent;
	int i;
	bool		playership_intruder_alert = false;
	bool		nothing_was_transported = true;
	bool		player_was_transported = false;
	if ( TransporterBounds && TargetEntityInSpace && TransporterBounds && TargetEntityInSpace->TransporterBounds /*&& TransporterPad && TargetEntityInSpace->TransporterPad*/ ) {
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ((ent->IsType( idAI::Type ) && dynamic_cast<idAI*>( ent )->ai_can_be_transported) || ent->IsType( idPlayer::Type ) ) && TargetEntityInSpace->TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(ent->GetPhysics()->GetAbsBounds()) ) {

				idVec3 destination_point = TransporterBounds->GetPhysics()->GetOrigin() + (ent->GetPhysics()->GetOrigin() - TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin());
				idBounds current_location_bounds = TargetEntityInSpace->TransporterBounds->GetPhysics()->GetAbsBounds(); // GetAbsBounds is apparently correct - GetBounds is not
				idBounds destination_bounds = TransporterBounds->GetPhysics()->GetAbsBounds(); // GetAbsBounds is apparently correct - GetBounds is not
				//idBounds destination_bounds = TransporterBounds->GetPhysics()->GetBounds();

				if ( destination_bounds.GetVolume() < current_location_bounds.GetVolume() ) { // see if we need to adjust the destination point for a smaller transporter pad or area.
					//gameLocal.Printf( "Your bounds was smaller. \n" );
					// BOYETTE NOTE: destination_bounds.GetRadius() doesn't seem to be giving me the radius I want - instead I used destination_bounds[1][0] - destination_bounds.GetCenter().x  //destination_point = TransporterBounds->GetPhysics()->GetOrigin() + ( (ent->GetPhysics()->GetOrigin() - TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() ) * (destination_bounds.GetRadius() / current_location_bounds.GetRadius()) );
					destination_point = TransporterBounds->GetPhysics()->GetOrigin() + ( (ent->GetPhysics()->GetOrigin() - TargetEntityInSpace->TransporterBounds->GetPhysics()->GetOrigin() ) * ( (destination_bounds[1][0] - destination_bounds.GetCenter().x) / (current_location_bounds[1][0] - current_location_bounds.GetCenter().x)) );
					
					destination_point.x = idMath::ClampFloat(destination_bounds[0][0],destination_bounds[1][0],destination_point.x); // just for good measure now
					destination_point.y = idMath::ClampFloat(destination_bounds[0][1],destination_bounds[1][1],destination_point.y); // just for good measure now
					//destination_point.z = idMath::ClampFloat(destination_bounds[0][2],destination_bounds[1][2],destination_point.z); // just for good measure now
					destination_point.z = destination_bounds[0][2]; // we don't want the AI to be up in the air.
				}

				if ( ent->IsType( idPlayer::Type ) ) {
					if ( g_enableWeirdTextureThrashingFix.GetBool() ) {
						gameLocal.GetLocalPlayer()->StartWeirdTextureThrashFix(destination_point,1);
					} else {
						ent->SetOrigin( destination_point );
					}
				} else {
					ent->SetOrigin( destination_point );
				}
				//gameLocal.Printf( ent->name + " has a gravity of: " + idStr(ent->GetPhysics()->GetGravity().x) + "," + idStr(ent->GetPhysics()->GetGravity().y) + "," + idStr(ent->GetPhysics()->GetGravity().z)  );
				ent->TemporarilySetThisEntityGravityToZero(200); // BOYETTE NOTE TODO IMPORTANT: MAYBE TODO: we could make the 200 a factor a the z distance the entity is being transported. BOYETTE NOTE: this is so they go immediately to the z value and are affected by gravity while transporting.
				nothing_was_transported = false;
				if ( ent && ent->IsType( idAI::Type ) ) {

					for ( int i = 0; i < TargetEntityInSpace->AIsOnBoard.size() ; i++ ) { // make sure nobody tries to keep moving to them
						if ( TargetEntityInSpace->AIsOnBoard[i] && TargetEntityInSpace->AIsOnBoard[i]->EntityCommandedToMoveTo == ent ) {
							TargetEntityInSpace->AIsOnBoard[i]->EntityCommandedToMoveTo == NULL;
						}
					}

					TargetEntityInSpace->AIsOnBoard.erase(std::remove(TargetEntityInSpace->AIsOnBoard.begin(), TargetEntityInSpace->AIsOnBoard.end(), dynamic_cast<idAI*>( ent )), TargetEntityInSpace->AIsOnBoard.end()); // remove the ai from the list of AI's on board the targetentityinspace
					AIsOnBoard.push_back(dynamic_cast<idAI*>( ent )); // add the ai to the list of AI's on board this ship
					dynamic_cast<idAI*>( ent )->ShipOnBoard = this;
					if ( ent->team != team ) red_alert = true; // if an enemy transports aboard a ship, the ship will go to red alert.
					if ( ent->team != team && gameLocal.GetLocalPlayer()->PlayerShip && this == gameLocal.GetLocalPlayer()->PlayerShip ) playership_intruder_alert = true;
					if ( !TargetEntityInSpace->is_derelict ) {
						EndNeutralityWithTeam( TargetEntityInSpace->team );
						TargetEntityInSpace->EndNeutralityWithTeam( team );
					}
					ent->EndTransporterMaterialShaderEffect();
				}
				if ( ent && ent->IsType( idPlayer::Type ) ) {
					TargetEntityInSpace->StopRedAlertSpecialFX(); // We don't need them to be playing on the ship we are transporting off of.
					g_enableSlowmo.SetBool( false );
					gameLocal.GetLocalPlayer()->blurEnabled = false;
					gameLocal.GetLocalPlayer()->desaturateEnabled = false;
					gameLocal.GetLocalPlayer()->bloomEnabled = false;
					gameLocal.GetLocalPlayer()->playerView.FreeWarp(0);
					gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),3200);
					if ( ent->team != team && ( !is_derelict || do_red_alert_when_derelict ) ) red_alert = true; // if an enemy transports aboard a ship, the ship will go to red alert.
					/* disabled 02/26/2017 to allow player to transport aboard the transitorian station to get the artifact.
					if ( !TargetEntityInSpace->is_derelict ) {
						EndNeutralityWithTeam( TargetEntityInSpace->team );
						TargetEntityInSpace->EndNeutralityWithTeam( team );
					}
					*/
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity->Hide();
					}
					dynamic_cast<idPlayer*>( ent )->ShipOnBoard = this;
					player_was_transported = true;
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->ViewScreenEntity->Show();
					}
					gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
					gameLocal.GetLocalPlayer()->UpdateSpaceCommandTabletGUIOnce();
					gameLocal.GetLocalPlayer()->player_just_transported = true;
					if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->MySkyPortalEnt ) {
						gameLocal.GetLocalPlayer()->ShipOnBoard->MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
					}
					if ( red_alert ) {
						SynchronizeRedAlertSpecialFX(); // this is just for good measure. The one above in the event scheduling function should have been successful. // Start the red alert effects on the retrieving ship if they are at red alert. // BOYETTE TODO: We might need to put a cleanup function here to make sure that any other ships that we aren't on board don't have any effects playing.
					}
					if ( is_derelict && !do_red_alert_when_derelict ) {
						DimRandomShipLightsOff();
					}
					// since the skyportal is inside the ship we want to hide it.
					gameLocal.UpdateSpaceEntityVisibilities();
					gameLocal.UpdateSpaceCommandViewscreenCamera();

					// BOYETTE MUSIC BEGIN
					if ( s_in_game_music_volume.GetFloat() >= -35.0f ) {
						if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
						} else {
							idStr potential_song_to_play;
							if ( this->is_derelict ) {
								potential_song_to_play = "derelict_music_long";
							} else if ( this->team != gameLocal.GetLocalPlayer()->team ) {
								potential_song_to_play = this->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
							} else {
								potential_song_to_play = this->spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" );
							}
							if ( gameLocal.GetLocalPlayer()->music_shader_is_playing && gameLocal.time < gameLocal.GetLocalPlayer()->currently_playing_music_shader_end_time && gameLocal.GetLocalPlayer()->currently_playing_music_shader == potential_song_to_play ) {
							} else {
								gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_song_to_play;
								int song_length = 0;
								//gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound("bellumturan_battle_music_short"), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
								gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound(gameLocal.GetLocalPlayer()->currently_playing_music_shader), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
								//gameLocal.GetLocalPlayer()->currently_playing_music_shader = this->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
								gameLocal.GetLocalPlayer()->currently_playing_music_shader_begin_time = gameLocal.time;
								gameLocal.GetLocalPlayer()->currently_playing_music_shader_end_time = gameLocal.time + song_length;
								gameLocal.GetLocalPlayer()->music_shader_is_playing = true;
								gameLocal.GetLocalPlayer()->BeginMonitoringMusic();
								//gameSoundWorld->PlayShaderDirectly("bellumturan_battle_music_short",SND_CHANNEL_ANY);
								gameSoundWorld->FadeSoundClasses(2,-100.0f,0.0f); // BOYETTE NOTE: so it can fade up
								gameSoundWorld->FadeSoundClasses(2,s_in_game_music_volume.GetFloat(),3.0f); // BOYETTE NOTE: music sounds are soundclass 2. 0.0f db is the default level.
								gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f); // fade the alarm sounds down while music is playing
							}
						}
					}
					// BOYETTE MUSIC END
				}
			}
		}

		//if ( !player_was_transported && !TargetEntityInSpace->red_alert ) {
		//	StopRedAlertSpecialFX();
		//	gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false);
		//}
		if ( playership_intruder_alert && gameLocal.GetLocalPlayer()->ShipOnBoard == gameLocal.GetLocalPlayer()->PlayerShip ) {
			gameLocal.GetLocalPlayer()->PlayerShip->SynchronizeRedAlertSpecialFX();
			gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_intruder_alert", "ship_alert_intruder_default" ) ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
			if ( gameLocal.GetLocalPlayer()->guiOverlay ) {
				gameLocal.GetLocalPlayer()->guiOverlay->HandleNamedEvent("IntruderAlert");
			} else if ( gameLocal.GetLocalPlayer()->hud ) {
				gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("IntruderAlert");
			}
		}
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			if ( nothing_was_transported ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("^1Nothing Was Transported^0");
			} else if ( !nothing_was_transported && !playership_intruder_alert ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("^5Transport Successful^0");
			}
		}
		HandleTransporterEventsOnPlayerGuis();
	}
	return;
}

bool sbShip::AttemptWarp(int stargriddestx,int stargriddesty) {
	//gameLocal.Printf( "The " + name + " is attempting to attempt the warp.\n" );
	//gameLocal.Printf( "Milliseconds at beginning:" + idStr( gameLocal.time ) + "\n" );
	// SOME NOTIFICATIONS FOR REASONS FOR WARP FAILURE BEGIN
	if ( !consoles[ENGINESMODULEID] || !consoles[ENGINESMODULEID]->ControlledModule ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP HAS NO ENGINES. IT CANNOT WARP.");
		}
		return false;
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->health <= 0 ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP'S ENGINES ARE DISABLED. IT CANNOT WARP.");
		}			
		return false;
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount < consoles[ENGINESMODULEID]->ControlledModule->max_charge_amount ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP'S ENGINES ARE NOT FULLY CHARGED. IT CANNOT WARP YET.");
		}				
		return false;
	}
	if ( ship_destruction_sequence_initiated ) {
		return false;
	}
	// SOME NOTIFICATIONS FOR REASONS FOR WARP FAILURE END

	int PlayerPosX;
	int PlayerPosY;

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
		PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
	} else { // boyette todo noto - space command note - the player can be either on an sbShip or an sbStationSpaceEntity - and maybe one day fighters or shuttles. So we will have a few variables called ShipOnBoard, PlanetOnBoard, ShuttleOnBoard etc.
		PlayerPosX = 0;
		PlayerPosY = 0;
	}

	// BOYETTE NOTE TODO: implement this same control flow in the attempt warp function - 4 possibilities - this is the playership and the shiponboard - this is not the playership but it is the shiponboard - this is the playership but it is not the shiponboard - this is not the playership or the shiponboard
	// BOYETTE NOTE TODO: don't let the player use the captain gui if the playership is not at the same stargrid position as the shiponboard - we will take care of this when we do the return to player autopilot for when the player gets to a different stargrid position then the playership
	if ( gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		// do just this as of right now
		g_enableSlowmo.SetBool( true );
	} else if ( gameLocal.GetLocalPlayer()->PlayerShip != this && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {

		if ( gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() ) {
			return false;
		}
		if ( gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(stargriddestx, stargriddesty) ) {
			return false;
		}
		g_enableSlowmo.SetBool( true );
	} else if ( gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard != this ) {
		// do nothing as of right now
	} else {
		//If you are moving to a new stargrid position (not the PlayerShip or ShipOnBoard position - Check to see if it is full. If it is, cancel the warp.
		if ( gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() ) {
			return false;
		}
		if ( stargriddestx == PlayerPosX && stargriddesty == PlayerPosY ) {
			if ( !gameLocal.CheckAllNonPlayerSkyPortalsForOccupancy() ) {
				return false;
			}
		}
		if ( gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(stargriddestx, stargriddesty) ) {
			return false;
		}
	}
	// IF WE MAKE IT TO THIS POINT THE WARP WILL BE SUCCESSFUL.
	if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		// disable the captain menu.
		if ( gameLocal.GetLocalPlayer()->guiOverlay ) {
			if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->CaptainGui ) {
				gameLocal.GetLocalPlayer()->CloseOverlayCaptainGui();
			} else if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
				gameLocal.GetLocalPlayer()->CloseOverlayHailGui();
			} else if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->AIDialogueGui ) {
				gameLocal.GetLocalPlayer()->CloseOverlayAIDialogeGui();
			} else {
				gameLocal.GetLocalPlayer()->CloseOverlayGui();
			}
		}
		// BOYETTE MUSIC BEGIN
		// fade out the music:
		gameSoundWorld->FadeSoundClasses(2,-100.0f,1.5f); // BOYETTE NOTE: music sounds are soundclass 2. 0.0f db is the default level.
		// BOYETTE MUSIC END
		// show the player specific ship on board warp effects.
		gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_initiate_warp", "initiate_warp_on_ship_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL, false ); // the last argument being false ensures that this sounds ignores slowmo - plays at normal speed
		gameLocal.GetLocalPlayer()->playerView.AddWarp( idVec3( 0, 0, 0 ), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 1000 );

		gameLocal.GetLocalPlayer()->blurEnabled = false;
		gameLocal.GetLocalPlayer()->desaturateEnabled = false;
		gameLocal.GetLocalPlayer()->bloomEnabled = true;
		g_testBloomNumPasses.SetInteger(0);
		gameLocal.GetLocalPlayer()->TransitionNumBloomPassesToThirty();
			//gameLocal.Printf( "Milliseconds at end:" + idStr( gameLocal.time ) + "\n" );

		gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(1,1,1,.82),1400);
		//gameLocal.GetLocalPlayer()->weapon.GetEntity()->DetermineTimeGroup( true );
		//gameLocal.GetLocalPlayer()->DetermineTimeGroup( true );
		//gameLocal.GetLocalPlayer()->PlayerShip->DetermineTimeGroup(true);
		//g_enableSlowmo.SetBool(true); // need to either make a separate cvar or set this to false when the gamelocal spawns. Because this doesn't get reset currently when you reload the level.
	}
	// show the general warp effects.
	//idEntityFx::StartFx( "fx/sparks", &(GetPhysics()->GetOrigin()), 0, this, true );

	// BOYETTE NOTE: was this before 08 12 2016 // if ( stargridpositionx == PlayerPosX && stargridpositiony == PlayerPosY && gameLocal.GetLocalPlayer()->ShipOnBoard != this) {
	if ( stargriddestx == PlayerPosX && stargriddesty == PlayerPosY && gameLocal.GetLocalPlayer()->ShipOnBoard != this) {
		idEntityFx::StartFx( spawnArgs.GetString("spaceship_warp_fx", "fx/spaceship_warp_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, true );
		gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_initiate_warp_in_space", "initiate_warp_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL );
	}
	// schedule the engage warp event.
	verified_warp_stargrid_postion_x = stargriddestx;
	verified_warp_stargrid_postion_y = stargriddesty;

	PostEventMS( &EV_EngageWarp, 1500 );
	return true;
}

void sbShip::EngageWarp(int stargriddestx,int stargriddesty) {
	is_attempting_warp = false;
	//gameLocal.Printf( "The " + name + " is attempting to engage the warp.\n" );
	// SOME NOTIFICATIONS FOR REASONS FOR WARP FAILURE BEGIN
	if ( !consoles[ENGINESMODULEID] || !consoles[ENGINESMODULEID]->ControlledModule ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP HAS NO ENGINES. IT CANNOT WARP.");
		}
		return;
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->health <= 0 ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP'S ENGINES ARE DISABLED. IT CANNOT WARP.");
		}			
		return;
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount < consoles[ENGINESMODULEID]->ControlledModule->max_charge_amount ) { // will add more things here. health needs to be above zero and power needs to be above zero in order to warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1" "YOUR SHIP'S ENGINES ARE NOT FULLY CHARGED. IT CANNOT WARP YET.");
		}				
		return;
	}
	// if there are more ships at the destination than there are non-player skyportals, and this is not the playership, then cancel the warp.
	if ( !gameLocal.GetLocalPlayer()->PlayerShip || ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip != this ) ) {
		if ( gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() ) {
			//gameLocal.Printf( "There is not an available skyportal for " + name + " at the stargrid destination of " + idStr(stargriddestx) + "," + idStr(stargriddesty) + ". The warp is cancelled.\n" );
			return;
		}
	}
	if ( ship_destruction_sequence_initiated ) {
		return;
	}
	// SOME NOTIFICATIONS FOR REASONS FOR WARP FAILURE END

	int PlayerPosX;
	int PlayerPosY;

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
		PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
	} else { // boyette todo noto - space command note - the player can be either on an sbShip or an sbStationSpaceEntity - and maybe one day fighters or shuttles. So we will have a few variables called ShipOnBoard, PlanetOnBoard, ShuttleOnBoard etc.
		PlayerPosX = 0;
		PlayerPosY = 0;
	}

	LeavingStargridPosition(stargridpositionx,stargridpositiony);

	bool should_update_dynamic_lights = false;

	if ( gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		stargridpositionx = stargriddestx;
		stargridpositiony = stargriddesty;
		gameLocal.SetAllSkyPortalsOccupancyStatusToFalse();
		//For all ship entities set MySkyPortalEnt to NULL;
		gameLocal.SetAllShipSkyPortalEntitiesToNull();
		// just in case:
		MySkyPortalEnt = NULL;
		//Move to the Skyportal Marked "player" and mark it as occupied.
		//MySkyPortalEnt = Skyportal Marked "player";
		ClaimUnnoccupiedPlayerSkyPortalEntity();
		//For all ship entities at the location, assign them to a skyportal(sbShip->FindUnnoccupiedSkyportal()), set their MySkYPortalEnt and mark that skyportal as occupied.
		gameLocal.AssignAllShipsAtCurrentLocToASkyPortalEntity();
		//Run some kind of update to hide all ship entities whose stargrid positions do not equal the gameLocal.Player->ShipOnBoard->stargridpositions. Maybe make a gameLocal function.
		gameLocal.UpdateSpaceEntityVisibilities();
		PostEventMS( &EV_UpdateViewscreenCamera, 2000 );

		// will uncomment this soon:
		gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();
		if ( MySkyPortalEnt ) {
			MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
		}

		should_update_dynamic_lights = true;
		gameLocal.GetLocalPlayer()->allow_overlay_captain_gui = false;
		PostEventMS( &EV_DisplayStoryWindow, 2500 );

		GivePlayerAppropriatePDAEmailsAndVideosForStargridPosition(stargriddestx,stargriddesty);
		// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
		if ( common->m_pStatsAndAchievements ) {
			common->m_pStatsAndAchievements->m_nPlayerShipWarps++;
			common->StoreSteamStats();
		}
#endif
		// BOYETTE STEAM INTEGRATION END
	} else if ( gameLocal.GetLocalPlayer()->PlayerShip != this && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		//If you are moving to the position of the Player ShipOnBoard - Check to see if they are all occupied. If they are, cancel the warp.
		//if ( stargriddestx == PlayerPosX && stargriddesty == PlayerPosY ) {
		//	if ( !gameLocal.CheckAllNonPlayerSkyPortalsForOccupancy() ) {
		//		return;
		//	}
		//}
		stargridpositionx = stargriddestx;
		stargridpositiony = stargriddesty;
		gameLocal.SetAllSkyPortalsOccupancyStatusToFalse();
		//For all ship entities set MySkyPortalEnt to NULL;
		gameLocal.SetAllShipSkyPortalEntitiesToNull();
		// just in case:
		MySkyPortalEnt = NULL;
		//Move to the FindUnoccupiedSkyportalEnt() and mark it as occupied.
		//MySkyPortalEnt = FindUnoccupiedSkyportalEnt()
		ClaimUnnoccupiedSkyPortalEntity();
		//For all ship entities at the location, assign them to a skyportal(sbShip->FindUnnoccupiedSkyportal()), set their MySkYPortalEnt and mark that skyportal as occupied.
		gameLocal.AssignAllShipsAtCurrentLocToASkyPortalEntity();
		//Run some kind of update to hide all ship entities whose stargrid positions do not equal the gameLocal.Player->ShipOnBoard->stargridpositions. Maybe make a gameLocal function.
		gameLocal.UpdateSpaceEntityVisibilities();
		PostEventMS( &EV_UpdateViewscreenCamera, 2000 );

		// will uncomment this soon:
		gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();
		if ( MySkyPortalEnt ) {
			MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
		}
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
		// might want to allow the playership to continue targetting the ship that the player is currently on board - even if it warps away. - maybe
		gameLocal.GetLocalPlayer()->PopulateShipList();
		//if the ShipOnBoard warps to a different stargrid position we can't target it anymore.
		if ( stargriddestx != PlayerPosX && stargriddesty != PlayerPosY ) {
			if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
				gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace = NULL;
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->SelectedEntityInSpace && gameLocal.GetLocalPlayer()->SelectedEntityInSpace == this ) {
				if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
					//gameLocal.GetLocalPlayer()->guiOverlay = NULL;
					gameLocal.GetLocalPlayer()->HailGui->HandleNamedEvent("HailedSelectedShipHasWarpedAway");
				}
				gameLocal.GetLocalPlayer()->SelectedEntityInSpace->currently_in_hail = false;
				gameLocal.GetLocalPlayer()->SelectedEntityInSpace = NULL;
			}
		}
		// 01/20/13 - something like this might be appropriate: gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace = NULL;
		// this (or something like it) might be a good idea so if the ShipOnBoard warps away we will still be able to view it's diagram layout.(although it might make sense if we can't because the PLayerShip won't be in sensor range. Maybe we should disable most of the captain gui if the playership is in a different stargrid position:
		/*
			if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
				gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this
			}
		*/
		should_update_dynamic_lights = true;
		gameLocal.GetLocalPlayer()->allow_overlay_captain_gui = false;
		PostEventMS( &EV_DisplayStoryWindow, 2500 );

		GivePlayerAppropriatePDAEmailsAndVideosForStargridPosition(stargriddestx,stargriddesty);
	} else  {
		//If you are moving to a new stargrid position (not the PlayerShip or ShipOnBoard position - Check to see if it is full. If it is, cancel the warp.
		if ( gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard != this ) {
			// do nothing as of right now - I might add something here later. But maybe not. In wich case we can simplify this control flow a little more.
		}
		if ( stargridpositionx == PlayerPosX && stargridpositiony == PlayerPosY ) {
			idStr text_color;
			if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^1"; }
			if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^8"; }
			if ( team == gameLocal.GetLocalPlayer()->team ) { text_color = "^4"; }
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( text_color + "The " + name + " has left " + text_color + "local " + text_color + "space.^0");
		}
		if ( stargriddestx == PlayerPosX && stargriddesty == PlayerPosY ) {
			stargridpositionx = stargriddestx;
			stargridpositiony = stargriddesty;
			if ( MySkyPortalEnt ) {
				MySkyPortalEnt->occupied = false;
			}
			MySkyPortalEnt = NULL;
			if ( gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard != this ) {
				ClaimUnnoccupiedPlayerSkyPortalEntity();
			} else {
				ClaimUnnoccupiedSkyPortalEntity();
			}
			gameLocal.UpdateSpaceEntityVisibilities();
			PostEventMS( &EV_UpdateViewscreenCamera, 2000 );
			gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
			gameLocal.GetLocalPlayer()->PopulateShipList();

			idStr text_color;
			if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^1"; }
			if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^8"; }
			if ( team == gameLocal.GetLocalPlayer()->team ) { text_color = "^4"; }
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( text_color + "The " + name + " has arrived in " + text_color + "local " + text_color + "space.^0");
		} else {
			stargridpositionx = stargriddestx;
			stargridpositiony = stargriddesty;
			if ( MySkyPortalEnt ) {
				MySkyPortalEnt->occupied = false;
			}
			MySkyPortalEnt = NULL;
		//Run some kind of update to hide all ship entities whose stargrid positions do not equal the gameLocal.Player->ShipOnBoard->stargridpositions. Maybe make a gameLocal function.
			gameLocal.UpdateSpaceEntityVisibilities();
			PostEventMS( &EV_UpdateViewscreenCamera, 2000 );
			gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();

			if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
					gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace = NULL;
					//gameLocal.Printf( "warp_testing" );
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->SelectedEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->SelectedEntityInSpace == this ) {
					if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
						//gameLocal.GetLocalPlayer()->guiOverlay = NULL;
						gameLocal.GetLocalPlayer()->HailGui->HandleNamedEvent("HailedSelectedShipHasWarpedAway");
					}
					gameLocal.GetLocalPlayer()->SelectedEntityInSpace->currently_in_hail = false;
					gameLocal.GetLocalPlayer()->SelectedEntityInSpace = NULL;
				}
			}

			// BOYETTE NTOE TODO: these don't seem necessary as they are done below these again.
			gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
			gameLocal.GetLocalPlayer()->UpdateSelectedEntityInSpaceOnGuis();
			gameLocal.GetLocalPlayer()->UpdateHailGui();
			// if the playership is currently in a hail with a ship that is leaving its stargrid location then we need to exit the HailGui on the player.
			gameLocal.GetLocalPlayer()->PopulateShipList();

		}
	}


	stargridpositionx = stargriddestx;
	stargridpositiony = stargriddesty;

	ArrivingAtStargridPosition(stargriddestx,stargriddesty);

	if ( should_update_dynamic_lights ) {
		gameLocal.UpdateSpaceCommandDynamicLights();
	}

	gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();
	gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
	gameLocal.GetLocalPlayer()->UpdateSelectedEntityInSpaceOnGuis();
	gameLocal.GetLocalPlayer()->UpdateHailGui();
	gameLocal.GetLocalPlayer()->PopulateShipList();
	gameLocal.UpdateSunEntity();

	// empty the charge bar
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount = 0;
		consoles[ENGINESMODULEID]->ControlledModule->current_charge_percentage = 0;
	}
	// empty all charge bars - warping causes a loss of all energy charges
	ReduceAllModuleChargesToZero();

	weapons_autofire_on = false;
	torpedos_autofire_on = false;
	TargetEntityInSpace = NULL;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
	}

	//gameLocal.UpdateSpaceEntityVisibilities(); // BOYETTE NOTE: commented this out 08/15/2016 because it was already being run in every scenario up above.
	PostEventMS( &EV_UpdateViewscreenCamera, 2000 );

	if ( projectile.GetEntity() ) {
		projectile.GetEntity()->CancelEvents( &EV_CheckTorpedoStatus );
		projectile.GetEntity()->CancelEvents( &EV_Remove );
		projectile.GetEntity()->Event_Remove();
		projectile = NULL;
	}
	// cycle through ships, and cycle through skyportal nodes - at the inputted destination coordinates. Assign this ship a skyportal node appropriately.
	// return true;
	// SHIP AI BEGIN
	Event_RemoveMiniGoalAction(SHIP_AI_BOARD); // BOYETTE NOTE: these actions are local to the stargrid position - so we don't need them any more. New ones could be generated at the new stargrid postion.
	Event_RemoveMiniGoalAction(SHIP_AI_DEFEND_AGAINST); // BOYETTE NOTE: these actions are local to the stargrid position - so we don't need them any more. New ones could be generated at the new stargrid postion.
	Event_RemoveMiniGoalAction(SHIP_AI_ATTACK); // BOYETTE NOTE: these actions are local to the stargrid position - so we don't need them any more. New ones could be generated at the new stargrid postion.

	Event_GetATargetShipInSpace(); // so the ship has a target (if possible) at it's new stargrid position
	// SHIP AI END

	UpdateGuisOnTransporterPad();

	// BOYETTE MUSIC BEGIN
	// fade in the music:
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) { // if this is the player's ship on board but not the playership it will just go on playing the shiponboard music probably
		gameLocal.GetLocalPlayer()->DeterminePlayerMusic();
		/*
		gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_MUSIC,false);
		// SONG SELECTION BEGIN
		if ( ships_at_my_stargrid_position.size() == 0 ) {
			gameLocal.GetLocalPlayer()->currently_playing_music_shader = "default_music_long";
		} else if ( ships_at_my_stargrid_position.size() == 1 && ships_at_my_stargrid_position[0]->is_derelict ) {
			gameLocal.GetLocalPlayer()->currently_playing_music_shader = "derelict_music_long";
		} else {

			bool hostile_entities_present = false;
			for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				if ( !ships_at_my_stargrid_position[i]->is_derelict && ships_at_my_stargrid_position[i]->team != gameLocal.GetLocalPlayer()->team && !ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) ) {
					hostile_entities_present = true;
					break;
				}
			}

			if ( hostile_entities_present ) {
				std::vector<idStr>		potential_songs_to_play;
				int						most_common_song_count = 0;
				idStr					most_common_song;
				for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
					if ( !ships_at_my_stargrid_position[i]->is_derelict && ships_at_my_stargrid_position[i]->team != gameLocal.GetLocalPlayer()->team && !ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) ) {
						potential_songs_to_play.push_back(ships_at_my_stargrid_position[i]->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" ));
					}
				}
				for ( int i = 0; i < potential_songs_to_play.size(); i++ ) {
					int new_count = 0;
					new_count = std::count(potential_songs_to_play.begin(), potential_songs_to_play.end(), potential_songs_to_play[i]);
					if ( most_common_song_count < new_count && new_count > 1) {
						most_common_song_count = new_count;
						most_common_song = potential_songs_to_play[i];
					}
				}
				gameLocal.Printf( "\n" + most_common_song + "\n" );
				if ( most_common_song.IsEmpty() ) {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_songs_to_play[gameLocal.random.RandomInt(potential_songs_to_play.size()-1)];
				} else {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = most_common_song;
				}
			} else {
				//std::vector<idStr>		potential_songs_to_play;
				//for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				//	if ( ships_at_my_stargrid_position[i]->is_derelict ) {
				//		potential_songs_to_play.push_back("derelict_music_long");
				//	} else {
				//		potential_songs_to_play.push_back(ships_at_my_stargrid_position[i]->spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" ));
				//	}
				//}
				//gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_songs_to_play[gameLocal.random.RandomInt(potential_songs_to_play.size()-1)];
				std::vector<idStr>		potential_songs_to_play;
				int						most_common_song_count = 0;
				idStr					most_common_song;
				for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
					if ( ships_at_my_stargrid_position[i]->is_derelict ) {
						potential_songs_to_play.push_back("derelict_music_long");
					} else {
						potential_songs_to_play.push_back(ships_at_my_stargrid_position[i]->spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" ));
					}
				}
				for ( int i = 0; i < potential_songs_to_play.size(); i++ ) {
					int new_count = 0;
					new_count = std::count(potential_songs_to_play.begin(), potential_songs_to_play.end(), potential_songs_to_play[i]);
					if ( most_common_song_count < new_count && new_count > 1) {
						most_common_song_count = new_count;
						most_common_song = potential_songs_to_play[i];
					}
				}
				if ( most_common_song.IsEmpty() ) {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_songs_to_play[gameLocal.random.RandomInt(potential_songs_to_play.size()-1)];
				} else {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = most_common_song;
				}
			}
		}
		// SONG SELECTION END
		int song_length = 0;
		//gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound("bellumturan_battle_music_short"), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
		gameLocal.GetLocalPlayer()->StartSoundShader(declManager->FindSound(gameLocal.GetLocalPlayer()->currently_playing_music_shader), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
		//gameLocal.GetLocalPlayer()->currently_playing_music_shader = this->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
		gameLocal.GetLocalPlayer()->currently_playing_music_shader_begin_time = gameLocal.time;
		gameLocal.GetLocalPlayer()->currently_playing_music_shader_end_time = gameLocal.time + song_length;
		gameLocal.GetLocalPlayer()->music_shader_is_playing = true;
		gameLocal.GetLocalPlayer()->BeginMonitoringMusic();
		//gameSoundWorld->PlayShaderDirectly("bellumturan_battle_music_short",SND_CHANNEL_ANY);
		gameSoundWorld->FadeSoundClasses(2,0.0f,5.0f); // BOYETTE NOTE: music sounds are soundclass 2. 0.0f db is the default level.
		gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f); // fade the alarm sounds down while music is playing
		*/
	}
	// BOYETTE MUSIC END
}

void sbShip::LeavingStargridPosition(int leaving_pos_x,int leaving_pos_y) {
	// remove ourself from the other ship's vectors at our old stargrid position
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] ) {
			if ( ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->TargetEntityInSpace == this ) {
				for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
					if ( ships_at_my_stargrid_position[i]->consoles[x] && ships_at_my_stargrid_position[i]->consoles[x]->ControlledModule ) {
						ships_at_my_stargrid_position[i]->consoles[x]->ControlledModule->CurrentTargetModule = NULL;
					}
				}
				ships_at_my_stargrid_position[i]->TargetEntityInSpace = NULL;
				ships_at_my_stargrid_position[i]->TempTargetEntityInSpace = NULL;
			}
			ships_at_my_stargrid_position[i]->ships_at_my_stargrid_position.erase(std::remove(ships_at_my_stargrid_position[i]->ships_at_my_stargrid_position.begin(), ships_at_my_stargrid_position[i]->ships_at_my_stargrid_position.end(), this), ships_at_my_stargrid_position[i]->ships_at_my_stargrid_position.end());
		}
	}

	// check if other ships at the old stargrid position should try to be dormant
	bool at_player_shiponboard_sg_pos = false;
	bool at_other_active_ship_sg_pos = false;
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] && ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && ( gameLocal.GetLocalPlayer()->ShipOnBoard == ships_at_my_stargrid_position[i] ) )  ) {
			at_player_shiponboard_sg_pos = true;
		}
		if ( ships_at_my_stargrid_position[i] && !ships_at_my_stargrid_position[i]->try_to_be_dormant ) {
			at_other_active_ship_sg_pos = true;
		}
	}

	// BOYETTE BEGIN NOTE: added this 07 04 216 - probably not necessary but just for good measure:
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		at_player_shiponboard_sg_pos = false;
	}
	// BOYETTE END NOTE added this 07 04 216
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] ) {
			if ( ships_at_my_stargrid_position[i]->ship_is_never_dormant ) {
				ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
			} else {
				if ( ships_at_my_stargrid_position[i]->ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos ) {
					if ( at_other_active_ship_sg_pos ) {
						ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
					} else {
						ships_at_my_stargrid_position[i]->try_to_be_dormant = true;
					}
				} else if ( ships_at_my_stargrid_position[i]->ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos ) {
					if ( at_player_shiponboard_sg_pos ) {
						ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
					} else {
						ships_at_my_stargrid_position[i]->try_to_be_dormant = true;
					}
				} else if ( at_player_shiponboard_sg_pos ) {
					ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
				}
			}
		}
	}

	// and clear our own vector
	ships_at_my_stargrid_position.clear();
}

void sbShip::ArrivingAtStargridPosition(int arriving_pos_x,int arriving_pos_y) {
	// populate the ships_at_my_stargrid_position vector for all relavant ships
	for ( int i = 0; i < gameLocal.num_entities; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {
			if ( gameLocal.entities[ i ]->stargridpositionx == arriving_pos_x && gameLocal.entities[ i ]->stargridpositiony == arriving_pos_y && gameLocal.entities[ i ] != this ) {
				ships_at_my_stargrid_position.push_back( dynamic_cast<sbShip*>( gameLocal.entities[ i ] ) );
				dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->ships_at_my_stargrid_position.push_back(this);
			}
		}
	}

	// SHIP DISCOVERY BEGIN:
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		// check if any ships at our stargrid position are in the same region as the player shiponboard or the playership. - if it then it has been discovered by the player.
		if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == ships_at_my_stargrid_position[i]->stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == ships_at_my_stargrid_position[i]->stargridpositiony ) {
			ships_at_my_stargrid_position[i]->discovered_by_player = true;
		} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == ships_at_my_stargrid_position[i]->stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == ships_at_my_stargrid_position[i]->stargridpositiony ) {
			ships_at_my_stargrid_position[i]->discovered_by_player = true;
		}
	}
	// check if this ship is in the same region as the player shiponboard or the playership. - if it is then it has been discovered by the player.
	if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
		discovered_by_player = true;
	} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
		discovered_by_player = true;
	}
	// SHIP DISCOVERY END

	// check if should try to be dormant
	bool at_player_shiponboard_sg_pos = false;
	bool at_other_active_ship_sg_pos = false;

	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] && ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && ( gameLocal.GetLocalPlayer()->ShipOnBoard == ships_at_my_stargrid_position[i] || gameLocal.GetLocalPlayer()->ShipOnBoard == this ) )  ) {
			at_player_shiponboard_sg_pos = true;
		}
		if ( ships_at_my_stargrid_position[i] && !ships_at_my_stargrid_position[i]->try_to_be_dormant ) {
			at_other_active_ship_sg_pos = true;
		}
	}

	// check this ship
	if ( ship_is_never_dormant ) {
		try_to_be_dormant = false;
	} else {
		if ( ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos ) {
			if ( at_other_active_ship_sg_pos ) {
				try_to_be_dormant = false;
			} else {
				try_to_be_dormant = true;
			}
		} else if ( ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos ) {
			if ( at_player_shiponboard_sg_pos ) {
				try_to_be_dormant = false;
			} else {
				try_to_be_dormant = true;
			}
		} else if ( at_player_shiponboard_sg_pos ) {
			try_to_be_dormant = false;
		}
		if ( ship_is_dormant_until_awoken_by_player_shiponboard && at_player_shiponboard_sg_pos ) {
			ship_is_never_dormant = true;
			spawnArgs.SetBool("ship_is_never_dormant", "1");
			try_to_be_dormant = false;
		}
		if ( ship_is_dormant_until_awoken_by_an_active_ship && ( at_other_active_ship_sg_pos || at_player_shiponboard_sg_pos ) ) {
			ship_is_never_dormant = true;
			spawnArgs.SetBool("ship_is_never_dormant", "1");
			try_to_be_dormant = false;
		}
	}

	// check other ships at this ship's stargrid position
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i]->ship_is_never_dormant ) {
			ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
		} else {
			if ( ships_at_my_stargrid_position[i]->ship_tries_to_be_dormant_when_not_at_active_ship_sg_pos ) {
				if ( at_player_shiponboard_sg_pos ) {
					ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
				} else {
					ships_at_my_stargrid_position[i]->try_to_be_dormant = true;
				}
			} else if ( ships_at_my_stargrid_position[i]->ship_tries_to_be_dormant_when_not_at_player_shiponboard_sg_pos ) {
				if ( at_player_shiponboard_sg_pos ) {
					ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
				} else {
					ships_at_my_stargrid_position[i]->try_to_be_dormant = true;
				}
			} else if ( at_player_shiponboard_sg_pos ) {
				ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
			}
			if ( ships_at_my_stargrid_position[i]->ship_is_dormant_until_awoken_by_player_shiponboard && at_player_shiponboard_sg_pos ) {
				ships_at_my_stargrid_position[i]->ship_is_never_dormant = true;
				ships_at_my_stargrid_position[i]->spawnArgs.SetBool("ship_is_never_dormant", "1");
				ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
			}
			if ( ships_at_my_stargrid_position[i]->ship_is_dormant_until_awoken_by_an_active_ship && ( at_other_active_ship_sg_pos || at_player_shiponboard_sg_pos ) ) {
				ships_at_my_stargrid_position[i]->ship_is_never_dormant = true;
				ships_at_my_stargrid_position[i]->spawnArgs.SetBool("ship_is_never_dormant", "1");
				ships_at_my_stargrid_position[i]->try_to_be_dormant = false;
			}
		}
	}
}

void sbShip::GivePlayerAppropriatePDAEmailsAndVideosForStargridPosition( int stargriddestx, int stargriddesty) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetPDA() ) {
		idVec2 current_location(stargriddestx,stargriddesty);
		idVec2 final_location(MAX_STARGRID_X_POSITIONS,MAX_STARGRID_Y_POSITIONS);
		float dist;
		dist = ( final_location - current_location ).Length();
		gameLocal.Printf(idStr(dist) + "\n");

		if ( dist < 18.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_01") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_01");
			}
		}
		if ( dist < 17.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_02") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_02");
			}
		}
		if ( dist < 16.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_03") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_03");
			}
		}
		if ( dist < 15.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_04") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_04");
			}
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_04") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_04",NULL);
			}
		}
		if ( dist < 14.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_05") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_05");
			}
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_05") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_05",NULL);
			}
		}
		if ( dist < 13.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_06") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_06");
			}
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_06") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_06",NULL);
			}
		}
		if ( dist < 12.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_07") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_07",NULL);
			}
		}
		if ( dist < 11.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_08") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_08",NULL);
			}
		}
		if ( dist < 9.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_07") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_07");
			}
		}
		if ( dist < 8.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_09") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_09",NULL);
			}
		}
		if ( dist < 7.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_10") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_10",NULL);
			}
		}
		if ( dist < 6.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_11") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_11",NULL);
			}
		}
		if ( dist < 5.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_12") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_12",NULL);
			}
		}
		if ( dist < 4.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_13") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_13",NULL);
			}
		}
		if ( dist < 3.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.videos.Find("barnaby_log_14") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveVideo("barnaby_log_14",NULL);
				// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
				if ( common->m_pStatsAndAchievements ) {
					if ( !common->m_pStatsAndAchievements->m_nTimesAllBarnabyLogsAcquired ) {
						common->m_pStatsAndAchievements->m_nTimesAllBarnabyLogsAcquired++;
						common->StoreSteamStats();
					}
				}
#endif
				// BOYETTE STEAM INTEGRATION END
			}
		}
		if ( dist < 2.0 ) {
			if ( gameLocal.GetLocalPlayer()->inventory.emails.Find("fleet_command_email_15") == NULL ) {
				gameLocal.GetLocalPlayer()->GiveEmail("fleet_command_email_15");
			}
		}
	}
}

void sbShip::ClaimUnnoccupiedSkyPortalEntity() {
	int			i;
	idEntity	*ent;
	idPortalSky* PortalSkyToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPortalSky::Type )  ) {
			PortalSkyToCheck = dynamic_cast<idPortalSky*>( ent );
			//gameLocal.Printf( ent->name );
			if ( PortalSkyToCheck->occupied == false && !(idStr::FindText(ent->name.c_str(),"player",false)+1) && (idStr::FindText(PortalSkyToCheck->name.c_str(),"ship",false)+1) ) {
				MySkyPortalEnt = PortalSkyToCheck;
				MySkyPortalEnt->occupied = true;
				GetPhysics()->SetOrigin( MySkyPortalEnt->GetPhysics()->GetOrigin() + idVec3(0,0,-8)); // we want to offset this a bit so it looks like the phasers torpedoes are coming from under the viewscreen/window.
				//GetPhysics()->SetAxis(spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1")); // we want the ship to face it's spawn direction if it is not the playership or is not at the skyportal entity that contains the string "player".
				GetPhysics()->SetAxis(PortalSkyToCheck->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1")); // we want the ship to face it's spawn direction if it is not the playership or is not at the skyportal entity that contains the string "player".
				//Event_RestorePosition(void); // we want to do something like this just to be safe. - but events are usually private and I am hesitant to change this to public.
				UpdateVisuals();
				//gameLocal.Printf( ent->name );
				return;
			}
		}
	}
}

void sbShip::ClaimUnnoccupiedPlayerSkyPortalEntity() {

	int			i;
	idEntity	*ent;
	idPortalSky* PortalSkyToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPortalSky::Type )  ) {
			PortalSkyToCheck = dynamic_cast<idPortalSky*>( ent );
			if ( (idStr::FindText(ent->name.c_str(),"player",false)+1) ) {
				MySkyPortalEnt = PortalSkyToCheck;
				MySkyPortalEnt->occupied = true;
				GetPhysics()->SetOrigin( MySkyPortalEnt->GetPhysics()->GetOrigin() + idVec3(0,0,-8)); // we want to offset this a bit so it looks like the phasers torpedoes are coming from under the viewscreen/window.
				//GetPhysics()->SetAxis(spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1") * idAngles( 0, 180, 0 ).ToMat3());//idMat3(-.9,0,0,0,1,0,0,0,1)); // www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToEuler/program/index.htm - java applet - takes a long time to load but provides good visualization for rot matrices - as of 12/27/2012 - or you can just use this idAngles to Mat 3 conversion method.
				GetPhysics()->SetAxis(MySkyPortalEnt->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1"));//idMat3(-.9,0,0,0,1,0,0,0,1)); // www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToEuler/program/index.htm - java applet - takes a long time to load but provides good visualization for rot matrices - as of 12/27/2012 - or you can just use this idAngles to Mat 3 conversion method.
				UpdateVisuals();
				//gameLocal.Printf( ent->name );
				return;
			}
		}
	}
}


void sbShip::ClaimSpecifiedSkyPortalEntity( idPortalSky* skyportal_to_claim ) {
	if ( skyportal_to_claim && skyportal_to_claim->IsType( idPortalSky::Type )  ) {
		if ( skyportal_to_claim->occupied == false ) {
			MySkyPortalEnt = skyportal_to_claim;
			MySkyPortalEnt->occupied = true;
			GetPhysics()->SetOrigin( MySkyPortalEnt->GetPhysics()->GetOrigin() + idVec3(0,0,-8));
			GetPhysics()->SetAxis(skyportal_to_claim->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1"));
			UpdateVisuals();
			return;
		}
	}
}

/***********************************************************************

	Thinking
	
***********************************************************************/

/*
================
sbShip::Think
================
*/
void sbShip::Think( void ) {
	if ( CheckDormant() ) {
		return; // BOYETTE NOTE TODO: might not want to return here. Not sure if we want dormant ships to skip their think function. They might be able to.
	}

	RunPhysics(); // this is called at spawn so it doesn't have to be called every frame in ::Think() now that this is a thinking entity // if we ever want to make it animated or moveable we might want to enable it again
	//gameLocal.Printf( "Thinking.\n" );
	Present(); // this is called at spawn so it doesn't have to be called every frame in ::Think() now that this is a thinking entity // if we ever want to make it animated or moveable we might want to enable it again
	VerifyCrewMemberPointers();
	UpdateModuleCharges();
	CheckForModuleActions();
	EvaluateCurrentOxygenLevelAndOxygenActions();
	UpdateCurrentComputerModuleBuffModifiers();
	UpdateShipDoorHealths();
	UpdateGuisOnConsolesAndModules();
	UpdateGuisOnCaptainChair();
	UpdateViewScreenEntityVisuals();
	EvaluateSelfDestructSequenceTimer();
	UpdateBeamToEnt();
	UpdateGuisOnTransporterPadEveryFrame();
	HandleModuleAndConsoleGUITimeCommandsEvenIfGUIsNotActive();

	/* Do this tomorrow. - BOYETTE NOTE: I don't believe this is necessary anymore.
	if ( ShieldEntity ) {
		if ( gameLocal.isNewFrame ) {
			if ( shieldStrength <= 0 ) {
				ShieldEntity->Hide();
			} else {
				ShieldEntity->Show();
			}
		}
	}
	*/
}

/*
================
sbShip::CheckDormant
================
*/
bool sbShip::CheckDormant() {
	bool dormant;
	
	dormant = DoDormantTests();
	if ( dormant && !fl.isDormant ) {
		fl.isDormant = true;
		DormantBegin();
	} else if ( !dormant && fl.isDormant ) {
		fl.isDormant = false;
		DormantEnd();
	}

	return dormant;
}

/*
================
sbShip::DoDormantTests

On sbShip this just checks fl.neverDormant
================
*/
bool sbShip::DoDormantTests() {
	if ( fl.neverDormant ) {
		return false;
	} else {
		return true;
	}
}

/*
================
sbShip::VerifyCrewMemberPointers
================
*/
void sbShip::VerifyCrewMemberPointers() {
	bool should_be_derelict = true;
	bool full_compliment = true;

	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] ) {
			should_be_derelict = false;
			if ( crew[i]->was_killed ) {
				if ( i != CAPTAINCREWID ) { // The PayerShip won't ever have a captain because it is the player
					full_compliment = false;
				}
				crew[i] = NULL;
			}
		} else {
			if ( i != CAPTAINCREWID ) { // The PayerShip won't ever have a captain because it is the player
				full_compliment = false;
			}
		}
	}

	if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		should_be_derelict = false;

		// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
		if ( full_compliment ) {
			if ( common->m_pStatsAndAchievements ) {
				if ( !common->m_pStatsAndAchievements->m_nTimesAllCrewFilled ) {
					common->m_pStatsAndAchievements->m_nTimesAllCrewFilled++;
					common->StoreSteamStats();
				}
			}
		}
		if ( reserve_Crew.size() >= max_reserve_crew ) {
			if ( common->m_pStatsAndAchievements ) {
				if ( !common->m_pStatsAndAchievements->m_nTimesAllReserveCrewFilled ) {
					common->m_pStatsAndAchievements->m_nTimesAllReserveCrewFilled++;
					common->StoreSteamStats();
				}
			}
		}
#endif
		// BOYETTE STEAM INTEGRATION END
	}

	if ( should_be_derelict && !is_derelict && !ship_destruction_sequence_initiated && !never_derelict ) { // if the ship destruction sequence is already intiated - that might kill all the crew - but we don't want to become derelict in that short period of time before the ship explodes
		BecomeDerelict();
		gameLocal.GetLocalPlayer()->PopulateShipList();
	}
}

/*
================
sbShip::UpdateModuleCharges
================
*/
void sbShip::UpdateModuleCharges() {
	//gameLocal.Printf( "Thinking.\n" );
	// DONE: BOYETTE NOTE TODO: need to put something here using gameLocal.time because right now it is based on frame rate which is obviously not what we want.
	if ( gameLocal.time >= update_module_charges_timer + 50 ) { // every one twentieth of a second.
		for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( consoles[i] && consoles[i]->ControlledModule  ) {
				consoles[i]->ControlledModule->UpdateChargeAmount();
			}
		}
		// BOYETTE DISABLED SHIELDS MODULE SUBTRACTION BEGIN
		if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule && ( shieldStrength > 0 && shieldStrength_copy > 0 ) && consoles[SHIELDSMODULEID]->ControlledModule && consoles[SHIELDSMODULEID]->ControlledModule->health < 0 ) {
			// Do a shield subtraction cycle if the shields module is disabled.
			shieldStrength_copy -= 2; //(shields_repair_per_cycle / 2);
			shieldStrength_copy = idMath::ClampInt(0,max_shieldStrength,shieldStrength_copy);
			//if ( shields_raised ) { // BOYTTE NOTE: I don't think we need this here
				shieldStrength -= 2; //(shields_repair_per_cycle / 2);
				shieldStrength = idMath::ClampInt(0,max_shieldStrength,shieldStrength);
				shieldStrength_copy = shieldStrength;
			//}
		}
		// BOYETTE DISABLED SHIELDS MODULE SUBTRACTION END
		update_module_charges_timer = gameLocal.time;
	} 
}

/*
================
sbShip::CheckForModuleActions
================
*/
void sbShip::CheckForModuleActions() {
	//gameLocal.Printf( "Thinking.\n" );
	/*
	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule  ) {
		consoles[MEDICALMODULEID]->ControlledModule->UpdateChargeAmount();
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule  ) {
		consoles[ENGINESMODULEID]->ControlledModule->UpdateChargeAmount();
	}
	*/

	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule && consoles[WEAPONSMODULEID]->ControlledModule->current_charge_amount >= consoles[WEAPONSMODULEID]->ControlledModule->max_charge_amount && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->allow_overlay_captain_gui ) { // and weapons is set to not fire
		
		if ( weapons_autofire_on ) {
			CheckWeaponsTargetQueue();
		}

		if ( consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule ) {
			// BOYETTE PRINT BEGIN
			//idFile *file;	
			//file = fileSystem->OpenFileAppend("file_write_test.txt");
			//file->Printf( name + ": CheckForModuleActions: weapons volley\n");
			//fileSystem->CloseFile( file );
			// BOYETTE PRINT END
			// BOYETTE NOTE BEGIN: added 05 21 2016 - so that nobody shoots at each other when a hail with no action hail mode should happen.
			bool should_wait_for_hails = false;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->should_hail_the_playership && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->should_go_into_no_action_hail_mode_on_hail ) {
				should_wait_for_hails = true;
			}
			if ( TargetEntityInSpace && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip && should_hail_the_playership && should_go_into_no_action_hail_mode_on_hail ) {
				should_wait_for_hails = true;
			}
			bool everyone_is_warped_in = true;
			if ( (TargetEntityInSpace && TargetEntityInSpace->should_warp_in_when_first_encountered) || should_warp_in_when_first_encountered ) {
				everyone_is_warped_in = false;
			}
			// BOYETTE NOTE END added 05 21 2016
			// fire a weapons volley.
			if ( TargetEntityInSpace && beam && beamTarget && !should_wait_for_hails && everyone_is_warped_in ) {
				if ( TargetEntityInSpace->consoles[ENGINESMODULEID] && TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule ) {
					float engines_efficiency = (float)TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->module_efficiency/100.00f;
					float max_hull_ratio = (float)TargetEntityInSpace->max_hullStrength/(float)MAX_MAX_HULLSTRENGTH;
					//float miss_chance = (engines_efficiency * 0.35f) - (max_hull_ratio * 0.25f);
					float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * (TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->max_power * 0.03) ) * (1.0f - max_hull_ratio)); // OLD BEFORE BALANCING: float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * 0.30f) * (1.0f - max_hull_ratio));
					miss_chance = idMath::ClampFloat( 0.0f, 0.70f, miss_chance );
					weapons_shot_missed = gameLocal.random.RandomFloat() < miss_chance; // TODO: calculate evasion chance based on the module efficiency and the size of the ship(using hullstrength here - maybe later we can add a Mass spawnarg to the ship)
					//weapons_shot_missed = gameLocal.random.RandomFloat() < 0.75f;
					//gameLocal.Printf( name + " Weapons miss chance: %f \n", miss_chance );
				} else {
					weapons_shot_missed = false;
				}

				ship_is_firing_weapons = true;
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					beam->SetOrigin( GetPhysics()->GetOrigin() );
					if ( weapons_shot_missed ) {
						beamTarget->SetOrigin( ReturnSuitableWeaponsOrTorpedoTargetPointForMiss() );
					} else {
						beamTarget->SetOrigin( TargetEntityInSpace->GetPhysics()->GetOrigin() );
					}
					beam->Show();
					beamTarget->Show();
				}
				PostEventMS( &EV_UpdateBeamVisibility, 350 ); // the beam will show up for a second and then disappear - this will make it look like a phaser. // this sets ship_is_firing_weapons to false so we can keep it even if not at the the same stargrid position

				consoles[WEAPONSMODULEID]->ControlledModule->current_charge_amount = 0;
				// then set an event HERE to hide the beam after a short time. - so it appears to be a phaser.
				if ( !weapons_shot_missed ) {
					DealShipToShipDamageWithWeapons(); // This will immediately damage the targeted ship. - it's shields,hull, and the targeted module.
				} else {
					// SHIP TO SHIP NOTIFICATION BEGIN
					if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
						idStr TargetEntityInSpaceColoredString;
						if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->PlayerShip->team ) {
							TargetEntityInSpaceColoredString = "^4" + TargetEntityInSpace->original_name;
						} else {
							TargetEntityInSpaceColoredString = "^1" + TargetEntityInSpace->original_name;
						}
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "^2" "Your ship ^0missed the " + TargetEntityInSpaceColoredString + ".");
					} else {
						idStr this_ship_text_color;
						if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^1"; }
						if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^8"; }
						if ( team == gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^4"; }
						idStr target_ship_text_color;
						if ( !TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^1"; }
						if ( TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^8"; }
						if ( TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^4"; }
						if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) {
							target_ship_text_color = "^2";
						}
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "The " + this_ship_text_color + original_name + "^0" " missed the " + target_ship_text_color + TargetEntityInSpace->original_name + ".");
					}
					// SHIP TO SHIP NOTIFICATION END
				}
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_weapons_launch", "spaceship_weapons_launch_snd_default" ) ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				} else if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_weapons_launch_in_space", "spaceship_weapons_launch_in_space_snd_default" ) ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == TargetEntityInSpace ) {
					if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
						if ( weapons_shot_missed ) {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_weapons_attack_x", gameLocal.random.RandomInt(1500) + 400 );
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_weapons_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_weapons_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_weapons_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->CaptainGui->SetStateString("targetship_incoming_weapons_material",spawnArgs.GetString("capgui_targetship_weap_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/TargetShipIncomingDefaultWeapons.tga") );
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingWeaponsAttack");
					}
				}
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
					// BOYETTE NOTE BEGIN: added 05 21 2016 so that if the playership is not targeting anything and it is fired upon that it targets something.
					if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this && !gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship ) {
						bool need_to_update_captain_gui = false;
						if ( gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID] && gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule && !gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule ) {
							gameLocal.GetLocalPlayer()->PlayerShip->weapons_autofire_on = true;
							gameLocal.GetLocalPlayer()->PlayerShip->CheckWeaponsTargetQueue();
							need_to_update_captain_gui = true;
						}
						if ( gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID] && gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule && !gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
							gameLocal.GetLocalPlayer()->PlayerShip->torpedos_autofire_on = true;
							gameLocal.GetLocalPlayer()->PlayerShip->CheckTorpedosTargetQueue();
							need_to_update_captain_gui = true;
						}
						if ( need_to_update_captain_gui ) {
							gameLocal.GetLocalPlayer()->UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
						}
					}
					// BOYETTE NOTE END: added 05 21 2016 so that if the playership is not targeting anything and it is fired upon that it targets something.
					if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
						if ( weapons_shot_missed ) {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_weapons_attack_x", gameLocal.random.RandomInt(1500) + 200 );
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_weapons_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_weapons_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_weapons_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->CaptainGui->SetStateString("playership_incoming_weapons_material",spawnArgs.GetString("capgui_playership_weap_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/PlayerShipIncomingDefaultWeapons.tga") );
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingWeaponsAttack");
					}
				}
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == TargetEntityInSpace ) {
					ship_that_just_fired_at_us = this;
					gameLocal.UpdateSpaceCommandViewscreenCamera();
					PostEventMS( &EV_UpdateViewscreenCamera, 1500 ); // to reset the camera to normal again after 1500 ms

					if ( gameLocal.GetLocalPlayer()->hud ) {
						if ( weapons_shot_missed ) {
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_weapons_attack_x", gameLocal.random.RandomInt(1500) + 200 );
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_weapons_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_weapons_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_weapons_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->hud->SetStateString("shiponboard_incoming_weapons_material",spawnArgs.GetString("capgui_playership_weap_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/PlayerShipIncomingDefaultWeapons.tga") );
						gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingWeaponsAttack");
					}
				}
			}
		}
	}

	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule && consoles[TORPEDOSMODULEID]->ControlledModule->current_charge_amount >= consoles[TORPEDOSMODULEID]->ControlledModule->max_charge_amount && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->allow_overlay_captain_gui ) { // and torpedos is set to not fire
		
		if ( torpedos_autofire_on ) {
			CheckTorpedosTargetQueue();
		}

		if ( consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
			// BOYETTE PRINT BEGIN
			//idFile *file;	
			//file = fileSystem->OpenFileAppend("file_write_test.txt");
			//file->Printf( name + ": CheckForModuleActions: torpedos volley\n");
			//fileSystem->CloseFile( file );
			// BOYETTE PRINT END
			// fire a torpedo volley.
			// BOYETTE NOTE BEGIN: added 05 21 2016 - so that nobody shoots at each other when a hail with no action hail mode should happen.
			bool should_wait_for_hails = false;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->should_hail_the_playership && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->should_go_into_no_action_hail_mode_on_hail ) {
				should_wait_for_hails = true;
			}
			if ( TargetEntityInSpace && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip && should_hail_the_playership && should_go_into_no_action_hail_mode_on_hail ) {
				should_wait_for_hails = true;
			}
			bool everyone_is_warped_in = true;
			if ( (TargetEntityInSpace && TargetEntityInSpace->should_warp_in_when_first_encountered) || should_warp_in_when_first_encountered ) {
				everyone_is_warped_in = false;
			}
			// BOYETTE NOTE END added 05 21 2016
			if ( TargetEntityInSpace && !should_wait_for_hails && everyone_is_warped_in ) {

				if ( projectile.GetEntity() ) {
					projectile.GetEntity()->CancelEvents( &EV_CheckTorpedoStatus ); // BOYETTE NOTE TODO: make sure this is correct for all other instances - the projectile shouldn't have this even.
					CancelEvents( &EV_CheckTorpedoStatus );
					projectile = NULL; // BOYETTE NOTE IMPORTANT: Added 02/25/2016 - so that if the old projectile is still flying around because we missed the last shot it will not cause problems.
					if ( !torpedo_shot_missed ) { // BOYETTE NOTE IMPORTANT: Added 02/25/2016 - if there was still a projectile and it did not miss - than just cause damage
						Event_CheckTorpedoStatus();
					}
				}
				if ( torpedo_shot_missed ) {
					Event_CheckTorpedoStatus(); // BOYETTE NOTE IMPORTANT: Added 02/25/2016 - so that if the old projectile is still flying around because we missed the last shot it will not cause problems.
				}
				if ( TargetEntityInSpace->consoles[ENGINESMODULEID] && TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule ) {
					float engines_efficiency = (float)TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->module_efficiency/100.00f;
					float max_hull_ratio = (float)TargetEntityInSpace->max_hullStrength/(float)MAX_MAX_HULLSTRENGTH;
					//float miss_chance = (engines_efficiency * 0.35f) - (max_hull_ratio * 0.25f);
					float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * (TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->max_power * 0.03) ) * (1.0f - max_hull_ratio)); // OLD BEFORE BALANCING: float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * 0.30f) * (1.0f - max_hull_ratio));
					miss_chance = idMath::ClampFloat( 0.0f, 0.70f, miss_chance );
					torpedo_shot_missed = gameLocal.random.RandomFloat() < miss_chance; // TODO: calculate evasion chance based on the module efficiency and the size of the ship(using hullstrength here - maybe later we can add a Mass spawnarg to the ship)
					//torpedo_shot_missed = gameLocal.random.RandomFloat() < 0.75f;
					//gameLocal.Printf( name + " Torpedo miss chance: %f \n", miss_chance );
				} else {
					torpedo_shot_missed = false;
				}

				ship_is_firing_torpedo = true;
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					if ( torpedo_shot_missed ) {
						idVec3 torpedoTargetPoint = ReturnSuitableWeaponsOrTorpedoTargetPointForMiss();
						CreateProjectile(GetPhysics()->GetOrigin(),torpedoTargetPoint);
						projectile.GetEntity()->Launch(GetPhysics()->GetOrigin(),(torpedoTargetPoint - GetPhysics()->GetOrigin()).ToAngles().ToForward(), vec3_origin); // THIS WORKS GREAT.
					} else {
						idVec3 torpedoLaunchPoint = ReturnSuitableTorpedoLaunchPoint();
						CreateProjectile(torpedoLaunchPoint,TargetEntityInSpace->GetPhysics()->GetOrigin());
						projectile.GetEntity()->Launch(torpedoLaunchPoint,(TargetEntityInSpace->GetPhysics()->GetOrigin() - torpedoLaunchPoint).ToAngles().ToForward(), vec3_origin); // THIS WORKS GREAT.
					}
				}

				PostEventMS( &EV_CheckTorpedoStatus, 50 ); // This is an event that will keep checking the projectile and deal the damage when the projectile is destroyed/hits its target. // this sets ship_is_firing_torpedo to false so we can keep it even if not at the the same stargrid position

				if ( torpedo_shot_missed ) {
					// SHIP TO SHIP NOTIFICATION BEGIN
					if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
						idStr TargetEntityInSpaceColoredString;
						if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->PlayerShip->team ) {
							TargetEntityInSpaceColoredString = "^4" + TargetEntityInSpace->original_name;
						} else {
							TargetEntityInSpaceColoredString = "^1" + TargetEntityInSpace->original_name;
						}
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "^2" "Your ship ^0missed the " + TargetEntityInSpaceColoredString + ".");
					} else {
						idStr this_ship_text_color;
						if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^1"; }
						if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^8"; }
						if ( team == gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^4"; }
						idStr target_ship_text_color;
						if ( !TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^1"; }
						if ( TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^8"; }
						if ( TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^4"; }
						if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) {
							target_ship_text_color = "^2";
						}
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "The " + this_ship_text_color + original_name + "^0" " missed the " + target_ship_text_color + TargetEntityInSpace->original_name + ".");
					}
					// SHIP TO SHIP NOTIFICATION END
				}
				consoles[TORPEDOSMODULEID]->ControlledModule->current_charge_amount = 0;
				// and also put logic here to damage the targeted ship. - it's shields,hull, and the targeted module.
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_torpedos_launch", "spaceship_torpedos_launch_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				} else if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_torpedos_launch_in_space", "spaceship_torpedos_launch_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == TargetEntityInSpace ) {
					if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
						if ( torpedo_shot_missed ) {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_x", 2200 );
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->CaptainGui->SetStateString("targetship_incoming_torpedos_material",spawnArgs.GetString("capgui_targetship_torp_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/TargetShipIncomingDefaultTorpedos.tga") );
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingTorpedosAttack");
					}
				}
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
					// BOYETTE NOTE BEGIN: added 05 21 2016 so that if the playership is not targeting anything and it is fired upon that it targets something.
					if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this && !gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship ) {
						bool need_to_update_captain_gui = false;
						if ( gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID] && gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule && !gameLocal.GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule ) {
							gameLocal.GetLocalPlayer()->PlayerShip->weapons_autofire_on = true;
							gameLocal.GetLocalPlayer()->PlayerShip->CheckWeaponsTargetQueue();
							need_to_update_captain_gui = true;
						}
						if ( gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID] && gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule && !gameLocal.GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
							gameLocal.GetLocalPlayer()->PlayerShip->torpedos_autofire_on = true;
							gameLocal.GetLocalPlayer()->PlayerShip->CheckTorpedosTargetQueue();
							need_to_update_captain_gui = true;
						}
						if ( need_to_update_captain_gui ) {
							gameLocal.GetLocalPlayer()->UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
						}
					}
					// BOYETTE NOTE END: added 05 21 2016 so that if the playership is not targeting anything and it is fired upon that it targets something.
					if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
						if ( torpedo_shot_missed ) {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_x", -300 );
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->CaptainGui->SetStateString("playership_incoming_torpedos_material",spawnArgs.GetString("capgui_playership_torp_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/PlayerShipIncomingDefaultTorpedos.tga") );
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingTorpedosAttack");
					}
				}
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == TargetEntityInSpace ) {
					TargetEntityInSpace->ship_that_just_fired_at_us = this;
					gameLocal.UpdateSpaceCommandViewscreenCamera();
					PostEventMS( &EV_UpdateViewscreenCamera, 1500 ); // to reset the camera to normal again after 1500 ms

					if ( gameLocal.GetLocalPlayer()->hud ) {
						if ( torpedo_shot_missed ) {
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_x", -300 );
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_y", gameLocal.random.RandomInt(2500) + 100 );
						} else {
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
							gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
						}
						gameLocal.GetLocalPlayer()->hud->SetStateString("shiponboard_incoming_torpedos_material",spawnArgs.GetString("capgui_playership_torp_image","guis/assets/steve_captain_display/ShipToShipAttackBackgrounds/PlayerShipIncomingDefaultTorpedos.tga") );
						gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingTorpedosAttack");
					}
				}
			}
		}
	}

	if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule && ( shieldStrength < max_shieldStrength && shieldStrength_copy < max_shieldStrength ) && consoles[SHIELDSMODULEID]->ControlledModule->current_charge_amount >= consoles[SHIELDSMODULEID]->ControlledModule->max_charge_amount ) {
		// Do a shield charge cycle.
		consoles[SHIELDSMODULEID]->ControlledModule->current_charge_amount = 0;
		shieldStrength_copy += shields_repair_per_cycle;
		shieldStrength_copy = idMath::ClampInt(0,max_shieldStrength,shieldStrength_copy);
		if ( shields_raised ) {
			shieldStrength += shields_repair_per_cycle;
			shieldStrength = idMath::ClampInt(0,max_shieldStrength,shieldStrength);
			shieldStrength_copy = shieldStrength;
		}
		if ( shieldStrength > 0 && ShieldEntity && ShieldEntity->IsHidden() && !IsHidden() ) {
			ShieldEntity->Show(); // if there is any shield strength left, make sure the shield is not hidden.
			if ( ShieldEntity && ShieldEntity->GetPhysics() ) {
				ShieldEntity->GetPhysics()->SetContents( CONTENTS_SOLID ); // set solid
			}
		}
	}
	
	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule && consoles[MEDICALMODULEID]->ControlledModule->current_charge_amount >= consoles[MEDICALMODULEID]->ControlledModule->max_charge_amount ) {
		// heal all friendly crew members on the ship.
		// and possibly have an area object in the medical room that if stood on will heal the player.
		// BOYETTE NOTE TODO - STILL NEED A HEALING SOUND EFFECT FOR THE PLAYER HERE.

		// THIS VERSION HEALS ALL AI ENTITIES ON THE SHIP OF THE SAME TEAM (and the player):
		/*
		idEntity	*ent;
		idAI		*CrewMemberToHealDamage;
		int i;
		for ( i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ent->IsType(idAI::Type) ) {
				CrewMemberToHealDamage = dynamic_cast<idAI*>(ent);
				if ( CrewMemberToHealDamage->ShipOnBoard && CrewMemberToHealDamage->ShipOnBoard == this && CrewMemberToHealDamage->health < CrewMemberToHealDamage->entity_max_health ) {
					CrewMemberToHealDamage->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
					CrewMemberToHealDamage->health = idMath::ClampInt(-100,CrewMemberToHealDamage->entity_max_health,CrewMemberToHealDamage->health); // this isn't necessary any more - it happens directly within the idActor::damage function.
				}
			}
		}
		if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this && gameLocal.GetLocalPlayer()->health < gameLocal.GetLocalPlayer()->entity_max_health && gameLocal.GetLocalPlayer()->team == team  ) {
			gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			//gameLocal.GetLocalPlayer()->idEntity::Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			gameLocal.GetLocalPlayer()->health = idMath::ClampInt(-100,gameLocal.GetLocalPlayer()->entity_max_health,gameLocal.GetLocalPlayer()->health); // this isn't necessary any more - it happens directly within the idPlayer::damage function.
		}
		consoles[MEDICALMODULEID]->ControlledModule->current_charge_amount = 0;
		*/
		// OLD VERSION HEAL END

		// THIS VERSION HEALS JUST THE CREW MEMBERS AND THE PLAYER IF ON THE SAME TEAM AS THE SHIP.
		/*
		bool medical_module_heal_cycle_occured = false;

		idStr	ship_heal_emitter_def_name = spawnArgs.GetString("heal_actor_emitter_def", "heal_actor_emitter_def_default");

		for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] ) {
				if ( !crew[i]->was_killed && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this && crew[i]->health < crew[i]->entity_max_health && crew[i]->health > 0 ) {
					crew[i]->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
					crew[i]->health = idMath::ClampInt(-100,crew[i]->entity_max_health,crew[i]->health);
					medical_module_heal_cycle_occured = true;
					if ( crew[i]->health >= crew[i]->entity_max_health && crew[i]->GetModelDefHandle() != -1 ) {
						gameRenderWorld->RemoveDecals( crew[i]->GetModelDefHandle() ); // get rid of damage decals
					}
					if ( crew[i]->ShipOnBoard == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
						crew[i]->TriggerHealFX(ship_heal_emitter_def_name);
					}
				}
			}
		}

		if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this && gameLocal.GetLocalPlayer()->health < gameLocal.GetLocalPlayer()->entity_max_health && gameLocal.GetLocalPlayer()->team == team && gameLocal.GetLocalPlayer()->health > 0 ) {
			gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			//gameLocal.GetLocalPlayer()->idEntity::Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			gameLocal.GetLocalPlayer()->health = idMath::ClampInt(-100,gameLocal.GetLocalPlayer()->entity_max_health,gameLocal.GetLocalPlayer()->health); // this isn't necessary any more - it happens directly within the idPlayer::damage function.
			medical_module_heal_cycle_occured = true;
			gameLocal.GetLocalPlayer()->HealFX.GetEntity()->Event_Remove(); // This is necessary on the player for some reason. It is not necessary for any other idActor's.
			gameLocal.GetLocalPlayer()->TriggerHealFX(ship_heal_emitter_def_name);
		}

		if ( medical_module_heal_cycle_occured ) {
			consoles[MEDICALMODULEID]->ControlledModule->current_charge_amount = 0;
		}
		*/
		// BEST VERSION: If any crew is damaged(or if this is the playership - the player) it will heal them first - whoever has the lowest health - then if the crew(or if this is the playership - the player possibly) is at full health it will check for any allies to heal.
		bool medical_module_heal_cycle_occured = false;

		idStr ship_heal_emitter_def_name = spawnArgs.GetString("heal_actor_emitter_def", "heal_actor_emitter_def_default");

		float lowest_health_percentage = idMath::INFINITY;
		idAI* BestAItoHeal = NULL;
		bool heal_the_player = false;
		// First - check our crew(and the player if this is the playership) - we treat the player equal to the crew - whoever has the lowest health percentage will get the heal
		for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] ) {
				if ( !crew[i]->was_killed && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this && crew[i]->health < crew[i]->entity_max_health && crew[i]->health > 0 ) {
					float health_percentage = ((float)crew[i]->health / (float)crew[i]->entity_max_health);
					if ( health_percentage < lowest_health_percentage ) {
						lowest_health_percentage = health_percentage;
						BestAItoHeal = crew[i];
					}
				}
			}
		}
		float player_health_percentage = ((float)gameLocal.GetLocalPlayer()->health / (float)gameLocal.GetLocalPlayer()->entity_max_health);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->ShipOnBoard == this && gameLocal.GetLocalPlayer()->health < gameLocal.GetLocalPlayer()->entity_max_health && gameLocal.GetLocalPlayer()->team == team && gameLocal.GetLocalPlayer()->health > 0 && player_health_percentage <= lowest_health_percentage ) {
			heal_the_player = true;
		}
		// Second - if our crew (and the player if this is the playership) is at full health - check for friendly allies on board to heal
		if ( BestAItoHeal == NULL && heal_the_player == false ) {
			for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
				if ( AIsOnBoard[i] ) {
					if ( !AIsOnBoard[i]->was_killed && AIsOnBoard[i]->ShipOnBoard && AIsOnBoard[i]->ShipOnBoard == this && AIsOnBoard[i]->health < AIsOnBoard[i]->entity_max_health && AIsOnBoard[i]->health > 0 && AIsOnBoard[i]->team == team ) {
						float health_percentage = ((float)AIsOnBoard[i]->health / (float)AIsOnBoard[i]->entity_max_health);
						if ( health_percentage < lowest_health_percentage ) {
							lowest_health_percentage = health_percentage;
							BestAItoHeal = AIsOnBoard[i];
						}
					}
				}
			}
		}

		// Third - do the heal on the entity that was determined above
		if ( heal_the_player && gameLocal.GetLocalPlayer()->health > 0 ) {
			gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			//gameLocal.GetLocalPlayer()->idEntity::Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			gameLocal.GetLocalPlayer()->health = idMath::ClampInt(-100,gameLocal.GetLocalPlayer()->entity_max_health,gameLocal.GetLocalPlayer()->health); // this isn't necessary any more - it happens directly within the idPlayer::damage function.
			medical_module_heal_cycle_occured = true;
			if ( gameLocal.GetLocalPlayer()->HealFX.GetEntity() ) {
				gameLocal.GetLocalPlayer()->HealFX.GetEntity()->Event_Remove(); // This is necessary on the player for some reason. It is not necessary for any other idActor's.
				gameLocal.GetLocalPlayer()->HealFX = NULL;
			}
			gameLocal.GetLocalPlayer()->TriggerHealFX(ship_heal_emitter_def_name);
		} else if ( BestAItoHeal ) {
			BestAItoHeal->Damage(this,this,idVec3(0,0,0),"damage_heal_spaceship_medical_module",1.0f,INVALID_JOINT);
			BestAItoHeal->health = idMath::ClampInt(-100,BestAItoHeal->entity_max_health,BestAItoHeal->health);
			medical_module_heal_cycle_occured = true;
			if ( BestAItoHeal->health >= BestAItoHeal->entity_max_health && BestAItoHeal->GetModelDefHandle() != -1 ) {
				gameRenderWorld->RemoveDecals( BestAItoHeal->GetModelDefHandle() ); // get rid of damage decals
			}
			if ( BestAItoHeal->ShipOnBoard == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
				BestAItoHeal->TriggerHealFX(ship_heal_emitter_def_name);
			}
		}

		// Fourth - use up the charge if we healed something
		if ( medical_module_heal_cycle_occured ) {
			consoles[MEDICALMODULEID]->ControlledModule->current_charge_amount = 0;
		}

	}

	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->current_charge_amount >= consoles[ENVIRONMENTMODULEID]->ControlledModule->max_charge_amount ) { // DONE: maybe only if security module module_efficiency > 0 - so that a charge is not wasted on nothing.
		current_oxygen_level++;
		current_oxygen_level = idMath::ClampInt(0,100,current_oxygen_level);
		consoles[ENVIRONMENTMODULEID]->ControlledModule->current_charge_amount = 0;
	}

	if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule && consoles[SECURITYMODULEID]->ControlledModule->current_charge_amount >= consoles[SECURITYMODULEID]->ControlledModule->max_charge_amount  && consoles[SECURITYMODULEID]->ControlledModule->module_efficiency > 0 ) {
		int earliest_door_disabled_time = gameLocal.time;
		idEntity* EarliestDoorDisabled = NULL;
		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) ) {
				if ( shipdoors[ i ].GetEntity()->health < (int)(shipdoors[ i ].GetEntity()->entity_max_health * ( (float)consoles[SECURITYMODULEID]->ControlledModule->module_efficiency / 100.00f )) && dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->door_disabled_time + 7000 <= earliest_door_disabled_time && dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->door_damaged_time + 2500 < gameLocal.time ) { //  Can make 2500 a constant later on - might want to make it 700 instead just to be consistent - call it maybe MAXIMUM_TIME_BETWEEN_AI_DOOR_BREAKING_ATTACKS. might want to heal the door if its health is less than the amount calculated from security module efficiency instead of a health of <= 0  - although we definitely want to prioritize disabled doors over damaged ones // the 7000 is so the door is not immediately repaired. Once they disable it they should have a chance to walk through it even if the door module is fully charged.
					earliest_door_disabled_time = dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->door_disabled_time;
					EarliestDoorDisabled = shipdoors[ i ].GetEntity();
				}
			}
		}
		if ( EarliestDoorDisabled ) {
			if ( EarliestDoorDisabled->IsType( idDoor::Type ) ) {
				dynamic_cast<idDoor*>( EarliestDoorDisabled )->SetDoorGroupsHealth(EarliestDoorDisabled->entity_max_health * ( (float)consoles[SECURITYMODULEID]->ControlledModule->module_efficiency / 100.00f )); // might need to use something other than module efficiency because if it is zero nothing will happen here.
				dynamic_cast<idDoor*>( EarliestDoorDisabled )->UpdateDoorGroupsMoverStatusShaderParms();
			}
			consoles[SECURITYMODULEID]->ControlledModule->current_charge_amount = 0;
		}
	}
	
}
// Does Damage to shields first and then to Hull/CurrentTargetModule.
// Does more damage to the Hull/CurrentTargetModule as the shields are depleted.
void sbShip::DealShipToShipDamageWithWeapons() {
	if ( !ship_destruction_sequence_initiated ) {

		int shieldDamageToDeal = 0;
		int hullDamageToDeal = 0;

		if ( TargetEntityInSpace && consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule && consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule) {
			shieldDamageToDeal = idMath::Rint((float)weapons_damage_modifier * ((float)TargetEntityInSpace->shieldStrength / (float)TargetEntityInSpace->max_shieldStrength));
			TargetEntityInSpace->shieldStrength = TargetEntityInSpace->shieldStrength - shieldDamageToDeal;
			hullDamageToDeal = weapons_damage_modifier - shieldDamageToDeal;
			if ( g_enableReducedHullDamage.GetBool() ) {
				if ( TargetEntityInSpace->is_derelict ) {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal * (int)2 );
				} else {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal / (int)2 );
				}
			} else {
				if ( TargetEntityInSpace->is_derelict ) {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal * (int)4 ); // BOYETTE NOTE: this is the original unreduced hull damage.
				} else {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - hullDamageToDeal; // BOYETTE NOTE: this is the original unreduced hull damage.
				}
			}
			consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule->RecieveShipToShipDamage( this, hullDamageToDeal );
			TargetEntityInSpace->shieldStrength = idMath::ClampInt(0,TargetEntityInSpace->max_shieldStrength,TargetEntityInSpace->shieldStrength);
			if ( TargetEntityInSpace->shields_raised ) {
				TargetEntityInSpace->shieldStrength_copy = TargetEntityInSpace->shieldStrength;
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				TargetEntityInSpace->friendlinessWithPlayer = 0; // need to put a check in here. If a ship is attacked by the playership, then it's relationship with the player should be set to zero and it should become hostile. We still need to do the become hostile logic with the teams and/or the sbShip AI.
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
				gameLocal.GetLocalPlayer()->UpdateHailGui();
			}
			if ( !TargetEntityInSpace->is_derelict ) {
				TargetEntityInSpace->GoToRedAlert();
				EndNeutralityWithTeam( TargetEntityInSpace->team );
				TargetEntityInSpace->EndNeutralityWithTeam( team );
			}

			if ( team == TargetEntityInSpace->team ) BecomeASpacePirateShip();
			TargetEntityInSpace->FlashShieldDamageFX(500);
			TargetEntityInSpace->was_just_damaged = true;
			TargetEntityInSpace->SetShaderParm( 10, 1.0f -( (float)TargetEntityInSpace->hullStrength / (float)TargetEntityInSpace->max_hullStrength ) ); // set the damage decal alpha

			// SHIP TO SHIP DAMAGE SOUND EFFECT BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
				float hullDamage_to_totalDamage_ratio = (float)hullDamageToDeal / (float)(shieldDamageToDeal + hullDamageToDeal);
				if ( hullDamage_to_totalDamage_ratio >= 0.00f && hullDamage_to_totalDamage_ratio < 0.25f ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_light" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( hullDamage_to_totalDamage_ratio >= 0.25f && hullDamage_to_totalDamage_ratio < 0.50f  ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_medium" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( hullDamage_to_totalDamage_ratio >= 0.50f && hullDamage_to_totalDamage_ratio < 0.75f  ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_heavy" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( hullDamage_to_totalDamage_ratio >= 0.75f && hullDamage_to_totalDamage_ratio <= 1.00f  ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_critical" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
			}
			// There is no sound in space but this might be a good idea even so just to emphasize that action is taking place. I guess theoretically some kind of energy wave or particles could travel from the explosion to the player ShipOnBoard.
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace != gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace->stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && TargetEntityInSpace->stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
				gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_in_space" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
			}
			// SHIP TO SHIP DAMAGE SOUND EFFECT END

			// SHIP TO SHIP DAMAGE NOTIFICATION BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
				if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					idStr TargetEntityInSpaceColoredString;
					if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->PlayerShip->team ) {
						TargetEntityInSpaceColoredString = "^4" + TargetEntityInSpace->original_name;
					} else {
						TargetEntityInSpaceColoredString = "^1" + TargetEntityInSpace->original_name;
					}
					gameLocal.GetLocalPlayer()->UpdateNotificationList( "^2" "Your ship " "^0" "does " "^5" + idStr(hullDamageToDeal) + "^0" " hull dmg and " "^5" + idStr(shieldDamageToDeal) + "^0" " shield dmg to " + TargetEntityInSpaceColoredString);
				} else {

					idStr this_ship_text_color;
					if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^1"; }
					if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^8"; }
					if ( team == gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^4"; }

					idStr target_ship_text_color;
					if ( !TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^1"; }
					if ( TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^8"; }
					if ( TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^4"; }

					if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) {
						target_ship_text_color = "^2";
					}
					gameLocal.GetLocalPlayer()->UpdateNotificationList( this_ship_text_color + original_name + "^0" " does " "^5" + idStr(hullDamageToDeal) + "^0" " hull dmg and " "^5" + idStr(shieldDamageToDeal) + "^0" " shield dmg to " + target_ship_text_color + TargetEntityInSpace->original_name);
				}
			}
			// SHIP TO SHIP DAMAGE NOTIFICATION END

			// CAPTAIN DISPLAY AND HUD INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH BEGIN
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace && gameLocal.GetLocalPlayer()->ShipOnBoard == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->hud ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			// CAPTAIN DISPLAY AND HUD INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH END

			if ( TargetEntityInSpace->hullStrength <= 0 && !TargetEntityInSpace->ship_destruction_sequence_initiated ) {
				// BOYETTE NOTE TODO: need to put notification here - "the ship destroyed the ship".
				CeaseFiringWeaponsAndTorpedos();
				// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
				if ( common->m_pStatsAndAchievements ) {
					if ( idStr::Icmp(TargetEntityInSpace->spawnArgs.GetString("faction_name", "Unknown"), "Space Insect" ) == 0 ) {
						common->m_pStatsAndAchievements->m_nPlayerSpaceInsectKills++;
						common->StoreSteamStats();
					} else if ( !TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) ) {
						common->m_pStatsAndAchievements->m_nPlayerStarshipKills++;
						common->StoreSteamStats();
					}
				}
#endif
				// BOYETTE STEAM INTEGRATION END
				TargetEntityInSpace->BeginShipDestructionSequence();
				TargetEntityInSpace = NULL;
			}
		}
	}
}
// Does more damage to the Hull/CurrentTargetModule as the shields are depleted.
void sbShip::DealShipToShipDamageWithTorpedos() {

	if ( !ship_destruction_sequence_initiated ) {
		int shieldDamageToDeal = 0;
		int hullDamageToDeal = 0;

		if ( TargetEntityInSpace && consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule && consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
			if ( TargetEntityInSpace->shieldStrength >= 0 && TargetEntityInSpace->shieldStrength <= torpedos_damage_modifier ) {
				shieldDamageToDeal = TargetEntityInSpace->shieldStrength;
				TargetEntityInSpace->shieldStrength = TargetEntityInSpace->shieldStrength - shieldDamageToDeal;
				hullDamageToDeal = torpedos_damage_modifier - shieldDamageToDeal;
			} else if ( TargetEntityInSpace->shieldStrength > torpedos_damage_modifier ) {
				shieldDamageToDeal = torpedos_damage_modifier;
				TargetEntityInSpace->shieldStrength = TargetEntityInSpace->shieldStrength - shieldDamageToDeal;
				hullDamageToDeal = 0;
			}
			if ( g_enableReducedHullDamage.GetBool() ) {
				if ( TargetEntityInSpace->is_derelict ) {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal * (int)2 );
				} else {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal / (int)2 );
				}
			} else {
				if ( TargetEntityInSpace->is_derelict ) {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - ( hullDamageToDeal * (int)4 ); // BOYETTE NOTE: this is the original unreduced hull damage.
				} else {
					TargetEntityInSpace->hullStrength = TargetEntityInSpace->hullStrength - hullDamageToDeal; // BOYETTE NOTE: this is the original unreduced hull damage.
				}
			}
			consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule->RecieveShipToShipDamage( this, hullDamageToDeal );
			TargetEntityInSpace->shieldStrength = idMath::ClampInt(0,TargetEntityInSpace->max_shieldStrength,TargetEntityInSpace->shieldStrength);
			if ( TargetEntityInSpace->shields_raised ) {
				TargetEntityInSpace->shieldStrength_copy = TargetEntityInSpace->shieldStrength;
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				TargetEntityInSpace->friendlinessWithPlayer = 0; // need to put a check in here. If a ship is attacked by the playership, then it's relationship with the player should be set to zero and it should become hostile. We still need to do the become hostile logic with the teams and/or the sbShip AI.
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
				gameLocal.GetLocalPlayer()->UpdateHailGui();
			}
			if ( !TargetEntityInSpace->is_derelict ) {
				TargetEntityInSpace->GoToRedAlert();
				EndNeutralityWithTeam( TargetEntityInSpace->team );
				TargetEntityInSpace->EndNeutralityWithTeam( team );
			}

			if ( team == TargetEntityInSpace->team ) BecomeASpacePirateShip();
			TargetEntityInSpace->FlashShieldDamageFX(500);
			TargetEntityInSpace->was_just_damaged = true;
			TargetEntityInSpace->SetShaderParm( 10, 1.0f -( (float)TargetEntityInSpace->hullStrength / (float)TargetEntityInSpace->max_hullStrength ) ); // set the damage decal alpha

			// SHIP TO SHIP DAMAGE SOUND EFFECT BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
				float hullDamage_to_totalDamage_ratio = (float)hullDamageToDeal / (float)(shieldDamageToDeal + hullDamageToDeal);
				float shieldStrength_to_max_shieldStrength_ratio = (float)TargetEntityInSpace->shieldStrength / (float)TargetEntityInSpace->max_shieldStrength;
				float hullStrength_to_max_hullStrength_ratio = (float)TargetEntityInSpace->hullStrength / (float)TargetEntityInSpace->max_hullStrength;
				float damageIntensity = (2.0f - (shieldStrength_to_max_shieldStrength_ratio + hullStrength_to_max_hullStrength_ratio)) / 2.0f;
				if ( damageIntensity >= 0.00f && damageIntensity < 0.25f ) {
					//gameLocal.Printf( idStr(damageIntensity) + "<25.\n" );
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_torpedos_impact_light" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( damageIntensity >= 0.25f && damageIntensity < 0.50f  ) {
					//gameLocal.Printf( idStr(damageIntensity) + "<50.\n" );
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_torpedos_impact_medium" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( damageIntensity >= 0.50f && damageIntensity < 0.75f  ) {
					//gameLocal.Printf( idStr(damageIntensity) + "<70.\n" );
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_torpedos_impact_heavy" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				if ( damageIntensity >= 0.75f && damageIntensity <= 1.00f  ) {
					//gameLocal.Printf( idStr(damageIntensity) + "<100.\n" );
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_torpedos_impact_critical" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				// Sound intensity should be based on how much hullDamageToDeal is being done. Even if it is zero there should still be some shaking/sound though because torpedos won't pierce shields right away.
				// There is no sound in space but this might be a good idea even so just to emphasize that action is taking place. I guess theoretically some kind of energy wave or particles could travel from the explosion to the player ShipOnBoard.
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace != gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace->stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && TargetEntityInSpace->stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_torpedos_impact_in_space" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
			}
			// SHIP TO SHIP DAMAGE SOUND EFFECT END

			// SHIP TO SHIP DAMAGE NOTIFICATION BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
				if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					idStr TargetEntityInSpaceColoredString;
					if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->PlayerShip->team ) {
						TargetEntityInSpaceColoredString = "^4" + TargetEntityInSpace->original_name;
					} else {
						TargetEntityInSpaceColoredString = "^1" + TargetEntityInSpace->original_name;
					}
					gameLocal.GetLocalPlayer()->UpdateNotificationList( "^2" "Your ship " "^0" "does " "^5" + idStr(hullDamageToDeal) + "^0" " hull dmg and " "^5" + idStr(shieldDamageToDeal) + "^0" " shield dmg to " + TargetEntityInSpaceColoredString);
				} else {
					idStr this_ship_text_color;
					if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^1"; }
					if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^8"; }
					if ( team == gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^4"; }

					idStr target_ship_text_color;
					if ( !TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^1"; }
					if ( TargetEntityInSpace->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && TargetEntityInSpace->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^8"; }
					if ( TargetEntityInSpace->team == gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^4"; }

					if ( gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) {
						target_ship_text_color = "^2";
					}
					gameLocal.GetLocalPlayer()->UpdateNotificationList( this_ship_text_color + original_name + "^0" " does " "^5" + idStr(hullDamageToDeal) + "^0" " hull dmg and " "^5" + idStr(shieldDamageToDeal) + "^0" " shield dmg to " + target_ship_text_color + TargetEntityInSpace->original_name);
				}
			}
			// SHIP TO SHIP DAMAGE NOTIFICATION END

			// CAPTAIN DISPLAY INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH BEGIN
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace && gameLocal.GetLocalPlayer()->ShipOnBoard == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->hud ) {
					if ( TargetEntityInSpace->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			// CAPTAIN DISPLAY INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH END

			if ( TargetEntityInSpace->hullStrength <= 0 && !TargetEntityInSpace->ship_destruction_sequence_initiated ) {
				// BOYETTE NOTE TODO: need to put notification here - "the ship destroyed the ship".
				CeaseFiringWeaponsAndTorpedos();
				// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
				if ( common->m_pStatsAndAchievements ) {
					if ( idStr::Icmp(TargetEntityInSpace->spawnArgs.GetString("faction_name", "Unknown"), "Space Insect" ) == 0 ) {
						common->m_pStatsAndAchievements->m_nPlayerSpaceInsectKills++;
						common->StoreSteamStats();
					} else if ( !TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) ) {
						common->m_pStatsAndAchievements->m_nPlayerStarshipKills++;
						common->StoreSteamStats();
					}
				}
#endif
				// BOYETTE STEAM INTEGRATION END
				TargetEntityInSpace->BeginShipDestructionSequence();
				TargetEntityInSpace = NULL;
			}

			// CAPTAIN DISPLAY AND HUD INCOMING TORPEDOS EXPLOSIONS
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("targetship_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingTorpedosExplosion");
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->CaptainGui->SetStateInt("playership_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingTorpedosExplosion");
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && TargetEntityInSpace && gameLocal.GetLocalPlayer()->ShipOnBoard == TargetEntityInSpace ) {
				if ( gameLocal.GetLocalPlayer()->hud ) {
					gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_x", TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->hud->SetStateInt("shiponboard_incoming_torpedos_attack_y", -TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule));
					gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingTorpedosExplosion");
				}
			}
		}
	}
}

void sbShip::FlashShieldDamageFX(int duration) {
	if( ShieldEntity ) {
		ShieldEntity->SetShaderParm(10,1.0f);
	}
	CancelEvents( &EV_UpdateShieldEntityVisibility );
	PostEventMS( &EV_UpdateShieldEntityVisibility, duration );
}

void sbShip::CheckWeaponsTargetQueue() {
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {

		if ( ai_always_targets_random_module && (!gameLocal.GetLocalPlayer() || (gameLocal.GetLocalPlayer() && !gameLocal.GetLocalPlayer()->PlayerShip) || (gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip != this)) ) {
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( WeaponsTargetQueue[i] == check_module && TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule ) {
						if ( gameLocal.random.RandomFloat() < 0.25f ) {
							consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
							return;
						}
					}
				}
			}
			std::vector<sbModule*> modules_that_exist;
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( TargetEntityInSpace && TargetEntityInSpace->consoles[i] && TargetEntityInSpace->consoles[i]->ControlledModule ) {
					modules_that_exist.push_back(TargetEntityInSpace->consoles[i]->ControlledModule);
				}
			}
			if ( modules_that_exist.size() > 0 ) {
				consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = modules_that_exist[gameLocal.random.RandomInt(modules_that_exist.size() - 1)];
			}
		} else {
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( WeaponsTargetQueue[i] == check_module && TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule && TargetEntityInSpace->consoles[check_module]->ControlledModule->health > 0 ) {
						consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
						return;
					}
				}
			}
			// if all of the module healths are zero, just pick a module to target.
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule ) {
						consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
						return;
					}
				}
			}
		}
	}
}

void sbShip::CheckTorpedosTargetQueue() {
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {

		if ( ai_always_targets_random_module && (!gameLocal.GetLocalPlayer() || (gameLocal.GetLocalPlayer() && !gameLocal.GetLocalPlayer()->PlayerShip) || (gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip != this)) ) {
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( TorpedosTargetQueue[i] == check_module && TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule ) {
						if ( gameLocal.random.RandomFloat() < 0.25f ) {
							consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
							return;
						}
					}
				}
			}
			std::vector<sbModule*> modules_that_exist;
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( TargetEntityInSpace && TargetEntityInSpace->consoles[i] && TargetEntityInSpace->consoles[i]->ControlledModule ) {
					modules_that_exist.push_back(TargetEntityInSpace->consoles[i]->ControlledModule);
				}
			}
			if ( modules_that_exist.size() > 0 ) {
				consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = modules_that_exist[gameLocal.random.RandomInt(modules_that_exist.size() - 1)];
			}
		} else {
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( TorpedosTargetQueue[i] == check_module && TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule && TargetEntityInSpace->consoles[check_module]->ControlledModule->health > 0 ) {
						consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
						return;
					}
				}
			}
			// if all of the module healths are zero, just pick a module to target.
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
					if ( TargetEntityInSpace && TargetEntityInSpace->consoles[check_module] && TargetEntityInSpace->consoles[check_module]->ControlledModule ) {
						consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = TargetEntityInSpace->consoles[check_module]->ControlledModule;
						return;
					}
				}
			}
		}
	}
}

// this function will not move the module up if it is already at the top.
void sbShip::MoveUpModuleInWeaponsTargetQueue(int ModuleID) {
	for ( int i = 1; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( WeaponsTargetQueue[i] == ModuleID ) {
			// swap it ahead
			int t;
			t = WeaponsTargetQueue[i-1];
			WeaponsTargetQueue[i-1] = ModuleID;
			WeaponsTargetQueue[i] = t;
			return;
		}
	}
}
// this function will not move the module down if it is already at the bottom.
void sbShip::MoveDownModuleInWeaponsTargetQueue(int ModuleID) {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS-1; i++ ) {
		if ( WeaponsTargetQueue[i] == ModuleID ) {
			// swap it backward
			int t;
			t = WeaponsTargetQueue[i+1];
			WeaponsTargetQueue[i+1] = ModuleID;
			WeaponsTargetQueue[i] = t;
			return;
		}
	}
}
// this function will not move the module up if it is already at the top.
void sbShip::MoveUpModuleInTorpedosTargetQueue(int ModuleID) {
	for ( int i = 1; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( TorpedosTargetQueue[i] == ModuleID ) {
			// swap it ahead
			int t;
			t = TorpedosTargetQueue[i-1];
			TorpedosTargetQueue[i-1] = ModuleID;
			TorpedosTargetQueue[i] = t;
			return;
		}
	}
}
// this function will not move the module down if it is already at the bottom.
void sbShip::MoveDownModuleInTorpedosTargetQueue(int ModuleID) {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS-1; i++ ) {
		if ( TorpedosTargetQueue[i] == ModuleID ) {
			// swap it backward
			int t;
			t = TorpedosTargetQueue[i+1];
			TorpedosTargetQueue[i+1] = ModuleID;
			TorpedosTargetQueue[i] = t;
			return;
		}
	}
}
// this function will not do the swap if the value is not within the number of modules possible
void sbShip::SwapModueleIDInWeaponsTargetQueue(int ModuleID,int swap_to_pos) {
	if ( WeaponsTargetQueue[swap_to_pos] == ModuleID ) {
		return;
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( WeaponsTargetQueue[i] == ModuleID && swap_to_pos >= 0 && swap_to_pos < MAX_MODULES_ON_SHIPS ) {
			// swap it
			int t;
			t = WeaponsTargetQueue[swap_to_pos];
			WeaponsTargetQueue[swap_to_pos] = ModuleID;
			WeaponsTargetQueue[i] = t;
			return;
		}
	}
}
// this function will not do the swap if the value is not within the number of modules possible
void sbShip::SwapModueleIDInTorpedosTargetQueue(int ModuleID,int swap_to_pos) {
	if ( TorpedosTargetQueue[swap_to_pos] == ModuleID ) {
		return;
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( TorpedosTargetQueue[i] == ModuleID && swap_to_pos >= 0 && swap_to_pos < MAX_MODULES_ON_SHIPS  ) {
			// swap it
			int t;
			t = TorpedosTargetQueue[swap_to_pos];
			TorpedosTargetQueue[swap_to_pos] = ModuleID;
			TorpedosTargetQueue[i] = t;
			return;
		}
	}
}

// this function will check to see if the ship is already firing at a ship before it targets a different ship. If it is indeed firing it will post an event to change the target once it is done.
void sbShip::SetTargetEntityInSpace(sbShip* SpaceEntityToTarget) {
		if ( ( ship_is_firing_weapons || ship_is_firing_torpedo ) && SpaceEntityToTarget ) {
			TempTargetEntityInSpace = SpaceEntityToTarget;
			PostEventMS( &EV_SetTargetEntityInSpace, 100 );
			// put a notice here in the captains log/menu letting the player know that the target will switch once the volley finishes.
		} else if ( SpaceEntityToTarget ) {
			TargetEntityInSpace = SpaceEntityToTarget;
			UpdateGuisOnTransporterPad();
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->UpdateCaptainHudOnce();
				gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
			}

			// clear all module targets - weapons and torpedos being the most important currently.
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( consoles[i] && consoles[i]->ControlledModule ) {
					consoles[i]->ControlledModule->CurrentTargetModule = NULL;
				}
			}

			// if switching targets to an unfriendly ship - the weapons should automatically attack - if friendly they should not - for now we will just clear the weapons targets when a new ship/space entity is targeted.
			if ( team == TargetEntityInSpace->team ) {
				weapons_autofire_on = false;
				if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
					consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			} else if ( TargetEntityInSpace && HasNeutralityWithShip( TargetEntityInSpace ) ) { // can obviously combine some of these if statements - but this works fine for now.
				weapons_autofire_on = false;
				if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
					consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			} else {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship) {
					weapons_autofire_on = false;
				} else {
					weapons_autofire_on = true;
				}
			}
			if ( team == TargetEntityInSpace->team ) {
				torpedos_autofire_on = false;
				if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
					consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			} else if ( TargetEntityInSpace && HasNeutralityWithShip( TargetEntityInSpace ) ) { // can obviously combine some of these if statements - but this works fine for now.
				torpedos_autofire_on = false;
				if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
					consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			} else {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship) {
					torpedos_autofire_on = false;
				} else {
					torpedos_autofire_on = true;
				}
			}

			if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				if ( team == TargetEntityInSpace->team ) {
					gameLocal.GetLocalPlayer()->UpdateNotificationList( "The ^4" + TargetEntityInSpace->original_name + "^0 is friendly. ^2 Your ship's ^0 auto-fire has been deactivated.");
				} else if ( TargetEntityInSpace && HasNeutralityWithShip( TargetEntityInSpace ) ) {
					gameLocal.GetLocalPlayer()->UpdateNotificationList( "The ^8" + TargetEntityInSpace->original_name + "^0 is non-hostile. ^2 Your ship's ^0 auto-fire has been deactivated.");	
				} else {
					gameLocal.GetLocalPlayer()->UpdateNotificationList( "The ^1" + TargetEntityInSpace->original_name + "^0 is an enemy. ^2 Your ship's ^0 auto-fire has been initiated.");	
				}
			}

			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->CaptainGui ) {
				if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
					if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace->shields_raised ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "TargetShipRaiseShields" );
					} else {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "TargetShipLowerShields" );
					}
				}
			}
			//TempTargetEntityInSpace = NULL;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
				gameLocal.UpdateSpaceCommandViewscreenCamera();
			}
		}
}



void sbShip::AutoManageModulePowerlevels() {

	// first we need to zero out the current power levels.
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		while ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->power_allocated > 0 ) {
				DecreaseModulePower(consoles[i]->ControlledModule);
		}
	}

	// then we need to allocate the power based on the power priority system and the automanage_target_power_level that have been set. 
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
			if ( ModulesPowerQueue[i] == check_module && consoles[check_module] && consoles[check_module]->ControlledModule && consoles[check_module]->ControlledModule->health > 0 ) {
				while ( consoles[check_module]->ControlledModule->power_allocated < consoles[check_module]->ControlledModule->automanage_target_power_level && consoles[check_module]->ControlledModule->power_allocated < consoles[check_module]->ControlledModule->damage_adjusted_max_power ) {
					IncreaseModulePower(consoles[check_module]->ControlledModule);
					if ( current_power_reserve <= 0 ) return;
				}
			}
		}
	}


	// then we need to allocate any left over power based on the queue position of each module. 
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		for ( int check_module = 0; check_module < MAX_MODULES_ON_SHIPS; check_module++ ) {
			if ( ModulesPowerQueue[i] == check_module && consoles[check_module] && consoles[check_module]->ControlledModule && consoles[check_module]->ControlledModule->health > 0 ) {
				while ( consoles[check_module]->ControlledModule->power_allocated < consoles[check_module]->ControlledModule->damage_adjusted_max_power ) {
					IncreaseModulePower(consoles[check_module]->ControlledModule);
					if ( current_power_reserve <= 0 ) return;
				}
			}
		}
	}

}

// this function will not move the module up if it is already at the top.
void sbShip::MoveUpModuleInModulesPowerQueue(int ModuleID) {
	for ( int i = 1; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( ModulesPowerQueue[i] == ModuleID ) {
			// swap it ahead
			int t;
			t = ModulesPowerQueue[i-1];
			ModulesPowerQueue[i-1] = ModuleID;
			ModulesPowerQueue[i] = t;
			return;
		}
	}
}
// this function will not move the module down if it is already at the bottom.
void sbShip::MoveDownModuleInModulesPowerQueue(int ModuleID) {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS-1; i++ ) {
		if ( ModulesPowerQueue[i] == ModuleID ) {
			// swap it backward
			int t;
			t = ModulesPowerQueue[i+1];
			ModulesPowerQueue[i+1] = ModuleID;
			ModulesPowerQueue[i] = t;
			return;
		}
	}
}
// this function will not do the swap if the value is not within the number of modules possible
void sbShip::SwapModueleIDInModulesPowerQueue(int ModuleID,int swap_to_pos) {
	if ( ModulesPowerQueue[swap_to_pos] == ModuleID ) {
		return;
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( ModulesPowerQueue[i] == ModuleID && swap_to_pos >= 0 && swap_to_pos < MAX_MODULES_ON_SHIPS ) {
			// swap it
			int t;
			t = ModulesPowerQueue[swap_to_pos];
			ModulesPowerQueue[swap_to_pos] = ModuleID;
			ModulesPowerQueue[i] = t;
			return;
		}
	}
}
// this function will check to see if the ship is already firing at a ship before it targets a different ship. If it is indeed firing it will post an event to change the target once it is done.
void sbShip::ReturnToReserveExcessPowerFromDamagedModules() {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		while ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->power_allocated > consoles[i]->ControlledModule->damage_adjusted_max_power ) { // BOYETTE NOTE TODO IMPORTANT: maybe prevent runaway loop here by only checking 8 times (or whatever we decide the maximum power to be)
			DecreaseModulePower(consoles[i]->ControlledModule);
		}
	}
}
void sbShip::IncreaseAutomanageTargetModulePower(sbModule *module) {
	if (current_automanage_power_reserve > 0 && (module->automanage_target_power_level < module->max_power) ) {
		module->automanage_target_power_level++;
		current_automanage_power_reserve--;
	}
	module->RecalculateModuleEfficiency();
}

void sbShip::DecreaseAutomanageTargetModulePower(sbModule *module) {
	if (module->automanage_target_power_level > 0) {
		module->automanage_target_power_level--;
		current_automanage_power_reserve++;
	}
	module->RecalculateModuleEfficiency();
}

void sbShip::ResetAutomanagePowerReserves() {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 0 ) {
			while ( consoles[i]->ControlledModule->automanage_target_power_level > 0 ) {
				DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule);
			}
		}
	}
}

void sbShip::LowOxygenDamageToAIOnBoard(float damage_scale) {
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] && AIsOnBoard[i]->ShipOnBoard && AIsOnBoard[i]->ShipOnBoard == this ) {
			AIsOnBoard[i]->Damage(this,this,idVec3(0,0,0),"damage_low_oxygen_spaceship",damage_scale,INVALID_JOINT);
		}
	}
	if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_low_oxygen_spaceship",damage_scale,INVALID_JOINT);
		gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL ); // BOYETTE NOTE: added 06 03 2016
		if ( play_low_oxygen_alert_sound ) {
			gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_low_atmosphere_alert", "ship_alert_low_atmosphere_default" ) ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
			play_low_oxygen_alert_sound = false;
		}
	}
}

void sbShip::EvaluateCurrentOxygenLevelAndOxygenActions() {
	if ( !infinite_oxygen ) {
		if ( gameLocal.time >= evaluate_current_oxygen_timer + 2000 ) { // 2000 milliseconds = every two seconds
			current_oxygen_level--;
			current_oxygen_level = idMath::ClampInt(0,100,current_oxygen_level);

			if ( current_oxygen_level >= 50 ) {
				play_low_oxygen_alert_sound = true;
			}

			// BOYETTE 06 03 2016 BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
				if ( current_oxygen_level > 90 ) {
					show_low_oxygen_alert_display = true;
				}
				if ( current_oxygen_level < 50 ) {
					show_low_oxygen_alert_display = false;
				}

				if ( show_low_oxygen_alert_display ) {
					if ( current_oxygen_level < 85 && current_oxygen_level >= 65 && current_oxygen_level % 4 == 0 ) { // only amounts not divisible by 4
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1WARNING: ^1OXYGEN ^1LEVEL ^1BELOW ^1NORMAL" );
						gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_low_oxygen_spaceship_no_shake",0,INVALID_JOINT);
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					} else if ( current_oxygen_level < 65 && current_oxygen_level >= 50 && current_oxygen_level % 2 == 0 ) { // only on evens not odds
						gameLocal.GetLocalPlayer()->UpdateNotificationList( "^1WARNING: ^1OXYGEN ^1LEVEL ^1CRITICAL" );
						gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_low_oxygen_spaceship_no_shake",0,INVALID_JOINT);
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					}
				}
			}
			// BOYETTE 06 03 2016 END

			if ( current_oxygen_level < 50 && current_oxygen_level > 30 ) {
				LowOxygenDamageToAIOnBoard(1.0f);
			} else if ( current_oxygen_level <= 30 && current_oxygen_level > 0 ) {
				LowOxygenDamageToAIOnBoard(3.0f);
			} else if ( current_oxygen_level <= 0 ) {
				LowOxygenDamageToAIOnBoard(5.0f);
			}

			evaluate_current_oxygen_timer = gameLocal.time;
		}
	}
}

void sbShip::TriggerShipTransporterPadFX() {
	if ( TransporterParticleEntitySpawnMarker ) {
		if ( !TransporterParticleEntityFX.GetEntity() || !TransporterParticleEntityFX.GetEntity()->IsActive() ) {
			if ( TransporterParticleEntityFX.GetEntity() ) {
				TransporterParticleEntityFX.GetEntity()->Event_Remove();
				TransporterParticleEntityFX = NULL;
			}
			idEntity *ent;
			const char *clsname;
			if ( !TransporterParticleEntityFX.GetEntity() ) {
				TransporterParticleEntityDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("transporter_pad_particle_def","module_transporter_circular_pad_particle_default"), true );
				gameLocal.SpawnEntityDef( *TransporterParticleEntityDef, &ent, false );
				TransporterParticleEntityFX = ( idFuncEmitter * )ent;
			}
			if ( TransporterParticleEntityFX.GetEntity() ) {
				TransporterParticleEntityFX.GetEntity()->SetOrigin(TransporterParticleEntitySpawnMarker->GetPhysics()->GetOrigin() + idVec3(0,0,0));
				TransporterParticleEntityFX.GetEntity()->Bind(this,false);
				TransporterParticleEntityFX.GetEntity()->Event_Activate(this);
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard ) { // BOYETTE NOTE: added 05 20 2016 because it was odd when two different sounds were played. This makes the most sense right now because the player will hear the full with the most consistency.
					TransporterParticleEntityFX.GetEntity()->StartSoundShader( declManager->FindSound( gameLocal.GetLocalPlayer()->ShipOnBoard->spawnArgs.GetString( "snd_transporter_pad_initiate", "transporter_pad_initiate_snd_default" ) ), SND_CHANNEL_ANY, 0, false, NULL );
				} else {
					TransporterParticleEntityFX.GetEntity()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_transporter_pad_initiate", "transporter_pad_initiate_snd_default" ) ), SND_CHANNEL_ANY, 0, false, NULL );
				}
			}
		} else if ( TransporterParticleEntityFX.GetEntity() && TransporterParticleEntityFX.GetEntity()->IsActive() ) {
			if ( TransporterParticleEntityFX.GetEntity() ) {
				TransporterParticleEntityFX.GetEntity()->Event_Activate(this);
			}
		}
	}
}

void sbShip::SendCrewToBattlestations() {
	for ( int i = 0; i < MAX_CREW_ON_SHIPS - 1; i ++ ) { // boyete note: MAX_CREW_ON_SHIPS - 1 because we treat the captain a little differently
		if ( crew[i] ) {
			if ( !crew[i]->was_killed && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this ) {
				idEntity* alt_battlestation_entity = NULL;
				if ( consoles[i] ) {
					if ( !(crew[i]->DistanceToEntity(consoles[i]) < 64.0f && crew[i]->working_on_console) ) {
						crew[i]->crew_auto_mode_activated = false;
						crew[i]->player_follow_mode_activated = false;
						crew[i]->handling_emergency_oxygen_situation = false;
						crew[i]->SetEntityToMoveToByCommand( consoles[i] );
						continue;
					}
				} else if ( room_node[MISCONEROOMID] ) {
					alt_battlestation_entity = room_node[MISCONEROOMID];
				} else if ( room_node[MISCTWOROOMID] ) {
					alt_battlestation_entity = room_node[MISCTWOROOMID];
				} else if ( room_node[MISCTHREEROOMID] ) {
					alt_battlestation_entity = room_node[MISCTHREEROOMID];
				} else if ( room_node[MISCFOURROOMID] ) {
					alt_battlestation_entity = room_node[MISCFOURROOMID];
				} else if ( TransporterBounds ) {
					alt_battlestation_entity = TransporterBounds;
				}
				if ( alt_battlestation_entity ) {
					if ( !crew[i]->DistanceToEntity(alt_battlestation_entity) < 64.0f ) {
						crew[i]->crew_auto_mode_activated = false;
						crew[i]->player_follow_mode_activated = false;
						crew[i]->handling_emergency_oxygen_situation = false;
						crew[i]->SetEntityToMoveToByCommand( alt_battlestation_entity );
						continue;
					}
				}
			}
		}
	}



/*
	if ( crew[MEDICALCREWID] ) {
		if ( !crew[MEDICALCREWID]->was_killed && consoles[MEDICALMODULEID] && crew[MEDICALCREWID]->ShipOnBoard && crew[MEDICALCREWID]->ShipOnBoard == this && !(crew[MEDICALCREWID]->DistanceToEntity(consoles[MEDICALMODULEID]) < 64.0f && crew[MEDICALCREWID]->working_on_console) ) {
			crew[MEDICALCREWID]->crew_auto_mode_activated = false;
			crew[MEDICALCREWID]->player_follow_mode_activated = false;
			crew[MEDICALCREWID]->handling_emergency_oxygen_situation = false;
			crew[MEDICALCREWID]->SetEntityToMoveToByCommand( consoles[MEDICALMODULEID] );
		}
	}
	if ( crew[ENGINESCREWID] ) {
		if ( !crew[ENGINESCREWID]->was_killed && consoles[ENGINESMODULEID] && crew[ENGINESMODULEID]->ShipOnBoard && crew[ENGINESMODULEID]->ShipOnBoard == this && !(crew[ENGINESMODULEID]->DistanceToEntity(consoles[ENGINESMODULEID]) < 64.0f && crew[ENGINESMODULEID]->working_on_console) ) {
			crew[ENGINESCREWID]->crew_auto_mode_activated = false;
			crew[ENGINESCREWID]->player_follow_mode_activated = false;
			crew[ENGINESCREWID]->handling_emergency_oxygen_situation = false;
			crew[ENGINESCREWID]->SetEntityToMoveToByCommand( consoles[ENGINESMODULEID] );
		}
	}
	if ( crew[WEAPONSCREWID] ) {
		if ( !crew[WEAPONSCREWID]->was_killed && consoles[WEAPONSMODULEID] && crew[WEAPONSMODULEID]->ShipOnBoard && crew[WEAPONSMODULEID]->ShipOnBoard == this && !(crew[WEAPONSMODULEID]->DistanceToEntity(consoles[WEAPONSMODULEID]) < 64.0f && crew[WEAPONSMODULEID]->working_on_console) ) {
			crew[WEAPONSCREWID]->crew_auto_mode_activated = false;
			crew[ENGINESCREWEAPONSCREWIDWID]->player_follow_mode_activated = false;
			crew[WEAPONSCREWID]->handling_emergency_oxygen_situation = false;
			crew[WEAPONSCREWID]->SetEntityToMoveToByCommand( consoles[WEAPONSMODULEID] );
		}
	}
	if ( crew[TORPEDOSCREWID] ) {
		if ( !crew[TORPEDOSCREWID]->was_killed && consoles[TORPEDOSMODULEID] && crew[TORPEDOSMODULEID]->ShipOnBoard && crew[TORPEDOSMODULEID]->ShipOnBoard == this && !(crew[TORPEDOSMODULEID]->DistanceToEntity(consoles[TORPEDOSMODULEID]) < 64.0f && crew[TORPEDOSMODULEID]->working_on_console) ) {
			crew[TORPEDOSCREWID]->crew_auto_mode_activated = false;
			crew[TORPEDOSCREWID]->player_follow_mode_activated = false;
			crew[TORPEDOSCREWID]->handling_emergency_oxygen_situation = false;
			crew[TORPEDOSCREWID]->SetEntityToMoveToByCommand( consoles[TORPEDOSMODULEID] );
		}
	}
	if ( crew[SHIELDSCREWID] ) {
		if ( !crew[SHIELDSCREWID]->was_killed && consoles[SHIELDSMODULEID] && crew[SHIELDSMODULEID]->ShipOnBoard && crew[SHIELDSMODULEID]->ShipOnBoard == this && !(crew[SHIELDSMODULEID]->DistanceToEntity(consoles[SHIELDSMODULEID]) < 64.0f && crew[SHIELDSMODULEID]->working_on_console) ) {
			crew[SHIELDSCREWID]->crew_auto_mode_activated = false;
			crew[SHIELDSCREWID]->player_follow_mode_activated = false;
			crew[SHIELDSCREWID]->handling_emergency_oxygen_situation = false;
			crew[SHIELDSCREWID]->SetEntityToMoveToByCommand( consoles[SHIELDSMODULEID] );
		}
	}
	if ( crew[SENSORSCREWID] ) {
		if ( !crew[SENSORSCREWID]->was_killed && consoles[SENSORSMODULEID] && crew[SENSORSMODULEID]->ShipOnBoard && crew[SENSORSMODULEID]->ShipOnBoard == this && !(crew[SENSORSMODULEID]->DistanceToEntity(consoles[SENSORSMODULEID]) < 64.0f && crew[SENSORSMODULEID]->working_on_console) ) {
			crew[SENSORSCREWID]->crew_auto_mode_activated = false;
			crew[SENSORSCREWID]->player_follow_mode_activated = false;
			crew[SENSORSCREWID]->handling_emergency_oxygen_situation = false;
			crew[SENSORSCREWID]->SetEntityToMoveToByCommand( consoles[SENSORSMODULEID] );
		}
	}
	if ( crew[ENVIRONMENTCREWID] ) {
		if ( !crew[ENVIRONMENTCREWID]->was_killed && consoles[ENVIRONMENTMODULEID] && crew[ENVIRONMENTMODULEID]->ShipOnBoard && crew[ENVIRONMENTMODULEID]->ShipOnBoard == this && !(crew[ENVIRONMENTMODULEID]->DistanceToEntity(consoles[ENVIRONMENTMODULEID]) < 64.0f && crew[ENVIRONMENTMODULEID]->working_on_console) ) {
			crew[ENVIRONMENTCREWID]->crew_auto_mode_activated = false;
			crew[ENVIRONMENTCREWID]->player_follow_mode_activated = false;
			crew[ENVIRONMENTCREWID]->handling_emergency_oxygen_situation = false;
			crew[ENVIRONMENTCREWID]->SetEntityToMoveToByCommand( consoles[ENVIRONMENTMODULEID] );
		}
	}
	if ( crew[COMPUTERCREWID] ) {
		if ( !crew[COMPUTERCREWID]->was_killed && consoles[COMPUTERMODULEID] && crew[COMPUTERMODULEID]->ShipOnBoard && crew[COMPUTERMODULEID]->ShipOnBoard == this && !(crew[COMPUTERMODULEID]->DistanceToEntity(consoles[COMPUTERMODULEID]) < 64.0f && crew[COMPUTERMODULEID]->working_on_console) ) {
			crew[COMPUTERCREWID]->crew_auto_mode_activated = false;
			crew[COMPUTERCREWID]->player_follow_mode_activated = false;
			crew[COMPUTERCREWID]->handling_emergency_oxygen_situation = false;
			crew[COMPUTERCREWID]->SetEntityToMoveToByCommand( consoles[COMPUTERMODULEID] );
		}
	}
	if ( crew[SECURITYCREWID] ) {
		if ( !crew[SECURITYCREWID]->was_killed && consoles[SECURITYMODULEID] && crew[SECURITYMODULEID]->ShipOnBoard && crew[SECURITYMODULEID]->ShipOnBoard == this && !(crew[SECURITYMODULEID]->DistanceToEntity(consoles[SECURITYMODULEID]) < 64.0f && crew[SECURITYMODULEID]->working_on_console) ) {
			crew[SECURITYCREWID]->crew_auto_mode_activated = false;
			crew[SECURITYCREWID]->player_follow_mode_activated = false;
			crew[SECURITYCREWID]->handling_emergency_oxygen_situation = false;
			crew[SECURITYCREWID]->SetEntityToMoveToByCommand( consoles[SECURITYMODULEID] );
		}
	}
*/

	if ( crew[CAPTAINCREWID] ) {
		if ( !crew[CAPTAINCREWID]->was_killed && CaptainChair && crew[CAPTAINCREWID]->ShipOnBoard && crew[CAPTAINCREWID]->ShipOnBoard == this ) {
			if ( !(crew[CAPTAINCREWID]->DistanceToEntity(CaptainChair) < 64.0f && crew[CAPTAINCREWID]->seated_in_captain_chair) ) {
				crew[CAPTAINCREWID]->crew_auto_mode_activated = false;
				crew[CAPTAINCREWID]->player_follow_mode_activated = false;
				crew[CAPTAINCREWID]->handling_emergency_oxygen_situation = false;
				crew[CAPTAINCREWID]->SetEntityToMoveToByCommand( CaptainChair );
			}
		} else if ( !crew[CAPTAINCREWID]->was_killed && ReadyRoomCaptainChair && crew[CAPTAINCREWID]->ShipOnBoard && crew[CAPTAINCREWID]->ShipOnBoard == this ) {
			if ( !(crew[CAPTAINCREWID]->DistanceToEntity(ReadyRoomCaptainChair) < 64.0f && crew[CAPTAINCREWID]->seated_in_captain_chair) ) {
				crew[CAPTAINCREWID]->crew_auto_mode_activated = false;
				crew[CAPTAINCREWID]->player_follow_mode_activated = false;
				crew[CAPTAINCREWID]->handling_emergency_oxygen_situation = false;
				crew[CAPTAINCREWID]->SetEntityToMoveToByCommand( ReadyRoomCaptainChair );
			}
		}
	}
}

void sbShip::CeaseFiringWeaponsAndTorpedos() {
	weapons_autofire_on = false;
	torpedos_autofire_on = false;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
	}
	if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
	}
	// BOYETTE NOTE TODO: put a notice here if the ship is at the same stargrid location as the player - that it has ceased firing at it the "ship name".
}

void sbShip::UpdateCurrentComputerModuleBuffModifiers() {
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
		float computer_buff_amount_modifier;

		computer_buff_amount_modifier = ( (float)consoles[COMPUTERMODULEID]->ControlledModule->module_efficiency / 100.00f );
		// OLD BEFORE BALANCING:
		//if ( computer_buff_amount_modifier > 1.20f ) { // we could do a clamp here instead but this is fine
		//	computer_buff_amount_modifier = 1.20f;		// make sure you change the captain gui module buff icon display logic if you change this.
		//}
		// OLD AFTER BALANCING BUT NOT AS GOOD
		//float computer_buff_amount_power_cap = 0.56f + ( (float)module_max_powers[COMPUTERMODULEID] * 0.08f ); // with a min of 0.58 and a max of 1.2
		//if ( computer_buff_amount_modifier > computer_buff_amount_power_cap ) { // we could do a clamp here instead but this is fine
		//	computer_buff_amount_modifier = computer_buff_amount_power_cap;		// make sure you change the captain gui module buff icon display logic if you change this.
		//}
		float buff_intensity_gameplay_balancing = 0.60f; // adjust this for gameplay balancing to increase/decrease how much buffing happens
		computer_buff_amount_modifier = computer_buff_amount_modifier * ( buff_intensity_gameplay_balancing * ( 1.00f + ( (float)module_max_powers[COMPUTERMODULEID] * 0.1f )) ); // from 1.05 to 1.40
		//computer_buff_amount_modifier = computer_buff_amount_modifier * (float)module_max_powers[COMPUTERMODULEID]; // from 1.05 to 1.40
		if ( computer_buff_amount_modifier < 0.20f ) { // we could do a clamp here instead but this is fine
			computer_buff_amount_modifier = 0.20f;		// make sure you change the captain gui module buff icon display logic if you change this.
		}

		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( consoles[i] && consoles[i]->ControlledModule  ) {
				consoles[i]->ControlledModule->module_buffed_amount_modifier = computer_buff_amount_modifier;
			}
		}
		// WE DON"T UPDATE THE COMPUTER MODULE's module_buffed_amount_modifier - I think that might result in an ever increasing recursive loop.
	}
}

void sbShip::RecalculateAllModuleEfficiencies() {
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule  ) {
			consoles[i]->ControlledModule->RecalculateModuleEfficiency();
		}
	}
}

void sbShip::SynchronizeRedAlertSpecialFX() {
		//Start an initial sound shader here that just plays once when the red alert button is pressed.
		//int synched_time = 0;
		//int current_time = gameLocal.time;

		int remainder = 2000 - ( gameLocal.time % 2000 );

		PostEventMS( &EV_StartSynchdRedAlertFX, remainder + 1000 );
}

void sbShip::StopRedAlertSpecialFX() {

	CancelEvents(&EV_StartSynchdRedAlertFX);

	for( int i = 0; i < shiplights.Num(); i++ ) {
		shiplights[ i ].GetEntity()->SetShaderParm(7,0); // Enable the standard material stage on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(8,0); // Disable the light dimming stage on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(9,0); // Disable pulsing flare effect on the light fixture model

		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(7,0); // Enable the standard material stage on the light material
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(8,0); // Get rid of all light dimming on the light material
			}
		}
	}

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false);
	}
}

void sbShip::GoToRedAlert() {
	red_alert = true;
	RaiseShields();
	if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		SynchronizeRedAlertSpecialFX();
	}
}

void sbShip::CancelRedAlert() {
	red_alert = false;
	if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		StopRedAlertSpecialFX();
	}
}

void sbShip::TurnShipLightsOff() {
	ship_lights_on = false;
	for( int i = 0; i < shiplights.Num(); i++ ) {
		shiplights[ i ].GetEntity()->SetShaderParm(7,3); // Disable all material stages
		shiplights[ i ].GetEntity()->SetShaderParm(8,0); // Get rid of all light dimming on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(9,0); // Disable pulsing flare effect on the light fixture model

		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(7,3); // Disable all material stages on the light material
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(8,0); // Get rid of all light dimming on the light material
			}
		}
	}
	if ( red_alert ) {
		CancelEvents(&EV_StartSynchdRedAlertFX);
	}
}
void sbShip::DimRandomShipLightsOff() {
	ship_lights_on = false;
	for( int i = 0; i < shiplights.Num(); i++ ) {
		float random_light_dimming = 1.0f - (gameLocal.random.RandomFloat() * 0.75f);

		shiplights[ i ].GetEntity()->SetShaderParm(7,2); // Enable the dimmed material stage on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(8,random_light_dimming); // Set light dimming on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(9,0); // Disable pulsing flare effect on the light fixture model

		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(7,2); // Enable the dimmed material stage on the light material
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(8,random_light_dimming); // Set light dimming on the light material
			}
		}
	}
	if ( red_alert ) {
		CancelEvents(&EV_StartSynchdRedAlertFX);
	}
	red_alert = false;
}
void sbShip::DimShipLightsOff() {
	ship_lights_on = false;
	for( int i = 0; i < shiplights.Num(); i++ ) {
		float light_dimming = 0.5f;
		shiplights[ i ].GetEntity()->SetShaderParm(7,2); // Enable the dimmed material stage on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(8,light_dimming); // Set light dimming on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(9,0); // Disable pulsing flare effect on the light fixture model

		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(7,2); // Enable the dimmed material stage on the light material
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(8,light_dimming); // Set light dimming on the light material
			}
		}
	}
	if ( red_alert ) {
		CancelEvents(&EV_StartSynchdRedAlertFX);
	}
	red_alert = false;
}

void sbShip::TurnShipLightsOn() {
	ship_lights_on = true;
	if ( red_alert ) {
		SynchronizeRedAlertSpecialFX(); // will turn back on and handle all appropriate lighting for red alert.
	} else {
		StopRedAlertSpecialFX(); // will turn back on and handle all appropriate lighting for not being at red alert.
	}
}

void sbShip::UpdateShipDoorHealths() {
	// THIS ISN'T DONE YET - If door health is greater than zero set it based on the security module efficiency.
	if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule && old_security_module_efficiency != consoles[SECURITYMODULEID]->ControlledModule->module_efficiency ) {
		old_security_module_efficiency = consoles[SECURITYMODULEID]->ControlledModule->module_efficiency;
		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) && shipdoors[ i ].GetEntity()->health > 0 ) {
				if ( dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->door_damaged_time + 2500 < gameLocal.time ) {     // Can make 2500 a constant later on - call it maybe MAXIMUM_TIME_BETWEEN_AI_DOOR_BREAKING_ATTACKS. 2500 should be the maximum amount of time between attacks if an AI is trying to break through a door - similar to the way module buffing works. That we we can tell if a door is under attack if this int is less than that.
					dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SetDoorGroupsHealth(shipdoors[ i ].GetEntity()->entity_max_health * ( (float)consoles[SECURITYMODULEID]->ControlledModule->module_efficiency / 100.00f ));
					dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->UpdateDoorGroupsMoverStatusShaderParms();
				}
			}
		}
	}
}

// we only need an open function because the player will be always be responsible for closing the hail(through "click_close_hailgui"). We could make a close hail function but I don't think it is necessary because we don't plan to use it at this time. When the ship becomes hostile we are still going to require the player to close the hail - but maybe we will have a start time function again so that the player can be attacked in this interval.
void sbShip::OpenHailWithLocalPlayer() {
	if ( !is_derelict ) {
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && !gameLocal.GetLocalPlayer()->PlayerShip->currently_in_hail && !gameLocal.GetLocalPlayer()->currently_in_story_gui && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony && !gameLocal.GetLocalPlayer()->PlayerShip->is_attempting_warp && gameLocal.GetLocalPlayer()->ShipOnBoard && !gameLocal.GetLocalPlayer()->ShipOnBoard->is_attempting_warp ) {
			gameLocal.GetLocalPlayer()->SelectedEntityInSpace = this;
			gameLocal.GetLocalPlayer()->SetOverlayHailGui(); // opens a hail with the selected entity - which we have just set to ourselves.
			currently_in_hail = true;
			gameLocal.GetLocalPlayer()->PlayerShip->currently_in_hail = true;
			wait_to_hail_order_num = 100.0f;
			// STILL NOT SURE IF WE WANT TO STOP TIME DURING A HAIL. - might need a variable that checks if we have already hailed someone.
			//g_stopTime.SetBool(true);
			//g_stopTimeForceFrameNumUpdateDuring.SetBool(true);
			//g_stopTimeForceRenderViewUpdateDuring.SetBool(true);
			if ( should_go_into_no_action_hail_mode_on_hail || hail_conditionals_met ) {
				in_no_action_hail_mode = true;
				gameLocal.GetLocalPlayer()->PlayerShip->CeaseFiringWeaponsAndTorpedos();
				Event_PutAllShipsAtTheSameSGPosIntoNoActionHailMode();
			}
			should_hail_the_playership = false;
			gameLocal.GetLocalPlayer()->UpdateHailGui();
		}
	}
}

void sbShip::UpdateGuisOnConsolesAndModules() {

	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {

		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( consoles[i] && consoles[i]->ControlledModule  ) {
				// update console gui
				consoles[i]->SetRenderEntityGui0Int( "console_health", consoles[i]->health );
				consoles[i]->SetRenderEntityGui0Float( "console_health_percentage", (float)consoles[i]->health / (float)consoles[i]->entity_max_health );
				consoles[i]->SetRenderEntityGui0Int( "module_health", consoles[i]->ControlledModule->health );
				consoles[i]->SetRenderEntityGui0Float( "module_health_percentage", (float)consoles[i]->ControlledModule->health / (float)consoles[i]->ControlledModule->entity_max_health );
				consoles[i]->SetRenderEntityGui0Int( "module_efficiency", consoles[i]->ControlledModule->module_efficiency );
				consoles[i]->SetRenderEntityGui0Float( "module_efficiency_percentage", (float)consoles[i]->ControlledModule->module_efficiency / 100.0f );
				consoles[i]->SetRenderEntityGui0Int( "module_buffed_amount", consoles[i]->ControlledModule->module_buffed_amount );
				consoles[i]->SetRenderEntityGui0Float( "module_buffed_amount_modifier", consoles[i]->ControlledModule->module_buffed_amount_modifier );
				consoles[i]->SetRenderEntityGui0Int( "module_power_allocated", consoles[i]->ControlledModule->power_allocated );
				consoles[i]->SetRenderEntityGui0Int( "module_max_power", consoles[i]->ControlledModule->max_power );
				consoles[i]->SetRenderEntityGui0Int( "module_max_health", consoles[i]->ControlledModule->entity_max_health );
				consoles[i]->SetRenderEntityGui0Int( "module_damage_adjusted_max_power", consoles[i]->ControlledModule->damage_adjusted_max_power );
				consoles[i]->SetRenderEntityGui0Float( "module_current_charge_percentage", consoles[i]->ControlledModule->current_charge_percentage );
				consoles[i]->SetRenderEntityGuisBools( "sit_question_visible", !(consoles[i]->ControlledModule->module_buffed_amount > 0) );

				if ( i == TORPEDOSMODULEID || i == WEAPONSMODULEID || i == SENSORSMODULEID ) {
					if ( TargetEntityInSpace ) {
						consoles[i]->SetRenderEntityGui0String( "targetship_icon_background", TargetEntityInSpace->ShipStargridIcon );
						consoles[i]->SetRenderEntityGui0Bool( "targetship_exists", true );
					} else {
						consoles[i]->SetRenderEntityGui0String( "targetship_icon_background", "" );
						consoles[i]->SetRenderEntityGui0Bool( "targetship_exists", false );
					}
				}
				if ( i == ENVIRONMENTMODULEID ) {
					consoles[i]->SetRenderEntityGui0Int( "current_oxygen_level", current_oxygen_level );
					consoles[i]->SetRenderEntityGui0Float( "current_oxygen_level_percentage", (float)current_oxygen_level / 100.0f );
				}
				if ( i == SHIELDSMODULEID ) {
					consoles[i]->SetRenderEntityGui0Float( "current_shieldstrength_percentage", (float)shieldStrength / (float)max_shieldStrength );
				}

				// update module gui
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "console_health", consoles[i]->health );
				consoles[i]->ControlledModule->SetRenderEntityGui0Float( "console_health_percentage", (float)consoles[i]->health / (float)consoles[i]->entity_max_health );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_health", consoles[i]->ControlledModule->health );
				consoles[i]->ControlledModule->SetRenderEntityGui0Float( "module_health_percentage", (float)consoles[i]->ControlledModule->health / (float)consoles[i]->ControlledModule->entity_max_health );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_efficiency", consoles[i]->ControlledModule->module_efficiency );
				consoles[i]->ControlledModule->SetRenderEntityGui0Float( "module_efficiency_percentage", (float)consoles[i]->ControlledModule->module_efficiency / 100.0f );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_buffed_amount", consoles[i]->ControlledModule->module_buffed_amount );
				consoles[i]->ControlledModule->SetRenderEntityGui0Float( "module_buffed_amount_modifier", consoles[i]->ControlledModule->module_buffed_amount_modifier );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_power_allocated", consoles[i]->ControlledModule->power_allocated );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_max_power", consoles[i]->ControlledModule->max_power );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_max_health", consoles[i]->ControlledModule->entity_max_health );
				consoles[i]->ControlledModule->SetRenderEntityGui0Int( "module_damage_adjusted_max_power", consoles[i]->ControlledModule->damage_adjusted_max_power );
				consoles[i]->ControlledModule->SetRenderEntityGui0Float( "module_current_charge_percentage", consoles[i]->ControlledModule->current_charge_percentage );

				if ( i == TORPEDOSMODULEID || i == WEAPONSMODULEID || i == SENSORSMODULEID ) {
					if ( TargetEntityInSpace ) {
						consoles[i]->ControlledModule->SetRenderEntityGui0String( "targetship_icon_background", TargetEntityInSpace->ShipStargridIcon );
						consoles[i]->ControlledModule->SetRenderEntityGui0Bool( "targetship_exists", true );
					} else {
						consoles[i]->ControlledModule->SetRenderEntityGui0String( "targetship_icon_background", "" );
						consoles[i]->ControlledModule->SetRenderEntityGui0Bool( "targetship_exists", false );
					}
				}
				if ( i == ENVIRONMENTMODULEID ) {
					consoles[i]->ControlledModule->SetRenderEntityGui0Int( "current_oxygen_level", current_oxygen_level );
					consoles[i]->ControlledModule->SetRenderEntityGui0Float( "current_oxygen_level_percentage", (float)current_oxygen_level / 100.0f );
				}
				if ( i == SHIELDSMODULEID ) {
					consoles[i]->ControlledModule->SetRenderEntityGui0Float( "current_shieldstrength_percentage", (float)shieldStrength / (float)max_shieldStrength );
				}

			}
		}

	}

	// HANDLE DAMAGE/REPAIR EVENTS
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {

		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( consoles[i] && consoles[i]->ControlledModule  ) {
				if ( consoles[i]->ControlledModule->module_was_just_damaged ) { // this gets reset on the player so we don't want to do that here.
					consoles[i]->HandleNamedEventOnGui0("ModuleDamaged");
					consoles[i]->ControlledModule->HandleNamedEventOnGui0("ModuleDamaged");
					consoles[i]->ControlledModule->module_was_just_damaged = false;
				}
				if ( consoles[i]->ControlledModule->module_was_just_repaired ) { // this gets reset on the player so we don't want to do that here.
					consoles[i]->HandleNamedEventOnGui0("ModuleRepaired");
					consoles[i]->ControlledModule->HandleNamedEventOnGui0("ModuleRepaired");
					consoles[i]->ControlledModule->module_was_just_repaired = false;
				}
				if ( consoles[i]->was_just_damaged ) { // this gets reset on the player so we don't want to do that here.
					consoles[i]->HandleNamedEventOnGui0("ConsoleDamaged");
					consoles[i]->was_just_damaged = false;
				}
				if ( consoles[i]->was_just_repaired ) { // this gets reset on the player so we don't want to do that here.
					consoles[i]->HandleNamedEventOnGui0("ConsoleRepaired");
					consoles[i]->was_just_repaired = false;
				}
			}
		}

	}

}

bool sbShip::CanBeTakenCommandOfByPlayer() {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip == this ) return false; // already in command

	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] && !crew[i]->was_killed && crew[i]->ShipOnBoard == this ) return false;
	}

	if ( spawnArgs.GetBool( "allow_player_to_take_command", "1" ) ) {
		return true;
	} else {
		return false;
	}
	return true; // just in case - probably not necessary to have this here
}

void sbShip::UpdateGuisOnCaptainChair() {
	if ( CaptainChair && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		if ( CaptainChair->SeatedEntity.GetEntity() ) {
			CaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
			CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",false);
			CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",false);
			CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",false);
			CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
			CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",true); // I don't think we need a message for this - the gui will just be hidden. It should be obvious that someone is in the captian chair.
		} else {
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				CaptainChair->SetRenderEntityGuisBools("sit_question_visible",true);
				CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",false);
			} else if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && !is_derelict && ( gameLocal.GetLocalPlayer()->PlayerShip->team == team || gameLocal.GetLocalPlayer()->team == team ) ) {
				CaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",false);
			} else if ( crew[CAPTAINCREWID] && !crew[CAPTAINCREWID]->was_killed ) {
				CaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",true);
				CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",false);
			} else if ( CanBeTakenCommandOfByPlayer() ) {
				CaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",true);
				CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
				if ( CaptainChair && CaptainChair->in_confirmation_process_of_player_take_command ) {
					CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",true);
				}
				CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",false);
			} else {
				CaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("the_captain_is_still_alive_notice_visible",false);
				CaptainChair->SetRenderEntityGuisBools("some_crew_is_still_alive_and_onboard_notice_visible",true);
				CaptainChair->SetRenderEntityGuisBools("confirm_take_command_question_visible",false);
				CaptainChair->SetRenderEntityGuisBools("someone_is_seated_in_the_captain_chair",false);
			}
		}
	}

	if ( ReadyRoomCaptainChair && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			ReadyRoomCaptainChair->SetRenderEntityGuisBools("is_the_playership",true);
			ReadyRoomCaptainChair->SetRenderEntityGuisBools("sit_question_visible",true);
		} else {
			ReadyRoomCaptainChair->SetRenderEntityGuisBools("is_the_playership",false);
			ReadyRoomCaptainChair->SetRenderEntityGuisBools("sit_question_visible",false);
		}
	}
}

void sbShip::UpdateViewScreenEntityVisuals() {
	if ( ViewScreenEntity && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		ViewScreenEntity->UpdateVisuals();
	}
}

void sbShip::UpdateGuisOnTransporterPad() {
	if ( TransporterPad && TransporterPad->IsType(sbTransporterPad::Type) ) {
		TransporterPad->SetRenderEntityGuisStrings("transporter_pad_parent_ship",spawnArgs.GetString("name"));
		TransporterPad->SetRenderEntityGuisStrings("transporter_pad_parent_ship_image_visual",spawnArgs.GetString("ship_image_visual", "textures/images_used_in_source/default_ship_image_visual.tga") );
		if ( TargetEntityInSpace ) {
			TransporterPad->SetRenderEntityGuisStrings("transporter_pad_target_ship",TargetEntityInSpace->spawnArgs.GetString("name"));
			TransporterPad->SetRenderEntityGuisStrings("transporter_pad_target_ship_image_visual",TargetEntityInSpace->spawnArgs.GetString("ship_image_visual", "textures/images_used_in_source/default_ship_image_visual.tga") );
		} else {
			TransporterPad->SetRenderEntityGuisStrings("transporter_pad_target_ship","No Target");
			TransporterPad->SetRenderEntityGuisStrings("transporter_pad_target_ship_image_visual","textures/images_used_in_source/default_no_target.tga");
		}
	}
}

void sbShip::UpdateGuisOnTransporterPadEveryFrame() {
	if ( TransporterPad && TransporterPad->IsType(sbTransporterPad::Type) ) {
		//TransporterPad->SetRenderEntityGuisStrings("transporter_pad_parent_ship",spawnArgs.GetString("name"));
		//TransporterPad->SetRenderEntityGuisStrings("transporter_pad_parent_ship_image_visual",spawnArgs.GetString("ship_image_visual", "textures/images_used_in_source/default_ship_image_visual.tga") );
		if (  consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
			TransporterPad->SetRenderEntityGuisFloats("transporter_pad_current_charge_percentage",consoles[COMPUTERMODULEID]->ControlledModule->current_charge_percentage);
		} else {
			TransporterPad->SetRenderEntityGuisFloats("transporter_pad_current_charge_percentage",0.0f);
		}
		if ( TargetEntityInSpace ) {
			// transporter status
			if (  consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
				if ( consoles[COMPUTERMODULEID]->ControlledModule->current_charge_percentage >= 1.0f ) {
					if ( ( (float)TargetEntityInSpace->shieldStrength/(float)TargetEntityInSpace->max_shieldStrength < TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters ) || TargetEntityInSpace->team == team ) {
						TransporterPad->SetRenderEntityGuisStrings("transporter_status","Initiate Transport");
					} else {
						TransporterPad->SetRenderEntityGuisStrings("transporter_status","Target Shields High");
					}
				} else if ( consoles[COMPUTERMODULEID]->ControlledModule->current_charge_percentage < 1.0f ) {
					if ( consoles[COMPUTERMODULEID]->ControlledModule->module_efficiency > 0 ) {
						TransporterPad->SetRenderEntityGuisStrings("transporter_status","Charging...");
					} else if ( consoles[COMPUTERMODULEID]->ControlledModule->health <= 0 ) {
						TransporterPad->SetRenderEntityGuisStrings("transporter_status","Computer Disabled");
					} else if ( consoles[COMPUTERMODULEID]->ControlledModule->power_allocated <= 0 ) {
						TransporterPad->SetRenderEntityGuisStrings("transporter_status","No Power");
					}
				}
			} else {
				TransporterPad->SetRenderEntityGuisStrings("transporter_status","NO COMPUTER");
			}
		} else {
			TransporterPad->SetRenderEntityGuisStrings("transporter_status","No Destination");
		}
	}
}

void sbShip::HandleModuleAndConsoleGUITimeCommandsEvenIfGUIsNotActive() { // cmds are not done in onTime when a GUI is not active - this forces them to but only for this ship's modules and consoles when the player is on board
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && ( gameLocal.GetLocalPlayer()->ShipOnBoard == this || (gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony) ) ) {
		for ( int module_id = 0; module_id < MAX_MODULES_ON_SHIPS; module_id++ ) {
			if ( consoles[module_id] && consoles[module_id]->GetRenderEntity() ) {
				for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
					if ( consoles[module_id]->GetRenderEntity()->gui[i] && !consoles[module_id]->GetRenderEntity()->gui[i]->Active() ) {
						const char	*cmd;
						sysEvent_t  ev;

						ev.evType = SE_NONE;
						consoles[module_id]->GetRenderEntity()->gui[i]->handle_time_commands_when_not_active = true;

						cmd = consoles[module_id]->GetRenderEntity()->gui[i]->HandleEvent( &ev, gameLocal.time );
						consoles[module_id]->HandleGuiCommands(consoles[module_id],cmd);
						consoles[module_id]->GetRenderEntity()->gui[i]->StateChanged(gameLocal.time, true);
					}
				}

				if ( consoles[module_id]->ControlledModule && consoles[module_id]->ControlledModule->GetRenderEntity() ) {
					for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
						if ( consoles[module_id]->ControlledModule->GetRenderEntity()->gui[i] && !consoles[module_id]->ControlledModule->GetRenderEntity()->gui[i]->Active() ) {
							const char	*cmd;
							sysEvent_t  ev;

							ev.evType = SE_NONE;
							consoles[module_id]->ControlledModule->GetRenderEntity()->gui[i]->handle_time_commands_when_not_active = true;

							cmd = consoles[module_id]->ControlledModule->GetRenderEntity()->gui[i]->HandleEvent( &ev, gameLocal.time );
							consoles[module_id]->ControlledModule->HandleGuiCommands(consoles[module_id]->ControlledModule,cmd);
							consoles[module_id]->ControlledModule->GetRenderEntity()->gui[i]->StateChanged(gameLocal.time, true);
						}
					}
				}
			}
		}
	}
}

void sbShip::BecomeDerelict() {
	if ( !never_derelict ) {
		is_derelict = true;
		always_neutral = true; // derelict ships are always neutral

		//BecomeNeutralShip();
		/* // Changed FROM this on 07 03 2016
		BecomeASpacePirateShip(); // give it a unique team - but make everyone neutral with it.
		for ( int i = 0; i < gameLocal.num_entities; i++ ) {
			if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {
				dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->StartNeutralityWithTeam( team );
				StartNeutralityWithTeam( dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->team );
			}
		}
		*/
		neutral_teams.clear(); // Changed TO this on 07 03 2016
		ChangeTeam(ALWAYS_NEUTRAL_TEAM); // Changed TO this on 07 03 2016

		CeaseFiringWeaponsAndTorpedos();

		gameLocal.GetLocalPlayer()->UpdateNotificationList( "^0" "The ^8" + name + " ^0has become a derelict vessel." );
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^0" "The ^8" + name + " ^0has become a derelict vessel." );
		}
		name += idStr(" - Derelict"); // BOYETTE NOTE TODO: going to make sure this is removed if the ship is taken over again by anyone (the player) - should just need to do this: name = spawnArgs.GetString("name"); - since the spawnArg name is still the original name.
		// BOYETTE NOTE TODO: need to reduce the power on all the modules except for oxygen. turn off automanage power - need to set a boolean that let's the script know to do nothing since the ship is derelict
		// gameLocal.GetLocalPlayer()->PopulateShipList(); // not necessary right now but could be in the future.
		// need to put logic in here to disable the ship's script - or send a boolean to the ship's script so it doesn't do anything.

		SetShaderParm(11, 1.0f ); // set the derelict decal alpha

		// if this is not our ship on board this will make everything look really derelict (except the environment module which will still allow the player to come aboard).
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard != this ) {
			CancelRedAlert();
			//TurnShipLightsOff();
			DimRandomShipLightsOff();

			for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
				if ( x != ENVIRONMENTMODULEID ) {
					if ( consoles[x] && consoles[x]->ControlledModule ) {
						consoles[x]->Damage(NULL,NULL,idVec3(0,0,0),"damage_massive",1000,0); // damage the console
						consoles[x]->ControlledModule->Damage(NULL,NULL,idVec3(0,0,0),"damage_massive",1000,0); // damage the console's module
					}
				}
			}
			Event_SetMinimumModulePowers();
		}

		// disable shields so we can always get aboard it
		if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
			consoles[SHIELDSMODULEID]->Damage(NULL,NULL,idVec3(0,0,0),"damage_massive",1000,0); // damage the console
			consoles[SHIELDSMODULEID]->ControlledModule->Damage(NULL,NULL,idVec3(0,0,0),"damage_massive",1000,0); // damage the console's module
			shieldStrength = shieldStrength / 4;
		}

		// stop firing on this ship if it becomes derelict - we can always fire on it again if we really do want to destroy it
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
			gameLocal.GetLocalPlayer()->PlayerShip->CeaseFiringWeaponsAndTorpedos();
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^2 Your ship's ^0 auto-fire has been deactivated." );
		}

		// notify the player
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->SelectedEntityInSpace ) {
			if ( gameLocal.GetLocalPlayer()->SelectedEntityInSpace == this ) {
				if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
					gameLocal.GetLocalPlayer()->HailGui->HandleNamedEvent("HailedSelectedShipHasBecomeDerelict");
				}
				gameLocal.GetLocalPlayer()->SelectedEntityInSpace->currently_in_hail = false;
				//gameLocal.GetLocalPlayer()->SelectedEntityInSpace = NULL;
			}
		}

		// UPDATE THE MUSIC AND QUICK WARP STATUS BEGIN
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
			gameLocal.GetLocalPlayer()->PlayerShip->UpdatePlayerShipQuickWarpStatus();
		}
		// UPDATE THE MUSIC AND QUICK WARP STATUS END
	}
}

void sbShip::BecomeNonDerelict() {
	is_derelict = false;
	name = spawnArgs.GetString("name");
	SetShaderParm(11, 0.0f ); // set the derelict decal alpha
}

void sbShip::InitiateSelfDestructSequence() {
	ship_self_destruct_sequence_initiated = true;
	ship_self_destruct_sequence_timer = gameLocal.time;
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Ship Self-Destruct Sequence: ^1Initiated^0");
	}
}
void sbShip::CancelSelfDestructSequence() {
	ship_self_destruct_sequence_initiated = false;
	ship_self_destruct_sequence_timer = 0;
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^4Ship Self-Destruct Sequence: ^4Cancelled^0");
	}
}
void sbShip::EvaluateSelfDestructSequenceTimer() {
	if ( ship_self_destruct_sequence_initiated ) {
		if ( (gameLocal.time - ship_self_destruct_sequence_timer) > 60000 ) {  // this is one minute
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1Your Ship Has Self-Destructed^0");
			}
			BeginShipDestructionSequence();
			CancelSelfDestructSequence();
		}
	}
}

void sbShip::SetToNullAllPointersToThisEntity() {
	idEntity*	ent;
	sbShip*		ship;
	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType(sbShip::Type) ) {
			ship = static_cast<sbShip*>(ent);
			if ( ship->TargetEntityInSpace && ship->TargetEntityInSpace == this ) {
				ship->TargetEntityInSpace = NULL;
				for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
					if ( ship->consoles[x] && ship->consoles[x]->ControlledModule ) {
						ship->consoles[x]->ControlledModule->CurrentTargetModule = NULL;
					}
				}
			}
			if ( ship->TempTargetEntityInSpace && ship->TempTargetEntityInSpace == this ) {
				ship->TempTargetEntityInSpace = NULL;
			}
			if ( ship->ship_that_just_fired_at_us && ship->ship_that_just_fired_at_us == this ) {
				ship->ship_that_just_fired_at_us = NULL;
			}

			// if we are the main goal of any other ships, clear all their goals
			if ( ship->main_goal.goal_entity == this ) {
				ship->main_goal.goal_action = 0;
				ship->main_goal.goal_entity = NULL;
				//ship->mini_goals.clear(); // happens in ship->Event_SetMainGoal
				ship->Event_SetMainGoal(SHIP_AI_IDLE,NULL);
				if ( ship->red_alert ) {
					ship->CancelRedAlert();
				}
			}

			// remove us from their mini_goals
			for ( int x = 0; x < ship->mini_goals.size(); x++ ) {
				if ( ship->mini_goals[x].goal_entity && ship->mini_goals[x].goal_entity == this ) {
					ship->mini_goals.erase(ship->mini_goals.begin() + x);
				}
			}

			if ( ship->priority_space_entity_to_target == this ) {
				ship->priority_space_entity_to_target = NULL;
			}
			if ( ship->priority_space_entity_to_protect == this ) {
				ship->priority_space_entity_to_protect = NULL;
			}
		}
	}

	// remove us as the ai shiponboard for any ai on board
	for ( int x = 0; x < AIsOnBoard.size() ; x++ ) {
		if ( AIsOnBoard[x] ) {
			AIsOnBoard[x]->ShipOnBoard = NULL;
		}
	}

	// remove us as the parentship for any of our crew
	for ( int x = 0; x < MAX_CREW_ON_SHIPS ; x++ ) {
		if ( crew[x] && crew[x]->ParentShip && crew[x]->ParentShip == this ) {
			crew[x]->ParentShip = NULL;
		}
	}

	// remove us from their ships_at_my_stargrid_position vector
	LeavingStargridPosition(stargridpositionx,stargridpositiony);

	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->PlayerShip = NULL;
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->ShipOnBoard = NULL;
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->SelectedEntityInSpace && gameLocal.GetLocalPlayer()->SelectedEntityInSpace == this ) {
		gameLocal.GetLocalPlayer()->SelectedEntityInSpace = NULL;
	}
}

void sbShip::SetupWarpInVisualEffects( int ms_from_now ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->PlayerShip->should_warp_in_when_first_encountered = false;
		return;
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		gameLocal.GetLocalPlayer()->ShipOnBoard->should_warp_in_when_first_encountered = false;
		return;
	}
	Hide();
	if ( GetPhysics() ) {
		GetPhysics()->SetContents( 0 ); // set non-solid
	}
	UpdateVisuals();
	Present();
	if ( ShieldEntity && !ShieldEntity->IsHidden() ) {
		ShieldEntity->Hide();
		if ( ShieldEntity->GetPhysics() ) {
			ShieldEntity->GetPhysics()->SetContents( 0 ); // set non-solid
		}
		ShieldEntity->SetShaderParm(10,0.0f);
	}
	PostEventMS( &EV_DoWarpInVisualEffects, ms_from_now );

	ReduceAllModuleChargesToZero(); // to prevent firing
}
void sbShip::Event_DoWarpInVisualEffects( void ) {
	idEntityFx::StartFx( spawnArgs.GetString("spaceship_warp_fx", "fx/spaceship_warp_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, true );
	gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_initiate_warp_in_space", "initiate_warp_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL );
	PostEventMS( &EV_FinishWarpInVisualEffects, 1500 );
}
void sbShip::Event_FinishWarpInVisualEffects( void ) {
	Show();
	if ( GetPhysics() ) {
		GetPhysics()->SetContents( CONTENTS_SOLID ); // set solid
	}
	UpdateVisuals();
	RunPhysics();
	Present();
	if ( ShieldEntity && shieldStrength > 0 && ShieldEntity->IsHidden() ) {
		ShieldEntity->Show();
		if ( ShieldEntity->GetPhysics() ) {
			ShieldEntity->GetPhysics()->SetContents( CONTENTS_SOLID ); // set solid
		}
	}
	should_warp_in_when_first_encountered = false;
	spawnArgs.SetBool( "should_warp_in_when_first_encountered", false );

	//ReduceAllModuleChargesToZero(); // to prevent firing

	idStr text_color;
	if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^1"; }
	if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^8"; }
	if ( team == gameLocal.GetLocalPlayer()->team ) { text_color = "^4"; }
	gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( text_color + "The " + name + " has arrived in " + text_color + "local " + text_color + "space.^0");
}

void sbShip::MaybeSpawnAWreck() {
	if ( spawnArgs.GetString( "wreck_to_spawn" , NULL ) != NULL ) {
		idDict	wreck_Def;
		wreck_Def.Set( "classname", spawnArgs.GetString( "wreck_to_spawn" ) ); //spawnArgs.GetString( "wreck_to_spawn" ) );
		wreck_Def.Set( "origin", GetPhysics()->GetOrigin().ToString() );
		wreck_Def.SetInt("stargridstartposx", stargridpositionx );
		wreck_Def.SetInt("stargridstartposy", stargridpositiony );
		wreck_Def.SetBool("stargridstartpos_random", "0");
		wreck_Def.SetBool("stargridstartposx_random", "0");
		wreck_Def.SetBool("stargridstartposy_random", "0");
		wreck_Def.SetBool("initially_discovered_by_player", "1");
		wreck_Def.SetBool("has_artifact_aboard", "0");
		wreck_Def.SetBool("was_sensor_scanned", "1");
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
			wreck_Def.SetBool("ship_begin_dormant", "0");
		}
		wreck_Def.Set( "name", original_name + " Wreck" );
		idEntity*	pointer_to_spawned_entity = NULL;
		gameLocal.SpawnEntityDef( wreck_Def, &pointer_to_spawned_entity, false );
		if ( pointer_to_spawned_entity->IsType( sbShip::Type ) ) {
			sbShip* wreck = NULL;
			wreck = static_cast<sbShip*>(pointer_to_spawned_entity);
			wreck->original_name = original_name + " Wreck";
			wreck->name = original_name + " Wreck";
			wreck->DoStuffAfterAllMapEntitiesHaveSpawned();
			wreck->DoStuffAfterPlayerHasSpawned();
		}
	}
}

void sbShip::BeginShipDestructionSequence() {

	if ( ship_destruction_sequence_initiated == false ) {

		CeaseFiringWeaponsAndTorpedos();
		TargetEntityInSpace = NULL;
		TempTargetEntityInSpace = NULL;
		ship_that_just_fired_at_us = NULL;

		main_goal.goal_action = 0;
		main_goal.goal_entity = NULL;
		Event_SetMainGoal(SHIP_AI_IDLE,NULL);
		if ( red_alert ) {
			CancelRedAlert();
		}

		try_to_be_dormant = true;

		idEvent::CancelEvents( this );

DeconstructScriptObject();
scriptObject.Free();

if ( thinkFlags ) {
	BecomeInactive( thinkFlags );
}
activeNode.Remove();



		// for hail gui begin
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->SelectedEntityInSpace ) {
			if ( gameLocal.GetLocalPlayer()->SelectedEntityInSpace == this ) {
				if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
					gameLocal.GetLocalPlayer()->HailGui->HandleNamedEvent("HailedSelectedShipHasBeenDestroyed");
				}
				gameLocal.GetLocalPlayer()->SelectedEntityInSpace->currently_in_hail = false;
				gameLocal.GetLocalPlayer()->SelectedEntityInSpace = NULL;
			}
		}
		// for hail gui end
		// local space notification begin
		int PlayerPosX;
		int PlayerPosY;
		if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
			PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
			PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
		} else {
			PlayerPosX = 0;
			PlayerPosY = 0;
		}
		if ( stargridpositionx == PlayerPosX && stargridpositiony == PlayerPosY ) {
			idStr text_color;
			if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^1"; }
			if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { text_color = "^8"; }
			if ( team == gameLocal.GetLocalPlayer()->team ) { text_color = "^4"; }
			if ( !ship_deconstruction_sequence_initiated ) {
				int materials_to_give_player_on_destruction = spawnArgs.GetInt("materials_to_give_player_on_destruction", "0");
				if ( materials_to_give_player_on_destruction > 0 && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( text_color + "The " + name + " has been " + text_color + "destroyed.\n"  + "^0" + idStr(materials_to_give_player_on_destruction)  + text_color + " materials recieved." );
				} else {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( text_color + "The " + name + " has been " + text_color + "destroyed." );
				}
			}
		}
		// local space notification end

		ship_destruction_sequence_initiated = true;

		if ( projectile.GetEntity() ) {
			projectile.GetEntity()->CancelEvents( &EV_CheckTorpedoStatus );
			projectile.GetEntity()->CancelEvents( &EV_Remove );
			projectile.GetEntity()->Event_Remove();
			projectile = NULL;
		}

		 // added 10/03/2016 - if this is the playership, then put any crew that is not on board in auto mode since the player can't control them any more since there is no PlayerShip
		for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] ) {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard != this ) {
					crew[i]->crew_auto_mode_activated = true;
					//crew[i]->player_follow_mode_activated = true; // this would increase the chance of them blocking the player
				}
			}
		}

		for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
			AIsOnBoard[i]->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",800,0);
			AIsOnBoard[i]->ShipOnBoard = NULL;
		}
		AIsOnBoard.clear();

		if ( gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
			gameLocal.GetLocalPlayer()->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",800,0);
		}

		// the officers
		for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] ) {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard != this ) { // if this is the playership, then put them in auto mode since the player can't control them any more since there is no PlayerShip
					crew[i]->crew_auto_mode_activated = true;
					//crew[i]->player_follow_mode_activated = true; // this would increase the chance of them blocking the player
				}
				crew[i]->ParentShip = NULL;
			}
			crew[i] = NULL;
		}

		// the room nodes
		for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
			if ( room_node[i] ) room_node[i]->Event_Remove();
			room_node[i] = NULL;
		}




		// shoot out explosion effects from the modules if this is the ship the player is aboard
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) { // BOYETTE NOTE: only shoot these out here if this is the player ship on board since this is just a visual effect since during ship destruction all the crew are going to be removed.
			for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {		
				if ( consoles[i] && consoles[i]->ControlledModule ) consoles[i]->ControlledModule->RecieveShipToShipDamage( this , 1000 );
				if ( consoles[i] && consoles[i]->ControlledModule ) consoles[i]->ControlledModule->RecieveShipToShipDamage( this , 1000 );
			}
		}

		// the console's modules
		for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			// we do this twice to shoot out more projectiles.
			if ( consoles[i] && consoles[i]->ControlledModule ) consoles[i]->ControlledModule->Event_Remove();
			if ( consoles[i] ) consoles[i]->ControlledModule = NULL;
			if ( consoles[i] ) consoles[i]->Event_Remove();
			consoles[i] = NULL;
		}

		// the captain chair
		if ( CaptainChair ) {
			if ( CaptainChair->SeatedEntity.GetEntity() ) {
				if ( gameLocal.GetLocalPlayer() && CaptainChair->SeatedEntity.GetEntity() == gameLocal.GetLocalPlayer() ) {
					CaptainChair->ReleasePlayerCaptain();
				}
			}
			CaptainChair->Event_Remove();
		}
		CaptainChair = NULL;
		// the ready room captain chair
		if ( ReadyRoomCaptainChair ) {
			if ( ReadyRoomCaptainChair->SeatedEntity.GetEntity() ) {
				if ( gameLocal.GetLocalPlayer() && ReadyRoomCaptainChair->SeatedEntity.GetEntity() == gameLocal.GetLocalPlayer() ) {
					ReadyRoomCaptainChair->ReleasePlayerCaptain();
				}
			}
			ReadyRoomCaptainChair->Event_Remove();
		}
		ReadyRoomCaptainChair = NULL;

		if ( ShieldEntity ) ShieldEntity->Event_Remove();
		if ( TransporterBounds ) TransporterBounds->Event_Remove();
		if ( TransporterPad ) TransporterPad->Event_Remove();
		if ( TransporterParticleEntitySpawnMarker ) TransporterParticleEntitySpawnMarker->Event_Remove();
		if ( ShipDiagramDisplayNode ) ShipDiagramDisplayNode->Event_Remove();
		if ( ViewScreenEntity ) ViewScreenEntity->Event_Remove();
		ShieldEntity = NULL;
		TransporterBounds = NULL;
		TransporterPad = NULL;
		TransporterParticleEntitySpawnMarker = NULL;
		ShipDiagramDisplayNode = NULL;
		ViewScreenEntity = NULL;

		if ( TransporterParticleEntityFX.GetEntity() ) {
			TransporterParticleEntityFX.GetEntity()->Event_Remove();
			TransporterParticleEntityFX = NULL;
		}

		// the ship's doors
		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) ) {
				dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->RemoveDoorGroup();
				shipdoors[ i ] = NULL;
			}
			//dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->GetActivateChain()->Event_Remove();
			//shipdoors[ i ].GetEntity() = NULL;
			//shipdoors[ i ] = NULL;
		}

		PostEventMS( &EV_ConcludeShipDestructionSequence, 4000 ); // the ship will have smaller explosions for 4 seconds until it has a large explosion. // BOYETTE NOTE TODO: might want to make this a spawnarg - then the mapper would just have to match up the fx with the destruction time.

		// DONE: BOYETTE NOTE TODO: only need to do this special effect if the stargrid position is the same as the player shiponboard.
		if ( !ship_deconstruction_sequence_initiated ) {
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				idEntityFx::StartFx( spawnArgs.GetString("destruction_in_progress_fx", "fx/spaceship_destruction_in_progress_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, true ); // BOYETTE NOTE TODO: might want to make this a spawnarg - then the mapper would just have to match up the fx with the destruction time.
		
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress", "destruction_in_progress_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress_in_space", "destruction_in_progress_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
			}
		} else {
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {

				// DECONSTRUCT SHADER EFFECT BEGIN
				// BOYETTE NOTE IMPORTANT: this is now done on the command on idPlayer - 12/21/15 - actually I moved it back to here for various reaons - but made it a little more clean
				/*
				idEntityFx::StartFx( spawnArgs.GetString("deconstruction_fx", "fx/ship_deconstruct_default"), &(GetPhysics()->GetOrigin()), 0, this, true ); // BOYETTE NOTE TODO: might want to make this a spawnarg - then the mapper would just have to match up the fx with the destruction time.
				noGrab = true;
				renderEntity.noShadow = true;
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
				UpdateVisuals();
				*/
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) { // the playership is the only one we will allow to deconstruct other ships - so this should work fine and solve issues with the deconstruct de-particalize effect not working properly
					BeginTransporterMaterialShaderEffect(gameLocal.GetLocalPlayer()->PlayerShip->fx_color_theme);
				}

				// DECONSTRUCT SHADER EFFECT END

				// put deconstruction sound effect here // or maybe it will just be in the fx
				/*
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress", "destruction_in_progress_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_in_progress_in_space", "destruction_in_progress_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				*/
			}
		}
		//idEntityFx::StartFx( "spawnArgs.GetString("spaceship_warp_fx", "fx/spaceship_warp_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, true ); // the last arg is whether or not to bind the fx to the ship - it is set to true here  - it will move and be removed with the ship.

		// update all the player menus:
		bool player_should_get_a_new_target = false;
		if ( gameLocal.GetLocalPlayer() ) {
			if ( gameLocal.GetLocalPlayer()->PlayerShip ) {
				if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
					gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace = NULL;
					player_should_get_a_new_target = true;
				}
				if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) { // close the captain menu or the hail menu if the playership is destroyed
					if ( gameLocal.GetLocalPlayer()->guiOverlay ) {
						if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->CaptainGui ) {
							gameLocal.GetLocalPlayer()->CloseOverlayCaptainGui();
						} else if ( gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
							gameLocal.GetLocalPlayer()->CloseOverlayHailGui();
						}
					}
				}
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->hud_map_visible = false; // BOYETTE NOTE: although the player should already be dead so this should not be an issue - but maybe it is good so the player won't see it when they are killed by the ShipOnBoard destruction
				}
				gameLocal.GetLocalPlayer()->UpdateDoorIconsOnShipDiagramsOnce();
				gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
				gameLocal.GetLocalPlayer()->UpdateCaptainHudOnce();
				gameLocal.GetLocalPlayer()->PlayerShip->UpdateGuisOnCaptainChair();
				gameLocal.GetLocalPlayer()->PlayerShip->UpdateGuisOnTransporterPad();
			}
		}

		SetToNullAllPointersToThisEntity();
		LeavingStargridPosition(stargridpositionx,stargridpositiony); // to clear ourselves out of the ships_at_my_stargrid_position vector of all other ships

		if ( player_should_get_a_new_target ) {
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
				gameLocal.GetLocalPlayer()->PlayerShip->Event_GetATargetShipInSpace();
				if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
					gameLocal.GetLocalPlayer()->SelectedEntityInSpace = gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace;
				}
				gameLocal.GetLocalPlayer()->UpdateSelectedEntityInSpaceOnGuis();
			}
		}

	}
}


void sbShip::Event_ConcludeShipDestructionSequence( void ) {
	CeaseFiringWeaponsAndTorpedos();
	TargetEntityInSpace = NULL;
	TempTargetEntityInSpace = NULL;
	ship_that_just_fired_at_us = NULL;

		main_goal.goal_action = 0;
		main_goal.goal_entity = NULL;
		Event_SetMainGoal(SHIP_AI_IDLE,NULL);
		if ( red_alert ) {
			CancelRedAlert();
		}

		try_to_be_dormant = true;

			idEvent::CancelEvents( this );

		// Used to be in Event_ConcludeShipDestructionSequence BEGIN
		ship_destruction_sequence_initiated = false;

		LeavingStargridPosition(stargridpositionx,stargridpositiony); // to clear ourselves out of the ships_at_my_stargrid_position vector of all other ships
		SetToNullAllPointersToThisEntity(); // to clear all pointers to this entity(on ships) // just for good measure - this function is already called above
		for( int i = 0; i < shiplights.Num(); i++ ) {
			for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
				if ( shiplights[ i ].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
					dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->Event_Remove();
					shiplights[ i ].GetEntity()->targets[ix] = NULL;
				}
			}
		}
		for( int i = 0; i < shiplights.Num(); i++ ) {
			if ( shiplights[ i ].GetEntity() ) {
				shiplights[ i ].GetEntity()->Event_Remove();
				shiplights[ i ] = NULL;
			}
		}
		// BOYETTE NOTE TODO: check this: this should already be done before the destruction sequence is initiated - but we might want to do it again just to be sure. - maybe we can look into this sometime in the futre.
		//if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
		//	gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace = NULL;
		//}

		// BOYETTE NOTE TODO: only need to do this special effect if the stargrid position is the same as the player shiponboard.
		if ( !ship_deconstruction_sequence_initiated ) {
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				idEntityFx::StartFx( spawnArgs.GetString("destruction_conclusion_fx", "fx/spaceship_destruction_conclusion_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, false );

				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion", "destruction_conclusion_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // BOYETTE NOTE: This one doesn't get played because their is no player ship on board by this point - it has already been destroyed - which is fine. - will leave this here for reference.
				} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion_in_space", "destruction_conclusion_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
			}
		} else {
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				/*
				idEntityFx::StartFx( spawnArgs.GetString("destruction_conclusion_fx", "fx/spaceship_destruction_conclusion_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, false );

				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion", "destruction_conclusion_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // BOYETTE NOTE: This one doesn't get played because their is no player ship on board by this point - it has already been destroyed - which is fine. - will leave this here for reference.
				} else if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString( "snd_destruction_conclusion_in_space", "destruction_conclusion_in_space_snd_default") ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
				*/
			}
		}
		//idEntityFx::StartFx( spawnArgs.GetString("spaceship_warp_fx", "fx/spaceship_warp_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, false ); // the last arg is whether or not to bind the fx to the ship - it is set to false here  - it will move and be removed with the ship.

		// UPDATE THE MUSIC AND QUICK WARP STATUS BEGIN
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
			gameLocal.GetLocalPlayer()->PlayerShip->UpdatePlayerShipQuickWarpStatus();

			idStr the_song_of_this_ship;
			if ( is_derelict ) {
				the_song_of_this_ship = "derelict_music_long";
			} else if ( team != gameLocal.GetLocalPlayer()->team ) {
				the_song_of_this_ship = spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" );
			} else {
				the_song_of_this_ship = spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" );
			}
			if ( gameLocal.GetLocalPlayer()->currently_playing_music_shader == the_song_of_this_ship ) {
				//gameLocal.GetLocalPlayer()->DeterminePlayerMusic(); // BOYETTE NOTE TODO IMPORTANT: I'm not sure if we want to do this  it might be best to just let the music play out and then we will just hear the alarms
			}
		}
		// UPDATE THE MUSIC AND QUICK WARP STATUS END

		// schedule some visual things to update after this ship is removed.
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard ) {
			gameLocal.GetLocalPlayer()->ScheduleThingsToUpdateAftersbShipIsRemoved();
		}

		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
			gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false);
			gameLocal.GetLocalPlayer()->ShipOnBoard = NULL;
		}

		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->grabbed_reserve_crew_dict = NULL;
			gameLocal.GetLocalPlayer()->PlayerShip = NULL;
		}

		if ( !ship_deconstruction_sequence_initiated ) {
			MaybeSpawnAWreck();
		}

		// remove the ship itself
		Event_Remove();

		return;

		// Used to be in Event_ConcludeShipDestructionSequence END

}

void sbShip::GiveThisShipCurrency( int amount_to_give ) {
	if ( amount_to_give != 0 ) {
		current_currency_reserves += amount_to_give;
		current_currency_reserves = idMath::ClampInt(0,999999,current_currency_reserves);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			if ( !gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->hud ) {
				gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( "AcquireCurrency" );
			}
			// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
			if ( common->m_pStatsAndAchievements ) {
				common->m_pStatsAndAchievements->m_nTotalCreditsAcquired += amount_to_give;
				common->StoreSteamStats();
			}
#endif
			// BOYETTE STEAM INTEGRATION END
		}
	}
}
void sbShip::GiveThisShipMaterials( int amount_to_give ) {
	if ( amount_to_give != 0 ) {
		current_materials_reserves += amount_to_give;
		current_materials_reserves = idMath::ClampInt(0,999999,current_materials_reserves);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			if ( !gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->hud ) {
				gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( "AcquireMaterials" );
			}
			// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
			if ( common->m_pStatsAndAchievements ) {
				common->m_pStatsAndAchievements->m_nTotalMaterialsAcquired += amount_to_give;
				common->StoreSteamStats();
			}
#endif
			// BOYETTE STEAM INTEGRATION END
		}
	}
}

void sbShip::InitiateShipRepairMode() {
	if ( hullStrength < max_hullStrength && current_materials_reserves > 0 ) {
		in_repair_mode = true;
		PostEventMS( &EV_EvaluateShipRepairModeCycle, TIME_BETWEEN_SHIP_REPAIR_CYCLES );
	} else {

		if ( hullStrength >= max_hullStrength && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^8YOUR SHIP'S HULL IS FULLY ^8REPAIRED" );
		} else if ( current_materials_reserves <= 0 && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1YOU ARE OUT OF MATERIALS ^1FOR SHIP REPAIR" );
		}
	}
}

void sbShip::RepairShipWithMaterials( int amount_to_repair ) {
	bool reason_out_of_materials = false;
	bool reason_ship_is_fully_repaired = false;

	if ( current_materials_reserves > 0 ) {
		if ( max_hullStrength - hullStrength > amount_to_repair ) {
			if ( current_materials_reserves > amount_to_repair ) {
				current_materials_reserves = current_materials_reserves - amount_to_repair;
				hullStrength = hullStrength + amount_to_repair;
			} else {
				hullStrength = hullStrength + current_materials_reserves;
				current_materials_reserves = 0;
				in_repair_mode = false;
				reason_out_of_materials = true;
			}
		} else if ( max_hullStrength - hullStrength <= amount_to_repair ) {
			if ( current_materials_reserves > max_hullStrength - hullStrength ) {
				current_materials_reserves = current_materials_reserves - ( max_hullStrength - hullStrength );
				hullStrength = hullStrength + (max_hullStrength - hullStrength);
				in_repair_mode = false;
				reason_ship_is_fully_repaired = true;
			} else {
				hullStrength = hullStrength + current_materials_reserves;
				current_materials_reserves = 0;
				in_repair_mode = false;
				reason_out_of_materials = true;
			}
		} else if ( hullStrength >= max_hullStrength ) {
			in_repair_mode = false;
			reason_ship_is_fully_repaired = true;
		}
	} else {
		in_repair_mode = false;
		reason_out_of_materials = true;
		if ( reason_out_of_materials && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1YOU ARE OUT OF MATERIALS ^1FOR SHIP REPAIR" );
			SetShaderParm( 10, 1.0f -( (float)hullStrength / (float)max_hullStrength ) ); // set the damage decal alpha
			return;
		}
	}

	if ( hullStrength >= max_hullStrength || ( reason_ship_is_fully_repaired && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) ) {
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^8YOUR SHIP'S HULL IS FULLY ^8REPAIRED" );
	} else {
		gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^8REPAIR CYCLE COMPLETE" );
	}
	SetShaderParm( 10, 1.0f -( (float)hullStrength / (float)max_hullStrength ) ); // set the damage decal alpha
}

void sbShip::CancelShipRepairMode() {
	in_repair_mode = false;
}

void sbShip::ReduceAllModuleChargesToZero() {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule ) consoles[i]->ControlledModule->current_charge_amount = 0;
	}
}

bool sbShip::CheckForHostileEntitiesAtCurrentStarGridPosition() {
	bool found_hostile_entity = false;
	for ( int i = 0; i < ships_at_my_stargrid_position.size() ; i++ ) {
		if ( ships_at_my_stargrid_position[ i ] && ships_at_my_stargrid_position[ i ]->IsType(sbShip::Type) ) {
			if ( ships_at_my_stargrid_position[ i ]->team != team && !HasNeutralityWithTeam(ships_at_my_stargrid_position[ i ]->team) ) {
				found_hostile_entity = true;
				break;
			}
		}
	}

	return found_hostile_entity;
}
bool sbShip::CheckForHostileEntitiesOnBoard() {
	bool found_hostile_entity = false;
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[ i ] ) {
			if ( AIsOnBoard[ i ]->team != team && !HasNeutralityWithTeam(AIsOnBoard[ i ]->team) ) {
				found_hostile_entity = true;
				break;
			}
		}
	}

	return found_hostile_entity;
}

bool sbShip::CanHireCrew() {
	bool can_hire_crew = false;
	for( int i = 0; i < MAX_CREW_ON_SHIPS - 1; i++ ) { // MAX_CREW_ON_SHIPS - 1 because we want to exclude the position of captain - captains will not be allowed to be hired. 
		if ( crew[i] == NULL ) {
			can_hire_crew = true;
			break;
		}
	}
	return can_hire_crew;
}
bool sbShip::CanHireReserveCrew() {
	bool can_hire_reserve_crew = false;
	if ( reserve_Crew.size() < max_reserve_crew ) {
		can_hire_reserve_crew = true;
	}
	return can_hire_reserve_crew;
}

int sbShip::HireCrew( const char* crew_to_hire_entity_def_name ) {
	int crew_id_hired = -1;
	for( int i = 0; i < MAX_CREW_ON_SHIPS - 1; i++ ) { // MAX_CREW_ON_SHIPS - 1 because we want to exclude the position of captain - captains will not be allowed to be hired. 
		if ( crew[i] == NULL ) {

			crew_id_hired = i;

			idDict		dict;

			dict.Set( "classname", crew_to_hire_entity_def_name );

			if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
				dict.Set( "origin", TransporterBounds->GetPhysics()->GetOrigin().ToString() );
			}
			dict.SetInt( "team", team );
			idEntity*	ent;
			gameLocal.SpawnEntityDef( dict, &ent, false );
			crew[i] = ( idAI * )ent;

			AIsOnBoard.push_back(crew[i]); // add the ai to the list of AI's on board this ship
			crew[i]->ShipOnBoard = this;
			crew[i]->ParentShip = this;

			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->SyncUpPlayerShipNameCVars();
			}

			if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
				// KEEP TRYING RANDOM ORIGINS WITH THE TRANSPORTER BOUNDS UNTIL WE ARE NOT WITHIN THE BOUNDS OF ANOTHER AI
				gameLocal.GetSuitableTransporterPositionWithinBounds(crew[i],&TransporterBounds->GetPhysics()->GetAbsBounds());
			}

			TriggerShipTransporterPadFX();
			

			// NEED TO FIND A WAY TO MOVE THE crew DOWN UNTIL IT IS ON THE GROUND. ALSO NEED TO MAKE SURE IT DOES NOT SPAWN INSIDE OF ANOTHER ENTITY.
			//idVec3		pos;
			//pos =  TransporterBounds->GetPhysics()->GetOrigin();
			//crew[i]->GetFloorPos(64.0f, pos );

			//crew[i]->Init();
			//crew[i]->BecomeActive( TH_THINK );

			break;
		}
	}
	return crew_id_hired;
}

bool sbShip::InviteAIToJoinCrew( idAI* ai_to_invite, bool force_transport, bool attempt_transport, bool allow_to_join_reserves, int transport_delay ) { // returns true if successful
	if ( allow_to_join_reserves ) {
		attempt_transport = true;
	}
	if ( ai_to_invite ) {
		if ( CanHireCrew() ) {
			int crew_id_hired = 0;
			for( int i = 0; i < MAX_CREW_ON_SHIPS - 1; i++ ) { // MAX_CREW_ON_SHIPS - 1 because we want to exclude the position of captain - captains will not be allowed to be join your crew. 
				if ( crew[i] == NULL ) {
					crew[i] = ai_to_invite;
					crew[i]->spawnArgs.SetInt( "team", team );
					crew[i]->team = team;

					if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
						gameLocal.GetLocalPlayer()->SyncUpPlayerShipNameCVars();
					}

					if ( !attempt_transport && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && !crew[i]->ShipOnBoard ) { // if the ai doesn't have a ship on board assume we are in an ai dialogue with it and just make it the player ship on board.
						gameLocal.GetLocalPlayer()->ShipOnBoard->AIsOnBoard.push_back(crew[i]); // add the ai to the list of AI's on board this ship
						crew[i]->ShipOnBoard = gameLocal.GetLocalPlayer()->ShipOnBoard;
					}

					if ( crew[i]->ParentShip && crew[i]->ParentShip != this ) {
						for ( int ix = 0; ix < MAX_CREW_ON_SHIPS; ix++ ) {
							if ( crew[i]->ParentShip->crew[ix] && crew[i] == crew[i]->ParentShip->crew[ix] ) {
								crew[i]->ParentShip->crew[ix] = NULL;
							}
						}
					}
					crew[i]->ParentShip = this;
					crew_id_hired = i;
					//PlayerShip->crew[i]->Init();
					//PlayerShip->crew[i]->BecomeActive( TH_THINK );

					break;
				}
			}

			if ( force_transport && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && TransporterBounds && TransporterBounds->GetPhysics() ) {
				crew[crew_id_hired]->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
				crew[crew_id_hired]->BeginTransporterMaterialShaderEffect(fx_color_theme);
				TriggerShipTransporterPadFX();
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->DialogueAI && gameLocal.GetLocalPlayer()->DialogueAI == crew[crew_id_hired] ) {
					gameLocal.GetLocalPlayer()->CloseOverlayAIDialogeGui();
				}
				gameLocal.GetLocalPlayer()->PlayerShip->PostEventMS( &EV_InitiateOffPadRetrievalTransport, transport_delay, crew[crew_id_hired] );
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					//gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer.");
					gameLocal.GetLocalPlayer()->ScheduleDisplayNonInteractiveAlertMessage( 1000, idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer.");
				}
			} else if ( attempt_transport && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard != gameLocal.GetLocalPlayer()->PlayerShip && TransporterBounds && TransporterBounds->GetPhysics() ) {
				crew[crew_id_hired]->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
				crew[crew_id_hired]->BeginTransporterMaterialShaderEffect(fx_color_theme);
				TriggerShipTransporterPadFX();
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->DialogueAI && gameLocal.GetLocalPlayer()->DialogueAI == crew[crew_id_hired] ) {
					gameLocal.GetLocalPlayer()->CloseOverlayAIDialogeGui();
				}
				gameLocal.GetLocalPlayer()->PlayerShip->PostEventMS( &EV_InitiateOffPadRetrievalTransport, transport_delay, crew[crew_id_hired] );
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					//gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer.");
					gameLocal.GetLocalPlayer()->ScheduleDisplayNonInteractiveAlertMessage( 1000, idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer.");
				}
			} else {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					if ( force_transport || attempt_transport ) {
						gameLocal.GetLocalPlayer()->ScheduleDisplayNonInteractiveAlertMessage( 1000, idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer but was unable to transport to your ship.");
					} else {
						gameLocal.GetLocalPlayer()->ScheduleDisplayNonInteractiveAlertMessage( 1000, idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your crew as the " + role_description[crew_id_hired] + " officer.");
					}
				}
			}
			return true;
		} else if ( CanHireReserveCrew() && allow_to_join_reserves ) {
			if ( attempt_transport && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
				ai_to_invite->TriggerTransporterFX(spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
				ai_to_invite->BeginTransporterMaterialShaderEffect(fx_color_theme);
				TriggerShipTransporterPadFX();
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->DialogueAI && gameLocal.GetLocalPlayer()->DialogueAI == ai_to_invite ) {
					gameLocal.GetLocalPlayer()->CloseOverlayAIDialogeGui();
				}
				gameLocal.GetLocalPlayer()->PlayerShip->PostEventMS( &EV_InitiateOffPadRetrievalTransportToReserveCrew, transport_delay, ai_to_invite );
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
					gameLocal.GetLocalPlayer()->ScheduleDisplayNonInteractiveAlertMessage( 1000, idStr(ai_to_invite->spawnArgs.GetString("npc_name", "This being")) + " has joined your reserve crew.");
				}
			}
			return true;
		} else {
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("Your crew is already at full capacity.");
			}
			return false;
		}
	} else {
		return false;
	}
}

/*
=====================
sbShip::CreateBeamToEnt
=====================
*/
void sbShip::CreateBeamToEnt( idEntity* ent ) {
	if ( ent ) {
		// to create the beam
		if ( ship_beam_active == false ) {
			memset( &ShipBeam.renderEntity, 0, sizeof( renderEntity_t ) );
			if ( GetPhysics() ) {
				ShipBeam.renderEntity.origin = GetPhysics()->GetOrigin();
				ShipBeam.renderEntity.axis = GetPhysics()->GetAxis();
			}
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = spawnArgs.GetFloat( "beam_width", "24.0" );
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			ShipBeam.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
			ShipBeam.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
			ShipBeam.renderEntity.callback = NULL;
			ShipBeam.renderEntity.numJoints = 0;
			ShipBeam.renderEntity.joints = NULL;
			ShipBeam.renderEntity.bounds.Clear();
			ShipBeam.renderEntity.customSkin = declManager->FindSkin( spawnArgs.GetString( "deconstruct_beam_skin", "skins/space_command/deconstruct_beam_default") );
			ShipBeam.modelDefHandle = gameRenderWorld->AddEntityDef( &ShipBeam.renderEntity );
			ship_beam_active = true;
			// to set the target of the beam
			ShipBeam.target = ent;
			gameLocal.Printf( "BEAM CREATED.\n" );
		}
	}
}

/*
=====================
sbShip::UpdateBeamToEnt
=====================
*/
void sbShip::UpdateBeamToEnt() {
	// boyette space command begin
	// to update the beam
	if ( ship_beam_active && ShipBeam.target.GetEntity() != NULL ) {

		idVec3				org;
		
		//org = ShipBeam.target.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter(); // probably want to change this to just the standard origin.
		if ( ShipBeam.target.GetEntity() ) {
			org = ShipBeam.target.GetEntity()->GetPhysics()->GetOrigin(); // probably want to change this to just the standard origin.
		}
		
		ShipBeam.renderEntity.origin = GetPhysics()->GetOrigin();

		ShipBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = org.x;
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = org.y;
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = org.z;
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_RED ] = 
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 
		ShipBeam.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
		gameRenderWorld->UpdateEntityDef( ShipBeam.modelDefHandle, &ShipBeam.renderEntity );

		UpdateVisuals();
		//gameLocal.Printf( "BEAM UPDATED.\n" );
	} else if ( ship_beam_active && ShipBeam.target.GetEntity() == NULL ) {
		RemoveBeamToEnt();
	}
	// boyette space command end
}

/*
=====================
sbShip::RemoveBeamToEnt
=====================
*/
void sbShip::RemoveBeamToEnt() {
	// to remove the beam
	if ( ship_beam_active ) {
		ship_beam_active = false;
		gameRenderWorld->FreeEntityDef( ShipBeam.modelDefHandle );
		ShipBeam.modelDefHandle = -1;
		UpdateVisuals();
		gameLocal.Printf( "BEAM REMOVED.\n" );
		ShipBeam.renderEntity.hModel = NULL;
		ShipBeam.target = NULL;
	}
}

/*
=====================
sbShip::LowerShields
=====================
*/
void sbShip::LowerShields() {
	if ( shields_raised ) {
		shields_raised = false;
		shieldStrength_copy = shieldStrength;
		shieldStrength = 0;
		CancelEvents( &EV_UpdateShieldEntityVisibility );
		if( ShieldEntity ) {
			ShieldEntity->SetShaderParm(10,0.0f);
		}
		//FlashShieldDamageFX(200); // shields are zero so it will get hidden anyways
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->CaptainGui && gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->CaptainGui ) {
			if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "PlayerShipLowerShields" );
			}
			if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
				gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "TargetShipLowerShields" );
			}
		}
	}
}
/*
=====================
sbShip::RaiseShields
=====================
*/
void sbShip::RaiseShields() {
	if ( !shields_raised ) {
		shields_raised = true;
		shieldStrength = shieldStrength_copy;
		FlashShieldDamageFX(1000);
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->CaptainGui && gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->CaptainGui ) {
			if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "PlayerShipRaiseShields" );
			}
			if ( gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
				gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "TargetShipRaiseShields" );
			}
		}
	}
}

/*
================
sbShip::ReturnSuitableTorpedoLaunchPoint
================
*/
// BOYETTE NOTE IMPORTANT: make sure that any entities that could be in the way but should not be an obstacle have "solid" set to "0". This sincludes the stars skysphere.
idVec3 sbShip::ReturnSuitableTorpedoLaunchPoint() {
	if ( GetPhysics() ) {
		if ( TargetEntityInSpace && TargetEntityInSpace->GetPhysics() ) {

			if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin(), TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
				suitable_torpedo_launchpoint_offset.Set(0,0,0);
				return GetPhysics()->GetOrigin();
			}

			// we can re-use the old launchpoint offset if it is still good.
			if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
				return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
			}

			//left//1//suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[0][0],0,0);
			//right//2//suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[1][0],0,0);
			//down//3//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[0][2]);
			//up//4//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[1][2]);
			//back//5//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[0][1],0);
			//front//6//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[1][1],0);

			for (int i = 1; i <= 7; i++) { // well keep expanding our bounds up until 3 times larger
				suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[0][0]*i,0,0);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[1][0]*i,0,0);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[0][2]*i);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[1][2]*i);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[0][1]*i,0);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[1][1]*i,0);
				if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
					return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
				}
				// random
				for (int x = 0; x < 5; x++ ) { // we'll do five attempts to find a random spot within the bounds
					suitable_torpedo_launchpoint_offset.Set(gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][0]*i) - gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][0]*i),gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][1]*i) - gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][1]*i),gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][2]*i) - gameLocal.random.RandomInt(GetPhysics()->GetBounds()[0][2]*i));
					if ( CheckTorpedoLaunchPoint(GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset, TargetEntityInSpace->GetPhysics()->GetOrigin() ) ) {
						return GetPhysics()->GetOrigin() + suitable_torpedo_launchpoint_offset;
					}
				}
			}
		}
				// BOYETTE PRINT BEGIN
				//idFile *file;	
				//file = fileSystem->OpenFileAppend("file_write_test.txt");
				//file->Printf( name + " Couldn't find a torpedo launch point\n" );
				//fileSystem->CloseFile( file );
				// BOYETTE PRINT END
		return GetPhysics()->GetOrigin();
	}
	return vec3_origin;
}

/*
================
sbShip::CheckTorpedoLaunchPoint
================
*/
bool sbShip::CheckTorpedoLaunchPoint(idVec3 start,idVec3 end) {
	idEntity *ent;
	idEntity *bestEnt;
	float scale;
	float bestScale;
	idBounds b;
	bestEnt = NULL;
	bestScale = 1.0f;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType(idEntity::Type) && ent != projectile.GetEntity() && (!ShieldEntity || (ShieldEntity && ent != ShieldEntity)) && ent != this && ent->GetPhysics() && ent->GetPhysics()->GetContents(CONTENTS_SOLID) ) { // BOYETTE NOTE TODO and has to be solid - so maybe this as well ent->GetPhysics()->GetContents() == CONTENTS_SOLID - GetContents() should really be called test contents flag bit - in this case whether or not it is first bit is set
			//b = ent->GetPhysics()->GetAbsBounds().Expand( 16 ); // original
			b = ent->GetPhysics()->GetAbsBounds().Expand( 2 ); // BOYETTE NOTE: maybe we only need to expand it 2
			//b = ent->GetPhysics()->GetAbsBounds(); // BOYETTE NOTE TODO: maybe we don't need to expand it if we reduce the size of size of ship torpedos defs.(max and mins spawnargs)
			if ( b.RayIntersection( start, end-start, scale ) ) {
				if ( scale >= 0.0f && scale < bestScale ) {
					bestEnt = ent;
					bestScale = scale;
				}
			}
		}
	}
				// BOYETTE PRINT BEGIN
				//idFile *file;	
				//file = fileSystem->OpenFileAppend("file_write_test.txt");
				//file->Printf( name + " : entity in the way is: " + bestEnt->name + "\n" );
				//fileSystem->CloseFile( file );
				// BOYETTE PRINT END
	if ( TargetEntityInSpace && ( bestEnt == TargetEntityInSpace || (TargetEntityInSpace->ShieldEntity && bestEnt == TargetEntityInSpace->ShieldEntity) ) ) {
		return true;
	} else {
		return false;
	}
}

/*
================
sbShip::ReturnSuitableWeaponsOrTorpedoTargetPointForMiss
================
*/
// BOYETTE NOTE IMPORTANT: make sure that any entities that could be in the way but should not be an obstacle have "solid" set to "0". This sincludes the stars skysphere.
idVec3 sbShip::ReturnSuitableWeaponsOrTorpedoTargetPointForMiss() {
	idVec3 suitable_torpedo_targetpoint_offset;
	idVec3 launch_point;
	idVec3 target_point;
	idVec3 difference_vec;
	float target_distace_scalar = 5.0f;

	if ( GetPhysics() ) {
		if ( TargetEntityInSpace && TargetEntityInSpace->GetPhysics() ) {
			for (int i = 1; i <= 7; i++) { // well keep expanding our bounds up until 7 times larger
				// random
				for (int x = 0; x < 5; x++ ) { // we'll do five attempts to find a random spot within the ever expanding bounds
					suitable_torpedo_targetpoint_offset.Set(gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][0]*i) - gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][0]*i),gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][1]*i) - gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][1]*i),gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][2]*i) - gameLocal.random.RandomInt(TargetEntityInSpace->GetPhysics()->GetBounds()[0][2]*i));
					launch_point = GetPhysics()->GetOrigin();					
					target_point = TargetEntityInSpace->GetPhysics()->GetOrigin() + suitable_torpedo_targetpoint_offset;
					difference_vec = target_point - launch_point;
					difference_vec = difference_vec * target_distace_scalar;
					target_point = launch_point + difference_vec;
					if ( CheckWeaponsOrTorpedoTargetPointForMiss(GetPhysics()->GetOrigin(), target_point ) ) {
						return target_point;
					}
				}
			}
		}
		return vec3_origin;
	}
	return vec3_origin;
}

/*
================
sbShip::CheckWeaponsOrTorpedoTargetPointForMiss
================
*/
bool sbShip::CheckWeaponsOrTorpedoTargetPointForMiss(idVec3 start,idVec3 end) {
	idEntity *ent;
	idEntity *bestEnt;
	float scale;
	float bestScale;
	idBounds b;
	bestEnt = NULL;
	bestScale = 1.0f;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType(idEntity::Type) && ent != projectile.GetEntity() && (!ShieldEntity || (ShieldEntity && ent != ShieldEntity)) && ent != this && ent->GetPhysics() && ent->GetPhysics()->GetContents(CONTENTS_SOLID) ) { // BOYETTE NOTE TODO and has to be solid - so maybe this as well ent->GetPhysics()->GetContents() == CONTENTS_SOLID - GetContents() should really be called test contents flag bit - in this case whether or not it is first bit is set
			b = ent->GetPhysics()->GetAbsBounds().Expand( 2 ); // guess for the size of the torpedo
			if ( b.RayIntersection( start, end-start, scale ) ) {
				if ( scale >= 0.0f && scale < bestScale ) {
					bestEnt = ent;
					bestScale = scale;
				}
			}
		}
	}
	// make sure we don't hit any ships at our stargrid position
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( bestEnt && ships_at_my_stargrid_position[i] && ( bestEnt == ships_at_my_stargrid_position[i] || (ships_at_my_stargrid_position[i]->ShieldEntity && bestEnt == ships_at_my_stargrid_position[i]->ShieldEntity) ) ) {
			return false; // we hit one
		}
	}
	if ( bestEnt && TargetEntityInSpace && ( bestEnt == TargetEntityInSpace || (TargetEntityInSpace->ShieldEntity && bestEnt == TargetEntityInSpace->ShieldEntity) ) ) {
		return false; // we hit our target - so this is not a miss
	} else {
		return true; // this is a good miss
	}
}


/*
================
sbShip::SetInitialShipAttributes
================
*/
void sbShip::SetInitialShipAttributes() {
	//int medical_module_max_power = spawnArgs.GetInt("medical_module_max_power", "1");
	//int engines_module_max_power = spawnArgs.GetInt("engines_module_max_power", "1");
	//int weapons_module_max_power = spawnArgs.GetInt("weapons_module_max_power", "1");
	//int torpedos_module_max_power = spawnArgs.GetInt("torpedos_module_max_power", "1");
	//int shields_module_max_power = spawnArgs.GetInt("shields_module_max_power", "1");
	//int sensors_module_max_power = spawnArgs.GetInt("sensors_module_max_power", "1");
	//int environment_module_max_power = spawnArgs.GetInt("environment_module_max_power", "1");
	//int computer_module_max_power = spawnArgs.GetInt("computer_module_max_power", "1");
	//int security_module_max_power = spawnArgs.GetInt("security_module_max_power", "1");


	// GET THE MAX POWER SPAWNARGS:
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		module_max_powers[i] = spawnArgs.GetInt( module_description[i] + "_module_max_power", "1" );
	}

	// FIRST DO MODULE POWER AND HEALTH:
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule ) {
			consoles[i]->ControlledModule->max_power = module_max_powers[i];
			consoles[i]->ControlledModule->spawnArgs.SetInt( "max_power", module_max_powers[i] );

			consoles[i]->ControlledModule->entity_max_health = module_max_powers[i] * 64;
			consoles[i]->ControlledModule->spawnArgs.SetInt( "entity_max_health", module_max_powers[i] * 64 );
			consoles[i]->ControlledModule->health = module_max_powers[i] * 64;
			consoles[i]->ControlledModule->spawnArgs.SetInt( "health", module_max_powers[i] * 64 );

			consoles[i]->ControlledModule->RecalculateModuleEfficiency();
		}
	}



	// THEN DO THE UNIQUE THINGS FOR EACH MODULE:
	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule ) {
		consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle = 1.625 + ( 0.375 * module_max_powers[MEDICALMODULEID] );
		consoles[MEDICALMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle = 0.02 + ( 0.01 * module_max_powers[ENGINESMODULEID] );
		consoles[ENGINESMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle );

		// Increases in evasion are handled in the evasion functions
	}
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		weapons_damage_modifier = (module_max_powers[WEAPONSMODULEID] * 10) + 40;
		spawnArgs.SetInt("weapons_damage_modifier", weapons_damage_modifier);
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		torpedos_damage_modifier = (module_max_powers[TORPEDOSMODULEID] * 15) + 50;
		spawnArgs.SetInt("torpedos_damage_modifier", torpedos_damage_modifier);
	}
	if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
		consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle = 3.0f + ( module_max_powers[SHIELDSMODULEID] * (0.0f + (module_max_powers[SHIELDSMODULEID] * 0.2f) ) ); //(was originally = 3 + ( module_max_powers[SHIELDSMODULEID] * 0.5 )) // then was = 3 + ( module_max_powers[SHIELDSMODULEID] * 2 ) - but that was too much at the lower levels.
		consoles[SHIELDSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle );

		max_shieldStrength = module_max_powers[SHIELDSMODULEID] * 200; // with a min of 200 or 0 and a max of 1600
		max_shieldStrength = idMath::ClampInt(0,MAX_MAX_SHIELDSTRENGTH,max_shieldStrength);
		spawnArgs.SetInt("max_shieldStrength", max_shieldStrength);
		if ( shieldStrength > max_shieldStrength ) {
			shieldStrength = max_shieldStrength;
		}
		shieldStrength_copy = shieldStrength;
		if ( !shields_raised ) {
			shieldStrength = 0;
		}
	}
	if ( consoles[SENSORSMODULEID] && consoles[SENSORSMODULEID]->ControlledModule ) {
		consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle = 0.2 + ( module_max_powers[SENSORSMODULEID] * 0.1 );
		consoles[SENSORSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
		consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle = 20 + ( module_max_powers[ENVIRONMENTMODULEID] * 5 );
		consoles[ENVIRONMENTMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
		consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle = 0.6 + ( module_max_powers[COMPUTERMODULEID] * 0.2 );
		consoles[COMPUTERMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle );

		//TODO: if ( computer_buff_amount_modifier > 1.20f ) { // maybe we can just increase the upper limit
	}
	if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule ) {
		consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle = 3.6 + ( module_max_powers[SECURITYMODULEID] * 0.2 );
		consoles[SECURITYMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle );

		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() ) {
				shipdoors[ i ].GetEntity()->entity_max_health = (module_max_powers[SECURITYMODULEID] * 10) + 20;
				shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "entity_max_health", shipdoors[ i ].GetEntity()->entity_max_health );
				shipdoors[ i ].GetEntity()->health = (module_max_powers[SECURITYMODULEID] * 10) + 20;
				shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "health", shipdoors[ i ].GetEntity()->health );

				if ( shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) && shipdoors[ i ].GetEntity()->health > 0 ) {
					dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SyncUpDoorGroupsSpawnArgs();
				}
			}
		}
	}

	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->UpdateModulesPowerQueueOnCaptainGui();
		gameLocal.GetLocalPlayer()->UpdateCaptainMenuEveryFrame();
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
		gameLocal.GetLocalPlayer()->UpdateCaptainHudOnce();
		gameLocal.Printf( name + " is setting PlayerShip module attributes\n" );
	}
}

/*
================
sbShip::UpdateShipAttributes
================
*/
void sbShip::UpdateShipAttributes() {
	// GET THE MAX POWER SPAWNARGS:
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		module_max_powers[i] = spawnArgs.GetInt( module_description[i] + "_module_max_power", "1" );
	}

	// FIRST DO MODULE POWER AND HEALTH:
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule ) {
			consoles[i]->ControlledModule->max_power = module_max_powers[i];
			consoles[i]->ControlledModule->spawnArgs.SetInt( "max_power", module_max_powers[i] );

			if ( consoles[i]->ControlledModule->health == consoles[i]->ControlledModule->entity_max_health ) {
				consoles[i]->ControlledModule->entity_max_health = module_max_powers[i] * 64;
				consoles[i]->ControlledModule->spawnArgs.SetInt( "entity_max_health", module_max_powers[i] * 64 );
				consoles[i]->ControlledModule->health = module_max_powers[i] * 64;
				consoles[i]->ControlledModule->spawnArgs.SetInt( "health", module_max_powers[i] * 64 );
			} else {
				consoles[i]->ControlledModule->entity_max_health = module_max_powers[i] * 64;
				consoles[i]->ControlledModule->spawnArgs.SetInt( "entity_max_health", module_max_powers[i] * 64 );
				consoles[i]->ControlledModule->spawnArgs.SetInt( "health", module_max_powers[i] * 64 );
			}

			consoles[i]->ControlledModule->RecalculateModuleEfficiency();
		}
	}



	// THEN DO THE UNIQUE THINGS FOR EACH MODULE:
	if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule ) {
		consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle = 1.625 + ( 0.375 * module_max_powers[MEDICALMODULEID] );
		consoles[MEDICALMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle = 0.02 + ( 0.01 * module_max_powers[ENGINESMODULEID] );
		consoles[ENGINESMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle );

		// Increases in evasion are handled in the evasion functions
	}
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		weapons_damage_modifier = (module_max_powers[WEAPONSMODULEID] * 10) + 40;
		spawnArgs.SetInt("weapons_damage_modifier", weapons_damage_modifier);
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		torpedos_damage_modifier = (module_max_powers[TORPEDOSMODULEID] * 15) + 50;
		spawnArgs.SetInt("torpedos_damage_modifier", torpedos_damage_modifier);
	}
	if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
		consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle = 3.0f + ( module_max_powers[SHIELDSMODULEID] * (0.0f + (module_max_powers[SHIELDSMODULEID] * 0.2f) ) ); //(was originally = 3 + ( module_max_powers[SHIELDSMODULEID] * 0.5 )) // then was = 3 + ( module_max_powers[SHIELDSMODULEID] * 2 ) - but that was too much at the lower levels.
		consoles[SHIELDSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle );

		max_shieldStrength = module_max_powers[SHIELDSMODULEID] * 200; // with a min of 200 or 0 and a max of 1600
		max_shieldStrength = idMath::ClampInt(0,MAX_MAX_SHIELDSTRENGTH,max_shieldStrength);
		spawnArgs.SetInt("max_shieldStrength", max_shieldStrength);
		if ( shieldStrength > max_shieldStrength ) {
			shieldStrength = max_shieldStrength;
		}
		shieldStrength_copy = shieldStrength;
		if ( !shields_raised ) {
			shieldStrength = 0;
		}
	}
	if ( consoles[SENSORSMODULEID] && consoles[SENSORSMODULEID]->ControlledModule ) {
		consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle = 0.2 + ( module_max_powers[SENSORSMODULEID] * 0.1 );
		consoles[SENSORSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
		consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle = 20 + ( module_max_powers[ENVIRONMENTMODULEID] * 5 );
		consoles[ENVIRONMENTMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle );
	}
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
		consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle = 0.6 + ( module_max_powers[COMPUTERMODULEID] * 0.2 );
		consoles[COMPUTERMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle );

		//TODO: if ( computer_buff_amount_modifier > 1.20f ) { // maybe we can just increase the upper limit
	}
	if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule ) {
		consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle = 3.6 + ( module_max_powers[SECURITYMODULEID] * 0.2 );
		consoles[SECURITYMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle );

		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() ) {
				if ( shipdoors[ i ].GetEntity()->health == shipdoors[ i ].GetEntity()->entity_max_health ) {
					shipdoors[ i ].GetEntity()->entity_max_health = (module_max_powers[SECURITYMODULEID] * 10) + 20;
					shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "entity_max_health", shipdoors[ i ].GetEntity()->entity_max_health );
					shipdoors[ i ].GetEntity()->health = (module_max_powers[SECURITYMODULEID] * 10) + 20;
					shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "health", shipdoors[ i ].GetEntity()->health );
				} else {
					shipdoors[ i ].GetEntity()->entity_max_health = (module_max_powers[SECURITYMODULEID] * 10) + 20;
					shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "entity_max_health", shipdoors[ i ].GetEntity()->entity_max_health );
					shipdoors[ i ].GetEntity()->spawnArgs.SetInt( "health", shipdoors[ i ].GetEntity()->health );
				}

				if ( shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) && shipdoors[ i ].GetEntity()->health > 0 ) {
					dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SyncUpDoorGroupsSpawnArgs();
				}
			}
		}
	}

	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->UpdateModulesPowerQueueOnCaptainGui();
		gameLocal.GetLocalPlayer()->UpdateCaptainMenuEveryFrame();
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
		gameLocal.GetLocalPlayer()->UpdateCaptainHudOnce();
		gameLocal.Printf( name + " is setting PlayerShip module attributes\n" );
	}
}

/*
================
sbShip::UpdateShipAttributes
================
*/
void sbShip::PrintShipAttributes() {
	gameLocal.Printf( "\nHull: The structure of your ship. When this reaches zero your ship will explode.\n___________________________\n\nUpgrade Cost: 300 Materials, Repair Cost: 100 Materials\n");
	gameLocal.Printf( "\nPower Reserve: This is the total power that your ship can produce to power your ship modules.\n___________________________\n\nUpgrade Cost: 200 Materials Materials\n");
	gameLocal.Printf( "\nMaterials: Used to repair and upgrade your ship.");
	gameLocal.Printf( "\nCurrency: Interstellar currency used to trade with other ships, spacestations and planets.");
	gameLocal.Printf( "\nAuto-Destruct: If your ship has been boarded you can destroy your own ship to prevent it falling into enemy hands.");

	gameLocal.Printf( "\nDim Lights: Dims the lights on your ship.");
	gameLocal.Printf( "\nOn/Off Lights: Toggles ship lights on and off.");
	gameLocal.Printf( "\nRed Alert: Alerts crew that the ship is in danger and proper action should be taken. Crew will run instead of walking when red alert lights are on.");

	gameLocal.Printf( "\nMedical Module: Heals all friendly crew aboard the ship.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[MEDICALMODULEID] && consoles[MEDICALMODULEID]->ControlledModule ) {
			consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle = 1.625 + ( 0.375 * i );
			consoles[MEDICALMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Heal Speed x" + idStr(consoles[MEDICALMODULEID]->ControlledModule->charge_added_each_cycle / 2.0f) + "\n");
		}
	}
	gameLocal.Printf( "\nEngines: Charges the FTL drive and allows the ship to evade incoming attacks.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
			consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle = 0.02 + ( 0.01 * i );
			consoles[ENGINESMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "FTL Charge Speed x" + idStr(consoles[ENGINESMODULEID]->ControlledModule->charge_added_each_cycle / 0.12f) + " - ");
			// Increases in evasion are handled in the evasion functions

			float miss_chance = (1.0f * 0.10f) + ((1.0 * ((float)i * 0.03) ) * (1.0f));
			//miss_chance = idMath::ClampFloat( 0.0f, 0.70f, miss_chance );

			gameLocal.Printf( "Evasion: " + idStr(miss_chance) + "\n");
		}
	}
	gameLocal.Printf( "\nBeam Cannon: Beam based weapon that can partially pass through defense shields but does less damage overall.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
			weapons_damage_modifier = (i * 10) + 40;
			spawnArgs.SetInt("weapons_damage_modifier", weapons_damage_modifier);
			gameLocal.Printf( "Weapons Damage x" + idStr((float)weapons_damage_modifier / 50.0f) + " - Per Volley: " + idStr(weapons_damage_modifier) + "\n");
			//gameLocal.Printf( "Weapons Damage Per Volley: " + idStr(weapons_damage_modifier) + "\n");
		}
	}
	gameLocal.Printf( "\nTorpedos: Explosive weapon that cannot pass through defense shields but does more damage overall.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
			torpedos_damage_modifier = (i * 15) + 50;
			spawnArgs.SetInt("torpedos_damage_modifier", torpedos_damage_modifier);
			gameLocal.Printf( "Torpedos Damage x" + idStr((float)torpedos_damage_modifier / 65.0f) + " - Per Volley: " + idStr(torpedos_damage_modifier) + "\n");
			//gameLocal.Printf( "Torpedos Damage Per Volley: " + idStr(torpedos_damage_modifier) + "\n");
		}
	}
	gameLocal.Printf( "\nShields: Defense shields help protect the ship from damage.\n___________________________\nUpgrade Cost: 500 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
			consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle = 3.0f + ( i * (0.0f + (i * 0.2f) ) );
			consoles[SHIELDSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Shield Charge Speed x" + idStr(consoles[SHIELDSMODULEID]->ControlledModule->charge_added_each_cycle / 3.2f));

			max_shieldStrength = i * 200; // with a min of 200 or 0 and a max of 1600
			max_shieldStrength = idMath::ClampInt(0,MAX_MAX_SHIELDSTRENGTH,max_shieldStrength);
			spawnArgs.SetInt("max_shieldStrength", max_shieldStrength);
			if ( shieldStrength > max_shieldStrength ) {
				shieldStrength = max_shieldStrength;
			}
			shieldStrength_copy = shieldStrength;
			if ( !shields_raised ) {
				shieldStrength = 0;
			}
			gameLocal.Printf( " - Shield Capacity: " + idStr(max_shieldStrength) + "\n");
		}
	}
	gameLocal.Printf( "\nSensors: Allows detection and analysis of entities in space and aboard other vessels.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[SENSORSMODULEID] && consoles[SENSORSMODULEID]->ControlledModule ) {
			consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle = 0.2 + ( i * 0.1 );
			consoles[SENSORSMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Sensors Charge Speed x" + idStr(consoles[SENSORSMODULEID]->ControlledModule->charge_added_each_cycle / 0.30f) + "\n");
		}
	}
	gameLocal.Printf( "\nEnvironment: Maintains the atmosphere inside the vessel.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
			consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle = 20.0f + ( (float)i * 5.0f );
			consoles[ENVIRONMENTMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Oxygen Replenishment Speed x" + idStr(consoles[ENVIRONMENTMODULEID]->ControlledModule->charge_added_each_cycle / 25.0f) + "\n");
		}
	}
	gameLocal.Printf( "\nComputer: Calculates transporter pad destinations and allows crew to increase module efficiency.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule ) {
			consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle = 0.6 + ( i * 0.2 );
			consoles[COMPUTERMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Transporter Charge Speed x" + idStr(consoles[COMPUTERMODULEID]->ControlledModule->charge_added_each_cycle / 0.8f));
			//TODO: if ( computer_buff_amount_modifier > 1.20f ) { // maybe we can just increase the upper limit

			//float computer_buff_amount_power_cap = 0.56f + ( (float)i * 0.08f ); // with a min of 0.58 and a max of 1.2
			//gameLocal.Printf( " - Crew Console Effectiveness x" + idStr(computer_buff_amount_power_cap / 0.64f) + "\n" );
			gameLocal.Printf( " - Crew Console Effectiveness x" + idStr( 1.00f + ( (float)i * 0.05f )) + "\n" );
		}
	}
	gameLocal.Printf( "\nSecurity: Controls and maintains blast doors.\n___________________________\nUpgrade Cost: 100 Materials\n___________________________\n");
	for ( int i = 1; i < 9; i++ ) {
		if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule ) {
			consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle = 3.6 + ( i * 0.2 );
			consoles[SECURITYMODULEID]->ControlledModule->spawnArgs.SetFloat("charge_added_each_cycle", consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle );
			gameLocal.Printf( "Door Repair Speed x" + idStr(consoles[SECURITYMODULEID]->ControlledModule->charge_added_each_cycle / 3.8f));

			if ( shipdoors[ 0 ].GetEntity() ) {
				shipdoors[ 0 ].GetEntity()->entity_max_health = (i * 10) + 20;
				shipdoors[ 0 ].GetEntity()->spawnArgs.SetInt( "entity_max_health", shipdoors[ 0 ].GetEntity()->entity_max_health );
				shipdoors[ 0 ].GetEntity()->health = (i * 10) + 20;
				shipdoors[ 0 ].GetEntity()->spawnArgs.SetInt( "health", shipdoors[ 0 ].GetEntity()->health );

				if ( shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) && shipdoors[ i ].GetEntity()->health > 0 ) {
					dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SyncUpDoorGroupsSpawnArgs();
				}
			}
			if ( shipdoors[ 0 ].GetEntity() ) {
				gameLocal.Printf( " - Door Strength x" + idStr(((float)shipdoors[ 0 ].GetEntity()->entity_max_health / 30.0f)) + "\n" );
			}
		}
	}
}

	// boyette map entites link up begin
/*
===============
sbShip::DoStuffAfterAllMapEntitiesHaveSpawned
===============
*/
void sbShip::DoStuffAfterAllMapEntitiesHaveSpawned() {
	//if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
		// BOYETTE better method we use now doesn't require events //PostEventMS( &EV_DoStuffAfterSpawn, 100 ); // boyette note: this delays the Event_DoStuffAfterSpawn function a little bit (100 milliseconds) while we wait for all the map entities to spawn. We might want to verify that all entities have spawned with gameLocal.GameState() == GAMESTATE_ACTIVE.
		//return; // DONE: boyette NOTE TODO: We might want to verify that the game that all the entities have spawned (which should be true when the gameLocal.GameState() == GAMESTATE_ACTIVE.
		// the consoles will have referenced their controlled modules by now. if it makes it past this check because consoles do it right away.
		// if we set to the consoles->controlled module's parentship to the console parent ship - that should solve the the problem.
		// might need to do a gamelocal thing where it cycles through all modules first and set's their sub entities, then all consoles and their subentities, then all ships and their sub-entities.
		// then the parent entities can be set.
	//}
	idStr TestHideShip = spawnArgs.GetString("test_hide_ship",NULL);
	TestHideShipEntity = gameLocal.FindEntity( TestHideShip );

	//gameLocal.Printf( "Do Stuff After Spawn.\n" );


	declManager->FindMaterial( ShipStargridIcon )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
	declManager->FindMaterial( ShipStargridArtifactIcon )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
	/* // BOYETTE NOTE: I think we should to this too:
	declManager->FindMaterial( ShipImageVisual )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		declManager->FindMaterial( "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
		declManager->FindMaterial( "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
	}
	*/

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {

		declManager->FindMaterial( "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
		declManager->FindMaterial( "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.

		consoles[i] = dynamic_cast<sbConsole*>( gameLocal.FindEntity( spawnArgs.GetString(module_description[i] + "_console",NULL) ) );
		if ( consoles[i] ) {
			consoles[i]->ParentShip = this;

			// could link these with some spawn args on the module
			//consoles[i]->SetRenderEntityGui0String( "module_icon_left", "guis/assets/steve_captain_display/CircularModuleChargeBar/ModuleIconBackgrounds/" + module_description_upper[i] + "ModuleEfficiencyBackgroundLeft.tga" );
			//consoles[i]->SetRenderEntityGui0String( "module_icon_right", "guis/assets/steve_captain_display/CircularModuleChargeBar/ModuleIconBackgrounds/" + module_description_upper[i] + "ModuleEfficiencyBackgroundRight.tga" );
			consoles[i]->SetRenderEntityGui0String( "module_icon_background", "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" );
			consoles[i]->SetRenderEntityGui0String( "module_icon_background_circle_only", "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" );
			consoles[i]->SetRenderEntityGui0String( "ship_icon_background", ShipStargridIcon );
		}
	}

	/*
	if ( spawnArgs.GetString("medical_room_node",NULL) ) {
		MedicalRoomNode = gameLocal.FindEntity( spawnArgs.GetString("medical_room_node",NULL) );
	}
	if ( spawnArgs.GetString("engines_room_node",NULL) ) {
		EnginesRoomNode = gameLocal.FindEntity( spawnArgs.GetString("engines_room_node",NULL) );
	}
	if ( spawnArgs.GetString("weapons_room_node",NULL) ) {
		WeaponsRoomNode = gameLocal.FindEntity( spawnArgs.GetString("weapons_room_node",NULL) );
	}
	if ( spawnArgs.GetString("torpedos_room_node",NULL) ) {
		TorpedosRoomNode = gameLocal.FindEntity( spawnArgs.GetString("torpedos_room_node",NULL) );
	}
	if ( spawnArgs.GetString("shields_room_node",NULL) ) {
		ShieldsRoomNode = gameLocal.FindEntity( spawnArgs.GetString("shields_room_node",NULL) );
	}
	if ( spawnArgs.GetString("sensors_room_node",NULL) ) {
		SensorsRoomNode = gameLocal.FindEntity( spawnArgs.GetString("sensors_room_node",NULL) );
	}
	if ( spawnArgs.GetString("environment_room_node",NULL) ) {
		EnvironmentRoomNode = gameLocal.FindEntity( spawnArgs.GetString("environment_room_node",NULL) );
	}
	if ( spawnArgs.GetString("computer_room_node",NULL) ) {
		ComputerRoomNode = gameLocal.FindEntity( spawnArgs.GetString("computer_room_node",NULL) );
	}
	if ( spawnArgs.GetString("security_room_node",NULL) ) {
		SecurityRoomNode = gameLocal.FindEntity( spawnArgs.GetString("security_room_node",NULL) );
	}
	*/
	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		room_node[i] = gameLocal.FindEntity( spawnArgs.GetString( room_description[i] + "_room_node",NULL) ); // BOYETTE NOTE - this line was previously not safe because FindEntity did not return NULL even if name was NULL - I think we fixed this.
	}
	// Coaking
		// put Cloaking here

	// Power Core
		// put Power Core here

	// Transporter
	idStr TransporterBoundsName = spawnArgs.GetString("transporter_bounds",NULL);
	TransporterBounds = gameLocal.FindEntity( TransporterBoundsName );

	idStr TransporterPadName = spawnArgs.GetString("transporter_pad",NULL);
	if ( gameLocal.FindEntity( TransporterPadName ) && gameLocal.FindEntity( TransporterPadName )->IsType(sbTransporterPad::Type) ) {
		TransporterPad = dynamic_cast<sbTransporterPad*>(gameLocal.FindEntity( TransporterPadName ));
		TransporterPad->ParentShip = this;
	} else {
			//gameLocal.Warning( name + " has no transporter pad or it is not of type sbTransporterPad" );
			//gameLocal.Printf( "Note: " + name + " has no transporter pad or it is not of type sbTransporterPad" );
	}
	idStr TransporterParticleEntitySpawnMarkerName = spawnArgs.GetString("transporter_particle_marker",NULL);

	TransporterParticleEntitySpawnMarker = gameLocal.FindEntity( TransporterParticleEntitySpawnMarkerName );

	idStr ShipDiagramDisplayNodeName = spawnArgs.GetString("ship_diagram_display_node",NULL);
	ShipDiagramDisplayNode = gameLocal.FindEntity( ShipDiagramDisplayNodeName );

	idStr ShieldEntityName = spawnArgs.GetString("ship_shield_entity",NULL);
	ShieldEntity = gameLocal.FindEntity( ShieldEntityName );
	if ( ShieldEntity ) {
		ShieldEntity->SetShaderParm(10,0.0f); // The shield entity is invisible to begin.
		ShieldEntity->GetPhysics()->GetClipModel()->SetOwner( this ); // This is so the torpedo doesn't hit the inside of the shield.
	}
	if ( !consoles[SHIELDSMODULEID] ) {
		if ( ShieldEntity ) {
			ShieldEntity->Event_Remove();
			ShieldEntity = NULL;

			shields_raised = false;
			shieldStrength = 0;
			max_shieldStrength = 1; // to make sure we can transport throught he shield - the shield strength should always be less than the transport limit.
			shieldStrength_copy = 0;
			//gameLocal.Printf( "NOTE: " + name + " does not have a shields console. shield entity is removed. shieldstrength = 0. shields_raised = false." );
		}
	}

	if ( spawnArgs.MatchPrefix( "shiplight" ) ) { // if there is a shiplight entities get them.
		gameLocal.GetTargets(spawnArgs, shiplights, "shiplight" );
	}

	if ( spawnArgs.MatchPrefix( "shipdoor" ) ) { // if there is a shipdoor entities get them.
		gameLocal.GetTargets(spawnArgs, shipdoors, "shipdoor" );
	}

	idStr CaptainChairName = spawnArgs.GetString("captain_chair",NULL);
	if ( gameLocal.FindEntity( CaptainChairName ) && gameLocal.FindEntity( CaptainChairName )->IsType(sbCaptainChair::Type) ) {
		CaptainChair = dynamic_cast<sbCaptainChair*>(gameLocal.FindEntity( CaptainChairName ));
		CaptainChair->ParentShip = this;
	} else {
		//gameLocal.Printf( "Note: " + name + " has no captain chair or it is not of type sbCaptainChair" );
	}

	idStr ReadyRoomCaptainChairName = spawnArgs.GetString("ready_room_captain_chair",NULL);
	if ( gameLocal.FindEntity( ReadyRoomCaptainChairName ) && gameLocal.FindEntity( ReadyRoomCaptainChairName )->IsType(sbCaptainChair::Type) ) {
		ReadyRoomCaptainChair = dynamic_cast<sbCaptainChair*>(gameLocal.FindEntity( ReadyRoomCaptainChairName ));
		ReadyRoomCaptainChair->ParentShip = this;
	}

	idStr ViewScreenName = spawnArgs.GetString("ship_viewscreen_entity",NULL);
	if ( gameLocal.FindEntity( ViewScreenName ) ) {
		ViewScreenEntity = gameLocal.FindEntity( ViewScreenName );
	}

	UpdateGuisOnCaptainChair();

	MySkyPortalEnt = dynamic_cast<idPortalSky*>( gameLocal.FindEntity( spawnArgs.GetString( "my_sky_portal_entity" ) ) );
	alway_snap_to_my_sky_portal_entity = spawnArgs.GetBool("alway_snap_to_my_sky_portal_entity", "0");

	//new crew system setup // this allows us to iterate and avoids alot of copy-paste code
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		idStr crewmember_name;
		crewmember_name = spawnArgs.GetString(role_description[i] + "_officer",NULL);
		crew[i] = dynamic_cast<idAI*>( gameLocal.FindEntity( crewmember_name ) );
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->SyncUpPlayerShipNameCVars();
		}
		if ( crew[i] ) {
			AIsOnBoard.push_back(crew[i]); // add the ai to the list of AI's on board this ship
			crew[i]->ShipOnBoard = this;
			crew[i]->ParentShip = this;
		}
	}

	if ( spawnArgs.MatchPrefix( "misc_ai_on_board" ) ) { // if there are misc ai's on board get them.
		idList< idEntityPtr<idEntity> >	misc_ais_on_board;
		gameLocal.GetTargets(spawnArgs, misc_ais_on_board, "misc_ai_on_board" );
		for ( int i = 0; i < misc_ais_on_board.Num(); i++ ) {
			if ( misc_ais_on_board[ i ].GetEntity()->IsType( idAI::Type ) ) {
				AIsOnBoard.push_back(static_cast<idAI*>( misc_ais_on_board[ i ].GetEntity() ) );
				static_cast<idAI*>( misc_ais_on_board[ i ].GetEntity() )->ShipOnBoard = this;
			}
		}
	}

	priority_space_entity_to_target = gameLocal.FindEntity( spawnArgs.GetString("priority_space_entity_to_target",NULL) );
	priority_space_entity_to_protect = gameLocal.FindEntity( spawnArgs.GetString("priority_space_entity_to_protect",NULL) );

	prioritize_playership_as_space_entity_to_target = spawnArgs.GetBool("prioritize_playership_as_space_entity_to_target","0");
	prioritize_playership_as_space_entity_to_protect = spawnArgs.GetBool("prioritize_playership_as_space_entity_to_protect","0");

	if ( crew[CAPTAINCREWID] ) {
		//AIsOnBoard.push_back(crew[CAPTAINCREWID]); // add the ai to the list of AI's on board this ship
		//crew[CAPTAINCREWID]->ShipOnBoard = this;
		if ( CaptainChair ) {
			/////////////crew[CAPTAINCREWID]->SetOrigin( CaptainChair->GetPhysics()->GetOrigin() );
			//crew[CAPTAINCREWID]->Bind( CaptainChair, false ); // this is not necessary.
			//crew[CAPTAINCREWID]->GetPhysics()->SetAxis( CaptainChair->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1") );
			//crew[CAPTAINCREWID]->SetAxis( CaptainChair->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1") );
			//crew[CAPTAINCREWID]->TurnToward( GetPhysics()->GetOrigin() + ( spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().ToForward() * 40 ) );
			//crew[CAPTAINCREWID]->GetRenderEntity()->axis = CaptainChair->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1");
			////////////////////crew[CAPTAINCREWID]->SetDeltaViewAngles( CaptainChair->spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles() ); // best
			//Captain->Event_PlayCycle(ANIMCHANNEL_TORSO, "captain_chair_sitting" );
		}
	}
}

/*
===============
sbShip::DoStuffAfterPlayerHasSpawned
===============
*/
void sbShip::DoStuffAfterPlayerHasSpawned() {
	if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) {
		friendlinessWithPlayer = 0;
	} else {
		friendlinessWithPlayer = spawnArgs.GetInt("initial_friendlinessWithPlayer","10");
	}
	if ( ReadyRoomCaptainChair && ReadyRoomCaptainChair->has_console_display ) {
		ReadyRoomCaptainChair->PopulateCaptainLaptop();
	}
	if ( ViewScreenEntity ) {
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
			ViewScreenEntity->Show();
		} else {
			ViewScreenEntity->Hide();
		}
	}

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] ) {
			if ( consoles[i]->ControlledModule ) {
				// could link these with some spawn args on the module
				//consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_left", "guis/assets/steve_captain_display/CircularModuleChargeBar/ModuleIconBackgrounds/" + module_description_upper[i] + "ModuleEfficiencyBackgroundLeft.tga" );
				//consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_right", "guis/assets/steve_captain_display/CircularModuleChargeBar/ModuleIconBackgrounds/" + module_description_upper[i] + "ModuleEfficiencyBackgroundRight.tga" );
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_background", "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" );
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_background_circle_only", "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" );
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "ship_icon_background", ShipStargridIcon );
			}
		}
	}

	LeavingStargridPosition(stargridpositionx,stargridpositiony); // BOYETTE NOTE: this is very important here(need to fix function) - if we consecutively run the arriving function without the leaving function it will create duplicate pointers(pointers to the same address) in the ships_at_my_stargrid_position vector
	ArrivingAtStargridPosition(stargridpositionx,stargridpositiony);

	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
		gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
		gameLocal.GetLocalPlayer()->PopulateShipList();
	}

	// RESERVE CREW BEGIN
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		if ( spawnArgs.MatchPrefix( "reserve_crew_def" ) ) { // if there are reserve crew specified
			if ( max_reserve_crew > 0 ) {
				const idDict* reserve_crew = gameLocal.FindEntityDefDict( spawnArgs.GetString("reserve_crew_def",NULL), false );
				if ( reserve_crew ) {
					reserve_Crew.push_back(*reserve_crew);
				}
			}
			for ( int i = 0; i < max_reserve_crew; i++ ) {
				if ( reserve_Crew.size() >= max_reserve_crew ) {
					break;
				}
				const idDict* reserve_crew = gameLocal.FindEntityDefDict( spawnArgs.GetString(va("reserve_crew_def%i",i),NULL), false );
				if ( reserve_crew ) {
					reserve_Crew.push_back(*reserve_crew);
				}
			}
		}
		gameLocal.GetLocalPlayer()->UpdateReserveCrewMemberPortraits();
	}
	// RESERVE CREW END

	// READY ROOM CONSOLE BEGIN
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && ReadyRoomCaptainChair ) {
		ReadyRoomCaptainChair->ship_directive_overridden = true;
		ReadyRoomCaptainChair->PopulateCaptainLaptop();
	}

	ChangeTeam(team);
	// READY ROOM CONSOLE BEGIN
	// THIS IS FOR ARRIVING - WE NEED TO DO ANOTHER VERSION FOR LEAVING A STARGRID POSITION
	// BOYETTE NOTE TODO: check if we should be a dormant ship here
// BOYETTE GAMEPLAY BALANCING BEGIN
	SetInitialShipAttributes();
// BOYETTE GAMEPLAY BALANCING END
}

void sbShip::Event_TestScriptFunction( void ) {
	gameLocal.Printf( "TestScriptFunction\n" );
	return;
}

void sbShip::Event_SetMinimumModulePowers( void ) {
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
	}
	if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[SHIELDSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[SHIELDSMODULEID]->ControlledModule);
	}
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[ENGINESMODULEID]->ControlledModule);
	}
	if ( consoles[SENSORSMODULEID] && consoles[SENSORSMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[SENSORSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[SENSORSMODULEID]->ControlledModule);
	}
	if ( consoles[SECURITYMODULEID] && consoles[SECURITYMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[SECURITYMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[SECURITYMODULEID]->ControlledModule);
	}
	// just in case, we'll do this again:
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);
	}
	return;
}

void sbShip::Event_HandleBeginningShipDormancy( void ) {
	if ( ship_begin_dormant && !ship_is_never_dormant && gameLocal.GetLocalPlayer() && ( !gameLocal.GetLocalPlayer()->ShipOnBoard || (gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard != this)) && ( !gameLocal.GetLocalPlayer()->PlayerShip || (gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip != this)) ) {
		bool at_player_shiponboard_sg_pos = false;
		bool at_other_active_ship_sg_pos = false;
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && ( gameLocal.GetLocalPlayer()->ShipOnBoard == ships_at_my_stargrid_position[i] || gameLocal.GetLocalPlayer()->ShipOnBoard == this ) ) ) {
				at_player_shiponboard_sg_pos = true;
			}
			if ( ships_at_my_stargrid_position[i] && !ships_at_my_stargrid_position[i]->ship_begin_dormant && ships_at_my_stargrid_position[i]->ship_is_never_dormant ) {
				at_other_active_ship_sg_pos = true;
			}
		}
		if ( !at_player_shiponboard_sg_pos && !at_other_active_ship_sg_pos ) {
			Event_BecomeDormantShip();
		}
	}
	return;
}

void sbShip::Event_UpdateBeamVisibility( void ) {
	beam->Hide();
	beamTarget->Hide();
	ship_is_firing_weapons = false;
}

void sbShip::Event_UpdateShieldEntityVisibility( void ) {
	/* // this beam stuff is defintely not necesary anymore as the beam only appears for 350 ms while the shield is visible for 500. the beam disappearing before the shield gives it a nice immediate look.
	if ( beam ) {
		beam->Hide(); // I don't think this is neccessary here - but it gives it a more immediate visual look.
	}
	if ( beamTarget ) {
		beamTarget->Hide(); // I don't think this is neccessary here - but it gives it a more immediate visual look.
	}
	ship_is_firing = false;
	*/
	if( ShieldEntity ) {
		ShieldEntity->SetShaderParm(10,0.0f);
		if ( shieldStrength <= 0 && !ShieldEntity->IsHidden() ) {
			ShieldEntity->Hide(); // if there is no shield strength left, make sure the shield is hidden.
			if ( ShieldEntity->GetPhysics() ) {
				ShieldEntity->GetPhysics()->SetContents( 0 ); // set non-solid
			}
		}
	}
}

void sbShip::Event_CheckTorpedoStatus( void ) {
	if ( projectile.GetEntity() && !projectile.GetEntity()->IsAtRest() ) { // this will check if the torpedo has hit the ship. We should put a check in here to make sure that it has hit the targetship. That way switching targets at the last minute won't lose you the damage dealt.
		PostEventMS( &EV_CheckTorpedoStatus, 50 );
	} else {
		if ( TargetEntityInSpace && !torpedo_shot_missed ) {
			//gameLocal.Printf( TargetEntityInSpace->name );
			// damage the target entity shield, then hull, then target module.
			DealShipToShipDamageWithTorpedos();
			//ship_is_firing_torpedo = false;
		}
		ship_is_firing_torpedo = false;
	}
}

void sbShip::Event_SetTargetEntityInSpace( void ) {
	SetTargetEntityInSpace( TempTargetEntityInSpace );
}

void sbShip::Event_EngageWarp( void ) {
	int PlayerPosX;
	int PlayerPosY;

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
		PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
	} else { // boyette todo noto - space command note - the player can be either on an sbShip or an sbStationSpaceEntity - and maybe one day fighters or shuttles. So we will have a few variables called ShipOnBoard, PlanetOnBoard, ShuttleOnBoard etc.
		PlayerPosX = 0;
		PlayerPosY = 0;
	}

	EngageWarp(verified_warp_stargrid_postion_x,verified_warp_stargrid_postion_y);

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
		// reset the player specific ship on board warp effects. No need to reset anything as of right now if it is not the playership.
		//gameLocal.Printf( "The number of bloom passes got up to" + idStr( g_testBloomNumPasses.GetInteger() ) + "\n" );
		gameLocal.Printf( "Milliseconds at end:" + idStr( gameLocal.time + 700 ) + "\n" );

		gameLocal.GetLocalPlayer()->ScheduleStopWarpEffects(700); //PERFECT//
		gameLocal.GetLocalPlayer()->ScheduleStopSlowMo(700); //PERFECT//
		gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),700); //PERFECT//
		gameLocal.GetLocalPlayer()->TransitionNumBloomPassesToZero();

		//PERFECT SHORTER//gameLocal.GetLocalPlayer()->ScheduleStopWarpEffects(100);  //PERFECT SHORTER//
		//PERFECT SHORTER//gameLocal.GetLocalPlayer()->ScheduleStopSlowMo(100);  //PERFECT SHORTER//
		//PERFECT SHORTER//gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),100);  //PERFECT SHORTER//

		//PERFECT IMMEDIATE//gameLocal.GetLocalPlayer()->playerView.FreeWarp(0); //PERFECT IMMEDIATE//
		//PERFECT IMMEDIATE//g_enableSlowmo.SetBool( false ); //PERFECT IMMEDIATE//
		//PERFECT IMMEDIATE//gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),20); //PERFECT IMMEDIATE//

		//gameLocal.GetLocalPlayer()->weapon.GetEntity()->DetermineTimeGroup( false );
		//gameLocal.GetLocalPlayer()->DetermineTimeGroup( false );
		//gameLocal.GetLocalPlayer()->PlayerShip->DetermineTimeGroup(false);
		//g_enableSlowmo.SetBool(false);

		bool already_visited_this_stargrid_position = false;
		for ( int i = 0; i < gameLocal.GetLocalPlayer()->stargrid_positions_visited.size(); i++ ) {
			// first check to see if we already visited this stargrid position
			if (stargridpositionx == gameLocal.GetLocalPlayer()->stargrid_positions_visited[i].x && stargridpositiony == gameLocal.GetLocalPlayer()->stargrid_positions_visited[i].y ) {
				already_visited_this_stargrid_position = true;
				break;
			}
		}
		if ( !already_visited_this_stargrid_position ) {
			gameLocal.GetLocalPlayer()->stargrid_positions_visited.push_back( idVec2(stargridpositionx,stargridpositiony) );
			// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
			if ( gameLocal.GetLocalPlayer()->stargrid_positions_visited.size() >= ( ( MAX_STARGRID_X_POSITIONS * MAX_STARGRID_Y_POSITIONS ) - 1 ) ) {
				if ( common->m_pStatsAndAchievements ) {
					if ( !common->m_pStatsAndAchievements->m_nTimesAllStarGridPositionsVisited ) {
						common->m_pStatsAndAchievements->m_nTimesAllStarGridPositionsVisited++;
						common->StoreSteamStats();
					}
				}
			}
#endif
			// BOYETTE STEAM INTEGRATION END
		}
	}
	if ( stargridpositionx == PlayerPosX && stargridpositiony == PlayerPosY ) {
		// BOYETTE NOTE: was this before 08 12 2016 // idEntityFx::StartFx( spawnArgs.GetString("spaceship_warp_fx", "fx/spaceship_warp_fx_default"), &(GetPhysics()->GetOrigin()), 0, this, true );
	}
	//gameLocal.QuickSlowmoReset();
	//gameLocal.ResetSlowTimeVars();

	if ( gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->PostWarpThingsToDoForThePlayer();
	}
}

void sbShip::Event_InitiateTransporter( void ) {
	InitiateTransporter();
}
void sbShip::Event_InitiateRetrievalTransport( void ) {
	InitiateRetrievalTransport();
}

void sbShip::Event_InitiateOffPadRetrievalTransport( idEntity* entity_to_transport ) {
	if ( entity_to_transport && entity_to_transport->IsType( idAI::Type ) ) {
		idVec3 destination_point;
		idAI* ai_to_transport = dynamic_cast<idAI*>(entity_to_transport);

		if ( ai_to_transport->ShipOnBoard && ai_to_transport->ShipOnBoard != this ) {
			for ( int i = 0; i < ai_to_transport->ShipOnBoard->AIsOnBoard.size() ; i++ ) { // make sure nobody tries to keep moving to them
				if ( ai_to_transport->ShipOnBoard->AIsOnBoard[i] && ai_to_transport->ShipOnBoard->AIsOnBoard[i]->EntityCommandedToMoveTo == ai_to_transport ) {
					ai_to_transport->ShipOnBoard->AIsOnBoard[i]->EntityCommandedToMoveTo == NULL;
				}
			}								
			ai_to_transport->ShipOnBoard->AIsOnBoard.erase(std::remove(ai_to_transport->ShipOnBoard->AIsOnBoard.begin(), ai_to_transport->ShipOnBoard->AIsOnBoard.end(), ai_to_transport), ai_to_transport->ShipOnBoard->AIsOnBoard.end()); // remove the ai from the list of AI's on board this ship
		}

		AIsOnBoard.push_back(ai_to_transport); // add the ai to the list of AI's on board this ship
		ai_to_transport->ShipOnBoard = this;

		if ( ai_to_transport->ParentShip && ai_to_transport->ParentShip != this ) {
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( ai_to_transport->ParentShip->crew[i] && ai_to_transport == ai_to_transport->ParentShip->crew[i] ) {
					ai_to_transport->ParentShip->crew[i] = NULL;
				}
			}
		}
		ai_to_transport->ParentShip = this;

		if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
			destination_point = TransporterBounds->GetPhysics()->GetAbsBounds().GetCenter();
			destination_point.z = TransporterBounds->GetPhysics()->GetAbsBounds()[0][2];
		}

		ai_to_transport->SetOrigin( destination_point );

		if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
			// KEEP TRYING RANDOM ORIGINS WITH THE TRANSPORTER BOUNDS UNTIL WE ARE NOT WITHIN THE BOUNDS OF ANOTHER AI
			gameLocal.GetSuitableTransporterPositionWithinBounds(ai_to_transport,&TransporterBounds->GetPhysics()->GetAbsBounds());
		}
		//gameLocal.Printf( ent->name + " has a gravity of: " + idStr(ent->GetPhysics()->GetGravity().x) + "," + idStr(ent->GetPhysics()->GetGravity().y) + "," + idStr(ent->GetPhysics()->GetGravity().z)  );
		ai_to_transport->TemporarilySetThisEntityGravityToZero(200); // BOYETTE NOTE TODO IMPORTANT: MAYBE TODO: we could make the 200 a factor a the z distance the entity is being transported. BOYETTE NOTE: this is so they go immediately to the z value and are affected by gravity while transporting.
					
		ai_to_transport->EndTransporterMaterialShaderEffect();
					
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
		gameLocal.GetLocalPlayer()->UpdateSpaceCommandTabletGUIOnce();
					
		HandleTransporterEventsOnPlayerGuis();
	}
}
void sbShip::Event_InitiateOffPadRetrievalTransportToReserveCrew( idEntity* entity_to_transport ) {
	if ( reserve_Crew.size() >= max_reserve_crew ) {
		return;
	}

	if ( entity_to_transport && entity_to_transport->IsType( idAI::Type ) ) {
		idVec3 destination_point;
		idAI* ai_to_transport = dynamic_cast<idAI*>(entity_to_transport);

		/*
		if ( ai_to_transport->ShipOnBoard && ai_to_transport->ShipOnBoard != this ) {
			for ( int i = 0; i < ai_to_transport->ShipOnBoard->AIsOnBoard.size() ; i++ ) { // make sure nobody tries to keep moving to them
				if ( ai_to_transport->ShipOnBoard->AIsOnBoard[i] && ai_to_transport->ShipOnBoard->AIsOnBoard[i]->EntityCommandedToMoveTo == ai_to_transport ) {
					ai_to_transport->ShipOnBoard->AIsOnBoard[i]->EntityCommandedToMoveTo == NULL;
				}
			}								
			ai_to_transport->ShipOnBoard->AIsOnBoard.erase(std::remove(ai_to_transport->ShipOnBoard->AIsOnBoard.begin(), ai_to_transport->ShipOnBoard->AIsOnBoard.end(), ai_to_transport), ai_to_transport->ShipOnBoard->AIsOnBoard.end()); // remove the ai from the list of AI's on board this ship
		}

		if ( ai_to_transport->ParentShip && ai_to_transport->ParentShip != this ) {
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( ai_to_transport->ParentShip->crew[i] && ai_to_transport == ai_to_transport->ParentShip->crew[i] ) {
					ai_to_transport->ParentShip->crew[i] = NULL;
				}
			}
		}
		*/

		if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
			destination_point = TransporterBounds->GetPhysics()->GetAbsBounds().GetCenter();
			destination_point.z = TransporterBounds->GetPhysics()->GetAbsBounds()[0][2];
		}

		ai_to_transport->SetOrigin( destination_point );

		//if ( TransporterBounds && TransporterBounds->GetPhysics() ) {
		//	// KEEP TRYING RANDOM ORIGINS WITH THE TRANSPORTER BOUNDS UNTIL WE ARE NOT WITHIN THE BOUNDS OF ANOTHER AI
		//	gameLocal.GetSuitableTransporterPositionWithinBounds(ai_to_transport,&TransporterBounds->GetPhysics()->GetAbsBounds());
		//}

		ai_to_transport->TemporarilySetThisEntityGravityToZero(200); // BOYETTE NOTE TODO IMPORTANT: MAYBE TODO: we could make the 200 a factor a the z distance the entity is being transported. BOYETTE NOTE: this is so they go immediately to the z value and are affected by gravity while transporting.
					
		ai_to_transport->EndTransporterMaterialShaderEffect();
					
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
		gameLocal.GetLocalPlayer()->UpdateSpaceCommandTabletGUIOnce();
					
		HandleTransporterEventsOnPlayerGuis();


		ai_to_transport->spawnArgs.SetVector( "origin", ai_to_transport->GetPhysics()->GetOrigin() );

		const idDict* reserve_crew = &ai_to_transport->spawnArgs;
		if ( reserve_crew ) {
			reserve_Crew.push_back(*reserve_crew);
		}

		ai_to_transport->Event_Remove();
		ai_to_transport = NULL;

		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->UpdateReserveCrewMemberPortraits();
		}
	}
}

void sbShip::UpdatePlayerShipQuickWarpStatus() {
	// BOYETTE QUICK WARP BEGIN - added 06 03 2016
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this /*&& gameLocal.GetLocalPlayer()->PlayerShip == gameLocal.GetLocalPlayer()->ShipOnBoard*/ ) {
		
		bool hostile_entities_present = false;
		bool derelict_entities_present = false;
		bool phenomenon_action_entities_present = false;
		bool phenomenon_action_shield_affecting_entities_present = false;
		bool at_least_one_hostile_entity_has_a_module = false;
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i]->team != gameLocal.GetLocalPlayer()->team && !ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) ) {
				for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
					if ( ships_at_my_stargrid_position[i]->consoles[x] && ships_at_my_stargrid_position[i]->consoles[x]->ControlledModule  ) {
						at_least_one_hostile_entity_has_a_module = true;
						break;
					}
				}
				if ( at_least_one_hostile_entity_has_a_module ) {
					hostile_entities_present = true;
				}
				break;
			}
		}
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i]->team != gameLocal.GetLocalPlayer()->team && ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && ships_at_my_stargrid_position[i]->is_derelict && !ships_at_my_stargrid_position[i]->never_derelict ) {
				derelict_entities_present = true;
				break;
			}
		}
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i]->phenomenon_should_damage_modules && ships_at_my_stargrid_position[i]->phenomenon_should_damage_random_module ) {
				phenomenon_action_entities_present = true;
				break;
			}
		}
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i]->phenomenon_should_do_ship_damage || ships_at_my_stargrid_position[i]->phenomenon_should_set_ship_shields_to_zero ) {
				phenomenon_action_shield_affecting_entities_present = true;
				break;
			}
		}
		
		if ( ships_at_my_stargrid_position.size() == 0 || ( !hostile_entities_present /*&& !derelict_entities_present*/ && !phenomenon_action_entities_present ) ) { // BOYETTE NOTE: so if there are only friendly entities or no entities the warp will charge quickly
			if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount < 100.0f ) { // this was originally 99.0 - but I don't see any reason why it can't be 100.0
				consoles[ENGINESMODULEID]->ControlledModule->health = consoles[ENGINESMODULEID]->ControlledModule->entity_max_health;
				consoles[ENGINESMODULEID]->ControlledModule->module_was_just_repaired = true;
				consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount = 100.0f; // this was originally 99.0 - but I don't see any reason why it can't be 100.0
				consoles[ENGINESMODULEID]->ControlledModule->current_charge_percentage = 1.00f; // this was originally 99.0 - but I don't see any reason why it can't be 100.0
				consoles[ENGINESMODULEID]->ControlledModule->UpdateChargeAmount();
			}
			if ( consoles[SHIELDSMODULEID] && consoles[SHIELDSMODULEID]->ControlledModule ) {
				shieldStrength = max_shieldStrength;
				shieldStrength_copy = max_shieldStrength;
				consoles[SHIELDSMODULEID]->ControlledModule->health = consoles[SHIELDSMODULEID]->ControlledModule->entity_max_health;
				consoles[SHIELDSMODULEID]->ControlledModule->module_was_just_repaired = true;
				consoles[SHIELDSMODULEID]->ControlledModule->current_charge_amount = 100.0f;
				consoles[SHIELDSMODULEID]->ControlledModule->current_charge_percentage = 1.00f;
				consoles[SHIELDSMODULEID]->ControlledModule->UpdateChargeAmount();
			}
		}
	}
	// BOYETTE QUICK WARP END
}

void sbShip::Event_DisplayStoryWindow( void ) {
	// BOYETTE QUICK WARP BEGIN - added 06 03 2016
	UpdatePlayerShipQuickWarpStatus();

	if ( gameLocal.GetLocalPlayer() && ((gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this) || (gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this)) ) {
		gameLocal.GetLocalPlayer()->allow_overlay_captain_gui = true;
	}
	// BOYETTE QUICK WARP END
	gameLocal.MaybeDisplayStarGridStoryWindow();
}

void sbShip::Event_StartSynchdRedAlertFX( void ) {

	// if player shiponboard

	for( int i = 0; i < shiplights.Num(); i++ ) {
		shiplights[ i ].GetEntity()->SetShaderParm(7,1); // Enable the red alert material stage on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(8,0); // Get rid of all light dimming on the light fixture model
		shiplights[ i ].GetEntity()->SetShaderParm(9,8); // Enable pulsing flare effect on the light fixture model

		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(7,1); // Enable the red alert material stage on the light material
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->SetLightParm(8,0); // Get rid of all light dimming on the light fixture material
			}
		}
	}

	gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false);
	if ( spawnArgs.GetBool( "play_ship_alarm_sounds", "1" ) ) {
		gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_red_alert", "red_alert_snd_default") ), SND_CHANNEL_SHIP_ALARMS, 0, false, NULL ); // we can make a spawnarg here so the sound can be changed. We are using two seconds in length to match up with the light material.
	}
	// BOYETTE MUSIC BEGIN
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->music_shader_is_playing ) {
		gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f);
	}
	// BOYETTE MUSIC END
}

/*
===============
sbShip::Event_EvaluateShipRepairModeCycle
===============
*/
void sbShip::Event_EvaluateShipRepairModeCycle( void ) {
	bool reason_out_of_materials = false;
	bool reason_ship_is_fully_repaired = false;
	if ( in_repair_mode ) {
		if ( current_materials_reserves > 0 ) {
			if ( max_hullStrength - hullStrength > MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE ) {
				if ( current_materials_reserves > MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE ) {
					current_materials_reserves = current_materials_reserves - MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE;
					hullStrength = hullStrength + MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE;
				} else {
					hullStrength = hullStrength + current_materials_reserves;
					current_materials_reserves = 0;
					in_repair_mode = false;
					reason_out_of_materials = true;
				}
			} else if ( max_hullStrength - hullStrength <= MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE ) {
				if ( current_materials_reserves > max_hullStrength - hullStrength ) {
					current_materials_reserves = current_materials_reserves - ( max_hullStrength - hullStrength );
					hullStrength = hullStrength + (max_hullStrength - hullStrength);
					in_repair_mode = false;
					reason_ship_is_fully_repaired = true;
				} else {
					hullStrength = hullStrength + current_materials_reserves;
					current_materials_reserves = 0;
					in_repair_mode = false;
					reason_out_of_materials = true;
				}
			} else if ( hullStrength >= max_hullStrength ) {
				in_repair_mode = false;
				reason_ship_is_fully_repaired = true;
			}
		} else {
			in_repair_mode = false;
			reason_out_of_materials = true;
		}


		if ( in_repair_mode ) {
			PostEventMS( &EV_EvaluateShipRepairModeCycle, TIME_BETWEEN_SHIP_REPAIR_CYCLES );
		} else if ( hullStrength >= max_hullStrength || ( reason_ship_is_fully_repaired && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^8YOUR SHIP'S HULL IS FULLY ^8REPAIRED" );
		} else if ( reason_out_of_materials && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1YOU ARE OUT OF MATERIALS ^1FOR SHIP REPAIR" );
		}
		SetShaderParm( 10, 1.0f -( (float)hullStrength / (float)max_hullStrength ) ); // set the damage decal alpha
	}
}


/*
===============
sbShip::Event_UpdateViewscreenCamera
===============
*/
void sbShip::Event_UpdateViewscreenCamera( void ) {
	gameLocal.UpdateSpaceCommandViewscreenCamera();
}

/*
==================
sbShip::ScheduleUpdateViewscreenCamera
==================
*/
void sbShip::ScheduleUpdateViewscreenCamera( int ms_from_now ) {
	PostEventMS( &EV_UpdateViewscreenCamera, ms_from_now );
}

// SHIP AI EVENTS BEGIN
/*
================
idEntity::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves. // BOYETTE NOTE IMPORTANT: we overwrote this function to check if the ship should have a AI based on the spawnargs
================
*/
bool sbShip::ShouldConstructScriptObjectAtSpawn( void ) const {
	return 	spawnArgs.GetBool("ship_has_ai","1");
}

void sbShip::Event_IsPlayerShip( void ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsAtPlayerSGPosition( void ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsAtPlayerShipSGPosition( void ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_SpaceEntityIsAtMySGPosition( idEntity* ent ) {
	if ( ent && ent->stargridpositionx == stargridpositionx && ent->stargridpositiony == stargridpositiony ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsDerelict( void ) {
	if ( is_derelict ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsDerelictShip( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip* ship = static_cast<sbShip*>(ship_to_check);
		if ( ship->is_derelict ) {
			idThread::ReturnInt( true );
		} else {
			idThread::ReturnInt( false );
		}
	} else {
		idThread::ReturnInt( true ); // this is true because if the pointer is NULL it is better to assume it is derelict
	}
}
void sbShip::Event_IsFriendlyShip( idEntity* ent ) {
	if( ent && ent->team == team ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsHostileShip( idEntity* ent ) {
	if( ent && ent->team != team && ent->IsType(sbShip::Type) && static_cast<sbShip*>(ent)->is_derelict == false && !HasNeutralityWithTeam(ent->team) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
bool sbShip::IsHostileShip( idEntity* ent ) {
	if( ent && ent->team != team && ent->IsType(sbShip::Type) && static_cast<sbShip*>(ent)->is_derelict == false && !HasNeutralityWithTeam(ent->team) ) {
		return true;
	} else {
		return false;
	}
}

void sbShip::Event_SetMainGoal( int verb, idEntity* noun ) {
	mini_goals.clear();
	Event_AddMiniGoal(verb,noun);
	main_goal.goal_action = verb;
	main_goal.goal_entity = noun;
}
void sbShip::Event_AddMiniGoal( int verb, idEntity* noun ) {
	for ( int i = 0; i < mini_goals.size(); i++ ) {
		if ( mini_goals[i].goal_action == verb && mini_goals[i].goal_entity == noun ) {
			return; // the goal already exists in the goal queue
		}
	}
	mini_goals.push_back( ai_ship_goal(verb,noun) );
}
void sbShip::Event_RemoveMiniGoal( int verb, idEntity* noun ) {
	if ( verb == main_goal.goal_action && noun == main_goal.goal_entity ) {
		Event_SetMainGoal(SHIP_AI_IDLE,NULL); // if the main goal was completed we will just become idle (until we find a new main goal). // otherwise we will just remove the mini goal
		TargetEntityInSpace = NULL; // BOYETTE NOTE TODO IMPORTANT: not sure about this - might want to do this manually in the script - but I think this is the right choice
		if ( red_alert ) {
			CancelRedAlert();
		}
	} else {
		for ( int i = 0; i < mini_goals.size(); i++ ) {
			if ( mini_goals[i].goal_action == verb && mini_goals[i].goal_entity == noun ) {
				mini_goals.erase(mini_goals.begin() + i);
				return;
			}
		}
	}
}
void sbShip::Event_RemoveMiniGoalAction( int verb ) {
	if ( verb == main_goal.goal_action ) {
		Event_SetMainGoal(SHIP_AI_IDLE,NULL); // if the main goal was completed we will just become idle (until we find a new main goal). // otherwise we will just remove the mini goal
		TargetEntityInSpace = NULL; // BOYETTE NOTE TODO IMPORTANT: not sure about this - might want to do this manually in the script - but I think this is the right choice
		if ( red_alert ) {
			CancelRedAlert();
		}
	} else {
		for ( int i = 0; i < mini_goals.size(); i++ ) {
			if ( mini_goals[i].goal_action == verb ) {
				mini_goals.erase(mini_goals.begin() + i);
				i--;
			}
		}
	}
}
void sbShip::Event_ClearMiniGoals( void ) {
	mini_goals.clear();
}
void sbShip::Event_NextMiniGoal( void ) {
	if ( current_goal_iterator >= mini_goals.size() - 1 ) {
		current_goal_iterator = 0;
	} else {
		current_goal_iterator++;
	}
}
void sbShip::Event_PreviousMiniGoal( void ) {
	if ( current_goal_iterator <= 0 ) {
		current_goal_iterator = mini_goals.size() - 1;
	} else {
		current_goal_iterator--;
	}
}
void sbShip::Event_PrioritizeMiniGoal( int verb, idEntity* noun ) {
	for ( int i = 0; i < mini_goals.size(); i++ ) {
		if ( mini_goals[i].goal_action == verb && mini_goals[i].goal_entity == noun ) {
			current_goal_iterator = i;
			return; // we've got the best one
		}
	}
}

void sbShip::Event_ReturnCurrentGoalAction( void ) {
	if ( current_goal_iterator > mini_goals.size() - 1 ) {
		current_goal_iterator = mini_goals.size() - 1;
	}
	if ( mini_goals.size() > 0 ) {
		idThread::ReturnInt( mini_goals[current_goal_iterator].goal_action );
	} else {
		idThread::ReturnInt( 0 );
	}
}
void sbShip::Event_ReturnCurrentGoalEntity( void ) {
	//gameLocal.Printf( "THE MINI GOAL SIZE OF THE " + name + " IS: " + idStr(mini_goals.size()) + "\n." );
	if ( current_goal_iterator > mini_goals.size() - 1 ) {
		current_goal_iterator = mini_goals.size() - 1;
	}

	if ( mini_goals.size() > 0 && mini_goals[current_goal_iterator].goal_entity ) {
		idThread::ReturnEntity( mini_goals[current_goal_iterator].goal_entity );
		//gameLocal.Printf( "THE GOAL ENTITY IS: " + mini_goals[current_goal_iterator].goal_entity->name );
	} else {
		idThread::ReturnEntity( NULL );
	}
}
void sbShip::Event_ReturnHostileTargetingMyGoalEntity( void ) { // BOYETTE NOTE IMPORTANT: ADDED LATER: if there is a hostile entity at the stargrid position but it is not targeting the protected ship - this will ensure that we attack it.
	if ( current_goal_iterator > mini_goals.size() - 1 ) {
		current_goal_iterator = mini_goals.size() - 1;
	}
	if ( mini_goals.size() > 0 && mini_goals[current_goal_iterator].goal_entity ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->TargetEntityInSpace == mini_goals[current_goal_iterator].goal_entity
				&& ships_at_my_stargrid_position[i]->IsHostileShip(mini_goals[current_goal_iterator].goal_entity) && ships_at_my_stargrid_position[i]->IsType( idEntity::Type)) {

				idThread::ReturnEntity( static_cast<idEntity*>(ships_at_my_stargrid_position[i]) );
				return;
			}
		}
		// BOYETTE ADDED LATER BEGIN:
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->IsHostileShip(this) && ships_at_my_stargrid_position[i]->IsType( idEntity::Type)) {

				idThread::ReturnEntity( static_cast<idEntity*>(ships_at_my_stargrid_position[i]) );
				return;
			}
		}
		// BOYETTE ADDED LATER END:
		idThread::ReturnEntity( NULL );
	} else {
		idThread::ReturnEntity( NULL );
	}
}
void sbShip::Event_SetTargetShipInSpace( idEntity* target ) {
	if ( target && target->IsType( sbShip::Type ) ) {
		if ( TargetEntityInSpace != target ) {
			if ( target->stargridpositionx == stargridpositionx && target->stargridpositiony == stargridpositiony ) {
				SetTargetEntityInSpace( static_cast<sbShip*>(target) );
			}
		}
	}
}
void sbShip::Event_ClearTargetShipInSpace() {
	TempTargetEntityInSpace = NULL; // just for good measure - probably not necessary to set this to NULL
	TargetEntityInSpace = NULL;
}
void sbShip::Event_ReturnTargetShipInSpace( void ) {
	if ( TargetEntityInSpace && TargetEntityInSpace->IsType(idEntity::Type) ) {
		idThread::ReturnEntity( static_cast<idEntity*>(TargetEntityInSpace) );
	} else {
		idThread::ReturnEntity( NULL );
	}
}
void sbShip::Event_GetATargetShipInSpace( void ) { // BOYETTE NOTE TODO: we might want to rename this function AssessThreatsAndSetTargetShipInSpace();
	sbShip* bestTarget = NULL;
	if ( prioritize_playership_as_space_entity_to_target && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
		bestTarget = gameLocal.GetLocalPlayer()->PlayerShip;
		priority_space_entity_to_target = gameLocal.GetLocalPlayer()->PlayerShip;
	} else {
		if ( priority_space_entity_to_target && priority_space_entity_to_target->IsType(sbShip::Type) ) {
			bestTarget = dynamic_cast<sbShip*>(priority_space_entity_to_target);
		}
	}
	if ( bestTarget && bestTarget->stargridpositionx == stargridpositionx && bestTarget->stargridpositiony == stargridpositiony && bestTarget->team != team && !HasNeutralityWithShip(bestTarget) && !bestTarget->ship_destruction_sequence_initiated ) {
		Event_SetTargetShipInSpace(bestTarget);
		return;		
	} else {
		bestTarget = NULL;
	}

	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !ships_at_my_stargrid_position[i]->is_derelict && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && !ships_at_my_stargrid_position[i]->ship_destruction_sequence_initiated ) {
			if ( ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && ships_at_my_stargrid_position[i]->TargetEntityInSpace == this && ( (ships_at_my_stargrid_position[i]->consoles[WEAPONSMODULEID] && ships_at_my_stargrid_position[i]->consoles[WEAPONSMODULEID]->ControlledModule) || (ships_at_my_stargrid_position[i]->consoles[TORPEDOSMODULEID] && ships_at_my_stargrid_position[i]->consoles[TORPEDOSMODULEID]->ControlledModule) ) ) { // prioritize ships that are targeting us.
				bestTarget = ships_at_my_stargrid_position[i];
				break;
			}
		}
	}
	if ( !bestTarget ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !ships_at_my_stargrid_position[i]->is_derelict && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && !ships_at_my_stargrid_position[i]->ship_destruction_sequence_initiated ) {
				if ( ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && ( (ships_at_my_stargrid_position[i]->consoles[WEAPONSMODULEID] && ships_at_my_stargrid_position[i]->consoles[WEAPONSMODULEID]->ControlledModule) || (ships_at_my_stargrid_position[i]->consoles[TORPEDOSMODULEID] && ships_at_my_stargrid_position[i]->consoles[TORPEDOSMODULEID]->ControlledModule) ) ) { // prioritize ships that are targeting us.
					bestTarget = ships_at_my_stargrid_position[i];
					break;
				}
			}
		}
	}
	if ( !bestTarget ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !ships_at_my_stargrid_position[i]->is_derelict && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && !ships_at_my_stargrid_position[i]->ship_destruction_sequence_initiated ) {
				if ( ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) ) { // prioritize ships that are targeting us.
					bestTarget = ships_at_my_stargrid_position[i];
					break;
				}
			}
		}
	}
	if ( !bestTarget ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !ships_at_my_stargrid_position[i]->is_derelict && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) && !ships_at_my_stargrid_position[i]->ship_destruction_sequence_initiated ) {
				bestTarget = ships_at_my_stargrid_position[i];
				break;
			}
		}
	}
	if ( !bestTarget ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->IsType( sbShip::Type) && !ships_at_my_stargrid_position[i]->IsType( sbStationarySpaceEntity::Type ) ) {
				bestTarget = ships_at_my_stargrid_position[i];
				break;
			}
		}
	}
	if ( !bestTarget ) {
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] ) {
				bestTarget = ships_at_my_stargrid_position[i];
				break;
			}
		}
	}
	if ( bestTarget ) {
		//gameLocal.Printf( "The targetentityinspace of the " + name + " is the " + bestTarget->name + "\n" );
		Event_SetTargetShipInSpace(bestTarget);
	} else if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this && ships_at_my_stargrid_position.size() >= 1 ) { // BOYETTE NOTE: if we are the playership - just target something - anything that is at the same stargrid position.
		Event_SetTargetShipInSpace(ships_at_my_stargrid_position[0]);
	}
}

void sbShip::Event_ReturnBestFriendlyToProtect( void ) {
	sbShip* bestTarget = NULL;
	if ( prioritize_playership_as_space_entity_to_protect && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
		bestTarget = gameLocal.GetLocalPlayer()->PlayerShip;
		priority_space_entity_to_protect = gameLocal.GetLocalPlayer()->PlayerShip;
	} else {
		if ( priority_space_entity_to_protect && priority_space_entity_to_protect->IsType(sbShip::Type) ) {
			bestTarget = dynamic_cast<sbShip*>(priority_space_entity_to_protect);
		}
	}
	if ( bestTarget && bestTarget->stargridpositionx == stargridpositionx && bestTarget->stargridpositiony == stargridpositiony && !bestTarget->ship_destruction_sequence_initiated ) {
		idThread::ReturnEntity( static_cast<idEntity*>(bestTarget) );
		return;		
	} else {
		bestTarget = NULL;
	}

	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( !bestTarget && ships_at_my_stargrid_position[i]->team == team ) {
			bestTarget = ships_at_my_stargrid_position[i];
		}
		if ( ships_at_my_stargrid_position[i]->TargetEntityInSpace && ships_at_my_stargrid_position[i]->TargetEntityInSpace->team == team && ships_at_my_stargrid_position[i]->TargetEntityInSpace->team != ships_at_my_stargrid_position[i]->team ) {
			bestTarget = ships_at_my_stargrid_position[i]->TargetEntityInSpace;
			idThread::ReturnEntity( static_cast<idEntity*>(bestTarget) );
			return;
		}
	}
	if ( bestTarget ) {
		idThread::ReturnEntity( static_cast<idEntity*>(bestTarget) );
	} else {
		idThread::ReturnEntity( NULL );
	}
}
void sbShip::Event_WeAreAProtector( void ) {
	if ( we_are_a_protector ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

void sbShip::Event_ShipAIAggressiveness( void ) {
	idThread::ReturnInt( ship_ai_aggressiveness );
}

void sbShip::Event_CrewAllAboard( void ) {
	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] ) {
			if ( crew[i]->ShipOnBoard && crew[i]->ShipOnBoard != this ) {
				idThread::ReturnInt( false );
				return;
			}
		}
	}
	idThread::ReturnInt( true );
}
void sbShip::Event_CrewIsAboard( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] ) {
				if ( crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == ship_to_check ) {
					idThread::ReturnInt( true );
					return;
				}
			}
		}
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_SpareCrewIsNotOnTransporterPad() {
	std::vector<idAI*> spare_crew;
	int modules_exists_counter = 0;

	for ( int i = MAX_MODULES_ON_SHIPS-1; i >= 0; i-- ) {
		if ( spare_crew.size() >= max_spare_crew_size || modules_exists_counter >= max_modules_to_take_spare_crew_from ) { // BOYETTE NOTE IMPORTANT TODO: max_spare_crew_size will be determined by a spawnarg
			break;
		}
		if ( consoles[ModulesPowerQueue[i]] && consoles[ModulesPowerQueue[i]]->ControlledModule ) {
			modules_exists_counter++;
			if ( crew[ModulesPowerQueue[i]] ) {
				if ( crew[ModulesPowerQueue[i]]->ShipOnBoard && crew[ModulesPowerQueue[i]]->ShipOnBoard == this ) {
					spare_crew.push_back(crew[ModulesPowerQueue[i]]);
				}
			}
		} else if ( crew[ModulesPowerQueue[i]] ) {
			spare_crew.push_back(crew[ModulesPowerQueue[i]]);
		}
	}

	for ( int i = 0; i < spare_crew.size(); i++ ) {
		if ( spare_crew[i] && TransporterBounds && TransporterBounds->GetPhysics() && TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(spare_crew[i]->GetPhysics()->GetAbsBounds()) ) {

		} else {
			idThread::ReturnInt( true );
			return;
		}
	}
	if ( spare_crew.size() > 0 ) {
		idThread::ReturnInt( false );
	} else {
		idThread::ReturnInt( true );
	}
}
void sbShip::Event_OrderSpareCrewToMoveToTransporterPad() {
	std::vector<idAI*> spare_crew;
	int modules_exists_counter = 0;

	for ( int i = MAX_MODULES_ON_SHIPS-1; i >= 0; i-- ) {
		if ( spare_crew.size() >= max_spare_crew_size || modules_exists_counter >= max_modules_to_take_spare_crew_from ) { // BOYETTE NOTE IMPORTANT TODO: max_spare_crew_size will be determined by a spawnarg
			break;
		}
		if ( consoles[ModulesPowerQueue[i]] && consoles[ModulesPowerQueue[i]]->ControlledModule ) {
			modules_exists_counter++;
			if ( crew[ModulesPowerQueue[i]] ) {
				if ( crew[ModulesPowerQueue[i]]->ShipOnBoard && crew[ModulesPowerQueue[i]]->ShipOnBoard == this ) {
					spare_crew.push_back(crew[ModulesPowerQueue[i]]);
				}
			}
		} else if ( crew[ModulesPowerQueue[i]] ) {
			spare_crew.push_back(crew[ModulesPowerQueue[i]]);
		}
	}
	for ( int i = 0; i < spare_crew.size(); i++ ) {
		if ( spare_crew[i] && spare_crew[i]->ShipOnBoard && spare_crew[i]->ShipOnBoard->TransporterBounds ) {
			SetSelectedCrewMember(spare_crew[i]);
			GiveCrewMoveCommand(spare_crew[i]->ShipOnBoard->TransporterBounds,spare_crew[i]->ShipOnBoard);
			ClearCrewMemberSelection();
		}
	}
}
void sbShip::Event_OrderSpareCrewToReturnToBattlestations() {
	std::vector<idAI*> spare_crew;
	int modules_exists_counter = 0;

	for ( int i = MAX_MODULES_ON_SHIPS-1; i >= 0; i-- ) {
		if ( spare_crew.size() >= max_spare_crew_size || modules_exists_counter >= max_modules_to_take_spare_crew_from ) { // BOYETTE NOTE IMPORTANT TODO: max_spare_crew_size will be determined by a spawnarg
			break;
		}
		if ( consoles[ModulesPowerQueue[i]] && consoles[ModulesPowerQueue[i]]->ControlledModule ) {
			modules_exists_counter++;
			if ( crew[ModulesPowerQueue[i]] ) {
				if ( crew[ModulesPowerQueue[i]]->ShipOnBoard && crew[ModulesPowerQueue[i]]->ShipOnBoard == this ) {
					spare_crew.push_back(crew[ModulesPowerQueue[i]]);
					if ( !crew[ModulesPowerQueue[i]]->was_killed && consoles[ModulesPowerQueue[i]] ) {
						crew[ModulesPowerQueue[i]]->crew_auto_mode_activated = false;
						crew[ModulesPowerQueue[i]]->player_follow_mode_activated = false;
						crew[ModulesPowerQueue[i]]->handling_emergency_oxygen_situation = false;
						crew[ModulesPowerQueue[i]]->SetEntityToMoveToByCommand( consoles[ModulesPowerQueue[i]] );
					}
				}
			}
		} else if ( crew[ModulesPowerQueue[i]] ) {
			spare_crew.push_back(crew[ModulesPowerQueue[i]]);
			for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
				if ( consoles[x] && consoles[x]->ControlledModule ) {
					if ( !crew[ModulesPowerQueue[i]]->was_killed && consoles[ModulesPowerQueue[i]] ) {
						crew[ModulesPowerQueue[i]]->crew_auto_mode_activated = false;
						crew[ModulesPowerQueue[i]]->player_follow_mode_activated = false;
						crew[ModulesPowerQueue[i]]->handling_emergency_oxygen_situation = false;
						crew[ModulesPowerQueue[i]]->SetEntityToMoveToByCommand( consoles[x] );
					}
				}
			}
		}
	}
}

void sbShip::Event_InitiateShipTransporter() {
	BeginTransporterSequence();
}
void sbShip::Event_ShipIsSuitableForBoarding( idEntity* ship_to_check ) {
	if ( ignore_boarding_problems ) {
		idThread::ReturnInt( true );
		return;
	}
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip* ship;
		ship = static_cast<sbShip*>(ship_to_check);
		
		if ( /*ship->TransporterPad &&*/ ship->TransporterBounds && ((float)ship->hullStrength/(float)ship->max_hullStrength) > min_hullstrength_percent_required_for_boarding ) {
			if ( ship->infinite_oxygen || 
				( ship->consoles[ENVIRONMENTMODULEID] && ship->consoles[ENVIRONMENTMODULEID]->ControlledModule && 
				ship->consoles[ENVIRONMENTMODULEID]->ControlledModule->module_efficiency > min_environment_module_efficiency_required_for_boarding &&
				(float)ship->current_oxygen_level/100.0f > min_oxygen_percent_required_for_boarding ) ) {

				idThread::ReturnInt( true );
			} else {
				idThread::ReturnInt( false );
			}
		} else {
			idThread::ReturnInt( false );
		}
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_ShipShieldsAreLowEnoughForBoarding( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip* ship;
		ship = static_cast<sbShip*>(ship_to_check);
		if ( ship->shieldStrength < ( ship->min_shields_percent_for_blocking_foreign_transporters * ship->max_shieldStrength ) || ship->team == team ) {
			idThread::ReturnInt( true );
		} else {
			idThread::ReturnInt( false );
		}
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_IsCrewRetreivableFrom( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip* ship = static_cast<sbShip*>(ship_to_check);
		if ( ship->stargridpositionx == stargridpositionx && ship->stargridpositiony == stargridpositiony ) { //BOYETTE NOTE 05 26 2016: not sure why this was here - && ( ship->shieldStrength >= ( ship->min_shields_percent_for_blocking_foreign_transporters * ship->max_shieldStrength ) || ship->team == team ) ) {
			idThread::ReturnInt( true );
		}
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_ShipWithOurCrewAboardIt( void ) {
	sbShip* ship_with_our_crew = NULL;
	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] ) {
			if ( crew[i]->ShipOnBoard && crew[i]->ShipOnBoard != this ) {

				if ( !ship_with_our_crew ) {
					ship_with_our_crew = crew[i]->ShipOnBoard;
				}
				if ( crew[i]->ShipOnBoard->stargridpositionx == stargridpositionx && crew[i]->ShipOnBoard->stargridpositiony == stargridpositiony ) { // prioritize ships that are at the same stargrid position
					ship_with_our_crew = crew[i]->ShipOnBoard;
					 //BOYETTE NOTE 05 26 2016: not sure why this was here - if ( ( ship_with_our_crew->min_shields_percent_for_blocking_foreign_transporters * ship_with_our_crew->max_shieldStrength ) || ship_with_our_crew->team == team ) {
						idThread::ReturnEntity( static_cast<idEntity*>(ship_with_our_crew) );
						return;
					//}
				}

			}
		}
	}
	if ( ship_with_our_crew ) {
		idThread::ReturnEntity( static_cast<idEntity*>(ship_with_our_crew) );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

void sbShip::Event_CrewOnBoardShipIsNotOnTransporterPad( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip*	ship = static_cast<sbShip*>(ship_to_check);

		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && ship && crew[i]->ShipOnBoard == ship && crew[i]->ShipOnBoard->stargridpositionx == stargridpositionx && crew[i]->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				if ( ship->TransporterBounds && ship->TransporterBounds->GetPhysics() && ship->TransporterBounds->GetPhysics()->GetAbsBounds().IntersectsBounds(crew[i]->GetPhysics()->GetAbsBounds()) ) {

				} else {
					idThread::ReturnInt( true );
					return;
				}
			}
		}

	}
	idThread::ReturnInt( false );
}
void sbShip::Event_OrderCrewOnBoardShipToMoveToTransporterPad( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip*	ship = static_cast<sbShip*>(ship_to_check);

		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && ship && crew[i]->ShipOnBoard == ship && crew[i]->ShipOnBoard->stargridpositionx == stargridpositionx && crew[i]->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				SetSelectedCrewMember(crew[i]);
				GiveCrewMoveCommand(crew[i]->ShipOnBoard->TransporterBounds,crew[i]->ShipOnBoard);
				ClearCrewMemberSelection();
			}
		}

	}
}
void sbShip::Event_InitiateShipRetrievalTransporter() {
	BeginRetrievalTransportSequence();
}

void sbShip::Event_ActivateAutoModeForCrewAboardShip( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip*	ship = static_cast<sbShip*>(ship_to_check);

		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && ship && crew[i]->ShipOnBoard == ship && crew[i]->ShipOnBoard->stargridpositionx == stargridpositionx && crew[i]->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				crew[i]->crew_auto_mode_activated = true;
				crew[i]->handling_emergency_oxygen_situation = false;
			}
		}

		// MAYBE SET A BOARDING GOAL BEGIN
		boarders_should_target_player = false;
		boarders_should_target_random_module = false;
		float random_boarding_goal = gameLocal.random.RandomFloat();
		if ( random_boarding_goal < 0.50f && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == ship ) {
			boarders_should_target_player = true;
			//gameLocal.Printf( name + " boarders_should_target_player = true\n" );
		} else if ( random_boarding_goal >= 0.50f && random_boarding_goal < 0.75f ) {
			std::vector<int> random_module_ids;
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( ship->consoles[i] && ship->consoles[i]->ControlledModule ) {
					random_module_ids.push_back(i);
				}
			}
			if ( random_module_ids.size() > 0 ) {
				boarders_should_target_random_module = true;
				boarders_should_target_module_id = random_module_ids[gameLocal.random.RandomInt(random_module_ids.size())];
				//gameLocal.Printf( name + " boarders_should_target_random_module = true" + " | module id: " + idStr(boarders_should_target_module_id) + "\n" );
			}
		} else if ( random_boarding_goal >= 0.75f ) {
			//gameLocal.Printf( name + " nearest_module = true\n" );
			// if neither of the above bools are set to true the boarders will just go to the closest module.
		}
		// MAYBE SET A BOARDING GOAL END
	}
}
void sbShip::Event_DeactivateAutoModeForCrewAboardShip( idEntity* ship_to_check ) {
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip*	ship = static_cast<sbShip*>(ship_to_check);

		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && ship && crew[i]->ShipOnBoard == ship && crew[i]->ShipOnBoard->stargridpositionx == stargridpositionx && crew[i]->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				crew[i]->crew_auto_mode_activated = false;
				crew[i]->player_follow_mode_activated = false;
			}
		}
	}
}


void sbShip::Event_PrioritizeWeaponsAndTorpedosModulesInMyAutoPowerQueue( void ) {
	modules_power_automanage_on = true;

	SwapModueleIDInModulesPowerQueue(ENVIRONMENTMODULEID,0);
	SwapModueleIDInModulesPowerQueue(TORPEDOSMODULEID,1); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	SwapModueleIDInModulesPowerQueue(WEAPONSMODULEID,2); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	SwapModueleIDInModulesPowerQueue(COMPUTERMODULEID,3); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);

	AutoManageModulePowerlevels(); // BOYETTE NOTE IMPORTANT TODO: need to make an Event_TurnOnAutoManageModulePower and include this function in it. Actually it should always be on so maybe do it at spawn in the ship AI script

	/*
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);
	MoveUpModuleInModulesPowerQueue(WEAPONSMODULEID);

	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);
	MoveUpModuleInModulesPowerQueue(TORPEDOSMODULEID);

	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	MoveUpModuleInModulesPowerQueue(ENVIRONMENTMODULEID);
	*/
	/*
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[WEAPONSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[WEAPONSMODULEID]->ControlledModule);

	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		IncreaseModulePower(consoles[TORPEDOSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[TORPEDOSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[TORPEDOSMODULEID]->ControlledModule);
		IncreaseModulePower(consoles[TORPEDOSMODULEID]->ControlledModule);
	}
	*/
}
void sbShip::Event_PrioritizeEnginesInMyAutoPowerQueue( void ) {
	modules_power_automanage_on = true;
	SwapModueleIDInModulesPowerQueue(ENVIRONMENTMODULEID,0);
	SwapModueleIDInModulesPowerQueue(ENGINESMODULEID,1); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	SwapModueleIDInModulesPowerQueue(COMPUTERMODULEID,2); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	AutoManageModulePowerlevels(); // BOYETTE NOTE IMPORTANT TODO: need to make an Event_TurnOnAutoManageModulePower and include this function in it. Actually it should always be on so maybe do it at spawn in the ship AI script
}
void sbShip::Event_PrioritizeEnginesShieldsOxygenWeaponsAndTorpedosModulesInMyAutoPowerQueue( void ) {
	modules_power_automanage_on = true;
	SwapModueleIDInModulesPowerQueue(ENGINESMODULEID,0); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	SwapModueleIDInModulesPowerQueue(SHIELDSMODULEID,1);
	SwapModueleIDInModulesPowerQueue(ENVIRONMENTMODULEID,2);
	SwapModueleIDInModulesPowerQueue(WEAPONSMODULEID,3);
	SwapModueleIDInModulesPowerQueue(TORPEDOSMODULEID,4);
	SwapModueleIDInModulesPowerQueue(COMPUTERMODULEID,5); // BOYETTE NOTE IMPORTANT TODO: we should probably AutoManageModulePowerlevels(); inside these kinds of module queue position functions (only if modules_power_automanage_on is true);
	AutoManageModulePowerlevels(); // BOYETTE NOTE IMPORTANT TODO: need to make an Event_TurnOnAutoManageModulePower and include this function in it. Actually it should always be on so maybe do it at spawn in the ship AI script
}

void sbShip::Event_TurnAutoFireOn( void ) {
	weapons_autofire_on = true;
	torpedos_autofire_on = true;
}

void sbShip::Event_CeaseFire( void ) {
	CeaseFiringWeaponsAndTorpedos();
}

void sbShip::Event_CanAttemptWarp( void ) {
	if ( !is_attempting_warp && consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->health > 0 && consoles[ENGINESMODULEID]->ControlledModule->current_charge_amount >= consoles[ENGINESMODULEID]->ControlledModule->max_charge_amount ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

void sbShip::Event_AttemptWarpTowardsEntityAvoidingHostilesIfPossible( idEntity* ent ) {
	if ( ent ) {
		std::vector<idVec2>	warp_options;
		idVec2				best_warp_option;
		float				best_distance = idMath::INFINITY;

		int option_x;
		int option_y;
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}

		for ( int i = 0; i < warp_options.size() ; i++ ) {
			if ( !gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(warp_options[i].x, warp_options[i].y) ) { // BOYETTE NOTE TODO IMPORTANT: we might also want to check if gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() //// We might also want to check CheckAllNonPlayerSkyPortalsForOccupancy()
				float distance = (float)idMath::Sqrt( (ent->stargridpositionx - warp_options[i].x)*(ent->stargridpositionx - warp_options[i].x) + (ent->stargridpositiony - warp_options[i].y)*(ent->stargridpositiony - warp_options[i].y) );
				if ( distance < best_distance ) {
					// BOYETTE NOTE TODO IMPORTANT: and then we need to find an efficient way to check if the position has hostiles and the x or y is greater than 1 distance from the best option.
					best_distance = distance;
					best_warp_option.x = warp_options[i].x;
					best_warp_option.y = warp_options[i].y;
				}
			}
		}

		is_attempting_warp = AttemptWarp(best_warp_option.x,best_warp_option.y);
	}
}
void sbShip::Event_AttemptWarpAwayFromEntityAvoidingHostilesIfPossible( idEntity* ent ) {
	if ( ent ) {
		std::vector<idVec2>	warp_options;
		idVec2				best_warp_option;
		float				best_distance = 0;

		int option_x;
		int option_y;
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}

		/* original - they woul always warp diagonally since that would put them the farthest away.
		for ( int i = 0; i < warp_options.size() ; i++ ) {
			if ( !gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(warp_options[i].x, warp_options[i].y) ) { // BOYETTE NOTE TODO IMPORTANT: we might also want to check if gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() //// We might also want to check CheckAllNonPlayerSkyPortalsForOccupancy()
				float distance = (float)idMath::Sqrt( (ent->stargridpositionx - warp_options[i].x)*(ent->stargridpositionx - warp_options[i].x) + (ent->stargridpositiony - warp_options[i].y)*(ent->stargridpositiony - warp_options[i].y) );
				if ( distance > best_distance ) {
					// BOYETTE NOTE TODO IMPORTANT: we need to first check if their are hostiles here. we need to find an efficient way to check if the position has hostiles and the x or y is greater than 1 distance from the best option.
					best_distance = distance;
					best_warp_option.x = warp_options[i].x;
					best_warp_option.y = warp_options[i].y;
				}
			}
		}
		*/

		float current_distance = (float)idMath::Sqrt( (ent->stargridpositionx - stargridpositionx)*(ent->stargridpositionx - stargridpositionx) + (ent->stargridpositiony - stargridpositiony)*(ent->stargridpositiony - stargridpositiony) );
		for ( int i = 0; i < warp_options.size() ; i++ ) {
			if ( gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(warp_options[i].x, warp_options[i].y) ) { // BOYETTE NOTE TODO IMPORTANT: we might also want to check if gameLocal.GetNumberOfNonPlayerShipsAtStarGridPosition(stargriddestx, stargriddesty) >= gameLocal.GetNumberOfNonPlayerShipSkyPortalEntities() //// We might also want to check CheckAllNonPlayerSkyPortalsForOccupancy()
				warp_options.erase(warp_options.begin() + i); // we remove any warp options that will move us closer the the specified entity.
				i--;
				continue;
			}
			float distance = (float)idMath::Sqrt( (ent->stargridpositionx - warp_options[i].x)*(ent->stargridpositionx - warp_options[i].x) + (ent->stargridpositiony - warp_options[i].y)*(ent->stargridpositiony - warp_options[i].y) );
			if ( distance < current_distance ) {
				// BOYETTE NOTE TODO IMPORTANT: we need to first check if their are hostiles here. we need to find an efficient way to check if the position has hostiles and the x or y is any closer to the hostile we are fleeing.
				warp_options.erase(warp_options.begin() + i); // we remove any warp options that will move us closer the the specified entity.
				i--;
			}
		}

		int warp_option = gameLocal.random.RandomInt(warp_options.size()-1);
		best_warp_option.x = warp_options[warp_option].x;
		best_warp_option.y = warp_options[warp_option].y;

		is_attempting_warp = AttemptWarp(best_warp_option.x,best_warp_option.y);
	}
}

void sbShip::Event_ShouldHailThePlayerShip( void ) {
	bool all_conditionals_satisfied = true;
	bool has_hail_conditional = false;

	if ( should_hail_the_playership == false ) {

		if ( hail_conditional_hull_below_this_percentage > 0.0f ) {
			has_hail_conditional = true;
			if ( (float)hullStrength/(float)max_hullStrength < hail_conditional_hull_below_this_percentage ) {
				hail_conditional_hull_below_this_percentage = 0.0f;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_no_hostiles_at_my_stargrid_position ) {
			has_hail_conditional = true;
			bool there_are_hostiles_at_my_stargrid_position = false;
			for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) ) {
					there_are_hostiles_at_my_stargrid_position = true;
					break;
				}
			}
			if ( !there_are_hostiles_at_my_stargrid_position ) {
				hail_conditional_no_hostiles_at_my_stargrid_position = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}
		if ( hail_conditional_hostiles_at_my_stargrid_position ) {
			has_hail_conditional = true;
			bool there_are_hostiles_at_my_stargrid_position = false;
			for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) ) {
					there_are_hostiles_at_my_stargrid_position = true;
					break;
				}
			}
			if ( there_are_hostiles_at_my_stargrid_position ) {
				hail_conditional_no_hostiles_at_my_stargrid_position = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_no_friendlies_at_my_stargrid_position ) {
			has_hail_conditional = true;
			bool there_are_friendlies_at_my_stargrid_position = false;
			for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team == team ) {
					there_are_friendlies_at_my_stargrid_position = true;
					break;
				}
			}
			if ( !there_are_friendlies_at_my_stargrid_position ) {
				hail_conditional_no_hostiles_at_my_stargrid_position = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}
		if ( hail_conditional_friendlies_at_my_stargrid_position ) {
			has_hail_conditional = true;
			bool there_are_friendlies_at_my_stargrid_position = false;
			for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
				if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team == team ) {
					there_are_friendlies_at_my_stargrid_position = true;
					break;
				}
			}
			if ( there_are_friendlies_at_my_stargrid_position ) {
				hail_conditional_no_hostiles_at_my_stargrid_position = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_captain_officer_killed ) {
			has_hail_conditional = true;
			if ( !crew[CAPTAINCREWID] || ( crew[CAPTAINCREWID] && crew[CAPTAINCREWID]->was_killed ) ) {
				hail_conditional_captain_officer_killed = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_player_is_aboard_playership ) {
			has_hail_conditional = true;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->ShipOnBoard == gameLocal.GetLocalPlayer()->PlayerShip ) {
				hail_conditional_player_is_aboard_playership = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_not_at_player_shiponboard_position ) {
			has_hail_conditional = true;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && ( gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx != stargridpositionx || gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony != stargridpositiony ) ) {
				hail_conditional_not_at_player_shiponboard_position = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( hail_conditional_is_playership_target ) {
			has_hail_conditional = true;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == this ) {
				hail_conditional_is_playership_target = false;
			} else {
				all_conditionals_satisfied = false;
			}
		}

		if ( has_hail_conditional && all_conditionals_satisfied ) {
			hail_conditionals_met = true;
			should_hail_the_playership = true;
			should_go_into_no_action_hail_mode_on_hail = true; // BOYETTE NOTE TODO IMPORTANT: whether this is set to true should be dependent on a spawnarg.
		}
	}

	idThread::ReturnInt( should_hail_the_playership );
}
void sbShip::Event_ReturnWaitToHailOrderNum( void ) {
	wait_to_hail_order_num = idMath::ClampFloat(0.1f,100.0f,wait_to_hail_order_num);
	idThread::ReturnFloat( wait_to_hail_order_num );
}
void sbShip::Event_PlayerIsInStarGridStoryWindowOrHailOrItIsNotOurTurnToHail( void ) {
	if ( gameLocal.GetLocalPlayer() ) {
		bool is_not_our_turn_to_hail = false;
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
			if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->wait_to_hail_order_num < wait_to_hail_order_num ) {
				is_not_our_turn_to_hail = true;
			}
		}
		idThread::ReturnInt( gameLocal.GetLocalPlayer()->currently_in_story_gui || ( gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->HailGui && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) || is_not_our_turn_to_hail );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_AttemptToHailThePlayerShip( void ) {
	OpenHailWithLocalPlayer();
}
void sbShip::Event_InNoActionHailMode( void ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
		idThread::ReturnInt( in_no_action_hail_mode );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_PutAllShipsAtTheSameSGPosIntoNoActionHailMode( void ) { // BOYETTE NOTE: this gets called by all ships at the stargrid position in their AI script as well.
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] ) {
			ships_at_my_stargrid_position[i]->in_no_action_hail_mode = true;
		}
	}
	in_no_action_hail_mode = true;

	if ( spawnArgs.GetBool( "should_pause_game_during_no_action_hail_mode", "1" ) ) {
		gameSoundWorld->FadeSoundClasses(0,-100.0f,2); //  -100.0f // all entities have a soundclass of zero unless it is set otherwise. 0.0f db is the default level.
		gameSoundWorld->FadeSoundClasses(3,-100.0f,2); //  -100.0f // ship alarms are soundclass 3. 0.0f db is the default level.
		gameSoundWorld->FadeSoundClasses(1,-10.0f,2); //  -10.0f // the ship hum sound volumes are set to soundclass 1 - we will dim it a bit here. The music shader for the story gui will be set to soundclass 2 I think. We can add more soundclasses if we need them, right now the limit is 4 - look for SOUND_MAX_CLASSES.
		g_stopTime.SetBool(true);
		g_stopTimeForceFrameNumUpdateDuring.SetBool(true);
		g_stopTimeForceRenderViewUpdateDuring.SetBool(true);
		g_enableSlowmo.SetBool( false );
	}
}
void sbShip::Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode( void ) {
	for ( int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
		if ( ships_at_my_stargrid_position[i] ) {
			ships_at_my_stargrid_position[i]->in_no_action_hail_mode = false;
		}
	}
	in_no_action_hail_mode = false;
	should_go_into_no_action_hail_mode_on_hail = false;

	if ( spawnArgs.GetBool( "should_pause_game_during_no_action_hail_mode", "1" ) ) {
		gameSoundWorld->FadeSoundClasses(0,0.0f,0.1f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. All entities have a soundclass of zero unless it is set otherwise.
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->music_shader_is_playing ) {
			gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f);
		} else {
			gameSoundWorld->FadeSoundClasses(3,0.0f,0.1f); // BOYETTE NOTE: ship alarms are soundclass 3. 0.0f db is the default level.
		}
		gameSoundWorld->FadeSoundClasses(1,0.0f,1.5f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. The space ship hum is set to soundclass 1.
		g_stopTime.SetBool(false);
		g_stopTimeForceFrameNumUpdateDuring.SetBool(false);
		g_stopTimeForceRenderViewUpdateDuring.SetBool(false);
	}
}
void sbShip::Event_GoToRedAlert( void ) {
	if ( !red_alert ) {
		GoToRedAlert();
	}
}
void sbShip::Event_CancelRedAlert( void ) {
	if ( red_alert ) {
		CancelRedAlert();
	}
}
void sbShip::Event_RaiseShields( void ) {
	RaiseShields();
}
void sbShip::Event_LowerShields( void ) {
	LowerShields();
}
void sbShip::Event_BattleStations( void ) {
	if ( !battlestations ) {
		SendCrewToBattlestations();
		battlestations = true;
	}
}
void sbShip::Event_HasFledFromShip( idEntity* ship_to_check ) { // BOYETTE NOTE: this function is run by the ship that is fleeing to see if it is far enough away from the chaser to stop fleeing.
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		sbShip* ship = static_cast<sbShip*>(ship_to_check);
		if ( gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(stargridpositionx,stargridpositiony) && (ship->stargridpositionx != stargridpositionx || ship->stargridpositiony != stargridpositiony) ) {
			idThread::ReturnInt( true );
		} else {
			if ( abs(ship->stargridpositionx - stargridpositionx) >= successful_flee_distance || abs(ship->stargridpositiony - stargridpositiony) >= successful_flee_distance ) {
				idThread::ReturnInt( true );
			} else {
				idThread::ReturnInt( false );
			}
		}
	} else {
		idThread::ReturnInt( true );
	}
}
void sbShip::Event_HasEscapedFromUs( idEntity* ship_to_check ) { // BOYETTE NOTE: this function is run by the chaser to see if the fleeing ship has escaped from us.
	if ( ship_to_check && ship_to_check->IsType(sbShip::Type) ) {
		if ( priority_space_entity_to_target && ship_to_check == priority_space_entity_to_target && spawnArgs.GetBool("always_move_to_priority_space_entity_to_target","0") ) {
			idThread::ReturnInt( false );
			return;
		}
		sbShip* ship = static_cast<sbShip*>(ship_to_check);
		if ( gameLocal.CheckIfStarGridPositionIsOffLimitsToShipAI(ship->stargridpositionx,ship->stargridpositiony) && (stargridpositionx != ship->stargridpositionx || stargridpositiony != ship->stargridpositiony) ) {
			idThread::ReturnInt( true );
		} else {
			if ( abs(ship->stargridpositionx - stargridpositionx) >= ship->successful_flee_distance || abs(ship->stargridpositiony - stargridpositiony) >= ship->successful_flee_distance ) {
				idThread::ReturnInt( true );
			} else {
				idThread::ReturnInt( false );
			}
		}
	} else {
		idThread::ReturnInt( true );
	}
}

void sbShip::Event_ShouldAlwaysMoveToPrioritySpaceEntityToTarget( void ) {
	idThread::ReturnInt( spawnArgs.GetBool("always_move_to_priority_space_entity_to_target","0") );
}
void sbShip::Event_ReturnPrioritySpaceEntityToTarget( void ) {
	if ( priority_space_entity_to_target && priority_space_entity_to_target->IsType(sbShip::Type) && priority_space_entity_to_target->team != team ) {
		idThread::ReturnEntity( priority_space_entity_to_target );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

//void sbShip::Event_ShouldAlwaysMoveToPrioritySpaceEntityToProtect( void ) {
//	idThread::ReturnInt( spawnArgs.GetBool("always_move_to_priority_space_entity_to_protect","0") );
//}
void sbShip::Event_ShouldAlwaysMoveToSpaceEntityToProtect( void ) {
	idThread::ReturnInt( spawnArgs.GetBool("always_move_to_space_entity_to_protect","0") );
}

void sbShip::Event_ShouldAlwaysMoveToPrioritySpaceEntityToProtect( void ) {
	idThread::ReturnInt( spawnArgs.GetBool("always_move_to_priority_space_entity_to_protect","0") );
}
void sbShip::Event_ReturnPrioritySpaceEntityToProtect( void ) {
	if ( priority_space_entity_to_protect && priority_space_entity_to_protect->IsType(sbShip::Type) && priority_space_entity_to_protect->team != team ) {
		idThread::ReturnEntity( priority_space_entity_to_protect );
	} else {
		idThread::ReturnEntity( NULL );
	}
}

	// MODULE PRIORITY QUEUES FUNCTIONS BEGIN
void sbShip::HandleEmergencyOxygenSituation() {
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && ( current_oxygen_level < 85 || consoles[ENVIRONMENTMODULEID]->ControlledModule->health <= 0 ) ) { // if we have an emergency oxygen situation - make sure we get that fixed
		SwapModueleIDInModulesPowerQueue(ENVIRONMENTMODULEID,0);
		IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule);

		if ( current_oxygen_level > 40 ) {
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( crew[i] && crew[i]->handling_emergency_oxygen_situation ) { // if the oxygen level is greater than 40 still, don't send more than one crewmember to the oxygen module
					return;
				}
			}
		}

		float best_distance = MAX_WORLD_SIZE;
		idAI*	ClosestCrewMember = NULL;
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this ) {
				float module_distance = ( crew[i]->GetPhysics()->GetOrigin() - consoles[ENVIRONMENTMODULEID]->ControlledModule->GetPhysics()->GetOrigin() ).LengthFast();
				if ( module_distance < best_distance && !crew[i]->handling_emergency_oxygen_situation ) { // if the oxygen level is less than 40, just keep sending crew to the oxygen module
					best_distance = module_distance;
					ClosestCrewMember = crew[i];
				}
			}
		}
		if ( ClosestCrewMember ) {
			SetSelectedCrewMember(ClosestCrewMember);
			GiveCrewMoveCommand(consoles[ENVIRONMENTMODULEID]->ControlledModule,this);
			ClearCrewMemberSelection();
			ClosestCrewMember->handling_emergency_oxygen_situation = true;
		}
	} else {
		// if there is no longer an emergency oxygen situation - return the crew to their stations
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this && crew[i]->handling_emergency_oxygen_situation ) {
				if ( consoles[i] && consoles[i]->ControlledModule ) {
					SetSelectedCrewMember(crew[i]);
					GiveCrewMoveCommand(consoles[i]->ControlledModule,this);
					ClearCrewMemberSelection();
				}
			}
		}

	}
}

void sbShip::Event_OptimizeModuleQueuesForIdleness( void ) {
	modules_power_automanage_on = true;
	ModulesPowerQueue[0] = ENVIRONMENTMODULEID;
	ModulesPowerQueue[1] = COMPUTERMODULEID;
	ModulesPowerQueue[2] = SHIELDSMODULEID;
	ModulesPowerQueue[3] = ENGINESMODULEID;
	ModulesPowerQueue[4] = MEDICALMODULEID;
	ModulesPowerQueue[5] = SECURITYMODULEID;
	ModulesPowerQueue[6] = SENSORSMODULEID;
	ModulesPowerQueue[7] = WEAPONSMODULEID;
	ModulesPowerQueue[8] = TORPEDOSMODULEID;

	ResetAutomanagePowerReserves();
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power ) {
		while ( consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power && current_automanage_power_reserve > 0 ) {
			IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule); // make sure the environment module gets full power.
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level < 1 ) {
			IncreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a minimum autopower power of 1
		}
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 1 ) {
			DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a maximum autopower power of 1
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForFleeing( void ) {
	modules_power_automanage_on = true;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		/*
		SwapModueleIDInWeaponsTargetQueue(WEAPONSMODULEID,0);
		SwapModueleIDInWeaponsTargetQueue(COMPUTERMODULEID,1);
		SwapModueleIDInWeaponsTargetQueue(TORPEDOSMODULEID,2);
		SwapModueleIDInWeaponsTargetQueue(ENGINESMODULEID,3);
		SwapModueleIDInWeaponsTargetQueue(SHIELDSMODULEID,4);
		SwapModueleIDInWeaponsTargetQueue(SENSORSMODULEID,5);
		SwapModueleIDInWeaponsTargetQueue(MEDICALMODULEID,6);
		SwapModueleIDInWeaponsTargetQueue(ENVIRONMENTMODULEID,7);
		SwapModueleIDInWeaponsTargetQueue(SECURITYMODULEID,8);
		*/
		WeaponsTargetQueue[0] = WEAPONSMODULEID;
		WeaponsTargetQueue[1] = COMPUTERMODULEID;
		WeaponsTargetQueue[2] = TORPEDOSMODULEID;
		WeaponsTargetQueue[3] = ENGINESMODULEID;
		WeaponsTargetQueue[4] = SHIELDSMODULEID;
		WeaponsTargetQueue[5] = SENSORSMODULEID;
		WeaponsTargetQueue[6] = MEDICALMODULEID;
		WeaponsTargetQueue[7] = ENVIRONMENTMODULEID;
		WeaponsTargetQueue[8] = SECURITYMODULEID;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		/*
		SwapModueleIDInTorpedosTargetQueue(WEAPONSMODULEID,0);
		SwapModueleIDInTorpedosTargetQueue(COMPUTERMODULEID,1);
		SwapModueleIDInTorpedosTargetQueue(TORPEDOSMODULEID,2);
		SwapModueleIDInTorpedosTargetQueue(ENGINESMODULEID,3);
		SwapModueleIDInTorpedosTargetQueue(SHIELDSMODULEID,4);
		SwapModueleIDInTorpedosTargetQueue(SENSORSMODULEID,5);
		SwapModueleIDInTorpedosTargetQueue(MEDICALMODULEID,6);
		SwapModueleIDInTorpedosTargetQueue(ENVIRONMENTMODULEID,7);
		SwapModueleIDInTorpedosTargetQueue(SECURITYMODULEID,8);
		*/
		TorpedosTargetQueue[0] = WEAPONSMODULEID;
		TorpedosTargetQueue[1] = COMPUTERMODULEID;
		TorpedosTargetQueue[2] = TORPEDOSMODULEID;
		TorpedosTargetQueue[3] = ENGINESMODULEID;
		TorpedosTargetQueue[4] = SHIELDSMODULEID;
		TorpedosTargetQueue[5] = SENSORSMODULEID;
		TorpedosTargetQueue[6] = MEDICALMODULEID;
		TorpedosTargetQueue[7] = ENVIRONMENTMODULEID;
		TorpedosTargetQueue[8] = SECURITYMODULEID;
	}
	/*
	SwapModueleIDInModulesPowerQueue(ENVIRONMENTMODULEID,0);
	SwapModueleIDInModulesPowerQueue(ENGINESMODULEID,1);
	SwapModueleIDInModulesPowerQueue(SHIELDSMODULEID,2);
	SwapModueleIDInModulesPowerQueue(WEAPONSMODULEID,3);
	SwapModueleIDInModulesPowerQueue(TORPEDOSMODULEID,4);
	SwapModueleIDInModulesPowerQueue(COMPUTERMODULEID,5);
	SwapModueleIDInModulesPowerQueue(SENSORSMODULEID,6);
	SwapModueleIDInModulesPowerQueue(SECURITYMODULEID,7);
	SwapModueleIDInModulesPowerQueue(MEDICALMODULEID,8);
	*/
	ModulesPowerQueue[0] = ENVIRONMENTMODULEID;
	ModulesPowerQueue[1] = ENGINESMODULEID;
	ModulesPowerQueue[2] = SHIELDSMODULEID;
	ModulesPowerQueue[3] = WEAPONSMODULEID;
	ModulesPowerQueue[4] = TORPEDOSMODULEID;
	ModulesPowerQueue[5] = COMPUTERMODULEID;
	ModulesPowerQueue[6] = SENSORSMODULEID;
	ModulesPowerQueue[7] = SECURITYMODULEID;
	ModulesPowerQueue[8] = MEDICALMODULEID;

	ResetAutomanagePowerReserves();
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power ) {
		while ( consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power && current_automanage_power_reserve > 0 ) {
			IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule); // make sure the environment module gets full power.
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level < 1 ) {
			IncreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a minimum autopower power of 1
		}
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 1 ) {
			DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a maximum autopower power of 1
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels(); // BOYETTE NOTE IMPORTANT TODO: need to make an Event_TurnOnAutoManageModulePower and include this function in it. Actually it should always be on so maybe do it at spawn in the ship AI script
}
void sbShip::Event_OptimizeModuleQueuesForDefending( void ) {
	modules_power_automanage_on = true;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		WeaponsTargetQueue[0] = WEAPONSMODULEID;
		WeaponsTargetQueue[1] = COMPUTERMODULEID;
		WeaponsTargetQueue[2] = TORPEDOSMODULEID;
		WeaponsTargetQueue[3] = SHIELDSMODULEID;
		WeaponsTargetQueue[4] = ENVIRONMENTMODULEID;
		WeaponsTargetQueue[5] = SENSORSMODULEID;
		WeaponsTargetQueue[6] = MEDICALMODULEID;
		WeaponsTargetQueue[7] = SECURITYMODULEID;
		WeaponsTargetQueue[8] = ENGINESMODULEID;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		TorpedosTargetQueue[0] = WEAPONSMODULEID;
		TorpedosTargetQueue[1] = COMPUTERMODULEID;
		TorpedosTargetQueue[2] = TORPEDOSMODULEID;
		TorpedosTargetQueue[3] = SHIELDSMODULEID;
		TorpedosTargetQueue[4] = ENVIRONMENTMODULEID;
		TorpedosTargetQueue[5] = SENSORSMODULEID;
		TorpedosTargetQueue[6] = MEDICALMODULEID;
		TorpedosTargetQueue[7] = SECURITYMODULEID;
		TorpedosTargetQueue[8] = ENGINESMODULEID;
	}
	ModulesPowerQueue[0] = ENVIRONMENTMODULEID;
	ModulesPowerQueue[1] = SHIELDSMODULEID;
	ModulesPowerQueue[2] = WEAPONSMODULEID;
	ModulesPowerQueue[3] = TORPEDOSMODULEID;
	ModulesPowerQueue[4] = COMPUTERMODULEID;
	ModulesPowerQueue[5] = SENSORSMODULEID;
	ModulesPowerQueue[6] = SECURITYMODULEID;
	ModulesPowerQueue[7] = MEDICALMODULEID;
	ModulesPowerQueue[8] = ENGINESMODULEID;

	ResetAutomanagePowerReserves();
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power ) {
		while ( consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power && current_automanage_power_reserve > 0 ) {
			IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule); // make sure the environment module gets full power.
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level < 1 ) {
			IncreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a minimum autopower power of 1
		}
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 1 ) {
			DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a maximum autopower power of 1
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForAttacking( void ) {
	modules_power_automanage_on = true;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		WeaponsTargetQueue[0] = SHIELDSMODULEID; // so that the target ship can be boarded
		WeaponsTargetQueue[1] = COMPUTERMODULEID;
		WeaponsTargetQueue[2] = TORPEDOSMODULEID;
		WeaponsTargetQueue[3] = WEAPONSMODULEID;
		WeaponsTargetQueue[4] = ENGINESMODULEID;
		WeaponsTargetQueue[5] = ENVIRONMENTMODULEID;
		WeaponsTargetQueue[6] = SENSORSMODULEID;
		WeaponsTargetQueue[7] = MEDICALMODULEID;
		WeaponsTargetQueue[8] = SECURITYMODULEID;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		TorpedosTargetQueue[0] = SHIELDSMODULEID; // so that the target ship can be boarded
		TorpedosTargetQueue[1] = COMPUTERMODULEID;
		TorpedosTargetQueue[2] = TORPEDOSMODULEID;
		TorpedosTargetQueue[3] = WEAPONSMODULEID;
		TorpedosTargetQueue[4] = ENGINESMODULEID;
		TorpedosTargetQueue[5] = ENVIRONMENTMODULEID;
		TorpedosTargetQueue[6] = SENSORSMODULEID;
		TorpedosTargetQueue[7] = MEDICALMODULEID;
		TorpedosTargetQueue[8] = SECURITYMODULEID;
	}
	ModulesPowerQueue[0] = ENVIRONMENTMODULEID;
	ModulesPowerQueue[1] = WEAPONSMODULEID;
	ModulesPowerQueue[2] = TORPEDOSMODULEID;
	ModulesPowerQueue[3] = COMPUTERMODULEID;
	ModulesPowerQueue[4] = SENSORSMODULEID;
	ModulesPowerQueue[5] = SHIELDSMODULEID;
	ModulesPowerQueue[6] = MEDICALMODULEID;
	ModulesPowerQueue[7] = SECURITYMODULEID;
	ModulesPowerQueue[8] = ENGINESMODULEID;

	ResetAutomanagePowerReserves();
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power ) {
		while ( consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power && current_automanage_power_reserve > 0 ) {
			IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule); // make sure the environment module gets full power.
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level < 1 ) {
			IncreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a minimum autopower power of 1
		}
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 1 ) {
			DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a maximum autopower power of 1
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForSeekingAndDestroying( void ) {
	modules_power_automanage_on = true;
	if ( consoles[WEAPONSMODULEID] && consoles[WEAPONSMODULEID]->ControlledModule ) {
		WeaponsTargetQueue[0] = ENGINESMODULEID;
		WeaponsTargetQueue[1] = SHIELDSMODULEID;
		WeaponsTargetQueue[2] = COMPUTERMODULEID;
		WeaponsTargetQueue[3] = ENVIRONMENTMODULEID;
		WeaponsTargetQueue[4] = MEDICALMODULEID;
		WeaponsTargetQueue[5] = WEAPONSMODULEID;
		WeaponsTargetQueue[6] = TORPEDOSMODULEID;
		WeaponsTargetQueue[7] = SENSORSMODULEID;
		WeaponsTargetQueue[8] = SECURITYMODULEID;
	}
	if ( consoles[TORPEDOSMODULEID] && consoles[TORPEDOSMODULEID]->ControlledModule ) {
		TorpedosTargetQueue[0] = ENGINESMODULEID;
		TorpedosTargetQueue[1] = SHIELDSMODULEID;
		TorpedosTargetQueue[2] = COMPUTERMODULEID;
		TorpedosTargetQueue[3] = ENVIRONMENTMODULEID;
		TorpedosTargetQueue[4] = MEDICALMODULEID;
		TorpedosTargetQueue[5] = WEAPONSMODULEID;
		TorpedosTargetQueue[6] = TORPEDOSMODULEID;
		TorpedosTargetQueue[7] = SENSORSMODULEID;
		TorpedosTargetQueue[8] = SECURITYMODULEID;
	}
	ModulesPowerQueue[0] = COMPUTERMODULEID;
	ModulesPowerQueue[1] = WEAPONSMODULEID;
	ModulesPowerQueue[2] = TORPEDOSMODULEID;
	ModulesPowerQueue[3] = ENGINESMODULEID;
	ModulesPowerQueue[4] = ENVIRONMENTMODULEID;
	ModulesPowerQueue[5] = SENSORSMODULEID;
	ModulesPowerQueue[6] = SHIELDSMODULEID;
	ModulesPowerQueue[7] = MEDICALMODULEID;
	ModulesPowerQueue[8] = SECURITYMODULEID;

	ResetAutomanagePowerReserves();
	if ( consoles[ENVIRONMENTMODULEID] && consoles[ENVIRONMENTMODULEID]->ControlledModule && consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power ) {
		while ( consoles[ENVIRONMENTMODULEID]->ControlledModule->automanage_target_power_level < consoles[ENVIRONMENTMODULEID]->ControlledModule->max_power && current_automanage_power_reserve > 0 ) {
			IncreaseAutomanageTargetModulePower(consoles[ENVIRONMENTMODULEID]->ControlledModule); // make sure the environment module gets full power.
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level < 1 ) {
			IncreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a minimum autopower power of 1
		}
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->automanage_target_power_level > 1 ) {
			DecreaseAutomanageTargetModulePower(consoles[i]->ControlledModule); // make sure each module gets a maximum autopower power of 1
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForMoving( void ) {
	modules_power_automanage_on = true;
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule && consoles[ENGINESMODULEID]->ControlledModule->module_efficiency < 100 ) {
		while ( ModulesPowerQueue[0] != ENGINESMODULEID ) {
			MoveUpModuleInModulesPowerQueue(ENGINESMODULEID);
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForBoarding( void ) {
	modules_power_automanage_on = true;
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->module_efficiency < 100 ) {
		while ( ModulesPowerQueue[0] != COMPUTERMODULEID ) {
			MoveUpModuleInModulesPowerQueue(COMPUTERMODULEID);
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}
void sbShip::Event_OptimizeModuleQueuesForRetrievingCrew( void ) {
	modules_power_automanage_on = true;
	if ( consoles[COMPUTERMODULEID] && consoles[COMPUTERMODULEID]->ControlledModule && consoles[COMPUTERMODULEID]->ControlledModule->module_efficiency < 100 ) {
		while ( ModulesPowerQueue[0] != COMPUTERMODULEID ) {
			MoveUpModuleInModulesPowerQueue(COMPUTERMODULEID);
		}
	}

	HandleEmergencyOxygenSituation();

	AutoManageModulePowerlevels();
}

void sbShip::Event_ReturnExtraShipAIWaitTimeForLowSensorsModuleEfficiency( void ) {
	if ( consoles[SENSORSMODULEID] && consoles[SENSORSMODULEID]->ControlledModule && consoles[SENSORSMODULEID]->ControlledModule->module_efficiency < 100 ) {
		float extra_wait_time = ( 1.00f - (float)((float)consoles[SENSORSMODULEID]->ControlledModule->module_efficiency/100.0f)) * 5.0f; // 5 is the maximum extra wait time for the AI actions due to low sensors module efficiency - we should make this a constant in the header file
		extra_wait_time = idMath::ClampFloat(0.1f,5.0f,extra_wait_time);
		idThread::ReturnFloat( extra_wait_time );
		//gameLocal.Printf( "The " + name + " is waiting " + idStr( extra_wait_time ) + " extra seconds due to low sensors module efficiency.\n" );
	} else {
		idThread::ReturnFloat( 0.1f );
	}
}
	// MODULE PRIORITY QUEUES FUNCTIONS END



	// SHIP DORMANCY EVENTS BEGIN
void sbShip::Event_IsDormantShip( void ) {
	idThread::ReturnInt( fl.isDormant );
}
void sbShip::Event_ShouldBecomeDormantShip( void ) {
	/*
	if ( !try_to_be_dormant ) {
		gameLocal.Printf( name + " main goal: " + idStr(main_goal.goal_action) + "\n" );
		for ( int i = 0; i < mini_goals.size(); i++ ) {
			gameLocal.Printf( name + " mini goal " + idStr(i) + " : " + idStr(mini_goals[i].goal_action) + "\n" );
		}
	}
	*/
	if ( try_to_be_dormant && main_goal.goal_action == SHIP_AI_IDLE ) {
		bool should_become_dormant = true;
		if ( ship_modules_must_be_repaired_to_go_dormant ) {
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->health < consoles[i]->ControlledModule->entity_max_health ) {
					should_become_dormant = false;
				}
			}
		}
		idThread::ReturnInt( should_become_dormant );
	} else {
		idThread::ReturnInt( false );
	}
}
void sbShip::Event_BecomeDormantShip( void ) {
	fl.neverDormant = false;
	try_to_be_dormant = true;
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] ) {
			AIsOnBoard[i]->fl.neverDormant = false;
		}
	}

	DespawnShipInteriorEntities();
}
void sbShip::Event_BecomeNonDormantShip( void ) {
	RespawnShipInteriorEntities();

	fl.neverDormant = true;
	try_to_be_dormant = false;
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] ) {
			AIsOnBoard[i]->fl.neverDormant = true;
			AIsOnBoard[i]->dormantStart = 0;
			AIsOnBoard[i]->fl.hasAwakened = true;
		}
	}
	// ADDED 07 05 2016 BEGIN
	if ( is_derelict && !never_derelict ) {
		Event_SetMinimumModulePowers();
		for( int i = 0; i < shipdoors.Num(); i++ ) {
			if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) && shipdoors[ i ].GetEntity()->health > 0 ) {
				dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->SetDoorGroupsHealth( 0 );
				dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->UpdateDoorGroupsMoverStatusShaderParms();
			}
		}
	}
	// ADDED 07 05 2016 END
}
	// SHIP DORMANCY EVENTS END


// SHIP DESPAWNING/RESPAWNING SYSTEM BEGIN
//BOYETTE NOTE TODO IMPORTANT: We need to change the spawnargs of the ship itself so that when we run DoStuffAfterAllMapEntitiesHaveSpawned() and DoStuffAfterPlayerHasSpawned() the entities will link up properly in case they changed from the originals. Any spawnargs that are in DoStuffAfterSpawn or DoStuffAFterPlayerSpawn. Do this for AIsOnBoard.size spawnargs
void sbShip::DespawnShipInteriorEntities() {
	// first we need to zero out the current power levels - so we can remove them.
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		while ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->power_allocated > 0 ) {
				DecreaseModulePower(consoles[i]->ControlledModule);
		}
	}
	
	// we don't need to store the projectile - it would collide with something soon anyways - just remove it.
	if ( projectile.GetEntity() ) {
		projectile.GetEntity()->CancelEvents( &EV_CheckTorpedoStatus );
		projectile.GetEntity()->CancelEvents( &EV_Remove );
		projectile.GetEntity()->Event_Remove();
		projectile = NULL;
	}

	// store the idDicts of the ai's on board who are not part of crew[MAX_CREW_ON_SHIPS]
	// BOYETTE NOTE TO THINK ABOUT: if an AIOnBoard has a parentship and we remove the AIOnBoard - we will have to make sure we store the parentship so we can link it up with it again later when we respawn it
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] && !IsThisAIACrewmember( AIsOnBoard[i] ) ) {
			spawnArgs_adjusted_AIsOnBoard.push_back( AIsOnBoard[i]->spawnArgs );
			spawnArgs_adjusted_AIsOnBoard.back().SetInt( "health", AIsOnBoard[i]->health );
			spawnArgs_adjusted_AIsOnBoard.back().SetInt( "entity_max_health", AIsOnBoard[i]->entity_max_health );
			spawnArgs_adjusted_AIsOnBoard.back().SetInt( "team", AIsOnBoard[i]->team );
			spawnArgs_adjusted_AIsOnBoard.back().SetVector( "origin", AIsOnBoard[i]->GetPhysics()->GetOrigin() );
			spawnArgs_adjusted_AIsOnBoard.back().SetInt( "space_command_level", AIsOnBoard[i]->space_command_level );
			if ( AIsOnBoard[i]->ParentShip && AIsOnBoard[i]->ParentShip != this ) {
				spawnArgs_adjusted_AIsOnBoard.back().Set( "ParentShip", AIsOnBoard[i]->ParentShip->name );
			}
		}
	}
	// remove the AI's on board who are not part of crew[MAX_CREW_ON_SHIPS]
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] && !IsThisAIACrewmember( AIsOnBoard[i] ) ) {
			AIsOnBoard[i]->ShipOnBoard = NULL;
			AIsOnBoard[i]->Event_Remove();
			AIsOnBoard[i] = NULL;
		}
	}
	AIsOnBoard.clear();


	// store the idDicts of the crew that is on board BOYETTE NOTE IMPORTANT: if the crew ship on board is a different ship - don't remove it - we can always link up with us again later.
	for ( int i = 0; i < MAX_CREW_ON_SHIPS ; i++ ) {
		if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this ) {
			spawnArgs_adjusted_crew[i] = crew[i]->spawnArgs;
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->SyncUpPlayerShipNameCVars();
			}
			spawnArgs_adjusted_crew[i].SetInt( "health", crew[i]->health );
			spawnArgs_adjusted_crew[i].SetInt( "entity_max_health", crew[i]->entity_max_health );
			spawnArgs_adjusted_crew[i].SetInt( "team", crew[i]->team );
			spawnArgs_adjusted_crew[i].SetVector( "origin", crew[i]->GetPhysics()->GetOrigin() );
		}
	}
	// remove the crew that is on board
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this ) {
			crew[i]->ParentShip = NULL;
			crew[i]->Event_Remove();
		}
		crew[i] = NULL;
	}


	// store the idDicts of the room nodes
	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		if ( room_node[i] ) {
			spawnArgs_adjusted_room_node[i] = room_node[i]->spawnArgs;
			spawnArgs_adjusted_room_node[i].SetInt( "team", room_node[i]->team ); // probably not necessary
			spawnArgs_adjusted_room_node[i].SetVector( "origin", room_node[i]->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_room_node[i].SetMatrix( "rotation", room_node[i]->GetPhysics()->GetAxis() ); // probably not necessary
		}
	}
	// remove the room nodes
	for( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
		if ( room_node[i] ) room_node[i]->Event_Remove();
		room_node[i] = NULL;
	}

	
	// store the idDicts of the consoles
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS ; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule ) {
			spawnArgs_adjusted_consoles_ControlledModule[i] = consoles[i]->ControlledModule->spawnArgs;
			spawnArgs_adjusted_consoles_ControlledModule[i].SetInt( "health", consoles[i]->ControlledModule->health );
			spawnArgs_adjusted_consoles_ControlledModule[i].SetInt( "entity_max_health", consoles[i]->ControlledModule->entity_max_health );
			spawnArgs_adjusted_consoles_ControlledModule[i].SetInt( "team", consoles[i]->ControlledModule->team );
			spawnArgs_adjusted_consoles_ControlledModule[i].SetVector( "origin", consoles[i]->ControlledModule->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_consoles_ControlledModule[i].SetMatrix( "rotation", consoles[i]->ControlledModule->GetPhysics()->GetAxis() ); // probably not necessary
		}
	}
	// store the idDicts of the consoles' modules
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS ; i++ ) {
		if ( consoles[i] ) {
			spawnArgs_adjusted_consoles[i] = consoles[i]->spawnArgs;
			spawnArgs_adjusted_consoles[i].SetInt( "health", consoles[i]->health );
			spawnArgs_adjusted_consoles[i].SetInt( "entity_max_health", consoles[i]->entity_max_health );
			spawnArgs_adjusted_consoles[i].SetInt( "team", consoles[i]->team );
			spawnArgs_adjusted_consoles[i].SetVector( "origin", consoles[i]->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_consoles[i].SetMatrix( "rotation", consoles[i]->GetPhysics()->GetAxis() ); // probably not necessary
		}
	}
	// remove the consoles and the consoles' modules
	for( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule ) consoles[i]->ControlledModule->Event_Remove();
		if ( consoles[i] ) consoles[i]->ControlledModule = NULL;
		if ( consoles[i] ) consoles[i]->Event_Remove();
		consoles[i] = NULL;
	}
	

	// store the idDict of the captain chair
	if ( CaptainChair ) {
			spawnArgs_adjusted_CaptainChair = CaptainChair->spawnArgs;
			spawnArgs_adjusted_CaptainChair.SetInt( "team", CaptainChair->team );
			spawnArgs_adjusted_CaptainChair.SetVector( "origin", CaptainChair->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_CaptainChair.SetMatrix( "rotation", CaptainChair->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the captain chair
	if ( CaptainChair ) {
		if ( CaptainChair->SeatedEntity.GetEntity() ) {
			if ( gameLocal.GetLocalPlayer() && CaptainChair->SeatedEntity.GetEntity() == gameLocal.GetLocalPlayer() ) {
				CaptainChair->ReleasePlayerCaptain();
			}
		}
		CaptainChair->Event_Remove();
		CaptainChair = NULL;
	}
	

	// store the idDict of the ready room captain chair
	if ( ReadyRoomCaptainChair ) {
			spawnArgs_adjusted_ReadyRoomCaptainChair = ReadyRoomCaptainChair->spawnArgs;
			spawnArgs_adjusted_ReadyRoomCaptainChair.SetInt( "team", ReadyRoomCaptainChair->team );
			spawnArgs_adjusted_ReadyRoomCaptainChair.SetVector( "origin", ReadyRoomCaptainChair->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_ReadyRoomCaptainChair.SetMatrix( "rotation", ReadyRoomCaptainChair->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// the ready room captain chair
	if ( ReadyRoomCaptainChair ) {
		if ( ReadyRoomCaptainChair->SeatedEntity.GetEntity() ) {
			if ( gameLocal.GetLocalPlayer() && ReadyRoomCaptainChair->SeatedEntity.GetEntity() == gameLocal.GetLocalPlayer() ) {
				ReadyRoomCaptainChair->ReleasePlayerCaptain();
			}
		}
		ReadyRoomCaptainChair->Event_Remove();
		ReadyRoomCaptainChair = NULL;
	}
	
	
	// store the idDict of the shield entity
	if ( ShieldEntity ) {
			spawnArgs_adjusted_ShieldEntity = ShieldEntity->spawnArgs;
			spawnArgs_adjusted_ShieldEntity.SetInt( "team", ShieldEntity->team );
			spawnArgs_adjusted_ShieldEntity.SetVector( "origin", ShieldEntity->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_ShieldEntity.SetMatrix( "rotation", ShieldEntity->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the shield entity
	if ( ShieldEntity ) {
		ShieldEntity->Event_Remove();
		ShieldEntity = NULL;
	}

	
	// store the idDict of the transporter bounds
	if ( TransporterBounds ) {
			spawnArgs_adjusted_TransporterBounds = TransporterBounds->spawnArgs;
			spawnArgs_adjusted_TransporterBounds.SetInt( "team", TransporterBounds->team );
			spawnArgs_adjusted_TransporterBounds.SetVector( "origin", TransporterBounds->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_TransporterBounds.SetMatrix( "rotation", TransporterBounds->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the transporter bounds
	if ( TransporterBounds ) {
		TransporterBounds->Event_Remove();
		TransporterBounds = NULL;
	}
	
	
	// store the idDict of the transporter pad
	if ( TransporterPad ) {
			spawnArgs_adjusted_TransporterPad = TransporterPad->spawnArgs;
			spawnArgs_adjusted_TransporterPad.SetInt( "team", TransporterPad->team );
			spawnArgs_adjusted_TransporterPad.SetVector( "origin", TransporterPad->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_TransporterPad.SetMatrix( "rotation", TransporterPad->GetPhysics()->GetAxis() ); // probably not necessary
	}	
	// remove the transporter pad
	if ( TransporterPad ) {
		TransporterPad->Event_Remove();
		TransporterPad = NULL;
	}
	
	
	// store the idDict of the transporter particle entity spawn marker
	if ( TransporterParticleEntitySpawnMarker ) {
			spawnArgs_adjusted_TransporterParticleEntitySpawnMarker = TransporterParticleEntitySpawnMarker->spawnArgs;
			spawnArgs_adjusted_TransporterParticleEntitySpawnMarker.SetInt( "team", TransporterParticleEntitySpawnMarker->team );
			spawnArgs_adjusted_TransporterParticleEntitySpawnMarker.SetVector( "origin", TransporterParticleEntitySpawnMarker->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_TransporterParticleEntitySpawnMarker.SetMatrix( "rotation", TransporterParticleEntitySpawnMarker->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the transporter particle entity spawn marker	
	if ( TransporterParticleEntitySpawnMarker ) {
		TransporterParticleEntitySpawnMarker->Event_Remove();
		TransporterParticleEntitySpawnMarker = NULL;
	}
	
	
	// store the idDict of the ship diagram display node
	if ( ShipDiagramDisplayNode ) {
			spawnArgs_adjusted_ShipDiagramDisplayNode = ShipDiagramDisplayNode->spawnArgs;
			spawnArgs_adjusted_ShipDiagramDisplayNode.SetInt( "team", ShipDiagramDisplayNode->team );
			spawnArgs_adjusted_ShipDiagramDisplayNode.SetVector( "origin", ShipDiagramDisplayNode->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_ShipDiagramDisplayNode.SetMatrix( "rotation", ShipDiagramDisplayNode->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the ship diagram display node
	if ( ShipDiagramDisplayNode ) {
		ShipDiagramDisplayNode->Event_Remove();
		ShipDiagramDisplayNode = NULL;
	}
	
	
	// store the idDict of the view screen entity
	if ( ViewScreenEntity ) {
			spawnArgs_adjusted_ViewScreenEntity = ViewScreenEntity->spawnArgs;
			spawnArgs_adjusted_ViewScreenEntity.SetInt( "team", ViewScreenEntity->team );
			spawnArgs_adjusted_ViewScreenEntity.SetVector( "origin", ViewScreenEntity->GetPhysics()->GetOrigin() ); // probably not necessary
			spawnArgs_adjusted_ViewScreenEntity.SetMatrix( "rotation", ViewScreenEntity->GetPhysics()->GetAxis() ); // probably not necessary
	}
	// remove the view screen entity
	if ( ViewScreenEntity ) {
		ViewScreenEntity->Event_Remove();
		ViewScreenEntity = NULL;
	}
	
	
	// we don't need to store the idDict of the transporter particle entity fx
	//if ( TransporterParticleEntityFX.GetEntity() ) {
	//		spawnArgs_adjusted_TransporterParticleEntityFX = TransporterParticleEntityFX.GetEntity().spawnArgs;
	//		spawnArgs_adjusted_TransporterParticleEntityFX.SetInt( "team", TransporterParticleEntityFX.GetEntity()->team );
	//		spawnArgs_adjusted_TransporterParticleEntityFX.SetVector( "origin", TransporterParticleEntityFX.GetEntity()->GetPhysics()->GetOrigin() ); // probably not necessary
	//		spawnArgs_adjusted_TransporterParticleEntityFX.SetMatrix( "rotation", TransporterParticleEntityFX.GetEntity()->GetPhysics()->GetAxis() ); // probably not necessary
	//}

	// remove the transporter particle entity fx
	if ( TransporterParticleEntityFX.GetEntity() ) {
		TransporterParticleEntityFX.GetEntity()->Event_Remove();
		TransporterParticleEntityFX = NULL;
	}


	// store the idDict of the ship doors // BOYETTE NOTE TODO IMPORTANT - we will need to store both the idDicts of the door and any doors in the door group
	// BOYETTE TODO: we will need to make a function on idDoor that returns a vector of vectors idDicts full of the moveMasters(and up its chain) and activateChains(and up its chain) for a given shipdoors[ i ],
	// once we have all these when we do the repawn function we can spawn all of them first and then spawn the spawnArgs_adjusted_shipdoors[i] (it will find it's partners in its own spawn function)
	//std::vector<idDict> spawnArgs_adjusted_shipdoors_partners;
	for( int i = 0; i < shipdoors.Num(); i++ ) {
		if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) ) {
			//shipdoors[ i ].GetEntity()->GetDoorGroupPartners( std::vector& add_to_this_vector); // insert spawnArgs_adjusted_shipdoors_partners
			
			idDoor* shipdoor_to_spawn_partners_from = dynamic_cast<idDoor*>(shipdoors[ i ].GetEntity());
			idDoor* ship_door_ActivateChain = NULL;
			idDoor* ship_door_MoveMaster = NULL;
			if ( shipdoor_to_spawn_partners_from ) {
				if ( shipdoor_to_spawn_partners_from->GetActivateChain() && shipdoor_to_spawn_partners_from->GetActivateChain() != shipdoor_to_spawn_partners_from && shipdoor_to_spawn_partners_from->GetActivateChain()->IsType( idDoor::Type ) ) {
					ship_door_ActivateChain = dynamic_cast<idDoor*>( shipdoor_to_spawn_partners_from->GetActivateChain() );
				}
				if ( shipdoor_to_spawn_partners_from->GetMoveMaster() && shipdoor_to_spawn_partners_from->GetMoveMaster() != shipdoor_to_spawn_partners_from && shipdoor_to_spawn_partners_from->GetMoveMaster()->IsType( idDoor::Type ) ) {
					ship_door_MoveMaster = dynamic_cast<idDoor*>( shipdoor_to_spawn_partners_from->GetMoveMaster() );
				}
			}

			if ( ship_door_ActivateChain ) {
				spawnArgs_adjusted_shipdoors_partners.push_back( ship_door_ActivateChain->spawnArgs );
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "idEntity_team", team ); // we might not be sure of the size of this vector so we'll use .back() here just to be safe
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "health", ship_door_ActivateChain->health );
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "entity_max_health", ship_door_ActivateChain->entity_max_health );
				//spawnArgs_adjusted_shipdoors_partners.back().SetVector( "origin", ship_door_ActivateChain->GetPhysics()->GetOrigin() ); // probably not necessary
				//spawnArgs_adjusted_shipdoors_partners.back().SetMatrix( "rotation", ship_door_ActivateChain->GetPhysics()->GetAxis() ); // probably not necessary
			}
			if ( ship_door_MoveMaster ) {
				spawnArgs_adjusted_shipdoors_partners.push_back( ship_door_MoveMaster->spawnArgs );
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "idEntity_team", team ); // we might not be sure of the size of this vector so we'll use .back() here just to be safe
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "health", ship_door_MoveMaster->health );
				spawnArgs_adjusted_shipdoors_partners.back().SetInt( "entity_max_health", ship_door_MoveMaster->entity_max_health );
				//spawnArgs_adjusted_shipdoors_partners.back().SetVector( "origin", ship_door_MoveMaster->GetPhysics()->GetOrigin() ); // probably not necessary
				//spawnArgs_adjusted_shipdoors_partners.back().SetMatrix( "rotation", ship_door_MoveMaster->GetPhysics()->GetAxis() ); // probably not necessary
			}
			
			spawnArgs_adjusted_shipdoors.push_back( shipdoors[ i ].GetEntity()->spawnArgs );
			spawnArgs_adjusted_shipdoors.back().SetInt( "idEntity_team", team );
			spawnArgs_adjusted_shipdoors.back().SetInt( "health", shipdoors[ i ].GetEntity()->health );
			spawnArgs_adjusted_shipdoors.back().SetInt( "entity_max_health", shipdoors[ i ].GetEntity()->entity_max_health );
			//spawnArgs_adjusted_shipdoors.back().SetVector( "origin", shipdoors[ i ].GetEntity()->GetPhysics()->GetOrigin() ); // probably not necessary
			//spawnArgs_adjusted_shipdoors.back().SetMatrix( "rotation", shipdoors[ i ].GetEntity()->GetPhysics()->GetAxis() ); // probably not necessary
		}
	}
	
	// remove the ship doors
	for( int i = 0; i < shipdoors.Num(); i++ ) {
		if ( shipdoors[ i ].GetEntity() && shipdoors[ i ].GetEntity()->IsType( idDoor::Type ) ) {
			dynamic_cast<idDoor*>( shipdoors[ i ].GetEntity() )->RemoveDoorGroup();
			//shipdoors[ i ] = NULL; // this is bad for idLists - use .RemoveIndex( i-- ) instead
			//shipdoors.RemoveIndex( i ); // this doesn't work either because it moves all the other entities down one so we will miss some doors
			shipdoors.RemoveIndex( i-- ); // this is good
		}
	}



	// store the idDict of the ship light fixtures' targets
	for( int i = 0; i < shiplights.Num(); i++ ) {
		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
			spawnArgs_adjusted_shiplights_targets.push_back( shiplights[ i ].GetEntity()->targets[ix].GetEntity()->spawnArgs );
			}
		}
	}
	// // remove the ship light fixtures' targets
	for( int i = 0; i < shiplights.Num(); i++ ) {
		for( int ix = 0; ix < shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
			if ( shiplights[ i ].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity() && shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
				dynamic_cast<idLight*>( shiplights[ i ].GetEntity()->targets[ix].GetEntity() )->Event_Remove();
				shiplights[ i ].GetEntity()->targets[ix] = NULL;
			}
		}
	}
	
	// store the idDict of the ship light fixtures
	for( int i = 0; i < shiplights.Num(); i++ ) {
		if ( shiplights[ i ].GetEntity() ) {
			spawnArgs_adjusted_shiplights.push_back( shiplights[ i ].GetEntity()->spawnArgs );
			spawnArgs_adjusted_shiplights.back().SetInt("team", shiplights[ i ].GetEntity()->team);
		}
	}
	// remove the ship light fixtures
	for( int i = 0; i < shiplights.Num(); i++ ) {
		if ( shiplights[ i ].GetEntity() ) {
			shiplights[ i ].GetEntity()->Event_Remove();
			//shiplights[ i ] = NULL; // this is bad for idLists - use .RemoveIndex( i-- ) instead
			//shiplights.RemoveIndex( i ); // this doesn't work either because it moves all the other entities down one so we will miss some doors
			shiplights.RemoveIndex( i-- );
		}
	}


	// etc. - or is that it?

	//DoStuffAfterAllMapEntitiesHaveSpawned()
	//DoStuffAfterPlayerHasSpawned()
}

void sbShip::RespawnShipInteriorEntities() {
	// example:
	//for ( int i = 0; i < MAX_MODULES_ON_SHIPS ; i++ ) {
	//	if ( spawnArgs_adjusted_consoles[i].GetNumKeyVals() > 0 ) {
	//		SpawnShipInteriorAdjustedidDict( spawnArgs_adjusted_consoles[i] );
	//	}
	//}
	//gameLocal.Error("GOT TO THIS POINT\n");	
	// spawn the AI's on board who are not part of crew[MAX_CREW_ON_SHIPS] and link them up to the ship
	AIsOnBoard.clear(); // probably not necessary
	for ( int i = 0; i < spawnArgs_adjusted_AIsOnBoard.size(); i++ ) {
		idAI* pointer_to_this_ai;
		pointer_to_this_ai = (idAI*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_AIsOnBoard[i] );
		pointer_to_this_ai->ShipOnBoard = this;
		AIsOnBoard.push_back(pointer_to_this_ai);
		if ( idStr::Icmp(spawnArgs_adjusted_AIsOnBoard[i].GetString( "ParentShip", "" ), name.c_str()) ) { // returns true if they are not equal
			sbShip* possible_parent_ship = dynamic_cast<sbShip*>( gameLocal.FindEntity( spawnArgs.GetString("ParentShip",NULL) ));
			if ( possible_parent_ship ) {
				pointer_to_this_ai->ParentShip = possible_parent_ship;
			}
		}
	}

	// spawn the crew that is on board and link them up to the ship
	for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( spawnArgs_adjusted_crew[i].GetNumKeyVals() > 0 ) {
			crew[i] = (idAI*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_crew[i] );
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
				gameLocal.GetLocalPlayer()->SyncUpPlayerShipNameCVars();
			}
			AIsOnBoard.push_back(crew[i]); // add the ai to the list of AI's on board this ship
			crew[i]->ShipOnBoard = this;
			crew[i]->ParentShip = this;
		}
	}

	// spawn the room nodes and link them up to the ship
	for ( int i = 0; i < MAX_ROOMS_ON_SHIPS ; i++ ) {
		if ( spawnArgs_adjusted_room_node[i].GetNumKeyVals() > 0 ) {
			room_node[i] = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_room_node[i] );
		}
	}
	// spawn the consoles and modules and link them up with each other and to the ship
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS ; i++ ) {
		declManager->FindMaterial( "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
		declManager->FindMaterial( "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" )->SetSort(SS_GUI); // BOYETTE NOTE: we only need to do this once on map spawn.
		
		if ( spawnArgs_adjusted_consoles[i].GetNumKeyVals() > 0 ) {
			consoles[i] = (sbConsole*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_consoles[i] );
			consoles[i]->ParentShip = this;
			consoles[i]->SetRenderEntityGui0String( "module_icon_background", "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" );
			consoles[i]->SetRenderEntityGui0String( "module_icon_background_circle_only", "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" );
			consoles[i]->SetRenderEntityGui0String( "ship_icon_background", ShipStargridIcon );
			
			if ( consoles[i] && spawnArgs_adjusted_consoles_ControlledModule[i].GetNumKeyVals() > 0 ) {
				consoles[i]->ControlledModule = (sbModule*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_consoles_ControlledModule[i] );
				consoles[i]->ControlledModule->ParentConsole = consoles[i];
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_background", "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleEfficiencyBackground.tga" );
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "module_icon_background_circle_only", "guis/assets/steve_captain_display/ModuleIcons/" + module_description_upper[i] + "ModuleEfficiencyBackgroundCircleOnly.tga" );
				consoles[i]->ControlledModule->SetRenderEntityGui0String( "ship_icon_background", ShipStargridIcon );
			}
		}
	}
	
	// spawn the captain chair and link it up to the ship
	if ( spawnArgs_adjusted_CaptainChair.GetNumKeyVals() > 0 ) {
		CaptainChair = (sbCaptainChair*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_CaptainChair );
		CaptainChair->ParentShip = this;
		UpdateGuisOnCaptainChair();
	}
	// spawn the ready room captain chair and link it up to the ship
	if ( spawnArgs_adjusted_ReadyRoomCaptainChair.GetNumKeyVals() > 0 ) {
		ReadyRoomCaptainChair = (sbCaptainChair*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_ReadyRoomCaptainChair );
		ReadyRoomCaptainChair->ParentShip = this;
		if ( ReadyRoomCaptainChair && ReadyRoomCaptainChair->has_console_display ) {
			ReadyRoomCaptainChair->PopulateCaptainLaptop();
		}
	}
	// spawn the shield entity and link it up to the ship
	if ( spawnArgs_adjusted_ShieldEntity.GetNumKeyVals() > 0 ) {
		ShieldEntity = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_ShieldEntity );
		ShieldEntity->SetShaderParm(10,0.0f); // The shield entity is invisible to begin.
		ShieldEntity->GetPhysics()->GetClipModel()->SetOwner( this ); // This is so the torpedo doesn't hit the inside of the shield.
		ShieldEntity->GetPhysics()->SetOrigin( this->GetPhysics()->GetOrigin() );
		ShieldEntity->GetPhysics()->SetAxis( this->GetPhysics()->GetAxis() );
		ShieldEntity->Bind( this, true );
	}
	if ( !consoles[SHIELDSMODULEID] ) {
		if ( ShieldEntity ) {
			ShieldEntity->Event_Remove();
			ShieldEntity = NULL;

			shields_raised = false;
			shieldStrength = 0;
			max_shieldStrength = 1; // to make sure we can transport throught he shield - the shield strength should always be less than the transport limit.
			shieldStrength_copy = 0;
			//gameLocal.Printf( "NOTE: " + name + " does not have a shields console. shield entity is removed. shieldstrength = 0. shields_raised = false." );
		}
	}
	// spawn the transporter bounds and link it up to the ship
	if ( spawnArgs_adjusted_TransporterBounds.GetNumKeyVals() > 0 ) {
		TransporterBounds = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_TransporterBounds );
	}
	// spawn the transporter pad and link it up to the ship
	if ( spawnArgs_adjusted_TransporterPad.GetNumKeyVals() > 0 ) {
		TransporterPad = (sbTransporterPad*)SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_TransporterPad );
		TransporterPad->ParentShip = this;
		UpdateGuisOnTransporterPad();
	}
	// spawn the transporter particle entity spawn marker and link it up to the ship
	if ( spawnArgs_adjusted_TransporterParticleEntitySpawnMarker.GetNumKeyVals() > 0 ) {
		TransporterParticleEntitySpawnMarker = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_TransporterParticleEntitySpawnMarker );
	}
	// spawn the ship diagram display node and link it up to the ship
	if ( spawnArgs_adjusted_ShipDiagramDisplayNode.GetNumKeyVals() > 0 ) {
		ShipDiagramDisplayNode = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_ShipDiagramDisplayNode );
	}
	// spawn the view screen entity and link it up to the ship
	if ( spawnArgs_adjusted_ViewScreenEntity.GetNumKeyVals() > 0 ) {
		ViewScreenEntity = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_ViewScreenEntity );
	}
	if ( ViewScreenEntity ) {
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == this ) {
			ViewScreenEntity->Show();
		} else {
			ViewScreenEntity->Hide();
		}
	}
	// spawn the partners of the ship doors and (it will find them in its own spawn function)
	for( int i = 0; i < spawnArgs_adjusted_shipdoors_partners.size(); i++ ) {
		if ( spawnArgs_adjusted_shipdoors_partners[i].GetNumKeyVals() > 0 ) {
			idEntity* pointer_to_spawned_ent = NULL; // this pointer probably is not necessary
			pointer_to_spawned_ent = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_shipdoors_partners[i] );
		}
	}

	// spawn the ship doors (they will find their partners in their own spawn function)
	for( int i = 0; i < spawnArgs_adjusted_shipdoors.size(); i++ ) {
		if ( spawnArgs_adjusted_shipdoors[i].GetNumKeyVals() > 0 ) {
			shipdoors.Alloc();
			shipdoors[ i ] = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_shipdoors[i] );
		}
	}

	// spawn the ship light fixture targets (the ship light fixtures will find their target lights in their own spawn function)
	for( int i = 0; i < spawnArgs_adjusted_shiplights_targets.size(); i++ ) {
		if ( spawnArgs_adjusted_shiplights_targets[i].GetNumKeyVals() > 0 ) {
			idEntity* pointer_to_spawned_ent = NULL; // this pointer probably is not necessary
			pointer_to_spawned_ent = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_shiplights_targets[i] );
		}
	}
	// spawn the ship light fixtures (they will find their target lights in their own spawn function)
	for( int i = 0; i < spawnArgs_adjusted_shiplights.size(); i++ ) {
		if ( spawnArgs_adjusted_shiplights[i].GetNumKeyVals() > 0 ) {
			shiplights.Alloc();
			shiplights[ i ] = SpawnShipInteriorEntityFromAdjustedidDict( spawnArgs_adjusted_shiplights[i] );
		}
	}
	
	// etc. - or is that it?


	// BEGIN CLEAR ALL THE ADJUSTED DICTS - TO SAVE ON MEMORY AND SO IT IS RESET IN CASE THE SHIP EVER DESPAWNS/RESPAWNS AGAIN.
	spawnArgs_adjusted_ShipDiagramDisplayNode.Clear();

	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i ++ ) {
		spawnArgs_adjusted_crew[i].Clear();
	}

	for ( int i = 0; i < spawnArgs_adjusted_AIsOnBoard.size(); i ++ ) {
		spawnArgs_adjusted_AIsOnBoard.clear();
	}

	for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i ++ ) {
		spawnArgs_adjusted_room_node[i].Clear();
	}

	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i ++ ) {
		spawnArgs_adjusted_consoles[i].Clear();
		spawnArgs_adjusted_consoles_ControlledModule[i].Clear();
	}

	spawnArgs_adjusted_CaptainChair.Clear();

	spawnArgs_adjusted_ReadyRoomCaptainChair.Clear();

	spawnArgs_adjusted_ShieldEntity.Clear();

	spawnArgs_adjusted_TransporterBounds.Clear();
	spawnArgs_adjusted_TransporterPad.Clear();
	spawnArgs_adjusted_TransporterParticleEntitySpawnMarker.Clear();

	spawnArgs_adjusted_ViewScreenEntity.Clear();

	for ( int i = 0; i < spawnArgs_adjusted_shipdoors.size(); i ++ ) {
		spawnArgs_adjusted_shipdoors.clear();
	}
	for ( int i = 0; i < spawnArgs_adjusted_shipdoors_partners.size(); i ++ ) {
		spawnArgs_adjusted_shipdoors_partners.clear();
	}

	for ( int i = 0; i < spawnArgs_adjusted_shiplights.size(); i ++ ) {
		spawnArgs_adjusted_shiplights.clear();
	}
	for ( int i = 0; i < spawnArgs_adjusted_shiplights_targets.size(); i ++ ) {
		spawnArgs_adjusted_shiplights_targets.clear();
	}
	// END CLEAR ALL THE ADJUSTED DICTS - TO SAVE ON MEMORY AND SO IT IS RESET IN CASE THE SHIP EVER DESPAWNS/RESPAWNS AGAIN.

	if ( gameLocal.GetLocalPlayer() ) {
		gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
	}
	//DoStuffAfterAllMapEntitiesHaveSpawned();
	//DoStuffAfterPlayerHasSpawned();
}
idEntity* sbShip::SpawnShipInteriorEntityFromAdjustedidDict( const idDict &dict_to_spawn ) {
	idEntity*	pointer_to_spawned_entity;
	gameLocal.SpawnEntityDef( dict_to_spawn, &pointer_to_spawned_entity, false );			
	return pointer_to_spawned_entity;
}
// SHIP DESPAWNING/RESPAWNING SYSTEM END

void sbShip::Event_ActivateShipAutoPower( void ) {
	if ( !modules_power_automanage_on ) {
		modules_power_automanage_on = true;
		AutoManageModulePowerlevels();
	}
	modules_power_automanage_on = true;
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == this ) {
		gameLocal.GetLocalPlayer()->UpdateModulesPowerQueueOnCaptainGui();
	}
}
void sbShip::Event_AttemptWarpTowardsShipPlayerIsOnBoard( void ) {
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		std::vector<idVec2>	warp_options;
		idVec2				best_warp_option;
		float				best_distance = idMath::INFINITY;

		int option_x;
		int option_y;
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx - 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony + 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		/* // BOYETTE NOTE: we don't allow diagonal warping anymore:
		option_x = stargridpositionx + 1;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}
		*/
		option_x = stargridpositionx;
		option_y = stargridpositiony - 1;
		if ( option_x > 0 && option_x <= MAX_STARGRID_X_POSITIONS && option_y > 0 && option_y <= MAX_STARGRID_Y_POSITIONS ) {
			warp_options.push_back(idVec2(option_x,option_y));
		}

		for ( int i = 0; i < warp_options.size() ; i++ ) {
			float distance = (float)idMath::Sqrt( (gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx - warp_options[i].x)*(gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx - warp_options[i].x) + (gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony - warp_options[i].y)*(gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony - warp_options[i].y) );
			if ( distance < best_distance ) {
				best_distance = distance;
				best_warp_option.x = warp_options[i].x;
				best_warp_option.y = warp_options[i].y;
			}
		}

		is_attempting_warp = AttemptWarp(best_warp_option.x,best_warp_option.y);
	}
}

void sbShip::Event_DetermineDefensiveActionsForSpareCrewOnBoard( void ) {
	idAI*		hostile_ai_on_board = NULL;
	for ( int i = 0; i < AIsOnBoard.size() ; i++ ) {
		if ( AIsOnBoard[i] && AIsOnBoard[i]->team != team ) {
			hostile_ai_on_board = AIsOnBoard[i];
		}
	}

	// FIND THE MODULE THAT THE INTRUDERS ARE NEAREST BEGIN
	float		bestDist;
	float		dist;
	idVec3		delta;
	float		best_distance = idMath::INFINITY;
	idEntity*	module_in_danger = NULL;
	if ( hostile_ai_on_board ) {
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( consoles[i] && consoles[i]->ControlledModule ) {
				delta = hostile_ai_on_board->GetPhysics()->GetOrigin() - consoles[i]->ControlledModule->GetPhysics()->GetOrigin();
				dist = delta.LengthSqr();
				if ( ( dist < bestDist ) && consoles[i]->ControlledModule->health < consoles[i]->ControlledModule->entity_max_health  ) {
					bestDist = dist;
					module_in_danger = consoles[i]->ControlledModule;
				}
			}
		}
	}
	// FIND THE MODULE THAT THE INTRUDERS ARE NEAREST END

	std::vector<idAI*> spare_crew;
	int modules_exists_counter = 0;

	for ( int i = MAX_MODULES_ON_SHIPS-1; i >= 0; i-- ) {
		if ( spare_crew.size() >= max_spare_crew_size || modules_exists_counter >= max_modules_to_take_spare_crew_from ) {
			break;
		}
		if ( consoles[ModulesPowerQueue[i]] && consoles[ModulesPowerQueue[i]]->ControlledModule ) {
			modules_exists_counter++;
			if ( crew[ModulesPowerQueue[i]] ) {
				if ( crew[ModulesPowerQueue[i]]->ShipOnBoard && crew[ModulesPowerQueue[i]]->ShipOnBoard == this ) {
					spare_crew.push_back(crew[ModulesPowerQueue[i]]);
				}
			}
		} else if ( crew[ModulesPowerQueue[i]] ) {
			spare_crew.push_back(crew[ModulesPowerQueue[i]]);
		}
	}

	if ( module_in_danger ) {
		for ( int i = 0; i < spare_crew.size(); i++ ) {
			if ( spare_crew[i] && spare_crew[i]->ShipOnBoard && spare_crew[i]->ShipOnBoard == this ) {
				SetSelectedCrewMember(spare_crew[i]);
				GiveCrewMoveCommand( module_in_danger, spare_crew[i]->ShipOnBoard ); // BOYETTE NOTE: if the hostile entity transports off the ship after we have set it as the entity commaned to move to. There will be a short period of time where the AI will try to move to an entity on another ship. This might be a major problem, it might not. Need to test it.
				ClearCrewMemberSelection();
			}
		}
	} else {
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && TargetEntityInSpace && TargetEntityInSpace == gameLocal.GetLocalPlayer()->PlayerShip ) { // BOYETTE NOTE: added 06 02 2016
			// BOYETTE NOTE: this is a bit of hackery to prevent enemy ships from crowding around damaged modules and fixing them too quickly against the playership.
		} else {
			for ( int i = 0; i < spare_crew.size(); i++ ) {
				if ( spare_crew[i] && spare_crew[i]->ShipOnBoard && spare_crew[i]->ShipOnBoard == this ) {
					spare_crew[i]->crew_auto_mode_activated = true;
				}
			}
		}
	}
}

void sbShip::Event_AllModulesAreFullyRepaired( void ) {
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if ( consoles[i] && consoles[i]->ControlledModule && consoles[i]->ControlledModule->health < consoles[i]->ControlledModule->entity_max_health ) {
			idThread::ReturnInt( false );
			return;
		}
	}
	idThread::ReturnInt( true );
	return;
}

void sbShip::Event_ActivateAutoModeForAllCrewAboardShip( void ) {
	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this && i != CAPTAINCREWID ) {
			crew[i]->crew_auto_mode_activated = true;
		}
	}
	battlestations = false;
}
void sbShip::Event_DeactivateAutoModeForAllCrewAboardShip( void ) {
	for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
		if ( crew[i] && crew[i]->ShipOnBoard && crew[i]->ShipOnBoard == this && i != CAPTAINCREWID ) {
			crew[i]->crew_auto_mode_activated = false;
			crew[i]->player_follow_mode_activated = false;
		}
	}
}
void sbShip::Event_SendCrewToStations( void ) {
	if ( !battlestations ) {
		SendCrewToBattlestations();
		battlestations = true;
	}
}

void sbShip::PhenomenonDealShipToShipDamage( sbShip* PhenomenonTarget, int phenomenon_module_id, int phenomenon_damage_amount ) {
	if ( !ship_destruction_sequence_initiated ) {

		int shieldDamageToDeal = 0;
		int hullDamageToDeal = 0;

		gameLocal.Printf( "PhenomenonDealShipToShipDamage\n");

		if ( PhenomenonTarget && PhenomenonTarget->consoles[phenomenon_module_id] && PhenomenonTarget->consoles[phenomenon_module_id]->ControlledModule ) {
			shieldDamageToDeal = idMath::Rint((float)phenomenon_damage_amount * ((float)PhenomenonTarget->shieldStrength / (float)PhenomenonTarget->max_shieldStrength));
			if ( phenomenon_should_do_ship_damage ) {
				PhenomenonTarget->shieldStrength = PhenomenonTarget->shieldStrength - shieldDamageToDeal;
			}
			hullDamageToDeal = phenomenon_damage_amount - shieldDamageToDeal;
			if ( phenomenon_should_do_ship_damage ) {
				PhenomenonTarget->hullStrength = PhenomenonTarget->hullStrength - hullDamageToDeal;
			}
			if ( phenomenon_should_do_ship_damage ) {
				PhenomenonTarget->consoles[phenomenon_module_id]->ControlledModule->RecieveShipToShipDamage( this, hullDamageToDeal ); // has to go through shields first
			} else {
				PhenomenonTarget->consoles[phenomenon_module_id]->ControlledModule->RecieveShipToShipDamage( this, hullDamageToDeal + shieldDamageToDeal ); // always does the full damage amount to the module
			}
			PhenomenonTarget->shieldStrength = idMath::ClampInt(0,PhenomenonTarget->max_shieldStrength,PhenomenonTarget->shieldStrength);
			if ( PhenomenonTarget->shields_raised ) {
				PhenomenonTarget->shieldStrength_copy = PhenomenonTarget->shieldStrength;
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->guiOverlay && gameLocal.GetLocalPlayer()->guiOverlay == gameLocal.GetLocalPlayer()->HailGui ) {
				gameLocal.GetLocalPlayer()->UpdateHailGui();
			}
			if ( !PhenomenonTarget->is_derelict ) {
				PhenomenonTarget->GoToRedAlert();
				EndNeutralityWithTeam( PhenomenonTarget->team );
				PhenomenonTarget->EndNeutralityWithTeam( team );
			}

			PhenomenonTarget->FlashShieldDamageFX(500);
			PhenomenonTarget->was_just_damaged = true;
			PhenomenonTarget->SetShaderParm( 10, 1.0f -( (float)PhenomenonTarget->hullStrength / (float)PhenomenonTarget->max_hullStrength ) ); // set the damage decal alpha

			// SHIP TO SHIP DAMAGE SOUND EFFECT BEGIN
			if ( phenomenon_should_do_ship_damage ) {
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && PhenomenonTarget == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
					float hullDamage_to_totalDamage_ratio = (float)hullDamageToDeal / (float)(shieldDamageToDeal + hullDamageToDeal);
					if ( hullDamage_to_totalDamage_ratio >= 0.00f && hullDamage_to_totalDamage_ratio < 0.25f ) {
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_light" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
					}
					if ( hullDamage_to_totalDamage_ratio >= 0.25f && hullDamage_to_totalDamage_ratio < 0.50f  ) {
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_medium" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
					}
					if ( hullDamage_to_totalDamage_ratio >= 0.50f && hullDamage_to_totalDamage_ratio < 0.75f  ) {
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_heavy" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
					}
					if ( hullDamage_to_totalDamage_ratio >= 0.75f && hullDamage_to_totalDamage_ratio <= 1.00f  ) {
						gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_critical" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
					}
				}
				// There is no sound in space but this might be a good idea even so just to emphasize that action is taking place. I guess theoretically some kind of energy wave or particles could travel from the explosion to the player ShipOnBoard.
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && PhenomenonTarget != gameLocal.GetLocalPlayer()->ShipOnBoard && PhenomenonTarget->stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && PhenomenonTarget->stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_in_space" ), SND_CHANNEL_ANY, 0, false, NULL ); // this should come from a def arg.
				}
			} else {
				// or maybe don't do any sound
				if ( gameLocal.GetLocalPlayer()->ShipOnBoard && PhenomenonTarget == gameLocal.GetLocalPlayer()->ShipOnBoard ) {
					gameLocal.GetLocalPlayer()->StartSoundShader( declManager->FindSound( "spaceship_weapons_impact_in_space" ), SND_CHANNEL_ANY, 0, false, NULL ); // so we have a really light sound
				}
			}
			// SHIP TO SHIP DAMAGE SOUND EFFECT END

			// SHIP TO SHIP DAMAGE NOTIFICATION BEGIN
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard && stargridpositionx == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx && stargridpositiony == gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
				idStr this_ship_text_color;
				if ( !HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^1"; }
				if ( HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && team != gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^8"; }
				if ( team == gameLocal.GetLocalPlayer()->team ) { this_ship_text_color = "^4"; }

				idStr target_ship_text_color;
				if ( !PhenomenonTarget->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && PhenomenonTarget->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^1"; }
				if ( PhenomenonTarget->HasNeutralityWithTeam(gameLocal.GetLocalPlayer()->team) && PhenomenonTarget->team != gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^8"; }
				if ( PhenomenonTarget->team == gameLocal.GetLocalPlayer()->team ) { target_ship_text_color = "^4"; }

				if ( gameLocal.GetLocalPlayer()->PlayerShip && PhenomenonTarget == gameLocal.GetLocalPlayer()->PlayerShip ) {
					target_ship_text_color = "^2";
				}
				if ( phenomenon_should_do_ship_damage ) {
					gameLocal.GetLocalPlayer()->UpdateNotificationList( this_ship_text_color + original_name + "^0" " does " "^5" + idStr(hullDamageToDeal) + "^0" " hull dmg " "^5" + idStr(shieldDamageToDeal) + "^0" " shield dmg to " + target_ship_text_color + PhenomenonTarget->original_name );
				} else {
					if ( consoles[phenomenon_module_id] && consoles[phenomenon_module_id]->ControlledModule ) {
						gameLocal.GetLocalPlayer()->UpdateNotificationList( this_ship_text_color + original_name + "^0" " does " "^5" + idStr(hullDamageToDeal + shieldDamageToDeal) + "^0" " dmg to " + target_ship_text_color + PhenomenonTarget->original_name + "\n^1 " + module_description[phenomenon_module_ids_to_disable[0]] + " ^1module" );
					}
				}
			}
			// SHIP TO SHIP DAMAGE NOTIFICATION END

			// CAPTAIN DISPLAY INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH BEGIN
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace == PhenomenonTarget ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( PhenomenonTarget->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("TargetShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && PhenomenonTarget && gameLocal.GetLocalPlayer()->PlayerShip == PhenomenonTarget ) {
				if ( gameLocal.GetLocalPlayer()->CaptainGui ) {
					if ( PhenomenonTarget->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent("PlayerShipIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && PhenomenonTarget && gameLocal.GetLocalPlayer()->ShipOnBoard == PhenomenonTarget ) {
				if ( gameLocal.GetLocalPlayer()->hud ) {
					if ( PhenomenonTarget->shieldStrength > 0 ) {
						gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("ShipOnBoardIncomingAttackShipDiagramShieldFlash");
					}
				}
			}
			// CAPTAIN DISPLAY INCOMING ATTACK SHIP DIAGRAM SHIELD FLASH END
			if ( phenomenon_show_damage_or_disable_beam ) {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					beam->SetOrigin( GetPhysics()->GetOrigin() );
					beamTarget->SetOrigin( PhenomenonTarget->GetPhysics()->GetOrigin() );
					beam->Show();
					beamTarget->Show();
				}
				CancelEvents( &EV_UpdateBeamVisibility );
				PostEventMS( &EV_UpdateBeamVisibility, 350 ); // the beam will show up for a second and then disappear - this will make it look like a phaser. // this sets ship_is_firing_weapons to false so we can keep it even if not at the the same stargrid position
			}

			if ( PhenomenonTarget->hullStrength <= 0 && !PhenomenonTarget->ship_destruction_sequence_initiated ) {
				// BOYETTE NOTE TODO: need to put notification here - "the ship destroyed the ship".
				CeaseFiringWeaponsAndTorpedos();
				PhenomenonTarget->BeginShipDestructionSequence();
				PhenomenonTarget = NULL;
			}
		}
	}
}

idAI* sbShip::SpawnEntityDefOnShip( const char* spawn_entity_def_name, idEntity* entity_to_spawn_on, int team_spawnarg_to_set ) {
	idDict		dict;

	dict.Set( "classname", spawn_entity_def_name );

	if ( entity_to_spawn_on && entity_to_spawn_on->GetPhysics() ) {
		dict.Set( "origin", entity_to_spawn_on->GetPhysics()->GetOrigin().ToString() );
	}
	dict.SetInt( "team", team_spawnarg_to_set );
	idEntity*	ent;
	gameLocal.SpawnEntityDef( dict, &ent, false );

	if ( ent->IsType( idAI::Type ) ) {
		idAI* spawned_entity = NULL;
		spawned_entity = ( idAI * )ent;
		AIsOnBoard.push_back(spawned_entity); // add the ai to the list of AI's on board this ship
		spawned_entity->ShipOnBoard = this;
		return spawned_entity;
	} else {
		return NULL;
	}

	// BOYETTE NOTE TODO: NEED TO FIND A WAY TO MOVE THE crew DOWN UNTIL IT IS ON THE GROUND. ALSO NEED TO MAKE SURE IT DOES NOT SPAWN INSIDE OF ANOTHER ENTITY.
	//idVec3		pos;
	//pos =  TransporterBounds->GetPhysics()->GetOrigin();
	//crew[i]->GetFloorPos(64.0f, pos );

	//crew[i]->Init();
	//crew[i]->BecomeActive( TH_THINK );
}

void sbShip::Event_DoPhenomenonActions( void ) {

	if ( phenomenon_should_damage_modules ) {
		int random_ship = gameLocal.random.RandomInt(ships_at_my_stargrid_position.size());
		for ( int i = 0; i < phenomenon_module_ids_to_damage.size(); i++ ) {
			PhenomenonDealShipToShipDamage(ships_at_my_stargrid_position[random_ship],phenomenon_module_ids_to_damage[i],phenomenon_module_damage_amount);
		}
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
			bool damaged_some_module = false;
			idStr modules_text;
			for ( int i = 0; i < phenomenon_module_ids_to_damage.size(); i++ ) {
				if ( ships_at_my_stargrid_position[random_ship] && ships_at_my_stargrid_position[random_ship]->consoles[phenomenon_module_ids_to_damage[i]] && ships_at_my_stargrid_position[random_ship]->consoles[phenomenon_module_ids_to_damage[i]]->ControlledModule ) {
					damaged_some_module = true;
					if ( i == 0  ) {
						modules_text = modules_text + module_description_upper[phenomenon_module_ids_to_damage[i]];
					} else {
						modules_text = modules_text + ", " + module_description_upper[phenomenon_module_ids_to_damage[i]];
					}
				}
			}
			if ( damaged_some_module ) {
				if ( ships_at_my_stargrid_position[random_ship] && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + "\n ^1is ^1damaging ^1the ^1following ^1in ^1local ^1space:\n^1" + modules_text );
				}
			}
		}
	}

	if ( phenomenon_should_damage_random_module ) {
		int random_ship = gameLocal.random.RandomInt(ships_at_my_stargrid_position.size());
		std::vector<int> random_module_ids;
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if ( ships_at_my_stargrid_position[random_ship] && ships_at_my_stargrid_position[random_ship]->consoles[i] && ships_at_my_stargrid_position[random_ship]->consoles[i]->ControlledModule ) {
				random_module_ids.push_back(i);
			}
		}
		if ( random_module_ids.size() > 0 ) {
			int random_module_id = random_module_ids[gameLocal.random.RandomInt(random_module_ids.size())];
			PhenomenonDealShipToShipDamage(ships_at_my_stargrid_position[random_ship],random_module_id,phenomenon_module_damage_amount);
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + "\n ^1is ^1damaging ^1random ^1modules ^1of ^1entities ^1in ^1local ^1space\n" );
			}
		}
	}
	
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->PlayerShip->stargridpositiony == stargridpositiony ) {
		sbShip* player_ship = gameLocal.GetLocalPlayer()->PlayerShip;
		if ( phenomenon_should_disable_modules ) { // ONE TIME THING
			bool successfully_did_phenomenon_action = false;
			for ( int i = 0; i < phenomenon_module_ids_to_disable.size(); i++ ) {
				if ( player_ship->consoles[phenomenon_module_ids_to_disable[i]] && player_ship->consoles[phenomenon_module_ids_to_disable[i]]->ControlledModule ) {
					sbModule* module_to_damage = player_ship->consoles[phenomenon_module_ids_to_disable[i]]->ControlledModule;
					//PhenomenonDealShipToShipDamage(player_ship,phenomenon_module_ids_to_disable[i],module_to_damage->entity_max_health);
					module_to_damage->RecieveShipToShipDamage(this,module_to_damage->entity_max_health);
					successfully_did_phenomenon_action = true;
					if ( phenomenon_show_damage_or_disable_beam ) {
						if ( player_ship->stargridpositionx == stargridpositionx && player_ship->stargridpositiony == stargridpositiony ) {
							beam->SetOrigin( GetPhysics()->GetOrigin() );
							beamTarget->SetOrigin( player_ship->GetPhysics()->GetOrigin() );
							beam->Show();
							beamTarget->Show();
						}
						CancelEvents( &EV_UpdateBeamVisibility );
						PostEventMS( &EV_UpdateBeamVisibility, 350 ); // the beam will show up for a second and then disappear - this will make it look like a phaser. // this sets ship_is_firing_weapons to false so we can keep it even if not at the the same stargrid position
					}
				}
			}
			if ( successfully_did_phenomenon_action ) {
				if ( phenomenon_module_ids_to_disable.size() == 1 ) {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1disabled ^1the \n^1" + module_description[phenomenon_module_ids_to_disable[0]] + " ^1module" );
				} else {
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1disabled ^1some ^1modules" );
				}
			}
			phenomenon_should_disable_modules = false; // this was a one time event
			spawnArgs.SetBool("phenomenon_should_disable_modules", "0"); // this was a one time event
		}
		if ( phenomenon_should_disable_random_module ) { // ONE TIME THING
			bool successfully_did_phenomenon_action = false;
			std::vector<int> random_module_ids;
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( player_ship->consoles[i] && player_ship->consoles[i]->ControlledModule ) {
					random_module_ids.push_back(i);
				}
			}
			int random_module_id = random_module_ids[gameLocal.random.RandomInt(random_module_ids.size())];
			if ( player_ship->consoles[random_module_id] && player_ship->consoles[random_module_id]->ControlledModule ) {
				sbModule* module_to_damage = player_ship->consoles[random_module_id]->ControlledModule;
				//PhenomenonDealShipToShipDamage(player_ship,random_module_id,module_to_damage->entity_max_health);
				module_to_damage->RecieveShipToShipDamage(this,module_to_damage->entity_max_health);
				successfully_did_phenomenon_action = true;
				if ( phenomenon_show_damage_or_disable_beam ) {
					if ( player_ship->stargridpositionx == stargridpositionx && player_ship->stargridpositiony == stargridpositiony ) {
						beam->SetOrigin( GetPhysics()->GetOrigin() );
						beamTarget->SetOrigin( player_ship->GetPhysics()->GetOrigin() );
						beam->Show();
						beamTarget->Show();
					}
					CancelEvents( &EV_UpdateBeamVisibility );
					PostEventMS( &EV_UpdateBeamVisibility, 350 ); // the beam will show up for a second and then disappear - this will make it look like a phaser. // this sets ship_is_firing_weapons to false so we can keep it even if not at the the same stargrid position
				}
			}
			if ( successfully_did_phenomenon_action ) {
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1disabled ^1the \n^1" + module_description[random_module_id] + " ^1module" );
			}
			phenomenon_should_disable_random_module = false; // this was a one time event
			spawnArgs.SetBool("phenomenon_should_disable_random_module", "0"); // this was a one time event
		}
		if ( phenomenon_should_set_oxygen_level ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1caused ^1a ^1drop ^1in ^1the ^1life ^1support ^1oxygen." );
			player_ship->current_oxygen_level = phenomenon_oxygen_level_to_set;
			player_ship->current_oxygen_level = idMath::ClampInt(0,100,player_ship->current_oxygen_level);
			phenomenon_should_set_oxygen_level = false; // this was a one time event
			spawnArgs.SetBool("phenomenon_should_set_oxygen_level", "0"); // this was a one time event
		}
		if ( phenomenon_should_spawn_entity_def_on_playership ) { // ONE TIME THING
			std::vector<int> random_room_node_ids;
			for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
				if ( player_ship->room_node[i] ) {
					random_room_node_ids.push_back(i);
				}
			}
			int random_team = gameLocal.random.RandomInt(idRandom::MAX_RAND);
			random_team = idMath::ClampInt(gameLocal.GetLocalPlayer()->team,idRandom::MAX_RAND,random_team); // BOYETTE NOTE: this should at least make sure we ar enot the player team. Hopefully it is not one of the players neutral teams.
			std::vector<idAI*> spawn_entities;
			for ( int i = 0; i < phenomenon_number_of_entity_defs_to_spawn && random_room_node_ids.size() > 0; i++ ) {
				int random_int = gameLocal.random.RandomInt(random_room_node_ids.size());
				int random_room_node_id = random_room_node_ids[random_int];
				spawn_entities.push_back(player_ship->SpawnEntityDefOnShip(phenomenon_entity_def_to_spawn,player_ship->room_node[random_room_node_id],random_team));
				random_room_node_ids.erase(random_room_node_ids.begin() + random_int);
			}
			// added 06 27 2016 BEGIN
			std::vector<int> random_crew_ids;
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( player_ship->crew[i] ) {
					random_crew_ids.push_back(i);
				}
			}
			for ( int i = 0; i < spawn_entities.size(); i++ ) {
				int random_crew_id;
				if ( gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == gameLocal.GetLocalPlayer()->PlayerShip ) {
					random_crew_id = gameLocal.random.RandomInt(random_crew_ids.size() + 1);
				} else {
					random_crew_id = gameLocal.random.RandomInt(random_crew_ids.size());
				}
				if ( random_crew_id > random_crew_ids.size() ) {
					if ( spawn_entities[i] && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == gameLocal.GetLocalPlayer()->PlayerShip ) {
						spawn_entities[i]->Activate( this );
						//spawn_entities[i]->MoveToEntity( gameLocal.GetLocalPlayer() ); // not necessary
						spawn_entities[i]->SetEnemy(gameLocal.GetLocalPlayer()); // this seems to work fine
					}
				} else {
					if ( spawn_entities[i] && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
						spawn_entities[i]->Activate( this );
						//spawn_entities[i]->MoveToEntity( player_ship->crew[random_crew_ids[gameLocal.random.RandomInt(random_crew_ids.size())]] ); // not necessary
						spawn_entities[i]->SetEnemy(player_ship->crew[random_crew_ids[gameLocal.random.RandomInt(random_crew_ids.size())]]); // this seems to work fine
					}
				}
			}
			// added 06 27 2016 END
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1caused ^1some ^1creatures ^1to ^1appear ^1on ^1your ^1ship." );
			phenomenon_should_spawn_entity_def_on_playership = false; // this was a one time event
			spawnArgs.SetBool("phenomenon_should_spawn_entity_def_on_playership", "0"); // this was a one time event
		}
		if ( phenomenon_should_change_random_playership_crewmember_team ) { // ONE TIME THING
			std::vector<int> random_crewmember_ids;
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( player_ship->crew[i] ) {
					random_crewmember_ids.push_back(i);
				}
			}
			int random_crew_id = random_crewmember_ids[gameLocal.random.RandomInt(random_crewmember_ids.size())];
			//remove from your crew and change team and refresh all the proper guis
			if ( player_ship->crew[random_crew_id] ) {
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->DialogueAI && gameLocal.GetLocalPlayer()->DialogueAI == player_ship->crew[random_crew_id] ) {
					gameLocal.GetLocalPlayer()->CloseOverlayAIDialogeGui();
					gameLocal.GetLocalPlayer()->DialogueAI = NULL;
				}
				if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->grabbed_crew_member && gameLocal.GetLocalPlayer()->grabbed_crew_member == player_ship->crew[random_crew_id] ) {
					gameLocal.GetLocalPlayer()->grabbed_crew_member = NULL;
				}
				if ( player_ship->crew[random_crew_id]->ParentShip && player_ship->crew[random_crew_id]->ParentShip == player_ship ) {
					player_ship->SelectedCrewMembers.erase(std::remove(player_ship->SelectedCrewMembers.begin(), player_ship->SelectedCrewMembers.end(), player_ship->crew[random_crew_id]), player_ship->SelectedCrewMembers.end()); // remove this ai from the list of AI's that the player_ship could have selected
					
					int new_team = gameLocal.random.RandomInt( idRandom::MAX_RAND );
					player_ship->crew[random_crew_id]->team = new_team; player_ship->crew[random_crew_id]->spawnArgs.SetInt("team",new_team);
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1have ^1affected ^1the ^1sanity ^1of ^1one ^1your ^1crew: \n^1" + player_ship->crew[random_crew_id]->spawnArgs.GetString( "npc_name", "" ) + "^1." );
					
					if ( player_ship->crew[random_crew_id] ) {
						if ( player_ship->room_node[random_crew_id] ) { 
							player_ship->crew[random_crew_id]->SetEntityToMoveToByCommand( player_ship->room_node[random_crew_id] );
						}
						player_ship->crew[random_crew_id]->crew_auto_mode_activated = true;
						player_ship->crew[random_crew_id]->handling_emergency_oxygen_situation = false;
						player_ship->crew[random_crew_id] = NULL;
					}
				}
			}
			phenomenon_should_change_random_playership_crewmember_team = false; // this was a one time event
			spawnArgs.SetBool("phenomenon_should_change_random_playership_crewmember_team", "0"); // this was a one time event
		}
	}
	if ( phenomenon_should_set_ship_shields_to_zero ) {
		bool neutralized_a_ships_shields_significantly = false;
		for ( int i = 0; i < ships_at_my_stargrid_position.size(); i ++ ) {
			if ( ships_at_my_stargrid_position[i] ) {
				if ( ships_at_my_stargrid_position[i]->shieldStrength > ships_at_my_stargrid_position[i]->max_shieldStrength / 5 ) { // if it neutralizes more than 20% of a ship's shields - it is arbitrarily considered significant.
					neutralized_a_ships_shields_significantly = true;
				}
				ships_at_my_stargrid_position[i]->shieldStrength_copy = 0;
				ships_at_my_stargrid_position[i]->shieldStrength = 0;
			}
		}
		if ( neutralized_a_ships_shields_significantly ) {
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1neutralize ^1defense ^1shields." );
		}
	}
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == stargridpositiony ) {
		if ( phenomenon_should_make_everything_go_slowmo && !gameLocal.GetLocalPlayer()->ShipOnBoard->is_attempting_warp ) {
			if ( g_enableSlowmo.GetBool() == false ) {
				g_enableSlowmo.SetBool( true );
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1be ^1affecting ^1the ^1space ^1time ^1continuum." );
			}
		}
		if ( phenomenon_should_toggle_slowmo_on_and_off && !gameLocal.GetLocalPlayer()->ShipOnBoard->is_attempting_warp ) {
			if ( g_enableSlowmo.GetBool() ) {
				g_enableSlowmo.SetBool( false );
			} else {
				if ( gameLocal.random.RandomFloat() < 0.20f ) { // goes slowmo 20% of the time - this is arbitrary now but should feel right
					g_enableSlowmo.SetBool( true );
					gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The ^1entity \n^1" + original_name + " ^1seems ^1to ^1be ^1affecting ^1the ^1space ^1time ^1continuum ^1sporadically." );
				}
			}
		}
	}

	idThread::ReturnInt( phenomenon_should_ignore_the_rest_of_the_ship_ai_loop );
}

void sbShip::Event_ShipShouldFlee( void ) {
	if ( consoles[ENGINESMODULEID] && consoles[ENGINESMODULEID]->ControlledModule ) {
		if ( TargetEntityInSpace && TargetEntityInSpace->team != team && !HasNeutralityWithShip(TargetEntityInSpace) && (float)hullStrength/(float)max_hullStrength < flee_hullstrength_percentage ) {
			Event_SetMainGoal( SHIP_AI_FLEE_FROM, TargetEntityInSpace );
			idThread::ReturnInt( true );
			return;
		} else {
			if ( (float)hullStrength/(float)max_hullStrength < flee_hullstrength_percentage ) {
				for (int i = 0; i < ships_at_my_stargrid_position.size(); i++ ) {
					if ( ships_at_my_stargrid_position[i] && ships_at_my_stargrid_position[i]->team != team && !HasNeutralityWithShip(ships_at_my_stargrid_position[i]) ) {
						Event_SetMainGoal( SHIP_AI_FLEE_FROM, ships_at_my_stargrid_position[i] );
						idThread::ReturnInt( true );
						return;
					}
				}
			} else {
				idThread::ReturnInt( false );
				return;
			}
			idThread::ReturnInt( false );
			return;
		}
	} else {
		idThread::ReturnInt( false );
		return;
	}
}
// SHIP AI EVENTS END


// torpedo test
idProjectile *sbShip::CreateProjectile( const idVec3 &pos, const idVec3 &dir ) {
	idEntity *ent;
	const char *clsname;

	projectile = NULL; // BOYETTE NOTE IMPORTANT: Added 02/25/2016 - so that if the old projectile is still flying around because we missed the last shot it will not cause problems.

	if ( !projectileDef ) {
		projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_spaceship_torpedo", "projectile_spaceship_torpedo_default"), false );
	}

	if ( !projectile.GetEntity() ) {
		//gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
		gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
		if ( !ent ) {
			clsname = projectileDef->GetString( "classname" );
			gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
		}
		
		if ( !ent->IsType( idProjectile::Type ) ) {
			clsname = ent->GetClassname();
			gameLocal.Error( "'%s' is not an idProjectile", clsname );
		}
		projectile = ( idProjectile * )ent; // boyette note: this is c-style cast that is converting(or designating) an idEntity into an idProjectile. A dynamic_cast or possibly a static_cast would probably work fine here instead but I haven't tried it yet.
	}

	projectile.GetEntity()->Create( this, pos, dir );

	return projectile.GetEntity();
}
	// boyette map entites link up end



CLASS_DECLARATION(sbShip, sbPlayerShip)

END_CLASS



/*
void sbPlayerShip::SetWarpDestination(int x, int y) {
	stargriddestinationx = x;
	stargriddestinationy = y;
}

void sbPlayerShip::SetWarpPosition(int x, int y) {
	stargridpositionx = x;
	stargridpositiony = y;
}

*/


CLASS_DECLARATION(sbShip, sbEnemyShip)


END_CLASS



CLASS_DECLARATION(sbShip, sbFriendlyShip)


END_CLASS


CLASS_DECLARATION(sbShip, sbStationarySpaceEntity)

END_CLASS

sbStationarySpaceEntity::sbStationarySpaceEntity()
: sbShip()
{
	track_on_stargrid = false;
	can_be_deconstructed = false;
};

sbStationarySpaceEntity::~sbStationarySpaceEntity(){
};

void sbStationarySpaceEntity::Spawn() {
	sbShip::Spawn();

	// stationary space entity dialogue system(this gui has somewhat different dialogue options than the default one for sbShip).
	hail_dialogue_gui_file = spawnArgs.GetString("hail_dialogue_gui_file","guis/steve_space_command/hail_guis/default_hail_no_response.gui");

	// currently we put the planet on the stargrid background manually - simply for aesthetic reasons
	track_on_stargrid = spawnArgs.GetBool( "track_on_stargrid", "0" );

	// ships can almost always be deconstructed - but spacestations/phenomenon/planets almost never
	can_be_deconstructed = spawnArgs.GetBool( "can_be_deconstructed", "0" );
}

//void sbStationarySpaceEntity::Save( idSaveGame *savefile ) const {
//	sbShip::Save(savefile);
//}

//void sbStationarySpaceEntity::Restore( idRestoreGame *savefile ) {
//	sbShip::Restore(savefile);
//}

void sbStationarySpaceEntity::Think( void ) {
	sbShip::Think();
	//RunPhysics();
	//Present();
}

bool sbStationarySpaceEntity::AttemptWarp(int stargriddestx,int stargriddesty) {
	return false;
}
void sbStationarySpaceEntity::EngageWarp(int stargriddestx,int stargriddesty) {
}

void sbStationarySpaceEntity::ClaimUnnoccupiedSkyPortalEntity() {
}

void sbStationarySpaceEntity::ClaimUnnoccupiedPlayerSkyPortalEntity() {
}



CLASS_DECLARATION(sbStationarySpaceEntity, sbPlanetaryBody)


END_CLASS



CLASS_DECLARATION(sbStationarySpaceEntity, sbPhenomenon)


END_CLASS




/////////////////////// CAPTAIN CHAIR /////////////////////////

CLASS_DECLARATION(idEntity, sbCaptainChair)

END_CLASS

sbCaptainChair::sbCaptainChair() {
	ParentShip = NULL;

	min_view_angles	= ang_zero;
	max_view_angles	= ang_zero;

	in_confirmation_process_of_player_take_command = false;

	log_page_number = 1;
	encyclopedia_page_number = 1;
	has_console_display = false;
	ship_directive_overridden = false;

	SeatedEntity = NULL;
}
void sbCaptainChair::Spawn() {
	min_view_angles.yaw = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().yaw + spawnArgs.GetFloat("seated_yaw_min", "-40.0");
	max_view_angles.yaw = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().yaw + spawnArgs.GetFloat("seated_yaw_max", "40.0");
	min_view_angles.pitch = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().pitch + spawnArgs.GetFloat("seated_pitch_min", "-89.0");
	max_view_angles.pitch = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().pitch + spawnArgs.GetFloat("seated_pitch_max", "40.0");

	min_view_angles.Normalize180();
	max_view_angles.Normalize180();


	has_console_display = spawnArgs.GetBool("has_console_display", "0");
	ship_directive_overridden = spawnArgs.GetBool("ship_directive_overridden", "0");
	if ( has_console_display ) {
		PopulateCaptainLaptop(); // BOYETTE NOTE TODO: This happens again after the player spawns - so we could probably get rid of this one. Just in case we get rid of that one - we will keep this one.
	}
	//spawnArgs.GetFloat("seated_yaw_min", "-40.0");
	//spawnArgs.GetFloat("seated_yaw_max", "40.0");
	//spawnArgs.GetFloat("seated_pitch_min", "-89.0");
	//spawnArgs.GetFloat("seated_pitch_max", "20.0");
}

void sbCaptainChair::Save( idSaveGame *savefile ) const {
	// BOYETTE SAVE BEGIN
	savefile->WriteObject( ParentShip );

	savefile->WriteAngles( min_view_angles );
	savefile->WriteAngles( max_view_angles );

	savefile->WriteBool( has_console_display );
	savefile->WriteInt( log_page_number );
	savefile->WriteInt( encyclopedia_page_number );
	savefile->WriteBool( ship_directive_overridden );

	savefile->WriteBool( in_confirmation_process_of_player_take_command );

	SeatedEntity.Save( savefile );
	// BOYETTE SAVE END
}

void sbCaptainChair::Restore( idRestoreGame *savefile ) {
	// BOYETTE RESTORE BEGIN
	savefile->ReadObject( reinterpret_cast<idClass *&>( ParentShip ) );

	savefile->ReadAngles( min_view_angles );
	savefile->ReadAngles( max_view_angles );

	savefile->ReadBool( has_console_display );
	savefile->ReadInt( log_page_number );
	savefile->ReadInt( encyclopedia_page_number );
	savefile->ReadBool( ship_directive_overridden );

	savefile->ReadBool( in_confirmation_process_of_player_take_command );

	SeatedEntity.Restore( savefile );
	// BOYETTE RESTORE END
}

bool sbCaptainChair::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "question_player_take_command_of_parent_ship" ) == 0 ) {
		if ( ParentShip && ParentShip->CanBeTakenCommandOfByPlayer() ) {
			in_confirmation_process_of_player_take_command = true;
			//ParentShip->UpdateGuisOnCaptainChair(); // happens in think now
			return true;
		}
	}

	if ( token.Icmp( "confirm_player_take_command_of_parent_ship" ) == 0 ) {
		if ( ParentShip && ParentShip->CanBeTakenCommandOfByPlayer() ) {
			gameLocal.Printf( "YOU TAKE COMMAND OF THE ENEMY SHIP. TEST\n" );
			if ( ParentShip ) {
				gameLocal.Printf( ParentShip->name + "TEST\n" );
			}
			if ( !SeatedEntity.GetEntity() ) {
				gameLocal.GetLocalPlayer()->SetOrigin( GetPhysics()->GetOrigin() + ( spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().ToForward() * spawnArgs.GetFloat("sit_down_distance","40") ) );
				gameLocal.GetLocalPlayer()->Bind( this , true );
				if ( spawnArgs.GetBool("player_can_sit_in_this","1") ) {
					cvarSystem->SetCVarFloat( "pm_crouchviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
					cvarSystem->SetCVarFloat( "pm_normalviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
				}
				gameLocal.GetLocalPlayer()->CaptainChairSeatedIn = this;
				// BOYETTE NOTE BEGIN: added 10/08/2016
				if ( !has_console_display ) {
					gameLocal.GetLocalPlayer()->SetViewAngles( GetPhysics()->GetAxis().ToAngles() );
					gameLocal.GetLocalPlayer()->SetAngles( GetPhysics()->GetAxis().ToAngles() );
					gameLocal.GetLocalPlayer()->SetOverlayCaptainGui();
				}
				// BOYETTE NOTE END
				gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("Press -" + idStr(common->KeysFromBinding("togglemenu")) + "- to exit the chair");
				//gameLocal.GetLocalPlayer()->LowerWeapon();// pops up again after the captain chair gui focus is lost - might need to make an event.
				gameLocal.GetLocalPlayer()->DisableWeapon();
				gameLocal.GetLocalPlayer()->disable_crosshairs = true;
				// BOYETTE NOTE TODO: still need to hide the weapons on the player
				SeatedEntity = gameLocal.GetLocalPlayer();
				if ( ParentShip ) {
					ParentShip->RecalculateAllModuleEfficiencies();
				}
				if ( has_console_display ) {
					PopulateCaptainLaptop();
				}
				if ( renderEntity.gui[0] ) {
					renderEntity.gui[0]->Activate(true,gameLocal.time);
				}
			}
			gameLocal.GetLocalPlayer()->TakeCommandOfShip( ParentShip );
			in_confirmation_process_of_player_take_command = false;
			return true;
		}
	}

	if ( token.Icmp( "cancel_player_take_command_of_parent_ship" ) == 0 ) {
		in_confirmation_process_of_player_take_command = false;
		//ParentShip->UpdateGuisOnCaptainChair(); // happens in think now
		return true;
	}

	if ( token.Icmp( "player_sit_in_captain_chair" ) == 0 ) {
		if ( !SeatedEntity.GetEntity() ) {
			gameLocal.GetLocalPlayer()->SetOrigin( GetPhysics()->GetOrigin() + ( spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().ToForward() * spawnArgs.GetFloat("sit_down_distance","40") ) );
			gameLocal.GetLocalPlayer()->Bind( this , true );
			if ( spawnArgs.GetBool("player_can_sit_in_this","1") ) {
				cvarSystem->SetCVarFloat( "pm_crouchviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
				cvarSystem->SetCVarFloat( "pm_normalviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
			}
			gameLocal.GetLocalPlayer()->CaptainChairSeatedIn = this;
			// BOYETTE NOTE BEGIN: added 10/08/2016
			if ( !has_console_display ) {
				gameLocal.GetLocalPlayer()->SetViewAngles( GetPhysics()->GetAxis().ToAngles() );
				gameLocal.GetLocalPlayer()->SetAngles( GetPhysics()->GetAxis().ToAngles() );
				gameLocal.GetLocalPlayer()->SetOverlayCaptainGui();
			}
			// BOYETTE NOTE END
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("Press -" + idStr(common->KeysFromBinding("togglemenu")) + "- to exit the chair");
			//gameLocal.GetLocalPlayer()->LowerWeapon();// pops up again after the captain chair gui focus is lost - might need to make an event.
			gameLocal.GetLocalPlayer()->DisableWeapon();
			gameLocal.GetLocalPlayer()->disable_crosshairs = true;
			// BOYETTE NOTE TODO: still need to hide the weapons on the player
			SeatedEntity = gameLocal.GetLocalPlayer();
			if ( ParentShip ) {
				ParentShip->RecalculateAllModuleEfficiencies();

				if ( !ParentShip->red_alert ) {
					ParentShip->DimShipLightsOff();
				}
			}
			if ( has_console_display ) {
				PopulateCaptainLaptop();
			}
			if ( renderEntity.gui[0] ) {
				renderEntity.gui[0]->Activate(true,gameLocal.time);
			}
		}
		return true;
	}

	if ( has_console_display ) {

		if ( token.Icmp( "click_shiplogList" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
				renderEntity.gui[0]->SetStateString( "selected_ship_log_name", spawnArgs.GetString(va("ship_log_key_%i",selected_item_num),"") );
				renderEntity.gui[0]->SetStateString( "selected_ship_log_text", spawnArgs.GetString(va("ship_log_value_%i",selected_item_num),"") );
			}
			return true;
		}
		if ( token.Icmp( "click_encycList" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_name", spawnArgs.GetString(va("ship_encyclopedia_key_%i",selected_item_num),"") );
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_text", spawnArgs.GetString(va("ship_encyclopedia_value_%i",selected_item_num),"") );
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_image", spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),"") );
				declManager->FindMaterial(spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),""))->SetSort(SS_GUI);
			}
			return true;
		}

		if ( token.Icmp( "increase_log_page_number" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				log_page_number++;
				log_page_number = idMath::ClampInt(1,6,log_page_number);
				renderEntity.gui[0]->SetStateInt( "log_page_number", log_page_number );
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
			}
			return true;
		}
		if ( token.Icmp( "decrease_log_page_number" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				log_page_number--;
				log_page_number = idMath::ClampInt(1,6,log_page_number);
				renderEntity.gui[0]->SetStateInt( "log_page_number", log_page_number );
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
			}
			return true;
		}
		if ( token.Icmp( "increase_encyclopedia_page_number" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				encyclopedia_page_number++;
				encyclopedia_page_number = idMath::ClampInt(1,10,encyclopedia_page_number); // BOYETTE NOTE TODO: have an int for max ecyclopedia and log page numbers (right now it is 6 for logs and 10 for encyclopedia entries
				renderEntity.gui[0]->SetStateInt( "encyclopedia_page_number", encyclopedia_page_number );
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
			}
			return true;
		}
		if ( token.Icmp( "decrease_encyclopedia_page_number" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				encyclopedia_page_number--;
				encyclopedia_page_number = idMath::ClampInt(1,10,encyclopedia_page_number);
				renderEntity.gui[0]->SetStateInt( "encyclopedia_page_number", encyclopedia_page_number );
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
			}
			return true;
		}
		if ( token.Icmp( "reset_page_numbers" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				log_page_number = 1;
				encyclopedia_page_number = 1;
				renderEntity.gui[0]->SetStateInt( "log_page_number", log_page_number );
				renderEntity.gui[0]->SetStateInt( "encyclopedia_page_number", encyclopedia_page_number );
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
			}
			return true;
		}

		if ( token.Icmp( "increase_log_list_item_selection" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
				if ( selected_item_num < MAX_SHIP_LOG_ITEMS ) {
					renderEntity.gui[0]->ScrollListGUIToBottom("shiplogList");
					selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
					renderEntity.gui[0]->SetStateString( "selected_ship_log_name", spawnArgs.GetString(va("ship_log_key_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_ship_log_text", spawnArgs.GetString(va("ship_log_value_%i",selected_item_num),"") );
				}
			}
			return true;
		}
		if ( token.Icmp( "decrease_log_list_item_selection" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
				if ( selected_item_num > 0 ) {
					renderEntity.gui[0]->ScrollListGUIToTop("shiplogList");
					selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
					renderEntity.gui[0]->SetStateString( "selected_ship_log_name", spawnArgs.GetString(va("ship_log_key_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_ship_log_text", spawnArgs.GetString(va("ship_log_value_%i",selected_item_num),"") );
				}
			}
			return true;
		}
		if ( token.Icmp( "increase_encyclopedia_list_item_selection" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
				if ( selected_item_num < MAX_SHIP_ENCYCLOPEDIA_ITEMS ) {
					renderEntity.gui[0]->ScrollListGUIToBottom("encycList");
					selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_name", spawnArgs.GetString(va("ship_encyclopedia_key_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_text", spawnArgs.GetString(va("ship_encyclopedia_value_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_image", spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),"") );
					declManager->FindMaterial(spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),""))->SetSort(SS_GUI);
				}
			}
			return true;
		}
		if ( token.Icmp( "decrease_encyclopedia_list_item_selection" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				int selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
				if ( selected_item_num > 0 ) {
					renderEntity.gui[0]->ScrollListGUIToTop("encycList");
					selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_name", spawnArgs.GetString(va("ship_encyclopedia_key_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_text", spawnArgs.GetString(va("ship_encyclopedia_value_%i",selected_item_num),"") );
					renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_image", spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),"") );
					declManager->FindMaterial(spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),""))->SetSort(SS_GUI);
				}
			}
			return true;
		}
		if ( token.Icmp( "reset_list_selections" ) == 0 ) {
			if ( renderEntity.gui[0] ) {
				renderEntity.gui[0]->SetStateInt("shiplogList_sel_0",0);
				renderEntity.gui[0]->SetStateString( "selected_ship_log_name", spawnArgs.GetString(va("ship_log_key_%i",0),"") );
				renderEntity.gui[0]->SetStateString( "selected_ship_log_text", spawnArgs.GetString(va("ship_log_value_%i",0),"") );

				renderEntity.gui[0]->SetStateInt("encycList_sel_0",0);
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_name", spawnArgs.GetString(va("ship_encyclopedia_key_%i",0),"") );
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_text", spawnArgs.GetString(va("ship_encyclopedia_value_%i",0),"") );
				renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_image", spawnArgs.GetString(va("ship_encyclopedia_image_%i",0),"") );
				declManager->FindMaterial(spawnArgs.GetString(va("ship_encyclopedia_image_%i",0),""))->SetSort(SS_GUI);
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );	
			}
			return true;
		}

		if ( token.Icmp( "override_ship_directive" ) == 0 ) {
			if ( ParentShip && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == ParentShip ) {
				ship_directive_overridden = true;
				renderEntity.gui[0]->SetStateBool( "ship_directive_overridden", ship_directive_overridden );
				if ( gameLocal.StoryRepositoryEntity ) {
					renderEntity.gui[0]->SetStateString( "ship_directive", gameLocal.StoryRepositoryEntity->spawnArgs.GetString("map_mission","map mission not defined on StoryRepositoryEntity") );
				} else {
					renderEntity.gui[0]->SetStateString( "ship_directive", "there is no StoryRepositoryEntity on this map" );
				}
			}
			PopulateCaptainLaptop();
			//ParentShip->UpdateGuisOnCaptainChair(); // happens in think now
			return true;
		}
	}

	src->UnreadToken( &token );
	return false;
}

void sbCaptainChair::PopulateCaptainLaptop() {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt("shiplogList_sel_0",0);
		renderEntity.gui[0]->SetStateInt("encycList_sel_0",0);

		int selected_item_num = renderEntity.gui[0]->State().GetInt( "shiplogList_sel_0", "-1");
		renderEntity.gui[0]->SetStateString( "selected_ship_log_name", spawnArgs.GetString(va("ship_log_key_%i",selected_item_num),"") );
		renderEntity.gui[0]->SetStateString( "selected_ship_log_text", spawnArgs.GetString(va("ship_log_value_%i",selected_item_num),"") );
		selected_item_num = renderEntity.gui[0]->State().GetInt( "encycList_sel_0", "-1");
		renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_name", spawnArgs.GetString(va("ship_encyclopedia_key_%i",selected_item_num),"") );
		renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_text", spawnArgs.GetString(va("ship_encyclopedia_value_%i",selected_item_num),"") );

		renderEntity.gui[0]->SetStateString( "selected_item_encyclopedia_image", spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),"") );
		// SetSort(SS_GUI) - this is only used by the gui system to force sorting order on images referenced from tga's instead of materials. this is done this way as there are 2000 tgas the guis use
		declManager->FindMaterial(spawnArgs.GetString(va("ship_encyclopedia_image_%i",selected_item_num),""))->SetSort(SS_GUI);

		//log_page_number = 1;
		//encyclopedia_page_number = 1;
		renderEntity.gui[0]->SetStateInt( "log_page_number", log_page_number );
		renderEntity.gui[0]->SetStateInt( "encyclopedia_page_number", encyclopedia_page_number );

		if ( gameLocal.StoryRepositoryEntity ) {
			renderEntity.gui[0]->SetStateString( "map_stardate", gameLocal.StoryRepositoryEntity->spawnArgs.GetString("map_stardate","Stardate: 42254.7") );
		} else {
			renderEntity.gui[0]->SetStateString( "map_stardate", "Stardate: 42254.7" );
		}

		if ( ship_directive_overridden ) {
			if ( gameLocal.StoryRepositoryEntity ) {
				renderEntity.gui[0]->SetStateString( "ship_directive", gameLocal.StoryRepositoryEntity->spawnArgs.GetString("map_mission","map mission not defined on StoryRepositoryEntity") ); // BOYETTE NOTE DONE: we should have an override button to override the ship_mission with the map_mission
			} else {
				renderEntity.gui[0]->SetStateString( "ship_directive", "there is no StoryRepositoryEntity on this map" ); // BOYETTE NOTE DONE: we should have an override button to override the ship_mission with the map_mission
			}
		} else {
			renderEntity.gui[0]->SetStateString( "ship_directive", spawnArgs.GetString("ship_directive","Continuing Mission: To explore.") ); // BOYETTE NOTE DONE: we should have an override button to override the ship_mission with the map_mission
		}

		if ( ParentShip && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip == ParentShip ) {
			renderEntity.gui[0]->SetStateBool( "ship_directive_overridden", ship_directive_overridden );
		} else {
			renderEntity.gui[0]->SetStateBool( "ship_directive_overridden", true );
		}

		int i;

		// clear the lists
		for (i = 0; i < MAX_SHIP_LOG_ITEMS; i++) {
			renderEntity.gui[0]->DeleteStateVar( va( "shiplogList_item_%i", i) );
		}
		for (i = 0; i < MAX_SHIP_ENCYCLOPEDIA_ITEMS; i++) {
			renderEntity.gui[0]->DeleteStateVar( va( "encycList_item_%i", i) );
		}

		// populate the list
		for ( i = 0; i < MAX_SHIP_LOG_ITEMS ; i++ ) {
			renderEntity.gui[0]->SetStateString( va("shiplogList_item_%i", i ), spawnArgs.GetString( va("ship_log_key_%i", i ), "") );
		}
		for ( i = 0; i < MAX_SHIP_ENCYCLOPEDIA_ITEMS ; i++ ) {
			renderEntity.gui[0]->SetStateString( va("encycList_item_%i", i ), spawnArgs.GetString( va("ship_encyclopedia_key_%i", i ), "") );
		}
		renderEntity.gui[0]->StateChanged( gameLocal.time, true );	
	}
	return;	
}

void sbCaptainChair::ReleasePlayerCaptain() {
	gameLocal.GetLocalPlayer()->CloseOverlayCaptainGui();
	gameLocal.GetLocalPlayer()->Unbind();
	// gameLocal.GetLocalPlayer()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0,40,0)); // this will just move it 40 units forward but will not take into account the rotation of the chair.
	gameLocal.GetLocalPlayer()->SetOrigin(GetPhysics()->GetOrigin() + ( spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().ToForward() * spawnArgs.GetFloat("stand_up_distance","40") ) ); // PERFECT - if 40 is not enough we can always increase and if necessary make a spawnarg so it can vary.
	cvarSystem->SetCVarFloat( "pm_crouchviewheight", 32 );
	cvarSystem->SetCVarFloat( "pm_normalviewheight", 68 );
	gameLocal.GetLocalPlayer()->CaptainChairSeatedIn = NULL;
	gameLocal.GetLocalPlayer()->disable_crosshairs = false;
	gameLocal.GetLocalPlayer()->EnableWeapon();
	gameLocal.GetLocalPlayer()->RaiseWeapon();
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->Activate(false,gameLocal.time + 1000);
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->Activate(false,gameLocal.time);
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->Activate(false,gameLocal.time);
	}
	SeatedEntity = NULL;
	if ( ParentShip ) {
		ParentShip->RecalculateAllModuleEfficiencies();
		// - commented this out 10/07/2016 - re-enabled it because it was too dark then
		if ( !ParentShip->red_alert ) {
			ParentShip->TurnShipLightsOn();
		}
		//
	}
}

void sbCaptainChair::SetRenderEntityGuisStrings( const char* varName, const char* value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateString( varName , value );
	}
}

void sbCaptainChair::SetRenderEntityGuisBools( const char* varName, bool value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateBool( varName , value );
	}
}

void sbCaptainChair::SetRenderEntityGuisInts( const char* varName, int value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateInt( varName , value );
	}
}

void sbCaptainChair::SetRenderEntityGuisFloats( const char* varName, float value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateFloat( varName , value );
	}
}

void sbCaptainChair::HandleNamedEventOnGuis( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->HandleNamedEvent( eventName );
	}
}

void sbCaptainChair::DoStuffAfterAllMapEntitiesHaveSpawned() {

}


/////////////////////// TRANSPORTER PAD /////////////////////////
CLASS_DECLARATION(idEntity, sbTransporterPad)

END_CLASS

sbTransporterPad::sbTransporterPad() {
	ParentShip = NULL;
}
void sbTransporterPad::Spawn() {

}

void sbTransporterPad::Save( idSaveGame *savefile ) const {
// BOYETTE SAVE BEGIN
savefile->WriteObject( ParentShip );
// BOYETTE SAVE END
}

void sbTransporterPad::Restore( idRestoreGame *savefile ) {
// BOYETTE RESTORE BEGIN
savefile->ReadObject( reinterpret_cast<idClass *&>( ParentShip ) );
// BOYETTE RESTORE END
}

bool sbTransporterPad::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "player_take_command_of_parent_ship" ) == 0 ) {
		gameLocal.Printf( "YOU TAKE COMMAND OF THE ENEMY SHIP. TEST\n" );
		return true;
	}

	if ( token.Icmp( "initiate_transport_to_the_targetship_of_parentship" ) == 0 ) {
		if ( ParentShip && ParentShip->TransporterBounds && ParentShip->TargetEntityInSpace && ParentShip->TargetEntityInSpace->TransporterBounds ) {
			if ( ParentShip->consoles[COMPUTERMODULEID] && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount >= ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount && ParentShip->TargetEntityInSpace->shieldStrength >= ( ParentShip->TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters * ParentShip->TargetEntityInSpace->max_shieldStrength ) && ParentShip->TargetEntityInSpace->team != ParentShip->team ) {
				HandleNamedEventOnGuis("TransportAttemptedButTargetShipShieldsAreTooHigh");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			} else if ( ParentShip->consoles[COMPUTERMODULEID] && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount < ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount ) {
				HandleNamedEventOnGuis("TransportAttemptedButNotSequenced");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			} else {
				HandleNamedEventOnGuis("TransportInitiatedSuccessfully");
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
			ParentShip->BeginTransporterSequence();
			return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
		}
	}

	src->UnreadToken( &token );
	return false;
}

void sbTransporterPad::SetRenderEntityGuisStrings( const char* varName, const char* value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateString( varName , value );
	}
}

void sbTransporterPad::SetRenderEntityGuisBools( const char* varName, bool value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateBool( varName , value );
	}
}

void sbTransporterPad::SetRenderEntityGuisInts( const char* varName, int value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateInt( varName , value );
	}
}

void sbTransporterPad::SetRenderEntityGuisFloats( const char* varName, float value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateFloat( varName , value );
	}
}

void sbTransporterPad::HandleNamedEventOnGuis( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->HandleNamedEvent( eventName );
	}
}

void sbTransporterPad::DoStuffAfterAllMapEntitiesHaveSpawned() {

}