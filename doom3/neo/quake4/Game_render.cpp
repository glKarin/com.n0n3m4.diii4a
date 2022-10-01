// Game_render.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

idCVar g_renderCasUpscale("g_renderCasUpscale", "1", CVAR_BOOL, "jmarshall: toggles cas upscaling");

/*
=======================================

Game Render

The engine renderer is designed to do two things, generate the geometry pass, and the shadow passes. The pipeline,
including post process, is now handled in the game code. This allows more granular control over how the final pixels,
are presented on screen based on whatever is going on in game.

=======================================
*/

/*
========================
idGameLocal::InitGameRenderSystem
========================
*/
void idGameLocal::InitGameRenderSystem(void) {
	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = renderSystem->GetScreenWidth();
		opts.height = renderSystem->GetScreenHeight();
		opts.numMSAASamples = 4; // renderSystem->GetNumMSAASamples();

		idImage *albedoImage = renderSystem->CreateImage("_forwardRenderAlbedo", &opts, TF_LINEAR);
		idImage *emissiveImage = renderSystem->CreateImage("_forwardRenderEmissive", &opts, TF_LINEAR);

		opts.numMSAASamples = 4; // renderSystem->GetNumMSAASamples();
		opts.format = FMT_DEPTH_STENCIL;
		idImage *depthImage = renderSystem->CreateImage("_forwardRenderDepth", &opts, TF_LINEAR);

		gameRender.forwardRenderPassRT = renderSystem->CreateRenderTexture(albedoImage, depthImage, emissiveImage);
	}

	for(int i = 0; i < 2; i++)
	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = renderSystem->GetScreenWidth();
		opts.height = renderSystem->GetScreenHeight();
		opts.numMSAASamples = 4; // renderSystem->GetNumMSAASamples();

		idImage* albedoImage = renderSystem->CreateImage(va("_postProcessAlbedo%d", i), &opts, TF_LINEAR);

		opts.numMSAASamples = 4; // renderSystem->GetNumMSAASamples();
		opts.format = FMT_DEPTH_STENCIL;
		idImage* depthImage = renderSystem->CreateImage(va("_postProcessDepth%d", i), &opts, TF_LINEAR);

		gameRender.postProcessRT[i] = renderSystem->CreateRenderTexture(albedoImage, depthImage, NULL);
	}

	{
		idImageOpts opts;
		opts.format = FMT_RGBA8;
		opts.colorFormat = CFM_DEFAULT;
		opts.numLevels = 1;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.width = renderSystem->GetScreenWidth();
		opts.height = renderSystem->GetScreenHeight();
		opts.numMSAASamples = 0;

		idImage *albedoImage = renderSystem->CreateImage("_forwardRenderResolvedAlbedo", &opts, TF_LINEAR);
		idImage *emissiveImage = renderSystem->CreateImage("_forwardRenderResolvedEmissive", &opts, TF_LINEAR);
		opts.format = FMT_DEPTH;
		idImage *depthImage = renderSystem->CreateImage("_forwardRenderResolvedDepth", &opts, TF_LINEAR);

		gameRender.forwardRenderPassResolvedRT = renderSystem->CreateRenderTexture(albedoImage, depthImage, emissiveImage);
	}

	gameRender.blackPostProcessMaterial = declManager->FindMaterial("postprocess/black", false);
	gameRender.noPostProcessMaterial = declManager->FindMaterial("postprocess/nopostprocess", false);
	gameRender.casPostProcessMaterial = declManager->FindMaterial("postprocess/casupscale", false);
	gameRender.resolvePostProcessMaterial = declManager->FindMaterial("postprocess/resolvepostprocess", false);
}

/*
========================
idGameLocal::ResizeRenderTextures
========================
*/
void idGameLocal::ResizeRenderTextures(int width, int height) {
	// Resize all of the different render textures.
	renderSystem->ResizeRenderTexture(gameRender.forwardRenderPassRT, width, height);
	renderSystem->ResizeRenderTexture(gameRender.forwardRenderPassResolvedRT, width, height);
}

/*
====================
idGameLocal::RenderScene
====================
*/
void idGameLocal::RenderScene(const renderView_t *view, idRenderWorld *renderWorld, idCamera* portalSky) {
	// Minimum render is used for screen captures(such as envcapture) calls, caller is responsible for all rendertarget setup.
	//if (view->minimumRender)
	//{
	//	RenderSky(view);
	//	if (view->cubeMapTargetImage)
	//	{
	//		renderView_t worldRefDef = *view;
	//		worldRefDef.cubeMapClearBuffer = false;
	//		renderWorld->RenderScene(&worldRefDef);
	//	}
	//	else
	//	{
	//		renderWorld->RenderScene(view);
	//	}
	//
	//	return;
	//}

	// Make sure all of our render textures are the right dimensions for this render.
	ResizeRenderTextures(renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight());

	// Render the scene to the forward render pass rendertexture.
	renderSystem->BindRenderTexture(gameRender.forwardRenderPassRT, nullptr);
	{
		// Clear the color/depth buffers
		renderSystem->ClearRenderTarget(true, true, 1.0f, 0.0f, 0.0f, 0.0f);
	
		// Render our sky first.
		if (portalSky) {
			renderView_t portalSkyView = *view;
			portalSky->GetViewParms(&portalSkyView);
			gameRenderWorld->RenderScene(&portalSkyView);
		}
	
		// Render the current world.
		renderWorld->RenderScene(view);
	}
	renderSystem->BindRenderTexture(nullptr, nullptr);

	// Resolve our MSAA buffer.
	renderSystem->ResolveMSAA(gameRender.forwardRenderPassRT, gameRender.forwardRenderPassResolvedRT);

	// Render the resolved buffer to the screen.
	renderSystem->BindRenderTexture(gameRender.postProcessRT[0], nullptr);
		renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameRender.resolvePostProcessMaterial);
	renderSystem->BindRenderTexture(nullptr, nullptr);

	// Now render CaS
	if (g_renderCasUpscale.GetBool())
	{
		renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameRender.casPostProcessMaterial);
	}
	else
	{
		renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameRender.noPostProcessMaterial);
	}

	// Copy everything to _currentRender
	renderSystem->CaptureRenderToImage("_currentRender");
}
