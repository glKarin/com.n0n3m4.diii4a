// RenderShadows.cpp
//

#include "RenderSystem_local.h"

rvmRenderShadowSystem renderShadowSystem;

idCVar rvmRenderShadowSystem::r_shadowMapAtlasSize("r_shadowMapAtlasSize", "16384", CVAR_RENDERER | CVAR_INTEGER | CVAR_ROM, "size of the shadowmap atlas"); // karin: 16384, GL_OUT_OF_MEMORY where glClear(GL_DEPTH_BUFFER_BIT)
idCVar rvmRenderShadowSystem::r_shadowMapAtlasSliceSize("r_shadowMapAtlasSliceSize", "1024", CVAR_RENDERER | CVAR_INTEGER | CVAR_ROM, "size of the shadow map atlas slice size");
idCVar rvmRenderShadowSystem::r_shadowMapEvictionTime("r_shadowMapEvictionTime", "5", CVAR_RENDERER | CVAR_INTEGER | CVAR_ROM, "shadow map eviction time in seconds");

#ifdef ID_ALLOW_TOOLS
/*
======================
R_NukeShadowCache_f
======================
*/
void R_NukeShadowCache_f(const idCmdArgs& args) {
	renderShadowSystem.NukeShadowMapCache();
}
#endif

/*
======================
rvmRenderShadowSystem::rvmRenderShadowAtlasEntry_t
======================
*/
rvmRenderShadowAtlasEntry_t::rvmRenderShadowAtlasEntry_t() {
	Reset();
}

/*
======================
rvmRenderShadowSystem::Reset
======================
*/
void rvmRenderShadowAtlasEntry_t::Reset(void) {
	x = -1;
	y = -1;
	sliceSizeX = -1;
	sliceSizeY = -1;
	textureX = -1;
	textureY = -1;
	uniqueLightId = -1;
	lastTouchedTime = -1;
}

/*
======================
rvmRenderShadowSystem::Mark
======================
*/
void rvmRenderShadowAtlasEntry_t::Mark(int uniqueLightId) {
	this->uniqueLightId = uniqueLightId;
	lastTouchedTime = Sys_GetClockTicks() + (Sys_ClockTicksPerSecond() * rvmRenderShadowSystem::r_shadowMapEvictionTime.GetInteger());
}


/*
======================
rvmRenderShadowSystem::rvmRenderShadowSystem
======================
*/
rvmRenderShadowSystem::rvmRenderShadowSystem() {
	shadowMapAtlasRT = NULL;
	shadowAtlasEntries = NULL;
}

/*
======================
rvmRenderShadowSystem::Init
======================
*/
void rvmRenderShadowSystem::Init(void) {
	common->Printf("...Init Render Shadow System...\n");

	// Create the shadow map atlas image.
	{
		idImageOpts opts;
		opts.format = FMT_DEPTH;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = r_shadowMapAtlasSize.GetInteger();
		opts.height = r_shadowMapAtlasSize.GetInteger();
		opts.numMSAASamples = 0;

		idImage* depthImage = renderSystem->CreateImage("_shadowMapAtlas", &opts, TF_LINEAR);

		shadowMapAtlasRT = renderSystem->AllocRenderTexture("shadowRT", nullptr, depthImage, nullptr, nullptr, nullptr);			
	}

#ifdef ID_ALLOW_TOOLS
	cmdSystem->AddCommand("nukeShadowCache", R_NukeShadowCache_f, CMD_FL_TOOL, "nukes shadow map cache");
#endif

	// Allocate our shadow map atlas slice entry table.
	int numEntriesPerAxis = r_shadowMapAtlasSize.GetInteger() / r_shadowMapAtlasSliceSize.GetInteger();
	shadowAtlasEntries = new rvmRenderShadowAtlasEntry_t[numEntriesPerAxis * numEntriesPerAxis];
 
	// Check to make sure the cvars are setup correctly.
	if(numEntriesPerAxis <= 0) {
		common->FatalError("Your shadow map cvars are set incorrectly r_shadowMapAtlasSize: %d r_shadowMapAtlasSliceSize: %d\n", r_shadowMapAtlasSize.GetInteger(), r_shadowMapAtlasSliceSize.GetInteger());
	}

	common->Printf("...Shadow Map Slice Table %dx%d\n", numEntriesPerAxis, numEntriesPerAxis);

	for(int y = 0; y < numEntriesPerAxis; y++) {
		for(int x = 0; x < numEntriesPerAxis; x++) {
			rvmRenderShadowAtlasEntry_t* entry = &shadowAtlasEntries[(y * numEntriesPerAxis) + x];

			entry->x = x * r_shadowMapAtlasSliceSize.GetInteger();
			entry->y = y * r_shadowMapAtlasSliceSize.GetInteger();

			entry->sliceSizeX = r_shadowMapAtlasSliceSize.GetInteger();
			entry->sliceSizeY = r_shadowMapAtlasSliceSize.GetInteger();

			entry->textureX = (float)entry->x / (float)r_shadowMapAtlasSize.GetInteger();
			entry->textureY = (float)entry->y / (float)r_shadowMapAtlasSize.GetInteger();
		}
	}

	// Create the atlas lookup texture.
	{
		idImageOpts opts;
		opts.format = FMT_RG32;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = numEntriesPerAxis * numEntriesPerAxis;
		opts.height = 1;
		opts.numMSAASamples = 0;

		atlasEntriesLookupTexture = renderSystem->CreateImage("_shadowMapAtlasLookup", &opts, TF_NEAREST);
		
		idTempArray<idVec2>	rawData(opts.width);

		for(int i = 0; i < opts.width; i++) {
			rawData[i].x = shadowAtlasEntries[i].textureX;
			rawData[i].y = shadowAtlasEntries[i].textureY;
		}

		atlasEntriesLookupTexture->SubImageUpload(0, 0, 0, 0, opts.width, 1, (const void*)rawData.Ptr());
	}
}

