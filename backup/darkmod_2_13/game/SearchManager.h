/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
/******************************************************************************
*
* DESCRIPTION: CSearchManager is a "search manager" that manages one or more AI
* in their effort to search suspicious events. Rather than have each AI manage
* its own search independently of other searching AI, the search manager assigns
* search areas and roles to all participating AI.
*
*******************************************************************************/

#ifndef __SEARCH_MANAGER_H__
#define __SEARCH_MANAGER_H__

#include "darkmodHidingSpotTree.h"
//#include "Game_local.h"

class idAI;
class idAASLocal;

// Searcher roles that AI can be assigned
typedef enum
{
	E_ROLE_NONE,		// no role
	E_ROLE_SEARCHER,	// actively searches, using a set of hiding spots
	E_ROLE_GUARD,		// stands at a location away from the search, using a set of guard spots
	E_ROLE_OBSERVER		// stands at the perimeter of the search, observing
} smRole_t;

// An Assignment is an assignment given to an AI. The assignment includes a set
// of hiding spots, a matching list of randomized indexes that are used to
// access the hiding spot set, the last spot assigned (represented by the
// index into the randomized indexes).
struct Assignment
{
	idVec3					_origin;		// center of search area, location of alert stimulus, spot to guard
	float					_outerRadius;	// outer radius of search boundary
	idBounds				_limits;		// boundary of the search
	idAI*					_searcher;		// AI assigned
	int						_lastSpotAssigned; // the most recent spot assigned to a searcher; index into _hidingSpotIndexes
	int						_sector;		// which sector you're assigned to (type >=4 searches); 1 = north, 2 = east, 3 = south, 4 = west
	smRole_t				_searcherRole;	// The role of the AI searcher (searcher, guard, observer)
};

// A Search is a set of hiding spots and a collection of AI assignments stemming from a single event.
struct Search
{
	int						_searchID;					// unique id for each search (starts at 0 and increments up)
	int						_eventID;					// the ID of the event this search belongs to
	bool					_isCoopSearch;				// true = cooperative search, 2 active searchers
														// false = swarm search, unlimited searchers
	int						_hidingSpotSearchHandle;	// handle for referencing the search
	idVec3					_origin;					// center of search area, location of alert stimulus
	idBounds				_limits;					// boundary of the search
	idBounds				_exclusion_limits;			// exclusion boundary of the search
	float					_outerRadius;				// outer radius of search boundary
	float					_referenceAlertLevel;		// used when allocating alert levels to AI responding to help requests
	CDarkmodHidingSpotTree	_hidingSpots;				// The hiding spots for this search
	bool					_hidingSpotsReady;			// false = still building the hiding spot list; true = list complete

	std::vector<int>		_hidingSpotIndexes;			// An array of numbers (>= 0) serving as indices into the hiding spot list.
														// i.e. A value of "5" says to obtain the 5th spot in the hiding spot tree.
														// The tree itself can't be simply accessed by index, so _hidingSpotIndexes
														// allows us a simple method of keeping track of the last requested spot.
														// When a good spot is obtained from the hiding spot list, the matching index
														// in _hidingSpotIndexes is set to "-1", indicating that that spot has been
														// used. This is useful when several AI searchers are sharing the list and we
														// want to assign them to unique spots. Prior to 2.02, the contents of
														// _hidingSpotIndexes was randomized, to provide random access to the hiding
														// spots, but starting with 2.03, we'll be accessing them in the order they're
														// provided in the hiding spot tree. The tree is sorted to place higher quality
														// spots at the front, where quality is a guesstimate of where the player
														// might be hiding.

	idList<idVec4>			_guardSpots;				// The spots where guards should be sent [x,y,z,w] where w = yaw angle to face
	bool					_guardSpotsReady;			// false = guard spot list not complete yet; true = list complete
	unsigned int			_assignmentFlags;			// bitwise flags that describe available assignments
	int						_searcherCount;				// Number of searchers (not assignments, because some of those might be deactivated).
														// When this number drops to 0, the search struct is destroyed.
	idList<Assignment>		_assignments;				// A list of assignments for this search. The list grows as AI join the search.
														// The list doesn't shrink when an AI leaves the search; the assignment is simply
														// marked as 'deactivated' by setting assignment._searcher to NULL. The list is
														// cleared (returning memory) when the search struct is destroyed.
};

