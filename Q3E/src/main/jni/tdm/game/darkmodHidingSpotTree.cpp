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
* Implementation of the darkmod hiding spot tree.
*
* Identified hiding spots are tracked by AAS area.
* Written for the darkmod.
*
*/

// Includes
#include "precompiled.h"
#pragma hdrstop



#include "darkmodHidingSpotTree.h"

//--------------------------------------------------------------------------

/*!
* Standard constructor, creates an empty tree.
*/
CDarkmodHidingSpotTree::CDarkmodHidingSpotTree() :
	maxAreaNodeId(0),
	numAreas(0),
	numSpots(0),
	p_firstArea(NULL),
	p_lastArea(NULL)
{}

//--------------------------------------------------------------------------

/*!
* Destructor
*/
CDarkmodHidingSpotTree::~CDarkmodHidingSpotTree()
{
	Clear();
}

//--------------------------------------------------------------------------

void CDarkmodHidingSpotTree::Clear()
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		p_node->spots.DeleteContents( true );

		TDarkmodHidingSpotAreaNode* p_temp2 = p_node->p_nextSibling;
		delete p_node;
		p_node = p_temp2;
	}

	// Now empty
	numAreas = 0;
	numSpots = 0;
	p_firstArea = NULL;
	p_lastArea = NULL;
	maxAreaNodeId = 0;
}

int CDarkmodHidingSpotTree::getAreaNodeId(TDarkmodHidingSpotAreaNode* area) const
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		if (p_node == area) 
		{
			return p_node->id;
		}

		p_node = p_node->p_nextSibling;
	}

	return -1;
}

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getAreaNode(int areaNodeId) const
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		if (p_node->id == areaNodeId) 
		{
			return p_node;
		}

		p_node = p_node->p_nextSibling;
	}

	return NULL;
}

void CDarkmodHidingSpotTree::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(maxAreaNodeId);

	savefile->WriteFloat(static_cast<float>(numAreas));
	savefile->WriteFloat(static_cast<float>(numSpots));

	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		// Save areaNode members
		savefile->WriteInt(p_node->id);
		savefile->WriteFloat(static_cast<float>(p_node->aasAreaIndex));
		savefile->WriteFloat(static_cast<float>(p_node->count));
		
		//p_prevSibling && p_nextSibling get automatically restored;

		savefile->WriteInt(p_node->spots.Num());
		for (int i = 0; i < p_node->spots.Num(); i++)
		{
			// Save the aasgoal_t
			savefile->WriteInt(p_node->spots[i]->goal.areaNum);
			savefile->WriteVec3(p_node->spots[i]->goal.origin);
			
			savefile->WriteInt(p_node->spots[i]->hidingSpotTypes);
			savefile->WriteFloat(p_node->spots[i]->lightQuotient);
			savefile->WriteFloat(p_node->spots[i]->qualityWithoutDistanceFactor);
			savefile->WriteFloat(p_node->spots[i]->quality);
		}

		// Quality of the best spot in the area
		savefile->WriteFloat(p_node->bestSpotQuality);
		savefile->WriteBounds(p_node->bounds);

		p_node = p_node->p_nextSibling;
	}

	//p_firstArea and p_lastArea get restored automatically
}

