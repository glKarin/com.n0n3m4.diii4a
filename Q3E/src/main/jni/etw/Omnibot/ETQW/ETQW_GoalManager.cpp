////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompETQW.h"
#include "ETQW_GoalManager.h"
#include "ETQW_NavigationFlags.h"

ETQW_GoalManager::ETQW_GoalManager()
{
	m_Instance = this;
}

ETQW_GoalManager::~ETQW_GoalManager()
{
	Shutdown();
}

//bool ETQW_GoalManager::AddGoal(const MapGoalDef &goaldef)
//{
	//MapGoalList newGoals;

	//switch(goaldef.m_GoalType)
	//{
	//case ETQW_GOAL_CONSTRUCTION:
	//	{
	//		MapGoalPtr goal(new ETQW_ConstructionGoal);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_DYNTARGET:
	//	{
	//		MapGoalPtr goal(new ETQW_SetDynamiteGoal);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_DYNAMITE:
	//	{
	//		MapGoalPtr goal(new ETQW_DefuseDynamiteGoal);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_FALLENTEAMMATE:
	//	{
	//		MapGoalPtr goal(new ETQW_FallenTeammateGoal);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_MOVER:
	//	{
	//		MapGoalPtr goal(new ETQW_VehicleGoal);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_MG42MOUNT:
	//	{
	//		MapGoalPtr goal1(new ETQW_MG42Mount);
	//		MapGoalPtr goal2(new ETQW_MG42Repair);
	//		newGoals.push_back(goal1);
	//		newGoals.push_back(goal2);
	//		break;
	//	}
	///*case ETQW_GOAL_SNIPERSPOT:
	//	break;
	//case ETQW_GOAL_MOBILEMG42SPOT:
	//	break;
	//case ETQW_GOAL_MORTARPOSITION:
	//	break;
	//case ETQW_GOAL_SATCHELTARGET:
	//	break;*/
	//case ETQW_GOAL_HEALTH_CAB:
	//	{
	//		MapGoalPtr goal(new ETQW_HealthCabinet);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_AMMO_CAB:
	//	{
	//		MapGoalPtr goal(new ETQW_AmmoCabinet);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//case ETQW_GOAL_CHECKPOINT:
	//	{
	//		MapGoalPtr goal(new ETQW_CheckPoint);
	//		newGoals.push_back(goal);
	//		break;
	//	}
	//default:
	//	GoalManager::AddGoal(goaldef);
	//	return;
	//}

	////////////////////////////////////////////////////////////////////////////
	//for(obuint32 i = 0; i < newGoals.size(); ++i)
	//{
	//	// Initialize the goal.
	//	if(goaldef.m_Team != 0)
	//		newGoals[i]->SetAvailabilityTeams(goaldef.m_Team);
	//	else
	//	{
	//		// For goals with no specific team tags, set them available to everything.
	//		for(int t = ETQW_TEAM_STROGG; t < ETQW_TEAM_MAX; ++t)
	//			newGoals[i]->SetAvailable(t, true);
	//	}

	//	if(goaldef.m_TagName[0])
	//		newGoals[i]->SetTagName(goaldef.m_TagName);
	//	newGoals[i]->SetEntity(goaldef.m_Entity);
	//	newGoals[i]->GenerateName(g_EngineFuncs->IDFromEntity(goaldef.m_Entity));
	//	if(!goaldef.m_UserData.IsNone())
	//		newGoals[i]->ProcessUserData(goaldef.m_UserData);

	//	PropertyMap props;
	//	if(newGoals[i]->InternalInit(props))
	//	{
	//		LOG((Format("Goal Created: %1%, tag: %2%") % newGoals[i]->GetMapGoalName() % newGoals[i]->GetTagName()).str());
	//		GoalManager::AddGoal(newGoals[i]);
	//	}
	//}
//return false;
//}

void ETQW_GoalManager::CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used)
{
	//MapGoalList newGoals;

	////////////////////////////////////////////////////////////////////////////

	//if(_wp->IsFlagOn(F_ETQW_NAV_MG42SPOT))
	//{
	//	MapGoalPtr goal(new ETQW_MobileMG42Goal());
	//	newGoals.push_back(goal);
	//}
	//if(_wp->IsFlagOn(F_ETQW_NAV_MORTAR))
	//{
	//	MapGoalPtr goal(new ETQW_MobileMortarGoal());
	//	newGoals.push_back(goal);
	//}
	//if(_wp->IsFlagOn(F_ETQW_NAV_ARTSPOT))
	//{
	//	MapGoalPtr goal(new ETQW_CallArtyGoal());
	//	newGoals.push_back(goal);
	//}
	//if(_wp->IsFlagOn(F_ETQW_NAV_ARTYTARGETQW_S))
	//{
	//	MapGoalPtr goal(new ETQW_CallArtyTargetGoal_Static());
	//	newGoals.push_back(goal);
	//}
	//if(_wp->IsFlagOn(F_ETQW_NAV_ARTYTARGETQW_D))
	//{
	//	MapGoalPtr goal(new ETQW_CallArtyTargetGoal_Dynamic());
	//	newGoals.push_back(goal);
	//}
	//if(_wp->IsFlagOn(F_ETQW_NAV_MINEAREA))
	//{
	//	MapGoalPtr goal(new ETQW_PlantMineGoal());
	//	newGoals.push_back(goal);
	//}

	// RegisterWaypointGoals(_wp,Definition,NumDefs);
	
	// Allow the base class to process it.
	GoalManager::CheckWaypointForGoal(_wp, _used);
}
