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
* Implementation file for the darkModLAS class
* SophisticatedZombie, using SparHawk's lightgem code
*
*/

#include "precompiled.h"
#pragma hdrstop



#include "darkModLAS.h"
#include "Pvs.h"
#include "renderer/frontend/RenderWorld.h"
#include "DarkModGlobals.h"
#include "Intersection.h"
#include "TimerManager.h"


//----------------------------------------------------------------------------

// Global instance of LAS
darkModLAS LAS;

//----------------------------------------------------------------------------

bool darkModLAS::Disabled() const
{
	return g_lightQuotientAlgo.GetInteger() == 2;
}

/*!
* Constructor
*/
darkModLAS::darkModLAS()
{
	// No areas
	m_numAreas = 0;
	m_pp_areaLightLists = NULL;

	INIT_TIMER_HANDLE(queryLightingAlongLineTimer);
}

/*!
* Destructor
*/
darkModLAS::~darkModLAS()
{
	shutDown();
}


void darkModLAS::Save(idSaveGame *savefile ) const 
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
		return;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Saving LAS.\r");

	savefile->WriteUnsignedInt(m_updateFrameIndex);

	int counter;

	for (int i = 0; i < m_numAreas + 1; i ++)
	{

		// number of entries in this area
		if (m_pp_areaLightLists[i] != NULL)
		{
			// angua: usually, the head of the list doesn't contain any data and works as a fixed starting point of the list
			// Num() therefore returns the number of elements after the head
			// in the LAS, the head is already the first light in the list, so we have to increase the counter by 1
			counter = m_pp_areaLightLists[i]->Num() + 1;
		}
		else
		{
			counter = 0;
		}
		savefile->WriteInt(counter);

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Area %d has %d lights.\r",i, counter);
	
		// save entries
		idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[i];
		while (p_cursor != NULL)
		{
			darkModLightRecord_t* p_thisLASLight = static_cast<darkModLightRecord_t*>(p_cursor->Owner());

			savefile->WriteObject(p_thisLASLight->p_idLight);
			savefile->WriteInt(p_thisLASLight->areaIndex);
			savefile->WriteVec3(p_thisLASLight->lastWorldPos);
			savefile->WriteUnsignedInt(p_thisLASLight->lastFrameUpdated);

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Saving light '%s' in area %d.\r", p_thisLASLight->p_idLight->name.c_str(), i);

			p_cursor = p_cursor->NextNode();
		}
	}
}

void darkModLAS::Restore(idRestoreGame *savefile)
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
	{
		shutDown();
		return;
	}

	// angua: initialize is called on reloading before Restore()
	// num areas already initialised
	// m_pp_areaLightLists already created and empty
	// pvsToAASMappingTable already initialised

	DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Restoring LAS.\r");

	savefile->ReadUnsignedInt(m_updateFrameIndex);

	int counter;
	for (int i = 0; i < m_numAreas + 1; i ++)
	{
		savefile->ReadInt(counter);

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Area %d has %d lights.\r", i, counter);

		for (int j = 0; j < counter; j ++)
		{
			darkModLightRecord_t* p_record = new darkModLightRecord_t;

			savefile->ReadObject(reinterpret_cast<idClass *&>(p_record->p_idLight));
			savefile->ReadInt(p_record->areaIndex);
			savefile->ReadVec3(p_record->lastWorldPos);
			savefile->ReadUnsignedInt(p_record->lastFrameUpdated);

			if (m_pp_areaLightLists[i] != NULL)
			{
				// list already has entries
				idLinkList<darkModLightRecord_t>* p_node = new idLinkList<darkModLightRecord_t>;
				p_node->SetOwner(p_record);
				p_node->AddToEnd (*(m_pp_areaLightLists[i]));

				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light was added to area %d at end of list, area now has %d lights.\r", i, m_pp_areaLightLists[i]->Num() + 1);
			}
			else
			{
				// First in area
				idLinkList<darkModLightRecord_t>* p_first = new idLinkList<darkModLightRecord_t>;
				p_first->SetOwner(p_record);
				if (p_first == NULL)
				{
					DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Failed to create node for LASLight record.\r");
					return;
				}
				else
				{
					m_pp_areaLightLists[i] = p_first;
				}

				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light was added to area %d as first in list.\r", i);
			}
		}
	}
}

//----------------------------------------------------------------------------

inline bool darkModLAS::moveLightBetweenAreas (darkModLightRecord_t* p_LASLight, int oldAreaNum, int newAreaNum )
{
	assert(oldAreaNum >= 0 && oldAreaNum < m_numAreas + 1);
	assert(newAreaNum >= 0 && newAreaNum < m_numAreas + 1);
	

	// Remove from old area
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[oldAreaNum];
	
	while (p_cursor != NULL)
	{
		darkModLightRecord_t* p_thisLASLight = static_cast<darkModLightRecord_t*>(p_cursor->Owner());

		if (p_thisLASLight == p_LASLight)
		{
			// greebo: Check if this is the list head, 
			// we need to update the lightlist head pointer in that case
			if (p_cursor->ListHead() == p_cursor)
			{
				// NextNode() will return NULL if this is the only light in this list
				m_pp_areaLightLists[oldAreaNum] = p_cursor->NextNode();
			}

			// Remove this node from its list
			p_cursor->RemoveHeadsafe();

			break;
		}
		else
		{
			p_cursor = p_cursor->NextNode();
		}
	}

	// Test for not found
	if (p_cursor == NULL)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Failed to remove LAS light record from list for area %d.\r", oldAreaNum);

		// Failed
		return false;
	}

	// Add to new area
	if (m_pp_areaLightLists[newAreaNum] != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Added to existing area %d\r", newAreaNum);
		p_cursor->AddToEnd (*m_pp_areaLightLists[newAreaNum]);
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Created new area %d\r", newAreaNum);
		m_pp_areaLightLists[newAreaNum] = p_cursor;
	}
	
	// Update the area index on the light after the move
	p_LASLight->areaIndex = newAreaNum;
	p_LASLight->p_idLight->LASAreaIndex = newAreaNum;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Area %d has now %d elements\r", oldAreaNum, 
		m_pp_areaLightLists[oldAreaNum] ? m_pp_areaLightLists[oldAreaNum]->Num() : 0);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Area %d has now %d elements\r", newAreaNum, m_pp_areaLightLists[newAreaNum]->Num());

	// Done
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light '%s' was moved from area %d to area %d.\r", p_cursor->Owner()->p_idLight->name.c_str(), oldAreaNum, newAreaNum);

	return true;
	

}

//----------------------------------------------------------------------------

// grayman #2853 - generalize tracing from a location to a light source

