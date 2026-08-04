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
/*!
* This is the interface for an n-tree structure which sorts and holds
* darkmod hiding spots.
*
* 
* 
*/
#ifndef DARKMOD_HIDING_SPOT_TREE
#define DARKMOD_HIDING_SPOT_TREE

/*!
// @structure darkModHidingSpot
// @author SophisticatedAZombie (DMH)
// This structure holds information about a hiding spot.
*/
struct darkModHidingSpot
{
	aasGoal_t goal;

	// The hiding spot characteristic bit flags
	// as defined by the darkModHidingSpotType enumeration
	int hidingSpotTypes;

	// The light quotient of the spot.
	float lightQuotient;
	
	// The hiding spot "hidingness" quality, without distance from the search
	// center factored in.
	float qualityWithoutDistanceFactor;

	// The hiding spot "hidingness" quality, from 0 to 1.0
	float quality;
};

//-------------------------------------------------------------------------

struct TDarkmodHidingSpotAreaNode
{
	// greebo: This is a unique ID to resolve pointers after map restore
	int id;

	unsigned int aasAreaIndex;
	unsigned int count;

	TDarkmodHidingSpotAreaNode* p_prevSibling;
	TDarkmodHidingSpotAreaNode* p_nextSibling;

	// Each area node holds a list of hiding spots
	idList<darkModHidingSpot*> spots;

	// Quality of the best spot in the area
	float bestSpotQuality;

	// The extents
	idBounds bounds;

};

//---------------------------------------------------------------------------

class CDarkmodHidingSpotTree
{
private:
	// The highest used area node id (0 on initialisiation)
	int maxAreaNodeId;

protected:

	// The number of areas
	unsigned int numAreas;

	unsigned int numSpots;

	// The first area
	TDarkmodHidingSpotAreaNode* p_firstArea;
	TDarkmodHidingSpotAreaNode* p_lastArea;

	// Mapping methods, retrieves node pointers for indices and vice versa
	int getAreaNodeId(TDarkmodHidingSpotAreaNode* area) const; // returns -1 for invalid pointer
	TDarkmodHidingSpotAreaNode* getAreaNode(int areaNodeId) const; // returns NULL for invalid Id

	/*!
	* Gets the Nth spot from the tree, where N is a 0 based index.
	* This is a slow iteration from the beginning of the tree
	* and should only be used if the index being retrieved is less than the
	* previous index retrieved or if none has yet been retrieved
	* since the last time the tree was altered.
	*
	* @return Pointer to spot, NULL if index was out of bounds
	*/
	darkModHidingSpot* getNthSpotInternal(unsigned int index, idBounds& out_areaNodeBounds);

	/*!
	* This method sorts the area by hiding spot light internally
	*
	* @param inout_p_firstNode On input, pointer to the first point in the area
	*	On return, pointer to the first point in the list, sorted from
	*	greatest to least quality.
	*
	* @param numSpots The number of spots in the list passed in.  The number of
	*	spots is not changed by sorting, unless the algorithm is broken, in which
	*	case you should fix it yourself, damnit.

	*/
	void quicksortHidingSpotList(darkModHidingSpot*& inout_p_firstNode,	unsigned int numSpots);

	/*!
	* This method sorts the given area list
	*
	* @param inout_p_firstNode On input, pointer to the first area
	*	On return, pointer to the first area in the list, sorted from
	*	greatest to least quality.
	*
	* @param numAreas The number of areas in the list passed in.  

	*/
	void quicksortAreaList
	(
		TDarkmodHidingSpotAreaNode*& inout_p_firstNode,
		unsigned int numAreas
	);

	/*!
	* This method breaks a node into 8 rough "octants" 
	* The original node is used as one of the octants, and
	* the other nodes created between this node and the next
	* original node in the  list
	*
	* @param in_p_areaNode The node that is to be sub-divided.
	*
	* @param out_numSubAreasWithPoints Passes back out the number
	*	of sub areas (up to 8) that had points.
	*
	* @param out_subAreas Passes back the pointers to the sub
	*	areas that had points (up to 8)
	*/
	bool subDivideArea
	(
		TDarkmodHidingSpotAreaNode* in_p_areaNode,
		unsigned int& out_numSubAreasWithPoints,
		TDarkmodHidingSpotAreaNode* out_p_subAreas[8]
	);

public:

	/*!
	* Constructor for an empty tree
	*/
	CDarkmodHidingSpotTree();

	/*!
	* Destroys all nodes and clears the
	* tree memory usage on destruction
	*/
	~CDarkmodHidingSpotTree();

	/*!
	* Destroys all nodes and clears the tree memory
	* usage.
	*/
	void Clear();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	/*!
	* Get the number of spots in the entire tree
	*/
	ID_INLINE unsigned int getNumSpots()
	{
		return numSpots;
	}

