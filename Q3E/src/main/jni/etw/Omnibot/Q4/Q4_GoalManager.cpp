////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompQ4.h"
#include "Q4_GoalManager.h"
#include "Q4_Config.h"
#include "Q4_NavigationFlags.h"

Q4_GoalManager::Q4_GoalManager()
{
	m_Instance = this;
}

Q4_GoalManager::~Q4_GoalManager()
{
	Shutdown();
}

void Q4_GoalManager::CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used)
{
	enum { MaxGoals=32 };

	MapGoalDef Definition[MaxGoals];
	int NumDefs = 0;

	if(_wp->IsFlagOn(F_Q4_NAV_CAPPOINT))
	{
		/*MapGoalPtr goal(new FlagCapGoal());
		newGoals.push_back(goal);*/

		Definition[NumDefs++].Props.SetString("Type","cappoint");
	}

	RegisterWaypointGoals(_wp,Definition,NumDefs);

	// Allow the base class to process it.
	GoalManager::CheckWaypointForGoal(_wp, _used);
}