bool darkModLAS::traceLightPath( idVec3 from, idVec3 to, idEntity* ignore, idLight* light ) // grayman #3584
{
	trace_t trace;

	bool results = false; // didn't complete the path

	// grayman #3584 - if this light has a lightholder, find the bind chain

	while ( true )
	{
		gameLocal.clip.TracePoint( trace, from, to, CONTENTS_OPAQUE, ignore );
		if ( cv_las_showtraces.GetBool() )
		{
			gameRenderWorld->DebugArrow(
					trace.fraction == 1 ? colorGreen : colorRed, 
					trace.fraction == 1 ? to : trace.endpos, 
					from, 1, 1000);
		}
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("TraceFraction: %f\r", trace.fraction);
		if ( trace.fraction == 1.0f )
		{
			results = true; // completed the path
			break;
		}

		// grayman #2902 - prevent infinite loops where we get stuck inside the intersection of 2 entities

		if ( trace.fraction < VECTOR_EPSILON )
		{
			break;
		}

		// grayman #3584 - end the trace if we hit the world

		if ( trace.c.entityNum == ENTITYNUM_WORLD )
		{
			break;
		}

		// End the trace if the entity hit casts shadows and is not part of the light's lightholder

		idEntity* entHit = gameLocal.entities[trace.c.entityNum];

		if ( entHit->CastsShadows() )
		{
			// grayman #3584 - continue the trace if we hit the light or an entity that is part of the light's lightholder
			bool hitLightHolder = false;
			for ( idEntity* bindMaster = light ; bindMaster ; bindMaster = bindMaster->GetBindMaster() )
			{
				if ( entHit == bindMaster )
				{
					hitLightHolder = true;
					break;
				}
			}

			if ( !hitLightHolder )
			{
				break;
			}
		}

		// Continue the trace from the struck point

		from = trace.endpos;
		ignore = entHit; // ignore the entity we struck
	}

	return results;
}

//----------------------------------------------------------------------------

