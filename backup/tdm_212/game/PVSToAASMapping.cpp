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



#include "PVSToAASMapping.h"
#include "DarkModGlobals.h"
#include "Pvs.h"
#include "renderer/frontend/RenderWorld.h"
#include "Intersection.h"

//----------------------------------------------------------------------------

PVSToAASMapping::PVSToAASMapping(void)
{
	// No allocated mapping
	aasName.Clear();
	numPVSAreas = 0;
	m_p_AASAreaIndicesPerPVSArea = NULL;
}

//----------------------------------------------------------------------------

PVSToAASMapping::~PVSToAASMapping(void)
{
	clear();
}


//----------------------------------------------------------------------------

void PVSToAASMapping::clearMappingList (PVSToAASMappingNode* p_header)
{
	if (p_header == NULL)
	{
		return;
	}

	// Ride the list and delete each node
	PVSToAASMappingNode* p_cursor = p_header;
	PVSToAASMappingNode* p_temp = NULL;
	while (p_cursor != NULL)
	{
		p_temp = p_cursor->p_next;
		delete p_cursor;
		p_cursor = p_temp;
	}
	
}

//----------------------------------------------------------------------------

void PVSToAASMapping::clear()
{
	if (m_p_AASAreaIndicesPerPVSArea != NULL)
	{
		// Clear each mapping list
		for (int i = 0; i < numPVSAreas; i ++)
		{
			clearMappingList (m_p_AASAreaIndicesPerPVSArea[i]);
		}

		// Done with mapping list header pointers
		delete[] m_p_AASAreaIndicesPerPVSArea;

		// Mapping list header pointers unallocated
		m_p_AASAreaIndicesPerPVSArea = NULL;
	}

	numPVSAreas = 0;
	aasName.Clear();
}

//----------------------------------------------------------------------------

bool PVSToAASMapping::buildMappings(idStr in_aasName)
{
	// If we already map this one, we are done
	if (aasName == in_aasName)
	{
		return true;
	}

	// Clear any previous mapping
	clear();

	// Get the aas 
	idAAS* p_aas = gameLocal.GetAAS (in_aasName);
	if (p_aas == NULL)
	{
		DM_LOG (LC_AI, LT_ERROR)LOGSTRING("No aas with name '%s' exists for this map, AI will not be able to locate darkness...\r", in_aasName.c_str());
		return false;
	}

	// Get number of PVS areas
	numPVSAreas = gameRenderWorld->NumAreas();
	if (numPVSAreas > 0)
	{
		// Allocate areas
		m_p_AASAreaIndicesPerPVSArea = new PVSToAASMappingNode*[numPVSAreas];
		if (m_p_AASAreaIndicesPerPVSArea == NULL)
		{
			numPVSAreas = 0;
			DM_LOG (LC_AI, LT_ERROR)LOGSTRING("Failed to alloate mapping table header pointers\r");
			return false;
		}
	}

	// All start empty
	for (int i = 0; i < numPVSAreas; i ++)
	{
		m_p_AASAreaIndicesPerPVSArea[i] = NULL;
	}

	// Iterate AAS areas and add each one to the appropriate PVS area
//	bool b_worked = true;
	int numAASAreas = p_aas->GetNumAreas();
	for (int aasAreaIndex = 0; aasAreaIndex  < numAASAreas; aasAreaIndex ++)
	{
		// Get AAS area center
		idVec3 aasAreaCenter = p_aas->AreaCenter (aasAreaIndex);

		// What PVS area does it go in?
		int pvsAreaIndex = gameLocal.pvs.GetPVSArea (aasAreaCenter);
		if (!insertAASAreaIntoPVSAreaMapping (aasAreaIndex, pvsAreaIndex))
		{
			clear();
			return false;
		}
	}

	// Remember file
	aasName = in_aasName;

	// Log success
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING
	(
		"Successfully set up mapping of %d PVS areas to %d AAS areas\r", 
		numPVSAreas,
		numAASAreas
	);

	// Done
	return true;
}

//---------------------------------------------------------------------------

idStr PVSToAASMapping::getAASName()
{
	return aasName;
}

//---------------------------------------------------------------------------

bool PVSToAASMapping::insertAASAreaIntoPVSAreaMapping (int aasAreaIndex, int pvsAreaIndex)
{

	if (pvsAreaIndex >= numPVSAreas)
	{
		// Log error
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING 
		(
			"AAS area %d falls in PVS area %d which is beyond supposed PVS area count of %d\r", 
			aasAreaIndex, 
			pvsAreaIndex,
			numPVSAreas
		);
		return false;
	}
	else if (pvsAreaIndex < 0)
	{
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING 
		(
			"AAS area %d falls in no PVS area, left out of mapping\r", 
			aasAreaIndex
		);
	}
	else 
	{
		// Allocate node
		PVSToAASMappingNode* p_node = new PVSToAASMappingNode;
		if (p_node == NULL)
		{
			DM_LOG(LC_AI, LT_ERROR)LOGSTRING 
			(
				"Failed to allocate mapping node\r"
			);
			return false;
		}
		else
		{
			p_node->AASAreaIndex = aasAreaIndex;
		}

		// Add to front
		if (m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] == NULL)
		{
			p_node->p_next = NULL;
			m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] = p_node;
		}
		else
		{
			p_node->p_next = m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex];
			m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] = p_node;
		}

	}

	// Success
	return true;

}

//----------------------------------------------------------------------------

PVSToAASMappingNode* PVSToAASMapping::getAASAreasForPVSArea (int pvsAreaIndex)
{
	if ((pvsAreaIndex < 0) || (pvsAreaIndex >= numPVSAreas))
	{
		return NULL;
	}
	else
	{
		return m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex];
	}
}

//----------------------------------------------------------------------------

void PVSToAASMapping::getAASAreasForPVSArea(int pvsAreaIndex, idList<int>& out_aasAreaIndices)
{
	out_aasAreaIndices.Clear();

	PVSToAASMappingNode* p_node = getAASAreasForPVSArea (pvsAreaIndex);

	while (p_node != NULL)
	{
		out_aasAreaIndices.AddUnique (p_node->AASAreaIndex);
		p_node = p_node->p_next;
	}

	// Done
}

void PVSToAASMapping::DebugShowMappings(int lifetime)
{
	if (numPVSAreas <= 0)
	{
		gameLocal.Printf("Cannot draw mappings, no PVS areas available.\n");
	}

	idAAS* aas = gameLocal.GetAAS(aasName.c_str());

	idMat3 playerViewMatrix(gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

	idVec4 color(1,1,1,1);

	for (int i = 0; i < numPVSAreas; i++)
	{
		PVSToAASMappingNode* node = m_p_AASAreaIndicesPerPVSArea[i];

		while (node != NULL)
		{
			int aasArea = node->AASAreaIndex;
			idBounds areaBounds = aas->GetAreaBounds(aasArea);
			idVec3 areaCenter = aas->AreaCenter(aasArea);
			// angua: only draw areas near the player, no need to see them at the other end of the map
			if ((areaCenter - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast() < 1000)
			{
				gameRenderWorld->DebugText(va("%d", aasArea), areaCenter, 0.2f, color, playerViewMatrix, 1, lifetime);
				gameRenderWorld->DebugBox(color, idBox(areaBounds), lifetime);
			}

			node = node->p_next;
		}

		color.x = gameLocal.random.RandomFloat() + 0.1f;
		color.y = gameLocal.random.RandomFloat() + 0.1f;
		color.z = gameLocal.random.RandomFloat() + 0.1f;
	}
}