void CDarkmodHidingSpotTree::Restore( idRestoreGame *savefile )
{
	float tempFloat;

	savefile->ReadInt(maxAreaNodeId);

	savefile->ReadFloat(tempFloat);
	numAreas = static_cast<unsigned int>(tempFloat);
	savefile->ReadFloat(tempFloat);
	numSpots = static_cast<unsigned int>(tempFloat);

	p_firstArea = NULL;

	TDarkmodHidingSpotAreaNode* lastArea = NULL;
	for (unsigned int areaIndex = 0; areaIndex < numAreas; areaIndex++)
	{
		TDarkmodHidingSpotAreaNode* curArea = new TDarkmodHidingSpotAreaNode;

		if (p_firstArea == NULL)
		{
			// Pointer to first area still NULL, take this one
			p_firstArea = curArea;
		}

		// Restore areaNode members
		savefile->ReadInt(curArea->id);

		savefile->ReadFloat(tempFloat);
		curArea->aasAreaIndex = static_cast<unsigned int>(tempFloat);

		savefile->ReadFloat(tempFloat);
		curArea->count = static_cast<unsigned int>(tempFloat);
		
		// Restore the "previous" pointer
		curArea->p_prevSibling = lastArea;
		curArea->p_nextSibling = NULL;

		if (curArea->p_prevSibling != NULL)
		{
			// Point the "next" pointer of the previous class to this one
			curArea->p_prevSibling->p_nextSibling = curArea;
		}

		int numSpots;
		savefile->ReadInt(numSpots);
		
		curArea->spots.SetNum(numSpots);
		for (int i = 0; i < numSpots; i++)
		{
			curArea->spots[i] = new darkModHidingSpot;

			// Read the aasgoal_t
			savefile->ReadInt(curArea->spots[i]->goal.areaNum);
			savefile->ReadVec3(curArea->spots[i]->goal.origin);
			
			savefile->ReadInt(curArea->spots[i]->hidingSpotTypes);
			savefile->ReadFloat(curArea->spots[i]->lightQuotient);
			savefile->ReadFloat(curArea->spots[i]->qualityWithoutDistanceFactor);
			savefile->ReadFloat(curArea->spots[i]->quality);
		}

		// Quality of the best spot in the area
		savefile->ReadFloat(curArea->bestSpotQuality);
		savefile->ReadBounds(curArea->bounds);

		// Update the "lastArea" before the next loop
		lastArea = curArea;
	}

	p_lastArea = lastArea;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getArea(unsigned int areaIndex)
{
	TDarkmodHidingSpotAreaNode* p_node = p_firstArea;
	while (p_node != NULL)
	{
		if (p_node->aasAreaIndex == areaIndex)
		{
			return p_node;
		}

		p_node = p_node->p_nextSibling;
	}

	return NULL;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::insertArea(unsigned int areaIndex)
{
	TDarkmodHidingSpotAreaNode* p_node = new TDarkmodHidingSpotAreaNode;
	if (p_node == NULL)
	{
		return NULL;
	}

	// Generate a unique id for this node
	maxAreaNodeId++;
	p_node->id = maxAreaNodeId;

	p_node->aasAreaIndex = areaIndex;
	p_node->count = 0;
	p_node->bestSpotQuality = 0.0;
	p_node->spots.Clear();
	// greebo: Set granularity to something large to keep reallocations low
	p_node->spots.SetGranularity(128);

	// Put at end (worst areas) of list for now
	p_node->p_nextSibling = NULL;
	if (p_lastArea != NULL)
	{
		p_lastArea->p_nextSibling = p_node;
	}
	p_node->p_prevSibling = p_lastArea;
	p_lastArea = p_node;

	// Special case, was empty list
	if (p_firstArea == NULL)
	{
		p_firstArea = p_node;
	}

	numAreas++;

	return p_node;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::determineSpotRedundancy
(
	TDarkmodHidingSpotAreaNode* p_areaNode,
	aasGoal_t goal,
	int hidingSpotTypes,
	float quality,
	float redundancyDistance
)
{
	// Test parameters
	if (p_areaNode == NULL)
	{
		return false;
	}

	//float redundancyDistSqr = redundancyDistance*redundancyDistance;

	// Compare distance with other points in the area
	for (int i = 0; i < p_areaNode->spots.Num(); i++)
	{
		// Compute distance
		idVec3 distanceVec = goal.origin - p_areaNode->spots[i]->goal.origin;
		if (distanceVec.LengthSqr() <= redundancyDistance)
		{
			// This point is redundant, should combine.
			p_areaNode->spots[i]->hidingSpotTypes |= hidingSpotTypes;
			if (p_areaNode->spots[i]->quality < quality)
			{
				// Use higher quality location
				p_areaNode->spots[i]->quality = quality;
				p_areaNode->spots[i]->goal = goal;
			}

			// Combined
			return true;
		}
	}

	/*darkModHidingSpot* p_cursor = p_areaNode->p_firstSpot;
	while (p_cursor != NULL)
	{
		// Compute distance
		idVec3 distanceVec = goal.origin - p_cursor->spot.goal.origin;
		if (distanceVec.LengthFast() <= redundancyDistance)
		{
			// This point is redundant, should combine.
			p_cursor->spot.hidingSpotTypes |= hidingSpotTypes;
			if (p_cursor->spot.quality < quality)
			{
				// Use higher quality location
				p_cursor->spot.quality = quality;
				p_cursor->spot.goal = goal;
			}

			// Combined
			return true;
		}

		// Next spot
		p_cursor = p_cursor->p_next;
	}*/

	// Not redundant
	return false;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::insertHidingSpot
(
	TDarkmodHidingSpotAreaNode* p_areaNode,
	aasGoal_t goal,
	int hidingSpotTypes,
	float lightQuotient,
	float qualityWithoutDistanceFactor,
	float quality,
	float redundancyDistance
)
{
	// Test parameters
	if (p_areaNode == NULL)
	{
		return false;
	}

	// Update best spot quality in area
	if (quality > p_areaNode->bestSpotQuality)
	{
		p_areaNode->bestSpotQuality = quality;

		// Slide area toward front of list until it is in
		// best spot quality order
		while (p_areaNode->p_prevSibling != NULL)
		{
			if (p_areaNode->p_prevSibling->bestSpotQuality < p_areaNode->bestSpotQuality)
			{
				TDarkmodHidingSpotAreaNode* p_bumped = p_areaNode->p_prevSibling;
				TDarkmodHidingSpotAreaNode* p_bumpedPrev = p_bumped->p_prevSibling;
				if (p_bumpedPrev != NULL)
				{
					p_bumpedPrev->p_nextSibling = p_areaNode;
				}
				else
				{
					p_firstArea = p_areaNode;
				}
				p_areaNode->p_prevSibling = p_bumpedPrev;

				p_bumped->p_nextSibling = p_areaNode->p_nextSibling;
				if (p_bumped->p_nextSibling != NULL)
				{
					p_bumped->p_nextSibling->p_prevSibling = p_bumped;
				}
				else
				{
					p_lastArea = p_bumped;
				}
				p_bumped->p_prevSibling = p_areaNode;
				p_areaNode->p_nextSibling = p_bumped;
				
			}
			else
			{
				break;
			}
		}
	}

	// Test if it is redundant
	if (redundancyDistance >= 0.0)
	{
		if (determineSpotRedundancy (p_areaNode, goal, hidingSpotTypes, quality, redundancyDistance))
		{
			// Spot was redundant with other points. The other points may have
			// been modified, but we do not add the new point
			return true;
		}
	}

	// Not redundant, so adding new spot
	darkModHidingSpot* p_spot = new darkModHidingSpot;
	if (p_spot == NULL)
	{
		return false;
	}

	p_spot->goal = goal;
	p_spot->hidingSpotTypes = hidingSpotTypes;
	p_spot->lightQuotient = lightQuotient;
	p_spot->qualityWithoutDistanceFactor = qualityWithoutDistanceFactor;
	p_spot->quality = quality;

	// Shortcut reference
	idList<darkModHidingSpot*>& spotList = p_areaNode->spots;

	// Add some randomness to the order of points in the areas.

	if ( spotList.Num() > 0 )
	{
		// grayman #4220

		if ( cv_ai_search_type.GetInteger() == 1 ) // 2.03 - style
		{
			// Insert the new spot to a random location in the list
			// and move the current occupier to the end.
			int randomLocation = gameLocal.random.RandomInt(spotList.Num());

			// greebo: Important: Don't do this: spotList.Append( spotList[randomLocation] )!
			// Copy the old pointer beforehand to avoid references to invalid memory
			darkModHidingSpot* oldSpot = spotList[randomLocation];
			spotList.Append(oldSpot);
			spotList[randomLocation] = p_spot;
		}
		else
		{
			// no randomness; order from best quality to worst quality

			int index = 0;
			for ( ; index < spotList.Num(); index++ )
			{
				if ( p_spot->quality > spotList[index]->quality )
				{
					spotList.Insert(p_spot, index);
					break;
				}
			}

			if ( index == spotList.Num() )
			{
				// smaller quality than any spots in the list
				spotList.Append(p_spot);
			}
		}
	}
	else
	{
		// List is still empty, just add the new spot
		spotList.Append(p_spot);
	}

	// The counter increases in any case
	p_areaNode->count++;

	// ---------------------------------------------

	// Randomly add to either front or back of the area list
	/*if (gameLocal.random.RandomFloat() < 0.5)
	{
		// Add to front of list

		if (p_areaNode->spots.Num() <= 0)
		{
			p_areaNode->spots.Append(p_spot);
		}
		else
		{
			// Move the first item to the back
			p_areaNode->spots.Append(p_spot);
		}

		// Append a dummy object at then end
		p_areaNode->spots.Append(NULL);
		
		p_spot->p_next = p_areaNode->p_firstSpot;
		p_areaNode->p_firstSpot = p_spot;
		if (p_areaNode->p_lastSpot == NULL)
		{
			p_areaNode->p_lastSpot = p_spot;
		}
		p_areaNode->count ++;
	}
	else
	{
		// Add to end of list
		p_spot->p_next = NULL;
		
		if (p_areaNode->p_lastSpot == NULL)
		{
			p_areaNode->p_lastSpot = p_spot;
			p_areaNode->p_firstSpot = p_spot;
		}
		else
		{
			p_areaNode->p_lastSpot->p_next = p_spot;
			p_areaNode->p_lastSpot = p_spot;
		}
		p_areaNode->count ++;
	}*/

	// Change bounds of area
	if (p_areaNode->spots.Num() == 1)
	{
		// We only have one point, clear the bounds beforehand
		p_areaNode->bounds.Clear();
	}
	// Include the point in the bounds
	p_areaNode->bounds.AddPoint(p_spot->goal.origin);

	// One more point 
	numSpots++;

	// Done
	return true;
}

//-------------------------------------------------------------------------
#define NUM_SECTORS_IN_SUBDIVIDE 8

bool CDarkmodHidingSpotTree::subDivideArea
(
	TDarkmodHidingSpotAreaNode* in_p_areaNode,
	unsigned int& out_numSubAreasWithPoints,
	TDarkmodHidingSpotAreaNode* out_p_subAreas[8]
)
{

	// test params
	if (out_p_subAreas == NULL || in_p_areaNode == NULL)
	{
		return false;
	}

	// No sub areas yet
	out_numSubAreasWithPoints = 0;
	for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex ++)
	{
		out_p_subAreas[subAreaIndex] = NULL;
	}

	// Divide bounds into subAreas
	idBounds subAreaBounds[NUM_SECTORS_IN_SUBDIVIDE];

	idVec3 midPoint = in_p_areaNode->bounds.GetCenter();
	
	// < x, < y, < z
	subAreaBounds[0][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[0][1].x = midPoint.x;
	subAreaBounds[0][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[0][1].y = midPoint.y;
	subAreaBounds[0][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[0][1].z = midPoint.z;

	// > x, < y, < z
	subAreaBounds[1][0].x = midPoint.x;
	subAreaBounds[1][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[1][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[1][1].y = midPoint.y;
	subAreaBounds[1][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[1][1].z = midPoint.z;

	// < x, > y, < z
	subAreaBounds[2][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[2][1].x = midPoint.x;
	subAreaBounds[2][0].y = midPoint.y;
	subAreaBounds[2][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[2][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[2][1].z = midPoint.z;

	// > x, > y, < z
	subAreaBounds[3][0].x = midPoint.x;
	subAreaBounds[3][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[3][0].y = midPoint.y;
	subAreaBounds[3][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[3][0].z = in_p_areaNode->bounds[0].z;
	subAreaBounds[3][1].z = midPoint.z;

	// < x, < y, > z
	subAreaBounds[4][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[4][1].x = midPoint.x;
	subAreaBounds[4][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[4][1].y = midPoint.y;
	subAreaBounds[4][0].z = midPoint.z;
	subAreaBounds[4][1].z = in_p_areaNode->bounds[1].z;

	// > x, < y, > z
	subAreaBounds[5][0].x = midPoint.x;
	subAreaBounds[5][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[5][0].y = in_p_areaNode->bounds[0].y;
	subAreaBounds[5][1].y = midPoint.y;
	subAreaBounds[5][0].z = midPoint.z;
	subAreaBounds[5][1].z = in_p_areaNode->bounds[1].z;

	// < x, > y, > z
	subAreaBounds[6][0].x = in_p_areaNode->bounds[0].x;
	subAreaBounds[6][1].x = midPoint.x;
	subAreaBounds[6][0].y = midPoint.y;
	subAreaBounds[6][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[6][0].z = midPoint.z;
	subAreaBounds[6][1].z = in_p_areaNode->bounds[1].z;

	// > x, > y, > z
	subAreaBounds[7][0].x = midPoint.x;
	subAreaBounds[7][1].x = in_p_areaNode->bounds[1].x;
	subAreaBounds[7][0].y = midPoint.y;
	subAreaBounds[7][1].y = in_p_areaNode->bounds[1].y;
	subAreaBounds[7][0].z = midPoint.z;
	subAreaBounds[7][1].z = in_p_areaNode->bounds[1].z;

	// Create sub lists for points
	idList<darkModHidingSpot*> subAreaSpots[NUM_SECTORS_IN_SUBDIVIDE];
	for (int i = 0; i < NUM_SECTORS_IN_SUBDIVIDE; i++)
	{
		subAreaSpots[i].SetGranularity(128);
	}

	unsigned int subAreaPointCounts[NUM_SECTORS_IN_SUBDIVIDE] = {0,0,0,0,0,0,0,0};
	float subAreaPointBestQualities[NUM_SECTORS_IN_SUBDIVIDE] = {0,0,0,0,0,0,0,0};

	// Iterate over the points and sort them into bounds
	for (int i = 0; i < in_p_areaNode->spots.Num(); i++)
	{
		// What subArea does the point fall within?
		for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex++)
		{
			// If point falls in here, or it is last subArea and no earlier one took it
			if (subAreaIndex >= NUM_SECTORS_IN_SUBDIVIDE-1 || subAreaBounds[subAreaIndex].ContainsPoint(in_p_areaNode->spots[i]->goal.origin))
			{
				// Point falls in this subArea
				subAreaPointCounts[subAreaIndex]++;
				if (in_p_areaNode->spots[i]->quality > subAreaPointBestQualities[subAreaIndex])
				{
					subAreaPointBestQualities[subAreaIndex] = in_p_areaNode->spots[i]->quality;
				}

				// Put it at the end of its point list
				subAreaSpots[subAreaIndex].Append(in_p_areaNode->spots[i]);

				break;
			}
		
		} // Test next subArea
	}

	// Remove all node pointers from the original list
	in_p_areaNode->spots.ClearFree();

	// Make nodes for any subArea that isn't empty
	bool b_originalUsed = false;
	//TDarkmodHidingSpotAreaNode* p_nodeAfterOriginal = in_p_areaNode->p_nextSibling;
	//TDarkmodHidingSpotAreaNode* p_nodeBeforeOriginal = in_p_areaNode->p_prevSibling;
	TDarkmodHidingSpotAreaNode* p_newNodePreviousSibling = in_p_areaNode;
	TDarkmodHidingSpotAreaNode* p_newNodeNextSibling = in_p_areaNode->p_nextSibling;

	for (int subAreaIndex = 0; subAreaIndex < NUM_SECTORS_IN_SUBDIVIDE; subAreaIndex ++)
	{
		TDarkmodHidingSpotAreaNode* p_areaNode = NULL;
		
		// Any points in this subArea?
		if (subAreaPointCounts[subAreaIndex] > 0)
		{
			// One more sub area with points
			out_numSubAreasWithPoints++;

			// Need node, is original already used?
			if (!b_originalUsed)
			{
				p_areaNode = in_p_areaNode;
				b_originalUsed = true;
			}
			else
			{
				// Make new node
				p_areaNode = new TDarkmodHidingSpotAreaNode;
				assert(p_areaNode != NULL);

				maxAreaNodeId++;
				p_areaNode->id = maxAreaNodeId;

				// Fill out node properties that don't change from original
				p_areaNode->aasAreaIndex = in_p_areaNode->aasAreaIndex;
	
				// One more area in the tree
				numAreas++;
			}

			// Make pointer to the area node in out array for caller
			out_p_subAreas[out_numSubAreasWithPoints-1] = p_areaNode;

			// Fill out node properties that are different than original
			p_areaNode->bestSpotQuality = subAreaPointBestQualities[subAreaIndex];
			p_areaNode->bounds = subAreaBounds[subAreaIndex];
			p_areaNode->count = subAreaPointCounts[subAreaIndex];
			p_areaNode->spots = subAreaSpots[subAreaIndex];
			
			// Link into area list just after original (if not original already)
			if (p_areaNode != in_p_areaNode)
			{
				// Link with previous sibling
				p_areaNode->p_prevSibling =	p_newNodePreviousSibling;
				if (p_newNodePreviousSibling != NULL)
				{
					p_newNodePreviousSibling->p_nextSibling = p_areaNode;
				}
				else
				{
					// This area is now at start of list
					p_firstArea = p_areaNode;
				}

				// Link to next sibling
				p_areaNode->p_nextSibling = p_newNodeNextSibling;
				if (p_newNodeNextSibling != NULL)
				{
					p_newNodeNextSibling->p_prevSibling = p_areaNode;
				}
				else
				{
					// This area is now at end of list
					p_lastArea = p_areaNode;
				}

				// Next new node comes after this new node but before next original node
				p_newNodePreviousSibling = p_areaNode;
			}
		} // Subarea had points, needs node
	} // Next subArea

	// Done
	return true;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::subDivideAreas(unsigned int maxPointsPerArea)
{
	// Ride the list of areas
	TDarkmodHidingSpotAreaNode* p_firstAreaNeedingSubDivision = p_firstArea;

	while (p_firstAreaNeedingSubDivision != NULL)
	{
		// Make a pass through the tree
		TDarkmodHidingSpotAreaNode* p_originalAreaCursor = p_firstAreaNeedingSubDivision;
		TDarkmodHidingSpotAreaNode* p_originalAreaNext = NULL;

		// No areas are known to need sub-division now
		p_firstAreaNeedingSubDivision = NULL;

		// Iterate the list
		while (p_originalAreaCursor != NULL)
		{
			// Remember the next original area we will process
			p_originalAreaNext = p_originalAreaCursor->p_nextSibling;

			// Subdivide this original area if it has more points that the limit
			// given by the caller
			if (p_originalAreaCursor->count > maxPointsPerArea)
			{
			
				// Sub-divide the area
				unsigned int numSubAreasWithPoints = 0;
				TDarkmodHidingSpotAreaNode* p_subAreas[8];

				if (!subDivideArea(p_originalAreaCursor, numSubAreasWithPoints,	p_subAreas))
				{
					// Failed to sub-divide this area due to lack of memory
					return false;
				}

				// is this the first area that we found this pass that needed sub-division?
				if (numSubAreasWithPoints > 1 && p_firstAreaNeedingSubDivision == NULL)
				{
					p_firstAreaNeedingSubDivision = p_originalAreaCursor;
				}


			} // Done subdividing this area
            
			// On to next original area
			p_originalAreaCursor = p_originalAreaNext;
		}

	} // As long as some area needed sub-division, we will do another pass

	// Done
	return true;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getFirstArea()
{
	return p_firstArea;
}

//-------------------------------------------------------------------------

TDarkmodHidingSpotAreaNode* CDarkmodHidingSpotTree::getNextArea(TDarkmodHidingSpotAreaNode* curArea)
{
	TDarkmodHidingSpotAreaNode* p_cursor = curArea;
	if (p_cursor != NULL)
	{
		p_cursor = p_cursor->p_nextSibling;
	}

    return p_cursor;
}

//-------------------------------------------------------------------------

darkModHidingSpot* CDarkmodHidingSpotTree::getFirstHidingSpotInArea(TDarkmodHidingSpotAreaNode* area)
{
	TDarkmodHidingSpotAreaNode* p_areaCursor = area;
	if (p_areaCursor == NULL)
	{
		return NULL;
	}

	if (p_areaCursor->spots.Num() > 0)
	{
		return p_areaCursor->spots[0];
	}
	
	return NULL;
}

//-------------------------------------------------------------------------

/*darkModHidingSpot* CDarkmodHidingSpotTree::getNextHidingSpotInArea
(
	TDarkModHidingSpotTreeIterationHandle& inout_spotHandle
)
{
	darkModHidingSpot* p_cursor = (darkModHidingSpot*) inout_spotHandle;
	if (p_cursor == NULL)
	{
		return NULL;
	}

	p_cursor = p_cursor->p_next;
	inout_spotHandle = (TDarkModHidingSpotTreeIterationHandle) p_cursor;

	if (p_cursor != NULL)
	{
		return &(p_cursor->spot);
	}
	else
	{
		return NULL;
	}
}*/

//-------------------------------------------------------------------------

darkModHidingSpot* CDarkmodHidingSpotTree::getNthSpotInternal
(
	unsigned int index,
	idBounds& out_areaNodeBounds
)
{
	unsigned int accumulatedIndex = 0;

	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;

	// Find correct area
	while (p_areaCursor != NULL)
	{
		if (accumulatedIndex + p_areaCursor->count > index)
		{
			break;
		}
		else
		{
			accumulatedIndex += p_areaCursor->count;
			p_areaCursor = p_areaCursor->p_nextSibling;
		}
	}

	// Is index beyond end of tree?
	if (p_areaCursor == NULL)
	{
		return NULL;
	}

	// Report bounds of area node to caller
	out_areaNodeBounds = p_areaCursor->bounds;

	// How many spots within is this?
	int chosenSpotIndex = index - accumulatedIndex;

	assert(p_areaCursor->spots.Num() > chosenSpotIndex);

	// Found it
	return p_areaCursor->spots[chosenSpotIndex];;
}

//-------------------------------------------------------------------------

darkModHidingSpot* CDarkmodHidingSpotTree::getNthSpot(unsigned int index)
{
	int spotDelta = index;
	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
	//darkModHidingSpot* p_spotCursor = NULL;

	// Iterate to correct point
	while (p_areaCursor != NULL)
	{
		// Check if we can just skip this entire area
		if (spotDelta >= p_areaCursor->spots.Num())
		{
			spotDelta -= p_areaCursor->spots.Num();
			p_areaCursor = p_areaCursor->p_nextSibling;

			if (p_areaCursor == NULL)
			{
				break;
			}
		}
		else
		{
			// No we can't skip the entire area, get the spot
			assert(spotDelta < p_areaCursor->spots.Num());

			// Found it
			return p_areaCursor->spots[spotDelta];
		}
	}

	// Index requested is out of bounds
	DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Requested spot index is out of bounds: %d\r", index);
	return NULL;
}

//-------------------------------------------------------------------------

darkModHidingSpot* CDarkmodHidingSpotTree::getNthSpotWithAreaNodeBounds
(
	unsigned int index,
	idBounds& out_areaNodeBounds
)
{
	int spotDelta = index;
	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
	//darkModHidingSpot* p_spotCursor = NULL;

	// Iterate to correct point
	while (p_areaCursor != NULL)
	{
		// Check if we can just skip this entire area
		if (spotDelta >= p_areaCursor->spots.Num())
		{
			spotDelta -= p_areaCursor->spots.Num();
			p_areaCursor = p_areaCursor->p_nextSibling;

			if (p_areaCursor == NULL)
			{
				break;
			}
		}
		else
		{
			// Copy bounds to output
			out_areaNodeBounds = p_areaCursor->bounds;

			// No we can't skip the entire area, get the spot
			assert(spotDelta < p_areaCursor->spots.Num());

			// Found it
			return p_areaCursor->spots[spotDelta];
		}
	}

	// Index requested is out of bounds
	DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Requested spot index is out of bounds: %d\r", index);
	out_areaNodeBounds.Clear();
	return NULL;
}

//-------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::copy(CDarkmodHidingSpotTree* p_out_otherTree)
{
	if (p_out_otherTree == NULL)
	{
		return false;
	}

	p_out_otherTree->Clear();

	TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
	while (p_areaCursor != NULL)
	{
		// Get or make area
		TDarkmodHidingSpotAreaNode* p_otherArea = p_out_otherTree->getArea (p_areaCursor->aasAreaIndex);
		if (p_otherArea == NULL)
		{
			p_otherArea = p_out_otherTree->insertArea(p_areaCursor->aasAreaIndex);
			if (p_otherArea == NULL)
			{
				return false;
			}
		}

		// Insert points
		for (int i = 0; i < p_areaCursor->spots.Num(); i++)
		{
			p_out_otherTree->insertHidingSpot
			(
				p_otherArea,
				p_areaCursor->spots[i]->goal,
				p_areaCursor->spots[i]->hidingSpotTypes,
				p_areaCursor->spots[i]->lightQuotient,
				p_areaCursor->spots[i]->qualityWithoutDistanceFactor,
				p_areaCursor->spots[i]->quality,
				-1.0 // No redundancy combination
			);
		}

		// Next area
		p_areaCursor = p_areaCursor->p_nextSibling;
	}

	return true;
}

//-------------------------------------------------------------------------
/* grayman #3857
bool CDarkmodHidingSpotTree::getOneNth(	unsigned int N, CDarkmodHidingSpotTree& out_otherTree)
{
	unsigned int numPointsMoved = 0;

	// Other tree should be empty
	out_otherTree.clear();

	// Test params
	if (N == 0)
	{
		return true;
	}

	// Split up our areas, as that is what human's would typically do, rather
	// than splitting up points within areas.
	if (numAreas > 1)
	{
		// Split number of areas
		unsigned int areaSplit = numAreas - (numAreas / N);

		TDarkmodHidingSpotAreaNode* p_areaCursor = p_firstArea;
		TDarkmodHidingSpotAreaNode* p_areaTrailer = NULL;

		unsigned int areaIndex = 0;
		while (areaIndex < areaSplit) 
		{
			p_areaTrailer = p_areaCursor;
			p_areaCursor = p_areaCursor->p_nextSibling;

			areaIndex ++;

			if (p_areaCursor == NULL)
			{
				// Bad area count in data structure
				DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Bad area count in data structure\r");
				return false;
			}
		}

		// Add areas to other list
		out_otherTree.numAreas = (numAreas - areaIndex);
		out_otherTree.p_firstArea = p_areaCursor;

		// Remove areas from this list
		if (p_areaTrailer != NULL)
		{
			p_areaTrailer->p_nextSibling = NULL;
			p_lastArea = p_areaTrailer;
		}
		else
		{
			p_lastArea = NULL;
			p_firstArea = NULL;
		}
		numAreas = areaIndex;
		
		// How many points were in the areas moved?
		TDarkmodHidingSpotAreaNode* p_countCursor = out_otherTree.p_firstArea;
		numPointsMoved = 0;
		while (p_countCursor != NULL)
		{
			numPointsMoved += p_countCursor->count;
			p_countCursor = p_countCursor->p_nextSibling;
		}
	}
	else if (numAreas == 0)
	{
		// Done, no hiding spots to split
		return true;
	}
	else
	{
		// Split points in the one and only area
		TDarkmodHidingSpotAreaNode* p_area = p_firstArea;
		if (p_area == NULL)
		{
			DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Serious: Area count != 0, but firstArea == NULL!\r");
			return false;
		}

		// Create new area in other tree
		TDarkmodHidingSpotAreaNode* p_otherArea = out_otherTree.insertArea(p_area->aasAreaIndex);
		if (p_otherArea == NULL)
		{
			DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Could not create area in other tree for copying.\r");
			return false;
		}

		int splitPointIndex = p_area->spots.Num() - (p_area->spots.Num() / N);
		//int pointIndex = 0;

		// Copy all points from [splitPointIndex...N] to the other tree
		for (int i = splitPointIndex; i < p_area->spots.Num(); i++)
		{
			p_otherArea->spots.Append(p_area->spots[i]);
		}

		// Now truncate the copied points from the source list
		p_area->spots.SetNum(splitPointIndex);
		
		p_otherArea->count = p_otherArea->spots.Num(); // TODO count is deprecated
		p_area->count = p_area->spots.Num(); // TODO count is deprecated

		// Set the overall counter
		numPointsMoved = p_otherArea->spots.Num();
	}

	// Set point totals in both list based on number of points moved
	numSpots -= numPointsMoved;
	out_otherTree.numSpots = numPointsMoved;

	// Done
	return true;
}
*/
//------------------------------------------------------------------------------------------

bool CDarkmodHidingSpotTree::sortForNewCenter(idVec3 center, float searchRadius)
{
	// Run through entire tree, and recalculate quality of each point given new distance from center
	unsigned int numSpots = getNumSpots();
	unsigned int spotIndex = 0;

	for (spotIndex = 0; spotIndex < numSpots; spotIndex ++)
	{
		darkModHidingSpot* p_spot = getNthSpot(spotIndex);
		
		if (p_spot != NULL)
		{
			float quality;

			// What is distance
			float distanceFromCenter = (p_spot->goal.origin - center).Length();

			// Perform distance fall-off
			if ((searchRadius > 0.0) && (p_spot->qualityWithoutDistanceFactor > 0.0))
			{
				float falloff = ((searchRadius - distanceFromCenter) / searchRadius);
				
				// Use power of 2 fallof
				quality =  p_spot->qualityWithoutDistanceFactor * falloff * falloff;
				if (quality < 0.0)
				{
					quality = 0.0;
				}
			}
			else
			{
				quality = 0.0;
			}

			// Update spot
			p_spot->quality = quality;
				
		}

	} // Next spot

	// Sort each area
	/*
	TDarkModHidingSpotTreeIterationHandle areaIterator;
	TDarkmodHidingSpotAreaNode* p_areaNode = getFirstArea (areaIterator);
	while (p_areaNode != NULL)
	{
		if (p_areaNode->p_firstSpot != NULL)
		{
			quicksortHidingSpotList (p_areaNode->p_firstSpot, p_areaNode->count);
			p_areaNode->bestSpotQuality = p_areaNode->p_firstSpot->spot.quality;
		}

		p_areaNode = getNextArea (areaIterator);
	}
	*/

	// Sort areas
	quicksortAreaList
	(
		p_firstArea,
		numAreas
	);

	// Find last area (ugh)
	TDarkmodHidingSpotAreaNode* p_cursor = p_firstArea;
	if (p_cursor != NULL)
	{
		p_lastArea = NULL;
	}
	else
	{
		while (p_cursor->p_nextSibling != NULL)
		{
				p_cursor = p_cursor->p_nextSibling;
		}
		p_lastArea = p_cursor;
	}

	return true;
}

//----------------------------------------------------------------------------------------

void CDarkmodHidingSpotTree::quicksortHidingSpotList
(
	darkModHidingSpot*& inout_p_firstNode,
	unsigned int numSpots
)
{
	/* If list is empty or only one node long , we are done
	if (inout_p_firstNode == NULL)
	{
		return;
	}
	else if (inout_p_firstNode->p_next == NULL)
	{
		return;
	}

	// We use median point as pivote
	darkModHidingSpot* p_pivot = inout_p_firstNode;
	for (unsigned int rideCount = 0; rideCount < (numSpots/2); rideCount ++)
	{
		p_pivot = p_pivot->p_next;
		if (p_pivot == NULL)
		{
			// Crap, list is broken, can't sort
			return;
		}
	}

	// Make two sub lists, those greater than pivot and those less than pivot.
	// We'll put equal into the greater list
	darkModHidingSpot* p_firstGreaterOrEqual = NULL;
	darkModHidingSpot* p_firstLess = NULL;
	unsigned int numGreaterOrEqual = 0;
	unsigned int numLess = 0;

	darkModHidingSpot* p_cursor = inout_p_firstNode;
	while (p_cursor != NULL)
	{
		if (p_cursor->spot.quality >= p_pivot->spot.quality)
		{
			p_cursor->p_next = p_firstGreaterOrEqual;
			p_firstGreaterOrEqual = p_cursor;
			numGreaterOrEqual ++;
		}	
		else
		{
			p_cursor->p_next = p_firstLess;
			p_firstLess = p_cursor;
			numLess ++;
		}

		p_cursor = p_cursor->p_next;
	}

	// Sort each sub list
	quicksortHidingSpotList (p_firstGreaterOrEqual, numGreaterOrEqual);
	quicksortHidingSpotList (p_firstLess, numLess);

	// Merge the sub lists, greater then lesser
	if (p_firstGreaterOrEqual != NULL)
	{
		p_cursor = p_firstGreaterOrEqual;
		while (p_cursor->p_next != NULL)
		{
			p_cursor = p_cursor->p_next;
		}

		// Join lists
		p_cursor->p_next = p_firstLess;

		// First is first in greater and equal list
		inout_p_firstNode = p_firstGreaterOrEqual;
	}
	else
	{
		inout_p_firstNode = p_firstLess;
	}

	// Done*/
	
}

//---------------------------------------------------------------------------------------------------

void CDarkmodHidingSpotTree::quicksortAreaList
(
	TDarkmodHidingSpotAreaNode*& inout_p_firstNode,
	unsigned int numAreas
)
{
	// If list is empty or only one node long , we are done
	if (inout_p_firstNode == NULL)
	{
		return;
	}
	else if (inout_p_firstNode->p_nextSibling == NULL)
	{
		return;
	}

	// We use median point as pivote
	TDarkmodHidingSpotAreaNode* p_pivot = inout_p_firstNode;
	for (unsigned int rideCount = 0; rideCount < (numAreas/2); rideCount ++)
	{
		p_pivot = p_pivot->p_nextSibling;
		if (p_pivot == NULL)
		{
			// Crap, list is broken, can't sort
			return;
		}
	}

	// Make two sub lists, those greater than pivot and those less than pivot.
	// We'll put equal into the greater list
	TDarkmodHidingSpotAreaNode* p_firstGreaterOrEqual = NULL;
	TDarkmodHidingSpotAreaNode* p_firstLess = NULL;
	unsigned int numGreaterOrEqual = 0;
	unsigned int numLess = 0;

	TDarkmodHidingSpotAreaNode* p_cursor = inout_p_firstNode;
	while (p_cursor != NULL)
	{
		if (p_cursor->bestSpotQuality >= p_pivot->bestSpotQuality)
		{
			p_cursor->p_nextSibling = p_firstGreaterOrEqual;
			p_firstGreaterOrEqual = p_cursor;
			numGreaterOrEqual ++;
		}	
		else
		{
			p_cursor->p_nextSibling = p_firstLess;
			p_firstLess = p_cursor;
			numLess ++;
		}

		p_cursor = p_cursor->p_nextSibling;
	}

	// Sort each sub list
	quicksortAreaList (p_firstGreaterOrEqual, numGreaterOrEqual);
	quicksortAreaList (p_firstLess, numLess);

	// Merge the sub lists, greater then lesser
	if (p_firstGreaterOrEqual != NULL)
	{
		p_cursor = p_firstGreaterOrEqual;
		while (p_cursor->p_nextSibling != NULL)
		{
			p_cursor = p_cursor->p_nextSibling;
		}

		// Join lists
		p_cursor->p_nextSibling = p_firstLess;

		// First is first in greater and equal list
		inout_p_firstNode = p_firstGreaterOrEqual;
	}
	else
	{
		inout_p_firstNode = p_firstLess;
	}
	// Done
}
