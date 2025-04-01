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

#include "renderer/tr_local.h"

idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
idCVar r_singleModelName( "r_singleModelName", "", CVAR_RENDERER, "filter entities by model name, e.g. 'models/darkmod/nature/flowers/flowers_patch_01.ase'" );

/*


All that is done in these functions is the creation of viewLights
and viewEntitys for the lightDefs and entityDefs that are visible
in the portal areas that can be seen from the current viewpoint.

*/


// if we hit this many planes, we will just stop cropping the
// view down, which is still correct, just conservative
const int MAX_PORTAL_PLANES	= 20;

// stgatilov: ensure that normal vector of a portal plane has error less than this coefficient
// if after winding is clipped, we see a portal plane with larger error, we simply drop such a plane
const float MAX_PLANE_NORMAL_ERROR = 0.001f;

typedef struct portalStack_s {
	portal_t	*p;
	const struct portalStack_s *next;

	idScreenRect	rect;

	int			numPortalPlanes;
	idPlane		portalPlanes[MAX_PORTAL_PLANES + 1];
	// positive side is outside the visible frustum
} portalStack_t;


//====================================================================


/*
===================
idRenderWorldLocal::ScreenRectForWinding
===================
*/
idScreenRect idRenderWorldLocal::ScreenRectFromWinding( const idWinding *w, viewEntity_t *space ) {
	idScreenRect	r;
	int				i;
	idVec3			v;
	idVec3			ndc;
	float			windowX, windowY;

	r.Clear();
	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		R_LocalPointToGlobal( space->modelMatrix, ( *w )[i].ToVec3(), v );
		R_GlobalToNormalizedDeviceCoordinates( v, ndc );

		windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
		windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

		r.AddPoint( windowX, windowY );
	}
	r.Expand();

	return r;
}

/*
===================
PortalIsFoggedOut
===================
*/
bool idRenderWorldLocal::PortalIsFoggedOut( const portal_t *p ) {
	idRenderLightLocal	*ldef;
	int			i;
	idPlane		forward;

	ldef = p->doublePortal->fogLight;

	if ( !ldef ) {
		return false;
	}

	// find the current density of the fog
	const idMaterial	*lightShader = ldef->lightShader;
	int		size = sizeof( float ) * lightShader->GetNumRegisters();
	float	*regs = ( float * )_alloca( size );

	lightShader->EvaluateRegisters( regs, ldef->parms.shaderParms, tr.viewDef, ldef->parms.referenceSound );

	const shaderStage_t	*stage = lightShader->GetStage( 0 );

	float a, alpha = regs[ stage->color.registers[3] ];

	// if they left the default value on, set a fog distance of 500
	if ( alpha <= 1.0f ) {
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / alpha;
	}
	forward[0] = a * tr.viewDef->worldSpace.modelViewMatrix[2];
	forward[1] = a * tr.viewDef->worldSpace.modelViewMatrix[6];
	forward[2] = a * tr.viewDef->worldSpace.modelViewMatrix[10];
	forward[3] = a * tr.viewDef->worldSpace.modelViewMatrix[14];

	auto w = &p->w;

	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		float d = forward.Distance( ( *w )[i].ToVec3() );
		if ( d < 0.5f ) {
			return false;		// a point not clipped off
		}
	}
	return true;
}

