/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

idRenderEntityLocal::idRenderEntityLocal()
{
	memset(&parms, 0, sizeof(parms));
	memset(modelMatrix, 0, sizeof(modelMatrix));

	world					= NULL;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= NULL;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= NULL;
	referenceBounds			= bounds_zero;
	viewCount				= 0;
	viewEntity				= NULL;
	visibleCount			= 0;
	decals					= NULL;
	overlay					= NULL;
	entityRefs				= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
	needsPortalSky			= false;
#ifdef _D3BFG_CULLING
    globalReferenceBounds	= bounds_zero;
#endif
#ifdef _SPLASHDAMAGE //karin: save last call renderEntity_t::callback game time
	lastModifiedGameTime	= -1;
	imposterModel			= NULL;
#endif
}

void idRenderEntityLocal::FreeRenderEntity()
{
}

void idRenderEntityLocal::UpdateRenderEntity(const renderEntity_t *re, bool forceUpdate)
{
}

void idRenderEntityLocal::GetRenderEntity(renderEntity_t *re)
{
}

void idRenderEntityLocal::ForceUpdate()
{
}

int idRenderEntityLocal::GetIndex()
{
	return index;
}

void idRenderEntityLocal::ProjectOverlay(const idPlane localTextureAxis[2], const idMaterial *material)
{
}
void idRenderEntityLocal::RemoveDecals()
{
}

//======================================================================

idRenderLightLocal::idRenderLightLocal()
{
	memset(&parms, 0, sizeof(parms));
	memset(modelMatrix, 0, sizeof(modelMatrix));
	memset(shadowFrustums, 0, sizeof(shadowFrustums));
	memset(lightProject, 0, sizeof(lightProject));
	memset(frustum, 0, sizeof(frustum));
	memset(frustumWindings, 0, sizeof(frustumWindings));

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	frustumTris				= NULL;
	numShadowFrustums		= 0;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
#if defined(_SHADOW_MAPPING) || defined(_D3BFG_CULLING)
    ID_RENDER_MATRIX_CAST(baseLightProject).Identity();
    ID_RENDER_MATRIX_CAST(inverseBaseLightProject).Identity();
#endif
#ifdef _D3BFG_CULLING
    globalLightBounds.Zero();
    //anon begin
    ID_RENDER_MATRIX_CAST(inverseBaseLightProject).Zero();
    //anon end
#endif
}

void idRenderLightLocal::FreeRenderLight()
{
}
void idRenderLightLocal::UpdateRenderLight(const renderLight_t *re, bool forceUpdate)
{
}
void idRenderLightLocal::GetRenderLight(renderLight_t *re)
{
}
void idRenderLightLocal::ForceUpdate()
{
}
int idRenderLightLocal::GetIndex()
{
	return index;
}

#ifdef _SPLASHDAMAGE //karin: entity inst
void idRenderEntityLocal::FreeInstanceList(void) {
	if(!instList.Num())
		return;

	for (int i = 0; i < instList.Num(); i++) {
		world->FreeEntityDef(instList[i]);
	}

	instList.Clear();
}

void idRenderEntityLocal::CreateInstanceList(void) {
	const sdInstInfo *inst;

	if (!parms.flags.pushByInstances || parms.numInsts == 0 || !parms.insts) {
		FreeInstanceList();
		return;
	}

	if (instList.Num() == parms.numInsts)
		return;

	FreeInstanceList();
	instList.SetNum(parms.numInsts);

	inst = parms.insts;
	for (int i = 0; i < parms.numInsts; i++, inst++) {
		renderEntity_t re = parms;
		re.origin = inst->inst.origin;
		re.axis = inst->inst.axis;
		if (parms.flags.overridenBounds && parms.hModel) {
			re.bounds = parms.hModel->Bounds();
		}
		// re.shaderParms[SHADERPARM_RED] = (float)inst->inst.color[0] / 255.0f;
		// re.shaderParms[SHADERPARM_GREEN] = (float)inst->inst.color[1] / 255.0f;
		// re.shaderParms[SHADERPARM_BLUE] = (float)inst->inst.color[2] / 255.0f;
		// re.shaderParms[SHADERPARM_ALPHA] = (float)inst->inst.color[3] / 255.0f;
		re.numInsts = 0;
		re.insts = NULL;
		re.flags.pushByOrigin = false;
		re.flags.pushByCenter = false;
		re.flags.pushByInstances = false;
		re.flags.overridenBounds = false;

		re.minVisDist = inst->minVisDist;
		re.maxVisDist = inst->maxVisDist;

		instList[i] = world->AddEntityDef(&re);
	}
}