void darkModLAS::accumulateEffectOfLightsInArea 
( 
	float& inout_totalIllumination,
	int areaIndex, 
	idVec3 testPoint1,
	idVec3 testPoint2,
	idEntity* p_ignoredEntity,
	bool b_useShadows
)
{
	/*
	* Note most of this code is adopted from SparHawk's lightgem alpha code.
	* And then heavily modified by grayman.
	*/

	// Set up target segment: Origin and Delta
	idVec3 vTargetSeg[LSG_COUNT];
	vTargetSeg[0] = testPoint1;
	vTargetSeg[1] = testPoint2 - testPoint1;

	if (cv_las_showtraces.GetBool())
	{
		gameRenderWorld->DebugArrow(colorBlue, testPoint1, testPoint2, 2, 1000);
	}

	assert( ( areaIndex >= 0 ) && ( areaIndex < m_numAreas ) );
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];

	/* grayman #3843 - OOOPS! This is getting added once for up to 4 area passes.
	// Moved upward before the looping starts.
	// grayman #3132 - factor in the ambient light, if any

	inout_totalIllumination += gameLocal.GetAmbientIllumination(testPoint1);
	*/
	// Iterate lights in this area
	while (p_cursor != NULL)
	{
		// Get the light to be tested
		darkModLightRecord_t* p_LASLight = p_cursor->Owner();

		if (p_LASLight == NULL)
		{
			// Log error
			DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("LASLight record in area %d is NULL.\r", areaIndex);

			// Return what we have so far
			return;
		}

		idLight* light = p_LASLight->p_idLight;

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"accumulateEffectOfLightsInArea (area = %d): accounting for light '%s'.\r", 
			areaIndex,
			light->name.c_str()
		);

		// grayman #2853 - If the light's OFF, ignore it

		if ( light->GetLightLevel() == 0 )
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
			continue;
		}

		// grayman #3902 - ignore blend and fog lights
		// SteveL  #4128 - let mapper use lights that ai can't see.
		if ( light->IsBlend() || light->IsFog() || !light->IsSeenByAI() )
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
			continue;
		}

		/*!
		// What follows in the rest of this method is mostly Sparkhawk's lightgem code.
		// grayman #3584 - though by this point, it probably no longer looks like that
		// code, given the number of things that needed to be fixed.
		*/

		idVec3 vLightCone[ELC_COUNT]; // Holds data on the light shape (point ellipsoid or projected cone).
		idVec3 vLight; // The real origin of the light (origin + offset).
		EIntersection inter;
		idVec3 vResult[2]; // If there's an intersection, [0] holds one point, [1] holds a second
		bool inside[LSG_COUNT]; // inside[0] is true if testPoint1 is inside the light volume, inside[1] ditto for testPoint2
		bool b_excludeLight = false;
		idVec3 p1, p2, p3; // test points for testing visibility to light source
		idVec3 p_illumination; // point where we determine illumination

		if ( light->IsPointlight() )
		{
			light->GetLightCone
			(
				vLightCone[ELL_ORIGIN], 
				vLightCone[ELA_AXIS], 
				vLightCone[ELA_CENTER]
			);

			// If this is a centerlight we have to move the origin from the original origin to where the
			// center of the light is supposed to be.
			// Centerlight means that the center of the ellipsoid is not the same as the origin. It has to
			// be adjusted because if it casts shadows we have to trace to it, and in this case the light
			// might be inside geometry and would be reported as not being visible even though it casts
			// a visible light outside the geometry it is embedded in. If it is not a centerlight and has
			// cast shadows enabled, it wouldn't cast any light at all in such a case because it would
			// be blocked by the geometry.

			vLight = vLightCone[ELL_ORIGIN] + vLightCone[ELA_CENTER];

			// grayman #3584 - IntersectLineEllipsoid() provides no information on whether
			// the line segment ends are inside or outside the ellipsoid. Let's use
			// IntersectLinesegmentLightEllipsoid() to get that information.

			inter = IntersectLinesegmentLightEllipsoid(	vTargetSeg, vLightCone, vResult, inside	);
			//inter = IntersectLineEllipsoid(	vTargetSeg, vLightCone, vResult	);

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLinesegmentLightEllipsoid() returned %u\r", inter);
		}
		else // projected light
		{
			light->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
			inter = IntersectLineLightCone(vTargetSeg, vLightCone, vResult, inside);
			vLight = vLightCone[ELC_ORIGIN]; // grayman #3524
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLineLightCone returned %u\r", inter);
		}

		// The line intersection returns one of four states. Either the line is entirely inside
		// the light cone (inter = INTERSECT_NONE), it's passing through the lightcone (inter = INTERSECT_FULL), the line
		// is not passing through which means that the test line is fully outside (inter = INTERSECT_OUTSIDE), or the line
		// is touching the cone in exactly one point (inter = INTERSECT_PARTIAL).

		if ( inter == INTERSECT_OUTSIDE ) // grayman #3584 - exclude the uninteresting case
		//if ( ( inter == INTERSECT_PARTIAL ) || ( inter == INTERSECT_OUTSIDE) ) // grayman #2853 - exclude the two uninteresting cases
		{
			b_excludeLight = true;
		}
		else
		{
			// grayman #3584 - the two points chosen for raytracing should be inside the light cone.
			// There are a few cases:

			// 1 - INTERSECT_NONE - Both ends of the line segment (testPoint1 and testPoint2) are inside the
			//     light cone, and should be tested for both visibility and illumination.
			//     Determine a third point, testPoint3, which is the midpoint between them.
			//
			//     a - If testPoint1 is visible from the light origin, use testPoint3 to determine illumination.
			//     b - If testPoint1 is not visible, move on to testPoint2. If testPoint2
			//         is visible from the light origin, use testPoint3 to determine illumination.
			//     c - If neither testPoint1 or testPoint2 is visible, move on to testPoint3.
			//         If testPoint3 is visible from the light origin, use it to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//
			// 2 - INTERSECT_PARTIAL - One end of the line segment (either testPoint1 or testPoint2) is inside the
			//     light cone, and the second point is the point of intersection with the line cone
			//     that lies on the line segment. Make the third point, testPoint3, the midpoint between the
			//     first two.
			//
			//     a - Let p1 be whichever of testPoint1 and testPoint2 is inside the light cone. It it's
			//         visible from the light origin, use it to determine illumination.
			//     b - If 'a' fails, move on to the point of intersection. If it's visible from the light
			//         origin, use testPoint3 to determine illumination.
			//     c - If 'b' fails, move on to testPoint3. If testPoint3 is visible from the light origin, use it
			//         to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//
			// 3 - INTERSECT_FULL - Both ends of the line segment lie outside the light cone. Treat the first intersection
			//     point as testPoint1, the second intersection point as testPoint2, and the midpoint
			//     between them as testPoint3.
			//
			//     a - If testPoint1 is visible from the light origin, use testPoint3 to determine illumination.
			//     b - If testPoint1 is not visible, move on to testPoint2. If testPoint2
			//         is visible from the light origin, use testPoint3 to determine illumination.
			//     c - If neither testPoint1 or testPoint2 is visible, move on to testPoint3.
			//         If testPoint3 is visible from the light origin, use it to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//

			if (b_useShadows && light->CastsShadow()) 
			{
				// grayman #2853 - If the trace hits something before completing, that thing has to be checked to see if
				// it casts shadows. If it doesn't, then it has to be ignored and the trace must be run again from the struck
				// point to the end. This has to be done iteratively, since there might be several non-shadow-casting entities
				// in the way. For example, a candleflame in a candle in a chandelier, and the latter two are marked with 'noshadows'.
				// Light holders must also be taken into account, since the holder entity in DR can be marked 'noshadows', which
				// also applies to the candle holding the flame.

				bool lightReaches;

				if ( inter == INTERSECT_NONE ) // the line segment is entirely inside the light volume
				{
					p3 = (testPoint1 + testPoint2)/2.0f;
					lightReaches = traceLightPath( testPoint1, vLight, p_ignoredEntity, light );
					if ( !lightReaches )
					{
						lightReaches = traceLightPath( testPoint2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
					p_illumination = p3;
				}
				else if ( ( inter == INTERSECT_PARTIAL ) && ( inside[0] || inside[1] ) ) // one line end inside, one outside
				{
					// either testPoint1 or testPoint2 is inside the ellipsoid
					if ( inside[0] )
					{
						p1 = testPoint1;
					}
					else
					{
						p1 = testPoint2;
					}

					p2 = vResult[0]; // the single point of intersection
					p3 = (p1 + p2)/2.0f;
					lightReaches = traceLightPath( p1, vLight, p_ignoredEntity, light );
					if ( lightReaches )
					{
						p_illumination = p1;
					}
					else
					{
						p_illumination = p3;
						lightReaches = traceLightPath( p2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
				}
				else if ( inter == INTERSECT_PARTIAL ) // both line ends outside, line touches volume at one intersection point
				{
					// Since the only point we can test is at the edge of the light volume,
					// we can safely assume the illumination there is zero. No need to test
					// LOS to the light origin or determine brightness.
					lightReaches = false;
				}
				else // INTERSECT_FULL
				{
					p1 = vResult[0]; // the first point of intersection
					p2 = vResult[1]; // the second point of intersection
					p3 = (p1 + p2)/2.0f;
					p_illumination = p3;
					lightReaches = traceLightPath( p1, vLight, p_ignoredEntity, light );
					if ( !lightReaches )
					{
						lightReaches = traceLightPath( p2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
				}
				
				b_excludeLight = !lightReaches;

				// end of new code

				/* old code

				trace_t trace;
				gameLocal.clip.TracePoint(trace, testPoint1, p_LASLight->lastWorldPos, CONTENTS_OPAQUE, p_ignoredEntity);
				if ( cv_las_showtraces.GetBool() )
				{
					gameRenderWorld->DebugArrow(
							trace.fraction == 1 ? colorGreen : colorRed, 
							trace.fraction == 1 ? testPoint1 : trace.endpos, 
							p_LASLight->lastWorldPos, 1, 1000);
				}
				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("TraceFraction: %f\r", trace.fraction);
				if ( trace.fraction < 1.0f )
				{
					gameLocal.clip.TracePoint (trace, testPoint2, p_LASLight->lastWorldPos, CONTENTS_OPAQUE, p_ignoredEntity);
					if (cv_las_showtraces.GetBool())
					{
						gameRenderWorld->DebugArrow(
							trace.fraction == 1 ? colorGreen : colorRed, 
							trace.fraction == 1 ? testPoint2 : trace.endpos, 
							p_LASLight->lastWorldPos, 1, 1000);
					}
					DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("TraceFraction: %f\r", trace.fraction);
					if ( trace.fraction < 1.0f )
					{
						DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light [%s]: test point is in a shadow of the light\r", light->name.c_str());
						b_excludeLight = true;
					}
				}
				*/
			}
			else // no shadows, so assume visibility between the light origin and the point of illumination
			{
				if ( inter == INTERSECT_NONE ) // the line segment is entirely inside the light volume
				{
					p_illumination = (testPoint1 + testPoint2)/2.0f;
				}
				else if ( ( inter == INTERSECT_PARTIAL ) && ( inside[0] || inside[1] ) ) // one line end inside, one outside
				{
					// either testPoint1 or testPoint2 is inside the ellipsoid
					if ( inside[0] )
					{
						p_illumination = testPoint1;
					}
					else
					{
						p_illumination = testPoint2;
					}
				}
				else if ( inter == INTERSECT_PARTIAL ) // both line ends outside, line touches volume at one intersection point
				{
					b_excludeLight = true; // not interested in this case, since illumination is zero at the volume edges
				}
				else // INTERSECT_FULL
				{
					p_illumination = (vResult[0] + vResult[1])/2.0f; // halfway between the points of intersection
				}
			}
		}

		// Process light if not excluded
		if (!b_excludeLight)
		{
			// Compute illumination value
			// We want the illumination at p_illumination.

			float fx, fy;

			if ( light->IsPointlight() )
			{
				fx = p_illumination.x - vLight.x;
				fy = p_illumination.y - vLight.y;

				// ELA_AXIS contains the radii [x,y,z]
			}
			else // projected light
			{
				// p_illumination needs to be relative to the vector
				// from the light origin to the light target. Since the
				// original code assumed that vector pointed down along
				// the z axis, we'll rotate the target vector to that axis
				// and apply the same rotation to p_illumination so it stays
				// relative.

				// Re-get the light cone parameters, some of which were clobbered in IntersectLineLightCone().
				light->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
				idVec3 target = vLightCone[ELA_TARGET]; // direction of light cone, already relative to vLight

				// TODO: need to map p_illumination[x,y] to p_illumination[right,up]
				// then right is the new x and up is the new y

				if ( ( target.x == 0 ) && ( target.y == 0 ) ) // transform only if 'target' is not already on the z axis
				{
					fx = p_illumination.x - vLight.x;
					fy = p_illumination.y - vLight.y;
				}
				else
				{
					idVec3 p = p_illumination - vLight; // p is now p_illumination relative to the light origin

					// Matrices and steps are from http://inside.mines.edu/fs_home/gmurray/ArbitraryAxisRotation/

					// rotate 'target' to XY plane
					idVec2 target2 = target.ToVec2();
					float d = target2.LengthFast();
					float a = target.x/d;
					float b = target.y/d;
					idMat4 T1( idVec4(  a, b, 0, 0 ),
							   idVec4( -b, a, 0, 0 ),
							   idVec4(  0, 0, 1, 0 ),
							   idVec4(  0, 0, 0, 1 ) );
					idVec3 target_xy = T1*target; // the 'target' vector rotated to the XY plane

					// rotate 'target_xy' to the Z axis
					float c = target.LengthFast();
					float e = target.z/c;
					float f = d/c;
					idMat4 T2( idVec4(  e, 0, -f, 0 ),
							   idVec4(  0, 1,  0, 0 ),
							   idVec4(  f, 0,  e, 0 ),
							   idVec4(  0, 0,  0, 1 ) );
					idVec3 target_z = -(T2*target_xy);

					// apply T1 and T2 to p
					idVec3 p_z = -(T2*(T1*p));
					fx = p_z.x;
					fy = p_z.y;
				}
			}


/*			int index;
			if (vResult[0].z < vResult[1].z)
			{
				index = 0;
			}
			else
			{
				index = 1;
			}

			if (vResult[index].z < testPoint1.z)
			{
				fx = testPoint1.x;
				fy = testPoint1.y;
			}
			else
			{
				fx = vResult[index].x;
				fy = vResult[index].y;
			}
*/
			inout_totalIllumination += light->GetDistanceColor(	(p_illumination - vLight).LengthFast(),fx,fy );
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
			(
				"%s in x/y: %f/%f   Distance: %f/%f   Brightness: %f\r",
				light->name.c_str(), 
				fx, 
				fy, 
				(p_illumination - vLight).LengthFast(), 
				light->m_MaxLightRadius,
				inout_totalIllumination
			);
		}

		// If total illumination is 1.0 or greater, we are done
		if (inout_totalIllumination >= 1.0f)
		{
			// Exit early as it's really really bright as is
			p_cursor = NULL;
		}
		else
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
		}
	}
}

void darkModLAS::accumulateEffectOfLightsInArea2 
( 
	float& inout_totalIllumination,
	int areaIndex, 
	idBox box,
	idEntity* p_ignoredEntity,
	bool b_useShadows
)
{
	/*
	* Note most of this code is adopted from SparHawk's lightgem alpha code.
	* And then heavily modified by grayman.
	*/

	assert( ( areaIndex >= 0 ) && ( areaIndex < m_numAreas ) );
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];

	/* grayman #3843 - OOOPS! This is getting added once for up to 4 area passes.
	// Moved upward before the looping starts.
	// grayman #3132 - factor in the ambient light, if any

	inout_totalIllumination += gameLocal.GetAmbientIllumination(box.GetCenter());
	*/
	
	idVec3 verts[8];
	box.GetVerts(verts);

	// Iterate lights in this area
	while (p_cursor != NULL)
	{
		// Get the light to be tested
		darkModLightRecord_t* p_LASLight = p_cursor->Owner();

		if (p_LASLight == NULL)
		{
			// Log error
			DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("LASLight record in area %d is NULL.\r", areaIndex);

			// Return what we have so far
			return;
		}

		idLight* light = p_LASLight->p_idLight;

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"accumulateEffectOfLightsInArea2 (area = %d): accounting for light '%s'.\r", 
			areaIndex,
			light->name.c_str()
		);

		// grayman #2853 - If the light's OFF, ignore it.
		// grayman #3584 - Also ignore if it's an ambient light. Illumination
		// has already been collected from ambient lights.

		if ( ( light->GetLightLevel() == 0 ) || light->IsAmbient() )
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
			continue;
		}

		// grayman #3902 - ignore blend and fog lights
		// SteveL  #4128 - let mapper use lights that ai can't see too.
		if ( light->IsBlend() || light->IsFog() || !light->IsSeenByAI() )
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
			continue;
		}



		// What follows in the rest of this method is mostly Sparkhawk's lightgem code.
		// grayman #3584 - Though by this point, it probably no longer looks like that
		// code, given the number of things that needed to be fixed.

		idVec3 vLightCone[ELC_COUNT]; // Holds data on the light shape (point ellipsoid or projected cone).
		idVec3 vLight; // The real origin of the light (origin + offset).
		EIntersection inter;
		idVec3 vResult[2]; // If there's an intersection, [0] holds one point, [1] holds a second
		bool inside[LSG_COUNT]; // inside[0] is true if testPoint1 is inside the light volume, inside[1] ditto for testPoint2
		bool b_excludeLight = false;
		idVec3 p1, p2, p3; // test points for testing visibility to light source
		idVec3 p_illumination; // point where we determine illumination

		if ( light->IsPointlight() )
		{
			light->GetLightCone
			(
				vLightCone[ELL_ORIGIN], 
				vLightCone[ELA_AXIS], 
				vLightCone[ELA_CENTER]
			);
			vLight = vLightCone[ELL_ORIGIN] + vLightCone[ELA_CENTER];
		}
		else // projected light
		{
			light->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
			vLight = vLightCone[ELC_ORIGIN]; // grayman #3524
		}

		// Set up target segment: Origin and Delta
		idVec3 vTargetSeg[LSG_COUNT];

		// grayman #3584 - for each corner of the bounding box, determine which 2
		// are closest to the light center. Use those 2 corners as the ends of the
		// line segment to be tested.

		idVec3 testPoint1, testPoint2, tempPoint, p;
		float shortestDistanceSqr(idMath::INFINITY);
		float nextShortestDistanceSqr(idMath::INFINITY);
		float d1, d2;

		for ( int i = 0 ; i < 8 ; i++)
		{
			p = verts[i];
			d1 = (p - vLight).LengthSqr();
			if ( d1 < shortestDistanceSqr )
			{
				d2 = shortestDistanceSqr;
				tempPoint = testPoint1;
				testPoint1 = p;
				shortestDistanceSqr = d1;
				if ( d2 < nextShortestDistanceSqr )
				{
					testPoint2 = tempPoint;
					nextShortestDistanceSqr = d2;
				}
			}
			else if ( d1 < nextShortestDistanceSqr )
			{
				testPoint2 = p;
				nextShortestDistanceSqr = d1;
			}
		}

		// testPoint1 -> testPoint2 is the boundary edge closest to the light center

		vTargetSeg[0] = testPoint1;
		vTargetSeg[1] = testPoint2 - testPoint1;

		if (cv_las_showtraces.GetBool())
		{
			gameRenderWorld->DebugArrow(colorBlue, testPoint1, testPoint2, 2, 1000);
		}

		if ( light->IsPointlight() )
		{
			// If this is a centerlight we have to move the origin from the original origin to where the
			// center of the light is supposed to be.
			// Centerlight means that the center of the ellipsoid is not the same as the origin. It has to
			// be adjusted because if it casts shadows we have to trace to it, and in this case the light
			// might be inside geometry and would be reported as not being visible even though it casts
			// a visible light outside the geometry it is embedded in. If it is not a centerlight and has
			// cast shadows enabled, it wouldn't cast any light at all in such a case because it would
			// be blocked by the geometry.

			// grayman #3584 - IntersectLineEllipsoid() provides no information on whether
			// the line segment ends are inside or outside the ellipsoid. Let's use
			// IntersectLinesegmentLightEllipsoid() to get that information.

			inter = IntersectLinesegmentLightEllipsoid(	vTargetSeg, vLightCone, vResult, inside	);

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLinesegmentLightEllipsoid() returned %u\r", inter);
		}
		else // projected light
		{
			inter = IntersectLineLightCone(vTargetSeg, vLightCone, vResult, inside);
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("IntersectLineLightCone returned %u\r", inter);
		}

		// The line intersection returns one of four states. Either the line is entirely inside
		// the light cone (inter = INTERSECT_NONE), it's passing through the lightcone (inter = INTERSECT_FULL), the line
		// is not passing through which means that the test line is fully outside (inter = INTERSECT_OUTSIDE), or the line
		// is touching the cone in exactly one point (inter = INTERSECT_PARTIAL).

		if ( inter == INTERSECT_OUTSIDE ) // grayman #3584 - exclude the uninteresting case
		{
			b_excludeLight = true;
		}
		else
		{
			// grayman #3584 - the two points chosen for raytracing should be inside the light cone.
			// There are a few cases:

			// 1 - INTERSECT_NONE - Both ends of the line segment (testPoint1 and testPoint2) are inside the
			//     light cone, and should be tested for both visibility and illumination.
			//     Determine a third point, testPoint3, which is the midpoint between them.
			//
			//     a - If testPoint1 is visible from the light origin, use testPoint3 to determine illumination.
			//     b - If testPoint1 is not visible, move on to testPoint2. If testPoint2
			//         is visible from the light origin, use testPoint3 to determine illumination.
			//     c - If neither testPoint1 or testPoint2 is visible, move on to testPoint3.
			//         If testPoint3 is visible from the light origin, use it to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//
			// 2 - INTERSECT_PARTIAL - One end of the line segment (either testPoint1 or testPoint2) is inside the
			//     light cone, and the second point is the point of intersection with the line cone
			//     that lies on the line segment. Make the third point, testPoint3, the midpoint between the
			//     first two.
			//
			//     a - Let p1 be whichever of testPoint1 and testPoint2 is inside the light cone. It it's
			//         visible from the light origin, use it to determine illumination.
			//     b - If 'a' fails, move on to the point of intersection. If it's visible from the light
			//         origin, use testPoint3 to determine illumination.
			//     c - If 'b' fails, move on to testPoint3. If testPoint3 is visible from the light origin, use it
			//         to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//
			// 3 - INTERSECT_FULL - Both ends of the line segment lie outside the light cone. Treat the first intersection
			//     point as testPoint1, the second intersection point as testPoint2, and the midpoint
			//     between them as testPoint3.
			//
			//     a - If testPoint1 is visible from the light origin, use testPoint3 to determine illumination.
			//     b - If testPoint1 is not visible, move on to testPoint2. If testPoint2
			//         is visible from the light origin, use testPoint3 to determine illumination.
			//     c - If neither testPoint1 or testPoint2 is visible, move on to testPoint3.
			//         If testPoint3 is visible from the light origin, use it to determine illumination.
			//     d - If none of these points is visible from the light origin, exclude the light.
			//

			if (b_useShadows && light->CastsShadow()) 
			{
				// grayman #2853 - If the trace hits something before completing, that thing has to be checked to see if
				// it casts shadows. If it doesn't, then it has to be ignored and the trace must be run again from the struck
				// point to the end. This has to be done iteratively, since there might be several non-shadow-casting entities
				// in the way. For example, a candleflame in a candle in a chandelier, and the latter two are marked with 'noshadows'.
				// Light holders must also be taken into account, since the holder entity in DR can be marked 'noshadows', which
				// also applies to the candle holding the flame.

				bool lightReaches;

				if ( inter == INTERSECT_NONE ) // the line segment is entirely inside the light volume
				{
					p3 = (testPoint1 + testPoint2)/2.0f;
					lightReaches = traceLightPath( testPoint1, vLight, p_ignoredEntity, light );
					if ( !lightReaches )
					{
						lightReaches = traceLightPath( testPoint2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
					p_illumination = p3;
				}
				else if ( ( inter == INTERSECT_PARTIAL ) && ( inside[0] || inside[1] ) ) // one line end inside, one outside
				{
					// either testPoint1 or testPoint2 is inside the ellipsoid
					if ( inside[0] )
					{
						p1 = testPoint1;
					}
					else
					{
						p1 = testPoint2;
					}

					p2 = vResult[0]; // the single point of intersection
					p3 = (p1 + p2)/2.0f;
					lightReaches = traceLightPath( p1, vLight, p_ignoredEntity, light );
					if ( lightReaches )
					{
						p_illumination = p1;
					}
					else
					{
						p_illumination = p3;
						lightReaches = traceLightPath( p2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
				}
				else if ( inter == INTERSECT_PARTIAL ) // both line ends outside, line touches volume at one intersection point
				{
					// Since the only point we can test is at the edge of the light volume,
					// we can safely assume the illumination there is zero. No need to test
					// LOS to the light origin or determine brightness.
					lightReaches = false;
				}
				else // INTERSECT_FULL
				{
					p1 = vResult[0]; // the first point of intersection
					p2 = vResult[1]; // the second point of intersection
					p3 = (p1 + p2)/2.0f;
					p_illumination = p3;
					lightReaches = traceLightPath( p1, vLight, p_ignoredEntity, light );
					if ( !lightReaches )
					{
						lightReaches = traceLightPath( p2, vLight, p_ignoredEntity, light );
						if ( !lightReaches )
						{
							lightReaches = traceLightPath( p3, vLight, p_ignoredEntity, light );
						}
					}
				}
				
				b_excludeLight = !lightReaches;
			}
			else // no shadows, so assume visibility between the light origin and the point of illumination
			{
				if ( inter == INTERSECT_NONE ) // the line segment is entirely inside the light volume
				{
					p_illumination = (testPoint1 + testPoint2)/2.0f;
				}
				else if ( ( inter == INTERSECT_PARTIAL ) && ( inside[0] || inside[1] ) ) // one line end inside, one outside
				{
					// either testPoint1 or testPoint2 is inside the ellipsoid
					if ( inside[0] )
					{
						p_illumination = testPoint1;
					}
					else
					{
						p_illumination = testPoint2;
					}
				}
				else if ( inter == INTERSECT_PARTIAL ) // both line ends outside, line touches volume at one intersection point
				{
					b_excludeLight = true; // not interested in this case, since illumination is zero at the volume edges
				}
				else // INTERSECT_FULL
				{
					p_illumination = (vResult[0] + vResult[1])/2.0f; // halfway between the points of intersection
				}
			}
		}

		// Process light if not excluded
		if (!b_excludeLight)
		{
			// Compute illumination value
			// We want the illumination at p_illumination.

			float fx, fy;

			if ( light->IsPointlight() )
			{
				fx = p_illumination.x - vLight.x;
				fy = p_illumination.y - vLight.y;

				// ELA_AXIS contains the radii [x,y,z]
			}
			else // projected light
			{
				// p_illumination needs to be relative to the vector
				// from the light origin to the light target. Since the
				// original code assumed that vector pointed down along
				// the z axis, we'll rotate the target vector to that axis
				// and apply the same rotation to p_illumination so it stays
				// relative.

				// Re-get the light cone parameters, some of which were clobbered in IntersectLineLightCone().
				light->GetLightCone(vLightCone[ELC_ORIGIN], vLightCone[ELA_TARGET], vLightCone[ELA_RIGHT], vLightCone[ELA_UP], vLightCone[ELA_START], vLightCone[ELA_END]);
				idVec3 target = vLightCone[ELA_TARGET]; // direction of light cone, already relative to vLight

				// TODO: need to map p_illumination[x,y] to p_illumination[right,up]
				// then right is the new x and up is the new y

				if ( ( target.x == 0 ) && ( target.y == 0 ) ) // transform only if 'target' is not already on the z axis
				{
					fx = p_illumination.x - vLight.x;
					fy = p_illumination.y - vLight.y;
				}
				else
				{
					idVec3 p = p_illumination - vLight; // p is now p_illumination relative to the light origin

					// Matrices and steps are from http://inside.mines.edu/fs_home/gmurray/ArbitraryAxisRotation/

					// rotate 'target' to XY plane
					idVec2 target2 = target.ToVec2();
					float d = target2.LengthFast();
					float a = target.x/d;
					float b = target.y/d;
					idMat4 T1( idVec4(  a, b, 0, 0 ),
							   idVec4( -b, a, 0, 0 ),
							   idVec4(  0, 0, 1, 0 ),
							   idVec4(  0, 0, 0, 1 ) );
					idVec3 target_xy = T1*target; // the 'target' vector rotated to the XY plane

					// rotate 'target_xy' to the Z axis
					float c = target.LengthFast();
					float e = target.z/c;
					float f = d/c;
					idMat4 T2( idVec4(  e, 0, -f, 0 ),
							   idVec4(  0, 1,  0, 0 ),
							   idVec4(  f, 0,  e, 0 ),
							   idVec4(  0, 0,  0, 1 ) );
					idVec3 target_z = -(T2*target_xy);

					// apply T1 and T2 to p
					idVec3 p_z = -(T2*(T1*p));
					fx = p_z.x;
					fy = p_z.y;
				}
			}

			inout_totalIllumination += light->GetDistanceColor(	(p_illumination - vLight).LengthFast(),fx,fy );
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
			(
				"%s in x/y: %f/%f   Distance: %f/%f   Brightness: %f\r",
				light->name.c_str(), 
				fx, 
				fy, 
				(p_illumination - vLight).LengthFast(), 
				light->m_MaxLightRadius,
				inout_totalIllumination
			);
		}

		// If total illumination is 1.0 or greater, we are done
		if (inout_totalIllumination >= 1.0f)
		{
			// Exit early as it's really really bright as is
			p_cursor = NULL;
		}
		else
		{
			// Iterate to next light in area
			p_cursor = p_cursor->NextNode();
		}
	}
}

//----------------------------------------------------------------------------

/*!
* Initialization
*/
void darkModLAS::initialize()
{	
	CREATE_TIMER(queryLightingAlongLineTimer, "LAS", "Lighting");

	if (Disabled())	// stgatilov #6546: completely disable LAS
		return;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Initializing Light Awareness System (LAS)\r");

	// Dispose of any previous information
	if (m_pp_areaLightLists != NULL)
	{
		delete[] m_pp_areaLightLists;
		m_pp_areaLightLists = NULL;
	}

	// Get number of areas in the map
	m_numAreas = gameRenderWorld->NumAreas();

	// Allocate a light record list for each area
	if (m_numAreas > 0)
	{
		// angua: add one entry for lights in the void
		m_pp_areaLightLists = new idLinkList<darkModLightRecord_t>*[m_numAreas + 1];
		if (m_pp_areaLightLists == NULL)
		{
			// Log error and exit
			DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Failed to allocate LAS area light lists.\r");
			m_numAreas = 0;
			return;
		}
	}

	// Lists all begin empty
	for (int i = 0; i < m_numAreas + 1; i ++)
	{
		m_pp_areaLightLists[i] = NULL;
	}

	// Frame index starts at 0
	m_updateFrameIndex = 0;


	// Log status
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS initialized for %d map areas.\r", m_numAreas);

	// Build default PVS to AAS Mapping table
	pvsToAASMappingTable.clear();
	if (!pvsToAASMappingTable.buildMappings("aas32"))
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Failed to initialize PVS to aas32 mapping table, trying aas48.\r");

		if (!pvsToAASMappingTable.buildMappings("aas48"))
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Failed to initialize PVS to aas48 mapping table.\r");
		}
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("PVS to aas32 mapping table initialized.\r");
	}

}

//-------------------------------------------------------------------------

void darkModLAS::addLight (idLight* p_idLight)
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
		return;

	// grayman #3843 - you can't use the light's origin by itself
	// if there's a meaningful light_center

	// Get the light position
	//idVec3 lightPos(p_idLight->GetPhysics()->GetOrigin());
	idVec3 lightPos;
	idVec3 lightAxis;
	idVec3 lightCenter;

	// fill in the light data
	p_idLight->GetLightCone(lightPos, lightAxis, lightCenter);

	lightPos += lightCenter; // true origin of light

	// Determine the index of the area containing this light
	int containingAreaIndex = gameRenderWorld->GetAreaAtPoint (lightPos);
	if (containingAreaIndex < 0)
	{
		// The light isn't in an area
		// TODO: Log error, light is not in an area
		DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Light is not contained in an area\r");
		return;
	}

	// Add it to the appropriate list
	assert (containingAreaIndex < m_numAreas);

	// Make a darkMod light record for it
	darkModLightRecord_t* p_record = new darkModLightRecord_t;
	p_record->lastFrameUpdated = m_updateFrameIndex;
	p_record->lastWorldPos = lightPos;
	p_record->p_idLight = p_idLight;
	p_record->areaIndex = containingAreaIndex;

	if (m_pp_areaLightLists[containingAreaIndex] != NULL)
	{
		idLinkList<darkModLightRecord_t>* p_node = new idLinkList<darkModLightRecord_t>;
		p_node->SetOwner (p_record);
		p_node->AddToEnd (*(m_pp_areaLightLists[containingAreaIndex]));

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light '%s' was added to area %d at end of list\r", p_idLight->name.c_str(), containingAreaIndex);
	}
	else
	{
		// First in area
		idLinkList<darkModLightRecord_t>* p_first = new idLinkList<darkModLightRecord_t>;
		p_first->SetOwner (p_record);
		if (p_first == NULL)
		{
			DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Failed to create node for LASLight record\r");
			return;
		}
		else
		{
			m_pp_areaLightLists[containingAreaIndex] = p_first;
		}

		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light '%s' was added to area %d as first in list\r", p_idLight->name.c_str(), containingAreaIndex);
	}

	// Note the area we just added the light to
	p_idLight->LASAreaIndex = containingAreaIndex;
}


