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


#pragma hdrstop

#include "RenderSystem_local.h"

idRenderEntityLocal::idRenderEntityLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

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
	needsPortalSky			= false;
}


/*
===============
R_CreateEntityRefs

Creates all needed model references in portal areas,
chaining them to both the area and the entityDef.

Bumps tr.viewCount.
===============
*/
void idRenderEntityLocal::CreateEntityRefs(void) {
	int			i;
	idVec3		transformed[8];
	idVec3		v;

	if (!parms.hModel) {
		parms.hModel = renderModelManager->DefaultModel();
	}

	// if the entity hasn't been fully specified due to expensive animation calcs
	// for md5 and particles, use the provided conservative bounds.
	if (parms.callback) {
		referenceBounds = parms.bounds;
	}
	else {
		referenceBounds = parms.hModel->Bounds(&parms);
	}

	// some models, like empty particles, may not need to be added at all
	if (referenceBounds.IsCleared()) {
		return;
	}

	if (r_showUpdates.GetBool() &&
		(referenceBounds[1][0] - referenceBounds[0][0] > 1024 ||
			referenceBounds[1][1] - referenceBounds[0][1] > 1024)) {
		common->Printf("big entityRef: %f,%f\n", referenceBounds[1][0] - referenceBounds[0][0],
			referenceBounds[1][1] - referenceBounds[0][1]);
	}

	for (i = 0; i < 8; i++) {
		v[0] = referenceBounds[i & 1][0];
		v[1] = referenceBounds[(i >> 1) & 1][1];
		v[2] = referenceBounds[(i >> 2) & 1][2];

		R_LocalPointToGlobal(modelMatrix, v, transformed[i]);
	}

	// bump the view count so we can tell if an
	// area already has a reference
	tr.viewCount++;
}



/*
===================
R_FreeEntityDefDerivedData

Used by both RE_FreeEntityDef and RE_UpdateEntityDef
Does not actually free the entityDef.
===================
*/
void idRenderEntityLocal::FreeEntityDefDerivedData(bool keepDecals, bool keepCachedDynamicModel) {
	int i;
	areaReference_t* ref, * next;

	// demo playback needs to free the joints, while normal play
	// leaves them in the control of the game
	if (session->readDemo) {
		if (parms.joints) {
			Mem_Free16(parms.joints);
			parms.joints = NULL;
		}
		if (parms.callbackData) {
			Mem_Free(parms.callbackData);
			parms.callbackData = NULL;
		}
		for (i = 0; i < MAX_RENDERENTITY_GUI; i++) {
			if (parms.gui[i]) {
				delete parms.gui[i];
				parms.gui[i] = NULL;
			}
		}
	}

	// clear the dynamic model if present
	if (dynamicModel) {
		dynamicModel = NULL;
	}

	if (!keepDecals) {
		FreeEntityDefDecals();
		FreeEntityDefOverlay();
	}

	if (!keepCachedDynamicModel) {
		delete cachedDynamicModel;
		cachedDynamicModel = NULL;
	}

	// free the entityRefs from the areas
	for (ref = entityRefs; ref; ref = next) {
		next = ref->ownerNext;

		// unlink from the area
		ref->areaNext->areaPrev = ref->areaPrev;
		ref->areaPrev->areaNext = ref->areaNext;
	}
	entityRefs = NULL;
}



/*
==================
R_ClearEntityDefDynamicModel

If we know the reference bounds stays the same, we
only need to do this on entity update, not the full
R_FreeEntityDefDerivedData
==================
*/
void idRenderEntityLocal::ClearEntityDefDynamicModel(void) {
	// clear the dynamic model if present
	if (dynamicModel) {
		dynamicModel = NULL;
	}
}

/*
===================
idRenderEntityLocal::FreeEntityDefDecals
===================
*/
void idRenderEntityLocal::FreeEntityDefDecals(void) {
	while (decals) {
		idRenderModelDecal* next = decals->Next();
		idRenderModelDecal::Free(decals);
		decals = next;
	}
}

/*
===================
idRenderEntityLocal::FreeEntityDefFadedDecals
===================
*/
void idRenderEntityLocal::FreeEntityDefFadedDecals(int time) {
	decals = idRenderModelDecal::RemoveFadedDecals(decals, time);
}

/*
===================
idRenderEntityLocal::FreeEntityDefOverlay
===================
*/
void idRenderEntityLocal::FreeEntityDefOverlay(void) {
	if (overlay) {
		idRenderModelOverlay::Free(overlay);
		overlay = NULL;
	}
}

void idRenderEntityLocal::FreeRenderEntity() {
}

void idRenderEntityLocal::UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate ) {
}

void idRenderEntityLocal::GetRenderEntity( renderEntity_t *re ) {
}

void idRenderEntityLocal::ForceUpdate() {
}

int idRenderEntityLocal::GetIndex() {
	return index;
}

void idRenderEntityLocal::ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial *material ) {
}
void idRenderEntityLocal::RemoveDecals() {
}

//======================================================================
