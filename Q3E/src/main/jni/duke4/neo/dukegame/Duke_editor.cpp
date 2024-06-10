// Duke_editor.cpp
//

#include "Gamelib/Game_local.h"

/*
======================
idGameEdit::DrawEditor
======================
*/
void idGameEdit::DrawEditor(renderView_t* view, idRenderWorld* editorRenderWorld, float windowWidth, float windowHeight, bool renderMode)
{
	int	frontEnd, backEnd;

	// render it
	renderSystem->BeginFrame(windowWidth, windowHeight);

	// Ensure out render targets are the right size.
	gameLocal.renderPlatform.frontEndPassRenderTarget->Resize(windowWidth, windowHeight);
	gameLocal.renderPlatform.frontEndPassRenderTargetResolved->Resize(windowWidth, windowHeight);
	gameLocal.renderPlatform.ssaoRenderTarget->Resize(windowWidth, windowHeight);

	// Bind our MSAA texture for rendering and clear it out.
	gameLocal.renderPlatform.frontEndPassRenderTarget->Bind();
	gameLocal.renderPlatform.frontEndPassRenderTarget->Clear();

	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameLocal.renderPlatform.blackMaterial);

	// Render the editor world.
	editorRenderWorld->RenderScene(view);

	// Resolve MSAA.
	DnFullscreenRenderTarget::BindNull();
	gameLocal.renderPlatform.frontEndPassRenderTarget->ResolveMSAA(gameLocal.renderPlatform.frontEndPassRenderTargetResolved);

	// Draw the resolved target.
	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameLocal.renderPlatform.upscaleFrontEndResolveMaterial);

	// Render post process if we are in lit mode. 
	if (renderMode)
	{
		renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameLocal.renderPlatform.bloomMaterial);

		// Render the SSAO to a render target so we can blur it.
		//renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameLocal.renderPlatform.ssaoMaterial);

		//renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 1.0f, 1.0f, 0.0f, gameLocal.renderPlatform.ssaoBlurMaterial);
	}

	renderSystem->EndFrame(&frontEnd, &backEnd);
}

/*
======================
idGameEdit::AllocEditorEntity
======================
*/
dnEditorEntity* idGameEdit::AllocEditorEntity(idRenderWorld* editorRenderWorld, dnEditorEntityType_t editorEntityType) {
	switch (editorEntityType)
	{
		case EDITOR_ENTITY_GENERIC:
			return new dnEditorEntity(editorRenderWorld);

		case EDITOR_ENTITY_LIGHT:
			return new dnEditorLight(editorRenderWorld);

		case EDITOR_ENTITY_MODEL:
			return new dnEditorModel(editorRenderWorld);
	}
	return nullptr;
}

/*
======================
idGameEdit::FreeEditorEntity
======================
*/
void idGameEdit::FreeEditorEntity(dnEditorEntity* entity) {
	delete entity;
}

/*
======================
idGameEdit::DrawSelectedGizmo
======================
*/
void dnEditorEntity::DrawSelectedGizmo(idVec3 origin) {
	editorRenderWorld->DebugArrow(colorRed, origin, origin + idVec3(0, 0, 75), 25);
	editorRenderWorld->DebugArrow(colorGreen, origin, origin + idVec3(0, 75, 0), 25);
	editorRenderWorld->DebugArrow(colorBlue, origin, origin + idVec3(75, 0, 0), 25);
}

/*
======================
idGameEdit::Render
======================
*/
void dnEditorEntity::Render(idDict& spawnArgs, bool isSelected, const renderView_t& renderView) {
	idVec3 maxs = spawnArgs.GetVector("editor_maxs");
	idVec3 mins = spawnArgs.GetVector("editor_mins");

	idVec3 origin = spawnArgs.GetVector("origin");

	idStr name = spawnArgs.GetString("name");
	if (name == "")
	{
		name = spawnArgs.GetString("classname");
	}

	float angle = spawnArgs.GetFloat("angle");
	idVec3 forward = idAngles(0.0f, angle, 0.0f).ToForward();

	idVec3 midpoint = origin + idVec3(0, 0, maxs.z * 0.5f);
	editorRenderWorld->DebugArrow(colorWhite, midpoint, midpoint + (forward * 50.0f), 20.0f);

	editorRenderWorld->DebugBounds(colorWhite, idBounds(mins, maxs), origin, 0, true);
	editorRenderWorld->DrawText(name, origin + idVec3(0, 0, maxs.z), 0.5f, colorWhite, renderView.viewaxis, 1, 0, true);
}
