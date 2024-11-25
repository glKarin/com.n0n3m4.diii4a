////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __Q4_GOALMANAGER_H__
#define __Q4_GOALMANAGER_H__

#include "GoalManager.h"

// class: Q4_GoalManager
//		The goal manager is responsible for keeping track of various goals,
//		from flags to capture points. Bots can request goals from the goal 
//		manager and the goal manager can assign goals to the bot based on
//		the needs of the game, and optionally the bot properties
class Q4_GoalManager : public GoalManager
{
public:
	friend class Q4_Game;

	void CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used = BitFlag64());

protected:

	Q4_GoalManager();
	virtual ~Q4_GoalManager();
};

#endif
