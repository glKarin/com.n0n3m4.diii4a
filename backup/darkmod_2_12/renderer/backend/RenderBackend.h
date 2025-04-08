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
#pragma once

#include "renderer/backend/stages/DepthStage.h"
#include "renderer/backend/stages/FrobOutlineStage.h"
#include "renderer/backend/stages/InteractionStage.h"
#include "renderer/backend/stages/ShadowMapStage.h"
#include "renderer/backend/stages/StencilShadowStage.h"
#include "renderer/backend/stages/SurfacePassesStage.h"
#include "renderer/backend/stages/LightPassesStage.h"
#include "renderer/backend/stages/TonemapStage.h"
#include "renderer/backend/stages/VolumetricStage.h"
#include "renderer/tr_local.h"

class FrameBuffer;

class RenderBackend {
public:
	RenderBackend();

	void Init();
	void Shutdown();

	void DrawView( const viewDef_t *viewDef, bool colorIsBackground );
	void DrawLightgem( const viewDef_t *viewDef, byte *lightgemData );

	void EndFrame();

	void Tonemap();

private:
	DepthStage depthStage;
	InteractionStage interactionStage;
	StencilShadowStage stencilShadowStage;
	ShadowMapStage shadowMapStage;
	SurfacePassesStage surfacePassesStage;
	LightPassesStage lightPassesStage;
	FrobOutlineStage frobOutlineStage;
	TonemapStage tonemapStage;

	FrameBuffer *lightgemFbo = nullptr;
	GLuint lightgemPbos[3] = { 0 };
	int currentLightgemPbo = 0;
	bool initialized = false;

	void DrawInteractionsWithShadowMapping( const viewDef_t *viewDef, const viewLight_t *vLight );
	void DrawInteractionsWithStencilShadows( const viewDef_t *viewDef, const viewLight_t *vLight );
	void DrawShadowsAndInteractions( const viewDef_t *viewDef );
};

extern RenderBackend *renderBackend;
