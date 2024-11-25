#include "PrecompET.h"
#include "ET_GoalManager.h"
#include "ET_NavigationFlags.h"

ET_GoalManager::ET_GoalManager()
{
	m_Instance = this;
}

ET_GoalManager::~ET_GoalManager()
{
	Shutdown();
}

void ET_GoalManager::CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used)
{
	enum { MaxGoals=10 };

	MapGoalDef Definition[MaxGoals];
	int NumDefs = 0;

	//////////////////////////////////////////////////////////////////////////

	// NumDefs 1
	if(_wp->IsFlagOn(F_ET_NAV_MG42SPOT))
	{
		/*MapGoalPtr goal(new ET_MobileMG42Goal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","MOBILEMG42");
	}

	// NumDefs 2
	if(_wp->IsFlagOn(F_ET_NAV_MORTAR))
	{
		/*MapGoalPtr goal(new ET_MobileMortarGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","MOBILEMORTAR");
	}

	// NumDefs 3
	if(_wp->IsFlagOn(F_ET_NAV_ARTSPOT))
	{
		/*MapGoalPtr goal(new ET_CallArtyGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","CALLARTILLERY");
	}

	// NumDefs 4
	if(_wp->IsFlagOn(F_ET_NAV_ARTYTARGET_S))
	{
		/*MapGoalPtr goal(new ET_CallArtyTargetGoal_Static());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","ARTILLERY_S");
	}

	// NumDefs 5
	if(_wp->IsFlagOn(F_ET_NAV_ARTYTARGET_D))
	{
		/*MapGoalPtr goal(new ET_CallArtyTargetGoal_Dynamic());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","ARTILLERY_D");
	}

	// NumDefs 6
	if(_wp->IsFlagOn(F_ET_NAV_MINEAREA))
	{
		/*MapGoalPtr goal(new ET_PlantMineGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","PLANTMINE");
	}

	// NumDefs 7
	if(_wp->IsFlagOn(F_ET_NAV_CAPPOINT))
	{
		/*MapGoalPtr goal(new FlagCapGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","cappoint");
	}

	// NumDefs 8
	if(_wp->IsFlagOn(F_ET_NAV_FLAMETHROWER))
	{
		/*MapGoalPtr goal(new ET_FlamethrowerGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","FLAME");
	}

	// NumDefs 9
	if(_wp->IsFlagOn(F_ET_NAV_PANZER))
	{
		/*MapGoalPtr goal(new ET_PanzerGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","PANZER");
	}

	// NOTE: pay attention to MaxGoals / NumDefs!!

	RegisterWaypointGoals(_wp,Definition,NumDefs);
	
	// Allow the base class to process it.
	GoalManager::CheckWaypointForGoal(_wp, _used);
}
