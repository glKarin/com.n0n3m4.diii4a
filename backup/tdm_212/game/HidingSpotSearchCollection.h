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

#ifndef HIDING_SPOT_SEARCH_COLLECTION_H
#define HIDING_SPOT_SEARCH_COLLECTION_H

/**
* The purpose of this class is to contain all the hiding spot searches in use by
* the AIs in the game.  Hiding spot searches can be shared, so this holds all
* the searches and controls whether or not new searches are required.  
*
* An AI that would like a search done makes a call to the  singleton instance
* of this class and asks for a search for a given stimulus. If it is simlar
* enough to an existing search that search is re-used.
*
*/

#include "DarkmodAASHidingSpotFinder.h"
#include <map>

//---------------------------------------------------------------------------

#define NULL_HIDING_SPOT_SEARCH_HANDLE -1
#define MAX_NUM_HIDING_SPOT_SEARCHES 12

//---------------------------------------------------------------------------

class CHidingSpotSearchCollection
{
private:

	// greebo: This is the main data structure for keeping track of ongoing searches
	struct HidingSpotSearchNode
	{
		// greebo: The id of this search, to resolve pointers after map restore
		int searchId;

		int refCount;
		CDarkmodAASHidingSpotFinder search;
	};

	// Smart pointer typedef
	typedef std::shared_ptr<HidingSpotSearchNode> HidingSpotSearchNodePtr;

	// greebo: The array holding all active hiding spot search pointers
	typedef std::map<int, HidingSpotSearchNodePtr> HidingSpotSearchMap;
	HidingSpotSearchMap searches;

	// This provides a unique ID for the searches
	int highestSearchId;

	/**
	* Constructor (private, this is a singleton class)
	*/
	CHidingSpotSearchCollection();

public:
	/**
	 * greebo: Contains the singleton instance of this class.
	 */
	static CHidingSpotSearchCollection& Instance();

	/**
	* This gets a search by its handle
	*/
	CDarkmodAASHidingSpotFinder* getSearchByHandle(int searchHandle);

	/**
	* This gets a search by its handle and indicates how many people
	* (including the caller) have a reference handle to the search.
	*/
	CDarkmodAASHidingSpotFinder* getSearchAndReferenceCountByHandle(int searchHandle, unsigned int& out_refCount);

	/**
	* This should be called to dereference a hiding spot search. It ensures
	* the search is destroyed when the last user deallocates it.
	*
	* Once this is called, the handle should be considered invalid and never
	* be used again.
	*/
	void dereference(int searchHandle);

	/**
	* This attempts to get or create a new search. If the search
	* already exists, the existing one is returned.
	*
	* The search is referenced if the handle returned is not the null
	* value, so the caller must eventually dereference it.
	*/
	int getOrCreateSearch
	(
		const idVec3 &hideFromPos, 
		idAAS* in_p_aas, 
		float in_hidingHeight,
		idBounds in_searchLimits, 
		idBounds in_searchExclusionLimits, 
		int in_hidingSpotTypesAllowed, 
		idEntity* in_p_ignoreEntity,
		int frameIndex,
		bool& out_b_searchCompleted
	);
		
	// Standard Save/Restore routines 
	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
	
private:
	/**
	* This destroys all searches. Don't call it unless you are shutting down the game.
	*/
	void clear();

	/**
	* This gets an empty hiding spot search from the list and references
	* it before returning it to the caller.
	*/
	HidingSpotSearchNodePtr getNewSearch();

	/**
	* This searches the list for a search with similar bounds.
	*/
	int findSearchByBounds(idBounds bounds, idBounds exclusionBounds);
};

#endif