/*
===================
FloodViewThroughArea_r
===================
*/
void idRenderWorldLocal::FloodViewThroughArea_r( const idVec3 origin, int areaNum,
        const struct portalStack_s *ps ) {
	//portal_t*		p;
	float			d;
	const portalStack_t	*check;
	portalStack_t	newStack;
	int				j;
	idVec3			v1, v2;
	int				addPlanes;
	idFixedWinding	w;		// we won't overflow because MAX_PORTAL_PLANES = 20

	auto &area = portalAreas[ areaNum ];

	// cull models and lights to the current collection of planes
	AddAreaRefs( areaNum, ps );

	if ( portalAreas[areaNum].areaScreenRect.IsEmpty() ) {
		portalAreas[areaNum].areaScreenRect = ps->rect;
	} else {
		portalAreas[areaNum].areaScreenRect.Union( ps->rect );
	}

	// For r_showPortals. Keep track whether the player's view flows through
	// individual portals, not just whole visleafs.  -- SteveL #4162
	if ( r_showPortals && ps->p )
		ps->p->doublePortal->portalViewCount = tr.viewCount;

	// go through all the portals
	for ( auto p : area.areaPortals ) {
		// an enclosing door may have sealed the portal off
		if ( p->doublePortal->blockingBits & PS_BLOCK_VIEW ) {
			continue;
		}

		// make sure this portal is facing away from the view
		d = p->plane.Distance( origin );
		if ( d < -0.1f ) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for ( check = ps; check; check = check->next ) {
			if ( check->p == p ) {
				break;		// don't recursively enter a stack
			}
		}
		if ( check ) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if ( d < 1.0f ) {
			// SteveL #3815: check the view origin is really near portal surface
			if ( p->w.PointInsideDst( p->plane.Normal(), origin, 1.0f ) ) {
				// go through this portal
				newStack = *ps;
				newStack.p = p;
				newStack.next = ps;
				FloodViewThroughArea_r( origin, p->intoArea, &newStack );
				continue;
			}
		}

		// clip the portal winding to all of the planes
		w = p->w;
		for ( j = 0; j < ps->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// see if it is fogged out
		if ( PortalIsFoggedOut( p ) ) {
			continue;
		}

		// go through this portal
		newStack.p = p;
		newStack.next = ps;

		// find the screen pixel bounding box of the remaining portal
		// so we can scissor things outside it
		newStack.rect = ScreenRectFromWinding( &w, &tr.identitySpace );

		// slop might have spread it a pixel outside, so trim it back
		newStack.rect.Intersect( ps->rect );

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect
		addPlanes = w.GetNumPoints();
		if ( addPlanes > MAX_PORTAL_PLANES ) {
			addPlanes = MAX_PORTAL_PLANES;
		}
		newStack.numPortalPlanes = 0;

		for ( int i = 0; i < addPlanes; i++ ) {
			j = i + 1;
			if ( j == w.GetNumPoints() ) {
				j = 0;
			}
			v1 = origin - w[i].ToVec3();
			v2 = origin - w[j].ToVec3();

			//stgatilov: drop plane if its direction is not precise enough
			idVec3 normal;
			normal.Cross( v2, v1 );
			float sinAng = normal.LengthFast() * idMath::RSqrt( v2.LengthSqr() * v1.LengthSqr() );
			static const float SIN_THRESHOLD = idMath::FLT_EPS / MAX_PLANE_NORMAL_ERROR;
			if ( sinAng <= SIN_THRESHOLD ) {
				continue;
			}
			newStack.portalPlanes[newStack.numPortalPlanes].Normal() = normal;
			newStack.portalPlanes[newStack.numPortalPlanes].Normalize();
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( origin );

			newStack.numPortalPlanes++;
		}

		// the last stack plane is the portal plane
		newStack.portalPlanes[newStack.numPortalPlanes] = p->plane;
		newStack.numPortalPlanes++;

		FloodViewThroughArea_r( origin, p->intoArea, &newStack );
	}
}

/*
=======================
FlowViewThroughPortals

Finds viewLights and viewEntities by flowing from an origin through the visible portals.
origin point can see into.  The planes array defines a volume (positive
sides facing in) that should contain the origin, such as a view frustum or a point light box.
Zero planes assumes an unbounded volume.
=======================
*/
void idRenderWorldLocal::FlowViewThroughPortals( const idVec3 origin, int numPlanes, const idPlane *planes ) {
	portalStack_t	ps;
	int				i;

	ps.next = nullptr;
	ps.p = nullptr;

	for ( i = 0 ; i < numPlanes ; i++ ) {
		ps.portalPlanes[i] = planes[i];
	}
	ps.numPortalPlanes = numPlanes;
	ps.rect = tr.viewDef->scissor;

	if ( tr.viewDef->areaNum < 0 ) {

		for ( auto &a : portalAreas ) {
			a.areaScreenRect = tr.viewDef->scissor;
		}

		// if outside the world, mark everything
		for ( i = 0; i < portalAreas.Num(); i++ ) {
			AddAreaRefs( i, &ps );
		}
	} else {

		for ( auto &a : portalAreas ) {
			a.areaScreenRect.Clear();
		}

		// flood out through portals, setting area viewCount
		FloodViewThroughArea_r( origin, tr.viewDef->areaNum, &ps );
	}
}

//==================================================================================================

struct idRenderWorldLocal::FlowLightThroughPortalsContext {
	const idRenderLightLocal *light;
	AreaList *resultAreaIds;
	lightPortalFlow_t *resultPortalFlow;
};

/*
===================
FloodLightThroughArea_r
===================
*/
void idRenderWorldLocal::FloodLightThroughArea_r( FlowLightThroughPortalsContext &context, int areaNum, const struct portalStack_s *ps ) const {
	float			d;
	const portalArea_t 	*area;
	const portalStack_t	*check, *firstPortalStack;
	portalStack_t	newStack;
	int				j;
	idVec3			v1, v2;
	int				addPlanes;
	idFixedWinding	w;		// we won't overflow because MAX_PORTAL_PLANES = 20

	area = &portalAreas[ areaNum ];

	// add area reference to result
	if ( context.resultAreaIds && !context.resultAreaIds->Find(areaNum) ) {
		context.resultAreaIds->AddGrow(areaNum);
	}
	if ( context.resultPortalFlow ) {
		lightPortalFlow_t &flow = *context.resultPortalFlow;
		lightPortalFlow_t::areaRef_t ref;
		ref.areaIdx = areaNum;
		ref.planeBeg = flow.planeStorage.Num();
		for ( int i = 0; i < ps->numPortalPlanes; i++ )
			flow.planeStorage.AddGrow( ps->portalPlanes[i] );
		ref.planeEnd = flow.planeStorage.Num();
		flow.areaRefs.AddGrow(ref);
	}

	// go through all the portals
	for ( auto p : area->areaPortals ) {
		// make sure this portal is facing away from the view
		d = p->plane.Distance( context.light->globalLightOrigin );
		if ( d < -0.1f ) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for ( check = ps; check; check = check->next ) {
			firstPortalStack = check;
			if ( check->p == p ) {
				break;		// don't recursively enter a stack
			}
		}

		if ( check ) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if ( d < 1.0f ) {
			// stgatilov #3815: check the view origin is really near portal surface
			if ( p->w.PointInsideDst( p->plane.Normal(), context.light->globalLightOrigin, 1.0f ) ) {
				// go through this portal
				newStack = *ps;
				newStack.p = p;
				newStack.next = ps;
				FloodLightThroughArea_r( context, p->intoArea, &newStack );
				continue;
			}
		}

		// clip the portal winding to all of the planes
		w = p->w;
		for ( j = 0; j < ps->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) ) {
				break;
			}
		}

		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// also always clip to the original light planes, because they aren't
		// necessarily extending to infinitiy like a view frustum
		for ( j = 0; j < firstPortalStack->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -firstPortalStack->portalPlanes[j], 0 ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.p = p;
		newStack.next = ps;

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect
		addPlanes = w.GetNumPoints();
		if ( addPlanes > MAX_PORTAL_PLANES ) {
			addPlanes = MAX_PORTAL_PLANES;
		}
		newStack.numPortalPlanes = 0;

		for ( int i = 0; i < addPlanes; i++ ) {
			j = i + 1;
			if ( j == w.GetNumPoints() ) {
				j = 0;
			}
			v1 = context.light->globalLightOrigin - w[i].ToVec3();
			v2 = context.light->globalLightOrigin - w[j].ToVec3();

			//stgatilov: drop plane if its direction is not precise enough
			idVec3 normal;
			normal.Cross( v2, v1 );
			float sinAng = normal.LengthFast() * idMath::RSqrt( v2.LengthSqr() * v1.LengthSqr() );
			static const float SIN_THRESHOLD = idMath::FLT_EPS / MAX_PLANE_NORMAL_ERROR;
			if ( sinAng <= SIN_THRESHOLD ) { 
				continue; 
			}
			newStack.portalPlanes[newStack.numPortalPlanes].Normal() = normal;
			newStack.portalPlanes[newStack.numPortalPlanes].Normalize();
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( context.light->globalLightOrigin );

			newStack.numPortalPlanes++;
		}
		FloodLightThroughArea_r( context, p->intoArea, &newStack );
	}
}


