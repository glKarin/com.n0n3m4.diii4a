// EditorLight.cpp
//

#include "../Gamelib/Game_local.h"

/*
===================
dnEditorLight::dnEditorLight
===================
*/
dnEditorLight::dnEditorLight(idRenderWorld* editorRenderWorld) {
	idDict args;
	gameLocal.ParseSpawnArgsToRenderLight(&args, &renderLightParams);
	renderLightHandle = editorRenderWorld->AddLightDef(&renderLightParams);

	this->editorRenderWorld = editorRenderWorld;
}

/*
===================
dnEditorLight::~dnEditorLight
===================
*/
dnEditorLight::~dnEditorLight() {
	if (renderLightHandle != -1)
	{
		editorRenderWorld->FreeLightDef(renderLightHandle);
		renderLightHandle = -1;
	}
}

/*
===============
dnEditorLight::Render
===============
*/
void dnEditorLight::Render(idDict& spawnArgs, bool isSelected, const renderView_t& renderView) {
	gameLocal.ParseSpawnArgsToRenderLight(&spawnArgs, &renderLightParams);

	if (isSelected)
	{
		DrawSelectedGizmo(renderLightParams.origin);
		DrawProjectedLight();
	}

	idStr name = spawnArgs.GetString("name");
	if (name == "")
	{
		name = spawnArgs.GetString("classname");
	}

	idVec4 color(renderLightParams.lightColor.x, renderLightParams.lightColor.y, renderLightParams.lightColor.z, 1);
	editorRenderWorld->DebugBounds(color, idBounds(idVec3(-10, -10, -10), idVec3(10, 10, 10)), renderLightParams.origin, 0, true);
	editorRenderWorld->DrawText(name, renderLightParams.origin + idVec3(0, 0, 20), 0.5f, colorWhite, renderView.viewaxis, 1, 0, true);
	editorRenderWorld->UpdateLightDef(renderLightHandle, &renderLightParams);
}

/*
===============
dnEditorLight::DrawProjectedLight
===============
*/
void dnEditorLight::DrawProjectedLight(void) {
	int		i;
	idVec3	v1, v2, cross, vieworg, edge[8][2], v[4];
	idVec3	target, start;

	// use the renderer to get the volume outline
	idPlane		lightProject[4];
	idPlane		planes[6];
	srfTriangles_t* tri;

	renderSystem->RenderLightFrustum(renderLightParams, planes);

	tri = renderSystem->PolytopeSurface(6, planes, nullptr);

	idVec4 color(renderLightParams.lightColor.x, renderLightParams.lightColor.y, renderLightParams.lightColor.z, 1);
	for (i = 0; i < tri->numIndexes; i += 3) {
		editorRenderWorld->DebugLine(color, tri->verts[tri->indexes[i]].xyz, tri->verts[tri->indexes[i + 1]].xyz);
		editorRenderWorld->DebugLine(color, tri->verts[tri->indexes[i + 1]].xyz, tri->verts[tri->indexes[i + 2]].xyz);
	}

	renderSystem->FreeStaticTriSurf(tri);
}
