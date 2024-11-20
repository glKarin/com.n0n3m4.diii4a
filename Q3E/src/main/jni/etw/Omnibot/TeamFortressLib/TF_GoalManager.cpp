////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF.h"
#include "TF_GoalManager.h"
#include "TF_NavigationFlags.h"
#include "Waypoint.h"

TF_GoalManager::TF_GoalManager()
{
	m_Instance = this;
}

TF_GoalManager::~TF_GoalManager()
{
}

void TF_GoalManager::CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used)
{
	enum { MaxGoals=32 };
	MapGoalDef Definition[MaxGoals];
	int NumDefs = 0;

	//////////////////////////////////////////////////////////////////////////

	if(_wp->IsFlagOn(F_TF_NAV_CAPPOINT))
	{
		/*MapGoalPtr goal(new FlagCapGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","cappoint");
	}
	if(_wp->IsFlagOn(F_TF_NAV_SENTRY))
	{
		/*MapGoalPtr goal(new TF_SentryGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","sentry");
	}
	if(_wp->IsFlagOn(F_TF_NAV_DISPENSER))
	{
		/*MapGoalPtr goal(new TF_DispenserGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","dispenser");
	}
	if(_wp->IsFlagOn(F_TF_NAV_PIPETRAP))
	{
		/*MapGoalPtr goal(new TF_PipeTrapGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","pipetrap");
	}
	if(_wp->IsFlagOn(F_TF_NAV_DETPACK))
	{
		/*MapGoalPtr goal(new TF_DetpackGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","detpack");
	}
	if(_wp->IsFlagOn(F_TF_NAV_ROCKETJUMP))
	{
		/*MapGoalPtr goal(new TF_RocketJumpGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","rocketjump");
	}
	if(_wp->IsFlagOn(F_TF_NAV_CONCJUMP))
	{
		/*MapGoalPtr goal(new TF_ConcJumpGoal);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","concjump");
	}
	if(_wp->IsFlagOn(F_TF_NAV_TELE_ENTER))
	{
		/*MapGoalPtr goal(new TF_Teleporter_Entrance);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","teleentrance");
	}
	if(_wp->IsFlagOn(F_TF_NAV_TELE_EXIT))
	{
		/*MapGoalPtr goal(new TF_Teleporter_Exit);
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","teleexit");
	}
	
	RegisterWaypointGoals(_wp,Definition,NumDefs);
	
	// Allow the base class to process it.
	GoalManager::CheckWaypointForGoal(_wp, _used);
}
