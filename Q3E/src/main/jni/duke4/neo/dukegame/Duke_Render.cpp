// Duke_Render.cpp
//

#include "Gamelib/Game_local.h"

idCVar g_drawVisibleLights("g_drawVisibleLights", "0", CVAR_GAME | CVAR_CHEAT, "");

/*
=====================
dnGameLocal::InitGameRender
=====================
*/
void dnGameLocal::InitGameRender(void)
{
	renderPlatform.frontEndPassRenderTarget = new DnFullscreenRenderTarget("frontEndRenderPass", true, true, true, FMT_RGBAF16, FMT_RGBAF16, FMT_RGBA8);
	renderPlatform.frontEndPassRenderTargetResolved = new DnFullscreenRenderTarget("frontEndRenderPassResolved", true, true, false, FMT_RGBAF16, FMT_RGBAF16, FMT_RGBA8);
	renderPlatform.ssaoRenderTarget = new DnFullscreenRenderTarget("ssaoRenderTarget", true, false, false);

	renderPlatform.upscaleFrontEndResolveMaterial = declManager->FindMaterial("postprocess/upScaleFrontEndResolve", false);
	renderPlatform.ssaoMaterial = declManager->FindMaterial("postprocess/ssao", false);
	renderPlatform.bloomMaterial = declManager->FindMaterial("postprocess/bloom", false);
	renderPlatform.ssaoBlurMaterial = declManager->FindMaterial("postprocess/ssao_blur", false);
	renderPlatform.blackMaterial = declManager->FindMaterial("_black", false);	
}

/*
=====================
dnGameLocal::DrawPortalSky
=====================
*/
void dnGameLocal::DrawPortalSky(renderView_t& hackedView)
{
	if (gamePortalSkyWorld != nullptr && g_enablePortalSky.GetBool()) {
		renderView_t	portalView = hackedView;
		portalView.vieworg = gamePortalSkyWorld->GetPortalSkyCameraPosition();

		// setup global fixup projection vars
		if (1) {
			int vidWidth, vidHeight;
			idVec2 shiftScale;

			renderSystem->GetGLSettings(vidWidth, vidHeight);

			float pot;
			int	 w = vidWidth;
			pot = MakePowerOfTwo(w);
			shiftScale.x = (float)w / pot;

			int	 h = vidHeight;
			pot = MakePowerOfTwo(h);
			shiftScale.y = (float)h / pot;

			hackedView.shaderParms[4] = shiftScale.x;
			hackedView.shaderParms[5] = shiftScale.y;
		}

		gamePortalSkyWorld->RenderScene(&portalView);
		renderSystem->CaptureRenderToImage("_currentRender");

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}
}


/*
=====================
dnGameLocal::Draw
=====================
*/
bool dnGameLocal::Draw(int clientNum) {
	idPlayer* player = GetLocalPlayer();
	const renderView_t* view = player->GetRenderView();

	idUserInterface* hud = player->hud;

	// place the sound origin for the player
	gameSoundWorld->PlaceListener(view->vieworg, view->viewaxis, player->entityNumber + 1);

	// Ensure out render targets are the right size.
	renderPlatform.frontEndPassRenderTarget->Resize(renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight());
	renderPlatform.frontEndPassRenderTargetResolved->Resize(renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight());
	renderPlatform.ssaoRenderTarget->Resize(renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight());

	// Bind our MSAA texture for rendering and clear it out.
	renderPlatform.frontEndPassRenderTarget->Bind();
	renderPlatform.frontEndPassRenderTarget->Clear();	

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis; // *ShakeAxis();

	// Draw the portal sky.
	DrawPortalSky(hackedView);

	// Debug command to draw visible lights
	if (g_drawVisibleLights.GetBool())
	{
		for (int i = 0; i < gameRenderWorld->GetNumRenderLights(); i++)
		{
			const renderLight_t* light = gameRenderWorld->GetRenderLight(i);

			if (!gameRenderWorld->IsRenderLightVisible(i))
			{
				continue;
			}

			idBox box = idBox(light->origin, idVec3(30, 30, 30), light->axis);
			gameRenderWorld->DebugBox(colorGreen, box);
		}
	}

	// do the first render
	gameRenderWorld->RenderScene(&hackedView);

	// Resolve MSAA.
	DnFullscreenRenderTarget::BindNull();
	renderPlatform.frontEndPassRenderTarget->ResolveMSAA(renderPlatform.frontEndPassRenderTargetResolved);

	// Draw the resolved target.
	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, renderPlatform.upscaleFrontEndResolveMaterial);

	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, renderPlatform.bloomMaterial);

	// Render the SSAO to a render target so we can blur it.
	//renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, renderPlatform.ssaoMaterial);

	//renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, renderPlatform.ssaoBlurMaterial);	

	// Finally draw the player hud.
	player->DrawHUD(hud);

	return true;
}
