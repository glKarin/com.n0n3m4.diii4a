#include "PrecompRTCW.h"
#include "RTCW_GoalManager.h"
#include "RTCW_NavigationFlags.h"

RTCW_GoalManager::RTCW_GoalManager()
{
	m_Instance = this;
}

RTCW_GoalManager::~RTCW_GoalManager()
{
	Shutdown();
}

void RTCW_GoalManager::CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used)
{
	enum { MaxGoals=8 };

	MapGoalDef Definition[MaxGoals];
	int NumDefs = 0;

	//////////////////////////////////////////////////////////////////////////

	// NumDefs 1
	if(_wp->IsFlagOn(F_RTCW_NAV_ARTSPOT))
	{
		/*MapGoalPtr goal(new RTCW_CallArtyGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","CALLARTILLERY");
	}

	// NumDefs 2
	if(_wp->IsFlagOn(F_RTCW_NAV_ARTYTARGET_S))
	{
		/*MapGoalPtr goal(new RTCW_CallArtyTargetGoal_Static());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","ARTILLERY_S");
	}

	// NumDefs 3
	if(_wp->IsFlagOn(F_RTCW_NAV_ARTYTARGET_D))
	{
		/*MapGoalPtr goal(new RTCW_CallArtyTargetGoal_Dynamic());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","ARTILLERY_D");
	}

	// NumDefs 4
	if(_wp->IsFlagOn(F_RTCW_NAV_CAPPOINT))
	{
		/*MapGoalPtr goal(new FlagCapGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","cappoint");
	}

	// NumDefs 5
	if(_wp->IsFlagOn(F_RTCW_NAV_PANZER))
	{
		/*MapGoalPtr goal(new RTCW_PanzerGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","PANZER");
	}

	// NumDefs 6
	if(_wp->IsFlagOn(F_RTCW_NAV_VENOM))
	{
		/*MapGoalPtr goal(new RTCW_VenomGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","VENOM");
	}

	// NumDefs 7
	if(_wp->IsFlagOn(F_RTCW_NAV_FLAMETHROWER))
	{
		/*MapGoalPtr goal(new RTCW_FlamethrowerGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","FLAME");
	}

	// NOTE: pay attention to MaxGoals / NumDefs!!

	RegisterWaypointGoals(_wp,Definition,NumDefs);
	
	// Allow the base class to process it.
	GoalManager::CheckWaypointForGoal(_wp, _used);
}
