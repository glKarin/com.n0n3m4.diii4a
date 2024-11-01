////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETQW_GOALMANAGER_H__
#define __ETQW_GOALMANAGER_H__

#include "GoalManager.h"

// class: ETQW_GoalManager
//		The goal manager is responsible for keeping track of various goals,
//		from flags to capture points. Bots can request goals from the goal 
//		manager and the goal manager can assign goals to the bot based on
//		the needs of the game, and optionally the bot properties
class ETQW_GoalManager : public GoalManager
{
public:

	friend class ETQW_Game;

	//bool AddGoal(const MapGoalDef &goaldef);
	void CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used = BitFlag64());

protected:

	ETQW_GoalManager();
	virtual ~ETQW_GoalManager();
};

#endif
