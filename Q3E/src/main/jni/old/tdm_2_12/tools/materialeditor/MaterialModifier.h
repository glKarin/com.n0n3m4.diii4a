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

class MaterialDocManager;
class MaterialTreeView;

/**
* Base class for modifications that can be made to a material that can be undone and redone.
*/
class MaterialModifier {

public:
	MaterialModifier(MaterialDocManager* manager, const char* materialName);
	virtual ~MaterialModifier() {};

	virtual void			Undo() = 0;
	virtual void			Redo() = 0;

protected:
	MaterialDocManager*		manager;
	idStr					materialName;
};

/**
* Base class for Undo/Redo operations for attribute changes
*/
class AttributeMaterialModifier : public MaterialModifier {

public:
	AttributeMaterialModifier(MaterialDocManager* manager, const char* materialName, int stage, const char* key);
	virtual ~AttributeMaterialModifier() {};

	virtual void 			Undo() = 0;
	virtual void 			Redo() = 0;

protected:
	int						stage;
	idStr					key;
};

/**
* Undo/Redo operation for string attribute changes
*/
class AttributeMaterialModifierString : public AttributeMaterialModifier {

public:
	AttributeMaterialModifierString(MaterialDocManager* manager, const char* materialName, int stage, const char* key, const char* value, const char* oldValue);
	virtual ~AttributeMaterialModifierString() {};

	virtual void 			Undo();
	virtual void 			Redo();

protected:
	idStr					value;
	idStr					oldValue;

};

/**
* Undo/Redo operation for boolean attribute changes
*/
class AttributeMaterialModifierBool : public AttributeMaterialModifier {

public:
	AttributeMaterialModifierBool(MaterialDocManager* manager, const char* materialName, int stage, const char* key, bool value, bool oldValue);
	virtual ~AttributeMaterialModifierBool() {};

	virtual void 			Undo();
	virtual void 			Redo();

protected:
	bool					value;
	bool					oldValue;

};

/**
* Undo/Redo operation for stage moves
*/
class StageMoveModifier : public MaterialModifier {

public:
	StageMoveModifier(MaterialDocManager* manager, const char* materialName, int from, int to);
	virtual ~StageMoveModifier() {};

	virtual void 			Undo();
	virtual void 			Redo();

protected:
	int						from;
	int						to;
};

/**
* Undo/Redo operation for stage deletes
*/
class StageDeleteModifier : public MaterialModifier {
public:
	StageDeleteModifier(MaterialDocManager* manager, const char* materialName, int stageNum, idDict stageData);
	virtual ~StageDeleteModifier() {};

	virtual void 			Undo();
	virtual void 			Redo();

protected:
	int						stageNum;
	idDict					stageData;
};

/**
* Undo/Redo operation for stage inserts
*/
class StageInsertModifier : public MaterialModifier {
public:
	StageInsertModifier(MaterialDocManager* manager, const char* materialName, int stageNum, int stageType, const char* stageName);
	virtual ~StageInsertModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	int						stageNum;
	int						stageType;
	idStr					stageName;	
};

/**
* Undo/Redo operation for adding materials
*/
class AddMaterialModifier : public MaterialModifier {
public:
	AddMaterialModifier(MaterialDocManager* manager, const char* materialName, const char* materialFile);
	virtual ~AddMaterialModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	idStr					materialFile;
};

/**
* Undo/Redo operation for deleting materials
*/
class DeleteMaterialModifier : public MaterialModifier {
public:
	DeleteMaterialModifier(MaterialDocManager* manager, const char* materialName);
	virtual ~DeleteMaterialModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	
};

/**
* Undo/Redo operation for moving materials
*/
class MoveMaterialModifier : public MaterialModifier {
public:
	MoveMaterialModifier(MaterialDocManager* manager, const char* materialName, const char* materialFile, const char* copyMaterial);
	virtual ~MoveMaterialModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	idStr					materialFile;
	idStr					copyMaterial;
};

/**
* Undo/Redo operation for renaming materials
*/
class RenameMaterialModifier : public MaterialModifier {
public:
	RenameMaterialModifier(MaterialDocManager* manager, const char* materialName, const char* oldName);
	virtual ~RenameMaterialModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	idStr					oldName;
};

/**
* Undo/Redo operation for adding material folders
*/
class AddMaterialFolderModifier : public MaterialModifier {
public:
	AddMaterialFolderModifier(MaterialDocManager* manager, const char* materialName, MaterialTreeView* view, HTREEITEM item, HTREEITEM parent);
	virtual ~AddMaterialFolderModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	MaterialTreeView*		view;
	HTREEITEM				item;
	HTREEITEM				parent;
};

/**
* Undo/Redo operation for renaming a material folder
*/
class RenameMaterialFolderModifier : public MaterialModifier {
public:
	RenameMaterialFolderModifier(MaterialDocManager* manager, const char* materialName, MaterialTreeView* view, HTREEITEM item, const char* oldName);
	virtual ~RenameMaterialFolderModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	MaterialTreeView*		view;
	HTREEITEM				item;
	idStr					oldName;
};

/**
* Undo/Redo operation for deleting a material folder
*/
class DeleteMaterialFolderModifier : public MaterialModifier {
public:
	DeleteMaterialFolderModifier(MaterialDocManager* manager, const char* materialName, MaterialTreeView* view, HTREEITEM parent, idStrList* affectedMaterials);
	virtual ~DeleteMaterialFolderModifier() {};

	virtual void			Undo();
	virtual void			Redo();

protected:
	MaterialTreeView*		view;
	idStrList				affectedMaterials;

	HTREEITEM				item;
	HTREEITEM				parent;
};