//------------------------------------------------------------------------------

void darkModLAS::removeLight (idLight* p_idLight)
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
		return;

	// Test parameters
	assert (p_idLight != NULL);
	
	if (p_idLight->LASAreaIndex < 0)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Attempted to remove the light '%s' with no assigned LAS area index\r", p_idLight->name.c_str());
		return;
	}
	// angua: lights in the void are at number m_numAreas
	else if (p_idLight->LASAreaIndex > m_numAreas)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Attempted to remove the light '%s' with out of bounds area index %d\r", p_idLight->name.c_str(), p_idLight->LASAreaIndex);
		return;
	}

	if (m_pp_areaLightLists == NULL)
	{
		// Log error
		DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("LAS not initialized. Remove light '%s' request ignored\r", p_idLight->name.c_str());
		return;
	}

	// Remove the light from the list it should be in
	idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[p_idLight->LASAreaIndex];
	while (p_cursor != NULL)
	{
		if (p_cursor->Owner()->p_idLight == p_idLight)
		{
			// Keep track of header, bass ackward idLinkedList can't
			// update the header pointer on its own because of the inverted
			// way it handles the container arrangement.
			if (m_pp_areaLightLists[p_idLight->LASAreaIndex] == p_cursor)
			{
				m_pp_areaLightLists[p_idLight->LASAreaIndex] = p_cursor->NextNode();
				if (m_pp_areaLightLists[p_idLight->LASAreaIndex] == p_cursor)
				{
					// If only one node left in the list, it makes circular link because idLinkList::Remove doesn't 
					// update the head. Also because of this, on removing the first node in the list, the head
					// of the list is no longer real and all iterations become circular. That is a problem.
					// pointer tracked in each node.  Its a logical error in idLinkList::Remove that
					// really should be fixed.
					m_pp_areaLightLists[p_idLight->LASAreaIndex] = NULL;
				}
			}

			// Remove this node from its list and destroy the record
			p_cursor->RemoveHeadsafe();


			// Light not in an LAS area
			int tempIndex = p_idLight->LASAreaIndex;
			p_idLight->LASAreaIndex = -1;

			// Destroy node
			delete p_cursor;

			// Log status
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light '%s' was removed from LAS area %d\r", p_idLight->name.c_str(), tempIndex);

			// Done
			return;
		}
		else
		{
			p_cursor = p_cursor->NextNode();
		}
	}

	// Light not found
	// Log error
	DM_LOG(LC_LIGHT, LT_ERROR)LOGSTRING("Light '%s' stating it was in LAS area index %d was not found in that LAS Area's list\r", p_idLight->name.c_str(), p_idLight->LASAreaIndex);

	// Done
}

