////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_GOALMANAGER_H__
#define __JA_GOALMANAGER_H__

#include "GoalManager.h"

// class: JA_GoalManager
//		The goal manager is responsible for keeping track of various goals,
//		from flags to capture points. Bots can request goals from the goal 
//		manager and the goal manager can assign goals to the bot based on
//		the needs of the game, and optionally the bot properties
class JA_GoalManager : public GoalManager
{
public:
	friend class JA_Game;

	void CheckWaypointForGoal(Waypoint *_wp, BitFlag64 _used = BitFlag64());

protected:

	JA_GoalManager();
	virtual ~JA_GoalManager();
};

#endif
