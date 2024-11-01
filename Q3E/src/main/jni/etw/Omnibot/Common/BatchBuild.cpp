//#ifndef __linux__

// Unity Build
#include "PrecompCommon.h"
#include "PrecompCommon.cpp"

#include "Client.cpp"
#include "IGame.cpp"
#include "IGameManager.cpp"

#include "BlackBoard.cpp"
#include "BlackBoardItems.cpp"

#include "DebugWindow.cpp"

//#include "BotExports.cpp"
#include "Omni-Bot.cpp"

#include "EventReciever.cpp"

#include "GoalManager.cpp"
#include "MapGoal.cpp"
#include "MapGoalDatabase.cpp"

// misc
#include "BotLoadLibrary.cpp"
#include "CallbackParameters.cpp"
#include "CommandReciever.cpp"
#include "Criteria.cpp"
#include "EngineFuncs.cpp"
#include "FileDownloader.cpp"
#include "InterfaceFuncs.cpp"
#include "Interprocess.cpp"
#include "KeyValueIni.cpp"
#include "Logger.cpp"
#include "NameManager.cpp"
#include "PropertyBinding.cpp"
#include "Regulator.cpp"
#include "Timer.cpp"
#include "Trajectory.cpp"
#include "TriggerManager.cpp"
#include "Utilities.cpp"

#if ENABLE_PATH_PLANNERS
#include "PathPlannerRecast.cpp"
#include "PathPlannerRecastCmds.cpp"
#include "PathPlannerRecastScript.cpp"

#include "PathPlannerFloodFill.cpp"
#include "PathPlannerFloodFillBuilder.cpp"
#include "PathPlannerFloodFillCmds.cpp"
#include "PathPlannerFloodFillScript.cpp"

#include "PathPlannerNavMesh.cpp"
#include "PathPlannerNavMeshBuilder.cpp"
#include "PathPlannerNavMeshCmds.cpp"
#include "PathPlannerNavMeshScript.cpp"
#include "QuadTree.cpp"
#endif

#include "PathPlannerWaypoint.cpp"
#include "PathPlannerWaypointCmds.cpp"
#include "PathPlannerWaypointScript.cpp"
#include "Waypoint.cpp"
#include "WaypointSerializer_V1.cpp"
#include "WaypointSerializer_V2.cpp"
#include "WaypointSerializer_V3.cpp"
#include "WaypointSerializer_V4.cpp"
#include "WaypointSerializer_V5.cpp"
#include "WaypointSerializer_V6.cpp"
#include "WaypointSerializer_V7.cpp"
#include "WaypointSerializer_V9.cpp"

#include "NavigationManager.cpp"
#include "Path.cpp"
#include "PathPlannerBase.cpp"

#include "gmBotLibrary.cpp"
#include "gmMathLibrary.cpp"
#include "gmSystemLibApp.cpp"
#include "gmUtilityLib.cpp"
#include "ScriptManager.cpp"

#include "gmAABB.cpp"
#include "gmBot.cpp"
#include "gmDebugWindow.cpp"
#include "gmGameEntity.cpp"
#include "gmMatrix3.cpp"
#include "gmNamesList.cpp"
#include "gmScriptGoal.cpp"
#include "gmTargetInfo.cpp"
#include "gmTimer.cpp"
#include "gmTriggerInfo.cpp"
#include "gmWeapon.cpp"

#include "StateMachine.cpp"
#include "BotGlobalStates.cpp"
#include "BotBaseStates.cpp"
#include "BotBaseStates_GM.cpp"
#include "BotSteeringSystem.cpp"
#include "BotWeaponSystem.cpp"
#include "FilterAllType.cpp"
#include "FilterClosest.cpp"
#include "FilterSensory.cpp"
#include "ScriptGoal.cpp"

#include "BotSensoryMemory.cpp"
#include "BotTargetingSystem.cpp"
#include "MemoryRecord.cpp"
#include "TargetInfo.cpp"

#include "Weapon.cpp"
#include "WeaponDatabase.cpp"

#include "FileSystem.cpp"

#include "RenderOverlay.cpp"
#include "RenderOverlayGame.cpp"

#include "mdump.cpp"

//#endif
