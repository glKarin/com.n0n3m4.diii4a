// EditorModel.cpp
//

#include "../Gamelib/Game_local.h"

/*
======================
dnEditorModel::dnEditorModel
======================
*/
dnEditorModel::dnEditorModel(idRenderWorld* editorRenderWorld) {
	renderEntityHandle = -1;
	this->editorRenderWorld = editorRenderWorld;
}

/*
======================
dnEditorModel::~dnEditorModel
======================
*/
dnEditorModel::~dnEditorModel() {
	if (renderEntityHandle != -1)
	{
		editorRenderWorld->FreeEntityDef(renderEntityHandle);
		renderEntityHandle = -1;
	}
}

/*
======================
dnEditorModel::Render
======================
*/
void dnEditorModel::Render(idDict& spawnArgs, bool isSelected, const renderView_t& renderView) {
	gameLocal.ParseSpawnArgsToRenderEntity(&spawnArgs, &renderEntityParams);

	renderEntityParams.isSelected = isSelected;

	if (isSelected)
	{
		DrawSelectedGizmo(renderEntityParams.origin + renderEntityParams.hModel->Bounds().GetCenter());
	}

	//if (renderEntityParams.origin != vec3_zero)
	//{
	//	editorRenderWorld->DebugBounds(colorGreen, renderEntityParams.bounds, renderEntityParams.origin);
	//}

	if (renderEntityHandle == -1)
	{
		renderEntityHandle = editorRenderWorld->AddEntityDef(&renderEntityParams);
	}
	else
	{
		editorRenderWorld->UpdateEntityDef(renderEntityHandle, &renderEntityParams);
	}
}