	/*!
	* Gets the node for an area by area index
	*/
	TDarkmodHidingSpotAreaNode* getArea(unsigned int areaIndex);

	/*!
	* Inserts an area and returns the are node handle
	* for use in inserting hiding spots.
	*/
	TDarkmodHidingSpotAreaNode* insertArea(unsigned int areaIndex);

	/*!
	* Tests a spot for redundancy with spots already in
	* the same area.  Redundancy occurs if the distance
	* is less than the redundancy distance. 
	* If redundant, the existing point may be updated
	* to higher quality values if the new point was
	* of higher quality than the existing point.
	*
	* @return true if redundant
	* @return false if not redundant
	*/
	bool determineSpotRedundancy
	(
		TDarkmodHidingSpotAreaNode* p_areaNode,
		aasGoal_t goal,
		int hidingSpotTypes,
		float quality,
		float redundancyDistance
	);

	/*!
	* Inserts a hiding spot into a given area node.
	*
	* @param p_areaNode The area node to which the point should be added
	*
	* @param goal The goal descriptor of the point
	*
	* @param hidingSpotTypes The types of hiding spot that apply to the point
	*
	* @param quality The measure of hiding spot quality from 0.0 to 1.0
	*
	* @param redundancyDistance The distance between points for a unique point
	*	to be created. If the new point is within this distance from a point
	*	already in the area, the points will be combined.
	*	If this number is < 0.0, then no redundancy testing is done.
	*
	*/
	bool insertHidingSpot
	(
		TDarkmodHidingSpotAreaNode* p_areaNode,
		aasGoal_t goal,
		int hidingSpotTypes,
		float lightQuotient,
		float qualityWithoutDistanceFactor,
		float quality,
		float redundancyDistance
	);

	/*!
	* This method is intended to be used once all of the
	* nodes in the tree have been created and all the points
	* inserted.  It subdivides each area in the tree
	* into octants based on x/y/z midline partitions.
	* It does this over and over on any area which contains
	* more than the limiting number of points.
	*
	* @param maxPointsPerArea
	*
	* @return true on succes
	*
	* @return false on failure, structure may be corrupted due
	*	to failure to allocate new area nodes. This should only 
	*	happen if the computer critically runs out of memory 
	*	(time to shut down the application).
	*
	*/
	bool subDivideAreas(unsigned int maxPointsPerArea);

	/*! 
	* Starts an iteration of the areas in the tree
	*/
	TDarkmodHidingSpotAreaNode* getFirstArea();

	/*!
	* Moves forward in an iteration of the tree
	*/
	TDarkmodHidingSpotAreaNode* getNextArea(TDarkmodHidingSpotAreaNode* curArea);

	/*!
	* Gets first hiding spot in area
	*/
	darkModHidingSpot* getFirstHidingSpotInArea(TDarkmodHidingSpotAreaNode* area);

	/*!
	* Gets next hiding spot in area
	*/
	// greebo: Commented this out, the hidingspot structures are a linked list anyway.
	//darkModHidingSpot* getNextHidingSpotInArea(TDarkModHidingSpotTreeIterationHandle& inout_spotHandle);

	/*!
	* This speeds up requests for the Nth spot if N is >= the
	* last N requested and not by much (such as N+1)
	*
	* @param index The index of the spot to get
	*
	*/
	darkModHidingSpot* getNthSpot(unsigned int index);

	/*!
	* This speeds up requests for the Nth spot if N is >= the
	* last N requested and not by much (such as N+1)
	*
	* @param index The index of the spot to get.
	*
	* @param out_areaBounds The bounds of the sub-area containing
	*	the spot
	*
	*/
	darkModHidingSpot* getNthSpotWithAreaNodeBounds(unsigned int index, idBounds& out_areaNodeBounds);

	/*!
	* Attempts to split off one Nth of the tree (1/N) in a logical fashion
	* for sharing a search. The caller provides a tree which after the call
	* contains only the areas and spots removed from this tree.
	*
	* @param N The fraction 1/Nth of this tree will be moved to the other tree
	*	If N is 0 nothing is moved.
	* 
	* @param out_otherTree This is the tree to which the fraction of the tree
	*	will be moved.  The tree is cleared before any areas or spots are moved
	*	to it.
	*/
	//bool getOneNth(unsigned int N, CDarkmodHidingSpotTree& out_otherTree); // grayman #3857

	/*!
	* This copies this tree into another tree.
	*/
	bool copy(CDarkmodHidingSpotTree* p_out_otherTree);

	/*!
	* This method rescores the points and repriortizes the list based on a new search center.
	* This can be used if a new stimulus is detected while searching, to bias the search
	* toward the new stimulous.
	*
	*/
	bool sortForNewCenter(idVec3 center, float searchRadius);
};

#endif
