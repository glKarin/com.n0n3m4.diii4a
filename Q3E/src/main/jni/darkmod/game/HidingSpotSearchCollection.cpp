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

#include "precompiled.h"
#pragma hdrstop



#include "./HidingSpotSearchCollection.h"

//--------------------------------------------------------------------

// Constructor

CHidingSpotSearchCollection::CHidingSpotSearchCollection() :
	highestSearchId(0)
{}

//--------------------------------------------------------------------

CHidingSpotSearchCollection& CHidingSpotSearchCollection::Instance()
{
	static CHidingSpotSearchCollection _instance;
	return _instance;
}

//--------------------------------------------------------------------

void CHidingSpotSearchCollection::clear()
{
	// Clear the map now that the pointers are destroyed
	searches.clear();
}

void CHidingSpotSearchCollection::Save(idSaveGame *savefile) const
{
    savefile->WriteInt(static_cast<int>(searches.size()));
	for (HidingSpotSearchMap::const_iterator i = searches.begin(); i != searches.end(); ++i)
	{
		const HidingSpotSearchNodePtr& node = i->second;

		savefile->WriteInt(node->searchId);
		savefile->WriteInt(node->refCount);
		node->search.Save(savefile);
	}

	savefile->WriteInt(highestSearchId);
}

void CHidingSpotSearchCollection::Restore(idRestoreGame *savefile)
{
	clear();

	int num;
	savefile->ReadInt(num);

	for (int i = 0; i < num; i++)
	{
		HidingSpotSearchNodePtr node(new HidingSpotSearchNode);

		savefile->ReadInt(node->searchId);
		savefile->ReadInt(node->refCount);
		node->search.Restore(savefile);

		// Insert the allocated search into the map and take the searchid as index
		searches.insert(
			HidingSpotSearchMap::value_type(node->searchId, node)
		);
	}

	savefile->ReadInt(highestSearchId);
}

//--------------------------------------------------------------------

CHidingSpotSearchCollection::HidingSpotSearchNodePtr CHidingSpotSearchCollection::getNewSearch()
{
	if (searches.size() >= MAX_NUM_HIDING_SPOT_SEARCHES)
	{
		return HidingSpotSearchNodePtr();
	}
	else
	{
		HidingSpotSearchNodePtr node(new HidingSpotSearchNode);
		
		// We are returning to somebody, so they have a reference
		node->refCount = 1;

		// greebo: Assign a unique ID to this searchnode
		node->searchId = highestSearchId;

		searches.insert(
			HidingSpotSearchMap::value_type(node->searchId, node)
		);

		// Increase the unique ID
		highestSearchId++;

		return node;
	}
}

//**********************************************************************
// Public
//**********************************************************************

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchByHandle(int searchHandle)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	// Return NULL if not found
	return (found != searches.end()) ? &found->second->search : NULL;
}

//----------------------------------------------------------------------------------

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchAndReferenceCountByHandle
(
	int searchHandle,
	unsigned int& out_refCount
)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	if (found != searches.end())
	{
		out_refCount = found->second->refCount;
		return &found->second->search;
	}
	else
	{
		// not found
		out_refCount = 0;
		return NULL;
	}
}

//------------------------------------------------------------------------------

void CHidingSpotSearchCollection::dereference(int searchHandle)
{
	HidingSpotSearchMap::iterator found = searches.find(searchHandle);

	if (found != searches.end())
	{
		found->second->refCount--;

		if (found->second->refCount <= 0)
		{
			// Remove the search from the map, triggers auto-deletion
			searches.erase(found);
		}
	}
}

//------------------------------------------------------------------------------

int CHidingSpotSearchCollection::findSearchByBounds 
(
	idBounds bounds,
	idBounds exclusionBounds
)
{
	for (HidingSpotSearchMap::iterator i = searches.begin(); i != searches.end(); ++i)
	{
		const HidingSpotSearchNodePtr& node = i->second;

		idBounds existingBounds = node->search.getSearchLimits();
		idBounds existingExclusionBounds = node->search.getSearchExclusionLimits();

		if (existingBounds.Compare(bounds, 50.0))
		{
			if (existingExclusionBounds.Compare(exclusionBounds, 50.0))
			{
				// Reuse this one and return the ID
				node->refCount++;
				return i->first;
			}
		}
	}

	// None found
	return NULL_HIDING_SPOT_SEARCH_HANDLE;
}

//------------------------------------------------------------------------------

int CHidingSpotSearchCollection::getOrCreateSearch
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
)
{
	// Search with same bounds already?
	int searchHandle = findSearchByBounds(in_searchLimits, in_searchExclusionLimits);

	if (searchHandle != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		CDarkmodAASHidingSpotFinder* p_search = getSearchByHandle(searchHandle);
		out_b_searchCompleted = p_search->isSearchCompleted();
		return searchHandle;
	}

	// Make new search
	HidingSpotSearchNodePtr node = getNewSearch();
	if (node == NULL)
	{
		return -1;
	}

	// At this point, we have a valid handle, we rely on it being inserted in the map
	
	// Initialize the search
	
	CDarkmodAASHidingSpotFinder& search = node->search;
	search.initialize
	(
		hideFromPos, 
		in_hidingHeight,
		in_searchLimits, 
		in_searchExclusionLimits,
		in_hidingSpotTypesAllowed, 
		in_p_ignoreEntity
	);

	// Start search
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Starting search for hiding spots\r");
	bool b_moreProcessingToDo = search.startHidingSpotSearch
	(
		search.hidingSpotList,
		cv_ai_max_hiding_spot_tests_per_frame.GetInteger(),
		frameIndex
	);
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("First pass of hiding spot search found %d spots\r", search.hidingSpotList.getNumSpots());

	// Is search completed?
	out_b_searchCompleted = !b_moreProcessingToDo;

	// Search created, return ID to caller
	return node->searchId;
}