/*
=======================
FlowLightThroughPortals

Adds an arearef in each area that the light center flows into.
This can only be used for shadow casting lights that have a generated
prelight, because shadows are cast from back side which may not be in visible areas.
=======================
*/
void idRenderWorldLocal::FlowLightThroughPortals( const idRenderLightLocal *light, const AreaList &startingAreaIds, AreaList *areaIds, lightPortalFlow_t *portalFlow ) const {
	if ( areaIds )
		areaIds->Clear();
	if ( portalFlow )
		portalFlow->Clear();

	FlowLightThroughPortalsContext context;
	context.light = light;
	context.resultAreaIds = areaIds;
	context.resultPortalFlow = portalFlow;

	portalStack_t ps;
	memset( &ps, 0, sizeof( ps ) );
	ps.numPortalPlanes = 6;
	for ( int i = 0; i < 6; i++ ) {
		ps.portalPlanes[i] = light->frustum[i];
	}

	// if the light origin areaNum is not in a valid area,
	// the light won't have any area refs
	for ( int i = 0; i < startingAreaIds.Num(); i++ ) {
		int areaNum = startingAreaIds[i];
		FloodLightThroughArea_r( context, areaNum, &ps );
	}

	if ( portalFlow ) {
		// sort area refs
		std::stable_sort( portalFlow->areaRefs.begin(), portalFlow->areaRefs.end() );
	}
}

