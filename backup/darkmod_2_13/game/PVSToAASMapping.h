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

#ifndef PVS_TO_AAS_MAPPING_H
#define PVS_TO_AAS_MAPPING_H

#include "../game/Light.h"

//------------------------------------------------------

typedef struct tagPVSToAASMappingNode
{
	tagPVSToAASMappingNode* p_next;

	int AASAreaIndex;

} PVSToAASMappingNode;

//------------------------------------------------------

class PVSToAASMapping
{
protected:

	// The map of PVS areas to AAS areas (one to many)
	// This is an array of linked lists, the offset into the array is
	// the PVS area number.  The list contains the indices of all the AAS
	// areas in that PVS area.
	int numPVSAreas;
	PVSToAASMappingNode** m_p_AASAreaIndicesPerPVSArea;

	// Which aas size name are we currently using
	idStr aasName;

	/*!
	* This method clears one mapping list
	*/
	void clearMappingList (PVSToAASMappingNode* p_header);

	/*!
	* Inserts an aas area index into the list for a pvs area
	* @return true on success
	* @return false on failure
	*/
	bool insertAASAreaIntoPVSAreaMapping (int aasAreaIndex, int pvsAreaIndex);

public:
	PVSToAASMapping(void);
	virtual ~PVSToAASMapping(void);

	/*!
	* This method clears the currently held mapping
	*/
	void clear();

	/*!
	* This method builds a mapping which allows the AAS areas to be identified
	* for a given PVS area. Building this mapping may take some time, so it should
	* be kept around for re-use.
	*
	* @param in_aasName: The name of the aas system to use.
	*
	* @return true on success
	* @return false on failure
	*/
	bool buildMappings(idStr in_aasName);

	/**
	* This method retrievs the name of the AAS which was used in the mapping
	*/
	idStr getAASName();

	/*!
	* This method gets the aas area index list for a particular pvs area
	*
	* @param pvsAreaIndex The index of the PVS area being querried
	* 
	* @return Pointer to the first node in the list of aas area indices for this pvs area
	* @return NULL If the requested mapping is empty or the pvs area requested is out of bounds

	*/
	PVSToAASMappingNode* getAASAreasForPVSArea (int pvsAreaIndex);

   /*!
   * Given a PVS area index, this retrieves a list of AAS area indices of AAS areas that it
   * contains. (More Doom3 familiar style container, but less efficient)
   */
   void getAASAreasForPVSArea(int pvsAreaIndex, idList<int>& out_aasAreaIndices);

	/**
     * greebo: Draws the AAS areas per PVS area.
	 *
	 * @lifetime: How long the debug draws should be visible (in msecs.)
	 */
	void DebugShowMappings(int lifetime);
};

#endif