void idRenderEntityLocal::UpdateInstanceList(void) {
	const sdInstInfo *inst;
	CreateInstanceList();

	if(!instList.Num())
		return;

	inst = parms.insts;
	for (int i = 0; i < instList.Num(); i++, inst++) {
		renderEntity_t re = parms;
		re.origin = inst->inst.origin;
		re.axis = inst->inst.axis;
		if (parms.flags.overridenBounds && parms.hModel) {
			re.bounds = parms.hModel->Bounds();
		}
		// re.shaderParms[SHADERPARM_RED] = (float)inst->inst.color[0] / 255.0f;
		// re.shaderParms[SHADERPARM_GREEN] = (float)inst->inst.color[1] / 255.0f;
		// re.shaderParms[SHADERPARM_BLUE] = (float)inst->inst.color[2] / 255.0f;
		// re.shaderParms[SHADERPARM_ALPHA] = (float)inst->inst.color[3] / 255.0f;
		re.numInsts = 0;
		re.insts = NULL;
		re.flags.pushByOrigin = false;
		re.flags.pushByCenter = false;
		re.flags.pushByInstances = false;
		re.flags.overridenBounds = false;

		re.minVisDist = inst->minVisDist;
		re.maxVisDist = inst->maxVisDist;

		world->UpdateEntityDef(instList[i], &re);
	}
}

// renderEntity.flags.pushByCenter = spawnArgs.GetBool( "pushByOrigin" ); in game
idVec3 idRenderEntityLocal::GetVisDistOrigin(void) const
{
	idVec3 origin;
	if (parms.hModel)
	{
		if(!idStr::Icmpn(parms.hModel->Name(), "_lodentity_", 11))
		{
			if(parms.flags.pushByCenter && !parms.origin.IsZero())
				origin = parms.origin;
			else
			{
				if(parms.flags.overridenBounds)
					origin = parms.hModel->Bounds(&parms).GetCenter();
				else
					origin = parms.bounds.GetCenter();
			}
		}
		else if(!idStr::Icmpn(parms.hModel->Name(), "_area", 5))
		{
			if(parms.flags.overridenBounds)
				origin = parms.hModel->Bounds(&parms).GetCenter();
			else
				origin = parms.bounds.GetCenter();
		}
		else
		{
			origin = parms.origin;
		}
	}
	else // parms.callback != NULL
	{
		/*
		   if(parms.flags.pushByOrigin)
		   origin = parms.origin;
		   else if(parms.flags.pushByCenter)
		   origin = parms.origin;
		   else if(!parms.bounds.IsCleared())
		   origin = parms.bounds.GetCenter();
		   else
		   */
		origin = parms.origin;
	}

	return origin;
}

idBounds idRenderEntityLocal::GetVisDistWorldBounds(idRenderModel *model) const
{
	if(!model/* && !parms.callback*/)
		model = parms.hModel;

	if (model)
	{
		if(!idStr::Icmpn(model->Name(), "_lodentity_", 11))
		{
			if(parms.flags.pushByCenter && !parms.origin.IsZero())
			{
				idBounds bounds = model->Bounds(&parms);
				if(bounds.IsCleared())
					return idBounds(parms.origin);
				bounds.RotateSelf(parms.axis);
				bounds.TranslateSelf(parms.origin);
				return bounds;
			}
			else
			{
				if(parms.flags.overridenBounds)
				{
					idBounds bounds = model->Bounds(&parms);
					return bounds.IsCleared() ? idBounds(parms.origin) : bounds;
				}
				else
					return parms.bounds;
			}
		}
		else if(!idStr::Icmpn(model->Name(), "_area", 5))
		{
			return model->Bounds(&parms);
		}
		else
		{
			idBounds bounds = model->Bounds(&parms);
			if(bounds.IsCleared())
				return idBounds(parms.origin);
			bounds.RotateSelf(parms.axis);
			bounds.TranslateSelf(parms.origin);
			return bounds;
		}
	}
	else // parms.callback != NULL
	{
		return parms.bounds;
	}
}

#endif