//======================================================================================================

struct idRenderWorldLocal::FloodShadowFrustumContext {
	//algorithm fails if this number of portal checks is not enough to finish
	static const int PORTALS_BUDGET = 32;

	idScreenRect scissorBounds;		//caller-specified bounds on resulting scissor rect
	idScreenRect scissorRect;		//union of rects of all areas visited so far
	idFrustum frustum;				//shadow frustum
	int portalBudgetRemains;
	int areaStack[PORTALS_BUDGET], areaStackNum;
};

/*
===================
idRenderWorldLocal::FloodShadowFrustumThroughArea_r
===================
*/
bool idRenderWorldLocal::FloodShadowFrustumThroughArea_r( FloodShadowFrustumContext &context, const idBounds &bounds ) const {
	int areaNum = context.areaStack[context.areaStackNum - 1];

	// add screen rect from the current area to result
	idScreenRect addedRect = GetAreaScreenRect(areaNum);
	addedRect.Intersect(context.scissorBounds);
	//TODO: transform "bounds" into screen space and take into account?
	if (!addedRect.IsEmpty())
		context.scissorRect.Union(addedRect);
	if (context.scissorRect.Equals(context.scissorBounds))
		return false;	//full scissor already -> finish immediately

	// go through all the portals
	const portalArea_t &portalArea = portalAreas[ areaNum ];
	for ( const portal_t *p : portalArea.areaPortals ) {
		// note: unlike what original D3 did, it is wrong to forbid visiting the same area twice
		// but we should omit portal if we already visited its area on the current recursion stack (avoid looping)
		int s;
		for (s = context.areaStackNum - 1; s >= 0; s--)
			if (context.areaStack[s] == p->intoArea)
				break;
		if (s >= 0)
			continue;

		// avoid spending too much time in complicated cases
		if (--context.portalBudgetRemains < 0) {
			context.scissorRect = context.scissorBounds;
			return false;	//give up -> finish immediately
		}

		// the frustum origin must be at the front of the portal plane
		if ( p->plane.Side( context.frustum.GetOrigin(), 0.1f ) == SIDE_BACK )
			continue;

		// assume that blocked portals (i.e. closed doors) block both light and shadows completely
		if (p->doublePortal->blockingBits & PS_BLOCK_VIEW)
			continue;

		// the frustum must cross the portal plane
		if ( context.frustum.PlaneSide( p->plane, 0.0f ) != PLANESIDE_CROSS )
			continue;

		// get the bounds for the portal winding projected in the frustum 
		idBounds newBounds;
		if (!context.frustum.ProjectionBounds( p->w, newBounds ))
			continue;

		newBounds.IntersectSelf( bounds );
		if (newBounds.IsBackwards())
			continue;	//no intersection
		newBounds[1][0] = context.frustum.GetFarDistance();

		// visit the new area recursively
		context.areaStack[context.areaStackNum++] = p->intoArea;
		bool res = FloodShadowFrustumThroughArea_r(context, newBounds);
		--context.areaStackNum;

		if (!res)
			return false;
	}

	return true;
}

/*
===================
idRenderWorldLocal::FlowShadowFrustumThroughPortals

Flows shadow frustum through portals, trying to reduce the scissor rect bounding it.
scissorRect should have some initial value, usually scissor rect of the light volume.
If it gets inverted, then the shadow can be completely culled away.
startAreas[0..startAreasNum) lists all areas which contain the object generating the shadow.
===================
*/
void idRenderWorldLocal::FlowShadowFrustumThroughPortals( idScreenRect &scissorRect, const idFrustum &frustum, const int *startAreas, int startAreasNum ) const {
	// bounds that cover the whole frustum
	idBounds bounds;
	bounds[0].Set( frustum.GetNearDistance(), -1.0f, -1.0f );
	bounds[1].Set( frustum.GetFarDistance(), 1.0f, 1.0f );

	//TODO: start tracing frustum from light source?
	//then we would be able to detect that light doesn't hit object via portals

	FloodShadowFrustumContext context;
	context.scissorRect.Clear();
	context.scissorBounds = scissorRect;
	context.frustum = frustum;
	context.portalBudgetRemains = FloodShadowFrustumContext::PORTALS_BUDGET;
	context.areaStackNum = 0;

	for (int i = 0; i < startAreasNum; i++) {
		context.areaStack[0] = startAreas[i];
		context.areaStackNum = 1;
		bool res = FloodShadowFrustumThroughArea_r(context, bounds);
		assert(context.areaStackNum == 1);
		if (!res)
			return;
	}

	scissorRect = context.scissorRect;
}