//----------------------------------------------------------------------------

void darkModLAS::shutDown()
{
	// Log activity
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown initiated...\r");

	// Delete array of list pointers
	if (m_pp_areaLightLists != NULL)
	{
		// Delete all records in each list
		// angua: also remove lights in the void
		for (int areaIndex = 0; areaIndex < m_numAreas + 1; areaIndex ++)
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown clearing light records for areaIndex %d...\r", areaIndex);
		
			// Destroy each light record
			idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];
			idLinkList<darkModLightRecord_t>* p_temp;
			while (p_cursor != NULL)
			{
				darkModLightRecord_t* p_LASLight = p_cursor->Owner();
				DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light list iterating node %d\r", p_cursor);

				if (p_LASLight != NULL)
				{
					DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light list clear is deleting light '%s'\r", p_LASLight->p_idLight->GetName());
					delete p_LASLight;
				}

				// Next node
				p_temp = p_cursor->NextNode();
				p_cursor = p_temp;
			}

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown destroying node list for areaIndex %d...\r", areaIndex);

			// Clear the list of nodes
			if (m_pp_areaLightLists[areaIndex] != NULL)
			{
				delete m_pp_areaLightLists[areaIndex];
				m_pp_areaLightLists[areaIndex] = NULL;
			}

			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown destroyed list for for areaIndex %d\r", areaIndex);

		} // Next area

		// Log activity
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown deleting array of per-area list pointers...\r");

		delete[] m_pp_areaLightLists;
		m_pp_areaLightLists = NULL;
	}

	// No areas
	m_numAreas = 0;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shutdown deleted array of per-area list pointers...\r");

	// Log activity
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS shut down and empty");

	// Destroy PVS to AAS mapping table contents
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Clearing PVS to AAS(0) mapping table ...\r");
	pvsToAASMappingTable.clear();
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("PVS to AAS(0) mapping table cleared\r");


}