// search flags
typedef enum
{
	SEARCH_SEARCHER_MILL	= BIT(0),	// searchers mill before searching
	SEARCH_SEARCH			= BIT(1),	// assign searchers
	SEARCH_GUARD_MILL		= BIT(2),	// guards mill before guarding
	SEARCH_GUARD			= BIT(3),	// assign guards
	SEARCH_OBSERVER_MILL	= BIT(4),	// observers mill before observing
	SEARCH_OBSERVE			= BIT(5)	// assign observers
} searchFlags_t;

#define SEARCH_NOTHING			(0)  // no searchers, no guards, no observers
#define SEARCH_EVERYTHING		(SEARCH_SEARCH|SEARCH_GUARD_MILL|SEARCH_GUARD|SEARCH_OBSERVER_MILL|SEARCH_OBSERVE) // guards and observers mill about at first // grayman #4220
#define SEARCH_SUSPICIOUS		(SEARCH_EVERYTHING) // EAlertTypeSuspicious, EAlertTypeSuspiciousVisual
#define SEARCH_ENEMY			(SEARCH_SEARCH|SEARCH_GUARD|SEARCH_OBSERVE) // EAlertTypeEnemy
#define SEARCH_WEAPON			(SEARCH_SEARCH|SEARCH_GUARD_MILL|SEARCH_OBSERVER_MILL) // EAlertTypeWeapon
#define SEARCH_BLINDED			(SEARCH_SEARCH|SEARCH_OBSERVE) // EAlertTypeBlinded
#define SEARCH_DEAD				(SEARCH_SEARCH|SEARCH_GUARD|SEARCH_OBSERVE) // EAlertTypeDeadPerson
#define SEARCH_UNCONSCIOUS		(SEARCH_SEARCH|SEARCH_GUARD|SEARCH_OBSERVE) // EAlertTypeUnconsciousPerson
#define SEARCH_BLOOD			(SEARCH_EVERYTHING) // EAlertTypeBlood
#define SEARCH_LIGHT			(SEARCH_SEARCH) // EAlertTypeLightSource
#define SEARCH_FAILED_KO		(SEARCH_SEARCH) // EAlertTypeFailedKO
#define SEARCH_MISSING			(SEARCH_SEARCH|SEARCH_GUARD_MILL|SEARCH_GUARD|SEARCH_OBSERVER_MILL|SEARCH_OBSERVE) // EAlertTypeMissingItem
#define SEARCH_BROKEN			(SEARCH_SEARCH|SEARCH_GUARD_MILL|SEARCH_OBSERVER_MILL) // EAlertTypeBrokenItem
#define SEARCH_DOOR				(SEARCH_SEARCH) // EAlertTypeDoor
#define SEARCH_SUSPICIOUSITEM	(SEARCH_EVERYTHING) // EAlertTypeSuspiciousItem
#define SEARCH_PICKED_POCKET	(SEARCH_SEARCH) // EAlertTypePickedPocket
#define SEARCH_ROPE				(SEARCH_SEARCH|SEARCH_GUARD|SEARCH_OBSERVE) // EAlertTypeRope
#define SEARCH_PROJECTILE		(SEARCH_SEARCH|SEARCH_GUARD|SEARCH_OBSERVE) // EAlertTypeHitByProjectile
#define SEARCH_MOVEABLE			(SEARCH_SEARCH) // EAlertTypeHitByMoveable
#define SEARCH_SWARM			(SEARCH_SEARCH) // for more than 2 searchers
#define SEARCH_THINK_INTERVAL	2000; // grayman #4220 - how often ProcessSearches() runs (ms) 