/*
=======================================================================

R_FindViewLightsAndEntities

=======================================================================
*/

/*
================
CullEntityByPortals

Return true if the entity reference bounds do not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullEntityByPortals( const idRenderEntityLocal *entity, const portalStack_t *ps ) {
	if ( r_useEntityPortalCulling.GetInteger() == 1 ) {
		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, entity->inverseBaseModelProject, bounds_unitCube );
		for ( int i = 0; i < ps->numPortalPlanes; i++ ) {
			if ( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT ) {
				return true;
			}
		}
	} else if ( r_useEntityPortalCulling.GetInteger() >= 2 ) {

		idRenderMatrix baseModelProject;
		// Was idRenderMatrix::Inverse but changed to InverseByDoubles for precision
		idRenderMatrix::Inverse( entity->inverseBaseModelProject, baseModelProject );
		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, baseModelProject, false, true );

		// exact clip of light faces against all planes
		for ( int i = 0; i < 6; i++ ) {
			// the entity frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the entity,
			// which must have some fragment inside the portal stack planes to be visible
			if ( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f ) {
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for ( int j = 0; j < 6; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) ) {
					break;
				}
			}

			if ( w.GetNumPoints() <= 2 ) {
				continue;
			}
			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for ( int j = 0; j < ps->numPortalPlanes - 1; j++ ) {
				if ( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) ) {
					break;
				}
			}

			if ( w.GetNumPoints() > 2 ) {
				// part of the winding is visible through the portalStack,
				// so the entity is not culled
				return false;
			}
		}

		// nothing was visible
		return true;

	}
	return false;
}

/*
===================
AddAreaEntityRefs

Any models that are visible through the current portalStack will
have their scissor
===================
*/
void idRenderWorldLocal::AddAreaEntityRefs( int areaNum, const portalStack_t *ps ) {
	idRenderEntityLocal	*entity;
	portalArea_t		*area;
	viewEntity_t		*vEnt;

	area = &portalAreas[ areaNum ];

	for ( int entityIdx : area->entityRefs ) {
		entity = entityDefs[entityIdx];

		// debug tool to allow viewing of only one entity at a time
		if ( r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != entity->index ) {
			continue;
		}
		if ( r_singleEntity.GetInteger() == -2 && strcmp( r_singleModelName.GetString(), entity->parms.hModel->Name() ) )
			continue;

		// remove decals that are completely faded away
		if( entity->decals )
			R_FreeEntityDefFadedDecals( entity, tr.viewDef->renderView.time );

		// check for completely suppressing the model
		if ( !r_skipSuppress.GetBool() ) {
			// nbohr1more: #4379 lightgem culling
			if ( !entity->parms.isLightgem && entity->parms.noShadow && tr.viewDef->IsLightGem() ) { 
				continue; 
			}
			if ( entity->parms.suppressSurfaceInViewID && entity->parms.suppressSurfaceInViewID == tr.viewDef->renderView.viewID ) {
				continue;
			}
			if ( entity->parms.allowSurfaceInViewID && entity->parms.allowSurfaceInViewID != tr.viewDef->renderView.viewID ) {
				continue;
			}
		}

		// cull reference bounds
		if ( CullEntityByPortals( entity, ps ) ) {
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}
		vEnt = R_SetEntityDefViewEntity( entity );

		// possibly expand the scissor rect
		vEnt->scissorRect.Union( ps->rect );
	}
}