//----------------------------------------------------------------------------

void darkModLAS::updateLASState()
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
		return;

	// Doing a new update frame
	m_updateFrameIndex ++;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Updating LAS state, new LAS frame index is %d\r", m_updateFrameIndex);

	// Go through each of the areas and for any light that has moved, see
	// if it has changed areas.
	// angua: also check for lights in the void

	for (int areaIndex = 0; areaIndex < m_numAreas + 1; areaIndex ++)
	{
		idLinkList<darkModLightRecord_t>* p_cursor = m_pp_areaLightLists[areaIndex];
		while (p_cursor != NULL)
		{
			// Get the darkMod light record
			darkModLightRecord_t* p_LASLight = p_cursor->Owner();

			// Remember next node in the current list
			idLinkList<darkModLightRecord_t>* p_nextTemp = p_cursor->NextNode();

			// Have we updated this light yet?
			if (p_LASLight->lastFrameUpdated != m_updateFrameIndex)
			{

				// grayman #3843 - apply the light_center value
				// Get the light position
				//idVec3 lightPos(p_LASLight->p_idLight->GetPhysics()->GetOrigin());
				idVec3 lightPos;
				idVec3 lightAxis;
				idVec3 lightCenter;

				// fill in the light data
				p_LASLight->p_idLight->GetLightCone(lightPos, lightAxis, lightCenter);

				lightPos += lightCenter; // true origin of light
	
				// Check to see if it has moved
				if (p_LASLight->lastWorldPos != lightPos)
				{
					// Update its world pos
					p_LASLight->lastWorldPos = lightPos;

					// This light may have moved between areas
					int newAreaIndex = gameRenderWorld->GetAreaAtPoint (p_LASLight->lastWorldPos);
					if (newAreaIndex == -1)
					{
						// Light is now in the void
						// add to the end of the list
						newAreaIndex = m_numAreas;
					}

					if (newAreaIndex != p_LASLight->areaIndex)
					{
						// Move between areas
						moveLightBetweenAreas(p_LASLight, p_LASLight->areaIndex, newAreaIndex);
					
					}  // Light changed areas
				
				} // Light moved
			
				// Mark light as updated this LAS frame
				p_LASLight->lastFrameUpdated = m_updateFrameIndex;
				
			} // Light not yet updated
	
			// Iterate to next light
			p_cursor = p_nextTemp;

		} // Next light in this area
	
	} // Next area

	// Done
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("LAS update frame %d complete\r", m_updateFrameIndex);
}