class CSearchManager
{
private:
	idList<Search*> _searches;       // A list of all active searches in the mission
	int				_uniqueSearchID; // the next unique id to assign to a new search
	int				_nextThinkTime;	 // grayman #4220 - the search manager's 'think' process only thinks every N seconds

public:
	CSearchManager();  // Constructor
	~CSearchManager(); // Destructor

	void		Clear();

	Search*		StartNewSearch(idAI* ai);

	void		RandomizeHidingSpotList(Search* search);

	int			ObtainSearchID(idAI* ai); // returns searchID for the AI to use

	bool		GetNextHidingSpot(Search* search, idAI* ai, idVec3& nextSpot);

	bool		GetNextSearchSpot(Search* search, idAI* ai, idVec3& nextSpot); // grayman #4220

	//void		RestartHidingSpotSearch(int searchID, idAI* ai); // Close and destroy the current search and start a new search

	void		PerformHidingSpotSearch(int searchID, idAI* ai); // Continue searching

	Search*		GetSearch(int searchID); // returns a pointer to the requested search

	Search*		GetSearchWithEventID(int eventID, bool seekCoopSearch); // returns a pointer to the requested search (event and co-op or swarm)

	Search*		GetSearchAtLocation(EventType eventType, idVec3 location, bool seekCoopSearch); // returns a pointer to the requested search (event type at location, and co-op or swarm)

	Assignment*	GetAssignment(Search* search, idAI* ai); // get ai's assignment for a given search

	Search*		CreateSearch(int searchID); // grayman #3857   
	/*!
	* This method finds hiding spots in the bounds given by two vectors, and also excludes
	* any points contained within a different pair of vectors.
	*
	* The first paramter is a vector which gives the location of the
	* eye from which hiding is desired.
	*
	* The second vector gives the minimums in each dimension for the
	* search space.  
	*
	* The third and fourth vectors give the min and max bounds within which spots should be tested
	*
	* The fifth and sixth vectors give the min and max bounds of an area where
	*	spots should NOT be tested. This overrides the third and fourth parameters where they overlap
	*	(producing a dead zone where points are not tested)
	*
	* The seventh parameter gives the bit flags of the types of hiding spots
	* for which the search should look.
	*
	* The eighth parameter indicates an entity that should be ignored in
	* the visual occlusion checks.  This is usually the searcher itself but
	* can be NULL.
	*
	* This method will only start the search, if it returns 1, you should call
	* continueSearchForHidingSpots every frame to do more processing until that function
	* returns 0.
	*
	* The return value is a 0 for failure, 1 for success.
	*/
	int			StartSearchForHidingSpotsWithExclusionArea
				(
				Search *search,
				const idVec3& hideFromLocation,
				int hidingSpotTypesAllowed,
				idAI* p_ignoreAI
				);

	/*
	* This method continues searching for hiding spots. It will only find so many before
	* returning so as not to cause long delays.  Detected spots are added to the currently
	* building hiding spot list.
	*
	* The return value is 0 if the end of the search was reached, or 1 if there
	* is more processing to do (call this method again next AI frame)
	*
	*/
	int			ContinueSearchForHidingSpots(int searchID, idAI* ai);

	bool		JoinSearch(int searchID, idAI* searcher); // adds searcher to search

	void		LeaveSearch(int searchID, idAI* ai); // searcher leaves a search

	void		AdjustSearchLimits(idBounds& bounds); // grayman #2422 - fit the search to the architecture of the area being searched

	void		destroyCurrentHidingSpotSearch(Search* search);

	void		ConsiderSwitchingSearchers(Search *search, int searcherNum); // grayman #4220

	void		CreateListOfGuardSpots(Search* search, idAI* ai);

	void		ProcessSearches();

	void		Save( idSaveGame *savefile );

	void		Restore( idRestoreGame *savefile );

	//void		DebugPrintHidingSpots(Search* search); // Print the current hiding spots/
	void		DebugPrintSearch(Search* search); // Print the contents of a search
	//void		DebugPrintAssignment(Assignment* assignment); // Print the contents of an assignment


	// Accessor to the singleton instance of this class
	static		CSearchManager* Instance();
};

#endif // __SEARCH_MANAGER_H__