/*
================
CullLightByPortals

Return true if the light frustum does not intersect the current portal chain.
The last stack plane is not used because lights are not near clipped.
================
*/
bool idRenderWorldLocal::CullLightByPortals( const idRenderLightLocal *light, const portalStack_t *ps ) {
	if ( r_useLightPortalCulling.GetInteger() == 1 ) {

		ALIGNTYPE16 frustumCorners_t corners;
		//idRenderMatrix::GetFrustumCorners( corners, light->inverseBaseLightProject, bounds_zeroOneCube );
		// by construction, see R_PolytopeSurfaceFrustumLike
		assert( light->frustumTris && light->frustumTris->numVerts == 8 );
		for ( int v = 0; v < 8; v++ ) {
			idVec3 pos = light->frustumTris->verts[v].xyz;
			corners.x[v] = pos.x;
			corners.y[v] = pos.y;
			corners.z[v] = pos.z;
		}
		for ( int i = 0; i < ps->numPortalPlanes; i++ ) {
			if ( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT ) {
				return true;
			}
		}
	} else if ( r_useLightPortalCulling.GetInteger() >= 2 ) {

		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

		// exact clip of light faces against all planes
		for ( int i = 0; i < 6; i++ ) {
			// the light frustum planes face inward, so the planes that have the
			// view origin on the positive side will be the "back" faces of the light,
			// which must have some fragment inside the the portal stack planes to be visible
			if ( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f ) {
				continue;
			}

			// calculate a winding for this frustum side
			idFixedWinding w;
			w.BaseForPlane( frustumPlanes[i] );
			for ( int j = 0; j < 6; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) ) {
					break;
				}
			}

			if ( w.GetNumPoints() <= 2 ) {
				continue;
			}
			assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
			assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

			// now clip the winding against each of the portalStack planes
			// skip the last plane which is the last portal itself
			for ( int j = 0; j < ps->numPortalPlanes - 1; j++ ) {
				if ( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) ) {
					break;
				}
			}

			if ( w.GetNumPoints() > 2 ) {
				// part of the winding is visible through the portalStack,
				// so the light is not culled
				return false;
			}
		}

		// nothing was visible
		return true;
	}
	return false;
}

/*
===================
AddAreaLightRefs

This is the only point where lights get added to the viewLights list
===================
*/
void idRenderWorldLocal::AddAreaLightRefs( int areaNum, const portalStack_t *ps ) {
	portalArea_t		*area;
	idRenderLightLocal	*light;
	viewLight_t			*vLight;

	area = &portalAreas[ areaNum ];

	for ( int lightIdx : area->lightRefs ) {
		light = lightDefs[lightIdx];


		// debug tool to allow viewing of only one light at a time
		if ( r_singleLight.GetInteger() >= 0 && r_singleLight.GetInteger() != light->index ) {
			continue;
		}
		
		// nbohr1more: disable the player in void light optimization when light area culling is disabled
		if ( r_useLightAreaCulling.GetInteger() ) {
			if ( tr.viewDef->areaNum < 0 && !light->lightShader->IsAmbientLight() )
				continue;
		}

		// check for being closed off behind a door
		// stgatilov #5172: there are many conditions when this should not be done, we just set areaNum = -1 in bad cases
		if ( r_useLightAreaCulling.GetInteger() &&
			light->areaNum != -1 && !tr.viewDef->connectedAreas[light->areaNum]
		) {
			// a light that doesn't cast shadows will still light even if it is behind a door
			assert( !light->parms.noShadows && light->lightShader->LightCastsShadows() );
			continue;
		}

		// cull frustum
		if ( CullLightByPortals( light, ps ) ) {
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}

		vLight = R_SetLightDefViewLight( light );

		// expand the scissor rect
		vLight->scissorRect.Union( ps->rect );
	}
}

/*
===================
AddAreaRefs

This may be entered multiple times with different planes
if more than one portal sees into the area
===================
*/
void idRenderWorldLocal::AddAreaRefs( int areaNum, const portalStack_t *ps ) {
	// mark the viewCount, so r_showPortals can display the
	// considered portals
	portalAreas[ areaNum ].areaViewCount = tr.viewCount;

	// add the models and lights, using more precise culling to the planes
	AddAreaEntityRefs( areaNum, ps );
	AddAreaLightRefs( areaNum, ps );
}

/*
===================
BuildConnectedAreas_r
===================
*/
void idRenderWorldLocal::BuildConnectedAreas_r( int areaNum ) {
	portalArea_t	*area;
	//portal_t		*portal;

	if ( tr.viewDef->connectedAreas[areaNum] ) {
		return;
	}
	tr.viewDef->connectedAreas[areaNum] = true;

	// flood through all non-blocked portals
	area = &portalAreas[ areaNum ];
	for ( auto const &portal : area->areaPortals ) {
		if ( !( portal->doublePortal->blockingBits & PS_BLOCK_VIEW ) ) {
			BuildConnectedAreas_r( portal->intoArea );
		}
	}
}