/*
======================
rvmRenderShadowSystem::FindNextAvailableShadowMap
======================
*/
int rvmRenderShadowSystem::FindNextAvailableShadowMap(idRenderLightCommitted* vLight, int numSlices) {
	int numEntriesPerAxis = r_shadowMapAtlasSize.GetInteger() / r_shadowMapAtlasSliceSize.GetInteger();

	for (int i = 0; i < numEntriesPerAxis * numEntriesPerAxis; i++) {
		// If we don't have enough shadow maps available return false. 
		if (i + numSlices >= numEntriesPerAxis * numEntriesPerAxis)
			return -1;

		if(shadowAtlasEntries[i].lastTouchedTime < Sys_GetClockTicks()) {
			for (int d = 0; d < numSlices; d++) { // TODO handle variable length slices!!
				shadowAtlasEntries[i + d].Mark(vLight->lightDef->parms.uniqueLightId);
			}
			return i;
		}
	}

	return -1;
}

/*
======================
rvmRenderShadowSystem::CheckShadowCache
======================
*/
int rvmRenderShadowSystem::CheckShadowCache(idRenderLightCommitted* vLight) {
	if (vLight->lightDef->parms.uniqueLightId == -1)
		return -1;

	int numEntriesPerAxis = r_shadowMapAtlasSize.GetInteger() / r_shadowMapAtlasSliceSize.GetInteger();
	for (int i = 0; i < numEntriesPerAxis * numEntriesPerAxis; i++) {
		if(shadowAtlasEntries[i].uniqueLightId == vLight->lightDef->parms.uniqueLightId) {
			shadowAtlasEntries[i].Mark(vLight->lightDef->parms.uniqueLightId);
			return i;
		}
	}

	return -1;
}

/*
======================
rvmRenderShadowSystem::Shutdown
======================
*/
void rvmRenderShadowSystem::Shutdown(void) {
	if(shadowMapAtlasRT != NULL) {
		delete shadowMapAtlasRT;
		shadowMapAtlasRT = NULL;
	}

	if(shadowAtlasEntries != NULL) {
		delete shadowAtlasEntries;
		shadowAtlasEntries = NULL;
	}
}

/*
==========================
rvmRenderShadowSystem::NukeShadowMapCache
==========================
*/
void rvmRenderShadowSystem::NukeShadowMapCache(void) {
	int numEntriesPerAxis = r_shadowMapAtlasSize.GetInteger() / r_shadowMapAtlasSliceSize.GetInteger();
	for (int i = 0; i < numEntriesPerAxis * numEntriesPerAxis; i++) {
		shadowAtlasEntries[i].uniqueLightId = -1;
		shadowAtlasEntries[i].lastTouchedTime = -1;
	}
}

/*
==========================
rvmRenderShadowSystem::InvalidateShadowMapForLight
==========================
*/
void rvmRenderShadowSystem::InvalidateShadowMapForLight(int uniqueLightId) {
	int numEntriesPerAxis = r_shadowMapAtlasSize.GetInteger() / r_shadowMapAtlasSliceSize.GetInteger();
	for (int i = 0; i < numEntriesPerAxis * numEntriesPerAxis; i++) {
		if (shadowAtlasEntries[i].uniqueLightId == uniqueLightId) {
			shadowAtlasEntries[i].uniqueLightId = -1;
			shadowAtlasEntries[i].lastTouchedTime = -1;
			return;
		}
	}

	common->Warning("InvalidateShadowMapForLight: %d is not present in the cache!", uniqueLightId);
}
