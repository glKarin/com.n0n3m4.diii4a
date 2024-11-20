#ifndef __ET_GOALMANAGER_H__
#define __ET_GOALMANAGER_H__

#include "GoalManager.h"

// class: ET_GoalManager
//		The goal manager is responsible for keeping track of various goals,
//		from flags to capture points. Bots can request goals from the goal 
//		manager and the goal manager can assign goals to the bot based on
//		the needs of the game, and optionally the bot properties
class ET_GoalManager : public GoalManager
{
public:

	friend class ET_Game;

	void CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used = BitFlag64());
protected:

	ET_GoalManager();
	virtual ~ET_GoalManager();
};

#endif