/*
===================
BuildConnectedAreas

This is only valid for a given view, not all views in a frame
===================
*/
void idRenderWorldLocal::BuildConnectedAreas( void ) {
	int		i;

	tr.viewDef->connectedAreas = ( bool * )R_FrameAlloc( portalAreas.Num()
	                             * sizeof( tr.viewDef->connectedAreas[0] ) );

	// if we are outside the world, we can see all areas
	if ( tr.viewDef->areaNum == -1 ) {
		for ( i = 0; i < portalAreas.Num(); i++ ) {
			tr.viewDef->connectedAreas[i] = true;
		}
		return;
	}

	// start with none visible, and flood fill from the current area
	memset( tr.viewDef->connectedAreas, 0, portalAreas.Num() * sizeof( tr.viewDef->connectedAreas[0] ) );
	BuildConnectedAreas_r( tr.viewDef->areaNum );
}

/*
=============
FindViewLightsAndEntites

All the modelrefs and lightrefs that are in visible areas
will have viewEntitys and viewLights created for them.

The scissorRects on the viewEntitys and viewLights may be empty if
they were considered, but not actually visible.
=============
*/
void idRenderWorldLocal::FindViewLightsAndEntities( void ) {
	TRACE_CPU_SCOPE( "FindViewLightsAndEntities" )

	// clear the visible lightDef and entityDef lists
	tr.viewDef->viewLights = nullptr;
	tr.viewDef->viewEntitys = nullptr;

	// find the area to start the portal flooding in
	if ( !r_usePortals.GetBool() ) {
		// debug tool to force no portal culling
		tr.viewDef->areaNum = -1;
	} else {
		tr.viewDef->areaNum = GetAreaAtPoint( tr.viewDef->initialViewAreaOrigin );
	}

	// determine all possible connected areas for
	// light-behind-door culling
	BuildConnectedAreas();

	// flow through all the portals and add models / lights
	if ( r_singleArea.GetBool() ) {
		// if debugging, only mark this area
		// if we are outside the world, don't draw anything
		if ( tr.viewDef->areaNum >= 0 ) {
			portalStack_t	ps;
			int				i;
			static int		lastPrintedAreaNum;

			if ( tr.viewDef->areaNum != lastPrintedAreaNum ) {
				lastPrintedAreaNum = tr.viewDef->areaNum;
				common->Printf( "entering portal area %i\n", tr.viewDef->areaNum );
			}

			for ( i = 0 ; i < 5 ; i++ ) {
				ps.portalPlanes[i] = tr.viewDef->frustum[i];
			}
			ps.numPortalPlanes = 5;
			ps.rect = tr.viewDef->scissor;

			AddAreaRefs( tr.viewDef->areaNum, &ps );
		}
	} else {
		// note that the center of projection for flowing through portals may
		// be a different point than initialViewAreaOrigin for subviews that
		// may have the viewOrigin in a solid/invalid area
		FlowViewThroughPortals( tr.viewDef->renderView.vieworg, 5, tr.viewDef->frustum );
	}
}

/*
==============
NumPortals
==============
*/
int idRenderWorldLocal::NumPortals( void ) const {
	return doublePortals.Num();
}

/*
==============
DoesVisportalContactBox

stgatilov #5354: checks whether idPortalEntity with given box applies to visportal with given winding.
This function is called from:
  idRenderWorldLocal::FindPortal --- assignment of portal entities to portals during game
  CheckInfoLocations --- static validation of info_locations with info_locationseparator-s
==============
*/
bool idRenderWorldLocal::DoesVisportalContactBox( const idWinding &visportalWinding, const idBounds &box ) {
	idBounds visportalBox;
	visportalBox.Clear();
	for ( int j = 0 ; j < visportalWinding.GetNumPoints() ; j++ ) {
		visportalBox.AddPoint( visportalWinding[j].ToVec3() );
	}

	return visportalBox.IntersectsBounds( box );
}

/*
==============
FindPortal

Game code uses this to identify which portals are inside doors.
Returns 0 if no portal contacts the bounds
==============
*/
qhandle_t idRenderWorldLocal::FindPortal( const idBounds &b ) const {
	for ( int i = 0; i < doublePortals.Num(); i++ ) {
		const doublePortal_t &portal = doublePortals[i];
		const idWinding &w = portal.portals[0].w;

		if ( DoesVisportalContactBox( w, b ) )
			return i + 1;
	}
	return 0;
}