//----------------------------------------------------------------------------

float darkModLAS::queryLightingAlongLine
(
		idVec3 testPoint1,
		idVec3 testPoint2,
		idEntity* p_ignoreEntity,
		bool b_useShadows
)
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
	{
		common->Warning("darkModLAS::queryLightingAlongLine disabled!");
		return 0.0f;
	}

	START_SCOPED_TIMING(queryLightingAlongLineTimer, scopedQueryLightingAlongLineTimer);

	if (p_ignoreEntity != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"queryLightingAlongLine [%s] to [%s], IgnoredEntity = '%s', UseShadows = %d'\r", 
			testPoint1.ToString(),
			testPoint2.ToString(),
			p_ignoreEntity->name.c_str(),
			b_useShadows
		);
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"queryLightingAlongLine [%s] to [%s],  UseShadows = %d'\r", 
			testPoint1.ToString(),
			testPoint2.ToString(),
			b_useShadows
		);

	}

	// Total illumination starts at 0
	float totalIllumination = 0.0f;

	// Find the area that the test points are in
//	int testPointAreaIndex = gameRenderWorld->GetAreaAtPoint (testPoint1);

	// Compute test bounds
	idVec3 mins, maxes;
	if (testPoint1.x < testPoint2.x)
	{
		mins.x = testPoint1.x;
		maxes.x = testPoint2.x;
	}
	else
	{
		mins.x = testPoint2.x;
		maxes.x = testPoint1.x;
	}

	if (testPoint1.y < testPoint2.y)
	{
		mins.y = testPoint1.y;
		maxes.y = testPoint2.y;
	}
	else
	{
		mins.y = testPoint2.y;
		maxes.y = testPoint1.y;
	}

	if (testPoint1.z < testPoint2.z)
	{
		mins.z = testPoint1.z;
		maxes.z = testPoint2.z;
	}
	else
	{
		mins.z = testPoint2.z;
		maxes.z = testPoint1.z;
	}

	idBounds testBounds (mins, maxes);

	// Run a local PVS query to determine which other areas are visible (and hence could
	// have lights shining on the target area)
	int pvsTestAreaIndices[idEntity::MAX_PVS_AREAS];
	int numPVSTestAreas = gameLocal.pvs.GetPVSAreas
	(
		testBounds,
		pvsTestAreaIndices,
		idEntity::MAX_PVS_AREAS
	);

	// Set up the graph
	pvsHandle_t h_lightPVS = gameLocal.pvs.SetupCurrentPVS
	(
		pvsTestAreaIndices, 
		numPVSTestAreas
	);

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
	(
		"queryLightingAlongLine: PVS test results in %d PVS areas\r", 
		numPVSTestAreas
	);

	// grayman #3843 - start with the ambient light, if any

	totalIllumination = gameLocal.GetAmbientIllumination(testPoint1);
	
	// Check all the lights in the PVS areas and factor them in
	for ( int pvsTestResultIndex = 0 ; pvsTestResultIndex < numPVSTestAreas ; pvsTestResultIndex++ )
	{
		// Add the effect of lights in this visible area to the effect at the point
		accumulateEffectOfLightsInArea 
		(
			totalIllumination,
			pvsTestAreaIndices[pvsTestResultIndex],
			testPoint1,
			testPoint2,
			p_ignoreEntity,
			b_useShadows
		);
	}

	// Done with PVS test
	gameLocal.pvs.FreeCurrentPVS( h_lightPVS );

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
	(
		"queryLightingAlongLine [%s] to [%s],  result is %.2f\r", 
		testPoint1.ToString(),
		testPoint2.ToString(),
		totalIllumination
	);

	// Return total illumination value to the caller
	return totalIllumination;
}

