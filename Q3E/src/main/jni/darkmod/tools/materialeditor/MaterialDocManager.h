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

#include "MaterialEditor.h"
#include "MaterialModifier.h"
#include "MaterialDoc.h"

class MaterialView;

#define MAX_UNDOREDO 32

/**
* Responsible for managing the materials that are being viewed and/or edited.
*/
class MaterialDocManager {

public:
	MaterialDocManager(void);
	~MaterialDocManager(void);

	//Reg/UnReg Material Views
	void			RegisterMaterialView(MaterialView* view);
	void			UnRegisterMaterialView(MaterialView* view);
	void			UnRegisterAllMaterialViews();

	//Material Selection
	void			SetSelectedMaterial(idMaterial* material);
	MaterialDoc*	GetCurrentMaterialDoc() { return currentMaterial; };
	
	//State Checking Methods
	bool			DoesFileNeedApply(const char* filename);
	bool			DoesAnyNeedApply();
	bool			IsFileModified(const char* filename);
	bool			IsAnyModified();

	//Adding or deleting a material
	void			AddMaterial(const char* name, const char* filename, const char* sourceText = NULL, bool addUndo = true);
	void			RedoAddMaterial(const char* name, bool clearData = true);
	void			DeleteMaterial(MaterialDoc* material, bool addUndo = true);

	//Applying
	void			ApplyMaterial(MaterialDoc* materialDoc);
	void			ApplyFile(const char* filename);
	void			ApplyAll();

	//Saving
	void			SaveMaterial(MaterialDoc* material);
	void			SaveFile(const char* filename);
	void			SaveAllMaterials();

	//File Reloading
	void			ReloadFile(const char *filename);


	//Used to get and/or create a MaterialDoc object for editing
	MaterialDoc* 	CreateMaterialDoc(const char* materialName);
	MaterialDoc* 	CreateMaterialDoc(idMaterial* material);
	MaterialDoc* 	GetInProgressDoc(idMaterial* material);

	//Copy Paste
	void			CopyMaterial(MaterialDoc* materialDoc = NULL, bool cut = false);
	void			ClearCopy();
	bool			IsCopyMaterial();
	idStr			GetCopyMaterialName();
	void			PasteMaterial(const char* name, const char* filename);

	void			CopyStage(MaterialDoc* materialDoc, int stageNum);
	void			ClearCopyStage();
	bool			IsCopyStage();
	void			PasteStage(MaterialDoc* materialDoc);
	void			GetCopyStageInfo(int& type, idStr& name);

	//Undo/Redo
	void			Undo();
	bool			IsUndoAvailable();
	void			ClearUndo();
	void			Redo();
	bool			IsRedoAvailable();
	void			ClearRedo();
	void			AddMaterialUndoModifier(MaterialModifier* mod, bool clearRedo = true);
	void			AddMaterialRedoModifier(MaterialModifier* mod);

	//Searching
	bool			FindMaterial(const char* name, MaterialSearchData_t* searchData, bool checkName);

	//Misc
	idStr			GetUniqueMaterialName(idStr name);
	
protected:
	
	/**
	* View notification types
	*/
	enum {
		SELECTION_CHANGE,
		MATERIAL_CHANGE,
		MATERIAL_APPLY,
		MATERIAL_SAVE,
		MATERIAL_SAVE_FILE,
		MATERIAL_ADD,
		MATERIAL_DELETE,
		MATERIAL_ADD_STAGE,
		MATERIAL_DELETE_STAGE,
		MATERIAL_MOVE_STAGE,
		MATERIAL_ATTRIBUTE_CHANGE,
		MATERIAL_NAME_CHANGE,
		FILE_RELOAD
	};
	void			NotifyViews(MaterialDoc* materialDoc, int notifyType, ... );

	//Doc Notification members
	friend	MaterialDoc;
	void			MaterialChanged(MaterialDoc* materialDoc);
	void			MaterialApplied(MaterialDoc* materialDoc);
	void			MaterialSaved(MaterialDoc* materialDoc);
	void			MaterialNameChanged(const char* oldName, MaterialDoc* materialDoc);
	void			StageAdded(MaterialDoc* materialDoc, int stageNum);
	void			StageDeleted(MaterialDoc* materialDoc, int stageNum);
	void			StageMoved(MaterialDoc* materialDoc, int from, int to);
	void			AttributeChanged(MaterialDoc* materialDoc, int stage, const char* attribName);

protected:
	idList<MaterialView*>		materialViews;
	MaterialDoc*				currentMaterial;
	idHashTable<MaterialDoc*>	inProgressMaterials;

	idList<MaterialModifier*>	undoModifiers;
	idList<MaterialModifier*>	redoModifiers;

	//Copy/Paste
	bool						cutMaterial;
	idStr						copyMaterial;

	//Copy/Paste Stage
	idStr						copyStageMaterial;
	MEStage_t					copyStage;
};