/*
=============
FloodConnectedAreas
=============
*/
void idRenderWorldLocal::FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex ) {
	if ( area->connectedAreaNum[portalAttributeIndex] == connectedAreaNum ) {
		return;
	}
	area->connectedAreaNum[portalAttributeIndex] = connectedAreaNum;

	for ( auto p : area->areaPortals ) {
		if ( !( p->doublePortal->blockingBits & ( 1 << portalAttributeIndex ) ) ) {
			FloodConnectedAreas( &portalAreas[p->intoArea], portalAttributeIndex );
		}
	}
}

/*
==============
AreasAreConnected

==============
*/
bool idRenderWorldLocal::AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) {
	if ( areaNum1 == -1 || areaNum2 == -1 ) {
		return false;
	}

	if ( areaNum1 > portalAreas.Num() || areaNum2 > ( int )portalAreas.Num() || areaNum1 < 0 || areaNum2 < 0 ) {
		common->Error( "idRenderWorldLocal::AreAreasConnected: bad parms: %i, %i", areaNum1, areaNum2 );
	}
	int	attribute = 0;
	int	intConnection = ( int )connection;

	while ( intConnection > 1 ) {
		attribute++;
		intConnection >>= 1;
	}

	if ( attribute >= NUM_PORTAL_ATTRIBUTES || ( 1 << attribute ) != ( int )connection ) {
		common->Error( "idRenderWorldLocal::AreasAreConnected: bad connection number: %i\n", ( int )connection );
	}
	return portalAreas[areaNum1].connectedAreaNum[attribute] == portalAreas[areaNum2].connectedAreaNum[attribute];
}


/*
==============
SetPortalState

doors explicitly close off portals when shut
==============
*/
void idRenderWorldLocal::SetPortalState( qhandle_t portal, int blockTypes ) {
	if ( portal == 0 ) {
		return;
	}

	if ( portal < 1 || portal > doublePortals.Num() ) {
		common->Error( "SetPortalState: bad portal number %i", portal );
	}
	int	old = doublePortals[portal - 1].blockingBits;

	if ( old == blockTypes ) {
		return;
	}
	doublePortals[portal - 1].blockingBits = blockTypes;

	// leave the connectedAreaGroup the same on one side,
	// then flood fill from the other side with a new number for each changed attribute
	for ( int i = 0 ; i < NUM_PORTAL_ATTRIBUTES ; i++ ) {
		if ( ( old ^ blockTypes ) & ( 1 << i ) ) {
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[doublePortals[portal - 1].portals[1].intoArea], i );
		}
	}

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_SET_PORTAL_STATE );
		session->writeDemo->WriteInt( portal );
		session->writeDemo->WriteInt( blockTypes );
	}
}

/*
==============
GetPortalState
==============
*/
int		idRenderWorldLocal::GetPortalState( qhandle_t portal ) {
	if ( portal == 0 ) {
		return 0;
	}

	if ( portal < 1 || portal > doublePortals.Num() ) {
		common->Error( "GetPortalState: bad portal number %i", portal );
	}
	return doublePortals[portal - 1].blockingBits;
}

idPlane idRenderWorldLocal::GetPortalPlane( qhandle_t portal ) {
	if ( portal == 0 ) {
		return plane_origin;
	}

	if ( portal < 1 || portal > doublePortals.Num() ) {
		common->Error( "GetPortalState: bad portal number %i", portal );
	}
	return doublePortals[portal - 1].portals[0].plane;
}

/*
=====================
idRenderWorldLocal::ShowPortals

Originally calculated and rendered in backend. Converted to only generate results to frame data.
=====================
*/
void idRenderWorldLocal::ShowPortals() {
	idStr results;
	// Check viewcount on areas and portals to see which got used this frame.
	for ( auto &area : portalAreas ) {
		if ( area.areaViewCount != tr.viewCount ) {
			results += 'C';
			continue;
		} else
			results += 'O';
		for ( auto p : area.areaPortals ) {
			// Changed to show 3 colours. -- SteveL #4162
			if ( p->doublePortal->portalViewCount == tr.viewCount )
				results += 'G';
			else if ( portalAreas[p->intoArea].areaViewCount == tr.viewCount )
				results += 'Y';
			else
				results += 'R';
		}
	}
	auto size = sizeof( char ) * results.Length();
	tr.viewDef->portalStates = (char*)R_FrameAlloc( size );
	memcpy( tr.viewDef->portalStates, results.c_str(), size );
}