// grayman #3584 - new version of queryLightingAlongLine() to pick the best
// line inside the object's representative box, where 'best' means 'lies closest
// to light center'. While the same line might not be chosen by all lights,
// it's probably okay to assume that lights illuminating different portions of
// the same object should yield the same result as lights illuminating the
// same portion.

float darkModLAS::queryLightingAlongBestLine
(
		idBox box,
		idEntity* p_ignoreEntity,
		bool b_useShadows
)
{
	if (Disabled())	// stgatilov #6546: completely disable LAS
	{
		common->Warning("darkModLAS::queryLightingAlongBestLine disabled!");
		return 0.0f;
	}

	START_SCOPED_TIMING(queryLightingAlongLineTimer, scopedQueryLightingAlongLineTimer);

	if (p_ignoreEntity != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"queryLightingAlongBestLine inside [%s], IgnoredEntity = '%s', UseShadows = %d'\r", 
			box.ToString(),
			p_ignoreEntity->name.c_str(),
			b_useShadows
		);
	}
	else
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
		(
			"queryLightingAlongBestLine inside [%s], UseShadows = %d'\r", 
			box.ToString(),
			b_useShadows
		);
	}

	// Total illumination starts at 0
	float totalIllumination = 0.0f;

	// Run a local PVS query to determine which other areas are visible (and hence could
	// have lights shining on the target area)
	int pvsTestAreaIndices[idEntity::MAX_PVS_AREAS];
	idBounds bounds = p_ignoreEntity->GetPhysics()->GetAbsBounds();
	int numPVSTestAreas = gameLocal.pvs.GetPVSAreas
	(
		bounds,
		pvsTestAreaIndices,
		idEntity::MAX_PVS_AREAS
	);

	// Set up the graph
	pvsHandle_t h_lightPVS = gameLocal.pvs.SetupCurrentPVS
	(
		pvsTestAreaIndices, 
		numPVSTestAreas
	);

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING
	(
		"queryLightingAlongLine: PVS test results in %d PVS areas\r", 
		numPVSTestAreas
	);

	// grayman #3843 - start with the ambient light, if any

	totalIllumination += gameLocal.GetAmbientIllumination(box.GetCenter());
	
	// Check all the lights in the PVS areas and factor them in
	for ( int pvsTestResultIndex = 0 ; pvsTestResultIndex < numPVSTestAreas ; pvsTestResultIndex++ )
	{
		// Add the effect of lights in this visible area to the effect at the point
		accumulateEffectOfLightsInArea2 
		(
			totalIllumination,
			pvsTestAreaIndices[pvsTestResultIndex],
			box,
			p_ignoreEntity,
			b_useShadows
		);
	}

	// Done with PVS test
	gameLocal.pvs.FreeCurrentPVS( h_lightPVS );

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("queryLightingAlongBestLine - result is %.2f\r",totalIllumination);

	// Return total illumination value to the caller
	return totalIllumination;
}

//----------------------------------------------------------------------------

idStr darkModLAS::getAASName()
{
	return pvsToAASMappingTable.getAASName();
}

